#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
int main();

int main() {
  cn_rt_init();
  long long r2, r4, r6, r10, r12, r20, r21, r22, r24, r26, r27, r28, r29, r30, r32, r34, r36, r37, r38, r39, r41, r43, r44, r46, r47, r48, r51, r53, r54, r55, r56, r58, r59, r60;
  char* r15;
  char* r18;
  void* r0;
  void* r1;
  void* r3;
  void* r5;
  void* r7;
  void* r8;
  void* r9;
  void* r11;
  void* r13;
  void* r14;
  void* r16;
  void* r17;
  void* r19;
  void* r25;
  void* r31;
  void* r35;
  void* r40;
  void* r42;
  void* r45;
  void* r49;
  void* r50;
  void* r52;
  void* r57;
  _Bool r23;
  _Bool r33;

  entry:
  cn_rt_print_string("开始数组功能完整测试\n");
  cn_rt_print_string("\n=== 测试1: 基本数组功能 ===\n");
  void* cn_var_arr;
  r0 = cn_rt_array_alloc(8, 5);
  cn_rt_array_set_element(r0, 0, ({long long _tmp_0 = 1; &_tmp_0;}), 8);
  cn_rt_array_set_element(r0, 1, ({long long _tmp_1 = 2; &_tmp_1;}), 8);
  cn_rt_array_set_element(r0, 2, ({long long _tmp_2 = 3; &_tmp_2;}), 8);
  cn_rt_array_set_element(r0, 3, ({long long _tmp_3 = 4; &_tmp_3;}), 8);
  cn_rt_array_set_element(r0, 4, ({long long _tmp_4 = 5; &_tmp_4;}), 8);
  cn_var_arr = r0;
  cn_rt_print_string("数组创建完成\n");
  cn_rt_print_string("数组长度: ");
  r1 = cn_var_arr;
  r2 = cn_rt_array_length(r1);
  cn_rt_print_int(r2);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 测试2: 数组访问和修改 ===\n");
  cn_rt_print_string("arr[0] = ");
  r3 = cn_var_arr;
  r4 = *(int*)cn_rt_array_get_element(r3, 0, 8);
  cn_rt_print_int(r4);
  cn_rt_print_string("\n");
  cn_rt_print_string("arr[2] = ");
  r5 = cn_var_arr;
  r6 = *(int*)cn_rt_array_get_element(r5, 2, 8);
  cn_rt_print_int(r6);
  cn_rt_print_string("\n");
  r7 = cn_var_arr;
  cn_rt_array_set_element(r7, 0, ({long long _tmp_5 = 100; &_tmp_5;}), 8);
  r8 = cn_var_arr;
  cn_rt_array_set_element(r8, 2, ({long long _tmp_6 = 300; &_tmp_6;}), 8);
  cn_rt_print_string("修改后 arr[0] = ");
  r9 = cn_var_arr;
  r10 = *(int*)cn_rt_array_get_element(r9, 0, 8);
  cn_rt_print_int(r10);
  cn_rt_print_string("\n");
  cn_rt_print_string("修改后 arr[2] = ");
  r11 = cn_var_arr;
  r12 = *(int*)cn_rt_array_get_element(r11, 2, 8);
  cn_rt_print_int(r12);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 测试3: 字符串数组 ===\n");
  void* cn_var_strArr;
  r13 = cn_rt_array_alloc(8, 3);
  cn_rt_array_set_element(r13, 0, &"一", 8);
  cn_rt_array_set_element(r13, 1, &"二", 8);
  cn_rt_array_set_element(r13, 2, &"三", 8);
  cn_var_strArr = r13;
  cn_rt_print_string("字符串数组创建完成\n");
  cn_rt_print_string("strArr[1] = ");
  r14 = cn_var_strArr;
  r15 = *(char**)cn_rt_array_get_element(r14, 1, 8);
  cn_rt_print_string(r15);
  cn_rt_print_string("\n");
  r16 = cn_var_strArr;
  cn_rt_array_set_element(r16, 1, &"贰", 8);
  cn_rt_print_string("修改后 strArr[1] = ");
  r17 = cn_var_strArr;
  r18 = *(char**)cn_rt_array_get_element(r17, 1, 8);
  cn_rt_print_string(r18);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 测试4: 循环遍历数组while ===\n");
  cn_rt_print_string("遍历arr数组:\n");
  int cn_var_i;
  cn_var_i = 0;
  int cn_var_arr_len;
  r19 = cn_var_arr;
  r20 = cn_rt_array_length(r19);
  cn_var_arr_len = r20;
  goto while_cond_0;

  while_cond_0:
  r21 = cn_var_i;
  r22 = cn_var_arr_len;
  r23 = r21 < r22;
  if (r23) goto while_body_1; else goto while_exit_2;

  while_body_1:
  cn_rt_print_string("  arr[");
  r24 = cn_var_i;
  cn_rt_print_int(r24);
  cn_rt_print_string("] = ");
  r25 = cn_var_arr;
  r26 = cn_var_i;
  r27 = *(int*)cn_rt_array_get_element(r25, r26, 8);
  cn_rt_print_int(r27);
  cn_rt_print_string("\n");
  r28 = cn_var_i;
  r29 = r28 + 1;
  cn_var_i = r29;
  goto while_cond_0;

  while_exit_2:
  cn_rt_print_string("\n=== 测试5: 循环遍历数组for ===\n");
  cn_rt_print_string("遍历arr数组:\n");
  int cn_var_j;
  cn_var_j = 0;
  goto for_cond_3;

  for_cond_3:
  r30 = cn_var_j;
  r31 = cn_var_arr;
  r32 = cn_rt_array_length(r31);
  r33 = r30 < r32;
  if (r33) goto for_body_4; else goto for_exit_6;

  for_body_4:
  cn_rt_print_string("  arr[");
  r34 = cn_var_j;
  cn_rt_print_int(r34);
  cn_rt_print_string("] = ");
  r35 = cn_var_arr;
  r36 = cn_var_j;
  r37 = *(int*)cn_rt_array_get_element(r35, r36, 8);
  cn_rt_print_int(r37);
  cn_rt_print_string("\n");
  goto for_update_5;

  for_update_5:
  r38 = cn_var_j;
  r39 = r38 + 1;
  cn_var_j = r39;
  goto for_cond_3;

  for_exit_6:
  cn_rt_print_string("\n=== 测试6: 数组在表达式中的使用 ===\n");
  int cn_var_和;
  r40 = cn_var_arr;
  r41 = *(int*)cn_rt_array_get_element(r40, 0, 8);
  r42 = cn_var_arr;
  r43 = *(int*)cn_rt_array_get_element(r42, 1, 8);
  r44 = r41 + r43;
  r45 = cn_var_arr;
  r46 = *(int*)cn_rt_array_get_element(r45, 2, 8);
  r47 = r44 + r46;
  cn_var_和 = r47;
  cn_rt_print_string("arr前三个元素的和 = ");
  r48 = cn_var_和;
  cn_rt_print_int(r48);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 测试7: 空数组 ===\n");
  void* cn_var_空数组;
  r49 = cn_rt_array_alloc(8, 0);
  cn_var_空数组 = r49;
  cn_rt_print_string("空数组创建完成\n");
  cn_rt_print_string("空数组长度: ");
  r50 = cn_var_空数组;
  r51 = cn_rt_array_length(r50);
  cn_rt_print_int(r51);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 测试8: 边界检查 ===\n");
  int cn_var_len;
  r52 = cn_var_arr;
  r53 = cn_rt_array_length(r52);
  cn_var_len = r53;
  cn_rt_print_string("数组长度: ");
  r54 = cn_var_len;
  cn_rt_print_int(r54);
  cn_rt_print_string("\n");
  cn_rt_print_string("最后一个元素: arr[");
  r55 = cn_var_len;
  r56 = r55 - 1;
  cn_rt_print_int(r56);
  cn_rt_print_string("] = ");
  r57 = cn_var_arr;
  r58 = cn_var_len;
  r59 = r58 - 1;
  r60 = *(int*)cn_rt_array_get_element(r57, r59, 8);
  cn_rt_print_int(r60);
  cn_rt_print_string("\n");
  cn_rt_print_string("\n=== 所有数组功能测试完成 ===\n");
  cn_rt_print_string("数组功能齐全，正常工作！\n");
  return 0;
  cn_rt_exit();
}

