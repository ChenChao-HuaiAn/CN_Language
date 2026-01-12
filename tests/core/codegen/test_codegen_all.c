/******************************************************************************
 * 文件名: test_codegen_all.c
 * 功能: CN_Language 代码生成模块总测试运行器
 * 
 * 这是代码生成模块的总测试运行器，调用各个子模块的测试。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月9日
 * 修改历史:
 *   - 2026年1月9日: 初始版本，重构为模块化测试
 * 版权: MIT许可证
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 声明子模块测试函数（不包含.c文件） */
bool test_codegen_interface_all(void);
bool test_codegen_factory_all(void);
bool test_codegen_ast_integration_all(void);
bool test_c_backend_all(void);
bool test_basic_optimizer_all(void);
bool test_bytecode_backend_all(void);
bool test_bytecode_generator_all(void);

/**
 * @brief 运行所有代码生成测试
 * 
 * 运行代码生成模块的所有子模块测试。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_codegen_all(void)
{
    printf("开始运行代码生成模块测试...\n");
    printf("================================\n\n");
    
    bool all_passed = true;
    int modules_passed = 0;
    int modules_total = 0;
    
    /* 运行接口测试 */
    printf("模块: 代码生成器接口测试\n");
    printf("------------------------\n");
    bool interface_passed = test_codegen_interface_all();
    if (interface_passed) {
        printf("✅ 接口测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 接口测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行工厂测试 */
    printf("模块: 代码生成器工厂测试\n");
    printf("------------------------\n");
    bool factory_passed = test_codegen_factory_all();
    if (factory_passed) {
        printf("✅ 工厂测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 工厂测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行集成测试 */
    printf("模块: 代码生成与AST集成测试\n");
    printf("---------------------------\n");
    bool integration_passed = test_codegen_ast_integration_all();
    if (integration_passed) {
        printf("✅ 集成测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 集成测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行C后端测试 */
    printf("模块: C语言后端测试\n");
    printf("-------------------\n");
    bool c_backend_passed = test_c_backend_all();
    if (c_backend_passed) {
        printf("✅ C后端测试通过\n");
        modules_passed++;
    } else {
        printf("❌ C后端测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行基础优化器测试 */
    printf("模块: 基础优化器测试\n");
    printf("--------------------\n");
    bool basic_optimizer_passed = test_basic_optimizer_all();
    if (basic_optimizer_passed) {
        printf("✅ 基础优化器测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 基础优化器测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行字节码后端测试 */
    printf("模块: 字节码后端测试\n");
    printf("--------------------\n");
    bool bytecode_backend_passed = test_bytecode_backend_all();
    if (bytecode_backend_passed) {
        printf("✅ 字节码后端测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 字节码后端测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 运行字节码生成器测试 */
    printf("模块: 字节码生成器测试\n");
    printf("----------------------\n");
    bool bytecode_generator_passed = test_bytecode_generator_all();
    if (bytecode_generator_passed) {
        printf("✅ 字节码生成器测试通过\n");
        modules_passed++;
    } else {
        printf("❌ 字节码生成器测试失败\n");
        all_passed = false;
    }
    modules_total++;
    printf("\n");
    
    /* 打印统计信息 */
    printf("================================\n");
    printf("测试统计:\n");
    printf("  ├─ 总模块数:      %d\n", modules_total);
    printf("  ├─ 模块通过:      %d\n", modules_passed);
    printf("  ├─ 模块失败:      %d\n", modules_total - modules_passed);
    printf("  └─ 通过率:        %.1f%%\n", (float)modules_passed / modules_total * 100);
    printf("\n");
    
    if (all_passed) {
        printf("✅ 所有代码生成测试通过！\n");
    } else {
        printf("❌ 有测试失败\n");
    }
    
    return all_passed;
}

/* 主函数在test_runner_main.c中定义，这里只提供测试函数 */
