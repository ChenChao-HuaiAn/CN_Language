#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 节点类型 {
    节点类型_字面量,
    节点类型_标识符,
    节点类型_二元表达式
};

// Global Variables

// Forward Declarations
long long 测试枚举Switch();
long long main();

long long 测试枚举Switch() {
  long long r0;
  _Bool r1;
  _Bool r2;

  entry:
  long long cn_var_类型_0;
  cn_var_类型_0 = 0;
  r0 = cn_var_类型_0;
  r1 = r0 == 0;
  if (r1) goto case_body_1; else goto switch_check_4;

  switch_check_4:
  r2 = r0 == 1;
  if (r2) goto case_body_2; else goto case_default_3;

  case_body_1:
  return 1;
  goto switch_merge_0;

  case_body_2:
  return 2;
  goto switch_merge_0;

  case_default_3:
  return 0;
  goto switch_merge_0;

  switch_merge_0:
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  long long cn_var_结果_0;
  r0 = 测试枚举Switch();
  cn_var_结果_0 = r0;
  r1 = cn_var_结果_0;
  return r1;
  cn_rt_exit();
}

