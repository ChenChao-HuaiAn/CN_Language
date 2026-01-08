/**
 * @file test_lexer_scanner.c
 * @brief 词法分析器扫描器模块测试
 * 
 * 测试CN_lexer_scanner模块的功能，包括：
 * 1. 字符扫描和位置管理
 * 2. 源文件读取和缓冲区管理
 * 3. 错误位置跟踪
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

#include "src/core/lexer/scanner/CN_lexer_scanner.h"

/* 测试用例定义 */
typedef struct {
    const char* test_name;
    bool (*test_func)(void);
} TestCase;

/* 测试辅助函数声明 */
static bool test_scanner_initialization(void);
static bool test_scanner_position_tracking(void);
static bool test_scanner_character_reading(void);
static bool test_scanner_peek_functions(void);
static bool test_scanner_error_handling(void);
static bool test_scanner_source_management(void);

/* 测试用例数组 */
static TestCase test_cases[] = {
    {"扫描器初始化", test_scanner_initialization},
    {"位置跟踪", test_scanner_position_tracking},
    {"字符读取", test_scanner_character_reading},
    {"前瞻函数", test_scanner_peek_functions},
    {"错误处理", test_scanner_error_handling},
    {"源文件管理", test_scanner_source_management},
};

static int total_tests = sizeof(test_cases) / sizeof(test_cases[0]);
static int tests_passed = 0;
static int tests_failed = 0;

/**
 * @brief 运行所有扫描器测试
 * 
 * @return true 所有测试通过
 * @return false 有测试失败
 */
bool test_lexer_scanner_all(void)
{
    printf("\n");
    printf("========================================\n");
    printf("    词法分析器扫描器模块测试\n");
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
 * @brief 测试扫描器初始化
 */
static bool test_scanner_initialization(void)
{
    // 创建扫描器状态
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        printf("错误: 无法创建扫描器状态\n");
        return false;
    }
    
    // 测试初始状态
    if (scanner->source != NULL) {
        printf("错误: 初始源代码应为NULL\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    if (scanner->source_length != 0) {
        printf("错误: 初始源代码长度应为0\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    if (scanner->current_line != 1) {
        printf("错误: 初始行号应为1\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    if (scanner->current_column != 1) {
        printf("错误: 初始列号应为1\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    if (scanner->current_pos != 0) {
        printf("错误: 初始位置应为0\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    if (scanner->has_error != false) {
        printf("错误: 初始错误状态应为false\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 清理
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 测试位置跟踪
 */
static bool test_scanner_position_tracking(void)
{
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        return false;
    }
    
    // 设置测试源代码
    const char* source_code = "测试\n代码\n位置";
    size_t source_length = strlen(source_code);
    if (!F_initialize_scanner_state(scanner, source_code, source_length, "test.cn")) {
        printf("错误: 无法初始化扫描器状态\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试初始位置
    if (scanner->current_line != 1 || scanner->current_column != 1) {
        printf("错误: 初始位置不正确\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 读取字符并检查位置更新
    char c;
    size_t expected_line = 1;
    size_t expected_column = 1;
    
    while ((c = F_next_char(scanner)) != '\0') {
        // 注意：读取字符后，位置已经更新
        // 所以我们需要检查读取后的位置
        // 对于第一个字符，读取后应该是行1列2
        
        // 更新期望位置（读取字符后）
        if (c == '\n') {
            expected_line++;
            expected_column = 1;
        } else {
            expected_column++;
        }
        
        // 检查位置
        if (scanner->current_line != expected_line || scanner->current_column != expected_column) {
            printf("错误: 位置跟踪不正确 (字符: 0x%02X, 期望: 行%zu列%zu, 实际: 行%zu列%zu)\n",
                   (unsigned char)c, expected_line, expected_column, scanner->current_line, scanner->current_column);
            F_destroy_scanner_state(scanner);
            return false;
        }
    }
    
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 测试字符读取
 */
static bool test_scanner_character_reading(void)
{
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        return false;
    }
    
    const char* source_code = "ABC\n123\nXYZ";
    size_t source_length = strlen(source_code);
    if (!F_initialize_scanner_state(scanner, source_code, source_length, "test.cn")) {
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试读取顺序
    char expected_chars[] = {'A', 'B', 'C', '\n', '1', '2', '3', '\n', 'X', 'Y', 'Z', '\0'};
    
    for (size_t i = 0; i < sizeof(expected_chars); i++) {
        char c = F_next_char(scanner);
        if (c != expected_chars[i]) {
            printf("错误: 字符读取不正确 (索引: %zu, 期望: %c(0x%02X), 实际: %c(0x%02X))\n",
                   i, expected_chars[i], expected_chars[i], c, c);
            F_destroy_scanner_state(scanner);
            return false;
        }
    }
    
    // 测试读取结束后的行为
    char c = F_next_char(scanner);
    if (c != '\0') {
        printf("错误: 读取结束后应返回'\\0'\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 测试前瞻函数
 */
static bool test_scanner_peek_functions(void)
{
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        return false;
    }
    
    const char* source_code = "测试前瞻函数";
    size_t source_length = strlen(source_code);
    if (!F_initialize_scanner_state(scanner, source_code, source_length, "test.cn")) {
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试peek_char（查看当前字符）
    char peeked = F_peek_char(scanner);
    // 注意：'测'是UTF-8编码的3字节字符，不能直接与char比较
    // 我们检查第一个字节（0xE6）或使用字符串比较
    if (peeked != (char)0xE6) { // '测'的UTF-8第一个字节
        printf("错误: peek_char返回错误字符 (期望: 0xE6, 实际: 0x%02X)\n", (unsigned char)peeked);
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 读取一个字符（实际上是'测'的第一个字节）
    char current = F_next_char(scanner);
    if (current != (char)0xE6) {
        printf("错误: next_char返回错误字符 (期望: 0xE6, 实际: 0x%02X)\n", (unsigned char)current);
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 跳过'测'的剩余字节（UTF-8编码）
    F_next_char(scanner); // 第二个字节
    F_next_char(scanner); // 第三个字节
    
    // 再次测试peek_char（查看下一个字符）
    peeked = F_peek_char(scanner);
    if (peeked != (char)0xE8) { // '试'的UTF-8第一个字节
        printf("错误: peek_char返回错误字符 (期望: 0xE8, 实际: 0x%02X)\n", (unsigned char)peeked);
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试has_more_chars
    if (!F_has_more_chars(scanner)) {
        printf("错误: 应有更多字符\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 测试错误处理
 */
static bool test_scanner_error_handling(void)
{
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        return false;
    }
    
    // 测试空源代码初始化
    if (F_initialize_scanner_state(scanner, NULL, 0, NULL)) {
        printf("错误: 空源代码应返回false\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试空字符串初始化
    if (!F_initialize_scanner_state(scanner, "", 0, "test.cn")) {
        printf("错误: 空字符串应返回true\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试读取空字符串
    char c = F_next_char(scanner);
    if (c != '\0') {
        printf("错误: 空字符串应返回'\\0'\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试错误设置和获取
    F_set_scanner_error(scanner, "测试错误: %s", "错误信息");
    if (!F_scanner_has_errors(scanner)) {
        printf("错误: 应有错误状态\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    const char* error_msg = F_get_scanner_last_error(scanner);
    if (strstr(error_msg, "测试错误") == NULL) {
        printf("错误: 错误信息不正确\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试清除错误
    F_clear_scanner_error(scanner);
    if (F_scanner_has_errors(scanner)) {
        printf("错误: 错误状态应已清除\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 测试源文件管理
 */
static bool test_scanner_source_management(void)
{
    Stru_LexerScannerState_t* scanner = F_create_scanner_state();
    if (scanner == NULL) {
        return false;
    }
    
    // 测试设置源代码
    const char* source1 = "第一段代码";
    size_t source1_length = strlen(source1);
    if (!F_initialize_scanner_state(scanner, source1, source1_length, "test1.cn")) {
        printf("错误: 无法初始化扫描器状态\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 读取部分字符
    F_next_char(scanner);
    F_next_char(scanner);
    
    // 测试重置扫描器状态
    F_reset_scanner_state(scanner);
    
    // 验证重置后的状态
    if (scanner->current_line != 1 || scanner->current_column != 1 || scanner->current_pos != 0) {
        printf("错误: 重置后状态不正确\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 重新初始化不同的源代码
    const char* source2 = "第二段代码";
    size_t source2_length = strlen(source2);
    if (!F_initialize_scanner_state(scanner, source2, source2_length, "test2.cn")) {
        printf("错误: 无法重新初始化扫描器状态\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 读取新源代码
    char c = F_next_char(scanner);
    // '第'的UTF-8第一个字节是0xE7
    if (c != (char)0xE7) {
        printf("错误: 重置后读取错误字符 (期望: 0xE7, 实际: 0x%02X)\n", (unsigned char)c);
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    // 测试获取位置和源文件名
    size_t line, column;
    F_get_scanner_position(scanner, &line, &column);
    if (line != 1 || column != 2) {
        printf("错误: 位置获取不正确\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    const char* source_name = F_get_scanner_source_name(scanner);
    if (strcmp(source_name, "test2.cn") != 0) {
        printf("错误: 源文件名获取不正确\n");
        F_destroy_scanner_state(scanner);
        return false;
    }
    
    F_destroy_scanner_state(scanner);
    return true;
}

/**
 * @brief 主函数（用于独立测试）
 */
#ifdef TEST_SCANNER_STANDALONE
int main(void)
{
    bool result = test_lexer_scanner_all();
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
