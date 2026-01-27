#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42;

  entry:
  int cn_var_a;
  cn_var_a = 10;
  int cn_var_b;
  cn_var_b = 20;
  int cn_var_最大值;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 > r1;
  if (r2) goto ternary_true_0; else goto ternary_false_1;
  cn_rt_exit();
}

