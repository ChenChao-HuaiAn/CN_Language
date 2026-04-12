#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
long long main();

long long main() {
  cn_rt_init();
  void* r2;
  void* r4;
  void* r6;
  void* r8;
  char* r3;
  char* r5;
  char* r7;
  _Bool r1;
  char r0;

  entry:
  char cn_var_ch_0;
  cn_var_ch_0 = 65;
  r0 = cn_var_ch_0;
  r1 = r0 == 65;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("字符比较成功");
  goto if_merge_1;

  if_merge_1:
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 64);
  r2 = cn_var_缓冲区;
  r3 = &r2[0];
  r3 = 72;
  r4 = cn_var_缓冲区;
  r5 = &r4[1];
  r5 = 105;
  r6 = cn_var_缓冲区;
  r7 = &r6[2];
  r7 = 0;
  r8 = cn_var_缓冲区;
  cn_rt_print_string(r8);
  return 0;
  cn_rt_exit();
}

