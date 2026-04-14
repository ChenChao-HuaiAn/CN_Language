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
extern long long cn_var_作用域指针大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号大小;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
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
extern long long cn_var_最大字符串数量;
extern long long cn_var_最大变量数量;
extern long long cn_var_最大缩进级别;
extern long long cn_var_默认缓冲区大小;
void 追加小数(struct 输出缓冲区*, double);
void 格式化整数(const char*, long long, long long);
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
char* 类型节点到C类型(struct 类型节点*, struct 输出缓冲区*);
char* 生成类型声明(struct 类型信息*, const char*, struct 输出缓冲区*);
char* 类型到C类型(struct 类型信息*, struct 输出缓冲区*);
void 减少缩进(struct 输出缓冲区*);
void 增加缩进(struct 输出缓冲区*);
void 追加换行(struct 输出缓冲区*);
void 追加缩进(struct 输出缓冲区*, long long);
_Bool 追加整数(struct 输出缓冲区*, long long);
_Bool 追加字符(struct 输出缓冲区*, long long);
_Bool 追加字符串(struct 输出缓冲区*, const char*);
_Bool 扩展缓冲区(struct 输出缓冲区*, long long);
void 释放输出缓冲区(struct 输出缓冲区*);
struct 输出缓冲区* 创建输出缓冲区(long long);
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
char* 生成头文件保护宏(const char*, struct 输出缓冲区*);
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
struct 模块代码生成上下文* 创建模块代码生成上下文(struct IR模块*, const char*, struct 代码生成选项);
void 释放模块代码生成上下文(struct 模块代码生成上下文*);
void 生成模块代码(struct 模块代码生成上下文*);
char* 获取头文件内容(struct 模块代码生成上下文*);
char* 获取源文件内容(struct 模块代码生成上下文*);
_Bool 写入代码文件(struct 模块代码生成上下文*, const char*);
long long 获取生成结构体数量(struct 模块代码生成上下文*);
long long 获取生成枚举数量(struct 模块代码生成上下文*);
long long 获取生成全局变量数量(struct 模块代码生成上下文*);
long long 获取生成函数数量(struct 模块代码生成上下文*);
long long 获取错误计数(struct 模块代码生成上下文*);
long long 获取警告计数(struct 模块代码生成上下文*);

char* 生成头文件保护宏(const char* cn_var_模块名, struct 输出缓冲区* cn_var_缓冲区) {
  long long r7;
  char* r3;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r5;
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
  r7 = cn_var_缓冲区.数据;
  return r7;
}

void 生成头文件开头(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r2, r8, r15, r16, r21, r27, r31;
  char* r17;
  struct 模块代码生成上下文* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r18;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r30;
  _Bool r4;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3545; else goto if_merge_3546;

  if_then_3545:
  return;
  goto if_merge_3546;

  if_merge_3546:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文.头文件缓冲区;
  cn_var_缓冲区_0 = r2;
  r3 = cn_var_缓冲区_0;
  r4 = 追加字符串(r3, "// ========================================\n");
  r5 = cn_var_缓冲区_0;
  r6 = 追加字符串(r5, "// 自动生成的头文件 - ");
  r7 = cn_var_缓冲区_0;
  r8 = cn_var_上下文.模块名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区_0;
  追加换行(r10);
  r11 = cn_var_缓冲区_0;
  r12 = 追加字符串(r11, "// ========================================\n");
  r13 = cn_var_缓冲区_0;
  追加换行(r13);
  long long cn_var_保护宏_1;
  r14 = 创建输出缓冲区(256);
  cn_var_保护宏_1 = r14;
  r15 = cn_var_上下文.模块名称;
  r16 = cn_var_保护宏_1;
  r17 = 生成头文件保护宏(r15, r16);
  r18 = cn_var_缓冲区_0;
  r19 = 追加字符串(r18, "#ifndef ");
  r20 = cn_var_缓冲区_0;
  r21 = cn_var_保护宏_1.数据;
  r22 = 追加字符串(r20, r21);
  r23 = cn_var_缓冲区_0;
  追加换行(r23);
  r24 = cn_var_缓冲区_0;
  r25 = 追加字符串(r24, "#define ");
  r26 = cn_var_缓冲区_0;
  r27 = cn_var_保护宏_1.数据;
  r28 = 追加字符串(r26, r27);
  r29 = cn_var_缓冲区_0;
  追加换行(r29);
  r30 = cn_var_缓冲区_0;
  追加换行(r30);
  r31 = cn_var_保护宏_1;
  释放输出缓冲区(r31);
  return;
}

void 生成头文件结尾(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r2, r7, r8, r11, r14;
  char* r9;
  struct 模块代码生成上下文* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r4;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r13;
  _Bool r5;
  _Bool r12;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3547; else goto if_merge_3548;

  if_then_3547:
  return;
  goto if_merge_3548;

  if_merge_3548:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文.头文件缓冲区;
  cn_var_缓冲区_0 = r2;
  r3 = cn_var_缓冲区_0;
  追加换行(r3);
  r4 = cn_var_缓冲区_0;
  r5 = 追加字符串(r4, "#endif // ");
  long long cn_var_保护宏_1;
  r6 = 创建输出缓冲区(256);
  cn_var_保护宏_1 = r6;
  r7 = cn_var_上下文.模块名称;
  r8 = cn_var_保护宏_1;
  r9 = 生成头文件保护宏(r7, r8);
  r10 = cn_var_缓冲区_0;
  r11 = cn_var_保护宏_1.数据;
  r12 = 追加字符串(r10, r11);
  r13 = cn_var_缓冲区_0;
  追加换行(r13);
  r14 = cn_var_保护宏_1;
  释放输出缓冲区(r14);
  return;
}

void 生成源文件开头(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r2, r8, r17;
  struct 模块代码生成上下文* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  _Bool r4;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r20;
  _Bool r23;
  _Bool r25;
  _Bool r27;
  _Bool r29;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3549; else goto if_merge_3550;

  if_then_3549:
  return;
  goto if_merge_3550;

  if_merge_3550:
  struct 输出缓冲区* cn_var_缓冲区_0;
  r2 = cn_var_上下文.源文件缓冲区;
  cn_var_缓冲区_0 = r2;
  r3 = cn_var_缓冲区_0;
  r4 = 追加字符串(r3, "// ========================================\n");
  r5 = cn_var_缓冲区_0;
  r6 = 追加字符串(r5, "// 自动生成的源文件 - ");
  r7 = cn_var_缓冲区_0;
  r8 = cn_var_上下文.模块名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区_0;
  追加换行(r10);
  r11 = cn_var_缓冲区_0;
  r12 = 追加字符串(r11, "// ========================================\n");
  r13 = cn_var_缓冲区_0;
  追加换行(r13);
  r14 = cn_var_缓冲区_0;
  r15 = 追加字符串(r14, "#include \"");
  r16 = cn_var_缓冲区_0;
  r17 = cn_var_上下文.输出文件名;
  r18 = 追加字符串(r16, r17);
  r19 = cn_var_缓冲区_0;
  r20 = 追加字符串(r19, ".h\"\n");
  r21 = cn_var_缓冲区_0;
  追加换行(r21);
  r22 = cn_var_缓冲区_0;
  r23 = 追加字符串(r22, "#include <cnlang/runtime/runtime.h>\n");
  r24 = cn_var_缓冲区_0;
  r25 = 追加字符串(r24, "#include <cnlang/runtime/stdlib.h>\n");
  r26 = cn_var_缓冲区_0;
  r27 = 追加字符串(r26, "#include <cnlang/runtime/memory.h>\n");
  r28 = cn_var_缓冲区_0;
  r29 = 追加字符串(r28, "#include <cnlang/runtime/io.h>\n");
  r30 = cn_var_缓冲区_0;
  追加换行(r30);
  return;
}

void 生成结构体定义(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8, r12, r13, r14, r16, r17, r24, r25, r30, r31;
  char* r20;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r15;
  void* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  void* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r22;
  _Bool r27;
  _Bool r29;
  _Bool r33;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3554; else goto logic_rhs_3553;

  if_then_3551:
  return;
  goto if_merge_3552;

  if_merge_3552:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "struct ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, " {\n");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3555;

  logic_rhs_3553:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_结构体;
  goto logic_merge_3554;

  logic_merge_3554:
  if (r4) goto if_then_3551; else goto if_merge_3552;

  for_cond_3555:
  r12 = cn_var_i_0;
  r13 = cn_var_类型.成员个数;
  r14 = r12 < r13;
  if (r14) goto for_body_3556; else goto for_exit_3558;

  for_body_3556:
  r15 = cn_var_缓冲区;
  追加缩进(r15, 4);
  r16 = cn_var_类型.成员类型列表;
  r17 = cn_var_i_0;
  r18 = (void*)cn_rt_array_get_element(r16, r17, 8);
  r19 = cn_var_缓冲区;
  r20 = 类型到C类型(r18, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, " ");
  r23 = cn_var_缓冲区;
  r24 = cn_var_类型.成员名称列表;
  r25 = cn_var_i_0;
  r26 = (void*)cn_rt_array_get_element(r24, r25, 8);
  r27 = 追加字符串(r23, r26);
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, ";\n");
  goto for_update_3557;

  for_update_3557:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_3555;

  for_exit_3558:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "};\n");
  r34 = cn_var_缓冲区;
  追加换行(r34);
  return;
}

void 生成结构体前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  _Bool r6;
  _Bool r9;
  _Bool r11;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3562; else goto logic_rhs_3561;

  if_then_3559:
  return;
  goto if_merge_3560;

  if_merge_3560:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "struct ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, ";\n");

  logic_rhs_3561:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_结构体;
  goto logic_merge_3562;

  logic_merge_3562:
  if (r4) goto if_then_3559; else goto if_merge_3560;
  return;
}

void 生成枚举定义(struct 类型信息* cn_var_类型, struct 符号* cn_var_符号, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8, r12, r13, r14, r15, r16, r20, r22, r23, r24, r25, r26, r30, r31, r33, r34, r35, r36, r40, r41;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  void* r17;
  struct 输出缓冲区* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r42;
  struct 输出缓冲区* r44;
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r21;
  _Bool r28;
  _Bool r32;
  _Bool r38;
  _Bool r43;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3566; else goto logic_rhs_3565;

  if_then_3563:
  return;
  goto if_merge_3564;

  if_merge_3564:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "enum ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, " {\n");
  long long cn_var_索引_0;
  cn_var_索引_0 = 0;
  goto for_cond_3567;

  logic_rhs_3565:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_枚举;
  goto logic_merge_3566;

  logic_merge_3566:
  if (r4) goto if_then_3563; else goto if_merge_3564;

  for_cond_3567:
  r12 = cn_var_索引_0;
  r13 = cn_var_符号.成员个数;
  r14 = r12 < r13;
  if (r14) goto for_body_3568; else goto for_exit_3570;

  for_body_3568:
  struct 符号* cn_var_成员_1;
  r15 = cn_var_符号.成员列表;
  r16 = cn_var_索引_0;
  r17 = (void*)cn_rt_array_get_element(r15, r16, 8);
  cn_var_成员_1 = r17;
  r18 = cn_var_缓冲区;
  追加缩进(r18, 4);
  r19 = cn_var_缓冲区;
  r20 = cn_var_成员_1->名称;
  r21 = 追加字符串(r19, r20);
  r23 = cn_var_成员_1->枚举值;
  r24 = r23 != 0;
  if (r24) goto logic_merge_3574; else goto logic_rhs_3573;

  for_update_3569:
  r40 = cn_var_索引_0;
  r41 = r40 + 1;
  cn_var_索引_0 = r41;
  goto for_cond_3567;

  for_exit_3570:
  r42 = cn_var_缓冲区;
  r43 = 追加字符串(r42, "};\n");
  r44 = cn_var_缓冲区;
  追加换行(r44);

  if_then_3571:
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, " = ");
  r29 = cn_var_缓冲区;
  r30 = cn_var_成员_1->枚举值;
  r31 = 0 + r30;
  r32 = 追加整数(r29, r31);
  goto if_merge_3572;

  if_merge_3572:
  r33 = cn_var_索引_0;
  r34 = cn_var_符号.成员个数;
  r35 = r34 - 1;
  r36 = r33 < r35;
  if (r36) goto if_then_3575; else goto if_merge_3576;

  logic_rhs_3573:
  r25 = cn_var_索引_0;
  r26 = r25 == 0;
  goto logic_merge_3574;

  logic_merge_3574:
  if (r26) goto if_then_3571; else goto if_merge_3572;

  if_then_3575:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ",");
  goto if_merge_3576;

  if_merge_3576:
  r39 = cn_var_缓冲区;
  追加换行(r39);
  goto for_update_3569;
  return;
}

void 生成枚举前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  _Bool r6;
  _Bool r9;
  _Bool r11;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3580; else goto logic_rhs_3579;

  if_then_3577:
  return;
  goto if_merge_3578;

  if_merge_3578:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "enum ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, ";\n");

  logic_rhs_3579:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_枚举;
  goto logic_merge_3580;

  logic_merge_3580:
  if (r4) goto if_then_3577; else goto if_merge_3578;
  return;
}

void 生成类定义(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8, r12, r13, r14, r16, r17, r24, r25, r30, r31;
  char* r20;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r15;
  void* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  void* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r22;
  _Bool r27;
  _Bool r29;
  _Bool r33;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3584; else goto logic_rhs_3583;

  if_then_3581:
  return;
  goto if_merge_3582;

  if_merge_3582:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "struct ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, " {\n");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3585;

  logic_rhs_3583:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_类;
  goto logic_merge_3584;

  logic_merge_3584:
  if (r4) goto if_then_3581; else goto if_merge_3582;

  for_cond_3585:
  r12 = cn_var_i_0;
  r13 = cn_var_类型.成员个数;
  r14 = r12 < r13;
  if (r14) goto for_body_3586; else goto for_exit_3588;

  for_body_3586:
  r15 = cn_var_缓冲区;
  追加缩进(r15, 4);
  r16 = cn_var_类型.成员类型列表;
  r17 = cn_var_i_0;
  r18 = (void*)cn_rt_array_get_element(r16, r17, 8);
  r19 = cn_var_缓冲区;
  r20 = 类型到C类型(r18, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, " ");
  r23 = cn_var_缓冲区;
  r24 = cn_var_类型.成员名称列表;
  r25 = cn_var_i_0;
  r26 = (void*)cn_rt_array_get_element(r24, r25, 8);
  r27 = 追加字符串(r23, r26);
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, ";\n");
  goto for_update_3587;

  for_update_3587:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_3585;

  for_exit_3588:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "};\n");
  r34 = cn_var_缓冲区;
  追加换行(r34);
  return;
}

void 生成类前向声明(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r8;
  struct 类型信息* r1;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  _Bool r6;
  _Bool r9;
  _Bool r11;

  entry:
  r1 = cn_var_类型;
  r2 = !r1;
  if (r2) goto logic_merge_3592; else goto logic_rhs_3591;

  if_then_3589:
  return;
  goto if_merge_3590;

  if_merge_3590:
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, "struct ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_类型.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, ";\n");

  logic_rhs_3591:
  r3 = cn_var_类型.种类;
  r4 = r3 != 类型种类_类型_类;
  goto logic_merge_3592;

  logic_merge_3592:
  if (r4) goto if_then_3589; else goto if_merge_3590;
  return;
}

void 生成全局变量声明(struct 全局变量* cn_var_全局变量信息, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r4, r10;
  char* r6;
  struct 全局变量* r0;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r12;
  _Bool r3;
  _Bool r8;
  _Bool r11;
  _Bool r13;

  entry:
  r0 = cn_var_全局变量信息;
  r1 = !r0;
  if (r1) goto if_then_3593; else goto if_merge_3594;

  if_then_3593:
  return;
  goto if_merge_3594;

  if_merge_3594:
  r2 = cn_var_缓冲区;
  r3 = 追加字符串(r2, "extern ");
  r4 = cn_var_全局变量信息.类型信息;
  r5 = cn_var_缓冲区;
  r6 = 类型节点到C类型(r4, r5);
  r7 = cn_var_缓冲区;
  r8 = 追加字符串(r7, " ");
  r9 = cn_var_缓冲区;
  r10 = cn_var_全局变量信息.名称;
  r11 = 追加字符串(r9, r10);
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, ";\n");
  return;
}

void 生成全局变量定义(struct 全局变量* cn_var_全局变量信息, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r2, r8, r10, r11, r12, r15;
  char* r4;
  char* r17;
  struct 全局变量* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r13;
  struct 输出缓冲区* r16;
  struct 输出缓冲区* r18;
  _Bool r6;
  _Bool r9;
  _Bool r14;
  _Bool r19;

  entry:
  r0 = cn_var_全局变量信息;
  r1 = !r0;
  if (r1) goto if_then_3595; else goto if_merge_3596;

  if_then_3595:
  return;
  goto if_merge_3596;

  if_merge_3596:
  r2 = cn_var_全局变量信息.类型信息;
  r3 = cn_var_缓冲区;
  r4 = 类型节点到C类型(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_全局变量信息.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_全局变量信息.初始值;
  r11 = r10.种类;
  r12 = r11 != IR操作数种类_无操作数;
  if (r12) goto if_then_3597; else goto if_merge_3598;

  if_then_3597:
  r13 = cn_var_缓冲区;
  r14 = 追加字符串(r13, " = ");
  r15 = cn_var_全局变量信息.初始值;
  r16 = cn_var_缓冲区;
  r17 = 生成操作数代码(r15, r16);
  goto if_merge_3598;

  if_merge_3598:
  r18 = cn_var_缓冲区;
  r19 = 追加字符串(r18, ";\n");
  return;
}

void 生成函数声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r2, r8, r12, r13, r14, r15, r16, r19, r20, r22, r25, r26;
  char* r4;
  char* r24;
  struct IR函数* r0;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r17;
  void* r21;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r18;
  _Bool r28;
  _Bool r30;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_3599; else goto if_merge_3600;

  if_then_3599:
  return;
  goto if_merge_3600;

  if_merge_3600:
  r2 = cn_var_函数指针.返回类型;
  r3 = cn_var_缓冲区;
  r4 = 类型节点到C类型(r2, r3);
  r5 = cn_var_缓冲区;
  r6 = 追加字符串(r5, " ");
  r7 = cn_var_缓冲区;
  r8 = cn_var_函数指针.名称;
  r9 = 追加字符串(r7, r8);
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3601;

  for_cond_3601:
  r12 = cn_var_i_0;
  r13 = cn_var_函数指针.参数数量;
  r14 = r12 < r13;
  if (r14) goto for_body_3602; else goto for_exit_3604;

  for_body_3602:
  r15 = cn_var_i_0;
  r16 = r15 > 0;
  if (r16) goto if_then_3605; else goto if_merge_3606;

  for_update_3603:
  r25 = cn_var_i_0;
  r26 = r25 + 1;
  cn_var_i_0 = r26;
  goto for_cond_3601;

  for_exit_3604:
  if (0) goto if_then_3607; else goto if_merge_3608;

  if_then_3605:
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, ", ");
  goto if_merge_3606;

  if_merge_3606:
  struct IR操作数 cn_var_参数_1;
  r19 = cn_var_函数指针.参数列表;
  r20 = cn_var_i_0;
  r21 = (void*)cn_rt_array_get_element(r19, r20, 8);
  cn_var_参数_1 = r21;
  r22 = cn_var_参数_1.类型信息;
  r23 = cn_var_缓冲区;
  r24 = 类型节点到C类型(r22, r23);
  goto for_update_3603;

  if_then_3607:
  r27 = cn_var_缓冲区;
  r28 = 追加字符串(r27, "void");
  goto if_merge_3608;

  if_merge_3608:
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, ");\n");
  return;
}

struct 模块代码生成上下文* 创建模块代码生成上下文(struct IR模块* cn_var_模块, const char* cn_var_模块名, struct 代码生成选项 cn_var_选项) {
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
  if (r2) goto if_then_3609; else goto if_merge_3610;

  if_then_3609:
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
  r22 = 复制字符串副本(r21);
  r23 = cn_var_模块名;
  r24 = 复制字符串副本(r23);
  goto if_merge_3610;

  if_merge_3610:
  r25 = cn_var_上下文_0;
  return r25;
}

void 释放模块代码生成上下文(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
  struct 模块代码生成上下文* r0;
  struct 模块代码生成上下文* r13;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3611; else goto if_merge_3612;

  if_then_3611:
  r1 = cn_var_上下文.头文件缓冲区;
  释放输出缓冲区(r1);
  r2 = cn_var_上下文.源文件缓冲区;
  释放输出缓冲区(r2);
  r3 = cn_var_上下文.前向声明缓冲区;
  释放输出缓冲区(r3);
  r4 = cn_var_上下文.结构体定义缓冲区;
  释放输出缓冲区(r4);
  r5 = cn_var_上下文.枚举定义缓冲区;
  释放输出缓冲区(r5);
  r6 = cn_var_上下文.全局变量缓冲区;
  释放输出缓冲区(r6);
  r7 = cn_var_上下文.函数声明缓冲区;
  释放输出缓冲区(r7);
  r8 = cn_var_上下文.函数定义缓冲区;
  释放输出缓冲区(r8);
  r9 = cn_var_上下文.模块名称;
  if (r9) goto if_then_3613; else goto if_merge_3614;

  if_merge_3612:

  if_then_3613:
  r10 = cn_var_上下文.模块名称;
  释放内存(r10);
  goto if_merge_3614;

  if_merge_3614:
  r11 = cn_var_上下文.输出文件名;
  if (r11) goto if_then_3615; else goto if_merge_3616;

  if_then_3615:
  r12 = cn_var_上下文.输出文件名;
  释放内存(r12);
  goto if_merge_3616;

  if_merge_3616:
  r13 = cn_var_上下文;
  释放内存(r13);
  goto if_merge_3612;
  return;
}

void 生成模块代码(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0, r2, r3, r4, r7, r8, r11, r13, r14, r15, r16, r17, r18, r21, r23, r24, r25, r26;
  struct 模块代码生成上下文* r1;
  struct 模块代码生成上下文* r5;
  struct 模块代码生成上下文* r6;
  struct 全局变量* r9;
  struct 全局变量* r10;
  struct 全局变量* r12;
  struct IR函数* r19;
  struct IR函数* r20;
  struct IR函数* r22;
  struct 模块代码生成上下文* r27;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3620; else goto logic_rhs_3619;

  if_then_3617:
  return;
  goto if_merge_3618;

  if_merge_3618:
  r5 = cn_var_上下文;
  生成头文件开头(r5);
  r6 = cn_var_上下文;
  生成源文件开头(r6);
  struct 全局变量* cn_var_全局_0;
  r7 = cn_var_上下文.模块;
  r8 = r7.首个全局;
  cn_var_全局_0 = r8;
  goto while_cond_3621;

  logic_rhs_3619:
  r3 = cn_var_上下文.模块;
  r4 = !r3;
  goto logic_merge_3620;

  logic_merge_3620:
  if (r4) goto if_then_3617; else goto if_merge_3618;

  while_cond_3621:
  r9 = cn_var_全局_0;
  if (r9) goto while_body_3622; else goto while_exit_3623;

  while_body_3622:
  r10 = cn_var_全局_0;
  r11 = cn_var_上下文.头文件缓冲区;
  生成全局变量声明(r10, r11);
  r12 = cn_var_全局_0;
  r13 = cn_var_上下文.源文件缓冲区;
  生成全局变量定义(r12, r13);
  r14 = cn_var_全局_0->下一个;
  cn_var_全局_0 = r14;
  r15 = cn_var_上下文.生成的全局变量数量;
  r16 = r15 + 1;
  goto while_cond_3621;

  while_exit_3623:
  struct IR函数* cn_var_函数指针_1;
  r17 = cn_var_上下文.模块;
  r18 = r17.首个函数;
  cn_var_函数指针_1 = r18;
  goto while_cond_3624;

  while_cond_3624:
  r19 = cn_var_函数指针_1;
  if (r19) goto while_body_3625; else goto while_exit_3626;

  while_body_3625:
  r20 = cn_var_函数指针_1;
  r21 = cn_var_上下文.头文件缓冲区;
  生成函数声明(r20, r21);
  r22 = cn_var_函数指针_1;
  r23 = cn_var_上下文.源文件缓冲区;
  生成函数代码(r22, r23);
  r24 = cn_var_函数指针_1->下一个;
  cn_var_函数指针_1 = r24;
  r25 = cn_var_上下文.生成的函数数量;
  r26 = r25 + 1;
  goto while_cond_3624;

  while_exit_3626:
  r27 = cn_var_上下文;
  生成头文件结尾(r27);
  return;
}

char* 获取头文件内容(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0, r2, r3, r4;
  struct 模块代码生成上下文* r1;

  entry:
  r1 = cn_var_上下文;
  if (r1) goto logic_rhs_3629; else goto logic_merge_3630;

  if_then_3627:
  r3 = cn_var_上下文.头文件缓冲区;
  r4 = r3.数据;
  return r4;
  goto if_merge_3628;

  if_merge_3628:
  return 0;

  logic_rhs_3629:
  r2 = cn_var_上下文.头文件缓冲区;
  goto logic_merge_3630;

  logic_merge_3630:
  if (r2) goto if_then_3627; else goto if_merge_3628;
  return NULL;
}

char* 获取源文件内容(struct 模块代码生成上下文* cn_var_上下文) {
  long long r0, r2, r3, r4;
  struct 模块代码生成上下文* r1;

  entry:
  r1 = cn_var_上下文;
  if (r1) goto logic_rhs_3633; else goto logic_merge_3634;

  if_then_3631:
  r3 = cn_var_上下文.源文件缓冲区;
  r4 = r3.数据;
  return r4;
  goto if_merge_3632;

  if_merge_3632:
  return 0;

  logic_rhs_3633:
  r2 = cn_var_上下文.源文件缓冲区;
  goto logic_merge_3634;

  logic_merge_3634:
  if (r2) goto if_then_3631; else goto if_merge_3632;
  return NULL;
}

_Bool 写入代码文件(struct 模块代码生成上下文* cn_var_上下文, const char* cn_var_输出目录) {
  long long r1, r3, r4, r5, r6, r7, r9, r12, r14, r15, r17, r20, r21, r22, r23, r24, r26, r29, r31, r32, r34, r36, r38, r39, r40, r41, r42, r43, r44, r45, r46, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57;
  char* r2;
  char* r10;
  char* r11;
  char* r13;
  char* r16;
  char* r18;
  char* r19;
  char* r27;
  char* r28;
  char* r30;
  char* r33;
  char* r35;
  struct 模块代码生成上下文* r0;
  void* r8;
  void* r25;
  void* r37;
  void* r47;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3635; else goto if_merge_3636;

  if_then_3635:
  return 0;
  goto if_merge_3636;

  if_merge_3636:
  long long cn_var_头文件路径_0;
  r2 = cn_var_输出目录;
  r3 = 获取字符串长度(r2);
  r4 = cn_var_上下文.输出文件名;
  r5 = 获取字符串长度(r4);
  r6 = r3 + r5;
  r7 = r6 + 4;
  r8 = 分配内存(r7);
  cn_var_头文件路径_0 = r8;
  r9 = cn_var_头文件路径_0;
  r10 = cn_var_输出目录;
  r11 = 连接字符串(r9, r10);
  r12 = cn_var_头文件路径_0;
  r13 = 连接字符串(r12, "/");
  r14 = cn_var_头文件路径_0;
  r15 = cn_var_上下文.输出文件名;
  r16 = 连接字符串(r14, r15);
  r17 = cn_var_头文件路径_0;
  r18 = 连接字符串(r17, ".h");
  long long cn_var_源文件路径_1;
  r19 = cn_var_输出目录;
  r20 = 获取字符串长度(r19);
  r21 = cn_var_上下文.输出文件名;
  r22 = 获取字符串长度(r21);
  r23 = r20 + r22;
  r24 = r23 + 4;
  r25 = 分配内存(r24);
  cn_var_源文件路径_1 = r25;
  r26 = cn_var_源文件路径_1;
  r27 = cn_var_输出目录;
  r28 = 连接字符串(r26, r27);
  r29 = cn_var_源文件路径_1;
  r30 = 连接字符串(r29, "/");
  r31 = cn_var_源文件路径_1;
  r32 = cn_var_上下文.输出文件名;
  r33 = 连接字符串(r31, r32);
  r34 = cn_var_源文件路径_1;
  r35 = 连接字符串(r34, ".c");
  long long cn_var_头文件_2;
  r36 = cn_var_头文件路径_0;
  r37 = 打开文件(r36, "w");
  cn_var_头文件_2 = r37;
  r38 = cn_var_头文件_2;
  if (r38) goto if_then_3637; else goto if_merge_3638;

  if_then_3637:
  r39 = cn_var_头文件_2;
  r40 = cn_var_上下文.头文件缓冲区;
  r41 = r40.数据;
  r42 = cn_var_上下文.头文件缓冲区;
  r43 = r42.长度;
  r44 = 写入文件(r39, r41, r43);
  r45 = cn_var_头文件_2;
  关闭文件(r45);
  goto if_merge_3638;

  if_merge_3638:
  long long cn_var_源文件_3;
  r46 = cn_var_源文件路径_1;
  r47 = 打开文件(r46, "w");
  cn_var_源文件_3 = r47;
  r48 = cn_var_源文件_3;
  if (r48) goto if_then_3639; else goto if_merge_3640;

  if_then_3639:
  r49 = cn_var_源文件_3;
  r50 = cn_var_上下文.源文件缓冲区;
  r51 = r50.数据;
  r52 = cn_var_上下文.源文件缓冲区;
  r53 = r52.长度;
  r54 = 写入文件(r49, r51, r53);
  r55 = cn_var_源文件_3;
  关闭文件(r55);
  goto if_merge_3640;

  if_merge_3640:
  r56 = cn_var_头文件路径_0;
  释放内存(r56);
  r57 = cn_var_源文件路径_1;
  释放内存(r57);
  return 1;
}

long long 获取生成结构体数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3641; else goto if_merge_3642;

  if_then_3641:
  r1 = cn_var_上下文.生成的结构体数量;
  return r1;
  goto if_merge_3642;

  if_merge_3642:
  return 0;
}

long long 获取生成枚举数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3643; else goto if_merge_3644;

  if_then_3643:
  r1 = cn_var_上下文.生成的枚举数量;
  return r1;
  goto if_merge_3644;

  if_merge_3644:
  return 0;
}

long long 获取生成全局变量数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3645; else goto if_merge_3646;

  if_then_3645:
  r1 = cn_var_上下文.生成的全局变量数量;
  return r1;
  goto if_merge_3646;

  if_merge_3646:
  return 0;
}

long long 获取生成函数数量(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3647; else goto if_merge_3648;

  if_then_3647:
  r1 = cn_var_上下文.生成的函数数量;
  return r1;
  goto if_merge_3648;

  if_merge_3648:
  return 0;
}

long long 获取错误计数(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3649; else goto if_merge_3650;

  if_then_3649:
  r1 = cn_var_上下文.错误计数;
  return r1;
  goto if_merge_3650;

  if_merge_3650:
  return 0;
}

long long 获取警告计数(struct 模块代码生成上下文* cn_var_上下文) {
  long long r1;
  struct 模块代码生成上下文* r0;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_3651; else goto if_merge_3652;

  if_then_3651:
  r1 = cn_var_上下文.警告计数;
  return r1;
  goto if_merge_3652;

  if_merge_3652:
  return 0;
}

