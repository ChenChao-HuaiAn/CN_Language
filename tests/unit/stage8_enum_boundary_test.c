/*
 * 阶段8边界测试：枚举类型
 * 
 * 测试目标：
 * - 负数枚举值
 * - 大范围枚举值
 * - 重复枚举值
 * - 枚举类型推断
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"

/* 测试1: 负数枚举值 */
static void test_negative_enum_values(void)
{
    printf("  [1] 测试负数枚举值...\n");
    
    const char *source = 
        "枚举 温度 {\n"
        "    极冷 = -30,\n"
        "    冷 = -10,\n"
        "    正常 = 0,\n"
        "    热 = 25,\n"
        "    极热 = 40\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 温度 t = 极冷;\n"
        "    打印(\"%d\\n\", t);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_negative_enum.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 负数枚举值解析成功\n");
}

/* 测试2: 大范围枚举值 */
static void test_large_enum_values(void)
{
    printf("  [2] 测试大范围枚举值...\n");
    
    const char *source = 
        "枚举 大数 {\n"
        "    小 = 0,\n"
        "    中 = 1000000,\n"
        "    大 = 2147483647   // INT_MAX\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 大数 n = 大;\n"
        "    打印(\"%d\\n\", n);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_large_enum.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 大范围枚举值解析成功\n");
}

/* 测试3: 十六进制枚举值 */
static void test_hex_enum_values(void)
{
    printf("  [3] 测试十六进制枚举值...\n");
    
    const char *source = 
        "枚举 权限 {\n"
        "    读 = 0x01,\n"
        "    写 = 0x02,\n"
        "    执行 = 0x04,\n"
        "    全部 = 0xFF\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 权限 p = 读;\n"
        "    打印(\"0x%x\\n\", p);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_hex_enum.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 十六进制枚举值解析成功\n");
}

/* 测试4: 自动递增枚举值 */
static void test_auto_increment_enum(void)
{
    printf("  [4] 测试自动递增枚举值...\n");
    
    const char *source = 
        "枚举 星期 {\n"
        "    周一 = 1,\n"
        "    周二,        // 2\n"
        "    周三,        // 3\n"
        "    周四,        // 4\n"
        "    周五,        // 5\n"
        "    周六,        // 6\n"
        "    周日         // 7\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    打印(\"%d %d %d\\n\", 周一, 周三, 周日);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_auto_enum.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 自动递增枚举值解析成功\n");
}

/* 测试5: 枚举作为函数参数和返回值 */
static void test_enum_as_parameter_and_return(void)
{
    printf("  [5] 测试枚举作为函数参数和返回值...\n");
    
    const char *source = 
        "枚举 状态 {\n"
        "    成功,\n"
        "    失败,\n"
        "    等待\n"
        "};\n"
        "\n"
        "函数 检查状态(状态 s) -> 状态 {\n"
        "    如果 (s == 成功) {\n"
        "        返回 成功;\n"
        "    }\n"
        "    返回 失败;\n"
        "}\n"
        "\n"
        "函数 测试() {\n"
        "    变量 状态 结果 = 检查状态(成功);\n"
        "    打印(\"%d\\n\", 结果);\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_enum_param.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 枚举作为参数和返回值解析成功\n");
}

/* 测试6: 枚举与switch语句 */
static void test_enum_with_switch(void)
{
    printf("  [6] 测试枚举与switch语句...\n");
    
    const char *source = 
        "枚举 方向 {\n"
        "    北,\n"
        "    东,\n"
        "    南,\n"
        "    西\n"
        "};\n"
        "\n"
        "函数 处理方向(方向 d) {\n"
        "    选择 (d) {\n"
        "        情况 北:\n"
        "            打印(\"向北\\n\");\n"
        "            跳出;\n"
        "        情况 东:\n"
        "            打印(\"向东\\n\");\n"
        "            跳出;\n"
        "        情况 南:\n"
        "            打印(\"向南\\n\");\n"
        "            跳出;\n"
        "        情况 西:\n"
        "            打印(\"向西\\n\");\n"
        "            跳出;\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_enum_switch.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 枚举switch语句解析成功\n");
}

/* 测试7: 枚举数组 */
static void test_enum_array(void)
{
    printf("  [7] 测试枚举数组...\n");
    
    const char *source = 
        "枚举 颜色 {\n"
        "    红,\n"
        "    绿,\n"
        "    蓝\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 颜色 调色板[3] = {红, 绿, 蓝};\n"
        "    \n"
        "    循环(变量 整数 i = 0; i < 3; i++) {\n"
        "        打印(\"颜色[%d] = %d\\n\", i, 调色板[i]);\n"
        "    }\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_enum_array.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 枚举数组解析成功\n");
}

/* 测试8: 枚举作为结构体成员 */
static void test_enum_in_struct(void)
{
    printf("  [8] 测试枚举作为结构体成员...\n");
    
    const char *source = 
        "枚举 类型 {\n"
        "    类型A,\n"
        "    类型B,\n"
        "    类型C\n"
        "};\n"
        "\n"
        "结构体 对象 {\n"
        "    整数 id;\n"
        "    类型 种类;\n"
        "    字符* 名称;\n"
        "};\n"
        "\n"
        "函数 测试() {\n"
        "    变量 对象 obj;\n"
        "    obj.id = 1;\n"
        "    obj.种类 = 类型A;\n"
        "    obj.名称 = \"对象1\";\n"
        "}\n";
    
    CnLexer lexer;
    cn_frontend_lexer_init(&lexer, source, strlen(source), "test_enum_struct.cn");
    
    CnParser parser;
    cn_frontend_parser_init(&parser, &lexer);
    
    CnAstProgram *program = cn_frontend_parser_parse_program(&parser);
    
    assert(program != NULL);
    printf("    ✓ 枚举结构体成员解析成功\n");
}

int main(void)
{
    printf("===== 阶段8边界测试：枚举类型 =====\n\n");
    
    test_negative_enum_values();
    test_large_enum_values();
    test_hex_enum_values();
    test_auto_increment_enum();
    test_enum_as_parameter_and_return();
    test_enum_with_switch();
    test_enum_array();
    test_enum_in_struct();
    
    printf("\n✅ 所有枚举类型边界测试通过\n");
    return 0;
}
