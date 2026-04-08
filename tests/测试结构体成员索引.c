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
  long long r6, r8;
  void* r0;
  struct 扫描器* r1;
  char* r3;
  struct 扫描器* r4;
  char* r7;
  struct 扫描器 r2;
  struct 扫描器 r5;

  entry:
  struct 扫描器* cn_var_实例_0;
  r0 = 分配内存(16);
  cn_var_实例_0 = r0;
  long long cn_var_c_1;
  r1 = cn_var_实例_0;
  r2 = *r1;
  r3 = r2.源码;
  r4 = cn_var_实例_0;
  r5 = *r4;
  r6 = r5.位置;
  r7 = *(char**)cn_rt_array_get_element(r3, r6, 8);
  cn_var_c_1 = r7;
  r8 = cn_var_c_1;
  return r8;
}

