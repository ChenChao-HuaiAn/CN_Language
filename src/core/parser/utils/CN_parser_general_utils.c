/**
 * @file CN_parser_general_utils.c
 * @brief CN_Language 语法分析器通用工具模块实现
 * 
 * 语法分析器通用工具函数模块实现，提供各种通用辅助函数。
 * 包括字符串操作、错误信息格式化、位置信息获取等函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_utils.h"
#include "../interface/CN_parser_interface_impl.h"
#include "../../../infrastructure/memory/CN_memory_interface.h"
#include "../../../infrastructure/utils/CN_utils_interface.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

// ============================================
// 通用工具函数实现
// ============================================

/**
 * @brief 复制字符串
 */
char* F_copy_string(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    size_t length = strlen(str);
    char* copy = (char*)cn_malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    
    strcpy(copy, str);
    return copy;
}

/**
 * @brief 格式化错误信息
 */
char* F_format_error_message(const char* format, ...) {
    if (format == NULL) {
        return NULL;
    }
    
    // 第一次调用获取所需缓冲区大小
    va_list args;
    va_start(args, format);
    int needed_size = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    if (needed_size < 0) {
        return NULL;
    }
    
    // 分配缓冲区
    char* buffer = (char*)cn_malloc(needed_size + 1);
    if (buffer == NULL) {
        return NULL;
    }
    
    // 第二次调用实际格式化字符串
    va_start(args, format);
    vsnprintf(buffer, needed_size + 1, format, args);
    va_end(args);
    
    return buffer;
}

/**
 * @brief 获取当前位置信息
 */
void F_get_current_position(Stru_ParserInterface_t* interface, size_t* line, size_t* column) {
    if (interface == NULL || line == NULL || column == NULL) {
        if (line != NULL) *line = 0;
        if (column != NULL) *column = 0;
        return;
    }
    
    // 通过当前令牌获取位置信息
    Stru_Token_t* current_token = interface->get_current_token(interface);
    if (current_token != NULL) {
        *line = current_token->line;
        *column = current_token->column;
    } else {
        *line = 0;
        *column = 0;
    }
}

/**
 * @brief 检查是否到达文件结尾
 */
bool F_is_at_end_of_file(Stru_ParserInterface_t* interface) {
    if (interface == NULL) {
        return true;
    }
    
    // 通过当前令牌检查是否到达文件结尾
    Stru_Token_t* current_token = interface->get_current_token(interface);
    return (current_token == NULL || current_token->type == Eum_TOKEN_EOF);
}
