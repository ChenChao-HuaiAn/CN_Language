/******************************************************************************
 * 文件名: CN_platform_windows_interrupt_tools.c
 * 功能: CN_Language Windows平台中断工具实现
 * 描述: 提供Windows平台中断处理工具功能，包括上下文管理、栈管理、
 *       延迟处理、中断过滤器、中断日志和性能分析等。
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现Windows平台中断工具功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows_interrupt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* MSVC内部函数头文件 */
#ifdef _MSC_VER
#include <intrin.h>
#endif

/* 内部数据结构定义 */

/** Windows中断工具内部状态 */
static Stru_CN_WindowsInterruptToolsInfo_t g_windows_interrupt_tools_info = {
    .is_initialized = false,
    .in_interrupt_context = false,
    .interrupt_thread_id = 0,
    .interrupt_count = 0,
    .nested_interrupt_depth = 0
};

/** 延迟处理任务结构体 */
typedef struct Stru_CN_WindowsDeferredTask_t
{
    CN_InterruptHandler_t handler;          /**< 延迟处理函数 */
    void* context;                          /**< 处理函数上下文 */
    uint32_t delay_ms;                      /**< 延迟时间（毫秒） */
    DWORD scheduled_time;                   /**< 计划执行时间 */
    bool is_active;                         /**< 是否激活 */
    struct Stru_CN_WindowsDeferredTask_t* next; /**< 下一个任务 */
} Stru_CN_WindowsDeferredTask_t;

/** 中断过滤器结构体 */
typedef struct Stru_CN_WindowsInterruptFilter_t
{
    CN_InterruptNumber_t interrupt;         /**< 中断号 */
    bool (*filter_func)(CN_InterruptNumber_t, void*); /**< 过滤函数 */
    void* filter_context;                   /**< 过滤函数上下文 */
    bool is_installed;                      /**< 是否已安装 */
} Stru_CN_WindowsInterruptFilter_t;

/** 中断日志条目结构体 */
typedef struct Stru_CN_WindowsInterruptLogEntry_t
{
    CN_InterruptNumber_t interrupt;         /**< 中断号 */
    uint64_t timestamp;                     /**< 时间戳 */
    uint32_t log_level;                     /**< 日志级别 */
    char message[256];                      /**< 日志消息 */
} Stru_CN_WindowsInterruptLogEntry_t;

/** 中断性能分析数据 */
typedef struct Stru_CN_WindowsInterruptProfileData_t
{
    CN_InterruptNumber_t interrupt;         /**< 中断号 */
    uint64_t start_time;                    /**< 开始时间 */
    uint64_t end_time;                      /**< 结束时间 */
    uint64_t total_time;                    /**< 总时间 */
    uint64_t call_count;                    /**< 调用次数 */
    uint64_t max_time;                      /**< 最大时间 */
    uint64_t min_time;                      /**< 最小时间 */
    uint64_t avg_time;                      /**< 平均时间 */
} Stru_CN_WindowsInterruptProfileData_t;

/* 静态全局变量 */

/** 延迟处理任务列表 */
static Stru_CN_WindowsDeferredTask_t* g_deferred_task_list = NULL;
static CRITICAL_SECTION g_deferred_task_lock;

/** 中断过滤器数组 */
static Stru_CN_WindowsInterruptFilter_t g_interrupt_filters[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
static CRITICAL_SECTION g_filter_lock;

/** 中断日志缓冲区 */
static Stru_CN_WindowsInterruptLogEntry_t g_interrupt_log_buffer[1024];
static uint32_t g_interrupt_log_index = 0;
static bool g_interrupt_logging_enabled = false;
static CRITICAL_SECTION g_log_lock;

/** 中断性能分析数据 */
static Stru_CN_WindowsInterruptProfileData_t g_interrupt_profile_data[CN_WINDOWS_MAX_INTERRUPT_HANDLERS];
static bool g_interrupt_profiling_enabled = false;
static CRITICAL_SECTION g_profile_lock;

/* 静态函数声明 */

// 中断工具函数
static bool windows_interrupt_tools_save_context(Stru_CN_InterruptContext_t* context);
static bool windows_interrupt_tools_restore_context(const Stru_CN_InterruptContext_t* context);
static bool windows_interrupt_tools_copy_context(Stru_CN_InterruptContext_t* dest, const Stru_CN_InterruptContext_t* src);
static void* windows_interrupt_tools_allocate_interrupt_stack(size_t size);
static bool windows_interrupt_tools_free_interrupt_stack(void* stack);
static bool windows_interrupt_tools_check_stack_overflow(void* stack_base, size_t stack_size);
static bool windows_interrupt_tools_schedule_deferred(CN_InterruptHandler_t handler, void* context, uint32_t delay_ms);
static bool windows_interrupt_tools_cancel_deferred(CN_InterruptHandler_t handler);
static bool windows_interrupt_tools_process_deferred(void);
static bool windows_interrupt_tools_install_filter(CN_InterruptNumber_t irq, 
                                                  bool (*filter)(CN_InterruptNumber_t, void*), 
                                                  void* filter_context);
static bool windows_interrupt_tools_remove_filter(CN_InterruptNumber_t irq);
static bool windows_interrupt_tools_enable_logging(CN_InterruptNumber_t irq, uint32_t log_level);
static bool windows_interrupt_tools_disable_logging(CN_InterruptNumber_t irq);
static bool windows_interrupt_tools_get_log_entries(CN_InterruptNumber_t irq, void* log_buffer, size_t buffer_size, size_t* entry_count);
static bool windows_interrupt_tools_start_profiling(CN_InterruptNumber_t irq);
static bool windows_interrupt_tools_stop_profiling(CN_InterruptNumber_t irq);
static bool windows_interrupt_tools_get_profile_data(CN_InterruptNumber_t irq, void* profile_buffer, size_t buffer_size);

// 工具函数
static bool windows_interrupt_tools_initialize_internal(void);
static void windows_interrupt_tools_cleanup_internal(void);
static DWORD windows_get_current_time_ms(void);
static void windows_add_log_entry(CN_InterruptNumber_t irq, uint32_t log_level, const char* message);
static void windows_update_profile_data(CN_InterruptNumber_t irq, uint64_t start_time, uint64_t end_time);

/* 全局接口变量定义 */

/** Windows中断工具接口 */
Stru_CN_InterruptToolsInterface_t g_windows_interrupt_tools_interface = {
    .save_context = windows_interrupt_tools_save_context,
    .restore_context = windows_interrupt_tools_restore_context,
    .copy_context = windows_interrupt_tools_copy_context,
    .allocate_interrupt_stack = windows_interrupt_tools_allocate_interrupt_stack,
    .free_interrupt_stack = windows_interrupt_tools_free_interrupt_stack,
    .check_stack_overflow = windows_interrupt_tools_check_stack_overflow,
    .schedule_deferred = windows_interrupt_tools_schedule_deferred,
    .cancel_deferred = windows_interrupt_tools_cancel_deferred,
    .process_deferred = windows_interrupt_tools_process_deferred,
    .install_filter = windows_interrupt_tools_install_filter,
    .remove_filter = windows_interrupt_tools_remove_filter,
    .enable_logging = windows_interrupt_tools_enable_logging,
    .disable_logging = windows_interrupt_tools_disable_logging,
    .get_log_entries = windows_interrupt_tools_get_log_entries,
    .start_profiling = windows_interrupt_tools_start_profiling,
    .stop_profiling = windows_interrupt_tools_stop_profiling,
    .get_profile_data = windows_interrupt_tools_get_profile_data
};

/******************************************************************************
 * 中断工具函数实现
 ******************************************************************************/

/**
 * @brief 初始化Windows中断工具内部状态
 */
static bool windows_interrupt_tools_initialize_internal(void)
{
    if (g_windows_interrupt_tools_info.is_initialized) {
        return true;
    }
    
    // 初始化临界区
    InitializeCriticalSection(&g_deferred_task_lock);
    InitializeCriticalSection(&g_filter_lock);
    InitializeCriticalSection(&g_log_lock);
    InitializeCriticalSection(&g_profile_lock);
    
    // 初始化延迟任务列表
    g_deferred_task_list = NULL;
    
    // 初始化过滤器数组
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_interrupt_filters[i].interrupt = 0;
        g_interrupt_filters[i].filter_func = NULL;
        g_interrupt_filters[i].filter_context = NULL;
        g_interrupt_filters[i].is_installed = false;
    }
    
    // 初始化日志缓冲区
    for (uint32_t i = 0; i < 1024; i++) {
        g_interrupt_log_buffer[i].interrupt = 0;
        g_interrupt_log_buffer[i].timestamp = 0;
        g_interrupt_log_buffer[i].log_level = 0;
        g_interrupt_log_buffer[i].message[0] = '\0';
    }
    g_interrupt_log_index = 0;
    g_interrupt_logging_enabled = false;
    
    // 初始化性能分析数据
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        g_interrupt_profile_data[i].interrupt = 0;
        g_interrupt_profile_data[i].start_time = 0;
        g_interrupt_profile_data[i].end_time = 0;
        g_interrupt_profile_data[i].total_time = 0;
        g_interrupt_profile_data[i].call_count = 0;
        g_interrupt_profile_data[i].max_time = 0;
        g_interrupt_profile_data[i].min_time = UINT64_MAX;
        g_interrupt_profile_data[i].avg_time = 0;
    }
    g_interrupt_profiling_enabled = false;
    
    // 获取当前线程ID
    g_windows_interrupt_tools_info.interrupt_thread_id = GetCurrentThreadId();
    
    g_windows_interrupt_tools_info.is_initialized = true;
    
    return true;
}

/**
 * @brief 清理Windows中断工具内部状态
 */
static void windows_interrupt_tools_cleanup_internal(void)
{
    if (!g_windows_interrupt_tools_info.is_initialized) {
        return;
    }
    
    // 清理延迟任务列表
    EnterCriticalSection(&g_deferred_task_lock);
    Stru_CN_WindowsDeferredTask_t* current = g_deferred_task_list;
    while (current != NULL) {
        Stru_CN_WindowsDeferredTask_t* next = current->next;
        free(current);
        current = next;
    }
    g_deferred_task_list = NULL;
    LeaveCriticalSection(&g_deferred_task_lock);
    
    // 删除临界区
    DeleteCriticalSection(&g_deferred_task_lock);
    DeleteCriticalSection(&g_filter_lock);
    DeleteCriticalSection(&g_log_lock);
    DeleteCriticalSection(&g_profile_lock);
    
    g_windows_interrupt_tools_info.is_initialized = false;
}

/**
 * @brief 保存中断上下文
 */
static bool windows_interrupt_tools_save_context(Stru_CN_InterruptContext_t* context)
{
    if (!context) {
        return false;
    }
    
    // 保存当前寄存器状态（简化实现，实际需要保存完整上下文）
    // 注意：在Windows用户模式下，我们无法直接访问所有寄存器
    // 这里使用一个简化的实现，实际应用中可能需要使用结构化异常处理
    
    // 获取当前指令指针（简化实现）
    uintptr_t ip = 0;
    uintptr_t sp = 0;
    uintptr_t bp = 0;
    
    // 使用内联汇编获取寄存器值（MSVC语法）
    // 注意：在64位Windows上，内联汇编不可用，这里使用简化实现
    // 对于实际应用，可能需要使用结构化异常处理或平台特定API
    
    // 简化实现：使用函数返回地址作为指令指针的近似值
    // 注意：_ReturnAddress和_AddressOfReturnAddress是MSVC内部函数
    // 对于GCC/MinGW，使用不同的实现
    #ifdef _MSC_VER
        ip = (uintptr_t)_ReturnAddress();
    #else
        // GCC/MinGW实现：使用__builtin_return_address
        ip = (uintptr_t)__builtin_return_address(0);
    #endif
    
    // 获取栈指针和基址指针
    // 注意：在Windows用户模式下，我们无法直接访问所有寄存器
    // 这里使用简化实现，实际应用中可能需要平台特定代码
    
    // 使用内联汇编获取栈指针和基址指针（x86架构）
    // 注意：在64位Windows上，内联汇编不可用
    #ifdef _M_IX86
        #ifdef _MSC_VER
            __asm {
                mov sp, esp
                mov bp, ebp
            }
        #else
            // GCC内联汇编语法
            __asm__ __volatile__ (
                "movl %%esp, %0\n"
                "movl %%ebp, %1\n"
                : "=r"(sp), "=r"(bp)
                :
                : "memory"
            );
        #endif
    #else
        // 对于x64架构，使用简化实现
        // 实际应用中可能需要使用RtlCaptureContext和适当的字段
        #ifdef _MSC_VER
            sp = (uintptr_t)_AddressOfReturnAddress();
        #else
            // GCC/MinGW实现：使用__builtin_frame_address
            sp = (uintptr_t)__builtin_frame_address(0);
        #endif
        bp = sp; // 简化实现
    #endif
    
    context->ip = ip;
    context->sp = sp;
    context->bp = bp;
    
    // 标记为在中断上下文中
    g_windows_interrupt_tools_info.in_interrupt_context = true;
    g_windows_interrupt_tools_info.nested_interrupt_depth++;
    g_windows_interrupt_tools_info.interrupt_count++;
    
    return true;
}

/**
 * @brief 恢复中断上下文
 */
static bool windows_interrupt_tools_restore_context(const Stru_CN_InterruptContext_t* context)
{
    if (!context) {
        return false;
    }
    
    // 恢复嵌套深度
    if (g_windows_interrupt_tools_info.nested_interrupt_depth > 0) {
        g_windows_interrupt_tools_info.nested_interrupt_depth--;
    }
    
    // 如果嵌套深度为0，标记为不在中断上下文中
    if (g_windows_interrupt_tools_info.nested_interrupt_depth == 0) {
        g_windows_interrupt_tools_info.in_interrupt_context = false;
    }
    
    return true;
}

/**
 * @brief 复制中断上下文
 */
static bool windows_interrupt_tools_copy_context(Stru_CN_InterruptContext_t* dest, const Stru_CN_InterruptContext_t* src)
{
    if (!dest || !src) {
        return false;
    }
    
    memcpy(dest, src, sizeof(Stru_CN_InterruptContext_t));
    return true;
}

/**
 * @brief 分配中断栈
 */
static void* windows_interrupt_tools_allocate_interrupt_stack(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    // 使用VirtualAlloc分配可执行栈内存
    void* stack = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!stack) {
        return NULL;
    }
    
    // 设置栈保护页
    DWORD old_protect;
    if (!VirtualProtect(stack, 4096, PAGE_READWRITE | PAGE_GUARD, &old_protect)) {
        VirtualFree(stack, 0, MEM_RELEASE);
        return NULL;
    }
    
    return stack;
}

/**
 * @brief 释放中断栈
 */
static bool windows_interrupt_tools_free_interrupt_stack(void* stack)
{
    if (!stack) {
        return false;
    }
    
    return VirtualFree(stack, 0, MEM_RELEASE) != 0;
}

/**
 * @brief 检查栈溢出
 */
static bool windows_interrupt_tools_check_stack_overflow(void* stack_base, size_t stack_size)
{
    if (!stack_base || stack_size == 0) {
        return false;
    }
    
    // 获取当前栈指针
    uintptr_t current_stack = 0;
    
    // 使用内联汇编获取栈指针（x86架构）
    #ifdef _M_IX86
        #ifdef _MSC_VER
            __asm {
                mov current_stack, esp
            }
        #else
            // GCC内联汇编语法
            __asm__ __volatile__ (
                "movl %%esp, %0\n"
                : "=r"(current_stack)
                :
                : "memory"
            );
        #endif
    #else
        // 对于x64架构，使用简化实现
        #ifdef _MSC_VER
            current_stack = (uintptr_t)_AddressOfReturnAddress();
        #else
            // GCC/MinGW实现：使用__builtin_frame_address
            current_stack = (uintptr_t)__builtin_frame_address(0);
        #endif
    #endif
    
    // 计算栈边界
    uintptr_t stack_start = (uintptr_t)stack_base;
    uintptr_t stack_end = stack_start + stack_size;
    
    // 检查当前栈指针是否在栈边界内
    if (current_stack < stack_start || current_stack >= stack_end) {
        return true; // 栈溢出
    }
    
    // 检查栈使用率（如果接近边界）
    uintptr_t stack_usage = stack_start - current_stack; // 栈向下增长
    if (stack_usage > (stack_size * 9 / 10)) {
        return true; // 栈使用率超过90%
    }
    
    return false;
}

/**
 * @brief 调度延迟处理
 */
static bool windows_interrupt_tools_schedule_deferred(CN_InterruptHandler_t handler, void* context, uint32_t delay_ms)
{
    if (!handler) {
        return false;
    }
    
    // 创建新的延迟任务
    Stru_CN_WindowsDeferredTask_t* new_task = (Stru_CN_WindowsDeferredTask_t*)malloc(sizeof(Stru_CN_WindowsDeferredTask_t));
    if (!new_task) {
        return false;
    }
    
    new_task->handler = handler;
    new_task->context = context;
    new_task->delay_ms = delay_ms;
    new_task->scheduled_time = windows_get_current_time_ms() + delay_ms;
    new_task->is_active = true;
    new_task->next = NULL;
    
    // 添加到延迟任务列表
    EnterCriticalSection(&g_deferred_task_lock);
    
    if (g_deferred_task_list == NULL) {
        g_deferred_task_list = new_task;
    } else {
        // 按计划时间排序插入
        Stru_CN_WindowsDeferredTask_t* current = g_deferred_task_list;
        Stru_CN_WindowsDeferredTask_t* prev = NULL;
        
        while (current != NULL && current->scheduled_time <= new_task->scheduled_time) {
            prev = current;
            current = current->next;
        }
        
        if (prev == NULL) {
            // 插入到列表头部
            new_task->next = g_deferred_task_list;
            g_deferred_task_list = new_task;
        } else {
            // 插入到中间或尾部
            new_task->next = current;
            prev->next = new_task;
        }
    }
    
    LeaveCriticalSection(&g_deferred_task_lock);
    
    return true;
}

/**
 * @brief 取消延迟处理
 */
static bool windows_interrupt_tools_cancel_deferred(CN_InterruptHandler_t handler)
{
    if (!handler) {
        return false;
    }
    
    bool found = false;
    
    EnterCriticalSection(&g_deferred_task_lock);
    
    Stru_CN_WindowsDeferredTask_t* current = g_deferred_task_list;
    Stru_CN_WindowsDeferredTask_t* prev = NULL;
    
    while (current != NULL) {
        if (current->handler == handler && current->is_active) {
            found = true;
            
            if (prev == NULL) {
                // 删除列表头部
                g_deferred_task_list = current->next;
                free(current);
                current = g_deferred_task_list;
            } else {
                // 删除中间或尾部节点
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    LeaveCriticalSection(&g_deferred_task_lock);
    
    return found;
}

/**
 * @brief 处理延迟任务
 */
static bool windows_interrupt_tools_process_deferred(void)
{
    DWORD current_time = windows_get_current_time_ms();
    bool processed = false;
    
    EnterCriticalSection(&g_deferred_task_lock);
    
    Stru_CN_WindowsDeferredTask_t* current = g_deferred_task_list;
    Stru_CN_WindowsDeferredTask_t* prev = NULL;
    
    while (current != NULL) {
        if (current->is_active && current->scheduled_time <= current_time) {
            // 执行延迟任务
            if (current->handler) {
                current->handler(current->context);
            }
            
            // 标记为已处理
            current->is_active = false;
            processed = true;
            
            // 从列表中移除
            if (prev == NULL) {
                g_deferred_task_list = current->next;
                free(current);
                current = g_deferred_task_list;
            } else {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    LeaveCriticalSection(&g_deferred_task_lock);
    
    return processed;
}

/**
 * @brief 安装中断过滤器
 */
static bool windows_interrupt_tools_install_filter(CN_InterruptNumber_t irq, 
                                                  bool (*filter)(CN_InterruptNumber_t, void*), 
                                                  void* filter_context)
{
    if (!filter) {
        return false;
    }
    
    EnterCriticalSection(&g_filter_lock);
    
    // 查找空闲的过滤器槽位
    int free_slot = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (!g_interrupt_filters[i].is_installed) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == -1) {
        LeaveCriticalSection(&g_filter_lock);
        return false;
    }
    
    // 安装过滤器
    g_interrupt_filters[free_slot].interrupt = irq;
    g_interrupt_filters[free_slot].filter_func = filter;
    g_interrupt_filters[free_slot].filter_context = filter_context;
    g_interrupt_filters[free_slot].is_installed = true;
    
    LeaveCriticalSection(&g_filter_lock);
    
    return true;
}

/**
 * @brief 移除中断过滤器
 */
static bool windows_interrupt_tools_remove_filter(CN_InterruptNumber_t irq)
{
    EnterCriticalSection(&g_filter_lock);
    
    bool found = false;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_filters[i].is_installed && g_interrupt_filters[i].interrupt == irq) {
            g_interrupt_filters[i].interrupt = 0;
            g_interrupt_filters[i].filter_func = NULL;
            g_interrupt_filters[i].filter_context = NULL;
            g_interrupt_filters[i].is_installed = false;
            found = true;
            break;
        }
    }
    
    LeaveCriticalSection(&g_filter_lock);
    
    return found;
}

/**
 * @brief 启用中断日志
 */
static bool windows_interrupt_tools_enable_logging(CN_InterruptNumber_t irq, uint32_t log_level)
{
    if (log_level == 0) {
        return false;
    }
    
    EnterCriticalSection(&g_log_lock);
    
    g_interrupt_logging_enabled = true;
    
    // 添加日志条目
    char message[256];
    snprintf(message, sizeof(message), "中断日志已启用，中断号: %u，日志级别: %u", 
             (unsigned int)irq, log_level);
    windows_add_log_entry(irq, log_level, message);
    
    LeaveCriticalSection(&g_log_lock);
    
    return true;
}

/**
 * @brief 禁用中断日志
 */
static bool windows_interrupt_tools_disable_logging(CN_InterruptNumber_t irq)
{
    EnterCriticalSection(&g_log_lock);
    
    g_interrupt_logging_enabled = false;
    
    // 添加日志条目
    char message[256];
    snprintf(message, sizeof(message), "中断日志已禁用，中断号: %u", (unsigned int)irq);
    windows_add_log_entry(irq, 1, message);
    
    LeaveCriticalSection(&g_log_lock);
    
    return true;
}

/**
 * @brief 获取日志条目
 */
static bool windows_interrupt_tools_get_log_entries(CN_InterruptNumber_t irq, void* log_buffer, size_t buffer_size, size_t* entry_count)
{
    if (!log_buffer || !entry_count || buffer_size == 0) {
        return false;
    }
    
    EnterCriticalSection(&g_log_lock);
    
    size_t max_entries = buffer_size / sizeof(Stru_CN_WindowsInterruptLogEntry_t);
    size_t copied_entries = 0;
    
    // 复制匹配的日志条目
    for (uint32_t i = 0; i < 1024 && copied_entries < max_entries; i++) {
        uint32_t index = (g_interrupt_log_index + i) % 1024;
        if (g_interrupt_log_buffer[index].interrupt == irq || irq == 0) {
            memcpy((char*)log_buffer + copied_entries * sizeof(Stru_CN_WindowsInterruptLogEntry_t),
                  &g_interrupt_log_buffer[index], sizeof(Stru_CN_WindowsInterruptLogEntry_t));
            copied_entries++;
        }
    }
    
    *entry_count = copied_entries;
    
    LeaveCriticalSection(&g_log_lock);
    
    return true;
}

/**
 * @brief 开始性能分析
 */
static bool windows_interrupt_tools_start_profiling(CN_InterruptNumber_t irq)
{
    EnterCriticalSection(&g_profile_lock);
    
    // 查找或创建性能分析数据
    int slot = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_profile_data[i].interrupt == irq) {
            slot = i;
            break;
        }
        if (slot == -1 && g_interrupt_profile_data[i].interrupt == 0) {
            slot = i;
        }
    }
    
    if (slot == -1) {
        LeaveCriticalSection(&g_profile_lock);
        return false;
    }
    
    // 初始化性能分析数据
    g_interrupt_profile_data[slot].interrupt = irq;
    g_interrupt_profile_data[slot].start_time = 0;
    g_interrupt_profile_data[slot].end_time = 0;
    g_interrupt_profile_data[slot].total_time = 0;
    g_interrupt_profile_data[slot].call_count = 0;
    g_interrupt_profile_data[slot].max_time = 0;
    g_interrupt_profile_data[slot].min_time = UINT64_MAX;
    g_interrupt_profile_data[slot].avg_time = 0;
    
    g_interrupt_profiling_enabled = true;
    
    LeaveCriticalSection(&g_profile_lock);
    
    return true;
}

/**
 * @brief 停止性能分析
 */
static bool windows_interrupt_tools_stop_profiling(CN_InterruptNumber_t irq)
{
    EnterCriticalSection(&g_profile_lock);
    
    // 查找性能分析数据
    int slot = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_profile_data[i].interrupt == irq) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LeaveCriticalSection(&g_profile_lock);
        return false;
    }
    
    // 停止性能分析
    g_interrupt_profile_data[slot].end_time = windows_get_current_time_ms();
    
    // 更新性能分析数据
    if (g_interrupt_profile_data[slot].start_time > 0) {
        windows_update_profile_data(irq, g_interrupt_profile_data[slot].start_time, 
                                   g_interrupt_profile_data[slot].end_time);
    }
    
    LeaveCriticalSection(&g_profile_lock);
    
    return true;
}

/**
 * @brief 获取性能分析数据
 */
static bool windows_interrupt_tools_get_profile_data(CN_InterruptNumber_t irq, void* profile_buffer, size_t buffer_size)
{
    if (!profile_buffer || buffer_size < sizeof(Stru_CN_WindowsInterruptProfileData_t)) {
        return false;
    }
    
    EnterCriticalSection(&g_profile_lock);
    
    // 查找性能分析数据
    int slot = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_profile_data[i].interrupt == irq) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LeaveCriticalSection(&g_profile_lock);
        return false;
    }
    
    // 复制性能分析数据
    memcpy(profile_buffer, &g_interrupt_profile_data[slot], sizeof(Stru_CN_WindowsInterruptProfileData_t));
    
    LeaveCriticalSection(&g_profile_lock);
    
    return true;
}

/**
 * @brief 获取当前时间（毫秒）
 */
static DWORD windows_get_current_time_ms(void)
{
    return GetTickCount();
}

/**
 * @brief 添加日志条目
 */
static void windows_add_log_entry(CN_InterruptNumber_t irq, uint32_t log_level, const char* message)
{
    if (!g_interrupt_logging_enabled) {
        return;
    }
    
    EnterCriticalSection(&g_log_lock);
    
    // 添加新的日志条目
    g_interrupt_log_buffer[g_interrupt_log_index].interrupt = irq;
    g_interrupt_log_buffer[g_interrupt_log_index].timestamp = windows_get_current_time_ms();
    g_interrupt_log_buffer[g_interrupt_log_index].log_level = log_level;
    
    if (message) {
        strncpy(g_interrupt_log_buffer[g_interrupt_log_index].message, message, 255);
        g_interrupt_log_buffer[g_interrupt_log_index].message[255] = '\0';
    } else {
        g_interrupt_log_buffer[g_interrupt_log_index].message[0] = '\0';
    }
    
    // 更新索引
    g_interrupt_log_index = (g_interrupt_log_index + 1) % 1024;
    
    LeaveCriticalSection(&g_log_lock);
}

/**
 * @brief 更新性能分析数据
 */
static void windows_update_profile_data(CN_InterruptNumber_t irq, uint64_t start_time, uint64_t end_time)
{
    if (!g_interrupt_profiling_enabled) {
        return;
    }
    
    EnterCriticalSection(&g_profile_lock);
    
    // 查找性能分析数据
    int slot = -1;
    for (uint32_t i = 0; i < CN_WINDOWS_MAX_INTERRUPT_HANDLERS; i++) {
        if (g_interrupt_profile_data[i].interrupt == irq) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        LeaveCriticalSection(&g_profile_lock);
        return;
    }
    
    // 更新性能分析数据
    uint64_t duration = end_time - start_time;
    
    g_interrupt_profile_data[slot].call_count++;
    g_interrupt_profile_data[slot].total_time += duration;
    
    if (duration > g_interrupt_profile_data[slot].max_time) {
        g_interrupt_profile_data[slot].max_time = duration;
    }
    
    if (duration < g_interrupt_profile_data[slot].min_time) {
        g_interrupt_profile_data[slot].min_time = duration;
    }
    
    if (g_interrupt_profile_data[slot].call_count > 0) {
        g_interrupt_profile_data[slot].avg_time = g_interrupt_profile_data[slot].total_time / g_interrupt_profile_data[slot].call_count;
    }
    
    LeaveCriticalSection(&g_profile_lock);
}

/**
 * @brief 获取Windows平台中断工具接口
 */
Stru_CN_InterruptToolsInterface_t* CN_platform_windows_get_interrupt_tools(void)
{
    // 确保内部状态已初始化
    if (!g_windows_interrupt_tools_info.is_initialized) {
        windows_interrupt_tools_initialize_internal();
    }
    
    return &g_windows_interrupt_tools_interface;
}
