// CN运行时实现：内存管理API + IO API（规格书10.1/10.2）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：分配→cn_alloc、释放→cn_free、重新分配→cn_realloc、
//                复制内存→cn_memcpy、置零内存→cn_memset、
//                打印→printLine、打印行→printNoLine（单参数防御路径）
// 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除——
//   打印/打印行 为变参函数，IR 层展开为 __cn_print_*（不换行）序列；
//   printLineInt/printLineFloat 保留（单元测试直接引用 + 防御 ABI 稳定）
#include "runtime/runtime.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
#include <malloc.h>  // _heapmin（堆压缩，归还空闲页）
#include <windows.h>  // ExitProcess/SetUnhandledExceptionFilter（718 崩溃处理器）
#endif

// ==================== 内存管理API（规格书10.2） ====================
//
// 内存管理策略（学习 C++ std::vector / Rust Vec 的成功经验）：
//   cn_alloc / cn_realloc / cn_free 使用 std::malloc / std::realloc / std::free，
//   支持正常的逐块释放（RAII：向量析构时释放数据数组，扩容时 realloc 释放旧块）。
//
//   cn_alloc_tracked 分配时注册到全局链表，供 内存::释放全部() 批量释放。
//   这用于字符串等"原始类型"（非类，无 RAII 析构）的内存管理——
//   CN 字符串拼接产生新串，旧串需显式 字符串释放()，但大规模编译时
//   可能遗漏释放，内存::释放全部() 作为兜底批量释放。
//
//   内存::释放全部() -> __cn_alloc_reset()：遍历 tracked 链表逐个 std::free，
//   然后清空链表。不影响 cn_alloc/cn_realloc 分配的内存（向量数据数组）。

// ---- 分配计数（自举前置 C-3，2026-08：泄漏检测） ----
// 活动分配数/累计分配次数 原子计数（cn_alloc/cn_realloc/cn_free 全路径维护）。
// CN 内置函数 内存::活动分配数()/内存::总分配次数() 查询——容器/字符串
// 释放后计数回落基线验证"无泄漏"，自举源码内存安全底线。
static std::atomic<long long> g_cn_alloc_live{0};   // 当前活动分配数
static std::atomic<long long> g_cn_alloc_total{0};  // 累计分配次数

// ---- 829/863 侦查：累计分配字节直方图（065 内存主体 + 074 中块定位）----
//   口径：累计「分配请求字节」（含 realloc 新尺寸）——allocator 不归还
//   （746-a）→ 进程 RSS ≈ 累计；atexit 打印 stderr。零内置注册（不碰双端表）。
//   863（074 侦查）：16KB 以上按 1KB 粒度分桶（16..256KB）·>256KB 单桶；
//   16KB 以下保留 5 档。纯 atomic 数组（禁 STL 静态容器——851 rc=1 教训）。
static std::atomic<long long> g_binCount[258];
static std::atomic<long long> g_binBytes[258];
static int cnBinOf(std::size_t s) {
    if (s <= 64) return 0;
    if (s <= 256) return 1;
    if (s <= 1024) return 2;
    if (s <= 4096) return 3;
    if (s <= 16384) return 4;
    const std::size_t kb = (s + 1023) / 1024;
    if (kb >= 16 && kb <= 256) return static_cast<int>(kb);
    return 257;
}
static void cnPoolReportLines();   // 074/102 观测点（定义于小对象池段后）

static void cnBinAdd(std::size_t s) {
    const int b = cnBinOf(s);
    ++g_binCount[b];
    g_binBytes[b] += static_cast<long long>(s);
}
// 891：CN_RT_MEM_STATS 环境探测（MSVC /W4 下 getenv=C4996·_dupenv_s 安全版，
//   cn_main getEnvVar 同款；GCC/Clang 用 getenv）——一次探测结果缓存（atexit 时机恒定）。
static bool cnMemStatsEnabled() {
#if defined(_MSC_VER)
    static int cached = -1;
    if (cached < 0) {
        char* v = nullptr;
        std::size_t len = 0;
        cached = (_dupenv_s(&v, &len, "CN_RT_MEM_STATS") == 0 && v != nullptr) ? 1 : 0;
        if (v != nullptr) std::free(v);
    }
    return cached == 1;
#else
    return std::getenv("CN_RT_MEM_STATS") != nullptr;
#endif
}

static void cnBinReport() {
    // 891：观测输出改环境变量门控（默认静默——CN_RT_MEM_STATS=1 打印）。原每次
    //   退出常打印：①与 CLI 契约门禁「程序输出→stdout、7 不入 stderr」格撞数字
    //   （池/直方图计数字面含 7 即红·实测 [cnrt-pool] 块 7 触发）；②E2E 全量每例
    //   stderr 噪声。设施不撤：074/102 内存轮以 env 显式开启（plans/001 附录口径同步）。
    if (!cnMemStatsEnabled()) return;
    std::fprintf(stderr, "[cnrt-alloc-hist] 档位 次数 累计字节\n");
    static const char* lowBinNames[5] = {"<=64B","<=256B","<=1K","<=4K","<=16K"};
    for (int i = 0; i < 5; ++i) {
        std::fprintf(stderr, "[cnrt-alloc-hist] %s %lld %lld\n", lowBinNames[i],
                     (long long)g_binCount[i].load(), (long long)g_binBytes[i].load());
    }
    for (int kb = 16; kb <= 256; ++kb) {
        const long long c = g_binCount[kb].load();
        if (c >= 300) {
            std::fprintf(stderr, "[cnrt-alloc-hist] %dK %lld %lld\n", kb, c,
                         (long long)g_binBytes[kb].load());
        }
    }
    std::fprintf(stderr, "[cnrt-alloc-hist] >256K %lld %lld\n",
                 (long long)g_binCount[257].load(), (long long)g_binBytes[257].load());
    cnPoolReportLines();   // 074/102 观测点：池规模与重复释放拦截计数（定义在小对象池段后）
    std::fflush(stderr);
}
struct CnBinReportAtExit { CnBinReportAtExit() { std::atexit(cnBinReport); } };
static CnBinReportAtExit g_cnBinReportAtExit;

// 829 侦查：累计分配字节查询（直方图总和·供 v2 树阶段差分定位——065 内存主体）
extern "C" long long __cn_alloc_bytes() {
    long long t = 0;
    for (int i = 0; i < 258; ++i) t += static_cast<long long>(g_binBytes[i].load());
    return t;
}

// ==================== 小对象池（074 波3·2026-09-29·[基准=019]） ====================
// 背景（885 侦查③实测归因·v2p 全树编译峰值 1.78GB）：
//   1450 万次 tracked 分配（平均 12 字节）走「malloc + unordered_set/unordered_map
//   双容器注册表」——注册表为**每个对象**挂两个节点（≈16B+24B）并把桶数组撑到
//   2×1678 万（≈268MB），加 CRT 每块 ~32B 头/对齐开销，合计 ≈1.1GB；而对象
//   载荷本身仅 179MB（**簿记成本是载荷的 6 倍**，且分配计数越多越糟）。
// 设计（Rust bumpalo / slab / rustc Arena 同款·【基准=019】「编译器一次性进程」）：
//   · **≤4KB 走定档对象池**：档位 8/16/24/32/48/64…4096（~1.25 倍阶梯·8 字节对齐）；
//     块 64KB 起（档位越大块越大）·块内顺序切分·**每档空闲链复用**（释放即回收，
//     长循环零增长——E2E 216 口径不变）。
//   · **>4KB 仍走 malloc + 注册表**（全树仅 ~900 次，注册表开销可忽略），
//     保留 内存::释放全部() 的批量释放语义。
//   · **指针溯源＝块目录**（按地址升序的块区间数组 + 二分查找 → 档位），
//     **不读对象头**——外部/悬垂/已重置指针绝不触碰内存（安全性与旧注册表等同，
//     且比 malloc 的 free(野指针) 更防御：非槽位对齐的指针一律忽略）。
//   · 计数语义完全不变：分配 ++活动分配数、释放 --（E2E 216 活动分配数差值断言不变）。
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
    unsigned char* bits;    // 槽位分配位图（1=占用 / 0=空闲·每槽 1 bit）
    std::size_t nslots;
};

struct PoolState {
    void* freeHead[kPoolClassCount];      // 每档空闲槽链（槽首 8 字节存 next 指针）
    char* carve[kPoolClassCount];         // 每档当前切分块游标
    std::size_t carveRemain[kPoolClassCount];   // 每档当前块剩余字节
    unsigned char* carveBits[kPoolClassCount];   // 每档当前块位图（malloc 持有·地址稳定）
    char* carveBegin[kPoolClassCount];           // 每档当前块首址（位图下标换算）
    PoolBlock* dir;                       // 块目录（按 begin 升序·二分查找溯源）
    std::size_t dirCount;
    std::size_t dirCap;
    long long live;                       // 池内活动对象数（供重置时扣减）
    long long rejectedFree;               // 防御计数：重复释放/非槽位释放被忽略
};

PoolState g_pool;
// 档位选择：返回档位下标；>4KB 返回 -1（走 malloc）
int poolClassOf(std::size_t size) {
    for (int i = 0; i < kPoolClassCount; ++i) {
        if (size <= kPoolClass[i]) return i;
    }
    return -1;
}

// 位图操作（裸指针版·切分路径用：位图数组由 malloc 独立持有·地址稳定，
//   不受块目录插入排序引起的下标漂移影响——885 实测缺陷：缓存"目录下标"
//   在后续块插入（升序插入排序搬移）后失效 → 位图置错块 → 槽位标志丢失
//   → 释放被误判重复 且 错块槽位被误判占用（假接受→空闲链重复入链→别名→堆损坏））
inline void poolBitSetRaw(unsigned char* bits, std::size_t idx) {
    bits[idx >> 3] = static_cast<unsigned char>(bits[idx >> 3] | (1u << (idx & 7)));
}

// 位图操作（槽位占用标志·双释放/非槽位释放的结构性拦截）
inline bool poolBitGet(const PoolBlock& b, std::size_t idx) {
    return (b.bits[idx >> 3] >> (idx & 7)) & 1u;
}
inline void poolBitSet(const PoolBlock& b, std::size_t idx) {
    b.bits[idx >> 3] = static_cast<unsigned char>(b.bits[idx >> 3] | (1u << (idx & 7)));
}
inline void poolBitClear(const PoolBlock& b, std::size_t idx) {
    b.bits[idx >> 3] = static_cast<unsigned char>(b.bits[idx >> 3] & ~(1u << (idx & 7)));
}

long long poolLocate(const void* ptr, std::size_t* outIdx);   // 前向声明（poolAlloc 复用链置位用）

// 块目录插入（升序·块数量级 ~10^3·插入排序摊销可忽略）；返回下标（失败 -1）
long long poolDirInsert(char* begin, std::size_t bytes, int cls, unsigned char* bits,
                        std::size_t nslots) {
    if (g_pool.dirCount == g_pool.dirCap) {
        const std::size_t cap = g_pool.dirCap == 0 ? 256 : g_pool.dirCap * 2;
        PoolBlock* nd = static_cast<PoolBlock*>(std::realloc(g_pool.dir, cap * sizeof(PoolBlock)));
        if (nd == nullptr) return -1;
        g_pool.dir = nd;
        g_pool.dirCap = cap;
    }
    std::size_t i = g_pool.dirCount;
    while (i > 0 && g_pool.dir[i - 1].begin > begin) {
        g_pool.dir[i] = g_pool.dir[i - 1];
        --i;
    }
    g_pool.dir[i].begin = begin;
    g_pool.dir[i].bytes = bytes;
    g_pool.dir[i].cls = cls;
    g_pool.dir[i].bits = bits;
    g_pool.dir[i].nslots = nslots;
    ++g_pool.dirCount;
    return static_cast<long long>(i);
}

// 新块：每档块大小 = max(64KB, 档位×64)（≤4KB 档最多浪费一个槽位）；
//   返回块字节（0=失败）·*out=块首·*outIdx=目录下标
std::size_t poolNewBlock(int cls, char** out, long long* outIdx) {
    const std::size_t slot = kPoolClass[cls];
    std::size_t bytes = slot * 64;
    const std::size_t minBytes = 64 * 1024;
    if (bytes < minBytes) bytes = minBytes;
    bytes = (bytes + 7) & ~std::size_t(7);
    char* b = static_cast<char*>(std::malloc(bytes));
    if (b == nullptr) return 0;
    const std::size_t nslots = bytes / slot;
    const std::size_t bitBytes = (nslots + 7) / 8;
    unsigned char* bits = static_cast<unsigned char*>(std::calloc(bitBytes, 1));
    if (bits == nullptr) {
        std::free(b);
        return 0;
    }
    const long long idx = poolDirInsert(b, bytes, cls, bits, nslots);
    if (idx < 0) {
        std::free(bits);
        std::free(b);
        return 0;
    }
    *out = b;
    *outIdx = idx;
    return bytes;
}

// 池分配：空闲链 → 当前块切分 → 新块切分；失败返回 nullptr（调用方回退 malloc）
void* poolAlloc(std::size_t size) {
    const int cls = poolClassOf(size);
    if (cls < 0) return nullptr;
    const std::size_t slot = kPoolClass[cls];
    void* head = g_pool.freeHead[cls];
    if (head != nullptr) {
        g_pool.freeHead[cls] = *reinterpret_cast<void**>(head);   // 空闲链复用
        std::size_t idx = 0;
        const long long bi = poolLocate(head, &idx);
        if (bi >= 0) poolBitSet(g_pool.dir[bi], idx);   // 复用即置位（否则下次释放被误判重复）
        ++g_pool.live;
        return head;
    }
    if (g_pool.carveRemain[cls] < slot) {
        char* b = nullptr;
        long long idx = 0;
        const std::size_t bytes = poolNewBlock(cls, &b, &idx);
        if (bytes == 0) return nullptr;
        g_pool.carve[cls] = b;
        g_pool.carveRemain[cls] = bytes;
        g_pool.carveBits[cls] = g_pool.dir[idx].bits;
        g_pool.carveBegin[cls] = b;
    }
    void* p = g_pool.carve[cls];
    const std::size_t newIdx =
        static_cast<std::size_t>(static_cast<char*>(p) - g_pool.carveBegin[cls]) / slot;
    g_pool.carve[cls] += slot;
    g_pool.carveRemain[cls] -= slot;
    poolBitSetRaw(g_pool.carveBits[cls], newIdx);   // 稳定位图指针·不受目录搬移影响
    ++g_pool.live;
    return p;
}

// 溯源：ptr 是否池内槽位起始地址（命中返回块下标·否则 -1·*outIdx=槽位下标）。
//   只读**块目录元数据**，不触碰对象内存（外部/悬垂指针安全）。
long long poolLocate(const void* ptr, std::size_t* outIdx) {
    const char* p = static_cast<const char*>(ptr);
    std::size_t lo = 0, hi = g_pool.dirCount;
    while (lo < hi) {                        // 二分：首个 begin > p 的位置
        const std::size_t mid = (lo + hi) / 2;
        if (g_pool.dir[mid].begin <= p) lo = mid + 1;
        else hi = mid;
    }
    if (lo == 0) return -1;
    const std::size_t bi = lo - 1;
    const PoolBlock& b = g_pool.dir[bi];
    if (p >= b.begin + b.bytes) return -1;
    const std::size_t slot = kPoolClass[b.cls];
    if (static_cast<std::size_t>(p - b.begin) % slot != 0) return -1;   // 非槽位起始：忽略
    const std::size_t idx = static_cast<std::size_t>(p - b.begin) / slot;
    if (idx >= b.nslots) return -1;
    if (outIdx != nullptr) *outIdx = idx;
    return static_cast<long long>(bi);
}

// 档位查询（非池指针返回 -1）
int poolFindClass(const void* ptr) {
    const long long bi = poolLocate(ptr, nullptr);
    return bi < 0 ? -1 : g_pool.dir[bi].cls;
}

// 池释放：位图校验（**双释放/非槽位释放一律忽略**——与旧注册表"未命中即忽略"
//   同款防御，避免空闲链被重复入链污染）→ 入空闲链。
bool poolFree(void* ptr, int cls) {
    std::size_t idx = 0;
    const long long bi = poolLocate(ptr, &idx);
    if (bi < 0) return false;
    const PoolBlock& b = g_pool.dir[bi];
    if (!poolBitGet(b, idx)) {          // 已空闲（重复释放）→ 忽略，不入链
        ++g_pool.rejectedFree;         // 102 验收观测点：重复释放计数（应恒为 0）
        return false;
    }
    poolBitClear(b, idx);
    *reinterpret_cast<void**>(ptr) = g_pool.freeHead[cls];
    g_pool.freeHead[cls] = ptr;
    --g_pool.live;
    return true;
}

// 池重置（内存::释放全部）：全部块归还（位图一并释放）、空闲链清空、目录清空
void poolReset() {
    for (std::size_t i = 0; i < g_pool.dirCount; ++i) {
        std::free(g_pool.dir[i].bits);
        std::free(g_pool.dir[i].begin);
    }
    if (g_pool.dir != nullptr) {
        std::free(g_pool.dir);
        g_pool.dir = nullptr;
    }
    g_pool.dirCount = 0;
    g_pool.dirCap = 0;
    for (int i = 0; i < kPoolClassCount; ++i) {
        g_pool.freeHead[i] = nullptr;
        g_pool.carve[i] = nullptr;
        g_pool.carveRemain[i] = 0;
        g_pool.carveBits[i] = nullptr;
        g_pool.carveBegin[i] = nullptr;
    }
    g_pool.live = 0;
}

} // namespace

// 074/102 观测点：池规模与重复释放拦截计数（102 验收判据＝拦截重复释放 应恒为 0）
static void cnPoolReportLines() {
    std::size_t poolBytes = 0;
    for (std::size_t i = 0; i < g_pool.dirCount; ++i) poolBytes += g_pool.dir[i].bytes;
    std::fprintf(stderr, "[cnrt-pool] 块 %llu 字节 %llu 活动对象 %lld 拦截重复释放 %lld\n",
                 (unsigned long long)g_pool.dirCount, (unsigned long long)poolBytes,
                 g_pool.live, g_pool.rejectedFree);
}

// ---- tracked 分配注册表（供 内存::释放全部() 批量释放）----
// 2026-08-24 结构性加固（78_chain_build 段错误根治）：
//   原实现用 std::malloc 手写链表（TrackedNode），reset/reset 之外的释放组合下
//   易出现重复节点/悬垂节点（78 第三次 reset 段错误 0xC0000005 复现）。
//   现改为 std::unordered_set<void*> 注册表——天然去重、O(1) 增删查、
//   reset 遍历即释放；size 另存 unordered_map 供 cn_realloc_tracked 精确拷贝。
//   学习 C++/Rust 经验：容器管理自己的内存；注册表只是“漏网字符串”兜底。
static std::unordered_set<void*> g_trackedSet;            // tracked 分配集合（去重）
static std::unordered_map<void*, std::size_t> g_trackedSize;  // ptr -> 分配大小

// 注册 tracked 分配（O(1)；重复注册天然去重）
static void trackedRegister(void* ptr, std::size_t size = 0) {
    if (ptr == nullptr) return;
    g_trackedSet.insert(ptr);
    if (size > 0) g_trackedSize[ptr] = size;
}

// 从注册表移除 tracked 分配（cn_free_tracked 调用时）；返回是否存在
static bool trackedUnregister(void* ptr) {
    if (ptr == nullptr) return false;
    const bool existed = g_trackedSet.erase(ptr) > 0;
    g_trackedSize.erase(ptr);
    return existed;
}

// 分配内存：对应CN内置函数 分配（malloc 语义，失败返回nullptr）
extern "C" void* cn_alloc(std::size_t size) {
    void* p = std::malloc(size);
    if (p != nullptr) {
        ++g_cn_alloc_total;
        ++g_cn_alloc_live;
        cnBinAdd(size);   // 829 侦查直方图
    }
    return p;
}

// 释放内存：对应CN内置函数 释放（free 语义）
extern "C" void cn_free(void* ptr) {
    // 074 波3：池化后 defensive 分流——池内指针按池归还（池块非 CRT 块，
    //   直接 std::free 会破坏堆）；其余走原路径。
    if (ptr == nullptr) return;
    const int cls = poolFindClass(ptr);
    if (cls >= 0) {
        if (poolFree(ptr, cls)) --g_cn_alloc_live;
        return;
    }
    --g_cn_alloc_live;
    std::free(ptr);
}

// 重新分配内存：对应CN内置函数 重新分配（realloc 语义）
// 计数规则：空指针=新分配（total++/live++）；size=0=释放（live--）；
//   常规扩容 保持 live 不变（块数不增不减）
extern "C" void* cn_realloc(void* ptr, std::size_t size) {
    // 724：异常大尺寸可观测拒绝（>2GB=发射缺陷信号——垃圾尺寸静默 NULL 会
    //   在调用方解引用时崩·Rust alloc 合同同款：Layout 超限返回分配错误）
    if (size > (1ull << 31)) {
        std::fprintf(stderr, "[cnrt] cn_realloc 拒绝异常尺寸 %llu\n",
                     (unsigned long long)size);
        std::fflush(stderr);
        return nullptr;
    }
    if (ptr == nullptr) {
        void* p = std::realloc(nullptr, size);
        if (p != nullptr) {
            ++g_cn_alloc_total;
            ++g_cn_alloc_live;
            cnBinAdd(size);   // 829 侦查直方图
        }
        return p;
    }
    if (size == 0) {
        --g_cn_alloc_live;
        return std::realloc(ptr, 0);
    }
    cnBinAdd(size);   // 829 侦查直方图（扩容：累计新尺寸）
    // 074 波3：池内指针不可 std::realloc（池块非 CRT 块·误用=堆损坏）——
    //   新分配 + 拷贝（旧档位容量内）+ 归还旧槽；跨族误用由此结构性消除。
    {
        const int cls = poolFindClass(ptr);
        if (cls >= 0) {
            const std::size_t cap = kPoolClass[cls];
            void* np = (size <= kPoolMax) ? poolAlloc(size) : nullptr;
            if (np == nullptr) {
                np = std::malloc(size);
                if (np == nullptr) return nullptr;
                trackedRegister(np, size);
            }
            std::memcpy(np, ptr, cap < size ? cap : size);
            if (poolFree(ptr, cls)) --g_cn_alloc_live;
            ++g_cn_alloc_live;
            return np;
        }
    }
    return std::realloc(ptr, size);
}

// ---- 计数分配辅助（自举前置 C-3，2026-08）----
// 字符串 API（string_api.cpp）等内部 malloc 直调改走 *_tracked，使 活动分配数/
//   总分配次数 覆盖全部动态内存（此前 __cn_str_free 经 cn_free 减计数而分配
//   未加计数 -> 计数为负，泄漏检测失真）。
// tracked 分配注册到全局链表，供 内存::释放全部() 批量释放（兜底防泄漏）。
// 074 波3（2026-09-29·〔基准=019〕）：**≤4KB 走定档对象池**（零逐对象簿记），
//   >4KB 才 malloc + 注册表——实测（885 侦查③）v2p 全树编译 1450 万次 tracked
//   分配中 >4KB 仅 ~900 次，逐对象注册表（双容器节点 + 1678 万桶）占 ~725MB
//   且是 v2p 峰值 1.78GB 的主因；池化后实测 228MB（-87%）。
extern "C" void* cn_alloc_tracked(std::size_t size) {
    void* p = poolAlloc(size);                     // ≤4KB：定档池（零逐对象簿记）
    if (p == nullptr) {
        p = std::malloc(size);                     // 大对象/池失败：malloc + 注册表
        if (p == nullptr) return nullptr;
        trackedRegister(p, size);  // 注册到链表（含 size，供 realloc 精确拷贝）
    }
    ++g_cn_alloc_total;
    ++g_cn_alloc_live;
    cnBinAdd(size);   // 829 侦查直方图
    return p;
}

// 宿主契约面分配（074 波3）：**返回 malloc 内存**（保持「调用方可用 std::free 释放」
//   的宿主 C 契约——string_api 三函数 __cn_format/__cn_str_from_bool/__cn_str_from_uint
//   的结果内存被 C++ 单测以 std::free 释放，池块不可 std::free → 这三个函数保持
//   malloc + 注册表路径）。计数/直方图口径与 cn_alloc_tracked 一致。
//   其余字符串族（子串/连接/复制/大缓冲…）走**池**：CN 层释放一律经
//   字符串释放 → cn_free_tracked（池内分流），契约一致。
//   〔契约演进登记〕全族池化 + 三个宿主契约面函数改走池（须同轮改单测为
//   __cn_str_free）属测试升级（AGENTS §4 须先报备用户）——本轮不动单测。
extern "C" void* cn_alloc_tracked_host(std::size_t size) {
    void* p = std::malloc(size);
    if (p == nullptr) return nullptr;
    trackedRegister(p, size);
    ++g_cn_alloc_total;
    ++g_cn_alloc_live;
    cnBinAdd(size);
    return p;
}

extern "C" void cn_free_tracked(void* ptr) {
    // 安全释放（2026-08-24 加固 + 074 波3 池化）：
    //   ① 池内指针（块目录溯源命中）→ 归还档位空闲链；**位图拦截重复释放**
    //      （槽位已空闲=重复释放 → 忽略，与旧注册表「未命中即忽略」同款防御）；
    //   ② 大对象（注册表命中）→ free；
    //   ③ 其余（reset 后遗留/外部/驻留常量指针）→ **忽略不触碰内存**，杜绝双重释放。
    if (ptr == nullptr) return;
    const int cls = poolFindClass(ptr);
    if (cls >= 0) {
        if (poolFree(ptr, cls)) --g_cn_alloc_live;
        return;
    }
    if (trackedUnregister(ptr)) {
        --g_cn_alloc_live;
        std::free(ptr);
    }
}

extern "C" void* cn_realloc_tracked(void* ptr, std::size_t size) {
    if (ptr == nullptr) {
        return cn_alloc_tracked(size);   // 空指针=新分配（计数/直方图口径与旧实现一致）
    }
    if (size == 0) {
        cn_free_tracked(ptr);            // 池内/大对象分流释放
        return nullptr;
    }
    // 池内指针：档位容量足够则原地复用；否则新分配 + 拷贝 + 归还旧槽
    //   （Rust alloc::realloc 合同同款：容量内复用零拷贝，超档位走搬移）
    const int cls = poolFindClass(ptr);
    if (cls >= 0) {
        const std::size_t cap = kPoolClass[cls];
        if (size <= cap) {
            cnBinAdd(size);
            return ptr;
        }
        void* np = cn_alloc_tracked(size);
        if (np == nullptr) return nullptr;   // 失败：旧块仍有效（调用方自行处理）
        std::memcpy(np, ptr, cap);
        cn_free_tracked(ptr);
        return np;
    }
    if (trackedUnregister(ptr)) {
        // 大对象：std::realloc 原地扩展优先（避免堆碎片——79 实测教训）
        cnBinAdd(size);
        void* new_p = std::realloc(ptr, size);
        if (new_p == nullptr) {
            trackedRegister(ptr, 0);     // 失败：旧块仍有效且恢复注册
            return nullptr;
        }
        trackedRegister(new_p, size);
        return new_p;
    }
    return nullptr;   // 未注册/已重置指针：不触碰（旧实现同款忽略）
}

// 当前活动分配数（未释放块数，泄漏检测基线）
extern "C" long long __cn_alloc_live() {
    return g_cn_alloc_live.load();
}

// RAII 辅助（2026-08-25 方案A，学习 C++ vector<string> 析构释放元素）：
// 释放 向量<字符串> 对象的全部元素字符串。由 IR 层在 向量<字符串> 局部变量
// 析构（DeleteObject）前调用，使字符串随局部向量离开作用域自动清理，
// 降低 78/79 组件链每模块百万级字符串在 reset 前的峰值累积。
// 参数：obj=向量对象指针；dataOffset/countOffset=数据指针/元素数量 字段偏移
//   （IR 层经 classFieldOffset 编译期算出，避免硬编码布局）。
// 语义：数据数组元素是 tracked 字符串（cn_alloc_tracked），逐个 cn_free_tracked
//   （仅"在册"指针释放，reset 后不存在则忽略，天然防 double-free）。
extern "C" void __cn_vector_free_strings(void* obj, long long dataOffset,
                                         long long countOffset) {
    if (obj == nullptr) return;
    char* base = static_cast<char*>(obj);
    char** data = *reinterpret_cast<char***>(base + dataOffset);
    const long long count = *reinterpret_cast<long long*>(base + countOffset);
    if (data == nullptr || count <= 0) return;
    for (long long i = 0; i < count; ++i) {
        if (data[i] != nullptr) {
            cn_free_tracked(data[i]);
            // 74-a（2026-09-11 第七十四轮，「释放+清零」幂等模型，plans/020 移植
            //   纪律 7）：释放后槽清零——多路径（块出口/跳出/函数级兜底/清空）共享
            //   同一元素数组，清零后重复经过的释放点 cn_free_tracked(nullptr) 空安全，
            //   杜绝二次释放（宿主 72-a 与 v2 73-a 同一模型）。
            data[i] = nullptr;
        }
    }
}

// 75-a（2026-09-12 第七十五轮）：映射<K,V> 字符串键/值释放。
//   缺口（探针实证）：~映射/清空 只释放四个内部数组，不释放字符串键/值本身
//   ——每个字符串元素永久泄漏；嵌套场景（向量<映射<…,字符串>> 的元素析构经
//   ~映射）同样由此覆盖。
//   元素模型：映射删除用「与末元素交换」→ 有效元素恒为 [0, 元素数量) → 平铺释放
//   安全（同 向量/栈）。freeKeys/freeValues 由 IR 层按实例化实参 K/V 是否为
//   字符串编译期决定（非 0=释放该侧）。
//   不变量：**释放后槽清零**——清空/析构多路径共享同一键/值数组（清空注入先
//   释放后 元素数量=0，若再析构则 count=0 天然跳过；清零为双保险），
//   cn_free_tracked 对驻留常量（非在册）空安全。
extern "C" void __cn_map_free_strings(void* obj, long long keysOffset,
                                     long long valuesOffset, long long countOffset,
                                     long long freeKeys, long long freeValues) {
    if (obj == nullptr) return;
    if (freeKeys == 0 && freeValues == 0) return;
    char* base = static_cast<char*>(obj);
    char** keys = *reinterpret_cast<char***>(base + keysOffset);
    char** values = *reinterpret_cast<char***>(base + valuesOffset);
    const long long count = *reinterpret_cast<long long*>(base + countOffset);
    if (count <= 0) return;
    for (long long i = 0; i < count; ++i) {
        if (freeKeys != 0 && keys != nullptr && keys[i] != nullptr) {
            cn_free_tracked(keys[i]);
            keys[i] = nullptr;
        }
        if (freeValues != 0 && values != nullptr && values[i] != nullptr) {
            cn_free_tracked(values[i]);
            values[i] = nullptr;
        }
    }
}

// 76-a（2026-09-12 第七十六轮）：映射<K,V> **单槽**字符串释放（删除/覆盖写路径）。
//   缺口（探针 76 实证）：交换式删除把被删项的键/值槽覆盖丢弃（值串泄漏，探针
//   76-A/B）；设置覆盖已有键丢弃旧值串（泄漏，探针 76-C）——两者都是**单槽**
//   释放，不适用全量遍历（被删槽在交换前被释放、覆盖写只换值不换键）。
//   调用方（stdlib）：映射.删除 在链摘除后、与末元素交换**前**调用挂点 析构键值
//   （键+值）；映射.设置 覆盖分支写入新值**前**调用挂点 析构值（仅值，键保留）。
//   freeKeys/freeValues 由 IR 层按实例化 K/V 是否为字符串编译期决定（非 0=释放该侧）。
//   不变量：**释放后槽清零**（与 __cn_map_free_strings 同款）——清零后交换/覆盖
//   写入新句柄；即使未来多路径再次经过本槽，cn_free_tracked(nullptr) 空安全。
//   index 合法性由 stdlib 保证（链查找返回值恒 < 元素数量）；此处仅防御负值。
extern "C" void __cn_map_free_slot(void* obj, long long keysOffset,
                                   long long valuesOffset, long long index,
                                   long long freeKeys, long long freeValues) {
    if (obj == nullptr || index < 0) return;
    if (freeKeys == 0 && freeValues == 0) return;
    char* base = static_cast<char*>(obj);
    if (freeKeys != 0) {
        char** keys = *reinterpret_cast<char***>(base + keysOffset);
        if (keys != nullptr && keys[index] != nullptr) {
            cn_free_tracked(keys[index]);
            keys[index] = nullptr;
        }
    }
    if (freeValues != 0) {
        char** values = *reinterpret_cast<char***>(base + valuesOffset);
        if (values != nullptr && values[index] != nullptr) {
            cn_free_tracked(values[index]);
            values[index] = nullptr;
        }
    }
}

// 76-a（2026-09-12 第七十六轮）：平铺数组容器（向量/栈/集合）**单槽**字符串释放。
//   缺口（探针 76 实证）：向量.删除(位置)（V1：移位浅拷覆盖被删槽）、向量.设置
//   覆盖（V2）、集合.删除(值)（前移覆盖）——被移除/被覆盖槽的串句柄丢失即泄漏。
//   调用方（stdlib 挂点 析构元素(索引)）：向量.删除 移位循环 / 向量.设置 覆盖前 /
//   集合.删除 前移前；IR 层对 T=字符串 实例注入本调用（T=有析构类 走 Call T$析构）。
//   不变量：**释放后槽清零**（同族模型）——清零后移位/覆盖写入新句柄。
//   边界：链式容器（链表 头/尾索引=槽序号，可 >= 元素数量）不做 count 上界检查
//   （index 合法性由 stdlib 保证；仅防御负值——与既有单元素析构注入同口径）。
extern "C" void __cn_seq_free_slot(void* obj, long long dataOffset, long long index) {
    if (obj == nullptr || index < 0) return;
    char* base = static_cast<char*>(obj);
    char** data = *reinterpret_cast<char***>(base + dataOffset);
    if (data == nullptr || data[index] == nullptr) return;
    cn_free_tracked(data[index]);
    data[index] = nullptr;
}

// 607-a（001-001）：结构体元素**字段串**释放（撤守卫态 v2p 注入专用；宿主侧同
//   语义由编译期展开 emitOwnedStrFieldFreesAt 负责，宿主编译器不调用本组符号）。
//   元素=含拥有型串字段结构体时，容器消亡/移除路径逐元素释放字段串：
//     __cn_vector_free_field_strings：平铺容器（向量/栈/集合）全量
//     __cn_chain_free_field_strings：链式容器（链表/队列）全量（链游·83-a 槽复用安全）
//     __cn_seq_free_field_slot：单槽（析构被移除/删除头部/删除尾部·唯一持有者槽）
//   fieldOffset=字段在元素内的偏移（嵌套结构体=内联偏移叠加，编译期收集传入）；
//   多字段=多次调用（每拥有串字段路径一次）。不变量：释放后槽清零（74-a 幂等
//   模型）；cn_free_tracked 对驻留常量（非在册）空安全——字面量驻留借用形态零影响。
extern "C" void __cn_vector_free_field_strings(void* obj, long long dataOffset,
                                               long long countOffset,
                                               long long stride,
                                               long long fieldOffset) {
    if (obj == nullptr || stride <= 0) return;
    char* base = static_cast<char*>(obj);
    char* data = *reinterpret_cast<char**>(base + dataOffset);
    const long long count = *reinterpret_cast<long long*>(base + countOffset);
    if (data == nullptr || count <= 0) return;
    for (long long i = 0; i < count; ++i) {
        char** slot = reinterpret_cast<char**>(data + i * stride + fieldOffset);
        if (*slot != nullptr) {
            cn_free_tracked(*slot);
            *slot = nullptr;
        }
    }
}

extern "C" void __cn_chain_free_field_strings(void* obj, long long dataOffset,
                                              long long nextOffset,
                                              long long headOffset,
                                              long long countOffset,
                                              long long stride,
                                              long long fieldOffset) {
    if (obj == nullptr || stride <= 0) return;
    char* base = static_cast<char*>(obj);
    char* data = *reinterpret_cast<char**>(base + dataOffset);
    long long* next = *reinterpret_cast<long long**>(base + nextOffset);
    const long long head = *reinterpret_cast<long long*>(base + headOffset);
    const long long count = *reinterpret_cast<long long*>(base + countOffset);
    if (data == nullptr || next == nullptr || head < 0 || count <= 0) return;
    long long idx = head;
    for (long long n = 0; n < count; ++n) {
        if (idx < 0) break;
        char** slot = reinterpret_cast<char**>(data + idx * stride + fieldOffset);
        if (*slot != nullptr) {
            cn_free_tracked(*slot);
            *slot = nullptr;
        }
        idx = next[idx];
    }
}

extern "C" void __cn_seq_free_field_slot(void* obj, long long dataOffset,
                                         long long index, long long stride,
                                         long long fieldOffset) {
    if (obj == nullptr || index < 0 || stride <= 0) return;
    char* base = static_cast<char*>(obj);
    char* data = *reinterpret_cast<char**>(base + dataOffset);
    if (data == nullptr) return;
    char** slot = reinterpret_cast<char**>(data + index * stride + fieldOffset);
    if (*slot != nullptr) {
        cn_free_tracked(*slot);
        *slot = nullptr;
    }
}

// 74-a（2026-09-11 第七十四轮）：链式容器（链表/队列）元素串释放。//   链表/队列 为「数组槽 + 下一索引链」模型：有效元素自 头索引 起沿 下一索引 串联，
//   而 出队/删除头部 会把元素**所有权转移给调用方**（元素槽序号可能 < 元素数量）——
//   故不能像 向量/栈 那样按 0..元素数量 平铺释放（会把已转移给调用方的串释放掉 =
//   双释放/悬垂）。此处按链游释放，只释放仍在容器内的元素。
// 参数：obj=容器对象指针；dataOffset/nextOffset/headOffset/countOffset 字段偏移
//   （IR 层经 classFieldOffset 编译期算出）。count 作链游上界防御（链损坏时不无限循环）。
extern "C" void __cn_chain_free_strings(void* obj, long long dataOffset,
                                        long long nextOffset, long long headOffset,
                                        long long countOffset) {
    if (obj == nullptr) return;
    char* base = static_cast<char*>(obj);
    char** data = *reinterpret_cast<char***>(base + dataOffset);
    long long* next = *reinterpret_cast<long long**>(base + nextOffset);
    const long long head = *reinterpret_cast<long long*>(base + headOffset);
    const long long count = *reinterpret_cast<long long*>(base + countOffset);
    if (data == nullptr || next == nullptr || head < 0 || count <= 0) return;
    long long idx = head;
    for (long long n = 0; n < count; ++n) {
        if (idx < 0) break;
        if (data[idx] != nullptr) {
            cn_free_tracked(data[idx]);
            data[idx] = nullptr;   // 幂等：释放后槽清零（同 __cn_vector_free_strings）
        }
        idx = next[idx];
    }
}

// 累计分配次数（吞吐统计）
extern "C" long long __cn_alloc_total() {
    return g_cn_alloc_total.load();
}

// ==================== 进程竞技场（自举前置 C-1，2026-08：一次性进程模式） ====================
// 适配编译器内存模式（长生命周期 + 一次分配）：块链 bump 分配——新块
// 64KB，按 8 字节对齐切分，全部块链入全局链表；内存::竞技场重置() 一次性
// 释放全部块（编译器进程生命周期内无需逐对象释放，杜绝泄漏/双重释放）。
// 线程安全：编译器单线程使用，不做原子保护（块链操作非重入）。

namespace {

struct ArenaBlock {
    ArenaBlock* next;
    std::size_t used;
    std::size_t capacity;
    // 数据区紧随其后（块头 32 字节对齐到 64，数据 8 字节对齐）
};

constexpr std::size_t kArenaBlockSize = 64 * 1024;
constexpr std::size_t kArenaHeaderPad = 64;  // 块头对齐（含 next/used/capacity 后补零）

ArenaBlock* g_arenaHead = nullptr;
long long g_arenaBytes = 0;    // 已分配（含块头）总字节
long long g_arenaBlocks = 0;   // 块数

ArenaBlock* newArenaBlock(std::size_t need) {
    std::size_t cap = (need + kArenaHeaderPad + 7) & ~std::size_t(7);
    if (cap < kArenaBlockSize) cap = kArenaBlockSize;
    ArenaBlock* b = static_cast<ArenaBlock*>(std::malloc(cap));
    if (b == nullptr) return nullptr;
    b->next = nullptr;
    b->used = kArenaHeaderPad;
    b->capacity = cap;
    g_arenaBytes += static_cast<long long>(cap);
    ++g_arenaBlocks;
    return b;
}

} // namespace

// 竞技场分配：块内 bump（8 字节对齐）；当前块不足时链新块
extern "C" void* __cn_arena_alloc(std::size_t size) {
    const std::size_t need = (size + 7) & ~std::size_t(7);  // 8 字节对齐
    if (g_arenaHead == nullptr ||
        g_arenaHead->capacity - g_arenaHead->used < need) {
        ArenaBlock* b = newArenaBlock(need);
        if (b == nullptr) return nullptr;
        b->next = g_arenaHead;
        g_arenaHead = b;
    }
    void* p = reinterpret_cast<char*>(g_arenaHead) + g_arenaHead->used;
    g_arenaHead->used += need;
    return p;
}

// 竞技场重置：一次性释放全部块（分配指针全部失效，编译器进程收尾用）
extern "C" void __cn_arena_reset() {
    ArenaBlock* b = g_arenaHead;
    while (b != nullptr) {
        ArenaBlock* next = b->next;
        std::free(b);
        b = next;
    }
    g_arenaHead = nullptr;
    g_arenaBytes = 0;
    g_arenaBlocks = 0;
}

// 竞技场已分配总字节（含块头）
extern "C" long long __cn_arena_bytes() {
    return g_arenaBytes;
}

// 竞技场块数
extern "C" long long __cn_arena_blocks() {
    return g_arenaBlocks;
}

// 复制内存：对应CN内置函数 复制内存（memcpy 语义）
extern "C" void cn_memcpy(void* dst, const void* src, std::size_t size) {
    std::memcpy(dst, src, size);
}

// 置零内存：对应CN内置函数 置零内存（memset(ptr, 0, size) 语义）
extern "C" void cn_memset(void* dst, std::size_t size) {
    std::memset(dst, 0, size);
}

// ==================== 对象内存辅助（阶段3 Task 3.1，规格书06） ====================

// 新建对象：分配 size 字节堆内存并清零（NewObject 指令展开调用）
// 使用 std::calloc（缺陷3 根治 2026-09-02：原 malloc 不清零——未初始化的
// 类类型字段槽为垃圾指针，LoadPtr 得野指针、级联析构 DeleteObject 崩溃；
// 零化后未构造字段=null，空安全析构跳过、误读报错误码3 而非野指针崩溃。
// 对标 C++ new T() 值初始化 / Rust 保证初始化；大块分配由 OS 零页直供，
// 清零开销可忽略）
// 失败时报错误码4（内存分配失败，规格书附录B）并终止，成功返回对象指针
// 虚表指针初始化由 codegen 负责（对象首地址 8 字节，NewObject 后写入）
// 107（946·008 总攻第九轮）：盒内类值副本摘取辅助——v2 侧盒亡释放免切块
//   路线（块出口条件块切割在 v2 编译期失控·945 实录）。语义：tag 假（错误态）
//   返回 nullptr（值字段=错误码垃圾句柄免疫）；真→摘取值字段句柄+清槽（幂等：
//   多释放点二过=字段已 0→句柄 0→调用方 IR_删除对象 空安全跳过）。调用方以
//   返回句柄发 IR_删除对象(载荷类)（含类析构+free）——对齐宿主 pendingBoxCopies
//   条件 DeleteObject 语义（939· Rust `a = b` drop 旧值对照）。
extern "C" void* __cn_box_class_delete(void* tagAddr, void* fieldAddr) {
    if (tagAddr == nullptr || fieldAddr == nullptr) return nullptr;
    if (*static_cast<int*>(tagAddr) == 0) return nullptr;
    void* handle = *static_cast<void**>(fieldAddr);
    *static_cast<void**>(fieldAddr) = nullptr;
    return handle;
}

extern "C" void* __cn_object_new(long long size) {
    void* ptr = std::calloc(1, static_cast<std::size_t>(size > 0 ? size : 1));
    if (ptr == nullptr) {
        __cn_runtime_error(4);  // 内存分配失败（不返回）
    }
    return ptr;
}

// 删除对象：释放对象内存（DeleteObject 指令展开调用；安全释放 nullptr）
extern "C" void __cn_object_delete(void* ptr) {
    // 074 波3：防御分流——池内指针归还池（std::free 池块=堆损坏）
    if (ptr == nullptr) return;
    const int cls = poolFindClass(ptr);
    if (cls >= 0) {
        if (poolFree(ptr, cls)) --g_cn_alloc_live;
        return;
    }
    std::free(ptr);
}

// ==================== 批量释放（2026-08-24 OOM 修复） ====================
// 内存::释放全部() -> __cn_alloc_reset()
// 遍历 tracked 链表，释放所有未释放的 tracked 内存（字符串等原始类型）。
// 不影响 cn_alloc/cn_realloc 分配的内存（向量数据数组由 RAII 析构管理）。
// 设计参考 C++ 智能指针池和 Rust 的 Drop trait——批量释放仅针对无 RAII 的分配。
extern "C" void __cn_alloc_reset() {
    // 074 波3：池整块归还（块目录遍历 std::free·空闲链/目录清空）——池内全部对象
    //   一次性失效（语义同旧注册表整批释放，且整块归远比逐对象 free 更彻底）。
    g_cn_alloc_live -= g_pool.live;
    poolReset();
    // 自愈式批量释放（2026-08-24 结构性加固）：
    //   unordered_set 遍历释放——天然去重，同一 ptr 只 free 一次（杜绝 double free）；
    //   边遍历边按值收集（set 迭代器不因 free 他人而失效）。
    //   完成后清空注册表；reset 之后旧 ptr 的 unregister/free 走"未找到"忽略。
    //   注意：reset 只应释放"无外部引用"的 tracked 内存（设计语义：组件间清场）。
    // 标识符用 ASCII（待释放）——GCC 9/7 不支持 UTF-8 标识符，中文标识符会中断 Linux 构建
    std::vector<void*> to_free;
    to_free.reserve(g_trackedSet.size());
    for (void* p : g_trackedSet) to_free.push_back(p);
    for (void* p : to_free) {
        if (p != nullptr) {
            --g_cn_alloc_live;
            std::free(p);
        }
    }
    g_trackedSet.clear();
    g_trackedSize.clear();
#ifdef _WIN32
    // 2026-08-25 堆压缩：78/79 组件链连续编译多个模块时，每个模块释放百万级
    //   小对象后 Windows 堆不把空闲页归还 OS——工作集持续攀升（实测 26GB 失控）。
    //   _heapmin() 压缩堆并尽量归还空闲页，使 内存::释放全部() 真正回收内存。
    _heapmin();
#endif
}

// ==================== IO API（规格书10.1，Task 2.9 语义调整） ====================
// 新语义（用户裁决，lessons.md 权重10.4）：
//   打印   = println（自动换行，printf/puts 语义）——printLine
//   打印行 = print（不换行，fputs 语义）——printNoLine
// IR 层统一把 打印/打印行 展开为 __cn_print_* 序列（打印 末尾加 newline），
//   单参数字符串路径（防御保留）经 codegen symbolName 映射到 printLine/printNoLine。

// 打印（字符串）：对应CN内置函数 打印（println 语义，自动换行）
extern "C" void printLine(const char* text) {
    std::puts(text);
}

// 打印行（字符串）：对应CN内置函数 打印行（print 语义，不换行）
extern "C" void printNoLine(const char* text) {
    if (text == nullptr) text = "";
    std::fputs(text, stdout);
}

// 打印（布尔）：对应CN内置函数 打印 布尔参数（输出 真/假，不换行）
// 2026-08（用户裁决，布尔打印统一）：布尔表达式（比较/逻辑结果）与 布尔 值
//   经 打印 输出统一为 真/假（此前 i1 走 __cn_print_int 输出 1/0，与
//   字符串拼接的 布尔转字符串（真/假）不一致）
extern "C" void __cn_print_bool(bool value) {
    std::fputs(value ? "真" : "假", stdout);
}

// 打印（整数）：对应CN内置函数 打印整数（printf "%lld\n" 语义）
extern "C" void printLineInt(long long value) {
    std::printf("%lld\n", value);
}

// 打印（浮点数）：对应CN内置函数 打印浮点（printf "%f\n" 语义）
extern "C" void printLineFloat(double value) {
    std::printf("%f\n", value);
}

// ==================== 运行时错误（规格书附录B错误码，Task 2.4） ====================

// 错误码 -> 错误消息文本（不终止进程；供测试与诊断直接验证消息表）
// 错误码（规格书附录B）：1=除零、2=数组越界、3=空指针解引用；
//   阶段3（Task 3.5）扩展：4=内存分配失败、5=文件打开失败、6=无效参数、
//   7=资源未初始化、8=溢出
extern "C" const char* __cn_error_message(long long errorCode) {
    switch (errorCode) {
        case 1: return "除零错误";
        case 2: return "数组越界";
        case 3: return "空指针解引用";
        case 4: return "内存分配失败";
        case 5: return "文件打开失败";
        case 6: return "无效参数";
        case 7: return "资源未初始化";
        case 8: return "溢出";
        default: return "未知运行时错误";
    }
}

// 运行时错误处理：打印错误信息（含错误码）后终止程序
// 718：终止改 ExitProcess——std::exit 走 atexit/stdio flush 依赖堆锁，堆损坏
//   场景 exit 内部死锁挂住（cn_self 第二跳「错误码 4 后挂住」实测）。
extern "C" void __cn_runtime_error(long long errorCode) {
    std::printf("运行时错误(错误码%lld): %s\n", errorCode, __cn_error_message(errorCode));
    std::fflush(stdout);
    std::fflush(stderr);
#ifdef _WIN32
    ExitProcess(1);
#else
    std::exit(1);
#endif
}

// ==================== 崩溃处理器（718·cn_self 第二跳侦查设施） ====================
// UEF 抓 C0000005 等未处理异常，stderr 直写 code/RIP/RSP/fault（WriteFile 不经
//   stdio 锁）。链接本 obj 的编译器进程经静态初始化自动安装（宿主 cn.exe 无害）。
#ifdef _WIN32
// 719：UEF 显式安装入口（runtime.cpp entry() 调用——cn_self 入口 shim 不跑
//   _initterm，auto_install 静态初始化器不执行，必须显式装）
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS*);
extern "C" void cn_install_crash_handler() {
    SetUnhandledExceptionFilter(cn_crash_filter);
}
static LONG WINAPI cn_crash_filter(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        char buf[256];
        void* fault = (info->ExceptionRecord->NumberParameters >= 2)
                          ? (void*)info->ExceptionRecord->ExceptionInformation[1]
                          : nullptr;
        int n = std::snprintf(buf, sizeof(buf),
                              "[crash] code=%08X addr=%p RIP=%p RSP=%p fault=%p\n",
                              (unsigned)info->ExceptionRecord->ExceptionCode,
                              (void*)info->ExceptionRecord->ExceptionAddress,
                              (void*)info->ContextRecord->Rip,
                              (void*)info->ContextRecord->Rsp, fault);
        if (n > 0) { DWORD written; WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, nullptr); }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
// 719b：VEH 版（第一顺位）——cn_self 的 C0000005 现场 UEF 未触发（原因待查），
//   VEH 挂异常分发链头必经。打印 code/RIP/RSP/fault 后 CONTINUE_SEARCH。
static LONG WINAPI cn_veh_filter(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        char buf[256];
        void* fault = (info->ExceptionRecord->NumberParameters >= 2)
                          ? (void*)info->ExceptionRecord->ExceptionInformation[1]
                          : nullptr;
        // 721：ASLR 下绝对 RIP 不可对位——打印 RVA（RIP-模块基址）
        HMODULE mod = nullptr;
        uintptr_t base = 0;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)info->ExceptionRecord->ExceptionAddress, &mod);
        if (mod) { base = (uintptr_t)mod; }
        // 735：实参寄存器并入（追踪入参 NULL 产出方向——rcx=this/rdx=值形参…）
        int n = std::snprintf(buf, sizeof(buf),
                              "[veh] code=%08X RIP=%p RSP=%p RBP=%p fault=%p base=%p RVA=%llx RAX=%p RCX=%p RDX=%p R8=%p R9=%p\n",
                              (unsigned)info->ExceptionRecord->ExceptionCode,
                              (void*)info->ExceptionRecord->ExceptionAddress,
                              (void*)info->ContextRecord->Rsp,
                              (void*)info->ContextRecord->Rbp, fault, (void*)base,
                              (unsigned long long)(info->ContextRecord->Rip - base),
                              (void*)info->ContextRecord->Rax, (void*)info->ContextRecord->Rcx,
                              (void*)info->ContextRecord->Rdx, (void*)info->ContextRecord->R8,
                              (void*)info->ContextRecord->R9);
        if (n > 0) { DWORD written; WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, nullptr); }
        // 990（171 深水·栈顶转储）：RSP 起 16 个四字——崩点 rdi/rsi=-1 且
        //   返回地址可能被写坏跳入随机符号时 rbp 链不可靠；栈顶原始数据供
        //   对位 map 手工解析真调用链。
        {
            uintptr_t rsp = info->ContextRecord ? info->ContextRecord->Rsp : 0;
            HMODULE mod3 = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)(info->ContextRecord ? info->ContextRecord->Rip : 0), &mod3);
            uintptr_t base3 = mod3 ? (uintptr_t)mod3 : 0;
            for (int q = 0; q < 16 && rsp; q++) {
                uintptr_t v = 0;
                SIZE_T rd = 0;
                if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(rsp + q * 8), &v, 8, &rd) || rd != 8) break;
                char b3[96];
                int n3 = std::snprintf(b3, sizeof(b3), "[veh] stk%02d %p%s", q, (void*)v,
                                       (v > base3 && v < base3 + 0x400000) ? " [mod]" : "");
                if (n3 > 0) { DWORD w; WriteFile(GetStdHandle(STD_ERROR_HANDLE), b3, (DWORD)n3, &w, nullptr); }
            }
        }
    }
    // 721：rbp 链回溯（CN 生成代码 push rbp/mov rbp,rsp 帧链）——打印各层返回地址 RVA
    {
        uintptr_t rbp = info->ContextRecord ? info->ContextRecord->Rbp : 0;
        HMODULE mod2 = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)(info->ContextRecord ? info->ContextRecord->Rip : 0), &mod2);
        uintptr_t base2 = mod2 ? (uintptr_t)mod2 : 0;
        for (int depth = 0; depth < 16 && rbp; depth++) {
            uintptr_t ret = 0, next = 0;
            SIZE_T rd = 0;
            if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(rbp + 8), &ret, 8, &rd) || rd != 8) break;
            ReadProcessMemory(GetCurrentProcess(), (LPCVOID)rbp, &next, 8, &rd);
            char b2[128];
            int n2 = std::snprintf(b2, sizeof(b2), "[veh] frame%02d ret RVA=%llx\n", depth,
                                   (unsigned long long)(ret > base2 ? ret - base2 : ret));
            if (n2 > 0) { DWORD w; WriteFile(GetStdHandle(STD_ERROR_HANDLE), b2, (DWORD)n2, &w, nullptr); }
            if (next <= rbp) break;
            rbp = next;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
extern "C" void cn_install_veh() {
    AddVectoredExceptionHandler(1, cn_veh_filter);
}
namespace {
// 718~735 自举侦查链设施：静态安装 UEF（崩溃时走 cn_crash_filter 输出诊断）。
//   866 清理（0928 审计第 10 条·用户裁决机制级当场修）：撤销启动横幅
//   「[crash] UEF installed」——侦查链已收尾，每程序启动必打的残留污染用户 stderr；
//   安装与崩溃诊断路径保留（诊断契约见 plans/001 §10.2a）。
struct cn_crash_auto_install {
    cn_crash_auto_install() {
        SetUnhandledExceptionFilter(cn_crash_filter);
    }
};
static const cn_crash_auto_install cn_crash_auto_install_instance;
}
#endif
