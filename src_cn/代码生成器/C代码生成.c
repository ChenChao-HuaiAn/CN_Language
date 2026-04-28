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
  if (r2) goto if_then_927; else goto if_merge_928;

  if_then_927:
  r3 = cn_var_初始容量;
  r4 = 分配内存(r3);
  r5 = cn_var_初始容量;
  r6 = cn_var_缓冲区_0->数据;
  if (r6) goto if_then_929; else goto if_merge_930;

  if_merge_928:
  r8 = cn_var_缓冲区_0;
  return r8;

  if_then_929:
  r7 = cn_var_缓冲区_0->数据;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r7, 0, &_tmp_i0, 8); }
  goto if_merge_930;

  if_merge_930:
  goto if_merge_928;
  return NULL;
}

void 释放输出缓冲区(struct 输出缓冲区* cn_var_缓冲区) {
  char* r1;
  char* r2;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r3;

  entry:
  r0 = cn_var_缓冲区;
  if (r0) goto if_then_931; else goto if_merge_932;

  if_then_931:
  r1 = cn_var_缓冲区->数据;
  if (r1) goto if_then_933; else goto if_merge_934;

  if_merge_932:

  if_then_933:
  r2 = cn_var_缓冲区->数据;
  释放内存(r2);
  goto if_merge_934;

  if_merge_934:
  r3 = cn_var_缓冲区;
  释放内存(r3);
  goto if_merge_932;
  return;
}

_Bool 扩展缓冲区(struct 输出缓冲区* cn_var_缓冲区, long long cn_var_新容量) {
  long long r0, r2, r3, r4, r5, r7, r11;
  char* r6;
  char* r9;
  char* r10;
  struct 输出缓冲区* r1;
  void* r8;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_938; else goto logic_rhs_937;

  if_then_935:
  return 0;
  goto if_merge_936;

  if_merge_936:
  char* cn_var_新数据_0;
  r6 = cn_var_缓冲区->数据;
  r7 = cn_var_新容量;
  r8 = 重新分配内存(r6, r7);
  cn_var_新数据_0 = r8;
  r9 = cn_var_新数据_0;
  if (r9) goto if_then_939; else goto if_merge_940;

  logic_rhs_937:
  r3 = cn_var_新容量;
  r4 = cn_var_缓冲区->容量;
  r5 = r3 <= r4;
  goto logic_merge_938;

  logic_merge_938:
  if (r5) goto if_then_935; else goto if_merge_936;

  if_then_939:
  r10 = cn_var_新数据_0;
  r11 = cn_var_新容量;
  return 1;
  goto if_merge_940;

  if_merge_940:
  return 0;
}

_Bool 追加字符串(struct 输出缓冲区* cn_var_缓冲区, const char* cn_var_字符串值) {
  long long r0, r2, r4, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r22, r24, r26, r27, r29, r31, r32, r33, r35;
  char* r3;
  char* r5;
  char* r25;
  char* r28;
  char* r34;
  struct 输出缓冲区* r1;
  struct 输出缓冲区* r21;
  void* r30;
  _Bool r23;

  entry:
  r1 = cn_var_缓冲区;
  r2 = !r1;
  if (r2) goto logic_merge_944; else goto logic_rhs_943;

  if_then_941:
  return 0;
  goto if_merge_942;

  if_merge_942:
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
  if (r13) goto if_then_945; else goto if_merge_946;

  logic_rhs_943:
  r3 = cn_var_字符串值;
  r4 = !r3;
  goto logic_merge_944;

  logic_merge_944:
  if (r4) goto if_then_941; else goto if_merge_942;

  if_then_945:
  long long cn_var_新容量_2;
  r14 = cn_var_缓冲区->容量;
  r15 = r14 << 1;
  cn_var_新容量_2 = r15;
  goto while_cond_947;

  if_merge_946:
  r25 = cn_var_缓冲区->数据;
  r26 = cn_var_缓冲区->长度;
  r27 = r25 + r26;
  r28 = cn_var_字符串值;
  r29 = cn_var_字符串长度_0;
  r30 = 复制内存(r27, r28, r29);
  r31 = cn_var_缓冲区->长度;
  r32 = cn_var_字符串长度_0;
  r33 = r31 + r32;
  r34 = cn_var_缓冲区->数据;
  r35 = cn_var_缓冲区->长度;
    { long long _tmp_i1 = 0; cn_rt_array_set_element(r34, r35, &_tmp_i1, 8); }
  return 1;

  while_cond_947:
  r16 = cn_var_新容量_2;
  r17 = cn_var_需要容量_1;
  r18 = r16 < r17;
  if (r18) goto while_body_948; else goto while_exit_949;

  while_body_948:
  r19 = cn_var_新容量_2;
  r20 = r19 << 1;
  cn_var_新容量_2 = r20;
  goto while_cond_947;

  while_exit_949:
  r21 = cn_var_缓冲区;
  r22 = cn_var_新容量_2;
  r23 = 扩展缓冲区(r21, r22);
  r24 = !r23;
  if (r24) goto if_then_950; else goto if_merge_951;

  if_then_950:
  return 0;
  goto if_merge_951;

  if_merge_951:
  goto if_merge_946;
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
  cn_var_字符串缓冲_0 = (char*)分配内存(2);
  r1 = cn_var_字符值;
  r2 = cn_var_字符串缓冲_0;
    { long long _tmp_r4 = r1; cn_rt_array_set_element(r2, 0, &_tmp_r4, 8); }
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
  cn_var_字符串缓冲_0 = (char*)分配内存(32);
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
  long long r1, r2, r3, r4, r5, r6, r9, r10;
  struct 输出缓冲区* r0;
  struct 输出缓冲区* r7;
  _Bool r8;

  entry:
  r0 = cn_var_缓冲区;
  r1 = !r0;
  if (r1) goto if_then_952; else goto if_merge_953;

  if_then_952:
  return;
  goto if_merge_953;

  if_merge_953:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_954;

  for_cond_954:
  r2 = cn_var_i_0;
  r3 = cn_var_缓冲区->缩进级别;
  r4 = cn_var_缩进宽度;
  r5 = r3 * r4;
  r6 = r2 < r5;
  if (r6) goto for_body_955; else goto for_exit_957;

  for_body_955:
  r7 = cn_var_缓冲区;
  r8 = 追加字符(r7, 32);
  goto for_update_956;

  for_update_956:
  r9 = cn_var_i_0;
  r10 = r9 + 1;
  cn_var_i_0 = r10;
  goto for_cond_954;

  for_exit_957:
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
  long long r0, r2, r3, r4, r5, r6;
  struct 输出缓冲区* r1;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_960; else goto logic_merge_961;

  if_then_958:
  r5 = cn_var_缓冲区->缩进级别;
  r6 = r5 + 1;
  goto if_merge_959;

  if_merge_959:

  logic_rhs_960:
  r2 = cn_var_缓冲区->缩进级别;
  r3 = cn_var_最大缩进级别;
  r4 = r2 < r3;
  goto logic_merge_961;

  logic_merge_961:
  if (r4) goto if_then_958; else goto if_merge_959;
  return;
}

void 减少缩进(struct 输出缓冲区* cn_var_缓冲区) {
  long long r0, r2, r3, r4, r5;
  struct 输出缓冲区* r1;

  entry:
  r1 = cn_var_缓冲区;
  if (r1) goto logic_rhs_964; else goto logic_merge_965;

  if_then_962:
  r4 = cn_var_缓冲区->缩进级别;
  r5 = r4 - 1;
  goto if_merge_963;

  if_merge_963:

  logic_rhs_964:
  r2 = cn_var_缓冲区->缩进级别;
  r3 = r2 > 0;
  goto logic_merge_965;

  logic_merge_965:
  if (r3) goto if_then_962; else goto if_merge_963;
  return;
}

char* 类型到C类型(struct 类型信息* cn_var_类型, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r29, r30, r66, r67, r68, r69, r70, r74, r78, r79;
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
  struct 类型信息* r75;
  struct 输出缓冲区* r76;
  struct 输出缓冲区* r80;
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
  _Bool r72;
  _Bool r81;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_966; else goto if_merge_967;

  if_then_966:
  return "void";
  goto if_merge_967;

  if_merge_967:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_空;
  if (r3) goto case_body_969; else goto switch_check_983;

  switch_check_983:
  r4 = r2 == 类型种类_类型_整数;
  if (r4) goto case_body_970; else goto switch_check_984;

  switch_check_984:
  r5 = r2 == 类型种类_类型_小数;
  if (r5) goto case_body_971; else goto switch_check_985;

  switch_check_985:
  r6 = r2 == 类型种类_类型_布尔;
  if (r6) goto case_body_972; else goto switch_check_986;

  switch_check_986:
  r7 = r2 == 类型种类_类型_字符串;
  if (r7) goto case_body_973; else goto switch_check_987;

  switch_check_987:
  r8 = r2 == 类型种类_类型_字符;
  if (r8) goto case_body_974; else goto switch_check_988;

  switch_check_988:
  r9 = r2 == 类型种类_类型_指针;
  if (r9) goto case_body_975; else goto switch_check_989;

  switch_check_989:
  r10 = r2 == 类型种类_类型_数组;
  if (r10) goto case_body_976; else goto switch_check_990;

  switch_check_990:
  r11 = r2 == 类型种类_类型_结构体;
  if (r11) goto case_body_977; else goto switch_check_991;

  switch_check_991:
  r12 = r2 == 类型种类_类型_枚举;
  if (r12) goto case_body_978; else goto switch_check_992;

  switch_check_992:
  r13 = r2 == 类型种类_类型_类;
  if (r13) goto case_body_979; else goto switch_check_993;

  switch_check_993:
  r14 = r2 == 类型种类_类型_接口;
  if (r14) goto case_body_980; else goto switch_check_994;

  switch_check_994:
  r15 = r2 == 类型种类_类型_函数;
  if (r15) goto case_body_981; else goto case_default_982;

  case_body_969:
  return "void";
  goto switch_merge_968;

  case_body_970:
  return "int";
  goto switch_merge_968;

  case_body_971:
  return "double";
  goto switch_merge_968;

  case_body_972:
  return "_Bool";
  goto switch_merge_968;

  case_body_973:
  return "cn_string_t";
  goto switch_merge_968;

  case_body_974:
  return "char";
  goto switch_merge_968;

  case_body_975:
  r16 = cn_var_类型->指向类型;
  r17 = cn_var_缓冲区;
  r18 = 类型到C类型(r16, r17);
  r19 = cn_var_缓冲区;
  r20 = 追加字符串(r19, "*");
  r21 = cn_var_缓冲区->数据;
  return r21;
  goto switch_merge_968;

  case_body_976:
  r22 = cn_var_类型->元素类型;
  r23 = cn_var_缓冲区;
  r24 = 类型到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, "[");
  r27 = cn_var_缓冲区;
  r28 = cn_var_类型->维度大小;
  r29 = *(long long)cn_rt_array_get_element(r28, 0, 8);
  r30 = 0 + r29;
  r31 = 追加整数(r27, r30);
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "]");
  r34 = cn_var_缓冲区->数据;
  return r34;
  goto switch_merge_968;

  case_body_977:
  r35 = cn_var_缓冲区;
  r36 = 追加字符串(r35, "struct ");
  r37 = cn_var_缓冲区;
  r38 = cn_var_类型->名称;
  r39 = 追加字符串(r37, r38);
  r40 = cn_var_缓冲区->数据;
  return r40;
  goto switch_merge_968;

  case_body_978:
  r41 = cn_var_缓冲区;
  r42 = 追加字符串(r41, "enum ");
  r43 = cn_var_缓冲区;
  r44 = cn_var_类型->名称;
  r45 = 追加字符串(r43, r44);
  r46 = cn_var_缓冲区->数据;
  return r46;
  goto switch_merge_968;

  case_body_979:
  r47 = cn_var_缓冲区;
  r48 = 追加字符串(r47, "struct ");
  r49 = cn_var_缓冲区;
  r50 = cn_var_类型->名称;
  r51 = 追加字符串(r49, r50);
  r52 = cn_var_缓冲区->数据;
  return r52;
  goto switch_merge_968;

  case_body_980:
  r53 = cn_var_缓冲区;
  r54 = 追加字符串(r53, "struct ");
  r55 = cn_var_缓冲区;
  r56 = cn_var_类型->名称;
  r57 = 追加字符串(r55, r56);
  r58 = cn_var_缓冲区;
  r59 = 追加字符串(r58, "_vtable");
  r60 = cn_var_缓冲区->数据;
  return r60;
  goto switch_merge_968;

  case_body_981:
  r61 = cn_var_类型->返回类型;
  r62 = cn_var_缓冲区;
  r63 = 类型到C类型(r61, r62);
  r64 = cn_var_缓冲区;
  r65 = 追加字符串(r64, " (*)(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_995;

  for_cond_995:
  r66 = cn_var_i_0;
  r67 = cn_var_类型->参数个数;
  r68 = r66 < r67;
  if (r68) goto for_body_996; else goto for_exit_998;

  for_body_996:
  r69 = cn_var_i_0;
  r70 = r69 > 0;
  if (r70) goto if_then_999; else goto if_merge_1000;

  for_update_997:
  r78 = cn_var_i_0;
  r79 = r78 + 1;
  cn_var_i_0 = r79;
  goto for_cond_995;

  for_exit_998:
  r80 = cn_var_缓冲区;
  r81 = 追加字符串(r80, ")");
  r82 = cn_var_缓冲区->数据;
  return r82;
  goto switch_merge_968;

  if_then_999:
  r71 = cn_var_缓冲区;
  r72 = 追加字符串(r71, ", ");
  goto if_merge_1000;

  if_merge_1000:
  r73 = cn_var_类型->参数类型列表;
  r74 = cn_var_i_0;
  r75 = (struct 类型信息*)cn_rt_array_get_element(r73, r74, 8);
  r76 = cn_var_缓冲区;
  r77 = 类型到C类型(r75, r76);
  goto for_update_997;

  case_default_982:
  return "void";
  goto switch_merge_968;

  switch_merge_968:
  return NULL;
}

char* 生成类型声明(struct 类型信息* cn_var_类型, const char* cn_var_名称, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r19, r20, r21, r26, r27, r28, r32, r33;
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
  struct 输出缓冲区* r30;
  struct 类型信息* r35;
  struct 输出缓冲区* r36;
  struct 输出缓冲区* r38;
  struct 输出缓冲区* r40;
  _Bool r6;
  _Bool r9;
  _Bool r15;
  _Bool r18;
  _Bool r23;
  _Bool r29;
  _Bool r31;
  _Bool r39;
  _Bool r42;

  entry:
  r0 = cn_var_类型;
  r1 = !r0;
  if (r1) goto if_then_1001; else goto if_merge_1002;

  if_then_1001:
  return "void";
  goto if_merge_1002;

  if_merge_1002:
  if (类型种类_类型_指针) goto if_then_1003; else goto if_merge_1004;

  if_then_1003:
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
  goto if_merge_1004;

  if_merge_1004:
  if (类型种类_类型_数组) goto if_then_1005; else goto if_merge_1006;

  if_then_1005:
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
  goto for_cond_1007;

  if_merge_1006:
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

  for_cond_1007:
  r19 = cn_var_i_0;
  r20 = cn_var_类型->数组维度;
  r21 = r19 < r20;
  if (r21) goto for_body_1008; else goto for_exit_1010;

  for_body_1008:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, "[");
  r24 = cn_var_缓冲区;
  r25 = cn_var_类型->维度大小;
  r26 = cn_var_i_0;
  r27 = *(long long)cn_rt_array_get_element(r25, r26, 8);
  r28 = 0 + r27;
  r29 = 追加整数(r24, r28);
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, "]");
  goto for_update_1009;

  for_update_1009:
  r32 = cn_var_i_0;
  r33 = r32 + 1;
  cn_var_i_0 = r33;
  goto for_cond_1007;

  for_exit_1010:
  r34 = cn_var_缓冲区->数据;
  return r34;
  goto if_merge_1006;
  return NULL;
}

char* 类型节点到C类型(struct 类型节点* cn_var_类型节点指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r12, r13, r15, r16, r18, r19, r21, r22, r24, r25, r27, r28, r45, r46;
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
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
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
  if (r1) goto if_then_1011; else goto if_merge_1012;

  if_then_1011:
  return "void";
  goto if_merge_1012;

  if_merge_1012:
  r2 = cn_var_类型节点指针->类型;
  r3 = r2 == 节点类型_基础类型;
  if (r3) goto case_body_1014; else goto switch_check_1023;

  switch_check_1023:
  r4 = r2 == 节点类型_指针类型;
  if (r4) goto case_body_1015; else goto switch_check_1024;

  switch_check_1024:
  r5 = r2 == 节点类型_数组类型;
  if (r5) goto case_body_1016; else goto switch_check_1025;

  switch_check_1025:
  r6 = r2 == 节点类型_结构体类型;
  if (r6) goto case_body_1017; else goto switch_check_1026;

  switch_check_1026:
  r7 = r2 == 节点类型_枚举类型;
  if (r7) goto case_body_1018; else goto switch_check_1027;

  switch_check_1027:
  r8 = r2 == 节点类型_类类型;
  if (r8) goto case_body_1019; else goto switch_check_1028;

  switch_check_1028:
  r9 = r2 == 节点类型_接口类型;
  if (r9) goto case_body_1020; else goto switch_check_1029;

  switch_check_1029:
  r10 = r2 == 节点类型_函数类型;
  if (r10) goto case_body_1021; else goto case_default_1022;

  case_body_1014:
  r11 = cn_var_类型节点指针->名称;
  r12 = 比较字符串(r11, "整数");
  r13 = r12 == 0;
  if (r13) goto if_then_1030; else goto if_merge_1031;

  if_then_1030:
  return "long long";
  goto if_merge_1031;

  if_merge_1031:
  r14 = cn_var_类型节点指针->名称;
  r15 = 比较字符串(r14, "小数");
  r16 = r15 == 0;
  if (r16) goto if_then_1032; else goto if_merge_1033;

  if_then_1032:
  return "double";
  goto if_merge_1033;

  if_merge_1033:
  r17 = cn_var_类型节点指针->名称;
  r18 = 比较字符串(r17, "布尔");
  r19 = r18 == 0;
  if (r19) goto if_then_1034; else goto if_merge_1035;

  if_then_1034:
  return "_Bool";
  goto if_merge_1035;

  if_merge_1035:
  r20 = cn_var_类型节点指针->名称;
  r21 = 比较字符串(r20, "字符串");
  r22 = r21 == 0;
  if (r22) goto if_then_1036; else goto if_merge_1037;

  if_then_1036:
  return "cn_string_t";
  goto if_merge_1037;

  if_merge_1037:
  r23 = cn_var_类型节点指针->名称;
  r24 = 比较字符串(r23, "字符");
  r25 = r24 == 0;
  if (r25) goto if_then_1038; else goto if_merge_1039;

  if_then_1038:
  return "char";
  goto if_merge_1039;

  if_merge_1039:
  r26 = cn_var_类型节点指针->名称;
  r27 = 比较字符串(r26, "空类型");
  r28 = r27 == 0;
  if (r28) goto if_then_1040; else goto if_merge_1041;

  if_then_1040:
  return "void";
  goto if_merge_1041;

  if_merge_1041:
  r29 = cn_var_缓冲区;
  r30 = cn_var_类型节点指针->名称;
  r31 = 追加字符串(r29, r30);
  r32 = cn_var_缓冲区->数据;
  return r32;
  goto switch_merge_1013;

  case_body_1015:
  r33 = cn_var_类型节点指针->元素类型;
  r34 = cn_var_缓冲区;
  r35 = 类型节点到C类型(r33, r34);
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, "*");
  r38 = cn_var_缓冲区->数据;
  return r38;
  goto switch_merge_1013;

  case_body_1016:
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
  goto switch_merge_1013;

  case_body_1017:
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, "struct ");
  r53 = cn_var_缓冲区;
  r54 = cn_var_类型节点指针->名称;
  r55 = 追加字符串(r53, r54);
  r56 = cn_var_缓冲区->数据;
  return r56;
  goto switch_merge_1013;

  case_body_1018:
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, "enum ");
  r59 = cn_var_缓冲区;
  r60 = cn_var_类型节点指针->名称;
  r61 = 追加字符串(r59, r60);
  r62 = cn_var_缓冲区->数据;
  return r62;
  goto switch_merge_1013;

  case_body_1019:
  r63 = cn_var_缓冲区;
  r64 = 追加字符串(r63, "struct ");
  r65 = cn_var_缓冲区;
  r66 = cn_var_类型节点指针->名称;
  r67 = 追加字符串(r65, r66);
  r68 = cn_var_缓冲区->数据;
  return r68;
  goto switch_merge_1013;

  case_body_1020:
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, "struct ");
  r71 = cn_var_缓冲区;
  r72 = cn_var_类型节点指针->名称;
  r73 = 追加字符串(r71, r72);
  r74 = cn_var_缓冲区->数据;
  return r74;
  goto switch_merge_1013;

  case_body_1021:
  r75 = cn_var_缓冲区;
  r76 = 追加字符串(r75, "void*");
  r77 = cn_var_缓冲区->数据;
  return r77;
  goto switch_merge_1013;

  case_default_1022:
  r78 = cn_var_类型节点指针->名称;
  if (r78) goto if_then_1042; else goto if_else_1043;

  if_then_1042:
  r79 = cn_var_缓冲区;
  r80 = cn_var_类型节点指针->名称;
  r81 = 追加字符串(r79, r80);
  goto if_merge_1044;

  if_else_1043:
  return "void";
  goto if_merge_1044;

  if_merge_1044:
  r82 = cn_var_缓冲区->数据;
  return r82;
  goto switch_merge_1013;

  switch_merge_1013:
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
  if (r1) goto case_body_1046; else goto switch_check_1053;

  switch_check_1053:
  r2 = r0 == IR操作数种类_整数常量;
  if (r2) goto case_body_1047; else goto switch_check_1054;

  switch_check_1054:
  r3 = r0 == IR操作数种类_小数常量;
  if (r3) goto case_body_1048; else goto switch_check_1055;

  switch_check_1055:
  r4 = r0 == IR操作数种类_字符串常量;
  if (r4) goto case_body_1049; else goto switch_check_1056;

  switch_check_1056:
  r5 = r0 == IR操作数种类_全局符号;
  if (r5) goto case_body_1050; else goto switch_check_1057;

  switch_check_1057:
  r6 = r0 == IR操作数种类_基本块标签;
  if (r6) goto case_body_1051; else goto case_default_1052;

  case_body_1046:
  r7 = cn_var_缓冲区;
  r8 = 追加字符串(r7, "r");
  r9 = cn_var_缓冲区;
  r10 = cn_var_操作数.寄存器编号;
  r11 = 追加整数(r9, r10);
  r12 = cn_var_缓冲区->数据;
  return r12;
  goto switch_merge_1045;

  case_body_1047:
  r13 = cn_var_缓冲区;
  r14 = cn_var_操作数.整数值;
  r15 = 追加整数(r13, r14);
  r16 = cn_var_缓冲区->数据;
  return r16;
  goto switch_merge_1045;

  case_body_1048:
  r17 = cn_var_缓冲区;
  r18 = cn_var_操作数.小数值;
  追加小数(r17, r18);
  r19 = cn_var_缓冲区->数据;
  return r19;
  goto switch_merge_1045;

  case_body_1049:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, "_str_");
  r22 = cn_var_缓冲区;
  r23 = cn_var_操作数.寄存器编号;
  r24 = 追加整数(r22, r23);
  r25 = cn_var_缓冲区->数据;
  return r25;
  goto switch_merge_1045;

  case_body_1050:
  r26 = cn_var_缓冲区;
  r27 = cn_var_操作数.符号名;
  r28 = 追加字符串(r26, r27);
  r29 = cn_var_缓冲区->数据;
  return r29;
  goto switch_merge_1045;

  case_body_1051:
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, "L");
  r32 = cn_var_缓冲区;
  r33 = cn_var_操作数.目标块;
  r34 = r33->块编号;
  r35 = 追加整数(r32, r34);
  r36 = cn_var_缓冲区->数据;
  return r36;
  goto switch_merge_1045;

  case_default_1052:
  return "";
  goto switch_merge_1045;

  switch_merge_1045:
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
  if (r10) goto case_body_1059; else goto switch_check_1070;

  switch_check_1070:
  r11 = r9 == IR操作码_减法指令;
  if (r11) goto case_body_1060; else goto switch_check_1071;

  switch_check_1071:
  r12 = r9 == IR操作码_乘法指令;
  if (r12) goto case_body_1061; else goto switch_check_1072;

  switch_check_1072:
  r13 = r9 == IR操作码_除法指令;
  if (r13) goto case_body_1062; else goto switch_check_1073;

  switch_check_1073:
  r14 = r9 == IR操作码_取模指令;
  if (r14) goto case_body_1063; else goto switch_check_1074;

  switch_check_1074:
  r15 = r9 == IR操作码_与指令;
  if (r15) goto case_body_1064; else goto switch_check_1075;

  switch_check_1075:
  r16 = r9 == IR操作码_或指令;
  if (r16) goto case_body_1065; else goto switch_check_1076;

  switch_check_1076:
  r17 = r9 == IR操作码_异或指令;
  if (r17) goto case_body_1066; else goto switch_check_1077;

  switch_check_1077:
  r18 = r9 == IR操作码_左移指令;
  if (r18) goto case_body_1067; else goto switch_check_1078;

  switch_check_1078:
  r19 = r9 == IR操作码_右移指令;
  if (r19) goto case_body_1068; else goto case_default_1069;

  case_body_1059:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, " + ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1060:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, " - ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1061:
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, " * ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1062:
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " / ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1063:
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, " % ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1064:
  r30 = cn_var_缓冲区;
  r31 = 追加字符串(r30, " & ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1065:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, " | ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1066:
  r34 = cn_var_缓冲区;
  r35 = 追加字符串(r34, " ^ ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1067:
  r36 = cn_var_缓冲区;
  r37 = 追加字符串(r36, " << ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_body_1068:
  r38 = cn_var_缓冲区;
  r39 = 追加字符串(r38, " >> ");
  goto switch_merge_1058;
  goto switch_merge_1058;

  case_default_1069:
  r40 = cn_var_缓冲区;
  r41 = 追加字符串(r40, " ? ");
  goto switch_merge_1058;

  switch_merge_1058:
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
  if (r7) goto case_body_1080; else goto switch_check_1084;

  switch_check_1084:
  r8 = r6 == IR操作码_取反指令;
  if (r8) goto case_body_1081; else goto switch_check_1085;

  switch_check_1085:
  r9 = r6 == IR操作码_逻辑非指令;
  if (r9) goto case_body_1082; else goto case_default_1083;

  case_body_1080:
  r10 = cn_var_缓冲区;
  r11 = 追加字符串(r10, "-");
  goto switch_merge_1079;
  goto switch_merge_1079;

  case_body_1081:
  r12 = cn_var_缓冲区;
  r13 = 追加字符串(r12, "~");
  goto switch_merge_1079;
  goto switch_merge_1079;

  case_body_1082:
  r14 = cn_var_缓冲区;
  r15 = 追加字符串(r14, "!");
  goto switch_merge_1079;
  goto switch_merge_1079;

  case_default_1083:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, "");
  goto switch_merge_1079;

  switch_merge_1079:
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
  if (r10) goto case_body_1087; else goto switch_check_1094;

  switch_check_1094:
  r11 = r9 == IR操作码_不等指令;
  if (r11) goto case_body_1088; else goto switch_check_1095;

  switch_check_1095:
  r12 = r9 == IR操作码_小于指令;
  if (r12) goto case_body_1089; else goto switch_check_1096;

  switch_check_1096:
  r13 = r9 == IR操作码_小于等于指令;
  if (r13) goto case_body_1090; else goto switch_check_1097;

  switch_check_1097:
  r14 = r9 == IR操作码_大于指令;
  if (r14) goto case_body_1091; else goto switch_check_1098;

  switch_check_1098:
  r15 = r9 == IR操作码_大于等于指令;
  if (r15) goto case_body_1092; else goto case_default_1093;

  case_body_1087:
  r16 = cn_var_缓冲区;
  r17 = 追加字符串(r16, " == ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_body_1088:
  r18 = cn_var_缓冲区;
  r19 = 追加字符串(r18, " != ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_body_1089:
  r20 = cn_var_缓冲区;
  r21 = 追加字符串(r20, " < ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_body_1090:
  r22 = cn_var_缓冲区;
  r23 = 追加字符串(r22, " <= ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_body_1091:
  r24 = cn_var_缓冲区;
  r25 = 追加字符串(r24, " > ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_body_1092:
  r26 = cn_var_缓冲区;
  r27 = 追加字符串(r26, " >= ");
  goto switch_merge_1086;
  goto switch_merge_1086;

  case_default_1093:
  r28 = cn_var_缓冲区;
  r29 = 追加字符串(r28, " ? ");
  goto switch_merge_1086;

  switch_merge_1086:
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
  long long r16, r17, r24;
  char* r10;
  char* r28;
  char* r33;
  char* r38;
  char* r45;
  char* r50;
  char* r59;
  char* r64;
  char* r71;
  char* r76;
  char* r83;
  char* r88;
  char* r95;
  char* r100;
  struct 输出缓冲区* r7;
  struct 输出缓冲区* r9;
  struct 输出缓冲区* r11;
  struct 输出缓冲区* r13;
  struct 类型节点* r15;
  struct 输出缓冲区* r19;
  struct 输出缓冲区* r21;
  struct 输出缓冲区* r25;
  struct 输出缓冲区* r27;
  struct 输出缓冲区* r29;
  struct 输出缓冲区* r32;
  struct 输出缓冲区* r34;
  struct 输出缓冲区* r37;
  struct 输出缓冲区* r39;
  struct 输出缓冲区* r41;
  struct 输出缓冲区* r42;
  struct 输出缓冲区* r44;
  struct 输出缓冲区* r46;
  struct 输出缓冲区* r49;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r67;
  struct 输出缓冲区* r68;
  struct 输出缓冲区* r70;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r75;
  struct 输出缓冲区* r77;
  struct 输出缓冲区* r79;
  struct 输出缓冲区* r80;
  struct 输出缓冲区* r82;
  struct 输出缓冲区* r84;
  struct 输出缓冲区* r87;
  struct 输出缓冲区* r89;
  struct 输出缓冲区* r91;
  struct 输出缓冲区* r92;
  struct 输出缓冲区* r94;
  struct 输出缓冲区* r96;
  struct 输出缓冲区* r99;
  struct 输出缓冲区* r101;
  struct 输出缓冲区* r103;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r12;
  _Bool r18;
  _Bool r20;
  _Bool r30;
  _Bool r35;
  _Bool r40;
  _Bool r47;
  _Bool r52;
  _Bool r56;
  _Bool r61;
  _Bool r66;
  _Bool r73;
  _Bool r78;
  _Bool r85;
  _Bool r90;
  _Bool r97;
  _Bool r102;
  struct IR操作数 r8;
  struct IR操作数 r14;
  struct IR操作数 r22;
  struct IR操作数 r26;
  struct IR操作数 r31;
  struct IR操作数 r36;
  struct IR操作数 r43;
  struct IR操作数 r48;
  struct IR操作数 r57;
  struct IR操作数 r62;
  struct IR操作数 r69;
  struct IR操作数 r74;
  struct IR操作数 r81;
  struct IR操作数 r86;
  struct IR操作数 r93;
  struct IR操作数 r98;
  enum IR操作码 r0;
  enum IR操作数种类 r23;

  entry:
  r0 = cn_var_指令->操作码;
  r1 = r0 == IR操作码_分配指令;
  if (r1) goto case_body_1100; else goto switch_check_1107;

  switch_check_1107:
  r2 = r0 == IR操作码_加载指令;
  if (r2) goto case_body_1101; else goto switch_check_1108;

  switch_check_1108:
  r3 = r0 == IR操作码_存储指令;
  if (r3) goto case_body_1102; else goto switch_check_1109;

  switch_check_1109:
  r4 = r0 == IR操作码_移动指令;
  if (r4) goto case_body_1103; else goto switch_check_1110;

  switch_check_1110:
  r5 = r0 == IR操作码_取地址指令;
  if (r5) goto case_body_1104; else goto switch_check_1111;

  switch_check_1111:
  r6 = r0 == IR操作码_解引用指令;
  if (r6) goto case_body_1105; else goto case_default_1106;

  case_body_1100:
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
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_body_1101:
  r22 = cn_var_指令->源操作数2;
  r23 = r22.种类;
  r24 = r23 != IR操作数种类_无操作数;
  if (r24) goto if_then_1112; else goto if_else_1113;

  if_then_1112:
  r25 = cn_var_缓冲区;
  追加缩进(r25, 4);
  r26 = cn_var_指令->目标;
  r27 = cn_var_缓冲区;
  r28 = 生成操作数代码(r26, r27);
  r29 = cn_var_缓冲区;
  r30 = 追加字符串(r29, " = ");
  r31 = cn_var_指令->源操作数1;
  r32 = cn_var_缓冲区;
  r33 = 生成操作数代码(r31, r32);
  r34 = cn_var_缓冲区;
  r35 = 追加字符串(r34, "[");
  r36 = cn_var_指令->源操作数2;
  r37 = cn_var_缓冲区;
  r38 = 生成操作数代码(r36, r37);
  r39 = cn_var_缓冲区;
  r40 = 追加字符串(r39, "];");
  r41 = cn_var_缓冲区;
  追加换行(r41);
  goto if_merge_1114;

  if_else_1113:
  r42 = cn_var_缓冲区;
  追加缩进(r42, 4);
  r43 = cn_var_指令->目标;
  r44 = cn_var_缓冲区;
  r45 = 生成操作数代码(r43, r44);
  r46 = cn_var_缓冲区;
  r47 = 追加字符串(r46, " = *");
  r48 = cn_var_指令->源操作数1;
  r49 = cn_var_缓冲区;
  r50 = 生成操作数代码(r48, r49);
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, ";");
  r53 = cn_var_缓冲区;
  追加换行(r53);
  goto if_merge_1114;

  if_merge_1114:
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_body_1102:
  r54 = cn_var_缓冲区;
  追加缩进(r54, 4);
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, "*");
  r57 = cn_var_指令->源操作数2;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, " = ");
  r62 = cn_var_指令->源操作数1;
  r63 = cn_var_缓冲区;
  r64 = 生成操作数代码(r62, r63);
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, ";");
  r67 = cn_var_缓冲区;
  追加换行(r67);
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_body_1103:
  r68 = cn_var_缓冲区;
  追加缩进(r68, 4);
  r69 = cn_var_指令->目标;
  r70 = cn_var_缓冲区;
  r71 = 生成操作数代码(r69, r70);
  r72 = cn_var_缓冲区;
  r73 = 追加字符串(r72, " = ");
  r74 = cn_var_指令->源操作数1;
  r75 = cn_var_缓冲区;
  r76 = 生成操作数代码(r74, r75);
  r77 = cn_var_缓冲区;
  r78 = 追加字符串(r77, ";");
  r79 = cn_var_缓冲区;
  追加换行(r79);
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_body_1104:
  r80 = cn_var_缓冲区;
  追加缩进(r80, 4);
  r81 = cn_var_指令->目标;
  r82 = cn_var_缓冲区;
  r83 = 生成操作数代码(r81, r82);
  r84 = cn_var_缓冲区;
  r85 = 追加字符串(r84, " = &");
  r86 = cn_var_指令->源操作数1;
  r87 = cn_var_缓冲区;
  r88 = 生成操作数代码(r86, r87);
  r89 = cn_var_缓冲区;
  r90 = 追加字符串(r89, ";");
  r91 = cn_var_缓冲区;
  追加换行(r91);
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_body_1105:
  r92 = cn_var_缓冲区;
  追加缩进(r92, 4);
  r93 = cn_var_指令->目标;
  r94 = cn_var_缓冲区;
  r95 = 生成操作数代码(r93, r94);
  r96 = cn_var_缓冲区;
  r97 = 追加字符串(r96, " = *");
  r98 = cn_var_指令->源操作数1;
  r99 = cn_var_缓冲区;
  r100 = 生成操作数代码(r98, r99);
  r101 = cn_var_缓冲区;
  r102 = 追加字符串(r101, ";");
  r103 = cn_var_缓冲区;
  追加换行(r103);
  goto switch_merge_1099;
  goto switch_merge_1099;

  case_default_1106:
  goto switch_merge_1099;
  goto switch_merge_1099;

  switch_merge_1099:
  return;
}

void 生成控制流指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r11, r22, r38, r45, r53, r64, r65, r66, r67, r68, r72, r76, r77, r86;
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
  void* r73;
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
  _Bool r58;
  _Bool r63;
  _Bool r70;
  _Bool r79;
  _Bool r83;
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
  if (r1) goto case_body_1116; else goto switch_check_1122;

  switch_check_1122:
  r2 = r0 == IR操作码_跳转指令;
  if (r2) goto case_body_1117; else goto switch_check_1123;

  switch_check_1123:
  r3 = r0 == IR操作码_条件跳转指令;
  if (r3) goto case_body_1118; else goto switch_check_1124;

  switch_check_1124:
  r4 = r0 == IR操作码_调用指令;
  if (r4) goto case_body_1119; else goto switch_check_1125;

  switch_check_1125:
  r5 = r0 == IR操作码_返回指令;
  if (r5) goto case_body_1120; else goto case_default_1121;

  case_body_1116:
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
  goto switch_merge_1115;
  goto switch_merge_1115;

  case_body_1117:
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
  goto switch_merge_1115;
  goto switch_merge_1115;

  case_body_1118:
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
  goto switch_merge_1115;
  goto switch_merge_1115;

  case_body_1119:
  r50 = cn_var_缓冲区;
  追加缩进(r50, 4);
  r51 = cn_var_指令->目标;
  r52 = r51.种类;
  r53 = r52 != IR操作数种类_无操作数;
  if (r53) goto if_then_1126; else goto if_merge_1127;

  if_then_1126:
  r54 = cn_var_指令->目标;
  r55 = cn_var_缓冲区;
  r56 = 生成操作数代码(r54, r55);
  r57 = cn_var_缓冲区;
  r58 = 追加字符串(r57, " = ");
  goto if_merge_1127;

  if_merge_1127:
  r59 = cn_var_指令->源操作数1;
  r60 = cn_var_缓冲区;
  r61 = 生成操作数代码(r59, r60);
  r62 = cn_var_缓冲区;
  r63 = 追加字符串(r62, "(");
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1128;

  for_cond_1128:
  r64 = cn_var_i_0;
  r65 = cn_var_指令->额外参数数量;
  r66 = r64 < r65;
  if (r66) goto for_body_1129; else goto for_exit_1131;

  for_body_1129:
  r67 = cn_var_i_0;
  r68 = r67 > 0;
  if (r68) goto if_then_1132; else goto if_merge_1133;

  for_update_1130:
  r76 = cn_var_i_0;
  r77 = r76 + 1;
  cn_var_i_0 = r77;
  goto for_cond_1128;

  for_exit_1131:
  r78 = cn_var_缓冲区;
  r79 = 追加字符串(r78, ");");
  r80 = cn_var_缓冲区;
  追加换行(r80);
  goto switch_merge_1115;
  goto switch_merge_1115;

  if_then_1132:
  r69 = cn_var_缓冲区;
  r70 = 追加字符串(r69, ", ");
  goto if_merge_1133;

  if_merge_1133:
  r71 = cn_var_指令->额外参数;
  r72 = cn_var_i_0;
  r73 = (void*)cn_rt_array_get_element(r71, r72, 8);
  r74 = cn_var_缓冲区;
  r75 = 生成操作数代码(r73, r74);
  goto for_update_1130;

  case_body_1120:
  r81 = cn_var_缓冲区;
  追加缩进(r81, 4);
  r82 = cn_var_缓冲区;
  r83 = 追加字符串(r82, "return");
  r84 = cn_var_指令->目标;
  r85 = r84.种类;
  r86 = r85 != IR操作数种类_无操作数;
  if (r86) goto if_then_1134; else goto if_merge_1135;

  if_then_1134:
  r87 = cn_var_缓冲区;
  r88 = 追加字符串(r87, " ");
  r89 = cn_var_指令->目标;
  r90 = cn_var_缓冲区;
  r91 = 生成操作数代码(r89, r90);
  goto if_merge_1135;

  if_merge_1135:
  r92 = cn_var_缓冲区;
  r93 = 追加字符串(r92, ";");
  r94 = cn_var_缓冲区;
  追加换行(r94);
  goto switch_merge_1115;
  goto switch_merge_1115;

  case_default_1121:
  goto switch_merge_1115;
  goto switch_merge_1115;

  switch_merge_1115:
  return;
}

void 生成结构体指令(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r33, r36, r37, r40, r41;
  char* r7;
  char* r12;
  char* r17;
  char* r25;
  char* r30;
  char* r49;
  char* r59;
  char* r64;
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
  struct 类型节点* r32;
  struct 类型节点* r35;
  struct 类型节点* r39;
  struct 输出缓冲区* r43;
  struct 输出缓冲区* r45;
  struct 输出缓冲区* r47;
  struct 输出缓冲区* r51;
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r54;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r67;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r9;
  _Bool r14;
  _Bool r18;
  _Bool r20;
  _Bool r27;
  _Bool r42;
  _Bool r44;
  _Bool r46;
  _Bool r50;
  _Bool r52;
  _Bool r56;
  _Bool r61;
  _Bool r66;
  struct IR操作数 r5;
  struct IR操作数 r10;
  struct IR操作数 r16;
  struct IR操作数 r23;
  struct IR操作数 r28;
  struct IR操作数 r31;
  struct IR操作数 r34;
  struct IR操作数 r38;
  struct IR操作数 r48;
  struct IR操作数 r57;
  struct IR操作数 r62;
  enum IR操作码 r0;

  entry:
  r0 = cn_var_指令->操作码;
  r1 = r0 == IR操作码_成员指针指令;
  if (r1) goto case_body_1137; else goto switch_check_1141;

  switch_check_1141:
  r2 = r0 == IR操作码_成员访问指令;
  if (r2) goto case_body_1138; else goto switch_check_1142;

  switch_check_1142:
  r3 = r0 == IR操作码_结构体初始化指令;
  if (r3) goto case_body_1139; else goto case_default_1140;

  case_body_1137:
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
  goto switch_merge_1136;
  goto switch_merge_1136;

  case_body_1138:
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
  _Bool cn_var_是指针_0;
  cn_var_是指针_0 = 0;
  r31 = cn_var_指令->源操作数1;
  r32 = r31.类型信息;
  r33 = r32 != 0;
  if (r33) goto if_then_1143; else goto if_merge_1144;

  if_then_1143:
  r34 = cn_var_指令->源操作数1;
  r35 = r34.类型信息;
  r36 = r35->种类;
  r37 = r36 == 类型种类_类型_指针;
  if (r37) goto if_then_1145; else goto if_merge_1146;

  if_merge_1144:
  r42 = cn_var_是指针_0;
  if (r42) goto if_then_1149; else goto if_else_1150;

  if_then_1145:
  cn_var_是指针_0 = 1;
  goto if_merge_1146;

  if_merge_1146:
  r38 = cn_var_指令->源操作数1;
  r39 = r38.类型信息;
  r40 = r39->指针层级;
  r41 = r40 > 0;
  if (r41) goto if_then_1147; else goto if_merge_1148;

  if_then_1147:
  cn_var_是指针_0 = 1;
  goto if_merge_1148;

  if_merge_1148:
  goto if_merge_1144;

  if_then_1149:
  r43 = cn_var_缓冲区;
  r44 = 追加字符串(r43, "->");
  goto if_merge_1151;

  if_else_1150:
  r45 = cn_var_缓冲区;
  r46 = 追加字符串(r45, ".");
  goto if_merge_1151;

  if_merge_1151:
  r47 = cn_var_缓冲区;
  r48 = cn_var_指令->源操作数2;
  r49 = r48.符号名;
  r50 = 追加字符串(r47, r49);
  r51 = cn_var_缓冲区;
  r52 = 追加字符串(r51, ";");
  r53 = cn_var_缓冲区;
  追加换行(r53);
  goto switch_merge_1136;
  goto switch_merge_1136;

  case_body_1139:
  r54 = cn_var_缓冲区;
  追加缩进(r54, 4);
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, "memset(&");
  r57 = cn_var_指令->目标;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, ", 0, sizeof(");
  r62 = cn_var_指令->目标;
  r63 = cn_var_缓冲区;
  r64 = 生成操作数代码(r62, r63);
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, "));");
  r67 = cn_var_缓冲区;
  追加换行(r67);
  goto switch_merge_1136;
  goto switch_merge_1136;

  case_default_1140:
  goto switch_merge_1136;
  goto switch_merge_1136;

  switch_merge_1136:
  return;
}

void 生成指令代码(struct IR指令* cn_var_指令, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r81;
  char* r54;
  char* r59;
  char* r64;
  char* r69;
  char* r76;
  char* r85;
  char* r92;
  struct IR指令* r0;
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
  struct 输出缓冲区* r53;
  struct 输出缓冲区* r55;
  struct 输出缓冲区* r58;
  struct 输出缓冲区* r60;
  struct 输出缓冲区* r63;
  struct 输出缓冲区* r65;
  struct 输出缓冲区* r68;
  struct 输出缓冲区* r70;
  struct 输出缓冲区* r72;
  struct 输出缓冲区* r73;
  struct 输出缓冲区* r75;
  struct 输出缓冲区* r77;
  struct 类型节点* r80;
  struct 类型节点* r83;
  struct 输出缓冲区* r84;
  struct 输出缓冲区* r86;
  struct 输出缓冲区* r88;
  struct 输出缓冲区* r91;
  struct 输出缓冲区* r93;
  struct 输出缓冲区* r95;
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
  _Bool r38;
  _Bool r56;
  _Bool r61;
  _Bool r66;
  _Bool r71;
  _Bool r78;
  _Bool r87;
  _Bool r89;
  _Bool r94;
  struct IR操作数 r52;
  struct IR操作数 r57;
  struct IR操作数 r62;
  struct IR操作数 r67;
  struct IR操作数 r74;
  struct IR操作数 r79;
  struct IR操作数 r82;
  struct IR操作数 r90;
  enum IR操作码 r2;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_1152; else goto if_merge_1153;

  if_then_1152:
  return;
  goto if_merge_1153;

  if_merge_1153:
  r2 = cn_var_指令->操作码;
  r3 = r2 == IR操作码_加法指令;
  if (r3) goto case_body_1155; else goto switch_check_1192;

  switch_check_1192:
  r4 = r2 == IR操作码_减法指令;
  if (r4) goto case_body_1156; else goto switch_check_1193;

  switch_check_1193:
  r5 = r2 == IR操作码_乘法指令;
  if (r5) goto case_body_1157; else goto switch_check_1194;

  switch_check_1194:
  r6 = r2 == IR操作码_除法指令;
  if (r6) goto case_body_1158; else goto switch_check_1195;

  switch_check_1195:
  r7 = r2 == IR操作码_取模指令;
  if (r7) goto case_body_1159; else goto switch_check_1196;

  switch_check_1196:
  r8 = r2 == IR操作码_与指令;
  if (r8) goto case_body_1160; else goto switch_check_1197;

  switch_check_1197:
  r9 = r2 == IR操作码_或指令;
  if (r9) goto case_body_1161; else goto switch_check_1198;

  switch_check_1198:
  r10 = r2 == IR操作码_异或指令;
  if (r10) goto case_body_1162; else goto switch_check_1199;

  switch_check_1199:
  r11 = r2 == IR操作码_左移指令;
  if (r11) goto case_body_1163; else goto switch_check_1200;

  switch_check_1200:
  r12 = r2 == IR操作码_右移指令;
  if (r12) goto case_body_1164; else goto switch_check_1201;

  switch_check_1201:
  r13 = r2 == IR操作码_负号指令;
  if (r13) goto case_body_1165; else goto switch_check_1202;

  switch_check_1202:
  r14 = r2 == IR操作码_取反指令;
  if (r14) goto case_body_1166; else goto switch_check_1203;

  switch_check_1203:
  r15 = r2 == IR操作码_逻辑非指令;
  if (r15) goto case_body_1167; else goto switch_check_1204;

  switch_check_1204:
  r16 = r2 == IR操作码_相等指令;
  if (r16) goto case_body_1168; else goto switch_check_1205;

  switch_check_1205:
  r17 = r2 == IR操作码_不等指令;
  if (r17) goto case_body_1169; else goto switch_check_1206;

  switch_check_1206:
  r18 = r2 == IR操作码_小于指令;
  if (r18) goto case_body_1170; else goto switch_check_1207;

  switch_check_1207:
  r19 = r2 == IR操作码_小于等于指令;
  if (r19) goto case_body_1171; else goto switch_check_1208;

  switch_check_1208:
  r20 = r2 == IR操作码_大于指令;
  if (r20) goto case_body_1172; else goto switch_check_1209;

  switch_check_1209:
  r21 = r2 == IR操作码_大于等于指令;
  if (r21) goto case_body_1173; else goto switch_check_1210;

  switch_check_1210:
  r22 = r2 == IR操作码_分配指令;
  if (r22) goto case_body_1174; else goto switch_check_1211;

  switch_check_1211:
  r23 = r2 == IR操作码_加载指令;
  if (r23) goto case_body_1175; else goto switch_check_1212;

  switch_check_1212:
  r24 = r2 == IR操作码_存储指令;
  if (r24) goto case_body_1176; else goto switch_check_1213;

  switch_check_1213:
  r25 = r2 == IR操作码_移动指令;
  if (r25) goto case_body_1177; else goto switch_check_1214;

  switch_check_1214:
  r26 = r2 == IR操作码_取地址指令;
  if (r26) goto case_body_1178; else goto switch_check_1215;

  switch_check_1215:
  r27 = r2 == IR操作码_解引用指令;
  if (r27) goto case_body_1179; else goto switch_check_1216;

  switch_check_1216:
  r28 = r2 == IR操作码_标签指令;
  if (r28) goto case_body_1180; else goto switch_check_1217;

  switch_check_1217:
  r29 = r2 == IR操作码_跳转指令;
  if (r29) goto case_body_1181; else goto switch_check_1218;

  switch_check_1218:
  r30 = r2 == IR操作码_条件跳转指令;
  if (r30) goto case_body_1182; else goto switch_check_1219;

  switch_check_1219:
  r31 = r2 == IR操作码_调用指令;
  if (r31) goto case_body_1183; else goto switch_check_1220;

  switch_check_1220:
  r32 = r2 == IR操作码_返回指令;
  if (r32) goto case_body_1184; else goto switch_check_1221;

  switch_check_1221:
  r33 = r2 == IR操作码_成员指针指令;
  if (r33) goto case_body_1185; else goto switch_check_1222;

  switch_check_1222:
  r34 = r2 == IR操作码_成员访问指令;
  if (r34) goto case_body_1186; else goto switch_check_1223;

  switch_check_1223:
  r35 = r2 == IR操作码_结构体初始化指令;
  if (r35) goto case_body_1187; else goto switch_check_1224;

  switch_check_1224:
  r36 = r2 == IR操作码_PHI指令;
  if (r36) goto case_body_1188; else goto switch_check_1225;

  switch_check_1225:
  r37 = r2 == IR操作码_选择指令;
  if (r37) goto case_body_1189; else goto switch_check_1226;

  switch_check_1226:
  r38 = r2 == IR操作码_类型转换指令;
  if (r38) goto case_body_1190; else goto case_default_1191;

  case_body_1155:
  goto switch_merge_1154;

  case_body_1156:
  goto switch_merge_1154;

  case_body_1157:
  goto switch_merge_1154;

  case_body_1158:
  goto switch_merge_1154;

  case_body_1159:
  goto switch_merge_1154;

  case_body_1160:
  goto switch_merge_1154;

  case_body_1161:
  goto switch_merge_1154;

  case_body_1162:
  goto switch_merge_1154;

  case_body_1163:
  goto switch_merge_1154;

  case_body_1164:
  r39 = cn_var_指令;
  r40 = cn_var_缓冲区;
  生成算术指令(r39, r40);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1165:
  goto switch_merge_1154;

  case_body_1166:
  goto switch_merge_1154;

  case_body_1167:
  r41 = cn_var_指令;
  r42 = cn_var_缓冲区;
  生成一元指令(r41, r42);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1168:
  goto switch_merge_1154;

  case_body_1169:
  goto switch_merge_1154;

  case_body_1170:
  goto switch_merge_1154;

  case_body_1171:
  goto switch_merge_1154;

  case_body_1172:
  goto switch_merge_1154;

  case_body_1173:
  r43 = cn_var_指令;
  r44 = cn_var_缓冲区;
  生成比较指令(r43, r44);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1174:
  goto switch_merge_1154;

  case_body_1175:
  goto switch_merge_1154;

  case_body_1176:
  goto switch_merge_1154;

  case_body_1177:
  goto switch_merge_1154;

  case_body_1178:
  goto switch_merge_1154;

  case_body_1179:
  r45 = cn_var_指令;
  r46 = cn_var_缓冲区;
  生成内存指令(r45, r46);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1180:
  goto switch_merge_1154;

  case_body_1181:
  goto switch_merge_1154;

  case_body_1182:
  goto switch_merge_1154;

  case_body_1183:
  goto switch_merge_1154;

  case_body_1184:
  r47 = cn_var_指令;
  r48 = cn_var_缓冲区;
  生成控制流指令(r47, r48);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1185:
  goto switch_merge_1154;

  case_body_1186:
  goto switch_merge_1154;

  case_body_1187:
  r49 = cn_var_指令;
  r50 = cn_var_缓冲区;
  生成结构体指令(r49, r50);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1188:
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1189:
  r51 = cn_var_缓冲区;
  追加缩进(r51, 4);
  r52 = cn_var_指令->目标;
  r53 = cn_var_缓冲区;
  r54 = 生成操作数代码(r52, r53);
  r55 = cn_var_缓冲区;
  r56 = 追加字符串(r55, " = ");
  r57 = cn_var_指令->目标;
  r58 = cn_var_缓冲区;
  r59 = 生成操作数代码(r57, r58);
  r60 = cn_var_缓冲区;
  r61 = 追加字符串(r60, " ? ");
  r62 = cn_var_指令->源操作数1;
  r63 = cn_var_缓冲区;
  r64 = 生成操作数代码(r62, r63);
  r65 = cn_var_缓冲区;
  r66 = 追加字符串(r65, " : ");
  r67 = cn_var_指令->源操作数2;
  r68 = cn_var_缓冲区;
  r69 = 生成操作数代码(r67, r68);
  r70 = cn_var_缓冲区;
  r71 = 追加字符串(r70, ";");
  r72 = cn_var_缓冲区;
  追加换行(r72);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_body_1190:
  r73 = cn_var_缓冲区;
  追加缩进(r73, 4);
  r74 = cn_var_指令->目标;
  r75 = cn_var_缓冲区;
  r76 = 生成操作数代码(r74, r75);
  r77 = cn_var_缓冲区;
  r78 = 追加字符串(r77, " = (");
  r79 = cn_var_指令->目标;
  r80 = r79.类型信息;
  r81 = r80 != 0;
  if (r81) goto if_then_1227; else goto if_else_1228;

  if_then_1227:
  r82 = cn_var_指令->目标;
  r83 = r82.类型信息;
  r84 = cn_var_缓冲区;
  r85 = 类型节点到C类型(r83, r84);
  goto if_merge_1229;

  if_else_1228:
  r86 = cn_var_缓冲区;
  r87 = 追加字符串(r86, "void*");
  goto if_merge_1229;

  if_merge_1229:
  r88 = cn_var_缓冲区;
  r89 = 追加字符串(r88, ")");
  r90 = cn_var_指令->源操作数1;
  r91 = cn_var_缓冲区;
  r92 = 生成操作数代码(r90, r91);
  r93 = cn_var_缓冲区;
  r94 = 追加字符串(r93, ";");
  r95 = cn_var_缓冲区;
  追加换行(r95);
  goto switch_merge_1154;
  goto switch_merge_1154;

  case_default_1191:
  goto switch_merge_1154;
  goto switch_merge_1154;

  switch_merge_1154:
  return;
}

void 生成基本块代码(struct 基本块* cn_var_块, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r5;
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
  _Bool r3;
  _Bool r6;
  _Bool r8;

  entry:
  r0 = cn_var_块;
  r1 = !r0;
  if (r1) goto if_then_1230; else goto if_merge_1231;

  if_then_1230:
  return;
  goto if_merge_1231;

  if_merge_1231:
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
  goto while_cond_1232;

  while_cond_1232:
  r11 = cn_var_指令_0;
  if (r11) goto while_body_1233; else goto while_exit_1234;

  while_body_1233:
  r12 = cn_var_指令_0;
  r13 = cn_var_缓冲区;
  生成指令代码(r12, r13);
  r14 = cn_var_指令_0->下一条;
  cn_var_指令_0 = r14;
  goto while_cond_1232;

  while_exit_1234:
  return;
}

void 生成局部变量声明(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r2, r3, r4, r6, r15, r20, r21;
  char* r11;
  struct IR函数* r0;
  struct IR操作数* r5;
  void* r7;
  struct 输出缓冲区* r8;
  struct 类型节点* r9;
  struct 输出缓冲区* r10;
  struct 输出缓冲区* r12;
  struct 输出缓冲区* r14;
  struct 输出缓冲区* r17;
  struct 输出缓冲区* r19;
  _Bool r13;
  _Bool r16;
  _Bool r18;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_1235; else goto if_merge_1236;

  if_then_1235:
  return;
  goto if_merge_1236;

  if_merge_1236:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1237;

  for_cond_1237:
  r2 = cn_var_i_0;
  r3 = cn_var_函数指针->局部变量数量;
  r4 = r2 < r3;
  if (r4) goto for_body_1238; else goto for_exit_1240;

  for_body_1238:
  struct IR操作数 cn_var_局部变量_1;
  r5 = cn_var_函数指针->局部变量;
  r6 = cn_var_i_0;
  r7 = (void*)cn_rt_array_get_element(r5, r6, 8);
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
  goto for_update_1239;

  for_update_1239:
  r20 = cn_var_i_0;
  r21 = r20 + 1;
  cn_var_i_0 = r21;
  goto for_cond_1237;

  for_exit_1240:
  return;
}

void 生成函数代码(struct IR函数* cn_var_函数指针, struct 输出缓冲区* cn_var_缓冲区) {
  long long r1, r12, r13, r14, r15, r16, r20, r22, r28, r30, r31;
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
  void* r21;
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
  _Bool r6;
  _Bool r9;
  _Bool r11;
  _Bool r18;
  _Bool r26;
  _Bool r29;
  _Bool r33;
  _Bool r35;
  _Bool r46;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_1241; else goto if_merge_1242;

  if_then_1241:
  return;
  goto if_merge_1242;

  if_merge_1242:
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
  goto for_cond_1243;

  for_cond_1243:
  r12 = cn_var_i_0;
  r13 = cn_var_函数指针->参数数量;
  r14 = r12 < r13;
  if (r14) goto for_body_1244; else goto for_exit_1246;

  for_body_1244:
  r15 = cn_var_i_0;
  r16 = r15 > 0;
  if (r16) goto if_then_1247; else goto if_merge_1248;

  for_update_1245:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_1243;

  for_exit_1246:
  if (0) goto if_then_1249; else goto if_merge_1250;

  if_then_1247:
  r17 = cn_var_缓冲区;
  r18 = 追加字符串(r17, ", ");
  goto if_merge_1248;

  if_merge_1248:
  struct IR操作数 cn_var_参数_1;
  r19 = cn_var_函数指针->参数列表;
  r20 = cn_var_i_0;
  r21 = (void*)cn_rt_array_get_element(r19, r20, 8);
  cn_var_参数_1 = r21;
  r22 = cn_var_参数_1.类型信息;
  r23 = cn_var_缓冲区;
  r24 = 类型节点到C类型(r22, r23);
  r25 = cn_var_缓冲区;
  r26 = 追加字符串(r25, " p");
  r27 = cn_var_缓冲区;
  r28 = cn_var_i_0;
  r29 = 追加整数(r27, r28);
  goto for_update_1245;

  if_then_1249:
  r32 = cn_var_缓冲区;
  r33 = 追加字符串(r32, "void");
  goto if_merge_1250;

  if_merge_1250:
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
  goto while_cond_1251;

  while_cond_1251:
  r41 = cn_var_块_2;
  if (r41) goto while_body_1252; else goto while_exit_1253;

  while_body_1252:
  r42 = cn_var_块_2;
  r43 = cn_var_缓冲区;
  生成基本块代码(r42, r43);
  r44 = cn_var_块_2->下一个;
  cn_var_块_2 = r44;
  goto while_cond_1251;

  while_exit_1253:
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
  if (r2) goto if_then_1254; else goto if_merge_1255;

  if_then_1254:
  r3 = cn_var_模块;
  r4 = cn_var_默认缓冲区大小;
  r5 = 创建输出缓冲区(r4);
  r6 = cn_var_默认缓冲区大小;
  r7 = 创建输出缓冲区(r6);
  r8 = cn_var_默认缓冲区大小;
  r9 = 创建输出缓冲区(r8);
  r10 = cn_var_选项;
  goto if_merge_1255;

  if_merge_1255:
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
  if (r0) goto if_then_1256; else goto if_merge_1257;

  if_then_1256:
  r1 = cn_var_上下文->头文件缓冲区;
  释放输出缓冲区(r1);
  r2 = cn_var_上下文->源文件缓冲区;
  释放输出缓冲区(r2);
  r3 = cn_var_上下文->函数体缓冲区;
  释放输出缓冲区(r3);
  r4 = cn_var_上下文;
  释放内存(r4);
  goto if_merge_1257;

  if_merge_1257:
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
  cn_var_选项_0 = (struct 代码生成选项*)分配清零内存(1, 类型大小(cn_var_代码生成选项));
  r2 = cn_var_选项_0;
  r3 = (void*)cn_rt_array_get_element(r2, 0, 8);
  return r3;
}

void 格式化整数(const char* cn_var_缓冲区, long long cn_var_缓冲区大小, long long cn_var_数值) {
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
  if (0) goto if_then_1258; else goto if_merge_1259;

  if_then_1258:
  r0 = cn_var_缓冲区;
    { long long _tmp_i3 = 48; cn_rt_array_set_element(r0, 0, &_tmp_i3, 8); }
  r1 = cn_var_缓冲区;
    { long long _tmp_i4 = 0; cn_rt_array_set_element(r1, 1, &_tmp_i4, 8); }
  return;
  goto if_merge_1259;

  if_merge_1259:
  char* cn_var_临时_0;
  r2 = 分配内存(32);
  cn_var_临时_0 = (char*)分配内存(32);
  long long cn_var_索引_1;
  cn_var_索引_1 = 0;
  long long cn_var_负数_2;
  cn_var_负数_2 = 0;
  r3 = cn_var_数值;
  r4 = r3 < 0;
  if (r4) goto if_then_1260; else goto if_merge_1261;

  if_then_1260:
  cn_var_负数_2 = 1;
  r5 = cn_var_数值;
  r6 = -r5;
  cn_var_数值 = r6;
  goto if_merge_1261;

  if_merge_1261:
  goto while_cond_1262;

  while_cond_1262:
  r7 = cn_var_数值;
  r8 = r7 > 0;
  if (r8) goto while_body_1263; else goto while_exit_1264;

  while_body_1263:
  r9 = cn_var_数值;
  r10 = r9 % 10;
  r11 = 48 + r10;
  r12 = cn_var_临时_0;
  r13 = cn_var_索引_1;
    { long long _tmp_r5 = r11; cn_rt_array_set_element(r12, r13, &_tmp_r5, 8); }
  r14 = cn_var_索引_1;
  r15 = r14 + 1;
  cn_var_索引_1 = r15;
  r16 = cn_var_数值;
  r17 = r16 / 10;
  cn_var_数值 = r17;
  goto while_cond_1262;

  while_exit_1264:
  long long cn_var_输出索引_3;
  cn_var_输出索引_3 = 0;
  r18 = cn_var_负数_2;
  if (r18) goto if_then_1265; else goto if_merge_1266;

  if_then_1265:
  r19 = cn_var_缓冲区;
  r20 = cn_var_输出索引_3;
    { long long _tmp_i5 = 45; cn_rt_array_set_element(r19, r20, &_tmp_i5, 8); }
  r21 = cn_var_输出索引_3;
  r22 = r21 + 1;
  cn_var_输出索引_3 = r22;
  goto if_merge_1266;

  if_merge_1266:
  goto while_cond_1267;

  while_cond_1267:
  r23 = cn_var_索引_1;
  r24 = r23 > 0;
  if (r24) goto while_body_1268; else goto while_exit_1269;

  while_body_1268:
  r25 = cn_var_索引_1;
  r26 = r25 - 1;
  cn_var_索引_1 = r26;
  r27 = cn_var_临时_0;
  r28 = cn_var_索引_1;
  r29 = (void*)cn_rt_array_get_element(r27, r28, 8);
  r30 = cn_var_缓冲区;
  r31 = cn_var_输出索引_3;
    { long long _tmp_r6 = r29; cn_rt_array_set_element(r30, r31, &_tmp_r6, 8); }
  r32 = cn_var_输出索引_3;
  r33 = r32 + 1;
  cn_var_输出索引_3 = r33;
  goto while_cond_1267;

  while_exit_1269:
  r34 = cn_var_缓冲区;
  r35 = cn_var_输出索引_3;
    { long long _tmp_i6 = 0; cn_rt_array_set_element(r34, r35, &_tmp_i6, 8); }
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
  cn_var_整数部分_0 = r0;
  double cn_var_小数临时_1;
  r1 = cn_var_数值;
  r2 = cn_var_整数部分_0;
  r3 = r1 - r2;
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

