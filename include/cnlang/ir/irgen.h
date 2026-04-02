#ifndef CN_IR_IRGEN_H
#define CN_IR_IRGEN_H

#include "cnlang/ir/ir.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
struct CnIrGenStaticVar;
struct CnIrLocalVarEntry;

// IR 生成上下文
typedef struct CnIrGenContext {
    CnIrModule *module;
    CnIrFunction *current_func;
    CnIrBasicBlock *current_block;
    
    // 用于 break/continue 的目标块
    CnIrBasicBlock *loop_exit;
    CnIrBasicBlock *loop_continue;
    
    // 语义分析作用域（用于查找符号信息）
    CnSemScope *global_scope;
    CnSemScope *current_scope;
    
    // AST程序（用于查找类定义）
    CnAstProgram *program;
    
    // 当前函数中的静态变量列表（用于跟踪静态变量名）
    struct CnIrGenStaticVar *current_static_vars;
    
    // 当前函数中的局部变量名映射表（原始名 -> 唯一名）
    struct CnIrLocalVarEntry *local_var_map;
} CnIrGenContext;

// 主入口：将 AST 程序转换为 IR 模块
CnIrModule *cn_ir_gen_program(CnAstProgram *program, CnSemScope *global_scope, CnTargetTriple target, CnCompileMode mode);

// 辅助接口（内部使用，但可用于单元测试）
CnIrGenContext *cn_ir_gen_context_new();
void cn_ir_gen_context_free(CnIrGenContext *ctx);

void cn_ir_gen_function(CnIrGenContext *ctx, CnAstFunctionDecl *func, CnSemScope *parent_scope);
void cn_ir_gen_block(CnIrGenContext *ctx, CnAstBlockStmt *block);
void cn_ir_gen_stmt(CnIrGenContext *ctx, CnAstStmt *stmt);
CnIrOperand cn_ir_gen_expr(CnIrGenContext *ctx, CnAstExpr *expr);

#ifdef __cplusplus
}
#endif

#endif /* CN_IR_IRGEN_H */
