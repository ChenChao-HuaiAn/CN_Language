#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 内层结构体;
struct 外层结构体;

// CN Language Global Struct Definitions
struct 内层结构体 {
    long long 值;
};

struct 外层结构体 {
    struct 内层结构体* 指针字段;
    struct 内层结构体 值字段;
};

// Global Variables

// Forward Declarations
long long 测试指针字段();

long long 测试指针字段() {
  long long r3;
  struct 内层结构体* r1;
  struct 内层结构体* r2;
  struct 外层结构体 r0;

  entry:
  struct 外层结构体 cn_var_外层_0;
  struct 内层结构体* cn_var_指针_1;
  r0 = cn_var_外层_0;
  r1 = r0.指针字段;
  cn_var_指针_1 = r1;
  r2 = cn_var_指针_1;
  r3 = r2->值;
  return r3;
}

