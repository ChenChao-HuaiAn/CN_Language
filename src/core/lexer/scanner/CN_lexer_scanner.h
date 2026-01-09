/**
 * @file CN_lexer_scanner.h
 * @brief CN_Language 词法分析器字符扫描模块
 * 
 * 提供字符扫描、位置管理和空白字符处理功能。
 * 负责源代码的字符级操作和位置跟踪。
 * 支持大文件处理和UTF-8编码。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 2.0.0
 */

#ifndef CN_LEXER_SCANNER_H
#define CN_LEXER_SCANNER_H

#include <stddef.h>
#include <stdbool.h>
#include "../errors/CN_lexer_errors.h"
#include "CN_lexer_buffer.h"

/**
 * @brief 扫描器状态结构体
 * 
 * 存储扫描器的内部状态，包括缓冲区、位置信息和错误状态。
 */
typedef struct Stru_LexerScannerState_t {
    Stru_LexerBuffer_t* buffer;          ///< 缓冲区管理
    const char* source_name;             ///< 源文件名
    size_t current_line;                 ///< 当前行号
    size_t current_column;               ///< 当前列号
    Stru_LexerErrorContext_t* error_ctx; ///< 错误上下文
} Stru_LexerScannerState_t;

/**
 * @brief 创建扫描器状态
 * 
 * 创建并初始化一个新的扫描器状态实例。
 * 
 * @return Stru_LexerScannerState_t* 新创建的扫描器状态
 * @retval NULL 创建失败
 */
Stru_LexerScannerState_t* F_create_scanner_state(void);

/**
 * @brief 销毁扫描器状态
 * 
 * 释放扫描器状态占用的所有资源。
 * 
 * @param state 要销毁的扫描器状态
 */
void F_destroy_scanner_state(Stru_LexerScannerState_t* state);

/**
 * @brief 初始化扫描器状态（完整加载）
 * 
 * 设置源代码和源文件名，准备进行扫描。
 * 
 * @param state 扫描器状态
 * @param source 源代码字符串
 * @param length 源代码长度
 * @param source_name 源文件名（可选）
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool F_initialize_scanner_state(Stru_LexerScannerState_t* state, 
                               const char* source, size_t length, 
                               const char* source_name);

/**
 * @brief 初始化扫描器状态（文件流式）
 * 
 * 从文件创建流式缓冲区，准备进行扫描。
 * 
 * @param state 扫描器状态
 * @param file_path 文件路径
 * @param window_size 滑动窗口大小
 * @param source_name 源文件名（可选）
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool F_initialize_scanner_state_from_file(Stru_LexerScannerState_t* state,
                                         const char* file_path,
                                         size_t window_size,
                                         const char* source_name);

/**
 * @brief 重置扫描器状态
 * 
 * 重置扫描器到初始状态，可以重新扫描相同的源代码。
 * 
 * @param state 扫描器状态
 */
void F_reset_scanner_state(Stru_LexerScannerState_t* state);

/**
 * @brief 查看下一个字符（不移动位置）
 * 
 * 返回当前位置的字符，但不移动扫描位置。
 * 
 * @param state 扫描器状态
 * @return char 下一个字符，'\0'表示结束
 */
char F_peek_char(const Stru_LexerScannerState_t* state);

/**
 * @brief 获取下一个字符并移动位置
 * 
 * 返回当前位置的字符，并将扫描位置向前移动。
 * 
 * @param state 扫描器状态
 * @return char 下一个字符，'\0'表示结束
 */
char F_next_char(Stru_LexerScannerState_t* state);

/**
 * @brief 跳过空白字符
 * 
 * 跳过所有空白字符（空格、制表符、换行符）和注释。
 * 
 * @param state 扫描器状态
 */
void F_skip_whitespace(Stru_LexerScannerState_t* state);

/**
 * @brief 跳过注释
 * 
 * 跳过从当前位置开始的注释。
 * 
 * @param state 扫描器状态
 */
void F_skip_comment(Stru_LexerScannerState_t* state);

/**
 * @brief 检查是否还有更多字符
 * 
 * 检查是否还有未处理的字符。
 * 
 * @param state 扫描器状态
 * @return true 还有更多字符
 * @return false 没有更多字符
 */
bool F_has_more_chars(const Stru_LexerScannerState_t* state);

/**
 * @brief 获取当前位置
 * 
 * 获取当前扫描的位置（行号和列号）。
 * 
 * @param state 扫描器状态
 * @param line 输出参数：行号（从1开始）
 * @param column 输出参数：列号（从1开始）
 */
void F_get_scanner_position(const Stru_LexerScannerState_t* state, 
                           size_t* line, size_t* column);

/**
 * @brief 获取源文件名
 * 
 * 获取当前正在扫描的源文件名。
 * 
 * @param state 扫描器状态
 * @return const char* 源文件名
 */
const char* F_get_scanner_source_name(const Stru_LexerScannerState_t* state);

/**
 * @brief 设置扫描器错误
 * 
 * 使用新的错误处理系统设置扫描器错误。
 * 
 * @param state 扫描器状态
 * @param code 错误码
 * @param format 错误信息格式字符串
 * @param ... 格式参数
 */
void F_set_scanner_error(Stru_LexerScannerState_t* state, 
                        Eum_LexerErrorCode_t code,
                        const char* format, ...);

/**
 * @brief 设置扫描器致命错误
 * 
 * 设置扫描器致命错误，表示无法继续扫描。
 * 
 * @param state 扫描器状态
 * @param code 错误码
 * @param format 错误信息格式字符串
 * @param ... 格式参数
 */
void F_set_scanner_fatal_error(Stru_LexerScannerState_t* state,
                              Eum_LexerErrorCode_t code,
                              const char* format, ...);

/**
 * @brief 清除扫描器错误
 * 
 * 清除扫描器的所有错误信息。
 * 
 * @param state 扫描器状态
 */
void F_clear_scanner_error(Stru_LexerScannerState_t* state);

/**
 * @brief 检查扫描器是否有错误
 * 
 * 检查扫描过程中是否发生了错误。
 * 
 * @param state 扫描器状态
 * @return true 有错误
 * @return false 没有错误
 */
bool F_scanner_has_errors(const Stru_LexerScannerState_t* state);

/**
 * @brief 检查扫描器是否有致命错误
 * 
 * 检查扫描过程中是否发生了致命错误。
 * 
 * @param state 扫描器状态
 * @return true 有致命错误
 * @return false 没有致命错误
 */
bool F_scanner_has_fatal_error(const Stru_LexerScannerState_t* state);

/**
 * @brief 获取扫描器最后一个错误信息
 * 
 * 获取最后一个错误的详细描述。
 * 
 * @param state 扫描器状态
 * @return const char* 错误信息字符串
 */
const char* F_get_scanner_last_error(const Stru_LexerScannerState_t* state);

/**
 * @brief 获取扫描器错误上下文
 * 
 * 获取扫描器的错误上下文，用于更详细的错误处理。
 * 
 * @param state 扫描器状态
 * @return Stru_LexerErrorContext_t* 错误上下文
 */
Stru_LexerErrorContext_t* F_get_scanner_error_context(const Stru_LexerScannerState_t* state);

/**
 * @brief 获取缓冲区
 * 
 * 获取扫描器使用的缓冲区。
 * 
 * @param state 扫描器状态
 * @return Stru_LexerBuffer_t* 缓冲区
 */
Stru_LexerBuffer_t* F_get_scanner_buffer(const Stru_LexerScannerState_t* state);

/**
 * @brief 设置缓冲区
 * 
 * 设置扫描器使用的缓冲区。
 * 
 * @param state 扫描器状态
 * @param buffer 缓冲区
 */
void F_set_scanner_buffer(Stru_LexerScannerState_t* state, Stru_LexerBuffer_t* buffer);

#endif // CN_LEXER_SCANNER_H
