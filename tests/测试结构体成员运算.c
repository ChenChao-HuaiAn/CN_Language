#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 位置;
    long long 列号;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r2, r4, r5, r6, r7;
  void* r0;
  struct 测试结构 r1;
  struct 测试结构 r3;

  entry:
  struct 测试结构* cn_var_实例_0;
  r0 = 分配内存(8);
  cn_var_实例_0 = r0;
  long long cn_var_起始列号_1;
  r1 = *cn_var_实例_0;
  r2 = r1.列号;
  cn_var_起始列号_1 = r2;
  long long cn_var_结果_2;
  r3 = *cn_var_实例_0;
  r4 = r3.位置;
  r5 = cn_var_起始列号_1;
  r6 = r4 - r5;
  cn_var_结果_2 = r6;
  r7 = cn_var_结果_2;
  return r7;
}

