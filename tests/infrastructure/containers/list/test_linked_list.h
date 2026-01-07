/**
 * @file test_linked_list.h
 * @brief 链表模块测试函数声明
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * 
 * @copyright MIT许可证 - 详见项目根目录LICENSE文件
 * 
 * 链表模块测试头文件，包含所有测试函数的声明。
 * 遵循模块化测试设计原则，每个测试功能在单独文件中实现。
 */

#ifndef TEST_LINKED_LIST_H
#define TEST_LINKED_LIST_H

#include <stdbool.h>

/**
 * @brief 测试链表基础功能
 * 
 * 测试链表的创建、销毁、插入、删除、查询等基础操作。
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_basic_operations(void);

/**
 * @brief 测试链表迭代器功能
 * 
 * 测试链表迭代器的创建、销毁、遍历、插入、删除等操作。
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_iterator_operations(void);

/**
 * @brief 测试链表排序功能
 * 
 * 测试链表的排序功能，包括冒泡排序、插入排序、归并排序等算法。
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_sort_operations(void);

/**
 * @brief 测试链表边界条件和异常处理
 * 
 * 测试链表的边界条件和异常处理，包括空链表、单元素链表、
 * 内存分配失败、无效参数等情况。
 * 
 * @return true 测试通过
 * @return false 测试失败
 */
bool test_linked_list_edge_cases(void);

#endif /* TEST_LINKED_LIST_H */
