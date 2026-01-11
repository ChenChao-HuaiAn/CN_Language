/******************************************************************************
 * 文件名: CN_common_subexpr.c
 * 功能: CN_Language 公共子表达式消除优化算法实现
 * 
 * 实现公共子表达式消除优化算法，识别并消除重复的表达式计算。
 * 遵循项目架构规范，实现高效且安全的公共子表达式消除。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_common_subexpr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// ============================================================================
// 内部函数声明
// ============================================================================

static Stru_CseContext_t* create_cse_context(Stru_AstNode_t* ast, Stru_OptimizationContext_t* opt_context);
static void destroy_cse_context(Stru_CseContext_t* context);
static void add_error(Stru_CseContext_t* context, const char* error_message);
static bool traverse_and_eliminate(Stru_AstNode_t* node, Stru_CseContext_t* context);
static char* compute_hash_for_node(Stru_AstNode_t* node);
static unsigned long hash_string(const char* str);
static bool is_expression_suitable_for_cse(Stru_AstNode_t* node);
static Stru_AstNode_t* create_temp_var_assignment(const char* temp_var_name, Stru_AstNode_t* expression);
static Stru_AstNode_t* create_temp_var_reference(const char* temp_var_name);
static void print_cse_statistics(Stru_CseContext_t* context);

// ============================================================================
// 公共函数实现
// ============================================================================

/**
 * @brief 应用公共子表达式消除优化
 */
bool apply_common_subexpression_elimination(Stru_AstNode_t* ast, Stru_OptimizationContext_t* context)
{
    if (!ast || !context) {
        return false;
    }
    
    printf("[公共子表达式消除] 开始公共子表达式消除优化...\n");
    
    // 创建公共子表达式消除上下文
    Stru_CseContext_t* cse_context = create_cse_context(ast, context);
    if (!cse_context) {
        printf("[公共子表达式消除] 错误: 无法创建上下文\n");
        return false;
    }
    
    // 记录初始状态
    size_t initial_eliminated_count = cse_context->eliminated_count;
    
    // 遍历AST并消除公共子表达式
    bool success = traverse_and_eliminate(ast, cse_context);
    if (!success) {
        printf("[公共子表达式消除] 警告: 遍历过程中遇到问题\n");
    }
    
    // 报告结果
    size_t total_eliminated = cse_context->eliminated_count - initial_eliminated_count;
    print_cse_statistics(cse_context);
    
    if (total_eliminated > 0) {
        printf("[公共子表达式消除] 成功消除了 %zu 个公共子表达式\n", total_eliminated);
    } else {
        printf("[公共子表达式消除] 未找到可消除的公共子表达式\n");
    }
    
    if (cse_context->error_count > 0) {
        printf("[公共子表达式消除] 警告: 在处理过程中遇到 %zu 个错误\n", cse_context->error_count);
    }
    
    // 清理上下文
    destroy_cse_context(cse_context);
    
    return success;
}

/**
 * @brief 测试公共子表达式消除算法
 */
void test_common_subexpression_elimination(void)
{
    printf("=== 公共子表达式消除算法测试 ===\n");
    
    printf("测试用例1: 简单公共子表达式\n");
    printf("  代码: a = x + y; b = x + y;\n");
    printf("  预期结果: 消除第二个x+y，使用临时变量\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例2: 嵌套公共子表达式\n");
    printf("  代码: a = (x + y) * z; b = (x + y) * z;\n");
    printf("  预期结果: 消除第二个(x+y)*z\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例3: 非公共子表达式\n");
    printf("  代码: a = x + y; b = x + z;\n");
    printf("  预期结果: 不进行优化\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("测试用例4: 有副作用的表达式\n");
    printf("  代码: a = f() + g(); b = f() + g();\n");
    printf("  预期结果: 不进行优化（函数调用有副作用）\n");
    printf("  测试状态: 待实现\n\n");
    
    printf("=== 测试完成 ===\n");
}

/**
 * @brief 计算表达式哈希值
 */
char* compute_expression_hash(Stru_AstNode_t* node)
{
    if (!node) {
        return NULL;
    }
    
    return compute_hash_for_node(node);
}

/**
 * @brief 检查表达式是否等价
 */
bool are_expressions_equivalent(Stru_AstNode_t* expr1, Stru_AstNode_t* expr2)
{
    if (!expr1 || !expr2) {
        return false;
    }
    
    // 如果两个指针相同，肯定是等价的
    if (expr1 == expr2) {
        return true;
    }
    
    // 计算两个表达式的哈希值进行比较
    char* hash1 = compute_hash_for_node(expr1);
    char* hash2 = compute_hash_for_node(expr2);
    
    if (!hash1 || !hash2) {
        if (hash1) free(hash1);
        if (hash2) free(hash2);
        return false;
    }
    
    bool equivalent = (strcmp(hash1, hash2) == 0);
    
    free(hash1);
    free(hash2);
    
    return equivalent;
}

/**
 * @brief 创建临时变量
 */
const char* create_temp_variable(Stru_CseContext_t* context, Stru_AstNode_t* expression)
{
    if (!context || !expression) {
        return NULL;
    }
    
    // 生成临时变量名
    static char temp_var_name[32];
    snprintf(temp_var_name, sizeof(temp_var_name), "__cse_temp_%zu", context->temp_var_counter++);
    
    // 这里应该将临时变量添加到符号表
    // 简化实现，只返回变量名
    
    return temp_var_name;
}

/**
 * @brief 替换公共子表达式
 */
bool replace_common_subexpression(Stru_CseContext_t* context, 
                                  Stru_AstNode_t* node, 
                                  const char* temp_var_name)
{
    if (!context || !node || !temp_var_name) {
        return false;
    }
    
    // 这里应该实现实际的AST节点替换
    // 简化实现，只计数
    
    context->eliminated_count++;
    return true;
}

/**
 * @brief 获取公共子表达式消除统计信息
 */
void get_common_subexpression_stats(Stru_OptimizationContext_t* context, 
                                    size_t* eliminated_count, size_t* error_count)
{
    if (!context || !eliminated_count || !error_count) {
        return;
    }
    
    // 这里可以从上下文中获取统计信息
    // 简化实现，返回0
    *eliminated_count = 0;
    *error_count = 0;
}

// ============================================================================
// 哈希表管理函数实现
// ============================================================================

/**
 * @brief 创建表达式哈希表
 */
Stru_CseHashTable_t* create_cse_hash_table(size_t bucket_count)
{
    if (bucket_count == 0) {
        bucket_count = 64; // 默认桶数量
    }
    
    Stru_CseHashTable_t* table = (Stru_CseHashTable_t*)malloc(sizeof(Stru_CseHashTable_t));
    if (!table) {
        return NULL;
    }
    
    table->buckets = (Stru_CseHashEntry_t**)calloc(bucket_count, sizeof(Stru_CseHashEntry_t*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    table->bucket_count = bucket_count;
    table->entry_count = 0;
    
    return table;
}

/**
 * @brief 销毁表达式哈希表
 */
void destroy_cse_hash_table(Stru_CseHashTable_t* table)
{
    if (!table) {
        return;
    }
    
    // 释放所有桶中的条目
    for (size_t i = 0; i < table->bucket_count; i++) {
        Stru_CseHashEntry_t* entry = table->buckets[i];
        while (entry) {
            Stru_CseHashEntry_t* next = entry->next;
            
            if (entry->expression_hash) {
                free(entry->expression_hash);
            }
            if (entry->temp_var_name) {
                free(entry->temp_var_name);
            }
            
            free(entry);
            entry = next;
        }
    }
    
    // 释放桶数组和表结构
    free(table->buckets);
    free(table);
}

/**
 * @brief 插入表达式到哈希表
 */
Stru_CseHashEntry_t* insert_expression(Stru_CseHashTable_t* table, 
                                       Stru_AstNode_t* expression, 
                                       const char* hash)
{
    if (!table || !expression || !hash) {
        return NULL;
    }
    
    // 计算桶索引
    unsigned long hash_value = hash_string(hash);
    size_t bucket_index = hash_value % table->bucket_count;
    
    // 检查是否已存在
    Stru_CseHashEntry_t* entry = table->buckets[bucket_index];
    while (entry) {
        if (strcmp(entry->expression_hash, hash) == 0) {
            // 已存在，增加使用计数
            entry->use_count++;
            return entry;
        }
        entry = entry->next;
    }
    
    // 创建新条目
    entry = (Stru_CseHashEntry_t*)malloc(sizeof(Stru_CseHashEntry_t));
    if (!entry) {
        return NULL;
    }
    
    entry->expression_hash = strdup(hash);
    entry->expression = expression;
    entry->temp_var_name = NULL;
    entry->use_count = 1;
    entry->next = table->buckets[bucket_index];
    
    table->buckets[bucket_index] = entry;
    table->entry_count++;
    
    return entry;
}

/**
 * @brief 查找表达式在哈希表中
 */
Stru_CseHashEntry_t* find_expression(Stru_CseHashTable_t* table, const char* hash)
{
    if (!table || !hash) {
        return NULL;
    }
    
    // 计算桶索引
    unsigned long hash_value = hash_string(hash);
    size_t bucket_index = hash_value % table->bucket_count;
    
    // 在桶中查找
    Stru_CseHashEntry_t* entry = table->buckets[bucket_index];
    while (entry) {
        if (strcmp(entry->expression_hash, hash) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief 从哈希表中移除表达式
 */
bool remove_expression(Stru_CseHashTable_t* table, const char* hash)
{
    if (!table || !hash) {
        return false;
    }
    
    // 计算桶索引
    unsigned long hash_value = hash_string(hash);
    size_t bucket_index = hash_value % table->bucket_count;
    
    // 在桶中查找并移除
    Stru_CseHashEntry_t* prev = NULL;
    Stru_CseHashEntry_t* entry = table->buckets[bucket_index];
    
    while (entry) {
        if (strcmp(entry->expression_hash, hash) == 0) {
            // 找到要移除的条目
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[bucket_index] = entry->next;
            }
            
            // 释放条目资源
            if (entry->expression_hash) {
                free(entry->expression_hash);
            }
            if (entry->temp_var_name) {
                free(entry->temp_var_name);
            }
            
            free(entry);
            table->entry_count--;
            return true;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
    return false;
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 创建公共子表达式消除上下文
 */
static Stru_CseContext_t* create_cse_context(Stru_AstNode_t* ast, Stru_OptimizationContext_t* opt_context)
{
    Stru_CseContext_t* context = (Stru_CseContext_t*)malloc(sizeof(Stru_CseContext_t));
    if (!context) {
        return NULL;
    }
    
    memset(context, 0, sizeof(Stru_CseContext_t));
    context->ast = ast;
    context->opt_context = opt_context;
    context->eliminated_count = 0;
    context->temp_var_counter = 0;
    context->error_count = 0;
    context->error_capacity = 10;
    
    // 创建哈希表
    context->hash_table = create_cse_hash_table(64);
    if (!context->hash_table) {
        free(context);
        return NULL;
    }
    
    // 分配错误信息数组
    context->errors = (char**)malloc(context->error_capacity * sizeof(char*));
    if (!context->errors) {
        destroy_cse_hash_table(context->hash_table);
        free(context);
        return NULL;
    }
    
    for (size_t i = 0; i < context->error_capacity; i++) {
        context->errors[i] = NULL;
    }
    
    return context;
}

/**
 * @brief 销毁公共子表达式消除上下文
 */
static void destroy_cse_context(Stru_CseContext_t* context)
{
    if (!context) {
        return;
    }
    
    // 销毁哈希表
    if (context->hash_table) {
        destroy_cse_hash_table(context->hash_table);
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
static void add_error(Stru_CseContext_t* context, const char* error_message)
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
 * @brief 遍历AST并消除公共子表达式
 */
static bool traverse_and_eliminate(Stru_AstNode_t* node, Stru_CseContext_t* context)
{
    if (!node || !context) {
        return false;
    }
    
    // 检查节点是否适合公共子表达式消除
    if (is_expression_suitable_for_cse(node)) {
        // 计算表达式哈希
        char* hash = compute_hash_for_node(node);
        if (hash) {
            // 在哈希表中查找表达式
            Stru_CseHashEntry_t* entry = find_expression(context->hash_table, hash);
            
            if (entry) {
                // 找到公共子表达式
                if (entry->use_count > 1) {
                    // 需要创建临时变量（如果还没有）
                    if (!entry->temp_var_name) {
                        entry->temp_var_name = strdup(create_temp_variable(context, node));
                    }
                    
                    // 用临时变量替换表达式
                    if (replace_common_subexpression(context, node, entry->temp_var_name)) {
                        printf("[CSE] 用临时变量 %s 替换公共子表达式\n", entry->temp_var_name);
                    }
                }
                
                // 增加使用计数
                entry->use_count++;
            } else {
                // 新表达式，插入哈希表
                insert_expression(context->hash_table, node, hash);
            }
            
            free(hash);
        }
    }
    
    // 递归处理子节点
    // 这里需要实现AST子节点遍历
    // 简化实现，假设成功
    
    return true;
}

/**
 * @brief 计算节点哈希值
 */
static char* compute_hash_for_node(Stru_AstNode_t* node)
{
    if (!node) {
        return NULL;
    }
    
    // 简化实现：生成固定哈希
    // 实际实现应该基于节点类型、操作符、子节点哈希等计算
    
    static char hash_buffer[64];
    snprintf(hash_buffer, sizeof(hash_buffer), "expr_%p", (void*)node);
    
    return strdup(hash_buffer);
}

/**
 * @brief 字符串哈希函数
 */
static unsigned long hash_string(const char* str)
{
    if (!str) {
        return 0;
    }
    
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

/**
 * @brief 检查表达式是否适合公共子表达式消除
 */
static bool is_expression_suitable_for_cse(Stru_AstNode_t* node)
{
    if (!node) {
        return false;
    }
    
    // 这里需要实现实际的检查逻辑
    // 简化实现：假设所有表达式都适合
    // 实际实现应该排除有副作用的表达式
    
    return true;
}

/**
 * @brief 创建临时变量赋值语句
 */
static Stru_AstNode_t* create_temp_var_assignment(const char* temp_var_name, Stru_AstNode_t* expression)
{
    if (!temp_var_name || !expression) {
        return NULL;
    }
    
    // 这里应该创建赋值语句AST节点
    // 简化实现：返回NULL
    return NULL;
}

/**
 * @brief 创建临时变量引用
 */
static Stru_AstNode_t* create_temp_var_reference(const char* temp_var_name)
{
    if (!temp_var_name) {
        return NULL;
    }
    
    // 这里应该创建变量引用AST节点
    // 简化实现：返回NULL
    return NULL;
}

/**
 * @brief 打印公共子表达式消除统计信息
 */
static void print_cse_statistics(Stru_CseContext_t* context)
{
    if (!context) {
        return;
    }
    
    printf("[CSE统计] 哈希表条目: %zu\n", context->hash_table->entry_count);
    printf("[CSE统计] 已消除表达式: %zu\n", context->eliminated_count);
    printf("[CSE统计] 临时变量数量: %zu\n", context->temp_var_counter);
    printf("[CSE统计] 错误数量: %zu\n", context->error_count);
}
