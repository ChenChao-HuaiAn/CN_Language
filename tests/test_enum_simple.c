#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 状态 {
    状态_成功,
    状态_失败
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  enum 状态 r0;

  entry:
  enum 状态 cn_var_s_0;
  cn_var_s_0 = 状态_成功;
  r0 = cn_var_s_0;
  cn_rt_print_int(r0);
  return 0;
  cn_rt_exit();
}

