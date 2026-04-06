#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 位置;
    long long 列号;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r3, r6, r7, r8, r9;
  void* r0;
  struct 测试结构* r1;
  struct 测试结构* r4;
  struct 测试结构 r2;
  struct 测试结构 r5;

  entry:
  struct 测试结构* cn_var_实例_0;
  r0 = 分配内存(8);
  cn_var_实例_0 = r0;
  long long cn_var_起始列号_1;
  r1 = cn_var_实例_0;
  r2 = *r1;
  r3 = r2.列号;
  cn_var_起始列号_1 = r3;
  long long cn_var_结果_2;
  r4 = cn_var_实例_0;
  r5 = *r4;
  r6 = r5.位置;
  r7 = cn_var_起始列号_1;
  r8 = r6 - r7;
  cn_var_结果_2 = r8;
  r9 = cn_var_结果_2;
  return r9;
}

