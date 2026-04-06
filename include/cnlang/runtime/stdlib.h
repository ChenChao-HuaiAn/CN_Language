#ifndef CNLANG_RUNTIME_STDLIB_H
#define CNLANG_RUNTIME_STDLIB_H

/*
 * CN_Language 标准库中文接口
 * 为标准C库函数提供中文名称的包装接口
 * 
 * 设计原则：
 * 1. 提供直观的中文命名，降低学习门槛
 * 2. 保持与底层C标准库函数的一致性语义
 * 3. 支持hosted和freestanding模式
 * 4. 函数命名采用动词+名词的方式，如"分配内存"、"复制字符串"
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/io.h"
#include "cnlang/runtime/freestanding.h"

// =============================================================================
// 内存管理函数 - 中文接口
// =============================================================================

/**
 * 分配内存：分配指定字节数的内存块
 * @param size 要分配的字节数
 * @return 指向分配内存的指针，失败返回 NULL
 * 
 * 对应标准C函数：malloc
 */
void* 分配内存(size_t size);

/**
 * 释放内存：释放之前分配的内存块
 * @param ptr 要释放的内存指针
 * 
 * 对应标准C函数：free
 */
void 释放内存(void* ptr);

/**
 * 重新分配内存：改变已分配内存块的大小
 * @param ptr 原内存指针
 * @param new_size 新的字节数
 * @return 指向新内存的指针，失败返回 NULL
 * 
 * 对应标准C函数：realloc
 */
void* 重新分配内存(void* ptr, size_t new_size);

/**
 * 分配清零内存：分配内存并初始化为0
 * @param count 元素个数
 * @param size 每个元素的字节数
 * @return 指向分配内存的指针，失败返回 NULL
 * 
 * 对应标准C函数：calloc
 */
void* 分配清零内存(size_t count, size_t size);

// =============================================================================
// 字符串处理函数 - 中文接口
// =============================================================================

/**
 * 复制字符串：将源字符串复制到目标缓冲区
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return 目标缓冲区指针
 * 
 * 对应标准C函数：strcpy
 * 注意：确保目标缓冲区足够大
 */
char* 复制字符串(char* dest, const char* src);

/**
 * 限长复制字符串：复制至多n个字符
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @param n 最大复制字符数
 * @return 目标缓冲区指针
 * 
 * 对应标准C函数：strncpy
 */
char* 限长复制字符串(char* dest, const char* src, size_t n);

/**
 * 连接字符串：将源字符串追加到目标字符串末尾
 * @param dest 目标字符串（需有足够空间）
 * @param src 源字符串
 * @return 目标字符串指针
 * 
 * 对应标准C函数：strcat
 */
char* 连接字符串(char* dest, const char* src);

/**
 * 限长连接字符串：追加至多n个字符
 * @param dest 目标字符串
 * @param src 源字符串
 * @param n 最大追加字符数
 * @return 目标字符串指针
 * 
 * 对应标准C函数：strncat
 */
char* 限长连接字符串(char* dest, const char* src, size_t n);

/**
 * 比较字符串：比较两个字符串
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 * 
 * 对应标准C函数：strcmp
 */
int 比较字符串(const char* s1, const char* s2);

/**
 * 限长比较字符串：比较两个字符串的前n个字符
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param n 比较的最大字符数
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 * 
 * 对应标准C函数：strncmp
 */
int 限长比较字符串(const char* s1, const char* s2, size_t n);

/**
 * 获取字符串长度：返回字符串中的字符数（不含'\0'）
 * @param s 字符串
 * @return 字符串长度
 * 
 * 对应标准C函数：strlen
 */
size_t 获取字符串长度(const char* s);

/**
 * 查找字符：在字符串中查找字符首次出现的位置
 * @param s 字符串
 * @param c 要查找的字符
 * @return 指向找到的字符的指针，未找到返回NULL
 * 
 * 对应标准C函数：strchr
 */
char* 查找字符(const char* s, int c);

/**
 * 反向查找字符：在字符串中查找字符最后出现的位置
 * @param s 字符串
 * @param c 要查找的字符
 * @return 指向找到的字符的指针，未找到返回NULL
 * 
 * 对应标准C函数：strrchr
 */
char* 反向查找字符(const char* s, int c);

/**
 * 查找子串：在字符串中查找子字符串
 * @param haystack 被查找的字符串
 * @param needle 要查找的子串
 * @return 指向找到的子串的指针，未找到返回NULL
 * 
 * 对应标准C函数：strstr
 */
char* 查找子串(const char* haystack, const char* needle);

/**
 * 复制内存：复制内存块
 * @param dest 目标内存
 * @param src 源内存
 * @param n 要复制的字节数
 * @return 目标内存指针
 * 
 * 对应标准C函数：memcpy
 */
void* 复制内存(void* dest, const void* src, size_t n);

/**
 * 设置内存：将内存块设置为指定值
 * @param s 目标内存
 * @param c 设置的值（字节）
 * @param n 设置的字节数
 * @return 目标内存指针
 * 
 * 对应标准C函数：memset
 */
void* 设置内存(void* s, int c, size_t n);

/**
 * 比较内存：比较两个内存块
 * @param s1 第一个内存块
 * @param s2 第二个内存块
 * @param n 比较的字节数
 * @return 0表示相等，<0表示s1<s2，>0表示s1>s2
 * 
 * 对应标准C函数：memcmp
 */
int 比较内存(const void* s1, const void* s2, size_t n);

// =============================================================================
// 标准I/O函数 - 中文接口
// =============================================================================

#ifndef CN_FREESTANDING

// =============================================================================
// 文件操作函数 - 中文接口
// =============================================================================

/**
 * 打开文件：以指定模式打开文件
 * @param path 文件路径
 * @param mode 打开模式（"r":读, "w":写, "a":追加, "r+":读写等）
 * @return 文件句柄，失败返回NULL
 * 
 * 对应标准C函数：fopen
 */
void* 打开文件(const char* path, const char* mode);

/**
 * 关闭文件：关闭已打开的文件
 * @param file 文件句柄
 * 
 * 对应标准C函数：fclose
 */
void 关闭文件(void* file);

/**
 * 读取文件：从文件读取数据到缓冲区
 * @param file 文件句柄
 * @param buffer 目标缓冲区
 * @param size 要读取的字节数
 * @return 实际读取的字节数
 * 
 * 对应标准C函数：fread
 */
size_t 读取文件(void* file, void* buffer, size_t size);

/**
 * 写入文件：将缓冲区数据写入文件
 * @param file 文件句柄
 * @param buffer 源数据缓冲区
 * @param size 要写入的字节数
 * @return 实际写入的字节数
 * 
 * 对应标准C函数：fwrite
 */
size_t 写入文件(void* file, const void* buffer, size_t size);

/**
 * 判断文件结束：检查是否到达文件末尾
 * @param file 文件句柄
 * @return 到达末尾返回非零值，否则返回0
 * 
 * 对应标准C函数：feof
 */
int 判断文件结束(void* file);

/**
 * 文件定位：设置文件读写位置
 * @param file 文件句柄
 * @param offset 偏移量
 * @param whence 起始位置（0:文件开头, 1:当前位置, 2:文件末尾）
 * @return 成功返回0，失败返回非零值
 * 
 * 对应标准C函数：fseek
 */
int 文件定位(void* file, long offset, int whence);

/**
 * 获取文件位置：获取当前文件读写位置
 * @param file 文件句柄
 * @return 当前位置偏移量，失败返回-1
 * 
 * 对应标准C函数：ftell
 */
long 获取文件位置(void* file);

/**
 * 刷新文件缓冲：将缓冲区数据立即写入文件
 * @param file 文件句柄
 * @return 成功返回0，失败返回EOF
 * 
 * 对应标准C函数：fflush
 */
int 刷新文件缓冲(void* file);

// =============================================================================
// 标准输入输出函数 - 中文接口
// =============================================================================

/**
 * 打印字符串：输出字符串到标准输出（不含换行）
 * @param str 要输出的字符串
 * @return 成功返回非负数，失败返回EOF
 * 
 * 对应标准C函数：fputs(str, stdout)
 */
int 打印字符串(const char* str);

/**
 * 打印行：输出字符串到标准输出（含换行）
 * @param str 要输出的字符串
 * @return 成功返回非负数，失败返回EOF
 * 
 * 对应标准C函数：puts
 */
int 打印行(const char* str);

/**
 * 读取行：从标准输入读取一行（需调用者释放内存）
 * @return 包含读取内容的字符串指针，失败返回NULL
 * 
 * 使用运行时函数：cn_rt_read_line
 */
char* 读取行(void);

/**
 * 读取整数：从标准输入读取一个整数
 * @param out_val 输出参数，存储读取的整数
 * @return 成功返回1，失败返回0
 * 
 * 对应标准C函数：scanf("%lld", ...)
 */
int 读取整数(long long* out_val);

/**
 * 读取小数：从标准输入读取一个浮点数
 * @param out_val 输出参数，存储读取的浮点数
 * @return 成功返回1，失败返回0
 * 
 * 对应标准C函数：scanf("%lf", ...)
 */
int 读取小数(double* out_val);

/**
 * 读取字符串：从标准输入读取字符串到指定缓冲区
 * @param buffer 目标缓冲区
 * @param size 缓冲区大小
 * @return 成功返回1，失败返回0
 * 
 * 使用运行时函数：cn_rt_read_string
 * 注意：会移除末尾的换行符
 */
int 读取字符串(char* buffer, size_t size);

/**
 * 读取字符：从标准输入读取一个字符
 * @param out_char 输出参数，存储读取的字符
 * @return 成功返回1，失败返回0（EOF）
 * 
 * 对应标准C函数：getchar
 */
int 读取字符(char* out_char);

// =============================================================================
// 通用输入函数（自动类型识别）
// =============================================================================

/**
 * 输入值类型枚举（中文别名）
 */
typedef enum {
    输入类型_无效 = 0,      // 无效/空输入
    输入类型_整数,          // 整数类型
    输入类型_小数,          // 浮点数类型
    输入类型_字符串         // 字符串类型
} 输入类型;

/**
 * 通用输入值结构（中文别名）
 */
typedef CnInputValue 输入值;

/**
 * 读取：通用智能读取函数，自动识别输入类型
 * @return 包含类型和值的输入值结构
 * 
 * 识别规则：
 * - 纯数字（无小数点）→ 整数
 * - 带小数点或科学计数法 → 小数
 * - 其他 → 字符串
 * 
 * 示例：
 *   输入值 val = 读取();
 *   如果 (是整数(&val)) { 整数 n = 取整数(&val); }
 */
输入值 读取(void);

/**
 * 是整数：判断输入值是否为整数类型
 * @param val 输入值指针
 * @return 是整数返回1，否则返回0
 */
int 是整数(const 输入值* val);

/**
 * 是小数：判断输入值是否为浮点数类型
 * @param val 输入值指针
 * @return 是小数返回1，否则返回0
 */
int 是小数(const 输入值* val);

/**
 * 是字符串：判断输入值是否为字符串类型
 * @param val 输入值指针
 * @return 是字符串返回1，否则返回0
 */
int 是字符串(const 输入值* val);

/**
 * 是数值：判断输入值是否为数值类型（整数或小数）
 * @param val 输入值指针
 * @return 是数值返回1，否则返回0
 */
int 是数值(const 输入值* val);

/**
 * 取整数：从输入值获取整数
 * @param val 输入值指针
 * @return 整数值（自动转换）
 */
long long 取整数(const 输入值* val);

/**
 * 取小数：从输入值获取浮点数
 * @param val 输入值指针
 * @return 浮点数值（自动转换）
 */
double 取小数(const 输入值* val);

/**
 * 取文本：从输入值获取字符串表示
 * @param val 输入值指针
 * @return 字符串指针
 */
const char* 取文本(const 输入值* val);

/**
 * 释放输入：释放输入值占用的资源
 * @param val 输入值指针
 */
void 释放输入(输入值* val);

// =============================================================================
// 简化版字符串转换函数（适用于 CN 语言直接调用）
// =============================================================================

/**
 * 转整数：将字符串转换为整数
 * @param str 要转换的字符串
 * @return 转换后的整数值，转换失败返回0
 */
long long 转整数(const char* str);

/**
 * 转小数：将字符串转换为浮点数
 * @param str 要转换的字符串
 * @return 转换后的浮点数值，转换失败返回0.0
 */
double 转小数(const char* str);

/**
 * 是数字文本：判断字符串是否为有效数字
 * @param str 要判断的字符串
 * @return 是数字返回1，否则返回0
 */
int 是数字文本(const char* str);

/**
 * 是整数文本：判断字符串是否为整数格式
 * @param str 要判断的字符串
 * @return 是整数格式返回1，否则返回0
 */
int 是整数文本(const char* str);

/**
 * 刷新输出：刷新标准输出缓冲区
 * @return 成功返回0，失败返回EOF
 * 
 * 对应标准C函数：fflush(stdout)
 */
int 刷新输出(void);

/**
 * 格式化打印：格式化输出到标准输出
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 输出的字符数，失败返回负数
 * 
 * 对应标准C函数：printf
 */
int 格式化打印(const char* format, ...);

/**
 * 格式化字符串：格式化输出到字符串缓冲区
 * @param str 目标缓冲区
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 输出的字符数（不含'\0'），失败返回负数
 * 
 * 对应标准C函数：sprintf
 * 注意：确保缓冲区足够大，建议使用安全格式化字符串
 */
int 格式化字符串(char* str, const char* format, ...);

/**
 * 安全格式化字符串：格式化输出到字符串缓冲区（带长度限制）
 * @param str 目标缓冲区
 * @param size 缓冲区大小
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 输出的字符数（不含'\0'），失败返回负数
 * 
 * 对应标准C函数：snprintf
 */
int 安全格式化字符串(char* str, size_t size, const char* format, ...);

#endif /* CN_FREESTANDING */

// =============================================================================
// 辅助函数 - 中文接口
// =============================================================================

/**
 * 获取绝对值：返回整数的绝对值
 * @param n 整数
 * @return 绝对值
 * 
 * 对应标准C函数：abs/labs/llabs
 */
long long 获取绝对值(long long n);

/**
 * 求最大值：返回两个整数中的较大值
 * @param a 第一个整数
 * @param b 第二个整数
 * @return 较大值
 */
long long 求最大值(long long a, long long b);

/**
 * 求最小值：返回两个整数中的较小值
 * @param a 第一个整数
 * @param b 第二个整数
 * @return 较小值
 */
long long 求最小值(long long a, long long b);

// =============================================================================
// 动态数组函数 - 中文接口
// =============================================================================

#include "cnlang/runtime/collections.h"

/**
 * 创建数组：创建一个动态数组
 * @param 初始容量 初始容量（元素个数）
 * @return 指向动态数组的指针，失败返回 NULL
 */
CnVector* 创建数组(size_t 初始容量);

/**
 * 销毁数组：释放动态数组内存
 * @param 数组 要销毁的数组指针
 */
void 销毁数组(void* 数组);

/**
 * 数组添加：向数组末尾添加元素
 * @param 数组 目标数组
 * @param 元素 要添加的元素指针
 * @return 成功返回 1，失败返回 0
 */
int 数组添加(void* 数组, void* 元素);

/**
 * 数组获取：获取数组中指定索引的元素
 * @param 数组 目标数组
 * @param 索引 元素索引
 * @return 指向元素的指针，索引越界返回 NULL
 */
void* 数组获取(void* 数组, long long 索引);

/**
 * 数组长度：获取数组中元素个数
 * @param 数组 目标数组
 * @return 元素个数
 */
long long 数组长度(void* 数组);

/**
 * 清空数组：清空数组中的所有元素
 * @param 数组 目标数组
 */
void 清空数组(void* 数组);

// =============================================================================
// 哈希表函数 - 中文接口
// =============================================================================

/**
 * 创建哈希表：创建一个字符串键哈希表
 * @param 初始容量 初始桶数量
 * @return 指向哈希表的指针，失败返回 NULL
 */
CnMap* 创建哈希表(size_t 初始容量);

/**
 * 销毁哈希表：释放哈希表内存
 * @param 表 要销毁的哈希表指针
 */
void 销毁哈希表(CnMap* 表);

/**
 * 哈希表插入：插入键值对
 * @param 表 目标哈希表
 * @param 键 字符串键
 * @param 值 值指针
 * @return 成功返回 1，失败返回 0
 */
int 哈希表插入(CnMap* 表, const char* 键, void* 值);

/**
 * 哈希表获取：获取键对应的值
 * @param 表 目标哈希表
 * @param 键 字符串键
 * @return 指向值的指针，键不存在返回 NULL
 */
void* 哈希表获取(CnMap* 表, const char* 键);

/**
 * 哈希表包含：检查键是否存在
 * @param 表 目标哈希表
 * @param 键 字符串键
 * @return 存在返回 1，不存在返回 0
 */
int 哈希表包含(CnMap* 表, const char* 键);

/**
 * 哈希表大小：获取键值对数量
 * @param 表 目标哈希表
 * @return 键值对数量
 */
size_t 哈希表大小(CnMap* 表);

/**
 * 清空哈希表：清空所有键值对
 * @param 表 目标哈希表
 */
void 清空哈希表(CnMap* 表);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_STDLIB_H */
