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

  entry:
  struct 点* cn_var_p_0;
  cn_var_p_0 = 0;
  return 0;
  cn_rt_exit();
}

