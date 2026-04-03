#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r3, r9;
  void* r6;
  long long* r1;
  long long* r2;
  long long* r4;
  long long** r5;
  long long* r7;
  long long* r8;
  long long* r10;

  entry:
  long long cn_var_值_0;
  cn_var_值_0 = 42;
  long long* cn_var_指针_1;
  r0 = cn_var_值_0;
  r1 = &cn_var_值;
  cn_var_指针_1 = r1;
  long long cn_var_取值_2;
  r2 = cn_var_指针_1;
  r3 = *r2;
  cn_var_取值_2 = r3;
  long long** cn_var_二级指针_3;
  r4 = cn_var_指针_1;
  r5 = &cn_var_指针;
  cn_var_二级指针_3 = r5;
  long long* cn_var_空指针_4;
  cn_var_空指针_4 = 0;
  void* cn_var_数组;
  cn_var_数组 = cn_rt_array_alloc(8, 5);
  long long* cn_var_数组指针_5;
  r6 = cn_var_数组;
  cn_var_数组指针_5 = r6;
  r7 = cn_var_数组指针_5;
  r8 = r7 + 1;
  cn_var_数组指针_5 = r8;
  void* cn_var_通用指针_6;
  r9 = cn_var_值_0;
  r10 = &cn_var_值;
  cn_var_通用指针_6 = r10;
  return 0;
  cn_rt_exit();
}

