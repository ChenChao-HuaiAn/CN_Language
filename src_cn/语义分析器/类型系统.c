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

// CN Language Enum Definitions
enum 类型种类 {
    类型种类_类型_空,
    类型种类_类型_整数,
    类型种类_类型_小数,
    类型种类_类型_布尔,
    类型种类_类型_字符串,
    类型种类_类型_字符,
    类型种类_类型_指针,
    类型种类_类型_数组,
    类型种类_类型_结构体,
    类型种类_类型_枚举,
    类型种类_类型_类,
    类型种类_类型_接口,
    类型种类_类型_函数,
    类型种类_类型_参数,
    类型种类_类型_未定义
};

enum 类型兼容性 {
    类型兼容性_完全兼容,
    类型兼容性_隐式转换,
    类型兼容性_显式转换,
    类型兼容性_不兼容
};

// CN Language Global Struct Forward Declarations
struct 类型信息;
struct 类型推断上下文;

// CN Language Global Struct Definitions
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

struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};

// Global Variables
long long cn_var_整数大小 = 4;
long long cn_var_小数大小 = 8;
long long cn_var_布尔大小 = 1;
long long cn_var_字符串大小 = 8;
long long cn_var_指针大小 = 8;

// Forward Declarations
struct 类型信息* 创建空类型();
struct 类型信息* 创建整数类型(const char*, long long, _Bool);
struct 类型信息* 创建小数类型(const char*, long long);
struct 类型信息* 创建布尔类型();
struct 类型信息* 创建字符串类型();
struct 类型信息* 创建指针类型(struct 类型信息*);
struct 类型信息* 创建数组类型(struct 类型信息*, long long*, long long);
struct 类型信息* 创建函数类型(struct 类型信息**, long long, struct 类型信息*);
struct 类型信息* 创建结构体类型(const char*, struct 符号*);
struct 类型信息* 创建枚举类型(const char*, struct 符号*);
struct 类型信息* 创建类类型(const char*, struct 符号*);
struct 类型信息* 创建接口类型(const char*, struct 符号*);
struct 类型信息* 创建未定义类型(const char*);
_Bool 类型相同(struct 类型信息*, struct 类型信息*);
enum 类型兼容性 检查类型兼容性(struct 类型信息*, struct 类型信息*);
_Bool 是派生类(struct 类型信息*, struct 类型信息*);
_Bool 实现接口(struct 类型信息*, struct 类型信息*);
_Bool 是数值类型(struct 类型信息*);
_Bool 是整数类型(struct 类型信息*);
_Bool 是指针类型(struct 类型信息*);
_Bool 是数组类型(struct 类型信息*);
_Bool 是结构体类型(struct 类型信息*);
_Bool 是类类型(struct 类型信息*);
_Bool 是函数类型(struct 类型信息*);
_Bool 是可调用类型(struct 类型信息*);
struct 类型信息* 从类型节点推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从基础类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从指针类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从数组类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从结构体类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从枚举类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从类类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从接口类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从函数类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型节点* 类型信息转类型节点(struct 类型信息*);
_Bool 是有效的类型种类(enum 类型种类);
char* 获取类型种类名称(enum 类型种类);

struct 类型信息* 创建空类型() {
  long long r0, r2;
  void* r1;
  struct 类型信息* r3;

  entry:
  long long cn_var_内存指针_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_内存指针_0 = r1;
  struct 类型信息* cn_var_类型_1;
  r2 = cn_var_内存指针_0;
  cn_var_类型_1 = (struct 类型信息*)cn_var_内存指针;
  r3 = cn_var_类型_1;
  return r3;
}

struct 类型信息* 创建整数类型(const char* cn_var_名称, long long cn_var_大小, _Bool cn_var_是有符号) {
  long long r0, r3, r4;
  char* r2;
  void* r1;
  struct 类型信息* r6;
  _Bool r5;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_大小;
  r4 = cn_var_大小;
  r5 = cn_var_是有符号;
  r6 = cn_var_类型_0;
  return r6;
}

struct 类型信息* 创建小数类型(const char* cn_var_名称, long long cn_var_大小) {
  long long r0, r3, r4;
  char* r2;
  void* r1;
  struct 类型信息* r5;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_大小;
  r4 = cn_var_大小;
  r5 = cn_var_类型_0;
  return r5;
}

struct 类型信息* 创建布尔类型() {
  long long r0, r2;
  void* r1;
  struct 类型信息* r3;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_布尔大小;
  r3 = cn_var_类型_0;
  return r3;
}

struct 类型信息* 创建字符串类型() {
  long long r0, r2, r3;
  void* r1;
  struct 类型信息* r4;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_字符串大小;
  r3 = cn_var_指针大小;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建指针类型(struct 类型信息* cn_var_指向类型) {
  long long r0, r4, r5, r6, r7, r10;
  char* r9;
  void* r1;
  struct 类型信息* r2;
  struct 类型信息* r3;
  struct 类型信息* r8;
  struct 类型信息* r11;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_指向类型;
  r3 = cn_var_指向类型;
  r4 = r3->指针层级;
  r5 = r4 + 1;
  r6 = cn_var_指针大小;
  r7 = cn_var_指针大小;
  r8 = cn_var_指向类型;
  r9 = r8->名称;
  r10 = r9 + "*";
  r11 = cn_var_类型_0;
  return r11;
}

struct 类型信息* 创建数组类型(struct 类型信息* cn_var_元素类型, long long* cn_var_维度大小, long long cn_var_维度) {
  long long r0, r3, r5, r6, r7, r8, r10, r12, r13, r14, r15, r16, r18, r19, r21;
  void* r1;
  struct 类型信息* r2;
  long long* r4;
  long long* r9;
  void* r11;
  struct 类型信息* r17;
  struct 类型信息* r20;
  struct 类型信息* r22;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_元素类型;
  r3 = cn_var_维度;
  r4 = cn_var_维度大小;
  long long cn_var_总数_1;
  cn_var_总数_1 = 1;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_536;

  for_cond_536:
  r5 = cn_var_i_2;
  r6 = cn_var_维度;
  r7 = r5 < r6;
  if (r7) goto for_body_537; else goto for_exit_539;

  for_body_537:
  r8 = cn_var_总数_1;
  r9 = cn_var_维度大小;
  r10 = cn_var_i_2;
  r11 = (void**)cn_rt_array_get_element(r9, r10, 8);
  r12 = r8 * r11;
  cn_var_总数_1 = r12;
  goto for_update_538;

  for_update_538:
  r13 = cn_var_i_2;
  r14 = r13 + 1;
  cn_var_i_2 = r14;
  goto for_cond_536;

  for_exit_539:
  r15 = cn_var_总数_1;
  r16 = cn_var_总数_1;
  r17 = cn_var_元素类型;
  r18 = r17->大小;
  r19 = r16 * r18;
  r20 = cn_var_元素类型;
  r21 = r20->对齐;
  r22 = cn_var_类型_0;
  return r22;
}

struct 类型信息* 创建函数类型(struct 类型信息** cn_var_参数类型列表, long long cn_var_参数个数, struct 类型信息* cn_var_返回类型) {
  long long r0, r3, r5, r6;
  void* r1;
  struct 类型信息** r2;
  struct 类型信息* r4;
  struct 类型信息* r7;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_参数类型列表;
  r3 = cn_var_参数个数;
  r4 = cn_var_返回类型;
  r5 = cn_var_指针大小;
  r6 = cn_var_指针大小;
  r7 = cn_var_类型_0;
  return r7;
}

struct 类型信息* 创建结构体类型(const char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r3;
  struct 类型信息* r4;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_类型符号;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建枚举类型(const char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0, r4, r5;
  char* r2;
  void* r1;
  struct 符号* r3;
  struct 类型信息* r6;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_类型符号;
  r4 = cn_var_整数大小;
  r5 = cn_var_整数大小;
  r6 = cn_var_类型_0;
  return r6;
}

struct 类型信息* 创建类类型(const char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r3;
  struct 类型信息* r4;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_类型符号;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建接口类型(const char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r3;
  struct 类型信息* r4;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_类型符号;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建未定义类型(const char* cn_var_名称) {
  long long r0;
  char* r2;
  void* r1;
  struct 类型信息* r3;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)0;
  r2 = cn_var_名称;
  r3 = cn_var_类型_0;
  return r3;
}

_Bool 类型相同(struct 类型信息* cn_var_类型1, struct 类型信息* cn_var_类型2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r62, r63, r64, r65, r66, r67, r68, r69, r70, r71, r72, r73, r74, r75, r76, r77, r78, r79, r80, r81, r82, r83, r84, r85, r86;

  entry:
  goto tail_rec_loop;
  return 0;
}

enum 类型兼容性 检查类型兼容性(struct 类型信息* cn_var_源类型, struct 类型信息* cn_var_目标类型) {
  long long r3, r6, r9, r13, r14, r17, r20, r21, r24, r27, r28, r31, r34, r35, r38, r41, r42, r45, r48, r49, r52, r55, r59, r62, r65;
  struct 类型信息* r0;
  struct 类型信息* r1;
  struct 类型信息* r4;
  struct 类型信息* r7;
  struct 类型信息* r10;
  struct 类型信息* r11;
  struct 类型信息* r15;
  struct 类型信息* r18;
  struct 类型信息* r22;
  struct 类型信息* r25;
  struct 类型信息* r29;
  struct 类型信息* r32;
  struct 类型信息* r36;
  struct 类型信息* r39;
  struct 类型信息* r43;
  struct 类型信息* r46;
  struct 类型信息* r50;
  struct 类型信息* r53;
  struct 类型信息* r56;
  struct 类型信息* r57;
  struct 类型信息* r60;
  struct 类型信息* r63;
  struct 类型信息* r66;
  struct 类型信息* r67;
  _Bool r2;
  _Bool r58;
  _Bool r68;
  enum 类型种类 r5;
  enum 类型种类 r8;
  enum 类型种类 r12;
  enum 类型种类 r16;
  enum 类型种类 r19;
  enum 类型种类 r23;
  enum 类型种类 r26;
  enum 类型种类 r30;
  enum 类型种类 r33;
  enum 类型种类 r37;
  enum 类型种类 r40;
  enum 类型种类 r44;
  enum 类型种类 r47;
  enum 类型种类 r51;
  enum 类型种类 r54;
  enum 类型种类 r61;
  enum 类型种类 r64;

  entry:
  r0 = cn_var_源类型;
  r1 = cn_var_目标类型;
  r2 = 类型相同(r0, r1);
  if (r2) goto if_then_585; else goto if_merge_586;

  if_then_585:
  return 类型兼容性_完全兼容;
  goto if_merge_586;

  if_merge_586:
  r4 = cn_var_源类型;
  r5 = r4->种类;
  r6 = r5 == 类型种类_类型_指针;
  if (r6) goto logic_rhs_589; else goto logic_merge_590;

  if_then_587:
  r10 = cn_var_源类型;
  r11 = r10->指向类型;
  r12 = r11->种类;
  r13 = r12 == 类型种类_类型_空;
  if (r13) goto if_then_591; else goto if_merge_592;

  if_merge_588:
  r15 = cn_var_源类型;
  r16 = r15->种类;
  r17 = r16 == 类型种类_类型_整数;
  if (r17) goto logic_rhs_595; else goto logic_merge_596;

  logic_rhs_589:
  r7 = cn_var_目标类型;
  r8 = r7->种类;
  r9 = r8 == 类型种类_类型_指针;
  goto logic_merge_590;

  logic_merge_590:
  if (r9) goto if_then_587; else goto if_merge_588;

  if_then_591:
  return 类型兼容性_隐式转换;
  goto if_merge_592;

  if_merge_592:
  goto if_merge_588;

  if_then_593:
  return 类型兼容性_隐式转换;
  goto if_merge_594;

  if_merge_594:
  r22 = cn_var_源类型;
  r23 = r22->种类;
  r24 = r23 == 类型种类_类型_整数;
  if (r24) goto logic_rhs_599; else goto logic_merge_600;

  logic_rhs_595:
  r18 = cn_var_目标类型;
  r19 = r18->种类;
  r20 = r19 == 类型种类_类型_小数;
  goto logic_merge_596;

  logic_merge_596:
  if (r20) goto if_then_593; else goto if_merge_594;

  if_then_597:
  return 类型兼容性_隐式转换;
  goto if_merge_598;

  if_merge_598:
  r29 = cn_var_源类型;
  r30 = r29->种类;
  r31 = r30 == 类型种类_类型_枚举;
  if (r31) goto logic_rhs_603; else goto logic_merge_604;

  logic_rhs_599:
  r25 = cn_var_目标类型;
  r26 = r25->种类;
  r27 = r26 == 类型种类_类型_整数;
  goto logic_merge_600;

  logic_merge_600:
  if (r27) goto if_then_597; else goto if_merge_598;

  if_then_601:
  return 类型兼容性_隐式转换;
  goto if_merge_602;

  if_merge_602:
  r36 = cn_var_源类型;
  r37 = r36->种类;
  r38 = r37 == 类型种类_类型_整数;
  if (r38) goto logic_rhs_607; else goto logic_merge_608;

  logic_rhs_603:
  r32 = cn_var_目标类型;
  r33 = r32->种类;
  r34 = r33 == 类型种类_类型_整数;
  goto logic_merge_604;

  logic_merge_604:
  if (r34) goto if_then_601; else goto if_merge_602;

  if_then_605:
  return 类型兼容性_显式转换;
  goto if_merge_606;

  if_merge_606:
  r43 = cn_var_源类型;
  r44 = r43->种类;
  r45 = r44 == 类型种类_类型_小数;
  if (r45) goto logic_rhs_611; else goto logic_merge_612;

  logic_rhs_607:
  r39 = cn_var_目标类型;
  r40 = r39->种类;
  r41 = r40 == 类型种类_类型_枚举;
  goto logic_merge_608;

  logic_merge_608:
  if (r41) goto if_then_605; else goto if_merge_606;

  if_then_609:
  return 类型兼容性_显式转换;
  goto if_merge_610;

  if_merge_610:
  r50 = cn_var_源类型;
  r51 = r50->种类;
  r52 = r51 == 类型种类_类型_类;
  if (r52) goto logic_rhs_615; else goto logic_merge_616;

  logic_rhs_611:
  r46 = cn_var_目标类型;
  r47 = r46->种类;
  r48 = r47 == 类型种类_类型_整数;
  goto logic_merge_612;

  logic_merge_612:
  if (r48) goto if_then_609; else goto if_merge_610;

  if_then_613:
  r56 = cn_var_源类型;
  r57 = cn_var_目标类型;
  r58 = 是派生类(r56, r57);
  if (r58) goto if_then_617; else goto if_merge_618;

  if_merge_614:
  r60 = cn_var_源类型;
  r61 = r60->种类;
  r62 = r61 == 类型种类_类型_类;
  if (r62) goto logic_rhs_621; else goto logic_merge_622;

  logic_rhs_615:
  r53 = cn_var_目标类型;
  r54 = r53->种类;
  r55 = r54 == 类型种类_类型_类;
  goto logic_merge_616;

  logic_merge_616:
  if (r55) goto if_then_613; else goto if_merge_614;

  if_then_617:
  return 类型兼容性_隐式转换;
  goto if_merge_618;

  if_merge_618:
  goto if_merge_614;

  if_then_619:
  r66 = cn_var_源类型;
  r67 = cn_var_目标类型;
  r68 = 实现接口(r66, r67);
  if (r68) goto if_then_623; else goto if_merge_624;

  if_merge_620:
  return 类型兼容性_不兼容;

  logic_rhs_621:
  r63 = cn_var_目标类型;
  r64 = r63->种类;
  r65 = r64 == 类型种类_类型_接口;
  goto logic_merge_622;

  logic_merge_622:
  if (r65) goto if_then_619; else goto if_merge_620;

  if_then_623:
  return 类型兼容性_隐式转换;
  goto if_merge_624;

  if_merge_624:
  goto if_merge_620;
  return 0;
}

_Bool 是派生类(struct 类型信息* cn_var_子类, struct 类型信息* cn_var_父类) {
  long long r0, r2, r4, r5, r8, r11, r14, r15, r16, r19, r20, r21;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 类型信息* r6;
  struct 类型信息* r9;
  struct 类型信息* r12;
  struct 符号* r13;
  struct 类型信息* r17;
  struct 符号* r18;
  enum 类型种类 r7;
  enum 类型种类 r10;

  entry:
  r1 = cn_var_子类;
  r2 = r1 == 0;
  if (r2) goto logic_merge_628; else goto logic_rhs_627;

  if_then_625:
  return 0;
  goto if_merge_626;

  if_merge_626:
  r6 = cn_var_子类;
  r7 = r6->种类;
  r8 = r7 != 类型种类_类型_类;
  if (r8) goto logic_merge_632; else goto logic_rhs_631;

  logic_rhs_627:
  r3 = cn_var_父类;
  r4 = r3 == 0;
  goto logic_merge_628;

  logic_merge_628:
  if (r4) goto if_then_625; else goto if_merge_626;

  if_then_629:
  return 0;
  goto if_merge_630;

  if_merge_630:
  long long cn_var_当前_0;
  r12 = cn_var_子类;
  r13 = r12->类型符号;
  cn_var_当前_0 = r13;
  goto while_cond_633;

  logic_rhs_631:
  r9 = cn_var_父类;
  r10 = r9->种类;
  r11 = r10 != 类型种类_类型_类;
  goto logic_merge_632;

  logic_merge_632:
  if (r11) goto if_then_629; else goto if_merge_630;

  while_cond_633:
  r14 = cn_var_当前_0;
  r15 = r14 != 0;
  if (r15) goto while_body_634; else goto while_exit_635;

  while_body_634:
  r16 = cn_var_当前_0;
  r17 = cn_var_父类;
  r18 = r17->类型符号;
  r19 = r16 == r18;
  if (r19) goto if_then_636; else goto if_merge_637;

  while_exit_635:
  return 0;

  if_then_636:
  return 1;
  goto if_merge_637;

  if_merge_637:
  r20 = cn_var_当前_0;
  r21 = r20.父类符号;
  cn_var_当前_0 = r21;
  goto while_cond_633;
  return 0;
}

_Bool 实现接口(struct 类型信息* cn_var_类类型, struct 类型信息* cn_var_接口类型) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42;

  entry:
  goto tail_rec_loop;
  return 0;
}

_Bool 是数值类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r5, r8;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r6;
  enum 类型种类 r4;
  enum 类型种类 r7;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_656; else goto if_merge_657;

  if_then_656:
  return 0;
  goto if_merge_657;

  if_merge_657:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 == 类型种类_类型_整数;
  if (r5) goto logic_merge_659; else goto logic_rhs_658;

  logic_rhs_658:
  r6 = cn_var_类型;
  r7 = r6->种类;
  r8 = r7 == 类型种类_类型_小数;
  goto logic_merge_659;

  logic_merge_659:
  return r8;
}

_Bool 是整数类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r5, r8;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r6;
  enum 类型种类 r4;
  enum 类型种类 r7;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_660; else goto if_merge_661;

  if_then_660:
  return 0;
  goto if_merge_661;

  if_merge_661:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 == 类型种类_类型_整数;
  if (r5) goto logic_merge_663; else goto logic_rhs_662;

  logic_rhs_662:
  r6 = cn_var_类型;
  r7 = r6->种类;
  r8 = r7 == 类型种类_类型_枚举;
  goto logic_merge_663;

  logic_merge_663:
  return r8;
}

_Bool 是指针类型(struct 类型信息* cn_var_类型) {
  long long r1, r4;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_664; else goto if_merge_665;

  if_then_664:
  return 0;
  goto if_merge_665;

  if_merge_665:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_指针;
  return r4;
}

_Bool 是数组类型(struct 类型信息* cn_var_类型) {
  long long r1, r4;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_666; else goto if_merge_667;

  if_then_666:
  return 0;
  goto if_merge_667;

  if_merge_667:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_数组;
  return r4;
}

_Bool 是结构体类型(struct 类型信息* cn_var_类型) {
  long long r1, r4;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_668; else goto if_merge_669;

  if_then_668:
  return 0;
  goto if_merge_669;

  if_merge_669:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_结构体;
  return r4;
}

_Bool 是类类型(struct 类型信息* cn_var_类型) {
  long long r1, r4;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_670; else goto if_merge_671;

  if_then_670:
  return 0;
  goto if_merge_671;

  if_merge_671:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_类;
  return r4;
}

_Bool 是函数类型(struct 类型信息* cn_var_类型) {
  long long r1, r4;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_672; else goto if_merge_673;

  if_then_672:
  return 0;
  goto if_merge_673;

  if_merge_673:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = r3 == 类型种类_类型_函数;
  return r4;
}

_Bool 是可调用类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r5, r6, r9, r13;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r7;
  struct 类型信息* r10;
  struct 类型信息* r11;
  enum 类型种类 r4;
  enum 类型种类 r8;
  enum 类型种类 r12;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_674; else goto if_merge_675;

  if_then_674:
  return 0;
  goto if_merge_675;

  if_merge_675:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = r4 == 类型种类_类型_函数;
  if (r5) goto logic_merge_677; else goto logic_rhs_676;

  logic_rhs_676:
  r7 = cn_var_类型;
  r8 = r7->种类;
  r9 = r8 == 类型种类_类型_指针;
  if (r9) goto logic_rhs_678; else goto logic_merge_679;

  logic_merge_677:
  return r13;

  logic_rhs_678:
  r10 = cn_var_类型;
  r11 = r10->指向类型;
  r12 = r11->种类;
  r13 = r12 == 类型种类_类型_函数;
  goto logic_merge_679;

  logic_merge_679:
  goto logic_merge_677;
  return 0;
}

struct 类型信息* 从类型节点推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r1;
  struct 类型节点* r0;
  struct 类型节点* r2;
  struct 类型节点* r13;
  struct 类型推断上下文* r14;
  struct 类型信息* r15;
  struct 类型节点* r16;
  struct 类型推断上下文* r17;
  struct 类型信息* r18;
  struct 类型节点* r19;
  struct 类型推断上下文* r20;
  struct 类型信息* r21;
  struct 类型节点* r22;
  struct 类型推断上下文* r23;
  struct 类型信息* r24;
  struct 类型节点* r25;
  struct 类型推断上下文* r26;
  struct 类型信息* r27;
  struct 类型节点* r28;
  struct 类型推断上下文* r29;
  struct 类型信息* r30;
  struct 类型节点* r31;
  struct 类型推断上下文* r32;
  struct 类型信息* r33;
  struct 类型节点* r34;
  struct 类型推断上下文* r35;
  struct 类型信息* r36;
  struct 类型节点* r37;
  struct 类型推断上下文* r38;
  struct 类型信息* r39;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  enum 节点类型 r3;

  entry:
  r0 = cn_var_类型节点指针;
  r1 = r0 == 0;
  if (r1) goto if_then_680; else goto if_merge_681;

  if_then_680:
  return 0;
  goto if_merge_681;

  if_merge_681:
  r2 = cn_var_类型节点指针;
  r3 = r2->类型;
  r4 = r3 == 节点类型_基础类型;
  if (r4) goto case_body_683; else goto switch_check_693;

  switch_check_693:
  r5 = r3 == 节点类型_指针类型;
  if (r5) goto case_body_684; else goto switch_check_694;

  switch_check_694:
  r6 = r3 == 节点类型_数组类型;
  if (r6) goto case_body_685; else goto switch_check_695;

  switch_check_695:
  r7 = r3 == 节点类型_结构体类型;
  if (r7) goto case_body_686; else goto switch_check_696;

  switch_check_696:
  r8 = r3 == 节点类型_枚举类型;
  if (r8) goto case_body_687; else goto switch_check_697;

  switch_check_697:
  r9 = r3 == 节点类型_类类型;
  if (r9) goto case_body_688; else goto switch_check_698;

  switch_check_698:
  r10 = r3 == 节点类型_接口类型;
  if (r10) goto case_body_689; else goto switch_check_699;

  switch_check_699:
  r11 = r3 == 节点类型_函数类型;
  if (r11) goto case_body_690; else goto switch_check_700;

  switch_check_700:
  r12 = r3 == 节点类型_标识符类型;
  if (r12) goto case_body_691; else goto case_default_692;

  case_body_683:
  r13 = cn_var_类型节点指针;
  r14 = cn_var_上下文;
  r15 = 从基础类型推断(r13, r14);
  return r15;
  goto switch_merge_682;

  case_body_684:
  r16 = cn_var_类型节点指针;
  r17 = cn_var_上下文;
  r18 = 从指针类型推断(r16, r17);
  return r18;
  goto switch_merge_682;

  case_body_685:
  r19 = cn_var_类型节点指针;
  r20 = cn_var_上下文;
  r21 = 从数组类型推断(r19, r20);
  return r21;
  goto switch_merge_682;

  case_body_686:
  r22 = cn_var_类型节点指针;
  r23 = cn_var_上下文;
  r24 = 从结构体类型推断(r22, r23);
  return r24;
  goto switch_merge_682;

  case_body_687:
  r25 = cn_var_类型节点指针;
  r26 = cn_var_上下文;
  r27 = 从枚举类型推断(r25, r26);
  return r27;
  goto switch_merge_682;

  case_body_688:
  r28 = cn_var_类型节点指针;
  r29 = cn_var_上下文;
  r30 = 从类类型推断(r28, r29);
  return r30;
  goto switch_merge_682;

  case_body_689:
  r31 = cn_var_类型节点指针;
  r32 = cn_var_上下文;
  r33 = 从接口类型推断(r31, r32);
  return r33;
  goto switch_merge_682;

  case_body_690:
  r34 = cn_var_类型节点指针;
  r35 = cn_var_上下文;
  r36 = 从函数类型推断(r34, r35);
  return r36;
  goto switch_merge_682;

  case_body_691:
  r37 = cn_var_类型节点指针;
  r38 = cn_var_上下文;
  r39 = 从基础类型推断(r37, r38);
  return r39;
  goto switch_merge_682;

  case_default_692:
  return 0;
  goto switch_merge_682;

  switch_merge_682:
  return NULL;
}

struct 类型信息* 从基础类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r2, r3, r4, r5, r7, r8, r9, r10, r12, r13, r14, r16, r17, r18, r20, r21, r22, r26, r28, r29, r30, r31, r32, r33, r34, r36, r37, r38, r39, r40, r42, r43, r44, r45, r46, r48, r49, r50, r51, r52, r54, r55, r56, r57, r58, r61;
  char* r1;
  struct 类型节点* r0;
  struct 类型信息* r6;
  struct 类型信息* r11;
  struct 类型信息* r15;
  struct 类型信息* r19;
  struct 类型信息* r23;
  struct 类型推断上下文* r24;
  struct 符号表管理器* r25;
  struct 符号* r27;
  struct 类型信息* r35;
  struct 类型信息* r41;
  struct 类型信息* r47;
  struct 类型信息* r53;
  struct 类型推断上下文* r59;
  struct 类型信息* r60;
  struct 类型信息* r62;

  entry:
  long long cn_var_名称_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->名称;
  cn_var_名称_0 = r1;
  r2 = cn_var_名称_0;
  r3 = 比较字符串(r2, "整数");
  r4 = r3 == 0;
  if (r4) goto if_then_701; else goto if_merge_702;

  if_then_701:
  r5 = cn_var_整数大小;
  r6 = 创建整数类型("整数", r5, 1);
  return r6;
  goto if_merge_702;

  if_merge_702:
  r7 = cn_var_名称_0;
  r8 = 比较字符串(r7, "小数");
  r9 = r8 == 0;
  if (r9) goto if_then_703; else goto if_merge_704;

  if_then_703:
  r10 = cn_var_小数大小;
  r11 = 创建小数类型("小数", r10);
  return r11;
  goto if_merge_704;

  if_merge_704:
  r12 = cn_var_名称_0;
  r13 = 比较字符串(r12, "布尔");
  r14 = r13 == 0;
  if (r14) goto if_then_705; else goto if_merge_706;

  if_then_705:
  r15 = 创建布尔类型();
  return r15;
  goto if_merge_706;

  if_merge_706:
  r16 = cn_var_名称_0;
  r17 = 比较字符串(r16, "字符串");
  r18 = r17 == 0;
  if (r18) goto if_then_707; else goto if_merge_708;

  if_then_707:
  r19 = 创建字符串类型();
  return r19;
  goto if_merge_708;

  if_merge_708:
  r20 = cn_var_名称_0;
  r21 = 比较字符串(r20, "空类型");
  r22 = r21 == 0;
  if (r22) goto if_then_709; else goto if_merge_710;

  if_then_709:
  r23 = 创建空类型();
  return r23;
  goto if_merge_710;

  if_merge_710:
  long long cn_var_符号指针_1;
  r24 = cn_var_上下文;
  r25 = r24->符号表;
  r26 = cn_var_名称_0;
  r27 = 查找符号(r25, r26);
  cn_var_符号指针_1 = r27;
  r28 = cn_var_符号指针_1;
  r29 = r28 != 0;
  if (r29) goto if_then_711; else goto if_merge_712;

  if_then_711:
  r30 = cn_var_符号指针_1;
  r31 = r30.种类;
  r32 = r31 == 符号类型_结构体符号;
  if (r32) goto if_then_713; else goto if_merge_714;

  if_merge_712:
  r61 = cn_var_名称_0;
  r62 = 创建未定义类型(r61);
  return r62;

  if_then_713:
  r33 = cn_var_名称_0;
  r34 = cn_var_符号指针_1;
  r35 = 创建结构体类型(r33, r34);
  return r35;
  goto if_merge_714;

  if_merge_714:
  r36 = cn_var_符号指针_1;
  r37 = r36.种类;
  r38 = r37 == 符号类型_枚举符号;
  if (r38) goto if_then_715; else goto if_merge_716;

  if_then_715:
  r39 = cn_var_名称_0;
  r40 = cn_var_符号指针_1;
  r41 = 创建枚举类型(r39, r40);
  return r41;
  goto if_merge_716;

  if_merge_716:
  r42 = cn_var_符号指针_1;
  r43 = r42.种类;
  r44 = r43 == 符号类型_类符号;
  if (r44) goto if_then_717; else goto if_merge_718;

  if_then_717:
  r45 = cn_var_名称_0;
  r46 = cn_var_符号指针_1;
  r47 = 创建类类型(r45, r46);
  return r47;
  goto if_merge_718;

  if_merge_718:
  r48 = cn_var_符号指针_1;
  r49 = r48.种类;
  r50 = r49 == 符号类型_接口符号;
  if (r50) goto if_then_719; else goto if_merge_720;

  if_then_719:
  r51 = cn_var_名称_0;
  r52 = cn_var_符号指针_1;
  r53 = 创建接口类型(r51, r52);
  return r53;
  goto if_merge_720;

  if_merge_720:
  r54 = cn_var_符号指针_1;
  r55 = r54.类型信息;
  r56 = r55 != 0;
  if (r56) goto if_then_721; else goto if_merge_722;

  if_then_721:
  r57 = cn_var_符号指针_1;
  r58 = r57.类型信息;
  r59 = cn_var_上下文;
  r60 = 从类型节点推断(r58, r59);
  return r60;
  goto if_merge_722;

  if_merge_722:
  goto if_merge_712;
  return NULL;
}

struct 类型信息* 从指针类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r6, r8, r10, r11, r12, r14, r15, r16;
  struct 类型节点* r0;
  struct 类型节点* r1;
  struct 类型推断上下文* r2;
  struct 类型信息* r3;
  struct 类型信息* r7;
  struct 类型节点* r9;
  struct 类型信息* r13;

  entry:
  long long cn_var_指向类型_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->元素类型;
  r2 = cn_var_上下文;
  r3 = 从类型节点推断(r1, r2);
  cn_var_指向类型_0 = r3;
  r4 = cn_var_指向类型_0;
  r5 = r4 == 0;
  if (r5) goto if_then_723; else goto if_merge_724;

  if_then_723:
  return 0;
  goto if_merge_724;

  if_merge_724:
  long long cn_var_结果_1;
  r6 = cn_var_指向类型_0;
  r7 = 创建指针类型(r6);
  cn_var_结果_1 = r7;
  long long cn_var_i_2;
  cn_var_i_2 = 1;
  goto for_cond_725;

  for_cond_725:
  r8 = cn_var_i_2;
  r9 = cn_var_类型节点指针;
  r10 = r9->指针层级;
  r11 = r8 < r10;
  if (r11) goto for_body_726; else goto for_exit_728;

  for_body_726:
  r12 = cn_var_结果_1;
  r13 = 创建指针类型(r12);
  cn_var_结果_1 = r13;
  goto for_update_727;

  for_update_727:
  r14 = cn_var_i_2;
  r15 = r14 + 1;
  cn_var_i_2 = r15;
  goto for_cond_725;

  for_exit_728:
  r16 = cn_var_结果_1;
  return r16;
}

struct 类型信息* 从数组类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r7, r8, r9, r12, r14, r17;
  struct 类型节点* r0;
  struct 类型节点* r1;
  struct 类型推断上下文* r2;
  struct 类型信息* r3;
  struct 类型节点* r6;
  void* r10;
  struct 类型节点* r11;
  long long* r13;
  long long* r15;
  struct 类型节点* r16;
  struct 类型信息* r18;

  entry:
  long long cn_var_元素类型_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->元素类型;
  r2 = cn_var_上下文;
  r3 = 从类型节点推断(r1, r2);
  cn_var_元素类型_0 = r3;
  r4 = cn_var_元素类型_0;
  r5 = r4 == 0;
  if (r5) goto if_then_729; else goto if_merge_730;

  if_then_729:
  return 0;
  goto if_merge_730;

  if_merge_730:
  long long* cn_var_维度大小_1;
  r6 = cn_var_类型节点指针;
  r7 = r6->数组维度;
  r8 = cn_var_整数大小;
  r9 = r7 * r8;
  r10 = 分配内存(r9);
  cn_var_维度大小_1 = (long long*)0;
  r11 = cn_var_类型节点指针;
  r12 = r11->数组大小;
  r13 = cn_var_维度大小_1;
    { long long _tmp_r6 = r12; cn_rt_array_set_element(r13, 0, &_tmp_r6, 8); }
  r14 = cn_var_元素类型_0;
  r15 = cn_var_维度大小_1;
  r16 = cn_var_类型节点指针;
  r17 = r16->数组维度;
  r18 = 创建数组类型(r14, r15, r17);
  return r18;
}

struct 类型信息* 从结构体类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r5, r6, r7, r8, r9, r10, r13;
  char* r3;
  char* r12;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 符号* r4;
  struct 类型节点* r11;
  struct 类型信息* r14;

  entry:
  long long cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  r4 = 查找符号(r1, r3);
  cn_var_符号指针_0 = r4;
  r6 = cn_var_符号指针_0;
  r7 = r6 == 0;
  if (r7) goto logic_merge_734; else goto logic_rhs_733;

  if_then_731:
  return 0;
  goto if_merge_732;

  if_merge_732:
  r11 = cn_var_类型节点指针;
  r12 = r11->名称;
  r13 = cn_var_符号指针_0;
  r14 = 创建结构体类型(r12, r13);
  return r14;

  logic_rhs_733:
  r8 = cn_var_符号指针_0;
  r9 = r8.种类;
  r10 = r9 != 符号类型_结构体符号;
  goto logic_merge_734;

  logic_merge_734:
  if (r10) goto if_then_731; else goto if_merge_732;
  return NULL;
}

struct 类型信息* 从枚举类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r5, r6, r7, r8, r9, r10, r13;
  char* r3;
  char* r12;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 符号* r4;
  struct 类型节点* r11;
  struct 类型信息* r14;

  entry:
  long long cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  r4 = 查找符号(r1, r3);
  cn_var_符号指针_0 = r4;
  r6 = cn_var_符号指针_0;
  r7 = r6 == 0;
  if (r7) goto logic_merge_738; else goto logic_rhs_737;

  if_then_735:
  return 0;
  goto if_merge_736;

  if_merge_736:
  r11 = cn_var_类型节点指针;
  r12 = r11->名称;
  r13 = cn_var_符号指针_0;
  r14 = 创建枚举类型(r12, r13);
  return r14;

  logic_rhs_737:
  r8 = cn_var_符号指针_0;
  r9 = r8.种类;
  r10 = r9 != 符号类型_枚举符号;
  goto logic_merge_738;

  logic_merge_738:
  if (r10) goto if_then_735; else goto if_merge_736;
  return NULL;
}

struct 类型信息* 从类类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r5, r6, r7, r8, r9, r10, r13;
  char* r3;
  char* r12;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 符号* r4;
  struct 类型节点* r11;
  struct 类型信息* r14;

  entry:
  long long cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  r4 = 查找符号(r1, r3);
  cn_var_符号指针_0 = r4;
  r6 = cn_var_符号指针_0;
  r7 = r6 == 0;
  if (r7) goto logic_merge_742; else goto logic_rhs_741;

  if_then_739:
  return 0;
  goto if_merge_740;

  if_merge_740:
  r11 = cn_var_类型节点指针;
  r12 = r11->名称;
  r13 = cn_var_符号指针_0;
  r14 = 创建类类型(r12, r13);
  return r14;

  logic_rhs_741:
  r8 = cn_var_符号指针_0;
  r9 = r8.种类;
  r10 = r9 != 符号类型_类符号;
  goto logic_merge_742;

  logic_merge_742:
  if (r10) goto if_then_739; else goto if_merge_740;
  return NULL;
}

struct 类型信息* 从接口类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r5, r6, r7, r8, r9, r10, r13;
  char* r3;
  char* r12;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 符号* r4;
  struct 类型节点* r11;
  struct 类型信息* r14;

  entry:
  long long cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  r4 = 查找符号(r1, r3);
  cn_var_符号指针_0 = r4;
  r6 = cn_var_符号指针_0;
  r7 = r6 == 0;
  if (r7) goto logic_merge_746; else goto logic_rhs_745;

  if_then_743:
  return 0;
  goto if_merge_744;

  if_merge_744:
  r11 = cn_var_类型节点指针;
  r12 = r11->名称;
  r13 = cn_var_符号指针_0;
  r14 = 创建接口类型(r12, r13);
  return r14;

  logic_rhs_745:
  r8 = cn_var_符号指针_0;
  r9 = r8.种类;
  r10 = r9 != 符号类型_接口符号;
  goto logic_merge_746;

  logic_merge_746:
  if (r10) goto if_then_743; else goto if_merge_744;
  return NULL;
}

struct 类型信息* 从函数类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {

  entry:
  return 0;
}

struct 类型节点* 类型信息转类型节点(struct 类型信息* cn_var_类型信息指针) {
  long long r1, r5, r6, r9, r33, r38, r40;
  char* r29;
  char* r31;
  char* r45;
  char* r47;
  char* r49;
  char* r51;
  char* r53;
  struct 类型信息* r0;
  struct 类型信息* r2;
  void* r7;
  struct 类型节点* r8;
  void* r10;
  struct 类型信息* r11;
  struct 类型信息* r28;
  struct 类型信息* r30;
  struct 类型信息* r32;
  struct 类型信息* r34;
  struct 类型信息* r35;
  struct 类型节点* r36;
  struct 类型信息* r37;
  struct 类型信息* r39;
  struct 类型信息* r41;
  struct 类型信息* r42;
  struct 类型节点* r43;
  struct 类型信息* r44;
  struct 类型信息* r46;
  struct 类型信息* r48;
  struct 类型信息* r50;
  struct 类型信息* r52;
  struct 类型信息* r54;
  struct 类型节点* r56;
  _Bool r4;
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
  _Bool r55;
  enum 类型种类 r3;
  enum 类型种类 r12;

  entry:
  r0 = cn_var_类型信息指针;
  r1 = r0 == 0;
  if (r1) goto if_then_747; else goto if_merge_748;

  if_then_747:
  return 0;
  goto if_merge_748;

  if_merge_748:
  r2 = cn_var_类型信息指针;
  r3 = r2->种类;
  r4 = 是有效的类型种类(r3);
  r5 = r4 == 0;
  if (r5) goto if_then_749; else goto if_merge_750;

  if_then_749:
  struct 类型节点* cn_var_节点_0;
  r6 = sizeof(struct 类型节点);
  r7 = 分配清零内存(1, r6);
  cn_var_节点_0 = (struct 类型节点*)0;
  r8 = cn_var_节点_0;
  return r8;
  goto if_merge_750;

  if_merge_750:
  struct 类型节点* cn_var_节点_1;
  r9 = sizeof(struct 类型节点);
  r10 = 分配清零内存(1, r9);
  cn_var_节点_1 = (struct 类型节点*)0;
  r11 = cn_var_类型信息指针;
  r12 = r11->种类;
  r13 = r12 == 类型种类_类型_空;
  if (r13) goto case_body_752; else goto switch_check_768;

  switch_check_768:
  r14 = r12 == 类型种类_类型_整数;
  if (r14) goto case_body_753; else goto switch_check_769;

  switch_check_769:
  r15 = r12 == 类型种类_类型_小数;
  if (r15) goto case_body_754; else goto switch_check_770;

  switch_check_770:
  r16 = r12 == 类型种类_类型_布尔;
  if (r16) goto case_body_755; else goto switch_check_771;

  switch_check_771:
  r17 = r12 == 类型种类_类型_字符串;
  if (r17) goto case_body_756; else goto switch_check_772;

  switch_check_772:
  r18 = r12 == 类型种类_类型_字符;
  if (r18) goto case_body_757; else goto switch_check_773;

  switch_check_773:
  r19 = r12 == 类型种类_类型_指针;
  if (r19) goto case_body_758; else goto switch_check_774;

  switch_check_774:
  r20 = r12 == 类型种类_类型_数组;
  if (r20) goto case_body_759; else goto switch_check_775;

  switch_check_775:
  r21 = r12 == 类型种类_类型_结构体;
  if (r21) goto case_body_760; else goto switch_check_776;

  switch_check_776:
  r22 = r12 == 类型种类_类型_枚举;
  if (r22) goto case_body_761; else goto switch_check_777;

  switch_check_777:
  r23 = r12 == 类型种类_类型_类;
  if (r23) goto case_body_762; else goto switch_check_778;

  switch_check_778:
  r24 = r12 == 类型种类_类型_接口;
  if (r24) goto case_body_763; else goto switch_check_779;

  switch_check_779:
  r25 = r12 == 类型种类_类型_函数;
  if (r25) goto case_body_764; else goto switch_check_780;

  switch_check_780:
  r26 = r12 == 类型种类_类型_参数;
  if (r26) goto case_body_765; else goto switch_check_781;

  switch_check_781:
  r27 = r12 == 类型种类_类型_未定义;
  if (r27) goto case_body_766; else goto case_default_767;

  case_body_752:
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_753:
  r28 = cn_var_类型信息指针;
  r29 = r28->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_754:
  r30 = cn_var_类型信息指针;
  r31 = r30->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_755:
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_756:
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_757:
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_758:
  r32 = cn_var_类型信息指针;
  r33 = r32->指针层级;
  r34 = cn_var_类型信息指针;
  r35 = r34->指向类型;
  r36 = 类型信息转类型节点(r35);
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_759:
  r37 = cn_var_类型信息指针;
  r38 = r37->数组维度;
  r39 = cn_var_类型信息指针;
  r40 = r39->总元素数;
  r41 = cn_var_类型信息指针;
  r42 = r41->元素类型;
  r43 = 类型信息转类型节点(r42);
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_760:
  r44 = cn_var_类型信息指针;
  r45 = r44->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_761:
  r46 = cn_var_类型信息指针;
  r47 = r46->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_762:
  r48 = cn_var_类型信息指针;
  r49 = r48->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_763:
  r50 = cn_var_类型信息指针;
  r51 = r50->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_764:
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_765:
  r52 = cn_var_类型信息指针;
  r53 = r52->名称;
  goto switch_merge_751;
  goto switch_merge_751;

  case_body_766:
  goto switch_merge_751;
  goto switch_merge_751;

  case_default_767:
  goto switch_merge_751;
  goto switch_merge_751;

  switch_merge_751:
  r54 = cn_var_类型信息指针;
  r55 = r54->是常量;
  r56 = cn_var_节点_1;
  return r56;
}

_Bool 是有效的类型种类(enum 类型种类 cn_var_种类) {
  long long r0, r2, r4;
  enum 类型种类 r1;
  enum 类型种类 r3;

  entry:
  r1 = cn_var_种类;
  r2 = r1 < 类型种类_类型_空;
  if (r2) goto logic_merge_785; else goto logic_rhs_784;

  if_then_782:
  return 0;
  goto if_merge_783;

  if_merge_783:
  return 1;

  logic_rhs_784:
  r3 = cn_var_种类;
  r4 = r3 > 类型种类_类型_未定义;
  goto logic_merge_785;

  logic_merge_785:
  if (r4) goto if_then_782; else goto if_merge_783;
  return 0;
}

char* 获取类型种类名称(enum 类型种类 cn_var_种类) {
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
  enum 类型种类 r0;

  entry:
  r0 = cn_var_种类;
  r1 = r0 == 类型种类_类型_空;
  if (r1) goto case_body_787; else goto switch_check_803;

  switch_check_803:
  r2 = r0 == 类型种类_类型_整数;
  if (r2) goto case_body_788; else goto switch_check_804;

  switch_check_804:
  r3 = r0 == 类型种类_类型_小数;
  if (r3) goto case_body_789; else goto switch_check_805;

  switch_check_805:
  r4 = r0 == 类型种类_类型_布尔;
  if (r4) goto case_body_790; else goto switch_check_806;

  switch_check_806:
  r5 = r0 == 类型种类_类型_字符串;
  if (r5) goto case_body_791; else goto switch_check_807;

  switch_check_807:
  r6 = r0 == 类型种类_类型_字符;
  if (r6) goto case_body_792; else goto switch_check_808;

  switch_check_808:
  r7 = r0 == 类型种类_类型_指针;
  if (r7) goto case_body_793; else goto switch_check_809;

  switch_check_809:
  r8 = r0 == 类型种类_类型_数组;
  if (r8) goto case_body_794; else goto switch_check_810;

  switch_check_810:
  r9 = r0 == 类型种类_类型_结构体;
  if (r9) goto case_body_795; else goto switch_check_811;

  switch_check_811:
  r10 = r0 == 类型种类_类型_枚举;
  if (r10) goto case_body_796; else goto switch_check_812;

  switch_check_812:
  r11 = r0 == 类型种类_类型_类;
  if (r11) goto case_body_797; else goto switch_check_813;

  switch_check_813:
  r12 = r0 == 类型种类_类型_接口;
  if (r12) goto case_body_798; else goto switch_check_814;

  switch_check_814:
  r13 = r0 == 类型种类_类型_函数;
  if (r13) goto case_body_799; else goto switch_check_815;

  switch_check_815:
  r14 = r0 == 类型种类_类型_参数;
  if (r14) goto case_body_800; else goto switch_check_816;

  switch_check_816:
  r15 = r0 == 类型种类_类型_未定义;
  if (r15) goto case_body_801; else goto case_default_802;

  case_body_787:
  return "空类型";
  goto switch_merge_786;

  case_body_788:
  return "整数";
  goto switch_merge_786;

  case_body_789:
  return "小数";
  goto switch_merge_786;

  case_body_790:
  return "布尔";
  goto switch_merge_786;

  case_body_791:
  return "字符串";
  goto switch_merge_786;

  case_body_792:
  return "字符";
  goto switch_merge_786;

  case_body_793:
  return "指针";
  goto switch_merge_786;

  case_body_794:
  return "数组";
  goto switch_merge_786;

  case_body_795:
  return "结构体";
  goto switch_merge_786;

  case_body_796:
  return "枚举";
  goto switch_merge_786;

  case_body_797:
  return "类";
  goto switch_merge_786;

  case_body_798:
  return "接口";
  goto switch_merge_786;

  case_body_799:
  return "函数";
  goto switch_merge_786;

  case_body_800:
  return "类型参数";
  goto switch_merge_786;

  case_body_801:
  return "未定义";
  goto switch_merge_786;

  case_default_802:
  return "未知";
  goto switch_merge_786;

  switch_merge_786:
  return NULL;
}

