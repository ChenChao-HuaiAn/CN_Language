/**
 * @file CN_token_interface.h
 * @brief CN_Language 令牌模块抽象接口定义
 * 
 * 定义令牌模块的抽象接口，遵循项目架构规范。
 * 提供统一的接口访问令牌功能，支持依赖注入和模块替换。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#ifndef CN_TOKEN_INTERFACE_H
#define CN_TOKEN_INTERFACE_H

#include "CN_token_types.h"
#include <stddef.h>
#include <stdbool.h>

// 前向声明令牌结构体
typedef struct Stru_Token_t Stru_Token_t;

/**
 * @brief 令牌接口结构体
 * 
 * 定义令牌模块的抽象接口，所有令牌操作通过此接口进行。
 * 遵循接口隔离原则，为不同客户端提供专用接口。
 */
typedef struct Stru_TokenInterface_t
{
    // ============================================
    // 令牌生命周期管理
    // ============================================
    
    /**
     * @brief 创建令牌
     * 
     * @param type 令牌类型
     * @param lexeme 词素字符串（会被复制）
     * @param line 行号
     * @param column 列号
     * @return Stru_Token_t* 新创建的令牌指针，失败返回NULL
     */
    Stru_Token_t* (*create_token)(Eum_TokenType type, const char* lexeme, size_t line, size_t column);
    
    /**
     * @brief 销毁令牌
     * 
     * @param token 要销毁的令牌指针
     */
    void (*destroy_token)(Stru_Token_t* token);
    
    /**
     * @brief 复制令牌
     * 
     * @param token 要复制的令牌指针
     * @return Stru_Token_t* 新复制的令牌指针
     */
    Stru_Token_t* (*copy_token)(const Stru_Token_t* token);
    
    // ============================================
    // 令牌属性访问
    // ============================================
    
    /**
     * @brief 获取令牌类型
     * 
     * @param token 令牌指针
     * @return Eum_TokenType 令牌类型
     */
    Eum_TokenType (*get_token_type)(const Stru_Token_t* token);
    
    /**
     * @brief 获取令牌词素
     * 
     * @param token 令牌指针
     * @return const char* 词素字符串
     */
    const char* (*get_token_lexeme)(const Stru_Token_t* token);
    
    /**
     * @brief 获取令牌位置
     * 
     * @param token 令牌指针
     * @param line 输出行号
     * @param column 输出列号
     */
    void (*get_token_position)(const Stru_Token_t* token, size_t* line, size_t* column);
    
    // ============================================
    // 字面量值操作
    // ============================================
    
    /**
     * @brief 设置整数字面量值
     * 
     * @param token 令牌指针
     * @param value 整数值
     */
    void (*set_int_value)(Stru_Token_t* token, long value);
    
    /**
     * @brief 设置浮点数字面量值
     * 
     * @param token 令牌指针
     * @param value 浮点数值
     */
    void (*set_float_value)(Stru_Token_t* token, double value);
    
    /**
     * @brief 设置布尔字面量值
     * 
     * @param token 令牌指针
     * @param value 布尔值
     */
    void (*set_bool_value)(Stru_Token_t* token, bool value);
    
    /**
     * @brief 获取整数字面量值
     * 
     * @param token 令牌指针
     * @return long 整数值
     */
    long (*get_int_value)(const Stru_Token_t* token);
    
    /**
     * @brief 获取浮点数字面量值
     * 
     * @param token 令牌指针
     * @return double 浮点数值
     */
    double (*get_float_value)(const Stru_Token_t* token);
    
    /**
     * @brief 获取布尔字面量值
     * 
     * @param token 令牌指针
     * @return bool 布尔值
     */
    bool (*get_bool_value)(const Stru_Token_t* token);
    
    // ============================================
    // 令牌类型查询
    // ============================================
    
    /**
     * @brief 判断是否为关键字令牌
     * 
     * @param type 令牌类型
     * @return true 是关键字
     * @return false 不是关键字
     */
    bool (*is_keyword)(Eum_TokenType type);
    
    /**
     * @brief 判断是否为运算符令牌
     * 
     * @param type 令牌类型
     * @return true 是运算符
     * @return false 不是运算符
     */
    bool (*is_operator)(Eum_TokenType type);
    
    /**
     * @brief 判断是否为字面量令牌
     * 
     * @param type 令牌类型
     * @return true 是字面量
     * @return false 不是字面量
     */
    bool (*is_literal)(Eum_TokenType type);
    
    /**
     * @brief 判断是否为分隔符令牌
     * 
     * @param type 令牌类型
     * @return true 是分隔符
     * @return false 不是分隔符
     */
    bool (*is_delimiter)(Eum_TokenType type);
    
    // ============================================
    // 关键字信息查询
    // ============================================
    
    /**
     * @brief 获取关键字优先级
     * 
     * @param type 令牌类型
     * @return int 优先级值（1-10），0表示非运算符关键字
     */
    int (*get_precedence)(Eum_TokenType type);
    
    /**
     * @brief 获取关键字的中文表示
     * 
     * @param type 令牌类型
     * @return const char* 中文关键字字符串
     */
    const char* (*get_chinese_keyword)(Eum_TokenType type);
    
    /**
     * @brief 获取关键字的英文表示
     * 
     * @param type 令牌类型
     * @return const char* 英文关键字字符串
     */
    const char* (*get_english_keyword)(Eum_TokenType type);
    
    /**
     * @brief 获取关键字的分类
     * 
     * @param type 令牌类型
     * @return int 分类编号（1-9），0表示非关键字
     */
    int (*get_keyword_category)(Eum_TokenType type);
    
    // ============================================
    // 工具函数
    // ============================================
    
    /**
     * @brief 令牌类型转字符串
     * 
     * @param type 令牌类型
     * @return const char* 类型字符串表示
     */
    const char* (*type_to_string)(Eum_TokenType type);
    
    /**
     * @brief 比较两个令牌
     * 
     * @param token1 第一个令牌
     * @param token2 第二个令牌
     * @return true 令牌相同
     * @return false 令牌不同
     */
    bool (*equals)(const Stru_Token_t* token1, const Stru_Token_t* token2);
    
    /**
     * @brief 打印令牌信息
     * 
     * @param token 令牌指针
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return int 写入的字符数（不包括终止空字符）
     */
    int (*to_string)(const Stru_Token_t* token, char* buffer, size_t buffer_size);
    
    // ============================================
    // 模块生命周期管理
    // ============================================
    
    /**
     * @brief 初始化令牌模块
     * 
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(void);
    
    /**
     * @brief 清理令牌模块
     */
    void (*cleanup)(void);
    
} Stru_TokenInterface_t;

/**
 * @brief 获取默认令牌接口实例
 * 
 * 返回项目默认实现的令牌接口实例。
 * 客户端可以通过此函数获取接口，然后通过接口调用功能。
 * 
 * @return const Stru_TokenInterface_t* 令牌接口实例指针
 */
const Stru_TokenInterface_t* F_get_token_interface(void);

#endif // CN_TOKEN_INTERFACE_H
