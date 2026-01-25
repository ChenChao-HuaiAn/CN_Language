#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  _Bool r0;

  entry:
  _Bool cn_var_x;
  cn_var_x = 1;
  _Bool cn_var_y;
  cn_var_y = 0;
  r0 = cn_var_x;
  if (r0) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("x是真");
  goto if_merge_1;

  if_merge_1:
  cn_rt_exit();
}

