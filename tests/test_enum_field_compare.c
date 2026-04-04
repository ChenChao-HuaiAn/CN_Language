#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 颜色 {
    红,
    绿,
    蓝
};

// CN Language Global Struct Definitions
struct 物体 {
    long long 颜色值;
    long long 大小;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r1;
  _Bool r2;
  struct 物体 r0;

  entry:
  struct 物体 cn_var_球_0;
  r0 = cn_var_球_0;
  r1 = r0.颜色值;
  r2 = r1 == 0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
  cn_rt_exit();
}

