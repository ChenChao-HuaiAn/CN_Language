#ifndef CN_IR_PASS_H
#define CN_IR_PASS_H

#include "cnlang/ir/ir.h"

#ifdef __cplusplus
extern "C" {
#endif

// IR 优化 Pass 的统一入口
typedef void (*CnIrPassFunc)(CnIrModule *module);

// 常量折叠优化：在基本块内部进行算术运算的提前计算
void cn_ir_pass_constant_folding(CnIrModule *module);

// 死代码删除优化：移除不可达的基本块或无副作用且结果未使用的指令
void cn_ir_pass_dead_code_elimination(CnIrModule *module);

// 公共子表达式消除：识别并消除重复的表达式计算
void cn_ir_pass_cse(CnIrModule *module);

// 复写传播：跟踪MOV指令的值等价关系，消除间接引用
void cn_ir_pass_copy_propagation(CnIrModule *module);

// 循环不变量外提：将循环内不变的计算移动到循环前执行
void cn_ir_pass_loop_invariant_code_motion(CnIrModule *module);

// 函数内联展开：将函数调用替换为被调用函数的函数体
void cn_ir_pass_inline(CnIrModule *module);

// 强度削弱：用低代价操作替代高代价操作（如乘法转移位）
void cn_ir_pass_strength_reduction(CnIrModule *module);

// 尾递归优化：将尾递归调用转换为循环，避免栈开销
void cn_ir_pass_tail_call_opt(CnIrModule *module);

// 运行所有基础优化 Pass
void cn_ir_run_default_passes(CnIrModule *module);

#ifdef __cplusplus
}
#endif

#endif /* CN_IR_PASS_H */
