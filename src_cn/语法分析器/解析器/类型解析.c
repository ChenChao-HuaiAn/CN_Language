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
struct 扫描器;
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};
enum 节点类型 {
    节点类型_接口类型 = 43,
    节点类型_类类型 = 42,
    节点类型_枚举类型 = 41,
    节点类型_结构体类型 = 40,
    节点类型_函数类型 = 39,
    节点类型_数组类型 = 38,
    节点类型_指针类型 = 37,
    节点类型_基础类型 = 36,
    节点类型_模板实例化表达式 = 35,
    节点类型_逻辑表达式 = 34,
    节点类型_结构体字面量表达式 = 33,
    节点类型_数组字面量表达式 = 32,
    节点类型_三元表达式 = 31,
    节点类型_赋值表达式 = 30,
    节点类型_数组访问表达式 = 29,
    节点类型_成员访问表达式 = 28,
    节点类型_函数调用表达式 = 27,
    节点类型_标识符表达式 = 26,
    节点类型_字面量表达式 = 25,
    节点类型_一元表达式 = 24,
    节点类型_二元表达式 = 23,
    节点类型_最终语句 = 22,
    节点类型_抛出语句 = 21,
    节点类型_尝试语句 = 20,
    节点类型_选择语句 = 19,
    节点类型_继续语句 = 18,
    节点类型_中断语句 = 17,
    节点类型_返回语句 = 16,
    节点类型_循环语句 = 15,
    节点类型_当语句 = 14,
    节点类型_如果语句 = 13,
    节点类型_块语句 = 12,
    节点类型_表达式语句 = 11,
    节点类型_模板结构体声明 = 10,
    节点类型_模板函数声明 = 9,
    节点类型_接口声明 = 8,
    节点类型_类声明 = 7,
    节点类型_导入声明 = 6,
    节点类型_模块声明 = 5,
    节点类型_枚举声明 = 4,
    节点类型_结构体声明 = 3,
    节点类型_变量声明 = 2,
    节点类型_函数声明 = 1,
    节点类型_程序节点 = 0
};
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct AST节点;
struct AST节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};

// Forward Declarations - 从导入模块

// Global Variables

// Forward Declarations
struct 类型节点* 解析类型(struct 解析器*);
struct 类型节点* 解析基础类型(struct 解析器*);
struct 类型节点* 解析指针类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析数组类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析函数类型(struct 解析器*);
struct 类型节点* 解析类型(struct 解析器*);
struct 类型节点* 解析基础类型(struct 解析器*);
struct 类型节点* 解析指针类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析数组类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析函数类型(struct 解析器*);

struct 类型节点* 解析类型(struct 解析器* cn_var_实例);
struct 类型节点* 解析基础类型(struct 解析器* cn_var_实例);
struct 类型节点* 解析指针类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_基类型);
struct 类型节点* 解析数组类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_元素类型);
struct 类型节点* 解析函数类型(struct 解析器* cn_var_实例);
struct 类型节点* 解析类型(struct 解析器* cn_var_实例) {
  long long r1, r12, r13, r15, r20, r24, r27, r33, r36, r42;
  char* r18;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r6;
  struct 类型节点* r7;
  struct 解析器* r8;
  struct 类型节点* r14;
  struct 解析器* r16;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r25;
  struct 解析器* r29;
  struct 类型节点* r30;
  struct 类型节点* r31;
  struct 类型节点* r32;
  struct 解析器* r34;
  struct 解析器* r38;
  struct 类型节点* r39;
  struct 类型节点* r40;
  struct 类型节点* r41;
  struct 类型节点* r43;
  _Bool r5;
  _Bool r28;
  _Bool r37;
  struct 词元 r3;
  struct 词元 r9;
  struct 词元 r17;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r11;
  enum 节点类型 r19;
  enum 诊断错误码 r23;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r35;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_948; else goto if_merge_949;

  if_then_948:
  return 0;
  goto if_merge_949;

  if_merge_949:
  struct 类型节点* cn_var_结果_0;
  cn_var_结果_0 = 0;
  r2 = cn_var_实例;
  r3 = r2->当前词元;
  r4 = r3.类型;
  r5 = 是否类型关键字(r4);
  if (r5) goto if_then_950; else goto if_else_951;

  if_then_950:
  r6 = cn_var_实例;
  r7 = 解析基础类型(r6);
  cn_var_结果_0 = r7;
  goto if_merge_952;

  if_else_951:
  r8 = cn_var_实例;
  r9 = r8->当前词元;
  r10 = r9.类型;
  r11 = cn_var_词元类型枚举;
  r12 = r11.标识符;
  r13 = r10 == r12;
  if (r13) goto if_then_953; else goto if_else_954;

  if_merge_952:
  goto while_cond_958;

  if_then_953:
  创建类型节点();
  cn_var_结果_0 = /* NONE */;
  r14 = cn_var_结果_0;
  r15 = r14 == 0;
  if (r15) goto if_then_956; else goto if_merge_957;

  if_else_954:
  r22 = cn_var_实例;
  r23 = cn_var_诊断错误码;
  r24 = r23.语法_期望标记;
  报告错误(r22, r24, "期望类型名");
  return 0;
  goto if_merge_955;

  if_merge_955:
  goto if_merge_952;

  if_then_956:
  return 0;
  goto if_merge_957;

  if_merge_957:
  r16 = cn_var_实例;
  r17 = r16->当前词元;
  r18 = r17.值;
  r19 = cn_var_节点类型;
  r20 = r19.标识符类型;
  r21 = cn_var_实例;
  前进词元(r21);
  goto if_merge_955;

  while_cond_958:
  r25 = cn_var_实例;
  r26 = cn_var_词元类型枚举;
  r27 = r26.星号;
  r28 = 检查(r25, r27);
  if (r28) goto while_body_959; else goto while_exit_960;

  while_body_959:
  r29 = cn_var_实例;
  r30 = cn_var_结果_0;
  r31 = 解析指针类型(r29, r30);
  cn_var_结果_0 = r31;
  r32 = cn_var_结果_0;
  r33 = r32 == 0;
  if (r33) goto if_then_961; else goto if_merge_962;

  while_exit_960:
  goto while_cond_963;

  if_then_961:
  return 0;
  goto if_merge_962;

  if_merge_962:
  goto while_cond_958;

  while_cond_963:
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.左方括号;
  r37 = 检查(r34, r36);
  if (r37) goto while_body_964; else goto while_exit_965;

  while_body_964:
  r38 = cn_var_实例;
  r39 = cn_var_结果_0;
  r40 = 解析数组类型(r38, r39);
  cn_var_结果_0 = r40;
  r41 = cn_var_结果_0;
  r42 = r41 == 0;
  if (r42) goto if_then_966; else goto if_merge_967;

  while_exit_965:
  r43 = cn_var_结果_0;
  return r43;

  if_then_966:
  return 0;
  goto if_merge_967;

  if_merge_967:
  goto while_cond_963;
  return NULL;
}

struct 类型节点* 解析基础类型(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r9, r12, r15, r18, r21, r24, r27, r30, r33;
  struct 解析器* r0;
  struct 类型节点* r2;
  struct 解析器* r4;
  struct 解析器* r10;
  struct 解析器* r16;
  struct 解析器* r22;
  struct 解析器* r28;
  struct 类型节点* r34;
  struct 解析器* r35;
  struct 类型节点* r36;
  _Bool r7;
  _Bool r13;
  _Bool r19;
  _Bool r25;
  _Bool r31;
  enum 词元类型枚举 r5;
  enum 节点类型 r8;
  enum 词元类型枚举 r11;
  enum 节点类型 r14;
  enum 词元类型枚举 r17;
  enum 节点类型 r20;
  enum 词元类型枚举 r23;
  enum 节点类型 r26;
  enum 词元类型枚举 r29;
  enum 节点类型 r32;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_968; else goto if_merge_969;

  if_then_968:
  return 0;
  goto if_merge_969;

  if_merge_969:
  struct 类型节点* cn_var_结果_0;
  创建类型节点();
  cn_var_结果_0 = /* NONE */;
  r2 = cn_var_结果_0;
  r3 = r2 == 0;
  if (r3) goto if_then_970; else goto if_merge_971;

  if_then_970:
  return 0;
  goto if_merge_971;

  if_merge_971:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.关键字_整数;
  r7 = 检查(r4, r6);
  if (r7) goto if_then_972; else goto if_else_973;

  if_then_972:
  r8 = cn_var_节点类型;
  r9 = r8.基础类型;
  goto if_merge_974;

  if_else_973:
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.关键字_小数;
  r13 = 检查(r10, r12);
  if (r13) goto if_then_975; else goto if_else_976;

  if_merge_974:
  r35 = cn_var_实例;
  前进词元(r35);
  r36 = cn_var_结果_0;
  return r36;

  if_then_975:
  r14 = cn_var_节点类型;
  r15 = r14.基础类型;
  goto if_merge_977;

  if_else_976:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.关键字_字符串;
  r19 = 检查(r16, r18);
  if (r19) goto if_then_978; else goto if_else_979;

  if_merge_977:
  goto if_merge_974;

  if_then_978:
  r20 = cn_var_节点类型;
  r21 = r20.基础类型;
  goto if_merge_980;

  if_else_979:
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.关键字_布尔;
  r25 = 检查(r22, r24);
  if (r25) goto if_then_981; else goto if_else_982;

  if_merge_980:
  goto if_merge_977;

  if_then_981:
  r26 = cn_var_节点类型;
  r27 = r26.基础类型;
  goto if_merge_983;

  if_else_982:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.关键字_空类型;
  r31 = 检查(r28, r30);
  if (r31) goto if_then_984; else goto if_else_985;

  if_merge_983:
  goto if_merge_980;

  if_then_984:
  r32 = cn_var_节点类型;
  r33 = r32.基础类型;
  goto if_merge_986;

  if_else_985:
  r34 = cn_var_结果_0;
  释放类型节点(r34);
  return 0;
  goto if_merge_986;

  if_merge_986:
  goto if_merge_983;
  return NULL;
}

struct 类型节点* 解析指针类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_基类型) {
  long long r0, r2, r4, r7, r9, r11, r12, r15, r18, r19, r22, r23;
  char* r21;
  struct 解析器* r1;
  struct 类型节点* r3;
  struct 解析器* r5;
  struct 类型节点* r10;
  struct 类型节点* r13;
  struct 类型节点* r16;
  struct 类型节点* r17;
  struct 类型节点* r20;
  _Bool r8;
  enum 词元类型枚举 r6;
  enum 节点类型 r14;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_990; else goto logic_rhs_989;

  if_then_987:
  return 0;
  goto if_merge_988;

  if_merge_988:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.星号;
  r8 = 期望(r5, r7);
  r9 = !r8;
  if (r9) goto if_then_991; else goto if_merge_992;

  logic_rhs_989:
  r3 = cn_var_基类型;
  r4 = r3 == 0;
  goto logic_merge_990;

  logic_merge_990:
  if (r4) goto if_then_987; else goto if_merge_988;

  if_then_991:
  r10 = cn_var_基类型;
  return r10;
  goto if_merge_992;

  if_merge_992:
  struct 类型节点* cn_var_指针类型_0;
  创建类型节点();
  cn_var_指针类型_0 = /* NONE */;
  r11 = cn_var_指针类型_0;
  r12 = r11 == 0;
  if (r12) goto if_then_993; else goto if_merge_994;

  if_then_993:
  r13 = cn_var_基类型;
  return r13;
  goto if_merge_994;

  if_merge_994:
  r14 = cn_var_节点类型;
  r15 = r14.指针类型;
  r16 = cn_var_基类型;
  r17 = cn_var_基类型;
  r18 = r17->指针层级;
  r19 = r18 + 1;
  r20 = cn_var_基类型;
  r21 = r20->名称;
  r22 = r21 + "*";
  r23 = cn_var_指针类型_0;
  return r23;
}

struct 类型节点* 解析数组类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_元素类型) {
  long long r0, r2, r4, r7, r9, r11, r12, r15, r18, r19, r22, r30, r32;
  char* r26;
  struct 解析器* r1;
  struct 类型节点* r3;
  struct 解析器* r5;
  struct 类型节点* r10;
  struct 类型节点* r13;
  struct 类型节点* r16;
  struct 类型节点* r17;
  struct 解析器* r20;
  struct 解析器* r24;
  struct 解析器* r27;
  struct 解析器* r28;
  _Bool r8;
  _Bool r23;
  _Bool r31;
  struct 词元 r25;
  enum 词元类型枚举 r6;
  enum 节点类型 r14;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r29;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_998; else goto logic_rhs_997;

  if_then_995:
  return 0;
  goto if_merge_996;

  if_merge_996:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左方括号;
  r8 = 期望(r5, r7);
  r9 = !r8;
  if (r9) goto if_then_999; else goto if_merge_1000;

  logic_rhs_997:
  r3 = cn_var_元素类型;
  r4 = r3 == 0;
  goto logic_merge_998;

  logic_merge_998:
  if (r4) goto if_then_995; else goto if_merge_996;

  if_then_999:
  r10 = cn_var_元素类型;
  return r10;
  goto if_merge_1000;

  if_merge_1000:
  struct 类型节点* cn_var_数组类型_0;
  创建类型节点();
  cn_var_数组类型_0 = /* NONE */;
  r11 = cn_var_数组类型_0;
  r12 = r11 == 0;
  if (r12) goto if_then_1001; else goto if_merge_1002;

  if_then_1001:
  r13 = cn_var_元素类型;
  return r13;
  goto if_merge_1002;

  if_merge_1002:
  r14 = cn_var_节点类型;
  r15 = r14.数组类型;
  r16 = cn_var_元素类型;
  r17 = cn_var_元素类型;
  r18 = r17->数组维度;
  r19 = r18 + 1;
  r20 = cn_var_实例;
  r21 = cn_var_词元类型枚举;
  r22 = r21.整数字面量;
  r23 = 检查(r20, r22);
  if (r23) goto if_then_1003; else goto if_merge_1004;

  if_then_1003:
  r24 = cn_var_实例;
  r25 = r24->当前词元;
  r26 = r25.值;
  字符串转整数(r26);
  r27 = cn_var_实例;
  前进词元(r27);
  goto if_merge_1004;

  if_merge_1004:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.右方括号;
  r31 = 期望(r28, r30);
  r32 = cn_var_数组类型_0;
  return r32;
}

struct 类型节点* 解析函数类型(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r5, r8, r10, r11, r14, r16, r17, r20, r22, r26, r27, r31, r33, r36, r40, r44;
  struct 解析器* r0;
  struct 解析器* r6;
  struct 解析器* r12;
  struct 解析器* r18;
  struct 解析器* r23;
  struct 类型节点* r24;
  struct 类型节点* r25;
  struct 类型节点* r28;
  struct 解析器* r29;
  struct 解析器* r34;
  struct 解析器* r38;
  struct 解析器* r42;
  struct 类型节点* r43;
  _Bool r9;
  _Bool r15;
  _Bool r21;
  _Bool r32;
  _Bool r37;
  _Bool r41;
  enum 节点类型 r4;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r30;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r39;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1005; else goto if_merge_1006;

  if_then_1005:
  return 0;
  goto if_merge_1006;

  if_merge_1006:
  struct 类型节点* cn_var_函数类型_0;
  创建类型节点();
  cn_var_函数类型_0 = /* NONE */;
  r2 = cn_var_函数类型_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1007; else goto if_merge_1008;

  if_then_1007:
  return 0;
  goto if_merge_1008;

  if_merge_1008:
  r4 = cn_var_节点类型;
  r5 = r4.函数类型;
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.关键字_函数;
  r9 = 期望(r6, r8);
  r10 = !r9;
  if (r10) goto if_then_1009; else goto if_merge_1010;

  if_then_1009:
  r11 = cn_var_函数类型_0;
  释放类型节点(r11);
  return 0;
  goto if_merge_1010;

  if_merge_1010:
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.左括号;
  r15 = 期望(r12, r14);
  r16 = !r15;
  if (r16) goto if_then_1011; else goto if_merge_1012;

  if_then_1011:
  r17 = cn_var_函数类型_0;
  释放类型节点(r17);
  return 0;
  goto if_merge_1012;

  if_merge_1012:
  goto while_cond_1013;

  while_cond_1013:
  r18 = cn_var_实例;
  r19 = cn_var_词元类型枚举;
  r20 = r19.右括号;
  r21 = 检查(r18, r20);
  r22 = !r21;
  if (r22) goto while_body_1014; else goto while_exit_1015;

  while_body_1014:
  struct 类型节点* cn_var_参数类型_1;
  r23 = cn_var_实例;
  r24 = 解析类型(r23);
  cn_var_参数类型_1 = r24;
  r25 = cn_var_参数类型_1;
  r26 = r25 != 0;
  if (r26) goto if_then_1016; else goto if_merge_1017;

  while_exit_1015:
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.右括号;
  r37 = 期望(r34, r36);
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.箭头;
  r41 = 匹配(r38, r40);
  if (r41) goto if_then_1020; else goto if_merge_1021;

  if_then_1016:
  r27 = cn_var_函数类型_0;
  r28 = cn_var_参数类型_1;
  函数类型添加参数(r27, r28);
  goto if_merge_1017;

  if_merge_1017:
  r29 = cn_var_实例;
  r30 = cn_var_词元类型枚举;
  r31 = r30.逗号;
  r32 = 匹配(r29, r31);
  r33 = !r32;
  if (r33) goto if_then_1018; else goto if_merge_1019;

  if_then_1018:
  goto while_exit_1015;
  goto if_merge_1019;

  if_merge_1019:
  goto while_cond_1013;

  if_then_1020:
  r42 = cn_var_实例;
  r43 = 解析类型(r42);
  goto if_merge_1021;

  if_merge_1021:
  r44 = cn_var_函数类型_0;
  return r44;
}

