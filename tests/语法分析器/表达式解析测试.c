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
  long long cn_var_a;
  cn_var_a = 10;
  long long cn_var_b;
  cn_var_b = 5;
  long long cn_var_c;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  cn_var_c = r2;
  long long cn_var_d;
  goto entry_inline_获取固定值;
  cn_rt_exit();
}

