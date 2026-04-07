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
long long cn_var_数组大小 = 3;
struct 测试结构* cn_var_测试数组 = 0;

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1, r3, r4, r5;
  _Bool r2;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_0;

  for_cond_0:
  r0 = cn_var_i_0;
  r1 = cn_var_数组大小;
  r2 = r0 < r1;
  if (r2) goto for_body_1; else goto for_exit_3;

  for_body_1:
  r3 = cn_var_i_0;
  goto for_update_2;

  for_update_2:
  r4 = cn_var_i_0;
  r5 = r4 + 1;
  cn_var_i_0 = r5;
  goto for_cond_0;

  for_exit_3:
  return 0;
  cn_rt_exit();
}

