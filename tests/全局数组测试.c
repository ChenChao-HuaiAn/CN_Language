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
  void* r0;

  entry:
  r0 = cn_var_全局数组;
    { long long _tmp_i0 = 1; cn_rt_array_set_element(r0, 0, &_tmp_i0, 8); }
  return 0;
  cn_rt_exit();
}

