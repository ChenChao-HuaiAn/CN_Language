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

  entry:
  long long cn_var_整数值_0;
  cn_var_整数值_0 = 42;
  double cn_var_小数值_1;
  cn_var_小数值_1 = 3.141590;
  char* cn_var_文本_2;
  cn_var_文本_2 = "你好，世界！";
  _Bool cn_var_是真_3;
  cn_var_是真_3 = 1;
  _Bool cn_var_是假_4;
  cn_var_是假_4 = 0;
  void* cn_var_通用指针_5;
  cn_var_通用指针_5 = 0;
  long long cn_var_十进制_6;
  cn_var_十进制_6 = 123;
  long long cn_var_十六进制_7;
  cn_var_十六进制_7 = 255;
  long long cn_var_二进制_8;
  cn_var_二进制_8 = 10;
  long long cn_var_八进制_9;
  cn_var_八进制_9 = 493;
  double cn_var_科学计数法_10;
  cn_var_科学计数法_10 = 0.001500;
  return 0;
  cn_rt_exit();
}

