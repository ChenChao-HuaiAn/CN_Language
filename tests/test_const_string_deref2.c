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
  char* r0;
  char* r3;
  char* r4;
  char* r5;
  _Bool r2;
  char r1;

  entry:
  char* cn_var_源指针_0;
  cn_var_源指针_0 = "测试字符串";
  goto while_cond_0;

  while_cond_0:
  r0 = cn_var_源指针_0;
  r1 = *r0;
  r2 = r1 != 0;
  if (r2) goto while_body_1; else goto while_exit_2;

  while_body_1:
  r3 = cn_var_源指针_0;
  r4 = cn_rt_int_to_string(1);
  r5 = cn_rt_string_concat(r3, r4);
  cn_var_源指针_0 = r5;
  goto while_cond_0;

  while_exit_2:
  return 0;
  cn_rt_exit();
}

