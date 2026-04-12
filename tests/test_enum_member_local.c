#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型 {
    词元类型_词元错误,
    词元类型_词元标识符,
    词元类型_词元整数,
    词元类型_词元结束
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0;
  _Bool r1;

  entry:
  long long cn_var_值_0;
  cn_var_值_0 = 词元类型_词元标识符;
  r0 = cn_var_值_0;
  r1 = r0 == 词元类型_词元标识符;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  return 0;
  cn_rt_exit();
}

