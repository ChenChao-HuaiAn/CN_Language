#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
    long long 数量;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r3, r6, r7, r8;
  void* r0;
  struct 测试结构* r1;
  struct 测试结构* r4;
  struct 测试结构 r2;
  struct 测试结构 r5;

  entry:
  struct 测试结构* cn_var_实例_0;
  r0 = 分配内存(8);
  cn_var_实例_0 = r0;
  long long cn_var_结果_1;
  r1 = cn_var_实例_0;
  r2 = *r1;
  r3 = r2.值;
  r4 = cn_var_实例_0;
  r5 = *r4;
  r6 = r5.数量;
  r7 = r3 + r6;
  cn_var_结果_1 = r7;
  r8 = cn_var_结果_1;
  return r8;
}

