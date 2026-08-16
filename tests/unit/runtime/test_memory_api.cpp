// 内存管理 API 单元测试（自举前置 C-1/C-3，2026-08）
// 覆盖：
//   C-3 分配计数：cn_alloc/cn_free/cn_realloc/cn_alloc_tracked 全路径
//       维护 活动分配数（__cn_alloc_live）/总分配次数（__cn_alloc_total）；
//       释放后回落基线（泄漏检测依据）
//   C-1 进程竞技场：__cn_arena_alloc bump 分配（8 字节对齐）、
//       __cn_arena_reset 整块释放、__cn_arena_bytes 统计
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>

#include "runtime/runtime.hpp"

namespace {

// 基线快照：活动分配数（测试开始前可能已有 CRT/argv 缓存分配）
long long liveBase() { return __cn_alloc_live(); }
long long totalBase() { return __cn_alloc_total(); }

} // namespace

// ==================== C-3：分配计数（泄漏检测） ====================

// cn_alloc + cn_free：活动分配数 先增后回落基线
TEST(MemoryApiTest, AllocFreeRoundTrip) {
    const long long live = liveBase();
    const long long total = totalBase();
    void* p = cn_alloc(64);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(__cn_alloc_live(), live + 1);
    EXPECT_EQ(__cn_alloc_total(), total + 1);
    cn_free(p);
    EXPECT_EQ(__cn_alloc_live(), live);
    EXPECT_EQ(__cn_alloc_total(), total + 1);  // 累计不回落
}

// cn_realloc：扩容不改变活动分配数；nullptr 起始 = 新分配
TEST(MemoryApiTest, ReallocAccounting) {
    const long long live = liveBase();
    const long long total = totalBase();
    void* p = cn_alloc(16);
    ASSERT_NE(p, nullptr);
    void* q = cn_realloc(p, 128);  // 扩容：live 不变
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(__cn_alloc_live(), live + 1);
    EXPECT_EQ(__cn_alloc_total(), total + 1);
    void* r2 = cn_realloc(nullptr, 32);  // 空指针起始：新分配
    ASSERT_NE(r2, nullptr);
    EXPECT_EQ(__cn_alloc_live(), live + 2);
    EXPECT_EQ(__cn_alloc_total(), total + 2);
    cn_free(q);
    cn_free(r2);
    EXPECT_EQ(__cn_alloc_live(), live);
}

// cn_free(nullptr)：安全空操作，计数不变
TEST(MemoryApiTest, FreeNullNoop) {
    const long long live = liveBase();
    cn_free(nullptr);
    EXPECT_EQ(__cn_alloc_live(), live);
}

// 计数分配辅助（字符串 API 内部路径）：与 cn_alloc 同语义同计数
TEST(MemoryApiTest, TrackedAllocAccounting) {
    const long long live = liveBase();
    const long long total = totalBase();
    void* p = cn_alloc_tracked(100);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(__cn_alloc_live(), live + 1);
    EXPECT_EQ(__cn_alloc_total(), total + 1);
    cn_free_tracked(p);
    EXPECT_EQ(__cn_alloc_live(), live);
    // 混合：tracked 分配 用 cn_free 释放（__cn_str_free 路径）计数一致
    void* q = cn_alloc_tracked(50);
    ASSERT_NE(q, nullptr);
    cn_free(q);
    EXPECT_EQ(__cn_alloc_live(), live);
}

// 多块分配/释放交错：活动分配数精确跟踪（泄漏检测回归）
TEST(MemoryApiTest, InterleavedAllocFree) {
    const long long live = liveBase();
    void* p1 = cn_alloc(8);
    void* p2 = cn_alloc_tracked(16);
    void* p3 = cn_alloc(24);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(__cn_alloc_live(), live + 3);
    cn_free(p1);
    EXPECT_EQ(__cn_alloc_live(), live + 2);
    cn_free(p3);
    cn_free_tracked(p2);
    EXPECT_EQ(__cn_alloc_live(), live);
}

// ==================== C-1：进程竞技场 ====================

// 竞技场 bump 分配：连续地址（块内单调递增）+ 8 字节对齐
TEST(MemoryApiTest, ArenaBumpAlloc) {
    __cn_arena_reset();  // 清理既有状态（保证可重复）
    void* a = __cn_arena_alloc(16);
    void* b = __cn_arena_alloc(16);
    void* c = __cn_arena_alloc(24);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    // 同块内 bump：b = a + 16（对齐后），c = b + 16
    EXPECT_EQ(static_cast<char*>(b) - static_cast<char*>(a), 16);
    EXPECT_EQ(static_cast<char*>(c) - static_cast<char*>(b), 16);
    // 8 字节对齐
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(a) % 8, 0u);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(b) % 8, 0u);
    // 字节统计：至少覆盖已分配（含块头）
    EXPECT_GE(__cn_arena_bytes(), 16LL + 16 + 24);
    EXPECT_GE(__cn_arena_blocks(), 1LL);
    __cn_arena_reset();
    EXPECT_EQ(__cn_arena_bytes(), 0LL);
    EXPECT_EQ(__cn_arena_blocks(), 0LL);
}

// 竞技场写入可用（分配的块可正常读写）
TEST(MemoryApiTest, ArenaWritable) {
    __cn_arena_reset();
    char* p = static_cast<char*>(__cn_arena_alloc(4096));
    ASSERT_NE(p, nullptr);
    std::memset(p, 0x5A, 4096);   // 写入
    EXPECT_EQ(static_cast<unsigned char>(p[0]), 0x5Au);
    EXPECT_EQ(static_cast<unsigned char>(p[4095]), 0x5Au);
    __cn_arena_reset();
}

// 超大分配：超出单块 64KB 自动扩容块容量
TEST(MemoryApiTest, ArenaLargeAlloc) {
    __cn_arena_reset();
    void* p = __cn_arena_alloc(200 * 1024);  // 200KB > 64KB 块
    ASSERT_NE(p, nullptr);
    std::memset(p, 1, 200 * 1024);           // 可写
    const unsigned char lastByte = static_cast<unsigned char*>(p)[199 * 1024];
    EXPECT_EQ(lastByte, 1u);
    __cn_arena_reset();
    EXPECT_EQ(__cn_arena_bytes(), 0LL);
}
