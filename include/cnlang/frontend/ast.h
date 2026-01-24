#ifndef CN_FRONTEND_AST_H
#define CN_FRONTEND_AST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// AST 节点种类（表达式）
typedef enum CnAstExprKind {
    CN_AST_EXPR_BINARY,     // 二元表达式
    CN_AST_EXPR_CALL,       // 函数调用表达式
    CN_AST_EXPR_IDENTIFIER,   // 标识符表达式
    CN_AST_EXPR_INTEGER_LITERAL, // 整数字面量表达式
    CN_AST_EXPR_STRING_LITERAL,  // 字符串字面量表达式
    CN_AST_EXPR_ASSIGN,     // 赋值表达式
    CN_AST_EXPR_LOGICAL,    // 逻辑表达式
    CN_AST_EXPR_UNARY,      // 一元表达式
    CN_AST_EXPR_ARRAY_LITERAL,  // 数组字面量
    CN_AST_EXPR_INDEX,          // 数组索引访问 arr[index]
    CN_AST_EXPR_MEMBER_ACCESS,  // 结构体成员访问 obj.member 或 ptr->member
    CN_AST_EXPR_STRUCT_LITERAL  // 结构体字面量
} CnAstExprKind;

// AST 节点种类（语句）
typedef enum CnAstStmtKind {
    CN_AST_STMT_BLOCK,      // 语句块
    CN_AST_STMT_VAR_DECL,   // 变量声明语句
    CN_AST_STMT_EXPR,       // 表达式语句   
    CN_AST_STMT_RETURN,     // 返回语句
    CN_AST_STMT_IF,         // 条件语句 if
    CN_AST_STMT_WHILE,      // while 循环语句
    CN_AST_STMT_FOR,        // for 循环语句
    CN_AST_STMT_BREAK,      // break 语句
    CN_AST_STMT_CONTINUE,   // continue 语句
    CN_AST_STMT_STRUCT_DECL // 结构体声明语句
} CnAstStmtKind;

// 二元运算符
typedef enum CnAstBinaryOp {
    CN_AST_BINARY_OP_ADD,     // +
    CN_AST_BINARY_OP_SUB,     // -
    CN_AST_BINARY_OP_MUL,     // *
    CN_AST_BINARY_OP_DIV,     // /
    CN_AST_BINARY_OP_MOD,   // % 取模
    CN_AST_BINARY_OP_EQ,     // ==
    CN_AST_BINARY_OP_NE,     // !=
    CN_AST_BINARY_OP_LT,     // <
    CN_AST_BINARY_OP_GT,     // >
    CN_AST_BINARY_OP_LE,     // <=
    CN_AST_BINARY_OP_GE      // >=
} CnAstBinaryOp;

// 逻辑运算符
typedef enum CnAstLogicalOp {
    CN_AST_LOGICAL_OP_AND,  // &&
    CN_AST_LOGICAL_OP_OR    // ||
} CnAstLogicalOp;

// 一元运算符
typedef enum CnAstUnaryOp {
    CN_AST_UNARY_OP_NOT,         // !
    CN_AST_UNARY_OP_MINUS,       // - (数值取负)
    CN_AST_UNARY_OP_ADDRESS_OF,  // 取地址 &
    CN_AST_UNARY_OP_DEREFERENCE  // 解引用 *
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

// 结构体字段声明
typedef struct CnAstStructField {
    const char *name;             // 字段名称
    size_t name_length;           // 字段名称长度
    struct CnType *field_type;    // 字段类型
} CnAstStructField;

// 结构体声明语句
typedef struct CnAstStructDecl {
    const char *name;             // 结构体名称
    size_t name_length;           // 结构体名称长度
    CnAstStructField *fields;     // 字段列表
    size_t field_count;           // 字段数量
} CnAstStructDecl;

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
    size_t struct_count;          // 结构体数量
    struct CnAstStmt **structs;   // 结构体声明列表
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

// 结构体成员访问表达式 obj.member 或 ptr->member
typedef struct CnAstMemberAccessExpr {
    struct CnAstExpr *object;     // 对象表达式
    const char *member_name;      // 成员名称
    size_t member_name_length;    // 成员名称长度
    int is_arrow;                 // 是否是箭头访问（-> vs .）
} CnAstMemberAccessExpr;

// 结构体字段初始化器
typedef struct CnAstStructFieldInit {
    const char *field_name;       // 字段名称
    size_t field_name_length;     // 字段名称长度
    struct CnAstExpr *value;      // 字段值表达式
} CnAstStructFieldInit;

// 结构体字面量表达式
typedef struct CnAstStructLiteralExpr {
    const char *struct_name;      // 结构体类型名称
    size_t struct_name_length;    // 结构体类型名称长度
    CnAstStructFieldInit *fields; // 字段初始化列表
    size_t field_count;           // 字段数量
} CnAstStructLiteralExpr;

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
        CnAstIndexExpr index;              // 数组索引访问
        CnAstMemberAccessExpr member;      // 结构体成员访问
        CnAstStructLiteralExpr struct_lit; // 结构体字面量
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
        CnAstStructDecl struct_decl; // 结构体声明
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
