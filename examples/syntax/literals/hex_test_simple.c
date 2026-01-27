#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5;

  entry:
  int cn_var_a;
  cn_var_a = 0;
  int cn_var_b;
  cn_var_b = 0;
  int cn_var_c;
  cn_var_c = 0;
  int cn_var_d;
  cn_var_d = 0;
  cn_rt_print_string("0x1000 = ");
  r0 = cn_var_a;
  cn_rt_print_int(r0);
  cn_rt_print_string("\n");
  cn_rt_print_string("0xFF = ");
  r1 = cn_var_b;
  cn_rt_print_int(r1);
  cn_rt_print_string("\n");
  cn_rt_print_string("0xABCD = ");
  r2 = cn_var_c;
  cn_rt_print_int(r2);
  cn_rt_print_string("\n");
  cn_rt_print_string("0X10 = ");
  r3 = cn_var_d;
  cn_rt_print_int(r3);
  cn_rt_print_string("\n");
  int cn_var_sum;
  r4 = 0;
  cn_var_sum = r4;
  cn_rt_print_string("0x10 + 0x20 = ");
  r5 = cn_var_sum;
  cn_rt_print_int(r5);
  cn_rt_print_string("\n");
  return 0;
  cn_rt_exit();
}

