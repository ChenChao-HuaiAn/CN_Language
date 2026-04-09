#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元类型枚举_错误 = 81,
    词元类型枚举_结束 = 80,
    词元类型枚举_问号 = 79,
    词元类型枚举_冒号 = 78,
    词元类型枚举_点 = 77,
    词元类型枚举_逗号 = 76,
    词元类型枚举_分号 = 75,
    词元类型枚举_右方括号 = 74,
    词元类型枚举_左方括号 = 73,
    词元类型枚举_右大括号 = 72,
    词元类型枚举_左大括号 = 71,
    词元类型枚举_右括号 = 70,
    词元类型枚举_左括号 = 69,
    词元类型枚举_箭头 = 68,
    词元类型枚举_自减 = 67,
    词元类型枚举_自增 = 66,
    词元类型枚举_右移 = 65,
    词元类型枚举_左移 = 64,
    词元类型枚举_按位取反 = 63,
    词元类型枚举_按位异或 = 62,
    词元类型枚举_按位或 = 61,
    词元类型枚举_按位与 = 60,
    词元类型枚举_逻辑非 = 59,
    词元类型枚举_逻辑或 = 58,
    词元类型枚举_逻辑与 = 57,
    词元类型枚举_大于等于 = 56,
    词元类型枚举_大于 = 55,
    词元类型枚举_小于等于 = 54,
    词元类型枚举_小于 = 53,
    词元类型枚举_不等于 = 52,
    词元类型枚举_赋值 = 51,
    词元类型枚举_等于 = 50,
    词元类型枚举_百分号 = 49,
    词元类型枚举_斜杠 = 48,
    词元类型枚举_星号 = 47,
    词元类型枚举_减号 = 46,
    词元类型枚举_加号 = 45,
    词元类型枚举_字符字面量 = 44,
    词元类型枚举_字符串字面量 = 43,
    词元类型枚举_浮点字面量 = 42,
    词元类型枚举_整数字面量 = 41,
    词元类型枚举_标识符 = 40,
    词元类型枚举_关键字_最终 = 39,
    词元类型枚举_关键字_抛出 = 38,
    词元类型枚举_关键字_捕获 = 37,
    词元类型枚举_关键字_尝试 = 36,
    词元类型枚举_关键字_基类 = 35,
    词元类型枚举_关键字_自身 = 34,
    词元类型枚举_关键字_实现 = 33,
    词元类型枚举_关键字_抽象 = 32,
    词元类型枚举_关键字_重写 = 31,
    词元类型枚举_关键字_虚拟 = 30,
    词元类型枚举_关键字_保护 = 29,
    词元类型枚举_关键字_接口 = 28,
    词元类型枚举_关键字_类 = 27,
    词元类型枚举_关键字_无 = 26,
    词元类型枚举_关键字_假 = 25,
    词元类型枚举_关键字_真 = 24,
    词元类型枚举_关键字_静态 = 23,
    词元类型枚举_关键字_私有 = 22,
    词元类型枚举_关键字_公开 = 21,
    词元类型枚举_关键字_从 = 20,
    词元类型枚举_关键字_导入 = 19,
    词元类型枚举_关键字_变量 = 18,
    词元类型枚举_关键字_函数 = 17,
    词元类型枚举_关键字_枚举 = 16,
    词元类型枚举_关键字_结构体 = 15,
    词元类型枚举_关键字_空类型 = 14,
    词元类型枚举_关键字_布尔 = 13,
    词元类型枚举_关键字_字符串 = 12,
    词元类型枚举_关键字_小数 = 11,
    词元类型枚举_关键字_整数 = 10,
    词元类型枚举_关键字_默认 = 9,
    词元类型枚举_关键字_情况 = 8,
    词元类型枚举_关键字_选择 = 7,
    词元类型枚举_关键字_继续 = 6,
    词元类型枚举_关键字_中断 = 5,
    词元类型枚举_关键字_返回 = 4,
    词元类型枚举_关键字_循环 = 3,
    词元类型枚举_关键字_当 = 2,
    词元类型枚举_关键字_否则 = 1,
    词元类型枚举_关键字_如果 = 0
};
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块

// CN Language Enum Definitions
enum 同步点类型 {
    同步点类型_同步点_分号,
    同步点类型_同步点_右大括号,
    同步点类型_同步点_右小括号,
    同步点类型_同步点_右中括号,
    同步点类型_同步点_逗号,
    同步点类型_同步点_关键字,
    同步点类型_同步点_类型关键字,
    同步点类型_同步点_声明关键字
};

enum 语法恢复策略 {
    语法恢复策略_不恢复,
    语法恢复策略_跳过词元,
    语法恢复策略_同步到分号,
    语法恢复策略_同步到大括号,
    语法恢复策略_同步到关键字
};

// CN Language Global Struct Forward Declarations
struct 语法错误恢复上下文;

// CN Language Global Struct Definitions
struct 语法错误恢复上下文 {
    long long 连续错误计数;
    long long 最大连续错误;
    long long 最后错误行号;
    long long 级联抑制计数;
    _Bool 抑制级联错误;
    long long 恢复尝试次数;
    long long 最大恢复次数;
};

// Global Variables
long long cn_var_默认最大连续错误 = 3;
long long cn_var_默认最大恢复次数 = 10;
long long cn_var_语法错误恢复上下文大小 = 28;

// Forward Declarations
_Bool 初始化错误恢复(struct 语法错误恢复上下文*);
void 销毁错误恢复上下文(struct 语法错误恢复上下文*);
long long 是同步点(enum 词元类型枚举, enum 同步点类型);
long long 是任意同步点(enum 词元类型枚举);
long long 跳转到同步点(struct 语法错误恢复上下文*, enum 词元类型枚举, enum 同步点类型);
_Bool 应该抑制错误(struct 语法错误恢复上下文*);
void 记录错误(struct 语法错误恢复上下文*, long long);
void 重置错误计数(struct 语法错误恢复上下文*);
enum 语法恢复策略 选择恢复策略(long long);
char* 同步点类型名称(enum 同步点类型);
char* 恢复策略名称(enum 语法恢复策略);

_Bool 初始化错误恢复(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r2, r3;
  struct 语法错误恢复上下文* r0;
  _Bool r1;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_默认最大连续错误;
  r3 = cn_var_默认最大恢复次数;
  return 1;
}

void 销毁错误恢复上下文(struct 语法错误恢复上下文* cn_var_上下文) {
  struct 语法错误恢复上下文* r0;
  _Bool r1;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 != 0;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  goto if_merge_3;

  if_merge_3:
  return;
}

long long 是同步点(enum 词元类型枚举 cn_var_词元类型, enum 同步点类型 cn_var_同步类型) {
  long long r19, r24, r29;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r21;
  _Bool r23;
  _Bool r26;
  _Bool r28;
  _Bool r31;
  _Bool r33;
  enum 同步点类型 r0;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r22;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r30;
  enum 词元类型枚举 r32;

  entry:
  r0 = cn_var_同步类型;
  r1 = r0 == 0;
  if (r1) goto case_body_5; else goto switch_check_14;

  switch_check_14:
  r2 = r0 == 1;
  if (r2) goto case_body_6; else goto switch_check_15;

  switch_check_15:
  r3 = r0 == 2;
  if (r3) goto case_body_7; else goto switch_check_16;

  switch_check_16:
  r4 = r0 == 3;
  if (r4) goto case_body_8; else goto switch_check_17;

  switch_check_17:
  r5 = r0 == 4;
  if (r5) goto case_body_9; else goto switch_check_18;

  switch_check_18:
  r6 = r0 == 5;
  if (r6) goto case_body_10; else goto switch_check_19;

  switch_check_19:
  r7 = r0 == 6;
  if (r7) goto case_body_11; else goto switch_check_20;

  switch_check_20:
  r8 = r0 == 7;
  if (r8) goto case_body_12; else goto case_default_13;

  case_body_5:
  r9 = cn_var_词元类型;
  r10 = r9 == 1;
  return r10;
  goto switch_merge_4;

  case_body_6:
  r11 = cn_var_词元类型;
  r12 = r11 == 2;
  return r12;
  goto switch_merge_4;

  case_body_7:
  r13 = cn_var_词元类型;
  r14 = r13 == 3;
  return r14;
  goto switch_merge_4;

  case_body_8:
  r15 = cn_var_词元类型;
  r16 = r15 == 4;
  return r16;
  goto switch_merge_4;

  case_body_9:
  r17 = cn_var_词元类型;
  r18 = r17 == 5;
  return r18;
  goto switch_merge_4;

  case_body_10:
  r20 = cn_var_词元类型;
  r21 = r20 >= 100;
  if (r21) goto logic_rhs_21; else goto logic_merge_22;

  logic_rhs_21:
  r22 = cn_var_词元类型;
  r23 = r22 <= 150;
  goto logic_merge_22;

  logic_merge_22:
  return r23;
  goto switch_merge_4;

  case_body_11:
  r25 = cn_var_词元类型;
  r26 = r25 >= 160;
  if (r26) goto logic_rhs_23; else goto logic_merge_24;

  logic_rhs_23:
  r27 = cn_var_词元类型;
  r28 = r27 <= 170;
  goto logic_merge_24;

  logic_merge_24:
  return r28;
  goto switch_merge_4;

  case_body_12:
  r30 = cn_var_词元类型;
  r31 = r30 >= 180;
  if (r31) goto logic_rhs_25; else goto logic_merge_26;

  logic_rhs_25:
  r32 = cn_var_词元类型;
  r33 = r32 <= 190;
  goto logic_merge_26;

  logic_merge_26:
  return r33;
  goto switch_merge_4;

  case_default_13:
  return 0;
  goto switch_merge_4;

  switch_merge_4:
  return 0;
}

long long 是任意同步点(enum 词元类型枚举 cn_var_词元类型) {
  long long r1, r3, r5, r7, r9, r11, r13, r15;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r14;

  entry:
  r0 = cn_var_词元类型;
  r1 = 是同步点(r0, 0);
  if (r1) goto if_then_27; else goto if_merge_28;

  if_then_27:
  return 1;
  goto if_merge_28;

  if_merge_28:
  r2 = cn_var_词元类型;
  r3 = 是同步点(r2, 1);
  if (r3) goto if_then_29; else goto if_merge_30;

  if_then_29:
  return 1;
  goto if_merge_30;

  if_merge_30:
  r4 = cn_var_词元类型;
  r5 = 是同步点(r4, 2);
  if (r5) goto if_then_31; else goto if_merge_32;

  if_then_31:
  return 1;
  goto if_merge_32;

  if_merge_32:
  r6 = cn_var_词元类型;
  r7 = 是同步点(r6, 3);
  if (r7) goto if_then_33; else goto if_merge_34;

  if_then_33:
  return 1;
  goto if_merge_34;

  if_merge_34:
  r8 = cn_var_词元类型;
  r9 = 是同步点(r8, 4);
  if (r9) goto if_then_35; else goto if_merge_36;

  if_then_35:
  return 1;
  goto if_merge_36;

  if_merge_36:
  r10 = cn_var_词元类型;
  r11 = 是同步点(r10, 5);
  if (r11) goto if_then_37; else goto if_merge_38;

  if_then_37:
  return 1;
  goto if_merge_38;

  if_merge_38:
  r12 = cn_var_词元类型;
  r13 = 是同步点(r12, 6);
  if (r13) goto if_then_39; else goto if_merge_40;

  if_then_39:
  return 1;
  goto if_merge_40;

  if_merge_40:
  r14 = cn_var_词元类型;
  r15 = 是同步点(r14, 7);
  if (r15) goto if_then_41; else goto if_merge_42;

  if_then_41:
  return 1;
  goto if_merge_42;

  if_merge_42:
  return 0;
}

long long 跳转到同步点(struct 语法错误恢复上下文* cn_var_上下文, enum 词元类型枚举 cn_var_当前词元类型, enum 同步点类型 cn_var_目标同步类型) {
  long long r3, r4, r6, r8, r12, r16;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r5;
  struct 语法错误恢复上下文* r7;
  _Bool r1;
  _Bool r9;
  _Bool r14;
  enum 词元类型枚举 r10;
  enum 同步点类型 r11;
  enum 同步点类型 r13;
  enum 词元类型枚举 r15;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_43; else goto if_merge_44;

  if_then_43:
  return 0;
  goto if_merge_44;

  if_merge_44:
  r2 = cn_var_上下文;
  r3 = r2->恢复尝试次数;
  r4 = r3 + 1;
  r5 = cn_var_上下文;
  r6 = r5->恢复尝试次数;
  r7 = cn_var_上下文;
  r8 = r7->最大恢复次数;
  r9 = r6 > r8;
  if (r9) goto if_then_45; else goto if_merge_46;

  if_then_45:
  return 0;
  goto if_merge_46;

  if_merge_46:
  r10 = cn_var_当前词元类型;
  r11 = cn_var_目标同步类型;
  r12 = 是同步点(r10, r11);
  if (r12) goto if_then_47; else goto if_merge_48;

  if_then_47:
  return 1;
  goto if_merge_48;

  if_merge_48:
  r13 = cn_var_目标同步类型;
  r14 = r13 == 0;
  if (r14) goto if_then_49; else goto if_merge_50;

  if_then_49:
  r15 = cn_var_当前词元类型;
  r16 = 是任意同步点(r15);
  if (r16) goto if_then_51; else goto if_merge_52;

  if_merge_50:
  return 0;

  if_then_51:
  return 1;
  goto if_merge_52;

  if_merge_52:
  goto if_merge_50;
  return 0;
}

_Bool 应该抑制错误(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r5, r7;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r4;
  struct 语法错误恢复上下文* r6;
  _Bool r1;
  _Bool r3;
  _Bool r8;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_53; else goto if_merge_54;

  if_then_53:
  return 0;
  goto if_merge_54;

  if_merge_54:
  r2 = cn_var_上下文;
  r3 = r2->抑制级联错误;
  if (r3) goto if_then_55; else goto if_merge_56;

  if_then_55:
  return 1;
  goto if_merge_56;

  if_merge_56:
  r4 = cn_var_上下文;
  r5 = r4->连续错误计数;
  r6 = cn_var_上下文;
  r7 = r6->最大连续错误;
  r8 = r5 >= r7;
  if (r8) goto if_then_57; else goto if_merge_58;

  if_then_57:
  return 1;
  goto if_merge_58;

  if_merge_58:
  return 0;
}

void 记录错误(struct 语法错误恢复上下文* cn_var_上下文, long long cn_var_行号) {
  long long r3, r4, r7, r8, r10, r12, r15, r16, r17;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r6;
  struct 语法错误恢复上下文* r9;
  struct 语法错误恢复上下文* r11;
  struct 语法错误恢复上下文* r14;
  _Bool r1;
  _Bool r5;
  _Bool r13;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_59; else goto if_merge_60;

  if_then_59:
  return;
  goto if_merge_60;

  if_merge_60:
  r2 = cn_var_上下文;
  r3 = r2->最后错误行号;
  r4 = cn_var_行号;
  r5 = r3 == r4;
  if (r5) goto if_then_61; else goto if_else_62;

  if_then_61:
  r6 = cn_var_上下文;
  r7 = r6->级联抑制计数;
  r8 = r7 + 1;
  r9 = cn_var_上下文;
  r10 = r9->级联抑制计数;
  r11 = cn_var_上下文;
  r12 = r11->最大连续错误;
  r13 = r10 >= r12;
  if (r13) goto if_then_64; else goto if_merge_65;

  if_else_62:
  goto if_merge_63;

  if_merge_63:
  r14 = cn_var_上下文;
  r15 = r14->连续错误计数;
  r16 = r15 + 1;
  r17 = cn_var_行号;

  if_then_64:
  goto if_merge_65;

  if_merge_65:
  goto if_merge_63;
  return;
}

void 重置错误计数(struct 语法错误恢复上下文* cn_var_上下文) {
  struct 语法错误恢复上下文* r0;
  _Bool r1;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_66; else goto if_merge_67;

  if_then_66:
  return;
  goto if_merge_67;

  if_merge_67:
  return;
}

enum 语法恢复策略 选择恢复策略(long long cn_var_错误码) {
  long long r0;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;

  entry:
  r0 = cn_var_错误码;
  r1 = r0 == 101;
  if (r1) goto case_body_69; else goto switch_check_82;

  switch_check_82:
  r2 = r0 == 102;
  if (r2) goto case_body_70; else goto switch_check_83;

  switch_check_83:
  r3 = r0 == 103;
  if (r3) goto case_body_71; else goto switch_check_84;

  switch_check_84:
  r4 = r0 == 104;
  if (r4) goto case_body_72; else goto switch_check_85;

  switch_check_85:
  r5 = r0 == 105;
  if (r5) goto case_body_73; else goto switch_check_86;

  switch_check_86:
  r6 = r0 == 106;
  if (r6) goto case_body_74; else goto switch_check_87;

  switch_check_87:
  r7 = r0 == 107;
  if (r7) goto case_body_75; else goto switch_check_88;

  switch_check_88:
  r8 = r0 == 108;
  if (r8) goto case_body_76; else goto switch_check_89;

  switch_check_89:
  r9 = r0 == 109;
  if (r9) goto case_body_77; else goto switch_check_90;

  switch_check_90:
  r10 = r0 == 1;
  if (r10) goto case_body_78; else goto switch_check_91;

  switch_check_91:
  r11 = r0 == 2;
  if (r11) goto case_body_79; else goto switch_check_92;

  switch_check_92:
  r12 = r0 == 3;
  if (r12) goto case_body_80; else goto case_default_81;

  case_body_69:
  goto switch_merge_68;

  case_body_70:
  return 2;
  goto switch_merge_68;

  case_body_71:
  goto switch_merge_68;

  case_body_72:
  return 3;
  goto switch_merge_68;

  case_body_73:
  goto switch_merge_68;

  case_body_74:
  goto switch_merge_68;

  case_body_75:
  return 4;
  goto switch_merge_68;

  case_body_76:
  goto switch_merge_68;

  case_body_77:
  return 1;
  goto switch_merge_68;

  case_body_78:
  goto switch_merge_68;

  case_body_79:
  goto switch_merge_68;

  case_body_80:
  return 1;
  goto switch_merge_68;

  case_default_81:
  return 2;
  goto switch_merge_68;

  switch_merge_68:
  return 0;
}

char* 同步点类型名称(enum 同步点类型 cn_var_同步类型) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  enum 同步点类型 r0;

  entry:
  r0 = cn_var_同步类型;
  r1 = r0 == 0;
  if (r1) goto case_body_94; else goto switch_check_103;

  switch_check_103:
  r2 = r0 == 1;
  if (r2) goto case_body_95; else goto switch_check_104;

  switch_check_104:
  r3 = r0 == 2;
  if (r3) goto case_body_96; else goto switch_check_105;

  switch_check_105:
  r4 = r0 == 3;
  if (r4) goto case_body_97; else goto switch_check_106;

  switch_check_106:
  r5 = r0 == 4;
  if (r5) goto case_body_98; else goto switch_check_107;

  switch_check_107:
  r6 = r0 == 5;
  if (r6) goto case_body_99; else goto switch_check_108;

  switch_check_108:
  r7 = r0 == 6;
  if (r7) goto case_body_100; else goto switch_check_109;

  switch_check_109:
  r8 = r0 == 7;
  if (r8) goto case_body_101; else goto case_default_102;

  case_body_94:
  return "分号";
  goto switch_merge_93;

  case_body_95:
  return "右大括号";
  goto switch_merge_93;

  case_body_96:
  return "右小括号";
  goto switch_merge_93;

  case_body_97:
  return "右中括号";
  goto switch_merge_93;

  case_body_98:
  return "逗号";
  goto switch_merge_93;

  case_body_99:
  return "关键字";
  goto switch_merge_93;

  case_body_100:
  return "类型关键字";
  goto switch_merge_93;

  case_body_101:
  return "声明关键字";
  goto switch_merge_93;

  case_default_102:
  return "未知同步点";
  goto switch_merge_93;

  switch_merge_93:
  return NULL;
}

char* 恢复策略名称(enum 语法恢复策略 cn_var_策略) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  enum 语法恢复策略 r0;

  entry:
  r0 = cn_var_策略;
  r1 = r0 == 0;
  if (r1) goto case_body_111; else goto switch_check_117;

  switch_check_117:
  r2 = r0 == 1;
  if (r2) goto case_body_112; else goto switch_check_118;

  switch_check_118:
  r3 = r0 == 2;
  if (r3) goto case_body_113; else goto switch_check_119;

  switch_check_119:
  r4 = r0 == 3;
  if (r4) goto case_body_114; else goto switch_check_120;

  switch_check_120:
  r5 = r0 == 4;
  if (r5) goto case_body_115; else goto case_default_116;

  case_body_111:
  return "不恢复";
  goto switch_merge_110;

  case_body_112:
  return "跳过词元";
  goto switch_merge_110;

  case_body_113:
  return "同步到分号";
  goto switch_merge_110;

  case_body_114:
  return "同步到大括号";
  goto switch_merge_110;

  case_body_115:
  return "同步到关键字";
  goto switch_merge_110;

  case_default_116:
  return "未知策略";
  goto switch_merge_110;

  switch_merge_110:
  return NULL;
}

