#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 内层;
struct 外层;

// CN Language Global Struct Definitions
struct 内层 {
    long long 值;
};

struct 外层 {
    struct 内层 成员;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r1;
  struct 内层 r0;

  entry:
  struct 外层 cn_var_对象_0;
  r0 = cn_var_对象_0.成员;
  r1 = r0.值;
  return r1;
  cn_rt_exit();
}

