#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 类型信息 {
    long long 大小;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0;

  entry:
  void* cn_var_ptr_0;
  cn_var_ptr_0 = (void*)0;
  struct 类型信息* cn_var_类型_1;
  r0 = cn_var_ptr_0;
  cn_var_类型_1 = (struct 类型信息*)cn_var_ptr;
  return 0;
  cn_rt_exit();
}

