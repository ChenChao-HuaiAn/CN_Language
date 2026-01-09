/**
 * @file CN_parser_interface.c
 * @brief CN_Language 语法分析器接口工厂函数
 * 
 * 语法分析器接口的工厂函数实现，创建并初始化语法分析器接口实例。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_parser_interface.h"
#include "interface/CN_parser_interface_impl.h"
#include <stdlib.h>
#include <string.h>

// ============================================
// 工厂函数实现
// ============================================

/**
 * @brief 创建语法分析器接口实例
 * 
 * 创建并返回一个新的语法分析器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_ParserInterface_t* 新创建的语法分析器接口实例
 */
Stru_ParserInterface_t* F_create_parser_interface(void) {
    // 分配接口内存
    Stru_ParserInterface_t* interface = (Stru_ParserInterface_t*)malloc(sizeof(Stru_ParserInterface_t));
    if (!interface) {
        return NULL;
    }
    
    // 分配内部状态内存
    Stru_ParserState_t* state = (Stru_ParserState_t*)malloc(sizeof(Stru_ParserState_t));
    if (!state) {
        free(interface);
        return NULL;
    }
    
    // 初始化内部状态
    memset(state, 0, sizeof(Stru_ParserState_t));
    state->max_errors = 100;
    
    // 设置接口函数指针
    interface->initialize = F_parser_initialize;
    interface->parse_program = F_parser_parse_program;
    interface->parse_statement = F_parser_parse_statement;
    interface->parse_expression = F_parser_parse_expression;
    interface->parse_declaration = F_parser_parse_declaration;
    interface->create_ast_node = F_parser_create_ast_node;
    interface->add_child_node = F_parser_add_child_node;
    interface->set_node_attribute = F_parser_set_node_attribute;
    interface->get_node_attribute = F_parser_get_node_attribute;
    interface->report_error = F_parser_report_error;
    interface->has_errors = F_parser_has_errors;
    interface->get_errors = F_parser_get_errors;
    interface->clear_errors = F_parser_clear_errors;
    interface->get_current_token = F_parser_get_current_token;
    interface->get_previous_token = F_parser_get_previous_token;
    interface->get_next_token = F_parser_get_next_token;
    interface->reset = F_parser_reset;
    interface->destroy = F_parser_destroy;
    
    // 设置内部状态指针
    interface->internal_state = state;
    
    return interface;
}
