#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
char* cn_func_判断等级(long long);
long long main();

char* cn_func_判断等级(long long cn_var_分数) {
  long long r0, r2, r4, r6;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r7;

  entry:
  r0 = cn_var_分数;
  r1 = r0 >= 90;
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  return "A";
  goto if_merge_2;

  if_else_1:
  r2 = cn_var_分数;
  r3 = r2 >= 80;
  if (r3) goto if_then_3; else goto if_else_4;

  if_merge_2:

  if_then_3:
  return "B";
  goto if_merge_5;

  if_else_4:
  r4 = cn_var_分数;
  r5 = r4 >= 70;
  if (r5) goto if_then_6; else goto if_else_7;

  if_merge_5:
  goto if_merge_2;

  if_then_6:
  return "C";
  goto if_merge_8;

  if_else_7:
  r6 = cn_var_分数;
  r7 = r6 >= 60;
  if (r7) goto if_then_9; else goto if_else_10;

  if_merge_8:
  goto if_merge_5;

  if_then_9:
  return "D";
  goto if_merge_11;

  if_else_10:
  return "F";
  goto if_merge_11;

  if_merge_11:
  goto if_merge_8;
}

long long main() {
  cn_rt_init();
  long long r0, r2, r4, r6, r7, r8, r10, r12;
  char* r9;
  char* r11;
  char* r13;
  _Bool r1;
  _Bool r3;
  _Bool r5;

  entry:
  long long cn_var_计数器;
  cn_var_计数器 = 0;
  goto while_cond_12;

  while_cond_12:
  r0 = cn_var_计数器;
  r1 = r0 < 5;
  if (r1) goto while_body_13; else goto while_exit_14;

  while_body_13:
  r2 = cn_var_计数器;
  r3 = r2 == 0;
  if (r3) goto if_then_15; else goto if_else_16;

  while_exit_14:
  long long cn_var_分数1;
  cn_var_分数1 = 95;
  long long cn_var_分数2;
  cn_var_分数2 = 75;
  long long cn_var_分数3;
  cn_var_分数3 = 55;
  r8 = cn_var_分数1;
  r9 = cn_func_判断等级(r8);
  cn_rt_print_string(r9);
  r10 = cn_var_分数2;
  r11 = cn_func_判断等级(r10);
  cn_rt_print_string(r11);
  r12 = cn_var_分数3;
  r13 = cn_func_判断等级(r12);
  cn_rt_print_string(r13);
  return 0;

  if_then_15:
  cn_rt_print_string("开始");
  goto if_merge_17;

  if_else_16:
  r4 = cn_var_计数器;
  r5 = r4 == 4;
  if (r5) goto if_then_18; else goto if_else_19;

  if_merge_17:
  r6 = cn_var_计数器;
  r7 = r6 + 1;
  cn_var_计数器 = r7;
  goto while_cond_12;

  if_then_18:
  cn_rt_print_string("结束");
  goto if_merge_20;

  if_else_19:
  cn_rt_print_string("中间");
  goto if_merge_20;

  if_merge_20:
  goto if_merge_17;
  cn_rt_exit();
}

