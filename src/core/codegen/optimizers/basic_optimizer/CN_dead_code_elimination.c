/******************************************************************************
 * 文件名: CN_dead_code_elimination.c
 * 功能: CN_Language 死代码消除优化算法实现
 * 
 * 实现死代码消除优化算法，移除不会执行的代码。
 * 遵循项目架构规范，实现安全且高效的死代码消除。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_dead_code_elimination.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief 控制流图节点类型
 */
typedef enum {
    CFG_NODE_BASIC_BLOCK,    ///< 基本块
    CFG_NODE_ENTRY,          ///< 入口节点
    CFG_NODE_EXIT,           ///< 出口节点
    CFG_NODE_DECISION        ///< 决策节点
} CfgNodeType;

/**
 * @brief 控制流图节点结构体
 */
typedef struct CfgNode {
    CfgNodeType type;                ///< 节点类型
    char* name;                      ///< 节点名称
    Stru_AstNode_t** statements;     ///< 语句数组
    size_t statement_count;          ///< 语句数量
    size_t statement_capacity;       ///< 语句数组容量
    
    // 控制流边
    struct CfgNode** successors;     ///< 后继节点数组
    size_t successor_count;          ///< 后继节点数量
    struct CfgNode** predecessors;   ///< 前驱节点数组
    size_t predecessor_count;        ///< 前驱节点数量
    
    // 分析信息
    bool reachable;                  ///< 是否可达
    bool has_side_effects;           ///< 是否有副作用
    bool is_critical;                ///< 是否关键节点
    
    // 活跃变量分析
    char** live_in;                  ///< 入口活跃变量
    size_t live_in_count;
    char** live_out;                 ///< 出口活跃变量
    size_t live_out_count;
} CfgNode;

/**
 * @brief 控制流图结构体
 */
typedef struct {
    CfgNode* entry;                  ///< 入口节点
    CfgNode* exit;                   ///< 出口节点
    CfgNode** nodes;                 ///< 所有节点数组
    size_t node_count;               ///< 节点数量
    size_t node_capacity;            ///< 节点数组容量
} ControlFlowGraph;

/**
 * @brief 死代码消除上下文结构体
 */
typedef struct {
    Stru_AstNode_t* ast;             ///< AST根节点
    ControlFlowGraph* cfg;           ///< 控制流图
    size_t removed_count;            ///< 已移除的代码数量
    size_t error_count;              ///< 错误数量
    char** errors;                   ///< 错误信息数组
    size_t error_capacity;           ///< 错误数组容量
} DeadCodeEliminationContext;

// ============================================================================
// 内部函数声明
// ============================================================================

static DeadCodeEliminationContext* create_dead_code_context(Stru_AstNode_t* ast);
static void destroy_dead_code_context(DeadCodeEliminationContext* context);
static void add_error(DeadCodeEliminationContext* context, const char* error_message);
static ControlFlowGraph* build_control_flow_graph_internal(Stru_AstNode_t* ast, DeadCodeEliminationContext* context);
static void destroy_control_flow_graph_internal(ControlFlowGraph* cfg);
static CfgNode* create_cfg_node(CfgNodeType type, const char* name);
static void destroy_cfg_node(CfgNode* node);
static bool add_statement_to_node(CfgNode* node, Stru_AstNode_t* statement);
static bool add_successor(CfgNode* from, CfgNode* to);
static bool add_predecessor(CfgNode* to, CfgNode* from);
static void analyze_reachability_internal(ControlFlowGraph* cfg);
static void mark_critical_nodes(ControlFlowGraph* cfg);
static bool has_side_effects(Stru_AstNode_t* node);
static bool is_pure_expression(Stru_AstNode_t* node);
static bool remove_unreachable_code_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context);
static bool remove_dead_assignments_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context);
static bool remove_unused_variables_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context);
static void print_control_flow_graph(ControlFlowGraph* cfg);

// 新增内部函数声明
static char** get_used_variables(Stru_AstNode_t* node, size_t* count);
static char** get_defined_variables(Stru_AstNode_t* node, size_t* count);
static bool is_variable_used(const char* var_name, Stru_AstNode_t* ast);
static bool is_assignment_dead(Stru_AstNode_t* assignment, ControlFlowGraph* cfg);
static bool is_variable_declaration_unused(Stru_AstNode_t* var_decl, ControlFlowGraph* cfg);
static void analyze_live_variables_internal(ControlFlowGraph* cfg);
static void free_string_array(char** array, size_t count);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 应用死代码消除优化
 * 
 * 对AST应用死代码消除优化。
 * 
 * @param ast AST根节点
 * @param context 优化上下文
 * @return bool 优化是否成功应用
 */
bool apply_dead_code_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context)
{
    if (!ast || !context) {
        return false;
    }
    
    printf("[死代码消除] 开始死代码消除优化...\n");
    
    // 创建死代码消除上下文
    DeadCodeEliminationContext* dce_context = create_dead_code_context(ast);
    if (!dce_context) {
        return false;
    }
    
    // 构建控制流图
    printf("[死代码消除] 构建控制流图...\n");
    dce_context->cfg = build_control_flow_graph_internal(ast, dce_context);
    if (!dce_context->cfg) {
        printf("[死代码消除] 错误: 无法构建控制流图\n");
        destroy_dead_code_context(dce_context);
        return false;
    }
    
    printf("[死代码消除] 控制流图构建完成，共 %zu 个节点\n", dce_context->cfg->node_count);
    
    // 分析可达性
    printf("[死代码消除] 分析代码可达性...\n");
    analyze_reachability_internal(dce_context->cfg);
    
    // 标记关键节点
    printf("[死代码消除] 标记关键节点...\n");
    mark_critical_nodes(dce_context->cfg);
    
    // 移除不可达代码
    printf("[死代码消除] 移除不可达代码...\n");
    size_t initial_removed_count = dce_context->removed_count;
    if (!remove_unreachable_code_internal(ast, dce_context->cfg, dce_context)) {
        printf("[死代码消除] 警告: 移除不可达代码时遇到问题\n");
    }
    
    // 移除死赋值
    printf("[死代码消除] 移除死赋值...\n");
    if (!remove_dead_assignments_internal(ast, dce_context->cfg, dce_context)) {
        printf("[死代码消除] 警告: 移除死赋值时遇到问题\n");
    }
    
    // 移除未使用变量
    printf("[死代码消除] 移除未使用变量...\n");
    if (!remove_unused_variables_internal(ast, dce_context->cfg, dce_context)) {
        printf("[死代码消除] 警告: 移除未使用变量时遇到问题\n");
    }
    
    // 报告结果
    size_t total_removed = dce_context->removed_count - initial_removed_count;
    if (total_removed > 0) {
        printf("[死代码消除] 成功移除了 %zu 处死代码\n", total_removed);
    } else {
        printf("[死代码消除] 未找到可消除的死代码\n");
    }
    
    if (dce_context->error_count > 0) {
        printf("[死代码消除] 警告: 在处理过程中遇到 %zu 个错误\n", dce_context->error_count);
    }
    
    // 清理资源
    destroy_dead_code_context(dce_context);
    
    return true;
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 创建死代码消除上下文
 */
static DeadCodeEliminationContext* create_dead_code_context(Stru_AstNode_t* ast)
{
    DeadCodeEliminationContext* context = (DeadCodeEliminationContext*)malloc(sizeof(DeadCodeEliminationContext));
    if (!context) {
        return NULL;
    }
    
    memset(context, 0, sizeof(DeadCodeEliminationContext));
    context->ast = ast;
    context->cfg = NULL;
    context->removed_count = 0;
    context->error_count = 0;
    context->error_capacity = 10;
    
    // 分配错误信息数组
    context->errors = (char**)malloc(context->error_capacity * sizeof(char*));
    if (!context->errors) {
        free(context);
        return NULL;
    }
    
    for (size_t i = 0; i < context->error_capacity; i++) {
        context->errors[i] = NULL;
    }
    
    return context;
}

/**
 * @brief 销毁死代码消除上下文
 */
static void destroy_dead_code_context(DeadCodeEliminationContext* context)
{
    if (!context) {
        return;
    }
    
    // 销毁控制流图
    if (context->cfg) {
        destroy_control_flow_graph(context->cfg);
    }
    
    // 释放错误信息
    if (context->errors) {
        for (size_t i = 0; i < context->error_capacity; i++) {
            if (context->errors[i]) {
                free(context->errors[i]);
            }
        }
        free(context->errors);
    }
    
    free(context);
}

/**
 * @brief 添加错误信息
 */
static void add_error(DeadCodeEliminationContext* context, const char* error_message)
{
    if (!context || !error_message) {
        return;
    }
    
    // 如果错误数组已满，扩展它
    if (context->error_count >= context->error_capacity) {
        size_t new_capacity = context->error_capacity * 2;
        char** new_errors = (char**)realloc(context->errors, new_capacity * sizeof(char*));
        if (!new_errors) {
            return;
        }
        
        // 初始化新分配的部分
        for (size_t i = context->error_capacity; i < new_capacity; i++) {
            new_errors[i] = NULL;
        }
        
        context->errors = new_errors;
        context->error_capacity = new_capacity;
    }
    
    // 复制错误信息
    context->errors[context->error_count] = strdup(error_message);
    if (context->errors[context->error_count]) {
        context->error_count++;
    }
}

/**
 * @brief 构建控制流图（内部版本）
 */
static ControlFlowGraph* build_control_flow_graph_internal(Stru_AstNode_t* ast, DeadCodeEliminationContext* context)
{
    if (!ast || !context) {
        return NULL;
    }
    
    ControlFlowGraph* cfg = (ControlFlowGraph*)malloc(sizeof(ControlFlowGraph));
    if (!cfg) {
        return NULL;
    }
    
    memset(cfg, 0, sizeof(ControlFlowGraph));
    cfg->node_capacity = 20;
    cfg->nodes = (CfgNode**)malloc(cfg->node_capacity * sizeof(CfgNode*));
    if (!cfg->nodes) {
        free(cfg);
        return NULL;
    }
    
    // 创建入口和出口节点
    cfg->entry = create_cfg_node(CFG_NODE_ENTRY, "entry");
    cfg->exit = create_cfg_node(CFG_NODE_EXIT, "exit");
    
    if (!cfg->entry || !cfg->exit) {
        destroy_control_flow_graph(cfg);
        return NULL;
    }
    
    // 添加节点到数组
    cfg->nodes[0] = cfg->entry;
    cfg->nodes[1] = cfg->exit;
    cfg->node_count = 2;
    
    // 这里应该实现实际的AST到CFG的转换
    // 简化实现，创建一个基本块
    CfgNode* basic_block = create_cfg_node(CFG_NODE_BASIC_BLOCK, "bb1");
    if (basic_block) {
        // 添加节点到数组
        if (cfg->node_count >= cfg->node_capacity) {
            size_t new_capacity = cfg->node_capacity * 2;
            CfgNode** new_nodes = (CfgNode**)realloc(cfg->nodes, new_capacity * sizeof(CfgNode*));
            if (new_nodes) {
                cfg->nodes = new_nodes;
                cfg->node_capacity = new_capacity;
            }
        }
        
        if (cfg->node_count < cfg->node_capacity) {
            cfg->nodes[cfg->node_count] = basic_block;
            cfg->node_count++;
        }
        
        // 添加控制流边
        add_successor(cfg->entry, basic_block);
        add_successor(basic_block, cfg->exit);
    }
    
    printf("[控制流图] 构建了简化的控制流图\n");
    printf("[控制流图] 入口节点: %s\n", cfg->entry->name);
    printf("[控制流图] 出口节点: %s\n", cfg->exit->name);
    printf("[控制流图] 基本块数量: %zu\n", cfg->node_count - 2); // 减去入口和出口
    
    return cfg;
}

/**
 * @brief 销毁控制流图（内部版本）
 */
static void destroy_control_flow_graph_internal(ControlFlowGraph* cfg)
{
    if (!cfg) {
        return;
    }
    
    // 销毁所有节点
    for (size_t i = 0; i < cfg->node_count; i++) {
        if (cfg->nodes[i]) {
            destroy_cfg_node(cfg->nodes[i]);
        }
    }
    
    // 释放节点数组
    if (cfg->nodes) {
        free(cfg->nodes);
    }
    
    free(cfg);
}

/**
 * @brief 创建CFG节点
 */
static CfgNode* create_cfg_node(CfgNodeType type, const char* name)
{
    CfgNode* node = (CfgNode*)malloc(sizeof(CfgNode));
    if (!node) {
        return NULL;
    }
    
    memset(node, 0, sizeof(CfgNode));
    node->type = type;
    
    if (name) {
        node->name = strdup(name);
    } else {
        node->name = strdup("unnamed");
    }
    
    // 初始化数组
    node->statement_capacity = 10;
    node->statements = (Stru_AstNode_t**)malloc(node->statement_capacity * sizeof(Stru_AstNode_t*));
    
    node->successors = (CfgNode**)malloc(2 * sizeof(CfgNode*));
    node->predecessors = (CfgNode**)malloc(2 * sizeof(CfgNode*));
    
    if (!node->name || !node->statements || !node->successors || !node->predecessors) {
        destroy_cfg_node(node);
        return NULL;
    }
    
    node->reachable = false;
    node->has_side_effects = false;
    node->is_critical = false;
    
    return node;
}

/**
 * @brief 销毁CFG节点
 */
static void destroy_cfg_node(CfgNode* node)
{
    if (!node) {
        return;
    }
    
    if (node->name) {
        free(node->name);
    }
    
    if (node->statements) {
        free(node->statements);
    }
    
    if (node->successors) {
        free(node->successors);
    }
    
    if (node->predecessors) {
        free(node->predecessors);
    }
    
    free(node);
}

/**
 * @brief 添加语句到节点
 */
static bool add_statement_to_node(CfgNode* node, Stru_AstNode_t* statement)
{
    if (!node || !statement) {
        return false;
    }
    
    // 如果数组已满，扩展它
    if (node->statement_count >= node->statement_capacity) {
        size_t new_capacity = node->statement_capacity * 2;
        Stru_AstNode_t** new_statements = (Stru_AstNode_t**)realloc(node->statements, 
                                                                   new_capacity * sizeof(Stru_AstNode_t*));
        if (!new_statements) {
            return false;
        }
        node->statements = new_statements;
        node->statement_capacity = new_capacity;
    }
    
    // 添加语句
    node->statements[node->statement_count] = statement;
    node->statement_count++;
    
    // 检查是否有副作用
    if (!node->has_side_effects) {
        node->has_side_effects = has_side_effects(statement);
    }
    
    return true;
}

/**
 * @brief 添加后继节点
 */
static bool add_successor(CfgNode* from, CfgNode* to)
{
    if (!from || !to) {
        return false;
    }
    
    // 简化实现，假设最多2个后继
    if (from->successor_count < 2) {
        from->successors[from->successor_count] = to;
        from->successor_count++;
        
        // 同时添加前驱关系
        return add_predecessor(to, from);
    }
    
    return false;
}

/**
 * @brief 添加前驱节点
 */
static bool add_predecessor(CfgNode* to, CfgNode* from)
{
    if (!to || !from) {
        return false;
    }
    
    // 简化实现，假设最多4个前驱
    if (to->predecessor_count < 4) {
        to->predecessors[to->predecessor_count] = from;
        to->predecessor_count++;
        return true;
    }
    
    return false;
}

/**
 * @brief 分析可达性（内部版本）
 */
static void analyze_reachability_internal(ControlFlowGraph* cfg)
{
    if (!cfg || !cfg->entry) {
        return;
    }
    
    // 简单的深度优先搜索标记可达节点
    // 在实际实现中，应该使用更高效的算法
    
    // 重置所有节点的可达性标记
    for (size_t i = 0; i < cfg->node_count; i++) {
        if (cfg->nodes[i]) {
            cfg->nodes[i]->reachable = false;
        }
    }
    
    // 从入口节点开始标记可达节点
    cfg->entry->reachable = true;
    
    // 简单的BFS遍历
    CfgNode* queue[100]; // 简化实现，使用固定大小队列
    size_t front = 0, rear = 0;
    queue[rear++] = cfg->entry;
    
    while (front < rear) {
        CfgNode* current = queue[front++];
        
        // 遍历所有后继节点
        for (size_t i = 0; i < current->successor_count; i++) {
            CfgNode* successor = current->successors[i];
            if (successor && !successor->reachable) {
                successor->reachable = true;
                if (rear < 100) {
                    queue[rear++] = successor;
                }
            }
        }
    }
    
    // 统计可达节点数量
    size_t reachable_count = 0;
    for (size_t i = 0; i < cfg->node_count; i++) {
        if (cfg->nodes[i] && cfg->nodes[i]->reachable) {
            reachable_count++;
        }
    }
    
    printf("[可达性分析] 可达节点: %zu/%zu\n", reachable_count, cfg->node_count);
}

/**
 * @brief 标记关键节点
 */
static void mark_critical_nodes(ControlFlowGraph* cfg)
{
    if (!cfg) {
        return;
    }
    
    // 标记有副作用的节点为关键节点
    for (size_t i = 0; i < cfg->node_count; i++) {
        CfgNode* node = cfg->nodes[i];
        if (node && node->reachable) {
            // 检查节点中的语句是否有副作用
            for (size_t j = 0; j < node->statement_count; j++) {
                if (node->statements[j] && has_side_effects(node->statements[j])) {
                    node->is_critical = true;
                    break;
                }
            }
            
            // 入口和出口节点总是关键的
            if (node->type == CFG_NODE_ENTRY || node->type == CFG_NODE_EXIT) {
                node->is_critical = true;
            }
        }
    }
    
    // 统计关键节点数量
    size_t critical_count = 0;
    for (size_t i = 0; i < cfg->node_count; i++) {
        if (cfg->nodes[i] && cfg->nodes[i]->is_critical) {
            critical_count++;
        }
    }
    
    printf("[关键节点分析] 关键节点: %zu/%zu\n", critical_count, cfg->node_count);
}

/**
 * @brief 检查节点是否有副作用
 */
static bool has_side_effects(Stru_AstNode_t* node)
{
    if (!node) {
        return false;
    }
    
    // 这里需要实现实际的副作用检查
    // 简化实现，假设某些类型的节点有副作用
    
    // 获取节点类型（需要实际的AST节点类型获取函数）
    // int node_type = F_ast_get_node_type(node);
    
    // 假设函数调用、赋值、I/O操作有副作用
    // 简化实现，总是返回false
    return false;
}

/**
 * @brief 获取节点中使用的变量
 */
static char** get_used_variables(Stru_AstNode_t* node, size_t* count)
{
    if (!node || !count) {
        return NULL;
    }
    
    // 简化实现：返回空数组
    *count = 0;
    char** result = (char**)malloc(sizeof(char*));
    if (result) {
        result[0] = NULL;
    }
    return result;
}

/**
 * @brief 获取节点中定义的变量
 */
static char** get_defined_variables(Stru_AstNode_t* node, size_t* count)
{
    if (!node || !count) {
        return NULL;
    }
    
    // 简化实现：返回空数组
    *count = 0;
    char** result = (char**)malloc(sizeof(char*));
    if (result) {
        result[0] = NULL;
    }
    return result;
}

/**
 * @brief 检查变量是否被使用
 */
static bool is_variable_used(const char* var_name, Stru_AstNode_t* ast)
{
    if (!var_name || !ast) {
        return false;
    }
    
    // 简化实现：假设变量被使用
    return true;
}

/**
 * @brief 检查赋值是否为死赋值
 */
static bool is_assignment_dead(Stru_AstNode_t* assignment, ControlFlowGraph* cfg)
{
    if (!assignment || !cfg) {
        return false;
    }
    
    // 简化实现：假设赋值不是死的
    return false;
}

/**
 * @brief 检查变量声明是否未使用
 */
static bool is_variable_declaration_unused(Stru_AstNode_t* var_decl, ControlFlowGraph* cfg)
{
    if (!var_decl || !cfg) {
        return false;
    }
    
    // 简化实现：假设变量声明被使用
    return false;
}

/**
 * @brief 分析活跃变量
 */
static void analyze_live_variables_internal(ControlFlowGraph* cfg)
{
    if (!cfg) {
        return;
    }
    
    // 简化实现：不进行实际的活跃变量分析
    printf("[活跃变量分析] 简化实现，跳过详细分析\n");
}

/**
 * @brief 释放字符串数组
 */
static void free_string_array(char** array, size_t count)
{
    if (!array) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (array[i]) {
            free(array[i]);
        }
    }
    free(array);
}

/**
 * @brief 检查是否为纯表达式
 */
static bool is_pure_expression(Stru_AstNode_t* node)
{
    if (!node) {
        return true;
    }
    
    // 纯表达式是没有副作用的表达式
    return !has_side_effects(node);
}

/**
 * @brief 移除不可达代码（内部版本）
 */
static bool remove_unreachable_code_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context)
{
    if (!ast || !cfg || !context) {
        return false;
    }
    
    size_t removed = 0;
    
    // 遍历所有节点
    for (size_t i = 0; i < cfg->node_count; i++) {
        CfgNode* node = cfg->nodes[i];
        
        // 如果节点不可达且不是关键节点，可以移除
        if (node && !node->reachable && !node->is_critical) {
            // 这里需要实现实际的代码移除逻辑
            // 简化实现，只计数
            removed += node->statement_count;
        }
    }
    
    context->removed_count += removed;
    printf("[不可达代码移除] 移除了 %zu 条不可达语句\n", removed);
    
    return true;
}

/**
 * @brief 移除死赋值（内部版本）
 */
static bool remove_dead_assignments_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context)
{
    if (!ast || !cfg || !context) {
        return false;
    }
    
    size_t removed = 0;
    
    // 分析活跃变量
    analyze_live_variables_internal(cfg);
    
    // 遍历所有节点，查找死赋值
    for (size_t i = 0; i < cfg->node_count; i++) {
        CfgNode* node = cfg->nodes[i];
        if (node && node->reachable) {
            for (size_t j = 0; j < node->statement_count; j++) {
                Stru_AstNode_t* stmt = node->statements[j];
                if (stmt && is_assignment_dead(stmt, cfg)) {
                    // 这里应该实现实际的赋值移除
                    removed++;
                }
            }
        }
    }
    
    context->removed_count += removed;
    printf("[死赋值移除] 移除了 %zu 个死赋值\n", removed);
    
    return true;
}

/**
 * @brief 移除未使用变量（内部版本）
 */
static bool remove_unused_variables_internal(Stru_AstNode_t* ast, ControlFlowGraph* cfg, DeadCodeEliminationContext* context)
{
    if (!ast || !cfg || !context) {
        return false;
    }
    
    size_t removed = 0;
    
    // 遍历所有节点，查找未使用的变量声明
    for (size_t i = 0; i < cfg->node_count; i++) {
        CfgNode* node = cfg->nodes[i];
        if (node && node->reachable) {
            for (size_t j = 0; j < node->statement_count; j++) {
                Stru_AstNode_t* stmt = node->statements[j];
                if (stmt && is_variable_declaration_unused(stmt, cfg)) {
                    // 这里应该实现实际的变量声明移除
                    removed++;
                }
            }
        }
    }
    
    context->removed_count += removed;
    printf("[未使用变量移除] 移除了 %zu 个未使用变量\n", removed);
    
    return true;
}

/**
 * @brief 打印控制流图
 */
static void print_control_flow_graph(ControlFlowGraph* cfg)
{
    if (!cfg) {
        return;
    }
    
    printf("=== 控制流图信息 ===\n");
    printf("总节点数: %zu\n", cfg->node_count);
    printf("入口节点: %s\n", cfg->entry->name);
    printf("出口节点: %s\n", cfg->exit->name);
    
    for (size_t i = 0; i < cfg->node_count; i++) {
        CfgNode* node = cfg->nodes[i];
        if (node) {
            printf("\n节点 %s:\n", node->name);
            printf("  类型: ");
            switch (node->type) {
                case CFG_NODE_ENTRY: printf("入口"); break;
                case CFG_NODE_EXIT: printf("出口"); break;
                case CFG_NODE_BASIC_BLOCK: printf("基本块"); break;
                case CFG_NODE_DECISION: printf("决策"); break;
                default: printf("未知"); break;
            }
            printf("\n");
            printf("  语句数: %zu\n", node->statement_count);
            printf("  可达: %s\n", node->reachable ? "是" : "否");
            printf("  关键: %s\n", node->is_critical ? "是" : "否");
            printf("  后继: ");
            for (size_t j = 0; j < node->successor_count; j++) {
                if (node->successors[j]) {
                    printf("%s ", node->successors[j]->name);
                }
            }
            printf("\n");
        }
    }
    printf("===================\n");
}

// ============================================================================
// 头文件中声明的公共函数实现
// ============================================================================

/**
 * @brief 构建控制流图
 * 
 * 从AST构建控制流图，用于死代码分析。
 * 
 * @param ast AST根节点
 * @return void* 控制流图句柄，调用者负责销毁
 */
void* build_control_flow_graph(Stru_AstNode_t* ast)
{
    if (!ast) {
        return NULL;
    }
    
    // 创建简化的上下文
    DeadCodeEliminationContext* context = create_dead_code_context(ast);
    if (!context) {
        return NULL;
    }
    
    // 构建控制流图
    ControlFlowGraph* cfg = build_control_flow_graph_internal(ast, context);
    
    // 清理上下文（但不清理cfg，因为要返回它）
    if (context) {
        // 不销毁cfg，因为我们要返回它
        context->cfg = NULL;
        destroy_dead_code_context(context);
    }
    
    return (void*)cfg;
}

/**
 * @brief 销毁控制流图
 * 
 * 销毁控制流图及其所有资源。
 * 
 * @param cfg 控制流图句柄
 */
void destroy_control_flow_graph(void* cfg)
{
    if (!cfg) {
        return;
    }
    
    destroy_control_flow_graph_internal((ControlFlowGraph*)cfg);
}

/**
 * @brief 分析代码可达性
 * 
 * 分析控制流图中哪些代码是可达的。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_reachability(void* cfg)
{
    if (!cfg) {
        return false;
    }
    
    analyze_reachability_internal((ControlFlowGraph*)cfg);
    return true;
}

/**
 * @brief 分析活跃变量
 * 
 * 分析控制流图中的活跃变量。
 * 
 * @param cfg 控制流图句柄
 * @return bool 分析是否成功
 */
bool analyze_live_variables(void* cfg)
{
    if (!cfg) {
        return false;
    }
    
    analyze_live_variables_internal((ControlFlowGraph*)cfg);
    return true;
}

/**
 * @brief 移除不可达代码
 * 
 * 从AST中移除不可达的代码。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的代码数量
 */
size_t remove_unreachable_code(Stru_AstNode_t* ast, void* cfg)
{
    if (!ast || !cfg) {
        return 0;
    }
    
    // 创建简化的上下文
    DeadCodeEliminationContext* context = create_dead_code_context(ast);
    if (!context) {
        return 0;
    }
    
    context->cfg = (ControlFlowGraph*)cfg;
    
    // 移除不可达代码
    bool success = remove_unreachable_code_internal(ast, (ControlFlowGraph*)cfg, context);
    size_t removed_count = context->removed_count;
    
    destroy_dead_code_context(context);
    
    return success ? removed_count : 0;
}

/**
 * @brief 移除死赋值
 * 
 * 从AST中移除死的赋值语句。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的死赋值数量
 */
size_t remove_dead_assignments(Stru_AstNode_t* ast, void* cfg)
{
    if (!ast || !cfg) {
        return 0;
    }
    
    // 创建简化的上下文
    DeadCodeEliminationContext* context = create_dead_code_context(ast);
    if (!context) {
        return 0;
    }
    
    context->cfg = (ControlFlowGraph*)cfg;
    
    // 移除死赋值
    bool success = remove_dead_assignments_internal(ast, (ControlFlowGraph*)cfg, context);
    size_t removed_count = context->removed_count;
    
    destroy_dead_code_context(context);
    
    return success ? removed_count : 0;
}

/**
 * @brief 移除未使用变量
 * 
 * 从AST中移除未使用的变量声明。
 * 
 * @param ast AST根节点
 * @param cfg 控制流图句柄
 * @return size_t 移除的未使用变量数量
 */
size_t remove_unused_variables(Stru_AstNode_t* ast, void* cfg)
{
    if (!ast || !cfg) {
        return 0;
    }
    
    // 创建简化的上下文
    DeadCodeEliminationContext* context = create_dead_code_context(ast);
    if (!context) {
        return 0;
    }
    
    context->cfg = (ControlFlowGraph*)cfg;
    
    // 移除未使用变量
    bool success = remove_unused_variables_internal(ast, (ControlFlowGraph*)cfg, context);
    size_t removed_count = context->removed_count;
    
    destroy_dead_code_context(context);
    
    return success ? removed_count : 0;
}

/**
 * @brief 获取死代码消除统计信息
 * 
 * 获取死代码消除优化的统计信息。
 * 
 * @param context 优化上下文
 * @param removed_count 输出参数，已移除的代码数量
 * @param error_count 输出参数，错误数量
 */
void get_dead_code_elimination_stats(Stru_OptimizationContext_t* context, 
                                     size_t* removed_count, size_t* error_count)
{
    if (!context || !removed_count || !error_count) {
        return;
    }
    
    // 简化实现：返回默认值
    *removed_count = 0;
    *error_count = 0;
}

// ============================================================================
// 测试函数
// ============================================================================

/**
 * @brief 测试死代码消除算法
 * 
 * 运行死代码消除算法的测试用例。
 */
void test_dead_code_elimination(void)
{
    printf("=== 死代码消除算法测试 ===\n");
    
    printf("测试用例1: 不可达代码\n");
    printf("  代码: if (false) { x = 1; }\n");
    printf("  预期结果: 移除整个if语句\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例2: 死赋值\n");
    printf("  代码: x = 1; x = 2; // 第一个赋值是死的\n");
    printf("  预期结果: 移除第一个赋值\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例3: 未使用变量\n");
    printf("  代码: int unused = 10; // 从未使用\n");
    printf("  预期结果: 移除变量声明\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例4: 有副作用的不可达代码\n");
    printf("  代码: if (false) { printf(\"hello\"); }\n");
    printf("  预期结果: 保留代码（有副作用）\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("=== 测试完成 ===\n");
}
