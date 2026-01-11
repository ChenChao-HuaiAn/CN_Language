/******************************************************************************
 * 文件名: test_ir_all.c
 * 功能: CN_Language IR模块综合测试
 * 
 * 运行IR模块的所有测试用例。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language测试团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* 包含各个测试模块的头文件 */
#include "interface/test_ir_interface.h"
#include "implementations/tac/test_tac_ir.h"

/**
 * @brief 运行所有IR测试
 * 
 * 运行IR模块的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_ir_module_all(void)
{
    printf("开始运行IR模块综合测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    
    /* 运行IR接口测试 */
    printf("运行IR接口测试...\n");
    printf("----------------\n");
    if (!test_ir_interface_all()) {
        printf("❌ IR接口测试失败\n");
        all_passed = false;
    } else {
        printf("✅ IR接口测试通过\n");
    }
    printf("\n");
    
    /* 运行TAC IR测试 */
    printf("运行TAC IR测试...\n");
    printf("----------------\n");
    if (!test_tac_ir_all()) {
        printf("❌ TAC IR测试失败\n");
        all_passed = false;
    } else {
        printf("✅ TAC IR测试通过\n");
    }
    printf("\n");
    
    printf("================================\n");
    if (all_passed) {
        printf("✅ 所有IR模块测试通过！\n");
    } else {
        printf("❌ 有IR模块测试失败\n");
    }
    
    return all_passed;
}

/**
 * @brief 主函数
 */
int main(void)
{
    if (test_ir_module_all()) {
        return 0;
    } else {
        return 1;
    }
}
