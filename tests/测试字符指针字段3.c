#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试 {
    char* 数据;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r5;
  void* r0;
  struct 测试* r1;
  char* r3;
  char r4;
  struct 测试 r2;

  entry:
  struct 测试* cn_var_t_0;
  r0 = 分配内存(8);
  cn_var_t_0 = r0;
  long long cn_var_c_1;
  r1 = cn_var_t_0;
  r2 = *r1;
  r3 = r2.数据;
  r4 = *(char*)cn_rt_array_get_element(r3, 0, 8);
  cn_var_c_1 = r4;
  r5 = cn_var_c_1;
  return r5;
}

