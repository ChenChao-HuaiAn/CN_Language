#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 使用测试();

long long 使用测试() {
  long long r0, r1;

  entry:
  r0 = 测试函数();
  r1 = r0 << 1;
  return r1;
}

