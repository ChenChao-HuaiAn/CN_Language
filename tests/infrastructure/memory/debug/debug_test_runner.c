/**
 * @file debug_test_runner.c
 * @brief 内存调试模块专用测试运行器
 * 
 * 这是一个专门用于内存调试模块的简单测试运行器。
 * 它直接调用test_memory_debug_all()函数来运行所有debug模块测试。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/* 外部测试函数声明 */
extern bool test_memory_debug_all(void);

/**
 * @brief 主函数
 */
int main(void)
{
    printf("\n");
    printf("========================================\n");
