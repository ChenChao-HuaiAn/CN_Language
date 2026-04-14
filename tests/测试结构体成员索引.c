#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 扫描器;

// CN Language Global Struct Definitions
struct 扫描器 {
    char* 源码;
    long long 位置;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r4, r7;
  void* r0;
  char* r2;
  char* r5;
  char r6;
  struct 扫描器 r1;
  struct 扫描器 r3;

  entry:
  struct 扫描器* cn_var_实例_0;
  r0 = 分配内存(16);
  cn_var_实例_0 = r0;
  long long cn_var_c_1;
  r1 = *cn_var_实例_0;
  r2 = r1.源码;
  r3 = *cn_var_实例_0;
  r4 = r3.位置;
  r5 = &r2[r4];
  r6 = *r5;
  cn_var_c_1 = r6;
  r7 = cn_var_c_1;
  return r7;
}

