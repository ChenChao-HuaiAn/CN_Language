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
  void* r0;
  void* r1;

  entry:
  long long* cn_var_一维数组;
  cn_var_一维数组 = cn_rt_array_alloc(8, 5);
  r0 = cn_var_一维数组;
    { long long _tmp_i0 = 10; cn_rt_array_set_element(r0, 0, &_tmp_i0, 8); }
  r1 = cn_var_一维数组;
    { long long _tmp_i1 = 20; cn_rt_array_set_element(r1, 1, &_tmp_i1, 8); }
  return 0;
  cn_rt_exit();
}

