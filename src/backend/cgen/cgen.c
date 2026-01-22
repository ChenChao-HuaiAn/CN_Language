#include "cnlang/backend/cgen.h"
#include "cnlang/support/diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// --- 控制流分析辅助结构 ---

typedef enum CnCFNodeType {
    CF_NODE_IF,
    CF_NODE_WHILE,
    CF_NODE_BASIC_BLOCK,
} CnCFNodeType;

typedef struct CnCFNode {
    CnCFNodeType type;
    CnIrBasicBlock *entry_block; // 对于 IF/WHILE，是条件块；对于 BB，是块本身
    struct CnCFNode *then_branch;  // IF 的 then 分支
    struct CnCFNode *else_branch;  // IF 的 else 分支
    struct CnCFNode *loop_body;    // WHILE 的循环体
    struct CnCFNode *next;         // 链表中的下一个节点
} CnCFNode;

// --- 辅助函数 ---

/**
 * @brief 获取对应于 CN 类型的 C 类型字符串
 * 
 * @param type CN 类型
 * @return const char* C 类型字符串
 */
static const char *get_c_type_string(CnType *type) {
    if (!type) return "void";

    switch (type->kind) {
        case CN_TYPE_INT:
            return "long long";
        case CN_TYPE_FLOAT:
            return "double";
        case CN_TYPE_BOOL:
            return "_Bool";
        case CN_TYPE_STRING:
            return "char*";
        case CN_TYPE_POINTER:
            // 递归获取指向的类型，并加上 *
            // 这里是一个简化实现，对于复杂的指针（如函数指针）需要更复杂的处理
            {
                static char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s*", get_c_type_string(type->as.pointer_to));
                return buffer;
            }
        case CN_TYPE_VOID:
            return "void";
        // TODO: 其他类型如数组、结构体等
        default:
            return "int"; // 默认类型
    }
}

/**
 * @brief 获取对应于 CN 函数名的 C 函数名
 * 
 * @param name CN 函数名
 * @return const char* C 函数名
 */
static const char *get_c_function_name(const char *name) {
    // 简单地在前面加上 cn_func_ 前缀
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_func_%s", name);
    return buffer;
}

/**
 * @brief 获取对应于 CN 变量名的 C 变量名
 * 
 * @param name CN 变量名
 * @return const char* C 变量名
 */
static const char *get_c_variable_name(const char *name) {
    // 简单地在前面加上 cn_var_ 前缀
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "cn_var_%s", name);
    return buffer;
}

/**
 * @brief 生成唯一的 C 标签名
 * 
 * @param ctx 生成器上下文
 * @return char* 标签名，调用者负责释放内存
 */
static char *generate_unique_label(CnCCodeGenContext *ctx) {
    char *label = malloc(64);
    snprintf(label, 64, "label_%d", ctx->label_counter++);
    return label;
}

/**
 * @brief 生成唯一的 C 临时变量名
 * 
 * @param ctx 生成器上下文
 * @return char* 临时变量名，调用者负责释放内存
 */
static char *generate_unique_temp_var(CnCCodeGenContext *ctx) {
    char *temp_var = malloc(64);
    snprintf(temp_var, 64, "temp_%d", ctx->temp_var_counter++);
    return temp_var;
}

/**
 * @brief 打印操作数到 C 代码
 * 
 * @param ctx 生成器上下文
 * @param op 操作数
 */
static void print_operand(CnCCodeGenContext *ctx, CnIrOperand op) {
    switch (op.kind) {
        case CN_IR_OP_NONE:
            fprintf(ctx->output_file, "/* NONE */");
            break;
        case CN_IR_OP_REG:
            fprintf(ctx->output_file, "r%d", op.as.reg_id);
            break;
        case CN_IR_OP_IMM_INT:
            fprintf(ctx->output_file, "%lld", op.as.imm_int);
            break;
        case CN_IR_OP_IMM_STR:
            fprintf(ctx->output_file, "\"%s\"", op.as.imm_str);
            break;
        case CN_IR_OP_SYMBOL:
            // 根据符号类型（变量、函数）使用不同的转换规则
            // 假设函数名和变量名都需要转换
            fprintf(ctx->output_file, "%s", get_c_variable_name(op.as.sym_name));
            break;
        case CN_IR_OP_LABEL:
            fprintf(ctx->output_file, "%s", op.as.label->name ? op.as.label->name : "unnamed_label");
            break;
        default:
            fprintf(ctx->output_file, "/* UNKNOWN_OPERAND */");
            break;
    }
}

// --- 核心生成函数 ---

// --- 控制流分析 ---

/**
 * @brief 分析函数的控制流图，构建控制流节点树
 * 
 * @param func IR 函数
 * @return CnCFNode* 控制流节点树的根节点链表
 */
static CnCFNode *analyze_cfg(CnIrFunction *func) {
    if (!func || !func->first_block) return NULL;

    // 这是一个非常简化的分析，仅用于演示目的。
    // 实际的控制流分析会更复杂，需要图算法（如支配树、循环检测等）。
    CnCFNode *head = NULL;
    CnCFNode *tail = NULL;

    CnIrBasicBlock *current_bb = func->first_block;
    while (current_bb) {
        CnIrInst *last_inst = current_bb->last_inst;
        if (last_inst && last_inst->kind == CN_IR_INST_BRANCH) {
            // 假设这是一个 IF 结构的开始
            CnCFNode *if_node = malloc(sizeof(CnCFNode));
            if_node->type = CF_NODE_IF;
            if_node->entry_block = current_bb;
            // 这里需要进一步分析后续块来确定 then/else/merge，
            // 为了简化，我们暂时只创建一个基本的 IF 节点，并将其余块作为普通块处理。
            if_node->then_branch = NULL; // TODO
            if_node->else_branch = NULL; // TODO
            if_node->loop_body = NULL;
            if_node->next = NULL;

            if (!head) {
                head = if_node;
                tail = if_node;
            } else {
                tail->next = if_node;
                tail = if_node;
            }
            // 跳过已处理的分支块，继续分析
        } else {
            // 普通基本块
            CnCFNode *bb_node = malloc(sizeof(CnCFNode));
            bb_node->type = CF_NODE_BASIC_BLOCK;
            bb_node->entry_block = current_bb;
            bb_node->then_branch = NULL;
            bb_node->else_branch = NULL;
            bb_node->loop_body = NULL;
            bb_node->next = NULL;

            if (!head) {
                head = bb_node;
                tail = bb_node;
            } else {
                tail->next = bb_node;
                tail = bb_node;
            }
        }
        current_bb = current_bb->next;
    }

    return head;
}

// --- 结构化代码生成 ---

static void print_structured_block_or_region(CnCCodeGenContext *ctx, CnCFNode *region);

static void print_structured_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block) {
    if (!block) return;

    if (block->name) {
        fprintf(ctx->output_file, "  /* Basic Block: %s */\n", block->name);
    }

    CnIrInst *inst = block->first_inst;
    while (inst) {
        cn_cgen_inst(ctx, inst);
        inst = inst->next;
    }
}

static void print_structured_if(CnCCodeGenContext *ctx, CnCFNode *if_node) {
    if (!if_node || if_node->type != CF_NODE_IF) return;

    CnIrBasicBlock *cond_block = if_node->entry_block;
    // 这里需要从 cond_block 中提取条件指令
    // 为简化，我们假设 BRANCH 指令的第一个源操作数就是条件
    CnIrInst *branch_inst = cond_block->last_inst;
    if (branch_inst && branch_inst->kind == CN_IR_INST_BRANCH) {
        fprintf(ctx->output_file, "  if (");
        print_operand(ctx, branch_inst->src1); // 条件
        fprintf(ctx->output_file, ") {\n");
        // 生成 then 分支
        if (if_node->then_branch) {
            print_structured_block_or_region(ctx, if_node->then_branch);
        }
        fprintf(ctx->output_file, "  } else {\n");
        // 生成 else 分支
        if (if_node->else_branch) {
            print_structured_block_or_region(ctx, if_node->else_branch);
        }
        fprintf(ctx->output_file, "  }\n");
    }
}

static void print_structured_while(CnCCodeGenContext *ctx, CnCFNode *while_node) {
    // TODO: 实现 while 循环的结构化生成
    // 需要检测循环头、循环体和出口
    fprintf(ctx->output_file, "  /* Structured WHILE loop generation not fully implemented */\n");
    if (while_node->entry_block) {
        print_structured_block(ctx, while_node->entry_block);
    }
    if (while_node->loop_body) {
        print_structured_block_or_region(ctx, while_node->loop_body);
    }
}

static void print_structured_block_or_region(CnCCodeGenContext *ctx, CnCFNode *region) {
    if (!region) return;
    switch (region->type) {
        case CF_NODE_BASIC_BLOCK:
            print_structured_block(ctx, region->entry_block);
            break;
        case CF_NODE_IF:
            print_structured_if(ctx, region);
            break;
        case CF_NODE_WHILE:
            print_structured_while(ctx, region);
            break;
        default:
            break;
    }
}

void cn_cgen_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block) {
    // 原来的实现是打印单个基本块
    // 现在我们改为分析整个函数的 CFG，然后打印整个结构化区域
    // 因此，这个函数的职责发生变化，它不再仅仅打印一个 block，而是打印由 analyze_cfg 识别出的结构。
    // 为了保持接口不变，我们在此处进行分析。
    // 但在实际设计中，分析和生成通常是分离的。
    
    CnIrFunction *func = ctx->current_func;
    if (!func) return;

    CnCFNode *cfg_tree = analyze_cfg(func);
    CnCFNode *current_region = cfg_tree;
    while (current_region) {
        print_structured_block_or_region(ctx, current_region);
        current_region = current_region->next;
    }

    // 清理分析结果
    CnCFNode *to_delete = cfg_tree;
    while (to_delete) {
        CnCFNode *next = to_delete->next;
        free(to_delete);
        to_delete = next;
    }
}


void cn_cgen_inst(CnCCodeGenContext *ctx, CnIrInst *inst) {
    if (!ctx || !inst) return;

    switch (inst->kind) {
        case CN_IR_INST_LABEL:
            // 标签指令：直接打印标签名和冒号
            fprintf(ctx->output_file, "  %s:\n", inst->dest.as.sym_name);
            break;

        case CN_IR_INST_ALLOCA:
            // ALLOCA: 生成局部变量声明
            // 目标通常是符号（变量名），源操作数是类型
            fprintf(ctx->output_file, "  %s %s;\n", get_c_type_string(inst->src1.type), get_c_variable_name(inst->dest.as.sym_name));
            break;

        case CN_IR_INST_LOAD:
            // LOAD: 从内存加载到寄存器
            // dest_reg = @var
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_STORE:
            // STORE: 从寄存器/立即数存储到内存
            // @var = src_value
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest); // 地址
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1); // 值
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_ADD:
        case CN_IR_INST_SUB:
        case CN_IR_INST_MUL:
        case CN_IR_INST_DIV:
        case CN_IR_INST_MOD:
        case CN_IR_INST_AND:
        case CN_IR_INST_OR:
        case CN_IR_INST_XOR:
        case CN_IR_INST_SHL:
        case CN_IR_INST_SHR:
        case CN_IR_INST_EQ:
        case CN_IR_INST_NE:
        case CN_IR_INST_LT:
        case CN_IR_INST_GT:
        case CN_IR_INST_LE:
        case CN_IR_INST_GE:
            // 二元运算指令: dest = op src1, src2
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, " ");
            
            // 根据 IR 指令类型映射到 C 运算符
            switch (inst->kind) {
                case CN_IR_INST_ADD: fprintf(ctx->output_file, "+"); break;
                case CN_IR_INST_SUB: fprintf(ctx->output_file, "-"); break;
                case CN_IR_INST_MUL: fprintf(ctx->output_file, "*"); break;
                case CN_IR_INST_DIV: fprintf(ctx->output_file, "/"); break;
                case CN_IR_INST_MOD: fprintf(ctx->output_file, "%%"); break;
                case CN_IR_INST_AND: fprintf(ctx->output_file, "&"); break;
                case CN_IR_INST_OR:  fprintf(ctx->output_file, "|"); break;
                case CN_IR_INST_XOR: fprintf(ctx->output_file, "^"); break;
                case CN_IR_INST_SHL: fprintf(ctx->output_file, "<<"); break;
                case CN_IR_INST_SHR: fprintf(ctx->output_file, ">>"); break;
                case CN_IR_INST_EQ:  fprintf(ctx->output_file, "=="); break;
                case CN_IR_INST_NE:  fprintf(ctx->output_file, "!="); break;
                case CN_IR_INST_LT:  fprintf(ctx->output_file, "<"); break;
                case CN_IR_INST_GT:  fprintf(ctx->output_file, ">"); break;
                case CN_IR_INST_LE:  fprintf(ctx->output_file, "<="); break;
                case CN_IR_INST_GE:  fprintf(ctx->output_file, ">="); break;
                default: break; // Should not happen
            }
            
            fprintf(ctx->output_file, " ");
            print_operand(ctx, inst->src2);
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_NEG:
        case CN_IR_INST_NOT:
            // 一元运算指令: dest = op src1
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            if (inst->kind == CN_IR_INST_NEG) fprintf(ctx->output_file, "-");
            else if (inst->kind == CN_IR_INST_NOT) fprintf(ctx->output_file, "!");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_MOV:
            // MOV: 寄存器间移动或加载立即数
            fprintf(ctx->output_file, "  ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, " = ");
            print_operand(ctx, inst->src1);
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_JUMP:
            // JUMP: 无条件跳转
            fprintf(ctx->output_file, "  goto ");
            print_operand(ctx, inst->dest);
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_BRANCH:
            // BRANCH: 条件跳转
            fprintf(ctx->output_file, "  if (");
            print_operand(ctx, inst->src1); // condition
            fprintf(ctx->output_file, ") goto ");
            print_operand(ctx, inst->dest); // true label
            fprintf(ctx->output_file, "; else goto ");
            print_operand(ctx, inst->src2); // false label
            fprintf(ctx->output_file, ";\n");
            break;

        case CN_IR_INST_CALL:
            // CALL: 函数调用
            if (inst->dest.kind != CN_IR_OP_NONE) {
                // 有返回值
                print_operand(ctx, inst->dest);
                fprintf(ctx->output_file, " = ");
            }
            print_operand(ctx, inst->src1); // callee
            fprintf(ctx->output_file, "(");
            for (size_t i = 0; i < inst->extra_args_count; i++) {
                print_operand(ctx, inst->extra_args[i]);
                if (i < inst->extra_args_count - 1) fprintf(ctx->output_file, ", ");
            }
            fprintf(ctx->output_file, ");\n");
            break;

        case CN_IR_INST_RET:
            // RET: 函数返回
            fprintf(ctx->output_file, "  return");
            if (inst->src1.kind != CN_IR_OP_NONE) {
                fprintf(ctx->output_file, " ");
                print_operand(ctx, inst->src1);
            }
            fprintf(ctx->output_file, ";\n");
            break;

        default:
            fprintf(ctx->output_file, "  /* Unsupported instruction: %d */\n", inst->kind);
            break;
    }
}

void cn_cgen_block(CnCCodeGenContext *ctx, CnIrBasicBlock *block) {
    if (!ctx || !block) return;

    // 打印基本块名称（作为注释或标签）
    if (block->name) {
        fprintf(ctx->output_file, "\n  /* Basic Block: %s */\n", block->name);
    }

    CnIrInst *inst = block->first_inst;
    while (inst) {
        cn_cgen_inst(ctx, inst);
        inst = inst->next;
    }
}

void cn_cgen_function(CnCCodeGenContext *ctx, CnIrFunction *func) {
    if (!ctx || !func) return;

    ctx->current_func = func;

    // 生成函数返回类型
    fprintf(ctx->output_file, "%s ", get_c_type_string(func->return_type));

    // 生成函数名
    fprintf(ctx->output_file, "%s(", get_c_function_name(func->name));

    // 生成参数列表
    for (size_t i = 0; i < func->param_count; i++) {
        fprintf(ctx->output_file, "%s %s", get_c_type_string(func->params[i].type), get_c_variable_name(func->params[i].as.sym_name));
        if (i < func->param_count - 1) {
            fprintf(ctx->output_file, ", ");
        }
    }
    fprintf(ctx->output_file, ") {\n");

    // 生成局部变量声明
    // 遍历函数中的所有基本块，找出所有 ALLOCA 指令对应的变量
    // 这里为了简化，假设局部变量已经在 ALLOCA 指令中生成了声明
    // 实际上可能需要两遍扫描，第一遍收集局部变量，第二遍生成代码

    // 生成函数体（基本块）
    CnIrBasicBlock *block = func->first_block;
    while (block) {
        ctx->current_block = block;
        cn_cgen_block(ctx, block);
        block = block->next;
    }

    fprintf(ctx->output_file, "}\n\n");
}

char *cn_cgen_module_to_string(CnIrModule *module) {
    if (!module) return NULL;

    // 使用内存流来构建字符串（一种方式是使用临时文件或动态分配缓冲区）
    // 这里我们使用一个简单的动态缓冲区追加的方式
    char *result = malloc(1); // 初始为空
    result[0] = '\0';
    size_t total_size = 1;

    // 预先估算大小或多次realloc，这里简化处理
    // 一个更健壮的方法是使用一个临时文件，然后读取其内容到字符串

    // 为了演示，我们直接返回一个固定格式的头部和占位符
    const char *header = "#include <stdio.h>\n#include <stdbool.h>\n\n";
    size_t header_len = strlen(header);
    result = realloc(result, header_len + 1);
    strcpy(result, header);
    total_size += header_len;

    CnIrFunction *func = module->first_func;
    while (func) {
        // 为每个函数生成代码并追加到结果字符串
        // 由于格式化输出到字符串比较复杂，这里用伪代码示意
        // 实际实现会需要一个更复杂的缓冲区管理机制，或者直接写入文件
        
        // 例如，可以创建一个临时FILE*到内存，然后读取
        // 或者使用一个动态增长的字符串缓冲区
        
        // 这里我们只是示意性地追加一些内容
        char *func_code = malloc(1024); // 假设足够大
        sprintf(func_code, "// Function: %s\n", func->name);
        size_t func_code_len = strlen(func_code);
        result = realloc(result, total_size + func_code_len);
        strcat(result, func_code);
        total_size += func_code_len;
        free(func_code);

        func = func->next;
    }

    // 注意：上面的实现是示意性的，实际的 `cn_cgen_module_to_string` 需要更复杂的缓冲区管理
    // 通常我们会直接实现 `cn_cgen_module_to_file`，因为它更直接

    return result;
}

int cn_cgen_module_to_file(CnIrModule *module, const char *filename) {
    if (!module || !filename) return -1;

    FILE *file = fopen(filename, "w");
    if (!file) {
        // TODO: 使用 diagnostics 报告错误
        return -1;
    }

    CnCCodeGenContext ctx = {0};
    ctx.output_file = file;
    ctx.label_counter = 0;
    ctx.temp_var_counter = 0;

    // 写入必要的 C 头文件
    fprintf(file, "#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n\n");

    // 遍历模块中的所有函数并生成 C 代码
    CnIrFunction *func = module->first_func;
    while (func) {
        cn_cgen_function(&ctx, func);
        func = func->next;
    }

    fclose(file);
    return 0;
}