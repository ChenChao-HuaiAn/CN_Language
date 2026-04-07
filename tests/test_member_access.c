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
  long long r1, r3, r4, r5;
  struct 点 r0;
  struct 点 r2;

  entry:
  struct 点 cn_var_p_0;
  long long cn_var_总和_1;
  r0 = cn_var_p_0;
  r1 = r0.x;
  r2 = cn_var_p_0;
  r3 = r2.y;
  r4 = r1 + r3;
  cn_var_总和_1 = r4;
  r5 = cn_var_总和_1;
  return r5;
  cn_rt_exit();
}

