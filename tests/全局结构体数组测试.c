#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
};

// Global Variables
struct 测试结构* cn_var_全局数组 = 0;

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  return 0;
  cn_rt_exit();
}

