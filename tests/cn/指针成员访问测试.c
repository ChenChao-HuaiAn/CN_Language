#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 函数声明;

// CN Language Global Struct Definitions
struct 函数声明 {
    long long 名称;
    long long 参数数量;
};

// Global Variables

// Forward Declarations
long long 测试指针成员访问();
long long main();

long long 测试指针成员访问() {
  long long r0, r1;

  entry:
  struct 函数声明* cn_var_声明指针_0;
  long long cn_var_名称值_1;
  r0 = cn_var_声明指针_0->名称;
  cn_var_名称值_1 = r0;
  r1 = cn_var_名称值_1;
  return r1;
}

long long main() {
  cn_rt_init();
  long long r0, r1;

  entry:
  long long cn_var_结果_0;
  r0 = 测试指针成员访问();
  cn_var_结果_0 = r0;
  r1 = cn_var_结果_0;
  return r1;
  cn_rt_exit();
}

