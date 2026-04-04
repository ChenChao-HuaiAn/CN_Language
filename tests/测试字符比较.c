#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 测试字符字面量比较();
_Bool 测试字符范围检查(long long);
long long 测试多个比较();
long long main();

long long 测试字符字面量比较() {
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_c_0;
  cn_var_c_0 = 65;
  r0 = cn_var_c_0;
  r1 = r0 == 65;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
}

_Bool 测试字符范围检查(long long cn_var_c) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_4; else goto logic_merge_5;

  if_then_2:
  return 1;
  goto if_merge_3;

  if_merge_3:
  return 0;

  logic_rhs_4:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_5;

  logic_merge_5:
  if (r4) goto if_then_2; else goto if_merge_3;
  return 0;
}

long long 测试多个比较() {
  long long r0, r1, r3, r4, r6, r7, r9, r10;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r11;

  entry:
  long long cn_var_a_0;
  cn_var_a_0 = 10;
  long long cn_var_b_1;
  cn_var_b_1 = 20;
  long long cn_var_c_2;
  cn_var_c_2 = 30;
  r0 = cn_var_a_0;
  r1 = cn_var_b_1;
  r2 = r0 < r1;
  if (r2) goto if_then_6; else goto if_merge_7;

  if_then_6:
  r3 = cn_var_b_1;
  r4 = cn_var_c_2;
  r5 = r3 <= r4;
  if (r5) goto if_then_8; else goto if_merge_9;

  if_merge_7:
  return 0;

  if_then_8:
  r6 = cn_var_c_2;
  r7 = cn_var_a_0;
  r8 = r6 > r7;
  if (r8) goto if_then_10; else goto if_merge_11;

  if_merge_9:
  goto if_merge_7;

  if_then_10:
  r9 = cn_var_a_0;
  r10 = cn_var_c_2;
  r11 = r9 != r10;
  if (r11) goto if_then_12; else goto if_merge_13;

  if_merge_11:
  goto if_merge_9;

  if_then_12:
  return 1;
  goto if_merge_13;

  if_merge_13:
  goto if_merge_11;
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0, r2, r5, r6, r7;
  _Bool r1;
  _Bool r3;
  _Bool r4;

  entry:
  long long cn_var_结果1_0;
  r0 = 测试字符字面量比较();
  cn_var_结果1_0 = r0;
  _Bool cn_var_结果2_1;
  r1 = 测试字符范围检查(70);
  cn_var_结果2_1 = r1;
  long long cn_var_结果3_2;
  r2 = 测试多个比较();
  cn_var_结果3_2 = r2;
  r3 = cn_var_结果2_1;
  r4 = r3 == 1;
  if (r4) goto if_then_14; else goto if_merge_15;

  if_then_14:
  r5 = cn_var_结果1_0;
  r6 = cn_var_结果3_2;
  r7 = r5 + r6;
  return r7;
  goto if_merge_15;

  if_merge_15:
  return 0;
  cn_rt_exit();
}

