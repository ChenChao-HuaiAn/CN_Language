#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试;

// CN Language Global Struct Definitions
struct 测试 {
    long long 值;
    char* 数据;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r3, r9;
  void* r0;
  struct 测试* r1;
  struct 测试* r4;
  char* r6;
  char* r7;
  char* r8;
  struct 测试 r2;
  struct 测试 r5;

  entry:
  struct 测试* cn_var_t_0;
  r0 = 分配内存(16);
  cn_var_t_0 = r0;
  long long cn_var_v_1;
  r1 = cn_var_t_0;
  r2 = *r1;
  r3 = r2.值;
  cn_var_v_1 = r3;
  char* cn_var_p_2;
  r4 = cn_var_t_0;
  r5 = *r4;
  r6 = r5.数据;
  cn_var_p_2 = r6;
  long long cn_var_c_3;
  r7 = cn_var_p_2;
  r8 = *(char**)cn_rt_array_get_element(r7, 0, 8);
  cn_var_c_3 = r8;
  r9 = cn_var_c_3;
  return r9;
}

