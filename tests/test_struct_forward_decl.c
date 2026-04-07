#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct A;
struct B;

// CN Language Global Struct Definitions
struct A {
    struct B* 指针;
};

struct B {
    long long 值;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r5;
  struct B* r1;
  struct B* r3;
  struct B r0;
  struct A r2;
  struct B r4;

  entry:
  struct A cn_var_变量A_0;
  struct B cn_var_变量B_1;
  r0 = cn_var_变量B_1;
  r1 = &cn_var_变量B_1;
  r2 = cn_var_变量A_0;
  r3 = r2.指针;
  r4 = *r3;
  r5 = r4.值;
  return r5;
  cn_rt_exit();
}

