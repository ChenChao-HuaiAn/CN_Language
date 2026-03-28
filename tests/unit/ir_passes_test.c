/**
 * @file ir_passes_test.c
 * @brief IR优化Pass单元测试
 * 
 * 测试覆盖以下优化Pass：
 * 1. 公共子表达式消除（CSE）
 * 2. 复写传播（Copy Propagation）
 * 3. 循环不变量外提（Loop Invariant Code Motion）
 * 4. 函数内联展开（Function Inlining）
 * 5. 强度削弱（Strength Reduction）
 * 6. 尾递归优化（Tail Call Optimization）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cnlang/ir/ir.h"
#include "cnlang/ir/pass.h"

// ============================================================================
// 测试统计
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    [FAIL] %s\n", msg); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_PASS(name) do { \
    printf("    [PASS] %s\n", name); \
    tests_passed++; \
} while(0)

// ============================================================================
// 辅助函数：创建测试用IR结构
// ============================================================================

/**
 * @brief 创建整数立即数操作数
 */
static CnIrOperand make_imm_int_op(long long value) {
    CnIrOperand op;
    op.kind = CN_IR_OP_IMM_INT;
    op.as.imm_int = value;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建寄存器操作数
 */
static CnIrOperand make_reg_op(int reg_id) {
    CnIrOperand op;
    op.kind = CN_IR_OP_REG;
    op.as.reg_id = reg_id;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建符号操作数
 */
static CnIrOperand make_symbol_op(const char *name) {
    CnIrOperand op;
    op.kind = CN_IR_OP_SYMBOL;
    op.as.sym_name = name;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建空操作数
 */
static CnIrOperand make_none_op(void) {
    CnIrOperand op;
    op.kind = CN_IR_OP_NONE;
    op.type = NULL;
    return op;
}

/**
 * @brief 创建IR指令
 */
static CnIrInst *create_inst(CnIrInstKind kind, CnIrOperand dest, 
                              CnIrOperand src1, CnIrOperand src2) {
    CnIrInst *inst = (CnIrInst *)malloc(sizeof(CnIrInst));
    if (!inst) return NULL;
    
    inst->kind = kind;
    inst->dest = dest;
    inst->src1 = src1;
    inst->src2 = src2;
    inst->extra_args = NULL;
    inst->extra_args_count = 0;
    inst->next = NULL;
    inst->prev = NULL;
    
    return inst;
}

/**
 * @brief 统计基本块中指定类型指令的数量
 */
static int count_inst_kind(CnIrBasicBlock *block, CnIrInstKind kind) {
    int count = 0;
    CnIrInst *inst = block->first_inst;
    while (inst) {
        if (inst->kind == kind) {
            count++;
        }
        inst = inst->next;
    }
    return count;
}

/**
 * @brief 查找基本块中指定类型的第n条指令
 */
static CnIrInst *find_inst_by_kind(CnIrBasicBlock *block, CnIrInstKind kind, int index) {
    int count = 0;
    CnIrInst *inst = block->first_inst;
    while (inst) {
        if (inst->kind == kind) {
            if (count == index) {
                return inst;
            }
            count++;
        }
        inst = inst->next;
    }
    return NULL;
}

// ============================================================================
// 测试用例：IR基础结构
// ============================================================================

/**
 * @brief 测试IR模块创建和释放
 */
static void test_ir_module_basic(void) {
    printf("测试：IR模块基础操作\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建IR模块失败");
    
    cn_ir_module_free(module);
    TEST_PASS("IR模块创建和释放");
}

/**
 * @brief 测试IR函数创建
 */
static void test_ir_function_basic(void) {
    printf("测试：IR函数基础操作\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建IR模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_func", NULL);
    TEST_ASSERT(func != NULL, "创建IR函数失败");
    TEST_ASSERT(strcmp(func->name, "test_func") == 0, "函数名不正确");
    
    // 添加到模块
    if (module->last_func) {
        module->last_func->next = func;
    } else {
        module->first_func = func;
    }
    module->last_func = func;
    
    cn_ir_module_free(module);
    TEST_PASS("IR函数创建");
}

/**
 * @brief 测试IR基本块创建
 */
static void test_ir_basic_block_basic(void) {
    printf("测试：IR基本块基础操作\n");
    
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    TEST_ASSERT(block != NULL, "创建基本块失败");
    TEST_ASSERT(block->name != NULL, "基本块名称为空");
    
    // 添加指令
    CnIrInst *inst = create_inst(CN_IR_INST_RET, 
                                  make_none_op(), 
                                  make_none_op(), 
                                  make_none_op());
    TEST_ASSERT(inst != NULL, "创建指令失败");
    
    cn_ir_basic_block_add_inst(block, inst);
    TEST_ASSERT(block->first_inst == inst, "指令未正确添加");
    TEST_ASSERT(block->last_inst == inst, "last_inst未正确设置");
    
    free(inst);
    free((void*)block->name);
    free(block);
    TEST_PASS("IR基本块创建");
}

// ============================================================================
// 测试用例：强度削弱（Strength Reduction）
// ============================================================================

/**
 * @brief 测试强度削弱：乘法转移位
 */
static void test_strength_reduction_mul_to_shift(void) {
    printf("测试：强度削弱 - 乘法转移位\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_strength", NULL);
    TEST_ASSERT(func != NULL, "创建函数失败");
    
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    TEST_ASSERT(block != NULL, "创建基本块失败");
    
    // 添加函数到模块
    module->first_func = func;
    module->last_func = func;
    
    // 添加基本块到函数
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = mul %0, 8
    CnIrInst *inst = create_inst(CN_IR_INST_MUL, 
                                  make_reg_op(1), 
                                  make_reg_op(0), 
                                  make_imm_int_op(8));
    cn_ir_basic_block_add_inst(block, inst);
    
    // 执行强度削弱
    cn_ir_pass_strength_reduction(module);
    
    // 验证：MUL应该被替换为SHL
    int shl_count = count_inst_kind(block, CN_IR_INST_SHL);
    TEST_ASSERT(shl_count == 1, "应该有SHL指令");
    
    cn_ir_module_free(module);
    TEST_PASS("强度削弱 - 乘法转移位");
}

/**
 * @brief 测试强度削弱：除法转移位（需要无符号类型）
 *
 * 注意：强度削弱对除法的优化需要无符号类型，否则可能产生错误结果
 */
static void test_strength_reduction_div_to_shift(void) {
    printf("测试：强度削弱 - 除法转移位\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_strength_div", NULL);
    TEST_ASSERT(func != NULL, "创建函数失败");
    
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    TEST_ASSERT(block != NULL, "创建基本块失败");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = div %0, 4
    // 注意：强度削弱对除法只优化无符号类型
    CnIrInst *inst = create_inst(CN_IR_INST_DIV,
                                  make_reg_op(1),
                                  make_reg_op(0),
                                  make_imm_int_op(4));
    cn_ir_basic_block_add_inst(block, inst);
    
    cn_ir_pass_strength_reduction(module);
    
    // 由于没有设置无符号类型，除法不会被优化为移位
    // 这是正确的行为（有符号数除法不能简单转换为移位）
    int div_count = count_inst_kind(block, CN_IR_INST_DIV);
    TEST_ASSERT(div_count == 1, "有符号除法不应被优化为移位");
    
    cn_ir_module_free(module);
    TEST_PASS("强度削弱 - 除法转移位（有符号数不优化）");
}

/**
 * @brief 测试强度削弱：取模转位与（需要无符号类型）
 *
 * 注意：强度削弱对取模的优化需要无符号类型，否则可能产生错误结果
 */
static void test_strength_reduction_mod_to_and(void) {
    printf("测试：强度削弱 - 取模转位与\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_strength_mod", NULL);
    TEST_ASSERT(func != NULL, "创建函数失败");
    
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    TEST_ASSERT(block != NULL, "创建基本块失败");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = mod %0, 8
    // 注意：强度削弱对取模只优化无符号类型
    CnIrInst *inst = create_inst(CN_IR_INST_MOD,
                                  make_reg_op(1),
                                  make_reg_op(0),
                                  make_imm_int_op(8));
    cn_ir_basic_block_add_inst(block, inst);
    
    cn_ir_pass_strength_reduction(module);
    
    // 由于没有设置无符号类型，取模不会被优化为位与
    // 这是正确的行为（有符号数取模不能简单转换为位与）
    int mod_count = count_inst_kind(block, CN_IR_INST_MOD);
    TEST_ASSERT(mod_count == 1, "有符号取模不应被优化为位与");
    
    cn_ir_module_free(module);
    TEST_PASS("强度削弱 - 取模转位与（有符号数不优化）");
}

// ============================================================================
// 测试用例：常量折叠（Constant Folding）
// ============================================================================

/**
 * @brief 测试常量折叠：基本算术运算
 */
static void test_constant_folding_basic(void) {
    printf("测试：常量折叠 - 基本算术\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_const_fold", NULL);
    TEST_ASSERT(func != NULL, "创建函数失败");
    
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    TEST_ASSERT(block != NULL, "创建基本块失败");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = add 5, 3
    CnIrInst *inst = create_inst(CN_IR_INST_ADD, 
                                  make_reg_op(1), 
                                  make_imm_int_op(5), 
                                  make_imm_int_op(3));
    cn_ir_basic_block_add_inst(block, inst);
    
    cn_ir_pass_constant_folding(module);
    
    // 验证：ADD应该被替换为MOV
    int mov_count = count_inst_kind(block, CN_IR_INST_MOV);
    TEST_ASSERT(mov_count == 1, "应该有MOV指令");
    
    // 验证折叠结果
    CnIrInst *mov_inst = find_inst_by_kind(block, CN_IR_INST_MOV, 0);
    TEST_ASSERT(mov_inst != NULL, "找不到MOV指令");
    TEST_ASSERT(mov_inst->src1.kind == CN_IR_OP_IMM_INT, "源操作数应该是立即数");
    TEST_ASSERT(mov_inst->src1.as.imm_int == 8, "折叠结果应该是8");
    
    cn_ir_module_free(module);
    TEST_PASS("常量折叠 - 基本算术");
}

/**
 * @brief 测试常量折叠：减法
 */
static void test_constant_folding_sub(void) {
    printf("测试：常量折叠 - 减法\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_fold_sub", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = sub 10, 3
    CnIrInst *inst = create_inst(CN_IR_INST_SUB, 
                                  make_reg_op(1), 
                                  make_imm_int_op(10), 
                                  make_imm_int_op(3));
    cn_ir_basic_block_add_inst(block, inst);
    
    cn_ir_pass_constant_folding(module);
    
    CnIrInst *mov_inst = find_inst_by_kind(block, CN_IR_INST_MOV, 0);
    TEST_ASSERT(mov_inst != NULL, "找不到MOV指令");
    TEST_ASSERT(mov_inst->src1.as.imm_int == 7, "折叠结果应该是7");
    
    cn_ir_module_free(module);
    TEST_PASS("常量折叠 - 减法");
}

/**
 * @brief 测试常量折叠：乘法
 */
static void test_constant_folding_mul(void) {
    printf("测试：常量折叠 - 乘法\n");
    
    CnIrModule *module = cn_ir_module_new();
    CnIrFunction *func = cn_ir_function_new("test_fold_mul", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = mul 6, 7
    CnIrInst *inst = create_inst(CN_IR_INST_MUL, 
                                  make_reg_op(1), 
                                  make_imm_int_op(6), 
                                  make_imm_int_op(7));
    cn_ir_basic_block_add_inst(block, inst);
    
    cn_ir_pass_constant_folding(module);
    
    CnIrInst *mov_inst = find_inst_by_kind(block, CN_IR_INST_MOV, 0);
    TEST_ASSERT(mov_inst != NULL, "找不到MOV指令");
    TEST_ASSERT(mov_inst->src1.as.imm_int == 42, "折叠结果应该是42");
    
    cn_ir_module_free(module);
    TEST_PASS("常量折叠 - 乘法");
}

// ============================================================================
// 测试用例：公共子表达式消除（CSE）
// ============================================================================

/**
 * @brief 测试CSE：相同表达式应被合并
 *
 * 注意：CSE只处理两个源操作数都是寄存器的情况
 */
static void test_cse_same_expression(void) {
    printf("测试：CSE - 相同表达式合并\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_cse", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 5;
    
    // %3 = add %0, %1（两个源操作数都是寄存器）
    CnIrInst *inst1 = create_inst(CN_IR_INST_ADD,
                                   make_reg_op(3),
                                   make_reg_op(0),
                                   make_reg_op(1));
    cn_ir_basic_block_add_inst(block, inst1);
    
    // %4 = add %0, %1（相同的表达式，两个源操作数都是寄存器）
    CnIrInst *inst2 = create_inst(CN_IR_INST_ADD,
                                   make_reg_op(4),
                                   make_reg_op(0),
                                   make_reg_op(1));
    cn_ir_basic_block_add_inst(block, inst2);
    
    cn_ir_pass_cse(module);
    
    // CSE应该将第二条ADD替换为MOV
    int add_count = count_inst_kind(block, CN_IR_INST_ADD);
    int mov_count = count_inst_kind(block, CN_IR_INST_MOV);
    
    TEST_ASSERT(add_count == 1, "应该只剩一条ADD指令");
    TEST_ASSERT(mov_count == 1, "应该有一条MOV指令");
    
    cn_ir_module_free(module);
    TEST_PASS("CSE - 相同表达式合并");
}

/**
 * @brief 测试CSE：不同表达式不应被合并
 */
static void test_cse_different_expression(void) {
    printf("测试：CSE - 不同表达式不合并\n");
    
    CnIrModule *module = cn_ir_module_new();
    CnIrFunction *func = cn_ir_function_new("test_cse_diff", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 3;
    
    // %1 = add %0, 5
    CnIrInst *inst1 = create_inst(CN_IR_INST_ADD, 
                                   make_reg_op(1), 
                                   make_reg_op(0), 
                                   make_imm_int_op(5));
    cn_ir_basic_block_add_inst(block, inst1);
    
    // %2 = add %0, 6（不同的立即数）
    CnIrInst *inst2 = create_inst(CN_IR_INST_ADD, 
                                   make_reg_op(2), 
                                   make_reg_op(0), 
                                   make_imm_int_op(6));
    cn_ir_basic_block_add_inst(block, inst2);
    
    cn_ir_pass_cse(module);
    
    int add_count = count_inst_kind(block, CN_IR_INST_ADD);
    TEST_ASSERT(add_count == 2, "两条ADD指令都应保留");
    
    cn_ir_module_free(module);
    TEST_PASS("CSE - 不同表达式不合并");
}

// ============================================================================
// 测试用例：复写传播（Copy Propagation）
// ============================================================================

/**
 * @brief 测试复写传播：基本传播
 */
static void test_copy_propagation_basic(void) {
    printf("测试：复写传播 - 基本传播\n");
    
    CnIrModule *module = cn_ir_module_new();
    CnIrFunction *func = cn_ir_function_new("test_copy_prop", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 3;
    
    // %1 = mov %0
    CnIrInst *inst1 = create_inst(CN_IR_INST_MOV, 
                                   make_reg_op(1), 
                                   make_reg_op(0), 
                                   make_none_op());
    cn_ir_basic_block_add_inst(block, inst1);
    
    // %2 = add %1, 5
    CnIrInst *inst2 = create_inst(CN_IR_INST_ADD, 
                                   make_reg_op(2), 
                                   make_reg_op(1), 
                                   make_imm_int_op(5));
    cn_ir_basic_block_add_inst(block, inst2);
    
    cn_ir_pass_copy_propagation(module);
    
    // 验证：ADD指令的源操作数应该变为%0
    CnIrInst *add_inst = find_inst_by_kind(block, CN_IR_INST_ADD, 0);
    TEST_ASSERT(add_inst != NULL, "应该有ADD指令");
    TEST_ASSERT(add_inst->src1.kind == CN_IR_OP_REG, "源操作数应该是寄存器");
    TEST_ASSERT(add_inst->src1.as.reg_id == 0, "源操作数应该被替换为%0");
    
    cn_ir_module_free(module);
    TEST_PASS("复写传播 - 基本传播");
}

// ============================================================================
// 测试用例：死代码消除（Dead Code Elimination）
// ============================================================================

/**
 * @brief 测试死代码消除：空模块
 */
static void test_dead_code_elimination_empty(void) {
    printf("测试：死代码消除 - 空模块\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    // 空模块，应该不崩溃
    cn_ir_pass_dead_code_elimination(module);
    
    cn_ir_module_free(module);
    TEST_PASS("死代码消除 - 空模块");
}

// ============================================================================
// 测试用例：循环不变量外提
// ============================================================================

/**
 * @brief 测试循环不变量外提：空模块
 */
static void test_loop_invariant_empty(void) {
    printf("测试：循环不变量外提 - 空模块\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    // 空模块，应该不崩溃
    cn_ir_pass_loop_invariant_code_motion(module);
    
    cn_ir_module_free(module);
    TEST_PASS("循环不变量外提 - 空模块");
}

// ============================================================================
// 测试用例：函数内联
// ============================================================================

/**
 * @brief 测试函数内联：空模块
 */
static void test_inlining_empty(void) {
    printf("测试：函数内联 - 空模块\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    // 空模块，应该不崩溃
    cn_ir_pass_inline(module);
    
    cn_ir_module_free(module);
    TEST_PASS("函数内联 - 空模块");
}

// ============================================================================
// 测试用例：尾递归优化
// ============================================================================

/**
 * @brief 测试尾递归优化：空模块
 */
static void test_tail_call_opt_empty(void) {
    printf("测试：尾递归优化 - 空模块\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    // 空模块，应该不崩溃
    cn_ir_pass_tail_call_opt(module);
    
    cn_ir_module_free(module);
    TEST_PASS("尾递归优化 - 空模块");
}

// ============================================================================
// 测试用例：默认优化Pass组合
// ============================================================================

/**
 * @brief 测试默认优化Pass组合
 */
static void test_default_passes(void) {
    printf("测试：默认优化Pass组合\n");
    
    CnIrModule *module = cn_ir_module_new();
    TEST_ASSERT(module != NULL, "创建模块失败");
    
    CnIrFunction *func = cn_ir_function_new("test_default", NULL);
    CnIrBasicBlock *block = cn_ir_basic_block_new("entry");
    
    module->first_func = func;
    module->last_func = func;
    cn_ir_function_add_block(func, block);
    func->next_reg_id = 2;
    
    // %1 = add 5, 3（可常量折叠）
    CnIrInst *inst = create_inst(CN_IR_INST_ADD, 
                                  make_reg_op(1), 
                                  make_imm_int_op(5), 
                                  make_imm_int_op(3));
    cn_ir_basic_block_add_inst(block, inst);
    
    // 运行所有默认优化
    cn_ir_run_default_passes(module);
    
    // 验证优化结果
    int mov_count = count_inst_kind(block, CN_IR_INST_MOV);
    TEST_ASSERT(mov_count >= 1, "应该有MOV指令（常量折叠结果）");
    
    cn_ir_module_free(module);
    TEST_PASS("默认优化Pass组合");
}

// ============================================================================
// 主测试函数
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("IR优化Pass单元测试\n");
    printf("========================================\n\n");
    
    // IR基础结构测试
    printf("--- IR基础结构测试 ---\n");
    test_ir_module_basic();
    test_ir_function_basic();
    test_ir_basic_block_basic();
    printf("\n");
    
    // 强度削弱测试
    printf("--- 强度削弱测试 ---\n");
    test_strength_reduction_mul_to_shift();
    test_strength_reduction_div_to_shift();
    test_strength_reduction_mod_to_and();
    printf("\n");
    
    // 常量折叠测试
    printf("--- 常量折叠测试 ---\n");
    test_constant_folding_basic();
    test_constant_folding_sub();
    test_constant_folding_mul();
    printf("\n");
    
    // CSE测试
    printf("--- 公共子表达式消除（CSE）测试 ---\n");
    test_cse_same_expression();
    test_cse_different_expression();
    printf("\n");
    
    // 复写传播测试
    printf("--- 复写传播测试 ---\n");
    test_copy_propagation_basic();
    printf("\n");
    
    // 死代码消除测试
    printf("--- 死代码消除测试 ---\n");
    test_dead_code_elimination_empty();
    printf("\n");
    
    // 循环不变量外提测试
    printf("--- 循环不变量外提测试 ---\n");
    test_loop_invariant_empty();
    printf("\n");
    
    // 函数内联测试
    printf("--- 函数内联测试 ---\n");
    test_inlining_empty();
    printf("\n");
    
    // 尾递归优化测试
    printf("--- 尾递归优化测试 ---\n");
    test_tail_call_opt_empty();
    printf("\n");
    
    // 默认优化Pass组合测试
    printf("--- 默认优化Pass组合测试 ---\n");
    test_default_passes();
    printf("\n");
    
    printf("========================================\n");
    printf("测试结果: %d 通过, %d 失败\n", tests_passed, tests_failed);
    printf("========================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
