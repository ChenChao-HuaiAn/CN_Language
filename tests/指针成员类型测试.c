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
    long long 标识;
};

struct 声明节点 {
    struct 函数声明* 作为函数声明;
};

// Global Variables

// Forward Declarations
long long 测试指针成员类型();

long long 测试指针成员类型() {
  long long r3;
  struct 函数声明* r1;
  struct 函数声明* r2;
  struct 函数声明 r0;

  entry:
  struct 声明节点 cn_var_节点_0;
  struct 函数声明 cn_var_声明_1;
  r0 = cn_var_声明_1;
  r1 = &cn_var_声明_1;
  struct 函数声明* cn_var_指针_2;
  r2 = cn_var_节点_0.作为函数声明;
  cn_var_指针_2 = r2;
  r3 = cn_var_指针_2->标识;
  return r3;
}

