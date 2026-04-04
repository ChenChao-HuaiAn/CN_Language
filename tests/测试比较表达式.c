#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 测试比较();
long long 测试字符比较();
long long main();

long long 测试比较() {
  long long r0, r1;
  _Bool r2;

  entry:
  long long cn_var_a_0;
  cn_var_a_0 = 10;
  long long cn_var_b_1;
  cn_var_b_1 = 20;
  r0 = cn_var_a_0;
  r1 = cn_var_b_1;
  r2 = r0 < r1;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
}

long long 测试字符比较() {
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_c_0;
  cn_var_c_0 = 65;
  r0 = cn_var_c_0;
  r1 = r0 == 65;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return 1;
  goto if_merge_3;

  if_merge_3:
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4;

  entry:
  long long cn_var_结果1_0;
  r0 = 测试比较();
  cn_var_结果1_0 = r0;
  long long cn_var_结果2_1;
  r1 = 测试字符比较();
  cn_var_结果2_1 = r1;
  r2 = cn_var_结果1_0;
  r3 = cn_var_结果2_1;
  r4 = r2 + r3;
  return r4;
  cn_rt_exit();
}

