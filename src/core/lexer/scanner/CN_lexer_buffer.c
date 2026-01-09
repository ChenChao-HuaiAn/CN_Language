/**
 * @file CN_lexer_buffer.c
 * @brief CN_Language 词法分析器缓冲区管理模块实现
 * 
 * 提供高效的缓冲区管理，支持大文件处理和流式读取。
 * 优化内存使用和性能，特别针对大文件场景。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 */

#include "CN_lexer_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/**
 * @brief 流式缓冲区内部数据结构
 */
typedef struct Stru_StreamingBufferData_t {
    FILE* file;                     ///< 文件指针
    size_t window_size;             ///< 窗口大小
    char* window_data;              ///< 窗口数据
    size_t window_start;            ///< 窗口起始位置
    size_t file_size;               ///< 文件大小
    bool is_eof;                    ///< 是否到达文件末尾
} Stru_StreamingBufferData_t;

/**
 * @brief 内存映射缓冲区内部数据结构
 */
typedef struct Stru_MmapBufferData_t {
#ifdef _WIN32
    HANDLE file_handle;             ///< 文件句柄
    HANDLE mapping_handle;          ///< 映射句柄
#else
    int fd;                         ///< 文件描述符
#endif
    size_t file_size;               ///< 文件大小
} Stru_MmapBufferData_t;

/**
 * @brief 性能统计数据结构
 */
typedef struct Stru_BufferStats_t {
    size_t total_reads;             ///< 总读取次数
    size_t cache_hits;              ///< 缓存命中次数
    size_t disk_reads;              ///< 磁盘读取次数
} Stru_BufferStats_t;

/**
 * @brief 完整缓冲区内部数据结构
 */
typedef struct Stru_FullBufferData_t {
    char* owned_data;               ///< 拥有的数据（如果需要释放）
} Stru_FullBufferData_t;

/**
 * @brief 缓冲区内部数据（联合体）
 */
typedef union Uni_BufferInternalData_t {
    Stru_FullBufferData_t full;     ///< 完整缓冲区数据
    Stru_StreamingBufferData_t streaming; ///< 流式缓冲区数据
    Stru_MmapBufferData_t mmap;     ///< 内存映射缓冲区数据
} Uni_BufferInternalData_t;

/**
 * @brief 缓冲区扩展结构（包含性能统计）
 */
typedef struct Stru_LexerBufferExtended_t {
    Stru_LexerBuffer_t base;        ///< 基础缓冲区结构
    Uni_BufferInternalData_t internal; ///< 内部数据
    Stru_BufferStats_t stats;       ///< 性能统计
} Stru_LexerBufferExtended_t;

/**
 * @brief 创建完整加载缓冲区
 */
Stru_LexerBuffer_t* F_create_buffer_full(const char* data, size_t size, bool take_ownership)
{
    if (data == NULL || size == 0) {
        return NULL;
    }

    Stru_LexerBufferExtended_t* buffer = (Stru_LexerBufferExtended_t*)malloc(sizeof(Stru_LexerBufferExtended_t));
    if (buffer == NULL) {
        return NULL;
    }

    // 初始化基础结构
    buffer->base.data = data;
    buffer->base.size = size;
    buffer->base.capacity = size;
    buffer->base.position = 0;
    buffer->base.strategy = Eum_BUFFER_STRATEGY_FULL;
    buffer->base.is_owner = take_ownership;

    // 初始化内部数据
    if (take_ownership) {
        buffer->internal.full.owned_data = (char*)data;
    } else {
        buffer->internal.full.owned_data = NULL;
    }

    // 初始化性能统计
    buffer->stats.total_reads = 0;
    buffer->stats.cache_hits = 0;
    buffer->stats.disk_reads = 0;

    return (Stru_LexerBuffer_t*)buffer;
}

/**
 * @brief 创建流式缓冲区
 */
Stru_LexerBuffer_t* F_create_buffer_streaming(const char* file_path, size_t window_size)
{
    if (file_path == NULL || window_size == 0) {
        return NULL;
    }

    // 打开文件
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        return NULL;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配缓冲区
    Stru_LexerBufferExtended_t* buffer = (Stru_LexerBufferExtended_t*)malloc(sizeof(Stru_LexerBufferExtended_t));
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    // 分配窗口数据
    char* window_data = (char*)malloc(window_size);
    if (window_data == NULL) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    // 初始化基础结构
    buffer->base.data = window_data;
    buffer->base.size = window_size; // 初始窗口大小
    buffer->base.capacity = window_size;
    buffer->base.position = 0;
    buffer->base.strategy = Eum_BUFFER_STRATEGY_STREAMING;
    buffer->base.is_owner = true;

    // 初始化流式数据
    buffer->internal.streaming.file = file;
    buffer->internal.streaming.window_size = window_size;
    buffer->internal.streaming.window_data = window_data;
    buffer->internal.streaming.window_start = 0;
    buffer->internal.streaming.file_size = file_size;
    buffer->internal.streaming.is_eof = false;

    // 初始化性能统计
    buffer->stats.total_reads = 0;
    buffer->stats.cache_hits = 0;
    buffer->stats.disk_reads = 0;

    // 加载初始窗口
    size_t read_size = fread(window_data, 1, window_size, file);
    if (read_size < window_size) {
        buffer->base.size = read_size;
        if (feof(file)) {
            buffer->internal.streaming.is_eof = true;
        }
    }

    buffer->stats.disk_reads++;

    return (Stru_LexerBuffer_t*)buffer;
}

/**
 * @brief 销毁缓冲区
 */
void F_destroy_buffer(Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return;
    }

    Stru_LexerBufferExtended_t* ext_buffer = (Stru_LexerBufferExtended_t*)buffer;

    // 根据策略清理资源
    switch (buffer->strategy) {
        case Eum_BUFFER_STRATEGY_FULL:
            if (buffer->is_owner && ext_buffer->internal.full.owned_data != NULL) {
                free(ext_buffer->internal.full.owned_data);
            }
            break;

        case Eum_BUFFER_STRATEGY_STREAMING:
            if (ext_buffer->internal.streaming.file != NULL) {
                fclose(ext_buffer->internal.streaming.file);
            }
            if (ext_buffer->internal.streaming.window_data != NULL) {
                free(ext_buffer->internal.streaming.window_data);
            }
            break;

        case Eum_BUFFER_STRATEGY_MMAP:
#ifdef _WIN32
            if (ext_buffer->internal.mmap.mapping_handle != NULL) {
                UnmapViewOfFile(buffer->data);
                CloseHandle(ext_buffer->internal.mmap.mapping_handle);
            }
            if (ext_buffer->internal.mmap.file_handle != INVALID_HANDLE_VALUE) {
                CloseHandle(ext_buffer->internal.mmap.file_handle);
            }
#else
            if (buffer->data != NULL) {
                munmap((void*)buffer->data, buffer->size);
            }
            if (ext_buffer->internal.mmap.fd != -1) {
                close(ext_buffer->internal.mmap.fd);
            }
#endif
            break;
    }

    free(ext_buffer);
}

/**
 * @brief 从缓冲区读取字符
 */
char F_buffer_read_char(const Stru_LexerBuffer_t* buffer, size_t position)
{
    if (buffer == NULL) {
        return '\0';
    }

    Stru_LexerBufferExtended_t* ext_buffer = (Stru_LexerBufferExtended_t*)buffer;
    ext_buffer->stats.total_reads++;

    // 检查位置是否有效
    if (position >= buffer->size) {
        return '\0';
    }

    // 根据策略读取字符
    switch (buffer->strategy) {
        case Eum_BUFFER_STRATEGY_FULL:
            return buffer->data[position];

        case Eum_BUFFER_STRATEGY_STREAMING:
            // 检查是否在窗口内
            if (position >= ext_buffer->internal.streaming.window_start &&
                position < ext_buffer->internal.streaming.window_start + buffer->size) {
                ext_buffer->stats.cache_hits++;
                return buffer->data[position - ext_buffer->internal.streaming.window_start];
            } else {
                // 需要移动窗口
                size_t window_start = position;
                if (window_start + buffer->capacity > ext_buffer->internal.streaming.file_size) {
                    window_start = ext_buffer->internal.streaming.file_size - buffer->capacity;
                    if (window_start > ext_buffer->internal.streaming.file_size) {
                        window_start = 0;
                    }
                }

                // 移动文件指针
                fseek(ext_buffer->internal.streaming.file, window_start, SEEK_SET);
                
                // 读取新窗口
                size_t read_size = fread(ext_buffer->internal.streaming.window_data, 1, 
                                        buffer->capacity, ext_buffer->internal.streaming.file);
                
                ext_buffer->internal.streaming.window_start = window_start;
                // 注意：这里不能直接修改const指针，但我们可以通过ext_buffer访问
                ((Stru_LexerBuffer_t*)ext_buffer)->data = ext_buffer->internal.streaming.window_data;
                ((Stru_LexerBuffer_t*)ext_buffer)->size = read_size;
                
                ext_buffer->stats.disk_reads++;
                
                if (position >= window_start && position < window_start + read_size) {
                    return ((Stru_LexerBuffer_t*)ext_buffer)->data[position - window_start];
                }
            }
            break;

        case Eum_BUFFER_STRATEGY_MMAP:
            if (position < buffer->size) {
                return buffer->data[position];
            }
            break;
    }

    return '\0';
}

/**
 * @brief 从当前位置读取字符并前进
 */
char F_buffer_next_char(Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return '\0';
    }

    char ch = F_buffer_read_char(buffer, buffer->position);
    if (ch != '\0') {
        buffer->position++;
    }
    
    return ch;
}

/**
 * @brief 查看当前位置的字符（不移动位置）
 */
char F_buffer_peek_char(const Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return '\0';
    }
    
    return F_buffer_read_char(buffer, buffer->position);
}

/**
 * @brief 检查是否还有更多字符
 */
bool F_buffer_has_more_chars(const Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return false;
    }
    
    return buffer->position < buffer->size;
}

/**
 * @brief 获取缓冲区当前位置
 */
size_t F_buffer_get_position(const Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return 0;
    }
    
    return buffer->position;
}

/**
 * @brief 设置缓冲区位置
 */
bool F_buffer_set_position(Stru_LexerBuffer_t* buffer, size_t position)
{
    if (buffer == NULL || position > buffer->size) {
        return false;
    }
    
    buffer->position = position;
    return true;
}

/**
 * @brief 获取缓冲区大小
 */
size_t F_buffer_get_size(const Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return 0;
    }
    
    return buffer->size;
}

/**
 * @brief 获取缓冲区数据
 */
const char* F_buffer_get_data(const Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return NULL;
    }
    
    return buffer->data;
}

/**
 * @brief 预加载数据
 */
bool F_buffer_preload(Stru_LexerBuffer_t* buffer, size_t start, size_t size)
{
    if (buffer == NULL || buffer->strategy != Eum_BUFFER_STRATEGY_STREAMING) {
        return false;
    }
    
    Stru_LexerBufferExtended_t* ext_buffer = (Stru_LexerBufferExtended_t*)buffer;
    
    // 检查是否需要预加载
    if (start >= ext_buffer->internal.streaming.window_start &&
        start + size <= ext_buffer->internal.streaming.window_start + buffer->size) {
        // 数据已经在窗口中
        return true;
    }
    
    // 移动窗口到包含请求范围的位置
    size_t window_start = start;
    if (window_start + buffer->capacity > ext_buffer->internal.streaming.file_size) {
        window_start = ext_buffer->internal.streaming.file_size - buffer->capacity;
        if (window_start > ext_buffer->internal.streaming.file_size) {
            window_start = 0;
        }
    }
    
    // 移动文件指针并读取
    fseek(ext_buffer->internal.streaming.file, window_start, SEEK_SET);
    size_t read_size = fread(ext_buffer->internal.streaming.window_data, 1, 
                            buffer->capacity, ext_buffer->internal.streaming.file);
    
    ext_buffer->internal.streaming.window_start = window_start;
    buffer->data = ext_buffer->internal.streaming.window_data;
    ((Stru_LexerBuffer_t*)buffer)->size = read_size;
    
    ext_buffer->stats.disk_reads++;
    
    return (read_size >= size && start >= window_start && start + size <= window_start + read_size);
}

/**
 * @brief 获取性能统计
 */
void F_buffer_get_stats(const Stru_LexerBuffer_t* buffer, 
                       size_t* reads, size_t* cache_hits, size_t* disk_reads)
{
    if (buffer == NULL || reads == NULL || cache_hits == NULL || disk_reads == NULL) {
        return;
    }
    
    Stru_LexerBufferExtended_t* ext_buffer = (Stru_LexerBufferExtended_t*)buffer;
    
    *reads = ext_buffer->stats.total_reads;
    *cache_hits = ext_buffer->stats.cache_hits;
    *disk_reads = ext_buffer->stats.disk_reads;
}

/**
 * @brief 重置性能统计
 */
void F_buffer_reset_stats(Stru_LexerBuffer_t* buffer)
{
    if (buffer == NULL) {
        return;
    }
    
    Stru_LexerBufferExtended_t* ext_buffer = (Stru_LexerBufferExtended_t*)buffer;
    
    ext_buffer->stats.total_reads = 0;
    ext_buffer->stats.cache_hits = 0;
    ext_buffer->stats.disk_reads = 0;
}
