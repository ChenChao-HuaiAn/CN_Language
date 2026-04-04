#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables
long long cn_var_全局变量 = 10;

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0;

  entry:
  long long cn_var_局部变量_0;
  r0 = cn_var_全局变量;
  cn_var_局部变量_0 = r0;
  return 0;
  cn_rt_exit();
}

