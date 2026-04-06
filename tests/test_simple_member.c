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
  long long r0, r1;
  _Bool r2;

  entry:
  long long cn_var_x_0;
  cn_var_x_0 = 10;
  long long cn_var_y_1;
  cn_var_y_1 = 20;
  r0 = cn_var_x_0;
  r1 = cn_var_y_1;
  r2 = r0 < r1;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
  cn_rt_exit();
}

