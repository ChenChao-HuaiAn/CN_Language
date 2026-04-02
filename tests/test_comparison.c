#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 测试比较运算符();
long long 测试复杂表达式();

long long 测试比较运算符() {
  long long r0, r1, r3, r4, r6, r7, r9, r10, r12, r13, r15, r16;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r11;
  _Bool r14;
  _Bool r17;

  entry:
  long long cn_var_a_0;
  cn_var_a_0 = 10;
  long long cn_var_b_1;
  cn_var_b_1 = 20;
  r0 = cn_var_a_0;
  r1 = cn_var_b_1;
  r2 = r0 < r1;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  r3 = cn_var_a_0;
  r4 = cn_var_b_1;
  r5 = r3 <= r4;
  if (r5) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return 2;
  goto if_merge_3;

  if_merge_3:
  r6 = cn_var_a_0;
  r7 = cn_var_b_1;
  r8 = r6 > r7;
  if (r8) goto if_then_4; else goto if_merge_5;

  if_then_4:
  return 3;
  goto if_merge_5;

  if_merge_5:
  r9 = cn_var_a_0;
  r10 = cn_var_b_1;
  r11 = r9 >= r10;
  if (r11) goto if_then_6; else goto if_merge_7;

  if_then_6:
  return 4;
  goto if_merge_7;

  if_merge_7:
  r12 = cn_var_a_0;
  r13 = cn_var_b_1;
  r14 = r12 == r13;
  if (r14) goto if_then_8; else goto if_merge_9;

  if_then_8:
  return 5;
  goto if_merge_9;

  if_merge_9:
  r15 = cn_var_a_0;
  r16 = cn_var_b_1;
  r17 = r15 != r16;
  if (r17) goto if_then_10; else goto if_merge_11;

  if_then_10:
  return 6;
  goto if_merge_11;

  if_merge_11:
  return 0;
}

long long 测试复杂表达式() {
  long long r0, r1, r2, r4, r5, r7, r8, r9, r11, r12;
  _Bool r3;
  _Bool r6;
  _Bool r10;
  _Bool r13;

  entry:
  long long cn_var_x_0;
  cn_var_x_0 = 5;
  long long cn_var_y_1;
  cn_var_y_1 = 10;
  long long cn_var_z_2;
  cn_var_z_2 = 15;
  r1 = cn_var_x_0;
  r2 = cn_var_y_1;
  r3 = r1 < r2;
  if (r3) goto logic_rhs_14; else goto logic_merge_15;

  if_then_12:
  return 1;
  goto if_merge_13;

  if_merge_13:
  r8 = cn_var_x_0;
  r9 = cn_var_y_1;
  r10 = r8 < r9;
  if (r10) goto logic_merge_19; else goto logic_rhs_18;

  logic_rhs_14:
  r4 = cn_var_y_1;
  r5 = cn_var_z_2;
  r6 = r4 < r5;
  goto logic_merge_15;

  logic_merge_15:
  if (r6) goto if_then_12; else goto if_merge_13;

  if_then_16:
  return 2;
  goto if_merge_17;

  if_merge_17:
  return 0;

  logic_rhs_18:
  r11 = cn_var_x_0;
  r12 = cn_var_z_2;
  r13 = r11 > r12;
  goto logic_merge_19;

  logic_merge_19:
  if (r13) goto if_then_16; else goto if_merge_17;
  return 0;
}

