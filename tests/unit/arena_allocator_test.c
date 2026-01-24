#include "cnlang/support/memory/arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * Arena 分配器单元测试
 */

// 测试基本分配和释放
static void test_arena_basic_alloc(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);

    // 分配小对象
    int *p1 = (int *)cn_arena_alloc(arena, sizeof(int));
    assert(p1 != NULL);
    *p1 = 42;
    assert(*p1 == 42);

    // 分配更多对象
    char *p2 = (char *)cn_arena_alloc(arena, 100);
    assert(p2 != NULL);
    memset(p2, 'A', 100);
    assert(p2[0] == 'A' && p2[99] == 'A');

    // 验证第一个对象未被覆盖
    assert(*p1 == 42);

    cn_arena_free(arena);
    printf("[PASS] test_arena_basic_alloc\n");
}

// 测试对齐分配
static void test_arena_aligned_alloc(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);

    // 分配对齐对象
    void *p1 = cn_arena_alloc_aligned(arena, 17, 16);
    assert(p1 != NULL);
    assert(((size_t)p1 & 15) == 0); // 验证16字节对齐

    void *p2 = cn_arena_alloc_aligned(arena, 33, 32);
    assert(p2 != NULL);
    assert(((size_t)p2 & 31) == 0); // 验证32字节对齐

    cn_arena_free(arena);
    printf("[PASS] test_arena_aligned_alloc\n");
}

// 测试多块分配
static void test_arena_multiple_blocks(void)
{
    CnArena *arena = cn_arena_new(256); // 小块大小
    assert(arena != NULL);

    // 分配多个对象，触发新块创建
    void *p1 = cn_arena_alloc(arena, 100);
    assert(p1 != NULL);
    
    void *p2 = cn_arena_alloc(arena, 100);
    assert(p2 != NULL);
    
    void *p3 = cn_arena_alloc(arena, 100); // 应该触发新块
    assert(p3 != NULL);

    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    assert(total >= 300);
    assert(block_count >= 2); // 至少有2个块

    cn_arena_free(arena);
    printf("[PASS] test_arena_multiple_blocks\n");
}

// 测试大对象分配
static void test_arena_large_alloc(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);

    // 分配超过默认块大小的对象
    size_t large_size = 2048;
    void *p = cn_arena_alloc(arena, large_size);
    assert(p != NULL);

    // 应该创建了专用块
    size_t total, block_count;
    cn_arena_get_stats(arena, &total, &block_count);
    assert(block_count >= 2);

    cn_arena_free(arena);
    printf("[PASS] test_arena_large_alloc\n");
}

// 测试重置功能
static void test_arena_reset(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);

    // 分配一些对象
    void *p1 = cn_arena_alloc(arena, 100);
    assert(p1 != NULL);
    void *p2 = cn_arena_alloc(arena, 200);
    assert(p2 != NULL);

    size_t total_before, blocks_before;
    cn_arena_get_stats(arena, &total_before, &blocks_before);
    assert(total_before >= 300);

    // 重置Arena
    cn_arena_reset(arena);

    size_t total_after, blocks_after;
    cn_arena_get_stats(arena, &total_after, &blocks_after);
    assert(total_after == 0);
    assert(blocks_after == 1); // 保留第一个块

    // 重置后可以继续分配
    void *p3 = cn_arena_alloc(arena, 50);
    assert(p3 != NULL);

    cn_arena_free(arena);
    printf("[PASS] test_arena_reset\n");
}

// 测试便利宏
static void test_arena_macros(void)
{
    CnArena *arena = cn_arena_new(1024);
    assert(arena != NULL);

    // 使用宏分配单个对象
    typedef struct TestStruct {
        int a;
        double b;
        char c[16];
    } TestStruct;

    TestStruct *s = CN_ARENA_ALLOC(arena, TestStruct);
    assert(s != NULL);
    s->a = 123;
    s->b = 3.14;
    assert(s->a == 123);

    // 使用宏分配数组
    int *arr = CN_ARENA_ALLOC_ARRAY(arena, int, 10);
    assert(arr != NULL);
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 2;
    }
    assert(arr[5] == 10);

    cn_arena_free(arena);
    printf("[PASS] test_arena_macros\n");
}

// 测试空输入处理
static void test_arena_edge_cases(void)
{
    // 创建 Arena 时传入 0 使用默认大小
    CnArena *arena = cn_arena_new(0);
    assert(arena != NULL);

    // 分配 0 字节应返回 NULL
    void *p = cn_arena_alloc(arena, 0);
    assert(p == NULL);

    // 对 NULL arena 调用函数应该安全
    cn_arena_free(NULL);
    cn_arena_reset(NULL);
    p = cn_arena_alloc(NULL, 100);
    assert(p == NULL);

    size_t total, blocks;
    cn_arena_get_stats(NULL, &total, &blocks);
    assert(total == 0 && blocks == 0);

    cn_arena_free(arena);
    printf("[PASS] test_arena_edge_cases\n");
}

// 测试内存统计
static void test_arena_stats(void)
{
    CnArena *arena = cn_arena_new(512);
    assert(arena != NULL);

    size_t total, blocks;
    
    // 初始状态
    cn_arena_get_stats(arena, &total, &blocks);
    assert(total == 0);
    assert(blocks == 1);

    // 分配后统计
    cn_arena_alloc(arena, 100);
    cn_arena_alloc(arena, 200);
    cn_arena_get_stats(arena, &total, &blocks);
    assert(total == 300);

    // 重置后统计
    cn_arena_reset(arena);
    cn_arena_get_stats(arena, &total, &blocks);
    assert(total == 0);
    assert(blocks == 1);

    cn_arena_free(arena);
    printf("[PASS] test_arena_stats\n");
}

int main(void)
{
    printf("运行 Arena 分配器单元测试...\n\n");

    test_arena_basic_alloc();
    test_arena_aligned_alloc();
    test_arena_multiple_blocks();
    test_arena_large_alloc();
    test_arena_reset();
    test_arena_macros();
    test_arena_edge_cases();
    test_arena_stats();

    printf("\n所有测试通过！\n");
    return 0;
}
