#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r0;
  double r1;
  double r2;

  entry:
  int cn_var_i;
  cn_var_i = 10;
  double cn_var_f;
  cn_var_f = 3.140000;
  double cn_var_result;
  r0 = cn_var_i;
  r1 = cn_var_f;
  r2 = r0 + r1;
  cn_var_result = r2;
  return 0;
  cn_rt_exit();
}

