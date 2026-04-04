#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 测试结构 {
    long long 值;
};

// Global Variables

// Forward Declarations
struct 测试结构* 测试分配();
long long 主();

struct 测试结构* 测试分配() {
  void* r0;
  struct 测试结构* r1;

  entry:
  struct 测试结构* cn_var_实例_0;
  r0 = 分配内存(8);
  cn_var_实例_0 = r0;
  r1 = cn_var_实例_0;
  return r1;
}

long long 主() {
  struct 测试结构* r0;

  entry:
  struct 测试结构* cn_var_p_0;
  r0 = 测试分配();
  cn_var_p_0 = r0;
  return 0;
}

