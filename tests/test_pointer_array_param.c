#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
};

// Global Variables

// Forward Declarations
void 测试指针数组参数(long long**, long long);
void 测试类型转换();

void 测试指针数组参数(long long** cn_var_数据, long long cn_var_个数) {
  long long r0, r1, r4, r8, r9, r10;
  long long** r3;
  long long* r5;
  long long* r6;
  long long** r7;
  _Bool r2;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_0;

  for_cond_0:
  r0 = cn_var_i_0;
  r1 = cn_var_个数;
  r2 = r0 < r1;
  if (r2) goto for_body_1; else goto for_exit_3;

  for_body_1:
  r3 = cn_var_数据;
  r4 = cn_var_i_0;
  r5 = *(long long**)cn_rt_array_get_element(r3, r4, 8);
  r6 = r5 + 1;
  r7 = cn_var_数据;
  r8 = cn_var_i_0;
    { long long _tmp_r0 = r6; cn_rt_array_set_element(r7, r8, &_tmp_r0, 8); }
  goto for_update_2;

  for_update_2:
  r9 = cn_var_i_0;
  r10 = r9 + 1;
  cn_var_i_0 = r10;
  goto for_cond_0;

  for_exit_3:
  return;
}

void 测试类型转换() {
  long long r1;
  void* r0;

  entry:
  void* cn_var_内存_0;
  r0 = 分配清零内存(10, 4);
  cn_var_内存_0 = r0;
  long long** cn_var_数据_1;
  r1 = cn_var_内存_0;
  cn_var_数据_1 = (long long**)cn_var_内存;
  return;
}

