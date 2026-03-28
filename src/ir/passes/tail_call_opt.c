/**
 * @file tail_call_opt.c
 * @brief 尾递归优化Pass
 * 
 * 将尾递归调用转换为循环，避免递归调用的栈开销：
 * - 识别尾递归：函数在返回前直接调用自身
 * - 转换为循环：在函数入口添加循环标签，用参数更新替换递归调用
 * 
 * 转换示例：
 * 原始尾递归：
 *   func factorial(n, acc) {
 *     if (n <= 1) return acc;
 *     return factorial(n - 1, n * acc);
 *   }
 * 
 * 转换后：
 *   func factorial(n, acc) {
 *   loop_start:
 *     if (n <= 1) return acc;
 *     n = n - 1;
 *     acc = n * acc;
 *     goto loop_start;
 *   }
 */

#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// ============================================================================
// 尾调用信息结构
// ============================================================================

/**
 * @brief 尾调用点信息
 * 记录一个尾递归调用的位置和相关信息
 */
typedef struct TailCallSite {
    CnIrBasicBlock *block;      // 包含尾调用的基本块
    CnIrInst *call_inst;        // CALL指令
    CnIrInst *ret_inst;         // 紧随其后的RET指令
    struct TailCallSite *next;  // 链表下一个
} TailCallSite;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查指令是否是调用指定函数
 * @param inst 指令指针
 * @param func_name 函数名
 * @return true 如果是调用该函数
 */
static bool is_call_to_function(CnIrInst *inst, const char *func_name) {
    if (!inst || inst->kind != CN_IR_INST_CALL) return false;
    if (!func_name) return false;
    
    // src1 存储被调用函数的符号名
    if (inst->src1.kind == CN_IR_OP_SYMBOL) {
        return strcmp(inst->src1.as.sym_name, func_name) == 0;
    }
    
    return false;
}

/**
 * @brief 检查CALL指令后是否紧跟RET指令（允许中间有MOV指令）
 * @param call_inst CALL指令
 * @return 如果找到RET指令返回其指针，否则返回NULL
 */
static CnIrInst *find_following_ret(CnIrInst *call_inst) {
    if (!call_inst) return NULL;
    
    CnIrInst *inst = call_inst->next;
    
    // 跳过可能的MOV指令（用于传递返回值）
    while (inst) {
        if (inst->kind == CN_IR_INST_RET) {
            return inst;
        }
        // 如果遇到其他类型的指令，不是尾调用
        if (inst->kind != CN_IR_INST_MOV) {
            break;
        }
        inst = inst->next;
    }
    
    return NULL;
}

/**
 * @brief 检查CALL指令的结果是否只用于紧随的RET指令
 * @param call_inst CALL指令
 * @param ret_inst RET指令
 * @return true 如果结果只用于RET
 */
static bool is_call_result_used_only_for_ret(CnIrInst *call_inst, CnIrInst *ret_inst) {
    if (!call_inst || !ret_inst) return false;
    
    // 如果CALL没有目标寄存器（void函数），直接返回true
    if (call_inst->dest.kind != CN_IR_OP_REG) {
        return true;
    }
    
    int result_reg = call_inst->dest.as.reg_id;
    
    // 检查CALL和RET之间的指令是否使用了结果寄存器
    CnIrInst *inst = call_inst->next;
    while (inst && inst != ret_inst) {
        // 检查src1
        if (inst->src1.kind == CN_IR_OP_REG && inst->src1.as.reg_id == result_reg) {
            // 如果是MOV指令将结果传递给RET，这是允许的
            if (inst->kind == CN_IR_INST_MOV) {
                // 继续检查后续指令
            } else {
                return false;
            }
        }
        // 检查src2
        if (inst->src2.kind == CN_IR_OP_REG && inst->src2.as.reg_id == result_reg) {
            return false;
        }
        inst = inst->next;
    }
    
    // 检查RET指令是否使用了结果寄存器
    if (ret_inst->src1.kind == CN_IR_OP_REG) {
        // RET直接使用CALL的结果寄存器，或者通过MOV传递
        return true;
    }
    
    return true;
}

/**
 * @brief 收集函数中的所有尾递归调用点
 * @param func 函数指针
 * @return 尾调用点链表，如果没有则返回NULL
 */
static TailCallSite *collect_tail_call_sites(CnIrFunction *func) {
    if (!func || !func->name) return NULL;
    
    TailCallSite *sites = NULL;
    TailCallSite *last_site = NULL;
    
    // 遍历所有基本块
    for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
        // 遍历块中的所有指令
        for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
            // 检查是否是调用自身的CALL指令
            if (is_call_to_function(inst, func->name)) {
                // 检查是否是尾调用（CALL后紧跟RET）
                CnIrInst *ret_inst = find_following_ret(inst);
                
                if (ret_inst) {
                    // 检查CALL结果是否只用于RET
                    if (is_call_result_used_only_for_ret(inst, ret_inst)) {
                        // 创建尾调用点记录
                        TailCallSite *site = malloc(sizeof(TailCallSite));
                        site->block = block;
                        site->call_inst = inst;
                        site->ret_inst = ret_inst;
                        site->next = NULL;
                        
                        // 添加到链表
                        if (!sites) {
                            sites = site;
                        } else {
                            last_site->next = site;
                        }
                        last_site = site;
                    }
                }
            }
        }
    }
    
    return sites;
}

/**
 * @brief 释放尾调用点链表
 * @param sites 尾调用点链表
 */
static void free_tail_call_sites(TailCallSite *sites) {
    while (sites) {
        TailCallSite *next = sites->next;
        free(sites);
        sites = next;
    }
}

/**
 * @brief 创建空操作数
 * @return 空操作数结构
 */
static CnIrOperand make_none_operand(void) {
    CnIrOperand op;
    op.kind = CN_IR_OP_NONE;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建标签操作数
 * @param block 目标基本块
 * @return 标签操作数
 */
static CnIrOperand make_label_operand(CnIrBasicBlock *block) {
    CnIrOperand op;
    op.kind = CN_IR_OP_LABEL;
    op.as.label = block;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建寄存器操作数
 * @param reg_id 寄存器ID
 * @param type 类型信息
 * @return 寄存器操作数
 */
static CnIrOperand make_reg_operand(int reg_id, CnType *type) {
    CnIrOperand op;
    op.kind = CN_IR_OP_REG;
    op.as.reg_id = reg_id;
    op.type = type;
    return op;
}

// ============================================================================
// 尾递归转换函数
// ============================================================================

/**
 * @brief 在尾调用点之前插入参数更新指令
 * 
 * 将尾递归调用的实参赋值给函数的形参：
 *   param[i] = call->extra_args[i]
 * 
 * 注意：需要处理参数依赖关系，使用临时变量避免覆盖
 * 
 * @param func 函数指针
 * @param site 尾调用点
 * @param temp_regs 临时寄存器数组（用于处理依赖）
 * @return 成功插入的指令数量
 */
static int insert_param_updates(CnIrFunction *func, TailCallSite *site, int *temp_regs) {
    if (!func || !site) return 0;
    
    int inserted_count = 0;
    CnIrBasicBlock *block = site->block;
    CnIrInst *insert_before = site->call_inst;
    
    // 第一步：将所有实参复制到临时寄存器（避免参数依赖问题）
    for (size_t i = 0; i < func->param_count && i < site->call_inst->extra_args_count; i++) {
        CnIrOperand arg = site->call_inst->extra_args[i];
        
        // 如果实参是寄存器，先复制到临时寄存器
        if (arg.kind == CN_IR_OP_REG) {
            // 创建MOV指令：temp_reg = arg
            CnIrInst *mov_inst = cn_ir_inst_new(
                CN_IR_INST_MOV,
                make_reg_operand(temp_regs[i], arg.type),
                arg,
                make_none_operand()
            );
            
            // 插入到CALL指令之前
            mov_inst->next = insert_before;
            mov_inst->prev = insert_before->prev;
            if (insert_before->prev) {
                insert_before->prev->next = mov_inst;
            } else {
                block->first_inst = mov_inst;
            }
            insert_before->prev = mov_inst;
            inserted_count++;
        }
    }
    
    // 第二步：将临时寄存器的值赋给形参
    for (size_t i = 0; i < func->param_count && i < site->call_inst->extra_args_count; i++) {
        CnIrOperand arg = site->call_inst->extra_args[i];
        CnIrOperand param = func->params[i];
        
        // 确定源操作数
        CnIrOperand src;
        if (arg.kind == CN_IR_OP_REG) {
            // 使用临时寄存器
            src = make_reg_operand(temp_regs[i], arg.type);
        } else {
            // 直接使用立即数或符号
            src = arg;
        }
        
        // 创建MOV指令：param = src
        CnIrInst *mov_inst = cn_ir_inst_new(
            CN_IR_INST_MOV,
            param,
            src,
            make_none_operand()
        );
        
        // 插入到CALL指令之前
        mov_inst->next = insert_before;
        mov_inst->prev = insert_before->prev;
        if (insert_before->prev) {
            insert_before->prev->next = mov_inst;
        } else {
            block->first_inst = mov_inst;
        }
        insert_before->prev = mov_inst;
        inserted_count++;
    }
    
    return inserted_count;
}

/**
 * @brief 将尾调用转换为跳转到循环头
 * @param site 尾调用点
 * @param loop_header 循环头基本块
 */
static void convert_call_to_jump(TailCallSite *site, CnIrBasicBlock *loop_header) {
    if (!site || !loop_header) return;
    
    CnIrInst *call_inst = site->call_inst;
    
    // 将CALL指令转换为JUMP指令
    call_inst->kind = CN_IR_INST_JUMP;
    call_inst->dest = make_label_operand(loop_header);
    call_inst->src1 = make_none_operand();
    call_inst->src2 = make_none_operand();
    
    // 释放extra_args（不再需要）
    if (call_inst->extra_args) {
        free(call_inst->extra_args);
        call_inst->extra_args = NULL;
        call_inst->extra_args_count = 0;
    }
}

/**
 * @brief 移除尾调用点之后的RET指令
 * @param site 尾调用点
 */
static void remove_tail_ret_inst(TailCallSite *site) {
    if (!site || !site->ret_inst) return;
    
    CnIrInst *ret_inst = site->ret_inst;
    
    // 从链表中移除
    if (ret_inst->prev) {
        ret_inst->prev->next = ret_inst->next;
    } else {
        site->block->first_inst = ret_inst->next;
    }
    
    if (ret_inst->next) {
        ret_inst->next->prev = ret_inst->prev;
    } else {
        site->block->last_inst = ret_inst->prev;
    }
    
    // 释放指令
    free(ret_inst);
}

/**
 * @brief 在函数入口创建循环头基本块
 * @param func 函数指针
 * @return 新创建的循环头基本块
 */
static CnIrBasicBlock *create_loop_header(CnIrFunction *func) {
    if (!func) return NULL;
    
    // 创建循环头基本块
    CnIrBasicBlock *loop_header = cn_ir_basic_block_new("tail_rec_loop");
    
    // 获取原入口块
    CnIrBasicBlock *entry = func->first_block;
    
    // 创建新的入口块（只包含跳转到循环头的指令）
    CnIrBasicBlock *new_entry = cn_ir_basic_block_new("entry");
    
    // 在新入口块中添加跳转到循环头的指令
    CnIrInst *jump_inst = cn_ir_inst_new(
        CN_IR_INST_JUMP,
        make_label_operand(loop_header),
        make_none_operand(),
        make_none_operand()
    );
    cn_ir_basic_block_add_inst(new_entry, jump_inst);
    
    // 将原入口块的指令移动到循环头
    loop_header->first_inst = entry->first_inst;
    loop_header->last_inst = entry->last_inst;
    
    // 更新指令的块归属（通过遍历更新前驱后继关系）
    // 这里简化处理，主要更新链表结构
    
    // 更新函数的基本块链表
    new_entry->next = loop_header;
    loop_header->prev = new_entry;
    loop_header->next = entry->next;
    if (entry->next) {
        entry->next->prev = loop_header;
    }
    
    func->first_block = new_entry;
    if (func->last_block == entry) {
        func->last_block = loop_header;
    }
    
    // 注意：原entry块被替换，需要更新前驱后继关系
    // 这里简化处理，实际实现可能需要更复杂的CFG维护
    
    return loop_header;
}

/**
 * @brief 转换函数的尾递归为循环
 * @param func 函数指针
 * @param sites 尾调用点链表
 * @return true 如果转换成功
 */
static bool transform_tail_recursion(CnIrFunction *func, TailCallSite *sites) {
    if (!func || !sites) return false;
    
    // 分配临时寄存器用于参数传递
    int *temp_regs = NULL;
    if (func->param_count > 0) {
        temp_regs = malloc(sizeof(int) * func->param_count);
        for (size_t i = 0; i < func->param_count; i++) {
            temp_regs[i] = func->next_reg_id++;
        }
    }
    
    // 创建循环头基本块
    CnIrBasicBlock *loop_header = create_loop_header(func);
    if (!loop_header) {
        if (temp_regs) free(temp_regs);
        return false;
    }
    
    // 处理每个尾调用点
    int transformed_count = 0;
    for (TailCallSite *site = sites; site; site = site->next) {
        // 插入参数更新指令
        insert_param_updates(func, site, temp_regs);
        
        // 将CALL转换为JUMP
        convert_call_to_jump(site, loop_header);
        
        // 移除RET指令
        remove_tail_ret_inst(site);
        
        transformed_count++;
    }
    
    // 释放临时寄存器数组
    if (temp_regs) free(temp_regs);
    
    return transformed_count > 0;
}

// ============================================================================
// 主Pass函数
// ============================================================================

/**
 * @brief 尾递归优化Pass主函数
 * 
 * 执行步骤：
 * 1. 遍历所有函数
 * 2. 检查函数是否有尾递归调用
 * 3. 如果有，进行转换：
 *    a. 在函数入口创建循环标签
 *    b. 找到尾递归调用点
 *    c. 用参数赋值替换调用参数
 *    d. 用跳转到入口标签替换CALL指令
 * 
 * @param module IR模块
 */
void cn_ir_pass_tail_call_opt(CnIrModule *module) {
    if (!module) return;
    
    int total_transformed = 0;
    
    // 遍历所有函数
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        // 收集尾递归调用点
        TailCallSite *sites = collect_tail_call_sites(func);
        
        if (sites) {
            // 执行转换
            if (transform_tail_recursion(func, sites)) {
                total_transformed++;
            }
            
            // 释放尾调用点链表
            free_tail_call_sites(sites);
        }
    }
    
    // 注意：转换后可能需要死代码消除清理旧的参数
    // 这里不主动调用DCE，由Pass调度器决定
}
