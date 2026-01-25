#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r6;

  entry:
  double cn_var_x;
  cn_var_x = /* NONE */;
  double cn_var_y;
  cn_var_y = /* NONE */;
  double cn_var_sum;
  r0 = cn_var_x;
  r1 = cn_var_y;
  r2 = r0 + r1;
  cn_var_sum = r2;
  double cn_var_prod;
  r3 = cn_var_x;
  r4 = cn_var_y;
  r5 = r3 * r4;
  cn_var_prod = r5;
  int cn_var_i;
  cn_var_i = 10;
  double cn_var_result;
  r6 = cn_var_i;
  r7 = cn_var_x;
  r8 = r6 + r7;
  cn_var_result = r8;
  double cn_var_auto_float;
  cn_var_auto_float = /* NONE */;
  return 0;
  cn_rt_exit();
}

