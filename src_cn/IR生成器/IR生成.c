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
enum 诊断严重级别 {
    诊断严重级别_诊断_警告 = 1,
    诊断严重级别_诊断_错误 = 0
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
struct 作用域;
struct 符号表管理器;
struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};
struct 符号;
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
struct 诊断信息;
struct 诊断信息 {
    enum 诊断严重级别 严重级别;
    enum 诊断错误码 错误码;
    struct 源位置 位置;
    char* 消息;
};
struct 诊断集合;
struct 诊断集合 {
    struct 诊断信息* 诊断数组;
    long long 容量;
    long long 长度;
    long long 错误计数;
    long long 警告计数;
    long long 最大错误数;
};
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};
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
    struct IR操作数** 参数列表;
    long long 参数数量;
    struct IR操作数** 局部变量;
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
    struct IR操作数** 额外参数;
    long long 额外参数数量;
    struct IR指令* 下一条;
    struct IR指令* 上一条;
    struct 源位置 指令位置;
};

// Forward Declarations - 从导入模块
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);
char* 获取作用域类型名称(enum 作用域类型);
char* 获取符号类型名称(enum 符号类型);
struct 符号* 查找类成员(struct 符号*, char*);
struct 符号* 在作用域查找符号(struct 作用域*, char*);
struct 符号* 查找全局符号(struct 符号表管理器*, char*);
struct 符号* 查找符号(struct 符号表管理器*, char*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, char*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在循环体内(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
void 离开作用域(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, char*, struct 符号*);
void 销毁符号表管理器(struct 符号表管理器*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁作用域(struct 作用域*);
struct 作用域* 创建作用域(enum 作用域类型, char*, struct 作用域*);
struct 符号* 创建接口符号(char*, struct 源位置);
struct 符号* 创建类符号(char*, struct 源位置, _Bool);
struct 符号* 创建枚举成员符号(char*, long long, _Bool, struct 源位置);
struct 符号* 创建枚举符号(char*, struct 源位置);
struct 符号* 创建结构体符号(char*, struct 源位置);
struct 符号* 创建参数符号(char*, struct 类型节点*, struct 源位置);
struct 符号* 创建函数符号(char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建变量符号(char*, struct 类型节点*, struct 源位置, struct 符号标志);
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 字符串格式(char*);
long long 字符串格式化(char*, long long, char*);
char* 读取行(void);
long long 获取绝对值(long long);
long long 求最大值(long long, long long);
long long 求最小值(long long, long long);
long long 获取参数个数(void);
char* 获取参数(long long);
char* 获取程序名称(void);
char* 查找选项(char*);
long long 选项存在(char*);
char* 获取位置参数(long long);
long long 获取位置参数个数(void);
void* 数组获取(void*, long long);
struct 诊断集合* 创建诊断集合(long long);
void 释放诊断集合(struct 诊断集合*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
_Bool 有错误(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
_Bool 应该继续(struct 诊断集合*);
char* 获取严重级别字符串(enum 诊断严重级别);
void 打印诊断信息(struct 诊断信息*);
void 打印所有诊断(struct 诊断集合*);
void 清空诊断集合(struct 诊断集合*);
struct 源位置 创建源位置(char*, long long, long long);
struct 源位置 创建未知位置(void);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, char*, char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, char*);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, char*);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 程序添加声明(struct 程序节点*, struct 声明节点*);
struct 程序节点* 创建程序节点(void);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
extern long long cn_var_指针大小;
extern long long cn_var_字符串大小;
extern long long cn_var_布尔大小;
extern long long cn_var_小数大小;
extern long long cn_var_整数大小;
char* 获取类型种类名称(enum 类型种类);
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
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
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

// CN Language Global Struct Forward Declarations
struct 局部变量映射;
struct 静态变量信息;
struct IR生成上下文;

// CN Language Global Struct Definitions
struct 局部变量映射 {
    char* 原始名称;
    char* 唯一名称;
    struct 局部变量映射* 下一个;
};

struct 静态变量信息 {
    char* 名称;
    long long 名称长度;
    struct 静态变量信息* 下一个;
};

struct IR生成上下文 {
    struct IR模块* 模块;
    struct IR函数* 当前函数;
    struct 基本块* 当前块;
    struct 基本块* 循环退出块;
    struct 基本块* 循环继续块;
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    struct 静态变量信息* 静态变量列表;
    struct 局部变量映射* 局部变量映射表;
};

// Global Variables
long long cn_var_局部变量计数器 = 0;
long long cn_var_基本块计数器 = 0;

// Forward Declarations
void 重置局部变量计数器();
char* 生成唯一变量名(char*);
char* 生成块名称(char*);
long long 分配虚拟寄存器(struct IR生成上下文*);
void 发射指令(struct IR生成上下文*, struct IR指令*);
void 切换基本块(struct IR生成上下文*, struct 基本块*);
char* 查找局部变量唯一名(struct IR生成上下文*, char*);
void 添加局部变量映射(struct IR生成上下文*, char*, char*);
void 释放局部变量映射表(struct IR生成上下文*);
struct IR生成上下文* 创建IR生成上下文();
void 释放IR生成上下文(struct IR生成上下文*);
enum IR操作码 二元运算符转IR(enum 二元运算符);
struct IR操作数 生成表达式IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成标识符IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成二元运算IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成一元运算IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成函数调用IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成成员访问IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成数组访问IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成赋值IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成取地址IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成解引用IR(struct IR生成上下文*, struct 表达式节点*);
void 生成语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成变量声明IR(struct IR生成上下文*, struct 语句节点*);
void 生成如果语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成当循环IR(struct IR生成上下文*, struct 语句节点*);
void 生成循环IR(struct IR生成上下文*, struct 语句节点*);
void 生成返回语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成中断语句IR(struct IR生成上下文*);
void 生成继续语句IR(struct IR生成上下文*);
void 生成块语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成函数IR(struct IR生成上下文*, struct 声明节点*);
void 生成程序IR(struct IR生成上下文*, struct 程序节点*);
void 生成全局变量IR(struct IR生成上下文*, struct 声明节点*);

void 重置局部变量计数器() {

  entry:
  cn_var_局部变量计数器 = 0;
  return;
}

char* 生成唯一变量名(char* cn_var_原始名称) {
  long long r2, r3, r4;
  char* r1;
  char* r6;
  void* r0;
  void* r5;

  entry:
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 256);
  r0 = cn_var_缓冲区;
  r1 = cn_var_原始名称;
  r2 = cn_var_局部变量计数器;
  格式化字符串(r0, 256, "%s_%d", r1, r2);
  r3 = cn_var_局部变量计数器;
  r4 = r3 + 1;
  cn_var_局部变量计数器 = r4;
  r5 = cn_var_缓冲区;
  r6 = 复制字符串(r5);
  return r6;
}

char* 生成块名称(char* cn_var_提示) {
  long long r2, r3, r4;
  char* r1;
  char* r6;
  void* r0;
  void* r5;

  entry:
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 64);
  r0 = cn_var_缓冲区;
  r1 = cn_var_提示;
  r2 = cn_var_基本块计数器;
  格式化字符串(r0, 64, "%s_%d", r1, r2);
  r3 = cn_var_基本块计数器;
  r4 = r3 + 1;
  cn_var_基本块计数器 = r4;
  r5 = cn_var_缓冲区;
  r6 = 复制字符串(r5);
  return r6;
}

long long 分配虚拟寄存器(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r5, r6, r9, r10, r11, r12;
  struct IR生成上下文* r1;
  struct IR生成上下文* r3;
  struct IR函数* r4;
  struct IR生成上下文* r7;
  struct IR函数* r8;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2860; else goto logic_rhs_2859;

  if_then_2857:
  r6 = -1;
  return r6;
  goto if_merge_2858;

  if_merge_2858:
  long long cn_var_编号_0;
  r7 = cn_var_上下文;
  r8 = r7->当前函数;
  r9 = r8->下个寄存器编号;
  cn_var_编号_0 = r9;
  r10 = cn_var_编号_0;
  r11 = r10 + 1;
  r12 = cn_var_编号_0;
  return r12;

  logic_rhs_2859:
  r3 = cn_var_上下文;
  r4 = r3->当前函数;
  r5 = !r4;
  goto logic_merge_2860;

  logic_merge_2860:
  if (r5) goto if_then_2857; else goto if_merge_2858;
  return 0;
}

void 发射指令(struct IR生成上下文* cn_var_上下文, struct IR指令* cn_var_指令) {
  long long r0, r1, r3, r6, r8;
  struct IR生成上下文* r2;
  struct IR生成上下文* r4;
  struct 基本块* r5;
  struct IR指令* r7;
  struct IR生成上下文* r9;
  struct 基本块* r10;
  struct IR指令* r11;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_2866; else goto logic_rhs_2865;

  if_then_2861:
  return;
  goto if_merge_2862;

  if_merge_2862:
  r9 = cn_var_上下文;
  r10 = r9->当前块;
  r11 = cn_var_指令;
  添加指令(r10, r11);

  logic_rhs_2863:
  r7 = cn_var_指令;
  r8 = !r7;
  goto logic_merge_2864;

  logic_merge_2864:
  if (r8) goto if_then_2861; else goto if_merge_2862;

  logic_rhs_2865:
  r4 = cn_var_上下文;
  r5 = r4->当前块;
  r6 = !r5;
  goto logic_merge_2866;

  logic_merge_2866:
  if (r6) goto logic_merge_2864; else goto logic_rhs_2863;
  return;
}

void 切换基本块(struct IR生成上下文* cn_var_上下文, struct 基本块* cn_var_块) {
  struct IR生成上下文* r0;
  struct 基本块* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_2867; else goto if_merge_2868;

  if_then_2867:
  r1 = cn_var_块;
  goto if_merge_2868;

  if_merge_2868:
  return;
}

char* 查找局部变量唯一名(struct IR生成上下文* cn_var_上下文, char* cn_var_原始名称) {
  long long r0, r2, r4, r11, r12;
  char* r3;
  char* r9;
  char* r10;
  char* r14;
  char* r15;
  struct IR生成上下文* r1;
  struct IR生成上下文* r5;
  struct 局部变量映射* r6;
  struct 局部变量映射* r7;
  struct 局部变量映射* r8;
  struct 局部变量映射* r13;
  struct 局部变量映射* r16;
  struct 局部变量映射* r17;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2872; else goto logic_rhs_2871;

  if_then_2869:
  return 0;
  goto if_merge_2870;

  if_merge_2870:
  struct 局部变量映射* cn_var_映射_0;
  r5 = cn_var_上下文;
  r6 = r5->局部变量映射表;
  cn_var_映射_0 = r6;
  goto while_cond_2873;

  logic_rhs_2871:
  r3 = cn_var_原始名称;
  r4 = !r3;
  goto logic_merge_2872;

  logic_merge_2872:
  if (r4) goto if_then_2869; else goto if_merge_2870;

  while_cond_2873:
  r7 = cn_var_映射_0;
  if (r7) goto while_body_2874; else goto while_exit_2875;

  while_body_2874:
  r8 = cn_var_映射_0;
  r9 = r8->原始名称;
  r10 = cn_var_原始名称;
  r11 = 比较字符串(r9, r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2876; else goto if_merge_2877;

  while_exit_2875:
  return 0;

  if_then_2876:
  r13 = cn_var_映射_0;
  r14 = r13->唯一名称;
  r15 = 复制字符串(r14);
  return r15;
  goto if_merge_2877;

  if_merge_2877:
  r16 = cn_var_映射_0;
  r17 = r16->下一个;
  cn_var_映射_0 = r17;
  goto while_cond_2873;
  return NULL;
}

void 添加局部变量映射(struct IR生成上下文* cn_var_上下文, char* cn_var_原始名称, char* cn_var_唯一名称) {
  long long r0, r1, r3, r5, r7, r8;
  char* r4;
  char* r6;
  char* r11;
  char* r12;
  char* r13;
  char* r14;
  struct IR生成上下文* r2;
  void* r9;
  struct 局部变量映射* r10;
  struct IR生成上下文* r15;
  struct 局部变量映射* r16;
  struct 局部变量映射* r17;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_2883; else goto logic_rhs_2882;

  if_then_2878:
  return;
  goto if_merge_2879;

  if_merge_2879:
  struct 局部变量映射* cn_var_映射_0;
  r8 = sizeof(struct 局部变量映射);
  r9 = 分配内存(r8);
  cn_var_映射_0 = r9;
  r10 = cn_var_映射_0;
  if (r10) goto if_then_2884; else goto if_merge_2885;

  logic_rhs_2880:
  r6 = cn_var_唯一名称;
  r7 = !r6;
  goto logic_merge_2881;

  logic_merge_2881:
  if (r7) goto if_then_2878; else goto if_merge_2879;

  logic_rhs_2882:
  r4 = cn_var_原始名称;
  r5 = !r4;
  goto logic_merge_2883;

  logic_merge_2883:
  if (r5) goto logic_merge_2881; else goto logic_rhs_2880;

  if_then_2884:
  r11 = cn_var_原始名称;
  r12 = 复制字符串(r11);
  r13 = cn_var_唯一名称;
  r14 = 复制字符串(r13);
  r15 = cn_var_上下文;
  r16 = r15->局部变量映射表;
  r17 = cn_var_映射_0;
  goto if_merge_2885;

  if_merge_2885:
  return;
}

void 释放局部变量映射表(struct IR生成上下文* cn_var_上下文) {
  long long r1;
  char* r8;
  char* r10;
  struct IR生成上下文* r0;
  struct IR生成上下文* r2;
  struct 局部变量映射* r3;
  struct 局部变量映射* r4;
  struct 局部变量映射* r5;
  struct 局部变量映射* r6;
  struct 局部变量映射* r7;
  struct 局部变量映射* r9;
  struct 局部变量映射* r11;
  struct 局部变量映射* r12;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_2886; else goto if_merge_2887;

  if_then_2886:
  return;
  goto if_merge_2887;

  if_merge_2887:
  struct 局部变量映射* cn_var_映射_0;
  r2 = cn_var_上下文;
  r3 = r2->局部变量映射表;
  cn_var_映射_0 = r3;
  goto while_cond_2888;

  while_cond_2888:
  r4 = cn_var_映射_0;
  if (r4) goto while_body_2889; else goto while_exit_2890;

  while_body_2889:
  struct 局部变量映射* cn_var_下个_1;
  r5 = cn_var_映射_0;
  r6 = r5->下一个;
  cn_var_下个_1 = r6;
  r7 = cn_var_映射_0;
  r8 = r7->原始名称;
  释放内存(r8);
  r9 = cn_var_映射_0;
  r10 = r9->唯一名称;
  释放内存(r10);
  r11 = cn_var_映射_0;
  释放内存(r11);
  r12 = cn_var_下个_1;
  cn_var_映射_0 = r12;
  goto while_cond_2888;

  while_exit_2890:
  return;
}

struct IR生成上下文* 创建IR生成上下文() {
  long long r0;
  void* r1;
  struct IR生成上下文* r2;
  struct IR模块* r3;
  struct IR生成上下文* r4;

  entry:
  struct IR生成上下文* cn_var_上下文_0;
  r0 = sizeof(struct IR生成上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_2891; else goto if_merge_2892;

  if_then_2891:
  r3 = 创建IR模块();
  goto if_merge_2892;

  if_merge_2892:
  r4 = cn_var_上下文_0;
  return r4;
}

void 释放IR生成上下文(struct IR生成上下文* cn_var_上下文) {
  long long r1;
  struct IR生成上下文* r0;
  struct IR生成上下文* r2;
  struct IR生成上下文* r3;
  struct IR模块* r4;
  struct IR生成上下文* r5;
  struct IR模块* r6;
  struct IR生成上下文* r7;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_2893; else goto if_merge_2894;

  if_then_2893:
  return;
  goto if_merge_2894;

  if_merge_2894:
  r2 = cn_var_上下文;
  释放局部变量映射表(r2);
  r3 = cn_var_上下文;
  r4 = r3->模块;
  if (r4) goto if_then_2895; else goto if_merge_2896;

  if_then_2895:
  r5 = cn_var_上下文;
  r6 = r5->模块;
  释放IR模块(r6);
  goto if_merge_2896;

  if_merge_2896:
  r7 = cn_var_上下文;
  释放内存(r7);
  return;
}

enum IR操作码 二元运算符转IR(enum 二元运算符 cn_var_运算符) {
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
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  enum 二元运算符 r0;

  entry:
  r0 = cn_var_运算符;
  r1 = r0 == 0;
  if (r1) goto case_body_2898; else goto switch_check_2915;

  switch_check_2915:
  r2 = r0 == 1;
  if (r2) goto case_body_2899; else goto switch_check_2916;

  switch_check_2916:
  r3 = r0 == 2;
  if (r3) goto case_body_2900; else goto switch_check_2917;

  switch_check_2917:
  r4 = r0 == 3;
  if (r4) goto case_body_2901; else goto switch_check_2918;

  switch_check_2918:
  r5 = r0 == 4;
  if (r5) goto case_body_2902; else goto switch_check_2919;

  switch_check_2919:
  r6 = r0 == 5;
  if (r6) goto case_body_2903; else goto switch_check_2920;

  switch_check_2920:
  r7 = r0 == 6;
  if (r7) goto case_body_2904; else goto switch_check_2921;

  switch_check_2921:
  r8 = r0 == 7;
  if (r8) goto case_body_2905; else goto switch_check_2922;

  switch_check_2922:
  r9 = r0 == 8;
  if (r9) goto case_body_2906; else goto switch_check_2923;

  switch_check_2923:
  r10 = r0 == 9;
  if (r10) goto case_body_2907; else goto switch_check_2924;

  switch_check_2924:
  r11 = r0 == 10;
  if (r11) goto case_body_2908; else goto switch_check_2925;

  switch_check_2925:
  r12 = r0 == 11;
  if (r12) goto case_body_2909; else goto switch_check_2926;

  switch_check_2926:
  r13 = r0 == 12;
  if (r13) goto case_body_2910; else goto switch_check_2927;

  switch_check_2927:
  r14 = r0 == 13;
  if (r14) goto case_body_2911; else goto switch_check_2928;

  switch_check_2928:
  r15 = r0 == 14;
  if (r15) goto case_body_2912; else goto switch_check_2929;

  switch_check_2929:
  r16 = r0 == 15;
  if (r16) goto case_body_2913; else goto case_default_2914;

  case_body_2898:
  return 0;
  goto switch_merge_2897;

  case_body_2899:
  return 1;
  goto switch_merge_2897;

  case_body_2900:
  return 2;
  goto switch_merge_2897;

  case_body_2901:
  return 3;
  goto switch_merge_2897;

  case_body_2902:
  return 4;
  goto switch_merge_2897;

  case_body_2903:
  return 13;
  goto switch_merge_2897;

  case_body_2904:
  return 14;
  goto switch_merge_2897;

  case_body_2905:
  return 15;
  goto switch_merge_2897;

  case_body_2906:
  return 17;
  goto switch_merge_2897;

  case_body_2907:
  return 16;
  goto switch_merge_2897;

  case_body_2908:
  return 18;
  goto switch_merge_2897;

  case_body_2909:
  return 5;
  goto switch_merge_2897;

  case_body_2910:
  return 6;
  goto switch_merge_2897;

  case_body_2911:
  return 7;
  goto switch_merge_2897;

  case_body_2912:
  return 8;
  goto switch_merge_2897;

  case_body_2913:
  return 9;
  goto switch_merge_2897;

  case_default_2914:
  return 0;
  goto switch_merge_2897;

  switch_merge_2897:
  return 0;
}

struct IR操作数 生成表达式IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r4, r14, r16;
  struct 表达式节点* r0;
  struct 表达式节点* r3;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct IR生成上下文* r18;
  struct 表达式节点* r19;
  struct IR生成上下文* r21;
  struct 表达式节点* r22;
  struct IR生成上下文* r24;
  struct 表达式节点* r25;
  struct IR生成上下文* r27;
  struct 表达式节点* r28;
  struct IR生成上下文* r30;
  struct 表达式节点* r31;
  struct IR生成上下文* r33;
  struct 表达式节点* r34;
  struct IR生成上下文* r36;
  struct 表达式节点* r37;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  struct IR操作数 r2;
  struct IR操作数 r17;
  struct IR操作数 r20;
  struct IR操作数 r23;
  struct IR操作数 r26;
  struct IR操作数 r29;
  struct IR操作数 r32;
  struct IR操作数 r35;
  struct IR操作数 r38;
  struct IR操作数 r39;

  entry:
  r0 = cn_var_表达式;
  r1 = !r0;
  if (r1) goto if_then_2930; else goto if_merge_2931;

  if_then_2930:
  r2 = 空操作数();
  return r2;
  goto if_merge_2931;

  if_merge_2931:
  r3 = cn_var_表达式;
  r4 = r3->类型;
  r5 = r4 == 25;
  if (r5) goto case_body_2933; else goto switch_check_2942;

  switch_check_2942:
  r6 = r4 == 26;
  if (r6) goto case_body_2934; else goto switch_check_2943;

  switch_check_2943:
  r7 = r4 == 23;
  if (r7) goto case_body_2935; else goto switch_check_2944;

  switch_check_2944:
  r8 = r4 == 24;
  if (r8) goto case_body_2936; else goto switch_check_2945;

  switch_check_2945:
  r9 = r4 == 27;
  if (r9) goto case_body_2937; else goto switch_check_2946;

  switch_check_2946:
  r10 = r4 == 28;
  if (r10) goto case_body_2938; else goto switch_check_2947;

  switch_check_2947:
  r11 = r4 == 29;
  if (r11) goto case_body_2939; else goto switch_check_2948;

  switch_check_2948:
  r12 = r4 == 30;
  if (r12) goto case_body_2940; else goto case_default_2941;

  case_body_2933:
  r13 = cn_var_表达式;
  r14 = r13->整数值;
  r15 = cn_var_表达式;
  r16 = r15->类型信息;
  r17 = 整数常量操作数(r14, r16);
  return r17;
  goto switch_merge_2932;

  case_body_2934:
  r18 = cn_var_上下文;
  r19 = cn_var_表达式;
  r20 = 生成标识符IR(r18, r19);
  return r20;
  goto switch_merge_2932;

  case_body_2935:
  r21 = cn_var_上下文;
  r22 = cn_var_表达式;
  r23 = 生成二元运算IR(r21, r22);
  return r23;
  goto switch_merge_2932;

  case_body_2936:
  r24 = cn_var_上下文;
  r25 = cn_var_表达式;
  r26 = 生成一元运算IR(r24, r25);
  return r26;
  goto switch_merge_2932;

  case_body_2937:
  r27 = cn_var_上下文;
  r28 = cn_var_表达式;
  r29 = 生成函数调用IR(r27, r28);
  return r29;
  goto switch_merge_2932;

  case_body_2938:
  r30 = cn_var_上下文;
  r31 = cn_var_表达式;
  r32 = 生成成员访问IR(r30, r31);
  return r32;
  goto switch_merge_2932;

  case_body_2939:
  r33 = cn_var_上下文;
  r34 = cn_var_表达式;
  r35 = 生成数组访问IR(r33, r34);
  return r35;
  goto switch_merge_2932;

  case_body_2940:
  r36 = cn_var_上下文;
  r37 = cn_var_表达式;
  r38 = 生成赋值IR(r36, r37);
  return r38;
  goto switch_merge_2932;

  case_default_2941:
  r39 = 空操作数();
  return r39;
  goto switch_merge_2932;

  switch_merge_2932:
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成标识符IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r12, r13, r15, r19, r30, r32;
  char* r9;
  char* r10;
  char* r17;
  char* r27;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r11;
  struct 表达式节点* r14;
  struct 表达式节点* r18;
  struct IR指令* r24;
  struct IR生成上下文* r25;
  struct IR指令* r26;
  struct 表达式节点* r29;
  struct 表达式节点* r31;
  struct IR操作数 r5;
  struct IR操作数 r16;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r23;
  struct IR操作数 r28;
  struct IR操作数 r33;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2952; else goto logic_rhs_2951;

  if_then_2949:
  r5 = 空操作数();
  return r5;
  goto if_merge_2950;

  if_merge_2950:
  char* cn_var_唯一名_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->标识符名称;
  r9 = 查找局部变量唯一名(r6, r8);
  cn_var_唯一名_0 = r9;
  r10 = cn_var_唯一名_0;
  if (r10) goto if_then_2953; else goto if_merge_2954;

  logic_rhs_2951:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2952;

  logic_merge_2952:
  if (r4) goto if_then_2949; else goto if_merge_2950;

  if_then_2953:
  long long cn_var_寄存器_1;
  r11 = cn_var_上下文;
  r12 = 分配虚拟寄存器(r11);
  cn_var_寄存器_1 = r12;
  struct IR操作数 cn_var_目标_2;
  r13 = cn_var_寄存器_1;
  r14 = cn_var_表达式;
  r15 = r14->类型信息;
  r16 = 寄存器操作数(r13, r15);
  cn_var_目标_2 = r16;
  struct IR操作数 cn_var_源_3;
  r17 = cn_var_唯一名_0;
  r18 = cn_var_表达式;
  r19 = r18->类型信息;
  r20 = 全局符号操作数(r17, r19);
  cn_var_源_3 = r20;
  struct IR指令* cn_var_指令_4;
  r21 = cn_var_目标_2;
  r22 = cn_var_源_3;
  r23 = 空操作数();
  r24 = 创建IR指令(20, r21, r22, r23);
  cn_var_指令_4 = r24;
  r25 = cn_var_上下文;
  r26 = cn_var_指令_4;
  发射指令(r25, r26);
  r27 = cn_var_唯一名_0;
  释放内存(r27);
  r28 = cn_var_目标_2;
  return r28;
  goto if_merge_2954;

  if_merge_2954:
  r29 = cn_var_表达式;
  r30 = r29->标识符名称;
  r31 = cn_var_表达式;
  r32 = r31->类型信息;
  r33 = 全局符号操作数(r30, r32);
  return r33;
}

struct IR操作数 生成二元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r12, r15, r16, r18, r21;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r11;
  struct IR生成上下文* r14;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct IR指令* r27;
  struct IR生成上下文* r28;
  struct IR指令* r29;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r13;
  struct IR操作数 r19;
  enum IR操作码 r22;
  enum IR操作码 r23;
  struct IR操作数 r24;
  struct IR操作数 r25;
  struct IR操作数 r26;
  struct IR操作数 r30;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2958; else goto logic_rhs_2957;

  if_then_2955:
  r5 = 空操作数();
  return r5;
  goto if_merge_2956;

  if_merge_2956:
  struct IR操作数 cn_var_左_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->左操作数;
  r9 = 生成表达式IR(r6, r8);
  cn_var_左_0 = r9;
  struct IR操作数 cn_var_右_1;
  r10 = cn_var_上下文;
  r11 = cn_var_表达式;
  r12 = r11->右操作数;
  r13 = 生成表达式IR(r10, r12);
  cn_var_右_1 = r13;
  long long cn_var_结果_2;
  r14 = cn_var_上下文;
  r15 = 分配虚拟寄存器(r14);
  cn_var_结果_2 = r15;
  struct IR操作数 cn_var_目标_3;
  r16 = cn_var_结果_2;
  r17 = cn_var_表达式;
  r18 = r17->类型信息;
  r19 = 寄存器操作数(r16, r18);
  cn_var_目标_3 = r19;
  enum IR操作码 cn_var_操作码_4;
  r20 = cn_var_表达式;
  r21 = r20->运算符;
  r22 = 二元运算符转IR(r21);
  cn_var_操作码_4 = r22;
  struct IR指令* cn_var_指令_5;
  r23 = cn_var_操作码_4;
  r24 = cn_var_目标_3;
  r25 = cn_var_左_0;
  r26 = cn_var_右_1;
  r27 = 创建IR指令(r23, r24, r25, r26);
  cn_var_指令_5 = r27;
  r28 = cn_var_上下文;
  r29 = cn_var_指令_5;
  发射指令(r28, r29);
  r30 = cn_var_目标_3;
  return r30;

  logic_rhs_2957:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2958;

  logic_merge_2958:
  if (r4) goto if_then_2955; else goto if_merge_2956;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成一元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14, r17;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r13;
  struct 表达式节点* r16;
  struct IR指令* r25;
  struct IR生成上下文* r26;
  struct IR指令* r27;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r15;
  enum IR操作码 r21;
  struct IR操作数 r22;
  struct IR操作数 r23;
  struct IR操作数 r24;
  struct IR操作数 r28;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2962; else goto logic_rhs_2961;

  if_then_2959:
  r5 = 空操作数();
  return r5;
  goto if_merge_2960;

  if_merge_2960:
  struct IR操作数 cn_var_操作数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->操作数;
  r9 = 生成表达式IR(r6, r8);
  cn_var_操作数_0 = r9;
  long long cn_var_结果_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_结果_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_结果_1;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  r15 = 寄存器操作数(r12, r14);
  cn_var_目标_2 = r15;
  enum IR操作码 cn_var_操作码_3;
  cn_var_操作码_3 = 10;
  r16 = cn_var_表达式;
  r17 = r16->运算符;
  r18 = r17 == 1;
  if (r18) goto case_body_2964; else goto switch_check_2968;

  logic_rhs_2961:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2962;

  logic_merge_2962:
  if (r4) goto if_then_2959; else goto if_merge_2960;

  switch_check_2968:
  r19 = r17 == 4;
  if (r19) goto case_body_2965; else goto switch_check_2969;

  switch_check_2969:
  r20 = r17 == 0;
  if (r20) goto case_body_2966; else goto case_default_2967;

  case_body_2964:
  cn_var_操作码_3 = 10;
  goto switch_merge_2963;
  goto switch_merge_2963;

  case_body_2965:
  cn_var_操作码_3 = 11;
  goto switch_merge_2963;
  goto switch_merge_2963;

  case_body_2966:
  cn_var_操作码_3 = 12;
  goto switch_merge_2963;
  goto switch_merge_2963;

  case_default_2967:
  goto switch_merge_2963;
  goto switch_merge_2963;

  switch_merge_2963:
  struct IR指令* cn_var_指令_4;
  r21 = cn_var_操作码_3;
  r22 = cn_var_目标_2;
  r23 = cn_var_操作数_0;
  r24 = 空操作数();
  r25 = 创建IR指令(r21, r22, r23, r24);
  cn_var_指令_4 = r25;
  r26 = cn_var_上下文;
  r27 = cn_var_指令_4;
  发射指令(r26, r27);
  r28 = cn_var_目标_2;
  return r28;
}

struct IR操作数 生成函数调用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14, r21, r23, r25, r26, r28, r29, r30, r31, r32, r33, r35, r37, r44, r45, r46, r48;
  void* r43;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r13;
  struct IR指令* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r22;
  void* r34;
  struct 表达式节点* r36;
  struct IR生成上下文* r39;
  struct IR指令* r42;
  struct IR生成上下文* r49;
  struct IR指令* r50;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct 参数 r24;
  struct 参数 r27;
  struct 参数 r38;
  struct 参数 r40;
  struct IR操作数 r41;
  struct 参数 r47;
  struct IR操作数 r51;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2973; else goto logic_rhs_2972;

  if_then_2970:
  r5 = 空操作数();
  return r5;
  goto if_merge_2971;

  if_merge_2971:
  struct IR操作数 cn_var_被调函数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->被调用函数;
  r9 = 生成表达式IR(r6, r8);
  cn_var_被调函数_0 = r9;
  long long cn_var_结果_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_结果_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_结果_1;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  r15 = 寄存器操作数(r12, r14);
  cn_var_目标_2 = r15;
  struct IR指令* cn_var_指令_3;
  r16 = cn_var_目标_2;
  r17 = cn_var_被调函数_0;
  r18 = 空操作数();
  r19 = 创建IR指令(28, r16, r17, r18);
  cn_var_指令_3 = r19;
  r20 = cn_var_表达式;
  r21 = r20->参数列表;
  if (r21) goto if_then_2974; else goto if_merge_2975;

  logic_rhs_2972:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2973;

  logic_merge_2973:
  if (r4) goto if_then_2970; else goto if_merge_2971;

  if_then_2974:
  long long cn_var_数量_4;
  cn_var_数量_4 = 0;
  struct 表达式节点* cn_var_参数_5;
  r22 = cn_var_表达式;
  r23 = r22->参数列表;
  cn_var_参数_5 = r23;
  goto while_cond_2976;

  if_merge_2975:
  r49 = cn_var_上下文;
  r50 = cn_var_指令_3;
  发射指令(r49, r50);
  r51 = cn_var_目标_2;
  return r51;

  while_cond_2976:
  r24 = cn_var_参数_5;
  if (r24) goto while_body_2977; else goto while_exit_2978;

  while_body_2977:
  r25 = cn_var_数量_4;
  r26 = r25 + 1;
  cn_var_数量_4 = r26;
  r27 = cn_var_参数_5;
  r28 = r27.下一个;
  cn_var_参数_5 = r28;
  goto while_cond_2976;

  while_exit_2978:
  r29 = cn_var_数量_4;
  r30 = r29 > 0;
  if (r30) goto if_then_2979; else goto if_merge_2980;

  if_then_2979:
  r31 = cn_var_数量_4;
  r32 = sizeof(struct IR操作数);
  r33 = r31 * r32;
  r34 = 分配内存(r33);
  r35 = cn_var_数量_4;
  r36 = cn_var_表达式;
  r37 = r36->参数列表;
  cn_var_参数_5 = r37;
  long long cn_var_索引_6;
  cn_var_索引_6 = 0;
  goto while_cond_2981;

  if_merge_2980:
  goto if_merge_2975;

  while_cond_2981:
  r38 = cn_var_参数_5;
  if (r38) goto while_body_2982; else goto while_exit_2983;

  while_body_2982:
  r39 = cn_var_上下文;
  r40 = cn_var_参数_5;
  r41 = 生成表达式IR(r39, r40);
  r42 = cn_var_指令_3;
  r43 = r42->额外参数;
  r44 = cn_var_索引_6;
    { long long _tmp_r13 = r41; cn_rt_array_set_element(r43, r44, &_tmp_r13, 8); }
  r45 = cn_var_索引_6;
  r46 = r45 + 1;
  cn_var_索引_6 = r46;
  r47 = cn_var_参数_5;
  r48 = r47.下一个;
  cn_var_参数_5 = r48;
  goto while_cond_2981;

  while_exit_2983:
  goto if_merge_2980;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成成员访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14, r17;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r13;
  struct 表达式节点* r16;
  struct IR指令* r22;
  struct IR生成上下文* r23;
  struct IR指令* r24;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r15;
  struct IR操作数 r18;
  struct IR操作数 r19;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r25;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2987; else goto logic_rhs_2986;

  if_then_2984:
  r5 = 空操作数();
  return r5;
  goto if_merge_2985;

  if_merge_2985:
  struct IR操作数 cn_var_对象_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->对象表达式;
  r9 = 生成表达式IR(r6, r8);
  cn_var_对象_0 = r9;
  long long cn_var_结果_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_结果_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_结果_1;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  r15 = 寄存器操作数(r12, r14);
  cn_var_目标_2 = r15;
  struct IR操作数 cn_var_成员索引_3;
  r16 = cn_var_表达式;
  r17 = r16->成员索引;
  r18 = 整数常量操作数(r17, 0);
  cn_var_成员索引_3 = r18;
  struct IR指令* cn_var_指令_4;
  r19 = cn_var_目标_2;
  r20 = cn_var_对象_0;
  r21 = cn_var_成员索引_3;
  r22 = 创建IR指令(31, r19, r20, r21);
  cn_var_指令_4 = r22;
  r23 = cn_var_上下文;
  r24 = cn_var_指令_4;
  发射指令(r23, r24);
  r25 = cn_var_目标_2;
  return r25;

  logic_rhs_2986:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2987;

  logic_merge_2987:
  if (r4) goto if_then_2984; else goto if_merge_2985;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成数组访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r12, r15, r16, r18;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r11;
  struct IR生成上下文* r14;
  struct 表达式节点* r17;
  struct IR指令* r23;
  struct IR生成上下文* r24;
  struct IR指令* r25;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r13;
  struct IR操作数 r19;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r26;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2991; else goto logic_rhs_2990;

  if_then_2988:
  r5 = 空操作数();
  return r5;
  goto if_merge_2989;

  if_merge_2989:
  struct IR操作数 cn_var_数组_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->数组表达式;
  r9 = 生成表达式IR(r6, r8);
  cn_var_数组_0 = r9;
  struct IR操作数 cn_var_索引_1;
  r10 = cn_var_上下文;
  r11 = cn_var_表达式;
  r12 = r11->索引表达式;
  r13 = 生成表达式IR(r10, r12);
  cn_var_索引_1 = r13;
  long long cn_var_结果_2;
  r14 = cn_var_上下文;
  r15 = 分配虚拟寄存器(r14);
  cn_var_结果_2 = r15;
  struct IR操作数 cn_var_目标_3;
  r16 = cn_var_结果_2;
  r17 = cn_var_表达式;
  r18 = r17->类型信息;
  r19 = 寄存器操作数(r16, r18);
  cn_var_目标_3 = r19;
  struct IR指令* cn_var_指令_4;
  r20 = cn_var_目标_3;
  r21 = cn_var_数组_0;
  r22 = cn_var_索引_1;
  r23 = 创建IR指令(20, r20, r21, r22);
  cn_var_指令_4 = r23;
  r24 = cn_var_上下文;
  r25 = cn_var_指令_4;
  发射指令(r24, r25);
  r26 = cn_var_目标_3;
  return r26;

  logic_rhs_2990:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2991;

  logic_merge_2991:
  if (r4) goto if_then_2988; else goto if_merge_2989;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成赋值IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14, r17, r18, r23, r24, r35;
  char* r19;
  char* r20;
  char* r21;
  char* r32;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct 表达式节点* r10;
  struct IR生成上下文* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r22;
  struct IR指令* r29;
  struct IR生成上下文* r30;
  struct IR指令* r31;
  struct IR生成上下文* r33;
  struct 表达式节点* r34;
  struct IR指令* r40;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct 标识符表达式 r13;
  struct IR操作数 r25;
  struct IR操作数 r26;
  struct IR操作数 r27;
  struct IR操作数 r28;
  struct IR操作数 r36;
  struct IR操作数 r37;
  struct IR操作数 r38;
  struct IR操作数 r39;
  struct IR操作数 r43;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2995; else goto logic_rhs_2994;

  if_then_2992:
  r5 = 空操作数();
  return r5;
  goto if_merge_2993;

  if_merge_2993:
  struct IR操作数 cn_var_值_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->右侧表达式;
  r9 = 生成表达式IR(r6, r8);
  cn_var_值_0 = r9;
  r10 = cn_var_表达式;
  r11 = r10->左侧表达式;
  r12 = r11.类型;
  r13 = cn_var_标识符表达式;
  r14 = r12 == r13;
  if (r14) goto if_then_2996; else goto if_else_2997;

  logic_rhs_2994:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2995;

  logic_merge_2995:
  if (r4) goto if_then_2992; else goto if_merge_2993;

  if_then_2996:
  char* cn_var_唯一名_1;
  r15 = cn_var_上下文;
  r16 = cn_var_表达式;
  r17 = r16->左侧表达式;
  r18 = r17.标识符名称;
  r19 = 查找局部变量唯一名(r15, r18);
  cn_var_唯一名_1 = r19;
  r20 = cn_var_唯一名_1;
  if (r20) goto if_then_2999; else goto if_merge_3000;

  if_else_2997:
  struct IR操作数 cn_var_目标_4;
  r33 = cn_var_上下文;
  r34 = cn_var_表达式;
  r35 = r34->左侧表达式;
  r36 = 生成表达式IR(r33, r35);
  cn_var_目标_4 = r36;
  struct IR指令* cn_var_指令_5;
  r37 = cn_var_目标_4;
  r38 = cn_var_值_0;
  r39 = 空操作数();
  r40 = 创建IR指令(21, r37, r38, r39);
  cn_var_指令_5 = r40;
  r41 = cn_var_上下文;
  r42 = cn_var_指令_5;
  发射指令(r41, r42);
  goto if_merge_2998;

  if_merge_2998:
  r43 = cn_var_值_0;
  return r43;

  if_then_2999:
  struct IR操作数 cn_var_目标_2;
  r21 = cn_var_唯一名_1;
  r22 = cn_var_表达式;
  r23 = r22->左侧表达式;
  r24 = r23.类型信息;
  r25 = 全局符号操作数(r21, r24);
  cn_var_目标_2 = r25;
  struct IR指令* cn_var_指令_3;
  r26 = cn_var_目标_2;
  r27 = cn_var_值_0;
  r28 = 空操作数();
  r29 = 创建IR指令(21, r26, r27, r28);
  cn_var_指令_3 = r29;
  r30 = cn_var_上下文;
  r31 = cn_var_指令_3;
  发射指令(r30, r31);
  r32 = cn_var_唯一名_1;
  释放内存(r32);
  goto if_merge_3000;

  if_merge_3000:
  goto if_merge_2998;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成取地址IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r13;
  struct IR指令* r19;
  struct IR生成上下文* r20;
  struct IR指令* r21;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r22;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3004; else goto logic_rhs_3003;

  if_then_3001:
  r5 = 空操作数();
  return r5;
  goto if_merge_3002;

  if_merge_3002:
  struct IR操作数 cn_var_操作数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->操作数;
  r9 = 生成表达式IR(r6, r8);
  cn_var_操作数_0 = r9;
  long long cn_var_结果_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_结果_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_结果_1;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  r15 = 寄存器操作数(r12, r14);
  cn_var_目标_2 = r15;
  struct IR指令* cn_var_指令_3;
  r16 = cn_var_目标_2;
  r17 = cn_var_操作数_0;
  r18 = 空操作数();
  r19 = 创建IR指令(23, r16, r17, r18);
  cn_var_指令_3 = r19;
  r20 = cn_var_上下文;
  r21 = cn_var_指令_3;
  发射指令(r20, r21);
  r22 = cn_var_目标_2;
  return r22;

  logic_rhs_3003:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3004;

  logic_merge_3004:
  if (r4) goto if_then_3001; else goto if_merge_3002;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成解引用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r8, r11, r12, r14;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r10;
  struct 表达式节点* r13;
  struct IR指令* r19;
  struct IR生成上下文* r20;
  struct IR指令* r21;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r22;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3008; else goto logic_rhs_3007;

  if_then_3005:
  r5 = 空操作数();
  return r5;
  goto if_merge_3006;

  if_merge_3006:
  struct IR操作数 cn_var_指针_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式;
  r8 = r7->操作数;
  r9 = 生成表达式IR(r6, r8);
  cn_var_指针_0 = r9;
  long long cn_var_结果_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_结果_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_结果_1;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  r15 = 寄存器操作数(r12, r14);
  cn_var_目标_2 = r15;
  struct IR指令* cn_var_指令_3;
  r16 = cn_var_目标_2;
  r17 = cn_var_指针_0;
  r18 = 空操作数();
  r19 = 创建IR指令(24, r16, r17, r18);
  cn_var_指令_3 = r19;
  r20 = cn_var_上下文;
  r21 = cn_var_指令_3;
  发射指令(r20, r21);
  r22 = cn_var_目标_2;
  return r22;

  logic_rhs_3007:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3008;

  logic_merge_3008:
  if (r4) goto if_then_3005; else goto if_merge_3006;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

void 生成语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r6, r18;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r5;
  struct IR生成上下文* r16;
  struct 语句节点* r17;
  struct IR生成上下文* r20;
  struct 语句节点* r21;
  struct IR生成上下文* r22;
  struct 语句节点* r23;
  struct IR生成上下文* r24;
  struct 语句节点* r25;
  struct IR生成上下文* r26;
  struct 语句节点* r27;
  struct IR生成上下文* r28;
  struct 语句节点* r29;
  struct IR生成上下文* r30;
  struct IR生成上下文* r31;
  struct IR生成上下文* r32;
  struct 语句节点* r33;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  struct IR操作数 r19;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3012; else goto logic_rhs_3011;

  if_then_3009:
  return;
  goto if_merge_3010;

  if_merge_3010:
  r5 = cn_var_语句;
  r6 = r5->类型;
  r7 = r6 == 11;
  if (r7) goto case_body_3014; else goto switch_check_3024;

  logic_rhs_3011:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3012;

  logic_merge_3012:
  if (r4) goto if_then_3009; else goto if_merge_3010;

  switch_check_3024:
  r8 = r6 == 2;
  if (r8) goto case_body_3015; else goto switch_check_3025;

  switch_check_3025:
  r9 = r6 == 13;
  if (r9) goto case_body_3016; else goto switch_check_3026;

  switch_check_3026:
  r10 = r6 == 14;
  if (r10) goto case_body_3017; else goto switch_check_3027;

  switch_check_3027:
  r11 = r6 == 15;
  if (r11) goto case_body_3018; else goto switch_check_3028;

  switch_check_3028:
  r12 = r6 == 16;
  if (r12) goto case_body_3019; else goto switch_check_3029;

  switch_check_3029:
  r13 = r6 == 17;
  if (r13) goto case_body_3020; else goto switch_check_3030;

  switch_check_3030:
  r14 = r6 == 18;
  if (r14) goto case_body_3021; else goto switch_check_3031;

  switch_check_3031:
  r15 = r6 == 12;
  if (r15) goto case_body_3022; else goto case_default_3023;

  case_body_3014:
  r16 = cn_var_上下文;
  r17 = cn_var_语句;
  r18 = r17->表达式;
  r19 = 生成表达式IR(r16, r18);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3015:
  r20 = cn_var_上下文;
  r21 = cn_var_语句;
  生成变量声明IR(r20, r21);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3016:
  r22 = cn_var_上下文;
  r23 = cn_var_语句;
  生成如果语句IR(r22, r23);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3017:
  r24 = cn_var_上下文;
  r25 = cn_var_语句;
  生成当循环IR(r24, r25);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3018:
  r26 = cn_var_上下文;
  r27 = cn_var_语句;
  生成循环IR(r26, r27);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3019:
  r28 = cn_var_上下文;
  r29 = cn_var_语句;
  生成返回语句IR(r28, r29);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3020:
  r30 = cn_var_上下文;
  生成中断语句IR(r30);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3021:
  r31 = cn_var_上下文;
  生成继续语句IR(r31);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_body_3022:
  r32 = cn_var_上下文;
  r33 = cn_var_语句;
  生成块语句IR(r32, r33);
  goto switch_merge_3013;
  goto switch_merge_3013;

  case_default_3023:
  goto switch_merge_3013;
  goto switch_merge_3013;

  switch_merge_3013:
  return;
}

void 生成变量声明IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r6, r10, r13, r14, r16, r19, r28, r31, r35, r37, r43;
  char* r7;
  char* r11;
  char* r33;
  char* r44;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r5;
  struct IR生成上下文* r8;
  struct 语句节点* r9;
  struct IR生成上下文* r12;
  struct 语句节点* r15;
  struct 语句节点* r18;
  struct IR指令* r24;
  struct IR生成上下文* r25;
  struct IR指令* r26;
  struct 语句节点* r27;
  struct IR生成上下文* r29;
  struct 语句节点* r30;
  struct 语句节点* r34;
  struct IR指令* r41;
  struct IR生成上下文* r42;
  struct IR操作数 r17;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r23;
  struct IR操作数 r32;
  struct IR操作数 r36;
  struct IR操作数 r38;
  struct IR操作数 r39;
  struct IR操作数 r40;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3035; else goto logic_rhs_3034;

  if_then_3032:
  return;
  goto if_merge_3033;

  if_merge_3033:
  char* cn_var_唯一名_0;
  r5 = cn_var_语句;
  r6 = r5->变量名称;
  r7 = 生成唯一变量名(r6);
  cn_var_唯一名_0 = r7;
  r8 = cn_var_上下文;
  r9 = cn_var_语句;
  r10 = r9->变量名称;
  r11 = cn_var_唯一名_0;
  添加局部变量映射(r8, r10, r11);
  long long cn_var_寄存器_1;
  r12 = cn_var_上下文;
  r13 = 分配虚拟寄存器(r12);
  cn_var_寄存器_1 = r13;
  struct IR操作数 cn_var_目标_2;
  r14 = cn_var_寄存器_1;
  r15 = cn_var_语句;
  r16 = r15->类型信息;
  r17 = 寄存器操作数(r14, r16);
  cn_var_目标_2 = r17;
  struct IR操作数 cn_var_大小_3;
  r18 = cn_var_语句;
  r19 = r18->类型大小;
  r20 = 整数常量操作数(r19, 0);
  cn_var_大小_3 = r20;
  struct IR指令* cn_var_指令_4;
  r21 = cn_var_目标_2;
  r22 = cn_var_大小_3;
  r23 = 空操作数();
  r24 = 创建IR指令(19, r21, r22, r23);
  cn_var_指令_4 = r24;
  r25 = cn_var_上下文;
  r26 = cn_var_指令_4;
  发射指令(r25, r26);
  r27 = cn_var_语句;
  r28 = r27->初始值表达式;
  if (r28) goto if_then_3036; else goto if_merge_3037;

  logic_rhs_3034:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3035;

  logic_merge_3035:
  if (r4) goto if_then_3032; else goto if_merge_3033;

  if_then_3036:
  struct IR操作数 cn_var_值_5;
  r29 = cn_var_上下文;
  r30 = cn_var_语句;
  r31 = r30->初始值表达式;
  r32 = 生成表达式IR(r29, r31);
  cn_var_值_5 = r32;
  struct IR操作数 cn_var_目标变量_6;
  r33 = cn_var_唯一名_0;
  r34 = cn_var_语句;
  r35 = r34->类型信息;
  r36 = 全局符号操作数(r33, r35);
  cn_var_目标变量_6 = r36;
  struct IR指令* cn_var_存储指令_7;
  r37 = cn_var_存储指令_7;
  r38 = cn_var_目标变量_6;
  r39 = cn_var_值_5;
  r40 = 空操作数();
  r41 = 创建IR指令(r37, r38, r39, r40);
  cn_var_存储指令_7 = r41;
  r42 = cn_var_上下文;
  r43 = cn_var_存储指令_7;
  发射指令(r42, r43);
  goto if_merge_3037;

  if_merge_3037:
  r44 = cn_var_唯一名_0;
  释放内存(r44);
  return;
}

void 生成如果语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r7, r36, r50, r53;
  char* r9;
  char* r10;
  char* r11;
  char* r12;
  char* r14;
  char* r16;
  char* r18;
  char* r19;
  char* r20;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r5;
  struct 语句节点* r6;
  struct 基本块* r13;
  struct 基本块* r15;
  struct 基本块* r17;
  struct 基本块* r22;
  struct 基本块* r24;
  struct IR指令* r26;
  struct IR生成上下文* r27;
  struct IR指令* r28;
  struct IR生成上下文* r29;
  struct IR函数* r30;
  struct 基本块* r31;
  struct IR生成上下文* r32;
  struct 基本块* r33;
  struct IR生成上下文* r34;
  struct 语句节点* r35;
  struct 基本块* r37;
  struct IR指令* r41;
  struct IR生成上下文* r42;
  struct IR指令* r43;
  struct IR生成上下文* r44;
  struct IR函数* r45;
  struct 基本块* r46;
  struct IR生成上下文* r47;
  struct 基本块* r48;
  struct 语句节点* r49;
  struct IR生成上下文* r51;
  struct 语句节点* r52;
  struct 基本块* r54;
  struct IR指令* r58;
  struct IR生成上下文* r59;
  struct IR指令* r60;
  struct IR生成上下文* r61;
  struct IR函数* r62;
  struct 基本块* r63;
  struct IR生成上下文* r64;
  struct 基本块* r65;
  struct IR操作数 r8;
  struct IR操作数 r21;
  struct IR操作数 r23;
  struct IR操作数 r25;
  struct IR操作数 r38;
  struct IR操作数 r39;
  struct IR操作数 r40;
  struct IR操作数 r55;
  struct IR操作数 r56;
  struct IR操作数 r57;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3041; else goto logic_rhs_3040;

  if_then_3038:
  return;
  goto if_merge_3039;

  if_merge_3039:
  struct IR操作数 cn_var_条件_0;
  r5 = cn_var_上下文;
  r6 = cn_var_语句;
  r7 = r6->条件表达式;
  r8 = 生成表达式IR(r5, r7);
  cn_var_条件_0 = r8;
  char* cn_var_真块名_1;
  r9 = 生成块名称("if_true");
  cn_var_真块名_1 = r9;
  char* cn_var_假块名_2;
  r10 = 生成块名称("if_false");
  cn_var_假块名_2 = r10;
  char* cn_var_结束块名_3;
  r11 = 生成块名称("if_end");
  cn_var_结束块名_3 = r11;
  struct 基本块* cn_var_真块_4;
  r12 = cn_var_真块名_1;
  r13 = 创建基本块(r12);
  cn_var_真块_4 = r13;
  struct 基本块* cn_var_假块_5;
  r14 = cn_var_假块名_2;
  r15 = 创建基本块(r14);
  cn_var_假块_5 = r15;
  struct 基本块* cn_var_结束块_6;
  r16 = cn_var_结束块名_3;
  r17 = 创建基本块(r16);
  cn_var_结束块_6 = r17;
  r18 = cn_var_真块名_1;
  释放内存(r18);
  r19 = cn_var_假块名_2;
  释放内存(r19);
  r20 = cn_var_结束块名_3;
  释放内存(r20);
  struct IR指令* cn_var_跳转_7;
  r21 = cn_var_条件_0;
  r22 = cn_var_真块_4;
  r23 = 标签操作数(r22);
  r24 = cn_var_假块_5;
  r25 = 标签操作数(r24);
  r26 = 创建IR指令(27, r21, r23, r25);
  cn_var_跳转_7 = r26;
  r27 = cn_var_上下文;
  r28 = cn_var_跳转_7;
  发射指令(r27, r28);
  r29 = cn_var_上下文;
  r30 = r29->当前函数;
  r31 = cn_var_真块_4;
  添加基本块(r30, r31);
  r32 = cn_var_上下文;
  r33 = cn_var_真块_4;
  切换基本块(r32, r33);
  r34 = cn_var_上下文;
  r35 = cn_var_语句;
  r36 = r35->真分支语句;
  生成语句IR(r34, r36);
  struct IR指令* cn_var_跳转到结束_8;
  r37 = cn_var_结束块_6;
  r38 = 标签操作数(r37);
  r39 = 空操作数();
  r40 = 空操作数();
  r41 = 创建IR指令(26, r38, r39, r40);
  cn_var_跳转到结束_8 = r41;
  r42 = cn_var_上下文;
  r43 = cn_var_跳转到结束_8;
  发射指令(r42, r43);
  r44 = cn_var_上下文;
  r45 = r44->当前函数;
  r46 = cn_var_假块_5;
  添加基本块(r45, r46);
  r47 = cn_var_上下文;
  r48 = cn_var_假块_5;
  切换基本块(r47, r48);
  r49 = cn_var_语句;
  r50 = r49->假分支语句;
  if (r50) goto if_then_3042; else goto if_merge_3043;

  logic_rhs_3040:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3041;

  logic_merge_3041:
  if (r4) goto if_then_3038; else goto if_merge_3039;

  if_then_3042:
  r51 = cn_var_上下文;
  r52 = cn_var_语句;
  r53 = r52->假分支语句;
  生成语句IR(r51, r53);
  goto if_merge_3043;

  if_merge_3043:
  struct IR指令* cn_var_跳转到结束2_9;
  r54 = cn_var_结束块_6;
  r55 = 标签操作数(r54);
  r56 = 空操作数();
  r57 = 空操作数();
  r58 = 创建IR指令(26, r55, r56, r57);
  cn_var_跳转到结束2_9 = r58;
  r59 = cn_var_上下文;
  r60 = cn_var_跳转到结束2_9;
  发射指令(r59, r60);
  r61 = cn_var_上下文;
  r62 = r61->当前函数;
  r63 = cn_var_结束块_6;
  添加基本块(r62, r63);
  r64 = cn_var_上下文;
  r65 = cn_var_结束块_6;
  切换基本块(r64, r65);
  return;
}

void 生成当循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r37, r54;
  char* r5;
  char* r6;
  char* r7;
  char* r8;
  char* r10;
  char* r12;
  char* r14;
  char* r15;
  char* r16;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 基本块* r9;
  struct 基本块* r11;
  struct 基本块* r13;
  struct IR生成上下文* r17;
  struct 基本块* r18;
  struct IR生成上下文* r19;
  struct 基本块* r20;
  struct 基本块* r21;
  struct 基本块* r22;
  struct 基本块* r23;
  struct IR指令* r27;
  struct IR生成上下文* r28;
  struct IR指令* r29;
  struct IR生成上下文* r30;
  struct IR函数* r31;
  struct 基本块* r32;
  struct IR生成上下文* r33;
  struct 基本块* r34;
  struct IR生成上下文* r35;
  struct 语句节点* r36;
  struct 基本块* r40;
  struct 基本块* r42;
  struct IR指令* r44;
  struct IR生成上下文* r45;
  struct IR指令* r46;
  struct IR生成上下文* r47;
  struct IR函数* r48;
  struct 基本块* r49;
  struct IR生成上下文* r50;
  struct 基本块* r51;
  struct IR生成上下文* r52;
  struct 语句节点* r53;
  struct 基本块* r55;
  struct IR指令* r59;
  struct IR生成上下文* r60;
  struct IR指令* r61;
  struct IR生成上下文* r62;
  struct IR函数* r63;
  struct 基本块* r64;
  struct IR生成上下文* r65;
  struct 基本块* r66;
  struct 基本块* r67;
  struct 基本块* r68;
  struct IR操作数 r24;
  struct IR操作数 r25;
  struct IR操作数 r26;
  struct IR操作数 r38;
  struct IR操作数 r39;
  struct IR操作数 r41;
  struct IR操作数 r43;
  struct IR操作数 r56;
  struct IR操作数 r57;
  struct IR操作数 r58;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3047; else goto logic_rhs_3046;

  if_then_3044:
  return;
  goto if_merge_3045;

  if_merge_3045:
  char* cn_var_条件块名_0;
  r5 = 生成块名称("while_cond");
  cn_var_条件块名_0 = r5;
  char* cn_var_循环体名_1;
  r6 = 生成块名称("while_body");
  cn_var_循环体名_1 = r6;
  char* cn_var_退出块名_2;
  r7 = 生成块名称("while_exit");
  cn_var_退出块名_2 = r7;
  struct 基本块* cn_var_条件块_3;
  r8 = cn_var_条件块名_0;
  r9 = 创建基本块(r8);
  cn_var_条件块_3 = r9;
  struct 基本块* cn_var_循环体块_4;
  r10 = cn_var_循环体名_1;
  r11 = 创建基本块(r10);
  cn_var_循环体块_4 = r11;
  struct 基本块* cn_var_退出块_5;
  r12 = cn_var_退出块名_2;
  r13 = 创建基本块(r12);
  cn_var_退出块_5 = r13;
  r14 = cn_var_条件块名_0;
  释放内存(r14);
  r15 = cn_var_循环体名_1;
  释放内存(r15);
  r16 = cn_var_退出块名_2;
  释放内存(r16);
  struct 基本块* cn_var_原退出_6;
  r17 = cn_var_上下文;
  r18 = r17->循环退出块;
  cn_var_原退出_6 = r18;
  struct 基本块* cn_var_原继续_7;
  r19 = cn_var_上下文;
  r20 = r19->循环继续块;
  cn_var_原继续_7 = r20;
  r21 = cn_var_退出块_5;
  r22 = cn_var_条件块_3;
  struct IR指令* cn_var_跳转到条件_8;
  r23 = cn_var_条件块_3;
  r24 = 标签操作数(r23);
  r25 = 空操作数();
  r26 = 空操作数();
  r27 = 创建IR指令(26, r24, r25, r26);
  cn_var_跳转到条件_8 = r27;
  r28 = cn_var_上下文;
  r29 = cn_var_跳转到条件_8;
  发射指令(r28, r29);
  r30 = cn_var_上下文;
  r31 = r30->当前函数;
  r32 = cn_var_条件块_3;
  添加基本块(r31, r32);
  r33 = cn_var_上下文;
  r34 = cn_var_条件块_3;
  切换基本块(r33, r34);
  struct IR操作数 cn_var_条件_9;
  r35 = cn_var_上下文;
  r36 = cn_var_语句;
  r37 = r36->条件表达式;
  r38 = 生成表达式IR(r35, r37);
  cn_var_条件_9 = r38;
  struct IR指令* cn_var_条件跳转_10;
  r39 = cn_var_条件_9;
  r40 = cn_var_循环体块_4;
  r41 = 标签操作数(r40);
  r42 = cn_var_退出块_5;
  r43 = 标签操作数(r42);
  r44 = 创建IR指令(27, r39, r41, r43);
  cn_var_条件跳转_10 = r44;
  r45 = cn_var_上下文;
  r46 = cn_var_条件跳转_10;
  发射指令(r45, r46);
  r47 = cn_var_上下文;
  r48 = r47->当前函数;
  r49 = cn_var_循环体块_4;
  添加基本块(r48, r49);
  r50 = cn_var_上下文;
  r51 = cn_var_循环体块_4;
  切换基本块(r50, r51);
  r52 = cn_var_上下文;
  r53 = cn_var_语句;
  r54 = r53->循环体语句;
  生成语句IR(r52, r54);
  struct IR指令* cn_var_跳回条件_11;
  r55 = cn_var_条件块_3;
  r56 = 标签操作数(r55);
  r57 = 空操作数();
  r58 = 空操作数();
  r59 = 创建IR指令(26, r56, r57, r58);
  cn_var_跳回条件_11 = r59;
  r60 = cn_var_上下文;
  r61 = cn_var_跳回条件_11;
  发射指令(r60, r61);
  r62 = cn_var_上下文;
  r63 = r62->当前函数;
  r64 = cn_var_退出块_5;
  添加基本块(r63, r64);
  r65 = cn_var_上下文;
  r66 = cn_var_退出块_5;
  切换基本块(r65, r66);
  r67 = cn_var_原退出_6;
  r68 = cn_var_原继续_7;

  logic_rhs_3046:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3047;

  logic_merge_3047:
  if (r4) goto if_then_3044; else goto if_merge_3045;
  return;
}

void 生成循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r32, r35, r49, r52, r76, r90, r93;
  char* r5;
  char* r6;
  char* r7;
  char* r8;
  char* r9;
  char* r10;
  char* r12;
  char* r14;
  char* r16;
  char* r18;
  char* r20;
  char* r21;
  char* r22;
  char* r23;
  char* r24;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 基本块* r11;
  struct 基本块* r13;
  struct 基本块* r15;
  struct 基本块* r17;
  struct 基本块* r19;
  struct IR生成上下文* r25;
  struct 基本块* r26;
  struct IR生成上下文* r27;
  struct 基本块* r28;
  struct 基本块* r29;
  struct 基本块* r30;
  struct 语句节点* r31;
  struct IR生成上下文* r33;
  struct 语句节点* r34;
  struct 基本块* r36;
  struct IR指令* r40;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR生成上下文* r43;
  struct IR函数* r44;
  struct 基本块* r45;
  struct IR生成上下文* r46;
  struct 基本块* r47;
  struct 语句节点* r48;
  struct IR生成上下文* r50;
  struct 语句节点* r51;
  struct 基本块* r55;
  struct 基本块* r57;
  struct IR指令* r59;
  struct IR生成上下文* r60;
  struct IR指令* r61;
  struct 基本块* r62;
  struct IR指令* r66;
  struct IR生成上下文* r67;
  struct IR指令* r68;
  struct IR生成上下文* r69;
  struct IR函数* r70;
  struct 基本块* r71;
  struct IR生成上下文* r72;
  struct 基本块* r73;
  struct IR生成上下文* r74;
  struct 语句节点* r75;
  struct 基本块* r77;
  struct IR指令* r81;
  struct IR生成上下文* r82;
  struct IR指令* r83;
  struct IR生成上下文* r84;
  struct IR函数* r85;
  struct 基本块* r86;
  struct IR生成上下文* r87;
  struct 基本块* r88;
  struct 语句节点* r89;
  struct IR生成上下文* r91;
  struct 语句节点* r92;
  struct 基本块* r95;
  struct IR指令* r99;
  struct IR生成上下文* r100;
  struct IR指令* r101;
  struct IR生成上下文* r102;
  struct IR函数* r103;
  struct 基本块* r104;
  struct IR生成上下文* r105;
  struct 基本块* r106;
  struct 基本块* r107;
  struct 基本块* r108;
  struct IR操作数 r37;
  struct IR操作数 r38;
  struct IR操作数 r39;
  struct IR操作数 r53;
  struct IR操作数 r54;
  struct IR操作数 r56;
  struct IR操作数 r58;
  struct IR操作数 r63;
  struct IR操作数 r64;
  struct IR操作数 r65;
  struct IR操作数 r78;
  struct IR操作数 r79;
  struct IR操作数 r80;
  struct IR操作数 r94;
  struct IR操作数 r96;
  struct IR操作数 r97;
  struct IR操作数 r98;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3051; else goto logic_rhs_3050;

  if_then_3048:
  return;
  goto if_merge_3049;

  if_merge_3049:
  char* cn_var_初始化块名_0;
  r5 = 生成块名称("for_init");
  cn_var_初始化块名_0 = r5;
  char* cn_var_条件块名_1;
  r6 = 生成块名称("for_cond");
  cn_var_条件块名_1 = r6;
  char* cn_var_循环体名_2;
  r7 = 生成块名称("for_body");
  cn_var_循环体名_2 = r7;
  char* cn_var_迭代块名_3;
  r8 = 生成块名称("for_iter");
  cn_var_迭代块名_3 = r8;
  char* cn_var_退出块名_4;
  r9 = 生成块名称("for_exit");
  cn_var_退出块名_4 = r9;
  struct 基本块* cn_var_初始化块_5;
  r10 = cn_var_初始化块名_0;
  r11 = 创建基本块(r10);
  cn_var_初始化块_5 = r11;
  struct 基本块* cn_var_条件块_6;
  r12 = cn_var_条件块名_1;
  r13 = 创建基本块(r12);
  cn_var_条件块_6 = r13;
  struct 基本块* cn_var_循环体块_7;
  r14 = cn_var_循环体名_2;
  r15 = 创建基本块(r14);
  cn_var_循环体块_7 = r15;
  struct 基本块* cn_var_迭代块_8;
  r16 = cn_var_迭代块名_3;
  r17 = 创建基本块(r16);
  cn_var_迭代块_8 = r17;
  struct 基本块* cn_var_退出块_9;
  r18 = cn_var_退出块名_4;
  r19 = 创建基本块(r18);
  cn_var_退出块_9 = r19;
  r20 = cn_var_初始化块名_0;
  释放内存(r20);
  r21 = cn_var_条件块名_1;
  释放内存(r21);
  r22 = cn_var_循环体名_2;
  释放内存(r22);
  r23 = cn_var_迭代块名_3;
  释放内存(r23);
  r24 = cn_var_退出块名_4;
  释放内存(r24);
  struct 基本块* cn_var_原退出_10;
  r25 = cn_var_上下文;
  r26 = r25->循环退出块;
  cn_var_原退出_10 = r26;
  struct 基本块* cn_var_原继续_11;
  r27 = cn_var_上下文;
  r28 = r27->循环继续块;
  cn_var_原继续_11 = r28;
  r29 = cn_var_退出块_9;
  r30 = cn_var_迭代块_8;
  r31 = cn_var_语句;
  r32 = r31->初始化语句;
  if (r32) goto if_then_3052; else goto if_merge_3053;

  logic_rhs_3050:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3051;

  logic_merge_3051:
  if (r4) goto if_then_3048; else goto if_merge_3049;

  if_then_3052:
  r33 = cn_var_上下文;
  r34 = cn_var_语句;
  r35 = r34->初始化语句;
  生成语句IR(r33, r35);
  goto if_merge_3053;

  if_merge_3053:
  struct IR指令* cn_var_跳转到条件_12;
  r36 = cn_var_条件块_6;
  r37 = 标签操作数(r36);
  r38 = 空操作数();
  r39 = 空操作数();
  r40 = 创建IR指令(26, r37, r38, r39);
  cn_var_跳转到条件_12 = r40;
  r41 = cn_var_上下文;
  r42 = cn_var_跳转到条件_12;
  发射指令(r41, r42);
  r43 = cn_var_上下文;
  r44 = r43->当前函数;
  r45 = cn_var_条件块_6;
  添加基本块(r44, r45);
  r46 = cn_var_上下文;
  r47 = cn_var_条件块_6;
  切换基本块(r46, r47);
  r48 = cn_var_语句;
  r49 = r48->条件表达式;
  if (r49) goto if_then_3054; else goto if_else_3055;

  if_then_3054:
  struct IR操作数 cn_var_条件_13;
  r50 = cn_var_上下文;
  r51 = cn_var_语句;
  r52 = r51->条件表达式;
  r53 = 生成表达式IR(r50, r52);
  cn_var_条件_13 = r53;
  struct IR指令* cn_var_条件跳转_14;
  r54 = cn_var_条件_13;
  r55 = cn_var_循环体块_7;
  r56 = 标签操作数(r55);
  r57 = cn_var_退出块_9;
  r58 = 标签操作数(r57);
  r59 = 创建IR指令(27, r54, r56, r58);
  cn_var_条件跳转_14 = r59;
  r60 = cn_var_上下文;
  r61 = cn_var_条件跳转_14;
  发射指令(r60, r61);
  goto if_merge_3056;

  if_else_3055:
  struct IR指令* cn_var_无条件跳转_15;
  r62 = cn_var_循环体块_7;
  r63 = 标签操作数(r62);
  r64 = 空操作数();
  r65 = 空操作数();
  r66 = 创建IR指令(26, r63, r64, r65);
  cn_var_无条件跳转_15 = r66;
  r67 = cn_var_上下文;
  r68 = cn_var_无条件跳转_15;
  发射指令(r67, r68);
  goto if_merge_3056;

  if_merge_3056:
  r69 = cn_var_上下文;
  r70 = r69->当前函数;
  r71 = cn_var_循环体块_7;
  添加基本块(r70, r71);
  r72 = cn_var_上下文;
  r73 = cn_var_循环体块_7;
  切换基本块(r72, r73);
  r74 = cn_var_上下文;
  r75 = cn_var_语句;
  r76 = r75->循环体语句;
  生成语句IR(r74, r76);
  struct IR指令* cn_var_跳转到迭代_16;
  r77 = cn_var_迭代块_8;
  r78 = 标签操作数(r77);
  r79 = 空操作数();
  r80 = 空操作数();
  r81 = 创建IR指令(26, r78, r79, r80);
  cn_var_跳转到迭代_16 = r81;
  r82 = cn_var_上下文;
  r83 = cn_var_跳转到迭代_16;
  发射指令(r82, r83);
  r84 = cn_var_上下文;
  r85 = r84->当前函数;
  r86 = cn_var_迭代块_8;
  添加基本块(r85, r86);
  r87 = cn_var_上下文;
  r88 = cn_var_迭代块_8;
  切换基本块(r87, r88);
  r89 = cn_var_语句;
  r90 = r89->迭代表达式;
  if (r90) goto if_then_3057; else goto if_merge_3058;

  if_then_3057:
  r91 = cn_var_上下文;
  r92 = cn_var_语句;
  r93 = r92->迭代表达式;
  r94 = 生成表达式IR(r91, r93);
  goto if_merge_3058;

  if_merge_3058:
  struct IR指令* cn_var_跳回条件_17;
  r95 = cn_var_条件块_6;
  r96 = 标签操作数(r95);
  r97 = 空操作数();
  r98 = 空操作数();
  r99 = 创建IR指令(26, r96, r97, r98);
  cn_var_跳回条件_17 = r99;
  r100 = cn_var_上下文;
  r101 = cn_var_跳回条件_17;
  发射指令(r100, r101);
  r102 = cn_var_上下文;
  r103 = r102->当前函数;
  r104 = cn_var_退出块_9;
  添加基本块(r103, r104);
  r105 = cn_var_上下文;
  r106 = cn_var_退出块_9;
  切换基本块(r105, r106);
  r107 = cn_var_原退出_10;
  r108 = cn_var_原继续_11;
  return;
}

void 生成返回语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r4, r7;
  struct IR生成上下文* r0;
  struct 语句节点* r3;
  struct IR生成上下文* r5;
  struct 语句节点* r6;
  struct IR指令* r12;
  struct IR生成上下文* r13;
  struct IR指令* r14;
  struct IR操作数 r2;
  struct IR操作数 r8;
  struct IR操作数 r9;
  struct IR操作数 r10;
  struct IR操作数 r11;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3059; else goto if_merge_3060;

  if_then_3059:
  return;
  goto if_merge_3060;

  if_merge_3060:
  struct IR操作数 cn_var_值_0;
  r2 = 空操作数();
  cn_var_值_0 = r2;
  r3 = cn_var_语句;
  r4 = r3->返回表达式;
  if (r4) goto if_then_3061; else goto if_merge_3062;

  if_then_3061:
  r5 = cn_var_上下文;
  r6 = cn_var_语句;
  r7 = r6->返回表达式;
  r8 = 生成表达式IR(r5, r7);
  cn_var_值_0 = r8;
  goto if_merge_3062;

  if_merge_3062:
  struct IR指令* cn_var_指令_1;
  r9 = cn_var_值_0;
  r10 = 空操作数();
  r11 = 空操作数();
  r12 = 创建IR指令(29, r9, r10, r11);
  cn_var_指令_1 = r12;
  r13 = cn_var_上下文;
  r14 = cn_var_指令_1;
  发射指令(r13, r14);
  return;
}

void 生成中断语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r5;
  struct IR生成上下文* r1;
  struct IR生成上下文* r3;
  struct 基本块* r4;
  struct IR生成上下文* r6;
  struct 基本块* r7;
  struct IR指令* r11;
  struct IR生成上下文* r12;
  struct IR指令* r13;
  struct IR操作数 r8;
  struct IR操作数 r9;
  struct IR操作数 r10;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3066; else goto logic_rhs_3065;

  if_then_3063:
  return;
  goto if_merge_3064;

  if_merge_3064:
  struct IR指令* cn_var_指令_0;
  r6 = cn_var_上下文;
  r7 = r6->循环退出块;
  r8 = 标签操作数(r7);
  r9 = 空操作数();
  r10 = 空操作数();
  r11 = 创建IR指令(26, r8, r9, r10);
  cn_var_指令_0 = r11;
  r12 = cn_var_上下文;
  r13 = cn_var_指令_0;
  发射指令(r12, r13);

  logic_rhs_3065:
  r3 = cn_var_上下文;
  r4 = r3->循环退出块;
  r5 = !r4;
  goto logic_merge_3066;

  logic_merge_3066:
  if (r5) goto if_then_3063; else goto if_merge_3064;
  return;
}

void 生成继续语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r5;
  struct IR生成上下文* r1;
  struct IR生成上下文* r3;
  struct 基本块* r4;
  struct IR生成上下文* r6;
  struct 基本块* r7;
  struct IR指令* r11;
  struct IR生成上下文* r12;
  struct IR指令* r13;
  struct IR操作数 r8;
  struct IR操作数 r9;
  struct IR操作数 r10;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3070; else goto logic_rhs_3069;

  if_then_3067:
  return;
  goto if_merge_3068;

  if_merge_3068:
  struct IR指令* cn_var_指令_0;
  r6 = cn_var_上下文;
  r7 = r6->循环继续块;
  r8 = 标签操作数(r7);
  r9 = 空操作数();
  r10 = 空操作数();
  r11 = 创建IR指令(26, r8, r9, r10);
  cn_var_指令_0 = r11;
  r12 = cn_var_上下文;
  r13 = cn_var_指令_0;
  发射指令(r12, r13);

  logic_rhs_3069:
  r3 = cn_var_上下文;
  r4 = r3->循环继续块;
  r5 = !r4;
  goto logic_merge_3070;

  logic_merge_3070:
  if (r5) goto if_then_3067; else goto if_merge_3068;
  return;
}

void 生成块语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r6, r11;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r5;
  struct 语句节点* r7;
  struct IR生成上下文* r8;
  struct 语句节点* r9;
  struct 语句节点* r10;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3074; else goto logic_rhs_3073;

  if_then_3071:
  return;
  goto if_merge_3072;

  if_merge_3072:
  struct 语句节点* cn_var_子语句_0;
  r5 = cn_var_语句;
  r6 = r5->块语句列表;
  cn_var_子语句_0 = r6;
  goto while_cond_3075;

  logic_rhs_3073:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3074;

  logic_merge_3074:
  if (r4) goto if_then_3071; else goto if_merge_3072;

  while_cond_3075:
  r7 = cn_var_子语句_0;
  if (r7) goto while_body_3076; else goto while_exit_3077;

  while_body_3076:
  r8 = cn_var_上下文;
  r9 = cn_var_子语句_0;
  生成语句IR(r8, r9);
  r10 = cn_var_子语句_0;
  r11 = r10->下一个;
  cn_var_子语句_0 = r11;
  goto while_cond_3075;

  while_exit_3077:
  return;
}

void 生成函数IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r6, r8, r12, r17, r27, r38;
  char* r15;
  char* r23;
  char* r25;
  char* r28;
  char* r29;
  char* r31;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct 声明节点* r5;
  struct 声明节点* r7;
  struct IR函数* r9;
  struct IR函数* r10;
  struct 声明节点* r11;
  struct IR函数* r19;
  struct IR生成上下文* r21;
  struct 基本块* r30;
  struct IR函数* r32;
  struct 基本块* r33;
  struct IR生成上下文* r34;
  struct 基本块* r35;
  struct IR生成上下文* r36;
  struct 声明节点* r37;
  struct IR生成上下文* r39;
  struct IR模块* r40;
  struct IR函数* r41;
  struct IR生成上下文* r42;
  struct 参数 r13;
  struct 参数 r14;
  struct 参数 r16;
  struct IR操作数 r18;
  struct IR操作数 r20;
  struct 参数 r22;
  struct 参数 r24;
  struct 参数 r26;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3081; else goto logic_rhs_3080;

  if_then_3078:
  return;
  goto if_merge_3079;

  if_merge_3079:
  重置局部变量计数器();
  struct IR函数* cn_var_当前IR函数_0;
  r5 = cn_var_声明;
  r6 = r5->函数名称;
  r7 = cn_var_声明;
  r8 = r7->返回类型;
  r9 = 创建IR函数(r6, r8);
  cn_var_当前IR函数_0 = r9;
  r10 = cn_var_当前IR函数_0;
  struct 参数节点* cn_var_参数_1;
  r11 = cn_var_声明;
  r12 = r11->参数列表;
  cn_var_参数_1 = r12;
  goto while_cond_3082;

  logic_rhs_3080:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3081;

  logic_merge_3081:
  if (r4) goto if_then_3078; else goto if_merge_3079;

  while_cond_3082:
  r13 = cn_var_参数_1;
  if (r13) goto while_body_3083; else goto while_exit_3084;

  while_body_3083:
  struct IR操作数 cn_var_参数操作数_2;
  r14 = cn_var_参数_1;
  r15 = r14.名称;
  r16 = cn_var_参数_1;
  r17 = r16.类型信息;
  r18 = 全局符号操作数(r15, r17);
  cn_var_参数操作数_2 = r18;
  r19 = cn_var_当前IR函数_0;
  r20 = cn_var_参数操作数_2;
  添加函数参数(r19, r20);
  r21 = cn_var_上下文;
  r22 = cn_var_参数_1;
  r23 = r22.名称;
  r24 = cn_var_参数_1;
  r25 = r24.名称;
  添加局部变量映射(r21, r23, r25);
  r26 = cn_var_参数_1;
  r27 = r26.下一个;
  cn_var_参数_1 = r27;
  goto while_cond_3082;

  while_exit_3084:
  char* cn_var_入口块名_3;
  r28 = 生成块名称("entry");
  cn_var_入口块名_3 = r28;
  struct 基本块* cn_var_入口块_4;
  r29 = cn_var_入口块名_3;
  r30 = 创建基本块(r29);
  cn_var_入口块_4 = r30;
  r31 = cn_var_入口块名_3;
  释放内存(r31);
  r32 = cn_var_当前IR函数_0;
  r33 = cn_var_入口块_4;
  添加基本块(r32, r33);
  r34 = cn_var_上下文;
  r35 = cn_var_入口块_4;
  切换基本块(r34, r35);
  r36 = cn_var_上下文;
  r37 = cn_var_声明;
  r38 = r37->函数体;
  生成语句IR(r36, r38);
  r39 = cn_var_上下文;
  r40 = r39->模块;
  r41 = cn_var_当前IR函数_0;
  添加函数(r40, r41);
  r42 = cn_var_上下文;
  释放局部变量映射表(r42);
  return;
}

void 生成程序IR(struct IR生成上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r0, r2, r4, r9, r19;
  struct IR生成上下文* r1;
  struct 程序节点* r3;
  struct 程序节点* r5;
  struct 声明节点列表* r6;
  struct 声明节点* r7;
  struct 声明节点* r8;
  struct IR生成上下文* r14;
  struct 声明节点* r15;
  struct IR生成上下文* r16;
  struct 声明节点* r17;
  struct 声明节点* r18;
  _Bool r11;
  _Bool r13;
  struct 函数声明 r10;
  struct 变量声明 r12;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3088; else goto logic_rhs_3087;

  if_then_3085:
  return;
  goto if_merge_3086;

  if_merge_3086:
  struct 声明节点* cn_var_声明_0;
  r5 = cn_var_程序;
  r6 = r5->声明列表;
  cn_var_声明_0 = r6;
  goto while_cond_3089;

  logic_rhs_3087:
  r3 = cn_var_程序;
  r4 = !r3;
  goto logic_merge_3088;

  logic_merge_3088:
  if (r4) goto if_then_3085; else goto if_merge_3086;

  while_cond_3089:
  r7 = cn_var_声明_0;
  if (r7) goto while_body_3090; else goto while_exit_3091;

  while_body_3090:
  r8 = cn_var_声明_0;
  r9 = r8->类型;
  r10 = cn_var_函数声明;
  r11 = r9 == r10;
  if (r11) goto case_body_3093; else goto switch_check_3096;

  while_exit_3091:

  switch_check_3096:
  r12 = cn_var_变量声明;
  r13 = r9 == r12;
  if (r13) goto case_body_3094; else goto case_default_3095;

  case_body_3093:
  r14 = cn_var_上下文;
  r15 = cn_var_声明_0;
  生成函数IR(r14, r15);
  goto switch_merge_3092;
  goto switch_merge_3092;

  case_body_3094:
  r16 = cn_var_上下文;
  r17 = cn_var_声明_0;
  生成全局变量IR(r16, r17);
  goto switch_merge_3092;
  goto switch_merge_3092;

  case_default_3095:
  goto switch_merge_3092;
  goto switch_merge_3092;

  switch_merge_3092:
  r18 = cn_var_声明_0;
  r19 = r18->下一个;
  cn_var_声明_0 = r19;
  goto while_cond_3089;
  return;
}

void 生成全局变量IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r7, r10, r13, r15, r18;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct 声明节点* r6;
  struct IR生成上下文* r8;
  struct 声明节点* r9;
  struct 声明节点* r12;
  struct 声明节点* r14;
  struct 声明节点* r17;
  struct 全局变量* r19;
  struct IR生成上下文* r20;
  struct IR模块* r21;
  struct 全局变量* r22;
  struct IR操作数 r5;
  struct IR操作数 r11;
  struct IR操作数 r16;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3100; else goto logic_rhs_3099;

  if_then_3097:
  return;
  goto if_merge_3098;

  if_merge_3098:
  struct IR操作数 cn_var_初始值_0;
  r5 = 空操作数();
  cn_var_初始值_0 = r5;
  r6 = cn_var_声明;
  r7 = r6->初始值表达式;
  if (r7) goto if_then_3101; else goto if_merge_3102;

  logic_rhs_3099:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3100;

  logic_merge_3100:
  if (r4) goto if_then_3097; else goto if_merge_3098;

  if_then_3101:
  r8 = cn_var_上下文;
  r9 = cn_var_声明;
  r10 = r9->初始值表达式;
  r11 = 生成表达式IR(r8, r10);
  cn_var_初始值_0 = r11;
  goto if_merge_3102;

  if_merge_3102:
  struct 全局变量* cn_var_全局变量信息_1;
  r12 = cn_var_声明;
  r13 = r12->变量名称;
  r14 = cn_var_声明;
  r15 = r14->类型信息;
  r16 = cn_var_初始值_0;
  r17 = cn_var_声明;
  r18 = r17->是常量;
  r19 = 创建全局变量(r13, r15, r16, r18);
  cn_var_全局变量信息_1 = r19;
  r20 = cn_var_上下文;
  r21 = r20->模块;
  r22 = cn_var_全局变量信息_1;
  添加全局变量(r21, r22);
  return;
}

