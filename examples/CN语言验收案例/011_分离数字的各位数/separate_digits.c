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
  long long r0, r1, r3, r4, r6, r7, r8, r9, r10, r11, r12, r14, r15, r16, r17, r18, r19, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32;
  _Bool r2;
  _Bool r5;
  _Bool r13;
  _Bool r20;

  entry:
  long long cn_var_输入数字;
  cn_var_输入数字 = 12345;
  cn_rt_print_string("====================================");
  cn_rt_print_string("    分离数字的各位数程序");
  cn_rt_print_string("====================================");
  cn_rt_print_string("");
  cn_rt_print_string("输入数字: ");
  r0 = cn_var_输入数字;
  cn_rt_print_int(r0);
  cn_rt_print_string("");
  r1 = cn_var_输入数字;
  r2 = r1 == 0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  cn_rt_print_string("该数字只有1位，值为: 0");
  return 0;
  goto if_merge_1;

  if_merge_1:
  long long cn_var_位数;
  cn_var_位数 = 0;
  long long cn_var_临时值;
  r3 = cn_var_输入数字;
  cn_var_临时值 = r3;
  goto while_cond_2;

  while_cond_2:
  r4 = cn_var_临时值;
  r5 = r4 != 0;
  if (r5) goto while_body_3; else goto while_exit_4;

  while_body_3:
  r6 = cn_var_临时值;
  r7 = r6 / 10;
  cn_var_临时值 = r7;
  r8 = cn_var_位数;
  r9 = r8 + 1;
  cn_var_位数 = r9;
  goto while_cond_2;

  while_exit_4:
  cn_rt_print_string("该数字共有 ");
  r10 = cn_var_位数;
  cn_rt_print_int(r10);
  cn_rt_print_string(" 位数字");
  cn_rt_print_string("");
  cn_rt_print_string("从高位到低位分离结果:");
  cn_rt_print_string("-----------------------------------");
  long long cn_var_除数;
  cn_var_除数 = 1;
  long long cn_var_计数器;
  cn_var_计数器 = 1;
  goto while_cond_5;

  while_cond_5:
  r11 = cn_var_计数器;
  r12 = cn_var_位数;
  r13 = r11 < r12;
  if (r13) goto while_body_6; else goto while_exit_7;

  while_body_6:
  r14 = cn_var_除数;
  r15 = r14 * 10;
  cn_var_除数 = r15;
  r16 = cn_var_计数器;
  r17 = r16 + 1;
  cn_var_计数器 = r17;
  goto while_cond_5;

  while_exit_7:
  long long cn_var_当前位置;
  cn_var_当前位置 = 1;
  long long cn_var_剩余数字;
  r18 = cn_var_输入数字;
  cn_var_剩余数字 = r18;
  goto while_cond_8;

  while_cond_8:
  r19 = cn_var_除数;
  r20 = r19 > 0;
  if (r20) goto while_body_9; else goto while_exit_10;

  while_body_9:
  long long cn_var_当前位数字;
  r21 = cn_var_剩余数字;
  r22 = cn_var_除数;
  r23 = r21 / r22;
  cn_var_当前位数字 = r23;
  cn_rt_print_string("第 ");
  r24 = cn_var_当前位置;
  cn_rt_print_int(r24);
  cn_rt_print_string(" 位: ");
  r25 = cn_var_当前位数字;
  cn_rt_print_int(r25);
  r26 = cn_var_剩余数字;
  r27 = cn_var_除数;
  r28 = r26 % r27;
  cn_var_剩余数字 = r28;
  r29 = cn_var_除数;
  r30 = r29 / 10;
  cn_var_除数 = r30;
  r31 = cn_var_当前位置;
  r32 = r31 + 1;
  cn_var_当前位置 = r32;
  goto while_cond_8;

  while_exit_10:
  cn_rt_print_string("");
  cn_rt_print_string("-----------------------------------");
  cn_rt_print_string("分离完成！");
  return 0;
  cn_rt_exit();
}

