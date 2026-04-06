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
  void* r2;
  void* r3;
  void* r4;
  void* r5;
  _Bool r1;
  char r0;

  entry:
  char cn_var_ch_0;
  cn_var_ch_0 = 65;
  r0 = cn_var_ch_0;
  r1 = r0 == 65;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("字符比较成功");
  goto if_merge_1;

  if_merge_1:
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 64);
  r2 = cn_var_缓冲区;
    { long long _tmp_i0 = 72; cn_rt_array_set_element(r2, 0, &_tmp_i0, 8); }
  r3 = cn_var_缓冲区;
    { long long _tmp_i1 = 105; cn_rt_array_set_element(r3, 1, &_tmp_i1, 8); }
  r4 = cn_var_缓冲区;
    { long long _tmp_i2 = 0; cn_rt_array_set_element(r4, 2, &_tmp_i2, 8); }
  r5 = cn_var_缓冲区;
  cn_rt_print_string(r5);
  return 0;
  cn_rt_exit();
}

