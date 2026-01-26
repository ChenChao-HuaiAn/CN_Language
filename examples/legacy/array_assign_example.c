#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r5, r7, r9;
  void* r0;
  void* r1;
  void* r2;
  void* r3;
  void* r4;
  void* r6;
  void* r8;

  entry:
  void* cn_var_arr;
  r0 = cn_rt_array_alloc(8, 3);
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 10; &_tmp_0;}), 8);
  cn_rt_array_set_element(r0, 1, ({long long _tmp_1 = 20; &_tmp_1;}), 8);
  cn_rt_array_set_element(r0, 2, ({long long _tmp_2 = 30; &_tmp_2;}), 8);
  cn_var_arr = r0;
  r1 = cn_var_arr;
  cn_rt_array_set_element(r1, 0, ({long long _tmp_3 = 100; &_tmp_3;}), 8);
  r2 = cn_var_arr;
  cn_rt_array_set_element(r2, 1, ({long long _tmp_4 = 200; &_tmp_4;}), 8);
  r3 = cn_var_arr;
  cn_rt_array_set_element(r3, 2, ({long long _tmp_5 = 300; &_tmp_5;}), 8);
  int cn_var_a;
  r4 = cn_var_arr;
  r5 = *(int*)cn_rt_array_get_element(r4, 0, 8);
  cn_var_a = r5;
  int cn_var_b;
  r6 = cn_var_arr;
  r7 = *(int*)cn_rt_array_get_element(r6, 1, 8);
  cn_var_b = r7;
  int cn_var_c;
  r8 = cn_var_arr;
  r9 = *(int*)cn_rt_array_get_element(r8, 2, 8);
  cn_var_c = r9;
  return 0;
  cn_rt_exit();
}

