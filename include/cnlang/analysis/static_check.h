#ifndef CN_ANALYSIS_STATIC_CHECK_H
#define CN_ANALYSIS_STATIC_CHECK_H

#include "cnlang/frontend/ast.h"
#include "cnlang/support/diagnostics.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 静态检查规则枚举
 * 每个规则对应一类静态代码质量检查
 */
typedef enum CnCheckRule {
    CN_CHECK_RULE_UNUSED_VAR = 0,         // 未使用的局部变量
    CN_CHECK_RULE_UNUSED_PARAM,           // 未使用的函数参数
    CN_CHECK_RULE_COMPLEXITY,             // 函数复杂度超限
    CN_CHECK_RULE_COUNT                   // 规则总数
} CnCheckRule;

/*
 * 静态检查配置
 */
typedef struct CnCheckConfig {
    bool enabled_rules[CN_CHECK_RULE_COUNT];  // 每个规则是否启用
    int max_statements_per_function;           // 函数最大语句数（复杂度指标）
    int max_nesting_level;                     // 最大嵌套层级
    bool check_only;                           // 仅检查模式（用于 CI）
} CnCheckConfig;

/*
 * 静态检查结果统计
 */
typedef struct CnCheckStats {
    size_t files_checked;                      // 检查的文件数
    size_t warnings_count;                     // 警告数量
    size_t errors_count;                       // 错误数量（严重问题）
} CnCheckStats;

/*
 * 初始化静态检查配置为默认值
 * - 启用所有规则
 * - 函数最大语句数: 50
 * - 最大嵌套层级: 5
 */
void cn_check_config_init_default(CnCheckConfig *config);

/*
 * 对单个 AST 程序执行静态检查
 * 
 * @param program AST 程序节点
 * @param config 检查配置
 * @param diagnostics 诊断信息收集器
 * @param filename 源文件名（用于诊断报告）
 * @return 检查是否通过（没有错误）
 */
bool cn_check_program(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename);

/*
 * 对单个文件执行静态检查
 * 
 * @param filename 源文件路径
 * @param config 检查配置
 * @param diagnostics 诊断信息收集器
 * @return 检查是否通过
 */
bool cn_check_file(
    const char *filename,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics);

/*
 * 规则相关辅助函数
 */

// 检查未使用的变量和参数
bool cn_check_unused_symbols(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename);

// 检查函数复杂度
bool cn_check_complexity(
    const CnAstProgram *program,
    const CnCheckConfig *config,
    CnDiagnostics *diagnostics,
    const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* CN_ANALYSIS_STATIC_CHECK_H */
