#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  cn_rt_print_string("=== CN语言模块系统功能展示 ===\n\n");
  cn_rt_print_string("3. 包导入:\n");
  long long cn_var_版本;
  r0 = cn_func_获取版本();
  cn_var_版本 = r0;
  cn_rt_print_string("  工具包版本: ");
  r1 = cn_var_版本;
  cn_rt_print_int(r1);
  cn_rt_print_string("\n\n");
  cn_rt_print_string("=== 所有功能测试通过! ===\n");
  return 0;
  cn_rt_exit();
}

