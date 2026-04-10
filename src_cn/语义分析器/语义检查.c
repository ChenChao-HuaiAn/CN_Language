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
struct 作用域构建器;
struct 作用域构建器 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
    long long 错误计数;
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
void 构建标识符表达式作用域(struct 作用域构建器*, struct 标识符表达式*);
void 构建三元表达式作用域(struct 作用域构建器*, struct 三元表达式*);
void 构建赋值表达式作用域(struct 作用域构建器*, struct 赋值表达式*);
void 构建数组访问表达式作用域(struct 作用域构建器*, struct 数组访问表达式*);
void 构建成员访问表达式作用域(struct 作用域构建器*, struct 成员访问表达式*);
void 构建函数调用表达式作用域(struct 作用域构建器*, struct 函数调用表达式*);
void 构建一元表达式作用域(struct 作用域构建器*, struct 一元表达式*);
void 构建二元表达式作用域(struct 作用域构建器*, struct 二元表达式*);
void 构建表达式作用域(struct 作用域构建器*, struct 表达式节点*);
void 构建捕获子句作用域(struct 作用域构建器*, struct 捕获子句*);
void 构建尝试语句作用域(struct 作用域构建器*, struct 尝试语句*);
void 构建返回语句作用域(struct 作用域构建器*, struct 返回语句*);
void 构建情况分支作用域(struct 作用域构建器*, struct 情况分支*);
void 构建选择语句作用域(struct 作用域构建器*, struct 选择语句*);
void 构建循环语句作用域(struct 作用域构建器*, struct 循环语句*);
void 构建当语句作用域(struct 作用域构建器*, struct 当语句*);
void 构建如果语句作用域(struct 作用域构建器*, struct 如果语句*);
void 构建块语句作用域(struct 作用域构建器*, struct 块语句*);
void 构建语句作用域(struct 作用域构建器*, struct 语句节点*);
void 构建导入声明作用域(struct 作用域构建器*, struct 导入声明*);
void 构建接口声明作用域(struct 作用域构建器*, struct 接口声明*);
void 构建类成员作用域(struct 作用域构建器*, struct 类成员*, struct 符号*, long long);
void 构建类声明作用域(struct 作用域构建器*, struct 类声明*);
void 构建枚举声明作用域(struct 作用域构建器*, struct 枚举声明*);
struct 符号标志 创建成员标志(struct 结构体成员*);
void 构建结构体声明作用域(struct 作用域构建器*, struct 结构体声明*);
void 构建变量声明作用域(struct 作用域构建器*, struct 变量声明*);
void 构建参数作用域(struct 作用域构建器*, struct 参数*);
struct 符号标志 创建符号标志(struct 函数声明*);
void 构建函数声明作用域(struct 作用域构建器*, struct 函数声明*);
void 构建声明作用域(struct 作用域构建器*, struct 声明节点*);
void 构建程序作用域(struct 作用域构建器*, struct 程序节点*);
void 销毁作用域构建器(struct 作用域构建器*);
struct 作用域构建器* 创建作用域构建器(struct 诊断集合*);
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;

// CN Language Global Struct Forward Declarations
struct 语义检查上下文;

// CN Language Global Struct Definitions
struct 语义检查上下文 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
    struct 作用域构建器* 作用域构建器;
    struct 符号* 当前函数;
    struct 类型信息* 期望返回类型;
    _Bool 有返回语句;
    long long 循环深度;
    long long 错误计数;
    long long 警告计数;
};

// Global Variables

// Forward Declarations
struct 语义检查上下文* 创建语义检查上下文(struct 诊断集合*);
void 销毁语义检查上下文(struct 语义检查上下文*);
_Bool 执行语义检查(struct 语义检查上下文*, struct 程序节点*);
void 检查程序(struct 语义检查上下文*, struct 程序节点*);
void 检查声明(struct 语义检查上下文*, struct 声明节点*);
void 检查函数声明(struct 语义检查上下文*, struct 函数声明*);
void 检查参数(struct 语义检查上下文*, struct 参数*);
void 检查变量声明(struct 语义检查上下文*, struct 变量声明*);
void 检查结构体声明(struct 语义检查上下文*, struct 结构体声明*);
void 检查枚举声明(struct 语义检查上下文*, struct 枚举声明*);
void 检查类声明(struct 语义检查上下文*, struct 类声明*);
void 检查类成员(struct 语义检查上下文*, struct 类成员*);
void 检查接口声明(struct 语义检查上下文*, struct 接口声明*);
void 检查语句(struct 语义检查上下文*, struct 语句节点*);
void 检查块语句(struct 语义检查上下文*, struct 块语句*);
void 检查如果语句(struct 语义检查上下文*, struct 如果语句*);
void 检查当语句(struct 语义检查上下文*, struct 当语句*);
void 检查循环语句(struct 语义检查上下文*, struct 循环语句*);
void 检查返回语句(struct 语义检查上下文*, struct 返回语句*);
void 检查中断语句(struct 语义检查上下文*, struct 中断语句*);
void 检查继续语句(struct 语义检查上下文*, struct 继续语句*);
void 检查选择语句(struct 语义检查上下文*, struct 选择语句*);
void 检查情况分支(struct 语义检查上下文*, struct 情况分支*);
void 检查尝试语句(struct 语义检查上下文*, struct 尝试语句*);
void 检查捕获子句(struct 语义检查上下文*, struct 捕获子句*);
void 检查抛出语句(struct 语义检查上下文*, struct 抛出语句*);
struct 类型信息* 检查表达式(struct 语义检查上下文*, struct 表达式节点*);
struct 类型信息* 检查二元表达式(struct 语义检查上下文*, struct 二元表达式*);
struct 类型信息* 检查一元表达式(struct 语义检查上下文*, struct 一元表达式*);
struct 类型信息* 检查字面量表达式(struct 语义检查上下文*, struct 字面量表达式*);
struct 类型信息* 检查标识符表达式(struct 语义检查上下文*, struct 标识符表达式*);
struct 类型信息* 检查函数调用表达式(struct 语义检查上下文*, struct 函数调用表达式*);
struct 类型信息* 检查成员访问表达式(struct 语义检查上下文*, struct 成员访问表达式*);
struct 类型信息* 检查数组访问表达式(struct 语义检查上下文*, struct 数组访问表达式*);
struct 类型信息* 检查赋值表达式(struct 语义检查上下文*, struct 赋值表达式*);
struct 类型信息* 检查三元表达式(struct 语义检查上下文*, struct 三元表达式*);
_Bool 是常量表达式(struct 表达式节点*);
void 报告错误(struct 语义检查上下文*, struct 源位置, enum 诊断错误码);
void 报告警告(struct 语义检查上下文*, struct 源位置, char*);
long long 语义检查上下文大小();

struct 语义检查上下文* 创建语义检查上下文(struct 诊断集合* cn_var_诊断器) {
  long long r0;
  void* r1;
  struct 诊断集合* r2;
  struct 作用域构建器* r3;
  struct 语义检查上下文* r4;
  struct 作用域构建器* r5;
  struct 符号表管理器* r6;
  struct 语义检查上下文* r7;
  struct 作用域构建器* r8;
  struct 类型推断上下文* r9;
  struct 诊断集合* r10;
  struct 语义检查上下文* r11;

  entry:
  struct 语义检查上下文* cn_var_上下文_0;
  r0 = 语义检查上下文大小();
  r1 = 分配清零内存(1, r0);
  cn_var_上下文_0 = (struct 语义检查上下文*)0;
  r2 = cn_var_诊断器;
  r3 = 创建作用域构建器(r2);
  r4 = cn_var_上下文_0;
  r5 = r4->作用域构建器;
  r6 = r5->符号表;
  r7 = cn_var_上下文_0;
  r8 = r7->作用域构建器;
  r9 = r8->类型上下文;
  r10 = cn_var_诊断器;
  r11 = cn_var_上下文_0;
  return r11;
}

void 销毁语义检查上下文(struct 语义检查上下文* cn_var_上下文) {
  long long r1;
  struct 语义检查上下文* r0;
  struct 语义检查上下文* r2;
  struct 作用域构建器* r3;
  struct 语义检查上下文* r4;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_2336; else goto if_merge_2337;

  if_then_2336:
  return;
  goto if_merge_2337;

  if_merge_2337:
  r2 = cn_var_上下文;
  r3 = r2->作用域构建器;
  销毁作用域构建器(r3);
  r4 = cn_var_上下文;
  释放内存(r4);
  return;
}

_Bool 执行语义检查(struct 语义检查上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r1, r8, r9;
  struct 程序节点* r0;
  struct 语义检查上下文* r2;
  struct 作用域构建器* r3;
  struct 程序节点* r4;
  struct 语义检查上下文* r5;
  struct 程序节点* r6;
  struct 语义检查上下文* r7;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2338; else goto if_merge_2339;

  if_then_2338:
  return 0;
  goto if_merge_2339;

  if_merge_2339:
  r2 = cn_var_上下文;
  r3 = r2->作用域构建器;
  r4 = cn_var_程序;
  构建程序作用域(r3, r4);
  r5 = cn_var_上下文;
  r6 = cn_var_程序;
  检查程序(r5, r6);
  r7 = cn_var_上下文;
  r8 = r7->错误计数;
  r9 = r8 == 0;
  return r9;
}

void 检查程序(struct 语义检查上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r1, r2, r4, r5, r9, r11, r12;
  struct 程序节点* r0;
  struct 程序节点* r3;
  struct 语义检查上下文* r6;
  struct 程序节点* r7;
  struct 声明节点列表* r8;
  void* r10;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2340; else goto if_merge_2341;

  if_then_2340:
  return;
  goto if_merge_2341;

  if_merge_2341:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2342;

  for_cond_2342:
  r2 = cn_var_i_0;
  r3 = cn_var_程序;
  r4 = r3->声明个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2343; else goto for_exit_2345;

  for_body_2343:
  r6 = cn_var_上下文;
  r7 = cn_var_程序;
  r8 = r7->声明列表;
  r9 = cn_var_i_0;
  r10 = *(void**)cn_rt_array_get_element(r8, r9, 8);
  检查声明(r6, r10);
  goto for_update_2344;

  for_update_2344:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2342;

  for_exit_2345:
  return;
}

void 检查声明(struct 语义检查上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r1, r3, r4, r5;
  struct 声明节点* r0;
  struct 声明节点* r2;
  struct 声明节点* r12;
  struct 语义检查上下文* r13;
  struct 函数声明* r14;
  struct 声明节点* r15;
  struct 语义检查上下文* r16;
  struct 变量声明* r17;
  struct 声明节点* r18;
  struct 语义检查上下文* r19;
  struct 结构体声明* r20;
  struct 声明节点* r21;
  struct 语义检查上下文* r22;
  struct 枚举声明* r23;
  struct 声明节点* r24;
  struct 语义检查上下文* r25;
  struct 类声明* r26;
  struct 声明节点* r27;
  struct 语义检查上下文* r28;
  struct 接口声明* r29;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;

  entry:
  r0 = cn_var_声明;
  r1 = r0 == 0;
  if (r1) goto if_then_2346; else goto if_merge_2347;

  if_then_2346:
  return;
  goto if_merge_2347;

  if_merge_2347:
  long long cn_var_节点类型值_0;
  r2 = cn_var_声明;
  r3 = r2->节点基类;
  r4 = r3.类型;
  cn_var_节点类型值_0 = r4;
  r5 = cn_var_节点类型值_0;
  r6 = r5 == 1;
  if (r6) goto case_body_2349; else goto switch_check_2356;

  switch_check_2356:
  r7 = r5 == 2;
  if (r7) goto case_body_2350; else goto switch_check_2357;

  switch_check_2357:
  r8 = r5 == 3;
  if (r8) goto case_body_2351; else goto switch_check_2358;

  switch_check_2358:
  r9 = r5 == 4;
  if (r9) goto case_body_2352; else goto switch_check_2359;

  switch_check_2359:
  r10 = r5 == 7;
  if (r10) goto case_body_2353; else goto switch_check_2360;

  switch_check_2360:
  r11 = r5 == 8;
  if (r11) goto case_body_2354; else goto case_default_2355;

  case_body_2349:
  struct 函数声明* cn_var_函数节点指针_1;
  r12 = cn_var_声明;
  cn_var_函数节点指针_1 = (struct 函数声明*)cn_var_声明;
  r13 = cn_var_上下文;
  r14 = cn_var_函数节点指针_1;
  检查函数声明(r13, r14);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_body_2350:
  struct 变量声明* cn_var_变量节点指针_2;
  r15 = cn_var_声明;
  cn_var_变量节点指针_2 = (struct 变量声明*)cn_var_声明;
  r16 = cn_var_上下文;
  r17 = cn_var_变量节点指针_2;
  检查变量声明(r16, r17);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_body_2351:
  struct 结构体声明* cn_var_结构体节点指针_3;
  r18 = cn_var_声明;
  cn_var_结构体节点指针_3 = (struct 结构体声明*)cn_var_声明;
  r19 = cn_var_上下文;
  r20 = cn_var_结构体节点指针_3;
  检查结构体声明(r19, r20);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_body_2352:
  struct 枚举声明* cn_var_枚举节点指针_4;
  r21 = cn_var_声明;
  cn_var_枚举节点指针_4 = (struct 枚举声明*)cn_var_声明;
  r22 = cn_var_上下文;
  r23 = cn_var_枚举节点指针_4;
  检查枚举声明(r22, r23);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_body_2353:
  struct 类声明* cn_var_类节点指针_5;
  r24 = cn_var_声明;
  cn_var_类节点指针_5 = (struct 类声明*)cn_var_声明;
  r25 = cn_var_上下文;
  r26 = cn_var_类节点指针_5;
  检查类声明(r25, r26);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_body_2354:
  struct 接口声明* cn_var_接口节点指针_6;
  r27 = cn_var_声明;
  cn_var_接口节点指针_6 = (struct 接口声明*)cn_var_声明;
  r28 = cn_var_上下文;
  r29 = cn_var_接口节点指针_6;
  检查接口声明(r28, r29);
  goto switch_merge_2348;
  goto switch_merge_2348;

  case_default_2355:
  goto switch_merge_2348;
  goto switch_merge_2348;

  switch_merge_2348:
  return;
}

void 检查函数声明(struct 语义检查上下文* cn_var_上下文, struct 函数声明* cn_var_函数节点) {
  long long r1, r18, r20, r21, r25, r27, r28, r31, r35, r38, r42, r43, r46, r49, r54, r55, r56;
  char* r11;
  void* r24;
  struct 函数声明* r0;
  struct 语义检查上下文* r2;
  struct 符号* r3;
  struct 语义检查上下文* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 语义检查上下文* r8;
  struct 符号表管理器* r9;
  struct 函数声明* r10;
  struct 符号* r12;
  struct 函数声明* r13;
  struct 类型节点* r14;
  struct 语义检查上下文* r15;
  struct 类型推断上下文* r16;
  struct 类型信息* r17;
  struct 函数声明* r19;
  struct 语义检查上下文* r22;
  struct 函数声明* r23;
  void* r26;
  struct 函数声明* r29;
  struct 块语句* r30;
  struct 语义检查上下文* r32;
  struct 函数声明* r33;
  struct 块语句* r34;
  struct 语义检查上下文* r36;
  struct 类型信息* r37;
  struct 语义检查上下文* r39;
  struct 类型信息* r40;
  struct 语义检查上下文* r44;
  struct 函数声明* r47;
  struct 语义检查上下文* r50;
  struct 函数声明* r51;
  _Bool r7;
  _Bool r45;
  _Bool r48;
  struct AST节点 r52;
  struct 源位置 r53;
  enum 类型种类 r41;

  entry:
  r0 = cn_var_函数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2361; else goto if_merge_2362;

  if_then_2361:
  return;
  goto if_merge_2362;

  if_merge_2362:
  long long cn_var_保存函数_0;
  r2 = cn_var_上下文;
  r3 = r2->当前函数;
  cn_var_保存函数_0 = r3;
  long long cn_var_保存返回类型_1;
  r4 = cn_var_上下文;
  r5 = r4->期望返回类型;
  cn_var_保存返回类型_1 = r5;
  long long cn_var_保存返回语句_2;
  r6 = cn_var_上下文;
  r7 = r6->有返回语句;
  cn_var_保存返回语句_2 = r7;
  r8 = cn_var_上下文;
  r9 = r8->符号表;
  r10 = cn_var_函数节点;
  r11 = r10->名称;
  r12 = 查找符号(r9, r11);
  r13 = cn_var_函数节点;
  r14 = r13->返回类型;
  r15 = cn_var_上下文;
  r16 = r15->类型上下文;
  r17 = 从类型节点推断(r14, r16);
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_2363;

  for_cond_2363:
  r18 = cn_var_i_3;
  r19 = cn_var_函数节点;
  r20 = r19->参数个数;
  r21 = r18 < r20;
  if (r21) goto for_body_2364; else goto for_exit_2366;

  for_body_2364:
  r22 = cn_var_上下文;
  r23 = cn_var_函数节点;
  r24 = r23->参数列表;
  r25 = cn_var_i_3;
  r26 = *(void**)cn_rt_array_get_element(r24, r25, 8);
  检查参数(r22, r26);
  goto for_update_2365;

  for_update_2365:
  r27 = cn_var_i_3;
  r28 = r27 + 1;
  cn_var_i_3 = r28;
  goto for_cond_2363;

  for_exit_2366:
  r29 = cn_var_函数节点;
  r30 = r29->函数体;
  r31 = r30 != 0;
  if (r31) goto if_then_2367; else goto if_merge_2368;

  if_then_2367:
  r32 = cn_var_上下文;
  r33 = cn_var_函数节点;
  r34 = r33->函数体;
  检查语句(r32, (struct 语句节点*)cn_var_函数节点.函数体);
  goto if_merge_2368;

  if_merge_2368:
  r36 = cn_var_上下文;
  r37 = r36->期望返回类型;
  r38 = r37 != 0;
  if (r38) goto logic_rhs_2371; else goto logic_merge_2372;

  if_then_2369:
  r44 = cn_var_上下文;
  r45 = r44->有返回语句;
  r46 = r45 == 0;
  if (r46) goto logic_rhs_2375; else goto logic_merge_2376;

  if_merge_2370:
  r54 = cn_var_保存函数_0;
  r55 = cn_var_保存返回类型_1;
  r56 = cn_var_保存返回语句_2;

  logic_rhs_2371:
  r39 = cn_var_上下文;
  r40 = r39->期望返回类型;
  r41 = r40->种类;
  r42 = r41 != 0;
  goto logic_merge_2372;

  logic_merge_2372:
  if (r42) goto if_then_2369; else goto if_merge_2370;

  if_then_2373:
  r50 = cn_var_上下文;
  r51 = cn_var_函数节点;
  r52 = r51->节点基类;
  r53 = r52.位置;
  报告错误(r50, r53, 28);
  goto if_merge_2374;

  if_merge_2374:
  goto if_merge_2370;

  logic_rhs_2375:
  r47 = cn_var_函数节点;
  r48 = r47->是抽象;
  r49 = r48 == 0;
  goto logic_merge_2376;

  logic_merge_2376:
  if (r49) goto if_then_2373; else goto if_merge_2374;
  return;
}

void 检查参数(struct 语义检查上下文* cn_var_上下文, struct 参数* cn_var_参数节点) {
  long long r0, r2, r5, r11, r12, r15;
  struct 参数* r1;
  struct 参数* r3;
  struct 类型节点* r4;
  struct 参数* r6;
  struct 类型节点* r7;
  struct 语义检查上下文* r8;
  struct 类型推断上下文* r9;
  struct 类型信息* r10;
  struct 语义检查上下文* r13;
  struct 参数* r14;

  entry:
  r1 = cn_var_参数节点;
  r2 = r1 == 0;
  if (r2) goto logic_merge_2380; else goto logic_rhs_2379;

  if_then_2377:
  return;
  goto if_merge_2378;

  if_merge_2378:
  long long cn_var_类型_0;
  r6 = cn_var_参数节点;
  r7 = r6->类型;
  r8 = cn_var_上下文;
  r9 = r8->类型上下文;
  r10 = 从类型节点推断(r7, r9);
  cn_var_类型_0 = r10;
  r11 = cn_var_类型_0;
  r12 = r11 == 0;
  if (r12) goto if_then_2381; else goto if_merge_2382;

  logic_rhs_2379:
  r3 = cn_var_参数节点;
  r4 = r3->类型;
  r5 = r4 == 0;
  goto logic_merge_2380;

  logic_merge_2380:
  if (r5) goto if_then_2377; else goto if_merge_2378;

  if_then_2381:
  r13 = cn_var_上下文;
  r14 = cn_var_参数节点;
  r15 = r14->定义位置;
  报告错误(r13, r15, 22);
  goto if_merge_2382;

  if_merge_2382:
  return;
}

void 检查变量声明(struct 语义检查上下文* cn_var_上下文, struct 变量声明* cn_var_变量节点) {
  long long r1, r6, r14, r23, r27, r32, r35, r39, r43, r44, r51, r58, r64, r65, r70, r71, r77, r78, r79, r80, r82, r83, r88, r89, r90, r97, r98;
  char* r42;
  char* r95;
  struct 变量声明* r0;
  struct 变量声明* r2;
  struct 变量声明* r4;
  struct 表达式节点* r5;
  struct 语义检查上下文* r7;
  struct 变量声明* r8;
  struct 变量声明* r11;
  struct 表达式节点* r12;
  struct 语义检查上下文* r15;
  struct 变量声明* r16;
  struct 变量声明* r19;
  struct 变量声明* r21;
  struct 表达式节点* r22;
  struct 变量声明* r24;
  struct 表达式节点* r25;
  struct 语义检查上下文* r28;
  struct 变量声明* r29;
  struct 变量声明* r33;
  struct 类型节点* r34;
  struct 变量声明* r36;
  struct 类型节点* r37;
  struct 变量声明* r40;
  struct 类型节点* r41;
  struct 语义检查上下文* r45;
  struct 变量声明* r46;
  struct 变量声明* r49;
  struct 表达式节点* r50;
  struct 语义检查上下文* r52;
  struct 变量声明* r53;
  struct 表达式节点* r54;
  struct 类型信息* r55;
  struct 变量声明* r56;
  struct 类型节点* r57;
  struct 变量声明* r59;
  struct 类型节点* r60;
  struct 语义检查上下文* r61;
  struct 类型推断上下文* r62;
  struct 类型信息* r63;
  struct 语义检查上下文* r66;
  struct 变量声明* r67;
  struct 变量声明* r72;
  struct 类型节点* r73;
  struct 语义检查上下文* r74;
  struct 类型推断上下文* r75;
  struct 类型信息* r76;
  struct 语义检查上下文* r84;
  struct 变量声明* r85;
  struct 类型节点* r91;
  struct 语义检查上下文* r92;
  struct 符号表管理器* r93;
  struct 变量声明* r94;
  struct 符号* r96;
  struct 变量声明* r99;
  struct 类型节点* r100;
  _Bool r3;
  _Bool r13;
  _Bool r20;
  _Bool r26;
  struct AST节点 r9;
  struct 源位置 r10;
  struct AST节点 r17;
  struct 源位置 r18;
  struct AST节点 r30;
  struct 源位置 r31;
  struct AST节点 r47;
  struct 源位置 r48;
  struct AST节点 r68;
  struct 源位置 r69;
  enum 类型兼容性 r81;
  struct AST节点 r86;
  struct 源位置 r87;
  enum 节点类型 r38;

  entry:
  r0 = cn_var_变量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2383; else goto if_merge_2384;

  if_then_2383:
  return;
  goto if_merge_2384;

  if_merge_2384:
  r2 = cn_var_变量节点;
  r3 = r2->是常量;
  if (r3) goto if_then_2385; else goto if_merge_2386;

  if_then_2385:
  r4 = cn_var_变量节点;
  r5 = r4->初始值;
  r6 = r5 == 0;
  if (r6) goto if_then_2387; else goto if_else_2388;

  if_merge_2386:
  r19 = cn_var_变量节点;
  r20 = r19->是静态;
  if (r20) goto if_then_2392; else goto if_merge_2393;

  if_then_2387:
  r7 = cn_var_上下文;
  r8 = cn_var_变量节点;
  r9 = r8->节点基类;
  r10 = r9.位置;
  报告错误(r7, r10, 34);
  goto if_merge_2389;

  if_else_2388:
  r11 = cn_var_变量节点;
  r12 = r11->初始值;
  r13 = 是常量表达式(r12);
  r14 = r13 == 0;
  if (r14) goto if_then_2390; else goto if_merge_2391;

  if_merge_2389:
  goto if_merge_2386;

  if_then_2390:
  r15 = cn_var_上下文;
  r16 = cn_var_变量节点;
  r17 = r16->节点基类;
  r18 = r17.位置;
  报告错误(r15, r18, 35);
  goto if_merge_2391;

  if_merge_2391:
  goto if_merge_2389;

  if_then_2392:
  r21 = cn_var_变量节点;
  r22 = r21->初始值;
  r23 = r22 != 0;
  if (r23) goto if_then_2394; else goto if_merge_2395;

  if_merge_2393:
  long long cn_var_推断类型_0;
  cn_var_推断类型_0 = 0;
  r49 = cn_var_变量节点;
  r50 = r49->初始值;
  r51 = r50 != 0;
  if (r51) goto if_then_2404; else goto if_merge_2405;

  if_then_2394:
  r24 = cn_var_变量节点;
  r25 = r24->初始值;
  r26 = 是常量表达式(r25);
  r27 = r26 == 0;
  if (r27) goto if_then_2396; else goto if_merge_2397;

  if_merge_2395:
  r33 = cn_var_变量节点;
  r34 = r33->类型;
  r35 = r34 != 0;
  if (r35) goto logic_rhs_2400; else goto logic_merge_2401;

  if_then_2396:
  r28 = cn_var_上下文;
  r29 = cn_var_变量节点;
  r30 = r29->节点基类;
  r31 = r30.位置;
  报告错误(r28, r31, 38);
  goto if_merge_2397;

  if_merge_2397:
  goto if_merge_2395;

  if_then_2398:
  r40 = cn_var_变量节点;
  r41 = r40->类型;
  r42 = r41->名称;
  r43 = 比较字符串(r42, "空类型");
  r44 = r43 == 0;
  if (r44) goto if_then_2402; else goto if_merge_2403;

  if_merge_2399:
  goto if_merge_2393;

  logic_rhs_2400:
  r36 = cn_var_变量节点;
  r37 = r36->类型;
  r38 = r37->类型;
  r39 = r38 == 36;
  goto logic_merge_2401;

  logic_merge_2401:
  if (r39) goto if_then_2398; else goto if_merge_2399;

  if_then_2402:
  r45 = cn_var_上下文;
  r46 = cn_var_变量节点;
  r47 = r46->节点基类;
  r48 = r47.位置;
  报告错误(r45, r48, 39);
  goto if_merge_2403;

  if_merge_2403:
  goto if_merge_2399;

  if_then_2404:
  r52 = cn_var_上下文;
  r53 = cn_var_变量节点;
  r54 = r53->初始值;
  r55 = 检查表达式(r52, r54);
  cn_var_推断类型_0 = r55;
  goto if_merge_2405;

  if_merge_2405:
  r56 = cn_var_变量节点;
  r57 = r56->类型;
  r58 = r57 != 0;
  if (r58) goto if_then_2406; else goto if_else_2407;

  if_then_2406:
  long long cn_var_类型_1;
  r59 = cn_var_变量节点;
  r60 = r59->类型;
  r61 = cn_var_上下文;
  r62 = r61->类型上下文;
  r63 = 从类型节点推断(r60, r62);
  cn_var_类型_1 = r63;
  r64 = cn_var_类型_1;
  r65 = r64 == 0;
  if (r65) goto if_then_2409; else goto if_merge_2410;

  if_else_2407:
  r88 = cn_var_推断类型_0;
  r89 = r88 != 0;
  if (r89) goto if_then_2417; else goto if_merge_2418;

  if_merge_2408:

  if_then_2409:
  r66 = cn_var_上下文;
  r67 = cn_var_变量节点;
  r68 = r67->节点基类;
  r69 = r68.位置;
  报告错误(r66, r69, 22);
  goto if_merge_2410;

  if_merge_2410:
  r70 = cn_var_推断类型_0;
  r71 = r70 != 0;
  if (r71) goto if_then_2411; else goto if_merge_2412;

  if_then_2411:
  long long cn_var_变量类型_2;
  r72 = cn_var_变量节点;
  r73 = r72->类型;
  r74 = cn_var_上下文;
  r75 = r74->类型上下文;
  r76 = 从类型节点推断(r73, r75);
  cn_var_变量类型_2 = r76;
  r77 = cn_var_变量类型_2;
  r78 = r77 != 0;
  if (r78) goto if_then_2413; else goto if_merge_2414;

  if_merge_2412:
  goto if_merge_2408;

  if_then_2413:
  long long cn_var_兼容性_3;
  r79 = cn_var_推断类型_0;
  r80 = cn_var_变量类型_2;
  r81 = 检查类型兼容性(r79, r80);
  cn_var_兼容性_3 = r81;
  r82 = cn_var_兼容性_3;
  r83 = r82 == 3;
  if (r83) goto if_then_2415; else goto if_merge_2416;

  if_merge_2414:
  goto if_merge_2412;

  if_then_2415:
  r84 = cn_var_上下文;
  r85 = cn_var_变量节点;
  r86 = r85->节点基类;
  r87 = r86.位置;
  报告错误(r84, r87, 23);
  goto if_merge_2416;

  if_merge_2416:
  goto if_merge_2414;

  if_then_2417:
  r90 = cn_var_推断类型_0;
  r91 = 类型信息转类型节点(r90);
  long long cn_var_符号指针_4;
  r92 = cn_var_上下文;
  r93 = r92->符号表;
  r94 = cn_var_变量节点;
  r95 = r94->名称;
  r96 = 查找符号(r93, r95);
  cn_var_符号指针_4 = r96;
  r97 = cn_var_符号指针_4;
  r98 = r97 != 0;
  if (r98) goto if_then_2419; else goto if_merge_2420;

  if_merge_2418:
  goto if_merge_2408;

  if_then_2419:
  r99 = cn_var_变量节点;
  r100 = r99->类型;
  goto if_merge_2420;

  if_merge_2420:
  goto if_merge_2418;
  return;
}

void 检查结构体声明(struct 语义检查上下文* cn_var_上下文, struct 结构体声明* cn_var_结构体节点) {
  long long r1, r2, r4, r5, r8, r10, r11, r12, r13, r14, r18, r19, r24, r25;
  void* r7;
  struct 结构体声明* r0;
  struct 结构体声明* r3;
  struct 结构体声明* r6;
  void* r9;
  struct 语义检查上下文* r15;
  struct 类型推断上下文* r16;
  struct 类型信息* r17;
  struct 语义检查上下文* r20;
  struct 结构体声明* r21;
  struct AST节点 r22;
  struct 源位置 r23;

  entry:
  r0 = cn_var_结构体节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2421; else goto if_merge_2422;

  if_then_2421:
  return;
  goto if_merge_2422;

  if_merge_2422:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2423;

  for_cond_2423:
  r2 = cn_var_i_0;
  r3 = cn_var_结构体节点;
  r4 = r3->成员个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2424; else goto for_exit_2426;

  for_body_2424:
  long long cn_var_成员_1;
  r6 = cn_var_结构体节点;
  r7 = r6->成员;
  r8 = cn_var_i_0;
  r9 = *(void**)cn_rt_array_get_element(r7, r8, 8);
  cn_var_成员_1 = r9;
  r10 = cn_var_成员_1;
  r11 = r10.类型;
  r12 = r11 != 0;
  if (r12) goto if_then_2427; else goto if_merge_2428;

  for_update_2425:
  r24 = cn_var_i_0;
  r25 = r24 + 1;
  cn_var_i_0 = r25;
  goto for_cond_2423;

  for_exit_2426:

  if_then_2427:
  long long cn_var_类型_2;
  r13 = cn_var_成员_1;
  r14 = r13.类型;
  r15 = cn_var_上下文;
  r16 = r15->类型上下文;
  r17 = 从类型节点推断(r14, r16);
  cn_var_类型_2 = r17;
  r18 = cn_var_类型_2;
  r19 = r18 == 0;
  if (r19) goto if_then_2429; else goto if_merge_2430;

  if_merge_2428:
  goto for_update_2425;

  if_then_2429:
  r20 = cn_var_上下文;
  r21 = cn_var_结构体节点;
  r22 = r21->节点基类;
  r23 = r22.位置;
  报告错误(r20, r23, 22);
  goto if_merge_2430;

  if_merge_2430:
  goto if_merge_2428;
  return;
}

void 检查枚举声明(struct 语义检查上下文* cn_var_上下文, struct 枚举声明* cn_var_枚举节点) {
  long long r1, r2, r4, r5, r6, r7, r8, r10, r11, r14, r16, r19, r21, r22, r23, r28, r29, r30, r31;
  void* r13;
  void* r18;
  struct 枚举声明* r0;
  struct 枚举声明* r3;
  struct 枚举声明* r9;
  struct 枚举声明* r12;
  void* r15;
  struct 枚举声明* r17;
  void* r20;
  struct 语义检查上下文* r24;
  struct 枚举声明* r25;
  struct AST节点 r26;
  struct 源位置 r27;

  entry:
  r0 = cn_var_枚举节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2431; else goto if_merge_2432;

  if_then_2431:
  return;
  goto if_merge_2432;

  if_merge_2432:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2433;

  for_cond_2433:
  r2 = cn_var_i_0;
  r3 = cn_var_枚举节点;
  r4 = r3->成员个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2434; else goto for_exit_2436;

  for_body_2434:
  long long cn_var_j_1;
  r6 = cn_var_i_0;
  r7 = r6 + 1;
  cn_var_j_1 = r7;
  goto for_cond_2437;

  for_update_2435:
  r30 = cn_var_i_0;
  r31 = r30 + 1;
  cn_var_i_0 = r31;
  goto for_cond_2433;

  for_exit_2436:

  for_cond_2437:
  r8 = cn_var_j_1;
  r9 = cn_var_枚举节点;
  r10 = r9->成员个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2438; else goto for_exit_2440;

  for_body_2438:
  r12 = cn_var_枚举节点;
  r13 = r12->成员;
  r14 = cn_var_i_0;
  r15 = *(void**)cn_rt_array_get_element(r13, r14, 8);
  r16 = r15->名称;
  r17 = cn_var_枚举节点;
  r18 = r17->成员;
  r19 = cn_var_j_1;
  r20 = *(void**)cn_rt_array_get_element(r18, r19, 8);
  r21 = r20->名称;
  r22 = 比较字符串(r16, r21);
  r23 = r22 == 0;
  if (r23) goto if_then_2441; else goto if_merge_2442;

  for_update_2439:
  r28 = cn_var_j_1;
  r29 = r28 + 1;
  cn_var_j_1 = r29;
  goto for_cond_2437;

  for_exit_2440:
  goto for_update_2435;

  if_then_2441:
  r24 = cn_var_上下文;
  r25 = cn_var_枚举节点;
  r26 = r25->节点基类;
  r27 = r26.位置;
  报告错误(r24, r27, 21);
  goto if_merge_2442;

  if_merge_2442:
  goto for_update_2439;
  return;
}

void 检查类声明(struct 语义检查上下文* cn_var_上下文, struct 类声明* cn_var_类节点) {
  long long r1, r2, r5, r8, r9, r15, r16, r17, r18, r19, r20, r25, r27, r28, r33, r36, r37, r38, r39, r40, r41, r46, r47, r48, r50, r51, r55, r57, r58;
  char* r4;
  char* r7;
  char* r13;
  void* r32;
  void* r54;
  struct 类声明* r0;
  struct 类声明* r3;
  struct 类声明* r6;
  struct 语义检查上下文* r10;
  struct 符号表管理器* r11;
  struct 类声明* r12;
  struct 符号* r14;
  struct 语义检查上下文* r21;
  struct 类声明* r22;
  struct 类声明* r26;
  struct 语义检查上下文* r29;
  struct 符号表管理器* r30;
  struct 类声明* r31;
  void* r34;
  struct 符号* r35;
  struct 语义检查上下文* r42;
  struct 类声明* r43;
  struct 类声明* r49;
  struct 语义检查上下文* r52;
  struct 类声明* r53;
  void* r56;
  struct AST节点 r23;
  struct 源位置 r24;
  struct AST节点 r44;
  struct 源位置 r45;

  entry:
  r0 = cn_var_类节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2443; else goto if_merge_2444;

  if_then_2443:
  return;
  goto if_merge_2444;

  if_merge_2444:
  r3 = cn_var_类节点;
  r4 = r3->基类名称;
  r5 = r4 != 0;
  if (r5) goto logic_rhs_2447; else goto logic_merge_2448;

  if_then_2445:
  long long cn_var_父类符号_0;
  r10 = cn_var_上下文;
  r11 = r10->符号表;
  r12 = cn_var_类节点;
  r13 = r12->基类名称;
  r14 = 查找符号(r11, r13);
  cn_var_父类符号_0 = r14;
  r16 = cn_var_父类符号_0;
  r17 = r16 == 0;
  if (r17) goto logic_merge_2452; else goto logic_rhs_2451;

  if_merge_2446:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2453;

  logic_rhs_2447:
  r6 = cn_var_类节点;
  r7 = r6->基类名称;
  r8 = 获取字符串长度(r7);
  r9 = r8 > 0;
  goto logic_merge_2448;

  logic_merge_2448:
  if (r9) goto if_then_2445; else goto if_merge_2446;

  if_then_2449:
  r21 = cn_var_上下文;
  r22 = cn_var_类节点;
  r23 = r22->节点基类;
  r24 = r23.位置;
  报告错误(r21, r24, 22);
  goto if_merge_2450;

  if_merge_2450:
  goto if_merge_2446;

  logic_rhs_2451:
  r18 = cn_var_父类符号_0;
  r19 = r18.种类;
  r20 = r19 != 6;
  goto logic_merge_2452;

  logic_merge_2452:
  if (r20) goto if_then_2449; else goto if_merge_2450;

  for_cond_2453:
  r25 = cn_var_i_1;
  r26 = cn_var_类节点;
  r27 = r26->接口个数;
  r28 = r25 < r27;
  if (r28) goto for_body_2454; else goto for_exit_2456;

  for_body_2454:
  long long cn_var_接口符号_2;
  r29 = cn_var_上下文;
  r30 = r29->符号表;
  r31 = cn_var_类节点;
  r32 = r31->实现接口;
  r33 = cn_var_i_1;
  r34 = *(void**)cn_rt_array_get_element(r32, r33, 8);
  r35 = 查找符号(r30, r34);
  cn_var_接口符号_2 = r35;
  r37 = cn_var_接口符号_2;
  r38 = r37 == 0;
  if (r38) goto logic_merge_2460; else goto logic_rhs_2459;

  for_update_2455:
  r46 = cn_var_i_1;
  r47 = r46 + 1;
  cn_var_i_1 = r47;
  goto for_cond_2453;

  for_exit_2456:
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_2461;

  if_then_2457:
  r42 = cn_var_上下文;
  r43 = cn_var_类节点;
  r44 = r43->节点基类;
  r45 = r44.位置;
  报告错误(r42, r45, 22);
  goto if_merge_2458;

  if_merge_2458:
  goto for_update_2455;

  logic_rhs_2459:
  r39 = cn_var_接口符号_2;
  r40 = r39.种类;
  r41 = r40 != 7;
  goto logic_merge_2460;

  logic_merge_2460:
  if (r41) goto if_then_2457; else goto if_merge_2458;

  for_cond_2461:
  r48 = cn_var_i_3;
  r49 = cn_var_类节点;
  r50 = r49->成员个数;
  r51 = r48 < r50;
  if (r51) goto for_body_2462; else goto for_exit_2464;

  for_body_2462:
  r52 = cn_var_上下文;
  r53 = cn_var_类节点;
  r54 = r53->成员;
  r55 = cn_var_i_3;
  r56 = *(void**)cn_rt_array_get_element(r54, r55, 8);
  检查类成员(r52, r56);
  goto for_update_2463;

  for_update_2463:
  r57 = cn_var_i_3;
  r58 = r57 + 1;
  cn_var_i_3 = r58;
  goto for_cond_2461;

  for_exit_2464:
  return;
}

void 检查类成员(struct 语义检查上下文* cn_var_上下文, struct 类成员* cn_var_成员节点) {
  long long r1, r4, r10;
  struct 类成员* r0;
  struct 类成员* r2;
  struct 语义检查上下文* r5;
  struct 类成员* r6;
  struct 变量声明* r7;
  struct 类成员* r8;
  struct 语义检查上下文* r11;
  struct 类成员* r12;
  struct 函数声明* r13;
  enum 节点类型 r3;
  enum 节点类型 r9;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2465; else goto if_merge_2466;

  if_then_2465:
  return;
  goto if_merge_2466;

  if_merge_2466:
  r2 = cn_var_成员节点;
  r3 = r2->类型;
  r4 = r3 == 2;
  if (r4) goto if_then_2467; else goto if_else_2468;

  if_then_2467:
  r5 = cn_var_上下文;
  r6 = cn_var_成员节点;
  r7 = r6->字段;
  检查变量声明(r5, r7);
  goto if_merge_2469;

  if_else_2468:
  r8 = cn_var_成员节点;
  r9 = r8->类型;
  r10 = r9 == 1;
  if (r10) goto if_then_2470; else goto if_merge_2471;

  if_merge_2469:

  if_then_2470:
  r11 = cn_var_上下文;
  r12 = cn_var_成员节点;
  r13 = r12->方法;
  检查函数声明(r11, r13);
  goto if_merge_2471;

  if_merge_2471:
  goto if_merge_2469;
  return;
}

void 检查接口声明(struct 语义检查上下文* cn_var_上下文, struct 接口声明* cn_var_接口节点) {
  long long r1, r2, r4, r5, r7, r8, r10, r11, r12, r13, r14, r15, r16, r18, r19, r20, r21, r22, r24, r28, r29, r34, r35, r36, r37, r38, r39, r40, r44, r45, r50, r51;
  struct 接口声明* r0;
  struct 接口声明* r3;
  struct 接口声明* r6;
  void* r9;
  void* r17;
  void* r23;
  struct 语义检查上下文* r25;
  struct 类型推断上下文* r26;
  struct 类型信息* r27;
  struct 语义检查上下文* r30;
  struct 接口声明* r31;
  struct 语义检查上下文* r41;
  struct 类型推断上下文* r42;
  struct 类型信息* r43;
  struct 语义检查上下文* r46;
  struct 接口声明* r47;
  struct AST节点 r32;
  struct 源位置 r33;
  struct AST节点 r48;
  struct 源位置 r49;

  entry:
  r0 = cn_var_接口节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2472; else goto if_merge_2473;

  if_then_2472:
  return;
  goto if_merge_2473;

  if_merge_2473:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2474;

  for_cond_2474:
  r2 = cn_var_i_0;
  r3 = cn_var_接口节点;
  r4 = r3->方法个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2475; else goto for_exit_2477;

  for_body_2475:
  long long cn_var_方法_1;
  r6 = cn_var_接口节点;
  r7 = r6->方法列表;
  r8 = cn_var_i_0;
  r9 = *(void**)cn_rt_array_get_element(r7, r8, 8);
  cn_var_方法_1 = r9;
  long long cn_var_j_2;
  cn_var_j_2 = 0;
  goto for_cond_2478;

  for_update_2476:
  r50 = cn_var_i_0;
  r51 = r50 + 1;
  cn_var_i_0 = r51;
  goto for_cond_2474;

  for_exit_2477:

  for_cond_2478:
  r10 = cn_var_j_2;
  r11 = cn_var_方法_1;
  r12 = r11.参数个数;
  r13 = r10 < r12;
  if (r13) goto for_body_2479; else goto for_exit_2481;

  for_body_2479:
  r14 = cn_var_方法_1;
  r15 = r14.参数列表;
  r16 = cn_var_j_2;
  r17 = *(void**)cn_rt_array_get_element(r15, r16, 8);
  r18 = r17->类型;
  r19 = r18 != 0;
  if (r19) goto if_then_2482; else goto if_merge_2483;

  for_update_2480:
  r34 = cn_var_j_2;
  r35 = r34 + 1;
  cn_var_j_2 = r35;
  goto for_cond_2478;

  for_exit_2481:
  r36 = cn_var_方法_1;
  r37 = r36.返回类型;
  r38 = r37 != 0;
  if (r38) goto if_then_2486; else goto if_merge_2487;

  if_then_2482:
  long long cn_var_类型_3;
  r20 = cn_var_方法_1;
  r21 = r20.参数列表;
  r22 = cn_var_j_2;
  r23 = *(void**)cn_rt_array_get_element(r21, r22, 8);
  r24 = r23->类型;
  r25 = cn_var_上下文;
  r26 = r25->类型上下文;
  r27 = 从类型节点推断(r24, r26);
  cn_var_类型_3 = r27;
  r28 = cn_var_类型_3;
  r29 = r28 == 0;
  if (r29) goto if_then_2484; else goto if_merge_2485;

  if_merge_2483:
  goto for_update_2480;

  if_then_2484:
  r30 = cn_var_上下文;
  r31 = cn_var_接口节点;
  r32 = r31->节点基类;
  r33 = r32.位置;
  报告错误(r30, r33, 22);
  goto if_merge_2485;

  if_merge_2485:
  goto if_merge_2483;

  if_then_2486:
  long long cn_var_类型_4;
  r39 = cn_var_方法_1;
  r40 = r39.返回类型;
  r41 = cn_var_上下文;
  r42 = r41->类型上下文;
  r43 = 从类型节点推断(r40, r42);
  cn_var_类型_4 = r43;
  r44 = cn_var_类型_4;
  r45 = r44 == 0;
  if (r45) goto if_then_2488; else goto if_merge_2489;

  if_merge_2487:
  goto for_update_2476;

  if_then_2488:
  r46 = cn_var_上下文;
  r47 = cn_var_接口节点;
  r48 = r47->节点基类;
  r49 = r48.位置;
  报告错误(r46, r49, 22);
  goto if_merge_2489;

  if_merge_2489:
  goto if_merge_2487;
  return;
}

void 检查语句(struct 语义检查上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r3, r4;
  struct 语句节点* r0;
  struct 语句节点* r2;
  struct 语义检查上下文* r16;
  struct 语句节点* r17;
  struct 语义检查上下文* r18;
  struct 语句节点* r19;
  struct 语义检查上下文* r20;
  struct 语句节点* r21;
  struct 语义检查上下文* r22;
  struct 语句节点* r23;
  struct 语义检查上下文* r24;
  struct 语句节点* r25;
  struct 语义检查上下文* r26;
  struct 语句节点* r27;
  struct 语义检查上下文* r28;
  struct 语句节点* r29;
  struct 语义检查上下文* r30;
  struct 语句节点* r31;
  struct 语义检查上下文* r32;
  struct 语句节点* r33;
  struct 表达式节点* r34;
  struct 类型信息* r35;
  struct 语义检查上下文* r36;
  struct 语句节点* r37;
  struct 语义检查上下文* r38;
  struct 语句节点* r39;
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

  entry:
  r0 = cn_var_语句;
  r1 = r0 == 0;
  if (r1) goto if_then_2490; else goto if_merge_2491;

  if_then_2490:
  return;
  goto if_merge_2491;

  if_merge_2491:
  r2 = cn_var_语句;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = r4 == 12;
  if (r5) goto case_body_2493; else goto switch_check_2505;

  switch_check_2505:
  r6 = r4 == 13;
  if (r6) goto case_body_2494; else goto switch_check_2506;

  switch_check_2506:
  r7 = r4 == 14;
  if (r7) goto case_body_2495; else goto switch_check_2507;

  switch_check_2507:
  r8 = r4 == 15;
  if (r8) goto case_body_2496; else goto switch_check_2508;

  switch_check_2508:
  r9 = r4 == 16;
  if (r9) goto case_body_2497; else goto switch_check_2509;

  switch_check_2509:
  r10 = r4 == 17;
  if (r10) goto case_body_2498; else goto switch_check_2510;

  switch_check_2510:
  r11 = r4 == 18;
  if (r11) goto case_body_2499; else goto switch_check_2511;

  switch_check_2511:
  r12 = r4 == 19;
  if (r12) goto case_body_2500; else goto switch_check_2512;

  switch_check_2512:
  r13 = r4 == 11;
  if (r13) goto case_body_2501; else goto switch_check_2513;

  switch_check_2513:
  r14 = r4 == 20;
  if (r14) goto case_body_2502; else goto switch_check_2514;

  switch_check_2514:
  r15 = r4 == 21;
  if (r15) goto case_body_2503; else goto case_default_2504;

  case_body_2493:
  r16 = cn_var_上下文;
  r17 = cn_var_语句;
  检查块语句(r16, (struct 块语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2494:
  r18 = cn_var_上下文;
  r19 = cn_var_语句;
  检查如果语句(r18, (struct 如果语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2495:
  r20 = cn_var_上下文;
  r21 = cn_var_语句;
  检查当语句(r20, (struct 当语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2496:
  r22 = cn_var_上下文;
  r23 = cn_var_语句;
  检查循环语句(r22, (struct 循环语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2497:
  r24 = cn_var_上下文;
  r25 = cn_var_语句;
  检查返回语句(r24, (struct 返回语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2498:
  r26 = cn_var_上下文;
  r27 = cn_var_语句;
  检查中断语句(r26, (struct 中断语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2499:
  r28 = cn_var_上下文;
  r29 = cn_var_语句;
  检查继续语句(r28, (struct 继续语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2500:
  r30 = cn_var_上下文;
  r31 = cn_var_语句;
  检查选择语句(r30, (struct 选择语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2501:
  r32 = cn_var_上下文;
  r33 = cn_var_语句;
  r34 = (struct 表达式语句*)cn_var_语句->表达式;
  r35 = 检查表达式(r32, r34);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2502:
  r36 = cn_var_上下文;
  r37 = cn_var_语句;
  检查尝试语句(r36, (struct 尝试语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_body_2503:
  r38 = cn_var_上下文;
  r39 = cn_var_语句;
  检查抛出语句(r38, (struct 抛出语句*)cn_var_语句);
  goto switch_merge_2492;
  goto switch_merge_2492;

  case_default_2504:
  goto switch_merge_2492;
  goto switch_merge_2492;

  switch_merge_2492:
  return;
}

void 检查块语句(struct 语义检查上下文* cn_var_上下文, struct 块语句* cn_var_块节点) {
  long long r1, r2, r4, r5, r8, r9, r11, r12;
  struct 块语句* r0;
  struct 块语句* r3;
  struct 语义检查上下文* r6;
  struct 块语句* r7;
  void* r10;

  entry:
  r0 = cn_var_块节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2515; else goto if_merge_2516;

  if_then_2515:
  return;
  goto if_merge_2516;

  if_merge_2516:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2517;

  for_cond_2517:
  r2 = cn_var_i_0;
  r3 = cn_var_块节点;
  r4 = r3->语句个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2518; else goto for_exit_2520;

  for_body_2518:
  r6 = cn_var_上下文;
  r7 = cn_var_块节点;
  r8 = r7->语句列表;
  r9 = cn_var_i_0;
  r10 = *(void**)cn_rt_array_get_element(r8, r9, 8);
  检查语句(r6, r10);
  goto for_update_2519;

  for_update_2519:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2517;

  for_exit_2520:
  return;
}

void 检查如果语句(struct 语义检查上下文* cn_var_上下文, struct 如果语句* cn_var_如果节点) {
  long long r1, r6, r7, r8, r9, r10, r11, r15, r16, r19, r25;
  struct 如果语句* r0;
  struct 语义检查上下文* r2;
  struct 如果语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 如果语句* r13;
  struct 表达式节点* r14;
  struct 如果语句* r17;
  struct 块语句* r18;
  struct 语义检查上下文* r20;
  struct 如果语句* r21;
  struct 块语句* r22;
  struct 如果语句* r23;
  struct 语句节点* r24;
  struct 语义检查上下文* r26;
  struct 如果语句* r27;
  struct 语句节点* r28;

  entry:
  r0 = cn_var_如果节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2521; else goto if_merge_2522;

  if_then_2521:
  return;
  goto if_merge_2522;

  if_merge_2522:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_如果节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  r7 = cn_var_条件类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2525; else goto logic_merge_2526;

  if_then_2523:
  r12 = cn_var_上下文;
  r13 = cn_var_如果节点;
  r14 = r13->条件;
  r15 = r14->节点基类;
  r16 = r15.位置;
  报告错误(r12, r16, 23);
  goto if_merge_2524;

  if_merge_2524:
  r17 = cn_var_如果节点;
  r18 = r17->真分支;
  r19 = r18 != 0;
  if (r19) goto if_then_2527; else goto if_merge_2528;

  logic_rhs_2525:
  r9 = cn_var_条件类型_0;
  r10 = r9.种类;
  r11 = r10 != /* NONE */;
  goto logic_merge_2526;

  logic_merge_2526:
  if (r11) goto if_then_2523; else goto if_merge_2524;

  if_then_2527:
  r20 = cn_var_上下文;
  r21 = cn_var_如果节点;
  r22 = r21->真分支;
  检查语句(r20, r22);
  goto if_merge_2528;

  if_merge_2528:
  r23 = cn_var_如果节点;
  r24 = r23->假分支;
  r25 = r24 != 0;
  if (r25) goto if_then_2529; else goto if_merge_2530;

  if_then_2529:
  r26 = cn_var_上下文;
  r27 = cn_var_如果节点;
  r28 = r27->假分支;
  检查语句(r26, r28);
  goto if_merge_2530;

  if_merge_2530:
  return;
}

void 检查当语句(struct 语义检查上下文* cn_var_上下文, struct 当语句* cn_var_当节点) {
  long long r1, r6, r7, r8, r9, r10, r11, r15, r16, r18, r19, r22, r27, r28;
  struct 当语句* r0;
  struct 语义检查上下文* r2;
  struct 当语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 当语句* r13;
  struct 表达式节点* r14;
  struct 语义检查上下文* r17;
  struct 当语句* r20;
  struct 块语句* r21;
  struct 语义检查上下文* r23;
  struct 当语句* r24;
  struct 块语句* r25;
  struct 语义检查上下文* r26;

  entry:
  r0 = cn_var_当节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2531; else goto if_merge_2532;

  if_then_2531:
  return;
  goto if_merge_2532;

  if_merge_2532:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_当节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  r7 = cn_var_条件类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2535; else goto logic_merge_2536;

  if_then_2533:
  r12 = cn_var_上下文;
  r13 = cn_var_当节点;
  r14 = r13->条件;
  r15 = r14->节点基类;
  r16 = r15.位置;
  报告错误(r12, r16, 23);
  goto if_merge_2534;

  if_merge_2534:
  r17 = cn_var_上下文;
  r18 = r17->循环深度;
  r19 = r18 + 1;
  r20 = cn_var_当节点;
  r21 = r20->循环体;
  r22 = r21 != 0;
  if (r22) goto if_then_2537; else goto if_merge_2538;

  logic_rhs_2535:
  r9 = cn_var_条件类型_0;
  r10 = r9.种类;
  r11 = r10 != /* NONE */;
  goto logic_merge_2536;

  logic_merge_2536:
  if (r11) goto if_then_2533; else goto if_merge_2534;

  if_then_2537:
  r23 = cn_var_上下文;
  r24 = cn_var_当节点;
  r25 = r24->循环体;
  检查语句(r23, r25);
  goto if_merge_2538;

  if_merge_2538:
  r26 = cn_var_上下文;
  r27 = r26->循环深度;
  r28 = r27 - 1;
  return;
}

void 检查循环语句(struct 语义检查上下文* cn_var_上下文, struct 循环语句* cn_var_循环节点) {
  long long r1, r4, r10, r15, r16, r17, r18, r19, r20, r24, r25, r28, r34, r35, r38, r43, r44;
  struct 循环语句* r0;
  struct 循环语句* r2;
  struct 语句节点* r3;
  struct 语义检查上下文* r5;
  struct 循环语句* r6;
  struct 语句节点* r7;
  struct 循环语句* r8;
  struct 表达式节点* r9;
  struct 语义检查上下文* r11;
  struct 循环语句* r12;
  struct 表达式节点* r13;
  struct 类型信息* r14;
  struct 语义检查上下文* r21;
  struct 循环语句* r22;
  struct 表达式节点* r23;
  struct 循环语句* r26;
  struct 表达式节点* r27;
  struct 语义检查上下文* r29;
  struct 循环语句* r30;
  struct 表达式节点* r31;
  struct 类型信息* r32;
  struct 语义检查上下文* r33;
  struct 循环语句* r36;
  struct 块语句* r37;
  struct 语义检查上下文* r39;
  struct 循环语句* r40;
  struct 块语句* r41;
  struct 语义检查上下文* r42;

  entry:
  r0 = cn_var_循环节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2539; else goto if_merge_2540;

  if_then_2539:
  return;
  goto if_merge_2540;

  if_merge_2540:
  r2 = cn_var_循环节点;
  r3 = r2->初始化;
  r4 = r3 != 0;
  if (r4) goto if_then_2541; else goto if_merge_2542;

  if_then_2541:
  r5 = cn_var_上下文;
  r6 = cn_var_循环节点;
  r7 = r6->初始化;
  检查语句(r5, r7);
  goto if_merge_2542;

  if_merge_2542:
  r8 = cn_var_循环节点;
  r9 = r8->条件;
  r10 = r9 != 0;
  if (r10) goto if_then_2543; else goto if_merge_2544;

  if_then_2543:
  long long cn_var_条件类型_0;
  r11 = cn_var_上下文;
  r12 = cn_var_循环节点;
  r13 = r12->条件;
  r14 = 检查表达式(r11, r13);
  cn_var_条件类型_0 = r14;
  r16 = cn_var_条件类型_0;
  r17 = r16 != 0;
  if (r17) goto logic_rhs_2547; else goto logic_merge_2548;

  if_merge_2544:
  r26 = cn_var_循环节点;
  r27 = r26->更新;
  r28 = r27 != 0;
  if (r28) goto if_then_2549; else goto if_merge_2550;

  if_then_2545:
  r21 = cn_var_上下文;
  r22 = cn_var_循环节点;
  r23 = r22->条件;
  r24 = r23->节点基类;
  r25 = r24.位置;
  报告错误(r21, r25, 23);
  goto if_merge_2546;

  if_merge_2546:
  goto if_merge_2544;

  logic_rhs_2547:
  r18 = cn_var_条件类型_0;
  r19 = r18.种类;
  r20 = r19 != /* NONE */;
  goto logic_merge_2548;

  logic_merge_2548:
  if (r20) goto if_then_2545; else goto if_merge_2546;

  if_then_2549:
  r29 = cn_var_上下文;
  r30 = cn_var_循环节点;
  r31 = r30->更新;
  r32 = 检查表达式(r29, r31);
  goto if_merge_2550;

  if_merge_2550:
  r33 = cn_var_上下文;
  r34 = r33->循环深度;
  r35 = r34 + 1;
  r36 = cn_var_循环节点;
  r37 = r36->循环体;
  r38 = r37 != 0;
  if (r38) goto if_then_2551; else goto if_merge_2552;

  if_then_2551:
  r39 = cn_var_上下文;
  r40 = cn_var_循环节点;
  r41 = r40->循环体;
  检查语句(r39, r41);
  goto if_merge_2552;

  if_merge_2552:
  r42 = cn_var_上下文;
  r43 = r42->循环深度;
  r44 = r43 - 1;
  return;
}

void 检查返回语句(struct 语义检查上下文* cn_var_上下文, struct 返回语句* cn_var_返回节点) {
  long long r1, r4, r11, r16, r19, r20, r21, r22, r26, r27, r32, r35, r39;
  struct 返回语句* r0;
  struct 语义检查上下文* r2;
  struct 符号* r3;
  struct 语义检查上下文* r5;
  struct 返回语句* r6;
  struct 返回语句* r9;
  struct 表达式节点* r10;
  struct 语义检查上下文* r12;
  struct 返回语句* r13;
  struct 表达式节点* r14;
  struct 类型信息* r15;
  struct 语义检查上下文* r17;
  struct 类型信息* r18;
  struct 语义检查上下文* r23;
  struct 类型信息* r24;
  struct 语义检查上下文* r28;
  struct 返回语句* r29;
  struct 语义检查上下文* r33;
  struct 类型信息* r34;
  struct 语义检查上下文* r36;
  struct 类型信息* r37;
  struct 语义检查上下文* r40;
  struct 返回语句* r41;
  struct AST节点 r7;
  struct 源位置 r8;
  enum 类型兼容性 r25;
  struct AST节点 r30;
  struct 源位置 r31;
  struct AST节点 r42;
  struct 源位置 r43;
  enum 类型种类 r38;

  entry:
  r0 = cn_var_返回节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2553; else goto if_merge_2554;

  if_then_2553:
  return;
  goto if_merge_2554;

  if_merge_2554:
  r2 = cn_var_上下文;
  r3 = r2->当前函数;
  r4 = r3 == 0;
  if (r4) goto if_then_2555; else goto if_merge_2556;

  if_then_2555:
  r5 = cn_var_上下文;
  r6 = cn_var_返回节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  报告错误(r5, r8, 26);
  return;
  goto if_merge_2556;

  if_merge_2556:
  r9 = cn_var_返回节点;
  r10 = r9->返回值;
  r11 = r10 != 0;
  if (r11) goto if_then_2557; else goto if_else_2558;

  if_then_2557:
  long long cn_var_返回值类型_0;
  r12 = cn_var_上下文;
  r13 = cn_var_返回节点;
  r14 = r13->返回值;
  r15 = 检查表达式(r12, r14);
  cn_var_返回值类型_0 = r15;
  r17 = cn_var_上下文;
  r18 = r17->期望返回类型;
  r19 = r18 != 0;
  if (r19) goto logic_rhs_2562; else goto logic_merge_2563;

  if_else_2558:
  r33 = cn_var_上下文;
  r34 = r33->期望返回类型;
  r35 = r34 != 0;
  if (r35) goto logic_rhs_2568; else goto logic_merge_2569;

  if_merge_2559:

  if_then_2560:
  long long cn_var_兼容性_1;
  r22 = cn_var_返回值类型_0;
  r23 = cn_var_上下文;
  r24 = r23->期望返回类型;
  r25 = 检查类型兼容性(r22, r24);
  cn_var_兼容性_1 = r25;
  r26 = cn_var_兼容性_1;
  r27 = r26 == 3;
  if (r27) goto if_then_2564; else goto if_merge_2565;

  if_merge_2561:
  goto if_merge_2559;

  logic_rhs_2562:
  r20 = cn_var_返回值类型_0;
  r21 = r20 != 0;
  goto logic_merge_2563;

  logic_merge_2563:
  if (r21) goto if_then_2560; else goto if_merge_2561;

  if_then_2564:
  r28 = cn_var_上下文;
  r29 = cn_var_返回节点;
  r30 = r29->节点基类;
  r31 = r30.位置;
  报告错误(r28, r31, 23);
  goto if_merge_2565;

  if_merge_2565:
  goto if_merge_2561;

  if_then_2566:
  r40 = cn_var_上下文;
  r41 = cn_var_返回节点;
  r42 = r41->节点基类;
  r43 = r42.位置;
  报告错误(r40, r43, 23);
  goto if_merge_2567;

  if_merge_2567:
  goto if_merge_2559;

  logic_rhs_2568:
  r36 = cn_var_上下文;
  r37 = r36->期望返回类型;
  r38 = r37->种类;
  r39 = r38 != 0;
  goto logic_merge_2569;

  logic_merge_2569:
  if (r39) goto if_then_2566; else goto if_merge_2567;
  return;
}

void 检查中断语句(struct 语义检查上下文* cn_var_上下文, struct 中断语句* cn_var_中断节点) {
  long long r1, r3, r4;
  struct 中断语句* r0;
  struct 语义检查上下文* r2;
  struct 语义检查上下文* r5;
  struct 中断语句* r6;
  struct AST节点 r7;
  struct 源位置 r8;

  entry:
  r0 = cn_var_中断节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2570; else goto if_merge_2571;

  if_then_2570:
  return;
  goto if_merge_2571;

  if_merge_2571:
  r2 = cn_var_上下文;
  r3 = r2->循环深度;
  r4 = r3 == 0;
  if (r4) goto if_then_2572; else goto if_merge_2573;

  if_then_2572:
  r5 = cn_var_上下文;
  r6 = cn_var_中断节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  报告错误(r5, r8, 27);
  goto if_merge_2573;

  if_merge_2573:
  return;
}

void 检查继续语句(struct 语义检查上下文* cn_var_上下文, struct 继续语句* cn_var_继续节点) {
  long long r1, r3, r4;
  struct 继续语句* r0;
  struct 语义检查上下文* r2;
  struct 语义检查上下文* r5;
  struct 继续语句* r6;
  struct AST节点 r7;
  struct 源位置 r8;

  entry:
  r0 = cn_var_继续节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2574; else goto if_merge_2575;

  if_then_2574:
  return;
  goto if_merge_2575;

  if_merge_2575:
  r2 = cn_var_上下文;
  r3 = r2->循环深度;
  r4 = r3 == 0;
  if (r4) goto if_then_2576; else goto if_merge_2577;

  if_then_2576:
  r5 = cn_var_上下文;
  r6 = cn_var_继续节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  报告错误(r5, r8, 27);
  goto if_merge_2577;

  if_merge_2577:
  return;
}

void 检查选择语句(struct 语义检查上下文* cn_var_上下文, struct 选择语句* cn_var_选择节点) {
  long long r1, r6, r7, r8, r9, r11, r15, r16, r17, r19, r20, r24, r26, r27, r30;
  void* r23;
  struct 选择语句* r0;
  struct 语义检查上下文* r2;
  struct 选择语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 选择语句* r13;
  struct 表达式节点* r14;
  struct 选择语句* r18;
  struct 语义检查上下文* r21;
  struct 选择语句* r22;
  void* r25;
  struct 选择语句* r28;
  struct 块语句* r29;
  struct 语义检查上下文* r31;
  struct 选择语句* r32;
  struct 块语句* r33;
  _Bool r10;

  entry:
  r0 = cn_var_选择节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2578; else goto if_merge_2579;

  if_then_2578:
  return;
  goto if_merge_2579;

  if_merge_2579:
  long long cn_var_选择类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_选择节点;
  r4 = r3->选择值;
  r5 = 检查表达式(r2, r4);
  cn_var_选择类型_0 = r5;
  r7 = cn_var_选择类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2582; else goto logic_merge_2583;

  if_then_2580:
  r12 = cn_var_上下文;
  r13 = cn_var_选择节点;
  r14 = r13->选择值;
  r15 = r14->节点基类;
  r16 = r15.位置;
  报告错误(r12, r16, 23);
  goto if_merge_2581;

  if_merge_2581:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2584;

  logic_rhs_2582:
  r9 = cn_var_选择类型_0;
  r10 = 是整数类型(r9);
  r11 = r10 == 0;
  goto logic_merge_2583;

  logic_merge_2583:
  if (r11) goto if_then_2580; else goto if_merge_2581;

  for_cond_2584:
  r17 = cn_var_i_1;
  r18 = cn_var_选择节点;
  r19 = r18->情况个数;
  r20 = r17 < r19;
  if (r20) goto for_body_2585; else goto for_exit_2587;

  for_body_2585:
  r21 = cn_var_上下文;
  r22 = cn_var_选择节点;
  r23 = r22->情况分支列表;
  r24 = cn_var_i_1;
  r25 = *(void**)cn_rt_array_get_element(r23, r24, 8);
  检查情况分支(r21, r25);
  goto for_update_2586;

  for_update_2586:
  r26 = cn_var_i_1;
  r27 = r26 + 1;
  cn_var_i_1 = r27;
  goto for_cond_2584;

  for_exit_2587:
  r28 = cn_var_选择节点;
  r29 = r28->默认分支;
  r30 = r29 != 0;
  if (r30) goto if_then_2588; else goto if_merge_2589;

  if_then_2588:
  r31 = cn_var_上下文;
  r32 = cn_var_选择节点;
  r33 = r32->默认分支;
  检查语句(r31, r33);
  goto if_merge_2589;

  if_merge_2589:
  return;
}

void 检查情况分支(struct 语义检查上下文* cn_var_上下文, struct 情况分支* cn_var_情况节点) {
  long long r1, r4, r8, r12, r13, r16;
  struct 情况分支* r0;
  struct 情况分支* r2;
  struct 表达式节点* r3;
  struct 情况分支* r5;
  struct 表达式节点* r6;
  struct 语义检查上下文* r9;
  struct 情况分支* r10;
  struct 表达式节点* r11;
  struct 情况分支* r14;
  struct 块语句* r15;
  struct 语义检查上下文* r17;
  struct 情况分支* r18;
  struct 块语句* r19;
  _Bool r7;

  entry:
  r0 = cn_var_情况节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2590; else goto if_merge_2591;

  if_then_2590:
  return;
  goto if_merge_2591;

  if_merge_2591:
  r2 = cn_var_情况节点;
  r3 = r2->匹配值;
  r4 = r3 != 0;
  if (r4) goto if_then_2592; else goto if_merge_2593;

  if_then_2592:
  r5 = cn_var_情况节点;
  r6 = r5->匹配值;
  r7 = 是常量表达式(r6);
  r8 = r7 == 0;
  if (r8) goto if_then_2594; else goto if_merge_2595;

  if_merge_2593:
  r14 = cn_var_情况节点;
  r15 = r14->语句体;
  r16 = r15 != 0;
  if (r16) goto if_then_2596; else goto if_merge_2597;

  if_then_2594:
  r9 = cn_var_上下文;
  r10 = cn_var_情况节点;
  r11 = r10->匹配值;
  r12 = r11->节点基类;
  r13 = r12.位置;
  报告错误(r9, r13, 36);
  goto if_merge_2595;

  if_merge_2595:
  goto if_merge_2593;

  if_then_2596:
  r17 = cn_var_上下文;
  r18 = cn_var_情况节点;
  r19 = r18->语句体;
  检查块语句(r17, r19);
  goto if_merge_2597;

  if_merge_2597:
  return;
}

void 检查尝试语句(struct 语义检查上下文* cn_var_上下文, struct 尝试语句* cn_var_尝试节点) {
  long long r1, r4, r8, r10, r11, r15, r17, r18, r21;
  void* r14;
  struct 尝试语句* r0;
  struct 尝试语句* r2;
  struct 块语句* r3;
  struct 语义检查上下文* r5;
  struct 尝试语句* r6;
  struct 块语句* r7;
  struct 尝试语句* r9;
  struct 语义检查上下文* r12;
  struct 尝试语句* r13;
  void* r16;
  struct 尝试语句* r19;
  struct 块语句* r20;
  struct 语义检查上下文* r22;
  struct 尝试语句* r23;
  struct 块语句* r24;

  entry:
  r0 = cn_var_尝试节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2598; else goto if_merge_2599;

  if_then_2598:
  return;
  goto if_merge_2599;

  if_merge_2599:
  r2 = cn_var_尝试节点;
  r3 = r2->尝试块;
  r4 = r3 != 0;
  if (r4) goto if_then_2600; else goto if_merge_2601;

  if_then_2600:
  r5 = cn_var_上下文;
  r6 = cn_var_尝试节点;
  r7 = r6->尝试块;
  检查语句(r5, r7);
  goto if_merge_2601;

  if_merge_2601:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2602;

  for_cond_2602:
  r8 = cn_var_i_0;
  r9 = cn_var_尝试节点;
  r10 = r9->捕获个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2603; else goto for_exit_2605;

  for_body_2603:
  r12 = cn_var_上下文;
  r13 = cn_var_尝试节点;
  r14 = r13->捕获子句列表;
  r15 = cn_var_i_0;
  r16 = *(void**)cn_rt_array_get_element(r14, r15, 8);
  检查捕获子句(r12, r16);
  goto for_update_2604;

  for_update_2604:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_2602;

  for_exit_2605:
  r19 = cn_var_尝试节点;
  r20 = r19->最终块;
  r21 = r20 != 0;
  if (r21) goto if_then_2606; else goto if_merge_2607;

  if_then_2606:
  r22 = cn_var_上下文;
  r23 = cn_var_尝试节点;
  r24 = r23->最终块;
  检查语句(r22, r24);
  goto if_merge_2607;

  if_merge_2607:
  return;
}

void 检查捕获子句(struct 语义检查上下文* cn_var_上下文, struct 捕获子句* cn_var_捕获节点) {
  long long r1, r4;
  struct 捕获子句* r0;
  struct 捕获子句* r2;
  struct 块语句* r3;
  struct 语义检查上下文* r5;
  struct 捕获子句* r6;
  struct 块语句* r7;

  entry:
  r0 = cn_var_捕获节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2608; else goto if_merge_2609;

  if_then_2608:
  return;
  goto if_merge_2609;

  if_merge_2609:
  r2 = cn_var_捕获节点;
  r3 = r2->语句体;
  r4 = r3 != 0;
  if (r4) goto if_then_2610; else goto if_merge_2611;

  if_then_2610:
  r5 = cn_var_上下文;
  r6 = cn_var_捕获节点;
  r7 = r6->语句体;
  检查块语句(r5, r7);
  goto if_merge_2611;

  if_merge_2611:
  return;
}

void 检查抛出语句(struct 语义检查上下文* cn_var_上下文, struct 抛出语句* cn_var_抛出节点) {
  long long r1, r3, r4, r7;
  struct 抛出语句* r0;
  struct 抛出语句* r2;
  struct 语义检查上下文* r5;
  struct 抛出语句* r6;
  struct 类型信息* r8;

  entry:
  r0 = cn_var_抛出节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2612; else goto if_merge_2613;

  if_then_2612:
  return;
  goto if_merge_2613;

  if_merge_2613:
  r2 = cn_var_抛出节点;
  r3 = r2->抛出值;
  r4 = r3 != 0;
  if (r4) goto if_then_2614; else goto if_merge_2615;

  if_then_2614:
  r5 = cn_var_上下文;
  r6 = cn_var_抛出节点;
  r7 = r6->抛出值;
  r8 = 检查表达式(r5, r7);
  goto if_merge_2615;

  if_merge_2615:
  return;
}

struct 类型信息* 检查表达式(struct 语义检查上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r3, r4, r41, r42, r43, r45;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 语义检查上下文* r14;
  struct 表达式节点* r15;
  struct 类型信息* r16;
  struct 语义检查上下文* r17;
  struct 表达式节点* r18;
  struct 类型信息* r19;
  struct 语义检查上下文* r20;
  struct 表达式节点* r21;
  struct 类型信息* r22;
  struct 语义检查上下文* r23;
  struct 表达式节点* r24;
  struct 类型信息* r25;
  struct 语义检查上下文* r26;
  struct 表达式节点* r27;
  struct 类型信息* r28;
  struct 语义检查上下文* r29;
  struct 表达式节点* r30;
  struct 类型信息* r31;
  struct 语义检查上下文* r32;
  struct 表达式节点* r33;
  struct 类型信息* r34;
  struct 语义检查上下文* r35;
  struct 表达式节点* r36;
  struct 类型信息* r37;
  struct 语义检查上下文* r38;
  struct 表达式节点* r39;
  struct 类型信息* r40;
  struct 类型节点* r44;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;

  entry:
  r0 = cn_var_表达式;
  r1 = r0 == 0;
  if (r1) goto if_then_2616; else goto if_merge_2617;

  if_then_2616:
  return 0;
  goto if_merge_2617;

  if_merge_2617:
  long long cn_var_结果类型_0;
  cn_var_结果类型_0 = 0;
  r2 = cn_var_表达式;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = r4 == 23;
  if (r5) goto case_body_2619; else goto switch_check_2629;

  switch_check_2629:
  r6 = r4 == 24;
  if (r6) goto case_body_2620; else goto switch_check_2630;

  switch_check_2630:
  r7 = r4 == 25;
  if (r7) goto case_body_2621; else goto switch_check_2631;

  switch_check_2631:
  r8 = r4 == 26;
  if (r8) goto case_body_2622; else goto switch_check_2632;

  switch_check_2632:
  r9 = r4 == 27;
  if (r9) goto case_body_2623; else goto switch_check_2633;

  switch_check_2633:
  r10 = r4 == 28;
  if (r10) goto case_body_2624; else goto switch_check_2634;

  switch_check_2634:
  r11 = r4 == 29;
  if (r11) goto case_body_2625; else goto switch_check_2635;

  switch_check_2635:
  r12 = r4 == 30;
  if (r12) goto case_body_2626; else goto switch_check_2636;

  switch_check_2636:
  r13 = r4 == 31;
  if (r13) goto case_body_2627; else goto case_default_2628;

  case_body_2619:
  r14 = cn_var_上下文;
  r15 = cn_var_表达式;
  r16 = 检查二元表达式(r14, (struct 二元表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r16;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2620:
  r17 = cn_var_上下文;
  r18 = cn_var_表达式;
  r19 = 检查一元表达式(r17, (struct 一元表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r19;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2621:
  r20 = cn_var_上下文;
  r21 = cn_var_表达式;
  r22 = 检查字面量表达式(r20, (struct 字面量表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r22;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2622:
  r23 = cn_var_上下文;
  r24 = cn_var_表达式;
  r25 = 检查标识符表达式(r23, (struct 标识符表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r25;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2623:
  r26 = cn_var_上下文;
  r27 = cn_var_表达式;
  r28 = 检查函数调用表达式(r26, (struct 函数调用表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r28;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2624:
  r29 = cn_var_上下文;
  r30 = cn_var_表达式;
  r31 = 检查成员访问表达式(r29, (struct 成员访问表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r31;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2625:
  r32 = cn_var_上下文;
  r33 = cn_var_表达式;
  r34 = 检查数组访问表达式(r32, (struct 数组访问表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r34;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2626:
  r35 = cn_var_上下文;
  r36 = cn_var_表达式;
  r37 = 检查赋值表达式(r35, (struct 赋值表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r37;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_body_2627:
  r38 = cn_var_上下文;
  r39 = cn_var_表达式;
  r40 = 检查三元表达式(r38, (struct 三元表达式*)cn_var_表达式);
  cn_var_结果类型_0 = r40;
  goto switch_merge_2618;
  goto switch_merge_2618;

  case_default_2628:
  cn_var_结果类型_0 = 0;
  goto switch_merge_2618;
  goto switch_merge_2618;

  switch_merge_2618:
  r41 = cn_var_结果类型_0;
  r42 = r41 != 0;
  if (r42) goto if_then_2637; else goto if_merge_2638;

  if_then_2637:
  r43 = cn_var_结果类型_0;
  r44 = 类型信息转类型节点(r43);
  goto if_merge_2638;

  if_merge_2638:
  r45 = cn_var_结果类型_0;
  return r45;
}

struct 类型信息* 检查二元表达式(struct 语义检查上下文* cn_var_上下文, struct 二元表达式* cn_var_二元节点) {
  long long r1, r28, r29, r31, r33, r39, r40, r42, r44;
  struct 二元表达式* r0;
  struct 语义检查上下文* r2;
  struct 二元表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 二元表达式* r7;
  struct 表达式节点* r8;
  struct 类型信息* r9;
  struct 二元表达式* r10;
  struct 语义检查上下文* r34;
  struct 二元表达式* r35;
  struct 类型信息* r38;
  struct 语义检查上下文* r45;
  struct 二元表达式* r46;
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
  _Bool r30;
  _Bool r32;
  _Bool r41;
  _Bool r43;
  struct AST节点 r36;
  struct 源位置 r37;
  struct AST节点 r47;
  struct 源位置 r48;
  enum 二元运算符 r11;

  entry:
  r0 = cn_var_二元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2639; else goto if_merge_2640;

  if_then_2639:
  return 0;
  goto if_merge_2640;

  if_merge_2640:
  long long cn_var_左类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_二元节点;
  r4 = r3->左操作数;
  r5 = 检查表达式(r2, r4);
  cn_var_左类型_0 = r5;
  long long cn_var_右类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_二元节点;
  r8 = r7->右操作数;
  r9 = 检查表达式(r6, r8);
  cn_var_右类型_1 = r9;
  r10 = cn_var_二元节点;
  r11 = r10->运算符;
  r12 = r11 == 0;
  if (r12) goto case_body_2642; else goto switch_check_2659;

  switch_check_2659:
  r13 = r11 == 1;
  if (r13) goto case_body_2643; else goto switch_check_2660;

  switch_check_2660:
  r14 = r11 == 2;
  if (r14) goto case_body_2644; else goto switch_check_2661;

  switch_check_2661:
  r15 = r11 == 3;
  if (r15) goto case_body_2645; else goto switch_check_2662;

  switch_check_2662:
  r16 = r11 == 4;
  if (r16) goto case_body_2646; else goto switch_check_2663;

  switch_check_2663:
  r17 = r11 == 5;
  if (r17) goto case_body_2647; else goto switch_check_2664;

  switch_check_2664:
  r18 = r11 == 6;
  if (r18) goto case_body_2648; else goto switch_check_2665;

  switch_check_2665:
  r19 = r11 == 7;
  if (r19) goto case_body_2649; else goto switch_check_2666;

  switch_check_2666:
  r20 = r11 == 9;
  if (r20) goto case_body_2650; else goto switch_check_2667;

  switch_check_2667:
  r21 = r11 == 8;
  if (r21) goto case_body_2651; else goto switch_check_2668;

  switch_check_2668:
  r22 = r11 == 10;
  if (r22) goto case_body_2652; else goto switch_check_2669;

  switch_check_2669:
  r23 = r11 == 11;
  if (r23) goto case_body_2653; else goto switch_check_2670;

  switch_check_2670:
  r24 = r11 == 12;
  if (r24) goto case_body_2654; else goto switch_check_2671;

  switch_check_2671:
  r25 = r11 == 13;
  if (r25) goto case_body_2655; else goto switch_check_2672;

  switch_check_2672:
  r26 = r11 == 14;
  if (r26) goto case_body_2656; else goto switch_check_2673;

  switch_check_2673:
  r27 = r11 == 15;
  if (r27) goto case_body_2657; else goto case_default_2658;

  case_body_2642:
  goto switch_merge_2641;

  case_body_2643:
  goto switch_merge_2641;

  case_body_2644:
  goto switch_merge_2641;

  case_body_2645:
  goto switch_merge_2641;

  case_body_2646:
  r29 = cn_var_左类型_0;
  r30 = 是数值类型(r29);
  if (r30) goto logic_rhs_2676; else goto logic_merge_2677;

  if_then_2674:
  r33 = cn_var_左类型_0;
  return r33;
  goto if_merge_2675;

  if_merge_2675:
  r34 = cn_var_上下文;
  r35 = cn_var_二元节点;
  r36 = r35->节点基类;
  r37 = r36.位置;
  报告错误(r34, r37, 23);
  return 0;
  goto switch_merge_2641;

  logic_rhs_2676:
  r31 = cn_var_右类型_1;
  r32 = 是数值类型(r31);
  goto logic_merge_2677;

  logic_merge_2677:
  if (r32) goto if_then_2674; else goto if_merge_2675;

  case_body_2647:
  goto switch_merge_2641;

  case_body_2648:
  goto switch_merge_2641;

  case_body_2649:
  goto switch_merge_2641;

  case_body_2650:
  goto switch_merge_2641;

  case_body_2651:
  goto switch_merge_2641;

  case_body_2652:
  r38 = 创建布尔类型();
  return r38;
  goto switch_merge_2641;

  case_body_2653:
  goto switch_merge_2641;

  case_body_2654:
  goto switch_merge_2641;

  case_body_2655:
  goto switch_merge_2641;

  case_body_2656:
  goto switch_merge_2641;

  case_body_2657:
  r40 = cn_var_左类型_0;
  r41 = 是整数类型(r40);
  if (r41) goto logic_rhs_2680; else goto logic_merge_2681;

  if_then_2678:
  r44 = cn_var_左类型_0;
  return r44;
  goto if_merge_2679;

  if_merge_2679:
  r45 = cn_var_上下文;
  r46 = cn_var_二元节点;
  r47 = r46->节点基类;
  r48 = r47.位置;
  报告错误(r45, r48, 23);
  return 0;
  goto switch_merge_2641;

  logic_rhs_2680:
  r42 = cn_var_右类型_1;
  r43 = 是整数类型(r42);
  goto logic_merge_2681;

  logic_merge_2681:
  if (r43) goto if_then_2678; else goto if_merge_2679;

  case_default_2658:
  return 0;
  goto switch_merge_2641;

  switch_merge_2641:
  return NULL;
}

struct 类型信息* 检查一元表达式(struct 语义检查上下文* cn_var_上下文, struct 一元表达式* cn_var_一元节点) {
  long long r1, r17, r19, r25, r27, r32, r34, r36, r37, r42, r44;
  struct 一元表达式* r0;
  struct 语义检查上下文* r2;
  struct 一元表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 一元表达式* r6;
  struct 语义检查上下文* r20;
  struct 一元表达式* r21;
  struct 类型信息* r24;
  struct 语义检查上下文* r28;
  struct 一元表达式* r29;
  struct 类型信息* r33;
  struct 语义检查上下文* r38;
  struct 一元表达式* r39;
  struct 语义检查上下文* r45;
  struct 一元表达式* r46;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r18;
  _Bool r26;
  _Bool r35;
  _Bool r43;
  struct AST节点 r22;
  struct 源位置 r23;
  struct AST节点 r30;
  struct 源位置 r31;
  struct AST节点 r40;
  struct 源位置 r41;
  struct AST节点 r47;
  struct 源位置 r48;
  enum 一元运算符 r7;

  entry:
  r0 = cn_var_一元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2682; else goto if_merge_2683;

  if_then_2682:
  return 0;
  goto if_merge_2683;

  if_merge_2683:
  long long cn_var_操作数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_一元节点;
  r4 = r3->操作数;
  r5 = 检查表达式(r2, r4);
  cn_var_操作数类型_0 = r5;
  r6 = cn_var_一元节点;
  r7 = r6->运算符;
  r8 = r7 == /* NONE */;
  if (r8) goto case_body_2685; else goto switch_check_2695;

  switch_check_2695:
  r9 = r7 == 0;
  if (r9) goto case_body_2686; else goto switch_check_2696;

  switch_check_2696:
  r10 = r7 == 4;
  if (r10) goto case_body_2687; else goto switch_check_2697;

  switch_check_2697:
  r11 = r7 == 2;
  if (r11) goto case_body_2688; else goto switch_check_2698;

  switch_check_2698:
  r12 = r7 == 3;
  if (r12) goto case_body_2689; else goto switch_check_2699;

  switch_check_2699:
  r13 = r7 == 5;
  if (r13) goto case_body_2690; else goto switch_check_2700;

  switch_check_2700:
  r14 = r7 == 6;
  if (r14) goto case_body_2691; else goto switch_check_2701;

  switch_check_2701:
  r15 = r7 == 7;
  if (r15) goto case_body_2692; else goto switch_check_2702;

  switch_check_2702:
  r16 = r7 == 8;
  if (r16) goto case_body_2693; else goto case_default_2694;

  case_body_2685:
  r17 = cn_var_操作数类型_0;
  r18 = 是数值类型(r17);
  if (r18) goto if_then_2703; else goto if_merge_2704;

  if_then_2703:
  r19 = cn_var_操作数类型_0;
  return r19;
  goto if_merge_2704;

  if_merge_2704:
  r20 = cn_var_上下文;
  r21 = cn_var_一元节点;
  r22 = r21->节点基类;
  r23 = r22.位置;
  报告错误(r20, r23, 23);
  return 0;
  goto switch_merge_2684;

  case_body_2686:
  r24 = 创建布尔类型();
  return r24;
  goto switch_merge_2684;

  case_body_2687:
  r25 = cn_var_操作数类型_0;
  r26 = 是整数类型(r25);
  if (r26) goto if_then_2705; else goto if_merge_2706;

  if_then_2705:
  r27 = cn_var_操作数类型_0;
  return r27;
  goto if_merge_2706;

  if_merge_2706:
  r28 = cn_var_上下文;
  r29 = cn_var_一元节点;
  r30 = r29->节点基类;
  r31 = r30.位置;
  报告错误(r28, r31, 23);
  return 0;
  goto switch_merge_2684;

  case_body_2688:
  r32 = cn_var_操作数类型_0;
  r33 = 创建指针类型(r32);
  return r33;
  goto switch_merge_2684;

  case_body_2689:
  r34 = cn_var_操作数类型_0;
  r35 = 是指针类型(r34);
  if (r35) goto if_then_2707; else goto if_merge_2708;

  if_then_2707:
  r36 = cn_var_操作数类型_0;
  r37 = r36.指向类型;
  return r37;
  goto if_merge_2708;

  if_merge_2708:
  r38 = cn_var_上下文;
  r39 = cn_var_一元节点;
  r40 = r39->节点基类;
  r41 = r40.位置;
  报告错误(r38, r41, 23);
  return 0;
  goto switch_merge_2684;

  case_body_2690:
  goto switch_merge_2684;

  case_body_2691:
  goto switch_merge_2684;

  case_body_2692:
  goto switch_merge_2684;

  case_body_2693:
  r42 = cn_var_操作数类型_0;
  r43 = 是数值类型(r42);
  if (r43) goto if_then_2709; else goto if_merge_2710;

  if_then_2709:
  r44 = cn_var_操作数类型_0;
  return r44;
  goto if_merge_2710;

  if_merge_2710:
  r45 = cn_var_上下文;
  r46 = cn_var_一元节点;
  r47 = r46->节点基类;
  r48 = r47.位置;
  报告错误(r45, r48, 23);
  return 0;
  goto switch_merge_2684;

  case_default_2694:
  return 0;
  goto switch_merge_2684;

  switch_merge_2684:
  return NULL;
}

struct 类型信息* 检查字面量表达式(struct 语义检查上下文* cn_var_上下文, struct 字面量表达式* cn_var_字面量节点) {
  long long r1, r9, r11;
  struct 字面量表达式* r0;
  struct 字面量表达式* r2;
  struct 类型信息* r10;
  struct 类型信息* r12;
  struct 类型信息* r13;
  struct 类型信息* r14;
  struct 类型信息* r15;
  struct 类型信息* r16;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  enum 字面量类型 r3;

  entry:
  r0 = cn_var_字面量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2711; else goto if_merge_2712;

  if_then_2711:
  return 0;
  goto if_merge_2712;

  if_merge_2712:
  r2 = cn_var_字面量节点;
  r3 = r2->类型;
  r4 = r3 == /* NONE */;
  if (r4) goto case_body_2714; else goto switch_check_2720;

  switch_check_2720:
  r5 = r3 == /* NONE */;
  if (r5) goto case_body_2715; else goto switch_check_2721;

  switch_check_2721:
  r6 = r3 == /* NONE */;
  if (r6) goto case_body_2716; else goto switch_check_2722;

  switch_check_2722:
  r7 = r3 == /* NONE */;
  if (r7) goto case_body_2717; else goto switch_check_2723;

  switch_check_2723:
  r8 = r3 == /* NONE */;
  if (r8) goto case_body_2718; else goto case_default_2719;

  case_body_2714:
  r9 = cn_var_整数大小;
  r10 = 创建整数类型("整数", r9, 1);
  return r10;
  goto switch_merge_2713;

  case_body_2715:
  r11 = cn_var_小数大小;
  r12 = 创建小数类型("小数", r11);
  return r12;
  goto switch_merge_2713;

  case_body_2716:
  r13 = 创建字符串类型();
  return r13;
  goto switch_merge_2713;

  case_body_2717:
  r14 = 创建布尔类型();
  return r14;
  goto switch_merge_2713;

  case_body_2718:
  r15 = 创建空类型();
  r16 = 创建指针类型(r15);
  return r16;
  goto switch_merge_2713;

  case_default_2719:
  return 0;
  goto switch_merge_2713;

  switch_merge_2713:
  return NULL;
}

struct 类型信息* 检查标识符表达式(struct 语义检查上下文* cn_var_上下文, struct 标识符表达式* cn_var_标识符节点) {
  long long r1, r7, r8, r13, r14, r15, r16, r17;
  char* r5;
  struct 标识符表达式* r0;
  struct 语义检查上下文* r2;
  struct 符号表管理器* r3;
  struct 标识符表达式* r4;
  struct 符号* r6;
  struct 语义检查上下文* r9;
  struct 标识符表达式* r10;
  struct 语义检查上下文* r18;
  struct 类型推断上下文* r19;
  struct 类型信息* r20;
  struct AST节点 r11;
  struct 源位置 r12;

  entry:
  r0 = cn_var_标识符节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2724; else goto if_merge_2725;

  if_then_2724:
  return 0;
  goto if_merge_2725;

  if_merge_2725:
  long long cn_var_符号指针_0;
  r2 = cn_var_上下文;
  r3 = r2->符号表;
  r4 = cn_var_标识符节点;
  r5 = r4->名称;
  r6 = 查找符号(r3, r5);
  cn_var_符号指针_0 = r6;
  r7 = cn_var_符号指针_0;
  r8 = r7 == 0;
  if (r8) goto if_then_2726; else goto if_merge_2727;

  if_then_2726:
  r9 = cn_var_上下文;
  r10 = cn_var_标识符节点;
  r11 = r10->节点基类;
  r12 = r11.位置;
  报告错误(r9, r12, 22);
  return 0;
  goto if_merge_2727;

  if_merge_2727:
  r13 = cn_var_符号指针_0;
  r14 = r13.类型信息;
  r15 = r14 != 0;
  if (r15) goto if_then_2728; else goto if_merge_2729;

  if_then_2728:
  r16 = cn_var_符号指针_0;
  r17 = r16.类型信息;
  r18 = cn_var_上下文;
  r19 = r18->类型上下文;
  r20 = 从类型节点推断(r17, r19);
  return r20;
  goto if_merge_2729;

  if_merge_2729:
  return 0;
}

struct 类型信息* 检查函数调用表达式(struct 语义检查上下文* cn_var_上下文, struct 函数调用表达式* cn_var_调用节点) {
  long long r1, r6, r7, r8, r9, r11, r17, r18, r19, r20, r25, r26, r28, r29, r30, r31, r32, r33, r37, r40, r41, r42, r44, r45, r47, r48, r52, r54, r55, r56, r57, r58, r59;
  void* r36;
  void* r51;
  struct 函数调用表达式* r0;
  struct 语义检查上下文* r2;
  struct 函数调用表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 函数调用表达式* r13;
  struct 函数调用表达式* r16;
  struct 语义检查上下文* r21;
  struct 函数调用表达式* r22;
  struct 函数调用表达式* r27;
  struct 语义检查上下文* r34;
  struct 函数调用表达式* r35;
  void* r38;
  struct 类型信息* r39;
  void* r43;
  struct 语义检查上下文* r49;
  struct 函数调用表达式* r50;
  void* r53;
  _Bool r10;
  struct AST节点 r14;
  struct 源位置 r15;
  struct AST节点 r23;
  struct 源位置 r24;
  enum 类型兼容性 r46;

  entry:
  r0 = cn_var_调用节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2730; else goto if_merge_2731;

  if_then_2730:
  return 0;
  goto if_merge_2731;

  if_merge_2731:
  long long cn_var_函数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_调用节点;
  r4 = r3->被调函数;
  r5 = 检查表达式(r2, r4);
  cn_var_函数类型_0 = r5;
  r7 = cn_var_函数类型_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_2735; else goto logic_rhs_2734;

  if_then_2732:
  r12 = cn_var_上下文;
  r13 = cn_var_调用节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  报告错误(r12, r15, 30);
  return 0;
  goto if_merge_2733;

  if_merge_2733:
  r16 = cn_var_调用节点;
  r17 = r16->参数个数;
  r18 = cn_var_函数类型_0;
  r19 = r18.参数个数;
  r20 = r17 != r19;
  if (r20) goto if_then_2736; else goto if_merge_2737;

  logic_rhs_2734:
  r9 = cn_var_函数类型_0;
  r10 = 是可调用类型(r9);
  r11 = r10 == 0;
  goto logic_merge_2735;

  logic_merge_2735:
  if (r11) goto if_then_2732; else goto if_merge_2733;

  if_then_2736:
  r21 = cn_var_上下文;
  r22 = cn_var_调用节点;
  r23 = r22->节点基类;
  r24 = r23.位置;
  报告错误(r21, r24, 24);
  goto if_merge_2737;

  if_merge_2737:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2738;

  for_cond_2738:
  r26 = cn_var_i_1;
  r27 = cn_var_调用节点;
  r28 = r27->参数个数;
  r29 = r26 < r28;
  if (r29) goto logic_rhs_2742; else goto logic_merge_2743;

  for_body_2739:
  long long cn_var_实参类型_2;
  r34 = cn_var_上下文;
  r35 = cn_var_调用节点;
  r36 = r35->参数;
  r37 = cn_var_i_1;
  r38 = *(void**)cn_rt_array_get_element(r36, r37, 8);
  r39 = 检查表达式(r34, r38);
  cn_var_实参类型_2 = r39;
  long long cn_var_形参类型_3;
  r40 = cn_var_函数类型_0;
  r41 = r40.参数类型列表;
  r42 = cn_var_i_1;
  r43 = *(void**)cn_rt_array_get_element(r41, r42, 8);
  cn_var_形参类型_3 = r43;
  long long cn_var_兼容性_4;
  r44 = cn_var_实参类型_2;
  r45 = cn_var_形参类型_3;
  r46 = 检查类型兼容性(r44, r45);
  cn_var_兼容性_4 = r46;
  r47 = cn_var_兼容性_4;
  r48 = r47 == 3;
  if (r48) goto if_then_2744; else goto if_merge_2745;

  for_update_2740:
  r56 = cn_var_i_1;
  r57 = r56 + 1;
  cn_var_i_1 = r57;
  goto for_cond_2738;

  for_exit_2741:
  r58 = cn_var_函数类型_0;
  r59 = r58.返回类型;
  return r59;

  logic_rhs_2742:
  r30 = cn_var_i_1;
  r31 = cn_var_函数类型_0;
  r32 = r31.参数个数;
  r33 = r30 < r32;
  goto logic_merge_2743;

  logic_merge_2743:
  if (r33) goto for_body_2739; else goto for_exit_2741;

  if_then_2744:
  r49 = cn_var_上下文;
  r50 = cn_var_调用节点;
  r51 = r50->参数;
  r52 = cn_var_i_1;
  r53 = *(void**)cn_rt_array_get_element(r51, r52, 8);
  r54 = r53->节点基类;
  r55 = r54.位置;
  报告错误(r49, r55, 25);
  goto if_merge_2745;

  if_merge_2745:
  goto for_update_2740;
  return NULL;
}

struct 类型信息* 检查成员访问表达式(struct 语义检查上下文* cn_var_上下文, struct 成员访问表达式* cn_var_成员节点) {
  long long r1, r6, r7, r10, r12, r17, r18, r19, r20, r22, r24, r25, r29, r30, r35, r40, r45, r46, r47, r48, r49;
  char* r27;
  struct 成员访问表达式* r0;
  struct 语义检查上下文* r2;
  struct 成员访问表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 成员访问表达式* r8;
  struct 语义检查上下文* r13;
  struct 成员访问表达式* r14;
  struct 成员访问表达式* r26;
  struct 符号* r28;
  struct 语义检查上下文* r31;
  struct 成员访问表达式* r32;
  struct 语义检查上下文* r36;
  struct 符号表管理器* r37;
  struct 作用域* r38;
  struct 语义检查上下文* r41;
  struct 成员访问表达式* r42;
  struct 语义检查上下文* r50;
  struct 类型推断上下文* r51;
  struct 类型信息* r52;
  _Bool r9;
  _Bool r11;
  _Bool r21;
  _Bool r23;
  _Bool r39;
  struct AST节点 r15;
  struct 源位置 r16;
  struct AST节点 r33;
  struct 源位置 r34;
  struct AST节点 r43;
  struct 源位置 r44;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2746; else goto if_merge_2747;

  if_then_2746:
  return 0;
  goto if_merge_2747;

  if_merge_2747:
  long long cn_var_对象类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_成员节点;
  r4 = r3->对象;
  r5 = 检查表达式(r2, r4);
  cn_var_对象类型_0 = r5;
  r6 = cn_var_对象类型_0;
  r7 = r6 == 0;
  if (r7) goto if_then_2748; else goto if_merge_2749;

  if_then_2748:
  return 0;
  goto if_merge_2749;

  if_merge_2749:
  r8 = cn_var_成员节点;
  r9 = r8->是指针访问;
  if (r9) goto if_then_2750; else goto if_merge_2751;

  if_then_2750:
  r10 = cn_var_对象类型_0;
  r11 = 是指针类型(r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2752; else goto if_merge_2753;

  if_merge_2751:
  long long cn_var_成员符号_1;
  cn_var_成员符号_1 = 0;
  r20 = cn_var_对象类型_0;
  r21 = 是结构体类型(r20);
  if (r21) goto logic_merge_2757; else goto logic_rhs_2756;

  if_then_2752:
  r13 = cn_var_上下文;
  r14 = cn_var_成员节点;
  r15 = r14->节点基类;
  r16 = r15.位置;
  报告错误(r13, r16, 23);
  return 0;
  goto if_merge_2753;

  if_merge_2753:
  r17 = cn_var_对象类型_0;
  r18 = r17.指向类型;
  cn_var_对象类型_0 = r18;
  goto if_merge_2751;

  if_then_2754:
  r24 = cn_var_对象类型_0;
  r25 = r24.类型符号;
  r26 = cn_var_成员节点;
  r27 = r26->成员名;
  r28 = 查找类成员(r25, r27);
  cn_var_成员符号_1 = r28;
  goto if_merge_2755;

  if_merge_2755:
  r29 = cn_var_成员符号_1;
  r30 = r29 == 0;
  if (r30) goto if_then_2758; else goto if_merge_2759;

  logic_rhs_2756:
  r22 = cn_var_对象类型_0;
  r23 = 是类类型(r22);
  goto logic_merge_2757;

  logic_merge_2757:
  if (r23) goto if_then_2754; else goto if_merge_2755;

  if_then_2758:
  r31 = cn_var_上下文;
  r32 = cn_var_成员节点;
  r33 = r32->节点基类;
  r34 = r33.位置;
  报告错误(r31, r34, 32);
  return 0;
  goto if_merge_2759;

  if_merge_2759:
  r35 = cn_var_成员符号_1;
  r36 = cn_var_上下文;
  r37 = r36->符号表;
  r38 = r37->当前作用域;
  r39 = 检查符号可访问性(r35, r38);
  r40 = r39 == 0;
  if (r40) goto if_then_2760; else goto if_merge_2761;

  if_then_2760:
  r41 = cn_var_上下文;
  r42 = cn_var_成员节点;
  r43 = r42->节点基类;
  r44 = r43.位置;
  报告错误(r41, r44, 33);
  goto if_merge_2761;

  if_merge_2761:
  r45 = cn_var_成员符号_1;
  r46 = r45.类型信息;
  r47 = r46 != 0;
  if (r47) goto if_then_2762; else goto if_merge_2763;

  if_then_2762:
  r48 = cn_var_成员符号_1;
  r49 = r48.类型信息;
  r50 = cn_var_上下文;
  r51 = r50->类型上下文;
  r52 = 从类型节点推断(r49, r51);
  return r52;
  goto if_merge_2763;

  if_merge_2763:
  return 0;
}

struct 类型信息* 检查数组访问表达式(struct 语义检查上下文* cn_var_上下文, struct 数组访问表达式* cn_var_数组节点) {
  long long r1, r10, r11, r13, r14, r16, r21, r23, r27, r28, r29, r31, r32, r33, r34;
  struct 数组访问表达式* r0;
  struct 语义检查上下文* r2;
  struct 数组访问表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 数组访问表达式* r7;
  struct 表达式节点* r8;
  struct 类型信息* r9;
  struct 语义检查上下文* r17;
  struct 数组访问表达式* r18;
  struct 语义检查上下文* r24;
  struct 数组访问表达式* r25;
  struct 表达式节点* r26;
  _Bool r12;
  _Bool r15;
  _Bool r22;
  _Bool r30;
  struct AST节点 r19;
  struct 源位置 r20;

  entry:
  r0 = cn_var_数组节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2764; else goto if_merge_2765;

  if_then_2764:
  return 0;
  goto if_merge_2765;

  if_merge_2765:
  long long cn_var_数组类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_数组节点;
  r4 = r3->数组;
  r5 = 检查表达式(r2, r4);
  cn_var_数组类型_0 = r5;
  long long cn_var_索引类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_数组节点;
  r8 = r7->索引;
  r9 = 检查表达式(r6, r8);
  cn_var_索引类型_1 = r9;
  r11 = cn_var_数组类型_0;
  r12 = 是数组类型(r11);
  r13 = r12 == 0;
  if (r13) goto logic_rhs_2768; else goto logic_merge_2769;

  if_then_2766:
  r17 = cn_var_上下文;
  r18 = cn_var_数组节点;
  r19 = r18->节点基类;
  r20 = r19.位置;
  报告错误(r17, r20, 31);
  return 0;
  goto if_merge_2767;

  if_merge_2767:
  r21 = cn_var_索引类型_1;
  r22 = 是整数类型(r21);
  r23 = r22 == 0;
  if (r23) goto if_then_2770; else goto if_merge_2771;

  logic_rhs_2768:
  r14 = cn_var_数组类型_0;
  r15 = 是指针类型(r14);
  r16 = r15 == 0;
  goto logic_merge_2769;

  logic_merge_2769:
  if (r16) goto if_then_2766; else goto if_merge_2767;

  if_then_2770:
  r24 = cn_var_上下文;
  r25 = cn_var_数组节点;
  r26 = r25->索引;
  r27 = r26->节点基类;
  r28 = r27.位置;
  报告错误(r24, r28, 23);
  goto if_merge_2771;

  if_merge_2771:
  r29 = cn_var_数组类型_0;
  r30 = 是数组类型(r29);
  if (r30) goto if_then_2772; else goto if_else_2773;

  if_then_2772:
  r31 = cn_var_数组类型_0;
  r32 = r31.元素类型;
  return r32;
  goto if_merge_2774;

  if_else_2773:
  r33 = cn_var_数组类型_0;
  r34 = r33.指向类型;
  return r34;
  goto if_merge_2774;

  if_merge_2774:
  return NULL;
}

struct 类型信息* 检查赋值表达式(struct 语义检查上下文* cn_var_上下文, struct 赋值表达式* cn_var_赋值节点) {
  long long r1, r10, r11, r12, r13, r14, r15, r16, r18, r19, r24;
  struct 赋值表达式* r0;
  struct 语义检查上下文* r2;
  struct 赋值表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 赋值表达式* r7;
  struct 表达式节点* r8;
  struct 类型信息* r9;
  struct 语义检查上下文* r20;
  struct 赋值表达式* r21;
  enum 类型兼容性 r17;
  struct AST节点 r22;
  struct 源位置 r23;

  entry:
  r0 = cn_var_赋值节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2775; else goto if_merge_2776;

  if_then_2775:
  return 0;
  goto if_merge_2776;

  if_merge_2776:
  long long cn_var_目标类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_赋值节点;
  r4 = r3->目标;
  r5 = 检查表达式(r2, r4);
  cn_var_目标类型_0 = r5;
  long long cn_var_值类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_赋值节点;
  r8 = r7->值;
  r9 = 检查表达式(r6, r8);
  cn_var_值类型_1 = r9;
  r11 = cn_var_目标类型_0;
  r12 = r11 != 0;
  if (r12) goto logic_rhs_2779; else goto logic_merge_2780;

  if_then_2777:
  long long cn_var_兼容性_2;
  r15 = cn_var_值类型_1;
  r16 = cn_var_目标类型_0;
  r17 = 检查类型兼容性(r15, r16);
  cn_var_兼容性_2 = r17;
  r18 = cn_var_兼容性_2;
  r19 = r18 == 3;
  if (r19) goto if_then_2781; else goto if_merge_2782;

  if_merge_2778:
  r24 = cn_var_目标类型_0;
  return r24;

  logic_rhs_2779:
  r13 = cn_var_值类型_1;
  r14 = r13 != 0;
  goto logic_merge_2780;

  logic_merge_2780:
  if (r14) goto if_then_2777; else goto if_merge_2778;

  if_then_2781:
  r20 = cn_var_上下文;
  r21 = cn_var_赋值节点;
  r22 = r21->节点基类;
  r23 = r22.位置;
  报告错误(r20, r23, 29);
  goto if_merge_2782;

  if_merge_2782:
  goto if_merge_2778;
  return NULL;
}

struct 类型信息* 检查三元表达式(struct 语义检查上下文* cn_var_上下文, struct 三元表达式* cn_var_三元节点) {
  long long r1, r14, r15, r16, r17, r18, r19, r23, r24, r25, r26, r27, r28, r29, r30, r31, r33, r34, r39;
  struct 三元表达式* r0;
  struct 语义检查上下文* r2;
  struct 三元表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 三元表达式* r7;
  struct 表达式节点* r8;
  struct 类型信息* r9;
  struct 语义检查上下文* r10;
  struct 三元表达式* r11;
  struct 表达式节点* r12;
  struct 类型信息* r13;
  struct 语义检查上下文* r20;
  struct 三元表达式* r21;
  struct 表达式节点* r22;
  struct 语义检查上下文* r35;
  struct 三元表达式* r36;
  enum 类型兼容性 r32;
  struct AST节点 r37;
  struct 源位置 r38;

  entry:
  r0 = cn_var_三元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2783; else goto if_merge_2784;

  if_then_2783:
  return 0;
  goto if_merge_2784;

  if_merge_2784:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_三元节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  long long cn_var_真值类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_三元节点;
  r8 = r7->真值;
  r9 = 检查表达式(r6, r8);
  cn_var_真值类型_1 = r9;
  long long cn_var_假值类型_2;
  r10 = cn_var_上下文;
  r11 = cn_var_三元节点;
  r12 = r11->假值;
  r13 = 检查表达式(r10, r12);
  cn_var_假值类型_2 = r13;
  r15 = cn_var_条件类型_0;
  r16 = r15 != 0;
  if (r16) goto logic_rhs_2787; else goto logic_merge_2788;

  if_then_2785:
  r20 = cn_var_上下文;
  r21 = cn_var_三元节点;
  r22 = r21->条件;
  r23 = r22->节点基类;
  r24 = r23.位置;
  报告错误(r20, r24, 23);
  goto if_merge_2786;

  if_merge_2786:
  r26 = cn_var_真值类型_1;
  r27 = r26 != 0;
  if (r27) goto logic_rhs_2791; else goto logic_merge_2792;

  logic_rhs_2787:
  r17 = cn_var_条件类型_0;
  r18 = r17.种类;
  r19 = r18 != /* NONE */;
  goto logic_merge_2788;

  logic_merge_2788:
  if (r19) goto if_then_2785; else goto if_merge_2786;

  if_then_2789:
  long long cn_var_兼容性_3;
  r30 = cn_var_真值类型_1;
  r31 = cn_var_假值类型_2;
  r32 = 检查类型兼容性(r30, r31);
  cn_var_兼容性_3 = r32;
  r33 = cn_var_兼容性_3;
  r34 = r33 == 3;
  if (r34) goto if_then_2793; else goto if_merge_2794;

  if_merge_2790:
  r39 = cn_var_真值类型_1;
  return r39;

  logic_rhs_2791:
  r28 = cn_var_假值类型_2;
  r29 = r28 != 0;
  goto logic_merge_2792;

  logic_merge_2792:
  if (r29) goto if_then_2789; else goto if_merge_2790;

  if_then_2793:
  r35 = cn_var_上下文;
  r36 = cn_var_三元节点;
  r37 = r36->节点基类;
  r38 = r37.位置;
  报告错误(r35, r38, 23);
  goto if_merge_2794;

  if_merge_2794:
  goto if_merge_2790;
  return NULL;
}

_Bool 是常量表达式(struct 表达式节点* cn_var_表达式) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  goto tail_rec_loop;
  return 0;
}

void 报告错误(struct 语义检查上下文* cn_var_上下文, struct 源位置 cn_var_位置, enum 诊断错误码 cn_var_错误码) {
  long long r1, r2;
  struct 语义检查上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0->错误计数;
  r2 = r1 + 1;
  return;
}

void 报告警告(struct 语义检查上下文* cn_var_上下文, struct 源位置 cn_var_位置, char* cn_var_消息) {
  long long r1, r2;
  struct 语义检查上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0->警告计数;
  r2 = r1 + 1;
  return;
}

long long 语义检查上下文大小() {

  entry:
  return 128;
}

