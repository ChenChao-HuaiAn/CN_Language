#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
// 取消运行时宏定义，避免与用户函数名冲突
#undef 打印整数

long long 打印问候();
long long 打印整数(long long);
long long 加法(long long, long long);
double 计算平均值(long long, long long, long long);
char* 拼接字符串(const char*, const char*);
long long main();

long long 打印问候() {

  entry:
  return 0;
}

long long 打印整数(long long cn_var_值) {

  entry:
  return 0;
}

long long 加法(long long cn_var_a, long long cn_var_b) {
  long long r0, r1, r2;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  return r2;
}

double 计算平均值(long long cn_var_a, long long cn_var_b, long long cn_var_c) {
  long long r0, r1, r2, r3, r4;
  double r5;

  entry:
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  r3 = cn_var_c;
  r4 = r2 + r3;
  r5 = r4 / 3.000000;
  return r5;
}

char* 拼接字符串(const char* cn_var_前缀, const char* cn_var_后缀) {
  char* r0;
  char* r1;
  char* r2;

  entry:
  r0 = cn_var_前缀;
  r1 = cn_var_后缀;
  r2 = cn_rt_string_concat(r0, r1);
  return r2;
}

long long main() {
  cn_rt_init();
  long long r0;
  double r1;

  entry:
  打印问候();
  long long cn_var_结果_0;
  r0 = 加法(10, 20);
  cn_var_结果_0 = r0;
  double cn_var_平均值_1;
  r1 = 计算平均值(80, 90, 85);
  cn_var_平均值_1 = r1;
  return 0;
  cn_rt_exit();
}

