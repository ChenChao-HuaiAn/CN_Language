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
  long long* r0;
  long long* r2;
  long long* r1;
  long long* r3;

  entry:
  long long* cn_var_一维数组;
  cn_var_一维数组 = cn_rt_array_alloc(8, 5);
  r0 = cn_var_一维数组;
  r1 = &r0[0];
  r1 = 10;
  r2 = cn_var_一维数组;
  r3 = &r2[1];
  r3 = 20;
  return 0;
  cn_rt_exit();
}

