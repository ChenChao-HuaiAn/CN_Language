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
  long long r2, r8;
  void* r0;
  char* r4;
  char* r5;
  char* r6;
  char r7;
  struct 测试 r1;
  struct 测试 r3;

  entry:
  struct 测试* cn_var_t_0;
  r0 = 分配内存(16);
  cn_var_t_0 = r0;
  long long cn_var_v_1;
  r1 = *cn_var_t_0;
  r2 = r1.值;
  cn_var_v_1 = r2;
  char* cn_var_p_2;
  r3 = *cn_var_t_0;
  r4 = r3.数据;
  cn_var_p_2 = r4;
  long long cn_var_c_3;
  r5 = cn_var_p_2;
  r6 = &r5[0];
  r7 = *r6;
  cn_var_c_3 = r7;
  r8 = cn_var_c_3;
  return r8;
}

