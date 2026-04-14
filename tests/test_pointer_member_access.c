#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 数据;

// CN Language Global Struct Definitions
struct 数据 {
    long long 值;
    long long 编号;
};

// Global Variables

// Forward Declarations
long long 测试指针成员访问(struct 数据*);
long long main();

long long 测试指针成员访问(struct 数据* cn_var_指针) {
  long long r0, r1, r2, r3, r4;

  entry:
  long long cn_var_值1_0;
  r0 = cn_var_指针->值;
  cn_var_值1_0 = r0;
  long long cn_var_值2_1;
  r1 = cn_var_指针->编号;
  cn_var_值2_1 = r1;
  r2 = cn_var_值1_0;
  r3 = cn_var_值2_1;
  r4 = r2 + r3;
  return r4;
}

long long main() {
  cn_rt_init();
  long long r2, r3;
  struct 数据* r1;
  struct 数据 r0;

  entry:
  struct 数据 cn_var_实例_0;
  long long cn_var_结果_1;
  r0 = cn_var_实例_0;
  r1 = &cn_var_实例_0;
  r2 = 测试指针成员访问(r1);
  cn_var_结果_1 = r2;
  r3 = cn_var_结果_1;
  return r3;
  cn_rt_exit();
}

