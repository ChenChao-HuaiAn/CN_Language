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
  void* r7;
  void* r8;
  void* r9;
  void* r10;
  void* r11;
  void* r12;
  void* r13;

  entry:
  void* cn_var_一维数组;
  r0 = cn_var_一维数组;
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 10; &_tmp_0;}), 8);
  r1 = cn_var_一维数组;
  cn_rt_array_set_element(r1, 1, ({long long _tmp_1 = 20; &_tmp_1;}), 8);
  r2 = cn_var_一维数组;
  cn_rt_array_set_element(r2, 2, ({long long _tmp_2 = 30; &_tmp_2;}), 8);
  r3 = cn_var_一维数组;
  cn_rt_array_set_element(r3, 3, ({long long _tmp_3 = 40; &_tmp_3;}), 8);
  r4 = cn_var_一维数组;
  cn_rt_array_set_element(r4, 4, ({long long _tmp_4 = 50; &_tmp_4;}), 8);
  void* cn_var_二维数组;
  r5 = cn_var_二维数组;
  r6 = *(void**)cn_rt_array_get_element(r5, 0, 8);
  cn_rt_array_set_element(r6, 0, ({long long _tmp_5 = 1; &_tmp_5;}), 8);
  r7 = cn_var_二维数组;
  r8 = *(void**)cn_rt_array_get_element(r7, 1, 8);
  cn_rt_array_set_element(r8, 2, ({long long _tmp_6 = 5; &_tmp_6;}), 8);
  void* cn_var_名称列表;
  r9 = cn_var_名称列表;
  cn_rt_array_set_element(r9, 0, &"张三", 8);
  r10 = cn_var_名称列表;
  cn_rt_array_set_element(r10, 1, &"李四", 8);
  r11 = cn_var_名称列表;
  cn_rt_array_set_element(r11, 2, &"王五", 8);
  void* cn_var_温度记录;
  r12 = cn_var_温度记录;
  cn_rt_array_set_element(r12, 0, &25.500000, 8);
  r13 = cn_var_温度记录;
  cn_rt_array_set_element(r13, 1, &26.300000, 8);
  return 0;
  cn_rt_exit();
}

