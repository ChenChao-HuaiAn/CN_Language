/******************************************************************************
 * 文件名: CN_error_chain.h
 * 功能: CN_Language错误链支持
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现错误链支持
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ERROR_CHAIN_H
#define CN_ERROR_CHAIN_H

#include "CN_error_context.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 错误链节点结构体
 * 
 * 表示错误链中的一个节点，包含错误上下文和指向下一个节点的指针。
 */
typedef struct Stru_CN_ErrorChainNode_t
{
    /** 错误上下文 */
    Stru_CN_ErrorContext_t context;
    
    /** 下一个错误节点 */
    struct Stru_CN_ErrorChainNode_t* next;
    
    /** 前一个错误节点 */
    struct Stru_CN_ErrorChainNode_t* prev;
    
} Stru_CN_ErrorChainNode_t;

/**
 * @brief 错误链结构体
 * 
 * 管理一系列相关的错误，支持错误链的构建和遍历。
 */
typedef struct Stru_CN_ErrorChain_t
{
    /** 链头节点 */
    Stru_CN_ErrorChainNode_t* head;
    
    /** 链尾节点 */
    Stru_CN_ErrorChainNode_t* tail;
    
    /** 节点数量 */
    size_t count;
    
    /** 最大节点数量（0表示无限制） */
    size_t max_nodes;
    
    /** 是否自动释放节点内存 */
    bool auto_free;
    
} Stru_CN_ErrorChain_t;

/**
 * @brief 创建错误链
 * 
 * @param max_nodes 最大节点数量（0表示无限制）
 * @param auto_free 是否自动释放节点内存
 * @return 错误链结构体
 */
Stru_CN_ErrorChain_t CN_error_chain_create(size_t max_nodes, bool auto_free);

/**
 * @brief 销毁错误链
 * 
 * @param chain 错误链
 */
void CN_error_chain_destroy(Stru_CN_ErrorChain_t* chain);

/**
 * @brief 清空错误链
 * 
 * @param chain 错误链
 */
void CN_error_chain_clear(Stru_CN_ErrorChain_t* chain);

/**
 * @brief 添加错误到错误链
 * 
 * @param chain 错误链
 * @param context 错误上下文
 * @return true 如果成功添加，false 如果失败
 */
bool CN_error_chain_add(
    Stru_CN_ErrorChain_t* chain,
    const Stru_CN_ErrorContext_t* context);

/**
 * @brief 从错误链创建并添加错误
 * 
 * @param chain 错误链
 * @param error_code 错误码
 * @param message 错误消息
 * @param filename 文件名
 * @param line 行号
 * @param column 列号
 * @param function 函数名
 * @param module 模块名
 * @return true 如果成功添加，false 如果失败
 */
bool CN_error_chain_add_new(
    Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code,
    const char* message,
    const char* filename,
    size_t line,
    size_t column,
    const char* function,
    const char* module);

/**
 * @brief 获取错误链中的第一个错误
 * 
 * @param chain 错误链
 * @return 第一个错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_first(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 获取错误链中的最后一个错误
 * 
 * @param chain 错误链
 * @return 最后一个错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_last(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 获取错误链中的错误数量
 * 
 * @param chain 错误链
 * @return 错误数量
 */
size_t CN_error_chain_get_count(const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 检查错误链是否为空
 * 
 * @param chain 错误链
 * @return true 如果为空，false 如果不为空
 */
bool CN_error_chain_is_empty(const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 检查错误链是否已满
 * 
 * @param chain 错误链
 * @return true 如果已满，false 如果未满
 */
bool CN_error_chain_is_full(const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 遍历错误链
 * 
 * @param chain 错误链
 * @param callback 回调函数，接收错误上下文和用户数据
 * @param user_data 用户数据
 */
void CN_error_chain_foreach(
    const Stru_CN_ErrorChain_t* chain,
    void (*callback)(const Stru_CN_ErrorContext_t* context, void* user_data),
    void* user_data);

/**
 * @brief 查找特定错误码的错误
 * 
 * @param chain 错误链
 * @param error_code 要查找的错误码
 * @return 找到的第一个错误的上下文，如果未找到则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_find(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code);

/**
 * @brief 检查错误链中是否包含特定错误码
 * 
 * @param chain 错误链
 * @param error_code 要检查的错误码
 * @return true 如果包含，false 如果不包含
 */
bool CN_error_chain_contains(
    const Stru_CN_ErrorChain_t* chain,
    Eum_CN_ErrorCode_t error_code);

/**
 * @brief 合并两个错误链
 * 
 * @param dest 目标错误链
 * @param src 源错误链
 * @return true 如果成功合并，false 如果失败
 */
bool CN_error_chain_merge(
    Stru_CN_ErrorChain_t* dest,
    const Stru_CN_ErrorChain_t* src);

/**
 * @brief 格式化错误链为字符串
 * 
 * @param chain 错误链
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param separator 错误之间的分隔符
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_chain_format(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size,
    const char* separator);

/**
 * @brief 获取错误链中最严重的错误
 * 
 * @param chain 错误链
 * @return 最严重的错误的上下文，如果链为空则返回NULL
 */
const Stru_CN_ErrorContext_t* CN_error_chain_get_most_severe(
    const Stru_CN_ErrorChain_t* chain);

/**
 * @brief 从错误链创建错误摘要
 * 
 * @param chain 错误链
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止符）
 */
size_t CN_error_chain_create_summary(
    const Stru_CN_ErrorChain_t* chain,
    char* buffer,
    size_t buffer_size);

#endif // CN_ERROR_CHAIN_H
