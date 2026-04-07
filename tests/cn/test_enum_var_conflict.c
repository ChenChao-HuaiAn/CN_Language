#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 状态 {
    状态_值 = 40,
    状态_其他 = 100
};

// CN Language Global Struct Forward Declarations
struct 数据;

// CN Language Global Struct Definitions
struct 数据 {
    long long 值;
    long long 数量;
};

// Global Variables

// Forward Declarations
long long 测试参数冲突(struct 数据*);
long long main();

long long 测试参数冲突(struct 数据* cn_var_值) {
  long long r1;
  struct 数据* r0;

  entry:
  r0 = cn_var_值;
  r1 = r0->数量;
  return r1;
}

long long main() {
  cn_rt_init();
  long long r2, r3;
  struct 数据* r1;
  struct 数据 r0;

  entry:
  struct 数据 cn_var_d_0;
  long long cn_var_结果_1;
  r0 = cn_var_d_0;
  r1 = &cn_var_d_0;
  r2 = 测试参数冲突(r1);
  cn_var_结果_1 = r2;
  long long cn_var_枚举值_2;
  cn_var_枚举值_2 = 40;
  r3 = cn_var_结果_1;
  return r3;
  cn_rt_exit();
}

