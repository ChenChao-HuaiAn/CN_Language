#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 加法(long long, long long);
long long 减法(long long, long long);
long long 乘法(long long, long long);
long long 除法(long long, long long);
long long 绝对值(long long);
long long 最大值(long long, long long);
long long 最小值(long long, long long);
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
  long long r0, r2, r3, r4;
  _Bool r1;

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

long long 绝对值(long long cn_var_n) {
  long long r0, r2, r3, r4;
  _Bool r1;

  entry:
  r0 = cn_var_n;
  r1 = r0 < 0;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  r2 = cn_var_n;
  r3 = 0 - r2;
  return r3;
  goto if_merge_3;

  if_merge_3:
  r4 = cn_var_n;
  return r4;
}

long long 最大值(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r3, r4;
  _Bool r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 > r1;
  if (r2) goto if_then_4; else goto if_merge_5;

  if_then_4:
  r3 = cn_var_a;
  return r3;
  goto if_merge_5;

  if_merge_5:
  r4 = cn_var_b;
  return r4;
}

long long 最小值(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r3, r4;
  _Bool r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 < r1;
  if (r2) goto if_then_6; else goto if_merge_7;

  if_then_6:
  r3 = cn_var_a;
  return r3;
  goto if_merge_7;

  if_merge_7:
  r4 = cn_var_b;
  return r4;
}

long long 内部辅助() {

  entry:
  return 42;
}

