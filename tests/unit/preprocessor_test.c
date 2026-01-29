#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 自定义断言宏,在 Release 模式下也能工作 */
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "断言失败: %s\n  条件: %s\n  文件: %s:%d\n", (msg), #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

/* 测试对象宏定义和展开 */
static void test_simple_macro(void)
{
    const char *source = 
        "#define MAX 100\n"
        "变量 x = MAX;\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    
    /* 调试:打印实际输出 */
    printf("test_simple_macro output (length=%zu):\n[%s]\n", 
           preprocessor.output_length, preprocessor.output ? preprocessor.output : "(null)");
    
    /* 检查输出包含展开后的值 */
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    TEST_ASSERT(strstr(preprocessor.output, "100") != NULL, "宏 MAX 应被展开为 100");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_simple_macro: PASSED\n");
}

/* 测试条件编译 #ifdef */
static void test_ifdef(void)
{
    const char *source = 
        "#define DEBUG\n"
        "#ifdef DEBUG\n"
        "变量 x = 1;\n"
        "#else\n"
        "变量 x = 0;\n"
        "#endif\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 应该包含 x = 1 */
    TEST_ASSERT(strstr(preprocessor.output, "变量 x = 1") != NULL, "#ifdef DEBUG 应包含 x = 1");
    /* 不应该包含 x = 0 */
    TEST_ASSERT(strstr(preprocessor.output, "变量 x = 0") == NULL, "#ifdef DEBUG 不应包含 x = 0");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_ifdef: PASSED\n");
}

/* 测试条件编译 #ifndef */
static void test_ifndef(void)
{
    const char *source = 
        "#ifndef RELEASE\n"
        "变量 debug_mode = 1;\n"
        "#else\n"
        "变量 debug_mode = 0;\n"
        "#endif\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* RELEASE 未定义,应该包含 debug_mode = 1 */
    TEST_ASSERT(strstr(preprocessor.output, "debug_mode = 1") != NULL, "#ifndef RELEASE 应包含 debug_mode = 1");
    /* 不应该包含 debug_mode = 0 */
    TEST_ASSERT(strstr(preprocessor.output, "debug_mode = 0") == NULL, "#ifndef RELEASE 不应包含 debug_mode = 0");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_ifndef: PASSED\n");
}

/* 测试嵌套条件编译 */
static void test_nested_ifdef(void)
{
    const char *source = 
        "#define PLATFORM_WINDOWS\n"
        "#ifdef PLATFORM_WINDOWS\n"
        "#define DEBUG\n"
        "#ifdef DEBUG\n"
        "变量 x = 1;\n"
        "#endif\n"
        "#endif\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 两个条件都满足,应该包含 x = 1 */
    TEST_ASSERT(strstr(preprocessor.output, "变量 x = 1") != NULL, "嵌套 #ifdef 应包含 x = 1");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_nested_ifdef: PASSED\n");
}

/* 测试函数宏 */
static void test_function_macro(void)
{
    const char *source = 
        "#define MAX(a, b) ((a) > (b) ? (a) : (b))\n"
        "变量 m = MAX(10, 20);\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 检查展开结果 */
    TEST_ASSERT(strstr(preprocessor.output, "((10) > (20) ? (10) : (20))") != NULL, "函数宏 MAX 应正确展开");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_function_macro: PASSED\n");
}

/* 测试宏参数字符串化 */
static void test_macro_stringification(void)
{
    const char *source = 
        "#define STR(x) #x\n"
        "变量 s = STR(hello);\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 检查字符串化结果 */
    TEST_ASSERT(strstr(preprocessor.output, "\"hello\"") != NULL, "字符串化应生成 \"hello\"");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_macro_stringification: PASSED\n");
}

/* 测试 #undef */
static void test_undef(void)
{
    const char *source = 
        "#define FOO 42\n"
        "变量 a = FOO;\n"
        "#undef FOO\n"
        "#define FOO 100\n"
        "变量 b = FOO;\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 第一次 FOO 应该是 42,第二次应该是 100 */
    const char *output = preprocessor.output;
    const char *first_foo = strstr(output, "变量 a = ");
    const char *second_foo = strstr(output, "变量 b = ");
    
    TEST_ASSERT(first_foo != NULL, "#undef 测试应包含 '变量 a = '");
    TEST_ASSERT(second_foo != NULL, "#undef 测试应包含 '变量 b = '");
    
    /* 验证值 */
    TEST_ASSERT(strstr(first_foo, "42") != NULL, "#undef 前 FOO 应为 42");
    TEST_ASSERT(strstr(second_foo, "100") != NULL, "#undef 后 FOO 应为 100");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_undef: PASSED\n");
}

/* 测试多参数函数宏 */
static void test_multi_param_macro(void)
{
    const char *source = 
        "#define ADD(x, y, z) ((x) + (y) + (z))\n"
        "变量 sum = ADD(1, 2, 3);\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 检查展开结果 */
    TEST_ASSERT(strstr(preprocessor.output, "((1) + (2) + (3))") != NULL, "多参数宏应正确展开");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_multi_param_macro: PASSED\n");
}

/* 测试空宏 */
static void test_empty_macro(void)
{
    const char *source = 
        "#define EMPTY\n"
        "变量 x = EMPTY 42;\n";
    
    CnPreprocessor preprocessor;
    CnDiagnostics diagnostics;
    
    cn_support_diagnostics_init(&diagnostics);
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    TEST_ASSERT(cn_frontend_preprocessor_process(&preprocessor), "预处理失败");
    TEST_ASSERT(preprocessor.output != NULL, "输出缓冲区为空");
    
    /* 空宏应该被替换为空 */
    TEST_ASSERT(strstr(preprocessor.output, "变量 x =  42") != NULL, "空宏应被替换为空");
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("test_empty_macro: PASSED\n");
}

int main(void)
{
    printf("开始预处理器测试...\n\n");
    
    test_simple_macro();
    test_ifdef();
    test_ifndef();
    test_nested_ifdef();
    test_function_macro();
    test_macro_stringification();
    test_undef();
    test_multi_param_macro();
    test_empty_macro();
    
    printf("\n所有预处理器测试通过!\n");
    return 0;
}
