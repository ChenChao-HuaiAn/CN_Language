#ifndef CNLANG_CLI_REPL_H
#define CNLANG_CLI_REPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CN Language REPL (Read-Eval-Print Loop) 公共接口
 * 
 * 本头文件定义 REPL 的公共 API，预留给未来模块化扩展使用
 */

#include <stdbool.h>
#include <stddef.h>

/* REPL 版本信息 */
#define CN_REPL_VERSION_MAJOR 0
#define CN_REPL_VERSION_MINOR 1
#define CN_REPL_VERSION_PATCH 0

/* REPL 执行结果状态码 */
typedef enum {
    CN_REPL_STATUS_OK = 0,          // 执行成功
    CN_REPL_STATUS_PARSE_ERROR,     // 解析错误
    CN_REPL_STATUS_SEMANTIC_ERROR,  // 语义错误
    CN_REPL_STATUS_RUNTIME_ERROR,   // 运行时错误
    CN_REPL_STATUS_EXIT             // 用户请求退出
} CnReplStatus;

/* REPL 配置选项 */
typedef struct {
    bool verbose;           // 详细输出模式
    bool show_ast;          // 是否显示 AST
    bool show_ir;           // 是否显示 IR
    const char *init_file;  // 初始化脚本文件路径（可选）
} CnReplOptions;

/*
 * 初始化 REPL 配置为默认值
 * 
 * @param options - 要初始化的配置结构指针
 */
void cn_repl_options_init(CnReplOptions *options);

/*
 * 执行单行 REPL 输入（预留接口，供外部调用）
 * 
 * @param input - 输入的代码字符串
 * @param options - REPL 配置选项
 * @return 执行状态码
 */
CnReplStatus cn_repl_eval_line(const char *input, const CnReplOptions *options);

/*
 * 启动 REPL 交互循环
 * 
 * @param options - REPL 配置选项
 * @return 退出状态码
 */
int cn_repl_run(const CnReplOptions *options);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_CLI_REPL_H */
