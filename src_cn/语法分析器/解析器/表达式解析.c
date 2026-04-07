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
struct 表达式节点* 解析表达式(struct 解析器*);
struct 表达式节点* 解析赋值表达式(struct 解析器*);
struct 表达式节点* 解析三元表达式(struct 解析器*);
struct 表达式节点* 解析逻辑或表达式(struct 解析器*);
struct 表达式节点* 解析逻辑与表达式(struct 解析器*);
struct 表达式节点* 解析按位或表达式(struct 解析器*);
struct 表达式节点* 解析按位异或表达式(struct 解析器*);
struct 表达式节点* 解析按位与表达式(struct 解析器*);
struct 表达式节点* 解析相等表达式(struct 解析器*);
struct 表达式节点* 解析比较表达式(struct 解析器*);
struct 表达式节点* 解析位移表达式(struct 解析器*);
struct 表达式节点* 解析加减表达式(struct 解析器*);
struct 表达式节点* 解析乘除表达式(struct 解析器*);
struct 表达式节点* 解析一元表达式(struct 解析器*);
struct 表达式节点* 解析后缀表达式(struct 解析器*);
struct 表达式节点* 解析基础表达式(struct 解析器*);
struct 表达式节点* 解析字面量(struct 解析器*);
struct 表达式节点* 解析标识符表达式(struct 解析器*);
struct 表达式节点* 解析括号表达式(struct 解析器*);
struct 表达式列表* 解析函数调用参数(struct 解析器*);
struct 表达式节点* 解析表达式(struct 解析器*);
struct 表达式节点* 解析赋值表达式(struct 解析器*);
struct 表达式节点* 解析三元表达式(struct 解析器*);
struct 表达式节点* 解析逻辑或表达式(struct 解析器*);
struct 表达式节点* 解析逻辑与表达式(struct 解析器*);
struct 表达式节点* 解析按位或表达式(struct 解析器*);
struct 表达式节点* 解析按位异或表达式(struct 解析器*);
struct 表达式节点* 解析按位与表达式(struct 解析器*);
struct 表达式节点* 解析相等表达式(struct 解析器*);
struct 表达式节点* 解析比较表达式(struct 解析器*);
struct 表达式节点* 解析位移表达式(struct 解析器*);
struct 表达式节点* 解析加减表达式(struct 解析器*);
struct 表达式节点* 解析乘除表达式(struct 解析器*);
struct 表达式节点* 解析一元表达式(struct 解析器*);
struct 表达式节点* 解析后缀表达式(struct 解析器*);
struct 表达式节点* 解析基础表达式(struct 解析器*);
struct 表达式节点* 解析字面量(struct 解析器*);
struct 表达式节点* 解析标识符表达式(struct 解析器*);
struct 表达式节点* 解析括号表达式(struct 解析器*);
struct 表达式列表* 解析函数调用参数(struct 解析器*);

struct 表达式节点* 解析表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析赋值表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例);
struct 表达式节点* 解析标识符表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析括号表达式(struct 解析器* cn_var_实例);
struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例);
struct 表达式节点* 解析表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;

  entry:
  r0 = cn_var_实例;
  解析赋值表达式(r0);
  return;
}

struct 表达式节点* 解析赋值表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 赋值运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1022; else goto if_merge_1023;

  if_then_1022:
  return 0;
  goto if_merge_1023;

  if_merge_1023:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析三元表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1024; else goto if_merge_1025;

  if_then_1024:
  return 0;
  goto if_merge_1025;

  if_merge_1025:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.赋值;
  检查(r5, r7);
  goto if_then_1026;

  if_then_1026:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析赋值表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_赋值节点_2;
  创建赋值表达式();
  cn_var_赋值节点_2 = /* NONE */;
  r10 = cn_var_赋值节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1028; else goto if_merge_1029;

  if_merge_1027:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1028:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1029;

  if_merge_1029:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_赋值运算符;
  r16 = r15.简单赋值;
  r17 = cn_var_赋值节点_2;
  return r17;
  goto if_merge_1027;
  return NULL;
}

struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r14;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 解析器* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1030; else goto if_merge_1031;

  if_then_1030:
  return 0;
  goto if_merge_1031;

  if_merge_1031:
  struct 表达式节点* cn_var_条件_0;
  r2 = cn_var_实例;
  解析逻辑或表达式(r2);
  cn_var_条件_0 = /* NONE */;
  r3 = cn_var_条件_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1032; else goto if_merge_1033;

  if_then_1032:
  return 0;
  goto if_merge_1033;

  if_merge_1033:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.问号;
  匹配(r5, r7);
  goto if_then_1034;

  if_then_1034:
  struct 表达式节点* cn_var_真值_1;
  r8 = cn_var_实例;
  解析表达式(r8);
  cn_var_真值_1 = /* NONE */;
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.冒号;
  期望(r9, r11);
  struct 表达式节点* cn_var_假值_2;
  r12 = cn_var_实例;
  解析三元表达式(r12);
  cn_var_假值_2 = /* NONE */;
  struct 表达式节点* cn_var_三元节点_3;
  创建三元表达式();
  cn_var_三元节点_3 = /* NONE */;
  r13 = cn_var_三元节点_3;
  r14 = r13 == 0;
  if (r14) goto if_then_1036; else goto if_merge_1037;

  if_merge_1035:
  r20 = cn_var_条件_0;
  return r20;

  if_then_1036:
  r15 = cn_var_条件_0;
  return r15;
  goto if_merge_1037;

  if_merge_1037:
  r16 = cn_var_条件_0;
  r17 = cn_var_真值_1;
  r18 = cn_var_假值_2;
  r19 = cn_var_三元节点_3;
  return r19;
  goto if_merge_1035;
  return NULL;
}

struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 二元运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1038; else goto if_merge_1039;

  if_then_1038:
  return 0;
  goto if_merge_1039;

  if_merge_1039:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析逻辑与表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1040; else goto if_merge_1041;

  if_then_1040:
  return 0;
  goto if_merge_1041;

  if_merge_1041:
  goto while_cond_1042;

  while_cond_1042:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.逻辑或;
  检查(r5, r7);
  goto while_body_1043;

  while_body_1043:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析逻辑与表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r10 = cn_var_二元节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1045; else goto if_merge_1046;

  while_exit_1044:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1045:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1046;

  if_merge_1046:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_二元运算符;
  r16 = r15.逻辑或;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_1042;
  return NULL;
}

struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 二元运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1047; else goto if_merge_1048;

  if_then_1047:
  return 0;
  goto if_merge_1048;

  if_merge_1048:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析按位或表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1049; else goto if_merge_1050;

  if_then_1049:
  return 0;
  goto if_merge_1050;

  if_merge_1050:
  goto while_cond_1051;

  while_cond_1051:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.逻辑与;
  检查(r5, r7);
  goto while_body_1052;

  while_body_1052:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析按位或表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r10 = cn_var_二元节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1054; else goto if_merge_1055;

  while_exit_1053:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1054:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1055;

  if_merge_1055:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_二元运算符;
  r16 = r15.逻辑与;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_1051;
  return NULL;
}

struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 二元运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1056; else goto if_merge_1057;

  if_then_1056:
  return 0;
  goto if_merge_1057;

  if_merge_1057:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析按位异或表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1058; else goto if_merge_1059;

  if_then_1058:
  return 0;
  goto if_merge_1059;

  if_merge_1059:
  goto while_cond_1060;

  while_cond_1060:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.按位或;
  检查(r5, r7);
  goto while_body_1061;

  while_body_1061:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析按位异或表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r10 = cn_var_二元节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1063; else goto if_merge_1064;

  while_exit_1062:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1063:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1064;

  if_merge_1064:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_二元运算符;
  r16 = r15.按位或;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_1060;
  return NULL;
}

struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 二元运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1065; else goto if_merge_1066;

  if_then_1065:
  return 0;
  goto if_merge_1066;

  if_merge_1066:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析按位与表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1067; else goto if_merge_1068;

  if_then_1067:
  return 0;
  goto if_merge_1068;

  if_merge_1068:
  goto while_cond_1069;

  while_cond_1069:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.按位异或;
  检查(r5, r7);
  goto while_body_1070;

  while_body_1070:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析按位与表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r10 = cn_var_二元节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1072; else goto if_merge_1073;

  while_exit_1071:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1072:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1073;

  if_merge_1073:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_二元运算符;
  r16 = r15.按位异或;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_1069;
  return NULL;
}

struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r11, r16;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  enum 词元类型枚举 r6;
  enum 二元运算符 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1074; else goto if_merge_1075;

  if_then_1074:
  return 0;
  goto if_merge_1075;

  if_merge_1075:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析相等表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1076; else goto if_merge_1077;

  if_then_1076:
  return 0;
  goto if_merge_1077;

  if_merge_1077:
  goto while_cond_1078;

  while_cond_1078:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.按位与;
  检查(r5, r7);
  goto while_body_1079;

  while_body_1079:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  解析相等表达式(r9);
  cn_var_右值_1 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r10 = cn_var_二元节点_2;
  r11 = r10 == 0;
  if (r11) goto if_then_1081; else goto if_merge_1082;

  while_exit_1080:
  r18 = cn_var_左值_0;
  return r18;

  if_then_1081:
  r12 = cn_var_左值_0;
  return r12;
  goto if_merge_1082;

  if_merge_1082:
  r13 = cn_var_左值_0;
  r14 = cn_var_右值_1;
  r15 = cn_var_二元运算符;
  r16 = r15.按位与;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_1078;
  return NULL;
}

struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r5, r8, r11, r13, r16, r18, r22;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r6;
  struct 解析器* r9;
  struct 解析器* r14;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct 二元运算符 r26;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 二元运算符 r12;
  enum 词元类型枚举 r15;
  enum 二元运算符 r17;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1083; else goto if_merge_1084;

  if_then_1083:
  return 0;
  goto if_merge_1084;

  if_merge_1084:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析比较表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1085; else goto if_merge_1086;

  if_then_1085:
  return 0;
  goto if_merge_1086;

  if_merge_1086:
  goto while_cond_1087;

  while_cond_1087:
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.等于;
  检查(r6, r8);
  goto logic_merge_1091;

  while_body_1088:
  struct 二元运算符 cn_var_运算符_1;
  r12 = cn_var_二元运算符;
  r13 = r12.等于;
  cn_var_运算符_1 = r13;
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.不等于;
  检查(r14, r16);
  goto if_then_1092;

  while_exit_1089:
  r28 = cn_var_左值_0;
  return r28;

  logic_rhs_1090:
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.不等于;
  检查(r9, r11);
  goto logic_merge_1091;

  logic_merge_1091:
  goto while_body_1088;

  if_then_1092:
  r17 = cn_var_二元运算符;
  r18 = r17.不等于;
  cn_var_运算符_1 = r18;
  goto if_merge_1093;

  if_merge_1093:
  r19 = cn_var_实例;
  前进词元(r19);
  struct 表达式节点* cn_var_右值_2;
  r20 = cn_var_实例;
  解析比较表达式(r20);
  cn_var_右值_2 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r21 = cn_var_二元节点_3;
  r22 = r21 == 0;
  if (r22) goto if_then_1094; else goto if_merge_1095;

  if_then_1094:
  r23 = cn_var_左值_0;
  return r23;
  goto if_merge_1095;

  if_merge_1095:
  r24 = cn_var_左值_0;
  r25 = cn_var_右值_2;
  r26 = cn_var_运算符_1;
  r27 = cn_var_二元节点_3;
  cn_var_左值_0 = r27;
  goto while_cond_1087;
  return NULL;
}

struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r5, r6, r7, r10, r13, r16, r19, r21, r24, r26, r29, r31, r34, r36, r40;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r22;
  struct 解析器* r27;
  struct 解析器* r32;
  struct 解析器* r37;
  struct 解析器* r38;
  struct 表达式节点* r39;
  struct 表达式节点* r41;
  struct 表达式节点* r42;
  struct 表达式节点* r43;
  struct 表达式节点* r45;
  struct 表达式节点* r46;
  struct 二元运算符 r44;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r18;
  enum 二元运算符 r20;
  enum 词元类型枚举 r23;
  enum 二元运算符 r25;
  enum 词元类型枚举 r28;
  enum 二元运算符 r30;
  enum 词元类型枚举 r33;
  enum 二元运算符 r35;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1096; else goto if_merge_1097;

  if_then_1096:
  return 0;
  goto if_merge_1097;

  if_merge_1097:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析位移表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1098; else goto if_merge_1099;

  if_then_1098:
  return 0;
  goto if_merge_1099;

  if_merge_1099:
  goto while_cond_1100;

  while_cond_1100:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.小于;
  检查(r8, r10);
  goto logic_merge_1108;

  while_body_1101:
  struct 二元运算符 cn_var_运算符_1;
  r20 = cn_var_二元运算符;
  r21 = r20.小于;
  cn_var_运算符_1 = r21;
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.小于等于;
  检查(r22, r24);
  goto if_then_1109;

  while_exit_1102:
  r46 = cn_var_左值_0;
  return r46;

  logic_rhs_1103:
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.大于等于;
  检查(r17, r19);
  goto logic_merge_1104;

  logic_merge_1104:
  goto while_body_1101;

  logic_rhs_1105:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.大于;
  检查(r14, r16);
  goto logic_merge_1106;

  logic_merge_1106:
  goto logic_merge_1104;

  logic_rhs_1107:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.小于等于;
  检查(r11, r13);
  goto logic_merge_1108;

  logic_merge_1108:
  goto logic_merge_1106;

  if_then_1109:
  r25 = cn_var_二元运算符;
  r26 = r25.小于等于;
  cn_var_运算符_1 = r26;
  goto if_merge_1111;

  if_else_1110:
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.大于;
  检查(r27, r29);
  goto if_then_1112;

  if_merge_1111:
  r37 = cn_var_实例;
  前进词元(r37);
  struct 表达式节点* cn_var_右值_2;
  r38 = cn_var_实例;
  解析位移表达式(r38);
  cn_var_右值_2 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r39 = cn_var_二元节点_3;
  r40 = r39 == 0;
  if (r40) goto if_then_1117; else goto if_merge_1118;

  if_then_1112:
  r30 = cn_var_二元运算符;
  r31 = r30.大于;
  cn_var_运算符_1 = r31;
  goto if_merge_1114;

  if_else_1113:
  r32 = cn_var_实例;
  r33 = cn_var_词元类型枚举;
  r34 = r33.大于等于;
  检查(r32, r34);
  goto if_then_1115;

  if_merge_1114:
  goto if_merge_1111;

  if_then_1115:
  r35 = cn_var_二元运算符;
  r36 = r35.大于等于;
  cn_var_运算符_1 = r36;
  goto if_merge_1116;

  if_merge_1116:
  goto if_merge_1114;

  if_then_1117:
  r41 = cn_var_左值_0;
  return r41;
  goto if_merge_1118;

  if_merge_1118:
  r42 = cn_var_左值_0;
  r43 = cn_var_右值_2;
  r44 = cn_var_运算符_1;
  r45 = cn_var_二元节点_3;
  cn_var_左值_0 = r45;
  goto while_cond_1100;
  return NULL;
}

struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r5, r8, r11, r13, r16, r18, r22;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r6;
  struct 解析器* r9;
  struct 解析器* r14;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct 二元运算符 r26;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 二元运算符 r12;
  enum 词元类型枚举 r15;
  enum 二元运算符 r17;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1119; else goto if_merge_1120;

  if_then_1119:
  return 0;
  goto if_merge_1120;

  if_merge_1120:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析加减表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1121; else goto if_merge_1122;

  if_then_1121:
  return 0;
  goto if_merge_1122;

  if_merge_1122:
  goto while_cond_1123;

  while_cond_1123:
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.左移;
  检查(r6, r8);
  goto logic_merge_1127;

  while_body_1124:
  struct 二元运算符 cn_var_运算符_1;
  r12 = cn_var_二元运算符;
  r13 = r12.左移;
  cn_var_运算符_1 = r13;
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.右移;
  检查(r14, r16);
  goto if_then_1128;

  while_exit_1125:
  r28 = cn_var_左值_0;
  return r28;

  logic_rhs_1126:
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.右移;
  检查(r9, r11);
  goto logic_merge_1127;

  logic_merge_1127:
  goto while_body_1124;

  if_then_1128:
  r17 = cn_var_二元运算符;
  r18 = r17.右移;
  cn_var_运算符_1 = r18;
  goto if_merge_1129;

  if_merge_1129:
  r19 = cn_var_实例;
  前进词元(r19);
  struct 表达式节点* cn_var_右值_2;
  r20 = cn_var_实例;
  解析加减表达式(r20);
  cn_var_右值_2 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r21 = cn_var_二元节点_3;
  r22 = r21 == 0;
  if (r22) goto if_then_1130; else goto if_merge_1131;

  if_then_1130:
  r23 = cn_var_左值_0;
  return r23;
  goto if_merge_1131;

  if_merge_1131:
  r24 = cn_var_左值_0;
  r25 = cn_var_右值_2;
  r26 = cn_var_运算符_1;
  r27 = cn_var_二元节点_3;
  cn_var_左值_0 = r27;
  goto while_cond_1123;
  return NULL;
}

struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r5, r8, r11, r13, r16, r18, r22;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r6;
  struct 解析器* r9;
  struct 解析器* r14;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct 二元运算符 r26;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 二元运算符 r12;
  enum 词元类型枚举 r15;
  enum 二元运算符 r17;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1132; else goto if_merge_1133;

  if_then_1132:
  return 0;
  goto if_merge_1133;

  if_merge_1133:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析乘除表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1134; else goto if_merge_1135;

  if_then_1134:
  return 0;
  goto if_merge_1135;

  if_merge_1135:
  goto while_cond_1136;

  while_cond_1136:
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.加号;
  检查(r6, r8);
  goto logic_merge_1140;

  while_body_1137:
  struct 二元运算符 cn_var_运算符_1;
  r12 = cn_var_二元运算符;
  r13 = r12.加法;
  cn_var_运算符_1 = r13;
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.减号;
  检查(r14, r16);
  goto if_then_1141;

  while_exit_1138:
  r28 = cn_var_左值_0;
  return r28;

  logic_rhs_1139:
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.减号;
  检查(r9, r11);
  goto logic_merge_1140;

  logic_merge_1140:
  goto while_body_1137;

  if_then_1141:
  r17 = cn_var_二元运算符;
  r18 = r17.减法;
  cn_var_运算符_1 = r18;
  goto if_merge_1142;

  if_merge_1142:
  r19 = cn_var_实例;
  前进词元(r19);
  struct 表达式节点* cn_var_右值_2;
  r20 = cn_var_实例;
  解析乘除表达式(r20);
  cn_var_右值_2 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r21 = cn_var_二元节点_3;
  r22 = r21 == 0;
  if (r22) goto if_then_1143; else goto if_merge_1144;

  if_then_1143:
  r23 = cn_var_左值_0;
  return r23;
  goto if_merge_1144;

  if_merge_1144:
  r24 = cn_var_左值_0;
  r25 = cn_var_右值_2;
  r26 = cn_var_运算符_1;
  r27 = cn_var_二元节点_3;
  cn_var_左值_0 = r27;
  goto while_cond_1136;
  return NULL;
}

struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r5, r6, r9, r12, r15, r17, r20, r22, r25, r27, r31;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r7;
  struct 解析器* r10;
  struct 解析器* r13;
  struct 解析器* r18;
  struct 解析器* r23;
  struct 解析器* r28;
  struct 解析器* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r32;
  struct 表达式节点* r33;
  struct 表达式节点* r34;
  struct 表达式节点* r36;
  struct 表达式节点* r37;
  struct 二元运算符 r35;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r14;
  enum 二元运算符 r16;
  enum 词元类型枚举 r19;
  enum 二元运算符 r21;
  enum 词元类型枚举 r24;
  enum 二元运算符 r26;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1145; else goto if_merge_1146;

  if_then_1145:
  return 0;
  goto if_merge_1146;

  if_merge_1146:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  解析一元表达式(r2);
  cn_var_左值_0 = /* NONE */;
  r3 = cn_var_左值_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1147; else goto if_merge_1148;

  if_then_1147:
  return 0;
  goto if_merge_1148;

  if_merge_1148:
  goto while_cond_1149;

  while_cond_1149:
  r7 = cn_var_实例;
  r8 = cn_var_词元类型枚举;
  r9 = r8.星号;
  检查(r7, r9);
  goto logic_merge_1155;

  while_body_1150:
  struct 二元运算符 cn_var_运算符_1;
  r16 = cn_var_二元运算符;
  r17 = r16.乘法;
  cn_var_运算符_1 = r17;
  r18 = cn_var_实例;
  r19 = cn_var_词元类型枚举;
  r20 = r19.斜杠;
  检查(r18, r20);
  goto if_then_1156;

  while_exit_1151:
  r37 = cn_var_左值_0;
  return r37;

  logic_rhs_1152:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.百分号;
  检查(r13, r15);
  goto logic_merge_1153;

  logic_merge_1153:
  goto while_body_1150;

  logic_rhs_1154:
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.斜杠;
  检查(r10, r12);
  goto logic_merge_1155;

  logic_merge_1155:
  goto logic_merge_1153;

  if_then_1156:
  r21 = cn_var_二元运算符;
  r22 = r21.除法;
  cn_var_运算符_1 = r22;
  goto if_merge_1158;

  if_else_1157:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.百分号;
  检查(r23, r25);
  goto if_then_1159;

  if_merge_1158:
  r28 = cn_var_实例;
  前进词元(r28);
  struct 表达式节点* cn_var_右值_2;
  r29 = cn_var_实例;
  解析一元表达式(r29);
  cn_var_右值_2 = /* NONE */;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r30 = cn_var_二元节点_3;
  r31 = r30 == 0;
  if (r31) goto if_then_1161; else goto if_merge_1162;

  if_then_1159:
  r26 = cn_var_二元运算符;
  r27 = r26.取模;
  cn_var_运算符_1 = r27;
  goto if_merge_1160;

  if_merge_1160:
  goto if_merge_1158;

  if_then_1161:
  r32 = cn_var_左值_0;
  return r32;
  goto if_merge_1162;

  if_merge_1162:
  r33 = cn_var_左值_0;
  r34 = cn_var_右值_2;
  r35 = cn_var_运算符_1;
  r36 = cn_var_二元节点_3;
  cn_var_左值_0 = r36;
  goto while_cond_1149;
  return NULL;
}

struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r5, r6, r7, r10, r13, r16, r19, r22, r25, r28, r30, r33, r35, r38, r40, r43, r45, r48, r50, r53, r55, r58, r60, r64;
  struct 解析器* r0;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 解析器* r31;
  struct 解析器* r36;
  struct 解析器* r41;
  struct 解析器* r46;
  struct 解析器* r51;
  struct 解析器* r56;
  struct 解析器* r61;
  struct 解析器* r62;
  struct 表达式节点* r63;
  struct 表达式节点* r65;
  struct 表达式节点* r66;
  struct 表达式节点* r68;
  struct 解析器* r69;
  struct 一元运算符 r67;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r27;
  enum 一元运算符 r29;
  enum 词元类型枚举 r32;
  enum 一元运算符 r34;
  enum 词元类型枚举 r37;
  enum 一元运算符 r39;
  enum 词元类型枚举 r42;
  enum 一元运算符 r44;
  enum 词元类型枚举 r47;
  enum 一元运算符 r49;
  enum 词元类型枚举 r52;
  enum 一元运算符 r54;
  enum 词元类型枚举 r57;
  enum 一元运算符 r59;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1163; else goto if_merge_1164;

  if_then_1163:
  return 0;
  goto if_merge_1164;

  if_merge_1164:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.逻辑非;
  检查(r8, r10);
  goto logic_merge_1178;

  if_then_1165:
  struct 一元运算符 cn_var_运算符_0;
  r29 = cn_var_一元运算符;
  r30 = r29.逻辑非;
  cn_var_运算符_0 = r30;
  r31 = cn_var_实例;
  r32 = cn_var_词元类型枚举;
  r33 = r32.减号;
  检查(r31, r33);
  goto if_then_1179;

  if_merge_1166:
  r69 = cn_var_实例;
  解析后缀表达式(r69);
  return;

  logic_rhs_1167:
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.自减;
  检查(r26, r28);
  goto logic_merge_1168;

  logic_merge_1168:
  goto if_then_1165;

  logic_rhs_1169:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.自增;
  检查(r23, r25);
  goto logic_merge_1170;

  logic_merge_1170:
  goto logic_merge_1168;

  logic_rhs_1171:
  r20 = cn_var_实例;
  r21 = cn_var_词元类型枚举;
  r22 = r21.按位与;
  检查(r20, r22);
  goto logic_merge_1172;

  logic_merge_1172:
  goto logic_merge_1170;

  logic_rhs_1173:
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.星号;
  检查(r17, r19);
  goto logic_merge_1174;

  logic_merge_1174:
  goto logic_merge_1172;

  logic_rhs_1175:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.按位取反;
  检查(r14, r16);
  goto logic_merge_1176;

  logic_merge_1176:
  goto logic_merge_1174;

  logic_rhs_1177:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.减号;
  检查(r11, r13);
  goto logic_merge_1178;

  logic_merge_1178:
  goto logic_merge_1176;

  if_then_1179:
  r34 = cn_var_一元运算符;
  r35 = r34.负号;
  cn_var_运算符_0 = r35;
  goto if_merge_1181;

  if_else_1180:
  r36 = cn_var_实例;
  r37 = cn_var_词元类型枚举;
  r38 = r37.按位取反;
  检查(r36, r38);
  goto if_then_1182;

  if_merge_1181:
  r61 = cn_var_实例;
  前进词元(r61);
  struct 表达式节点* cn_var_操作数_1;
  r62 = cn_var_实例;
  解析一元表达式(r62);
  cn_var_操作数_1 = /* NONE */;
  struct 表达式节点* cn_var_一元节点_2;
  创建一元表达式();
  cn_var_一元节点_2 = /* NONE */;
  r63 = cn_var_一元节点_2;
  r64 = r63 == 0;
  if (r64) goto if_then_1196; else goto if_merge_1197;

  if_then_1182:
  r39 = cn_var_一元运算符;
  r40 = r39.按位取反;
  cn_var_运算符_0 = r40;
  goto if_merge_1184;

  if_else_1183:
  r41 = cn_var_实例;
  r42 = cn_var_词元类型枚举;
  r43 = r42.星号;
  检查(r41, r43);
  goto if_then_1185;

  if_merge_1184:
  goto if_merge_1181;

  if_then_1185:
  r44 = cn_var_一元运算符;
  r45 = r44.解引用;
  cn_var_运算符_0 = r45;
  goto if_merge_1187;

  if_else_1186:
  r46 = cn_var_实例;
  r47 = cn_var_词元类型枚举;
  r48 = r47.按位与;
  检查(r46, r48);
  goto if_then_1188;

  if_merge_1187:
  goto if_merge_1184;

  if_then_1188:
  r49 = cn_var_一元运算符;
  r50 = r49.取地址;
  cn_var_运算符_0 = r50;
  goto if_merge_1190;

  if_else_1189:
  r51 = cn_var_实例;
  r52 = cn_var_词元类型枚举;
  r53 = r52.自增;
  检查(r51, r53);
  goto if_then_1191;

  if_merge_1190:
  goto if_merge_1187;

  if_then_1191:
  r54 = cn_var_一元运算符;
  r55 = r54.前缀自增;
  cn_var_运算符_0 = r55;
  goto if_merge_1193;

  if_else_1192:
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.自减;
  检查(r56, r58);
  goto if_then_1194;

  if_merge_1193:
  goto if_merge_1190;

  if_then_1194:
  r59 = cn_var_一元运算符;
  r60 = r59.前缀自减;
  cn_var_运算符_0 = r60;
  goto if_merge_1195;

  if_merge_1195:
  goto if_merge_1193;

  if_then_1196:
  r65 = cn_var_操作数_1;
  return r65;
  goto if_merge_1197;

  if_merge_1197:
  r66 = cn_var_操作数_1;
  r67 = cn_var_运算符_0;
  r68 = cn_var_一元节点_2;
  return r68;
  goto if_merge_1166;
  return NULL;
}

struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r12, r14, r21, r28, r30, r37, r44, r46, r53, r58, r60, r67, r70, r74, r78, r81, r85;
  char* r25;
  char* r33;
  char* r41;
  char* r49;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 解析器* r10;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r18;
  struct 解析器* r19;
  struct 解析器* r22;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 表达式节点* r29;
  struct 表达式节点* r31;
  struct 表达式节点* r32;
  struct 表达式节点* r34;
  struct 解析器* r35;
  struct 解析器* r38;
  struct 解析器* r39;
  struct 解析器* r42;
  struct 表达式节点* r45;
  struct 表达式节点* r47;
  struct 表达式节点* r48;
  struct 表达式节点* r50;
  struct 解析器* r51;
  struct 解析器* r54;
  struct 解析器* r55;
  struct 解析器* r56;
  struct 表达式节点* r59;
  struct 表达式节点* r61;
  struct 表达式节点* r62;
  struct 表达式节点* r63;
  struct 表达式节点* r64;
  struct 解析器* r65;
  struct 解析器* r68;
  struct 表达式节点* r69;
  struct 表达式节点* r71;
  struct 表达式节点* r72;
  struct 表达式节点* r75;
  struct 解析器* r76;
  struct 解析器* r79;
  struct 表达式节点* r80;
  struct 表达式节点* r82;
  struct 表达式节点* r83;
  struct 表达式节点* r86;
  struct 表达式节点* r87;
  struct 参数 r17;
  struct 词元 r24;
  struct 词元 r40;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r36;
  enum 词元类型枚举 r43;
  enum 词元类型枚举 r52;
  enum 词元类型枚举 r57;
  enum 词元类型枚举 r66;
  enum 一元运算符 r73;
  enum 词元类型枚举 r77;
  enum 一元运算符 r84;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1198; else goto if_merge_1199;

  if_then_1198:
  return 0;
  goto if_merge_1199;

  if_merge_1199:
  struct 表达式节点* cn_var_表达式_0;
  r2 = cn_var_实例;
  解析基础表达式(r2);
  cn_var_表达式_0 = /* NONE */;
  r3 = cn_var_表达式_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1200; else goto if_merge_1201;

  if_then_1200:
  return 0;
  goto if_merge_1201;

  if_merge_1201:
  goto while_cond_1202;

  while_cond_1202:
  if (1) goto while_body_1203; else goto while_exit_1204;

  while_body_1203:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左括号;
  检查(r5, r7);
  goto if_then_1205;

  while_exit_1204:
  r87 = cn_var_表达式_0;
  return r87;

  if_then_1205:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式列表* cn_var_参数_1;
  r9 = cn_var_实例;
  解析函数调用参数(r9);
  cn_var_参数_1 = /* NONE */;
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.右括号;
  期望(r10, r12);
  struct 表达式节点* cn_var_调用节点_2;
  创建函数调用表达式();
  cn_var_调用节点_2 = /* NONE */;
  r13 = cn_var_调用节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1208; else goto if_merge_1209;

  if_else_1206:
  r19 = cn_var_实例;
  r20 = cn_var_词元类型枚举;
  r21 = r20.点;
  检查(r19, r21);
  goto if_then_1210;

  if_merge_1207:
  goto while_cond_1202;

  if_then_1208:
  r15 = cn_var_表达式_0;
  return r15;
  goto if_merge_1209;

  if_merge_1209:
  r16 = cn_var_表达式_0;
  r17 = cn_var_参数_1;
  r18 = cn_var_调用节点_2;
  cn_var_表达式_0 = r18;
  goto if_merge_1207;

  if_then_1210:
  r22 = cn_var_实例;
  前进词元(r22);
  char* cn_var_成员名_3;
  r23 = cn_var_实例;
  r24 = r23->当前词元;
  r25 = r24.值;
  cn_var_成员名_3 = r25;
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.标识符;
  期望(r26, r28);
  struct 表达式节点* cn_var_成员节点_4;
  创建成员访问表达式();
  cn_var_成员节点_4 = /* NONE */;
  r29 = cn_var_成员节点_4;
  r30 = r29 == 0;
  if (r30) goto if_then_1213; else goto if_merge_1214;

  if_else_1211:
  r35 = cn_var_实例;
  r36 = cn_var_词元类型枚举;
  r37 = r36.箭头;
  检查(r35, r37);
  goto if_then_1215;

  if_merge_1212:
  goto if_merge_1207;

  if_then_1213:
  r31 = cn_var_表达式_0;
  return r31;
  goto if_merge_1214;

  if_merge_1214:
  r32 = cn_var_表达式_0;
  r33 = cn_var_成员名_3;
  r34 = cn_var_成员节点_4;
  cn_var_表达式_0 = r34;
  goto if_merge_1212;

  if_then_1215:
  r38 = cn_var_实例;
  前进词元(r38);
  char* cn_var_成员名_5;
  r39 = cn_var_实例;
  r40 = r39->当前词元;
  r41 = r40.值;
  cn_var_成员名_5 = r41;
  r42 = cn_var_实例;
  r43 = cn_var_词元类型枚举;
  r44 = r43.标识符;
  期望(r42, r44);
  struct 表达式节点* cn_var_成员节点_6;
  创建成员访问表达式();
  cn_var_成员节点_6 = /* NONE */;
  r45 = cn_var_成员节点_6;
  r46 = r45 == 0;
  if (r46) goto if_then_1218; else goto if_merge_1219;

  if_else_1216:
  r51 = cn_var_实例;
  r52 = cn_var_词元类型枚举;
  r53 = r52.左方括号;
  检查(r51, r53);
  goto if_then_1220;

  if_merge_1217:
  goto if_merge_1212;

  if_then_1218:
  r47 = cn_var_表达式_0;
  return r47;
  goto if_merge_1219;

  if_merge_1219:
  r48 = cn_var_表达式_0;
  r49 = cn_var_成员名_5;
  r50 = cn_var_成员节点_6;
  cn_var_表达式_0 = r50;
  goto if_merge_1217;

  if_then_1220:
  r54 = cn_var_实例;
  前进词元(r54);
  struct 表达式节点* cn_var_索引_7;
  r55 = cn_var_实例;
  解析表达式(r55);
  cn_var_索引_7 = /* NONE */;
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.右方括号;
  期望(r56, r58);
  struct 表达式节点* cn_var_数组节点_8;
  创建数组访问表达式();
  cn_var_数组节点_8 = /* NONE */;
  r59 = cn_var_数组节点_8;
  r60 = r59 == 0;
  if (r60) goto if_then_1223; else goto if_merge_1224;

  if_else_1221:
  r65 = cn_var_实例;
  r66 = cn_var_词元类型枚举;
  r67 = r66.自增;
  检查(r65, r67);
  goto if_then_1225;

  if_merge_1222:
  goto if_merge_1217;

  if_then_1223:
  r61 = cn_var_表达式_0;
  return r61;
  goto if_merge_1224;

  if_merge_1224:
  r62 = cn_var_表达式_0;
  r63 = cn_var_索引_7;
  r64 = cn_var_数组节点_8;
  cn_var_表达式_0 = r64;
  goto if_merge_1222;

  if_then_1225:
  r68 = cn_var_实例;
  前进词元(r68);
  struct 表达式节点* cn_var_一元节点_9;
  创建一元表达式();
  cn_var_一元节点_9 = /* NONE */;
  r69 = cn_var_一元节点_9;
  r70 = r69 == 0;
  if (r70) goto if_then_1228; else goto if_merge_1229;

  if_else_1226:
  r76 = cn_var_实例;
  r77 = cn_var_词元类型枚举;
  r78 = r77.自减;
  检查(r76, r78);
  goto if_then_1230;

  if_merge_1227:
  goto if_merge_1222;

  if_then_1228:
  r71 = cn_var_表达式_0;
  return r71;
  goto if_merge_1229;

  if_merge_1229:
  r72 = cn_var_表达式_0;
  r73 = cn_var_一元运算符;
  r74 = r73.后缀自增;
  r75 = cn_var_一元节点_9;
  cn_var_表达式_0 = r75;
  goto if_merge_1227;

  if_then_1230:
  r79 = cn_var_实例;
  前进词元(r79);
  struct 表达式节点* cn_var_一元节点_10;
  创建一元表达式();
  cn_var_一元节点_10 = /* NONE */;
  r80 = cn_var_一元节点_10;
  r81 = r80 == 0;
  if (r81) goto if_then_1233; else goto if_merge_1234;

  if_else_1231:
  goto while_exit_1204;
  goto if_merge_1232;

  if_merge_1232:
  goto if_merge_1227;

  if_then_1233:
  r82 = cn_var_表达式_0;
  return r82;
  goto if_merge_1234;

  if_merge_1234:
  r83 = cn_var_表达式_0;
  r84 = cn_var_一元运算符;
  r85 = r84.后缀自减;
  r86 = cn_var_一元节点_10;
  cn_var_表达式_0 = r86;
  goto if_merge_1232;
  return NULL;
}

struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r7, r10, r13, r16, r18, r21, r24, r28, r32, r36, r40, r44, r48, r54, r58;
  char* r51;
  char* r55;
  struct 解析器* r0;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r22;
  struct 解析器* r25;
  struct 解析器* r26;
  struct 解析器* r29;
  struct 解析器* r30;
  struct 解析器* r33;
  struct 解析器* r34;
  struct 解析器* r37;
  struct 解析器* r38;
  struct 解析器* r41;
  struct 解析器* r42;
  struct 解析器* r45;
  struct 解析器* r46;
  struct 解析器* r49;
  struct 解析器* r52;
  struct 解析器* r56;
  struct 词元 r50;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r39;
  enum 词元类型枚举 r43;
  enum 词元类型枚举 r47;
  enum 词元类型枚举 r53;
  enum 诊断错误码 r57;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1235; else goto if_merge_1236;

  if_then_1235:
  return 0;
  goto if_merge_1236;

  if_merge_1236:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.整数字面量;
  检查(r5, r7);
  goto logic_merge_1244;

  if_then_1237:
  r17 = cn_var_实例;
  解析字面量(r17);
  return;
  goto if_merge_1238;

  if_merge_1238:
  r19 = cn_var_实例;
  r20 = cn_var_词元类型枚举;
  r21 = r20.关键字_真;
  检查(r19, r21);
  goto logic_merge_1248;

  logic_rhs_1239:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.字符字面量;
  检查(r14, r16);
  goto logic_merge_1240;

  logic_merge_1240:
  goto if_then_1237;

  logic_rhs_1241:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.字符串字面量;
  检查(r11, r13);
  goto logic_merge_1242;

  logic_merge_1242:
  goto logic_merge_1240;

  logic_rhs_1243:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.浮点字面量;
  检查(r8, r10);
  goto logic_merge_1244;

  logic_merge_1244:
  goto logic_merge_1242;

  if_then_1245:
  r25 = cn_var_实例;
  解析字面量(r25);
  return;
  goto if_merge_1246;

  if_merge_1246:
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.关键字_无;
  检查(r26, r28);
  goto if_then_1249;

  logic_rhs_1247:
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.关键字_假;
  检查(r22, r24);
  goto logic_merge_1248;

  logic_merge_1248:
  goto if_then_1245;

  if_then_1249:
  r29 = cn_var_实例;
  前进词元(r29);
  创建空值表达式();
  return;
  goto if_merge_1250;

  if_merge_1250:
  r30 = cn_var_实例;
  r31 = cn_var_词元类型枚举;
  r32 = r31.标识符;
  检查(r30, r32);
  goto if_then_1251;

  if_then_1251:
  r33 = cn_var_实例;
  解析标识符表达式(r33);
  return;
  goto if_merge_1252;

  if_merge_1252:
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.左括号;
  检查(r34, r36);
  goto if_then_1253;

  if_then_1253:
  r37 = cn_var_实例;
  解析括号表达式(r37);
  return;
  goto if_merge_1254;

  if_merge_1254:
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.关键字_自身;
  检查(r38, r40);
  goto if_then_1255;

  if_then_1255:
  r41 = cn_var_实例;
  前进词元(r41);
  创建自身表达式();
  return;
  goto if_merge_1256;

  if_merge_1256:
  r42 = cn_var_实例;
  r43 = cn_var_词元类型枚举;
  r44 = r43.关键字_基类;
  检查(r42, r44);
  goto if_then_1257;

  if_then_1257:
  r45 = cn_var_实例;
  前进词元(r45);
  r46 = cn_var_实例;
  r47 = cn_var_词元类型枚举;
  r48 = r47.点;
  期望(r46, r48);
  char* cn_var_成员名_0;
  r49 = cn_var_实例;
  r50 = r49->当前词元;
  r51 = r50.值;
  cn_var_成员名_0 = r51;
  r52 = cn_var_实例;
  r53 = cn_var_词元类型枚举;
  r54 = r53.标识符;
  期望(r52, r54);
  r55 = cn_var_成员名_0;
  创建基类访问表达式(r55);
  return;
  goto if_merge_1258;

  if_merge_1258:
  r56 = cn_var_实例;
  r57 = cn_var_诊断错误码;
  r58 = r57.语法_无效表达式;
  报告错误(r56, r58, "期望表达式");
  return 0;
}

struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例) {
  long long r1, r3, r8, r11, r14, r17, r20, r23, r25, r28;
  char* r34;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 解析器* r4;
  struct 解析器* r26;
  struct 表达式节点* r27;
  struct 解析器* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r31;
  struct 解析器* r32;
  struct 解析器* r35;
  struct 表达式节点* r36;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  struct 词元 r5;
  struct 词元 r33;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1259; else goto if_merge_1260;

  if_then_1259:
  return 0;
  goto if_merge_1260;

  if_merge_1260:
  struct 表达式节点* cn_var_节点_0;
  创建字面量表达式();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1261; else goto if_merge_1262;

  if_then_1261:
  return 0;
  goto if_merge_1262;

  if_merge_1262:
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.类型;
  r7 = cn_var_词元类型枚举;
  r8 = r7.整数字面量;
  r9 = r6 == r8;
  if (r9) goto case_body_1264; else goto switch_check_1271;

  switch_check_1271:
  r10 = cn_var_词元类型枚举;
  r11 = r10.浮点字面量;
  r12 = r6 == r11;
  if (r12) goto case_body_1265; else goto switch_check_1272;

  switch_check_1272:
  r13 = cn_var_词元类型枚举;
  r14 = r13.字符串字面量;
  r15 = r6 == r14;
  if (r15) goto case_body_1266; else goto switch_check_1273;

  switch_check_1273:
  r16 = cn_var_词元类型枚举;
  r17 = r16.字符字面量;
  r18 = r6 == r17;
  if (r18) goto case_body_1267; else goto switch_check_1274;

  switch_check_1274:
  r19 = cn_var_词元类型枚举;
  r20 = r19.关键字_真;
  r21 = r6 == r20;
  if (r21) goto case_body_1268; else goto switch_check_1275;

  switch_check_1275:
  r22 = cn_var_词元类型枚举;
  r23 = r22.关键字_假;
  r24 = r6 == r23;
  if (r24) goto case_body_1269; else goto case_default_1270;

  case_body_1264:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1265:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1266:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1267:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1268:
  r25 = cn_var_布尔字面量;
  r26 = cn_var_实例;
  前进词元(r26);
  r27 = cn_var_节点_0;
  return r27;
  goto switch_merge_1263;

  case_body_1269:
  r28 = cn_var_布尔字面量;
  r29 = cn_var_实例;
  前进词元(r29);
  r30 = cn_var_节点_0;
  return r30;
  goto switch_merge_1263;

  case_default_1270:
  r31 = cn_var_节点_0;
  释放表达式节点(r31);
  return 0;
  goto switch_merge_1263;

  switch_merge_1263:
  r32 = cn_var_实例;
  r33 = r32->当前词元;
  r34 = r33.值;
  r35 = cn_var_实例;
  前进词元(r35);
  r36 = cn_var_节点_0;
  return r36;
}

struct 表达式节点* 解析标识符表达式(struct 解析器* cn_var_实例) {
  long long r1, r3;
  char* r6;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 词元 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1276; else goto if_merge_1277;

  if_then_1276:
  return 0;
  goto if_merge_1277;

  if_merge_1277:
  struct 表达式节点* cn_var_节点_0;
  创建标识符表达式();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1278; else goto if_merge_1279;

  if_then_1278:
  return 0;
  goto if_merge_1279;

  if_merge_1279:
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  前进词元(r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 表达式节点* 解析括号表达式(struct 解析器* cn_var_实例) {
  long long r1, r4, r8;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 解析器* r6;
  struct 表达式节点* r9;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1280; else goto if_merge_1281;

  if_then_1280:
  return 0;
  goto if_merge_1281;

  if_merge_1281:
  r2 = cn_var_实例;
  r3 = cn_var_词元类型枚举;
  r4 = r3.左括号;
  期望(r2, r4);
  struct 表达式节点* cn_var_表达式_0;
  r5 = cn_var_实例;
  解析表达式(r5);
  cn_var_表达式_0 = /* NONE */;
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.右括号;
  期望(r6, r8);
  r9 = cn_var_表达式_0;
  return r9;
}

struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r7, r10, r15, r16;
  struct 解析器* r0;
  struct 表达式列表* r2;
  struct 解析器* r4;
  struct 解析器* r8;
  struct 表达式列表* r11;
  struct 解析器* r13;
  struct 表达式列表* r17;
  struct 参数 r9;
  struct 参数 r12;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1282; else goto if_merge_1283;

  if_then_1282:
  return 0;
  goto if_merge_1283;

  if_merge_1283:
  struct 表达式列表* cn_var_列表_0;
  创建表达式列表();
  cn_var_列表_0 = /* NONE */;
  r2 = cn_var_列表_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1284; else goto if_merge_1285;

  if_then_1284:
  return 0;
  goto if_merge_1285;

  if_merge_1285:
  goto while_cond_1286;

  while_cond_1286:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.右括号;
  检查(r4, r6);
  r7 = !/* NONE */;
  if (r7) goto while_body_1287; else goto while_exit_1288;

  while_body_1287:
  struct 表达式节点* cn_var_参数_1;
  r8 = cn_var_实例;
  解析表达式(r8);
  cn_var_参数_1 = /* NONE */;
  r9 = cn_var_参数_1;
  r10 = r9 != 0;
  if (r10) goto if_then_1289; else goto if_merge_1290;

  while_exit_1288:
  r17 = cn_var_列表_0;
  return r17;

  if_then_1289:
  r11 = cn_var_列表_0;
  r12 = cn_var_参数_1;
  表达式列表添加(r11, r12);
  goto if_merge_1290;

  if_merge_1290:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.逗号;
  匹配(r13, r15);
  r16 = !/* NONE */;
  if (r16) goto if_then_1291; else goto if_merge_1292;

  if_then_1291:
  goto while_exit_1288;
  goto if_merge_1292;

  if_merge_1292:
  goto while_cond_1286;
  return NULL;
}

