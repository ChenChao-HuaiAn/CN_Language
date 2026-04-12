#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 导入类型 {
    导入类型_标准导入 = 0,
    导入类型_相对导入 = 1,
    导入类型_模块导入 = 2
};
enum 可见性 {
    可见性_私有级别 = 0,
    可见性_保护级别 = 1,
    可见性_公开级别 = 2
};
enum 节点类型 {
    节点类型_程序节点 = 0,
    节点类型_函数声明 = 1,
    节点类型_变量声明 = 2,
    节点类型_结构体声明 = 3,
    节点类型_枚举声明 = 4,
    节点类型_模块声明 = 5,
    节点类型_导入声明 = 6,
    节点类型_类声明 = 7,
    节点类型_接口声明 = 8,
    节点类型_模板函数声明 = 9,
    节点类型_模板结构体声明 = 10,
    节点类型_表达式语句 = 11,
    节点类型_块语句 = 12,
    节点类型_如果语句 = 13,
    节点类型_当语句 = 14,
    节点类型_循环语句 = 15,
    节点类型_返回语句 = 16,
    节点类型_中断语句 = 17,
    节点类型_继续语句 = 18,
    节点类型_选择语句 = 19,
    节点类型_尝试语句 = 20,
    节点类型_抛出语句 = 21,
    节点类型_最终语句 = 22,
    节点类型_二元表达式 = 23,
    节点类型_一元表达式 = 24,
    节点类型_字面量表达式 = 25,
    节点类型_标识符表达式 = 26,
    节点类型_函数调用表达式 = 27,
    节点类型_成员访问表达式 = 28,
    节点类型_数组访问表达式 = 29,
    节点类型_赋值表达式 = 30,
    节点类型_三元表达式 = 31,
    节点类型_数组字面量表达式 = 32,
    节点类型_结构体字面量表达式 = 33,
    节点类型_逻辑表达式 = 34,
    节点类型_模板实例化表达式 = 35,
    节点类型_基础类型 = 36,
    节点类型_指针类型 = 37,
    节点类型_数组类型 = 38,
    节点类型_函数类型 = 39,
    节点类型_结构体类型 = 40,
    节点类型_枚举类型 = 41,
    节点类型_类类型 = 42,
    节点类型_接口类型 = 43,
    节点类型_标识符类型 = 44
};
enum 字面量类型 {
    字面量类型_字面量_整数 = 0,
    字面量类型_字面量_浮点 = 1,
    字面量类型_字面量_字符串 = 2,
    字面量类型_字面量_布尔 = 3,
    字面量类型_字面量_空 = 4,
    字面量类型_字面量_字符 = 5
};
enum 赋值运算符 {
    赋值运算符_赋值_简单 = 0,
    赋值运算符_赋值_加法 = 1,
    赋值运算符_赋值_减法 = 2,
    赋值运算符_赋值_乘法 = 3,
    赋值运算符_赋值_除法 = 4,
    赋值运算符_赋值_取模 = 5,
    赋值运算符_赋值_位与 = 6,
    赋值运算符_赋值_位或 = 7,
    赋值运算符_赋值_位异或 = 8,
    赋值运算符_赋值_左移 = 9,
    赋值运算符_赋值_右移 = 10
};
enum 一元运算符 {
    一元运算符_一元_逻辑非 = 0,
    一元运算符_一元_取负 = 1,
    一元运算符_一元_取地址 = 2,
    一元运算符_一元_解引用 = 3,
    一元运算符_一元_位取反 = 4,
    一元运算符_一元_前置自增 = 5,
    一元运算符_一元_前置自减 = 6,
    一元运算符_一元_后置自增 = 7,
    一元运算符_一元_后置自减 = 8
};
enum 逻辑运算符 {
    逻辑运算符_逻辑_与 = 0,
    逻辑运算符_逻辑_或 = 1
};
enum 二元运算符 {
    二元运算符_二元_加 = 0,
    二元运算符_二元_减 = 1,
    二元运算符_二元_乘 = 2,
    二元运算符_二元_除 = 3,
    二元运算符_二元_取模 = 4,
    二元运算符_二元_等于 = 5,
    二元运算符_二元_不等于 = 6,
    二元运算符_二元_小于 = 7,
    二元运算符_二元_大于 = 8,
    二元运算符_二元_小于等于 = 9,
    二元运算符_二元_大于等于 = 10,
    二元运算符_二元_位与 = 11,
    二元运算符_二元_位或 = 12,
    二元运算符_二元_位异或 = 13,
    二元运算符_二元_左移 = 14,
    二元运算符_二元_右移 = 15
};

// Struct Definitions - 从导入模块
struct 类型节点;
struct 类型节点;
struct 类型节点 {
    enum 节点类型 类型;
    char* 名称;
    struct 类型节点* 元素类型;
    long long 指针层级;
    long long 数组维度;
    long long 数组大小;
    _Bool 是常量;
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

// CN Language Global Struct Forward Declarations
struct 二元表达式;
struct 一元表达式;
struct 逻辑表达式;
struct 字面量表达式;
struct 标识符表达式;
struct 函数调用表达式;
struct 成员访问表达式;
struct 数组访问表达式;
struct 赋值表达式;
struct 三元表达式;
struct 数组字面量表达式;
struct 结构体字段初始化;
struct 结构体字面量表达式;
struct 模板实例化表达式;
struct 表达式节点;
struct 表达式列表;

// CN Language Global Struct Definitions
struct 二元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 二元运算符 运算符;
};

struct 一元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 操作数;
    enum 一元运算符 运算符;
    _Bool 是前缀;
};

struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
};

struct 字面量表达式 {
    struct AST节点 节点基类;
    enum 字面量类型 类型;
    char* 值;
    long long 整数值;
    double 浮点值;
    _Bool 布尔值;
};

struct 标识符表达式 {
    struct AST节点 节点基类;
    char* 名称;
};

struct 函数调用表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 被调函数;
    struct 表达式节点** 参数;
    long long 参数个数;
};

struct 成员访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 对象;
    char* 成员名;
    _Bool 是指针访问;
    _Bool 是静态成员;
    char* 类名;
};

struct 数组访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
};

struct 赋值表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 目标;
    struct 表达式节点* 值;
    enum 赋值运算符 运算符;
};

struct 三元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 表达式节点* 真值;
    struct 表达式节点* 假值;
};

struct 数组字面量表达式 {
    struct AST节点 节点基类;
    struct 表达式节点** 元素;
    long long 元素个数;
};

struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};

struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};

struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};

struct 表达式节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
    struct 类型节点* 类型信息;
    _Bool 是自身指针;
    long long 整数值;
    double 小数值;
    char* 字符串值;
    _Bool 布尔值;
    char* 标识符名称;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    long long 运算符;
    struct 表达式节点* 操作数;
    struct 表达式节点* 被调函数;
    struct 表达式节点* 参数列表;
    long long 参数个数;
    struct 表达式节点* 对象;
    char* 成员名;
    long long 成员索引;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
    struct 表达式节点* 左侧表达式;
    struct 表达式节点* 值;
    struct 表达式节点* 指针;
};

struct 表达式列表 {
    struct 表达式节点* 节点;
    struct 表达式列表* 下一个;
};

// Global Variables

// Forward Declarations
struct 表达式节点* 创建表达式节点(enum 节点类型);
struct 表达式节点* 创建赋值表达式();
struct 表达式节点* 创建二元表达式();
struct 表达式节点* 创建三元表达式();
struct 表达式节点* 创建一元表达式();
struct 表达式节点* 创建函数调用表达式();
struct 表达式节点* 创建成员访问表达式();
struct 表达式节点* 创建数组访问表达式();
struct 表达式节点* 创建字面量表达式();
struct 表达式节点* 创建标识符表达式();
struct 表达式节点* 创建空值表达式();
struct 表达式节点* 创建自身表达式();
struct 表达式节点* 创建基类访问表达式(char*);
struct 表达式列表* 创建表达式列表();
void 表达式列表添加(struct 表达式列表*, struct 表达式节点*);
void 释放表达式节点(struct 表达式节点*);
void 释放表达式列表(struct 表达式列表*);

struct 表达式节点* 创建表达式节点(enum 节点类型 cn_var_类型) {
  long long r0, r3;
  void* r1;
  struct 表达式节点* r2;
  struct 表达式节点* r5;
  enum 节点类型 r4;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = sizeof(struct 表达式节点);
  r1 = 分配内存(r0);
  cn_var_节点_0 = r1;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_758; else goto if_merge_759;

  if_then_758:
  return 0;
  goto if_merge_759;

  if_merge_759:
  r4 = cn_var_类型;
  r5 = cn_var_节点_0;
  return r5;
}

struct 表达式节点* 创建赋值表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_赋值表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建二元表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_二元表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建三元表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_三元表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建一元表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_一元表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建函数调用表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_函数调用表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建成员访问表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_成员访问表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建数组访问表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_数组访问表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建字面量表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_字面量表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建标识符表达式() {
  struct 表达式节点* r0;
  struct 表达式节点* r1;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_标识符表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  return r1;
}

struct 表达式节点* 创建空值表达式() {
  long long r2;
  struct 表达式节点* r0;
  struct 表达式节点* r1;
  struct 表达式节点* r3;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_字面量表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  r2 = r1 != 0;
  if (r2) goto if_then_760; else goto if_merge_761;

  if_then_760:
  goto if_merge_761;

  if_merge_761:
  r3 = cn_var_节点_0;
  return r3;
}

struct 表达式节点* 创建自身表达式() {
  long long r2;
  struct 表达式节点* r0;
  struct 表达式节点* r1;
  struct 表达式节点* r3;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_标识符表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  r2 = r1 != 0;
  if (r2) goto if_then_762; else goto if_merge_763;

  if_then_762:
  goto if_merge_763;

  if_merge_763:
  r3 = cn_var_节点_0;
  return r3;
}

struct 表达式节点* 创建基类访问表达式(char* cn_var_成员名) {
  long long r2;
  char* r3;
  struct 表达式节点* r0;
  struct 表达式节点* r1;
  struct 表达式节点* r4;

  entry:
  struct 表达式节点* cn_var_节点_0;
  r0 = 创建表达式节点(节点类型_成员访问表达式);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  r2 = r1 != 0;
  if (r2) goto if_then_764; else goto if_merge_765;

  if_then_764:
  r3 = cn_var_成员名;
  goto if_merge_765;

  if_merge_765:
  r4 = cn_var_节点_0;
  return r4;
}

struct 表达式列表* 创建表达式列表() {
  long long r0, r3;
  void* r1;
  struct 表达式列表* r2;
  struct 表达式列表* r4;

  entry:
  struct 表达式列表* cn_var_列表_0;
  r0 = sizeof(struct 表达式列表);
  r1 = 分配内存(r0);
  cn_var_列表_0 = r1;
  r2 = cn_var_列表_0;
  r3 = r2 == 0;
  if (r3) goto if_then_766; else goto if_merge_767;

  if_then_766:
  return 0;
  goto if_merge_767;

  if_merge_767:
  r4 = cn_var_列表_0;
  return r4;
}

void 表达式列表添加(struct 表达式列表* cn_var_列表, struct 表达式节点* cn_var_节点) {
  long long r0, r2, r4, r7, r12, r17;
  struct 表达式列表* r1;
  struct 表达式节点* r3;
  struct 表达式列表* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r8;
  struct 表达式列表* r9;
  struct 表达式列表* r10;
  struct 表达式列表* r11;
  struct 表达式列表* r13;
  struct 表达式列表* r14;
  struct 表达式列表* r15;
  struct 表达式列表* r16;
  struct 表达式节点* r18;
  struct 表达式列表* r19;

  entry:
  r1 = cn_var_列表;
  r2 = r1 == 0;
  if (r2) goto logic_merge_771; else goto logic_rhs_770;

  if_then_768:
  return;
  goto if_merge_769;

  if_merge_769:
  r5 = cn_var_列表;
  r6 = r5->节点;
  r7 = r6 == 0;
  if (r7) goto if_then_772; else goto if_merge_773;

  logic_rhs_770:
  r3 = cn_var_节点;
  r4 = r3 == 0;
  goto logic_merge_771;

  logic_merge_771:
  if (r4) goto if_then_768; else goto if_merge_769;

  if_then_772:
  r8 = cn_var_节点;
  return;
  goto if_merge_773;

  if_merge_773:
  struct 表达式列表* cn_var_当前_0;
  r9 = cn_var_列表;
  cn_var_当前_0 = r9;
  goto while_cond_774;

  while_cond_774:
  r10 = cn_var_当前_0;
  r11 = r10->下一个;
  r12 = r11 != 0;
  if (r12) goto while_body_775; else goto while_exit_776;

  while_body_775:
  r13 = cn_var_当前_0;
  r14 = r13->下一个;
  cn_var_当前_0 = r14;
  goto while_cond_774;

  while_exit_776:
  struct 表达式列表* cn_var_新节点_1;
  r15 = 创建表达式列表();
  cn_var_新节点_1 = r15;
  r16 = cn_var_新节点_1;
  r17 = r16 != 0;
  if (r17) goto if_then_777; else goto if_merge_778;

  if_then_777:
  r18 = cn_var_节点;
  r19 = cn_var_新节点_1;
  goto if_merge_778;

  if_merge_778:
  return;
}

void 释放表达式节点(struct 表达式节点* cn_var_节点) {
  long long r1, r4, r9, r14, r19, r24, r29, r34, r39, r44, r49, r54;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r7;
  struct 表达式节点* r8;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r22;
  struct 表达式节点* r23;
  struct 表达式节点* r25;
  struct 表达式节点* r26;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct 表达式节点* r30;
  struct 表达式节点* r31;
  struct 表达式节点* r32;
  struct 表达式节点* r33;
  struct 表达式节点* r35;
  struct 表达式节点* r36;
  struct 表达式节点* r37;
  struct 表达式节点* r38;
  struct 表达式节点* r40;
  struct 表达式节点* r41;
  struct 表达式节点* r42;
  struct 表达式节点* r43;
  struct 表达式节点* r45;
  struct 表达式节点* r46;
  struct 表达式节点* r47;
  struct 表达式节点* r48;
  struct 表达式节点* r50;
  struct 表达式节点* r51;
  struct 表达式节点* r52;
  struct 表达式节点* r53;
  struct 表达式节点* r55;
  struct 表达式节点* r56;
  struct 表达式节点* r57;

  entry:
  r0 = cn_var_节点;
  r1 = r0 == 0;
  if (r1) goto if_then_779; else goto if_merge_780;

  if_then_779:
  return;
  goto if_merge_780;

  if_merge_780:
  r2 = cn_var_节点;
  r3 = r2->左操作数;
  r4 = r3 != 0;
  if (r4) goto if_then_781; else goto if_merge_782;

  if_then_781:
  r5 = cn_var_节点;
  r6 = r5->左操作数;
  释放表达式节点(r6);
  goto if_merge_782;

  if_merge_782:
  r7 = cn_var_节点;
  r8 = r7->右操作数;
  r9 = r8 != 0;
  if (r9) goto if_then_783; else goto if_merge_784;

  if_then_783:
  r10 = cn_var_节点;
  r11 = r10->右操作数;
  释放表达式节点(r11);
  goto if_merge_784;

  if_merge_784:
  r12 = cn_var_节点;
  r13 = r12->操作数;
  r14 = r13 != 0;
  if (r14) goto if_then_785; else goto if_merge_786;

  if_then_785:
  r15 = cn_var_节点;
  r16 = r15->操作数;
  释放表达式节点(r16);
  goto if_merge_786;

  if_merge_786:
  r17 = cn_var_节点;
  r18 = r17->被调函数;
  r19 = r18 != 0;
  if (r19) goto if_then_787; else goto if_merge_788;

  if_then_787:
  r20 = cn_var_节点;
  r21 = r20->被调函数;
  释放表达式节点(r21);
  goto if_merge_788;

  if_merge_788:
  r22 = cn_var_节点;
  r23 = r22->参数列表;
  r24 = r23 != 0;
  if (r24) goto if_then_789; else goto if_merge_790;

  if_then_789:
  r25 = cn_var_节点;
  r26 = r25->参数列表;
  释放表达式节点(r26);
  goto if_merge_790;

  if_merge_790:
  r27 = cn_var_节点;
  r28 = r27->对象;
  r29 = r28 != 0;
  if (r29) goto if_then_791; else goto if_merge_792;

  if_then_791:
  r30 = cn_var_节点;
  r31 = r30->对象;
  释放表达式节点(r31);
  goto if_merge_792;

  if_merge_792:
  r32 = cn_var_节点;
  r33 = r32->数组;
  r34 = r33 != 0;
  if (r34) goto if_then_793; else goto if_merge_794;

  if_then_793:
  r35 = cn_var_节点;
  r36 = r35->数组;
  释放表达式节点(r36);
  goto if_merge_794;

  if_merge_794:
  r37 = cn_var_节点;
  r38 = r37->索引;
  r39 = r38 != 0;
  if (r39) goto if_then_795; else goto if_merge_796;

  if_then_795:
  r40 = cn_var_节点;
  r41 = r40->索引;
  释放表达式节点(r41);
  goto if_merge_796;

  if_merge_796:
  r42 = cn_var_节点;
  r43 = r42->左侧表达式;
  r44 = r43 != 0;
  if (r44) goto if_then_797; else goto if_merge_798;

  if_then_797:
  r45 = cn_var_节点;
  r46 = r45->左侧表达式;
  释放表达式节点(r46);
  goto if_merge_798;

  if_merge_798:
  r47 = cn_var_节点;
  r48 = r47->值;
  r49 = r48 != 0;
  if (r49) goto if_then_799; else goto if_merge_800;

  if_then_799:
  r50 = cn_var_节点;
  r51 = r50->值;
  释放表达式节点(r51);
  goto if_merge_800;

  if_merge_800:
  r52 = cn_var_节点;
  r53 = r52->指针;
  r54 = r53 != 0;
  if (r54) goto if_then_801; else goto if_merge_802;

  if_then_801:
  r55 = cn_var_节点;
  r56 = r55->指针;
  释放表达式节点(r56);
  goto if_merge_802;

  if_merge_802:
  r57 = cn_var_节点;
  释放内存(r57);
  return;
}

void 释放表达式列表(struct 表达式列表* cn_var_列表) {
  long long r1, r4, r9;
  struct 表达式列表* r0;
  struct 表达式列表* r2;
  struct 表达式节点* r3;
  struct 表达式列表* r5;
  struct 表达式节点* r6;
  struct 表达式列表* r7;
  struct 表达式列表* r8;
  struct 表达式列表* r10;
  struct 表达式列表* r11;
  struct 表达式列表* r12;

  entry:
  r0 = cn_var_列表;
  r1 = r0 == 0;
  if (r1) goto if_then_803; else goto if_merge_804;

  if_then_803:
  return;
  goto if_merge_804;

  if_merge_804:
  r2 = cn_var_列表;
  r3 = r2->节点;
  r4 = r3 != 0;
  if (r4) goto if_then_805; else goto if_merge_806;

  if_then_805:
  r5 = cn_var_列表;
  r6 = r5->节点;
  释放表达式节点(r6);
  goto if_merge_806;

  if_merge_806:
  r7 = cn_var_列表;
  r8 = r7->下一个;
  r9 = r8 != 0;
  if (r9) goto if_then_807; else goto if_merge_808;

  if_then_807:
  r10 = cn_var_列表;
  r11 = r10->下一个;
  释放表达式列表(r11);
  goto if_merge_808;

  if_merge_808:
  r12 = cn_var_列表;
  释放内存(r12);
  return;
}

