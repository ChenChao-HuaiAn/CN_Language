#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元类型枚举_关键字_如果 = 0,
    词元类型枚举_关键字_否则 = 1,
    词元类型枚举_关键字_当 = 2,
    词元类型枚举_关键字_循环 = 3,
    词元类型枚举_关键字_返回 = 4,
    词元类型枚举_关键字_中断 = 5,
    词元类型枚举_关键字_继续 = 6,
    词元类型枚举_关键字_选择 = 7,
    词元类型枚举_关键字_情况 = 8,
    词元类型枚举_关键字_默认 = 9,
    词元类型枚举_关键字_整数 = 10,
    词元类型枚举_关键字_小数 = 11,
    词元类型枚举_关键字_字符串 = 12,
    词元类型枚举_关键字_布尔 = 13,
    词元类型枚举_关键字_空类型 = 14,
    词元类型枚举_关键字_结构体 = 15,
    词元类型枚举_关键字_枚举 = 16,
    词元类型枚举_关键字_函数 = 17,
    词元类型枚举_关键字_变量 = 18,
    词元类型枚举_关键字_导入 = 19,
    词元类型枚举_关键字_从 = 20,
    词元类型枚举_关键字_公开 = 21,
    词元类型枚举_关键字_私有 = 22,
    词元类型枚举_关键字_静态 = 23,
    词元类型枚举_关键字_真 = 24,
    词元类型枚举_关键字_假 = 25,
    词元类型枚举_关键字_无 = 26,
    词元类型枚举_关键字_类 = 27,
    词元类型枚举_关键字_接口 = 28,
    词元类型枚举_关键字_保护 = 29,
    词元类型枚举_关键字_虚拟 = 30,
    词元类型枚举_关键字_重写 = 31,
    词元类型枚举_关键字_抽象 = 32,
    词元类型枚举_关键字_实现 = 33,
    词元类型枚举_关键字_自身 = 34,
    词元类型枚举_关键字_基类 = 35,
    词元类型枚举_关键字_尝试 = 36,
    词元类型枚举_关键字_捕获 = 37,
    词元类型枚举_关键字_抛出 = 38,
    词元类型枚举_关键字_最终 = 39,
    词元类型枚举_标识符 = 40,
    词元类型枚举_整数字面量 = 41,
    词元类型枚举_浮点字面量 = 42,
    词元类型枚举_字符串字面量 = 43,
    词元类型枚举_字符字面量 = 44,
    词元类型枚举_加号 = 45,
    词元类型枚举_减号 = 46,
    词元类型枚举_星号 = 47,
    词元类型枚举_斜杠 = 48,
    词元类型枚举_百分号 = 49,
    词元类型枚举_等于 = 50,
    词元类型枚举_赋值 = 51,
    词元类型枚举_不等于 = 52,
    词元类型枚举_小于 = 53,
    词元类型枚举_小于等于 = 54,
    词元类型枚举_大于 = 55,
    词元类型枚举_大于等于 = 56,
    词元类型枚举_逻辑与 = 57,
    词元类型枚举_逻辑或 = 58,
    词元类型枚举_逻辑非 = 59,
    词元类型枚举_按位与 = 60,
    词元类型枚举_按位或 = 61,
    词元类型枚举_按位异或 = 62,
    词元类型枚举_按位取反 = 63,
    词元类型枚举_左移 = 64,
    词元类型枚举_右移 = 65,
    词元类型枚举_自增 = 66,
    词元类型枚举_自减 = 67,
    词元类型枚举_箭头 = 68,
    词元类型枚举_左括号 = 69,
    词元类型枚举_右括号 = 70,
    词元类型枚举_左大括号 = 71,
    词元类型枚举_右大括号 = 72,
    词元类型枚举_左方括号 = 73,
    词元类型枚举_右方括号 = 74,
    词元类型枚举_分号 = 75,
    词元类型枚举_逗号 = 76,
    词元类型枚举_点 = 77,
    词元类型枚举_冒号 = 78,
    词元类型枚举_问号 = 79,
    词元类型枚举_结束 = 80,
    词元类型枚举_错误 = 81
};
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};
struct 扫描器;
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};

// Struct Definitions - 从导入模块
struct 关键字条目;
struct 关键字条目 {
    char* 名称;
    long long 类型;
};

// Forward Declarations - 从导入模块

// CN Language Enum Definitions
enum 词法错误类型 {
    词法错误类型_非法字符,
    词法错误类型_无效字符编码,
    词法错误类型_未闭合字符串,
    词法错误类型_字符串过长,
    词法错误类型_无效转义序列,
    词法错误类型_未闭合字符,
    词法错误类型_空字符字面量,
    词法错误类型_字符字面量过长,
    词法错误类型_未闭合块注释,
    词法错误类型_数字格式错误,
    词法错误类型_无效十六进制,
    词法错误类型_无效二进制,
    词法错误类型_无效八进制,
    词法错误类型_整数溢出,
    词法错误类型_浮点数溢出,
    词法错误类型_浮点数格式错误
};

enum 恢复策略 {
    恢复策略_无恢复,
    恢复策略_跳过字符,
    恢复策略_跳过到行尾,
    恢复策略_跳过到同步点,
    恢复策略_插入词元,
    恢复策略_跳过词元
};

// CN Language Global Struct Forward Declarations
struct 错误恢复上下文;

// CN Language Global Struct Definitions
struct 错误恢复上下文 {
    long long 连续错误计数;
    long long 最大连续错误;
    long long 最后错误行号;
    long long 级联抑制计数;
    _Bool 抑制级联错误;
    enum 词元类型枚举 期望词元;
};

// Global Variables
long long cn_var_默认最大连续错误 = 10;
long long cn_var_级联错误阈值 = 3;
long long cn_var_错误恢复上下文大小 = 24;

// Forward Declarations
void 初始化错误恢复(struct 错误恢复上下文*);
struct 词元 报告词法错误(struct 扫描器*, enum 词法错误类型, const char*);
enum 恢复策略 选择恢复策略(struct 扫描器*, enum 词法错误类型);
void 执行恢复(struct 扫描器*, enum 恢复策略);
_Bool 是否同步点(enum 词元类型枚举);
_Bool 应该抑制错误(struct 错误恢复上下文*);
void 重置错误计数(struct 错误恢复上下文*);
char* 词法错误名称(enum 词法错误类型);
char* 恢复策略名称(enum 恢复策略);
_Bool 是空白字符(long long);
_Bool 是分隔符(long long);
void 更新错误上下文(struct 错误恢复上下文*, long long);
_Bool 应该停止编译(struct 错误恢复上下文*);

// Extern Declarations - 跨模块调用的函数
extern char* 字符串格式();
extern long long 前进();
extern long long 当前字符();
extern _Bool 是否结束();
extern void* 下一个词元();

void 初始化错误恢复(struct 错误恢复上下文* cn_var_上下文) {
  long long r2;
  struct 错误恢复上下文* r0;
  _Bool r1;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_默认最大连续错误;
  return;
}

struct 词元 报告词法错误(struct 扫描器* cn_var_实例, enum 词法错误类型 cn_var_错误类型, const char* cn_var_详情) {
  long long r2, r5, r6, r12, r13, r15;
  char* r0;
  char* r1;
  char* r8;
  char* r9;
  char* r10;
  char* r11;
  char* r14;
  struct 扫描器* r3;
  _Bool r4;
  struct 词元 r16;
  enum 词法错误类型 r7;

  entry:
  struct 词元 cn_var_错误词元_0;
  r0 = cn_var_详情;
  r1 = cn_var_详情;
  r2 = 字符串长度(r1);
  r3 = cn_var_实例;
  r4 = r3 != 0;
  if (r4) goto if_then_2; else goto if_else_3;

  if_then_2:
  r5 = cn_var_实例->行号;
  r6 = cn_var_实例->列号;
  goto if_merge_4;

  if_else_3:
  goto if_merge_4;

  if_merge_4:
  char* cn_var_错误名称_1;
  r7 = cn_var_错误类型;
  r8 = 词法错误名称(r7);
  cn_var_错误名称_1 = r8;
  char* cn_var_完整消息_2;
  r9 = cn_var_错误名称_1;
  r10 = cn_var_详情;
  r11 = 字符串格式("词法错误: %s - %s", r9, r10);
  cn_var_完整消息_2 = r11;
  r12 = cn_var_错误词元_0.行号;
  r13 = cn_var_错误词元_0.列号;
  r14 = cn_var_完整消息_2;
  r15 = 打印格式("错误:%d:%d: %s\n", r12, r13, r14);
  r16 = cn_var_错误词元_0;
  return r16;
}

enum 恢复策略 选择恢复策略(struct 扫描器* cn_var_实例, enum 词法错误类型 cn_var_错误类型) {
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
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  enum 词法错误类型 r0;

  entry:
  r0 = cn_var_错误类型;
  r1 = r0 == 词法错误类型_非法字符;
  if (r1) goto case_body_6; else goto switch_check_23;

  switch_check_23:
  r2 = r0 == 词法错误类型_无效字符编码;
  if (r2) goto case_body_7; else goto switch_check_24;

  switch_check_24:
  r3 = r0 == 词法错误类型_未闭合字符串;
  if (r3) goto case_body_8; else goto switch_check_25;

  switch_check_25:
  r4 = r0 == 词法错误类型_字符串过长;
  if (r4) goto case_body_9; else goto switch_check_26;

  switch_check_26:
  r5 = r0 == 词法错误类型_无效转义序列;
  if (r5) goto case_body_10; else goto switch_check_27;

  switch_check_27:
  r6 = r0 == 词法错误类型_未闭合字符;
  if (r6) goto case_body_11; else goto switch_check_28;

  switch_check_28:
  r7 = r0 == 词法错误类型_空字符字面量;
  if (r7) goto case_body_12; else goto switch_check_29;

  switch_check_29:
  r8 = r0 == 词法错误类型_字符字面量过长;
  if (r8) goto case_body_13; else goto switch_check_30;

  switch_check_30:
  r9 = r0 == 词法错误类型_未闭合块注释;
  if (r9) goto case_body_14; else goto switch_check_31;

  switch_check_31:
  r10 = r0 == 词法错误类型_数字格式错误;
  if (r10) goto case_body_15; else goto switch_check_32;

  switch_check_32:
  r11 = r0 == 词法错误类型_无效十六进制;
  if (r11) goto case_body_16; else goto switch_check_33;

  switch_check_33:
  r12 = r0 == 词法错误类型_无效二进制;
  if (r12) goto case_body_17; else goto switch_check_34;

  switch_check_34:
  r13 = r0 == 词法错误类型_无效八进制;
  if (r13) goto case_body_18; else goto switch_check_35;

  switch_check_35:
  r14 = r0 == 词法错误类型_整数溢出;
  if (r14) goto case_body_19; else goto switch_check_36;

  switch_check_36:
  r15 = r0 == 词法错误类型_浮点数溢出;
  if (r15) goto case_body_20; else goto switch_check_37;

  switch_check_37:
  r16 = r0 == 词法错误类型_浮点数格式错误;
  if (r16) goto case_body_21; else goto case_default_22;

  case_body_6:
  goto switch_merge_5;

  case_body_7:
  return 恢复策略_跳过字符;
  goto switch_merge_5;

  case_body_8:
  goto switch_merge_5;

  case_body_9:
  goto switch_merge_5;

  case_body_10:
  return 恢复策略_跳过到行尾;
  goto switch_merge_5;

  case_body_11:
  goto switch_merge_5;

  case_body_12:
  goto switch_merge_5;

  case_body_13:
  return 恢复策略_跳过到行尾;
  goto switch_merge_5;

  case_body_14:
  return 恢复策略_跳过到同步点;
  goto switch_merge_5;

  case_body_15:
  goto switch_merge_5;

  case_body_16:
  goto switch_merge_5;

  case_body_17:
  goto switch_merge_5;

  case_body_18:
  goto switch_merge_5;

  case_body_19:
  goto switch_merge_5;

  case_body_20:
  goto switch_merge_5;

  case_body_21:
  return 恢复策略_跳过词元;
  goto switch_merge_5;

  case_default_22:
  return 恢复策略_跳过字符;
  goto switch_merge_5;

  switch_merge_5:
  return 0;
}

void 执行恢复(struct 扫描器* cn_var_实例, enum 恢复策略 cn_var_策略) {
  long long r10, r12, r13, r14, r16, r19, r22, r23, r24, r29, r30, r31, r33, r34, r35, r37, r42;
  struct 扫描器* r0;
  struct 扫描器* r9;
  struct 扫描器* r11;
  struct 扫描器* r18;
  struct 扫描器* r20;
  struct 扫描器* r25;
  struct 扫描器* r32;
  struct 扫描器* r39;
  struct 扫描器* r41;
  _Bool r1;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r15;
  _Bool r17;
  _Bool r21;
  _Bool r28;
  _Bool r36;
  _Bool r38;
  _Bool r40;
  struct 词元 r26;
  enum 恢复策略 r2;
  enum 词元类型枚举 r27;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_38; else goto if_merge_39;

  if_then_38:
  return;
  goto if_merge_39;

  if_merge_39:
  r2 = cn_var_策略;
  r3 = r2 == 恢复策略_无恢复;
  if (r3) goto case_body_41; else goto switch_check_48;

  switch_check_48:
  r4 = r2 == 恢复策略_跳过字符;
  if (r4) goto case_body_42; else goto switch_check_49;

  switch_check_49:
  r5 = r2 == 恢复策略_跳过到行尾;
  if (r5) goto case_body_43; else goto switch_check_50;

  switch_check_50:
  r6 = r2 == 恢复策略_跳过到同步点;
  if (r6) goto case_body_44; else goto switch_check_51;

  switch_check_51:
  r7 = r2 == 恢复策略_插入词元;
  if (r7) goto case_body_45; else goto switch_check_52;

  switch_check_52:
  r8 = r2 == 恢复策略_跳过词元;
  if (r8) goto case_body_46; else goto case_default_47;

  case_body_41:
  return;
  goto switch_merge_40;

  case_body_42:
  r9 = cn_var_实例;
  r10 = 前进(r9);
  return;
  goto switch_merge_40;

  case_body_43:
  goto while_cond_53;

  while_cond_53:
  if (1) goto while_body_54; else goto while_exit_55;

  while_body_54:
  long long cn_var_c_0;
  r11 = cn_var_实例;
  r12 = 当前字符(r11);
  cn_var_c_0 = r12;
  r14 = cn_var_c_0;
  r15 = r14 == 0;
  if (r15) goto logic_merge_59; else goto logic_rhs_58;

  while_exit_55:
  return;
  goto switch_merge_40;

  if_then_56:
  goto while_exit_55;
  goto if_merge_57;

  if_merge_57:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_53;

  logic_rhs_58:
  r16 = cn_var_c_0;
  r17 = r16 == 10;
  goto logic_merge_59;

  logic_merge_59:
  if (r17) goto if_then_56; else goto if_merge_57;

  case_body_44:
  goto while_cond_60;

  while_cond_60:
  if (1) goto while_body_61; else goto while_exit_62;

  while_body_61:
  r20 = cn_var_实例;
  r21 = 是否结束(r20);
  if (r21) goto if_then_63; else goto if_merge_64;

  while_exit_62:
  return;
  goto switch_merge_40;

  if_then_63:
  goto while_exit_62;
  goto if_merge_64;

  if_merge_64:
  long long cn_var_保存位置_1;
  r22 = cn_var_实例->位置;
  cn_var_保存位置_1 = r22;
  long long cn_var_保存行号_2;
  r23 = cn_var_实例->行号;
  cn_var_保存行号_2 = r23;
  long long cn_var_保存列号_3;
  r24 = cn_var_实例->列号;
  cn_var_保存列号_3 = r24;
  struct 词元 cn_var_当前词元_4;
  r25 = cn_var_实例;
  r26 = 下一个词元(r25);
  cn_var_当前词元_4 = r26;
  r27 = cn_var_当前词元_4.类型;
  r28 = 是否同步点(r27);
  if (r28) goto if_then_65; else goto if_merge_66;

  if_then_65:
  r29 = cn_var_保存位置_1;
  r30 = cn_var_保存行号_2;
  r31 = cn_var_保存列号_3;
  goto while_exit_62;
  goto if_merge_66;

  if_merge_66:
  goto while_cond_60;

  case_body_45:
  return;
  goto switch_merge_40;

  case_body_46:
  goto while_cond_67;

  while_cond_67:
  if (1) goto while_body_68; else goto while_exit_69;

  while_body_68:
  long long cn_var_c_5;
  r32 = cn_var_实例;
  r33 = 当前字符(r32);
  cn_var_c_5 = r33;
  r35 = cn_var_c_5;
  r36 = 是空白字符(r35);
  if (r36) goto logic_merge_73; else goto logic_rhs_72;

  while_exit_69:
  return;
  goto switch_merge_40;

  if_then_70:
  goto while_exit_69;
  goto if_merge_71;

  if_merge_71:
  r39 = cn_var_实例;
  r40 = 是否结束(r39);
  if (r40) goto if_then_74; else goto if_merge_75;

  logic_rhs_72:
  r37 = cn_var_c_5;
  r38 = 是分隔符(r37);
  goto logic_merge_73;

  logic_merge_73:
  if (r38) goto if_then_70; else goto if_merge_71;

  if_then_74:
  goto while_exit_69;
  goto if_merge_75;

  if_merge_75:
  r41 = cn_var_实例;
  r42 = 前进(r41);
  goto while_cond_67;

  case_default_47:
  return;
  goto switch_merge_40;

  switch_merge_40:
  return;
}

_Bool 是否同步点(enum 词元类型枚举 cn_var_类型) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  enum 词元类型枚举 r0;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 词元类型枚举_分号;
  if (r1) goto case_body_77; else goto switch_check_83;

  switch_check_83:
  r2 = r0 == 词元类型枚举_右大括号;
  if (r2) goto case_body_78; else goto switch_check_84;

  switch_check_84:
  r3 = r0 == 词元类型枚举_右括号;
  if (r3) goto case_body_79; else goto switch_check_85;

  switch_check_85:
  r4 = r0 == 词元类型枚举_右方括号;
  if (r4) goto case_body_80; else goto switch_check_86;

  switch_check_86:
  r5 = r0 == 词元类型枚举_逗号;
  if (r5) goto case_body_81; else goto case_default_82;

  case_body_77:
  goto switch_merge_76;

  case_body_78:
  goto switch_merge_76;

  case_body_79:
  goto switch_merge_76;

  case_body_80:
  goto switch_merge_76;

  case_body_81:
  return 1;
  goto switch_merge_76;

  case_default_82:
  return 0;
  goto switch_merge_76;

  switch_merge_76:
  return 0;
}

_Bool 应该抑制错误(struct 错误恢复上下文* cn_var_上下文) {
  long long r2, r3, r5, r6;
  struct 错误恢复上下文* r0;
  _Bool r1;
  _Bool r4;
  _Bool r7;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_87; else goto if_merge_88;

  if_then_87:
  return 0;
  goto if_merge_88;

  if_merge_88:
  r2 = cn_var_上下文->级联抑制计数;
  r3 = cn_var_级联错误阈值;
  r4 = r2 >= r3;
  if (r4) goto if_then_89; else goto if_merge_90;

  if_then_89:
  return 1;
  goto if_merge_90;

  if_merge_90:
  r5 = cn_var_上下文->连续错误计数;
  r6 = cn_var_上下文->最大连续错误;
  r7 = r5 >= r6;
  if (r7) goto if_then_91; else goto if_merge_92;

  if_then_91:
  return 1;
  goto if_merge_92;

  if_merge_92:
  return 0;
}

void 重置错误计数(struct 错误恢复上下文* cn_var_上下文) {
  struct 错误恢复上下文* r0;
  _Bool r1;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_93; else goto if_merge_94;

  if_then_93:
  return;
  goto if_merge_94;

  if_merge_94:
  return;
}

char* 词法错误名称(enum 词法错误类型 cn_var_错误类型) {
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
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  enum 词法错误类型 r0;

  entry:
  r0 = cn_var_错误类型;
  r1 = r0 == 词法错误类型_非法字符;
  if (r1) goto case_body_96; else goto switch_check_113;

  switch_check_113:
  r2 = r0 == 词法错误类型_无效字符编码;
  if (r2) goto case_body_97; else goto switch_check_114;

  switch_check_114:
  r3 = r0 == 词法错误类型_未闭合字符串;
  if (r3) goto case_body_98; else goto switch_check_115;

  switch_check_115:
  r4 = r0 == 词法错误类型_字符串过长;
  if (r4) goto case_body_99; else goto switch_check_116;

  switch_check_116:
  r5 = r0 == 词法错误类型_无效转义序列;
  if (r5) goto case_body_100; else goto switch_check_117;

  switch_check_117:
  r6 = r0 == 词法错误类型_未闭合字符;
  if (r6) goto case_body_101; else goto switch_check_118;

  switch_check_118:
  r7 = r0 == 词法错误类型_空字符字面量;
  if (r7) goto case_body_102; else goto switch_check_119;

  switch_check_119:
  r8 = r0 == 词法错误类型_字符字面量过长;
  if (r8) goto case_body_103; else goto switch_check_120;

  switch_check_120:
  r9 = r0 == 词法错误类型_未闭合块注释;
  if (r9) goto case_body_104; else goto switch_check_121;

  switch_check_121:
  r10 = r0 == 词法错误类型_数字格式错误;
  if (r10) goto case_body_105; else goto switch_check_122;

  switch_check_122:
  r11 = r0 == 词法错误类型_无效十六进制;
  if (r11) goto case_body_106; else goto switch_check_123;

  switch_check_123:
  r12 = r0 == 词法错误类型_无效二进制;
  if (r12) goto case_body_107; else goto switch_check_124;

  switch_check_124:
  r13 = r0 == 词法错误类型_无效八进制;
  if (r13) goto case_body_108; else goto switch_check_125;

  switch_check_125:
  r14 = r0 == 词法错误类型_整数溢出;
  if (r14) goto case_body_109; else goto switch_check_126;

  switch_check_126:
  r15 = r0 == 词法错误类型_浮点数溢出;
  if (r15) goto case_body_110; else goto switch_check_127;

  switch_check_127:
  r16 = r0 == 词法错误类型_浮点数格式错误;
  if (r16) goto case_body_111; else goto case_default_112;

  case_body_96:
  return "非法字符";
  goto switch_merge_95;

  case_body_97:
  return "无效字符编码";
  goto switch_merge_95;

  case_body_98:
  return "未闭合字符串";
  goto switch_merge_95;

  case_body_99:
  return "字符串过长";
  goto switch_merge_95;

  case_body_100:
  return "无效转义序列";
  goto switch_merge_95;

  case_body_101:
  return "未闭合字符字面量";
  goto switch_merge_95;

  case_body_102:
  return "空字符字面量";
  goto switch_merge_95;

  case_body_103:
  return "字符字面量过长";
  goto switch_merge_95;

  case_body_104:
  return "未闭合块注释";
  goto switch_merge_95;

  case_body_105:
  return "数字格式错误";
  goto switch_merge_95;

  case_body_106:
  return "无效十六进制";
  goto switch_merge_95;

  case_body_107:
  return "无效二进制";
  goto switch_merge_95;

  case_body_108:
  return "无效八进制";
  goto switch_merge_95;

  case_body_109:
  return "整数溢出";
  goto switch_merge_95;

  case_body_110:
  return "浮点数溢出";
  goto switch_merge_95;

  case_body_111:
  return "浮点数格式错误";
  goto switch_merge_95;

  case_default_112:
  return "未知错误";
  goto switch_merge_95;

  switch_merge_95:
  return NULL;
}

char* 恢复策略名称(enum 恢复策略 cn_var_策略) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  enum 恢复策略 r0;

  entry:
  r0 = cn_var_策略;
  r1 = r0 == 恢复策略_无恢复;
  if (r1) goto case_body_129; else goto switch_check_136;

  switch_check_136:
  r2 = r0 == 恢复策略_跳过字符;
  if (r2) goto case_body_130; else goto switch_check_137;

  switch_check_137:
  r3 = r0 == 恢复策略_跳过到行尾;
  if (r3) goto case_body_131; else goto switch_check_138;

  switch_check_138:
  r4 = r0 == 恢复策略_跳过到同步点;
  if (r4) goto case_body_132; else goto switch_check_139;

  switch_check_139:
  r5 = r0 == 恢复策略_插入词元;
  if (r5) goto case_body_133; else goto switch_check_140;

  switch_check_140:
  r6 = r0 == 恢复策略_跳过词元;
  if (r6) goto case_body_134; else goto case_default_135;

  case_body_129:
  return "无恢复";
  goto switch_merge_128;

  case_body_130:
  return "跳过字符";
  goto switch_merge_128;

  case_body_131:
  return "跳过到行尾";
  goto switch_merge_128;

  case_body_132:
  return "跳过到同步点";
  goto switch_merge_128;

  case_body_133:
  return "插入词元";
  goto switch_merge_128;

  case_body_134:
  return "跳过词元";
  goto switch_merge_128;

  case_default_135:
  return "未知策略";
  goto switch_merge_128;

  switch_merge_128:
  return NULL;
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
  if (r4) goto logic_merge_146; else goto logic_rhs_145;

  logic_rhs_141:
  r9 = cn_var_c;
  r10 = r9 == 13;
  goto logic_merge_142;

  logic_merge_142:
  return r10;

  logic_rhs_143:
  r7 = cn_var_c;
  r8 = r7 == 10;
  goto logic_merge_144;

  logic_merge_144:
  if (r8) goto logic_merge_142; else goto logic_rhs_141;

  logic_rhs_145:
  r5 = cn_var_c;
  r6 = r5 == 9;
  goto logic_merge_146;

  logic_merge_146:
  if (r6) goto logic_merge_144; else goto logic_rhs_143;
  return 0;
}

_Bool 是分隔符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r9, r11, r13, r15, r17, r19, r21;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r20;
  _Bool r22;

  entry:
  r7 = cn_var_c;
  r8 = r7 == 59;
  if (r8) goto logic_merge_160; else goto logic_rhs_159;

  logic_rhs_147:
  r21 = cn_var_c;
  r22 = r21 == 93;
  goto logic_merge_148;

  logic_merge_148:
  return r22;

  logic_rhs_149:
  r19 = cn_var_c;
  r20 = r19 == 91;
  goto logic_merge_150;

  logic_merge_150:
  if (r20) goto logic_merge_148; else goto logic_rhs_147;

  logic_rhs_151:
  r17 = cn_var_c;
  r18 = r17 == 125;
  goto logic_merge_152;

  logic_merge_152:
  if (r18) goto logic_merge_150; else goto logic_rhs_149;

  logic_rhs_153:
  r15 = cn_var_c;
  r16 = r15 == 123;
  goto logic_merge_154;

  logic_merge_154:
  if (r16) goto logic_merge_152; else goto logic_rhs_151;

  logic_rhs_155:
  r13 = cn_var_c;
  r14 = r13 == 41;
  goto logic_merge_156;

  logic_merge_156:
  if (r14) goto logic_merge_154; else goto logic_rhs_153;

  logic_rhs_157:
  r11 = cn_var_c;
  r12 = r11 == 40;
  goto logic_merge_158;

  logic_merge_158:
  if (r12) goto logic_merge_156; else goto logic_rhs_155;

  logic_rhs_159:
  r9 = cn_var_c;
  r10 = r9 == 44;
  goto logic_merge_160;

  logic_merge_160:
  if (r10) goto logic_merge_158; else goto logic_rhs_157;
  return 0;
}

void 更新错误上下文(struct 错误恢复上下文* cn_var_上下文, long long cn_var_当前行号) {
  long long r2, r3, r4, r5, r7, r8, r9, r10, r12;
  struct 错误恢复上下文* r0;
  _Bool r1;
  _Bool r6;
  _Bool r11;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_161; else goto if_merge_162;

  if_then_161:
  return;
  goto if_merge_162;

  if_merge_162:
  r2 = cn_var_上下文->连续错误计数;
  r3 = r2 + 1;
  r4 = cn_var_当前行号;
  r5 = cn_var_上下文->最后错误行号;
  r6 = r4 == r5;
  if (r6) goto if_then_163; else goto if_else_164;

  if_then_163:
  r7 = cn_var_上下文->级联抑制计数;
  r8 = r7 + 1;
  r9 = cn_var_上下文->级联抑制计数;
  r10 = cn_var_级联错误阈值;
  r11 = r9 >= r10;
  if (r11) goto if_then_166; else goto if_merge_167;

  if_else_164:
  goto if_merge_165;

  if_merge_165:
  r12 = cn_var_当前行号;

  if_then_166:
  goto if_merge_167;

  if_merge_167:
  goto if_merge_165;
  return;
}

_Bool 应该停止编译(struct 错误恢复上下文* cn_var_上下文) {
  long long r2, r3;
  struct 错误恢复上下文* r0;
  _Bool r1;
  _Bool r4;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_168; else goto if_merge_169;

  if_then_168:
  return 0;
  goto if_merge_169;

  if_merge_169:
  r2 = cn_var_上下文->连续错误计数;
  r3 = cn_var_上下文->最大连续错误;
  r4 = r2 >= r3;
  return r4;
}

