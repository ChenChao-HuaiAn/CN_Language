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
  void* r0;
  long long* r1;

  entry:
  long long* cn_var_p_0;
  r0 = 分配内存(4);
  cn_var_p_0 = r0;
  r1 = cn_var_p_0;
  释放内存(r1);
  return 0;
  cn_rt_exit();
}

