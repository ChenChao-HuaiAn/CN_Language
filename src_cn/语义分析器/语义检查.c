#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
enum 逻辑运算符 {
    逻辑运算符_逻辑_或 = 1,
    逻辑运算符_逻辑_与 = 0
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
enum 字面量类型 {
    字面量类型_字面量_字符 = 5,
    字面量类型_字面量_空 = 4,
    字面量类型_字面量_布尔 = 3,
    字面量类型_字面量_字符串 = 2,
    字面量类型_字面量_浮点 = 1,
    字面量类型_字面量_整数 = 0
};
enum 可见性 {
    可见性_可见性_私有 = 2,
    可见性_可见性_公开 = 1,
    可见性_可见性_默认 = 0
};
enum 导入类型 {
    导入类型_导入_从包导入模块 = 5,
    导入类型_导入_从模块通配符 = 4,
    导入类型_导入_从模块选择性 = 3,
    导入类型_导入_别名 = 2,
    导入类型_导入_选择性 = 1,
    导入类型_导入_全量 = 0
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

// Struct Definitions - 从导入模块
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};
struct 声明节点列表;
struct AST节点 {
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
struct 类型节点;
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
};
struct 表达式节点;
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
struct 块语句;
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
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 结构体声明;
struct 结构体声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 结构体成员** 成员;
    long long 成员个数;
};
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 枚举声明;
struct 枚举声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 枚举成员** 成员;
    long long 成员个数;
};
struct 节点类型 {
};
struct 类成员;
struct 类成员 {
    char* 名称;
    struct 节点类型 类型;
    struct 变量声明* 字段;
    struct 函数声明* 方法;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
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
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
struct 接口声明;
struct 接口声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 接口方法** 方法;
    long long 方法个数;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
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
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
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
struct 声明节点;
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct 声明节点;
struct 声明节点 {
    struct 节点类型 类型;
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
struct 表达式语句;
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};
struct 语句节点;
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
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};
struct 选择语句;
struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
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
struct 逻辑表达式;
struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 结构体字面量表达式;
struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};
struct 模板实例化表达式;
struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
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

// Forward Declarations - 从导入模块
struct 程序节点* 创建程序节点(void);
void 程序添加声明(struct 程序节点*, struct 声明节点*);
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
void 检查情况语句(struct 语义检查上下文*, struct 情况语句*);
void 检查尝试语句(struct 语义检查上下文*, struct 尝试语句*);
void 检查捕获语句(struct 语义检查上下文*, struct 捕获语句*);
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
  if (r1) goto if_then_2297; else goto if_merge_2298;

  if_then_2297:
  return;
  goto if_merge_2298;

  if_merge_2298:
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
  if (r1) goto if_then_2299; else goto if_merge_2300;

  if_then_2299:
  return 0;
  goto if_merge_2300;

  if_merge_2300:
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
  long long r1, r2, r4, r5, r9, r10, r11, r12;
  struct 程序节点* r0;
  struct 程序节点* r3;
  struct 语义检查上下文* r6;
  struct 程序节点* r7;
  struct 声明节点列表* r8;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2301; else goto if_merge_2302;

  if_then_2301:
  return;
  goto if_merge_2302;

  if_merge_2302:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2303;

  for_cond_2303:
  r2 = cn_var_i_0;
  r3 = cn_var_程序;
  r4 = r3->声明个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2304; else goto for_exit_2306;

  for_body_2304:
  r6 = cn_var_上下文;
  r7 = cn_var_程序;
  r8 = r7->声明列表;
  r9 = cn_var_i_0;
  r10 = *(void*)cn_rt_array_get_element(r8, r9, 8);
  检查声明(r6, r10);
  goto for_update_2305;

  for_update_2305:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2303;

  for_exit_2306:
  return;
}

void 检查声明(struct 语义检查上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r1, r3, r4, r5, r6, r7, r9, r10, r12, r13, r15, r16, r18, r19, r21, r22;
  struct 声明节点* r0;
  struct 声明节点* r2;
  struct 声明节点* r24;
  struct 语义检查上下文* r25;
  struct 函数声明* r26;
  struct 声明节点* r27;
  struct 语义检查上下文* r28;
  struct 变量声明* r29;
  struct 声明节点* r30;
  struct 语义检查上下文* r31;
  struct 结构体声明* r32;
  struct 声明节点* r33;
  struct 语义检查上下文* r34;
  struct 枚举声明* r35;
  struct 声明节点* r36;
  struct 语义检查上下文* r37;
  struct 类声明* r38;
  struct 声明节点* r39;
  struct 语义检查上下文* r40;
  struct 接口声明* r41;
  _Bool r8;
  _Bool r11;
  _Bool r14;
  _Bool r17;
  _Bool r20;
  _Bool r23;

  entry:
  r0 = cn_var_声明;
  r1 = r0 == 0;
  if (r1) goto if_then_2307; else goto if_merge_2308;

  if_then_2307:
  return;
  goto if_merge_2308;

  if_merge_2308:
  void cn_var_节点类型值_0;
  r2 = cn_var_声明;
  r3 = r2->节点基类;
  r4 = r3.类型;
  cn_var_节点类型值_0 = r4;
  r5 = cn_var_节点类型值_0;
  r6 = cn_var_节点类型;
  r7 = r6.函数声明;
  r8 = r5 == r7;
  if (r8) goto case_body_2310; else goto switch_check_2317;

  switch_check_2317:
  r9 = cn_var_节点类型;
  r10 = r9.变量声明;
  r11 = r5 == r10;
  if (r11) goto case_body_2311; else goto switch_check_2318;

  switch_check_2318:
  r12 = cn_var_节点类型;
  r13 = r12.结构体声明;
  r14 = r5 == r13;
  if (r14) goto case_body_2312; else goto switch_check_2319;

  switch_check_2319:
  r15 = cn_var_节点类型;
  r16 = r15.枚举声明;
  r17 = r5 == r16;
  if (r17) goto case_body_2313; else goto switch_check_2320;

  switch_check_2320:
  r18 = cn_var_节点类型;
  r19 = r18.类声明;
  r20 = r5 == r19;
  if (r20) goto case_body_2314; else goto switch_check_2321;

  switch_check_2321:
  r21 = cn_var_节点类型;
  r22 = r21.接口声明;
  r23 = r5 == r22;
  if (r23) goto case_body_2315; else goto case_default_2316;

  case_body_2310:
  struct 函数声明* cn_var_函数节点指针_1;
  r24 = cn_var_声明;
  cn_var_函数节点指针_1 = (struct 函数声明*)cn_var_声明;
  r25 = cn_var_上下文;
  r26 = cn_var_函数节点指针_1;
  检查函数声明(r25, r26);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_body_2311:
  struct 变量声明* cn_var_变量节点指针_2;
  r27 = cn_var_声明;
  cn_var_变量节点指针_2 = (struct 变量声明*)cn_var_声明;
  r28 = cn_var_上下文;
  r29 = cn_var_变量节点指针_2;
  检查变量声明(r28, r29);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_body_2312:
  struct 结构体声明* cn_var_结构体节点指针_3;
  r30 = cn_var_声明;
  cn_var_结构体节点指针_3 = (struct 结构体声明*)cn_var_声明;
  r31 = cn_var_上下文;
  r32 = cn_var_结构体节点指针_3;
  检查结构体声明(r31, r32);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_body_2313:
  struct 枚举声明* cn_var_枚举节点指针_4;
  r33 = cn_var_声明;
  cn_var_枚举节点指针_4 = (struct 枚举声明*)cn_var_声明;
  r34 = cn_var_上下文;
  r35 = cn_var_枚举节点指针_4;
  检查枚举声明(r34, r35);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_body_2314:
  struct 类声明* cn_var_类节点指针_5;
  r36 = cn_var_声明;
  cn_var_类节点指针_5 = (struct 类声明*)cn_var_声明;
  r37 = cn_var_上下文;
  r38 = cn_var_类节点指针_5;
  检查类声明(r37, r38);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_body_2315:
  struct 接口声明* cn_var_接口节点指针_6;
  r39 = cn_var_声明;
  cn_var_接口节点指针_6 = (struct 接口声明*)cn_var_声明;
  r40 = cn_var_上下文;
  r41 = cn_var_接口节点指针_6;
  检查接口声明(r40, r41);
  goto switch_merge_2309;
  goto switch_merge_2309;

  case_default_2316:
  goto switch_merge_2309;
  goto switch_merge_2309;

  switch_merge_2309:
  return;
}

void 检查函数声明(struct 语义检查上下文* cn_var_上下文, struct 函数声明* cn_var_函数节点) {
  long long r1, r18, r20, r21, r25, r26, r27, r28, r31, r35, r38, r43, r44, r45, r48, r51, r55, r57, r58, r59, r60;
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
  struct 函数声明* r29;
  struct 块语句* r30;
  struct 语义检查上下文* r32;
  struct 函数声明* r33;
  struct 块语句* r34;
  struct 语义检查上下文* r36;
  struct 类型信息* r37;
  struct 语义检查上下文* r39;
  struct 类型信息* r40;
  struct 语义检查上下文* r46;
  struct 函数声明* r49;
  struct 语义检查上下文* r52;
  struct 函数声明* r53;
  _Bool r7;
  _Bool r47;
  _Bool r50;
  struct AST节点 r54;
  enum 类型种类 r41;
  enum 类型种类 r42;
  enum 诊断错误码 r56;

  entry:
  r0 = cn_var_函数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2322; else goto if_merge_2323;

  if_then_2322:
  return;
  goto if_merge_2323;

  if_merge_2323:
  void cn_var_保存函数_0;
  r2 = cn_var_上下文;
  r3 = r2->当前函数;
  cn_var_保存函数_0 = r3;
  void cn_var_保存返回类型_1;
  r4 = cn_var_上下文;
  r5 = r4->期望返回类型;
  cn_var_保存返回类型_1 = r5;
  void cn_var_保存返回语句_2;
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
  goto for_cond_2324;

  for_cond_2324:
  r18 = cn_var_i_3;
  r19 = cn_var_函数节点;
  r20 = r19->参数个数;
  r21 = r18 < r20;
  if (r21) goto for_body_2325; else goto for_exit_2327;

  for_body_2325:
  r22 = cn_var_上下文;
  r23 = cn_var_函数节点;
  r24 = r23->参数列表;
  r25 = cn_var_i_3;
  r26 = *(void*)cn_rt_array_get_element(r24, r25, 8);
  检查参数(r22, r26);
  goto for_update_2326;

  for_update_2326:
  r27 = cn_var_i_3;
  r28 = r27 + 1;
  cn_var_i_3 = r28;
  goto for_cond_2324;

  for_exit_2327:
  r29 = cn_var_函数节点;
  r30 = r29->函数体;
  r31 = r30 != 0;
  if (r31) goto if_then_2328; else goto if_merge_2329;

  if_then_2328:
  r32 = cn_var_上下文;
  r33 = cn_var_函数节点;
  r34 = r33->函数体;
  检查语句(r32, (struct 语句节点*)cn_var_函数节点.函数体);
  goto if_merge_2329;

  if_merge_2329:
  r36 = cn_var_上下文;
  r37 = r36->期望返回类型;
  r38 = r37 != 0;
  if (r38) goto logic_rhs_2332; else goto logic_merge_2333;

  if_then_2330:
  r46 = cn_var_上下文;
  r47 = r46->有返回语句;
  r48 = r47 == 0;
  if (r48) goto logic_rhs_2336; else goto logic_merge_2337;

  if_merge_2331:
  r58 = cn_var_保存函数_0;
  r59 = cn_var_保存返回类型_1;
  r60 = cn_var_保存返回语句_2;

  logic_rhs_2332:
  r39 = cn_var_上下文;
  r40 = r39->期望返回类型;
  r41 = r40->种类;
  r42 = cn_var_类型种类;
  r43 = r42.类型_空;
  r44 = r41 != r43;
  goto logic_merge_2333;

  logic_merge_2333:
  if (r44) goto if_then_2330; else goto if_merge_2331;

  if_then_2334:
  r52 = cn_var_上下文;
  r53 = cn_var_函数节点;
  r54 = r53->节点基类;
  r55 = r54.位置;
  r56 = cn_var_诊断错误码;
  r57 = r56.语义_缺少返回语句;
  报告错误(r52, r55, r57);
  goto if_merge_2335;

  if_merge_2335:
  goto if_merge_2331;

  logic_rhs_2336:
  r49 = cn_var_函数节点;
  r50 = r49->是抽象;
  r51 = r50 == 0;
  goto logic_merge_2337;

  logic_merge_2337:
  if (r51) goto if_then_2334; else goto if_merge_2335;
  return;
}

void 检查参数(struct 语义检查上下文* cn_var_上下文, struct 参数* cn_var_参数节点) {
  long long r0, r2, r5, r11, r12, r15, r17;
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
  enum 诊断错误码 r16;

  entry:
  r1 = cn_var_参数节点;
  r2 = r1 == 0;
  if (r2) goto logic_merge_2341; else goto logic_rhs_2340;

  if_then_2338:
  return;
  goto if_merge_2339;

  if_merge_2339:
  void cn_var_类型_0;
  r6 = cn_var_参数节点;
  r7 = r6->类型;
  r8 = cn_var_上下文;
  r9 = r8->类型上下文;
  r10 = 从类型节点推断(r7, r9);
  cn_var_类型_0 = r10;
  r11 = cn_var_类型_0;
  r12 = r11 == 0;
  if (r12) goto if_then_2342; else goto if_merge_2343;

  logic_rhs_2340:
  r3 = cn_var_参数节点;
  r4 = r3->类型;
  r5 = r4 == 0;
  goto logic_merge_2341;

  logic_merge_2341:
  if (r5) goto if_then_2338; else goto if_merge_2339;

  if_then_2342:
  r13 = cn_var_上下文;
  r14 = cn_var_参数节点;
  r15 = r14->定义位置;
  r16 = cn_var_诊断错误码;
  r17 = r16.语义_未定义标识符;
  报告错误(r13, r15, r17);
  goto if_merge_2343;

  if_merge_2343:
  return;
}

void 检查变量声明(struct 语义检查上下文* cn_var_上下文, struct 变量声明* cn_var_变量节点) {
  long long r1, r6, r10, r12, r16, r20, r22, r27, r31, r35, r37, r38, r41, r44, r45, r46, r47, r50, r51, r52, r56, r58, r61, r67, r68, r72, r74, r77, r82, r85, r86, r87, r93, r94, r95, r96, r98, r100, r101, r105, r107;
  struct 变量声明* r0;
  struct 变量声明* r2;
  struct 变量声明* r4;
  struct 表达式节点* r5;
  struct 语义检查上下文* r7;
  struct 变量声明* r8;
  struct 变量声明* r13;
  struct 表达式节点* r14;
  struct 语义检查上下文* r17;
  struct 变量声明* r18;
  struct 变量声明* r23;
  struct 变量声明* r25;
  struct 表达式节点* r26;
  struct 变量声明* r28;
  struct 表达式节点* r29;
  struct 语义检查上下文* r32;
  struct 变量声明* r33;
  struct 变量声明* r39;
  struct 类型节点* r40;
  struct 变量声明* r42;
  struct 类型节点* r43;
  struct 变量声明* r48;
  struct 类型节点* r49;
  struct 语义检查上下文* r53;
  struct 变量声明* r54;
  struct 变量声明* r59;
  struct 类型节点* r60;
  struct 变量声明* r62;
  struct 类型节点* r63;
  struct 语义检查上下文* r64;
  struct 类型推断上下文* r65;
  struct 类型信息* r66;
  struct 语义检查上下文* r69;
  struct 变量声明* r70;
  struct 变量声明* r75;
  struct 表达式节点* r76;
  struct 语义检查上下文* r78;
  struct 变量声明* r79;
  struct 表达式节点* r80;
  struct 类型信息* r81;
  struct 变量声明* r83;
  struct 类型节点* r84;
  struct 变量声明* r88;
  struct 类型节点* r89;
  struct 语义检查上下文* r90;
  struct 类型推断上下文* r91;
  struct 类型信息* r92;
  struct 语义检查上下文* r102;
  struct 变量声明* r103;
  _Bool r3;
  _Bool r15;
  _Bool r24;
  _Bool r30;
  struct AST节点 r9;
  struct AST节点 r19;
  struct AST节点 r34;
  struct AST节点 r55;
  struct AST节点 r71;
  enum 类型兼容性 r97;
  struct AST节点 r104;
  enum 诊断错误码 r11;
  enum 诊断错误码 r21;
  enum 诊断错误码 r36;
  enum 诊断错误码 r57;
  enum 诊断错误码 r73;
  enum 类型兼容性 r99;
  enum 诊断错误码 r106;

  entry:
  r0 = cn_var_变量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2344; else goto if_merge_2345;

  if_then_2344:
  return;
  goto if_merge_2345;

  if_merge_2345:
  r2 = cn_var_变量节点;
  r3 = r2->是常量;
  if (r3) goto if_then_2346; else goto if_merge_2347;

  if_then_2346:
  r4 = cn_var_变量节点;
  r5 = r4->初始值;
  r6 = r5 == 0;
  if (r6) goto if_then_2348; else goto if_else_2349;

  if_merge_2347:
  r23 = cn_var_变量节点;
  r24 = r23->是静态;
  if (r24) goto if_then_2353; else goto if_merge_2354;

  if_then_2348:
  r7 = cn_var_上下文;
  r8 = cn_var_变量节点;
  r9 = r8->节点基类;
  r10 = r9.位置;
  r11 = cn_var_诊断错误码;
  r12 = r11.语义_常量缺少初始化;
  报告错误(r7, r10, r12);
  goto if_merge_2350;

  if_else_2349:
  r13 = cn_var_变量节点;
  r14 = r13->初始值;
  r15 = 是常量表达式(r14);
  r16 = r15 == 0;
  if (r16) goto if_then_2351; else goto if_merge_2352;

  if_merge_2350:
  goto if_merge_2347;

  if_then_2351:
  r17 = cn_var_上下文;
  r18 = cn_var_变量节点;
  r19 = r18->节点基类;
  r20 = r19.位置;
  r21 = cn_var_诊断错误码;
  r22 = r21.语义_常量非常量初始化;
  报告错误(r17, r20, r22);
  goto if_merge_2352;

  if_merge_2352:
  goto if_merge_2350;

  if_then_2353:
  r25 = cn_var_变量节点;
  r26 = r25->初始值;
  r27 = r26 != 0;
  if (r27) goto if_then_2355; else goto if_merge_2356;

  if_merge_2354:
  r59 = cn_var_变量节点;
  r60 = r59->类型;
  r61 = r60 != 0;
  if (r61) goto if_then_2365; else goto if_merge_2366;

  if_then_2355:
  r28 = cn_var_变量节点;
  r29 = r28->初始值;
  r30 = 是常量表达式(r29);
  r31 = r30 == 0;
  if (r31) goto if_then_2357; else goto if_merge_2358;

  if_merge_2356:
  r39 = cn_var_变量节点;
  r40 = r39->类型;
  r41 = r40 != 0;
  if (r41) goto logic_rhs_2361; else goto logic_merge_2362;

  if_then_2357:
  r32 = cn_var_上下文;
  r33 = cn_var_变量节点;
  r34 = r33->节点基类;
  r35 = r34.位置;
  r36 = cn_var_诊断错误码;
  r37 = r36.语义_静态非常量初始化;
  报告错误(r32, r35, r37);
  goto if_merge_2358;

  if_merge_2358:
  goto if_merge_2356;

  if_then_2359:
  r48 = cn_var_变量节点;
  r49 = r48->类型;
  r50 = r49->名称;
  r51 = 比较字符串(r50, "空类型");
  r52 = r51 == 0;
  if (r52) goto if_then_2363; else goto if_merge_2364;

  if_merge_2360:
  goto if_merge_2354;

  logic_rhs_2361:
  r42 = cn_var_变量节点;
  r43 = r42->类型;
  r44 = r43->类型;
  r45 = cn_var_节点类型;
  r46 = r45.基础类型;
  r47 = r44 == r46;
  goto logic_merge_2362;

  logic_merge_2362:
  if (r47) goto if_then_2359; else goto if_merge_2360;

  if_then_2363:
  r53 = cn_var_上下文;
  r54 = cn_var_变量节点;
  r55 = r54->节点基类;
  r56 = r55.位置;
  r57 = cn_var_诊断错误码;
  r58 = r57.语义_静态空类型;
  报告错误(r53, r56, r58);
  goto if_merge_2364;

  if_merge_2364:
  goto if_merge_2360;

  if_then_2365:
  void cn_var_类型_0;
  r62 = cn_var_变量节点;
  r63 = r62->类型;
  r64 = cn_var_上下文;
  r65 = r64->类型上下文;
  r66 = 从类型节点推断(r63, r65);
  cn_var_类型_0 = r66;
  r67 = cn_var_类型_0;
  r68 = r67 == 0;
  if (r68) goto if_then_2367; else goto if_merge_2368;

  if_merge_2366:
  r75 = cn_var_变量节点;
  r76 = r75->初始值;
  r77 = r76 != 0;
  if (r77) goto if_then_2369; else goto if_merge_2370;

  if_then_2367:
  r69 = cn_var_上下文;
  r70 = cn_var_变量节点;
  r71 = r70->节点基类;
  r72 = r71.位置;
  r73 = cn_var_诊断错误码;
  r74 = r73.语义_未定义标识符;
  报告错误(r69, r72, r74);
  goto if_merge_2368;

  if_merge_2368:
  goto if_merge_2366;

  if_then_2369:
  void cn_var_初始值类型_1;
  r78 = cn_var_上下文;
  r79 = cn_var_变量节点;
  r80 = r79->初始值;
  r81 = 检查表达式(r78, r80);
  cn_var_初始值类型_1 = r81;
  r83 = cn_var_变量节点;
  r84 = r83->类型;
  r85 = r84 != 0;
  if (r85) goto logic_rhs_2373; else goto logic_merge_2374;

  if_merge_2370:

  if_then_2371:
  void cn_var_变量类型_2;
  r88 = cn_var_变量节点;
  r89 = r88->类型;
  r90 = cn_var_上下文;
  r91 = r90->类型上下文;
  r92 = 从类型节点推断(r89, r91);
  cn_var_变量类型_2 = r92;
  r93 = cn_var_变量类型_2;
  r94 = r93 != 0;
  if (r94) goto if_then_2375; else goto if_merge_2376;

  if_merge_2372:
  goto if_merge_2370;

  logic_rhs_2373:
  r86 = cn_var_初始值类型_1;
  r87 = r86 != 0;
  goto logic_merge_2374;

  logic_merge_2374:
  if (r87) goto if_then_2371; else goto if_merge_2372;

  if_then_2375:
  void cn_var_兼容性_3;
  r95 = cn_var_初始值类型_1;
  r96 = cn_var_变量类型_2;
  r97 = 检查类型兼容性(r95, r96);
  cn_var_兼容性_3 = r97;
  r98 = cn_var_兼容性_3;
  r99 = cn_var_类型兼容性;
  r100 = r99.不兼容;
  r101 = r98 == r100;
  if (r101) goto if_then_2377; else goto if_merge_2378;

  if_merge_2376:
  goto if_merge_2372;

  if_then_2377:
  r102 = cn_var_上下文;
  r103 = cn_var_变量节点;
  r104 = r103->节点基类;
  r105 = r104.位置;
  r106 = cn_var_诊断错误码;
  r107 = r106.语义_类型不匹配;
  报告错误(r102, r105, r107);
  goto if_merge_2378;

  if_merge_2378:
  goto if_merge_2376;
  return;
}

void 检查结构体声明(struct 语义检查上下文* cn_var_上下文, struct 结构体声明* cn_var_结构体节点) {
  long long r1, r2, r4, r5, r8, r9, r10, r11, r12, r13, r14, r18, r19, r23, r25, r26, r27;
  void* r7;
  struct 结构体声明* r0;
  struct 结构体声明* r3;
  struct 结构体声明* r6;
  struct 语义检查上下文* r15;
  struct 类型推断上下文* r16;
  struct 类型信息* r17;
  struct 语义检查上下文* r20;
  struct 结构体声明* r21;
  struct AST节点 r22;
  enum 诊断错误码 r24;

  entry:
  r0 = cn_var_结构体节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2379; else goto if_merge_2380;

  if_then_2379:
  return;
  goto if_merge_2380;

  if_merge_2380:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2381;

  for_cond_2381:
  r2 = cn_var_i_0;
  r3 = cn_var_结构体节点;
  r4 = r3->成员个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2382; else goto for_exit_2384;

  for_body_2382:
  void cn_var_成员_1;
  r6 = cn_var_结构体节点;
  r7 = r6->成员;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  cn_var_成员_1 = r9;
  r10 = cn_var_成员_1;
  r11 = r10.类型;
  r12 = r11 != 0;
  if (r12) goto if_then_2385; else goto if_merge_2386;

  for_update_2383:
  r26 = cn_var_i_0;
  r27 = r26 + 1;
  cn_var_i_0 = r27;
  goto for_cond_2381;

  for_exit_2384:

  if_then_2385:
  void cn_var_类型_2;
  r13 = cn_var_成员_1;
  r14 = r13.类型;
  r15 = cn_var_上下文;
  r16 = r15->类型上下文;
  r17 = 从类型节点推断(r14, r16);
  cn_var_类型_2 = r17;
  r18 = cn_var_类型_2;
  r19 = r18 == 0;
  if (r19) goto if_then_2387; else goto if_merge_2388;

  if_merge_2386:
  goto for_update_2383;

  if_then_2387:
  r20 = cn_var_上下文;
  r21 = cn_var_结构体节点;
  r22 = r21->节点基类;
  r23 = r22.位置;
  r24 = cn_var_诊断错误码;
  r25 = r24.语义_未定义标识符;
  报告错误(r20, r23, r25);
  goto if_merge_2388;

  if_merge_2388:
  goto if_merge_2386;
  return;
}

void 检查枚举声明(struct 语义检查上下文* cn_var_上下文, struct 枚举声明* cn_var_枚举节点) {
  long long r1, r2, r4, r5, r6, r7, r8, r10, r11, r14, r15, r16, r19, r20, r21, r22, r23, r27, r29, r30, r31, r32, r33;
  void* r13;
  void* r18;
  struct 枚举声明* r0;
  struct 枚举声明* r3;
  struct 枚举声明* r9;
  struct 枚举声明* r12;
  struct 枚举声明* r17;
  struct 语义检查上下文* r24;
  struct 枚举声明* r25;
  struct AST节点 r26;
  enum 诊断错误码 r28;

  entry:
  r0 = cn_var_枚举节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2389; else goto if_merge_2390;

  if_then_2389:
  return;
  goto if_merge_2390;

  if_merge_2390:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2391;

  for_cond_2391:
  r2 = cn_var_i_0;
  r3 = cn_var_枚举节点;
  r4 = r3->成员个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2392; else goto for_exit_2394;

  for_body_2392:
  long long cn_var_j_1;
  r6 = cn_var_i_0;
  r7 = r6 + 1;
  cn_var_j_1 = r7;
  goto for_cond_2395;

  for_update_2393:
  r32 = cn_var_i_0;
  r33 = r32 + 1;
  cn_var_i_0 = r33;
  goto for_cond_2391;

  for_exit_2394:

  for_cond_2395:
  r8 = cn_var_j_1;
  r9 = cn_var_枚举节点;
  r10 = r9->成员个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2396; else goto for_exit_2398;

  for_body_2396:
  r12 = cn_var_枚举节点;
  r13 = r12->成员;
  r14 = cn_var_i_0;
  r15 = *(void*)cn_rt_array_get_element(r13, r14, 8);
  r16 = r15.名称;
  r17 = cn_var_枚举节点;
  r18 = r17->成员;
  r19 = cn_var_j_1;
  r20 = *(void*)cn_rt_array_get_element(r18, r19, 8);
  r21 = r20.名称;
  r22 = 比较字符串(r16, r21);
  r23 = r22 == 0;
  if (r23) goto if_then_2399; else goto if_merge_2400;

  for_update_2397:
  r30 = cn_var_j_1;
  r31 = r30 + 1;
  cn_var_j_1 = r31;
  goto for_cond_2395;

  for_exit_2398:
  goto for_update_2393;

  if_then_2399:
  r24 = cn_var_上下文;
  r25 = cn_var_枚举节点;
  r26 = r25->节点基类;
  r27 = r26.位置;
  r28 = cn_var_诊断错误码;
  r29 = r28.语义_重复符号;
  报告错误(r24, r27, r29);
  goto if_merge_2400;

  if_merge_2400:
  goto for_update_2397;
  return;
}

void 检查类声明(struct 语义检查上下文* cn_var_上下文, struct 类声明* cn_var_类节点) {
  long long r1, r2, r5, r8, r9, r15, r16, r17, r18, r19, r21, r22, r26, r28, r29, r31, r32, r37, r38, r40, r41, r42, r43, r44, r46, r47, r51, r53, r54, r55, r56, r58, r59, r63, r64, r65, r66;
  char* r4;
  char* r7;
  char* r13;
  void* r36;
  void* r62;
  struct 类声明* r0;
  struct 类声明* r3;
  struct 类声明* r6;
  struct 语义检查上下文* r10;
  struct 符号表管理器* r11;
  struct 类声明* r12;
  struct 符号* r14;
  struct 语义检查上下文* r23;
  struct 类声明* r24;
  struct 类声明* r30;
  struct 语义检查上下文* r33;
  struct 符号表管理器* r34;
  struct 类声明* r35;
  struct 符号* r39;
  struct 语义检查上下文* r48;
  struct 类声明* r49;
  struct 类声明* r57;
  struct 语义检查上下文* r60;
  struct 类声明* r61;
  struct AST节点 r25;
  struct AST节点 r50;
  enum 符号类型 r20;
  enum 诊断错误码 r27;
  enum 符号类型 r45;
  enum 诊断错误码 r52;

  entry:
  r0 = cn_var_类节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2401; else goto if_merge_2402;

  if_then_2401:
  return;
  goto if_merge_2402;

  if_merge_2402:
  r3 = cn_var_类节点;
  r4 = r3->基类名称;
  r5 = r4 != 0;
  if (r5) goto logic_rhs_2405; else goto logic_merge_2406;

  if_then_2403:
  void cn_var_父类符号_0;
  r10 = cn_var_上下文;
  r11 = r10->符号表;
  r12 = cn_var_类节点;
  r13 = r12->基类名称;
  r14 = 查找符号(r11, r13);
  cn_var_父类符号_0 = r14;
  r16 = cn_var_父类符号_0;
  r17 = r16 == 0;
  if (r17) goto logic_merge_2410; else goto logic_rhs_2409;

  if_merge_2404:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2411;

  logic_rhs_2405:
  r6 = cn_var_类节点;
  r7 = r6->基类名称;
  r8 = 获取字符串长度(r7);
  r9 = r8 > 0;
  goto logic_merge_2406;

  logic_merge_2406:
  if (r9) goto if_then_2403; else goto if_merge_2404;

  if_then_2407:
  r23 = cn_var_上下文;
  r24 = cn_var_类节点;
  r25 = r24->节点基类;
  r26 = r25.位置;
  r27 = cn_var_诊断错误码;
  r28 = r27.语义_未定义标识符;
  报告错误(r23, r26, r28);
  goto if_merge_2408;

  if_merge_2408:
  goto if_merge_2404;

  logic_rhs_2409:
  r18 = cn_var_父类符号_0;
  r19 = r18.种类;
  r20 = cn_var_符号类型;
  r21 = r20.类符号;
  r22 = r19 != r21;
  goto logic_merge_2410;

  logic_merge_2410:
  if (r22) goto if_then_2407; else goto if_merge_2408;

  for_cond_2411:
  r29 = cn_var_i_1;
  r30 = cn_var_类节点;
  r31 = r30->接口个数;
  r32 = r29 < r31;
  if (r32) goto for_body_2412; else goto for_exit_2414;

  for_body_2412:
  void cn_var_接口符号_2;
  r33 = cn_var_上下文;
  r34 = r33->符号表;
  r35 = cn_var_类节点;
  r36 = r35->实现接口;
  r37 = cn_var_i_1;
  r38 = *(void*)cn_rt_array_get_element(r36, r37, 8);
  r39 = 查找符号(r34, r38);
  cn_var_接口符号_2 = r39;
  r41 = cn_var_接口符号_2;
  r42 = r41 == 0;
  if (r42) goto logic_merge_2418; else goto logic_rhs_2417;

  for_update_2413:
  r54 = cn_var_i_1;
  r55 = r54 + 1;
  cn_var_i_1 = r55;
  goto for_cond_2411;

  for_exit_2414:
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_2419;

  if_then_2415:
  r48 = cn_var_上下文;
  r49 = cn_var_类节点;
  r50 = r49->节点基类;
  r51 = r50.位置;
  r52 = cn_var_诊断错误码;
  r53 = r52.语义_未定义标识符;
  报告错误(r48, r51, r53);
  goto if_merge_2416;

  if_merge_2416:
  goto for_update_2413;

  logic_rhs_2417:
  r43 = cn_var_接口符号_2;
  r44 = r43.种类;
  r45 = cn_var_符号类型;
  r46 = r45.接口符号;
  r47 = r44 != r46;
  goto logic_merge_2418;

  logic_merge_2418:
  if (r47) goto if_then_2415; else goto if_merge_2416;

  for_cond_2419:
  r56 = cn_var_i_3;
  r57 = cn_var_类节点;
  r58 = r57->成员个数;
  r59 = r56 < r58;
  if (r59) goto for_body_2420; else goto for_exit_2422;

  for_body_2420:
  r60 = cn_var_上下文;
  r61 = cn_var_类节点;
  r62 = r61->成员;
  r63 = cn_var_i_3;
  r64 = *(void*)cn_rt_array_get_element(r62, r63, 8);
  检查类成员(r60, r64);
  goto for_update_2421;

  for_update_2421:
  r65 = cn_var_i_3;
  r66 = r65 + 1;
  cn_var_i_3 = r66;
  goto for_cond_2419;

  for_exit_2422:
  return;
}

void 检查类成员(struct 语义检查上下文* cn_var_上下文, struct 类成员* cn_var_成员节点) {
  long long r1, r4, r5, r6, r12, r13, r14;
  struct 类成员* r0;
  struct 类成员* r2;
  struct 语义检查上下文* r7;
  struct 类成员* r8;
  struct 变量声明* r9;
  struct 类成员* r10;
  struct 语义检查上下文* r15;
  struct 类成员* r16;
  struct 函数声明* r17;
  struct 节点类型 r3;
  struct 节点类型 r11;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2423; else goto if_merge_2424;

  if_then_2423:
  return;
  goto if_merge_2424;

  if_merge_2424:
  r2 = cn_var_成员节点;
  r3 = r2->类型;
  r4 = cn_var_节点类型;
  r5 = r4.变量声明;
  r6 = r3 == r5;
  if (r6) goto if_then_2425; else goto if_else_2426;

  if_then_2425:
  r7 = cn_var_上下文;
  r8 = cn_var_成员节点;
  r9 = r8->字段;
  检查变量声明(r7, r9);
  goto if_merge_2427;

  if_else_2426:
  r10 = cn_var_成员节点;
  r11 = r10->类型;
  r12 = cn_var_节点类型;
  r13 = r12.函数声明;
  r14 = r11 == r13;
  if (r14) goto if_then_2428; else goto if_merge_2429;

  if_merge_2427:

  if_then_2428:
  r15 = cn_var_上下文;
  r16 = cn_var_成员节点;
  r17 = r16->方法;
  检查函数声明(r15, r17);
  goto if_merge_2429;

  if_merge_2429:
  goto if_merge_2427;
  return;
}

void 检查接口声明(struct 语义检查上下文* cn_var_上下文, struct 接口声明* cn_var_接口节点) {
  long long r1, r2, r4, r5, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r28, r29, r33, r35, r36, r37, r38, r39, r40, r41, r42, r46, r47, r51, r53, r54, r55;
  struct 接口声明* r0;
  struct 接口声明* r3;
  struct 接口声明* r6;
  struct 语义检查上下文* r25;
  struct 类型推断上下文* r26;
  struct 类型信息* r27;
  struct 语义检查上下文* r30;
  struct 接口声明* r31;
  struct 语义检查上下文* r43;
  struct 类型推断上下文* r44;
  struct 类型信息* r45;
  struct 语义检查上下文* r48;
  struct 接口声明* r49;
  struct AST节点 r32;
  struct AST节点 r50;
  enum 诊断错误码 r34;
  enum 诊断错误码 r52;

  entry:
  r0 = cn_var_接口节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2430; else goto if_merge_2431;

  if_then_2430:
  return;
  goto if_merge_2431;

  if_merge_2431:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2432;

  for_cond_2432:
  r2 = cn_var_i_0;
  r3 = cn_var_接口节点;
  r4 = r3->方法个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2433; else goto for_exit_2435;

  for_body_2433:
  void cn_var_方法_1;
  r6 = cn_var_接口节点;
  r7 = r6->方法列表;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  cn_var_方法_1 = r9;
  long long cn_var_j_2;
  cn_var_j_2 = 0;
  goto for_cond_2436;

  for_update_2434:
  r54 = cn_var_i_0;
  r55 = r54 + 1;
  cn_var_i_0 = r55;
  goto for_cond_2432;

  for_exit_2435:

  for_cond_2436:
  r10 = cn_var_j_2;
  r11 = cn_var_方法_1;
  r12 = r11.参数个数;
  r13 = r10 < r12;
  if (r13) goto for_body_2437; else goto for_exit_2439;

  for_body_2437:
  r14 = cn_var_方法_1;
  r15 = r14.参数列表;
  r16 = cn_var_j_2;
  r17 = *(void*)cn_rt_array_get_element(r15, r16, 8);
  r18 = r17.类型;
  r19 = r18 != 0;
  if (r19) goto if_then_2440; else goto if_merge_2441;

  for_update_2438:
  r36 = cn_var_j_2;
  r37 = r36 + 1;
  cn_var_j_2 = r37;
  goto for_cond_2436;

  for_exit_2439:
  r38 = cn_var_方法_1;
  r39 = r38.返回类型;
  r40 = r39 != 0;
  if (r40) goto if_then_2444; else goto if_merge_2445;

  if_then_2440:
  void cn_var_类型_3;
  r20 = cn_var_方法_1;
  r21 = r20.参数列表;
  r22 = cn_var_j_2;
  r23 = *(void*)cn_rt_array_get_element(r21, r22, 8);
  r24 = r23.类型;
  r25 = cn_var_上下文;
  r26 = r25->类型上下文;
  r27 = 从类型节点推断(r24, r26);
  cn_var_类型_3 = r27;
  r28 = cn_var_类型_3;
  r29 = r28 == 0;
  if (r29) goto if_then_2442; else goto if_merge_2443;

  if_merge_2441:
  goto for_update_2438;

  if_then_2442:
  r30 = cn_var_上下文;
  r31 = cn_var_接口节点;
  r32 = r31->节点基类;
  r33 = r32.位置;
  r34 = cn_var_诊断错误码;
  r35 = r34.语义_未定义标识符;
  报告错误(r30, r33, r35);
  goto if_merge_2443;

  if_merge_2443:
  goto if_merge_2441;

  if_then_2444:
  void cn_var_类型_4;
  r41 = cn_var_方法_1;
  r42 = r41.返回类型;
  r43 = cn_var_上下文;
  r44 = r43->类型上下文;
  r45 = 从类型节点推断(r42, r44);
  cn_var_类型_4 = r45;
  r46 = cn_var_类型_4;
  r47 = r46 == 0;
  if (r47) goto if_then_2446; else goto if_merge_2447;

  if_merge_2445:
  goto for_update_2434;

  if_then_2446:
  r48 = cn_var_上下文;
  r49 = cn_var_接口节点;
  r50 = r49->节点基类;
  r51 = r50.位置;
  r52 = cn_var_诊断错误码;
  r53 = r52.语义_未定义标识符;
  报告错误(r48, r51, r53);
  goto if_merge_2447;

  if_merge_2447:
  goto if_merge_2445;
  return;
}

void 检查语句(struct 语义检查上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r3, r4, r5, r6, r8, r9, r11, r12, r14, r15, r17, r18, r20, r21, r23, r24, r26, r27, r29, r30, r32, r33, r35, r36;
  struct 语句节点* r0;
  struct 语句节点* r2;
  struct 语义检查上下文* r38;
  struct 语句节点* r39;
  struct 语义检查上下文* r40;
  struct 语句节点* r41;
  struct 语义检查上下文* r42;
  struct 语句节点* r43;
  struct 语义检查上下文* r44;
  struct 语句节点* r45;
  struct 语义检查上下文* r46;
  struct 语句节点* r47;
  struct 语义检查上下文* r48;
  struct 语句节点* r49;
  struct 语义检查上下文* r50;
  struct 语句节点* r51;
  struct 语义检查上下文* r52;
  struct 语句节点* r53;
  struct 语义检查上下文* r54;
  struct 语句节点* r55;
  struct 表达式节点* r56;
  struct 类型信息* r57;
  struct 语义检查上下文* r58;
  struct 语句节点* r59;
  struct 语义检查上下文* r60;
  struct 语句节点* r61;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;
  _Bool r34;
  _Bool r37;

  entry:
  r0 = cn_var_语句;
  r1 = r0 == 0;
  if (r1) goto if_then_2448; else goto if_merge_2449;

  if_then_2448:
  return;
  goto if_merge_2449;

  if_merge_2449:
  r2 = cn_var_语句;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = cn_var_节点类型;
  r6 = r5.块语句;
  r7 = r4 == r6;
  if (r7) goto case_body_2451; else goto switch_check_2463;

  switch_check_2463:
  r8 = cn_var_节点类型;
  r9 = r8.如果语句;
  r10 = r4 == r9;
  if (r10) goto case_body_2452; else goto switch_check_2464;

  switch_check_2464:
  r11 = cn_var_节点类型;
  r12 = r11.当语句;
  r13 = r4 == r12;
  if (r13) goto case_body_2453; else goto switch_check_2465;

  switch_check_2465:
  r14 = cn_var_节点类型;
  r15 = r14.循环语句;
  r16 = r4 == r15;
  if (r16) goto case_body_2454; else goto switch_check_2466;

  switch_check_2466:
  r17 = cn_var_节点类型;
  r18 = r17.返回语句;
  r19 = r4 == r18;
  if (r19) goto case_body_2455; else goto switch_check_2467;

  switch_check_2467:
  r20 = cn_var_节点类型;
  r21 = r20.中断语句;
  r22 = r4 == r21;
  if (r22) goto case_body_2456; else goto switch_check_2468;

  switch_check_2468:
  r23 = cn_var_节点类型;
  r24 = r23.继续语句;
  r25 = r4 == r24;
  if (r25) goto case_body_2457; else goto switch_check_2469;

  switch_check_2469:
  r26 = cn_var_节点类型;
  r27 = r26.选择语句;
  r28 = r4 == r27;
  if (r28) goto case_body_2458; else goto switch_check_2470;

  switch_check_2470:
  r29 = cn_var_节点类型;
  r30 = r29.表达式语句;
  r31 = r4 == r30;
  if (r31) goto case_body_2459; else goto switch_check_2471;

  switch_check_2471:
  r32 = cn_var_节点类型;
  r33 = r32.尝试语句;
  r34 = r4 == r33;
  if (r34) goto case_body_2460; else goto switch_check_2472;

  switch_check_2472:
  r35 = cn_var_节点类型;
  r36 = r35.抛出语句;
  r37 = r4 == r36;
  if (r37) goto case_body_2461; else goto case_default_2462;

  case_body_2451:
  r38 = cn_var_上下文;
  r39 = cn_var_语句;
  检查块语句(r38, (struct 块语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2452:
  r40 = cn_var_上下文;
  r41 = cn_var_语句;
  检查如果语句(r40, (struct 如果语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2453:
  r42 = cn_var_上下文;
  r43 = cn_var_语句;
  检查当语句(r42, (struct 当语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2454:
  r44 = cn_var_上下文;
  r45 = cn_var_语句;
  检查循环语句(r44, (struct 循环语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2455:
  r46 = cn_var_上下文;
  r47 = cn_var_语句;
  检查返回语句(r46, (struct 返回语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2456:
  r48 = cn_var_上下文;
  r49 = cn_var_语句;
  检查中断语句(r48, (struct 中断语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2457:
  r50 = cn_var_上下文;
  r51 = cn_var_语句;
  检查继续语句(r50, (struct 继续语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2458:
  r52 = cn_var_上下文;
  r53 = cn_var_语句;
  检查选择语句(r52, (struct 选择语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2459:
  r54 = cn_var_上下文;
  r55 = cn_var_语句;
  r56 = (struct 表达式语句*)cn_var_语句->表达式;
  r57 = 检查表达式(r54, r56);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2460:
  r58 = cn_var_上下文;
  r59 = cn_var_语句;
  检查尝试语句(r58, (struct 尝试语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_body_2461:
  r60 = cn_var_上下文;
  r61 = cn_var_语句;
  检查抛出语句(r60, (struct 抛出语句*)cn_var_语句);
  goto switch_merge_2450;
  goto switch_merge_2450;

  case_default_2462:
  goto switch_merge_2450;
  goto switch_merge_2450;

  switch_merge_2450:
  return;
}

void 检查块语句(struct 语义检查上下文* cn_var_上下文, struct 块语句* cn_var_块节点) {
  long long r1, r2, r4, r5, r8, r9, r10, r11, r12;
  struct 块语句* r0;
  struct 块语句* r3;
  struct 语义检查上下文* r6;
  struct 块语句* r7;

  entry:
  r0 = cn_var_块节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2473; else goto if_merge_2474;

  if_then_2473:
  return;
  goto if_merge_2474;

  if_merge_2474:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2475;

  for_cond_2475:
  r2 = cn_var_i_0;
  r3 = cn_var_块节点;
  r4 = r3->语句个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2476; else goto for_exit_2478;

  for_body_2476:
  r6 = cn_var_上下文;
  r7 = cn_var_块节点;
  r8 = r7->语句列表;
  r9 = cn_var_i_0;
  r10 = *(void*)cn_rt_array_get_element(r8, r9, 8);
  检查语句(r6, r10);
  goto for_update_2477;

  for_update_2477:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2475;

  for_exit_2478:
  return;
}

void 检查如果语句(struct 语义检查上下文* cn_var_上下文, struct 如果语句* cn_var_如果节点) {
  long long r1, r6, r7, r8, r9, r10, r12, r13, r17, r18, r20, r23, r29;
  struct 如果语句* r0;
  struct 语义检查上下文* r2;
  struct 如果语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r14;
  struct 如果语句* r15;
  struct 表达式节点* r16;
  struct 如果语句* r21;
  struct 块语句* r22;
  struct 语义检查上下文* r24;
  struct 如果语句* r25;
  struct 块语句* r26;
  struct 如果语句* r27;
  struct 语句节点* r28;
  struct 语义检查上下文* r30;
  struct 如果语句* r31;
  struct 语句节点* r32;
  enum 类型种类 r11;
  enum 诊断错误码 r19;

  entry:
  r0 = cn_var_如果节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2479; else goto if_merge_2480;

  if_then_2479:
  return;
  goto if_merge_2480;

  if_merge_2480:
  void cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_如果节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  r7 = cn_var_条件类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2483; else goto logic_merge_2484;

  if_then_2481:
  r14 = cn_var_上下文;
  r15 = cn_var_如果节点;
  r16 = r15->条件;
  r17 = r16->节点基类;
  r18 = r17.位置;
  r19 = cn_var_诊断错误码;
  r20 = r19.语义_类型不匹配;
  报告错误(r14, r18, r20);
  goto if_merge_2482;

  if_merge_2482:
  r21 = cn_var_如果节点;
  r22 = r21->真分支;
  r23 = r22 != 0;
  if (r23) goto if_then_2485; else goto if_merge_2486;

  logic_rhs_2483:
  r9 = cn_var_条件类型_0;
  r10 = r9.种类;
  r11 = cn_var_类型种类;
  r12 = r11.布尔类型;
  r13 = r10 != r12;
  goto logic_merge_2484;

  logic_merge_2484:
  if (r13) goto if_then_2481; else goto if_merge_2482;

  if_then_2485:
  r24 = cn_var_上下文;
  r25 = cn_var_如果节点;
  r26 = r25->真分支;
  检查语句(r24, r26);
  goto if_merge_2486;

  if_merge_2486:
  r27 = cn_var_如果节点;
  r28 = r27->假分支;
  r29 = r28 != 0;
  if (r29) goto if_then_2487; else goto if_merge_2488;

  if_then_2487:
  r30 = cn_var_上下文;
  r31 = cn_var_如果节点;
  r32 = r31->假分支;
  检查语句(r30, r32);
  goto if_merge_2488;

  if_merge_2488:
  return;
}

void 检查当语句(struct 语义检查上下文* cn_var_上下文, struct 当语句* cn_var_当节点) {
  long long r1, r6, r7, r8, r9, r10, r12, r13, r17, r18, r20, r22, r23, r26, r31, r32;
  struct 当语句* r0;
  struct 语义检查上下文* r2;
  struct 当语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r14;
  struct 当语句* r15;
  struct 表达式节点* r16;
  struct 语义检查上下文* r21;
  struct 当语句* r24;
  struct 块语句* r25;
  struct 语义检查上下文* r27;
  struct 当语句* r28;
  struct 块语句* r29;
  struct 语义检查上下文* r30;
  enum 类型种类 r11;
  enum 诊断错误码 r19;

  entry:
  r0 = cn_var_当节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2489; else goto if_merge_2490;

  if_then_2489:
  return;
  goto if_merge_2490;

  if_merge_2490:
  void cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_当节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  r7 = cn_var_条件类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2493; else goto logic_merge_2494;

  if_then_2491:
  r14 = cn_var_上下文;
  r15 = cn_var_当节点;
  r16 = r15->条件;
  r17 = r16->节点基类;
  r18 = r17.位置;
  r19 = cn_var_诊断错误码;
  r20 = r19.语义_类型不匹配;
  报告错误(r14, r18, r20);
  goto if_merge_2492;

  if_merge_2492:
  r21 = cn_var_上下文;
  r22 = r21->循环深度;
  r23 = r22 + 1;
  r24 = cn_var_当节点;
  r25 = r24->循环体;
  r26 = r25 != 0;
  if (r26) goto if_then_2495; else goto if_merge_2496;

  logic_rhs_2493:
  r9 = cn_var_条件类型_0;
  r10 = r9.种类;
  r11 = cn_var_类型种类;
  r12 = r11.布尔类型;
  r13 = r10 != r12;
  goto logic_merge_2494;

  logic_merge_2494:
  if (r13) goto if_then_2491; else goto if_merge_2492;

  if_then_2495:
  r27 = cn_var_上下文;
  r28 = cn_var_当节点;
  r29 = r28->循环体;
  检查语句(r27, r29);
  goto if_merge_2496;

  if_merge_2496:
  r30 = cn_var_上下文;
  r31 = r30->循环深度;
  r32 = r31 - 1;
  return;
}

void 检查循环语句(struct 语义检查上下文* cn_var_上下文, struct 循环语句* cn_var_循环节点) {
  long long r1, r4, r10, r15, r16, r17, r18, r19, r21, r22, r26, r27, r29, r32, r38, r39, r42, r47, r48;
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
  struct 语义检查上下文* r23;
  struct 循环语句* r24;
  struct 表达式节点* r25;
  struct 循环语句* r30;
  struct 表达式节点* r31;
  struct 语义检查上下文* r33;
  struct 循环语句* r34;
  struct 表达式节点* r35;
  struct 类型信息* r36;
  struct 语义检查上下文* r37;
  struct 循环语句* r40;
  struct 块语句* r41;
  struct 语义检查上下文* r43;
  struct 循环语句* r44;
  struct 块语句* r45;
  struct 语义检查上下文* r46;
  enum 类型种类 r20;
  enum 诊断错误码 r28;

  entry:
  r0 = cn_var_循环节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2497; else goto if_merge_2498;

  if_then_2497:
  return;
  goto if_merge_2498;

  if_merge_2498:
  r2 = cn_var_循环节点;
  r3 = r2->初始化;
  r4 = r3 != 0;
  if (r4) goto if_then_2499; else goto if_merge_2500;

  if_then_2499:
  r5 = cn_var_上下文;
  r6 = cn_var_循环节点;
  r7 = r6->初始化;
  检查语句(r5, r7);
  goto if_merge_2500;

  if_merge_2500:
  r8 = cn_var_循环节点;
  r9 = r8->条件;
  r10 = r9 != 0;
  if (r10) goto if_then_2501; else goto if_merge_2502;

  if_then_2501:
  void cn_var_条件类型_0;
  r11 = cn_var_上下文;
  r12 = cn_var_循环节点;
  r13 = r12->条件;
  r14 = 检查表达式(r11, r13);
  cn_var_条件类型_0 = r14;
  r16 = cn_var_条件类型_0;
  r17 = r16 != 0;
  if (r17) goto logic_rhs_2505; else goto logic_merge_2506;

  if_merge_2502:
  r30 = cn_var_循环节点;
  r31 = r30->更新;
  r32 = r31 != 0;
  if (r32) goto if_then_2507; else goto if_merge_2508;

  if_then_2503:
  r23 = cn_var_上下文;
  r24 = cn_var_循环节点;
  r25 = r24->条件;
  r26 = r25->节点基类;
  r27 = r26.位置;
  r28 = cn_var_诊断错误码;
  r29 = r28.语义_类型不匹配;
  报告错误(r23, r27, r29);
  goto if_merge_2504;

  if_merge_2504:
  goto if_merge_2502;

  logic_rhs_2505:
  r18 = cn_var_条件类型_0;
  r19 = r18.种类;
  r20 = cn_var_类型种类;
  r21 = r20.布尔类型;
  r22 = r19 != r21;
  goto logic_merge_2506;

  logic_merge_2506:
  if (r22) goto if_then_2503; else goto if_merge_2504;

  if_then_2507:
  r33 = cn_var_上下文;
  r34 = cn_var_循环节点;
  r35 = r34->更新;
  r36 = 检查表达式(r33, r35);
  goto if_merge_2508;

  if_merge_2508:
  r37 = cn_var_上下文;
  r38 = r37->循环深度;
  r39 = r38 + 1;
  r40 = cn_var_循环节点;
  r41 = r40->循环体;
  r42 = r41 != 0;
  if (r42) goto if_then_2509; else goto if_merge_2510;

  if_then_2509:
  r43 = cn_var_上下文;
  r44 = cn_var_循环节点;
  r45 = r44->循环体;
  检查语句(r43, r45);
  goto if_merge_2510;

  if_merge_2510:
  r46 = cn_var_上下文;
  r47 = r46->循环深度;
  r48 = r47 - 1;
  return;
}

void 检查返回语句(struct 语义检查上下文* cn_var_上下文, struct 返回语句* cn_var_返回节点) {
  long long r1, r4, r8, r10, r13, r18, r21, r22, r23, r24, r28, r30, r31, r35, r37, r38, r41, r46, r47, r51, r53;
  struct 返回语句* r0;
  struct 语义检查上下文* r2;
  struct 符号* r3;
  struct 语义检查上下文* r5;
  struct 返回语句* r6;
  struct 返回语句* r11;
  struct 表达式节点* r12;
  struct 语义检查上下文* r14;
  struct 返回语句* r15;
  struct 表达式节点* r16;
  struct 类型信息* r17;
  struct 语义检查上下文* r19;
  struct 类型信息* r20;
  struct 语义检查上下文* r25;
  struct 类型信息* r26;
  struct 语义检查上下文* r32;
  struct 返回语句* r33;
  struct 语义检查上下文* r39;
  struct 类型信息* r40;
  struct 语义检查上下文* r42;
  struct 类型信息* r43;
  struct 语义检查上下文* r48;
  struct 返回语句* r49;
  struct AST节点 r7;
  enum 类型兼容性 r27;
  struct AST节点 r34;
  struct AST节点 r50;
  enum 诊断错误码 r9;
  enum 类型兼容性 r29;
  enum 诊断错误码 r36;
  enum 类型种类 r44;
  enum 类型种类 r45;
  enum 诊断错误码 r52;

  entry:
  r0 = cn_var_返回节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2511; else goto if_merge_2512;

  if_then_2511:
  return;
  goto if_merge_2512;

  if_merge_2512:
  r2 = cn_var_上下文;
  r3 = r2->当前函数;
  r4 = r3 == 0;
  if (r4) goto if_then_2513; else goto if_merge_2514;

  if_then_2513:
  r5 = cn_var_上下文;
  r6 = cn_var_返回节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  r9 = cn_var_诊断错误码;
  r10 = r9.语义_返回语句在函数外;
  报告错误(r5, r8, r10);
  return;
  goto if_merge_2514;

  if_merge_2514:
  r11 = cn_var_返回节点;
  r12 = r11->返回值;
  r13 = r12 != 0;
  if (r13) goto if_then_2515; else goto if_else_2516;

  if_then_2515:
  void cn_var_返回值类型_0;
  r14 = cn_var_上下文;
  r15 = cn_var_返回节点;
  r16 = r15->返回值;
  r17 = 检查表达式(r14, r16);
  cn_var_返回值类型_0 = r17;
  r19 = cn_var_上下文;
  r20 = r19->期望返回类型;
  r21 = r20 != 0;
  if (r21) goto logic_rhs_2520; else goto logic_merge_2521;

  if_else_2516:
  r39 = cn_var_上下文;
  r40 = r39->期望返回类型;
  r41 = r40 != 0;
  if (r41) goto logic_rhs_2526; else goto logic_merge_2527;

  if_merge_2517:

  if_then_2518:
  void cn_var_兼容性_1;
  r24 = cn_var_返回值类型_0;
  r25 = cn_var_上下文;
  r26 = r25->期望返回类型;
  r27 = 检查类型兼容性(r24, r26);
  cn_var_兼容性_1 = r27;
  r28 = cn_var_兼容性_1;
  r29 = cn_var_类型兼容性;
  r30 = r29.不兼容;
  r31 = r28 == r30;
  if (r31) goto if_then_2522; else goto if_merge_2523;

  if_merge_2519:
  goto if_merge_2517;

  logic_rhs_2520:
  r22 = cn_var_返回值类型_0;
  r23 = r22 != 0;
  goto logic_merge_2521;

  logic_merge_2521:
  if (r23) goto if_then_2518; else goto if_merge_2519;

  if_then_2522:
  r32 = cn_var_上下文;
  r33 = cn_var_返回节点;
  r34 = r33->节点基类;
  r35 = r34.位置;
  r36 = cn_var_诊断错误码;
  r37 = r36.语义_类型不匹配;
  报告错误(r32, r35, r37);
  goto if_merge_2523;

  if_merge_2523:
  goto if_merge_2519;

  if_then_2524:
  r48 = cn_var_上下文;
  r49 = cn_var_返回节点;
  r50 = r49->节点基类;
  r51 = r50.位置;
  r52 = cn_var_诊断错误码;
  r53 = r52.语义_类型不匹配;
  报告错误(r48, r51, r53);
  goto if_merge_2525;

  if_merge_2525:
  goto if_merge_2517;

  logic_rhs_2526:
  r42 = cn_var_上下文;
  r43 = r42->期望返回类型;
  r44 = r43->种类;
  r45 = cn_var_类型种类;
  r46 = r45.类型_空;
  r47 = r44 != r46;
  goto logic_merge_2527;

  logic_merge_2527:
  if (r47) goto if_then_2524; else goto if_merge_2525;
  return;
}

void 检查中断语句(struct 语义检查上下文* cn_var_上下文, struct 中断语句* cn_var_中断节点) {
  long long r1, r3, r4, r8, r10;
  struct 中断语句* r0;
  struct 语义检查上下文* r2;
  struct 语义检查上下文* r5;
  struct 中断语句* r6;
  struct AST节点 r7;
  enum 诊断错误码 r9;

  entry:
  r0 = cn_var_中断节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2528; else goto if_merge_2529;

  if_then_2528:
  return;
  goto if_merge_2529;

  if_merge_2529:
  r2 = cn_var_上下文;
  r3 = r2->循环深度;
  r4 = r3 == 0;
  if (r4) goto if_then_2530; else goto if_merge_2531;

  if_then_2530:
  r5 = cn_var_上下文;
  r6 = cn_var_中断节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  r9 = cn_var_诊断错误码;
  r10 = r9.语义_中断继续在循环外;
  报告错误(r5, r8, r10);
  goto if_merge_2531;

  if_merge_2531:
  return;
}

void 检查继续语句(struct 语义检查上下文* cn_var_上下文, struct 继续语句* cn_var_继续节点) {
  long long r1, r3, r4, r8, r10;
  struct 继续语句* r0;
  struct 语义检查上下文* r2;
  struct 语义检查上下文* r5;
  struct 继续语句* r6;
  struct AST节点 r7;
  enum 诊断错误码 r9;

  entry:
  r0 = cn_var_继续节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2532; else goto if_merge_2533;

  if_then_2532:
  return;
  goto if_merge_2533;

  if_merge_2533:
  r2 = cn_var_上下文;
  r3 = r2->循环深度;
  r4 = r3 == 0;
  if (r4) goto if_then_2534; else goto if_merge_2535;

  if_then_2534:
  r5 = cn_var_上下文;
  r6 = cn_var_继续节点;
  r7 = r6->节点基类;
  r8 = r7.位置;
  r9 = cn_var_诊断错误码;
  r10 = r9.语义_中断继续在循环外;
  报告错误(r5, r8, r10);
  goto if_merge_2535;

  if_merge_2535:
  return;
}

void 检查选择语句(struct 语义检查上下文* cn_var_上下文, struct 选择语句* cn_var_选择节点) {
  long long r1, r6, r7, r8, r9, r11, r15, r16, r18, r19, r21, r22, r25, r26, r27, r28, r29, r32;
  struct 选择语句* r0;
  struct 语义检查上下文* r2;
  struct 选择语句* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 选择语句* r13;
  struct 表达式节点* r14;
  struct 选择语句* r20;
  struct 语义检查上下文* r23;
  struct 选择语句* r24;
  struct 选择语句* r30;
  struct 块语句* r31;
  struct 语义检查上下文* r33;
  struct 选择语句* r34;
  struct 块语句* r35;
  _Bool r10;
  enum 诊断错误码 r17;

  entry:
  r0 = cn_var_选择节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2536; else goto if_merge_2537;

  if_then_2536:
  return;
  goto if_merge_2537;

  if_merge_2537:
  void cn_var_选择类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_选择节点;
  r4 = r3->选择值;
  r5 = 检查表达式(r2, r4);
  cn_var_选择类型_0 = r5;
  r7 = cn_var_选择类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2540; else goto logic_merge_2541;

  if_then_2538:
  r12 = cn_var_上下文;
  r13 = cn_var_选择节点;
  r14 = r13->选择值;
  r15 = r14->节点基类;
  r16 = r15.位置;
  r17 = cn_var_诊断错误码;
  r18 = r17.语义_类型不匹配;
  报告错误(r12, r16, r18);
  goto if_merge_2539;

  if_merge_2539:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2542;

  logic_rhs_2540:
  r9 = cn_var_选择类型_0;
  r10 = 是整数类型(r9);
  r11 = r10 == 0;
  goto logic_merge_2541;

  logic_merge_2541:
  if (r11) goto if_then_2538; else goto if_merge_2539;

  for_cond_2542:
  r19 = cn_var_i_1;
  r20 = cn_var_选择节点;
  r21 = r20->情况个数;
  r22 = r19 < r21;
  if (r22) goto for_body_2543; else goto for_exit_2545;

  for_body_2543:
  r23 = cn_var_上下文;
  r24 = cn_var_选择节点;
  r25 = r24->情况列表;
  r26 = cn_var_i_1;
  r27 = *(void*)cn_rt_array_get_element(r25, r26, 8);
  检查情况语句(r23, r27);
  goto for_update_2544;

  for_update_2544:
  r28 = cn_var_i_1;
  r29 = r28 + 1;
  cn_var_i_1 = r29;
  goto for_cond_2542;

  for_exit_2545:
  r30 = cn_var_选择节点;
  r31 = r30->默认分支;
  r32 = r31 != 0;
  if (r32) goto if_then_2546; else goto if_merge_2547;

  if_then_2546:
  r33 = cn_var_上下文;
  r34 = cn_var_选择节点;
  r35 = r34->默认分支;
  检查语句(r33, r35);
  goto if_merge_2547;

  if_merge_2547:
  return;
}

void 检查情况语句(struct 语义检查上下文* cn_var_上下文, struct 情况语句* cn_var_情况节点) {
  long long r1, r3, r4, r6, r8, r11, r12, r13, r15, r16, r18, r19, r22, r23, r24, r25, r26;
  struct 情况语句* r0;
  struct 情况语句* r2;
  struct 情况语句* r5;
  struct 语义检查上下文* r9;
  struct 情况语句* r10;
  struct 情况语句* r17;
  struct 语义检查上下文* r20;
  struct 情况语句* r21;
  _Bool r7;
  enum 诊断错误码 r14;

  entry:
  r0 = cn_var_情况节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2548; else goto if_merge_2549;

  if_then_2548:
  return;
  goto if_merge_2549;

  if_merge_2549:
  r2 = cn_var_情况节点;
  r3 = r2->值;
  r4 = r3 != 0;
  if (r4) goto if_then_2550; else goto if_merge_2551;

  if_then_2550:
  r5 = cn_var_情况节点;
  r6 = r5->值;
  r7 = 是常量表达式(r6);
  r8 = r7 == 0;
  if (r8) goto if_then_2552; else goto if_merge_2553;

  if_merge_2551:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2554;

  if_then_2552:
  r9 = cn_var_上下文;
  r10 = cn_var_情况节点;
  r11 = r10->值;
  r12 = r11.节点基类;
  r13 = r12.位置;
  r14 = cn_var_诊断错误码;
  r15 = r14.语义_开关非常量情况;
  报告错误(r9, r13, r15);
  goto if_merge_2553;

  if_merge_2553:
  goto if_merge_2551;

  for_cond_2554:
  r16 = cn_var_i_0;
  r17 = cn_var_情况节点;
  r18 = r17->语句个数;
  r19 = r16 < r18;
  if (r19) goto for_body_2555; else goto for_exit_2557;

  for_body_2555:
  r20 = cn_var_上下文;
  r21 = cn_var_情况节点;
  r22 = r21->语句列表;
  r23 = cn_var_i_0;
  r24 = *(void*)cn_rt_array_get_element(r22, r23, 8);
  检查语句(r20, r24);
  goto for_update_2556;

  for_update_2556:
  r25 = cn_var_i_0;
  r26 = r25 + 1;
  cn_var_i_0 = r26;
  goto for_cond_2554;

  for_exit_2557:
  return;
}

void 检查尝试语句(struct 语义检查上下文* cn_var_上下文, struct 尝试语句* cn_var_尝试节点) {
  long long r1, r4, r8, r10, r11, r14, r15, r16, r17, r18, r21;
  struct 尝试语句* r0;
  struct 尝试语句* r2;
  struct 块语句* r3;
  struct 语义检查上下文* r5;
  struct 尝试语句* r6;
  struct 块语句* r7;
  struct 尝试语句* r9;
  struct 语义检查上下文* r12;
  struct 尝试语句* r13;
  struct 尝试语句* r19;
  struct 块语句* r20;
  struct 语义检查上下文* r22;
  struct 尝试语句* r23;
  struct 块语句* r24;

  entry:
  r0 = cn_var_尝试节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2558; else goto if_merge_2559;

  if_then_2558:
  return;
  goto if_merge_2559;

  if_merge_2559:
  r2 = cn_var_尝试节点;
  r3 = r2->尝试块;
  r4 = r3 != 0;
  if (r4) goto if_then_2560; else goto if_merge_2561;

  if_then_2560:
  r5 = cn_var_上下文;
  r6 = cn_var_尝试节点;
  r7 = r6->尝试块;
  检查语句(r5, r7);
  goto if_merge_2561;

  if_merge_2561:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2562;

  for_cond_2562:
  r8 = cn_var_i_0;
  r9 = cn_var_尝试节点;
  r10 = r9->捕获个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2563; else goto for_exit_2565;

  for_body_2563:
  r12 = cn_var_上下文;
  r13 = cn_var_尝试节点;
  r14 = r13->捕获列表;
  r15 = cn_var_i_0;
  r16 = *(void*)cn_rt_array_get_element(r14, r15, 8);
  检查捕获语句(r12, r16);
  goto for_update_2564;

  for_update_2564:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_2562;

  for_exit_2565:
  r19 = cn_var_尝试节点;
  r20 = r19->最终块;
  r21 = r20 != 0;
  if (r21) goto if_then_2566; else goto if_merge_2567;

  if_then_2566:
  r22 = cn_var_上下文;
  r23 = cn_var_尝试节点;
  r24 = r23->最终块;
  检查语句(r22, r24);
  goto if_merge_2567;

  if_merge_2567:
  return;
}

void 检查捕获语句(struct 语义检查上下文* cn_var_上下文, struct 捕获语句* cn_var_捕获节点) {
  long long r1, r3, r4, r7, r9, r10, r13;
  struct 捕获语句* r0;
  struct 捕获语句* r2;
  struct 语义检查上下文* r5;
  struct 捕获语句* r6;
  struct 捕获语句* r8;
  struct 语义检查上下文* r11;
  struct 捕获语句* r12;

  entry:
  r0 = cn_var_捕获节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2568; else goto if_merge_2569;

  if_then_2568:
  return;
  goto if_merge_2569;

  if_merge_2569:
  r2 = cn_var_捕获节点;
  r3 = r2->异常变量;
  r4 = r3 != 0;
  if (r4) goto if_then_2570; else goto if_merge_2571;

  if_then_2570:
  r5 = cn_var_上下文;
  r6 = cn_var_捕获节点;
  r7 = r6->异常变量;
  检查变量声明(r5, r7);
  goto if_merge_2571;

  if_merge_2571:
  r8 = cn_var_捕获节点;
  r9 = r8->捕获体;
  r10 = r9 != 0;
  if (r10) goto if_then_2572; else goto if_merge_2573;

  if_then_2572:
  r11 = cn_var_上下文;
  r12 = cn_var_捕获节点;
  r13 = r12->捕获体;
  检查语句(r11, r13);
  goto if_merge_2573;

  if_merge_2573:
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
  if (r1) goto if_then_2574; else goto if_merge_2575;

  if_then_2574:
  return;
  goto if_merge_2575;

  if_merge_2575:
  r2 = cn_var_抛出节点;
  r3 = r2->抛出值;
  r4 = r3 != 0;
  if (r4) goto if_then_2576; else goto if_merge_2577;

  if_then_2576:
  r5 = cn_var_上下文;
  r6 = cn_var_抛出节点;
  r7 = r6->抛出值;
  r8 = 检查表达式(r5, r7);
  goto if_merge_2577;

  if_merge_2577:
  return;
}

struct 类型信息* 检查表达式(struct 语义检查上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r3, r4, r5, r6, r8, r9, r11, r12, r14, r15, r17, r18, r20, r21, r23, r24, r26, r27, r29, r30;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 语义检查上下文* r32;
  struct 表达式节点* r33;
  struct 类型信息* r34;
  struct 语义检查上下文* r35;
  struct 表达式节点* r36;
  struct 类型信息* r37;
  struct 语义检查上下文* r38;
  struct 表达式节点* r39;
  struct 类型信息* r40;
  struct 语义检查上下文* r41;
  struct 表达式节点* r42;
  struct 类型信息* r43;
  struct 语义检查上下文* r44;
  struct 表达式节点* r45;
  struct 类型信息* r46;
  struct 语义检查上下文* r47;
  struct 表达式节点* r48;
  struct 类型信息* r49;
  struct 语义检查上下文* r50;
  struct 表达式节点* r51;
  struct 类型信息* r52;
  struct 语义检查上下文* r53;
  struct 表达式节点* r54;
  struct 类型信息* r55;
  struct 语义检查上下文* r56;
  struct 表达式节点* r57;
  struct 类型信息* r58;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;

  entry:
  r0 = cn_var_表达式;
  r1 = r0 == 0;
  if (r1) goto if_then_2578; else goto if_merge_2579;

  if_then_2578:
  return 0;
  goto if_merge_2579;

  if_merge_2579:
  r2 = cn_var_表达式;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = cn_var_节点类型;
  r6 = r5.二元表达式;
  r7 = r4 == r6;
  if (r7) goto case_body_2581; else goto switch_check_2591;

  switch_check_2591:
  r8 = cn_var_节点类型;
  r9 = r8.一元表达式;
  r10 = r4 == r9;
  if (r10) goto case_body_2582; else goto switch_check_2592;

  switch_check_2592:
  r11 = cn_var_节点类型;
  r12 = r11.字面量表达式;
  r13 = r4 == r12;
  if (r13) goto case_body_2583; else goto switch_check_2593;

  switch_check_2593:
  r14 = cn_var_节点类型;
  r15 = r14.标识符表达式;
  r16 = r4 == r15;
  if (r16) goto case_body_2584; else goto switch_check_2594;

  switch_check_2594:
  r17 = cn_var_节点类型;
  r18 = r17.函数调用表达式;
  r19 = r4 == r18;
  if (r19) goto case_body_2585; else goto switch_check_2595;

  switch_check_2595:
  r20 = cn_var_节点类型;
  r21 = r20.成员访问表达式;
  r22 = r4 == r21;
  if (r22) goto case_body_2586; else goto switch_check_2596;

  switch_check_2596:
  r23 = cn_var_节点类型;
  r24 = r23.数组访问表达式;
  r25 = r4 == r24;
  if (r25) goto case_body_2587; else goto switch_check_2597;

  switch_check_2597:
  r26 = cn_var_节点类型;
  r27 = r26.赋值表达式;
  r28 = r4 == r27;
  if (r28) goto case_body_2588; else goto switch_check_2598;

  switch_check_2598:
  r29 = cn_var_节点类型;
  r30 = r29.三元表达式;
  r31 = r4 == r30;
  if (r31) goto case_body_2589; else goto case_default_2590;

  case_body_2581:
  r32 = cn_var_上下文;
  r33 = cn_var_表达式;
  r34 = 检查二元表达式(r32, (struct 二元表达式*)cn_var_表达式);
  return r34;
  goto switch_merge_2580;

  case_body_2582:
  r35 = cn_var_上下文;
  r36 = cn_var_表达式;
  r37 = 检查一元表达式(r35, (struct 一元表达式*)cn_var_表达式);
  return r37;
  goto switch_merge_2580;

  case_body_2583:
  r38 = cn_var_上下文;
  r39 = cn_var_表达式;
  r40 = 检查字面量表达式(r38, (struct 字面量表达式*)cn_var_表达式);
  return r40;
  goto switch_merge_2580;

  case_body_2584:
  r41 = cn_var_上下文;
  r42 = cn_var_表达式;
  r43 = 检查标识符表达式(r41, (struct 标识符表达式*)cn_var_表达式);
  return r43;
  goto switch_merge_2580;

  case_body_2585:
  r44 = cn_var_上下文;
  r45 = cn_var_表达式;
  r46 = 检查函数调用表达式(r44, (struct 函数调用表达式*)cn_var_表达式);
  return r46;
  goto switch_merge_2580;

  case_body_2586:
  r47 = cn_var_上下文;
  r48 = cn_var_表达式;
  r49 = 检查成员访问表达式(r47, (struct 成员访问表达式*)cn_var_表达式);
  return r49;
  goto switch_merge_2580;

  case_body_2587:
  r50 = cn_var_上下文;
  r51 = cn_var_表达式;
  r52 = 检查数组访问表达式(r50, (struct 数组访问表达式*)cn_var_表达式);
  return r52;
  goto switch_merge_2580;

  case_body_2588:
  r53 = cn_var_上下文;
  r54 = cn_var_表达式;
  r55 = 检查赋值表达式(r53, (struct 赋值表达式*)cn_var_表达式);
  return r55;
  goto switch_merge_2580;

  case_body_2589:
  r56 = cn_var_上下文;
  r57 = cn_var_表达式;
  r58 = 检查三元表达式(r56, (struct 三元表达式*)cn_var_表达式);
  return r58;
  goto switch_merge_2580;

  case_default_2590:
  return 0;
  goto switch_merge_2580;

  switch_merge_2580:
  return NULL;
}

struct 类型信息* 检查二元表达式(struct 语义检查上下文* cn_var_上下文, struct 二元表达式* cn_var_二元节点) {
  long long r1, r13, r16, r19, r22, r25, r28, r31, r34, r37, r40, r43, r46, r49, r52, r55, r58, r60, r61, r63, r65, r69, r71, r73, r74, r76, r78, r82, r84;
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
  struct 语义检查上下文* r66;
  struct 二元表达式* r67;
  struct 类型信息* r72;
  struct 语义检查上下文* r79;
  struct 二元表达式* r80;
  _Bool r14;
  _Bool r17;
  _Bool r20;
  _Bool r23;
  _Bool r26;
  _Bool r29;
  _Bool r32;
  _Bool r35;
  _Bool r38;
  _Bool r41;
  _Bool r44;
  _Bool r47;
  _Bool r50;
  _Bool r53;
  _Bool r56;
  _Bool r59;
  _Bool r62;
  _Bool r64;
  _Bool r75;
  _Bool r77;
  struct AST节点 r68;
  struct AST节点 r81;
  enum 二元运算符 r11;
  enum 二元运算符 r12;
  enum 二元运算符 r15;
  enum 二元运算符 r18;
  enum 二元运算符 r21;
  enum 二元运算符 r24;
  enum 二元运算符 r27;
  enum 二元运算符 r30;
  enum 二元运算符 r33;
  enum 二元运算符 r36;
  enum 二元运算符 r39;
  enum 二元运算符 r42;
  enum 二元运算符 r45;
  enum 二元运算符 r48;
  enum 二元运算符 r51;
  enum 二元运算符 r54;
  enum 二元运算符 r57;
  enum 诊断错误码 r70;
  enum 诊断错误码 r83;

  entry:
  r0 = cn_var_二元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2599; else goto if_merge_2600;

  if_then_2599:
  return 0;
  goto if_merge_2600;

  if_merge_2600:
  void cn_var_左类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_二元节点;
  r4 = r3->左操作数;
  r5 = 检查表达式(r2, r4);
  cn_var_左类型_0 = r5;
  void cn_var_右类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_二元节点;
  r8 = r7->右操作数;
  r9 = 检查表达式(r6, r8);
  cn_var_右类型_1 = r9;
  r10 = cn_var_二元节点;
  r11 = r10->运算符;
  r12 = cn_var_二元运算符;
  r13 = r12.加;
  r14 = r11 == r13;
  if (r14) goto case_body_2602; else goto switch_check_2619;

  switch_check_2619:
  r15 = cn_var_二元运算符;
  r16 = r15.减;
  r17 = r11 == r16;
  if (r17) goto case_body_2603; else goto switch_check_2620;

  switch_check_2620:
  r18 = cn_var_二元运算符;
  r19 = r18.乘;
  r20 = r11 == r19;
  if (r20) goto case_body_2604; else goto switch_check_2621;

  switch_check_2621:
  r21 = cn_var_二元运算符;
  r22 = r21.除;
  r23 = r11 == r22;
  if (r23) goto case_body_2605; else goto switch_check_2622;

  switch_check_2622:
  r24 = cn_var_二元运算符;
  r25 = r24.取模;
  r26 = r11 == r25;
  if (r26) goto case_body_2606; else goto switch_check_2623;

  switch_check_2623:
  r27 = cn_var_二元运算符;
  r28 = r27.等于;
  r29 = r11 == r28;
  if (r29) goto case_body_2607; else goto switch_check_2624;

  switch_check_2624:
  r30 = cn_var_二元运算符;
  r31 = r30.不等于;
  r32 = r11 == r31;
  if (r32) goto case_body_2608; else goto switch_check_2625;

  switch_check_2625:
  r33 = cn_var_二元运算符;
  r34 = r33.小于;
  r35 = r11 == r34;
  if (r35) goto case_body_2609; else goto switch_check_2626;

  switch_check_2626:
  r36 = cn_var_二元运算符;
  r37 = r36.小于等于;
  r38 = r11 == r37;
  if (r38) goto case_body_2610; else goto switch_check_2627;

  switch_check_2627:
  r39 = cn_var_二元运算符;
  r40 = r39.大于;
  r41 = r11 == r40;
  if (r41) goto case_body_2611; else goto switch_check_2628;

  switch_check_2628:
  r42 = cn_var_二元运算符;
  r43 = r42.大于等于;
  r44 = r11 == r43;
  if (r44) goto case_body_2612; else goto switch_check_2629;

  switch_check_2629:
  r45 = cn_var_二元运算符;
  r46 = r45.位与;
  r47 = r11 == r46;
  if (r47) goto case_body_2613; else goto switch_check_2630;

  switch_check_2630:
  r48 = cn_var_二元运算符;
  r49 = r48.位或;
  r50 = r11 == r49;
  if (r50) goto case_body_2614; else goto switch_check_2631;

  switch_check_2631:
  r51 = cn_var_二元运算符;
  r52 = r51.位异或;
  r53 = r11 == r52;
  if (r53) goto case_body_2615; else goto switch_check_2632;

  switch_check_2632:
  r54 = cn_var_二元运算符;
  r55 = r54.左移;
  r56 = r11 == r55;
  if (r56) goto case_body_2616; else goto switch_check_2633;

  switch_check_2633:
  r57 = cn_var_二元运算符;
  r58 = r57.右移;
  r59 = r11 == r58;
  if (r59) goto case_body_2617; else goto case_default_2618;

  case_body_2602:
  goto switch_merge_2601;

  case_body_2603:
  goto switch_merge_2601;

  case_body_2604:
  goto switch_merge_2601;

  case_body_2605:
  goto switch_merge_2601;

  case_body_2606:
  r61 = cn_var_左类型_0;
  r62 = 是数值类型(r61);
  if (r62) goto logic_rhs_2636; else goto logic_merge_2637;

  if_then_2634:
  r65 = cn_var_左类型_0;
  return r65;
  goto if_merge_2635;

  if_merge_2635:
  r66 = cn_var_上下文;
  r67 = cn_var_二元节点;
  r68 = r67->节点基类;
  r69 = r68.位置;
  r70 = cn_var_诊断错误码;
  r71 = r70.语义_类型不匹配;
  报告错误(r66, r69, r71);
  return 0;
  goto switch_merge_2601;

  logic_rhs_2636:
  r63 = cn_var_右类型_1;
  r64 = 是数值类型(r63);
  goto logic_merge_2637;

  logic_merge_2637:
  if (r64) goto if_then_2634; else goto if_merge_2635;

  case_body_2607:
  goto switch_merge_2601;

  case_body_2608:
  goto switch_merge_2601;

  case_body_2609:
  goto switch_merge_2601;

  case_body_2610:
  goto switch_merge_2601;

  case_body_2611:
  goto switch_merge_2601;

  case_body_2612:
  r72 = 创建布尔类型();
  return r72;
  goto switch_merge_2601;

  case_body_2613:
  goto switch_merge_2601;

  case_body_2614:
  goto switch_merge_2601;

  case_body_2615:
  goto switch_merge_2601;

  case_body_2616:
  goto switch_merge_2601;

  case_body_2617:
  r74 = cn_var_左类型_0;
  r75 = 是整数类型(r74);
  if (r75) goto logic_rhs_2640; else goto logic_merge_2641;

  if_then_2638:
  r78 = cn_var_左类型_0;
  return r78;
  goto if_merge_2639;

  if_merge_2639:
  r79 = cn_var_上下文;
  r80 = cn_var_二元节点;
  r81 = r80->节点基类;
  r82 = r81.位置;
  r83 = cn_var_诊断错误码;
  r84 = r83.语义_类型不匹配;
  报告错误(r79, r82, r84);
  return 0;
  goto switch_merge_2601;

  logic_rhs_2640:
  r76 = cn_var_右类型_1;
  r77 = 是整数类型(r76);
  goto logic_merge_2641;

  logic_merge_2641:
  if (r77) goto if_then_2638; else goto if_merge_2639;

  case_default_2618:
  return 0;
  goto switch_merge_2601;

  switch_merge_2601:
  return NULL;
}

struct 类型信息* 检查一元表达式(struct 语义检查上下文* cn_var_上下文, struct 一元表达式* cn_var_一元节点) {
  long long r1, r9, r12, r15, r18, r21, r24, r27, r30, r33, r35, r37, r41, r43, r45, r47, r51, r53, r54, r56, r58, r59, r63, r65, r66, r68, r72, r74;
  struct 一元表达式* r0;
  struct 语义检查上下文* r2;
  struct 一元表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 一元表达式* r6;
  struct 语义检查上下文* r38;
  struct 一元表达式* r39;
  struct 类型信息* r44;
  struct 语义检查上下文* r48;
  struct 一元表达式* r49;
  struct 类型信息* r55;
  struct 语义检查上下文* r60;
  struct 一元表达式* r61;
  struct 语义检查上下文* r69;
  struct 一元表达式* r70;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;
  _Bool r34;
  _Bool r36;
  _Bool r46;
  _Bool r57;
  _Bool r67;
  struct AST节点 r40;
  struct AST节点 r50;
  struct AST节点 r62;
  struct AST节点 r71;
  enum 一元运算符 r7;
  enum 一元运算符 r8;
  enum 一元运算符 r11;
  enum 一元运算符 r14;
  enum 一元运算符 r17;
  enum 一元运算符 r20;
  enum 一元运算符 r23;
  enum 一元运算符 r26;
  enum 一元运算符 r29;
  enum 一元运算符 r32;
  enum 诊断错误码 r42;
  enum 诊断错误码 r52;
  enum 诊断错误码 r64;
  enum 诊断错误码 r73;

  entry:
  r0 = cn_var_一元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2642; else goto if_merge_2643;

  if_then_2642:
  return 0;
  goto if_merge_2643;

  if_merge_2643:
  void cn_var_操作数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_一元节点;
  r4 = r3->操作数;
  r5 = 检查表达式(r2, r4);
  cn_var_操作数类型_0 = r5;
  r6 = cn_var_一元节点;
  r7 = r6->运算符;
  r8 = cn_var_一元运算符;
  r9 = r8.负号;
  r10 = r7 == r9;
  if (r10) goto case_body_2645; else goto switch_check_2655;

  switch_check_2655:
  r11 = cn_var_一元运算符;
  r12 = r11.逻辑非;
  r13 = r7 == r12;
  if (r13) goto case_body_2646; else goto switch_check_2656;

  switch_check_2656:
  r14 = cn_var_一元运算符;
  r15 = r14.位取反;
  r16 = r7 == r15;
  if (r16) goto case_body_2647; else goto switch_check_2657;

  switch_check_2657:
  r17 = cn_var_一元运算符;
  r18 = r17.取地址;
  r19 = r7 == r18;
  if (r19) goto case_body_2648; else goto switch_check_2658;

  switch_check_2658:
  r20 = cn_var_一元运算符;
  r21 = r20.解引用;
  r22 = r7 == r21;
  if (r22) goto case_body_2649; else goto switch_check_2659;

  switch_check_2659:
  r23 = cn_var_一元运算符;
  r24 = r23.前置自增;
  r25 = r7 == r24;
  if (r25) goto case_body_2650; else goto switch_check_2660;

  switch_check_2660:
  r26 = cn_var_一元运算符;
  r27 = r26.前置自减;
  r28 = r7 == r27;
  if (r28) goto case_body_2651; else goto switch_check_2661;

  switch_check_2661:
  r29 = cn_var_一元运算符;
  r30 = r29.后置自增;
  r31 = r7 == r30;
  if (r31) goto case_body_2652; else goto switch_check_2662;

  switch_check_2662:
  r32 = cn_var_一元运算符;
  r33 = r32.后置自减;
  r34 = r7 == r33;
  if (r34) goto case_body_2653; else goto case_default_2654;

  case_body_2645:
  r35 = cn_var_操作数类型_0;
  r36 = 是数值类型(r35);
  if (r36) goto if_then_2663; else goto if_merge_2664;

  if_then_2663:
  r37 = cn_var_操作数类型_0;
  return r37;
  goto if_merge_2664;

  if_merge_2664:
  r38 = cn_var_上下文;
  r39 = cn_var_一元节点;
  r40 = r39->节点基类;
  r41 = r40.位置;
  r42 = cn_var_诊断错误码;
  r43 = r42.语义_类型不匹配;
  报告错误(r38, r41, r43);
  return 0;
  goto switch_merge_2644;

  case_body_2646:
  r44 = 创建布尔类型();
  return r44;
  goto switch_merge_2644;

  case_body_2647:
  r45 = cn_var_操作数类型_0;
  r46 = 是整数类型(r45);
  if (r46) goto if_then_2665; else goto if_merge_2666;

  if_then_2665:
  r47 = cn_var_操作数类型_0;
  return r47;
  goto if_merge_2666;

  if_merge_2666:
  r48 = cn_var_上下文;
  r49 = cn_var_一元节点;
  r50 = r49->节点基类;
  r51 = r50.位置;
  r52 = cn_var_诊断错误码;
  r53 = r52.语义_类型不匹配;
  报告错误(r48, r51, r53);
  return 0;
  goto switch_merge_2644;

  case_body_2648:
  r54 = cn_var_操作数类型_0;
  r55 = 创建指针类型(r54);
  return r55;
  goto switch_merge_2644;

  case_body_2649:
  r56 = cn_var_操作数类型_0;
  r57 = 是指针类型(r56);
  if (r57) goto if_then_2667; else goto if_merge_2668;

  if_then_2667:
  r58 = cn_var_操作数类型_0;
  r59 = r58.指向类型;
  return r59;
  goto if_merge_2668;

  if_merge_2668:
  r60 = cn_var_上下文;
  r61 = cn_var_一元节点;
  r62 = r61->节点基类;
  r63 = r62.位置;
  r64 = cn_var_诊断错误码;
  r65 = r64.语义_类型不匹配;
  报告错误(r60, r63, r65);
  return 0;
  goto switch_merge_2644;

  case_body_2650:
  goto switch_merge_2644;

  case_body_2651:
  goto switch_merge_2644;

  case_body_2652:
  goto switch_merge_2644;

  case_body_2653:
  r66 = cn_var_操作数类型_0;
  r67 = 是数值类型(r66);
  if (r67) goto if_then_2669; else goto if_merge_2670;

  if_then_2669:
  r68 = cn_var_操作数类型_0;
  return r68;
  goto if_merge_2670;

  if_merge_2670:
  r69 = cn_var_上下文;
  r70 = cn_var_一元节点;
  r71 = r70->节点基类;
  r72 = r71.位置;
  r73 = cn_var_诊断错误码;
  r74 = r73.语义_类型不匹配;
  报告错误(r69, r72, r74);
  return 0;
  goto switch_merge_2644;

  case_default_2654:
  return 0;
  goto switch_merge_2644;

  switch_merge_2644:
  return NULL;
}

struct 类型信息* 检查字面量表达式(struct 语义检查上下文* cn_var_上下文, struct 字面量表达式* cn_var_字面量节点) {
  long long r1, r5, r8, r11, r14, r17, r19, r21;
  struct 字面量表达式* r0;
  struct 字面量表达式* r2;
  struct 类型信息* r20;
  struct 类型信息* r22;
  struct 类型信息* r23;
  struct 类型信息* r24;
  struct 类型信息* r25;
  struct 类型信息* r26;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  enum 字面量类型 r3;
  enum 字面量类型 r4;
  enum 字面量类型 r7;
  enum 字面量类型 r10;
  enum 字面量类型 r13;
  enum 字面量类型 r16;

  entry:
  r0 = cn_var_字面量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2671; else goto if_merge_2672;

  if_then_2671:
  return 0;
  goto if_merge_2672;

  if_merge_2672:
  r2 = cn_var_字面量节点;
  r3 = r2->类型;
  r4 = cn_var_字面量类型;
  r5 = r4.整数字面量;
  r6 = r3 == r5;
  if (r6) goto case_body_2674; else goto switch_check_2680;

  switch_check_2680:
  r7 = cn_var_字面量类型;
  r8 = r7.浮点字面量;
  r9 = r3 == r8;
  if (r9) goto case_body_2675; else goto switch_check_2681;

  switch_check_2681:
  r10 = cn_var_字面量类型;
  r11 = r10.字符串字面量;
  r12 = r3 == r11;
  if (r12) goto case_body_2676; else goto switch_check_2682;

  switch_check_2682:
  r13 = cn_var_字面量类型;
  r14 = r13.布尔字面量;
  r15 = r3 == r14;
  if (r15) goto case_body_2677; else goto switch_check_2683;

  switch_check_2683:
  r16 = cn_var_字面量类型;
  r17 = r16.空指针字面量;
  r18 = r3 == r17;
  if (r18) goto case_body_2678; else goto case_default_2679;

  case_body_2674:
  r19 = cn_var_整数大小;
  r20 = 创建整数类型("整数", r19, 1);
  return r20;
  goto switch_merge_2673;

  case_body_2675:
  r21 = cn_var_小数大小;
  r22 = 创建小数类型("小数", r21);
  return r22;
  goto switch_merge_2673;

  case_body_2676:
  r23 = 创建字符串类型();
  return r23;
  goto switch_merge_2673;

  case_body_2677:
  r24 = 创建布尔类型();
  return r24;
  goto switch_merge_2673;

  case_body_2678:
  r25 = 创建空类型();
  r26 = 创建指针类型(r25);
  return r26;
  goto switch_merge_2673;

  case_default_2679:
  return 0;
  goto switch_merge_2673;

  switch_merge_2673:
  return NULL;
}

struct 类型信息* 检查标识符表达式(struct 语义检查上下文* cn_var_上下文, struct 标识符表达式* cn_var_标识符节点) {
  long long r1, r7, r8, r12, r14, r15, r16, r17, r18, r19;
  char* r5;
  struct 标识符表达式* r0;
  struct 语义检查上下文* r2;
  struct 符号表管理器* r3;
  struct 标识符表达式* r4;
  struct 符号* r6;
  struct 语义检查上下文* r9;
  struct 标识符表达式* r10;
  struct 语义检查上下文* r20;
  struct 类型推断上下文* r21;
  struct 类型信息* r22;
  struct AST节点 r11;
  enum 诊断错误码 r13;

  entry:
  r0 = cn_var_标识符节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2684; else goto if_merge_2685;

  if_then_2684:
  return 0;
  goto if_merge_2685;

  if_merge_2685:
  void cn_var_符号指针_0;
  r2 = cn_var_上下文;
  r3 = r2->符号表;
  r4 = cn_var_标识符节点;
  r5 = r4->名称;
  r6 = 查找符号(r3, r5);
  cn_var_符号指针_0 = r6;
  r7 = cn_var_符号指针_0;
  r8 = r7 == 0;
  if (r8) goto if_then_2686; else goto if_merge_2687;

  if_then_2686:
  r9 = cn_var_上下文;
  r10 = cn_var_标识符节点;
  r11 = r10->节点基类;
  r12 = r11.位置;
  r13 = cn_var_诊断错误码;
  r14 = r13.语义_未定义标识符;
  报告错误(r9, r12, r14);
  return 0;
  goto if_merge_2687;

  if_merge_2687:
  r15 = cn_var_符号指针_0;
  r16 = r15.类型信息;
  r17 = r16 != 0;
  if (r17) goto if_then_2688; else goto if_merge_2689;

  if_then_2688:
  r18 = cn_var_符号指针_0;
  r19 = r18.类型信息;
  r20 = cn_var_上下文;
  r21 = r20->类型上下文;
  r22 = 从类型节点推断(r19, r21);
  return r22;
  goto if_merge_2689;

  if_merge_2689:
  return 0;
}

struct 类型信息* 检查函数调用表达式(struct 语义检查上下文* cn_var_上下文, struct 函数调用表达式* cn_var_调用节点) {
  long long r1, r6, r7, r8, r9, r11, r15, r17, r19, r20, r21, r22, r26, r28, r29, r30, r32, r33, r34, r35, r36, r37, r41, r42, r44, r45, r46, r47, r48, r49, r51, r53, r54, r58, r59, r60, r61, r63, r64, r65, r66, r67;
  void* r40;
  void* r57;
  struct 函数调用表达式* r0;
  struct 语义检查上下文* r2;
  struct 函数调用表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r12;
  struct 函数调用表达式* r13;
  struct 函数调用表达式* r18;
  struct 语义检查上下文* r23;
  struct 函数调用表达式* r24;
  struct 函数调用表达式* r31;
  struct 语义检查上下文* r38;
  struct 函数调用表达式* r39;
  struct 类型信息* r43;
  struct 语义检查上下文* r55;
  struct 函数调用表达式* r56;
  _Bool r10;
  struct AST节点 r14;
  struct AST节点 r25;
  enum 类型兼容性 r50;
  enum 诊断错误码 r16;
  enum 诊断错误码 r27;
  enum 类型兼容性 r52;
  enum 诊断错误码 r62;

  entry:
  r0 = cn_var_调用节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2690; else goto if_merge_2691;

  if_then_2690:
  return 0;
  goto if_merge_2691;

  if_merge_2691:
  void cn_var_函数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_调用节点;
  r4 = r3->被调函数;
  r5 = 检查表达式(r2, r4);
  cn_var_函数类型_0 = r5;
  r7 = cn_var_函数类型_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_2695; else goto logic_rhs_2694;

  if_then_2692:
  r12 = cn_var_上下文;
  r13 = cn_var_调用节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  r16 = cn_var_诊断错误码;
  r17 = r16.语义_不可调用;
  报告错误(r12, r15, r17);
  return 0;
  goto if_merge_2693;

  if_merge_2693:
  r18 = cn_var_调用节点;
  r19 = r18->参数个数;
  r20 = cn_var_函数类型_0;
  r21 = r20.参数个数;
  r22 = r19 != r21;
  if (r22) goto if_then_2696; else goto if_merge_2697;

  logic_rhs_2694:
  r9 = cn_var_函数类型_0;
  r10 = 是可调用类型(r9);
  r11 = r10 == 0;
  goto logic_merge_2695;

  logic_merge_2695:
  if (r11) goto if_then_2692; else goto if_merge_2693;

  if_then_2696:
  r23 = cn_var_上下文;
  r24 = cn_var_调用节点;
  r25 = r24->节点基类;
  r26 = r25.位置;
  r27 = cn_var_诊断错误码;
  r28 = r27.语义_参数数量不匹配;
  报告错误(r23, r26, r28);
  goto if_merge_2697;

  if_merge_2697:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2698;

  for_cond_2698:
  r30 = cn_var_i_1;
  r31 = cn_var_调用节点;
  r32 = r31->参数个数;
  r33 = r30 < r32;
  if (r33) goto logic_rhs_2702; else goto logic_merge_2703;

  for_body_2699:
  void cn_var_实参类型_2;
  r38 = cn_var_上下文;
  r39 = cn_var_调用节点;
  r40 = r39->参数;
  r41 = cn_var_i_1;
  r42 = *(void*)cn_rt_array_get_element(r40, r41, 8);
  r43 = 检查表达式(r38, r42);
  cn_var_实参类型_2 = r43;
  void cn_var_形参类型_3;
  r44 = cn_var_函数类型_0;
  r45 = r44.参数类型列表;
  r46 = cn_var_i_1;
  r47 = *(void*)cn_rt_array_get_element(r45, r46, 8);
  cn_var_形参类型_3 = r47;
  void cn_var_兼容性_4;
  r48 = cn_var_实参类型_2;
  r49 = cn_var_形参类型_3;
  r50 = 检查类型兼容性(r48, r49);
  cn_var_兼容性_4 = r50;
  r51 = cn_var_兼容性_4;
  r52 = cn_var_类型兼容性;
  r53 = r52.不兼容;
  r54 = r51 == r53;
  if (r54) goto if_then_2704; else goto if_merge_2705;

  for_update_2700:
  r64 = cn_var_i_1;
  r65 = r64 + 1;
  cn_var_i_1 = r65;
  goto for_cond_2698;

  for_exit_2701:
  r66 = cn_var_函数类型_0;
  r67 = r66.返回类型;
  return r67;

  logic_rhs_2702:
  r34 = cn_var_i_1;
  r35 = cn_var_函数类型_0;
  r36 = r35.参数个数;
  r37 = r34 < r36;
  goto logic_merge_2703;

  logic_merge_2703:
  if (r37) goto for_body_2699; else goto for_exit_2701;

  if_then_2704:
  r55 = cn_var_上下文;
  r56 = cn_var_调用节点;
  r57 = r56->参数;
  r58 = cn_var_i_1;
  r59 = *(void*)cn_rt_array_get_element(r57, r58, 8);
  r60 = r59.节点基类;
  r61 = r60.位置;
  r62 = cn_var_诊断错误码;
  r63 = r62.语义_参数类型不匹配;
  报告错误(r55, r61, r63);
  goto if_merge_2705;

  if_merge_2705:
  goto for_update_2700;
  return NULL;
}

struct 类型信息* 检查成员访问表达式(struct 语义检查上下文* cn_var_上下文, struct 成员访问表达式* cn_var_成员节点) {
  long long r1, r6, r7, r10, r12, r16, r18, r19, r20, r21, r22, r24, r26, r27, r31, r32, r36, r38, r39, r44, r48, r50, r51, r52, r53, r54, r55;
  char* r29;
  struct 成员访问表达式* r0;
  struct 语义检查上下文* r2;
  struct 成员访问表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 成员访问表达式* r8;
  struct 语义检查上下文* r13;
  struct 成员访问表达式* r14;
  struct 成员访问表达式* r28;
  struct 符号* r30;
  struct 语义检查上下文* r33;
  struct 成员访问表达式* r34;
  struct 语义检查上下文* r40;
  struct 符号表管理器* r41;
  struct 作用域* r42;
  struct 语义检查上下文* r45;
  struct 成员访问表达式* r46;
  struct 语义检查上下文* r56;
  struct 类型推断上下文* r57;
  struct 类型信息* r58;
  _Bool r9;
  _Bool r11;
  _Bool r23;
  _Bool r25;
  _Bool r43;
  struct AST节点 r15;
  struct AST节点 r35;
  struct AST节点 r47;
  enum 诊断错误码 r17;
  enum 诊断错误码 r37;
  enum 诊断错误码 r49;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2706; else goto if_merge_2707;

  if_then_2706:
  return 0;
  goto if_merge_2707;

  if_merge_2707:
  void cn_var_对象类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_成员节点;
  r4 = r3->对象;
  r5 = 检查表达式(r2, r4);
  cn_var_对象类型_0 = r5;
  r6 = cn_var_对象类型_0;
  r7 = r6 == 0;
  if (r7) goto if_then_2708; else goto if_merge_2709;

  if_then_2708:
  return 0;
  goto if_merge_2709;

  if_merge_2709:
  r8 = cn_var_成员节点;
  r9 = r8->是指针访问;
  if (r9) goto if_then_2710; else goto if_merge_2711;

  if_then_2710:
  r10 = cn_var_对象类型_0;
  r11 = 是指针类型(r10);
  r12 = r11 == 0;
  if (r12) goto if_then_2712; else goto if_merge_2713;

  if_merge_2711:
  void cn_var_成员符号_1;
  cn_var_成员符号_1 = 0;
  r22 = cn_var_对象类型_0;
  r23 = 是结构体类型(r22);
  if (r23) goto logic_merge_2717; else goto logic_rhs_2716;

  if_then_2712:
  r13 = cn_var_上下文;
  r14 = cn_var_成员节点;
  r15 = r14->节点基类;
  r16 = r15.位置;
  r17 = cn_var_诊断错误码;
  r18 = r17.语义_类型不匹配;
  报告错误(r13, r16, r18);
  return 0;
  goto if_merge_2713;

  if_merge_2713:
  r19 = cn_var_对象类型_0;
  r20 = r19.指向类型;
  cn_var_对象类型_0 = r20;
  goto if_merge_2711;

  if_then_2714:
  r26 = cn_var_对象类型_0;
  r27 = r26.类型符号;
  r28 = cn_var_成员节点;
  r29 = r28->成员名;
  r30 = 查找类成员(r27, r29);
  cn_var_成员符号_1 = r30;
  goto if_merge_2715;

  if_merge_2715:
  r31 = cn_var_成员符号_1;
  r32 = r31 == 0;
  if (r32) goto if_then_2718; else goto if_merge_2719;

  logic_rhs_2716:
  r24 = cn_var_对象类型_0;
  r25 = 是类类型(r24);
  goto logic_merge_2717;

  logic_merge_2717:
  if (r25) goto if_then_2714; else goto if_merge_2715;

  if_then_2718:
  r33 = cn_var_上下文;
  r34 = cn_var_成员节点;
  r35 = r34->节点基类;
  r36 = r35.位置;
  r37 = cn_var_诊断错误码;
  r38 = r37.语义_成员未找到;
  报告错误(r33, r36, r38);
  return 0;
  goto if_merge_2719;

  if_merge_2719:
  r39 = cn_var_成员符号_1;
  r40 = cn_var_上下文;
  r41 = r40->符号表;
  r42 = r41->当前作用域;
  r43 = 检查符号可访问性(r39, r42);
  r44 = r43 == 0;
  if (r44) goto if_then_2720; else goto if_merge_2721;

  if_then_2720:
  r45 = cn_var_上下文;
  r46 = cn_var_成员节点;
  r47 = r46->节点基类;
  r48 = r47.位置;
  r49 = cn_var_诊断错误码;
  r50 = r49.语义_访问被拒绝;
  报告错误(r45, r48, r50);
  goto if_merge_2721;

  if_merge_2721:
  r51 = cn_var_成员符号_1;
  r52 = r51.类型信息;
  r53 = r52 != 0;
  if (r53) goto if_then_2722; else goto if_merge_2723;

  if_then_2722:
  r54 = cn_var_成员符号_1;
  r55 = r54.类型信息;
  r56 = cn_var_上下文;
  r57 = r56->类型上下文;
  r58 = 从类型节点推断(r55, r57);
  return r58;
  goto if_merge_2723;

  if_merge_2723:
  return 0;
}

struct 类型信息* 检查数组访问表达式(struct 语义检查上下文* cn_var_上下文, struct 数组访问表达式* cn_var_数组节点) {
  long long r1, r10, r11, r13, r14, r16, r20, r22, r23, r25, r29, r30, r32, r33, r35, r36, r37, r38;
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
  struct 语义检查上下文* r26;
  struct 数组访问表达式* r27;
  struct 表达式节点* r28;
  _Bool r12;
  _Bool r15;
  _Bool r24;
  _Bool r34;
  struct AST节点 r19;
  enum 诊断错误码 r21;
  enum 诊断错误码 r31;

  entry:
  r0 = cn_var_数组节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2724; else goto if_merge_2725;

  if_then_2724:
  return 0;
  goto if_merge_2725;

  if_merge_2725:
  void cn_var_数组类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_数组节点;
  r4 = r3->数组;
  r5 = 检查表达式(r2, r4);
  cn_var_数组类型_0 = r5;
  void cn_var_索引类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_数组节点;
  r8 = r7->索引;
  r9 = 检查表达式(r6, r8);
  cn_var_索引类型_1 = r9;
  r11 = cn_var_数组类型_0;
  r12 = 是数组类型(r11);
  r13 = r12 == 0;
  if (r13) goto logic_rhs_2728; else goto logic_merge_2729;

  if_then_2726:
  r17 = cn_var_上下文;
  r18 = cn_var_数组节点;
  r19 = r18->节点基类;
  r20 = r19.位置;
  r21 = cn_var_诊断错误码;
  r22 = r21.语义_非结构体类型;
  报告错误(r17, r20, r22);
  return 0;
  goto if_merge_2727;

  if_merge_2727:
  r23 = cn_var_索引类型_1;
  r24 = 是整数类型(r23);
  r25 = r24 == 0;
  if (r25) goto if_then_2730; else goto if_merge_2731;

  logic_rhs_2728:
  r14 = cn_var_数组类型_0;
  r15 = 是指针类型(r14);
  r16 = r15 == 0;
  goto logic_merge_2729;

  logic_merge_2729:
  if (r16) goto if_then_2726; else goto if_merge_2727;

  if_then_2730:
  r26 = cn_var_上下文;
  r27 = cn_var_数组节点;
  r28 = r27->索引;
  r29 = r28->节点基类;
  r30 = r29.位置;
  r31 = cn_var_诊断错误码;
  r32 = r31.语义_类型不匹配;
  报告错误(r26, r30, r32);
  goto if_merge_2731;

  if_merge_2731:
  r33 = cn_var_数组类型_0;
  r34 = 是数组类型(r33);
  if (r34) goto if_then_2732; else goto if_else_2733;

  if_then_2732:
  r35 = cn_var_数组类型_0;
  r36 = r35.元素类型;
  return r36;
  goto if_merge_2734;

  if_else_2733:
  r37 = cn_var_数组类型_0;
  r38 = r37.指向类型;
  return r38;
  goto if_merge_2734;

  if_merge_2734:
  return NULL;
}

struct 类型信息* 检查赋值表达式(struct 语义检查上下文* cn_var_上下文, struct 赋值表达式* cn_var_赋值节点) {
  long long r1, r10, r11, r12, r13, r14, r15, r16, r18, r20, r21, r25, r27, r28;
  struct 赋值表达式* r0;
  struct 语义检查上下文* r2;
  struct 赋值表达式* r3;
  struct 表达式节点* r4;
  struct 类型信息* r5;
  struct 语义检查上下文* r6;
  struct 赋值表达式* r7;
  struct 表达式节点* r8;
  struct 类型信息* r9;
  struct 语义检查上下文* r22;
  struct 赋值表达式* r23;
  enum 类型兼容性 r17;
  struct AST节点 r24;
  enum 类型兼容性 r19;
  enum 诊断错误码 r26;

  entry:
  r0 = cn_var_赋值节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2735; else goto if_merge_2736;

  if_then_2735:
  return 0;
  goto if_merge_2736;

  if_merge_2736:
  void cn_var_目标类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_赋值节点;
  r4 = r3->目标;
  r5 = 检查表达式(r2, r4);
  cn_var_目标类型_0 = r5;
  void cn_var_值类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_赋值节点;
  r8 = r7->值;
  r9 = 检查表达式(r6, r8);
  cn_var_值类型_1 = r9;
  r11 = cn_var_目标类型_0;
  r12 = r11 != 0;
  if (r12) goto logic_rhs_2739; else goto logic_merge_2740;

  if_then_2737:
  void cn_var_兼容性_2;
  r15 = cn_var_值类型_1;
  r16 = cn_var_目标类型_0;
  r17 = 检查类型兼容性(r15, r16);
  cn_var_兼容性_2 = r17;
  r18 = cn_var_兼容性_2;
  r19 = cn_var_类型兼容性;
  r20 = r19.不兼容;
  r21 = r18 == r20;
  if (r21) goto if_then_2741; else goto if_merge_2742;

  if_merge_2738:
  r28 = cn_var_目标类型_0;
  return r28;

  logic_rhs_2739:
  r13 = cn_var_值类型_1;
  r14 = r13 != 0;
  goto logic_merge_2740;

  logic_merge_2740:
  if (r14) goto if_then_2737; else goto if_merge_2738;

  if_then_2741:
  r22 = cn_var_上下文;
  r23 = cn_var_赋值节点;
  r24 = r23->节点基类;
  r25 = r24.位置;
  r26 = cn_var_诊断错误码;
  r27 = r26.语义_无效赋值;
  报告错误(r22, r25, r27);
  goto if_merge_2742;

  if_merge_2742:
  goto if_merge_2738;
  return NULL;
}

struct 类型信息* 检查三元表达式(struct 语义检查上下文* cn_var_上下文, struct 三元表达式* cn_var_三元节点) {
  long long r1, r14, r15, r16, r17, r18, r20, r21, r25, r26, r28, r29, r30, r31, r32, r33, r34, r35, r37, r39, r40, r44, r46, r47;
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
  struct 语义检查上下文* r22;
  struct 三元表达式* r23;
  struct 表达式节点* r24;
  struct 语义检查上下文* r41;
  struct 三元表达式* r42;
  enum 类型兼容性 r36;
  struct AST节点 r43;
  enum 类型种类 r19;
  enum 诊断错误码 r27;
  enum 类型兼容性 r38;
  enum 诊断错误码 r45;

  entry:
  r0 = cn_var_三元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2743; else goto if_merge_2744;

  if_then_2743:
  return 0;
  goto if_merge_2744;

  if_merge_2744:
  void cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_三元节点;
  r4 = r3->条件;
  r5 = 检查表达式(r2, r4);
  cn_var_条件类型_0 = r5;
  void cn_var_真值类型_1;
  r6 = cn_var_上下文;
  r7 = cn_var_三元节点;
  r8 = r7->真值;
  r9 = 检查表达式(r6, r8);
  cn_var_真值类型_1 = r9;
  void cn_var_假值类型_2;
  r10 = cn_var_上下文;
  r11 = cn_var_三元节点;
  r12 = r11->假值;
  r13 = 检查表达式(r10, r12);
  cn_var_假值类型_2 = r13;
  r15 = cn_var_条件类型_0;
  r16 = r15 != 0;
  if (r16) goto logic_rhs_2747; else goto logic_merge_2748;

  if_then_2745:
  r22 = cn_var_上下文;
  r23 = cn_var_三元节点;
  r24 = r23->条件;
  r25 = r24->节点基类;
  r26 = r25.位置;
  r27 = cn_var_诊断错误码;
  r28 = r27.语义_类型不匹配;
  报告错误(r22, r26, r28);
  goto if_merge_2746;

  if_merge_2746:
  r30 = cn_var_真值类型_1;
  r31 = r30 != 0;
  if (r31) goto logic_rhs_2751; else goto logic_merge_2752;

  logic_rhs_2747:
  r17 = cn_var_条件类型_0;
  r18 = r17.种类;
  r19 = cn_var_类型种类;
  r20 = r19.布尔类型;
  r21 = r18 != r20;
  goto logic_merge_2748;

  logic_merge_2748:
  if (r21) goto if_then_2745; else goto if_merge_2746;

  if_then_2749:
  void cn_var_兼容性_3;
  r34 = cn_var_真值类型_1;
  r35 = cn_var_假值类型_2;
  r36 = 检查类型兼容性(r34, r35);
  cn_var_兼容性_3 = r36;
  r37 = cn_var_兼容性_3;
  r38 = cn_var_类型兼容性;
  r39 = r38.不兼容;
  r40 = r37 == r39;
  if (r40) goto if_then_2753; else goto if_merge_2754;

  if_merge_2750:
  r47 = cn_var_真值类型_1;
  return r47;

  logic_rhs_2751:
  r32 = cn_var_假值类型_2;
  r33 = r32 != 0;
  goto logic_merge_2752;

  logic_merge_2752:
  if (r33) goto if_then_2749; else goto if_merge_2750;

  if_then_2753:
  r41 = cn_var_上下文;
  r42 = cn_var_三元节点;
  r43 = r42->节点基类;
  r44 = r43.位置;
  r45 = cn_var_诊断错误码;
  r46 = r45.语义_类型不匹配;
  报告错误(r41, r44, r46);
  goto if_merge_2754;

  if_merge_2754:
  goto if_merge_2750;
  return NULL;
}

_Bool 是常量表达式(struct 表达式节点* cn_var_表达式) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29;

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

