/******************************************************************************
 * 文件名: CN_cfg_control_flow_graph.c
 * 功能: CN_Language 控制流图数据结构实现
 * 
 * 实现控制流图数据结构，包括基本块集合、入口出口块、
 * 支配树、循环信息等。
 * 
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_cfg_control_flow_graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../../infrastructure/memory/CN_memory_interface.h"

// ============================================================================
// 常量定义
// ============================================================================

#define INITIAL_BLOCK_CAPACITY 16
#define INITIAL_LOOP_HEADER_CAPACITY 8

// ============================================================================
// 静态变量
// ============================================================================

static size_t g_next_cfg_id = 1;

// ============================================================================
// 静态函数声明
// ============================================================================

static char* cn_strdup(const char* str);
static bool ensure_block_capacity(Stru_ControlFlowGraph_t* cfg);
static bool ensure_loop_header_capacity(Stru_ControlFlowGraph_t* cfg);
static void free_basic_block_array(Stru_BasicBlock_t** blocks, size_t count);
static void free_postorder_array(Stru_BasicBlock_t** order, size_t count);
static void free_loop_header_array(Stru_BasicBlock_t** headers, size_t count);

// ============================================================================
// 静态函数实现
// ============================================================================

/**
 * @brief 复制字符串
 * 
 * 复制字符串到新分配的内存中。
 * 
 * @param str 源字符串
 * @return char* 新分配的字符串，失败返回NULL
 */
static char* cn_strdup(const char* str)
{
    if (!str)
    {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)cn_malloc(len);
    if (!copy)
    {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}

/**
 * @brief 确保基本块数组容量
 * 
 * 确保控制流图的基本块数组有足够容量。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
static bool ensure_block_capacity(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return false;
    }
    
    if (cfg->block_count < cfg->block_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = cfg->block_capacity * 2;
    if (new_capacity < INITIAL_BLOCK_CAPACITY)
    {
        new_capacity = INITIAL_BLOCK_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        cfg->blocks, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    cfg->blocks = new_array;
    cfg->block_capacity = new_capacity;
    
    return true;
}

/**
 * @brief 确保循环头数组容量
 * 
 * 确保控制流图的循环头数组有足够容量。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
static bool ensure_loop_header_capacity(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return false;
    }
    
    if (cfg->loop_header_count < cfg->loop_header_capacity)
    {
        return true; // 容量足够
    }
    
    // 计算新容量
    size_t new_capacity = cfg->loop_header_capacity * 2;
    if (new_capacity < INITIAL_LOOP_HEADER_CAPACITY)
    {
        new_capacity = INITIAL_LOOP_HEADER_CAPACITY;
    }
    
    // 重新分配数组
    Stru_BasicBlock_t** new_array = (Stru_BasicBlock_t**)cn_realloc(
        cfg->loop_headers, sizeof(Stru_BasicBlock_t*) * new_capacity);
    
    if (!new_array)
    {
        return false;
    }
    
    cfg->loop_headers = new_array;
    cfg->loop_header_capacity = new_capacity;
    
    return true;
}

/**
 * @brief 释放基本块数组
 * 
 * 释放基本块数组内存（不释放基本块本身）。
 * 
 * @param blocks 基本块数组
 * @param count 数组元素数量
 */
static void free_basic_block_array(Stru_BasicBlock_t** blocks, size_t count)
{
    if (!blocks)
    {
        return;
    }
    
    // 注意：不释放基本块本身，由调用者负责
    cn_free(blocks);
}

/**
 * @brief 释放后序遍历数组
 * 
 * 释放后序遍历数组内存（不释放基本块本身）。
 * 
 * @param order 后序遍历数组
 * @param count 数组元素数量
 */
static void free_postorder_array(Stru_BasicBlock_t** order, size_t count)
{
    if (!order)
    {
        return;
    }
    
    // 注意：不释放基本块本身，由调用者负责
    cn_free(order);
}

/**
 * @brief 释放循环头数组
 * 
 * 释放循环头数组内存（不释放基本块本身）。
 * 
 * @param headers 循环头数组
 * @param count 数组元素数量
 */
static void free_loop_header_array(Stru_BasicBlock_t** headers, size_t count)
{
    if (!headers)
    {
        return;
    }
    
    // 注意：不释放基本块本身，由调用者负责
    cn_free(headers);
}

// ============================================================================
// 工厂函数实现
// ============================================================================

Stru_ControlFlowGraph_t* F_create_control_flow_graph(const char* name, size_t id)
{
    // 分配控制流图结构体
    Stru_ControlFlowGraph_t* cfg = (Stru_ControlFlowGraph_t*)cn_malloc(sizeof(Stru_ControlFlowGraph_t));
    if (!cfg)
    {
        return NULL;
    }
    
    // 初始化基本信息
    memset(cfg, 0, sizeof(Stru_ControlFlowGraph_t));
    
    // 设置ID
    if (id == 0)
    {
        cfg->id = g_next_cfg_id++;
    }
    else
    {
        cfg->id = id;
        if (id >= g_next_cfg_id)
        {
            g_next_cfg_id = id + 1;
        }
    }
    
    // 设置名称
    if (name)
    {
        cfg->name = cn_strdup(name);
        if (!cfg->name)
        {
            cn_free(cfg);
            return NULL;
        }
    }
    else
    {
        cfg->name = F_generate_cfg_name(cfg->id);
        if (!cfg->name)
        {
            cn_free(cfg);
            return NULL;
        }
    }
    
    // 初始化数组容量
    cfg->block_capacity = INITIAL_BLOCK_CAPACITY;
    cfg->loop_header_capacity = INITIAL_LOOP_HEADER_CAPACITY;
    
    // 分配数组
    cfg->blocks = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * cfg->block_capacity);
    cfg->loop_headers = (Stru_BasicBlock_t**)cn_malloc(
        sizeof(Stru_BasicBlock_t*) * cfg->loop_header_capacity);
    
    // 检查分配是否成功
    if (!cfg->blocks || !cfg->loop_headers)
    {
        if (cfg->blocks) cn_free(cfg->blocks);
        if (cfg->loop_headers) cn_free(cfg->loop_headers);
        if (cfg->name) cn_free(cfg->name);
        cn_free(cfg);
        return NULL;
    }
    
    // 初始化计数
    cfg->block_count = 0;
    cfg->loop_header_count = 0;
    cfg->postorder_count = 0;
    cfg->reverse_postorder_count = 0;
    
    // 初始化指针
    cfg->entry_block = NULL;
    cfg->exit_block = NULL;
    cfg->postorder = NULL;
    cfg->reverse_postorder = NULL;
    
    // 初始化数据流分析信息
    cfg->reaching_definitions = NULL;
    cfg->live_variables = NULL;
    cfg->available_expressions = NULL;
    
    // 初始化统计信息
    cfg->max_loop_depth = 0;
    cfg->total_instructions = 0;
    cfg->total_basic_blocks = 0;
    
    // 初始化内部状态
    cfg->internal_data = NULL;
    cfg->analyzed = false;
    
    return cfg;
}

void F_destroy_control_flow_graph(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return;
    }
    
    // 释放名称
    if (cfg->name)
    {
        cn_free(cfg->name);
    }
    
    // 释放基本块数组（注意：不释放基本块本身，由调用者负责）
    free_basic_block_array(cfg->blocks, cfg->block_count);
    
    // 释放后序遍历数组
    free_postorder_array(cfg->postorder, cfg->postorder_count);
    free_postorder_array(cfg->reverse_postorder, cfg->reverse_postorder_count);
    
    // 释放循环头数组
    free_loop_header_array(cfg->loop_headers, cfg->loop_header_count);
    
    // 释放数据流分析信息
    if (cfg->reaching_definitions) cn_free(cfg->reaching_definitions);
    if (cfg->live_variables) cn_free(cfg->live_variables);
    if (cfg->available_expressions) cn_free(cfg->available_expressions);
    
    // 释放内部数据
    if (cfg->internal_data) cn_free(cfg->internal_data);
    
    // 释放控制流图结构体
    cn_free(cfg);
}

Stru_ControlFlowGraph_t* F_copy_control_flow_graph(const Stru_ControlFlowGraph_t* src)
{
    if (!src)
    {
        return NULL;
    }
    
    // 创建新控制流图
    Stru_ControlFlowGraph_t* dst = F_create_control_flow_graph(src->name, src->id);
    if (!dst)
    {
        return NULL;
    }
    
    // 复制基本块（浅拷贝，基本块本身不复制）
    for (size_t i = 0; i < src->block_count; i++)
    {
        if (!F_cfg_add_basic_block(dst, src->blocks[i]))
        {
            F_destroy_control_flow_graph(dst);
            return NULL;
        }
    }
    
    // 复制入口出口块
    if (src->entry_block)
    {
        dst->entry_block = src->entry_block;
    }
    
    if (src->exit_block)
    {
        dst->exit_block = src->exit_block;
    }
    
    // 复制统计信息
    dst->max_loop_depth = src->max_loop_depth;
    dst->total_instructions = src->total_instructions;
    dst->total_basic_blocks = src->total_basic_blocks;
    
    // 复制分析状态
    dst->analyzed = src->analyzed;
    
    // 注意：不复制后序遍历、循环头、数据流分析信息，这些需要重新计算
    
    return dst;
}

// ============================================================================
// 基本块管理函数实现
// ============================================================================

bool F_cfg_add_basic_block(Stru_ControlFlowGraph_t* cfg, Stru_BasicBlock_t* block)
{
    if (!cfg || !block)
    {
        return false;
    }
    
    // 检查是否已经存在
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] == block)
        {
            return true; // 已经存在，返回成功
        }
    }
    
    // 确保容量
    if (!ensure_block_capacity(cfg))
    {
        return false;
    }
    
    // 添加基本块
    cfg->blocks[cfg->block_count] = block;
    cfg->block_count++;
    
    // 更新统计信息
    cfg->total_basic_blocks = cfg->block_count;
    
    return true;
}

Stru_BasicBlock_t* F_cfg_remove_basic_block(Stru_ControlFlowGraph_t* cfg, 
                                            Stru_BasicBlock_t* block)
{
    if (!cfg || !block)
    {
        return NULL;
    }
    
    // 查找基本块索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] == block)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return NULL; // 不存在
    }
    
    // 如果是入口块，清空入口块
    if (cfg->entry_block == block)
    {
        cfg->entry_block = NULL;
    }
    
    // 如果是出口块，清空出口块
    if (cfg->exit_block == block)
    {
        cfg->exit_block = NULL;
    }
    
    // 从循环头数组中移除
    for (size_t i = 0; i < cfg->loop_header_count; i++)
    {
        if (cfg->loop_headers[i] == block)
        {
            // 移动后续元素
            for (size_t j = i; j < cfg->loop_header_count - 1; j++)
            {
                cfg->loop_headers[j] = cfg->loop_headers[j + 1];
            }
            cfg->loop_header_count--;
            break;
        }
    }
    
    // 移动后续元素
    for (size_t i = index; i < cfg->block_count - 1; i++)
    {
        cfg->blocks[i] = cfg->blocks[i + 1];
    }
    
    cfg->block_count--;
    
    // 更新统计信息
    cfg->total_basic_blocks = cfg->block_count;
    
    return block;
}

Stru_BasicBlock_t* F_cfg_get_basic_block(const Stru_ControlFlowGraph_t* cfg, 
                                         size_t index)
{
    if (!cfg || index >= cfg->block_count)
    {
        return NULL;
    }
    
    return cfg->blocks[index];
}

Stru_BasicBlock_t* F_cfg_find_basic_block_by_id(const Stru_ControlFlowGraph_t* cfg, 
                                                size_t id)
{
    if (!cfg)
    {
        return NULL;
    }
    
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] && cfg->blocks[i]->id == id)
        {
            return cfg->blocks[i];
        }
    }
    
    return NULL;
}

Stru_BasicBlock_t* F_cfg_find_basic_block_by_name(const Stru_ControlFlowGraph_t* cfg, 
                                                  const char* name)
{
    if (!cfg || !name)
    {
        return NULL;
    }
    
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] && cfg->blocks[i]->name && 
            strcmp(cfg->blocks[i]->name, name) == 0)
        {
            return cfg->blocks[i];
        }
    }
    
    return NULL;
}

void F_cfg_clear_basic_blocks(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return;
    }
    
    // 清空基本块数组（注意：不释放基本块本身，由调用者负责）
    cfg->block_count = 0;
    
    // 清空入口出口块
    cfg->entry_block = NULL;
    cfg->exit_block = NULL;
    
    // 清空循环头数组
    cfg->loop_header_count = 0;
    
    // 清空后序遍历数组
    if (cfg->postorder)
    {
        free_postorder_array(cfg->postorder, cfg->postorder_count);
        cfg->postorder = NULL;
        cfg->postorder_count = 0;
    }
    
    if (cfg->reverse_postorder)
    {
        free_postorder_array(cfg->reverse_postorder, cfg->reverse_postorder_count);
        cfg->reverse_postorder = NULL;
        cfg->reverse_postorder_count = 0;
    }
    
    // 更新统计信息
    cfg->total_basic_blocks = 0;
    cfg->total_instructions = 0;
    cfg->max_loop_depth = 0;
}

// ============================================================================
// 入口出口块管理函数实现
// ============================================================================

bool F_cfg_set_entry_block(Stru_ControlFlowGraph_t* cfg, 
                           Stru_BasicBlock_t* entry_block)
{
    if (!cfg || !entry_block)
    {
        return false;
    }
    
    // 检查基本块是否在控制流图中
    bool found = false;
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] == entry_block)
        {
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        // 如果不在控制流图中，先添加
        if (!F_cfg_add_basic_block(cfg, entry_block))
        {
            return false;
        }
    }
    
    cfg->entry_block = entry_block;
    return true;
}

/**
 * @brief 设置出口基本块
 * 
 * 设置控制流图的出口基本块。
 * 
 * @param cfg 控制流图
 * @param exit_block 出口基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_exit_block(Stru_ControlFlowGraph_t* cfg, 
                          Stru_BasicBlock_t* exit_block)
{
    if (!cfg || !exit_block)
    {
        return false;
    }
    
    // 检查基本块是否在控制流图中
    bool found = false;
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i] == exit_block)
        {
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        // 如果不在控制流图中，先添加
        if (!F_cfg_add_basic_block(cfg, exit_block))
        {
            return false;
        }
    }
    
    cfg->exit_block = exit_block;
    return true;
}

/**
 * @brief 获取入口基本块
 * 
 * 获取控制流图的入口基本块。
 * 
 * @param cfg 控制流图
 * @return Stru_BasicBlock_t* 入口基本块
 */
Stru_BasicBlock_t* F_cfg_get_entry_block(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->entry_block;
}

/**
 * @brief 获取出口基本块
 * 
 * 获取控制流图的出口基本块。
 * 
 * @param cfg 控制流图
 * @return Stru_BasicBlock_t* 出口基本块
 */
Stru_BasicBlock_t* F_cfg_get_exit_block(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->exit_block;
}

// ============================================================================
// 遍历顺序管理函数实现
// ============================================================================

/**
 * @brief 设置后序遍历顺序
 * 
 * 设置控制流图的后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param order 后序遍历顺序数组
 * @param count 数组元素数量
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_postorder(Stru_ControlFlowGraph_t* cfg, 
                         Stru_BasicBlock_t** order, size_t count)
{
    if (!cfg || !order)
    {
        return false;
    }
    
    // 释放旧的后序遍历数组
    if (cfg->postorder)
    {
        free_postorder_array(cfg->postorder, cfg->postorder_count);
    }
    
    // 分配新的后序遍历数组
    cfg->postorder = (Stru_BasicBlock_t**)cn_malloc(sizeof(Stru_BasicBlock_t*) * count);
    if (!cfg->postorder)
    {
        return false;
    }
    
    // 复制后序遍历顺序
    for (size_t i = 0; i < count; i++)
    {
        cfg->postorder[i] = order[i];
    }
    
    cfg->postorder_count = count;
    return true;
}

/**
 * @brief 设置逆后序遍历顺序
 * 
 * 设置控制流图的逆后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param order 逆后序遍历顺序数组
 * @param count 数组元素数量
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_reverse_postorder(Stru_ControlFlowGraph_t* cfg, 
                                 Stru_BasicBlock_t** order, size_t count)
{
    if (!cfg || !order)
    {
        return false;
    }
    
    // 释放旧的逆后序遍历数组
    if (cfg->reverse_postorder)
    {
        free_postorder_array(cfg->reverse_postorder, cfg->reverse_postorder_count);
    }
    
    // 分配新的逆后序遍历数组
    cfg->reverse_postorder = (Stru_BasicBlock_t**)cn_malloc(sizeof(Stru_BasicBlock_t*) * count);
    if (!cfg->reverse_postorder)
    {
        return false;
    }
    
    // 复制逆后序遍历顺序
    for (size_t i = 0; i < count; i++)
    {
        cfg->reverse_postorder[i] = order[i];
    }
    
    cfg->reverse_postorder_count = count;
    return true;
}

/**
 * @brief 获取后序遍历顺序
 * 
 * 获取控制流图的后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 后序遍历顺序数组
 */
Stru_BasicBlock_t** F_cfg_get_postorder(const Stru_ControlFlowGraph_t* cfg, 
                                        size_t* count)
{
    if (!cfg || !count)
    {
        return NULL;
    }
    
    *count = cfg->postorder_count;
    return cfg->postorder;
}

/**
 * @brief 获取逆后序遍历顺序
 * 
 * 获取控制流图的逆后序遍历顺序。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 逆后序遍历顺序数组
 */
Stru_BasicBlock_t** F_cfg_get_reverse_postorder(const Stru_ControlFlowGraph_t* cfg, 
                                                size_t* count)
{
    if (!cfg || !count)
    {
        return NULL;
    }
    
    *count = cfg->reverse_postorder_count;
    return cfg->reverse_postorder;
}

// ============================================================================
// 循环信息管理函数实现
// ============================================================================

/**
 * @brief 添加循环头
 * 
 * 向控制流图添加循环头。
 * 
 * @param cfg 控制流图
 * @param loop_header 循环头基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_add_loop_header(Stru_ControlFlowGraph_t* cfg, 
                           Stru_BasicBlock_t* loop_header)
{
    if (!cfg || !loop_header)
    {
        return false;
    }
    
    // 检查是否已经是循环头
    for (size_t i = 0; i < cfg->loop_header_count; i++)
    {
        if (cfg->loop_headers[i] == loop_header)
        {
            return true; // 已经是循环头，返回成功
        }
    }
    
    // 确保容量
    if (!ensure_loop_header_capacity(cfg))
    {
        return false;
    }
    
    // 添加循环头
    cfg->loop_headers[cfg->loop_header_count] = loop_header;
    cfg->loop_header_count++;
    
    return true;
}

/**
 * @brief 移除循环头
 * 
 * 从控制流图移除循环头。
 * 
 * @param cfg 控制流图
 * @param loop_header 循环头基本块
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_remove_loop_header(Stru_ControlFlowGraph_t* cfg, 
                              Stru_BasicBlock_t* loop_header)
{
    if (!cfg || !loop_header)
    {
        return false;
    }
    
    // 查找循环头索引
    size_t index = 0;
    bool found = false;
    for (size_t i = 0; i < cfg->loop_header_count; i++)
    {
        if (cfg->loop_headers[i] == loop_header)
        {
            index = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return false; // 不是循环头
    }
    
    // 移动后续元素
    for (size_t i = index; i < cfg->loop_header_count - 1; i++)
    {
        cfg->loop_headers[i] = cfg->loop_headers[i + 1];
    }
    
    cfg->loop_header_count--;
    
    return true;
}

/**
 * @brief 获取循环头数组
 * 
 * 获取控制流图的循环头数组。
 * 
 * @param cfg 控制流图
 * @param count 输出参数，数组元素数量
 * @return Stru_BasicBlock_t** 循环头数组
 */
Stru_BasicBlock_t** F_cfg_get_loop_headers(const Stru_ControlFlowGraph_t* cfg, 
                                           size_t* count)
{
    if (!cfg || !count)
    {
        return NULL;
    }
    
    *count = cfg->loop_header_count;
    return cfg->loop_headers;
}

// ============================================================================
// 数据流分析信息管理函数实现
// ============================================================================

/**
 * @brief 设置到达定义分析结果
 * 
 * 设置控制流图的到达定义分析结果。
 * 
 * @param cfg 控制流图
 * @param reaching_definitions 到达定义分析结果
 */
void F_cfg_set_reaching_definitions(Stru_ControlFlowGraph_t* cfg, 
                                    void* reaching_definitions)
{
    if (!cfg)
    {
        return;
    }
    
    // 释放旧的到达定义分析结果
    if (cfg->reaching_definitions)
    {
        cn_free(cfg->reaching_definitions);
    }
    
    cfg->reaching_definitions = reaching_definitions;
}

/**
 * @brief 设置活跃变量分析结果
 * 
 * 设置控制流图的活跃变量分析结果。
 * 
 * @param cfg 控制流图
 * @param live_variables 活跃变量分析结果
 */
void F_cfg_set_live_variables(Stru_ControlFlowGraph_t* cfg, 
                              void* live_variables)
{
    if (!cfg)
    {
        return;
    }
    
    // 释放旧的活跃变量分析结果
    if (cfg->live_variables)
    {
        cn_free(cfg->live_variables);
    }
    
    cfg->live_variables = live_variables;
}

/**
 * @brief 设置可用表达式分析结果
 * 
 * 设置控制流图的可用表达式分析结果。
 * 
 * @param cfg 控制流图
 * @param available_expressions 可用表达式分析结果
 */
void F_cfg_set_available_expressions(Stru_ControlFlowGraph_t* cfg, 
                                     void* available_expressions)
{
    if (!cfg)
    {
        return;
    }
    
    // 释放旧的可用表达式分析结果
    if (cfg->available_expressions)
    {
        cn_free(cfg->available_expressions);
    }
    
    cfg->available_expressions = available_expressions;
}

/**
 * @brief 获取到达定义分析结果
 * 
 * 获取控制流图的到达定义分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 到达定义分析结果
 */
void* F_cfg_get_reaching_definitions(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->reaching_definitions;
}

/**
 * @brief 获取活跃变量分析结果
 * 
 * 获取控制流图的活跃变量分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 活跃变量分析结果
 */
void* F_cfg_get_live_variables(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->live_variables;
}

/**
 * @brief 获取可用表达式分析结果
 * 
 * 获取控制流图的可用表达式分析结果。
 * 
 * @param cfg 控制流图
 * @return void* 可用表达式分析结果
 */
void* F_cfg_get_available_expressions(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->available_expressions;
}

// ============================================================================
// 查询函数实现
// ============================================================================

/**
 * @brief 获取控制流图名称
 * 
 * 获取控制流图的名称。
 * 
 * @param cfg 控制流图
 * @return const char* 控制流图名称
 */
const char* F_cfg_get_name(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return NULL;
    }
    
    return cfg->name;
}

/**
 * @brief 设置控制流图名称
 * 
 * 设置控制流图的名称。
 * 
 * @param cfg 控制流图
 * @param name 新名称
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_set_name(Stru_ControlFlowGraph_t* cfg, const char* name)
{
    if (!cfg || !name)
    {
        return false;
    }
    
    // 释放旧名称
    if (cfg->name)
    {
        cn_free(cfg->name);
    }
    
    // 分配新名称
    cfg->name = cn_strdup(name);
    return cfg->name != NULL;
}

/**
 * @brief 获取控制流图ID
 * 
 * 获取控制流图的ID。
 * 
 * @param cfg 控制流图
 * @return size_t 控制流图ID
 */
size_t F_cfg_get_id(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return 0;
    }
    
    return cfg->id;
}

/**
 * @brief 检查控制流图是否为空
 * 
 * 检查控制流图是否为空（无基本块）。
 * 
 * @param cfg 控制流图
 * @return bool 为空返回true，否则返回false
 */
bool F_cfg_is_empty(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return true;
    }
    
    return cfg->block_count == 0;
}

/**
 * @brief 检查控制流图是否已分析
 * 
 * 检查控制流图是否已分析。
 * 
 * @param cfg 控制流图
 * @return bool 已分析返回true，否则返回false
 */
bool F_cfg_is_analyzed(const Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return false;
    }
    
    return cfg->analyzed;
}

/**
 * @brief 设置控制流图分析状态
 * 
 * 设置控制流图的分析状态。
 * 
 * @param cfg 控制流图
 * @param analyzed 分析状态
 */
void F_cfg_set_analyzed(Stru_ControlFlowGraph_t* cfg, bool analyzed)
{
    if (!cfg)
    {
        return;
    }
    
    cfg->analyzed = analyzed;
}

/**
 * @brief 获取控制流图统计信息
 * 
 * 获取控制流图的统计信息。
 * 
 * @param cfg 控制流图
 * @param max_loop_depth 输出参数，最大循环嵌套深度
 * @param total_instructions 输出参数，总指令数
 * @param total_basic_blocks 输出参数，总基本块数
 */
void F_cfg_get_statistics(const Stru_ControlFlowGraph_t* cfg,
                          size_t* max_loop_depth,
                          size_t* total_instructions,
                          size_t* total_basic_blocks)
{
    if (!cfg || !max_loop_depth || !total_instructions || !total_basic_blocks)
    {
        return;
    }
    
    *max_loop_depth = cfg->max_loop_depth;
    *total_instructions = cfg->total_instructions;
    *total_basic_blocks = cfg->total_basic_blocks;
}

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief 生成控制流图名称
 * 
 * 根据控制流图ID生成默认名称。
 * 
 * @param id 控制流图ID
 * @return char* 生成的名称，调用者负责释放
 */
char* F_generate_cfg_name(size_t id)
{
    // 分配足够的内存：前缀"CFG_" + 数字 + 空字符
    size_t buffer_size = 32; // 足够存储"CFG_4294967295\0"
    char* name = (char*)cn_malloc(buffer_size);
    if (!name)
    {
        return NULL;
    }
    
    snprintf(name, buffer_size, "CFG_%zu", id);
    return name;
}

/**
 * @brief 验证控制流图
 * 
 * 验证控制流图的完整性。
 * 
 * @param cfg 控制流图
 * @param error_message 输出参数，错误信息
 * @return bool 有效返回true，无效返回false
 */
bool F_validate_control_flow_graph(const Stru_ControlFlowGraph_t* cfg, 
                                   char** error_message)
{
    if (!cfg)
    {
        if (error_message)
        {
            *error_message = cn_strdup("控制流图指针为NULL");
        }
        return false;
    }
    
    // 检查名称
    if (!cfg->name)
    {
        if (error_message)
        {
            *error_message = cn_strdup("控制流图名称为NULL");
        }
        return false;
    }
    
    // 检查ID
    if (cfg->id == 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("控制流图ID为0");
        }
        return false;
    }
    
    // 检查基本块数组
    if (!cfg->blocks && cfg->block_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查基本块计数
    if (cfg->block_count > cfg->block_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("基本块计数超过容量");
        }
        return false;
    }
    
    // 检查循环头数组
    if (!cfg->loop_headers && cfg->loop_header_capacity > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("循环头数组为NULL但容量大于0");
        }
        return false;
    }
    
    // 检查循环头计数
    if (cfg->loop_header_count > cfg->loop_header_capacity)
    {
        if (error_message)
        {
            *error_message = cn_strdup("循环头计数超过容量");
        }
        return false;
    }
    
    // 检查后序遍历数组
    if (!cfg->postorder && cfg->postorder_count > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("后序遍历数组为NULL但计数大于0");
        }
        return false;
    }
    
    // 检查逆后序遍历数组
    if (!cfg->reverse_postorder && cfg->reverse_postorder_count > 0)
    {
        if (error_message)
        {
            *error_message = cn_strdup("逆后序遍历数组为NULL但计数大于0");
        }
        return false;
    }
    
    // 检查基本块指针
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (!cfg->blocks[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("基本块数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查循环头指针
    for (size_t i = 0; i < cfg->loop_header_count; i++)
    {
        if (!cfg->loop_headers[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("循环头数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查后序遍历指针
    for (size_t i = 0; i < cfg->postorder_count; i++)
    {
        if (!cfg->postorder[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("后序遍历数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查逆后序遍历指针
    for (size_t i = 0; i < cfg->reverse_postorder_count; i++)
    {
        if (!cfg->reverse_postorder[i])
        {
            if (error_message)
            {
                *error_message = cn_strdup("逆后序遍历数组中存在NULL指针");
            }
            return false;
        }
    }
    
    // 检查入口块是否在基本块数组中
    if (cfg->entry_block)
    {
        bool found = false;
        for (size_t i = 0; i < cfg->block_count; i++)
        {
            if (cfg->blocks[i] == cfg->entry_block)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            if (error_message)
            {
                *error_message = cn_strdup("入口块不在基本块数组中");
            }
            return false;
        }
    }
    
    // 检查出口块是否在基本块数组中
    if (cfg->exit_block)
    {
        bool found = false;
        for (size_t i = 0; i < cfg->block_count; i++)
        {
            if (cfg->blocks[i] == cfg->exit_block)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            if (error_message)
            {
                *error_message = cn_strdup("出口块不在基本块数组中");
            }
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 重置控制流图访问标记
 * 
 * 重置控制流图中所有基本块的访问标记。
 * 
 * @param cfg 控制流图
 */
void F_cfg_reset_visited_marks(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return;
    }
    
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i])
        {
            F_basic_block_reset_visited(cfg->blocks[i]);
        }
    }
}

/**
 * @brief 计算控制流图统计信息
 * 
 * 计算控制流图的统计信息并更新内部状态。
 * 
 * @param cfg 控制流图
 * @return bool 成功返回true，失败返回false
 */
bool F_cfg_compute_statistics(Stru_ControlFlowGraph_t* cfg)
{
    if (!cfg)
    {
        return false;
    }
    
    // 重置统计信息
    cfg->total_instructions = 0;
    cfg->max_loop_depth = 0;
    
    // 计算总指令数和最大循环深度
    for (size_t i = 0; i < cfg->block_count; i++)
    {
        if (cfg->blocks[i])
        {
            // 累加指令数
            cfg->total_instructions += cfg->blocks[i]->instruction_count;
            
            // 更新最大循环深度
            if (cfg->blocks[i]->loop_depth > cfg->max_loop_depth)
            {
                cfg->max_loop_depth = cfg->blocks[i]->loop_depth;
            }
        }
    }
    
    // 更新总基本块数
    cfg->total_basic_blocks = cfg->block_count;
    
    return true;
}
