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
  void* r2;
  void* r3;
  void* r4;
  void* r5;
  void* r6;

  entry:
  long long cn_var_x_0;
  cn_var_x_0 = 10;
  long long cn_var_y_1;
  cn_var_y_1 = 20;
  double cn_var_pi_2;
  cn_var_pi_2 = 3.141590;
  char* cn_var_消息_3;
  cn_var_消息_3 = "你好，CN语言！";
  _Bool cn_var_flag_4;
  cn_var_flag_4 = 1;
  long long* cn_var_arr;
  cn_var_arr = cn_rt_array_alloc(8, 5);
  r0 = cn_var_arr;
    { long long _tmp_i0 = 1; cn_rt_array_set_element(r0, 0, &_tmp_i0, 8); }
  r1 = cn_var_arr;
    { long long _tmp_i1 = 2; cn_rt_array_set_element(r1, 1, &_tmp_i1, 8); }
  r2 = cn_var_arr;
    { long long _tmp_i2 = 3; cn_rt_array_set_element(r2, 2, &_tmp_i2, 8); }
  long long** cn_var_matrix;
  cn_var_matrix = cn_rt_array_alloc(8, 2);
  r3 = cn_var_matrix;
  r4 = *(long long**)cn_rt_array_get_element(r3, 0, 8);
    { long long _tmp_i3 = 1; cn_rt_array_set_element(r4, 0, &_tmp_i3, 8); }
  r5 = cn_var_matrix;
  r6 = *(long long**)cn_rt_array_get_element(r5, 0, 8);
    { long long _tmp_i4 = 2; cn_rt_array_set_element(r6, 1, &_tmp_i4, 8); }
  return 0;
  cn_rt_exit();
}

