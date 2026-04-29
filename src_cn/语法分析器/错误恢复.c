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
  if (r1) goto if_then_945; else goto if_merge_946;

  if_then_945:
  return 0;
  goto if_merge_946;

  if_merge_946:
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
  if (r1) goto if_then_947; else goto if_merge_948;

  if_then_947:
  goto if_merge_948;

  if_merge_948:
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
  r1 = r0 == 同步点类型_同步点_分号;
  if (r1) goto case_body_950; else goto switch_check_959;

  switch_check_959:
  r2 = r0 == 同步点类型_同步点_右大括号;
  if (r2) goto case_body_951; else goto switch_check_960;

  switch_check_960:
  r3 = r0 == 同步点类型_同步点_右小括号;
  if (r3) goto case_body_952; else goto switch_check_961;

  switch_check_961:
  r4 = r0 == 同步点类型_同步点_右中括号;
  if (r4) goto case_body_953; else goto switch_check_962;

  switch_check_962:
  r5 = r0 == 同步点类型_同步点_逗号;
  if (r5) goto case_body_954; else goto switch_check_963;

  switch_check_963:
  r6 = r0 == 同步点类型_同步点_关键字;
  if (r6) goto case_body_955; else goto switch_check_964;

  switch_check_964:
  r7 = r0 == 同步点类型_同步点_类型关键字;
  if (r7) goto case_body_956; else goto switch_check_965;

  switch_check_965:
  r8 = r0 == 同步点类型_同步点_声明关键字;
  if (r8) goto case_body_957; else goto case_default_958;

  case_body_950:
  r9 = cn_var_词元类型;
  r10 = r9 == 1;
  return r10;
  goto switch_merge_949;

  case_body_951:
  r11 = cn_var_词元类型;
  r12 = r11 == 2;
  return r12;
  goto switch_merge_949;

  case_body_952:
  r13 = cn_var_词元类型;
  r14 = r13 == 3;
  return r14;
  goto switch_merge_949;

  case_body_953:
  r15 = cn_var_词元类型;
  r16 = r15 == 4;
  return r16;
  goto switch_merge_949;

  case_body_954:
  r17 = cn_var_词元类型;
  r18 = r17 == 5;
  return r18;
  goto switch_merge_949;

  case_body_955:
  r20 = cn_var_词元类型;
  r21 = r20 >= 100;
  if (r21) goto logic_rhs_966; else goto logic_merge_967;

  logic_rhs_966:
  r22 = cn_var_词元类型;
  r23 = r22 <= 150;
  goto logic_merge_967;

  logic_merge_967:
  return r23;
  goto switch_merge_949;

  case_body_956:
  r25 = cn_var_词元类型;
  r26 = r25 >= 160;
  if (r26) goto logic_rhs_968; else goto logic_merge_969;

  logic_rhs_968:
  r27 = cn_var_词元类型;
  r28 = r27 <= 170;
  goto logic_merge_969;

  logic_merge_969:
  return r28;
  goto switch_merge_949;

  case_body_957:
  r30 = cn_var_词元类型;
  r31 = r30 >= 180;
  if (r31) goto logic_rhs_970; else goto logic_merge_971;

  logic_rhs_970:
  r32 = cn_var_词元类型;
  r33 = r32 <= 190;
  goto logic_merge_971;

  logic_merge_971:
  return r33;
  goto switch_merge_949;

  case_default_958:
  return 0;
  goto switch_merge_949;

  switch_merge_949:
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
  r1 = 是同步点(r0, 同步点类型_同步点_分号);
  if (r1) goto if_then_972; else goto if_merge_973;

  if_then_972:
  return 1;
  goto if_merge_973;

  if_merge_973:
  r2 = cn_var_词元类型;
  r3 = 是同步点(r2, 同步点类型_同步点_右大括号);
  if (r3) goto if_then_974; else goto if_merge_975;

  if_then_974:
  return 1;
  goto if_merge_975;

  if_merge_975:
  r4 = cn_var_词元类型;
  r5 = 是同步点(r4, 同步点类型_同步点_右小括号);
  if (r5) goto if_then_976; else goto if_merge_977;

  if_then_976:
  return 1;
  goto if_merge_977;

  if_merge_977:
  r6 = cn_var_词元类型;
  r7 = 是同步点(r6, 同步点类型_同步点_右中括号);
  if (r7) goto if_then_978; else goto if_merge_979;

  if_then_978:
  return 1;
  goto if_merge_979;

  if_merge_979:
  r8 = cn_var_词元类型;
  r9 = 是同步点(r8, 同步点类型_同步点_逗号);
  if (r9) goto if_then_980; else goto if_merge_981;

  if_then_980:
  return 1;
  goto if_merge_981;

  if_merge_981:
  r10 = cn_var_词元类型;
  r11 = 是同步点(r10, 同步点类型_同步点_关键字);
  if (r11) goto if_then_982; else goto if_merge_983;

  if_then_982:
  return 1;
  goto if_merge_983;

  if_merge_983:
  r12 = cn_var_词元类型;
  r13 = 是同步点(r12, 同步点类型_同步点_类型关键字);
  if (r13) goto if_then_984; else goto if_merge_985;

  if_then_984:
  return 1;
  goto if_merge_985;

  if_merge_985:
  r14 = cn_var_词元类型;
  r15 = 是同步点(r14, 同步点类型_同步点_声明关键字);
  if (r15) goto if_then_986; else goto if_merge_987;

  if_then_986:
  return 1;
  goto if_merge_987;

  if_merge_987:
  return 0;
}

long long 跳转到同步点(struct 语法错误恢复上下文* cn_var_上下文, enum 词元类型枚举 cn_var_当前词元类型, enum 同步点类型 cn_var_目标同步类型) {
  long long r1, r2, r3, r4, r5, r6, r9, r11, r13;
  struct 语法错误恢复上下文* r0;
  enum 词元类型枚举 r7;
  enum 同步点类型 r8;
  enum 同步点类型 r10;
  enum 词元类型枚举 r12;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_988; else goto if_merge_989;

  if_then_988:
  return 0;
  goto if_merge_989;

  if_merge_989:
  r2 = cn_var_上下文->恢复尝试次数;
  r3 = r2 + 1;
  r4 = cn_var_上下文->恢复尝试次数;
  r5 = cn_var_上下文->最大恢复次数;
  r6 = r4 > r5;
  if (r6) goto if_then_990; else goto if_merge_991;

  if_then_990:
  return 0;
  goto if_merge_991;

  if_merge_991:
  r7 = cn_var_当前词元类型;
  r8 = cn_var_目标同步类型;
  r9 = 是同步点(r7, r8);
  if (r9) goto if_then_992; else goto if_merge_993;

  if_then_992:
  return 1;
  goto if_merge_993;

  if_merge_993:
  r10 = cn_var_目标同步类型;
  r11 = r10 == 同步点类型_同步点_分号;
  if (r11) goto if_then_994; else goto if_merge_995;

  if_then_994:
  r12 = cn_var_当前词元类型;
  r13 = 是任意同步点(r12);
  if (r13) goto if_then_996; else goto if_merge_997;

  if_merge_995:
  return 0;

  if_then_996:
  return 1;
  goto if_merge_997;

  if_merge_997:
  goto if_merge_995;
  return 0;
}

_Bool 应该抑制错误(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1, r3, r4, r5;
  struct 语法错误恢复上下文* r0;
  _Bool r2;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_998; else goto if_merge_999;

  if_then_998:
  return 0;
  goto if_merge_999;

  if_merge_999:
  r2 = cn_var_上下文->抑制级联错误;
  if (r2) goto if_then_1000; else goto if_merge_1001;

  if_then_1000:
  return 1;
  goto if_merge_1001;

  if_merge_1001:
  r3 = cn_var_上下文->连续错误计数;
  r4 = cn_var_上下文->最大连续错误;
  r5 = r3 >= r4;
  if (r5) goto if_then_1002; else goto if_merge_1003;

  if_then_1002:
  return 1;
  goto if_merge_1003;

  if_merge_1003:
  return 0;
}

void 记录错误(struct 语法错误恢复上下文* cn_var_上下文, long long cn_var_行号) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_1004; else goto if_merge_1005;

  if_then_1004:
  return;
  goto if_merge_1005;

  if_merge_1005:
  r2 = cn_var_上下文->最后错误行号;
  r3 = cn_var_行号;
  r4 = r2 == r3;
  if (r4) goto if_then_1006; else goto if_else_1007;

  if_then_1006:
  r5 = cn_var_上下文->级联抑制计数;
  r6 = r5 + 1;
  r7 = cn_var_上下文->级联抑制计数;
  r8 = cn_var_上下文->最大连续错误;
  r9 = r7 >= r8;
  if (r9) goto if_then_1009; else goto if_merge_1010;

  if_else_1007:
  goto if_merge_1008;

  if_merge_1008:
  r10 = cn_var_上下文->连续错误计数;
  r11 = r10 + 1;
  r12 = cn_var_行号;

  if_then_1009:
  goto if_merge_1010;

  if_merge_1010:
  goto if_merge_1008;
  return;
}

void 重置错误计数(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_1011; else goto if_merge_1012;

  if_then_1011:
  return;
  goto if_merge_1012;

  if_merge_1012:
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
  if (r1) goto case_body_1014; else goto switch_check_1027;

  switch_check_1027:
  r2 = r0 == 102;
  if (r2) goto case_body_1015; else goto switch_check_1028;

  switch_check_1028:
  r3 = r0 == 103;
  if (r3) goto case_body_1016; else goto switch_check_1029;

  switch_check_1029:
  r4 = r0 == 104;
  if (r4) goto case_body_1017; else goto switch_check_1030;

  switch_check_1030:
  r5 = r0 == 105;
  if (r5) goto case_body_1018; else goto switch_check_1031;

  switch_check_1031:
  r6 = r0 == 106;
  if (r6) goto case_body_1019; else goto switch_check_1032;

  switch_check_1032:
  r7 = r0 == 107;
  if (r7) goto case_body_1020; else goto switch_check_1033;

  switch_check_1033:
  r8 = r0 == 108;
  if (r8) goto case_body_1021; else goto switch_check_1034;

  switch_check_1034:
  r9 = r0 == 109;
  if (r9) goto case_body_1022; else goto switch_check_1035;

  switch_check_1035:
  r10 = r0 == 1;
  if (r10) goto case_body_1023; else goto switch_check_1036;

  switch_check_1036:
  r11 = r0 == 2;
  if (r11) goto case_body_1024; else goto switch_check_1037;

  switch_check_1037:
  r12 = r0 == 3;
  if (r12) goto case_body_1025; else goto case_default_1026;

  case_body_1014:
  goto switch_merge_1013;

  case_body_1015:
  return 语法恢复策略_同步到分号;
  goto switch_merge_1013;

  case_body_1016:
  goto switch_merge_1013;

  case_body_1017:
  return 语法恢复策略_同步到大括号;
  goto switch_merge_1013;

  case_body_1018:
  goto switch_merge_1013;

  case_body_1019:
  goto switch_merge_1013;

  case_body_1020:
  return 语法恢复策略_同步到关键字;
  goto switch_merge_1013;

  case_body_1021:
  goto switch_merge_1013;

  case_body_1022:
  return 语法恢复策略_跳过词元;
  goto switch_merge_1013;

  case_body_1023:
  goto switch_merge_1013;

  case_body_1024:
  goto switch_merge_1013;

  case_body_1025:
  return 语法恢复策略_跳过词元;
  goto switch_merge_1013;

  case_default_1026:
  return 语法恢复策略_同步到分号;
  goto switch_merge_1013;

  switch_merge_1013:
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
  r1 = r0 == 同步点类型_同步点_分号;
  if (r1) goto case_body_1039; else goto switch_check_1048;

  switch_check_1048:
  r2 = r0 == 同步点类型_同步点_右大括号;
  if (r2) goto case_body_1040; else goto switch_check_1049;

  switch_check_1049:
  r3 = r0 == 同步点类型_同步点_右小括号;
  if (r3) goto case_body_1041; else goto switch_check_1050;

  switch_check_1050:
  r4 = r0 == 同步点类型_同步点_右中括号;
  if (r4) goto case_body_1042; else goto switch_check_1051;

  switch_check_1051:
  r5 = r0 == 同步点类型_同步点_逗号;
  if (r5) goto case_body_1043; else goto switch_check_1052;

  switch_check_1052:
  r6 = r0 == 同步点类型_同步点_关键字;
  if (r6) goto case_body_1044; else goto switch_check_1053;

  switch_check_1053:
  r7 = r0 == 同步点类型_同步点_类型关键字;
  if (r7) goto case_body_1045; else goto switch_check_1054;

  switch_check_1054:
  r8 = r0 == 同步点类型_同步点_声明关键字;
  if (r8) goto case_body_1046; else goto case_default_1047;

  case_body_1039:
  return "分号";
  goto switch_merge_1038;

  case_body_1040:
  return "右大括号";
  goto switch_merge_1038;

  case_body_1041:
  return "右小括号";
  goto switch_merge_1038;

  case_body_1042:
  return "右中括号";
  goto switch_merge_1038;

  case_body_1043:
  return "逗号";
  goto switch_merge_1038;

  case_body_1044:
  return "关键字";
  goto switch_merge_1038;

  case_body_1045:
  return "类型关键字";
  goto switch_merge_1038;

  case_body_1046:
  return "声明关键字";
  goto switch_merge_1038;

  case_default_1047:
  return "未知同步点";
  goto switch_merge_1038;

  switch_merge_1038:
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
  r1 = r0 == 语法恢复策略_不恢复;
  if (r1) goto case_body_1056; else goto switch_check_1062;

  switch_check_1062:
  r2 = r0 == 语法恢复策略_跳过词元;
  if (r2) goto case_body_1057; else goto switch_check_1063;

  switch_check_1063:
  r3 = r0 == 语法恢复策略_同步到分号;
  if (r3) goto case_body_1058; else goto switch_check_1064;

  switch_check_1064:
  r4 = r0 == 语法恢复策略_同步到大括号;
  if (r4) goto case_body_1059; else goto switch_check_1065;

  switch_check_1065:
  r5 = r0 == 语法恢复策略_同步到关键字;
  if (r5) goto case_body_1060; else goto case_default_1061;

  case_body_1056:
  return "不恢复";
  goto switch_merge_1055;

  case_body_1057:
  return "跳过词元";
  goto switch_merge_1055;

  case_body_1058:
  return "同步到分号";
  goto switch_merge_1055;

  case_body_1059:
  return "同步到大括号";
  goto switch_merge_1055;

  case_body_1060:
  return "同步到关键字";
  goto switch_merge_1055;

  case_default_1061:
  return "未知策略";
  goto switch_merge_1055;

  switch_merge_1055:
  return NULL;
}

