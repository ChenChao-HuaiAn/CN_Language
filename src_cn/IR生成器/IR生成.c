#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
struct 作用域;
struct 符号表管理器;
struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};
struct 类型节点;
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
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
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
  复制字符串(r5);
  return;
}

char* 生成块名称(char* cn_var_提示) {
  long long r2, r3, r4;
  char* r1;
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
  复制字符串(r5);
  return;
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
  if (r2) goto logic_merge_2868; else goto logic_rhs_2867;

  if_then_2865:
  r6 = -1;
  return r6;
  goto if_merge_2866;

  if_merge_2866:
  long long cn_var_编号_0;
  r7 = cn_var_上下文;
  r8 = r7->当前函数;
  r9 = r8->下个寄存器编号;
  cn_var_编号_0 = r9;
  r10 = cn_var_编号_0;
  r11 = r10 + 1;
  r12 = cn_var_编号_0;
  return r12;

  logic_rhs_2867:
  r3 = cn_var_上下文;
  r4 = r3->当前函数;
  r5 = !r4;
  goto logic_merge_2868;

  logic_merge_2868:
  if (r5) goto if_then_2865; else goto if_merge_2866;
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
  if (r3) goto logic_merge_2874; else goto logic_rhs_2873;

  if_then_2869:
  return;
  goto if_merge_2870;

  if_merge_2870:
  r9 = cn_var_上下文;
  r10 = r9->当前块;
  r11 = cn_var_指令;
  添加指令(r10, r11);

  logic_rhs_2871:
  r7 = cn_var_指令;
  r8 = !r7;
  goto logic_merge_2872;

  logic_merge_2872:
  if (r8) goto if_then_2869; else goto if_merge_2870;

  logic_rhs_2873:
  r4 = cn_var_上下文;
  r5 = r4->当前块;
  r6 = !r5;
  goto logic_merge_2874;

  logic_merge_2874:
  if (r6) goto logic_merge_2872; else goto logic_rhs_2871;
  return;
}

void 切换基本块(struct IR生成上下文* cn_var_上下文, struct 基本块* cn_var_块) {
  struct IR生成上下文* r0;
  struct 基本块* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_2875; else goto if_merge_2876;

  if_then_2875:
  r1 = cn_var_块;
  goto if_merge_2876;

  if_merge_2876:
  return;
}

char* 查找局部变量唯一名(struct IR生成上下文* cn_var_上下文, char* cn_var_原始名称) {
  long long r0, r2, r4, r11;
  char* r3;
  char* r9;
  char* r10;
  char* r13;
  struct IR生成上下文* r1;
  struct IR生成上下文* r5;
  struct 局部变量映射* r6;
  struct 局部变量映射* r7;
  struct 局部变量映射* r8;
  struct 局部变量映射* r12;
  struct 局部变量映射* r14;
  struct 局部变量映射* r15;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2880; else goto logic_rhs_2879;

  if_then_2877:
  return 0;
  goto if_merge_2878;

  if_merge_2878:
  struct 局部变量映射* cn_var_映射_0;
  r5 = cn_var_上下文;
  r6 = r5->局部变量映射表;
  cn_var_映射_0 = r6;
  goto while_cond_2881;

  logic_rhs_2879:
  r3 = cn_var_原始名称;
  r4 = !r3;
  goto logic_merge_2880;

  logic_merge_2880:
  if (r4) goto if_then_2877; else goto if_merge_2878;

  while_cond_2881:
  r7 = cn_var_映射_0;
  if (r7) goto while_body_2882; else goto while_exit_2883;

  while_body_2882:
  r8 = cn_var_映射_0;
  r9 = r8->原始名称;
  r10 = cn_var_原始名称;
  比较字符串(r9, r10);
  r11 = /* NONE */ == 0;
  if (r11) goto if_then_2884; else goto if_merge_2885;

  while_exit_2883:
  return 0;

  if_then_2884:
  r12 = cn_var_映射_0;
  r13 = r12->唯一名称;
  复制字符串(r13);
  return;
  goto if_merge_2885;

  if_merge_2885:
  r14 = cn_var_映射_0;
  r15 = r14->下一个;
  cn_var_映射_0 = r15;
  goto while_cond_2881;
  return NULL;
}

void 添加局部变量映射(struct IR生成上下文* cn_var_上下文, char* cn_var_原始名称, char* cn_var_唯一名称) {
  long long r0, r1, r3, r5, r7, r8;
  char* r4;
  char* r6;
  char* r10;
  char* r11;
  struct IR生成上下文* r2;
  struct 局部变量映射* r9;
  struct IR生成上下文* r12;
  struct 局部变量映射* r13;
  struct 局部变量映射* r14;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_2891; else goto logic_rhs_2890;

  if_then_2886:
  return;
  goto if_merge_2887;

  if_merge_2887:
  struct 局部变量映射* cn_var_映射_0;
  r8 = sizeof(struct 局部变量映射);
  分配内存(r8);
  cn_var_映射_0 = /* NONE */;
  r9 = cn_var_映射_0;
  if (r9) goto if_then_2892; else goto if_merge_2893;

  logic_rhs_2888:
  r6 = cn_var_唯一名称;
  r7 = !r6;
  goto logic_merge_2889;

  logic_merge_2889:
  if (r7) goto if_then_2886; else goto if_merge_2887;

  logic_rhs_2890:
  r4 = cn_var_原始名称;
  r5 = !r4;
  goto logic_merge_2891;

  logic_merge_2891:
  if (r5) goto logic_merge_2889; else goto logic_rhs_2888;

  if_then_2892:
  r10 = cn_var_原始名称;
  复制字符串(r10);
  r11 = cn_var_唯一名称;
  复制字符串(r11);
  r12 = cn_var_上下文;
  r13 = r12->局部变量映射表;
  r14 = cn_var_映射_0;
  goto if_merge_2893;

  if_merge_2893:
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
  if (r1) goto if_then_2894; else goto if_merge_2895;

  if_then_2894:
  return;
  goto if_merge_2895;

  if_merge_2895:
  struct 局部变量映射* cn_var_映射_0;
  r2 = cn_var_上下文;
  r3 = r2->局部变量映射表;
  cn_var_映射_0 = r3;
  goto while_cond_2896;

  while_cond_2896:
  r4 = cn_var_映射_0;
  if (r4) goto while_body_2897; else goto while_exit_2898;

  while_body_2897:
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
  goto while_cond_2896;

  while_exit_2898:
  return;
}

struct IR生成上下文* 创建IR生成上下文() {
  long long r0;
  struct IR生成上下文* r1;
  struct IR生成上下文* r2;

  entry:
  struct IR生成上下文* cn_var_上下文_0;
  r0 = sizeof(struct IR生成上下文);
  分配内存(r0);
  cn_var_上下文_0 = /* NONE */;
  r1 = cn_var_上下文_0;
  if (r1) goto if_then_2899; else goto if_merge_2900;

  if_then_2899:
  创建IR模块();
  goto if_merge_2900;

  if_merge_2900:
  r2 = cn_var_上下文_0;
  return r2;
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
  if (r1) goto if_then_2901; else goto if_merge_2902;

  if_then_2901:
  return;
  goto if_merge_2902;

  if_merge_2902:
  r2 = cn_var_上下文;
  释放局部变量映射表(r2);
  r3 = cn_var_上下文;
  r4 = r3->模块;
  if (r4) goto if_then_2903; else goto if_merge_2904;

  if_then_2903:
  r5 = cn_var_上下文;
  r6 = r5->模块;
  释放IR模块(r6);
  goto if_merge_2904;

  if_merge_2904:
  r7 = cn_var_上下文;
  释放内存(r7);
  return;
}

enum IR操作码 二元运算符转IR(enum 二元运算符 cn_var_运算符) {
  long long r1, r3, r5, r7, r9, r11, r13, r15, r17, r19, r21, r23, r25, r27, r29, r31;
  _Bool r2;
  _Bool r4;
  _Bool r6;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r20;
  _Bool r22;
  _Bool r24;
  _Bool r26;
  _Bool r28;
  _Bool r30;
  _Bool r32;
  enum 二元运算符 r0;

  entry:
  r0 = cn_var_运算符;
  r1 = cn_var_加法运算符;
  r2 = r0 == r1;
  if (r2) goto case_body_2906; else goto switch_check_2923;

  switch_check_2923:
  r3 = cn_var_减法运算符;
  r4 = r0 == r3;
  if (r4) goto case_body_2907; else goto switch_check_2924;

  switch_check_2924:
  r5 = cn_var_乘法运算符;
  r6 = r0 == r5;
  if (r6) goto case_body_2908; else goto switch_check_2925;

  switch_check_2925:
  r7 = cn_var_除法运算符;
  r8 = r0 == r7;
  if (r8) goto case_body_2909; else goto switch_check_2926;

  switch_check_2926:
  r9 = cn_var_取模运算符;
  r10 = r0 == r9;
  if (r10) goto case_body_2910; else goto switch_check_2927;

  switch_check_2927:
  r11 = cn_var_相等运算符;
  r12 = r0 == r11;
  if (r12) goto case_body_2911; else goto switch_check_2928;

  switch_check_2928:
  r13 = cn_var_不等运算符;
  r14 = r0 == r13;
  if (r14) goto case_body_2912; else goto switch_check_2929;

  switch_check_2929:
  r15 = cn_var_小于运算符;
  r16 = r0 == r15;
  if (r16) goto case_body_2913; else goto switch_check_2930;

  switch_check_2930:
  r17 = cn_var_大于运算符;
  r18 = r0 == r17;
  if (r18) goto case_body_2914; else goto switch_check_2931;

  switch_check_2931:
  r19 = cn_var_小于等于运算符;
  r20 = r0 == r19;
  if (r20) goto case_body_2915; else goto switch_check_2932;

  switch_check_2932:
  r21 = cn_var_大于等于运算符;
  r22 = r0 == r21;
  if (r22) goto case_body_2916; else goto switch_check_2933;

  switch_check_2933:
  r23 = cn_var_位与运算符;
  r24 = r0 == r23;
  if (r24) goto case_body_2917; else goto switch_check_2934;

  switch_check_2934:
  r25 = cn_var_位或运算符;
  r26 = r0 == r25;
  if (r26) goto case_body_2918; else goto switch_check_2935;

  switch_check_2935:
  r27 = cn_var_异或运算符;
  r28 = r0 == r27;
  if (r28) goto case_body_2919; else goto switch_check_2936;

  switch_check_2936:
  r29 = cn_var_左移运算符;
  r30 = r0 == r29;
  if (r30) goto case_body_2920; else goto switch_check_2937;

  switch_check_2937:
  r31 = cn_var_右移运算符;
  r32 = r0 == r31;
  if (r32) goto case_body_2921; else goto case_default_2922;

  case_body_2906:
  return 0;
  goto switch_merge_2905;

  case_body_2907:
  return 1;
  goto switch_merge_2905;

  case_body_2908:
  return 2;
  goto switch_merge_2905;

  case_body_2909:
  return 3;
  goto switch_merge_2905;

  case_body_2910:
  return 4;
  goto switch_merge_2905;

  case_body_2911:
  return 13;
  goto switch_merge_2905;

  case_body_2912:
  return 14;
  goto switch_merge_2905;

  case_body_2913:
  return 15;
  goto switch_merge_2905;

  case_body_2914:
  return 17;
  goto switch_merge_2905;

  case_body_2915:
  return 16;
  goto switch_merge_2905;

  case_body_2916:
  return 18;
  goto switch_merge_2905;

  case_body_2917:
  return 5;
  goto switch_merge_2905;

  case_body_2918:
  return 6;
  goto switch_merge_2905;

  case_body_2919:
  return 7;
  goto switch_merge_2905;

  case_body_2920:
  return 8;
  goto switch_merge_2905;

  case_body_2921:
  return 9;
  goto switch_merge_2905;

  case_default_2922:
  return 0;
  goto switch_merge_2905;

  switch_merge_2905:
  return 0;
}

struct IR操作数 生成表达式IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r4, r6, r8, r10, r13, r15, r21, r23, r26, r39;
  char* r34;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 表达式节点* r25;
  struct 表达式节点* r27;
  struct 类型节点* r28;
  struct 表达式节点* r29;
  struct 表达式节点* r31;
  struct 类型节点* r32;
  struct 表达式节点* r33;
  struct 表达式节点* r35;
  struct 类型节点* r36;
  struct 表达式节点* r37;
  struct 表达式节点* r40;
  struct 类型节点* r41;
  struct IR生成上下文* r42;
  struct 表达式节点* r43;
  struct IR生成上下文* r44;
  struct 表达式节点* r45;
  struct IR生成上下文* r46;
  struct 表达式节点* r47;
  struct IR生成上下文* r48;
  struct 表达式节点* r49;
  struct IR生成上下文* r50;
  struct 表达式节点* r51;
  struct IR生成上下文* r52;
  struct 表达式节点* r53;
  struct IR生成上下文* r54;
  struct 表达式节点* r55;
  struct IR生成上下文* r56;
  struct 表达式节点* r57;
  struct IR生成上下文* r58;
  struct 表达式节点* r59;
  _Bool r5;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r17;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r22;
  _Bool r24;
  _Bool r38;
  double r30;
  enum 节点类型 r3;

  entry:
  r0 = cn_var_表达式;
  r1 = !r0;
  if (r1) goto if_then_2938; else goto if_merge_2939;

  if_then_2938:
  空操作数();
  return;
  goto if_merge_2939;

  if_merge_2939:
  r2 = cn_var_表达式;
  r3 = r2->类型;
  r4 = cn_var_整数字面量表达式;
  r5 = r3 == r4;
  if (r5) goto case_body_2941; else goto switch_check_2955;

  switch_check_2955:
  r6 = cn_var_小数字面量表达式;
  r7 = r3 == r6;
  if (r7) goto case_body_2942; else goto switch_check_2956;

  switch_check_2956:
  r8 = cn_var_字符串字面量表达式;
  r9 = r3 == r8;
  if (r9) goto case_body_2943; else goto switch_check_2957;

  switch_check_2957:
  r10 = cn_var_布尔字面量表达式;
  r11 = r3 == r10;
  if (r11) goto case_body_2944; else goto switch_check_2958;

  switch_check_2958:
  r12 = r3 == -842150451;
  if (r12) goto case_body_2945; else goto switch_check_2959;

  switch_check_2959:
  r13 = cn_var_二元运算表达式;
  r14 = r3 == r13;
  if (r14) goto case_body_2946; else goto switch_check_2960;

  switch_check_2960:
  r15 = cn_var_一元运算表达式;
  r16 = r3 == r15;
  if (r16) goto case_body_2947; else goto switch_check_2961;

  switch_check_2961:
  r17 = r3 == -842150451;
  if (r17) goto case_body_2948; else goto switch_check_2962;

  switch_check_2962:
  r18 = r3 == -842150451;
  if (r18) goto case_body_2949; else goto switch_check_2963;

  switch_check_2963:
  r19 = r3 == -842150451;
  if (r19) goto case_body_2950; else goto switch_check_2964;

  switch_check_2964:
  r20 = r3 == -842150451;
  if (r20) goto case_body_2951; else goto switch_check_2965;

  switch_check_2965:
  r21 = cn_var_取地址表达式;
  r22 = r3 == r21;
  if (r22) goto case_body_2952; else goto switch_check_2966;

  switch_check_2966:
  r23 = cn_var_解引用表达式;
  r24 = r3 == r23;
  if (r24) goto case_body_2953; else goto case_default_2954;

  case_body_2941:
  r25 = cn_var_表达式;
  r26 = r25->整数值;
  r27 = cn_var_表达式;
  r28 = r27->类型信息;
  整数常量操作数(r26, r28);
  return;
  goto switch_merge_2940;

  case_body_2942:
  r29 = cn_var_表达式;
  r30 = r29->小数值;
  r31 = cn_var_表达式;
  r32 = r31->类型信息;
  小数常量操作数(r30, r32);
  return;
  goto switch_merge_2940;

  case_body_2943:
  r33 = cn_var_表达式;
  r34 = r33->字符串值;
  r35 = cn_var_表达式;
  r36 = r35->类型信息;
  字符串常量操作数(r34, r36);
  return;
  goto switch_merge_2940;

  case_body_2944:
  r37 = cn_var_表达式;
  r38 = r37->布尔值;
  r39 = (r38 ? 1 : 0);
  r40 = cn_var_表达式;
  r41 = r40->类型信息;
  整数常量操作数(r39, r41);
  return;
  goto switch_merge_2940;

  case_body_2945:
  r42 = cn_var_上下文;
  r43 = cn_var_表达式;
  生成标识符IR(r42, r43);
  return;
  goto switch_merge_2940;

  case_body_2946:
  r44 = cn_var_上下文;
  r45 = cn_var_表达式;
  生成二元运算IR(r44, r45);
  return;
  goto switch_merge_2940;

  case_body_2947:
  r46 = cn_var_上下文;
  r47 = cn_var_表达式;
  生成一元运算IR(r46, r47);
  return;
  goto switch_merge_2940;

  case_body_2948:
  r48 = cn_var_上下文;
  r49 = cn_var_表达式;
  生成函数调用IR(r48, r49);
  return;
  goto switch_merge_2940;

  case_body_2949:
  r50 = cn_var_上下文;
  r51 = cn_var_表达式;
  生成成员访问IR(r50, r51);
  return;
  goto switch_merge_2940;

  case_body_2950:
  r52 = cn_var_上下文;
  r53 = cn_var_表达式;
  生成数组访问IR(r52, r53);
  return;
  goto switch_merge_2940;

  case_body_2951:
  r54 = cn_var_上下文;
  r55 = cn_var_表达式;
  生成赋值IR(r54, r55);
  return;
  goto switch_merge_2940;

  case_body_2952:
  r56 = cn_var_上下文;
  r57 = cn_var_表达式;
  生成取地址IR(r56, r57);
  return;
  goto switch_merge_2940;

  case_body_2953:
  r58 = cn_var_上下文;
  r59 = cn_var_表达式;
  生成解引用IR(r58, r59);
  return;
  goto switch_merge_2940;

  case_default_2954:
  空操作数();
  return;
  goto switch_merge_2940;

  switch_merge_2940:
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成标识符IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10;
  char* r7;
  char* r8;
  char* r13;
  char* r20;
  char* r23;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct IR生成上下文* r9;
  struct 表达式节点* r11;
  struct 类型节点* r12;
  struct 表达式节点* r14;
  struct 类型节点* r15;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct 表达式节点* r22;
  struct 表达式节点* r24;
  struct 类型节点* r25;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r21;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2970; else goto logic_rhs_2969;

  if_then_2967:
  空操作数();
  return;
  goto if_merge_2968;

  if_merge_2968:
  char* cn_var_唯一名_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->标识符名称;
  查找局部变量唯一名(r5, r7);
  cn_var_唯一名_0 = /* NONE */;
  r8 = cn_var_唯一名_0;
  if (r8) goto if_then_2971; else goto if_merge_2972;

  logic_rhs_2969:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2970;

  logic_merge_2970:
  if (r4) goto if_then_2967; else goto if_merge_2968;

  if_then_2971:
  long long cn_var_寄存器_1;
  r9 = cn_var_上下文;
  分配虚拟寄存器(r9);
  cn_var_寄存器_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r10 = cn_var_寄存器_1;
  r11 = cn_var_表达式;
  r12 = r11->类型信息;
  寄存器操作数(r10, r12);
  cn_var_目标_2 = /* NONE */;
  struct IR操作数 cn_var_源_3;
  r13 = cn_var_唯一名_0;
  r14 = cn_var_表达式;
  r15 = r14->类型信息;
  全局符号操作数(r13, r15);
  cn_var_源_3 = /* NONE */;
  struct IR指令* cn_var_指令_4;
  r16 = cn_var_目标_2;
  r17 = cn_var_源_3;
  空操作数();
  创建IR指令(20, r16, r17, /* NONE */);
  cn_var_指令_4 = /* NONE */;
  r18 = cn_var_上下文;
  r19 = cn_var_指令_4;
  发射指令(r18, r19);
  r20 = cn_var_唯一名_0;
  释放内存(r20);
  r21 = cn_var_目标_2;
  return r21;
  goto if_merge_2972;

  if_merge_2972:
  r22 = cn_var_表达式;
  r23 = r22->标识符名称;
  r24 = cn_var_表达式;
  r25 = r24->类型信息;
  全局符号操作数(r23, r25);
  return;
}

struct IR操作数 生成二元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r12, r16;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r8;
  struct 表达式节点* r9;
  struct 表达式节点* r10;
  struct IR生成上下文* r11;
  struct 表达式节点* r13;
  struct 类型节点* r14;
  struct 表达式节点* r15;
  struct IR生成上下文* r21;
  struct IR指令* r22;
  enum IR操作码 r17;
  struct IR操作数 r18;
  struct IR操作数 r19;
  struct IR操作数 r20;
  struct IR操作数 r23;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2976; else goto logic_rhs_2975;

  if_then_2973:
  空操作数();
  return;
  goto if_merge_2974;

  if_merge_2974:
  struct IR操作数 cn_var_左_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->左操作数;
  生成表达式IR(r5, r7);
  cn_var_左_0 = /* NONE */;
  struct IR操作数 cn_var_右_1;
  r8 = cn_var_上下文;
  r9 = cn_var_表达式;
  r10 = r9->右操作数;
  生成表达式IR(r8, r10);
  cn_var_右_1 = /* NONE */;
  long long cn_var_结果_2;
  r11 = cn_var_上下文;
  分配虚拟寄存器(r11);
  cn_var_结果_2 = /* NONE */;
  struct IR操作数 cn_var_目标_3;
  r12 = cn_var_结果_2;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  寄存器操作数(r12, r14);
  cn_var_目标_3 = /* NONE */;
  enum IR操作码 cn_var_操作码_4;
  r15 = cn_var_表达式;
  r16 = r15->运算符;
  二元运算符转IR(r16);
  cn_var_操作码_4 = /* NONE */;
  struct IR指令* cn_var_指令_5;
  r17 = cn_var_操作码_4;
  r18 = cn_var_目标_3;
  r19 = cn_var_左_0;
  r20 = cn_var_右_1;
  创建IR指令(r17, r18, r19, r20);
  cn_var_指令_5 = /* NONE */;
  r21 = cn_var_上下文;
  r22 = cn_var_指令_5;
  发射指令(r21, r22);
  r23 = cn_var_目标_3;
  return r23;

  logic_rhs_2975:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2976;

  logic_merge_2976:
  if (r4) goto if_then_2973; else goto if_merge_2974;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成一元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r9, r13, r14, r16, r18;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r8;
  struct 表达式节点* r10;
  struct 类型节点* r11;
  struct 表达式节点* r12;
  struct IR生成上下文* r23;
  struct IR指令* r24;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  enum IR操作码 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r25;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2980; else goto logic_rhs_2979;

  if_then_2977:
  空操作数();
  return;
  goto if_merge_2978;

  if_merge_2978:
  struct IR操作数 cn_var_操作数_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->操作数;
  生成表达式IR(r5, r7);
  cn_var_操作数_0 = /* NONE */;
  long long cn_var_结果_1;
  r8 = cn_var_上下文;
  分配虚拟寄存器(r8);
  cn_var_结果_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r9 = cn_var_结果_1;
  r10 = cn_var_表达式;
  r11 = r10->类型信息;
  寄存器操作数(r9, r11);
  cn_var_目标_2 = /* NONE */;
  enum IR操作码 cn_var_操作码_3;
  cn_var_操作码_3 = 10;
  r12 = cn_var_表达式;
  r13 = r12->运算符;
  r14 = cn_var_负号运算符;
  r15 = r13 == r14;
  if (r15) goto case_body_2982; else goto switch_check_2986;

  logic_rhs_2979:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2980;

  logic_merge_2980:
  if (r4) goto if_then_2977; else goto if_merge_2978;

  switch_check_2986:
  r16 = cn_var_取反运算符;
  r17 = r13 == r16;
  if (r17) goto case_body_2983; else goto switch_check_2987;

  switch_check_2987:
  r18 = cn_var_逻辑非运算符;
  r19 = r13 == r18;
  if (r19) goto case_body_2984; else goto case_default_2985;

  case_body_2982:
  cn_var_操作码_3 = 10;
  goto switch_merge_2981;
  goto switch_merge_2981;

  case_body_2983:
  cn_var_操作码_3 = 11;
  goto switch_merge_2981;
  goto switch_merge_2981;

  case_body_2984:
  cn_var_操作码_3 = 12;
  goto switch_merge_2981;
  goto switch_merge_2981;

  case_default_2985:
  goto switch_merge_2981;
  goto switch_merge_2981;

  switch_merge_2981:
  struct IR指令* cn_var_指令_4;
  r20 = cn_var_操作码_3;
  r21 = cn_var_目标_2;
  r22 = cn_var_操作数_0;
  空操作数();
  创建IR指令(r20, r21, r22, /* NONE */);
  cn_var_指令_4 = /* NONE */;
  r23 = cn_var_上下文;
  r24 = cn_var_指令_4;
  发射指令(r23, r24);
  r25 = cn_var_目标_2;
  return r25;
}

struct IR操作数 生成函数调用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r7, r9, r19, r20, r22, r23, r24, r25, r26, r27, r28, r36, r37, r38, r40;
  void* r35;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct IR生成上下文* r8;
  struct 表达式节点* r10;
  struct 类型节点* r11;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r29;
  struct 表达式节点* r30;
  struct IR生成上下文* r32;
  struct IR指令* r34;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR操作数 r12;
  struct IR操作数 r13;
  struct 参数 r18;
  struct 参数 r21;
  struct 参数 r31;
  struct 参数 r33;
  struct 参数 r39;
  struct IR操作数 r43;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2991; else goto logic_rhs_2990;

  if_then_2988:
  空操作数();
  return;
  goto if_merge_2989;

  if_merge_2989:
  struct IR操作数 cn_var_被调函数_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->被调用函数;
  生成表达式IR(r5, r7);
  cn_var_被调函数_0 = /* NONE */;
  long long cn_var_结果_1;
  r8 = cn_var_上下文;
  分配虚拟寄存器(r8);
  cn_var_结果_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r9 = cn_var_结果_1;
  r10 = cn_var_表达式;
  r11 = r10->类型信息;
  寄存器操作数(r9, r11);
  cn_var_目标_2 = /* NONE */;
  struct IR指令* cn_var_指令_3;
  r12 = cn_var_目标_2;
  r13 = cn_var_被调函数_0;
  空操作数();
  创建IR指令(28, r12, r13, /* NONE */);
  cn_var_指令_3 = /* NONE */;
  r14 = cn_var_表达式;
  r15 = r14->参数列表;
  if (r15) goto if_then_2992; else goto if_merge_2993;

  logic_rhs_2990:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_2991;

  logic_merge_2991:
  if (r4) goto if_then_2988; else goto if_merge_2989;

  if_then_2992:
  long long cn_var_数量_4;
  cn_var_数量_4 = 0;
  struct 表达式节点* cn_var_参数_5;
  r16 = cn_var_表达式;
  r17 = r16->参数列表;
  cn_var_参数_5 = r17;
  goto while_cond_2994;

  if_merge_2993:
  r41 = cn_var_上下文;
  r42 = cn_var_指令_3;
  发射指令(r41, r42);
  r43 = cn_var_目标_2;
  return r43;

  while_cond_2994:
  r18 = cn_var_参数_5;
  if (r18) goto while_body_2995; else goto while_exit_2996;

  while_body_2995:
  r19 = cn_var_数量_4;
  r20 = r19 + 1;
  cn_var_数量_4 = r20;
  r21 = cn_var_参数_5;
  r22 = r21.下一个;
  cn_var_参数_5 = r22;
  goto while_cond_2994;

  while_exit_2996:
  r23 = cn_var_数量_4;
  r24 = r23 > 0;
  if (r24) goto if_then_2997; else goto if_merge_2998;

  if_then_2997:
  r25 = cn_var_数量_4;
  r26 = sizeof(struct IR操作数);
  r27 = r25 * r26;
  分配内存(r27);
  r28 = cn_var_数量_4;
  r29 = cn_var_表达式;
  r30 = r29->参数列表;
  cn_var_参数_5 = r30;
  long long cn_var_索引_6;
  cn_var_索引_6 = 0;
  goto while_cond_2999;

  if_merge_2998:
  goto if_merge_2993;

  while_cond_2999:
  r31 = cn_var_参数_5;
  if (r31) goto while_body_3000; else goto while_exit_3001;

  while_body_3000:
  r32 = cn_var_上下文;
  r33 = cn_var_参数_5;
  生成表达式IR(r32, r33);
  r34 = cn_var_指令_3;
  r35 = r34->额外参数;
  r36 = cn_var_索引_6;
    cn_rt_array_set_element(r35, r36, &/* NONE */, 8);
  r37 = cn_var_索引_6;
  r38 = r37 + 1;
  cn_var_索引_6 = r38;
  r39 = cn_var_参数_5;
  r40 = r39.下一个;
  cn_var_参数_5 = r40;
  goto while_cond_2999;

  while_exit_3001:
  goto if_merge_2998;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成成员访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r7, r9, r13;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct IR生成上下文* r8;
  struct 表达式节点* r10;
  struct 类型节点* r11;
  struct 表达式节点* r12;
  struct IR生成上下文* r17;
  struct IR指令* r18;
  struct IR操作数 r14;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r19;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3005; else goto logic_rhs_3004;

  if_then_3002:
  空操作数();
  return;
  goto if_merge_3003;

  if_merge_3003:
  struct IR操作数 cn_var_对象_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->对象表达式;
  生成表达式IR(r5, r7);
  cn_var_对象_0 = /* NONE */;
  long long cn_var_结果_1;
  r8 = cn_var_上下文;
  分配虚拟寄存器(r8);
  cn_var_结果_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r9 = cn_var_结果_1;
  r10 = cn_var_表达式;
  r11 = r10->类型信息;
  寄存器操作数(r9, r11);
  cn_var_目标_2 = /* NONE */;
  struct IR操作数 cn_var_成员索引_3;
  r12 = cn_var_表达式;
  r13 = r12->成员索引;
  整数常量操作数(r13, 0);
  cn_var_成员索引_3 = /* NONE */;
  struct IR指令* cn_var_指令_4;
  r14 = cn_var_目标_2;
  r15 = cn_var_对象_0;
  r16 = cn_var_成员索引_3;
  创建IR指令(31, r14, r15, r16);
  cn_var_指令_4 = /* NONE */;
  r17 = cn_var_上下文;
  r18 = cn_var_指令_4;
  发射指令(r17, r18);
  r19 = cn_var_目标_2;
  return r19;

  logic_rhs_3004:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3005;

  logic_merge_3005:
  if (r4) goto if_then_3002; else goto if_merge_3003;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成数组访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r7, r10, r12;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct IR生成上下文* r8;
  struct 表达式节点* r9;
  struct IR生成上下文* r11;
  struct 表达式节点* r13;
  struct 类型节点* r14;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r20;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3009; else goto logic_rhs_3008;

  if_then_3006:
  空操作数();
  return;
  goto if_merge_3007;

  if_merge_3007:
  struct IR操作数 cn_var_数组_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->数组表达式;
  生成表达式IR(r5, r7);
  cn_var_数组_0 = /* NONE */;
  struct IR操作数 cn_var_索引_1;
  r8 = cn_var_上下文;
  r9 = cn_var_表达式;
  r10 = r9->索引表达式;
  生成表达式IR(r8, r10);
  cn_var_索引_1 = /* NONE */;
  long long cn_var_结果_2;
  r11 = cn_var_上下文;
  分配虚拟寄存器(r11);
  cn_var_结果_2 = /* NONE */;
  struct IR操作数 cn_var_目标_3;
  r12 = cn_var_结果_2;
  r13 = cn_var_表达式;
  r14 = r13->类型信息;
  寄存器操作数(r12, r14);
  cn_var_目标_3 = /* NONE */;
  struct IR指令* cn_var_指令_4;
  r15 = cn_var_目标_3;
  r16 = cn_var_数组_0;
  r17 = cn_var_索引_1;
  创建IR指令(20, r15, r16, r17);
  cn_var_指令_4 = /* NONE */;
  r18 = cn_var_上下文;
  r19 = cn_var_指令_4;
  发射指令(r18, r19);
  r20 = cn_var_目标_3;
  return r20;

  logic_rhs_3008:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3009;

  logic_merge_3009:
  if (r4) goto if_then_3006; else goto if_merge_3007;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成赋值IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r7, r11;
  char* r15;
  char* r16;
  char* r17;
  char* r25;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r8;
  struct 表达式节点* r9;
  struct IR生成上下文* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 类型节点* r20;
  struct IR生成上下文* r23;
  struct IR指令* r24;
  struct IR生成上下文* r26;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct IR生成上下文* r31;
  struct IR指令* r32;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r29;
  struct IR操作数 r30;
  struct IR操作数 r33;
  enum 节点类型 r10;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3013; else goto logic_rhs_3012;

  if_then_3010:
  空操作数();
  return;
  goto if_merge_3011;

  if_merge_3011:
  struct IR操作数 cn_var_值_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->右侧表达式;
  生成表达式IR(r5, r7);
  cn_var_值_0 = /* NONE */;
  r8 = cn_var_表达式;
  r9 = r8->左侧表达式;
  r10 = r9->类型;
  r11 = r10 == -842150451;
  if (r11) goto if_then_3014; else goto if_else_3015;

  logic_rhs_3012:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3013;

  logic_merge_3013:
  if (r4) goto if_then_3010; else goto if_merge_3011;

  if_then_3014:
  char* cn_var_唯一名_1;
  r12 = cn_var_上下文;
  r13 = cn_var_表达式;
  r14 = r13->左侧表达式;
  r15 = r14->标识符名称;
  查找局部变量唯一名(r12, r15);
  cn_var_唯一名_1 = /* NONE */;
  r16 = cn_var_唯一名_1;
  if (r16) goto if_then_3017; else goto if_merge_3018;

  if_else_3015:
  struct IR操作数 cn_var_目标_4;
  r26 = cn_var_上下文;
  r27 = cn_var_表达式;
  r28 = r27->左侧表达式;
  生成表达式IR(r26, r28);
  cn_var_目标_4 = /* NONE */;
  struct IR指令* cn_var_指令_5;
  r29 = cn_var_目标_4;
  r30 = cn_var_值_0;
  空操作数();
  创建IR指令(21, r29, r30, /* NONE */);
  cn_var_指令_5 = /* NONE */;
  r31 = cn_var_上下文;
  r32 = cn_var_指令_5;
  发射指令(r31, r32);
  goto if_merge_3016;

  if_merge_3016:
  r33 = cn_var_值_0;
  return r33;

  if_then_3017:
  struct IR操作数 cn_var_目标_2;
  r17 = cn_var_唯一名_1;
  r18 = cn_var_表达式;
  r19 = r18->左侧表达式;
  r20 = r19->类型信息;
  全局符号操作数(r17, r20);
  cn_var_目标_2 = /* NONE */;
  struct IR指令* cn_var_指令_3;
  r21 = cn_var_目标_2;
  r22 = cn_var_值_0;
  空操作数();
  创建IR指令(21, r21, r22, /* NONE */);
  cn_var_指令_3 = /* NONE */;
  r23 = cn_var_上下文;
  r24 = cn_var_指令_3;
  发射指令(r23, r24);
  r25 = cn_var_唯一名_1;
  释放内存(r25);
  goto if_merge_3018;

  if_merge_3018:
  goto if_merge_3016;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成取地址IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r9;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r8;
  struct 表达式节点* r10;
  struct 类型节点* r11;
  struct IR生成上下文* r14;
  struct IR指令* r15;
  struct IR操作数 r12;
  struct IR操作数 r13;
  struct IR操作数 r16;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3022; else goto logic_rhs_3021;

  if_then_3019:
  空操作数();
  return;
  goto if_merge_3020;

  if_merge_3020:
  struct IR操作数 cn_var_操作数_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->操作数;
  生成表达式IR(r5, r7);
  cn_var_操作数_0 = /* NONE */;
  long long cn_var_结果_1;
  r8 = cn_var_上下文;
  分配虚拟寄存器(r8);
  cn_var_结果_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r9 = cn_var_结果_1;
  r10 = cn_var_表达式;
  r11 = r10->类型信息;
  寄存器操作数(r9, r11);
  cn_var_目标_2 = /* NONE */;
  struct IR指令* cn_var_指令_3;
  r12 = cn_var_目标_2;
  r13 = cn_var_操作数_0;
  空操作数();
  创建IR指令(23, r12, r13, /* NONE */);
  cn_var_指令_3 = /* NONE */;
  r14 = cn_var_上下文;
  r15 = cn_var_指令_3;
  发射指令(r14, r15);
  r16 = cn_var_目标_2;
  return r16;

  logic_rhs_3021:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3022;

  logic_merge_3022:
  if (r4) goto if_then_3019; else goto if_merge_3020;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成解引用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r9;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r5;
  struct 表达式节点* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r8;
  struct 表达式节点* r10;
  struct 类型节点* r11;
  struct IR生成上下文* r14;
  struct IR指令* r15;
  struct IR操作数 r12;
  struct IR操作数 r13;
  struct IR操作数 r16;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3026; else goto logic_rhs_3025;

  if_then_3023:
  空操作数();
  return;
  goto if_merge_3024;

  if_merge_3024:
  struct IR操作数 cn_var_指针_0;
  r5 = cn_var_上下文;
  r6 = cn_var_表达式;
  r7 = r6->操作数;
  生成表达式IR(r5, r7);
  cn_var_指针_0 = /* NONE */;
  long long cn_var_结果_1;
  r8 = cn_var_上下文;
  分配虚拟寄存器(r8);
  cn_var_结果_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r9 = cn_var_结果_1;
  r10 = cn_var_表达式;
  r11 = r10->类型信息;
  寄存器操作数(r9, r11);
  cn_var_目标_2 = /* NONE */;
  struct IR指令* cn_var_指令_3;
  r12 = cn_var_目标_2;
  r13 = cn_var_指针_0;
  空操作数();
  创建IR指令(24, r12, r13, /* NONE */);
  cn_var_指令_3 = /* NONE */;
  r14 = cn_var_上下文;
  r15 = cn_var_指令_3;
  发射指令(r14, r15);
  r16 = cn_var_目标_2;
  return r16;

  logic_rhs_3025:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3026;

  logic_merge_3026:
  if (r4) goto if_then_3023; else goto if_merge_3024;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

void 生成语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r8, r11, r20;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r5;
  struct IR生成上下文* r18;
  struct 语句节点* r19;
  struct IR生成上下文* r21;
  struct 语句节点* r22;
  struct IR生成上下文* r23;
  struct 语句节点* r24;
  struct IR生成上下文* r25;
  struct 语句节点* r26;
  struct IR生成上下文* r27;
  struct 语句节点* r28;
  struct IR生成上下文* r29;
  struct 语句节点* r30;
  struct IR生成上下文* r31;
  struct IR生成上下文* r32;
  struct IR生成上下文* r33;
  struct 语句节点* r34;
  _Bool r7;
  _Bool r9;
  _Bool r10;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  enum 节点类型 r6;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3030; else goto logic_rhs_3029;

  if_then_3027:
  return;
  goto if_merge_3028;

  if_merge_3028:
  r5 = cn_var_语句;
  r6 = r5->类型;
  r7 = r6 == -842150451;
  if (r7) goto case_body_3032; else goto switch_check_3042;

  logic_rhs_3029:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3030;

  logic_merge_3030:
  if (r4) goto if_then_3027; else goto if_merge_3028;

  switch_check_3042:
  r8 = cn_var_变量声明语句;
  r9 = r6 == r8;
  if (r9) goto case_body_3033; else goto switch_check_3043;

  switch_check_3043:
  r10 = r6 == -842150451;
  if (r10) goto case_body_3034; else goto switch_check_3044;

  switch_check_3044:
  r11 = cn_var_当循环语句;
  r12 = r6 == r11;
  if (r12) goto case_body_3035; else goto switch_check_3045;

  switch_check_3045:
  r13 = r6 == -842150451;
  if (r13) goto case_body_3036; else goto switch_check_3046;

  switch_check_3046:
  r14 = r6 == -842150451;
  if (r14) goto case_body_3037; else goto switch_check_3047;

  switch_check_3047:
  r15 = r6 == -842150451;
  if (r15) goto case_body_3038; else goto switch_check_3048;

  switch_check_3048:
  r16 = r6 == -842150451;
  if (r16) goto case_body_3039; else goto switch_check_3049;

  switch_check_3049:
  r17 = r6 == -842150451;
  if (r17) goto case_body_3040; else goto case_default_3041;

  case_body_3032:
  r18 = cn_var_上下文;
  r19 = cn_var_语句;
  r20 = r19->表达式;
  生成表达式IR(r18, r20);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3033:
  r21 = cn_var_上下文;
  r22 = cn_var_语句;
  生成变量声明IR(r21, r22);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3034:
  r23 = cn_var_上下文;
  r24 = cn_var_语句;
  生成如果语句IR(r23, r24);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3035:
  r25 = cn_var_上下文;
  r26 = cn_var_语句;
  生成当循环IR(r25, r26);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3036:
  r27 = cn_var_上下文;
  r28 = cn_var_语句;
  生成循环IR(r27, r28);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3037:
  r29 = cn_var_上下文;
  r30 = cn_var_语句;
  生成返回语句IR(r29, r30);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3038:
  r31 = cn_var_上下文;
  生成中断语句IR(r31);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3039:
  r32 = cn_var_上下文;
  生成继续语句IR(r32);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_body_3040:
  r33 = cn_var_上下文;
  r34 = cn_var_语句;
  生成块语句IR(r33, r34);
  goto switch_merge_3031;
  goto switch_merge_3031;

  case_default_3041:
  goto switch_merge_3031;
  goto switch_merge_3031;

  switch_merge_3031:
  return;
}

void 生成变量声明IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r6, r9, r12, r14, r16, r22, r25, r28;
  char* r10;
  char* r26;
  char* r32;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r5;
  struct IR生成上下文* r7;
  struct 语句节点* r8;
  struct IR生成上下文* r11;
  struct 语句节点* r13;
  struct 语句节点* r15;
  struct IR生成上下文* r19;
  struct IR指令* r20;
  struct 语句节点* r21;
  struct IR生成上下文* r23;
  struct 语句节点* r24;
  struct 语句节点* r27;
  struct IR生成上下文* r31;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r29;
  struct IR操作数 r30;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3053; else goto logic_rhs_3052;

  if_then_3050:
  return;
  goto if_merge_3051;

  if_merge_3051:
  char* cn_var_唯一名_0;
  r5 = cn_var_语句;
  r6 = r5->变量名称;
  生成唯一变量名(r6);
  cn_var_唯一名_0 = /* NONE */;
  r7 = cn_var_上下文;
  r8 = cn_var_语句;
  r9 = r8->变量名称;
  r10 = cn_var_唯一名_0;
  添加局部变量映射(r7, r9, r10);
  long long cn_var_寄存器_1;
  r11 = cn_var_上下文;
  分配虚拟寄存器(r11);
  cn_var_寄存器_1 = /* NONE */;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_寄存器_1;
  r13 = cn_var_语句;
  r14 = r13->类型信息;
  寄存器操作数(r12, r14);
  cn_var_目标_2 = /* NONE */;
  struct IR操作数 cn_var_大小_3;
  r15 = cn_var_语句;
  r16 = r15->类型大小;
  整数常量操作数(r16, 0);
  cn_var_大小_3 = /* NONE */;
  struct IR指令* cn_var_指令_4;
  r17 = cn_var_目标_2;
  r18 = cn_var_大小_3;
  空操作数();
  创建IR指令(19, r17, r18, /* NONE */);
  cn_var_指令_4 = /* NONE */;
  r19 = cn_var_上下文;
  r20 = cn_var_指令_4;
  发射指令(r19, r20);
  r21 = cn_var_语句;
  r22 = r21->初始值表达式;
  if (r22) goto if_then_3054; else goto if_merge_3055;

  logic_rhs_3052:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3053;

  logic_merge_3053:
  if (r4) goto if_then_3050; else goto if_merge_3051;

  if_then_3054:
  struct IR操作数 cn_var_值_5;
  r23 = cn_var_上下文;
  r24 = cn_var_语句;
  r25 = r24->初始值表达式;
  生成表达式IR(r23, r25);
  cn_var_值_5 = /* NONE */;
  struct IR操作数 cn_var_目标变量_6;
  r26 = cn_var_唯一名_0;
  r27 = cn_var_语句;
  r28 = r27->类型信息;
  全局符号操作数(r26, r28);
  cn_var_目标变量_6 = /* NONE */;
  struct IR指令* cn_var_存储指令_7;
  r29 = cn_var_目标变量_6;
  r30 = cn_var_值_5;
  空操作数();
  创建IR指令(21, r29, r30, /* NONE */);
  cn_var_存储指令_7 = /* NONE */;
  r31 = cn_var_上下文;
  发射指令(r31, 21);
  goto if_merge_3055;

  if_merge_3055:
  r32 = cn_var_唯一名_0;
  释放内存(r32);
  return;
}

void 生成如果语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r7, r26, r36, r39;
  char* r8;
  char* r9;
  char* r10;
  char* r11;
  char* r12;
  char* r13;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r5;
  struct 语句节点* r6;
  struct 基本块* r15;
  struct 基本块* r16;
  struct IR生成上下文* r17;
  struct IR指令* r18;
  struct IR生成上下文* r19;
  struct IR函数* r20;
  struct 基本块* r21;
  struct IR生成上下文* r22;
  struct 基本块* r23;
  struct IR生成上下文* r24;
  struct 语句节点* r25;
  struct 基本块* r27;
  struct IR生成上下文* r28;
  struct IR指令* r29;
  struct IR生成上下文* r30;
  struct IR函数* r31;
  struct 基本块* r32;
  struct IR生成上下文* r33;
  struct 基本块* r34;
  struct 语句节点* r35;
  struct IR生成上下文* r37;
  struct 语句节点* r38;
  struct 基本块* r40;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR生成上下文* r43;
  struct IR函数* r44;
  struct 基本块* r45;
  struct IR生成上下文* r46;
  struct 基本块* r47;
  struct IR操作数 r14;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3059; else goto logic_rhs_3058;

  if_then_3056:
  return;
  goto if_merge_3057;

  if_merge_3057:
  struct IR操作数 cn_var_条件_0;
  r5 = cn_var_上下文;
  r6 = cn_var_语句;
  r7 = r6->条件表达式;
  生成表达式IR(r5, r7);
  cn_var_条件_0 = /* NONE */;
  char* cn_var_真块名_1;
  生成块名称("if_true");
  cn_var_真块名_1 = /* NONE */;
  char* cn_var_假块名_2;
  生成块名称("if_false");
  cn_var_假块名_2 = /* NONE */;
  char* cn_var_结束块名_3;
  生成块名称("if_end");
  cn_var_结束块名_3 = /* NONE */;
  struct 基本块* cn_var_真块_4;
  r8 = cn_var_真块名_1;
  创建基本块(r8);
  cn_var_真块_4 = /* NONE */;
  struct 基本块* cn_var_假块_5;
  r9 = cn_var_假块名_2;
  创建基本块(r9);
  cn_var_假块_5 = /* NONE */;
  struct 基本块* cn_var_结束块_6;
  r10 = cn_var_结束块名_3;
  创建基本块(r10);
  cn_var_结束块_6 = /* NONE */;
  r11 = cn_var_真块名_1;
  释放内存(r11);
  r12 = cn_var_假块名_2;
  释放内存(r12);
  r13 = cn_var_结束块名_3;
  释放内存(r13);
  struct IR指令* cn_var_跳转_7;
  r14 = cn_var_条件_0;
  r15 = cn_var_真块_4;
  标签操作数(r15);
  r16 = cn_var_假块_5;
  标签操作数(r16);
  创建IR指令(27, r14, /* NONE */, /* NONE */);
  cn_var_跳转_7 = /* NONE */;
  r17 = cn_var_上下文;
  r18 = cn_var_跳转_7;
  发射指令(r17, r18);
  r19 = cn_var_上下文;
  r20 = r19->当前函数;
  r21 = cn_var_真块_4;
  添加基本块(r20, r21);
  r22 = cn_var_上下文;
  r23 = cn_var_真块_4;
  切换基本块(r22, r23);
  r24 = cn_var_上下文;
  r25 = cn_var_语句;
  r26 = r25->真分支语句;
  生成语句IR(r24, r26);
  struct IR指令* cn_var_跳转到结束_8;
  r27 = cn_var_结束块_6;
  标签操作数(r27);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳转到结束_8 = /* NONE */;
  r28 = cn_var_上下文;
  r29 = cn_var_跳转到结束_8;
  发射指令(r28, r29);
  r30 = cn_var_上下文;
  r31 = r30->当前函数;
  r32 = cn_var_假块_5;
  添加基本块(r31, r32);
  r33 = cn_var_上下文;
  r34 = cn_var_假块_5;
  切换基本块(r33, r34);
  r35 = cn_var_语句;
  r36 = r35->假分支语句;
  if (r36) goto if_then_3060; else goto if_merge_3061;

  logic_rhs_3058:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3059;

  logic_merge_3059:
  if (r4) goto if_then_3056; else goto if_merge_3057;

  if_then_3060:
  r37 = cn_var_上下文;
  r38 = cn_var_语句;
  r39 = r38->假分支语句;
  生成语句IR(r37, r39);
  goto if_merge_3061;

  if_merge_3061:
  struct IR指令* cn_var_跳转到结束2_9;
  r40 = cn_var_结束块_6;
  标签操作数(r40);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳转到结束2_9 = /* NONE */;
  r41 = cn_var_上下文;
  r42 = cn_var_跳转到结束2_9;
  发射指令(r41, r42);
  r43 = cn_var_上下文;
  r44 = r43->当前函数;
  r45 = cn_var_结束块_6;
  添加基本块(r44, r45);
  r46 = cn_var_上下文;
  r47 = cn_var_结束块_6;
  切换基本块(r46, r47);
  return;
}

void 生成当循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r27, r40;
  char* r5;
  char* r6;
  char* r7;
  char* r8;
  char* r9;
  char* r10;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r11;
  struct 基本块* r12;
  struct IR生成上下文* r13;
  struct 基本块* r14;
  struct 基本块* r15;
  struct 基本块* r16;
  struct 基本块* r17;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct IR生成上下文* r20;
  struct IR函数* r21;
  struct 基本块* r22;
  struct IR生成上下文* r23;
  struct 基本块* r24;
  struct IR生成上下文* r25;
  struct 语句节点* r26;
  struct 基本块* r29;
  struct 基本块* r30;
  struct IR生成上下文* r31;
  struct IR指令* r32;
  struct IR生成上下文* r33;
  struct IR函数* r34;
  struct 基本块* r35;
  struct IR生成上下文* r36;
  struct 基本块* r37;
  struct IR生成上下文* r38;
  struct 语句节点* r39;
  struct 基本块* r41;
  struct IR生成上下文* r42;
  struct IR指令* r43;
  struct IR生成上下文* r44;
  struct IR函数* r45;
  struct 基本块* r46;
  struct IR生成上下文* r47;
  struct 基本块* r48;
  struct 基本块* r49;
  struct 基本块* r50;
  struct IR操作数 r28;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3065; else goto logic_rhs_3064;

  if_then_3062:
  return;
  goto if_merge_3063;

  if_merge_3063:
  char* cn_var_条件块名_0;
  生成块名称("while_cond");
  cn_var_条件块名_0 = /* NONE */;
  char* cn_var_循环体名_1;
  生成块名称("while_body");
  cn_var_循环体名_1 = /* NONE */;
  char* cn_var_退出块名_2;
  生成块名称("while_exit");
  cn_var_退出块名_2 = /* NONE */;
  struct 基本块* cn_var_条件块_3;
  r5 = cn_var_条件块名_0;
  创建基本块(r5);
  cn_var_条件块_3 = /* NONE */;
  struct 基本块* cn_var_循环体块_4;
  r6 = cn_var_循环体名_1;
  创建基本块(r6);
  cn_var_循环体块_4 = /* NONE */;
  struct 基本块* cn_var_退出块_5;
  r7 = cn_var_退出块名_2;
  创建基本块(r7);
  cn_var_退出块_5 = /* NONE */;
  r8 = cn_var_条件块名_0;
  释放内存(r8);
  r9 = cn_var_循环体名_1;
  释放内存(r9);
  r10 = cn_var_退出块名_2;
  释放内存(r10);
  struct 基本块* cn_var_原退出_6;
  r11 = cn_var_上下文;
  r12 = r11->循环退出块;
  cn_var_原退出_6 = r12;
  struct 基本块* cn_var_原继续_7;
  r13 = cn_var_上下文;
  r14 = r13->循环继续块;
  cn_var_原继续_7 = r14;
  r15 = cn_var_退出块_5;
  r16 = cn_var_条件块_3;
  struct IR指令* cn_var_跳转到条件_8;
  r17 = cn_var_条件块_3;
  标签操作数(r17);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳转到条件_8 = /* NONE */;
  r18 = cn_var_上下文;
  r19 = cn_var_跳转到条件_8;
  发射指令(r18, r19);
  r20 = cn_var_上下文;
  r21 = r20->当前函数;
  r22 = cn_var_条件块_3;
  添加基本块(r21, r22);
  r23 = cn_var_上下文;
  r24 = cn_var_条件块_3;
  切换基本块(r23, r24);
  struct IR操作数 cn_var_条件_9;
  r25 = cn_var_上下文;
  r26 = cn_var_语句;
  r27 = r26->条件表达式;
  生成表达式IR(r25, r27);
  cn_var_条件_9 = /* NONE */;
  struct IR指令* cn_var_条件跳转_10;
  r28 = cn_var_条件_9;
  r29 = cn_var_循环体块_4;
  标签操作数(r29);
  r30 = cn_var_退出块_5;
  标签操作数(r30);
  创建IR指令(27, r28, /* NONE */, /* NONE */);
  cn_var_条件跳转_10 = /* NONE */;
  r31 = cn_var_上下文;
  r32 = cn_var_条件跳转_10;
  发射指令(r31, r32);
  r33 = cn_var_上下文;
  r34 = r33->当前函数;
  r35 = cn_var_循环体块_4;
  添加基本块(r34, r35);
  r36 = cn_var_上下文;
  r37 = cn_var_循环体块_4;
  切换基本块(r36, r37);
  r38 = cn_var_上下文;
  r39 = cn_var_语句;
  r40 = r39->循环体语句;
  生成语句IR(r38, r40);
  struct IR指令* cn_var_跳回条件_11;
  r41 = cn_var_条件块_3;
  标签操作数(r41);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳回条件_11 = /* NONE */;
  r42 = cn_var_上下文;
  r43 = cn_var_跳回条件_11;
  发射指令(r42, r43);
  r44 = cn_var_上下文;
  r45 = r44->当前函数;
  r46 = cn_var_退出块_5;
  添加基本块(r45, r46);
  r47 = cn_var_上下文;
  r48 = cn_var_退出块_5;
  切换基本块(r47, r48);
  r49 = cn_var_原退出_6;
  r50 = cn_var_原继续_7;

  logic_rhs_3064:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3065;

  logic_merge_3065:
  if (r4) goto if_then_3062; else goto if_merge_3063;
  return;
}

void 生成循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r22, r25, r35, r38, r54, r64, r67;
  char* r5;
  char* r6;
  char* r7;
  char* r8;
  char* r9;
  char* r10;
  char* r11;
  char* r12;
  char* r13;
  char* r14;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r15;
  struct 基本块* r16;
  struct IR生成上下文* r17;
  struct 基本块* r18;
  struct 基本块* r19;
  struct 基本块* r20;
  struct 语句节点* r21;
  struct IR生成上下文* r23;
  struct 语句节点* r24;
  struct 基本块* r26;
  struct IR生成上下文* r27;
  struct IR指令* r28;
  struct IR生成上下文* r29;
  struct IR函数* r30;
  struct 基本块* r31;
  struct IR生成上下文* r32;
  struct 基本块* r33;
  struct 语句节点* r34;
  struct IR生成上下文* r36;
  struct 语句节点* r37;
  struct 基本块* r40;
  struct 基本块* r41;
  struct IR生成上下文* r42;
  struct IR指令* r43;
  struct 基本块* r44;
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
  struct IR生成上下文* r56;
  struct IR指令* r57;
  struct IR生成上下文* r58;
  struct IR函数* r59;
  struct 基本块* r60;
  struct IR生成上下文* r61;
  struct 基本块* r62;
  struct 语句节点* r63;
  struct IR生成上下文* r65;
  struct 语句节点* r66;
  struct 基本块* r68;
  struct IR生成上下文* r69;
  struct IR指令* r70;
  struct IR生成上下文* r71;
  struct IR函数* r72;
  struct 基本块* r73;
  struct IR生成上下文* r74;
  struct 基本块* r75;
  struct 基本块* r76;
  struct 基本块* r77;
  struct IR操作数 r39;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3069; else goto logic_rhs_3068;

  if_then_3066:
  return;
  goto if_merge_3067;

  if_merge_3067:
  char* cn_var_初始化块名_0;
  生成块名称("for_init");
  cn_var_初始化块名_0 = /* NONE */;
  char* cn_var_条件块名_1;
  生成块名称("for_cond");
  cn_var_条件块名_1 = /* NONE */;
  char* cn_var_循环体名_2;
  生成块名称("for_body");
  cn_var_循环体名_2 = /* NONE */;
  char* cn_var_迭代块名_3;
  生成块名称("for_iter");
  cn_var_迭代块名_3 = /* NONE */;
  char* cn_var_退出块名_4;
  生成块名称("for_exit");
  cn_var_退出块名_4 = /* NONE */;
  struct 基本块* cn_var_初始化块_5;
  r5 = cn_var_初始化块名_0;
  创建基本块(r5);
  cn_var_初始化块_5 = /* NONE */;
  struct 基本块* cn_var_条件块_6;
  r6 = cn_var_条件块名_1;
  创建基本块(r6);
  cn_var_条件块_6 = /* NONE */;
  struct 基本块* cn_var_循环体块_7;
  r7 = cn_var_循环体名_2;
  创建基本块(r7);
  cn_var_循环体块_7 = /* NONE */;
  struct 基本块* cn_var_迭代块_8;
  r8 = cn_var_迭代块名_3;
  创建基本块(r8);
  cn_var_迭代块_8 = /* NONE */;
  struct 基本块* cn_var_退出块_9;
  r9 = cn_var_退出块名_4;
  创建基本块(r9);
  cn_var_退出块_9 = /* NONE */;
  r10 = cn_var_初始化块名_0;
  释放内存(r10);
  r11 = cn_var_条件块名_1;
  释放内存(r11);
  r12 = cn_var_循环体名_2;
  释放内存(r12);
  r13 = cn_var_迭代块名_3;
  释放内存(r13);
  r14 = cn_var_退出块名_4;
  释放内存(r14);
  struct 基本块* cn_var_原退出_10;
  r15 = cn_var_上下文;
  r16 = r15->循环退出块;
  cn_var_原退出_10 = r16;
  struct 基本块* cn_var_原继续_11;
  r17 = cn_var_上下文;
  r18 = r17->循环继续块;
  cn_var_原继续_11 = r18;
  r19 = cn_var_退出块_9;
  r20 = cn_var_迭代块_8;
  r21 = cn_var_语句;
  r22 = r21->初始化语句;
  if (r22) goto if_then_3070; else goto if_merge_3071;

  logic_rhs_3068:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3069;

  logic_merge_3069:
  if (r4) goto if_then_3066; else goto if_merge_3067;

  if_then_3070:
  r23 = cn_var_上下文;
  r24 = cn_var_语句;
  r25 = r24->初始化语句;
  生成语句IR(r23, r25);
  goto if_merge_3071;

  if_merge_3071:
  struct IR指令* cn_var_跳转到条件_12;
  r26 = cn_var_条件块_6;
  标签操作数(r26);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳转到条件_12 = /* NONE */;
  r27 = cn_var_上下文;
  r28 = cn_var_跳转到条件_12;
  发射指令(r27, r28);
  r29 = cn_var_上下文;
  r30 = r29->当前函数;
  r31 = cn_var_条件块_6;
  添加基本块(r30, r31);
  r32 = cn_var_上下文;
  r33 = cn_var_条件块_6;
  切换基本块(r32, r33);
  r34 = cn_var_语句;
  r35 = r34->条件表达式;
  if (r35) goto if_then_3072; else goto if_else_3073;

  if_then_3072:
  struct IR操作数 cn_var_条件_13;
  r36 = cn_var_上下文;
  r37 = cn_var_语句;
  r38 = r37->条件表达式;
  生成表达式IR(r36, r38);
  cn_var_条件_13 = /* NONE */;
  struct IR指令* cn_var_条件跳转_14;
  r39 = cn_var_条件_13;
  r40 = cn_var_循环体块_7;
  标签操作数(r40);
  r41 = cn_var_退出块_9;
  标签操作数(r41);
  创建IR指令(27, r39, /* NONE */, /* NONE */);
  cn_var_条件跳转_14 = /* NONE */;
  r42 = cn_var_上下文;
  r43 = cn_var_条件跳转_14;
  发射指令(r42, r43);
  goto if_merge_3074;

  if_else_3073:
  struct IR指令* cn_var_无条件跳转_15;
  r44 = cn_var_循环体块_7;
  标签操作数(r44);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_无条件跳转_15 = /* NONE */;
  r45 = cn_var_上下文;
  r46 = cn_var_无条件跳转_15;
  发射指令(r45, r46);
  goto if_merge_3074;

  if_merge_3074:
  r47 = cn_var_上下文;
  r48 = r47->当前函数;
  r49 = cn_var_循环体块_7;
  添加基本块(r48, r49);
  r50 = cn_var_上下文;
  r51 = cn_var_循环体块_7;
  切换基本块(r50, r51);
  r52 = cn_var_上下文;
  r53 = cn_var_语句;
  r54 = r53->循环体语句;
  生成语句IR(r52, r54);
  struct IR指令* cn_var_跳转到迭代_16;
  r55 = cn_var_迭代块_8;
  标签操作数(r55);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳转到迭代_16 = /* NONE */;
  r56 = cn_var_上下文;
  r57 = cn_var_跳转到迭代_16;
  发射指令(r56, r57);
  r58 = cn_var_上下文;
  r59 = r58->当前函数;
  r60 = cn_var_迭代块_8;
  添加基本块(r59, r60);
  r61 = cn_var_上下文;
  r62 = cn_var_迭代块_8;
  切换基本块(r61, r62);
  r63 = cn_var_语句;
  r64 = r63->迭代表达式;
  if (r64) goto if_then_3075; else goto if_merge_3076;

  if_then_3075:
  r65 = cn_var_上下文;
  r66 = cn_var_语句;
  r67 = r66->迭代表达式;
  生成表达式IR(r65, r67);
  goto if_merge_3076;

  if_merge_3076:
  struct IR指令* cn_var_跳回条件_17;
  r68 = cn_var_条件块_6;
  标签操作数(r68);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_跳回条件_17 = /* NONE */;
  r69 = cn_var_上下文;
  r70 = cn_var_跳回条件_17;
  发射指令(r69, r70);
  r71 = cn_var_上下文;
  r72 = r71->当前函数;
  r73 = cn_var_退出块_9;
  添加基本块(r72, r73);
  r74 = cn_var_上下文;
  r75 = cn_var_退出块_9;
  切换基本块(r74, r75);
  r76 = cn_var_原退出_10;
  r77 = cn_var_原继续_11;
  return;
}

void 生成返回语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r3, r6;
  struct IR生成上下文* r0;
  struct 语句节点* r2;
  struct IR生成上下文* r4;
  struct 语句节点* r5;
  struct IR生成上下文* r8;
  struct IR指令* r9;
  struct IR操作数 r7;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3077; else goto if_merge_3078;

  if_then_3077:
  return;
  goto if_merge_3078;

  if_merge_3078:
  struct IR操作数 cn_var_值_0;
  空操作数();
  cn_var_值_0 = /* NONE */;
  r2 = cn_var_语句;
  r3 = r2->返回表达式;
  if (r3) goto if_then_3079; else goto if_merge_3080;

  if_then_3079:
  r4 = cn_var_上下文;
  r5 = cn_var_语句;
  r6 = r5->返回表达式;
  生成表达式IR(r4, r6);
  cn_var_值_0 = /* NONE */;
  goto if_merge_3080;

  if_merge_3080:
  struct IR指令* cn_var_指令_1;
  r7 = cn_var_值_0;
  空操作数();
  空操作数();
  创建IR指令(29, r7, /* NONE */, /* NONE */);
  cn_var_指令_1 = /* NONE */;
  r8 = cn_var_上下文;
  r9 = cn_var_指令_1;
  发射指令(r8, r9);
  return;
}

void 生成中断语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r5;
  struct IR生成上下文* r1;
  struct IR生成上下文* r3;
  struct 基本块* r4;
  struct IR生成上下文* r6;
  struct 基本块* r7;
  struct IR生成上下文* r8;
  struct IR指令* r9;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3084; else goto logic_rhs_3083;

  if_then_3081:
  return;
  goto if_merge_3082;

  if_merge_3082:
  struct IR指令* cn_var_指令_0;
  r6 = cn_var_上下文;
  r7 = r6->循环退出块;
  标签操作数(r7);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_指令_0 = /* NONE */;
  r8 = cn_var_上下文;
  r9 = cn_var_指令_0;
  发射指令(r8, r9);

  logic_rhs_3083:
  r3 = cn_var_上下文;
  r4 = r3->循环退出块;
  r5 = !r4;
  goto logic_merge_3084;

  logic_merge_3084:
  if (r5) goto if_then_3081; else goto if_merge_3082;
  return;
}

void 生成继续语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r5;
  struct IR生成上下文* r1;
  struct IR生成上下文* r3;
  struct 基本块* r4;
  struct IR生成上下文* r6;
  struct 基本块* r7;
  struct IR生成上下文* r8;
  struct IR指令* r9;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3088; else goto logic_rhs_3087;

  if_then_3085:
  return;
  goto if_merge_3086;

  if_merge_3086:
  struct IR指令* cn_var_指令_0;
  r6 = cn_var_上下文;
  r7 = r6->循环继续块;
  标签操作数(r7);
  空操作数();
  空操作数();
  创建IR指令(26, /* NONE */, /* NONE */, /* NONE */);
  cn_var_指令_0 = /* NONE */;
  r8 = cn_var_上下文;
  r9 = cn_var_指令_0;
  发射指令(r8, r9);

  logic_rhs_3087:
  r3 = cn_var_上下文;
  r4 = r3->循环继续块;
  r5 = !r4;
  goto logic_merge_3088;

  logic_merge_3088:
  if (r5) goto if_then_3085; else goto if_merge_3086;
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
  if (r2) goto logic_merge_3092; else goto logic_rhs_3091;

  if_then_3089:
  return;
  goto if_merge_3090;

  if_merge_3090:
  struct 语句节点* cn_var_子语句_0;
  r5 = cn_var_语句;
  r6 = r5->块语句列表;
  cn_var_子语句_0 = r6;
  goto while_cond_3093;

  logic_rhs_3091:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3092;

  logic_merge_3092:
  if (r4) goto if_then_3089; else goto if_merge_3090;

  while_cond_3093:
  r7 = cn_var_子语句_0;
  if (r7) goto while_body_3094; else goto while_exit_3095;

  while_body_3094:
  r8 = cn_var_上下文;
  r9 = cn_var_子语句_0;
  生成语句IR(r8, r9);
  r10 = cn_var_子语句_0;
  r11 = r10->下一个;
  cn_var_子语句_0 = r11;
  goto while_cond_3093;

  while_exit_3095:
  return;
}

void 生成函数IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r6, r8, r11, r16, r25, r34;
  char* r14;
  char* r21;
  char* r23;
  char* r26;
  char* r27;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct 声明节点* r5;
  struct 声明节点* r7;
  struct IR函数* r9;
  struct 声明节点* r10;
  struct IR函数* r17;
  struct IR生成上下文* r19;
  struct IR函数* r28;
  struct 基本块* r29;
  struct IR生成上下文* r30;
  struct 基本块* r31;
  struct IR生成上下文* r32;
  struct 声明节点* r33;
  struct IR生成上下文* r35;
  struct IR模块* r36;
  struct IR函数* r37;
  struct IR生成上下文* r38;
  struct 参数 r12;
  struct 参数 r13;
  struct 参数 r15;
  struct IR操作数 r18;
  struct 参数 r20;
  struct 参数 r22;
  struct 参数 r24;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3099; else goto logic_rhs_3098;

  if_then_3096:
  return;
  goto if_merge_3097;

  if_merge_3097:
  重置局部变量计数器();
  struct IR函数* cn_var_当前IR函数_0;
  r5 = cn_var_声明;
  r6 = r5->函数名称;
  r7 = cn_var_声明;
  r8 = r7->返回类型;
  创建IR函数(r6, r8);
  cn_var_当前IR函数_0 = /* NONE */;
  r9 = cn_var_当前IR函数_0;
  struct 参数节点* cn_var_参数_1;
  r10 = cn_var_声明;
  r11 = r10->参数列表;
  cn_var_参数_1 = r11;
  goto while_cond_3100;

  logic_rhs_3098:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3099;

  logic_merge_3099:
  if (r4) goto if_then_3096; else goto if_merge_3097;

  while_cond_3100:
  r12 = cn_var_参数_1;
  if (r12) goto while_body_3101; else goto while_exit_3102;

  while_body_3101:
  struct IR操作数 cn_var_参数操作数_2;
  r13 = cn_var_参数_1;
  r14 = r13.名称;
  r15 = cn_var_参数_1;
  r16 = r15.类型信息;
  全局符号操作数(r14, r16);
  cn_var_参数操作数_2 = /* NONE */;
  r17 = cn_var_当前IR函数_0;
  r18 = cn_var_参数操作数_2;
  添加函数参数(r17, r18);
  r19 = cn_var_上下文;
  r20 = cn_var_参数_1;
  r21 = r20.名称;
  r22 = cn_var_参数_1;
  r23 = r22.名称;
  添加局部变量映射(r19, r21, r23);
  r24 = cn_var_参数_1;
  r25 = r24.下一个;
  cn_var_参数_1 = r25;
  goto while_cond_3100;

  while_exit_3102:
  char* cn_var_入口块名_3;
  生成块名称("entry");
  cn_var_入口块名_3 = /* NONE */;
  struct 基本块* cn_var_入口块_4;
  r26 = cn_var_入口块名_3;
  创建基本块(r26);
  cn_var_入口块_4 = /* NONE */;
  r27 = cn_var_入口块名_3;
  释放内存(r27);
  r28 = cn_var_当前IR函数_0;
  r29 = cn_var_入口块_4;
  添加基本块(r28, r29);
  r30 = cn_var_上下文;
  r31 = cn_var_入口块_4;
  切换基本块(r30, r31);
  r32 = cn_var_上下文;
  r33 = cn_var_声明;
  r34 = r33->函数体;
  生成语句IR(r32, r34);
  r35 = cn_var_上下文;
  r36 = r35->模块;
  r37 = cn_var_当前IR函数_0;
  添加函数(r36, r37);
  r38 = cn_var_上下文;
  释放局部变量映射表(r38);
  return;
}

void 生成程序IR(struct IR生成上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r0, r2, r4, r6, r17;
  struct IR生成上下文* r1;
  struct 程序节点* r3;
  struct 程序节点* r5;
  struct 声明节点* r7;
  struct 声明节点* r8;
  struct IR生成上下文* r12;
  struct 声明节点* r13;
  struct IR生成上下文* r14;
  struct 声明节点* r15;
  struct 声明节点* r16;
  _Bool r10;
  _Bool r11;
  enum 节点类型 r9;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3106; else goto logic_rhs_3105;

  if_then_3103:
  return;
  goto if_merge_3104;

  if_merge_3104:
  struct 声明节点* cn_var_声明_0;
  r5 = cn_var_程序;
  r6 = r5->声明列表;
  cn_var_声明_0 = r6;
  goto while_cond_3107;

  logic_rhs_3105:
  r3 = cn_var_程序;
  r4 = !r3;
  goto logic_merge_3106;

  logic_merge_3106:
  if (r4) goto if_then_3103; else goto if_merge_3104;

  while_cond_3107:
  r7 = cn_var_声明_0;
  if (r7) goto while_body_3108; else goto while_exit_3109;

  while_body_3108:
  r8 = cn_var_声明_0;
  r9 = r8->类型;
  r10 = r9 == -842150451;
  if (r10) goto case_body_3111; else goto switch_check_3114;

  while_exit_3109:

  switch_check_3114:
  r11 = r9 == -842150451;
  if (r11) goto case_body_3112; else goto case_default_3113;

  case_body_3111:
  r12 = cn_var_上下文;
  r13 = cn_var_声明_0;
  生成函数IR(r12, r13);
  goto switch_merge_3110;
  goto switch_merge_3110;

  case_body_3112:
  r14 = cn_var_上下文;
  r15 = cn_var_声明_0;
  生成全局变量IR(r14, r15);
  goto switch_merge_3110;
  goto switch_merge_3110;

  case_default_3113:
  goto switch_merge_3110;
  goto switch_merge_3110;

  switch_merge_3110:
  r16 = cn_var_声明_0;
  r17 = r16->下一个;
  cn_var_声明_0 = r17;
  goto while_cond_3107;
  return;
}

void 生成全局变量IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r6, r9, r11, r13, r16;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct 声明节点* r5;
  struct IR生成上下文* r7;
  struct 声明节点* r8;
  struct 声明节点* r10;
  struct 声明节点* r12;
  struct 声明节点* r15;
  struct IR生成上下文* r17;
  struct IR模块* r18;
  struct 全局变量* r19;
  struct IR操作数 r14;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3118; else goto logic_rhs_3117;

  if_then_3115:
  return;
  goto if_merge_3116;

  if_merge_3116:
  struct IR操作数 cn_var_初始值_0;
  空操作数();
  cn_var_初始值_0 = /* NONE */;
  r5 = cn_var_声明;
  r6 = r5->初始值表达式;
  if (r6) goto if_then_3119; else goto if_merge_3120;

  logic_rhs_3117:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3118;

  logic_merge_3118:
  if (r4) goto if_then_3115; else goto if_merge_3116;

  if_then_3119:
  r7 = cn_var_上下文;
  r8 = cn_var_声明;
  r9 = r8->初始值表达式;
  生成表达式IR(r7, r9);
  cn_var_初始值_0 = /* NONE */;
  goto if_merge_3120;

  if_merge_3120:
  struct 全局变量* cn_var_全局变量信息_1;
  r10 = cn_var_声明;
  r11 = r10->变量名称;
  r12 = cn_var_声明;
  r13 = r12->类型信息;
  r14 = cn_var_初始值_0;
  r15 = cn_var_声明;
  r16 = r15->是常量;
  创建全局变量(r11, r13, r14, r16);
  cn_var_全局变量信息_1 = /* NONE */;
  r17 = cn_var_上下文;
  r18 = r17->模块;
  r19 = cn_var_全局变量信息_1;
  添加全局变量(r18, r19);
  return;
}

