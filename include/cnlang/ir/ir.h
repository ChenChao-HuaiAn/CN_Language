#ifndef CN_IR_IR_H
#define CN_IR_IR_H

#include "cnlang/frontend/semantics.h"
#include "cnlang/support/config.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// IR 指令种类
typedef enum CnIrInstKind {
    // 算术与逻辑指令
    CN_IR_INST_ADD,
    CN_IR_INST_SUB,
    CN_IR_INST_MUL,
    CN_IR_INST_DIV,
    CN_IR_INST_MOD,
    CN_IR_INST_AND,
    CN_IR_INST_OR,
    CN_IR_INST_XOR,
    CN_IR_INST_SHL,
    CN_IR_INST_SHR,
    CN_IR_INST_NEG,
    CN_IR_INST_NOT,

    // 比较指令
    CN_IR_INST_EQ,
    CN_IR_INST_NE,
    CN_IR_INST_LT,
    CN_IR_INST_LE,
    CN_IR_INST_GT,
    CN_IR_INST_GE,

    // 内存操作指令
    CN_IR_INST_ALLOCA, // 在栈上分配空间
    CN_IR_INST_LOAD,   // 从地址加载到寄存器
    CN_IR_INST_STORE,  // 从寄存器/立即数存入地址
    CN_IR_INST_MOV,    // 寄存器间移动或加载立即数
    CN_IR_INST_ADDRESS_OF, // 取地址运算符 &
    CN_IR_INST_DEREF,  // 解引用运算符 *

    // 控制流指令
    CN_IR_INST_LABEL,  // 标签指令（通常作为跳转目标）
    CN_IR_INST_JUMP,   // 无条件跳转
    CN_IR_INST_BRANCH, // 条件跳转
    CN_IR_INST_CALL,   // 函数调用
    CN_IR_INST_RET,    // 函数返回

    // 结构体操作指令
    CN_IR_INST_GET_ELEMENT_PTR,  // 获取结构体成员地址
    CN_IR_INST_MEMBER_ACCESS,    // 结构体成员访问

    // 其他
    CN_IR_INST_PHI,    // SSA 形式下的 PHI 指令（预留）
    CN_IR_INST_SELECT  // 选择指令：dest = condition ? true_val : false_val (三元运算符)
} CnIrInstKind;

// IR 操作数种类
typedef enum CnIrOperandKind {
    CN_IR_OP_NONE,     // 无操作数
    CN_IR_OP_REG,      // 虚拟寄存器 (reg_id)
    CN_IR_OP_IMM_INT,  // 整数立即数
    CN_IR_OP_IMM_FLOAT,// 浮点数立即数
    CN_IR_OP_IMM_STR,  // 字符串字面量
    CN_IR_OP_SYMBOL,   // 全局符号（变量名、函数名）
    CN_IR_OP_LABEL,    // 跳转目标（指向 BasicBlock）
    CN_IR_OP_AST_EXPR  // AST表达式（用于复杂字面量如结构体）
} CnIrOperandKind;

// IR 操作数结构
typedef struct CnIrOperand {
    CnIrOperandKind kind;
    union {
        int reg_id;
        long long imm_int;
        double imm_float;
        const char *imm_str;
        const char *sym_name;
        struct CnIrBasicBlock *label;
        struct CnAstExpr *ast_expr;  // 用于结构体字面量等复杂表达式
    } as;
    CnType *type; // 与操作数关联的类型信息
} CnIrOperand;

// IR 指令结构
typedef struct CnIrInst {
    CnIrInstKind kind;
    CnIrOperand dest;  // 目标操作数
    CnIrOperand src1;  // 源操作数 1
    CnIrOperand src2;  // 源操作数 2

    // 针对 CALL 等指令的额外参数列表
    CnIrOperand *extra_args;
    size_t extra_args_count;

    struct CnIrInst *next;
    struct CnIrInst *prev;
} CnIrInst;

// 基本块列表（用于前驱/后继）
typedef struct CnIrBasicBlockList {
    struct CnIrBasicBlock *block;
    struct CnIrBasicBlockList *next;
} CnIrBasicBlockList;

// 基本块结构
typedef struct CnIrBasicBlock {
    const char *name;        // 基本块名称（调试用）
    CnIrInst *first_inst;
    CnIrInst *last_inst;
    
    CnIrBasicBlockList *preds; // 前驱块
    CnIrBasicBlockList *succs; // 后继块

    struct CnIrBasicBlock *next; // 线性链表中的下一个（函数内的物理顺序）
    struct CnIrBasicBlock *prev;
} CnIrBasicBlock;

// 静态局部变量结构
typedef struct CnIrStaticVar {
    const char *name;           // 变量名
    size_t name_length;         // 变量名长度
    CnType *type;               // 变量类型
    CnIrOperand initializer;    // 初始化值（可能是立即数或NONE）
    struct CnIrStaticVar *next; // 链表下一个
} CnIrStaticVar;

// 函数结构
typedef struct CnIrFunction {
    const char *name;
    CnType *return_type;
    
    CnIrOperand *params;     // 参数列表
    size_t param_count;
    
    CnIrOperand *locals;     // 局部变量（栈分配等）
    size_t local_count;

    // 静态局部变量列表（新增）
    CnIrStaticVar *first_static_var;  // 静态变量链表头
    CnIrStaticVar *last_static_var;   // 静态变量链表尾

    CnIrBasicBlock *first_block;
    CnIrBasicBlock *last_block;
    int next_reg_id;         // 用于生成新的虚拟寄存器 ID
    
    // 中断处理相关
    int is_interrupt_handler;  // 是否是中断服务程序
    uint32_t interrupt_vector; // 中断向量号（仅is_interrupt_handler=1时有效）
    
    struct CnIrFunction *next;
} CnIrFunction;

// 全局变量结构
typedef struct CnIrGlobalVar {
    const char *name;
    CnType *type;
    CnIrOperand initializer;  // 初始化值（可能是立即数或符号）
    int is_const;             // 是否为常量
    struct CnIrGlobalVar *next;
} CnIrGlobalVar;

// IR 模块（一个编译单元）
typedef struct CnIrModule {
    CnIrFunction *first_func;
    CnIrFunction *last_func;
    CnIrGlobalVar *first_global;  // 全局变量链表
    CnIrGlobalVar *last_global;
    CnTargetTriple target;      // 目标三元组信息，用于后端映射和数据布局
    CnCompileMode compile_mode; // 编译模式：宿主 / freestanding
} CnIrModule;

// IR 管理接口
CnIrModule *cn_ir_module_new();
void cn_ir_module_free(CnIrModule *module);

CnIrFunction *cn_ir_function_new(const char *name, CnType *return_type);
void cn_ir_function_add_param(CnIrFunction *func, CnIrOperand param);
void cn_ir_function_add_local(CnIrFunction *func, CnIrOperand local);
void cn_ir_function_add_block(CnIrFunction *func, CnIrBasicBlock *block);
void cn_ir_function_add_static_var(CnIrFunction *func, CnIrStaticVar *static_var);  // 新增：添加静态变量

CnIrBasicBlock *cn_ir_basic_block_new(const char *name_hint);
void cn_ir_basic_block_add_inst(CnIrBasicBlock *block, CnIrInst *inst);
void cn_ir_basic_block_connect(CnIrBasicBlock *from, CnIrBasicBlock *to);

CnIrInst *cn_ir_inst_new(CnIrInstKind kind, CnIrOperand dest, CnIrOperand src1, CnIrOperand src2);

// IR 打印工具
void cn_ir_dump_module(CnIrModule *module);
void cn_ir_dump_module_to_file(CnIrModule *module, FILE *file);
void cn_ir_dump_function(CnIrFunction *func);
void cn_ir_dump_function_to_file(CnIrFunction *func, FILE *file);
void cn_ir_dump_block(CnIrBasicBlock *block);
void cn_ir_dump_block_to_file(CnIrBasicBlock *block, FILE *file);
void cn_ir_dump_inst(CnIrInst *inst);
void cn_ir_dump_inst_to_file(CnIrInst *inst, FILE *file);
void cn_ir_dump_operand(CnIrOperand op);
void cn_ir_dump_operand_to_file(CnIrOperand op, FILE *file);

// 辅助工具：创建操作数
CnIrOperand cn_ir_op_none();
CnIrOperand cn_ir_op_reg(int reg_id, CnType *type);
CnIrOperand cn_ir_op_imm_int(long long val, CnType *type);
CnIrOperand cn_ir_op_imm_float(double val, CnType *type);
CnIrOperand cn_ir_op_imm_str(const char *val, CnType *type);  // 新增：创建字符串立即数
CnIrOperand cn_ir_op_label(CnIrBasicBlock *block);
CnIrOperand cn_ir_op_symbol(const char *name, CnType *type);
CnIrOperand cn_ir_op_ast_expr(struct CnAstExpr *expr, CnType *type);

#ifdef __cplusplus
}
#endif

#endif /* CN_IR_IR_H */
