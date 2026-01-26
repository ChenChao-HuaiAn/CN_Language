#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r0, r3, r4;
  int* r1;
  int* r2;

  entry:
  int cn_var_x;
  cn_var_x = 42;
  int* cn_var_p;
  r0 = cn_var_x;
  r1 = &cn_var_x;
  cn_var_p = r1;
  int cn_var_y;
  r2 = cn_var_p;
  r3 = *r2;
  cn_var_y = r3;
  r4 = cn_var_y;
  return r4;
  cn_rt_exit();
}

