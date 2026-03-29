/**
 * @file template_test.c
 * @brief CN语言泛型编程模板功能单元测试
 *
 * 测试范围：
 * - 词法分析：模板关键字识别
 * - 模板实例化：类型映射表、缓存、注册表、名称修饰
 * - 类型替换：类型替换基本功能
 *
 * @version 1.0
 * @date 2026-03-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* 包含被测试的头文件 */
#include "cnlang/frontend/token.h"
#include "cnlang/frontend/keywords.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/semantics/template.h"

/* ============================================================================
 * 测试辅助宏
 * ============================================================================ */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "测试失败: %s (行 %d)\n", message, __LINE__); \
            test_fail_count++; \
        } else { \
            test_pass_count++; \
        } \
    } while(0)

#define TEST_BEGIN(name) \
    fprintf(stdout, "测试: %s\n", name)

/* 全局测试计数器 */
static int test_pass_count = 0;
static int test_fail_count = 0;

/* ============================================================================
 * 词法分析测试
 * ============================================================================ */

/**
 * @brief 测试模板关键字识别
 *
 * 验证"模板"关键字能被正确识别为 CN_TOKEN_KEYWORD_TEMPLATE
 */
static void test_lexer_template_keyword(void) {
    TEST_BEGIN("词法分析 - 模板关键字识别");
    
    /* 测试关键字查找函数 */
    const char *keyword = "模板";
    size_t keyword_len = strlen(keyword);
    
    CnTokenKind kind = cn_frontend_lookup_keyword(keyword, keyword_len);
    TEST_ASSERT(kind == CN_TOKEN_KEYWORD_TEMPLATE, 
                "模板关键字应被识别为 CN_TOKEN_KEYWORD_TEMPLATE");
    
    /* 测试关键字表包含模板关键字 */
    size_t count;
    const CnKeywordEntry *keywords = cn_frontend_get_keywords(&count);
    
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(keywords[i].text, "模板") == 0) {
            TEST_ASSERT(keywords[i].kind == CN_TOKEN_KEYWORD_TEMPLATE,
                        "关键字表中模板关键字的类型应正确");
            found = true;
            break;
        }
    }
    TEST_ASSERT(found, "关键字表中应包含模板关键字");
}

/* ============================================================================
 * 模板实例化测试
 * ============================================================================ */

/**
 * @brief 测试类型映射表操作
 *
 * 验证类型映射表的创建、插入、查找功能
 */
static void test_type_map_operations(void) {
    TEST_BEGIN("模板实例化 - 类型映射表操作");
    
    /* 创建类型映射表 */
    CnTypeMap *map = cn_type_map_new();
    TEST_ASSERT(map != NULL, "类型映射表创建成功");
    TEST_ASSERT(map->entry_count == 0, "初始映射项数量应为0");
    
    /* 创建测试类型 */
    CnType *int_type = cn_type_new_primitive(CN_TYPE_INT);
    TEST_ASSERT(int_type != NULL, "整数类型创建成功");
    
    CnType *float_type = cn_type_new_primitive(CN_TYPE_FLOAT);
    TEST_ASSERT(float_type != NULL, "浮点类型创建成功");
    
    /* 插入映射 */
    bool result = cn_type_map_insert(map, "T", 1, int_type);
    TEST_ASSERT(result, "插入T->整数映射成功");
    TEST_ASSERT(map->entry_count == 1, "映射项数量应为1");
    
    result = cn_type_map_insert(map, "K", 1, float_type);
    TEST_ASSERT(result, "插入K->浮点映射成功");
    TEST_ASSERT(map->entry_count == 2, "映射项数量应为2");
    
    /* 查找映射 */
    CnType *found = cn_type_map_lookup(map, "T", 1);
    TEST_ASSERT(found == int_type, "查找T应返回整数类型");
    
    found = cn_type_map_lookup(map, "K", 1);
    TEST_ASSERT(found == float_type, "查找K应返回浮点类型");
    
    found = cn_type_map_lookup(map, "V", 1);
    TEST_ASSERT(found == NULL, "查找不存在的V应返回NULL");
    
    /* 更新映射 */
    result = cn_type_map_insert(map, "T", 1, float_type);
    TEST_ASSERT(result, "更新T映射成功");
    found = cn_type_map_lookup(map, "T", 1);
    TEST_ASSERT(found == float_type, "更新后T应映射到浮点类型");
    
    /* 清理 */
    cn_type_map_free(map);
    /* 注意：基本类型不需要释放，它们是静态分配的 */
}

/**
 * @brief 测试模板实例化缓存操作
 *
 * 验证缓存的创建、添加、查找功能
 */
static void test_template_cache_operations(void) {
    TEST_BEGIN("模板实例化 - 模板缓存操作");
    
    /* 创建缓存 */
    CnTemplateCache *cache = cn_template_cache_new();
    TEST_ASSERT(cache != NULL, "模板缓存创建成功");
    TEST_ASSERT(cache->instance_count == 0, "初始实例数量应为0");
    
    /* 创建实例化项 */
    CnTemplateInstance *instance = (CnTemplateInstance *)malloc(sizeof(CnTemplateInstance));
    TEST_ASSERT(instance != NULL, "实例化项分配成功");
    
    /* 初始化实例化项 */
    instance->mangled_name = strdup("__cn_template_测试_整数");
    instance->mangled_name_length = strlen(instance->mangled_name);
    instance->template_name = "测试";
    instance->template_name_length = strlen("测试");
    instance->type_args = NULL;
    instance->type_arg_count = 0;
    instance->instantiated_function = NULL;
    instance->instantiated_struct = NULL;
    instance->instantiated_type = NULL;
    
    /* 添加到缓存 */
    bool result = cn_template_cache_add(cache, instance);
    TEST_ASSERT(result, "添加实例化项到缓存成功");
    TEST_ASSERT(cache->instance_count == 1, "缓存实例数量应为1");
    
    /* 清理 */
    cn_template_cache_free(cache);
}

/**
 * @brief 测试模板注册表操作
 *
 * 验证注册表的创建、注册功能
 */
static void test_template_registry_operations(void) {
    TEST_BEGIN("模板实例化 - 模板注册表操作");
    
    /* 创建注册表 */
    CnTemplateRegistry *registry = cn_template_registry_new();
    TEST_ASSERT(registry != NULL, "模板注册表创建成功");
    TEST_ASSERT(registry->entry_count == 0, "初始注册项数量应为0");
    
    /* 清理 */
    cn_template_registry_free(registry);
}

/**
 * @brief 测试名称修饰
 *
 * 验证模板实例化名称修饰规则
 */
static void test_name_mangling(void) {
    TEST_BEGIN("模板实例化 - 名称修饰");
    
    /* 创建测试类型 */
    CnType *int_type = cn_type_new_primitive(CN_TYPE_INT);
    TEST_ASSERT(int_type != NULL, "整数类型创建成功");
    
    CnType *float_type = cn_type_new_primitive(CN_TYPE_FLOAT);
    TEST_ASSERT(float_type != NULL, "浮点类型创建成功");
    
    /* 测试单参数名称修饰 */
    CnType *type_args1[1] = { int_type };
    char *mangled = cn_template_mangle_name("最大值", strlen("最大值"), type_args1, 1);
    TEST_ASSERT(mangled != NULL, "名称修饰成功");
    TEST_ASSERT(strstr(mangled, "__cn_template_") != NULL, 
                "修饰名称应包含前缀");
    TEST_ASSERT(strstr(mangled, "最大值") != NULL,
                "修饰名称应包含原名称");
    free(mangled);
    
    /* 测试多参数名称修饰 */
    CnType *type_args2[2] = { int_type, float_type };
    mangled = cn_template_mangle_name("映射", strlen("映射"), type_args2, 2);
    TEST_ASSERT(mangled != NULL, "多参数名称修饰成功");
    TEST_ASSERT(strstr(mangled, "__cn_template_") != NULL,
                "多参数修饰名称应包含前缀");
    free(mangled);
    
    /* 注意：基本类型不需要释放 */
}

/* ============================================================================
 * 类型替换测试
 * ============================================================================ */

/**
 * @brief 测试类型替换
 *
 * 验证模板参数类型能被正确替换为具体类型
 */
static void test_type_substitution(void) {
    TEST_BEGIN("类型替换 - 基本类型替换");
    
    /* 创建类型映射表 */
    CnTypeMap *map = cn_type_map_new();
    TEST_ASSERT(map != NULL, "类型映射表创建成功");
    
    /* 创建具体类型 */
    CnType *int_type = cn_type_new_primitive(CN_TYPE_INT);
    TEST_ASSERT(int_type != NULL, "整数类型创建成功");
    
    /* 插入映射 */
    cn_type_map_insert(map, "T", 1, int_type);
    
    /* 创建模板参数类型（模拟）- 使用完整的参数列表 */
    CnType *template_param = cn_type_new_struct("T", 1, NULL, 0, NULL, NULL, 0);
    TEST_ASSERT(template_param != NULL, "模板参数类型创建成功");
    
    /* 执行类型替换 */
    CnType *result = cn_type_substitute(template_param, map);
    TEST_ASSERT(result != NULL, "类型替换成功");
    TEST_ASSERT(result == int_type, "模板参数T应被替换为整数类型");
    
    /* 清理 */
    cn_type_map_free(map);
    /* 注意：基本类型不需要释放 */
}

/**
 * @brief 测试指针类型替换
 *
 * 验证指针类型中的模板参数能被正确替换
 */
static void test_pointer_type_substitution(void) {
    TEST_BEGIN("类型替换 - 指针类型替换");
    
    /* 创建类型映射表 */
    CnTypeMap *map = cn_type_map_new();
    
    /* 创建具体类型 */
    CnType *int_type = cn_type_new_primitive(CN_TYPE_INT);
    cn_type_map_insert(map, "T", 1, int_type);
    
    /* 创建模板参数指针类型 T* */
    CnType *template_param = cn_type_new_struct("T", 1, NULL, 0, NULL, NULL, 0);
    CnType *pointer_type = cn_type_new_pointer(template_param);
    TEST_ASSERT(pointer_type != NULL, "指针类型创建成功");
    
    /* 执行类型替换 */
    CnType *result = cn_type_substitute(pointer_type, map);
    TEST_ASSERT(result != NULL, "指针类型替换成功");
    TEST_ASSERT(result->kind == CN_TYPE_POINTER, "结果应仍为指针类型");
    TEST_ASSERT(result->as.pointer_to == int_type, "指针指向的类型应为整数");
    
    /* 清理 */
    cn_type_map_free(map);
    /* 注意：基本类型不需要释放 */
}

/**
 * @brief 测试数组类型替换
 *
 * 验证数组类型中的模板参数能被正确替换
 */
static void test_array_type_substitution(void) {
    TEST_BEGIN("类型替换 - 数组类型替换");
    
    /* 创建类型映射表 */
    CnTypeMap *map = cn_type_map_new();
    
    /* 创建具体类型 */
    CnType *float_type = cn_type_new_primitive(CN_TYPE_FLOAT);
    cn_type_map_insert(map, "T", 1, float_type);
    
    /* 创建模板参数数组类型 T[10] */
    CnType *template_param = cn_type_new_struct("T", 1, NULL, 0, NULL, NULL, 0);
    CnType *array_type = cn_type_new_array(template_param, 10);
    TEST_ASSERT(array_type != NULL, "数组类型创建成功");
    
    /* 执行类型替换 */
    CnType *result = cn_type_substitute(array_type, map);
    TEST_ASSERT(result != NULL, "数组类型替换成功");
    TEST_ASSERT(result->kind == CN_TYPE_ARRAY, "结果应仍为数组类型");
    TEST_ASSERT(result->as.array.element_type == float_type, 
                "数组元素类型应为浮点");
    TEST_ASSERT(result->as.array.length == 10, "数组长度应保持不变");
    
    /* 清理 */
    cn_type_map_free(map);
    /* 注意：基本类型不需要释放 */
}

/* ============================================================================
 * 代码生成测试
 * ============================================================================ */

/**
 * @brief 测试模板函数代码生成
 *
 * 验证模板函数实例化后的代码生成
 */
static void test_template_function_cgen(void) {
    TEST_BEGIN("代码生成 - 模板函数代码生成");
    
    /* 创建实例化项 */
    CnTemplateInstance *instance = (CnTemplateInstance *)malloc(sizeof(CnTemplateInstance));
    TEST_ASSERT(instance != NULL, "实例化项分配成功");
    
    /* 初始化实例化项 */
    instance->mangled_name = strdup("__cn_template_最大值_整数");
    instance->mangled_name_length = strlen(instance->mangled_name);
    instance->template_name = "最大值";
    instance->template_name_length = strlen("最大值");
    instance->type_args = NULL;
    instance->type_arg_count = 0;
    instance->instantiated_function = NULL;
    instance->instantiated_struct = NULL;
    instance->instantiated_type = NULL;
    
    /* 验证名称修饰规则 */
    TEST_ASSERT(strstr(instance->mangled_name, "__cn_template_") != NULL,
                "修饰名称应包含前缀");
    TEST_ASSERT(strstr(instance->mangled_name, "最大值") != NULL,
                "修饰名称应包含原函数名");
    
    /* 清理 */
    free(instance->mangled_name);
    free(instance);
}

/**
 * @brief 测试模板结构体代码生成
 *
 * 验证模板结构体实例化后的代码生成
 */
static void test_template_struct_cgen(void) {
    TEST_BEGIN("代码生成 - 模板结构体代码生成");
    
    /* 创建实例化项 */
    CnTemplateInstance *instance = (CnTemplateInstance *)malloc(sizeof(CnTemplateInstance));
    TEST_ASSERT(instance != NULL, "实例化项分配成功");
    
    /* 初始化实例化项 */
    instance->mangled_name = strdup("__cn_template_数组容器_整数");
    instance->mangled_name_length = strlen(instance->mangled_name);
    instance->template_name = "数组容器";
    instance->template_name_length = strlen("数组容器");
    instance->type_args = NULL;
    instance->type_arg_count = 0;
    instance->instantiated_function = NULL;
    instance->instantiated_struct = NULL;
    instance->instantiated_type = NULL;
    
    /* 验证名称修饰规则 */
    TEST_ASSERT(strstr(instance->mangled_name, "__cn_template_") != NULL,
                "修饰名称应包含前缀");
    TEST_ASSERT(strstr(instance->mangled_name, "数组容器") != NULL,
                "修饰名称应包含原结构体名");
    
    /* 清理 */
    free(instance->mangled_name);
    free(instance);
}

/**
 * @brief 测试名称修饰规则
 *
 * 验证不同类型参数组合的名称修饰
 */
static void test_mangling_rules(void) {
    TEST_BEGIN("代码生成 - 名称修饰规则");
    
    /* 创建测试类型 */
    CnType *int_type = cn_type_new_primitive(CN_TYPE_INT);
    CnType *float_type = cn_type_new_primitive(CN_TYPE_FLOAT);
    CnType *string_type = cn_type_new_primitive(CN_TYPE_STRING);
    
    /* 测试单类型参数 */
    CnType *args1[1] = { int_type };
    char *name1 = cn_template_mangle_name("测试", strlen("测试"), args1, 1);
    TEST_ASSERT(name1 != NULL, "单参数修饰成功");
    TEST_ASSERT(strstr(name1, "整数") != NULL, "应包含类型名");
    free(name1);
    
    /* 测试双类型参数 */
    CnType *args2[2] = { int_type, float_type };
    char *name2 = cn_template_mangle_name("映射", strlen("映射"), args2, 2);
    TEST_ASSERT(name2 != NULL, "双参数修饰成功");
    TEST_ASSERT(strstr(name2, "整数") != NULL, "应包含第一个类型名");
    TEST_ASSERT(strstr(name2, "小数") != NULL, "应包含第二个类型名");
    free(name2);
    
    /* 测试三类型参数 */
    CnType *args3[3] = { int_type, float_type, string_type };
    char *name3 = cn_template_mangle_name("元组", strlen("元组"), args3, 3);
    TEST_ASSERT(name3 != NULL, "三参数修饰成功");
    free(name3);
    
    /* 注意：基本类型不需要释放 */
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("CN语言泛型编程模板功能单元测试\n");
    printf("========================================\n\n");
    
    /* 词法分析测试 */
    printf("--- 词法分析测试 ---\n");
    test_lexer_template_keyword();
    printf("\n");
    
    /* 模板实例化测试 */
    printf("--- 模板实例化测试 ---\n");
    test_type_map_operations();
    test_template_cache_operations();
    test_template_registry_operations();
    test_name_mangling();
    printf("\n");
    
    /* 类型替换测试 */
    printf("--- 类型替换测试 ---\n");
    test_type_substitution();
    test_pointer_type_substitution();
    test_array_type_substitution();
    printf("\n");
    
    /* 代码生成测试 */
    printf("--- 代码生成测试 ---\n");
    test_template_function_cgen();
    test_template_struct_cgen();
    test_mangling_rules();
    printf("\n");
    
    /* 输出测试结果 */
    printf("========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", test_pass_count, test_fail_count);
    printf("========================================\n");
    
    return test_fail_count > 0 ? 1 : 0;
}
