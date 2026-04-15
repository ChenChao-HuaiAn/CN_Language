#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 函数声明;
struct 声明节点;

// CN Language Global Struct Definitions
struct 函数声明 {
    char* 名称;
    long long 参数个数;
};

struct 声明节点 {
    struct 函数声明* 作为函数声明;
};

// Global Variables

// Forward Declarations
long long 测试MOV类型传播();
long long main();

long long 测试MOV类型传播() {
  struct 函数声明* r0;
  struct 函数声明* r1;
  _Bool r2;

  entry:
  struct 声明节点 cn_var_声明_0;
  struct 函数声明* cn_var_函数声明指针_1;
  r0 = cn_var_声明_0.作为函数声明;
  cn_var_函数声明指针_1 = r0;
  r1 = cn_var_函数声明指针_1;
  r2 = r1 != 0;
  if (r2) goto if_then_0; else goto if_else_1;

  if_then_0:
  return 1;
  goto if_merge_2;

  if_else_1:
  return 0;
  goto if_merge_2;

  if_merge_2:
  return 0;
}

long long main() {
  cn_rt_init();
  long long r0;

  entry:
  r0 = 测试MOV类型传播();
  return r0;
  cn_rt_exit();
}

