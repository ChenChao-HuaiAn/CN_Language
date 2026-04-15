#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 作用域;

// CN Language Global Struct Definitions
struct 作用域 {
    long long 种类;
    long long 父作用域;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r1;
  void* r0;

  entry:
  struct 作用域* cn_var_作用域指针_0;
  r0 = 分配内存(8);
  cn_var_作用域指针_0 = (struct 作用域*)0;
  r1 = cn_var_作用域指针_0->种类;
  return r1;
}

