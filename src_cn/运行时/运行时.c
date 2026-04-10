#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 复制字符串副本(char*);
char* 字符串格式(char*);
long long 字符串格式化(char*, long long, char*);
char* cn_rt_read_line();
long long 获取绝对值(long long);
long long 求最大值(long long, long long);
long long 求最小值(long long, long long);
long long 获取参数个数();
char* 获取参数(long long);
char* 获取程序名称();
char* 查找选项(char*);
long long 选项存在(char*);
char* 获取位置参数(long long);
long long 获取位置参数个数();
void* 数组获取(void*, long long);

void* 分配内存数组(long long cn_var_类型大小, long long cn_var_数量);
long long 类型大小(long long cn_var_类型名);
char* 复制字符串副本(char* cn_var_源);
char* 字符串格式(char* cn_var_格式);
long long 字符串格式化(char* cn_var_缓冲区, long long cn_var_大小, char* cn_var_格式);
char* cn_rt_read_line();
long long 获取绝对值(long long cn_var_n);
long long 求最大值(long long cn_var_a, long long cn_var_b);
long long 求最小值(long long cn_var_a, long long cn_var_b);
long long 获取参数个数();
char* 获取参数(long long cn_var_索引);
char* 获取程序名称();
char* 查找选项(char* cn_var_选项名);
long long 选项存在(char* cn_var_选项名);
char* 获取位置参数(long long cn_var_索引);
long long 获取位置参数个数();
void* 数组获取(void* cn_var_数组, long long cn_var_索引);
