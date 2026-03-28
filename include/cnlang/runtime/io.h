#ifndef CNLANG_RUNTIME_IO_H
#define CNLANG_RUNTIME_IO_H

#include <stddef.h>
#include <stdio.h>

/*
 * CN Language 运行时 I/O 模块
 * 提供控制台和文件操作接口
 *
 * Freestanding 模式支持：
 * - [HOSTED] 所有 I/O 接口仅在宿主模式下可用
 * - Freestanding 模式下不包含此模块
 */

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// 以下所有接口仅在宿主模式下可用 [HOSTED]
// =============================================================================
#ifndef CN_FREESTANDING

// =============================================================================
// 通用输入值类型（支持自动类型识别）
// =============================================================================

/**
 * 输入值类型枚举
 */
typedef enum {
    CN_INPUT_TYPE_NONE = 0,     // 无效/空输入
    CN_INPUT_TYPE_INT,          // 整数类型
    CN_INPUT_TYPE_FLOAT,        // 浮点数类型
    CN_INPUT_TYPE_STRING        // 字符串类型
} CnInputType;

/**
 * 通用输入值结构
 * 用于存储自动识别类型的用户输入
 */
typedef struct {
    CnInputType type;           // 值的类型
    union {
        long long int_val;      // 整数值
        double float_val;       // 浮点数值
        char* string_val;       // 字符串值（动态分配）
    } data;
} CnInputValue;

// =============================================================================
// 控制台输入函数
// =============================================================================

// 通用智能读取函数（自动识别类型）
CnInputValue cn_rt_read(void);

// 类型判断辅助函数
int cn_rt_input_is_int(const CnInputValue* val);
int cn_rt_input_is_float(const CnInputValue* val);
int cn_rt_input_is_string(const CnInputValue* val);
int cn_rt_input_is_number(const CnInputValue* val);

// 类型转换函数
long long cn_rt_input_to_int(const CnInputValue* val);
double cn_rt_input_to_float(const CnInputValue* val);
const char* cn_rt_input_to_string(const CnInputValue* val);

// 释放输入值资源
void cn_rt_input_free(CnInputValue* val);

// 字符串转换辅助函数
long long cn_rt_str_to_int(const char* str);
double cn_rt_str_to_float(const char* str);
int cn_rt_is_numeric_str(const char* str);
int cn_rt_is_int_str(const char* str);

// 特定类型读取函数
char* cn_rt_read_line(void);
int   cn_rt_read_int(long long* out_val);
int   cn_rt_read_float(double* out_val);
int   cn_rt_read_string(char* buffer, size_t size);
int   cn_rt_read_char(char* out_char);

// 文件操作
typedef void* CnRtFile;

// 基础文件操作
CnRtFile cn_rt_file_open(const char* path, const char* mode);
void     cn_rt_file_close(CnRtFile file);
size_t   cn_rt_file_read(CnRtFile file, void* buffer, size_t size);
size_t   cn_rt_file_write(CnRtFile file, const void* buffer, size_t size);
int      cn_rt_file_eof(CnRtFile file);

// =============================================================================
// 文件操作扩展函数（高优先级）
// =============================================================================

/**
 * 检查文件是否存在
 * @param path 文件路径
 * @return 1表示存在，0表示不存在，-1表示错误
 */
int cn_rt_file_exists(const char* path);

/**
 * 获取文件大小（字节数）
 * @param file 文件句柄
 * @return 文件大小（字节数），失败返回 -1
 */
long long cn_rt_file_size(CnRtFile file);

/**
 * 从文件读取一行
 * @param file 文件句柄
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 * @return 读取的字符数，失败返回 -1，EOF返回 0
 */
int cn_rt_file_read_line(CnRtFile file, char* buffer, size_t size);

/**
 * 写入一行到文件（自动添加换行符）
 * @param file 文件句柄
 * @param str 要写入的字符串
 * @return 成功写入的字符数，失败返回 -1
 */
int cn_rt_file_write_line(CnRtFile file, const char* str);

/**
 * 移动文件指针位置
 * @param file 文件句柄
 * @param offset 偏移量
 * @param origin 起始位置（0:文件头, 1:当前位置, 2:文件尾）
 * @return 成功返回 0，失败返回 -1
 */
int cn_rt_file_seek(CnRtFile file, long long offset, int origin);

/**
 * 获取当前文件指针位置
 * @param file 文件句柄
 * @return 当前位置，失败返回 -1
 */
long long cn_rt_file_tell(CnRtFile file);

// =============================================================================
// 文件操作扩展函数（中优先级）
// =============================================================================

/**
 * 检查文件操作错误
 * @param file 文件句柄
 * @return 0表示无错误，非0表示有错误
 */
int cn_rt_file_error(CnRtFile file);

/**
 * 删除指定文件
 * @param path 文件路径
 * @return 成功返回 0，失败返回 -1
 */
int cn_rt_file_remove(const char* path);

/**
 * 重命名文件
 * @param old_path 原文件路径
 * @param new_path 新文件路径
 * @return 成功返回 0，失败返回 -1
 */
int cn_rt_file_rename(const char* old_path, const char* new_path);

// 缓冲区管理

/**
 * 设置文件缓冲区：为文件流设置缓冲区
 * @param file 文件句柄
 * @param buffer 缓冲区指针（为NULL则使用默认缓冲）
 * @param mode 缓冲模式（0:全缓冲, 1:行缓冲, 2:无缓冲）
 * @param size 缓冲区大小
 * @return 成功返回0，失败返回非零值
 */
int cn_rt_file_setbuf(CnRtFile file, char* buffer, int mode, size_t size);

/**
 * 刷新所有缓冲：刷新所有打开的文件流缓冲区
 * @return 成功返回0，失败返回EOF
 */
int cn_rt_flush_all(void);

#else

// =============================================================================
// Freestanding 模式：I/O 函数不可用，提供编译期错误
// =============================================================================

// 在 freestanding 模式下，尝试使用 I/O 函数将导致编译错误
#define cn_rt_read(...) \
    _Static_assert(0, "cn_rt_read is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_read_line(...) \
    _Static_assert(0, "cn_rt_read_line is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_read_int(...) \
    _Static_assert(0, "cn_rt_read_int is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_read_float(...) \
    _Static_assert(0, "cn_rt_read_float is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_read_string(...) \
    _Static_assert(0, "cn_rt_read_string is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_read_char(...) \
    _Static_assert(0, "cn_rt_read_char is not available in freestanding mode. " \
                      "Use kernel-provided input mechanisms instead.")

#define cn_rt_input_is_int(...) \
    _Static_assert(0, "cn_rt_input_is_int is not available in freestanding mode.")

#define cn_rt_input_is_float(...) \
    _Static_assert(0, "cn_rt_input_is_float is not available in freestanding mode.")

#define cn_rt_input_is_string(...) \
    _Static_assert(0, "cn_rt_input_is_string is not available in freestanding mode.")

#define cn_rt_input_is_number(...) \
    _Static_assert(0, "cn_rt_input_is_number is not available in freestanding mode.")

#define cn_rt_input_to_int(...) \
    _Static_assert(0, "cn_rt_input_to_int is not available in freestanding mode.")

#define cn_rt_input_to_float(...) \
    _Static_assert(0, "cn_rt_input_to_float is not available in freestanding mode.")

#define cn_rt_input_to_string(...) \
    _Static_assert(0, "cn_rt_input_to_string is not available in freestanding mode.")

#define cn_rt_input_free(...) \
    _Static_assert(0, "cn_rt_input_free is not available in freestanding mode.")

#define cn_rt_file_open(...) \
    _Static_assert(0, "cn_rt_file_open is not available in freestanding mode. " \
                      "Kernel environments typically have no filesystem.")

#define cn_rt_file_close(...) \
    _Static_assert(0, "cn_rt_file_close is not available in freestanding mode.")

#define cn_rt_file_read(...) \
    _Static_assert(0, "cn_rt_file_read is not available in freestanding mode.")

#define cn_rt_file_write(...) \
    _Static_assert(0, "cn_rt_file_write is not available in freestanding mode.")

#define cn_rt_file_eof(...) \
    _Static_assert(0, "cn_rt_file_eof is not available in freestanding mode.")

#endif /* CN_FREESTANDING */

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
//
// 注意：为避免与 stdlib.h 中的中文函数名冲突，文件操作扩展函数使用
// "文件"前缀或更具体的名称来区分
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

#ifndef CN_FREESTANDING

// 基础文件操作中文别名
#define 设置文件缓冲 cn_rt_file_setbuf

// 文件操作扩展函数中文别名（高优先级）
// 注意：使用"文件"前缀区分，避免与 stdlib.h 中的同名函数冲突
#define 文件存在 cn_rt_file_exists
#define 文件大小 cn_rt_file_size
#define 文件读取行 cn_rt_file_read_line      // 从文件读取一行（区别于 stdlib.h 的 读取行）
#define 文件写入行 cn_rt_file_write_line     // 写入一行到文件
#define 文件指针定位 cn_rt_file_seek         // 移动文件指针（区别于 stdlib.h 的 文件定位）
#define 获取文件指针位置 cn_rt_file_tell     // 获取文件指针位置（区别于 stdlib.h 的 获取文件位置）

// 文件操作扩展函数中文别名（中优先级）
#define 文件错误 cn_rt_file_error
#define 删除文件 cn_rt_file_remove
#define 重命名文件 cn_rt_file_rename

#endif // CN_FREESTANDING

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_IO_H */
