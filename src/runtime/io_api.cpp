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
// 链表节点：记录 tracked 分配的指针，供 __cn_alloc_reset() 遍历释放。
// 设计：节点本身也用 std::malloc 分配（非 tracked），避免递归注册。
struct TrackedNode {
    void* ptr;            // tracked 分配的用户指针
    TrackedNode* next;    // 链表下一节点
};
static TrackedNode* g_trackedHead = nullptr;  // tracked 链表头

// 注册 tracked 分配到链表（失败时不影响分配本身，仅无法批量释放）
static void trackedRegister(void* ptr) {
    if (ptr == nullptr) return;
    TrackedNode* node = static_cast<TrackedNode*>(std::malloc(sizeof(TrackedNode)));
    if (node == nullptr) return;  // 注册失败不影响功能，仅无法批量释放
    node->ptr = ptr;
    node->next = g_trackedHead;
    g_trackedHead = node;
}

// 从链表中移除 tracked 分配（cn_free_tracked 调用时）
static void trackedUnregister(void* ptr) {
    if (ptr == nullptr) return;
    TrackedNode** pp = &g_trackedHead;
    while (*pp != nullptr) {
        if ((*pp)->ptr == ptr) {
            TrackedNode* node = *pp;
            *pp = node->next;
            std::free(node);
            return;
        }
        pp = &((*pp)->next);
    }
    // 未找到：可能是 __cn_alloc_reset 已清空链表后的释放，忽略
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
        trackedRegister(p);  // 注册到链表，供批量释放
    }
    return p;
}

extern "C" void cn_free_tracked(void* ptr) {
    if (ptr != nullptr) {
        --g_cn_alloc_live;
        trackedUnregister(ptr);  // 从链表移除
        std::free(ptr);
    }
}

extern "C" void* cn_realloc_tracked(void* ptr, std::size_t size) {
    if (ptr == nullptr) {
        void* p = std::realloc(nullptr, size);
        if (p != nullptr) {
            ++g_cn_alloc_total;
            ++g_cn_alloc_live;
            trackedRegister(p);
        }
        return p;
    }
    if (size == 0) {
        --g_cn_alloc_live;
        trackedUnregister(ptr);
        return std::realloc(ptr, 0);
    }
    // realloc 扩容：更新链表中的指针
    void* new_p = std::realloc(ptr, size);
    if (new_p != nullptr && new_p != ptr) {
        // realloc 可能返回新地址：更新链表中的指针
        trackedUnregister(ptr);
        trackedRegister(new_p);
    }
    return new_p;
}

// 当前活动分配数（未释放块数，泄漏检测基线）
extern "C" long long __cn_alloc_live() {
    return g_cn_alloc_live.load();
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

// 新建对象：分配 size 字节堆内存（NewObject 指令展开调用）
// 使用 std::malloc（支持正常的 RAII 释放，与 C++ new / Rust Box 一致）
// 失败时报错误码4（内存分配失败，规格书附录B）并终止，成功返回对象指针
// 虚表指针初始化由 codegen 负责（对象首地址 8 字节，NewObject 后写入）
extern "C" void* __cn_object_new(long long size) {
    void* ptr = std::malloc(static_cast<std::size_t>(size > 0 ? size : 1));
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
    TrackedNode* node = g_trackedHead;
    while (node != nullptr) {
        TrackedNode* next = node->next;
        if (node->ptr != nullptr) {
            --g_cn_alloc_live;
            std::free(node->ptr);
        }
        std::free(node);
        node = next;
    }
    g_trackedHead = nullptr;
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
extern "C" void __cn_runtime_error(long long errorCode) {
    std::printf("运行时错误(错误码%lld): %s\n", errorCode, __cn_error_message(errorCode));
    std::exit(1);
}
