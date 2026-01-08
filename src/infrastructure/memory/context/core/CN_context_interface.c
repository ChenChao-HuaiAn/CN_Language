/**
 * @file CN_context_interface.c
 * @brief 内存上下文接口模块
 * 
 * 实现了内存上下文接口的初始化和全局函数。
 * 包含全局状态管理和接口实例的创建。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_context_core.h"
#include "CN_context_interface.h"
#include "CN_context_struct.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 全局变量定义
// ============================================================================

/** 全局内存上下文接口实例 */
Stru_MemoryContextInterface_t g_memory_context_interface = {0};

/** 默认根上下文 */
Stru_MemoryContext_t* g_root_context = NULL;

/** 线程本地当前上下文 */
__declspec(thread) Stru_MemoryContext_t* g_thread_current_context = NULL;

/** 上下文ID计数器 */
uint64_t g_next_context_id = 1;

/** 系统初始化标志 */
bool g_system_initialized = false;

// ============================================================================
// 函数声明（在其他模块中定义）
// ============================================================================

// 上下文管理函数
extern Stru_MemoryContext_t* F_context_create(const char* name,
                                             Stru_MemoryContext_t* parent);
extern void F_context_destroy(Stru_MemoryContext_t* context);
extern void F_context_reset(Stru_MemoryContext_t* context);

// 上下文操作函数
extern Stru_MemoryContext_t* F_context_get_current(void);
extern void F_context_set_current(Stru_MemoryContext_t* context);
extern Stru_MemoryContext_t* F_context_get_parent(Stru_MemoryContext_t* context);
extern Stru_MemoryContext_t* F_context_get_first_child(Stru_MemoryContext_t* context);
extern Stru_MemoryContext_t* F_context_get_next_sibling(Stru_MemoryContext_t* context);
extern const char* F_context_get_name(Stru_MemoryContext_t* context);
extern uint64_t F_context_get_id(Stru_MemoryContext_t* context);
extern bool F_context_is_valid(Stru_MemoryContext_t* context);
extern uint64_t F_context_get_creation_time(Stru_MemoryContext_t* context);
extern size_t F_context_get_depth(Stru_MemoryContext_t* context);

// 内存分配函数
extern void* F_context_allocate(Stru_MemoryContext_t* context,
                               size_t size, size_t alignment);
extern void* F_context_reallocate(Stru_MemoryContext_t* context,
                                 void* ptr, size_t new_size);
extern void F_context_deallocate(Stru_MemoryContext_t* context, void* ptr);

// 内存管理函数
extern size_t F_context_get_allocated_size(Stru_MemoryContext_t* context,
                                          void* ptr);
extern size_t F_context_get_total_allocated(Stru_MemoryContext_t* context);
extern size_t F_context_get_peak_allocated(Stru_MemoryContext_t* context);
extern size_t F_context_get_current_usage(Stru_MemoryContext_t* context);

// ============================================================================
// 接口初始化函数
// ============================================================================

/**
 * @brief 初始化内存上下文接口
 */
static bool F_initialize_context_interface(void)
{
    // 检查是否已初始化
    if (g_system_initialized)
    {
        return true;
    }
    
    // 初始化接口函数指针
    g_memory_context_interface.create = F_context_create;
    g_memory_context_interface.destroy = F_context_destroy;
    g_memory_context_interface.reset = F_context_reset;
    
    g_memory_context_interface.get_current = F_context_get_current;
    g_memory_context_interface.set_current = F_context_set_current;
    g_memory_context_interface.get_parent = F_context_get_parent;
    g_memory_context_interface.get_first_child = F_context_get_first_child;
    g_memory_context_interface.get_next_sibling = F_context_get_next_sibling;
    
    g_memory_context_interface.allocate = F_context_allocate;
    g_memory_context_interface.reallocate = F_context_reallocate;
    g_memory_context_interface.deallocate = F_context_deallocate;
    
    g_memory_context_interface.get_allocated_size = F_context_get_allocated_size;
    g_memory_context_interface.get_total_allocated = F_context_get_total_allocated;
    g_memory_context_interface.get_peak_allocated = F_context_get_peak_allocated;
    g_memory_context_interface.get_current_usage = F_context_get_current_usage;
    
    g_memory_context_interface.get_name = F_context_get_name;
    g_memory_context_interface.get_id = F_context_get_id;
    g_memory_context_interface.is_valid = F_context_is_valid;
    g_memory_context_interface.get_creation_time = F_context_get_creation_time;
    g_memory_context_interface.get_depth = F_context_get_depth;
    
    g_memory_context_interface.cleanup = NULL; // 在shutdown中设置
    g_memory_context_interface.private_data = NULL;
    
    // 创建默认根上下文
    g_root_context = F_context_create("RootContext", NULL);
    if (g_root_context == NULL)
    {
        return false;
    }
    
    g_system_initialized = true;
    return true;
}

/**
 * @brief 清理内存上下文接口
 */
static void F_cleanup_context_interface(void)
{
    if (!g_system_initialized)
    {
        return;
    }
    
    // 销毁根上下文
    if (g_root_context != NULL)
    {
        F_context_destroy(g_root_context);
        g_root_context = NULL;
    }
    
    // 重置接口
    memset(&g_memory_context_interface, 0, sizeof(g_memory_context_interface));
    
    // 重置线程本地上下文
    g_thread_current_context = NULL;
    
    // 重置ID计数器
    g_next_context_id = 1;
    
    g_system_initialized = false;
}

// ============================================================================
// 全局函数实现
// ============================================================================

/**
 * @brief 获取内存上下文接口实例
 */
Stru_MemoryContextInterface_t* F_get_memory_context_interface(void)
{
    // 如果未初始化，先初始化
    if (!g_system_initialized)
    {
        if (!F_initialize_context_interface())
        {
            return NULL;
        }
    }
    
    return &g_memory_context_interface;
}

/**
 * @brief 初始化内存上下文系统
 */
bool F_initialize_memory_context_system(void)
{
    return F_initialize_context_interface();
}

/**
 * @brief 关闭内存上下文系统
 */
void F_shutdown_memory_context_system(void)
{
    F_cleanup_context_interface();
}

/**
 * @brief 获取默认根上下文
 */
Stru_MemoryContext_t* F_get_root_context(void)
{
    if (!g_system_initialized)
    {
        if (!F_initialize_context_interface())
        {
            return NULL;
        }
    }
    
    return g_root_context;
}

/**
 * @brief 设置默认根上下文
 */
void F_set_root_context(Stru_MemoryContext_t* root_context)
{
    if (g_root_context != NULL && g_root_context != root_context)
    {
        F_context_destroy(g_root_context);
    }
    
    g_root_context = root_context;
}

/**
 * @brief 验证上下文是否有效
 */
bool F_validate_context(Stru_MemoryContext_t* context)
{
    return F_context_is_valid(context);
}

/**
 * @brief 验证上下文名称是否有效
 */
bool F_validate_context_name(const char* name)
{
    if (name == NULL)
    {
        return false;
    }
    
    // 检查名称长度
    size_t length = strlen(name);
    if (length == 0 || length > 256)
    {
        return false;
    }
    
    // 检查名称是否只包含有效字符
    for (size_t i = 0; i < length; i++)
    {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.'))
        {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 验证内存地址是否属于指定上下文
 */
bool F_validate_memory_address(Stru_MemoryContext_t* context, void* address)
{
    if (context == NULL || address == NULL)
    {
        return false;
    }
    
    // 查找分配记录
    Stru_AllocationRecord_t* record = context->allocations;
    while (record != NULL)
    {
        if (record->address == address)
        {
            return true;
        }
        record = record->next;
    }
    
    return false;
}
