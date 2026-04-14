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

// CN Language Global Struct Forward Declarations
struct 词元;

// CN Language Global Struct Definitions
struct 词元 {
    enum 词元类型枚举 类型;
    char* 内容;
    long long 行号;
    long long 列号;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  enum 词元类型枚举* r10;
  enum 词元类型枚举* r12;
  enum 词元类型枚举* r14;
  enum 词元类型枚举* r16;
  enum 词元类型枚举* r20;
  enum 词元类型枚举* r24;
  enum 词元类型枚举* r11;
  enum 词元类型枚举* r13;
  enum 词元类型枚举* r15;
  enum 词元类型枚举* r17;
  enum 词元类型枚举* r18;
  enum 词元类型枚举* r21;
  enum 词元类型枚举* r22;
  enum 词元类型枚举* r25;
  enum 词元类型枚举* r26;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r6;
  _Bool r8;
  _Bool r9;
  _Bool r19;
  _Bool r23;
  _Bool r27;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r4;
  enum 二元运算符 r7;

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
  r4 = cn_var_类型_0;
  r5 = r4 == 词元类型枚举_标识符;
  if (r5) goto case_body_7; else goto switch_check_10;

  switch_check_10:
  r6 = r4 == 词元类型枚举_整数字面量;
  if (r6) goto case_body_8; else goto case_default_9;

  case_body_7:
  goto switch_merge_6;
  goto switch_merge_6;

  case_body_8:
  return 3;
  goto switch_merge_6;

  case_default_9:
  return 4;
  goto switch_merge_6;

  switch_merge_6:
  enum 二元运算符 cn_var_运算符_2;
  cn_var_运算符_2 = 二元运算符_加法;
  r7 = cn_var_运算符_2;
  r8 = r7 == 二元运算符_加法;
  if (r8) goto case_body_12; else goto switch_check_15;

  switch_check_15:
  r9 = r7 == 二元运算符_减法;
  if (r9) goto case_body_13; else goto case_default_14;

  case_body_12:
  goto switch_merge_11;
  goto switch_merge_11;

  case_body_13:
  return 5;
  goto switch_merge_11;

  case_default_14:
  return 6;
  goto switch_merge_11;

  switch_merge_11:
  enum 词元类型枚举* cn_var_类型数组;
  cn_var_类型数组 = cn_rt_array_alloc(8, 3);
  r10 = cn_var_类型数组;
  r11 = &r10[0];
  r11 = 词元类型枚举_标识符;
  r12 = cn_var_类型数组;
  r13 = &r12[1];
  r13 = 词元类型枚举_整数字面量;
  r14 = cn_var_类型数组;
  r15 = &r14[2];
  r15 = 词元类型枚举_字符串字面量;
  r16 = cn_var_类型数组;
  r17 = &r16[0];
  r18 = r17;
  r19 = r18 != 词元类型枚举_标识符;
  if (r19) goto if_then_16; else goto if_merge_17;

  if_then_16:
  return 7;
  goto if_merge_17;

  if_merge_17:
  r20 = cn_var_类型数组;
  r21 = &r20[1];
  r22 = r21;
  r23 = r22 != 词元类型枚举_整数字面量;
  if (r23) goto if_then_18; else goto if_merge_19;

  if_then_18:
  return 8;
  goto if_merge_19;

  if_merge_19:
  r24 = cn_var_类型数组;
  r25 = &r24[2];
  r26 = r25;
  r27 = r26 != 词元类型枚举_字符串字面量;
  if (r27) goto if_then_20; else goto if_merge_21;

  if_then_20:
  return 9;
  goto if_merge_21;

  if_merge_21:
  return 0;
  cn_rt_exit();
}

