#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 测试枚举 {
    值一,
    值二,
    值三
};

// CN Language Global Struct Definitions
struct 测试结构 {
    enum 测试枚举 值;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  struct 测试结构 cn_var_obj_0;
  return 0;
  cn_rt_exit();
}

