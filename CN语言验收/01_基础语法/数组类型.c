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
  cn_var_一维数组 = cn_rt_array_alloc(8, 5);
  r0 = cn_var_一维数组;
    { long long _tmp_i0 = 10; cn_rt_array_set_element(r0, 0, &_tmp_i0, 8); }
  r1 = cn_var_一维数组;
    { long long _tmp_i1 = 20; cn_rt_array_set_element(r1, 1, &_tmp_i1, 8); }
  r2 = cn_var_一维数组;
    { long long _tmp_i2 = 30; cn_rt_array_set_element(r2, 2, &_tmp_i2, 8); }
  r3 = cn_var_一维数组;
    { long long _tmp_i3 = 40; cn_rt_array_set_element(r3, 3, &_tmp_i3, 8); }
  r4 = cn_var_一维数组;
    { long long _tmp_i4 = 50; cn_rt_array_set_element(r4, 4, &_tmp_i4, 8); }
  void* cn_var_二维数组;
  cn_var_二维数组 = cn_rt_array_alloc(8, 3);
  r5 = cn_var_二维数组;
  r6 = *(void**)cn_rt_array_get_element(r5, 0, 8);
    { long long _tmp_i5 = 1; cn_rt_array_set_element(r6, 0, &_tmp_i5, 8); }
  r7 = cn_var_二维数组;
  r8 = *(void**)cn_rt_array_get_element(r7, 1, 8);
    { long long _tmp_i6 = 5; cn_rt_array_set_element(r8, 2, &_tmp_i6, 8); }
  void* cn_var_名称列表;
  cn_var_名称列表 = cn_rt_array_alloc(8, 3);
  r9 = cn_var_名称列表;
    { char* _tmp_s0 = "张三"; cn_rt_array_set_element(r9, 0, &_tmp_s0, 8); }
  r10 = cn_var_名称列表;
    { char* _tmp_s1 = "李四"; cn_rt_array_set_element(r10, 1, &_tmp_s1, 8); }
  r11 = cn_var_名称列表;
    { char* _tmp_s2 = "王五"; cn_rt_array_set_element(r11, 2, &_tmp_s2, 8); }
  void* cn_var_温度记录;
  cn_var_温度记录 = cn_rt_array_alloc(8, 7);
  r12 = cn_var_温度记录;
    { double _tmp_f0 = 25.500000; cn_rt_array_set_element(r12, 0, &_tmp_f0, 8); }
  r13 = cn_var_温度记录;
    { double _tmp_f1 = 26.300000; cn_rt_array_set_element(r13, 1, &_tmp_f1, 8); }
  return 0;
  cn_rt_exit();
}

