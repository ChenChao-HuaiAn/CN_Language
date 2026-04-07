#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 测试结构;

// CN Language Global Struct Definitions
struct 测试结构 {
    char* 名称;
    long long 值;
};

// Global Variables
struct 测试结构* cn_var_全局数组 = 0;
_Bool cn_var_已初始化 = 0;

// Forward Declarations
void 初始化数组();
long long main();

void 初始化数组() {
  _Bool r0;

  entry:
  r0 = cn_var_已初始化;
  if (r0) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return;
  goto if_merge_1;

  if_merge_1:
  cn_var_已初始化 = 1;
  return;
}

long long main() {
  cn_rt_init();

  entry:
  初始化数组();
  return 0;
  cn_rt_exit();
}

