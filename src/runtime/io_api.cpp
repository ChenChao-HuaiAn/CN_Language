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
    }
    return p;
}

// 释放内存：对应CN内置函数 释放（free 语义）
extern "C" void cn_free(void* ptr) {
    if (ptr != nullptr) {
        --g_cn_alloc_live;
        std::free(ptr);
    }
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
        }
        return p;
    }
    if (size == 0) {
        --g_cn_alloc_live;
        return std::realloc(ptr, 0);
    }
    return std::realloc(ptr, size);
}

// ---- 计数分配辅助（自举前置 C-3，2026-08）----
// 字符串 API（string_api.cpp）等内部 malloc 直调改走 *_tracked，使 活动分配数/
//   总分配次数 覆盖全部动态内存（此前 __cn_str_free 经 cn_free 减计数而分配
//   未加计数 -> 计数为负，泄漏检测失真）。
// tracked 分配注册到全局链表，供 内存::释放全部() 批量释放（兜底防泄漏）。
extern "C" void* cn_alloc_tracked(std::size_t size) {
    void* p = std::malloc(size);
    if (p != nullptr) {
        ++g_cn_alloc_total;
        ++g_cn_alloc_live;
        trackedRegister(p, size);  // 注册到链表（含 size，供 realloc 精确拷贝）
    }
    return p;
}

extern "C" void cn_free_tracked(void* ptr) {
    // 安全释放（2026-08-24 加固）：仅在 ptr 确实在本批 tracked 注册表中时释放——
    //   reset 之后旧 ptr 不在表内，此处忽略而非 free，杜绝"reset 后遗留释放"双重释放
    //   （78 段错误候选根因之一：组件链对已批量释放的旧串再次 字符串释放）。
    if (ptr != nullptr && trackedUnregister(ptr)) {
        --g_cn_alloc_live;
        std::free(ptr);
    }
}

extern "C" void* cn_realloc_tracked(void* ptr, std::size_t size) {
    if (ptr == nullptr) {
        void* p = std::realloc(nullptr, size);
        if (p != nullptr) {
            ++g_cn_alloc_total;
            ++g_cn_alloc_live;
            trackedRegister(p, size);
        }
        return p;
    }
    if (size == 0) {
        if (trackedUnregister(ptr)) --g_cn_alloc_live;
        return std::realloc(ptr, 0);
    }
    // 2026-08-24 终版：std::realloc 扩容（保留"原地扩展"的堆效率，避免
    //   malloc+memcpy+free 的堆碎片导致工作集失控——79 实测 8GB+ 未回落的教训）。
    //   注册表为 unordered_set（天然去重）+ size map，换址时原子地
    //   unregister(旧)+register(新,size)，杜绝原链表实现的重复/悬垂节点双 free。
    void* new_p = std::realloc(ptr, size);
    if (new_p == nullptr) return nullptr;  // 失败：旧块仍有效且仍注册，调用方自行处理
    if (new_p != ptr) {
        trackedUnregister(ptr);
        trackedRegister(new_p, size);
    } else {
        // 原地扩展：仅更新 size 元数据
        g_trackedSize[ptr] = size;
    }
    return new_p;
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
extern "C" void* __cn_object_new(long long size) {
    void* ptr = std::calloc(1, static_cast<std::size_t>(size > 0 ? size : 1));
    if (ptr == nullptr) {
        __cn_runtime_error(4);  // 内存分配失败（不返回）
    }
    return ptr;
}

// 删除对象：释放对象内存（DeleteObject 指令展开调用；安全释放 nullptr）
extern "C" void __cn_object_delete(void* ptr) {
    std::free(ptr);
}

// ==================== 批量释放（2026-08-24 OOM 修复） ====================
// 内存::释放全部() -> __cn_alloc_reset()
// 遍历 tracked 链表，释放所有未释放的 tracked 内存（字符串等原始类型）。
// 不影响 cn_alloc/cn_realloc 分配的内存（向量数据数组由 RAII 析构管理）。
// 设计参考 C++ 智能指针池和 Rust 的 Drop trait——批量释放仅针对无 RAII 的分配。
extern "C" void __cn_alloc_reset() {
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
        int n = std::snprintf(buf, sizeof(buf),
                              "[veh] code=%08X RIP=%p RSP=%p fault=%p base=%p RVA=%llx\n",
                              (unsigned)info->ExceptionRecord->ExceptionCode,
                              (void*)info->ExceptionRecord->ExceptionAddress,
                              (void*)info->ContextRecord->Rsp, fault, (void*)base,
                              (unsigned long long)(info->ContextRecord->Rip - base));
        if (n > 0) { DWORD written; WriteFile(GetStdHandle(STD_ERROR_HANDLE), buf, (DWORD)n, &written, nullptr); }
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
struct cn_crash_auto_install {
    cn_crash_auto_install() {
        SetUnhandledExceptionFilter(cn_crash_filter);
        DWORD written; char m[] = "[crash] UEF installed\n";
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), m, (DWORD)(sizeof(m) - 1), &written, nullptr);
    }
};
static const cn_crash_auto_install cn_crash_auto_install_instance;
}
#endif
