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
  long long* r0;
  long long* r2;
  long long* r4;
  long long** r6;
  long long** r10;
  long long* r1;
  long long* r3;
  long long* r5;
  long long** r7;
  long long** r8;
  long long* r9;
  long long** r11;
  long long** r12;
  long long* r13;

  entry:
  long long cn_var_x_0;
  cn_var_x_0 = 10;
  long long cn_var_y_1;
  cn_var_y_1 = 20;
  double cn_var_pi_2;
  cn_var_pi_2 = 3.141590;
  char* cn_var_消息_3;
  cn_var_消息_3 = "你好，CN语言！";
  _Bool cn_var_flag_4;
  cn_var_flag_4 = 1;
  long long* cn_var_arr;
  cn_var_arr = cn_rt_array_alloc(8, 5);
  r0 = cn_var_arr;
  r1 = &r0[0];
  r1 = 1;
  r2 = cn_var_arr;
  r3 = &r2[1];
  r3 = 2;
  r4 = cn_var_arr;
  r5 = &r4[2];
  r5 = 3;
  long long** cn_var_matrix;
  cn_var_matrix = cn_rt_array_alloc(8, 2);
  r6 = cn_var_matrix;
  r7 = &r6[0];
  r8 = r7;
  r9 = &r8[0];
  r9 = 1;
  r10 = cn_var_matrix;
  r11 = &r10[0];
  r12 = r11;
  r13 = &r12[1];
  r13 = 2;
  return 0;
  cn_rt_exit();
}

