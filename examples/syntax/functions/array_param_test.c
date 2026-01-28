#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long cn_func_求和(long long*, long long);
long long cn_func_打印前三(void*);
long long main();

long long cn_func_求和(long long* cn_var_arr, long long cn_var_长度) {
  long long r0, r1, r3, r5, r6, r7, r8, r9, r10, r11;
  long long* r4;
  _Bool r2;

  entry:
  long long cn_var_总和;
  cn_var_总和 = 0;
  long long cn_var_i;
  cn_var_i = 0;
  goto for_cond_0;

  for_cond_0:
  r0 = cn_var_i;
  r1 = cn_var_长度;
  r2 = r0 < r1;
  if (r2) goto for_body_1; else goto for_exit_3;

  for_body_1:
  r3 = cn_var_总和;
  r4 = cn_var_arr;
  r5 = cn_var_i;
  r6 = *(long long*)cn_rt_array_get_element(r4, r5, 8);
  r7 = r3 + r6;
  cn_var_总和 = r7;
  goto for_update_2;

  for_update_2:
  r8 = cn_var_i;
  r10 = r8;
  r9 = r8 + 1;
  r8 = r9;
  goto for_cond_0;

  for_exit_3:
  r11 = cn_var_总和;
  return r11;
}

long long cn_func_打印前三(void* cn_var_arr) {
  long long r1, r3, r5;
  void* r0;
  void* r2;
  void* r4;

  entry:
  r0 = cn_var_arr;
  r1 = *(long long*)cn_rt_array_get_element(r0, 0, 8);
  cn_rt_print_int(r1);
  r2 = cn_var_arr;
  r3 = *(long long*)cn_rt_array_get_element(r2, 1, 8);
  cn_rt_print_int(r3);
  r4 = cn_var_arr;
  r5 = *(long long*)cn_rt_array_get_element(r4, 2, 8);
  cn_rt_print_int(r5);
}

long long main() {
  cn_rt_init();
  long long r2, r3;
  void* r0;
  void* r1;
  void* r4;
  void* r5;

  entry:
  void* cn_var_数字;
  r0 = cn_rt_array_alloc(8, 5);
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 1; &_tmp_0;}), 8);
  cn_rt_array_set_element(r0, 1, ({long long _tmp_1 = 2; &_tmp_1;}), 8);
  cn_rt_array_set_element(r0, 2, ({long long _tmp_2 = 3; &_tmp_2;}), 8);
  cn_rt_array_set_element(r0, 3, ({long long _tmp_3 = 4; &_tmp_3;}), 8);
  cn_rt_array_set_element(r0, 4, ({long long _tmp_4 = 5; &_tmp_4;}), 8);
  cn_var_数字 = r0;
  long long cn_var_结果;
  r1 = cn_var_数字;
  r2 = cn_func_求和(r1, 5);
  cn_var_结果 = r2;
  r3 = cn_var_结果;
  cn_rt_print_int(r3);
  void* cn_var_测试数组;
  r4 = cn_rt_array_alloc(8, 10);
  cn_rt_array_set_element(r4, 0, ({long long _tmp_5 = 10; &_tmp_5;}), 8);
  cn_rt_array_set_element(r4, 1, ({long long _tmp_6 = 20; &_tmp_6;}), 8);
  cn_rt_array_set_element(r4, 2, ({long long _tmp_7 = 30; &_tmp_7;}), 8);
  cn_rt_array_set_element(r4, 3, ({long long _tmp_8 = 40; &_tmp_8;}), 8);
  cn_rt_array_set_element(r4, 4, ({long long _tmp_9 = 50; &_tmp_9;}), 8);
  cn_rt_array_set_element(r4, 5, ({long long _tmp_10 = 60; &_tmp_10;}), 8);
  cn_rt_array_set_element(r4, 6, ({long long _tmp_11 = 70; &_tmp_11;}), 8);
  cn_rt_array_set_element(r4, 7, ({long long _tmp_12 = 80; &_tmp_12;}), 8);
  cn_rt_array_set_element(r4, 8, ({long long _tmp_13 = 90; &_tmp_13;}), 8);
  cn_rt_array_set_element(r4, 9, ({long long _tmp_14 = 100; &_tmp_14;}), 8);
  cn_var_测试数组 = r4;
  r5 = cn_var_测试数组;
  cn_func_打印前三(r5);
  cn_rt_exit();
}

