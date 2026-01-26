#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r0;
  _Bool r1;
  _Bool r2;

  entry:
  int cn_var_x;
  cn_var_x = 2;
  r0 = cn_var_x;
  r1 = r0 == 1;
  if (r1) goto case_body_1; else goto switch_check_4;

  switch_check_4:
  r2 = r0 == 2;
  if (r2) goto case_body_2; else goto case_default_3;

  case_body_1:
  cn_rt_print_string("一");
  goto switch_merge_0;

  case_body_2:
  cn_rt_print_string("二");
  goto switch_merge_0;

  case_default_3:
  cn_rt_print_string("其他");
  goto switch_merge_0;

  switch_merge_0:
  return 0;
  cn_rt_exit();
}

