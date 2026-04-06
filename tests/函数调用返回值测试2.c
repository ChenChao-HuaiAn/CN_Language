#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 获取值();
_Bool 是偶数(long long);
long long 打印消息(long long);
long long main();

long long 获取值() {

  entry:
  return 42;
}

_Bool 是偶数(long long cn_var_n) {
  long long r0, r1;
  _Bool r2;

  entry:
  r0 = cn_var_n;
  r1 = r0 % 2;
  r2 = r1 == 0;
  return r2;
}

long long 打印消息(long long cn_var_消息) {
  long long r0;

  entry:
  r0 = cn_var_消息;
  cn_rt_print_int(r0);
  return 0;
}

long long main() {
  cn_rt_init();
  long long r1, r2, r4, r5, r6, r7, r8, r9, r10;
  _Bool r0;
  _Bool r3;
  _Bool r11;

  entry:
  r0 = 是偶数(10);
  if (r0) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_int(1);
  goto if_merge_1;

  if_merge_1:
  long long cn_var_计数_0;
  cn_var_计数_0 = 0;
  goto while_cond_2;

  while_cond_2:
  r1 = cn_var_计数_0;
  r2 = 获取值();
  r3 = r1 < r2;
  if (r3) goto while_body_3; else goto while_exit_4;

  while_body_3:
  r4 = cn_var_计数_0;
  r5 = r4 + 10;
  cn_var_计数_0 = r5;
  goto while_cond_2;

  while_exit_4:
  r6 = cn_var_计数_0;
  cn_rt_print_int(r6);
  long long cn_var_结果_1;
  r7 = 获取值();
  r8 = r7 + 8;
  cn_var_结果_1 = r8;
  r9 = cn_var_结果_1;
  cn_rt_print_int(r9);
  r10 = 获取值();
  r11 = 是偶数(r10);
  if (r11) goto if_then_5; else goto if_merge_6;

  if_then_5:
  cn_rt_print_int(1);
  goto if_merge_6;

  if_merge_6:
  return 0;
  cn_rt_exit();
}

