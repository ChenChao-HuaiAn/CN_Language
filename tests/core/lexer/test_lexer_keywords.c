/**
 * @file test_lexer_keywords.c
 * @brief 词法分析器关键字模块测试
 * 
 * 测试CN_lexer_keywords模块的功能，包括：
 * 1. 中文关键字识别
 * 2. 关键字分类识别
 * 3. 关键字查找和匹配
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "src/core/lexer/keywords/CN_lexer_keywords.h"
#include "src/core/token/CN_token_types.h"

/* 测试用例定义 */
typedef struct {
    const char* test_name;
    bool (*test_func)(void);
} TestCase;

/* 测试辅助函数声明 */
static bool test_keyword_identification(void);
static bool test_keyword_categories(void);
static bool test_keyword_edge_cases(void);
static bool test_non_keyword_identification(void);

/* 测试用例数组 */
static TestCase test_cases[] = {
    {"关键字识别", test_keyword_identification},
    {"关键字分类", test_keyword_categories},
    {"边界情况", test_keyword_edge_cases},
    {"非关键字识别", test_non_keyword_identification},
};

static int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief 运行所有关键字测试
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_lexer_keywords_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器关键字模块测试\n");
    printf("========================================\n");
    printf("\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    for (int i = 0; i < total_tests; i++) {
        printf("测试用例 %d/%d: %s... ", i + 1, total_tests, test_cases[i].test_name);
        
        bool result = test_cases[i].test_func();
        
        if (result) {
            printf("✅ 通过\n");
            tests_passed++;
        } else {
            printf("❌ 失败\n");
            tests_failed++;
        }
    }
    
    printf("\n");
    printf("测试结果:\n");
    printf("  总测试数: %d\n", total_tests);
    printf("  通过: %d\n", tests_passed);
    printf("  失败: %d\n", tests_failed);
    printf("  通过率: %.1f%%\n", (float)tests_passed / total_tests * 100);
    printf("\n");
    
    return (tests_failed == 0);
}

/**
 * @brief 测试关键字识别
 */
static bool test_keyword_identification(void)
{
    // 测试一些常见关键字
    struct {
        const char* keyword;
        Eum_TokenType expected_type;
    } test_cases[] = {
        {"变量", Eum_TOKEN_KEYWORD_VAR},
        {"整数", Eum_TOKEN_KEYWORD_INT},
        {"小数", Eum_TOKEN_KEYWORD_FLOAT},
        {"字符串", Eum_TOKEN_KEYWORD_STRING},
        {"布尔", Eum_TOKEN_KEYWORD_BOOL},
        {"数组", Eum_TOKEN_KEYWORD_ARRAY},
        {"结构体", Eum_TOKEN_KEYWORD_STRUCT},
        {"枚举", Eum_TOKEN_KEYWORD_ENUM},
        {"指针", Eum_TOKEN_KEYWORD_POINTER},
        {"引用", Eum_TOKEN_KEYWORD_REFERENCE},
        {"如果", Eum_TOKEN_KEYWORD_IF},
        {"否则", Eum_TOKEN_KEYWORD_ELSE},
        {"当", Eum_TOKEN_KEYWORD_WHILE},
        {"循环", Eum_TOKEN_KEYWORD_FOR},
        {"中断", Eum_TOKEN_KEYWORD_BREAK},
        {"继续", Eum_TOKEN_KEYWORD_CONTINUE},
        {"选择", Eum_TOKEN_KEYWORD_SWITCH},
        {"情况", Eum_TOKEN_KEYWORD_CASE},
        {"默认", Eum_TOKEN_KEYWORD_DEFAULT},
        {"函数", Eum_TOKEN_KEYWORD_FUNCTION},
        {"返回", Eum_TOKEN_KEYWORD_RETURN},
        {"主程序", Eum_TOKEN_KEYWORD_MAIN},
        {"参数", Eum_TOKEN_KEYWORD_PARAM},
        {"递归", Eum_TOKEN_KEYWORD_RECURSIVE},
        {"与", Eum_TOKEN_KEYWORD_AND},
        {"且", Eum_TOKEN_KEYWORD_AND2},
        {"或", Eum_TOKEN_KEYWORD_OR},
        {"非", Eum_TOKEN_KEYWORD_NOT},
        {"真", Eum_TOKEN_KEYWORD_TRUE},
        {"假", Eum_TOKEN_KEYWORD_FALSE},
        {"空", Eum_TOKEN_KEYWORD_NULL},
        {"无", Eum_TOKEN_KEYWORD_VOID},
        {"模块", Eum_TOKEN_KEYWORD_MODULE},
        {"导入", Eum_TOKEN_KEYWORD_IMPORT},
        {"导出", Eum_TOKEN_KEYWORD_EXPORT},
        {"包", Eum_TOKEN_KEYWORD_PACKAGE},
        {"命名空间", Eum_TOKEN_KEYWORD_NAMESPACE},
        {"加", Eum_TOKEN_KEYWORD_ADD},
        {"减", Eum_TOKEN_KEYWORD_SUBTRACT},
        {"乘", Eum_TOKEN_KEYWORD_MULTIPLY},
        {"除", Eum_TOKEN_KEYWORD_DIVIDE},
        {"取模", Eum_TOKEN_KEYWORD_MODULO},
        {"等于", Eum_TOKEN_KEYWORD_EQUAL},
        {"不等于", Eum_TOKEN_KEYWORD_NOT_EQUAL},
        {"小于", Eum_TOKEN_KEYWORD_LESS},
        {"大于", Eum_TOKEN_KEYWORD_GREATER},
        {"小于等于", Eum_TOKEN_KEYWORD_LESS_EQUAL},
        {"大于等于", Eum_TOKEN_KEYWORD_GREATER_EQUAL},
        {"类型", Eum_TOKEN_KEYWORD_TYPE},
        {"接口", Eum_TOKEN_KEYWORD_INTERFACE},
        {"类", Eum_TOKEN_KEYWORD_CLASS},
        {"对象", Eum_TOKEN_KEYWORD_OBJECT},
        {"泛型", Eum_TOKEN_KEYWORD_GENERIC},
        {"模板", Eum_TOKEN_KEYWORD_TEMPLATE},
        {"常量", Eum_TOKEN_KEYWORD_CONST},
        {"静态", Eum_TOKEN_KEYWORD_STATIC},
        {"公开", Eum_TOKEN_KEYWORD_PUBLIC},
        {"私有", Eum_TOKEN_KEYWORD_PRIVATE},
        {"保护", Eum_TOKEN_KEYWORD_PROTECTED},
        {"虚拟", Eum_TOKEN_KEYWORD_VIRTUAL},
        {"重写", Eum_TOKEN_KEYWORD_OVERRIDE},
        {"抽象", Eum_TOKEN_KEYWORD_ABSTRACT},
        {"最终", Eum_TOKEN_KEYWORD_FINAL},
        {"同步", Eum_TOKEN_KEYWORD_SYNC},
        {"异步", Eum_TOKEN_KEYWORD_ASYNC},
        {"等待", Eum_TOKEN_KEYWORD_AWAIT},
        {"抛出", Eum_TOKEN_KEYWORD_THROW},
        {"捕获", Eum_TOKEN_KEYWORD_CATCH},
        {"尝试", Eum_TOKEN_KEYWORD_TRY},
        {"最终块", Eum_TOKEN_KEYWORD_FINALLY},
    };
    
    int test_count = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < test_count; i++) {
        Eum_TokenType type = F_identify_keyword(test_cases[i].keyword);
        
        if (type != test_cases[i].expected_type) {
            printf("错误: 关键字识别失败 - '%s' (期望: %d, 实际: %d)\n",
                   test_cases[i].keyword, test_cases[i].expected_type, type);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 测试关键字分类
 */
static bool test_keyword_categories(void)
{
    // 测试变量和类型声明关键字
    if (F_identify_var_type_keyword("变量") != Eum_TOKEN_KEYWORD_VAR) {
        printf("错误: 变量和类型声明关键字识别失败\n");
        return false;
    }
    
    // 测试控制结构关键字
    if (F_identify_control_keyword("如果") != Eum_TOKEN_KEYWORD_IF) {
        printf("错误: 控制结构关键字识别失败\n");
        return false;
    }
    
    // 测试函数相关关键字
    if (F_identify_function_keyword("函数") != Eum_TOKEN_KEYWORD_FUNCTION) {
        printf("错误: 函数相关关键字识别失败\n");
        return false;
    }
    
    // 测试逻辑运算符关键字
    if (F_identify_logic_keyword("与") != Eum_TOKEN_KEYWORD_AND) {
        printf("错误: 逻辑运算符关键字识别失败\n");
        return false;
    }
    
    // 测试字面量关键字
    if (F_identify_literal_keyword("真") != Eum_TOKEN_KEYWORD_TRUE) {
        printf("错误: 字面量关键字识别失败\n");
        return false;
    }
    
    // 测试模块系统关键字
    if (F_identify_module_keyword("模块") != Eum_TOKEN_KEYWORD_MODULE) {
        printf("错误: 模块系统关键字识别失败\n");
        return false;
    }
    
    // 测试运算符关键字
    if (F_identify_operator_keyword("加") != Eum_TOKEN_KEYWORD_ADD) {
        printf("错误: 运算符关键字识别失败\n");
        return false;
    }
    
    // 测试类型关键字
    if (F_identify_type_keyword("类型") != Eum_TOKEN_KEYWORD_TYPE) {
        printf("错误: 类型关键字识别失败\n");
        return false;
    }
    
    // 测试其他关键字
    if (F_identify_other_keyword("常量") != Eum_TOKEN_KEYWORD_CONST) {
        printf("错误: 其他关键字识别失败\n");
        return false;
    }
    
    return true;
}

/**
 * @brief 测试边界情况
 */
static bool test_keyword_edge_cases(void)
{
    // 测试空字符串
    if (F_identify_keyword("") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 空字符串应返回标识符类型\n");
        return false;
    }
    
    // 测试NULL指针
    if (F_identify_keyword(NULL) != Eum_TOKEN_IDENTIFIER) {
        printf("错误: NULL指针应返回标识符类型\n");
        return false;
    }
    
    // 测试大小写敏感性（中文关键字应大小写不敏感，但英文不应匹配）
    if (F_identify_keyword("VARIABLE") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 英文大写不应匹配中文关键字\n");
        return false;
    }
    
    if (F_identify_keyword("Variable") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 英文混合不应匹配中文关键字\n");
        return false;
    }
    
    // 测试部分匹配（不应识别为关键字）
    if (F_identify_keyword("变") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 部分匹配'变'应识别为标识符\n");
        return false;
    }
    
    if (F_identify_keyword("函") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 部分匹配'函'应识别为标识符\n");
        return false;
    }
    
    if (F_identify_keyword("如") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 部分匹配'如'应识别为标识符\n");
        return false;
    }
    
    // 测试包含关键字但不是关键字
    if (F_identify_keyword("循环测试") != Eum_TOKEN_IDENTIFIER) {
        printf("错误: '循环测试'应识别为标识符\n");
        return false;
    }
    
    // 测试超长字符串
    char long_string[256];
    memset(long_string, 'A', 255);
    long_string[255] = '\0';
    
    if (F_identify_keyword(long_string) != Eum_TOKEN_IDENTIFIER) {
        printf("错误: 超长字符串应返回标识符类型\n");
        return false;
    }
    
    return true;
}

/**
 * @brief 测试非关键字识别
 */
static bool test_non_keyword_identification(void)
{
    // 测试一些非关键字（应为标识符）
    const char* non_keywords[] = {
        "我的变量",
        "计算函数",
        "如果条件",
        "循环计数",
        "返回值",
        "临时数据",
        "用户输入",
        "计算结果",
        "配置文件",
        "日志记录"
    };
    
    for (int i = 0; i < sizeof(non_keywords) / sizeof(non_keywords[0]); i++) {
        Eum_TokenType type = F_identify_keyword(non_keywords[i]);
        
        if (type != Eum_TOKEN_IDENTIFIER) {
            printf("错误: 非关键字识别错误 - '%s' (期望: 标识符, 实际: %d)\n",
                   non_keywords[i], type);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef TEST_KEYWORDS_STANDALONE
int main(void)
{
    bool result = test_lexer_keywords_all();
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
