#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 数组字段测试;

// CN Language Global Struct Definitions
struct 数组字段测试 {
    long long* 数组字段;
    long long** 指针数组字段;
    char** 字符串数组;
    double* 固定大小数组;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  return 0;
  cn_rt_exit();
}

