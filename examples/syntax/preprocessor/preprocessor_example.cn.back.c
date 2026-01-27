#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int cn_func_find_max(int, int);
int cn_func_find_min(int, int);
int main();

int cn_func_find_max(int cn_var_x, int cn_var_y) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  r0 = cn_var_x;
  r1 = cn_var_y;
  r2 = r0 > r1;
  if (r2) goto ternary_true_0; else goto ternary_false_1;
}

int cn_func_find_min(int cn_var_x, int cn_var_y) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  r0 = cn_var_x;
  r1 = cn_var_y;
  r2 = r0 < r1;
  if (r2) goto ternary_true_3; else goto ternary_false_4;
}

int main() {
  cn_rt_init();
  long long r0, r1, r3, r4;

  entry:
  int cn_var_a;
  cn_var_a = 50;
  int cn_var_b;
  cn_var_b = 75;
  int cn_var_max_value;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = cn_func_find_max(r0, r1);
  cn_var_max_value = r2;
  int cn_var_min_value;
  r3 = cn_var_a;
  r4 = cn_var_b;
  r5 = cn_func_find_min(r3, r4);
  cn_var_min_value = r5;
  cn_rt_print_string("调试模式已启用\n");
  return 0;
  cn_rt_exit();
}

