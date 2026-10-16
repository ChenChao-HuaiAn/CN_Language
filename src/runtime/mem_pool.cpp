// CN 运行时：小对象池（074 波3 立·2026-09-29·[基准=019]；945 并发安全改造·2026-10-16）
//
// 背景（885 侦查③实测归因）：v2p 全树编译 1450 万次 tracked 分配（平均 12 字节）
//   走「malloc + 注册表」合计 ≈1.1GB 簿记；074 波3 改为「≤4KB 定档对象池」后
//   实测峰值 228MB（-87%）。设计（Rust bumpalo / slab / rustc Arena 同款）：
//   · 档位 8/16/24/32/48/64…4096（~1.25 倍阶梯·8 字节对齐）；块 64KB 起；
//     块内顺序切分·每档空闲链复用（释放即回收·长循环零增长）。
//   · >4KB 仍走 malloc + 注册表（mem_api.cpp）。
//   · 指针溯源=块目录二分，不读对象头（外部/悬垂/已重置指针绝不触碰内存）。
//
// 945（2026-10-16·P1）——无锁并发分配数据竞争根治（两线程独立对象构造/赋值/
//   析构 = rc=139 段错误·C8 探针 8/8 复现）：
//   病根=074 波3 的 g_pool 全局无锁（freeHead/carve/dir/live）+ tracked 注册表
//   （unordered_set/map 无锁）被多线程并发读写。改造采用**线程局部快路径 +
//   全局锁慢路径**（对标 Rust/jemalloc/tcmalloc 的 per-thread arena）：
//     · 快路径（分配/释放热路径）全线程局部：TLS 自由链 + TLS 切分游标 +
//       本线程块目录——无锁、无原子、无锁前缀指令 = **单线程零税**；
//     · 慢路径（全局 std::mutex·g.mu）只承担：新块创建（每块一次·64KB 级）、
//       跨线程释放（罕见）、中央链消费、线程退出回收、reset、观测报告；
//     · tracked 注册表（>4KB 大对象）在 mem_api.cpp 单独加锁（v2p 全树仅 ~900 次）。
//   **独占不变量**（正确性基石·无原子化的依据）：每个槽在任意时刻只属于一个
//   「所有者」——TLS 链（本线程）/中央链（锁）/用户手里；位图按**每槽 1 字节**
//   存放（945 由 1bit/槽 改 1B/槽——字节粒度独占：只有槽的当前所有者读写该
//   字节，不同线程操作不同槽=不同字节，无数据竞争；同槽并发=用户 double free
//   UB，仅退化该 UB 的防御窗口，合法程序全防御保持）。
//   **TLS 链不变量**：TLS 链上的槽，其块必属本线程（入链=本线程释放的本线程块
//   槽；中央链的槽不经 TLS 链——慢路径直接返回/孤儿搬回中央链）→ 分配弹出置位
//   只需本线程目录二分。跨线程释放的槽经中央链流转：任意线程弹出复用（锁内
//   全局置位），此后每次「非属主释放」重回中央链、「属主释放」回属主 TLS 链。
//   单线程路径（v2p 编译主场景）= 100% 快路径（块创建每 64KB 一次锁）——
//   与原全局无锁实现在单线程下同阶（阈值实测见 945 提交记录）。
#include "runtime/mem_pool.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>

namespace cnrt {
namespace {

constexpr int kPoolClassCount = 30;
constexpr std::size_t kPoolMax = 4096;   // 池上限（超过走 malloc + 注册表）
// 档位（载荷字节·8 字节对齐·约 1.25 倍阶梯）
const std::size_t kPoolClass[kPoolClassCount] = {
    8, 16, 24, 32, 48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,
    384, 448, 512, 640, 768, 896, 1024, 1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096};

struct PoolBlock {
    char* begin;
    std::size_t bytes;
    int cls;
    unsigned char* bits;    // 槽位分配表（每槽 1 字节：1=占用 / 0=空闲；945 字节粒度
                            //   独占——槽的当前所有者单独读写，无需原子/锁）
    std::size_t nslots;
};

// 线程局部池状态（945：原全局 g_pool 拆分——快路径状态全私有）
struct ThreadPool {
    PoolBlock* dir = nullptr;                    // 本线程创建的块（按 begin 升序·锁内增长）
    std::size_t dirCount = 0;
    std::size_t dirCap = 0;
    void* freeHead[kPoolClassCount] = {};        // TLS 自由链（只放本线程块的槽·不变量）
    char* carve[kPoolClassCount] = {};           // 当前切分块游标（每线程独立）
    std::size_t carveRemain[kPoolClassCount] = {};
    unsigned char* carveBits[kPoolClassCount] = {};   // 切分块位图（稳定指针·885 教训）
    char* carveBegin[kPoolClassCount] = {};
    bool dead = false;              // 线程已退出（观测标记；dir 保留=孤儿池供跨线程释放溯源）
    ThreadPool* nextInGlobal = nullptr;
};

// 全局状态（945：慢路径专用——新块/跨线程释放/线程退出/reset/观测）
struct PoolGlobal {
    std::mutex mu;
    ThreadPool* pools = nullptr;                  // 全部线程池链（活跃 + 孤儿）
    void* centralHead[kPoolClassCount] = {};      // 跨线程释放的槽（每档一条·锁保护）
    std::atomic<long long> rejectedFree{0};       // 重复释放拦截计数（102 观测点·仅拒绝路径 ++）
    long long blockCount = 0;                     // 块数（报告用·全部访问在锁内）
    long long blockBytes = 0;                     // 块字节（报告用·全部访问在锁内）
};

// 全局单例（**永不析构**·进程级设施：规避静态析构时序——851 rc=1 教训；
//   magic static 首次初始化线程安全（C++11 起保证））
PoolGlobal& poolGlobal() {
    static PoolGlobal* g = new PoolGlobal();
    return *g;
}

// 内部不变量违反（不可达：实现缺陷显形·110 教训「miss 不可静默」）——
//   报内部错误（不静默取默认）；调用方走显式哨兵分流
void poolInvariantFail(const char* what) {
    std::fprintf(stderr, "[cnrt] 内部错误：小对象池不变量违反（%s·945）\n", what);
    std::fflush(stderr);
}

// 线程退出（thread_local 析构）：TLS 链空闲槽搬中央链（拿锁复用）+ 标记 dead。
//   池对象与目录**保留**（孤儿池）——他线程仍可能释放「本线程此前援出」的槽，
//   溯源必须可见（槽入中央链后由任意线程复用/最终 reset 统一归还）。
struct ThreadPoolHolder {
    ThreadPool* pool = nullptr;
    ~ThreadPoolHolder() {
        if (pool == nullptr) return;
        PoolGlobal& g = poolGlobal();
        std::lock_guard<std::mutex> lk(g.mu);
        for (int c = 0; c < kPoolClassCount; ++c) {
            void* head = pool->freeHead[c];
            if (head == nullptr) continue;
            void* last = head;
            while (*reinterpret_cast<void**>(last) != nullptr) {
                last = *reinterpret_cast<void**>(last);
            }
            *reinterpret_cast<void**>(last) = g.centralHead[c];
            g.centralHead[c] = head;
            pool->freeHead[c] = nullptr;
        }
        pool->dead = true;
    }
};
thread_local ThreadPoolHolder t_holder;

// 当前线程池：首次调用创建并挂号（magic 时机=首次池分配）
ThreadPool* poolThread() {
    if (t_holder.pool != nullptr) return t_holder.pool;
    ThreadPool* tp = new (std::nothrow) ThreadPool();
    if (tp == nullptr) return nullptr;      // 失败：调用方回退 malloc（同池失败口径）
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    tp->nextInGlobal = g.pools;
    g.pools = tp;
    t_holder.pool = tp;
    return tp;
}

// 档位选择：返回档位下标；>4KB 返回 -1（走 malloc）
int poolClassOf(std::size_t size) {
    for (int i = 0; i < kPoolClassCount; ++i) {
        if (size <= kPoolClass[i]) return i;
    }
    return -1;
}

// 目录内二分溯源：命中返回块下标并写 *outIdx；否则 -1。只读 **块目录元数据**，
//   不触碰对象内存（外部/悬垂指针安全）。不缓存目录下标（885 教训：目录插入
//   搬移会使缓存失效——位图指针由 malloc 独立持有·地址稳定）。
long long poolLocateInDir(const PoolBlock* dir, std::size_t count,
                          const void* ptr, std::size_t* outIdx) {
    const char* p = static_cast<const char*>(ptr);
    std::size_t lo = 0, hi = count;
    while (lo < hi) {                        // 二分：首个 begin > p 的位置
        const std::size_t mid = (lo + hi) / 2;
        if (dir[mid].begin <= p) lo = mid + 1;
        else hi = mid;
    }
    if (lo == 0) return -1;
    const std::size_t bi = lo - 1;
    const PoolBlock& b = dir[bi];
    if (p >= b.begin + b.bytes) return -1;
    const std::size_t slot = kPoolClass[b.cls];
    if (static_cast<std::size_t>(p - b.begin) % slot != 0) return -1;   // 非槽位起始：忽略
    const std::size_t idx = static_cast<std::size_t>(p - b.begin) / slot;
    if (idx >= b.nslots) return -1;
    if (outIdx != nullptr) *outIdx = idx;
    return static_cast<long long>(bi);
}

// 全局溯源（**锁内**）：遍历全部线程池（活跃 + 孤儿）目录二分——跨线程释放/
//   中央链复用定位用。dir 数组的增长（realloc）同样在锁内（poolNewBlockLocked），
//   与本函数互斥；任何调用点不得缓存 PoolBlock* 跨锁使用。
bool poolLocateGlobalLocked(const void* ptr, int* outCls, unsigned char** outBits,
                            std::size_t* outIdx) {
    PoolGlobal& g = poolGlobal();
    for (ThreadPool* tp = g.pools; tp != nullptr; tp = tp->nextInGlobal) {
        std::size_t idx = 0;
        const long long bi = poolLocateInDir(tp->dir, tp->dirCount, ptr, &idx);
        if (bi < 0) continue;
        const PoolBlock& b = tp->dir[bi];
        if (outCls != nullptr) *outCls = b.cls;
        if (outBits != nullptr) *outBits = b.bits;
        if (outIdx != nullptr) *outIdx = idx;
        return true;
    }
    return false;
}

// 新块（**锁内**）：块大小 = max(64KB, 档位×64)；位图=每槽 1 字节（calloc 清零）。
//   返回块字节（0=失败）·*out=块首·*outBits=位图（稳定指针）。
std::size_t poolNewBlockLocked(ThreadPool* tp, int cls, char** out,
                               unsigned char** outBits) {
    const std::size_t slot = kPoolClass[cls];
    std::size_t bytes = slot * 64;
    const std::size_t minBytes = 64 * 1024;
    if (bytes < minBytes) bytes = minBytes;
    bytes = (bytes + 7) & ~std::size_t(7);
    char* b = static_cast<char*>(std::malloc(bytes));
    if (b == nullptr) return 0;
    const std::size_t nslots = bytes / slot;
    unsigned char* bits = static_cast<unsigned char*>(std::calloc(nslots, 1));
    if (bits == nullptr) {
        std::free(b);
        return 0;
    }
    if (tp->dirCount == tp->dirCap) {
        const std::size_t cap = tp->dirCap == 0 ? 256 : tp->dirCap * 2;
        PoolBlock* nd = static_cast<PoolBlock*>(std::realloc(tp->dir, cap * sizeof(PoolBlock)));
        if (nd == nullptr) {
            std::free(bits);
            std::free(b);
            return 0;
        }
        tp->dir = nd;
        tp->dirCap = cap;
    }
    std::size_t i = tp->dirCount;            // 升序插入（块数量级 ~10^3·插入排序摊销可忽略）
    while (i > 0 && tp->dir[i - 1].begin > b) {
        tp->dir[i] = tp->dir[i - 1];
        --i;
    }
    tp->dir[i].begin = b;
    tp->dir[i].bytes = bytes;
    tp->dir[i].cls = cls;
    tp->dir[i].bits = bits;
    tp->dir[i].nslots = nslots;
    ++tp->dirCount;
    PoolGlobal& g = poolGlobal();
    ++g.blockCount;
    g.blockBytes += static_cast<long long>(bytes);
    *out = b;
    *outBits = bits;
    return bytes;
}

// 池补充（**锁内**）：中央链优先（跨线程释放/孤儿回收的槽——内存复用），无货建新块。
//   返回 0=失败（调用方回退 malloc）；1=中央链命中（*out=槽·锁内置位）；2=新块就绪
int poolRefillLocked(ThreadPool* tp, int cls, void** out) {
    *out = nullptr;
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    if (g.centralHead[cls] != nullptr) {
        void* p = g.centralHead[cls];
        int c2 = -1;
        std::size_t idx = 0;
        unsigned char* bits = nullptr;
        if (!poolLocateGlobalLocked(p, &c2, &bits, &idx)) {
            // 不可达（中央链不变量：入链槽的块始终可见）——不静默：缺陷显形；
            //   槽不摘链（留在中央链·状态自洽），调用方回退 malloc 路径
            poolInvariantFail("中央链槽归属丢失");
            return 0;
        }
        g.centralHead[cls] = *reinterpret_cast<void**>(p);
        bits[idx] = 1;                        // 复用置位（槽在本线程手里=字节独占）
        *out = p;
        return 1;
    }
    char* b = nullptr;
    unsigned char* bits = nullptr;
    const std::size_t bytes = poolNewBlockLocked(tp, cls, &b, &bits);
    if (bytes == 0) return 0;
    tp->carve[cls] = b;
    tp->carveRemain[cls] = bytes;
    tp->carveBits[cls] = bits;
    tp->carveBegin[cls] = b;
    return 2;
}

// 慢路径释放（**锁内**）：跨线程释放（他池块）——位图清 0 后槽入中央链。
//   重复释放（位图已 0）拦截（同旧注册表「未命中即忽略」防御）——返回
//   RejectedDouble（调用方不扣活动计数·旧语义逐字对齐）。
PoolFreeResult poolTryFreeSlow(void* ptr) {
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    int cls = -1;
    unsigned char* bits = nullptr;
    std::size_t idx = 0;
    if (!poolLocateGlobalLocked(ptr, &cls, &bits, &idx)) return PoolFreeResult::NotPool;
    if (bits[idx] == 0) {
        g.rejectedFree.fetch_add(1, std::memory_order_relaxed);
        return PoolFreeResult::RejectedDouble;
    }
    bits[idx] = 0;
    *reinterpret_cast<void**>(ptr) = g.centralHead[cls];
    g.centralHead[cls] = ptr;
    return PoolFreeResult::Freed;
}

// 扫描全部池「占用位图=1 的槽数」= 池内活动对象数（**锁内**；reset/报告用——
//   945 以扫描替代旧逐分配 live 计数，把计数维护从热路径整体移除）
long long poolLiveScanLocked() {
    PoolGlobal& g = poolGlobal();
    long long live = 0;
    for (ThreadPool* tp = g.pools; tp != nullptr; tp = tp->nextInGlobal) {
        for (std::size_t i = 0; i < tp->dirCount; ++i) {
            const PoolBlock& b = tp->dir[i];
            for (std::size_t s = 0; s < b.nslots; ++s) {
                if (b.bits[s] != 0) ++live;
            }
        }
    }
    return live;
}

} // namespace

// 池分配（**快路径·无锁**）：TLS 链 → 切分游标 → （冷）锁补充（中央链/新块）
void* poolAlloc(std::size_t size) {
    const int cls = poolClassOf(size);
    if (cls < 0) return nullptr;             // >4KB：调用方走 malloc + 注册表
    ThreadPool* tp = poolThread();
    if (tp == nullptr) return nullptr;
    const std::size_t slot = kPoolClass[cls];
    void* head = tp->freeHead[cls];
    if (head != nullptr) {
        tp->freeHead[cls] = *reinterpret_cast<void**>(head);
        // TLS 链不变量：链上槽的块必属本线程 → 必然命中本线程目录（见文件头证明）
        std::size_t idx = 0;
        const long long bi = poolLocateInDir(tp->dir, tp->dirCount, head, &idx);
        if (bi >= 0) {
            tp->dir[bi].bits[idx] = 1;       // 复用即置位（否则下次释放被误判重复）
        } else {
            // 不可达（不变量违反·实现缺陷）：不静默——报内部错误后槽交还中央链
            //   （位图仍=0=空闲态·状态自洽：不悬垂/不泄漏/不双入链）
            poolInvariantFail("TLS 链槽归属丢失");
            PoolGlobal& g = poolGlobal();
            std::lock_guard<std::mutex> lk(g.mu);
            *reinterpret_cast<void**>(head) = g.centralHead[cls];
            g.centralHead[cls] = head;
            return nullptr;                  // 哨兵分流：调用方回退 malloc（不返回坏槽）
        }
        return head;
    }
    if (tp->carveRemain[cls] < slot) {
        void* fromCentral = nullptr;
        const int rr = poolRefillLocked(tp, cls, &fromCentral);
        if (rr == 0) return nullptr;
        if (rr == 1) return fromCentral;     // 中央链槽（已锁内置位）
    }
    void* p = tp->carve[cls];
    const std::size_t newIdx =
        static_cast<std::size_t>(static_cast<char*>(p) - tp->carveBegin[cls]) / slot;
    tp->carve[cls] += slot;
    tp->carveRemain[cls] -= slot;
    tp->carveBits[cls][newIdx] = 1;          // 稳定位图指针·不受目录搬移影响（885 教训）
    return p;
}

// 池释放分流（**快路径无锁 · 跨线程走慢路径锁**）：
//   返回 Freed=池内槽已归还（调用方扣活动计数）；RejectedDouble=重复释放被
//   拦截（不扣计数·旧语义）；NotPool=未命中（调用方继续自己的释放路径）
PoolFreeResult poolTryFree(void* ptr) {
    if (ptr == nullptr) return PoolFreeResult::NotPool;
    ThreadPool* tp = t_holder.pool;
    if (tp != nullptr) {
        std::size_t idx = 0;
        const long long bi = poolLocateInDir(tp->dir, tp->dirCount, ptr, &idx);
        if (bi >= 0) {
            const PoolBlock& b = tp->dir[bi];
            if (b.bits[idx] == 0) {          // 已空闲（重复释放）→ 忽略不入链
                poolGlobal().rejectedFree.fetch_add(1, std::memory_order_relaxed);
                return PoolFreeResult::RejectedDouble;
            }
            b.bits[idx] = 0;
            *reinterpret_cast<void**>(ptr) = tp->freeHead[b.cls];
            tp->freeHead[b.cls] = ptr;
            return PoolFreeResult::Freed;
        }
    }
    return poolTryFreeSlow(ptr);             // 他线程块 / 孤儿池 / 非池指针
}

// 池指针档位查询（cn_realloc 容量内复用判定用）
int poolClassOfPtr(void* ptr) {
    if (ptr == nullptr) return -1;
    ThreadPool* tp = t_holder.pool;
    if (tp != nullptr) {
        const long long bi = poolLocateInDir(tp->dir, tp->dirCount, ptr, nullptr);
        if (bi >= 0) return tp->dir[bi].cls;
    }
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    int cls = -1;
    if (poolLocateGlobalLocked(ptr, &cls, nullptr, nullptr)) return cls;
    return -1;
}

std::size_t poolClassCapacity(int cls) {
    if (cls < 0 || cls >= kPoolClassCount) return 0;   // 哨兵：调用方分流
    return kPoolClass[cls];
}

// 池重置（内存::释放全部）：全部块归还（位图一并释放）、链/目录清空；
//   返回重置前池内活动对象数（供 g_cn_alloc_live 扣减）。
//   945：覆盖**全部线程池**（含孤儿）——reset 语义前提=无并发池操作（组件间清场）。
long long poolReset() {
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    const long long live = poolLiveScanLocked();
    for (ThreadPool* tp = g.pools; tp != nullptr; tp = tp->nextInGlobal) {
        for (std::size_t i = 0; i < tp->dirCount; ++i) {
            std::free(tp->dir[i].bits);
            std::free(tp->dir[i].begin);
        }
        if (tp->dir != nullptr) {
            std::free(tp->dir);
            tp->dir = nullptr;
        }
        tp->dirCount = 0;
        tp->dirCap = 0;
        for (int c = 0; c < kPoolClassCount; ++c) {
            tp->freeHead[c] = nullptr;
            tp->carve[c] = nullptr;
            tp->carveRemain[c] = 0;
            tp->carveBits[c] = nullptr;
            tp->carveBegin[c] = nullptr;
        }
    }
    for (int c = 0; c < kPoolClassCount; ++c) g.centralHead[c] = nullptr;
    g.blockCount = 0;
    g.blockBytes = 0;
    return live;
}

// 观测报告（atexit·CN_RT_MEM_STATS 门控）：池规模/活动对象数/重复释放拦截计数
void poolReportLines() {
    PoolGlobal& g = poolGlobal();
    std::lock_guard<std::mutex> lk(g.mu);
    const long long live = poolLiveScanLocked();
    std::fprintf(stderr, "[cnrt-pool] 块 %lld 字节 %lld 活动对象 %lld 拦截重复释放 %lld\n",
                 g.blockCount, g.blockBytes, live, g.rejectedFree.load());
}

} // namespace cnrt
