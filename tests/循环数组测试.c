#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r2, r3, r4;
  _Bool r1;

  entry:
  struct 测试结构* cn_var_arr;
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
  goto for_update_2;

  for_update_2:
  r3 = cn_var_i_0;
  r4 = r3 + 1;
  cn_var_i_0 = r4;
  goto for_cond_0;

  for_exit_3:
  return 0;
  cn_rt_exit();
}

