#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long 主();

long long 主() {
  void* r0;
  void* r1;
  char* r2;

  entry:
  void* cn_var_ptr_0;
  r0 = 分配内存(100);
  cn_var_ptr_0 = r0;
  char* cn_var_buf_1;
  r1 = cn_var_ptr_0;
  cn_var_buf_1 = r1;
  char* cn_var_s_2;
  r2 = cn_var_buf_1;
  cn_var_s_2 = r2;
  return 0;
}

