#ifndef CN_SUPPORT_MEMORY_ESTIMATOR_H
#define CN_SUPPORT_MEMORY_ESTIMATOR_H

#include <stddef.h>
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/ir/ir.h"
#include "cnlang/support/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 内存占用估算模块
 * 用于估算编译器内部各关键数据结构的内存占用
 */

/* 估算 AST 的内存占用（字节） */
size_t cn_mem_estimate_ast(const CnAstProgram *program);

/* 估算符号表的内存占用（字节） */
size_t cn_mem_estimate_symbol_table(const CnSemScope *global_scope);

/* 估算 IR 模块的内存占用（字节） */
size_t cn_mem_estimate_ir(const CnIrModule *module);

/* 估算诊断信息的内存占用（字节） */
size_t cn_mem_estimate_diagnostics(const CnDiagnostics *diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* CN_SUPPORT_MEMORY_ESTIMATOR_H */
