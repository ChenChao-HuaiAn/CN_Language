#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
void* 测试返回空指针();
long long main();

void* 测试返回空指针() {

  entry:
  return 0;
}

long long main() {
  cn_rt_init();

  entry:
  return 0;
  cn_rt_exit();
}

