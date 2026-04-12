#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元类型枚举_关键字_如果 = 0,
    词元类型枚举_关键字_否则 = 1,
    词元类型枚举_关键字_当 = 2,
    词元类型枚举_关键字_循环 = 3,
    词元类型枚举_关键字_返回 = 4,
    词元类型枚举_关键字_中断 = 5,
    词元类型枚举_关键字_继续 = 6,
    词元类型枚举_关键字_选择 = 7,
    词元类型枚举_关键字_情况 = 8,
    词元类型枚举_关键字_默认 = 9,
    词元类型枚举_关键字_整数 = 10,
    词元类型枚举_关键字_小数 = 11,
    词元类型枚举_关键字_字符串 = 12,
    词元类型枚举_关键字_布尔 = 13,
    词元类型枚举_关键字_空类型 = 14,
    词元类型枚举_关键字_结构体 = 15,
    词元类型枚举_关键字_枚举 = 16,
    词元类型枚举_关键字_函数 = 17,
    词元类型枚举_关键字_变量 = 18,
    词元类型枚举_关键字_导入 = 19,
    词元类型枚举_关键字_从 = 20,
    词元类型枚举_关键字_公开 = 21,
    词元类型枚举_关键字_私有 = 22,
    词元类型枚举_关键字_静态 = 23,
    词元类型枚举_关键字_真 = 24,
    词元类型枚举_关键字_假 = 25,
    词元类型枚举_关键字_无 = 26,
    词元类型枚举_关键字_类 = 27,
    词元类型枚举_关键字_接口 = 28,
    词元类型枚举_关键字_保护 = 29,
    词元类型枚举_关键字_虚拟 = 30,
    词元类型枚举_关键字_重写 = 31,
    词元类型枚举_关键字_抽象 = 32,
    词元类型枚举_关键字_实现 = 33,
    词元类型枚举_关键字_自身 = 34,
    词元类型枚举_关键字_基类 = 35,
    词元类型枚举_关键字_尝试 = 36,
    词元类型枚举_关键字_捕获 = 37,
    词元类型枚举_关键字_抛出 = 38,
    词元类型枚举_关键字_最终 = 39,
    词元类型枚举_标识符 = 40,
    词元类型枚举_整数字面量 = 41,
    词元类型枚举_浮点字面量 = 42,
    词元类型枚举_字符串字面量 = 43,
    词元类型枚举_字符字面量 = 44,
    词元类型枚举_加号 = 45,
    词元类型枚举_减号 = 46,
    词元类型枚举_星号 = 47,
    词元类型枚举_斜杠 = 48,
    词元类型枚举_百分号 = 49,
    词元类型枚举_等于 = 50,
    词元类型枚举_赋值 = 51,
    词元类型枚举_不等于 = 52,
    词元类型枚举_小于 = 53,
    词元类型枚举_小于等于 = 54,
    词元类型枚举_大于 = 55,
    词元类型枚举_大于等于 = 56,
    词元类型枚举_逻辑与 = 57,
    词元类型枚举_逻辑或 = 58,
    词元类型枚举_逻辑非 = 59,
    词元类型枚举_按位与 = 60,
    词元类型枚举_按位或 = 61,
    词元类型枚举_按位异或 = 62,
    词元类型枚举_按位取反 = 63,
    词元类型枚举_左移 = 64,
    词元类型枚举_右移 = 65,
    词元类型枚举_自增 = 66,
    词元类型枚举_自减 = 67,
    词元类型枚举_箭头 = 68,
    词元类型枚举_左括号 = 69,
    词元类型枚举_右括号 = 70,
    词元类型枚举_左大括号 = 71,
    词元类型枚举_右大括号 = 72,
    词元类型枚举_左方括号 = 73,
    词元类型枚举_右方括号 = 74,
    词元类型枚举_分号 = 75,
    词元类型枚举_逗号 = 76,
    词元类型枚举_点 = 77,
    词元类型枚举_冒号 = 78,
    词元类型枚举_问号 = 79,
    词元类型枚举_结束 = 80,
    词元类型枚举_错误 = 81
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
    导入类型_导入_全量 = 0,
    导入类型_导入_选择性 = 1,
    导入类型_导入_别名 = 2,
    导入类型_导入_从模块选择性 = 3,
    导入类型_导入_从模块通配符 = 4,
    导入类型_导入_从包导入模块 = 5
};
enum 可见性 {
    可见性_可见性_默认 = 0,
    可见性_可见性_公开 = 1,
    可见性_可见性_私有 = 2
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
enum 类型兼容性 {
    类型兼容性_完全兼容 = 0,
    类型兼容性_隐式转换 = 1,
    类型兼容性_显式转换 = 2,
    类型兼容性_不兼容 = 3
};
enum 类型种类 {
    类型种类_类型_空 = 0,
    类型种类_类型_整数 = 1,
    类型种类_类型_小数 = 2,
    类型种类_类型_布尔 = 3,
    类型种类_类型_字符串 = 4,
    类型种类_类型_字符 = 5,
    类型种类_类型_指针 = 6,
    类型种类_类型_数组 = 7,
    类型种类_类型_结构体 = 8,
    类型种类_类型_枚举 = 9,
    类型种类_类型_类 = 10,
    类型种类_类型_接口 = 11,
    类型种类_类型_函数 = 12,
    类型种类_类型_参数 = 13,
    类型种类_类型_未定义 = 14
};
enum 诊断错误码 {
    诊断错误码_未知错误 = 0,
    诊断错误码_词法_未终止字符串 = 1,
    诊断错误码_词法_无效字符 = 2,
    诊断错误码_词法_无效十六进制 = 3,
    诊断错误码_词法_无效二进制 = 4,
    诊断错误码_词法_无效八进制 = 5,
    诊断错误码_词法_字面量溢出 = 6,
    诊断错误码_词法_未终止块注释 = 7,
    诊断错误码_语法_期望标记 = 11,
    诊断错误码_语法_预留特性 = 12,
    诊断错误码_语法_无效函数名 = 13,
    诊断错误码_语法_无效参数 = 14,
    诊断错误码_语法_无效变量声明 = 15,
    诊断错误码_语法_无效比较运算符 = 16,
    诊断错误码_语法_无效表达式 = 17,
    诊断错误码_语义_重复符号 = 21,
    诊断错误码_语义_未定义标识符 = 22,
    诊断错误码_语义_类型不匹配 = 23,
    诊断错误码_语义_参数数量不匹配 = 24,
    诊断错误码_语义_参数类型不匹配 = 25,
    诊断错误码_语义_返回语句在函数外 = 26,
    诊断错误码_语义_中断继续在循环外 = 27,
    诊断错误码_语义_缺少返回语句 = 28,
    诊断错误码_语义_无效赋值 = 29,
    诊断错误码_语义_不可调用 = 30,
    诊断错误码_语义_非结构体类型 = 31,
    诊断错误码_语义_成员未找到 = 32,
    诊断错误码_语义_访问被拒绝 = 33,
    诊断错误码_语义_常量缺少初始化 = 34,
    诊断错误码_语义_常量非常量初始化 = 35,
    诊断错误码_语义_开关非常量情况 = 36,
    诊断错误码_语义_开关重复情况 = 37,
    诊断错误码_语义_静态非常量初始化 = 38,
    诊断错误码_语义_静态空类型 = 39,
    诊断错误码_语义_无效自身使用 = 40,
    诊断错误码_语义_抽象类实例化 = 41,
    诊断错误码_语义_纯虚函数未实现 = 42,
    诊断错误码_语义_纯虚函数调用 = 43
};
enum 诊断严重级别 {
    诊断严重级别_诊断_错误 = 0,
    诊断严重级别_诊断_警告 = 1
};
enum 符号类型 {
    符号类型_变量符号 = 0,
    符号类型_函数符号 = 1,
    符号类型_参数符号 = 2,
    符号类型_结构体符号 = 3,
    符号类型_枚举符号 = 4,
    符号类型_枚举成员符号 = 5,
    符号类型_类符号 = 6,
    符号类型_接口符号 = 7,
    符号类型_类成员符号 = 8,
    符号类型_模块符号 = 9,
    符号类型_导入符号 = 10,
    符号类型_类型参数符号 = 11
};
enum 作用域类型 {
    作用域类型_全局作用域 = 0,
    作用域类型_函数作用域 = 1,
    作用域类型_块作用域 = 2,
    作用域类型_类作用域 = 3,
    作用域类型_结构体作用域 = 4,
    作用域类型_枚举作用域 = 5,
    作用域类型_模块作用域 = 6,
    作用域类型_循环作用域 = 7
};
enum 编译模式 {
    编译模式_宿主模式 = 0,
    编译模式_目标模式 = 1
};
enum IR操作码 {
    IR操作码_加法指令 = 0,
    IR操作码_减法指令 = 1,
    IR操作码_乘法指令 = 2,
    IR操作码_除法指令 = 3,
    IR操作码_取模指令 = 4,
    IR操作码_与指令 = 5,
    IR操作码_或指令 = 6,
    IR操作码_异或指令 = 7,
    IR操作码_左移指令 = 8,
    IR操作码_右移指令 = 9,
    IR操作码_负号指令 = 10,
    IR操作码_取反指令 = 11,
    IR操作码_逻辑非指令 = 12,
    IR操作码_相等指令 = 13,
    IR操作码_不等指令 = 14,
    IR操作码_小于指令 = 15,
    IR操作码_小于等于指令 = 16,
    IR操作码_大于指令 = 17,
    IR操作码_大于等于指令 = 18,
    IR操作码_分配指令 = 19,
    IR操作码_加载指令 = 20,
    IR操作码_存储指令 = 21,
    IR操作码_移动指令 = 22,
    IR操作码_取地址指令 = 23,
    IR操作码_解引用指令 = 24,
    IR操作码_标签指令 = 25,
    IR操作码_跳转指令 = 26,
    IR操作码_条件跳转指令 = 27,
    IR操作码_调用指令 = 28,
    IR操作码_返回指令 = 29,
    IR操作码_成员指针指令 = 30,
    IR操作码_成员访问指令 = 31,
    IR操作码_结构体初始化指令 = 32,
    IR操作码_PHI指令 = 33,
    IR操作码_选择指令 = 34
};
enum IR操作数种类 {
    IR操作数种类_无操作数 = 0,
    IR操作数种类_虚拟寄存器 = 1,
    IR操作数种类_整数常量 = 2,
    IR操作数种类_小数常量 = 3,
    IR操作数种类_字符串常量 = 4,
    IR操作数种类_全局符号 = 5,
    IR操作数种类_基本块标签 = 6,
    IR操作数种类_AST表达式 = 7
};
enum IR指令种类 {
    IR指令种类_加法指令 = 0,
    IR指令种类_减法指令 = 1,
    IR指令种类_乘法指令 = 2,
    IR指令种类_除法指令 = 3,
    IR指令种类_取模指令 = 4,
    IR指令种类_与指令 = 5,
    IR指令种类_或指令 = 6,
    IR指令种类_异或指令 = 7,
    IR指令种类_左移指令 = 8,
    IR指令种类_右移指令 = 9,
    IR指令种类_负号指令 = 10,
    IR指令种类_取反指令 = 11,
    IR指令种类_逻辑非指令 = 12,
    IR指令种类_相等指令 = 13,
    IR指令种类_不等指令 = 14,
    IR指令种类_小于指令 = 15,
    IR指令种类_小于等于指令 = 16,
    IR指令种类_大于指令 = 17,
    IR指令种类_大于等于指令 = 18,
    IR指令种类_分配指令 = 19,
    IR指令种类_加载指令 = 20,
    IR指令种类_存储指令 = 21,
    IR指令种类_移动指令 = 22,
    IR指令种类_取地址指令 = 23,
    IR指令种类_解引用指令 = 24,
    IR指令种类_标签指令 = 25,
    IR指令种类_跳转指令 = 26,
    IR指令种类_条件跳转指令 = 27,
    IR指令种类_调用指令 = 28,
    IR指令种类_返回指令 = 29,
    IR指令种类_成员指针指令 = 30,
    IR指令种类_成员访问指令 = 31,
    IR指令种类_结构体初始化指令 = 32,
    IR指令种类_PHI指令 = 33,
    IR指令种类_选择指令 = 34
};
enum IR值类型 {
    IR值类型_无类型 = 0,
    IR值类型_整数类型 = 1,
    IR值类型_小数类型 = 2,
    IR值类型_布尔类型 = 3,
    IR值类型_字符串类型 = 4,
    IR值类型_指针类型 = 5,
    IR值类型_数组类型 = 6,
    IR值类型_结构体类型 = 7,
    IR值类型_函数类型 = 8,
    IR值类型_空值类型 = 9,
    IR值类型_标签类型 = 10
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
struct 结构体声明;
struct 枚举声明;
struct 类声明;
struct 接口声明;
struct 导入声明;
struct 模板函数声明;
struct 模板结构体声明;
struct 声明节点;
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
struct 声明节点列表;
struct 声明节点列表;
struct 声明节点列表 {
    struct 声明节点* 节点;
    struct 声明节点列表* 下一个;
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
struct 表达式列表;
struct 表达式列表;
struct 表达式列表 {
    struct 表达式节点* 节点;
    struct 表达式列表* 下一个;
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
extern long long cn_var_符号大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_作用域指针大小;
void 释放表达式列表(struct 表达式列表*);
void 释放表达式节点(struct 表达式节点*);
void 表达式列表添加(struct 表达式列表*, struct 表达式节点*);
struct 表达式列表* 创建表达式列表(void);
struct 表达式节点* 创建基类访问表达式(char*);
struct 表达式节点* 创建自身表达式(void);
struct 表达式节点* 创建空值表达式(void);
struct 表达式节点* 创建标识符表达式(void);
struct 表达式节点* 创建字面量表达式(void);
struct 表达式节点* 创建数组访问表达式(void);
struct 表达式节点* 创建成员访问表达式(void);
struct 表达式节点* 创建函数调用表达式(void);
struct 表达式节点* 创建一元表达式(void);
struct 表达式节点* 创建三元表达式(void);
struct 表达式节点* 创建二元表达式(void);
struct 表达式节点* 创建赋值表达式(void);
struct 表达式节点* 创建表达式节点(enum 节点类型);
void 程序添加声明(struct 程序节点*, struct 声明节点*);
struct 程序节点* 创建程序节点(void);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
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
extern long long cn_var_作用域指针大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号大小;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;

// CN Language Enum Definitions
enum C类型种类 {
    C类型种类_C空类型,
    C类型种类_C整数类型,
    C类型种类_C长整数类型,
    C类型种类_C小数类型,
    C类型种类_C浮点类型,
    C类型种类_C字符类型,
    C类型种类_C布尔类型,
    C类型种类_C字符串类型,
    C类型种类_C指针类型,
    C类型种类_C数组类型,
    C类型种类_C结构体类型,
    C类型种类_C联合体类型,
    C类型种类_C枚举类型,
    C类型种类_C函数指针类型
};

// CN Language Global Struct Forward Declarations
struct 代码生成选项;
struct 输出缓冲区;
struct C代码生成上下文;

// CN Language Global Struct Definitions
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

struct 输出缓冲区 {
    char* 数据;
    long long 容量;
    long long 长度;
    long long 缩进级别;
};

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

// Global Variables
long long cn_var_默认缓冲区大小 = 4096;
long long cn_var_最大缩进级别 = 100;
long long cn_var_最大变量数量 = 1024;
long long cn_var_最大字符串数量 = 256;

// Forward Declarations
struct 输出缓冲区* 创建输出缓冲区(long long);
void 释放输出缓冲区(struct 输出缓冲区*);
_Bool 扩展缓冲区(struct 输出缓冲区*, long long);
_Bool 追加字符串(struct 输出缓冲区*, char*);
_Bool 追加字符(struct 输出缓冲区*, long long);
_Bool 追加整数(struct 输出缓冲区*, long long);
void 追加缩进(struct 输出缓冲区*, long long);
void 追加换行(struct 输出缓冲区*);
void 增加缩进(struct 输出缓冲区*);
void 减少缩进(struct 输出缓冲区*);
char* 类型到C类型(struct 类型信息*, struct 输出缓冲区*);
char* 生成类型声明(struct 类型信息*, char*, struct 输出缓冲区*);
char* 类型节点到C类型(struct 类型节点*, struct 输出缓冲区*);
char* 生成操作数代码(struct IR操作数, struct 输出缓冲区*);
void 生成算术指令(struct IR指令*, struct 输出缓冲区*);
void 生成一元指令(struct IR指令*, struct 输出缓冲区*);
void 生成比较指令(struct IR指令*, struct 输出缓冲区*);
void 生成内存指令(struct IR指令*, struct 输出缓冲区*);
void 生成控制流指令(struct IR指令*, struct 输出缓冲区*);
void 生成结构体指令(struct IR指令*, struct 输出缓冲区*);
void 生成指令代码(struct IR指令*, struct 输出缓冲区*);
void 生成基本块代码(struct 基本块*, struct 输出缓冲区*);
void 生成局部变量声明(struct IR函数*, struct 输出缓冲区*);
void 生成函数代码(struct IR函数*, struct 输出缓冲区*);
struct C代码生成上下文* 创建C代码生成上下文(struct IR模块*, struct 代码生成选项);
void 释放C代码生成上下文(struct C代码生成上下文*);
struct 代码生成选项 获取默认生成选项();
void 格式化整数(char*, long long, long long);
void 追加小数(struct 输出缓冲区*, double);

struct 输出缓冲区* 创建输出缓冲区(long long cn_var_初始容量) {
  long long r0, r3, r5;
  char* r7;
  char* r9;
  void* r1;
  struct 输出缓冲区* r2;
  void* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;

  entry:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r0 = sizeof(struct 输出缓冲区);
  r1 = 分配内存(r0);
  cn_var_缓冲区_0 = r1;
  r2 = cn_var_缓冲区_0;
  if (r2) goto if_then_3219; else goto if_merge_3220;

  if_then_3219:
  r3 = cn_var_初始容量;
  r4 = 分配内存(r3);
  r5 = cn_var_初始容量;
  r6 = cn_var_缓冲区_0;
  r7 = r6->数据;
  if (r7) goto if_then_3221; else goto if_merge_3222;

  if_merge_3220:
  r10 = cn_var_缓冲区_0;
  return r10;

  if_then_3221:
  r8 = cn_var_缓冲区_0;
  r9 = r8->数据;
    { long long _tmp_i2 = 0; cn_rt_array_set_element(r9, 0, &_tmp_i2, 8); }
  goto if_merge_3222;

  if_merge_3222:
  goto if_merge_3220;
  return NULL;
}

void 释放输出缓冲区(struct 输出缓冲区* cn_var_缓冲区) {
  char* r2;
  char* r4;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;

  entry:
  r0 = cn_var_缓冲区;
  if (r0) goto if_then_3223; else goto if_merge_3224;

  if_then_3223:
  r1 = cn_var_缓冲区;
  r2 = r1->数据;
  if (r2) goto if_then_3225; else goto if_merge_3226;

  if_merge_3224:

  if_then_3225:
  r3 = cn_var_缓冲区;
  r4 = r3->数据;
  释放内存(r4);
  goto if_merge_3226;

  if_merge_3226:
  r5 = cn_var_缓冲区;
  释放内存(r5);
  goto if_merge_3224;
  return;
}

_Bool 扩展缓冲区(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_新容量) {
  long long r0, r2, r3, r5, r6, r9, r13;
  char* r8;
  char* r11;
  char* r12;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r7;
  void* r10;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_3230; else goto logic_rhs_3229;

  if_then_3227:
  return 0;
  goto if_merge_3228;

  if_merge_3228:
  char* cn_var_新数据_0;
  r7 = cn_var_缓冲区;
  r8 = r7->数据;
  r9 = cn_var_新容量;
  r10 = 重新分配内存(r8, r9);
  cn_var_新数据_0 = r10;
  r11 = cn_var_新数据_0;
  if (r11) goto if_then_3231; else goto if_merge_3232;

  logic_rhs_3229:
  r3 = cn_var_新容量;
  r4 = cn_var_缓冲区;
  r5 = r4->容量;
  r6 = r3 <= r5;
  goto logic_merge_3230;

  logic_merge_3230:
  if (r6) goto if_then_3227; else goto if_merge_3228;

  if_then_3231:
  r12 = cn_var_新数据_0;
  r13 = cn_var_新容量;
  return 1;
  goto if_merge_3232;

  if_merge_3232:
  return 0;
}

_Bool 追加字符串(struct 输出缓冲区* cn_var_缓冲区, char* cn_var_字符串值) {
  long long r0, r2, r4, r6, r8, r10, r11, r12, r14, r15, r17, r18, r19, r20, r21, r22, r23, r25, r27, r31, r32, r37, r39, r43;
  char* r3;
  char* r5;
  char* r29;
  char* r33;
  char* r41;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  void* r35;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r42;
  _Bool r26;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_3236; else goto logic_rhs_3235;

  if_then_3233:
  return 0;
  goto if_merge_3234;

  if_merge_3234:
  long long cn_var_字符串长度_0;
  r5 = cn_var_字符串值;
  r6 = 获取字符串长度(r5);
  cn_var_字符串长度_0 = r6;
  long long cn_var_需要容量_1;
  r7 = cn_var_缓冲区;
  r8 = r7->长度;
  r9 = cn_var_字符串长度_0;
  r10 = r8 + r9;
  r11 = r10 + 1;
  cn_var_需要容量_1 = r11;
  r12 = cn_var_需要容量_1;
  r13 = cn_var_缓冲区;
  r14 = r13->容量;
  r15 = r12 > r14;
  if (r15) goto if_then_3237; else goto if_merge_3238;

  logic_rhs_3235:
  r3 = cn_var_字符串值;
  r4 = !r3;
  goto logic_merge_3236;

  logic_merge_3236:
  if (r4) goto if_then_3233; else goto if_merge_3234;

  if_then_3237:
  long long cn_var_新容量_2;
  r16 = cn_var_缓冲区;
  r17 = r16->容量;
  r18 = r17 << 1;
  cn_var_新容量_2 = r18;
  goto while_cond_3239;

  if_merge_3238:
  r28 = cn_var_缓冲区;
  r29 = r28->数据;
  r30 = cn_var_缓冲区;
  r31 = r30->长度;
  r32 = r29 + r31;
  r33 = cn_var_字符串值;
  r34 = cn_var_字符串长度_0;
  r35 = 复制内存(r32, r33, r34);
  r36 = cn_var_缓冲区;
  r37 = r36->长度;
  r38 = cn_var_字符串长度_0;
  r39 = r37 + r38;
  r40 = cn_var_缓冲区;
  r41 = r40->数据;
  r42 = cn_var_缓冲区;
  r43 = r42->长度;
    { long long _tmp_i3 = 0; cn_rt_array_set_element(r41, r43, &_tmp_i3, 8); }
  return 1;

  while_cond_3239:
  r19 = cn_var_新容量_2;
  r20 = cn_var_需要容量_1;
  r21 = r19 < r20;
  if (r21) goto while_body_3240; else goto while_exit_3241;

  while_body_3240:
  r22 = cn_var_新容量_2;
  r23 = r22 << 1;
  cn_var_新容量_2 = r23;
  goto while_cond_3239;

  while_exit_3241:
  r24 = cn_var_缓冲区;
  r25 = cn_var_新容量_2;
  r26 = 扩展缓冲区(r24, r25);
  r27 = !r26;
  if (r27) goto if_then_3242; else goto if_merge_3243;

  if_then_3242:
  return 0;
  goto if_merge_3243;

  if_merge_3243:
  goto if_merge_3238;
  return 0;
}

_Bool 追加字符(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_字符值) {
  long long r1, r8;
  char* r2;
  char* r3;
  char* r5;
  char* r7;
  void* r0;
  struct 输出缓冲区* r4;
  _Bool r6;

  entry:
  char* cn_var_字符串缓冲_0;
  r0 = 分配内存(2);
  cn_var_字符串缓冲_0 = (char*)0;
  r1 = cn_var_字符值;
  r2 = cn_var_字符串缓冲_0;
    { long long _tmp_r14 = r1; cn_rt_array_set_element(r2, 0, &_tmp_r14, 8); }
  r3 = cn_var_字符串缓冲_0;
    { long long _tmp_i4 = 0; cn_rt_array_set_element(r3, 1, &_tmp_i4, 8); }
  long long cn_var_结果_1;
  r4 = cn_var_缓冲区;
  r5 = cn_var_字符串缓冲_0;
  r6 = 追加字符串(r4, r5);
  cn_var_结果_1 = r6;
  r7 = cn_var_字符串缓冲_0;
  释放内存(r7);
  r8 = cn_var_结果_1;
  return r8;
}

_Bool 追加整数(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_数值) {
  long long r2, r7;
  char* r1;
  char* r4;
  char* r6;
  void* r0;
  struct 输出缓冲区* r3;
  _Bool r5;

  entry:
  char* cn_var_字符串缓冲_0;
  r0 = 分配内存(32);
  cn_var_字符串缓冲_0 = (char*)0;
  r1 = cn_var_字符串缓冲_0;
  r2 = cn_var_数值;
  格式化整数(r1, 32, r2);
  long long cn_var_结果_1;
  r3 = cn_var_缓冲区;
  r4 = cn_var_字符串缓冲_0;
  r5 = 追加字符串(r3, r4);
  cn_var_结果_1 = r5;
  r6 = cn_var_字符串缓冲_0;
  释放内存(r6);
  r7 = cn_var_结果_1;
  return r7;
}

void 追加缩进(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_缩进宽度) {
  long long r1, r2, r4, r5, r6, r7, r10, r11;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r8;
  _Bool r9;

  entry:
  r0 = cn_var_缓冲区;
  r1 = !r0;
  if (r1) goto if_then_3244; else goto if_merge_3245;

  if_then_3244:
  return;
  goto if_merge_3245;

  if_merge_3245:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3246;

  for_cond_3246:
  r2 = cn_var_i_0;
  r3 = cn_var_缓冲区;
  r4 = r3->缩进级别;
  r5 = cn_var_缩进宽度;
  r6 = r4 * r5;
  r7 = r2 < r6;
  if (r7) goto for_body_3247; else goto for_exit_3249;

  for_body_3247:
  r8 = cn_var_缓冲区;
  r9 = 追加字符(r8, 32);
  goto for_update_3248;

  for_update_3248:
  r10 = cn_var_i_0;
  r11 = r10 + 1;
  cn_var_i_0 = r11;
  goto for_cond_3246;

  for_exit_3249:
  return;
}

void 追加换行(struct 输出缓冲区* cn_var_缓冲区) {
  struct 输出缓冲区* r0;
  _Bool r1;

  entry:
  r0 = cn_var_缓冲区;
  r1 = 追加字符串(r0, "\n");
  return;
}

void 增加缩进(struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r3, r4, r5, r7, r8;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r6;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_3252; else goto logic_merge_3253;

  if_then_3250:
  r6 = cn_var_缓冲区;
  r7 = r6->缩进级别;
  r8 = r7 + 1;
  goto if_merge_3251;

  if_merge_3251:

  logic_rhs_3252:
  r2 = cn_var_缓冲区;
  r3 = r2->缩进级别;
  r4 = cn_var_最大缩进级别;
  r5 = r3 < r4;
  goto logic_merge_3253;

  logic_merge_3253:
  if (r5) goto if_then_3250; else goto if_merge_3251;
  return;
}

void 减少缩进(struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r3, r4, r6, r7;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r5;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_3256; else goto logic_merge_3257;

  if_then_3254:
  r5 = cn_var_缓冲区;
  r6 = r5->缩进级别;
  r7 = r6 - 1;
  goto if_merge_3255;

  if_merge_3255:

  logic_rhs_3256:
  r2 = cn_var_缓冲区;
  r3 = r2->缩进级别;
  r4 = r3 > 0;
  goto logic_merge_3257;

  logic_merge_3257:
  if (r4) goto if_then_3254; else goto if_merge_3255;
  return;
}

char* 类型到C类型(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r35, r81, r83, r84, r85, r86, r91, r95, r96;
  char* r20;
  char* r24;
  char* r28;
  char* r40;
  char* r45;
  char* r48;
  char* r53;
  char* r56;
  char* r61;
  char* r64;
  char* r69;
  char* r74;
  char* r78;
  char* r94;
  char* r100;
  void* r33;
  void* r90;
  struct 类型信息* r0;
  struct 类型信息* r2;
  struct 类型信息* r17;
  struct 类型信息* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  struct 类型信息* r25;
  struct 类型信息* r26;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 类型信息* r32;
  void* r34;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r43;
  struct 类型信息* r44;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r51;
  struct 类型信息* r52;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r59;
  struct 类型信息* r60;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r67;
  struct 类型信息* r68;
  struct 输出缓冲区* r71;
  struct 输出缓冲区* r73;
  struct 类型信息* r75;
  struct 类型信息* r76;
  struct 输出缓冲区* r77;
  struct 输出缓冲区* r79;
  struct 类型信息* r82;
  struct 输出缓冲区* r87;
  struct 类型信息* r89;
  void* r92;
  struct 输出缓冲区* r93;
  struct 输出缓冲区* r97;
  struct 输出缓冲区* r99;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r22;
  _Bool r30;
  _Bool r36;
  _Bool r38;
  _Bool r42;
  _Bool r46;
  _Bool r50;
  _Bool r54;
  _Bool r58;
  _Bool r62;
  _Bool r66;
  _Bool r70;
  _Bool r72;
  _Bool r80;
  _Bool r88;
  _Bool r98;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_3258; else goto if_merge_3259;

  if_then_3258:
  return "void";
  goto if_merge_3259;

  if_merge_3259:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_空;
  if (r4) goto case_body_3261; else goto switch_check_3275;

  switch_check_3275:
  r5 = r3 == 类型种类_类型_整数;
  if (r5) goto case_body_3262; else goto switch_check_3276;

  switch_check_3276:
  r6 = r3 == 类型种类_类型_小数;
  if (r6) goto case_body_3263; else goto switch_check_3277;

  switch_check_3277:
  r7 = r3 == 类型种类_类型_布尔;
  if (r7) goto case_body_3264; else goto switch_check_3278;

  switch_check_3278:
  r8 = r3 == 类型种类_类型_字符串;
  if (r8) goto case_body_3265; else goto switch_check_3279;

  switch_check_3279:
  r9 = r3 == 类型种类_类型_字符;
  if (r9) goto case_body_3266; else goto switch_check_3280;

  switch_check_3280:
  r10 = r3 == 类型种类_类型_指针;
  if (r10) goto case_body_3267; else goto switch_check_3281;

  switch_check_3281:
  r11 = r3 == 类型种类_类型_数组;
  if (r11) goto case_body_3268; else goto switch_check_3282;

  switch_check_3282:
  r12 = r3 == 类型种类_类型_结构体;
  if (r12) goto case_body_3269; else goto switch_check_3283;

  switch_check_3283:
  r13 = r3 == 类型种类_类型_枚举;
  if (r13) goto case_body_3270; else goto switch_check_3284;

  switch_check_3284:
  r14 = r3 == 类型种类_类型_类;
  if (r14) goto case_body_3271; else goto switch_check_3285;

  switch_check_3285:
  r15 = r3 == 类型种类_类型_接口;
  if (r15) goto case_body_3272; else goto switch_check_3286;

  switch_check_3286:
  r16 = r3 == 类型种类_类型_函数;
  if (r16) goto case_body_3273; else goto case_default_3274;

  case_body_3261:
  return "void";
  goto switch_merge_3260;

  case_body_3262:
  return "int";
  goto switch_merge_3260;

  case_body_3263:
  return "double";
  goto switch_merge_3260;

  case_body_3264:
  return "_Bool";
  goto switch_merge_3260;

  case_body_3265:
  return "cn_string_t";
  goto switch_merge_3260;

  case_body_3266:
  return "char";
  goto switch_merge_3260;

  case_body_3267:
  r17 = cn_var_类型;
  r18 = r17->指向类型;
  r19 = cn_var_缓冲区;
  r20 = 类型到C类型(r18, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, "*");
  r23 = cn_var_缓冲区;
  r24 = r23->数据;
  return r24;
  goto switch_merge_3260;

  case_body_3268:
  r25 = cn_var_类型;
  r26 = r25->元素类型;
  r27 = cn_var_缓冲区;
  r28 = 类型到C类型(r26, r27);
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, "[");
  r31 = cn_var_缓冲区;
  r32 = cn_var_类型;
  r33 = r32->维度大小;
  r34 = (void**)cn_rt_array_get_element(r33, 0, 8);
  r35 = 0 + r34;
  r36 = 追加整数(r31, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, "]");
  r39 = cn_var_缓冲区;
  r40 = r39->数据;
  return r40;
  goto switch_merge_3260;

  case_body_3269:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "struct ");
  r43 = cn_var_缓冲区;
  r44 = cn_var_类型;
  r45 = r44->名称;
  r46 = 追加字符串(r43, r45);
  r47 = cn_var_缓冲区;
  r48 = r47->数据;
  return r48;
  goto switch_merge_3260;

  case_body_3270:
  r49 = cn_var_缓冲区;
  r50 = 追加字符串(r49, "enum ");
  r51 = cn_var_缓冲区;
  r52 = cn_var_类型;
  r53 = r52->名称;
  r54 = 追加字符串(r51, r53);
  r55 = cn_var_缓冲区;
  r56 = r55->数据;
  return r56;
  goto switch_merge_3260;

  case_body_3271:
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, "struct ");
  r59 = cn_var_缓冲区;
  r60 = cn_var_类型;
  r61 = r60->名称;
  r62 = 追加字符串(r59, r61);
  r63 = cn_var_缓冲区;
  r64 = r63->数据;
  return r64;
  goto switch_merge_3260;

  case_body_3272:
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, "struct ");
  r67 = cn_var_缓冲区;
  r68 = cn_var_类型;
  r69 = r68->名称;
  r70 = 追加字符串(r67, r69);
  r71 = cn_var_缓冲区;
  r72 = 追加字符串(r71, "_vtable");
  r73 = cn_var_缓冲区;
  r74 = r73->数据;
  return r74;
  goto switch_merge_3260;

  case_body_3273:
  r75 = cn_var_类型;
  r76 = r75->返回类型;
  r77 = cn_var_缓冲区;
  r78 = 类型到C类型(r76, r77);
  r79 = cn_var_缓冲区;
  r80 = 追加字符串(r79, " (*)(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3287;

  for_cond_3287:
  r81 = cn_var_i_0;
  r82 = cn_var_类型;
  r83 = r82->参数个数;
  r84 = r81 < r83;
  if (r84) goto for_body_3288; else goto for_exit_3290;

  for_body_3288:
  r85 = cn_var_i_0;
  r86 = r85 > 0;
  if (r86) goto if_then_3291; else goto if_merge_3292;

  for_update_3289:
  r95 = cn_var_i_0;
  r96 = r95 + 1;
  cn_var_i_0 = r96;
  goto for_cond_3287;

  for_exit_3290:
  r97 = cn_var_缓冲区;
  r98 = 追加字符串(r97, ")");
  r99 = cn_var_缓冲区;
  r100 = r99->数据;
  return r100;
  goto switch_merge_3260;

  if_then_3291:
  r87 = cn_var_缓冲区;
  r88 = 追加字符串(r87, ", ");
  goto if_merge_3292;

  if_merge_3292:
  r89 = cn_var_类型;
  r90 = r89->参数类型列表;
  r91 = cn_var_i_0;
  r92 = (void**)cn_rt_array_get_element(r90, r91, 8);
  r93 = cn_var_缓冲区;
  r94 = 类型到C类型(r92, r93);
  goto for_update_3289;

  case_default_3274:
  return "void";
  goto switch_merge_3260;

  switch_merge_3260:
  return NULL;
}

char* 生成类型声明(struct 类型信息* cn_var_类型, char* cn_var_名称, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r22, r24, r25, r31, r33, r37, r38;
  char* r5;
  char* r9;
  char* r12;
  char* r16;
  char* r20;
  char* r40;
  char* r43;
  char* r47;
  char* r50;
  void* r30;
  struct 类型信息* r0;
  struct 类型信息* r2;
  struct 类型信息* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r11;
  struct 类型信息* r13;
  struct 类型信息* r14;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  struct 类型信息* r23;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r28;
  struct 类型信息* r29;
  void* r32;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r39;
  struct 类型信息* r41;
  struct 输出缓冲区* r42;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct 输出缓冲区* r49;
  _Bool r7;
  _Bool r10;
  _Bool r18;
  _Bool r21;
  _Bool r27;
  _Bool r34;
  _Bool r36;
  _Bool r45;
  _Bool r48;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_3293; else goto if_merge_3294;

  if_then_3293:
  return "void";
  goto if_merge_3294;

  if_merge_3294:
  if (类型种类_类型_指针) goto if_then_3295; else goto if_merge_3296;

  if_then_3295:
  r2 = cn_var_类型;
  r3 = r2->指向类型;
  r4 = cn_var_缓冲区;
  r5 = 类型到C类型(r3, r4);
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "* ");
  r8 = cn_var_缓冲区;
  r9 = cn_var_名称;
  r10 = 追加字符串(r8, r9);
  r11 = cn_var_缓冲区;
  r12 = r11->数据;
  return r12;
  goto if_merge_3296;

  if_merge_3296:
  if (类型种类_类型_数组) goto if_then_3297; else goto if_merge_3298;

  if_then_3297:
  r13 = cn_var_类型;
  r14 = r13->元素类型;
  r15 = cn_var_缓冲区;
  r16 = 类型到C类型(r14, r15);
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, " ");
  r19 = cn_var_缓冲区;
  r20 = cn_var_名称;
  r21 = 追加字符串(r19, r20);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3299;

  if_merge_3298:
  r41 = cn_var_类型;
  r42 = cn_var_缓冲区;
  r43 = 类型到C类型(r41, r42);
  r44 = cn_var_缓冲区;
  r45 = 追加字符串(r44, " ");
  r46 = cn_var_缓冲区;
  r47 = cn_var_名称;
  r48 = 追加字符串(r46, r47);
  r49 = cn_var_缓冲区;
  r50 = r49->数据;
  return r50;

  for_cond_3299:
  r22 = cn_var_i_0;
  r23 = cn_var_类型;
  r24 = r23->数组维度;
  r25 = r22 < r24;
  if (r25) goto for_body_3300; else goto for_exit_3302;

  for_body_3300:
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, "[");
  r28 = cn_var_缓冲区;
  r29 = cn_var_类型;
  r30 = r29->维度大小;
  r31 = cn_var_i_0;
  r32 = (void**)cn_rt_array_get_element(r30, r31, 8);
  r33 = 0 + r32;
  r34 = 追加整数(r28, r33);
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, "]");
  goto for_update_3301;

  for_update_3301:
  r37 = cn_var_i_0;
  r38 = r37 + 1;
  cn_var_i_0 = r38;
  goto for_cond_3299;

  for_exit_3302:
  r39 = cn_var_缓冲区;
  r40 = r39->数据;
  return r40;
  goto if_merge_3298;
  return NULL;
}

char* 类型节点到C类型(struct 类型节点* cn_var_类型节点指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r14, r15, r18, r19, r22, r23, r26, r27, r30, r31, r34, r35, r58, r59;
  char* r13;
  char* r17;
  char* r21;
  char* r25;
  char* r29;
  char* r33;
  char* r38;
  char* r41;
  char* r45;
  char* r49;
  char* r53;
  char* r64;
  char* r69;
  char* r72;
  char* r77;
  char* r80;
  char* r85;
  char* r88;
  char* r93;
  char* r96;
  char* r100;
  char* r102;
  char* r105;
  char* r108;
  struct 类型节点* r0;
  struct 类型节点* r2;
  struct 类型节点* r12;
  struct 类型节点* r16;
  struct 类型节点* r20;
  struct 类型节点* r24;
  struct 类型节点* r28;
  struct 类型节点* r32;
  struct 输出缓冲区* r36;
  struct 类型节点* r37;
  struct 输出缓冲区* r40;
  struct 类型节点* r42;
  struct 类型节点* r43;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct 输出缓冲区* r48;
  struct 类型节点* r50;
  struct 类型节点* r51;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r56;
  struct 类型节点* r57;
  struct 输出缓冲区* r61;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r67;
  struct 类型节点* r68;
  struct 输出缓冲区* r71;
  struct 输出缓冲区* r73;
  struct 输出缓冲区* r75;
  struct 类型节点* r76;
  struct 输出缓冲区* r79;
  struct 输出缓冲区* r81;
  struct 输出缓冲区* r83;
  struct 类型节点* r84;
  struct 输出缓冲区* r87;
  struct 输出缓冲区* r89;
  struct 输出缓冲区* r91;
  struct 类型节点* r92;
  struct 输出缓冲区* r95;
  struct 输出缓冲区* r97;
  struct 输出缓冲区* r99;
  struct 类型节点* r101;
  struct 输出缓冲区* r103;
  struct 类型节点* r104;
  struct 输出缓冲区* r107;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r39;
  _Bool r47;
  _Bool r55;
  _Bool r60;
  _Bool r62;
  _Bool r66;
  _Bool r70;
  _Bool r74;
  _Bool r78;
  _Bool r82;
  _Bool r86;
  _Bool r90;
  _Bool r94;
  _Bool r98;
  _Bool r106;
  enum 节点类型 r3;

  entry:
  r0 = cn_var_类型节点指针;
  r1 = !r0;
  if (r1) goto if_then_3303; else goto if_merge_3304;

  if_then_3303:
  return "void";
  goto if_merge_3304;

  if_merge_3304:
  r2 = cn_var_类型节点指针;
  r3 = r2->类型;
  r4 = r3 == 节点类型_基础类型;
  if (r4) goto case_body_3306; else goto switch_check_3315;

  switch_check_3315:
  r5 = r3 == 节点类型_指针类型;
  if (r5) goto case_body_3307; else goto switch_check_3316;

  switch_check_3316:
  r6 = r3 == 节点类型_数组类型;
  if (r6) goto case_body_3308; else goto switch_check_3317;

  switch_check_3317:
  r7 = r3 == 节点类型_结构体类型;
  if (r7) goto case_body_3309; else goto switch_check_3318;

  switch_check_3318:
  r8 = r3 == 节点类型_枚举类型;
  if (r8) goto case_body_3310; else goto switch_check_3319;

  switch_check_3319:
  r9 = r3 == 节点类型_类类型;
  if (r9) goto case_body_3311; else goto switch_check_3320;

  switch_check_3320:
  r10 = r3 == 节点类型_接口类型;
  if (r10) goto case_body_3312; else goto switch_check_3321;

  switch_check_3321:
  r11 = r3 == 节点类型_函数类型;
  if (r11) goto case_body_3313; else goto case_default_3314;

  case_body_3306:
  r12 = cn_var_类型节点指针;
  r13 = r12->名称;
  r14 = 比较字符串(r13, "整数");
  r15 = r14 == 0;
  if (r15) goto if_then_3322; else goto if_merge_3323;

  if_then_3322:
  return "long long";
  goto if_merge_3323;

  if_merge_3323:
  r16 = cn_var_类型节点指针;
  r17 = r16->名称;
  r18 = 比较字符串(r17, "小数");
  r19 = r18 == 0;
  if (r19) goto if_then_3324; else goto if_merge_3325;

  if_then_3324:
  return "double";
  goto if_merge_3325;

  if_merge_3325:
  r20 = cn_var_类型节点指针;
  r21 = r20->名称;
  r22 = 比较字符串(r21, "布尔");
  r23 = r22 == 0;
  if (r23) goto if_then_3326; else goto if_merge_3327;

  if_then_3326:
  return "_Bool";
  goto if_merge_3327;

  if_merge_3327:
  r24 = cn_var_类型节点指针;
  r25 = r24->名称;
  r26 = 比较字符串(r25, "字符串");
  r27 = r26 == 0;
  if (r27) goto if_then_3328; else goto if_merge_3329;

  if_then_3328:
  return "cn_string_t";
  goto if_merge_3329;

  if_merge_3329:
  r28 = cn_var_类型节点指针;
  r29 = r28->名称;
  r30 = 比较字符串(r29, "字符");
  r31 = r30 == 0;
  if (r31) goto if_then_3330; else goto if_merge_3331;

  if_then_3330:
  return "char";
  goto if_merge_3331;

  if_merge_3331:
  r32 = cn_var_类型节点指针;
  r33 = r32->名称;
  r34 = 比较字符串(r33, "空类型");
  r35 = r34 == 0;
  if (r35) goto if_then_3332; else goto if_merge_3333;

  if_then_3332:
  return "void";
  goto if_merge_3333;

  if_merge_3333:
  r36 = cn_var_缓冲区;
  r37 = cn_var_类型节点指针;
  r38 = r37->名称;
  r39 = 追加字符串(r36, r38);
  r40 = cn_var_缓冲区;
  r41 = r40->数据;
  return r41;
  goto switch_merge_3305;

  case_body_3307:
  r42 = cn_var_类型节点指针;
  r43 = r42->元素类型;
  r44 = cn_var_缓冲区;
  r45 = 类型节点到C类型(r43, r44);
  r46 = cn_var_缓冲区;
  r47 = 追加字符串(r46, "*");
  r48 = cn_var_缓冲区;
  r49 = r48->数据;
  return r49;
  goto switch_merge_3305;

  case_body_3308:
  r50 = cn_var_类型节点指针;
  r51 = r50->元素类型;
  r52 = cn_var_缓冲区;
  r53 = 类型节点到C类型(r51, r52);
  r54 = cn_var_缓冲区;
  r55 = 追加字符串(r54, "[");
  r56 = cn_var_缓冲区;
  r57 = cn_var_类型节点指针;
  r58 = r57->数组大小;
  r59 = 0 + r58;
  r60 = 追加整数(r56, r59);
  r61 = cn_var_缓冲区;
  r62 = 追加字符串(r61, "]");
  r63 = cn_var_缓冲区;
  r64 = r63->数据;
  return r64;
  goto switch_merge_3305;

  case_body_3309:
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, "struct ");
  r67 = cn_var_缓冲区;
  r68 = cn_var_类型节点指针;
  r69 = r68->名称;
  r70 = 追加字符串(r67, r69);
  r71 = cn_var_缓冲区;
  r72 = r71->数据;
  return r72;
  goto switch_merge_3305;

  case_body_3310:
  r73 = cn_var_缓冲区;
  r74 = 追加字符串(r73, "enum ");
  r75 = cn_var_缓冲区;
  r76 = cn_var_类型节点指针;
  r77 = r76->名称;
  r78 = 追加字符串(r75, r77);
  r79 = cn_var_缓冲区;
  r80 = r79->数据;
  return r80;
  goto switch_merge_3305;

  case_body_3311:
  r81 = cn_var_缓冲区;
  r82 = 追加字符串(r81, "struct ");
  r83 = cn_var_缓冲区;
  r84 = cn_var_类型节点指针;
  r85 = r84->名称;
  r86 = 追加字符串(r83, r85);
  r87 = cn_var_缓冲区;
  r88 = r87->数据;
  return r88;
  goto switch_merge_3305;

  case_body_3312:
  r89 = cn_var_缓冲区;
  r90 = 追加字符串(r89, "struct ");
  r91 = cn_var_缓冲区;
  r92 = cn_var_类型节点指针;
  r93 = r92->名称;
  r94 = 追加字符串(r91, r93);
  r95 = cn_var_缓冲区;
  r96 = r95->数据;
  return r96;
  goto switch_merge_3305;

  case_body_3313:
  r97 = cn_var_缓冲区;
  r98 = 追加字符串(r97, "void*");
  r99 = cn_var_缓冲区;
  r100 = r99->数据;
  return r100;
  goto switch_merge_3305;

  case_default_3314:
  r101 = cn_var_类型节点指针;
  r102 = r101->名称;
  if (r102) goto if_then_3334; else goto if_else_3335;

  if_then_3334:
  r103 = cn_var_缓冲区;
  r104 = cn_var_类型节点指针;
  r105 = r104->名称;
  r106 = 追加字符串(r103, r105);
  goto if_merge_3336;

  if_else_3335:
  return "void";
  goto if_merge_3336;

  if_merge_3336:
  r107 = cn_var_缓冲区;
  r108 = r107->数据;
  return r108;
  goto switch_merge_3305;

  switch_merge_3305:
  return NULL;
}

char* 生成操作数代码(struct IR操作数 cn_var_操作数, struct 输出缓冲区* cn_var_缓冲区) {
  long long r12, r18, r31, r46;
  char* r15;
  char* r21;
  char* r26;
  char* r34;
  char* r37;
  char* r40;
  char* r49;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r43;
  struct 基本块* r45;
  struct 输出缓冲区* r48;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r9;
  _Bool r13;
  _Bool r19;
  _Bool r28;
  _Bool r32;
  _Bool r38;
  _Bool r42;
  _Bool r47;
  double r24;
  struct IR操作数 r0;
  struct IR操作数 r11;
  struct IR操作数 r17;
  struct IR操作数 r23;
  struct IR操作数 r30;
  struct IR操作数 r36;
  struct IR操作数 r44;
  enum IR操作数种类 r1;

  entry:
  r0 = cn_var_操作数;
  r1 = r0.种类;
  r2 = r1 == IR操作数种类_虚拟寄存器;
  if (r2) goto case_body_3338; else goto switch_check_3345;

  switch_check_3345:
  r3 = r1 == IR操作数种类_整数常量;
  if (r3) goto case_body_3339; else goto switch_check_3346;

  switch_check_3346:
  r4 = r1 == IR操作数种类_小数常量;
  if (r4) goto case_body_3340; else goto switch_check_3347;

  switch_check_3347:
  r5 = r1 == IR操作数种类_字符串常量;
  if (r5) goto case_body_3341; else goto switch_check_3348;

  switch_check_3348:
  r6 = r1 == IR操作数种类_全局符号;
  if (r6) goto case_body_3342; else goto switch_check_3349;

  switch_check_3349:
  r7 = r1 == IR操作数种类_基本块标签;
  if (r7) goto case_body_3343; else goto case_default_3344;

  case_body_3338:
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, "r");
  r10 = cn_var_缓冲区;
  r11 = cn_var_操作数;
  r12 = r11.寄存器编号;
  r13 = 追加整数(r10, r12);
  r14 = cn_var_缓冲区;
  r15 = r14->数据;
  return r15;
  goto switch_merge_3337;

  case_body_3339:
  r16 = cn_var_缓冲区;
  r17 = cn_var_操作数;
  r18 = r17.整数值;
  r19 = 追加整数(r16, r18);
  r20 = cn_var_缓冲区;
  r21 = r20->数据;
  return r21;
  goto switch_merge_3337;

  case_body_3340:
  r22 = cn_var_缓冲区;
  r23 = cn_var_操作数;
  r24 = r23.小数值;
  追加小数(r22, r24);
  r25 = cn_var_缓冲区;
  r26 = r25->数据;
  return r26;
  goto switch_merge_3337;

  case_body_3341:
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, "_str_");
  r29 = cn_var_缓冲区;
  r30 = cn_var_操作数;
  r31 = r30.寄存器编号;
  r32 = 追加整数(r29, r31);
  r33 = cn_var_缓冲区;
  r34 = r33->数据;
  return r34;
  goto switch_merge_3337;

  case_body_3342:
  r35 = cn_var_缓冲区;
  r36 = cn_var_操作数;
  r37 = r36.符号名;
  r38 = 追加字符串(r35, r37);
  r39 = cn_var_缓冲区;
  r40 = r39->数据;
  return r40;
  goto switch_merge_3337;

  case_body_3343:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "L");
  r43 = cn_var_缓冲区;
  r44 = cn_var_操作数;
  r45 = r44.目标块;
  r46 = r45->块编号;
  r47 = 追加整数(r43, r46);
  r48 = cn_var_缓冲区;
  r49 = r48->数据;
  return r49;
  goto switch_merge_3337;

  case_default_3344:
  return "";
  goto switch_merge_3337;

  switch_merge_3337:
  return NULL;
}

void 生成算术指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r4;
  char* r10;
  char* r48;
  struct 输出缓冲区* r0;
  struct IR指令* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct IR指令* r7;
  struct 输出缓冲区* r9;
  struct IR指令* r11;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r43;
  struct IR指令* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r51;
  _Bool r6;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r21;
  _Bool r22;
  _Bool r24;
  _Bool r26;
  _Bool r28;
  _Bool r30;
  _Bool r32;
  _Bool r34;
  _Bool r36;
  _Bool r38;
  _Bool r40;
  _Bool r42;
  _Bool r44;
  _Bool r50;
  struct IR操作数 r2;
  struct IR操作数 r8;
  struct IR操作数 r46;
  enum IR操作码 r12;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令;
  r2 = r1->目标;
  r3 = cn_var_缓冲区;
  r4 = 生成操作数代码(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " = ");
  r7 = cn_var_指令;
  r8 = r7->源操作数1;
  r9 = cn_var_缓冲区;
  r10 = 生成操作数代码(r8, r9);
  r11 = cn_var_指令;
  r12 = r11->操作码;
  r13 = r12 == IR操作码_加法指令;
  if (r13) goto case_body_3351; else goto switch_check_3362;

  switch_check_3362:
  r14 = r12 == IR操作码_减法指令;
  if (r14) goto case_body_3352; else goto switch_check_3363;

  switch_check_3363:
  r15 = r12 == IR操作码_乘法指令;
  if (r15) goto case_body_3353; else goto switch_check_3364;

  switch_check_3364:
  r16 = r12 == IR操作码_除法指令;
  if (r16) goto case_body_3354; else goto switch_check_3365;

  switch_check_3365:
  r17 = r12 == IR操作码_取模指令;
  if (r17) goto case_body_3355; else goto switch_check_3366;

  switch_check_3366:
  r18 = r12 == IR操作码_与指令;
  if (r18) goto case_body_3356; else goto switch_check_3367;

  switch_check_3367:
  r19 = r12 == IR操作码_或指令;
  if (r19) goto case_body_3357; else goto switch_check_3368;

  switch_check_3368:
  r20 = r12 == IR操作码_异或指令;
  if (r20) goto case_body_3358; else goto switch_check_3369;

  switch_check_3369:
  r21 = r12 == IR操作码_左移指令;
  if (r21) goto case_body_3359; else goto switch_check_3370;

  switch_check_3370:
  r22 = r12 == IR操作码_右移指令;
  if (r22) goto case_body_3360; else goto case_default_3361;

  case_body_3351:
  r23 = cn_var_缓冲区;
  r24 = 追加字符串(r23, " + ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3352:
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " - ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3353:
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, " * ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3354:
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, " / ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3355:
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, " % ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3356:
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, " & ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3357:
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, " | ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3358:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, " ^ ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3359:
  r39 = cn_var_缓冲区;
  r40 = 追加字符串(r39, " << ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_body_3360:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, " >> ");
  goto switch_merge_3350;
  goto switch_merge_3350;

  case_default_3361:
  r43 = cn_var_缓冲区;
  r44 = 追加字符串(r43, " ? ");
  goto switch_merge_3350;

  switch_merge_3350:
  r45 = cn_var_指令;
  r46 = r45->源操作数2;
  r47 = cn_var_缓冲区;
  r48 = 生成操作数代码(r46, r47);
  r49 = cn_var_缓冲区;
  r50 = 追加字符串(r49, ";");
  r51 = cn_var_缓冲区;
  追加换行(r51);
  return;
}

void 生成一元指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r4;
  char* r23;
  struct 输出缓冲区* r0;
  struct IR指令* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct IR指令* r7;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r18;
  struct IR指令* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  _Bool r6;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  _Bool r25;
  struct IR操作数 r2;
  struct IR操作数 r21;
  enum IR操作码 r8;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令;
  r2 = r1->目标;
  r3 = cn_var_缓冲区;
  r4 = 生成操作数代码(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " = ");
  r7 = cn_var_指令;
  r8 = r7->操作码;
  r9 = r8 == IR操作码_负号指令;
  if (r9) goto case_body_3372; else goto switch_check_3376;

  switch_check_3376:
  r10 = r8 == IR操作码_取反指令;
  if (r10) goto case_body_3373; else goto switch_check_3377;

  switch_check_3377:
  r11 = r8 == IR操作码_逻辑非指令;
  if (r11) goto case_body_3374; else goto case_default_3375;

  case_body_3372:
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, "-");
  goto switch_merge_3371;
  goto switch_merge_3371;

  case_body_3373:
  r14 = cn_var_缓冲区;
  r15 = 追加字符串(r14, "~");
  goto switch_merge_3371;
  goto switch_merge_3371;

  case_body_3374:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, "!");
  goto switch_merge_3371;
  goto switch_merge_3371;

  case_default_3375:
  r18 = cn_var_缓冲区;
  r19 = 追加字符串(r18, "");
  goto switch_merge_3371;

  switch_merge_3371:
  r20 = cn_var_指令;
  r21 = r20->源操作数1;
  r22 = cn_var_缓冲区;
  r23 = 生成操作数代码(r21, r22);
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, ";");
  r26 = cn_var_缓冲区;
  追加换行(r26);
  return;
}

void 生成比较指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r4;
  char* r10;
  char* r36;
  struct 输出缓冲区* r0;
  struct IR指令* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct IR指令* r7;
  struct 输出缓冲区* r9;
  struct IR指令* r11;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct IR指令* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  _Bool r6;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r18;
  _Bool r20;
  _Bool r22;
  _Bool r24;
  _Bool r26;
  _Bool r28;
  _Bool r30;
  _Bool r32;
  _Bool r38;
  struct IR操作数 r2;
  struct IR操作数 r8;
  struct IR操作数 r34;
  enum IR操作码 r12;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令;
  r2 = r1->目标;
  r3 = cn_var_缓冲区;
  r4 = 生成操作数代码(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " = ");
  r7 = cn_var_指令;
  r8 = r7->源操作数1;
  r9 = cn_var_缓冲区;
  r10 = 生成操作数代码(r8, r9);
  r11 = cn_var_指令;
  r12 = r11->操作码;
  r13 = r12 == IR操作码_相等指令;
  if (r13) goto case_body_3379; else goto switch_check_3386;

  switch_check_3386:
  r14 = r12 == IR操作码_不等指令;
  if (r14) goto case_body_3380; else goto switch_check_3387;

  switch_check_3387:
  r15 = r12 == IR操作码_小于指令;
  if (r15) goto case_body_3381; else goto switch_check_3388;

  switch_check_3388:
  r16 = r12 == IR操作码_小于等于指令;
  if (r16) goto case_body_3382; else goto switch_check_3389;

  switch_check_3389:
  r17 = r12 == IR操作码_大于指令;
  if (r17) goto case_body_3383; else goto switch_check_3390;

  switch_check_3390:
  r18 = r12 == IR操作码_大于等于指令;
  if (r18) goto case_body_3384; else goto case_default_3385;

  case_body_3379:
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, " == ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_body_3380:
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, " != ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_body_3381:
  r23 = cn_var_缓冲区;
  r24 = 追加字符串(r23, " < ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_body_3382:
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " <= ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_body_3383:
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, " > ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_body_3384:
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, " >= ");
  goto switch_merge_3378;
  goto switch_merge_3378;

  case_default_3385:
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, " ? ");
  goto switch_merge_3378;

  switch_merge_3378:
  r33 = cn_var_指令;
  r34 = r33->源操作数2;
  r35 = cn_var_缓冲区;
  r36 = 生成操作数代码(r34, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ";");
  r39 = cn_var_缓冲区;
  追加换行(r39);
  return;
}

void 生成内存指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r19, r20;
  char* r12;
  char* r29;
  char* r35;
  char* r45;
  char* r51;
  char* r59;
  char* r65;
  char* r73;
  char* r79;
  char* r87;
  char* r93;
  struct IR指令* r0;
  struct 输出缓冲区* r8;
  struct IR指令* r9;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct IR指令* r16;
  struct 类型节点* r18;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r25;
  struct IR指令* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  struct IR指令* r32;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r40;
  struct IR指令* r42;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct IR指令* r48;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r55;
  struct IR指令* r56;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct IR指令* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r66;
  struct 输出缓冲区* r68;
  struct 输出缓冲区* r69;
  struct IR指令* r70;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r74;
  struct IR指令* r76;
  struct 输出缓冲区* r78;
  struct 输出缓冲区* r80;
  struct 输出缓冲区* r82;
  struct 输出缓冲区* r83;
  struct IR指令* r84;
  struct 输出缓冲区* r86;
  struct 输出缓冲区* r88;
  struct IR指令* r90;
  struct 输出缓冲区* r92;
  struct 输出缓冲区* r94;
  struct 输出缓冲区* r96;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r14;
  _Bool r21;
  _Bool r23;
  _Bool r31;
  _Bool r37;
  _Bool r41;
  _Bool r47;
  _Bool r53;
  _Bool r61;
  _Bool r67;
  _Bool r75;
  _Bool r81;
  _Bool r89;
  _Bool r95;
  struct IR操作数 r10;
  struct IR操作数 r17;
  struct IR操作数 r27;
  struct IR操作数 r33;
  struct IR操作数 r43;
  struct IR操作数 r49;
  struct IR操作数 r57;
  struct IR操作数 r63;
  struct IR操作数 r71;
  struct IR操作数 r77;
  struct IR操作数 r85;
  struct IR操作数 r91;
  enum IR操作码 r1;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = r1 == IR操作码_分配指令;
  if (r2) goto case_body_3392; else goto switch_check_3399;

  switch_check_3399:
  r3 = r1 == IR操作码_加载指令;
  if (r3) goto case_body_3393; else goto switch_check_3400;

  switch_check_3400:
  r4 = r1 == IR操作码_存储指令;
  if (r4) goto case_body_3394; else goto switch_check_3401;

  switch_check_3401:
  r5 = r1 == IR操作码_移动指令;
  if (r5) goto case_body_3395; else goto switch_check_3402;

  switch_check_3402:
  r6 = r1 == IR操作码_取地址指令;
  if (r6) goto case_body_3396; else goto switch_check_3403;

  switch_check_3403:
  r7 = r1 == IR操作码_解引用指令;
  if (r7) goto case_body_3397; else goto case_default_3398;

  case_body_3392:
  r8 = cn_var_缓冲区;
  追加缩进(r8, 4);
  r9 = cn_var_指令;
  r10 = r9->目标;
  r11 = cn_var_缓冲区;
  r12 = 生成操作数代码(r10, r11);
  r13 = cn_var_缓冲区;
  r14 = 追加字符串(r13, " = alloca(");
  r15 = cn_var_缓冲区;
  r16 = cn_var_指令;
  r17 = r16->目标;
  r18 = r17.类型信息;
  r19 = r18->大小;
  r20 = 0 + r19;
  r21 = 追加整数(r15, r20);
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, ");");
  r24 = cn_var_缓冲区;
  追加换行(r24);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_body_3393:
  r25 = cn_var_缓冲区;
  追加缩进(r25, 4);
  r26 = cn_var_指令;
  r27 = r26->目标;
  r28 = cn_var_缓冲区;
  r29 = 生成操作数代码(r27, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, " = *");
  r32 = cn_var_指令;
  r33 = r32->源操作数1;
  r34 = cn_var_缓冲区;
  r35 = 生成操作数代码(r33, r34);
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, ";");
  r38 = cn_var_缓冲区;
  追加换行(r38);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_body_3394:
  r39 = cn_var_缓冲区;
  追加缩进(r39, 4);
  r40 = cn_var_缓冲区;
  r41 = 追加字符串(r40, "*");
  r42 = cn_var_指令;
  r43 = r42->源操作数2;
  r44 = cn_var_缓冲区;
  r45 = 生成操作数代码(r43, r44);
  r46 = cn_var_缓冲区;
  r47 = 追加字符串(r46, " = ");
  r48 = cn_var_指令;
  r49 = r48->源操作数1;
  r50 = cn_var_缓冲区;
  r51 = 生成操作数代码(r49, r50);
  r52 = cn_var_缓冲区;
  r53 = 追加字符串(r52, ";");
  r54 = cn_var_缓冲区;
  追加换行(r54);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_body_3395:
  r55 = cn_var_缓冲区;
  追加缩进(r55, 4);
  r56 = cn_var_指令;
  r57 = r56->目标;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, " = ");
  r62 = cn_var_指令;
  r63 = r62->源操作数1;
  r64 = cn_var_缓冲区;
  r65 = 生成操作数代码(r63, r64);
  r66 = cn_var_缓冲区;
  r67 = 追加字符串(r66, ";");
  r68 = cn_var_缓冲区;
  追加换行(r68);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_body_3396:
  r69 = cn_var_缓冲区;
  追加缩进(r69, 4);
  r70 = cn_var_指令;
  r71 = r70->目标;
  r72 = cn_var_缓冲区;
  r73 = 生成操作数代码(r71, r72);
  r74 = cn_var_缓冲区;
  r75 = 追加字符串(r74, " = &");
  r76 = cn_var_指令;
  r77 = r76->源操作数1;
  r78 = cn_var_缓冲区;
  r79 = 生成操作数代码(r77, r78);
  r80 = cn_var_缓冲区;
  r81 = 追加字符串(r80, ";");
  r82 = cn_var_缓冲区;
  追加换行(r82);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_body_3397:
  r83 = cn_var_缓冲区;
  追加缩进(r83, 4);
  r84 = cn_var_指令;
  r85 = r84->目标;
  r86 = cn_var_缓冲区;
  r87 = 生成操作数代码(r85, r86);
  r88 = cn_var_缓冲区;
  r89 = 追加字符串(r88, " = *");
  r90 = cn_var_指令;
  r91 = r90->源操作数1;
  r92 = cn_var_缓冲区;
  r93 = 生成操作数代码(r91, r92);
  r94 = cn_var_缓冲区;
  r95 = 追加字符串(r94, ";");
  r96 = cn_var_缓冲区;
  追加换行(r96);
  goto switch_merge_3391;
  goto switch_merge_3391;

  case_default_3398:
  goto switch_merge_3391;
  goto switch_merge_3391;

  switch_merge_3391:
  return;
}

void 生成控制流指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r13, r25, r43, r51, r60, r73, r75, r76, r77, r78, r83, r87, r88, r98;
  char* r36;
  char* r64;
  char* r70;
  char* r86;
  char* r104;
  struct IR指令* r0;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct IR指令* r10;
  struct 基本块* r12;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct IR指令* r22;
  struct 基本块* r24;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r31;
  struct IR指令* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct IR指令* r40;
  struct 基本块* r42;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct IR指令* r48;
  struct 基本块* r50;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r56;
  struct IR指令* r57;
  struct IR指令* r61;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct IR指令* r67;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  struct IR指令* r74;
  struct 输出缓冲区* r79;
  struct IR指令* r81;
  struct IR操作数* r82;
  void* r84;
  struct 输出缓冲区* r85;
  struct 输出缓冲区* r89;
  struct 输出缓冲区* r91;
  struct 输出缓冲区* r92;
  struct 输出缓冲区* r93;
  struct IR指令* r95;
  struct 输出缓冲区* r99;
  struct IR指令* r101;
  struct 输出缓冲区* r103;
  struct 输出缓冲区* r105;
  struct 输出缓冲区* r107;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r8;
  _Bool r14;
  _Bool r16;
  _Bool r20;
  _Bool r26;
  _Bool r28;
  _Bool r32;
  _Bool r38;
  _Bool r44;
  _Bool r46;
  _Bool r52;
  _Bool r54;
  _Bool r66;
  _Bool r72;
  _Bool r80;
  _Bool r90;
  _Bool r94;
  _Bool r100;
  _Bool r106;
  struct IR操作数 r11;
  struct IR操作数 r23;
  struct IR操作数 r34;
  struct IR操作数 r41;
  struct IR操作数 r49;
  struct IR操作数 r58;
  struct IR操作数 r62;
  struct IR操作数 r68;
  struct IR操作数 r96;
  struct IR操作数 r102;
  enum IR操作码 r1;
  enum IR操作数种类 r59;
  enum IR操作数种类 r97;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = r1 == IR操作码_标签指令;
  if (r2) goto case_body_3405; else goto switch_check_3411;

  switch_check_3411:
  r3 = r1 == IR操作码_跳转指令;
  if (r3) goto case_body_3406; else goto switch_check_3412;

  switch_check_3412:
  r4 = r1 == IR操作码_条件跳转指令;
  if (r4) goto case_body_3407; else goto switch_check_3413;

  switch_check_3413:
  r5 = r1 == IR操作码_调用指令;
  if (r5) goto case_body_3408; else goto switch_check_3414;

  switch_check_3414:
  r6 = r1 == IR操作码_返回指令;
  if (r6) goto case_body_3409; else goto case_default_3410;

  case_body_3405:
  r7 = cn_var_缓冲区;
  r8 = 追加字符串(r7, "L");
  r9 = cn_var_缓冲区;
  r10 = cn_var_指令;
  r11 = r10->目标;
  r12 = r11.目标块;
  r13 = r12->块编号;
  r14 = 追加整数(r9, r13);
  r15 = cn_var_缓冲区;
  r16 = 追加字符串(r15, ":");
  r17 = cn_var_缓冲区;
  追加换行(r17);
  goto switch_merge_3404;
  goto switch_merge_3404;

  case_body_3406:
  r18 = cn_var_缓冲区;
  追加缩进(r18, 4);
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, "goto L");
  r21 = cn_var_缓冲区;
  r22 = cn_var_指令;
  r23 = r22->目标;
  r24 = r23.目标块;
  r25 = r24->块编号;
  r26 = 追加整数(r21, r25);
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, ";");
  r29 = cn_var_缓冲区;
  追加换行(r29);
  goto switch_merge_3404;
  goto switch_merge_3404;

  case_body_3407:
  r30 = cn_var_缓冲区;
  追加缩进(r30, 4);
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, "if (");
  r33 = cn_var_指令;
  r34 = r33->目标;
  r35 = cn_var_缓冲区;
  r36 = 生成操作数代码(r34, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ") goto L");
  r39 = cn_var_缓冲区;
  r40 = cn_var_指令;
  r41 = r40->源操作数1;
  r42 = r41.目标块;
  r43 = r42->块编号;
  r44 = 追加整数(r39, r43);
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, "; else goto L");
  r47 = cn_var_缓冲区;
  r48 = cn_var_指令;
  r49 = r48->源操作数2;
  r50 = r49.目标块;
  r51 = r50->块编号;
  r52 = 追加整数(r47, r51);
  r53 = cn_var_缓冲区;
  r54 = 追加字符串(r53, ";");
  r55 = cn_var_缓冲区;
  追加换行(r55);
  goto switch_merge_3404;
  goto switch_merge_3404;

  case_body_3408:
  r56 = cn_var_缓冲区;
  追加缩进(r56, 4);
  r57 = cn_var_指令;
  r58 = r57->目标;
  r59 = r58.种类;
  r60 = r59 != IR操作数种类_无操作数;
  if (r60) goto if_then_3415; else goto if_merge_3416;

  if_then_3415:
  r61 = cn_var_指令;
  r62 = r61->目标;
  r63 = cn_var_缓冲区;
  r64 = 生成操作数代码(r62, r63);
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, " = ");
  goto if_merge_3416;

  if_merge_3416:
  r67 = cn_var_指令;
  r68 = r67->源操作数1;
  r69 = cn_var_缓冲区;
  r70 = 生成操作数代码(r68, r69);
  r71 = cn_var_缓冲区;
  r72 = 追加字符串(r71, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3417;

  for_cond_3417:
  r73 = cn_var_i_0;
  r74 = cn_var_指令;
  r75 = r74->额外参数数量;
  r76 = r73 < r75;
  if (r76) goto for_body_3418; else goto for_exit_3420;

  for_body_3418:
  r77 = cn_var_i_0;
  r78 = r77 > 0;
  if (r78) goto if_then_3421; else goto if_merge_3422;

  for_update_3419:
  r87 = cn_var_i_0;
  r88 = r87 + 1;
  cn_var_i_0 = r88;
  goto for_cond_3417;

  for_exit_3420:
  r89 = cn_var_缓冲区;
  r90 = 追加字符串(r89, ");");
  r91 = cn_var_缓冲区;
  追加换行(r91);
  goto switch_merge_3404;
  goto switch_merge_3404;

  if_then_3421:
  r79 = cn_var_缓冲区;
  r80 = 追加字符串(r79, ", ");
  goto if_merge_3422;

  if_merge_3422:
  r81 = cn_var_指令;
  r82 = r81->额外参数;
  r83 = cn_var_i_0;
  r84 = (void**)cn_rt_array_get_element(r82, r83, 8);
  r85 = cn_var_缓冲区;
  r86 = 生成操作数代码(r84, r85);
  goto for_update_3419;

  case_body_3409:
  r92 = cn_var_缓冲区;
  追加缩进(r92, 4);
  r93 = cn_var_缓冲区;
  r94 = 追加字符串(r93, "return");
  r95 = cn_var_指令;
  r96 = r95->目标;
  r97 = r96.种类;
  r98 = r97 != IR操作数种类_无操作数;
  if (r98) goto if_then_3423; else goto if_merge_3424;

  if_then_3423:
  r99 = cn_var_缓冲区;
  r100 = 追加字符串(r99, " ");
  r101 = cn_var_指令;
  r102 = r101->目标;
  r103 = cn_var_缓冲区;
  r104 = 生成操作数代码(r102, r103);
  goto if_merge_3424;

  if_merge_3424:
  r105 = cn_var_缓冲区;
  r106 = 追加字符串(r105, ";");
  r107 = cn_var_缓冲区;
  追加换行(r107);
  goto switch_merge_3404;
  goto switch_merge_3404;

  case_default_3410:
  goto switch_merge_3404;
  goto switch_merge_3404;

  switch_merge_3404:
  return;
}

void 生成结构体指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r9;
  char* r15;
  char* r21;
  char* r30;
  char* r36;
  char* r42;
  char* r53;
  char* r59;
  struct IR指令* r0;
  struct 输出缓冲区* r5;
  struct IR指令* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  struct IR指令* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r18;
  struct IR指令* r19;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r26;
  struct IR指令* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct IR指令* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct IR指令* r40;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r48;
  struct IR指令* r50;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r54;
  struct IR指令* r56;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r62;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r11;
  _Bool r17;
  _Bool r22;
  _Bool r24;
  _Bool r32;
  _Bool r38;
  _Bool r43;
  _Bool r45;
  _Bool r49;
  _Bool r55;
  _Bool r61;
  struct IR操作数 r7;
  struct IR操作数 r13;
  struct IR操作数 r20;
  struct IR操作数 r28;
  struct IR操作数 r34;
  struct IR操作数 r41;
  struct IR操作数 r51;
  struct IR操作数 r57;
  enum IR操作码 r1;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = r1 == IR操作码_成员指针指令;
  if (r2) goto case_body_3426; else goto switch_check_3430;

  switch_check_3430:
  r3 = r1 == IR操作码_成员访问指令;
  if (r3) goto case_body_3427; else goto switch_check_3431;

  switch_check_3431:
  r4 = r1 == IR操作码_结构体初始化指令;
  if (r4) goto case_body_3428; else goto case_default_3429;

  case_body_3426:
  r5 = cn_var_缓冲区;
  追加缩进(r5, 4);
  r6 = cn_var_指令;
  r7 = r6->目标;
  r8 = cn_var_缓冲区;
  r9 = 生成操作数代码(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, " = &");
  r12 = cn_var_指令;
  r13 = r12->源操作数1;
  r14 = cn_var_缓冲区;
  r15 = 生成操作数代码(r13, r14);
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, "->");
  r18 = cn_var_缓冲区;
  r19 = cn_var_指令;
  r20 = r19->源操作数2;
  r21 = r20.符号名;
  r22 = 追加字符串(r18, r21);
  r23 = cn_var_缓冲区;
  r24 = 追加字符串(r23, ";");
  r25 = cn_var_缓冲区;
  追加换行(r25);
  goto switch_merge_3425;
  goto switch_merge_3425;

  case_body_3427:
  r26 = cn_var_缓冲区;
  追加缩进(r26, 4);
  r27 = cn_var_指令;
  r28 = r27->目标;
  r29 = cn_var_缓冲区;
  r30 = 生成操作数代码(r28, r29);
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, " = ");
  r33 = cn_var_指令;
  r34 = r33->源操作数1;
  r35 = cn_var_缓冲区;
  r36 = 生成操作数代码(r34, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ".");
  r39 = cn_var_缓冲区;
  r40 = cn_var_指令;
  r41 = r40->源操作数2;
  r42 = r41.符号名;
  r43 = 追加字符串(r39, r42);
  r44 = cn_var_缓冲区;
  r45 = 追加字符串(r44, ";");
  r46 = cn_var_缓冲区;
  追加换行(r46);
  goto switch_merge_3425;
  goto switch_merge_3425;

  case_body_3428:
  r47 = cn_var_缓冲区;
  追加缩进(r47, 4);
  r48 = cn_var_缓冲区;
  r49 = 追加字符串(r48, "memset(&");
  r50 = cn_var_指令;
  r51 = r50->目标;
  r52 = cn_var_缓冲区;
  r53 = 生成操作数代码(r51, r52);
  r54 = cn_var_缓冲区;
  r55 = 追加字符串(r54, ", 0, sizeof(");
  r56 = cn_var_指令;
  r57 = r56->目标;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, "));");
  r62 = cn_var_缓冲区;
  追加换行(r62);
  goto switch_merge_3425;
  goto switch_merge_3425;

  case_default_3429:
  goto switch_merge_3425;
  goto switch_merge_3425;

  switch_merge_3425:
  return;
}

void 生成指令代码(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1;
  char* r55;
  char* r61;
  char* r67;
  char* r73;
  struct IR指令* r0;
  struct IR指令* r2;
  struct IR指令* r39;
  struct 输出缓冲区* r40;
  struct IR指令* r41;
  struct 输出缓冲区* r42;
  struct IR指令* r43;
  struct 输出缓冲区* r44;
  struct IR指令* r45;
  struct 输出缓冲区* r46;
  struct IR指令* r47;
  struct 输出缓冲区* r48;
  struct IR指令* r49;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r51;
  struct IR指令* r52;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r56;
  struct IR指令* r58;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r62;
  struct IR指令* r64;
  struct 输出缓冲区* r66;
  struct 输出缓冲区* r68;
  struct IR指令* r70;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r74;
  struct 输出缓冲区* r76;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r21;
  _Bool r22;
  _Bool r23;
  _Bool r24;
  _Bool r25;
  _Bool r26;
  _Bool r27;
  _Bool r28;
  _Bool r29;
  _Bool r30;
  _Bool r31;
  _Bool r32;
  _Bool r33;
  _Bool r34;
  _Bool r35;
  _Bool r36;
  _Bool r37;
  _Bool r38;
  _Bool r57;
  _Bool r63;
  _Bool r69;
  _Bool r75;
  struct IR操作数 r53;
  struct IR操作数 r59;
  struct IR操作数 r65;
  struct IR操作数 r71;
  enum IR操作码 r3;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_3432; else goto if_merge_3433;

  if_then_3432:
  return;
  goto if_merge_3433;

  if_merge_3433:
  r2 = cn_var_指令;
  r3 = r2->操作码;
  r4 = r3 == IR操作码_加法指令;
  if (r4) goto case_body_3435; else goto switch_check_3471;

  switch_check_3471:
  r5 = r3 == IR操作码_减法指令;
  if (r5) goto case_body_3436; else goto switch_check_3472;

  switch_check_3472:
  r6 = r3 == IR操作码_乘法指令;
  if (r6) goto case_body_3437; else goto switch_check_3473;

  switch_check_3473:
  r7 = r3 == IR操作码_除法指令;
  if (r7) goto case_body_3438; else goto switch_check_3474;

  switch_check_3474:
  r8 = r3 == IR操作码_取模指令;
  if (r8) goto case_body_3439; else goto switch_check_3475;

  switch_check_3475:
  r9 = r3 == IR操作码_与指令;
  if (r9) goto case_body_3440; else goto switch_check_3476;

  switch_check_3476:
  r10 = r3 == IR操作码_或指令;
  if (r10) goto case_body_3441; else goto switch_check_3477;

  switch_check_3477:
  r11 = r3 == IR操作码_异或指令;
  if (r11) goto case_body_3442; else goto switch_check_3478;

  switch_check_3478:
  r12 = r3 == IR操作码_左移指令;
  if (r12) goto case_body_3443; else goto switch_check_3479;

  switch_check_3479:
  r13 = r3 == IR操作码_右移指令;
  if (r13) goto case_body_3444; else goto switch_check_3480;

  switch_check_3480:
  r14 = r3 == IR操作码_负号指令;
  if (r14) goto case_body_3445; else goto switch_check_3481;

  switch_check_3481:
  r15 = r3 == IR操作码_取反指令;
  if (r15) goto case_body_3446; else goto switch_check_3482;

  switch_check_3482:
  r16 = r3 == IR操作码_逻辑非指令;
  if (r16) goto case_body_3447; else goto switch_check_3483;

  switch_check_3483:
  r17 = r3 == IR操作码_相等指令;
  if (r17) goto case_body_3448; else goto switch_check_3484;

  switch_check_3484:
  r18 = r3 == IR操作码_不等指令;
  if (r18) goto case_body_3449; else goto switch_check_3485;

  switch_check_3485:
  r19 = r3 == IR操作码_小于指令;
  if (r19) goto case_body_3450; else goto switch_check_3486;

  switch_check_3486:
  r20 = r3 == IR操作码_小于等于指令;
  if (r20) goto case_body_3451; else goto switch_check_3487;

  switch_check_3487:
  r21 = r3 == IR操作码_大于指令;
  if (r21) goto case_body_3452; else goto switch_check_3488;

  switch_check_3488:
  r22 = r3 == IR操作码_大于等于指令;
  if (r22) goto case_body_3453; else goto switch_check_3489;

  switch_check_3489:
  r23 = r3 == IR操作码_分配指令;
  if (r23) goto case_body_3454; else goto switch_check_3490;

  switch_check_3490:
  r24 = r3 == IR操作码_加载指令;
  if (r24) goto case_body_3455; else goto switch_check_3491;

  switch_check_3491:
  r25 = r3 == IR操作码_存储指令;
  if (r25) goto case_body_3456; else goto switch_check_3492;

  switch_check_3492:
  r26 = r3 == IR操作码_移动指令;
  if (r26) goto case_body_3457; else goto switch_check_3493;

  switch_check_3493:
  r27 = r3 == IR操作码_取地址指令;
  if (r27) goto case_body_3458; else goto switch_check_3494;

  switch_check_3494:
  r28 = r3 == IR操作码_解引用指令;
  if (r28) goto case_body_3459; else goto switch_check_3495;

  switch_check_3495:
  r29 = r3 == IR操作码_标签指令;
  if (r29) goto case_body_3460; else goto switch_check_3496;

  switch_check_3496:
  r30 = r3 == IR操作码_跳转指令;
  if (r30) goto case_body_3461; else goto switch_check_3497;

  switch_check_3497:
  r31 = r3 == IR操作码_条件跳转指令;
  if (r31) goto case_body_3462; else goto switch_check_3498;

  switch_check_3498:
  r32 = r3 == IR操作码_调用指令;
  if (r32) goto case_body_3463; else goto switch_check_3499;

  switch_check_3499:
  r33 = r3 == IR操作码_返回指令;
  if (r33) goto case_body_3464; else goto switch_check_3500;

  switch_check_3500:
  r34 = r3 == IR操作码_成员指针指令;
  if (r34) goto case_body_3465; else goto switch_check_3501;

  switch_check_3501:
  r35 = r3 == IR操作码_成员访问指令;
  if (r35) goto case_body_3466; else goto switch_check_3502;

  switch_check_3502:
  r36 = r3 == IR操作码_结构体初始化指令;
  if (r36) goto case_body_3467; else goto switch_check_3503;

  switch_check_3503:
  r37 = r3 == IR操作码_PHI指令;
  if (r37) goto case_body_3468; else goto switch_check_3504;

  switch_check_3504:
  r38 = r3 == IR操作码_选择指令;
  if (r38) goto case_body_3469; else goto case_default_3470;

  case_body_3435:
  goto switch_merge_3434;

  case_body_3436:
  goto switch_merge_3434;

  case_body_3437:
  goto switch_merge_3434;

  case_body_3438:
  goto switch_merge_3434;

  case_body_3439:
  goto switch_merge_3434;

  case_body_3440:
  goto switch_merge_3434;

  case_body_3441:
  goto switch_merge_3434;

  case_body_3442:
  goto switch_merge_3434;

  case_body_3443:
  goto switch_merge_3434;

  case_body_3444:
  r39 = cn_var_指令;
  r40 = cn_var_缓冲区;
  生成算术指令(r39, r40);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3445:
  goto switch_merge_3434;

  case_body_3446:
  goto switch_merge_3434;

  case_body_3447:
  r41 = cn_var_指令;
  r42 = cn_var_缓冲区;
  生成一元指令(r41, r42);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3448:
  goto switch_merge_3434;

  case_body_3449:
  goto switch_merge_3434;

  case_body_3450:
  goto switch_merge_3434;

  case_body_3451:
  goto switch_merge_3434;

  case_body_3452:
  goto switch_merge_3434;

  case_body_3453:
  r43 = cn_var_指令;
  r44 = cn_var_缓冲区;
  生成比较指令(r43, r44);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3454:
  goto switch_merge_3434;

  case_body_3455:
  goto switch_merge_3434;

  case_body_3456:
  goto switch_merge_3434;

  case_body_3457:
  goto switch_merge_3434;

  case_body_3458:
  goto switch_merge_3434;

  case_body_3459:
  r45 = cn_var_指令;
  r46 = cn_var_缓冲区;
  生成内存指令(r45, r46);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3460:
  goto switch_merge_3434;

  case_body_3461:
  goto switch_merge_3434;

  case_body_3462:
  goto switch_merge_3434;

  case_body_3463:
  goto switch_merge_3434;

  case_body_3464:
  r47 = cn_var_指令;
  r48 = cn_var_缓冲区;
  生成控制流指令(r47, r48);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3465:
  goto switch_merge_3434;

  case_body_3466:
  goto switch_merge_3434;

  case_body_3467:
  r49 = cn_var_指令;
  r50 = cn_var_缓冲区;
  生成结构体指令(r49, r50);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3468:
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_body_3469:
  r51 = cn_var_缓冲区;
  追加缩进(r51, 4);
  r52 = cn_var_指令;
  r53 = r52->目标;
  r54 = cn_var_缓冲区;
  r55 = 生成操作数代码(r53, r54);
  r56 = cn_var_缓冲区;
  r57 = 追加字符串(r56, " = ");
  r58 = cn_var_指令;
  r59 = r58->目标;
  r60 = cn_var_缓冲区;
  r61 = 生成操作数代码(r59, r60);
  r62 = cn_var_缓冲区;
  r63 = 追加字符串(r62, " ? ");
  r64 = cn_var_指令;
  r65 = r64->源操作数1;
  r66 = cn_var_缓冲区;
  r67 = 生成操作数代码(r65, r66);
  r68 = cn_var_缓冲区;
  r69 = 追加字符串(r68, " : ");
  r70 = cn_var_指令;
  r71 = r70->源操作数2;
  r72 = cn_var_缓冲区;
  r73 = 生成操作数代码(r71, r72);
  r74 = cn_var_缓冲区;
  r75 = 追加字符串(r74, ";");
  r76 = cn_var_缓冲区;
  追加换行(r76);
  goto switch_merge_3434;
  goto switch_merge_3434;

  case_default_3470:
  goto switch_merge_3434;
  goto switch_merge_3434;

  switch_merge_3434:
  return;
}

void 生成基本块代码(struct 基本块* cn_var_块, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r6;
  struct 基本块* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r4;
  struct 基本块* r5;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  struct 基本块* r11;
  struct IR指令* r12;
  struct IR指令* r13;
  struct IR指令* r14;
  struct 输出缓冲区* r15;
  struct IR指令* r16;
  struct IR指令* r17;
  _Bool r3;
  _Bool r7;
  _Bool r9;

  entry:
  r0 = cn_var_块;
  r1 = !r0;
  if (r1) goto if_then_3505; else goto if_merge_3506;

  if_then_3505:
  return;
  goto if_merge_3506;

  if_merge_3506:
  r2 = cn_var_缓冲区;
  r3 = 追加字符串(r2, "L");
  r4 = cn_var_缓冲区;
  r5 = cn_var_块;
  r6 = r5->块编号;
  r7 = 追加整数(r4, r6);
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, ":");
  r10 = cn_var_缓冲区;
  追加换行(r10);
  struct IR指令* cn_var_指令_0;
  r11 = cn_var_块;
  r12 = r11->首条指令;
  cn_var_指令_0 = r12;
  goto while_cond_3507;

  while_cond_3507:
  r13 = cn_var_指令_0;
  if (r13) goto while_body_3508; else goto while_exit_3509;

  while_body_3508:
  r14 = cn_var_指令_0;
  r15 = cn_var_缓冲区;
  生成指令代码(r14, r15);
  r16 = cn_var_指令_0;
  r17 = r16->下一条;
  cn_var_指令_0 = r17;
  goto while_cond_3507;

  while_exit_3509:
  return;
}

void 生成局部变量声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r2, r4, r5, r8, r19, r24, r25;
  char* r14;
  struct IR函数* r0;
  struct IR函数* r3;
  struct IR函数* r6;
  struct IR操作数* r7;
  void* r9;
  struct 输出缓冲区* r10;
  struct 类型节点* r12;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  _Bool r16;
  _Bool r20;
  _Bool r22;
  struct IR操作数 r11;
  struct IR操作数 r18;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_3510; else goto if_merge_3511;

  if_then_3510:
  return;
  goto if_merge_3511;

  if_merge_3511:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3512;

  for_cond_3512:
  r2 = cn_var_i_0;
  r3 = cn_var_函数指针;
  r4 = r3->局部变量数量;
  r5 = r2 < r4;
  if (r5) goto for_body_3513; else goto for_exit_3515;

  for_body_3513:
  struct IR操作数 cn_var_局部变量_1;
  r6 = cn_var_函数指针;
  r7 = r6->局部变量;
  r8 = cn_var_i_0;
  r9 = (void**)cn_rt_array_get_element(r7, r8, 8);
  cn_var_局部变量_1 = r9;
  r10 = cn_var_缓冲区;
  追加缩进(r10, 4);
  r11 = cn_var_局部变量_1;
  r12 = r11.类型信息;
  r13 = cn_var_缓冲区;
  r14 = 类型节点到C类型(r12, r13);
  r15 = cn_var_缓冲区;
  r16 = 追加字符串(r15, " r");
  r17 = cn_var_缓冲区;
  r18 = cn_var_局部变量_1;
  r19 = r18.寄存器编号;
  r20 = 追加整数(r17, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, ";");
  r23 = cn_var_缓冲区;
  追加换行(r23);
  goto for_update_3514;

  for_update_3514:
  r24 = cn_var_i_0;
  r25 = r24 + 1;
  cn_var_i_0 = r25;
  goto for_cond_3512;

  for_exit_3515:
  return;
}

void 生成函数代码(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r14, r16, r17, r18, r19, r24, r27, r33, r35, r36;
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
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct IR函数* r42;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r44;
  struct IR函数* r45;
  struct 基本块* r46;
  struct 基本块* r47;
  struct 基本块* r48;
  struct 输出缓冲区* r49;
  struct 基本块* r50;
  struct 基本块* r51;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r55;
  _Bool r7;
  _Bool r11;
  _Bool r13;
  _Bool r21;
  _Bool r31;
  _Bool r34;
  _Bool r38;
  _Bool r40;
  _Bool r53;
  struct 参数 r26;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_3516; else goto if_merge_3517;

  if_then_3516:
  return;
  goto if_merge_3517;

  if_merge_3517:
  r2 = cn_var_函数指针;
  r3 = r2->返回类型;
  r4 = cn_var_缓冲区;
  r5 = 类型节点到C类型(r3, r4);
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
  goto for_cond_3518;

  for_cond_3518:
  r14 = cn_var_i_0;
  r15 = cn_var_函数指针;
  r16 = r15->参数数量;
  r17 = r14 < r16;
  if (r17) goto for_body_3519; else goto for_exit_3521;

  for_body_3519:
  r18 = cn_var_i_0;
  r19 = r18 > 0;
  if (r19) goto if_then_3522; else goto if_merge_3523;

  for_update_3520:
  r35 = cn_var_i_0;
  r36 = r35 + 1;
  cn_var_i_0 = r36;
  goto for_cond_3518;

  for_exit_3521:
  if (0) goto if_then_3524; else goto if_merge_3525;

  if_then_3522:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, ", ");
  goto if_merge_3523;

  if_merge_3523:
  struct IR操作数 cn_var_参数_1;
  r22 = cn_var_函数指针;
  r23 = r22->参数列表;
  r24 = cn_var_i_0;
  r25 = (void**)cn_rt_array_get_element(r23, r24, 8);
  cn_var_参数_1 = r25;
  r26 = cn_var_参数_1;
  r27 = r26.类型信息;
  r28 = cn_var_缓冲区;
  r29 = 类型节点到C类型(r27, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, " p");
  r32 = cn_var_缓冲区;
  r33 = cn_var_i_0;
  r34 = 追加整数(r32, r33);
  goto for_update_3520;

  if_then_3524:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, "void");
  goto if_merge_3525;

  if_merge_3525:
  r39 = cn_var_缓冲区;
  r40 = 追加字符串(r39, ") {");
  r41 = cn_var_缓冲区;
  追加换行(r41);
  r42 = cn_var_函数指针;
  r43 = cn_var_缓冲区;
  生成局部变量声明(r42, r43);
  r44 = cn_var_缓冲区;
  追加换行(r44);
  struct 基本块* cn_var_块_2;
  r45 = cn_var_函数指针;
  r46 = r45->首个块;
  cn_var_块_2 = r46;
  goto while_cond_3526;

  while_cond_3526:
  r47 = cn_var_块_2;
  if (r47) goto while_body_3527; else goto while_exit_3528;

  while_body_3527:
  r48 = cn_var_块_2;
  r49 = cn_var_缓冲区;
  生成基本块代码(r48, r49);
  r50 = cn_var_块_2;
  r51 = r50->下一个;
  cn_var_块_2 = r51;
  goto while_cond_3526;

  while_exit_3528:
  r52 = cn_var_缓冲区;
  r53 = 追加字符串(r52, "}");
  r54 = cn_var_缓冲区;
  追加换行(r54);
  r55 = cn_var_缓冲区;
  追加换行(r55);
  return;
}

struct C代码生成上下文* 创建C代码生成上下文(struct IR模块* cn_var_模块, struct 代码生成选项 cn_var_选项) {
  long long r0, r4, r6, r8;
  void* r1;
  struct C代码生成上下文* r2;
  struct IR模块* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct C代码生成上下文* r11;
  struct 代码生成选项 r10;

  entry:
  struct C代码生成上下文* cn_var_上下文_0;
  r0 = sizeof(struct C代码生成上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_3529; else goto if_merge_3530;

  if_then_3529:
  r3 = cn_var_模块;
  r4 = cn_var_默认缓冲区大小;
  r5 = 创建输出缓冲区(r4);
  r6 = cn_var_默认缓冲区大小;
  r7 = 创建输出缓冲区(r6);
  r8 = cn_var_默认缓冲区大小;
  r9 = 创建输出缓冲区(r8);
  r10 = cn_var_选项;
  goto if_merge_3530;

  if_merge_3530:
  r11 = cn_var_上下文_0;
  return r11;
}

void 释放C代码生成上下文(struct C代码生成上下文* cn_var_上下文) {
  struct C代码生成上下文* r0;
  struct C代码生成上下文* r1;
  struct 输出缓冲区* r2;
  struct C代码生成上下文* r3;
  struct 输出缓冲区* r4;
  struct C代码生成上下文* r5;
  struct 输出缓冲区* r6;
  struct C代码生成上下文* r7;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3531; else goto if_merge_3532;

  if_then_3531:
  r1 = cn_var_上下文;
  r2 = r1->头文件缓冲区;
  释放输出缓冲区(r2);
  r3 = cn_var_上下文;
  r4 = r3->源文件缓冲区;
  释放输出缓冲区(r4);
  r5 = cn_var_上下文;
  r6 = r5->函数体缓冲区;
  释放输出缓冲区(r6);
  r7 = cn_var_上下文;
  释放内存(r7);
  goto if_merge_3532;

  if_merge_3532:
  return;
}

struct 代码生成选项 获取默认生成选项() {
  long long r0;
  void* r1;
  struct 代码生成选项* r2;
  void* r3;

  entry:
  struct 代码生成选项* cn_var_选项_0;
  r0 = sizeof(struct 代码生成选项);
  r1 = 分配清零内存(1, r0);
  cn_var_选项_0 = (struct 代码生成选项*)0;
  r2 = cn_var_选项_0;
  r3 = (void**)cn_rt_array_get_element(r2, 0, 8);
  return r3;
}

void 格式化整数(char* cn_var_缓冲区, long long cn_var_缓冲区大小, long long cn_var_数值) {
  long long r3, r4, r5, r6, r7, r8, r9, r10, r11, r13, r14, r15, r16, r17, r18, r20, r21, r22, r23, r24, r25, r26, r28, r31, r32, r33, r35;
  char* r0;
  char* r1;
  char* r12;
  char* r19;
  char* r27;
  char* r30;
  char* r34;
  char* r36;
  void* r2;
  void* r29;

  entry:
  cn_var_数值 = 0;
  if (0) goto if_then_3533; else goto if_merge_3534;

  if_then_3533:
  r0 = cn_var_缓冲区;
    { long long _tmp_i5 = 48; cn_rt_array_set_element(r0, 0, &_tmp_i5, 8); }
  r1 = cn_var_缓冲区;
    { long long _tmp_i6 = 0; cn_rt_array_set_element(r1, 1, &_tmp_i6, 8); }
  return;
  goto if_merge_3534;

  if_merge_3534:
  char* cn_var_临时_0;
  r2 = 分配内存(32);
  cn_var_临时_0 = (char*)0;
  long long cn_var_索引_1;
  cn_var_索引_1 = 0;
  long long cn_var_负数_2;
  cn_var_负数_2 = 0;
  r3 = cn_var_数值;
  r4 = r3 < 0;
  if (r4) goto if_then_3535; else goto if_merge_3536;

  if_then_3535:
  cn_var_负数_2 = 1;
  r5 = cn_var_数值;
  r6 = -r5;
  cn_var_数值 = r6;
  goto if_merge_3536;

  if_merge_3536:
  goto while_cond_3537;

  while_cond_3537:
  r7 = cn_var_数值;
  r8 = r7 > 0;
  if (r8) goto while_body_3538; else goto while_exit_3539;

  while_body_3538:
  r9 = cn_var_数值;
  r10 = r9 % 10;
  r11 = 48 + r10;
  r12 = cn_var_临时_0;
  r13 = cn_var_索引_1;
    { long long _tmp_r15 = r11; cn_rt_array_set_element(r12, r13, &_tmp_r15, 8); }
  r14 = cn_var_索引_1;
  r15 = r14 + 1;
  cn_var_索引_1 = r15;
  r16 = cn_var_数值;
  r17 = r16 / 10;
  cn_var_数值 = r17;
  goto while_cond_3537;

  while_exit_3539:
  long long cn_var_输出索引_3;
  cn_var_输出索引_3 = 0;
  r18 = cn_var_负数_2;
  if (r18) goto if_then_3540; else goto if_merge_3541;

  if_then_3540:
  r19 = cn_var_缓冲区;
  r20 = cn_var_输出索引_3;
    { long long _tmp_i7 = 45; cn_rt_array_set_element(r19, r20, &_tmp_i7, 8); }
  r21 = cn_var_输出索引_3;
  r22 = r21 + 1;
  cn_var_输出索引_3 = r22;
  goto if_merge_3541;

  if_merge_3541:
  goto while_cond_3542;

  while_cond_3542:
  r23 = cn_var_索引_1;
  r24 = r23 > 0;
  if (r24) goto while_body_3543; else goto while_exit_3544;

  while_body_3543:
  r25 = cn_var_索引_1;
  r26 = r25 - 1;
  cn_var_索引_1 = r26;
  r27 = cn_var_临时_0;
  r28 = cn_var_索引_1;
  r29 = (void**)cn_rt_array_get_element(r27, r28, 8);
  r30 = cn_var_缓冲区;
  r31 = cn_var_输出索引_3;
    { long long _tmp_r16 = r29; cn_rt_array_set_element(r30, r31, &_tmp_r16, 8); }
  r32 = cn_var_输出索引_3;
  r33 = r32 + 1;
  cn_var_输出索引_3 = r33;
  goto while_cond_3542;

  while_exit_3544:
  r34 = cn_var_缓冲区;
  r35 = cn_var_输出索引_3;
    { long long _tmp_i8 = 0; cn_rt_array_set_element(r34, r35, &_tmp_i8, 8); }
  r36 = cn_var_临时_0;
  释放内存(r36);
  return;
}

void 追加小数(struct 输出缓冲区* cn_var_缓冲区, double cn_var_数值) {
  long long r2, r3, r5, r7, r12;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r11;
  _Bool r8;
  _Bool r10;
  _Bool r13;
  double r0;
  double r1;
  double r4;

  entry:
  long long cn_var_整数部分_0;
  r0 = cn_var_数值;
  cn_var_整数部分_0 = (long long)cn_var_数值;
  double cn_var_小数临时_1;
  r1 = cn_var_数值;
  r2 = cn_var_整数部分_0;
  r3 = r1 - (double)cn_var_整数部分;
  cn_var_小数临时_1 = r3;
  long long cn_var_小数部分_2;
  r4 = cn_var_小数临时_1;
  r5 = r4 * 1000000;
  cn_var_小数部分_2 = (long long)(cn_var_小数临时 * 1000000);
  r6 = cn_var_缓冲区;
  r7 = cn_var_整数部分_0;
  r8 = 追加整数(r6, r7);
  r9 = cn_var_缓冲区;
  r10 = 追加字符串(r9, ".");
  r11 = cn_var_缓冲区;
  r12 = cn_var_小数部分_2;
  r13 = 追加整数(r11, r12);
  return;
}

