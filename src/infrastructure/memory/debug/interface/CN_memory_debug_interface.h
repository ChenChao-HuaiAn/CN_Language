/**
 * @file CN_memory_debug_interface.h
 * @brief 内存调试器抽象接口定义
 * 
 * 本文件定义了CN_Language项目的统一内存调试接口，遵循SOLID设计原则，
 * 提供可扩展、可替换的内存调试功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_DEBUG_INTERFACE_H
#define CN_MEMORY_DEBUG_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 内存调试器抽象接口
 * 
 * 定义了内存调试的基本操作，支持多种调试功能：
 * - 内存泄漏检测
 * - 内存错误检测（越界、双重释放、野指针等）
 * - 内存使用分析
 * - 调试工具（内存转储、验证等）
 * 
 * 遵循依赖倒置原则，高层模块通过此接口使用内存调试服务。
 */
typedef struct Stru_MemoryDebuggerInterface_t Stru_MemoryDebuggerInterface_t;

/**
 * @brief 内存调试器操作接口
 */
struct Stru_MemoryDebuggerInterface_t
{
    // ============================================================================
    // 初始化/清理
    // ============================================================================
    
    /**
     * @brief 初始化内存调试器
     * 
     * @param debugger 调试器实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 清理内存调试器资源
     * 
     * @param debugger 调试器实例
     */
    void (*cleanup)(Stru_MemoryDebuggerInterface_t* debugger);
    
    // ============================================================================
    // 内存监控控制
    // ============================================================================
    
    /**
     * @brief 启用或禁用内存监控
     * 
     * @param debugger 调试器实例
     * @param enable 是否启用监控
     */
    void (*enable_monitoring)(Stru_MemoryDebuggerInterface_t* debugger, bool enable);
    
    /**
     * @brief 检查内存监控是否启用
     * 
     * @param debugger 调试器实例
     * @return bool 监控启用返回true，否则返回false
     */
    bool (*is_monitoring_enabled)(Stru_MemoryDebuggerInterface_t* debugger);
    
    // ============================================================================
    // 内存泄漏检测
    // ============================================================================
    
    /**
     * @brief 获取内存泄漏数量
     * 
     * @param debugger 调试器实例
     * @return size_t 泄漏的内存块数量
     */
    size_t (*get_leak_count)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 报告内存泄漏
     * 
     * @param debugger 调试器实例
     */
    void (*report_leaks)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 清除泄漏记录
     * 
     * @param debugger 调试器实例
     */
    void (*clear_leak_records)(Stru_MemoryDebuggerInterface_t* debugger);
    
    // ============================================================================
    // 内存错误检测
    // ============================================================================
    
    /**
     * @brief 启用或禁用缓冲区溢出检查
     * 
     * @param debugger 调试器实例
     * @param enable 是否启用检查
     */
    void (*enable_overflow_check)(Stru_MemoryDebuggerInterface_t* debugger, bool enable);
    
    /**
     * @brief 启用或禁用双重释放检查
     * 
     * @param debugger 调试器实例
     * @param enable 是否启用检查
     */
    void (*enable_double_free_check)(Stru_MemoryDebuggerInterface_t* debugger, bool enable);
    
    /**
     * @brief 启用或禁用未初始化内存检查
     * 
     * @param debugger 调试器实例
     * @param enable 是否启用检查
     */
    void (*enable_uninitialized_check)(Stru_MemoryDebuggerInterface_t* debugger, bool enable);
    
    // ============================================================================
    // 内存验证
    // ============================================================================
    
    /**
     * @brief 验证指针有效性
     * 
     * @param debugger 调试器实例
     * @param ptr 要验证的指针
     * @return bool 指针有效返回true，否则返回false
     */
    bool (*validate_pointer)(Stru_MemoryDebuggerInterface_t* debugger, const void* ptr);
    
    /**
     * @brief 验证内存范围有效性
     * 
     * @param debugger 调试器实例
     * @param ptr 内存起始指针
     * @param size 内存大小
     * @return bool 内存范围有效返回true，否则返回false
     */
    bool (*validate_memory_range)(Stru_MemoryDebuggerInterface_t* debugger, 
                                 const void* ptr, size_t size);
    
    /**
     * @brief 检查所有活动内存分配
     * 
     * @param debugger 调试器实例
     */
    void (*check_all_allocations)(Stru_MemoryDebuggerInterface_t* debugger);
    
    // ============================================================================
    // 调试信息
    // ============================================================================
    
    /**
     * @brief 转储内存信息
     * 
     * @param debugger 调试器实例
     */
    void (*dump_memory_info)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 转储分配统计信息
     * 
     * @param debugger 调试器实例
     */
    void (*dump_allocation_stats)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 设置调试输出函数
     * 
     * @param debugger 调试器实例
     * @param output_func 输出函数指针
     */
    void (*set_debug_output)(Stru_MemoryDebuggerInterface_t* debugger, 
                            void (*output_func)(const char*));
    
    // ============================================================================
    // 调用栈跟踪
    // ============================================================================
    
    /**
     * @brief 启用或禁用调用栈跟踪
     * 
     * @param debugger 调试器实例
     * @param enable 是否启用跟踪
     */
    void (*enable_stack_trace)(Stru_MemoryDebuggerInterface_t* debugger, bool enable);
    
    /**
     * @brief 获取调用栈跟踪深度
     * 
     * @param debugger 调试器实例
     * @return size_t 当前跟踪深度
     */
    size_t (*get_stack_trace_depth)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 设置调用栈跟踪深度
     * 
     * @param debugger 调试器实例
     * @param depth 跟踪深度
     */
    void (*set_stack_trace_depth)(Stru_MemoryDebuggerInterface_t* debugger, size_t depth);
    
    // ============================================================================
    // 性能分析
    // ============================================================================
    
    /**
     * @brief 获取总分配次数
     * 
     * @param debugger 调试器实例
     * @return uint64_t 总分配次数
     */
    uint64_t (*get_total_allocations)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 获取总释放次数
     * 
     * @param debugger 调试器实例
     * @return uint64_t 总释放次数
     */
    uint64_t (*get_total_deallocations)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 获取峰值内存使用量
     * 
     * @param debugger 调试器实例
     * @return size_t 峰值内存使用量（字节）
     */
    size_t (*get_peak_memory_usage)(Stru_MemoryDebuggerInterface_t* debugger);
    
    /**
     * @brief 获取当前内存使用量
     * 
     * @param debugger 调试器实例
     * @return size_t 当前内存使用量（字节）
     */
    size_t (*get_current_memory_usage)(Stru_MemoryDebuggerInterface_t* debugger);
    
    // ============================================================================
    // 私有数据
    // ============================================================================
    
    /**
     * @brief 私有数据指针
     * 
     * 用于存储调试器实现特定的数据。
     */
    void* private_data;
};

#endif // CN_MEMORY_DEBUG_INTERFACE_H
