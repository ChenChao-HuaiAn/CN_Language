#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
    char* 名称;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  void* r0;
  struct 测试结构* r1;
  struct 测试结构* r3;
  _Bool r2;

  entry:
  struct 测试结构* cn_var_p_0;
  r0 = 分配内存(16);
  cn_var_p_0 = r0;
  r1 = cn_var_p_0;
  r2 = r1 == 0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  r3 = cn_var_p_0;
  释放内存(r3);
  return 0;
  cn_rt_exit();
}

