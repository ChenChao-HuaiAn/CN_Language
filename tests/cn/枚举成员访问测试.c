#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 状态 {
    状态_待处理 = 0,
    状态_处理中 = 1,
    状态_已完成 = 2
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0;

  entry:
  long long cn_var_值_0;
  cn_var_值_0 = 2;
  r0 = cn_var_值_0;
  return r0;
  cn_rt_exit();
}

