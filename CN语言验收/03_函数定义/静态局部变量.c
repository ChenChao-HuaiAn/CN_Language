#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 计数器();
long long 累加器(long long);
long long 统计函数();
_Bool 首次调用检测();
long long main();

long long 计数器() {
    /* 静态局部变量 */
    static long long cn_static_计数器_计数 = 0;

  long long r0, r1, r2;

  entry:
  r0 = cn_static_计数器_计数;
  r1 = r0 + 1;
  cn_static_计数器_计数 = r1;
  r2 = cn_static_计数器_计数;
  return r2;
}

long long 累加器(long long cn_var_值) {
    /* 静态局部变量 */
    static long long cn_static_累加器_总和 = 0;

  long long r0, r1, r2, r3;

  entry:
  r0 = cn_static_累加器_总和;
  r1 = cn_var_值;
  r2 = r0 + r1;
  cn_static_累加器_总和 = r2;
  r3 = cn_static_累加器_总和;
  return r3;
}

long long 统计函数() {
    /* 静态局部变量 */
    static long long cn_static_统计函数_调用次数 = 0;
    static double cn_static_统计函数_累计值 = 0.000000;

  long long r0, r1;
  double r2;
  double r3;

  entry:
  r0 = cn_static_统计函数_调用次数;
  r1 = r0 + 1;
  cn_static_统计函数_调用次数 = r1;
  r2 = cn_static_统计函数_累计值;
  r3 = r2 + 1.500000;
  cn_static_统计函数_累计值 = r3;
}

_Bool 首次调用检测() {
    /* 静态局部变量 */
    static _Bool cn_static_首次调用检测_已初始化 = 0;

  _Bool r0;
  _Bool r1;

  entry:
  r0 = cn_static_首次调用检测_已初始化;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_static_首次调用检测_已初始化 = 1;
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23;

  entry:
  long long cn_var_第一次;
  goto entry_inline_计数器;
  cn_rt_exit();
}

