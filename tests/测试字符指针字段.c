#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试;

// CN Language Global Struct Definitions
struct 测试 {
    char* 数据;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r4, r6;
  void* r0;
  struct 测试* r1;
  char* r3;
  char r5;
  struct 测试 r2;

  entry:
  struct 测试* cn_var_t_0;
  r0 = 分配内存(8);
  cn_var_t_0 = r0;
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  long long cn_var_c_2;
  r1 = cn_var_t_0;
  r2 = *r1;
  r3 = r2.数据;
  r4 = cn_var_i_1;
  r5 = *(char*)cn_rt_array_get_element(r3, r4, 8);
  cn_var_c_2 = r5;
  r6 = cn_var_c_2;
  return r6;
}

