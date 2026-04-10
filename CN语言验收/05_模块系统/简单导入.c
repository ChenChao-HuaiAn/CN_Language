#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块
extern long long cn_var_最大值;
extern char* cn_var_模块版本;
long long 绝对值(long long);
long long 平方(long long);
long long 除法(long long, long long);
long long 乘法(long long, long long);
long long 减法(long long, long long);
long long 加法(long long, long long);

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r6, r7, r8;
  char* r5;

  entry:
  long long cn_var_和_0;
  r0 = 加法(10, 20);
  cn_var_和_0 = r0;
  long long cn_var_差_1;
  r1 = 减法(30, 15);
  cn_var_差_1 = r1;
  long long cn_var_积_2;
  r2 = 乘法(5, 6);
  cn_var_积_2 = r2;
  long long cn_var_商_3;
  r3 = 除法(20, 4);
  cn_var_商_3 = r3;
  long long cn_var_上限_4;
  r4 = cn_var_最大值;
  cn_var_上限_4 = r4;
  char* cn_var_版本_5;
  r5 = cn_var_模块版本;
  cn_var_版本_5 = r5;
  long long cn_var_平方值_6;
  r6 = 平方(9);
  cn_var_平方值_6 = r6;
  long long cn_var_绝对值结果_7;
  r7 = -15;
  r8 = 绝对值(r7);
  cn_var_绝对值结果_7 = r8;
  return 0;
  cn_rt_exit();
}

