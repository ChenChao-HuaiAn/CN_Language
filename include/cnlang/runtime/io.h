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

CnRtFile cn_rt_file_open(const char* path, const char* mode);
void     cn_rt_file_close(CnRtFile file);
size_t   cn_rt_file_read(CnRtFile file, void* buffer, size_t size);
size_t   cn_rt_file_write(CnRtFile file, const void* buffer, size_t size);
int      cn_rt_file_eof(CnRtFile file);

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
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

#ifndef CN_FREESTANDING
#define 设置文件缓冲 cn_rt_file_setbuf
#endif

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_IO_H */
