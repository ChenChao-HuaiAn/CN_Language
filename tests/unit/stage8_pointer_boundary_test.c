/*
 * 阶段8边界测试：指针类型
 * 
 * 测试目标：
 * - 多级指针边界情况
 * - 空指针处理
 * - 指针运算边界
 * - 指针类型转换
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"

/* 测试1: 多级指针（最多3级） */
static void test_multilevel_pointers(void)
{
    printf("  [1] 测试多级指针...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 x = 42;\n"
        "    变量 整数* ptr1 = &x;\n"
        "    变量 整数** ptr2 = &ptr1;\n"
        "    变量 整数*** ptr3 = &ptr2;\n"
        "    打印(\"%d\\n\", ***ptr3);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_multilevel_ptr.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    assert(program->function_count == 1);
    
    printf("    ✓ 多级指针解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试2: 空指针常量 */
static void test_null_pointer(void)
{
    printf("  [2] 测试空指针常量...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数* ptr = 空;\n"
        "    如果 (ptr == 空) {\n"
        "        打印(\"指针为空\\n\");\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_null_ptr.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ 空指针常量解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试3: 指针运算边界 */
static void test_pointer_arithmetic_bounds(void)
{
    printf("  [3] 测试指针运算边界...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 数组[10];\n"
        "    变量 整数* ptr = 数组;\n"
        "    \n"
        "    // 合法：指针指向数组末尾的下一个位置\n"
        "    变量 整数* end = ptr + 10;\n"
        "    \n"
        "    // 指针差值\n"
        "    变量 整数 diff = end - ptr;\n"
        "    打印(\"diff = %d\\n\", diff);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_ptr_arithmetic.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ 指针运算边界解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试4: void指针 */
static void test_void_pointer(void)
{
    printf("  [4] 测试void指针...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 x = 42;\n"
        "    变量 空* vptr = &x;\n"
        "    \n"
        "    // 需要显式转换才能解引用\n"
        "    变量 整数* iptr = 作为<整数*>(vptr);\n"
        "    打印(\"%d\\n\", *iptr);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_void_ptr.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ void指针解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试5: 指针与数组的关系 */
static void test_pointer_array_relationship(void)
{
    printf("  [5] 测试指针与数组关系...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 数组[] = {1, 2, 3, 4, 5};\n"
        "    变量 整数* ptr = 数组;\n"
        "    \n"
        "    // 数组名可以作为指针使用\n"
        "    打印(\"%d\\n\", *数组);\n"
        "    打印(\"%d\\n\", 数组[2]);\n"
        "    打印(\"%d\\n\", *(ptr + 2));\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_ptr_array.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ 指针数组关系解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试6: 函数指针与NULL */
static void test_function_pointer_null(void)
{
    printf("  [6] 测试函数指针与NULL...\n");
    
    const char *source = 
        "类型别名 回调 = 函数(整数) -> 空;\n"
        "\n"
        "函数 处理(回调 f, 整数 x) {\n"
        "    如果 (f != 空) {\n"
        "        f(x);\n"
        "    } 否则 {\n"
        "        打印(\"回调为空\\n\");\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_funcptr_null.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ 函数指针NULL检查解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试7: 指针比较边界 */
static void test_pointer_comparison_bounds(void)
{
    printf("  [7] 测试指针比较边界...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 数组[10];\n"
        "    变量 整数* start = 数组;\n"
        "    变量 整数* end = 数组 + 10;\n"
        "    \n"
        "    // 指针比较\n"
        "    如果 (start < end) {\n"
        "        打印(\"start在end之前\\n\");\n"
        "    }\n"
        "    \n"
        "    // 指针相等比较\n"
        "    如果 (start == 数组) {\n"
        "        打印(\"start指向数组开始\\n\");\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_ptr_comparison.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ 指针比较边界解析成功\n");
    cn_frontend_parser_free(parser);
}

/* 测试8: const指针 */
static void test_const_pointer(void)
{
    printf("  [8] 测试const指针...\n");
    
    const char *source = 
        "函数 测试() {\n"
        "    变量 整数 x = 42;\n"
        "    \n"
        "    // 指向常量的指针\n"
        "    变量 常量 整数* ptr1 = &x;\n"
        "    // *ptr1 = 100;  // 错误：不能修改\n"
        "    \n"
        "    // 常量指针\n"
        "    变量 整数* 常量 ptr2 = &x;\n"
        "    *ptr2 = 100;     // 可以修改值\n"
        "    // ptr2 = &y;    // 错误：不能修改指针\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_const_ptr.cn");
    
    CnParser *parser = cn_frontend_parser_new(&lexer);
    CnAstProgram *program = NULL;
    bool success = cn_frontend_parse_program(parser, &program);
    
    assert(success && program != NULL);
    printf("    ✓ const指针解析成功\n");
    cn_frontend_parser_free(parser);
}

int main(void)
{
    printf("===== 阶段8边界测试：指针类型 =====\n\n");
    
    test_multilevel_pointers();
    test_null_pointer();
    test_pointer_arithmetic_bounds();
    test_void_pointer();
    test_pointer_array_relationship();
    test_function_pointer_null();
    test_pointer_comparison_bounds();
    test_const_pointer();
    
    printf("\n✅ 所有指针类型边界测试通过\n");
    return 0;
}
