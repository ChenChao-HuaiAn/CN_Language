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
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long);
char* 获取类型名称(enum 词元类型枚举);

struct 词元 创建词元(enum 词元类型枚举 cn_var_类型, char* cn_var_内容, long long cn_var_行号, long long cn_var_列号) {
  long long r2, r3;
  char* r1;
  struct 词元 r4;
  enum 词元类型枚举 r0;

  entry:
  struct 词元 cn_var_结果_0;
  r0 = cn_var_类型;
  r1 = cn_var_内容;
  r2 = cn_var_行号;
  r3 = cn_var_列号;
  r4 = cn_var_结果_0;
  return r4;
}

char* 获取类型名称(enum 词元类型枚举 cn_var_类型) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  enum 词元类型枚举 r0;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 词元类型枚举_标识符;
  if (r1) goto case_body_1; else goto switch_check_9;

  switch_check_9:
  r2 = r0 == 词元类型枚举_整数字面量;
  if (r2) goto case_body_2; else goto switch_check_10;

  switch_check_10:
  r3 = r0 == 词元类型枚举_字符串字面量;
  if (r3) goto case_body_3; else goto switch_check_11;

  switch_check_11:
  r4 = r0 == 词元类型枚举_运算符;
  if (r4) goto case_body_4; else goto switch_check_12;

  switch_check_12:
  r5 = r0 == 词元类型枚举_分隔符;
  if (r5) goto case_body_5; else goto switch_check_13;

  switch_check_13:
  r6 = r0 == 词元类型枚举_关键字;
  if (r6) goto case_body_6; else goto switch_check_14;

  switch_check_14:
  r7 = r0 == 词元类型枚举_错误;
  if (r7) goto case_body_7; else goto case_default_8;

  case_body_1:
  return "标识符";
  goto switch_merge_0;

  case_body_2:
  return "整数字面量";
  goto switch_merge_0;

  case_body_3:
  return "字符串字面量";
  goto switch_merge_0;

  case_body_4:
  return "运算符";
  goto switch_merge_0;

  case_body_5:
  return "分隔符";
  goto switch_merge_0;

  case_body_6:
  return "关键字";
  goto switch_merge_0;

  case_body_7:
  return "错误";
  goto switch_merge_0;

  case_default_8:
  return "未知";
  goto switch_merge_0;

  switch_merge_0:
  return NULL;
}

