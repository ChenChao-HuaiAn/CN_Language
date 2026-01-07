/**
 * @file test_queue_runner.c
 * @brief 队列模块测试运行器
 * 
 * 实现队列模块的测试运行器函数，供总测试运行器调用。
 * 不包含main函数，专门用于模块化测试框架。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-07
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "test_queue.h"

/**
 * @brief 运行所有队列测试（总测试运行器接口）
 * @return 所有测试通过返回true，否则返回false
 */
bool test_queue_all(void)
{
    int passed = 0;
    int total = 0;
    
    /* ==================== 核心模块测试 ==================== */
    total++; if (test_queue_core_create_and_destroy()) passed++;
    total++; if (test_queue_core_basic_operations()) passed++;
    total++; if (test_queue_core_query_operations()) passed++;
    total++; if (test_queue_core_management_operations()) passed++;
    total++; if (test_queue_core_error_handling()) passed++;
    
    /* ==================== 迭代器模块测试 ==================== */
    total++; if (test_queue_iterator_create_and_destroy()) passed++;
    total++; if (test_queue_iterator_traversal()) passed++;
    total++; if (test_queue_iterator_state_queries()) passed++;
    total++; if (test_queue_iterator_reset()) passed++;
    
    /* ==================== 工具模块测试 ==================== */
    total++; if (test_queue_utils_copy()) passed++;
    total++; if (test_queue_utils_equals()) passed++;
    total++; if (test_queue_utils_batch_operations()) passed++;
    total++; if (test_queue_utils_conversion()) passed++;
    total++; if (test_queue_utils_search()) passed++;
    
    /* ==================== 综合测试 ==================== */
    total++; if (test_queue_edge_cases()) passed++;
    total++; if (test_queue_performance()) passed++;
    total++; if (test_queue_integration()) passed++;
    
    return (passed == total);
}
