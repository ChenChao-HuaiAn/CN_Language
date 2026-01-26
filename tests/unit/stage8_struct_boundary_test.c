/*
 * 阶段8边界测试：结构体
 * 
 * 测试目标：
 * - 空结构体
 * - 嵌套结构体深度
 * - 大型结构体
 * - 结构体对齐
 * - 柔性数组成员
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"

/* 测试1: 空结构体 */
static void test_empty_struct(void)
{
    printf("  [1] 测试空结构体...\n");
    
    const char *source = 
        "结构体 空 {\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 空 e;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_empty_struct.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 空结构体解析成功\n");
}

/* 测试2: 深度嵌套结构体 */
static void test_deeply_nested_struct(void)
{
    printf("  [2] 测试深度嵌套结构体...\n");
    
    const char *source = 
        "结构体 层级1 {\n"
        "    整数 值1;\n"
        "};\n"
        "\n"
        "结构体 层级2 {\n"
        "    层级1 子1;\n"
        "    整数 值2;\n"
        "};\n"
        "\n"
        "结构体 层级3 {\n"
        "    层级2 子2;\n"
        "    整数 值3;\n"
        "};\n"
        "\n"
        "结构体 层级4 {\n"
        "    层级3 子3;\n"
        "    整数 值4;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 层级4 深层;\n"
        "    深层.子3.子2.子1.值1 = 42;\n"
        "    打印(\"%d\\n\", 深层.子3.子2.子1.值1);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_nested_struct.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 深度嵌套结构体解析成功\n");
}

/* 测试3: 大型结构体（多字段） */
static void test_large_struct(void)
{
    printf("  [3] 测试大型结构体...\n");
    
    const char *source = 
        "结构体 大型数据 {\n"
        "    整数 字段1;\n"
        "    整数 字段2;\n"
        "    整数 字段3;\n"
        "    整数 字段4;\n"
        "    整数 字段5;\n"
        "    整数 字段6;\n"
        "    整数 字段7;\n"
        "    整数 字段8;\n"
        "    整数 字段9;\n"
        "    整数 字段10;\n"
        "    整数 字段11;\n"
        "    整数 字段12;\n"
        "    整数 字段13;\n"
        "    整数 字段14;\n"
        "    整数 字段15;\n"
        "    整数 字段16;\n"
        "    整数 字段17;\n"
        "    整数 字段18;\n"
        "    整数 字段19;\n"
        "    整数 字段20;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 大型数据 数据;\n"
        "    数据.字段1 = 1;\n"
        "    数据.字段20 = 20;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_large_struct.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 大型结构体解析成功\n");
}

/* 测试4: 结构体对齐 */
static void test_struct_alignment(void)
{
    printf("  [4] 测试结构体对齐...\n");
    
    const char *source = 
        "结构体 对齐测试 {\n"
        "    字符 c1;        // 1字节\n"
        "    整数 i;         // 4字节，对齐到4字节边界\n"
        "    字符 c2;        // 1字节\n"
        "    长整数 l;       // 8字节，对齐到8字节边界\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 对齐测试 t;\n"
        "    打印(\"size = %zu\\n\", 求大小(对齐测试));\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_struct_align.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 结构体对齐解析成功\n");
}

/* 测试5: 匿名结构体成员 */
static void test_anonymous_struct_member(void)
{
    printf("  [5] 测试匿名结构体成员...\n");
    
    const char *source = 
        "结构体 外层 {\n"
        "    整数 外部字段;\n"
        "    结构体 {\n"
        "        整数 内部字段1;\n"
        "        整数 内部字段2;\n"
        "    } 内层;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 外层 o;\n"
        "    o.外部字段 = 1;\n"
        "    o.内层.内部字段1 = 2;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_anon_struct.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 匿名结构体成员解析成功\n");
}

/* 测试6: 结构体前向声明 */
static void test_struct_forward_declaration(void)
{
    printf("  [6] 测试结构体前向声明...\n");
    
    const char *source = 
        "// 前向声明\n"
        "结构体 节点;\n"
        "\n"
        "结构体 节点 {\n"
        "    整数 数据;\n"
        "    节点* 下一个;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 节点 n1;\n"
        "    变量 节点 n2;\n"
        "    n1.数据 = 1;\n"
        "    n1.下一个 = &n2;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_forward_decl.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 结构体前向声明解析成功\n");
}

/* 测试7: 结构体数组 */
static void test_struct_array(void)
{
    printf("  [7] 测试结构体数组...\n");
    
    const char *source = 
        "结构体 点 {\n"
        "    整数 x;\n"
        "    整数 y;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 点 点数组[10];\n"
        "    \n"
        "    循环(变量 整数 i = 0; i < 10; i++) {\n"
        "        点数组[i].x = i;\n"
        "        点数组[i].y = i * 2;\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_struct_array.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 结构体数组解析成功\n");
}

/* 测试8: 位字段结构体 */
static void test_bitfield_struct(void)
{
    printf("  [8] 测试位字段结构体...\n");
    
    const char *source = 
        "结构体 标志 {\n"
        "    整数 位1 : 1;\n"
        "    整数 位2 : 1;\n"
        "    整数 位3 : 1;\n"
        "    整数 位4_7 : 4;\n"
        "    整数 位8_15 : 8;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 标志 f;\n"
        "    f.位1 = 1;\n"
        "    f.位4_7 = 15;\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_bitfield.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 位字段结构体解析成功\n");
}

int main(void)
{
    printf("===== 阶段8边界测试：结构体 =====\n\n");
    
    test_empty_struct();
    test_deeply_nested_struct();
    test_large_struct();
    test_struct_alignment();
    test_anonymous_struct_member();
    test_struct_forward_declaration();
    test_struct_array();
    test_bitfield_struct();
    
    printf("\n✅ 所有结构体边界测试通过\n");
    return 0;
}
