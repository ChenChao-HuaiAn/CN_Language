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
  long long r3, r6, r8;
  void* r0;
  struct 数据* r1;
  struct 数据* r4;
  struct 数据* r7;
  double r5;
  struct 数据 r2;

  entry:
  struct 数据* cn_var_ptr_0;
  r0 = 分配内存(8);
  cn_var_ptr_0 = r0;
  long long cn_var_结果_1;
  r1 = cn_var_ptr_0;
  r2 = *r1;
  r3 = r2.值;
  r4 = cn_var_ptr_0;
  r5 = r4->数值;
  r6 = r3 + (long long)cn_var_ptr->数值;
  cn_var_结果_1 = r6;
  r7 = cn_var_ptr_0;
  释放内存(r7);
  r8 = cn_var_结果_1;
  return r8;
  cn_rt_exit();
}

