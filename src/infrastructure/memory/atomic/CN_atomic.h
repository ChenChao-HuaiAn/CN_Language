/******************************************************************************
 * 文件名: CN_atomic.h
 * 功能: CN_Language原子操作和内存屏障原语
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义原子操作和内存屏障接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ATOMIC_H
#define CN_ATOMIC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内存顺序枚举
// ============================================================================

/**
 * @brief 内存顺序枚举
 * 
 * 定义原子操作的内存顺序约束，遵循C11/C++11内存模型。
 */
typedef enum Eum_CN_MemoryOrder_t
{
    Eum_MEMORY_ORDER_RELAXED = 0,     /**< 无顺序约束，仅保证原子性 */
    Eum_MEMORY_ORDER_CONSUME = 1,     /**< 数据依赖顺序（C++11 consume，C11 acquire） */
    Eum_MEMORY_ORDER_ACQUIRE = 2,     /**< 获取顺序，确保后续读/写不会重排到之前 */
    Eum_MEMORY_ORDER_RELEASE = 3,     /**< 释放顺序，确保之前读/写不会重排到之后 */
    Eum_MEMORY_ORDER_ACQ_REL = 4,     /**< 获取-释放顺序，同时具有acquire和release语义 */
    Eum_MEMORY_ORDER_SEQ_CST = 5      /**< 顺序一致性，最强内存顺序 */
} Eum_CN_MemoryOrder_t;

// ============================================================================
// 原子操作接口
// ============================================================================

/**
 * @brief 原子操作接口结构体
 * 
 * 提供统一的原子操作接口，支持多种数据类型和操作。
 */
typedef struct Stru_CN_AtomicInterface_t
{
    // ========================================================================
    // 整数原子操作（32位）
    // ========================================================================
    
    /**
     * @brief 原子加载32位整数
     * 
     * @param ptr 原子变量指针
     * @param order 内存顺序
     * @return 加载的值
     */
    int32_t (*load_i32)(const volatile int32_t* ptr, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子存储32位整数
     * 
     * @param ptr 原子变量指针
     * @param value 要存储的值
     * @param order 内存顺序
     */
    void (*store_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子交换32位整数
     * 
     * @param ptr 原子变量指针
     * @param value 新值
     * @param order 内存顺序
     * @return 旧值
     */
    int32_t (*exchange_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子比较并交换32位整数
     * 
     * @param ptr 原子变量指针
     * @param expected 期望值的指针
     * @param desired 期望的新值
     * @param success_order 成功时的内存顺序
     * @param failure_order 失败时的内存顺序
     * @return 如果交换成功返回true，否则返回false
     */
    bool (*compare_exchange_i32)(volatile int32_t* ptr, int32_t* expected, 
                                 int32_t desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    
    /**
     * @brief 原子加法（32位）
     * 
     * @param ptr 原子变量指针
     * @param value 要加的值
     * @param order 内存顺序
     * @return 加法前的值
     */
    int32_t (*fetch_add_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子减法（32位）
     * 
     * @param ptr 原子变量指针
     * @param value 要减的值
     * @param order 内存顺序
     * @return 减法前的值
     */
    int32_t (*fetch_sub_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子与运算（32位）
     * 
     * @param ptr 原子变量指针
     * @param value 要与的值
     * @param order 内存顺序
     * @return 运算前的值
     */
    int32_t (*fetch_and_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子或运算（32位）
     * 
     * @param ptr 原子变量指针
     * @param value 要或的值
     * @param order 内存顺序
     * @return 运算前的值
     */
    int32_t (*fetch_or_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子异或运算（32位）
     * 
     * @param ptr 原子变量指针
     * @param value 要异或的值
     * @param order 内存顺序
     * @return 运算前的值
     */
    int32_t (*fetch_xor_i32)(volatile int32_t* ptr, int32_t value, Eum_CN_MemoryOrder_t order);
    
    // ========================================================================
    // 整数原子操作（64位）
    // ========================================================================
    
    /**
     * @brief 原子加载64位整数
     * 
     * @param ptr 原子变量指针
     * @param order 内存顺序
     * @return 加载的值
     */
    int64_t (*load_i64)(const volatile int64_t* ptr, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子存储64位整数
     * 
     * @param ptr 原子变量指针
     * @param value 要存储的值
     * @param order 内存顺序
     */
    void (*store_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子交换64位整数
     * 
     * @param ptr 原子变量指针
     * @param value 新值
     * @param order 内存顺序
     * @return 旧值
     */
    int64_t (*exchange_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子比较并交换64位整数
     * 
     * @param ptr 原子变量指针
     * @param expected 期望值的指针
     * @param desired 期望的新值
     * @param success_order 成功时的内存顺序
     * @param failure_order 失败时的内存顺序
     * @return 如果交换成功返回true，否则返回false
     */
    bool (*compare_exchange_i64)(volatile int64_t* ptr, int64_t* expected, 
                                 int64_t desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    
    /**
     * @brief 原子加法（64位）
     * 
     * @param ptr 原子变量指针
     * @param value 要加的值
     * @param order 内存顺序
     * @return 加法前的值
     */
    int64_t (*fetch_add_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子减法（64位）
     * 
     * @param ptr 原子变量指针
     * @param value 要减的值
     * @param order 内存顺序
     * @return 减法前的值
     */
    int64_t (*fetch_sub_i64)(volatile int64_t* ptr, int64_t value, Eum_CN_MemoryOrder_t order);
    
    // ========================================================================
    // 指针原子操作
    // ========================================================================
    
    /**
     * @brief 原子加载指针
     * 
     * @param ptr 原子指针变量指针
     * @param order 内存顺序
     * @return 加载的指针
     */
    void* (*load_ptr)(void* const volatile* ptr, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子存储指针
     * 
     * @param ptr 原子指针变量指针
     * @param value 要存储的指针
     * @param order 内存顺序
     */
    void (*store_ptr)(void* volatile* ptr, void* value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子交换指针
     * 
     * @param ptr 原子指针变量指针
     * @param value 新指针
     * @param order 内存顺序
     * @return 旧指针
     */
    void* (*exchange_ptr)(void* volatile* ptr, void* value, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子比较并交换指针
     * 
     * @param ptr 原子指针变量指针
     * @param expected 期望指针的指针
     * @param desired 期望的新指针
     * @param success_order 成功时的内存顺序
     * @param failure_order 失败时的内存顺序
     * @return 如果交换成功返回true，否则返回false
     */
    bool (*compare_exchange_ptr)(void* volatile* ptr, void** expected, 
                                 void* desired, Eum_CN_MemoryOrder_t success_order,
                                 Eum_CN_MemoryOrder_t failure_order);
    
    /**
     * @brief 原子指针加法（按字节偏移）
     * 
     * @param ptr 原子指针变量指针
     * @param offset 字节偏移量
     * @param order 内存顺序
     * @return 加法前的指针
     */
    void* (*fetch_add_ptr)(void* volatile* ptr, ptrdiff_t offset, Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 原子指针减法（按字节偏移）
     * 
     * @param ptr 原子指针变量指针
     * @param offset 字节偏移量
     * @param order 内存顺序
     * @return 减法前的指针
     */
    void* (*fetch_sub_ptr)(void* volatile* ptr, ptrdiff_t offset, Eum_CN_MemoryOrder_t order);
    
    // ========================================================================
    // 内存屏障操作
    // ========================================================================
    
    /**
     * @brief 线程间内存屏障
     * 
     * 确保屏障前的所有内存操作在屏障后的操作之前完成。
     * 
     * @param order 内存顺序
     */
    void (*thread_fence)(Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 信号量内存屏障
     * 
     * 确保CPU不会重排屏障前后的内存操作。
     */
    void (*signal_fence)(Eum_CN_MemoryOrder_t order);
    
    /**
     * @brief 编译器内存屏障
     * 
     * 仅防止编译器重排，不生成CPU指令。
     */
    void (*compiler_barrier)(void);
    
    /**
     * @brief 全内存屏障（顺序一致性）
     * 
     * 最强的内存屏障，确保所有内存操作按程序顺序执行。
     */
    void (*full_memory_barrier)(void);
    
    /**
     * @brief 加载-加载内存屏障
     * 
     * 确保屏障前的加载操作在屏障后的加载操作之前完成。
     */
    void (*load_load_barrier)(void);
    
    /**
     * @brief 存储-存储内存屏障
     * 
     * 确保屏障前的存储操作在屏障后的存储操作之前完成。
     */
    void (*store_store_barrier)(void);
    
    /**
     * @brief 加载-存储内存屏障
     * 
     * 确保屏障前的加载操作在屏障后的存储操作之前完成。
     */
    void (*load_store_barrier)(void);
    
    /**
     * @brief 存储-加载内存屏障
     * 
     * 确保屏障前的存储操作在屏障后的加载操作之前完成。
     */
    void (*store_load_barrier)(void);
    
} Stru_CN_AtomicInterface_t;

// ============================================================================
// 原子操作管理函数
// ============================================================================

/**
 * @brief 获取默认原子操作接口
 * 
 * 根据当前平台返回相应的原子操作接口实现。
 * 
 * @return 原子操作接口指针，失败返回NULL
 */
const Stru_CN_AtomicInterface_t* CN_atomic_get_default(void);

/**
 * @brief 创建自定义原子操作接口
 * 
 * 允许用户提供自定义的原子操作接口实现。
 * 
 * @param interface 自定义接口实现
 * @return 新创建的原子操作接口，失败返回NULL
 */
Stru_CN_AtomicInterface_t* CN_atomic_create_custom(const Stru_CN_AtomicInterface_t* interface);

/**
 * @brief 销毁原子操作接口
 * 
 * @param interface 要销毁的原子操作接口
 */
void CN_atomic_destroy(Stru_CN_AtomicInterface_t* interface);

// ============================================================================
// 便捷函数（使用默认接口）
// ============================================================================

/**
 * @brief 原子加载32位整数（使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @return 加载的值
 */
int32_t CN_atomic_load_i32(const volatile int32_t* ptr);

/**
 * @brief 原子存储32位整数（使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @param value 要存储的值
 */
void CN_atomic_store_i32(volatile int32_t* ptr, int32_t value);

/**
 * @brief 原子交换32位整数（使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @param value 新值
 * @return 旧值
 */
int32_t CN_atomic_exchange_i32(volatile int32_t* ptr, int32_t value);

/**
 * @brief 原子比较并交换32位整数（使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @param expected 期望值的指针
 * @param desired 期望的新值
 * @return 如果交换成功返回true，否则返回false
 */
bool CN_atomic_compare_exchange_i32(volatile int32_t* ptr, int32_t* expected, int32_t desired);

/**
 * @brief 原子加法（32位，使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @param value 要加的值
 * @return 加法前的值
 */
int32_t CN_atomic_fetch_add_i32(volatile int32_t* ptr, int32_t value);

/**
 * @brief 原子减法（32位，使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @param value 要减的值
 * @return 减法前的值
 */
int32_t CN_atomic_fetch_sub_i32(volatile int32_t* ptr, int32_t value);

/**
 * @brief 原子递增（32位，使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @return 递增前的值
 */
int32_t CN_atomic_fetch_increment_i32(volatile int32_t* ptr);

/**
 * @brief 原子递减（32位，使用顺序一致性）
 * 
 * @param ptr 原子变量指针
 * @return 递减前的值
 */
int32_t CN_atomic_fetch_decrement_i32(volatile int32_t* ptr);

/**
 * @brief 全内存屏障（顺序一致性）
 */
void CN_atomic_full_memory_barrier(void);

/**
 * @brief 编译器内存屏障
 */
void CN_atomic_compiler_barrier(void);

#ifdef __cplusplus
}
#endif

#endif // CN_ATOMIC_H
