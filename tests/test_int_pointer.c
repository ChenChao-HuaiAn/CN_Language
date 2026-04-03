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
  long long* r1;

  entry:
  long long cn_var_值_0;
  cn_var_值_0 = 42;
  long long* cn_var_指针_1;
  r0 = cn_var_值_0;
  r1 = &cn_var_值_0;
  cn_var_指针_1 = r1;
  return 0;
  cn_rt_exit();
}

