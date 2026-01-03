/******************************************************************************
 * 文件名: CN_error_chain.c
 * 功能: CN_Language错误链实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现错误链功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_error_chain.h"
#include "CN_error.h"  // 添加CN_error.h以获取CN_error_get_severity等函数声明
#include <stdlib.h>
#include <string.h>
#include <stdio.h>     // 添加stdio.h以使用snprintf

/**
 * @brief 创建错误链节点
 * 
 * @param context 错误上下文
 * @return 新创建的节点，失败返回NULL
 */
static Stru_CN_ErrorChainNode_t* create_error_chain_node(
    const Stru_CN_ErrorContext_t* context)
{
    if (!context)
    {
        return NULL;
    }
    
    Stru_CN_ErrorChainNode_t* node = 
        (Stru_CN_ErrorChainNode_t*)malloc(sizeof(Stru_CN_ErrorChainNode_t));
    if (!node)
    {
        return NULL;
    }
    
    // 复制错误上下文
    if (!CN_error_copy_context(&node->context, context))
    {
        free(node);
        return NULL;
    }
    
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * @brief 销毁错误链节点
 * 
 * @param node 要销毁的节点
 */
static void destroy_error_chain_node(Stru_CN_ErrorChainNode_t* node)
{
    if (!node)
    {
        return;
    }
    
    // 清空上下文
    CN_error_clear_context(&node->context);
    
    free(node);
}

/**
 * @brief 创建错误链
 */
Stru_CN_ErrorChain_t CN_error_chain_create(size_t max_nodes, bool auto_free)
{
    Stru_CN_ErrorChain_t chain;
    
    chain.head = NULL;
    chain.tail = NULL;
    chain.count = 0;
    chain.max_nodes = max_nodes;
    chain.auto_free = auto_free;
    
    return chain;
}

/**
 * @brief 销毁错误链
 */
void CN_error_chain_destroy(Stru_CN_ErrorChain_t* chain)
{
    if (!chain)
    {
        return;
    }
    
    CN_error_chain_clear(chain);
    
    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
}

/**
 * @brief 清空错误链
 */
void CN_error_chain_clear(Stru_CN_ErrorChain_t* chain)
{
    if (!chain)
    {
        return;
    }
    
    Stru_CN_ErrorChainNode_t* current = chain->head;
    while (current)
    {
        Stru_CN_ErrorChainNode_t* next = current->next;
        destroy_error_chain_node(current);
        current = next;
    }
    
    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
}

/**
 * @brief 添加错误到错误链
 */
bool CN_error_chain_add(
    Stru_CN_ErrorChain_t* chain,
    const Stru_CN_ErrorContext_t* context)
{
    if (!chain || !context)
    {
        return false;
    }
    
    // 检查是否已满
    if (chain->max_nodes > 0 && chain->count >= chain->max_nodes)
    {
        return false;
    }
    
    // 创建新节点
    Stru_CN_ErrorChainNode_t* node = create_error_chain_node(context);
    if (!node)
    {
        return false;
    }
    
    // 添加到链尾
    if (!chain->head)
    {
        // 链为空
        chain->head = node;
        chain->tail = node;
    }
    else
    {
        // 链不为空
        node->prev = chain->tail;
        chain->tail->next = node;
        chain->tail = node;
    }
    
    chain->count++;
    return true;
}

/**
 * @brief 从错误链创建并添加错误
 */
bool CN_error_chain_add_new(
    Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module)
{
    if (!chain)
    {
        return false;
    }
    
    Stru_CN_ErrorContext_t context = CN_error_create_context(
        error_code, message, filename, line, column, function, module);
    
    return CN_error_chain_add(chain, &context);
}

/**
 * @brief 获取错误链中的第一个错误
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_first(
    const Stru_CN_ErrorChain_t* chain)
{
    if (!chain || !chain->head)
    {
        return NULL;
    }
    
    return &chain->head->context;
}

/**
 * @brief 获取错误链中的最后一个错误
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_last(
    const Stru_CN_ErrorChain_t* chain)
{
    if (!chain || !chain->tail)
    {
        return NULL;
    }
    
    return &chain->tail->context;
}

/**
 * @brief 获取错误链中的错误数量
 */
size_t CN_error_chain_get_count(const Stru_CN_ErrorChain_t* chain)
{
    if (!chain)
    {
        return 0;
    }
    
    return chain->count;
}

/**
 * @brief 检查错误链是否为空
 */
bool CN_error_chain_is_empty(const Stru_CN_ErrorChain_t* chain)
{
    if (!chain)
    {
        return true;
    }
    
    return chain->count == 0;
}

/**
 * @brief 检查错误链是否已满
 */
bool CN_error_chain_is_full(const Stru_CN_ErrorChain_t* chain)
{
    if (!chain)
    {
        return false;
    }
    
    if (chain->max_nodes == 0)
    {
        return false; // 无限制
    }
    
    return chain->count >= chain->max_nodes;
}

/**
 * @brief 遍历错误链
 */
void CN_error_chain_foreach(
    const Stru_CN_ErrorChain_t* chain,
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data)
{
    if (!chain || !callback)
    {
        return;
    }
    
    Stru_CN_ErrorChainNode_t* current = chain->head;
    while (current)
    {
        callback(&current->context, user_data);
        current = current->next;
    }
}

/**
 * @brief 查找特定错误码的错误
 */
const Stru_CN_ErrorContext_t* CN_error_chain_find(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code)
{
    if (!chain)
    {
        return NULL;
    }
    
    Stru_CN_ErrorChainNode_t* current = chain->head;
    while (current)
    {
        if (current->context.error_code == error_code)
        {
            return &current->context;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 检查错误链中是否包含特定错误码
 */
bool CN_error_chain_contains(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code)
{
    return CN_error_chain_find(chain, error_code) != NULL;
}

/**
 * @brief 合并两个错误链
 */
bool CN_error_chain_merge(
    Stru_CN_ErrorChain_t* dest,
    const Stru_CN_ErrorChain_t* src)
{
    if (!dest || !src)
    {
        return false;
    }
    
    // 遍历源链的所有节点
    Stru_CN_ErrorChainNode_t* current = src->head;
    while (current)
    {
        if (!CN_error_chain_add(dest, &current->context))
        {
            return false; // 添加失败
        }
        current = current->next;
    }
    
    return true;
}

/**
 * @brief 格式化错误链为字符串
 */
size_t CN_error_chain_format(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size,
    const char* separator)
{
    if (!chain || !buffer || buffer_size == 0)
    {
        return 0;
    }
    
    const char* sep = separator ? separator : "\n";
    size_t sep_len = strlen(sep);
    size_t total_written = 0;
    size_t remaining = buffer_size;
    
    // 遍历所有节点
    Stru_CN_ErrorChainNode_t* current = chain->head;
    bool first = true;
    
    while (current && remaining > 1)
    {
        if (!first)
        {
            // 添加分隔符
            size_t to_copy = sep_len < remaining ? sep_len : remaining - 1;
            strncpy(buffer + total_written, sep, to_copy);
            total_written += to_copy;
            remaining -= to_copy;
        }
        
        // 格式化当前错误
        size_t written = CN_error_format_context(
            &current->context, 
            buffer + total_written, 
            remaining);
        
        if (written > 0)
        {
            total_written += written;
            remaining -= written;
        }
        
        first = false;
        current = current->next;
    }
    
    // 确保字符串以null结尾
    if (total_written < buffer_size)
    {
        buffer[total_written] = '\0';
    }
    else
    {
        buffer[buffer_size - 1] = '\0';
    }
    
    return total_written;
}

/**
 * @brief 获取错误链中最严重的错误
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_most_severe(
    const Stru_CN_ErrorChain_t* chain)
{
    if (!chain || !chain->head)
    {
        return NULL;
    }
    
    Stru_CN_ErrorChainNode_t* current = chain->head;
    const Stru_CN_ErrorChainNode_t* most_severe = current;
    int max_severity = CN_error_get_severity(current->context.error_code);
    
    current = current->next;
    while (current)
    {
        int severity = CN_error_get_severity(current->context.error_code);
        if (severity > max_severity)
        {
            max_severity = severity;
            most_severe = current;
        }
        current = current->next;
    }
    
    return &most_severe->context;
}

/**
 * @brief 从错误链创建错误摘要
 */
size_t CN_error_chain_create_summary(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size)
{
    if (!chain || !buffer || buffer_size == 0)
    {
        return 0;
    }
    
    size_t total_errors = chain->count;
    if (total_errors == 0)
    {
        const char* msg = "无错误";
        size_t len = strlen(msg);
        size_t to_copy = len < buffer_size ? len : buffer_size - 1;
        strncpy(buffer, msg, to_copy);
        buffer[to_copy] = '\0';
        return to_copy;
    }
    
    // 统计各类错误
    size_t fatal_count = 0;
    size_t error_count = 0;
    size_t warning_count = 0;
    size_t info_count = 0;
    
    Stru_CN_ErrorChainNode_t* current = chain->head;
    while (current)
    {
        int severity = CN_error_get_severity(current->context.error_code);
        if (severity >= 8)
            fatal_count++;
        else if (severity >= 5)
            error_count++;
        else if (severity >= 3)
            warning_count++;
        else
            info_count++;
        
        current = current->next;
    }
    
    // 创建摘要
    int written = snprintf(buffer, buffer_size,
        "错误摘要: 总共 %lu 个错误 (致命: %lu, 错误: %lu, 警告: %lu, 信息: %lu)",
        (unsigned long)total_errors, 
        (unsigned long)fatal_count, 
        (unsigned long)error_count, 
        (unsigned long)warning_count, 
        (unsigned long)info_count);
    
    if (written < 0)
    {
        return 0;
    }
    
    if ((size_t)written >= buffer_size)
    {
        return buffer_size - 1;
    }
    
    return (size_t)written;
}
