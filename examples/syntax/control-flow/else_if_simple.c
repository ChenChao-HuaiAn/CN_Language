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
  long long r0, r2, r4, r6;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r7;

  entry:
  long long cn_var_分数;
  cn_var_分数 = 85;
  r0 = cn_var_分数;
  r1 = r0 >= 90;
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  cn_rt_print_string("优秀");
  goto if_merge_2;

  if_else_1:
  r2 = cn_var_分数;
  r3 = r2 >= 80;
  if (r3) goto if_then_3; else goto if_else_4;

  if_merge_2:
  return 0;

  if_then_3:
  cn_rt_print_string("良好");
  goto if_merge_5;

  if_else_4:
  r4 = cn_var_分数;
  r5 = r4 >= 70;
  if (r5) goto if_then_6; else goto if_else_7;

  if_merge_5:
  goto if_merge_2;

  if_then_6:
  cn_rt_print_string("中等");
  goto if_merge_8;

  if_else_7:
  r6 = cn_var_分数;
  r7 = r6 >= 60;
  if (r7) goto if_then_9; else goto if_else_10;

  if_merge_8:
  goto if_merge_5;

  if_then_9:
  cn_rt_print_string("及格");
  goto if_merge_11;

  if_else_10:
  cn_rt_print_string("不及格");
  goto if_merge_11;

  if_merge_11:
  goto if_merge_8;
  cn_rt_exit();
}

