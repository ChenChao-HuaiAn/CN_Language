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
  long long r0, r2, r3, r4, r6, r8, r9, r10, r11, r12, r14, r16, r17, r18, r20, r21, r23, r24;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;
  _Bool r15;
  _Bool r19;
  _Bool r22;

  entry:
  long long cn_var_i;
  cn_var_i = 0;
  goto for_cond_0;

  for_cond_0:
  r0 = cn_var_i;
  r1 = r0 < 10;
  if (r1) goto for_body_1; else goto for_exit_3;

  for_body_1:
  goto for_update_2;

  for_update_2:
  r2 = cn_var_i;
  r3 = r2 + 1;
  cn_var_i = r3;
  goto for_cond_0;

  for_exit_3:
  long long cn_var_i;
  cn_var_i = 0;
  goto for_cond_4;

  for_cond_4:
  r4 = cn_var_i;
  r5 = r4 < 3;
  if (r5) goto for_body_5; else goto for_exit_7;

  for_body_5:
  long long cn_var_j;
  cn_var_j = 0;
  goto for_cond_8;

  for_update_6:
  r10 = cn_var_i;
  r11 = r10 + 1;
  cn_var_i = r11;
  goto for_cond_4;

  for_exit_7:
  long long cn_var_i;
  cn_var_i = 0;
  goto for_cond_12;

  for_cond_8:
  r6 = cn_var_j;
  r7 = r6 < 4;
  if (r7) goto for_body_9; else goto for_exit_11;

  for_body_9:
  goto for_update_10;

  for_update_10:
  r8 = cn_var_j;
  r9 = r8 + 1;
  cn_var_j = r9;
  goto for_cond_8;

  for_exit_11:
  goto for_update_6;

  for_cond_12:
  r12 = cn_var_i;
  r13 = r12 < 100;
  if (r13) goto for_body_13; else goto for_exit_15;

  for_body_13:
  r14 = cn_var_i;
  r15 = r14 == 10;
  if (r15) goto if_then_16; else goto if_merge_17;

  for_update_14:
  r16 = cn_var_i;
  r17 = r16 + 1;
  cn_var_i = r17;
  goto for_cond_12;

  for_exit_15:
  long long cn_var_i;
  cn_var_i = 0;
  goto for_cond_18;

  if_then_16:
  goto for_exit_15;
  goto if_merge_17;

  if_merge_17:
  goto for_update_14;

  for_cond_18:
  r18 = cn_var_i;
  r19 = r18 < 10;
  if (r19) goto for_body_19; else goto for_exit_21;

  for_body_19:
  r20 = cn_var_i;
  r21 = r20 % 2;
  r22 = r21 == 0;
  if (r22) goto if_then_22; else goto if_merge_23;

  for_update_20:
  r23 = cn_var_i;
  r24 = r23 + 1;
  cn_var_i = r24;
  goto for_cond_18;

  for_exit_21:
  return 0;

  if_then_22:
  goto for_update_20;
  goto if_merge_23;

  if_merge_23:
  goto for_update_20;
  cn_rt_exit();
}

