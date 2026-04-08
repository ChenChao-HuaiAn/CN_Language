#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 函数声明;

// CN Language Global Struct Definitions
struct 函数声明 {
    long long 名称;
};

// Global Variables

// Forward Declarations
long long 测试LOAD();
long long main();

long long 测试LOAD() {
  long long r2;
  struct 函数声明* r0;
  _Bool r1;

  entry:
  struct 函数声明* cn_var_声明_0;
  cn_var_声明_0 = 0;
  long long cn_var_结果_1;
  cn_var_结果_1 = 0;
  r0 = cn_var_声明_0;
  r1 = r0 != 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_var_结果_1 = 1;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_结果_1;
  return r2;
}

long long main() {
  cn_rt_init();
  long long r0;

  entry:
  r0 = 测试LOAD();
  return r0;
  cn_rt_exit();
}

