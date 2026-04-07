#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 主函数();

long long 主函数() {
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_n_0;
  cn_var_n_0 = 5;
  r0 = cn_var_n_0;
  r1 = r0 < 10;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
}

