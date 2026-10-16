// CN运行时实现：内存管理API + 对象内存辅助 + 批量释放（规格书10.2）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：分配→cn_alloc、释放→cn_free、重新分配→cn_realloc
// 方案C（2026-08-14）已修复：遗留的 打印行整数/打印行浮点 已删除——
//   打印/打印行 为变参函数，IR 层展开为 __cn_print_*（不换行）序列（IO 侧见 io_api.cpp）；
//   printLineInt/printLineFloat 保留（单元测试直接引用 + 防御 ABI 稳定）
// 349 重构E 自 io_api.cpp 纯机械搬移·零逻辑变化：原 24-784 行（内存管理API+小对象池）
//   与 870-947 行（对象内存辅助+批量释放）——后两节引用本文件级 static
//   （g_cn_alloc_live/cnrt 池接口），按 AGENTS 内聚性原则并入本文件，避免跨文件改链接性。
// 945（2026-10-16·P1）无锁并发分配数据竞争根治：**小对象池整体拆出 mem_pool.cpp**
//   （线程局部快路径 + 全局锁慢路径·设计总纲见该文件头注）；本文件保留 API 层，
//   tracked 注册表（>4KB 大对象）改 std::mutex 保护（945 前为无锁 unordered_set/map，
//   多线程并发 insert/erase 即数据结构破坏 = C8 探针 rc=139 的另一半病根）。
#include "runtime/runtime.hpp"
#include "runtime/mem_pool.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
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
// 074/102 观测点：池规模与重复释放拦截计数（945 移入 cnrt::poolReportLines·mem_pool.cpp）

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
    cnrt::poolReportLines();   // 074/102 观测点：池规模与重复释放拦截计数（mem_pool.cpp）
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

// ==================== 小对象池（074 波3·945 拆出） ====================
// 945（2026-10-16）：原全局无锁 g_pool（freeHead/carve/dir/live）并发数据竞争
//   （C8 探针两线程独立对象 = rc=139）——池实现整体拆出 mem_pool.cpp（线程局部
//   快路径 + 全局锁慢路径·设计总纲见该文件头注）；本文件经 runtime/mem_pool.hpp
//   接口调用：poolAlloc/poolTryFree/poolClassOfPtr/poolClassCapacity/poolReset/
//   poolReportLines。语义面（档位/溯源/重复释放拦截/释放全部整块归还）不变。
// ---- tracked 分配注册表（供 内存::释放全部() 批量释放）----
// 2026-08-24 结构性加固（78_chain_build 段错误根治）：
//   原实现用 std::malloc 手写链表（TrackedNode），reset/reset 之外的释放组合下
//   易出现重复节点/悬垂节点（78 第三次 reset 段错误 0xC0000005 复现）。
//   现改为 std::unordered_set<void*> 注册表——天然去重、O(1) 增删查、
//   reset 遍历即释放；size 另存 unordered_map 供 cn_realloc_tracked 精确拷贝。
//   学习 C++/Rust 经验：容器管理自己的内存；注册表只是“漏网字符串”兜底。

// 945（2026-10-16）：注册表读写整体加锁——945 前为无锁访问，多线程并发
//   insert/erase（各线程字符串字面量副本/大缓冲释放）即容器结构破坏 =
//   C8 探针 rc=139 的另一半病根。注册表只服务 >4KB 大对象 + 宿主契约面
//   （v2p 全树仅 ~900 次）+ reset 清表——锁开销不在小对象热路径上。
//   锁对象与容器同生命周期：堆单例（永不析构）——规避静态析构时序（851 教训）。
struct TrackedTable {
    std::mutex mu;
    std::unordered_set<void*> set;
    std::unordered_map<void*, std::size_t> size;
};
TrackedTable& trackedTable() {
    static TrackedTable* t = new TrackedTable();
    return *t;
}

// 注册 tracked 分配（O(1)；重复注册天然去重）
static void trackedRegister(void* ptr, std::size_t size = 0) {
    if (ptr == nullptr) return;
    TrackedTable& t = trackedTable();
    std::lock_guard<std::mutex> lk(t.mu);
    t.set.insert(ptr);
    if (size > 0) t.size[ptr] = size;
}

// 从注册表移除 tracked 分配（cn_free_tracked 调用时）；返回是否存在
static bool trackedUnregister(void* ptr) {
    if (ptr == nullptr) return false;
    TrackedTable& t = trackedTable();
    std::lock_guard<std::mutex> lk(t.mu);
    const bool existed = t.set.erase(ptr) > 0;
    t.size.erase(ptr);
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
    // 221：双释放侦查对位（CN_RT_DOUBLE_FREE=1 时 alloc/free 全序列打印）
    //   r32b：_dupenv_s=MSVC 专有（linux g++ 编译失败·批预验红实录）——
    //   平台门控对齐 891 cnMemStatsEnabled 先例（GCC/Clang 用 getenv）
#if defined(_MSC_VER)
    static const int dfProbeA = []{ char* v = nullptr; std::size_t len = 0; return _dupenv_s(&v, &len, "CN_RT_DOUBLE_FREE") == 0 && v != nullptr; }();
#else
    static const int dfProbeA = std::getenv("CN_RT_DOUBLE_FREE") != nullptr;
#endif
    if (dfProbeA) {
        std::fprintf(stderr, "[dblalloc] alloc ptr=%p size=%zu\n", p, size);
    }
    return p;
}

// 释放内存：对应CN内置函数 释放（free 语义）
extern "C" void cn_free(void* ptr) {
    // 074 波3：池化后 defensive 分流——池内指针按池归还（池块非 CRT 块，
    //   直接 std::free 会破坏堆）；其余走原路径。
    if (ptr == nullptr) return;
    // 221：双释放可观测（CN_RT_DOUBLE_FREE=1 启用·堆损坏 C374/C5 的
    //   第一现场定位——双 free 时 stderr 打印指针+调用不拦截继续走堆检测）
#if defined(_MSC_VER)
    static const int dfProbe = []{ char* v = nullptr; std::size_t len = 0; return _dupenv_s(&v, &len, "CN_RT_DOUBLE_FREE") == 0 && v != nullptr; }();
#else
    static const int dfProbe = std::getenv("CN_RT_DOUBLE_FREE") != nullptr;
#endif
    if (dfProbe) {
        std::fprintf(stderr, "[dblfree?] free ptr=%p\n", (void*)ptr);
    }
    // 945：池分流（快路径线程局部无锁·跨线程走池慢路径锁）；命中池=已处理。
    //   计数口径逐字对齐旧实现：重复释放拦截（RejectedDouble）不扣活动数。
    const cnrt::PoolFreeResult pr = cnrt::poolTryFree(ptr);
    if (pr == cnrt::PoolFreeResult::Freed) {
        --g_cn_alloc_live;
        return;
    }
    if (pr == cnrt::PoolFreeResult::RejectedDouble) return;
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
    //   945：分流与容量查询走线程安全池接口（>4KB 时 poolAlloc 自返回 nullptr）。
    {
        const int cls = cnrt::poolClassOfPtr(ptr);
        if (cls >= 0) {
            const std::size_t cap = cnrt::poolClassCapacity(cls);
            void* np = cnrt::poolAlloc(size);
            if (np == nullptr) {
                np = std::malloc(size);
                if (np == nullptr) return nullptr;
                trackedRegister(np, size);
            }
            std::memcpy(np, ptr, cap < size ? cap : size);
            if (cnrt::poolTryFree(ptr) == cnrt::PoolFreeResult::Freed) --g_cn_alloc_live;
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
    void* p = cnrt::poolAlloc(size);               // ≤4KB：定档池（零逐对象簿记）
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
    // 安全释放（2026-08-24 加固 + 074 波3 池化 + 945 线程安全）：
    //   ① 池内指针（块目录溯源命中·含跨线程释放慢路径）→ 归还空闲链；**位图
    //      拦截重复释放**（槽位已空闲=重复释放 → 忽略，与旧注册表「未命中即
    //      忽略」同款防御）；
    //   ② 大对象（注册表命中）→ free；
    //   ③ 其余（reset 后遗留/外部/驻留常量指针）→ **忽略不触碰内存**，杜绝双重释放。
    if (ptr == nullptr) return;
    const cnrt::PoolFreeResult pr = cnrt::poolTryFree(ptr);
    if (pr == cnrt::PoolFreeResult::Freed) {
        --g_cn_alloc_live;
        return;
    }
    if (pr == cnrt::PoolFreeResult::RejectedDouble) return;   // 重复释放拦截（不扣计数）
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
    const int cls = cnrt::poolClassOfPtr(ptr);
    if (cls >= 0) {
        const std::size_t cap = cnrt::poolClassCapacity(cls);
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

// 571（2026-10-14·#571）：盒载荷条件深拷辅助——合成体字段（可选<T>/结果<T,E>
//   T=字符串）整体写 deepCopy 面的单调用形态（946 box_class_delete 同族·「条件
//   块切割在编译期失控」同先例）。语义：tag 假（错误/无值态）→ no-op（dst 值位
//   =错误码整数·保留不动）；真 → dst 值位 = __cn_str_copy(src 句柄)（独立副本
//   落堆·源保持拥有）。调用方（emitOwnedStrFieldPostCopy 条件字段分支）的 dst
//   旧值=刚位拷入的共享句柄（非 dst 拥有）直接覆盖丢弃，无需释放。
extern "C" void __cn_box_field_str_copy(void* tagAddr, void* srcFieldAddr,
                                        void* dstFieldAddr) {
    if (tagAddr == nullptr || srcFieldAddr == nullptr || dstFieldAddr == nullptr) {
        return;
    }
    if (*static_cast<int*>(tagAddr) == 0) return;
    *static_cast<void**>(dstFieldAddr) =
        __cn_str_copy(*static_cast<char**>(srcFieldAddr));
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
    // 074 波3：防御分流——池内指针归还池（std::free 池块=堆损坏）；945：线程安全接口
    if (ptr == nullptr) return;
    const cnrt::PoolFreeResult pr = cnrt::poolTryFree(ptr);
    if (pr == cnrt::PoolFreeResult::Freed) {
        --g_cn_alloc_live;
        return;
    }
    if (pr == cnrt::PoolFreeResult::RejectedDouble) return;   // 重复释放拦截（不扣计数）
    std::free(ptr);
}

// ==================== 批量释放（2026-08-24 OOM 修复） ====================
// 内存::释放全部() -> __cn_alloc_reset()
// 遍历 tracked 链表，释放所有未释放的 tracked 内存（字符串等原始类型）。
// 不影响 cn_alloc/cn_realloc 分配的内存（向量数据数组由 RAII 析构管理）。
// 设计参考 C++ 智能指针池和 Rust 的 Drop trait——批量释放仅针对无 RAII 的分配。
extern "C" void __cn_alloc_reset() {
    // 074 波3 / 945：池整块归还（全部线程池块目录遍历 std::free·链/目录清空）——
    //   池内全部对象一次性失效（语义同旧注册表整批释放，且整块归远比逐对象 free
    //   更彻底）。945：活动对象数由池内位图扫描统计（poolReset 返回）——
    //   计数维护已整体移出分配热路径（原 g_pool.live 逐分配加减）。
    g_cn_alloc_live -= cnrt::poolReset();
    // 自愈式批量释放（2026-08-24 结构性加固）：
    //   unordered_set 遍历释放——天然去重，同一 ptr 只 free 一次（杜绝 double free）；
    //   边遍历边按值收集（set 迭代器不因 free 他人而失效）。
    //   完成后清空注册表；reset 之后旧 ptr 的 unregister/free 走"未找到"忽略。
    //   注意：reset 只应释放"无外部引用"的 tracked 内存（设计语义：组件间清场）。
    //   945：注册表读写持表锁（并发结构保护）；reset 与并发注册的时序=用户责任
    //   （组件间清场语义前提同池）。
    // 标识符用 ASCII（待释放）——GCC 9/7 不支持 UTF-8 标识符，中文标识符会中断 Linux 构建
    TrackedTable& t = trackedTable();
    std::vector<void*> to_free;
    {
        std::lock_guard<std::mutex> lk(t.mu);
        to_free.reserve(t.set.size());
        for (void* p : t.set) to_free.push_back(p);
        t.set.clear();
        t.size.clear();
    }
    for (void* p : to_free) {
        if (p != nullptr) {
            --g_cn_alloc_live;
            std::free(p);
        }
    }
#ifdef _WIN32
    // 2026-08-25 堆压缩：78/79 组件链连续编译多个模块时，每个模块释放百万级
    //   小对象后 Windows 堆不把空闲页归还 OS——工作集持续攀升（实测 26GB 失控）。
    //   _heapmin() 压缩堆并尽量归还空闲页，使 内存::释放全部() 真正回收内存。
    _heapmin();
#endif
}
