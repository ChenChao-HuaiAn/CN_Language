/**
 * @file CN_memory_error_detection.h
 * @brief 内存错误检测模块
 * 
 * 本模块提供了内存错误检测功能，包括：
 * - 缓冲区溢出检测
 * - 双重释放检测
 * - 野指针检测
 * - 未初始化内存检测
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_MEMORY_ERROR_DETECTION_H
#define CN_MEMORY_ERROR_DETECTION_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 内存错误类型
 */
typedef enum Eum_MemoryErrorType_t
{
    Eum_MEMORY_ERROR_NONE = 0,           ///< 无错误
    Eum_MEMORY_ERROR_OVERFLOW,           ///< 缓冲区溢出
    Eum_MEMORY_ERROR_UNDERFLOW,          ///< 缓冲区下溢
    Eum_MEMORY_ERROR_DOUBLE_FREE,        ///< 双重释放
    Eum_MEMORY_ERROR_INVALID_FREE,       ///< 无效释放（非分配地址）
    Eum_MEMORY_ERROR_USE_AFTER_FREE,     ///< 释放后使用
    Eum_MEMORY_ERROR_UNINITIALIZED_READ, ///< 未初始化读取
    Eum_MEMORY_ERROR_CORRUPTION,         ///< 内存损坏
    Eum_MEMORY_ERROR_ALIGNMENT,          ///< 对齐错误
    Eum_MEMORY_ERROR_OUT_OF_BOUNDS       ///< 越界访问
} Eum_MemoryErrorType_t;

/**
 * @brief 内存错误信息
 */
typedef struct Stru_MemoryErrorInfo_t
{
    Eum_MemoryErrorType_t error_type;    ///< 错误类型
    void* address;                       ///< 错误地址
    size_t size;                         ///< 相关大小
    const char* file;                    ///< 源文件名
    size_t line;                         ///< 行号
    const char* function;                ///< 函数名
    const char* description;             ///< 错误描述
} Stru_MemoryErrorInfo_t;

/**
 * @brief 错误检测器上下文
 * 
 * 错误检测器的私有数据。
 */
typedef struct Stru_ErrorDetectorContext_t Stru_ErrorDetectorContext_t;

/**
 * @brief 创建错误检测器
 * 
 * 创建一个新的错误检测器实例。
 * 
 * @param enable_overflow_check 是否启用缓冲区溢出检查
 * @param enable_double_free_check 是否启用双重释放检查
 * @param enable_uninitialized_check 是否启用未初始化内存检查
 * @param guard_zone_size 保护区域大小（用于溢出检测）
 * @return Stru_ErrorDetectorContext_t* 错误检测器上下文，失败返回NULL
 */
Stru_ErrorDetectorContext_t* F_create_error_detector(
    bool enable_overflow_check,
    bool enable_double_free_check,
    bool enable_uninitialized_check,
    size_t guard_zone_size);

/**
 * @brief 销毁错误检测器
 * 
 * 销毁错误检测器实例，释放相关资源。
 * 
 * @param detector 要销毁的错误检测器
 */
void F_destroy_error_detector(Stru_ErrorDetectorContext_t* detector);

/**
 * @brief 保护内存区域
 * 
 * 为内存区域添加保护，用于检测缓冲区溢出。
 * 
 * @param detector 错误检测器
 * @param address 内存地址
 * @param size 内存大小
 * @param file 源文件名（可选）
 * @param line 行号（可选）
 * @param function 函数名（可选）
 * @return bool 保护成功返回true，否则返回false
 */
bool F_protect_memory_region(Stru_ErrorDetectorContext_t* detector,
                            void* address, size_t size,
                            const char* file, size_t line,
                            const char* function);

/**
 * @brief 取消内存区域保护
 * 
 * 移除内存区域的保护。
 * 
 * @param detector 错误检测器
 * @param address 内存地址
 */
void F_unprotect_memory_region(Stru_ErrorDetectorContext_t* detector, void* address);

/**
 * @brief 检查内存区域
 * 
 * 检查内存区域是否有错误。
 * 
 * @param detector 错误检测器
 * @param address 内存地址
 * @param size 内存大小
 * @return bool 检查通过返回true，否则返回false
 */
bool F_check_memory_region(Stru_ErrorDetectorContext_t* detector,
                          void* address, size_t size);

/**
 * @brief 记录内存分配（错误检测模块）
 * 
 * 记录内存分配，用于检测双重释放和无效释放。
 * 
 * @param detector 错误检测器
 * @param address 分配的内存地址
 * @param size 分配的大小
 */
void F_error_detector_record_allocation(Stru_ErrorDetectorContext_t* detector,
                        void* address, size_t size);

/**
 * @brief 记录内存释放（错误检测模块）
 * 
 * 记录内存释放，用于检测双重释放和无效释放。
 * 
 * @param detector 错误检测器
 * @param address 释放的内存地址
 * @return bool 释放有效返回true，否则返回false
 */
bool F_error_detector_record_deallocation(Stru_ErrorDetectorContext_t* detector, void* address);

/**
 * @brief 验证指针有效性
 * 
 * 验证指针是否指向有效的已分配内存。
 * 
 * @param detector 错误检测器
 * @param ptr 要验证的指针
 * @return bool 指针有效返回true，否则返回false
 */
bool F_validate_pointer(Stru_ErrorDetectorContext_t* detector, const void* ptr);

/**
 * @brief 验证内存范围有效性
 * 
 * 验证内存范围是否完全在有效的已分配内存内。
 * 
 * @param detector 错误检测器
 * @param ptr 内存起始指针
 * @param size 内存大小
 * @return bool 内存范围有效返回true，否则返回false
 */
bool F_validate_memory_range(Stru_ErrorDetectorContext_t* detector,
                            const void* ptr, size_t size);

/**
 * @brief 检查所有活动内存分配
 * 
 * 检查所有活动内存分配是否有错误。
 * 
 * @param detector 错误检测器
 * @return size_t 发现的错误数量
 */
size_t F_check_all_allocations(Stru_ErrorDetectorContext_t* detector);

/**
 * @brief 获取错误数量
 * 
 * 获取当前检测到的错误数量。
 * 
 * @param detector 错误检测器
 * @return size_t 错误数量
 */
size_t F_get_error_count(Stru_ErrorDetectorContext_t* detector);

/**
 * @brief 获取错误信息
 * 
 * 获取指定索引的错误信息。
 * 
 * @param detector 错误检测器
 * @param index 错误索引
 * @param error_info 输出参数：错误信息
 * @return bool 获取成功返回true，否则返回false
 */
bool F_get_error_info(Stru_ErrorDetectorContext_t* detector,
                     size_t index, Stru_MemoryErrorInfo_t* error_info);

/**
 * @brief 清除错误记录
 * 
 * 清除所有错误记录。
 * 
 * @param detector 错误检测器
 */
void F_clear_error_records(Stru_ErrorDetectorContext_t* detector);

#ifdef __cplusplus
}
#endif

#endif // CN_MEMORY_ERROR_DETECTION_H
