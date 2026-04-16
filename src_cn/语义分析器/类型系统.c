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

// CN Language Enum Definitions
enum 类型种类 {
    类型种类_类型_空,
    类型种类_类型_整数,
    类型种类_类型_小数,
    类型种类_类型_字符,
    类型种类_类型_布尔,
    类型种类_类型_字符串,
    类型种类_类型_指针,
    类型种类_类型_数组,
    类型种类_类型_结构体,
    类型种类_类型_枚举,
    类型种类_类型_类,
    类型种类_类型_接口,
    类型种类_类型_函数,
    类型种类_类型_参数,
    类型种类_类型_内存地址,
    类型种类_类型_32位整数,
    类型种类_类型_64位整数,
    类型种类_类型_32位无符号,
    类型种类_类型_64位无符号,
    类型种类_类型_64位无符号LL,
    类型种类_类型_单精度小数,
    类型种类_类型_双精度小数,
    类型种类_类型_自身,
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
  long long r0;
  void* r1;
  struct 类型信息* r2;
  struct 类型信息* r3;

  entry:
  void* cn_var_内存指针_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_内存指针_0 = r1;
  struct 类型信息* cn_var_类型_1;
  r2 = cn_var_内存指针_0;
  cn_var_类型_1 = r2;
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
  r2 = cn_var_字符串大小;
  r3 = cn_var_指针大小;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建指针类型(struct 类型信息* cn_var_指向类型) {
  long long r0, r3, r4, r5, r6;
  char* r7;
  char* r8;
  void* r1;
  struct 类型信息* r2;
  struct 类型信息* r9;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
  r2 = cn_var_指向类型;
  r3 = cn_var_指向类型->指针层级;
  r4 = r3 + 1;
  r5 = cn_var_指针大小;
  r6 = cn_var_指针大小;
  r7 = cn_var_指向类型->名称;
  r8 = cn_rt_string_concat(r7, "*");
  r9 = cn_var_类型_0;
  return r9;
}

struct 类型信息* 创建数组类型(struct 类型信息* cn_var_元素类型, long long* cn_var_维度大小, long long cn_var_维度) {
  long long r0, r3, r5, r6, r8, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
  void* r1;
  struct 类型信息* r2;
  long long* r4;
  long long* r9;
  struct 类型信息* r20;
  _Bool r7;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  r1 = 分配清零内存(1, r0);
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
  r2 = cn_var_元素类型;
  r3 = cn_var_维度;
  r4 = cn_var_维度大小;
  long long cn_var_总数_1;
  cn_var_总数_1 = 1;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_0;

  for_cond_0:
  r5 = cn_var_i_2;
  r6 = cn_var_维度;
  r7 = r5 < r6;
  if (r7) goto for_body_1; else goto for_exit_3;

  for_body_1:
  r8 = cn_var_总数_1;
  r9 = cn_var_维度大小;
  r10 = cn_var_i_2;
  r11 = *(long long)cn_rt_array_get_element(r9, r10, 8);
  r12 = r8 * r11;
  cn_var_总数_1 = r12;
  goto for_update_2;

  for_update_2:
  r13 = cn_var_i_2;
  r14 = r13 + 1;
  cn_var_i_2 = r14;
  goto for_cond_0;

  for_exit_3:
  r15 = cn_var_总数_1;
  r16 = cn_var_总数_1;
  r17 = cn_var_元素类型->大小;
  r18 = r16 * r17;
  r19 = cn_var_元素类型->对齐;
  r20 = cn_var_类型_0;
  return r20;
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
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
  cn_var_类型_0 = (struct 类型信息*)分配清零内存(1, 类型大小(cn_var_类型信息));
  r2 = cn_var_名称;
  r3 = cn_var_类型_0;
  return r3;
}

_Bool 类型相同(struct 类型信息* cn_var_类型1, struct 类型信息* cn_var_类型2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r62, r63, r64;

  entry:
  goto tail_rec_loop;
  return 0;
}

enum 类型兼容性 检查类型兼容性(struct 类型信息* cn_var_源类型, struct 类型信息* cn_var_目标类型) {
  long long r3, r11, r16, r21, r26, r31, r36, r44;
  struct 类型信息* r0;
  struct 类型信息* r1;
  struct 类型信息* r8;
  struct 类型信息* r41;
  struct 类型信息* r42;
  struct 类型信息* r49;
  struct 类型信息* r50;
  _Bool r2;
  _Bool r5;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r15;
  _Bool r18;
  _Bool r20;
  _Bool r23;
  _Bool r25;
  _Bool r28;
  _Bool r30;
  _Bool r33;
  _Bool r35;
  _Bool r38;
  _Bool r40;
  _Bool r43;
  _Bool r46;
  _Bool r48;
  _Bool r51;
  enum 类型种类 r4;
  enum 类型种类 r6;
  enum 类型种类 r9;
  enum 类型种类 r12;
  enum 类型种类 r14;
  enum 类型种类 r17;
  enum 类型种类 r19;
  enum 类型种类 r22;
  enum 类型种类 r24;
  enum 类型种类 r27;
  enum 类型种类 r29;
  enum 类型种类 r32;
  enum 类型种类 r34;
  enum 类型种类 r37;
  enum 类型种类 r39;
  enum 类型种类 r45;
  enum 类型种类 r47;

  entry:
  r0 = cn_var_源类型;
  r1 = cn_var_目标类型;
  r2 = 类型相同(r0, r1);
  if (r2) goto if_then_49; else goto if_merge_50;

  if_then_49:
  return 类型兼容性_完全兼容;
  goto if_merge_50;

  if_merge_50:
  r4 = cn_var_源类型->种类;
  r5 = r4 == 类型种类_类型_指针;
  if (r5) goto logic_rhs_53; else goto logic_merge_54;

  if_then_51:
  r8 = cn_var_源类型->指向类型;
  r9 = r8->种类;
  r10 = r9 == 类型种类_类型_空;
  if (r10) goto if_then_55; else goto if_merge_56;

  if_merge_52:
  r12 = cn_var_源类型->种类;
  r13 = r12 == 类型种类_类型_整数;
  if (r13) goto logic_rhs_59; else goto logic_merge_60;

  logic_rhs_53:
  r6 = cn_var_目标类型->种类;
  r7 = r6 == 类型种类_类型_指针;
  goto logic_merge_54;

  logic_merge_54:
  if (r7) goto if_then_51; else goto if_merge_52;

  if_then_55:
  return 类型兼容性_隐式转换;
  goto if_merge_56;

  if_merge_56:
  goto if_merge_52;

  if_then_57:
  return 类型兼容性_隐式转换;
  goto if_merge_58;

  if_merge_58:
  r17 = cn_var_源类型->种类;
  r18 = r17 == 类型种类_类型_整数;
  if (r18) goto logic_rhs_63; else goto logic_merge_64;

  logic_rhs_59:
  r14 = cn_var_目标类型->种类;
  r15 = r14 == 类型种类_类型_小数;
  goto logic_merge_60;

  logic_merge_60:
  if (r15) goto if_then_57; else goto if_merge_58;

  if_then_61:
  return 类型兼容性_隐式转换;
  goto if_merge_62;

  if_merge_62:
  r22 = cn_var_源类型->种类;
  r23 = r22 == 类型种类_类型_枚举;
  if (r23) goto logic_rhs_67; else goto logic_merge_68;

  logic_rhs_63:
  r19 = cn_var_目标类型->种类;
  r20 = r19 == 类型种类_类型_整数;
  goto logic_merge_64;

  logic_merge_64:
  if (r20) goto if_then_61; else goto if_merge_62;

  if_then_65:
  return 类型兼容性_隐式转换;
  goto if_merge_66;

  if_merge_66:
  r27 = cn_var_源类型->种类;
  r28 = r27 == 类型种类_类型_整数;
  if (r28) goto logic_rhs_71; else goto logic_merge_72;

  logic_rhs_67:
  r24 = cn_var_目标类型->种类;
  r25 = r24 == 类型种类_类型_整数;
  goto logic_merge_68;

  logic_merge_68:
  if (r25) goto if_then_65; else goto if_merge_66;

  if_then_69:
  return 类型兼容性_显式转换;
  goto if_merge_70;

  if_merge_70:
  r32 = cn_var_源类型->种类;
  r33 = r32 == 类型种类_类型_小数;
  if (r33) goto logic_rhs_75; else goto logic_merge_76;

  logic_rhs_71:
  r29 = cn_var_目标类型->种类;
  r30 = r29 == 类型种类_类型_枚举;
  goto logic_merge_72;

  logic_merge_72:
  if (r30) goto if_then_69; else goto if_merge_70;

  if_then_73:
  return 类型兼容性_显式转换;
  goto if_merge_74;

  if_merge_74:
  r37 = cn_var_源类型->种类;
  r38 = r37 == 类型种类_类型_类;
  if (r38) goto logic_rhs_79; else goto logic_merge_80;

  logic_rhs_75:
  r34 = cn_var_目标类型->种类;
  r35 = r34 == 类型种类_类型_整数;
  goto logic_merge_76;

  logic_merge_76:
  if (r35) goto if_then_73; else goto if_merge_74;

  if_then_77:
  r41 = cn_var_源类型;
  r42 = cn_var_目标类型;
  r43 = 是派生类(r41, r42);
  if (r43) goto if_then_81; else goto if_merge_82;

  if_merge_78:
  r45 = cn_var_源类型->种类;
  r46 = r45 == 类型种类_类型_类;
  if (r46) goto logic_rhs_85; else goto logic_merge_86;

  logic_rhs_79:
  r39 = cn_var_目标类型->种类;
  r40 = r39 == 类型种类_类型_类;
  goto logic_merge_80;

  logic_merge_80:
  if (r40) goto if_then_77; else goto if_merge_78;

  if_then_81:
  return 类型兼容性_隐式转换;
  goto if_merge_82;

  if_merge_82:
  goto if_merge_78;

  if_then_83:
  r49 = cn_var_源类型;
  r50 = cn_var_目标类型;
  r51 = 实现接口(r49, r50);
  if (r51) goto if_then_87; else goto if_merge_88;

  if_merge_84:
  return 类型兼容性_不兼容;

  logic_rhs_85:
  r47 = cn_var_目标类型->种类;
  r48 = r47 == 类型种类_类型_接口;
  goto logic_merge_86;

  logic_merge_86:
  if (r48) goto if_then_83; else goto if_merge_84;

  if_then_87:
  return 类型兼容性_隐式转换;
  goto if_merge_88;

  if_merge_88:
  goto if_merge_84;
  return 0;
}

_Bool 是派生类(struct 类型信息* cn_var_子类, struct 类型信息* cn_var_父类) {
  long long r0, r5;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 符号* r10;
  struct 符号* r11;
  struct 符号* r13;
  struct 符号* r14;
  struct 符号* r16;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  enum 类型种类 r6;
  enum 类型种类 r8;

  entry:
  r1 = cn_var_子类;
  r2 = r1 == 0;
  if (r2) goto logic_merge_92; else goto logic_rhs_91;

  if_then_89:
  return 0;
  goto if_merge_90;

  if_merge_90:
  r6 = cn_var_子类->种类;
  r7 = r6 != 类型种类_类型_类;
  if (r7) goto logic_merge_96; else goto logic_rhs_95;

  logic_rhs_91:
  r3 = cn_var_父类;
  r4 = r3 == 0;
  goto logic_merge_92;

  logic_merge_92:
  if (r4) goto if_then_89; else goto if_merge_90;

  if_then_93:
  return 0;
  goto if_merge_94;

  if_merge_94:
  struct 符号* cn_var_当前_0;
  r10 = cn_var_子类->类型符号;
  cn_var_当前_0 = r10;
  goto while_cond_97;

  logic_rhs_95:
  r8 = cn_var_父类->种类;
  r9 = r8 != 类型种类_类型_类;
  goto logic_merge_96;

  logic_merge_96:
  if (r9) goto if_then_93; else goto if_merge_94;

  while_cond_97:
  r11 = cn_var_当前_0;
  r12 = r11 != 0;
  if (r12) goto while_body_98; else goto while_exit_99;

  while_body_98:
  r13 = cn_var_当前_0;
  r14 = cn_var_父类->类型符号;
  r15 = r13 == r14;
  if (r15) goto if_then_100; else goto if_merge_101;

  while_exit_99:
  return 0;

  if_then_100:
  return 1;
  goto if_merge_101;

  if_merge_101:
  r16 = cn_var_当前_0->父类符号;
  cn_var_当前_0 = r16;
  goto while_cond_97;
  return 0;
}

_Bool 实现接口(struct 类型信息* cn_var_类类型, struct 类型信息* cn_var_接口类型) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33;

  entry:
  goto tail_rec_loop;
  return 0;
}

_Bool 是数值类型(struct 类型信息* cn_var_类型) {
  long long r2;
  struct 类型信息* r0;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  enum 类型种类 r3;
  enum 类型种类 r5;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_120; else goto if_merge_121;

  if_then_120:
  return 0;
  goto if_merge_121;

  if_merge_121:
  r3 = cn_var_类型->种类;
  r4 = r3 == 类型种类_类型_整数;
  if (r4) goto logic_merge_123; else goto logic_rhs_122;

  logic_rhs_122:
  r5 = cn_var_类型->种类;
  r6 = r5 == 类型种类_类型_小数;
  goto logic_merge_123;

  logic_merge_123:
  return r6;
}

_Bool 是整数类型(struct 类型信息* cn_var_类型) {
  long long r2;
  struct 类型信息* r0;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  enum 类型种类 r3;
  enum 类型种类 r5;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_124; else goto if_merge_125;

  if_then_124:
  return 0;
  goto if_merge_125;

  if_merge_125:
  r3 = cn_var_类型->种类;
  r4 = r3 == 类型种类_类型_整数;
  if (r4) goto logic_merge_127; else goto logic_rhs_126;

  logic_rhs_126:
  r5 = cn_var_类型->种类;
  r6 = r5 == 类型种类_类型_枚举;
  goto logic_merge_127;

  logic_merge_127:
  return r6;
}

_Bool 是指针类型(struct 类型信息* cn_var_类型) {
  struct 类型信息* r0;
  _Bool r1;
  _Bool r3;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_128; else goto if_merge_129;

  if_then_128:
  return 0;
  goto if_merge_129;

  if_merge_129:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_指针;
  return r3;
}

_Bool 是数组类型(struct 类型信息* cn_var_类型) {
  struct 类型信息* r0;
  _Bool r1;
  _Bool r3;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_130; else goto if_merge_131;

  if_then_130:
  return 0;
  goto if_merge_131;

  if_merge_131:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_数组;
  return r3;
}

_Bool 是结构体类型(struct 类型信息* cn_var_类型) {
  struct 类型信息* r0;
  _Bool r1;
  _Bool r3;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_132; else goto if_merge_133;

  if_then_132:
  return 0;
  goto if_merge_133;

  if_merge_133:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_结构体;
  return r3;
}

_Bool 是类类型(struct 类型信息* cn_var_类型) {
  struct 类型信息* r0;
  _Bool r1;
  _Bool r3;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_134; else goto if_merge_135;

  if_then_134:
  return 0;
  goto if_merge_135;

  if_merge_135:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_类;
  return r3;
}

_Bool 是函数类型(struct 类型信息* cn_var_类型) {
  struct 类型信息* r0;
  _Bool r1;
  _Bool r3;
  enum 类型种类 r2;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_136; else goto if_merge_137;

  if_then_136:
  return 0;
  goto if_merge_137;

  if_merge_137:
  r2 = cn_var_类型->种类;
  r3 = r2 == 类型种类_类型_函数;
  return r3;
}

_Bool 是可调用类型(struct 类型信息* cn_var_类型) {
  long long r2, r5;
  struct 类型信息* r0;
  struct 类型信息* r8;
  _Bool r1;
  _Bool r4;
  _Bool r7;
  _Bool r10;
  enum 类型种类 r3;
  enum 类型种类 r6;
  enum 类型种类 r9;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_138; else goto if_merge_139;

  if_then_138:
  return 0;
  goto if_merge_139;

  if_merge_139:
  r3 = cn_var_类型->种类;
  r4 = r3 == 类型种类_类型_函数;
  if (r4) goto logic_merge_141; else goto logic_rhs_140;

  logic_rhs_140:
  r6 = cn_var_类型->种类;
  r7 = r6 == 类型种类_类型_指针;
  if (r7) goto logic_rhs_142; else goto logic_merge_143;

  logic_merge_141:
  return r10;

  logic_rhs_142:
  r8 = cn_var_类型->指向类型;
  r9 = r8->种类;
  r10 = r9 == 类型种类_类型_函数;
  goto logic_merge_143;

  logic_merge_143:
  goto logic_merge_141;
  return 0;
}

struct 类型信息* 从类型节点推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  struct 类型节点* r0;
  struct 类型节点* r12;
  struct 类型推断上下文* r13;
  struct 类型信息* r14;
  struct 类型节点* r15;
  struct 类型推断上下文* r16;
  struct 类型信息* r17;
  struct 类型节点* r18;
  struct 类型推断上下文* r19;
  struct 类型信息* r20;
  struct 类型节点* r21;
  struct 类型推断上下文* r22;
  struct 类型信息* r23;
  struct 类型节点* r24;
  struct 类型推断上下文* r25;
  struct 类型信息* r26;
  struct 类型节点* r27;
  struct 类型推断上下文* r28;
  struct 类型信息* r29;
  struct 类型节点* r30;
  struct 类型推断上下文* r31;
  struct 类型信息* r32;
  struct 类型节点* r33;
  struct 类型推断上下文* r34;
  struct 类型信息* r35;
  struct 类型节点* r36;
  struct 类型推断上下文* r37;
  struct 类型信息* r38;
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
  enum 节点类型 r2;

  entry:
  r0 = cn_var_类型节点指针;
  r1 = r0 == 0;
  if (r1) goto if_then_144; else goto if_merge_145;

  if_then_144:
  return 0;
  goto if_merge_145;

  if_merge_145:
  r2 = cn_var_类型节点指针->类型;
  r3 = r2 == 节点类型_基础类型;
  if (r3) goto case_body_147; else goto switch_check_157;

  switch_check_157:
  r4 = r2 == 节点类型_指针类型;
  if (r4) goto case_body_148; else goto switch_check_158;

  switch_check_158:
  r5 = r2 == 节点类型_数组类型;
  if (r5) goto case_body_149; else goto switch_check_159;

  switch_check_159:
  r6 = r2 == 节点类型_结构体类型;
  if (r6) goto case_body_150; else goto switch_check_160;

  switch_check_160:
  r7 = r2 == 节点类型_枚举类型;
  if (r7) goto case_body_151; else goto switch_check_161;

  switch_check_161:
  r8 = r2 == 节点类型_类类型;
  if (r8) goto case_body_152; else goto switch_check_162;

  switch_check_162:
  r9 = r2 == 节点类型_接口类型;
  if (r9) goto case_body_153; else goto switch_check_163;

  switch_check_163:
  r10 = r2 == 节点类型_函数类型;
  if (r10) goto case_body_154; else goto switch_check_164;

  switch_check_164:
  r11 = r2 == 节点类型_标识符类型;
  if (r11) goto case_body_155; else goto case_default_156;

  case_body_147:
  r12 = cn_var_类型节点指针;
  r13 = cn_var_上下文;
  r14 = 从基础类型推断(r12, r13);
  return r14;
  goto switch_merge_146;

  case_body_148:
  r15 = cn_var_类型节点指针;
  r16 = cn_var_上下文;
  r17 = 从指针类型推断(r15, r16);
  return r17;
  goto switch_merge_146;

  case_body_149:
  r18 = cn_var_类型节点指针;
  r19 = cn_var_上下文;
  r20 = 从数组类型推断(r18, r19);
  return r20;
  goto switch_merge_146;

  case_body_150:
  r21 = cn_var_类型节点指针;
  r22 = cn_var_上下文;
  r23 = 从结构体类型推断(r21, r22);
  return r23;
  goto switch_merge_146;

  case_body_151:
  r24 = cn_var_类型节点指针;
  r25 = cn_var_上下文;
  r26 = 从枚举类型推断(r24, r25);
  return r26;
  goto switch_merge_146;

  case_body_152:
  r27 = cn_var_类型节点指针;
  r28 = cn_var_上下文;
  r29 = 从类类型推断(r27, r28);
  return r29;
  goto switch_merge_146;

  case_body_153:
  r30 = cn_var_类型节点指针;
  r31 = cn_var_上下文;
  r32 = 从接口类型推断(r30, r31);
  return r32;
  goto switch_merge_146;

  case_body_154:
  r33 = cn_var_类型节点指针;
  r34 = cn_var_上下文;
  r35 = 从函数类型推断(r33, r34);
  return r35;
  goto switch_merge_146;

  case_body_155:
  r36 = cn_var_类型节点指针;
  r37 = cn_var_上下文;
  r38 = 从基础类型推断(r36, r37);
  return r38;
  goto switch_merge_146;

  case_default_156:
  return 0;
  goto switch_merge_146;

  switch_merge_146:
  return NULL;
}

struct 类型信息* 从基础类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r2, r4, r7, r9, r12, r16, r20;
  char* r0;
  char* r1;
  char* r6;
  char* r11;
  char* r15;
  char* r19;
  char* r24;
  char* r30;
  char* r35;
  char* r40;
  char* r45;
  char* r53;
  struct 类型信息* r5;
  struct 类型信息* r10;
  struct 类型信息* r14;
  struct 类型信息* r18;
  struct 类型信息* r22;
  struct 符号表管理器* r23;
  struct 符号* r25;
  struct 符号* r26;
  struct 符号* r31;
  struct 类型信息* r32;
  struct 符号* r36;
  struct 类型信息* r37;
  struct 符号* r41;
  struct 类型信息* r42;
  struct 符号* r46;
  struct 类型信息* r47;
  struct 类型节点* r48;
  struct 类型节点* r50;
  struct 类型推断上下文* r51;
  struct 类型信息* r52;
  struct 类型信息* r54;
  _Bool r3;
  _Bool r8;
  _Bool r13;
  _Bool r17;
  _Bool r21;
  _Bool r27;
  _Bool r29;
  _Bool r34;
  _Bool r39;
  _Bool r44;
  _Bool r49;
  enum 符号类型 r28;
  enum 符号类型 r33;
  enum 符号类型 r38;
  enum 符号类型 r43;

  entry:
  char* cn_var_名称_0;
  r0 = cn_var_类型节点指针->名称;
  cn_var_名称_0 = r0;
  r1 = cn_var_名称_0;
  r2 = 比较字符串(r1, "整数");
  r3 = r2 == 0;
  if (r3) goto if_then_165; else goto if_merge_166;

  if_then_165:
  r4 = cn_var_整数大小;
  r5 = 创建整数类型("整数", r4, 1);
  return r5;
  goto if_merge_166;

  if_merge_166:
  r6 = cn_var_名称_0;
  r7 = 比较字符串(r6, "小数");
  r8 = r7 == 0;
  if (r8) goto if_then_167; else goto if_merge_168;

  if_then_167:
  r9 = cn_var_小数大小;
  r10 = 创建小数类型("小数", r9);
  return r10;
  goto if_merge_168;

  if_merge_168:
  r11 = cn_var_名称_0;
  r12 = 比较字符串(r11, "布尔");
  r13 = r12 == 0;
  if (r13) goto if_then_169; else goto if_merge_170;

  if_then_169:
  r14 = 创建布尔类型();
  return r14;
  goto if_merge_170;

  if_merge_170:
  r15 = cn_var_名称_0;
  r16 = 比较字符串(r15, "字符串");
  r17 = r16 == 0;
  if (r17) goto if_then_171; else goto if_merge_172;

  if_then_171:
  r18 = 创建字符串类型();
  return r18;
  goto if_merge_172;

  if_merge_172:
  r19 = cn_var_名称_0;
  r20 = 比较字符串(r19, "空类型");
  r21 = r20 == 0;
  if (r21) goto if_then_173; else goto if_merge_174;

  if_then_173:
  r22 = 创建空类型();
  return r22;
  goto if_merge_174;

  if_merge_174:
  struct 符号* cn_var_符号指针_1;
  r23 = cn_var_上下文->符号表;
  r24 = cn_var_名称_0;
  r25 = 查找符号(r23, r24);
  cn_var_符号指针_1 = r25;
  r26 = cn_var_符号指针_1;
  r27 = r26 != 0;
  if (r27) goto if_then_175; else goto if_merge_176;

  if_then_175:
  r28 = cn_var_符号指针_1->种类;
  r29 = r28 == 符号类型_结构体符号;
  if (r29) goto if_then_177; else goto if_merge_178;

  if_merge_176:
  r53 = cn_var_名称_0;
  r54 = 创建未定义类型(r53);
  return r54;

  if_then_177:
  r30 = cn_var_名称_0;
  r31 = cn_var_符号指针_1;
  r32 = 创建结构体类型(r30, r31);
  return r32;
  goto if_merge_178;

  if_merge_178:
  r33 = cn_var_符号指针_1->种类;
  r34 = r33 == 符号类型_枚举符号;
  if (r34) goto if_then_179; else goto if_merge_180;

  if_then_179:
  r35 = cn_var_名称_0;
  r36 = cn_var_符号指针_1;
  r37 = 创建枚举类型(r35, r36);
  return r37;
  goto if_merge_180;

  if_merge_180:
  r38 = cn_var_符号指针_1->种类;
  r39 = r38 == 符号类型_类符号;
  if (r39) goto if_then_181; else goto if_merge_182;

  if_then_181:
  r40 = cn_var_名称_0;
  r41 = cn_var_符号指针_1;
  r42 = 创建类类型(r40, r41);
  return r42;
  goto if_merge_182;

  if_merge_182:
  r43 = cn_var_符号指针_1->种类;
  r44 = r43 == 符号类型_接口符号;
  if (r44) goto if_then_183; else goto if_merge_184;

  if_then_183:
  r45 = cn_var_名称_0;
  r46 = cn_var_符号指针_1;
  r47 = 创建接口类型(r45, r46);
  return r47;
  goto if_merge_184;

  if_merge_184:
  r48 = cn_var_符号指针_1->类型信息;
  r49 = r48 != 0;
  if (r49) goto if_then_185; else goto if_merge_186;

  if_then_185:
  r50 = cn_var_符号指针_1->类型信息;
  r51 = cn_var_上下文;
  r52 = 从类型节点推断(r50, r51);
  return r52;
  goto if_merge_186;

  if_merge_186:
  goto if_merge_176;
  return NULL;
}

struct 类型信息* 从指针类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r7, r8, r12, r13;
  struct 类型节点* r0;
  struct 类型推断上下文* r1;
  struct 类型信息* r2;
  struct 类型信息* r3;
  struct 类型信息* r5;
  struct 类型信息* r6;
  struct 类型信息* r10;
  struct 类型信息* r11;
  struct 类型信息* r14;
  _Bool r4;
  _Bool r9;

  entry:
  struct 类型信息* cn_var_指向类型_0;
  r0 = cn_var_类型节点指针->元素类型;
  r1 = cn_var_上下文;
  r2 = 从类型节点推断(r0, r1);
  cn_var_指向类型_0 = r2;
  r3 = cn_var_指向类型_0;
  r4 = r3 == 0;
  if (r4) goto if_then_187; else goto if_merge_188;

  if_then_187:
  return 0;
  goto if_merge_188;

  if_merge_188:
  struct 类型信息* cn_var_结果_1;
  r5 = cn_var_指向类型_0;
  r6 = 创建指针类型(r5);
  cn_var_结果_1 = r6;
  long long cn_var_i_2;
  cn_var_i_2 = 1;
  goto for_cond_189;

  for_cond_189:
  r7 = cn_var_i_2;
  r8 = cn_var_类型节点指针->指针层级;
  r9 = r7 < r8;
  if (r9) goto for_body_190; else goto for_exit_192;

  for_body_190:
  r10 = cn_var_结果_1;
  r11 = 创建指针类型(r10);
  cn_var_结果_1 = r11;
  goto for_update_191;

  for_update_191:
  r12 = cn_var_i_2;
  r13 = r12 + 1;
  cn_var_i_2 = r13;
  goto for_cond_189;

  for_exit_192:
  r14 = cn_var_结果_1;
  return r14;
}

struct 类型信息* 从数组类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r5, r6, r7, r9, r14;
  struct 类型节点* r0;
  struct 类型推断上下文* r1;
  struct 类型信息* r2;
  struct 类型信息* r3;
  void* r8;
  long long* r10;
  long long* r11;
  struct 类型信息* r12;
  long long* r13;
  struct 类型信息* r15;
  _Bool r4;

  entry:
  struct 类型信息* cn_var_元素类型_0;
  r0 = cn_var_类型节点指针->元素类型;
  r1 = cn_var_上下文;
  r2 = 从类型节点推断(r0, r1);
  cn_var_元素类型_0 = r2;
  r3 = cn_var_元素类型_0;
  r4 = r3 == 0;
  if (r4) goto if_then_193; else goto if_merge_194;

  if_then_193:
  return 0;
  goto if_merge_194;

  if_merge_194:
  long long* cn_var_维度大小_1;
  r5 = cn_var_类型节点指针->数组维度;
  r6 = cn_var_整数大小;
  r7 = r5 * r6;
  r8 = 分配内存(r7);
  cn_var_维度大小_1 = (long long*)分配内存((cn_var_类型节点指针->数组维度 * cn_var_整数大小));
  r9 = cn_var_类型节点指针->数组大小;
  r10 = cn_var_维度大小_1;
  r11 = &r10[0];
  *r11 = r9;
  r12 = cn_var_元素类型_0;
  r13 = cn_var_维度大小_1;
  r14 = cn_var_类型节点指针->数组维度;
  r15 = 创建数组类型(r12, r13, r14);
  return r15;
}

struct 类型信息* 从结构体类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3;
  char* r1;
  char* r8;
  struct 符号表管理器* r0;
  struct 符号* r2;
  struct 符号* r4;
  struct 符号* r9;
  struct 类型信息* r10;
  _Bool r5;
  _Bool r7;
  enum 符号类型 r6;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_上下文->符号表;
  r1 = cn_var_类型节点指针->名称;
  r2 = 查找符号(r0, r1);
  cn_var_符号指针_0 = r2;
  r4 = cn_var_符号指针_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_198; else goto logic_rhs_197;

  if_then_195:
  return 0;
  goto if_merge_196;

  if_merge_196:
  r8 = cn_var_类型节点指针->名称;
  r9 = cn_var_符号指针_0;
  r10 = 创建结构体类型(r8, r9);
  return r10;

  logic_rhs_197:
  r6 = cn_var_符号指针_0->种类;
  r7 = r6 != 符号类型_结构体符号;
  goto logic_merge_198;

  logic_merge_198:
  if (r7) goto if_then_195; else goto if_merge_196;
  return NULL;
}

struct 类型信息* 从枚举类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3;
  char* r1;
  char* r8;
  struct 符号表管理器* r0;
  struct 符号* r2;
  struct 符号* r4;
  struct 符号* r9;
  struct 类型信息* r10;
  _Bool r5;
  _Bool r7;
  enum 符号类型 r6;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_上下文->符号表;
  r1 = cn_var_类型节点指针->名称;
  r2 = 查找符号(r0, r1);
  cn_var_符号指针_0 = r2;
  r4 = cn_var_符号指针_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_202; else goto logic_rhs_201;

  if_then_199:
  return 0;
  goto if_merge_200;

  if_merge_200:
  r8 = cn_var_类型节点指针->名称;
  r9 = cn_var_符号指针_0;
  r10 = 创建枚举类型(r8, r9);
  return r10;

  logic_rhs_201:
  r6 = cn_var_符号指针_0->种类;
  r7 = r6 != 符号类型_枚举符号;
  goto logic_merge_202;

  logic_merge_202:
  if (r7) goto if_then_199; else goto if_merge_200;
  return NULL;
}

struct 类型信息* 从类类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3;
  char* r1;
  char* r8;
  struct 符号表管理器* r0;
  struct 符号* r2;
  struct 符号* r4;
  struct 符号* r9;
  struct 类型信息* r10;
  _Bool r5;
  _Bool r7;
  enum 符号类型 r6;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_上下文->符号表;
  r1 = cn_var_类型节点指针->名称;
  r2 = 查找符号(r0, r1);
  cn_var_符号指针_0 = r2;
  r4 = cn_var_符号指针_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_206; else goto logic_rhs_205;

  if_then_203:
  return 0;
  goto if_merge_204;

  if_merge_204:
  r8 = cn_var_类型节点指针->名称;
  r9 = cn_var_符号指针_0;
  r10 = 创建类类型(r8, r9);
  return r10;

  logic_rhs_205:
  r6 = cn_var_符号指针_0->种类;
  r7 = r6 != 符号类型_类符号;
  goto logic_merge_206;

  logic_merge_206:
  if (r7) goto if_then_203; else goto if_merge_204;
  return NULL;
}

struct 类型信息* 从接口类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3;
  char* r1;
  char* r8;
  struct 符号表管理器* r0;
  struct 符号* r2;
  struct 符号* r4;
  struct 符号* r9;
  struct 类型信息* r10;
  _Bool r5;
  _Bool r7;
  enum 符号类型 r6;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_上下文->符号表;
  r1 = cn_var_类型节点指针->名称;
  r2 = 查找符号(r0, r1);
  cn_var_符号指针_0 = r2;
  r4 = cn_var_符号指针_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_210; else goto logic_rhs_209;

  if_then_207:
  return 0;
  goto if_merge_208;

  if_merge_208:
  r8 = cn_var_类型节点指针->名称;
  r9 = cn_var_符号指针_0;
  r10 = 创建接口类型(r8, r9);
  return r10;

  logic_rhs_209:
  r6 = cn_var_符号指针_0->种类;
  r7 = r6 != 符号类型_接口符号;
  goto logic_merge_210;

  logic_merge_210:
  if (r7) goto if_then_207; else goto if_merge_208;
  return NULL;
}

struct 类型信息* 从函数类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {

  entry:
  return 0;
}

struct 类型节点* 类型信息转类型节点(struct 类型信息* cn_var_类型信息指针) {
  long long r6, r9, r33, r38, r40;
  char* r29;
  char* r31;
  char* r45;
  char* r47;
  char* r49;
  char* r51;
  char* r53;
  struct 类型信息* r0;
  void* r7;
  struct 类型节点* r8;
  void* r10;
  struct 类型信息* r35;
  struct 类型节点* r36;
  struct 类型信息* r42;
  struct 类型节点* r43;
  struct 类型节点* r56;
  _Bool r1;
  _Bool r4;
  _Bool r5;
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
  struct 类型信息 r2;
  struct 类型信息 r11;
  struct 类型信息 r28;
  struct 类型信息 r30;
  struct 类型信息 r32;
  struct 类型信息 r34;
  struct 类型信息 r37;
  struct 类型信息 r39;
  struct 类型信息 r41;
  struct 类型信息 r44;
  struct 类型信息 r46;
  struct 类型信息 r48;
  struct 类型信息 r50;
  struct 类型信息 r52;
  struct 类型信息 r54;
  enum 类型种类 r3;
  enum 类型种类 r12;

  entry:
  r0 = cn_var_类型信息指针;
  r1 = r0 == 0;
  if (r1) goto if_then_211; else goto if_merge_212;

  if_then_211:
  return 0;
  goto if_merge_212;

  if_merge_212:
  r2 = *cn_var_类型信息指针;
  r3 = r2.种类;
  r4 = 是有效的类型种类(r3);
  r5 = r4 == 0;
  if (r5) goto if_then_213; else goto if_merge_214;

  if_then_213:
  struct 类型节点* cn_var_节点_0;
  r6 = sizeof(struct 类型节点);
  r7 = 分配清零内存(1, r6);
  cn_var_节点_0 = (struct 类型节点*)分配清零内存(1, 类型大小(cn_var_类型节点));
  r8 = cn_var_节点_0;
  return r8;
  goto if_merge_214;

  if_merge_214:
  struct 类型节点* cn_var_节点_1;
  r9 = sizeof(struct 类型节点);
  r10 = 分配清零内存(1, r9);
  cn_var_节点_1 = (struct 类型节点*)分配清零内存(1, 类型大小(cn_var_类型节点));
  r11 = *cn_var_类型信息指针;
  r12 = r11.种类;
  r13 = r12 == 类型种类_类型_空;
  if (r13) goto case_body_216; else goto switch_check_232;

  switch_check_232:
  r14 = r12 == 类型种类_类型_整数;
  if (r14) goto case_body_217; else goto switch_check_233;

  switch_check_233:
  r15 = r12 == 类型种类_类型_小数;
  if (r15) goto case_body_218; else goto switch_check_234;

  switch_check_234:
  r16 = r12 == 类型种类_类型_布尔;
  if (r16) goto case_body_219; else goto switch_check_235;

  switch_check_235:
  r17 = r12 == 类型种类_类型_字符串;
  if (r17) goto case_body_220; else goto switch_check_236;

  switch_check_236:
  r18 = r12 == 类型种类_类型_字符;
  if (r18) goto case_body_221; else goto switch_check_237;

  switch_check_237:
  r19 = r12 == 类型种类_类型_指针;
  if (r19) goto case_body_222; else goto switch_check_238;

  switch_check_238:
  r20 = r12 == 类型种类_类型_数组;
  if (r20) goto case_body_223; else goto switch_check_239;

  switch_check_239:
  r21 = r12 == 类型种类_类型_结构体;
  if (r21) goto case_body_224; else goto switch_check_240;

  switch_check_240:
  r22 = r12 == 类型种类_类型_枚举;
  if (r22) goto case_body_225; else goto switch_check_241;

  switch_check_241:
  r23 = r12 == 类型种类_类型_类;
  if (r23) goto case_body_226; else goto switch_check_242;

  switch_check_242:
  r24 = r12 == 类型种类_类型_接口;
  if (r24) goto case_body_227; else goto switch_check_243;

  switch_check_243:
  r25 = r12 == 类型种类_类型_函数;
  if (r25) goto case_body_228; else goto switch_check_244;

  switch_check_244:
  r26 = r12 == 类型种类_类型_参数;
  if (r26) goto case_body_229; else goto switch_check_245;

  switch_check_245:
  r27 = r12 == 类型种类_类型_未定义;
  if (r27) goto case_body_230; else goto case_default_231;

  case_body_216:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_217:
  r28 = *cn_var_类型信息指针;
  r29 = r28.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_218:
  r30 = *cn_var_类型信息指针;
  r31 = r30.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_219:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_220:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_221:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_222:
  r32 = *cn_var_类型信息指针;
  r33 = r32.指针层级;
  r34 = *cn_var_类型信息指针;
  r35 = r34.指向类型;
  r36 = 类型信息转类型节点(r35);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_223:
  r37 = *cn_var_类型信息指针;
  r38 = r37.数组维度;
  r39 = *cn_var_类型信息指针;
  r40 = r39.总元素数;
  r41 = *cn_var_类型信息指针;
  r42 = r41.元素类型;
  r43 = 类型信息转类型节点(r42);
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_224:
  r44 = *cn_var_类型信息指针;
  r45 = r44.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_225:
  r46 = *cn_var_类型信息指针;
  r47 = r46.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_226:
  r48 = *cn_var_类型信息指针;
  r49 = r48.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_227:
  r50 = *cn_var_类型信息指针;
  r51 = r50.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_228:
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_229:
  r52 = *cn_var_类型信息指针;
  r53 = r52.名称;
  goto switch_merge_215;
  goto switch_merge_215;

  case_body_230:
  goto switch_merge_215;
  goto switch_merge_215;

  case_default_231:
  goto switch_merge_215;
  goto switch_merge_215;

  switch_merge_215:
  r54 = *cn_var_类型信息指针;
  r55 = r54.是常量;
  r56 = cn_var_节点_1;
  return r56;
}

_Bool 是有效的类型种类(enum 类型种类 cn_var_种类) {
  long long r0;
  _Bool r2;
  _Bool r4;
  enum 类型种类 r1;
  enum 类型种类 r3;

  entry:
  r1 = cn_var_种类;
  r2 = r1 < 类型种类_类型_空;
  if (r2) goto logic_merge_249; else goto logic_rhs_248;

  if_then_246:
  return 0;
  goto if_merge_247;

  if_merge_247:
  return 1;

  logic_rhs_248:
  r3 = cn_var_种类;
  r4 = r3 > 类型种类_类型_未定义;
  goto logic_merge_249;

  logic_merge_249:
  if (r4) goto if_then_246; else goto if_merge_247;
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
  if (r1) goto case_body_251; else goto switch_check_267;

  switch_check_267:
  r2 = r0 == 类型种类_类型_整数;
  if (r2) goto case_body_252; else goto switch_check_268;

  switch_check_268:
  r3 = r0 == 类型种类_类型_小数;
  if (r3) goto case_body_253; else goto switch_check_269;

  switch_check_269:
  r4 = r0 == 类型种类_类型_布尔;
  if (r4) goto case_body_254; else goto switch_check_270;

  switch_check_270:
  r5 = r0 == 类型种类_类型_字符串;
  if (r5) goto case_body_255; else goto switch_check_271;

  switch_check_271:
  r6 = r0 == 类型种类_类型_字符;
  if (r6) goto case_body_256; else goto switch_check_272;

  switch_check_272:
  r7 = r0 == 类型种类_类型_指针;
  if (r7) goto case_body_257; else goto switch_check_273;

  switch_check_273:
  r8 = r0 == 类型种类_类型_数组;
  if (r8) goto case_body_258; else goto switch_check_274;

  switch_check_274:
  r9 = r0 == 类型种类_类型_结构体;
  if (r9) goto case_body_259; else goto switch_check_275;

  switch_check_275:
  r10 = r0 == 类型种类_类型_枚举;
  if (r10) goto case_body_260; else goto switch_check_276;

  switch_check_276:
  r11 = r0 == 类型种类_类型_类;
  if (r11) goto case_body_261; else goto switch_check_277;

  switch_check_277:
  r12 = r0 == 类型种类_类型_接口;
  if (r12) goto case_body_262; else goto switch_check_278;

  switch_check_278:
  r13 = r0 == 类型种类_类型_函数;
  if (r13) goto case_body_263; else goto switch_check_279;

  switch_check_279:
  r14 = r0 == 类型种类_类型_参数;
  if (r14) goto case_body_264; else goto switch_check_280;

  switch_check_280:
  r15 = r0 == 类型种类_类型_未定义;
  if (r15) goto case_body_265; else goto case_default_266;

  case_body_251:
  return "空类型";
  goto switch_merge_250;

  case_body_252:
  return "整数";
  goto switch_merge_250;

  case_body_253:
  return "小数";
  goto switch_merge_250;

  case_body_254:
  return "布尔";
  goto switch_merge_250;

  case_body_255:
  return "字符串";
  goto switch_merge_250;

  case_body_256:
  return "字符";
  goto switch_merge_250;

  case_body_257:
  return "指针";
  goto switch_merge_250;

  case_body_258:
  return "数组";
  goto switch_merge_250;

  case_body_259:
  return "结构体";
  goto switch_merge_250;

  case_body_260:
  return "枚举";
  goto switch_merge_250;

  case_body_261:
  return "类";
  goto switch_merge_250;

  case_body_262:
  return "接口";
  goto switch_merge_250;

  case_body_263:
  return "函数";
  goto switch_merge_250;

  case_body_264:
  return "类型参数";
  goto switch_merge_250;

  case_body_265:
  return "未定义";
  goto switch_merge_250;

  case_default_266:
  return "未知";
  goto switch_merge_250;

  switch_merge_250:
  return NULL;
}

