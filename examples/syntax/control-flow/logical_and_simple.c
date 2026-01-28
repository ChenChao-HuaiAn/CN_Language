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
  _Bool cn_var_result;
  r1 = cn_var_x;
  r2 = r1 == 2;
  if (r2) goto logic_rhs_0; else goto logic_merge_1;

  logic_rhs_0:
  r3 = cn_var_y;
  r4 = r3 == 3;
  goto logic_merge_1;

  logic_merge_1:
  cn_var_result = r4;
  return 0;
  cn_rt_exit();
}

