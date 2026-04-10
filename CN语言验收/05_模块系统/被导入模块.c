#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables
void cn_var_模块版本 = "1.0.0";
long long cn_var_最大值 = 100;
long long cn_var_内部计数 = 0;

// Forward Declarations
long long 加法(long long, long long);
long long 减法(long long, long long);
long long 乘法(long long, long long);
long long 除法(long long, long long);
long long 平方(long long);
long long 绝对值(long long);
long long 内部辅助();

long long 加法(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  return r2;
}

long long 减法(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 - r1;
  return r2;
}

long long 乘法(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 * r1;
  return r2;
}

long long 除法(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r2, r3, r4;

  entry:
  r0 = cn_var_b;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_a;
  r3 = cn_var_b;
  r4 = r2 / r3;
  return r4;
}

long long 平方(long long cn_var_x) {
  long long r0, r1, r2;

  entry:
  r0 = cn_var_x;
  r1 = cn_var_x;
  r2 = r0 * r1;
  return r2;
}

long long 绝对值(long long cn_var_x) {
  long long r0, r1, r2, r3, r4;

  entry:
  r0 = cn_var_x;
  r1 = r0 < 0;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  r2 = cn_var_x;
  r3 = -r2;
  return r3;
  goto if_merge_3;

  if_merge_3:
  r4 = cn_var_x;
  return r4;
}

long long 内部辅助() {
  long long r0, r1;

  entry:
  r0 = cn_var_内部计数;
  r1 = r0 + 1;
  cn_var_内部计数 = r1;
  return 0;
}

