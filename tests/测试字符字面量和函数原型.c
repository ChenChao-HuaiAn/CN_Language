#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 计算阶乘(long long);
long long 测试字符();
long long main();
long long 计算阶乘(long long);

long long 计算阶乘(long long cn_var_n);
long long 测试字符() {
  long long r0;

  entry:
  long long cn_var_ch_0;
  cn_var_ch_0 = 65;
  long long cn_var_换行_1;
  cn_var_换行_1 = 10;
  long long cn_var_制表_2;
  cn_var_制表_2 = 9;
  long long cn_var_反斜杠_3;
  cn_var_反斜杠_3 = 92;
  long long cn_var_单引号_4;
  cn_var_单引号_4 = 39;
  long long cn_var_空字符_5;
  cn_var_空字符_5 = 0;
  r0 = cn_var_ch_0;
  return r0;
}

long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  long long cn_var_结果_0;
  r0 = 测试字符();
  cn_var_结果_0 = r0;
  long long cn_var_阶乘结果_1;
  r1 = 计算阶乘(5);
  cn_var_阶乘结果_1 = r1;
  return 0;
  cn_rt_exit();
}

long long 计算阶乘(long long cn_var_n) {
  long long r0, r2, r3, r4, r5, r6;
  _Bool r1;

  entry:
  r0 = cn_var_n;
  r1 = r0 <= 1;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_n;
  r3 = cn_var_n;
  r4 = r3 - 1;
  r5 = 计算阶乘(r4);
  r6 = r2 * r5;
  return r6;
}

