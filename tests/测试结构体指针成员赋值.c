#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 扫描器;

// CN Language Global Struct Definitions
struct 扫描器 {
    long long 位置;
    long long 行号;
    long long 列号;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r1, r5, r6, r9, r10, r13;
  void* r0;
  struct 扫描器* r3;
  struct 扫描器* r7;
  struct 扫描器* r11;
  _Bool r2;
  struct 扫描器 r4;
  struct 扫描器 r8;
  struct 扫描器 r12;

  entry:
  struct 扫描器* cn_var_实例_0;
  r0 = 分配内存(12);
  cn_var_实例_0 = r0;
  long long cn_var_c_1;
  cn_var_c_1 = 10;
  r1 = cn_var_c_1;
  r2 = r1 == 10;
  if (r2) goto if_then_0; else goto if_else_1;

  if_then_0:
  r3 = cn_var_实例_0;
  r4 = *r3;
  r5 = r4.行号;
  r6 = r5 + 1;
  goto if_merge_2;

  if_else_1:
  r7 = cn_var_实例_0;
  r8 = *r7;
  r9 = r8.列号;
  r10 = r9 + 1;
  goto if_merge_2;

  if_merge_2:
  r11 = cn_var_实例_0;
  r12 = *r11;
  r13 = r12.列号;
  return r13;
}

