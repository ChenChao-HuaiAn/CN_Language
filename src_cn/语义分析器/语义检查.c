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
struct 作用域构建器;
struct 作用域构建器 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
    long long 错误计数;
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
extern long long cn_var_类型推断上下文大小;
extern long long cn_var_作用域构建器大小;
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
extern struct 源位置 cn_var_空源位置;
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_符号大小;
extern long long cn_var_作用域大小;
extern long long cn_var_符号表管理器大小;
extern long long cn_var_符号指针大小;
extern long long cn_var_作用域指针大小;
extern struct 源位置 cn_var_空源位置;

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
void 报告警告(struct 语义检查上下文*, struct 源位置, const char*);
long long 语义检查上下文大小();

struct 语义检查上下文* 创建语义检查上下文(struct 诊断集合* cn_var_诊断器) {
  long long r0;
  void* r1;
  struct 诊断集合* r2;
  struct 作用域构建器* r3;
  struct 作用域构建器* r4;
  struct 符号表管理器* r5;
  struct 作用域构建器* r6;
  struct 类型推断上下文* r7;
  struct 诊断集合* r8;
  struct 语义检查上下文* r9;

  entry:
  struct 语义检查上下文* cn_var_上下文_0;
  r0 = 语义检查上下文大小();
  r1 = 分配清零内存(1, r0);
  cn_var_上下文_0 = (struct 语义检查上下文*)分配清零内存(1, 语义检查上下文大小());
  r2 = cn_var_诊断器;
  r3 = 创建作用域构建器(r2);
  r4 = cn_var_上下文_0->作用域构建器;
  r5 = r4->符号表;
  r6 = cn_var_上下文_0->作用域构建器;
  r7 = r6->类型上下文;
  r8 = cn_var_诊断器;
  r9 = cn_var_上下文_0;
  return r9;
}

void 销毁语义检查上下文(struct 语义检查上下文* cn_var_上下文) {
  long long r1;
  struct 语义检查上下文* r0;
  struct 作用域构建器* r2;
  struct 语义检查上下文* r3;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_2397; else goto if_merge_2398;

  if_then_2397:
  return;
  goto if_merge_2398;

  if_merge_2398:
  r2 = cn_var_上下文->作用域构建器;
  销毁作用域构建器(r2);
  r3 = cn_var_上下文;
  释放内存(r3);
  return;
}

_Bool 执行语义检查(struct 语义检查上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r1, r6, r7;
  struct 程序节点* r0;
  struct 作用域构建器* r2;
  struct 程序节点* r3;
  struct 语义检查上下文* r4;
  struct 程序节点* r5;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2399; else goto if_merge_2400;

  if_then_2399:
  return 0;
  goto if_merge_2400;

  if_merge_2400:
  r2 = cn_var_上下文->作用域构建器;
  r3 = cn_var_程序;
  构建程序作用域(r2, r3);
  r4 = cn_var_上下文;
  r5 = cn_var_程序;
  检查程序(r4, r5);
  r6 = cn_var_上下文->错误计数;
  r7 = r6 == 0;
  return r7;
}

void 检查程序(struct 语义检查上下文* cn_var_上下文, struct 程序节点* cn_var_程序) {
  long long r1, r2, r3, r4, r7, r9, r10;
  struct 程序节点* r0;
  struct 语义检查上下文* r5;
  struct 声明节点列表* r6;
  void* r8;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2401; else goto if_merge_2402;

  if_then_2401:
  return;
  goto if_merge_2402;

  if_merge_2402:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2403;

  for_cond_2403:
  r2 = cn_var_i_0;
  r3 = cn_var_程序->声明个数;
  r4 = r2 < r3;
  if (r4) goto for_body_2404; else goto for_exit_2406;

  for_body_2404:
  r5 = cn_var_上下文;
  r6 = cn_var_程序->声明列表;
  r7 = cn_var_i_0;
  r8 = (void*)cn_rt_array_get_element(r6, r7, 8);
  检查声明(r5, r8);
  goto for_update_2405;

  for_update_2405:
  r9 = cn_var_i_0;
  r10 = r9 + 1;
  cn_var_i_0 = r10;
  goto for_cond_2403;

  for_exit_2406:
  return;
}

void 检查声明(struct 语义检查上下文* cn_var_上下文, struct 声明节点* cn_var_声明) {
  long long r1, r2, r3, r4;
  struct 声明节点* r0;
  struct 函数声明* r11;
  struct 语义检查上下文* r12;
  struct 函数声明* r13;
  struct 变量声明* r14;
  struct 语义检查上下文* r15;
  struct 变量声明* r16;
  struct 结构体声明* r17;
  struct 语义检查上下文* r18;
  struct 结构体声明* r19;
  struct 枚举声明* r20;
  struct 语义检查上下文* r21;
  struct 枚举声明* r22;
  struct 类声明* r23;
  struct 语义检查上下文* r24;
  struct 类声明* r25;
  struct 接口声明* r26;
  struct 语义检查上下文* r27;
  struct 接口声明* r28;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;

  entry:
  r0 = cn_var_声明;
  r1 = r0 == 0;
  if (r1) goto if_then_2407; else goto if_merge_2408;

  if_then_2407:
  return;
  goto if_merge_2408;

  if_merge_2408:
  long long cn_var_节点类型值_0;
  r2 = cn_var_声明->节点基类;
  r3 = r2.类型;
  cn_var_节点类型值_0 = r3;
  r4 = cn_var_节点类型值_0;
  r5 = r4 == 节点类型_函数声明;
  if (r5) goto case_body_2410; else goto switch_check_2417;

  switch_check_2417:
  r6 = r4 == 节点类型_变量声明;
  if (r6) goto case_body_2411; else goto switch_check_2418;

  switch_check_2418:
  r7 = r4 == 节点类型_结构体声明;
  if (r7) goto case_body_2412; else goto switch_check_2419;

  switch_check_2419:
  r8 = r4 == 节点类型_枚举声明;
  if (r8) goto case_body_2413; else goto switch_check_2420;

  switch_check_2420:
  r9 = r4 == 节点类型_类声明;
  if (r9) goto case_body_2414; else goto switch_check_2421;

  switch_check_2421:
  r10 = r4 == 节点类型_接口声明;
  if (r10) goto case_body_2415; else goto case_default_2416;

  case_body_2410:
  struct 函数声明* cn_var_函数节点指针_1;
  r11 = cn_var_声明;
  cn_var_函数节点指针_1 = r11;
  r12 = cn_var_上下文;
  r13 = cn_var_函数节点指针_1;
  检查函数声明(r12, r13);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_body_2411:
  struct 变量声明* cn_var_变量节点指针_2;
  r14 = cn_var_声明;
  cn_var_变量节点指针_2 = r14;
  r15 = cn_var_上下文;
  r16 = cn_var_变量节点指针_2;
  检查变量声明(r15, r16);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_body_2412:
  struct 结构体声明* cn_var_结构体节点指针_3;
  r17 = cn_var_声明;
  cn_var_结构体节点指针_3 = r17;
  r18 = cn_var_上下文;
  r19 = cn_var_结构体节点指针_3;
  检查结构体声明(r18, r19);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_body_2413:
  struct 枚举声明* cn_var_枚举节点指针_4;
  r20 = cn_var_声明;
  cn_var_枚举节点指针_4 = r20;
  r21 = cn_var_上下文;
  r22 = cn_var_枚举节点指针_4;
  检查枚举声明(r21, r22);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_body_2414:
  struct 类声明* cn_var_类节点指针_5;
  r23 = cn_var_声明;
  cn_var_类节点指针_5 = r23;
  r24 = cn_var_上下文;
  r25 = cn_var_类节点指针_5;
  检查类声明(r24, r25);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_body_2415:
  struct 接口声明* cn_var_接口节点指针_6;
  r26 = cn_var_声明;
  cn_var_接口节点指针_6 = r26;
  r27 = cn_var_上下文;
  r28 = cn_var_接口节点指针_6;
  检查接口声明(r27, r28);
  goto switch_merge_2409;
  goto switch_merge_2409;

  case_default_2416:
  goto switch_merge_2409;
  goto switch_merge_2409;

  switch_merge_2409:
  return;
}

void 检查函数声明(struct 语义检查上下文* cn_var_上下文, struct 函数声明* cn_var_函数节点) {
  long long r1, r11, r12, r13, r16, r18, r19, r21, r24, r26, r29, r30, r32, r34, r38, r39, r40;
  char* r6;
  struct 参数** r15;
  struct 函数声明* r0;
  struct 符号* r2;
  struct 类型信息* r3;
  struct 符号表管理器* r5;
  struct 符号* r7;
  struct 类型节点* r8;
  struct 类型推断上下文* r9;
  struct 类型信息* r10;
  struct 语义检查上下文* r14;
  struct 参数* r17;
  struct 块语句* r20;
  struct 语义检查上下文* r22;
  struct 块语句* r23;
  struct 类型信息* r25;
  struct 类型信息* r27;
  struct 语义检查上下文* r35;
  _Bool r4;
  _Bool r31;
  _Bool r33;
  struct AST节点 r36;
  struct 源位置 r37;
  enum 类型种类 r28;

  entry:
  r0 = cn_var_函数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2422; else goto if_merge_2423;

  if_then_2422:
  return;
  goto if_merge_2423;

  if_merge_2423:
  long long cn_var_保存函数_0;
  r2 = cn_var_上下文->当前函数;
  cn_var_保存函数_0 = r2;
  long long cn_var_保存返回类型_1;
  r3 = cn_var_上下文->期望返回类型;
  cn_var_保存返回类型_1 = r3;
  long long cn_var_保存返回语句_2;
  r4 = cn_var_上下文->有返回语句;
  cn_var_保存返回语句_2 = r4;
  r5 = cn_var_上下文->符号表;
  r6 = cn_var_函数节点->名称;
  r7 = 查找符号(r5, r6);
  r8 = cn_var_函数节点->返回类型;
  r9 = cn_var_上下文->类型上下文;
  r10 = 从类型节点推断(r8, r9);
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_2424;

  for_cond_2424:
  r11 = cn_var_i_3;
  r12 = cn_var_函数节点->参数个数;
  r13 = r11 < r12;
  if (r13) goto for_body_2425; else goto for_exit_2427;

  for_body_2425:
  r14 = cn_var_上下文;
  r15 = cn_var_函数节点->参数列表;
  r16 = cn_var_i_3;
  r17 = (struct 参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
    块语句* 函数体;         
    可见性 可见性;          
    布尔 是静态;            
    布尔 是重写;            
    布尔 是虚拟;            
    布尔 是抽象;            
    布尔 是中断处理;        
    整数 中断向量;          
}






结构体 结构体成员 {
    字符串 名称;            
    类型节点* 类型;         
    布尔 是常量;            
    可见性 可见性;          
}






结构体 结构体声明 {
    AST节点 节点基类;           
    字符串 名称;            
    结构体成员*[] 成员;     
    整数 成员个数;          
}






结构体 枚举成员 {
    字符串 名称;            
    整数 值;                
    布尔 有显式值;          
}






结构体 枚举声明 {
    AST节点 节点基类;           
    字符串 名称;            
    枚举成员*[] 成员;       
    整数 成员个数;          
}






结构体 类成员 {
    字符串 名称;            
    节点类型 类型;          
    变量声明* 字段;         
    函数声明* 方法;         
    可见性 可见性;          
    布尔 是静态;            
    布尔 是虚拟;            
    布尔 是重写;            
    布尔 是抽象;            
}






结构体 类声明 {
    AST节点 节点基类;           
    字符串 名称;            
    字符串 基类名称;        
    字符串[] 实现接口;      
    整数 接口个数;          
    类成员*[] 成员;         
    整数 成员个数;          
    布尔 是抽象;            
}






结构体 接口方法 {
    字符串 名称;            
    参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
}






结构体 接口声明 {
    AST节点 节点基类;           
    字符串 名称;            
    接口方法*[] 方法;       
    整数 方法个数;          
}






结构体 模板参数 {
    字符串 名称;            
    类型节点* 约束;         
    类型节点* 默认类型;     
}






结构体 模板函数声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    函数声明* 函数声明节点;     
}






结构体 模板结构体声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    结构体声明* 结构体声明节点;   
}






结构体 导入成员 {
    字符串 名称;            
    字符串 别名;            
}






结构体 导入声明 {
    AST节点 节点基类;           
    导入类型 类型;          
    字符串 模块名;          
    字符串 别名;            
    导入成员*[] 成员;       
    整数 成员个数;          
    布尔 是通配符;          
    布尔 是相对导入;        
    整数 相对层级;          
}







结构体 声明节点 {
    节点类型 类型;              
    源位置 位置;                
    布尔 是否公开;              
    
    
    函数声明* 作为函数声明;      
    变量声明* 作为变量声明;      
    变量声明* 作为常量声明;      
    结构体声明* 作为结构体声明;  
    枚举声明* 作为枚举声明;      
    类声明* 作为类声明;          
    接口声明* 作为接口声明;      
    导入声明* 作为导入语句;      
    模板函数声明* 作为模板函数声明;  
    模板结构体声明* 作为模板结构体声明;  
    
    
    声明节点* 下一个;           
}






结构体 声明节点列表 {
    声明节点* 节点;            
    声明节点列表* 下一个;       
}
*)cn_rt_array_get_element(r15, r16, 8);
  检查参数(r14, r17);
  goto for_update_2426;

  for_update_2426:
  r18 = cn_var_i_3;
  r19 = r18 + 1;
  cn_var_i_3 = r19;
  goto for_cond_2424;

  for_exit_2427:
  r20 = cn_var_函数节点->函数体;
  r21 = r20 != 0;
  if (r21) goto if_then_2428; else goto if_merge_2429;

  if_then_2428:
  r22 = cn_var_上下文;
  r23 = cn_var_函数节点->函数体;
  检查语句(r22, (struct 语句节点*)cn_var_函数节点->函数体);
  goto if_merge_2429;

  if_merge_2429:
  r25 = cn_var_上下文->期望返回类型;
  r26 = r25 != 0;
  if (r26) goto logic_rhs_2432; else goto logic_merge_2433;

  if_then_2430:
  r31 = cn_var_上下文->有返回语句;
  r32 = r31 == 0;
  if (r32) goto logic_rhs_2436; else goto logic_merge_2437;

  if_merge_2431:
  r38 = cn_var_保存函数_0;
  r39 = cn_var_保存返回类型_1;
  r40 = cn_var_保存返回语句_2;

  logic_rhs_2432:
  r27 = cn_var_上下文->期望返回类型;
  r28 = r27->种类;
  r29 = r28 != 类型种类_类型_空;
  goto logic_merge_2433;

  logic_merge_2433:
  if (r29) goto if_then_2430; else goto if_merge_2431;

  if_then_2434:
  r35 = cn_var_上下文;
  r36 = cn_var_函数节点->节点基类;
  r37 = r36.位置;
  报告错误(r35, r37, 诊断错误码_语义_缺少返回语句);
  goto if_merge_2435;

  if_merge_2435:
  goto if_merge_2431;

  logic_rhs_2436:
  r33 = cn_var_函数节点->是抽象;
  r34 = r33 == 0;
  goto logic_merge_2437;

  logic_merge_2437:
  if (r34) goto if_then_2434; else goto if_merge_2435;
  return;
}

void 检查参数(struct 语义检查上下文* cn_var_上下文, struct 参数* cn_var_参数节点) {
  long long r0, r2, r4, r8, r9, r11;
  struct 参数* r1;
  struct 类型节点* r3;
  struct 类型节点* r5;
  struct 类型推断上下文* r6;
  struct 类型信息* r7;
  struct 语义检查上下文* r10;

  entry:
  r1 = cn_var_参数节点;
  r2 = r1 == 0;
  if (r2) goto logic_merge_2441; else goto logic_rhs_2440;

  if_then_2438:
  return;
  goto if_merge_2439;

  if_merge_2439:
  long long cn_var_类型_0;
  r5 = cn_var_参数节点->类型;
  r6 = cn_var_上下文->类型上下文;
  r7 = 从类型节点推断(r5, r6);
  cn_var_类型_0 = r7;
  r8 = cn_var_类型_0;
  r9 = r8 == 0;
  if (r9) goto if_then_2442; else goto if_merge_2443;

  logic_rhs_2440:
  r3 = cn_var_参数节点->类型;
  r4 = r3 == 0;
  goto logic_merge_2441;

  logic_merge_2441:
  if (r4) goto if_then_2438; else goto if_merge_2439;

  if_then_2442:
  r10 = cn_var_上下文;
  r11 = cn_var_参数节点->定义位置;
  报告错误(r10, r11, 诊断错误码_语义_未定义标识符);
  goto if_merge_2443;

  if_merge_2443:
  return;
}

void 检查变量声明(struct 语义检查上下文* cn_var_上下文, struct 变量声明* cn_var_变量节点) {
  long long r1, r4, r10, r16, r19, r23, r25, r28, r31, r32, r37, r42, r46, r47, r51, r52, r56, r57, r58, r59, r61, r62, r66, r67, r68, r73, r74;
  char* r30;
  char* r71;
  struct 变量声明* r0;
  struct 表达式节点* r3;
  struct 语义检查上下文* r5;
  struct 表达式节点* r8;
  struct 语义检查上下文* r11;
  struct 表达式节点* r15;
  struct 表达式节点* r17;
  struct 语义检查上下文* r20;
  struct 类型节点* r24;
  struct 类型节点* r26;
  struct 类型节点* r29;
  struct 语义检查上下文* r33;
  struct 表达式节点* r36;
  struct 语义检查上下文* r38;
  struct 表达式节点* r39;
  struct 类型信息* r40;
  struct 类型节点* r41;
  struct 类型节点* r43;
  struct 类型推断上下文* r44;
  struct 类型信息* r45;
  struct 语义检查上下文* r48;
  struct 类型节点* r53;
  struct 类型推断上下文* r54;
  struct 类型信息* r55;
  struct 语义检查上下文* r63;
  struct 类型节点* r69;
  struct 符号表管理器* r70;
  struct 符号* r72;
  struct 类型节点* r75;
  _Bool r2;
  _Bool r9;
  _Bool r14;
  _Bool r18;
  struct AST节点 r6;
  struct 源位置 r7;
  struct AST节点 r12;
  struct 源位置 r13;
  struct AST节点 r21;
  struct 源位置 r22;
  struct AST节点 r34;
  struct 源位置 r35;
  struct AST节点 r49;
  struct 源位置 r50;
  enum 类型兼容性 r60;
  struct AST节点 r64;
  struct 源位置 r65;
  enum 节点类型 r27;

  entry:
  r0 = cn_var_变量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2444; else goto if_merge_2445;

  if_then_2444:
  return;
  goto if_merge_2445;

  if_merge_2445:
  r2 = cn_var_变量节点->是常量;
  if (r2) goto if_then_2446; else goto if_merge_2447;

  if_then_2446:
  r3 = cn_var_变量节点->初始值;
  r4 = r3 == 0;
  if (r4) goto if_then_2448; else goto if_else_2449;

  if_merge_2447:
  r14 = cn_var_变量节点->是静态;
  if (r14) goto if_then_2453; else goto if_merge_2454;

  if_then_2448:
  r5 = cn_var_上下文;
  r6 = cn_var_变量节点->节点基类;
  r7 = r6.位置;
  报告错误(r5, r7, 诊断错误码_语义_常量缺少初始化);
  goto if_merge_2450;

  if_else_2449:
  r8 = cn_var_变量节点->初始值;
  r9 = 是常量表达式(r8);
  r10 = r9 == 0;
  if (r10) goto if_then_2451; else goto if_merge_2452;

  if_merge_2450:
  goto if_merge_2447;

  if_then_2451:
  r11 = cn_var_上下文;
  r12 = cn_var_变量节点->节点基类;
  r13 = r12.位置;
  报告错误(r11, r13, 诊断错误码_语义_常量非常量初始化);
  goto if_merge_2452;

  if_merge_2452:
  goto if_merge_2450;

  if_then_2453:
  r15 = cn_var_变量节点->初始值;
  r16 = r15 != 0;
  if (r16) goto if_then_2455; else goto if_merge_2456;

  if_merge_2454:
  long long cn_var_推断类型_0;
  cn_var_推断类型_0 = 0;
  r36 = cn_var_变量节点->初始值;
  r37 = r36 != 0;
  if (r37) goto if_then_2465; else goto if_merge_2466;

  if_then_2455:
  r17 = cn_var_变量节点->初始值;
  r18 = 是常量表达式(r17);
  r19 = r18 == 0;
  if (r19) goto if_then_2457; else goto if_merge_2458;

  if_merge_2456:
  r24 = cn_var_变量节点->类型;
  r25 = r24 != 0;
  if (r25) goto logic_rhs_2461; else goto logic_merge_2462;

  if_then_2457:
  r20 = cn_var_上下文;
  r21 = cn_var_变量节点->节点基类;
  r22 = r21.位置;
  报告错误(r20, r22, 诊断错误码_语义_静态非常量初始化);
  goto if_merge_2458;

  if_merge_2458:
  goto if_merge_2456;

  if_then_2459:
  r29 = cn_var_变量节点->类型;
  r30 = r29->名称;
  r31 = 比较字符串(r30, "空类型");
  r32 = r31 == 0;
  if (r32) goto if_then_2463; else goto if_merge_2464;

  if_merge_2460:
  goto if_merge_2454;

  logic_rhs_2461:
  r26 = cn_var_变量节点->类型;
  r27 = r26->类型;
  r28 = r27 == 节点类型_基础类型;
  goto logic_merge_2462;

  logic_merge_2462:
  if (r28) goto if_then_2459; else goto if_merge_2460;

  if_then_2463:
  r33 = cn_var_上下文;
  r34 = cn_var_变量节点->节点基类;
  r35 = r34.位置;
  报告错误(r33, r35, 诊断错误码_语义_静态空类型);
  goto if_merge_2464;

  if_merge_2464:
  goto if_merge_2460;

  if_then_2465:
  r38 = cn_var_上下文;
  r39 = cn_var_变量节点->初始值;
  r40 = 检查表达式(r38, r39);
  cn_var_推断类型_0 = r40;
  goto if_merge_2466;

  if_merge_2466:
  r41 = cn_var_变量节点->类型;
  r42 = r41 != 0;
  if (r42) goto if_then_2467; else goto if_else_2468;

  if_then_2467:
  long long cn_var_类型_1;
  r43 = cn_var_变量节点->类型;
  r44 = cn_var_上下文->类型上下文;
  r45 = 从类型节点推断(r43, r44);
  cn_var_类型_1 = r45;
  r46 = cn_var_类型_1;
  r47 = r46 == 0;
  if (r47) goto if_then_2470; else goto if_merge_2471;

  if_else_2468:
  r66 = cn_var_推断类型_0;
  r67 = r66 != 0;
  if (r67) goto if_then_2478; else goto if_merge_2479;

  if_merge_2469:

  if_then_2470:
  r48 = cn_var_上下文;
  r49 = cn_var_变量节点->节点基类;
  r50 = r49.位置;
  报告错误(r48, r50, 诊断错误码_语义_未定义标识符);
  goto if_merge_2471;

  if_merge_2471:
  r51 = cn_var_推断类型_0;
  r52 = r51 != 0;
  if (r52) goto if_then_2472; else goto if_merge_2473;

  if_then_2472:
  long long cn_var_变量类型_2;
  r53 = cn_var_变量节点->类型;
  r54 = cn_var_上下文->类型上下文;
  r55 = 从类型节点推断(r53, r54);
  cn_var_变量类型_2 = r55;
  r56 = cn_var_变量类型_2;
  r57 = r56 != 0;
  if (r57) goto if_then_2474; else goto if_merge_2475;

  if_merge_2473:
  goto if_merge_2469;

  if_then_2474:
  long long cn_var_兼容性_3;
  r58 = cn_var_推断类型_0;
  r59 = cn_var_变量类型_2;
  r60 = 检查类型兼容性(r58, r59);
  cn_var_兼容性_3 = r60;
  r61 = cn_var_兼容性_3;
  r62 = r61 == 类型兼容性_不兼容;
  if (r62) goto if_then_2476; else goto if_merge_2477;

  if_merge_2475:
  goto if_merge_2473;

  if_then_2476:
  r63 = cn_var_上下文;
  r64 = cn_var_变量节点->节点基类;
  r65 = r64.位置;
  报告错误(r63, r65, 诊断错误码_语义_类型不匹配);
  goto if_merge_2477;

  if_merge_2477:
  goto if_merge_2475;

  if_then_2478:
  r68 = cn_var_推断类型_0;
  r69 = 类型信息转类型节点(r68);
  long long cn_var_符号指针_4;
  r70 = cn_var_上下文->符号表;
  r71 = cn_var_变量节点->名称;
  r72 = 查找符号(r70, r71);
  cn_var_符号指针_4 = r72;
  r73 = cn_var_符号指针_4;
  r74 = r73 != 0;
  if (r74) goto if_then_2480; else goto if_merge_2481;

  if_merge_2479:
  goto if_merge_2469;

  if_then_2480:
  r75 = cn_var_变量节点->类型;
  goto if_merge_2481;

  if_merge_2481:
  goto if_merge_2479;
  return;
}

void 检查结构体声明(struct 语义检查上下文* cn_var_上下文, struct 结构体声明* cn_var_结构体节点) {
  long long r1, r2, r3, r4, r6, r8, r9, r10, r13, r14, r18, r19;
  struct 结构体成员** r5;
  struct 结构体声明* r0;
  struct 结构体成员* r7;
  struct 类型推断上下文* r11;
  struct 类型信息* r12;
  struct 语义检查上下文* r15;
  struct AST节点 r16;
  struct 源位置 r17;

  entry:
  r0 = cn_var_结构体节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2482; else goto if_merge_2483;

  if_then_2482:
  return;
  goto if_merge_2483;

  if_merge_2483:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2484;

  for_cond_2484:
  r2 = cn_var_i_0;
  r3 = cn_var_结构体节点->成员个数;
  r4 = r2 < r3;
  if (r4) goto for_body_2485; else goto for_exit_2487;

  for_body_2485:
  long long cn_var_成员_1;
  r5 = cn_var_结构体节点->成员;
  r6 = cn_var_i_0;
  r7 = (struct 结构体成员*[] 成员;     
    整数 成员个数;          
}






结构体 枚举成员 {
    字符串 名称;            
    整数 值;                
    布尔 有显式值;          
}






结构体 枚举声明 {
    AST节点 节点基类;           
    字符串 名称;            
    枚举成员*[] 成员;       
    整数 成员个数;          
}






结构体 类成员 {
    字符串 名称;            
    节点类型 类型;          
    变量声明* 字段;         
    函数声明* 方法;         
    可见性 可见性;          
    布尔 是静态;            
    布尔 是虚拟;            
    布尔 是重写;            
    布尔 是抽象;            
}






结构体 类声明 {
    AST节点 节点基类;           
    字符串 名称;            
    字符串 基类名称;        
    字符串[] 实现接口;      
    整数 接口个数;          
    类成员*[] 成员;         
    整数 成员个数;          
    布尔 是抽象;            
}






结构体 接口方法 {
    字符串 名称;            
    参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
}






结构体 接口声明 {
    AST节点 节点基类;           
    字符串 名称;            
    接口方法*[] 方法;       
    整数 方法个数;          
}






结构体 模板参数 {
    字符串 名称;            
    类型节点* 约束;         
    类型节点* 默认类型;     
}






结构体 模板函数声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    函数声明* 函数声明节点;     
}






结构体 模板结构体声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    结构体声明* 结构体声明节点;   
}






结构体 导入成员 {
    字符串 名称;            
    字符串 别名;            
}






结构体 导入声明 {
    AST节点 节点基类;           
    导入类型 类型;          
    字符串 模块名;          
    字符串 别名;            
    导入成员*[] 成员;       
    整数 成员个数;          
    布尔 是通配符;          
    布尔 是相对导入;        
    整数 相对层级;          
}







结构体 声明节点 {
    节点类型 类型;              
    源位置 位置;                
    布尔 是否公开;              
    
    
    函数声明* 作为函数声明;      
    变量声明* 作为变量声明;      
    变量声明* 作为常量声明;      
    结构体声明* 作为结构体声明;  
    枚举声明* 作为枚举声明;      
    类声明* 作为类声明;          
    接口声明* 作为接口声明;      
    导入声明* 作为导入语句;      
    模板函数声明* 作为模板函数声明;  
    模板结构体声明* 作为模板结构体声明;  
    
    
    声明节点* 下一个;           
}






结构体 声明节点列表 {
    声明节点* 节点;            
    声明节点列表* 下一个;       
}
*)cn_rt_array_get_element(r5, r6, 8);
  cn_var_成员_1 = r7;
  r8 = cn_var_成员_1.类型;
  r9 = r8 != 0;
  if (r9) goto if_then_2488; else goto if_merge_2489;

  for_update_2486:
  r18 = cn_var_i_0;
  r19 = r18 + 1;
  cn_var_i_0 = r19;
  goto for_cond_2484;

  for_exit_2487:

  if_then_2488:
  long long cn_var_类型_2;
  r10 = cn_var_成员_1.类型;
  r11 = cn_var_上下文->类型上下文;
  r12 = 从类型节点推断(r10, r11);
  cn_var_类型_2 = r12;
  r13 = cn_var_类型_2;
  r14 = r13 == 0;
  if (r14) goto if_then_2490; else goto if_merge_2491;

  if_merge_2489:
  goto for_update_2486;

  if_then_2490:
  r15 = cn_var_上下文;
  r16 = cn_var_结构体节点->节点基类;
  r17 = r16.位置;
  报告错误(r15, r17, 诊断错误码_语义_未定义标识符);
  goto if_merge_2491;

  if_merge_2491:
  goto if_merge_2489;
  return;
}

void 检查枚举声明(struct 语义检查上下文* cn_var_上下文, struct 枚举声明* cn_var_枚举节点) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r11, r15, r18, r19, r23, r24, r25, r26;
  char* r13;
  char* r17;
  struct 枚举成员** r10;
  struct 枚举成员** r14;
  struct 枚举声明* r0;
  struct 枚举成员* r12;
  struct 枚举成员* r16;
  struct 语义检查上下文* r20;
  struct AST节点 r21;
  struct 源位置 r22;

  entry:
  r0 = cn_var_枚举节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2492; else goto if_merge_2493;

  if_then_2492:
  return;
  goto if_merge_2493;

  if_merge_2493:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2494;

  for_cond_2494:
  r2 = cn_var_i_0;
  r3 = cn_var_枚举节点->成员个数;
  r4 = r2 < r3;
  if (r4) goto for_body_2495; else goto for_exit_2497;

  for_body_2495:
  long long cn_var_j_1;
  r5 = cn_var_i_0;
  r6 = r5 + 1;
  cn_var_j_1 = r6;
  goto for_cond_2498;

  for_update_2496:
  r25 = cn_var_i_0;
  r26 = r25 + 1;
  cn_var_i_0 = r26;
  goto for_cond_2494;

  for_exit_2497:

  for_cond_2498:
  r7 = cn_var_j_1;
  r8 = cn_var_枚举节点->成员个数;
  r9 = r7 < r8;
  if (r9) goto for_body_2499; else goto for_exit_2501;

  for_body_2499:
  r10 = cn_var_枚举节点->成员;
  r11 = cn_var_i_0;
  r12 = (struct 枚举成员*[] 成员;       
    整数 成员个数;          
}






结构体 类成员 {
    字符串 名称;            
    节点类型 类型;          
    变量声明* 字段;         
    函数声明* 方法;         
    可见性 可见性;          
    布尔 是静态;            
    布尔 是虚拟;            
    布尔 是重写;            
    布尔 是抽象;            
}






结构体 类声明 {
    AST节点 节点基类;           
    字符串 名称;            
    字符串 基类名称;        
    字符串[] 实现接口;      
    整数 接口个数;          
    类成员*[] 成员;         
    整数 成员个数;          
    布尔 是抽象;            
}






结构体 接口方法 {
    字符串 名称;            
    参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
}






结构体 接口声明 {
    AST节点 节点基类;           
    字符串 名称;            
    接口方法*[] 方法;       
    整数 方法个数;          
}






结构体 模板参数 {
    字符串 名称;            
    类型节点* 约束;         
    类型节点* 默认类型;     
}






结构体 模板函数声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    函数声明* 函数声明节点;     
}






结构体 模板结构体声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    结构体声明* 结构体声明节点;   
}






结构体 导入成员 {
    字符串 名称;            
    字符串 别名;            
}






结构体 导入声明 {
    AST节点 节点基类;           
    导入类型 类型;          
    字符串 模块名;          
    字符串 别名;            
    导入成员*[] 成员;       
    整数 成员个数;          
    布尔 是通配符;          
    布尔 是相对导入;        
    整数 相对层级;          
}







结构体 声明节点 {
    节点类型 类型;              
    源位置 位置;                
    布尔 是否公开;              
    
    
    函数声明* 作为函数声明;      
    变量声明* 作为变量声明;      
    变量声明* 作为常量声明;      
    结构体声明* 作为结构体声明;  
    枚举声明* 作为枚举声明;      
    类声明* 作为类声明;          
    接口声明* 作为接口声明;      
    导入声明* 作为导入语句;      
    模板函数声明* 作为模板函数声明;  
    模板结构体声明* 作为模板结构体声明;  
    
    
    声明节点* 下一个;           
}






结构体 声明节点列表 {
    声明节点* 节点;            
    声明节点列表* 下一个;       
}
*)cn_rt_array_get_element(r10, r11, 8);
  r13 = r12->名称;
  r14 = cn_var_枚举节点->成员;
  r15 = cn_var_j_1;
  r16 = (struct 枚举成员*[] 成员;       
    整数 成员个数;          
}






结构体 类成员 {
    字符串 名称;            
    节点类型 类型;          
    变量声明* 字段;         
    函数声明* 方法;         
    可见性 可见性;          
    布尔 是静态;            
    布尔 是虚拟;            
    布尔 是重写;            
    布尔 是抽象;            
}






结构体 类声明 {
    AST节点 节点基类;           
    字符串 名称;            
    字符串 基类名称;        
    字符串[] 实现接口;      
    整数 接口个数;          
    类成员*[] 成员;         
    整数 成员个数;          
    布尔 是抽象;            
}






结构体 接口方法 {
    字符串 名称;            
    参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
}






结构体 接口声明 {
    AST节点 节点基类;           
    字符串 名称;            
    接口方法*[] 方法;       
    整数 方法个数;          
}






结构体 模板参数 {
    字符串 名称;            
    类型节点* 约束;         
    类型节点* 默认类型;     
}






结构体 模板函数声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    函数声明* 函数声明节点;     
}






结构体 模板结构体声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    结构体声明* 结构体声明节点;   
}






结构体 导入成员 {
    字符串 名称;            
    字符串 别名;            
}






结构体 导入声明 {
    AST节点 节点基类;           
    导入类型 类型;          
    字符串 模块名;          
    字符串 别名;            
    导入成员*[] 成员;       
    整数 成员个数;          
    布尔 是通配符;          
    布尔 是相对导入;        
    整数 相对层级;          
}







结构体 声明节点 {
    节点类型 类型;              
    源位置 位置;                
    布尔 是否公开;              
    
    
    函数声明* 作为函数声明;      
    变量声明* 作为变量声明;      
    变量声明* 作为常量声明;      
    结构体声明* 作为结构体声明;  
    枚举声明* 作为枚举声明;      
    类声明* 作为类声明;          
    接口声明* 作为接口声明;      
    导入声明* 作为导入语句;      
    模板函数声明* 作为模板函数声明;  
    模板结构体声明* 作为模板结构体声明;  
    
    
    声明节点* 下一个;           
}






结构体 声明节点列表 {
    声明节点* 节点;            
    声明节点列表* 下一个;       
}
*)cn_rt_array_get_element(r14, r15, 8);
  r17 = r16->名称;
  r18 = 比较字符串(r13, r17);
  r19 = r18 == 0;
  if (r19) goto if_then_2502; else goto if_merge_2503;

  for_update_2500:
  r23 = cn_var_j_1;
  r24 = r23 + 1;
  cn_var_j_1 = r24;
  goto for_cond_2498;

  for_exit_2501:
  goto for_update_2496;

  if_then_2502:
  r20 = cn_var_上下文;
  r21 = cn_var_枚举节点->节点基类;
  r22 = r21.位置;
  报告错误(r20, r22, 诊断错误码_语义_重复符号);
  goto if_merge_2503;

  if_merge_2503:
  goto for_update_2500;
  return;
}

void 检查类声明(struct 语义检查上下文* cn_var_上下文, struct 类声明* cn_var_类节点) {
  long long r1, r2, r4, r6, r7, r11, r12, r13, r14, r15, r19, r20, r21, r24, r27, r29, r30, r31, r35, r36, r37, r38, r39, r42, r44, r45;
  char* r3;
  char* r5;
  char* r9;
  char* r25;
  char** r23;
  struct 类成员** r41;
  struct 类声明* r0;
  struct 符号表管理器* r8;
  struct 符号* r10;
  struct 语义检查上下文* r16;
  struct 符号表管理器* r22;
  struct 符号* r26;
  struct 语义检查上下文* r32;
  struct 语义检查上下文* r40;
  struct 类成员* r43;
  struct AST节点 r17;
  struct 源位置 r18;
  struct AST节点 r33;
  struct 源位置 r34;
  enum 符号类型 r28;

  entry:
  r0 = cn_var_类节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2504; else goto if_merge_2505;

  if_then_2504:
  return;
  goto if_merge_2505;

  if_merge_2505:
  r3 = cn_var_类节点->基类名称;
  r4 = r3 != 0;
  if (r4) goto logic_rhs_2508; else goto logic_merge_2509;

  if_then_2506:
  long long cn_var_父类符号_0;
  r8 = cn_var_上下文->符号表;
  r9 = cn_var_类节点->基类名称;
  r10 = 查找符号(r8, r9);
  cn_var_父类符号_0 = r10;
  r12 = cn_var_父类符号_0;
  r13 = r12 == 0;
  if (r13) goto logic_merge_2513; else goto logic_rhs_2512;

  if_merge_2507:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2514;

  logic_rhs_2508:
  r5 = cn_var_类节点->基类名称;
  r6 = 获取字符串长度(r5);
  r7 = r6 > 0;
  goto logic_merge_2509;

  logic_merge_2509:
  if (r7) goto if_then_2506; else goto if_merge_2507;

  if_then_2510:
  r16 = cn_var_上下文;
  r17 = cn_var_类节点->节点基类;
  r18 = r17.位置;
  报告错误(r16, r18, 诊断错误码_语义_未定义标识符);
  goto if_merge_2511;

  if_merge_2511:
  goto if_merge_2507;

  logic_rhs_2512:
  r14 = cn_var_父类符号_0.种类;
  r15 = r14 != 符号类型_类符号;
  goto logic_merge_2513;

  logic_merge_2513:
  if (r15) goto if_then_2510; else goto if_merge_2511;

  for_cond_2514:
  r19 = cn_var_i_1;
  r20 = cn_var_类节点->接口个数;
  r21 = r19 < r20;
  if (r21) goto for_body_2515; else goto for_exit_2517;

  for_body_2515:
  enum 符号类型 cn_var_接口符号_2;
  r22 = cn_var_上下文->符号表;
  r23 = cn_var_类节点->实现接口;
  r24 = cn_var_i_1;
  r25 = *(char*)cn_rt_array_get_element(r23, r24, 8);
  r26 = 查找符号(r22, r25);
  cn_var_接口符号_2 = r26;
  r28 = cn_var_接口符号_2;
  r29 = r28 == 0;
  if (r29) goto logic_merge_2521; else goto logic_rhs_2520;

  for_update_2516:
  r35 = cn_var_i_1;
  r36 = r35 + 1;
  cn_var_i_1 = r36;
  goto for_cond_2514;

  for_exit_2517:
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_2522;

  if_then_2518:
  r32 = cn_var_上下文;
  r33 = cn_var_类节点->节点基类;
  r34 = r33.位置;
  报告错误(r32, r34, 诊断错误码_语义_未定义标识符);
  goto if_merge_2519;

  if_merge_2519:
  goto for_update_2516;

  logic_rhs_2520:
  r30 = cn_var_接口符号_2.种类;
  r31 = r30 != 符号类型_接口符号;
  goto logic_merge_2521;

  logic_merge_2521:
  if (r31) goto if_then_2518; else goto if_merge_2519;

  for_cond_2522:
  r37 = cn_var_i_3;
  r38 = cn_var_类节点->成员个数;
  r39 = r37 < r38;
  if (r39) goto for_body_2523; else goto for_exit_2525;

  for_body_2523:
  r40 = cn_var_上下文;
  r41 = cn_var_类节点->成员;
  r42 = cn_var_i_3;
  r43 = (struct 类成员*[] 成员;         
    整数 成员个数;          
    布尔 是抽象;            
}






结构体 接口方法 {
    字符串 名称;            
    参数*[] 参数列表;       
    整数 参数个数;          
    类型节点* 返回类型;     
}






结构体 接口声明 {
    AST节点 节点基类;           
    字符串 名称;            
    接口方法*[] 方法;       
    整数 方法个数;          
}






结构体 模板参数 {
    字符串 名称;            
    类型节点* 约束;         
    类型节点* 默认类型;     
}






结构体 模板函数声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    函数声明* 函数声明节点;     
}






结构体 模板结构体声明 {
    AST节点 节点基类;           
    模板参数*[] 模板参数;   
    整数 参数个数;          
    结构体声明* 结构体声明节点;   
}






结构体 导入成员 {
    字符串 名称;            
    字符串 别名;            
}






结构体 导入声明 {
    AST节点 节点基类;           
    导入类型 类型;          
    字符串 模块名;          
    字符串 别名;            
    导入成员*[] 成员;       
    整数 成员个数;          
    布尔 是通配符;          
    布尔 是相对导入;        
    整数 相对层级;          
}







结构体 声明节点 {
    节点类型 类型;              
    源位置 位置;                
    布尔 是否公开;              
    
    
    函数声明* 作为函数声明;      
    变量声明* 作为变量声明;      
    变量声明* 作为常量声明;      
    结构体声明* 作为结构体声明;  
    枚举声明* 作为枚举声明;      
    类声明* 作为类声明;          
    接口声明* 作为接口声明;      
    导入声明* 作为导入语句;      
    模板函数声明* 作为模板函数声明;  
    模板结构体声明* 作为模板结构体声明;  
    
    
    声明节点* 下一个;           
}






结构体 声明节点列表 {
    声明节点* 节点;            
    声明节点列表* 下一个;       
}
*)cn_rt_array_get_element(r41, r42, 8);
  检查类成员(r40, r43);
  goto for_update_2524;

  for_update_2524:
  r44 = cn_var_i_3;
  r45 = r44 + 1;
  cn_var_i_3 = r45;
  goto for_cond_2522;

  for_exit_2525:
  return;
}

void 检查类成员(struct 语义检查上下文* cn_var_上下文, struct 类成员* cn_var_成员节点) {
  long long r1, r3, r7;
  struct 类成员* r0;
  struct 语义检查上下文* r4;
  struct 变量声明* r5;
  struct 语义检查上下文* r8;
  struct 函数声明* r9;
  enum 节点类型 r2;
  enum 节点类型 r6;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2526; else goto if_merge_2527;

  if_then_2526:
  return;
  goto if_merge_2527;

  if_merge_2527:
  r2 = cn_var_成员节点->类型;
  r3 = r2 == 节点类型_变量声明;
  if (r3) goto if_then_2528; else goto if_else_2529;

  if_then_2528:
  r4 = cn_var_上下文;
  r5 = cn_var_成员节点->字段;
  检查变量声明(r4, r5);
  goto if_merge_2530;

  if_else_2529:
  r6 = cn_var_成员节点->类型;
  r7 = r6 == 节点类型_函数声明;
  if (r7) goto if_then_2531; else goto if_merge_2532;

  if_merge_2530:

  if_then_2531:
  r8 = cn_var_上下文;
  r9 = cn_var_成员节点->方法;
  检查函数声明(r8, r9);
  goto if_merge_2532;

  if_merge_2532:
  goto if_merge_2530;
  return;
}

void 检查接口声明(struct 语义检查上下文* cn_var_上下文, struct 接口声明* cn_var_接口节点) {
  long long r1, r2, r3, r4, r5, r6, r8, r9, r10, r11, r12, r14, r15, r16, r17, r19, r22, r23, r27, r28, r29, r30, r31, r34, r35, r39, r40;
  struct 接口声明* r0;
  void* r7;
  void* r13;
  void* r18;
  struct 类型推断上下文* r20;
  struct 类型信息* r21;
  struct 语义检查上下文* r24;
  struct 类型推断上下文* r32;
  struct 类型信息* r33;
  struct 语义检查上下文* r36;
  struct AST节点 r25;
  struct 源位置 r26;
  struct AST节点 r37;
  struct 源位置 r38;

  entry:
  r0 = cn_var_接口节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2533; else goto if_merge_2534;

  if_then_2533:
  return;
  goto if_merge_2534;

  if_merge_2534:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2535;

  for_cond_2535:
  r2 = cn_var_i_0;
  r3 = cn_var_接口节点->方法个数;
  r4 = r2 < r3;
  if (r4) goto for_body_2536; else goto for_exit_2538;

  for_body_2536:
  long long cn_var_方法_1;
  r5 = cn_var_接口节点->方法列表;
  r6 = cn_var_i_0;
  r7 = (void*)cn_rt_array_get_element(r5, r6, 8);
  cn_var_方法_1 = r7;
  long long cn_var_j_2;
  cn_var_j_2 = 0;
  goto for_cond_2539;

  for_update_2537:
  r39 = cn_var_i_0;
  r40 = r39 + 1;
  cn_var_i_0 = r40;
  goto for_cond_2535;

  for_exit_2538:

  for_cond_2539:
  r8 = cn_var_j_2;
  r9 = cn_var_方法_1.参数个数;
  r10 = r8 < r9;
  if (r10) goto for_body_2540; else goto for_exit_2542;

  for_body_2540:
  r11 = cn_var_方法_1.参数列表;
  r12 = cn_var_j_2;
  r13 = (void*)cn_rt_array_get_element(r11, r12, 8);
  r14 = r13->类型;
  r15 = r14 != 0;
  if (r15) goto if_then_2543; else goto if_merge_2544;

  for_update_2541:
  r27 = cn_var_j_2;
  r28 = r27 + 1;
  cn_var_j_2 = r28;
  goto for_cond_2539;

  for_exit_2542:
  r29 = cn_var_方法_1.返回类型;
  r30 = r29 != 0;
  if (r30) goto if_then_2547; else goto if_merge_2548;

  if_then_2543:
  long long cn_var_类型_3;
  r16 = cn_var_方法_1.参数列表;
  r17 = cn_var_j_2;
  r18 = (void*)cn_rt_array_get_element(r16, r17, 8);
  r19 = r18->类型;
  r20 = cn_var_上下文->类型上下文;
  r21 = 从类型节点推断(r19, r20);
  cn_var_类型_3 = r21;
  r22 = cn_var_类型_3;
  r23 = r22 == 0;
  if (r23) goto if_then_2545; else goto if_merge_2546;

  if_merge_2544:
  goto for_update_2541;

  if_then_2545:
  r24 = cn_var_上下文;
  r25 = cn_var_接口节点->节点基类;
  r26 = r25.位置;
  报告错误(r24, r26, 诊断错误码_语义_未定义标识符);
  goto if_merge_2546;

  if_merge_2546:
  goto if_merge_2544;

  if_then_2547:
  long long cn_var_类型_4;
  r31 = cn_var_方法_1.返回类型;
  r32 = cn_var_上下文->类型上下文;
  r33 = 从类型节点推断(r31, r32);
  cn_var_类型_4 = r33;
  r34 = cn_var_类型_4;
  r35 = r34 == 0;
  if (r35) goto if_then_2549; else goto if_merge_2550;

  if_merge_2548:
  goto for_update_2537;

  if_then_2549:
  r36 = cn_var_上下文;
  r37 = cn_var_接口节点->节点基类;
  r38 = r37.位置;
  报告错误(r36, r38, 诊断错误码_语义_未定义标识符);
  goto if_merge_2550;

  if_merge_2550:
  goto if_merge_2548;
  return;
}

void 检查语句(struct 语义检查上下文* cn_var_上下文, struct 语句节点* cn_var_语句) {
  long long r1, r2, r3;
  struct 语句节点* r0;
  struct 语义检查上下文* r15;
  struct 块语句* r16;
  struct 语义检查上下文* r17;
  struct 如果语句* r18;
  struct 语义检查上下文* r19;
  struct 当语句* r20;
  struct 语义检查上下文* r21;
  struct 循环语句* r22;
  struct 语义检查上下文* r23;
  struct 返回语句* r24;
  struct 语义检查上下文* r25;
  struct 中断语句* r26;
  struct 语义检查上下文* r27;
  struct 继续语句* r28;
  struct 语义检查上下文* r29;
  struct 选择语句* r30;
  struct 语义检查上下文* r31;
  struct 表达式语句* r32;
  struct 表达式节点* r33;
  struct 类型信息* r34;
  struct 语义检查上下文* r35;
  struct 尝试语句* r36;
  struct 语义检查上下文* r37;
  struct 抛出语句* r38;
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

  entry:
  r0 = cn_var_语句;
  r1 = r0 == 0;
  if (r1) goto if_then_2551; else goto if_merge_2552;

  if_then_2551:
  return;
  goto if_merge_2552;

  if_merge_2552:
  r2 = cn_var_语句->节点基类;
  r3 = r2.类型;
  r4 = r3 == 节点类型_块语句;
  if (r4) goto case_body_2554; else goto switch_check_2566;

  switch_check_2566:
  r5 = r3 == 节点类型_如果语句;
  if (r5) goto case_body_2555; else goto switch_check_2567;

  switch_check_2567:
  r6 = r3 == 节点类型_当语句;
  if (r6) goto case_body_2556; else goto switch_check_2568;

  switch_check_2568:
  r7 = r3 == 节点类型_循环语句;
  if (r7) goto case_body_2557; else goto switch_check_2569;

  switch_check_2569:
  r8 = r3 == 节点类型_返回语句;
  if (r8) goto case_body_2558; else goto switch_check_2570;

  switch_check_2570:
  r9 = r3 == 节点类型_中断语句;
  if (r9) goto case_body_2559; else goto switch_check_2571;

  switch_check_2571:
  r10 = r3 == 节点类型_继续语句;
  if (r10) goto case_body_2560; else goto switch_check_2572;

  switch_check_2572:
  r11 = r3 == 节点类型_选择语句;
  if (r11) goto case_body_2561; else goto switch_check_2573;

  switch_check_2573:
  r12 = r3 == 节点类型_表达式语句;
  if (r12) goto case_body_2562; else goto switch_check_2574;

  switch_check_2574:
  r13 = r3 == 节点类型_尝试语句;
  if (r13) goto case_body_2563; else goto switch_check_2575;

  switch_check_2575:
  r14 = r3 == 节点类型_抛出语句;
  if (r14) goto case_body_2564; else goto case_default_2565;

  case_body_2554:
  r15 = cn_var_上下文;
  r16 = cn_var_语句;
  检查块语句(r15, r16);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2555:
  r17 = cn_var_上下文;
  r18 = cn_var_语句;
  检查如果语句(r17, r18);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2556:
  r19 = cn_var_上下文;
  r20 = cn_var_语句;
  检查当语句(r19, r20);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2557:
  r21 = cn_var_上下文;
  r22 = cn_var_语句;
  检查循环语句(r21, r22);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2558:
  r23 = cn_var_上下文;
  r24 = cn_var_语句;
  检查返回语句(r23, r24);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2559:
  r25 = cn_var_上下文;
  r26 = cn_var_语句;
  检查中断语句(r25, r26);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2560:
  r27 = cn_var_上下文;
  r28 = cn_var_语句;
  检查继续语句(r27, r28);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2561:
  r29 = cn_var_上下文;
  r30 = cn_var_语句;
  检查选择语句(r29, r30);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2562:
  r31 = cn_var_上下文;
  r32 = cn_var_语句;
  r33 = r32->表达式;
  r34 = 检查表达式(r31, r33);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2563:
  r35 = cn_var_上下文;
  r36 = cn_var_语句;
  检查尝试语句(r35, r36);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_body_2564:
  r37 = cn_var_上下文;
  r38 = cn_var_语句;
  检查抛出语句(r37, r38);
  goto switch_merge_2553;
  goto switch_merge_2553;

  case_default_2565:
  goto switch_merge_2553;
  goto switch_merge_2553;

  switch_merge_2553:
  return;
}

void 检查块语句(struct 语义检查上下文* cn_var_上下文, struct 块语句* cn_var_块节点) {
  long long r1, r2, r3, r4, r6, r7, r9, r10;
  struct 块语句* r0;
  struct 语义检查上下文* r5;
  void* r8;

  entry:
  r0 = cn_var_块节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2576; else goto if_merge_2577;

  if_then_2576:
  return;
  goto if_merge_2577;

  if_merge_2577:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2578;

  for_cond_2578:
  r2 = cn_var_i_0;
  r3 = cn_var_块节点->语句个数;
  r4 = r2 < r3;
  if (r4) goto for_body_2579; else goto for_exit_2581;

  for_body_2579:
  r5 = cn_var_上下文;
  r6 = cn_var_块节点->语句列表;
  r7 = cn_var_i_0;
  r8 = (void*)cn_rt_array_get_element(r6, r7, 8);
  检查语句(r5, r8);
  goto for_update_2580;

  for_update_2580:
  r9 = cn_var_i_0;
  r10 = r9 + 1;
  cn_var_i_0 = r10;
  goto for_cond_2578;

  for_exit_2581:
  return;
}

void 检查如果语句(struct 语义检查上下文* cn_var_上下文, struct 如果语句* cn_var_如果节点) {
  long long r1, r5, r6, r7, r8, r9, r12, r13, r15, r19;
  struct 如果语句* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r10;
  struct 表达式节点* r11;
  struct 块语句* r14;
  struct 语义检查上下文* r16;
  struct 块语句* r17;
  struct 语句节点* r18;
  struct 语义检查上下文* r20;
  struct 语句节点* r21;

  entry:
  r0 = cn_var_如果节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2582; else goto if_merge_2583;

  if_then_2582:
  return;
  goto if_merge_2583;

  if_merge_2583:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_如果节点->条件;
  r4 = 检查表达式(r2, r3);
  cn_var_条件类型_0 = r4;
  r6 = cn_var_条件类型_0;
  r7 = r6 != 0;
  if (r7) goto logic_rhs_2586; else goto logic_merge_2587;

  if_then_2584:
  r10 = cn_var_上下文;
  r11 = cn_var_如果节点->条件;
  r12 = r11->节点基类;
  r13 = r12.位置;
  报告错误(r10, r13, 诊断错误码_语义_类型不匹配);
  goto if_merge_2585;

  if_merge_2585:
  r14 = cn_var_如果节点->真分支;
  r15 = r14 != 0;
  if (r15) goto if_then_2588; else goto if_merge_2589;

  logic_rhs_2586:
  r8 = cn_var_条件类型_0.种类;
  r9 = r8 != /* NONE */;
  goto logic_merge_2587;

  logic_merge_2587:
  if (r9) goto if_then_2584; else goto if_merge_2585;

  if_then_2588:
  r16 = cn_var_上下文;
  r17 = cn_var_如果节点->真分支;
  检查语句(r16, r17);
  goto if_merge_2589;

  if_merge_2589:
  r18 = cn_var_如果节点->假分支;
  r19 = r18 != 0;
  if (r19) goto if_then_2590; else goto if_merge_2591;

  if_then_2590:
  r20 = cn_var_上下文;
  r21 = cn_var_如果节点->假分支;
  检查语句(r20, r21);
  goto if_merge_2591;

  if_merge_2591:
  return;
}

void 检查当语句(struct 语义检查上下文* cn_var_上下文, struct 当语句* cn_var_当节点) {
  long long r1, r5, r6, r7, r8, r9, r12, r13, r14, r15, r17, r20, r21;
  struct 当语句* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r10;
  struct 表达式节点* r11;
  struct 块语句* r16;
  struct 语义检查上下文* r18;
  struct 块语句* r19;

  entry:
  r0 = cn_var_当节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2592; else goto if_merge_2593;

  if_then_2592:
  return;
  goto if_merge_2593;

  if_merge_2593:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_当节点->条件;
  r4 = 检查表达式(r2, r3);
  cn_var_条件类型_0 = r4;
  r6 = cn_var_条件类型_0;
  r7 = r6 != 0;
  if (r7) goto logic_rhs_2596; else goto logic_merge_2597;

  if_then_2594:
  r10 = cn_var_上下文;
  r11 = cn_var_当节点->条件;
  r12 = r11->节点基类;
  r13 = r12.位置;
  报告错误(r10, r13, 诊断错误码_语义_类型不匹配);
  goto if_merge_2595;

  if_merge_2595:
  r14 = cn_var_上下文->循环深度;
  r15 = r14 + 1;
  r16 = cn_var_当节点->循环体;
  r17 = r16 != 0;
  if (r17) goto if_then_2598; else goto if_merge_2599;

  logic_rhs_2596:
  r8 = cn_var_条件类型_0.种类;
  r9 = r8 != /* NONE */;
  goto logic_merge_2597;

  logic_merge_2597:
  if (r9) goto if_then_2594; else goto if_merge_2595;

  if_then_2598:
  r18 = cn_var_上下文;
  r19 = cn_var_当节点->循环体;
  检查语句(r18, r19);
  goto if_merge_2599;

  if_merge_2599:
  r20 = cn_var_上下文->循环深度;
  r21 = r20 - 1;
  return;
}

void 检查循环语句(struct 语义检查上下文* cn_var_上下文, struct 循环语句* cn_var_循环节点) {
  long long r1, r3, r7, r11, r12, r13, r14, r15, r18, r19, r21, r25, r26, r28, r31, r32;
  struct 循环语句* r0;
  struct 语句节点* r2;
  struct 语义检查上下文* r4;
  struct 语句节点* r5;
  struct 表达式节点* r6;
  struct 语义检查上下文* r8;
  struct 表达式节点* r9;
  struct 类型信息* r10;
  struct 语义检查上下文* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 语义检查上下文* r22;
  struct 表达式节点* r23;
  struct 类型信息* r24;
  struct 块语句* r27;
  struct 语义检查上下文* r29;
  struct 块语句* r30;

  entry:
  r0 = cn_var_循环节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2600; else goto if_merge_2601;

  if_then_2600:
  return;
  goto if_merge_2601;

  if_merge_2601:
  r2 = cn_var_循环节点->初始化;
  r3 = r2 != 0;
  if (r3) goto if_then_2602; else goto if_merge_2603;

  if_then_2602:
  r4 = cn_var_上下文;
  r5 = cn_var_循环节点->初始化;
  检查语句(r4, r5);
  goto if_merge_2603;

  if_merge_2603:
  r6 = cn_var_循环节点->条件;
  r7 = r6 != 0;
  if (r7) goto if_then_2604; else goto if_merge_2605;

  if_then_2604:
  long long cn_var_条件类型_0;
  r8 = cn_var_上下文;
  r9 = cn_var_循环节点->条件;
  r10 = 检查表达式(r8, r9);
  cn_var_条件类型_0 = r10;
  r12 = cn_var_条件类型_0;
  r13 = r12 != 0;
  if (r13) goto logic_rhs_2608; else goto logic_merge_2609;

  if_merge_2605:
  r20 = cn_var_循环节点->更新;
  r21 = r20 != 0;
  if (r21) goto if_then_2610; else goto if_merge_2611;

  if_then_2606:
  r16 = cn_var_上下文;
  r17 = cn_var_循环节点->条件;
  r18 = r17->节点基类;
  r19 = r18.位置;
  报告错误(r16, r19, 诊断错误码_语义_类型不匹配);
  goto if_merge_2607;

  if_merge_2607:
  goto if_merge_2605;

  logic_rhs_2608:
  r14 = cn_var_条件类型_0.种类;
  r15 = r14 != /* NONE */;
  goto logic_merge_2609;

  logic_merge_2609:
  if (r15) goto if_then_2606; else goto if_merge_2607;

  if_then_2610:
  r22 = cn_var_上下文;
  r23 = cn_var_循环节点->更新;
  r24 = 检查表达式(r22, r23);
  goto if_merge_2611;

  if_merge_2611:
  r25 = cn_var_上下文->循环深度;
  r26 = r25 + 1;
  r27 = cn_var_循环节点->循环体;
  r28 = r27 != 0;
  if (r28) goto if_then_2612; else goto if_merge_2613;

  if_then_2612:
  r29 = cn_var_上下文;
  r30 = cn_var_循环节点->循环体;
  检查语句(r29, r30);
  goto if_merge_2613;

  if_merge_2613:
  r31 = cn_var_上下文->循环深度;
  r32 = r31 - 1;
  return;
}

void 检查返回语句(struct 语义检查上下文* cn_var_上下文, struct 返回语句* cn_var_返回节点) {
  long long r1, r3, r8, r12, r14, r15, r16, r17, r20, r21, r25, r27, r30;
  struct 返回语句* r0;
  struct 符号* r2;
  struct 语义检查上下文* r4;
  struct 表达式节点* r7;
  struct 语义检查上下文* r9;
  struct 表达式节点* r10;
  struct 类型信息* r11;
  struct 类型信息* r13;
  struct 类型信息* r18;
  struct 语义检查上下文* r22;
  struct 类型信息* r26;
  struct 类型信息* r28;
  struct 语义检查上下文* r31;
  struct AST节点 r5;
  struct 源位置 r6;
  enum 类型兼容性 r19;
  struct AST节点 r23;
  struct 源位置 r24;
  struct AST节点 r32;
  struct 源位置 r33;
  enum 类型种类 r29;

  entry:
  r0 = cn_var_返回节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2614; else goto if_merge_2615;

  if_then_2614:
  return;
  goto if_merge_2615;

  if_merge_2615:
  r2 = cn_var_上下文->当前函数;
  r3 = r2 == 0;
  if (r3) goto if_then_2616; else goto if_merge_2617;

  if_then_2616:
  r4 = cn_var_上下文;
  r5 = cn_var_返回节点->节点基类;
  r6 = r5.位置;
  报告错误(r4, r6, 诊断错误码_语义_返回语句在函数外);
  return;
  goto if_merge_2617;

  if_merge_2617:
  r7 = cn_var_返回节点->返回值;
  r8 = r7 != 0;
  if (r8) goto if_then_2618; else goto if_else_2619;

  if_then_2618:
  long long cn_var_返回值类型_0;
  r9 = cn_var_上下文;
  r10 = cn_var_返回节点->返回值;
  r11 = 检查表达式(r9, r10);
  cn_var_返回值类型_0 = r11;
  r13 = cn_var_上下文->期望返回类型;
  r14 = r13 != 0;
  if (r14) goto logic_rhs_2623; else goto logic_merge_2624;

  if_else_2619:
  r26 = cn_var_上下文->期望返回类型;
  r27 = r26 != 0;
  if (r27) goto logic_rhs_2629; else goto logic_merge_2630;

  if_merge_2620:

  if_then_2621:
  long long cn_var_兼容性_1;
  r17 = cn_var_返回值类型_0;
  r18 = cn_var_上下文->期望返回类型;
  r19 = 检查类型兼容性(r17, r18);
  cn_var_兼容性_1 = r19;
  r20 = cn_var_兼容性_1;
  r21 = r20 == 类型兼容性_不兼容;
  if (r21) goto if_then_2625; else goto if_merge_2626;

  if_merge_2622:
  goto if_merge_2620;

  logic_rhs_2623:
  r15 = cn_var_返回值类型_0;
  r16 = r15 != 0;
  goto logic_merge_2624;

  logic_merge_2624:
  if (r16) goto if_then_2621; else goto if_merge_2622;

  if_then_2625:
  r22 = cn_var_上下文;
  r23 = cn_var_返回节点->节点基类;
  r24 = r23.位置;
  报告错误(r22, r24, 诊断错误码_语义_类型不匹配);
  goto if_merge_2626;

  if_merge_2626:
  goto if_merge_2622;

  if_then_2627:
  r31 = cn_var_上下文;
  r32 = cn_var_返回节点->节点基类;
  r33 = r32.位置;
  报告错误(r31, r33, 诊断错误码_语义_类型不匹配);
  goto if_merge_2628;

  if_merge_2628:
  goto if_merge_2620;

  logic_rhs_2629:
  r28 = cn_var_上下文->期望返回类型;
  r29 = r28->种类;
  r30 = r29 != 类型种类_类型_空;
  goto logic_merge_2630;

  logic_merge_2630:
  if (r30) goto if_then_2627; else goto if_merge_2628;
  return;
}

void 检查中断语句(struct 语义检查上下文* cn_var_上下文, struct 中断语句* cn_var_中断节点) {
  long long r1, r2, r3;
  struct 中断语句* r0;
  struct 语义检查上下文* r4;
  struct AST节点 r5;
  struct 源位置 r6;

  entry:
  r0 = cn_var_中断节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2631; else goto if_merge_2632;

  if_then_2631:
  return;
  goto if_merge_2632;

  if_merge_2632:
  r2 = cn_var_上下文->循环深度;
  r3 = r2 == 0;
  if (r3) goto if_then_2633; else goto if_merge_2634;

  if_then_2633:
  r4 = cn_var_上下文;
  r5 = cn_var_中断节点->节点基类;
  r6 = r5.位置;
  报告错误(r4, r6, 诊断错误码_语义_中断继续在循环外);
  goto if_merge_2634;

  if_merge_2634:
  return;
}

void 检查继续语句(struct 语义检查上下文* cn_var_上下文, struct 继续语句* cn_var_继续节点) {
  long long r1, r2, r3;
  struct 继续语句* r0;
  struct 语义检查上下文* r4;
  struct AST节点 r5;
  struct 源位置 r6;

  entry:
  r0 = cn_var_继续节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2635; else goto if_merge_2636;

  if_then_2635:
  return;
  goto if_merge_2636;

  if_merge_2636:
  r2 = cn_var_上下文->循环深度;
  r3 = r2 == 0;
  if (r3) goto if_then_2637; else goto if_merge_2638;

  if_then_2637:
  r4 = cn_var_上下文;
  r5 = cn_var_继续节点->节点基类;
  r6 = r5.位置;
  报告错误(r4, r6, 诊断错误码_语义_中断继续在循环外);
  goto if_merge_2638;

  if_merge_2638:
  return;
}

void 检查选择语句(struct 语义检查上下文* cn_var_上下文, struct 选择语句* cn_var_选择节点) {
  long long r1, r5, r6, r7, r8, r9, r11, r12, r13, r16, r17, r18, r19, r20, r23, r25, r26, r28;
  struct 情况分支** r22;
  struct 选择语句* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r14;
  struct 表达式节点* r15;
  struct 语义检查上下文* r21;
  struct 情况分支* r24;
  struct 块语句* r27;
  struct 语义检查上下文* r29;
  struct 块语句* r30;
  _Bool r10;

  entry:
  r0 = cn_var_选择节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2639; else goto if_merge_2640;

  if_then_2639:
  return;
  goto if_merge_2640;

  if_merge_2640:
  long long cn_var_选择类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_选择节点->选择值;
  r4 = 检查表达式(r2, r3);
  cn_var_选择类型_0 = r4;
  r7 = cn_var_选择类型_0;
  r8 = r7 != 0;
  if (r8) goto logic_rhs_2645; else goto logic_merge_2646;

  if_then_2641:
  r14 = cn_var_上下文;
  r15 = cn_var_选择节点->选择值;
  r16 = r15->节点基类;
  r17 = r16.位置;
  报告错误(r14, r17, 诊断错误码_语义_类型不匹配);
  goto if_merge_2642;

  if_merge_2642:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2647;

  logic_rhs_2643:
  r12 = cn_var_选择类型_0.种类;
  r13 = r12 != /* NONE */;
  goto logic_merge_2644;

  logic_merge_2644:
  if (r13) goto if_then_2641; else goto if_merge_2642;

  logic_rhs_2645:
  r9 = cn_var_选择类型_0;
  r10 = 是整数类型(r9);
  r11 = r10 == 0;
  goto logic_merge_2646;

  logic_merge_2646:
  if (r11) goto logic_rhs_2643; else goto logic_merge_2644;

  for_cond_2647:
  r18 = cn_var_i_1;
  r19 = cn_var_选择节点->情况个数;
  r20 = r18 < r19;
  if (r20) goto for_body_2648; else goto for_exit_2650;

  for_body_2648:
  r21 = cn_var_上下文;
  r22 = cn_var_选择节点->情况分支列表;
  r23 = cn_var_i_1;
  r24 = (struct 情况分支*[] 情况分支列表;   
    整数 情况个数;          
    块语句* 默认分支;       
}






结构体 捕获子句 {
    字符串 异常类型;        
    字符串 变量名;          
    块语句* 语句体;         
}






结构体 尝试语句 {
    AST节点 节点基类;       
    块语句* 尝试块;         
    捕获子句*[] 捕获子句列表;   
    整数 捕获个数;          
    块语句* 最终块;         
}






结构体 抛出语句 {
    AST节点 节点基类;       
    表达式节点* 异常表达式; 
    字符串 异常类型;        
    字符串 消息;            
}






结构体 最终语句 {
    AST节点 节点基类;       
    块语句* 语句体;         
}






结构体 语句节点 {
    节点类型 类型;          
    源位置 位置;            
}
*)cn_rt_array_get_element(r22, r23, 8);
  检查情况分支(r21, r24);
  goto for_update_2649;

  for_update_2649:
  r25 = cn_var_i_1;
  r26 = r25 + 1;
  cn_var_i_1 = r26;
  goto for_cond_2647;

  for_exit_2650:
  r27 = cn_var_选择节点->默认分支;
  r28 = r27 != 0;
  if (r28) goto if_then_2651; else goto if_merge_2652;

  if_then_2651:
  r29 = cn_var_上下文;
  r30 = cn_var_选择节点->默认分支;
  检查语句(r29, r30);
  goto if_merge_2652;

  if_merge_2652:
  return;
}

void 检查情况分支(struct 语义检查上下文* cn_var_上下文, struct 情况分支* cn_var_情况节点) {
  long long r1, r3, r6, r9, r10, r12;
  struct 情况分支* r0;
  struct 表达式节点* r2;
  struct 表达式节点* r4;
  struct 语义检查上下文* r7;
  struct 表达式节点* r8;
  struct 块语句* r11;
  struct 语义检查上下文* r13;
  struct 块语句* r14;
  _Bool r5;

  entry:
  r0 = cn_var_情况节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2653; else goto if_merge_2654;

  if_then_2653:
  return;
  goto if_merge_2654;

  if_merge_2654:
  r2 = cn_var_情况节点->匹配值;
  r3 = r2 != 0;
  if (r3) goto if_then_2655; else goto if_merge_2656;

  if_then_2655:
  r4 = cn_var_情况节点->匹配值;
  r5 = 是常量表达式(r4);
  r6 = r5 == 0;
  if (r6) goto if_then_2657; else goto if_merge_2658;

  if_merge_2656:
  r11 = cn_var_情况节点->语句体;
  r12 = r11 != 0;
  if (r12) goto if_then_2659; else goto if_merge_2660;

  if_then_2657:
  r7 = cn_var_上下文;
  r8 = cn_var_情况节点->匹配值;
  r9 = r8->节点基类;
  r10 = r9.位置;
  报告错误(r7, r10, 诊断错误码_语义_开关非常量情况);
  goto if_merge_2658;

  if_merge_2658:
  goto if_merge_2656;

  if_then_2659:
  r13 = cn_var_上下文;
  r14 = cn_var_情况节点->语句体;
  检查块语句(r13, r14);
  goto if_merge_2660;

  if_merge_2660:
  return;
}

void 检查尝试语句(struct 语义检查上下文* cn_var_上下文, struct 尝试语句* cn_var_尝试节点) {
  long long r1, r3, r6, r7, r8, r11, r13, r14, r16;
  struct 捕获子句** r10;
  struct 尝试语句* r0;
  struct 块语句* r2;
  struct 语义检查上下文* r4;
  struct 块语句* r5;
  struct 语义检查上下文* r9;
  struct 捕获子句* r12;
  struct 块语句* r15;
  struct 语义检查上下文* r17;
  struct 块语句* r18;

  entry:
  r0 = cn_var_尝试节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2661; else goto if_merge_2662;

  if_then_2661:
  return;
  goto if_merge_2662;

  if_merge_2662:
  r2 = cn_var_尝试节点->尝试块;
  r3 = r2 != 0;
  if (r3) goto if_then_2663; else goto if_merge_2664;

  if_then_2663:
  r4 = cn_var_上下文;
  r5 = cn_var_尝试节点->尝试块;
  检查语句(r4, r5);
  goto if_merge_2664;

  if_merge_2664:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2665;

  for_cond_2665:
  r6 = cn_var_i_0;
  r7 = cn_var_尝试节点->捕获个数;
  r8 = r6 < r7;
  if (r8) goto for_body_2666; else goto for_exit_2668;

  for_body_2666:
  r9 = cn_var_上下文;
  r10 = cn_var_尝试节点->捕获子句列表;
  r11 = cn_var_i_0;
  r12 = (struct 捕获子句*[] 捕获子句列表;   
    整数 捕获个数;          
    块语句* 最终块;         
}






结构体 抛出语句 {
    AST节点 节点基类;       
    表达式节点* 异常表达式; 
    字符串 异常类型;        
    字符串 消息;            
}






结构体 最终语句 {
    AST节点 节点基类;       
    块语句* 语句体;         
}






结构体 语句节点 {
    节点类型 类型;          
    源位置 位置;            
}
*)cn_rt_array_get_element(r10, r11, 8);
  检查捕获子句(r9, r12);
  goto for_update_2667;

  for_update_2667:
  r13 = cn_var_i_0;
  r14 = r13 + 1;
  cn_var_i_0 = r14;
  goto for_cond_2665;

  for_exit_2668:
  r15 = cn_var_尝试节点->最终块;
  r16 = r15 != 0;
  if (r16) goto if_then_2669; else goto if_merge_2670;

  if_then_2669:
  r17 = cn_var_上下文;
  r18 = cn_var_尝试节点->最终块;
  检查语句(r17, r18);
  goto if_merge_2670;

  if_merge_2670:
  return;
}

void 检查捕获子句(struct 语义检查上下文* cn_var_上下文, struct 捕获子句* cn_var_捕获节点) {
  long long r1, r3;
  struct 捕获子句* r0;
  struct 块语句* r2;
  struct 语义检查上下文* r4;
  struct 块语句* r5;

  entry:
  r0 = cn_var_捕获节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2671; else goto if_merge_2672;

  if_then_2671:
  return;
  goto if_merge_2672;

  if_merge_2672:
  r2 = cn_var_捕获节点->语句体;
  r3 = r2 != 0;
  if (r3) goto if_then_2673; else goto if_merge_2674;

  if_then_2673:
  r4 = cn_var_上下文;
  r5 = cn_var_捕获节点->语句体;
  检查块语句(r4, r5);
  goto if_merge_2674;

  if_merge_2674:
  return;
}

void 检查抛出语句(struct 语义检查上下文* cn_var_上下文, struct 抛出语句* cn_var_抛出节点) {
  long long r1, r2, r3, r5;
  struct 抛出语句* r0;
  struct 语义检查上下文* r4;
  struct 类型信息* r6;

  entry:
  r0 = cn_var_抛出节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2675; else goto if_merge_2676;

  if_then_2675:
  return;
  goto if_merge_2676;

  if_merge_2676:
  r2 = cn_var_抛出节点->抛出值;
  r3 = r2 != 0;
  if (r3) goto if_then_2677; else goto if_merge_2678;

  if_then_2677:
  r4 = cn_var_上下文;
  r5 = cn_var_抛出节点->抛出值;
  r6 = 检查表达式(r4, r5);
  goto if_merge_2678;

  if_merge_2678:
  return;
}

struct 类型信息* 检查表达式(struct 语义检查上下文* cn_var_上下文, struct 表达式节点* cn_var_表达式) {
  long long r1, r2, r3, r40, r41, r42, r44;
  struct 表达式节点* r0;
  struct 语义检查上下文* r13;
  struct 二元表达式* r14;
  struct 类型信息* r15;
  struct 语义检查上下文* r16;
  struct 一元表达式* r17;
  struct 类型信息* r18;
  struct 语义检查上下文* r19;
  struct 字面量表达式* r20;
  struct 类型信息* r21;
  struct 语义检查上下文* r22;
  struct 标识符表达式* r23;
  struct 类型信息* r24;
  struct 语义检查上下文* r25;
  struct 函数调用表达式* r26;
  struct 类型信息* r27;
  struct 语义检查上下文* r28;
  struct 成员访问表达式* r29;
  struct 类型信息* r30;
  struct 语义检查上下文* r31;
  struct 数组访问表达式* r32;
  struct 类型信息* r33;
  struct 语义检查上下文* r34;
  struct 赋值表达式* r35;
  struct 类型信息* r36;
  struct 语义检查上下文* r37;
  struct 三元表达式* r38;
  struct 类型信息* r39;
  struct 类型节点* r43;
  _Bool r4;
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
  if (r1) goto if_then_2679; else goto if_merge_2680;

  if_then_2679:
  return 0;
  goto if_merge_2680;

  if_merge_2680:
  long long cn_var_结果类型_0;
  cn_var_结果类型_0 = 0;
  r2 = cn_var_表达式->节点基类;
  r3 = r2.类型;
  r4 = r3 == 节点类型_二元表达式;
  if (r4) goto case_body_2682; else goto switch_check_2692;

  switch_check_2692:
  r5 = r3 == 节点类型_一元表达式;
  if (r5) goto case_body_2683; else goto switch_check_2693;

  switch_check_2693:
  r6 = r3 == 节点类型_字面量表达式;
  if (r6) goto case_body_2684; else goto switch_check_2694;

  switch_check_2694:
  r7 = r3 == 节点类型_标识符表达式;
  if (r7) goto case_body_2685; else goto switch_check_2695;

  switch_check_2695:
  r8 = r3 == 节点类型_函数调用表达式;
  if (r8) goto case_body_2686; else goto switch_check_2696;

  switch_check_2696:
  r9 = r3 == 节点类型_成员访问表达式;
  if (r9) goto case_body_2687; else goto switch_check_2697;

  switch_check_2697:
  r10 = r3 == 节点类型_数组访问表达式;
  if (r10) goto case_body_2688; else goto switch_check_2698;

  switch_check_2698:
  r11 = r3 == 节点类型_赋值表达式;
  if (r11) goto case_body_2689; else goto switch_check_2699;

  switch_check_2699:
  r12 = r3 == 节点类型_三元表达式;
  if (r12) goto case_body_2690; else goto case_default_2691;

  case_body_2682:
  r13 = cn_var_上下文;
  r14 = cn_var_表达式;
  r15 = 检查二元表达式(r13, r14);
  cn_var_结果类型_0 = r15;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2683:
  r16 = cn_var_上下文;
  r17 = cn_var_表达式;
  r18 = 检查一元表达式(r16, r17);
  cn_var_结果类型_0 = r18;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2684:
  r19 = cn_var_上下文;
  r20 = cn_var_表达式;
  r21 = 检查字面量表达式(r19, r20);
  cn_var_结果类型_0 = r21;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2685:
  r22 = cn_var_上下文;
  r23 = cn_var_表达式;
  r24 = 检查标识符表达式(r22, r23);
  cn_var_结果类型_0 = r24;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2686:
  r25 = cn_var_上下文;
  r26 = cn_var_表达式;
  r27 = 检查函数调用表达式(r25, r26);
  cn_var_结果类型_0 = r27;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2687:
  r28 = cn_var_上下文;
  r29 = cn_var_表达式;
  r30 = 检查成员访问表达式(r28, r29);
  cn_var_结果类型_0 = r30;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2688:
  r31 = cn_var_上下文;
  r32 = cn_var_表达式;
  r33 = 检查数组访问表达式(r31, r32);
  cn_var_结果类型_0 = r33;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2689:
  r34 = cn_var_上下文;
  r35 = cn_var_表达式;
  r36 = 检查赋值表达式(r34, r35);
  cn_var_结果类型_0 = r36;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_body_2690:
  r37 = cn_var_上下文;
  r38 = cn_var_表达式;
  r39 = 检查三元表达式(r37, r38);
  cn_var_结果类型_0 = r39;
  goto switch_merge_2681;
  goto switch_merge_2681;

  case_default_2691:
  cn_var_结果类型_0 = 0;
  goto switch_merge_2681;
  goto switch_merge_2681;

  switch_merge_2681:
  r40 = cn_var_结果类型_0;
  r41 = r40 != 0;
  if (r41) goto if_then_2700; else goto if_merge_2701;

  if_then_2700:
  r42 = cn_var_结果类型_0;
  r43 = 类型信息转类型节点(r42);
  goto if_merge_2701;

  if_merge_2701:
  r44 = cn_var_结果类型_0;
  return r44;
}

struct 类型信息* 检查二元表达式(struct 语义检查上下文* cn_var_上下文, struct 二元表达式* cn_var_二元节点) {
  long long r1, r25, r26, r28, r30, r35, r36, r38, r40;
  struct 二元表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r5;
  struct 表达式节点* r6;
  struct 类型信息* r7;
  struct 语义检查上下文* r31;
  struct 类型信息* r34;
  struct 语义检查上下文* r41;
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
  _Bool r27;
  _Bool r29;
  _Bool r37;
  _Bool r39;
  struct AST节点 r32;
  struct 源位置 r33;
  struct AST节点 r42;
  struct 源位置 r43;
  enum 二元运算符 r8;

  entry:
  r0 = cn_var_二元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2702; else goto if_merge_2703;

  if_then_2702:
  return 0;
  goto if_merge_2703;

  if_merge_2703:
  long long cn_var_左类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_二元节点->左操作数;
  r4 = 检查表达式(r2, r3);
  cn_var_左类型_0 = r4;
  long long cn_var_右类型_1;
  r5 = cn_var_上下文;
  r6 = cn_var_二元节点->右操作数;
  r7 = 检查表达式(r5, r6);
  cn_var_右类型_1 = r7;
  r8 = cn_var_二元节点->运算符;
  r9 = r8 == 二元运算符_二元_加;
  if (r9) goto case_body_2705; else goto switch_check_2722;

  switch_check_2722:
  r10 = r8 == 二元运算符_二元_减;
  if (r10) goto case_body_2706; else goto switch_check_2723;

  switch_check_2723:
  r11 = r8 == 二元运算符_二元_乘;
  if (r11) goto case_body_2707; else goto switch_check_2724;

  switch_check_2724:
  r12 = r8 == 二元运算符_二元_除;
  if (r12) goto case_body_2708; else goto switch_check_2725;

  switch_check_2725:
  r13 = r8 == 二元运算符_二元_取模;
  if (r13) goto case_body_2709; else goto switch_check_2726;

  switch_check_2726:
  r14 = r8 == 二元运算符_二元_等于;
  if (r14) goto case_body_2710; else goto switch_check_2727;

  switch_check_2727:
  r15 = r8 == 二元运算符_二元_不等于;
  if (r15) goto case_body_2711; else goto switch_check_2728;

  switch_check_2728:
  r16 = r8 == 二元运算符_二元_小于;
  if (r16) goto case_body_2712; else goto switch_check_2729;

  switch_check_2729:
  r17 = r8 == 二元运算符_二元_小于等于;
  if (r17) goto case_body_2713; else goto switch_check_2730;

  switch_check_2730:
  r18 = r8 == 二元运算符_二元_大于;
  if (r18) goto case_body_2714; else goto switch_check_2731;

  switch_check_2731:
  r19 = r8 == 二元运算符_二元_大于等于;
  if (r19) goto case_body_2715; else goto switch_check_2732;

  switch_check_2732:
  r20 = r8 == 二元运算符_二元_位与;
  if (r20) goto case_body_2716; else goto switch_check_2733;

  switch_check_2733:
  r21 = r8 == 二元运算符_二元_位或;
  if (r21) goto case_body_2717; else goto switch_check_2734;

  switch_check_2734:
  r22 = r8 == 二元运算符_二元_位异或;
  if (r22) goto case_body_2718; else goto switch_check_2735;

  switch_check_2735:
  r23 = r8 == 二元运算符_二元_左移;
  if (r23) goto case_body_2719; else goto switch_check_2736;

  switch_check_2736:
  r24 = r8 == 二元运算符_二元_右移;
  if (r24) goto case_body_2720; else goto case_default_2721;

  case_body_2705:
  goto switch_merge_2704;

  case_body_2706:
  goto switch_merge_2704;

  case_body_2707:
  goto switch_merge_2704;

  case_body_2708:
  goto switch_merge_2704;

  case_body_2709:
  r26 = cn_var_左类型_0;
  r27 = 是数值类型(r26);
  if (r27) goto logic_rhs_2739; else goto logic_merge_2740;

  if_then_2737:
  r30 = cn_var_左类型_0;
  return r30;
  goto if_merge_2738;

  if_merge_2738:
  r31 = cn_var_上下文;
  r32 = cn_var_二元节点->节点基类;
  r33 = r32.位置;
  报告错误(r31, r33, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2704;

  logic_rhs_2739:
  r28 = cn_var_右类型_1;
  r29 = 是数值类型(r28);
  goto logic_merge_2740;

  logic_merge_2740:
  if (r29) goto if_then_2737; else goto if_merge_2738;

  case_body_2710:
  goto switch_merge_2704;

  case_body_2711:
  goto switch_merge_2704;

  case_body_2712:
  goto switch_merge_2704;

  case_body_2713:
  goto switch_merge_2704;

  case_body_2714:
  goto switch_merge_2704;

  case_body_2715:
  r34 = 创建布尔类型();
  return r34;
  goto switch_merge_2704;

  case_body_2716:
  goto switch_merge_2704;

  case_body_2717:
  goto switch_merge_2704;

  case_body_2718:
  goto switch_merge_2704;

  case_body_2719:
  goto switch_merge_2704;

  case_body_2720:
  r36 = cn_var_左类型_0;
  r37 = 是整数类型(r36);
  if (r37) goto logic_rhs_2743; else goto logic_merge_2744;

  if_then_2741:
  r40 = cn_var_左类型_0;
  return r40;
  goto if_merge_2742;

  if_merge_2742:
  r41 = cn_var_上下文;
  r42 = cn_var_二元节点->节点基类;
  r43 = r42.位置;
  报告错误(r41, r43, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2704;

  logic_rhs_2743:
  r38 = cn_var_右类型_1;
  r39 = 是整数类型(r38);
  goto logic_merge_2744;

  logic_merge_2744:
  if (r39) goto if_then_2741; else goto if_merge_2742;

  case_default_2721:
  return 0;
  goto switch_merge_2704;

  switch_merge_2704:
  return NULL;
}

struct 类型信息* 检查一元表达式(struct 语义检查上下文* cn_var_上下文, struct 一元表达式* cn_var_一元节点) {
  long long r1, r15, r17, r22, r24, r28, r30, r32, r36, r38;
  struct 一元表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r18;
  struct 类型信息* r21;
  struct 语义检查上下文* r25;
  struct 类型信息* r29;
  struct 语义检查上下文* r33;
  struct 语义检查上下文* r39;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r16;
  _Bool r23;
  _Bool r31;
  _Bool r37;
  struct AST节点 r19;
  struct 源位置 r20;
  struct AST节点 r26;
  struct 源位置 r27;
  struct AST节点 r34;
  struct 源位置 r35;
  struct AST节点 r40;
  struct 源位置 r41;
  enum 一元运算符 r5;

  entry:
  r0 = cn_var_一元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2745; else goto if_merge_2746;

  if_then_2745:
  return 0;
  goto if_merge_2746;

  if_merge_2746:
  long long cn_var_操作数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_一元节点->操作数;
  r4 = 检查表达式(r2, r3);
  cn_var_操作数类型_0 = r4;
  r5 = cn_var_一元节点->运算符;
  r6 = r5 == 一元运算符_一元_取负;
  if (r6) goto case_body_2748; else goto switch_check_2758;

  switch_check_2758:
  r7 = r5 == 一元运算符_一元_逻辑非;
  if (r7) goto case_body_2749; else goto switch_check_2759;

  switch_check_2759:
  r8 = r5 == 一元运算符_一元_位取反;
  if (r8) goto case_body_2750; else goto switch_check_2760;

  switch_check_2760:
  r9 = r5 == 一元运算符_一元_取地址;
  if (r9) goto case_body_2751; else goto switch_check_2761;

  switch_check_2761:
  r10 = r5 == 一元运算符_一元_解引用;
  if (r10) goto case_body_2752; else goto switch_check_2762;

  switch_check_2762:
  r11 = r5 == 一元运算符_一元_前置自增;
  if (r11) goto case_body_2753; else goto switch_check_2763;

  switch_check_2763:
  r12 = r5 == 一元运算符_一元_前置自减;
  if (r12) goto case_body_2754; else goto switch_check_2764;

  switch_check_2764:
  r13 = r5 == 一元运算符_一元_后置自增;
  if (r13) goto case_body_2755; else goto switch_check_2765;

  switch_check_2765:
  r14 = r5 == 一元运算符_一元_后置自减;
  if (r14) goto case_body_2756; else goto case_default_2757;

  case_body_2748:
  r15 = cn_var_操作数类型_0;
  r16 = 是数值类型(r15);
  if (r16) goto if_then_2766; else goto if_merge_2767;

  if_then_2766:
  r17 = cn_var_操作数类型_0;
  return r17;
  goto if_merge_2767;

  if_merge_2767:
  r18 = cn_var_上下文;
  r19 = cn_var_一元节点->节点基类;
  r20 = r19.位置;
  报告错误(r18, r20, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2747;

  case_body_2749:
  r21 = 创建布尔类型();
  return r21;
  goto switch_merge_2747;

  case_body_2750:
  r22 = cn_var_操作数类型_0;
  r23 = 是整数类型(r22);
  if (r23) goto if_then_2768; else goto if_merge_2769;

  if_then_2768:
  r24 = cn_var_操作数类型_0;
  return r24;
  goto if_merge_2769;

  if_merge_2769:
  r25 = cn_var_上下文;
  r26 = cn_var_一元节点->节点基类;
  r27 = r26.位置;
  报告错误(r25, r27, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2747;

  case_body_2751:
  r28 = cn_var_操作数类型_0;
  r29 = 创建指针类型(r28);
  return r29;
  goto switch_merge_2747;

  case_body_2752:
  r30 = cn_var_操作数类型_0;
  r31 = 是指针类型(r30);
  if (r31) goto if_then_2770; else goto if_merge_2771;

  if_then_2770:
  r32 = cn_var_操作数类型_0.指向类型;
  return r32;
  goto if_merge_2771;

  if_merge_2771:
  r33 = cn_var_上下文;
  r34 = cn_var_一元节点->节点基类;
  r35 = r34.位置;
  报告错误(r33, r35, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2747;

  case_body_2753:
  goto switch_merge_2747;

  case_body_2754:
  goto switch_merge_2747;

  case_body_2755:
  goto switch_merge_2747;

  case_body_2756:
  r36 = cn_var_操作数类型_0;
  r37 = 是数值类型(r36);
  if (r37) goto if_then_2772; else goto if_merge_2773;

  if_then_2772:
  r38 = cn_var_操作数类型_0;
  return r38;
  goto if_merge_2773;

  if_merge_2773:
  r39 = cn_var_上下文;
  r40 = cn_var_一元节点->节点基类;
  r41 = r40.位置;
  报告错误(r39, r41, 诊断错误码_语义_类型不匹配);
  return 0;
  goto switch_merge_2747;

  case_default_2757:
  return 0;
  goto switch_merge_2747;

  switch_merge_2747:
  return NULL;
}

struct 类型信息* 检查字面量表达式(struct 语义检查上下文* cn_var_上下文, struct 字面量表达式* cn_var_字面量节点) {
  long long r1, r8, r10;
  struct 字面量表达式* r0;
  struct 类型信息* r9;
  struct 类型信息* r11;
  struct 类型信息* r12;
  struct 类型信息* r13;
  struct 类型信息* r14;
  struct 类型信息* r15;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  enum 字面量类型 r2;

  entry:
  r0 = cn_var_字面量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2774; else goto if_merge_2775;

  if_then_2774:
  return 0;
  goto if_merge_2775;

  if_merge_2775:
  r2 = cn_var_字面量节点->类型;
  r3 = r2 == 字面量类型_字面量_整数;
  if (r3) goto case_body_2777; else goto switch_check_2783;

  switch_check_2783:
  r4 = r2 == 字面量类型_字面量_浮点;
  if (r4) goto case_body_2778; else goto switch_check_2784;

  switch_check_2784:
  r5 = r2 == 字面量类型_字面量_字符串;
  if (r5) goto case_body_2779; else goto switch_check_2785;

  switch_check_2785:
  r6 = r2 == 字面量类型_字面量_布尔;
  if (r6) goto case_body_2780; else goto switch_check_2786;

  switch_check_2786:
  r7 = r2 == 字面量类型_字面量_空;
  if (r7) goto case_body_2781; else goto case_default_2782;

  case_body_2777:
  r8 = cn_var_整数大小;
  r9 = 创建整数类型("整数", r8, 1);
  return r9;
  goto switch_merge_2776;

  case_body_2778:
  r10 = cn_var_小数大小;
  r11 = 创建小数类型("小数", r10);
  return r11;
  goto switch_merge_2776;

  case_body_2779:
  r12 = 创建字符串类型();
  return r12;
  goto switch_merge_2776;

  case_body_2780:
  r13 = 创建布尔类型();
  return r13;
  goto switch_merge_2776;

  case_body_2781:
  r14 = 创建空类型();
  r15 = 创建指针类型(r14);
  return r15;
  goto switch_merge_2776;

  case_default_2782:
  return 0;
  goto switch_merge_2776;

  switch_merge_2776:
  return NULL;
}

struct 类型信息* 检查标识符表达式(struct 语义检查上下文* cn_var_上下文, struct 标识符表达式* cn_var_标识符节点) {
  long long r1, r5, r6, r10, r11, r12;
  char* r3;
  struct 标识符表达式* r0;
  struct 符号表管理器* r2;
  struct 符号* r4;
  struct 语义检查上下文* r7;
  struct 类型推断上下文* r13;
  struct 类型信息* r14;
  struct AST节点 r8;
  struct 源位置 r9;

  entry:
  r0 = cn_var_标识符节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2787; else goto if_merge_2788;

  if_then_2787:
  return 0;
  goto if_merge_2788;

  if_merge_2788:
  long long cn_var_符号指针_0;
  r2 = cn_var_上下文->符号表;
  r3 = cn_var_标识符节点->名称;
  r4 = 查找符号(r2, r3);
  cn_var_符号指针_0 = r4;
  r5 = cn_var_符号指针_0;
  r6 = r5 == 0;
  if (r6) goto if_then_2789; else goto if_merge_2790;

  if_then_2789:
  r7 = cn_var_上下文;
  r8 = cn_var_标识符节点->节点基类;
  r9 = r8.位置;
  报告错误(r7, r9, 诊断错误码_语义_未定义标识符);
  return 0;
  goto if_merge_2790;

  if_merge_2790:
  r10 = cn_var_符号指针_0.类型信息;
  r11 = r10 != 0;
  if (r11) goto if_then_2791; else goto if_merge_2792;

  if_then_2791:
  r12 = cn_var_符号指针_0.类型信息;
  r13 = cn_var_上下文->类型上下文;
  r14 = 从类型节点推断(r12, r13);
  return r14;
  goto if_merge_2792;

  if_merge_2792:
  return 0;
}

struct 类型信息* 检查函数调用表达式(struct 语义检查上下文* cn_var_上下文, struct 函数调用表达式* cn_var_调用节点) {
  long long r1, r5, r7, r10, r14, r15, r16, r20, r21, r22, r23, r24, r25, r26, r29, r32, r33, r35, r36, r38, r39, r42, r44, r45, r46, r47, r48;
  struct 表达式节点** r28;
  struct 表达式节点** r41;
  struct 函数调用表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r11;
  struct 语义检查上下文* r17;
  struct 语义检查上下文* r27;
  struct 表达式节点* r30;
  struct 类型信息* r31;
  void* r34;
  struct 语义检查上下文* r40;
  struct 表达式节点* r43;
  _Bool r9;
  struct AST节点 r12;
  struct 源位置 r13;
  struct AST节点 r18;
  struct 源位置 r19;
  enum 类型兼容性 r37;
  enum 节点类型 r6;
  enum 节点类型 r8;

  entry:
  r0 = cn_var_调用节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2793; else goto if_merge_2794;

  if_then_2793:
  return 0;
  goto if_merge_2794;

  if_merge_2794:
  enum 节点类型 cn_var_函数类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_调用节点->被调函数;
  r4 = 检查表达式(r2, r3);
  cn_var_函数类型_0 = r4;
  r6 = cn_var_函数类型_0;
  r7 = r6 == 0;
  if (r7) goto logic_merge_2798; else goto logic_rhs_2797;

  if_then_2795:
  r11 = cn_var_上下文;
  r12 = cn_var_调用节点->节点基类;
  r13 = r12.位置;
  报告错误(r11, r13, 诊断错误码_语义_不可调用);
  return 0;
  goto if_merge_2796;

  if_merge_2796:
  r14 = cn_var_调用节点->参数个数;
  r15 = cn_var_函数类型_0.参数个数;
  r16 = r14 != r15;
  if (r16) goto if_then_2799; else goto if_merge_2800;

  logic_rhs_2797:
  r8 = cn_var_函数类型_0;
  r9 = 是可调用类型(r8);
  r10 = r9 == 0;
  goto logic_merge_2798;

  logic_merge_2798:
  if (r10) goto if_then_2795; else goto if_merge_2796;

  if_then_2799:
  r17 = cn_var_上下文;
  r18 = cn_var_调用节点->节点基类;
  r19 = r18.位置;
  报告错误(r17, r19, 诊断错误码_语义_参数数量不匹配);
  goto if_merge_2800;

  if_merge_2800:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2801;

  for_cond_2801:
  r21 = cn_var_i_1;
  r22 = cn_var_调用节点->参数个数;
  r23 = r21 < r22;
  if (r23) goto logic_rhs_2805; else goto logic_merge_2806;

  for_body_2802:
  long long cn_var_实参类型_2;
  r27 = cn_var_上下文;
  r28 = cn_var_调用节点->参数;
  r29 = cn_var_i_1;
  r30 = (struct 表达式节点*[] 参数;     
    整数 参数个数;          
}






结构体 成员访问表达式 {
    AST节点 节点基类;       
    表达式节点* 对象;       
    字符串 成员名;          
    布尔 是指针访问;        
    布尔 是静态成员;        
    字符串 类名;            
}






结构体 数组访问表达式 {
    AST节点 节点基类;       
    表达式节点* 数组;       
    表达式节点* 索引;       
}






结构体 赋值表达式 {
    AST节点 节点基类;       
    表达式节点* 目标;       
    表达式节点* 值;         
    赋值运算符 运算符;      
}






结构体 三元表达式 {
    AST节点 节点基类;       
    表达式节点* 条件;       
    表达式节点* 真值;       
    表达式节点* 假值;       
}






结构体 数组字面量表达式 {
    AST节点 节点基类;       
    表达式节点*[] 元素;     
    整数 元素个数;          
}






结构体 结构体字段初始化 {
    字符串 字段名;          
    表达式节点* 值;         
}






结构体 结构体字面量表达式 {
    AST节点 节点基类;       
    字符串 结构体名;        
    结构体字段初始化*[] 字段; 
    整数 字段个数;          
}






结构体 模板实例化表达式 {
    AST节点 节点基类;       
    字符串 模板名;          
    类型节点*[] 类型参数;   
    整数 参数个数;          
}







结构体 表达式节点 {
    节点类型 类型;          
    源位置 位置;            
    类型节点* 类型信息;     
    布尔 是自身指针;        
    
    
    整数 整数值;            
    小数 小数值;            
    字符串 字符串值;        
    布尔 布尔值;            
    
    
    字符串 标识符名称;      
    
    
    表达式节点* 左操作数;   
    表达式节点* 右操作数;   
    整数 运算符;            
    
    
    表达式节点* 操作数;     
    
    
    表达式节点* 被调函数;   
    表达式列表* 参数列表;   
    整数 参数个数;          
    
    
    表达式节点* 对象;       
    字符串 成员名;          
    整数 成员索引;          
    
    
    表达式节点* 数组;       
    表达式节点* 索引;       
    
    
    表达式节点* 左侧表达式; 
    表达式节点* 值;         
    
    
    赋值表达式 赋值表达式;   
    
    
    三元表达式 三元表达式;  
    
    
    表达式节点* 指针;       
    
    
    类型节点* 目标类型;     
    表达式节点* 转换操作数; 
}






结构体 表达式列表 {
    表达式节点* 节点;          
    表达式列表* 下一个;        
}








函数 创建表达式节点(节点类型 类型) -> 表达式节点* {
    
    表达式节点* 节点 = 分配内存(类型大小(表达式节点));
    如果 (节点 == 无) {
        返回 无;
    }
    
    
    节点.类型 = 类型;
    节点.位置.文件名 = "";
    节点.位置.行号 = 0;
    节点.位置.列号 = 0;
    节点.类型信息 = 无;
    节点.是自身指针 = 假;
    
    
    节点.整数值 = 0;
    节点.小数值 = 0.0;
    节点.字符串值 = "";
    节点.布尔值 = 假;
    节点.标识符名称 = "";
    节点.左操作数 = 无;
    节点.右操作数 = 无;
    节点.运算符 = 0;
    节点.操作数 = 无;
    节点.被调函数 = 无;
    节点.参数列表 = 无;
    节点.参数个数 = 0;
    节点.对象 = 无;
    节点.成员名 = "";
    节点.成员索引 = 0;
    节点.数组 = 无;
    节点.索引 = 无;
    节点.左侧表达式 = 无;
    节点.值 = 无;
    节点.指针 = 无;
    
    返回 节点;
}



函数 创建赋值表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.赋值表达式);
    返回 节点;
}



函数 创建二元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.二元表达式);
    返回 节点;
}



函数 创建三元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.三元表达式);
    返回 节点;
}



函数 创建一元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.一元表达式);
    返回 节点;
}



函数 创建函数调用表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.函数调用表达式);
    返回 节点;
}



函数 创建成员访问表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.成员访问表达式);
    返回 节点;
}



函数 创建数组访问表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.数组访问表达式);
    返回 节点;
}



函数 创建字面量表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.字面量表达式);
    返回 节点;
}



函数 创建标识符表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.标识符表达式);
    返回 节点;
}



函数 创建空值表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.字面量表达式);
    如果 (节点 != 无) {
        节点.布尔值 = 假;
    }
    返回 节点;
}



函数 创建自身表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.标识符表达式);
    如果 (节点 != 无) {
        节点.是自身指针 = 真;
        节点.标识符名称 = "自身";
    }
    返回 节点;
}




函数 创建基类访问表达式(字符串 成员名) -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.成员访问表达式);
    如果 (节点 != 无) {
        节点.成员名 = 成员名;
    }
    返回 节点;
}





函数 创建类型转换表达式(类型节点* 目标类型, 表达式节点* 操作数) -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.类型转换表达式);
    如果 (节点 != 无) {
        节点.目标类型 = 目标类型;
        节点.转换操作数 = 操作数;
        节点.类型信息 = 目标类型;  
    }
    返回 节点;
}



函数 创建表达式列表() -> 表达式列表* {
    表达式列表* 列表 = 分配内存(类型大小(表达式列表));
    如果 (列表 == 无) {
        返回 无;
    }
    
    列表.节点 = 无;
    列表.下一个 = 无;
    
    返回 列表;
}




函数 表达式列表添加(表达式列表* 列表, 表达式节点* 节点) -> 空类型 {
    如果 (列表 == 无 || 节点 == 无) {
        返回;
    }
    
    
    如果 (列表.节点 == 无) {
        列表.节点 = 节点;
        返回;
    }
    
    
    表达式列表* 当前 = 列表;
    当 (当前.下一个 != 无) {
        当前 = 当前.下一个;
    }
    
    
    表达式列表* 新节点 = 创建表达式列表();
    如果 (新节点 != 无) {
        新节点.节点 = 节点;
        当前.下一个 = 新节点;
    }
}



函数 释放表达式节点(表达式节点* 节点) -> 空类型 {
    如果 (节点 == 无) {
        返回;
    }
    
    
    如果 (节点.左操作数 != 无) {
        释放表达式节点(节点.左操作数);
    }
    如果 (节点.右操作数 != 无) {
        释放表达式节点(节点.右操作数);
    }
    如果 (节点.操作数 != 无) {
        释放表达式节点(节点.操作数);
    }
    如果 (节点.被调函数 != 无) {
        释放表达式节点(节点.被调函数);
    }
    如果 (节点.参数列表 != 无) {
        释放表达式列表(节点.参数列表);
    }
    如果 (节点.对象 != 无) {
        释放表达式节点(节点.对象);
    }
    如果 (节点.数组 != 无) {
        释放表达式节点(节点.数组);
    }
    如果 (节点.索引 != 无) {
        释放表达式节点(节点.索引);
    }
    如果 (节点.左侧表达式 != 无) {
        释放表达式节点(节点.左侧表达式);
    }
    如果 (节点.值 != 无) {
        释放表达式节点(节点.值);
    }
    如果 (节点.指针 != 无) {
        释放表达式节点(节点.指针);
    }
    
    
    释放内存(节点);
}



函数 释放表达式列表(表达式列表* 列表) -> 空类型 {
    如果 (列表 == 无) {
        返回;
    }
    
    
    如果 (列表.节点 != 无) {
        释放表达式节点(列表.节点);
    }
    
    
    如果 (列表.下一个 != 无) {
        释放表达式列表(列表.下一个);
    }
    
    
    释放内存(列表);
}
*)cn_rt_array_get_element(r28, r29, 8);
  r31 = 检查表达式(r27, r30);
  cn_var_实参类型_2 = r31;
  long long cn_var_形参类型_3;
  r32 = cn_var_函数类型_0.参数类型列表;
  r33 = cn_var_i_1;
  r34 = (void*)cn_rt_array_get_element(r32, r33, 8);
  cn_var_形参类型_3 = r34;
  long long cn_var_兼容性_4;
  r35 = cn_var_实参类型_2;
  r36 = cn_var_形参类型_3;
  r37 = 检查类型兼容性(r35, r36);
  cn_var_兼容性_4 = r37;
  r38 = cn_var_兼容性_4;
  r39 = r38 == 类型兼容性_不兼容;
  if (r39) goto if_then_2807; else goto if_merge_2808;

  for_update_2803:
  r46 = cn_var_i_1;
  r47 = r46 + 1;
  cn_var_i_1 = r47;
  goto for_cond_2801;

  for_exit_2804:
  r48 = cn_var_函数类型_0.返回类型;
  return r48;

  logic_rhs_2805:
  r24 = cn_var_i_1;
  r25 = cn_var_函数类型_0.参数个数;
  r26 = r24 < r25;
  goto logic_merge_2806;

  logic_merge_2806:
  if (r26) goto for_body_2802; else goto for_exit_2804;

  if_then_2807:
  r40 = cn_var_上下文;
  r41 = cn_var_调用节点->参数;
  r42 = cn_var_i_1;
  r43 = (struct 表达式节点*[] 参数;     
    整数 参数个数;          
}






结构体 成员访问表达式 {
    AST节点 节点基类;       
    表达式节点* 对象;       
    字符串 成员名;          
    布尔 是指针访问;        
    布尔 是静态成员;        
    字符串 类名;            
}






结构体 数组访问表达式 {
    AST节点 节点基类;       
    表达式节点* 数组;       
    表达式节点* 索引;       
}






结构体 赋值表达式 {
    AST节点 节点基类;       
    表达式节点* 目标;       
    表达式节点* 值;         
    赋值运算符 运算符;      
}






结构体 三元表达式 {
    AST节点 节点基类;       
    表达式节点* 条件;       
    表达式节点* 真值;       
    表达式节点* 假值;       
}






结构体 数组字面量表达式 {
    AST节点 节点基类;       
    表达式节点*[] 元素;     
    整数 元素个数;          
}






结构体 结构体字段初始化 {
    字符串 字段名;          
    表达式节点* 值;         
}






结构体 结构体字面量表达式 {
    AST节点 节点基类;       
    字符串 结构体名;        
    结构体字段初始化*[] 字段; 
    整数 字段个数;          
}






结构体 模板实例化表达式 {
    AST节点 节点基类;       
    字符串 模板名;          
    类型节点*[] 类型参数;   
    整数 参数个数;          
}







结构体 表达式节点 {
    节点类型 类型;          
    源位置 位置;            
    类型节点* 类型信息;     
    布尔 是自身指针;        
    
    
    整数 整数值;            
    小数 小数值;            
    字符串 字符串值;        
    布尔 布尔值;            
    
    
    字符串 标识符名称;      
    
    
    表达式节点* 左操作数;   
    表达式节点* 右操作数;   
    整数 运算符;            
    
    
    表达式节点* 操作数;     
    
    
    表达式节点* 被调函数;   
    表达式列表* 参数列表;   
    整数 参数个数;          
    
    
    表达式节点* 对象;       
    字符串 成员名;          
    整数 成员索引;          
    
    
    表达式节点* 数组;       
    表达式节点* 索引;       
    
    
    表达式节点* 左侧表达式; 
    表达式节点* 值;         
    
    
    赋值表达式 赋值表达式;   
    
    
    三元表达式 三元表达式;  
    
    
    表达式节点* 指针;       
    
    
    类型节点* 目标类型;     
    表达式节点* 转换操作数; 
}






结构体 表达式列表 {
    表达式节点* 节点;          
    表达式列表* 下一个;        
}








函数 创建表达式节点(节点类型 类型) -> 表达式节点* {
    
    表达式节点* 节点 = 分配内存(类型大小(表达式节点));
    如果 (节点 == 无) {
        返回 无;
    }
    
    
    节点.类型 = 类型;
    节点.位置.文件名 = "";
    节点.位置.行号 = 0;
    节点.位置.列号 = 0;
    节点.类型信息 = 无;
    节点.是自身指针 = 假;
    
    
    节点.整数值 = 0;
    节点.小数值 = 0.0;
    节点.字符串值 = "";
    节点.布尔值 = 假;
    节点.标识符名称 = "";
    节点.左操作数 = 无;
    节点.右操作数 = 无;
    节点.运算符 = 0;
    节点.操作数 = 无;
    节点.被调函数 = 无;
    节点.参数列表 = 无;
    节点.参数个数 = 0;
    节点.对象 = 无;
    节点.成员名 = "";
    节点.成员索引 = 0;
    节点.数组 = 无;
    节点.索引 = 无;
    节点.左侧表达式 = 无;
    节点.值 = 无;
    节点.指针 = 无;
    
    返回 节点;
}



函数 创建赋值表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.赋值表达式);
    返回 节点;
}



函数 创建二元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.二元表达式);
    返回 节点;
}



函数 创建三元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.三元表达式);
    返回 节点;
}



函数 创建一元表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.一元表达式);
    返回 节点;
}



函数 创建函数调用表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.函数调用表达式);
    返回 节点;
}



函数 创建成员访问表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.成员访问表达式);
    返回 节点;
}



函数 创建数组访问表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.数组访问表达式);
    返回 节点;
}



函数 创建字面量表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.字面量表达式);
    返回 节点;
}



函数 创建标识符表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.标识符表达式);
    返回 节点;
}



函数 创建空值表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.字面量表达式);
    如果 (节点 != 无) {
        节点.布尔值 = 假;
    }
    返回 节点;
}



函数 创建自身表达式() -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.标识符表达式);
    如果 (节点 != 无) {
        节点.是自身指针 = 真;
        节点.标识符名称 = "自身";
    }
    返回 节点;
}




函数 创建基类访问表达式(字符串 成员名) -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.成员访问表达式);
    如果 (节点 != 无) {
        节点.成员名 = 成员名;
    }
    返回 节点;
}





函数 创建类型转换表达式(类型节点* 目标类型, 表达式节点* 操作数) -> 表达式节点* {
    表达式节点* 节点 = 创建表达式节点(节点类型.类型转换表达式);
    如果 (节点 != 无) {
        节点.目标类型 = 目标类型;
        节点.转换操作数 = 操作数;
        节点.类型信息 = 目标类型;  
    }
    返回 节点;
}



函数 创建表达式列表() -> 表达式列表* {
    表达式列表* 列表 = 分配内存(类型大小(表达式列表));
    如果 (列表 == 无) {
        返回 无;
    }
    
    列表.节点 = 无;
    列表.下一个 = 无;
    
    返回 列表;
}




函数 表达式列表添加(表达式列表* 列表, 表达式节点* 节点) -> 空类型 {
    如果 (列表 == 无 || 节点 == 无) {
        返回;
    }
    
    
    如果 (列表.节点 == 无) {
        列表.节点 = 节点;
        返回;
    }
    
    
    表达式列表* 当前 = 列表;
    当 (当前.下一个 != 无) {
        当前 = 当前.下一个;
    }
    
    
    表达式列表* 新节点 = 创建表达式列表();
    如果 (新节点 != 无) {
        新节点.节点 = 节点;
        当前.下一个 = 新节点;
    }
}



函数 释放表达式节点(表达式节点* 节点) -> 空类型 {
    如果 (节点 == 无) {
        返回;
    }
    
    
    如果 (节点.左操作数 != 无) {
        释放表达式节点(节点.左操作数);
    }
    如果 (节点.右操作数 != 无) {
        释放表达式节点(节点.右操作数);
    }
    如果 (节点.操作数 != 无) {
        释放表达式节点(节点.操作数);
    }
    如果 (节点.被调函数 != 无) {
        释放表达式节点(节点.被调函数);
    }
    如果 (节点.参数列表 != 无) {
        释放表达式列表(节点.参数列表);
    }
    如果 (节点.对象 != 无) {
        释放表达式节点(节点.对象);
    }
    如果 (节点.数组 != 无) {
        释放表达式节点(节点.数组);
    }
    如果 (节点.索引 != 无) {
        释放表达式节点(节点.索引);
    }
    如果 (节点.左侧表达式 != 无) {
        释放表达式节点(节点.左侧表达式);
    }
    如果 (节点.值 != 无) {
        释放表达式节点(节点.值);
    }
    如果 (节点.指针 != 无) {
        释放表达式节点(节点.指针);
    }
    
    
    释放内存(节点);
}



函数 释放表达式列表(表达式列表* 列表) -> 空类型 {
    如果 (列表 == 无) {
        返回;
    }
    
    
    如果 (列表.节点 != 无) {
        释放表达式节点(列表.节点);
    }
    
    
    如果 (列表.下一个 != 无) {
        释放表达式列表(列表.下一个);
    }
    
    
    释放内存(列表);
}
*)cn_rt_array_get_element(r41, r42, 8);
  r44 = r43->节点基类;
  r45 = r44.位置;
  报告错误(r40, r45, 诊断错误码_语义_参数类型不匹配);
  goto if_merge_2808;

  if_merge_2808:
  goto for_update_2803;
  return NULL;
}

struct 类型信息* 检查成员访问表达式(struct 语义检查上下文* cn_var_上下文, struct 成员访问表达式* cn_var_成员节点) {
  long long r1, r5, r6, r8, r10, r14, r15, r16, r18, r20, r23, r24, r26, r27, r28, r29, r30, r34, r38, r42, r43, r44;
  char* r21;
  char* r25;
  struct 成员访问表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r11;
  struct 符号* r22;
  struct 语义检查上下文* r31;
  struct 符号表管理器* r35;
  struct 作用域* r36;
  struct 语义检查上下文* r39;
  struct 类型推断上下文* r45;
  struct 类型信息* r46;
  _Bool r7;
  _Bool r9;
  _Bool r17;
  _Bool r19;
  _Bool r37;
  struct AST节点 r12;
  struct 源位置 r13;
  struct AST节点 r32;
  struct 源位置 r33;
  struct AST节点 r40;
  struct 源位置 r41;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2809; else goto if_merge_2810;

  if_then_2809:
  return 0;
  goto if_merge_2810;

  if_merge_2810:
  long long cn_var_对象类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_成员节点->对象;
  r4 = 检查表达式(r2, r3);
  cn_var_对象类型_0 = r4;
  r5 = cn_var_对象类型_0;
  r6 = r5 == 0;
  if (r6) goto if_then_2811; else goto if_merge_2812;

  if_then_2811:
  return 0;
  goto if_merge_2812;

  if_merge_2812:
  r7 = cn_var_成员节点->是指针访问;
  if (r7) goto if_then_2813; else goto if_merge_2814;

  if_then_2813:
  r8 = cn_var_对象类型_0;
  r9 = 是指针类型(r8);
  r10 = r9 == 0;
  if (r10) goto if_then_2815; else goto if_merge_2816;

  if_merge_2814:
  long long cn_var_成员符号_1;
  cn_var_成员符号_1 = 0;
  r16 = cn_var_对象类型_0;
  r17 = 是结构体类型(r16);
  if (r17) goto logic_merge_2821; else goto logic_rhs_2820;

  if_then_2815:
  r11 = cn_var_上下文;
  r12 = cn_var_成员节点->节点基类;
  r13 = r12.位置;
  报告错误(r11, r13, 诊断错误码_语义_类型不匹配);
  return 0;
  goto if_merge_2816;

  if_merge_2816:
  r14 = cn_var_对象类型_0.指向类型;
  cn_var_对象类型_0 = r14;
  goto if_merge_2814;

  if_then_2817:
  r20 = cn_var_对象类型_0.类型符号;
  r21 = cn_var_成员节点->成员名;
  r22 = 查找类成员(r20, r21);
  cn_var_成员符号_1 = r22;
  goto if_merge_2819;

  if_else_2818:
  r23 = cn_var_对象类型_0;
  是枚举类型(r23);
  goto if_then_2822;

  if_merge_2819:
  r29 = cn_var_成员符号_1;
  r30 = r29 == 0;
  if (r30) goto if_then_2826; else goto if_merge_2827;

  logic_rhs_2820:
  r18 = cn_var_对象类型_0;
  r19 = 是类类型(r18);
  goto logic_merge_2821;

  logic_merge_2821:
  if (r19) goto if_then_2817; else goto if_else_2818;

  if_then_2822:
  r24 = cn_var_对象类型_0.类型符号;
  r25 = cn_var_成员节点->成员名;
  查找枚举成员(r24, r25);
  cn_var_成员符号_1 = /* NONE */;
  r26 = cn_var_成员符号_1;
  r27 = r26 != 0;
  if (r27) goto if_then_2824; else goto if_merge_2825;

  if_merge_2823:
  goto if_merge_2819;

  if_then_2824:
  r28 = cn_var_成员符号_1.类型信息;
  return r28;
  goto if_merge_2825;

  if_merge_2825:
  goto if_merge_2823;

  if_then_2826:
  r31 = cn_var_上下文;
  r32 = cn_var_成员节点->节点基类;
  r33 = r32.位置;
  报告错误(r31, r33, 诊断错误码_语义_成员未找到);
  return 0;
  goto if_merge_2827;

  if_merge_2827:
  r34 = cn_var_成员符号_1;
  r35 = cn_var_上下文->符号表;
  r36 = r35->当前作用域;
  r37 = 检查符号可访问性(r34, r36);
  r38 = r37 == 0;
  if (r38) goto if_then_2828; else goto if_merge_2829;

  if_then_2828:
  r39 = cn_var_上下文;
  r40 = cn_var_成员节点->节点基类;
  r41 = r40.位置;
  报告错误(r39, r41, 诊断错误码_语义_访问被拒绝);
  goto if_merge_2829;

  if_merge_2829:
  r42 = cn_var_成员符号_1.类型信息;
  r43 = r42 != 0;
  if (r43) goto if_then_2830; else goto if_merge_2831;

  if_then_2830:
  r44 = cn_var_成员符号_1.类型信息;
  r45 = cn_var_上下文->类型上下文;
  r46 = 从类型节点推断(r44, r45);
  return r46;
  goto if_merge_2831;

  if_merge_2831:
  return 0;
}

struct 类型信息* 检查数组访问表达式(struct 语义检查上下文* cn_var_上下文, struct 数组访问表达式* cn_var_数组节点) {
  long long r1, r8, r11, r14, r18, r20, r23, r24, r27, r28;
  struct 数组访问表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r5;
  struct 表达式节点* r6;
  struct 类型信息* r7;
  struct 语义检查上下文* r15;
  struct 语义检查上下文* r21;
  struct 表达式节点* r22;
  _Bool r10;
  _Bool r13;
  _Bool r19;
  _Bool r26;
  struct AST节点 r16;
  struct 源位置 r17;
  enum 节点类型 r9;
  enum 节点类型 r12;
  enum 节点类型 r25;

  entry:
  r0 = cn_var_数组节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2832; else goto if_merge_2833;

  if_then_2832:
  return 0;
  goto if_merge_2833;

  if_merge_2833:
  enum 节点类型 cn_var_数组类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_数组节点->数组;
  r4 = 检查表达式(r2, r3);
  cn_var_数组类型_0 = r4;
  long long cn_var_索引类型_1;
  r5 = cn_var_上下文;
  r6 = cn_var_数组节点->索引;
  r7 = 检查表达式(r5, r6);
  cn_var_索引类型_1 = r7;
  r9 = cn_var_数组类型_0;
  r10 = 是数组类型(r9);
  r11 = r10 == 0;
  if (r11) goto logic_rhs_2836; else goto logic_merge_2837;

  if_then_2834:
  r15 = cn_var_上下文;
  r16 = cn_var_数组节点->节点基类;
  r17 = r16.位置;
  报告错误(r15, r17, 诊断错误码_语义_非结构体类型);
  return 0;
  goto if_merge_2835;

  if_merge_2835:
  r18 = cn_var_索引类型_1;
  r19 = 是整数类型(r18);
  r20 = r19 == 0;
  if (r20) goto if_then_2838; else goto if_merge_2839;

  logic_rhs_2836:
  r12 = cn_var_数组类型_0;
  r13 = 是指针类型(r12);
  r14 = r13 == 0;
  goto logic_merge_2837;

  logic_merge_2837:
  if (r14) goto if_then_2834; else goto if_merge_2835;

  if_then_2838:
  r21 = cn_var_上下文;
  r22 = cn_var_数组节点->索引;
  r23 = r22->节点基类;
  r24 = r23.位置;
  报告错误(r21, r24, 诊断错误码_语义_类型不匹配);
  goto if_merge_2839;

  if_merge_2839:
  r25 = cn_var_数组类型_0;
  r26 = 是数组类型(r25);
  if (r26) goto if_then_2840; else goto if_else_2841;

  if_then_2840:
  r27 = cn_var_数组类型_0.元素类型;
  return r27;
  goto if_merge_2842;

  if_else_2841:
  r28 = cn_var_数组类型_0.指向类型;
  return r28;
  goto if_merge_2842;

  if_merge_2842:
  return NULL;
}

struct 类型信息* 检查赋值表达式(struct 语义检查上下文* cn_var_上下文, struct 赋值表达式* cn_var_赋值节点) {
  long long r1, r8, r9, r10, r11, r12, r13, r14, r16, r17, r21;
  struct 赋值表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r5;
  struct 表达式节点* r6;
  struct 类型信息* r7;
  struct 语义检查上下文* r18;
  enum 类型兼容性 r15;
  struct AST节点 r19;
  struct 源位置 r20;

  entry:
  r0 = cn_var_赋值节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2843; else goto if_merge_2844;

  if_then_2843:
  return 0;
  goto if_merge_2844;

  if_merge_2844:
  long long cn_var_目标类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_赋值节点->目标;
  r4 = 检查表达式(r2, r3);
  cn_var_目标类型_0 = r4;
  long long cn_var_值类型_1;
  r5 = cn_var_上下文;
  r6 = cn_var_赋值节点->值;
  r7 = 检查表达式(r5, r6);
  cn_var_值类型_1 = r7;
  r9 = cn_var_目标类型_0;
  r10 = r9 != 0;
  if (r10) goto logic_rhs_2847; else goto logic_merge_2848;

  if_then_2845:
  long long cn_var_兼容性_2;
  r13 = cn_var_值类型_1;
  r14 = cn_var_目标类型_0;
  r15 = 检查类型兼容性(r13, r14);
  cn_var_兼容性_2 = r15;
  r16 = cn_var_兼容性_2;
  r17 = r16 == 类型兼容性_不兼容;
  if (r17) goto if_then_2849; else goto if_merge_2850;

  if_merge_2846:
  r21 = cn_var_目标类型_0;
  return r21;

  logic_rhs_2847:
  r11 = cn_var_值类型_1;
  r12 = r11 != 0;
  goto logic_merge_2848;

  logic_merge_2848:
  if (r12) goto if_then_2845; else goto if_merge_2846;

  if_then_2849:
  r18 = cn_var_上下文;
  r19 = cn_var_赋值节点->节点基类;
  r20 = r19.位置;
  报告错误(r18, r20, 诊断错误码_语义_无效赋值);
  goto if_merge_2850;

  if_merge_2850:
  goto if_merge_2846;
  return NULL;
}

struct 类型信息* 检查三元表达式(struct 语义检查上下文* cn_var_上下文, struct 三元表达式* cn_var_三元节点) {
  long long r1, r11, r12, r13, r14, r15, r18, r19, r20, r21, r22, r23, r24, r25, r26, r28, r29, r33;
  struct 三元表达式* r0;
  struct 语义检查上下文* r2;
  struct 表达式节点* r3;
  struct 类型信息* r4;
  struct 语义检查上下文* r5;
  struct 表达式节点* r6;
  struct 类型信息* r7;
  struct 语义检查上下文* r8;
  struct 表达式节点* r9;
  struct 类型信息* r10;
  struct 语义检查上下文* r16;
  struct 表达式节点* r17;
  struct 语义检查上下文* r30;
  enum 类型兼容性 r27;
  struct AST节点 r31;
  struct 源位置 r32;

  entry:
  r0 = cn_var_三元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2851; else goto if_merge_2852;

  if_then_2851:
  return 0;
  goto if_merge_2852;

  if_merge_2852:
  long long cn_var_条件类型_0;
  r2 = cn_var_上下文;
  r3 = cn_var_三元节点->条件;
  r4 = 检查表达式(r2, r3);
  cn_var_条件类型_0 = r4;
  long long cn_var_真值类型_1;
  r5 = cn_var_上下文;
  r6 = cn_var_三元节点->真值;
  r7 = 检查表达式(r5, r6);
  cn_var_真值类型_1 = r7;
  long long cn_var_假值类型_2;
  r8 = cn_var_上下文;
  r9 = cn_var_三元节点->假值;
  r10 = 检查表达式(r8, r9);
  cn_var_假值类型_2 = r10;
  r12 = cn_var_条件类型_0;
  r13 = r12 != 0;
  if (r13) goto logic_rhs_2855; else goto logic_merge_2856;

  if_then_2853:
  r16 = cn_var_上下文;
  r17 = cn_var_三元节点->条件;
  r18 = r17->节点基类;
  r19 = r18.位置;
  报告错误(r16, r19, 诊断错误码_语义_类型不匹配);
  goto if_merge_2854;

  if_merge_2854:
  r21 = cn_var_真值类型_1;
  r22 = r21 != 0;
  if (r22) goto logic_rhs_2859; else goto logic_merge_2860;

  logic_rhs_2855:
  r14 = cn_var_条件类型_0.种类;
  r15 = r14 != /* NONE */;
  goto logic_merge_2856;

  logic_merge_2856:
  if (r15) goto if_then_2853; else goto if_merge_2854;

  if_then_2857:
  long long cn_var_兼容性_3;
  r25 = cn_var_真值类型_1;
  r26 = cn_var_假值类型_2;
  r27 = 检查类型兼容性(r25, r26);
  cn_var_兼容性_3 = r27;
  r28 = cn_var_兼容性_3;
  r29 = r28 == 类型兼容性_不兼容;
  if (r29) goto if_then_2861; else goto if_merge_2862;

  if_merge_2858:
  r33 = cn_var_真值类型_1;
  return r33;

  logic_rhs_2859:
  r23 = cn_var_假值类型_2;
  r24 = r23 != 0;
  goto logic_merge_2860;

  logic_merge_2860:
  if (r24) goto if_then_2857; else goto if_merge_2858;

  if_then_2861:
  r30 = cn_var_上下文;
  r31 = cn_var_三元节点->节点基类;
  r32 = r31.位置;
  报告错误(r30, r32, 诊断错误码_语义_类型不匹配);
  goto if_merge_2862;

  if_merge_2862:
  goto if_merge_2858;
  return NULL;
}

_Bool 是常量表达式(struct 表达式节点* cn_var_表达式) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33;

  entry:
  goto tail_rec_loop;
  return 0;
}

void 报告错误(struct 语义检查上下文* cn_var_上下文, struct 源位置 cn_var_位置, enum 诊断错误码 cn_var_错误码) {
  long long r0, r1;

  entry:
  r0 = cn_var_上下文->错误计数;
  r1 = r0 + 1;
  return;
}

void 报告警告(struct 语义检查上下文* cn_var_上下文, struct 源位置 cn_var_位置, const char* cn_var_消息) {
  long long r0, r1;

  entry:
  r0 = cn_var_上下文->警告计数;
  r1 = r0 + 1;
  return;
}

long long 语义检查上下文大小() {

  entry:
  return 128;
}

