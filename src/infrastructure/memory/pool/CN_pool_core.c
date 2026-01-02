/******************************************************************************
 * 文件名: CN_pool_core.c
 * 功能: CN_Language对象池分配器核心实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_pool_allocator.c拆分，创建核心模块
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_pool_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>


// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 调试日志输出
 */
void pool_debug_log(Stru_CN_PoolAllocator_t* pool, const char* format, ...)
{
    if (pool->debug_callback != NULL)
    {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        pool->debug_callback(buffer, pool->debug_user_data);
    }
}

/**
 * @brief 创建新的内存块
 */
static Stru_CN_PoolBlock_t* create_pool_block(size_t object_size, size_t capacity)
{
    // 计算总内存需求
    size_t block_header_size = sizeof(Stru_CN_PoolBlock_t);
    size_t bitmap_size = (capacity + 7) / 8;  // 每个位表示一个对象
    size_t objects_size = object_size * capacity;
    size_t total_size = block_header_size + bitmap_size + objects_size;
    
    // 分配内存
    unsigned char* memory = (unsigned char*)malloc(total_size);
    if (memory == NULL)
    {
        return NULL;
    }
    
    // 设置块头
    Stru_CN_PoolBlock_t* block = (Stru_CN_PoolBlock_t*)memory;
    block->next = NULL;
    block->memory = memory + block_header_size + bitmap_size;
    block->capacity = capacity;
    block->used_count = 0;
    block->usage_bitmap = memory + block_header_size;
    
    // 初始化位图（全部空闲）
    memset(block->usage_bitmap, 0, bitmap_size);
    
    return block;
}

/**
 * @brief 销毁内存块
 */
static void destroy_pool_block(Stru_CN_PoolBlock_t* block)
{
    if (block != NULL)
    {
        free(block);
    }
}

/**
 * @brief 在块中分配对象
 */
void* allocate_from_block(Stru_CN_PoolBlock_t* block, size_t object_size)
{
    // 查找第一个空闲位置
    size_t bitmap_size = (block->capacity + 7) / 8;
    
    for (size_t byte_idx = 0; byte_idx < bitmap_size; byte_idx++)
    {
        unsigned char byte = block->usage_bitmap[byte_idx];
        if (byte != 0xFF)  // 这个字节还有空闲位
        {
            for (int bit_idx = 0; bit_idx < 8; bit_idx++)
            {
                size_t obj_idx = byte_idx * 8 + bit_idx;
                if (obj_idx >= block->capacity)
                {
                    break;  // 超出块容量
                }
                
                if (!(byte & (1 << bit_idx)))  // 这个位是0，表示空闲
                {
                    // 标记为已使用
                    block->usage_bitmap[byte_idx] |= (1 << bit_idx);
                    block->used_count++;
                    
                    // 计算对象地址
                    void* obj = (char*)block->memory + obj_idx * object_size;
                    return obj;
                }
            }
        }
    }
    
    return NULL;  // 块已满
}

/**
 * @brief 释放块中的对象
 */
bool free_in_block(Stru_CN_PoolBlock_t* block, void* ptr, size_t object_size)
{
    // 检查指针是否在块范围内
    char* block_start = (char*)block->memory;
    char* block_end = block_start + block->capacity * object_size;
    char* obj_ptr = (char*)ptr;
    
    if (obj_ptr < block_start || obj_ptr >= block_end)
    {
        return false;  // 指针不在这个块中
    }
    
    // 计算对象索引
    size_t offset = obj_ptr - block_start;
    size_t obj_idx = offset / object_size;
    
    // 验证索引有效性
    if (obj_idx >= block->capacity || (offset % object_size) != 0)
    {
        return false;  // 无效的对象地址
    }
    
    // 计算位图位置
    size_t byte_idx = obj_idx / 8;
    int bit_idx = obj_idx % 8;
    
    // 验证对象当前是否已分配
    if (!(block->usage_bitmap[byte_idx] & (1 << bit_idx)))
    {
        return false;  // 对象未分配或已释放
    }
    
    // 标记为空闲
    block->usage_bitmap[byte_idx] &= ~(1 << bit_idx);
    block->used_count--;
    
    return true;
}

/**
 * @brief 扩展对象池
 */
bool expand_pool(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return false;
    }
    
    // 检查是否达到最大容量
    size_t current_total = pool->stats.total_objects;
    size_t new_capacity = pool->config.expand_increment;
    
    if (pool->config.max_capacity > 0)
    {
        if (current_total + new_capacity > pool->config.max_capacity)
        {
            // 尝试使用剩余容量
            if (current_total >= pool->config.max_capacity)
            {
                pool_debug_log(pool, "对象池已达到最大容量 %zu", pool->config.max_capacity);
                return false;
            }
            new_capacity = pool->config.max_capacity - current_total;
        }
    }
    
    // 创建新块
    Stru_CN_PoolBlock_t* new_block = create_pool_block(pool->config.object_size, new_capacity);
    if (new_block == NULL)
    {
        pool->stats.allocation_failures++;
        pool_debug_log(pool, "扩展对象池失败：内存不足");
        return false;
    }
    
    // 添加到链表
    if (pool->last_block == NULL)
    {
        pool->first_block = new_block;
        pool->last_block = new_block;
    }
    else
    {
        pool->last_block->next = new_block;
        pool->last_block = new_block;
    }
    
    // 更新统计信息
    pool->block_count++;
    pool->stats.total_objects += new_capacity;
    pool->stats.free_objects += new_capacity;
    pool->stats.pool_expansions++;
    
    // 更新内存使用统计
    size_t block_memory = sizeof(Stru_CN_PoolBlock_t) + 
                         ((new_capacity + 7) / 8) + 
                         (pool->config.object_size * new_capacity);
    pool->stats.memory_usage += block_memory;
    if (pool->stats.memory_usage > pool->stats.peak_memory_usage)
    {
        pool->stats.peak_memory_usage = pool->stats.memory_usage;
    }
    
    pool_debug_log(pool, "对象池扩展成功，新增 %zu 个对象", new_capacity);
    return true;
}

// ============================================================================
// 公共接口实现 - 池创建和销毁
// ============================================================================

Stru_CN_PoolAllocator_t* CN_pool_create(const Stru_CN_PoolConfig_t* config)
{
    if (config == NULL)
    {
        return NULL;
    }
    
    // 验证配置
    if (config->object_size == 0)
    {
        return NULL;  // 必须指定对象大小
    }
    
    if (config->initial_capacity == 0)
    {
        return NULL;  // 必须指定初始容量
    }
    
    // 分配对象池结构
    Stru_CN_PoolAllocator_t* pool = (Stru_CN_PoolAllocator_t*)malloc(sizeof(Stru_CN_PoolAllocator_t));
    if (pool == NULL)
    {
        return NULL;
    }
    
    // 初始化结构
    memset(pool, 0, sizeof(Stru_CN_PoolAllocator_t));
    pool->config = *config;
    pool->initialized = true;
    
    // 创建初始内存块
    Stru_CN_PoolBlock_t* initial_block = create_pool_block(config->object_size, config->initial_capacity);
    if (initial_block == NULL)
    {
        free(pool);
        return NULL;
    }
    
    pool->first_block = initial_block;
    pool->last_block = initial_block;
    pool->block_count = 1;
    
    // 初始化统计信息
    pool->stats.total_objects = config->initial_capacity;
    pool->stats.free_objects = config->initial_capacity;
    pool->stats.memory_usage = sizeof(Stru_CN_PoolAllocator_t) + 
                              sizeof(Stru_CN_PoolBlock_t) + 
                              ((config->initial_capacity + 7) / 8) + 
                              (config->object_size * config->initial_capacity);
    pool->stats.peak_memory_usage = pool->stats.memory_usage;
    
    pool_debug_log(pool, "对象池创建成功，对象大小: %zu, 初始容量: %zu", 
                   config->object_size, config->initial_capacity);
    
    return pool;
}

void CN_pool_destroy(Stru_CN_PoolAllocator_t* pool)
{
    if (pool == NULL || !pool->initialized)
    {
        return;
    }
    
    pool_debug_log(pool, "销毁对象池，总分配: %zu, 总释放: %zu", 
                   pool->stats.total_allocations, pool->stats.total_deallocations);
    
    // 销毁所有内存块
    Stru_CN_PoolBlock_t* block = pool->first_block;
    while (block != NULL)
    {
        Stru_CN_PoolBlock_t* next = block->next;
        destroy_pool_block(block);
        block = next;
    }
    
    // 清理池结构
    pool->initialized = false;
    free(pool);
}
