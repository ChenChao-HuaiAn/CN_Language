#include "cnlang/frontend/semantics.h"

// TODO: 名称解析语义 pass 当前仅为占位实现，后续需要在此处遍历 AST 并使用 diagnostics 报告语义错误。
bool cn_sem_resolve_names(CnSemScope *global_scope,
                          CnAstProgram *program,
                          struct CnDiagnostics *diagnostics)
{
    (void)global_scope;
    (void)program;
    (void)diagnostics;

    return true;
}

// TODO: 类型检查语义 pass 当前仅为占位实现，后续需要在此处对表达式和语句进行类型推断与检查。
bool cn_sem_check_types(CnSemScope *global_scope,
                        CnAstProgram *program,
                        struct CnDiagnostics *diagnostics)
{
    (void)global_scope;
    (void)program;
    (void)diagnostics;

    return true;
}
