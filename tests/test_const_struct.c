#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 源位置;

// CN Language Global Struct Definitions
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};

// Global Variables
struct 源位置 cn_var_空源位置 = {0, 0, 0};

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  return 0;
  cn_rt_exit();
}

