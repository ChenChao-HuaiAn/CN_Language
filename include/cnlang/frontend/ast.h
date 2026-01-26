#ifndef CN_FRONTEND_AST_H
#define CN_FRONTEND_AST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// AST 节点种类（表达式）
typedef enum CnAstExprKind {
    CN_AST_EXPR_BINARY,     // 二元表达式
    CN_AST_EXPR_CALL,       // 函数调用表达式
    CN_AST_EXPR_IDENTIFIER,   // 标识符表达式
    CN_AST_EXPR_INTEGER_LITERAL, // 整数字面量表达式
    CN_AST_EXPR_FLOAT_LITERAL,   // 浮点数字面量表达式
    CN_AST_EXPR_STRING_LITERAL,  // 字符串字面量表达式
    CN_AST_EXPR_BOOL_LITERAL,    // 布尔字面量表达式
    CN_AST_EXPR_ASSIGN,     // 赋值表达式
    CN_AST_EXPR_LOGICAL,    // 逻辑表达式
    CN_AST_EXPR_UNARY,      // 一元表达式
    CN_AST_EXPR_ARRAY_LITERAL,  // 数组字面量
    CN_AST_EXPR_INDEX,          // 数组索引访问 arr[index]
    CN_AST_EXPR_MEMBER_ACCESS,  // 结构体成员访问 obj.member 或 ptr->member
    CN_AST_EXPR_STRUCT_LITERAL, // 结构体字面量
    CN_AST_EXPR_MEMORY_READ,    // 读取内存 read_memory(addr)
    CN_AST_EXPR_MEMORY_WRITE,   // 写入内存 write_memory(addr, value)
    CN_AST_EXPR_MEMORY_COPY,    // 内存复制 memory_copy(dest, src, size)
    CN_AST_EXPR_MEMORY_SET,     // 内存设置 memory_set(addr, value, size)
    CN_AST_EXPR_MEMORY_MAP,     // 内存映射 map_memory(addr, size, prot, flags)
    CN_AST_EXPR_MEMORY_UNMAP,   // 解除内存映射 unmap_memory(addr, size)
    CN_AST_EXPR_INLINE_ASM     // 内联汇编 inline_asm("code", outputs, inputs, clobbers)
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
    CN_AST_STMT_SWITCH,     // switch 选择语句
    CN_AST_STMT_STRUCT_DECL,// 结构体声明语句
    CN_AST_STMT_ENUM_DECL,  // 枚举声明语句
    CN_AST_STMT_MODULE_DECL,// 模块声明语句
    CN_AST_STMT_IMPORT      // 导入语句
} CnAstStmtKind;

// 二元运算符
typedef enum CnAstBinaryOp {
    CN_AST_BINARY_OP_ADD,     // +
    CN_AST_BINARY_OP_SUB,     // -
    CN_AST_BINARY_OP_MUL,     // *
    CN_AST_BINARY_OP_DIV,     // /
    CN_AST_BINARY_OP_MOD,     // % 取模
    CN_AST_BINARY_OP_EQ,      // ==
    CN_AST_BINARY_OP_NE,      // !=
    CN_AST_BINARY_OP_LT,      // <
    CN_AST_BINARY_OP_GT,      // >
    CN_AST_BINARY_OP_LE,      // <=
    CN_AST_BINARY_OP_GE,      // >=
    CN_AST_BINARY_OP_BITWISE_AND, // & 按位与
    CN_AST_BINARY_OP_BITWISE_OR,  // | 按位或
    CN_AST_BINARY_OP_BITWISE_XOR, // ^ 按位异或
    CN_AST_BINARY_OP_LEFT_SHIFT,  // << 左移
    CN_AST_BINARY_OP_RIGHT_SHIFT // >> 右移
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
    CN_AST_UNARY_OP_DEREFERENCE, // 解引用 *
    CN_AST_UNARY_OP_BITWISE_NOT  // ~ 按位取反
} CnAstUnaryOp;

struct CnType;
struct CnAstExpr;
struct CnAstStmt;
struct CnAstBlockStmt;

// 可见性枚举
typedef enum CnVisibility {
    CN_VISIBILITY_DEFAULT,  // 默认可见性（用于非模块成员）
    CN_VISIBILITY_PUBLIC,   // 公开
    CN_VISIBILITY_PRIVATE   // 私有
} CnVisibility;

// 变量声明
typedef struct CnAstVarDecl {
    const char *name;
    size_t name_length;
    struct CnType *declared_type;  // 显式声明的类型，如果是"变量"则为 NULL 或特定类型
    struct CnAstExpr *initializer; // 可以为 NULL
    CnVisibility visibility;       // 可见性（用于模块成员）
    int is_const;                  // 是否为常量声明（使用“常量”关键字）
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

// switch 语句的单个 case 分支
typedef struct CnAstSwitchCase {
    struct CnAstExpr *value;     // case 的常量表达式，NULL 表示 default 分支
    CnAstBlockStmt *body;        // case 分支的语句块
} CnAstSwitchCase;

// switch 选择语句
typedef struct CnAstSwitchStmt {
    struct CnAstExpr *expr;      // switch 的判断表达式
    CnAstSwitchCase *cases;      // case 分支数组
    size_t case_count;           // case 分支数量（包括 default）
} CnAstSwitchStmt;

// 结构体字段声明
typedef struct CnAstStructField {
    const char *name;             // 字段名称
    size_t name_length;           // 字段名称长度
    struct CnType *field_type;    // 字段类型
    int is_const;                 // 是否为常量字段（使用"常量"关键字）
} CnAstStructField;

// 结构体声明语句
typedef struct CnAstStructDecl {
    const char *name;             // 结构体名称
    size_t name_length;           // 结构体名称长度
    CnAstStructField *fields;     // 字段列表
    size_t field_count;           // 字段数量
} CnAstStructDecl;

// 枚举成员
typedef struct CnAstEnumMember {
    const char *name;             // 枚举成员名称
    size_t name_length;           // 枚举成员名称长度
    int has_value;                // 是否有显式赋值
    long value;                   // 枚举成员的值（如果有显式赋值）
} CnAstEnumMember;

// 枚举声明语句
typedef struct CnAstEnumDecl {
    const char *name;             // 枚举类型名称
    size_t name_length;           // 枚举类型名称长度
    CnAstEnumMember *members;     // 枚举成员列表
    size_t member_count;          // 枚举成员数量
} CnAstEnumDecl;

// 模块声明语句
typedef struct CnAstModuleDecl {
    const char *name;             // 模块名称
    size_t name_length;           // 模块名称长度
    struct CnAstStmt **stmts;     // 模块内的语句列表（变量等）
    size_t stmt_count;            // 语句数量
    struct CnAstFunctionDecl **functions; // 模块内的函数声明列表
    size_t function_count;        // 函数数量
} CnAstModuleDecl;

// 导入成员项（用于选择性导入）
typedef struct CnAstImportMember {
    const char *name;             // 成员名称
    size_t name_length;           // 成员名称长度
} CnAstImportMember;

// 导入语句
typedef struct CnAstImportStmt {
    const char *module_name;      // 被导入的模块名称
    size_t module_name_length;    // 模块名称长度
    const char *alias;            // 模块别名（NULL表示不使用别名）
    size_t alias_length;          // 别名长度
    CnAstImportMember *members;   // 要导入的成员列表（NULL表示导入所有）
    size_t member_count;          // 成员数量（0表示导入所有）
} CnAstImportStmt;

// 函数参数
typedef struct CnAstParameter {
    const char *name;
    size_t name_length;
    struct CnType *declared_type; // 参数声明类型
    int is_const;                 // 是否为常量参数（使用"常量"关键字）
} CnAstParameter;

// 函数声明
typedef struct CnAstFunctionDecl {
    const char *name;
    size_t name_length;
    CnAstParameter *parameters;   // 参数列表
    size_t parameter_count;       // 参数数量
    CnAstBlockStmt *body;         // 函数体语句块
    int is_interrupt_handler;     // 是否是中断服务程序
    uint32_t interrupt_vector;    // 中断向量号（仅中断处理有效）
} CnAstFunctionDecl;

// 程序根节点
typedef struct CnAstProgram {
    size_t function_count;
    CnAstFunctionDecl **functions;
    size_t struct_count;          // 结构体数量
    struct CnAstStmt **structs;   // 结构体声明列表
    size_t enum_count;            // 枚举数量
    struct CnAstStmt **enums;     // 枚举声明列表
    size_t module_count;          // 模块数量
    struct CnAstStmt **modules;   // 模块声明列表
    size_t import_count;          // 导入语句数量
    struct CnAstStmt **imports;   // 导入语句列表
    size_t global_var_count;      // 全局变量数量
    struct CnAstStmt **global_vars; // 全局变量声明列表
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

// 浮点数字面量
typedef struct CnAstFloatLiteralExpr {
    double value;
} CnAstFloatLiteralExpr;

// 字符串字面量
typedef struct CnAstStringLiteralExpr {
    const char *value;
    size_t length;
} CnAstStringLiteralExpr;

// 布尔字面量
typedef struct CnAstBoolLiteralExpr {
    int value; // 0 为 false, 1 为 true
} CnAstBoolLiteralExpr;

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

// 内存读取表达式 read_memory(addr)
typedef struct CnAstMemoryReadExpr {
    struct CnAstExpr *address;    // 内存地址
} CnAstMemoryReadExpr;

// 内存写入表达式 write_memory(addr, value)
typedef struct CnAstMemoryWriteExpr {
    struct CnAstExpr *address;    // 内存地址
    struct CnAstExpr *value;      // 要写入的值
} CnAstMemoryWriteExpr;

// 内存复制表达式 memory_copy(dest, src, size)
typedef struct CnAstMemoryCopyExpr {
    struct CnAstExpr *dest;       // 目标地址
    struct CnAstExpr *src;        // 源地址
    struct CnAstExpr *size;       // 复制大小
} CnAstMemoryCopyExpr;

// 内存设置表达式 memory_set(addr, value, size)
typedef struct CnAstMemorySetExpr {
    struct CnAstExpr *address;    // 内存地址
    struct CnAstExpr *value;      // 设置值
    struct CnAstExpr *size;       // 设置大小
} CnAstMemorySetExpr;

// 内存映射表达式 map_memory(addr, size, prot, flags)
typedef struct CnAstMemoryMapExpr {
    struct CnAstExpr *address;    // 映射地址（可为NULL表示自动分配）
    struct CnAstExpr *size;       // 映射大小
    struct CnAstExpr *prot;       // 保护标志
    struct CnAstExpr *flags;      // 映射标志
} CnAstMemoryMapExpr;

// 解除内存映射表达式 unmap_memory(addr, size)
typedef struct CnAstMemoryUnmapExpr {
    struct CnAstExpr *address;    // 映射地址
    struct CnAstExpr *size;       // 映射大小
} CnAstMemoryUnmapExpr;

// 内联汇编表达式 inline_asm("code", outputs, inputs, clobbers)
typedef struct CnAstInlineAsmExpr {
    struct CnAstExpr *asm_code;      // 汇编代码字符串
    struct CnAstExpr **outputs;      // 输出变量列表
    size_t output_count;              // 输出变量数量
    struct CnAstExpr **inputs;       // 输入变量列表
    size_t input_count;               // 输入变量数量
    struct CnAstExpr *clobbers;     // 破坏列表（字符串字面量或标识符列表）
} CnAstInlineAsmExpr;

// 表达式统一节点
typedef struct CnAstExpr {
    CnAstExprKind kind;
    struct CnType *type; // 语义分析阶段填充的类型信息
    union {
        CnAstBinaryExpr binary;
        CnAstCallExpr call;
        CnAstIdentifierExpr identifier;
        CnAstIntegerLiteralExpr integer_literal;
        CnAstFloatLiteralExpr float_literal;
        CnAstStringLiteralExpr string_literal;
        CnAstBoolLiteralExpr bool_literal;
        CnAstAssignExpr assign;
        CnAstLogicalExpr logical;
        CnAstUnaryExpr unary;
        CnAstArrayLiteralExpr array_literal;
        CnAstIndexExpr index;              // 数组索引访问
        CnAstMemberAccessExpr member;      // 结构体成员访问
        CnAstStructLiteralExpr struct_lit; // 结构体字面量
        CnAstMemoryReadExpr memory_read;   // 内存读取
        CnAstMemoryWriteExpr memory_write; // 内存写入
        CnAstMemoryCopyExpr memory_copy;   // 内存复制
        CnAstMemorySetExpr memory_set;     // 内存设置
        CnAstMemoryMapExpr memory_map;     // 内存映射
        CnAstMemoryUnmapExpr memory_unmap; // 解除内存映射
        CnAstInlineAsmExpr inline_asm;     // 内联汇编
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
        CnAstSwitchStmt switch_stmt; // switch 选择语句
        CnAstStructDecl struct_decl; // 结构体声明
        CnAstEnumDecl enum_decl;     // 枚举声明
        CnAstModuleDecl module_decl; // 模块声明
        CnAstImportStmt import_stmt; // 导入语句
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
