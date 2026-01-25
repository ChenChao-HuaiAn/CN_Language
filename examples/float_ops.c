#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  double r0;
  double r1;
  double r2;

  entry:
  double cn_var_a;
  cn_var_a = 1.500000;
  double cn_var_b;
  cn_var_b = 2.500000;
  double cn_var_c;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  cn_var_c = r2;
  return 0;
  cn_rt_exit();
}

