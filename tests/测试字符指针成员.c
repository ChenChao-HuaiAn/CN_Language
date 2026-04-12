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
    long long 行号;
    long long 列号;
    long long 源码长度;
};

// Global Variables

// Forward Declarations
long long 当前字符(struct 扫描器*);
long long 主();

long long 当前字符(struct 扫描器* cn_var_实例) {
  long long r0, r5, r8, r15;
  struct 扫描器* r1;
  struct 扫描器* r3;
  struct 扫描器* r6;
  struct 扫描器* r10;
  char* r12;
  struct 扫描器* r13;
  char* r16;
  _Bool r2;
  _Bool r9;
  struct 扫描器 r4;
  struct 扫描器 r7;
  struct 扫描器 r11;
  struct 扫描器 r14;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3; else goto logic_rhs_2;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  r10 = cn_var_实例;
  r11 = *r10;
  r12 = r11.源码;
  r13 = cn_var_实例;
  r14 = *r13;
  r15 = r14.位置;
  r16 = (void**)cn_rt_array_get_element(r12, r15, 8);
  return r16;

  logic_rhs_2:
  r3 = cn_var_实例;
  r4 = *r3;
  r5 = r4.位置;
  r6 = cn_var_实例;
  r7 = *r6;
  r8 = r7.源码长度;
  r9 = r5 >= r8;
  goto logic_merge_3;

  logic_merge_3:
  if (r9) goto if_then_0; else goto if_merge_1;
  return 0;
}

long long 主() {
  long long r2, r3, r7, r8, r11, r12, r15;
  void* r0;
  struct 扫描器* r1;
  struct 扫描器* r5;
  struct 扫描器* r9;
  struct 扫描器* r13;
  _Bool r4;
  struct 扫描器 r6;
  struct 扫描器 r10;
  struct 扫描器 r14;

  entry:
  struct 扫描器* cn_var_实例_0;
  r0 = 分配内存(24);
  cn_var_实例_0 = r0;
  long long cn_var_c_1;
  r1 = cn_var_实例_0;
  r2 = 当前字符(r1);
  cn_var_c_1 = r2;
  r3 = cn_var_c_1;
  r4 = r3 == 10;
  if (r4) goto if_then_4; else goto if_else_5;

  if_then_4:
  r5 = cn_var_实例_0;
  r6 = *r5;
  r7 = r6.行号;
  r8 = r7 + 1;
  goto if_merge_6;

  if_else_5:
  r9 = cn_var_实例_0;
  r10 = *r9;
  r11 = r10.列号;
  r12 = r11 + 1;
  goto if_merge_6;

  if_merge_6:
  r13 = cn_var_实例_0;
  r14 = *r13;
  r15 = r14.列号;
  return r15;
}

