/******************************************************************************
 * 文件名: CN_stack_internal.h
 * 功能: CN_Language栈容器内部结构定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义栈内部结构
 * 版权: MIT许可证
 * 
 * 注意: 此文件仅供CN_stack模块内部使用，外部模块不应包含此文件
 ******************************************************************************/

#ifndef CN_STACK_INTERNAL_H
#define CN_STACK_INTERNAL_H

#include "CN_stack.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 栈节点结构体（用于链表实现）
 */
typedef struct Stru_CN_StackNode_t
{
    void* data;                     /**< 节点数据 */
    struct Stru_CN_StackNode_t* next; /**< 下一个节点指针 */
} Stru_CN_StackNode_t;

/**
 * @brief 栈结构体（完整定义，仅供内部使用）
 */
struct Stru_CN_Stack_t
{
    // 通用字段
    Eum_CN_StackImplementation_t implementation; /**< 栈实现类型 */
    size_t element_size;                         /**< 元素大小（字节） */
    size_t size;                                 /**< 当前大小（元素数量） */
    Eum_CN_StackThreadSafety_t thread_safety;    /**< 线程安全选项 */
    CN_StackFreeFunc free_func;                  /**< 释放函数 */
    CN_StackCopyFunc copy_func;                  /**< 复制函数 */
    
    // 线程同步
#ifdef _WIN32
    CRITICAL_SECTION lock;                       /**< Windows临界区 */
#else
    pthread_mutex_t lock;                        /**< POSIX互斥锁 */
#endif
    bool lock_initialized;                       /**< 锁是否已初始化 */
    
    // 实现特定字段
    union
    {
        // 数组实现
        struct
        {
            void* data;                          /**< 数据数组 */
            size_t capacity;                     /**< 数组容量 */
            size_t top_index;                    /**< 栈顶索引 */
        } array;
        
        // 链表实现
        struct
        {
            Stru_CN_StackNode_t* top;            /**< 栈顶节点 */
        } list;
        
        // 循环数组实现
        struct
        {
            void* data;                          /**< 数据数组 */
            size_t capacity;                     /**< 数组容量 */
            size_t top_index;                    /**< 栈顶索引 */
            size_t base_index;                   /**< 栈底索引（用于循环） */
            bool is_wrapped;                     /**< 是否已环绕 */
        } circular;
    } impl;
};

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 初始化栈锁
 * 
 * @param stack 栈
 * @return 初始化成功返回true，失败返回false
 */
bool CN_stack_internal_init_lock(Stru_CN_Stack_t* stack);

/**
 * @brief 销毁栈锁
 * 
 * @param stack 栈
 */
void CN_stack_internal_destroy_lock(Stru_CN_Stack_t* stack);

/**
 * @brief 锁定栈（线程安全）
 * 
 * @param stack 栈
 */
void CN_stack_internal_lock(Stru_CN_Stack_t* stack);

/**
 * @brief 解锁栈（线程安全）
 * 
 * @param stack 栈
 */
void CN_stack_internal_unlock(Stru_CN_Stack_t* stack);

/**
 * @brief 创建数组实现的栈
 * 
 * @param element_size 元素大小
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的栈，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_create_array(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func);

/**
 * @brief 创建链表实现的栈
 * 
 * @param element_size 元素大小
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的栈，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_create_list(
    size_t element_size,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func);

/**
 * @brief 创建循环数组实现的栈
 * 
 * @param element_size 元素大小
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数
 * @param copy_func 复制函数
 * @return 新创建的栈，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_create_circular(
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func);

/**
 * @brief 销毁数组实现的栈
 * 
 * @param stack 栈
 */
void CN_stack_internal_destroy_array(Stru_CN_Stack_t* stack);

/**
 * @brief 销毁链表实现的栈
 * 
 * @param stack 栈
 */
void CN_stack_internal_destroy_list(Stru_CN_Stack_t* stack);

/**
 * @brief 销毁循环数组实现的栈
 * 
 * @param stack 栈
 */
void CN_stack_internal_destroy_circular(Stru_CN_Stack_t* stack);

/**
 * @brief 数组实现的压栈操作
 * 
 * @param stack 栈
 * @param element 要压入的元素
 * @return 压入成功返回true，失败返回false
 */
bool CN_stack_internal_array_push(Stru_CN_Stack_t* stack, const void* element);

/**
 * @brief 链表实现的压栈操作
 * 
 * @param stack 栈
 * @param element 要压入的元素
 * @return 压入成功返回true，失败返回false
 */
bool CN_stack_internal_list_push(Stru_CN_Stack_t* stack, const void* element);

/**
 * @brief 循环数组实现的压栈操作
 * 
 * @param stack 栈
 * @param element 要压入的元素
 * @return 压入成功返回true，失败返回false
 */
bool CN_stack_internal_circular_push(Stru_CN_Stack_t* stack, const void* element);

/**
 * @brief 数组实现的弹栈操作
 * 
 * @param stack 栈
 * @param element 输出参数，接收弹出的元素值
 * @return 弹出成功返回true，失败返回false
 */
bool CN_stack_internal_array_pop(Stru_CN_Stack_t* stack, void* element);

/**
 * @brief 链表实现的弹栈操作
 * 
 * @param stack 栈
 * @param element 输出参数，接收弹出的元素值
 * @return 弹出成功返回true，失败返回false
 */
bool CN_stack_internal_list_pop(Stru_CN_Stack_t* stack, void* element);

/**
 * @brief 循环数组实现的弹栈操作
 * 
 * @param stack 栈
 * @param element 输出参数，接收弹出的元素值
 * @return 弹出成功返回true，失败返回false
 */
bool CN_stack_internal_circular_pop(Stru_CN_Stack_t* stack, void* element);

/**
 * @brief 数组实现的查看栈顶操作
 * 
 * @param stack 栈
 * @return 栈顶元素的指针
 */
void* CN_stack_internal_array_peek(const Stru_CN_Stack_t* stack);

/**
 * @brief 链表实现的查看栈顶操作
 * 
 * @param stack 栈
 * @return 栈顶元素的指针
 */
void* CN_stack_internal_list_peek(const Stru_CN_Stack_t* stack);

/**
 * @brief 循环数组实现的查看栈顶操作
 * 
 * @param stack 栈
 * @return 栈顶元素的指针
 */
void* CN_stack_internal_circular_peek(const Stru_CN_Stack_t* stack);

/**
 * @brief 确保数组栈有足够容量
 * 
 * @param stack 栈
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_stack_internal_array_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity);

/**
 * @brief 确保循环数组栈有足够容量
 * 
 * @param stack 栈
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_stack_internal_circular_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity);

/**
 * @brief 复制数组栈
 * 
 * @param src 源栈
 * @return 新创建的栈副本，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_array_copy(const Stru_CN_Stack_t* src);

/**
 * @brief 复制链表栈
 * 
 * @param src 源栈
 * @return 新创建的栈副本，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_list_copy(const Stru_CN_Stack_t* src);

/**
 * @brief 复制循环数组栈
 * 
 * @param src 源栈
 * @return 新创建的栈副本，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_internal_circular_copy(const Stru_CN_Stack_t* src);

/**
 * @brief 获取栈元素指针（通用）
 * 
 * @param stack 栈
 * @param index 元素索引（0表示栈顶）
 * @return 元素指针，索引无效返回NULL
 */
void* CN_stack_internal_get_element(const Stru_CN_Stack_t* stack, size_t index);

/**
 * @brief 获取数组栈元素指针
 * 
 * @param stack 栈
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_stack_internal_array_get_element(const Stru_CN_Stack_t* stack, size_t index);

/**
 * @brief 获取链表栈元素指针
 * 
 * @param stack 栈
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_stack_internal_list_get_element(const Stru_CN_Stack_t* stack, size_t index);

/**
 * @brief 获取循环数组栈元素指针
 * 
 * @param stack 栈
 * @param index 元素索引
 * @return 元素指针
 */
void* CN_stack_internal_circular_get_element(const Stru_CN_Stack_t* stack, size_t index);

/**
 * @brief 计算循环数组索引
 * 
 * @param stack 栈
 * @param logical_index 逻辑索引
 * @return 物理数组索引
 */
size_t CN_stack_internal_circular_compute_index(const Stru_CN_Stack_t* stack, size_t logical_index);

#endif // CN_STACK_INTERNAL_H
