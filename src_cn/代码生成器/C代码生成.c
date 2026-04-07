#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
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
struct 符号表管理器;
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
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
struct 符号;
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
struct 表达式节点;
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 语句节点;
struct 语句节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    long long* 语句体;
};
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    long long* 语句体;
};
struct 声明节点;
struct 声明节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
};
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
struct 类成员;
struct 类成员 {
    char* 名称;
    enum 节点类型 类型;
    long long* 字段;
    long long* 方法;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
};
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
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
struct 静态变量;
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
struct 静态变量;
struct 静态变量 {
    char* 名称;
    long long 名称长度;
    struct 类型节点* 类型信息;
    struct IR操作数 初始值;
    struct 静态变量* 下一个;
};
struct 基本块节点;
struct 基本块节点;
struct 基本块节点 {
    struct 基本块* 块;
    struct 基本块节点* 下一个;
};
struct IR指令;
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
  if (r2) goto if_then_3121; else goto if_merge_3122;

  if_then_3121:
  r3 = cn_var_初始容量;
  r4 = 分配内存(r3);
  r5 = cn_var_初始容量;
  r6 = cn_var_缓冲区_0;
  r7 = r6->数据;
  if (r7) goto if_then_3123; else goto if_merge_3124;

  if_merge_3122:
  r10 = cn_var_缓冲区_0;
  return r10;

  if_then_3123:
  r8 = cn_var_缓冲区_0;
  r9 = r8->数据;
    { long long _tmp_i3 = 0; cn_rt_array_set_element(r9, 0, &_tmp_i3, 8); }
  goto if_merge_3124;

  if_merge_3124:
  goto if_merge_3122;
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
  if (r0) goto if_then_3125; else goto if_merge_3126;

  if_then_3125:
  r1 = cn_var_缓冲区;
  r2 = r1->数据;
  if (r2) goto if_then_3127; else goto if_merge_3128;

  if_merge_3126:

  if_then_3127:
  r3 = cn_var_缓冲区;
  r4 = r3->数据;
  释放内存(r4);
  goto if_merge_3128;

  if_merge_3128:
  r5 = cn_var_缓冲区;
  释放内存(r5);
  goto if_merge_3126;
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
  if (r2) goto logic_merge_3132; else goto logic_rhs_3131;

  if_then_3129:
  return 0;
  goto if_merge_3130;

  if_merge_3130:
  char* cn_var_新数据_0;
  r7 = cn_var_缓冲区;
  r8 = r7->数据;
  r9 = cn_var_新容量;
  r10 = 重新分配内存(r8, r9);
  cn_var_新数据_0 = r10;
  r11 = cn_var_新数据_0;
  if (r11) goto if_then_3133; else goto if_merge_3134;

  logic_rhs_3131:
  r3 = cn_var_新容量;
  r4 = cn_var_缓冲区;
  r5 = r4->容量;
  r6 = r3 <= r5;
  goto logic_merge_3132;

  logic_merge_3132:
  if (r6) goto if_then_3129; else goto if_merge_3130;

  if_then_3133:
  r12 = cn_var_新数据_0;
  r13 = cn_var_新容量;
  return 1;
  goto if_merge_3134;

  if_merge_3134:
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
  if (r2) goto logic_merge_3138; else goto logic_rhs_3137;

  if_then_3135:
  return 0;
  goto if_merge_3136;

  if_merge_3136:
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
  if (r15) goto if_then_3139; else goto if_merge_3140;

  logic_rhs_3137:
  r3 = cn_var_字符串值;
  r4 = !r3;
  goto logic_merge_3138;

  logic_merge_3138:
  if (r4) goto if_then_3135; else goto if_merge_3136;

  if_then_3139:
  long long cn_var_新容量_2;
  r16 = cn_var_缓冲区;
  r17 = r16->容量;
  r18 = r17 << 1;
  cn_var_新容量_2 = r18;
  goto while_cond_3141;

  if_merge_3140:
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
    { long long _tmp_i4 = 0; cn_rt_array_set_element(r41, r43, &_tmp_i4, 8); }
  return 1;

  while_cond_3141:
  r19 = cn_var_新容量_2;
  r20 = cn_var_需要容量_1;
  r21 = r19 < r20;
  if (r21) goto while_body_3142; else goto while_exit_3143;

  while_body_3142:
  r22 = cn_var_新容量_2;
  r23 = r22 << 1;
  cn_var_新容量_2 = r23;
  goto while_cond_3141;

  while_exit_3143:
  r24 = cn_var_缓冲区;
  r25 = cn_var_新容量_2;
  r26 = 扩展缓冲区(r24, r25);
  r27 = !r26;
  if (r27) goto if_then_3144; else goto if_merge_3145;

  if_then_3144:
  return 0;
  goto if_merge_3145;

  if_merge_3145:
  goto if_merge_3140;
  return 0;
}

_Bool 追加字符(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_字符值) {
  long long r1, r2, r3, r5, r7, r8;
  void* r0;
  struct 输出缓冲区* r4;
  _Bool r6;

  entry:
  void cn_var_字符串缓冲_0;
  r0 = 分配内存(2);
  cn_var_字符串缓冲_0 = r0;
  r1 = cn_var_字符值;
  r2 = cn_var_字符串缓冲_0;
    { long long _tmp_r14 = r1; cn_rt_array_set_element(r2, 0, &_tmp_r14, 8); }
  r3 = cn_var_字符串缓冲_0;
    { long long _tmp_i5 = 0; cn_rt_array_set_element(r3, 1, &_tmp_i5, 8); }
  void cn_var_结果_1;
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
  long long r1, r2, r4, r6, r7;
  void* r0;
  struct 输出缓冲区* r3;
  _Bool r5;

  entry:
  void cn_var_字符串缓冲_0;
  r0 = 分配内存(32);
  cn_var_字符串缓冲_0 = r0;
  r1 = cn_var_字符串缓冲_0;
  r2 = cn_var_数值;
  格式化整数(r1, 32, r2);
  void cn_var_结果_1;
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
  if (r1) goto if_then_3146; else goto if_merge_3147;

  if_then_3146:
  return;
  goto if_merge_3147;

  if_merge_3147:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3148;

  for_cond_3148:
  r2 = cn_var_i_0;
  r3 = cn_var_缓冲区;
  r4 = r3->缩进级别;
  r5 = cn_var_缩进宽度;
  r6 = r4 * r5;
  r7 = r2 < r6;
  if (r7) goto for_body_3149; else goto for_exit_3151;

  for_body_3149:
  r8 = cn_var_缓冲区;
  r9 = 追加字符(r8, 32);
  goto for_update_3150;

  for_update_3150:
  r10 = cn_var_i_0;
  r11 = r10 + 1;
  cn_var_i_0 = r11;
  goto for_cond_3148;

  for_exit_3151:
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
  if (r1) goto logic_rhs_3154; else goto logic_merge_3155;

  if_then_3152:
  r6 = cn_var_缓冲区;
  r7 = r6->缩进级别;
  r8 = r7 + 1;
  goto if_merge_3153;

  if_merge_3153:

  logic_rhs_3154:
  r2 = cn_var_缓冲区;
  r3 = r2->缩进级别;
  r4 = cn_var_最大缩进级别;
  r5 = r3 < r4;
  goto logic_merge_3155;

  logic_merge_3155:
  if (r5) goto if_then_3152; else goto if_merge_3153;
  return;
}

void 减少缩进(struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r3, r4, r6, r7;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r2;
  struct 输出缓冲区* r5;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_3158; else goto logic_merge_3159;

  if_then_3156:
  r5 = cn_var_缓冲区;
  r6 = r5->缩进级别;
  r7 = r6 - 1;
  goto if_merge_3157;

  if_merge_3157:

  logic_rhs_3158:
  r2 = cn_var_缓冲区;
  r3 = r2->缩进级别;
  r4 = r3 > 0;
  goto logic_merge_3159;

  logic_merge_3159:
  if (r4) goto if_then_3156; else goto if_merge_3157;
  return;
}

char* 类型到C类型(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r5, r8, r11, r14, r17, r20, r23, r26, r29, r32, r35, r38, r41, r60, r106, r108, r109, r110, r111, r116, r117, r120, r121;
  char* r46;
  char* r50;
  char* r54;
  char* r65;
  char* r70;
  char* r73;
  char* r78;
  char* r81;
  char* r86;
  char* r89;
  char* r94;
  char* r99;
  char* r103;
  char* r119;
  char* r125;
  void* r59;
  void* r115;
  struct 类型信息* r0;
  struct 类型信息* r2;
  struct 类型信息* r43;
  struct 类型信息* r44;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 类型信息* r51;
  struct 类型信息* r52;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct 类型信息* r58;
  struct 输出缓冲区* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r66;
  struct 输出缓冲区* r68;
  struct 类型信息* r69;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r74;
  struct 输出缓冲区* r76;
  struct 类型信息* r77;
  struct 输出缓冲区* r80;
  struct 输出缓冲区* r82;
  struct 输出缓冲区* r84;
  struct 类型信息* r85;
  struct 输出缓冲区* r88;
  struct 输出缓冲区* r90;
  struct 输出缓冲区* r92;
  struct 类型信息* r93;
  struct 输出缓冲区* r96;
  struct 输出缓冲区* r98;
  struct 类型信息* r100;
  struct 类型信息* r101;
  struct 输出缓冲区* r102;
  struct 输出缓冲区* r104;
  struct 类型信息* r107;
  struct 输出缓冲区* r112;
  struct 类型信息* r114;
  struct 输出缓冲区* r118;
  struct 输出缓冲区* r122;
  struct 输出缓冲区* r124;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r33;
  _Bool r36;
  _Bool r39;
  _Bool r42;
  _Bool r48;
  _Bool r56;
  _Bool r61;
  _Bool r63;
  _Bool r67;
  _Bool r71;
  _Bool r75;
  _Bool r79;
  _Bool r83;
  _Bool r87;
  _Bool r91;
  _Bool r95;
  _Bool r97;
  _Bool r105;
  _Bool r113;
  _Bool r123;
  enum 类型种类 r3;
  enum 类型种类 r4;
  enum 类型种类 r7;
  enum 类型种类 r10;
  enum 类型种类 r13;
  enum 类型种类 r16;
  enum 类型种类 r19;
  enum 类型种类 r22;
  enum 类型种类 r25;
  enum 类型种类 r28;
  enum 类型种类 r31;
  enum 类型种类 r34;
  enum 类型种类 r37;
  enum 类型种类 r40;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_3160; else goto if_merge_3161;

  if_then_3160:
  return "void";
  goto if_merge_3161;

  if_merge_3161:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_空;
  r6 = r3 == r5;
  if (r6) goto case_body_3163; else goto switch_check_3177;

  switch_check_3177:
  r7 = cn_var_类型种类;
  r8 = r7.类型_整数;
  r9 = r3 == r8;
  if (r9) goto case_body_3164; else goto switch_check_3178;

  switch_check_3178:
  r10 = cn_var_类型种类;
  r11 = r10.类型_小数;
  r12 = r3 == r11;
  if (r12) goto case_body_3165; else goto switch_check_3179;

  switch_check_3179:
  r13 = cn_var_类型种类;
  r14 = r13.类型_布尔;
  r15 = r3 == r14;
  if (r15) goto case_body_3166; else goto switch_check_3180;

  switch_check_3180:
  r16 = cn_var_类型种类;
  r17 = r16.类型_字符串;
  r18 = r3 == r17;
  if (r18) goto case_body_3167; else goto switch_check_3181;

  switch_check_3181:
  r19 = cn_var_类型种类;
  r20 = r19.类型_字符;
  r21 = r3 == r20;
  if (r21) goto case_body_3168; else goto switch_check_3182;

  switch_check_3182:
  r22 = cn_var_类型种类;
  r23 = r22.类型_指针;
  r24 = r3 == r23;
  if (r24) goto case_body_3169; else goto switch_check_3183;

  switch_check_3183:
  r25 = cn_var_类型种类;
  r26 = r25.类型_数组;
  r27 = r3 == r26;
  if (r27) goto case_body_3170; else goto switch_check_3184;

  switch_check_3184:
  r28 = cn_var_类型种类;
  r29 = r28.类型_结构体;
  r30 = r3 == r29;
  if (r30) goto case_body_3171; else goto switch_check_3185;

  switch_check_3185:
  r31 = cn_var_类型种类;
  r32 = r31.类型_枚举;
  r33 = r3 == r32;
  if (r33) goto case_body_3172; else goto switch_check_3186;

  switch_check_3186:
  r34 = cn_var_类型种类;
  r35 = r34.类型_类;
  r36 = r3 == r35;
  if (r36) goto case_body_3173; else goto switch_check_3187;

  switch_check_3187:
  r37 = cn_var_类型种类;
  r38 = r37.类型_接口;
  r39 = r3 == r38;
  if (r39) goto case_body_3174; else goto switch_check_3188;

  switch_check_3188:
  r40 = cn_var_类型种类;
  r41 = r40.类型_函数;
  r42 = r3 == r41;
  if (r42) goto case_body_3175; else goto case_default_3176;

  case_body_3163:
  return "void";
  goto switch_merge_3162;

  case_body_3164:
  return "int";
  goto switch_merge_3162;

  case_body_3165:
  return "double";
  goto switch_merge_3162;

  case_body_3166:
  return "_Bool";
  goto switch_merge_3162;

  case_body_3167:
  return "cn_string_t";
  goto switch_merge_3162;

  case_body_3168:
  return "char";
  goto switch_merge_3162;

  case_body_3169:
  r43 = cn_var_类型;
  r44 = r43->指向类型;
  r45 = cn_var_缓冲区;
  r46 = 类型到C类型(r44, r45);
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, "*");
  r49 = cn_var_缓冲区;
  r50 = r49->数据;
  return r50;
  goto switch_merge_3162;

  case_body_3170:
  r51 = cn_var_类型;
  r52 = r51->元素类型;
  r53 = cn_var_缓冲区;
  r54 = 类型到C类型(r52, r53);
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, "[");
  r57 = cn_var_缓冲区;
  r58 = cn_var_类型;
  r59 = r58->维度大小;
  r60 = *(void*)cn_rt_array_get_element(r59, 0, 8);
  r61 = 追加整数(r57, r60);
  r62 = cn_var_缓冲区;
  r63 = 追加字符串(r62, "]");
  r64 = cn_var_缓冲区;
  r65 = r64->数据;
  return r65;
  goto switch_merge_3162;

  case_body_3171:
  r66 = cn_var_缓冲区;
  r67 = 追加字符串(r66, "struct ");
  r68 = cn_var_缓冲区;
  r69 = cn_var_类型;
  r70 = r69->名称;
  r71 = 追加字符串(r68, r70);
  r72 = cn_var_缓冲区;
  r73 = r72->数据;
  return r73;
  goto switch_merge_3162;

  case_body_3172:
  r74 = cn_var_缓冲区;
  r75 = 追加字符串(r74, "enum ");
  r76 = cn_var_缓冲区;
  r77 = cn_var_类型;
  r78 = r77->名称;
  r79 = 追加字符串(r76, r78);
  r80 = cn_var_缓冲区;
  r81 = r80->数据;
  return r81;
  goto switch_merge_3162;

  case_body_3173:
  r82 = cn_var_缓冲区;
  r83 = 追加字符串(r82, "struct ");
  r84 = cn_var_缓冲区;
  r85 = cn_var_类型;
  r86 = r85->名称;
  r87 = 追加字符串(r84, r86);
  r88 = cn_var_缓冲区;
  r89 = r88->数据;
  return r89;
  goto switch_merge_3162;

  case_body_3174:
  r90 = cn_var_缓冲区;
  r91 = 追加字符串(r90, "struct ");
  r92 = cn_var_缓冲区;
  r93 = cn_var_类型;
  r94 = r93->名称;
  r95 = 追加字符串(r92, r94);
  r96 = cn_var_缓冲区;
  r97 = 追加字符串(r96, "_vtable");
  r98 = cn_var_缓冲区;
  r99 = r98->数据;
  return r99;
  goto switch_merge_3162;

  case_body_3175:
  r100 = cn_var_类型;
  r101 = r100->返回类型;
  r102 = cn_var_缓冲区;
  r103 = 类型到C类型(r101, r102);
  r104 = cn_var_缓冲区;
  r105 = 追加字符串(r104, " (*)(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3189;

  for_cond_3189:
  r106 = cn_var_i_0;
  r107 = cn_var_类型;
  r108 = r107->参数个数;
  r109 = r106 < r108;
  if (r109) goto for_body_3190; else goto for_exit_3192;

  for_body_3190:
  r110 = cn_var_i_0;
  r111 = r110 > 0;
  if (r111) goto if_then_3193; else goto if_merge_3194;

  for_update_3191:
  r120 = cn_var_i_0;
  r121 = r120 + 1;
  cn_var_i_0 = r121;
  goto for_cond_3189;

  for_exit_3192:
  r122 = cn_var_缓冲区;
  r123 = 追加字符串(r122, ")");
  r124 = cn_var_缓冲区;
  r125 = r124->数据;
  return r125;
  goto switch_merge_3162;

  if_then_3193:
  r112 = cn_var_缓冲区;
  r113 = 追加字符串(r112, ", ");
  goto if_merge_3194;

  if_merge_3194:
  r114 = cn_var_类型;
  r115 = r114->参数类型列表;
  r116 = cn_var_i_0;
  r117 = *(void*)cn_rt_array_get_element(r115, r116, 8);
  r118 = cn_var_缓冲区;
  r119 = 类型到C类型(r117, r118);
  goto for_update_3191;

  case_default_3176:
  return "void";
  goto switch_merge_3162;

  switch_merge_3162:
  return NULL;
}

char* 生成类型声明(struct 类型信息* cn_var_类型, char* cn_var_名称, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r3, r16, r26, r28, r29, r35, r36, r40, r41;
  char* r7;
  char* r11;
  char* r14;
  char* r20;
  char* r24;
  char* r43;
  char* r46;
  char* r50;
  char* r53;
  void* r34;
  struct 类型信息* r0;
  struct 类型信息* r4;
  struct 类型信息* r5;
  struct 输出缓冲区* r6;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r13;
  struct 类型信息* r17;
  struct 类型信息* r18;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r23;
  struct 类型信息* r27;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r32;
  struct 类型信息* r33;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r42;
  struct 类型信息* r44;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r52;
  _Bool r9;
  _Bool r12;
  _Bool r22;
  _Bool r25;
  _Bool r31;
  _Bool r37;
  _Bool r39;
  _Bool r48;
  _Bool r51;
  enum 类型种类 r2;
  enum 类型种类 r15;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_3195; else goto if_merge_3196;

  if_then_3195:
  return "void";
  goto if_merge_3196;

  if_merge_3196:
  r2 = cn_var_类型种类;
  r3 = r2.类型_指针;
  if (r3) goto if_then_3197; else goto if_merge_3198;

  if_then_3197:
  r4 = cn_var_类型;
  r5 = r4->指向类型;
  r6 = cn_var_缓冲区;
  r7 = 类型到C类型(r5, r6);
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, "* ");
  r10 = cn_var_缓冲区;
  r11 = cn_var_名称;
  r12 = 追加字符串(r10, r11);
  r13 = cn_var_缓冲区;
  r14 = r13->数据;
  return r14;
  goto if_merge_3198;

  if_merge_3198:
  r15 = cn_var_类型种类;
  r16 = r15.类型_数组;
  if (r16) goto if_then_3199; else goto if_merge_3200;

  if_then_3199:
  r17 = cn_var_类型;
  r18 = r17->元素类型;
  r19 = cn_var_缓冲区;
  r20 = 类型到C类型(r18, r19);
  r21 = cn_var_缓冲区;
  r22 = 追加字符串(r21, " ");
  r23 = cn_var_缓冲区;
  r24 = cn_var_名称;
  r25 = 追加字符串(r23, r24);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3201;

  if_merge_3200:
  r44 = cn_var_类型;
  r45 = cn_var_缓冲区;
  r46 = 类型到C类型(r44, r45);
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, " ");
  r49 = cn_var_缓冲区;
  r50 = cn_var_名称;
  r51 = 追加字符串(r49, r50);
  r52 = cn_var_缓冲区;
  r53 = r52->数据;
  return r53;

  for_cond_3201:
  r26 = cn_var_i_0;
  r27 = cn_var_类型;
  r28 = r27->数组维度;
  r29 = r26 < r28;
  if (r29) goto for_body_3202; else goto for_exit_3204;

  for_body_3202:
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, "[");
  r32 = cn_var_缓冲区;
  r33 = cn_var_类型;
  r34 = r33->维度大小;
  r35 = cn_var_i_0;
  r36 = *(void*)cn_rt_array_get_element(r34, r35, 8);
  r37 = 追加整数(r32, r36);
  r38 = cn_var_缓冲区;
  r39 = 追加字符串(r38, "]");
  goto for_update_3203;

  for_update_3203:
  r40 = cn_var_i_0;
  r41 = r40 + 1;
  cn_var_i_0 = r41;
  goto for_cond_3201;

  for_exit_3204:
  r42 = cn_var_缓冲区;
  r43 = r42->数据;
  return r43;
  goto if_merge_3200;
  return NULL;
}

char* 生成操作数代码(struct IR操作数 cn_var_操作数, struct 输出缓冲区* cn_var_缓冲区) {
  long long r3, r6, r9, r12, r15, r18, r24, r30, r43, r58;
  char* r27;
  char* r33;
  char* r38;
  char* r46;
  char* r49;
  char* r52;
  char* r61;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 基本块* r57;
  struct 输出缓冲区* r60;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r21;
  _Bool r25;
  _Bool r31;
  _Bool r40;
  _Bool r44;
  _Bool r50;
  _Bool r54;
  _Bool r59;
  double r36;
  struct IR操作数 r0;
  struct IR操作数 r23;
  struct IR操作数 r29;
  struct IR操作数 r35;
  struct IR操作数 r42;
  struct IR操作数 r48;
  struct IR操作数 r56;
  enum IR操作数种类 r1;
  enum IR操作数种类 r2;
  enum IR操作数种类 r5;
  enum IR操作数种类 r8;
  enum IR操作数种类 r11;
  enum IR操作数种类 r14;
  enum IR操作数种类 r17;

  entry:
  r0 = cn_var_操作数;
  r1 = r0.种类;
  r2 = cn_var_IR操作数种类;
  r3 = r2.虚拟寄存器;
  r4 = r1 == r3;
  if (r4) goto case_body_3206; else goto switch_check_3213;

  switch_check_3213:
  r5 = cn_var_IR操作数种类;
  r6 = r5.整数常量;
  r7 = r1 == r6;
  if (r7) goto case_body_3207; else goto switch_check_3214;

  switch_check_3214:
  r8 = cn_var_IR操作数种类;
  r9 = r8.小数常量;
  r10 = r1 == r9;
  if (r10) goto case_body_3208; else goto switch_check_3215;

  switch_check_3215:
  r11 = cn_var_IR操作数种类;
  r12 = r11.字符串常量;
  r13 = r1 == r12;
  if (r13) goto case_body_3209; else goto switch_check_3216;

  switch_check_3216:
  r14 = cn_var_IR操作数种类;
  r15 = r14.全局符号;
  r16 = r1 == r15;
  if (r16) goto case_body_3210; else goto switch_check_3217;

  switch_check_3217:
  r17 = cn_var_IR操作数种类;
  r18 = r17.基本块标签;
  r19 = r1 == r18;
  if (r19) goto case_body_3211; else goto case_default_3212;

  case_body_3206:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, "r");
  r22 = cn_var_缓冲区;
  r23 = cn_var_操作数;
  r24 = r23.寄存器编号;
  r25 = 追加整数(r22, r24);
  r26 = cn_var_缓冲区;
  r27 = r26->数据;
  return r27;
  goto switch_merge_3205;

  case_body_3207:
  r28 = cn_var_缓冲区;
  r29 = cn_var_操作数;
  r30 = r29.整数值;
  r31 = 追加整数(r28, r30);
  r32 = cn_var_缓冲区;
  r33 = r32->数据;
  return r33;
  goto switch_merge_3205;

  case_body_3208:
  r34 = cn_var_缓冲区;
  r35 = cn_var_操作数;
  r36 = r35.小数值;
  追加小数(r34, r36);
  r37 = cn_var_缓冲区;
  r38 = r37->数据;
  return r38;
  goto switch_merge_3205;

  case_body_3209:
  r39 = cn_var_缓冲区;
  r40 = 追加字符串(r39, "_str_");
  r41 = cn_var_缓冲区;
  r42 = cn_var_操作数;
  r43 = r42.寄存器编号;
  r44 = 追加整数(r41, r43);
  r45 = cn_var_缓冲区;
  r46 = r45->数据;
  return r46;
  goto switch_merge_3205;

  case_body_3210:
  r47 = cn_var_缓冲区;
  r48 = cn_var_操作数;
  r49 = r48.符号名;
  r50 = 追加字符串(r47, r49);
  r51 = cn_var_缓冲区;
  r52 = r51->数据;
  return r52;
  goto switch_merge_3205;

  case_body_3211:
  r53 = cn_var_缓冲区;
  r54 = 追加字符串(r53, "L");
  r55 = cn_var_缓冲区;
  r56 = cn_var_操作数;
  r57 = r56.目标块;
  r58 = r57->块编号;
  r59 = 追加整数(r55, r58);
  r60 = cn_var_缓冲区;
  r61 = r60->数据;
  return r61;
  goto switch_merge_3205;

  case_default_3212:
  return "";
  goto switch_merge_3205;

  switch_merge_3205:
  return NULL;
}

void 生成算术指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r14, r17, r20, r23, r26, r29, r32, r35, r38, r41;
  char* r4;
  char* r10;
  char* r68;
  struct 输出缓冲区* r0;
  struct IR指令* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct IR指令* r7;
  struct 输出缓冲区* r9;
  struct IR指令* r11;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct 输出缓冲区* r59;
  struct 输出缓冲区* r61;
  struct 输出缓冲区* r63;
  struct IR指令* r65;
  struct 输出缓冲区* r67;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  _Bool r6;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r33;
  _Bool r36;
  _Bool r39;
  _Bool r42;
  _Bool r44;
  _Bool r46;
  _Bool r48;
  _Bool r50;
  _Bool r52;
  _Bool r54;
  _Bool r56;
  _Bool r58;
  _Bool r60;
  _Bool r62;
  _Bool r64;
  _Bool r70;
  struct IR操作数 r2;
  struct IR操作数 r8;
  struct IR操作数 r66;
  enum IR操作码 r12;
  enum IR操作码 r13;
  enum IR操作码 r16;
  enum IR操作码 r19;
  enum IR操作码 r22;
  enum IR操作码 r25;
  enum IR操作码 r28;
  enum IR操作码 r31;
  enum IR操作码 r34;
  enum IR操作码 r37;
  enum IR操作码 r40;

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
  r13 = cn_var_IR操作码;
  r14 = r13.加法指令;
  r15 = r12 == r14;
  if (r15) goto case_body_3219; else goto switch_check_3230;

  switch_check_3230:
  r16 = cn_var_IR操作码;
  r17 = r16.减法指令;
  r18 = r12 == r17;
  if (r18) goto case_body_3220; else goto switch_check_3231;

  switch_check_3231:
  r19 = cn_var_IR操作码;
  r20 = r19.乘法指令;
  r21 = r12 == r20;
  if (r21) goto case_body_3221; else goto switch_check_3232;

  switch_check_3232:
  r22 = cn_var_IR操作码;
  r23 = r22.除法指令;
  r24 = r12 == r23;
  if (r24) goto case_body_3222; else goto switch_check_3233;

  switch_check_3233:
  r25 = cn_var_IR操作码;
  r26 = r25.取模指令;
  r27 = r12 == r26;
  if (r27) goto case_body_3223; else goto switch_check_3234;

  switch_check_3234:
  r28 = cn_var_IR操作码;
  r29 = r28.与指令;
  r30 = r12 == r29;
  if (r30) goto case_body_3224; else goto switch_check_3235;

  switch_check_3235:
  r31 = cn_var_IR操作码;
  r32 = r31.或指令;
  r33 = r12 == r32;
  if (r33) goto case_body_3225; else goto switch_check_3236;

  switch_check_3236:
  r34 = cn_var_IR操作码;
  r35 = r34.异或指令;
  r36 = r12 == r35;
  if (r36) goto case_body_3226; else goto switch_check_3237;

  switch_check_3237:
  r37 = cn_var_IR操作码;
  r38 = r37.左移指令;
  r39 = r12 == r38;
  if (r39) goto case_body_3227; else goto switch_check_3238;

  switch_check_3238:
  r40 = cn_var_IR操作码;
  r41 = r40.右移指令;
  r42 = r12 == r41;
  if (r42) goto case_body_3228; else goto case_default_3229;

  case_body_3219:
  r43 = cn_var_缓冲区;
  r44 = 追加字符串(r43, " + ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3220:
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, " - ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3221:
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, " * ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3222:
  r49 = cn_var_缓冲区;
  r50 = 追加字符串(r49, " / ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3223:
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, " % ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3224:
  r53 = cn_var_缓冲区;
  r54 = 追加字符串(r53, " & ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3225:
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, " | ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3226:
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, " ^ ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3227:
  r59 = cn_var_缓冲区;
  r60 = 追加字符串(r59, " << ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_body_3228:
  r61 = cn_var_缓冲区;
  r62 = 追加字符串(r61, " >> ");
  goto switch_merge_3218;
  goto switch_merge_3218;

  case_default_3229:
  r63 = cn_var_缓冲区;
  r64 = 追加字符串(r63, " ? ");
  goto switch_merge_3218;

  switch_merge_3218:
  r65 = cn_var_指令;
  r66 = r65->源操作数2;
  r67 = cn_var_缓冲区;
  r68 = 生成操作数代码(r66, r67);
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, ";");
  r71 = cn_var_缓冲区;
  追加换行(r71);
  return;
}

void 生成一元指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r10, r13, r16;
  char* r4;
  char* r29;
  struct 输出缓冲区* r0;
  struct IR指令* r1;
  struct 输出缓冲区* r3;
  struct 输出缓冲区* r5;
  struct IR指令* r7;
  struct 输出缓冲区* r18;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct IR指令* r26;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r30;
  struct 输出缓冲区* r32;
  _Bool r6;
  _Bool r11;
  _Bool r14;
  _Bool r17;
  _Bool r19;
  _Bool r21;
  _Bool r23;
  _Bool r25;
  _Bool r31;
  struct IR操作数 r2;
  struct IR操作数 r27;
  enum IR操作码 r8;
  enum IR操作码 r9;
  enum IR操作码 r12;
  enum IR操作码 r15;

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
  r9 = cn_var_IR操作码;
  r10 = r9.负号指令;
  r11 = r8 == r10;
  if (r11) goto case_body_3240; else goto switch_check_3244;

  switch_check_3244:
  r12 = cn_var_IR操作码;
  r13 = r12.取反指令;
  r14 = r8 == r13;
  if (r14) goto case_body_3241; else goto switch_check_3245;

  switch_check_3245:
  r15 = cn_var_IR操作码;
  r16 = r15.逻辑非指令;
  r17 = r8 == r16;
  if (r17) goto case_body_3242; else goto case_default_3243;

  case_body_3240:
  r18 = cn_var_缓冲区;
  r19 = 追加字符串(r18, "-");
  goto switch_merge_3239;
  goto switch_merge_3239;

  case_body_3241:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, "~");
  goto switch_merge_3239;
  goto switch_merge_3239;

  case_body_3242:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, "!");
  goto switch_merge_3239;
  goto switch_merge_3239;

  case_default_3243:
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, "");
  goto switch_merge_3239;

  switch_merge_3239:
  r26 = cn_var_指令;
  r27 = r26->源操作数1;
  r28 = cn_var_缓冲区;
  r29 = 生成操作数代码(r27, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, ";");
  r32 = cn_var_缓冲区;
  追加换行(r32);
  return;
}

void 生成比较指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r14, r17, r20, r23, r26, r29;
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
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
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
  enum IR操作码 r13;
  enum IR操作码 r16;
  enum IR操作码 r19;
  enum IR操作码 r22;
  enum IR操作码 r25;
  enum IR操作码 r28;

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
  r13 = cn_var_IR操作码;
  r14 = r13.相等指令;
  r15 = r12 == r14;
  if (r15) goto case_body_3247; else goto switch_check_3254;

  switch_check_3254:
  r16 = cn_var_IR操作码;
  r17 = r16.不等指令;
  r18 = r12 == r17;
  if (r18) goto case_body_3248; else goto switch_check_3255;

  switch_check_3255:
  r19 = cn_var_IR操作码;
  r20 = r19.小于指令;
  r21 = r12 == r20;
  if (r21) goto case_body_3249; else goto switch_check_3256;

  switch_check_3256:
  r22 = cn_var_IR操作码;
  r23 = r22.小于等于指令;
  r24 = r12 == r23;
  if (r24) goto case_body_3250; else goto switch_check_3257;

  switch_check_3257:
  r25 = cn_var_IR操作码;
  r26 = r25.大于指令;
  r27 = r12 == r26;
  if (r27) goto case_body_3251; else goto switch_check_3258;

  switch_check_3258:
  r28 = cn_var_IR操作码;
  r29 = r28.大于等于指令;
  r30 = r12 == r29;
  if (r30) goto case_body_3252; else goto case_default_3253;

  case_body_3247:
  r31 = cn_var_缓冲区;
  r32 = 追加字符串(r31, " == ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_body_3248:
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, " != ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_body_3249:
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, " < ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_body_3250:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, " <= ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_body_3251:
  r39 = cn_var_缓冲区;
  r40 = 追加字符串(r39, " > ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_body_3252:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, " >= ");
  goto switch_merge_3246;
  goto switch_merge_3246;

  case_default_3253:
  r43 = cn_var_缓冲区;
  r44 = 追加字符串(r43, " ? ");
  goto switch_merge_3246;

  switch_merge_3246:
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

void 生成内存指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r3, r6, r9, r12, r15, r18, r31;
  char* r24;
  char* r40;
  char* r46;
  char* r56;
  char* r62;
  char* r70;
  char* r76;
  char* r84;
  char* r90;
  char* r98;
  char* r104;
  struct IR指令* r0;
  struct 输出缓冲区* r20;
  struct IR指令* r21;
  struct 输出缓冲区* r23;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct IR指令* r28;
  struct 类型节点* r30;
  struct 输出缓冲区* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r36;
  struct IR指令* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct IR指令* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r51;
  struct IR指令* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct IR指令* r59;
  struct 输出缓冲区* r61;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r66;
  struct IR指令* r67;
  struct 输出缓冲区* r69;
  struct 输出缓冲区* r71;
  struct IR指令* r73;
  struct 输出缓冲区* r75;
  struct 输出缓冲区* r77;
  struct 输出缓冲区* r79;
  struct 输出缓冲区* r80;
  struct IR指令* r81;
  struct 输出缓冲区* r83;
  struct 输出缓冲区* r85;
  struct IR指令* r87;
  struct 输出缓冲区* r89;
  struct 输出缓冲区* r91;
  struct 输出缓冲区* r93;
  struct 输出缓冲区* r94;
  struct IR指令* r95;
  struct 输出缓冲区* r97;
  struct 输出缓冲区* r99;
  struct IR指令* r101;
  struct 输出缓冲区* r103;
  struct 输出缓冲区* r105;
  struct 输出缓冲区* r107;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r26;
  _Bool r32;
  _Bool r34;
  _Bool r42;
  _Bool r48;
  _Bool r52;
  _Bool r58;
  _Bool r64;
  _Bool r72;
  _Bool r78;
  _Bool r86;
  _Bool r92;
  _Bool r100;
  _Bool r106;
  struct IR操作数 r22;
  struct IR操作数 r29;
  struct IR操作数 r38;
  struct IR操作数 r44;
  struct IR操作数 r54;
  struct IR操作数 r60;
  struct IR操作数 r68;
  struct IR操作数 r74;
  struct IR操作数 r82;
  struct IR操作数 r88;
  struct IR操作数 r96;
  struct IR操作数 r102;
  enum IR操作码 r1;
  enum IR操作码 r2;
  enum IR操作码 r5;
  enum IR操作码 r8;
  enum IR操作码 r11;
  enum IR操作码 r14;
  enum IR操作码 r17;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = cn_var_IR操作码;
  r3 = r2.分配指令;
  r4 = r1 == r3;
  if (r4) goto case_body_3260; else goto switch_check_3267;

  switch_check_3267:
  r5 = cn_var_IR操作码;
  r6 = r5.加载指令;
  r7 = r1 == r6;
  if (r7) goto case_body_3261; else goto switch_check_3268;

  switch_check_3268:
  r8 = cn_var_IR操作码;
  r9 = r8.存储指令;
  r10 = r1 == r9;
  if (r10) goto case_body_3262; else goto switch_check_3269;

  switch_check_3269:
  r11 = cn_var_IR操作码;
  r12 = r11.移动指令;
  r13 = r1 == r12;
  if (r13) goto case_body_3263; else goto switch_check_3270;

  switch_check_3270:
  r14 = cn_var_IR操作码;
  r15 = r14.取地址指令;
  r16 = r1 == r15;
  if (r16) goto case_body_3264; else goto switch_check_3271;

  switch_check_3271:
  r17 = cn_var_IR操作码;
  r18 = r17.解引用指令;
  r19 = r1 == r18;
  if (r19) goto case_body_3265; else goto case_default_3266;

  case_body_3260:
  r20 = cn_var_缓冲区;
  追加缩进(r20, 4);
  r21 = cn_var_指令;
  r22 = r21->目标;
  r23 = cn_var_缓冲区;
  r24 = 生成操作数代码(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " = alloca(");
  r27 = cn_var_缓冲区;
  r28 = cn_var_指令;
  r29 = r28->目标;
  r30 = r29.类型信息;
  r31 = r30->大小;
  r32 = 追加整数(r27, r31);
  r33 = cn_var_缓冲区;
  r34 = 追加字符串(r33, ");");
  r35 = cn_var_缓冲区;
  追加换行(r35);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_body_3261:
  r36 = cn_var_缓冲区;
  追加缩进(r36, 4);
  r37 = cn_var_指令;
  r38 = r37->目标;
  r39 = cn_var_缓冲区;
  r40 = 生成操作数代码(r38, r39);
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, " = *");
  r43 = cn_var_指令;
  r44 = r43->源操作数1;
  r45 = cn_var_缓冲区;
  r46 = 生成操作数代码(r44, r45);
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, ";");
  r49 = cn_var_缓冲区;
  追加换行(r49);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_body_3262:
  r50 = cn_var_缓冲区;
  追加缩进(r50, 4);
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, "*");
  r53 = cn_var_指令;
  r54 = r53->源操作数2;
  r55 = cn_var_缓冲区;
  r56 = 生成操作数代码(r54, r55);
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, " = ");
  r59 = cn_var_指令;
  r60 = r59->源操作数1;
  r61 = cn_var_缓冲区;
  r62 = 生成操作数代码(r60, r61);
  r63 = cn_var_缓冲区;
  r64 = 追加字符串(r63, ";");
  r65 = cn_var_缓冲区;
  追加换行(r65);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_body_3263:
  r66 = cn_var_缓冲区;
  追加缩进(r66, 4);
  r67 = cn_var_指令;
  r68 = r67->目标;
  r69 = cn_var_缓冲区;
  r70 = 生成操作数代码(r68, r69);
  r71 = cn_var_缓冲区;
  r72 = 追加字符串(r71, " = ");
  r73 = cn_var_指令;
  r74 = r73->源操作数1;
  r75 = cn_var_缓冲区;
  r76 = 生成操作数代码(r74, r75);
  r77 = cn_var_缓冲区;
  r78 = 追加字符串(r77, ";");
  r79 = cn_var_缓冲区;
  追加换行(r79);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_body_3264:
  r80 = cn_var_缓冲区;
  追加缩进(r80, 4);
  r81 = cn_var_指令;
  r82 = r81->目标;
  r83 = cn_var_缓冲区;
  r84 = 生成操作数代码(r82, r83);
  r85 = cn_var_缓冲区;
  r86 = 追加字符串(r85, " = &");
  r87 = cn_var_指令;
  r88 = r87->源操作数1;
  r89 = cn_var_缓冲区;
  r90 = 生成操作数代码(r88, r89);
  r91 = cn_var_缓冲区;
  r92 = 追加字符串(r91, ";");
  r93 = cn_var_缓冲区;
  追加换行(r93);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_body_3265:
  r94 = cn_var_缓冲区;
  追加缩进(r94, 4);
  r95 = cn_var_指令;
  r96 = r95->目标;
  r97 = cn_var_缓冲区;
  r98 = 生成操作数代码(r96, r97);
  r99 = cn_var_缓冲区;
  r100 = 追加字符串(r99, " = *");
  r101 = cn_var_指令;
  r102 = r101->源操作数1;
  r103 = cn_var_缓冲区;
  r104 = 生成操作数代码(r102, r103);
  r105 = cn_var_缓冲区;
  r106 = 追加字符串(r105, ";");
  r107 = cn_var_缓冲区;
  追加换行(r107);
  goto switch_merge_3259;
  goto switch_merge_3259;

  case_default_3266:
  goto switch_merge_3259;
  goto switch_merge_3259;

  switch_merge_3259:
  return;
}

void 生成控制流指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r3, r6, r9, r12, r15, r23, r35, r53, r61, r71, r72, r85, r87, r88, r89, r90, r95, r96, r99, r100, r111, r112;
  char* r46;
  char* r76;
  char* r82;
  char* r98;
  char* r118;
  void* r94;
  struct IR指令* r0;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  struct IR指令* r20;
  struct 基本块* r22;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r28;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct IR指令* r32;
  struct 基本块* r34;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r40;
  struct 输出缓冲区* r41;
  struct IR指令* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r49;
  struct IR指令* r50;
  struct 基本块* r52;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r57;
  struct IR指令* r58;
  struct 基本块* r60;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r66;
  struct IR指令* r67;
  struct IR指令* r73;
  struct 输出缓冲区* r75;
  struct 输出缓冲区* r77;
  struct IR指令* r79;
  struct 输出缓冲区* r81;
  struct 输出缓冲区* r83;
  struct IR指令* r86;
  struct 输出缓冲区* r91;
  struct IR指令* r93;
  struct 输出缓冲区* r97;
  struct 输出缓冲区* r101;
  struct 输出缓冲区* r103;
  struct 输出缓冲区* r104;
  struct 输出缓冲区* r105;
  struct IR指令* r107;
  struct 输出缓冲区* r113;
  struct IR指令* r115;
  struct 输出缓冲区* r117;
  struct 输出缓冲区* r119;
  struct 输出缓冲区* r121;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r18;
  _Bool r24;
  _Bool r26;
  _Bool r30;
  _Bool r36;
  _Bool r38;
  _Bool r42;
  _Bool r48;
  _Bool r54;
  _Bool r56;
  _Bool r62;
  _Bool r64;
  _Bool r78;
  _Bool r84;
  _Bool r92;
  _Bool r102;
  _Bool r106;
  _Bool r114;
  _Bool r120;
  struct IR操作数 r21;
  struct IR操作数 r33;
  struct IR操作数 r44;
  struct IR操作数 r51;
  struct IR操作数 r59;
  struct IR操作数 r68;
  struct IR操作数 r74;
  struct IR操作数 r80;
  struct IR操作数 r108;
  struct IR操作数 r116;
  enum IR操作码 r1;
  enum IR操作码 r2;
  enum IR操作码 r5;
  enum IR操作码 r8;
  enum IR操作码 r11;
  enum IR操作码 r14;
  enum IR操作数种类 r69;
  enum IR操作数种类 r70;
  enum IR操作数种类 r109;
  enum IR操作数种类 r110;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = cn_var_IR操作码;
  r3 = r2.标签指令;
  r4 = r1 == r3;
  if (r4) goto case_body_3273; else goto switch_check_3279;

  switch_check_3279:
  r5 = cn_var_IR操作码;
  r6 = r5.跳转指令;
  r7 = r1 == r6;
  if (r7) goto case_body_3274; else goto switch_check_3280;

  switch_check_3280:
  r8 = cn_var_IR操作码;
  r9 = r8.条件跳转指令;
  r10 = r1 == r9;
  if (r10) goto case_body_3275; else goto switch_check_3281;

  switch_check_3281:
  r11 = cn_var_IR操作码;
  r12 = r11.调用指令;
  r13 = r1 == r12;
  if (r13) goto case_body_3276; else goto switch_check_3282;

  switch_check_3282:
  r14 = cn_var_IR操作码;
  r15 = r14.返回指令;
  r16 = r1 == r15;
  if (r16) goto case_body_3277; else goto case_default_3278;

  case_body_3273:
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, "L");
  r19 = cn_var_缓冲区;
  r20 = cn_var_指令;
  r21 = r20->目标;
  r22 = r21.目标块;
  r23 = r22->块编号;
  r24 = 追加整数(r19, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, ":");
  r27 = cn_var_缓冲区;
  追加换行(r27);
  goto switch_merge_3272;
  goto switch_merge_3272;

  case_body_3274:
  r28 = cn_var_缓冲区;
  追加缩进(r28, 4);
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, "goto L");
  r31 = cn_var_缓冲区;
  r32 = cn_var_指令;
  r33 = r32->目标;
  r34 = r33.目标块;
  r35 = r34->块编号;
  r36 = 追加整数(r31, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, ";");
  r39 = cn_var_缓冲区;
  追加换行(r39);
  goto switch_merge_3272;
  goto switch_merge_3272;

  case_body_3275:
  r40 = cn_var_缓冲区;
  追加缩进(r40, 4);
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "if (");
  r43 = cn_var_指令;
  r44 = r43->目标;
  r45 = cn_var_缓冲区;
  r46 = 生成操作数代码(r44, r45);
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, ") goto L");
  r49 = cn_var_缓冲区;
  r50 = cn_var_指令;
  r51 = r50->源操作数1;
  r52 = r51.目标块;
  r53 = r52->块编号;
  r54 = 追加整数(r49, r53);
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, "; else goto L");
  r57 = cn_var_缓冲区;
  r58 = cn_var_指令;
  r59 = r58->源操作数2;
  r60 = r59.目标块;
  r61 = r60->块编号;
  r62 = 追加整数(r57, r61);
  r63 = cn_var_缓冲区;
  r64 = 追加字符串(r63, ";");
  r65 = cn_var_缓冲区;
  追加换行(r65);
  goto switch_merge_3272;
  goto switch_merge_3272;

  case_body_3276:
  r66 = cn_var_缓冲区;
  追加缩进(r66, 4);
  r67 = cn_var_指令;
  r68 = r67->目标;
  r69 = r68.种类;
  r70 = cn_var_IR操作数种类;
  r71 = r70.无操作数;
  r72 = r69 != r71;
  if (r72) goto if_then_3283; else goto if_merge_3284;

  if_then_3283:
  r73 = cn_var_指令;
  r74 = r73->目标;
  r75 = cn_var_缓冲区;
  r76 = 生成操作数代码(r74, r75);
  r77 = cn_var_缓冲区;
  r78 = 追加字符串(r77, " = ");
  goto if_merge_3284;

  if_merge_3284:
  r79 = cn_var_指令;
  r80 = r79->源操作数1;
  r81 = cn_var_缓冲区;
  r82 = 生成操作数代码(r80, r81);
  r83 = cn_var_缓冲区;
  r84 = 追加字符串(r83, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3285;

  for_cond_3285:
  r85 = cn_var_i_0;
  r86 = cn_var_指令;
  r87 = r86->额外参数数量;
  r88 = r85 < r87;
  if (r88) goto for_body_3286; else goto for_exit_3288;

  for_body_3286:
  r89 = cn_var_i_0;
  r90 = r89 > 0;
  if (r90) goto if_then_3289; else goto if_merge_3290;

  for_update_3287:
  r99 = cn_var_i_0;
  r100 = r99 + 1;
  cn_var_i_0 = r100;
  goto for_cond_3285;

  for_exit_3288:
  r101 = cn_var_缓冲区;
  r102 = 追加字符串(r101, ");");
  r103 = cn_var_缓冲区;
  追加换行(r103);
  goto switch_merge_3272;
  goto switch_merge_3272;

  if_then_3289:
  r91 = cn_var_缓冲区;
  r92 = 追加字符串(r91, ", ");
  goto if_merge_3290;

  if_merge_3290:
  r93 = cn_var_指令;
  r94 = r93->额外参数;
  r95 = cn_var_i_0;
  r96 = *(void*)cn_rt_array_get_element(r94, r95, 8);
  r97 = cn_var_缓冲区;
  r98 = 生成操作数代码(r96, r97);
  goto for_update_3287;

  case_body_3277:
  r104 = cn_var_缓冲区;
  追加缩进(r104, 4);
  r105 = cn_var_缓冲区;
  r106 = 追加字符串(r105, "return");
  r107 = cn_var_指令;
  r108 = r107->目标;
  r109 = r108.种类;
  r110 = cn_var_IR操作数种类;
  r111 = r110.无操作数;
  r112 = r109 != r111;
  if (r112) goto if_then_3291; else goto if_merge_3292;

  if_then_3291:
  r113 = cn_var_缓冲区;
  r114 = 追加字符串(r113, " ");
  r115 = cn_var_指令;
  r116 = r115->目标;
  r117 = cn_var_缓冲区;
  r118 = 生成操作数代码(r116, r117);
  goto if_merge_3292;

  if_merge_3292:
  r119 = cn_var_缓冲区;
  r120 = 追加字符串(r119, ";");
  r121 = cn_var_缓冲区;
  追加换行(r121);
  goto switch_merge_3272;
  goto switch_merge_3272;

  case_default_3278:
  goto switch_merge_3272;
  goto switch_merge_3272;

  switch_merge_3272:
  return;
}

void 生成结构体指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r3, r6, r9;
  char* r15;
  char* r21;
  char* r27;
  char* r36;
  char* r42;
  char* r48;
  char* r59;
  char* r65;
  struct IR指令* r0;
  struct 输出缓冲区* r11;
  struct IR指令* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r16;
  struct IR指令* r18;
  struct 输出缓冲区* r20;
  struct 输出缓冲区* r22;
  struct 输出缓冲区* r24;
  struct IR指令* r25;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r31;
  struct 输出缓冲区* r32;
  struct IR指令* r33;
  struct 输出缓冲区* r35;
  struct 输出缓冲区* r37;
  struct IR指令* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r45;
  struct IR指令* r46;
  struct 输出缓冲区* r50;
  struct 输出缓冲区* r52;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r54;
  struct IR指令* r56;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct IR指令* r62;
  struct 输出缓冲区* r64;
  struct 输出缓冲区* r66;
  struct 输出缓冲区* r68;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  _Bool r17;
  _Bool r23;
  _Bool r28;
  _Bool r30;
  _Bool r38;
  _Bool r44;
  _Bool r49;
  _Bool r51;
  _Bool r55;
  _Bool r61;
  _Bool r67;
  struct IR操作数 r13;
  struct IR操作数 r19;
  struct IR操作数 r26;
  struct IR操作数 r34;
  struct IR操作数 r40;
  struct IR操作数 r47;
  struct IR操作数 r57;
  struct IR操作数 r63;
  enum IR操作码 r1;
  enum IR操作码 r2;
  enum IR操作码 r5;
  enum IR操作码 r8;

  entry:
  r0 = cn_var_指令;
  r1 = r0->操作码;
  r2 = cn_var_IR操作码;
  r3 = r2.成员指针指令;
  r4 = r1 == r3;
  if (r4) goto case_body_3294; else goto switch_check_3298;

  switch_check_3298:
  r5 = cn_var_IR操作码;
  r6 = r5.成员访问指令;
  r7 = r1 == r6;
  if (r7) goto case_body_3295; else goto switch_check_3299;

  switch_check_3299:
  r8 = cn_var_IR操作码;
  r9 = r8.结构体初始化指令;
  r10 = r1 == r9;
  if (r10) goto case_body_3296; else goto case_default_3297;

  case_body_3294:
  r11 = cn_var_缓冲区;
  追加缩进(r11, 4);
  r12 = cn_var_指令;
  r13 = r12->目标;
  r14 = cn_var_缓冲区;
  r15 = 生成操作数代码(r13, r14);
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, " = &");
  r18 = cn_var_指令;
  r19 = r18->源操作数1;
  r20 = cn_var_缓冲区;
  r21 = 生成操作数代码(r19, r20);
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, "->");
  r24 = cn_var_缓冲区;
  r25 = cn_var_指令;
  r26 = r25->源操作数2;
  r27 = r26.符号名;
  r28 = 追加字符串(r24, r27);
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, ";");
  r31 = cn_var_缓冲区;
  追加换行(r31);
  goto switch_merge_3293;
  goto switch_merge_3293;

  case_body_3295:
  r32 = cn_var_缓冲区;
  追加缩进(r32, 4);
  r33 = cn_var_指令;
  r34 = r33->目标;
  r35 = cn_var_缓冲区;
  r36 = 生成操作数代码(r34, r35);
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, " = ");
  r39 = cn_var_指令;
  r40 = r39->源操作数1;
  r41 = cn_var_缓冲区;
  r42 = 生成操作数代码(r40, r41);
  r43 = cn_var_缓冲区;
  r44 = 追加字符串(r43, ".");
  r45 = cn_var_缓冲区;
  r46 = cn_var_指令;
  r47 = r46->源操作数2;
  r48 = r47.符号名;
  r49 = 追加字符串(r45, r48);
  r50 = cn_var_缓冲区;
  r51 = 追加字符串(r50, ";");
  r52 = cn_var_缓冲区;
  追加换行(r52);
  goto switch_merge_3293;
  goto switch_merge_3293;

  case_body_3296:
  r53 = cn_var_缓冲区;
  追加缩进(r53, 4);
  r54 = cn_var_缓冲区;
  r55 = 追加字符串(r54, "memset(&");
  r56 = cn_var_指令;
  r57 = r56->目标;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, ", 0, sizeof(");
  r62 = cn_var_指令;
  r63 = r62->目标;
  r64 = cn_var_缓冲区;
  r65 = 生成操作数代码(r63, r64);
  r66 = cn_var_缓冲区;
  r67 = 追加字符串(r66, "));");
  r68 = cn_var_缓冲区;
  追加换行(r68);
  goto switch_merge_3293;
  goto switch_merge_3293;

  case_default_3297:
  goto switch_merge_3293;
  goto switch_merge_3293;

  switch_merge_3293:
  return;
}

void 生成指令代码(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r5, r8, r11, r14, r17, r20, r23, r26, r29, r32, r35, r38, r41, r44, r47, r50, r53, r56, r59, r62, r65, r68, r71, r74, r77, r80, r83, r86, r89, r92, r95, r98, r101, r104, r107;
  char* r125;
  char* r131;
  char* r137;
  char* r143;
  struct IR指令* r0;
  struct IR指令* r2;
  struct IR指令* r109;
  struct 输出缓冲区* r110;
  struct IR指令* r111;
  struct 输出缓冲区* r112;
  struct IR指令* r113;
  struct 输出缓冲区* r114;
  struct IR指令* r115;
  struct 输出缓冲区* r116;
  struct IR指令* r117;
  struct 输出缓冲区* r118;
  struct IR指令* r119;
  struct 输出缓冲区* r120;
  struct 输出缓冲区* r121;
  struct IR指令* r122;
  struct 输出缓冲区* r124;
  struct 输出缓冲区* r126;
  struct IR指令* r128;
  struct 输出缓冲区* r130;
  struct 输出缓冲区* r132;
  struct IR指令* r134;
  struct 输出缓冲区* r136;
  struct 输出缓冲区* r138;
  struct IR指令* r140;
  struct 输出缓冲区* r142;
  struct 输出缓冲区* r144;
  struct 输出缓冲区* r146;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r33;
  _Bool r36;
  _Bool r39;
  _Bool r42;
  _Bool r45;
  _Bool r48;
  _Bool r51;
  _Bool r54;
  _Bool r57;
  _Bool r60;
  _Bool r63;
  _Bool r66;
  _Bool r69;
  _Bool r72;
  _Bool r75;
  _Bool r78;
  _Bool r81;
  _Bool r84;
  _Bool r87;
  _Bool r90;
  _Bool r93;
  _Bool r96;
  _Bool r99;
  _Bool r102;
  _Bool r105;
  _Bool r108;
  _Bool r127;
  _Bool r133;
  _Bool r139;
  _Bool r145;
  struct IR操作数 r123;
  struct IR操作数 r129;
  struct IR操作数 r135;
  struct IR操作数 r141;
  enum IR操作码 r3;
  enum IR操作码 r4;
  enum IR操作码 r7;
  enum IR操作码 r10;
  enum IR操作码 r13;
  enum IR操作码 r16;
  enum IR操作码 r19;
  enum IR操作码 r22;
  enum IR操作码 r25;
  enum IR操作码 r28;
  enum IR操作码 r31;
  enum IR操作码 r34;
  enum IR操作码 r37;
  enum IR操作码 r40;
  enum IR操作码 r43;
  enum IR操作码 r46;
  enum IR操作码 r49;
  enum IR操作码 r52;
  enum IR操作码 r55;
  enum IR操作码 r58;
  enum IR操作码 r61;
  enum IR操作码 r64;
  enum IR操作码 r67;
  enum IR操作码 r70;
  enum IR操作码 r73;
  enum IR操作码 r76;
  enum IR操作码 r79;
  enum IR操作码 r82;
  enum IR操作码 r85;
  enum IR操作码 r88;
  enum IR操作码 r91;
  enum IR操作码 r94;
  enum IR操作码 r97;
  enum IR操作码 r100;
  enum IR操作码 r103;
  enum IR操作码 r106;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_3300; else goto if_merge_3301;

  if_then_3300:
  return;
  goto if_merge_3301;

  if_merge_3301:
  r2 = cn_var_指令;
  r3 = r2->操作码;
  r4 = cn_var_IR操作码;
  r5 = r4.加法指令;
  r6 = r3 == r5;
  if (r6) goto case_body_3303; else goto switch_check_3339;

  switch_check_3339:
  r7 = cn_var_IR操作码;
  r8 = r7.减法指令;
  r9 = r3 == r8;
  if (r9) goto case_body_3304; else goto switch_check_3340;

  switch_check_3340:
  r10 = cn_var_IR操作码;
  r11 = r10.乘法指令;
  r12 = r3 == r11;
  if (r12) goto case_body_3305; else goto switch_check_3341;

  switch_check_3341:
  r13 = cn_var_IR操作码;
  r14 = r13.除法指令;
  r15 = r3 == r14;
  if (r15) goto case_body_3306; else goto switch_check_3342;

  switch_check_3342:
  r16 = cn_var_IR操作码;
  r17 = r16.取模指令;
  r18 = r3 == r17;
  if (r18) goto case_body_3307; else goto switch_check_3343;

  switch_check_3343:
  r19 = cn_var_IR操作码;
  r20 = r19.与指令;
  r21 = r3 == r20;
  if (r21) goto case_body_3308; else goto switch_check_3344;

  switch_check_3344:
  r22 = cn_var_IR操作码;
  r23 = r22.或指令;
  r24 = r3 == r23;
  if (r24) goto case_body_3309; else goto switch_check_3345;

  switch_check_3345:
  r25 = cn_var_IR操作码;
  r26 = r25.异或指令;
  r27 = r3 == r26;
  if (r27) goto case_body_3310; else goto switch_check_3346;

  switch_check_3346:
  r28 = cn_var_IR操作码;
  r29 = r28.左移指令;
  r30 = r3 == r29;
  if (r30) goto case_body_3311; else goto switch_check_3347;

  switch_check_3347:
  r31 = cn_var_IR操作码;
  r32 = r31.右移指令;
  r33 = r3 == r32;
  if (r33) goto case_body_3312; else goto switch_check_3348;

  switch_check_3348:
  r34 = cn_var_IR操作码;
  r35 = r34.负号指令;
  r36 = r3 == r35;
  if (r36) goto case_body_3313; else goto switch_check_3349;

  switch_check_3349:
  r37 = cn_var_IR操作码;
  r38 = r37.取反指令;
  r39 = r3 == r38;
  if (r39) goto case_body_3314; else goto switch_check_3350;

  switch_check_3350:
  r40 = cn_var_IR操作码;
  r41 = r40.逻辑非指令;
  r42 = r3 == r41;
  if (r42) goto case_body_3315; else goto switch_check_3351;

  switch_check_3351:
  r43 = cn_var_IR操作码;
  r44 = r43.相等指令;
  r45 = r3 == r44;
  if (r45) goto case_body_3316; else goto switch_check_3352;

  switch_check_3352:
  r46 = cn_var_IR操作码;
  r47 = r46.不等指令;
  r48 = r3 == r47;
  if (r48) goto case_body_3317; else goto switch_check_3353;

  switch_check_3353:
  r49 = cn_var_IR操作码;
  r50 = r49.小于指令;
  r51 = r3 == r50;
  if (r51) goto case_body_3318; else goto switch_check_3354;

  switch_check_3354:
  r52 = cn_var_IR操作码;
  r53 = r52.小于等于指令;
  r54 = r3 == r53;
  if (r54) goto case_body_3319; else goto switch_check_3355;

  switch_check_3355:
  r55 = cn_var_IR操作码;
  r56 = r55.大于指令;
  r57 = r3 == r56;
  if (r57) goto case_body_3320; else goto switch_check_3356;

  switch_check_3356:
  r58 = cn_var_IR操作码;
  r59 = r58.大于等于指令;
  r60 = r3 == r59;
  if (r60) goto case_body_3321; else goto switch_check_3357;

  switch_check_3357:
  r61 = cn_var_IR操作码;
  r62 = r61.分配指令;
  r63 = r3 == r62;
  if (r63) goto case_body_3322; else goto switch_check_3358;

  switch_check_3358:
  r64 = cn_var_IR操作码;
  r65 = r64.加载指令;
  r66 = r3 == r65;
  if (r66) goto case_body_3323; else goto switch_check_3359;

  switch_check_3359:
  r67 = cn_var_IR操作码;
  r68 = r67.存储指令;
  r69 = r3 == r68;
  if (r69) goto case_body_3324; else goto switch_check_3360;

  switch_check_3360:
  r70 = cn_var_IR操作码;
  r71 = r70.移动指令;
  r72 = r3 == r71;
  if (r72) goto case_body_3325; else goto switch_check_3361;

  switch_check_3361:
  r73 = cn_var_IR操作码;
  r74 = r73.取地址指令;
  r75 = r3 == r74;
  if (r75) goto case_body_3326; else goto switch_check_3362;

  switch_check_3362:
  r76 = cn_var_IR操作码;
  r77 = r76.解引用指令;
  r78 = r3 == r77;
  if (r78) goto case_body_3327; else goto switch_check_3363;

  switch_check_3363:
  r79 = cn_var_IR操作码;
  r80 = r79.标签指令;
  r81 = r3 == r80;
  if (r81) goto case_body_3328; else goto switch_check_3364;

  switch_check_3364:
  r82 = cn_var_IR操作码;
  r83 = r82.跳转指令;
  r84 = r3 == r83;
  if (r84) goto case_body_3329; else goto switch_check_3365;

  switch_check_3365:
  r85 = cn_var_IR操作码;
  r86 = r85.条件跳转指令;
  r87 = r3 == r86;
  if (r87) goto case_body_3330; else goto switch_check_3366;

  switch_check_3366:
  r88 = cn_var_IR操作码;
  r89 = r88.调用指令;
  r90 = r3 == r89;
  if (r90) goto case_body_3331; else goto switch_check_3367;

  switch_check_3367:
  r91 = cn_var_IR操作码;
  r92 = r91.返回指令;
  r93 = r3 == r92;
  if (r93) goto case_body_3332; else goto switch_check_3368;

  switch_check_3368:
  r94 = cn_var_IR操作码;
  r95 = r94.成员指针指令;
  r96 = r3 == r95;
  if (r96) goto case_body_3333; else goto switch_check_3369;

  switch_check_3369:
  r97 = cn_var_IR操作码;
  r98 = r97.成员访问指令;
  r99 = r3 == r98;
  if (r99) goto case_body_3334; else goto switch_check_3370;

  switch_check_3370:
  r100 = cn_var_IR操作码;
  r101 = r100.结构体初始化指令;
  r102 = r3 == r101;
  if (r102) goto case_body_3335; else goto switch_check_3371;

  switch_check_3371:
  r103 = cn_var_IR操作码;
  r104 = r103.PHI指令;
  r105 = r3 == r104;
  if (r105) goto case_body_3336; else goto switch_check_3372;

  switch_check_3372:
  r106 = cn_var_IR操作码;
  r107 = r106.选择指令;
  r108 = r3 == r107;
  if (r108) goto case_body_3337; else goto case_default_3338;

  case_body_3303:
  goto switch_merge_3302;

  case_body_3304:
  goto switch_merge_3302;

  case_body_3305:
  goto switch_merge_3302;

  case_body_3306:
  goto switch_merge_3302;

  case_body_3307:
  goto switch_merge_3302;

  case_body_3308:
  goto switch_merge_3302;

  case_body_3309:
  goto switch_merge_3302;

  case_body_3310:
  goto switch_merge_3302;

  case_body_3311:
  goto switch_merge_3302;

  case_body_3312:
  r109 = cn_var_指令;
  r110 = cn_var_缓冲区;
  生成算术指令(r109, r110);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3313:
  goto switch_merge_3302;

  case_body_3314:
  goto switch_merge_3302;

  case_body_3315:
  r111 = cn_var_指令;
  r112 = cn_var_缓冲区;
  生成一元指令(r111, r112);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3316:
  goto switch_merge_3302;

  case_body_3317:
  goto switch_merge_3302;

  case_body_3318:
  goto switch_merge_3302;

  case_body_3319:
  goto switch_merge_3302;

  case_body_3320:
  goto switch_merge_3302;

  case_body_3321:
  r113 = cn_var_指令;
  r114 = cn_var_缓冲区;
  生成比较指令(r113, r114);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3322:
  goto switch_merge_3302;

  case_body_3323:
  goto switch_merge_3302;

  case_body_3324:
  goto switch_merge_3302;

  case_body_3325:
  goto switch_merge_3302;

  case_body_3326:
  goto switch_merge_3302;

  case_body_3327:
  r115 = cn_var_指令;
  r116 = cn_var_缓冲区;
  生成内存指令(r115, r116);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3328:
  goto switch_merge_3302;

  case_body_3329:
  goto switch_merge_3302;

  case_body_3330:
  goto switch_merge_3302;

  case_body_3331:
  goto switch_merge_3302;

  case_body_3332:
  r117 = cn_var_指令;
  r118 = cn_var_缓冲区;
  生成控制流指令(r117, r118);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3333:
  goto switch_merge_3302;

  case_body_3334:
  goto switch_merge_3302;

  case_body_3335:
  r119 = cn_var_指令;
  r120 = cn_var_缓冲区;
  生成结构体指令(r119, r120);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3336:
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_body_3337:
  r121 = cn_var_缓冲区;
  追加缩进(r121, 4);
  r122 = cn_var_指令;
  r123 = r122->目标;
  r124 = cn_var_缓冲区;
  r125 = 生成操作数代码(r123, r124);
  r126 = cn_var_缓冲区;
  r127 = 追加字符串(r126, " = ");
  r128 = cn_var_指令;
  r129 = r128->目标;
  r130 = cn_var_缓冲区;
  r131 = 生成操作数代码(r129, r130);
  r132 = cn_var_缓冲区;
  r133 = 追加字符串(r132, " ? ");
  r134 = cn_var_指令;
  r135 = r134->源操作数1;
  r136 = cn_var_缓冲区;
  r137 = 生成操作数代码(r135, r136);
  r138 = cn_var_缓冲区;
  r139 = 追加字符串(r138, " : ");
  r140 = cn_var_指令;
  r141 = r140->源操作数2;
  r142 = cn_var_缓冲区;
  r143 = 生成操作数代码(r141, r142);
  r144 = cn_var_缓冲区;
  r145 = 追加字符串(r144, ";");
  r146 = cn_var_缓冲区;
  追加换行(r146);
  goto switch_merge_3302;
  goto switch_merge_3302;

  case_default_3338:
  goto switch_merge_3302;
  goto switch_merge_3302;

  switch_merge_3302:
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
  if (r1) goto if_then_3373; else goto if_merge_3374;

  if_then_3373:
  return;
  goto if_merge_3374;

  if_merge_3374:
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
  goto while_cond_3375;

  while_cond_3375:
  r13 = cn_var_指令_0;
  if (r13) goto while_body_3376; else goto while_exit_3377;

  while_body_3376:
  r14 = cn_var_指令_0;
  r15 = cn_var_缓冲区;
  生成指令代码(r14, r15);
  r16 = cn_var_指令_0;
  r17 = r16->下一条;
  cn_var_指令_0 = r17;
  goto while_cond_3375;

  while_exit_3377:
  return;
}

void 生成局部变量声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r2, r4, r5, r8, r9, r19, r24, r25;
  char* r14;
  void* r7;
  struct IR函数* r0;
  struct IR函数* r3;
  struct IR函数* r6;
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
  if (r1) goto if_then_3378; else goto if_merge_3379;

  if_then_3378:
  return;
  goto if_merge_3379;

  if_merge_3379:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3380;

  for_cond_3380:
  r2 = cn_var_i_0;
  r3 = cn_var_函数指针;
  r4 = r3->局部变量数量;
  r5 = r2 < r4;
  if (r5) goto for_body_3381; else goto for_exit_3383;

  for_body_3381:
  struct IR操作数 cn_var_局部变量_1;
  r6 = cn_var_函数指针;
  r7 = r6->局部变量;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  cn_var_局部变量_1 = r9;
  r10 = cn_var_缓冲区;
  追加缩进(r10, 4);
  r11 = cn_var_局部变量_1;
  r12 = r11.类型信息;
  r13 = cn_var_缓冲区;
  r14 = 类型到C类型(r12, r13);
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
  goto for_update_3382;

  for_update_3382:
  r24 = cn_var_i_0;
  r25 = r24 + 1;
  cn_var_i_0 = r25;
  goto for_cond_3380;

  for_exit_3383:
  return;
}

void 生成函数代码(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r14, r16, r17, r18, r19, r24, r25, r27, r33, r35, r36;
  char* r5;
  char* r10;
  char* r29;
  void* r23;
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
  if (r1) goto if_then_3384; else goto if_merge_3385;

  if_then_3384:
  return;
  goto if_merge_3385;

  if_merge_3385:
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
  goto for_cond_3386;

  for_cond_3386:
  r14 = cn_var_i_0;
  r15 = cn_var_函数指针;
  r16 = r15->参数数量;
  r17 = r14 < r16;
  if (r17) goto for_body_3387; else goto for_exit_3389;

  for_body_3387:
  r18 = cn_var_i_0;
  r19 = r18 > 0;
  if (r19) goto if_then_3390; else goto if_merge_3391;

  for_update_3388:
  r35 = cn_var_i_0;
  r36 = r35 + 1;
  cn_var_i_0 = r36;
  goto for_cond_3386;

  for_exit_3389:
  if (0) goto if_then_3392; else goto if_merge_3393;

  if_then_3390:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, ", ");
  goto if_merge_3391;

  if_merge_3391:
  struct IR操作数 cn_var_参数_1;
  r22 = cn_var_函数指针;
  r23 = r22->参数列表;
  r24 = cn_var_i_0;
  r25 = *(void*)cn_rt_array_get_element(r23, r24, 8);
  cn_var_参数_1 = r25;
  r26 = cn_var_参数_1;
  r27 = r26.类型信息;
  r28 = cn_var_缓冲区;
  r29 = 类型到C类型(r27, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, " p");
  r32 = cn_var_缓冲区;
  r33 = cn_var_i_0;
  r34 = 追加整数(r32, r33);
  goto for_update_3388;

  if_then_3392:
  r37 = cn_var_缓冲区;
  r38 = 追加字符串(r37, "void");
  goto if_merge_3393;

  if_merge_3393:
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
  goto while_cond_3394;

  while_cond_3394:
  r47 = cn_var_块_2;
  if (r47) goto while_body_3395; else goto while_exit_3396;

  while_body_3395:
  r48 = cn_var_块_2;
  r49 = cn_var_缓冲区;
  生成基本块代码(r48, r49);
  r50 = cn_var_块_2;
  r51 = r50->下一个;
  cn_var_块_2 = r51;
  goto while_cond_3394;

  while_exit_3396:
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
  if (r2) goto if_then_3397; else goto if_merge_3398;

  if_then_3397:
  r3 = cn_var_模块;
  r4 = cn_var_默认缓冲区大小;
  r5 = 创建输出缓冲区(r4);
  r6 = cn_var_默认缓冲区大小;
  r7 = 创建输出缓冲区(r6);
  r8 = cn_var_默认缓冲区大小;
  r9 = 创建输出缓冲区(r8);
  r10 = cn_var_选项;
  goto if_merge_3398;

  if_merge_3398:
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
  if (r0) goto if_then_3399; else goto if_merge_3400;

  if_then_3399:
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
  goto if_merge_3400;

  if_merge_3400:
  return;
}

struct 代码生成选项 获取默认生成选项() {
  long long r0, r3;
  void* r1;
  struct 代码生成选项* r2;

  entry:
  struct 代码生成选项* cn_var_选项_0;
  r0 = sizeof(struct 代码生成选项);
  r1 = 分配清零内存(1, r0);
  cn_var_选项_0 = (struct 代码生成选项*)0;
  r2 = cn_var_选项_0;
  r3 = *(void*)cn_rt_array_get_element(r2, 0, 8);
  return r3;
}

void 格式化整数(char* cn_var_缓冲区, long long cn_var_缓冲区大小, long long cn_var_数值) {
  long long r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r31, r32, r33, r35, r36;
  char* r0;
  char* r1;
  char* r19;
  char* r30;
  char* r34;
  void* r2;

  entry:
  cn_var_数值 = 0;
  if (0) goto if_then_3401; else goto if_merge_3402;

  if_then_3401:
  r0 = cn_var_缓冲区;
    { long long _tmp_i6 = 48; cn_rt_array_set_element(r0, 0, &_tmp_i6, 8); }
  r1 = cn_var_缓冲区;
    { long long _tmp_i7 = 0; cn_rt_array_set_element(r1, 1, &_tmp_i7, 8); }
  return;
  goto if_merge_3402;

  if_merge_3402:
  void cn_var_临时_0;
  r2 = 分配内存(32);
  cn_var_临时_0 = r2;
  long long cn_var_索引_1;
  cn_var_索引_1 = 0;
  void cn_var_负数_2;
  cn_var_负数_2 = 0;
  r3 = cn_var_数值;
  r4 = r3 < 0;
  if (r4) goto if_then_3403; else goto if_merge_3404;

  if_then_3403:
  cn_var_负数_2 = 1;
  r5 = cn_var_数值;
  r6 = -r5;
  cn_var_数值 = r6;
  goto if_merge_3404;

  if_merge_3404:
  goto while_cond_3405;

  while_cond_3405:
  r7 = cn_var_数值;
  r8 = r7 > 0;
  if (r8) goto while_body_3406; else goto while_exit_3407;

  while_body_3406:
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
  goto while_cond_3405;

  while_exit_3407:
  long long cn_var_输出索引_3;
  cn_var_输出索引_3 = 0;
  r18 = cn_var_负数_2;
  if (r18) goto if_then_3408; else goto if_merge_3409;

  if_then_3408:
  r19 = cn_var_缓冲区;
  r20 = cn_var_输出索引_3;
    { long long _tmp_i8 = 45; cn_rt_array_set_element(r19, r20, &_tmp_i8, 8); }
  r21 = cn_var_输出索引_3;
  r22 = r21 + 1;
  cn_var_输出索引_3 = r22;
  goto if_merge_3409;

  if_merge_3409:
  goto while_cond_3410;

  while_cond_3410:
  r23 = cn_var_索引_1;
  r24 = r23 > 0;
  if (r24) goto while_body_3411; else goto while_exit_3412;

  while_body_3411:
  r25 = cn_var_索引_1;
  r26 = r25 - 1;
  cn_var_索引_1 = r26;
  r27 = cn_var_临时_0;
  r28 = cn_var_索引_1;
  r29 = *(void*)cn_rt_array_get_element(r27, r28, 8);
  r30 = cn_var_缓冲区;
  r31 = cn_var_输出索引_3;
    { long long _tmp_r16 = r29; cn_rt_array_set_element(r30, r31, &_tmp_r16, 8); }
  r32 = cn_var_输出索引_3;
  r33 = r32 + 1;
  cn_var_输出索引_3 = r33;
  goto while_cond_3410;

  while_exit_3412:
  r34 = cn_var_缓冲区;
  r35 = cn_var_输出索引_3;
    { long long _tmp_i9 = 0; cn_rt_array_set_element(r34, r35, &_tmp_i9, 8); }
  r36 = cn_var_临时_0;
  释放内存(r36);
  return;
}

void 追加小数(struct 输出缓冲区* cn_var_缓冲区, double cn_var_数值) {
  long long r2, r3, r4, r6, r11;
  struct 输出缓冲区* r5;
  struct 输出缓冲区* r8;
  struct 输出缓冲区* r10;
  _Bool r7;
  _Bool r9;
  _Bool r12;
  double r0;
  double r1;

  entry:
  long long cn_var_整数部分_0;
  r0 = cn_var_数值;
  cn_var_整数部分_0 = (long long)cn_var_数值;
  long long cn_var_小数部分_1;
  r1 = cn_var_数值;
  r2 = cn_var_整数部分_0;
  r3 = r1 - r2;
  r4 = r3 * 1000000;
  cn_var_小数部分_1 = (long long)((cn_var_数值 - cn_var_整数部分) * 1000000);
  r5 = cn_var_缓冲区;
  r6 = cn_var_整数部分_0;
  r7 = 追加整数(r5, r6);
  r8 = cn_var_缓冲区;
  r9 = 追加字符串(r8, ".");
  r10 = cn_var_缓冲区;
  r11 = cn_var_小数部分_1;
  r12 = 追加整数(r10, r11);
  return;
}

