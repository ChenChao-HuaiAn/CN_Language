#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块
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
  long long r1, r2, r3;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_827; else goto if_merge_828;

  if_then_827:
  return 0;
  goto if_merge_828;

  if_merge_828:
  r2 = cn_var_默认最大连续错误;
  r3 = cn_var_默认最大恢复次数;
  return 1;
}

void 销毁错误恢复上下文(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 != 0;
  if (r1) goto if_then_829; else goto if_merge_830;

  if_then_829:
  goto if_merge_830;

  if_merge_830:
  return;
}

long long 是同步点(enum 词元类型枚举 cn_var_词元类型, enum 同步点类型 cn_var_同步类型) {
  long long r2, r5, r8, r11, r14, r17, r20, r23, r26, r28, r30, r32, r34, r35, r37, r39, r40, r42, r44, r45, r47, r49;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  enum 同步点类型 r0;
  enum 同步点类型 r1;
  enum 同步点类型 r4;
  enum 同步点类型 r7;
  enum 同步点类型 r10;
  enum 同步点类型 r13;
  enum 同步点类型 r16;
  enum 同步点类型 r19;
  enum 同步点类型 r22;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r33;
  enum 词元类型枚举 r36;
  enum 词元类型枚举 r38;
  enum 词元类型枚举 r41;
  enum 词元类型枚举 r43;
  enum 词元类型枚举 r46;
  enum 词元类型枚举 r48;

  entry:
  r0 = cn_var_同步类型;
  r1 = cn_var_同步点类型;
  r2 = r1.同步点_分号;
  r3 = r0 == r2;
  if (r3) goto case_body_832; else goto switch_check_841;

  switch_check_841:
  r4 = cn_var_同步点类型;
  r5 = r4.同步点_右大括号;
  r6 = r0 == r5;
  if (r6) goto case_body_833; else goto switch_check_842;

  switch_check_842:
  r7 = cn_var_同步点类型;
  r8 = r7.同步点_右小括号;
  r9 = r0 == r8;
  if (r9) goto case_body_834; else goto switch_check_843;

  switch_check_843:
  r10 = cn_var_同步点类型;
  r11 = r10.同步点_右中括号;
  r12 = r0 == r11;
  if (r12) goto case_body_835; else goto switch_check_844;

  switch_check_844:
  r13 = cn_var_同步点类型;
  r14 = r13.同步点_逗号;
  r15 = r0 == r14;
  if (r15) goto case_body_836; else goto switch_check_845;

  switch_check_845:
  r16 = cn_var_同步点类型;
  r17 = r16.同步点_关键字;
  r18 = r0 == r17;
  if (r18) goto case_body_837; else goto switch_check_846;

  switch_check_846:
  r19 = cn_var_同步点类型;
  r20 = r19.同步点_类型关键字;
  r21 = r0 == r20;
  if (r21) goto case_body_838; else goto switch_check_847;

  switch_check_847:
  r22 = cn_var_同步点类型;
  r23 = r22.同步点_声明关键字;
  r24 = r0 == r23;
  if (r24) goto case_body_839; else goto case_default_840;

  case_body_832:
  r25 = cn_var_词元类型;
  r26 = r25 == 1;
  return r26;
  goto switch_merge_831;

  case_body_833:
  r27 = cn_var_词元类型;
  r28 = r27 == 2;
  return r28;
  goto switch_merge_831;

  case_body_834:
  r29 = cn_var_词元类型;
  r30 = r29 == 3;
  return r30;
  goto switch_merge_831;

  case_body_835:
  r31 = cn_var_词元类型;
  r32 = r31 == 4;
  return r32;
  goto switch_merge_831;

  case_body_836:
  r33 = cn_var_词元类型;
  r34 = r33 == 5;
  return r34;
  goto switch_merge_831;

  case_body_837:
  r36 = cn_var_词元类型;
  r37 = r36 >= 100;
  if (r37) goto logic_rhs_848; else goto logic_merge_849;

  logic_rhs_848:
  r38 = cn_var_词元类型;
  r39 = r38 <= 150;
  goto logic_merge_849;

  logic_merge_849:
  return r39;
  goto switch_merge_831;

  case_body_838:
  r41 = cn_var_词元类型;
  r42 = r41 >= 160;
  if (r42) goto logic_rhs_850; else goto logic_merge_851;

  logic_rhs_850:
  r43 = cn_var_词元类型;
  r44 = r43 <= 170;
  goto logic_merge_851;

  logic_merge_851:
  return r44;
  goto switch_merge_831;

  case_body_839:
  r46 = cn_var_词元类型;
  r47 = r46 >= 180;
  if (r47) goto logic_rhs_852; else goto logic_merge_853;

  logic_rhs_852:
  r48 = cn_var_词元类型;
  r49 = r48 <= 190;
  goto logic_merge_853;

  logic_merge_853:
  return r49;
  goto switch_merge_831;

  case_default_840:
  return 0;
  goto switch_merge_831;

  switch_merge_831:
  return 0;
}

long long 是任意同步点(enum 词元类型枚举 cn_var_词元类型) {
  long long r2, r5, r8, r11, r14, r17, r20, r23;
  enum 词元类型枚举 r0;
  enum 同步点类型 r1;
  enum 词元类型枚举 r3;
  enum 同步点类型 r4;
  enum 词元类型枚举 r6;
  enum 同步点类型 r7;
  enum 词元类型枚举 r9;
  enum 同步点类型 r10;
  enum 词元类型枚举 r12;
  enum 同步点类型 r13;
  enum 词元类型枚举 r15;
  enum 同步点类型 r16;
  enum 词元类型枚举 r18;
  enum 同步点类型 r19;
  enum 词元类型枚举 r21;
  enum 同步点类型 r22;

  entry:
  r0 = cn_var_词元类型;
  r1 = cn_var_同步点类型;
  r2 = r1.同步点_分号;
  是同步点(r0, r2);
  goto if_then_854;

  if_then_854:
  return 1;
  goto if_merge_855;

  if_merge_855:
  r3 = cn_var_词元类型;
  r4 = cn_var_同步点类型;
  r5 = r4.同步点_右大括号;
  是同步点(r3, r5);
  goto if_then_856;

  if_then_856:
  return 1;
  goto if_merge_857;

  if_merge_857:
  r6 = cn_var_词元类型;
  r7 = cn_var_同步点类型;
  r8 = r7.同步点_右小括号;
  是同步点(r6, r8);
  goto if_then_858;

  if_then_858:
  return 1;
  goto if_merge_859;

  if_merge_859:
  r9 = cn_var_词元类型;
  r10 = cn_var_同步点类型;
  r11 = r10.同步点_右中括号;
  是同步点(r9, r11);
  goto if_then_860;

  if_then_860:
  return 1;
  goto if_merge_861;

  if_merge_861:
  r12 = cn_var_词元类型;
  r13 = cn_var_同步点类型;
  r14 = r13.同步点_逗号;
  是同步点(r12, r14);
  goto if_then_862;

  if_then_862:
  return 1;
  goto if_merge_863;

  if_merge_863:
  r15 = cn_var_词元类型;
  r16 = cn_var_同步点类型;
  r17 = r16.同步点_关键字;
  是同步点(r15, r17);
  goto if_then_864;

  if_then_864:
  return 1;
  goto if_merge_865;

  if_merge_865:
  r18 = cn_var_词元类型;
  r19 = cn_var_同步点类型;
  r20 = r19.同步点_类型关键字;
  是同步点(r18, r20);
  goto if_then_866;

  if_then_866:
  return 1;
  goto if_merge_867;

  if_merge_867:
  r21 = cn_var_词元类型;
  r22 = cn_var_同步点类型;
  r23 = r22.同步点_声明关键字;
  是同步点(r21, r23);
  goto if_then_868;

  if_then_868:
  return 1;
  goto if_merge_869;

  if_merge_869:
  return 0;
}

long long 跳转到同步点(struct 语法错误恢复上下文* cn_var_上下文, enum 词元类型枚举 cn_var_当前词元类型, enum 同步点类型 cn_var_目标同步类型) {
  long long r1, r3, r4, r6, r8, r9, r14, r15;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r5;
  struct 语法错误恢复上下文* r7;
  enum 词元类型枚举 r10;
  enum 同步点类型 r11;
  enum 同步点类型 r12;
  enum 同步点类型 r13;
  enum 词元类型枚举 r16;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_870; else goto if_merge_871;

  if_then_870:
  return 0;
  goto if_merge_871;

  if_merge_871:
  r2 = cn_var_上下文;
  r3 = r2->恢复尝试次数;
  r4 = r3 + 1;
  r5 = cn_var_上下文;
  r6 = r5->恢复尝试次数;
  r7 = cn_var_上下文;
  r8 = r7->最大恢复次数;
  r9 = r6 > r8;
  if (r9) goto if_then_872; else goto if_merge_873;

  if_then_872:
  return 0;
  goto if_merge_873;

  if_merge_873:
  r10 = cn_var_当前词元类型;
  r11 = cn_var_目标同步类型;
  是同步点(r10, r11);
  goto if_then_874;

  if_then_874:
  return 1;
  goto if_merge_875;

  if_merge_875:
  r12 = cn_var_目标同步类型;
  r13 = cn_var_同步点类型;
  r14 = r13.同步点_分号;
  r15 = r12 == r14;
  if (r15) goto if_then_876; else goto if_merge_877;

  if_then_876:
  r16 = cn_var_当前词元类型;
  是任意同步点(r16);
  goto if_then_878;

  if_merge_877:
  return 0;

  if_then_878:
  return 1;
  goto if_merge_879;

  if_merge_879:
  goto if_merge_877;
  return 0;
}

_Bool 应该抑制错误(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1, r5, r7, r8;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r4;
  struct 语法错误恢复上下文* r6;
  _Bool r3;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_880; else goto if_merge_881;

  if_then_880:
  return 0;
  goto if_merge_881;

  if_merge_881:
  r2 = cn_var_上下文;
  r3 = r2->抑制级联错误;
  if (r3) goto if_then_882; else goto if_merge_883;

  if_then_882:
  return 1;
  goto if_merge_883;

  if_merge_883:
  r4 = cn_var_上下文;
  r5 = r4->连续错误计数;
  r6 = cn_var_上下文;
  r7 = r6->最大连续错误;
  r8 = r5 >= r7;
  if (r8) goto if_then_884; else goto if_merge_885;

  if_then_884:
  return 1;
  goto if_merge_885;

  if_merge_885:
  return 0;
}

void 记录错误(struct 语法错误恢复上下文* cn_var_上下文, long long cn_var_行号) {
  long long r1, r3, r4, r5, r7, r8, r10, r12, r13, r15, r16, r17;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r6;
  struct 语法错误恢复上下文* r9;
  struct 语法错误恢复上下文* r11;
  struct 语法错误恢复上下文* r14;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_886; else goto if_merge_887;

  if_then_886:
  return;
  goto if_merge_887;

  if_merge_887:
  r2 = cn_var_上下文;
  r3 = r2->最后错误行号;
  r4 = cn_var_行号;
  r5 = r3 == r4;
  if (r5) goto if_then_888; else goto if_else_889;

  if_then_888:
  r6 = cn_var_上下文;
  r7 = r6->级联抑制计数;
  r8 = r7 + 1;
  r9 = cn_var_上下文;
  r10 = r9->级联抑制计数;
  r11 = cn_var_上下文;
  r12 = r11->最大连续错误;
  r13 = r10 >= r12;
  if (r13) goto if_then_891; else goto if_merge_892;

  if_else_889:
  goto if_merge_890;

  if_merge_890:
  r14 = cn_var_上下文;
  r15 = r14->连续错误计数;
  r16 = r15 + 1;
  r17 = cn_var_行号;

  if_then_891:
  goto if_merge_892;

  if_merge_892:
  goto if_merge_890;
  return;
}

void 重置错误计数(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_893; else goto if_merge_894;

  if_then_893:
  return;
  goto if_merge_894;

  if_merge_894:
  return;
}

enum 语法恢复策略 选择恢复策略(long long cn_var_错误码) {
  long long r0, r14, r16, r18, r20, r22, r24;
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
  enum 语法恢复策略 r13;
  enum 语法恢复策略 r15;
  enum 语法恢复策略 r17;
  enum 语法恢复策略 r19;
  enum 语法恢复策略 r21;
  enum 语法恢复策略 r23;

  entry:
  r0 = cn_var_错误码;
  r1 = r0 == 101;
  if (r1) goto case_body_896; else goto switch_check_909;

  switch_check_909:
  r2 = r0 == 102;
  if (r2) goto case_body_897; else goto switch_check_910;

  switch_check_910:
  r3 = r0 == 103;
  if (r3) goto case_body_898; else goto switch_check_911;

  switch_check_911:
  r4 = r0 == 104;
  if (r4) goto case_body_899; else goto switch_check_912;

  switch_check_912:
  r5 = r0 == 105;
  if (r5) goto case_body_900; else goto switch_check_913;

  switch_check_913:
  r6 = r0 == 106;
  if (r6) goto case_body_901; else goto switch_check_914;

  switch_check_914:
  r7 = r0 == 107;
  if (r7) goto case_body_902; else goto switch_check_915;

  switch_check_915:
  r8 = r0 == 108;
  if (r8) goto case_body_903; else goto switch_check_916;

  switch_check_916:
  r9 = r0 == 109;
  if (r9) goto case_body_904; else goto switch_check_917;

  switch_check_917:
  r10 = r0 == 1;
  if (r10) goto case_body_905; else goto switch_check_918;

  switch_check_918:
  r11 = r0 == 2;
  if (r11) goto case_body_906; else goto switch_check_919;

  switch_check_919:
  r12 = r0 == 3;
  if (r12) goto case_body_907; else goto case_default_908;

  case_body_896:
  goto switch_merge_895;

  case_body_897:
  r13 = cn_var_语法恢复策略;
  r14 = r13.同步到分号;
  return r14;
  goto switch_merge_895;

  case_body_898:
  goto switch_merge_895;

  case_body_899:
  r15 = cn_var_语法恢复策略;
  r16 = r15.同步到大括号;
  return r16;
  goto switch_merge_895;

  case_body_900:
  goto switch_merge_895;

  case_body_901:
  goto switch_merge_895;

  case_body_902:
  r17 = cn_var_语法恢复策略;
  r18 = r17.同步到关键字;
  return r18;
  goto switch_merge_895;

  case_body_903:
  goto switch_merge_895;

  case_body_904:
  r19 = cn_var_语法恢复策略;
  r20 = r19.跳过词元;
  return r20;
  goto switch_merge_895;

  case_body_905:
  goto switch_merge_895;

  case_body_906:
  goto switch_merge_895;

  case_body_907:
  r21 = cn_var_语法恢复策略;
  r22 = r21.跳过词元;
  return r22;
  goto switch_merge_895;

  case_default_908:
  r23 = cn_var_语法恢复策略;
  r24 = r23.同步到分号;
  return r24;
  goto switch_merge_895;

  switch_merge_895:
  return 0;
}

char* 同步点类型名称(enum 同步点类型 cn_var_同步类型) {
  long long r2, r5, r8, r11, r14, r17, r20, r23;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  enum 同步点类型 r0;
  enum 同步点类型 r1;
  enum 同步点类型 r4;
  enum 同步点类型 r7;
  enum 同步点类型 r10;
  enum 同步点类型 r13;
  enum 同步点类型 r16;
  enum 同步点类型 r19;
  enum 同步点类型 r22;

  entry:
  r0 = cn_var_同步类型;
  r1 = cn_var_同步点类型;
  r2 = r1.同步点_分号;
  r3 = r0 == r2;
  if (r3) goto case_body_921; else goto switch_check_930;

  switch_check_930:
  r4 = cn_var_同步点类型;
  r5 = r4.同步点_右大括号;
  r6 = r0 == r5;
  if (r6) goto case_body_922; else goto switch_check_931;

  switch_check_931:
  r7 = cn_var_同步点类型;
  r8 = r7.同步点_右小括号;
  r9 = r0 == r8;
  if (r9) goto case_body_923; else goto switch_check_932;

  switch_check_932:
  r10 = cn_var_同步点类型;
  r11 = r10.同步点_右中括号;
  r12 = r0 == r11;
  if (r12) goto case_body_924; else goto switch_check_933;

  switch_check_933:
  r13 = cn_var_同步点类型;
  r14 = r13.同步点_逗号;
  r15 = r0 == r14;
  if (r15) goto case_body_925; else goto switch_check_934;

  switch_check_934:
  r16 = cn_var_同步点类型;
  r17 = r16.同步点_关键字;
  r18 = r0 == r17;
  if (r18) goto case_body_926; else goto switch_check_935;

  switch_check_935:
  r19 = cn_var_同步点类型;
  r20 = r19.同步点_类型关键字;
  r21 = r0 == r20;
  if (r21) goto case_body_927; else goto switch_check_936;

  switch_check_936:
  r22 = cn_var_同步点类型;
  r23 = r22.同步点_声明关键字;
  r24 = r0 == r23;
  if (r24) goto case_body_928; else goto case_default_929;

  case_body_921:
  return "分号";
  goto switch_merge_920;

  case_body_922:
  return "右大括号";
  goto switch_merge_920;

  case_body_923:
  return "右小括号";
  goto switch_merge_920;

  case_body_924:
  return "右中括号";
  goto switch_merge_920;

  case_body_925:
  return "逗号";
  goto switch_merge_920;

  case_body_926:
  return "关键字";
  goto switch_merge_920;

  case_body_927:
  return "类型关键字";
  goto switch_merge_920;

  case_body_928:
  return "声明关键字";
  goto switch_merge_920;

  case_default_929:
  return "未知同步点";
  goto switch_merge_920;

  switch_merge_920:
  return NULL;
}

char* 恢复策略名称(enum 语法恢复策略 cn_var_策略) {
  long long r2, r5, r8, r11, r14;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  enum 语法恢复策略 r0;
  enum 语法恢复策略 r1;
  enum 语法恢复策略 r4;
  enum 语法恢复策略 r7;
  enum 语法恢复策略 r10;
  enum 语法恢复策略 r13;

  entry:
  r0 = cn_var_策略;
  r1 = cn_var_语法恢复策略;
  r2 = r1.不恢复;
  r3 = r0 == r2;
  if (r3) goto case_body_938; else goto switch_check_944;

  switch_check_944:
  r4 = cn_var_语法恢复策略;
  r5 = r4.跳过词元;
  r6 = r0 == r5;
  if (r6) goto case_body_939; else goto switch_check_945;

  switch_check_945:
  r7 = cn_var_语法恢复策略;
  r8 = r7.同步到分号;
  r9 = r0 == r8;
  if (r9) goto case_body_940; else goto switch_check_946;

  switch_check_946:
  r10 = cn_var_语法恢复策略;
  r11 = r10.同步到大括号;
  r12 = r0 == r11;
  if (r12) goto case_body_941; else goto switch_check_947;

  switch_check_947:
  r13 = cn_var_语法恢复策略;
  r14 = r13.同步到关键字;
  r15 = r0 == r14;
  if (r15) goto case_body_942; else goto case_default_943;

  case_body_938:
  return "不恢复";
  goto switch_merge_937;

  case_body_939:
  return "跳过词元";
  goto switch_merge_937;

  case_body_940:
  return "同步到分号";
  goto switch_merge_937;

  case_body_941:
  return "同步到大括号";
  goto switch_merge_937;

  case_body_942:
  return "同步到关键字";
  goto switch_merge_937;

  case_default_943:
  return "未知策略";
  goto switch_merge_937;

  switch_merge_937:
  return NULL;
}

