#ifndef CN_SUPPORT_MEMORY_ARENA_H
#define CN_SUPPORT_MEMORY_ARENA_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CN Language Arena 分配器
 * 用于高效管理具有相同生命周期的对象集合
 * 特别适用于 AST 节点、IR 指令等编译器临时数据结构
 */

// Arena 块大小默认值（64KB）
#ifndef CN_ARENA_BLOCK_SIZE
#define CN_ARENA_BLOCK_SIZE (64 * 1024)
#endif

// 对齐要求（8字节对齐）
#ifndef CN_ARENA_ALIGNMENT
#define CN_ARENA_ALIGNMENT 8
#endif

// 内存块结构
typedef struct CnArenaBlock {
    char *data;                    // 块数据指针（对齐后）
    char *raw_data;                // 原始 malloc 返回的指针（用于 free）
    size_t size;                   // 块大小
    size_t used;                   // 已使用字节数
    struct CnArenaBlock *next;     // 下一个块
} CnArenaBlock;

// Arena 分配器结构
typedef struct CnArena {
    CnArenaBlock *first_block;     // 第一个块
    CnArenaBlock *current_block;   // 当前分配块
    size_t block_size;             // 默认块大小
    size_t total_allocated;        // 总分配字节数
    size_t block_count;            // 块数量
} CnArena;

// =============================================================================
// Arena 管理接口
// =============================================================================

/*
 * 创建新的 Arena 分配器
 * @param block_size 块大小，传入 0 使用默认大小
 * @return Arena 指针，失败返回 NULL
 */
CnArena *cn_arena_new(size_t block_size);

/*
 * 销毁 Arena 及其所有分配的内存
 * @param arena Arena 指针
 */
void cn_arena_free(CnArena *arena);

/*
 * 从 Arena 分配内存
 * @param arena Arena 指针
 * @param size 分配大小
 * @return 分配的内存指针，失败返回 NULL
 */
void *cn_arena_alloc(CnArena *arena, size_t size);

/*
 * 从 Arena 分配对齐的内存
 * @param arena Arena 指针
 * @param size 分配大小
 * @param alignment 对齐要求（必须是2的幂）
 * @return 分配的内存指针，失败返回 NULL
 */
void *cn_arena_alloc_aligned(CnArena *arena, size_t size, size_t alignment);

/*
 * 重置 Arena，释放所有块但保留第一个块供复用
 * @param arena Arena 指针
 */
void cn_arena_reset(CnArena *arena);

/*
 * 获取 Arena 统计信息
 * @param arena Arena 指针
 * @param out_total_allocated 输出总分配字节数
 * @param out_block_count 输出块数量
 */
void cn_arena_get_stats(CnArena *arena, size_t *out_total_allocated, size_t *out_block_count);

// =============================================================================
// 便利宏
// =============================================================================

// 从 Arena 分配类型化对象
#define CN_ARENA_ALLOC(arena, type) \
    ((type *)cn_arena_alloc((arena), sizeof(type)))

// 从 Arena 分配类型化数组
#define CN_ARENA_ALLOC_ARRAY(arena, type, count) \
    ((type *)cn_arena_alloc((arena), sizeof(type) * (count)))

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_MEMORY_ARENA_H */
