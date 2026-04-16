#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 类型信息;

// CN Language Global Struct Definitions
struct 类型信息 {
    long long 种类;
    long long 指针层级;
};

// Global Variables

// Forward Declarations
long long 获取种类(struct 类型信息*);
long long main();

long long 获取种类(struct 类型信息* cn_var_信息指针) {
  long long r0;

  entry:
  r0 = cn_var_信息指针->种类;
  return r0;
}

long long main() {
  cn_rt_init();
  long long r2;
  struct 类型信息* r1;
  struct 类型信息 r0;

  entry:
  struct 类型信息 cn_var_信息_0;
  cn_var_信息_0 = (struct 类型信息){.种类 = 1, .指针层级 = 0};
  long long cn_var_结果_1;
  r0 = cn_var_信息_0;
  r1 = &cn_var_信息_0;
  r2 = 获取种类(r1);
  cn_var_结果_1 = r2;
  return 0;
  cn_rt_exit();
}

