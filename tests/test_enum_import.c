#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元类型枚举_标识符 = 0,
    词元类型枚举_整数字面量 = 1,
    词元类型枚举_字符串字面量 = 2,
    词元类型枚举_运算符 = 3,
    词元类型枚举_分隔符 = 4,
    词元类型枚举_关键字 = 5,
    词元类型枚举_错误 = 6
};
enum 二元运算符 {
    二元运算符_加法 = 0,
    二元运算符_减法 = 1,
    二元运算符_乘法 = 2,
    二元运算符_除法 = 3,
    二元运算符_取模 = 4
};

// Struct Definitions - 从导入模块
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 内容;
    long long 行号;
    long long 列号;
};

// Struct Forward Declarations - 用于函数参数
struct 词元;
struct 词元类型枚举;

// Forward Declarations - 从导入模块
struct 词元 创建词元(enum 词元类型枚举, const char*, long long, long long);
char* 获取类型名称(enum 词元类型枚举);

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  char* r8;
  enum 词元类型枚举* r12;
  enum 词元类型枚举* r14;
  enum 词元类型枚举* r16;
  enum 词元类型枚举* r18;
  enum 词元类型枚举* r22;
  enum 词元类型枚举* r26;
  enum 词元类型枚举* r13;
  enum 词元类型枚举* r15;
  enum 词元类型枚举* r17;
  enum 词元类型枚举* r19;
  enum 词元类型枚举* r20;
  enum 词元类型枚举* r23;
  enum 词元类型枚举* r24;
  enum 词元类型枚举* r27;
  enum 词元类型枚举* r28;
  _Bool r1;
  _Bool r3;
  _Bool r6;
  _Bool r10;
  _Bool r11;
  _Bool r21;
  _Bool r25;
  _Bool r29;
  struct 词元 r4;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r7;
  enum 二元运算符 r9;

  entry:
  enum 词元类型枚举 cn_var_类型_0;
  cn_var_类型_0 = 词元类型枚举_标识符;
  r0 = cn_var_类型_0;
  r1 = r0 == 词元类型枚举_标识符;
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  goto if_merge_2;

  if_else_1:
  return 1;
  goto if_merge_2;

  if_merge_2:
  struct 词元 cn_var_词元实例_1;
  r2 = cn_var_词元实例_1.类型;
  r3 = r2 == 词元类型枚举_关键字;
  if (r3) goto if_then_3; else goto if_else_4;

  if_then_3:
  goto if_merge_5;

  if_else_4:
  return 2;
  goto if_merge_5;

  if_merge_5:
  struct 词元 cn_var_另一个词元_2;
  r4 = 创建词元(词元类型枚举_整数字面量, "123", 2, 5);
  cn_var_另一个词元_2 = r4;
  r5 = cn_var_另一个词元_2.类型;
  r6 = r5 != 词元类型枚举_整数字面量;
  if (r6) goto if_then_6; else goto if_merge_7;

  if_then_6:
  return 3;
  goto if_merge_7;

  if_merge_7:
  char* cn_var_类型名称_3;
  r7 = cn_var_另一个词元_2.类型;
  r8 = 获取类型名称(r7);
  cn_var_类型名称_3 = r8;
  enum 二元运算符 cn_var_运算符_4;
  cn_var_运算符_4 = 二元运算符_加法;
  r9 = cn_var_运算符_4;
  r10 = r9 == 二元运算符_加法;
  if (r10) goto case_body_9; else goto switch_check_12;

  switch_check_12:
  r11 = r9 == 二元运算符_减法;
  if (r11) goto case_body_10; else goto case_default_11;

  case_body_9:
  goto switch_merge_8;
  goto switch_merge_8;

  case_body_10:
  return 4;
  goto switch_merge_8;

  case_default_11:
  return 5;
  goto switch_merge_8;

  switch_merge_8:
  enum 词元类型枚举* cn_var_类型数组;
  cn_var_类型数组 = cn_rt_array_alloc(8, 3);
  r12 = cn_var_类型数组;
  r13 = &r12[0];
  r13 = 词元类型枚举_标识符;
  r14 = cn_var_类型数组;
  r15 = &r14[1];
  r15 = 词元类型枚举_整数字面量;
  r16 = cn_var_类型数组;
  r17 = &r16[2];
  r17 = 词元类型枚举_字符串字面量;
  r18 = cn_var_类型数组;
  r19 = &r18[0];
  r20 = r19;
  r21 = r20 != 词元类型枚举_标识符;
  if (r21) goto if_then_13; else goto if_merge_14;

  if_then_13:
  return 6;
  goto if_merge_14;

  if_merge_14:
  r22 = cn_var_类型数组;
  r23 = &r22[1];
  r24 = r23;
  r25 = r24 != 词元类型枚举_整数字面量;
  if (r25) goto if_then_15; else goto if_merge_16;

  if_then_15:
  return 7;
  goto if_merge_16;

  if_merge_16:
  r26 = cn_var_类型数组;
  r27 = &r26[2];
  r28 = r27;
  r29 = r28 != 词元类型枚举_字符串字面量;
  if (r29) goto if_then_17; else goto if_merge_18;

  if_then_17:
  return 8;
  goto if_merge_18;

  if_merge_18:
  return 0;
  cn_rt_exit();
}

