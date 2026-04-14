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
struct 表达式列表;
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
struct 类型信息* 创建未定义类型(const char*);
struct 类型信息* 创建接口类型(const char*, struct 符号*);
struct 类型信息* 创建类类型(const char*, struct 符号*);
struct 类型信息* 创建枚举类型(const char*, struct 符号*);
struct 类型信息* 创建结构体类型(const char*, struct 符号*);
struct 类型信息* 创建函数类型(struct 类型信息**, long long, struct 类型信息*);
struct 类型信息* 创建数组类型(struct 类型信息*, long long*, long long);
struct 类型信息* 创建指针类型(struct 类型信息*);
struct 类型信息* 创建字符串类型(void);
struct 类型信息* 创建布尔类型(void);
struct 类型信息* 创建小数类型(const char*, long long);
struct 类型信息* 创建整数类型(const char*, long long, _Bool);
struct 类型信息* 创建空类型(void);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, const char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, const char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, const char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, const char*, const char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, const char*);
struct 源位置 创建未知位置(void);
struct 源位置 创建源位置(const char*, long long, long long);
void 清空诊断集合(struct 诊断集合*);
void 打印所有诊断(struct 诊断集合*);
void 打印诊断信息(struct 诊断信息*);
char* 获取严重级别字符串(enum 诊断严重级别);
_Bool 应该继续(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
_Bool 有错误(struct 诊断集合*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, const char*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 释放诊断集合(struct 诊断集合*);
struct 诊断集合* 创建诊断集合(long long);
void* 数组获取(void*, long long);
long long 获取位置参数个数(void);
char* 获取位置参数(long long);
long long 选项存在(const char*);
char* 查找选项(const char*);
char* 获取程序名称(void);
char* 获取参数(long long);
long long 获取参数个数(void);
long long 求最小值(long long, long long);
long long 求最大值(long long, long long);
long long 获取绝对值(long long);
char* 读取行(void);
long long 字符串格式化(const char*, long long, const char*);
char* 字符串格式(const char*);
char* 复制字符串副本(const char*);
long long 类型大小(long long);
void* 分配内存数组(long long, long long);
struct 符号* 创建变量符号(const char*, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建函数符号(const char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建参数符号(const char*, struct 类型节点*, struct 源位置);
struct 符号* 创建结构体符号(const char*, struct 源位置);
struct 符号* 创建枚举符号(const char*, struct 源位置);
struct 符号* 创建枚举成员符号(const char*, long long, _Bool, struct 源位置);
struct 符号* 创建类符号(const char*, struct 源位置, _Bool);
struct 符号* 创建接口符号(const char*, struct 源位置);
struct 作用域* 创建作用域(enum 作用域类型, const char*, struct 作用域*);
void 销毁作用域(struct 作用域*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁符号表管理器(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, const char*, struct 符号*);
void 离开作用域(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
_Bool 在循环体内(struct 符号表管理器*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, const char*);
struct 符号* 查找符号(struct 符号表管理器*, const char*);
struct 符号* 查找全局符号(struct 符号表管理器*, const char*);
struct 符号* 在作用域查找符号(struct 作用域*, const char*);
struct 符号* 查找类成员(struct 符号*, const char*);
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
struct 表达式节点* 创建基类访问表达式(const char*);
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
struct 词元 创建词元(enum 词元类型枚举, const char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
struct 静态变量* 创建静态变量(const char*, struct 类型节点*, struct IR操作数);
void 添加函数(struct IR模块*, struct IR函数*);
void 添加全局变量(struct IR模块*, struct 全局变量*);
struct 全局变量* 创建全局变量(const char*, struct 类型节点*, struct IR操作数, _Bool);
struct IR操作数 标签操作数(struct 基本块*);
struct IR操作数 全局符号操作数(const char*, struct 类型节点*);
struct IR操作数 字符串常量操作数(const char*, struct 类型节点*);
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
struct 基本块* 创建基本块(const char*);
long long 分配寄存器(struct IR函数*);
void 添加静态变量(struct IR函数*, struct 静态变量*);
void 添加基本块(struct IR函数*, struct 基本块*);
void 添加局部变量(struct IR函数*, struct IR操作数);
void 添加函数参数(struct IR函数*, struct IR操作数);
void 释放IR函数(struct IR函数*);
struct IR函数* 创建IR函数(const char*, struct 类型节点*);
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
_Bool 追加字符串(struct 输出缓冲区*, const char*);
_Bool 追加字符(struct 输出缓冲区*, long long);
_Bool 追加整数(struct 输出缓冲区*, long long);
void 追加缩进(struct 输出缓冲区*, long long);
void 追加换行(struct 输出缓冲区*);
void 增加缩进(struct 输出缓冲区*);
void 减少缩进(struct 输出缓冲区*);
char* 类型到C类型(struct 类型信息*, struct 输出缓冲区*);
char* 生成类型声明(struct 类型信息*, const char*, struct 输出缓冲区*);
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
void 格式化整数(const char*, long long, long long);
void 追加小数(struct 输出缓冲区*, double);

struct 输出缓冲区* 创建输出缓冲区(long long cn_var_初始容量) {
  long long r0, r3, r5;
  char* r6;
  char* r7;
  void* r1;
  struct 输出缓冲区* r2;
  void* r4;
  struct 输出缓冲区* r8;

  entry:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r0 = sizeof(struct 输出缓冲区);
  r1 = 分配内存(r0);
  cn_var_缓冲区_0 = r1;
  r2 = cn_var_缓冲区_0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  r3 = cn_var_初始容量;
  r4 = 分配内存(r3);
  r5 = cn_var_初始容量;
  r6 = cn_var_缓冲区_0->数据;
  if (r6) goto if_then_2; else goto if_merge_3;

  if_merge_1:
  r8 = cn_var_缓冲区_0;
  return r8;

  if_then_2:
  r7 = cn_var_缓冲区_0->数据;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r7, 0, &_tmp_i0, 8); }
  goto if_merge_3;

  if_merge_3:
  goto if_merge_1;
  return NULL;
}

void 释放输出缓冲区(struct 输出缓冲区* cn_var_缓冲区) {
  char* r1;
  char* r2;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r3;

  entry:
  r0 = cn_var_缓冲区;
  if (r0) goto if_then_4; else goto if_merge_5;

  if_then_4:
  r1 = cn_var_缓冲区->数据;
  if (r1) goto if_then_6; else goto if_merge_7;

  if_merge_5:

  if_then_6:
  r2 = cn_var_缓冲区->数据;
  释放内存(r2);
  goto if_merge_7;

  if_merge_7:
  r3 = cn_var_缓冲区;
  释放内存(r3);
  goto if_merge_5;
  return;
}

_Bool 扩展缓冲区(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_新容量) {
  long long r0, r3, r4, r7, r11;
  char* r6;
  char* r9;
  char* r10;
  struct 输出缓冲区* r1;
  void* r8;
  _Bool r2;
  _Bool r5;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_11; else goto logic_rhs_10;

  if_then_8:
  return 0;
  goto if_merge_9;

  if_merge_9:
  char* cn_var_新数据_0;
  r6 = cn_var_缓冲区->数据;
  r7 = cn_var_新容量;
  r8 = 重新分配内存(r6, r7);
  cn_var_新数据_0 = r8;
  r9 = cn_var_新数据_0;
  if (r9) goto if_then_12; else goto if_merge_13;

  logic_rhs_10:
  r3 = cn_var_新容量;
  r4 = cn_var_缓冲区->容量;
  r5 = r3 <= r4;
  goto logic_merge_11;

  logic_merge_11:
  if (r5) goto if_then_8; else goto if_merge_9;

  if_then_12:
  r10 = cn_var_新数据_0;
  r11 = cn_var_新容量;
  return 1;
  goto if_merge_13;

  if_merge_13:
  return 0;
}

_Bool 追加字符串(struct 输出缓冲区* cn_var_缓冲区, const char* cn_var_字符串值) {
  long long r0, r6, r7, r8, r9, r10, r11, r12, r14, r15, r16, r17, r19, r20, r22, r26, r30, r32, r33, r34, r36;
  char* r3;
  char* r5;
  char* r25;
  char* r27;
  char* r28;
  char* r29;
  char* r35;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r21;
  void* r31;
  _Bool r2;
  _Bool r4;
  _Bool r13;
  _Bool r18;
  _Bool r23;
  _Bool r24;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_17; else goto logic_rhs_16;

  if_then_14:
  return 0;
  goto if_merge_15;

  if_merge_15:
  long long cn_var_字符串长度_0;
  r5 = cn_var_字符串值;
  r6 = 获取字符串长度(r5);
  cn_var_字符串长度_0 = r6;
  long long cn_var_需要容量_1;
  r7 = cn_var_缓冲区->长度;
  r8 = cn_var_字符串长度_0;
  r9 = r7 + r8;
  r10 = r9 + 1;
  cn_var_需要容量_1 = r10;
  r11 = cn_var_需要容量_1;
  r12 = cn_var_缓冲区->容量;
  r13 = r11 > r12;
  if (r13) goto if_then_18; else goto if_merge_19;

  logic_rhs_16:
  r3 = cn_var_字符串值;
  r4 = !r3;
  goto logic_merge_17;

  logic_merge_17:
  if (r4) goto if_then_14; else goto if_merge_15;

  if_then_18:
  long long cn_var_新容量_2;
  r14 = cn_var_缓冲区->容量;
  r15 = r14 << 1;
  cn_var_新容量_2 = r15;
  goto while_cond_20;

  if_merge_19:
  r25 = cn_var_缓冲区->数据;
  r26 = cn_var_缓冲区->长度;
  r27 = cn_rt_int_to_string(r26);
  r28 = cn_rt_string_concat(r25, r27);
  r29 = cn_var_字符串值;
  r30 = cn_var_字符串长度_0;
  r31 = 复制内存(r28, r29, r30);
  r32 = cn_var_缓冲区->长度;
  r33 = cn_var_字符串长度_0;
  r34 = r32 + r33;
  r35 = cn_var_缓冲区->数据;
  r36 = cn_var_缓冲区->长度;
    { long long _tmp_i1 = 0; cn_rt_array_set_element(r35, r36, &_tmp_i1, 8); }
  return 1;

  while_cond_20:
  r16 = cn_var_新容量_2;
  r17 = cn_var_需要容量_1;
  r18 = r16 < r17;
  if (r18) goto while_body_21; else goto while_exit_22;

  while_body_21:
  r19 = cn_var_新容量_2;
  r20 = r19 << 1;
  cn_var_新容量_2 = r20;
  goto while_cond_20;

  while_exit_22:
  r21 = cn_var_缓冲区;
  r22 = cn_var_新容量_2;
  r23 = 扩展缓冲区(r21, r22);
  r24 = !r23;
  if (r24) goto if_then_23; else goto if_merge_24;

  if_then_23:
  return 0;
  goto if_merge_24;

  if_merge_24:
  goto if_merge_19;
  return 0;
}

_Bool 追加字符(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_字符值) {
  long long r1;
  char* r2;
  char* r3;
  char* r5;
  char* r7;
  void* r0;
  struct 输出缓冲区* r4;
  _Bool r6;
  _Bool r8;

  entry:
  char* cn_var_字符串缓冲_0;
  r0 = 分配内存(2);
  cn_var_字符串缓冲_0 = (char*)0;
  r1 = cn_var_字符值;
  r2 = cn_var_字符串缓冲_0;
    { long long _tmp_r0 = r1; cn_rt_array_set_element(r2, 0, &_tmp_r0, 8); }
  r3 = cn_var_字符串缓冲_0;
    { long long _tmp_i2 = 0; cn_rt_array_set_element(r3, 1, &_tmp_i2, 8); }
  _Bool cn_var_结果_1;
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
  long long r2;
  char* r1;
  char* r4;
  char* r6;
  void* r0;
  struct 输出缓冲区* r3;
  _Bool r5;
  _Bool r7;

  entry:
  char* cn_var_字符串缓冲_0;
  r0 = 分配内存(32);
  cn_var_字符串缓冲_0 = (char*)0;
  r1 = cn_var_字符串缓冲_0;
  r2 = cn_var_数值;
  格式化整数(r1, 32, r2);
  _Bool cn_var_结果_1;
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
  long long r2, r3, r4, r5, r9, r10;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r7;
  _Bool r1;
  _Bool r6;
  _Bool r8;

  entry:
  r0 = cn_var_缓冲区;
  r1 = !r0;
  if (r1) goto if_then_25; else goto if_merge_26;

  if_then_25:
  return;
  goto if_merge_26;

  if_merge_26:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_27;

  for_cond_27:
  r2 = cn_var_i_0;
  r3 = cn_var_缓冲区->缩进级别;
  r4 = cn_var_缩进宽度;
  r5 = r3 * r4;
  r6 = r2 < r5;
  if (r6) goto for_body_28; else goto for_exit_30;

  for_body_28:
  r7 = cn_var_缓冲区;
  r8 = 追加字符(r7, 32);
  goto for_update_29;

  for_update_29:
  r9 = cn_var_i_0;
  r10 = r9 + 1;
  cn_var_i_0 = r10;
  goto for_cond_27;

  for_exit_30:
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
  long long r0, r2, r3, r5, r6;
  struct 输出缓冲区* r1;
  _Bool r4;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_33; else goto logic_merge_34;

  if_then_31:
  r5 = cn_var_缓冲区->缩进级别;
  r6 = r5 + 1;
  goto if_merge_32;

  if_merge_32:

  logic_rhs_33:
  r2 = cn_var_缓冲区->缩进级别;
  r3 = cn_var_最大缩进级别;
  r4 = r2 < r3;
  goto logic_merge_34;

  logic_merge_34:
  if (r4) goto if_then_31; else goto if_merge_32;
  return;
}

void 减少缩进(struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r4, r5;
  struct 输出缓冲区* r1;
  _Bool r3;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_37; else goto logic_merge_38;

  if_then_35:
  r4 = cn_var_缓冲区->缩进级别;
  r5 = r4 - 1;
  goto if_merge_36;

  if_merge_36:

  logic_rhs_37:
  r2 = cn_var_缓冲区->缩进级别;
  r3 = r2 > 0;
  goto logic_merge_38;

  logic_merge_38:
  if (r3) goto if_then_35; else goto if_merge_36;
  return;
}

char* 类型到C类型(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r30, r66, r67, r69, r74, r78, r79;
  char* r18;
  char* r21;
  char* r24;
  char* r34;
  char* r38;
  char* r40;
  char* r44;
  char* r46;
  char* r50;
  char* r52;
  char* r56;
  char* r60;
  char* r63;
  char* r77;
  char* r82;
  long long* r28;
  struct 类型信息** r73;
  struct 类型信息* r0;
  struct 类型信息* r16;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  struct 类型信息* r22;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  long long* r29;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r58;
  struct 类型信息* r61;
  struct 输出缓冲区* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r71;
  struct 类型信息** r75;
  struct 输出缓冲区* r76;
  struct 输出缓冲区* r80;
  _Bool r1;
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
  _Bool r20;
  _Bool r26;
  _Bool r31;
  _Bool r33;
  _Bool r36;
  _Bool r39;
  _Bool r42;
  _Bool r45;
  _Bool r48;
  _Bool r51;
  _Bool r54;
  _Bool r57;
  _Bool r59;
  _Bool r65;
  _Bool r68;
  _Bool r70;
  _Bool r72;
  _Bool r81;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_39; else goto if_merge_40;

  if_then_39:
  return "void";
  goto if_merge_40;

  if_merge_40:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_空;
  if (r3) goto case_body_42; else goto switch_check_56;

  switch_check_56:
  r4 = r2 == 类型种类_类型_整数;
  if (r4) goto case_body_43; else goto switch_check_57;

  switch_check_57:
  r5 = r2 == 类型种类_类型_小数;
  if (r5) goto case_body_44; else goto switch_check_58;

  switch_check_58:
  r6 = r2 == 类型种类_类型_布尔;
  if (r6) goto case_body_45; else goto switch_check_59;

  switch_check_59:
  r7 = r2 == 类型种类_类型_字符串;
  if (r7) goto case_body_46; else goto switch_check_60;

  switch_check_60:
  r8 = r2 == 类型种类_类型_字符;
  if (r8) goto case_body_47; else goto switch_check_61;

  switch_check_61:
  r9 = r2 == 类型种类_类型_指针;
  if (r9) goto case_body_48; else goto switch_check_62;

  switch_check_62:
  r10 = r2 == 类型种类_类型_数组;
  if (r10) goto case_body_49; else goto switch_check_63;

  switch_check_63:
  r11 = r2 == 类型种类_类型_结构体;
  if (r11) goto case_body_50; else goto switch_check_64;

  switch_check_64:
  r12 = r2 == 类型种类_类型_枚举;
  if (r12) goto case_body_51; else goto switch_check_65;

  switch_check_65:
  r13 = r2 == 类型种类_类型_类;
  if (r13) goto case_body_52; else goto switch_check_66;

  switch_check_66:
  r14 = r2 == 类型种类_类型_接口;
  if (r14) goto case_body_53; else goto switch_check_67;

  switch_check_67:
  r15 = r2 == 类型种类_类型_函数;
  if (r15) goto case_body_54; else goto case_default_55;

  case_body_42:
  return "void";
  goto switch_merge_41;

  case_body_43:
  return "int";
  goto switch_merge_41;

  case_body_44:
  return "double";
  goto switch_merge_41;

  case_body_45:
  return "_Bool";
  goto switch_merge_41;

  case_body_46:
  return "cn_string_t";
  goto switch_merge_41;

  case_body_47:
  return "char";
  goto switch_merge_41;

  case_body_48:
  r16 = cn_var_类型->指向类型;
  r17 = cn_var_缓冲区;
  r18 = 类型到C类型(r16, r17);
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, "*");
  r21 = cn_var_缓冲区->数据;
  return r21;
  goto switch_merge_41;

  case_body_49:
  r22 = cn_var_类型->元素类型;
  r23 = cn_var_缓冲区;
  r24 = 类型到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, "[");
  r27 = cn_var_缓冲区;
  r28 = cn_var_类型->维度大小;
  r29 = (long long*)cn_rt_array_get_element(r28, 0, 8);
  r30 = 0 + r29;
  r31 = 追加整数(r27, r30);
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "]");
  r34 = cn_var_缓冲区->数据;
  return r34;
  goto switch_merge_41;

  case_body_50:
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, "struct ");
  r37 = cn_var_缓冲区;
  r38 = cn_var_类型->名称;
  r39 = 追加字符串(r37, r38);
  r40 = cn_var_缓冲区->数据;
  return r40;
  goto switch_merge_41;

  case_body_51:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "enum ");
  r43 = cn_var_缓冲区;
  r44 = cn_var_类型->名称;
  r45 = 追加字符串(r43, r44);
  r46 = cn_var_缓冲区->数据;
  return r46;
  goto switch_merge_41;

  case_body_52:
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, "struct ");
  r49 = cn_var_缓冲区;
  r50 = cn_var_类型->名称;
  r51 = 追加字符串(r49, r50);
  r52 = cn_var_缓冲区->数据;
  return r52;
  goto switch_merge_41;

  case_body_53:
  r53 = cn_var_缓冲区;
  r54 = 追加字符串(r53, "struct ");
  r55 = cn_var_缓冲区;
  r56 = cn_var_类型->名称;
  r57 = 追加字符串(r55, r56);
  r58 = cn_var_缓冲区;
  r59 = 追加字符串(r58, "_vtable");
  r60 = cn_var_缓冲区->数据;
  return r60;
  goto switch_merge_41;

  case_body_54:
  r61 = cn_var_类型->返回类型;
  r62 = cn_var_缓冲区;
  r63 = 类型到C类型(r61, r62);
  r64 = cn_var_缓冲区;
  r65 = 追加字符串(r64, " (*)(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_68;

  for_cond_68:
  r66 = cn_var_i_0;
  r67 = cn_var_类型->参数个数;
  r68 = r66 < r67;
  if (r68) goto for_body_69; else goto for_exit_71;

  for_body_69:
  r69 = cn_var_i_0;
  r70 = r69 > 0;
  if (r70) goto if_then_72; else goto if_merge_73;

  for_update_70:
  r78 = cn_var_i_0;
  r79 = r78 + 1;
  cn_var_i_0 = r79;
  goto for_cond_68;

  for_exit_71:
  r80 = cn_var_缓冲区;
  r81 = 追加字符串(r80, ")");
  r82 = cn_var_缓冲区->数据;
  return r82;
  goto switch_merge_41;

  if_then_72:
  r71 = cn_var_缓冲区;
  r72 = 追加字符串(r71, ", ");
  goto if_merge_73;

  if_merge_73:
  r73 = cn_var_类型->参数类型列表;
  r74 = cn_var_i_0;
  r75 = (struct 类型信息*[] 参数类型列表; 
    整数 参数个数;           
    类型信息* 返回类型;      
    
    
    符号* 类型符号;          
    类型信息*[] 成员类型列表; 
    字符串[] 成员名称列表;    
    整数 成员个数;           
    
    
    布尔 是常量;             
    布尔 是有符号;           
    布尔 已定义;             
}





枚举 类型兼容性 {
    完全兼容,        
    隐式转换,        
    显式转换,        
    不兼容           
}





结构体 类型推断上下文 {
    符号表管理器* 符号表;    
    类型信息* 期望类型;      
    整数 错误计数;           
}





常量 整数 整数大小 = 4;
常量 整数 小数大小 = 8;
常量 整数 布尔大小 = 1;
常量 整数 字符串大小 = 8;    
常量 整数 指针大小 = 8;







函数 创建空类型() -> 类型信息* {
    变量 内存指针 = 分配清零内存(1, 类型大小(类型信息));
    变量 类型 = (类型信息*)内存指针;
    类型.种类 = 类型种类.类型_空;
    类型.名称 = "空类型";
    类型.大小 = 0;
    类型.对齐 = 1;
    类型.已定义 = 真;
    返回 类型;
}






函数 创建整数类型(字符串 名称, 整数 大小, 布尔 是有符号) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_整数;
    类型.名称 = 名称;
    类型.大小 = 大小;
    类型.对齐 = 大小;
    类型.是有符号 = 是有符号;
    类型.已定义 = 真;
    返回 类型;
}





函数 创建小数类型(字符串 名称, 整数 大小) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_小数;
    类型.名称 = 名称;
    类型.大小 = 大小;
    类型.对齐 = 大小;
    类型.已定义 = 真;
    返回 类型;
}



函数 创建布尔类型() -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_布尔;
    类型.名称 = "布尔";
    类型.大小 = 布尔大小;
    类型.对齐 = 1;
    类型.已定义 = 真;
    返回 类型;
}



函数 创建字符串类型() -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_字符串;
    类型.名称 = "字符串";
    类型.大小 = 字符串大小;
    类型.对齐 = 指针大小;
    类型.已定义 = 真;
    返回 类型;
}




函数 创建指针类型(类型信息* 指向类型) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_指针;
    类型.指向类型 = 指向类型;
    类型.指针层级 = 指向类型.指针层级 + 1;
    类型.大小 = 指针大小;
    类型.对齐 = 指针大小;
    类型.已定义 = 真;
    类型.名称 = 指向类型.名称 + "*";
    返回 类型;
}






函数 创建数组类型(类型信息* 元素类型, 整数* 维度大小, 整数 维度) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_数组;
    类型.元素类型 = 元素类型;
    类型.数组维度 = 维度;
    类型.维度大小 = 维度大小;
    
    
    变量 总数 = 1;
    循环 (整数 i = 0; i < 维度; i = i + 1) {
        总数 = 总数 * 维度大小[i];
    }
    类型.总元素数 = 总数;
    类型.大小 = 总数 * 元素类型.大小;
    类型.对齐 = 元素类型.对齐;
    类型.已定义 = 真;
    返回 类型;
}






函数 创建函数类型(类型信息** 参数类型列表, 整数 参数个数, 类型信息* 返回类型) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_函数;
    类型.参数类型列表 = 参数类型列表;
    类型.参数个数 = 参数个数;
    类型.返回类型 = 返回类型;
    类型.大小 = 指针大小;
    类型.对齐 = 指针大小;
    类型.已定义 = 真;
    返回 类型;
}





函数 创建结构体类型(字符串 名称, 符号* 类型符号) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_结构体;
    类型.名称 = 名称;
    类型.类型符号 = 类型符号;
    类型.已定义 = 真;
    返回 类型;
}





函数 创建枚举类型(字符串 名称, 符号* 类型符号) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_枚举;
    类型.名称 = 名称;
    类型.类型符号 = 类型符号;
    类型.大小 = 整数大小;
    类型.对齐 = 整数大小;
    类型.是有符号 = 真;
    类型.已定义 = 真;
    返回 类型;
}





函数 创建类类型(字符串 名称, 符号* 类型符号) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_类;
    类型.名称 = 名称;
    类型.类型符号 = 类型符号;
    类型.已定义 = 真;
    返回 类型;
}





函数 创建接口类型(字符串 名称, 符号* 类型符号) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_接口;
    类型.名称 = 名称;
    类型.类型符号 = 类型符号;
    类型.已定义 = 真;
    返回 类型;
}




函数 创建未定义类型(字符串 名称) -> 类型信息* {
    变量 类型 = (类型信息*)分配清零内存(1, 类型大小(类型信息));
    类型.种类 = 类型种类.类型_未定义;
    类型.名称 = 名称;
    类型.已定义 = 假;
    返回 类型;
}









函数 类型相同(类型信息* 类型1, 类型信息* 类型2) -> 布尔 {
    如果 (类型1 == 无 || 类型2 == 无) {
        返回 类型1 == 类型2;
    }
    
    如果 (类型1.种类 != 类型2.种类) {
        返回 假;
    }
    
    选择 (类型1.种类) {
        情况 类型种类.类型_空:
            返回 真;

        情况 类型种类.类型_整数:
        情况 类型种类.类型_小数:
        情况 类型种类.类型_布尔:
        情况 类型种类.类型_字符串:
            返回 比较字符串(类型1.名称, 类型2.名称) == 0;

        情况 类型种类.类型_指针:
            返回 类型相同(类型1.指向类型, 类型2.指向类型);

        情况 类型种类.类型_数组:
            如果 (类型1.数组维度 != 类型2.数组维度) {
                返回 假;
            }
            如果 (类型1.总元素数 != 类型2.总元素数) {
                返回 假;
            }
            返回 类型相同(类型1.元素类型, 类型2.元素类型);
            
        情况 类型种类.类型_结构体:
        情况 类型种类.类型_枚举:
        情况 类型种类.类型_类:
        情况 类型种类.类型_接口:
            返回 类型1.类型符号 == 类型2.类型符号;

        情况 类型种类.类型_函数:
            如果 (类型1.参数个数 != 类型2.参数个数) {
                返回 假;
            }
            如果 (类型相同(类型1.返回类型, 类型2.返回类型) == 假) {
                返回 假;
            }
            循环 (整数 i = 0; i < 类型1.参数个数; i = i + 1) {
                如果 (类型相同(类型1.参数类型列表[i], 类型2.参数类型列表[i]) == 假) {
                    返回 假;
                }
            }
            返回 真;
            
        默认:
            返回 假;
    }
}





函数 检查类型兼容性(类型信息* 源类型, 类型信息* 目标类型) -> 类型兼容性 {
    如果 (类型相同(源类型, 目标类型)) {
        返回 类型兼容性.完全兼容;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_指针 && 目标类型.种类 == 类型种类.类型_指针) {
        如果 (源类型.指向类型.种类 == 类型种类.类型_空) {
            返回 类型兼容性.隐式转换;
        }
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_整数 && 目标类型.种类 == 类型种类.类型_小数) {
        返回 类型兼容性.隐式转换;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_整数 && 目标类型.种类 == 类型种类.类型_整数) {
        返回 类型兼容性.隐式转换;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_枚举 && 目标类型.种类 == 类型种类.类型_整数) {
        返回 类型兼容性.隐式转换;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_整数 && 目标类型.种类 == 类型种类.类型_枚举) {
        返回 类型兼容性.显式转换;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_小数 && 目标类型.种类 == 类型种类.类型_整数) {
        返回 类型兼容性.显式转换;
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_类 && 目标类型.种类 == 类型种类.类型_类) {
        如果 (是派生类(源类型, 目标类型)) {
            返回 类型兼容性.隐式转换;
        }
    }
    
    
    如果 (源类型.种类 == 类型种类.类型_类 && 目标类型.种类 == 类型种类.类型_接口) {
        如果 (实现接口(源类型, 目标类型)) {
            返回 类型兼容性.隐式转换;
        }
    }
    
    返回 类型兼容性.不兼容;
}





函数 是派生类(类型信息* 子类, 类型信息* 父类) -> 布尔 {
    如果 (子类 == 无 || 父类 == 无) {
        返回 假;
    }
    
    如果 (子类.种类 != 类型种类.类型_类 || 父类.种类 != 类型种类.类型_类) {
        返回 假;
    }
    
    变量 当前 = 子类.类型符号;
    当 (当前 != 无) {
        如果 (当前 == 父类.类型符号) {
            返回 真;
        }
        当前 = 当前.父类符号;
    }
    
    返回 假;
}





函数 实现接口(类型信息* 类类型, 类型信息* 接口类型) -> 布尔 {
    如果 (类类型 == 无 || 接口类型 == 无) {
        返回 假;
    }
    
    如果 (类类型.种类 != 类型种类.类型_类 || 接口类型.种类 != 类型种类.类型_接口) {
        返回 假;
    }
    
    变量 类符号 = 类类型.类型符号;
    如果 (类符号 == 无) {
        返回 假;
    }
    
    循环 (整数 i = 0; i < 类符号.接口个数; i = i + 1) {
        如果 (类符号.实现接口[i] == 接口类型.类型符号) {
            返回 真;
        }
    }
    
    
    如果 (类符号.父类符号 != 无) {
        变量 父类类型 = 创建类类型(类符号.父类符号.名称, 类符号.父类符号);
        返回 实现接口(父类类型, 接口类型);
    }
    
    返回 假;
}








函数 是数值类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_整数 || 类型.种类 == 类型种类.类型_小数;
}




函数 是整数类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_整数 || 类型.种类 == 类型种类.类型_枚举;
}




函数 是指针类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_指针;
}




函数 是数组类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_数组;
}




函数 是结构体类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_结构体;
}




函数 是类类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_类;
}




函数 是函数类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_函数;
}




函数 是可调用类型(类型信息* 类型) -> 布尔 {
    如果 (类型 == 无) {
        返回 假;
    }
    返回 类型.种类 == 类型种类.类型_函数 ||
           类型.种类 == 类型种类.类型_指针 && 类型.指向类型.种类 == 类型种类.类型_函数;
}









函数 从类型节点推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    如果 (类型节点指针 == 无) {
        返回 无;
    }
    
    选择 (类型节点指针.类型) {
        情况 节点类型.基础类型:
            返回 从基础类型推断(类型节点指针, 上下文);
            
        情况 节点类型.指针类型:
            返回 从指针类型推断(类型节点指针, 上下文);
            
        情况 节点类型.数组类型:
            返回 从数组类型推断(类型节点指针, 上下文);
            
        情况 节点类型.结构体类型:
            返回 从结构体类型推断(类型节点指针, 上下文);
            
        情况 节点类型.枚举类型:
            返回 从枚举类型推断(类型节点指针, 上下文);
            
        情况 节点类型.类类型:
            返回 从类类型推断(类型节点指针, 上下文);
            
        情况 节点类型.接口类型:
            返回 从接口类型推断(类型节点指针, 上下文);
            
        情况 节点类型.函数类型:
            返回 从函数类型推断(类型节点指针, 上下文);
            
        情况 节点类型.标识符类型:
            
            返回 从基础类型推断(类型节点指针, 上下文);
            
        默认:
            返回 无;
    }
}





函数 从基础类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 名称 = 类型节点指针.名称;
    
    如果 (比较字符串(名称, "整数") == 0) {
        返回 创建整数类型("整数", 整数大小, 真);
    }
    如果 (比较字符串(名称, "小数") == 0) {
        返回 创建小数类型("小数", 小数大小);
    }
    如果 (比较字符串(名称, "布尔") == 0) {
        返回 创建布尔类型();
    }
    如果 (比较字符串(名称, "字符串") == 0) {
        返回 创建字符串类型();
    }
    如果 (比较字符串(名称, "空类型") == 0) {
        返回 创建空类型();
    }
    
    
    变量 符号指针 = 查找符号(上下文.符号表, 名称);
    如果 (符号指针 != 无) {
        如果 (符号指针.种类 == 符号类型.结构体符号) {
            返回 创建结构体类型(名称, 符号指针);
        }
        如果 (符号指针.种类 == 符号类型.枚举符号) {
            返回 创建枚举类型(名称, 符号指针);
        }
        如果 (符号指针.种类 == 符号类型.类符号) {
            返回 创建类类型(名称, 符号指针);
        }
        如果 (符号指针.种类 == 符号类型.接口符号) {
            返回 创建接口类型(名称, 符号指针);
        }
        
        如果 (符号指针.类型信息 != 无) {
            返回 从类型节点推断(符号指针.类型信息, 上下文);
        }
    }
    
    
    返回 创建未定义类型(名称);
}





函数 从指针类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 指向类型 = 从类型节点推断(类型节点指针.元素类型, 上下文);
    如果 (指向类型 == 无) {
        返回 无;
    }
    
    
    变量 结果 = 创建指针类型(指向类型);
    循环 (整数 i = 1; i < 类型节点指针.指针层级; i = i + 1) {
        结果 = 创建指针类型(结果);
    }
    
    返回 结果;
}





函数 从数组类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 元素类型 = 从类型节点推断(类型节点指针.元素类型, 上下文);
    如果 (元素类型 == 无) {
        返回 无;
    }
    
    变量 维度大小 = (整数*)分配内存(类型节点指针.数组维度 * 整数大小);
    维度大小[0] = 类型节点指针.数组大小;
    
    返回 创建数组类型(元素类型, 维度大小, 类型节点指针.数组维度);
}





函数 从结构体类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 符号指针 = 查找符号(上下文.符号表, 类型节点指针.名称);
    如果 (符号指针 == 无 || 符号指针.种类 != 符号类型.结构体符号) {
        返回 无;
    }
    返回 创建结构体类型(类型节点指针.名称, 符号指针);
}





函数 从枚举类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 符号指针 = 查找符号(上下文.符号表, 类型节点指针.名称);
    如果 (符号指针 == 无 || 符号指针.种类 != 符号类型.枚举符号) {
        返回 无;
    }
    返回 创建枚举类型(类型节点指针.名称, 符号指针);
}





函数 从类类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 符号指针 = 查找符号(上下文.符号表, 类型节点指针.名称);
    如果 (符号指针 == 无 || 符号指针.种类 != 符号类型.类符号) {
        返回 无;
    }
    返回 创建类类型(类型节点指针.名称, 符号指针);
}





函数 从接口类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    变量 符号指针 = 查找符号(上下文.符号表, 类型节点指针.名称);
    如果 (符号指针 == 无 || 符号指针.种类 != 符号类型.接口符号) {
        返回 无;
    }
    返回 创建接口类型(类型节点指针.名称, 符号指针);
}





函数 从函数类型推断(类型节点* 类型节点指针, 类型推断上下文* 上下文) -> 类型信息* {
    
    
    返回 无;
}








函数 类型信息转类型节点(类型信息* 类型信息指针) -> 类型节点* {
    如果 (类型信息指针 == 无) {
        返回 无;
    }
    
    
    如果 (是有效的类型种类(类型信息指针.种类) == 假) {
        
        变量 节点 = (类型节点*)分配清零内存(1, 类型大小(类型节点));
        节点.类型 = 节点类型.基础类型;
        节点.名称 = "未知";
        返回 节点;
    }
    
    变量 节点 = (类型节点*)分配清零内存(1, 类型大小(类型节点));
    
    选择 (类型信息指针.种类) {
        情况 类型种类.类型_空:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "空类型";
            中断;
            
        情况 类型种类.类型_整数:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_小数:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_布尔:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "布尔";
            中断;
            
        情况 类型种类.类型_字符串:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "字符串";
            中断;
            
        情况 类型种类.类型_字符:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "字符";
            中断;
            
        情况 类型种类.类型_指针:
            节点.类型 = 节点类型.指针类型;
            节点.指针层级 = 类型信息指针.指针层级;
            节点.元素类型 = 类型信息转类型节点(类型信息指针.指向类型);
            中断;
            
        情况 类型种类.类型_数组:
            节点.类型 = 节点类型.数组类型;
            节点.数组维度 = 类型信息指针.数组维度;
            节点.数组大小 = 类型信息指针.总元素数;
            节点.元素类型 = 类型信息转类型节点(类型信息指针.元素类型);
            中断;
            
        情况 类型种类.类型_结构体:
            节点.类型 = 节点类型.结构体类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_枚举:
            节点.类型 = 节点类型.枚举类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_类:
            节点.类型 = 节点类型.类类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_接口:
            节点.类型 = 节点类型.接口类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_函数:
            节点.类型 = 节点类型.函数类型;
            中断;
            
        情况 类型种类.类型_参数:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = 类型信息指针.名称;
            中断;
            
        情况 类型种类.类型_未定义:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "未定义";
            中断;
            
        默认:
            节点.类型 = 节点类型.基础类型;
            节点.名称 = "未知";
            中断;
    }
    
    节点.是常量 = 类型信息指针.是常量;
    返回 节点;
}








函数 是有效的类型种类(类型种类 种类) -> 布尔 {
    
    
    
    
    如果 (种类 < 类型种类.类型_空 || 种类 > 类型种类.类型_未定义) {
        返回 假;
    }
    返回 真;
}




函数 获取类型种类名称(类型种类 种类) -> 字符串 {
    选择 (种类) {
        情况 类型种类.类型_空:
            返回 "空类型";
        情况 类型种类.类型_整数:
            返回 "整数";
        情况 类型种类.类型_小数:
            返回 "小数";
        情况 类型种类.类型_布尔:
            返回 "布尔";
        情况 类型种类.类型_字符串:
            返回 "字符串";
        情况 类型种类.类型_字符:
            返回 "字符";
        情况 类型种类.类型_指针:
            返回 "指针";
        情况 类型种类.类型_数组:
            返回 "数组";
        情况 类型种类.类型_结构体:
            返回 "结构体";
        情况 类型种类.类型_枚举:
            返回 "枚举";
        情况 类型种类.类型_类:
            返回 "类";
        情况 类型种类.类型_接口:
            返回 "接口";
        情况 类型种类.类型_函数:
            返回 "函数";
        情况 类型种类.类型_参数:
            返回 "类型参数";
        情况 类型种类.类型_未定义:
            返回 "未定义";
        默认:
            返回 "未知";
    }
}
**)cn_rt_array_get_element(r73, r74, 8);
  r76 = cn_var_缓冲区;
  r77 = 类型到C类型(r75, r76);
  goto for_update_70;

  case_default_55:
  return "void";
  goto switch_merge_41;

  switch_merge_41:
  return NULL;
}

char* 生成类型声明(struct 类型信息* cn_var_类型, const char* cn_var_名称, struct 输出缓冲区* cn_var_缓冲区) {
  long long r19, r20, r26, r28, r32, r33;
  char* r4;
  char* r8;
  char* r10;
  char* r13;
  char* r17;
  char* r34;
  char* r37;
  char* r41;
  char* r43;
  long long* r25;
  struct 类型信息* r0;
  struct 类型信息* r2;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 类型信息* r11;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  long long* r27;
  struct 输出缓冲区* r30;
  struct 类型信息* r35;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r40;
  _Bool r1;
  _Bool r6;
  _Bool r9;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r23;
  _Bool r29;
  _Bool r31;
  _Bool r39;
  _Bool r42;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_74; else goto if_merge_75;

  if_then_74:
  return "void";
  goto if_merge_75;

  if_merge_75:
  if (类型种类_类型_指针) goto if_then_76; else goto if_merge_77;

  if_then_76:
  r2 = cn_var_类型->指向类型;
  r3 = cn_var_缓冲区;
  r4 = 类型到C类型(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "* ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区->数据;
  return r10;
  goto if_merge_77;

  if_merge_77:
  if (类型种类_类型_数组) goto if_then_78; else goto if_merge_79;

  if_then_78:
  r11 = cn_var_类型->元素类型;
  r12 = cn_var_缓冲区;
  r13 = 类型到C类型(r11, r12);
  r14 = cn_var_缓冲区;
  r15 = 追加字符串(r14, " ");
  r16 = cn_var_缓冲区;
  r17 = cn_var_名称;
  r18 = 追加字符串(r16, r17);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_80;

  if_merge_79:
  r35 = cn_var_类型;
  r36 = cn_var_缓冲区;
  r37 = 类型到C类型(r35, r36);
  r38 = cn_var_缓冲区;
  r39 = 追加字符串(r38, " ");
  r40 = cn_var_缓冲区;
  r41 = cn_var_名称;
  r42 = 追加字符串(r40, r41);
  r43 = cn_var_缓冲区->数据;
  return r43;

  for_cond_80:
  r19 = cn_var_i_0;
  r20 = cn_var_类型->数组维度;
  r21 = r19 < r20;
  if (r21) goto for_body_81; else goto for_exit_83;

  for_body_81:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, "[");
  r24 = cn_var_缓冲区;
  r25 = cn_var_类型->维度大小;
  r26 = cn_var_i_0;
  r27 = (long long*)cn_rt_array_get_element(r25, r26, 8);
  r28 = 0 + r27;
  r29 = 追加整数(r24, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, "]");
  goto for_update_82;

  for_update_82:
  r32 = cn_var_i_0;
  r33 = r32 + 1;
  cn_var_i_0 = r33;
  goto for_cond_80;

  for_exit_83:
  r34 = cn_var_缓冲区->数据;
  return r34;
  goto if_merge_79;
  return NULL;
}

char* 类型节点到C类型(struct 类型节点* cn_var_类型节点指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r12, r15, r18, r21, r24, r27, r45, r46;
  char* r11;
  char* r14;
  char* r17;
  char* r20;
  char* r23;
  char* r26;
  char* r30;
  char* r32;
  char* r35;
  char* r38;
  char* r41;
  char* r50;
  char* r54;
  char* r56;
  char* r60;
  char* r62;
  char* r66;
  char* r68;
  char* r72;
  char* r74;
  char* r77;
  char* r78;
  char* r80;
  char* r82;
  struct 类型节点* r0;
  struct 输出缓冲区* r29;
  struct 类型节点* r33;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r36;
  struct 类型节点* r39;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r42;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r48;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r59;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  struct 输出缓冲区* r75;
  struct 输出缓冲区* r79;
  _Bool r1;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;
  _Bool r37;
  _Bool r43;
  _Bool r47;
  _Bool r49;
  _Bool r52;
  _Bool r55;
  _Bool r58;
  _Bool r61;
  _Bool r64;
  _Bool r67;
  _Bool r70;
  _Bool r73;
  _Bool r76;
  _Bool r81;
  enum 节点类型 r2;

  entry:
  r0 = cn_var_类型节点指针;
  r1 = !r0;
  if (r1) goto if_then_84; else goto if_merge_85;

  if_then_84:
  return "void";
  goto if_merge_85;

  if_merge_85:
  r2 = cn_var_类型节点指针->类型;
  r3 = r2 == 节点类型_基础类型;
  if (r3) goto case_body_87; else goto switch_check_96;

  switch_check_96:
  r4 = r2 == 节点类型_指针类型;
  if (r4) goto case_body_88; else goto switch_check_97;

  switch_check_97:
  r5 = r2 == 节点类型_数组类型;
  if (r5) goto case_body_89; else goto switch_check_98;

  switch_check_98:
  r6 = r2 == 节点类型_结构体类型;
  if (r6) goto case_body_90; else goto switch_check_99;

  switch_check_99:
  r7 = r2 == 节点类型_枚举类型;
  if (r7) goto case_body_91; else goto switch_check_100;

  switch_check_100:
  r8 = r2 == 节点类型_类类型;
  if (r8) goto case_body_92; else goto switch_check_101;

  switch_check_101:
  r9 = r2 == 节点类型_接口类型;
  if (r9) goto case_body_93; else goto switch_check_102;

  switch_check_102:
  r10 = r2 == 节点类型_函数类型;
  if (r10) goto case_body_94; else goto case_default_95;

  case_body_87:
  r11 = cn_var_类型节点指针->名称;
  r12 = 比较字符串(r11, "整数");
  r13 = r12 == 0;
  if (r13) goto if_then_103; else goto if_merge_104;

  if_then_103:
  return "long long";
  goto if_merge_104;

  if_merge_104:
  r14 = cn_var_类型节点指针->名称;
  r15 = 比较字符串(r14, "小数");
  r16 = r15 == 0;
  if (r16) goto if_then_105; else goto if_merge_106;

  if_then_105:
  return "double";
  goto if_merge_106;

  if_merge_106:
  r17 = cn_var_类型节点指针->名称;
  r18 = 比较字符串(r17, "布尔");
  r19 = r18 == 0;
  if (r19) goto if_then_107; else goto if_merge_108;

  if_then_107:
  return "_Bool";
  goto if_merge_108;

  if_merge_108:
  r20 = cn_var_类型节点指针->名称;
  r21 = 比较字符串(r20, "字符串");
  r22 = r21 == 0;
  if (r22) goto if_then_109; else goto if_merge_110;

  if_then_109:
  return "cn_string_t";
  goto if_merge_110;

  if_merge_110:
  r23 = cn_var_类型节点指针->名称;
  r24 = 比较字符串(r23, "字符");
  r25 = r24 == 0;
  if (r25) goto if_then_111; else goto if_merge_112;

  if_then_111:
  return "char";
  goto if_merge_112;

  if_merge_112:
  r26 = cn_var_类型节点指针->名称;
  r27 = 比较字符串(r26, "空类型");
  r28 = r27 == 0;
  if (r28) goto if_then_113; else goto if_merge_114;

  if_then_113:
  return "void";
  goto if_merge_114;

  if_merge_114:
  r29 = cn_var_缓冲区;
  r30 = cn_var_类型节点指针->名称;
  r31 = 追加字符串(r29, r30);
  r32 = cn_var_缓冲区->数据;
  return r32;
  goto switch_merge_86;

  case_body_88:
  r33 = cn_var_类型节点指针->元素类型;
  r34 = cn_var_缓冲区;
  r35 = 类型节点到C类型(r33, r34);
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, "*");
  r38 = cn_var_缓冲区->数据;
  return r38;
  goto switch_merge_86;

  case_body_89:
  r39 = cn_var_类型节点指针->元素类型;
  r40 = cn_var_缓冲区;
  r41 = 类型节点到C类型(r39, r40);
  r42 = cn_var_缓冲区;
  r43 = 追加字符串(r42, "[");
  r44 = cn_var_缓冲区;
  r45 = cn_var_类型节点指针->数组大小;
  r46 = 0 + r45;
  r47 = 追加整数(r44, r46);
  r48 = cn_var_缓冲区;
  r49 = 追加字符串(r48, "]");
  r50 = cn_var_缓冲区->数据;
  return r50;
  goto switch_merge_86;

  case_body_90:
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, "struct ");
  r53 = cn_var_缓冲区;
  r54 = cn_var_类型节点指针->名称;
  r55 = 追加字符串(r53, r54);
  r56 = cn_var_缓冲区->数据;
  return r56;
  goto switch_merge_86;

  case_body_91:
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, "enum ");
  r59 = cn_var_缓冲区;
  r60 = cn_var_类型节点指针->名称;
  r61 = 追加字符串(r59, r60);
  r62 = cn_var_缓冲区->数据;
  return r62;
  goto switch_merge_86;

  case_body_92:
  r63 = cn_var_缓冲区;
  r64 = 追加字符串(r63, "struct ");
  r65 = cn_var_缓冲区;
  r66 = cn_var_类型节点指针->名称;
  r67 = 追加字符串(r65, r66);
  r68 = cn_var_缓冲区->数据;
  return r68;
  goto switch_merge_86;

  case_body_93:
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, "struct ");
  r71 = cn_var_缓冲区;
  r72 = cn_var_类型节点指针->名称;
  r73 = 追加字符串(r71, r72);
  r74 = cn_var_缓冲区->数据;
  return r74;
  goto switch_merge_86;

  case_body_94:
  r75 = cn_var_缓冲区;
  r76 = 追加字符串(r75, "void*");
  r77 = cn_var_缓冲区->数据;
  return r77;
  goto switch_merge_86;

  case_default_95:
  r78 = cn_var_类型节点指针->名称;
  if (r78) goto if_then_115; else goto if_else_116;

  if_then_115:
  r79 = cn_var_缓冲区;
  r80 = cn_var_类型节点指针->名称;
  r81 = 追加字符串(r79, r80);
  goto if_merge_117;

  if_else_116:
  return "void";
  goto if_merge_117;

  if_merge_117:
  r82 = cn_var_缓冲区->数据;
  return r82;
  goto switch_merge_86;

  switch_merge_86:
  return NULL;
}

char* 生成操作数代码(struct IR操作数 cn_var_操作数, struct 输出缓冲区* cn_var_缓冲区) {
  long long r10, r14, r23, r34;
  char* r12;
  char* r16;
  char* r19;
  char* r25;
  char* r27;
  char* r29;
  char* r36;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r32;
  struct 基本块* r33;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r8;
  _Bool r11;
  _Bool r15;
  _Bool r21;
  _Bool r24;
  _Bool r28;
  _Bool r31;
  _Bool r35;
  double r18;
  enum IR操作数种类 r0;

  entry:
  r0 = cn_var_操作数.种类;
  r1 = r0 == IR操作数种类_虚拟寄存器;
  if (r1) goto case_body_119; else goto switch_check_126;

  switch_check_126:
  r2 = r0 == IR操作数种类_整数常量;
  if (r2) goto case_body_120; else goto switch_check_127;

  switch_check_127:
  r3 = r0 == IR操作数种类_小数常量;
  if (r3) goto case_body_121; else goto switch_check_128;

  switch_check_128:
  r4 = r0 == IR操作数种类_字符串常量;
  if (r4) goto case_body_122; else goto switch_check_129;

  switch_check_129:
  r5 = r0 == IR操作数种类_全局符号;
  if (r5) goto case_body_123; else goto switch_check_130;

  switch_check_130:
  r6 = r0 == IR操作数种类_基本块标签;
  if (r6) goto case_body_124; else goto case_default_125;

  case_body_119:
  r7 = cn_var_缓冲区;
  r8 = 追加字符串(r7, "r");
  r9 = cn_var_缓冲区;
  r10 = cn_var_操作数.寄存器编号;
  r11 = 追加整数(r9, r10);
  r12 = cn_var_缓冲区->数据;
  return r12;
  goto switch_merge_118;

  case_body_120:
  r13 = cn_var_缓冲区;
  r14 = cn_var_操作数.整数值;
  r15 = 追加整数(r13, r14);
  r16 = cn_var_缓冲区->数据;
  return r16;
  goto switch_merge_118;

  case_body_121:
  r17 = cn_var_缓冲区;
  r18 = cn_var_操作数.小数值;
  追加小数(r17, r18);
  r19 = cn_var_缓冲区->数据;
  return r19;
  goto switch_merge_118;

  case_body_122:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, "_str_");
  r22 = cn_var_缓冲区;
  r23 = cn_var_操作数.寄存器编号;
  r24 = 追加整数(r22, r23);
  r25 = cn_var_缓冲区->数据;
  return r25;
  goto switch_merge_118;

  case_body_123:
  r26 = cn_var_缓冲区;
  r27 = cn_var_操作数.符号名;
  r28 = 追加字符串(r26, r27);
  r29 = cn_var_缓冲区->数据;
  return r29;
  goto switch_merge_118;

  case_body_124:
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, "L");
  r32 = cn_var_缓冲区;
  r33 = cn_var_操作数.目标块;
  r34 = r33->块编号;
  r35 = 追加整数(r32, r34);
  r36 = cn_var_缓冲区->数据;
  return r36;
  goto switch_merge_118;

  case_default_125:
  return "";
  goto switch_merge_118;

  switch_merge_118:
  return NULL;
}

void 生成算术指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r3;
  char* r8;
  char* r44;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  _Bool r5;
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
  _Bool r21;
  _Bool r23;
  _Bool r25;
  _Bool r27;
  _Bool r29;
  _Bool r31;
  _Bool r33;
  _Bool r35;
  _Bool r37;
  _Bool r39;
  _Bool r41;
  _Bool r46;
  struct IR操作数 r1;
  struct IR操作数 r6;
  struct IR操作数 r42;
  enum IR操作码 r9;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令->目标;
  r2 = cn_var_缓冲区;
  r3 = 生成操作数代码(r1, r2);
  r4 = cn_var_缓冲区;
  r5 = 追加字符串(r4, " = ");
  r6 = cn_var_指令->源操作数1;
  r7 = cn_var_缓冲区;
  r8 = 生成操作数代码(r6, r7);
  r9 = cn_var_指令->操作码;
  r10 = r9 == IR操作码_加法指令;
  if (r10) goto case_body_132; else goto switch_check_143;

  switch_check_143:
  r11 = r9 == IR操作码_减法指令;
  if (r11) goto case_body_133; else goto switch_check_144;

  switch_check_144:
  r12 = r9 == IR操作码_乘法指令;
  if (r12) goto case_body_134; else goto switch_check_145;

  switch_check_145:
  r13 = r9 == IR操作码_除法指令;
  if (r13) goto case_body_135; else goto switch_check_146;

  switch_check_146:
  r14 = r9 == IR操作码_取模指令;
  if (r14) goto case_body_136; else goto switch_check_147;

  switch_check_147:
  r15 = r9 == IR操作码_与指令;
  if (r15) goto case_body_137; else goto switch_check_148;

  switch_check_148:
  r16 = r9 == IR操作码_或指令;
  if (r16) goto case_body_138; else goto switch_check_149;

  switch_check_149:
  r17 = r9 == IR操作码_异或指令;
  if (r17) goto case_body_139; else goto switch_check_150;

  switch_check_150:
  r18 = r9 == IR操作码_左移指令;
  if (r18) goto case_body_140; else goto switch_check_151;

  switch_check_151:
  r19 = r9 == IR操作码_右移指令;
  if (r19) goto case_body_141; else goto case_default_142;

  case_body_132:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, " + ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_133:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, " - ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_134:
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, " * ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_135:
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " / ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_136:
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, " % ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_137:
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, " & ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_138:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, " | ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_139:
  r34 = cn_var_缓冲区;
  r35 = 追加字符串(r34, " ^ ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_140:
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, " << ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_body_141:
  r38 = cn_var_缓冲区;
  r39 = 追加字符串(r38, " >> ");
  goto switch_merge_131;
  goto switch_merge_131;

  case_default_142:
  r40 = cn_var_缓冲区;
  r41 = 追加字符串(r40, " ? ");
  goto switch_merge_131;

  switch_merge_131:
  r42 = cn_var_指令->源操作数2;
  r43 = cn_var_缓冲区;
  r44 = 生成操作数代码(r42, r43);
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, ";");
  r47 = cn_var_缓冲区;
  追加换行(r47);
  return;
}

void 生成一元指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r3;
  char* r20;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  _Bool r5;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r22;
  struct IR操作数 r1;
  struct IR操作数 r18;
  enum IR操作码 r6;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令->目标;
  r2 = cn_var_缓冲区;
  r3 = 生成操作数代码(r1, r2);
  r4 = cn_var_缓冲区;
  r5 = 追加字符串(r4, " = ");
  r6 = cn_var_指令->操作码;
  r7 = r6 == IR操作码_负号指令;
  if (r7) goto case_body_153; else goto switch_check_157;

  switch_check_157:
  r8 = r6 == IR操作码_取反指令;
  if (r8) goto case_body_154; else goto switch_check_158;

  switch_check_158:
  r9 = r6 == IR操作码_逻辑非指令;
  if (r9) goto case_body_155; else goto case_default_156;

  case_body_153:
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, "-");
  goto switch_merge_152;
  goto switch_merge_152;

  case_body_154:
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, "~");
  goto switch_merge_152;
  goto switch_merge_152;

  case_body_155:
  r14 = cn_var_缓冲区;
  r15 = 追加字符串(r14, "!");
  goto switch_merge_152;
  goto switch_merge_152;

  case_default_156:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, "");
  goto switch_merge_152;

  switch_merge_152:
  r18 = cn_var_指令->源操作数1;
  r19 = cn_var_缓冲区;
  r20 = 生成操作数代码(r18, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, ";");
  r23 = cn_var_缓冲区;
  追加换行(r23);
  return;
}

void 生成比较指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r3;
  char* r8;
  char* r32;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r18;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r35;
  _Bool r5;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  _Bool r21;
  _Bool r23;
  _Bool r25;
  _Bool r27;
  _Bool r29;
  _Bool r34;
  struct IR操作数 r1;
  struct IR操作数 r6;
  struct IR操作数 r30;
  enum IR操作码 r9;

  entry:
  r0 = cn_var_缓冲区;
  追加缩进(r0, 4);
  r1 = cn_var_指令->目标;
  r2 = cn_var_缓冲区;
  r3 = 生成操作数代码(r1, r2);
  r4 = cn_var_缓冲区;
  r5 = 追加字符串(r4, " = ");
  r6 = cn_var_指令->源操作数1;
  r7 = cn_var_缓冲区;
  r8 = 生成操作数代码(r6, r7);
  r9 = cn_var_指令->操作码;
  r10 = r9 == IR操作码_相等指令;
  if (r10) goto case_body_160; else goto switch_check_167;

  switch_check_167:
  r11 = r9 == IR操作码_不等指令;
  if (r11) goto case_body_161; else goto switch_check_168;

  switch_check_168:
  r12 = r9 == IR操作码_小于指令;
  if (r12) goto case_body_162; else goto switch_check_169;

  switch_check_169:
  r13 = r9 == IR操作码_小于等于指令;
  if (r13) goto case_body_163; else goto switch_check_170;

  switch_check_170:
  r14 = r9 == IR操作码_大于指令;
  if (r14) goto case_body_164; else goto switch_check_171;

  switch_check_171:
  r15 = r9 == IR操作码_大于等于指令;
  if (r15) goto case_body_165; else goto case_default_166;

  case_body_160:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, " == ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_body_161:
  r18 = cn_var_缓冲区;
  r19 = 追加字符串(r18, " != ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_body_162:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, " < ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_body_163:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, " <= ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_body_164:
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, " > ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_body_165:
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " >= ");
  goto switch_merge_159;
  goto switch_merge_159;

  case_default_166:
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, " ? ");
  goto switch_merge_159;

  switch_merge_159:
  r30 = cn_var_指令->源操作数2;
  r31 = cn_var_缓冲区;
  r32 = 生成操作数代码(r30, r31);
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, ";");
  r35 = cn_var_缓冲区;
  追加换行(r35);
  return;
}

void 生成内存指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r16, r17;
  char* r10;
  char* r25;
  char* r30;
  char* r39;
  char* r44;
  char* r51;
  char* r56;
  char* r63;
  char* r68;
  char* r75;
  char* r80;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 类型节点* r15;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r48;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r59;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r67;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r74;
  struct 输出缓冲区* r76;
  struct 输出缓冲区* r79;
  struct 输出缓冲区* r81;
  struct 输出缓冲区* r83;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r12;
  _Bool r18;
  _Bool r20;
  _Bool r27;
  _Bool r32;
  _Bool r36;
  _Bool r41;
  _Bool r46;
  _Bool r53;
  _Bool r58;
  _Bool r65;
  _Bool r70;
  _Bool r77;
  _Bool r82;
  struct IR操作数 r8;
  struct IR操作数 r14;
  struct IR操作数 r23;
  struct IR操作数 r28;
  struct IR操作数 r37;
  struct IR操作数 r42;
  struct IR操作数 r49;
  struct IR操作数 r54;
  struct IR操作数 r61;
  struct IR操作数 r66;
  struct IR操作数 r73;
  struct IR操作数 r78;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_指令->操作码;
  r1 = r0 == IR操作码_分配指令;
  if (r1) goto case_body_173; else goto switch_check_180;

  switch_check_180:
  r2 = r0 == IR操作码_加载指令;
  if (r2) goto case_body_174; else goto switch_check_181;

  switch_check_181:
  r3 = r0 == IR操作码_存储指令;
  if (r3) goto case_body_175; else goto switch_check_182;

  switch_check_182:
  r4 = r0 == IR操作码_移动指令;
  if (r4) goto case_body_176; else goto switch_check_183;

  switch_check_183:
  r5 = r0 == IR操作码_取地址指令;
  if (r5) goto case_body_177; else goto switch_check_184;

  switch_check_184:
  r6 = r0 == IR操作码_解引用指令;
  if (r6) goto case_body_178; else goto case_default_179;

  case_body_173:
  r7 = cn_var_缓冲区;
  追加缩进(r7, 4);
  r8 = cn_var_指令->目标;
  r9 = cn_var_缓冲区;
  r10 = 生成操作数代码(r8, r9);
  r11 = cn_var_缓冲区;
  r12 = 追加字符串(r11, " = alloca(");
  r13 = cn_var_缓冲区;
  r14 = cn_var_指令->目标;
  r15 = r14.类型信息;
  r16 = r15->大小;
  r17 = 0 + r16;
  r18 = 追加整数(r13, r17);
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, ");");
  r21 = cn_var_缓冲区;
  追加换行(r21);
  goto switch_merge_172;
  goto switch_merge_172;

  case_body_174:
  r22 = cn_var_缓冲区;
  追加缩进(r22, 4);
  r23 = cn_var_指令->目标;
  r24 = cn_var_缓冲区;
  r25 = 生成操作数代码(r23, r24);
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " = *");
  r28 = cn_var_指令->源操作数1;
  r29 = cn_var_缓冲区;
  r30 = 生成操作数代码(r28, r29);
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, ";");
  r33 = cn_var_缓冲区;
  追加换行(r33);
  goto switch_merge_172;
  goto switch_merge_172;

  case_body_175:
  r34 = cn_var_缓冲区;
  追加缩进(r34, 4);
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, "*");
  r37 = cn_var_指令->源操作数2;
  r38 = cn_var_缓冲区;
  r39 = 生成操作数代码(r37, r38);
  r40 = cn_var_缓冲区;
  r41 = 追加字符串(r40, " = ");
  r42 = cn_var_指令->源操作数1;
  r43 = cn_var_缓冲区;
  r44 = 生成操作数代码(r42, r43);
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, ";");
  r47 = cn_var_缓冲区;
  追加换行(r47);
  goto switch_merge_172;
  goto switch_merge_172;

  case_body_176:
  r48 = cn_var_缓冲区;
  追加缩进(r48, 4);
  r49 = cn_var_指令->目标;
  r50 = cn_var_缓冲区;
  r51 = 生成操作数代码(r49, r50);
  r52 = cn_var_缓冲区;
  r53 = 追加字符串(r52, " = ");
  r54 = cn_var_指令->源操作数1;
  r55 = cn_var_缓冲区;
  r56 = 生成操作数代码(r54, r55);
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, ";");
  r59 = cn_var_缓冲区;
  追加换行(r59);
  goto switch_merge_172;
  goto switch_merge_172;

  case_body_177:
  r60 = cn_var_缓冲区;
  追加缩进(r60, 4);
  r61 = cn_var_指令->目标;
  r62 = cn_var_缓冲区;
  r63 = 生成操作数代码(r61, r62);
  r64 = cn_var_缓冲区;
  r65 = 追加字符串(r64, " = &");
  r66 = cn_var_指令->源操作数1;
  r67 = cn_var_缓冲区;
  r68 = 生成操作数代码(r66, r67);
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, ";");
  r71 = cn_var_缓冲区;
  追加换行(r71);
  goto switch_merge_172;
  goto switch_merge_172;

  case_body_178:
  r72 = cn_var_缓冲区;
  追加缩进(r72, 4);
  r73 = cn_var_指令->目标;
  r74 = cn_var_缓冲区;
  r75 = 生成操作数代码(r73, r74);
  r76 = cn_var_缓冲区;
  r77 = 追加字符串(r76, " = *");
  r78 = cn_var_指令->源操作数1;
  r79 = cn_var_缓冲区;
  r80 = 生成操作数代码(r78, r79);
  r81 = cn_var_缓冲区;
  r82 = 追加字符串(r81, ";");
  r83 = cn_var_缓冲区;
  追加换行(r83);
  goto switch_merge_172;
  goto switch_merge_172;

  case_default_179:
  goto switch_merge_172;
  goto switch_merge_172;

  switch_merge_172:
  return;
}

void 生成控制流指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r11, r22, r38, r45, r64, r65, r67, r72, r76, r77;
  char* r32;
  char* r56;
  char* r61;
  char* r75;
  char* r91;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 基本块* r10;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  struct 基本块* r21;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r35;
  struct 基本块* r37;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r42;
  struct 基本块* r44;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r62;
  struct 输出缓冲区* r69;
  struct IR操作数* r71;
  struct IR操作数* r73;
  struct 输出缓冲区* r74;
  struct 输出缓冲区* r78;
  struct 输出缓冲区* r80;
  struct 输出缓冲区* r81;
  struct 输出缓冲区* r82;
  struct 输出缓冲区* r87;
  struct 输出缓冲区* r90;
  struct 输出缓冲区* r92;
  struct 输出缓冲区* r94;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r7;
  _Bool r12;
  _Bool r14;
  _Bool r18;
  _Bool r23;
  _Bool r25;
  _Bool r29;
  _Bool r34;
  _Bool r39;
  _Bool r41;
  _Bool r46;
  _Bool r48;
  _Bool r53;
  _Bool r58;
  _Bool r63;
  _Bool r66;
  _Bool r68;
  _Bool r70;
  _Bool r79;
  _Bool r83;
  _Bool r86;
  _Bool r88;
  _Bool r93;
  struct IR操作数 r9;
  struct IR操作数 r20;
  struct IR操作数 r30;
  struct IR操作数 r36;
  struct IR操作数 r43;
  struct IR操作数 r51;
  struct IR操作数 r54;
  struct IR操作数 r59;
  struct IR操作数 r84;
  struct IR操作数 r89;
  enum IR操作码 r0;
  enum IR操作数种类 r52;
  enum IR操作数种类 r85;

  entry:
  r0 = cn_var_指令->操作码;
  r1 = r0 == IR操作码_标签指令;
  if (r1) goto case_body_186; else goto switch_check_192;

  switch_check_192:
  r2 = r0 == IR操作码_跳转指令;
  if (r2) goto case_body_187; else goto switch_check_193;

  switch_check_193:
  r3 = r0 == IR操作码_条件跳转指令;
  if (r3) goto case_body_188; else goto switch_check_194;

  switch_check_194:
  r4 = r0 == IR操作码_调用指令;
  if (r4) goto case_body_189; else goto switch_check_195;

  switch_check_195:
  r5 = r0 == IR操作码_返回指令;
  if (r5) goto case_body_190; else goto case_default_191;

  case_body_186:
  r6 = cn_var_缓冲区;
  r7 = 追加字符串(r6, "L");
  r8 = cn_var_缓冲区;
  r9 = cn_var_指令->目标;
  r10 = r9.目标块;
  r11 = r10->块编号;
  r12 = 追加整数(r8, r11);
  r13 = cn_var_缓冲区;
  r14 = 追加字符串(r13, ":");
  r15 = cn_var_缓冲区;
  追加换行(r15);
  goto switch_merge_185;
  goto switch_merge_185;

  case_body_187:
  r16 = cn_var_缓冲区;
  追加缩进(r16, 4);
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, "goto L");
  r19 = cn_var_缓冲区;
  r20 = cn_var_指令->目标;
  r21 = r20.目标块;
  r22 = r21->块编号;
  r23 = 追加整数(r19, r22);
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, ";");
  r26 = cn_var_缓冲区;
  追加换行(r26);
  goto switch_merge_185;
  goto switch_merge_185;

  case_body_188:
  r27 = cn_var_缓冲区;
  追加缩进(r27, 4);
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, "if (");
  r30 = cn_var_指令->目标;
  r31 = cn_var_缓冲区;
  r32 = 生成操作数代码(r30, r31);
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, ") goto L");
  r35 = cn_var_缓冲区;
  r36 = cn_var_指令->源操作数1;
  r37 = r36.目标块;
  r38 = r37->块编号;
  r39 = 追加整数(r35, r38);
  r40 = cn_var_缓冲区;
  r41 = 追加字符串(r40, "; else goto L");
  r42 = cn_var_缓冲区;
  r43 = cn_var_指令->源操作数2;
  r44 = r43.目标块;
  r45 = r44->块编号;
  r46 = 追加整数(r42, r45);
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, ";");
  r49 = cn_var_缓冲区;
  追加换行(r49);
  goto switch_merge_185;
  goto switch_merge_185;

  case_body_189:
  r50 = cn_var_缓冲区;
  追加缩进(r50, 4);
  r51 = cn_var_指令->目标;
  r52 = r51.种类;
  r53 = r52 != IR操作数种类_无操作数;
  if (r53) goto if_then_196; else goto if_merge_197;

  if_then_196:
  r54 = cn_var_指令->目标;
  r55 = cn_var_缓冲区;
  r56 = 生成操作数代码(r54, r55);
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, " = ");
  goto if_merge_197;

  if_merge_197:
  r59 = cn_var_指令->源操作数1;
  r60 = cn_var_缓冲区;
  r61 = 生成操作数代码(r59, r60);
  r62 = cn_var_缓冲区;
  r63 = 追加字符串(r62, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_198;

  for_cond_198:
  r64 = cn_var_i_0;
  r65 = cn_var_指令->额外参数数量;
  r66 = r64 < r65;
  if (r66) goto for_body_199; else goto for_exit_201;

  for_body_199:
  r67 = cn_var_i_0;
  r68 = r67 > 0;
  if (r68) goto if_then_202; else goto if_merge_203;

  for_update_200:
  r76 = cn_var_i_0;
  r77 = r76 + 1;
  cn_var_i_0 = r77;
  goto for_cond_198;

  for_exit_201:
  r78 = cn_var_缓冲区;
  r79 = 追加字符串(r78, ");");
  r80 = cn_var_缓冲区;
  追加换行(r80);
  goto switch_merge_185;
  goto switch_merge_185;

  if_then_202:
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, ", ");
  goto if_merge_203;

  if_merge_203:
  r71 = cn_var_指令->额外参数;
  r72 = cn_var_i_0;
  r73 = (struct IR操作数 {
    IR操作数种类 种类;       
    类型节点* 类型信息;      
    
    
    整数 寄存器编号;         
    整数 整数值;             
    小数 小数值;             
    字符串 字符串值;         
    字符串 符号名;           
    基本块* 目标块;          
    表达式节点* 表达式;      
}






结构体 IR指令 {
    IR操作码 操作码;         
    IR操作数 目标;           
    IR操作数 源操作数1;      
    IR操作数 源操作数2;      
    
    
    IR操作数* 额外参数;      
    整数 额外参数数量;       
    
    
    IR指令* 下一条;          
    IR指令* 上一条;          
    
    
    源位置 指令位置;         
}







结构体 基本块节点 {
    基本块* 块;              
    基本块节点* 下一个;      
}






结构体 基本块 {
    字符串 名称;             
    IR指令* 首条指令;        
    IR指令* 末条指令;        
    
    
    基本块节点* 前驱列表;    
    基本块节点* 后继列表;    
    
    
    基本块* 下一个;          
    基本块* 上一个;          
    
    
    布尔 已访问;             
    整数 块编号;             
}






结构体 静态变量 {
    字符串 名称;             
    整数 名称长度;           
    类型节点* 类型信息;      
    IR操作数 初始值;         
    静态变量* 下一个;        
}






结构体 IR函数 {
    字符串 名称;             
    类型节点* 返回类型;      
    
    
    IR操作数* 参数列表;      
    整数 参数数量;           
    
    
    IR操作数* 局部变量;      
    整数 局部变量数量;       
    
    
    静态变量* 首个静态变量;  
    静态变量* 末个静态变量;  
    
    
    基本块* 首个块;          
    基本块* 末个块;          
    
    
    整数 下个寄存器编号;     
    
    
    布尔 是中断处理;         
    整数 中断向量号;         
    
    
    IR函数* 下一个;          
}






结构体 目标三元组 {
    字符串 架构;              
    字符串 厂商;              
    字符串 系统;              
    字符串 环境;              
}






枚举 编译模式 {
    宿主模式,        
    目标模式         
}






结构体 全局变量 {
    字符串 名称;             
    类型节点* 类型信息;      
    IR操作数 初始值;         
    布尔 是常量;             
    
    
    全局变量* 下一个;        
}






结构体 IR模块 {
    
    IR函数* 首个函数;        
    IR函数* 末个函数;        
    
    
    全局变量* 首个全局;      
    全局变量* 末个全局;      
    
    
    目标三元组 目标信息;     
    编译模式 编译模式值;     
}







函数 创建IR模块() -> IR模块* {
    IR模块* 模块 = 分配内存(类型大小(IR模块));
    如果 (模块) {
        模块.首个函数 = 无;
        模块.末个函数 = 无;
        模块.首个全局 = 无;
        模块.末个全局 = 无;
        设置内存((空类型*)&模块.目标信息, 0, 类型大小(目标三元组));
        模块.编译模式值 = 宿主模式;
    }
    返回 模块;
}



函数 释放IR模块(IR模块* 模块) -> 空类型 {
    如果 (!模块) {
        返回;
    }
    
    
    全局变量* 全局 = 模块.首个全局;
    当 (全局) {
        全局变量* 下个 = 全局.下一个;
        如果 (全局.名称) {
            释放内存(全局.名称);
        }
        释放内存(全局);
        全局 = 下个;
    }
    
    
    IR函数* 函数指针 = 模块.首个函数;
    当 (函数指针) {
        IR函数* 下个函数 = 函数指针.下一个;
        释放IR函数(函数指针);
        函数指针 = 下个函数;
    }
    
    释放内存(模块);
}









函数 创建IR函数(字符串 名称, 类型节点* 返回类型) -> IR函数* {
    IR函数* 函数指针 = 分配内存(类型大小(IR函数));
    如果 (函数指针) {
        函数指针.名称 = 复制字符串副本(名称);
        函数指针.返回类型 = 返回类型;
        函数指针.参数列表 = 无;
        函数指针.参数数量 = 0;
        函数指针.局部变量 = 无;
        函数指针.局部变量数量 = 0;
        函数指针.首个静态变量 = 无;
        函数指针.末个静态变量 = 无;
        函数指针.首个块 = 无;
        函数指针.末个块 = 无;
        函数指针.下个寄存器编号 = 0;
        函数指针.是中断处理 = 假;
        函数指针.中断向量号 = 0;
        函数指针.下一个 = 无;
    }
    返回 函数指针;
}



函数 释放IR函数(IR函数* 函数指针) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    
    
    静态变量* 当前静态 = 函数指针.首个静态变量;
    当 (当前静态) {
        静态变量* 下个 = 当前静态.下一个;
        如果 (当前静态.名称) {
            释放内存(当前静态.名称);
        }
        释放内存(当前静态);
        当前静态 = 下个;
    }
    
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        基本块* 下个块 = 块.下一个;
        释放基本块(块);
        块 = 下个块;
    }
    
    
    如果 (函数指针.参数列表) {
        释放内存(函数指针.参数列表);
    }
    如果 (函数指针.局部变量) {
        释放内存(函数指针.局部变量);
    }
    如果 (函数指针.名称) {
        释放内存(函数指针.名称);
    }
    释放内存(函数指针);
}




函数 添加函数参数(IR函数* 函数指针, IR操作数 参数) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.参数列表 = 重新分配内存(
        函数指针.参数列表,
        (函数指针.参数数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.参数列表[函数指针.参数数量] = 参数;
    函数指针.参数数量 = 函数指针.参数数量 + 1;
}




函数 添加局部变量(IR函数* 函数指针, IR操作数 局部变量值) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.局部变量 = 重新分配内存(
        函数指针.局部变量,
        (函数指针.局部变量数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.局部变量[函数指针.局部变量数量] = 局部变量值;
    函数指针.局部变量数量 = 函数指针.局部变量数量 + 1;
}




函数 添加基本块(IR函数* 函数指针, 基本块* 块) -> 空类型 {
    如果 (!函数指针 || !块) {
        返回;
    }
    如果 (!函数指针.首个块) {
        函数指针.首个块 = 块;
        函数指针.末个块 = 块;
    } 否则 {
        函数指针.末个块.下一个 = 块;
        块.上一个 = 函数指针.末个块;
        函数指针.末个块 = 块;
    }
}




函数 添加静态变量(IR函数* 函数指针, 静态变量* 静态变量指针) -> 空类型 {
    如果 (!函数指针 || !静态变量指针) {
        返回;
    }
    如果 (!函数指针.首个静态变量) {
        函数指针.首个静态变量 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    } 否则 {
        函数指针.末个静态变量.下一个 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    }
}




函数 分配寄存器(IR函数* 函数指针) -> 整数 {
    如果 (!函数指针) {
        返回 -1;
    }
    整数 编号 = 函数指针.下个寄存器编号;
    函数指针.下个寄存器编号 = 函数指针.下个寄存器编号 + 1;
    返回 编号;
}








函数 创建基本块(字符串 名称提示) -> 基本块* {
    基本块* 块 = 分配内存(类型大小(基本块));
    如果 (块) {
        块.名称 = 名称提示 ? 复制字符串副本(名称提示) : 无;
        块.首条指令 = 无;
        块.末条指令 = 无;
        块.前驱列表 = 无;
        块.后继列表 = 无;
        块.下一个 = 无;
        块.上一个 = 无;
        块.已访问 = 假;
        块.块编号 = 0;
    }
    返回 块;
}



函数 释放基本块(基本块* 块) -> 空类型 {
    如果 (!块) {
        返回;
    }
    
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        IR指令* 下条 = 指令.下一条;
        释放IR指令(指令);
        指令 = 下条;
    }
    
    
    释放基本块列表(块.前驱列表);
    释放基本块列表(块.后继列表);
    
    如果 (块.名称) {
        释放内存(块.名称);
    }
    释放内存(块);
}



函数 释放基本块列表(基本块节点* 列表) -> 空类型 {
    当 (列表) {
        基本块节点* 下个 = 列表.下一个;
        释放内存(列表);
        列表 = 下个;
    }
}




函数 添加指令(基本块* 块, IR指令* 指令) -> 空类型 {
    如果 (!块 || !指令) {
        返回;
    }
    如果 (!块.首条指令) {
        块.首条指令 = 指令;
        块.末条指令 = 指令;
    } 否则 {
        块.末条指令.下一条 = 指令;
        指令.上一条 = 块.末条指令;
        块.末条指令 = 指令;
    }
}




函数 连接基本块(基本块* 源块, 基本块* 目标块) -> 空类型 {
    如果 (!源块 || !目标块) {
        返回;
    }
    
    
    基本块节点* 后继节点 = 分配内存(类型大小(基本块节点));
    后继节点.块 = 目标块;
    后继节点.下一个 = 源块.后继列表;
    源块.后继列表 = 后继节点;
    
    
    基本块节点* 前驱节点 = 分配内存(类型大小(基本块节点));
    前驱节点.块 = 源块;
    前驱节点.下一个 = 目标块.前驱列表;
    目标块.前驱列表 = 前驱节点;
}











函数 创建IR指令(IR操作码 操作码, IR操作数 目标, IR操作数 源1, IR操作数 源2) -> IR指令* {
    IR指令* 指令 = 分配内存(类型大小(IR指令));
    如果 (指令) {
        指令.操作码 = 操作码;
        指令.目标 = 目标;
        指令.源操作数1 = 源1;
        指令.源操作数2 = 源2;
        指令.额外参数 = 无;
        指令.额外参数数量 = 0;
        指令.下一条 = 无;
        指令.上一条 = 无;
    }
    返回 指令;
}



函数 释放IR指令(IR指令* 指令) -> 空类型 {
    如果 (!指令) {
        返回;
    }
    如果 (指令.额外参数) {
        释放内存(指令.额外参数);
    }
    释放内存(指令);
}







函数 空操作数() -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 无操作数;
    操作数.类型信息 = 无;
    返回 操作数;
}





函数 寄存器操作数(整数 编号, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 虚拟寄存器;
    操作数.寄存器编号 = 编号;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 整数常量操作数(整数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 整数常量;
    操作数.整数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 小数常量操作数(小数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 小数常量;
    操作数.小数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 字符串常量操作数(字符串 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 字符串常量;
    操作数.字符串值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 全局符号操作数(字符串 名称, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 全局符号;
    操作数.符号名 = 名称;
    操作数.类型信息 = 类型;
    返回 操作数;
}




函数 标签操作数(基本块* 块) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 基本块标签;
    操作数.目标块 = 块;
    操作数.类型信息 = 无;
    返回 操作数;
}











函数 创建全局变量(字符串 名称, 类型节点* 类型, IR操作数 初始值, 布尔 是常量) -> 全局变量* {
    全局变量* 全局变量指针 = 分配内存(类型大小(全局变量));
    如果 (全局变量指针) {
        全局变量指针.名称 = 复制字符串副本(名称);
        全局变量指针.类型信息 = 类型;
        全局变量指针.初始值 = 初始值;
        全局变量指针.是常量 = 是常量;
        全局变量指针.下一个 = 无;
    }
    返回 全局变量指针;
}




函数 添加全局变量(IR模块* 模块, 全局变量* 全局变量指针) -> 空类型 {
    如果 (!模块 || !全局变量指针) {
        返回;
    }
    如果 (!模块.首个全局) {
        模块.首个全局 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    } 否则 {
        模块.末个全局.下一个 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    }
}




函数 添加函数(IR模块* 模块, IR函数* 函数指针) -> 空类型 {
    如果 (!模块 || !函数指针) {
        返回;
    }
    如果 (!模块.首个函数) {
        模块.首个函数 = 函数指针;
        模块.末个函数 = 函数指针;
    } 否则 {
        模块.末个函数.下一个 = 函数指针;
        模块.末个函数 = 函数指针;
    }
}










函数 创建静态变量(字符串 名称, 类型节点* 类型, IR操作数 初始值) -> 静态变量* {
    静态变量* 静态变量指针 = 分配内存(类型大小(静态变量));
    如果 (静态变量指针) {
        静态变量指针.名称 = 复制字符串副本(名称);
        静态变量指针.名称长度 = 获取字符串长度(名称);
        静态变量指针.类型信息 = 类型;
        静态变量指针.初始值 = 初始值;
        静态变量指针.下一个 = 无;
    }
    返回 静态变量指针;
}
*)cn_rt_array_get_element(r71, r72, 8);
  r74 = cn_var_缓冲区;
  r75 = 生成操作数代码(r73, r74);
  goto for_update_200;

  case_body_190:
  r81 = cn_var_缓冲区;
  追加缩进(r81, 4);
  r82 = cn_var_缓冲区;
  r83 = 追加字符串(r82, "return");
  r84 = cn_var_指令->目标;
  r85 = r84.种类;
  r86 = r85 != IR操作数种类_无操作数;
  if (r86) goto if_then_204; else goto if_merge_205;

  if_then_204:
  r87 = cn_var_缓冲区;
  r88 = 追加字符串(r87, " ");
  r89 = cn_var_指令->目标;
  r90 = cn_var_缓冲区;
  r91 = 生成操作数代码(r89, r90);
  goto if_merge_205;

  if_merge_205:
  r92 = cn_var_缓冲区;
  r93 = 追加字符串(r92, ";");
  r94 = cn_var_缓冲区;
  追加换行(r94);
  goto switch_merge_185;
  goto switch_merge_185;

  case_default_191:
  goto switch_merge_185;
  goto switch_merge_185;

  switch_merge_185:
  return;
}

void 生成结构体指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r7;
  char* r12;
  char* r17;
  char* r25;
  char* r30;
  char* r35;
  char* r45;
  char* r50;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r15;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r9;
  _Bool r14;
  _Bool r18;
  _Bool r20;
  _Bool r27;
  _Bool r32;
  _Bool r36;
  _Bool r38;
  _Bool r42;
  _Bool r47;
  _Bool r52;
  struct IR操作数 r5;
  struct IR操作数 r10;
  struct IR操作数 r16;
  struct IR操作数 r23;
  struct IR操作数 r28;
  struct IR操作数 r34;
  struct IR操作数 r43;
  struct IR操作数 r48;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_指令->操作码;
  r1 = r0 == IR操作码_成员指针指令;
  if (r1) goto case_body_207; else goto switch_check_211;

  switch_check_211:
  r2 = r0 == IR操作码_成员访问指令;
  if (r2) goto case_body_208; else goto switch_check_212;

  switch_check_212:
  r3 = r0 == IR操作码_结构体初始化指令;
  if (r3) goto case_body_209; else goto case_default_210;

  case_body_207:
  r4 = cn_var_缓冲区;
  追加缩进(r4, 4);
  r5 = cn_var_指令->目标;
  r6 = cn_var_缓冲区;
  r7 = 生成操作数代码(r5, r6);
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, " = &");
  r10 = cn_var_指令->源操作数1;
  r11 = cn_var_缓冲区;
  r12 = 生成操作数代码(r10, r11);
  r13 = cn_var_缓冲区;
  r14 = 追加字符串(r13, "->");
  r15 = cn_var_缓冲区;
  r16 = cn_var_指令->源操作数2;
  r17 = r16.符号名;
  r18 = 追加字符串(r15, r17);
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, ";");
  r21 = cn_var_缓冲区;
  追加换行(r21);
  goto switch_merge_206;
  goto switch_merge_206;

  case_body_208:
  r22 = cn_var_缓冲区;
  追加缩进(r22, 4);
  r23 = cn_var_指令->目标;
  r24 = cn_var_缓冲区;
  r25 = 生成操作数代码(r23, r24);
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " = ");
  r28 = cn_var_指令->源操作数1;
  r29 = cn_var_缓冲区;
  r30 = 生成操作数代码(r28, r29);
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, ".");
  r33 = cn_var_缓冲区;
  r34 = cn_var_指令->源操作数2;
  r35 = r34.符号名;
  r36 = 追加字符串(r33, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ";");
  r39 = cn_var_缓冲区;
  追加换行(r39);
  goto switch_merge_206;
  goto switch_merge_206;

  case_body_209:
  r40 = cn_var_缓冲区;
  追加缩进(r40, 4);
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "memset(&");
  r43 = cn_var_指令->目标;
  r44 = cn_var_缓冲区;
  r45 = 生成操作数代码(r43, r44);
  r46 = cn_var_缓冲区;
  r47 = 追加字符串(r46, ", 0, sizeof(");
  r48 = cn_var_指令->目标;
  r49 = cn_var_缓冲区;
  r50 = 生成操作数代码(r48, r49);
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, "));");
  r53 = cn_var_缓冲区;
  追加换行(r53);
  goto switch_merge_206;
  goto switch_merge_206;

  case_default_210:
  goto switch_merge_206;
  goto switch_merge_206;

  switch_merge_206:
  return;
}

void 生成指令代码(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  char* r53;
  char* r58;
  char* r63;
  char* r68;
  struct IR指令* r0;
  struct IR指令* r38;
  struct 输出缓冲区* r39;
  struct IR指令* r40;
  struct 输出缓冲区* r41;
  struct IR指令* r42;
  struct 输出缓冲区* r43;
  struct IR指令* r44;
  struct 输出缓冲区* r45;
  struct IR指令* r46;
  struct 输出缓冲区* r47;
  struct IR指令* r48;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r59;
  struct 输出缓冲区* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r67;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  _Bool r1;
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
  _Bool r55;
  _Bool r60;
  _Bool r65;
  _Bool r70;
  struct IR操作数 r51;
  struct IR操作数 r56;
  struct IR操作数 r61;
  struct IR操作数 r66;
  enum IR操作码 r2;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_213; else goto if_merge_214;

  if_then_213:
  return;
  goto if_merge_214;

  if_merge_214:
  r2 = cn_var_指令->操作码;
  r3 = r2 == IR操作码_加法指令;
  if (r3) goto case_body_216; else goto switch_check_252;

  switch_check_252:
  r4 = r2 == IR操作码_减法指令;
  if (r4) goto case_body_217; else goto switch_check_253;

  switch_check_253:
  r5 = r2 == IR操作码_乘法指令;
  if (r5) goto case_body_218; else goto switch_check_254;

  switch_check_254:
  r6 = r2 == IR操作码_除法指令;
  if (r6) goto case_body_219; else goto switch_check_255;

  switch_check_255:
  r7 = r2 == IR操作码_取模指令;
  if (r7) goto case_body_220; else goto switch_check_256;

  switch_check_256:
  r8 = r2 == IR操作码_与指令;
  if (r8) goto case_body_221; else goto switch_check_257;

  switch_check_257:
  r9 = r2 == IR操作码_或指令;
  if (r9) goto case_body_222; else goto switch_check_258;

  switch_check_258:
  r10 = r2 == IR操作码_异或指令;
  if (r10) goto case_body_223; else goto switch_check_259;

  switch_check_259:
  r11 = r2 == IR操作码_左移指令;
  if (r11) goto case_body_224; else goto switch_check_260;

  switch_check_260:
  r12 = r2 == IR操作码_右移指令;
  if (r12) goto case_body_225; else goto switch_check_261;

  switch_check_261:
  r13 = r2 == IR操作码_负号指令;
  if (r13) goto case_body_226; else goto switch_check_262;

  switch_check_262:
  r14 = r2 == IR操作码_取反指令;
  if (r14) goto case_body_227; else goto switch_check_263;

  switch_check_263:
  r15 = r2 == IR操作码_逻辑非指令;
  if (r15) goto case_body_228; else goto switch_check_264;

  switch_check_264:
  r16 = r2 == IR操作码_相等指令;
  if (r16) goto case_body_229; else goto switch_check_265;

  switch_check_265:
  r17 = r2 == IR操作码_不等指令;
  if (r17) goto case_body_230; else goto switch_check_266;

  switch_check_266:
  r18 = r2 == IR操作码_小于指令;
  if (r18) goto case_body_231; else goto switch_check_267;

  switch_check_267:
  r19 = r2 == IR操作码_小于等于指令;
  if (r19) goto case_body_232; else goto switch_check_268;

  switch_check_268:
  r20 = r2 == IR操作码_大于指令;
  if (r20) goto case_body_233; else goto switch_check_269;

  switch_check_269:
  r21 = r2 == IR操作码_大于等于指令;
  if (r21) goto case_body_234; else goto switch_check_270;

  switch_check_270:
  r22 = r2 == IR操作码_分配指令;
  if (r22) goto case_body_235; else goto switch_check_271;

  switch_check_271:
  r23 = r2 == IR操作码_加载指令;
  if (r23) goto case_body_236; else goto switch_check_272;

  switch_check_272:
  r24 = r2 == IR操作码_存储指令;
  if (r24) goto case_body_237; else goto switch_check_273;

  switch_check_273:
  r25 = r2 == IR操作码_移动指令;
  if (r25) goto case_body_238; else goto switch_check_274;

  switch_check_274:
  r26 = r2 == IR操作码_取地址指令;
  if (r26) goto case_body_239; else goto switch_check_275;

  switch_check_275:
  r27 = r2 == IR操作码_解引用指令;
  if (r27) goto case_body_240; else goto switch_check_276;

  switch_check_276:
  r28 = r2 == IR操作码_标签指令;
  if (r28) goto case_body_241; else goto switch_check_277;

  switch_check_277:
  r29 = r2 == IR操作码_跳转指令;
  if (r29) goto case_body_242; else goto switch_check_278;

  switch_check_278:
  r30 = r2 == IR操作码_条件跳转指令;
  if (r30) goto case_body_243; else goto switch_check_279;

  switch_check_279:
  r31 = r2 == IR操作码_调用指令;
  if (r31) goto case_body_244; else goto switch_check_280;

  switch_check_280:
  r32 = r2 == IR操作码_返回指令;
  if (r32) goto case_body_245; else goto switch_check_281;

  switch_check_281:
  r33 = r2 == IR操作码_成员指针指令;
  if (r33) goto case_body_246; else goto switch_check_282;

  switch_check_282:
  r34 = r2 == IR操作码_成员访问指令;
  if (r34) goto case_body_247; else goto switch_check_283;

  switch_check_283:
  r35 = r2 == IR操作码_结构体初始化指令;
  if (r35) goto case_body_248; else goto switch_check_284;

  switch_check_284:
  r36 = r2 == IR操作码_PHI指令;
  if (r36) goto case_body_249; else goto switch_check_285;

  switch_check_285:
  r37 = r2 == IR操作码_选择指令;
  if (r37) goto case_body_250; else goto case_default_251;

  case_body_216:
  goto switch_merge_215;

  case_body_217:
  goto switch_merge_215;

  case_body_218:
  goto switch_merge_215;

  case_body_219:
  goto switch_merge_215;

  case_body_220:
  goto switch_merge_215;

  case_body_221:
  goto switch_merge_215;

  case_body_222:
  goto switch_merge_215;

  case_body_223:
  goto switch_merge_215;

  case_body_224:
  goto switch_merge_215;

  case_body_225:
  r38 = cn_var_指令;
  r39 = cn_var_缓冲区;
  生成算术指令(r38, r39);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_226:
  goto switch_merge_215;

  case_body_227:
  goto switch_merge_215;

  case_body_228:
  r40 = cn_var_指令;
  r41 = cn_var_缓冲区;
  生成一元指令(r40, r41);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_229:
  goto switch_merge_215;

  case_body_230:
  goto switch_merge_215;

  case_body_231:
  goto switch_merge_215;

  case_body_232:
  goto switch_merge_215;

  case_body_233:
  goto switch_merge_215;

  case_body_234:
  r42 = cn_var_指令;
  r43 = cn_var_缓冲区;
  生成比较指令(r42, r43);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_235:
  goto switch_merge_215;

  case_body_236:
  goto switch_merge_215;

  case_body_237:
  goto switch_merge_215;

  case_body_238:
  goto switch_merge_215;

  case_body_239:
  goto switch_merge_215;

  case_body_240:
  r44 = cn_var_指令;
  r45 = cn_var_缓冲区;
  生成内存指令(r44, r45);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_241:
  goto switch_merge_215;

  case_body_242:
  goto switch_merge_215;

  case_body_243:
  goto switch_merge_215;

  case_body_244:
  goto switch_merge_215;

  case_body_245:
  r46 = cn_var_指令;
  r47 = cn_var_缓冲区;
  生成控制流指令(r46, r47);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_246:
  goto switch_merge_215;

  case_body_247:
  goto switch_merge_215;

  case_body_248:
  r48 = cn_var_指令;
  r49 = cn_var_缓冲区;
  生成结构体指令(r48, r49);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_249:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_250:
  r50 = cn_var_缓冲区;
  追加缩进(r50, 4);
  r51 = cn_var_指令->目标;
  r52 = cn_var_缓冲区;
  r53 = 生成操作数代码(r51, r52);
  r54 = cn_var_缓冲区;
  r55 = 追加字符串(r54, " = ");
  r56 = cn_var_指令->目标;
  r57 = cn_var_缓冲区;
  r58 = 生成操作数代码(r56, r57);
  r59 = cn_var_缓冲区;
  r60 = 追加字符串(r59, " ? ");
  r61 = cn_var_指令->源操作数1;
  r62 = cn_var_缓冲区;
  r63 = 生成操作数代码(r61, r62);
  r64 = cn_var_缓冲区;
  r65 = 追加字符串(r64, " : ");
  r66 = cn_var_指令->源操作数2;
  r67 = cn_var_缓冲区;
  r68 = 生成操作数代码(r66, r67);
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, ";");
  r71 = cn_var_缓冲区;
  追加换行(r71);
  goto switch_merge_215;
  goto switch_merge_215;

  case_default_251:
  goto switch_merge_215;
  goto switch_merge_215;

  switch_merge_215:
  return;
}

void 生成基本块代码(struct 基本块* cn_var_块, struct 输出缓冲区* cn_var_缓冲区) {
  long long r5;
  struct 基本块* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct IR指令* r10;
  struct IR指令* r11;
  struct IR指令* r12;
  struct 输出缓冲区* r13;
  struct IR指令* r14;
  _Bool r1;
  _Bool r3;
  _Bool r6;
  _Bool r8;

  entry:
  r0 = cn_var_块;
  r1 = !r0;
  if (r1) goto if_then_286; else goto if_merge_287;

  if_then_286:
  return;
  goto if_merge_287;

  if_merge_287:
  r2 = cn_var_缓冲区;
  r3 = 追加字符串(r2, "L");
  r4 = cn_var_缓冲区;
  r5 = cn_var_块->块编号;
  r6 = 追加整数(r4, r5);
  r7 = cn_var_缓冲区;
  r8 = 追加字符串(r7, ":");
  r9 = cn_var_缓冲区;
  追加换行(r9);
  struct IR指令* cn_var_指令_0;
  r10 = cn_var_块->首条指令;
  cn_var_指令_0 = r10;
  goto while_cond_288;

  while_cond_288:
  r11 = cn_var_指令_0;
  if (r11) goto while_body_289; else goto while_exit_290;

  while_body_289:
  r12 = cn_var_指令_0;
  r13 = cn_var_缓冲区;
  生成指令代码(r12, r13);
  r14 = cn_var_指令_0->下一条;
  cn_var_指令_0 = r14;
  goto while_cond_288;

  while_exit_290:
  return;
}

void 生成局部变量声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r2, r3, r6, r15, r20, r21;
  char* r11;
  struct IR函数* r0;
  struct IR操作数* r5;
  struct IR操作数* r7;
  struct 输出缓冲区* r8;
  struct 类型节点* r9;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  _Bool r1;
  _Bool r4;
  _Bool r13;
  _Bool r16;
  _Bool r18;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_291; else goto if_merge_292;

  if_then_291:
  return;
  goto if_merge_292;

  if_merge_292:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_293;

  for_cond_293:
  r2 = cn_var_i_0;
  r3 = cn_var_函数指针->局部变量数量;
  r4 = r2 < r3;
  if (r4) goto for_body_294; else goto for_exit_296;

  for_body_294:
  struct IR操作数 cn_var_局部变量_1;
  r5 = cn_var_函数指针->局部变量;
  r6 = cn_var_i_0;
  r7 = (struct IR操作数 {
    IR操作数种类 种类;       
    类型节点* 类型信息;      
    
    
    整数 寄存器编号;         
    整数 整数值;             
    小数 小数值;             
    字符串 字符串值;         
    字符串 符号名;           
    基本块* 目标块;          
    表达式节点* 表达式;      
}






结构体 IR指令 {
    IR操作码 操作码;         
    IR操作数 目标;           
    IR操作数 源操作数1;      
    IR操作数 源操作数2;      
    
    
    IR操作数* 额外参数;      
    整数 额外参数数量;       
    
    
    IR指令* 下一条;          
    IR指令* 上一条;          
    
    
    源位置 指令位置;         
}







结构体 基本块节点 {
    基本块* 块;              
    基本块节点* 下一个;      
}






结构体 基本块 {
    字符串 名称;             
    IR指令* 首条指令;        
    IR指令* 末条指令;        
    
    
    基本块节点* 前驱列表;    
    基本块节点* 后继列表;    
    
    
    基本块* 下一个;          
    基本块* 上一个;          
    
    
    布尔 已访问;             
    整数 块编号;             
}






结构体 静态变量 {
    字符串 名称;             
    整数 名称长度;           
    类型节点* 类型信息;      
    IR操作数 初始值;         
    静态变量* 下一个;        
}






结构体 IR函数 {
    字符串 名称;             
    类型节点* 返回类型;      
    
    
    IR操作数* 参数列表;      
    整数 参数数量;           
    
    
    IR操作数* 局部变量;      
    整数 局部变量数量;       
    
    
    静态变量* 首个静态变量;  
    静态变量* 末个静态变量;  
    
    
    基本块* 首个块;          
    基本块* 末个块;          
    
    
    整数 下个寄存器编号;     
    
    
    布尔 是中断处理;         
    整数 中断向量号;         
    
    
    IR函数* 下一个;          
}






结构体 目标三元组 {
    字符串 架构;              
    字符串 厂商;              
    字符串 系统;              
    字符串 环境;              
}






枚举 编译模式 {
    宿主模式,        
    目标模式         
}






结构体 全局变量 {
    字符串 名称;             
    类型节点* 类型信息;      
    IR操作数 初始值;         
    布尔 是常量;             
    
    
    全局变量* 下一个;        
}






结构体 IR模块 {
    
    IR函数* 首个函数;        
    IR函数* 末个函数;        
    
    
    全局变量* 首个全局;      
    全局变量* 末个全局;      
    
    
    目标三元组 目标信息;     
    编译模式 编译模式值;     
}







函数 创建IR模块() -> IR模块* {
    IR模块* 模块 = 分配内存(类型大小(IR模块));
    如果 (模块) {
        模块.首个函数 = 无;
        模块.末个函数 = 无;
        模块.首个全局 = 无;
        模块.末个全局 = 无;
        设置内存((空类型*)&模块.目标信息, 0, 类型大小(目标三元组));
        模块.编译模式值 = 宿主模式;
    }
    返回 模块;
}



函数 释放IR模块(IR模块* 模块) -> 空类型 {
    如果 (!模块) {
        返回;
    }
    
    
    全局变量* 全局 = 模块.首个全局;
    当 (全局) {
        全局变量* 下个 = 全局.下一个;
        如果 (全局.名称) {
            释放内存(全局.名称);
        }
        释放内存(全局);
        全局 = 下个;
    }
    
    
    IR函数* 函数指针 = 模块.首个函数;
    当 (函数指针) {
        IR函数* 下个函数 = 函数指针.下一个;
        释放IR函数(函数指针);
        函数指针 = 下个函数;
    }
    
    释放内存(模块);
}









函数 创建IR函数(字符串 名称, 类型节点* 返回类型) -> IR函数* {
    IR函数* 函数指针 = 分配内存(类型大小(IR函数));
    如果 (函数指针) {
        函数指针.名称 = 复制字符串副本(名称);
        函数指针.返回类型 = 返回类型;
        函数指针.参数列表 = 无;
        函数指针.参数数量 = 0;
        函数指针.局部变量 = 无;
        函数指针.局部变量数量 = 0;
        函数指针.首个静态变量 = 无;
        函数指针.末个静态变量 = 无;
        函数指针.首个块 = 无;
        函数指针.末个块 = 无;
        函数指针.下个寄存器编号 = 0;
        函数指针.是中断处理 = 假;
        函数指针.中断向量号 = 0;
        函数指针.下一个 = 无;
    }
    返回 函数指针;
}



函数 释放IR函数(IR函数* 函数指针) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    
    
    静态变量* 当前静态 = 函数指针.首个静态变量;
    当 (当前静态) {
        静态变量* 下个 = 当前静态.下一个;
        如果 (当前静态.名称) {
            释放内存(当前静态.名称);
        }
        释放内存(当前静态);
        当前静态 = 下个;
    }
    
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        基本块* 下个块 = 块.下一个;
        释放基本块(块);
        块 = 下个块;
    }
    
    
    如果 (函数指针.参数列表) {
        释放内存(函数指针.参数列表);
    }
    如果 (函数指针.局部变量) {
        释放内存(函数指针.局部变量);
    }
    如果 (函数指针.名称) {
        释放内存(函数指针.名称);
    }
    释放内存(函数指针);
}




函数 添加函数参数(IR函数* 函数指针, IR操作数 参数) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.参数列表 = 重新分配内存(
        函数指针.参数列表,
        (函数指针.参数数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.参数列表[函数指针.参数数量] = 参数;
    函数指针.参数数量 = 函数指针.参数数量 + 1;
}




函数 添加局部变量(IR函数* 函数指针, IR操作数 局部变量值) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.局部变量 = 重新分配内存(
        函数指针.局部变量,
        (函数指针.局部变量数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.局部变量[函数指针.局部变量数量] = 局部变量值;
    函数指针.局部变量数量 = 函数指针.局部变量数量 + 1;
}




函数 添加基本块(IR函数* 函数指针, 基本块* 块) -> 空类型 {
    如果 (!函数指针 || !块) {
        返回;
    }
    如果 (!函数指针.首个块) {
        函数指针.首个块 = 块;
        函数指针.末个块 = 块;
    } 否则 {
        函数指针.末个块.下一个 = 块;
        块.上一个 = 函数指针.末个块;
        函数指针.末个块 = 块;
    }
}




函数 添加静态变量(IR函数* 函数指针, 静态变量* 静态变量指针) -> 空类型 {
    如果 (!函数指针 || !静态变量指针) {
        返回;
    }
    如果 (!函数指针.首个静态变量) {
        函数指针.首个静态变量 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    } 否则 {
        函数指针.末个静态变量.下一个 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    }
}




函数 分配寄存器(IR函数* 函数指针) -> 整数 {
    如果 (!函数指针) {
        返回 -1;
    }
    整数 编号 = 函数指针.下个寄存器编号;
    函数指针.下个寄存器编号 = 函数指针.下个寄存器编号 + 1;
    返回 编号;
}








函数 创建基本块(字符串 名称提示) -> 基本块* {
    基本块* 块 = 分配内存(类型大小(基本块));
    如果 (块) {
        块.名称 = 名称提示 ? 复制字符串副本(名称提示) : 无;
        块.首条指令 = 无;
        块.末条指令 = 无;
        块.前驱列表 = 无;
        块.后继列表 = 无;
        块.下一个 = 无;
        块.上一个 = 无;
        块.已访问 = 假;
        块.块编号 = 0;
    }
    返回 块;
}



函数 释放基本块(基本块* 块) -> 空类型 {
    如果 (!块) {
        返回;
    }
    
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        IR指令* 下条 = 指令.下一条;
        释放IR指令(指令);
        指令 = 下条;
    }
    
    
    释放基本块列表(块.前驱列表);
    释放基本块列表(块.后继列表);
    
    如果 (块.名称) {
        释放内存(块.名称);
    }
    释放内存(块);
}



函数 释放基本块列表(基本块节点* 列表) -> 空类型 {
    当 (列表) {
        基本块节点* 下个 = 列表.下一个;
        释放内存(列表);
        列表 = 下个;
    }
}




函数 添加指令(基本块* 块, IR指令* 指令) -> 空类型 {
    如果 (!块 || !指令) {
        返回;
    }
    如果 (!块.首条指令) {
        块.首条指令 = 指令;
        块.末条指令 = 指令;
    } 否则 {
        块.末条指令.下一条 = 指令;
        指令.上一条 = 块.末条指令;
        块.末条指令 = 指令;
    }
}




函数 连接基本块(基本块* 源块, 基本块* 目标块) -> 空类型 {
    如果 (!源块 || !目标块) {
        返回;
    }
    
    
    基本块节点* 后继节点 = 分配内存(类型大小(基本块节点));
    后继节点.块 = 目标块;
    后继节点.下一个 = 源块.后继列表;
    源块.后继列表 = 后继节点;
    
    
    基本块节点* 前驱节点 = 分配内存(类型大小(基本块节点));
    前驱节点.块 = 源块;
    前驱节点.下一个 = 目标块.前驱列表;
    目标块.前驱列表 = 前驱节点;
}











函数 创建IR指令(IR操作码 操作码, IR操作数 目标, IR操作数 源1, IR操作数 源2) -> IR指令* {
    IR指令* 指令 = 分配内存(类型大小(IR指令));
    如果 (指令) {
        指令.操作码 = 操作码;
        指令.目标 = 目标;
        指令.源操作数1 = 源1;
        指令.源操作数2 = 源2;
        指令.额外参数 = 无;
        指令.额外参数数量 = 0;
        指令.下一条 = 无;
        指令.上一条 = 无;
    }
    返回 指令;
}



函数 释放IR指令(IR指令* 指令) -> 空类型 {
    如果 (!指令) {
        返回;
    }
    如果 (指令.额外参数) {
        释放内存(指令.额外参数);
    }
    释放内存(指令);
}







函数 空操作数() -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 无操作数;
    操作数.类型信息 = 无;
    返回 操作数;
}





函数 寄存器操作数(整数 编号, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 虚拟寄存器;
    操作数.寄存器编号 = 编号;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 整数常量操作数(整数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 整数常量;
    操作数.整数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 小数常量操作数(小数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 小数常量;
    操作数.小数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 字符串常量操作数(字符串 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 字符串常量;
    操作数.字符串值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 全局符号操作数(字符串 名称, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 全局符号;
    操作数.符号名 = 名称;
    操作数.类型信息 = 类型;
    返回 操作数;
}




函数 标签操作数(基本块* 块) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 基本块标签;
    操作数.目标块 = 块;
    操作数.类型信息 = 无;
    返回 操作数;
}











函数 创建全局变量(字符串 名称, 类型节点* 类型, IR操作数 初始值, 布尔 是常量) -> 全局变量* {
    全局变量* 全局变量指针 = 分配内存(类型大小(全局变量));
    如果 (全局变量指针) {
        全局变量指针.名称 = 复制字符串副本(名称);
        全局变量指针.类型信息 = 类型;
        全局变量指针.初始值 = 初始值;
        全局变量指针.是常量 = 是常量;
        全局变量指针.下一个 = 无;
    }
    返回 全局变量指针;
}




函数 添加全局变量(IR模块* 模块, 全局变量* 全局变量指针) -> 空类型 {
    如果 (!模块 || !全局变量指针) {
        返回;
    }
    如果 (!模块.首个全局) {
        模块.首个全局 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    } 否则 {
        模块.末个全局.下一个 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    }
}




函数 添加函数(IR模块* 模块, IR函数* 函数指针) -> 空类型 {
    如果 (!模块 || !函数指针) {
        返回;
    }
    如果 (!模块.首个函数) {
        模块.首个函数 = 函数指针;
        模块.末个函数 = 函数指针;
    } 否则 {
        模块.末个函数.下一个 = 函数指针;
        模块.末个函数 = 函数指针;
    }
}










函数 创建静态变量(字符串 名称, 类型节点* 类型, IR操作数 初始值) -> 静态变量* {
    静态变量* 静态变量指针 = 分配内存(类型大小(静态变量));
    如果 (静态变量指针) {
        静态变量指针.名称 = 复制字符串副本(名称);
        静态变量指针.名称长度 = 获取字符串长度(名称);
        静态变量指针.类型信息 = 类型;
        静态变量指针.初始值 = 初始值;
        静态变量指针.下一个 = 无;
    }
    返回 静态变量指针;
}
*)cn_rt_array_get_element(r5, r6, 8);
  cn_var_局部变量_1 = r7;
  r8 = cn_var_缓冲区;
  追加缩进(r8, 4);
  r9 = cn_var_局部变量_1.类型信息;
  r10 = cn_var_缓冲区;
  r11 = 类型节点到C类型(r9, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, " r");
  r14 = cn_var_缓冲区;
  r15 = cn_var_局部变量_1.寄存器编号;
  r16 = 追加整数(r14, r15);
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, ";");
  r19 = cn_var_缓冲区;
  追加换行(r19);
  goto for_update_295;

  for_update_295:
  r20 = cn_var_i_0;
  r21 = r20 + 1;
  cn_var_i_0 = r21;
  goto for_cond_293;

  for_exit_296:
  return;
}

void 生成函数代码(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r12, r13, r15, r20, r28, r30, r31;
  char* r4;
  char* r8;
  char* r24;
  struct IR函数* r0;
  struct 类型节点* r2;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r17;
  struct IR操作数* r19;
  struct IR操作数* r21;
  struct 类型节点* r22;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r36;
  struct IR函数* r37;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r39;
  struct 基本块* r40;
  struct 基本块* r41;
  struct 基本块* r42;
  struct 输出缓冲区* r43;
  struct 基本块* r44;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r48;
  _Bool r1;
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r26;
  _Bool r29;
  _Bool r33;
  _Bool r35;
  _Bool r46;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_297; else goto if_merge_298;

  if_then_297:
  return;
  goto if_merge_298;

  if_merge_298:
  r2 = cn_var_函数指针->返回类型;
  r3 = cn_var_缓冲区;
  r4 = 类型节点到C类型(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_函数指针->名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_299;

  for_cond_299:
  r12 = cn_var_i_0;
  r13 = cn_var_函数指针->参数数量;
  r14 = r12 < r13;
  if (r14) goto for_body_300; else goto for_exit_302;

  for_body_300:
  r15 = cn_var_i_0;
  r16 = r15 > 0;
  if (r16) goto if_then_303; else goto if_merge_304;

  for_update_301:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_299;

  for_exit_302:
  if (0) goto if_then_305; else goto if_merge_306;

  if_then_303:
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, ", ");
  goto if_merge_304;

  if_merge_304:
  struct IR操作数 cn_var_参数_1;
  r19 = cn_var_函数指针->参数列表;
  r20 = cn_var_i_0;
  r21 = (struct IR操作数 {
    IR操作数种类 种类;       
    类型节点* 类型信息;      
    
    
    整数 寄存器编号;         
    整数 整数值;             
    小数 小数值;             
    字符串 字符串值;         
    字符串 符号名;           
    基本块* 目标块;          
    表达式节点* 表达式;      
}






结构体 IR指令 {
    IR操作码 操作码;         
    IR操作数 目标;           
    IR操作数 源操作数1;      
    IR操作数 源操作数2;      
    
    
    IR操作数* 额外参数;      
    整数 额外参数数量;       
    
    
    IR指令* 下一条;          
    IR指令* 上一条;          
    
    
    源位置 指令位置;         
}







结构体 基本块节点 {
    基本块* 块;              
    基本块节点* 下一个;      
}






结构体 基本块 {
    字符串 名称;             
    IR指令* 首条指令;        
    IR指令* 末条指令;        
    
    
    基本块节点* 前驱列表;    
    基本块节点* 后继列表;    
    
    
    基本块* 下一个;          
    基本块* 上一个;          
    
    
    布尔 已访问;             
    整数 块编号;             
}






结构体 静态变量 {
    字符串 名称;             
    整数 名称长度;           
    类型节点* 类型信息;      
    IR操作数 初始值;         
    静态变量* 下一个;        
}






结构体 IR函数 {
    字符串 名称;             
    类型节点* 返回类型;      
    
    
    IR操作数* 参数列表;      
    整数 参数数量;           
    
    
    IR操作数* 局部变量;      
    整数 局部变量数量;       
    
    
    静态变量* 首个静态变量;  
    静态变量* 末个静态变量;  
    
    
    基本块* 首个块;          
    基本块* 末个块;          
    
    
    整数 下个寄存器编号;     
    
    
    布尔 是中断处理;         
    整数 中断向量号;         
    
    
    IR函数* 下一个;          
}






结构体 目标三元组 {
    字符串 架构;              
    字符串 厂商;              
    字符串 系统;              
    字符串 环境;              
}






枚举 编译模式 {
    宿主模式,        
    目标模式         
}






结构体 全局变量 {
    字符串 名称;             
    类型节点* 类型信息;      
    IR操作数 初始值;         
    布尔 是常量;             
    
    
    全局变量* 下一个;        
}






结构体 IR模块 {
    
    IR函数* 首个函数;        
    IR函数* 末个函数;        
    
    
    全局变量* 首个全局;      
    全局变量* 末个全局;      
    
    
    目标三元组 目标信息;     
    编译模式 编译模式值;     
}







函数 创建IR模块() -> IR模块* {
    IR模块* 模块 = 分配内存(类型大小(IR模块));
    如果 (模块) {
        模块.首个函数 = 无;
        模块.末个函数 = 无;
        模块.首个全局 = 无;
        模块.末个全局 = 无;
        设置内存((空类型*)&模块.目标信息, 0, 类型大小(目标三元组));
        模块.编译模式值 = 宿主模式;
    }
    返回 模块;
}



函数 释放IR模块(IR模块* 模块) -> 空类型 {
    如果 (!模块) {
        返回;
    }
    
    
    全局变量* 全局 = 模块.首个全局;
    当 (全局) {
        全局变量* 下个 = 全局.下一个;
        如果 (全局.名称) {
            释放内存(全局.名称);
        }
        释放内存(全局);
        全局 = 下个;
    }
    
    
    IR函数* 函数指针 = 模块.首个函数;
    当 (函数指针) {
        IR函数* 下个函数 = 函数指针.下一个;
        释放IR函数(函数指针);
        函数指针 = 下个函数;
    }
    
    释放内存(模块);
}









函数 创建IR函数(字符串 名称, 类型节点* 返回类型) -> IR函数* {
    IR函数* 函数指针 = 分配内存(类型大小(IR函数));
    如果 (函数指针) {
        函数指针.名称 = 复制字符串副本(名称);
        函数指针.返回类型 = 返回类型;
        函数指针.参数列表 = 无;
        函数指针.参数数量 = 0;
        函数指针.局部变量 = 无;
        函数指针.局部变量数量 = 0;
        函数指针.首个静态变量 = 无;
        函数指针.末个静态变量 = 无;
        函数指针.首个块 = 无;
        函数指针.末个块 = 无;
        函数指针.下个寄存器编号 = 0;
        函数指针.是中断处理 = 假;
        函数指针.中断向量号 = 0;
        函数指针.下一个 = 无;
    }
    返回 函数指针;
}



函数 释放IR函数(IR函数* 函数指针) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    
    
    静态变量* 当前静态 = 函数指针.首个静态变量;
    当 (当前静态) {
        静态变量* 下个 = 当前静态.下一个;
        如果 (当前静态.名称) {
            释放内存(当前静态.名称);
        }
        释放内存(当前静态);
        当前静态 = 下个;
    }
    
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        基本块* 下个块 = 块.下一个;
        释放基本块(块);
        块 = 下个块;
    }
    
    
    如果 (函数指针.参数列表) {
        释放内存(函数指针.参数列表);
    }
    如果 (函数指针.局部变量) {
        释放内存(函数指针.局部变量);
    }
    如果 (函数指针.名称) {
        释放内存(函数指针.名称);
    }
    释放内存(函数指针);
}




函数 添加函数参数(IR函数* 函数指针, IR操作数 参数) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.参数列表 = 重新分配内存(
        函数指针.参数列表,
        (函数指针.参数数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.参数列表[函数指针.参数数量] = 参数;
    函数指针.参数数量 = 函数指针.参数数量 + 1;
}




函数 添加局部变量(IR函数* 函数指针, IR操作数 局部变量值) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    函数指针.局部变量 = 重新分配内存(
        函数指针.局部变量,
        (函数指针.局部变量数量 + 1) * 类型大小(IR操作数)
    );
    函数指针.局部变量[函数指针.局部变量数量] = 局部变量值;
    函数指针.局部变量数量 = 函数指针.局部变量数量 + 1;
}




函数 添加基本块(IR函数* 函数指针, 基本块* 块) -> 空类型 {
    如果 (!函数指针 || !块) {
        返回;
    }
    如果 (!函数指针.首个块) {
        函数指针.首个块 = 块;
        函数指针.末个块 = 块;
    } 否则 {
        函数指针.末个块.下一个 = 块;
        块.上一个 = 函数指针.末个块;
        函数指针.末个块 = 块;
    }
}




函数 添加静态变量(IR函数* 函数指针, 静态变量* 静态变量指针) -> 空类型 {
    如果 (!函数指针 || !静态变量指针) {
        返回;
    }
    如果 (!函数指针.首个静态变量) {
        函数指针.首个静态变量 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    } 否则 {
        函数指针.末个静态变量.下一个 = 静态变量指针;
        函数指针.末个静态变量 = 静态变量指针;
    }
}




函数 分配寄存器(IR函数* 函数指针) -> 整数 {
    如果 (!函数指针) {
        返回 -1;
    }
    整数 编号 = 函数指针.下个寄存器编号;
    函数指针.下个寄存器编号 = 函数指针.下个寄存器编号 + 1;
    返回 编号;
}








函数 创建基本块(字符串 名称提示) -> 基本块* {
    基本块* 块 = 分配内存(类型大小(基本块));
    如果 (块) {
        块.名称 = 名称提示 ? 复制字符串副本(名称提示) : 无;
        块.首条指令 = 无;
        块.末条指令 = 无;
        块.前驱列表 = 无;
        块.后继列表 = 无;
        块.下一个 = 无;
        块.上一个 = 无;
        块.已访问 = 假;
        块.块编号 = 0;
    }
    返回 块;
}



函数 释放基本块(基本块* 块) -> 空类型 {
    如果 (!块) {
        返回;
    }
    
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        IR指令* 下条 = 指令.下一条;
        释放IR指令(指令);
        指令 = 下条;
    }
    
    
    释放基本块列表(块.前驱列表);
    释放基本块列表(块.后继列表);
    
    如果 (块.名称) {
        释放内存(块.名称);
    }
    释放内存(块);
}



函数 释放基本块列表(基本块节点* 列表) -> 空类型 {
    当 (列表) {
        基本块节点* 下个 = 列表.下一个;
        释放内存(列表);
        列表 = 下个;
    }
}




函数 添加指令(基本块* 块, IR指令* 指令) -> 空类型 {
    如果 (!块 || !指令) {
        返回;
    }
    如果 (!块.首条指令) {
        块.首条指令 = 指令;
        块.末条指令 = 指令;
    } 否则 {
        块.末条指令.下一条 = 指令;
        指令.上一条 = 块.末条指令;
        块.末条指令 = 指令;
    }
}




函数 连接基本块(基本块* 源块, 基本块* 目标块) -> 空类型 {
    如果 (!源块 || !目标块) {
        返回;
    }
    
    
    基本块节点* 后继节点 = 分配内存(类型大小(基本块节点));
    后继节点.块 = 目标块;
    后继节点.下一个 = 源块.后继列表;
    源块.后继列表 = 后继节点;
    
    
    基本块节点* 前驱节点 = 分配内存(类型大小(基本块节点));
    前驱节点.块 = 源块;
    前驱节点.下一个 = 目标块.前驱列表;
    目标块.前驱列表 = 前驱节点;
}











函数 创建IR指令(IR操作码 操作码, IR操作数 目标, IR操作数 源1, IR操作数 源2) -> IR指令* {
    IR指令* 指令 = 分配内存(类型大小(IR指令));
    如果 (指令) {
        指令.操作码 = 操作码;
        指令.目标 = 目标;
        指令.源操作数1 = 源1;
        指令.源操作数2 = 源2;
        指令.额外参数 = 无;
        指令.额外参数数量 = 0;
        指令.下一条 = 无;
        指令.上一条 = 无;
    }
    返回 指令;
}



函数 释放IR指令(IR指令* 指令) -> 空类型 {
    如果 (!指令) {
        返回;
    }
    如果 (指令.额外参数) {
        释放内存(指令.额外参数);
    }
    释放内存(指令);
}







函数 空操作数() -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 无操作数;
    操作数.类型信息 = 无;
    返回 操作数;
}





函数 寄存器操作数(整数 编号, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 虚拟寄存器;
    操作数.寄存器编号 = 编号;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 整数常量操作数(整数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 整数常量;
    操作数.整数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 小数常量操作数(小数 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 小数常量;
    操作数.小数值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 字符串常量操作数(字符串 值, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 字符串常量;
    操作数.字符串值 = 值;
    操作数.类型信息 = 类型;
    返回 操作数;
}





函数 全局符号操作数(字符串 名称, 类型节点* 类型) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 全局符号;
    操作数.符号名 = 名称;
    操作数.类型信息 = 类型;
    返回 操作数;
}




函数 标签操作数(基本块* 块) -> IR操作数 {
    IR操作数 操作数;
    操作数.种类 = 基本块标签;
    操作数.目标块 = 块;
    操作数.类型信息 = 无;
    返回 操作数;
}











函数 创建全局变量(字符串 名称, 类型节点* 类型, IR操作数 初始值, 布尔 是常量) -> 全局变量* {
    全局变量* 全局变量指针 = 分配内存(类型大小(全局变量));
    如果 (全局变量指针) {
        全局变量指针.名称 = 复制字符串副本(名称);
        全局变量指针.类型信息 = 类型;
        全局变量指针.初始值 = 初始值;
        全局变量指针.是常量 = 是常量;
        全局变量指针.下一个 = 无;
    }
    返回 全局变量指针;
}




函数 添加全局变量(IR模块* 模块, 全局变量* 全局变量指针) -> 空类型 {
    如果 (!模块 || !全局变量指针) {
        返回;
    }
    如果 (!模块.首个全局) {
        模块.首个全局 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    } 否则 {
        模块.末个全局.下一个 = 全局变量指针;
        模块.末个全局 = 全局变量指针;
    }
}




函数 添加函数(IR模块* 模块, IR函数* 函数指针) -> 空类型 {
    如果 (!模块 || !函数指针) {
        返回;
    }
    如果 (!模块.首个函数) {
        模块.首个函数 = 函数指针;
        模块.末个函数 = 函数指针;
    } 否则 {
        模块.末个函数.下一个 = 函数指针;
        模块.末个函数 = 函数指针;
    }
}










函数 创建静态变量(字符串 名称, 类型节点* 类型, IR操作数 初始值) -> 静态变量* {
    静态变量* 静态变量指针 = 分配内存(类型大小(静态变量));
    如果 (静态变量指针) {
        静态变量指针.名称 = 复制字符串副本(名称);
        静态变量指针.名称长度 = 获取字符串长度(名称);
        静态变量指针.类型信息 = 类型;
        静态变量指针.初始值 = 初始值;
        静态变量指针.下一个 = 无;
    }
    返回 静态变量指针;
}
*)cn_rt_array_get_element(r19, r20, 8);
  cn_var_参数_1 = r21;
  r22 = cn_var_参数_1.类型信息;
  r23 = cn_var_缓冲区;
  r24 = 类型节点到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " p");
  r27 = cn_var_缓冲区;
  r28 = cn_var_i_0;
  r29 = 追加整数(r27, r28);
  goto for_update_301;

  if_then_305:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "void");
  goto if_merge_306;

  if_merge_306:
  r34 = cn_var_缓冲区;
  r35 = 追加字符串(r34, ") {");
  r36 = cn_var_缓冲区;
  追加换行(r36);
  r37 = cn_var_函数指针;
  r38 = cn_var_缓冲区;
  生成局部变量声明(r37, r38);
  r39 = cn_var_缓冲区;
  追加换行(r39);
  struct 基本块* cn_var_块_2;
  r40 = cn_var_函数指针->首个块;
  cn_var_块_2 = r40;
  goto while_cond_307;

  while_cond_307:
  r41 = cn_var_块_2;
  if (r41) goto while_body_308; else goto while_exit_309;

  while_body_308:
  r42 = cn_var_块_2;
  r43 = cn_var_缓冲区;
  生成基本块代码(r42, r43);
  r44 = cn_var_块_2->下一个;
  cn_var_块_2 = r44;
  goto while_cond_307;

  while_exit_309:
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, "}");
  r47 = cn_var_缓冲区;
  追加换行(r47);
  r48 = cn_var_缓冲区;
  追加换行(r48);
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
  if (r2) goto if_then_310; else goto if_merge_311;

  if_then_310:
  r3 = cn_var_模块;
  r4 = cn_var_默认缓冲区大小;
  r5 = 创建输出缓冲区(r4);
  r6 = cn_var_默认缓冲区大小;
  r7 = 创建输出缓冲区(r6);
  r8 = cn_var_默认缓冲区大小;
  r9 = 创建输出缓冲区(r8);
  r10 = cn_var_选项;
  goto if_merge_311;

  if_merge_311:
  r11 = cn_var_上下文_0;
  return r11;
}

void 释放C代码生成上下文(struct C代码生成上下文* cn_var_上下文) {
  struct C代码生成上下文* r0;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r3;
  struct C代码生成上下文* r4;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_312; else goto if_merge_313;

  if_then_312:
  r1 = cn_var_上下文->头文件缓冲区;
  释放输出缓冲区(r1);
  r2 = cn_var_上下文->源文件缓冲区;
  释放输出缓冲区(r2);
  r3 = cn_var_上下文->函数体缓冲区;
  释放输出缓冲区(r3);
  r4 = cn_var_上下文;
  释放内存(r4);
  goto if_merge_313;

  if_merge_313:
  return;
}

struct 代码生成选项 获取默认生成选项() {
  long long r0;
  void* r1;
  struct 代码生成选项* r2;
  struct 代码生成选项* r3;

  entry:
  struct 代码生成选项* cn_var_选项_0;
  r0 = sizeof(struct 代码生成选项);
  r1 = 分配清零内存(1, r0);
  cn_var_选项_0 = (struct 代码生成选项*)0;
  r2 = cn_var_选项_0;
  r3 = (struct 代码生成选项 {
    布尔 生成调试信息;       
    布尔 生成行号注释;       
    布尔 使用C99标准;        
    布尔 使用C11标准;        
    布尔 生成位置无关代码;   
    整数 缩进宽度;           
    布尔 使用制表符缩进;     
    字符串 输出目录;         
    字符串 头文件扩展名;     
    字符串 源文件扩展名;     
}






结构体 输出缓冲区 {
    字符串 数据;             
    整数 容量;               
    整数 长度;               
    整数 缩进级别;           
}






结构体 C代码生成上下文 {
    IR模块* 模块;                    
    IR函数* 当前函数;                
    基本块* 当前基本块;              
    输出缓冲区* 头文件缓冲区;        
    输出缓冲区* 源文件缓冲区;        
    输出缓冲区* 函数体缓冲区;        
    代码生成选项 选项;               
    
    
    字符串[] 局部变量名列表;         
    整数 局部变量数量;               
    字符串[] 寄存器名列表;           
    整数 寄存器数量;                 
    
    
    字符串[] 字符串字面量列表;       
    整数 字符串字面量数量;           
    
    
    整数 错误计数;                   
    整数 警告计数;                   
}





常量 整数 默认缓冲区大小 = 4096;
常量 整数 最大缩进级别 = 100;
常量 整数 最大变量数量 = 1024;
常量 整数 最大字符串数量 = 256;








函数 创建输出缓冲区(整数 初始容量) -> 输出缓冲区* {
    输出缓冲区* 缓冲区 = 分配内存(类型大小(输出缓冲区));
    如果 (缓冲区) {
        缓冲区.数据 = 分配内存(初始容量);
        缓冲区.容量 = 初始容量;
        缓冲区.长度 = 0;
        缓冲区.缩进级别 = 0;
        如果 (缓冲区.数据) {
            缓冲区.数据[0] = '\0';
        }
    }
    返回 缓冲区;
}



函数 释放输出缓冲区(输出缓冲区* 缓冲区) -> 空类型 {
    如果 (缓冲区) {
        如果 (缓冲区.数据) {
            释放内存(缓冲区.数据);
        }
        释放内存(缓冲区);
    }
}





函数 扩展缓冲区(输出缓冲区* 缓冲区, 整数 新容量) -> 布尔 {
    如果 (!缓冲区 || 新容量 <= 缓冲区.容量) {
        返回 假;
    }
    字符串 新数据 = 重新分配内存(缓冲区.数据, 新容量);
    如果 (新数据) {
        缓冲区.数据 = 新数据;
        缓冲区.容量 = 新容量;
        返回 真;
    }
    返回 假;
}





函数 追加字符串(输出缓冲区* 缓冲区, 字符串 字符串值) -> 布尔 {
    如果 (!缓冲区 || !字符串值) {
        返回 假;
    }
    
    整数 字符串长度 = 获取字符串长度(字符串值);
    整数 需要容量 = 缓冲区.长度 + 字符串长度 + 1;
    
    如果 (需要容量 > 缓冲区.容量) {
        整数 新容量 = 缓冲区.容量 * 2;
        当 (新容量 < 需要容量) {
            新容量 = 新容量 * 2;
        }
        如果 (!扩展缓冲区(缓冲区, 新容量)) {
            返回 假;
        }
    }
    
    复制内存(缓冲区.数据 + 缓冲区.长度, 字符串值, 字符串长度);
    缓冲区.长度 = 缓冲区.长度 + 字符串长度;
    缓冲区.数据[缓冲区.长度] = '\0';
    
    返回 真;
}





函数 追加字符(输出缓冲区* 缓冲区, 整数 字符值) -> 布尔 {
    变量 字符串缓冲 = (字符串)分配内存(2);
    字符串缓冲[0] = 字符值;
    字符串缓冲[1] = '\0';
    变量 结果 = 追加字符串(缓冲区, 字符串缓冲);
    释放内存(字符串缓冲);
    返回 结果;
}





函数 追加整数(输出缓冲区* 缓冲区, 整数 数值) -> 布尔 {
    变量 字符串缓冲 = (字符串)分配内存(32);
    格式化整数(字符串缓冲, 32, 数值);
    变量 结果 = 追加字符串(缓冲区, 字符串缓冲);
    释放内存(字符串缓冲);
    返回 结果;
}




函数 追加缩进(输出缓冲区* 缓冲区, 整数 缩进宽度) -> 空类型 {
    如果 (!缓冲区) {
        返回;
    }
    循环 (整数 i = 0; i < 缓冲区.缩进级别 * 缩进宽度; i = i + 1) {
        追加字符(缓冲区, ' ');
    }
}



函数 追加换行(输出缓冲区* 缓冲区) -> 空类型 {
    追加字符串(缓冲区, "\n");
}



函数 增加缩进(输出缓冲区* 缓冲区) -> 空类型 {
    如果 (缓冲区 && 缓冲区.缩进级别 < 最大缩进级别) {
        缓冲区.缩进级别 = 缓冲区.缩进级别 + 1;
    }
}



函数 减少缩进(输出缓冲区* 缓冲区) -> 空类型 {
    如果 (缓冲区 && 缓冲区.缩进级别 > 0) {
        缓冲区.缩进级别 = 缓冲区.缩进级别 - 1;
    }
}









函数 类型到C类型(类型信息* 类型, 输出缓冲区* 缓冲区) -> 字符串 {
    如果 (!类型) {
        返回 "void";
    }
    
    选择 (类型.种类) {
        情况 类型种类.类型_空:
            返回 "void";
            
        情况 类型种类.类型_整数:
            返回 "int";
            
        情况 类型种类.类型_小数:
            返回 "double";
            
        情况 类型种类.类型_布尔:
            返回 "_Bool";
            
        情况 类型种类.类型_字符串:
            返回 "cn_string_t";
            
        情况 类型种类.类型_字符:
            返回 "char";
            
        情况 类型种类.类型_指针:
            
            类型到C类型(类型.指向类型, 缓冲区);
            追加字符串(缓冲区, "*");
            返回 缓冲区.数据;
            
        情况 类型种类.类型_数组:
            
            类型到C类型(类型.元素类型, 缓冲区);
            追加字符串(缓冲区, "[");
            
            追加整数(缓冲区, 0 + 类型.维度大小[0]);
            追加字符串(缓冲区, "]");
            返回 缓冲区.数据;
            
        情况 类型种类.类型_结构体:
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型.名称);
            返回 缓冲区.数据;
            
        情况 类型种类.类型_枚举:
            追加字符串(缓冲区, "enum ");
            追加字符串(缓冲区, 类型.名称);
            返回 缓冲区.数据;
            
        情况 类型种类.类型_类:
            
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型.名称);
            返回 缓冲区.数据;
            
        情况 类型种类.类型_接口:
            
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型.名称);
            追加字符串(缓冲区, "_vtable");
            返回 缓冲区.数据;
            
        情况 类型种类.类型_函数:
            
            类型到C类型(类型.返回类型, 缓冲区);
            追加字符串(缓冲区, " (*)(");
            
            循环 (整数 i = 0; i < 类型.参数个数; i = i + 1) {
                如果 (i > 0) {
                    追加字符串(缓冲区, ", ");
                }
                类型到C类型(类型.参数类型列表[i], 缓冲区);
            }
            追加字符串(缓冲区, ")");
            返回 缓冲区.数据;
            
        默认:
            返回 "void";
    }
}






函数 生成类型声明(类型信息* 类型, 字符串 名称, 输出缓冲区* 缓冲区) -> 字符串 {
    如果 (!类型) {
        返回 "void";
    }
    
    
    如果 (类型.种类 = 类型种类.类型_指针) {
        类型到C类型(类型.指向类型, 缓冲区);
        追加字符串(缓冲区, "* ");
        追加字符串(缓冲区, 名称);
        返回 缓冲区.数据;
    }
    
    
    如果 (类型.种类 = 类型种类.类型_数组) {
        类型到C类型(类型.元素类型, 缓冲区);
        追加字符串(缓冲区, " ");
        追加字符串(缓冲区, 名称);
        循环 (整数 i = 0; i < 类型.数组维度; i = i + 1) {
            追加字符串(缓冲区, "[");
            
            追加整数(缓冲区, 0 + 类型.维度大小[i]);
            追加字符串(缓冲区, "]");
        }
        返回 缓冲区.数据;
    }
    
    
    类型到C类型(类型, 缓冲区);
    追加字符串(缓冲区, " ");
    追加字符串(缓冲区, 名称);
    返回 缓冲区.数据;
}










函数 类型节点到C类型(类型节点* 类型节点指针, 输出缓冲区* 缓冲区) -> 字符串 {
    如果 (!类型节点指针) {
        返回 "void";
    }
    
    选择 (类型节点指针.类型) {
        情况 节点类型.基础类型:
            
            如果 (比较字符串(类型节点指针.名称, "整数") == 0) {
                返回 "long long";
            }
            如果 (比较字符串(类型节点指针.名称, "小数") == 0) {
                返回 "double";
            }
            如果 (比较字符串(类型节点指针.名称, "布尔") == 0) {
                返回 "_Bool";
            }
            如果 (比较字符串(类型节点指针.名称, "字符串") == 0) {
                返回 "cn_string_t";
            }
            如果 (比较字符串(类型节点指针.名称, "字符") == 0) {
                返回 "char";
            }
            如果 (比较字符串(类型节点指针.名称, "空类型") == 0) {
                返回 "void";
            }
            
            追加字符串(缓冲区, 类型节点指针.名称);
            返回 缓冲区.数据;
            
        情况 节点类型.指针类型:
            
            类型节点到C类型(类型节点指针.元素类型, 缓冲区);
            追加字符串(缓冲区, "*");
            返回 缓冲区.数据;
            
        情况 节点类型.数组类型:
            
            类型节点到C类型(类型节点指针.元素类型, 缓冲区);
            追加字符串(缓冲区, "[");
            
            追加整数(缓冲区, 0 + 类型节点指针.数组大小);
            追加字符串(缓冲区, "]");
            返回 缓冲区.数据;
            
        情况 节点类型.结构体类型:
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型节点指针.名称);
            返回 缓冲区.数据;
            
        情况 节点类型.枚举类型:
            追加字符串(缓冲区, "enum ");
            追加字符串(缓冲区, 类型节点指针.名称);
            返回 缓冲区.数据;
            
        情况 节点类型.类类型:
            
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型节点指针.名称);
            返回 缓冲区.数据;
            
        情况 节点类型.接口类型:
            
            追加字符串(缓冲区, "struct ");
            追加字符串(缓冲区, 类型节点指针.名称);
            返回 缓冲区.数据;
            
        情况 节点类型.函数类型:
            
            追加字符串(缓冲区, "void*");
            返回 缓冲区.数据;
            
        默认:
            
            如果 (类型节点指针.名称) {
                追加字符串(缓冲区, 类型节点指针.名称);
            } 否则 {
                返回 "void";
            }
            返回 缓冲区.数据;
    }
}









函数 生成操作数代码(IR操作数 操作数, 输出缓冲区* 缓冲区) -> 字符串 {
    选择 (操作数.种类) {
        情况 IR操作数种类.虚拟寄存器:
            
            追加字符串(缓冲区, "r");
            追加整数(缓冲区, 操作数.寄存器编号);
            返回 缓冲区.数据;
            
        情况 IR操作数种类.整数常量:
            追加整数(缓冲区, 操作数.整数值);
            返回 缓冲区.数据;
            
        情况 IR操作数种类.小数常量:
            追加小数(缓冲区, 操作数.小数值);
            返回 缓冲区.数据;
            
        情况 IR操作数种类.字符串常量:
            
            追加字符串(缓冲区, "_str_");
            追加整数(缓冲区, 操作数.寄存器编号);
            返回 缓冲区.数据;
            
        情况 IR操作数种类.全局符号:
            追加字符串(缓冲区, 操作数.符号名);
            返回 缓冲区.数据;
            
        情况 IR操作数种类.基本块标签:
            追加字符串(缓冲区, "L");
            追加整数(缓冲区, 操作数.目标块.块编号);
            返回 缓冲区.数据;
            
        默认:
            返回 "";
    }
}








函数 生成算术指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    追加缩进(缓冲区, 4);
    生成操作数代码(指令.目标, 缓冲区);
    追加字符串(缓冲区, " = ");
    生成操作数代码(指令.源操作数1, 缓冲区);
    
    选择 (指令.操作码) {
        情况 IR操作码.加法指令:
            追加字符串(缓冲区, " + ");
            中断;
        情况 IR操作码.减法指令:
            追加字符串(缓冲区, " - ");
            中断;
        情况 IR操作码.乘法指令:
            追加字符串(缓冲区, " * ");
            中断;
        情况 IR操作码.除法指令:
            追加字符串(缓冲区, " / ");
            中断;
        情况 IR操作码.取模指令:
            追加字符串(缓冲区, " % ");
            中断;
        情况 IR操作码.与指令:
            追加字符串(缓冲区, " & ");
            中断;
        情况 IR操作码.或指令:
            追加字符串(缓冲区, " | ");
            中断;
        情况 IR操作码.异或指令:
            追加字符串(缓冲区, " ^ ");
            中断;
        情况 IR操作码.左移指令:
            追加字符串(缓冲区, " << ");
            中断;
        情况 IR操作码.右移指令:
            追加字符串(缓冲区, " >> ");
            中断;
        默认:
            追加字符串(缓冲区, " ? ");
    }
    
    生成操作数代码(指令.源操作数2, 缓冲区);
    追加字符串(缓冲区, ";");
    追加换行(缓冲区);
}




函数 生成一元指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    追加缩进(缓冲区, 4);
    生成操作数代码(指令.目标, 缓冲区);
    追加字符串(缓冲区, " = ");
    
    选择 (指令.操作码) {
        情况 IR操作码.负号指令:
            追加字符串(缓冲区, "-");
            中断;
        情况 IR操作码.取反指令:
            追加字符串(缓冲区, "~");
            中断;
        情况 IR操作码.逻辑非指令:
            追加字符串(缓冲区, "!");
            中断;
        默认:
            追加字符串(缓冲区, "");
    }
    
    生成操作数代码(指令.源操作数1, 缓冲区);
    追加字符串(缓冲区, ";");
    追加换行(缓冲区);
}




函数 生成比较指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    追加缩进(缓冲区, 4);
    生成操作数代码(指令.目标, 缓冲区);
    追加字符串(缓冲区, " = ");
    生成操作数代码(指令.源操作数1, 缓冲区);
    
    选择 (指令.操作码) {
        情况 IR操作码.相等指令:
            追加字符串(缓冲区, " == ");
            中断;
        情况 IR操作码.不等指令:
            追加字符串(缓冲区, " != ");
            中断;
        情况 IR操作码.小于指令:
            追加字符串(缓冲区, " < ");
            中断;
        情况 IR操作码.小于等于指令:
            追加字符串(缓冲区, " <= ");
            中断;
        情况 IR操作码.大于指令:
            追加字符串(缓冲区, " > ");
            中断;
        情况 IR操作码.大于等于指令:
            追加字符串(缓冲区, " >= ");
            中断;
        默认:
            追加字符串(缓冲区, " ? ");
    }
    
    生成操作数代码(指令.源操作数2, 缓冲区);
    追加字符串(缓冲区, ";");
    追加换行(缓冲区);
}




函数 生成内存指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    选择 (指令.操作码) {
        情况 IR操作码.分配指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = alloca(");
            
            追加整数(缓冲区, 0 + 指令.目标.类型信息.大小);
            追加字符串(缓冲区, ");");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.加载指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = *");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.存储指令:
            
            追加缩进(缓冲区, 4);
            追加字符串(缓冲区, "*");
            生成操作数代码(指令.源操作数2, 缓冲区);
            追加字符串(缓冲区, " = ");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.移动指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = ");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.取地址指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = &");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.解引用指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = *");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        默认:
            中断;
    }
}




函数 生成控制流指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    选择 (指令.操作码) {
        情况 IR操作码.标签指令:
            
            追加字符串(缓冲区, "L");
            追加整数(缓冲区, 指令.目标.目标块.块编号);
            追加字符串(缓冲区, ":");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.跳转指令:
            
            追加缩进(缓冲区, 4);
            追加字符串(缓冲区, "goto L");
            追加整数(缓冲区, 指令.目标.目标块.块编号);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.条件跳转指令:
            
            追加缩进(缓冲区, 4);
            追加字符串(缓冲区, "if (");
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, ") goto L");
            追加整数(缓冲区, 指令.源操作数1.目标块.块编号);
            追加字符串(缓冲区, "; else goto L");
            追加整数(缓冲区, 指令.源操作数2.目标块.块编号);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.调用指令:
            
            追加缩进(缓冲区, 4);
            如果 (指令.目标.种类 != IR操作数种类.无操作数) {
                生成操作数代码(指令.目标, 缓冲区);
                追加字符串(缓冲区, " = ");
            }
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, "(");
            
            循环 (整数 i = 0; i < 指令.额外参数数量; i = i + 1) {
                如果 (i > 0) {
                    追加字符串(缓冲区, ", ");
                }
                生成操作数代码(指令.额外参数[i], 缓冲区);
            }
            追加字符串(缓冲区, ");");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.返回指令:
            
            追加缩进(缓冲区, 4);
            追加字符串(缓冲区, "return");
            如果 (指令.目标.种类 != IR操作数种类.无操作数) {
                追加字符串(缓冲区, " ");
                生成操作数代码(指令.目标, 缓冲区);
            }
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        默认:
            中断;
    }
}




函数 生成结构体指令(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    选择 (指令.操作码) {
        情况 IR操作码.成员指针指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = &");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, "->");
            追加字符串(缓冲区, 指令.源操作数2.符号名);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.成员访问指令:
            
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = ");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, ".");
            追加字符串(缓冲区, 指令.源操作数2.符号名);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        情况 IR操作码.结构体初始化指令:
            
            追加缩进(缓冲区, 4);
            追加字符串(缓冲区, "memset(&");
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, ", 0, sizeof(");
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, "));");
            追加换行(缓冲区);
            中断;
            
        默认:
            中断;
    }
}




函数 生成指令代码(IR指令* 指令, 输出缓冲区* 缓冲区) -> 空类型 {
    如果 (!指令) {
        返回;
    }
    
    选择 (指令.操作码) {
        
        情况 IR操作码.加法指令:
        情况 IR操作码.减法指令:
        情况 IR操作码.乘法指令:
        情况 IR操作码.除法指令:
        情况 IR操作码.取模指令:
        情况 IR操作码.与指令:
        情况 IR操作码.或指令:
        情况 IR操作码.异或指令:
        情况 IR操作码.左移指令:
        情况 IR操作码.右移指令:
            生成算术指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.负号指令:
        情况 IR操作码.取反指令:
        情况 IR操作码.逻辑非指令:
            生成一元指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.相等指令:
        情况 IR操作码.不等指令:
        情况 IR操作码.小于指令:
        情况 IR操作码.小于等于指令:
        情况 IR操作码.大于指令:
        情况 IR操作码.大于等于指令:
            生成比较指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.分配指令:
        情况 IR操作码.加载指令:
        情况 IR操作码.存储指令:
        情况 IR操作码.移动指令:
        情况 IR操作码.取地址指令:
        情况 IR操作码.解引用指令:
            生成内存指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.标签指令:
        情况 IR操作码.跳转指令:
        情况 IR操作码.条件跳转指令:
        情况 IR操作码.调用指令:
        情况 IR操作码.返回指令:
            生成控制流指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.成员指针指令:
        情况 IR操作码.成员访问指令:
        情况 IR操作码.结构体初始化指令:
            生成结构体指令(指令, 缓冲区);
            中断;
            
        
        情况 IR操作码.PHI指令:
            
            中断;
            
        
        情况 IR操作码.选择指令:
            追加缩进(缓冲区, 4);
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " = ");
            生成操作数代码(指令.目标, 缓冲区);
            追加字符串(缓冲区, " ? ");
            生成操作数代码(指令.源操作数1, 缓冲区);
            追加字符串(缓冲区, " : ");
            生成操作数代码(指令.源操作数2, 缓冲区);
            追加字符串(缓冲区, ";");
            追加换行(缓冲区);
            中断;
            
        默认:
            中断;
    }
}








函数 生成基本块代码(基本块* 块, 输出缓冲区* 缓冲区) -> 空类型 {
    如果 (!块) {
        返回;
    }
    
    
    追加字符串(缓冲区, "L");
    追加整数(缓冲区, 块.块编号);
    追加字符串(缓冲区, ":");
    追加换行(缓冲区);
    
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        生成指令代码(指令, 缓冲区);
        指令 = 指令.下一条;
    }
}








函数 生成局部变量声明(IR函数* 函数指针, 输出缓冲区* 缓冲区) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    
    
    循环 (整数 i = 0; i < 函数指针.局部变量数量; i = i + 1) {
        IR操作数 局部变量 = 函数指针.局部变量[i];
        追加缩进(缓冲区, 4);
        类型节点到C类型(局部变量.类型信息, 缓冲区);
        追加字符串(缓冲区, " r");
        追加整数(缓冲区, 局部变量.寄存器编号);
        追加字符串(缓冲区, ";");
        追加换行(缓冲区);
    }
}




函数 生成函数代码(IR函数* 函数指针, 输出缓冲区* 缓冲区) -> 空类型 {
    如果 (!函数指针) {
        返回;
    }
    
    
    类型节点到C类型(函数指针.返回类型, 缓冲区);
    追加字符串(缓冲区, " ");
    
    
    追加字符串(缓冲区, 函数指针.名称);
    追加字符串(缓冲区, "(");
    
    
    循环 (整数 i = 0; i < 函数指针.参数数量; i = i + 1) {
        如果 (i > 0) {
            追加字符串(缓冲区, ", ");
        }
        IR操作数 参数 = 函数指针.参数列表[i];
        类型节点到C类型(参数.类型信息, 缓冲区);
        追加字符串(缓冲区, " p");
        追加整数(缓冲区, i);
    }
    
    
    如果 (函数指针.参数数量 = 0) {
        追加字符串(缓冲区, "void");
    }
    
    追加字符串(缓冲区, ") {");
    追加换行(缓冲区);
    
    
    生成局部变量声明(函数指针, 缓冲区);
    
    追加换行(缓冲区);
    
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        生成基本块代码(块, 缓冲区);
        块 = 块.下一个;
    }
    
    
    追加字符串(缓冲区, "}");
    追加换行(缓冲区);
    追加换行(缓冲区);
}









函数 创建C代码生成上下文(IR模块* 模块, 代码生成选项 选项) -> C代码生成上下文* {
    C代码生成上下文* 上下文 = 分配内存(类型大小(C代码生成上下文));
    如果 (上下文) {
        上下文.模块 = 模块;
        上下文.当前函数 = 无;
        上下文.当前基本块 = 无;
        上下文.头文件缓冲区 = 创建输出缓冲区(默认缓冲区大小);
        上下文.源文件缓冲区 = 创建输出缓冲区(默认缓冲区大小);
        上下文.函数体缓冲区 = 创建输出缓冲区(默认缓冲区大小);
        上下文.选项 = 选项;
        上下文.局部变量数量 = 0;
        上下文.寄存器数量 = 0;
        上下文.字符串字面量数量 = 0;
        上下文.错误计数 = 0;
        上下文.警告计数 = 0;
    }
    返回 上下文;
}



函数 释放C代码生成上下文(C代码生成上下文* 上下文) -> 空类型 {
    如果 (上下文) {
        释放输出缓冲区(上下文.头文件缓冲区);
        释放输出缓冲区(上下文.源文件缓冲区);
        释放输出缓冲区(上下文.函数体缓冲区);
        释放内存(上下文);
    }
}



函数 获取默认生成选项() -> 代码生成选项 {
    变量 选项 = (代码生成选项*)分配清零内存(1, 类型大小(代码生成选项));
    选项.生成调试信息 = 假;
    选项.生成行号注释 = 真;
    选项.使用C99标准 = 真;
    选项.使用C11标准 = 假;
    选项.生成位置无关代码 = 假;
    选项.缩进宽度 = 4;
    选项.使用制表符缩进 = 假;
    选项.输出目录 = ".";
    选项.头文件扩展名 = ".h";
    选项.源文件扩展名 = ".c";
    返回 选项[0];
}









函数 格式化整数(字符串 缓冲区, 整数 缓冲区大小, 整数 数值) -> 空类型 {
    
    如果 (数值 = 0) {
        缓冲区[0] = '0';
        缓冲区[1] = '\0';
        返回;
    }
    
    变量 临时 = (字符串)分配内存(32);
    变量 索引 = 0;
    变量 负数 = 假;
    
    如果 (数值 < 0) {
        负数 = 真;
        数值 = -数值;
    }
    
    当 (数值 > 0) {
        临时[索引] = '0' + (数值 % 10);
        索引 = 索引 + 1;
        数值 = 数值 / 10;
    }
    
    变量 输出索引 = 0;
    如果 (负数) {
        缓冲区[输出索引] = '-';
        输出索引 = 输出索引 + 1;
    }
    
    当 (索引 > 0) {
        索引 = 索引 - 1;
        缓冲区[输出索引] = 临时[索引];
        输出索引 = 输出索引 + 1;
    }
    
    缓冲区[输出索引] = '\0';
    释放内存(临时);
}




函数 追加小数(输出缓冲区* 缓冲区, 小数 数值) -> 空类型 {
    
    整数 整数部分 = (整数)数值;
    小数 小数临时 = 数值 - (小数)整数部分;
    整数 小数部分 = (整数)(小数临时 * 1000000);
    
    追加整数(缓冲区, 整数部分);
    追加字符串(缓冲区, ".");
    追加整数(缓冲区, 小数部分);
}
*)cn_rt_array_get_element(r2, 0, 8);
  return r3;
}

void 格式化整数(const char* cn_var_缓冲区, long long cn_var_缓冲区大小, long long cn_var_数值) {
  long long r3, r5, r6, r7, r9, r10, r11, r13, r14, r15, r16, r17, r20, r21, r22, r23, r25, r26, r28, r32, r33, r34, r36;
  char* r0;
  char* r1;
  char* r12;
  char* r19;
  char* r27;
  char* r31;
  char* r35;
  char* r37;
  void* r2;
  char* r29;
  _Bool r4;
  _Bool r8;
  _Bool r18;
  _Bool r24;
  char r30;

  entry:
  cn_var_数值 = 0;
  if (0) goto if_then_314; else goto if_merge_315;

  if_then_314:
  r0 = cn_var_缓冲区;
    { long long _tmp_i3 = 48; cn_rt_array_set_element(r0, 0, &_tmp_i3, 8); }
  r1 = cn_var_缓冲区;
    { long long _tmp_i4 = 0; cn_rt_array_set_element(r1, 1, &_tmp_i4, 8); }
  return;
  goto if_merge_315;

  if_merge_315:
  char* cn_var_临时_0;
  r2 = 分配内存(32);
  cn_var_临时_0 = (char*)0;
  long long cn_var_索引_1;
  cn_var_索引_1 = 0;
  _Bool cn_var_负数_2;
  cn_var_负数_2 = 0;
  r3 = cn_var_数值;
  r4 = r3 < 0;
  if (r4) goto if_then_316; else goto if_merge_317;

  if_then_316:
  cn_var_负数_2 = 1;
  r5 = cn_var_数值;
  r6 = -r5;
  cn_var_数值 = r6;
  goto if_merge_317;

  if_merge_317:
  goto while_cond_318;

  while_cond_318:
  r7 = cn_var_数值;
  r8 = r7 > 0;
  if (r8) goto while_body_319; else goto while_exit_320;

  while_body_319:
  r9 = cn_var_数值;
  r10 = r9 % 10;
  r11 = 48 + r10;
  r12 = cn_var_临时_0;
  r13 = cn_var_索引_1;
    { long long _tmp_r1 = r11; cn_rt_array_set_element(r12, r13, &_tmp_r1, 8); }
  r14 = cn_var_索引_1;
  r15 = r14 + 1;
  cn_var_索引_1 = r15;
  r16 = cn_var_数值;
  r17 = r16 / 10;
  cn_var_数值 = r17;
  goto while_cond_318;

  while_exit_320:
  long long cn_var_输出索引_3;
  cn_var_输出索引_3 = 0;
  r18 = cn_var_负数_2;
  if (r18) goto if_then_321; else goto if_merge_322;

  if_then_321:
  r19 = cn_var_缓冲区;
  r20 = cn_var_输出索引_3;
    { long long _tmp_i5 = 45; cn_rt_array_set_element(r19, r20, &_tmp_i5, 8); }
  r21 = cn_var_输出索引_3;
  r22 = r21 + 1;
  cn_var_输出索引_3 = r22;
  goto if_merge_322;

  if_merge_322:
  goto while_cond_323;

  while_cond_323:
  r23 = cn_var_索引_1;
  r24 = r23 > 0;
  if (r24) goto while_body_324; else goto while_exit_325;

  while_body_324:
  r25 = cn_var_索引_1;
  r26 = r25 - 1;
  cn_var_索引_1 = r26;
  r27 = cn_var_临时_0;
  r28 = cn_var_索引_1;
  r29 = &r27[r28];
  r30 = *r29;
  r31 = cn_var_缓冲区;
  r32 = cn_var_输出索引_3;
    { long long _tmp_r2 = r30; cn_rt_array_set_element(r31, r32, &_tmp_r2, 8); }
  r33 = cn_var_输出索引_3;
  r34 = r33 + 1;
  cn_var_输出索引_3 = r34;
  goto while_cond_323;

  while_exit_325:
  r35 = cn_var_缓冲区;
  r36 = cn_var_输出索引_3;
    { long long _tmp_i6 = 0; cn_rt_array_set_element(r35, r36, &_tmp_i6, 8); }
  r37 = cn_var_临时_0;
  释放内存(r37);
  return;
}

void 追加小数(struct 输出缓冲区* cn_var_缓冲区, double cn_var_数值) {
  long long r2, r5, r7, r12;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r11;
  _Bool r8;
  _Bool r10;
  _Bool r13;
  double r0;
  double r1;
  double r3;
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

