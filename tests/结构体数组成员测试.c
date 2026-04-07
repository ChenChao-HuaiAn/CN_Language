#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  struct 测试结构* cn_var_arr;
  cn_var_arr = cn_rt_array_alloc(8, 10);
  return 0;
  cn_rt_exit();
}

