#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 测试();
long long main();

long long 测试() {
  long long* r0;
  _Bool r1;

  entry:
  long long* cn_var_p_0;
  cn_var_p_0 = 0;
  r0 = cn_var_p_0;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  long long cn_var_result_0;
  r0 = 测试();
  cn_var_result_0 = r0;
  r1 = cn_var_result_0;
  return r1;
  cn_rt_exit();
}

