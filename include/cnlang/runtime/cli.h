#ifndef CNLANG_RUNTIME_CLI_H
#define CNLANG_RUNTIME_CLI_H

#include <stddef.h>

/*
 * CN Language 命令行参数运行时库
 * 提供命令行参数处理功能，支持自托管编译器
 *
 * 功能：
 * - 获取命令行参数个数和内容
 * - 查找选项值（如 -o output）
 * - 检查选项是否存在（如 --verbose）
 * - 支持短选项（-o）和长选项（--output）
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 命令行参数初始化
// =============================================================================

/**
 * 初始化命令行参数模块
 * @param argc 参数个数（来自 main 函数）
 * @param argv 参数数组（来自 main 函数）
 * @note 此函数由运行时初始化自动调用，用户通常不需要手动调用
 */
void cn_rt_cli_init(int argc, char **argv);

/**
 * 清理命令行参数模块
 * @note 此函数由运行时退出自动调用
 */
void cn_rt_cli_cleanup(void);

// =============================================================================
// 基础参数获取函数
// =============================================================================

/**
 * 获取命令行参数个数
 * @return 参数个数（argc）
 */
int cn_rt_cli_argc(void);

/**
 * 获取指定索引的命令行参数
 * @param index 参数索引（0 到 argc-1）
 * @return 参数字符串，如果索引越界返回 NULL
 */
const char* cn_rt_cli_argv(int index);

/**
 * 获取程序名称（第0个参数）
 * @return 程序名称字符串
 */
const char* cn_rt_cli_program_name(void);

// =============================================================================
// 选项查找函数
// =============================================================================

/**
 * 查找选项的值
 * 支持短选项（-o value）和长选项（--output value）
 * 
 * @param option_name 选项名称（不含前导横杠）
 *                    短选项如 "o"，长选项如 "output"
 * @return 选项值字符串，如果选项不存在或没有值则返回 NULL
 * 
 * @example
 * // 命令行: ./program -o output.txt --input test.cn
 * cn_rt_cli_find_option("o");     // 返回 "output.txt"
 * cn_rt_cli_find_option("output"); // 返回 "output.txt"（同上，支持短/长选项）
 * cn_rt_cli_find_option("input");  // 返回 "test.cn"
 * cn_rt_cli_find_option("x");      // 返回 NULL
 */
const char* cn_rt_cli_find_option(const char* option_name);

/**
 * 检查选项是否存在
 * 支持短选项（-v）和长选项（--verbose）
 * 
 * @param option_name 选项名称（不含前导横杠）
 * @return 1 表示存在，0 表示不存在
 * 
 * @example
 * // 命令行: ./program -v --verbose --debug
 * cn_rt_cli_has_option("v");       // 返回 1
 * cn_rt_cli_has_option("verbose"); // 返回 1
 * cn_rt_cli_has_option("debug");   // 返回 1
 * cn_rt_cli_has_option("help");    // 返回 0
 */
int cn_rt_cli_has_option(const char* option_name);

// =============================================================================
// 高级选项查找函数
// =============================================================================

/**
 * 查找选项的值，支持同时指定短选项和长选项
 * 
 * @param short_opt 短选项名称（如 "o"），可以为 NULL
 * @param long_opt  长选项名称（如 "output"），可以为 NULL
 * @return 选项值字符串，如果选项不存在或没有值则返回 NULL
 * 
 * @example
 * // 命令行: ./program -o output.txt
 * cn_rt_cli_find_option_ex("o", "output"); // 返回 "output.txt"
 */
const char* cn_rt_cli_find_option_ex(const char* short_opt, const char* long_opt);

/**
 * 检查选项是否存在，支持同时指定短选项和长选项
 * 
 * @param short_opt 短选项名称（如 "v"），可以为 NULL
 * @param long_opt  长选项名称（如 "verbose"），可以为 NULL
 * @return 1 表示存在，0 表示不存在
 */
int cn_rt_cli_has_option_ex(const char* short_opt, const char* long_opt);

/**
 * 获取非选项参数
 * 跳过选项及其值，返回位置参数
 * 
 * @param index 位置参数索引（从0开始）
 * @return 位置参数字符串，如果索引越界返回 NULL
 * 
 * @example
 * // 命令行: ./program -o output.txt file1.cn file2.cn
 * cn_rt_cli_get_positional_arg(0); // 返回 "file1.cn"
 * cn_rt_cli_get_positional_arg(1); // 返回 "file2.cn"
 */
const char* cn_rt_cli_get_positional_arg(int index);

/**
 * 获取非选项参数个数
 * @return 位置参数个数
 */
int cn_rt_cli_get_positional_arg_count(void);

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

// 初始化和清理
#define 初始化命令行参数 cn_rt_cli_init
#define 清理命令行参数 cn_rt_cli_cleanup

// 基础参数获取
#define 获取参数个数 cn_rt_cli_argc
#define 获取参数 cn_rt_cli_argv
#define 获取程序名称 cn_rt_cli_program_name

// 选项查找
#define 查找选项 cn_rt_cli_find_option
#define 选项存在 cn_rt_cli_has_option
#define 查找选项扩展 cn_rt_cli_find_option_ex
#define 选项存在扩展 cn_rt_cli_has_option_ex

// 位置参数
#define 获取位置参数 cn_rt_cli_get_positional_arg
#define 获取位置参数个数 cn_rt_cli_get_positional_arg_count

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_CLI_H */
