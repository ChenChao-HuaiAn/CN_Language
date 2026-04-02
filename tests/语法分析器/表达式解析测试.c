#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 获取固定值();
long long main();

long long 获取固定值() {

  entry:
  return 42;
}

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4;

  entry:
  long long cn_var_a_0;
  cn_var_a_0 = 10;
  long long cn_var_b_1;
  cn_var_b_1 = 5;
  long long cn_var_c_2;
  r0 = cn_var_a_0;
  r1 = cn_var_b_1;
  r2 = r0 + r1;
  cn_var_c_2 = r2;
  long long cn_var_d_3;
  goto entry_inline_获取固定值;
  return 0;
  cn_rt_exit();
}

