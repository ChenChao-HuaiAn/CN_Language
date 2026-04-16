#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 符号;

// CN Language Global Struct Definitions
struct 符号 {
    long long 种类;
    long long 标志;
};

// Global Variables

// Forward Declarations
long long 测试指针成员访问(struct 符号*);
long long 测试值成员访问(struct 符号);
long long main();

long long 测试指针成员访问(struct 符号* cn_var_指针) {
  long long r0;

  entry:
  r0 = cn_var_指针->种类;
  return r0;
}

long long 测试值成员访问(struct 符号 cn_var_值) {
  long long r0;

  entry:
  r0 = cn_var_值.标志;
  return r0;
}

long long main() {
  cn_rt_init();
  long long r2, r4, r5, r6, r7;
  struct 符号* r1;
  struct 符号 r0;
  struct 符号 r3;

  entry:
  struct 符号 cn_var_符号实例_0;
  cn_var_符号实例_0 = (struct 符号){};
  long long cn_var_结果1_1;
  r0 = cn_var_符号实例_0;
  r1 = &cn_var_符号实例_0;
  r2 = 测试指针成员访问(r1);
  cn_var_结果1_1 = r2;
  long long cn_var_结果2_2;
  r3 = cn_var_符号实例_0;
  r4 = 测试值成员访问(r3);
  cn_var_结果2_2 = r4;
  r5 = cn_var_结果1_1;
  r6 = cn_var_结果2_2;
  r7 = r5 + r6;
  return r7;
  cn_rt_exit();
}

