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
long long 测试指针访问(struct 测试结构*);
long long main();

long long 测试指针访问(struct 测试结构* cn_var_指针) {
  long long r1, r2;
  struct 测试结构* r0;

  entry:
  long long cn_var_结果_0;
  r0 = cn_var_指针;
  r1 = r0->值;
  cn_var_结果_0 = r1;
  r2 = cn_var_结果_0;
  return r2;
}

long long main() {
  cn_rt_init();
  long long r2, r3;
  struct 测试结构* r1;
  struct 测试结构 r0;

  entry:
  struct 测试结构 cn_var_数据_0;
  long long cn_var_结果_1;
  r0 = cn_var_数据_0;
  r1 = &cn_var_数据_0;
  r2 = 测试指针访问(r1);
  cn_var_结果_1 = r2;
  r3 = cn_var_结果_1;
  return r3;
  cn_rt_exit();
}

