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
  long long r6;
  void* r0;
  char* r2;
  char* r3;
  char* r4;
  char r5;
  struct 测试 r1;

  entry:
  struct 测试* cn_var_t_0;
  r0 = 分配内存(8);
  cn_var_t_0 = r0;
  char* cn_var_p_1;
  r1 = *cn_var_t_0;
  r2 = r1.数据;
  cn_var_p_1 = r2;
  long long cn_var_c_2;
  r3 = cn_var_p_1;
  r4 = &r3[0];
  r5 = *r4;
  cn_var_c_2 = r5;
  r6 = cn_var_c_2;
  return r6;
}

