#include "cnlang/runtime/io.h"
#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/*
 * CN Language 运行时 I/O 实现
 */

// =============================================================================
// 通用智能读取函数实现
// =============================================================================

/**
 * 智能读取函数：读取一行输入并自动识别类型
 * 
 * 识别规则：
 * 1. 如果输入可以完整解析为整数（无小数点），返回整数类型
 * 2. 如果输入可以完整解析为浮点数（有小数点或科学计数法），返回浮点类型
 * 3. 否则返回字符串类型
 */
CnInputValue cn_rt_read(void) {
    CnInputValue result;
    result.type = CN_INPUT_TYPE_NONE;
    result.data.string_val = NULL;
    
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return result;  // 返回空值
    }
    
    // 移除换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    // 跳过前导空白
    char* start = buffer;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    // 空输入返回空字符串
    if (*start == '\0') {
        result.type = CN_INPUT_TYPE_STRING;
        result.data.string_val = (char*)cn_rt_malloc(1);
        if (result.data.string_val) {
            result.data.string_val[0] = '\0';
        }
        return result;
    }
    
    // 尝试解析为整数
    char* end_ptr = NULL;
    errno = 0;
    long long int_val = strtoll(start, &end_ptr, 10);
    
    // 检查是否完整解析为整数（无小数点）
    if (errno == 0 && end_ptr != start) {
        // 跳过尾部空白
        while (*end_ptr && isspace((unsigned char)*end_ptr)) {
            end_ptr++;
        }
        // 如果到达末尾且没有小数点，则是整数
        if (*end_ptr == '\0' && strchr(start, '.') == NULL) {
            result.type = CN_INPUT_TYPE_INT;
            result.data.int_val = int_val;
            return result;
        }
    }
    
    // 尝试解析为浮点数
    errno = 0;
    double float_val = strtod(start, &end_ptr);
    
    if (errno == 0 && end_ptr != start) {
        // 跳过尾部空白
        while (*end_ptr && isspace((unsigned char)*end_ptr)) {
            end_ptr++;
        }
        // 如果完整解析，则是浮点数
        if (*end_ptr == '\0') {
            result.type = CN_INPUT_TYPE_FLOAT;
            result.data.float_val = float_val;
            return result;
        }
    }
    
    // 默认作为字符串处理
    result.type = CN_INPUT_TYPE_STRING;
    size_t str_len = strlen(start);
    result.data.string_val = (char*)cn_rt_malloc(str_len + 1);
    if (result.data.string_val) {
        memcpy(result.data.string_val, start, str_len + 1);
    }
    
    return result;
}

// 类型判断辅助函数
int cn_rt_input_is_int(const CnInputValue* val) {
    return val && val->type == CN_INPUT_TYPE_INT;
}

int cn_rt_input_is_float(const CnInputValue* val) {
    return val && val->type == CN_INPUT_TYPE_FLOAT;
}

int cn_rt_input_is_string(const CnInputValue* val) {
    return val && val->type == CN_INPUT_TYPE_STRING;
}

int cn_rt_input_is_number(const CnInputValue* val) {
    return val && (val->type == CN_INPUT_TYPE_INT || val->type == CN_INPUT_TYPE_FLOAT);
}

// 类型转换函数
long long cn_rt_input_to_int(const CnInputValue* val) {
    if (!val) return 0;
    
    switch (val->type) {
        case CN_INPUT_TYPE_INT:
            return val->data.int_val;
        case CN_INPUT_TYPE_FLOAT:
            return (long long)val->data.float_val;
        case CN_INPUT_TYPE_STRING:
            if (val->data.string_val) {
                return strtoll(val->data.string_val, NULL, 10);
            }
            return 0;
        default:
            return 0;
    }
}

double cn_rt_input_to_float(const CnInputValue* val) {
    if (!val) return 0.0;
    
    switch (val->type) {
        case CN_INPUT_TYPE_INT:
            return (double)val->data.int_val;
        case CN_INPUT_TYPE_FLOAT:
            return val->data.float_val;
        case CN_INPUT_TYPE_STRING:
            if (val->data.string_val) {
                return strtod(val->data.string_val, NULL);
            }
            return 0.0;
        default:
            return 0.0;
    }
}

// 静态缓冲区用于整数和浮点数转字符串
static char g_input_string_buffer[64];

const char* cn_rt_input_to_string(const CnInputValue* val) {
    if (!val) return "";
    
    switch (val->type) {
        case CN_INPUT_TYPE_INT:
            snprintf(g_input_string_buffer, sizeof(g_input_string_buffer), 
                     "%lld", val->data.int_val);
            return g_input_string_buffer;
        case CN_INPUT_TYPE_FLOAT:
            snprintf(g_input_string_buffer, sizeof(g_input_string_buffer), 
                     "%g", val->data.float_val);
            return g_input_string_buffer;
        case CN_INPUT_TYPE_STRING:
            return val->data.string_val ? val->data.string_val : "";
        default:
            return "";
    }
}

void cn_rt_input_free(CnInputValue* val) {
    if (val && val->type == CN_INPUT_TYPE_STRING && val->data.string_val) {
        cn_rt_free(val->data.string_val);
        val->data.string_val = NULL;
    }
    if (val) {
        val->type = CN_INPUT_TYPE_NONE;
    }
}

// =============================================================================
// 字符串转换辅助函数实现
// =============================================================================

long long cn_rt_str_to_int(const char* str) {
    if (str == NULL) return 0;
    return strtoll(str, NULL, 10);
}

double cn_rt_str_to_float(const char* str) {
    if (str == NULL) return 0.0;
    return strtod(str, NULL);
}

int cn_rt_is_numeric_str(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    // 跳过前导空白
    while (*str && isspace((unsigned char)*str)) str++;
    if (*str == '\0') return 0;
    
    // 检查可选的负号
    if (*str == '-' || *str == '+') str++;
    if (*str == '\0') return 0;
    
    int has_digit = 0;
    int has_dot = 0;
    
    while (*str) {
        if (isdigit((unsigned char)*str)) {
            has_digit = 1;
        } else if (*str == '.' && !has_dot) {
            has_dot = 1;
        } else if (isspace((unsigned char)*str)) {
            // 跳过尾部空白
            while (*str && isspace((unsigned char)*str)) str++;
            return *str == '\0' && has_digit;
        } else {
            return 0;
        }
        str++;
    }
    
    return has_digit;
}

int cn_rt_is_int_str(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    // 跳过前导空白
    while (*str && isspace((unsigned char)*str)) str++;
    if (*str == '\0') return 0;
    
    // 检查可选的负号
    if (*str == '-' || *str == '+') str++;
    if (*str == '\0') return 0;
    
    int has_digit = 0;
    
    while (*str) {
        if (isdigit((unsigned char)*str)) {
            has_digit = 1;
        } else if (isspace((unsigned char)*str)) {
            // 跳过尾部空白
            while (*str && isspace((unsigned char)*str)) str++;
            return *str == '\0' && has_digit;
        } else {
            return 0;  // 包含非数字字符（包括小数点）
        }
        str++;
    }
    
    return has_digit;
}

// =============================================================================
// 特定类型读取函数实现
// =============================================================================

char* cn_rt_read_line(void) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return NULL;
    }
    
    // 移除换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    char* result = (char*)cn_rt_malloc(len + 1);
    if (result) {
        memcpy(result, buffer, len + 1);
    }
    return result;
}

int cn_rt_read_int(long long* out_val) {
    if (out_val == NULL) return 0;
    return scanf("%lld", out_val) == 1;
}

int cn_rt_read_float(double* out_val) {
    if (out_val == NULL) return 0;
    return scanf("%lf", out_val) == 1;
}

int cn_rt_read_string(char* buffer, size_t size) {
    if (buffer == NULL || size == 0) return 0;
    
    // 读取一行到缓冲区，最多读取 size-1 个字符
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return 0;
    }
    
    // 移除末尾的换行符
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    return 1;
}

int cn_rt_read_char(char* out_char) {
    if (out_char == NULL) return 0;
    int c = getchar();
    if (c == EOF) return 0;
    *out_char = (char)c;
    return 1;
}

CnRtFile cn_rt_file_open(const char* path, const char* mode) {
    if (path == NULL || mode == NULL) return NULL;
    return (CnRtFile)fopen(path, mode);
}

void cn_rt_file_close(CnRtFile file) {
    if (file) {
        fclose((FILE*)file);
    }
}

size_t cn_rt_file_read(CnRtFile file, void* buffer, size_t size) {
    if (file == NULL || buffer == NULL) return 0;
    return fread(buffer, 1, size, (FILE*)file);
}

size_t cn_rt_file_write(CnRtFile file, const void* buffer, size_t size) {
    if (file == NULL || buffer == NULL) return 0;
    return fwrite(buffer, 1, size, (FILE*)file);
}

int cn_rt_file_eof(CnRtFile file) {
    if (file == NULL) return 1;
    return feof((FILE*)file);
}

int cn_rt_file_setbuf(CnRtFile file, char* buffer, int mode, size_t size) {
    if (file == NULL) return -1;
    
    FILE* fp = (FILE*)file;
    
    // 根据缓冲模式设置
    if (mode == 2) {  // 无缓冲
        setvbuf(fp, NULL, _IONBF, 0);
    } else if (mode == 1) {  // 行缓冲
        setvbuf(fp, buffer, _IOLBF, size);
    } else {  // 全缓冲
        setvbuf(fp, buffer, _IOFBF, size);
    }
    
    return 0;
}

int cn_rt_flush_all(void) {
    return fflush(NULL);
}
