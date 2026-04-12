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
  void* r11;
  void* r13;
  void* r15;
  void* r17;
  void* r21;
  void* r25;
  enum 词元类型枚举* r12;
  enum 词元类型枚举* r14;
  enum 词元类型枚举* r16;
  enum 词元类型枚举* r18;
  enum 词元类型枚举* r19;
  enum 词元类型枚举* r22;
  enum 词元类型枚举* r23;
  enum 词元类型枚举* r26;
  enum 词元类型枚举* r27;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  _Bool r7;
  _Bool r9;
  _Bool r10;
  _Bool r20;
  _Bool r24;
  _Bool r28;
  struct 词元 r2;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r5;
  enum 二元运算符 r8;

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
  r2 = cn_var_词元实例_1;
  r3 = r2.类型;
  r4 = r3 == 词元类型枚举_关键字;
  if (r4) goto if_then_3; else goto if_else_4;

  if_then_3:
  goto if_merge_5;

  if_else_4:
  return 2;
  goto if_merge_5;

  if_merge_5:
  r5 = cn_var_类型_0;
  r6 = r5 == 词元类型枚举_标识符;
  if (r6) goto case_body_7; else goto switch_check_10;

  switch_check_10:
  r7 = r5 == 词元类型枚举_整数字面量;
  if (r7) goto case_body_8; else goto case_default_9;

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
  r8 = cn_var_运算符_2;
  r9 = r8 == 二元运算符_加法;
  if (r9) goto case_body_12; else goto switch_check_15;

  switch_check_15:
  r10 = r8 == 二元运算符_减法;
  if (r10) goto case_body_13; else goto case_default_14;

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
  r11 = cn_var_类型数组;
  r12 = &r11[0];
  r12 = 词元类型枚举_标识符;
  r13 = cn_var_类型数组;
  r14 = &r13[1];
  r14 = 词元类型枚举_整数字面量;
  r15 = cn_var_类型数组;
  r16 = &r15[2];
  r16 = 词元类型枚举_字符串字面量;
  r17 = cn_var_类型数组;
  r18 = &r17[0];
  r19 = r18;
  r20 = r19 != 词元类型枚举_标识符;
  if (r20) goto if_then_16; else goto if_merge_17;

  if_then_16:
  return 7;
  goto if_merge_17;

  if_merge_17:
  r21 = cn_var_类型数组;
  r22 = &r21[1];
  r23 = r22;
  r24 = r23 != 词元类型枚举_整数字面量;
  if (r24) goto if_then_18; else goto if_merge_19;

  if_then_18:
  return 8;
  goto if_merge_19;

  if_merge_19:
  r25 = cn_var_类型数组;
  r26 = &r25[2];
  r27 = r26;
  r28 = r27 != 词元类型枚举_字符串字面量;
  if (r28) goto if_then_20; else goto if_merge_21;

  if_then_20:
  return 9;
  goto if_merge_21;

  if_merge_21:
  return 0;
  cn_rt_exit();
}

