#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 点;

// CN Language Global Struct Definitions
struct 点 {
    long long x;
    long long y;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2, r3, r4;
  struct 点* r1;
  struct 点 r0;

  entry:
  struct 点 cn_var_p_0;
  struct 点* cn_var_ptr_1;
  r0 = cn_var_p_0;
  r1 = &cn_var_p_0;
  cn_var_ptr_1 = r1;
  r2 = cn_var_p_0.x;
  r3 = cn_var_p_0.y;
  r4 = r2 + r3;
  return r4;
  cn_rt_exit();
}

