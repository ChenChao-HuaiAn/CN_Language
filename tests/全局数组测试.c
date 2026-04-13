#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables
long long* cn_var_全局数组 = 0;

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long* r0;
  long long* r1;

  entry:
  r0 = cn_var_全局数组;
  r1 = &r0[0];
  r1 = 1;
  return 0;
  cn_rt_exit();
}

