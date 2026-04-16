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
    节点类型_类型转换表达式 = 36,
    节点类型_基础类型 = 37,
    节点类型_指针类型 = 38,
    节点类型_数组类型 = 39,
    节点类型_函数类型 = 40,
    节点类型_结构体类型 = 41,
    节点类型_枚举类型 = 42,
    节点类型_类类型 = 43,
    节点类型_接口类型 = 44,
    节点类型_标识符类型 = 45
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
    类型种类_类型_未定义 = 23,
    类型种类_类型_自身 = 22,
    类型种类_类型_双精度小数 = 21,
    类型种类_类型_单精度小数 = 20,
    类型种类_类型_64位无符号LL = 19,
    类型种类_类型_64位无符号 = 18,
    类型种类_类型_32位无符号 = 17,
    类型种类_类型_64位整数 = 16,
    类型种类_类型_32位整数 = 15,
    类型种类_类型_内存地址 = 14,
    类型种类_类型_参数 = 13,
    类型种类_类型_函数 = 12,
    类型种类_类型_接口 = 11,
    类型种类_类型_类 = 10,
    类型种类_类型_枚举 = 9,
    类型种类_类型_结构体 = 8,
    类型种类_类型_数组 = 7,
    类型种类_类型_指针 = 6,
    类型种类_类型_字符串 = 5,
    类型种类_类型_布尔 = 4,
    类型种类_类型_字符 = 3,
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
    编译模式_目标模式 = 1,
    编译模式_宿主模式 = 0
};
enum IR操作码 {
    IR操作码_类型转换指令 = 35,
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
    struct 类型节点* 目标类型;
    struct 表达式节点* 转换操作数;
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
extern struct 源位置 cn_var_空源位置;
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
struct 表达式节点* 创建类型转换表达式(struct 类型节点*, struct 表达式节点*);
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
extern struct 源位置 cn_var_空源位置;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;

// CN Language Global Struct Forward Declarations
struct 常量折叠上下文;
struct 常量值;
struct 常量传播上下文;

// CN Language Global Struct Definitions
struct 常量折叠上下文 {
    long long 折叠次数;
    _Bool 已修改;
    struct IR模块* 模块;
};

struct 常量值 {
    _Bool 是常量;
    enum IR操作数种类 种类;
    long long 整数值;
    double 小数值;
    char* 字符串值;
    struct 类型节点* 类型信息;
};

struct 常量传播上下文 {
    struct 常量值* 寄存器值;
    long long 寄存器数量;
    long long 传播次数;
};

// Global Variables

// Forward Declarations
struct 常量值 非常量();
struct 常量值 创建整数常量值(long long, struct 类型节点*);
struct 常量值 创建小数常量值(double, struct 类型节点*);
struct 常量值 操作数转常量值(struct IR操作数);
struct IR操作数 常量值转操作数(struct 常量值);
struct 常量折叠上下文* 创建常量折叠上下文(struct IR模块*);
void 释放常量折叠上下文(struct 常量折叠上下文*);
long long 计算整数二元运算(enum IR操作码, long long, long long);
double 计算小数二元运算(enum IR操作码, double, double);
long long 计算整数一元运算(enum IR操作码, long long);
double 计算小数一元运算(enum IR操作码, double);
_Bool 折叠二元运算(struct IR指令*);
_Bool 折叠一元运算(struct IR指令*);
_Bool 折叠指令(struct IR指令*);
void 折叠基本块(struct 基本块*, struct 常量折叠上下文*);
void 折叠函数(struct IR函数*, struct 常量折叠上下文*);
long long 折叠模块(struct IR模块*);
long long 迭代常量折叠(struct IR模块*, long long);
struct 常量传播上下文* 创建常量传播上下文(long long);
void 释放常量传播上下文(struct 常量传播上下文*);
void 设置寄存器常量(struct 常量传播上下文*, long long, struct 常量值);
struct 常量值 获取寄存器常量(struct 常量传播上下文*, long long);
_Bool 传播操作数(struct IR操作数*, struct 常量传播上下文*);
void 传播指令(struct IR指令*, struct 常量传播上下文*);
void 传播基本块(struct 基本块*, struct 常量传播上下文*);
long long 传播函数(struct IR函数*);
long long 执行常量优化(struct IR模块*, long long);

struct 常量值 非常量() {
  struct 常量值 r0;

  entry:
  struct 常量值 cn_var_值_0;
  r0 = cn_var_值_0;
  return r0;
}

struct 常量值 创建整数常量值(long long cn_var_值, struct 类型节点* cn_var_类型) {
  long long r0;
  struct 类型节点* r1;
  struct 常量值 r2;

  entry:
  struct 常量值 cn_var_结果_0;
  r0 = cn_var_值;
  r1 = cn_var_类型;
  r2 = cn_var_结果_0;
  return r2;
}

struct 常量值 创建小数常量值(double cn_var_值, struct 类型节点* cn_var_类型) {
  struct 类型节点* r1;
  double r0;
  struct 常量值 r2;

  entry:
  struct 常量值 cn_var_结果_0;
  r0 = cn_var_值;
  r1 = cn_var_类型;
  r2 = cn_var_结果_0;
  return r2;
}

struct 常量值 操作数转常量值(struct IR操作数 cn_var_操作数) {
  long long r2;
  struct 类型节点* r3;
  struct 类型节点* r8;
  _Bool r1;
  _Bool r6;
  double r7;
  struct 常量值 r4;
  struct 常量值 r9;
  struct 常量值 r10;
  enum IR操作数种类 r0;
  enum IR操作数种类 r5;

  entry:
  r0 = cn_var_操作数.种类;
  r1 = r0 == IR操作数种类_整数常量;
  if (r1) goto if_then_0; else goto if_else_1;

  if_then_0:
  r2 = cn_var_操作数.整数值;
  r3 = cn_var_操作数.类型信息;
  r4 = 创建整数常量值(r2, r3);
  return r4;
  goto if_merge_2;

  if_else_1:
  r5 = cn_var_操作数.种类;
  r6 = r5 == IR操作数种类_小数常量;
  if (r6) goto if_then_3; else goto if_merge_4;

  if_merge_2:
  r10 = 非常量();
  return r10;

  if_then_3:
  r7 = cn_var_操作数.小数值;
  r8 = cn_var_操作数.类型信息;
  r9 = 创建小数常量值(r7, r8);
  return r9;
  goto if_merge_4;

  if_merge_4:
  goto if_merge_2;
  {
    static struct 常量值 _zero = {0};
    return _zero;
  }
}

struct IR操作数 常量值转操作数(struct 常量值 cn_var_值) {
  long long r5;
  struct 类型节点* r6;
  struct 类型节点* r11;
  _Bool r0;
  _Bool r1;
  _Bool r4;
  _Bool r9;
  double r10;
  struct IR操作数 r2;
  struct IR操作数 r7;
  struct IR操作数 r12;
  struct IR操作数 r13;
  enum IR操作数种类 r3;
  enum IR操作数种类 r8;

  entry:
  r0 = cn_var_值.是常量;
  r1 = !r0;
  if (r1) goto if_then_5; else goto if_merge_6;

  if_then_5:
  r2 = 空操作数();
  return r2;
  goto if_merge_6;

  if_merge_6:
  r3 = cn_var_值.种类;
  r4 = r3 == IR操作数种类_整数常量;
  if (r4) goto if_then_7; else goto if_else_8;

  if_then_7:
  r5 = cn_var_值.整数值;
  r6 = cn_var_值.类型信息;
  r7 = 整数常量操作数(r5, r6);
  return r7;
  goto if_merge_9;

  if_else_8:
  r8 = cn_var_值.种类;
  r9 = r8 == IR操作数种类_小数常量;
  if (r9) goto if_then_10; else goto if_merge_11;

  if_merge_9:
  r13 = 空操作数();
  return r13;

  if_then_10:
  r10 = cn_var_值.小数值;
  r11 = cn_var_值.类型信息;
  r12 = 小数常量操作数(r10, r11);
  return r12;
  goto if_merge_11;

  if_merge_11:
  goto if_merge_9;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct 常量折叠上下文* 创建常量折叠上下文(struct IR模块* cn_var_模块) {
  long long r0;
  void* r1;
  struct 常量折叠上下文* r2;
  struct IR模块* r3;
  struct 常量折叠上下文* r4;

  entry:
  struct 常量折叠上下文* cn_var_上下文_0;
  r0 = sizeof(struct 常量折叠上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_12; else goto if_merge_13;

  if_then_12:
  r3 = cn_var_模块;
  goto if_merge_13;

  if_merge_13:
  r4 = cn_var_上下文_0;
  return r4;
}

void 释放常量折叠上下文(struct 常量折叠上下文* cn_var_上下文) {
  struct 常量折叠上下文* r0;
  struct 常量折叠上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_14; else goto if_merge_15;

  if_then_14:
  r1 = cn_var_上下文;
  释放内存(r1);
  goto if_merge_15;

  if_merge_15:
  return;
}

long long 计算整数二元运算(enum IR操作码 cn_var_操作码, long long cn_var_左, long long cn_var_右) {
  long long r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r28, r29, r30, r31, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r54, r55, r56, r58, r59, r60, r62, r63, r64, r66, r67, r68, r70, r71, r72, r74;
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
  _Bool r27;
  _Bool r32;
  _Bool r53;
  _Bool r57;
  _Bool r61;
  _Bool r65;
  _Bool r69;
  _Bool r73;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_操作码;
  r1 = r0 == IR指令种类_加法指令;
  if (r1) goto case_body_17; else goto switch_check_34;

  switch_check_34:
  r2 = r0 == IR指令种类_减法指令;
  if (r2) goto case_body_18; else goto switch_check_35;

  switch_check_35:
  r3 = r0 == IR指令种类_乘法指令;
  if (r3) goto case_body_19; else goto switch_check_36;

  switch_check_36:
  r4 = r0 == IR指令种类_除法指令;
  if (r4) goto case_body_20; else goto switch_check_37;

  switch_check_37:
  r5 = r0 == IR指令种类_取模指令;
  if (r5) goto case_body_21; else goto switch_check_38;

  switch_check_38:
  r6 = r0 == IR指令种类_与指令;
  if (r6) goto case_body_22; else goto switch_check_39;

  switch_check_39:
  r7 = r0 == IR指令种类_或指令;
  if (r7) goto case_body_23; else goto switch_check_40;

  switch_check_40:
  r8 = r0 == IR指令种类_异或指令;
  if (r8) goto case_body_24; else goto switch_check_41;

  switch_check_41:
  r9 = r0 == IR指令种类_左移指令;
  if (r9) goto case_body_25; else goto switch_check_42;

  switch_check_42:
  r10 = r0 == IR指令种类_右移指令;
  if (r10) goto case_body_26; else goto switch_check_43;

  switch_check_43:
  r11 = r0 == IR指令种类_相等指令;
  if (r11) goto case_body_27; else goto switch_check_44;

  switch_check_44:
  r12 = r0 == IR指令种类_不等指令;
  if (r12) goto case_body_28; else goto switch_check_45;

  switch_check_45:
  r13 = r0 == IR指令种类_小于指令;
  if (r13) goto case_body_29; else goto switch_check_46;

  switch_check_46:
  r14 = r0 == IR指令种类_小于等于指令;
  if (r14) goto case_body_30; else goto switch_check_47;

  switch_check_47:
  r15 = r0 == IR指令种类_大于指令;
  if (r15) goto case_body_31; else goto switch_check_48;

  switch_check_48:
  r16 = r0 == IR指令种类_大于等于指令;
  if (r16) goto case_body_32; else goto case_default_33;

  case_body_17:
  r17 = cn_var_左;
  r18 = cn_var_右;
  r19 = r17 + r18;
  return r19;
  goto switch_merge_16;

  case_body_18:
  r20 = cn_var_左;
  r21 = cn_var_右;
  r22 = r20 - r21;
  return r22;
  goto switch_merge_16;

  case_body_19:
  r23 = cn_var_左;
  r24 = cn_var_右;
  r25 = r23 * r24;
  return r25;
  goto switch_merge_16;

  case_body_20:
  r26 = cn_var_右;
  r27 = r26 == 0;
  if (r27) goto if_then_49; else goto if_merge_50;

  if_then_49:
  return 0;
  goto if_merge_50;

  if_merge_50:
  r28 = cn_var_左;
  r29 = cn_var_右;
  r30 = r28 / r29;
  return r30;
  goto switch_merge_16;

  case_body_21:
  r31 = cn_var_右;
  r32 = r31 == 0;
  if (r32) goto if_then_51; else goto if_merge_52;

  if_then_51:
  return 0;
  goto if_merge_52;

  if_merge_52:
  r33 = cn_var_左;
  r34 = cn_var_右;
  r35 = r33 % r34;
  return r35;
  goto switch_merge_16;

  case_body_22:
  r36 = cn_var_左;
  r37 = cn_var_右;
  r38 = r36 & r37;
  return r38;
  goto switch_merge_16;

  case_body_23:
  r39 = cn_var_左;
  r40 = cn_var_右;
  r41 = r39 | r40;
  return r41;
  goto switch_merge_16;

  case_body_24:
  r42 = cn_var_左;
  r43 = cn_var_右;
  r44 = r42 ^ r43;
  return r44;
  goto switch_merge_16;

  case_body_25:
  r45 = cn_var_左;
  r46 = cn_var_右;
  r47 = r45 << r46;
  return r47;
  goto switch_merge_16;

  case_body_26:
  r48 = cn_var_左;
  r49 = cn_var_右;
  r50 = r48 >> r49;
  return r50;
  goto switch_merge_16;

  case_body_27:
  r51 = cn_var_左;
  r52 = cn_var_右;
  r53 = r51 == r52;
  r54 = (r53 ? 1 : 0);
  return r54;
  goto switch_merge_16;

  case_body_28:
  r55 = cn_var_左;
  r56 = cn_var_右;
  r57 = r55 != r56;
  r58 = (r57 ? 1 : 0);
  return r58;
  goto switch_merge_16;

  case_body_29:
  r59 = cn_var_左;
  r60 = cn_var_右;
  r61 = r59 < r60;
  r62 = (r61 ? 1 : 0);
  return r62;
  goto switch_merge_16;

  case_body_30:
  r63 = cn_var_左;
  r64 = cn_var_右;
  r65 = r63 <= r64;
  r66 = (r65 ? 1 : 0);
  return r66;
  goto switch_merge_16;

  case_body_31:
  r67 = cn_var_左;
  r68 = cn_var_右;
  r69 = r67 > r68;
  r70 = (r69 ? 1 : 0);
  return r70;
  goto switch_merge_16;

  case_body_32:
  r71 = cn_var_左;
  r72 = cn_var_右;
  r73 = r71 >= r72;
  r74 = (r73 ? 1 : 0);
  return r74;
  goto switch_merge_16;

  case_default_33:
  return 0;
  goto switch_merge_16;

  switch_merge_16:
  return 0;
}

double 计算小数二元运算(enum IR操作码 cn_var_操作码, double cn_var_左, double cn_var_右) {
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
  _Bool r21;
  _Bool r27;
  _Bool r31;
  _Bool r35;
  _Bool r39;
  _Bool r43;
  _Bool r47;
  double r11;
  double r12;
  double r13;
  double r14;
  double r15;
  double r16;
  double r17;
  double r18;
  double r19;
  double r20;
  double r22;
  double r23;
  double r24;
  double r25;
  double r26;
  double r28;
  double r29;
  double r30;
  double r32;
  double r33;
  double r34;
  double r36;
  double r37;
  double r38;
  double r40;
  double r41;
  double r42;
  double r44;
  double r45;
  double r46;
  double r48;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_操作码;
  r1 = r0 == IR指令种类_加法指令;
  if (r1) goto case_body_54; else goto switch_check_65;

  switch_check_65:
  r2 = r0 == IR指令种类_减法指令;
  if (r2) goto case_body_55; else goto switch_check_66;

  switch_check_66:
  r3 = r0 == IR指令种类_乘法指令;
  if (r3) goto case_body_56; else goto switch_check_67;

  switch_check_67:
  r4 = r0 == IR指令种类_除法指令;
  if (r4) goto case_body_57; else goto switch_check_68;

  switch_check_68:
  r5 = r0 == IR指令种类_相等指令;
  if (r5) goto case_body_58; else goto switch_check_69;

  switch_check_69:
  r6 = r0 == IR指令种类_不等指令;
  if (r6) goto case_body_59; else goto switch_check_70;

  switch_check_70:
  r7 = r0 == IR指令种类_小于指令;
  if (r7) goto case_body_60; else goto switch_check_71;

  switch_check_71:
  r8 = r0 == IR指令种类_小于等于指令;
  if (r8) goto case_body_61; else goto switch_check_72;

  switch_check_72:
  r9 = r0 == IR指令种类_大于指令;
  if (r9) goto case_body_62; else goto switch_check_73;

  switch_check_73:
  r10 = r0 == IR指令种类_大于等于指令;
  if (r10) goto case_body_63; else goto case_default_64;

  case_body_54:
  r11 = cn_var_左;
  r12 = cn_var_右;
  r13 = r11 + r12;
  return r13;
  goto switch_merge_53;

  case_body_55:
  r14 = cn_var_左;
  r15 = cn_var_右;
  r16 = r14 - r15;
  return r16;
  goto switch_merge_53;

  case_body_56:
  r17 = cn_var_左;
  r18 = cn_var_右;
  r19 = r17 * r18;
  return r19;
  goto switch_merge_53;

  case_body_57:
  r20 = cn_var_右;
  r21 = r20 == 0.000000;
  if (r21) goto if_then_74; else goto if_merge_75;

  if_then_74:
  return 0.000000;
  goto if_merge_75;

  if_merge_75:
  r22 = cn_var_左;
  r23 = cn_var_右;
  r24 = r22 / r23;
  return r24;
  goto switch_merge_53;

  case_body_58:
  r25 = cn_var_左;
  r26 = cn_var_右;
  r27 = r25 == r26;
  r28 = (r27 ? 1.000000 : 0.000000);
  return r28;
  goto switch_merge_53;

  case_body_59:
  r29 = cn_var_左;
  r30 = cn_var_右;
  r31 = r29 != r30;
  r32 = (r31 ? 1.000000 : 0.000000);
  return r32;
  goto switch_merge_53;

  case_body_60:
  r33 = cn_var_左;
  r34 = cn_var_右;
  r35 = r33 < r34;
  r36 = (r35 ? 1.000000 : 0.000000);
  return r36;
  goto switch_merge_53;

  case_body_61:
  r37 = cn_var_左;
  r38 = cn_var_右;
  r39 = r37 <= r38;
  r40 = (r39 ? 1.000000 : 0.000000);
  return r40;
  goto switch_merge_53;

  case_body_62:
  r41 = cn_var_左;
  r42 = cn_var_右;
  r43 = r41 > r42;
  r44 = (r43 ? 1.000000 : 0.000000);
  return r44;
  goto switch_merge_53;

  case_body_63:
  r45 = cn_var_左;
  r46 = cn_var_右;
  r47 = r45 >= r46;
  r48 = (r47 ? 1.000000 : 0.000000);
  return r48;
  goto switch_merge_53;

  case_default_64:
  return 0.000000;
  goto switch_merge_53;

  switch_merge_53:
  return 0.0;
}

long long 计算整数一元运算(enum IR操作码 cn_var_操作码, long long cn_var_值) {
  long long r4, r5, r6, r7, r8, r10, r11;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r9;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_操作码;
  r1 = r0 == IR指令种类_负号指令;
  if (r1) goto case_body_77; else goto switch_check_81;

  switch_check_81:
  r2 = r0 == IR指令种类_取反指令;
  if (r2) goto case_body_78; else goto switch_check_82;

  switch_check_82:
  r3 = r0 == IR指令种类_逻辑非指令;
  if (r3) goto case_body_79; else goto case_default_80;

  case_body_77:
  r4 = cn_var_值;
  r5 = -r4;
  return r5;
  goto switch_merge_76;

  case_body_78:
  r6 = cn_var_值;
  return r7;
  goto switch_merge_76;

  case_body_79:
  r8 = cn_var_值;
  r9 = r8 == 0;
  r10 = (r9 ? 1 : 0);
  return r10;
  goto switch_merge_76;

  case_default_80:
  r11 = cn_var_值;
  return r11;
  goto switch_merge_76;

  switch_merge_76:
  return 0;
}

double 计算小数一元运算(enum IR操作码 cn_var_操作码, double cn_var_值) {
  _Bool r1;
  double r2;
  double r3;
  double r4;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_操作码;
  r1 = r0 == IR指令种类_负号指令;
  if (r1) goto case_body_84; else goto case_default_85;

  case_body_84:
  r2 = cn_var_值;
  r3 = -r2;
  return r3;
  goto switch_merge_83;

  case_default_85:
  r4 = cn_var_值;
  return r4;
  goto switch_merge_83;

  switch_merge_83:
  return 0.0;
}

_Bool 折叠二元运算(struct IR指令* cn_var_指令) {
  long long r2, r9, r16, r24, r26, r30, r36, r49, r57;
  struct IR指令* r0;
  struct 类型节点* r33;
  struct 类型节点* r65;
  _Bool r1;
  _Bool r5;
  _Bool r8;
  _Bool r12;
  _Bool r15;
  _Bool r19;
  _Bool r22;
  _Bool r39;
  _Bool r42;
  _Bool r45;
  _Bool r53;
  double r28;
  double r29;
  double r31;
  double r47;
  double r50;
  double r55;
  double r58;
  double r60;
  double r61;
  double r62;
  double r63;
  struct IR操作数 r3;
  struct IR操作数 r6;
  struct IR操作数 r10;
  struct IR操作数 r13;
  struct IR操作数 r17;
  struct IR操作数 r20;
  struct IR操作数 r23;
  struct IR操作数 r25;
  struct IR操作数 r32;
  struct IR操作数 r34;
  struct IR操作数 r35;
  struct IR操作数 r37;
  struct IR操作数 r40;
  struct IR操作数 r43;
  struct IR操作数 r46;
  struct IR操作数 r48;
  struct IR操作数 r51;
  struct IR操作数 r54;
  struct IR操作数 r56;
  struct IR操作数 r64;
  struct IR操作数 r66;
  struct IR操作数 r67;
  enum IR操作数种类 r4;
  enum IR操作数种类 r7;
  enum IR操作数种类 r11;
  enum IR操作数种类 r14;
  enum IR操作数种类 r18;
  enum IR操作数种类 r21;
  enum IR操作码 r27;
  enum IR操作数种类 r38;
  enum IR操作数种类 r41;
  enum IR操作数种类 r44;
  enum IR操作数种类 r52;
  enum IR操作码 r59;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_86; else goto if_merge_87;

  if_then_86:
  return 0;
  goto if_merge_87;

  if_merge_87:
  r3 = cn_var_指令->源操作数1;
  r4 = r3.种类;
  r5 = r4 != IR操作数种类_整数常量;
  if (r5) goto logic_rhs_90; else goto logic_merge_91;

  if_then_88:
  return 0;
  goto if_merge_89;

  if_merge_89:
  r10 = cn_var_指令->源操作数2;
  r11 = r10.种类;
  r12 = r11 != IR操作数种类_整数常量;
  if (r12) goto logic_rhs_94; else goto logic_merge_95;

  logic_rhs_90:
  r6 = cn_var_指令->源操作数1;
  r7 = r6.种类;
  r8 = r7 != IR操作数种类_小数常量;
  goto logic_merge_91;

  logic_merge_91:
  if (r8) goto if_then_88; else goto if_merge_89;

  if_then_92:
  return 0;
  goto if_merge_93;

  if_merge_93:
  r17 = cn_var_指令->源操作数1;
  r18 = r17.种类;
  r19 = r18 == IR操作数种类_整数常量;
  if (r19) goto logic_rhs_98; else goto logic_merge_99;

  logic_rhs_94:
  r13 = cn_var_指令->源操作数2;
  r14 = r13.种类;
  r15 = r14 != IR操作数种类_小数常量;
  goto logic_merge_95;

  logic_merge_95:
  if (r15) goto if_then_92; else goto if_merge_93;

  if_then_96:
  long long cn_var_左_0;
  r23 = cn_var_指令->源操作数1;
  r24 = r23.整数值;
  cn_var_左_0 = r24;
  long long cn_var_右_1;
  r25 = cn_var_指令->源操作数2;
  r26 = r25.整数值;
  cn_var_右_1 = r26;
  long long cn_var_结果_2;
  r27 = cn_var_指令->操作码;
  r28 = cn_var_左_0;
  r29 = cn_var_右_1;
  r30 = 计算整数二元运算(r27, r28, r29);
  cn_var_结果_2 = r30;
  r31 = cn_var_结果_2;
  r32 = cn_var_指令->目标;
  r33 = r32.类型信息;
  r34 = 整数常量操作数(r31, r33);
  r35 = 空操作数();
  return 1;
  goto if_merge_97;

  if_merge_97:
  r37 = cn_var_指令->源操作数1;
  r38 = r37.种类;
  r39 = r38 == IR操作数种类_小数常量;
  if (r39) goto logic_merge_103; else goto logic_rhs_102;

  logic_rhs_98:
  r20 = cn_var_指令->源操作数2;
  r21 = r20.种类;
  r22 = r21 == IR操作数种类_整数常量;
  goto logic_merge_99;

  logic_merge_99:
  if (r22) goto if_then_96; else goto if_merge_97;

  if_then_100:
  double cn_var_左_3;
  r43 = cn_var_指令->源操作数1;
  r44 = r43.种类;
  r45 = r44 == IR操作数种类_小数常量;
  r46 = cn_var_指令->源操作数1;
  r47 = r46.小数值;
  r48 = cn_var_指令->源操作数1;
  r49 = r48.整数值;
  r50 = (r45 ? r47 : r49);
  cn_var_左_3 = r50;
  double cn_var_右_4;
  r51 = cn_var_指令->源操作数2;
  r52 = r51.种类;
  r53 = r52 == IR操作数种类_小数常量;
  r54 = cn_var_指令->源操作数2;
  r55 = r54.小数值;
  r56 = cn_var_指令->源操作数2;
  r57 = r56.整数值;
  r58 = (r53 ? r55 : r57);
  cn_var_右_4 = r58;
  double cn_var_结果_5;
  r59 = cn_var_指令->操作码;
  r60 = cn_var_左_3;
  r61 = cn_var_右_4;
  r62 = 计算小数二元运算(r59, r60, r61);
  cn_var_结果_5 = r62;
  r63 = cn_var_结果_5;
  r64 = cn_var_指令->目标;
  r65 = r64.类型信息;
  r66 = 小数常量操作数(r63, r65);
  r67 = 空操作数();
  return 1;
  goto if_merge_101;

  if_merge_101:
  return 0;

  logic_rhs_102:
  r40 = cn_var_指令->源操作数2;
  r41 = r40.种类;
  r42 = r41 == IR操作数种类_小数常量;
  goto logic_merge_103;

  logic_merge_103:
  if (r42) goto if_then_100; else goto if_merge_101;
  return 0;
}

_Bool 折叠一元运算(struct IR指令* cn_var_指令) {
  long long r2, r13, r16;
  struct IR指令* r0;
  struct 类型节点* r19;
  struct 类型节点* r32;
  _Bool r1;
  _Bool r5;
  _Bool r8;
  _Bool r11;
  _Bool r24;
  double r15;
  double r17;
  double r26;
  double r28;
  double r29;
  double r30;
  struct IR操作数 r3;
  struct IR操作数 r6;
  struct IR操作数 r9;
  struct IR操作数 r12;
  struct IR操作数 r18;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r25;
  struct IR操作数 r31;
  struct IR操作数 r33;
  struct IR操作数 r34;
  enum IR操作数种类 r4;
  enum IR操作数种类 r7;
  enum IR操作数种类 r10;
  enum IR操作码 r14;
  enum IR操作数种类 r23;
  enum IR操作码 r27;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_104; else goto if_merge_105;

  if_then_104:
  return 0;
  goto if_merge_105;

  if_merge_105:
  r3 = cn_var_指令->源操作数1;
  r4 = r3.种类;
  r5 = r4 != IR操作数种类_整数常量;
  if (r5) goto logic_rhs_108; else goto logic_merge_109;

  if_then_106:
  return 0;
  goto if_merge_107;

  if_merge_107:
  r9 = cn_var_指令->源操作数1;
  r10 = r9.种类;
  r11 = r10 == IR操作数种类_整数常量;
  if (r11) goto if_then_110; else goto if_merge_111;

  logic_rhs_108:
  r6 = cn_var_指令->源操作数1;
  r7 = r6.种类;
  r8 = r7 != IR操作数种类_小数常量;
  goto logic_merge_109;

  logic_merge_109:
  if (r8) goto if_then_106; else goto if_merge_107;

  if_then_110:
  long long cn_var_值_0;
  r12 = cn_var_指令->源操作数1;
  r13 = r12.整数值;
  cn_var_值_0 = r13;
  long long cn_var_结果_1;
  r14 = cn_var_指令->操作码;
  r15 = cn_var_值_0;
  r16 = 计算整数一元运算(r14, r15);
  cn_var_结果_1 = r16;
  r17 = cn_var_结果_1;
  r18 = cn_var_指令->目标;
  r19 = r18.类型信息;
  r20 = 整数常量操作数(r17, r19);
  r21 = 空操作数();
  return 1;
  goto if_merge_111;

  if_merge_111:
  r22 = cn_var_指令->源操作数1;
  r23 = r22.种类;
  r24 = r23 == IR操作数种类_小数常量;
  if (r24) goto if_then_112; else goto if_merge_113;

  if_then_112:
  double cn_var_值_2;
  r25 = cn_var_指令->源操作数1;
  r26 = r25.小数值;
  cn_var_值_2 = r26;
  double cn_var_结果_3;
  r27 = cn_var_指令->操作码;
  r28 = cn_var_值_2;
  r29 = 计算小数一元运算(r27, r28);
  cn_var_结果_3 = r29;
  r30 = cn_var_结果_3;
  r31 = cn_var_指令->目标;
  r32 = r31.类型信息;
  r33 = 小数常量操作数(r30, r32);
  r34 = 空操作数();
  return 1;
  goto if_merge_113;

  if_merge_113:
  return 0;
}

_Bool 折叠指令(struct IR指令* cn_var_指令) {
  struct IR指令* r0;
  struct IR指令* r22;
  struct IR指令* r24;
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
  _Bool r23;
  _Bool r25;
  enum IR操作码 r2;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_114; else goto if_merge_115;

  if_then_114:
  return 0;
  goto if_merge_115;

  if_merge_115:
  r2 = cn_var_指令->操作码;
  r3 = r2 == IR指令种类_加法指令;
  if (r3) goto case_body_117; else goto switch_check_137;

  switch_check_137:
  r4 = r2 == IR指令种类_减法指令;
  if (r4) goto case_body_118; else goto switch_check_138;

  switch_check_138:
  r5 = r2 == IR指令种类_乘法指令;
  if (r5) goto case_body_119; else goto switch_check_139;

  switch_check_139:
  r6 = r2 == IR指令种类_除法指令;
  if (r6) goto case_body_120; else goto switch_check_140;

  switch_check_140:
  r7 = r2 == IR指令种类_取模指令;
  if (r7) goto case_body_121; else goto switch_check_141;

  switch_check_141:
  r8 = r2 == IR指令种类_与指令;
  if (r8) goto case_body_122; else goto switch_check_142;

  switch_check_142:
  r9 = r2 == IR指令种类_或指令;
  if (r9) goto case_body_123; else goto switch_check_143;

  switch_check_143:
  r10 = r2 == IR指令种类_异或指令;
  if (r10) goto case_body_124; else goto switch_check_144;

  switch_check_144:
  r11 = r2 == IR指令种类_左移指令;
  if (r11) goto case_body_125; else goto switch_check_145;

  switch_check_145:
  r12 = r2 == IR指令种类_右移指令;
  if (r12) goto case_body_126; else goto switch_check_146;

  switch_check_146:
  r13 = r2 == IR指令种类_相等指令;
  if (r13) goto case_body_127; else goto switch_check_147;

  switch_check_147:
  r14 = r2 == IR指令种类_不等指令;
  if (r14) goto case_body_128; else goto switch_check_148;

  switch_check_148:
  r15 = r2 == IR指令种类_小于指令;
  if (r15) goto case_body_129; else goto switch_check_149;

  switch_check_149:
  r16 = r2 == IR指令种类_小于等于指令;
  if (r16) goto case_body_130; else goto switch_check_150;

  switch_check_150:
  r17 = r2 == IR指令种类_大于指令;
  if (r17) goto case_body_131; else goto switch_check_151;

  switch_check_151:
  r18 = r2 == IR指令种类_大于等于指令;
  if (r18) goto case_body_132; else goto switch_check_152;

  switch_check_152:
  r19 = r2 == IR指令种类_负号指令;
  if (r19) goto case_body_133; else goto switch_check_153;

  switch_check_153:
  r20 = r2 == IR指令种类_取反指令;
  if (r20) goto case_body_134; else goto switch_check_154;

  switch_check_154:
  r21 = r2 == IR指令种类_逻辑非指令;
  if (r21) goto case_body_135; else goto case_default_136;

  case_body_117:
  goto switch_merge_116;

  case_body_118:
  goto switch_merge_116;

  case_body_119:
  goto switch_merge_116;

  case_body_120:
  goto switch_merge_116;

  case_body_121:
  goto switch_merge_116;

  case_body_122:
  goto switch_merge_116;

  case_body_123:
  goto switch_merge_116;

  case_body_124:
  goto switch_merge_116;

  case_body_125:
  goto switch_merge_116;

  case_body_126:
  goto switch_merge_116;

  case_body_127:
  goto switch_merge_116;

  case_body_128:
  goto switch_merge_116;

  case_body_129:
  goto switch_merge_116;

  case_body_130:
  goto switch_merge_116;

  case_body_131:
  goto switch_merge_116;

  case_body_132:
  r22 = cn_var_指令;
  r23 = 折叠二元运算(r22);
  return r23;
  goto switch_merge_116;

  case_body_133:
  goto switch_merge_116;

  case_body_134:
  goto switch_merge_116;

  case_body_135:
  r24 = cn_var_指令;
  r25 = 折叠一元运算(r24);
  return r25;
  goto switch_merge_116;

  case_default_136:
  return 0;
  goto switch_merge_116;

  switch_merge_116:
  return 0;
}

void 折叠基本块(struct 基本块* cn_var_块, struct 常量折叠上下文* cn_var_上下文) {
  long long r0, r9, r10;
  struct 基本块* r1;
  struct 常量折叠上下文* r3;
  struct IR指令* r5;
  struct IR指令* r6;
  struct IR指令* r7;
  struct IR指令* r11;
  _Bool r2;
  _Bool r4;
  _Bool r8;

  entry:
  r1 = cn_var_块;
  r2 = !r1;
  if (r2) goto logic_merge_158; else goto logic_rhs_157;

  if_then_155:
  return;
  goto if_merge_156;

  if_merge_156:
  struct IR指令* cn_var_指令_0;
  r5 = cn_var_块->首条指令;
  cn_var_指令_0 = r5;
  goto while_cond_159;

  logic_rhs_157:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_158;

  logic_merge_158:
  if (r4) goto if_then_155; else goto if_merge_156;

  while_cond_159:
  r6 = cn_var_指令_0;
  if (r6) goto while_body_160; else goto while_exit_161;

  while_body_160:
  r7 = cn_var_指令_0;
  r8 = 折叠指令(r7);
  if (r8) goto if_then_162; else goto if_merge_163;

  while_exit_161:

  if_then_162:
  r9 = cn_var_上下文->折叠次数;
  r10 = r9 + 1;
  goto if_merge_163;

  if_merge_163:
  r11 = cn_var_指令_0->下一条;
  cn_var_指令_0 = r11;
  goto while_cond_159;
  return;
}

void 折叠函数(struct IR函数* cn_var_函数指针, struct 常量折叠上下文* cn_var_上下文) {
  long long r0;
  struct IR函数* r1;
  struct 常量折叠上下文* r3;
  struct 基本块* r5;
  struct 基本块* r6;
  struct 基本块* r7;
  struct 常量折叠上下文* r8;
  struct 基本块* r9;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_167; else goto logic_rhs_166;

  if_then_164:
  return;
  goto if_merge_165;

  if_merge_165:
  struct 基本块* cn_var_块_0;
  r5 = cn_var_函数指针->首个块;
  cn_var_块_0 = r5;
  goto while_cond_168;

  logic_rhs_166:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_167;

  logic_merge_167:
  if (r4) goto if_then_164; else goto if_merge_165;

  while_cond_168:
  r6 = cn_var_块_0;
  if (r6) goto while_body_169; else goto while_exit_170;

  while_body_169:
  r7 = cn_var_块_0;
  r8 = cn_var_上下文;
  折叠基本块(r7, r8);
  r9 = cn_var_块_0->下一个;
  cn_var_块_0 = r9;
  goto while_cond_168;

  while_exit_170:
  return;
}

long long 折叠模块(struct IR模块* cn_var_模块) {
  long long r11, r13;
  struct IR模块* r0;
  struct IR模块* r2;
  struct 常量折叠上下文* r3;
  struct 常量折叠上下文* r4;
  struct IR函数* r6;
  struct IR函数* r7;
  struct IR函数* r8;
  struct 常量折叠上下文* r9;
  struct IR函数* r10;
  struct 常量折叠上下文* r12;
  _Bool r1;
  _Bool r5;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_171; else goto if_merge_172;

  if_then_171:
  return 0;
  goto if_merge_172;

  if_merge_172:
  struct 常量折叠上下文* cn_var_上下文_0;
  r2 = cn_var_模块;
  r3 = 创建常量折叠上下文(r2);
  cn_var_上下文_0 = r3;
  r4 = cn_var_上下文_0;
  r5 = !r4;
  if (r5) goto if_then_173; else goto if_merge_174;

  if_then_173:
  return 0;
  goto if_merge_174;

  if_merge_174:
  struct IR函数* cn_var_函数指针_1;
  r6 = cn_var_模块->首个函数;
  cn_var_函数指针_1 = r6;
  goto while_cond_175;

  while_cond_175:
  r7 = cn_var_函数指针_1;
  if (r7) goto while_body_176; else goto while_exit_177;

  while_body_176:
  r8 = cn_var_函数指针_1;
  r9 = cn_var_上下文_0;
  折叠函数(r8, r9);
  r10 = cn_var_函数指针_1->下一个;
  cn_var_函数指针_1 = r10;
  goto while_cond_175;

  while_exit_177:
  long long cn_var_次数_2;
  r11 = cn_var_上下文_0->折叠次数;
  cn_var_次数_2 = r11;
  r12 = cn_var_上下文_0;
  释放常量折叠上下文(r12);
  r13 = cn_var_次数_2;
  return r13;
}

long long 迭代常量折叠(struct IR模块* cn_var_模块, long long cn_var_最大迭代次数) {
  long long r2, r3, r6, r7, r8, r9, r10, r12, r13, r14;
  struct IR模块* r0;
  struct IR模块* r5;
  _Bool r1;
  _Bool r4;
  _Bool r11;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_178; else goto if_merge_179;

  if_then_178:
  return 0;
  goto if_merge_179;

  if_merge_179:
  long long cn_var_总次数_0;
  cn_var_总次数_0 = 0;
  long long cn_var_迭代_1;
  cn_var_迭代_1 = 0;
  goto while_cond_180;

  while_cond_180:
  r2 = cn_var_迭代_1;
  r3 = cn_var_最大迭代次数;
  r4 = r2 < r3;
  if (r4) goto while_body_181; else goto while_exit_182;

  while_body_181:
  long long cn_var_次数_2;
  r5 = cn_var_模块;
  r6 = 折叠模块(r5);
  cn_var_次数_2 = r6;
  r7 = cn_var_总次数_0;
  r8 = cn_var_次数_2;
  r9 = r7 + r8;
  cn_var_总次数_0 = r9;
  r10 = cn_var_次数_2;
  r11 = r10 == 0;
  if (r11) goto if_then_183; else goto if_merge_184;

  while_exit_182:
  r14 = cn_var_总次数_0;
  return r14;

  if_then_183:
  goto while_exit_182;
  goto if_merge_184;

  if_merge_184:
  r12 = cn_var_迭代_1;
  r13 = r12 + 1;
  cn_var_迭代_1 = r13;
  goto while_cond_180;
  return 0;
}

struct 常量传播上下文* 创建常量传播上下文(long long cn_var_寄存器数量) {
  long long r0, r3, r4, r6, r7, r8, r12, r14, r15;
  void* r1;
  struct 常量传播上下文* r2;
  void* r5;
  struct 常量值* r11;
  struct 常量值* r13;
  struct 常量传播上下文* r16;
  _Bool r9;
  struct 常量值 r10;

  entry:
  struct 常量传播上下文* cn_var_上下文_0;
  r0 = sizeof(struct 常量传播上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_185; else goto if_merge_186;

  if_then_185:
  r3 = cn_var_寄存器数量;
  r4 = sizeof(struct 常量值);
  r5 = 分配清零内存(r3, r4);
  r6 = cn_var_寄存器数量;
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto while_cond_187;

  if_merge_186:
  r16 = cn_var_上下文_0;
  return r16;

  while_cond_187:
  r7 = cn_var_i_1;
  r8 = cn_var_寄存器数量;
  r9 = r7 < r8;
  if (r9) goto while_body_188; else goto while_exit_189;

  while_body_188:
  r10 = 非常量();
  r11 = cn_var_上下文_0->寄存器值;
  r12 = cn_var_i_1;
  r13 = &r11[r12];
  *r13 = r10;
  r14 = cn_var_i_1;
  r15 = r14 + 1;
  cn_var_i_1 = r15;
  goto while_cond_187;

  while_exit_189:
  goto if_merge_186;
  return NULL;
}

void 释放常量传播上下文(struct 常量传播上下文* cn_var_上下文) {
  struct 常量传播上下文* r0;
  struct 常量值* r1;
  struct 常量值* r2;
  struct 常量传播上下文* r3;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_190; else goto if_merge_191;

  if_then_190:
  r1 = cn_var_上下文->寄存器值;
  if (r1) goto if_then_192; else goto if_merge_193;

  if_merge_191:

  if_then_192:
  r2 = cn_var_上下文->寄存器值;
  释放内存(r2);
  goto if_merge_193;

  if_merge_193:
  r3 = cn_var_上下文;
  释放内存(r3);
  goto if_merge_191;
  return;
}

void 设置寄存器常量(struct 常量传播上下文* cn_var_上下文, long long cn_var_寄存器, struct 常量值 cn_var_值) {
  long long r0, r1, r4, r6, r7, r11;
  struct 常量传播上下文* r2;
  struct 常量值* r10;
  struct 常量值* r12;
  _Bool r3;
  _Bool r5;
  _Bool r8;
  struct 常量值 r9;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_199; else goto logic_rhs_198;

  if_then_194:
  return;
  goto if_merge_195;

  if_merge_195:
  r9 = cn_var_值;
  r10 = cn_var_上下文->寄存器值;
  r11 = cn_var_寄存器;
  r12 = &r10[r11];
  *r12 = r9;

  logic_rhs_196:
  r6 = cn_var_寄存器;
  r7 = cn_var_上下文->寄存器数量;
  r8 = r6 >= r7;
  goto logic_merge_197;

  logic_merge_197:
  if (r8) goto if_then_194; else goto if_merge_195;

  logic_rhs_198:
  r4 = cn_var_寄存器;
  r5 = r4 < 0;
  goto logic_merge_199;

  logic_merge_199:
  if (r5) goto logic_merge_197; else goto logic_rhs_196;
  return;
}

struct 常量值 获取寄存器常量(struct 常量传播上下文* cn_var_上下文, long long cn_var_寄存器) {
  long long r0, r1, r4, r6, r7, r11;
  struct 常量传播上下文* r2;
  struct 常量值* r10;
  _Bool r3;
  _Bool r5;
  _Bool r8;
  struct 常量值 r9;
  struct 常量值 r12;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_205; else goto logic_rhs_204;

  if_then_200:
  r9 = 非常量();
  return r9;
  goto if_merge_201;

  if_merge_201:
  r10 = cn_var_上下文->寄存器值;
  r11 = cn_var_寄存器;
  r12 = (struct 常量值 {
    布尔 是常量;             
    IR操作数种类 种类;       
    整数 整数值;             
    小数 小数值;             
    字符串 字符串值;         
    类型节点* 类型信息;      
}






函数 非常量() -> 常量值 {
    常量值 值;
    值.是常量 = 假;
    值.种类 = 无操作数;
    值.整数值 = 0;
    值.小数值 = 0.0;
    值.字符串值 = 无;
    值.类型信息 = 无;
    返回 值;
}



函数 创建整数常量值(整数 值, 类型节点* 类型) -> 常量值 {
    常量值 结果;
    结果.是常量 = 真;
    结果.种类 = 整数常量;
    结果.整数值 = 值;
    结果.小数值 = 0.0;
    结果.字符串值 = 无;
    结果.类型信息 = 类型;
    返回 结果;
}



函数 创建小数常量值(小数 值, 类型节点* 类型) -> 常量值 {
    常量值 结果;
    结果.是常量 = 真;
    结果.种类 = 小数常量;
    结果.整数值 = 0;
    结果.小数值 = 值;
    结果.字符串值 = 无;
    结果.类型信息 = 类型;
    返回 结果;
}


函数 操作数转常量值(IR操作数 操作数) -> 常量值 {
    如果 (操作数.种类 == 整数常量) {
        返回 创建整数常量值(操作数.整数值, 操作数.类型信息);
    } 否则 如果 (操作数.种类 == 小数常量) {
        返回 创建小数常量值(操作数.小数值, 操作数.类型信息);
    }
    返回 非常量();
}


函数 常量值转操作数(常量值 值) -> IR操作数 {
    如果 (!值.是常量) {
        返回 空操作数();
    }
    
    如果 (值.种类 == 整数常量) {
        返回 整数常量操作数(值.整数值, 值.类型信息);
    } 否则 如果 (值.种类 == 小数常量) {
        返回 小数常量操作数(值.小数值, 值.类型信息);
    }
    返回 空操作数();
}






函数 创建常量折叠上下文(IR模块* 模块) -> 常量折叠上下文* {
    常量折叠上下文* 上下文 = 分配内存(类型大小(常量折叠上下文));
    如果 (上下文) {
        上下文.折叠次数 = 0;
        上下文.已修改 = 假;
        上下文.模块 = 模块;
    }
    返回 上下文;
}


函数 释放常量折叠上下文(常量折叠上下文* 上下文) -> 空类型 {
    如果 (上下文) {
        释放内存(上下文);
    }
}






函数 计算整数二元运算(IR操作码 操作码, 整数 左, 整数 右) -> 整数 {
    选择 (操作码) {
        情况 加法指令: 返回 左 + 右;
        情况 减法指令: 返回 左 - 右;
        情况 乘法指令: 返回 左 * 右;
        情况 除法指令:
            如果 (右 == 0) { 返回 0; }
            返回 左 / 右;
        情况 取模指令:
            如果 (右 == 0) { 返回 0; }
            返回 左 % 右;
        情况 与指令: 返回 左 & 右;
        情况 或指令: 返回 左 | 右;
        情况 异或指令: 返回 左 ^ 右;
        情况 左移指令: 返回 左 << 右;
        情况 右移指令: 返回 左 >> 右;
        情况 相等指令: 返回 左 == 右 ? 1 : 0;
        情况 不等指令: 返回 左 != 右 ? 1 : 0;
        情况 小于指令: 返回 左 < 右 ? 1 : 0;
        情况 小于等于指令: 返回 左 <= 右 ? 1 : 0;
        情况 大于指令: 返回 左 > 右 ? 1 : 0;
        情况 大于等于指令: 返回 左 >= 右 ? 1 : 0;
        默认: 返回 0;
    }
}


函数 计算小数二元运算(IR操作码 操作码, 小数 左, 小数 右) -> 小数 {
    选择 (操作码) {
        情况 加法指令: 返回 左 + 右;
        情况 减法指令: 返回 左 - 右;
        情况 乘法指令: 返回 左 * 右;
        情况 除法指令:
            如果 (右 == 0.0) { 返回 0.0; }
            返回 左 / 右;
        情况 相等指令: 返回 左 == 右 ? 1.0 : 0.0;
        情况 不等指令: 返回 左 != 右 ? 1.0 : 0.0;
        情况 小于指令: 返回 左 < 右 ? 1.0 : 0.0;
        情况 小于等于指令: 返回 左 <= 右 ? 1.0 : 0.0;
        情况 大于指令: 返回 左 > 右 ? 1.0 : 0.0;
        情况 大于等于指令: 返回 左 >= 右 ? 1.0 : 0.0;
        默认: 返回 0.0;
    }
}


函数 计算整数一元运算(IR操作码 操作码, 整数 值) -> 整数 {
    选择 (操作码) {
        情况 负号指令: 返回 -值;
        情况 取反指令: 返回 ~值;
        情况 逻辑非指令: 返回 值 == 0 ? 1 : 0;
        默认: 返回 值;
    }
}


函数 计算小数一元运算(IR操作码 操作码, 小数 值) -> 小数 {
    选择 (操作码) {
        情况 负号指令: 返回 -值;
        默认: 返回 值;
    }
}






函数 折叠二元运算(IR指令* 指令) -> 布尔 {
    如果 (!指令) {
        返回 假;
    }
    
    
    如果 (指令.源操作数1.种类 != 整数常量 && 指令.源操作数1.种类 != 小数常量) {
        返回 假;
    }
    如果 (指令.源操作数2.种类 != 整数常量 && 指令.源操作数2.种类 != 小数常量) {
        返回 假;
    }
    
    
    如果 (指令.源操作数1.种类 == 整数常量 && 指令.源操作数2.种类 == 整数常量) {
        整数 左 = 指令.源操作数1.整数值;
        整数 右 = 指令.源操作数2.整数值;
        整数 结果 = 计算整数二元运算(指令.操作码, 左, 右);
        
        
        指令.操作码 = 移动指令;
        指令.源操作数1 = 整数常量操作数(结果, 指令.目标.类型信息);
        指令.源操作数2 = 空操作数();
        返回 真;
    }
    
    
    如果 (指令.源操作数1.种类 == 小数常量 || 指令.源操作数2.种类 == 小数常量) {
        小数 左 = 指令.源操作数1.种类 == 小数常量 ? 指令.源操作数1.小数值 : 指令.源操作数1.整数值;
        小数 右 = 指令.源操作数2.种类 == 小数常量 ? 指令.源操作数2.小数值 : 指令.源操作数2.整数值;
        小数 结果 = 计算小数二元运算(指令.操作码, 左, 右);
        
        
        指令.操作码 = 移动指令;
        指令.源操作数1 = 小数常量操作数(结果, 指令.目标.类型信息);
        指令.源操作数2 = 空操作数();
        返回 真;
    }
    
    返回 假;
}


函数 折叠一元运算(IR指令* 指令) -> 布尔 {
    如果 (!指令) {
        返回 假;
    }
    
    
    如果 (指令.源操作数1.种类 != 整数常量 && 指令.源操作数1.种类 != 小数常量) {
        返回 假;
    }
    
    
    如果 (指令.源操作数1.种类 == 整数常量) {
        整数 值 = 指令.源操作数1.整数值;
        整数 结果 = 计算整数一元运算(指令.操作码, 值);
        
        
        指令.操作码 = 移动指令;
        指令.源操作数1 = 整数常量操作数(结果, 指令.目标.类型信息);
        指令.源操作数2 = 空操作数();
        返回 真;
    }
    
    
    如果 (指令.源操作数1.种类 == 小数常量) {
        小数 值 = 指令.源操作数1.小数值;
        小数 结果 = 计算小数一元运算(指令.操作码, 值);
        
        
        指令.操作码 = 移动指令;
        指令.源操作数1 = 小数常量操作数(结果, 指令.目标.类型信息);
        指令.源操作数2 = 空操作数();
        返回 真;
    }
    
    返回 假;
}


函数 折叠指令(IR指令* 指令) -> 布尔 {
    如果 (!指令) {
        返回 假;
    }
    
    选择 (指令.操作码) {
        
        情况 加法指令:
        情况 减法指令:
        情况 乘法指令:
        情况 除法指令:
        情况 取模指令:
        情况 与指令:
        情况 或指令:
        情况 异或指令:
        情况 左移指令:
        情况 右移指令:
        情况 相等指令:
        情况 不等指令:
        情况 小于指令:
        情况 小于等于指令:
        情况 大于指令:
        情况 大于等于指令:
            返回 折叠二元运算(指令);
            
        
        情况 负号指令:
        情况 取反指令:
        情况 逻辑非指令:
            返回 折叠一元运算(指令);
            
        默认:
            返回 假;
    }
}






函数 折叠基本块(基本块* 块, 常量折叠上下文* 上下文) -> 空类型 {
    如果 (!块 || !上下文) {
        返回;
    }
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        如果 (折叠指令(指令)) {
            上下文.折叠次数 = 上下文.折叠次数 + 1;
            上下文.已修改 = 真;
        }
        指令 = 指令.下一条;
    }
}






函数 折叠函数(IR函数* 函数指针, 常量折叠上下文* 上下文) -> 空类型 {
    如果 (!函数指针 || !上下文) {
        返回;
    }
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        折叠基本块(块, 上下文);
        块 = 块.下一个;
    }
}






函数 折叠模块(IR模块* 模块) -> 整数 {
    如果 (!模块) {
        返回 0;
    }
    
    常量折叠上下文* 上下文 = 创建常量折叠上下文(模块);
    如果 (!上下文) {
        返回 0;
    }
    
    
    IR函数* 函数指针 = 模块.首个函数;
    当 (函数指针) {
        折叠函数(函数指针, 上下文);
        函数指针 = 函数指针.下一个;
    }
    
    整数 次数 = 上下文.折叠次数;
    释放常量折叠上下文(上下文);
    返回 次数;
}









函数 迭代常量折叠(IR模块* 模块, 整数 最大迭代次数) -> 整数 {
    如果 (!模块) {
        返回 0;
    }
    
    整数 总次数 = 0;
    整数 迭代 = 0;
    
    当 (迭代 < 最大迭代次数) {
        整数 次数 = 折叠模块(模块);
        总次数 = 总次数 + 次数;
        
        
        如果 (次数 == 0) {
            中断;
        }
        
        迭代 = 迭代 + 1;
    }
    
    返回 总次数;
}






结构体 常量传播上下文 {
    常量值* 寄存器值;       
    整数 寄存器数量;        
    整数 传播次数;          
}


函数 创建常量传播上下文(整数 寄存器数量) -> 常量传播上下文* {
    常量传播上下文* 上下文 = 分配内存(类型大小(常量传播上下文));
    如果 (上下文) {
        上下文.寄存器值 = 分配清零内存(寄存器数量, 类型大小(常量值));
        上下文.寄存器数量 = 寄存器数量;
        上下文.传播次数 = 0;
        
        
        整数 i = 0;
        当 (i < 寄存器数量) {
            上下文.寄存器值[i] = 非常量();
            i = i + 1;
        }
    }
    返回 上下文;
}


函数 释放常量传播上下文(常量传播上下文* 上下文) -> 空类型 {
    如果 (上下文) {
        如果 (上下文.寄存器值) {
            释放内存(上下文.寄存器值);
        }
        释放内存(上下文);
    }
}


函数 设置寄存器常量(常量传播上下文* 上下文, 整数 寄存器, 常量值 值) -> 空类型 {
    如果 (!上下文 || 寄存器 < 0 || 寄存器 >= 上下文.寄存器数量) {
        返回;
    }
    上下文.寄存器值[寄存器] = 值;
}


函数 获取寄存器常量(常量传播上下文* 上下文, 整数 寄存器) -> 常量值 {
    如果 (!上下文 || 寄存器 < 0 || 寄存器 >= 上下文.寄存器数量) {
        返回 非常量();
    }
    返回 上下文.寄存器值[寄存器];
}


函数 传播操作数(IR操作数* 操作数, 常量传播上下文* 上下文) -> 布尔 {
    如果 (!操作数 || !上下文) {
        返回 假;
    }
    
    
    如果 (操作数.种类 == 虚拟寄存器) {
        常量值 值 = 获取寄存器常量(上下文, 操作数.寄存器编号);
        如果 (值.是常量) {
            
            如果 (值.种类 == 整数常量) {
                操作数.种类 = 整数常量;
                操作数.整数值 = 值.整数值;
            } 否则 如果 (值.种类 == 小数常量) {
                操作数.种类 = 小数常量;
                操作数.小数值 = 值.小数值;
            }
            上下文.传播次数 = 上下文.传播次数 + 1;
            返回 真;
        }
    }
    返回 假;
}


函数 传播指令(IR指令* 指令, 常量传播上下文* 上下文) -> 空类型 {
    如果 (!指令 || !上下文) {
        返回;
    }
    
    
    传播操作数(&指令.源操作数1, 上下文);
    传播操作数(&指令.源操作数2, 上下文);
    
    
    如果 (指令.操作码 == 移动指令 && 指令.目标.种类 == 虚拟寄存器) {
        如果 (指令.源操作数1.种类 == 整数常量) {
            设置寄存器常量(上下文, 指令.目标.寄存器编号,
                创建整数常量值(指令.源操作数1.整数值, 指令.目标.类型信息));
        } 否则 如果 (指令.源操作数1.种类 == 小数常量) {
            设置寄存器常量(上下文, 指令.目标.寄存器编号,
                创建小数常量值(指令.源操作数1.小数值, 指令.目标.类型信息));
        }
    }
}


函数 传播基本块(基本块* 块, 常量传播上下文* 上下文) -> 空类型 {
    如果 (!块 || !上下文) {
        返回;
    }
    
    IR指令* 指令 = 块.首条指令;
    当 (指令) {
        传播指令(指令, 上下文);
        指令 = 指令.下一条;
    }
}


函数 传播函数(IR函数* 函数指针) -> 整数 {
    如果 (!函数指针) {
        返回 0;
    }
    
    常量传播上下文* 上下文 = 创建常量传播上下文(函数指针.下个寄存器编号 + 1);
    如果 (!上下文) {
        返回 0;
    }
    
    基本块* 块 = 函数指针.首个块;
    当 (块) {
        传播基本块(块, 上下文);
        块 = 块.下一个;
    }
    
    整数 次数 = 上下文.传播次数;
    释放常量传播上下文(上下文);
    返回 次数;
}









函数 执行常量优化(IR模块* 模块, 整数 最大迭代次数) -> 整数 {
    如果 (!模块) {
        返回 0;
    }
    
    整数 总次数 = 0;
    整数 迭代 = 0;
    
    当 (迭代 < 最大迭代次数) {
        整数 本轮次数 = 0;
        
        
        IR函数* 函数指针 = 模块.首个函数;
        当 (函数指针) {
            本轮次数 = 本轮次数 + 传播函数(函数指针);
            函数指针 = 函数指针.下一个;
        }
        
        
        本轮次数 = 本轮次数 + 折叠模块(模块);
        
        总次数 = 总次数 + 本轮次数;
        
        
        如果 (本轮次数 == 0) {
            中断;
        }
        
        迭代 = 迭代 + 1;
    }
    
    返回 总次数;
}
*)cn_rt_array_get_element(r10, r11, 8);
  return r12;

  logic_rhs_202:
  r6 = cn_var_寄存器;
  r7 = cn_var_上下文->寄存器数量;
  r8 = r6 >= r7;
  goto logic_merge_203;

  logic_merge_203:
  if (r8) goto if_then_200; else goto if_merge_201;

  logic_rhs_204:
  r4 = cn_var_寄存器;
  r5 = r4 < 0;
  goto logic_merge_205;

  logic_merge_205:
  if (r5) goto logic_merge_203; else goto logic_rhs_202;
  {
    static struct 常量值 _zero = {0};
    return _zero;
  }
}

_Bool 传播操作数(struct IR操作数* cn_var_操作数, struct 常量传播上下文* cn_var_上下文) {
  long long r0, r8, r13, r17, r18;
  struct IR操作数* r1;
  struct 常量传播上下文* r3;
  struct 常量传播上下文* r7;
  _Bool r2;
  _Bool r4;
  _Bool r6;
  _Bool r10;
  _Bool r12;
  _Bool r15;
  double r16;
  struct 常量值 r9;
  enum IR操作数种类 r5;
  enum IR操作数种类 r11;
  enum IR操作数种类 r14;

  entry:
  r1 = cn_var_操作数;
  r2 = !r1;
  if (r2) goto logic_merge_209; else goto logic_rhs_208;

  if_then_206:
  return 0;
  goto if_merge_207;

  if_merge_207:
  r5 = cn_var_操作数->种类;
  r6 = r5 == IR操作数种类_虚拟寄存器;
  if (r6) goto if_then_210; else goto if_merge_211;

  logic_rhs_208:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_209;

  logic_merge_209:
  if (r4) goto if_then_206; else goto if_merge_207;

  if_then_210:
  struct 常量值 cn_var_值_0;
  r7 = cn_var_上下文;
  r8 = cn_var_操作数->寄存器编号;
  r9 = 获取寄存器常量(r7, r8);
  cn_var_值_0 = r9;
  r10 = cn_var_值_0.是常量;
  if (r10) goto if_then_212; else goto if_merge_213;

  if_merge_211:
  return 0;

  if_then_212:
  r11 = cn_var_值_0.种类;
  r12 = r11 == IR操作数种类_整数常量;
  if (r12) goto if_then_214; else goto if_else_215;

  if_merge_213:
  goto if_merge_211;

  if_then_214:
  r13 = cn_var_值_0.整数值;
  goto if_merge_216;

  if_else_215:
  r14 = cn_var_值_0.种类;
  r15 = r14 == IR操作数种类_小数常量;
  if (r15) goto if_then_217; else goto if_merge_218;

  if_merge_216:
  r17 = cn_var_上下文->传播次数;
  r18 = r17 + 1;
  return 1;
  goto if_merge_213;

  if_then_217:
  r16 = cn_var_值_0.小数值;
  goto if_merge_218;

  if_merge_218:
  goto if_merge_216;
  return 0;
}

void 传播指令(struct IR指令* cn_var_指令, struct 常量传播上下文* cn_var_上下文) {
  long long r0, r15, r26, r28, r37;
  struct IR指令* r1;
  struct 常量传播上下文* r3;
  struct IR操作数* r6;
  struct 常量传播上下文* r8;
  struct IR操作数* r11;
  struct 常量传播上下文* r13;
  struct 常量传播上下文* r24;
  struct 类型节点* r30;
  struct 常量传播上下文* r35;
  struct 类型节点* r41;
  _Bool r2;
  _Bool r4;
  _Bool r9;
  _Bool r14;
  _Bool r17;
  _Bool r20;
  _Bool r23;
  _Bool r34;
  double r39;
  struct IR操作数 r5;
  struct IR操作数 r7;
  struct IR操作数 r10;
  struct IR操作数 r12;
  struct IR操作数 r18;
  struct IR操作数 r21;
  struct IR操作数 r25;
  struct IR操作数 r27;
  struct IR操作数 r29;
  struct 常量值 r31;
  struct IR操作数 r32;
  struct IR操作数 r36;
  struct IR操作数 r38;
  struct IR操作数 r40;
  struct 常量值 r42;
  enum IR操作码 r16;
  enum IR操作数种类 r19;
  enum IR操作数种类 r22;
  enum IR操作数种类 r33;

  entry:
  r1 = cn_var_指令;
  r2 = !r1;
  if (r2) goto logic_merge_222; else goto logic_rhs_221;

  if_then_219:
  return;
  goto if_merge_220;

  if_merge_220:
  r5 = cn_var_指令->源操作数1;
  r7 = cn_var_指令->源操作数1;
  r6 = &r7;
  r8 = cn_var_上下文;
  r9 = 传播操作数(r6, r8);
  r10 = cn_var_指令->源操作数2;
  r12 = cn_var_指令->源操作数2;
  r11 = &r12;
  r13 = cn_var_上下文;
  r14 = 传播操作数(r11, r13);
  r16 = cn_var_指令->操作码;
  r17 = r16 == IR指令种类_移动指令;
  if (r17) goto logic_rhs_225; else goto logic_merge_226;

  logic_rhs_221:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_222;

  logic_merge_222:
  if (r4) goto if_then_219; else goto if_merge_220;

  if_then_223:
  r21 = cn_var_指令->源操作数1;
  r22 = r21.种类;
  r23 = r22 == IR操作数种类_整数常量;
  if (r23) goto if_then_227; else goto if_else_228;

  if_merge_224:

  logic_rhs_225:
  r18 = cn_var_指令->目标;
  r19 = r18.种类;
  r20 = r19 == IR操作数种类_虚拟寄存器;
  goto logic_merge_226;

  logic_merge_226:
  if (r20) goto if_then_223; else goto if_merge_224;

  if_then_227:
  r24 = cn_var_上下文;
  r25 = cn_var_指令->目标;
  r26 = r25.寄存器编号;
  r27 = cn_var_指令->源操作数1;
  r28 = r27.整数值;
  r29 = cn_var_指令->目标;
  r30 = r29.类型信息;
  r31 = 创建整数常量值(r28, r30);
  设置寄存器常量(r24, r26, r31);
  goto if_merge_229;

  if_else_228:
  r32 = cn_var_指令->源操作数1;
  r33 = r32.种类;
  r34 = r33 == IR操作数种类_小数常量;
  if (r34) goto if_then_230; else goto if_merge_231;

  if_merge_229:
  goto if_merge_224;

  if_then_230:
  r35 = cn_var_上下文;
  r36 = cn_var_指令->目标;
  r37 = r36.寄存器编号;
  r38 = cn_var_指令->源操作数1;
  r39 = r38.小数值;
  r40 = cn_var_指令->目标;
  r41 = r40.类型信息;
  r42 = 创建小数常量值(r39, r41);
  设置寄存器常量(r35, r37, r42);
  goto if_merge_231;

  if_merge_231:
  goto if_merge_229;
  return;
}

void 传播基本块(struct 基本块* cn_var_块, struct 常量传播上下文* cn_var_上下文) {
  long long r0;
  struct 基本块* r1;
  struct 常量传播上下文* r3;
  struct IR指令* r5;
  struct IR指令* r6;
  struct IR指令* r7;
  struct 常量传播上下文* r8;
  struct IR指令* r9;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_块;
  r2 = !r1;
  if (r2) goto logic_merge_235; else goto logic_rhs_234;

  if_then_232:
  return;
  goto if_merge_233;

  if_merge_233:
  struct IR指令* cn_var_指令_0;
  r5 = cn_var_块->首条指令;
  cn_var_指令_0 = r5;
  goto while_cond_236;

  logic_rhs_234:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_235;

  logic_merge_235:
  if (r4) goto if_then_232; else goto if_merge_233;

  while_cond_236:
  r6 = cn_var_指令_0;
  if (r6) goto while_body_237; else goto while_exit_238;

  while_body_237:
  r7 = cn_var_指令_0;
  r8 = cn_var_上下文;
  传播指令(r7, r8);
  r9 = cn_var_指令_0->下一条;
  cn_var_指令_0 = r9;
  goto while_cond_236;

  while_exit_238:
  return;
}

long long 传播函数(struct IR函数* cn_var_函数指针) {
  long long r2, r3, r12, r14;
  struct IR函数* r0;
  struct 常量传播上下文* r4;
  struct 常量传播上下文* r5;
  struct 基本块* r7;
  struct 基本块* r8;
  struct 基本块* r9;
  struct 常量传播上下文* r10;
  struct 基本块* r11;
  struct 常量传播上下文* r13;
  _Bool r1;
  _Bool r6;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_239; else goto if_merge_240;

  if_then_239:
  return 0;
  goto if_merge_240;

  if_merge_240:
  struct 常量传播上下文* cn_var_上下文_0;
  r2 = cn_var_函数指针->下个寄存器编号;
  r3 = r2 + 1;
  r4 = 创建常量传播上下文(r3);
  cn_var_上下文_0 = r4;
  r5 = cn_var_上下文_0;
  r6 = !r5;
  if (r6) goto if_then_241; else goto if_merge_242;

  if_then_241:
  return 0;
  goto if_merge_242;

  if_merge_242:
  struct 基本块* cn_var_块_1;
  r7 = cn_var_函数指针->首个块;
  cn_var_块_1 = r7;
  goto while_cond_243;

  while_cond_243:
  r8 = cn_var_块_1;
  if (r8) goto while_body_244; else goto while_exit_245;

  while_body_244:
  r9 = cn_var_块_1;
  r10 = cn_var_上下文_0;
  传播基本块(r9, r10);
  r11 = cn_var_块_1->下一个;
  cn_var_块_1 = r11;
  goto while_cond_243;

  while_exit_245:
  long long cn_var_次数_2;
  r12 = cn_var_上下文_0->传播次数;
  cn_var_次数_2 = r12;
  r13 = cn_var_上下文_0;
  释放常量传播上下文(r13);
  r14 = cn_var_次数_2;
  return r14;
}

long long 执行常量优化(struct IR模块* cn_var_模块, long long cn_var_最大迭代次数) {
  long long r2, r3, r7, r9, r10, r12, r14, r15, r16, r17, r18, r19, r21, r22, r23;
  struct IR模块* r0;
  struct IR函数* r5;
  struct IR函数* r6;
  struct IR函数* r8;
  struct IR函数* r11;
  struct IR模块* r13;
  _Bool r1;
  _Bool r4;
  _Bool r20;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_246; else goto if_merge_247;

  if_then_246:
  return 0;
  goto if_merge_247;

  if_merge_247:
  long long cn_var_总次数_0;
  cn_var_总次数_0 = 0;
  long long cn_var_迭代_1;
  cn_var_迭代_1 = 0;
  goto while_cond_248;

  while_cond_248:
  r2 = cn_var_迭代_1;
  r3 = cn_var_最大迭代次数;
  r4 = r2 < r3;
  if (r4) goto while_body_249; else goto while_exit_250;

  while_body_249:
  long long cn_var_本轮次数_2;
  cn_var_本轮次数_2 = 0;
  struct IR函数* cn_var_函数指针_3;
  r5 = cn_var_模块->首个函数;
  cn_var_函数指针_3 = r5;
  goto while_cond_251;

  while_exit_250:
  r23 = cn_var_总次数_0;
  return r23;

  while_cond_251:
  r6 = cn_var_函数指针_3;
  if (r6) goto while_body_252; else goto while_exit_253;

  while_body_252:
  r7 = cn_var_本轮次数_2;
  r8 = cn_var_函数指针_3;
  r9 = 传播函数(r8);
  r10 = r7 + r9;
  cn_var_本轮次数_2 = r10;
  r11 = cn_var_函数指针_3->下一个;
  cn_var_函数指针_3 = r11;
  goto while_cond_251;

  while_exit_253:
  r12 = cn_var_本轮次数_2;
  r13 = cn_var_模块;
  r14 = 折叠模块(r13);
  r15 = r12 + r14;
  cn_var_本轮次数_2 = r15;
  r16 = cn_var_总次数_0;
  r17 = cn_var_本轮次数_2;
  r18 = r16 + r17;
  cn_var_总次数_0 = r18;
  r19 = cn_var_本轮次数_2;
  r20 = r19 == 0;
  if (r20) goto if_then_254; else goto if_merge_255;

  if_then_254:
  goto while_exit_250;
  goto if_merge_255;

  if_merge_255:
  r21 = cn_var_迭代_1;
  r22 = r21 + 1;
  cn_var_迭代_1 = r22;
  goto while_cond_248;
  return 0;
}

