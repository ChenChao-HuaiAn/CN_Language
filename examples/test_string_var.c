#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r4;
  char* r0;
  char* r1;
  char* r2;
  char* r3;

  entry:
  char* cn_var_文本;
  cn_var_文本 = "Hello, 世界!";
  r0 = cn_var_文本;
  cn_rt_print_string(r0);
  cn_rt_print_string("\n");
  char* cn_var_文本2;
  r1 = cn_var_文本;
  cn_var_文本2 = r1;
  r2 = cn_var_文本2;
  cn_rt_print_string(r2);
  cn_rt_print_string("\n");
  int cn_var_长度;
  r3 = cn_var_文本;
  r4 = cn_rt_string_length(r3);
  cn_var_长度 = r4;
  return 0;
  cn_rt_exit();
}

