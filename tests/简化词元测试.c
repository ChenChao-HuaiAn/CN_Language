#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 托肯类型 {
    托肯类型_托肯如果,
    托肯类型_托肯否则,
    托肯类型_托肯标识符,
    托肯类型_托肯结束,
    托肯类型_托肯错误
};

// CN Language Global Struct Definitions
struct 托肯 {
    long long 类型字段;
    char* 值;
    long long 行号;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r1;
  _Bool r2;
  struct 托肯 r0;

  entry:
  struct 托肯 cn_var_测试托肯_0;
  r0 = cn_var_测试托肯_0;
  r1 = r0.类型字段;
  r2 = r1 == 0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("匹配成功");
  goto if_merge_1;

  if_merge_1:
  return 0;
  cn_rt_exit();
}

