#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型 {
    词元类型_标识符,
    词元类型_整数字面量,
    词元类型_加号,
    词元类型_减号,
    词元类型_分号,
    词元类型_结束,
    词元类型_错误
};

// CN Language Global Struct Forward Declarations
struct 词元;
struct 扫描器;

// CN Language Global Struct Definitions
struct 词元 {
    enum 词元类型 类型;
    char* 值;
    long long 行号;
    long long 列号;
};

struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 长度;
};

// Global Variables

// Forward Declarations
struct 扫描器* 创建扫描器(char*, long long);
void 销毁扫描器(struct 扫描器*);
long long 当前字符(struct 扫描器*);
long long 前进(struct 扫描器*);
_Bool 是数字(long long);
_Bool 是字母(long long);
_Bool 是标识符开头(long long);
_Bool 是空白字符(long long);
void 跳过空白字符(struct 扫描器*);
struct 词元 下一个词元(struct 扫描器*);

struct 扫描器* 创建扫描器(char* cn_var_源码内容, long long cn_var_源码长度) {
  long long r4;
  char* r3;
  void* r0;
  struct 扫描器* r1;
  struct 扫描器* r5;
  _Bool r2;

  entry:
  struct 扫描器* cn_var_实例_0;
  r0 = 分配内存(24);
  cn_var_实例_0 = r0;
  r1 = cn_var_实例_0;
  r2 = r1 == 0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  r3 = cn_var_源码内容;
  r4 = cn_var_源码长度;
  r5 = cn_var_实例_0;
  return r5;
}

void 销毁扫描器(struct 扫描器* cn_var_实例) {
  struct 扫描器* r0;
  struct 扫描器* r2;
  _Bool r1;

  entry:
  r0 = cn_var_实例;
  r1 = r0 != 0;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_3;

  if_merge_3:
  return;
}

long long 当前字符(struct 扫描器* cn_var_实例) {
  long long r0, r5, r8, r15;
  char* r12;
  struct 扫描器* r1;
  struct 扫描器* r3;
  struct 扫描器* r6;
  struct 扫描器* r10;
  struct 扫描器* r13;
  _Bool r2;
  _Bool r9;
  char r16;
  struct 扫描器 r4;
  struct 扫描器 r7;
  struct 扫描器 r11;
  struct 扫描器 r14;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_7; else goto logic_rhs_6;

  if_then_4:
  return 0;
  goto if_merge_5;

  if_merge_5:
  r10 = cn_var_实例;
  r11 = *r10;
  r12 = r11.源码;
  r13 = cn_var_实例;
  r14 = *r13;
  r15 = r14.位置;
  r16 = *(char*)cn_rt_array_get_element(r12, r15, 8);
  return r16;

  logic_rhs_6:
  r3 = cn_var_实例;
  r4 = *r3;
  r5 = r4.位置;
  r6 = cn_var_实例;
  r7 = *r6;
  r8 = r7.长度;
  r9 = r5 >= r8;
  goto logic_merge_7;

  logic_merge_7:
  if (r9) goto if_then_4; else goto if_merge_5;
  return 0;
}

long long 前进(struct 扫描器* cn_var_实例) {
  long long r1, r2, r6, r7, r8, r12, r13, r16, r17, r18;
  struct 扫描器* r0;
  struct 扫描器* r4;
  struct 扫描器* r10;
  struct 扫描器* r14;
  _Bool r3;
  _Bool r9;
  struct 扫描器 r5;
  struct 扫描器 r11;
  struct 扫描器 r15;

  entry:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = r2 != 0;
  if (r3) goto if_then_8; else goto if_merge_9;

  if_then_8:
  r4 = cn_var_实例;
  r5 = *r4;
  r6 = r5.位置;
  r7 = r6 + 1;
  r8 = cn_var_c_0;
  r9 = r8 == 10;
  if (r9) goto if_then_10; else goto if_else_11;

  if_merge_9:
  r18 = cn_var_c_0;
  return r18;

  if_then_10:
  r10 = cn_var_实例;
  r11 = *r10;
  r12 = r11.行号;
  r13 = r12 + 1;
  goto if_merge_12;

  if_else_11:
  r14 = cn_var_实例;
  r15 = *r14;
  r16 = r15.列号;
  r17 = r16 + 1;
  goto if_merge_12;

  if_merge_12:
  goto if_merge_9;
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_13; else goto logic_merge_14;

  logic_rhs_13:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_14;

  logic_merge_14:
  return r4;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r4, r6, r7, r9;
  _Bool r3;
  _Bool r5;
  _Bool r8;
  _Bool r10;

  entry:
  r2 = cn_var_c;
  r3 = r2 >= 97;
  if (r3) goto logic_rhs_17; else goto logic_merge_18;

  logic_rhs_15:
  r7 = cn_var_c;
  r8 = r7 >= 65;
  if (r8) goto logic_rhs_19; else goto logic_merge_20;

  logic_merge_16:
  return r10;

  logic_rhs_17:
  r4 = cn_var_c;
  r5 = r4 <= 122;
  goto logic_merge_18;

  logic_merge_18:
  if (r5) goto logic_merge_16; else goto logic_rhs_15;

  logic_rhs_19:
  r9 = cn_var_c;
  r10 = r9 <= 90;
  goto logic_merge_20;

  logic_merge_20:
  goto logic_merge_16;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r6;
  _Bool r3;
  _Bool r5;
  _Bool r7;

  entry:
  r2 = cn_var_c;
  r3 = r2 == 95;
  if (r3) goto logic_merge_24; else goto logic_rhs_23;

  logic_rhs_21:
  r6 = cn_var_c;
  r7 = r6 >= 128;
  goto logic_merge_22;

  logic_merge_22:
  return r7;

  logic_rhs_23:
  r4 = cn_var_c;
  r5 = 是字母(r4);
  goto logic_merge_24;

  logic_merge_24:
  if (r5) goto logic_merge_22; else goto logic_rhs_21;
  return 0;
}

_Bool 是空白字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r5, r7, r9;
  _Bool r4;
  _Bool r6;
  _Bool r8;
  _Bool r10;

  entry:
  r3 = cn_var_c;
  r4 = r3 == 32;
  if (r4) goto logic_merge_30; else goto logic_rhs_29;

  logic_rhs_25:
  r9 = cn_var_c;
  r10 = r9 == 13;
  goto logic_merge_26;

  logic_merge_26:
  return r10;

  logic_rhs_27:
  r7 = cn_var_c;
  r8 = r7 == 10;
  goto logic_merge_28;

  logic_merge_28:
  if (r8) goto logic_merge_26; else goto logic_rhs_25;

  logic_rhs_29:
  r5 = cn_var_c;
  r6 = r5 == 9;
  goto logic_merge_30;

  logic_merge_30:
  if (r6) goto logic_merge_28; else goto logic_rhs_27;
  return 0;
}

void 跳过空白字符(struct 扫描器* cn_var_实例) {
  long long r1, r4;
  struct 扫描器* r0;
  struct 扫描器* r3;
  _Bool r2;

  entry:
  goto while_cond_31;

  while_cond_31:
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  r2 = 是空白字符(r1);
  if (r2) goto while_body_32; else goto while_exit_33;

  while_body_32:
  r3 = cn_var_实例;
  r4 = 前进(r3);
  goto while_cond_31;

  while_exit_33:
  return;
}

struct 词元 下一个词元(struct 扫描器* cn_var_实例) {
  long long r6, r9, r11, r12, r14, r15, r17, r20, r23, r24, r25, r27, r29, r31, r34, r37, r38, r39, r41, r44, r45, r46, r48, r51, r52, r53, r55, r58, r59, r60, r63, r64, r65;
  struct 扫描器* r0;
  struct 扫描器* r3;
  struct 扫描器* r4;
  struct 扫描器* r7;
  struct 扫描器* r10;
  struct 扫描器* r19;
  struct 扫描器* r22;
  struct 扫描器* r30;
  struct 扫描器* r33;
  struct 扫描器* r36;
  struct 扫描器* r43;
  struct 扫描器* r50;
  struct 扫描器* r57;
  struct 扫描器* r62;
  _Bool r1;
  _Bool r13;
  _Bool r18;
  _Bool r21;
  _Bool r28;
  _Bool r32;
  _Bool r35;
  _Bool r42;
  _Bool r49;
  _Bool r56;
  struct 词元 r2;
  struct 扫描器 r5;
  struct 扫描器 r8;
  struct 词元 r16;
  struct 词元 r26;
  struct 词元 r40;
  struct 词元 r47;
  struct 词元 r54;
  struct 词元 r61;
  struct 词元 r66;

  entry:
  struct 词元 cn_var_结果_0;
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_34; else goto if_merge_35;

  if_then_34:
  r2 = cn_var_结果_0;
  return r2;
  goto if_merge_35;

  if_merge_35:
  r3 = cn_var_实例;
  跳过空白字符(r3);
  long long cn_var_起始行号_1;
  r4 = cn_var_实例;
  r5 = *r4;
  r6 = r5.行号;
  cn_var_起始行号_1 = r6;
  long long cn_var_起始列号_2;
  r7 = cn_var_实例;
  r8 = *r7;
  r9 = r8.列号;
  cn_var_起始列号_2 = r9;
  long long cn_var_c_3;
  r10 = cn_var_实例;
  r11 = 当前字符(r10);
  cn_var_c_3 = r11;
  r12 = cn_var_c_3;
  r13 = r12 == 0;
  if (r13) goto if_then_36; else goto if_merge_37;

  if_then_36:
  r14 = cn_var_起始行号_1;
  r15 = cn_var_起始列号_2;
  r16 = cn_var_结果_0;
  return r16;
  goto if_merge_37;

  if_merge_37:
  r17 = cn_var_c_3;
  r18 = 是数字(r17);
  if (r18) goto if_then_38; else goto if_merge_39;

  if_then_38:
  goto while_cond_40;

  if_merge_39:
  r27 = cn_var_c_3;
  r28 = 是标识符开头(r27);
  if (r28) goto if_then_43; else goto if_merge_44;

  while_cond_40:
  r19 = cn_var_实例;
  r20 = 当前字符(r19);
  r21 = 是数字(r20);
  if (r21) goto while_body_41; else goto while_exit_42;

  while_body_41:
  r22 = cn_var_实例;
  r23 = 前进(r22);
  goto while_cond_40;

  while_exit_42:
  r24 = cn_var_起始行号_1;
  r25 = cn_var_起始列号_2;
  r26 = cn_var_结果_0;
  return r26;
  goto if_merge_39;

  if_then_43:
  goto while_cond_45;

  if_merge_44:
  r41 = cn_var_c_3;
  r42 = r41 == 43;
  if (r42) goto if_then_50; else goto if_merge_51;

  while_cond_45:
  r30 = cn_var_实例;
  r31 = 当前字符(r30);
  r32 = 是标识符开头(r31);
  if (r32) goto logic_merge_49; else goto logic_rhs_48;

  while_body_46:
  r36 = cn_var_实例;
  r37 = 前进(r36);
  goto while_cond_45;

  while_exit_47:
  r38 = cn_var_起始行号_1;
  r39 = cn_var_起始列号_2;
  r40 = cn_var_结果_0;
  return r40;
  goto if_merge_44;

  logic_rhs_48:
  r33 = cn_var_实例;
  r34 = 当前字符(r33);
  r35 = 是数字(r34);
  goto logic_merge_49;

  logic_merge_49:
  if (r35) goto while_body_46; else goto while_exit_47;

  if_then_50:
  r43 = cn_var_实例;
  r44 = 前进(r43);
  r45 = cn_var_起始行号_1;
  r46 = cn_var_起始列号_2;
  r47 = cn_var_结果_0;
  return r47;
  goto if_merge_51;

  if_merge_51:
  r48 = cn_var_c_3;
  r49 = r48 == 45;
  if (r49) goto if_then_52; else goto if_merge_53;

  if_then_52:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  r52 = cn_var_起始行号_1;
  r53 = cn_var_起始列号_2;
  r54 = cn_var_结果_0;
  return r54;
  goto if_merge_53;

  if_merge_53:
  r55 = cn_var_c_3;
  r56 = r55 == 59;
  if (r56) goto if_then_54; else goto if_merge_55;

  if_then_54:
  r57 = cn_var_实例;
  r58 = 前进(r57);
  r59 = cn_var_起始行号_1;
  r60 = cn_var_起始列号_2;
  r61 = cn_var_结果_0;
  return r61;
  goto if_merge_55;

  if_merge_55:
  r62 = cn_var_实例;
  r63 = 前进(r62);
  r64 = cn_var_起始行号_1;
  r65 = cn_var_起始列号_2;
  r66 = cn_var_结果_0;
  return r66;
}

