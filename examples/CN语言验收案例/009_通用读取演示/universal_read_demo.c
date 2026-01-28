#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2, r4, r5, r8, r10, r11, r12;
  char* r0;
  char* r1;
  long long* r3;
  double* r7;
  double r6;
  double r9;

  entry:
  cn_rt_print_string("请输入您的名字：");
  char* cn_var_名字;
  r0 = cn_rt_read_line();
  cn_var_名字 = r0;
  cn_rt_print_string("您好，");
  r1 = cn_var_名字;
  cn_rt_print_string(r1);
  cn_rt_print_string("\n");
  cn_rt_print_string("请输入一个整数：");
  long long cn_var_数值;
  r2 = cn_var_数值;
  r3 = &cn_var_数值;
  r4 = cn_rt_read_int(r3);
  cn_rt_print_string("您输入的整数是：");
  r5 = cn_var_数值;
  cn_rt_print_int(r5);
  cn_rt_print_string("\n");
  cn_rt_print_string("请输入一个小数：");
  double cn_var_浮点数;
  r6 = cn_var_浮点数;
  r7 = &cn_var_浮点数;
  r8 = cn_rt_read_float(r7);
  cn_rt_print_string("您输入的小数是：");
  r9 = cn_var_浮点数;
  cn_rt_print_float(r9);
  cn_rt_print_string("\n");
  long long cn_var_倍数;
  r10 = cn_var_数值;
  r11 = r10 * 2;
  cn_var_倍数 = r11;
  cn_rt_print_string("整数的两倍是：");
  r12 = cn_var_倍数;
  cn_rt_print_int(r12);
  cn_rt_print_string("\n");
  return 0;
  cn_rt_exit();
}

