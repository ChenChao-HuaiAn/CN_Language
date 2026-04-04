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
  long long r0, r2, r4, r5, r6;
  void* r3;
  _Bool r1;

  entry:
  long long* cn_var_arr;
  cn_var_arr = cn_rt_array_alloc(8, 10);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_0;

  for_cond_0:
  r0 = cn_var_i_0;
  r1 = r0 < 10;
  if (r1) goto for_body_1; else goto for_exit_3;

  for_body_1:
  r2 = cn_var_i_0;
  r3 = cn_var_arr;
  r4 = cn_var_i_0;
    { long long _tmp_r0 = r2; cn_rt_array_set_element(r3, r4, &_tmp_r0, 8); }
  goto for_update_2;

  for_update_2:
  r5 = cn_var_i_0;
  r6 = r5 + 1;
  cn_var_i_0 = r6;
  goto for_cond_0;

  for_exit_3:
  return 0;
  cn_rt_exit();
}

