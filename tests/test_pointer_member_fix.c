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
    double 数值;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2, r3, r4, r6;
  void* r0;
  struct 数据* r5;
  struct 数据 r1;

  entry:
  struct 数据* cn_var_ptr_0;
  r0 = 分配内存(8);
  cn_var_ptr_0 = r0;
  long long cn_var_结果_1;
  r1 = *cn_var_ptr_0;
  r2 = r1.值;
  r3 = cn_var_ptr_0->数值;
  r4 = r2 + (long long)cn_var_ptr->数值;
  cn_var_结果_1 = r4;
  r5 = cn_var_ptr_0;
  释放内存(r5);
  r6 = cn_var_结果_1;
  return r6;
  cn_rt_exit();
}

