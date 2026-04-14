#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
    long long 数量;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r2, r4, r5, r6;
  void* r0;
  struct 测试结构 r1;
  struct 测试结构 r3;

  entry:
  struct 测试结构* cn_var_实例_0;
  r0 = 分配内存(8);
  cn_var_实例_0 = r0;
  long long cn_var_结果_1;
  r1 = *cn_var_实例_0;
  r2 = r1.值;
  r3 = *cn_var_实例_0;
  r4 = r3.数量;
  r5 = r2 + r4;
  cn_var_结果_1 = r5;
  r6 = cn_var_结果_1;
  return r6;
}

