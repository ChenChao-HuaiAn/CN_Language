#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Definitions
struct 词元 {
    long long 类型;
    long long 行号;
    long long 列号;
    long long 长度;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  long long r0, r1, r2, r3, r5;
  struct 词元 r4;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始列号_1;
  cn_var_起始列号_1 = 5;
  long long cn_var_位置_2;
  cn_var_位置_2 = 10;
  r0 = cn_var_起始列号_1;
  r1 = cn_var_位置_2;
  r2 = cn_var_起始列号_1;
  r3 = r1 - r2;
  r4 = cn_var_结果_0;
  r5 = r4.长度;
  return r5;
}

