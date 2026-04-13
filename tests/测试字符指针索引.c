#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r1, r4;
  char* r0;
  char* r2;
  char r3;

  entry:
  char* cn_var_源码_0;
  cn_var_源码_0 = "Hello";
  long long cn_var_位置_1;
  cn_var_位置_1 = 0;
  long long cn_var_c_2;
  r0 = cn_var_源码_0;
  r1 = cn_var_位置_1;
  r2 = &r0[r1];
  r3 = *r2;
  cn_var_c_2 = r3;
  r4 = cn_var_c_2;
  return r4;
}

