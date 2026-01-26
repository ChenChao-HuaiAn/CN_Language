#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"

// Global Variables

// Forward Declarations
int cn_func_测试浮点声明();
int cn_func_测试浮点运算();
int cn_func_测试混合运算();
int cn_func_测试浮点比较();
int cn_func_计算圆面积(double);
int cn_func_测试浮点数组();
int cn_func_测试复杂表达式();
int main();

int cn_func_测试浮点声明() {
  double r0;

  entry:
  double cn_var_pi;
  cn_var_pi = 3.141590;
  double cn_var_e;
  cn_var_e = 2.718280;
  double cn_var_x;
  cn_var_x = 1.500000;
  cn_rt_print_string("Pi = ");
  r0 = cn_var_pi;
  cn_rt_print_float(r0);
  cn_rt_print_string("\n");
  return 0;
}

int cn_func_测试浮点运算() {
  double r0;
  double r1;
  double r2;
  double r3;
  double r4;
  double r5;
  double r6;
  double r7;
  double r8;
  double r9;
  double r10;
  double r11;

  entry:
  double cn_var_a;
  cn_var_a = 10.500000;
  double cn_var_b;
  cn_var_b = 2.500000;
  double cn_var_sum;
  r0 = cn_var_a;
  r1 = cn_var_b;
  r2 = r0 + r1;
  cn_var_sum = r2;
  double cn_var_diff;
  r3 = cn_var_a;
  r4 = cn_var_b;
  r5 = r3 - r4;
  cn_var_diff = r5;
  double cn_var_prod;
  r6 = cn_var_a;
  r7 = cn_var_b;
  r8 = r6 * r7;
  cn_var_prod = r8;
  double cn_var_quot;
  r9 = cn_var_a;
  r10 = cn_var_b;
  r11 = r9 / r10;
  cn_var_quot = r11;
  return 0;
}

int cn_func_测试混合运算() {
  long long r0, r4, r6;
  double r1;
  double r2;
  double r3;
  double r5;
  double r7;
  double r8;
  double r9;

  entry:
  int cn_var_i;
  cn_var_i = 10;
  double cn_var_f;
  cn_var_f = 3.500000;
  double cn_var_result1;
  r0 = cn_var_i;
  r1 = cn_var_f;
  r2 = r0 + r1;
  cn_var_result1 = r2;
  double cn_var_result2;
  r3 = cn_var_f;
  r4 = cn_var_i;
  r5 = r3 * r4;
  cn_var_result2 = r5;
  double cn_var_result3;
  r6 = cn_var_i;
  r7 = cn_var_f;
  r8 = r6 / r7;
  cn_var_result3 = r8;
  double cn_var_mixed;
  r9 = 5 + 2.500000;
  cn_var_mixed = r9;
  return 0;
}

int cn_func_测试浮点比较() {
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  double r0;
  double r1;
  double r3;
  double r5;
  double r6;
  double r8;
  double r9;

  entry:
  double cn_var_x;
  cn_var_x = 3.140000;
  double cn_var_y;
  cn_var_y = 2.710000;
  _Bool cn_var_greater;
  r0 = cn_var_x;
  r1 = cn_var_y;
  r2 = r0 > r1;
  cn_var_greater = r2;
  _Bool cn_var_equal;
  r3 = cn_var_x;
  r4 = r3 == 3.140000;
  cn_var_equal = r4;
  _Bool cn_var_less_eq;
  r5 = cn_var_y;
  r6 = cn_var_x;
  r7 = r5 <= r6;
  cn_var_less_eq = r7;
  r8 = cn_var_x;
  r9 = cn_var_y;
  r10 = r8 > r9;
  if (r10) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("x 大于 y\n");
  goto if_merge_1;

  if_merge_1:
  return 0;
}

int cn_func_计算圆面积(double cn_var_半径) {
  double r0;
  double r1;
  double r2;
  double r3;
  double r4;

  entry:
  double cn_var_pi;
  cn_var_pi = 3.141590;
  double cn_var_面积;
  r0 = cn_var_pi;
  r1 = cn_var_半径;
  r2 = r0 * r1;
  r3 = cn_var_半径;
  r4 = r2 * r3;
  cn_var_面积 = r4;
  return 0;
}

int cn_func_测试浮点数组() {
  void* r0;
  void* r1;
  double r2;

  entry:
  void* cn_var_temps;
  r0 = cn_rt_array_alloc(8, 4);
  cn_rt_array_set_element(r0, 0, &20.500000, 8);
  cn_rt_array_set_element(r0, 1, &21.300000, 8);
  cn_rt_array_set_element(r0, 2, &19.800000, 8);
  cn_rt_array_set_element(r0, 3, &22.100000, 8);
  cn_var_temps = r0;
  double cn_var_first;
  r1 = cn_var_temps;
  r2 = *(double*)cn_rt_array_get_element(r1, 0, 8);
  cn_var_first = r2;
  return 0;
}

int cn_func_测试复杂表达式() {
  long long r0, r4;
  double r1;
  double r2;
  double r3;
  double r5;

  entry:
  int cn_var_count;
  cn_var_count = 5;
  double cn_var_price;
  cn_var_price = 12.990000;
  double cn_var_total;
  r0 = cn_var_count;
  r1 = cn_var_price;
  r2 = r0 * r1;
  cn_var_total = r2;
  double cn_var_avg;
  r3 = cn_var_total;
  r4 = cn_var_count;
  r5 = r3 / r4;
  cn_var_avg = r5;
  return 0;
}

int main() {
  cn_rt_init();
  long long r0, r1, r2, r3, r4, r5, r6;

  entry:
  r0 = cn_func_测试浮点声明();
  r1 = cn_func_测试浮点运算();
  r2 = cn_func_测试混合运算();
  r3 = cn_func_测试浮点比较();
  r4 = cn_func_计算圆面积(5.000000);
  r5 = cn_func_测试浮点数组();
  r6 = cn_func_测试复杂表达式();
  return 0;
  cn_rt_exit();
}

