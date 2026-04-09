#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元类型枚举_错误 = 81,
    词元类型枚举_结束 = 80,
    词元类型枚举_问号 = 79,
    词元类型枚举_冒号 = 78,
    词元类型枚举_点 = 77,
    词元类型枚举_逗号 = 76,
    词元类型枚举_分号 = 75,
    词元类型枚举_右方括号 = 74,
    词元类型枚举_左方括号 = 73,
    词元类型枚举_右大括号 = 72,
    词元类型枚举_左大括号 = 71,
    词元类型枚举_右括号 = 70,
    词元类型枚举_左括号 = 69,
    词元类型枚举_箭头 = 68,
    词元类型枚举_自减 = 67,
    词元类型枚举_自增 = 66,
    词元类型枚举_右移 = 65,
    词元类型枚举_左移 = 64,
    词元类型枚举_按位取反 = 63,
    词元类型枚举_按位异或 = 62,
    词元类型枚举_按位或 = 61,
    词元类型枚举_按位与 = 60,
    词元类型枚举_逻辑非 = 59,
    词元类型枚举_逻辑或 = 58,
    词元类型枚举_逻辑与 = 57,
    词元类型枚举_大于等于 = 56,
    词元类型枚举_大于 = 55,
    词元类型枚举_小于等于 = 54,
    词元类型枚举_小于 = 53,
    词元类型枚举_不等于 = 52,
    词元类型枚举_赋值 = 51,
    词元类型枚举_等于 = 50,
    词元类型枚举_百分号 = 49,
    词元类型枚举_斜杠 = 48,
    词元类型枚举_星号 = 47,
    词元类型枚举_减号 = 46,
    词元类型枚举_加号 = 45,
    词元类型枚举_字符字面量 = 44,
    词元类型枚举_字符串字面量 = 43,
    词元类型枚举_浮点字面量 = 42,
    词元类型枚举_整数字面量 = 41,
    词元类型枚举_标识符 = 40,
    词元类型枚举_关键字_最终 = 39,
    词元类型枚举_关键字_抛出 = 38,
    词元类型枚举_关键字_捕获 = 37,
    词元类型枚举_关键字_尝试 = 36,
    词元类型枚举_关键字_基类 = 35,
    词元类型枚举_关键字_自身 = 34,
    词元类型枚举_关键字_实现 = 33,
    词元类型枚举_关键字_抽象 = 32,
    词元类型枚举_关键字_重写 = 31,
    词元类型枚举_关键字_虚拟 = 30,
    词元类型枚举_关键字_保护 = 29,
    词元类型枚举_关键字_接口 = 28,
    词元类型枚举_关键字_类 = 27,
    词元类型枚举_关键字_无 = 26,
    词元类型枚举_关键字_假 = 25,
    词元类型枚举_关键字_真 = 24,
    词元类型枚举_关键字_静态 = 23,
    词元类型枚举_关键字_私有 = 22,
    词元类型枚举_关键字_公开 = 21,
    词元类型枚举_关键字_从 = 20,
    词元类型枚举_关键字_导入 = 19,
    词元类型枚举_关键字_变量 = 18,
    词元类型枚举_关键字_函数 = 17,
    词元类型枚举_关键字_枚举 = 16,
    词元类型枚举_关键字_结构体 = 15,
    词元类型枚举_关键字_空类型 = 14,
    词元类型枚举_关键字_布尔 = 13,
    词元类型枚举_关键字_字符串 = 12,
    词元类型枚举_关键字_小数 = 11,
    词元类型枚举_关键字_整数 = 10,
    词元类型枚举_关键字_默认 = 9,
    词元类型枚举_关键字_情况 = 8,
    词元类型枚举_关键字_选择 = 7,
    词元类型枚举_关键字_继续 = 6,
    词元类型枚举_关键字_中断 = 5,
    词元类型枚举_关键字_返回 = 4,
    词元类型枚举_关键字_循环 = 3,
    词元类型枚举_关键字_当 = 2,
    词元类型枚举_关键字_否则 = 1,
    词元类型枚举_关键字_如果 = 0
};
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块
void* 数组获取(void*, long long);
long long 获取位置参数个数(void);
char* 获取位置参数(long long);
long long 选项存在(char*);
char* 查找选项(char*);
char* 获取程序名称(void);
char* 获取参数(long long);
long long 获取参数个数(void);
long long 求最小值(long long, long long);
long long 求最大值(long long, long long);
long long 获取绝对值(long long);
char* 读取行(void);
long long 字符串格式化(char*, long long, char*);
char* 字符串格式(char*);
long long 类型大小(long long);
void* 分配内存数组(long long, long long);

// CN Language Global Struct Forward Declarations
struct 关键字条目;

// CN Language Global Struct Definitions
struct 关键字条目 {
    char* 名称;
    long long 类型;
};

// Global Variables
long long cn_var_关键字表大小 = 40;
struct 关键字条目* cn_var_关键字表 = 0;
_Bool cn_var_关键字表已初始化 = 0;

// Forward Declarations
enum 词元类型枚举 查找关键字(char*);
_Bool 是关键字字符串(char*);
long long 关键字总数();
struct 关键字条目* 获取关键字表();
void 初始化关键字表();

enum 词元类型枚举 查找关键字(char* cn_var_名称) {
  long long r1, r2, r3, r4, r7, r9, r10, r11, r13, r15, r16, r17;
  char* r5;
  void* r6;
  void* r12;
  void* r8;
  void* r14;
  _Bool r0;

  entry:
  r0 = cn_var_关键字表已初始化;
  r1 = !r0;
  if (r1) goto if_then_194; else goto if_merge_195;

  if_then_194:
  初始化关键字表();
  goto if_merge_195;

  if_merge_195:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_196;

  for_cond_196:
  r2 = cn_var_i_0;
  r3 = cn_var_关键字表大小;
  r4 = r2 < r3;
  if (r4) goto for_body_197; else goto for_exit_199;

  for_body_197:
  r5 = cn_var_名称;
  r6 = cn_var_关键字表;
  r7 = cn_var_i_0;
  r8 = *(void**)cn_rt_array_get_element(r6, r7, 8);
  r9 = r8->名称;
  r10 = 比较字符串(r5, r9);
  r11 = r10 == 0;
  if (r11) goto if_then_200; else goto if_merge_201;

  for_update_198:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_196;

  for_exit_199:
  return 40;

  if_then_200:
  r12 = cn_var_关键字表;
  r13 = cn_var_i_0;
  r14 = *(void**)cn_rt_array_get_element(r12, r13, 8);
  r15 = r14->类型;
  return r15;
  goto if_merge_201;

  if_merge_201:
  goto for_update_198;
  return 0;
}

_Bool 是关键字字符串(char* cn_var_名称) {
  long long r3;
  char* r0;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r2;

  entry:
  enum 词元类型枚举 cn_var_类型_0;
  r0 = cn_var_名称;
  r1 = 查找关键字(r0);
  cn_var_类型_0 = r1;
  r2 = cn_var_类型_0;
  r3 = r2 != 40;
  return r3;
}

long long 关键字总数() {
  long long r0;

  entry:
  r0 = cn_var_关键字表大小;
  return r0;
}

struct 关键字条目* 获取关键字表() {
  void* r0;

  entry:
  r0 = cn_var_关键字表;
  return r0;
}

void 初始化关键字表() {
  _Bool r0;

  entry:
  r0 = cn_var_关键字表已初始化;
  if (r0) goto if_then_202; else goto if_merge_203;

  if_then_202:
  return;
  goto if_merge_203;

  if_merge_203:
  cn_var_关键字表已初始化 = 1;
  return;
}

