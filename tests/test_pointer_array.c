#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 符号;

// CN Language Global Struct Definitions
struct 符号 {
    char* 名称;
    long long 值;
};

// Global Variables
struct 符号* cn_var_全局符号表[10];

// Forward Declarations
long long 主();

long long 主() {

  entry:
  struct 符号** cn_var_局部符号表;
  cn_var_局部符号表 = cn_rt_array_alloc(8, 10);
  return 0;
}

