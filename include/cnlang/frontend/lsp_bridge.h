#ifndef CN_FRONTEND_LSP_BRIDGE_H
#define CN_FRONTEND_LSP_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

// LSP 位置信息（基于 LSP 规范：行列从 0 开始）
typedef struct CnLspPosition {
    int line;   // 行号（0-based）
    int column; // 列号（0-based，UTF-16 代码单元）
} CnLspPosition;

typedef struct CnLspRange {
    CnLspPosition start;
    CnLspPosition end;
} CnLspRange;

// LSP 符号信息
typedef struct CnLspSymbolInfo {
    const char *name;           // 符号名称
    CnSemSymbolKind kind;       // 符号类型（变量/函数）
    CnLspRange definition_range; // 定义位置
    CnType *type;               // 类型信息（可选）
} CnLspSymbolInfo;

// LSP 诊断严重级别（对应 LSP DiagnosticSeverity）
typedef enum CnLspDiagnosticSeverity {
    CN_LSP_DIAG_ERROR = 1,
    CN_LSP_DIAG_WARNING = 2,
    CN_LSP_DIAG_INFO = 3,
    CN_LSP_DIAG_HINT = 4
} CnLspDiagnosticSeverity;

// LSP 诊断信息
typedef struct CnLspDiagnostic {
    CnLspRange range;
    CnLspDiagnosticSeverity severity;
    const char *message;
    const char *source; // 固定为 "CN_Language"
} CnLspDiagnostic;

// 文档分析结果（封装编译前端输出）
typedef struct CnLspDocumentAnalysis {
    const char *source;          // 源代码文本（不拥有所有权，由上层管理）
    size_t source_length;        // 源代码长度
    CnAstProgram *program;      // AST 根节点
    CnSemScope *global_scope;   // 全局作用域（含符号表）
    CnLspDiagnostic *diagnostics; // 诊断信息数组
    size_t diagnostic_count;
} CnLspDocumentAnalysis;

// 分析文档：执行 Lexer + Parser + Semantics，返回分析结果
// 参数：
//   source: 源代码文本
//   source_length: 源代码长度
//   uri: 文档 URI（用于诊断信息）
// 返回：
//   分析结果结构，失败时返回 NULL
CnLspDocumentAnalysis *cn_lsp_analyze_document(
    const char *source,
    size_t source_length,
    const char *uri
);

// 释放分析结果
void cn_lsp_free_analysis(CnLspDocumentAnalysis *analysis);

// 查找指定位置的符号定义
// 参数：
//   analysis: 文档分析结果
//   position: LSP 位置（0-based 行列）
//   out_symbol: 输出符号信息
// 返回：
//   找到符号返回 true，否则返回 false
bool cn_lsp_find_definition(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspSymbolInfo *out_symbol
);

// 查找符号的所有引用
// 参数：
//   analysis: 文档分析结果
//   position: LSP 位置（指向符号使用或定义处）
//   out_ranges: 输出引用位置数组（调用者需释放）
//   out_count: 输出引用数量
// 返回：
//   找到引用返回 true，否则返回 false
bool cn_lsp_find_references(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspRange **out_ranges,
    size_t *out_count
);

// 获取文档符号列表（函数、变量等）
// 参数：
//   analysis: 文档分析结果
//   out_symbols: 输出符号信息数组（调用者需释放）
//   out_count: 输出符号数量
// 返回：
//   成功返回 true，否则返回 false
bool cn_lsp_get_document_symbols(
    CnLspDocumentAnalysis *analysis,
    CnLspSymbolInfo **out_symbols,
    size_t *out_count
);

// 转换诊断信息：CnDiagnostics -> CnLspDiagnostic[]
// 参数：
//   diagnostics: 编译器诊断信息
//   out_diagnostics: 输出 LSP 诊断数组（调用者需释放）
//   out_count: 输出诊断数量
void cn_lsp_convert_diagnostics(
    const CnDiagnostics *diagnostics,
    CnLspDiagnostic **out_diagnostics,
    size_t *out_count
);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_LSP_BRIDGE_H */
