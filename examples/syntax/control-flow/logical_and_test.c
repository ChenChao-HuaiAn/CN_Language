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
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  long long cn_var_x;
  cn_var_x = 2;
  long long cn_var_y;
  cn_var_y = 3;
  r1 = cn_var_x;
  r2 = r1 == 2;
  if (r2) goto logic_rhs_2; else goto logic_merge_3;

  if_then_0:
  cn_rt_print_int(1);
  goto if_merge_1;

  if_merge_1:
  return 0;

  logic_rhs_2:
  r3 = cn_var_y;
  r4 = r3 == 3;
  goto logic_merge_3;

  logic_merge_3:
  if (r4) goto if_then_0; else goto if_merge_1;
  cn_rt_exit();
}

