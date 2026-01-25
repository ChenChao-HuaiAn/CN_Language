#include "cnlang/runtime/allocator.h"
#include <stdio.h>
#include <string.h>

/*
 * CN Language 内存分配器使用示例
 * 
 * 本示例演示:
 * 1. 创建内存分配器
 * 2. 创建和管理多个内存池
 * 3. 使用不同的分配策略
 * 4. 内存泄漏检测
 * 5. 性能统计
 */

// 模拟一个简单的内存管理系统
typedef struct Node {
    int id;
    char name[32];
    struct Node *next;
} Node;

int main(void)
{
    printf("========================================\n");
    printf("  CN Language 内存分配器示例\n");
    printf("========================================\n\n");
    
    // 1. 创建内存分配器（默认1MB池大小，使用首次适应策略）
    printf("1. 创建内存分配器...\n");
    CnAllocator *allocator = cn_allocator_create(1024 * 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    if (!allocator) {
        fprintf(stderr, "创建分配器失败\n");
        return 1;
    }
    printf("   分配器创建成功\n\n");
    
    // 2. 创建多个内存池，每个池使用不同的策略
    printf("2. 创建内存池...\n");
    
    // 快速分配池 - 使用首次适应
    CnMemPool *fast_pool = cn_pool_create(allocator, "快速分配池", 
                                          256 * 1024, CN_ALLOC_STRATEGY_FIRST_FIT);
    printf("   创建快速分配池 (首次适应策略)\n");
    
    // 节省空间池 - 使用最佳适应
    CnMemPool *compact_pool = cn_pool_create(allocator, "节省空间池", 
                                             512 * 1024, CN_ALLOC_STRATEGY_BEST_FIT);
    printf("   创建节省空间池 (最佳适应策略)\n");
    
    // 大对象池 - 使用最差适应
    CnMemPool *large_pool = cn_pool_create(allocator, "大对象池", 
                                           1024 * 1024, CN_ALLOC_STRATEGY_WORST_FIT);
    printf("   创建大对象池 (最差适应策略)\n\n");
    
    // 3. 从不同的池分配内存
    printf("3. 分配内存...\n");
    
    // 从快速池分配小对象
    Node *node1 = (Node *)cn_pool_alloc(fast_pool, sizeof(Node));
    node1->id = 1;
    strcpy(node1->name, "节点1");
    printf("   从快速池分配节点1: %s\n", node1->name);
    
    Node *node2 = (Node *)cn_pool_alloc(fast_pool, sizeof(Node));
    node2->id = 2;
    strcpy(node2->name, "节点2");
    printf("   从快速池分配节点2: %s\n", node2->name);
    
    // 从紧凑池分配
    char *str1 = (char *)cn_pool_alloc(compact_pool, 100);
    strcpy(str1, "这是一个字符串");
    printf("   从节省空间池分配字符串: %s\n", str1);
    
    // 从大对象池分配
    int *large_array = (int *)cn_pool_alloc(large_pool, 10000 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        large_array[i] = i * 10;
    }
    printf("   从大对象池分配数组: [0, 10, 20, ...]\n\n");
    
    // 4. 打印统计信息
    printf("4. 内存使用统计...\n");
    cn_pool_print_stats(fast_pool);
    printf("\n");
    cn_pool_print_stats(compact_pool);
    printf("\n");
    cn_pool_print_stats(large_pool);
    printf("\n");
    
    // 5. 演示内存泄漏检测
    printf("5. 内存泄漏检测...\n");
    
    // 启用泄漏检测
    cn_allocator_set_leak_detection(allocator, true);
    
    // 故意不释放一些内存来演示泄漏检测
    void *leak1 = cn_pool_alloc(fast_pool, 200);
    void *leak2 = cn_pool_alloc(compact_pool, 300);
    
    printf("   已分配但未释放的内存块数量: %zu\n", cn_allocator_check_leaks(allocator));
    
    // 打印泄漏报告
    printf("\n");
    cn_allocator_print_leak_report(allocator);
    
    // 6. 释放一些内存
    printf("6. 释放内存...\n");
    cn_pool_free(fast_pool, node1);
    cn_pool_free(fast_pool, node2);
    cn_pool_free(compact_pool, str1);
    cn_pool_free(large_pool, large_array);
    printf("   已释放部分内存\n\n");
    
    // 7. 再次检查泄漏
    printf("7. 再次检查泄漏...\n");
    size_t remaining_leaks = cn_allocator_check_leaks(allocator);
    printf("   剩余未释放的内存块: %zu\n\n", remaining_leaks);
    
    // 8. 内存碎片整理
    printf("8. 内存碎片整理...\n");
    size_t merged = cn_pool_merge_free_blocks(fast_pool);
    printf("   快速池合并了 %zu 个空闲块\n", merged);
    
    merged = cn_pool_merge_free_blocks(compact_pool);
    printf("   节省空间池合并了 %zu 个空闲块\n", merged);
    
    merged = cn_pool_merge_free_blocks(large_pool);
    printf("   大对象池合并了 %zu 个空闲块\n\n", merged);
    
    // 9. 打印最终统计
    printf("9. 最终统计...\n");
    cn_allocator_print_stats(allocator);
    
    // 10. 清理
    printf("\n10. 清理资源...\n");
    cn_allocator_destroy(allocator);
    printf("   分配器已销毁\n\n");
    
    printf("========================================\n");
    printf("  示例完成\n");
    printf("========================================\n");
    
    return 0;
}
