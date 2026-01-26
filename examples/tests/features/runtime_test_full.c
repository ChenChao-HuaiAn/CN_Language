#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r0, r4, r7;
  char* r1;
  char* r2;
  char* r3;
  void* r5;
  void* r6;

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
  void* cn_var_整数数组;
  r5 = cn_rt_array_alloc(8, 5);
  cn_rt_array_set_element(r5, 0, ({long long _tmp_0 = 1; &_tmp_0;}), 8);
  cn_rt_array_set_element(r5, 1, ({long long _tmp_1 = 2; &_tmp_1;}), 8);
  cn_rt_array_set_element(r5, 2, ({long long _tmp_2 = 3; &_tmp_2;}), 8);
  cn_rt_array_set_element(r5, 3, ({long long _tmp_3 = 4; &_tmp_3;}), 8);
  cn_rt_array_set_element(r5, 4, ({long long _tmp_4 = 5; &_tmp_4;}), 8);
  cn_var_整数数组 = r5;
  int cn_var_数组长度;
  r6 = cn_var_整数数组;
  r7 = cn_rt_array_length(r6);
  cn_var_数组长度 = r7;
  return 0;
  cn_rt_exit();
}

