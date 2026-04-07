#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
    char* 名称;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  struct 测试结构 cn_var_obj_0;
  cn_var_obj_0 = (struct 测试结构){.值 = 1, .名称 = "测试"};
  return 0;
  cn_rt_exit();
}

