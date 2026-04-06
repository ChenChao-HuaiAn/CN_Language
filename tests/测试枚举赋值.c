#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型 {
    词元类型_整数字面量,
    词元类型_浮点字面量,
    词元类型_错误
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r1;
  _Bool r0;

  entry:
  enum 词元类型 cn_var_结果_0;
  _Bool cn_var_是浮点数_1;
  cn_var_是浮点数_1 = 0;
  r0 = cn_var_是浮点数_1;
  r1 = (r0 ? 1 : 0);
  cn_var_结果_0 = r1;
  return 0;
}

