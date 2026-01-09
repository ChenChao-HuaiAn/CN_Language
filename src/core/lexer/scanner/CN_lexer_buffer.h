/**
 * @file CN_lexer_buffer.h
 * @brief CN_Language 词法分析器缓冲区管理模块
 * 
 * 提供高效的缓冲区管理，支持大文件处理和流式读取。
 * 优化内存使用和性能，特别针对大文件场景。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 */

#ifndef CN_LEXER_BUFFER_H
#define CN_LEXER_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief 缓冲区管理策略枚举
 */
typedef enum Eum_BufferStrategy_t {
    Eum_BUFFER_STRATEGY_FULL,      ///< 完整加载整个文件到内存
    Eum_BUFFER_STRATEGY_STREAMING, ///< 流式读取，使用滑动窗口
    Eum_BUFFER_STRATEGY_MMAP       ///< 内存映射文件（如果支持）
} Eum_BufferStrategy_t;

/**
 * @brief 缓冲区结构体
 */
typedef struct Stru_LexerBuffer_t {
    const char* data;              ///< 缓冲区数据
    size_t size;                   ///< 缓冲区大小（字节）
    size_t capacity;               ///< 缓冲区容量（字节）
    size_t position;               ///< 当前位置（字节偏移）
    Eum_BufferStrategy_t strategy; ///< 缓冲区策略
    void* internal_data;           ///< 内部数据（文件句柄、映射等）
    bool is_owner;                 ///< 是否拥有数据所有权
} Stru_LexerBuffer_t;

/**
 * @brief 创建缓冲区（完整加载策略）
 * 
 * 创建并初始化一个缓冲区，完整加载数据到内存。
 * 
 * @param data 数据指针
 * @param size 数据大小
 * @param take_ownership 是否取得数据所有权
 * @return Stru_LexerBuffer_t* 新创建的缓冲区
 * @retval NULL 创建失败
 */
Stru_LexerBuffer_t* F_create_buffer_full(const char* data, size_t size, bool take_ownership);

/**
 * @brief 创建缓冲区（文件流式策略）
 * 
 * 创建并初始化一个缓冲区，使用流式读取文件。
 * 
 * @param file_path 文件路径
 * @param window_size 滑动窗口大小
 * @return Stru_LexerBuffer_t* 新创建的缓冲区
 * @retval NULL 创建失败
 */
Stru_LexerBuffer_t* F_create_buffer_streaming(const char* file_path, size_t window_size);

/**
 * @brief 销毁缓冲区
 * 
 * 释放缓冲区占用的所有资源。
 * 
 * @param buffer 要销毁的缓冲区
 */
void F_destroy_buffer(Stru_LexerBuffer_t* buffer);

/**
 * @brief 从缓冲区读取字符
 * 
 * 从缓冲区读取指定位置的字符。
 * 
 * @param buffer 缓冲区
 * @param position 位置
 * @return char 字符，'\0'表示位置无效或结束
 */
char F_buffer_read_char(const Stru_LexerBuffer_t* buffer, size_t position);

/**
 * @brief 从当前位置读取字符并前进
 * 
 * 从缓冲区当前位置读取字符，并将位置向前移动。
 * 
 * @param buffer 缓冲区
 * @return char 字符，'\0'表示结束
 */
char F_buffer_next_char(Stru_LexerBuffer_t* buffer);

/**
 * @brief 查看当前位置的字符（不移动位置）
 * 
 * 查看缓冲区当前位置的字符，但不移动位置。
 * 
 * @param buffer 缓冲区
 * @return char 字符，'\0'表示结束
 */
char F_buffer_peek_char(const Stru_LexerBuffer_t* buffer);

/**
 * @brief 检查是否还有更多字符
 * 
 * 检查缓冲区是否还有更多字符可读。
 * 
 * @param buffer 缓冲区
 * @return true 还有更多字符
 * @return false 没有更多字符
 */
bool F_buffer_has_more_chars(const Stru_LexerBuffer_t* buffer);

/**
 * @brief 获取缓冲区当前位置
 * 
 * 获取缓冲区的当前位置。
 * 
 * @param buffer 缓冲区
 * @return size_t 当前位置
 */
size_t F_buffer_get_position(const Stru_LexerBuffer_t* buffer);

/**
 * @brief 设置缓冲区位置
 * 
 * 设置缓冲区的当前位置。
 * 
 * @param buffer 缓冲区
 * @param position 新位置
 * @return bool 设置是否成功
 */
bool F_buffer_set_position(Stru_LexerBuffer_t* buffer, size_t position);

/**
 * @brief 获取缓冲区大小
 * 
 * 获取缓冲区的总大小。
 * 
 * @param buffer 缓冲区
 * @return size_t 缓冲区大小
 */
size_t F_buffer_get_size(const Stru_LexerBuffer_t* buffer);

/**
 * @brief 获取缓冲区数据
 * 
 * 获取缓冲区的数据指针。
 * 
 * @param buffer 缓冲区
 * @return const char* 数据指针
 */
const char* F_buffer_get_data(const Stru_LexerBuffer_t* buffer);

/**
 * @brief 预加载数据
 * 
 * 预加载指定范围的数据到缓冲区。
 * 
 * @param buffer 缓冲区
 * @param start 起始位置
 * @param size 大小
 * @return bool 预加载是否成功
 */
bool F_buffer_preload(Stru_LexerBuffer_t* buffer, size_t start, size_t size);

/**
 * @brief 获取性能统计
 * 
 * 获取缓冲区的性能统计信息。
 * 
 * @param buffer 缓冲区
 * @param reads 输出参数：读取次数
 * @param cache_hits 输出参数：缓存命中次数
 * @param disk_reads 输出参数：磁盘读取次数
 */
void F_buffer_get_stats(const Stru_LexerBuffer_t* buffer, 
                       size_t* reads, size_t* cache_hits, size_t* disk_reads);

/**
 * @brief 重置性能统计
 * 
 * 重置缓冲区的性能统计信息。
 * 
 * @param buffer 缓冲区
 */
void F_buffer_reset_stats(Stru_LexerBuffer_t* buffer);

#endif // CN_LEXER_BUFFER_H
