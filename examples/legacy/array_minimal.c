#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r4, r6;
  void* r0;
  void* r1;
  void* r2;
  void* r3;
  void* r5;
  void* r7;
  void* r8;
  void* r9;

  entry:
  void* cn_var_arr1;
  r0 = cn_rt_array_alloc(8, 3);
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 1; &_tmp_0;}), 8);
  cn_rt_array_set_element(r0, 1, ({long long _tmp_1 = 2; &_tmp_1;}), 8);
  cn_rt_array_set_element(r0, 2, ({long long _tmp_2 = 3; &_tmp_2;}), 8);
  cn_var_arr1 = r0;
  void* cn_var_arr2;
  r1 = cn_rt_array_alloc(8, 3);
  cn_rt_array_set_element(r1, 0, ({long long _tmp_3 = 10; &_tmp_3;}), 8);
  cn_rt_array_set_element(r1, 1, ({long long _tmp_4 = 20; &_tmp_4;}), 8);
  cn_rt_array_set_element(r1, 2, ({long long _tmp_5 = 30; &_tmp_5;}), 8);
  cn_var_arr2 = r1;
  void* cn_var_arr3;
  r2 = cn_rt_array_alloc(8, 3);
  cn_rt_array_set_element(r2, 0, &"一", 8);
  cn_rt_array_set_element(r2, 1, &"二", 8);
  cn_rt_array_set_element(r2, 2, &"三", 8);
  cn_var_arr3 = r2;
  int cn_var_x;
  r3 = cn_var_arr1;
  r4 = *(int*)cn_rt_array_get_element(r3, 0, 8);
  cn_var_x = r4;
  int cn_var_y;
  r5 = cn_var_arr2;
  r6 = *(int*)cn_rt_array_get_element(r5, 1, 8);
  cn_var_y = r6;
  r7 = cn_var_arr1;
  cn_rt_array_set_element(r7, 0, ({long long _tmp_6 = 100; &_tmp_6;}), 8);
  r8 = cn_var_arr2;
  cn_rt_array_set_element(r8, 1, ({long long _tmp_7 = 200; &_tmp_7;}), 8);
  void* cn_var_arr4;
  r9 = cn_rt_array_alloc(8, 0);
  cn_var_arr4 = r9;
  return 0;
  cn_rt_exit();
}

