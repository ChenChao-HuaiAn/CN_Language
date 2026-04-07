#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 结构体A;
struct 结构体B;

// CN Language Global Struct Definitions
struct 结构体B;
struct 结构体A {
    long long 值;
    struct 结构体B* 指向B;
};

struct 结构体A;
struct 结构体B {
    long long 数据;
    struct 结构体A* 指向A;
};

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();

  entry:
  struct 结构体A cn_var_变量A_0;
  struct 结构体B cn_var_变量B_1;
  return 0;
  cn_rt_exit();
}

