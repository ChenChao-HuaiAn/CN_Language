#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

int main() {
  cn_rt_init();
  long long r0, r4;
  char* r1;
  char* r2;
  char* r3;

  entry:
  cn_rt_print_string("  你好, CN_Language!  ");
  cn_rt_print_string("\n");
  int cn_var_长度值;
  r0 = cn_rt_string_length("  你好, CN_Language!  ");
  cn_var_长度值 = r0;
  cn_rt_print_string("文本长度: ");
  char* cn_var_文本;
  cn_var_文本 = "  你好, CN_Language!  ";
  char* cn_var_修整后的文本;
  r1 = cn_var_文本;
  cn_var_修整后的文本 = r1;
  r2 = cn_var_文本;
  cn_rt_print_string(r2);
  cn_rt_print_string("\n");
  int cn_var_文本长度;
  r3 = cn_var_文本;
  r4 = cn_rt_string_length(r3);
  cn_var_文本长度 = r4;
  return 0;
  cn_rt_exit();
}

