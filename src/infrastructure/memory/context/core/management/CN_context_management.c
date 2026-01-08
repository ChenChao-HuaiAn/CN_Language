/**
 * @file CN_context_management.c
 * @brief 内存上下文管理模块
 * 
 * 实现了内存上下文的管理功能，包括创建、销毁和重置。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "../CN_context_core.h"
#include "../CN_context_struct.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static uint64_t F_generate_context_id(void);
static uint64_t F_get_current_timestamp(void);

// ============================================================================
// 上下文管理函数实现
// ============================================================================

/**
 * @brief 生成唯一上下文ID
 */
static uint64_t F_generate_context_id(void)
{
    return g_next_context_id++;
}

/**
 * @brief 获取当前时间戳（毫秒）
 */
static uint64_t F_get_current_timestamp(void)
{
    // 简单实现，实际项目中应使用平台特定的高精度时间函数
    return (uint64_t)time(NULL) * 1000;
}

/**
 * @brief 创建新的内存上下文
 */
Stru_MemoryContext_t* F_context_create(const char* name,
                                      Stru_MemoryContext_t* parent)
{
    // 分配上下文结构体
    Stru_MemoryContext_t* context = 
        (Stru_MemoryContext_t*)malloc(sizeof(Stru_MemoryContext_t));
    
    if (context == NULL)
    {
        return NULL;
    }
    
    // 初始化基本信息
    if (name != NULL)
    {
        context->name = (char*)malloc(strlen(name) + 1);
        if (context->name == NULL)
        {
            free(context);
            return NULL;
        }
        strcpy(context->name, name);
    }
    else
    {
        // 允许空名称，使用默认名称
        const char* default_name = "UnnamedContext";
        context->name = (char*)malloc(strlen(default_name) + 1);
        if (context->name == NULL)
        {
            free(context);
            return NULL;
        }
        strcpy(context->name, default_name);
    }
    
    context->id = F_generate_context_id();
    context->creation_time = F_get_current_timestamp();
    
    // 初始化层次结构
    context->parent = parent;
    context->first_child = NULL;
    context->next_sibling = NULL;
    
    if (parent != NULL)
    {
        context->depth = parent->depth + 1;
        
        // 添加到父上下文的子链表
        context->next_sibling = parent->first_child;
        parent->first_child = context;
    }
    else
    {
        context->depth = 0;
    }
    
    // 初始化内存管理
    context->allocations = NULL;
    context->total_allocated = 0;
    context->peak_allocated = 0;
    context->current_usage = 0;
    context->allocation_count = 0;
    
    // 初始化分配器接口（使用系统分配器）
    context->allocate_func = NULL;
    context->reallocate_func = NULL;
    context->deallocate_func = NULL;
    context->allocator_data = NULL;
    
    // 初始化线程本地存储
    context->is_thread_local = false;
    context->thread_data = NULL;
    
    // 初始化统计和调试
    context->enable_statistics = true;
    context->enable_debugging = false;
    context->statistics_data = NULL;
    context->debug_data = NULL;
    
    // 初始化引用计数
    context->reference_count = 1;
    
    // 初始化状态标志
    context->is_valid = true;
    context->is_destroying = false;
    context->is_resetting = false;
    
    return context;
}

/**
 * @brief 销毁内存上下文及其所有内存
 */
void F_context_destroy(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    // 标记为正在销毁
    context->is_destroying = true;
    
    // 递归销毁所有子上下文
    Stru_MemoryContext_t* child = context->first_child;
    while (child != NULL)
    {
        Stru_MemoryContext_t* next = child->next_sibling;
        F_context_destroy(child);
        child = next;
    }
    
    // 从父上下文的子链表中移除
    if (context->parent != NULL)
    {
        Stru_MemoryContext_t* prev = NULL;
        Stru_MemoryContext_t* current = context->parent->first_child;
        
        while (current != NULL)
        {
            if (current == context)
            {
                if (prev == NULL)
                {
                    context->parent->first_child = current->next_sibling;
                }
                else
                {
                    prev->next_sibling = current->next_sibling;
                }
                break;
            }
            
            prev = current;
            current = current->next_sibling;
        }
    }
    
    // 释放所有分配的内存
    Stru_AllocationRecord_t* record = context->allocations;
    while (record != NULL)
    {
        Stru_AllocationRecord_t* next = record->next;
        
        if (context->deallocate_func != NULL)
        {
            context->deallocate_func(context->allocator_data, record->address);
        }
        else
        {
            free(record->address);
        }
        
        free(record);
        record = next;
    }
    
    // 释放分配器数据
    if (context->allocator_data != NULL)
    {
        free(context->allocator_data);
    }
    
    // 释放线程数据
    if (context->thread_data != NULL)
    {
        free(context->thread_data);
    }
    
    // 释放统计和调试数据
    if (context->statistics_data != NULL)
    {
        free(context->statistics_data);
    }
    
    if (context->debug_data != NULL)
    {
        free(context->debug_data);
    }
    
    // 释放上下文名称
    free(context->name);
    
    // 标记为无效并释放上下文结构体
    context->is_valid = false;
    free(context);
}

/**
 * @brief 重置内存上下文
 */
void F_context_reset(Stru_MemoryContext_t* context)
{
    if (context == NULL || !context->is_valid)
    {
        return;
    }
    
    // 标记为正在重置
    context->is_resetting = true;
    
    // 递归重置所有子上下文
    Stru_MemoryContext_t* child = context->first_child;
    while (child != NULL)
    {
        F_context_reset(child);
        child = child->next_sibling;
    }
    
    // 释放所有分配的内存
    Stru_AllocationRecord_t* record = context->allocations;
    while (record != NULL)
    {
        Stru_AllocationRecord_t* next = record->next;
        
        if (context->deallocate_func != NULL)
        {
            context->deallocate_func(context->allocator_data, record->address);
        }
        else
        {
            free(record->address);
        }
        
        free(record);
        record = next;
    }
    
    // 重置内存管理状态
    context->allocations = NULL;
    context->total_allocated = 0;
    context->peak_allocated = 0;
    context->current_usage = 0;
    context->allocation_count = 0;
    
    // 清除重置标志
    context->is_resetting = false;
}
