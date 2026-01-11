/******************************************************************************
 * 文件名: CN_cfg_basic_block.c
 * 功能: CN_Language 控制流图基本块数据结构实现
 * 
 * 实现控制流图的基本块数据结构，包括基本块属性、前驱后继关系、
 * 支配关系和数据流信息。
 * 
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_cfg_basic_block.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../../infrastructure/memory/CN_memory_interface.h"

// ============================================================================
// 常量定义
// ============================================================================

#define INITIAL_INSTRUCTION_CAPACITY 16
#define INITIAL_PREDECESSOR_CAPACITY 4
#define INITIAL_SUCCESSOR_CAPACITY 4
#define INITIAL_DOMINATOR_CAPACITY 4
#define INITIAL_DOMINATED_CAPACITY 4

// ============================================================================
// 静态变量
// ============================================================================

static size_t g_next_basic_block_id = 1;

// ============================================================================
// 静态函数声明
// ============================================================================

static bool ensure_instruction_capacity(Stru_BasicBlock_t* block);
static bool ensure_predecessor_capacity(Stru_BasicBlock_t* block);
static bool ensure_successor_capacity(Stru_BasicBlock_t* block);
static bool ensure_dominator_capacity(Stru_BasicBlock_t* block);
static bool ensure_dominated_capacity(Stru_BasicBlock_t* block);
static void free_instruction_array(Stru_IrInstruction_t** instructions, size_t count);
static void free_basic_block_array(Stru_BasicBlock_t** blocks, size_t count);

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_BasicBlock_t* F_create_basic_block(const char* name, 
                                        Eum_BasicBlockType type, 
                                        size_t id)
{
    // 分配基本块结构体
    Stru_BasicBlock_t* block = (Stru_BasicBlock_t*)cn_malloc(sizeof(Stru_BasicBlock_t));
    if (!block)
    {
        return NULL;
    }
    
    // 初始化基本信息
    memset(block, 0, sizeof(Stru_BasicBlock_t));
    
    // 设置ID
    if (id == 0)
    {
        block->id = g_next_basic_block_id++;
    }
    else
    {
        block->id = id;
        if (id >= g_next_basic_block_id)
        {
            g_next_basic_block_id = id + 1;
        }
    }
    
    // 设置名称
    if (name)
    {
        block->name = cn_strdup(name);
        if (!block->name)
        {
            cn_free(block);
            return NULL;
        }
    }
    else
    {
        block->name = F_generate_basic_block_name(block->id);
        if (!block->name)
        {
            cn_free(block);
            return NULL;
        }
    }
    
    // 设置类型
    block->type = type;
    
    // 初始化数组容量
    block->instruction_capacity = INITIAL_INSTRUCTION_CAPACITY;
    block->predecessor_capacity = INITIAL_PREDECESSOR_CAPACITY;
    block->successor_capacity = INITIAL_SUCCESSOR_CAPACITY;
    block->dominator_capacity = INITIAL_DOMINATOR_CAPACITY;
    block->dominated_capacity = INITIAL_DOMINATED_CAPACITY;
    
    // 分配数组
    block->instructions = (Stru_IrInstruction_t**)cn_malloc(
        sizeof(Stru_IrInstruction_t*) * block->instruction_capacity);
    block->predecessors = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * block->predecessor_capacity);
    block->successors = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * block->successor_capacity);
    block->dominators = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * block->dominator_capacity);
    block->dominated = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * block->dominated_capacity);
    
    // 检查分配是否成功
    if (!block->instructions || !block->predecessors || !block->successors ||
        !block->dominators || !block->dominated)
    {
        if (block->instructions) cn_free(block->instructions);
        if (block->predecessors) cn_free(block->predecessors);
        if (block->successors) cn_free(block->successors);
        if (block->dominators) cn_free(block->dominators);
        if (block->dominated) cn_free(block->dominated);
        if (block->name) cn_free(block->name);
        cn_free(block);
        return NULL;
    }
    
    // 初始化计数
    block->instruction_count = 0;
    block->predecessor_count = 0;
    block->successor_count = 0;
    block->dominator_count = 0;
    block->dominated_count = 0;
    
    // 初始化指针
    block->immediate_dominator = NULL;
    block->loop_header = NULL;
    block->in_set = NULL;
    block->out_set = NULL;
    block->gen_set = NULL;
    block->kill_set = NULL;
    block->internal_data = NULL;
    block->visited = false;
    
    return block;
}

void F_destroy_basic_block(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return;
    }
    
    // 释放名称
    if (block->name)
    {
        cn_free(block->name);
    }
    
    // 释放指令数组（注意：不释放指令本身，由调用者负责）
    free_instruction_array(block->instructions, block->instruction_count);
    
    // 释放基本块数组（注意：不释放基本块本身，由调用者负责）
    free_basic_block_array(block->predecessors, block->predecessor_count);
    free_basic_block_array(block->successors, block->successor_count);
    free_basic_block_array(block->dominators, block->dominator_count);
    free_basic_block_array(block->dominated, block->dominated_count);
    
    // 释放数据流集合
    if (block->in_set) cn_free(block->in_set);
    if (block->out_set) cn_free(block->out_set);
    if (block->gen_set) cn_free(block->gen_set);
    if (block->kill_set) cn_free(block->kill_set);
    
    // 释放内部数据
    if (block->internal_data) cn_free(block->internal_data);
    
    // 释放基本块结构体
    cn_free(block);
}

Stru_BasicBlock_t* F_copy_basic_block(const Stru_BasicBlock_t* src)
{
    if (!src)
    {
        return NULL;
    }
    
    // 创建新基本块
    Stru_BasicBlock_t* dst = F_create_basic_block(src->name, src->type, src->id);
    if (!dst)
    {
        return NULL;
    }
    
    // 复制指令（浅拷贝，指令本身不复制）
    for (size_t i = 0; i < src->instruction_count; i++)
    {
        if (!F_basic_block_add_instruction(dst, src->instructions[i]))
        {
            F_destroy_basic_block(dst);
            return NULL;
        }
    }
    
    // 复制调试信息
    dst->start_line = src->start_line;
    dst->end_line = src->end_line;
    dst->start_column = src->start_column;
    dst->end_column = src->end_column;
    
    // 复制循环信息
    dst->loop_depth = src->loop_depth;
    dst->loop_id = src->loop_id;
    
    // 注意：不复制前驱、后继、支配关系，这些需要在控制流图中重建
    
    return dst;
}

// ============================================================================
// 基本块操作函数实现
// ============================================================================

bool F_basic_block_add_instruction(Stru_BasicBlock_t* block, 
                                   Stru_IrInstruction_t* instruction)
{
    if (!block || !instruction)
    {
        return false;
    }
    
    // 确保容量
    if (!ensure_instruction_capacity(block))
    {
        return false;
    }
    
    // 添加指令
    block->instructions[block->instruction_count] = instruction;
    block->instruction_count++;
    
    // 更新调试信息
    if (block->instruction_count == 1)
    {
        // 第一条指令，设置起始位置
        block->start_line = instruction->line;
        block->start_column = instruction->column;
    }
    
    // 更新结束位置
    block->end_line = instruction->line;
    block->end_column = instruction->column;
    
    return true;
}

Stru_IrInstruction_t* F_basic_block_remove_instruction(Stru_BasicBlock_t* block, 
                                                       size_t index)
{
    if (!block || index >= block->instruction_count)
    {
        return NULL;
    }
    
    // 获取要移除的指令
    Stru_IrInstruction_t* instruction = block->instructions[index];
    
    // 移动后续指令
    for (size_t i = index; i < block->instruction_count - 1; i++)
    {
        block->instructions[i] = block->instructions[i + 1];
    }
    
    block->instruction_count--;
    
    // 更新调试信息
    if (block->instruction_count == 0)
    {
        // 无指令，清空调试信息
        block->start_line = 0;
        block->end_line = 0;
        block->start_column = 0;
        block->end_column = 0;
    }
    else if (index == 0)
    {
        // 移除第一条指令，更新起始位置
        block->start_line = block->instructions[0]->line;
        block->start_column = block->instructions[0]->column;
    }
    else if (index == block->instruction_count)
    {
        // 移除最后一条指令，更新结束位置
        block->end_line = block->instructions[block->instruction_count - 1]->line;
        block->end_column = block->instructions[block->instruction_count - 1]->column;
    }
    
    return instruction;
}

Stru_IrInstruction_t* F_basic_block_get_instruction(const Stru_BasicBlock_t* block, 
                                                    size_t index)
{
    if (!block || index >= block->instruction_count)
    {
        return NULL;
    }
    
    return block->instructions[index];
}

void F_basic_block_clear_instructions(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return;
    }
    
    // 清空指令数组（注意：不释放指令本身，由调用者负责）
    block->instruction_count = 0;
    
    // 清空调试信息
    block->start_line = 0;
    block->end_line = 0;
    block->start_column = 0;
    block->end_column = 0;
}

// ============================================================================
// 控制流关系函数实现
// ============================================================================

bool F_basic_block_add_predecessor(Stru_BasicBlock_t* block, 
                                   Stru_BasicBlock_t* predecessor)
{
    if (!block || !predecessor)
    {
        return false;
    }
    
    // 检查是否已经是前驱
    if (F_basic_block_is_predecessor(block, predecessor))
    {
        return true; // 已经是前驱，返回成功
    }
    
    // 确保容量
    if (!ensure_predecessor_capacity(block))
    {
        return false;
    }
    
    // 添加前驱
    block->predecessors[block->predecessor_count] = predecessor;
    block->predecessor_count++;
    
    return true;
}

bool F_basic_block_remove_predecessor(Stru_BasicBlock_t* block, 
                                      Stru_BasicBlock_t* predecessor)
{
    if (!block || !predecessor)
    {
        return false;
    }
    
    // 查找前驱索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < block->predecessor_count; i++)
    {
        if (block->predecessors[i] == predecessor)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是前驱
    }
    
    // 移动后续元素
    for (size_t i = index; i < block->predecessor_count - 1; i++)
    {
        block->predecessors[i] = block->predecessors[i + 1];
    }
    
    block->predecessor_count--;
    
    return true;
}

bool F_basic_block_add_successor(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* successor)
{
    if (!block || !successor)
    {
        return false;
    }
    
    // 检查是否已经是后继
    if (F_basic_block_is_successor(block, successor))
    {
        return true; // 已经是后继，返回成功
    }
    
    // 确保容量
    if (!ensure_successor_capacity(block))
    {
        return false;
    }
    
    // 添加后继
    block->successors[block->successor_count] = successor;
    block->successor_count++;
    
    return true;
}

bool F_basic_block_remove_successor(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* successor)
{
    if (!block || !successor)
    {
        return false;
    }
    
    // 查找后继索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < block->successor_count; i++)
    {
        if (block->successors[i] == successor)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是后继
    }
    
    // 移动后续元素
    for (size_t i = index; i < block->successor_count - 1; i++)
    {
        block->successors[i] = block->successors[i + 1];
    }
    
    block->successor_count--;
    
    return true;
}

bool F_basic_block_is_predecessor(const Stru_BasicBlock_t* block, 
                                  const Stru_BasicBlock_t* predecessor)
{
    if (!block || !predecessor)
    {
        return false;
    }
    
    for (size_t i = 0; i < block->predecessor_count; i++)
    {
        if (block->predecessors[i] == predecessor)
        {
            return true;
        }
    }
    
    return false;
}

bool F_basic_block_is_successor(const Stru_BasicBlock_t* block, 
                                const Stru_BasicBlock_t* successor)
{
    if (!block || !successor)
    {
        return false;
    }
    
    for (size_t i = 0; i < block->successor_count; i++)
    {
        if (block->successors[i] == successor)
        {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// 支配关系函数实现
// ============================================================================

bool F_basic_block_add_dominator(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* dominator)
{
    if (!block || !dominator)
    {
        return false;
    }
    
    // 检查是否已经是支配者
    for (size_t i = 0; i < block->dominator_count; i++)
    {
        if (block->dominators[i] == dominator)
        {
            return true; // 已经是支配者，返回成功
        }
    }
    
    // 确保容量
    if (!ensure_dominator_capacity(block))
    {
        return false;
    }
    
    // 添加支配者
    block->dominators[block->dominator_count] = dominator;
    block->dominator_count++;
    
    return true;
}

bool F_basic_block_remove_dominator(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* dominator)
{
    if (!block || !dominator)
    {
        return false;
    }
    
    // 查找支配者索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < block->dominator_count; i++)
    {
        if (block->dominators[i] == dominator)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是支配者
    }
    
    // 移动后续元素
    for (size_t i = index; i < block->dominator_count - 1; i++)
    {
        block->dominators[i] = block->dominators[i + 1];
    }
    
    block->dominator_count--;
    
    return true;
}

bool F_basic_block_add_dominated(Stru_BasicBlock_t* block, 
                                 Stru_BasicBlock_t* dominated)
{
    if (!block || !dominated)
    {
        return false;
    }
    
    // 检查是否已经被支配
    for (size_t i = 0; i < block->dominated_count; i++)
    {
        if (block->dominated[i] == dominated)
        {
            return true; // 已经被支配，返回成功
        }
    }
    
    // 确保容量
    if (!ensure_dominated_capacity(block))
    {
        return false;
    }
    
    // 添加被支配者
    block->dominated[block->dominated_count] = dominated;
    block->dominated_count++;
    
    return true;
}

bool F_basic_block_remove_dominated(Stru_BasicBlock_t* block, 
                                    Stru_BasicBlock_t* dominated)
{
    if (!block || !dominated)
    {
        return false;
    }
    
    // 查找被支配者索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < block->dominated_count; i++)
    {
        if (block->dominated[i] == dominated)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是被支配者
    }
    
    // 移动后续元素
    for (size_t i = index; i < block->dominated_count - 1; i++)
    {
        block->dominated[i] = block->dominated[i + 1];
    }
    
    block->dominated_count--;
    
    return true;
}

bool F_basic_block_dominates(const Stru_BasicBlock_t* dominator, 
                             const Stru_BasicBlock_t* dominated)
{
    if (!dominator || !dominated)
    {
        return false;
    }
    
    // 检查支配者是否在支配者集合中
    for (size_t i = 0; i < dominated->dominator_count; i++)
    {
        if (dominated->dominators[i] == dominator)
        {
            return true;
        }
    }
    
    return false;
}

bool F_basic_block_strictly_dominates(const Stru_BasicBlock_t* dominator, 
                                      const Stru_BasicBlock_t* dominated)
{
    if (!dominator || !dominated || dominator == dominated)
    {
        return false;
    }
    
    return F_basic_block_dominates(dominator, dominated);
}

// ============================================================================
// 查询函数实现
// ============================================================================

const char* F_basic_block_get_name(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return NULL;
    }
    
    return block->name;
}

bool F_basic_block_set_name(Stru_BasicBlock_t* block, const char* name)
{
    if (!block || !name)
    {
        return false;
    }
    
    // 释放旧名称
    if (block->name)
    {
        cn_free(block->name);
    }
    
    // 分配新名称
    block->name = cn_strdup(name);
    return block->name != NULL;
}

Eum_BasicBlockType F_basic_block_get_type(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return Eum_BASIC_BLOCK_NORMAL;
    }
    
    return block->type;
}

void F_basic_block_set_type(Stru_BasicBlock_t* block, Eum_BasicBlockType type)
{
    if (!block)
    {
        return;
    }
    
    block->type = type;
}

size_t F_basic_block_get_id(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return 0;
    }
    
    return block->id;
}

bool F_basic_block_is_empty(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return true;
    }
    
    return block->instruction_count == 0;
}

bool F_basic_block_is_jump_only(const Stru_BasicBlock_t* block)
{
    if (!block || block->instruction_count == 0)
    {
        return false;
    }
    
    // 检查最后一条指令是否为跳转指令
    Stru_IrInstruction_t* last_instr = block->instructions[block->instruction_count - 1];
    if (!last_instr)
    {
        return false;
    }
    
    // 检查是否为跳转或分支指令
    return (last_instr->type == Eum_IR_INSTR_BRANCH || 
            last_instr->type == Eum_IR_INSTR_JUMP ||
            last_instr->type == Eum_IR_INSTR_RETURN);
}

// ============================================================================
// 工具函数实现
// ============================================================================

const char* F_basic_block_type_to_string(Eum_BasicBlockType type)
{
    switch (type)
    {
        case Eum_BASIC_BLOCK_NORMAL:
            return "NORMAL";
        case Eum_BASIC_BLOCK_ENTRY:
            return "ENTRY";
        case Eum_BASIC_BLOCK_EXIT:
            return "EXIT";
        case Eum_BASIC_BLOCK_LOOP_HEADER:
            return "LOOP_HEADER";
        case Eum_BASIC_BLOCK_LOOP_BODY:
            return "LOOP_BODY";
        case Eum_BASIC_BLOCK_LOOP_EXIT:
            return "LOOP_EXIT";
        case Eum_BASIC_BLOCK_IF_THEN:
            return "IF_THEN";
        case Eum_BASIC_BLOCK_IF_ELSE:
            return "IF_ELSE";
        case Eum_BASIC_BLOCK_SWITCH_CASE:
            return "SWITCH_CASE";
        case Eum_BASIC_BLOCK_HANDLER:
            return "HANDLER";
        default:
            return "UNKNOWN";
    }
}

char* F_generate_basic_block_name(size_t id)
{
    // 分配足够的内存：前缀"BB_" + 数字 + 空字符
    size_t buffer_size = 32; // 足够存储"BB_4294967295\0"
    char* name = (char*)cn_malloc(buffer_size);
    if (!name)
    {
        return NULL;
    }
    
    snprintf(name, buffer_size, "BB_%zu", id);
    return name;
}

void F_basic_block_reset_visited(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return;
    }
    
    block->visited = false;
}

void F_basic_block_set_visited(Stru_BasicBlock_t* block, bool visited)
{
    if (!block)
    {
        return;
    }
    
    block->visited = visited;
}

bool F_basic_block_is_visited(const Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    return block->visited;
}

// ============================================================================
// 验证函数实现
// ============================================================================

bool F_validate_basic_block(const Stru_BasicBlock_t* block, char** error_message)
{
    if (!block)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块指针为NULL");
        }
        return false;
    }
    
    // 检查名称
    if (!block->name)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块名称为NULL");
        }
        return false;
    }
    
    // 检查ID
    if (block->id == 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块ID为0");
        }
        return false;
    }
    
    // 检查指令数组
    if (!block->instructions && block->instruction_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("指令数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查指令计数
    if (block->instruction_count > block->instruction_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("指令计数超过容量");
        }
        return false;
    }
    
    // 检查前驱数组
    if (!block->predecessors && block->predecessor_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("前驱数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查前驱计数
    if (block->predecessor_count > block->predecessor_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("前驱计数超过容量");
        }
        return false;
    }
    
    // 检查后继数组
    if (!block->successors && block->successor_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("后继数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查后继计数
    if (block->successor_count > block->successor_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("后继计数超过容量");
        }
        return false;
    }
    
    // 检查支配者数组
    if (!block->dominators && block->dominator_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("支配者数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查支配者计数
    if (block->dominator_count > block->dominator_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("支配者计数超过容量");
        }
        return false;
    }
    
    // 检查被支配者数组
    if (!block->dominated && block->dominated_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("被支配者数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查被支配者计数
    if (block->dominated_count > block->dominated_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("被支配者计数超过容量");
        }
        return false;
    }
    
    // 检查指令指针
    for (size_t i = 0; i < block->instruction_count; i++)
    {
        if (!block->instructions[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("指令数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查前驱指针
    for (size_t i = 0; i < block->predecessor_count; i++)
    {
        if (!block->predecessors[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("前驱数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查后继指针
    for (size_t i = 0; i < block->successor_count; i++)
    {
        if (!block->successors[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("后继数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查支配者指针
    for (size_t i = 0; i < block->dominator_count; i++)
    {
        if (!block->dominators[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("支配者数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查被支配者指针
    for (size_t i = 0; i < block->dominated_count; i++)
    {
        if (!block->dominated[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("被支配者数组中存在NULL指针");
            }
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// 静态函数实现
// ============================================================================

static bool ensure_instruction_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->instruction_count < block->instruction_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->instruction_capacity * 2;
    if (new_capacity < INITIAL_INSTRUCTION_CAPACITY)
    {
        new_capacity = INITIAL_INSTRUCTION_CAPACITY;
    }
    
    // 重新分配数组
    Stru_IrInstruction_t** new_array = (Stru_IrInstruction_t**)cn_realloc(
        block->instructions, sizeof(Stru_IrInstruction_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->instructions = new_array;
    block->instruction_capacity = new_capacity;
    
    return true;
}

static bool ensure_predecessor_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->predecessor_count < block->predecessor_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->predecessor_capacity * 2;
    if (new_capacity < INITIAL_PREDECESSOR_CAPACITY)
    {
        new_capacity = INITIAL_PREDECESSOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->predecessors, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->predecessors = new_array;
    block->predecessor_capacity = new_capacity;
    
    return true;
}

static bool ensure_successor_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->successor_count < block->successor_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->successor_capacity * 2;
    if (new_capacity < INITIAL_SUCCESSOR_CAPACITY)
    {
        new_capacity = INITIAL_SUCCESSOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->successors, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->successors = new_array;
    block->successor_capacity = new_capacity;
    
    return true;
}

static bool ensure_dominator_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->dominator_count < block->dominator_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->dominator_capacity * 2;
    if (new_capacity < INITIAL_DOMINATOR_CAPACITY)
    {
        new_capacity = INITIAL_DOMINATOR_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->dominators, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->dominators = new_array;
    block->dominator_capacity = new_capacity;
    
    return true;
}

static bool ensure_dominated_capacity(Stru_BasicBlock_t* block)
{
    if (!block)
    {
        return false;
    }
    
    if (block->dominated_count < block->dominated_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = block->dominated_capacity * 2;
    if (new_capacity < INITIAL_DOMINATED_CAPACITY)
    {
        new_capacity = INITIAL_DOMINATED_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        block->dominated, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    block->dominated = new_array;
    block->dominated_capacity = new_capacity;
    
    return true;
}

static void free_instruction_array(Stru_IrInstruction_t** instructions, size_t count)
{
    if (!instructions)
    {
        return;
    }
    
    // 注意：不释放指令本身，由调用者负责
    cn_free(instructions);
}

static void free_basic_block_array(Stru_BasicBlock_t** blocks, size_t count)
{
    if (!blocks)
    {
        return;
    }
    
    // 注意：不释放基本块本身，由调用者负责
    cn_free(blocks);
}
