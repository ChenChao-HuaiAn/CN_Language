#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 颜色 {
    颜色_红,
    颜色_绿,
    颜色_蓝,
    颜色_黄
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r2;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  enum 颜色 r0;
  enum 颜色 r3;
  enum 颜色 r5;

  entry:
  enum 颜色 cn_var_c_0;
  cn_var_c_0 = 1;
  r0 = cn_var_c_0;
  r1 = r0 == 1;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  r3 = cn_var_c_0;
  r4 = r3 >= 0;
  if (r4) goto logic_rhs_4; else goto logic_merge_5;

  if_merge_1:
  return 0;

  if_then_2:
  return 1;
  goto if_merge_3;

  if_merge_3:
  goto if_merge_1;

  logic_rhs_4:
  r5 = cn_var_c_0;
  r6 = r5 <= 3;
  goto logic_merge_5;

  logic_merge_5:
  if (r6) goto if_then_2; else goto if_merge_3;
  return 0;
  cn_rt_exit();
}

