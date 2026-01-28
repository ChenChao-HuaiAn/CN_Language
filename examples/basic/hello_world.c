#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();

  entry:
  cn_rt_print_string("你好，世界！\n");
  return 0;
  cn_rt_exit();
}

