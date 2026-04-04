#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
void* 分配内存(long long);
long long main();

void* 分配内存(long long cn_var_大小);
long long main() {
  cn_rt_init();

  entry:
  return 0;
  cn_rt_exit();
}

