#ifndef CN_FORMAT_FORMATTER_H
#define CN_FORMAT_FORMATTER_H

#include <stdio.h>
#include <stdbool.h>
#include "cnlang/frontend/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

// 格式化配置（基于 CN_Language C 代码风格规范）
typedef struct CnFormatConfig {
    // 缩进与空白 (规范 3.1)
    int indent_size;           // 缩进空格数，默认 4（规范要求 4 空格）
    int max_line_width;        // 最大行宽，默认 100（规范建议 100 列）
    
    // 空格规则 (规范 3.3)
    bool space_around_ops;     // 运算符两侧是否加空格，默认 true
    bool space_after_comma;    // 逗号后是否加空格，默认 true
    bool space_after_keywords; // 关键字后是否加空格（if/while/for 等），默认 true
    
    // 括号与大括号 (规范 3.2)
    bool brace_on_new_line_func;    // 函数定义的左大括号是否独立成行，默认 true
    bool brace_on_same_line_ctrl;   // 控制语句的左大括号是否在同一行，默认 true
    bool always_use_braces;         // 即使只有一条语句也使用大括号，默认 true
    
    // 空行使用 (规范 3.1)
    bool empty_line_between_funcs;  // 函数之间是否用空行分隔，默认 true
    int max_consecutive_empty_lines; // 最大连续空行数，默认 1
} CnFormatConfig;

// 格式化选项
typedef struct CnFormatOptions {
    bool in_place;             // 是否原地修改文件
    bool to_stdout;            // 是否输出到标准输出
    const char *output_file;   // 输出文件路径（可选）
    bool check_only;           // 仅检查模式：不修改文件，只检查是否需要格式化
    bool verify_idempotence;   // 验证幂等性：检查多次格式化结果是否一致
    CnFormatConfig config;     // 格式化配置
} CnFormatOptions;

// 初始化默认格式化配置
void cn_format_config_init_default(CnFormatConfig *config);

// 初始化默认格式化选项
void cn_format_options_init_default(CnFormatOptions *options);

// 格式化单个程序 AST 并输出到文件
// 返回值：true 表示成功，false 表示失败
bool cn_format_program_to_file(
    const CnAstProgram *program,
    FILE *output,
    const CnFormatConfig *config
);

// 格式化单个程序 AST 并返回字符串（调用者负责释放）
char *cn_format_program_to_string(
    const CnAstProgram *program,
    const CnFormatConfig *config,
    size_t *out_length
);

// 格式化源代码文件
// 参数：
//   input_file: 输入文件路径
//   options: 格式化选项
// 返回值：true 表示成功，false 表示失败
bool cn_format_file(const char *input_file, const CnFormatOptions *options);

// 递归格式化目录下所有 .cn 文件
// 参数：
//   dir_path: 目录路径
//   options: 格式化选项
// 返回值：成功格式化的文件数量，失败返回 -1
int cn_format_directory(const char *dir_path, const CnFormatOptions *options);

#ifdef __cplusplus
}
#endif

#endif /* CN_FORMAT_FORMATTER_H */
