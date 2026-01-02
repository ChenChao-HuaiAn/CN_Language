/******************************************************************************
 * 文件名: CN_stack.h
 * 功能: CN_Language栈容器
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义栈接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_STACK_H
#define CN_STACK_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 栈实现类型枚举
 */
typedef enum Eum_CN_StackImplementation_t
{
    Eum_STACK_IMPLEMENTATION_ARRAY = 0,    /**< 基于数组实现的栈 */
    Eum_STACK_IMPLEMENTATION_LIST = 1,     /**< 基于链表实现的栈 */
    Eum_STACK_IMPLEMENTATION_CIRCULAR = 2  /**< 基于循环数组实现的栈 */
} Eum_CN_StackImplementation_t;

/**
 * @brief 栈线程安全选项枚举
 */
typedef enum Eum_CN_StackThreadSafety_t
{
    Eum_STACK_THREAD_UNSAFE = 0,           /**< 非线程安全栈 */
    Eum_STACK_THREAD_SAFE = 1              /**< 线程安全栈 */
} Eum_CN_StackThreadSafety_t;

/**
 * @brief 栈结构体（不透明类型）
 */
typedef struct Stru_CN_Stack_t Stru_CN_Stack_t;

/**
 * @brief 栈迭代器结构体
 */
typedef struct Stru_CN_StackIterator_t
{
    Stru_CN_Stack_t* stack;               /**< 关联的栈 */
    size_t current_index;                 /**< 当前索引（从栈顶开始） */
    void* current_element;                /**< 当前元素指针 */
} Stru_CN_StackIterator_t;

/**
 * @brief 栈元素释放函数类型
 * 
 * @param element 要释放的元素
 */
typedef void (*CN_StackFreeFunc)(void* element);

/**
 * @brief 栈元素复制函数类型
 * 
 * @param src 源元素
 * @return 新分配的元素副本
 */
typedef void* (*CN_StackCopyFunc)(const void* src);

// ============================================================================
// 栈创建和销毁
// ============================================================================

/**
 * @brief 创建栈
 * 
 * @param implementation 栈实现类型
 * @param element_size 元素大小（字节）
 * @param initial_capacity 初始容量（仅对数组实现有效）
 * @param thread_safety 线程安全选项
 * @return 新创建的栈，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_create(Eum_CN_StackImplementation_t implementation,
                                 size_t element_size, 
                                 size_t initial_capacity,
                                 Eum_CN_StackThreadSafety_t thread_safety);

/**
 * @brief 创建带自定义函数的栈
 * 
 * @param implementation 栈实现类型
 * @param element_size 元素大小（字节）
 * @param initial_capacity 初始容量
 * @param thread_safety 线程安全选项
 * @param free_func 释放函数（可为NULL）
 * @param copy_func 复制函数（可为NULL）
 * @return 新创建的栈，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_create_custom(
    Eum_CN_StackImplementation_t implementation,
    size_t element_size,
    size_t initial_capacity,
    Eum_CN_StackThreadSafety_t thread_safety,
    CN_StackFreeFunc free_func,
    CN_StackCopyFunc copy_func);

/**
 * @brief 销毁栈
 * 
 * @param stack 要销毁的栈
 */
void CN_stack_destroy(Stru_CN_Stack_t* stack);

/**
 * @brief 清空栈（移除所有元素）
 * 
 * @param stack 要清空的栈
 */
void CN_stack_clear(Stru_CN_Stack_t* stack);

// ============================================================================
// 栈属性查询
// ============================================================================

/**
 * @brief 获取栈大小（元素数量）
 * 
 * @param stack 栈
 * @return 栈大小
 */
size_t CN_stack_size(const Stru_CN_Stack_t* stack);

/**
 * @brief 检查栈是否为空
 * 
 * @param stack 栈
 * @return 如果栈为空返回true，否则返回false
 */
bool CN_stack_is_empty(const Stru_CN_Stack_t* stack);

/**
 * @brief 检查栈是否已满（仅对固定容量栈有效）
 * 
 * @param stack 栈
 * @return 如果栈已满返回true，否则返回false
 */
bool CN_stack_is_full(const Stru_CN_Stack_t* stack);

/**
 * @brief 获取栈容量（仅对数组实现有效）
 * 
 * @param stack 栈
 * @return 栈容量
 */
size_t CN_stack_capacity(const Stru_CN_Stack_t* stack);

/**
 * @brief 获取元素大小
 * 
 * @param stack 栈
 * @return 元素大小（字节）
 */
size_t CN_stack_element_size(const Stru_CN_Stack_t* stack);

/**
 * @brief 获取栈实现类型
 * 
 * @param stack 栈
 * @return 栈实现类型
 */
Eum_CN_StackImplementation_t CN_stack_implementation(const Stru_CN_Stack_t* stack);

// ============================================================================
// 栈操作（核心功能）
// ============================================================================

/**
 * @brief 压入元素到栈顶
 * 
 * @param stack 栈
 * @param element 要压入的元素
 * @return 压入成功返回true，失败返回false
 */
bool CN_stack_push(Stru_CN_Stack_t* stack, const void* element);

/**
 * @brief 弹出栈顶元素
 * 
 * @param stack 栈
 * @param element 输出参数，接收弹出的元素值（可为NULL）
 * @return 弹出成功返回true，失败返回false
 */
bool CN_stack_pop(Stru_CN_Stack_t* stack, void* element);

/**
 * @brief 查看栈顶元素（不移除）
 * 
 * @param stack 栈
 * @return 栈顶元素的指针，栈为空返回NULL
 */
void* CN_stack_peek(const Stru_CN_Stack_t* stack);

/**
 * @brief 查看栈顶元素（带索引偏移）
 * 
 * @param stack 栈
 * @param offset 从栈顶向下的偏移量（0表示栈顶）
 * @return 指定偏移处元素的指针，偏移无效返回NULL
 */
void* CN_stack_peek_at(const Stru_CN_Stack_t* stack, size_t offset);

// ============================================================================
// 栈操作（高级功能）
// ============================================================================

/**
 * @brief 确保栈有足够容量（仅对数组实现有效）
 * 
 * @param stack 栈
 * @param min_capacity 最小容量要求
 * @return 调整成功返回true，失败返回false
 */
bool CN_stack_ensure_capacity(Stru_CN_Stack_t* stack, size_t min_capacity);

/**
 * @brief 缩小栈容量以匹配大小（仅对数组实现有效）
 * 
 * @param stack 栈
 * @return 调整成功返回true，失败返回false
 */
bool CN_stack_shrink_to_fit(Stru_CN_Stack_t* stack);

/**
 * @brief 复制栈
 * 
 * @param src 源栈
 * @return 新创建的栈副本，失败返回NULL
 */
Stru_CN_Stack_t* CN_stack_copy(const Stru_CN_Stack_t* src);

/**
 * @brief 反转栈中元素的顺序
 * 
 * @param stack 栈
 * @return 反转成功返回true，失败返回false
 */
bool CN_stack_reverse(Stru_CN_Stack_t* stack);

// ============================================================================
// 线程安全操作（仅对线程安全栈有效）
// ============================================================================

/**
 * @brief 线程安全地压入元素
 * 
 * @param stack 栈
 * @param element 要压入的元素
 * @return 压入成功返回true，失败返回false
 */
bool CN_stack_push_safe(Stru_CN_Stack_t* stack, const void* element);

/**
 * @brief 线程安全地弹出元素
 * 
 * @param stack 栈
 * @param element 输出参数，接收弹出的元素值（可为NULL）
 * @return 弹出成功返回true，失败返回false
 */
bool CN_stack_pop_safe(Stru_CN_Stack_t* stack, void* element);

/**
 * @brief 线程安全地查看栈顶元素
 * 
 * @param stack 栈
 * @return 栈顶元素的指针，栈为空返回NULL
 */
void* CN_stack_peek_safe(const Stru_CN_Stack_t* stack);

/**
 * @brief 获取栈的互斥锁（用于手动同步）
 * 
 * @param stack 栈
 * @return 成功获取锁返回true，失败返回false
 * @note 调用者必须在操作完成后调用CN_stack_unlock
 */
bool CN_stack_lock(Stru_CN_Stack_t* stack);

/**
 * @brief 释放栈的互斥锁
 * 
 * @param stack 栈
 */
void CN_stack_unlock(Stru_CN_Stack_t* stack);

// ============================================================================
// 迭代器支持
// ============================================================================

/**
 * @brief 创建栈迭代器（从栈顶开始）
 * 
 * @param stack 栈
 * @return 新创建的迭代器，失败返回NULL
 */
Stru_CN_StackIterator_t* CN_stack_iterator_create(Stru_CN_Stack_t* stack);

/**
 * @brief 销毁迭代器
 * 
 * @param iterator 要销毁的迭代器
 */
void CN_stack_iterator_destroy(Stru_CN_StackIterator_t* iterator);

/**
 * @brief 重置迭代器到栈顶
 * 
 * @param iterator 迭代器
 */
void CN_stack_iterator_reset(Stru_CN_StackIterator_t* iterator);

/**
 * @brief 检查迭代器是否有下一个元素（向栈底方向）
 * 
 * @param iterator 迭代器
 * @return 如果有下一个元素返回true，否则返回false
 */
bool CN_stack_iterator_has_next(const Stru_CN_StackIterator_t* iterator);

/**
 * @brief 获取下一个元素（向栈底方向）
 * 
 * @param iterator 迭代器
 * @return 下一个元素的指针，没有更多元素返回NULL
 */
void* CN_stack_iterator_next(Stru_CN_StackIterator_t* iterator);

/**
 * @brief 获取当前元素
 * 
 * @param iterator 迭代器
 * @return 当前元素的指针
 */
void* CN_stack_iterator_current(const Stru_CN_StackIterator_t* iterator);

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 比较两个栈
 * 
 * @param stack1 第一个栈
 * @param stack2 第二个栈
 * @return 如果栈相等返回true，否则返回false
 */
bool CN_stack_equal(const Stru_CN_Stack_t* stack1, const Stru_CN_Stack_t* stack2);

/**
 * @brief 转储栈信息到控制台（调试用）
 * 
 * @param stack 栈
 */
void CN_stack_dump(const Stru_CN_Stack_t* stack);

/**
 * @brief 将栈内容转换为数组（从栈顶到底）
 * 
 * @param stack 栈
 * @param array 输出数组（必须足够大）
 * @param max_elements 最大元素数量
 * @return 实际复制的元素数量
 */
size_t CN_stack_to_array(const Stru_CN_Stack_t* stack, void* array, size_t max_elements);

#endif // CN_STACK_H
