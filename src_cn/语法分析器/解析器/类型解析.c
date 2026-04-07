#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 二元运算符 {
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
enum 逻辑运算符 {
    逻辑运算符_逻辑_或 = 1,
    逻辑运算符_逻辑_与 = 0
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
enum 字面量类型 {
    字面量类型_字面量_字符 = 5,
    字面量类型_字面量_空 = 4,
    字面量类型_字面量_布尔 = 3,
    字面量类型_字面量_字符串 = 2,
    字面量类型_字面量_浮点 = 1,
    字面量类型_字面量_整数 = 0
};
enum 可见性 {
    可见性_可见性_私有 = 2,
    可见性_可见性_公开 = 1,
    可见性_可见性_默认 = 0
};
enum 导入类型 {
    导入类型_导入_从包导入模块 = 5,
    导入类型_导入_从模块通配符 = 4,
    导入类型_导入_从模块选择性 = 3,
    导入类型_导入_别名 = 2,
    导入类型_导入_选择性 = 1,
    导入类型_导入_全量 = 0
};

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
struct 声明节点列表;
struct AST节点 {
};
struct 程序节点;
struct 程序节点 {
    struct AST节点 节点基类;
    struct 声明节点列表* 声明列表;
    struct 函数声明** 函数列表;
    long long 函数个数;
    struct 结构体声明** 结构体列表;
    long long 结构体个数;
    struct 枚举声明** 枚举列表;
    long long 枚举个数;
    struct 类声明** 类列表;
    long long 类个数;
    struct 接口声明** 接口列表;
    long long 接口个数;
    struct 导入声明** 导入列表;
    long long 导入个数;
    struct 变量声明** 全局变量;
    long long 全局变量个数;
    struct 模板函数声明** 模板函数列表;
    long long 模板函数个数;
    struct 模板结构体声明** 模板结构体列表;
    long long 模板结构体个数;
};
struct 类型节点;
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
};
struct 表达式节点;
struct 变量声明;
struct 变量声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 类型节点* 类型;
    struct 表达式节点* 初始值;
    _Bool 是常量;
    _Bool 是静态;
    _Bool 是数组;
    long long 数组维度;
    long long* 数组大小;
    enum 可见性 可见性;
};
struct 块语句;
struct 函数声明;
struct 函数声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
    struct 块语句* 函数体;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是重写;
    _Bool 是虚拟;
    _Bool 是抽象;
    _Bool 是中断处理;
    long long 中断向量;
};
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 结构体声明;
struct 结构体声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 结构体成员** 成员;
    long long 成员个数;
};
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 枚举声明;
struct 枚举声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 枚举成员** 成员;
    long long 成员个数;
};
struct 节点类型 {
};
struct 类成员;
struct 类成员 {
    char* 名称;
    struct 节点类型 类型;
    struct 变量声明* 字段;
    struct 函数声明* 方法;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
};
struct 类声明;
struct 类声明 {
    struct AST节点 节点基类;
    char* 名称;
    char* 基类名称;
    char** 实现接口;
    long long 接口个数;
    struct 类成员** 成员;
    long long 成员个数;
    _Bool 是抽象;
};
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
struct 接口声明;
struct 接口声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 接口方法** 方法;
    long long 方法个数;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
};
struct 模板函数声明;
struct 模板函数声明 {
    struct AST节点 节点基类;
    struct 模板参数** 模板参数;
    long long 参数个数;
    struct 函数声明* 函数声明节点;
};
struct 模板结构体声明;
struct 模板结构体声明 {
    struct AST节点 节点基类;
    struct 模板参数** 模板参数;
    long long 参数个数;
    struct 结构体声明* 结构体声明节点;
};
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
};
struct 导入声明;
struct 导入声明 {
    struct AST节点 节点基类;
    enum 导入类型 类型;
    char* 模块名;
    char* 别名;
    struct 导入成员** 成员;
    long long 成员个数;
    _Bool 是通配符;
    _Bool 是相对导入;
    long long 相对层级;
};
struct 声明节点;
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct 声明节点;
struct 声明节点 {
    struct 节点类型 类型;
    struct 源位置 位置;
    _Bool 是否公开;
    struct 函数声明* 作为函数声明;
    struct 变量声明* 作为变量声明;
    struct 变量声明* 作为常量声明;
    struct 结构体声明* 作为结构体声明;
    struct 枚举声明* 作为枚举声明;
    struct 类声明* 作为类声明;
    struct 接口声明* 作为接口声明;
    struct 导入声明* 作为导入语句;
    struct 模板函数声明* 作为模板函数声明;
    struct 模板结构体声明* 作为模板结构体声明;
    struct 声明节点* 下一个;
};
struct 表达式语句;
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};
struct 语句节点;
struct 如果语句;
struct 如果语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 真分支;
    struct 语句节点* 假分支;
};
struct 当语句;
struct 当语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 循环体;
};
struct 循环语句;
struct 循环语句 {
    struct AST节点 节点基类;
    struct 语句节点* 初始化;
    struct 表达式节点* 条件;
    struct 表达式节点* 更新;
    struct 块语句* 循环体;
};
struct 返回语句;
struct 返回语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 返回值;
};
struct 中断语句;
struct 中断语句 {
    struct AST节点 节点基类;
};
struct 继续语句;
struct 继续语句 {
    struct AST节点 节点基类;
};
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};
struct 选择语句;
struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
};
struct 尝试语句;
struct 尝试语句 {
    struct AST节点 节点基类;
    struct 块语句* 尝试块;
    struct 捕获子句** 捕获子句列表;
    long long 捕获个数;
    struct 块语句* 最终块;
};
struct 抛出语句;
struct 抛出语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 异常表达式;
    char* 异常类型;
    char* 消息;
};
struct 最终语句;
struct 最终语句 {
    struct AST节点 节点基类;
    struct 块语句* 语句体;
};
struct 二元表达式;
struct 二元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 二元运算符 运算符;
};
struct 一元表达式;
struct 一元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 操作数;
    enum 一元运算符 运算符;
    _Bool 是前缀;
};
struct 逻辑表达式;
struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
};
struct 字面量表达式;
struct 字面量表达式 {
    struct AST节点 节点基类;
    enum 字面量类型 类型;
    char* 值;
    long long 整数值;
    double 浮点值;
    _Bool 布尔值;
};
struct 标识符表达式;
struct 标识符表达式 {
    struct AST节点 节点基类;
    char* 名称;
};
struct 函数调用表达式;
struct 函数调用表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 被调函数;
    struct 表达式节点** 参数;
    long long 参数个数;
};
struct 成员访问表达式;
struct 成员访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 对象;
    char* 成员名;
    _Bool 是指针访问;
    _Bool 是静态成员;
    char* 类名;
};
struct 数组访问表达式;
struct 数组访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
};
struct 赋值表达式;
struct 赋值表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 目标;
    struct 表达式节点* 值;
    enum 赋值运算符 运算符;
};
struct 三元表达式;
struct 三元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 表达式节点* 真值;
    struct 表达式节点* 假值;
};
struct 数组字面量表达式;
struct 数组字面量表达式 {
    struct AST节点 节点基类;
    struct 表达式节点** 元素;
    long long 元素个数;
};
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 结构体字面量表达式;
struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};
struct 模板实例化表达式;
struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};

// Forward Declarations - 从导入模块
struct 程序节点* 创建程序节点(void);
void 程序添加声明(struct 程序节点*, struct 声明节点*);

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
  long long r1, r12, r13, r15, r19, r20, r24, r27, r33, r36, r42;
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
  long long r1, r3, r6, r8, r9, r12, r14, r15, r18, r20, r21, r24, r26, r27, r30, r32, r33;
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
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r29;

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
  long long r0, r2, r4, r7, r9, r11, r12, r14, r15, r18, r19, r21, r22, r23;
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
  long long r0, r2, r4, r7, r9, r11, r12, r14, r15, r18, r19, r22, r30, r32;
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
  long long r1, r2, r3, r4, r5, r8, r10, r11, r14, r16, r17, r20, r22, r26, r27, r31, r33, r36, r40, r44;
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

