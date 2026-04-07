#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 是空指针(long long);
long long 比较值(long long, long long);
long long main();

long long 是空指针(long long cn_var_指针) {
  long long r0;
  _Bool r1;

  entry:
  r0 = cn_var_指针;
  r1 = r0 == 0;
  return r1;
}

long long 比较值(long long cn_var_值1, long long cn_var_值2) {
  long long r0, r1;
  _Bool r2;

  entry:
  r0 = cn_var_值1;
  r1 = cn_var_值2;
  r2 = r0 == r1;
  return r2;
}

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  long long cn_var_变量1_0;
  cn_var_变量1_0 = 10;
  long long cn_var_变量2_1;
  cn_var_变量2_1 = 20;
  long long cn_var_结果1_2;
  r0 = cn_var_变量1_0;
  r1 = 是空指针(r0);
  cn_var_结果1_2 = r1;
  long long cn_var_结果2_3;
  r2 = cn_var_变量1_0;
  r3 = cn_var_变量2_1;
  r4 = 比较值(r2, r3);
  cn_var_结果2_3 = r4;
  r5 = cn_var_结果1_2;
  r6 = cn_var_结果2_3;
  r7 = r5 + r6;
  return r7;
  cn_rt_exit();
}

