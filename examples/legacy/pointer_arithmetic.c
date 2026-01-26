#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r0;
  int* r1;
  int* r2;
  int* r3;

  entry:
  int cn_var_x;
  cn_var_x = 10;
  int* cn_var_p;
  r0 = cn_var_x;
  r1 = &cn_var_x;
  cn_var_p = r1;
  int* cn_var_q;
  r2 = cn_var_p;
  r3 = r2 + 1;
  cn_var_q = r3;
  return 0;
  cn_rt_exit();
}

