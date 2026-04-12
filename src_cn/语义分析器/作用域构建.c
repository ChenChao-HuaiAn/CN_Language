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
char* 复制字符串副本(char*);
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
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_符号大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_作用域指针大小;

// CN Language Global Struct Forward Declarations
struct 作用域构建器;

// CN Language Global Struct Definitions
struct 作用域构建器 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
    long long 错误计数;
};

// Global Variables
long long cn_var_作用域构建器大小 = 32;
long long cn_var_类型推断上下文大小 = 16;

// Forward Declarations
struct 作用域构建器* 创建作用域构建器(struct 诊断集合*);
void 销毁作用域构建器(struct 作用域构建器*);
void 构建程序作用域(struct 作用域构建器*, struct 程序节点*);
void 构建声明作用域(struct 作用域构建器*, struct 声明节点*);
void 构建函数声明作用域(struct 作用域构建器*, struct 函数声明*);
struct 符号标志 创建符号标志(struct 函数声明*);
void 构建参数作用域(struct 作用域构建器*, struct 参数*);
void 构建变量声明作用域(struct 作用域构建器*, struct 变量声明*);
void 构建结构体声明作用域(struct 作用域构建器*, struct 结构体声明*);
struct 符号标志 创建成员标志(struct 结构体成员*);
void 构建枚举声明作用域(struct 作用域构建器*, struct 枚举声明*);
void 构建类声明作用域(struct 作用域构建器*, struct 类声明*);
void 构建类成员作用域(struct 作用域构建器*, struct 类成员*, struct 符号*, long long);
void 构建接口声明作用域(struct 作用域构建器*, struct 接口声明*);
void 构建导入声明作用域(struct 作用域构建器*, struct 导入声明*);
void 构建语句作用域(struct 作用域构建器*, struct 语句节点*);
void 构建块语句作用域(struct 作用域构建器*, struct 块语句*);
void 构建如果语句作用域(struct 作用域构建器*, struct 如果语句*);
void 构建当语句作用域(struct 作用域构建器*, struct 当语句*);
void 构建循环语句作用域(struct 作用域构建器*, struct 循环语句*);
void 构建选择语句作用域(struct 作用域构建器*, struct 选择语句*);
void 构建情况分支作用域(struct 作用域构建器*, struct 情况分支*);
void 构建返回语句作用域(struct 作用域构建器*, struct 返回语句*);
void 构建尝试语句作用域(struct 作用域构建器*, struct 尝试语句*);
void 构建捕获子句作用域(struct 作用域构建器*, struct 捕获子句*);
void 构建表达式作用域(struct 作用域构建器*, struct 表达式节点*);
void 构建二元表达式作用域(struct 作用域构建器*, struct 二元表达式*);
void 构建一元表达式作用域(struct 作用域构建器*, struct 一元表达式*);
void 构建函数调用表达式作用域(struct 作用域构建器*, struct 函数调用表达式*);
void 构建成员访问表达式作用域(struct 作用域构建器*, struct 成员访问表达式*);
void 构建数组访问表达式作用域(struct 作用域构建器*, struct 数组访问表达式*);
void 构建赋值表达式作用域(struct 作用域构建器*, struct 赋值表达式*);
void 构建三元表达式作用域(struct 作用域构建器*, struct 三元表达式*);
void 构建标识符表达式作用域(struct 作用域构建器*, struct 标识符表达式*);
void 报告重复符号错误(struct 作用域构建器*, char*, struct 源位置);
void 报告未定义标识符错误(struct 作用域构建器*, char*, struct 源位置);

struct 作用域构建器* 创建作用域构建器(struct 诊断集合* cn_var_诊断器) {
  long long r0, r3;
  void* r1;
  struct 符号表管理器* r2;
  void* r4;
  struct 作用域构建器* r5;
  struct 符号表管理器* r6;
  struct 诊断集合* r7;
  struct 作用域构建器* r8;

  entry:
  struct 作用域构建器* cn_var_构建器_0;
  r0 = cn_var_作用域构建器大小;
  r1 = 分配清零内存(1, r0);
  cn_var_构建器_0 = (struct 作用域构建器*)0;
  r2 = 创建符号表管理器();
  r3 = cn_var_类型推断上下文大小;
  r4 = 分配清零内存(1, r3);
  r5 = cn_var_构建器_0;
  r6 = r5->符号表;
  r7 = cn_var_诊断器;
  r8 = cn_var_构建器_0;
  return r8;
}

void 销毁作用域构建器(struct 作用域构建器* cn_var_构建器) {
  long long r1;
  struct 作用域构建器* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 作用域构建器* r4;
  struct 类型推断上下文* r5;
  struct 作用域构建器* r6;

  entry:
  r0 = cn_var_构建器;
  r1 = r0 == 0;
  if (r1) goto if_then_2162; else goto if_merge_2163;

  if_then_2162:
  return;
  goto if_merge_2163;

  if_merge_2163:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  销毁符号表管理器(r3);
  r4 = cn_var_构建器;
  r5 = r4->类型上下文;
  释放内存(r5);
  r6 = cn_var_构建器;
  释放内存(r6);
  return;
}

void 构建程序作用域(struct 作用域构建器* cn_var_构建器, struct 程序节点* cn_var_程序) {
  long long r1, r2, r4, r5, r9, r11, r12;
  struct 程序节点* r0;
  struct 程序节点* r3;
  struct 作用域构建器* r6;
  struct 程序节点* r7;
  struct 声明节点列表* r8;
  void* r10;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2164; else goto if_merge_2165;

  if_then_2164:
  return;
  goto if_merge_2165;

  if_merge_2165:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2166;

  for_cond_2166:
  r2 = cn_var_i_0;
  r3 = cn_var_程序;
  r4 = r3->声明个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2167; else goto for_exit_2169;

  for_body_2167:
  r6 = cn_var_构建器;
  r7 = cn_var_程序;
  r8 = r7->声明列表;
  r9 = cn_var_i_0;
  r10 = (void**)cn_rt_array_get_element(r8, r9, 8);
  构建声明作用域(r6, r10);
  goto for_update_2168;

  for_update_2168:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2166;

  for_exit_2169:
  return;
}

void 构建声明作用域(struct 作用域构建器* cn_var_构建器, struct 声明节点* cn_var_声明) {
  long long r1, r3, r4;
  struct 声明节点* r0;
  struct 声明节点* r2;
  struct 作用域构建器* r12;
  struct 声明节点* r13;
  struct 作用域构建器* r14;
  struct 声明节点* r15;
  struct 作用域构建器* r16;
  struct 声明节点* r17;
  struct 作用域构建器* r18;
  struct 声明节点* r19;
  struct 作用域构建器* r20;
  struct 声明节点* r21;
  struct 作用域构建器* r22;
  struct 声明节点* r23;
  struct 作用域构建器* r24;
  struct 声明节点* r25;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;

  entry:
  r0 = cn_var_声明;
  r1 = r0 == 0;
  if (r1) goto if_then_2170; else goto if_merge_2171;

  if_then_2170:
  return;
  goto if_merge_2171;

  if_merge_2171:
  r2 = cn_var_声明;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = r4 == 节点类型_函数声明;
  if (r5) goto case_body_2173; else goto switch_check_2181;

  switch_check_2181:
  r6 = r4 == 节点类型_变量声明;
  if (r6) goto case_body_2174; else goto switch_check_2182;

  switch_check_2182:
  r7 = r4 == 节点类型_结构体声明;
  if (r7) goto case_body_2175; else goto switch_check_2183;

  switch_check_2183:
  r8 = r4 == 节点类型_枚举声明;
  if (r8) goto case_body_2176; else goto switch_check_2184;

  switch_check_2184:
  r9 = r4 == 节点类型_类声明;
  if (r9) goto case_body_2177; else goto switch_check_2185;

  switch_check_2185:
  r10 = r4 == 节点类型_接口声明;
  if (r10) goto case_body_2178; else goto switch_check_2186;

  switch_check_2186:
  r11 = r4 == 节点类型_导入声明;
  if (r11) goto case_body_2179; else goto case_default_2180;

  case_body_2173:
  r12 = cn_var_构建器;
  r13 = cn_var_声明;
  构建函数声明作用域(r12, (struct 函数声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2174:
  r14 = cn_var_构建器;
  r15 = cn_var_声明;
  构建变量声明作用域(r14, (struct 变量声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2175:
  r16 = cn_var_构建器;
  r17 = cn_var_声明;
  构建结构体声明作用域(r16, (struct 结构体声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2176:
  r18 = cn_var_构建器;
  r19 = cn_var_声明;
  构建枚举声明作用域(r18, (struct 枚举声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2177:
  r20 = cn_var_构建器;
  r21 = cn_var_声明;
  构建类声明作用域(r20, (struct 类声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2178:
  r22 = cn_var_构建器;
  r23 = cn_var_声明;
  构建接口声明作用域(r22, (struct 接口声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_body_2179:
  r24 = cn_var_构建器;
  r25 = cn_var_声明;
  构建导入声明作用域(r24, (struct 导入声明*)cn_var_声明);
  goto switch_merge_2172;
  goto switch_merge_2172;

  case_default_2180:
  goto switch_merge_2172;
  goto switch_merge_2172;

  switch_merge_2172:
  return;
}

void 构建函数声明作用域(struct 作用域构建器* cn_var_构建器, struct 函数声明* cn_var_函数节点) {
  long long r1, r9, r15, r21, r36, r38, r39, r43, r45, r46, r49;
  char* r5;
  char* r24;
  char* r31;
  void* r7;
  void* r42;
  struct 函数声明* r0;
  struct 函数声明* r2;
  struct 函数声明* r4;
  struct 函数声明* r6;
  struct 函数声明* r8;
  struct 函数声明* r10;
  struct 类型节点* r11;
  struct 函数声明* r12;
  struct 符号* r16;
  struct 作用域构建器* r17;
  struct 符号表管理器* r18;
  struct 作用域构建器* r22;
  struct 函数声明* r23;
  struct 函数声明* r25;
  struct 作用域构建器* r28;
  struct 符号表管理器* r29;
  struct 函数声明* r30;
  struct 作用域构建器* r33;
  struct 符号表管理器* r34;
  struct 作用域* r35;
  struct 函数声明* r37;
  struct 作用域构建器* r40;
  struct 函数声明* r41;
  void* r44;
  struct 函数声明* r47;
  struct 块语句* r48;
  struct 作用域构建器* r50;
  struct 函数声明* r51;
  struct 块语句* r52;
  struct 作用域构建器* r53;
  struct 符号表管理器* r54;
  _Bool r20;
  struct 符号标志 r3;
  struct AST节点 r13;
  struct 源位置 r14;
  struct AST节点 r26;
  struct 源位置 r27;
  enum 符号类型 r19;
  enum 符号类型 r32;

  entry:
  r0 = cn_var_函数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2187; else goto if_merge_2188;

  if_then_2187:
  return;
  goto if_merge_2188;

  if_merge_2188:
  long long cn_var_标志_0;
  r2 = cn_var_函数节点;
  r3 = 创建符号标志(r2);
  cn_var_标志_0 = r3;
  enum 符号类型 cn_var_函数符号_1;
  r4 = cn_var_函数节点;
  r5 = r4->名称;
  r6 = cn_var_函数节点;
  r7 = r6->参数列表;
  r8 = cn_var_函数节点;
  r9 = r8->参数个数;
  r10 = cn_var_函数节点;
  r11 = r10->返回类型;
  r12 = cn_var_函数节点;
  r13 = r12->节点基类;
  r14 = r13.位置;
  r15 = cn_var_标志_0;
  r16 = 创建函数符号(r5, r7, r9, r11, r14, r15);
  cn_var_函数符号_1 = r16;
  r17 = cn_var_构建器;
  r18 = r17->符号表;
  r19 = cn_var_函数符号_1;
  r20 = 插入符号(r18, r19);
  r21 = r20 == 0;
  if (r21) goto if_then_2189; else goto if_merge_2190;

  if_then_2189:
  r22 = cn_var_构建器;
  r23 = cn_var_函数节点;
  r24 = r23->名称;
  r25 = cn_var_函数节点;
  r26 = r25->节点基类;
  r27 = r26.位置;
  报告重复符号错误(r22, r24, r27);
  return;
  goto if_merge_2190;

  if_merge_2190:
  r28 = cn_var_构建器;
  r29 = r28->符号表;
  r30 = cn_var_函数节点;
  r31 = r30->名称;
  r32 = cn_var_函数符号_1;
  进入作用域(r29, 作用域类型_函数作用域, r31, r32);
  r33 = cn_var_构建器;
  r34 = r33->符号表;
  r35 = r34->当前作用域;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2191;

  for_cond_2191:
  r36 = cn_var_i_2;
  r37 = cn_var_函数节点;
  r38 = r37->参数个数;
  r39 = r36 < r38;
  if (r39) goto for_body_2192; else goto for_exit_2194;

  for_body_2192:
  r40 = cn_var_构建器;
  r41 = cn_var_函数节点;
  r42 = r41->参数列表;
  r43 = cn_var_i_2;
  r44 = (void**)cn_rt_array_get_element(r42, r43, 8);
  构建参数作用域(r40, r44);
  goto for_update_2193;

  for_update_2193:
  r45 = cn_var_i_2;
  r46 = r45 + 1;
  cn_var_i_2 = r46;
  goto for_cond_2191;

  for_exit_2194:
  r47 = cn_var_函数节点;
  r48 = r47->函数体;
  r49 = r48 != 0;
  if (r49) goto if_then_2195; else goto if_merge_2196;

  if_then_2195:
  r50 = cn_var_构建器;
  r51 = cn_var_函数节点;
  r52 = r51->函数体;
  构建语句作用域(r50, (struct 语句节点*)cn_var_函数节点.函数体);
  goto if_merge_2196;

  if_merge_2196:
  r53 = cn_var_构建器;
  r54 = r53->符号表;
  离开作用域(r54);
  return;
}

struct 符号标志 创建符号标志(struct 函数声明* cn_var_函数节点) {
  long long r1, r2;
  void* r3;
  struct 函数声明* r4;
  struct 函数声明* r6;
  struct 函数声明* r8;
  struct 函数声明* r10;
  _Bool r5;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  struct 符号标志 r0;
  struct 符号标志 r12;

  entry:
  struct 符号标志 cn_var_标志_0;
  r0 = cn_var_标志_0;
  r1 = &cn_var_标志_0;
  r2 = sizeof(struct 符号标志);
  r3 = 设置内存((void*)cn_var_标志, 0, r2);
  r4 = cn_var_函数节点;
  r5 = r4->是静态;
  r6 = cn_var_函数节点;
  r7 = r6->是虚拟;
  r8 = cn_var_函数节点;
  r9 = r8->是重写;
  r10 = cn_var_函数节点;
  r11 = r10->是抽象;
  r12 = cn_var_标志_0;
  return r12;
}

void 构建参数作用域(struct 作用域构建器* cn_var_构建器, struct 参数* cn_var_参数节点) {
  long long r1, r7, r13, r18;
  char* r3;
  char* r16;
  struct 参数* r0;
  struct 参数* r2;
  struct 参数* r4;
  struct 类型节点* r5;
  struct 参数* r6;
  struct 符号* r8;
  struct 作用域构建器* r9;
  struct 符号表管理器* r10;
  struct 作用域构建器* r14;
  struct 参数* r15;
  struct 参数* r17;
  _Bool r12;
  enum 符号类型 r11;

  entry:
  r0 = cn_var_参数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2197; else goto if_merge_2198;

  if_then_2197:
  return;
  goto if_merge_2198;

  if_merge_2198:
  enum 符号类型 cn_var_参数符号_0;
  r2 = cn_var_参数节点;
  r3 = r2->名称;
  r4 = cn_var_参数节点;
  r5 = r4->类型;
  r6 = cn_var_参数节点;
  r7 = r6->定义位置;
  r8 = 创建参数符号(r3, r5, r7);
  cn_var_参数符号_0 = r8;
  r9 = cn_var_构建器;
  r10 = r9->符号表;
  r11 = cn_var_参数符号_0;
  r12 = 插入符号(r10, r11);
  r13 = r12 == 0;
  if (r13) goto if_then_2199; else goto if_merge_2200;

  if_then_2199:
  r14 = cn_var_构建器;
  r15 = cn_var_参数节点;
  r16 = r15->名称;
  r17 = cn_var_参数节点;
  r18 = r17->定义位置;
  报告重复符号错误(r14, r16, r18);
  goto if_merge_2200;

  if_merge_2200:
  return;
}

void 构建变量声明作用域(struct 作用域构建器* cn_var_构建器, struct 变量声明* cn_var_变量节点) {
  long long r1, r8, r16, r22, r31;
  char* r10;
  char* r25;
  struct 变量声明* r0;
  struct 变量声明* r2;
  struct 变量声明* r4;
  struct 变量声明* r6;
  struct 表达式节点* r7;
  struct 变量声明* r9;
  struct 变量声明* r11;
  struct 类型节点* r12;
  struct 变量声明* r13;
  struct 符号* r17;
  struct 作用域构建器* r18;
  struct 符号表管理器* r19;
  struct 作用域构建器* r23;
  struct 变量声明* r24;
  struct 变量声明* r26;
  struct 变量声明* r29;
  struct 表达式节点* r30;
  struct 作用域构建器* r32;
  struct 变量声明* r33;
  struct 表达式节点* r34;
  _Bool r3;
  _Bool r5;
  _Bool r21;
  struct AST节点 r14;
  struct 源位置 r15;
  struct AST节点 r27;
  struct 源位置 r28;
  enum 符号类型 r20;

  entry:
  r0 = cn_var_变量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2201; else goto if_merge_2202;

  if_then_2201:
  return;
  goto if_merge_2202;

  if_merge_2202:
  long long cn_var_标志_0;
  r2 = cn_var_变量节点;
  r3 = r2->是常量;
  r4 = cn_var_变量节点;
  r5 = r4->是静态;
  r6 = cn_var_变量节点;
  r7 = r6->初始值;
  r8 = r7 != 0;
  enum 符号类型 cn_var_变量符号_1;
  r9 = cn_var_变量节点;
  r10 = r9->名称;
  r11 = cn_var_变量节点;
  r12 = r11->类型;
  r13 = cn_var_变量节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  r16 = cn_var_标志_0;
  r17 = 创建变量符号(r10, r12, r15, r16);
  cn_var_变量符号_1 = r17;
  r18 = cn_var_构建器;
  r19 = r18->符号表;
  r20 = cn_var_变量符号_1;
  r21 = 插入符号(r19, r20);
  r22 = r21 == 0;
  if (r22) goto if_then_2203; else goto if_merge_2204;

  if_then_2203:
  r23 = cn_var_构建器;
  r24 = cn_var_变量节点;
  r25 = r24->名称;
  r26 = cn_var_变量节点;
  r27 = r26->节点基类;
  r28 = r27.位置;
  报告重复符号错误(r23, r25, r28);
  return;
  goto if_merge_2204;

  if_merge_2204:
  r29 = cn_var_变量节点;
  r30 = r29->初始值;
  r31 = r30 != 0;
  if (r31) goto if_then_2205; else goto if_merge_2206;

  if_then_2205:
  r32 = cn_var_构建器;
  r33 = cn_var_变量节点;
  r34 = r33->初始值;
  构建表达式作用域(r32, r34);
  goto if_merge_2206;

  if_merge_2206:
  return;
}

void 构建结构体声明作用域(struct 作用域构建器* cn_var_构建器, struct 结构体声明* cn_var_结构体节点) {
  long long r1, r12, r28, r30, r31, r33, r35, r36, r39, r41, r42, r43, r44, r48, r51, r53, r54, r57, r59, r60;
  char* r3;
  char* r15;
  char* r22;
  void* r38;
  struct 结构体声明* r0;
  struct 结构体声明* r2;
  struct 结构体声明* r4;
  struct 符号* r7;
  struct 作用域构建器* r8;
  struct 符号表管理器* r9;
  struct 作用域构建器* r13;
  struct 结构体声明* r14;
  struct 结构体声明* r16;
  struct 作用域构建器* r19;
  struct 符号表管理器* r20;
  struct 结构体声明* r21;
  struct 作用域构建器* r24;
  struct 符号表管理器* r25;
  struct 作用域* r26;
  struct 结构体声明* r27;
  struct 结构体声明* r29;
  void* r32;
  struct 结构体声明* r34;
  struct 结构体声明* r37;
  void* r40;
  struct 结构体声明* r45;
  struct 符号* r50;
  struct 作用域构建器* r55;
  struct 符号表管理器* r56;
  struct 作用域构建器* r61;
  struct 符号表管理器* r62;
  _Bool r11;
  _Bool r58;
  struct AST节点 r5;
  struct 源位置 r6;
  struct AST节点 r17;
  struct 源位置 r18;
  struct AST节点 r46;
  struct 源位置 r47;
  struct 符号标志 r49;
  enum 符号类型 r10;
  enum 符号类型 r23;
  enum 符号类型 r52;

  entry:
  r0 = cn_var_结构体节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2207; else goto if_merge_2208;

  if_then_2207:
  return;
  goto if_merge_2208;

  if_merge_2208:
  enum 符号类型 cn_var_结构体符号_0;
  r2 = cn_var_结构体节点;
  r3 = r2->名称;
  r4 = cn_var_结构体节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  r7 = 创建结构体符号(r3, r6);
  cn_var_结构体符号_0 = r7;
  r8 = cn_var_构建器;
  r9 = r8->符号表;
  r10 = cn_var_结构体符号_0;
  r11 = 插入符号(r9, r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2209; else goto if_merge_2210;

  if_then_2209:
  r13 = cn_var_构建器;
  r14 = cn_var_结构体节点;
  r15 = r14->名称;
  r16 = cn_var_结构体节点;
  r17 = r16->节点基类;
  r18 = r17.位置;
  报告重复符号错误(r13, r15, r18);
  return;
  goto if_merge_2210;

  if_merge_2210:
  r19 = cn_var_构建器;
  r20 = r19->符号表;
  r21 = cn_var_结构体节点;
  r22 = r21->名称;
  r23 = cn_var_结构体符号_0;
  进入作用域(r20, 作用域类型_结构体作用域, r22, r23);
  r24 = cn_var_构建器;
  r25 = r24->符号表;
  r26 = r25->当前作用域;
  r27 = cn_var_结构体节点;
  r28 = r27->成员个数;
  r29 = cn_var_结构体节点;
  r30 = r29->成员个数;
  r31 = cn_var_符号指针大小;
  r32 = 分配清零内存(r30, r31);
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2211;

  for_cond_2211:
  r33 = cn_var_i_1;
  r34 = cn_var_结构体节点;
  r35 = r34->成员个数;
  r36 = r33 < r35;
  if (r36) goto for_body_2212; else goto for_exit_2214;

  for_body_2212:
  long long cn_var_成员_2;
  r37 = cn_var_结构体节点;
  r38 = r37->成员;
  r39 = cn_var_i_1;
  r40 = (void**)cn_rt_array_get_element(r38, r39, 8);
  cn_var_成员_2 = r40;
  long long cn_var_成员符号_3;
  r41 = cn_var_成员_2;
  r42 = r41.名称;
  r43 = cn_var_成员_2;
  r44 = r43.类型;
  r45 = cn_var_结构体节点;
  r46 = r45->节点基类;
  r47 = r46.位置;
  r48 = cn_var_成员_2;
  r49 = 创建成员标志(r48);
  r50 = 创建变量符号(r42, r44, r47, r49);
  cn_var_成员符号_3 = r50;
  r51 = cn_var_成员符号_3;
  r52 = cn_var_结构体符号_0;
  r53 = r52.成员列表;
  r54 = cn_var_i_1;
    { long long _tmp_r6 = r51; cn_rt_array_set_element(r53, r54, &_tmp_r6, 8); }
  r55 = cn_var_构建器;
  r56 = r55->符号表;
  r57 = cn_var_成员符号_3;
  r58 = 插入符号(r56, r57);
  goto for_update_2213;

  for_update_2213:
  r59 = cn_var_i_1;
  r60 = r59 + 1;
  cn_var_i_1 = r60;
  goto for_cond_2211;

  for_exit_2214:
  r61 = cn_var_构建器;
  r62 = r61->符号表;
  离开作用域(r62);
  return;
}

struct 符号标志 创建成员标志(struct 结构体成员* cn_var_成员) {
  long long r4, r7, r10, r11;
  struct 结构体成员* r0;
  struct 结构体成员* r2;
  struct 结构体成员* r5;
  struct 结构体成员* r8;
  _Bool r1;
  enum 可见性 r3;
  enum 可见性 r6;
  enum 可见性 r9;

  entry:
  long long cn_var_标志_0;
  r0 = cn_var_成员;
  r1 = r0->是常量;
  r2 = cn_var_成员;
  r3 = r2->可见性;
  r4 = r3 == /* NONE */;
  r5 = cn_var_成员;
  r6 = r5->可见性;
  r7 = r6 == /* NONE */;
  r8 = cn_var_成员;
  r9 = r8->可见性;
  r10 = r9 == /* NONE */;
  r11 = cn_var_标志_0;
  return r11;
}

void 构建枚举声明作用域(struct 作用域构建器* cn_var_构建器, struct 枚举声明* cn_var_枚举节点) {
  long long r1, r12, r28, r30, r31, r33, r35, r36, r39, r41, r42, r43, r44, r45, r46, r47, r48, r49, r54, r56, r57, r60, r62, r63, r64, r65;
  char* r3;
  char* r15;
  char* r22;
  void* r38;
  struct 枚举声明* r0;
  struct 枚举声明* r2;
  struct 枚举声明* r4;
  struct 符号* r7;
  struct 作用域构建器* r8;
  struct 符号表管理器* r9;
  struct 作用域构建器* r13;
  struct 枚举声明* r14;
  struct 枚举声明* r16;
  struct 作用域构建器* r19;
  struct 符号表管理器* r20;
  struct 枚举声明* r21;
  struct 作用域构建器* r24;
  struct 符号表管理器* r25;
  struct 作用域* r26;
  struct 枚举声明* r27;
  struct 枚举声明* r29;
  void* r32;
  struct 枚举声明* r34;
  struct 枚举声明* r37;
  void* r40;
  struct 枚举声明* r50;
  struct 符号* r53;
  struct 作用域构建器* r58;
  struct 符号表管理器* r59;
  struct 作用域构建器* r66;
  struct 符号表管理器* r67;
  _Bool r11;
  _Bool r61;
  struct AST节点 r5;
  struct 源位置 r6;
  struct AST节点 r17;
  struct 源位置 r18;
  struct AST节点 r51;
  struct 源位置 r52;
  enum 符号类型 r10;
  enum 符号类型 r23;
  enum 符号类型 r55;

  entry:
  r0 = cn_var_枚举节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2215; else goto if_merge_2216;

  if_then_2215:
  return;
  goto if_merge_2216;

  if_merge_2216:
  enum 符号类型 cn_var_枚举符号_0;
  r2 = cn_var_枚举节点;
  r3 = r2->名称;
  r4 = cn_var_枚举节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  r7 = 创建枚举符号(r3, r6);
  cn_var_枚举符号_0 = r7;
  r8 = cn_var_构建器;
  r9 = r8->符号表;
  r10 = cn_var_枚举符号_0;
  r11 = 插入符号(r9, r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2217; else goto if_merge_2218;

  if_then_2217:
  r13 = cn_var_构建器;
  r14 = cn_var_枚举节点;
  r15 = r14->名称;
  r16 = cn_var_枚举节点;
  r17 = r16->节点基类;
  r18 = r17.位置;
  报告重复符号错误(r13, r15, r18);
  return;
  goto if_merge_2218;

  if_merge_2218:
  r19 = cn_var_构建器;
  r20 = r19->符号表;
  r21 = cn_var_枚举节点;
  r22 = r21->名称;
  r23 = cn_var_枚举符号_0;
  进入作用域(r20, 作用域类型_枚举作用域, r22, r23);
  r24 = cn_var_构建器;
  r25 = r24->符号表;
  r26 = r25->当前作用域;
  r27 = cn_var_枚举节点;
  r28 = r27->成员个数;
  r29 = cn_var_枚举节点;
  r30 = r29->成员个数;
  r31 = cn_var_符号指针大小;
  r32 = 分配清零内存(r30, r31);
  long long cn_var_当前值_1;
  cn_var_当前值_1 = 0;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2219;

  for_cond_2219:
  r33 = cn_var_i_2;
  r34 = cn_var_枚举节点;
  r35 = r34->成员个数;
  r36 = r33 < r35;
  if (r36) goto for_body_2220; else goto for_exit_2222;

  for_body_2220:
  long long cn_var_成员_3;
  r37 = cn_var_枚举节点;
  r38 = r37->成员;
  r39 = cn_var_i_2;
  r40 = (void**)cn_rt_array_get_element(r38, r39, 8);
  cn_var_成员_3 = r40;
  r41 = cn_var_成员_3;
  r42 = r41.有显式值;
  if (r42) goto if_then_2223; else goto if_merge_2224;

  for_update_2221:
  r64 = cn_var_i_2;
  r65 = r64 + 1;
  cn_var_i_2 = r65;
  goto for_cond_2219;

  for_exit_2222:
  r66 = cn_var_构建器;
  r67 = r66->符号表;
  离开作用域(r67);

  if_then_2223:
  r43 = cn_var_成员_3;
  r44 = r43.值;
  cn_var_当前值_1 = r44;
  goto if_merge_2224;

  if_merge_2224:
  long long cn_var_成员符号_4;
  r45 = cn_var_成员_3;
  r46 = r45.名称;
  r47 = cn_var_当前值_1;
  r48 = cn_var_成员_3;
  r49 = r48.有显式值;
  r50 = cn_var_枚举节点;
  r51 = r50->节点基类;
  r52 = r51.位置;
  r53 = 创建枚举成员符号(r46, r47, r49, r52);
  cn_var_成员符号_4 = r53;
  r54 = cn_var_成员符号_4;
  r55 = cn_var_枚举符号_0;
  r56 = r55.成员列表;
  r57 = cn_var_i_2;
    { long long _tmp_r7 = r54; cn_rt_array_set_element(r56, r57, &_tmp_r7, 8); }
  r58 = cn_var_构建器;
  r59 = r58->符号表;
  r60 = cn_var_成员符号_4;
  r61 = 插入符号(r59, r60);
  r62 = cn_var_当前值_1;
  r63 = r62 + 1;
  cn_var_当前值_1 = r63;
  goto for_update_2221;
  return;
}

void 构建类声明作用域(struct 作用域构建器* cn_var_构建器, struct 类声明* cn_var_类节点) {
  long long r1, r14, r21, r24, r27, r28, r34, r35, r36, r37, r38, r39, r40, r42, r43, r45, r47, r48, r50, r52, r53, r58, r61, r63, r65, r66, r69, r70, r71, r72, r82, r84, r85, r87, r89, r90, r94, r97, r98, r99;
  char* r3;
  char* r17;
  char* r23;
  char* r26;
  char* r32;
  char* r76;
  void* r57;
  void* r93;
  struct 类声明* r0;
  struct 类声明* r2;
  struct 类声明* r4;
  struct 类声明* r7;
  struct 符号* r9;
  struct 作用域构建器* r10;
  struct 符号表管理器* r11;
  struct 作用域构建器* r15;
  struct 类声明* r16;
  struct 类声明* r18;
  struct 类声明* r22;
  struct 类声明* r25;
  struct 作用域构建器* r29;
  struct 符号表管理器* r30;
  struct 类声明* r31;
  struct 符号* r33;
  struct 类声明* r41;
  struct 类声明* r44;
  struct 类声明* r46;
  void* r49;
  struct 类声明* r51;
  struct 作用域构建器* r54;
  struct 符号表管理器* r55;
  struct 类声明* r56;
  void* r59;
  struct 符号* r60;
  struct 作用域构建器* r73;
  struct 符号表管理器* r74;
  struct 类声明* r75;
  struct 作用域构建器* r78;
  struct 符号表管理器* r79;
  struct 作用域* r80;
  struct 类声明* r81;
  struct 类声明* r83;
  void* r86;
  struct 类声明* r88;
  struct 作用域构建器* r91;
  struct 类声明* r92;
  void* r95;
  struct 作用域构建器* r100;
  struct 符号表管理器* r101;
  _Bool r8;
  _Bool r13;
  struct AST节点 r5;
  struct 源位置 r6;
  struct AST节点 r19;
  struct 源位置 r20;
  enum 符号类型 r12;
  enum 符号类型 r62;
  enum 符号类型 r64;
  enum 符号类型 r67;
  enum 符号类型 r68;
  enum 符号类型 r77;
  enum 符号类型 r96;

  entry:
  r0 = cn_var_类节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2225; else goto if_merge_2226;

  if_then_2225:
  return;
  goto if_merge_2226;

  if_merge_2226:
  enum 符号类型 cn_var_类符号_0;
  r2 = cn_var_类节点;
  r3 = r2->名称;
  r4 = cn_var_类节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  r7 = cn_var_类节点;
  r8 = r7->是抽象;
  r9 = 创建类符号(r3, r6, r8);
  cn_var_类符号_0 = r9;
  r10 = cn_var_构建器;
  r11 = r10->符号表;
  r12 = cn_var_类符号_0;
  r13 = 插入符号(r11, r12);
  r14 = r13 == 0;
  if (r14) goto if_then_2227; else goto if_merge_2228;

  if_then_2227:
  r15 = cn_var_构建器;
  r16 = cn_var_类节点;
  r17 = r16->名称;
  r18 = cn_var_类节点;
  r19 = r18->节点基类;
  r20 = r19.位置;
  报告重复符号错误(r15, r17, r20);
  return;
  goto if_merge_2228;

  if_merge_2228:
  r22 = cn_var_类节点;
  r23 = r22->基类名称;
  r24 = r23 != 0;
  if (r24) goto logic_rhs_2231; else goto logic_merge_2232;

  if_then_2229:
  long long cn_var_父类符号_1;
  r29 = cn_var_构建器;
  r30 = r29->符号表;
  r31 = cn_var_类节点;
  r32 = r31->基类名称;
  r33 = 查找符号(r30, r32);
  cn_var_父类符号_1 = r33;
  r35 = cn_var_父类符号_1;
  r36 = r35 != 0;
  if (r36) goto logic_rhs_2235; else goto logic_merge_2236;

  if_merge_2230:
  r41 = cn_var_类节点;
  r42 = r41->接口个数;
  r43 = r42 > 0;
  if (r43) goto if_then_2237; else goto if_merge_2238;

  logic_rhs_2231:
  r25 = cn_var_类节点;
  r26 = r25->基类名称;
  r27 = 获取字符串长度(r26);
  r28 = r27 > 0;
  goto logic_merge_2232;

  logic_merge_2232:
  if (r28) goto if_then_2229; else goto if_merge_2230;

  if_then_2233:
  r40 = cn_var_父类符号_1;
  goto if_merge_2234;

  if_merge_2234:
  goto if_merge_2230;

  logic_rhs_2235:
  r37 = cn_var_父类符号_1;
  r38 = r37.种类;
  r39 = r38 == 符号类型_类符号;
  goto logic_merge_2236;

  logic_merge_2236:
  if (r39) goto if_then_2233; else goto if_merge_2234;

  if_then_2237:
  r44 = cn_var_类节点;
  r45 = r44->接口个数;
  r46 = cn_var_类节点;
  r47 = r46->接口个数;
  r48 = cn_var_符号指针大小;
  r49 = 分配清零内存(r47, r48);
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2239;

  if_merge_2238:
  r73 = cn_var_构建器;
  r74 = r73->符号表;
  r75 = cn_var_类节点;
  r76 = r75->名称;
  r77 = cn_var_类符号_0;
  进入作用域(r74, 作用域类型_类作用域, r76, r77);
  r78 = cn_var_构建器;
  r79 = r78->符号表;
  r80 = r79->当前作用域;
  r81 = cn_var_类节点;
  r82 = r81->成员个数;
  r83 = cn_var_类节点;
  r84 = r83->成员个数;
  r85 = cn_var_符号指针大小;
  r86 = 分配清零内存(r84, r85);
  long long cn_var_i_4;
  cn_var_i_4 = 0;
  goto for_cond_2247;

  for_cond_2239:
  r50 = cn_var_i_2;
  r51 = cn_var_类节点;
  r52 = r51->接口个数;
  r53 = r50 < r52;
  if (r53) goto for_body_2240; else goto for_exit_2242;

  for_body_2240:
  enum 符号类型 cn_var_接口符号_3;
  r54 = cn_var_构建器;
  r55 = r54->符号表;
  r56 = cn_var_类节点;
  r57 = r56->实现接口;
  r58 = cn_var_i_2;
  r59 = (void**)cn_rt_array_get_element(r57, r58, 8);
  r60 = 查找符号(r55, r59);
  cn_var_接口符号_3 = r60;
  r62 = cn_var_接口符号_3;
  r63 = r62 != 0;
  if (r63) goto logic_rhs_2245; else goto logic_merge_2246;

  for_update_2241:
  r71 = cn_var_i_2;
  r72 = r71 + 1;
  cn_var_i_2 = r72;
  goto for_cond_2239;

  for_exit_2242:
  goto if_merge_2238;

  if_then_2243:
  r67 = cn_var_接口符号_3;
  r68 = cn_var_类符号_0;
  r69 = r68.实现接口;
  r70 = cn_var_i_2;
    { long long _tmp_r8 = r67; cn_rt_array_set_element(r69, r70, &_tmp_r8, 8); }
  goto if_merge_2244;

  if_merge_2244:
  goto for_update_2241;

  logic_rhs_2245:
  r64 = cn_var_接口符号_3;
  r65 = r64.种类;
  r66 = r65 == 符号类型_接口符号;
  goto logic_merge_2246;

  logic_merge_2246:
  if (r66) goto if_then_2243; else goto if_merge_2244;

  for_cond_2247:
  r87 = cn_var_i_4;
  r88 = cn_var_类节点;
  r89 = r88->成员个数;
  r90 = r87 < r89;
  if (r90) goto for_body_2248; else goto for_exit_2250;

  for_body_2248:
  r91 = cn_var_构建器;
  r92 = cn_var_类节点;
  r93 = r92->成员;
  r94 = cn_var_i_4;
  r95 = (void**)cn_rt_array_get_element(r93, r94, 8);
  r96 = cn_var_类符号_0;
  r97 = cn_var_i_4;
  构建类成员作用域(r91, r95, r96, r97);
  goto for_update_2249;

  for_update_2249:
  r98 = cn_var_i_4;
  r99 = r98 + 1;
  cn_var_i_4 = r99;
  goto for_cond_2247;

  for_exit_2250:
  r100 = cn_var_构建器;
  r101 = r100->符号表;
  离开作用域(r101);
  return;
}

void 构建类成员作用域(struct 作用域构建器* cn_var_构建器, struct 类成员* cn_var_成员节点, struct 符号* cn_var_类符号, long long cn_var_索引) {
  long long r1, r4, r9, r12, r15, r25, r29, r40, r43, r46, r54, r62, r64, r65, r66, r68, r69, r72;
  char* r17;
  char* r48;
  void* r51;
  struct 类成员* r0;
  struct 类成员* r2;
  struct 类成员* r5;
  struct 类成员* r7;
  struct 类成员* r10;
  struct 类成员* r13;
  struct 类成员* r16;
  struct 类成员* r18;
  struct 变量声明* r19;
  struct 类型节点* r20;
  struct 类成员* r21;
  struct 变量声明* r22;
  struct 符号* r26;
  struct 类成员* r27;
  struct 类成员* r30;
  struct 类成员* r32;
  struct 类成员* r34;
  struct 类成员* r36;
  struct 类成员* r38;
  struct 类成员* r41;
  struct 类成员* r44;
  struct 类成员* r47;
  struct 类成员* r49;
  struct 函数声明* r50;
  struct 类成员* r52;
  struct 函数声明* r53;
  struct 类成员* r55;
  struct 函数声明* r56;
  struct 类型节点* r57;
  struct 类成员* r58;
  struct 函数声明* r59;
  struct 符号* r63;
  struct 作用域构建器* r70;
  struct 符号表管理器* r71;
  _Bool r6;
  _Bool r31;
  _Bool r33;
  _Bool r35;
  _Bool r37;
  _Bool r73;
  struct AST节点 r23;
  struct 源位置 r24;
  struct AST节点 r60;
  struct 源位置 r61;
  enum 节点类型 r3;
  enum 可见性 r8;
  enum 可见性 r11;
  enum 可见性 r14;
  enum 节点类型 r28;
  enum 可见性 r39;
  enum 可见性 r42;
  enum 可见性 r45;
  enum 符号类型 r67;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2251; else goto if_merge_2252;

  if_then_2251:
  return;
  goto if_merge_2252;

  if_merge_2252:
  long long cn_var_成员符号_0;
  r2 = cn_var_成员节点;
  r3 = r2->类型;
  r4 = r3 == 节点类型_变量声明;
  if (r4) goto if_then_2253; else goto if_else_2254;

  if_then_2253:
  long long cn_var_标志_1;
  r5 = cn_var_成员节点;
  r6 = r5->是静态;
  r7 = cn_var_成员节点;
  r8 = r7->可见性;
  r9 = r8 == /* NONE */;
  r10 = cn_var_成员节点;
  r11 = r10->可见性;
  r12 = r11 == /* NONE */;
  r13 = cn_var_成员节点;
  r14 = r13->可见性;
  r15 = r14 == /* NONE */;
  r16 = cn_var_成员节点;
  r17 = r16->名称;
  r18 = cn_var_成员节点;
  r19 = r18->字段;
  r20 = r19->类型;
  r21 = cn_var_成员节点;
  r22 = r21->字段;
  r23 = r22->节点基类;
  r24 = r23.位置;
  r25 = cn_var_标志_1;
  r26 = 创建变量符号(r17, r20, r24, r25);
  cn_var_成员符号_0 = r26;
  goto if_merge_2255;

  if_else_2254:
  r27 = cn_var_成员节点;
  r28 = r27->类型;
  r29 = r28 == 节点类型_函数声明;
  if (r29) goto if_then_2256; else goto if_merge_2257;

  if_merge_2255:
  r64 = cn_var_成员符号_0;
  r65 = r64 != 0;
  if (r65) goto if_then_2258; else goto if_merge_2259;

  if_then_2256:
  long long cn_var_标志_2;
  r30 = cn_var_成员节点;
  r31 = r30->是静态;
  r32 = cn_var_成员节点;
  r33 = r32->是虚拟;
  r34 = cn_var_成员节点;
  r35 = r34->是重写;
  r36 = cn_var_成员节点;
  r37 = r36->是抽象;
  r38 = cn_var_成员节点;
  r39 = r38->可见性;
  r40 = r39 == /* NONE */;
  r41 = cn_var_成员节点;
  r42 = r41->可见性;
  r43 = r42 == /* NONE */;
  r44 = cn_var_成员节点;
  r45 = r44->可见性;
  r46 = r45 == /* NONE */;
  r47 = cn_var_成员节点;
  r48 = r47->名称;
  r49 = cn_var_成员节点;
  r50 = r49->方法;
  r51 = r50->参数列表;
  r52 = cn_var_成员节点;
  r53 = r52->方法;
  r54 = r53->参数个数;
  r55 = cn_var_成员节点;
  r56 = r55->方法;
  r57 = r56->返回类型;
  r58 = cn_var_成员节点;
  r59 = r58->方法;
  r60 = r59->节点基类;
  r61 = r60.位置;
  r62 = cn_var_标志_2;
  r63 = 创建函数符号(r48, r51, r54, r57, r61, r62);
  cn_var_成员符号_0 = r63;
  goto if_merge_2257;

  if_merge_2257:
  goto if_merge_2255;

  if_then_2258:
  r66 = cn_var_成员符号_0;
  r67 = cn_var_类符号;
  r68 = r67.成员列表;
  r69 = cn_var_索引;
    { long long _tmp_r9 = r66; cn_rt_array_set_element(r68, r69, &_tmp_r9, 8); }
  r70 = cn_var_构建器;
  r71 = r70->符号表;
  r72 = cn_var_成员符号_0;
  r73 = 插入符号(r71, r72);
  goto if_merge_2259;

  if_merge_2259:
  return;
}

void 构建接口声明作用域(struct 作用域构建器* cn_var_构建器, struct 接口声明* cn_var_接口节点) {
  long long r1, r12, r28, r30, r31, r33, r35, r36, r38, r39, r41, r42, r43, r44, r45, r46, r47, r48, r52, r54, r56, r57, r60, r62, r63;
  char* r3;
  char* r15;
  char* r22;
  struct 接口声明* r0;
  struct 接口声明* r2;
  struct 接口声明* r4;
  struct 符号* r7;
  struct 作用域构建器* r8;
  struct 符号表管理器* r9;
  struct 作用域构建器* r13;
  struct 接口声明* r14;
  struct 接口声明* r16;
  struct 作用域构建器* r19;
  struct 符号表管理器* r20;
  struct 接口声明* r21;
  struct 作用域构建器* r24;
  struct 符号表管理器* r25;
  struct 作用域* r26;
  struct 接口声明* r27;
  struct 接口声明* r29;
  void* r32;
  struct 接口声明* r34;
  struct 接口声明* r37;
  void* r40;
  struct 接口声明* r49;
  struct 符号* r53;
  struct 作用域构建器* r58;
  struct 符号表管理器* r59;
  struct 作用域构建器* r64;
  struct 符号表管理器* r65;
  _Bool r11;
  _Bool r61;
  struct AST节点 r5;
  struct 源位置 r6;
  struct AST节点 r17;
  struct 源位置 r18;
  struct AST节点 r50;
  struct 源位置 r51;
  enum 符号类型 r10;
  enum 符号类型 r23;
  enum 符号类型 r55;

  entry:
  r0 = cn_var_接口节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2260; else goto if_merge_2261;

  if_then_2260:
  return;
  goto if_merge_2261;

  if_merge_2261:
  enum 符号类型 cn_var_接口符号_0;
  r2 = cn_var_接口节点;
  r3 = r2->名称;
  r4 = cn_var_接口节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  r7 = 创建接口符号(r3, r6);
  cn_var_接口符号_0 = r7;
  r8 = cn_var_构建器;
  r9 = r8->符号表;
  r10 = cn_var_接口符号_0;
  r11 = 插入符号(r9, r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2262; else goto if_merge_2263;

  if_then_2262:
  r13 = cn_var_构建器;
  r14 = cn_var_接口节点;
  r15 = r14->名称;
  r16 = cn_var_接口节点;
  r17 = r16->节点基类;
  r18 = r17.位置;
  报告重复符号错误(r13, r15, r18);
  return;
  goto if_merge_2263;

  if_merge_2263:
  r19 = cn_var_构建器;
  r20 = r19->符号表;
  r21 = cn_var_接口节点;
  r22 = r21->名称;
  r23 = cn_var_接口符号_0;
  进入作用域(r20, 作用域类型_类作用域, r22, r23);
  r24 = cn_var_构建器;
  r25 = r24->符号表;
  r26 = r25->当前作用域;
  r27 = cn_var_接口节点;
  r28 = r27->方法个数;
  r29 = cn_var_接口节点;
  r30 = r29->方法个数;
  r31 = cn_var_符号指针大小;
  r32 = 分配清零内存(r30, r31);
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2264;

  for_cond_2264:
  r33 = cn_var_i_1;
  r34 = cn_var_接口节点;
  r35 = r34->方法个数;
  r36 = r33 < r35;
  if (r36) goto for_body_2265; else goto for_exit_2267;

  for_body_2265:
  long long cn_var_方法_2;
  r37 = cn_var_接口节点;
  r38 = r37->方法列表;
  r39 = cn_var_i_1;
  r40 = (void**)cn_rt_array_get_element(r38, r39, 8);
  cn_var_方法_2 = r40;
  long long cn_var_标志_3;
  long long cn_var_方法符号_4;
  r41 = cn_var_方法_2;
  r42 = r41.名称;
  r43 = cn_var_方法_2;
  r44 = r43.参数列表;
  r45 = cn_var_方法_2;
  r46 = r45.参数个数;
  r47 = cn_var_方法_2;
  r48 = r47.返回类型;
  r49 = cn_var_接口节点;
  r50 = r49->节点基类;
  r51 = r50.位置;
  r52 = cn_var_标志_3;
  r53 = 创建函数符号(r42, r44, r46, r48, r51, r52);
  cn_var_方法符号_4 = r53;
  r54 = cn_var_方法符号_4;
  r55 = cn_var_接口符号_0;
  r56 = r55.成员列表;
  r57 = cn_var_i_1;
    { long long _tmp_r10 = r54; cn_rt_array_set_element(r56, r57, &_tmp_r10, 8); }
  r58 = cn_var_构建器;
  r59 = r58->符号表;
  r60 = cn_var_方法符号_4;
  r61 = 插入符号(r59, r60);
  goto for_update_2266;

  for_update_2266:
  r62 = cn_var_i_1;
  r63 = r62 + 1;
  cn_var_i_1 = r63;
  goto for_cond_2264;

  for_exit_2267:
  r64 = cn_var_构建器;
  r65 = r64->符号表;
  离开作用域(r65);
  return;
}

void 构建导入声明作用域(struct 作用域构建器* cn_var_构建器, struct 导入声明* cn_var_导入节点) {
  long long r1, r2;
  char* r5;
  struct 导入声明* r0;
  void* r3;
  struct 导入声明* r4;
  struct 导入声明* r6;
  struct 作用域构建器* r9;
  struct 符号表管理器* r10;
  _Bool r12;
  struct AST节点 r7;
  struct 源位置 r8;
  enum 符号类型 r11;

  entry:
  r0 = cn_var_导入节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2268; else goto if_merge_2269;

  if_then_2268:
  return;
  goto if_merge_2269;

  if_merge_2269:
  struct 符号* cn_var_导入符号_0;
  r2 = cn_var_符号大小;
  r3 = 分配清零内存(1, r2);
  cn_var_导入符号_0 = (struct 符号*)0;
  r4 = cn_var_导入节点;
  r5 = r4->模块名;
  r6 = cn_var_导入节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  r9 = cn_var_构建器;
  r10 = r9->符号表;
  r11 = cn_var_导入符号_0;
  r12 = 插入符号(r10, r11);
  return;
}

void 构建语句作用域(struct 作用域构建器* cn_var_构建器, struct 语句节点* cn_var_语句) {
  long long r1, r3, r4;
  struct 语句节点* r0;
  struct 语句节点* r2;
  struct 作用域构建器* r13;
  struct 语句节点* r14;
  struct 作用域构建器* r15;
  struct 语句节点* r16;
  struct 作用域构建器* r17;
  struct 语句节点* r18;
  struct 作用域构建器* r19;
  struct 语句节点* r20;
  struct 作用域构建器* r21;
  struct 语句节点* r22;
  struct 作用域构建器* r23;
  struct 语句节点* r24;
  struct 表达式节点* r25;
  struct 作用域构建器* r26;
  struct 语句节点* r27;
  struct 作用域构建器* r28;
  struct 语句节点* r29;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;

  entry:
  r0 = cn_var_语句;
  r1 = r0 == 0;
  if (r1) goto if_then_2270; else goto if_merge_2271;

  if_then_2270:
  return;
  goto if_merge_2271;

  if_merge_2271:
  r2 = cn_var_语句;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = r4 == 节点类型_块语句;
  if (r5) goto case_body_2273; else goto switch_check_2282;

  switch_check_2282:
  r6 = r4 == 节点类型_如果语句;
  if (r6) goto case_body_2274; else goto switch_check_2283;

  switch_check_2283:
  r7 = r4 == 节点类型_当语句;
  if (r7) goto case_body_2275; else goto switch_check_2284;

  switch_check_2284:
  r8 = r4 == 节点类型_循环语句;
  if (r8) goto case_body_2276; else goto switch_check_2285;

  switch_check_2285:
  r9 = r4 == 节点类型_选择语句;
  if (r9) goto case_body_2277; else goto switch_check_2286;

  switch_check_2286:
  r10 = r4 == 节点类型_表达式语句;
  if (r10) goto case_body_2278; else goto switch_check_2287;

  switch_check_2287:
  r11 = r4 == 节点类型_返回语句;
  if (r11) goto case_body_2279; else goto switch_check_2288;

  switch_check_2288:
  r12 = r4 == 节点类型_尝试语句;
  if (r12) goto case_body_2280; else goto case_default_2281;

  case_body_2273:
  r13 = cn_var_构建器;
  r14 = cn_var_语句;
  构建块语句作用域(r13, (struct 块语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2274:
  r15 = cn_var_构建器;
  r16 = cn_var_语句;
  构建如果语句作用域(r15, (struct 如果语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2275:
  r17 = cn_var_构建器;
  r18 = cn_var_语句;
  构建当语句作用域(r17, (struct 当语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2276:
  r19 = cn_var_构建器;
  r20 = cn_var_语句;
  构建循环语句作用域(r19, (struct 循环语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2277:
  r21 = cn_var_构建器;
  r22 = cn_var_语句;
  构建选择语句作用域(r21, (struct 选择语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2278:
  r23 = cn_var_构建器;
  r24 = cn_var_语句;
  r25 = (struct 表达式语句*)cn_var_语句->表达式;
  构建表达式作用域(r23, r25);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2279:
  r26 = cn_var_构建器;
  r27 = cn_var_语句;
  构建返回语句作用域(r26, (struct 返回语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_body_2280:
  r28 = cn_var_构建器;
  r29 = cn_var_语句;
  构建尝试语句作用域(r28, (struct 尝试语句*)cn_var_语句);
  goto switch_merge_2272;
  goto switch_merge_2272;

  case_default_2281:
  goto switch_merge_2272;
  goto switch_merge_2272;

  switch_merge_2272:
  return;
}

void 构建块语句作用域(struct 作用域构建器* cn_var_构建器, struct 块语句* cn_var_块节点) {
  long long r1, r4, r6, r7, r10, r11, r13, r14;
  struct 块语句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 块语句* r5;
  struct 作用域构建器* r8;
  struct 块语句* r9;
  void* r12;
  struct 作用域构建器* r15;
  struct 符号表管理器* r16;

  entry:
  r0 = cn_var_块节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2289; else goto if_merge_2290;

  if_then_2289:
  return;
  goto if_merge_2290;

  if_merge_2290:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  进入作用域(r3, 作用域类型_块作用域, "块", 0);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2291;

  for_cond_2291:
  r4 = cn_var_i_0;
  r5 = cn_var_块节点;
  r6 = r5->语句个数;
  r7 = r4 < r6;
  if (r7) goto for_body_2292; else goto for_exit_2294;

  for_body_2292:
  r8 = cn_var_构建器;
  r9 = cn_var_块节点;
  r10 = r9->语句列表;
  r11 = cn_var_i_0;
  r12 = (void**)cn_rt_array_get_element(r10, r11, 8);
  构建语句作用域(r8, r12);
  goto for_update_2293;

  for_update_2293:
  r13 = cn_var_i_0;
  r14 = r13 + 1;
  cn_var_i_0 = r14;
  goto for_cond_2291;

  for_exit_2294:
  r15 = cn_var_构建器;
  r16 = r15->符号表;
  离开作用域(r16);
  return;
}

void 构建如果语句作用域(struct 作用域构建器* cn_var_构建器, struct 如果语句* cn_var_如果节点) {
  long long r1, r7, r13;
  struct 如果语句* r0;
  struct 作用域构建器* r2;
  struct 如果语句* r3;
  struct 表达式节点* r4;
  struct 如果语句* r5;
  struct 块语句* r6;
  struct 作用域构建器* r8;
  struct 如果语句* r9;
  struct 块语句* r10;
  struct 如果语句* r11;
  struct 语句节点* r12;
  struct 作用域构建器* r14;
  struct 如果语句* r15;
  struct 语句节点* r16;

  entry:
  r0 = cn_var_如果节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2295; else goto if_merge_2296;

  if_then_2295:
  return;
  goto if_merge_2296;

  if_merge_2296:
  r2 = cn_var_构建器;
  r3 = cn_var_如果节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_如果节点;
  r6 = r5->真分支;
  r7 = r6 != 0;
  if (r7) goto if_then_2297; else goto if_merge_2298;

  if_then_2297:
  r8 = cn_var_构建器;
  r9 = cn_var_如果节点;
  r10 = r9->真分支;
  构建语句作用域(r8, r10);
  goto if_merge_2298;

  if_merge_2298:
  r11 = cn_var_如果节点;
  r12 = r11->假分支;
  r13 = r12 != 0;
  if (r13) goto if_then_2299; else goto if_merge_2300;

  if_then_2299:
  r14 = cn_var_构建器;
  r15 = cn_var_如果节点;
  r16 = r15->假分支;
  构建语句作用域(r14, r16);
  goto if_merge_2300;

  if_merge_2300:
  return;
}

void 构建当语句作用域(struct 作用域构建器* cn_var_构建器, struct 当语句* cn_var_当节点) {
  long long r1, r11;
  struct 当语句* r0;
  struct 作用域构建器* r2;
  struct 当语句* r3;
  struct 表达式节点* r4;
  struct 作用域构建器* r5;
  struct 符号表管理器* r6;
  struct 作用域构建器* r7;
  struct 符号表管理器* r8;
  struct 当语句* r9;
  struct 块语句* r10;
  struct 作用域构建器* r12;
  struct 当语句* r13;
  struct 块语句* r14;
  struct 作用域构建器* r15;
  struct 符号表管理器* r16;

  entry:
  r0 = cn_var_当节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2301; else goto if_merge_2302;

  if_then_2301:
  return;
  goto if_merge_2302;

  if_merge_2302:
  r2 = cn_var_构建器;
  r3 = cn_var_当节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = r5->符号表;
  进入作用域(r6, 作用域类型_循环作用域, "当循环", 0);
  r7 = cn_var_构建器;
  r8 = r7->符号表;
  设置循环作用域(r8);
  r9 = cn_var_当节点;
  r10 = r9->循环体;
  r11 = r10 != 0;
  if (r11) goto if_then_2303; else goto if_merge_2304;

  if_then_2303:
  r12 = cn_var_构建器;
  r13 = cn_var_当节点;
  r14 = r13->循环体;
  构建语句作用域(r12, r14);
  goto if_merge_2304;

  if_merge_2304:
  r15 = cn_var_构建器;
  r16 = r15->符号表;
  离开作用域(r16);
  return;
}

void 构建循环语句作用域(struct 作用域构建器* cn_var_构建器, struct 循环语句* cn_var_循环节点) {
  long long r1, r8, r14, r20, r26;
  struct 循环语句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 作用域构建器* r4;
  struct 符号表管理器* r5;
  struct 循环语句* r6;
  struct 语句节点* r7;
  struct 作用域构建器* r9;
  struct 循环语句* r10;
  struct 语句节点* r11;
  struct 循环语句* r12;
  struct 表达式节点* r13;
  struct 作用域构建器* r15;
  struct 循环语句* r16;
  struct 表达式节点* r17;
  struct 循环语句* r18;
  struct 表达式节点* r19;
  struct 作用域构建器* r21;
  struct 循环语句* r22;
  struct 表达式节点* r23;
  struct 循环语句* r24;
  struct 块语句* r25;
  struct 作用域构建器* r27;
  struct 循环语句* r28;
  struct 块语句* r29;
  struct 作用域构建器* r30;
  struct 符号表管理器* r31;

  entry:
  r0 = cn_var_循环节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2305; else goto if_merge_2306;

  if_then_2305:
  return;
  goto if_merge_2306;

  if_merge_2306:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  进入作用域(r3, 作用域类型_循环作用域, "循环", 0);
  r4 = cn_var_构建器;
  r5 = r4->符号表;
  设置循环作用域(r5);
  r6 = cn_var_循环节点;
  r7 = r6->初始化;
  r8 = r7 != 0;
  if (r8) goto if_then_2307; else goto if_merge_2308;

  if_then_2307:
  r9 = cn_var_构建器;
  r10 = cn_var_循环节点;
  r11 = r10->初始化;
  构建语句作用域(r9, r11);
  goto if_merge_2308;

  if_merge_2308:
  r12 = cn_var_循环节点;
  r13 = r12->条件;
  r14 = r13 != 0;
  if (r14) goto if_then_2309; else goto if_merge_2310;

  if_then_2309:
  r15 = cn_var_构建器;
  r16 = cn_var_循环节点;
  r17 = r16->条件;
  构建表达式作用域(r15, r17);
  goto if_merge_2310;

  if_merge_2310:
  r18 = cn_var_循环节点;
  r19 = r18->更新;
  r20 = r19 != 0;
  if (r20) goto if_then_2311; else goto if_merge_2312;

  if_then_2311:
  r21 = cn_var_构建器;
  r22 = cn_var_循环节点;
  r23 = r22->更新;
  构建表达式作用域(r21, r23);
  goto if_merge_2312;

  if_merge_2312:
  r24 = cn_var_循环节点;
  r25 = r24->循环体;
  r26 = r25 != 0;
  if (r26) goto if_then_2313; else goto if_merge_2314;

  if_then_2313:
  r27 = cn_var_构建器;
  r28 = cn_var_循环节点;
  r29 = r28->循环体;
  构建语句作用域(r27, r29);
  goto if_merge_2314;

  if_merge_2314:
  r30 = cn_var_构建器;
  r31 = r30->符号表;
  离开作用域(r31);
  return;
}

void 构建选择语句作用域(struct 作用域构建器* cn_var_构建器, struct 选择语句* cn_var_选择节点) {
  long long r1, r5, r7, r8, r12, r14, r15, r18;
  void* r11;
  struct 选择语句* r0;
  struct 作用域构建器* r2;
  struct 选择语句* r3;
  struct 表达式节点* r4;
  struct 选择语句* r6;
  struct 作用域构建器* r9;
  struct 选择语句* r10;
  void* r13;
  struct 选择语句* r16;
  struct 块语句* r17;
  struct 作用域构建器* r19;
  struct 选择语句* r20;
  struct 块语句* r21;

  entry:
  r0 = cn_var_选择节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2315; else goto if_merge_2316;

  if_then_2315:
  return;
  goto if_merge_2316;

  if_merge_2316:
  r2 = cn_var_构建器;
  r3 = cn_var_选择节点;
  r4 = r3->选择值;
  构建表达式作用域(r2, r4);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2317;

  for_cond_2317:
  r5 = cn_var_i_0;
  r6 = cn_var_选择节点;
  r7 = r6->情况个数;
  r8 = r5 < r7;
  if (r8) goto for_body_2318; else goto for_exit_2320;

  for_body_2318:
  r9 = cn_var_构建器;
  r10 = cn_var_选择节点;
  r11 = r10->情况分支列表;
  r12 = cn_var_i_0;
  r13 = (void**)cn_rt_array_get_element(r11, r12, 8);
  构建情况分支作用域(r9, r13);
  goto for_update_2319;

  for_update_2319:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_2317;

  for_exit_2320:
  r16 = cn_var_选择节点;
  r17 = r16->默认分支;
  r18 = r17 != 0;
  if (r18) goto if_then_2321; else goto if_merge_2322;

  if_then_2321:
  r19 = cn_var_构建器;
  r20 = cn_var_选择节点;
  r21 = r20->默认分支;
  构建块语句作用域(r19, r21);
  goto if_merge_2322;

  if_merge_2322:
  return;
}

void 构建情况分支作用域(struct 作用域构建器* cn_var_构建器, struct 情况分支* cn_var_情况节点) {
  long long r1, r7;
  struct 情况分支* r0;
  struct 作用域构建器* r2;
  struct 情况分支* r3;
  struct 表达式节点* r4;
  struct 情况分支* r5;
  struct 块语句* r6;
  struct 作用域构建器* r8;
  struct 情况分支* r9;
  struct 块语句* r10;

  entry:
  r0 = cn_var_情况节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2323; else goto if_merge_2324;

  if_then_2323:
  return;
  goto if_merge_2324;

  if_merge_2324:
  r2 = cn_var_构建器;
  r3 = cn_var_情况节点;
  r4 = r3->匹配值;
  构建表达式作用域(r2, r4);
  r5 = cn_var_情况节点;
  r6 = r5->语句体;
  r7 = r6 != 0;
  if (r7) goto if_then_2325; else goto if_merge_2326;

  if_then_2325:
  r8 = cn_var_构建器;
  r9 = cn_var_情况节点;
  r10 = r9->语句体;
  构建块语句作用域(r8, r10);
  goto if_merge_2326;

  if_merge_2326:
  return;
}

void 构建返回语句作用域(struct 作用域构建器* cn_var_构建器, struct 返回语句* cn_var_返回节点) {
  long long r1, r4;
  struct 返回语句* r0;
  struct 返回语句* r2;
  struct 表达式节点* r3;
  struct 作用域构建器* r5;
  struct 返回语句* r6;
  struct 表达式节点* r7;

  entry:
  r0 = cn_var_返回节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2327; else goto if_merge_2328;

  if_then_2327:
  return;
  goto if_merge_2328;

  if_merge_2328:
  r2 = cn_var_返回节点;
  r3 = r2->返回值;
  r4 = r3 != 0;
  if (r4) goto if_then_2329; else goto if_merge_2330;

  if_then_2329:
  r5 = cn_var_构建器;
  r6 = cn_var_返回节点;
  r7 = r6->返回值;
  构建表达式作用域(r5, r7);
  goto if_merge_2330;

  if_merge_2330:
  return;
}

void 构建尝试语句作用域(struct 作用域构建器* cn_var_构建器, struct 尝试语句* cn_var_尝试节点) {
  long long r1, r4, r8, r10, r11, r15, r17, r18, r21;
  void* r14;
  struct 尝试语句* r0;
  struct 尝试语句* r2;
  struct 块语句* r3;
  struct 作用域构建器* r5;
  struct 尝试语句* r6;
  struct 块语句* r7;
  struct 尝试语句* r9;
  struct 作用域构建器* r12;
  struct 尝试语句* r13;
  void* r16;
  struct 尝试语句* r19;
  struct 块语句* r20;
  struct 作用域构建器* r22;
  struct 尝试语句* r23;
  struct 块语句* r24;

  entry:
  r0 = cn_var_尝试节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2331; else goto if_merge_2332;

  if_then_2331:
  return;
  goto if_merge_2332;

  if_merge_2332:
  r2 = cn_var_尝试节点;
  r3 = r2->尝试块;
  r4 = r3 != 0;
  if (r4) goto if_then_2333; else goto if_merge_2334;

  if_then_2333:
  r5 = cn_var_构建器;
  r6 = cn_var_尝试节点;
  r7 = r6->尝试块;
  构建语句作用域(r5, r7);
  goto if_merge_2334;

  if_merge_2334:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2335;

  for_cond_2335:
  r8 = cn_var_i_0;
  r9 = cn_var_尝试节点;
  r10 = r9->捕获个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2336; else goto for_exit_2338;

  for_body_2336:
  r12 = cn_var_构建器;
  r13 = cn_var_尝试节点;
  r14 = r13->捕获子句列表;
  r15 = cn_var_i_0;
  r16 = (void**)cn_rt_array_get_element(r14, r15, 8);
  构建捕获子句作用域(r12, r16);
  goto for_update_2337;

  for_update_2337:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_2335;

  for_exit_2338:
  r19 = cn_var_尝试节点;
  r20 = r19->最终块;
  r21 = r20 != 0;
  if (r21) goto if_then_2339; else goto if_merge_2340;

  if_then_2339:
  r22 = cn_var_构建器;
  r23 = cn_var_尝试节点;
  r24 = r23->最终块;
  构建块语句作用域(r22, r24);
  goto if_merge_2340;

  if_merge_2340:
  return;
}

void 构建捕获子句作用域(struct 作用域构建器* cn_var_构建器, struct 捕获子句* cn_var_捕获节点) {
  long long r1, r6;
  struct 捕获子句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 捕获子句* r4;
  struct 块语句* r5;
  struct 作用域构建器* r7;
  struct 捕获子句* r8;
  struct 块语句* r9;
  struct 作用域构建器* r10;
  struct 符号表管理器* r11;

  entry:
  r0 = cn_var_捕获节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2341; else goto if_merge_2342;

  if_then_2341:
  return;
  goto if_merge_2342;

  if_merge_2342:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  进入作用域(r3, 作用域类型_块作用域, "捕获", 0);
  r4 = cn_var_捕获节点;
  r5 = r4->语句体;
  r6 = r5 != 0;
  if (r6) goto if_then_2343; else goto if_merge_2344;

  if_then_2343:
  r7 = cn_var_构建器;
  r8 = cn_var_捕获节点;
  r9 = r8->语句体;
  构建块语句作用域(r7, r9);
  goto if_merge_2344;

  if_merge_2344:
  r10 = cn_var_构建器;
  r11 = r10->符号表;
  离开作用域(r11);
  return;
}

void 构建表达式作用域(struct 作用域构建器* cn_var_构建器, struct 表达式节点* cn_var_表达式) {
  long long r1, r3, r4;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 作用域构建器* r13;
  struct 表达式节点* r14;
  struct 作用域构建器* r15;
  struct 表达式节点* r16;
  struct 作用域构建器* r17;
  struct 表达式节点* r18;
  struct 作用域构建器* r19;
  struct 表达式节点* r20;
  struct 作用域构建器* r21;
  struct 表达式节点* r22;
  struct 作用域构建器* r23;
  struct 表达式节点* r24;
  struct 作用域构建器* r25;
  struct 表达式节点* r26;
  struct 作用域构建器* r27;
  struct 表达式节点* r28;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;

  entry:
  r0 = cn_var_表达式;
  r1 = r0 == 0;
  if (r1) goto if_then_2345; else goto if_merge_2346;

  if_then_2345:
  return;
  goto if_merge_2346;

  if_merge_2346:
  r2 = cn_var_表达式;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = r4 == 节点类型_二元表达式;
  if (r5) goto case_body_2348; else goto switch_check_2357;

  switch_check_2357:
  r6 = r4 == 节点类型_一元表达式;
  if (r6) goto case_body_2349; else goto switch_check_2358;

  switch_check_2358:
  r7 = r4 == 节点类型_函数调用表达式;
  if (r7) goto case_body_2350; else goto switch_check_2359;

  switch_check_2359:
  r8 = r4 == 节点类型_成员访问表达式;
  if (r8) goto case_body_2351; else goto switch_check_2360;

  switch_check_2360:
  r9 = r4 == 节点类型_数组访问表达式;
  if (r9) goto case_body_2352; else goto switch_check_2361;

  switch_check_2361:
  r10 = r4 == 节点类型_赋值表达式;
  if (r10) goto case_body_2353; else goto switch_check_2362;

  switch_check_2362:
  r11 = r4 == 节点类型_三元表达式;
  if (r11) goto case_body_2354; else goto switch_check_2363;

  switch_check_2363:
  r12 = r4 == 节点类型_标识符表达式;
  if (r12) goto case_body_2355; else goto case_default_2356;

  case_body_2348:
  r13 = cn_var_构建器;
  r14 = cn_var_表达式;
  构建二元表达式作用域(r13, (struct 二元表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2349:
  r15 = cn_var_构建器;
  r16 = cn_var_表达式;
  构建一元表达式作用域(r15, (struct 一元表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2350:
  r17 = cn_var_构建器;
  r18 = cn_var_表达式;
  构建函数调用表达式作用域(r17, (struct 函数调用表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2351:
  r19 = cn_var_构建器;
  r20 = cn_var_表达式;
  构建成员访问表达式作用域(r19, (struct 成员访问表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2352:
  r21 = cn_var_构建器;
  r22 = cn_var_表达式;
  构建数组访问表达式作用域(r21, (struct 数组访问表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2353:
  r23 = cn_var_构建器;
  r24 = cn_var_表达式;
  构建赋值表达式作用域(r23, (struct 赋值表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2354:
  r25 = cn_var_构建器;
  r26 = cn_var_表达式;
  构建三元表达式作用域(r25, (struct 三元表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_body_2355:
  r27 = cn_var_构建器;
  r28 = cn_var_表达式;
  构建标识符表达式作用域(r27, (struct 标识符表达式*)cn_var_表达式);
  goto switch_merge_2347;
  goto switch_merge_2347;

  case_default_2356:
  goto switch_merge_2347;
  goto switch_merge_2347;

  switch_merge_2347:
  return;
}

void 构建二元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 二元表达式* cn_var_二元节点) {
  long long r1;
  struct 二元表达式* r0;
  struct 作用域构建器* r2;
  struct 二元表达式* r3;
  struct 表达式节点* r4;
  struct 作用域构建器* r5;
  struct 二元表达式* r6;
  struct 表达式节点* r7;

  entry:
  r0 = cn_var_二元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2364; else goto if_merge_2365;

  if_then_2364:
  return;
  goto if_merge_2365;

  if_merge_2365:
  r2 = cn_var_构建器;
  r3 = cn_var_二元节点;
  r4 = r3->左操作数;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_二元节点;
  r7 = r6->右操作数;
  构建表达式作用域(r5, r7);
  return;
}

void 构建一元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 一元表达式* cn_var_一元节点) {
  long long r1;
  struct 一元表达式* r0;
  struct 作用域构建器* r2;
  struct 一元表达式* r3;
  struct 表达式节点* r4;

  entry:
  r0 = cn_var_一元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2366; else goto if_merge_2367;

  if_then_2366:
  return;
  goto if_merge_2367;

  if_merge_2367:
  r2 = cn_var_构建器;
  r3 = cn_var_一元节点;
  r4 = r3->操作数;
  构建表达式作用域(r2, r4);
  return;
}

void 构建函数调用表达式作用域(struct 作用域构建器* cn_var_构建器, struct 函数调用表达式* cn_var_调用节点) {
  long long r1, r5, r7, r8, r12, r14, r15;
  void* r11;
  struct 函数调用表达式* r0;
  struct 作用域构建器* r2;
  struct 函数调用表达式* r3;
  struct 表达式节点* r4;
  struct 函数调用表达式* r6;
  struct 作用域构建器* r9;
  struct 函数调用表达式* r10;
  void* r13;

  entry:
  r0 = cn_var_调用节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2368; else goto if_merge_2369;

  if_then_2368:
  return;
  goto if_merge_2369;

  if_merge_2369:
  r2 = cn_var_构建器;
  r3 = cn_var_调用节点;
  r4 = r3->被调函数;
  构建表达式作用域(r2, r4);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2370;

  for_cond_2370:
  r5 = cn_var_i_0;
  r6 = cn_var_调用节点;
  r7 = r6->参数个数;
  r8 = r5 < r7;
  if (r8) goto for_body_2371; else goto for_exit_2373;

  for_body_2371:
  r9 = cn_var_构建器;
  r10 = cn_var_调用节点;
  r11 = r10->参数;
  r12 = cn_var_i_0;
  r13 = (void**)cn_rt_array_get_element(r11, r12, 8);
  构建表达式作用域(r9, r13);
  goto for_update_2372;

  for_update_2372:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_2370;

  for_exit_2373:
  return;
}

void 构建成员访问表达式作用域(struct 作用域构建器* cn_var_构建器, struct 成员访问表达式* cn_var_成员节点) {
  long long r1;
  struct 成员访问表达式* r0;
  struct 作用域构建器* r2;
  struct 成员访问表达式* r3;
  struct 表达式节点* r4;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2374; else goto if_merge_2375;

  if_then_2374:
  return;
  goto if_merge_2375;

  if_merge_2375:
  r2 = cn_var_构建器;
  r3 = cn_var_成员节点;
  r4 = r3->对象;
  构建表达式作用域(r2, r4);
  return;
}

void 构建数组访问表达式作用域(struct 作用域构建器* cn_var_构建器, struct 数组访问表达式* cn_var_数组节点) {
  long long r1;
  struct 数组访问表达式* r0;
  struct 作用域构建器* r2;
  struct 数组访问表达式* r3;
  struct 表达式节点* r4;
  struct 作用域构建器* r5;
  struct 数组访问表达式* r6;
  struct 表达式节点* r7;

  entry:
  r0 = cn_var_数组节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2376; else goto if_merge_2377;

  if_then_2376:
  return;
  goto if_merge_2377;

  if_merge_2377:
  r2 = cn_var_构建器;
  r3 = cn_var_数组节点;
  r4 = r3->数组;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_数组节点;
  r7 = r6->索引;
  构建表达式作用域(r5, r7);
  return;
}

void 构建赋值表达式作用域(struct 作用域构建器* cn_var_构建器, struct 赋值表达式* cn_var_赋值节点) {
  long long r1;
  struct 赋值表达式* r0;
  struct 作用域构建器* r2;
  struct 赋值表达式* r3;
  struct 表达式节点* r4;
  struct 作用域构建器* r5;
  struct 赋值表达式* r6;
  struct 表达式节点* r7;

  entry:
  r0 = cn_var_赋值节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2378; else goto if_merge_2379;

  if_then_2378:
  return;
  goto if_merge_2379;

  if_merge_2379:
  r2 = cn_var_构建器;
  r3 = cn_var_赋值节点;
  r4 = r3->目标;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_赋值节点;
  r7 = r6->值;
  构建表达式作用域(r5, r7);
  return;
}

void 构建三元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 三元表达式* cn_var_三元节点) {
  long long r1;
  struct 三元表达式* r0;
  struct 作用域构建器* r2;
  struct 三元表达式* r3;
  struct 表达式节点* r4;
  struct 作用域构建器* r5;
  struct 三元表达式* r6;
  struct 表达式节点* r7;
  struct 作用域构建器* r8;
  struct 三元表达式* r9;
  struct 表达式节点* r10;

  entry:
  r0 = cn_var_三元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2380; else goto if_merge_2381;

  if_then_2380:
  return;
  goto if_merge_2381;

  if_merge_2381:
  r2 = cn_var_构建器;
  r3 = cn_var_三元节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_三元节点;
  r7 = r6->真值;
  构建表达式作用域(r5, r7);
  r8 = cn_var_构建器;
  r9 = cn_var_三元节点;
  r10 = r9->假值;
  构建表达式作用域(r8, r10);
  return;
}

void 构建标识符表达式作用域(struct 作用域构建器* cn_var_构建器, struct 标识符表达式* cn_var_标识符节点) {
  long long r1, r7, r8;
  char* r5;
  char* r11;
  struct 标识符表达式* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 标识符表达式* r4;
  struct 符号* r6;
  struct 作用域构建器* r9;
  struct 标识符表达式* r10;
  struct 标识符表达式* r12;
  struct AST节点 r13;
  struct 源位置 r14;

  entry:
  r0 = cn_var_标识符节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2382; else goto if_merge_2383;

  if_then_2382:
  return;
  goto if_merge_2383;

  if_merge_2383:
  long long cn_var_符号指针_0;
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  r4 = cn_var_标识符节点;
  r5 = r4->名称;
  r6 = 查找符号(r3, r5);
  cn_var_符号指针_0 = r6;
  r7 = cn_var_符号指针_0;
  r8 = r7 == 0;
  if (r8) goto if_then_2384; else goto if_else_2385;

  if_then_2384:
  r9 = cn_var_构建器;
  r10 = cn_var_标识符节点;
  r11 = r10->名称;
  r12 = cn_var_标识符节点;
  r13 = r12->节点基类;
  r14 = r13.位置;
  报告未定义标识符错误(r9, r11, r14);
  goto if_merge_2386;

  if_else_2385:
  goto if_merge_2386;

  if_merge_2386:
  return;
}

void 报告重复符号错误(struct 作用域构建器* cn_var_构建器, char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r1, r2;
  struct 作用域构建器* r0;

  entry:
  r0 = cn_var_构建器;
  r1 = r0->错误计数;
  r2 = r1 + 1;
  return;
}

void 报告未定义标识符错误(struct 作用域构建器* cn_var_构建器, char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r1, r2;
  struct 作用域构建器* r0;

  entry:
  r0 = cn_var_构建器;
  r1 = r0->错误计数;
  r2 = r1 + 1;
  return;
}

