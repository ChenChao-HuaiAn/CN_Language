/**
 * @file CN_lexer_interface.h
 * @brief CN_Language 词法分析器抽象接口
 * 
 * 定义词法分析器的抽象接口，支持所有70个中文关键字的识别。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_LEXER_INTERFACE_H
#define CN_LEXER_INTERFACE_H

#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../token/CN_token.h"

/**
 * @brief 词法分析器抽象接口结构体
 * 
 * 定义词法分析器的完整接口，包含初始化、令牌化、状态查询和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_LexerInterface_t {
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化词法分析器
     * 
     * 设置源代码和源文件名，准备进行词法分析。
     * 
     * @param interface 词法分析器接口指针
     * @param source 源代码字符串
     * @param length 源代码长度
     * @param source_name 源文件名（可选）
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_LexerInterface_t* interface, const char* source, size_t length, const char* source_name);
    
    // ============================================
    // 核心功能
    // ============================================
    
    /**
     * @brief 获取下一个令牌
     * 
     * 从源代码中读取并返回下一个令牌。
     * 调用者负责销毁返回的令牌。
     * 
     * @param interface 词法分析器接口指针
     * @return Stru_Token_t* 下一个令牌，NULL表示错误或结束
     */
    Stru_Token_t* (*next_token)(struct Stru_LexerInterface_t* interface);
    
    /**
     * @brief 检查是否还有更多令牌
     * 
     * 检查是否还有未处理的令牌。
     * 
     * @param interface 词法分析器接口指针
     * @return true 还有更多令牌
     * @return false 没有更多令牌
     */
    bool (*has_more_tokens)(struct Stru_LexerInterface_t* interface);
    
    /**
     * @brief 批量令牌化
     * 
     * 将整个源代码一次性令牌化，返回所有令牌的数组。
     * 调用者负责销毁返回的数组和其中的令牌。
     * 
     * @param interface 词法分析器接口指针
     * @return Stru_DynamicArray_t* 令牌数组，NULL表示错误
     */
    Stru_DynamicArray_t* (*tokenize_all)(struct Stru_LexerInterface_t* interface);
    
    // ============================================
    // 状态查询
    // ============================================
    
    /**
     * @brief 获取当前位置
     * 
     * 获取当前词法分析的位置（行号和列号）。
     * 
     * @param interface 词法分析器接口指针
     * @param line 输出参数：行号（从1开始）
     * @param column 输出参数：列号（从1开始）
     */
    void (*get_position)(struct Stru_LexerInterface_t* interface, size_t* line, size_t* column);
    
    /**
     * @brief 获取源文件名
     * 
     * 获取当前正在分析的源文件名。
     * 
     * @param interface 词法分析器接口指针
     * @return const char* 源文件名
     */
    const char* (*get_source_name)(struct Stru_LexerInterface_t* interface);
    
    // ============================================
    // 错误处理
    // ============================================
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查词法分析过程中是否发生了错误。
     * 
     * @param interface 词法分析器接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_LexerInterface_t* interface);
    
    /**
     * @brief 获取最后一个错误信息
     * 
     * 获取最后一个错误的详细描述。
     * 
     * @param interface 词法分析器接口指针
     * @return const char* 错误信息字符串
     */
    const char* (*get_last_error)(struct Stru_LexerInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置词法分析器
     * 
     * 重置词法分析器到初始状态，可以重新分析相同的源代码。
     * 
     * @param interface 词法分析器接口指针
     */
    void (*reset)(struct Stru_LexerInterface_t* interface);
    
    /**
     * @brief 销毁词法分析器
     * 
     * 释放词法分析器占用的所有资源。
     * 
     * @param interface 词法分析器接口指针
     */
    void (*destroy)(struct Stru_LexerInterface_t* interface);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_LexerInterface_t;

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建词法分析器接口实例
 * 
 * 创建并返回一个新的词法分析器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_LexerInterface_t* 新创建的词法分析器接口实例
 */
Stru_LexerInterface_t* F_create_lexer_interface(void);

#endif // CN_LEXER_INTERFACE_H
