#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型枚举 {
    词元类型枚举_标识符,
    词元类型枚举_整数字面量,
    词元类型枚举_字符串字面量,
    词元类型枚举_运算符,
    词元类型枚举_分隔符,
    词元类型枚举_关键字,
    词元类型枚举_错误
};

enum 二元运算符 {
    二元运算符_加法,
    二元运算符_减法,
    二元运算符_乘法,
    二元运算符_除法,
    二元运算符_取模
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  _Bool r1;
  _Bool r3;
  _Bool r4;
  _Bool r6;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 二元运算符 r5;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r10;

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
  r2 = cn_var_类型_0;
  r3 = r2 == 词元类型枚举_标识符;
  if (r3) goto case_body_4; else goto switch_check_7;

  switch_check_7:
  r4 = r2 == 词元类型枚举_整数字面量;
  if (r4) goto case_body_5; else goto case_default_6;

  case_body_4:
  goto switch_merge_3;
  goto switch_merge_3;

  case_body_5:
  return 2;
  goto switch_merge_3;

  case_default_6:
  return 3;
  goto switch_merge_3;

  switch_merge_3:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_加法;
  r5 = cn_var_运算符_1;
  r6 = r5 == 二元运算符_加法;
  if (r6) goto case_body_9; else goto switch_check_12;

  switch_check_12:
  r7 = r5 == 二元运算符_减法;
  if (r7) goto case_body_10; else goto case_default_11;

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
  enum 词元类型枚举 cn_var_另一个类型_2;
  cn_var_另一个类型_2 = 词元类型枚举_关键字;
  r8 = cn_var_另一个类型_2;
  r9 = r8 == 词元类型枚举_关键字;
  if (r9) goto if_then_13; else goto if_else_14;

  if_then_13:
  goto if_merge_15;

  if_else_14:
  return 6;
  goto if_merge_15;

  if_merge_15:
  r10 = cn_var_类型_0;
  r11 = r10 == 0;
  if (r11) goto if_then_16; else goto if_else_17;

  if_then_16:
  goto if_merge_18;

  if_else_17:
  return 7;
  goto if_merge_18;

  if_merge_18:
  return 0;
  cn_rt_exit();
}

