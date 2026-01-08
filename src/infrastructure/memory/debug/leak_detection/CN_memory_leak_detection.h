/**
 * @file CN_memory_leak_detection.h
 * @brief 内存泄漏检测模块
 * 
 * 本模块提供了内存泄漏检测功能，包括：
 * - 实时监控内存分配和释放
 * - 泄漏报告生成
 * - 调用栈跟踪记录
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_LEAK_DETECTION_H
#define CN_MEMORY_LEAK_DETECTION_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存分配记录
 * 
 * 记录每次内存分配的详细信息。
 */
typedef struct Stru_MemoryAllocationRecord_t
{
    void* address;              ///< 分配的内存地址
    size_t size;                ///< 分配的大小
    const char* file;           ///< 分配所在的源文件
    size_t line;                ///< 分配所在的行号
    const char* function;       ///< 分配所在的函数名
    void* stack_trace[16];      ///< 调用栈跟踪（最多16层）
    size_t stack_depth;         ///< 调用栈深度
    struct Stru_MemoryAllocationRecord_t* next;  ///< 下一个记录
} Stru_MemoryAllocationRecord_t;

/**
 * @brief 泄漏检测器上下文
 * 
 * 泄漏检测器的私有数据。
 */
typedef struct Stru_LeakDetectorContext_t Stru_LeakDetectorContext_t;

/**
 * @brief 创建泄漏检测器
 * 
 * 创建一个新的泄漏检测器实例。
 * 
 * @param enable_stack_trace 是否启用调用栈跟踪
 * @param max_stack_depth 最大调用栈深度（如果启用栈跟踪）
 * @return Stru_LeakDetectorContext_t* 泄漏检测器上下文，失败返回NULL
 */
Stru_LeakDetectorContext_t* F_create_leak_detector(bool enable_stack_trace, size_t max_stack_depth);

/**
 * @brief 销毁泄漏检测器
 * 
 * 销毁泄漏检测器实例，释放相关资源。
 * 
 * @param detector 要销毁的泄漏检测器
 */
void F_destroy_leak_detector(Stru_LeakDetectorContext_t* detector);

/**
 * @brief 记录内存分配
 * 
 * 记录一次内存分配操作。
 * 
 * @param detector 泄漏检测器
 * @param address 分配的内存地址
 * @param size 分配的大小
 * @param file 源文件名（可选）
 * @param line 行号（可选）
 * @param function 函数名（可选）
 */
void F_record_allocation(Stru_LeakDetectorContext_t* detector,
                        void* address, size_t size,
                        const char* file, size_t line,
                        const char* function);

/**
 * @brief 记录内存释放
 * 
 * 记录一次内存释放操作。
 * 
 * @param detector 泄漏检测器
 * @param address 释放的内存地址
 */
void F_record_deallocation(Stru_LeakDetectorContext_t* detector, void* address);

/**
 * @brief 检查内存泄漏
 * 
 * 检查当前是否有内存泄漏，并返回泄漏数量。
 * 
 * @param detector 泄漏检测器
 * @return size_t 泄漏的内存块数量
 */
size_t F_check_leaks(Stru_LeakDetectorContext_t* detector);

/**
 * @brief 生成泄漏报告
 * 
 * 生成详细的内存泄漏报告。
 * 
 * @param detector 泄漏检测器
 * @param report_buffer 报告缓冲区
 * @param buffer_size 缓冲区大小
 */
void F_generate_leak_report(Stru_LeakDetectorContext_t* detector,
                           char* report_buffer, size_t buffer_size);

/**
 * @brief 清除泄漏记录
 * 
 * 清除所有泄漏记录，重新开始检测。
 * 
 * @param detector 泄漏检测器
 */
void F_clear_leak_records(Stru_LeakDetectorContext_t* detector);

/**
 * @brief 获取活动分配数量
 * 
 * 获取当前活动的内存分配数量。
 * 
 * @param detector 泄漏检测器
 * @return size_t 活动分配数量
 */
size_t F_get_active_allocations(Stru_LeakDetectorContext_t* detector);

/**
 * @brief 获取总分配内存
 * 
 * 获取当前活动的总内存分配量。
 * 
 * @param detector 泄漏检测器
 * @return size_t 总分配内存（字节）
 */
size_t F_get_total_allocated_memory(Stru_LeakDetectorContext_t* detector);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_LEAK_DETECTION_H
