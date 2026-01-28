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
  long long r0, r2, r3;
  char* r4;
  char* r5;
  long long* r1;

  entry:
  cn_rt_print_string("CN语言输入功能测试\n");
  cn_rt_print_string("===================\n\n");
  cn_rt_print_string("请输入一个整数：");
  long long cn_var_数值;
  r0 = cn_var_数值;
  r1 = &cn_var_数值;
  r2 = cn_rt_read_int(r1);
  cn_rt_print_string("您输入的数值是：");
  r3 = cn_var_数值;
  cn_rt_print_int(r3);
  cn_rt_print_string("\n");
  cn_rt_print_string("请输入您的名字：");
  char* cn_var_名字;
  r4 = cn_rt_read_line();
  cn_var_名字 = r4;
  cn_rt_print_string("您好，");
  r5 = cn_var_名字;
  cn_rt_print_string(r5);
  cn_rt_print_string("！\n");
  cn_rt_print_string("\n测试完成！\n");
  return 0;
  cn_rt_exit();
}

