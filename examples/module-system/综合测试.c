#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations - 从导入模块
long long 加法(long long a, long long b);
long long 减法(long long a, long long b);
long long 乘法(long long a, long long b);
long long 测试函数();
long long 获取版本();
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  long long cn_var_和;
  r0 = 加法(10, 20);
  cn_var_和 = r0;
  long long cn_var_差;
  r1 = 减法(30, 15);
  cn_var_差 = r1;
  long long cn_var_积;
  r2 = 乘法(5, 6);
  cn_var_积 = r2;
  long long cn_var_测试值;
  r3 = 测试函数();
  cn_var_测试值 = r3;
  long long cn_var_版本;
  r4 = 获取版本();
  cn_var_版本 = r4;
  cn_rt_print_string("加法: ");
  r5 = cn_var_和;
  cn_rt_print_int(r5);
  cn_rt_print_string("\n");
  cn_rt_print_string("减法: ");
  r6 = cn_var_差;
  cn_rt_print_int(r6);
  cn_rt_print_string("\n");
  cn_rt_print_string("乘法: ");
  r7 = cn_var_积;
  cn_rt_print_int(r7);
  cn_rt_print_string("\n");
  cn_rt_print_string("测试值: ");
  r8 = cn_var_测试值;
  cn_rt_print_int(r8);
  cn_rt_print_string("\n");
  cn_rt_print_string("版本: ");
  r9 = cn_var_版本;
  cn_rt_print_int(r9);
  cn_rt_print_string("\n");
  return 0;
  cn_rt_exit();
}

