#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2, r5, r6, r7, r8, r9, r11;
  char* r0;
  char* r1;
  char* r4;
  char* r10;
  char* r13;
  char* r16;
  _Bool r3;
  _Bool r12;
  double r14;
  double r15;

  entry:
  cn_rt_print_string("=== 通用输入演示 ===\n");
  cn_rt_print_string("请输入任意内容：");
  char* cn_var_输入;
  r0 = cn_rt_read_line();
  cn_var_输入 = r0;
  r1 = cn_var_输入;
  r2 = cn_rt_is_int_str(r1);
  r3 = r2 == 1;
  if (r3) goto if_then_0; else goto if_else_1;

  if_then_0:
  long long cn_var_整数值;
  r4 = cn_var_输入;
  r5 = cn_rt_str_to_int(r4);
  cn_var_整数值 = r5;
  cn_rt_print_string("识别为整数：");
  r6 = cn_var_整数值;
  cn_rt_print_int(r6);
  cn_rt_print_string("\n");
  long long cn_var_倍数;
  r7 = cn_var_整数值;
  r8 = r7 * 2;
  cn_var_倍数 = r8;
  cn_rt_print_string("它的两倍是：");
  r9 = cn_var_倍数;
  cn_rt_print_int(r9);
  cn_rt_print_string("\n");
  goto if_merge_2;

  if_else_1:
  r10 = cn_var_输入;
  r11 = cn_rt_is_numeric_str(r10);
  r12 = r11 == 1;
  if (r12) goto if_then_3; else goto if_else_4;

  if_merge_2:
  return 0;

  if_then_3:
  double cn_var_小数值;
  r13 = cn_var_输入;
  r14 = cn_rt_str_to_float(r13);
  cn_var_小数值 = r14;
  cn_rt_print_string("识别为小数：");
  r15 = cn_var_小数值;
  cn_rt_print_float(r15);
  cn_rt_print_string("\n");
  goto if_merge_5;

  if_else_4:
  cn_rt_print_string("识别为字符串：");
  r16 = cn_var_输入;
  cn_rt_print_string(r16);
  cn_rt_print_string("\n");
  goto if_merge_5;

  if_merge_5:
  goto if_merge_2;
  cn_rt_exit();
}

