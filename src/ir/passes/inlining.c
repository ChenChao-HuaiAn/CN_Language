/**
 * @file inlining.c
 * @brief 函数内联展开（Function Inlining）Pass实现
 * 
 * 算法原理：
 * 将函数调用替换为被调用函数的函数体，消除函数调用开销，
 * 并暴露更多优化机会（如常量传播、死代码消除）。
 * 
 * 示例：
 * 优化前：
 *   函数 add(a, b) { 返回 a + b; }
 *   x = add(1, 2);
 * 
 * 优化后：
 *   x = 1 + 2;  // 内联后可进一步常量折叠为 x = 3
 * 
 * 实现要点：
 * 1. 内联决策：基于函数大小、递归检测等启发式规则
 * 2. 寄存器重命名：避免内联后寄存器ID冲突
 * 3. 参数替换：将形参替换为实参
 * 4. 返回值处理：将RET指令转换为结果赋值
 * 5. 基本块连接：正确连接调用点和内联代码
 */

#include "cnlang/ir/pass.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* ========== 配置常量 ========== */

#define MAX_INST_COUNT_FOR_INLINE 10    // 内联的最大指令数阈值
#define MAX_REG_ID 1024                 // 最大寄存器ID
#define MAX_BLOCKS_IN_FUNC 64           // 函数内最大基本块数
#define MAX_PARAMS 32                   // 最大参数数量

/* ========== 数据结构定义 ========== */

/**
 * @brief 内联配置
 */
typedef struct CnIrInlineConfig {
    int max_inst_count;      // 最大指令数阈值
    bool inline_recursive;   // 是否内联递归函数（通常为false）
} CnIrInlineConfig;

/**
 * @brief 寄存器映射表
 * 
 * 用于在内联时重命名寄存器，避免ID冲突
 * 旧寄存器ID -> 新寄存器ID
 */
typedef struct CnIrRegMap {
    int mapping[MAX_REG_ID];     // 寄存器映射表
    bool valid[MAX_REG_ID];      // 映射是否有效
} CnIrRegMap;

/**
 * @brief 参数映射表
 * 
 * 将被调用函数的形参映射到调用点传入的实参
 * 形参寄存器ID -> 实参操作数
 */
typedef struct CnIrArgMap {
    CnIrOperand param_to_arg[MAX_PARAMS];  // 形参到实参的映射
    int param_count;                        // 参数数量
} CnIrArgMap;

/**
 * @brief 内联上下文
 * 
 * 包含内联过程中需要的所有信息
 */
typedef struct CnIrInlineContext {
    CnIrFunction *caller;            // 调用者函数
    CnIrFunction *callee;            // 被调用者函数
    CnIrBasicBlock *call_block;      // 调用点所在基本块
    CnIrInst *call_inst;             // CALL指令
    CnIrRegMap reg_map;              // 寄存器映射
    CnIrArgMap arg_map;              // 参数映射
    int next_reg_id;                 // 下一个可用的寄存器ID
    CnIrOperand result_reg;          // 用于存储返回结果的寄存器
} CnIrInlineContext;

/* ========== 辅助函数：寄存器映射 ========== */

/**
 * @brief 初始化寄存器映射表
 */
static void reg_map_init(CnIrRegMap *map) {
    memset(map, 0, sizeof(CnIrRegMap));
}

/**
 * @brief 设置寄存器映射
 */
static void reg_map_set(CnIrRegMap *map, int old_id, int new_id) {
    if (old_id >= 0 && old_id < MAX_REG_ID) {
        map->mapping[old_id] = new_id;
        map->valid[old_id] = true;
    }
}

/**
 * @brief 获取寄存器映射
 * 
 * @return 新寄存器ID，如果无映射则返回原ID
 */
static int reg_map_get(CnIrRegMap *map, int old_id) {
    if (old_id >= 0 && old_id < MAX_REG_ID && map->valid[old_id]) {
        return map->mapping[old_id];
    }
    return old_id;
}

/**
 * @brief 检查寄存器是否有映射
 */
static bool reg_map_has(CnIrRegMap *map, int old_id) {
    return old_id >= 0 && old_id < MAX_REG_ID && map->valid[old_id];
}

/* ========== 辅助函数：参数映射 ========== */

/**
 * @brief 初始化参数映射表
 */
static void arg_map_init(CnIrArgMap *map) {
    memset(map, 0, sizeof(CnIrArgMap));
}

/**
 * @brief 设置参数映射
 */
static void arg_map_set(CnIrArgMap *map, int param_idx, CnIrOperand arg) {
    if (param_idx >= 0 && param_idx < MAX_PARAMS) {
        map->param_to_arg[param_idx] = arg;
        if (param_idx + 1 > map->param_count) {
            map->param_count = param_idx + 1;
        }
    }
}

/**
 * @brief 获取参数对应的实参
 */
static CnIrOperand *arg_map_get(CnIrArgMap *map, int param_idx) {
    if (param_idx >= 0 && param_idx < map->param_count) {
        return &map->param_to_arg[param_idx];
    }
    return NULL;
}

/* ========== 辅助函数：指令计数 ========== */

/**
 * @brief 统计函数中的指令数量
 */
static int count_instructions(CnIrFunction *func) {
    int count = 0;
    for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
        for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
            count++;
        }
    }
    return count;
}

/**
 * @brief 检查函数是否是递归函数
 * 
 * 遍历函数体，检查是否有CALL指令调用自身
 */
static bool is_recursive_function(CnIrFunction *func) {
    for (CnIrBasicBlock *block = func->first_block; block; block = block->next) {
        for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
            if (inst->kind == CN_IR_INST_CALL) {
                // 检查是否调用自身
                if (inst->src1.kind == CN_IR_OP_SYMBOL && 
                    strcmp(inst->src1.as.sym_name, func->name) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief 根据名称查找函数
 */
static CnIrFunction *find_function(CnIrModule *module, const char *name) {
    for (CnIrFunction *func = module->first_func; func; func = func->next) {
        if (strcmp(func->name, name) == 0) {
            return func;
        }
    }
    return NULL;
}

/* ========== 辅助函数：操作数处理 ========== */

/**
 * @brief 重映射操作数中的寄存器ID
 */
static CnIrOperand remap_operand(CnIrOperand op, CnIrRegMap *reg_map, CnIrArgMap *arg_map) {
    if (op.kind == CN_IR_OP_REG) {
        int old_id = op.as.reg_id;
        
        // 首先检查是否是参数（参数需要替换为实参）
        // 注意：参数的寄存器ID通常从0开始
        if (arg_map && old_id < arg_map->param_count) {
            CnIrOperand *arg = arg_map_get(arg_map, old_id);
            if (arg) {
                return *arg;  // 返回实参
            }
        }
        
        // 否则进行寄存器重命名
        if (reg_map_has(reg_map, old_id)) {
            op.as.reg_id = reg_map_get(reg_map, old_id);
        }
    }
    return op;
}

/* ========== 辅助函数：指令复制 ========== */

/**
 * @brief 复制单条指令
 * 
 * 创建指令的深拷贝，并重映射寄存器ID
 */
static CnIrInst *copy_instruction(CnIrInst *inst, CnIrInlineContext *ctx) {
    CnIrInst *new_inst = cn_ir_inst_new(inst->kind, inst->dest, inst->src1, inst->src2);
    
    // 重映射目标操作数
    new_inst->dest = remap_operand(new_inst->dest, &ctx->reg_map, NULL);
    
    // 重映射源操作数（可能需要参数替换）
    new_inst->src1 = remap_operand(new_inst->src1, &ctx->reg_map, &ctx->arg_map);
    new_inst->src2 = remap_operand(new_inst->src2, &ctx->reg_map, &ctx->arg_map);
    
    // 处理额外参数（如CALL指令的参数列表）
    if (inst->extra_args_count > 0) {
        new_inst->extra_args = malloc(sizeof(CnIrOperand) * inst->extra_args_count);
        new_inst->extra_args_count = inst->extra_args_count;
        for (size_t i = 0; i < inst->extra_args_count; i++) {
            new_inst->extra_args[i] = remap_operand(inst->extra_args[i], &ctx->reg_map, &ctx->arg_map);
        }
    }
    
    return new_inst;
}

/**
 * @brief 复制基本块
 * 
 * 创建基本块的深拷贝，复制所有指令并重映射寄存器
 */
static CnIrBasicBlock *copy_basic_block(CnIrBasicBlock *block, CnIrInlineContext *ctx) {
    // 创建新基本块
    char new_name[128];
    snprintf(new_name, sizeof(new_name), "%s_inline_%s", 
             block->name ? block->name : "block", 
             ctx->callee->name);
    CnIrBasicBlock *new_block = cn_ir_basic_block_new(new_name);
    
    // 复制所有指令
    for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
        CnIrInst *new_inst = copy_instruction(inst, ctx);
        cn_ir_basic_block_add_inst(new_block, new_inst);
    }
    
    return new_block;
}

/* ========== 辅助函数：寄存器ID分配 ========== */

/**
 * @brief 为内联代码分配新的寄存器ID
 * 
 * 遍历被调用函数，为每个定义的寄存器分配新的ID
 */
static void allocate_new_register_ids(CnIrInlineContext *ctx) {
    ctx->next_reg_id = ctx->caller->next_reg_id;
    
    // 遍历被调用函数，为每个寄存器定义分配新ID
    for (CnIrBasicBlock *block = ctx->callee->first_block; block; block = block->next) {
        for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
            if (inst->dest.kind == CN_IR_OP_REG) {
                int old_id = inst->dest.as.reg_id;
                // 参数寄存器不需要重命名（会被实参替换）
                if (old_id >= ctx->callee->param_count) {
                    reg_map_set(&ctx->reg_map, old_id, ctx->next_reg_id++);
                }
            }
        }
    }
    
    // 更新调用者的下一个可用寄存器ID
    ctx->caller->next_reg_id = ctx->next_reg_id;
}

/* ========== 辅助函数：参数映射构建 ========== */

/**
 * @brief 构建形参到实参的映射
 */
static void build_argument_mapping(CnIrInlineContext *ctx) {
    arg_map_init(&ctx->arg_map);
    
    // CALL指令的extra_args存储了实参列表
    // src1是函数名（符号），extra_args是参数列表
    for (size_t i = 0; i < ctx->call_inst->extra_args_count && i < ctx->callee->param_count; i++) {
        arg_map_set(&ctx->arg_map, i, ctx->call_inst->extra_args[i]);
    }
}

/* ========== 核心函数：内联决策 ========== */

/**
 * @brief 判断是否应该内联函数
 * 
 * 内联条件：
 * 1. 被调用函数存在
 * 2. 不是递归调用（caller != callee）
 * 3. 函数体大小适中（指令数 <= 阈值）
 * 4. 不是中断处理函数
 */
static bool should_inline(CnIrFunction *caller, CnIrFunction *callee, CnIrInlineConfig *config) {
    // 被调用函数不存在
    if (!callee) return false;
    
    // 不内联递归调用
    if (caller == callee) return false;
    
    // 不内联递归函数（会导致无限展开）
    if (is_recursive_function(callee)) return false;
    
    // 不内联中断处理函数
    if (callee->is_interrupt_handler) return false;
    
    // 检查函数体大小
    int inst_count = count_instructions(callee);
    if (inst_count > config->max_inst_count) return false;
    
    return true;
}

/* ========== 核心函数：执行内联 ========== */

/**
 * @brief 在调用点执行内联展开
 * 
 * 步骤：
 * 1. 分配新的寄存器ID
 * 2. 构建参数映射
 * 3. 分割调用点基本块
 * 4. 复制被调用函数的基本块
 * 5. 处理返回指令
 * 6. 连接基本块
 * 7. 删除原CALL指令
 */
static bool inline_function(CnIrInlineContext *ctx) {
    // 1. 分配新的寄存器ID
    allocate_new_register_ids(ctx);
    
    // 2. 构建参数映射
    build_argument_mapping(ctx);
    
    // 3. 分割调用点基本块
    // 将CALL指令之后的所有指令移到新块
    CnIrBasicBlock *after_block = cn_ir_basic_block_new("after_inline");
    
    // 移动CALL指令之后的指令到after_block
    CnIrInst *inst = ctx->call_inst->next;
    while (inst) {
        CnIrInst *next = inst->next;
        
        // 从原块移除
        if (inst->prev) inst->prev->next = inst->next;
        else ctx->call_block->first_inst = inst->next;
        if (inst->next) inst->next->prev = inst->prev;
        else ctx->call_block->last_inst = inst->prev;
        
        // 添加到after_block
        inst->prev = inst->next = NULL;
        cn_ir_basic_block_add_inst(after_block, inst);
        
        inst = next;
    }
    
    // 4. 复制被调用函数的基本块
    CnIrBasicBlock *first_copied = NULL;
    CnIrBasicBlock *last_copied = NULL;
    
    for (CnIrBasicBlock *block = ctx->callee->first_block; block; block = block->next) {
        CnIrBasicBlock *copied = copy_basic_block(block, ctx);
        
        // 添加到调用者函数
        cn_ir_function_add_block(ctx->caller, copied);
        
        if (!first_copied) first_copied = copied;
        last_copied = copied;
    }
    
    // 5. 处理返回指令
    // 将RET指令转换为结果赋值 + 跳转到after_block
    for (CnIrBasicBlock *block = first_copied; block; block = block->next) {
        for (CnIrInst *i = block->first_inst; i; i = i->next) {
            if (i->kind == CN_IR_INST_RET) {
                // RET指令的src1包含返回值
                if (i->src1.kind != CN_IR_OP_NONE && ctx->call_inst->dest.kind == CN_IR_OP_REG) {
                    // 创建MOV指令：将返回值赋给CALL指令的目标寄存器
                    CnIrInst *mov_inst = cn_ir_inst_new(
                        CN_IR_INST_MOV,
                        ctx->call_inst->dest,  // CALL的目标寄存器
                        i->src1,               // 返回值
                        cn_ir_op_none()
                    );
                    
                    // 替换RET为MOV
                    i->kind = mov_inst->kind;
                    i->dest = mov_inst->dest;
                    i->src1 = mov_inst->src1;
                    i->src2 = mov_inst->src2;
                    free(mov_inst);
                }
                
                // 添加跳转到after_block
                CnIrInst *jump_inst = cn_ir_inst_new(
                    CN_IR_INST_JUMP,
                    cn_ir_op_label(after_block),
                    cn_ir_op_none(),
                    cn_ir_op_none()
                );
                
                // 在MOV之后插入JUMP（或直接替换RET为JUMP）
                if (i->kind == CN_IR_INST_MOV) {
                    // 在i之后插入jump
                    jump_inst->prev = i;
                    jump_inst->next = i->next;
                    if (i->next) i->next->prev = jump_inst;
                    else block->last_inst = jump_inst;
                    i->next = jump_inst;
                } else {
                    // 直接替换RET为JUMP
                    i->kind = CN_IR_INST_JUMP;
                    i->dest = cn_ir_op_label(after_block);
                    i->src1 = cn_ir_op_none();
                    i->src2 = cn_ir_op_none();
                    free(jump_inst);
                }
            }
        }
    }
    
    // 6. 连接基本块
    // 在CALL指令位置插入跳转到内联代码
    CnIrInst *jump_to_inline = cn_ir_inst_new(
        CN_IR_INST_JUMP,
        cn_ir_op_label(first_copied),
        cn_ir_op_none(),
        cn_ir_op_none()
    );
    
    // 替换CALL指令为JUMP
    ctx->call_inst->kind = jump_to_inline->kind;
    ctx->call_inst->dest = jump_to_inline->dest;
    ctx->call_inst->src1 = jump_to_inline->src1;
    ctx->call_inst->src2 = jump_to_inline->src2;
    ctx->call_inst->extra_args_count = 0;
    if (ctx->call_inst->extra_args) {
        free(ctx->call_inst->extra_args);
        ctx->call_inst->extra_args = NULL;
    }
    free(jump_to_inline);
    
    // 添加after_block到函数
    cn_ir_function_add_block(ctx->caller, after_block);
    
    return true;
}

/* ========== 主Pass函数 ========== */

/**
 * @brief 函数内联展开Pass主函数
 * 
 * 遍历模块中所有函数的调用点，根据启发式规则决定是否内联
 */
void cn_ir_pass_inline(CnIrModule *module) {
    if (!module) return;
    
    // 默认配置
    CnIrInlineConfig config = {
        .max_inst_count = MAX_INST_COUNT_FOR_INLINE,
        .inline_recursive = false
    };
    
    bool changed = true;
    int max_iterations = 10;  // 防止无限循环
    int iteration = 0;
    
    while (changed && iteration < max_iterations) {
        changed = false;
        iteration++;
        
        for (CnIrFunction *caller = module->first_func; caller; caller = caller->next) {
            for (CnIrBasicBlock *block = caller->first_block; block; block = block->next) {
                for (CnIrInst *inst = block->first_inst; inst; inst = inst->next) {
                    if (inst->kind == CN_IR_INST_CALL) {
                        // 获取被调用函数
                        if (inst->src1.kind != CN_IR_OP_SYMBOL) continue;
                        
                        CnIrFunction *callee = find_function(module, inst->src1.as.sym_name);
                        
                        // 判断是否应该内联
                        if (should_inline(caller, callee, &config)) {
                            // 准备内联上下文
                            CnIrInlineContext ctx = {0};
                            ctx.caller = caller;
                            ctx.callee = callee;
                            ctx.call_block = block;
                            ctx.call_inst = inst;
                            reg_map_init(&ctx.reg_map);
                            
                            // 执行内联
                            if (inline_function(&ctx)) {
                                changed = true;
                                // 内联后重新开始遍历（因为结构已改变）
                                goto next_iteration;
                            }
                        }
                    }
                }
            }
        }
        next_iteration:;
    }
}
