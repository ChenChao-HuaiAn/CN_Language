#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Forward Declarations - 从导入模块
enum 词元类型枚举 {
    错误 = 81,
    结束 = 80,
    问号 = 79,
    冒号 = 78,
    点 = 77,
    逗号 = 76,
    分号 = 75,
    右方括号 = 74,
    左方括号 = 73,
    右大括号 = 72,
    左大括号 = 71,
    右括号 = 70,
    左括号 = 69,
    箭头 = 68,
    自减 = 67,
    自增 = 66,
    右移 = 65,
    左移 = 64,
    按位取反 = 63,
    按位异或 = 62,
    按位或 = 61,
    按位与 = 60,
    逻辑非 = 59,
    逻辑或 = 58,
    逻辑与 = 57,
    大于等于 = 56,
    大于 = 55,
    小于等于 = 54,
    小于 = 53,
    不等于 = 52,
    赋值 = 51,
    等于 = 50,
    百分号 = 49,
    斜杠 = 48,
    星号 = 47,
    减号 = 46,
    加号 = 45,
    字符字面量 = 44,
    字符串字面量 = 43,
    浮点字面量 = 42,
    整数字面量 = 41,
    标识符 = 40,
    关键字_最终 = 39,
    关键字_抛出 = 38,
    关键字_捕获 = 37,
    关键字_尝试 = 36,
    关键字_基类 = 35,
    关键字_自身 = 34,
    关键字_实现 = 33,
    关键字_抽象 = 32,
    关键字_重写 = 31,
    关键字_虚拟 = 30,
    关键字_保护 = 29,
    关键字_接口 = 28,
    关键字_类 = 27,
    关键字_无 = 26,
    关键字_假 = 25,
    关键字_真 = 24,
    关键字_静态 = 23,
    关键字_私有 = 22,
    关键字_公开 = 21,
    关键字_从 = 20,
    关键字_导入 = 19,
    关键字_变量 = 18,
    关键字_函数 = 17,
    关键字_枚举 = 16,
    关键字_结构体 = 15,
    关键字_空类型 = 14,
    关键字_布尔 = 13,
    关键字_字符串 = 12,
    关键字_小数 = 11,
    关键字_整数 = 10,
    关键字_默认 = 9,
    关键字_情况 = 8,
    关键字_选择 = 7,
    关键字_继续 = 6,
    关键字_中断 = 5,
    关键字_返回 = 4,
    关键字_循环 = 3,
    关键字_当 = 2,
    关键字_否则 = 1,
    关键字_如果 = 0
};
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};
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
enum 词元类型枚举 查找关键字(char*);
_Bool 是关键字字符串(char*);
long long 关键字总数();
struct 关键字条目* 获取关键字表();
void 初始化关键字表();

enum 词元类型枚举 查找关键字(char* cn_var_名称);
_Bool 是关键字字符串(char* cn_var_名称);
long long 关键字总数();
struct 关键字条目* 获取关键字表();
void 初始化关键字表();
enum 词元类型枚举 查找关键字(char* cn_var_名称) {
  long long r2, r3, r7, r10, r13, r15, r16, r17;
  char* r5;
  char* r9;
  void* r6;
  void* r12;
  _Bool r0;
  _Bool r1;
  _Bool r4;
  _Bool r11;
  struct 关键字条目 r8;
  struct 关键字条目 r14;

  entry:
  r0 = cn_var_关键字表已初始化;
  r1 = !r0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  初始化关键字表();
  goto if_merge_1;

  if_merge_1:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2;

  for_cond_2:
  r2 = cn_var_i_0;
  r3 = cn_var_关键字表大小;
  r4 = r2 < r3;
  if (r4) goto for_body_3; else goto for_exit_5;

  for_body_3:
  r5 = cn_var_名称;
  r6 = cn_var_关键字表;
  r7 = cn_var_i_0;
  r8 = *(struct 关键字条目*)cn_rt_array_get_element(r6, r7, 8);
  r9 = r8.名称;
  r10 = 比较字符串(r5, r9);
  r11 = r10 == 0;
  if (r11) goto if_then_6; else goto if_merge_7;

  for_update_4:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_2;

  for_exit_5:
  return 40;

  if_then_6:
  r12 = cn_var_关键字表;
  r13 = cn_var_i_0;
  r14 = *(struct 关键字条目*)cn_rt_array_get_element(r12, r13, 8);
  r15 = r14.类型;
  return r15;
  goto if_merge_7;

  if_merge_7:
  goto for_update_4;
  return 0;
}

_Bool 是关键字字符串(char* cn_var_名称) {
  long long r1;
  char* r0;
  _Bool r3;
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
  if (r0) goto if_then_8; else goto if_merge_9;

  if_then_8:
  return;
  goto if_merge_9;

  if_merge_9:
  cn_var_关键字表已初始化 = 1;
  return;
}

