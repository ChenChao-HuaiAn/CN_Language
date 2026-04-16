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
    诊断严重级别_诊断_错误 = 0,
    诊断严重级别_诊断_警告 = 1
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
extern long long cn_var_作用域指针大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号大小;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);
char* 获取作用域类型名称(enum 作用域类型);
char* 获取符号类型名称(enum 符号类型);
struct 符号* 查找类成员(struct 符号*, const char*);
struct 符号* 在作用域查找符号(struct 作用域*, const char*);
struct 符号* 查找全局符号(struct 符号表管理器*, const char*);
struct 符号* 查找符号(struct 符号表管理器*, const char*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, const char*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在循环体内(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
void 离开作用域(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, const char*, struct 符号*);
void 销毁符号表管理器(struct 符号表管理器*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁作用域(struct 作用域*);
struct 作用域* 创建作用域(enum 作用域类型, const char*, struct 作用域*);
struct 符号* 创建接口符号(const char*, struct 源位置);
struct 符号* 创建类符号(const char*, struct 源位置, _Bool);
struct 符号* 创建枚举成员符号(const char*, long long, _Bool, struct 源位置);
struct 符号* 创建枚举符号(const char*, struct 源位置);
struct 符号* 创建结构体符号(const char*, struct 源位置);
struct 符号* 创建参数符号(const char*, struct 类型节点*, struct 源位置);
struct 符号* 创建函数符号(const char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建变量符号(const char*, struct 类型节点*, struct 源位置, struct 符号标志);
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 复制字符串副本(const char*);
char* 字符串格式(const char*);
long long 字符串格式化(const char*, long long, const char*);
char* 读取行(void);
long long 获取绝对值(long long);
long long 求最大值(long long, long long);
long long 求最小值(long long, long long);
long long 获取参数个数(void);
char* 获取参数(long long);
char* 获取程序名称(void);
char* 查找选项(const char*);
long long 选项存在(const char*);
char* 获取位置参数(long long);
long long 获取位置参数个数(void);
void* 数组获取(void*, long long);
struct 诊断集合* 创建诊断集合(long long);
void 释放诊断集合(struct 诊断集合*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, const char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
_Bool 有错误(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
_Bool 应该继续(struct 诊断集合*);
char* 获取严重级别字符串(enum 诊断严重级别);
void 打印诊断信息(struct 诊断信息*);
void 打印所有诊断(struct 诊断集合*);
void 清空诊断集合(struct 诊断集合*);
struct 源位置 创建源位置(const char*, long long, long long);
struct 源位置 创建未知位置(void);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, const char*, const char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, const char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, const char*);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, const char*);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
extern struct 源位置 cn_var_空源位置;
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
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_符号大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_作用域指针大小;
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
char* 生成唯一变量名(const char*);
char* 生成块名称(const char*);
long long 分配虚拟寄存器(struct IR生成上下文*);
void 发射指令(struct IR生成上下文*, struct IR指令*);
void 切换基本块(struct IR生成上下文*, struct 基本块*);
char* 查找局部变量唯一名(struct IR生成上下文*, const char*);
void 添加局部变量映射(struct IR生成上下文*, const char*, const char*);
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
struct IR操作数 生成类型转换IR(struct IR生成上下文*, struct 表达式节点*);
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

char* 生成唯一变量名(const char* cn_var_原始名称) {
  long long r2, r3, r4;
  char* r1;
  char* r6;
  char* r0;
  char* r5;

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
  r6 = 复制字符串副本(r5);
  return r6;
}

char* 生成块名称(const char* cn_var_提示) {
  long long r2, r3, r4;
  char* r1;
  char* r6;
  char* r0;
  char* r5;

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
  r6 = 复制字符串副本(r5);
  return r6;
}

long long 分配虚拟寄存器(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r4, r5, r7, r8, r9, r10;
  struct IR生成上下文* r1;
  struct IR函数* r3;
  struct IR函数* r6;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2986; else goto logic_rhs_2985;

  if_then_2983:
  r5 = -1;
  return r5;
  goto if_merge_2984;

  if_merge_2984:
  long long cn_var_编号_0;
  r6 = cn_var_上下文->当前函数;
  r7 = r6->下个寄存器编号;
  cn_var_编号_0 = r7;
  r8 = cn_var_编号_0;
  r9 = r8 + 1;
  r10 = cn_var_编号_0;
  return r10;

  logic_rhs_2985:
  r3 = cn_var_上下文->当前函数;
  r4 = !r3;
  goto logic_merge_2986;

  logic_merge_2986:
  if (r4) goto if_then_2983; else goto if_merge_2984;
  return 0;
}

void 发射指令(struct IR生成上下文* cn_var_上下文, struct IR指令* cn_var_指令) {
  long long r0, r1, r3, r5, r7;
  struct IR生成上下文* r2;
  struct 基本块* r4;
  struct IR指令* r6;
  struct 基本块* r8;
  struct IR指令* r9;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_2992; else goto logic_rhs_2991;

  if_then_2987:
  return;
  goto if_merge_2988;

  if_merge_2988:
  r8 = cn_var_上下文->当前块;
  r9 = cn_var_指令;
  添加指令(r8, r9);

  logic_rhs_2989:
  r6 = cn_var_指令;
  r7 = !r6;
  goto logic_merge_2990;

  logic_merge_2990:
  if (r7) goto if_then_2987; else goto if_merge_2988;

  logic_rhs_2991:
  r4 = cn_var_上下文->当前块;
  r5 = !r4;
  goto logic_merge_2992;

  logic_merge_2992:
  if (r5) goto logic_merge_2990; else goto logic_rhs_2989;
  return;
}

void 切换基本块(struct IR生成上下文* cn_var_上下文, struct 基本块* cn_var_块) {
  struct IR生成上下文* r0;
  struct 基本块* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_2993; else goto if_merge_2994;

  if_then_2993:
  r1 = cn_var_块;
  goto if_merge_2994;

  if_merge_2994:
  return;
}

char* 查找局部变量唯一名(struct IR生成上下文* cn_var_上下文, const char* cn_var_原始名称) {
  long long r0, r2, r4, r9, r10;
  char* r3;
  char* r7;
  char* r8;
  char* r11;
  char* r12;
  struct IR生成上下文* r1;
  struct 局部变量映射* r5;
  struct 局部变量映射* r6;
  struct 局部变量映射* r13;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_2998; else goto logic_rhs_2997;

  if_then_2995:
  return 0;
  goto if_merge_2996;

  if_merge_2996:
  struct 局部变量映射* cn_var_映射_0;
  r5 = cn_var_上下文->局部变量映射表;
  cn_var_映射_0 = r5;
  goto while_cond_2999;

  logic_rhs_2997:
  r3 = cn_var_原始名称;
  r4 = !r3;
  goto logic_merge_2998;

  logic_merge_2998:
  if (r4) goto if_then_2995; else goto if_merge_2996;

  while_cond_2999:
  r6 = cn_var_映射_0;
  if (r6) goto while_body_3000; else goto while_exit_3001;

  while_body_3000:
  r7 = cn_var_映射_0->原始名称;
  r8 = cn_var_原始名称;
  r9 = 比较字符串(r7, r8);
  r10 = r9 == 0;
  if (r10) goto if_then_3002; else goto if_merge_3003;

  while_exit_3001:
  return 0;

  if_then_3002:
  r11 = cn_var_映射_0->唯一名称;
  r12 = 复制字符串副本(r11);
  return r12;
  goto if_merge_3003;

  if_merge_3003:
  r13 = cn_var_映射_0->下一个;
  cn_var_映射_0 = r13;
  goto while_cond_2999;
  return NULL;
}

void 添加局部变量映射(struct IR生成上下文* cn_var_上下文, const char* cn_var_原始名称, const char* cn_var_唯一名称) {
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
  struct 局部变量映射* r15;
  struct 局部变量映射* r16;

  entry:
  r2 = cn_var_上下文;
  r3 = !r2;
  if (r3) goto logic_merge_3009; else goto logic_rhs_3008;

  if_then_3004:
  return;
  goto if_merge_3005;

  if_merge_3005:
  struct 局部变量映射* cn_var_映射_0;
  r8 = sizeof(struct 局部变量映射);
  r9 = 分配内存(r8);
  cn_var_映射_0 = r9;
  r10 = cn_var_映射_0;
  if (r10) goto if_then_3010; else goto if_merge_3011;

  logic_rhs_3006:
  r6 = cn_var_唯一名称;
  r7 = !r6;
  goto logic_merge_3007;

  logic_merge_3007:
  if (r7) goto if_then_3004; else goto if_merge_3005;

  logic_rhs_3008:
  r4 = cn_var_原始名称;
  r5 = !r4;
  goto logic_merge_3009;

  logic_merge_3009:
  if (r5) goto logic_merge_3007; else goto logic_rhs_3006;

  if_then_3010:
  r11 = cn_var_原始名称;
  r12 = 复制字符串副本(r11);
  r13 = cn_var_唯一名称;
  r14 = 复制字符串副本(r13);
  r15 = cn_var_上下文->局部变量映射表;
  r16 = cn_var_映射_0;
  goto if_merge_3011;

  if_merge_3011:
  return;
}

void 释放局部变量映射表(struct IR生成上下文* cn_var_上下文) {
  long long r1;
  char* r5;
  char* r6;
  struct IR生成上下文* r0;
  struct 局部变量映射* r2;
  struct 局部变量映射* r3;
  struct 局部变量映射* r4;
  struct 局部变量映射* r7;
  struct 局部变量映射* r8;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3012; else goto if_merge_3013;

  if_then_3012:
  return;
  goto if_merge_3013;

  if_merge_3013:
  struct 局部变量映射* cn_var_映射_0;
  r2 = cn_var_上下文->局部变量映射表;
  cn_var_映射_0 = r2;
  goto while_cond_3014;

  while_cond_3014:
  r3 = cn_var_映射_0;
  if (r3) goto while_body_3015; else goto while_exit_3016;

  while_body_3015:
  struct 局部变量映射* cn_var_下个_1;
  r4 = cn_var_映射_0->下一个;
  cn_var_下个_1 = r4;
  r5 = cn_var_映射_0->原始名称;
  释放内存(r5);
  r6 = cn_var_映射_0->唯一名称;
  释放内存(r6);
  r7 = cn_var_映射_0;
  释放内存(r7);
  r8 = cn_var_下个_1;
  cn_var_映射_0 = r8;
  goto while_cond_3014;

  while_exit_3016:
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
  if (r2) goto if_then_3017; else goto if_merge_3018;

  if_then_3017:
  r3 = 创建IR模块();
  goto if_merge_3018;

  if_merge_3018:
  r4 = cn_var_上下文_0;
  return r4;
}

void 释放IR生成上下文(struct IR生成上下文* cn_var_上下文) {
  long long r1;
  struct IR生成上下文* r0;
  struct IR生成上下文* r2;
  struct IR模块* r3;
  struct IR模块* r4;
  struct IR生成上下文* r5;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3019; else goto if_merge_3020;

  if_then_3019:
  return;
  goto if_merge_3020;

  if_merge_3020:
  r2 = cn_var_上下文;
  释放局部变量映射表(r2);
  r3 = cn_var_上下文->模块;
  if (r3) goto if_then_3021; else goto if_merge_3022;

  if_then_3021:
  r4 = cn_var_上下文->模块;
  释放IR模块(r4);
  goto if_merge_3022;

  if_merge_3022:
  r5 = cn_var_上下文;
  释放内存(r5);
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
  r1 = r0 == 二元运算符_二元_加;
  if (r1) goto case_body_3024; else goto switch_check_3041;

  switch_check_3041:
  r2 = r0 == 二元运算符_二元_减;
  if (r2) goto case_body_3025; else goto switch_check_3042;

  switch_check_3042:
  r3 = r0 == 二元运算符_二元_乘;
  if (r3) goto case_body_3026; else goto switch_check_3043;

  switch_check_3043:
  r4 = r0 == 二元运算符_二元_除;
  if (r4) goto case_body_3027; else goto switch_check_3044;

  switch_check_3044:
  r5 = r0 == 二元运算符_二元_取模;
  if (r5) goto case_body_3028; else goto switch_check_3045;

  switch_check_3045:
  r6 = r0 == 二元运算符_二元_等于;
  if (r6) goto case_body_3029; else goto switch_check_3046;

  switch_check_3046:
  r7 = r0 == 二元运算符_二元_不等于;
  if (r7) goto case_body_3030; else goto switch_check_3047;

  switch_check_3047:
  r8 = r0 == 二元运算符_二元_小于;
  if (r8) goto case_body_3031; else goto switch_check_3048;

  switch_check_3048:
  r9 = r0 == 二元运算符_二元_大于;
  if (r9) goto case_body_3032; else goto switch_check_3049;

  switch_check_3049:
  r10 = r0 == 二元运算符_二元_小于等于;
  if (r10) goto case_body_3033; else goto switch_check_3050;

  switch_check_3050:
  r11 = r0 == 二元运算符_二元_大于等于;
  if (r11) goto case_body_3034; else goto switch_check_3051;

  switch_check_3051:
  r12 = r0 == 二元运算符_二元_位与;
  if (r12) goto case_body_3035; else goto switch_check_3052;

  switch_check_3052:
  r13 = r0 == 二元运算符_二元_位或;
  if (r13) goto case_body_3036; else goto switch_check_3053;

  switch_check_3053:
  r14 = r0 == 二元运算符_二元_位异或;
  if (r14) goto case_body_3037; else goto switch_check_3054;

  switch_check_3054:
  r15 = r0 == 二元运算符_二元_左移;
  if (r15) goto case_body_3038; else goto switch_check_3055;

  switch_check_3055:
  r16 = r0 == 二元运算符_二元_右移;
  if (r16) goto case_body_3039; else goto case_default_3040;

  case_body_3024:
  return IR指令种类_加法指令;
  goto switch_merge_3023;

  case_body_3025:
  return IR指令种类_减法指令;
  goto switch_merge_3023;

  case_body_3026:
  return IR指令种类_乘法指令;
  goto switch_merge_3023;

  case_body_3027:
  return IR指令种类_除法指令;
  goto switch_merge_3023;

  case_body_3028:
  return IR指令种类_取模指令;
  goto switch_merge_3023;

  case_body_3029:
  return IR指令种类_相等指令;
  goto switch_merge_3023;

  case_body_3030:
  return IR指令种类_不等指令;
  goto switch_merge_3023;

  case_body_3031:
  return IR指令种类_小于指令;
  goto switch_merge_3023;

  case_body_3032:
  return IR指令种类_大于指令;
  goto switch_merge_3023;

  case_body_3033:
  return IR指令种类_小于等于指令;
  goto switch_merge_3023;

  case_body_3034:
  return IR指令种类_大于等于指令;
  goto switch_merge_3023;

  case_body_3035:
  return IR指令种类_与指令;
  goto switch_merge_3023;

  case_body_3036:
  return IR指令种类_或指令;
  goto switch_merge_3023;

  case_body_3037:
  return IR指令种类_异或指令;
  goto switch_merge_3023;

  case_body_3038:
  return IR指令种类_左移指令;
  goto switch_merge_3023;

  case_body_3039:
  return IR指令种类_右移指令;
  goto switch_merge_3023;

  case_default_3040:
  return IR指令种类_加法指令;
  goto switch_merge_3023;

  switch_merge_3023:
  return 0;
}

struct IR操作数 生成表达式IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r13;
  struct 表达式节点* r0;
  struct 类型节点* r14;
  struct IR生成上下文* r16;
  struct 表达式节点* r17;
  struct IR生成上下文* r19;
  struct 表达式节点* r20;
  struct IR生成上下文* r22;
  struct 表达式节点* r23;
  struct IR生成上下文* r25;
  struct 表达式节点* r26;
  struct IR生成上下文* r28;
  struct 表达式节点* r29;
  struct IR生成上下文* r31;
  struct 表达式节点* r32;
  struct IR生成上下文* r34;
  struct 表达式节点* r35;
  struct IR生成上下文* r37;
  struct 表达式节点* r38;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  struct IR操作数 r2;
  struct IR操作数 r15;
  struct IR操作数 r18;
  struct IR操作数 r21;
  struct IR操作数 r24;
  struct IR操作数 r27;
  struct IR操作数 r30;
  struct IR操作数 r33;
  struct IR操作数 r36;
  struct IR操作数 r39;
  struct IR操作数 r40;
  enum 节点类型 r3;

  entry:
  r0 = cn_var_表达式;
  r1 = !r0;
  if (r1) goto if_then_3056; else goto if_merge_3057;

  if_then_3056:
  r2 = 空操作数();
  return r2;
  goto if_merge_3057;

  if_merge_3057:
  r3 = cn_var_表达式->类型;
  r4 = r3 == 节点类型_字面量表达式;
  if (r4) goto case_body_3059; else goto switch_check_3069;

  switch_check_3069:
  r5 = r3 == 节点类型_标识符表达式;
  if (r5) goto case_body_3060; else goto switch_check_3070;

  switch_check_3070:
  r6 = r3 == 节点类型_二元表达式;
  if (r6) goto case_body_3061; else goto switch_check_3071;

  switch_check_3071:
  r7 = r3 == 节点类型_一元表达式;
  if (r7) goto case_body_3062; else goto switch_check_3072;

  switch_check_3072:
  r8 = r3 == 节点类型_函数调用表达式;
  if (r8) goto case_body_3063; else goto switch_check_3073;

  switch_check_3073:
  r9 = r3 == 节点类型_成员访问表达式;
  if (r9) goto case_body_3064; else goto switch_check_3074;

  switch_check_3074:
  r10 = r3 == 节点类型_数组访问表达式;
  if (r10) goto case_body_3065; else goto switch_check_3075;

  switch_check_3075:
  r11 = r3 == 节点类型_赋值表达式;
  if (r11) goto case_body_3066; else goto switch_check_3076;

  switch_check_3076:
  r12 = r3 == 节点类型_类型转换表达式;
  if (r12) goto case_body_3067; else goto case_default_3068;

  case_body_3059:
  r13 = cn_var_表达式->整数值;
  r14 = cn_var_表达式->类型信息;
  r15 = 整数常量操作数(r13, r14);
  return r15;
  goto switch_merge_3058;

  case_body_3060:
  r16 = cn_var_上下文;
  r17 = cn_var_表达式;
  r18 = 生成标识符IR(r16, r17);
  return r18;
  goto switch_merge_3058;

  case_body_3061:
  r19 = cn_var_上下文;
  r20 = cn_var_表达式;
  r21 = 生成二元运算IR(r19, r20);
  return r21;
  goto switch_merge_3058;

  case_body_3062:
  r22 = cn_var_上下文;
  r23 = cn_var_表达式;
  r24 = 生成一元运算IR(r22, r23);
  return r24;
  goto switch_merge_3058;

  case_body_3063:
  r25 = cn_var_上下文;
  r26 = cn_var_表达式;
  r27 = 生成函数调用IR(r25, r26);
  return r27;
  goto switch_merge_3058;

  case_body_3064:
  r28 = cn_var_上下文;
  r29 = cn_var_表达式;
  r30 = 生成成员访问IR(r28, r29);
  return r30;
  goto switch_merge_3058;

  case_body_3065:
  r31 = cn_var_上下文;
  r32 = cn_var_表达式;
  r33 = 生成数组访问IR(r31, r32);
  return r33;
  goto switch_merge_3058;

  case_body_3066:
  r34 = cn_var_上下文;
  r35 = cn_var_表达式;
  r36 = 生成赋值IR(r34, r35);
  return r36;
  goto switch_merge_3058;

  case_body_3067:
  r37 = cn_var_上下文;
  r38 = cn_var_表达式;
  r39 = 生成类型转换IR(r37, r38);
  return r39;
  goto switch_merge_3058;

  case_default_3068:
  r40 = 空操作数();
  return r40;
  goto switch_merge_3058;

  switch_merge_3058:
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成标识符IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r11, r12;
  char* r7;
  char* r8;
  char* r9;
  char* r15;
  char* r24;
  char* r26;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct IR生成上下文* r10;
  struct 类型节点* r13;
  struct 类型节点* r16;
  struct IR指令* r21;
  struct IR生成上下文* r22;
  struct IR指令* r23;
  struct 类型节点* r27;
  struct IR操作数 r5;
  struct IR操作数 r14;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r19;
  struct IR操作数 r20;
  struct IR操作数 r25;
  struct IR操作数 r28;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3080; else goto logic_rhs_3079;

  if_then_3077:
  r5 = 空操作数();
  return r5;
  goto if_merge_3078;

  if_merge_3078:
  char* cn_var_唯一名_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->标识符名称;
  r8 = 查找局部变量唯一名(r6, r7);
  cn_var_唯一名_0 = r8;
  r9 = cn_var_唯一名_0;
  if (r9) goto if_then_3081; else goto if_merge_3082;

  logic_rhs_3079:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3080;

  logic_merge_3080:
  if (r4) goto if_then_3077; else goto if_merge_3078;

  if_then_3081:
  long long cn_var_寄存器_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_寄存器_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_寄存器_1;
  r13 = cn_var_表达式->类型信息;
  r14 = 寄存器操作数(r12, r13);
  cn_var_目标_2 = r14;
  struct IR操作数 cn_var_源_3;
  r15 = cn_var_唯一名_0;
  r16 = cn_var_表达式->类型信息;
  r17 = 全局符号操作数(r15, r16);
  cn_var_源_3 = r17;
  struct IR指令* cn_var_指令_4;
  r18 = cn_var_目标_2;
  r19 = cn_var_源_3;
  r20 = 空操作数();
  r21 = 创建IR指令(IR指令种类_加载指令, r18, r19, r20);
  cn_var_指令_4 = r21;
  r22 = cn_var_上下文;
  r23 = cn_var_指令_4;
  发射指令(r22, r23);
  r24 = cn_var_唯一名_0;
  释放内存(r24);
  r25 = cn_var_目标_2;
  return r25;
  goto if_merge_3082;

  if_merge_3082:
  r26 = cn_var_表达式->标识符名称;
  r27 = cn_var_表达式->类型信息;
  r28 = 全局符号操作数(r26, r27);
  return r28;
}

struct IR操作数 生成二元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r13, r14, r17;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 表达式节点* r10;
  struct IR生成上下文* r12;
  struct 类型节点* r15;
  struct IR指令* r23;
  struct IR生成上下文* r24;
  struct IR指令* r25;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r11;
  struct IR操作数 r16;
  enum IR操作码 r19;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r26;
  enum IR操作码 r18;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3086; else goto logic_rhs_3085;

  if_then_3083:
  r5 = 空操作数();
  return r5;
  goto if_merge_3084;

  if_merge_3084:
  struct IR操作数 cn_var_左_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->左操作数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_左_0 = r8;
  struct IR操作数 cn_var_右_1;
  r9 = cn_var_上下文;
  r10 = cn_var_表达式->右操作数;
  r11 = 生成表达式IR(r9, r10);
  cn_var_右_1 = r11;
  long long cn_var_结果_2;
  r12 = cn_var_上下文;
  r13 = 分配虚拟寄存器(r12);
  cn_var_结果_2 = r13;
  struct IR操作数 cn_var_目标_3;
  r14 = cn_var_结果_2;
  r15 = cn_var_表达式->类型信息;
  r16 = 寄存器操作数(r14, r15);
  cn_var_目标_3 = r16;
  enum IR操作码 cn_var_操作码_4;
  r17 = cn_var_表达式->运算符;
  r18 = 二元运算符转IR(r17);
  cn_var_操作码_4 = r18;
  struct IR指令* cn_var_指令_5;
  r19 = cn_var_操作码_4;
  r20 = cn_var_目标_3;
  r21 = cn_var_左_0;
  r22 = cn_var_右_1;
  r23 = 创建IR指令(r19, r20, r21, r22);
  cn_var_指令_5 = r23;
  r24 = cn_var_上下文;
  r25 = cn_var_指令_5;
  发射指令(r24, r25);
  r26 = cn_var_目标_3;
  return r26;

  logic_rhs_3085:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3086;

  logic_merge_3086:
  if (r4) goto if_then_3083; else goto if_merge_3084;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成一元运算IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11, r14;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r22;
  struct IR生成上下文* r23;
  struct IR指令* r24;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  enum IR操作码 r18;
  struct IR操作数 r19;
  struct IR操作数 r20;
  struct IR操作数 r21;
  struct IR操作数 r25;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3090; else goto logic_rhs_3089;

  if_then_3087:
  r5 = 空操作数();
  return r5;
  goto if_merge_3088;

  if_merge_3088:
  struct IR操作数 cn_var_操作数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->操作数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_操作数_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->类型信息;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  enum IR操作码 cn_var_操作码_3;
  cn_var_操作码_3 = IR指令种类_负号指令;
  r14 = cn_var_表达式->运算符;
  r15 = r14 == 一元运算符_一元_取负;
  if (r15) goto case_body_3092; else goto switch_check_3096;

  logic_rhs_3089:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3090;

  logic_merge_3090:
  if (r4) goto if_then_3087; else goto if_merge_3088;

  switch_check_3096:
  r16 = r14 == 一元运算符_一元_位取反;
  if (r16) goto case_body_3093; else goto switch_check_3097;

  switch_check_3097:
  r17 = r14 == 一元运算符_一元_逻辑非;
  if (r17) goto case_body_3094; else goto case_default_3095;

  case_body_3092:
  cn_var_操作码_3 = IR指令种类_负号指令;
  goto switch_merge_3091;
  goto switch_merge_3091;

  case_body_3093:
  cn_var_操作码_3 = IR指令种类_取反指令;
  goto switch_merge_3091;
  goto switch_merge_3091;

  case_body_3094:
  cn_var_操作码_3 = IR指令种类_逻辑非指令;
  goto switch_merge_3091;
  goto switch_merge_3091;

  case_default_3095:
  goto switch_merge_3091;
  goto switch_merge_3091;

  switch_merge_3091:
  struct IR指令* cn_var_指令_4;
  r18 = cn_var_操作码_3;
  r19 = cn_var_目标_2;
  r20 = cn_var_操作数_0;
  r21 = 空操作数();
  r22 = 创建IR指令(r18, r19, r20, r21);
  cn_var_指令_4 = r22;
  r23 = cn_var_上下文;
  r24 = cn_var_指令_4;
  发射指令(r23, r24);
  r25 = cn_var_目标_2;
  return r25;
}

struct IR操作数 生成函数调用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11, r21, r22, r23, r24, r25, r26, r27, r28, r30, r37, r38, r39, r40;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r17;
  struct 表达式列表* r18;
  struct 表达式列表* r19;
  struct 表达式节点* r20;
  void* r29;
  struct 表达式列表* r31;
  struct 表达式节点* r32;
  struct IR生成上下文* r33;
  struct 表达式节点* r34;
  struct IR操作数* r36;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  struct IR操作数 r14;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r35;
  struct IR操作数 r43;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3101; else goto logic_rhs_3100;

  if_then_3098:
  r5 = 空操作数();
  return r5;
  goto if_merge_3099;

  if_merge_3099:
  struct IR操作数 cn_var_被调函数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->被调函数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_被调函数_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->类型信息;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  struct IR指令* cn_var_指令_3;
  r14 = cn_var_目标_2;
  r15 = cn_var_被调函数_0;
  r16 = 空操作数();
  r17 = 创建IR指令(IR指令种类_调用指令, r14, r15, r16);
  cn_var_指令_3 = r17;
  r18 = cn_var_表达式->参数列表;
  if (r18) goto if_then_3102; else goto if_merge_3103;

  logic_rhs_3100:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3101;

  logic_merge_3101:
  if (r4) goto if_then_3098; else goto if_merge_3099;

  if_then_3102:
  long long cn_var_数量_4;
  cn_var_数量_4 = 0;
  struct 表达式节点* cn_var_参数_5;
  r19 = cn_var_表达式->参数列表;
  cn_var_参数_5 = r19;
  goto while_cond_3104;

  if_merge_3103:
  r41 = cn_var_上下文;
  r42 = cn_var_指令_3;
  发射指令(r41, r42);
  r43 = cn_var_目标_2;
  return r43;

  while_cond_3104:
  r20 = cn_var_参数_5;
  if (r20) goto while_body_3105; else goto while_exit_3106;

  while_body_3105:
  r21 = cn_var_数量_4;
  r22 = r21 + 1;
  cn_var_数量_4 = r22;
  r23 = cn_var_参数_5.下一个;
  cn_var_参数_5 = r23;
  goto while_cond_3104;

  while_exit_3106:
  r24 = cn_var_数量_4;
  r25 = r24 > 0;
  if (r25) goto if_then_3107; else goto if_merge_3108;

  if_then_3107:
  r26 = cn_var_数量_4;
  r27 = sizeof(struct IR操作数);
  r28 = r26 * r27;
  r29 = 分配内存(r28);
  r30 = cn_var_数量_4;
  r31 = cn_var_表达式->参数列表;
  cn_var_参数_5 = r31;
  long long cn_var_索引_6;
  cn_var_索引_6 = 0;
  goto while_cond_3109;

  if_merge_3108:
  goto if_merge_3103;

  while_cond_3109:
  r32 = cn_var_参数_5;
  if (r32) goto while_body_3110; else goto while_exit_3111;

  while_body_3110:
  r33 = cn_var_上下文;
  r34 = cn_var_参数_5;
  r35 = 生成表达式IR(r33, r34);
  r36 = cn_var_指令_3->额外参数;
  r37 = cn_var_索引_6;
    { struct IR操作数 _tmp_r10 = r35; cn_rt_array_set_element(r36, r37, &_tmp_r10, 8); }
  r38 = cn_var_索引_6;
  r39 = r38 + 1;
  cn_var_索引_6 = r39;
  r40 = cn_var_参数_5.下一个;
  cn_var_参数_5 = r40;
  goto while_cond_3109;

  while_exit_3111:
  goto if_merge_3108;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成成员访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11;
  char* r14;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r19;
  struct IR生成上下文* r20;
  struct IR指令* r21;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r22;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3115; else goto logic_rhs_3114;

  if_then_3112:
  r5 = 空操作数();
  return r5;
  goto if_merge_3113;

  if_merge_3113:
  struct IR操作数 cn_var_对象_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->对象;
  r8 = 生成表达式IR(r6, r7);
  cn_var_对象_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->类型信息;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  struct IR操作数 cn_var_成员名_3;
  r14 = cn_var_表达式->成员名;
  r15 = 全局符号操作数(r14, 0);
  cn_var_成员名_3 = r15;
  struct IR指令* cn_var_指令_4;
  r16 = cn_var_目标_2;
  r17 = cn_var_对象_0;
  r18 = cn_var_成员名_3;
  r19 = 创建IR指令(IR指令种类_成员访问指令, r16, r17, r18);
  cn_var_指令_4 = r19;
  r20 = cn_var_上下文;
  r21 = cn_var_指令_4;
  发射指令(r20, r21);
  r22 = cn_var_目标_2;
  return r22;

  logic_rhs_3114:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3115;

  logic_merge_3115:
  if (r4) goto if_then_3112; else goto if_merge_3113;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成数组访问IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r13, r14;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 表达式节点* r10;
  struct IR生成上下文* r12;
  struct 类型节点* r15;
  struct IR指令* r20;
  struct IR生成上下文* r21;
  struct IR指令* r22;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r11;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r19;
  struct IR操作数 r23;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3119; else goto logic_rhs_3118;

  if_then_3116:
  r5 = 空操作数();
  return r5;
  goto if_merge_3117;

  if_merge_3117:
  struct IR操作数 cn_var_数组_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->数组;
  r8 = 生成表达式IR(r6, r7);
  cn_var_数组_0 = r8;
  struct IR操作数 cn_var_索引_1;
  r9 = cn_var_上下文;
  r10 = cn_var_表达式->索引;
  r11 = 生成表达式IR(r9, r10);
  cn_var_索引_1 = r11;
  long long cn_var_结果_2;
  r12 = cn_var_上下文;
  r13 = 分配虚拟寄存器(r12);
  cn_var_结果_2 = r13;
  struct IR操作数 cn_var_目标_3;
  r14 = cn_var_结果_2;
  r15 = cn_var_表达式->类型信息;
  r16 = 寄存器操作数(r14, r15);
  cn_var_目标_3 = r16;
  struct IR指令* cn_var_指令_4;
  r17 = cn_var_目标_3;
  r18 = cn_var_数组_0;
  r19 = cn_var_索引_1;
  r20 = 创建IR指令(IR指令种类_加载指令, r17, r18, r19);
  cn_var_指令_4 = r20;
  r21 = cn_var_上下文;
  r22 = cn_var_指令_4;
  发射指令(r21, r22);
  r23 = cn_var_目标_3;
  return r23;

  logic_rhs_3118:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3119;

  logic_merge_3119:
  if (r4) goto if_then_3116; else goto if_merge_3117;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成赋值IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r12;
  char* r15;
  char* r16;
  char* r17;
  char* r18;
  char* r28;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct 表达式节点* r9;
  struct IR生成上下文* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r19;
  struct 类型节点* r20;
  struct IR指令* r25;
  struct IR生成上下文* r26;
  struct IR指令* r27;
  struct IR生成上下文* r29;
  struct 表达式节点* r30;
  struct IR指令* r35;
  struct IR生成上下文* r36;
  struct IR指令* r37;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct 标识符表达式 r11;
  struct IR操作数 r21;
  struct IR操作数 r22;
  struct IR操作数 r23;
  struct IR操作数 r24;
  struct IR操作数 r31;
  struct IR操作数 r32;
  struct IR操作数 r33;
  struct IR操作数 r34;
  struct IR操作数 r38;
  enum 节点类型 r10;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3123; else goto logic_rhs_3122;

  if_then_3120:
  r5 = 空操作数();
  return r5;
  goto if_merge_3121;

  if_merge_3121:
  struct IR操作数 cn_var_值_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->值;
  r8 = 生成表达式IR(r6, r7);
  cn_var_值_0 = r8;
  r9 = cn_var_表达式->左侧表达式;
  r10 = r9->类型;
  r11 = cn_var_标识符表达式;
  r12 = r10 == r11;
  if (r12) goto if_then_3124; else goto if_else_3125;

  logic_rhs_3122:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3123;

  logic_merge_3123:
  if (r4) goto if_then_3120; else goto if_merge_3121;

  if_then_3124:
  char* cn_var_唯一名_1;
  r13 = cn_var_上下文;
  r14 = cn_var_表达式->左侧表达式;
  r15 = r14->标识符名称;
  r16 = 查找局部变量唯一名(r13, r15);
  cn_var_唯一名_1 = r16;
  r17 = cn_var_唯一名_1;
  if (r17) goto if_then_3127; else goto if_merge_3128;

  if_else_3125:
  struct IR操作数 cn_var_目标_4;
  r29 = cn_var_上下文;
  r30 = cn_var_表达式->左侧表达式;
  r31 = 生成表达式IR(r29, r30);
  cn_var_目标_4 = r31;
  struct IR指令* cn_var_指令_5;
  r32 = cn_var_目标_4;
  r33 = cn_var_值_0;
  r34 = 空操作数();
  r35 = 创建IR指令(IR指令种类_存储指令, r32, r33, r34);
  cn_var_指令_5 = r35;
  r36 = cn_var_上下文;
  r37 = cn_var_指令_5;
  发射指令(r36, r37);
  goto if_merge_3126;

  if_merge_3126:
  r38 = cn_var_值_0;
  return r38;

  if_then_3127:
  struct IR操作数 cn_var_目标_2;
  r18 = cn_var_唯一名_1;
  r19 = cn_var_表达式->左侧表达式;
  r20 = r19->类型信息;
  r21 = 全局符号操作数(r18, r20);
  cn_var_目标_2 = r21;
  struct IR指令* cn_var_指令_3;
  r22 = cn_var_目标_2;
  r23 = cn_var_值_0;
  r24 = 空操作数();
  r25 = 创建IR指令(IR指令种类_存储指令, r22, r23, r24);
  cn_var_指令_3 = r25;
  r26 = cn_var_上下文;
  r27 = cn_var_指令_3;
  发射指令(r26, r27);
  r28 = cn_var_唯一名_1;
  释放内存(r28);
  goto if_merge_3128;

  if_merge_3128:
  goto if_merge_3126;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成类型转换IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r17;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  struct IR操作数 r14;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r20;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3132; else goto logic_rhs_3131;

  if_then_3129:
  r5 = 空操作数();
  return r5;
  goto if_merge_3130;

  if_merge_3130:
  struct IR操作数 cn_var_操作数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->转换操作数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_操作数_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->目标类型;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  struct IR指令* cn_var_指令_3;
  r14 = cn_var_目标_2;
  r15 = cn_var_操作数_0;
  r16 = 空操作数();
  r17 = 创建IR指令(IR操作码_类型转换指令, r14, r15, r16);
  cn_var_指令_3 = r17;
  r18 = cn_var_上下文;
  r19 = cn_var_指令_3;
  发射指令(r18, r19);
  r20 = cn_var_目标_2;
  return r20;

  logic_rhs_3131:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3132;

  logic_merge_3132:
  if (r4) goto if_then_3129; else goto if_merge_3130;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成取地址IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r17;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  struct IR操作数 r14;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r20;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3136; else goto logic_rhs_3135;

  if_then_3133:
  r5 = 空操作数();
  return r5;
  goto if_merge_3134;

  if_merge_3134:
  struct IR操作数 cn_var_操作数_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->操作数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_操作数_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->类型信息;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  struct IR指令* cn_var_指令_3;
  r14 = cn_var_目标_2;
  r15 = cn_var_操作数_0;
  r16 = 空操作数();
  r17 = 创建IR指令(IR指令种类_取地址指令, r14, r15, r16);
  cn_var_指令_3 = r17;
  r18 = cn_var_上下文;
  r19 = cn_var_指令_3;
  发射指令(r18, r19);
  r20 = cn_var_目标_2;
  return r20;

  logic_rhs_3135:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3136;

  logic_merge_3136:
  if (r4) goto if_then_3133; else goto if_merge_3134;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

struct IR操作数 生成解引用IR(struct IR生成上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r0, r2, r4, r10, r11;
  struct IR生成上下文* r1;
  struct 表达式节点* r3;
  struct IR生成上下文* r6;
  struct 表达式节点* r7;
  struct IR生成上下文* r9;
  struct 类型节点* r12;
  struct IR指令* r17;
  struct IR生成上下文* r18;
  struct IR指令* r19;
  struct IR操作数 r5;
  struct IR操作数 r8;
  struct IR操作数 r13;
  struct IR操作数 r14;
  struct IR操作数 r15;
  struct IR操作数 r16;
  struct IR操作数 r20;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3140; else goto logic_rhs_3139;

  if_then_3137:
  r5 = 空操作数();
  return r5;
  goto if_merge_3138;

  if_merge_3138:
  struct IR操作数 cn_var_指针_0;
  r6 = cn_var_上下文;
  r7 = cn_var_表达式->操作数;
  r8 = 生成表达式IR(r6, r7);
  cn_var_指针_0 = r8;
  long long cn_var_结果_1;
  r9 = cn_var_上下文;
  r10 = 分配虚拟寄存器(r9);
  cn_var_结果_1 = r10;
  struct IR操作数 cn_var_目标_2;
  r11 = cn_var_结果_1;
  r12 = cn_var_表达式->类型信息;
  r13 = 寄存器操作数(r11, r12);
  cn_var_目标_2 = r13;
  struct IR指令* cn_var_指令_3;
  r14 = cn_var_目标_2;
  r15 = cn_var_指针_0;
  r16 = 空操作数();
  r17 = 创建IR指令(IR指令种类_解引用指令, r14, r15, r16);
  cn_var_指令_3 = r17;
  r18 = cn_var_上下文;
  r19 = cn_var_指令_3;
  发射指令(r18, r19);
  r20 = cn_var_目标_2;
  return r20;

  logic_rhs_3139:
  r3 = cn_var_表达式;
  r4 = !r3;
  goto logic_merge_3140;

  logic_merge_3140:
  if (r4) goto if_then_3137; else goto if_merge_3138;
  {
    static struct IR操作数 _zero = {0};
    return _zero;
  }
}

void 生成语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r16;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r15;
  struct IR生成上下文* r18;
  struct 语句节点* r19;
  struct IR生成上下文* r20;
  struct 语句节点* r21;
  struct IR生成上下文* r22;
  struct 语句节点* r23;
  struct IR生成上下文* r24;
  struct 语句节点* r25;
  struct IR生成上下文* r26;
  struct 语句节点* r27;
  struct IR生成上下文* r28;
  struct IR生成上下文* r29;
  struct IR生成上下文* r30;
  struct 语句节点* r31;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  struct IR操作数 r17;
  enum 节点类型 r5;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3144; else goto logic_rhs_3143;

  if_then_3141:
  return;
  goto if_merge_3142;

  if_merge_3142:
  r5 = cn_var_语句->类型;
  r6 = r5 == 节点类型_表达式语句;
  if (r6) goto case_body_3146; else goto switch_check_3156;

  logic_rhs_3143:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3144;

  logic_merge_3144:
  if (r4) goto if_then_3141; else goto if_merge_3142;

  switch_check_3156:
  r7 = r5 == 节点类型_变量声明;
  if (r7) goto case_body_3147; else goto switch_check_3157;

  switch_check_3157:
  r8 = r5 == 节点类型_如果语句;
  if (r8) goto case_body_3148; else goto switch_check_3158;

  switch_check_3158:
  r9 = r5 == 节点类型_当语句;
  if (r9) goto case_body_3149; else goto switch_check_3159;

  switch_check_3159:
  r10 = r5 == 节点类型_循环语句;
  if (r10) goto case_body_3150; else goto switch_check_3160;

  switch_check_3160:
  r11 = r5 == 节点类型_返回语句;
  if (r11) goto case_body_3151; else goto switch_check_3161;

  switch_check_3161:
  r12 = r5 == 节点类型_中断语句;
  if (r12) goto case_body_3152; else goto switch_check_3162;

  switch_check_3162:
  r13 = r5 == 节点类型_继续语句;
  if (r13) goto case_body_3153; else goto switch_check_3163;

  switch_check_3163:
  r14 = r5 == 节点类型_块语句;
  if (r14) goto case_body_3154; else goto case_default_3155;

  case_body_3146:
  r15 = cn_var_上下文;
  r16 = cn_var_语句->表达式;
  r17 = 生成表达式IR(r15, r16);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3147:
  r18 = cn_var_上下文;
  r19 = cn_var_语句;
  生成变量声明IR(r18, r19);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3148:
  r20 = cn_var_上下文;
  r21 = cn_var_语句;
  生成如果语句IR(r20, r21);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3149:
  r22 = cn_var_上下文;
  r23 = cn_var_语句;
  生成当循环IR(r22, r23);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3150:
  r24 = cn_var_上下文;
  r25 = cn_var_语句;
  生成循环IR(r24, r25);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3151:
  r26 = cn_var_上下文;
  r27 = cn_var_语句;
  生成返回语句IR(r26, r27);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3152:
  r28 = cn_var_上下文;
  生成中断语句IR(r28);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3153:
  r29 = cn_var_上下文;
  生成继续语句IR(r29);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_body_3154:
  r30 = cn_var_上下文;
  r31 = cn_var_语句;
  生成块语句IR(r30, r31);
  goto switch_merge_3145;
  goto switch_merge_3145;

  case_default_3155:
  goto switch_merge_3145;
  goto switch_merge_3145;

  switch_merge_3145:
  return;
}

void 生成变量声明IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r5, r8, r11, r12, r13, r15, r23, r25, r28;
  char* r6;
  char* r9;
  char* r27;
  char* r37;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r7;
  struct IR生成上下文* r10;
  struct IR指令* r20;
  struct IR生成上下文* r21;
  struct IR指令* r22;
  struct IR生成上下文* r24;
  struct IR指令* r30;
  struct IR指令* r34;
  struct IR生成上下文* r35;
  struct IR指令* r36;
  struct IR操作数 r14;
  struct IR操作数 r16;
  struct IR操作数 r17;
  struct IR操作数 r18;
  struct IR操作数 r19;
  struct IR操作数 r26;
  struct IR操作数 r29;
  struct IR操作数 r31;
  struct IR操作数 r32;
  struct IR操作数 r33;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3167; else goto logic_rhs_3166;

  if_then_3164:
  return;
  goto if_merge_3165;

  if_merge_3165:
  char* cn_var_唯一名_0;
  r5 = cn_var_语句->变量名称;
  r6 = 生成唯一变量名(r5);
  cn_var_唯一名_0 = r6;
  r7 = cn_var_上下文;
  r8 = cn_var_语句->变量名称;
  r9 = cn_var_唯一名_0;
  添加局部变量映射(r7, r8, r9);
  long long cn_var_寄存器_1;
  r10 = cn_var_上下文;
  r11 = 分配虚拟寄存器(r10);
  cn_var_寄存器_1 = r11;
  struct IR操作数 cn_var_目标_2;
  r12 = cn_var_寄存器_1;
  r13 = cn_var_语句->类型信息;
  r14 = 寄存器操作数(r12, r13);
  cn_var_目标_2 = r14;
  struct IR操作数 cn_var_大小_3;
  r15 = cn_var_语句->类型大小;
  r16 = 整数常量操作数(r15, 0);
  cn_var_大小_3 = r16;
  struct IR指令* cn_var_指令_4;
  r17 = cn_var_目标_2;
  r18 = cn_var_大小_3;
  r19 = 空操作数();
  r20 = 创建IR指令(IR指令种类_分配指令, r17, r18, r19);
  cn_var_指令_4 = r20;
  r21 = cn_var_上下文;
  r22 = cn_var_指令_4;
  发射指令(r21, r22);
  r23 = cn_var_语句->初始值表达式;
  if (r23) goto if_then_3168; else goto if_merge_3169;

  logic_rhs_3166:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3167;

  logic_merge_3167:
  if (r4) goto if_then_3164; else goto if_merge_3165;

  if_then_3168:
  struct IR操作数 cn_var_值_5;
  r24 = cn_var_上下文;
  r25 = cn_var_语句->初始值表达式;
  r26 = 生成表达式IR(r24, r25);
  cn_var_值_5 = r26;
  struct IR操作数 cn_var_目标变量_6;
  r27 = cn_var_唯一名_0;
  r28 = cn_var_语句->类型信息;
  r29 = 全局符号操作数(r27, r28);
  cn_var_目标变量_6 = r29;
  struct IR指令* cn_var_存储指令_7;
  r30 = cn_var_存储指令_7;
  r31 = cn_var_目标变量_6;
  r32 = cn_var_值_5;
  r33 = 空操作数();
  r34 = 创建IR指令(r30, r31, r32, r33);
  cn_var_存储指令_7 = r34;
  r35 = cn_var_上下文;
  r36 = cn_var_存储指令_7;
  发射指令(r35, r36);
  goto if_merge_3169;

  if_merge_3169:
  r37 = cn_var_唯一名_0;
  释放内存(r37);
  return;
}

void 生成如果语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r6, r33, r45, r47;
  char* r8;
  char* r9;
  char* r10;
  char* r11;
  char* r13;
  char* r15;
  char* r17;
  char* r18;
  char* r19;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct IR生成上下文* r5;
  struct 基本块* r12;
  struct 基本块* r14;
  struct 基本块* r16;
  struct 基本块* r21;
  struct 基本块* r23;
  struct IR指令* r25;
  struct IR生成上下文* r26;
  struct IR指令* r27;
  struct IR函数* r28;
  struct 基本块* r29;
  struct IR生成上下文* r30;
  struct 基本块* r31;
  struct IR生成上下文* r32;
  struct 基本块* r34;
  struct IR指令* r38;
  struct IR生成上下文* r39;
  struct IR指令* r40;
  struct IR函数* r41;
  struct 基本块* r42;
  struct IR生成上下文* r43;
  struct 基本块* r44;
  struct IR生成上下文* r46;
  struct 基本块* r48;
  struct IR指令* r52;
  struct IR生成上下文* r53;
  struct IR指令* r54;
  struct IR函数* r55;
  struct 基本块* r56;
  struct IR生成上下文* r57;
  struct 基本块* r58;
  struct IR操作数 r7;
  struct IR操作数 r20;
  struct IR操作数 r22;
  struct IR操作数 r24;
  struct IR操作数 r35;
  struct IR操作数 r36;
  struct IR操作数 r37;
  struct IR操作数 r49;
  struct IR操作数 r50;
  struct IR操作数 r51;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3173; else goto logic_rhs_3172;

  if_then_3170:
  return;
  goto if_merge_3171;

  if_merge_3171:
  struct IR操作数 cn_var_条件_0;
  r5 = cn_var_上下文;
  r6 = cn_var_语句->条件表达式;
  r7 = 生成表达式IR(r5, r6);
  cn_var_条件_0 = r7;
  char* cn_var_真块名_1;
  r8 = 生成块名称("if_true");
  cn_var_真块名_1 = r8;
  char* cn_var_假块名_2;
  r9 = 生成块名称("if_false");
  cn_var_假块名_2 = r9;
  char* cn_var_结束块名_3;
  r10 = 生成块名称("if_end");
  cn_var_结束块名_3 = r10;
  struct 基本块* cn_var_真块_4;
  r11 = cn_var_真块名_1;
  r12 = 创建基本块(r11);
  cn_var_真块_4 = r12;
  struct 基本块* cn_var_假块_5;
  r13 = cn_var_假块名_2;
  r14 = 创建基本块(r13);
  cn_var_假块_5 = r14;
  struct 基本块* cn_var_结束块_6;
  r15 = cn_var_结束块名_3;
  r16 = 创建基本块(r15);
  cn_var_结束块_6 = r16;
  r17 = cn_var_真块名_1;
  释放内存(r17);
  r18 = cn_var_假块名_2;
  释放内存(r18);
  r19 = cn_var_结束块名_3;
  释放内存(r19);
  struct IR指令* cn_var_跳转_7;
  r20 = cn_var_条件_0;
  r21 = cn_var_真块_4;
  r22 = 标签操作数(r21);
  r23 = cn_var_假块_5;
  r24 = 标签操作数(r23);
  r25 = 创建IR指令(IR指令种类_条件跳转指令, r20, r22, r24);
  cn_var_跳转_7 = r25;
  r26 = cn_var_上下文;
  r27 = cn_var_跳转_7;
  发射指令(r26, r27);
  r28 = cn_var_上下文->当前函数;
  r29 = cn_var_真块_4;
  添加基本块(r28, r29);
  r30 = cn_var_上下文;
  r31 = cn_var_真块_4;
  切换基本块(r30, r31);
  r32 = cn_var_上下文;
  r33 = cn_var_语句->真分支语句;
  生成语句IR(r32, r33);
  struct IR指令* cn_var_跳转到结束_8;
  r34 = cn_var_结束块_6;
  r35 = 标签操作数(r34);
  r36 = 空操作数();
  r37 = 空操作数();
  r38 = 创建IR指令(IR指令种类_跳转指令, r35, r36, r37);
  cn_var_跳转到结束_8 = r38;
  r39 = cn_var_上下文;
  r40 = cn_var_跳转到结束_8;
  发射指令(r39, r40);
  r41 = cn_var_上下文->当前函数;
  r42 = cn_var_假块_5;
  添加基本块(r41, r42);
  r43 = cn_var_上下文;
  r44 = cn_var_假块_5;
  切换基本块(r43, r44);
  r45 = cn_var_语句->假分支语句;
  if (r45) goto if_then_3174; else goto if_merge_3175;

  logic_rhs_3172:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3173;

  logic_merge_3173:
  if (r4) goto if_then_3170; else goto if_merge_3171;

  if_then_3174:
  r46 = cn_var_上下文;
  r47 = cn_var_语句->假分支语句;
  生成语句IR(r46, r47);
  goto if_merge_3175;

  if_merge_3175:
  struct IR指令* cn_var_跳转到结束2_9;
  r48 = cn_var_结束块_6;
  r49 = 标签操作数(r48);
  r50 = 空操作数();
  r51 = 空操作数();
  r52 = 创建IR指令(IR指令种类_跳转指令, r49, r50, r51);
  cn_var_跳转到结束2_9 = r52;
  r53 = cn_var_上下文;
  r54 = cn_var_跳转到结束2_9;
  发射指令(r53, r54);
  r55 = cn_var_上下文->当前函数;
  r56 = cn_var_结束块_6;
  添加基本块(r55, r56);
  r57 = cn_var_上下文;
  r58 = cn_var_结束块_6;
  切换基本块(r57, r58);
  return;
}

void 生成当循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r33, r48;
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
  struct 基本块* r17;
  struct 基本块* r18;
  struct 基本块* r19;
  struct 基本块* r20;
  struct 基本块* r21;
  struct IR指令* r25;
  struct IR生成上下文* r26;
  struct IR指令* r27;
  struct IR函数* r28;
  struct 基本块* r29;
  struct IR生成上下文* r30;
  struct 基本块* r31;
  struct IR生成上下文* r32;
  struct 基本块* r36;
  struct 基本块* r38;
  struct IR指令* r40;
  struct IR生成上下文* r41;
  struct IR指令* r42;
  struct IR函数* r43;
  struct 基本块* r44;
  struct IR生成上下文* r45;
  struct 基本块* r46;
  struct IR生成上下文* r47;
  struct 基本块* r49;
  struct IR指令* r53;
  struct IR生成上下文* r54;
  struct IR指令* r55;
  struct IR函数* r56;
  struct 基本块* r57;
  struct IR生成上下文* r58;
  struct 基本块* r59;
  struct 基本块* r60;
  struct 基本块* r61;
  struct IR操作数 r22;
  struct IR操作数 r23;
  struct IR操作数 r24;
  struct IR操作数 r34;
  struct IR操作数 r35;
  struct IR操作数 r37;
  struct IR操作数 r39;
  struct IR操作数 r50;
  struct IR操作数 r51;
  struct IR操作数 r52;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3179; else goto logic_rhs_3178;

  if_then_3176:
  return;
  goto if_merge_3177;

  if_merge_3177:
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
  r17 = cn_var_上下文->循环退出块;
  cn_var_原退出_6 = r17;
  struct 基本块* cn_var_原继续_7;
  r18 = cn_var_上下文->循环继续块;
  cn_var_原继续_7 = r18;
  r19 = cn_var_退出块_5;
  r20 = cn_var_条件块_3;
  struct IR指令* cn_var_跳转到条件_8;
  r21 = cn_var_条件块_3;
  r22 = 标签操作数(r21);
  r23 = 空操作数();
  r24 = 空操作数();
  r25 = 创建IR指令(IR指令种类_跳转指令, r22, r23, r24);
  cn_var_跳转到条件_8 = r25;
  r26 = cn_var_上下文;
  r27 = cn_var_跳转到条件_8;
  发射指令(r26, r27);
  r28 = cn_var_上下文->当前函数;
  r29 = cn_var_条件块_3;
  添加基本块(r28, r29);
  r30 = cn_var_上下文;
  r31 = cn_var_条件块_3;
  切换基本块(r30, r31);
  struct IR操作数 cn_var_条件_9;
  r32 = cn_var_上下文;
  r33 = cn_var_语句->条件表达式;
  r34 = 生成表达式IR(r32, r33);
  cn_var_条件_9 = r34;
  struct IR指令* cn_var_条件跳转_10;
  r35 = cn_var_条件_9;
  r36 = cn_var_循环体块_4;
  r37 = 标签操作数(r36);
  r38 = cn_var_退出块_5;
  r39 = 标签操作数(r38);
  r40 = 创建IR指令(IR指令种类_条件跳转指令, r35, r37, r39);
  cn_var_条件跳转_10 = r40;
  r41 = cn_var_上下文;
  r42 = cn_var_条件跳转_10;
  发射指令(r41, r42);
  r43 = cn_var_上下文->当前函数;
  r44 = cn_var_循环体块_4;
  添加基本块(r43, r44);
  r45 = cn_var_上下文;
  r46 = cn_var_循环体块_4;
  切换基本块(r45, r46);
  r47 = cn_var_上下文;
  r48 = cn_var_语句->循环体语句;
  生成语句IR(r47, r48);
  struct IR指令* cn_var_跳回条件_11;
  r49 = cn_var_条件块_3;
  r50 = 标签操作数(r49);
  r51 = 空操作数();
  r52 = 空操作数();
  r53 = 创建IR指令(IR指令种类_跳转指令, r50, r51, r52);
  cn_var_跳回条件_11 = r53;
  r54 = cn_var_上下文;
  r55 = cn_var_跳回条件_11;
  发射指令(r54, r55);
  r56 = cn_var_上下文->当前函数;
  r57 = cn_var_退出块_5;
  添加基本块(r56, r57);
  r58 = cn_var_上下文;
  r59 = cn_var_退出块_5;
  切换基本块(r58, r59);
  r60 = cn_var_原退出_6;
  r61 = cn_var_原继续_7;

  logic_rhs_3178:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3179;

  logic_merge_3179:
  if (r4) goto if_then_3176; else goto if_merge_3177;
  return;
}

void 生成循环IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r29, r31, r43, r45, r67, r79, r81;
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
  struct 基本块* r25;
  struct 基本块* r26;
  struct 基本块* r27;
  struct 基本块* r28;
  struct IR生成上下文* r30;
  struct 基本块* r32;
  struct IR指令* r36;
  struct IR生成上下文* r37;
  struct IR指令* r38;
  struct IR函数* r39;
  struct 基本块* r40;
  struct IR生成上下文* r41;
  struct 基本块* r42;
  struct IR生成上下文* r44;
  struct 基本块* r48;
  struct 基本块* r50;
  struct IR指令* r52;
  struct IR生成上下文* r53;
  struct IR指令* r54;
  struct 基本块* r55;
  struct IR指令* r59;
  struct IR生成上下文* r60;
  struct IR指令* r61;
  struct IR函数* r62;
  struct 基本块* r63;
  struct IR生成上下文* r64;
  struct 基本块* r65;
  struct IR生成上下文* r66;
  struct 基本块* r68;
  struct IR指令* r72;
  struct IR生成上下文* r73;
  struct IR指令* r74;
  struct IR函数* r75;
  struct 基本块* r76;
  struct IR生成上下文* r77;
  struct 基本块* r78;
  struct IR生成上下文* r80;
  struct 基本块* r83;
  struct IR指令* r87;
  struct IR生成上下文* r88;
  struct IR指令* r89;
  struct IR函数* r90;
  struct 基本块* r91;
  struct IR生成上下文* r92;
  struct 基本块* r93;
  struct 基本块* r94;
  struct 基本块* r95;
  struct IR操作数 r33;
  struct IR操作数 r34;
  struct IR操作数 r35;
  struct IR操作数 r46;
  struct IR操作数 r47;
  struct IR操作数 r49;
  struct IR操作数 r51;
  struct IR操作数 r56;
  struct IR操作数 r57;
  struct IR操作数 r58;
  struct IR操作数 r69;
  struct IR操作数 r70;
  struct IR操作数 r71;
  struct IR操作数 r82;
  struct IR操作数 r84;
  struct IR操作数 r85;
  struct IR操作数 r86;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3183; else goto logic_rhs_3182;

  if_then_3180:
  return;
  goto if_merge_3181;

  if_merge_3181:
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
  r25 = cn_var_上下文->循环退出块;
  cn_var_原退出_10 = r25;
  struct 基本块* cn_var_原继续_11;
  r26 = cn_var_上下文->循环继续块;
  cn_var_原继续_11 = r26;
  r27 = cn_var_退出块_9;
  r28 = cn_var_迭代块_8;
  r29 = cn_var_语句->初始化语句;
  if (r29) goto if_then_3184; else goto if_merge_3185;

  logic_rhs_3182:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3183;

  logic_merge_3183:
  if (r4) goto if_then_3180; else goto if_merge_3181;

  if_then_3184:
  r30 = cn_var_上下文;
  r31 = cn_var_语句->初始化语句;
  生成语句IR(r30, r31);
  goto if_merge_3185;

  if_merge_3185:
  struct IR指令* cn_var_跳转到条件_12;
  r32 = cn_var_条件块_6;
  r33 = 标签操作数(r32);
  r34 = 空操作数();
  r35 = 空操作数();
  r36 = 创建IR指令(IR指令种类_跳转指令, r33, r34, r35);
  cn_var_跳转到条件_12 = r36;
  r37 = cn_var_上下文;
  r38 = cn_var_跳转到条件_12;
  发射指令(r37, r38);
  r39 = cn_var_上下文->当前函数;
  r40 = cn_var_条件块_6;
  添加基本块(r39, r40);
  r41 = cn_var_上下文;
  r42 = cn_var_条件块_6;
  切换基本块(r41, r42);
  r43 = cn_var_语句->条件表达式;
  if (r43) goto if_then_3186; else goto if_else_3187;

  if_then_3186:
  struct IR操作数 cn_var_条件_13;
  r44 = cn_var_上下文;
  r45 = cn_var_语句->条件表达式;
  r46 = 生成表达式IR(r44, r45);
  cn_var_条件_13 = r46;
  struct IR指令* cn_var_条件跳转_14;
  r47 = cn_var_条件_13;
  r48 = cn_var_循环体块_7;
  r49 = 标签操作数(r48);
  r50 = cn_var_退出块_9;
  r51 = 标签操作数(r50);
  r52 = 创建IR指令(IR指令种类_条件跳转指令, r47, r49, r51);
  cn_var_条件跳转_14 = r52;
  r53 = cn_var_上下文;
  r54 = cn_var_条件跳转_14;
  发射指令(r53, r54);
  goto if_merge_3188;

  if_else_3187:
  struct IR指令* cn_var_无条件跳转_15;
  r55 = cn_var_循环体块_7;
  r56 = 标签操作数(r55);
  r57 = 空操作数();
  r58 = 空操作数();
  r59 = 创建IR指令(IR指令种类_跳转指令, r56, r57, r58);
  cn_var_无条件跳转_15 = r59;
  r60 = cn_var_上下文;
  r61 = cn_var_无条件跳转_15;
  发射指令(r60, r61);
  goto if_merge_3188;

  if_merge_3188:
  r62 = cn_var_上下文->当前函数;
  r63 = cn_var_循环体块_7;
  添加基本块(r62, r63);
  r64 = cn_var_上下文;
  r65 = cn_var_循环体块_7;
  切换基本块(r64, r65);
  r66 = cn_var_上下文;
  r67 = cn_var_语句->循环体语句;
  生成语句IR(r66, r67);
  struct IR指令* cn_var_跳转到迭代_16;
  r68 = cn_var_迭代块_8;
  r69 = 标签操作数(r68);
  r70 = 空操作数();
  r71 = 空操作数();
  r72 = 创建IR指令(IR指令种类_跳转指令, r69, r70, r71);
  cn_var_跳转到迭代_16 = r72;
  r73 = cn_var_上下文;
  r74 = cn_var_跳转到迭代_16;
  发射指令(r73, r74);
  r75 = cn_var_上下文->当前函数;
  r76 = cn_var_迭代块_8;
  添加基本块(r75, r76);
  r77 = cn_var_上下文;
  r78 = cn_var_迭代块_8;
  切换基本块(r77, r78);
  r79 = cn_var_语句->迭代表达式;
  if (r79) goto if_then_3189; else goto if_merge_3190;

  if_then_3189:
  r80 = cn_var_上下文;
  r81 = cn_var_语句->迭代表达式;
  r82 = 生成表达式IR(r80, r81);
  goto if_merge_3190;

  if_merge_3190:
  struct IR指令* cn_var_跳回条件_17;
  r83 = cn_var_条件块_6;
  r84 = 标签操作数(r83);
  r85 = 空操作数();
  r86 = 空操作数();
  r87 = 创建IR指令(IR指令种类_跳转指令, r84, r85, r86);
  cn_var_跳回条件_17 = r87;
  r88 = cn_var_上下文;
  r89 = cn_var_跳回条件_17;
  发射指令(r88, r89);
  r90 = cn_var_上下文->当前函数;
  r91 = cn_var_退出块_9;
  添加基本块(r90, r91);
  r92 = cn_var_上下文;
  r93 = cn_var_退出块_9;
  切换基本块(r92, r93);
  r94 = cn_var_原退出_10;
  r95 = cn_var_原继续_11;
  return;
}

void 生成返回语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r3, r5;
  struct IR生成上下文* r0;
  struct IR生成上下文* r4;
  struct IR指令* r10;
  struct IR生成上下文* r11;
  struct IR指令* r12;
  struct IR操作数 r2;
  struct IR操作数 r6;
  struct IR操作数 r7;
  struct IR操作数 r8;
  struct IR操作数 r9;

  entry:
  r0 = cn_var_上下文;
  r1 = !r0;
  if (r1) goto if_then_3191; else goto if_merge_3192;

  if_then_3191:
  return;
  goto if_merge_3192;

  if_merge_3192:
  struct IR操作数 cn_var_值_0;
  r2 = 空操作数();
  cn_var_值_0 = r2;
  r3 = cn_var_语句->返回表达式;
  if (r3) goto if_then_3193; else goto if_merge_3194;

  if_then_3193:
  r4 = cn_var_上下文;
  r5 = cn_var_语句->返回表达式;
  r6 = 生成表达式IR(r4, r5);
  cn_var_值_0 = r6;
  goto if_merge_3194;

  if_merge_3194:
  struct IR指令* cn_var_指令_1;
  r7 = cn_var_值_0;
  r8 = 空操作数();
  r9 = 空操作数();
  r10 = 创建IR指令(IR指令种类_返回指令, r7, r8, r9);
  cn_var_指令_1 = r10;
  r11 = cn_var_上下文;
  r12 = cn_var_指令_1;
  发射指令(r11, r12);
  return;
}

void 生成中断语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r4;
  struct IR生成上下文* r1;
  struct 基本块* r3;
  struct 基本块* r5;
  struct IR指令* r9;
  struct IR生成上下文* r10;
  struct IR指令* r11;
  struct IR操作数 r6;
  struct IR操作数 r7;
  struct IR操作数 r8;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3198; else goto logic_rhs_3197;

  if_then_3195:
  return;
  goto if_merge_3196;

  if_merge_3196:
  struct IR指令* cn_var_指令_0;
  r5 = cn_var_上下文->循环退出块;
  r6 = 标签操作数(r5);
  r7 = 空操作数();
  r8 = 空操作数();
  r9 = 创建IR指令(IR指令种类_跳转指令, r6, r7, r8);
  cn_var_指令_0 = r9;
  r10 = cn_var_上下文;
  r11 = cn_var_指令_0;
  发射指令(r10, r11);

  logic_rhs_3197:
  r3 = cn_var_上下文->循环退出块;
  r4 = !r3;
  goto logic_merge_3198;

  logic_merge_3198:
  if (r4) goto if_then_3195; else goto if_merge_3196;
  return;
}

void 生成继续语句IR(struct IR生成上下文* cn_var_上下文) {
  long long r0, r2, r4;
  struct IR生成上下文* r1;
  struct 基本块* r3;
  struct 基本块* r5;
  struct IR指令* r9;
  struct IR生成上下文* r10;
  struct IR指令* r11;
  struct IR操作数 r6;
  struct IR操作数 r7;
  struct IR操作数 r8;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3202; else goto logic_rhs_3201;

  if_then_3199:
  return;
  goto if_merge_3200;

  if_merge_3200:
  struct IR指令* cn_var_指令_0;
  r5 = cn_var_上下文->循环继续块;
  r6 = 标签操作数(r5);
  r7 = 空操作数();
  r8 = 空操作数();
  r9 = 创建IR指令(IR指令种类_跳转指令, r6, r7, r8);
  cn_var_指令_0 = r9;
  r10 = cn_var_上下文;
  r11 = cn_var_指令_0;
  发射指令(r10, r11);

  logic_rhs_3201:
  r3 = cn_var_上下文->循环继续块;
  r4 = !r3;
  goto logic_merge_3202;

  logic_merge_3202:
  if (r4) goto if_then_3199; else goto if_merge_3200;
  return;
}

void 生成块语句IR(struct IR生成上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r0, r2, r4, r5, r9;
  struct IR生成上下文* r1;
  struct 语句节点* r3;
  struct 语句节点* r6;
  struct IR生成上下文* r7;
  struct 语句节点* r8;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3206; else goto logic_rhs_3205;

  if_then_3203:
  return;
  goto if_merge_3204;

  if_merge_3204:
  struct 语句节点* cn_var_子语句_0;
  r5 = cn_var_语句->块语句列表;
  cn_var_子语句_0 = r5;
  goto while_cond_3207;

  logic_rhs_3205:
  r3 = cn_var_语句;
  r4 = !r3;
  goto logic_merge_3206;

  logic_merge_3206:
  if (r4) goto if_then_3203; else goto if_merge_3204;

  while_cond_3207:
  r6 = cn_var_子语句_0;
  if (r6) goto while_body_3208; else goto while_exit_3209;

  while_body_3208:
  r7 = cn_var_上下文;
  r8 = cn_var_子语句_0;
  生成语句IR(r7, r8);
  r9 = cn_var_子语句_0->下一个;
  cn_var_子语句_0 = r9;
  goto while_cond_3207;

  while_exit_3209:
  return;
}

void 生成函数IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r5, r6, r9, r19, r29;
  char* r11;
  char* r17;
  char* r18;
  char* r20;
  char* r21;
  char* r23;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct IR函数* r7;
  struct IR函数* r8;
  struct 参数节点* r10;
  struct 类型节点* r12;
  struct IR函数* r14;
  struct IR生成上下文* r16;
  struct 基本块* r22;
  struct IR函数* r24;
  struct 基本块* r25;
  struct IR生成上下文* r26;
  struct 基本块* r27;
  struct IR生成上下文* r28;
  struct IR模块* r30;
  struct IR函数* r31;
  struct IR生成上下文* r32;
  struct IR操作数 r13;
  struct IR操作数 r15;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3213; else goto logic_rhs_3212;

  if_then_3210:
  return;
  goto if_merge_3211;

  if_merge_3211:
  重置局部变量计数器();
  struct IR函数* cn_var_当前IR函数_0;
  r5 = cn_var_声明->函数名称;
  r6 = cn_var_声明->返回类型;
  r7 = 创建IR函数(r5, r6);
  cn_var_当前IR函数_0 = r7;
  r8 = cn_var_当前IR函数_0;
  struct 参数节点* cn_var_参数_1;
  r9 = cn_var_声明->参数列表;
  cn_var_参数_1 = r9;
  goto while_cond_3214;

  logic_rhs_3212:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3213;

  logic_merge_3213:
  if (r4) goto if_then_3210; else goto if_merge_3211;

  while_cond_3214:
  r10 = cn_var_参数_1;
  if (r10) goto while_body_3215; else goto while_exit_3216;

  while_body_3215:
  struct IR操作数 cn_var_参数操作数_2;
  r11 = cn_var_参数_1.名称;
  r12 = cn_var_参数_1.类型;
  r13 = 全局符号操作数(r11, r12);
  cn_var_参数操作数_2 = r13;
  r14 = cn_var_当前IR函数_0;
  r15 = cn_var_参数操作数_2;
  添加函数参数(r14, r15);
  r16 = cn_var_上下文;
  r17 = cn_var_参数_1.名称;
  r18 = cn_var_参数_1.名称;
  添加局部变量映射(r16, r17, r18);
  r19 = cn_var_参数_1.下一个;
  cn_var_参数_1 = r19;
  goto while_cond_3214;

  while_exit_3216:
  char* cn_var_入口块名_3;
  r20 = 生成块名称("entry");
  cn_var_入口块名_3 = r20;
  struct 基本块* cn_var_入口块_4;
  r21 = cn_var_入口块名_3;
  r22 = 创建基本块(r21);
  cn_var_入口块_4 = r22;
  r23 = cn_var_入口块名_3;
  释放内存(r23);
  r24 = cn_var_当前IR函数_0;
  r25 = cn_var_入口块_4;
  添加基本块(r24, r25);
  r26 = cn_var_上下文;
  r27 = cn_var_入口块_4;
  切换基本块(r26, r27);
  r28 = cn_var_上下文;
  r29 = cn_var_声明->函数体;
  生成语句IR(r28, r29);
  r30 = cn_var_上下文->模块;
  r31 = cn_var_当前IR函数_0;
  添加函数(r30, r31);
  r32 = cn_var_上下文;
  释放局部变量映射表(r32);
  return;
}

void 生成程序IR(struct IR生成上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r0, r2, r4;
  struct IR生成上下文* r1;
  struct 程序节点* r3;
  struct 声明节点列表* r5;
  struct 声明节点* r6;
  struct IR生成上下文* r10;
  struct 声明节点* r11;
  struct IR生成上下文* r12;
  struct 声明节点* r13;
  struct 声明节点* r14;
  _Bool r8;
  _Bool r9;
  enum 节点类型 r7;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3220; else goto logic_rhs_3219;

  if_then_3217:
  return;
  goto if_merge_3218;

  if_merge_3218:
  struct 声明节点* cn_var_声明_0;
  r5 = cn_var_程序->声明列表;
  cn_var_声明_0 = r5;
  goto while_cond_3221;

  logic_rhs_3219:
  r3 = cn_var_程序;
  r4 = !r3;
  goto logic_merge_3220;

  logic_merge_3220:
  if (r4) goto if_then_3217; else goto if_merge_3218;

  while_cond_3221:
  r6 = cn_var_声明_0;
  if (r6) goto while_body_3222; else goto while_exit_3223;

  while_body_3222:
  r7 = cn_var_声明_0->类型;
  r8 = r7 == 节点类型_函数声明;
  if (r8) goto case_body_3225; else goto switch_check_3228;

  while_exit_3223:

  switch_check_3228:
  r9 = r7 == 节点类型_变量声明;
  if (r9) goto case_body_3226; else goto case_default_3227;

  case_body_3225:
  r10 = cn_var_上下文;
  r11 = cn_var_声明_0;
  生成函数IR(r10, r11);
  goto switch_merge_3224;
  goto switch_merge_3224;

  case_body_3226:
  r12 = cn_var_上下文;
  r13 = cn_var_声明_0;
  生成全局变量IR(r12, r13);
  goto switch_merge_3224;
  goto switch_merge_3224;

  case_default_3227:
  goto switch_merge_3224;
  goto switch_merge_3224;

  switch_merge_3224:
  r14 = cn_var_声明_0->下一个;
  cn_var_声明_0 = r14;
  goto while_cond_3221;
  return;
}

void 生成全局变量IR(struct IR生成上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4, r6, r8, r10, r11, r13;
  struct IR生成上下文* r1;
  struct 声明节点* r3;
  struct IR生成上下文* r7;
  struct 全局变量* r14;
  struct IR模块* r15;
  struct 全局变量* r16;
  struct IR操作数 r5;
  struct IR操作数 r9;
  struct IR操作数 r12;

  entry:
  r1 = cn_var_上下文;
  r2 = !r1;
  if (r2) goto logic_merge_3232; else goto logic_rhs_3231;

  if_then_3229:
  return;
  goto if_merge_3230;

  if_merge_3230:
  struct IR操作数 cn_var_初始值_0;
  r5 = 空操作数();
  cn_var_初始值_0 = r5;
  r6 = cn_var_声明->初始值表达式;
  if (r6) goto if_then_3233; else goto if_merge_3234;

  logic_rhs_3231:
  r3 = cn_var_声明;
  r4 = !r3;
  goto logic_merge_3232;

  logic_merge_3232:
  if (r4) goto if_then_3229; else goto if_merge_3230;

  if_then_3233:
  r7 = cn_var_上下文;
  r8 = cn_var_声明->初始值表达式;
  r9 = 生成表达式IR(r7, r8);
  cn_var_初始值_0 = r9;
  goto if_merge_3234;

  if_merge_3234:
  struct 全局变量* cn_var_全局变量信息_1;
  r10 = cn_var_声明->变量名称;
  r11 = cn_var_声明->类型信息;
  r12 = cn_var_初始值_0;
  r13 = cn_var_声明->是常量;
  r14 = 创建全局变量(r10, r11, r12, r13);
  cn_var_全局变量信息_1 = r14;
  r15 = cn_var_上下文->模块;
  r16 = cn_var_全局变量信息_1;
  添加全局变量(r15, r16);
  return;
}

