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
_Bool 是暖色(enum 颜色);
long long main();

_Bool 是暖色(enum 颜色 cn_var_c) {
  long long r0;
  _Bool r2;
  _Bool r4;
  enum 颜色 r1;
  enum 颜色 r3;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 颜色_红;
  if (r2) goto logic_rhs_2; else goto logic_merge_3;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;

  logic_rhs_2:
  r3 = cn_var_c;
  r4 = r3 <= 颜色_黄;
  goto logic_merge_3;

  logic_merge_3:
  if (r4) goto if_then_0; else goto if_merge_1;
  return 0;
}

long long main() {
  cn_rt_init();
  _Bool r1;
  enum 颜色 r0;

  entry:
  enum 颜色 cn_var_col_0;
  cn_var_col_0 = 颜色_绿;
  r0 = cn_var_col_0;
  r1 = 是暖色(r0);
  if (r1) goto if_then_4; else goto if_merge_5;

  if_then_4:
  return 1;
  goto if_merge_5;

  if_merge_5:
  return 0;
  cn_rt_exit();
}

