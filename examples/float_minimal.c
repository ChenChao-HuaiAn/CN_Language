#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();

  entry:
  double cn_var_f;
  cn_var_f = 3.140000;
  return 0;
  cn_rt_exit();
}

