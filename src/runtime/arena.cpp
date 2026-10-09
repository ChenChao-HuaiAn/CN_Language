// CN运行时实现：进程竞技场 + 复制/置零内存辅助（规格书10.2）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：复制内存→cn_memcpy、置零内存→cn_memset
// 349 重构E 自 io_api.cpp 纯机械搬移·零逻辑变化：原 785-868 行（进程竞技场节+
//   cn_memcpy/cn_memset）——本节零跨节 static 引用，独立成文件不改任何链接性。
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
