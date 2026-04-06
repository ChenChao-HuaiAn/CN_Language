#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 基础信息 {
    long long 类型;
};

struct 声明节点 {
    struct 基础信息 基础;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2;
  _Bool r3;
  struct 声明节点 r0;
  struct 基础信息 r1;

  entry:
  struct 声明节点 cn_var_声明_0;
  r0 = cn_var_声明_0;
  r1 = r0.基础;
  r2 = r1.类型;
  r3 = r2 == 1;
  if (r3) goto case_body_1; else goto case_default_2;

  case_body_1:
  return 1;
  goto switch_merge_0;

  case_default_2:
  return 0;
  goto switch_merge_0;

  switch_merge_0:
  return 0;
  cn_rt_exit();
}

