#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Global Struct Forward Declarations
struct 词元;

// CN Language Global Struct Definitions
struct 词元 {
    long long 类型;
    void* 值;
    long long 行号;
};

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  void* r0;
  void* r1;
  char* r2;

  entry:
  struct 词元 cn_var_结果_0;
  void* cn_var_ptr_1;
  r0 = 分配内存(100);
  cn_var_ptr_1 = r0;
  char* cn_var_buf_2;
  r1 = cn_var_ptr_1;
  cn_var_buf_2 = r1;
  r2 = cn_var_buf_2;
  return 0;
}

