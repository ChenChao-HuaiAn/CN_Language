#ifndef CN_FRONTEND_AST_H
#define CN_FRONTEND_AST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// AST 节点种类（表达式）
typedef enum CnAstExprKind {
    CN_AST_EXPR_BINARY,
    CN_AST_EXPR_CALL,
    CN_AST_EXPR_IDENTIFIER,
    CN_AST_EXPR_INTEGER_LITERAL,
    CN_AST_EXPR_STRING_LITERAL,
    CN_AST_EXPR_ASSIGN,
    CN_AST_EXPR_LOGICAL,
    CN_AST_EXPR_UNARY,
    CN_AST_EXPR_ARRAY_LITERAL,  // 数组字面量
    CN_AST_EXPR_INDEX            // 数组索引访问 arr[index]
} CnAstExprKind;

// AST 节点种类（语句）
typedef enum CnAstStmtKind {
    CN_AST_STMT_BLOCK,
    CN_AST_STMT_VAR_DECL,
    CN_AST_STMT_EXPR,
    CN_AST_STMT_RETURN,
    CN_AST_STMT_IF,
    CN_AST_STMT_WHILE,
    CN_AST_STMT_FOR,
    CN_AST_STMT_BREAK,
    CN_AST_STMT_CONTINUE
} CnAstStmtKind;

// 二元运算符
typedef enum CnAstBinaryOp {
    CN_AST_BINARY_OP_ADD,
    CN_AST_BINARY_OP_SUB,
    CN_AST_BINARY_OP_MUL,
    CN_AST_BINARY_OP_DIV,
    CN_AST_BINARY_OP_EQ,
    CN_AST_BINARY_OP_NE,
    CN_AST_BINARY_OP_LT,
    CN_AST_BINARY_OP_GT,
    CN_AST_BINARY_OP_LE,
    CN_AST_BINARY_OP_GE
} CnAstBinaryOp;

// 逻辑运算符
typedef enum CnAstLogicalOp {
    CN_AST_LOGICAL_OP_AND,  // &&
    CN_AST_LOGICAL_OP_OR    // ||
} CnAstLogicalOp;

// 一元运算符
typedef enum CnAstUnaryOp {
    CN_AST_UNARY_OP_NOT,    // !
    CN_AST_UNARY_OP_MINUS   // - (负号，预留)
} CnAstUnaryOp;

struct CnType;
struct CnAstExpr;
struct CnAstStmt;
struct CnAstBlockStmt;

// 变量声明
typedef struct CnAstVarDecl {
    const char *name;
    size_t name_length;
    struct CnType *declared_type;  // 显式声明的类型，如果是“变量”则为 NULL 或特定类型
    struct CnAstExpr *initializer; // 可以为 NULL
} CnAstVarDecl;

// 表达式语句
typedef struct CnAstExprStmt {
    struct CnAstExpr *expr;
} CnAstExprStmt;

// 语句块
typedef struct CnAstBlockStmt {
    size_t stmt_count;
    struct CnAstStmt **stmts;
} CnAstBlockStmt;

// 返回语句
typedef struct CnAstReturnStmt {
    struct CnAstExpr *expr; // 可以为 NULL
} CnAstReturnStmt;

// 条件语句 if
typedef struct CnAstIfStmt {
    struct CnAstExpr *condition;
    CnAstBlockStmt *then_block;
    CnAstBlockStmt *else_block; // 可以为 NULL
} CnAstIfStmt;

// while 循环语句
typedef struct CnAstWhileStmt {
    struct CnAstExpr *condition;
    CnAstBlockStmt *body;
} CnAstWhileStmt;

// for 循环语句
typedef struct CnAstForStmt {
    struct CnAstStmt *init;      // 可以为 NULL
    struct CnAstExpr *condition; // 可以为 NULL
    struct CnAstExpr *update;    // 可以为 NULL
    CnAstBlockStmt *body;
} CnAstForStmt;

// 函数参数
typedef struct CnAstParameter {
    const char *name;
    size_t name_length;
    struct CnType *declared_type; // 参数声明类型
} CnAstParameter;

// 函数声明
typedef struct CnAstFunctionDecl {
    const char *name;
    size_t name_length;
    CnAstParameter *parameters;   // 参数列表
    size_t parameter_count;       // 参数数量
    CnAstBlockStmt *body;         // 函数体语句块
} CnAstFunctionDecl;

// 程序根节点
typedef struct CnAstProgram {
    size_t function_count;
    CnAstFunctionDecl **functions;
} CnAstProgram;

// 各种表达式节点定义
typedef struct CnAstBinaryExpr {
    CnAstBinaryOp op;
    struct CnAstExpr *left;
    struct CnAstExpr *right;
} CnAstBinaryExpr;

typedef struct CnAstCallExpr {
    struct CnAstExpr *callee;
    struct CnAstExpr **arguments;
    size_t argument_count;
} CnAstCallExpr;

typedef struct CnAstIdentifierExpr {
    const char *name;
    size_t name_length;
} CnAstIdentifierExpr;

typedef struct CnAstIntegerLiteralExpr {
    long value;
} CnAstIntegerLiteralExpr;

// 字符串字面量
typedef struct CnAstStringLiteralExpr {
    const char *value;
    size_t length;
} CnAstStringLiteralExpr;

// 赋值表达式
typedef struct CnAstAssignExpr {
    struct CnAstExpr *target;  // 赋值目标（左值，如标识符）
    struct CnAstExpr *value;   // 赋值的值（右值）
} CnAstAssignExpr;

// 逻辑表达式
typedef struct CnAstLogicalExpr {
    CnAstLogicalOp op;
    struct CnAstExpr *left;
    struct CnAstExpr *right;
} CnAstLogicalExpr;

// 一元表达式
typedef struct CnAstUnaryExpr {
    CnAstUnaryOp op;
    struct CnAstExpr *operand;
} CnAstUnaryExpr;

// 数组字面量表达式
typedef struct CnAstArrayLiteralExpr {
    struct CnAstExpr **elements;  // 数组元素列表
    size_t element_count;         // 元素数量
} CnAstArrayLiteralExpr;

// 数组索引访问表达式 arr[index]
typedef struct CnAstIndexExpr {
    struct CnAstExpr *array;      // 数组表达式
    struct CnAstExpr *index;      // 索引表达式
} CnAstIndexExpr;

// 表达式统一节点
typedef struct CnAstExpr {
    CnAstExprKind kind;
    struct CnType *type; // 语义分析阶段填充的类型信息
    union {
        CnAstBinaryExpr binary;
        CnAstCallExpr call;
        CnAstIdentifierExpr identifier;
        CnAstIntegerLiteralExpr integer_literal;
        CnAstStringLiteralExpr string_literal;
        CnAstAssignExpr assign;
        CnAstLogicalExpr logical;
        CnAstUnaryExpr unary;
        CnAstArrayLiteralExpr array_literal;
        CnAstIndexExpr index;  // 数组索引访问
    } as;
} CnAstExpr;

// 语句统一节点
typedef struct CnAstStmt {
    CnAstStmtKind kind;
    union {
        CnAstBlockStmt *block;
        CnAstVarDecl var_decl;
        CnAstExprStmt expr;
        CnAstReturnStmt return_stmt;
        CnAstIfStmt if_stmt;
        CnAstWhileStmt while_stmt;
        CnAstForStmt for_stmt;
    } as;
} CnAstStmt;

// 内存管理接口，仅负责释放由解析器创建的 AST
void cn_frontend_ast_program_free(CnAstProgram *program);
void cn_frontend_ast_function_free(CnAstFunctionDecl *function_decl);
void cn_frontend_ast_block_free(CnAstBlockStmt *block);
void cn_frontend_ast_stmt_free(CnAstStmt *stmt);
void cn_frontend_ast_expr_free(CnAstExpr *expr);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_AST_H */
