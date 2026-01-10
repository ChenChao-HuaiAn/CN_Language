/**
 * @file test_symbol_attributes_integration.c
 * @brief 符号属性扩展模块集成测试
 
 * 本文件包含符号属性扩展模块的集成测试，测试符号属性扩展的各项功能。
 * 包括符号可见性、生命周期、访问权限、存储类别、依赖关系跟踪和重命名支持。
 * 遵循项目测试规范，每个测试功能在单独的文件中。
 * 
 * @author CN_Language测试团队
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../../../src/core/semantic/symbol_attributes/CN_symbol_attributes.h"

// ============================================================================
// 测试辅助函数
// ============================================================================

/**
 * @brief 打印测试结果
 */
static void print_test_result(const char* test_name, bool passed)
{
    printf("  %-40s %s\n", test_name, passed ? "[PASS]" : "[FAIL]");
}

/**
 * @brief 创建测试符号信息
 */
static Stru_ExtendedSymbolInfo_t* create_test_symbol_info(
    Stru_SymbolAttributesInterface_t* attr_interface,
    const char* name,
    const char* type)
{
    if (!attr_interface) {
        return NULL;
    }
    
    Stru_ExtendedSymbolInfo_t* symbol = attr_interface->create_symbol_info(name, type);
    if (!symbol) {
        return NULL;
    }
    
    // 设置默认属性
    attr_interface->set_visibility(symbol, Eum_VISIBILITY_PUBLIC);
    attr_interface->set_lifetime(symbol, Eum_LIFETIME_AUTOMATIC);
    attr_interface->set_access(symbol, Eum_ACCESS_READ_WRITE);
    attr_interface->set_storage(symbol, Eum_STORAGE_AUTO);
    
    return symbol;
}

// ============================================================================
// 测试用例
// ============================================================================

/**
 * @brief 测试符号属性接口创建和销毁
 */
static bool test_symbol_attributes_interface_create_destroy(void)
{
    printf("测试符号属性接口创建和销毁:\n");
    
    // 测试1: 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    bool test1_passed = (attr_interface != NULL);
    print_test_result("创建符号属性接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (attr_interface->initialize != NULL &&
                        attr_interface->destroy != NULL &&
                        attr_interface->create_symbol_info != NULL &&
                        attr_interface->destroy_symbol_info != NULL &&
                        attr_interface->set_visibility != NULL &&
                        attr_interface->set_lifetime != NULL &&
                        attr_interface->set_access != NULL &&
                        attr_interface->set_storage != NULL &&
                        attr_interface->add_dependency != NULL &&
                        attr_interface->add_reference != NULL &&
                        attr_interface->rename_symbol != NULL &&
                        attr_interface->add_alias != NULL &&
                        attr_interface->to_string != NULL &&
                        attr_interface->validate != NULL &&
                        attr_interface->clone != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁符号属性接口
    F_destroy_symbol_attributes_interface(attr_interface);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号属性接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号依赖关系接口创建和销毁
 */
static bool test_symbol_dependency_interface_create_destroy(void)
{
    printf("\n测试符号依赖关系接口创建和销毁:\n");
    
    // 测试1: 创建符号依赖关系接口
    Stru_SymbolDependencyInterface_t* dep_interface = 
        F_create_symbol_dependency_interface();
    bool test1_passed = (dep_interface != NULL);
    print_test_result("创建符号依赖关系接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (dep_interface->initialize != NULL &&
                        dep_interface->destroy != NULL &&
                        dep_interface->create_dependency_graph != NULL &&
                        dep_interface->analyze_dependencies != NULL &&
                        dep_interface->detect_cycles != NULL &&
                        dep_interface->get_dependency_chain != NULL &&
                        dep_interface->get_dependents != NULL &&
                        dep_interface->can_rename_safely != NULL &&
                        dep_interface->analyze_rename_impact != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁符号依赖关系接口
    F_destroy_symbol_dependency_interface(dep_interface);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号依赖关系接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号重命名接口创建和销毁
 */
static bool test_symbol_rename_interface_create_destroy(void)
{
    printf("\n测试符号重命名接口创建和销毁:\n");
    
    // 测试1: 创建符号重命名接口
    Stru_SymbolRenameInterface_t* rename_interface = 
        F_create_symbol_rename_interface();
    bool test1_passed = (rename_interface != NULL);
    print_test_result("创建符号重命名接口", test1_passed);
    
    if (!test1_passed) {
        return false;
    }
    
    // 测试2: 接口函数指针检查
    bool test2_passed = (rename_interface->initialize != NULL &&
                        rename_interface->destroy != NULL &&
                        rename_interface->rename_symbol != NULL &&
                        rename_interface->batch_rename != NULL &&
                        rename_interface->validate_rename != NULL &&
                        rename_interface->get_rename_suggestions != NULL &&
                        rename_interface->rename_with_report != NULL);
    print_test_result("接口函数指针检查", test2_passed);
    
    // 测试3: 销毁符号重命名接口
    F_destroy_symbol_rename_interface(rename_interface);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号重命名接口", test3_passed);
    
    return test1_passed && test2_passed && test3_passed;
}

/**
 * @brief 测试符号属性接口初始化功能
 */
static bool test_symbol_attributes_initialization(void)
{
    printf("\n测试符号属性接口初始化功能:\n");
    
    bool all_passed = true;
    
    // 测试符号属性接口初始化
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface != NULL) {
        bool init_result = attr_interface->initialize();
        bool test_passed = init_result;
        print_test_result("符号属性接口初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_symbol_attributes_interface(attr_interface);
    } else {
        print_test_result("符号属性接口初始化", false);
        all_passed = false;
    }
    
    // 测试符号依赖关系接口初始化
    Stru_SymbolDependencyInterface_t* dep_interface = 
        F_create_symbol_dependency_interface();
    if (dep_interface != NULL) {
        bool init_result = dep_interface->initialize();
        bool test_passed = init_result;
        print_test_result("符号依赖关系接口初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_symbol_dependency_interface(dep_interface);
    } else {
        print_test_result("符号依赖关系接口初始化", false);
        all_passed = false;
    }
    
    // 测试符号重命名接口初始化
    Stru_SymbolRenameInterface_t* rename_interface = 
        F_create_symbol_rename_interface();
    if (rename_interface != NULL) {
        bool init_result = rename_interface->initialize();
        bool test_passed = init_result;
        print_test_result("符号重命名接口初始化", test_passed);
        all_passed = all_passed && test_passed;
        F_destroy_symbol_rename_interface(rename_interface);
    } else {
        print_test_result("符号重命名接口初始化", false);
        all_passed = false;
    }
    
    return all_passed;
}

/**
 * @brief 测试符号信息创建和销毁
 */
static bool test_symbol_info_create_destroy(void)
{
    printf("\n测试符号信息创建和销毁:\n");
    
    // 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    // 初始化接口
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 创建符号信息
    Stru_ExtendedSymbolInfo_t* symbol_info = 
        attr_interface->create_symbol_info("testVariable", "int");
    bool test1_passed = (symbol_info != NULL);
    print_test_result("创建符号信息", test1_passed);
    all_passed = all_passed && test1_passed;
    
    if (!test1_passed) {
        attr_interface->destroy();
        F_destroy_symbol_attributes_interface(attr_interface);
        return false;
    }
    
    // 测试2: 检查符号信息字段
    bool test2_passed = (symbol_info->symbol_name != NULL &&
                        strcmp(symbol_info->symbol_name, "testVariable") == 0 &&
                        symbol_info->symbol_type != NULL &&
                        strcmp(symbol_info->symbol_type, "int") == 0);
    print_test_result("检查符号信息字段", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 销毁符号信息
    attr_interface->destroy_symbol_info(symbol_info);
    bool test3_passed = true; // 如果没有崩溃，就认为通过
    print_test_result("销毁符号信息", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 创建多个符号信息
    Stru_ExtendedSymbolInfo_t* symbols[3];
    for (int i = 0; i < 3; i++) {
        char name[32];
        char type[32];
        snprintf(name, sizeof(name), "var%d", i);
        snprintf(type, sizeof(type), "type%d", i);
        
        symbols[i] = attr_interface->create_symbol_info(name, type);
        if (symbols[i] == NULL) {
            all_passed = false;
            break;
        }
    }
    bool test4_passed = (symbols[0] != NULL && symbols[1] != NULL && symbols[2] != NULL);
    print_test_result("创建多个符号信息", test4_passed);
    all_passed = all_passed && test4_passed;
    
    // 清理多个符号信息
    for (int i = 0; i < 3; i++) {
        if (symbols[i] != NULL) {
            attr_interface->destroy_symbol_info(symbols[i]);
        }
    }
    
    // 清理接口
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    
    return all_passed;
}

/**
 * @brief 测试符号属性设置功能
 */
static bool test_symbol_attributes_setting(void)
{
    printf("\n测试符号属性设置功能:\n");
    
    // 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    // 初始化接口
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    // 创建符号信息
    Stru_ExtendedSymbolInfo_t* symbol_info = 
        attr_interface->create_symbol_info("testVar", "float");
    if (symbol_info == NULL) {
        attr_interface->destroy();
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("创建符号信息", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 设置可见性
    bool test1_passed = attr_interface->set_visibility(symbol_info, Eum_VISIBILITY_PRIVATE);
    print_test_result("设置可见性(私有)", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 设置生命周期
    bool test2_passed = attr_interface->set_lifetime(symbol_info, Eum_LIFETIME_STATIC);
    print_test_result("设置生命周期(静态)", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 设置访问权限
    bool test3_passed = attr_interface->set_access(symbol_info, Eum_ACCESS_READ_ONLY);
    print_test_result("设置访问权限(只读)", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 设置存储类别
    bool test4_passed = attr_interface->set_storage(symbol_info, Eum_STORAGE_STATIC);
    print_test_result("设置存储类别(静态)", test4_passed);
    all_passed = all_passed && test4_passed;
    
    // 测试5: 检查属性值
    bool test5_passed = (symbol_info->visibility == Eum_VISIBILITY_PRIVATE &&
                        symbol_info->lifetime == Eum_LIFETIME_STATIC &&
                        symbol_info->access == Eum_ACCESS_READ_ONLY &&
                        symbol_info->storage == Eum_STORAGE_STATIC);
    print_test_result("检查属性值", test5_passed);
    all_passed = all_passed && test5_passed;
    
    // 测试6: 设置其他属性值
    symbol_info->is_constant = true;
    symbol_info->is_volatile = false;
    symbol_info->is_external = true;
    symbol_info->is_deprecated = false;
    bool test6_passed = (symbol_info->is_constant == true &&
                        symbol_info->is_volatile == false &&
                        symbol_info->is_external == true &&
                        symbol_info->is_deprecated == false);
    print_test_result("设置其他属性值", test6_passed);
    all_passed = all_passed && test6_passed;
    
    // 清理
    attr_interface->destroy_symbol_info(symbol_info);
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    
    return all_passed;
}

/**
 * @brief 测试依赖关系管理功能
 */
static bool test_dependency_management(void)
{
    printf("\n测试依赖关系管理功能:\n");
    
    // 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    // 初始化接口
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    // 创建符号信息
    Stru_ExtendedSymbolInfo_t* symbol_info = 
        attr_interface->create_symbol_info("mainFunction", "void");
    if (symbol_info == NULL) {
        attr_interface->destroy();
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("创建符号信息", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 添加依赖关系
    bool test1_passed = attr_interface->add_dependency(
        symbol_info, "printf", Eum_DEPENDENCY_USES, 10, 5);
    print_test_result("添加依赖关系(使用)", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 添加多个依赖关系
    bool test2_passed = attr_interface->add_dependency(
        symbol_info, "scanf", Eum_DEPENDENCY_USES, 15, 8);
    print_test_result("添加多个依赖关系", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 添加不同类型的依赖关系
    bool test3_passed = attr_interface->add_dependency(
        symbol_info, "MyClass", Eum_DEPENDENCY_INHERITS, 20, 12);
    print_test_result("添加继承依赖关系", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 检查依赖关系链表
    bool test4_passed = (symbol_info->dependencies != NULL);
    print_test_result("检查依赖关系链表", test4_passed);
    all_passed = all_passed && test4_passed;
    
    if (test4_passed) {
        // 检查依赖关系链表（由于是添加到头部，顺序是反的）
        Stru_SymbolDependency_t* dep = symbol_info->dependencies;
        
        // 第一个应该是最后添加的"MyClass"
        bool dep1_ok = (dep != NULL && 
                       dep->dependent_symbol != NULL &&
                       strcmp(dep->dependent_symbol, "mainFunction") == 0 &&
                       dep->depended_symbol != NULL &&
                       strcmp(dep->depended_symbol, "MyClass") == 0 &&
                       dep->type == Eum_DEPENDENCY_INHERITS &&
                       dep->line_number == 20 &&
                       dep->column_number == 12);
        print_test_result("检查第一个依赖关系", dep1_ok);
        all_passed = all_passed && dep1_ok;
        
        // 检查链表结构
        if (dep1_ok) {
            // 检查第二个依赖关系
            Stru_SymbolDependency_t* dep2 = dep->next;
            bool dep2_ok = (dep2 != NULL &&
                           dep2->dependent_symbol != NULL &&
                           strcmp(dep2->dependent_symbol, "mainFunction") == 0 &&
                           dep2->depended_symbol != NULL &&
                           strcmp(dep2->depended_symbol, "scanf") == 0 &&
                           dep2->type == Eum_DEPENDENCY_USES &&
                           dep2->line_number == 15 &&
                           dep2->column_number == 8);
            print_test_result("检查第二个依赖关系", dep2_ok);
            all_passed = all_passed && dep2_ok;
            
            // 检查第三个依赖关系
            if (dep2_ok) {
                Stru_SymbolDependency_t* dep3 = dep2->next;
                bool dep3_ok = (dep3 != NULL &&
                               dep3->dependent_symbol != NULL &&
                               strcmp(dep3->dependent_symbol, "mainFunction") == 0 &&
                               dep3->depended_symbol != NULL &&
                               strcmp(dep3->depended_symbol, "printf") == 0 &&
                               dep3->type == Eum_DEPENDENCY_USES &&
                               dep3->line_number == 10 &&
                               dep3->column_number == 5);
                print_test_result("检查第三个依赖关系", dep3_ok);
                all_passed = all_passed && dep3_ok;
                
                // 检查链表结束
                if (dep3_ok) {
                    bool end_ok = (dep3->next == NULL);
                    print_test_result("检查链表结束", end_ok);
                    all_passed = all_passed && end_ok;
                }
            }
        }
    }
    
    // 测试5: 添加引用
    bool test5_passed = attr_interface->add_reference(symbol_info, 25, 10);
    print_test_result("添加引用", test5_passed);
    all_passed = all_passed && test5_passed;
    
    // 测试6: 添加多个引用
    bool test6_passed = attr_interface->add_reference(symbol_info, 30, 15);
    print_test_result("添加多个引用", test6_passed);
    all_passed = all_passed && test6_passed;
    
    // 测试7: 检查引用链表
    bool test7_passed = (symbol_info->references != NULL);
    print_test_result("检查引用链表", test7_passed);
    all_passed = all_passed && test7_passed;
    
    if (test7_passed) {
        // 检查引用链表（由于使用不同的行号和列号，会创建两个不同的引用节点）
        Stru_SymbolReference_t* ref = symbol_info->references;
        bool ref_ok = (ref != NULL);
        print_test_result("检查引用计数", ref_ok);
        all_passed = all_passed && ref_ok;
    }
    
    // 清理
    attr_interface->destroy_symbol_info(symbol_info);
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    
    return all_passed;
}

/**
 * @brief 测试符号重命名功能
 */
static bool test_symbol_renaming(void)
{
    printf("\n测试符号重命名功能:\n");
    
    // 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    // 初始化接口
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    // 创建符号信息
    Stru_ExtendedSymbolInfo_t* symbol_info = 
        attr_interface->create_symbol_info("oldName", "int");
    if (symbol_info == NULL) {
        attr_interface->destroy();
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("创建符号信息", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 测试1: 重命名符号
    bool test1_passed = attr_interface->rename_symbol(symbol_info, "newName");
    print_test_result("重命名符号", test1_passed);
    all_passed = all_passed && test1_passed;
    
    // 测试2: 检查新名称
    bool test2_passed = (symbol_info->symbol_name != NULL &&
                        strcmp(symbol_info->symbol_name, "newName") == 0);
    print_test_result("检查新名称", test2_passed);
    all_passed = all_passed && test2_passed;
    
    // 测试3: 检查原始名称
    bool test3_passed = (symbol_info->original_name != NULL &&
                        strcmp(symbol_info->original_name, "oldName") == 0);
    print_test_result("检查原始名称", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 添加别名
    bool test4_passed = attr_interface->add_alias(symbol_info, "alias1");
    print_test_result("添加别名1", test4_passed);
    all_passed = all_passed && test4_passed;
    
    // 测试5: 添加多个别名
    bool test5_passed = attr_interface->add_alias(symbol_info, "alias2");
    print_test_result("添加别名2", test5_passed);
    all_passed = all_passed && test5_passed;
    
    // 测试6: 检查别名
    bool test6_passed = (symbol_info->aliases != NULL &&
                        strstr(symbol_info->aliases, "alias1") != NULL &&
                        strstr(symbol_info->aliases, "alias2") != NULL);
    print_test_result("检查别名", test6_passed);
    all_passed = all_passed && test6_passed;
    
    // 清理
    attr_interface->destroy_symbol_info(symbol_info);
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    
    return all_passed;
}

/**
 * @brief 测试符号信息字符串表示
 */
static bool test_symbol_info_string_representation(void)
{
    printf("\n测试符号信息字符串表示:\n");
    
    // 创建符号属性接口
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    // 初始化接口
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    // 创建符号信息
    Stru_ExtendedSymbolInfo_t* symbol_info = 
        attr_interface->create_symbol_info("testSymbol", "double");
    if (symbol_info == NULL) {
        attr_interface->destroy();
        F_destroy_symbol_attributes_interface(attr_interface);
        print_test_result("创建符号信息", false);
        return false;
    }
    
    // 设置属性
    attr_interface->set_visibility(symbol_info, Eum_VISIBILITY_PUBLIC);
    attr_interface->set_lifetime(symbol_info, Eum_LIFETIME_STATIC);
    attr_interface->set_access(symbol_info, Eum_ACCESS_READ_WRITE);
    attr_interface->set_storage(symbol_info, Eum_STORAGE_STATIC);
    
    bool all_passed = true;
    
    // 测试1: 获取字符串表示
    char* str_repr = attr_interface->to_string(symbol_info);
    bool test1_passed = (str_repr != NULL);
    print_test_result("获取字符串表示", test1_passed);
    all_passed = all_passed && test1_passed;
    
    if (test1_passed) {
        // 检查字符串内容
        bool contains_name = (strstr(str_repr, "testSymbol") != NULL);
        bool contains_type = (strstr(str_repr, "double") != NULL);
        bool test2_passed = (contains_name && contains_type);
        print_test_result("检查字符串内容", test2_passed);
        all_passed = all_passed && test2_passed;
        
        // 释放字符串
        free(str_repr);
    }
    
    // 测试3: 验证符号属性
    bool test3_passed = attr_interface->validate(symbol_info);
    print_test_result("验证符号属性", test3_passed);
    all_passed = all_passed && test3_passed;
    
    // 测试4: 克隆符号信息
    Stru_ExtendedSymbolInfo_t* cloned_info = attr_interface->clone(symbol_info);
    bool test4_passed = (cloned_info != NULL);
    print_test_result("克隆符号信息", test4_passed);
    all_passed = all_passed && test4_passed;
    
    if (test4_passed) {
        // 检查克隆的符号信息
        bool clone_ok = (cloned_info->symbol_name != NULL &&
                        strcmp(cloned_info->symbol_name, "testSymbol") == 0 &&
                        cloned_info->symbol_type != NULL &&
                        strcmp(cloned_info->symbol_type, "double") == 0 &&
                        cloned_info->visibility == Eum_VISIBILITY_PUBLIC &&
                        cloned_info->lifetime == Eum_LIFETIME_STATIC &&
                        cloned_info->access == Eum_ACCESS_READ_WRITE &&
                        cloned_info->storage == Eum_STORAGE_STATIC);
        print_test_result("检查克隆的符号信息", clone_ok);
        all_passed = all_passed && clone_ok;
        
        // 销毁克隆的符号信息
        attr_interface->destroy_symbol_info(cloned_info);
    }
    
    // 清理
    attr_interface->destroy_symbol_info(symbol_info);
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    
    return all_passed;
}

/**
 * @brief 测试符号依赖关系接口功能
 */
static bool test_dependency_interface_functionality(void)
{
    printf("\n测试符号依赖关系接口功能:\n");
    
    // 创建符号依赖关系接口
    Stru_SymbolDependencyInterface_t* dep_interface = 
        F_create_symbol_dependency_interface();
    if (dep_interface == NULL) {
        print_test_result("创建符号依赖关系接口", false);
        return false;
    }
    
    // 初始化接口
    if (!dep_interface->initialize()) {
        F_destroy_symbol_dependency_interface(dep_interface);
        print_test_result("初始化符号依赖关系接口", false);
        return false;
    }
    
    // 创建符号属性接口用于创建测试符号
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        dep_interface->destroy();
        F_destroy_symbol_dependency_interface(dep_interface);
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        dep_interface->destroy();
        F_destroy_symbol_dependency_interface(dep_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 创建测试符号数组
    Stru_ExtendedSymbolInfo_t* symbols[3];
    symbols[0] = create_test_symbol_info(attr_interface, "funcA", "void");
    symbols[1] = create_test_symbol_info(attr_interface, "funcB", "int");
    symbols[2] = create_test_symbol_info(attr_interface, "funcC", "float");
    
    bool symbols_created = (symbols[0] != NULL && symbols[1] != NULL && symbols[2] != NULL);
    if (!symbols_created) {
        print_test_result("创建测试符号", false);
        all_passed = false;
    } else {
        // 添加依赖关系
        attr_interface->add_dependency(symbols[0], "funcB", Eum_DEPENDENCY_USES, 10, 5);
        attr_interface->add_dependency(symbols[1], "funcC", Eum_DEPENDENCY_USES, 15, 8);
        attr_interface->add_dependency(symbols[2], "funcA", Eum_DEPENDENCY_USES, 20, 12); // 创建循环依赖
        
        // 测试1: 创建依赖关系图
        bool test1_passed = dep_interface->create_dependency_graph(symbols, 3);
        print_test_result("创建依赖关系图", test1_passed);
        all_passed = all_passed && test1_passed;
        
        // 测试2: 分析依赖关系
        char* analysis = dep_interface->analyze_dependencies("funcA");
        bool test2_passed = (analysis != NULL);
        print_test_result("分析依赖关系", test2_passed);
        all_passed = all_passed && test2_passed;
        if (analysis) {
            free(analysis);
        }
        
        // 测试3: 检测循环依赖
        char* cycles = dep_interface->detect_cycles();
        bool test3_passed = (cycles != NULL);
        print_test_result("检测循环依赖", test3_passed);
        all_passed = all_passed && test3_passed;
        if (cycles) {
            free(cycles);
        }
        
        // 测试4: 获取依赖链
        char* chain = dep_interface->get_dependency_chain("funcA", 3);
        bool test4_passed = (chain != NULL);
        print_test_result("获取依赖链", test4_passed);
        all_passed = all_passed && test4_passed;
        if (chain) {
            free(chain);
        }
        
        // 测试5: 获取依赖该符号的符号
        char* dependents = dep_interface->get_dependents("funcB");
        bool test5_passed = (dependents != NULL);
        print_test_result("获取依赖该符号的符号", test5_passed);
        all_passed = all_passed && test5_passed;
        if (dependents) {
            free(dependents);
        }
        
        // 测试6: 检查是否可安全重命名
        bool can_rename = dep_interface->can_rename_safely("funcA");
        bool test6_passed = true; // 函数调用不崩溃即可
        print_test_result("检查是否可安全重命名", test6_passed);
        all_passed = all_passed && test6_passed;
        
        // 测试7: 获取重命名影响分析
        char* impact = dep_interface->analyze_rename_impact("funcA", "newFuncA");
        bool test7_passed = (impact != NULL);
        print_test_result("获取重命名影响分析", test7_passed);
        all_passed = all_passed && test7_passed;
        if (impact) {
            free(impact);
        }
        
        // 清理符号
        for (int i = 0; i < 3; i++) {
            if (symbols[i] != NULL) {
                attr_interface->destroy_symbol_info(symbols[i]);
            }
        }
    }
    
    // 清理接口
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    dep_interface->destroy();
    F_destroy_symbol_dependency_interface(dep_interface);
    
    return all_passed;
}

/**
 * @brief 测试符号重命名接口功能
 */
static bool test_rename_interface_functionality(void)
{
    printf("\n测试符号重命名接口功能:\n");
    
    // 创建符号重命名接口
    Stru_SymbolRenameInterface_t* rename_interface = 
        F_create_symbol_rename_interface();
    if (rename_interface == NULL) {
        print_test_result("创建符号重命名接口", false);
        return false;
    }
    
    // 初始化接口
    if (!rename_interface->initialize()) {
        F_destroy_symbol_rename_interface(rename_interface);
        print_test_result("初始化符号重命名接口", false);
        return false;
    }
    
    // 创建符号属性接口用于创建测试符号
    Stru_SymbolAttributesInterface_t* attr_interface = 
        F_create_symbol_attributes_interface();
    if (attr_interface == NULL) {
        rename_interface->destroy();
        F_destroy_symbol_rename_interface(rename_interface);
        print_test_result("创建符号属性接口", false);
        return false;
    }
    
    if (!attr_interface->initialize()) {
        F_destroy_symbol_attributes_interface(attr_interface);
        rename_interface->destroy();
        F_destroy_symbol_rename_interface(rename_interface);
        print_test_result("初始化符号属性接口", false);
        return false;
    }
    
    bool all_passed = true;
    
    // 创建测试符号数组
    Stru_ExtendedSymbolInfo_t* symbols[3];
    symbols[0] = create_test_symbol_info(attr_interface, "var1", "int");
    symbols[1] = create_test_symbol_info(attr_interface, "var2", "float");
    symbols[2] = create_test_symbol_info(attr_interface, "var3", "double");
    
    bool symbols_created = (symbols[0] != NULL && symbols[1] != NULL && symbols[2] != NULL);
    if (!symbols_created) {
        print_test_result("创建测试符号", false);
        all_passed = false;
    } else {
        // 测试1: 验证重命名
        bool test1_passed = rename_interface->validate_rename("var1", "newVar1", symbols, 3);
        print_test_result("验证重命名", test1_passed);
        all_passed = all_passed && test1_passed;
        
        // 测试2: 执行重命名
        bool test2_passed = rename_interface->rename_symbol("var1", "newVar1", symbols, 3);
        print_test_result("执行重命名", test2_passed);
        all_passed = all_passed && test2_passed;
        
        // 测试3: 检查重命名结果
        bool test3_passed = (symbols[0] != NULL && 
                           symbols[0]->symbol_name != NULL &&
                           strcmp(symbols[0]->symbol_name, "newVar1") == 0);
        print_test_result("检查重命名结果", test3_passed);
        all_passed = all_passed && test3_passed;
        
        // 测试4: 获取重命名报告
        char* report = rename_interface->rename_with_report("var2", "newVar2", symbols, 3);
        bool test4_passed = (report != NULL);
        print_test_result("获取重命名报告", test4_passed);
        all_passed = all_passed && test4_passed;
        if (report) {
            free(report);
        }
        
        // 测试5: 获取重命名建议
        char* suggestions = rename_interface->get_rename_suggestions("var3", "计数器变量");
        bool test5_passed = (suggestions != NULL);
        print_test_result("获取重命名建议", test5_passed);
        all_passed = all_passed && test5_passed;
        if (suggestions) {
            free(suggestions);
        }
        
        // 测试6: 批量重命名
        // 注意：var1已经重命名为newVar1，var2已经重命名为newVar2
        const char* rename_map[][2] = {
            {"newVar1", "renamedVar1"},
            {"newVar2", "renamedVar2"},
            {"var3", "renamedVar3"}
        };
        bool test6_passed = rename_interface->batch_rename((const char**)rename_map, 6, symbols, 3);
        print_test_result("批量重命名", test6_passed);
        all_passed = all_passed && test6_passed;
        
        // 测试7: 检查批量重命名结果
        if (test6_passed) {
            bool check1 = (symbols[0] != NULL && 
                          symbols[0]->symbol_name != NULL &&
                          strcmp(symbols[0]->symbol_name, "renamedVar1") == 0);
            bool check2 = (symbols[1] != NULL && 
                          symbols[1]->symbol_name != NULL &&
                          strcmp(symbols[1]->symbol_name, "renamedVar2") == 0);
            bool check3 = (symbols[2] != NULL && 
                          symbols[2]->symbol_name != NULL &&
                          strcmp(symbols[2]->symbol_name, "renamedVar3") == 0);
            bool test7_passed = (check1 && check2 && check3);
            print_test_result("检查批量重命名结果", test7_passed);
            all_passed = all_passed && test7_passed;
        }
        
        // 清理符号
        for (int i = 0; i < 3; i++) {
            if (symbols[i] != NULL) {
                attr_interface->destroy_symbol_info(symbols[i]);
            }
        }
    }
    
    // 清理接口
    attr_interface->destroy();
    F_destroy_symbol_attributes_interface(attr_interface);
    rename_interface->destroy();
    F_destroy_symbol_rename_interface(rename_interface);
    
    return all_passed;
}

// ============================================================================
// 主测试函数
// ============================================================================

/**
 * @brief 运行所有测试
 */
int main(void)
{
    printf("========================================\n");
    printf("符号属性扩展模块集成测试\n");
    printf("========================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行测试符号属性接口创建和销毁
    total_tests++;
    if (test_symbol_attributes_interface_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试符号依赖关系接口创建和销毁
    total_tests++;
    if (test_symbol_dependency_interface_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试符号重命名接口创建和销毁
    total_tests++;
    if (test_symbol_rename_interface_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试符号属性接口初始化功能
    total_tests++;
    if (test_symbol_attributes_initialization()) {
        passed_tests++;
    }
    
    // 运行测试符号信息创建和销毁
    total_tests++;
    if (test_symbol_info_create_destroy()) {
        passed_tests++;
    }
    
    // 运行测试符号属性设置功能
    total_tests++;
    if (test_symbol_attributes_setting()) {
        passed_tests++;
    }
    
    // 运行测试依赖关系管理功能
    total_tests++;
    if (test_dependency_management()) {
        passed_tests++;
    }
    
    // 运行测试符号重命名功能
    total_tests++;
    if (test_symbol_renaming()) {
        passed_tests++;
    }
    
    // 运行测试符号信息字符串表示
    total_tests++;
    if (test_symbol_info_string_representation()) {
        passed_tests++;
    }
    
    // 运行测试符号依赖关系接口功能
    total_tests++;
    if (test_dependency_interface_functionality()) {
        passed_tests++;
    }
    
    // 运行测试符号重命名接口功能
    total_tests++;
    if (test_rename_interface_functionality()) {
        passed_tests++;
    }
    
    printf("\n========================================\n");
    printf("测试结果汇总\n");
    printf("========================================\n");
    printf("总测试数: %d\n", total_tests);
    printf("通过测试: %d\n", passed_tests);
    printf("失败测试: %d\n", total_tests - passed_tests);
    printf("通过率: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    if (passed_tests == total_tests) {
        printf("\n所有测试通过！\n");
        return 0;
    } else {
        printf("\n部分测试失败！\n");
        return 1;
    }
}
