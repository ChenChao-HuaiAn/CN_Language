#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 导入类型 {
    导入类型_模块导入 = 2,
    导入类型_相对导入 = 1,
    导入类型_标准导入 = 0
};
enum 可见性 {
    可见性_公开级别 = 2,
    可见性_保护级别 = 1,
    可见性_私有级别 = 0
};
enum 节点类型 {
    节点类型_标识符类型 = 44,
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
enum 字面量类型 {
    字面量类型_字面量_字符 = 5,
    字面量类型_字面量_空 = 4,
    字面量类型_字面量_布尔 = 3,
    字面量类型_字面量_字符串 = 2,
    字面量类型_字面量_浮点 = 1,
    字面量类型_字面量_整数 = 0
};
enum 赋值运算符 {
    赋值运算符_赋值_右移 = 10,
    赋值运算符_赋值_左移 = 9,
    赋值运算符_赋值_位异或 = 8,
    赋值运算符_赋值_位或 = 7,
    赋值运算符_赋值_位与 = 6,
    赋值运算符_赋值_取模 = 5,
    赋值运算符_赋值_除法 = 4,
    赋值运算符_赋值_乘法 = 3,
    赋值运算符_赋值_减法 = 2,
    赋值运算符_赋值_加法 = 1,
    赋值运算符_赋值_简单 = 0
};
enum 一元运算符 {
    一元运算符_一元_后置自减 = 8,
    一元运算符_一元_后置自增 = 7,
    一元运算符_一元_前置自减 = 6,
    一元运算符_一元_前置自增 = 5,
    一元运算符_一元_位取反 = 4,
    一元运算符_一元_解引用 = 3,
    一元运算符_一元_取地址 = 2,
    一元运算符_一元_取负 = 1,
    一元运算符_一元_逻辑非 = 0
};
enum 逻辑运算符 {
    逻辑运算符_逻辑_或 = 1,
    逻辑运算符_逻辑_与 = 0
};
enum 二元运算符 {
    二元运算符_二元_逻辑或 = 17,
    二元运算符_二元_逻辑与 = 16,
    二元运算符_二元_右移 = 15,
    二元运算符_二元_左移 = 14,
    二元运算符_二元_位异或 = 13,
    二元运算符_二元_位或 = 12,
    二元运算符_二元_位与 = 11,
    二元运算符_二元_大于等于 = 10,
    二元运算符_二元_小于等于 = 9,
    二元运算符_二元_大于 = 8,
    二元运算符_二元_小于 = 7,
    二元运算符_二元_不等于 = 6,
    二元运算符_二元_等于 = 5,
    二元运算符_二元_取模 = 4,
    二元运算符_二元_除 = 3,
    二元运算符_二元_乘 = 2,
    二元运算符_二元_减 = 1,
    二元运算符_二元_加 = 0
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
    struct 表达式列表* 参数列表;
    long long 参数个数;
    struct 表达式节点* 对象;
    char* 成员名;
    long long 成员索引;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
    struct 表达式节点* 左侧表达式;
    struct 表达式节点* 值;
    struct 赋值表达式 赋值表达式;
    struct 三元表达式 三元表达式;
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
struct 表达式节点* 创建基类访问表达式(const char*);
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

struct 表达式节点* 创建基类访问表达式(const char* cn_var_成员名) {
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
  long long r0, r2, r4, r5, r6, r9, r10, r11, r14;
  struct 表达式列表* r1;
  struct 表达式节点* r3;
  struct 表达式节点* r7;
  struct 表达式列表* r8;
  struct 表达式列表* r12;
  struct 表达式列表* r13;
  struct 表达式节点* r15;
  struct 表达式列表* r16;

  entry:
  r1 = cn_var_列表;
  r2 = r1 == 0;
  if (r2) goto logic_merge_771; else goto logic_rhs_770;

  if_then_768:
  return;
  goto if_merge_769;

  if_merge_769:
  r5 = cn_var_列表.节点;
  r6 = r5 == 0;
  if (r6) goto if_then_772; else goto if_merge_773;

  logic_rhs_770:
  r3 = cn_var_节点;
  r4 = r3 == 0;
  goto logic_merge_771;

  logic_merge_771:
  if (r4) goto if_then_768; else goto if_merge_769;

  if_then_772:
  r7 = cn_var_节点;
  return;
  goto if_merge_773;

  if_merge_773:
  struct 表达式列表* cn_var_当前_0;
  r8 = cn_var_列表;
  cn_var_当前_0 = r8;
  goto while_cond_774;

  while_cond_774:
  r9 = cn_var_当前_0->下一个;
  r10 = r9 != 0;
  if (r10) goto while_body_775; else goto while_exit_776;

  while_body_775:
  r11 = cn_var_当前_0->下一个;
  cn_var_当前_0 = r11;
  goto while_cond_774;

  while_exit_776:
  struct 表达式列表* cn_var_新节点_1;
  r12 = 创建表达式列表();
  cn_var_新节点_1 = r12;
  r13 = cn_var_新节点_1;
  r14 = r13 != 0;
  if (r14) goto if_then_777; else goto if_merge_778;

  if_then_777:
  r15 = cn_var_节点;
  r16 = cn_var_新节点_1;
  goto if_merge_778;

  if_merge_778:
  return;
}

void 释放表达式节点(struct 表达式节点* cn_var_节点) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34;
  struct 表达式节点* r0;
  struct 表达式节点* r35;

  entry:
  r0 = cn_var_节点;
  r1 = r0 == 0;
  if (r1) goto if_then_779; else goto if_merge_780;

  if_then_779:
  return;
  goto if_merge_780;

  if_merge_780:
  r2 = cn_var_节点.左操作数;
  r3 = r2 != 0;
  if (r3) goto if_then_781; else goto if_merge_782;

  if_then_781:
  r4 = cn_var_节点.左操作数;
  释放表达式节点(r4);
  goto if_merge_782;

  if_merge_782:
  r5 = cn_var_节点.右操作数;
  r6 = r5 != 0;
  if (r6) goto if_then_783; else goto if_merge_784;

  if_then_783:
  r7 = cn_var_节点.右操作数;
  释放表达式节点(r7);
  goto if_merge_784;

  if_merge_784:
  r8 = cn_var_节点.操作数;
  r9 = r8 != 0;
  if (r9) goto if_then_785; else goto if_merge_786;

  if_then_785:
  r10 = cn_var_节点.操作数;
  释放表达式节点(r10);
  goto if_merge_786;

  if_merge_786:
  r11 = cn_var_节点.被调函数;
  r12 = r11 != 0;
  if (r12) goto if_then_787; else goto if_merge_788;

  if_then_787:
  r13 = cn_var_节点.被调函数;
  释放表达式节点(r13);
  goto if_merge_788;

  if_merge_788:
  r14 = cn_var_节点.参数列表;
  r15 = r14 != 0;
  if (r15) goto if_then_789; else goto if_merge_790;

  if_then_789:
  r16 = cn_var_节点.参数列表;
  释放表达式列表(r16);
  goto if_merge_790;

  if_merge_790:
  r17 = cn_var_节点.对象;
  r18 = r17 != 0;
  if (r18) goto if_then_791; else goto if_merge_792;

  if_then_791:
  r19 = cn_var_节点.对象;
  释放表达式节点(r19);
  goto if_merge_792;

  if_merge_792:
  r20 = cn_var_节点.数组;
  r21 = r20 != 0;
  if (r21) goto if_then_793; else goto if_merge_794;

  if_then_793:
  r22 = cn_var_节点.数组;
  释放表达式节点(r22);
  goto if_merge_794;

  if_merge_794:
  r23 = cn_var_节点.索引;
  r24 = r23 != 0;
  if (r24) goto if_then_795; else goto if_merge_796;

  if_then_795:
  r25 = cn_var_节点.索引;
  释放表达式节点(r25);
  goto if_merge_796;

  if_merge_796:
  r26 = cn_var_节点.左侧表达式;
  r27 = r26 != 0;
  if (r27) goto if_then_797; else goto if_merge_798;

  if_then_797:
  r28 = cn_var_节点.左侧表达式;
  释放表达式节点(r28);
  goto if_merge_798;

  if_merge_798:
  r29 = cn_var_节点.值;
  r30 = r29 != 0;
  if (r30) goto if_then_799; else goto if_merge_800;

  if_then_799:
  r31 = cn_var_节点.值;
  释放表达式节点(r31);
  goto if_merge_800;

  if_merge_800:
  r32 = cn_var_节点.指针;
  r33 = r32 != 0;
  if (r33) goto if_then_801; else goto if_merge_802;

  if_then_801:
  r34 = cn_var_节点.指针;
  释放表达式节点(r34);
  goto if_merge_802;

  if_merge_802:
  r35 = cn_var_节点;
  释放内存(r35);
  return;
}

void 释放表达式列表(struct 表达式列表* cn_var_列表) {
  long long r1, r2, r3, r4, r5, r6, r7;
  struct 表达式列表* r0;
  struct 表达式列表* r8;

  entry:
  r0 = cn_var_列表;
  r1 = r0 == 0;
  if (r1) goto if_then_803; else goto if_merge_804;

  if_then_803:
  return;
  goto if_merge_804;

  if_merge_804:
  r2 = cn_var_列表.节点;
  r3 = r2 != 0;
  if (r3) goto if_then_805; else goto if_merge_806;

  if_then_805:
  r4 = cn_var_列表.节点;
  释放表达式节点(r4);
  goto if_merge_806;

  if_merge_806:
  r5 = cn_var_列表.下一个;
  r6 = r5 != 0;
  if (r6) goto if_then_807; else goto if_merge_808;

  if_then_807:
  r7 = cn_var_列表.下一个;
  释放表达式列表(r7);
  goto if_merge_808;

  if_merge_808:
  r8 = cn_var_列表;
  释放内存(r8);
  return;
}

