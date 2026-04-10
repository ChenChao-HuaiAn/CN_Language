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
enum 导入类型 {
    导入类型_导入_从包导入模块 = 5,
    导入类型_导入_从模块通配符 = 4,
    导入类型_导入_从模块选择性 = 3,
    导入类型_导入_别名 = 2,
    导入类型_导入_选择性 = 1,
    导入类型_导入_全量 = 0
};
enum 可见性 {
    可见性_可见性_私有 = 2,
    可见性_可见性_公开 = 1,
    可见性_可见性_默认 = 0
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
enum 类型兼容性 {
    类型兼容性_不兼容 = 3,
    类型兼容性_显式转换 = 2,
    类型兼容性_隐式转换 = 1,
    类型兼容性_完全兼容 = 0
};
enum 类型种类 {
    类型种类_类型_未定义 = 14,
    类型种类_类型_参数 = 13,
    类型种类_类型_函数 = 12,
    类型种类_类型_接口 = 11,
    类型种类_类型_类 = 10,
    类型种类_类型_枚举 = 9,
    类型种类_类型_结构体 = 8,
    类型种类_类型_数组 = 7,
    类型种类_类型_指针 = 6,
    类型种类_类型_字符 = 5,
    类型种类_类型_字符串 = 4,
    类型种类_类型_布尔 = 3,
    类型种类_类型_小数 = 2,
    类型种类_类型_整数 = 1,
    类型种类_类型_空 = 0
};
enum 诊断错误码 {
    诊断错误码_语义_纯虚函数调用 = 43,
    诊断错误码_语义_纯虚函数未实现 = 42,
    诊断错误码_语义_抽象类实例化 = 41,
    诊断错误码_语义_无效自身使用 = 40,
    诊断错误码_语义_静态空类型 = 39,
    诊断错误码_语义_静态非常量初始化 = 38,
    诊断错误码_语义_开关重复情况 = 37,
    诊断错误码_语义_开关非常量情况 = 36,
    诊断错误码_语义_常量非常量初始化 = 35,
    诊断错误码_语义_常量缺少初始化 = 34,
    诊断错误码_语义_访问被拒绝 = 33,
    诊断错误码_语义_成员未找到 = 32,
    诊断错误码_语义_非结构体类型 = 31,
    诊断错误码_语义_不可调用 = 30,
    诊断错误码_语义_无效赋值 = 29,
    诊断错误码_语义_缺少返回语句 = 28,
    诊断错误码_语义_中断继续在循环外 = 27,
    诊断错误码_语义_返回语句在函数外 = 26,
    诊断错误码_语义_参数类型不匹配 = 25,
    诊断错误码_语义_参数数量不匹配 = 24,
    诊断错误码_语义_类型不匹配 = 23,
    诊断错误码_语义_未定义标识符 = 22,
    诊断错误码_语义_重复符号 = 21,
    诊断错误码_语法_无效表达式 = 17,
    诊断错误码_语法_无效比较运算符 = 16,
    诊断错误码_语法_无效变量声明 = 15,
    诊断错误码_语法_无效参数 = 14,
    诊断错误码_语法_无效函数名 = 13,
    诊断错误码_语法_预留特性 = 12,
    诊断错误码_语法_期望标记 = 11,
    诊断错误码_词法_未终止块注释 = 7,
    诊断错误码_词法_字面量溢出 = 6,
    诊断错误码_词法_无效八进制 = 5,
    诊断错误码_词法_无效二进制 = 4,
    诊断错误码_词法_无效十六进制 = 3,
    诊断错误码_词法_无效字符 = 2,
    诊断错误码_词法_未终止字符串 = 1,
    诊断错误码_未知错误 = 0
};
enum 诊断严重级别 {
    诊断严重级别_诊断_警告 = 1,
    诊断严重级别_诊断_错误 = 0
};
enum 符号类型 {
    符号类型_类型参数符号 = 11,
    符号类型_导入符号 = 10,
    符号类型_模块符号 = 9,
    符号类型_类成员符号 = 8,
    符号类型_接口符号 = 7,
    符号类型_类符号 = 6,
    符号类型_枚举成员符号 = 5,
    符号类型_枚举符号 = 4,
    符号类型_结构体符号 = 3,
    符号类型_参数符号 = 2,
    符号类型_函数符号 = 1,
    符号类型_变量符号 = 0
};
enum 作用域类型 {
    作用域类型_循环作用域 = 7,
    作用域类型_模块作用域 = 6,
    作用域类型_枚举作用域 = 5,
    作用域类型_结构体作用域 = 4,
    作用域类型_类作用域 = 3,
    作用域类型_块作用域 = 2,
    作用域类型_函数作用域 = 1,
    作用域类型_全局作用域 = 0
};
enum 编译模式 {
    编译模式_目标模式 = 1,
    编译模式_宿主模式 = 0
};
enum IR操作码 {
    IR操作码_选择指令 = 34,
    IR操作码_PHI指令 = 33,
    IR操作码_结构体初始化指令 = 32,
    IR操作码_成员访问指令 = 31,
    IR操作码_成员指针指令 = 30,
    IR操作码_返回指令 = 29,
    IR操作码_调用指令 = 28,
    IR操作码_条件跳转指令 = 27,
    IR操作码_跳转指令 = 26,
    IR操作码_标签指令 = 25,
    IR操作码_解引用指令 = 24,
    IR操作码_取地址指令 = 23,
    IR操作码_移动指令 = 22,
    IR操作码_存储指令 = 21,
    IR操作码_加载指令 = 20,
    IR操作码_分配指令 = 19,
    IR操作码_大于等于指令 = 18,
    IR操作码_大于指令 = 17,
    IR操作码_小于等于指令 = 16,
    IR操作码_小于指令 = 15,
    IR操作码_不等指令 = 14,
    IR操作码_相等指令 = 13,
    IR操作码_逻辑非指令 = 12,
    IR操作码_取反指令 = 11,
    IR操作码_负号指令 = 10,
    IR操作码_右移指令 = 9,
    IR操作码_左移指令 = 8,
    IR操作码_异或指令 = 7,
    IR操作码_或指令 = 6,
    IR操作码_与指令 = 5,
    IR操作码_取模指令 = 4,
    IR操作码_除法指令 = 3,
    IR操作码_乘法指令 = 2,
    IR操作码_减法指令 = 1,
    IR操作码_加法指令 = 0
};
enum IR操作数种类 {
    IR操作数种类_AST表达式 = 7,
    IR操作数种类_基本块标签 = 6,
    IR操作数种类_全局符号 = 5,
    IR操作数种类_字符串常量 = 4,
    IR操作数种类_小数常量 = 3,
    IR操作数种类_整数常量 = 2,
    IR操作数种类_虚拟寄存器 = 1,
    IR操作数种类_无操作数 = 0
};
enum IR指令种类 {
    IR指令种类_选择指令 = 34,
    IR指令种类_PHI指令 = 33,
    IR指令种类_结构体初始化指令 = 32,
    IR指令种类_成员访问指令 = 31,
    IR指令种类_成员指针指令 = 30,
    IR指令种类_返回指令 = 29,
    IR指令种类_调用指令 = 28,
    IR指令种类_条件跳转指令 = 27,
    IR指令种类_跳转指令 = 26,
    IR指令种类_标签指令 = 25,
    IR指令种类_解引用指令 = 24,
    IR指令种类_取地址指令 = 23,
    IR指令种类_移动指令 = 22,
    IR指令种类_存储指令 = 21,
    IR指令种类_加载指令 = 20,
    IR指令种类_分配指令 = 19,
    IR指令种类_大于等于指令 = 18,
    IR指令种类_大于指令 = 17,
    IR指令种类_小于等于指令 = 16,
    IR指令种类_小于指令 = 15,
    IR指令种类_不等指令 = 14,
    IR指令种类_相等指令 = 13,
    IR指令种类_逻辑非指令 = 12,
    IR指令种类_取反指令 = 11,
    IR指令种类_负号指令 = 10,
    IR指令种类_右移指令 = 9,
    IR指令种类_左移指令 = 8,
    IR指令种类_异或指令 = 7,
    IR指令种类_或指令 = 6,
    IR指令种类_与指令 = 5,
    IR指令种类_取模指令 = 4,
    IR指令种类_除法指令 = 3,
    IR指令种类_乘法指令 = 2,
    IR指令种类_减法指令 = 1,
    IR指令种类_加法指令 = 0
};
enum IR值类型 {
    IR值类型_标签类型 = 10,
    IR值类型_空值类型 = 9,
    IR值类型_函数类型 = 8,
    IR值类型_结构体类型 = 7,
    IR值类型_数组类型 = 6,
    IR值类型_指针类型 = 5,
    IR值类型_字符串类型 = 4,
    IR值类型_布尔类型 = 3,
    IR值类型_小数类型 = 2,
    IR值类型_整数类型 = 1,
    IR值类型_无类型 = 0
};
enum C类型种类 {
    C类型种类_C函数指针类型 = 13,
    C类型种类_C枚举类型 = 12,
    C类型种类_C联合体类型 = 11,
    C类型种类_C结构体类型 = 10,
    C类型种类_C数组类型 = 9,
    C类型种类_C指针类型 = 8,
    C类型种类_C字符串类型 = 7,
    C类型种类_C布尔类型 = 6,
    C类型种类_C字符类型 = 5,
    C类型种类_C浮点类型 = 4,
    C类型种类_C小数类型 = 3,
    C类型种类_C长整数类型 = 2,
    C类型种类_C整数类型 = 1,
    C类型种类_C空类型 = 0
};

// Struct Definitions - 从导入模块
struct 类型节点;
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
};
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 变量声明;
struct 函数声明;
struct 类成员;
struct 类成员 {
    char* 名称;
    enum 节点类型 类型;
    struct 变量声明* 字段;
    struct 函数声明* 方法;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
};
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
};
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
};
struct 声明节点;
struct 声明节点列表;
struct 声明节点列表;
struct 声明节点列表 {
    struct 声明节点* 节点;
    struct 声明节点列表* 下一个;
};
struct 结构体声明;
struct 枚举声明;
struct 类声明;
struct 接口声明;
struct 导入声明;
struct 模板函数声明;
struct 模板结构体声明;
struct 声明节点;
struct 声明节点 {
    enum 节点类型 类型;
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
struct 表达式节点;
struct 块语句;
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
};
struct 语句节点;
struct 语句节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 表达式节点;
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
struct 结构体字面量表达式;
struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};
struct 逻辑表达式;
struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
};
struct 模板实例化表达式;
struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};
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
struct 表达式语句;
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};
struct 块语句;
struct 块语句 {
    struct AST节点 节点基类;
    struct 语句节点** 语句;
    long long 语句个数;
};
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
struct 选择语句;
struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
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
struct 结构体声明;
struct 结构体声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 结构体成员** 成员;
    long long 成员个数;
};
struct 枚举声明;
struct 枚举声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 枚举成员** 成员;
    long long 成员个数;
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
struct 接口声明;
struct 接口声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 接口方法** 方法;
    long long 方法个数;
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
struct 符号表管理器;
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};
struct 符号;
struct 类型信息;
struct 类型信息 {
    enum 类型种类 种类;
    char* 名称;
    long long 大小;
    long long 对齐;
    struct 类型信息* 指向类型;
    long long 指针层级;
    struct 类型信息* 元素类型;
    long long 数组维度;
    long long* 维度大小;
    long long 总元素数;
    struct 类型信息** 参数类型列表;
    long long 参数个数;
    struct 类型信息* 返回类型;
    struct 符号* 类型符号;
    struct 类型信息** 成员类型列表;
    char** 成员名称列表;
    long long 成员个数;
    _Bool 是常量;
    _Bool 是有符号;
    _Bool 已定义;
};
struct 诊断信息;
struct 诊断集合;
struct 诊断集合 {
    struct 诊断信息* 诊断数组;
    long long 容量;
    long long 长度;
    long long 错误计数;
    long long 警告计数;
    long long 最大错误数;
};
struct 诊断信息;
struct 诊断信息 {
    enum 诊断严重级别 严重级别;
    enum 诊断错误码 错误码;
    struct 源位置 位置;
    char* 消息;
};
struct 符号标志;
struct 符号标志 {
    _Bool 是常量;
    _Bool 是静态;
    _Bool 是公开;
    _Bool 是私有;
    _Bool 是保护;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
    _Bool 已初始化;
    _Bool 已使用;
};
struct 作用域;
struct 作用域;
struct 作用域 {
    enum 作用域类型 种类;
    char* 名称;
    struct 源位置 起始位置;
    struct 源位置 结束位置;
    struct 作用域* 父作用域;
    struct 符号* 关联符号;
    struct 符号** 符号表;
    long long 符号数量;
    long long 符号容量;
    struct 作用域** 子作用域列表;
    long long 子作用域数量;
    long long 子作用域容量;
    _Bool 是循环体;
};
struct 符号;
struct 符号 {
    enum 符号类型 种类;
    char* 名称;
    struct 类型节点* 类型信息;
    struct 源位置 定义位置;
    enum 可见性 访问级别;
    struct 符号标志 标志;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
    struct 符号** 成员列表;
    long long 成员个数;
    struct 符号* 父类符号;
    struct 符号** 实现接口;
    long long 接口个数;
    long long 枚举值;
    _Bool 有显式值;
    struct 作用域* 所属作用域;
    struct 作用域* 子作用域;
};
struct 符号表管理器;
struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};
struct IR函数;
struct 全局变量;
struct 目标三元组 {
    char* 架构;
    char* 厂商;
    char* 系统;
    char* 环境;
};
struct IR模块;
struct IR模块 {
    struct IR函数* 首个函数;
    struct IR函数* 末个函数;
    struct 全局变量* 首个全局;
    struct 全局变量* 末个全局;
    struct 目标三元组 目标信息;
    enum 编译模式 编译模式值;
};
struct 基本块;
struct IR操作数 {
    enum IR操作数种类 种类;
    struct 类型节点* 类型信息;
    long long 寄存器编号;
    long long 整数值;
    double 小数值;
    char* 字符串值;
    char* 符号名;
    struct 基本块* 目标块;
    struct 表达式节点* 表达式;
};
struct 全局变量;
struct 全局变量 {
    char* 名称;
    struct 类型节点* 类型信息;
    struct IR操作数 初始值;
    _Bool 是常量;
    struct 全局变量* 下一个;
};
struct 静态变量;
struct IR函数;
struct IR函数 {
    char* 名称;
    struct 类型节点* 返回类型;
    struct IR操作数* 参数列表;
    long long 参数数量;
    struct IR操作数* 局部变量;
    long long 局部变量数量;
    struct 静态变量* 首个静态变量;
    struct 静态变量* 末个静态变量;
    struct 基本块* 首个块;
    struct 基本块* 末个块;
    long long 下个寄存器编号;
    _Bool 是中断处理;
    long long 中断向量号;
    struct IR函数* 下一个;
};
struct 静态变量;
struct 静态变量 {
    char* 名称;
    long long 名称长度;
    struct 类型节点* 类型信息;
    struct IR操作数 初始值;
    struct 静态变量* 下一个;
};
struct IR指令;
struct 基本块节点;
struct 基本块;
struct 基本块 {
    char* 名称;
    struct IR指令* 首条指令;
    struct IR指令* 末条指令;
    struct 基本块节点* 前驱列表;
    struct 基本块节点* 后继列表;
    struct 基本块* 下一个;
    struct 基本块* 上一个;
    _Bool 已访问;
    long long 块编号;
};
struct 基本块节点;
struct 基本块节点 {
    struct 基本块* 块;
    struct 基本块节点* 下一个;
};
struct IR指令;
struct IR指令 {
    enum IR操作码 操作码;
    struct IR操作数 目标;
    struct IR操作数 源操作数1;
    struct IR操作数 源操作数2;
    struct IR操作数* 额外参数;
    long long 额外参数数量;
    struct IR指令* 下一条;
    struct IR指令* 上一条;
    struct 源位置 指令位置;
};
struct 输出缓冲区;
struct 代码生成选项 {
    _Bool 生成调试信息;
    _Bool 生成行号注释;
    _Bool 使用C99标准;
    _Bool 使用C11标准;
    _Bool 生成位置无关代码;
    long long 缩进宽度;
    _Bool 使用制表符缩进;
    char* 输出目录;
    char* 头文件扩展名;
    char* 源文件扩展名;
};
struct C代码生成上下文;
struct C代码生成上下文 {
    struct IR模块* 模块;
    struct IR函数* 当前函数;
    struct 基本块* 当前基本块;
    struct 输出缓冲区* 头文件缓冲区;
    struct 输出缓冲区* 源文件缓冲区;
    struct 输出缓冲区* 函数体缓冲区;
    struct 代码生成选项 选项;
    char** 局部变量名列表;
    long long 局部变量数量;
    char** 寄存器名列表;
    long long 寄存器数量;
    char** 字符串字面量列表;
    long long 字符串字面量数量;
    long long 错误计数;
    long long 警告计数;
};
struct 输出缓冲区;
struct 输出缓冲区 {
    char* 数据;
    long long 容量;
    long long 长度;
    long long 缩进级别;
};

// Forward Declarations - 从导入模块
extern long long cn_var_指针大小;
extern long long cn_var_字符串大小;
extern long long cn_var_布尔大小;
extern long long cn_var_小数大小;
extern long long cn_var_整数大小;
char* 获取类型种类名称(enum 类型种类);
_Bool 是有效的类型种类(enum 类型种类);
struct 类型节点* 类型信息转类型节点(struct 类型信息*);
struct 类型信息* 从函数类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从接口类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从类类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从枚举类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从结构体类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从数组类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从指针类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从基础类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从类型节点推断(struct 类型节点*, struct 类型推断上下文*);
_Bool 是可调用类型(struct 类型信息*);
_Bool 是函数类型(struct 类型信息*);
_Bool 是类类型(struct 类型信息*);
_Bool 是结构体类型(struct 类型信息*);
_Bool 是数组类型(struct 类型信息*);
_Bool 是指针类型(struct 类型信息*);
_Bool 是整数类型(struct 类型信息*);
_Bool 是数值类型(struct 类型信息*);
_Bool 实现接口(struct 类型信息*, struct 类型信息*);
_Bool 是派生类(struct 类型信息*, struct 类型信息*);
enum 类型兼容性 检查类型兼容性(struct 类型信息*, struct 类型信息*);
_Bool 类型相同(struct 类型信息*, struct 类型信息*);
struct 类型信息* 创建未定义类型(char*);
struct 类型信息* 创建接口类型(char*, struct 符号*);
struct 类型信息* 创建类类型(char*, struct 符号*);
struct 类型信息* 创建枚举类型(char*, struct 符号*);
struct 类型信息* 创建结构体类型(char*, struct 符号*);
struct 类型信息* 创建函数类型(struct 类型信息**, long long, struct 类型信息*);
struct 类型信息* 创建数组类型(struct 类型信息*, long long*, long long);
struct 类型信息* 创建指针类型(struct 类型信息*);
struct 类型信息* 创建字符串类型(void);
struct 类型信息* 创建布尔类型(void);
struct 类型信息* 创建小数类型(char*, long long);
struct 类型信息* 创建整数类型(char*, long long, _Bool);
struct 类型信息* 创建空类型(void);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, char*, char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, char*);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, char*);
struct 源位置 创建未知位置(void);
struct 源位置 创建源位置(char*, long long, long long);
void 清空诊断集合(struct 诊断集合*);
void 打印所有诊断(struct 诊断集合*);
void 打印诊断信息(struct 诊断信息*);
char* 获取严重级别字符串(enum 诊断严重级别);
_Bool 应该继续(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
_Bool 有错误(struct 诊断集合*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, char*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 释放诊断集合(struct 诊断集合*);
struct 诊断集合* 创建诊断集合(long long);
void* 数组获取(void*, long long);
long long 获取位置参数个数(void);
char* 获取位置参数(long long);
long long 选项存在(char*);
char* 查找选项(char*);
char* 获取程序名称(void);
char* 获取参数(long long);
long long 获取参数个数(void);
long long 求最小值(long long, long long);
long long 求最大值(long long, long long);
long long 获取绝对值(long long);
char* 读取行(void);
long long 字符串格式化(char*, long long, char*);
char* 字符串格式(char*);
char* 复制字符串副本(char*);
long long 类型大小(long long);
void* 分配内存数组(long long, long long);
struct 符号* 创建变量符号(char*, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建函数符号(char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建参数符号(char*, struct 类型节点*, struct 源位置);
struct 符号* 创建结构体符号(char*, struct 源位置);
struct 符号* 创建枚举符号(char*, struct 源位置);
struct 符号* 创建枚举成员符号(char*, long long, _Bool, struct 源位置);
struct 符号* 创建类符号(char*, struct 源位置, _Bool);
struct 符号* 创建接口符号(char*, struct 源位置);
struct 作用域* 创建作用域(enum 作用域类型, char*, struct 作用域*);
void 销毁作用域(struct 作用域*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁符号表管理器(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, char*, struct 符号*);
void 离开作用域(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
_Bool 在循环体内(struct 符号表管理器*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, char*);
struct 符号* 查找符号(struct 符号表管理器*, char*);
struct 符号* 查找全局符号(struct 符号表管理器*, char*);
struct 符号* 在作用域查找符号(struct 作用域*, char*);
struct 符号* 查找类成员(struct 符号*, char*);
char* 获取符号类型名称(enum 符号类型);
char* 获取作用域类型名称(enum 作用域类型);
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
void 程序添加声明(struct 程序节点*, struct 声明节点*);
struct 程序节点* 创建程序节点(void);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
struct 静态变量* 创建静态变量(char*, struct 类型节点*, struct IR操作数);
void 添加函数(struct IR模块*, struct IR函数*);
void 添加全局变量(struct IR模块*, struct 全局变量*);
struct 全局变量* 创建全局变量(char*, struct 类型节点*, struct IR操作数, _Bool);
struct IR操作数 标签操作数(struct 基本块*);
struct IR操作数 全局符号操作数(char*, struct 类型节点*);
struct IR操作数 字符串常量操作数(char*, struct 类型节点*);
struct IR操作数 小数常量操作数(double, struct 类型节点*);
struct IR操作数 整数常量操作数(long long, struct 类型节点*);
struct IR操作数 寄存器操作数(long long, struct 类型节点*);
struct IR操作数 空操作数(void);
void 释放IR指令(struct IR指令*);
struct IR指令* 创建IR指令(enum IR操作码, struct IR操作数, struct IR操作数, struct IR操作数);
void 连接基本块(struct 基本块*, struct 基本块*);
void 添加指令(struct 基本块*, struct IR指令*);
void 释放基本块列表(struct 基本块节点*);
void 释放基本块(struct 基本块*);
struct 基本块* 创建基本块(char*);
long long 分配寄存器(struct IR函数*);
void 添加静态变量(struct IR函数*, struct 静态变量*);
void 添加基本块(struct IR函数*, struct 基本块*);
void 添加局部变量(struct IR函数*, struct IR操作数);
void 添加函数参数(struct IR函数*, struct IR操作数);
void 释放IR函数(struct IR函数*);
struct IR函数* 创建IR函数(char*, struct 类型节点*);
void 释放IR模块(struct IR模块*);
struct IR模块* 创建IR模块(void);
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_最大字符串数量;
extern long long cn_var_最大变量数量;
extern long long cn_var_最大缩进级别;
extern long long cn_var_默认缓冲区大小;
void 追加小数(struct 输出缓冲区*, double);
void 格式化整数(char*, long long, long long);
struct 代码生成选项 获取默认生成选项(void);
void 释放C代码生成上下文(struct C代码生成上下文*);
struct C代码生成上下文* 创建C代码生成上下文(struct IR模块*, struct 代码生成选项);
void 生成函数代码(struct IR函数*, struct 输出缓冲区*);
void 生成局部变量声明(struct IR函数*, struct 输出缓冲区*);
void 生成基本块代码(struct 基本块*, struct 输出缓冲区*);
void 生成指令代码(struct IR指令*, struct 输出缓冲区*);
void 生成结构体指令(struct IR指令*, struct 输出缓冲区*);
void 生成控制流指令(struct IR指令*, struct 输出缓冲区*);
void 生成内存指令(struct IR指令*, struct 输出缓冲区*);
void 生成比较指令(struct IR指令*, struct 输出缓冲区*);
void 生成一元指令(struct IR指令*, struct 输出缓冲区*);
void 生成算术指令(struct IR指令*, struct 输出缓冲区*);
char* 生成操作数代码(struct IR操作数, struct 输出缓冲区*);
char* 生成类型声明(struct 类型信息*, char*, struct 输出缓冲区*);
char* 类型到C类型(struct 类型信息*, struct 输出缓冲区*);
void 减少缩进(struct 输出缓冲区*);
void 增加缩进(struct 输出缓冲区*);
void 追加换行(struct 输出缓冲区*);
void 追加缩进(struct 输出缓冲区*, long long);
_Bool 追加整数(struct 输出缓冲区*, long long);
_Bool 追加字符(struct 输出缓冲区*, long long);
_Bool 追加字符串(struct 输出缓冲区*, char*);
_Bool 扩展缓冲区(struct 输出缓冲区*, long long);
void 释放输出缓冲区(struct 输出缓冲区*);
struct 输出缓冲区* 创建输出缓冲区(long long);
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;

// CN Language Global Struct Forward Declarations
struct 模块代码生成上下文;

// CN Language Global Struct Definitions
struct 模块代码生成上下文 {
    struct IR模块* 模块;
    struct 输出缓冲区* 头文件缓冲区;
    struct 输出缓冲区* 源文件缓冲区;
    struct 输出缓冲区* 前向声明缓冲区;
    struct 输出缓冲区* 结构体定义缓冲区;
    struct 输出缓冲区* 枚举定义缓冲区;
    struct 输出缓冲区* 全局变量缓冲区;
    struct 输出缓冲区* 函数声明缓冲区;
    struct 输出缓冲区* 函数定义缓冲区;
    struct 代码生成选项 选项;
    char* 模块名称;
    char* 输出文件名;
    long long 生成的结构体数量;
    long long 生成的枚举数量;
    long long 生成的全局变量数量;
    long long 生成的函数数量;
    long long 错误计数;
    long long 警告计数;
};

// Global Variables

// Forward Declarations
char* 生成头文件保护宏(char*, struct 输出缓冲区*);
void 生成头文件开头(struct 模块代码生成上下文*);
void 生成头文件结尾(struct 模块代码生成上下文*);
void 生成源文件开头(struct 模块代码生成上下文*);
void 生成结构体定义(struct 类型信息*, struct 输出缓冲区*);
void 生成结构体前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成枚举定义(struct 类型信息*, struct 符号*, struct 输出缓冲区*);
void 生成枚举前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成类定义(struct 类型信息*, struct 输出缓冲区*);
void 生成类前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成全局变量声明(struct 全局变量*, struct 输出缓冲区*);
void 生成全局变量定义(struct 全局变量*, struct 输出缓冲区*);
void 生成函数声明(struct IR函数*, struct 输出缓冲区*);
struct 模块代码生成上下文* 创建模块代码生成上下文(struct IR模块*, char*, struct 代码生成选项);
void 释放模块代码生成上下文(struct 模块代码生成上下文*);
void 生成模块代码(struct 模块代码生成上下文*);
char* 获取头文件内容(struct 模块代码生成上下文*);
char* 获取源文件内容(struct 模块代码生成上下文*);
_Bool 写入代码文件(struct 模块代码生成上下文*, char*);
long long 获取生成结构体数量(struct 模块代码生成上下文*);
long long 获取生成枚举数量(struct 模块代码生成上下文*);
long long 获取生成全局变量数量(struct 模块代码生成上下文*);
long long 获取生成函数数量(struct 模块代码生成上下文*);
long long 获取错误计数(struct 模块代码生成上下文*);
long long 获取警告计数(struct 模块代码生成上下文*);

char* 生成头文件保护宏(char* cn_var_模块名, struct 输出缓冲区* cn_var_缓冲区) {
  char* r3;
  char* r8;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  _Bool r1;
  _Bool r4;
  _Bool r6;

  entry:
  r0 = cn_var_缓冲区;
  r1 = 追加字符串(r0, "_");
  r2 = cn_var_缓冲区;
  r3 = cn_var_模块名;
  r4 = 追加字符串(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "_H_");
  r7 = cn_var_缓冲区;
  r8 = r7->数据;
  return r8;
}

void 生成头文件开头(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r19, r24, r25, r31, r32, r36;
  char* r10;
  char* r18;
  char* r20;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r2;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 模块代码生成上下文* r9;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r16;
  struct 模块代码生成上下文* r17;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r35;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r14;
  _Bool r22;
  _Bool r26;
  _Bool r29;
  _Bool r33;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3443; else goto if_merge_3444;

  if_then_3443:
  return;
  goto if_merge_3444;

  if_merge_3444:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文;
  r3 = r2->头文件缓冲区;
  cn_var_缓冲区_0 = r3;
  r4 = cn_var_缓冲区_0;
  r5 = 追加字符串(r4, "// ========================================\n");
  r6 = cn_var_缓冲区_0;
  r7 = 追加字符串(r6, "// 自动生成的头文件 - ");
  r8 = cn_var_缓冲区_0;
  r9 = cn_var_上下文;
  r10 = r9->模块名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区_0;
  追加换行(r12);
  r13 = cn_var_缓冲区_0;
  r14 = 追加字符串(r13, "// ========================================\n");
  r15 = cn_var_缓冲区_0;
  追加换行(r15);
  long long cn_var_保护宏_1;
  r16 = 创建输出缓冲区(256);
  cn_var_保护宏_1 = r16;
  r17 = cn_var_上下文;
  r18 = r17->模块名称;
  r19 = cn_var_保护宏_1;
  r20 = 生成头文件保护宏(r18, r19);
  r21 = cn_var_缓冲区_0;
  r22 = 追加字符串(r21, "#ifndef ");
  r23 = cn_var_缓冲区_0;
  r24 = cn_var_保护宏_1;
  r25 = r24.数据;
  r26 = 追加字符串(r23, r25);
  r27 = cn_var_缓冲区_0;
  追加换行(r27);
  r28 = cn_var_缓冲区_0;
  r29 = 追加字符串(r28, "#define ");
  r30 = cn_var_缓冲区_0;
  r31 = cn_var_保护宏_1;
  r32 = r31.数据;
  r33 = 追加字符串(r30, r32);
  r34 = cn_var_缓冲区_0;
  追加换行(r34);
  r35 = cn_var_缓冲区_0;
  追加换行(r35);
  r36 = cn_var_保护宏_1;
  释放输出缓冲区(r36);
  return;
}

void 生成头文件结尾(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r10, r13, r14, r17;
  char* r9;
  char* r11;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r2;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 模块代码生成上下文* r8;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r16;
  _Bool r6;
  _Bool r15;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3445; else goto if_merge_3446;

  if_then_3445:
  return;
  goto if_merge_3446;

  if_merge_3446:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文;
  r3 = r2->头文件缓冲区;
  cn_var_缓冲区_0 = r3;
  r4 = cn_var_缓冲区_0;
  追加换行(r4);
  r5 = cn_var_缓冲区_0;
  r6 = 追加字符串(r5, "#endif // ");
  long long cn_var_保护宏_1;
  r7 = 创建输出缓冲区(256);
  cn_var_保护宏_1 = r7;
  r8 = cn_var_上下文;
  r9 = r8->模块名称;
  r10 = cn_var_保护宏_1;
  r11 = 生成头文件保护宏(r9, r10);
  r12 = cn_var_缓冲区_0;
  r13 = cn_var_保护宏_1;
  r14 = r13.数据;
  r15 = 追加字符串(r12, r14);
  r16 = cn_var_缓冲区_0;
  追加换行(r16);
  r17 = cn_var_保护宏_1;
  释放输出缓冲区(r17);
  return;
}

void 生成源文件开头(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  char* r10;
  char* r20;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r2;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 模块代码生成上下文* r9;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r18;
  struct 模块代码生成上下文* r19;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r14;
  _Bool r17;
  _Bool r21;
  _Bool r23;
  _Bool r26;
  _Bool r28;
  _Bool r30;
  _Bool r32;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3447; else goto if_merge_3448;

  if_then_3447:
  return;
  goto if_merge_3448;

  if_merge_3448:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文;
  r3 = r2->源文件缓冲区;
  cn_var_缓冲区_0 = r3;
  r4 = cn_var_缓冲区_0;
  r5 = 追加字符串(r4, "// ========================================\n");
  r6 = cn_var_缓冲区_0;
  r7 = 追加字符串(r6, "// 自动生成的源文件 - ");
  r8 = cn_var_缓冲区_0;
  r9 = cn_var_上下文;
  r10 = r9->模块名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区_0;
  追加换行(r12);
  r13 = cn_var_缓冲区_0;
  r14 = 追加字符串(r13, "// ========================================\n");
  r15 = cn_var_缓冲区_0;
  追加换行(r15);
  r16 = cn_var_缓冲区_0;
  r17 = 追加字符串(r16, "#include \"");
  r18 = cn_var_缓冲区_0;
  r19 = cn_var_上下文;
  r20 = r19->输出文件名;
  r21 = 追加字符串(r18, r20);
  r22 = cn_var_缓冲区_0;
  r23 = 追加字符串(r22, ".h\"\n");
  r24 = cn_var_缓冲区_0;
  追加换行(r24);
  r25 = cn_var_缓冲区_0;
  r26 = 追加字符串(r25, "#include <cnlang/runtime/runtime.h>\n");
  r27 = cn_var_缓冲区_0;
  r28 = 追加字符串(r27, "#include <cnlang/runtime/stdlib.h>\n");
  r29 = cn_var_缓冲区_0;
  r30 = 追加字符串(r29, "#include <cnlang/runtime/memory.h>\n");
  r31 = cn_var_缓冲区_0;
  r32 = 追加字符串(r31, "#include <cnlang/runtime/io.h>\n");
  r33 = cn_var_缓冲区_0;
  追加换行(r33);
  return;
}

void 生成结构体定义(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5, r14, r16, r17, r21, r30, r35, r36;
  char* r10;
  char* r24;
  void* r20;
  void* r29;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  struct 类型信息* r15;
  struct 输出缓冲区* r18;
  struct 类型信息* r19;
  void* r22;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 类型信息* r28;
  void* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  _Bool r26;
  _Bool r32;
  _Bool r34;
  _Bool r38;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3452; else goto logic_rhs_3451;

  if_then_3449:
  return;
  goto if_merge_3450;

  if_merge_3450:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "struct ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, " {\n");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3453;

  logic_rhs_3451:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 8;
  goto logic_merge_3452;

  logic_merge_3452:
  if (r5) goto if_then_3449; else goto if_merge_3450;

  for_cond_3453:
  r14 = cn_var_i_0;
  r15 = cn_var_类型;
  r16 = r15->成员个数;
  r17 = r14 < r16;
  if (r17) goto for_body_3454; else goto for_exit_3456;

  for_body_3454:
  r18 = cn_var_缓冲区;
  追加缩进(r18, 4);
  r19 = cn_var_类型;
  r20 = r19->成员类型列表;
  r21 = cn_var_i_0;
  r22 = *(void**)cn_rt_array_get_element(r20, r21, 8);
  r23 = cn_var_缓冲区;
  r24 = 类型到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " ");
  r27 = cn_var_缓冲区;
  r28 = cn_var_类型;
  r29 = r28->成员名称列表;
  r30 = cn_var_i_0;
  r31 = *(void**)cn_rt_array_get_element(r29, r30, 8);
  r32 = 追加字符串(r27, r31);
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, ";\n");
  goto for_update_3455;

  for_update_3455:
  r35 = cn_var_i_0;
  r36 = r35 + 1;
  cn_var_i_0 = r36;
  goto for_cond_3453;

  for_exit_3456:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, "};\n");
  r39 = cn_var_缓冲区;
  追加换行(r39);
  return;
}

void 生成结构体前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5;
  char* r10;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3460; else goto logic_rhs_3459;

  if_then_3457:
  return;
  goto if_merge_3458;

  if_merge_3458:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "struct ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, ";\n");

  logic_rhs_3459:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 8;
  goto logic_merge_3460;

  logic_merge_3460:
  if (r5) goto if_then_3457; else goto if_merge_3458;
  return;
}

void 生成枚举定义(struct 类型信息* cn_var_类型, struct 符号* cn_var_符号, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5, r15, r26, r29, r34, r35;
  char* r10;
  char* r20;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  struct 符号* r16;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r18;
  struct 符号* r19;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 符号* r25;
  struct 符号* r28;
  struct 符号* r30;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r38;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  _Bool r21;
  _Bool r23;
  _Bool r27;
  _Bool r32;
  _Bool r37;
  struct 符号 r14;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3464; else goto logic_rhs_3463;

  if_then_3461:
  return;
  goto if_merge_3462;

  if_merge_3462:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "enum ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, " {\n");
  struct 符号* cn_var_成员_0;
  r14 = cn_var_符号;
  r15 = r14.首个成员;
  cn_var_成员_0 = r15;
  long long cn_var_索引_1;
  cn_var_索引_1 = 0;
  goto while_cond_3465;

  logic_rhs_3463:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 9;
  goto logic_merge_3464;

  logic_merge_3464:
  if (r5) goto if_then_3461; else goto if_merge_3462;

  while_cond_3465:
  r16 = cn_var_成员_0;
  if (r16) goto while_body_3466; else goto while_exit_3467;

  while_body_3466:
  r17 = cn_var_缓冲区;
  追加缩进(r17, 4);
  r18 = cn_var_缓冲区;
  r19 = cn_var_成员_0;
  r20 = r19->名称;
  r21 = 追加字符串(r18, r20);
  if (0) goto if_then_3468; else goto if_merge_3469;

  while_exit_3467:
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, "};\n");
  r38 = cn_var_缓冲区;
  追加换行(r38);

  if_then_3468:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, " = ");
  r24 = cn_var_缓冲区;
  r25 = cn_var_成员_0;
  r26 = r25->值;
  r27 = 追加整数(r24, r26);
  goto if_merge_3469;

  if_merge_3469:
  r28 = cn_var_成员_0;
  r29 = r28->下一个成员;
  cn_var_成员_0 = r29;
  r30 = cn_var_成员_0;
  if (r30) goto if_then_3470; else goto if_merge_3471;

  if_then_3470:
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, ",");
  goto if_merge_3471;

  if_merge_3471:
  r33 = cn_var_缓冲区;
  追加换行(r33);
  r34 = cn_var_索引_1;
  r35 = r34 + 1;
  cn_var_索引_1 = r35;
  goto while_cond_3465;
  return;
}

void 生成枚举前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5;
  char* r10;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3475; else goto logic_rhs_3474;

  if_then_3472:
  return;
  goto if_merge_3473;

  if_merge_3473:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "enum ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, ";\n");

  logic_rhs_3474:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 9;
  goto logic_merge_3475;

  logic_merge_3475:
  if (r5) goto if_then_3472; else goto if_merge_3473;
  return;
}

void 生成类定义(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5, r14, r16, r17, r21, r30, r35, r36;
  char* r10;
  char* r24;
  void* r20;
  void* r29;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  struct 类型信息* r15;
  struct 输出缓冲区* r18;
  struct 类型信息* r19;
  void* r22;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 类型信息* r28;
  void* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  _Bool r26;
  _Bool r32;
  _Bool r34;
  _Bool r38;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3479; else goto logic_rhs_3478;

  if_then_3476:
  return;
  goto if_merge_3477;

  if_merge_3477:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "struct ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, " {\n");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3480;

  logic_rhs_3478:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 10;
  goto logic_merge_3479;

  logic_merge_3479:
  if (r5) goto if_then_3476; else goto if_merge_3477;

  for_cond_3480:
  r14 = cn_var_i_0;
  r15 = cn_var_类型;
  r16 = r15->成员个数;
  r17 = r14 < r16;
  if (r17) goto for_body_3481; else goto for_exit_3483;

  for_body_3481:
  r18 = cn_var_缓冲区;
  追加缩进(r18, 4);
  r19 = cn_var_类型;
  r20 = r19->成员类型列表;
  r21 = cn_var_i_0;
  r22 = *(void**)cn_rt_array_get_element(r20, r21, 8);
  r23 = cn_var_缓冲区;
  r24 = 类型到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " ");
  r27 = cn_var_缓冲区;
  r28 = cn_var_类型;
  r29 = r28->成员名称列表;
  r30 = cn_var_i_0;
  r31 = *(void**)cn_rt_array_get_element(r29, r30, 8);
  r32 = 追加字符串(r27, r31);
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, ";\n");
  goto for_update_3482;

  for_update_3482:
  r35 = cn_var_i_0;
  r36 = r35 + 1;
  cn_var_i_0 = r36;
  goto for_cond_3480;

  for_exit_3483:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, "};\n");
  r39 = cn_var_缓冲区;
  追加换行(r39);
  return;
}

void 生成类前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r5;
  char* r10;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 类型信息* r9;
  struct 输出缓冲区* r12;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  enum 类型种类 r4;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3487; else goto logic_rhs_3486;

  if_then_3484:
  return;
  goto if_merge_3485;

  if_merge_3485:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "struct ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_类型;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, ";\n");

  logic_rhs_3486:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 != 10;
  goto logic_merge_3487;

  logic_merge_3487:
  if (r5) goto if_then_3484; else goto if_merge_3485;
  return;
}

void 生成全局变量声明(struct 全局变量* cn_var_全局变量信息, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1;
  char* r7;
  char* r12;
  struct 全局变量* r0;
  struct 输出缓冲区* r2;
  struct 全局变量* r4;
  struct 类型节点* r5;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  struct 全局变量* r11;
  struct 输出缓冲区* r14;
  _Bool r3;
  _Bool r9;
  _Bool r13;
  _Bool r15;

  entry:
  r0 = cn_var_全局变量信息;
  r1 = !r0;
  if (r1) goto if_then_3488; else goto if_merge_3489;

  if_then_3488:
  return;
  goto if_merge_3489;

  if_merge_3489:
  r2 = cn_var_缓冲区;
  r3 = 追加字符串(r2, "extern ");
  r4 = cn_var_全局变量信息;
  r5 = r4->类型信息;
  r6 = cn_var_缓冲区;
  r7 = 类型到C类型(r5, r6);
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, " ");
  r10 = cn_var_缓冲区;
  r11 = cn_var_全局变量信息;
  r12 = r11->名称;
  r13 = 追加字符串(r10, r12);
  r14 = cn_var_缓冲区;
  r15 = 追加字符串(r14, ";\n");
  return;
}

void 生成全局变量定义(struct 全局变量* cn_var_全局变量信息, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r15;
  char* r5;
  char* r10;
  char* r21;
  struct 全局变量* r0;
  struct 全局变量* r2;
  struct 类型节点* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 全局变量* r9;
  struct 全局变量* r12;
  struct 输出缓冲区* r16;
  struct 全局变量* r18;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  _Bool r7;
  _Bool r11;
  _Bool r17;
  _Bool r23;
  struct IR操作数 r13;
  struct IR操作数 r19;
  enum IR操作数种类 r14;

  entry:
  r0 = cn_var_全局变量信息;
  r1 = !r0;
  if (r1) goto if_then_3490; else goto if_merge_3491;

  if_then_3490:
  return;
  goto if_merge_3491;

  if_merge_3491:
  r2 = cn_var_全局变量信息;
  r3 = r2->类型信息;
  r4 = cn_var_缓冲区;
  r5 = 类型到C类型(r3, r4);
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, " ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_全局变量信息;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_全局变量信息;
  r13 = r12->初始值;
  r14 = r13.种类;
  r15 = r14 != 0;
  if (r15) goto if_then_3492; else goto if_merge_3493;

  if_then_3492:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, " = ");
  r18 = cn_var_全局变量信息;
  r19 = r18->初始值;
  r20 = cn_var_缓冲区;
  r21 = 生成操作数代码(r19, r20);
  goto if_merge_3493;

  if_merge_3493:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, ";\n");
  return;
}

void 生成函数声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r14, r16, r17, r18, r19, r24, r27, r30, r31;
  char* r5;
  char* r10;
  char* r29;
  struct IR函数* r0;
  struct IR函数* r2;
  struct 类型节点* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct IR函数* r9;
  struct 输出缓冲区* r12;
  struct IR函数* r15;
  struct 输出缓冲区* r20;
  struct IR函数* r22;
  struct IR操作数* r23;
  void* r25;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  _Bool r21;
  _Bool r33;
  _Bool r35;
  struct 参数 r26;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_3494; else goto if_merge_3495;

  if_then_3494:
  return;
  goto if_merge_3495;

  if_merge_3495:
  r2 = cn_var_函数指针;
  r3 = r2->返回类型;
  r4 = cn_var_缓冲区;
  r5 = 类型到C类型(r3, r4);
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, " ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_函数指针;
  r10 = r9->名称;
  r11 = 追加字符串(r8, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3496;

  for_cond_3496:
  r14 = cn_var_i_0;
  r15 = cn_var_函数指针;
  r16 = r15->参数数量;
  r17 = r14 < r16;
  if (r17) goto for_body_3497; else goto for_exit_3499;

  for_body_3497:
  r18 = cn_var_i_0;
  r19 = r18 > 0;
  if (r19) goto if_then_3500; else goto if_merge_3501;

  for_update_3498:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_3496;

  for_exit_3499:
  if (0) goto if_then_3502; else goto if_merge_3503;

  if_then_3500:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, ", ");
  goto if_merge_3501;

  if_merge_3501:
  struct IR操作数 cn_var_参数_1;
  r22 = cn_var_函数指针;
  r23 = r22->参数列表;
  r24 = cn_var_i_0;
  r25 = *(void**)cn_rt_array_get_element(r23, r24, 8);
  cn_var_参数_1 = r25;
  r26 = cn_var_参数_1;
  r27 = r26.类型信息;
  r28 = cn_var_缓冲区;
  r29 = 类型到C类型(r27, r28);
  goto for_update_3498;

  if_then_3502:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "void");
  goto if_merge_3503;

  if_merge_3503:
  r34 = cn_var_缓冲区;
  r35 = 追加字符串(r34, ");\n");
  return;
}

struct 模块代码生成上下文* 创建模块代码生成上下文(struct IR模块* cn_var_模块, char* cn_var_模块名, struct 代码生成选项 cn_var_选项) {
  long long r0, r4, r6, r8, r10, r12, r14, r16, r18;
  char* r21;
  char* r22;
  char* r23;
  char* r24;
  void* r1;
  struct 模块代码生成上下文* r2;
  struct IR模块* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  struct 模块代码生成上下文* r25;
  struct 代码生成选项 r20;

  entry:
  struct 模块代码生成上下文* cn_var_上下文_0;
  r0 = sizeof(struct 模块代码生成上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_3504; else goto if_merge_3505;

  if_then_3504:
  r3 = cn_var_模块;
  r4 = cn_var_默认缓冲区大小;
  r5 = 创建输出缓冲区(r4);
  r6 = cn_var_默认缓冲区大小;
  r7 = 创建输出缓冲区(r6);
  r8 = cn_var_默认缓冲区大小;
  r9 = 创建输出缓冲区(r8);
  r10 = cn_var_默认缓冲区大小;
  r11 = 创建输出缓冲区(r10);
  r12 = cn_var_默认缓冲区大小;
  r13 = 创建输出缓冲区(r12);
  r14 = cn_var_默认缓冲区大小;
  r15 = 创建输出缓冲区(r14);
  r16 = cn_var_默认缓冲区大小;
  r17 = 创建输出缓冲区(r16);
  r18 = cn_var_默认缓冲区大小;
  r19 = 创建输出缓冲区(r18);
  r20 = cn_var_选项;
  r21 = cn_var_模块名;
  r22 = 复制字符串(r21);
  r23 = cn_var_模块名;
  r24 = 复制字符串(r23);
  goto if_merge_3505;

  if_merge_3505:
  r25 = cn_var_上下文_0;
  return r25;
}

void 释放模块代码生成上下文(struct 模块代码生成上下文* cn_var_上下文) {
  char* r18;
  char* r20;
  char* r22;
  char* r24;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;
  struct 输出缓冲区* r2;
  struct 模块代码生成上下文* r3;
  struct 输出缓冲区* r4;
  struct 模块代码生成上下文* r5;
  struct 输出缓冲区* r6;
  struct 模块代码生成上下文* r7;
  struct 输出缓冲区* r8;
  struct 模块代码生成上下文* r9;
  struct 输出缓冲区* r10;
  struct 模块代码生成上下文* r11;
  struct 输出缓冲区* r12;
  struct 模块代码生成上下文* r13;
  struct 输出缓冲区* r14;
  struct 模块代码生成上下文* r15;
  struct 输出缓冲区* r16;
  struct 模块代码生成上下文* r17;
  struct 模块代码生成上下文* r19;
  struct 模块代码生成上下文* r21;
  struct 模块代码生成上下文* r23;
  struct 模块代码生成上下文* r25;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3506; else goto if_merge_3507;

  if_then_3506:
  r1 = cn_var_上下文;
  r2 = r1->头文件缓冲区;
  释放输出缓冲区(r2);
  r3 = cn_var_上下文;
  r4 = r3->源文件缓冲区;
  释放输出缓冲区(r4);
  r5 = cn_var_上下文;
  r6 = r5->前向声明缓冲区;
  释放输出缓冲区(r6);
  r7 = cn_var_上下文;
  r8 = r7->结构体定义缓冲区;
  释放输出缓冲区(r8);
  r9 = cn_var_上下文;
  r10 = r9->枚举定义缓冲区;
  释放输出缓冲区(r10);
  r11 = cn_var_上下文;
  r12 = r11->全局变量缓冲区;
  释放输出缓冲区(r12);
  r13 = cn_var_上下文;
  r14 = r13->函数声明缓冲区;
  释放输出缓冲区(r14);
  r15 = cn_var_上下文;
  r16 = r15->函数定义缓冲区;
  释放输出缓冲区(r16);
  r17 = cn_var_上下文;
  r18 = r17->模块名称;
  if (r18) goto if_then_3508; else goto if_merge_3509;

  if_merge_3507:

  if_then_3508:
  r19 = cn_var_上下文;
  r20 = r19->模块名称;
  释放内存(r20);
  goto if_merge_3509;

  if_merge_3509:
  r21 = cn_var_上下文;
  r22 = r21->输出文件名;
  if (r22) goto if_then_3510; else goto if_merge_3511;

  if_then_3510:
  r23 = cn_var_上下文;
  r24 = r23->输出文件名;
  释放内存(r24);
  goto if_merge_3511;

  if_merge_3511:
  r25 = cn_var_上下文;
  释放内存(r25);
  goto if_merge_3507;
  return;
}

void 生成模块代码(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0, r2, r5, r21, r22, r36, r37;
  struct 模块代码生成上下文* r1;
  struct 模块代码生成上下文* r3;
  struct IR模块* r4;
  struct 模块代码生成上下文* r6;
  struct 模块代码生成上下文* r7;
  struct 模块代码生成上下文* r8;
  struct IR模块* r9;
  struct 全局变量* r10;
  struct 全局变量* r11;
  struct 全局变量* r12;
  struct 模块代码生成上下文* r13;
  struct 输出缓冲区* r14;
  struct 全局变量* r15;
  struct 模块代码生成上下文* r16;
  struct 输出缓冲区* r17;
  struct 全局变量* r18;
  struct 全局变量* r19;
  struct 模块代码生成上下文* r20;
  struct 模块代码生成上下文* r23;
  struct IR模块* r24;
  struct IR函数* r25;
  struct IR函数* r26;
  struct IR函数* r27;
  struct 模块代码生成上下文* r28;
  struct 输出缓冲区* r29;
  struct IR函数* r30;
  struct 模块代码生成上下文* r31;
  struct 输出缓冲区* r32;
  struct IR函数* r33;
  struct IR函数* r34;
  struct 模块代码生成上下文* r35;
  struct 模块代码生成上下文* r38;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3515; else goto logic_rhs_3514;

  if_then_3512:
  return;
  goto if_merge_3513;

  if_merge_3513:
  r6 = cn_var_上下文;
  生成头文件开头(r6);
  r7 = cn_var_上下文;
  生成源文件开头(r7);
  struct 全局变量* cn_var_全局_0;
  r8 = cn_var_上下文;
  r9 = r8->模块;
  r10 = r9->首个全局;
  cn_var_全局_0 = r10;
  goto while_cond_3516;

  logic_rhs_3514:
  r3 = cn_var_上下文;
  r4 = r3->模块;
  r5 = !r4;
  goto logic_merge_3515;

  logic_merge_3515:
  if (r5) goto if_then_3512; else goto if_merge_3513;

  while_cond_3516:
  r11 = cn_var_全局_0;
  if (r11) goto while_body_3517; else goto while_exit_3518;

  while_body_3517:
  r12 = cn_var_全局_0;
  r13 = cn_var_上下文;
  r14 = r13->头文件缓冲区;
  生成全局变量声明(r12, r14);
  r15 = cn_var_全局_0;
  r16 = cn_var_上下文;
  r17 = r16->源文件缓冲区;
  生成全局变量定义(r15, r17);
  r18 = cn_var_全局_0;
  r19 = r18->下一个;
  cn_var_全局_0 = r19;
  r20 = cn_var_上下文;
  r21 = r20->生成的全局变量数量;
  r22 = r21 + 1;
  goto while_cond_3516;

  while_exit_3518:
  struct IR函数* cn_var_函数指针_1;
  r23 = cn_var_上下文;
  r24 = r23->模块;
  r25 = r24->首个函数;
  cn_var_函数指针_1 = r25;
  goto while_cond_3519;

  while_cond_3519:
  r26 = cn_var_函数指针_1;
  if (r26) goto while_body_3520; else goto while_exit_3521;

  while_body_3520:
  r27 = cn_var_函数指针_1;
  r28 = cn_var_上下文;
  r29 = r28->头文件缓冲区;
  生成函数声明(r27, r29);
  r30 = cn_var_函数指针_1;
  r31 = cn_var_上下文;
  r32 = r31->源文件缓冲区;
  生成函数代码(r30, r32);
  r33 = cn_var_函数指针_1;
  r34 = r33->下一个;
  cn_var_函数指针_1 = r34;
  r35 = cn_var_上下文;
  r36 = r35->生成的函数数量;
  r37 = r36 + 1;
  goto while_cond_3519;

  while_exit_3521:
  r38 = cn_var_上下文;
  生成头文件结尾(r38);
  return;
}

char* 获取头文件内容(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0;
  char* r6;
  struct 模块代码生成上下文* r1;
  struct 模块代码生成上下文* r2;
  struct 输出缓冲区* r3;
  struct 模块代码生成上下文* r4;
  struct 输出缓冲区* r5;

  entry:
  r1 = cn_var_上下文;
  if (r1) goto logic_rhs_3524; else goto logic_merge_3525;

  if_then_3522:
  r4 = cn_var_上下文;
  r5 = r4->头文件缓冲区;
  r6 = r5->数据;
  return r6;
  goto if_merge_3523;

  if_merge_3523:
  return 0;

  logic_rhs_3524:
  r2 = cn_var_上下文;
  r3 = r2->头文件缓冲区;
  goto logic_merge_3525;

  logic_merge_3525:
  if (r3) goto if_then_3522; else goto if_merge_3523;
  return NULL;
}

char* 获取源文件内容(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0;
  char* r6;
  struct 模块代码生成上下文* r1;
  struct 模块代码生成上下文* r2;
  struct 输出缓冲区* r3;
  struct 模块代码生成上下文* r4;
  struct 输出缓冲区* r5;

  entry:
  r1 = cn_var_上下文;
  if (r1) goto logic_rhs_3528; else goto logic_merge_3529;

  if_then_3526:
  r4 = cn_var_上下文;
  r5 = r4->源文件缓冲区;
  r6 = r5->数据;
  return r6;
  goto if_merge_3527;

  if_merge_3527:
  return 0;

  logic_rhs_3528:
  r2 = cn_var_上下文;
  r3 = r2->源文件缓冲区;
  goto logic_merge_3529;

  logic_merge_3529:
  if (r3) goto if_then_3526; else goto if_merge_3527;
  return NULL;
}

_Bool 写入代码文件(struct 模块代码生成上下文* cn_var_上下文, char* cn_var_输出目录) {
  long long r1, r3, r6, r7, r8, r10, r13, r15, r19, r22, r25, r26, r27, r29, r32, r34, r38, r40, r42, r43, r49, r50, r51, r52, r54, r55, r61, r62, r63, r64, r65;
  char* r2;
  char* r5;
  char* r11;
  char* r12;
  char* r14;
  char* r17;
  char* r18;
  char* r20;
  char* r21;
  char* r24;
  char* r30;
  char* r31;
  char* r33;
  char* r36;
  char* r37;
  char* r39;
  char* r46;
  char* r58;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r4;
  void* r9;
  struct 模块代码生成上下文* r16;
  struct 模块代码生成上下文* r23;
  void* r28;
  struct 模块代码生成上下文* r35;
  void* r41;
  struct 模块代码生成上下文* r44;
  struct 输出缓冲区* r45;
  struct 模块代码生成上下文* r47;
  struct 输出缓冲区* r48;
  void* r53;
  struct 模块代码生成上下文* r56;
  struct 输出缓冲区* r57;
  struct 模块代码生成上下文* r59;
  struct 输出缓冲区* r60;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3530; else goto if_merge_3531;

  if_then_3530:
  return 0;
  goto if_merge_3531;

  if_merge_3531:
  long long cn_var_头文件路径_0;
  r2 = cn_var_输出目录;
  r3 = 获取字符串长度(r2);
  r4 = cn_var_上下文;
  r5 = r4->输出文件名;
  r6 = 获取字符串长度(r5);
  r7 = r3 + r6;
  r8 = r7 + 4;
  r9 = 分配内存(r8);
  cn_var_头文件路径_0 = r9;
  r10 = cn_var_头文件路径_0;
  r11 = cn_var_输出目录;
  r12 = 连接字符串(r10, r11);
  r13 = cn_var_头文件路径_0;
  r14 = 连接字符串(r13, "/");
  r15 = cn_var_头文件路径_0;
  r16 = cn_var_上下文;
  r17 = r16->输出文件名;
  r18 = 连接字符串(r15, r17);
  r19 = cn_var_头文件路径_0;
  r20 = 连接字符串(r19, ".h");
  long long cn_var_源文件路径_1;
  r21 = cn_var_输出目录;
  r22 = 获取字符串长度(r21);
  r23 = cn_var_上下文;
  r24 = r23->输出文件名;
  r25 = 获取字符串长度(r24);
  r26 = r22 + r25;
  r27 = r26 + 4;
  r28 = 分配内存(r27);
  cn_var_源文件路径_1 = r28;
  r29 = cn_var_源文件路径_1;
  r30 = cn_var_输出目录;
  r31 = 连接字符串(r29, r30);
  r32 = cn_var_源文件路径_1;
  r33 = 连接字符串(r32, "/");
  r34 = cn_var_源文件路径_1;
  r35 = cn_var_上下文;
  r36 = r35->输出文件名;
  r37 = 连接字符串(r34, r36);
  r38 = cn_var_源文件路径_1;
  r39 = 连接字符串(r38, ".c");
  long long cn_var_头文件_2;
  r40 = cn_var_头文件路径_0;
  r41 = 打开文件(r40, "w");
  cn_var_头文件_2 = r41;
  r42 = cn_var_头文件_2;
  if (r42) goto if_then_3532; else goto if_merge_3533;

  if_then_3532:
  r43 = cn_var_头文件_2;
  r44 = cn_var_上下文;
  r45 = r44->头文件缓冲区;
  r46 = r45->数据;
  r47 = cn_var_上下文;
  r48 = r47->头文件缓冲区;
  r49 = r48->长度;
  r50 = 写入文件(r43, r46, r49);
  r51 = cn_var_头文件_2;
  关闭文件(r51);
  goto if_merge_3533;

  if_merge_3533:
  long long cn_var_源文件_3;
  r52 = cn_var_源文件路径_1;
  r53 = 打开文件(r52, "w");
  cn_var_源文件_3 = r53;
  r54 = cn_var_源文件_3;
  if (r54) goto if_then_3534; else goto if_merge_3535;

  if_then_3534:
  r55 = cn_var_源文件_3;
  r56 = cn_var_上下文;
  r57 = r56->源文件缓冲区;
  r58 = r57->数据;
  r59 = cn_var_上下文;
  r60 = r59->源文件缓冲区;
  r61 = r60->长度;
  r62 = 写入文件(r55, r58, r61);
  r63 = cn_var_源文件_3;
  关闭文件(r63);
  goto if_merge_3535;

  if_merge_3535:
  r64 = cn_var_头文件路径_0;
  释放内存(r64);
  r65 = cn_var_源文件路径_1;
  释放内存(r65);
  return 1;
}

long long 获取生成结构体数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3536; else goto if_merge_3537;

  if_then_3536:
  r1 = cn_var_上下文;
  r2 = r1->生成的结构体数量;
  return r2;
  goto if_merge_3537;

  if_merge_3537:
  return 0;
}

long long 获取生成枚举数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3538; else goto if_merge_3539;

  if_then_3538:
  r1 = cn_var_上下文;
  r2 = r1->生成的枚举数量;
  return r2;
  goto if_merge_3539;

  if_merge_3539:
  return 0;
}

long long 获取生成全局变量数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3540; else goto if_merge_3541;

  if_then_3540:
  r1 = cn_var_上下文;
  r2 = r1->生成的全局变量数量;
  return r2;
  goto if_merge_3541;

  if_merge_3541:
  return 0;
}

long long 获取生成函数数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3542; else goto if_merge_3543;

  if_then_3542:
  r1 = cn_var_上下文;
  r2 = r1->生成的函数数量;
  return r2;
  goto if_merge_3543;

  if_merge_3543:
  return 0;
}

long long 获取错误计数(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3544; else goto if_merge_3545;

  if_then_3544:
  r1 = cn_var_上下文;
  r2 = r1->错误计数;
  return r2;
  goto if_merge_3545;

  if_merge_3545:
  return 0;
}

long long 获取警告计数(struct 模块代码生成上下文* cn_var_上下文) {
  long long r2;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3546; else goto if_merge_3547;

  if_then_3546:
  r1 = cn_var_上下文;
  r2 = r1->警告计数;
  return r2;
  goto if_merge_3547;

  if_merge_3547:
  return 0;
}

