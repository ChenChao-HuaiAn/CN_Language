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
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_n;
  cn_var_n = 5;
  r0 = cn_var_n;
  r1 = r0 < 10;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
  cn_rt_exit();
}

