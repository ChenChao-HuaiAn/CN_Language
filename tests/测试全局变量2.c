#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables
long long cn_var_全局计数 = 0;

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1, r2;

  entry:
  r0 = cn_var_全局计数;
  r1 = r0 + 1;
  cn_var_全局计数 = r1;
  r2 = cn_var_全局计数;
  return r2;
  cn_rt_exit();
}

