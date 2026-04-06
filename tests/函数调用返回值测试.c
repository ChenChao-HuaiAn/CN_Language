#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
_Bool 是正数(long long);
long long main();

_Bool 是正数(long long cn_var_值) {
  long long r0;
  _Bool r1;

  entry:
  r0 = cn_var_值;
  r1 = r0 > 0;
  return r1;
}

long long main() {
  cn_rt_init();
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_数值_0;
  cn_var_数值_0 = 10;
  r0 = cn_var_数值_0;
  r1 = 是正数(r0);
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  cn_rt_print_int(1);
  goto if_merge_2;

  if_else_1:
  cn_rt_print_int(0);
  goto if_merge_2;

  if_merge_2:
  return 0;
  cn_rt_exit();
}

