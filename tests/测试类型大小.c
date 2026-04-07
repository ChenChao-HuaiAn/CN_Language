#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct IR生成上下文 {
    long long 数据;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r1, r2;
  struct IR生成上下文 r0;

  entry:
  struct IR生成上下文 cn_var_IR上下文_0;
  long long cn_var_大小_1;
  r0 = cn_var_IR生成上下文;
  r1 = 类型大小(r0);
  cn_var_大小_1 = r1;
  r2 = cn_var_大小_1;
  return r2;
  cn_rt_exit();
}

