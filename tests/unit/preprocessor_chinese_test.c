#include "cnlang/frontend/preprocessor.h"
#include "cnlang/support/diagnostics.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试中文预处理器指令 */

static void test_chinese_define(void) {
    const char *source = 
        "#定义 最大值 100\n"
        "变量 x = 最大值;\n";
    
    printf("测试: 中文 #定义 指令\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "100") != NULL);  // 宏被展开为 100
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文 #定义 指令测试通过\n");
}

static void test_chinese_ifdef(void) {
    const char *source = 
        "#定义 调试模式\n"
        "#如果定义 调试模式\n"
        "变量 x = 1;\n"
        "#结束如果\n";
    
    printf("测试: 中文 #如果定义 指令\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "变量 x = 1") != NULL);  // 条件块被保留
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文 #如果定义 指令测试通过\n");
}

static void test_chinese_ifndef(void) {
    const char *source = 
        "#如果未定义 不存在\n"
        "变量 y = 2;\n"
        "#结束如果\n";
    
    printf("测试: 中文 #如果未定义 指令\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "变量 y = 2") != NULL);  // 条件块被保留
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文 #如果未定义 指令测试通过\n");
}

static void test_chinese_else(void) {
    const char *source = 
        "#定义 功能A\n"
        "#如果定义 功能B\n"
        "变量 a = 1;\n"
        "#否则\n"
        "变量 b = 2;\n"
        "#结束如果\n";
    
    printf("测试: 中文 #否则 指令\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "变量 b = 2") != NULL);  // else 分支被保留
    assert(strstr(preprocessor.output, "变量 a = 1") == NULL);  // if 分支被跳过
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文 #否则 指令测试通过\n");
}

static void test_chinese_undef(void) {
    const char *source = 
        "#定义 临时 999\n"
        "#未定义 临时\n"
        "变量 x = 临时;\n";  // 临时不应该被展开
    
    printf("测试: 中文 #未定义 指令\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "临时") != NULL);   // 宏名保留
    assert(strstr(preprocessor.output, "999") == NULL);   // 不展开
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文 #未定义 指令测试通过\n");
}

static void test_chinese_nested(void) {
    const char *source = 
        "#定义 外层\n"
        "#如果定义 外层\n"
        "#定义 内层\n"
        "#如果定义 内层\n"
        "变量 z = 3;\n"
        "#结束如果\n"
        "#结束如果\n";
    
    printf("测试: 中文嵌套条件编译\n");
    
    CnDiagnostics diagnostics;
    cn_support_diagnostics_init(&diagnostics);
    
    CnPreprocessor preprocessor;
    cn_frontend_preprocessor_init(&preprocessor, source, strlen(source), "test.cn");
    cn_frontend_preprocessor_set_diagnostics(&preprocessor, &diagnostics);
    
    bool success = cn_frontend_preprocessor_process(&preprocessor);
    
    assert(success);
    assert(strstr(preprocessor.output, "变量 z = 3") != NULL);  // 嵌套块被保留
    
    cn_frontend_preprocessor_free(&preprocessor);
    cn_support_diagnostics_free(&diagnostics);
    
    printf("  ✓ 中文嵌套条件编译测试通过\n");
}

int main(void) {
    printf("=== 中文预处理器指令测试 ===\n\n");
    
    test_chinese_define();
    test_chinese_ifdef();
    test_chinese_ifndef();
    test_chinese_else();
    test_chinese_undef();
    test_chinese_nested();
    
    printf("\n所有中文预处理器测试通过!\n");
    return 0;
}
