#include "cnlang/support/memory/arena.h"
#include <stdlib.h>
#include <string.h>

/*
 * CN Language Arena 分配器实现
 * 提供快速批量内存分配和统一释放能力
 */

// 辅助函数：对齐计算
static inline size_t align_up(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

// 创建新的内存块
static CnArenaBlock *arena_block_new(size_t size)
{
    CnArenaBlock *block;

    if (size == 0) {
        return NULL;
    }

    block = (CnArenaBlock *)malloc(sizeof(CnArenaBlock));
    if (!block) {
        return NULL;
    }

    // 为了确保对齐，额外分配一些空间
    size_t total_size = size + CN_ARENA_ALIGNMENT;
    char *raw_data = (char *)malloc(total_size);
    if (!raw_data) {
        free(block);
        return NULL;
    }

    // 计算对齐后的地址
    size_t addr = (size_t)raw_data;
    size_t aligned_addr = (addr + CN_ARENA_ALIGNMENT - 1) & ~(CN_ARENA_ALIGNMENT - 1);
    
    block->raw_data = raw_data;  // 保存原始指针用于 free
    block->data = (char *)aligned_addr;
    block->size = size;
    block->used = 0;
    block->next = NULL;

    return block;
}

// 释放内存块
static void arena_block_free(CnArenaBlock *block)
{
    if (!block) {
        return;
    }

    free(block->raw_data);  // 使用原始指针释放
    free(block);
}

CnArena *cn_arena_new(size_t block_size)
{
    CnArena *arena;
    CnArenaBlock *first_block;

    // 使用默认块大小
    if (block_size == 0) {
        block_size = CN_ARENA_BLOCK_SIZE;
    }

    arena = (CnArena *)malloc(sizeof(CnArena));
    if (!arena) {
        return NULL;
    }

    // 创建第一个块
    first_block = arena_block_new(block_size);
    if (!first_block) {
        free(arena);
        return NULL;
    }

    arena->first_block = first_block;
    arena->current_block = first_block;
    arena->block_size = block_size;
    arena->total_allocated = 0;
    arena->block_count = 1;

    return arena;
}

void cn_arena_free(CnArena *arena)
{
    CnArenaBlock *block;
    CnArenaBlock *next;

    if (!arena) {
        return;
    }

    // 释放所有块
    block = arena->first_block;
    while (block) {
        next = block->next;
        arena_block_free(block);
        block = next;
    }

    free(arena);
}

void *cn_arena_alloc_aligned(CnArena *arena, size_t size, size_t alignment)
{
    CnArenaBlock *block;
    size_t aligned_offset;
    size_t available;
    void *ptr;

    if (!arena || size == 0) {
        return NULL;
    }

    // 确保对齐是2的幂
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        alignment = CN_ARENA_ALIGNMENT;
    }

    block = arena->current_block;

    // 计算对齐后的偏移
    // 需要考虑block->data本身的地址可能不对齐的情况
    size_t data_addr = (size_t)block->data;
    size_t aligned_addr = align_up(data_addr + block->used, alignment);
    aligned_offset = aligned_addr - data_addr;
    available = block->size - aligned_offset;

    // 当前块空间不足
    if (available < size) {
        // 对于大对象，创建专用块
        size_t new_block_size = (size > arena->block_size) ? size : arena->block_size;
        CnArenaBlock *new_block = arena_block_new(new_block_size);
        
        if (!new_block) {
            return NULL;
        }

        // 链接新块
        block->next = new_block;
        arena->current_block = new_block;
        arena->block_count++;
        
        block = new_block;
        
        // 新块也需要计算对齐偏移
        data_addr = (size_t)block->data;
        aligned_addr = align_up(data_addr, alignment);
        aligned_offset = aligned_addr - data_addr;
    }

    // 分配内存
    ptr = block->data + aligned_offset;
    block->used = aligned_offset + size;
    arena->total_allocated += size;

    return ptr;
}

void *cn_arena_alloc(CnArena *arena, size_t size)
{
    return cn_arena_alloc_aligned(arena, size, CN_ARENA_ALIGNMENT);
}

void cn_arena_reset(CnArena *arena)
{
    CnArenaBlock *block;
    CnArenaBlock *next;

    if (!arena) {
        return;
    }

    // 释放第一个块之后的所有块
    block = arena->first_block->next;
    while (block) {
        next = block->next;
        arena_block_free(block);
        block = next;
    }

    // 重置第一个块
    arena->first_block->used = 0;
    arena->first_block->next = NULL;
    arena->current_block = arena->first_block;
    arena->total_allocated = 0;
    arena->block_count = 1;
}

void cn_arena_get_stats(CnArena *arena, size_t *out_total_allocated, size_t *out_block_count)
{
    if (!arena) {
        if (out_total_allocated) {
            *out_total_allocated = 0;
        }
        if (out_block_count) {
            *out_block_count = 0;
        }
        return;
    }

    if (out_total_allocated) {
        *out_total_allocated = arena->total_allocated;
    }
    if (out_block_count) {
        *out_block_count = arena->block_count;
    }
}
