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
  void* r12;
  void* r13;
  void* r14;
  void* r17;
  void* r20;
  enum 词元类型枚举* r15;
  enum 词元类型枚举* r18;
  enum 词元类型枚举* r21;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  _Bool r7;
  _Bool r9;
  _Bool r10;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  struct 词元 r2;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r5;
  enum 二元运算符 r8;

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
  r5 = cn_var_类型_0;
  r6 = r5 == 0;
  if (r6) goto case_body_7; else goto switch_check_10;

  switch_check_10:
  r7 = r5 == 1;
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
  cn_var_运算符_2 = 0;
  r8 = cn_var_运算符_2;
  r9 = r8 == 0;
  if (r9) goto case_body_12; else goto switch_check_15;

  switch_check_15:
  r10 = r8 == 1;
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
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r11, 0, &_tmp_i0, 8); }
  r12 = cn_var_类型数组;
    { long long _tmp_i1 = 1; cn_rt_array_set_element(r12, 1, &_tmp_i1, 8); }
  r13 = cn_var_类型数组;
    { long long _tmp_i2 = 2; cn_rt_array_set_element(r13, 2, &_tmp_i2, 8); }
  r14 = cn_var_类型数组;
  r15 = *(enum 词元类型枚举**)cn_rt_array_get_element(r14, 0, 8);
  r16 = r15 != 0;
  if (r16) goto if_then_16; else goto if_merge_17;

  if_then_16:
  return 7;
  goto if_merge_17;

  if_merge_17:
  r17 = cn_var_类型数组;
  r18 = *(enum 词元类型枚举**)cn_rt_array_get_element(r17, 1, 8);
  r19 = r18 != 1;
  if (r19) goto if_then_18; else goto if_merge_19;

  if_then_18:
  return 8;
  goto if_merge_19;

  if_merge_19:
  r20 = cn_var_类型数组;
  r21 = *(enum 词元类型枚举**)cn_rt_array_get_element(r20, 2, 8);
  r22 = r21 != 2;
  if (r22) goto if_then_20; else goto if_merge_21;

  if_then_20:
  return 9;
  goto if_merge_21;

  if_merge_21:
  return 0;
  cn_rt_exit();
}

