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
struct 声明语句;
struct 声明语句 {
    struct AST节点 节点基类;
    struct 声明节点* 声明;
};
struct 表达式语句;
struct 如果语句;
struct 当语句;
struct 循环语句;
struct 返回语句;
struct 中断语句;
struct 继续语句;
struct 选择语句;
struct 尝试语句;
struct 抛出语句;
struct 语句节点;
struct 语句节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
    struct 表达式语句* 作为表达式语句;
    struct 块语句* 作为块语句;
    struct 如果语句* 作为如果语句;
    struct 当语句* 作为当语句;
    struct 循环语句* 作为循环语句;
    struct 返回语句* 作为返回语句;
    struct 中断语句* 作为中断语句;
    struct 继续语句* 作为继续语句;
    struct 选择语句* 作为选择语句;
    struct 尝试语句* 作为尝试语句;
    struct 抛出语句* 作为抛出语句;
    struct 声明语句* 作为声明语句;
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
struct 死代码消除上下文;
struct 使用信息;

// CN Language Global Struct Definitions
struct 死代码消除上下文 {
    long long 消除次数;
    _Bool 已修改;
    struct IR模块* 模块;
};

struct 使用信息 {
    _Bool 已定义;
    _Bool 已使用;
    long long 定义次数;
    long long 使用次数;
};

// Global Variables

// Forward Declarations
struct 死代码消除上下文* 创建死代码消除上下文(struct IR模块*);
void 释放死代码消除上下文(struct 死代码消除上下文*);
void 标记基本块已访问(struct 基本块*);
void 标记可达基本块(struct IR函数*);
void 消除不可达基本块(struct IR函数*, struct 死代码消除上下文*);
struct 使用信息* 创建使用信息数组(long long);
void 释放使用信息数组(struct 使用信息*);
void 记录定义(struct 使用信息*, long long);
void 记录使用(struct 使用信息*, long long);
void 分析操作数使用(struct IR操作数, struct 使用信息*, _Bool);
void 分析指令使用(struct IR指令*, struct 使用信息*);
void 分析基本块使用(struct 基本块*, struct 使用信息*);
void 分析函数使用(struct IR函数*, struct 使用信息*);
_Bool 可以删除指令(struct IR指令*, struct 使用信息*);
void 消除基本块死代码(struct 基本块*, struct 使用信息*, struct 死代码消除上下文*);
void 消除函数死代码(struct IR函数*, struct 死代码消除上下文*);
long long 消除模块死代码(struct IR模块*);
long long 迭代死代码消除(struct IR模块*, long long);

struct 死代码消除上下文* 创建死代码消除上下文(struct IR模块* cn_var_模块) {
  long long r0;
  void* r1;
  struct 死代码消除上下文* r2;
  struct IR模块* r3;
  struct 死代码消除上下文* r4;

  entry:
  struct 死代码消除上下文* cn_var_上下文_0;
  r0 = sizeof(struct 死代码消除上下文);
  r1 = 分配内存(r0);
  cn_var_上下文_0 = r1;
  r2 = cn_var_上下文_0;
  if (r2) goto if_then_0; else goto if_merge_1;

  if_then_0:
  r3 = cn_var_模块;
  goto if_merge_1;

  if_merge_1:
  r4 = cn_var_上下文_0;
  return r4;
}

void 释放死代码消除上下文(struct 死代码消除上下文* cn_var_上下文) {
  struct 死代码消除上下文* r0;
  struct 死代码消除上下文* r1;

  entry:
  r0 = cn_var_上下文;
  if (r0) goto if_then_2; else goto if_merge_3;

  if_then_2:
  r1 = cn_var_上下文;
  释放内存(r1);
  goto if_merge_3;

  if_merge_3:
  return;
}

void 标记基本块已访问(struct 基本块* cn_var_块) {
  long long r0;
  struct 基本块* r1;
  struct 基本块节点* r4;
  struct 基本块节点* r5;
  struct 基本块* r6;
  struct 基本块节点* r7;
  _Bool r2;
  _Bool r3;

  entry:
  r1 = cn_var_块;
  r2 = !r1;
  if (r2) goto logic_merge_7; else goto logic_rhs_6;

  if_then_4:
  return;
  goto if_merge_5;

  if_merge_5:
  struct 基本块节点* cn_var_后继_0;
  r4 = cn_var_块->后继列表;
  cn_var_后继_0 = r4;
  goto while_cond_8;

  logic_rhs_6:
  r3 = cn_var_块->已访问;
  goto logic_merge_7;

  logic_merge_7:
  if (r3) goto if_then_4; else goto if_merge_5;

  while_cond_8:
  r5 = cn_var_后继_0;
  if (r5) goto while_body_9; else goto while_exit_10;

  while_body_9:
  r6 = cn_var_后继_0->块;
  标记基本块已访问(r6);
  r7 = cn_var_后继_0->下一个;
  cn_var_后继_0 = r7;
  goto while_cond_8;

  while_exit_10:
  return;
}

void 标记可达基本块(struct IR函数* cn_var_函数指针) {
  long long r0;
  struct IR函数* r1;
  struct 基本块* r3;
  struct 基本块* r5;
  struct 基本块* r6;
  struct 基本块* r7;
  struct 基本块* r8;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_14; else goto logic_rhs_13;

  if_then_11:
  return;
  goto if_merge_12;

  if_merge_12:
  struct 基本块* cn_var_块_0;
  r5 = cn_var_函数指针->首个块;
  cn_var_块_0 = r5;
  goto while_cond_15;

  logic_rhs_13:
  r3 = cn_var_函数指针->首个块;
  r4 = !r3;
  goto logic_merge_14;

  logic_merge_14:
  if (r4) goto if_then_11; else goto if_merge_12;

  while_cond_15:
  r6 = cn_var_块_0;
  if (r6) goto while_body_16; else goto while_exit_17;

  while_body_16:
  r7 = cn_var_块_0->下一个;
  cn_var_块_0 = r7;
  goto while_cond_15;

  while_exit_17:
  r8 = cn_var_函数指针->首个块;
  标记基本块已访问(r8);
  return;
}

void 消除不可达基本块(struct IR函数* cn_var_函数指针, struct 死代码消除上下文* cn_var_上下文) {
  long long r0, r18, r19;
  struct IR函数* r1;
  struct 死代码消除上下文* r3;
  struct IR函数* r5;
  struct 基本块* r6;
  struct 基本块* r7;
  struct 基本块* r8;
  struct 基本块* r11;
  struct 基本块* r12;
  struct 基本块* r13;
  struct 基本块* r14;
  struct 基本块* r15;
  struct 基本块* r16;
  struct 基本块* r17;
  struct 基本块* r20;
  struct 基本块* r21;
  _Bool r2;
  _Bool r4;
  _Bool r9;
  _Bool r10;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_21; else goto logic_rhs_20;

  if_then_18:
  return;
  goto if_merge_19;

  if_merge_19:
  r5 = cn_var_函数指针;
  标记可达基本块(r5);
  struct 基本块* cn_var_块_0;
  r6 = cn_var_函数指针->首个块;
  cn_var_块_0 = r6;
  struct 基本块* cn_var_前一个_1;
  cn_var_前一个_1 = 0;
  goto while_cond_22;

  logic_rhs_20:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_21;

  logic_merge_21:
  if (r4) goto if_then_18; else goto if_merge_19;

  while_cond_22:
  r7 = cn_var_块_0;
  if (r7) goto while_body_23; else goto while_exit_24;

  while_body_23:
  struct 基本块* cn_var_下一个_2;
  r8 = cn_var_块_0->下一个;
  cn_var_下一个_2 = r8;
  r9 = cn_var_块_0->已访问;
  r10 = !r9;
  if (r10) goto if_then_25; else goto if_else_26;

  while_exit_24:

  if_then_25:
  r11 = cn_var_前一个_1;
  if (r11) goto if_then_28; else goto if_else_29;

  if_else_26:
  r20 = cn_var_块_0;
  cn_var_前一个_1 = r20;
  goto if_merge_27;

  if_merge_27:
  r21 = cn_var_下一个_2;
  cn_var_块_0 = r21;
  goto while_cond_22;

  if_then_28:
  r12 = cn_var_下一个_2;
  goto if_merge_30;

  if_else_29:
  r13 = cn_var_下一个_2;
  goto if_merge_30;

  if_merge_30:
  r14 = cn_var_下一个_2;
  if (r14) goto if_then_31; else goto if_else_32;

  if_then_31:
  r15 = cn_var_前一个_1;
  goto if_merge_33;

  if_else_32:
  r16 = cn_var_前一个_1;
  goto if_merge_33;

  if_merge_33:
  r17 = cn_var_块_0;
  释放基本块(r17);
  r18 = cn_var_上下文->消除次数;
  r19 = r18 + 1;
  goto if_merge_27;
  return;
}

struct 使用信息* 创建使用信息数组(long long cn_var_寄存器数量) {
  long long r0, r1;
  void* r2;
  struct 使用信息* r3;

  entry:
  struct 使用信息* cn_var_信息_0;
  r0 = cn_var_寄存器数量;
  r1 = sizeof(struct 使用信息);
  r2 = 分配清零内存(r0, r1);
  cn_var_信息_0 = r2;
  r3 = cn_var_信息_0;
  return r3;
}

void 释放使用信息数组(struct 使用信息* cn_var_信息) {
  struct 使用信息* r0;
  struct 使用信息* r1;

  entry:
  r0 = cn_var_信息;
  if (r0) goto if_then_34; else goto if_merge_35;

  if_then_34:
  r1 = cn_var_信息;
  释放内存(r1);
  goto if_merge_35;

  if_merge_35:
  return;
}

void 记录定义(struct 使用信息* cn_var_信息, long long cn_var_寄存器编号) {
  long long r0, r3, r6, r8, r9;
  struct 使用信息* r1;
  struct 使用信息* r5;
  _Bool r2;
  _Bool r4;
  struct 使用信息 r7;

  entry:
  r1 = cn_var_信息;
  r2 = !r1;
  if (r2) goto logic_merge_39; else goto logic_rhs_38;

  if_then_36:
  return;
  goto if_merge_37;

  if_merge_37:
  r5 = cn_var_信息;
  r6 = cn_var_寄存器编号;
  r7 = (struct 使用信息*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7.定义次数;
  r9 = r8 + 1;

  logic_rhs_38:
  r3 = cn_var_寄存器编号;
  r4 = r3 < 0;
  goto logic_merge_39;

  logic_merge_39:
  if (r4) goto if_then_36; else goto if_merge_37;
  return;
}

void 记录使用(struct 使用信息* cn_var_信息, long long cn_var_寄存器编号) {
  long long r0, r3, r6, r8, r9;
  struct 使用信息* r1;
  struct 使用信息* r5;
  _Bool r2;
  _Bool r4;
  struct 使用信息 r7;

  entry:
  r1 = cn_var_信息;
  r2 = !r1;
  if (r2) goto logic_merge_43; else goto logic_rhs_42;

  if_then_40:
  return;
  goto if_merge_41;

  if_merge_41:
  r5 = cn_var_信息;
  r6 = cn_var_寄存器编号;
  r7 = (struct 使用信息*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7.使用次数;
  r9 = r8 + 1;

  logic_rhs_42:
  r3 = cn_var_寄存器编号;
  r4 = r3 < 0;
  goto logic_merge_43;

  logic_merge_43:
  if (r4) goto if_then_40; else goto if_merge_41;
  return;
}

void 分析操作数使用(struct IR操作数 cn_var_操作数, struct 使用信息* cn_var_信息, _Bool cn_var_是定义) {
  long long r4, r6;
  struct 使用信息* r3;
  struct 使用信息* r5;
  _Bool r1;
  _Bool r2;
  enum IR操作数种类 r0;

  entry:
  r0 = cn_var_操作数.种类;
  r1 = r0 == IR操作数种类_虚拟寄存器;
  if (r1) goto if_then_44; else goto if_merge_45;

  if_then_44:
  r2 = cn_var_是定义;
  if (r2) goto if_then_46; else goto if_else_47;

  if_merge_45:

  if_then_46:
  r3 = cn_var_信息;
  r4 = cn_var_操作数.寄存器编号;
  记录定义(r3, r4);
  goto if_merge_48;

  if_else_47:
  r5 = cn_var_信息;
  r6 = cn_var_操作数.寄存器编号;
  记录使用(r5, r6);
  goto if_merge_48;

  if_merge_48:
  goto if_merge_45;
  return;
}

void 分析指令使用(struct IR指令* cn_var_指令, struct 使用信息* cn_var_信息) {
  long long r0, r12, r13, r16, r19, r20;
  struct IR指令* r1;
  struct 使用信息* r3;
  struct 使用信息* r6;
  struct 使用信息* r8;
  struct 使用信息* r10;
  struct IR操作数* r11;
  struct IR操作数* r15;
  void* r17;
  struct 使用信息* r18;
  _Bool r2;
  _Bool r4;
  _Bool r14;
  struct IR操作数 r5;
  struct IR操作数 r7;
  struct IR操作数 r9;

  entry:
  r1 = cn_var_指令;
  r2 = !r1;
  if (r2) goto logic_merge_52; else goto logic_rhs_51;

  if_then_49:
  return;
  goto if_merge_50;

  if_merge_50:
  r5 = cn_var_指令->目标;
  r6 = cn_var_信息;
  分析操作数使用(r5, r6, 1);
  r7 = cn_var_指令->源操作数1;
  r8 = cn_var_信息;
  分析操作数使用(r7, r8, 0);
  r9 = cn_var_指令->源操作数2;
  r10 = cn_var_信息;
  分析操作数使用(r9, r10, 0);
  r11 = cn_var_指令->额外参数;
  if (r11) goto if_then_53; else goto if_merge_54;

  logic_rhs_51:
  r3 = cn_var_信息;
  r4 = !r3;
  goto logic_merge_52;

  logic_merge_52:
  if (r4) goto if_then_49; else goto if_merge_50;

  if_then_53:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto while_cond_55;

  if_merge_54:

  while_cond_55:
  r12 = cn_var_i_0;
  r13 = cn_var_指令->额外参数数量;
  r14 = r12 < r13;
  if (r14) goto while_body_56; else goto while_exit_57;

  while_body_56:
  r15 = cn_var_指令->额外参数;
  r16 = cn_var_i_0;
  r17 = (void*)cn_rt_array_get_element(r15, r16, 8);
  r18 = cn_var_信息;
  分析操作数使用(r17, r18, 0);
  r19 = cn_var_i_0;
  r20 = r19 + 1;
  cn_var_i_0 = r20;
  goto while_cond_55;

  while_exit_57:
  goto if_merge_54;
  return;
}

void 分析基本块使用(struct 基本块* cn_var_块, struct 使用信息* cn_var_信息) {
  long long r0;
  struct 基本块* r1;
  struct 使用信息* r3;
  struct IR指令* r5;
  struct IR指令* r6;
  struct IR指令* r7;
  struct 使用信息* r8;
  struct IR指令* r9;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_块;
  r2 = !r1;
  if (r2) goto logic_merge_61; else goto logic_rhs_60;

  if_then_58:
  return;
  goto if_merge_59;

  if_merge_59:
  struct IR指令* cn_var_指令_0;
  r5 = cn_var_块->首条指令;
  cn_var_指令_0 = r5;
  goto while_cond_62;

  logic_rhs_60:
  r3 = cn_var_信息;
  r4 = !r3;
  goto logic_merge_61;

  logic_merge_61:
  if (r4) goto if_then_58; else goto if_merge_59;

  while_cond_62:
  r6 = cn_var_指令_0;
  if (r6) goto while_body_63; else goto while_exit_64;

  while_body_63:
  r7 = cn_var_指令_0;
  r8 = cn_var_信息;
  分析指令使用(r7, r8);
  r9 = cn_var_指令_0->下一条;
  cn_var_指令_0 = r9;
  goto while_cond_62;

  while_exit_64:
  return;
}

void 分析函数使用(struct IR函数* cn_var_函数指针, struct 使用信息* cn_var_信息) {
  long long r0;
  struct IR函数* r1;
  struct 使用信息* r3;
  struct 基本块* r5;
  struct 基本块* r6;
  struct 基本块* r7;
  struct 使用信息* r8;
  struct 基本块* r9;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_68; else goto logic_rhs_67;

  if_then_65:
  return;
  goto if_merge_66;

  if_merge_66:
  struct 基本块* cn_var_块_0;
  r5 = cn_var_函数指针->首个块;
  cn_var_块_0 = r5;
  goto while_cond_69;

  logic_rhs_67:
  r3 = cn_var_信息;
  r4 = !r3;
  goto logic_merge_68;

  logic_merge_68:
  if (r4) goto if_then_65; else goto if_merge_66;

  while_cond_69:
  r6 = cn_var_块_0;
  if (r6) goto while_body_70; else goto while_exit_71;

  while_body_70:
  r7 = cn_var_块_0;
  r8 = cn_var_信息;
  分析基本块使用(r7, r8);
  r9 = cn_var_块_0->下一个;
  cn_var_块_0 = r9;
  goto while_cond_69;

  while_exit_71:
  return;
}

_Bool 可以删除指令(struct IR指令* cn_var_指令, struct 使用信息* cn_var_信息) {
  long long r0, r15, r16, r18, r22;
  struct IR指令* r1;
  struct 使用信息* r3;
  struct 使用信息* r17;
  struct 使用信息* r21;
  _Bool r2;
  _Bool r4;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r13;
  _Bool r20;
  _Bool r24;
  _Bool r25;
  struct IR操作数 r11;
  struct IR操作数 r14;
  struct 使用信息 r19;
  struct 使用信息 r23;
  enum IR操作码 r5;
  enum IR操作数种类 r12;

  entry:
  r1 = cn_var_指令;
  r2 = !r1;
  if (r2) goto logic_merge_75; else goto logic_rhs_74;

  if_then_72:
  return 0;
  goto if_merge_73;

  if_merge_73:
  r5 = cn_var_指令->操作码;
  r6 = r5 == IR指令种类_存储指令;
  if (r6) goto case_body_77; else goto switch_check_83;

  logic_rhs_74:
  r3 = cn_var_信息;
  r4 = !r3;
  goto logic_merge_75;

  logic_merge_75:
  if (r4) goto if_then_72; else goto if_merge_73;

  switch_check_83:
  r7 = r5 == IR指令种类_调用指令;
  if (r7) goto case_body_78; else goto switch_check_84;

  switch_check_84:
  r8 = r5 == IR指令种类_返回指令;
  if (r8) goto case_body_79; else goto switch_check_85;

  switch_check_85:
  r9 = r5 == IR指令种类_跳转指令;
  if (r9) goto case_body_80; else goto switch_check_86;

  switch_check_86:
  r10 = r5 == IR指令种类_条件跳转指令;
  if (r10) goto case_body_81; else goto case_default_82;

  case_body_77:
  goto switch_merge_76;

  case_body_78:
  goto switch_merge_76;

  case_body_79:
  goto switch_merge_76;

  case_body_80:
  goto switch_merge_76;

  case_body_81:
  return 0;
  goto switch_merge_76;

  case_default_82:
  goto switch_merge_76;
  goto switch_merge_76;

  switch_merge_76:
  r11 = cn_var_指令->目标;
  r12 = r11.种类;
  r13 = r12 == IR操作数种类_虚拟寄存器;
  if (r13) goto if_then_87; else goto if_merge_88;

  if_then_87:
  long long cn_var_编号_0;
  r14 = cn_var_指令->目标;
  r15 = r14.寄存器编号;
  cn_var_编号_0 = r15;
  r17 = cn_var_信息;
  r18 = cn_var_编号_0;
  r19 = (struct 使用信息*)cn_rt_array_get_element(r17, r18, 8);
  r20 = r19.已定义;
  if (r20) goto logic_rhs_91; else goto logic_merge_92;

  if_merge_88:
  return 0;

  if_then_89:
  return 1;
  goto if_merge_90;

  if_merge_90:
  goto if_merge_88;

  logic_rhs_91:
  r21 = cn_var_信息;
  r22 = cn_var_编号_0;
  r23 = (struct 使用信息*)cn_rt_array_get_element(r21, r22, 8);
  r24 = r23.已使用;
  r25 = !r24;
  goto logic_merge_92;

  logic_merge_92:
  if (r25) goto if_then_89; else goto if_merge_90;
  return 0;
}

void 消除基本块死代码(struct 基本块* cn_var_块, struct 使用信息* cn_var_信息, struct 死代码消除上下文* cn_var_上下文) {
  long long r0, r1, r21, r22;
  struct 基本块* r2;
  struct 使用信息* r4;
  struct 死代码消除上下文* r6;
  struct IR指令* r8;
  struct IR指令* r9;
  struct IR指令* r10;
  struct IR指令* r11;
  struct 使用信息* r12;
  struct IR指令* r14;
  struct IR指令* r15;
  struct IR指令* r16;
  struct IR指令* r17;
  struct IR指令* r18;
  struct IR指令* r19;
  struct IR指令* r20;
  struct IR指令* r23;
  struct IR指令* r24;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r2 = cn_var_块;
  r3 = !r2;
  if (r3) goto logic_merge_98; else goto logic_rhs_97;

  if_then_93:
  return;
  goto if_merge_94;

  if_merge_94:
  struct IR指令* cn_var_指令_0;
  r8 = cn_var_块->首条指令;
  cn_var_指令_0 = r8;
  struct IR指令* cn_var_前一条_1;
  cn_var_前一条_1 = 0;
  goto while_cond_99;

  logic_rhs_95:
  r6 = cn_var_上下文;
  r7 = !r6;
  goto logic_merge_96;

  logic_merge_96:
  if (r7) goto if_then_93; else goto if_merge_94;

  logic_rhs_97:
  r4 = cn_var_信息;
  r5 = !r4;
  goto logic_merge_98;

  logic_merge_98:
  if (r5) goto logic_merge_96; else goto logic_rhs_95;

  while_cond_99:
  r9 = cn_var_指令_0;
  if (r9) goto while_body_100; else goto while_exit_101;

  while_body_100:
  struct IR指令* cn_var_下一条_2;
  r10 = cn_var_指令_0->下一条;
  cn_var_下一条_2 = r10;
  r11 = cn_var_指令_0;
  r12 = cn_var_信息;
  r13 = 可以删除指令(r11, r12);
  if (r13) goto if_then_102; else goto if_else_103;

  while_exit_101:

  if_then_102:
  r14 = cn_var_前一条_1;
  if (r14) goto if_then_105; else goto if_else_106;

  if_else_103:
  r23 = cn_var_指令_0;
  cn_var_前一条_1 = r23;
  goto if_merge_104;

  if_merge_104:
  r24 = cn_var_下一条_2;
  cn_var_指令_0 = r24;
  goto while_cond_99;

  if_then_105:
  r15 = cn_var_下一条_2;
  goto if_merge_107;

  if_else_106:
  r16 = cn_var_下一条_2;
  goto if_merge_107;

  if_merge_107:
  r17 = cn_var_下一条_2;
  if (r17) goto if_then_108; else goto if_else_109;

  if_then_108:
  r18 = cn_var_前一条_1;
  goto if_merge_110;

  if_else_109:
  r19 = cn_var_前一条_1;
  goto if_merge_110;

  if_merge_110:
  r20 = cn_var_指令_0;
  释放IR指令(r20);
  r21 = cn_var_上下文->消除次数;
  r22 = r21 + 1;
  goto if_merge_104;
  return;
}

void 消除函数死代码(struct IR函数* cn_var_函数指针, struct 死代码消除上下文* cn_var_上下文) {
  long long r0, r7, r8;
  struct IR函数* r1;
  struct 死代码消除上下文* r3;
  struct IR函数* r5;
  struct 死代码消除上下文* r6;
  struct 使用信息* r9;
  struct 使用信息* r10;
  struct IR函数* r12;
  struct 使用信息* r13;
  struct 基本块* r14;
  struct 基本块* r15;
  struct 基本块* r16;
  struct 使用信息* r17;
  struct 死代码消除上下文* r18;
  struct 基本块* r19;
  struct 使用信息* r20;
  _Bool r2;
  _Bool r4;
  _Bool r11;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_114; else goto logic_rhs_113;

  if_then_111:
  return;
  goto if_merge_112;

  if_merge_112:
  r5 = cn_var_函数指针;
  r6 = cn_var_上下文;
  消除不可达基本块(r5, r6);
  struct 使用信息* cn_var_信息_0;
  r7 = cn_var_函数指针->下个寄存器编号;
  r8 = r7 + 1;
  r9 = 创建使用信息数组(r8);
  cn_var_信息_0 = r9;
  r10 = cn_var_信息_0;
  r11 = !r10;
  if (r11) goto if_then_115; else goto if_merge_116;

  logic_rhs_113:
  r3 = cn_var_上下文;
  r4 = !r3;
  goto logic_merge_114;

  logic_merge_114:
  if (r4) goto if_then_111; else goto if_merge_112;

  if_then_115:
  return;
  goto if_merge_116;

  if_merge_116:
  r12 = cn_var_函数指针;
  r13 = cn_var_信息_0;
  分析函数使用(r12, r13);
  struct 基本块* cn_var_块_1;
  r14 = cn_var_函数指针->首个块;
  cn_var_块_1 = r14;
  goto while_cond_117;

  while_cond_117:
  r15 = cn_var_块_1;
  if (r15) goto while_body_118; else goto while_exit_119;

  while_body_118:
  r16 = cn_var_块_1;
  r17 = cn_var_信息_0;
  r18 = cn_var_上下文;
  消除基本块死代码(r16, r17, r18);
  r19 = cn_var_块_1->下一个;
  cn_var_块_1 = r19;
  goto while_cond_117;

  while_exit_119:
  r20 = cn_var_信息_0;
  释放使用信息数组(r20);
  return;
}

long long 消除模块死代码(struct IR模块* cn_var_模块) {
  long long r11, r13;
  struct IR模块* r0;
  struct IR模块* r2;
  struct 死代码消除上下文* r3;
  struct 死代码消除上下文* r4;
  struct IR函数* r6;
  struct IR函数* r7;
  struct IR函数* r8;
  struct 死代码消除上下文* r9;
  struct IR函数* r10;
  struct 死代码消除上下文* r12;
  _Bool r1;
  _Bool r5;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_120; else goto if_merge_121;

  if_then_120:
  return 0;
  goto if_merge_121;

  if_merge_121:
  struct 死代码消除上下文* cn_var_上下文_0;
  r2 = cn_var_模块;
  r3 = 创建死代码消除上下文(r2);
  cn_var_上下文_0 = r3;
  r4 = cn_var_上下文_0;
  r5 = !r4;
  if (r5) goto if_then_122; else goto if_merge_123;

  if_then_122:
  return 0;
  goto if_merge_123;

  if_merge_123:
  struct IR函数* cn_var_函数指针_1;
  r6 = cn_var_模块->首个函数;
  cn_var_函数指针_1 = r6;
  goto while_cond_124;

  while_cond_124:
  r7 = cn_var_函数指针_1;
  if (r7) goto while_body_125; else goto while_exit_126;

  while_body_125:
  r8 = cn_var_函数指针_1;
  r9 = cn_var_上下文_0;
  消除函数死代码(r8, r9);
  r10 = cn_var_函数指针_1->下一个;
  cn_var_函数指针_1 = r10;
  goto while_cond_124;

  while_exit_126:
  long long cn_var_次数_2;
  r11 = cn_var_上下文_0->消除次数;
  cn_var_次数_2 = r11;
  r12 = cn_var_上下文_0;
  释放死代码消除上下文(r12);
  r13 = cn_var_次数_2;
  return r13;
}

long long 迭代死代码消除(struct IR模块* cn_var_模块, long long cn_var_最大迭代次数) {
  long long r2, r3, r6, r7, r8, r9, r10, r12, r13, r14;
  struct IR模块* r0;
  struct IR模块* r5;
  _Bool r1;
  _Bool r4;
  _Bool r11;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_127; else goto if_merge_128;

  if_then_127:
  return 0;
  goto if_merge_128;

  if_merge_128:
  long long cn_var_总次数_0;
  cn_var_总次数_0 = 0;
  long long cn_var_迭代_1;
  cn_var_迭代_1 = 0;
  goto while_cond_129;

  while_cond_129:
  r2 = cn_var_迭代_1;
  r3 = cn_var_最大迭代次数;
  r4 = r2 < r3;
  if (r4) goto while_body_130; else goto while_exit_131;

  while_body_130:
  long long cn_var_次数_2;
  r5 = cn_var_模块;
  r6 = 消除模块死代码(r5);
  cn_var_次数_2 = r6;
  r7 = cn_var_总次数_0;
  r8 = cn_var_次数_2;
  r9 = r7 + r8;
  cn_var_总次数_0 = r9;
  r10 = cn_var_次数_2;
  r11 = r10 == 0;
  if (r11) goto if_then_132; else goto if_merge_133;

  while_exit_131:
  r14 = cn_var_总次数_0;
  return r14;

  if_then_132:
  goto while_exit_131;
  goto if_merge_133;

  if_merge_133:
  r12 = cn_var_迭代_1;
  r13 = r12 + 1;
  cn_var_迭代_1 = r13;
  goto while_cond_129;
  return 0;
}

