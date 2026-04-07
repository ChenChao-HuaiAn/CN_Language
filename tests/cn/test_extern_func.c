#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 外部函数();
long long main();

long long 外部函数();
long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  long long cn_var_结果_0;
  r0 = 外部函数();
  cn_var_结果_0 = r0;
  r1 = cn_var_结果_0;
  return r1;
  cn_rt_exit();
}

