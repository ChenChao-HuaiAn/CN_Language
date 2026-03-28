/**
 * @file file_ext.c
 * @brief CN语言文件操作扩展实现
 * 
 * 实现文件存在检查、文件大小获取、逐行读写、文件定位等扩展功能。
 * 支持跨平台（Windows/Linux/macOS）。
 */

#include "cnlang/runtime/io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* 跨平台文件存在检查支持 */
#ifdef _WIN32
    #include <io.h>
    #define F_OK 0
    #define access _access
#else
    #include <unistd.h>
#endif

/* 跨平台大文件支持 */
#ifdef _WIN32
    #define fseek_large _fseeki64
    #define ftell_large _ftelli64
#else
    #define fseek_large fseeko
    #define ftell_large ftello
#endif

// =============================================================================
// 高优先级函数实现
// =============================================================================

/**
 * 检查文件是否存在
 * 使用标准C的access函数实现跨平台支持
 */
int cn_rt_file_exists(const char* path) {
    if (path == NULL) {
        return -1;  // 无效参数
    }
    
    /* 使用access函数检查文件是否存在 */
    if (access(path, F_OK) == 0) {
        return 1;   // 文件存在
    }
    
    /* 检查错误类型 */
    if (errno == ENOENT) {
        return 0;   // 文件不存在
    }
    
    return -1;  // 其他错误（如权限不足）
}

/**
 * 获取文件大小（字节数）
 * 通过定位到文件末尾获取文件大小
 */
long long cn_rt_file_size(CnRtFile file) {
    if (file == NULL) {
        return -1;  // 无效文件句柄
    }
    
    FILE* fp = (FILE*)file;
    
    /* 保存当前文件指针位置 */
    long long current_pos = ftell_large(fp);
    if (current_pos < 0) {
        return -1;  // 获取当前位置失败
    }
    
    /* 定位到文件末尾 */
    if (fseek_large(fp, 0, SEEK_END) != 0) {
        return -1;  // 定位失败
    }
    
    /* 获取文件大小 */
    long long size = ftell_large(fp);
    
    /* 恢复原文件指针位置 */
    fseek_large(fp, current_pos, SEEK_SET);
    
    return size;
}

/**
 * 从文件读取一行
 * 支持跨平台换行符（\n / \r\n）
 */
int cn_rt_file_read_line(CnRtFile file, char* buffer, size_t size) {
    if (file == NULL || buffer == NULL || size == 0) {
        return -1;  // 无效参数
    }
    
    FILE* fp = (FILE*)file;
    size_t i = 0;
    int c;
    
    /* 逐字符读取直到遇到换行符或EOF */
    while (i < size - 1 && (c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            break;  // 遇到换行符，停止读取
        }
        
        /* 跳过回车符（处理Windows换行符 \r\n） */
        if (c != '\r') {
            buffer[i++] = (char)c;
        }
    }
    
    /* 添加字符串结束符 */
    buffer[i] = '\0';
    
    /* 返回读取的字符数 */
    if (i == 0 && c == EOF) {
        return 0;   // 已到达文件末尾
    }
    
    return (int)i;
}

/**
 * 写入一行到文件
 * 自动添加换行符
 */
int cn_rt_file_write_line(CnRtFile file, const char* str) {
    if (file == NULL || str == NULL) {
        return -1;  // 无效参数
    }
    
    FILE* fp = (FILE*)file;
    
    /* 写入字符串内容 */
    size_t len = strlen(str);
    size_t written = fwrite(str, 1, len, fp);
    
    if (written != len) {
        return -1;  // 写入失败
    }
    
    /* 写入换行符 */
    if (fputc('\n', fp) == EOF) {
        return -1;  // 写入换行符失败
    }
    
    return (int)(written + 1);  // 返回写入的总字符数（包括换行符）
}

/**
 * 移动文件指针位置
 * 支持大文件定位
 */
int cn_rt_file_seek(CnRtFile file, long long offset, int origin) {
    if (file == NULL) {
        return -1;  // 无效文件句柄
    }
    
    FILE* fp = (FILE*)file;
    
    /* 转换origin参数 */
    int whence;
    switch (origin) {
        case 0:  whence = SEEK_SET; break;  // 文件头
        case 1:  whence = SEEK_CUR; break;  // 当前位置
        case 2:  whence = SEEK_END; break;  // 文件尾
        default: return -1;  // 无效参数
    }
    
    /* 执行定位操作 */
    return (fseek_large(fp, offset, whence) == 0) ? 0 : -1;
}

/**
 * 获取当前文件指针位置
 * 支持大文件
 */
long long cn_rt_file_tell(CnRtFile file) {
    if (file == NULL) {
        return -1;  // 无效文件句柄
    }
    
    FILE* fp = (FILE*)file;
    return ftell_large(fp);
}

// =============================================================================
// 中优先级函数实现
// =============================================================================

/**
 * 检查文件操作错误
 */
int cn_rt_file_error(CnRtFile file) {
    if (file == NULL) {
        return 1;  // 无效文件句柄视为错误
    }
    
    FILE* fp = (FILE*)file;
    return ferror(fp);
}

/**
 * 删除指定文件
 */
int cn_rt_file_remove(const char* path) {
    if (path == NULL) {
        return -1;  // 无效参数
    }
    
    /* 使用标准C的remove函数删除文件 */
    return (remove(path) == 0) ? 0 : -1;
}

/**
 * 重命名文件
 */
int cn_rt_file_rename(const char* old_path, const char* new_path) {
    if (old_path == NULL || new_path == NULL) {
        return -1;  // 无效参数
    }
    
    /* 使用标准C的rename函数重命名文件 */
    return (rename(old_path, new_path) == 0) ? 0 : -1;
}
