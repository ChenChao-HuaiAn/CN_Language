/**
 * @file CN_token_interface_impl.c
 * @brief CN_Language 令牌接口实现模块
 * 
 * 实现令牌模块的抽象接口，提供统一的接口访问令牌功能。
 * 遵循依赖倒置原则，高层模块定义接口，低层模块实现接口。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 */

#include "../CN_token_interface.h"
#include "../lifecycle/CN_token_lifecycle.h"
#include "../values/CN_token_values.h"
#include "../query/CN_token_query.h"
#include "../keywords/CN_token_keywords.h"
#include "../tools/CN_token_tools.h"

// ============================================
// 令牌生命周期管理函数
// ============================================

static Stru_Token_t* interface_create_token(Eum_TokenType type, const char* lexeme, size_t line, size_t column)
{
    return F_token_lifecycle_create(type, lexeme, line, column);
}

static void interface_destroy_token(Stru_Token_t* token)
{
    F_token_lifecycle_destroy(token);
}

static Stru_Token_t* interface_copy_token(const Stru_Token_t* token)
{
    return F_token_lifecycle_copy(token);
}

static Eum_TokenType interface_get_token_type(const Stru_Token_t* token)
{
    return F_token_lifecycle_get_type(token);
}

static const char* interface_get_token_lexeme(const Stru_Token_t* token)
{
    return F_token_lifecycle_get_lexeme(token);
}

static void interface_get_token_position(const Stru_Token_t* token, size_t* line, size_t* column)
{
    F_token_lifecycle_get_position(token, line, column);
}

// ============================================
// 字面量值操作函数
// ============================================

static void interface_set_int_value(Stru_Token_t* token, long value)
{
    F_token_values_set_int(token, value);
}

static void interface_set_float_value(Stru_Token_t* token, double value)
{
    F_token_values_set_float(token, value);
}

static void interface_set_bool_value(Stru_Token_t* token, bool value)
{
    F_token_values_set_bool(token, value);
}

static long interface_get_int_value(const Stru_Token_t* token)
{
    return F_token_values_get_int(token);
}

static double interface_get_float_value(const Stru_Token_t* token)
{
    return F_token_values_get_float(token);
}

static bool interface_get_bool_value(const Stru_Token_t* token)
{
    return F_token_values_get_bool(token);
}

// ============================================
// 令牌类型查询函数
// ============================================

static bool interface_is_keyword(Eum_TokenType type)
{
    return F_token_query_is_keyword(type);
}

static bool interface_is_operator(Eum_TokenType type)
{
    return F_token_query_is_operator(type);
}

static bool interface_is_literal(Eum_TokenType type)
{
    return F_token_query_is_literal(type);
}

static bool interface_is_delimiter(Eum_TokenType type)
{
    return F_token_query_is_delimiter(type);
}

// ============================================
// 关键字信息查询函数
// ============================================

static int interface_get_precedence(Eum_TokenType type)
{
    return F_token_query_get_precedence(type);
}

static const char* interface_get_chinese_keyword(Eum_TokenType type)
{
    return F_token_keywords_get_chinese(type);
}

static const char* interface_get_english_keyword(Eum_TokenType type)
{
    return F_token_keywords_get_english(type);
}

static int interface_get_keyword_category(Eum_TokenType type)
{
    return F_token_keywords_get_category(type);
}

// ============================================
// 工具函数
// ============================================

static const char* interface_type_to_string(Eum_TokenType type)
{
    return F_token_tools_type_to_string(type);
}

static bool interface_equals(const Stru_Token_t* token1, const Stru_Token_t* token2)
{
    return F_token_lifecycle_equals(token1, token2);
}

static int interface_to_string(const Stru_Token_t* token, char* buffer, size_t buffer_size)
{
    return F_token_tools_to_string(token, buffer, buffer_size);
}

// ============================================
// 模块生命周期管理函数
// ============================================

static bool interface_initialize(void)
{
    // 令牌模块初始化，目前没有需要初始化的资源
    return true;
}

static void interface_cleanup(void)
{
    // 令牌模块清理，目前没有需要清理的资源
}

// ============================================
// 默认令牌接口实例
// ============================================

static const Stru_TokenInterface_t g_default_token_interface = {
    // 令牌生命周期管理
    .create_token = interface_create_token,
    .destroy_token = interface_destroy_token,
    .copy_token = interface_copy_token,
    .get_token_type = interface_get_token_type,
    .get_token_lexeme = interface_get_token_lexeme,
    .get_token_position = interface_get_token_position,
    
    // 字面量值操作
    .set_int_value = interface_set_int_value,
    .set_float_value = interface_set_float_value,
    .set_bool_value = interface_set_bool_value,
    .get_int_value = interface_get_int_value,
    .get_float_value = interface_get_float_value,
    .get_bool_value = interface_get_bool_value,
    
    // 令牌类型查询
    .is_keyword = interface_is_keyword,
    .is_operator = interface_is_operator,
    .is_literal = interface_is_literal,
    .is_delimiter = interface_is_delimiter,
    
    // 关键字信息查询
    .get_precedence = interface_get_precedence,
    .get_chinese_keyword = interface_get_chinese_keyword,
    .get_english_keyword = interface_get_english_keyword,
    .get_keyword_category = interface_get_keyword_category,
    
    // 工具函数
    .type_to_string = interface_type_to_string,
    .equals = interface_equals,
    .to_string = interface_to_string,
    
    // 模块生命周期管理
    .initialize = interface_initialize,
    .cleanup = interface_cleanup
};

// ============================================
// 公共接口函数
// ============================================

/**
 * @brief 获取默认令牌接口实例
 * 
 * 返回项目默认实现的令牌接口实例。
 * 客户端可以通过此函数获取接口，然后通过接口调用功能。
 * 
 * @return const Stru_TokenInterface_t* 令牌接口实例指针
 */
const Stru_TokenInterface_t* F_get_token_interface(void)
{
    return &g_default_token_interface;
}
