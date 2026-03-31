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
  long long r0, r1, r2, r3, r4, r5, r6;

  entry:
  long long cn_var_a;
  cn_var_a = 10;
  long long cn_var_b;
  cn_var_b = 20;
  int cn_var_结果;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 > r1;
  r3 = cn_var_a;
  r4 = cn_var_b;
  r5 = (r2 ? r3 : r4);
  cn_var_结果 = r5;
  r6 = cn_var_结果;
  return r6;
  cn_rt_exit();
}

