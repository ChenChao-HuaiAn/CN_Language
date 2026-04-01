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
  long long cn_var_x;
  cn_var_x = 10;
  long long cn_var_y;
  cn_var_y = 20;
  double cn_var_pi;
  cn_var_pi = 3.141590;
  char* cn_var_消息;
  cn_var_消息 = "你好，CN语言！";
  _Bool cn_var_flag;
  cn_var_flag = 1;
  void* cn_var_arr;
  r0 = cn_var_arr;
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 1; &_tmp_0;}), 8);
  r1 = cn_var_arr;
  cn_rt_array_set_element(r1, 1, ({long long _tmp_1 = 2; &_tmp_1;}), 8);
  r2 = cn_var_arr;
  cn_rt_array_set_element(r2, 2, ({long long _tmp_2 = 3; &_tmp_2;}), 8);
  void* cn_var_matrix;
  r3 = cn_var_matrix;
  r4 = *(void**)cn_rt_array_get_element(r3, 0, 8);
  cn_rt_array_set_element(r4, 0, ({long long _tmp_3 = 1; &_tmp_3;}), 8);
  r5 = cn_var_matrix;
  r6 = *(void**)cn_rt_array_get_element(r5, 0, 8);
  cn_rt_array_set_element(r6, 1, ({long long _tmp_4 = 2; &_tmp_4;}), 8);
  return 0;
  cn_rt_exit();
}

