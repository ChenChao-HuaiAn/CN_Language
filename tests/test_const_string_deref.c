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
  char* r0;
  char r1;

  entry:
  char* cn_var_源指针_0;
  cn_var_源指针_0 = "测试";
  char cn_var_首字符_1;
  r0 = cn_var_源指针_0;
  r1 = *r0;
  cn_var_首字符_1 = r1;
  return 0;
  cn_rt_exit();
}

