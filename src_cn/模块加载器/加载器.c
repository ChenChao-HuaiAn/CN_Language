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
struct 扫描器;
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};
struct 诊断集合;
struct 语法错误恢复上下文 {
    long long 连续错误计数;
    long long 最大连续错误;
    long long 最后错误行号;
    long long 级联抑制计数;
    _Bool 抑制级联错误;
    long long 恢复尝试次数;
    long long 最大恢复次数;
};
struct 解析器;
struct 解析器 {
    struct 词元 当前词元;
    struct 词元 下一个词元;
    struct 扫描器* 扫描器实例;
    struct 诊断集合* 诊断集合指针;
    long long 错误计数;
    long long 恢复计数;
    struct 语法错误恢复上下文 恢复上下文;
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
enum 诊断严重级别 {
    诊断严重级别_诊断_错误 = 0,
    诊断严重级别_诊断_警告 = 1
};
enum 语法恢复策略 {
    语法恢复策略_不恢复 = 0,
    语法恢复策略_跳过词元 = 1,
    语法恢复策略_同步到分号 = 2,
    语法恢复策略_同步到大括号 = 3,
    语法恢复策略_同步到关键字 = 4
};
enum 同步点类型 {
    同步点类型_同步点_分号 = 0,
    同步点类型_同步点_右大括号 = 1,
    同步点类型_同步点_右小括号 = 2,
    同步点类型_同步点_右中括号 = 3,
    同步点类型_同步点_逗号 = 4,
    同步点类型_同步点_关键字 = 5,
    同步点类型_同步点_类型关键字 = 6,
    同步点类型_同步点_声明关键字 = 7
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

// Struct Definitions - 从导入模块
struct 关键字条目;
struct 关键字条目 {
    char* 名称;
    long long 类型;
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
struct 类型节点;
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
struct 模板实例化表达式;
struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};
struct 表达式节点;
struct 逻辑表达式;
struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
};
struct 结构体字面量表达式;
struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};
struct 数组字面量表达式;
struct 数组字面量表达式 {
    struct AST节点 节点基类;
    struct 表达式节点** 元素;
    long long 元素个数;
};
struct 三元表达式;
struct 三元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 表达式节点* 真值;
    struct 表达式节点* 假值;
};
struct 赋值表达式;
struct 赋值表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 目标;
    struct 表达式节点* 值;
    enum 赋值运算符 运算符;
};
struct 数组访问表达式;
struct 数组访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
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
struct 函数调用表达式;
struct 函数调用表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 被调函数;
    struct 表达式节点** 参数;
    long long 参数个数;
};
struct 标识符表达式;
struct 标识符表达式 {
    struct AST节点 节点基类;
    char* 名称;
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
struct 一元表达式;
struct 一元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 操作数;
    enum 一元运算符 运算符;
    _Bool 是前缀;
};
struct 二元表达式;
struct 二元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 二元运算符 运算符;
};
struct 块语句;
struct 最终语句;
struct 最终语句 {
    struct AST节点 节点基类;
    struct 块语句* 语句体;
};
struct 抛出语句;
struct 抛出语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 异常表达式;
    char* 异常类型;
    char* 消息;
};
struct 尝试语句;
struct 尝试语句 {
    struct AST节点 节点基类;
    struct 块语句* 尝试块;
    struct 捕获子句** 捕获子句列表;
    long long 捕获个数;
    struct 块语句* 最终块;
};
struct 选择语句;
struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
};
struct 继续语句;
struct 继续语句 {
    struct AST节点 节点基类;
};
struct 中断语句;
struct 中断语句 {
    struct AST节点 节点基类;
};
struct 返回语句;
struct 返回语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 返回值;
};
struct 语句节点;
struct 循环语句;
struct 循环语句 {
    struct AST节点 节点基类;
    struct 语句节点* 初始化;
    struct 表达式节点* 条件;
    struct 表达式节点* 更新;
    struct 块语句* 循环体;
};
struct 当语句;
struct 当语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 循环体;
};
struct 如果语句;
struct 如果语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 真分支;
    struct 语句节点* 假分支;
};
struct 块语句;
struct 块语句 {
    struct AST节点 节点基类;
    struct 语句节点** 语句;
    long long 语句个数;
};
struct 表达式语句;
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};
struct 结构体声明;
struct 模板结构体声明;
struct 模板结构体声明 {
    struct AST节点 节点基类;
    struct 模板参数** 模板参数;
    long long 参数个数;
    struct 结构体声明* 结构体声明节点;
};
struct 函数声明;
struct 模板函数声明;
struct 模板函数声明 {
    struct AST节点 节点基类;
    struct 模板参数** 模板参数;
    long long 参数个数;
    struct 函数声明* 函数声明节点;
};
struct 接口声明;
struct 接口声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 接口方法** 方法;
    long long 方法个数;
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
struct 枚举声明;
struct 枚举声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 枚举成员** 成员;
    long long 成员个数;
};
struct 结构体声明;
struct 结构体声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 结构体成员** 成员;
    long long 成员个数;
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
struct 声明节点列表;
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
struct 表达式列表;
struct 表达式列表;
struct 表达式列表 {
    struct 表达式节点* 节点;
    struct 表达式列表* 下一个;
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 语句节点;
struct 语句节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
};
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};
struct 声明节点;
struct 声明节点列表;
struct 声明节点列表 {
    struct 声明节点* 节点;
    struct 声明节点列表* 下一个;
};
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
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
};
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
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
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
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

// CN Language Enum Definitions
enum 模块类型 {
    模块类型_文件模块,
    模块类型_包模块,
    模块类型_内联模块,
    模块类型_内建模块
};

enum 模块加载状态 {
    模块加载状态_未加载,
    模块加载状态_加载中,
    模块加载状态_已加载,
    模块加载状态_加载失败
};

// CN Language Global Struct Forward Declarations
struct 模块标识符;
struct 导出符号;
struct 模块依赖;
struct 模块元数据;
struct 模块缓存条目;
struct 模块加载器;

// CN Language Global Struct Definitions
struct 模块标识符 {
    char* 完全限定名;
    long long 完全限定名长度;
    char** 路径段列表;
    long long 路径段数量;
};

struct 导出符号 {
    char* 名称;
    long long 名称长度;
    _Bool 是函数;
    _Bool 是常量;
    void* 符号信息;
};

struct 模块依赖 {
    struct 模块标识符* 模块标识;
    _Bool 是必需;
    _Bool 是循环;
};

struct 模块元数据 {
    struct 模块标识符* 标识符;
    enum 模块类型 类型;
    enum 模块加载状态 状态;
    char* 文件路径;
    long long 文件路径长度;
    struct 导出符号* 导出符号表;
    long long 导出符号数量;
    long long 导出符号容量;
    struct 模块依赖* 依赖列表;
    long long 依赖数量;
    long long 依赖容量;
    struct 程序节点* 程序节点;
    struct 符号表管理器* 符号表;
};

struct 模块缓存条目 {
    char* 模块名;
    struct 模块元数据* 元数据;
    struct 模块缓存条目* 下一个;
};

struct 模块加载器 {
    char** 搜索路径列表;
    long long 搜索路径数量;
    struct 模块缓存条目** 缓存列表;
    long long 缓存数量;
    struct 诊断集合* 诊断集合;
    struct 模块元数据* 当前模块;
    long long 当前依赖深度;
    long long 加载成功数;
    long long 加载失败数;
};

// Global Variables
long long cn_var_最大模块路径长度 = 1024;
long long cn_var_最大搜索路径数量 = 32;
long long cn_var_最大模块缓存数量 = 256;
long long cn_var_最大依赖深度 = 16;

// Forward Declarations
struct 模块加载器* 创建模块加载器(struct 诊断集合*);
void 销毁模块加载器(struct 模块加载器*);
_Bool 添加搜索路径(struct 模块加载器*, char*);
void 设置默认搜索路径(struct 模块加载器*, char*);
struct 模块标识符* 解析模块标识符(char*);
void 销毁模块标识符(struct 模块标识符*);
_Bool 解析模块文件路径(struct 模块加载器*, struct 模块标识符*, char*, long long);
long long 计算哈希值(char*);
struct 模块元数据* 从缓存获取模块(struct 模块加载器*, char*);
_Bool 添加模块到缓存(struct 模块加载器*, char*, struct 模块元数据*);
_Bool 读取文件内容(char*, char**, long long*);
struct 模块元数据* 创建模块元数据(struct 模块标识符*);
void 销毁模块元数据(struct 模块元数据*);
struct 模块元数据* 加载模块核心(struct 模块加载器*, char*);
struct 模块元数据* 加载模块(struct 模块加载器*, char*);
_Bool 处理导入语句(struct 模块加载器*, struct 导入声明*, struct 模块元数据*);
_Bool 处理所有导入语句(struct 模块加载器*, struct 模块元数据*);
_Bool 收集导出符号(struct 模块元数据*);
long long 模块加载器大小();
long long 模块标识符大小();
long long 模块元数据大小();
long long 模块缓存条目大小();
long long 模块依赖大小();
long long 导出符号大小();
long long 字符串指针大小();
long long 获取文件大小(void*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置*, char*);

struct 模块加载器* 创建模块加载器(struct 诊断集合* cn_var_诊断集合指针) {
  long long r0;
  void* r1;
  struct 模块加载器* r2;
  struct 诊断集合* r4;
  struct 模块加载器* r5;
  _Bool r3;

  entry:
  struct 模块加载器* cn_var_加载器_0;
  r0 = 模块加载器大小();
  r1 = 分配清零内存(1, r0);
  cn_var_加载器_0 = (struct 模块加载器*)0;
  r2 = cn_var_加载器_0;
  r3 = r2 == 0;
  if (r3) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  r4 = cn_var_诊断集合指针;
  r5 = cn_var_加载器_0;
  return r5;
}

void 销毁模块加载器(struct 模块加载器* cn_var_加载器) {
  long long r2, r4, r8, r14, r17, r18, r19, r20, r24, r34, r35;
  char* r31;
  void* r7;
  void* r13;
  void* r23;
  struct 模块加载器* r0;
  struct 模块加载器* r3;
  struct 模块加载器* r6;
  char** r9;
  char** r10;
  struct 模块加载器* r12;
  char** r15;
  char** r16;
  struct 模块加载器* r22;
  struct 模块缓存条目* r25;
  struct 模块缓存条目* r26;
  struct 模块缓存条目* r28;
  struct 模块缓存条目* r29;
  struct 模块缓存条目* r30;
  struct 模块缓存条目* r32;
  struct 模块缓存条目* r33;
  struct 模块加载器* r36;
  _Bool r1;
  _Bool r5;
  _Bool r11;
  _Bool r21;
  _Bool r27;

  entry:
  r0 = cn_var_加载器;
  r1 = r0 == 0;
  if (r1) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return;
  goto if_merge_3;

  if_merge_3:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_4;

  for_cond_4:
  r2 = cn_var_i_0;
  r3 = cn_var_加载器;
  r4 = r3->搜索路径数量;
  r5 = r2 < r4;
  if (r5) goto for_body_5; else goto for_exit_7;

  for_body_5:
  r6 = cn_var_加载器;
  r7 = r6->搜索路径列表;
  r8 = cn_var_i_0;
  r9 = &r7[r8];
  r10 = r9;
  r11 = r10 != 0;
  if (r11) goto if_then_8; else goto if_merge_9;

  for_update_6:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_4;

  for_exit_7:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_10;

  if_then_8:
  r12 = cn_var_加载器;
  r13 = r12->搜索路径列表;
  r14 = cn_var_i_0;
  r15 = &r13[r14];
  r16 = r15;
  释放内存(r16);
  goto if_merge_9;

  if_merge_9:
  goto for_update_6;

  for_cond_10:
  r19 = cn_var_i_1;
  r20 = cn_var_最大模块缓存数量;
  r21 = r19 < r20;
  if (r21) goto for_body_11; else goto for_exit_13;

  for_body_11:
  struct 模块缓存条目* cn_var_条目_2;
  r22 = cn_var_加载器;
  r23 = r22->缓存列表;
  r24 = cn_var_i_1;
  r25 = &r23[r24];
  cn_var_条目_2 = r25;
  goto while_cond_14;

  for_update_12:
  r34 = cn_var_i_1;
  r35 = r34 + 1;
  cn_var_i_1 = r35;
  goto for_cond_10;

  for_exit_13:
  r36 = cn_var_加载器;
  释放内存(r36);

  while_cond_14:
  r26 = cn_var_条目_2;
  r27 = r26 != 0;
  if (r27) goto while_body_15; else goto while_exit_16;

  while_body_15:
  struct 模块缓存条目* cn_var_下一个_3;
  r28 = cn_var_条目_2;
  r29 = r28->下一个;
  cn_var_下一个_3 = r29;
  r30 = cn_var_条目_2;
  r31 = r30->模块名;
  释放内存(r31);
  r32 = cn_var_条目_2;
  释放内存(r32);
  r33 = cn_var_下一个_3;
  cn_var_条目_2 = r33;
  goto while_cond_14;

  while_exit_16:
  goto for_update_12;
  return;
}

_Bool 添加搜索路径(struct 模块加载器* cn_var_加载器, char* cn_var_路径) {
  long long r0, r6, r7, r10, r11, r12, r23, r26, r27;
  char* r3;
  char* r9;
  char* r14;
  char* r16;
  char* r17;
  char* r18;
  char* r19;
  void* r21;
  struct 模块加载器* r1;
  struct 模块加载器* r5;
  void* r13;
  struct 模块加载器* r20;
  struct 模块加载器* r22;
  char** r24;
  struct 模块加载器* r25;
  _Bool r2;
  _Bool r4;
  _Bool r8;
  _Bool r15;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_20; else goto logic_rhs_19;

  if_then_17:
  return 0;
  goto if_merge_18;

  if_merge_18:
  r5 = cn_var_加载器;
  r6 = r5->搜索路径数量;
  r7 = cn_var_最大搜索路径数量;
  r8 = r6 >= r7;
  if (r8) goto if_then_21; else goto if_merge_22;

  logic_rhs_19:
  r3 = cn_var_路径;
  r4 = r3 == 0;
  goto logic_merge_20;

  logic_merge_20:
  if (r4) goto if_then_17; else goto if_merge_18;

  if_then_21:
  return 0;
  goto if_merge_22;

  if_merge_22:
  long long cn_var_路径长度_0;
  r9 = cn_var_路径;
  r10 = 获取字符串长度(r9);
  cn_var_路径长度_0 = r10;
  char* cn_var_路径副本_1;
  r11 = cn_var_路径长度_0;
  r12 = r11 + 1;
  r13 = 分配内存(r12);
  cn_var_路径副本_1 = (char*)0;
  r14 = cn_var_路径副本_1;
  r15 = r14 == 0;
  if (r15) goto if_then_23; else goto if_merge_24;

  if_then_23:
  return 0;
  goto if_merge_24;

  if_merge_24:
  r16 = cn_var_路径副本_1;
  r17 = cn_var_路径;
  r18 = 复制字符串(r16, r17);
  r19 = cn_var_路径副本_1;
  r20 = cn_var_加载器;
  r21 = r20->搜索路径列表;
  r22 = cn_var_加载器;
  r23 = r22->搜索路径数量;
  r24 = &r21[r23];
  r24 = r19;
  r25 = cn_var_加载器;
  r26 = r25->搜索路径数量;
  r27 = r26 + 1;
  return 1;
}

void 设置默认搜索路径(struct 模块加载器* cn_var_加载器, char* cn_var_源文件目录) {
  char* r3;
  struct 模块加载器* r0;
  struct 模块加载器* r2;
  struct 模块加载器* r5;
  struct 模块加载器* r7;
  struct 模块加载器* r9;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  _Bool r8;
  _Bool r10;

  entry:
  r0 = cn_var_加载器;
  r1 = r0 == 0;
  if (r1) goto if_then_25; else goto if_merge_26;

  if_then_25:
  return;
  goto if_merge_26;

  if_merge_26:
  r2 = cn_var_加载器;
  r3 = cn_var_源文件目录;
  r4 = 添加搜索路径(r2, r3);
  r5 = cn_var_加载器;
  r6 = 添加搜索路径(r5, ".");
  r7 = cn_var_加载器;
  r8 = 添加搜索路径(r7, "标准库");
  r9 = cn_var_加载器;
  r10 = 添加搜索路径(r9, "stdlib");
  return;
}

struct 模块标识符* 解析模块标识符(char* cn_var_模块名) {
  long long r3, r4, r5, r7, r8, r10, r14, r15, r25, r26, r27, r30, r33, r34, r35, r36, r37, r38, r46, r47, r48, r50, r52, r56, r60, r63, r64, r65, r69, r70, r74, r76, r77, r89, r90, r91, r93, r96, r98, r99, r100, r101;
  char* r0;
  char* r2;
  char* r18;
  char* r22;
  char* r23;
  char* r24;
  char* r29;
  char* r44;
  char* r51;
  char* r55;
  char* r67;
  char* r81;
  char* r83;
  char* r85;
  char* r86;
  char* r92;
  void* r59;
  void* r62;
  void* r84;
  void* r95;
  void* r11;
  struct 模块标识符* r12;
  void* r16;
  struct 模块标识符* r17;
  struct 模块标识符* r20;
  struct 模块标识符* r21;
  char* r31;
  void* r39;
  struct 模块标识符* r40;
  char** r41;
  struct 模块标识符* r43;
  struct 模块标识符* r45;
  char* r53;
  char* r57;
  char* r61;
  void* r66;
  struct 模块标识符* r72;
  char** r73;
  char** r75;
  struct 模块标识符* r78;
  char** r79;
  struct 模块标识符* r80;
  struct 模块标识符* r82;
  struct 模块标识符* r87;
  char** r88;
  char* r94;
  char* r97;
  struct 模块标识符* r102;
  _Bool r1;
  _Bool r6;
  _Bool r9;
  _Bool r13;
  _Bool r19;
  _Bool r28;
  _Bool r32;
  _Bool r42;
  _Bool r49;
  _Bool r54;
  _Bool r58;
  _Bool r68;
  _Bool r71;

  entry:
  r0 = cn_var_模块名;
  r1 = r0 == 0;
  if (r1) goto if_then_27; else goto if_merge_28;

  if_then_27:
  return 0;
  goto if_merge_28;

  if_merge_28:
  long long cn_var_名称长度_0;
  r2 = cn_var_模块名;
  r3 = 获取字符串长度(r2);
  cn_var_名称长度_0 = r3;
  r5 = cn_var_名称长度_0;
  r6 = r5 == 0;
  if (r6) goto logic_merge_32; else goto logic_rhs_31;

  if_then_29:
  return 0;
  goto if_merge_30;

  if_merge_30:
  struct 模块标识符* cn_var_标识符_1;
  r10 = 模块标识符大小();
  r11 = 分配清零内存(1, r10);
  cn_var_标识符_1 = (struct 模块标识符*)0;
  r12 = cn_var_标识符_1;
  r13 = r12 == 0;
  if (r13) goto if_then_33; else goto if_merge_34;

  logic_rhs_31:
  r7 = cn_var_名称长度_0;
  r8 = cn_var_最大模块路径长度;
  r9 = r7 >= r8;
  goto logic_merge_32;

  logic_merge_32:
  if (r9) goto if_then_29; else goto if_merge_30;

  if_then_33:
  return 0;
  goto if_merge_34;

  if_merge_34:
  r14 = cn_var_名称长度_0;
  r15 = r14 + 1;
  r16 = 分配内存(r15);
  r17 = cn_var_标识符_1;
  r18 = r17->完全限定名;
  r19 = r18 == 0;
  if (r19) goto if_then_35; else goto if_merge_36;

  if_then_35:
  r20 = cn_var_标识符_1;
  释放内存(r20);
  return 0;
  goto if_merge_36;

  if_merge_36:
  r21 = cn_var_标识符_1;
  r22 = r21->完全限定名;
  r23 = cn_var_模块名;
  r24 = 复制字符串(r22, r23);
  r25 = cn_var_名称长度_0;
  long long cn_var_段数量_2;
  cn_var_段数量_2 = 1;
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_37;

  for_cond_37:
  r26 = cn_var_i_3;
  r27 = cn_var_名称长度_0;
  r28 = r26 < r27;
  if (r28) goto for_body_38; else goto for_exit_40;

  for_body_38:
  r29 = cn_var_模块名;
  r30 = cn_var_i_3;
  r31 = (void**)cn_rt_array_get_element(r29, r30, 8);
  r32 = r31 == 46;
  if (r32) goto if_then_41; else goto if_merge_42;

  for_update_39:
  r35 = cn_var_i_3;
  r36 = r35 + 1;
  cn_var_i_3 = r36;
  goto for_cond_37;

  for_exit_40:
  r37 = cn_var_段数量_2;
  r38 = 字符串指针大小();
  r39 = 分配清零内存(r37, r38);
  r40 = cn_var_标识符_1;
  r41 = r40->路径段列表;
  r42 = r41 == 0;
  if (r42) goto if_then_43; else goto if_merge_44;

  if_then_41:
  r33 = cn_var_段数量_2;
  r34 = r33 + 1;
  cn_var_段数量_2 = r34;
  goto if_merge_42;

  if_merge_42:
  goto for_update_39;

  if_then_43:
  r43 = cn_var_标识符_1;
  r44 = r43->完全限定名;
  释放内存(r44);
  r45 = cn_var_标识符_1;
  释放内存(r45);
  return 0;
  goto if_merge_44;

  if_merge_44:
  r46 = cn_var_段数量_2;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 1024);
  long long cn_var_缓冲区位置_4;
  cn_var_缓冲区位置_4 = 0;
  long long cn_var_当前段索引_5;
  cn_var_当前段索引_5 = 0;
  long long cn_var_i_6;
  cn_var_i_6 = 0;
  goto for_cond_45;

  for_cond_45:
  r47 = cn_var_i_6;
  r48 = cn_var_名称长度_0;
  r49 = r47 <= r48;
  if (r49) goto for_body_46; else goto for_exit_48;

  for_body_46:
  r51 = cn_var_模块名;
  r52 = cn_var_i_6;
  r53 = (void**)cn_rt_array_get_element(r51, r52, 8);
  r54 = r53 == 46;
  if (r54) goto logic_merge_53; else goto logic_rhs_52;

  for_update_47:
  r100 = cn_var_i_6;
  r101 = r100 + 1;
  cn_var_i_6 = r101;
  goto for_cond_45;

  for_exit_48:
  r102 = cn_var_标识符_1;
  return r102;

  if_then_49:
  r59 = cn_var_缓冲区;
  r60 = cn_var_缓冲区位置_4;
  r61 = &r59[r60];
  r61 = 0;
  long long cn_var_段长度_7;
  r62 = cn_var_缓冲区;
  r63 = 获取字符串长度(r62);
  cn_var_段长度_7 = r63;
  char* cn_var_段副本_8;
  r64 = cn_var_段长度_7;
  r65 = r64 + 1;
  r66 = 分配内存(r65);
  cn_var_段副本_8 = (char*)0;
  r67 = cn_var_段副本_8;
  r68 = r67 == 0;
  if (r68) goto if_then_54; else goto if_merge_55;

  if_else_50:
  r92 = cn_var_模块名;
  r93 = cn_var_i_6;
  r94 = (void**)cn_rt_array_get_element(r92, r93, 8);
  r95 = cn_var_缓冲区;
  r96 = cn_var_缓冲区位置_4;
  r97 = &r95[r96];
  r97 = r94;
  r98 = cn_var_缓冲区位置_4;
  r99 = r98 + 1;
  cn_var_缓冲区位置_4 = r99;
  goto if_merge_51;

  if_merge_51:
  goto for_update_47;

  logic_rhs_52:
  r55 = cn_var_模块名;
  r56 = cn_var_i_6;
  r57 = (void**)cn_rt_array_get_element(r55, r56, 8);
  r58 = r57 == 0;
  goto logic_merge_53;

  logic_merge_53:
  if (r58) goto if_then_49; else goto if_else_50;

  if_then_54:
  long long cn_var_j_9;
  cn_var_j_9 = 0;
  goto for_cond_56;

  if_merge_55:
  r83 = cn_var_段副本_8;
  r84 = cn_var_缓冲区;
  r85 = 复制字符串(r83, r84);
  r86 = cn_var_段副本_8;
  r87 = cn_var_标识符_1;
  r88 = r87->路径段列表;
  r89 = cn_var_当前段索引_5;
    { char* _tmp_r0 = r86; cn_rt_array_set_element(r88, r89, &_tmp_r0, 8); }
  r90 = cn_var_当前段索引_5;
  r91 = r90 + 1;
  cn_var_当前段索引_5 = r91;
  cn_var_缓冲区位置_4 = 0;
  goto if_merge_51;

  for_cond_56:
  r69 = cn_var_j_9;
  r70 = cn_var_当前段索引_5;
  r71 = r69 < r70;
  if (r71) goto for_body_57; else goto for_exit_59;

  for_body_57:
  r72 = cn_var_标识符_1;
  r73 = r72->路径段列表;
  r74 = cn_var_j_9;
  r75 = (void**)cn_rt_array_get_element(r73, r74, 8);
  释放内存(r75);
  goto for_update_58;

  for_update_58:
  r76 = cn_var_j_9;
  r77 = r76 + 1;
  cn_var_j_9 = r77;
  goto for_cond_56;

  for_exit_59:
  r78 = cn_var_标识符_1;
  r79 = r78->路径段列表;
  释放内存(r79);
  r80 = cn_var_标识符_1;
  r81 = r80->完全限定名;
  释放内存(r81);
  r82 = cn_var_标识符_1;
  释放内存(r82);
  return 0;
  goto if_merge_55;
  return NULL;
}

void 销毁模块标识符(struct 模块标识符* cn_var_标识符) {
  long long r10, r12, r16, r21, r23, r24;
  char* r3;
  char* r6;
  struct 模块标识符* r0;
  struct 模块标识符* r2;
  struct 模块标识符* r5;
  struct 模块标识符* r7;
  char** r8;
  struct 模块标识符* r11;
  struct 模块标识符* r14;
  char** r15;
  char** r17;
  struct 模块标识符* r19;
  char** r20;
  char** r22;
  struct 模块标识符* r25;
  char** r26;
  struct 模块标识符* r27;
  _Bool r1;
  _Bool r4;
  _Bool r9;
  _Bool r13;
  _Bool r18;

  entry:
  r0 = cn_var_标识符;
  r1 = r0 == 0;
  if (r1) goto if_then_60; else goto if_merge_61;

  if_then_60:
  return;
  goto if_merge_61;

  if_merge_61:
  r2 = cn_var_标识符;
  r3 = r2->完全限定名;
  r4 = r3 != 0;
  if (r4) goto if_then_62; else goto if_merge_63;

  if_then_62:
  r5 = cn_var_标识符;
  r6 = r5->完全限定名;
  释放内存(r6);
  goto if_merge_63;

  if_merge_63:
  r7 = cn_var_标识符;
  r8 = r7->路径段列表;
  r9 = r8 != 0;
  if (r9) goto if_then_64; else goto if_merge_65;

  if_then_64:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_66;

  if_merge_65:
  r27 = cn_var_标识符;
  释放内存(r27);

  for_cond_66:
  r10 = cn_var_i_0;
  r11 = cn_var_标识符;
  r12 = r11->路径段数量;
  r13 = r10 < r12;
  if (r13) goto for_body_67; else goto for_exit_69;

  for_body_67:
  r14 = cn_var_标识符;
  r15 = r14->路径段列表;
  r16 = cn_var_i_0;
  r17 = (void**)cn_rt_array_get_element(r15, r16, 8);
  r18 = r17 != 0;
  if (r18) goto if_then_70; else goto if_merge_71;

  for_update_68:
  r23 = cn_var_i_0;
  r24 = r23 + 1;
  cn_var_i_0 = r24;
  goto for_cond_66;

  for_exit_69:
  r25 = cn_var_标识符;
  r26 = r25->路径段列表;
  释放内存(r26);
  goto if_merge_65;

  if_then_70:
  r19 = cn_var_标识符;
  r20 = r19->路径段列表;
  r21 = cn_var_i_0;
  r22 = (void**)cn_rt_array_get_element(r20, r21, 8);
  释放内存(r22);
  goto if_merge_71;

  if_merge_71:
  goto for_update_68;
  return;
}

_Bool 解析模块文件路径(struct 模块加载器* cn_var_加载器, struct 模块标识符* cn_var_标识符, char* cn_var_输出路径, long long cn_var_输出路径大小) {
  long long r0, r1, r8, r10, r14, r18, r21, r23, r24, r26, r29, r31, r32, r35, r38, r40, r41, r43, r45, r46, r47, r48, r49, r51, r52, r54, r55, r57, r64, r65, r70, r71;
  char* r6;
  char* r17;
  char* r20;
  char* r37;
  char* r42;
  char* r53;
  char* r67;
  char* r69;
  void* r13;
  void* r19;
  void* r28;
  void* r39;
  void* r50;
  void* r56;
  void* r59;
  void* r68;
  struct 模块加载器* r2;
  struct 模块标识符* r4;
  struct 模块加载器* r9;
  struct 模块加载器* r12;
  char** r15;
  char** r16;
  void* r22;
  struct 模块标识符* r25;
  char* r30;
  struct 模块标识符* r33;
  char** r34;
  char** r36;
  void* r44;
  char* r58;
  void* r60;
  void* r61;
  void* r63;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r27;
  _Bool r62;
  _Bool r66;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_77; else goto logic_rhs_76;

  if_then_72:
  return 0;
  goto if_merge_73;

  if_merge_73:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_78;

  logic_rhs_74:
  r6 = cn_var_输出路径;
  r7 = r6 == 0;
  goto logic_merge_75;

  logic_merge_75:
  if (r7) goto if_then_72; else goto if_merge_73;

  logic_rhs_76:
  r4 = cn_var_标识符;
  r5 = r4 == 0;
  goto logic_merge_77;

  logic_merge_77:
  if (r5) goto logic_merge_75; else goto logic_rhs_74;

  for_cond_78:
  r8 = cn_var_i_0;
  r9 = cn_var_加载器;
  r10 = r9->搜索路径数量;
  r11 = r8 < r10;
  if (r11) goto for_body_79; else goto for_exit_81;

  for_body_79:
  char* cn_var_搜索路径_1;
  r12 = cn_var_加载器;
  r13 = r12->搜索路径列表;
  r14 = cn_var_i_0;
  r15 = &r13[r14];
  r16 = r15;
  cn_var_搜索路径_1 = r16;
  char* cn_var_路径缓冲区;
  cn_var_路径缓冲区 = cn_rt_array_alloc(8, 1024);
  long long cn_var_路径位置_2;
  cn_var_路径位置_2 = 0;
  long long cn_var_搜索路径长度_3;
  r17 = cn_var_搜索路径_1;
  r18 = 获取字符串长度(r17);
  cn_var_搜索路径长度_3 = r18;
  r19 = cn_var_路径缓冲区;
  r20 = cn_var_搜索路径_1;
  r21 = cn_var_搜索路径长度_3;
  r22 = 复制内存(r19, r20, r21);
  r23 = cn_var_搜索路径长度_3;
  cn_var_路径位置_2 = r23;
  long long cn_var_j_4;
  cn_var_j_4 = 0;
  goto for_cond_82;

  for_update_80:
  r70 = cn_var_i_0;
  r71 = r70 + 1;
  cn_var_i_0 = r71;
  goto for_cond_78;

  for_exit_81:
  return 0;

  for_cond_82:
  r24 = cn_var_j_4;
  r25 = cn_var_标识符;
  r26 = r25->路径段数量;
  r27 = r24 < r26;
  if (r27) goto for_body_83; else goto for_exit_85;

  for_body_83:
  r28 = cn_var_路径缓冲区;
  r29 = cn_var_路径位置_2;
  r30 = &r28[r29];
  r30 = 47;
  r31 = cn_var_路径位置_2;
  r32 = r31 + 1;
  cn_var_路径位置_2 = r32;
  char* cn_var_段_5;
  r33 = cn_var_标识符;
  r34 = r33->路径段列表;
  r35 = cn_var_j_4;
  r36 = (void**)cn_rt_array_get_element(r34, r35, 8);
  cn_var_段_5 = r36;
  long long cn_var_段长度_6;
  r37 = cn_var_段_5;
  r38 = 获取字符串长度(r37);
  cn_var_段长度_6 = r38;
  r39 = cn_var_路径缓冲区;
  r40 = cn_var_路径位置_2;
  r41 = r39 + r40;
  r42 = cn_var_段_5;
  r43 = cn_var_段长度_6;
  r44 = 复制内存(r41, r42, r43);
  r45 = cn_var_路径位置_2;
  r46 = cn_var_段长度_6;
  r47 = r45 + r46;
  cn_var_路径位置_2 = r47;
  goto for_update_84;

  for_update_84:
  r48 = cn_var_j_4;
  r49 = r48 + 1;
  cn_var_j_4 = r49;
  goto for_cond_82;

  for_exit_85:
  r50 = cn_var_路径缓冲区;
  r51 = cn_var_路径位置_2;
  r52 = r50 + r51;
  r53 = 复制字符串(r52, ".cn");
  r54 = cn_var_路径位置_2;
  r55 = r54 + 3;
  cn_var_路径位置_2 = r55;
  r56 = cn_var_路径缓冲区;
  r57 = cn_var_路径位置_2;
  r58 = &r56[r57];
  r58 = 0;
  void* cn_var_文件_7;
  r59 = cn_var_路径缓冲区;
  r60 = 打开文件(r59, "r");
  cn_var_文件_7 = r60;
  r61 = cn_var_文件_7;
  r62 = r61 != 0;
  if (r62) goto if_then_86; else goto if_merge_87;

  if_then_86:
  r63 = cn_var_文件_7;
  关闭文件(r63);
  r64 = cn_var_路径位置_2;
  r65 = cn_var_输出路径大小;
  r66 = r64 < r65;
  if (r66) goto if_then_88; else goto if_merge_89;

  if_merge_87:
  goto for_update_80;

  if_then_88:
  r67 = cn_var_输出路径;
  r68 = cn_var_路径缓冲区;
  r69 = 复制字符串(r67, r68);
  return 1;
  goto if_merge_89;

  if_merge_89:
  goto if_merge_87;
  return 0;
}

long long 计算哈希值(char* cn_var_字符串指针) {
  long long r2, r4, r7, r8, r10, r12, r13, r14, r15, r17, r18, r19;
  char* r0;
  char* r3;
  char* r9;
  char* r5;
  char* r11;
  _Bool r1;
  _Bool r6;
  _Bool r16;

  entry:
  long long cn_var_哈希_0;
  cn_var_哈希_0 = 0;
  r0 = cn_var_字符串指针;
  r1 = r0 == 0;
  if (r1) goto if_then_90; else goto if_merge_91;

  if_then_90:
  r2 = cn_var_哈希_0;
  return r2;
  goto if_merge_91;

  if_merge_91:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_92;

  for_cond_92:
  r3 = cn_var_字符串指针;
  r4 = cn_var_i_1;
  r5 = (void**)cn_rt_array_get_element(r3, r4, 8);
  r6 = r5 != 0;
  if (r6) goto for_body_93; else goto for_exit_95;

  for_body_93:
  r7 = cn_var_哈希_0;
  r8 = r7 * 31;
  r9 = cn_var_字符串指针;
  r10 = cn_var_i_1;
  r11 = (void**)cn_rt_array_get_element(r9, r10, 8);
  r12 = r8 + r11;
  cn_var_哈希_0 = r12;
  goto for_update_94;

  for_update_94:
  r13 = cn_var_i_1;
  r14 = r13 + 1;
  cn_var_i_1 = r14;
  goto for_cond_92;

  for_exit_95:
  r15 = cn_var_哈希_0;
  r16 = r15 < 0;
  if (r16) goto if_then_96; else goto if_merge_97;

  if_then_96:
  r17 = cn_var_哈希_0;
  r18 = -r17;
  cn_var_哈希_0 = r18;
  goto if_merge_97;

  if_merge_97:
  r19 = cn_var_哈希_0;
  return r19;
}

struct 模块元数据* 从缓存获取模块(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r6, r7, r8, r9, r12, r19;
  char* r3;
  char* r5;
  char* r17;
  char* r18;
  void* r11;
  struct 模块加载器* r1;
  struct 模块加载器* r10;
  struct 模块缓存条目* r13;
  struct 模块缓存条目* r14;
  struct 模块缓存条目* r16;
  struct 模块缓存条目* r21;
  struct 模块元数据* r22;
  struct 模块缓存条目* r23;
  struct 模块缓存条目* r24;
  _Bool r2;
  _Bool r4;
  _Bool r15;
  _Bool r20;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_101; else goto logic_rhs_100;

  if_then_98:
  return 0;
  goto if_merge_99;

  if_merge_99:
  long long cn_var_哈希_0;
  r5 = cn_var_模块名;
  r6 = 计算哈希值(r5);
  cn_var_哈希_0 = r6;
  long long cn_var_索引_1;
  r7 = cn_var_哈希_0;
  r8 = cn_var_最大模块缓存数量;
  r9 = r7 % r8;
  cn_var_索引_1 = r9;
  struct 模块缓存条目* cn_var_条目_2;
  r10 = cn_var_加载器;
  r11 = r10->缓存列表;
  r12 = cn_var_索引_1;
  r13 = &r11[r12];
  cn_var_条目_2 = r13;
  goto while_cond_102;

  logic_rhs_100:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_101;

  logic_merge_101:
  if (r4) goto if_then_98; else goto if_merge_99;

  while_cond_102:
  r14 = cn_var_条目_2;
  r15 = r14 != 0;
  if (r15) goto while_body_103; else goto while_exit_104;

  while_body_103:
  r16 = cn_var_条目_2;
  r17 = r16->模块名;
  r18 = cn_var_模块名;
  r19 = 比较字符串(r17, r18);
  r20 = r19 == 0;
  if (r20) goto if_then_105; else goto if_merge_106;

  while_exit_104:
  return 0;

  if_then_105:
  r21 = cn_var_条目_2;
  r22 = r21->元数据;
  return r22;
  goto if_merge_106;

  if_merge_106:
  r23 = cn_var_条目_2;
  r24 = r23->下一个;
  cn_var_条目_2 = r24;
  goto while_cond_102;
  return NULL;
}

_Bool 添加模块到缓存(struct 模块加载器* cn_var_加载器, char* cn_var_模块名, struct 模块元数据* cn_var_元数据) {
  long long r0, r1, r9, r10, r11, r12, r13, r18, r19, r20, r33, r38, r41, r42;
  char* r4;
  char* r8;
  char* r17;
  char* r23;
  char* r27;
  char* r28;
  char* r29;
  void* r32;
  void* r37;
  struct 模块加载器* r2;
  struct 模块元数据* r6;
  void* r14;
  struct 模块缓存条目* r15;
  void* r21;
  struct 模块缓存条目* r22;
  struct 模块缓存条目* r25;
  struct 模块缓存条目* r26;
  struct 模块元数据* r30;
  struct 模块加载器* r31;
  struct 模块缓存条目* r34;
  struct 模块缓存条目* r35;
  struct 模块加载器* r36;
  struct 模块缓存条目** r39;
  struct 模块加载器* r40;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r16;
  _Bool r24;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_112; else goto logic_rhs_111;

  if_then_107:
  return 0;
  goto if_merge_108;

  if_merge_108:
  long long cn_var_哈希_0;
  r8 = cn_var_模块名;
  r9 = 计算哈希值(r8);
  cn_var_哈希_0 = r9;
  long long cn_var_索引_1;
  r10 = cn_var_哈希_0;
  r11 = cn_var_最大模块缓存数量;
  r12 = r10 % r11;
  cn_var_索引_1 = r12;
  struct 模块缓存条目* cn_var_新条目_2;
  r13 = 模块缓存条目大小();
  r14 = 分配清零内存(1, r13);
  cn_var_新条目_2 = (struct 模块缓存条目*)0;
  r15 = cn_var_新条目_2;
  r16 = r15 == 0;
  if (r16) goto if_then_113; else goto if_merge_114;

  logic_rhs_109:
  r6 = cn_var_元数据;
  r7 = r6 == 0;
  goto logic_merge_110;

  logic_merge_110:
  if (r7) goto if_then_107; else goto if_merge_108;

  logic_rhs_111:
  r4 = cn_var_模块名;
  r5 = r4 == 0;
  goto logic_merge_112;

  logic_merge_112:
  if (r5) goto logic_merge_110; else goto logic_rhs_109;

  if_then_113:
  return 0;
  goto if_merge_114;

  if_merge_114:
  long long cn_var_名称长度_3;
  r17 = cn_var_模块名;
  r18 = 获取字符串长度(r17);
  cn_var_名称长度_3 = r18;
  r19 = cn_var_名称长度_3;
  r20 = r19 + 1;
  r21 = 分配内存(r20);
  r22 = cn_var_新条目_2;
  r23 = r22->模块名;
  r24 = r23 == 0;
  if (r24) goto if_then_115; else goto if_merge_116;

  if_then_115:
  r25 = cn_var_新条目_2;
  释放内存(r25);
  return 0;
  goto if_merge_116;

  if_merge_116:
  r26 = cn_var_新条目_2;
  r27 = r26->模块名;
  r28 = cn_var_模块名;
  r29 = 复制字符串(r27, r28);
  r30 = cn_var_元数据;
  r31 = cn_var_加载器;
  r32 = r31->缓存列表;
  r33 = cn_var_索引_1;
  r34 = &r32[r33];
  r35 = cn_var_新条目_2;
  r36 = cn_var_加载器;
  r37 = r36->缓存列表;
  r38 = cn_var_索引_1;
  r39 = &r37[r38];
  r39 = r35;
  r40 = cn_var_加载器;
  r41 = r40->缓存数量;
  r42 = r41 + 1;
  return 1;
}

_Bool 读取文件内容(char* cn_var_文件路径, char** cn_var_输出缓冲区, long long* cn_var_输出大小) {
  long long r0, r1, r13, r14, r17, r18, r25, r26, r28, r29, r33, r35;
  char* r2;
  char* r8;
  char* r20;
  char* r24;
  char* r31;
  char* r32;
  char* r34;
  char** r4;
  long long* r6;
  void* r9;
  void* r10;
  void* r12;
  void* r16;
  void* r19;
  void* r22;
  void* r23;
  void* r27;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r15;
  _Bool r21;
  _Bool r30;

  entry:
  r2 = cn_var_文件路径;
  r3 = r2 == 0;
  if (r3) goto logic_merge_122; else goto logic_rhs_121;

  if_then_117:
  return 0;
  goto if_merge_118;

  if_merge_118:
  void* cn_var_文件_0;
  r8 = cn_var_文件路径;
  r9 = 打开文件(r8, "rb");
  cn_var_文件_0 = r9;
  r10 = cn_var_文件_0;
  r11 = r10 == 0;
  if (r11) goto if_then_123; else goto if_merge_124;

  logic_rhs_119:
  r6 = cn_var_输出大小;
  r7 = r6 == 0;
  goto logic_merge_120;

  logic_merge_120:
  if (r7) goto if_then_117; else goto if_merge_118;

  logic_rhs_121:
  r4 = cn_var_输出缓冲区;
  r5 = r4 == 0;
  goto logic_merge_122;

  logic_merge_122:
  if (r5) goto logic_merge_120; else goto logic_rhs_119;

  if_then_123:
  return 0;
  goto if_merge_124;

  if_merge_124:
  long long cn_var_文件大小_1;
  r12 = cn_var_文件_0;
  r13 = 获取文件大小(r12);
  cn_var_文件大小_1 = r13;
  r14 = cn_var_文件大小_1;
  r15 = r14 <= 0;
  if (r15) goto if_then_125; else goto if_merge_126;

  if_then_125:
  r16 = cn_var_文件_0;
  关闭文件(r16);
  return 0;
  goto if_merge_126;

  if_merge_126:
  char* cn_var_缓冲区_2;
  r17 = cn_var_文件大小_1;
  r18 = r17 + 1;
  r19 = 分配内存(r18);
  cn_var_缓冲区_2 = (char*)0;
  r20 = cn_var_缓冲区_2;
  r21 = r20 == 0;
  if (r21) goto if_then_127; else goto if_merge_128;

  if_then_127:
  r22 = cn_var_文件_0;
  关闭文件(r22);
  return 0;
  goto if_merge_128;

  if_merge_128:
  long long cn_var_读取字节数_3;
  r23 = cn_var_文件_0;
  r24 = cn_var_缓冲区_2;
  r25 = cn_var_文件大小_1;
  r26 = 读取文件(r23, r24, r25);
  cn_var_读取字节数_3 = r26;
  r27 = cn_var_文件_0;
  关闭文件(r27);
  r28 = cn_var_读取字节数_3;
  r29 = cn_var_文件大小_1;
  r30 = r28 != r29;
  if (r30) goto if_then_129; else goto if_merge_130;

  if_then_129:
  r31 = cn_var_缓冲区_2;
  释放内存(r31);
  return 0;
  goto if_merge_130;

  if_merge_130:
  r32 = cn_var_缓冲区_2;
  r33 = cn_var_文件大小_1;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r32, r33, &_tmp_i0, 8); }
  r34 = cn_var_缓冲区_2;
  r35 = cn_var_文件大小_1;
  return 1;
}

struct 模块元数据* 创建模块元数据(struct 模块标识符* cn_var_标识符) {
  long long r2;
  struct 模块标识符* r0;
  void* r3;
  struct 模块元数据* r4;
  struct 模块标识符* r6;
  struct 模块元数据* r7;
  _Bool r1;
  _Bool r5;

  entry:
  r0 = cn_var_标识符;
  r1 = r0 == 0;
  if (r1) goto if_then_131; else goto if_merge_132;

  if_then_131:
  return 0;
  goto if_merge_132;

  if_merge_132:
  struct 模块元数据* cn_var_元数据_0;
  r2 = 模块元数据大小();
  r3 = 分配清零内存(1, r2);
  cn_var_元数据_0 = (struct 模块元数据*)0;
  r4 = cn_var_元数据_0;
  r5 = r4 == 0;
  if (r5) goto if_then_133; else goto if_merge_134;

  if_then_133:
  return 0;
  goto if_merge_134;

  if_merge_134:
  r6 = cn_var_标识符;
  r7 = cn_var_元数据_0;
  return r7;
}

void 销毁模块元数据(struct 模块元数据* cn_var_元数据) {
  char* r8;
  char* r11;
  struct 模块元数据* r0;
  struct 模块元数据* r2;
  struct 模块标识符* r3;
  struct 模块元数据* r5;
  struct 模块标识符* r6;
  struct 模块元数据* r7;
  struct 模块元数据* r10;
  struct 模块元数据* r12;
  struct 导出符号* r13;
  struct 模块元数据* r15;
  struct 导出符号* r16;
  struct 模块元数据* r17;
  struct 模块依赖* r18;
  struct 模块元数据* r20;
  struct 模块依赖* r21;
  struct 模块元数据* r22;
  _Bool r1;
  _Bool r4;
  _Bool r9;
  _Bool r14;
  _Bool r19;

  entry:
  r0 = cn_var_元数据;
  r1 = r0 == 0;
  if (r1) goto if_then_135; else goto if_merge_136;

  if_then_135:
  return;
  goto if_merge_136;

  if_merge_136:
  r2 = cn_var_元数据;
  r3 = r2->标识符;
  r4 = r3 != 0;
  if (r4) goto if_then_137; else goto if_merge_138;

  if_then_137:
  r5 = cn_var_元数据;
  r6 = r5->标识符;
  销毁模块标识符(r6);
  goto if_merge_138;

  if_merge_138:
  r7 = cn_var_元数据;
  r8 = r7->文件路径;
  r9 = r8 != 0;
  if (r9) goto if_then_139; else goto if_merge_140;

  if_then_139:
  r10 = cn_var_元数据;
  r11 = r10->文件路径;
  释放内存(r11);
  goto if_merge_140;

  if_merge_140:
  r12 = cn_var_元数据;
  r13 = r12->导出符号表;
  r14 = r13 != 0;
  if (r14) goto if_then_141; else goto if_merge_142;

  if_then_141:
  r15 = cn_var_元数据;
  r16 = r15->导出符号表;
  释放内存(r16);
  goto if_merge_142;

  if_merge_142:
  r17 = cn_var_元数据;
  r18 = r17->依赖列表;
  r19 = r18 != 0;
  if (r19) goto if_then_143; else goto if_merge_144;

  if_then_143:
  r20 = cn_var_元数据;
  r21 = r20->依赖列表;
  释放内存(r21);
  goto if_merge_144;

  if_merge_144:
  r22 = cn_var_元数据;
  释放内存(r22);
  return;
}

struct 模块元数据* 加载模块核心(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r12, r13, r37, r38, r40, r41, r42, r49, r50, r55, r59, r67, r68, r70, r77, r78, r89, r90, r100, r101, r111, r112;
  char* r3;
  char* r6;
  char* r17;
  char* r45;
  char* r52;
  char* r54;
  char* r57;
  char* r69;
  char* r74;
  char* r86;
  char* r97;
  char* r105;
  char* r107;
  void* r30;
  void* r39;
  void* r53;
  void* r56;
  struct 模块加载器* r1;
  struct 模块加载器* r5;
  struct 模块元数据* r7;
  struct 模块元数据* r8;
  struct 模块元数据* r10;
  struct 模块加载器* r11;
  struct 模块加载器* r15;
  struct 诊断集合* r16;
  struct 模块标识符* r18;
  struct 模块标识符* r19;
  struct 模块加载器* r21;
  struct 诊断集合* r22;
  struct 模块标识符* r23;
  struct 模块元数据* r24;
  struct 模块元数据* r25;
  struct 模块标识符* r27;
  struct 模块加载器* r28;
  struct 模块标识符* r29;
  struct 模块加载器* r33;
  struct 诊断集合* r34;
  struct 模块元数据* r35;
  struct 模块加载器* r36;
  void* r43;
  struct 模块元数据* r44;
  struct 模块元数据* r47;
  struct 模块加载器* r48;
  struct 模块元数据* r51;
  char** r58;
  long long* r60;
  struct 模块加载器* r63;
  struct 诊断集合* r64;
  struct 模块元数据* r65;
  struct 模块加载器* r66;
  struct 扫描器* r71;
  struct 扫描器* r72;
  struct 模块元数据* r75;
  struct 模块加载器* r76;
  struct 扫描器* r79;
  struct 模块加载器* r80;
  struct 诊断集合* r81;
  struct 解析器* r82;
  struct 解析器* r83;
  struct 扫描器* r85;
  struct 模块元数据* r87;
  struct 模块加载器* r88;
  struct 解析器* r91;
  struct 程序节点* r92;
  struct 程序节点* r93;
  struct 解析器* r95;
  struct 扫描器* r96;
  struct 模块元数据* r98;
  struct 模块加载器* r99;
  struct 程序节点* r102;
  struct 解析器* r103;
  struct 扫描器* r104;
  struct 模块加载器* r106;
  struct 模块元数据* r108;
  struct 模块加载器* r110;
  struct 模块元数据* r113;
  _Bool r2;
  _Bool r4;
  _Bool r9;
  _Bool r14;
  _Bool r20;
  _Bool r26;
  _Bool r31;
  _Bool r32;
  _Bool r46;
  _Bool r61;
  _Bool r62;
  _Bool r73;
  _Bool r84;
  _Bool r94;
  _Bool r109;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_148; else goto logic_rhs_147;

  if_then_145:
  return 0;
  goto if_merge_146;

  if_merge_146:
  struct 模块元数据* cn_var_缓存模块_0;
  r5 = cn_var_加载器;
  r6 = cn_var_模块名;
  r7 = 从缓存获取模块(r5, r6);
  cn_var_缓存模块_0 = r7;
  r8 = cn_var_缓存模块_0;
  r9 = r8 != 0;
  if (r9) goto if_then_149; else goto if_merge_150;

  logic_rhs_147:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_148;

  logic_merge_148:
  if (r4) goto if_then_145; else goto if_merge_146;

  if_then_149:
  r10 = cn_var_缓存模块_0;
  return r10;
  goto if_merge_150;

  if_merge_150:
  r11 = cn_var_加载器;
  r12 = r11->当前依赖深度;
  r13 = cn_var_最大依赖深度;
  r14 = r12 >= r13;
  if (r14) goto if_then_151; else goto if_merge_152;

  if_then_151:
  r15 = cn_var_加载器;
  r16 = r15->诊断集合;
  报告错误(r16, 诊断错误码_未知错误, 0, "模块依赖深度超过限制");
  return 0;
  goto if_merge_152;

  if_merge_152:
  struct 模块标识符* cn_var_标识符_1;
  r17 = cn_var_模块名;
  r18 = 解析模块标识符(r17);
  cn_var_标识符_1 = r18;
  r19 = cn_var_标识符_1;
  r20 = r19 == 0;
  if (r20) goto if_then_153; else goto if_merge_154;

  if_then_153:
  r21 = cn_var_加载器;
  r22 = r21->诊断集合;
  报告错误(r22, 诊断错误码_未知错误, 0, "无法解析模块标识符");
  return 0;
  goto if_merge_154;

  if_merge_154:
  struct 模块元数据* cn_var_元数据_2;
  r23 = cn_var_标识符_1;
  r24 = 创建模块元数据(r23);
  cn_var_元数据_2 = r24;
  r25 = cn_var_元数据_2;
  r26 = r25 == 0;
  if (r26) goto if_then_155; else goto if_merge_156;

  if_then_155:
  r27 = cn_var_标识符_1;
  销毁模块标识符(r27);
  return 0;
  goto if_merge_156;

  if_merge_156:
  char* cn_var_文件路径缓冲区;
  cn_var_文件路径缓冲区 = cn_rt_array_alloc(8, 1024);
  r28 = cn_var_加载器;
  r29 = cn_var_标识符_1;
  r30 = cn_var_文件路径缓冲区;
  r31 = 解析模块文件路径(r28, r29, r30, 1024);
  r32 = !r31;
  if (r32) goto if_then_157; else goto if_merge_158;

  if_then_157:
  r33 = cn_var_加载器;
  r34 = r33->诊断集合;
  报告错误(r34, 诊断错误码_未知错误, 0, "无法找到模块文件");
  r35 = cn_var_元数据_2;
  销毁模块元数据(r35);
  r36 = cn_var_加载器;
  r37 = r36->加载失败数;
  r38 = r37 + 1;
  return 0;
  goto if_merge_158;

  if_merge_158:
  long long cn_var_路径长度_3;
  r39 = cn_var_文件路径缓冲区;
  r40 = 获取字符串长度(r39);
  cn_var_路径长度_3 = r40;
  r41 = cn_var_路径长度_3;
  r42 = r41 + 1;
  r43 = 分配内存(r42);
  r44 = cn_var_元数据_2;
  r45 = r44->文件路径;
  r46 = r45 == 0;
  if (r46) goto if_then_159; else goto if_merge_160;

  if_then_159:
  r47 = cn_var_元数据_2;
  销毁模块元数据(r47);
  r48 = cn_var_加载器;
  r49 = r48->加载失败数;
  r50 = r49 + 1;
  return 0;
  goto if_merge_160;

  if_merge_160:
  r51 = cn_var_元数据_2;
  r52 = r51->文件路径;
  r53 = cn_var_文件路径缓冲区;
  r54 = 复制字符串(r52, r53);
  r55 = cn_var_路径长度_3;
  char* cn_var_源码内容_4;
  cn_var_源码内容_4 = 0;
  long long cn_var_源码大小_5;
  cn_var_源码大小_5 = 0;
  r56 = cn_var_文件路径缓冲区;
  r57 = cn_var_源码内容_4;
  r58 = &cn_var_源码内容_4;
  r59 = cn_var_源码大小_5;
  r60 = &cn_var_源码大小_5;
  r61 = 读取文件内容(r56, r58, r60);
  r62 = !r61;
  if (r62) goto if_then_161; else goto if_merge_162;

  if_then_161:
  r63 = cn_var_加载器;
  r64 = r63->诊断集合;
  报告错误(r64, 诊断错误码_未知错误, 0, "无法读取模块文件");
  r65 = cn_var_元数据_2;
  销毁模块元数据(r65);
  r66 = cn_var_加载器;
  r67 = r66->加载失败数;
  r68 = r67 + 1;
  return 0;
  goto if_merge_162;

  if_merge_162:
  struct 扫描器* cn_var_扫描器实例_6;
  r69 = cn_var_源码内容_4;
  r70 = cn_var_源码大小_5;
  r71 = 创建扫描器(r69, r70);
  cn_var_扫描器实例_6 = r71;
  r72 = cn_var_扫描器实例_6;
  r73 = r72 == 0;
  if (r73) goto if_then_163; else goto if_merge_164;

  if_then_163:
  r74 = cn_var_源码内容_4;
  释放内存(r74);
  r75 = cn_var_元数据_2;
  销毁模块元数据(r75);
  r76 = cn_var_加载器;
  r77 = r76->加载失败数;
  r78 = r77 + 1;
  return 0;
  goto if_merge_164;

  if_merge_164:
  struct 解析器* cn_var_解析器实例_7;
  r79 = cn_var_扫描器实例_6;
  r80 = cn_var_加载器;
  r81 = r80->诊断集合;
  r82 = 创建解析器(r79, r81);
  cn_var_解析器实例_7 = r82;
  r83 = cn_var_解析器实例_7;
  r84 = r83 == 0;
  if (r84) goto if_then_165; else goto if_merge_166;

  if_then_165:
  r85 = cn_var_扫描器实例_6;
  销毁扫描器(r85);
  r86 = cn_var_源码内容_4;
  释放内存(r86);
  r87 = cn_var_元数据_2;
  销毁模块元数据(r87);
  r88 = cn_var_加载器;
  r89 = r88->加载失败数;
  r90 = r89 + 1;
  return 0;
  goto if_merge_166;

  if_merge_166:
  struct 程序节点* cn_var_程序_8;
  r91 = cn_var_解析器实例_7;
  r92 = 解析程序(r91);
  cn_var_程序_8 = r92;
  r93 = cn_var_程序_8;
  r94 = r93 == 0;
  if (r94) goto if_then_167; else goto if_merge_168;

  if_then_167:
  r95 = cn_var_解析器实例_7;
  销毁解析器(r95);
  r96 = cn_var_扫描器实例_6;
  销毁扫描器(r96);
  r97 = cn_var_源码内容_4;
  释放内存(r97);
  r98 = cn_var_元数据_2;
  销毁模块元数据(r98);
  r99 = cn_var_加载器;
  r100 = r99->加载失败数;
  r101 = r100 + 1;
  return 0;
  goto if_merge_168;

  if_merge_168:
  r102 = cn_var_程序_8;
  r103 = cn_var_解析器实例_7;
  销毁解析器(r103);
  r104 = cn_var_扫描器实例_6;
  销毁扫描器(r104);
  r105 = cn_var_源码内容_4;
  释放内存(r105);
  r106 = cn_var_加载器;
  r107 = cn_var_模块名;
  r108 = cn_var_元数据_2;
  r109 = 添加模块到缓存(r106, r107, r108);
  r110 = cn_var_加载器;
  r111 = r110->加载成功数;
  r112 = r111 + 1;
  r113 = cn_var_元数据_2;
  return r113;
}

struct 模块元数据* 加载模块(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r8, r10, r11, r16;
  char* r3;
  char* r13;
  struct 模块加载器* r1;
  struct 模块加载器* r5;
  struct 模块元数据* r6;
  struct 模块加载器* r7;
  struct 模块加载器* r9;
  struct 模块加载器* r12;
  struct 模块元数据* r14;
  struct 模块元数据* r15;
  struct 模块元数据* r17;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_172; else goto logic_rhs_171;

  if_then_169:
  return 0;
  goto if_merge_170;

  if_merge_170:
  struct 模块元数据* cn_var_保存当前模块_0;
  r5 = cn_var_加载器;
  r6 = r5->当前模块;
  cn_var_保存当前模块_0 = r6;
  long long cn_var_保存依赖深度_1;
  r7 = cn_var_加载器;
  r8 = r7->当前依赖深度;
  cn_var_保存依赖深度_1 = r8;
  r9 = cn_var_加载器;
  r10 = r9->当前依赖深度;
  r11 = r10 + 1;
  struct 模块元数据* cn_var_结果_2;
  r12 = cn_var_加载器;
  r13 = cn_var_模块名;
  r14 = 加载模块核心(r12, r13);
  cn_var_结果_2 = r14;
  r15 = cn_var_保存当前模块_0;
  r16 = cn_var_保存依赖深度_1;
  r17 = cn_var_结果_2;
  return r17;

  logic_rhs_171:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_172;

  logic_merge_172:
  if (r4) goto if_then_169; else goto if_merge_170;
  return NULL;
}

_Bool 处理导入语句(struct 模块加载器* cn_var_加载器, struct 导入声明* cn_var_导入节点, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r18, r20, r23, r24, r25, r29, r30, r31, r36, r40, r46, r47;
  char* r9;
  char* r10;
  char* r13;
  struct 模块加载器* r2;
  struct 导入声明* r4;
  struct 模块元数据* r6;
  struct 导入声明* r8;
  struct 模块加载器* r12;
  struct 模块元数据* r14;
  struct 模块元数据* r15;
  struct 模块元数据* r17;
  struct 模块元数据* r19;
  struct 模块元数据* r22;
  struct 模块元数据* r27;
  struct 模块依赖* r28;
  void* r32;
  struct 模块依赖* r33;
  struct 模块依赖* r35;
  struct 模块元数据* r37;
  struct 模块依赖* r38;
  struct 模块元数据* r39;
  struct 模块依赖* r41;
  struct 模块依赖* r42;
  struct 模块元数据* r43;
  struct 模块标识符* r44;
  struct 模块元数据* r45;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r16;
  _Bool r21;
  _Bool r26;
  _Bool r34;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_178; else goto logic_rhs_177;

  if_then_173:
  return 0;
  goto if_merge_174;

  if_merge_174:
  char* cn_var_模块名_0;
  r8 = cn_var_导入节点;
  r9 = r8->模块名;
  cn_var_模块名_0 = r9;
  r10 = cn_var_模块名_0;
  r11 = r10 == 0;
  if (r11) goto if_then_179; else goto if_merge_180;

  logic_rhs_175:
  r6 = cn_var_当前模块;
  r7 = r6 == 0;
  goto logic_merge_176;

  logic_merge_176:
  if (r7) goto if_then_173; else goto if_merge_174;

  logic_rhs_177:
  r4 = cn_var_导入节点;
  r5 = r4 == 0;
  goto logic_merge_178;

  logic_merge_178:
  if (r5) goto logic_merge_176; else goto logic_rhs_175;

  if_then_179:
  return 0;
  goto if_merge_180;

  if_merge_180:
  struct 模块元数据* cn_var_依赖模块_1;
  r12 = cn_var_加载器;
  r13 = cn_var_模块名_0;
  r14 = 加载模块(r12, r13);
  cn_var_依赖模块_1 = r14;
  r15 = cn_var_依赖模块_1;
  r16 = r15 == 0;
  if (r16) goto if_then_181; else goto if_merge_182;

  if_then_181:
  return 0;
  goto if_merge_182;

  if_merge_182:
  r17 = cn_var_当前模块;
  r18 = r17->依赖数量;
  r19 = cn_var_当前模块;
  r20 = r19->依赖容量;
  r21 = r18 >= r20;
  if (r21) goto if_then_183; else goto if_merge_184;

  if_then_183:
  long long cn_var_新容量_2;
  r22 = cn_var_当前模块;
  r23 = r22->依赖容量;
  r24 = r23 << 1;
  cn_var_新容量_2 = r24;
  r25 = cn_var_新容量_2;
  r26 = r25 == 0;
  if (r26) goto if_then_185; else goto if_merge_186;

  if_merge_184:
  struct 模块依赖* cn_var_依赖_4;
  r37 = cn_var_当前模块;
  r38 = r37->依赖列表;
  r39 = cn_var_当前模块;
  r40 = r39->依赖数量;
  r41 = (struct 模块依赖 {
    模块标识符* 模块标识;    
    布尔 是必需;             
    布尔 是循环;             
}






结构体 模块元数据 {
    模块标识符* 标识符;      
    模块类型 类型;           
    模块加载状态 状态;       
    
    
    字符串 文件路径;         
    整数 文件路径长度;       
    
    
    导出符号* 导出符号表;    
    整数 导出符号数量;       
    整数 导出符号容量;       
    
    
    模块依赖* 依赖列表;      
    整数 依赖数量;           
    整数 依赖容量;           
    
    
    程序节点* 程序节点;      
    
    
    符号表管理器* 符号表;    
}






结构体 模块缓存条目 {
    字符串 模块名;           
    模块元数据* 元数据;      
    模块缓存条目* 下一个;    
}







结构体 模块加载器 {
    
    字符串[32] 搜索路径列表;  
    整数 搜索路径数量;                      
    
    
    模块缓存条目*[256] 缓存列表;  
    整数 缓存数量;                            
    
    
    诊断集合* 诊断集合;                      
    
    
    模块元数据* 当前模块;       
    整数 当前依赖深度;          
    
    
    整数 加载成功数;            
    整数 加载失败数;            
}








函数 创建模块加载器(诊断集合* 诊断集合指针) -> 模块加载器* {
    模块加载器* 加载器 = (模块加载器*)分配清零内存(1, 模块加载器大小());
    如果 (加载器 == 无) {
        返回 无;
    }
    
    加载器.诊断集合 = 诊断集合指针;
    加载器.搜索路径数量 = 0;
    加载器.缓存数量 = 0;
    加载器.当前模块 = 无;
    加载器.当前依赖深度 = 0;
    加载器.加载成功数 = 0;
    加载器.加载失败数 = 0;
    
    返回 加载器;
}



函数 销毁模块加载器(模块加载器* 加载器) -> 空类型 {
    如果 (加载器 == 无) {
        返回;
    }
    
    
    循环 (整数 i = 0; i < 加载器.搜索路径数量; i = i + 1) {
        如果 (加载器.搜索路径列表[i] != 无) {
            释放内存(加载器.搜索路径列表[i]);
        }
    }
    
    
    循环 (整数 i = 0; i < 最大模块缓存数量; i = i + 1) {
        模块缓存条目* 条目 = 加载器.缓存列表[i];
        当 (条目 != 无) {
            模块缓存条目* 下一个 = 条目.下一个;
            释放内存(条目.模块名);
            
            释放内存(条目);
            条目 = 下一个;
        }
    }
    
    释放内存(加载器);
}









函数 添加搜索路径(模块加载器* 加载器, 字符串 路径) -> 布尔 {
    如果 (加载器 == 无 || 路径 == 无) {
        返回 假;
    }
    
    如果 (加载器.搜索路径数量 >= 最大搜索路径数量) {
        返回 假;
    }
    
    
    整数 路径长度 = 获取字符串长度(路径);
    字符串 路径副本 = (字符串)分配内存(路径长度 + 1);
    如果 (路径副本 == 无) {
        返回 假;
    }
    复制字符串(路径副本, 路径);
    
    加载器.搜索路径列表[加载器.搜索路径数量] = 路径副本;
    加载器.搜索路径数量 = 加载器.搜索路径数量 + 1;
    
    返回 真;
}




函数 设置默认搜索路径(模块加载器* 加载器, 字符串 源文件目录) -> 空类型 {
    如果 (加载器 == 无) {
        返回;
    }
    
    
    添加搜索路径(加载器, 源文件目录);
    
    
    添加搜索路径(加载器, ".");
    
    
    添加搜索路径(加载器, "标准库");
    添加搜索路径(加载器, "stdlib");
}








函数 解析模块标识符(字符串 模块名) -> 模块标识符* {
    如果 (模块名 == 无) {
        返回 无;
    }
    
    整数 名称长度 = 获取字符串长度(模块名);
    如果 (名称长度 == 0 || 名称长度 >= 最大模块路径长度) {
        返回 无;
    }
    
    模块标识符* 标识符 = (模块标识符*)分配清零内存(1, 模块标识符大小());
    如果 (标识符 == 无) {
        返回 无;
    }
    
    
    标识符.完全限定名 = (字符串)分配内存(名称长度 + 1);
    如果 (标识符.完全限定名 == 无) {
        释放内存(标识符);
        返回 无;
    }
    复制字符串(标识符.完全限定名, 模块名);
    标识符.完全限定名长度 = 名称长度;
    
    
    整数 段数量 = 1;
    循环 (整数 i = 0; i < 名称长度; i = i + 1) {
        如果 (模块名[i] == '.') {
            段数量 = 段数量 + 1;
        }
    }
    
    
    标识符.路径段列表 = (字符串*)分配清零内存(段数量, 字符串指针大小());
    如果 (标识符.路径段列表 == 无) {
        释放内存(标识符.完全限定名);
        释放内存(标识符);
        返回 无;
    }
    标识符.路径段数量 = 段数量;
    
    
    字符 缓冲区[1024];
    整数 缓冲区位置 = 0;
    整数 当前段索引 = 0;
    
    循环 (整数 i = 0; i <= 名称长度; i = i + 1) {
        如果 (模块名[i] == '.' || 模块名[i] == '\0') {
            缓冲区[缓冲区位置] = '\0';
            
            
            整数 段长度 = 获取字符串长度(缓冲区);
            字符串 段副本 = (字符串)分配内存(段长度 + 1);
            如果 (段副本 == 无) {
                
                循环 (整数 j = 0; j < 当前段索引; j = j + 1) {
                    释放内存(标识符.路径段列表[j]);
                }
                释放内存(标识符.路径段列表);
                释放内存(标识符.完全限定名);
                释放内存(标识符);
                返回 无;
            }
            复制字符串(段副本, 缓冲区);
            标识符.路径段列表[当前段索引] = 段副本;
            当前段索引 = 当前段索引 + 1;
            缓冲区位置 = 0;
        } 否则 {
            缓冲区[缓冲区位置] = 模块名[i];
            缓冲区位置 = 缓冲区位置 + 1;
        }
    }
    
    返回 标识符;
}



函数 销毁模块标识符(模块标识符* 标识符) -> 空类型 {
    如果 (标识符 == 无) {
        返回;
    }
    
    如果 (标识符.完全限定名 != 无) {
        释放内存(标识符.完全限定名);
    }
    
    如果 (标识符.路径段列表 != 无) {
        循环 (整数 i = 0; i < 标识符.路径段数量; i = i + 1) {
            如果 (标识符.路径段列表[i] != 无) {
                释放内存(标识符.路径段列表[i]);
            }
        }
        释放内存(标识符.路径段列表);
    }
    
    释放内存(标识符);
}











函数 解析模块文件路径(模块加载器* 加载器, 模块标识符* 标识符, 字符串 输出路径, 整数 输出路径大小) -> 布尔 {
    如果 (加载器 == 无 || 标识符 == 无 || 输出路径 == 无) {
        返回 假;
    }
    
    
    循环 (整数 i = 0; i < 加载器.搜索路径数量; i = i + 1) {
        字符串 搜索路径 = 加载器.搜索路径列表[i];
        
        
        字符 路径缓冲区[1024];
        整数 路径位置 = 0;
        
        
        整数 搜索路径长度 = 获取字符串长度(搜索路径);
        复制内存(路径缓冲区, 搜索路径, 搜索路径长度);
        路径位置 = 搜索路径长度;
        
        
        循环 (整数 j = 0; j < 标识符.路径段数量; j = j + 1) {
            路径缓冲区[路径位置] = '/';
            路径位置 = 路径位置 + 1;
            
            字符串 段 = 标识符.路径段列表[j];
            整数 段长度 = 获取字符串长度(段);
            复制内存(路径缓冲区 + 路径位置, 段, 段长度);
            路径位置 = 路径位置 + 段长度;
        }
        
        
        复制字符串(路径缓冲区 + 路径位置, ".cn");
        路径位置 = 路径位置 + 3;
        路径缓冲区[路径位置] = '\0';
        
        
        空类型* 文件 = 打开文件(路径缓冲区, "r");
        如果 (文件 != 无) {
            关闭文件(文件);
            如果 (路径位置 < 输出路径大小) {
                复制字符串(输出路径, 路径缓冲区);
                返回 真;
            }
        }
    }
    
    返回 假;
}








函数 计算哈希值(字符串 字符串指针) -> 整数 {
    整数 哈希 = 0;
    如果 (字符串指针 == 无) {
        返回 哈希;
    }
    
    循环 (整数 i = 0; 字符串指针[i] != '\0'; i = i + 1) {
        哈希 = 哈希 * 31 + 字符串指针[i];
    }
    
    如果 (哈希 < 0) {
        哈希 = -哈希;
    }
    
    返回 哈希;
}





函数 从缓存获取模块(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    整数 哈希 = 计算哈希值(模块名);
    整数 索引 = 哈希 % 最大模块缓存数量;
    
    模块缓存条目* 条目 = 加载器.缓存列表[索引];
    当 (条目 != 无) {
        如果 (比较字符串(条目.模块名, 模块名) == 0) {
            返回 条目.元数据;
        }
        条目 = 条目.下一个;
    }
    
    返回 无;
}






函数 添加模块到缓存(模块加载器* 加载器, 字符串 模块名, 模块元数据* 元数据) -> 布尔 {
    如果 (加载器 == 无 || 模块名 == 无 || 元数据 == 无) {
        返回 假;
    }
    
    整数 哈希 = 计算哈希值(模块名);
    整数 索引 = 哈希 % 最大模块缓存数量;
    
    
    模块缓存条目* 新条目 = (模块缓存条目*)分配清零内存(1, 模块缓存条目大小());
    如果 (新条目 == 无) {
        返回 假;
    }
    
    
    整数 名称长度 = 获取字符串长度(模块名);
    新条目.模块名 = (字符串)分配内存(名称长度 + 1);
    如果 (新条目.模块名 == 无) {
        释放内存(新条目);
        返回 假;
    }
    复制字符串(新条目.模块名, 模块名);
    
    新条目.元数据 = 元数据;
    新条目.下一个 = 加载器.缓存列表[索引];
    加载器.缓存列表[索引] = 新条目;
    加载器.缓存数量 = 加载器.缓存数量 + 1;
    
    返回 真;
}










函数 读取文件内容(字符串 文件路径, 字符串* 输出缓冲区, 整数* 输出大小) -> 布尔 {
    如果 (文件路径 == 无 || 输出缓冲区 == 无 || 输出大小 == 无) {
        返回 假;
    }
    
    空类型* 文件 = 打开文件(文件路径, "rb");
    如果 (文件 == 无) {
        返回 假;
    }
    
    
    整数 文件大小 = 获取文件大小(文件);
    如果 (文件大小 <= 0) {
        关闭文件(文件);
        返回 假;
    }
    
    
    字符串 缓冲区 = (字符串)分配内存(文件大小 + 1);
    如果 (缓冲区 == 无) {
        关闭文件(文件);
        返回 假;
    }
    
    
    整数 读取字节数 = 读取文件(文件, 缓冲区, 文件大小);
    关闭文件(文件);
    
    如果 (读取字节数 != 文件大小) {
        释放内存(缓冲区);
        返回 假;
    }
    
    缓冲区[文件大小] = '\0';
    *输出缓冲区 = 缓冲区;
    *输出大小 = 文件大小;
    
    返回 真;
}








函数 创建模块元数据(模块标识符* 标识符) -> 模块元数据* {
    如果 (标识符 == 无) {
        返回 无;
    }
    
    模块元数据* 元数据 = (模块元数据*)分配清零内存(1, 模块元数据大小());
    如果 (元数据 == 无) {
        返回 无;
    }
    
    元数据.标识符 = 标识符;
    元数据.类型 = 文件模块;
    元数据.状态 = 未加载;
    元数据.文件路径 = 无;
    元数据.文件路径长度 = 0;
    元数据.导出符号表 = 无;
    元数据.导出符号数量 = 0;
    元数据.导出符号容量 = 0;
    元数据.依赖列表 = 无;
    元数据.依赖数量 = 0;
    元数据.依赖容量 = 0;
    元数据.程序节点 = 无;
    元数据.符号表 = 无;
    
    返回 元数据;
}



函数 销毁模块元数据(模块元数据* 元数据) -> 空类型 {
    如果 (元数据 == 无) {
        返回;
    }
    
    如果 (元数据.标识符 != 无) {
        销毁模块标识符(元数据.标识符);
    }
    
    如果 (元数据.文件路径 != 无) {
        释放内存(元数据.文件路径);
    }
    
    如果 (元数据.导出符号表 != 无) {
        释放内存(元数据.导出符号表);
    }
    
    如果 (元数据.依赖列表 != 无) {
        释放内存(元数据.依赖列表);
    }
    
    
    
    释放内存(元数据);
}





函数 加载模块核心(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    
    模块元数据* 缓存模块 = 从缓存获取模块(加载器, 模块名);
    如果 (缓存模块 != 无) {
        返回 缓存模块;
    }
    
    
    如果 (加载器.当前依赖深度 >= 最大依赖深度) {
        报告错误(加载器.诊断集合, 未知错误, 无, "模块依赖深度超过限制");
        返回 无;
    }
    
    
    模块标识符* 标识符 = 解析模块标识符(模块名);
    如果 (标识符 == 无) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法解析模块标识符");
        返回 无;
    }
    
    
    模块元数据* 元数据 = 创建模块元数据(标识符);
    如果 (元数据 == 无) {
        销毁模块标识符(标识符);
        返回 无;
    }
    
    
    元数据.状态 = 加载中;
    
    
    字符 文件路径缓冲区[1024];
    如果 (!解析模块文件路径(加载器, 标识符, 文件路径缓冲区, 1024)) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法找到模块文件");
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    整数 路径长度 = 获取字符串长度(文件路径缓冲区);
    元数据.文件路径 = (字符串)分配内存(路径长度 + 1);
    如果 (元数据.文件路径 == 无) {
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    复制字符串(元数据.文件路径, 文件路径缓冲区);
    元数据.文件路径长度 = 路径长度;
    
    
    字符串 源码内容 = 无;
    整数 源码大小 = 0;
    如果 (!读取文件内容(文件路径缓冲区, &源码内容, &源码大小)) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法读取模块文件");
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    扫描器* 扫描器实例 = 创建扫描器(源码内容, 源码大小);
    如果 (扫描器实例 == 无) {
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    解析器* 解析器实例 = 创建解析器(扫描器实例, 加载器.诊断集合);
    如果 (解析器实例 == 无) {
        销毁扫描器(扫描器实例);
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    程序节点* 程序 = 解析程序(解析器实例);
    如果 (程序 == 无) {
        销毁解析器(解析器实例);
        销毁扫描器(扫描器实例);
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    元数据.程序节点 = 程序;
    
    
    销毁解析器(解析器实例);
    销毁扫描器(扫描器实例);
    释放内存(源码内容);
    
    
    元数据.状态 = 已加载;
    
    
    添加模块到缓存(加载器, 模块名, 元数据);
    加载器.加载成功数 = 加载器.加载成功数 + 1;
    
    返回 元数据;
}





函数 加载模块(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    
    模块元数据* 保存当前模块 = 加载器.当前模块;
    整数 保存依赖深度 = 加载器.当前依赖深度;
    
    
    加载器.当前依赖深度 = 加载器.当前依赖深度 + 1;
    
    
    模块元数据* 结果 = 加载模块核心(加载器, 模块名);
    
    
    加载器.当前模块 = 保存当前模块;
    加载器.当前依赖深度 = 保存依赖深度;
    
    返回 结果;
}










函数 处理导入语句(模块加载器* 加载器, 导入声明* 导入节点, 模块元数据* 当前模块) -> 布尔 {
    如果 (加载器 == 无 || 导入节点 == 无 || 当前模块 == 无) {
        返回 假;
    }
    
    
    字符串 模块名 = 导入节点.模块名;
    如果 (模块名 == 无) {
        返回 假;
    }
    
    
    模块元数据* 依赖模块 = 加载模块(加载器, 模块名);
    如果 (依赖模块 == 无) {
        返回 假;
    }
    
    
    如果 (当前模块.依赖数量 >= 当前模块.依赖容量) {
        
        整数 新容量 = 当前模块.依赖容量 * 2;
        如果 (新容量 == 0) {
            新容量 = 4;
        }
        
        模块依赖* 新数组 = (模块依赖*)重新分配内存(当前模块.依赖列表, 新容量 * 模块依赖大小());
        如果 (新数组 == 无) {
            返回 假;
        }
        
        当前模块.依赖列表 = 新数组;
        当前模块.依赖容量 = 新容量;
    }
    
    
    模块依赖* 依赖 = &当前模块.依赖列表[当前模块.依赖数量];
    依赖.模块标识 = 依赖模块.标识符;
    依赖.是必需 = 真;
    依赖.是循环 = 假;
    当前模块.依赖数量 = 当前模块.依赖数量 + 1;
    
    返回 真;
}





函数 处理所有导入语句(模块加载器* 加载器, 模块元数据* 当前模块) -> 布尔 {
    如果 (加载器 == 无 || 当前模块 == 无 || 当前模块.程序节点 == 无) {
        返回 假;
    }
    
    程序节点* 程序 = 当前模块.程序节点;
    
    
    声明节点列表* 声明列表 = 程序.声明列表;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.类型 == 节点类型.导入声明) {
            导入声明* 导入节点 = 声明.作为导入语句;
            如果 (!处理导入语句(加载器, 导入节点, 当前模块)) {
                
            }
        }
        声明列表 = 声明列表.下一个;
    }
    
    返回 真;
}








函数 收集导出符号(模块元数据* 元数据) -> 布尔 {
    如果 (元数据 == 无 || 元数据.程序节点 == 无) {
        返回 假;
    }
    
    程序节点* 程序 = 元数据.程序节点;
    
    
    整数 符号数量 = 0;
    声明节点列表* 声明列表 = 程序.声明列表;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.是否公开) {
            符号数量 = 符号数量 + 1;
        }
        声明列表 = 声明列表.下一个;
    }
    
    如果 (符号数量 == 0) {
        返回 真;
    }
    
    
    元数据.导出符号表 = (导出符号*)分配清零内存(符号数量, 导出符号大小());
    如果 (元数据.导出符号表 == 无) {
        返回 假;
    }
    元数据.导出符号容量 = 符号数量;
    
    
    声明列表 = 程序.声明列表;
    整数 当前索引 = 0;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.是否公开) {
            导出符号* 符号 = &元数据.导出符号表[当前索引];
            
            
            如果 (声明.类型 == 节点类型.函数声明) {
                函数声明* 函数声明 = 声明.作为函数声明;
                如果 (函数声明 != 无) {
                    符号.名称 = 函数声明.名称;
                    符号.名称长度 = 获取字符串长度(函数声明.名称);
                    符号.是函数 = 真;
                    符号.是常量 = 假;
                }
            } 否则 如果 (声明.类型 == 节点类型.变量声明) {
                变量声明* 变量声明 = 声明.作为变量声明;
                如果 (变量声明 != 无) {
                    符号.名称 = 变量声明.名称;
                    符号.名称长度 = 获取字符串长度(变量声明.名称);
                    符号.是函数 = 假;
                    符号.是常量 = 变量声明.是常量;
                }
            } 否则 如果 (声明.类型 == 节点类型.变量声明) {
                变量声明* 常量声明 = 声明.作为常量声明;
                如果 (常量声明 != 无) {
                    符号.名称 = 常量声明.名称;
                    符号.名称长度 = 获取字符串长度(常量声明.名称);
                    符号.是函数 = 假;
                    符号.是常量 = 真;
                }
            }
            
            当前索引 = 当前索引 + 1;
            元数据.导出符号数量 = 当前索引;
        }
        声明列表 = 声明列表.下一个;
    }
    
    返回 真;
}







函数 模块加载器大小() -> 整数 {
    返回 1024;  
}



函数 模块标识符大小() -> 整数 {
    返回 32;  
}



函数 模块元数据大小() -> 整数 {
    返回 128;  
}



函数 模块缓存条目大小() -> 整数 {
    返回 24;  
}



函数 模块依赖大小() -> 整数 {
    返回 16;  
}



函数 导出符号大小() -> 整数 {
    返回 32;  
}



函数 字符串指针大小() -> 整数 {
    返回 8;  
}




函数 获取文件大小(空类型* 文件) -> 整数 {
    如果 (文件 == 无) {
        返回 0;
    }
    
    
    文件定位(文件, 0, 2);  
    
    
    整数 大小 = 获取文件位置(文件);
    
    
    文件定位(文件, 0, 0);  
    
    返回 大小;
}






函数 报告错误(诊断集合* 诊断集合指针, 诊断错误码 错误码, 源位置* 位置, 字符串 消息) -> 空类型 {
    
    如果 (消息 != 无) {
        打印格式("错误: %s\n", 消息);
    }
}
*)cn_rt_array_get_element(r38, r40, 8);
  cn_var_依赖_4 = r42;
  r43 = cn_var_依赖模块_1;
  r44 = r43->标识符;
  r45 = cn_var_当前模块;
  r46 = r45->依赖数量;
  r47 = r46 + 1;
  return 1;

  if_then_185:
  cn_var_新容量_2 = 4;
  goto if_merge_186;

  if_merge_186:
  struct 模块依赖* cn_var_新数组_3;
  r27 = cn_var_当前模块;
  r28 = r27->依赖列表;
  r29 = cn_var_新容量_2;
  r30 = 模块依赖大小();
  r31 = r29 * r30;
  r32 = 重新分配内存(r28, r31);
  cn_var_新数组_3 = (struct 模块依赖*)0;
  r33 = cn_var_新数组_3;
  r34 = r33 == 0;
  if (r34) goto if_then_187; else goto if_merge_188;

  if_then_187:
  return 0;
  goto if_merge_188;

  if_merge_188:
  r35 = cn_var_新数组_3;
  r36 = cn_var_新容量_2;
  goto if_merge_184;
  return 0;
}

_Bool 处理所有导入语句(struct 模块加载器* cn_var_加载器, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r17;
  struct 模块加载器* r2;
  struct 模块元数据* r4;
  struct 模块元数据* r6;
  struct 程序节点* r7;
  struct 模块元数据* r9;
  struct 程序节点* r10;
  struct 程序节点* r11;
  struct 声明节点列表* r12;
  struct 声明节点列表* r13;
  struct 声明节点列表* r15;
  struct 声明节点* r16;
  struct 声明节点* r18;
  struct 声明节点* r20;
  struct 声明节点* r23;
  struct 导入声明* r24;
  struct 模块加载器* r25;
  struct 导入声明* r26;
  struct 模块元数据* r27;
  struct 声明节点列表* r30;
  struct 声明节点列表* r31;
  _Bool r3;
  _Bool r5;
  _Bool r8;
  _Bool r14;
  _Bool r19;
  _Bool r22;
  _Bool r28;
  _Bool r29;
  enum 节点类型 r21;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_194; else goto logic_rhs_193;

  if_then_189:
  return 0;
  goto if_merge_190;

  if_merge_190:
  struct 程序节点* cn_var_程序_0;
  r9 = cn_var_当前模块;
  r10 = r9->程序节点;
  cn_var_程序_0 = r10;
  struct 声明节点列表* cn_var_声明列表_1;
  r11 = cn_var_程序_0;
  r12 = r11->声明列表;
  cn_var_声明列表_1 = r12;
  goto while_cond_195;

  logic_rhs_191:
  r6 = cn_var_当前模块;
  r7 = r6->程序节点;
  r8 = r7 == 0;
  goto logic_merge_192;

  logic_merge_192:
  if (r8) goto if_then_189; else goto if_merge_190;

  logic_rhs_193:
  r4 = cn_var_当前模块;
  r5 = r4 == 0;
  goto logic_merge_194;

  logic_merge_194:
  if (r5) goto logic_merge_192; else goto logic_rhs_191;

  while_cond_195:
  r13 = cn_var_声明列表_1;
  r14 = r13 != 0;
  if (r14) goto while_body_196; else goto while_exit_197;

  while_body_196:
  struct 声明节点* cn_var_声明_2;
  r15 = cn_var_声明列表_1;
  r16 = r15->节点;
  cn_var_声明_2 = r16;
  r18 = cn_var_声明_2;
  r19 = r18 != 0;
  if (r19) goto logic_rhs_200; else goto logic_merge_201;

  while_exit_197:
  return 1;

  if_then_198:
  struct 导入声明* cn_var_导入节点_3;
  r23 = cn_var_声明_2;
  r24 = r23->作为导入语句;
  cn_var_导入节点_3 = r24;
  r25 = cn_var_加载器;
  r26 = cn_var_导入节点_3;
  r27 = cn_var_当前模块;
  r28 = 处理导入语句(r25, r26, r27);
  r29 = !r28;
  if (r29) goto if_then_202; else goto if_merge_203;

  if_merge_199:
  r30 = cn_var_声明列表_1;
  r31 = r30->下一个;
  cn_var_声明列表_1 = r31;
  goto while_cond_195;

  logic_rhs_200:
  r20 = cn_var_声明_2;
  r21 = r20->类型;
  r22 = r21 == 节点类型_导入声明;
  goto logic_merge_201;

  logic_merge_201:
  if (r22) goto if_then_198; else goto if_merge_199;

  if_then_202:
  goto if_merge_203;

  if_merge_203:
  goto if_merge_199;
  return 0;
}

_Bool 收集导出符号(struct 模块元数据* cn_var_元数据) {
  long long r0, r14, r19, r20, r23, r25, r26, r31, r38, r45, r56, r58, r59, r68, r70, r71, r73, r85, r86, r87, r88;
  char* r82;
  char* r84;
  struct 模块元数据* r1;
  struct 模块元数据* r3;
  struct 程序节点* r4;
  struct 模块元数据* r6;
  struct 程序节点* r7;
  struct 程序节点* r8;
  struct 声明节点列表* r9;
  struct 声明节点列表* r10;
  struct 声明节点列表* r12;
  struct 声明节点* r13;
  struct 声明节点* r15;
  struct 声明节点* r17;
  struct 声明节点列表* r21;
  struct 声明节点列表* r22;
  void* r27;
  struct 模块元数据* r28;
  struct 导出符号* r29;
  struct 程序节点* r32;
  struct 声明节点列表* r33;
  struct 声明节点列表* r34;
  struct 声明节点列表* r36;
  struct 声明节点* r37;
  struct 声明节点* r39;
  struct 声明节点* r41;
  struct 模块元数据* r43;
  struct 导出符号* r44;
  struct 导出符号* r46;
  struct 导出符号* r47;
  struct 声明节点* r48;
  struct 声明节点* r51;
  struct 函数声明* r52;
  struct 函数声明* r53;
  struct 函数声明* r55;
  struct 函数声明* r57;
  struct 声明节点* r60;
  struct 声明节点* r63;
  struct 变量声明* r64;
  struct 变量声明* r65;
  struct 变量声明* r67;
  struct 变量声明* r69;
  struct 变量声明* r72;
  struct 声明节点* r74;
  struct 声明节点* r77;
  struct 变量声明* r78;
  struct 变量声明* r79;
  struct 变量声明* r81;
  struct 变量声明* r83;
  struct 声明节点列表* r89;
  struct 声明节点列表* r90;
  _Bool r2;
  _Bool r5;
  _Bool r11;
  _Bool r16;
  _Bool r18;
  _Bool r24;
  _Bool r30;
  _Bool r35;
  _Bool r40;
  _Bool r42;
  _Bool r50;
  _Bool r54;
  _Bool r62;
  _Bool r66;
  _Bool r76;
  _Bool r80;
  enum 节点类型 r49;
  enum 节点类型 r61;
  enum 节点类型 r75;

  entry:
  r1 = cn_var_元数据;
  r2 = r1 == 0;
  if (r2) goto logic_merge_207; else goto logic_rhs_206;

  if_then_204:
  return 0;
  goto if_merge_205;

  if_merge_205:
  struct 程序节点* cn_var_程序_0;
  r6 = cn_var_元数据;
  r7 = r6->程序节点;
  cn_var_程序_0 = r7;
  long long cn_var_符号数量_1;
  cn_var_符号数量_1 = 0;
  struct 声明节点列表* cn_var_声明列表_2;
  r8 = cn_var_程序_0;
  r9 = r8->声明列表;
  cn_var_声明列表_2 = r9;
  goto while_cond_208;

  logic_rhs_206:
  r3 = cn_var_元数据;
  r4 = r3->程序节点;
  r5 = r4 == 0;
  goto logic_merge_207;

  logic_merge_207:
  if (r5) goto if_then_204; else goto if_merge_205;

  while_cond_208:
  r10 = cn_var_声明列表_2;
  r11 = r10 != 0;
  if (r11) goto while_body_209; else goto while_exit_210;

  while_body_209:
  struct 声明节点* cn_var_声明_3;
  r12 = cn_var_声明列表_2;
  r13 = r12->节点;
  cn_var_声明_3 = r13;
  r15 = cn_var_声明_3;
  r16 = r15 != 0;
  if (r16) goto logic_rhs_213; else goto logic_merge_214;

  while_exit_210:
  r23 = cn_var_符号数量_1;
  r24 = r23 == 0;
  if (r24) goto if_then_215; else goto if_merge_216;

  if_then_211:
  r19 = cn_var_符号数量_1;
  r20 = r19 + 1;
  cn_var_符号数量_1 = r20;
  goto if_merge_212;

  if_merge_212:
  r21 = cn_var_声明列表_2;
  r22 = r21->下一个;
  cn_var_声明列表_2 = r22;
  goto while_cond_208;

  logic_rhs_213:
  r17 = cn_var_声明_3;
  r18 = r17->是否公开;
  goto logic_merge_214;

  logic_merge_214:
  if (r18) goto if_then_211; else goto if_merge_212;

  if_then_215:
  return 1;
  goto if_merge_216;

  if_merge_216:
  r25 = cn_var_符号数量_1;
  r26 = 导出符号大小();
  r27 = 分配清零内存(r25, r26);
  r28 = cn_var_元数据;
  r29 = r28->导出符号表;
  r30 = r29 == 0;
  if (r30) goto if_then_217; else goto if_merge_218;

  if_then_217:
  return 0;
  goto if_merge_218;

  if_merge_218:
  r31 = cn_var_符号数量_1;
  r32 = cn_var_程序_0;
  r33 = r32->声明列表;
  cn_var_声明列表_2 = r33;
  long long cn_var_当前索引_4;
  cn_var_当前索引_4 = 0;
  goto while_cond_219;

  while_cond_219:
  r34 = cn_var_声明列表_2;
  r35 = r34 != 0;
  if (r35) goto while_body_220; else goto while_exit_221;

  while_body_220:
  struct 声明节点* cn_var_声明_5;
  r36 = cn_var_声明列表_2;
  r37 = r36->节点;
  cn_var_声明_5 = r37;
  r39 = cn_var_声明_5;
  r40 = r39 != 0;
  if (r40) goto logic_rhs_224; else goto logic_merge_225;

  while_exit_221:
  return 1;

  if_then_222:
  struct 导出符号* cn_var_符号_6;
  r43 = cn_var_元数据;
  r44 = r43->导出符号表;
  r45 = cn_var_当前索引_4;
  r46 = (struct 导出符号 {
    字符串 名称;             
    整数 名称长度;           
    布尔 是函数;             
    布尔 是常量;             
    空类型* 符号信息;        
}






结构体 模块依赖 {
    模块标识符* 模块标识;    
    布尔 是必需;             
    布尔 是循环;             
}






结构体 模块元数据 {
    模块标识符* 标识符;      
    模块类型 类型;           
    模块加载状态 状态;       
    
    
    字符串 文件路径;         
    整数 文件路径长度;       
    
    
    导出符号* 导出符号表;    
    整数 导出符号数量;       
    整数 导出符号容量;       
    
    
    模块依赖* 依赖列表;      
    整数 依赖数量;           
    整数 依赖容量;           
    
    
    程序节点* 程序节点;      
    
    
    符号表管理器* 符号表;    
}






结构体 模块缓存条目 {
    字符串 模块名;           
    模块元数据* 元数据;      
    模块缓存条目* 下一个;    
}







结构体 模块加载器 {
    
    字符串[32] 搜索路径列表;  
    整数 搜索路径数量;                      
    
    
    模块缓存条目*[256] 缓存列表;  
    整数 缓存数量;                            
    
    
    诊断集合* 诊断集合;                      
    
    
    模块元数据* 当前模块;       
    整数 当前依赖深度;          
    
    
    整数 加载成功数;            
    整数 加载失败数;            
}








函数 创建模块加载器(诊断集合* 诊断集合指针) -> 模块加载器* {
    模块加载器* 加载器 = (模块加载器*)分配清零内存(1, 模块加载器大小());
    如果 (加载器 == 无) {
        返回 无;
    }
    
    加载器.诊断集合 = 诊断集合指针;
    加载器.搜索路径数量 = 0;
    加载器.缓存数量 = 0;
    加载器.当前模块 = 无;
    加载器.当前依赖深度 = 0;
    加载器.加载成功数 = 0;
    加载器.加载失败数 = 0;
    
    返回 加载器;
}



函数 销毁模块加载器(模块加载器* 加载器) -> 空类型 {
    如果 (加载器 == 无) {
        返回;
    }
    
    
    循环 (整数 i = 0; i < 加载器.搜索路径数量; i = i + 1) {
        如果 (加载器.搜索路径列表[i] != 无) {
            释放内存(加载器.搜索路径列表[i]);
        }
    }
    
    
    循环 (整数 i = 0; i < 最大模块缓存数量; i = i + 1) {
        模块缓存条目* 条目 = 加载器.缓存列表[i];
        当 (条目 != 无) {
            模块缓存条目* 下一个 = 条目.下一个;
            释放内存(条目.模块名);
            
            释放内存(条目);
            条目 = 下一个;
        }
    }
    
    释放内存(加载器);
}









函数 添加搜索路径(模块加载器* 加载器, 字符串 路径) -> 布尔 {
    如果 (加载器 == 无 || 路径 == 无) {
        返回 假;
    }
    
    如果 (加载器.搜索路径数量 >= 最大搜索路径数量) {
        返回 假;
    }
    
    
    整数 路径长度 = 获取字符串长度(路径);
    字符串 路径副本 = (字符串)分配内存(路径长度 + 1);
    如果 (路径副本 == 无) {
        返回 假;
    }
    复制字符串(路径副本, 路径);
    
    加载器.搜索路径列表[加载器.搜索路径数量] = 路径副本;
    加载器.搜索路径数量 = 加载器.搜索路径数量 + 1;
    
    返回 真;
}




函数 设置默认搜索路径(模块加载器* 加载器, 字符串 源文件目录) -> 空类型 {
    如果 (加载器 == 无) {
        返回;
    }
    
    
    添加搜索路径(加载器, 源文件目录);
    
    
    添加搜索路径(加载器, ".");
    
    
    添加搜索路径(加载器, "标准库");
    添加搜索路径(加载器, "stdlib");
}








函数 解析模块标识符(字符串 模块名) -> 模块标识符* {
    如果 (模块名 == 无) {
        返回 无;
    }
    
    整数 名称长度 = 获取字符串长度(模块名);
    如果 (名称长度 == 0 || 名称长度 >= 最大模块路径长度) {
        返回 无;
    }
    
    模块标识符* 标识符 = (模块标识符*)分配清零内存(1, 模块标识符大小());
    如果 (标识符 == 无) {
        返回 无;
    }
    
    
    标识符.完全限定名 = (字符串)分配内存(名称长度 + 1);
    如果 (标识符.完全限定名 == 无) {
        释放内存(标识符);
        返回 无;
    }
    复制字符串(标识符.完全限定名, 模块名);
    标识符.完全限定名长度 = 名称长度;
    
    
    整数 段数量 = 1;
    循环 (整数 i = 0; i < 名称长度; i = i + 1) {
        如果 (模块名[i] == '.') {
            段数量 = 段数量 + 1;
        }
    }
    
    
    标识符.路径段列表 = (字符串*)分配清零内存(段数量, 字符串指针大小());
    如果 (标识符.路径段列表 == 无) {
        释放内存(标识符.完全限定名);
        释放内存(标识符);
        返回 无;
    }
    标识符.路径段数量 = 段数量;
    
    
    字符 缓冲区[1024];
    整数 缓冲区位置 = 0;
    整数 当前段索引 = 0;
    
    循环 (整数 i = 0; i <= 名称长度; i = i + 1) {
        如果 (模块名[i] == '.' || 模块名[i] == '\0') {
            缓冲区[缓冲区位置] = '\0';
            
            
            整数 段长度 = 获取字符串长度(缓冲区);
            字符串 段副本 = (字符串)分配内存(段长度 + 1);
            如果 (段副本 == 无) {
                
                循环 (整数 j = 0; j < 当前段索引; j = j + 1) {
                    释放内存(标识符.路径段列表[j]);
                }
                释放内存(标识符.路径段列表);
                释放内存(标识符.完全限定名);
                释放内存(标识符);
                返回 无;
            }
            复制字符串(段副本, 缓冲区);
            标识符.路径段列表[当前段索引] = 段副本;
            当前段索引 = 当前段索引 + 1;
            缓冲区位置 = 0;
        } 否则 {
            缓冲区[缓冲区位置] = 模块名[i];
            缓冲区位置 = 缓冲区位置 + 1;
        }
    }
    
    返回 标识符;
}



函数 销毁模块标识符(模块标识符* 标识符) -> 空类型 {
    如果 (标识符 == 无) {
        返回;
    }
    
    如果 (标识符.完全限定名 != 无) {
        释放内存(标识符.完全限定名);
    }
    
    如果 (标识符.路径段列表 != 无) {
        循环 (整数 i = 0; i < 标识符.路径段数量; i = i + 1) {
            如果 (标识符.路径段列表[i] != 无) {
                释放内存(标识符.路径段列表[i]);
            }
        }
        释放内存(标识符.路径段列表);
    }
    
    释放内存(标识符);
}











函数 解析模块文件路径(模块加载器* 加载器, 模块标识符* 标识符, 字符串 输出路径, 整数 输出路径大小) -> 布尔 {
    如果 (加载器 == 无 || 标识符 == 无 || 输出路径 == 无) {
        返回 假;
    }
    
    
    循环 (整数 i = 0; i < 加载器.搜索路径数量; i = i + 1) {
        字符串 搜索路径 = 加载器.搜索路径列表[i];
        
        
        字符 路径缓冲区[1024];
        整数 路径位置 = 0;
        
        
        整数 搜索路径长度 = 获取字符串长度(搜索路径);
        复制内存(路径缓冲区, 搜索路径, 搜索路径长度);
        路径位置 = 搜索路径长度;
        
        
        循环 (整数 j = 0; j < 标识符.路径段数量; j = j + 1) {
            路径缓冲区[路径位置] = '/';
            路径位置 = 路径位置 + 1;
            
            字符串 段 = 标识符.路径段列表[j];
            整数 段长度 = 获取字符串长度(段);
            复制内存(路径缓冲区 + 路径位置, 段, 段长度);
            路径位置 = 路径位置 + 段长度;
        }
        
        
        复制字符串(路径缓冲区 + 路径位置, ".cn");
        路径位置 = 路径位置 + 3;
        路径缓冲区[路径位置] = '\0';
        
        
        空类型* 文件 = 打开文件(路径缓冲区, "r");
        如果 (文件 != 无) {
            关闭文件(文件);
            如果 (路径位置 < 输出路径大小) {
                复制字符串(输出路径, 路径缓冲区);
                返回 真;
            }
        }
    }
    
    返回 假;
}








函数 计算哈希值(字符串 字符串指针) -> 整数 {
    整数 哈希 = 0;
    如果 (字符串指针 == 无) {
        返回 哈希;
    }
    
    循环 (整数 i = 0; 字符串指针[i] != '\0'; i = i + 1) {
        哈希 = 哈希 * 31 + 字符串指针[i];
    }
    
    如果 (哈希 < 0) {
        哈希 = -哈希;
    }
    
    返回 哈希;
}





函数 从缓存获取模块(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    整数 哈希 = 计算哈希值(模块名);
    整数 索引 = 哈希 % 最大模块缓存数量;
    
    模块缓存条目* 条目 = 加载器.缓存列表[索引];
    当 (条目 != 无) {
        如果 (比较字符串(条目.模块名, 模块名) == 0) {
            返回 条目.元数据;
        }
        条目 = 条目.下一个;
    }
    
    返回 无;
}






函数 添加模块到缓存(模块加载器* 加载器, 字符串 模块名, 模块元数据* 元数据) -> 布尔 {
    如果 (加载器 == 无 || 模块名 == 无 || 元数据 == 无) {
        返回 假;
    }
    
    整数 哈希 = 计算哈希值(模块名);
    整数 索引 = 哈希 % 最大模块缓存数量;
    
    
    模块缓存条目* 新条目 = (模块缓存条目*)分配清零内存(1, 模块缓存条目大小());
    如果 (新条目 == 无) {
        返回 假;
    }
    
    
    整数 名称长度 = 获取字符串长度(模块名);
    新条目.模块名 = (字符串)分配内存(名称长度 + 1);
    如果 (新条目.模块名 == 无) {
        释放内存(新条目);
        返回 假;
    }
    复制字符串(新条目.模块名, 模块名);
    
    新条目.元数据 = 元数据;
    新条目.下一个 = 加载器.缓存列表[索引];
    加载器.缓存列表[索引] = 新条目;
    加载器.缓存数量 = 加载器.缓存数量 + 1;
    
    返回 真;
}










函数 读取文件内容(字符串 文件路径, 字符串* 输出缓冲区, 整数* 输出大小) -> 布尔 {
    如果 (文件路径 == 无 || 输出缓冲区 == 无 || 输出大小 == 无) {
        返回 假;
    }
    
    空类型* 文件 = 打开文件(文件路径, "rb");
    如果 (文件 == 无) {
        返回 假;
    }
    
    
    整数 文件大小 = 获取文件大小(文件);
    如果 (文件大小 <= 0) {
        关闭文件(文件);
        返回 假;
    }
    
    
    字符串 缓冲区 = (字符串)分配内存(文件大小 + 1);
    如果 (缓冲区 == 无) {
        关闭文件(文件);
        返回 假;
    }
    
    
    整数 读取字节数 = 读取文件(文件, 缓冲区, 文件大小);
    关闭文件(文件);
    
    如果 (读取字节数 != 文件大小) {
        释放内存(缓冲区);
        返回 假;
    }
    
    缓冲区[文件大小] = '\0';
    *输出缓冲区 = 缓冲区;
    *输出大小 = 文件大小;
    
    返回 真;
}








函数 创建模块元数据(模块标识符* 标识符) -> 模块元数据* {
    如果 (标识符 == 无) {
        返回 无;
    }
    
    模块元数据* 元数据 = (模块元数据*)分配清零内存(1, 模块元数据大小());
    如果 (元数据 == 无) {
        返回 无;
    }
    
    元数据.标识符 = 标识符;
    元数据.类型 = 文件模块;
    元数据.状态 = 未加载;
    元数据.文件路径 = 无;
    元数据.文件路径长度 = 0;
    元数据.导出符号表 = 无;
    元数据.导出符号数量 = 0;
    元数据.导出符号容量 = 0;
    元数据.依赖列表 = 无;
    元数据.依赖数量 = 0;
    元数据.依赖容量 = 0;
    元数据.程序节点 = 无;
    元数据.符号表 = 无;
    
    返回 元数据;
}



函数 销毁模块元数据(模块元数据* 元数据) -> 空类型 {
    如果 (元数据 == 无) {
        返回;
    }
    
    如果 (元数据.标识符 != 无) {
        销毁模块标识符(元数据.标识符);
    }
    
    如果 (元数据.文件路径 != 无) {
        释放内存(元数据.文件路径);
    }
    
    如果 (元数据.导出符号表 != 无) {
        释放内存(元数据.导出符号表);
    }
    
    如果 (元数据.依赖列表 != 无) {
        释放内存(元数据.依赖列表);
    }
    
    
    
    释放内存(元数据);
}





函数 加载模块核心(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    
    模块元数据* 缓存模块 = 从缓存获取模块(加载器, 模块名);
    如果 (缓存模块 != 无) {
        返回 缓存模块;
    }
    
    
    如果 (加载器.当前依赖深度 >= 最大依赖深度) {
        报告错误(加载器.诊断集合, 未知错误, 无, "模块依赖深度超过限制");
        返回 无;
    }
    
    
    模块标识符* 标识符 = 解析模块标识符(模块名);
    如果 (标识符 == 无) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法解析模块标识符");
        返回 无;
    }
    
    
    模块元数据* 元数据 = 创建模块元数据(标识符);
    如果 (元数据 == 无) {
        销毁模块标识符(标识符);
        返回 无;
    }
    
    
    元数据.状态 = 加载中;
    
    
    字符 文件路径缓冲区[1024];
    如果 (!解析模块文件路径(加载器, 标识符, 文件路径缓冲区, 1024)) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法找到模块文件");
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    整数 路径长度 = 获取字符串长度(文件路径缓冲区);
    元数据.文件路径 = (字符串)分配内存(路径长度 + 1);
    如果 (元数据.文件路径 == 无) {
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    复制字符串(元数据.文件路径, 文件路径缓冲区);
    元数据.文件路径长度 = 路径长度;
    
    
    字符串 源码内容 = 无;
    整数 源码大小 = 0;
    如果 (!读取文件内容(文件路径缓冲区, &源码内容, &源码大小)) {
        报告错误(加载器.诊断集合, 未知错误, 无, "无法读取模块文件");
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    扫描器* 扫描器实例 = 创建扫描器(源码内容, 源码大小);
    如果 (扫描器实例 == 无) {
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    解析器* 解析器实例 = 创建解析器(扫描器实例, 加载器.诊断集合);
    如果 (解析器实例 == 无) {
        销毁扫描器(扫描器实例);
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    
    程序节点* 程序 = 解析程序(解析器实例);
    如果 (程序 == 无) {
        销毁解析器(解析器实例);
        销毁扫描器(扫描器实例);
        释放内存(源码内容);
        元数据.状态 = 加载失败;
        销毁模块元数据(元数据);
        加载器.加载失败数 = 加载器.加载失败数 + 1;
        返回 无;
    }
    
    元数据.程序节点 = 程序;
    
    
    销毁解析器(解析器实例);
    销毁扫描器(扫描器实例);
    释放内存(源码内容);
    
    
    元数据.状态 = 已加载;
    
    
    添加模块到缓存(加载器, 模块名, 元数据);
    加载器.加载成功数 = 加载器.加载成功数 + 1;
    
    返回 元数据;
}





函数 加载模块(模块加载器* 加载器, 字符串 模块名) -> 模块元数据* {
    如果 (加载器 == 无 || 模块名 == 无) {
        返回 无;
    }
    
    
    模块元数据* 保存当前模块 = 加载器.当前模块;
    整数 保存依赖深度 = 加载器.当前依赖深度;
    
    
    加载器.当前依赖深度 = 加载器.当前依赖深度 + 1;
    
    
    模块元数据* 结果 = 加载模块核心(加载器, 模块名);
    
    
    加载器.当前模块 = 保存当前模块;
    加载器.当前依赖深度 = 保存依赖深度;
    
    返回 结果;
}










函数 处理导入语句(模块加载器* 加载器, 导入声明* 导入节点, 模块元数据* 当前模块) -> 布尔 {
    如果 (加载器 == 无 || 导入节点 == 无 || 当前模块 == 无) {
        返回 假;
    }
    
    
    字符串 模块名 = 导入节点.模块名;
    如果 (模块名 == 无) {
        返回 假;
    }
    
    
    模块元数据* 依赖模块 = 加载模块(加载器, 模块名);
    如果 (依赖模块 == 无) {
        返回 假;
    }
    
    
    如果 (当前模块.依赖数量 >= 当前模块.依赖容量) {
        
        整数 新容量 = 当前模块.依赖容量 * 2;
        如果 (新容量 == 0) {
            新容量 = 4;
        }
        
        模块依赖* 新数组 = (模块依赖*)重新分配内存(当前模块.依赖列表, 新容量 * 模块依赖大小());
        如果 (新数组 == 无) {
            返回 假;
        }
        
        当前模块.依赖列表 = 新数组;
        当前模块.依赖容量 = 新容量;
    }
    
    
    模块依赖* 依赖 = &当前模块.依赖列表[当前模块.依赖数量];
    依赖.模块标识 = 依赖模块.标识符;
    依赖.是必需 = 真;
    依赖.是循环 = 假;
    当前模块.依赖数量 = 当前模块.依赖数量 + 1;
    
    返回 真;
}





函数 处理所有导入语句(模块加载器* 加载器, 模块元数据* 当前模块) -> 布尔 {
    如果 (加载器 == 无 || 当前模块 == 无 || 当前模块.程序节点 == 无) {
        返回 假;
    }
    
    程序节点* 程序 = 当前模块.程序节点;
    
    
    声明节点列表* 声明列表 = 程序.声明列表;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.类型 == 节点类型.导入声明) {
            导入声明* 导入节点 = 声明.作为导入语句;
            如果 (!处理导入语句(加载器, 导入节点, 当前模块)) {
                
            }
        }
        声明列表 = 声明列表.下一个;
    }
    
    返回 真;
}








函数 收集导出符号(模块元数据* 元数据) -> 布尔 {
    如果 (元数据 == 无 || 元数据.程序节点 == 无) {
        返回 假;
    }
    
    程序节点* 程序 = 元数据.程序节点;
    
    
    整数 符号数量 = 0;
    声明节点列表* 声明列表 = 程序.声明列表;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.是否公开) {
            符号数量 = 符号数量 + 1;
        }
        声明列表 = 声明列表.下一个;
    }
    
    如果 (符号数量 == 0) {
        返回 真;
    }
    
    
    元数据.导出符号表 = (导出符号*)分配清零内存(符号数量, 导出符号大小());
    如果 (元数据.导出符号表 == 无) {
        返回 假;
    }
    元数据.导出符号容量 = 符号数量;
    
    
    声明列表 = 程序.声明列表;
    整数 当前索引 = 0;
    当 (声明列表 != 无) {
        声明节点* 声明 = 声明列表.节点;
        如果 (声明 != 无 && 声明.是否公开) {
            导出符号* 符号 = &元数据.导出符号表[当前索引];
            
            
            如果 (声明.类型 == 节点类型.函数声明) {
                函数声明* 函数声明 = 声明.作为函数声明;
                如果 (函数声明 != 无) {
                    符号.名称 = 函数声明.名称;
                    符号.名称长度 = 获取字符串长度(函数声明.名称);
                    符号.是函数 = 真;
                    符号.是常量 = 假;
                }
            } 否则 如果 (声明.类型 == 节点类型.变量声明) {
                变量声明* 变量声明 = 声明.作为变量声明;
                如果 (变量声明 != 无) {
                    符号.名称 = 变量声明.名称;
                    符号.名称长度 = 获取字符串长度(变量声明.名称);
                    符号.是函数 = 假;
                    符号.是常量 = 变量声明.是常量;
                }
            } 否则 如果 (声明.类型 == 节点类型.变量声明) {
                变量声明* 常量声明 = 声明.作为常量声明;
                如果 (常量声明 != 无) {
                    符号.名称 = 常量声明.名称;
                    符号.名称长度 = 获取字符串长度(常量声明.名称);
                    符号.是函数 = 假;
                    符号.是常量 = 真;
                }
            }
            
            当前索引 = 当前索引 + 1;
            元数据.导出符号数量 = 当前索引;
        }
        声明列表 = 声明列表.下一个;
    }
    
    返回 真;
}







函数 模块加载器大小() -> 整数 {
    返回 1024;  
}



函数 模块标识符大小() -> 整数 {
    返回 32;  
}



函数 模块元数据大小() -> 整数 {
    返回 128;  
}



函数 模块缓存条目大小() -> 整数 {
    返回 24;  
}



函数 模块依赖大小() -> 整数 {
    返回 16;  
}



函数 导出符号大小() -> 整数 {
    返回 32;  
}



函数 字符串指针大小() -> 整数 {
    返回 8;  
}




函数 获取文件大小(空类型* 文件) -> 整数 {
    如果 (文件 == 无) {
        返回 0;
    }
    
    
    文件定位(文件, 0, 2);  
    
    
    整数 大小 = 获取文件位置(文件);
    
    
    文件定位(文件, 0, 0);  
    
    返回 大小;
}






函数 报告错误(诊断集合* 诊断集合指针, 诊断错误码 错误码, 源位置* 位置, 字符串 消息) -> 空类型 {
    
    如果 (消息 != 无) {
        打印格式("错误: %s\n", 消息);
    }
}
*)cn_rt_array_get_element(r44, r45, 8);
  cn_var_符号_6 = r47;
  r48 = cn_var_声明_5;
  r49 = r48->类型;
  r50 = r49 == 节点类型_函数声明;
  if (r50) goto if_then_226; else goto if_else_227;

  if_merge_223:
  r89 = cn_var_声明列表_2;
  r90 = r89->下一个;
  cn_var_声明列表_2 = r90;
  goto while_cond_219;

  logic_rhs_224:
  r41 = cn_var_声明_5;
  r42 = r41->是否公开;
  goto logic_merge_225;

  logic_merge_225:
  if (r42) goto if_then_222; else goto if_merge_223;

  if_then_226:
  struct 函数声明* cn_var_函数声明_7;
  r51 = cn_var_声明_5;
  r52 = r51->作为函数声明;
  cn_var_函数声明_7 = r52;
  r53 = cn_var_函数声明_7;
  r54 = r53 != 0;
  if (r54) goto if_then_229; else goto if_merge_230;

  if_else_227:
  r60 = cn_var_声明_5;
  r61 = r60->类型;
  r62 = r61 == 节点类型_变量声明;
  if (r62) goto if_then_231; else goto if_else_232;

  if_merge_228:
  r86 = cn_var_当前索引_4;
  r87 = r86 + 1;
  cn_var_当前索引_4 = r87;
  r88 = cn_var_当前索引_4;
  goto if_merge_223;

  if_then_229:
  r55 = cn_var_函数声明_7;
  r56 = r55->名称;
  r57 = cn_var_函数声明_7;
  r58 = r57->名称;
  r59 = 获取字符串长度(r58);
  goto if_merge_230;

  if_merge_230:
  goto if_merge_228;

  if_then_231:
  struct 变量声明* cn_var_变量声明_8;
  r63 = cn_var_声明_5;
  r64 = r63->作为变量声明;
  cn_var_变量声明_8 = r64;
  r65 = cn_var_变量声明_8;
  r66 = r65 != 0;
  if (r66) goto if_then_234; else goto if_merge_235;

  if_else_232:
  r74 = cn_var_声明_5;
  r75 = r74->类型;
  r76 = r75 == 节点类型_变量声明;
  if (r76) goto if_then_236; else goto if_merge_237;

  if_merge_233:
  goto if_merge_228;

  if_then_234:
  r67 = cn_var_变量声明_8;
  r68 = r67->名称;
  r69 = cn_var_变量声明_8;
  r70 = r69->名称;
  r71 = 获取字符串长度(r70);
  r72 = cn_var_变量声明_8;
  r73 = r72->是常量;
  goto if_merge_235;

  if_merge_235:
  goto if_merge_233;

  if_then_236:
  struct 变量声明* cn_var_常量声明_9;
  r77 = cn_var_声明_5;
  r78 = r77->作为常量声明;
  cn_var_常量声明_9 = r78;
  r79 = cn_var_常量声明_9;
  r80 = r79 != 0;
  if (r80) goto if_then_238; else goto if_merge_239;

  if_merge_237:
  goto if_merge_233;

  if_then_238:
  r81 = cn_var_常量声明_9;
  r82 = r81->名称;
  r83 = cn_var_常量声明_9;
  r84 = r83->名称;
  r85 = 获取字符串长度(r84);
  goto if_merge_239;

  if_merge_239:
  goto if_merge_237;
  return 0;
}

long long 模块加载器大小() {

  entry:
  return 1024;
}

long long 模块标识符大小() {

  entry:
  return 32;
}

long long 模块元数据大小() {

  entry:
  return 128;
}

long long 模块缓存条目大小() {

  entry:
  return 24;
}

long long 模块依赖大小() {

  entry:
  return 16;
}

long long 导出符号大小() {

  entry:
  return 32;
}

long long 字符串指针大小() {

  entry:
  return 8;
}

long long 获取文件大小(void* cn_var_文件) {
  long long r3, r5, r7, r8;
  void* r0;
  void* r2;
  void* r4;
  void* r6;
  _Bool r1;

  entry:
  r0 = cn_var_文件;
  r1 = r0 == 0;
  if (r1) goto if_then_240; else goto if_merge_241;

  if_then_240:
  return 0;
  goto if_merge_241;

  if_merge_241:
  r2 = cn_var_文件;
  r3 = 文件定位(r2, 0, 2);
  long long cn_var_大小_0;
  r4 = cn_var_文件;
  r5 = 获取文件位置(r4);
  cn_var_大小_0 = r5;
  r6 = cn_var_文件;
  r7 = 文件定位(r6, 0, 0);
  r8 = cn_var_大小_0;
  return r8;
}

void 报告错误(struct 诊断集合* cn_var_诊断集合指针, enum 诊断错误码 cn_var_错误码, struct 源位置* cn_var_位置, char* cn_var_消息) {
  long long r3;
  char* r0;
  char* r2;
  _Bool r1;

  entry:
  r0 = cn_var_消息;
  r1 = r0 != 0;
  if (r1) goto if_then_242; else goto if_merge_243;

  if_then_242:
  r2 = cn_var_消息;
  r3 = 打印格式("错误: %s\n", r2);
  goto if_merge_243;

  if_merge_243:
  return;
}

