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
  long long r0;

  entry:
  long long cn_var_n;
  cn_var_n = 5;
  r0 = cn_var_n;
  return r0;
  cn_rt_exit();
}

