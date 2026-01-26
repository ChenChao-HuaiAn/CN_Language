#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r0, r4;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;

  entry:
  int cn_var_x;
  cn_var_x = 2;
  r0 = cn_var_x;
  r1 = r0 == 1;
  if (r1) goto case_body_1; else goto switch_check_5;

  switch_check_5:
  r2 = r0 == 2;
  if (r2) goto case_body_2; else goto switch_check_6;

  switch_check_6:
  r3 = r0 == 3;
  if (r3) goto case_body_3; else goto case_default_4;

  case_body_1:
  cn_rt_print_string("情况 1");
  goto switch_merge_0;
  goto switch_merge_0;

  case_body_2:
  cn_rt_print_string("情况 2");
  goto switch_merge_0;
  goto switch_merge_0;

  case_body_3:
  cn_rt_print_string("情况 3");
  goto switch_merge_0;
  goto switch_merge_0;

  case_default_4:
  cn_rt_print_string("默认情况");
  goto switch_merge_0;
  goto switch_merge_0;

  switch_merge_0:
  int cn_var_day;
  cn_var_day = 3;
  r4 = cn_var_day;
  r5 = r4 == 1;
  if (r5) goto case_body_8; else goto switch_check_14;

  switch_check_14:
  r6 = r4 == 2;
  if (r6) goto case_body_9; else goto switch_check_15;

  switch_check_15:
  r7 = r4 == 3;
  if (r7) goto case_body_10; else goto switch_check_16;

  switch_check_16:
  r8 = r4 == 4;
  if (r8) goto case_body_11; else goto switch_check_17;

  switch_check_17:
  r9 = r4 == 5;
  if (r9) goto case_body_12; else goto case_default_13;

  case_body_8:
  cn_rt_print_string("星期一");
  goto switch_merge_7;
  goto switch_merge_7;

  case_body_9:
  cn_rt_print_string("星期二");
  goto switch_merge_7;
  goto switch_merge_7;

  case_body_10:
  cn_rt_print_string("星期三");
  goto switch_merge_7;
  goto switch_merge_7;

  case_body_11:
  cn_rt_print_string("星期四");
  goto switch_merge_7;
  goto switch_merge_7;

  case_body_12:
  cn_rt_print_string("星期五");
  goto switch_merge_7;
  goto switch_merge_7;

  case_default_13:
  cn_rt_print_string("周末");
  goto switch_merge_7;
  goto switch_merge_7;

  switch_merge_7:
  return 0;
  cn_rt_exit();
}

