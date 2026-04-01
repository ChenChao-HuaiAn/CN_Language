#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 获取整数();
double 获取圆周率();
char* 获取问候语();
_Bool 判断正数(long long);
long long* 创建整数指针(long long);
long long 执行操作();
long long 绝对值(long long);
long long main();

long long 获取整数() {

  entry:
  return 42;
}

double 获取圆周率() {

  entry:
  return 3.141590;
}

char* 获取问候语() {

  entry:
  return "你好，世界！";
}

_Bool 判断正数(long long cn_var_值) {
  long long r0;
  _Bool r1;

  entry:
  r0 = cn_var_值;
  r1 = r0 > 0;
  return r1;
}

long long* 创建整数指针(long long cn_var_值) {
  long long r0;
  long long* r1;
  long long* r2;

  entry:
  long long* cn_var_指针;
  r0 = cn_var_值;
  r1 = &cn_var_值;
  cn_var_指针 = r1;
  r2 = cn_var_指针;
  return r2;
}

long long 执行操作() {

  entry:
}

long long 绝对值(long long cn_var_值) {
  long long r0, r2, r3, r4;
  _Bool r1;

  entry:
  r0 = cn_var_值;
  r1 = r0 < 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  r2 = cn_var_值;
  /* Unsupported inst 10 */
  return r3;
  goto if_merge_1;

  if_merge_1:
  r4 = cn_var_值;
  return r4;
}

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4;

  entry:
  long long cn_var_num;
  goto entry_inline_获取整数;
  cn_rt_exit();
}

