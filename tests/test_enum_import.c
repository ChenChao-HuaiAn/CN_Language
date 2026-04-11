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
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long);
char* 获取类型名称(enum 词元类型枚举);

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  char* r11;
  void* r15;
  void* r16;
  void* r17;
  void* r18;
  void* r21;
  void* r24;
  enum 词元类型枚举* r19;
  enum 词元类型枚举* r22;
  enum 词元类型枚举* r25;
  _Bool r1;
  _Bool r4;
  _Bool r8;
  _Bool r13;
  _Bool r14;
  _Bool r20;
  _Bool r23;
  _Bool r26;
  struct 词元 r2;
  struct 词元 r5;
  struct 词元 r6;
  struct 词元 r9;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 二元运算符 r12;

  entry:
  enum 词元类型枚举 cn_var_类型_0;
  cn_var_类型_0 = 0;
  r0 = cn_var_类型_0;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  goto if_merge_2;

  if_else_1:
  return 1;
  goto if_merge_2;

  if_merge_2:
  struct 词元 cn_var_词元实例_1;
  r2 = cn_var_词元实例_1;
  r3 = r2.类型;
  r4 = r3 == 5;
  if (r4) goto if_then_3; else goto if_else_4;

  if_then_3:
  goto if_merge_5;

  if_else_4:
  return 2;
  goto if_merge_5;

  if_merge_5:
  struct 词元 cn_var_另一个词元_2;
  r5 = 创建词元(1, "123", 2, 5);
  cn_var_另一个词元_2 = r5;
  r6 = cn_var_另一个词元_2;
  r7 = r6.类型;
  r8 = r7 != 1;
  if (r8) goto if_then_6; else goto if_merge_7;

  if_then_6:
  return 3;
  goto if_merge_7;

  if_merge_7:
  char* cn_var_类型名称_3;
  r9 = cn_var_另一个词元_2;
  r10 = r9.类型;
  r11 = 获取类型名称(r10);
  cn_var_类型名称_3 = r11;
  enum 二元运算符 cn_var_运算符_4;
  cn_var_运算符_4 = 0;
  r12 = cn_var_运算符_4;
  r13 = r12 == 0;
  if (r13) goto case_body_9; else goto switch_check_12;

  switch_check_12:
  r14 = r12 == 1;
  if (r14) goto case_body_10; else goto case_default_11;

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
  r15 = cn_var_类型数组;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r15, 0, &_tmp_i0, 8); }
  r16 = cn_var_类型数组;
    { long long _tmp_i1 = 1; cn_rt_array_set_element(r16, 1, &_tmp_i1, 8); }
  r17 = cn_var_类型数组;
    { long long _tmp_i2 = 2; cn_rt_array_set_element(r17, 2, &_tmp_i2, 8); }
  r18 = cn_var_类型数组;
  r19 = *(enum 词元类型枚举**)cn_rt_array_get_element(r18, 0, 8);
  r20 = r19 != 0;
  if (r20) goto if_then_13; else goto if_merge_14;

  if_then_13:
  return 6;
  goto if_merge_14;

  if_merge_14:
  r21 = cn_var_类型数组;
  r22 = *(enum 词元类型枚举**)cn_rt_array_get_element(r21, 1, 8);
  r23 = r22 != 1;
  if (r23) goto if_then_15; else goto if_merge_16;

  if_then_15:
  return 7;
  goto if_merge_16;

  if_merge_16:
  r24 = cn_var_类型数组;
  r25 = *(enum 词元类型枚举**)cn_rt_array_get_element(r24, 2, 8);
  r26 = r25 != 2;
  if (r26) goto if_then_17; else goto if_merge_18;

  if_then_17:
  return 8;
  goto if_merge_18;

  if_merge_18:
  return 0;
  cn_rt_exit();
}

