/**
 * @file exception_cgen.c
 * @brief CN语言异常处理代码生成实现
 * 
 * 实现try-catch-finally和throw语句的C代码生成
 * 使用setjmp/longjmp机制实现异常跳转
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include "cnlang/backend/cgen.h"
#include "cnlang/runtime/exception.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 生成缩进
 */
static void emit_indent(FILE *out, int level)
{
    for (int i = 0; i < level; i++) {
        fprintf(out, "    ");
    }
}

/**
 * @brief 生成唯一的标签名
 */
static void make_unique_label(char *buffer, size_t size, const char *prefix, int counter)
{
    snprintf(buffer, size, "__%s_%d", prefix, counter);
}

/* ============================================================================
 * try-catch-finally 代码生成
 * ============================================================================ */

/**
 * @brief 生成try语句的C代码
 * 
 * CN代码：
 *   尝试 {
 *       // try块
 *   } 捕获 (异常类型 e) {
 *       // catch块
 *   } 最终 {
 *       // finally块
 *   }
 * 
 * 生成的C代码：
 *   {
 *       CnExceptionFrame __frame;
 *       if (cn_try_begin(&__frame) == 0) {
 *           // try块
 *           cn_try_end(&__frame);
 *       } else {
 *           CnException* e = cn_catch("异常类型");
 *           if (e != NULL) {
 *               // catch块
 *           } else {
 *               cn_rethrow();
 *           }
 *       }
 *       // finally块（如果有）
 *   }
 */

/**
 * @brief 生成try-catch-finally语句的C代码
 * 
 * @param ctx 代码生成上下文
 * @param stmt try语句AST节点
 * @param emit_block_func 生成语句块的函数指针
 * @param user_data 用户数据（传递给emit_block_func）
 */
void cn_cgen_try_stmt(CnCCodeGenContext *ctx, CnAstStmt *stmt,
                       void (*emit_block_func)(CnCCodeGenContext *, CnAstBlockStmt *, void *),
                       void *user_data)
{
    if (!ctx || !stmt || stmt->kind != CN_AST_STMT_TRY) {
        return;
    }

    CnAstTryStmt *try_stmt = stmt->as.try_stmt;
    FILE *out = ctx->output_file;
    int counter = ctx->label_counter++;

    char try_label[64], catch_label[64], end_label[64];
    make_unique_label(try_label, sizeof(try_label), "try", counter);
    make_unique_label(catch_label, sizeof(catch_label), "catch", counter);
    make_unique_label(end_label, sizeof(end_label), "try_end", counter);

    /* 开始异常处理块 */
    emit_indent(out, 1);
    fprintf(out, "{\n");
    
    /* 声明异常帧 */
    emit_indent(out, 2);
    fprintf(out, "CnExceptionFrame __frame_%d;\n", counter);
    
    /* 声明异常变量（如果有catch块） */
    if (try_stmt->catch_count > 0) {
        emit_indent(out, 2);
        fprintf(out, "CnException* __ex_%d = NULL;\n", counter);
    }
    
    /* 开始try块 */
    emit_indent(out, 2);
    fprintf(out, "if (cn_try_begin(&__frame_%d) == 0) {\n", counter);
    
    /* 生成try块体 */
    if (try_stmt->try_block) {
        if (emit_block_func) {
            emit_block_func(ctx, try_stmt->try_block, user_data);
        } else {
            /* 默认生成空块 */
            emit_indent(out, 3);
            fprintf(out, "// try block\n");
        }
    }
    
    /* 结束try块 */
    emit_indent(out, 3);
    fprintf(out, "cn_try_end(&__frame_%d);\n", counter);
    
    emit_indent(out, 2);
    fprintf(out, "} else {\n");
    
    /* 生成catch块 */
    if (try_stmt->catch_count > 0) {
        for (size_t i = 0; i < try_stmt->catch_count; i++) {
            CnAstCatchClause *catch_clause = &try_stmt->catches[i];
            
            emit_indent(out, 3);
            if (catch_clause->exception_type) {
                fprintf(out, "__ex_%d = cn_catch(\"%.*s\");\n", 
                        counter,
                        (int)catch_clause->exception_type_length,
                        catch_clause->exception_type);
            } else {
                fprintf(out, "__ex_%d = cn_catch_any();\n", counter);
            }
            
            emit_indent(out, 3);
            fprintf(out, "if (__ex_%d != NULL) {\n", counter);
            
            /* 如果有异常变量，声明它 */
            if (catch_clause->var_name) {
                emit_indent(out, 4);
                fprintf(out, "CnException* %.*s = __ex_%d;\n",
                        (int)catch_clause->var_name_length,
                        catch_clause->var_name,
                        counter);
            }
            
            /* 生成catch块体 */
            if (catch_clause->body) {
                if (emit_block_func) {
                    emit_block_func(ctx, catch_clause->body, user_data);
                }
            }
            
            /* 标记异常已处理 */
            emit_indent(out, 4);
            fprintf(out, "__frame_%d.catch_handled = 1;\n", counter);
            
            emit_indent(out, 3);
            fprintf(out, "}");
            
            /* 如果不是最后一个catch，添加else if */
            if (i < try_stmt->catch_count - 1) {
                fprintf(out, " else ");
            } else {
                fprintf(out, " else {\n");
                /* 未匹配的异常重新抛出 */
                emit_indent(out, 4);
                fprintf(out, "cn_rethrow();\n");
                emit_indent(out, 3);
                fprintf(out, "}\n");
            }
        }
    } else {
        /* 没有catch块，直接重新抛出 */
        emit_indent(out, 3);
        fprintf(out, "cn_rethrow();\n");
    }
    
    emit_indent(out, 2);
    fprintf(out, "}\n");
    
    /* 生成finally块 */
    if (try_stmt->finally_block) {
        emit_indent(out, 2);
        fprintf(out, "// finally block\n");
        if (emit_block_func) {
            emit_block_func(ctx, try_stmt->finally_block, user_data);
        }
    }
    
    /* 结束异常处理块 */
    emit_indent(out, 1);
    fprintf(out, "}\n");
}

/* ============================================================================
 * throw 代码生成
 * ============================================================================ */

/**
 * @brief 生成throw语句的C代码
 * 
 * CN代码：
 *   抛出 "异常类型" "消息";
 *   或
 *   抛出 异常表达式;
 * 
 * 生成的C代码：
 *   cn_throw_simple("异常类型", "消息", __FILE__, __LINE__);
 *   或
 *   cn_throw(exception_expr, __FILE__, __LINE__);
 * 
 * @param ctx 代码生成上下文
 * @param stmt throw语句AST节点
 * @param emit_expr_func 生成表达式的函数指针
 * @param user_data 用户数据
 */
void cn_cgen_throw_stmt(CnCCodeGenContext *ctx, CnAstStmt *stmt,
                         void (*emit_expr_func)(CnCCodeGenContext *, CnAstExpr *, void *),
                         void *user_data)
{
    if (!ctx || !stmt || stmt->kind != CN_AST_STMT_THROW) {
        return;
    }

    CnAstThrowStmt *throw_stmt = &stmt->as.throw_stmt;
    FILE *out = ctx->output_file;

    emit_indent(out, 1);
    
    if (throw_stmt->exception_expr) {
        /* 表达式形式：抛出 表达式 */
        fprintf(out, "{\n");
        emit_indent(out, 2);
        fprintf(out, "CnException __ex;\n");
        emit_indent(out, 2);
        fprintf(out, "cn_exception_init(&__ex, ");
        
        /* 生成异常表达式 */
        if (emit_expr_func) {
            emit_expr_func(ctx, throw_stmt->exception_expr, user_data);
        }
        
        fprintf(out, ", NULL);\n");
        emit_indent(out, 2);
        fprintf(out, "cn_throw(&__ex, __FILE__, __LINE__);\n");
        emit_indent(out, 1);
        fprintf(out, "}\n");
    } else if (throw_stmt->exception_type) {
        /* 简单形式：抛出 "类型" "消息" */
        fprintf(out, "cn_throw_simple(\"%.*s\", ",
                (int)throw_stmt->exception_type_length,
                throw_stmt->exception_type);
        
        if (throw_stmt->message) {
            fprintf(out, "\"%.*s\"",
                    (int)throw_stmt->message_length,
                    throw_stmt->message);
        } else {
            fprintf(out, "NULL");
        }
        
        fprintf(out, ", __FILE__, __LINE__);\n");
    } else {
        /* 重新抛出当前异常 */
        fprintf(out, "cn_rethrow();\n");
    }
}

/* ============================================================================
 * 辅助代码生成
 * ============================================================================ */

/**
 * @brief 生成异常处理运行时头文件包含
 * 
 * @param out 输出文件
 */
void cn_cgen_exception_includes(FILE *out)
{
    fprintf(out, "#include \"cnlang/runtime/exception.h\"\n");
}

/**
 * @brief 生成异常处理运行时初始化代码
 * 
 * @param out 输出文件
 */
void cn_cgen_exception_runtime_init(FILE *out)
{
    /* 异常处理运行时不需要全局初始化 */
    /* 异常栈使用线程局部存储，自动初始化 */
}

/* ============================================================================
 * 完整语句生成接口
 * ============================================================================ */

/**
 * @brief 生成异常处理语句的C代码
 * 
 * 根据语句类型调用相应的生成函数
 * 
 * @param ctx 代码生成上下文
 * @param stmt 语句AST节点
 * @param emit_block_func 生成语句块的函数指针
 * @param emit_expr_func 生成表达式的函数指针
 * @param user_data 用户数据
 */
void cn_cgen_exception_stmt(CnCCodeGenContext *ctx, CnAstStmt *stmt,
                             void (*emit_block_func)(CnCCodeGenContext *, CnAstBlockStmt *, void *),
                             void (*emit_expr_func)(CnCCodeGenContext *, CnAstExpr *, void *),
                             void *user_data)
{
    if (!ctx || !stmt) {
        return;
    }

    switch (stmt->kind) {
        case CN_AST_STMT_TRY:
            cn_cgen_try_stmt(ctx, stmt, emit_block_func, user_data);
            break;
            
        case CN_AST_STMT_THROW:
            cn_cgen_throw_stmt(ctx, stmt, emit_expr_func, user_data);
            break;
            
        default:
            /* 不是异常处理语句 */
            break;
    }
}
