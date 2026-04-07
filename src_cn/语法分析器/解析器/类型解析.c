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
  long long r1, r11, r13, r25, r32;
  char* r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r6;
  struct 类型节点* r7;
  struct 解析器* r8;
  struct 类型节点* r12;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r18;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 类型节点* r22;
  struct 类型节点* r23;
  struct 类型节点* r24;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 类型节点* r29;
  struct 类型节点* r30;
  struct 类型节点* r31;
  struct 类型节点* r33;
  _Bool r5;
  _Bool r20;
  _Bool r27;
  struct 词元 r3;
  struct 词元 r9;
  struct 词元 r15;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r10;

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
  r11 = r10 == 40;
  if (r11) goto if_then_953; else goto if_else_954;

  if_merge_952:
  goto while_cond_958;

  if_then_953:
  创建类型节点();
  cn_var_结果_0 = /* NONE */;
  r12 = cn_var_结果_0;
  r13 = r12 == 0;
  if (r13) goto if_then_956; else goto if_merge_957;

  if_else_954:
  r18 = cn_var_实例;
  报告错误(r18, 11, "期望类型名");
  return 0;
  goto if_merge_955;

  if_merge_955:
  goto if_merge_952;

  if_then_956:
  return 0;
  goto if_merge_957;

  if_merge_957:
  r14 = cn_var_实例;
  r15 = r14->当前词元;
  r16 = r15.值;
  r17 = cn_var_实例;
  前进词元(r17);
  goto if_merge_955;

  while_cond_958:
  r19 = cn_var_实例;
  r20 = 检查(r19, 47);
  if (r20) goto while_body_959; else goto while_exit_960;

  while_body_959:
  r21 = cn_var_实例;
  r22 = cn_var_结果_0;
  r23 = 解析指针类型(r21, r22);
  cn_var_结果_0 = r23;
  r24 = cn_var_结果_0;
  r25 = r24 == 0;
  if (r25) goto if_then_961; else goto if_merge_962;

  while_exit_960:
  goto while_cond_963;

  if_then_961:
  return 0;
  goto if_merge_962;

  if_merge_962:
  goto while_cond_958;

  while_cond_963:
  r26 = cn_var_实例;
  r27 = 检查(r26, 73);
  if (r27) goto while_body_964; else goto while_exit_965;

  while_body_964:
  r28 = cn_var_实例;
  r29 = cn_var_结果_0;
  r30 = 解析数组类型(r28, r29);
  cn_var_结果_0 = r30;
  r31 = cn_var_结果_0;
  r32 = r31 == 0;
  if (r32) goto if_then_966; else goto if_merge_967;

  while_exit_965:
  r33 = cn_var_结果_0;
  return r33;

  if_then_966:
  return 0;
  goto if_merge_967;

  if_merge_967:
  goto while_cond_963;
  return NULL;
}

struct 类型节点* 解析基础类型(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 类型节点* r2;
  struct 解析器* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 类型节点* r14;
  struct 解析器* r15;
  struct 类型节点* r16;
  _Bool r5;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  _Bool r13;

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
  r5 = 检查(r4, 10);
  if (r5) goto if_then_972; else goto if_else_973;

  if_then_972:
  goto if_merge_974;

  if_else_973:
  r6 = cn_var_实例;
  r7 = 检查(r6, 11);
  if (r7) goto if_then_975; else goto if_else_976;

  if_merge_974:
  r15 = cn_var_实例;
  前进词元(r15);
  r16 = cn_var_结果_0;
  return r16;

  if_then_975:
  goto if_merge_977;

  if_else_976:
  r8 = cn_var_实例;
  r9 = 检查(r8, 12);
  if (r9) goto if_then_978; else goto if_else_979;

  if_merge_977:
  goto if_merge_974;

  if_then_978:
  goto if_merge_980;

  if_else_979:
  r10 = cn_var_实例;
  r11 = 检查(r10, 13);
  if (r11) goto if_then_981; else goto if_else_982;

  if_merge_980:
  goto if_merge_977;

  if_then_981:
  goto if_merge_983;

  if_else_982:
  r12 = cn_var_实例;
  r13 = 检查(r12, 14);
  if (r13) goto if_then_984; else goto if_else_985;

  if_merge_983:
  goto if_merge_980;

  if_then_984:
  goto if_merge_986;

  if_else_985:
  r14 = cn_var_结果_0;
  释放类型节点(r14);
  return 0;
  goto if_merge_986;

  if_merge_986:
  goto if_merge_983;
  return NULL;
}

struct 类型节点* 解析指针类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_基类型) {
  long long r0, r2, r4, r7, r9, r10, r14, r15, r18, r19;
  char* r17;
  struct 解析器* r1;
  struct 类型节点* r3;
  struct 解析器* r5;
  struct 类型节点* r8;
  struct 类型节点* r11;
  struct 类型节点* r12;
  struct 类型节点* r13;
  struct 类型节点* r16;
  _Bool r6;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_990; else goto logic_rhs_989;

  if_then_987:
  return 0;
  goto if_merge_988;

  if_merge_988:
  r5 = cn_var_实例;
  r6 = 期望(r5, 47);
  r7 = !r6;
  if (r7) goto if_then_991; else goto if_merge_992;

  logic_rhs_989:
  r3 = cn_var_基类型;
  r4 = r3 == 0;
  goto logic_merge_990;

  logic_merge_990:
  if (r4) goto if_then_987; else goto if_merge_988;

  if_then_991:
  r8 = cn_var_基类型;
  return r8;
  goto if_merge_992;

  if_merge_992:
  struct 类型节点* cn_var_指针类型_0;
  创建类型节点();
  cn_var_指针类型_0 = /* NONE */;
  r9 = cn_var_指针类型_0;
  r10 = r9 == 0;
  if (r10) goto if_then_993; else goto if_merge_994;

  if_then_993:
  r11 = cn_var_基类型;
  return r11;
  goto if_merge_994;

  if_merge_994:
  r12 = cn_var_基类型;
  r13 = cn_var_基类型;
  r14 = r13->指针层级;
  r15 = r14 + 1;
  r16 = cn_var_基类型;
  r17 = r16->名称;
  r18 = r17 + "*";
  r19 = cn_var_指针类型_0;
  return r19;
}

struct 类型节点* 解析数组类型(struct 解析器* cn_var_实例, struct 类型节点* cn_var_元素类型) {
  long long r0, r2, r4, r7, r9, r10, r14, r15, r24;
  char* r20;
  struct 解析器* r1;
  struct 类型节点* r3;
  struct 解析器* r5;
  struct 类型节点* r8;
  struct 类型节点* r11;
  struct 类型节点* r12;
  struct 类型节点* r13;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 解析器* r21;
  struct 解析器* r22;
  _Bool r6;
  _Bool r17;
  _Bool r23;
  struct 词元 r19;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_998; else goto logic_rhs_997;

  if_then_995:
  return 0;
  goto if_merge_996;

  if_merge_996:
  r5 = cn_var_实例;
  r6 = 期望(r5, 73);
  r7 = !r6;
  if (r7) goto if_then_999; else goto if_merge_1000;

  logic_rhs_997:
  r3 = cn_var_元素类型;
  r4 = r3 == 0;
  goto logic_merge_998;

  logic_merge_998:
  if (r4) goto if_then_995; else goto if_merge_996;

  if_then_999:
  r8 = cn_var_元素类型;
  return r8;
  goto if_merge_1000;

  if_merge_1000:
  struct 类型节点* cn_var_数组类型_0;
  创建类型节点();
  cn_var_数组类型_0 = /* NONE */;
  r9 = cn_var_数组类型_0;
  r10 = r9 == 0;
  if (r10) goto if_then_1001; else goto if_merge_1002;

  if_then_1001:
  r11 = cn_var_元素类型;
  return r11;
  goto if_merge_1002;

  if_merge_1002:
  r12 = cn_var_元素类型;
  r13 = cn_var_元素类型;
  r14 = r13->数组维度;
  r15 = r14 + 1;
  r16 = cn_var_实例;
  r17 = 检查(r16, 41);
  if (r17) goto if_then_1003; else goto if_merge_1004;

  if_then_1003:
  r18 = cn_var_实例;
  r19 = r18->当前词元;
  r20 = r19.值;
  字符串转整数(r20);
  r21 = cn_var_实例;
  前进词元(r21);
  goto if_merge_1004;

  if_merge_1004:
  r22 = cn_var_实例;
  r23 = 期望(r22, 74);
  r24 = cn_var_数组类型_0;
  return r24;
}

struct 类型节点* 解析函数类型(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r6, r7, r10, r11, r14, r18, r19, r23, r30;
  struct 解析器* r0;
  struct 解析器* r4;
  struct 解析器* r8;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 类型节点* r16;
  struct 类型节点* r17;
  struct 类型节点* r20;
  struct 解析器* r21;
  struct 解析器* r24;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 类型节点* r29;
  _Bool r5;
  _Bool r9;
  _Bool r13;
  _Bool r22;
  _Bool r25;
  _Bool r27;

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
  r4 = cn_var_实例;
  r5 = 期望(r4, 17);
  r6 = !r5;
  if (r6) goto if_then_1009; else goto if_merge_1010;

  if_then_1009:
  r7 = cn_var_函数类型_0;
  释放类型节点(r7);
  return 0;
  goto if_merge_1010;

  if_merge_1010:
  r8 = cn_var_实例;
  r9 = 期望(r8, 69);
  r10 = !r9;
  if (r10) goto if_then_1011; else goto if_merge_1012;

  if_then_1011:
  r11 = cn_var_函数类型_0;
  释放类型节点(r11);
  return 0;
  goto if_merge_1012;

  if_merge_1012:
  goto while_cond_1013;

  while_cond_1013:
  r12 = cn_var_实例;
  r13 = 检查(r12, 70);
  r14 = !r13;
  if (r14) goto while_body_1014; else goto while_exit_1015;

  while_body_1014:
  struct 类型节点* cn_var_参数类型_1;
  r15 = cn_var_实例;
  r16 = 解析类型(r15);
  cn_var_参数类型_1 = r16;
  r17 = cn_var_参数类型_1;
  r18 = r17 != 0;
  if (r18) goto if_then_1016; else goto if_merge_1017;

  while_exit_1015:
  r24 = cn_var_实例;
  r25 = 期望(r24, 70);
  r26 = cn_var_实例;
  r27 = 匹配(r26, 68);
  if (r27) goto if_then_1020; else goto if_merge_1021;

  if_then_1016:
  r19 = cn_var_函数类型_0;
  r20 = cn_var_参数类型_1;
  函数类型添加参数(r19, r20);
  goto if_merge_1017;

  if_merge_1017:
  r21 = cn_var_实例;
  r22 = 匹配(r21, 76);
  r23 = !r22;
  if (r23) goto if_then_1018; else goto if_merge_1019;

  if_then_1018:
  goto while_exit_1015;
  goto if_merge_1019;

  if_merge_1019:
  goto while_cond_1013;

  if_then_1020:
  r28 = cn_var_实例;
  r29 = 解析类型(r28);
  goto if_merge_1021;

  if_merge_1021:
  r30 = cn_var_函数类型_0;
  return r30;
}

