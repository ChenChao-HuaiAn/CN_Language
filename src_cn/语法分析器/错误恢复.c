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
  long long r1, r2, r3;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_892; else goto if_merge_893;

  if_then_892:
  return 0;
  goto if_merge_893;

  if_merge_893:
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
  if (r1) goto if_then_894; else goto if_merge_895;

  if_then_894:
  goto if_merge_895;

  if_merge_895:
  return;
}

long long 是同步点(enum 词元类型枚举 cn_var_词元类型, enum 同步点类型 cn_var_同步类型) {
  long long r10, r12, r14, r16, r18, r19, r21, r23, r24, r26, r28, r29, r31, r33;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
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
  if (r1) goto case_body_897; else goto switch_check_906;

  switch_check_906:
  r2 = r0 == 1;
  if (r2) goto case_body_898; else goto switch_check_907;

  switch_check_907:
  r3 = r0 == 2;
  if (r3) goto case_body_899; else goto switch_check_908;

  switch_check_908:
  r4 = r0 == 3;
  if (r4) goto case_body_900; else goto switch_check_909;

  switch_check_909:
  r5 = r0 == 4;
  if (r5) goto case_body_901; else goto switch_check_910;

  switch_check_910:
  r6 = r0 == 5;
  if (r6) goto case_body_902; else goto switch_check_911;

  switch_check_911:
  r7 = r0 == 6;
  if (r7) goto case_body_903; else goto switch_check_912;

  switch_check_912:
  r8 = r0 == 7;
  if (r8) goto case_body_904; else goto case_default_905;

  case_body_897:
  r9 = cn_var_词元类型;
  r10 = r9 == 1;
  return r10;
  goto switch_merge_896;

  case_body_898:
  r11 = cn_var_词元类型;
  r12 = r11 == 2;
  return r12;
  goto switch_merge_896;

  case_body_899:
  r13 = cn_var_词元类型;
  r14 = r13 == 3;
  return r14;
  goto switch_merge_896;

  case_body_900:
  r15 = cn_var_词元类型;
  r16 = r15 == 4;
  return r16;
  goto switch_merge_896;

  case_body_901:
  r17 = cn_var_词元类型;
  r18 = r17 == 5;
  return r18;
  goto switch_merge_896;

  case_body_902:
  r20 = cn_var_词元类型;
  r21 = r20 >= 100;
  if (r21) goto logic_rhs_913; else goto logic_merge_914;

  logic_rhs_913:
  r22 = cn_var_词元类型;
  r23 = r22 <= 150;
  goto logic_merge_914;

  logic_merge_914:
  return r23;
  goto switch_merge_896;

  case_body_903:
  r25 = cn_var_词元类型;
  r26 = r25 >= 160;
  if (r26) goto logic_rhs_915; else goto logic_merge_916;

  logic_rhs_915:
  r27 = cn_var_词元类型;
  r28 = r27 <= 170;
  goto logic_merge_916;

  logic_merge_916:
  return r28;
  goto switch_merge_896;

  case_body_904:
  r30 = cn_var_词元类型;
  r31 = r30 >= 180;
  if (r31) goto logic_rhs_917; else goto logic_merge_918;

  logic_rhs_917:
  r32 = cn_var_词元类型;
  r33 = r32 <= 190;
  goto logic_merge_918;

  logic_merge_918:
  return r33;
  goto switch_merge_896;

  case_default_905:
  return 0;
  goto switch_merge_896;

  switch_merge_896:
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
  if (r1) goto if_then_919; else goto if_merge_920;

  if_then_919:
  return 1;
  goto if_merge_920;

  if_merge_920:
  r2 = cn_var_词元类型;
  r3 = 是同步点(r2, 1);
  if (r3) goto if_then_921; else goto if_merge_922;

  if_then_921:
  return 1;
  goto if_merge_922;

  if_merge_922:
  r4 = cn_var_词元类型;
  r5 = 是同步点(r4, 2);
  if (r5) goto if_then_923; else goto if_merge_924;

  if_then_923:
  return 1;
  goto if_merge_924;

  if_merge_924:
  r6 = cn_var_词元类型;
  r7 = 是同步点(r6, 3);
  if (r7) goto if_then_925; else goto if_merge_926;

  if_then_925:
  return 1;
  goto if_merge_926;

  if_merge_926:
  r8 = cn_var_词元类型;
  r9 = 是同步点(r8, 4);
  if (r9) goto if_then_927; else goto if_merge_928;

  if_then_927:
  return 1;
  goto if_merge_928;

  if_merge_928:
  r10 = cn_var_词元类型;
  r11 = 是同步点(r10, 5);
  if (r11) goto if_then_929; else goto if_merge_930;

  if_then_929:
  return 1;
  goto if_merge_930;

  if_merge_930:
  r12 = cn_var_词元类型;
  r13 = 是同步点(r12, 6);
  if (r13) goto if_then_931; else goto if_merge_932;

  if_then_931:
  return 1;
  goto if_merge_932;

  if_merge_932:
  r14 = cn_var_词元类型;
  r15 = 是同步点(r14, 7);
  if (r15) goto if_then_933; else goto if_merge_934;

  if_then_933:
  return 1;
  goto if_merge_934;

  if_merge_934:
  return 0;
}

long long 跳转到同步点(struct 语法错误恢复上下文* cn_var_上下文, enum 词元类型枚举 cn_var_当前词元类型, enum 同步点类型 cn_var_目标同步类型) {
  long long r1, r3, r4, r6, r8, r9, r12, r14, r16;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r5;
  struct 语法错误恢复上下文* r7;
  enum 词元类型枚举 r10;
  enum 同步点类型 r11;
  enum 同步点类型 r13;
  enum 词元类型枚举 r15;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_935; else goto if_merge_936;

  if_then_935:
  return 0;
  goto if_merge_936;

  if_merge_936:
  r2 = cn_var_上下文;
  r3 = r2->恢复尝试次数;
  r4 = r3 + 1;
  r5 = cn_var_上下文;
  r6 = r5->恢复尝试次数;
  r7 = cn_var_上下文;
  r8 = r7->最大恢复次数;
  r9 = r6 > r8;
  if (r9) goto if_then_937; else goto if_merge_938;

  if_then_937:
  return 0;
  goto if_merge_938;

  if_merge_938:
  r10 = cn_var_当前词元类型;
  r11 = cn_var_目标同步类型;
  r12 = 是同步点(r10, r11);
  if (r12) goto if_then_939; else goto if_merge_940;

  if_then_939:
  return 1;
  goto if_merge_940;

  if_merge_940:
  r13 = cn_var_目标同步类型;
  r14 = r13 == 0;
  if (r14) goto if_then_941; else goto if_merge_942;

  if_then_941:
  r15 = cn_var_当前词元类型;
  r16 = 是任意同步点(r15);
  if (r16) goto if_then_943; else goto if_merge_944;

  if_merge_942:
  return 0;

  if_then_943:
  return 1;
  goto if_merge_944;

  if_merge_944:
  goto if_merge_942;
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
  if (r1) goto if_then_945; else goto if_merge_946;

  if_then_945:
  return 0;
  goto if_merge_946;

  if_merge_946:
  r2 = cn_var_上下文;
  r3 = r2->抑制级联错误;
  if (r3) goto if_then_947; else goto if_merge_948;

  if_then_947:
  return 1;
  goto if_merge_948;

  if_merge_948:
  r4 = cn_var_上下文;
  r5 = r4->连续错误计数;
  r6 = cn_var_上下文;
  r7 = r6->最大连续错误;
  r8 = r5 >= r7;
  if (r8) goto if_then_949; else goto if_merge_950;

  if_then_949:
  return 1;
  goto if_merge_950;

  if_merge_950:
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
  if (r1) goto if_then_951; else goto if_merge_952;

  if_then_951:
  return;
  goto if_merge_952;

  if_merge_952:
  r2 = cn_var_上下文;
  r3 = r2->最后错误行号;
  r4 = cn_var_行号;
  r5 = r3 == r4;
  if (r5) goto if_then_953; else goto if_else_954;

  if_then_953:
  r6 = cn_var_上下文;
  r7 = r6->级联抑制计数;
  r8 = r7 + 1;
  r9 = cn_var_上下文;
  r10 = r9->级联抑制计数;
  r11 = cn_var_上下文;
  r12 = r11->最大连续错误;
  r13 = r10 >= r12;
  if (r13) goto if_then_956; else goto if_merge_957;

  if_else_954:
  goto if_merge_955;

  if_merge_955:
  r14 = cn_var_上下文;
  r15 = r14->连续错误计数;
  r16 = r15 + 1;
  r17 = cn_var_行号;

  if_then_956:
  goto if_merge_957;

  if_merge_957:
  goto if_merge_955;
  return;
}

void 重置错误计数(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_958; else goto if_merge_959;

  if_then_958:
  return;
  goto if_merge_959;

  if_merge_959:
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
  if (r1) goto case_body_961; else goto switch_check_974;

  switch_check_974:
  r2 = r0 == 102;
  if (r2) goto case_body_962; else goto switch_check_975;

  switch_check_975:
  r3 = r0 == 103;
  if (r3) goto case_body_963; else goto switch_check_976;

  switch_check_976:
  r4 = r0 == 104;
  if (r4) goto case_body_964; else goto switch_check_977;

  switch_check_977:
  r5 = r0 == 105;
  if (r5) goto case_body_965; else goto switch_check_978;

  switch_check_978:
  r6 = r0 == 106;
  if (r6) goto case_body_966; else goto switch_check_979;

  switch_check_979:
  r7 = r0 == 107;
  if (r7) goto case_body_967; else goto switch_check_980;

  switch_check_980:
  r8 = r0 == 108;
  if (r8) goto case_body_968; else goto switch_check_981;

  switch_check_981:
  r9 = r0 == 109;
  if (r9) goto case_body_969; else goto switch_check_982;

  switch_check_982:
  r10 = r0 == 1;
  if (r10) goto case_body_970; else goto switch_check_983;

  switch_check_983:
  r11 = r0 == 2;
  if (r11) goto case_body_971; else goto switch_check_984;

  switch_check_984:
  r12 = r0 == 3;
  if (r12) goto case_body_972; else goto case_default_973;

  case_body_961:
  goto switch_merge_960;

  case_body_962:
  return 2;
  goto switch_merge_960;

  case_body_963:
  goto switch_merge_960;

  case_body_964:
  return 3;
  goto switch_merge_960;

  case_body_965:
  goto switch_merge_960;

  case_body_966:
  goto switch_merge_960;

  case_body_967:
  return 4;
  goto switch_merge_960;

  case_body_968:
  goto switch_merge_960;

  case_body_969:
  return 1;
  goto switch_merge_960;

  case_body_970:
  goto switch_merge_960;

  case_body_971:
  goto switch_merge_960;

  case_body_972:
  return 1;
  goto switch_merge_960;

  case_default_973:
  return 2;
  goto switch_merge_960;

  switch_merge_960:
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
  if (r1) goto case_body_986; else goto switch_check_995;

  switch_check_995:
  r2 = r0 == 1;
  if (r2) goto case_body_987; else goto switch_check_996;

  switch_check_996:
  r3 = r0 == 2;
  if (r3) goto case_body_988; else goto switch_check_997;

  switch_check_997:
  r4 = r0 == 3;
  if (r4) goto case_body_989; else goto switch_check_998;

  switch_check_998:
  r5 = r0 == 4;
  if (r5) goto case_body_990; else goto switch_check_999;

  switch_check_999:
  r6 = r0 == 5;
  if (r6) goto case_body_991; else goto switch_check_1000;

  switch_check_1000:
  r7 = r0 == 6;
  if (r7) goto case_body_992; else goto switch_check_1001;

  switch_check_1001:
  r8 = r0 == 7;
  if (r8) goto case_body_993; else goto case_default_994;

  case_body_986:
  return "分号";
  goto switch_merge_985;

  case_body_987:
  return "右大括号";
  goto switch_merge_985;

  case_body_988:
  return "右小括号";
  goto switch_merge_985;

  case_body_989:
  return "右中括号";
  goto switch_merge_985;

  case_body_990:
  return "逗号";
  goto switch_merge_985;

  case_body_991:
  return "关键字";
  goto switch_merge_985;

  case_body_992:
  return "类型关键字";
  goto switch_merge_985;

  case_body_993:
  return "声明关键字";
  goto switch_merge_985;

  case_default_994:
  return "未知同步点";
  goto switch_merge_985;

  switch_merge_985:
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
  if (r1) goto case_body_1003; else goto switch_check_1009;

  switch_check_1009:
  r2 = r0 == 1;
  if (r2) goto case_body_1004; else goto switch_check_1010;

  switch_check_1010:
  r3 = r0 == 2;
  if (r3) goto case_body_1005; else goto switch_check_1011;

  switch_check_1011:
  r4 = r0 == 3;
  if (r4) goto case_body_1006; else goto switch_check_1012;

  switch_check_1012:
  r5 = r0 == 4;
  if (r5) goto case_body_1007; else goto case_default_1008;

  case_body_1003:
  return "不恢复";
  goto switch_merge_1002;

  case_body_1004:
  return "跳过词元";
  goto switch_merge_1002;

  case_body_1005:
  return "同步到分号";
  goto switch_merge_1002;

  case_body_1006:
  return "同步到大括号";
  goto switch_merge_1002;

  case_body_1007:
  return "同步到关键字";
  goto switch_merge_1002;

  case_default_1008:
  return "未知策略";
  goto switch_merge_1002;

  switch_merge_1002:
  return NULL;
}

