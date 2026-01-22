#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

long long main() {

  entry:
  cn_rt_print_string("你好，世界！\n");
  return 0;
}

