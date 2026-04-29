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
enum 语法恢复策略 {
    语法恢复策略_同步到关键字 = 4,
    语法恢复策略_同步到大括号 = 3,
    语法恢复策略_同步到分号 = 2,
    语法恢复策略_跳过词元 = 1,
    语法恢复策略_不恢复 = 0
};
enum 同步点类型 {
    同步点类型_同步点_声明关键字 = 7,
    同步点类型_同步点_类型关键字 = 6,
    同步点类型_同步点_关键字 = 5,
    同步点类型_同步点_逗号 = 4,
    同步点类型_同步点_右中括号 = 3,
    同步点类型_同步点_右小括号 = 2,
    同步点类型_同步点_右大括号 = 1,
    同步点类型_同步点_分号 = 0
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
enum 节点类型 {
    节点类型_标识符类型 = 45,
    节点类型_接口类型 = 44,
    节点类型_类类型 = 43,
    节点类型_枚举类型 = 42,
    节点类型_结构体类型 = 41,
    节点类型_函数类型 = 40,
    节点类型_数组类型 = 39,
    节点类型_指针类型 = 38,
    节点类型_基础类型 = 37,
    节点类型_类型转换表达式 = 36,
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 声明语句;
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
struct 声明节点;
struct 声明语句;
struct 声明语句 {
    struct AST节点 节点基类;
    struct 声明节点* 声明;
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
extern struct 源位置 cn_var_空源位置;

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
_Bool 添加搜索路径(struct 模块加载器*, const char*);
void 设置默认搜索路径(struct 模块加载器*, const char*);
struct 模块标识符* 解析模块标识符(const char*);
void 销毁模块标识符(struct 模块标识符*);
_Bool 解析模块文件路径(struct 模块加载器*, struct 模块标识符*, const char*, long long);
long long 计算哈希值(const char*);
struct 模块元数据* 从缓存获取模块(struct 模块加载器*, const char*);
_Bool 添加模块到缓存(struct 模块加载器*, const char*, struct 模块元数据*);
_Bool 读取文件内容(const char*, char**, long long*);
struct 模块元数据* 创建模块元数据(struct 模块标识符*);
void 销毁模块元数据(struct 模块元数据*);
struct 模块元数据* 加载模块核心(struct 模块加载器*, const char*);
struct 模块元数据* 加载模块(struct 模块加载器*, const char*);
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
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);

// Extern Declarations - 跨模块调用的函数（ANSI原型风格）
extern void* 创建扫描器(void*, void*);
extern void* 创建解析器(void*, void*);
extern void* 销毁扫描器(void*);
extern void* 解析程序(void*);
extern void* 销毁解析器(void*);

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
  cn_var_加载器_0 = (struct 模块加载器*)分配清零内存(1, 模块加载器大小());
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
  long long r2, r3, r6, r11, r14, r15, r16, r17, r20, r28, r29;
  char* r8;
  char* r13;
  char* r25;
  char** r5;
  char** r10;
  struct 模块缓存条目** r19;
  struct 模块加载器* r0;
  char** r7;
  char** r12;
  struct 模块缓存条目** r21;
  struct 模块缓存条目* r22;
  struct 模块缓存条目* r24;
  struct 模块缓存条目* r26;
  struct 模块缓存条目* r27;
  struct 模块加载器* r30;
  _Bool r1;
  _Bool r4;
  _Bool r9;
  _Bool r18;
  _Bool r23;

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
  r3 = cn_var_加载器->搜索路径数量;
  r4 = r2 < r3;
  if (r4) goto for_body_5; else goto for_exit_7;

  for_body_5:
  r5 = cn_var_加载器->搜索路径列表;
  r6 = cn_var_i_0;
  r7 = &r5[r6];
  r8 = r7;
  r9 = r8 != 0;
  if (r9) goto if_then_8; else goto if_merge_9;

  for_update_6:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_4;

  for_exit_7:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_10;

  if_then_8:
  r10 = cn_var_加载器->搜索路径列表;
  r11 = cn_var_i_0;
  r12 = &r10[r11];
  r13 = r12;
  释放内存(r13);
  goto if_merge_9;

  if_merge_9:
  goto for_update_6;

  for_cond_10:
  r16 = cn_var_i_1;
  r17 = cn_var_最大模块缓存数量;
  r18 = r16 < r17;
  if (r18) goto for_body_11; else goto for_exit_13;

  for_body_11:
  struct 模块缓存条目* cn_var_条目_2;
  r19 = cn_var_加载器->缓存列表;
  r20 = cn_var_i_1;
  r21 = &r19[r20];
  cn_var_条目_2 = r21;
  goto while_cond_14;

  for_update_12:
  r28 = cn_var_i_1;
  r29 = r28 + 1;
  cn_var_i_1 = r29;
  goto for_cond_10;

  for_exit_13:
  r30 = cn_var_加载器;
  释放内存(r30);

  while_cond_14:
  r22 = cn_var_条目_2;
  r23 = r22 != 0;
  if (r23) goto while_body_15; else goto while_exit_16;

  while_body_15:
  struct 模块缓存条目* cn_var_下一个_3;
  r24 = cn_var_条目_2->下一个;
  cn_var_下一个_3 = r24;
  r25 = cn_var_条目_2->模块名;
  释放内存(r25);
  r26 = cn_var_条目_2;
  释放内存(r26);
  r27 = cn_var_下一个_3;
  cn_var_条目_2 = r27;
  goto while_cond_14;

  while_exit_16:
  goto for_update_12;
  return;
}

_Bool 添加搜索路径(struct 模块加载器* cn_var_加载器, const char* cn_var_路径) {
  long long r0, r5, r6, r9, r10, r11, r20, r22, r23;
  char* r3;
  char* r8;
  char* r13;
  char* r15;
  char* r16;
  char* r17;
  char* r18;
  char** r19;
  struct 模块加载器* r1;
  void* r12;
  char** r21;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r14;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_20; else goto logic_rhs_19;

  if_then_17:
  return 0;
  goto if_merge_18;

  if_merge_18:
  r5 = cn_var_加载器->搜索路径数量;
  r6 = cn_var_最大搜索路径数量;
  r7 = r5 >= r6;
  if (r7) goto if_then_21; else goto if_merge_22;

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
  r8 = cn_var_路径;
  r9 = 获取字符串长度(r8);
  cn_var_路径长度_0 = r9;
  char* cn_var_路径副本_1;
  r10 = cn_var_路径长度_0;
  r11 = r10 + 1;
  r12 = 分配内存(r11);
  cn_var_路径副本_1 = (char*)分配内存((cn_var_路径长度 + 1));
  r13 = cn_var_路径副本_1;
  r14 = r13 == 0;
  if (r14) goto if_then_23; else goto if_merge_24;

  if_then_23:
  return 0;
  goto if_merge_24;

  if_merge_24:
  r15 = cn_var_路径副本_1;
  r16 = cn_var_路径;
  r17 = 复制字符串(r15, r16);
  r18 = cn_var_路径副本_1;
  r19 = cn_var_加载器->搜索路径列表;
  r20 = cn_var_加载器->搜索路径数量;
  r21 = &r19[r20];
  *r21 = r18;
  r22 = cn_var_加载器->搜索路径数量;
  r23 = r22 + 1;
  return 1;
}

void 设置默认搜索路径(struct 模块加载器* cn_var_加载器, const char* cn_var_源文件目录) {
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

struct 模块标识符* 解析模块标识符(const char* cn_var_模块名) {
  long long r3, r4, r5, r7, r8, r10, r14, r15, r20, r23, r24, r25, r28, r32, r33, r34, r35, r36, r37, r41, r43, r44, r45, r47, r49, r54, r59, r62, r63, r64, r68, r69, r71, r72, r74, r75, r76, r77, r84, r86, r87, r89, r93, r95, r96, r97, r98;
  char* r0;
  char* r2;
  char* r17;
  char* r21;
  char* r22;
  char* r27;
  char* r48;
  char* r53;
  char* r66;
  char* r79;
  char* r81;
  char* r82;
  char* r88;
  char* r58;
  char* r61;
  char* r80;
  char* r92;
  void* r11;
  struct 模块标识符* r12;
  void* r16;
  struct 模块标识符* r19;
  void* r38;
  char** r39;
  struct 模块标识符* r42;
  char* r60;
  void* r65;
  void* r73;
  struct 模块标识符* r78;
  char** r83;
  char** r85;
  char* r94;
  struct 模块标识符* r99;
  _Bool r1;
  _Bool r6;
  _Bool r9;
  _Bool r13;
  _Bool r18;
  _Bool r26;
  _Bool r31;
  _Bool r40;
  _Bool r46;
  _Bool r52;
  _Bool r57;
  _Bool r67;
  _Bool r70;
  char r29;
  char r30;
  char r50;
  char r51;
  char r55;
  char r56;
  char r90;
  char r91;

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
  cn_var_标识符_1 = (struct 模块标识符*)分配清零内存(1, 模块标识符大小());
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
  r17 = cn_var_标识符_1->完全限定名;
  r18 = r17 == 0;
  if (r18) goto if_then_35; else goto if_merge_36;

  if_then_35:
  r19 = cn_var_标识符_1;
  释放内存(r19);
  return 0;
  goto if_merge_36;

  if_merge_36:
  r20 = cn_var_标识符_1->完全限定名;
  r21 = cn_var_模块名;
  r22 = 复制字符串(r20, r21);
  r23 = cn_var_名称长度_0;
  long long cn_var_段数量_2;
  cn_var_段数量_2 = 1;
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_37;

  for_cond_37:
  r24 = cn_var_i_3;
  r25 = cn_var_名称长度_0;
  r26 = r24 < r25;
  if (r26) goto for_body_38; else goto for_exit_40;

  for_body_38:
  r27 = cn_var_模块名;
  r28 = cn_var_i_3;
  r29 = &r27[r28];
  r30 = *r29;
  r31 = r30 == 46;
  if (r31) goto if_then_41; else goto if_merge_42;

  for_update_39:
  r34 = cn_var_i_3;
  r35 = r34 + 1;
  cn_var_i_3 = r35;
  goto for_cond_37;

  for_exit_40:
  r36 = cn_var_段数量_2;
  r37 = 字符串指针大小();
  r38 = 分配清零内存(r36, r37);
  r39 = cn_var_标识符_1->路径段列表;
  r40 = r39 == 0;
  if (r40) goto if_then_43; else goto if_merge_44;

  if_then_41:
  r32 = cn_var_段数量_2;
  r33 = r32 + 1;
  cn_var_段数量_2 = r33;
  goto if_merge_42;

  if_merge_42:
  goto for_update_39;

  if_then_43:
  r41 = cn_var_标识符_1->完全限定名;
  释放内存(r41);
  r42 = cn_var_标识符_1;
  释放内存(r42);
  return 0;
  goto if_merge_44;

  if_merge_44:
  r43 = cn_var_段数量_2;
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
  r44 = cn_var_i_6;
  r45 = cn_var_名称长度_0;
  r46 = r44 <= r45;
  if (r46) goto for_body_46; else goto for_exit_48;

  for_body_46:
  r48 = cn_var_模块名;
  r49 = cn_var_i_6;
  r50 = &r48[r49];
  r51 = *r50;
  r52 = r51 == 46;
  if (r52) goto logic_merge_53; else goto logic_rhs_52;

  for_update_47:
  r97 = cn_var_i_6;
  r98 = r97 + 1;
  cn_var_i_6 = r98;
  goto for_cond_45;

  for_exit_48:
  r99 = cn_var_标识符_1;
  return r99;

  if_then_49:
  r58 = cn_var_缓冲区;
  r59 = cn_var_缓冲区位置_4;
  r60 = &r58[r59];
  *r60 = 0;
  long long cn_var_段长度_7;
  r61 = cn_var_缓冲区;
  r62 = 获取字符串长度(r61);
  cn_var_段长度_7 = r62;
  char* cn_var_段副本_8;
  r63 = cn_var_段长度_7;
  r64 = r63 + 1;
  r65 = 分配内存(r64);
  cn_var_段副本_8 = (char*)分配内存((cn_var_段长度 + 1));
  r66 = cn_var_段副本_8;
  r67 = r66 == 0;
  if (r67) goto if_then_54; else goto if_merge_55;

  if_else_50:
  r88 = cn_var_模块名;
  r89 = cn_var_i_6;
  r90 = &r88[r89];
  r91 = *r90;
  r92 = cn_var_缓冲区;
  r93 = cn_var_缓冲区位置_4;
  r94 = &r92[r93];
  *r94 = r91;
  r95 = cn_var_缓冲区位置_4;
  r96 = r95 + 1;
  cn_var_缓冲区位置_4 = r96;
  goto if_merge_51;

  if_merge_51:
  goto for_update_47;

  logic_rhs_52:
  r53 = cn_var_模块名;
  r54 = cn_var_i_6;
  r55 = &r53[r54];
  r56 = *r55;
  r57 = r56 == 0;
  goto logic_merge_53;

  logic_merge_53:
  if (r57) goto if_then_49; else goto if_else_50;

  if_then_54:
  long long cn_var_j_9;
  cn_var_j_9 = 0;
  goto for_cond_56;

  if_merge_55:
  r79 = cn_var_段副本_8;
  r80 = cn_var_缓冲区;
  r81 = 复制字符串(r79, r80);
  r82 = cn_var_段副本_8;
  r83 = cn_var_标识符_1->路径段列表;
  r84 = cn_var_当前段索引_5;
  r85 = &r83[r84];
  *r85 = r82;
  r86 = cn_var_当前段索引_5;
  r87 = r86 + 1;
  cn_var_当前段索引_5 = r87;
  cn_var_缓冲区位置_4 = 0;
  goto if_merge_51;

  for_cond_56:
  r68 = cn_var_j_9;
  r69 = cn_var_当前段索引_5;
  r70 = r68 < r69;
  if (r70) goto for_body_57; else goto for_exit_59;

  for_body_57:
  r71 = cn_var_标识符_1->路径段列表;
  r72 = cn_var_j_9;
  r73 = (void*)cn_rt_array_get_element(r71, r72, 8);
  释放内存(r73);
  goto for_update_58;

  for_update_58:
  r74 = cn_var_j_9;
  r75 = r74 + 1;
  cn_var_j_9 = r75;
  goto for_cond_56;

  for_exit_59:
  r76 = cn_var_标识符_1->路径段列表;
  释放内存(r76);
  r77 = cn_var_标识符_1->完全限定名;
  释放内存(r77);
  r78 = cn_var_标识符_1;
  释放内存(r78);
  return 0;
  goto if_merge_55;
  return NULL;
}

void 销毁模块标识符(struct 模块标识符* cn_var_标识符) {
  long long r7, r8, r11, r15, r17, r18;
  char* r2;
  char* r4;
  char* r12;
  struct 模块标识符* r0;
  char** r5;
  char** r10;
  char** r14;
  void* r16;
  char** r19;
  struct 模块标识符* r20;
  _Bool r1;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r13;

  entry:
  r0 = cn_var_标识符;
  r1 = r0 == 0;
  if (r1) goto if_then_60; else goto if_merge_61;

  if_then_60:
  return;
  goto if_merge_61;

  if_merge_61:
  r2 = cn_var_标识符->完全限定名;
  r3 = r2 != 0;
  if (r3) goto if_then_62; else goto if_merge_63;

  if_then_62:
  r4 = cn_var_标识符->完全限定名;
  释放内存(r4);
  goto if_merge_63;

  if_merge_63:
  r5 = cn_var_标识符->路径段列表;
  r6 = r5 != 0;
  if (r6) goto if_then_64; else goto if_merge_65;

  if_then_64:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_66;

  if_merge_65:
  r20 = cn_var_标识符;
  释放内存(r20);

  for_cond_66:
  r7 = cn_var_i_0;
  r8 = cn_var_标识符->路径段数量;
  r9 = r7 < r8;
  if (r9) goto for_body_67; else goto for_exit_69;

  for_body_67:
  r10 = cn_var_标识符->路径段列表;
  r11 = cn_var_i_0;
  r12 = *(char*)cn_rt_array_get_element(r10, r11, 8);
  r13 = r12 != 0;
  if (r13) goto if_then_70; else goto if_merge_71;

  for_update_68:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_66;

  for_exit_69:
  r19 = cn_var_标识符->路径段列表;
  释放内存(r19);
  goto if_merge_65;

  if_then_70:
  r14 = cn_var_标识符->路径段列表;
  r15 = cn_var_i_0;
  r16 = (void*)cn_rt_array_get_element(r14, r15, 8);
  释放内存(r16);
  goto if_merge_71;

  if_merge_71:
  goto for_update_68;
  return;
}

_Bool 解析模块文件路径(struct 模块加载器* cn_var_加载器, struct 模块标识符* cn_var_标识符, const char* cn_var_输出路径, long long cn_var_输出路径大小) {
  long long r0, r1, r8, r9, r12, r16, r19, r21, r22, r23, r26, r28, r29, r31, r34, r36, r37, r39, r41, r42, r43, r44, r45, r47, r48, r50, r51, r53, r60, r61, r66, r67;
  char* r6;
  char* r14;
  char* r15;
  char* r18;
  char* r32;
  char* r33;
  char* r38;
  char* r49;
  char* r63;
  char* r65;
  char** r11;
  char* r17;
  char* r25;
  char* r35;
  char* r46;
  char* r52;
  char* r55;
  char* r64;
  struct 模块加载器* r2;
  struct 模块标识符* r4;
  char** r13;
  void* r20;
  char* r27;
  char** r30;
  void* r40;
  char* r54;
  void* r56;
  void* r57;
  void* r59;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r10;
  _Bool r24;
  _Bool r58;
  _Bool r62;

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
  r9 = cn_var_加载器->搜索路径数量;
  r10 = r8 < r9;
  if (r10) goto for_body_79; else goto for_exit_81;

  for_body_79:
  char* cn_var_搜索路径_1;
  r11 = cn_var_加载器->搜索路径列表;
  r12 = cn_var_i_0;
  r13 = &r11[r12];
  r14 = r13;
  cn_var_搜索路径_1 = r14;
  char* cn_var_路径缓冲区;
  cn_var_路径缓冲区 = cn_rt_array_alloc(8, 1024);
  long long cn_var_路径位置_2;
  cn_var_路径位置_2 = 0;
  long long cn_var_搜索路径长度_3;
  r15 = cn_var_搜索路径_1;
  r16 = 获取字符串长度(r15);
  cn_var_搜索路径长度_3 = r16;
  r17 = cn_var_路径缓冲区;
  r18 = cn_var_搜索路径_1;
  r19 = cn_var_搜索路径长度_3;
  r20 = 复制内存(r17, r18, r19);
  r21 = cn_var_搜索路径长度_3;
  cn_var_路径位置_2 = r21;
  long long cn_var_j_4;
  cn_var_j_4 = 0;
  goto for_cond_82;

  for_update_80:
  r66 = cn_var_i_0;
  r67 = r66 + 1;
  cn_var_i_0 = r67;
  goto for_cond_78;

  for_exit_81:
  return 0;

  for_cond_82:
  r22 = cn_var_j_4;
  r23 = cn_var_标识符->路径段数量;
  r24 = r22 < r23;
  if (r24) goto for_body_83; else goto for_exit_85;

  for_body_83:
  r25 = cn_var_路径缓冲区;
  r26 = cn_var_路径位置_2;
  r27 = &r25[r26];
  *r27 = 47;
  r28 = cn_var_路径位置_2;
  r29 = r28 + 1;
  cn_var_路径位置_2 = r29;
  char* cn_var_段_5;
  r30 = cn_var_标识符->路径段列表;
  r31 = cn_var_j_4;
  r32 = *(char*)cn_rt_array_get_element(r30, r31, 8);
  cn_var_段_5 = r32;
  long long cn_var_段长度_6;
  r33 = cn_var_段_5;
  r34 = 获取字符串长度(r33);
  cn_var_段长度_6 = r34;
  r35 = cn_var_路径缓冲区;
  r36 = cn_var_路径位置_2;
  r37 = r35 + r36;
  r38 = cn_var_段_5;
  r39 = cn_var_段长度_6;
  r40 = 复制内存(r37, r38, r39);
  r41 = cn_var_路径位置_2;
  r42 = cn_var_段长度_6;
  r43 = r41 + r42;
  cn_var_路径位置_2 = r43;
  goto for_update_84;

  for_update_84:
  r44 = cn_var_j_4;
  r45 = r44 + 1;
  cn_var_j_4 = r45;
  goto for_cond_82;

  for_exit_85:
  r46 = cn_var_路径缓冲区;
  r47 = cn_var_路径位置_2;
  r48 = r46 + r47;
  r49 = 复制字符串(r48, ".cn");
  r50 = cn_var_路径位置_2;
  r51 = r50 + 3;
  cn_var_路径位置_2 = r51;
  r52 = cn_var_路径缓冲区;
  r53 = cn_var_路径位置_2;
  r54 = &r52[r53];
  *r54 = 0;
  void* cn_var_文件_7;
  r55 = cn_var_路径缓冲区;
  r56 = 打开文件(r55, "r");
  cn_var_文件_7 = r56;
  r57 = cn_var_文件_7;
  r58 = r57 != 0;
  if (r58) goto if_then_86; else goto if_merge_87;

  if_then_86:
  r59 = cn_var_文件_7;
  关闭文件(r59);
  r60 = cn_var_路径位置_2;
  r61 = cn_var_输出路径大小;
  r62 = r60 < r61;
  if (r62) goto if_then_88; else goto if_merge_89;

  if_merge_87:
  goto for_update_80;

  if_then_88:
  r63 = cn_var_输出路径;
  r64 = cn_var_路径缓冲区;
  r65 = 复制字符串(r63, r64);
  return 1;
  goto if_merge_89;

  if_merge_89:
  goto if_merge_87;
  return 0;
}

long long 计算哈希值(const char* cn_var_字符串指针) {
  long long r2, r4, r8, r9, r11, r14, r15, r16, r17, r19, r20, r21;
  char* r0;
  char* r3;
  char* r10;
  _Bool r1;
  _Bool r7;
  _Bool r18;
  char r5;
  char r6;
  char r12;
  char r13;

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
  r5 = &r3[r4];
  r6 = *r5;
  r7 = r6 != 0;
  if (r7) goto for_body_93; else goto for_exit_95;

  for_body_93:
  r8 = cn_var_哈希_0;
  r9 = r8 * 31;
  r10 = cn_var_字符串指针;
  r11 = cn_var_i_1;
  r12 = &r10[r11];
  r13 = *r12;
  r14 = r9 + r13;
  cn_var_哈希_0 = r14;
  goto for_update_94;

  for_update_94:
  r15 = cn_var_i_1;
  r16 = r15 + 1;
  cn_var_i_1 = r16;
  goto for_cond_92;

  for_exit_95:
  r17 = cn_var_哈希_0;
  r18 = r17 < 0;
  if (r18) goto if_then_96; else goto if_merge_97;

  if_then_96:
  r19 = cn_var_哈希_0;
  r20 = -r19;
  cn_var_哈希_0 = r20;
  goto if_merge_97;

  if_merge_97:
  r21 = cn_var_哈希_0;
  return r21;
}

struct 模块元数据* 从缓存获取模块(struct 模块加载器* cn_var_加载器, const char* cn_var_模块名) {
  long long r0, r6, r7, r8, r9, r11, r17;
  char* r3;
  char* r5;
  char* r15;
  char* r16;
  struct 模块缓存条目** r10;
  struct 模块加载器* r1;
  struct 模块缓存条目** r12;
  struct 模块缓存条目* r13;
  struct 模块元数据* r19;
  struct 模块缓存条目* r20;
  _Bool r2;
  _Bool r4;
  _Bool r14;
  _Bool r18;

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
  r10 = cn_var_加载器->缓存列表;
  r11 = cn_var_索引_1;
  r12 = &r10[r11];
  cn_var_条目_2 = r12;
  goto while_cond_102;

  logic_rhs_100:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_101;

  logic_merge_101:
  if (r4) goto if_then_98; else goto if_merge_99;

  while_cond_102:
  r13 = cn_var_条目_2;
  r14 = r13 != 0;
  if (r14) goto while_body_103; else goto while_exit_104;

  while_body_103:
  r15 = cn_var_条目_2->模块名;
  r16 = cn_var_模块名;
  r17 = 比较字符串(r15, r16);
  r18 = r17 == 0;
  if (r18) goto if_then_105; else goto if_merge_106;

  while_exit_104:
  return 0;

  if_then_105:
  r19 = cn_var_条目_2->元数据;
  return r19;
  goto if_merge_106;

  if_merge_106:
  r20 = cn_var_条目_2->下一个;
  cn_var_条目_2 = r20;
  goto while_cond_102;
  return NULL;
}

_Bool 添加模块到缓存(struct 模块加载器* cn_var_加载器, const char* cn_var_模块名, struct 模块元数据* cn_var_元数据) {
  long long r0, r1, r9, r10, r11, r12, r13, r18, r19, r20, r30, r34, r36, r37;
  char* r4;
  char* r8;
  char* r17;
  char* r22;
  char* r25;
  char* r26;
  char* r27;
  struct 模块缓存条目** r29;
  struct 模块缓存条目** r33;
  struct 模块加载器* r2;
  struct 模块元数据* r6;
  void* r14;
  struct 模块缓存条目* r15;
  void* r21;
  struct 模块缓存条目* r24;
  struct 模块元数据* r28;
  struct 模块缓存条目** r31;
  struct 模块缓存条目* r32;
  struct 模块缓存条目** r35;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r16;
  _Bool r23;

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
  cn_var_新条目_2 = (struct 模块缓存条目*)分配清零内存(1, 模块缓存条目大小());
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
  r22 = cn_var_新条目_2->模块名;
  r23 = r22 == 0;
  if (r23) goto if_then_115; else goto if_merge_116;

  if_then_115:
  r24 = cn_var_新条目_2;
  释放内存(r24);
  return 0;
  goto if_merge_116;

  if_merge_116:
  r25 = cn_var_新条目_2->模块名;
  r26 = cn_var_模块名;
  r27 = 复制字符串(r25, r26);
  r28 = cn_var_元数据;
  r29 = cn_var_加载器->缓存列表;
  r30 = cn_var_索引_1;
  r31 = &r29[r30];
  r32 = cn_var_新条目_2;
  r33 = cn_var_加载器->缓存列表;
  r34 = cn_var_索引_1;
  r35 = &r33[r34];
  *r35 = r32;
  r36 = cn_var_加载器->缓存数量;
  r37 = r36 + 1;
  return 1;
}

_Bool 读取文件内容(const char* cn_var_文件路径, char** cn_var_输出缓冲区, long long* cn_var_输出大小) {
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
  cn_var_缓冲区_2 = (char*)分配内存((cn_var_文件大小 + 1));
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
  cn_var_元数据_0 = (struct 模块元数据*)分配清零内存(1, 模块元数据大小());
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
  char* r5;
  char* r7;
  struct 模块元数据* r0;
  struct 模块标识符* r2;
  struct 模块标识符* r4;
  struct 导出符号* r8;
  struct 导出符号* r10;
  struct 模块依赖* r11;
  struct 模块依赖* r13;
  struct 模块元数据* r14;
  _Bool r1;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;

  entry:
  r0 = cn_var_元数据;
  r1 = r0 == 0;
  if (r1) goto if_then_135; else goto if_merge_136;

  if_then_135:
  return;
  goto if_merge_136;

  if_merge_136:
  r2 = cn_var_元数据->标识符;
  r3 = r2 != 0;
  if (r3) goto if_then_137; else goto if_merge_138;

  if_then_137:
  r4 = cn_var_元数据->标识符;
  销毁模块标识符(r4);
  goto if_merge_138;

  if_merge_138:
  r5 = cn_var_元数据->文件路径;
  r6 = r5 != 0;
  if (r6) goto if_then_139; else goto if_merge_140;

  if_then_139:
  r7 = cn_var_元数据->文件路径;
  释放内存(r7);
  goto if_merge_140;

  if_merge_140:
  r8 = cn_var_元数据->导出符号表;
  r9 = r8 != 0;
  if (r9) goto if_then_141; else goto if_merge_142;

  if_then_141:
  r10 = cn_var_元数据->导出符号表;
  释放内存(r10);
  goto if_merge_142;

  if_merge_142:
  r11 = cn_var_元数据->依赖列表;
  r12 = r11 != 0;
  if (r12) goto if_then_143; else goto if_merge_144;

  if_then_143:
  r13 = cn_var_元数据->依赖列表;
  释放内存(r13);
  goto if_merge_144;

  if_merge_144:
  r14 = cn_var_元数据;
  释放内存(r14);
  return;
}

struct 模块元数据* 加载模块核心(struct 模块加载器* cn_var_加载器, const char* cn_var_模块名) {
  long long r0, r11, r12, r35, r36, r38, r39, r40, r45, r46, r50, r53, r54, r55, r61, r62, r64, r70, r71, r80, r81, r90, r91, r100, r101;
  char* r3;
  char* r6;
  char* r16;
  char* r42;
  char* r47;
  char* r49;
  char* r52;
  char* r63;
  char* r68;
  char* r78;
  char* r88;
  char* r95;
  char* r97;
  char* r29;
  char* r37;
  char* r48;
  char* r51;
  struct 模块加载器* r1;
  struct 模块加载器* r5;
  struct 模块元数据* r7;
  struct 模块元数据* r8;
  struct 模块元数据* r10;
  struct 诊断集合* r14;
  struct 模块标识符* r17;
  struct 模块标识符* r18;
  struct 诊断集合* r20;
  struct 模块标识符* r22;
  struct 模块元数据* r23;
  struct 模块元数据* r24;
  struct 模块标识符* r26;
  struct 模块加载器* r27;
  struct 模块标识符* r28;
  struct 诊断集合* r32;
  struct 模块元数据* r34;
  void* r41;
  struct 模块元数据* r44;
  struct 诊断集合* r58;
  struct 模块元数据* r60;
  struct 扫描器* r65;
  struct 扫描器* r66;
  struct 模块元数据* r69;
  struct 扫描器* r72;
  struct 诊断集合* r73;
  struct 解析器* r74;
  struct 解析器* r75;
  struct 扫描器* r77;
  struct 模块元数据* r79;
  struct 解析器* r82;
  struct 程序节点* r83;
  struct 程序节点* r84;
  struct 解析器* r86;
  struct 扫描器* r87;
  struct 模块元数据* r89;
  struct 程序节点* r92;
  struct 解析器* r93;
  struct 扫描器* r94;
  struct 模块加载器* r96;
  struct 模块元数据* r98;
  struct 模块元数据* r102;
  _Bool r2;
  _Bool r4;
  _Bool r9;
  _Bool r13;
  _Bool r19;
  _Bool r25;
  _Bool r30;
  _Bool r31;
  _Bool r43;
  _Bool r56;
  _Bool r57;
  _Bool r67;
  _Bool r76;
  _Bool r85;
  _Bool r99;
  struct 源位置 r15;
  struct 源位置 r21;
  struct 源位置 r33;
  struct 源位置 r59;

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
  r11 = cn_var_加载器->当前依赖深度;
  r12 = cn_var_最大依赖深度;
  r13 = r11 >= r12;
  if (r13) goto if_then_151; else goto if_merge_152;

  if_then_151:
  r14 = cn_var_加载器->诊断集合;
  r15 = cn_var_空源位置;
  报告错误(r14, 诊断错误码_未知错误, r15, "模块依赖深度超过限制");
  return 0;
  goto if_merge_152;

  if_merge_152:
  struct 模块标识符* cn_var_标识符_1;
  r16 = cn_var_模块名;
  r17 = 解析模块标识符(r16);
  cn_var_标识符_1 = r17;
  r18 = cn_var_标识符_1;
  r19 = r18 == 0;
  if (r19) goto if_then_153; else goto if_merge_154;

  if_then_153:
  r20 = cn_var_加载器->诊断集合;
  r21 = cn_var_空源位置;
  报告错误(r20, 诊断错误码_未知错误, r21, "无法解析模块标识符");
  return 0;
  goto if_merge_154;

  if_merge_154:
  struct 模块元数据* cn_var_元数据_2;
  r22 = cn_var_标识符_1;
  r23 = 创建模块元数据(r22);
  cn_var_元数据_2 = r23;
  r24 = cn_var_元数据_2;
  r25 = r24 == 0;
  if (r25) goto if_then_155; else goto if_merge_156;

  if_then_155:
  r26 = cn_var_标识符_1;
  销毁模块标识符(r26);
  return 0;
  goto if_merge_156;

  if_merge_156:
  char* cn_var_文件路径缓冲区;
  cn_var_文件路径缓冲区 = cn_rt_array_alloc(8, 1024);
  r27 = cn_var_加载器;
  r28 = cn_var_标识符_1;
  r29 = cn_var_文件路径缓冲区;
  r30 = 解析模块文件路径(r27, r28, r29, 1024);
  r31 = !r30;
  if (r31) goto if_then_157; else goto if_merge_158;

  if_then_157:
  r32 = cn_var_加载器->诊断集合;
  r33 = cn_var_空源位置;
  报告错误(r32, 诊断错误码_未知错误, r33, "无法找到模块文件");
  r34 = cn_var_元数据_2;
  销毁模块元数据(r34);
  r35 = cn_var_加载器->加载失败数;
  r36 = r35 + 1;
  return 0;
  goto if_merge_158;

  if_merge_158:
  long long cn_var_路径长度_3;
  r37 = cn_var_文件路径缓冲区;
  r38 = 获取字符串长度(r37);
  cn_var_路径长度_3 = r38;
  r39 = cn_var_路径长度_3;
  r40 = r39 + 1;
  r41 = 分配内存(r40);
  r42 = cn_var_元数据_2->文件路径;
  r43 = r42 == 0;
  if (r43) goto if_then_159; else goto if_merge_160;

  if_then_159:
  r44 = cn_var_元数据_2;
  销毁模块元数据(r44);
  r45 = cn_var_加载器->加载失败数;
  r46 = r45 + 1;
  return 0;
  goto if_merge_160;

  if_merge_160:
  r47 = cn_var_元数据_2->文件路径;
  r48 = cn_var_文件路径缓冲区;
  r49 = 复制字符串(r47, r48);
  r50 = cn_var_路径长度_3;
  char* cn_var_源码内容_4;
  cn_var_源码内容_4 = 0;
  long long cn_var_源码大小_5;
  cn_var_源码大小_5 = 0;
  r51 = cn_var_文件路径缓冲区;
  r52 = cn_var_源码内容_4;
  r53 = &cn_var_源码内容_4;
  r54 = cn_var_源码大小_5;
  r55 = &cn_var_源码大小_5;
  r56 = 读取文件内容(r51, r53, r55);
  r57 = !r56;
  if (r57) goto if_then_161; else goto if_merge_162;

  if_then_161:
  r58 = cn_var_加载器->诊断集合;
  r59 = cn_var_空源位置;
  报告错误(r58, 诊断错误码_未知错误, r59, "无法读取模块文件");
  r60 = cn_var_元数据_2;
  销毁模块元数据(r60);
  r61 = cn_var_加载器->加载失败数;
  r62 = r61 + 1;
  return 0;
  goto if_merge_162;

  if_merge_162:
  struct 扫描器* cn_var_扫描器实例_6;
  r63 = cn_var_源码内容_4;
  r64 = cn_var_源码大小_5;
  r65 = 创建扫描器(r63, r64);
  cn_var_扫描器实例_6 = r65;
  r66 = cn_var_扫描器实例_6;
  r67 = r66 == 0;
  if (r67) goto if_then_163; else goto if_merge_164;

  if_then_163:
  r68 = cn_var_源码内容_4;
  释放内存(r68);
  r69 = cn_var_元数据_2;
  销毁模块元数据(r69);
  r70 = cn_var_加载器->加载失败数;
  r71 = r70 + 1;
  return 0;
  goto if_merge_164;

  if_merge_164:
  struct 解析器* cn_var_解析器实例_7;
  r72 = cn_var_扫描器实例_6;
  r73 = cn_var_加载器->诊断集合;
  r74 = 创建解析器(r72, r73);
  cn_var_解析器实例_7 = r74;
  r75 = cn_var_解析器实例_7;
  r76 = r75 == 0;
  if (r76) goto if_then_165; else goto if_merge_166;

  if_then_165:
  r77 = cn_var_扫描器实例_6;
  销毁扫描器(r77);
  r78 = cn_var_源码内容_4;
  释放内存(r78);
  r79 = cn_var_元数据_2;
  销毁模块元数据(r79);
  r80 = cn_var_加载器->加载失败数;
  r81 = r80 + 1;
  return 0;
  goto if_merge_166;

  if_merge_166:
  struct 程序节点* cn_var_程序_8;
  r82 = cn_var_解析器实例_7;
  r83 = 解析程序(r82);
  cn_var_程序_8 = r83;
  r84 = cn_var_程序_8;
  r85 = r84 == 0;
  if (r85) goto if_then_167; else goto if_merge_168;

  if_then_167:
  r86 = cn_var_解析器实例_7;
  销毁解析器(r86);
  r87 = cn_var_扫描器实例_6;
  销毁扫描器(r87);
  r88 = cn_var_源码内容_4;
  释放内存(r88);
  r89 = cn_var_元数据_2;
  销毁模块元数据(r89);
  r90 = cn_var_加载器->加载失败数;
  r91 = r90 + 1;
  return 0;
  goto if_merge_168;

  if_merge_168:
  r92 = cn_var_程序_8;
  r93 = cn_var_解析器实例_7;
  销毁解析器(r93);
  r94 = cn_var_扫描器实例_6;
  销毁扫描器(r94);
  r95 = cn_var_源码内容_4;
  释放内存(r95);
  r96 = cn_var_加载器;
  r97 = cn_var_模块名;
  r98 = cn_var_元数据_2;
  r99 = 添加模块到缓存(r96, r97, r98);
  r100 = cn_var_加载器->加载成功数;
  r101 = r100 + 1;
  r102 = cn_var_元数据_2;
  return r102;
}

struct 模块元数据* 加载模块(struct 模块加载器* cn_var_加载器, const char* cn_var_模块名) {
  long long r0, r6, r7, r8, r13;
  char* r3;
  char* r10;
  struct 模块加载器* r1;
  struct 模块元数据* r5;
  struct 模块加载器* r9;
  struct 模块元数据* r11;
  struct 模块元数据* r12;
  struct 模块元数据* r14;
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
  r5 = cn_var_加载器->当前模块;
  cn_var_保存当前模块_0 = r5;
  long long cn_var_保存依赖深度_1;
  r6 = cn_var_加载器->当前依赖深度;
  cn_var_保存依赖深度_1 = r6;
  r7 = cn_var_加载器->当前依赖深度;
  r8 = r7 + 1;
  struct 模块元数据* cn_var_结果_2;
  r9 = cn_var_加载器;
  r10 = cn_var_模块名;
  r11 = 加载模块核心(r9, r10);
  cn_var_结果_2 = r11;
  r12 = cn_var_保存当前模块_0;
  r13 = cn_var_保存依赖深度_1;
  r14 = cn_var_结果_2;
  return r14;

  logic_rhs_171:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_172;

  logic_merge_172:
  if (r4) goto if_then_169; else goto if_merge_170;
  return NULL;
}

_Bool 处理导入语句(struct 模块加载器* cn_var_加载器, struct 导入声明* cn_var_导入节点, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r16, r17, r19, r20, r21, r24, r25, r26, r31, r33, r37, r38;
  char* r8;
  char* r9;
  char* r12;
  struct 模块加载器* r2;
  struct 导入声明* r4;
  struct 模块元数据* r6;
  struct 模块加载器* r11;
  struct 模块元数据* r13;
  struct 模块元数据* r14;
  struct 模块依赖* r23;
  void* r27;
  struct 模块依赖* r28;
  struct 模块依赖* r30;
  struct 模块依赖* r32;
  struct 模块依赖* r35;
  struct 模块标识符* r36;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r10;
  _Bool r15;
  _Bool r18;
  _Bool r22;
  _Bool r29;
  struct 模块依赖 r34;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_178; else goto logic_rhs_177;

  if_then_173:
  return 0;
  goto if_merge_174;

  if_merge_174:
  char* cn_var_模块名_0;
  r8 = cn_var_导入节点->模块名;
  cn_var_模块名_0 = r8;
  r9 = cn_var_模块名_0;
  r10 = r9 == 0;
  if (r10) goto if_then_179; else goto if_merge_180;

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
  r11 = cn_var_加载器;
  r12 = cn_var_模块名_0;
  r13 = 加载模块(r11, r12);
  cn_var_依赖模块_1 = r13;
  r14 = cn_var_依赖模块_1;
  r15 = r14 == 0;
  if (r15) goto if_then_181; else goto if_merge_182;

  if_then_181:
  return 0;
  goto if_merge_182;

  if_merge_182:
  r16 = cn_var_当前模块->依赖数量;
  r17 = cn_var_当前模块->依赖容量;
  r18 = r16 >= r17;
  if (r18) goto if_then_183; else goto if_merge_184;

  if_then_183:
  long long cn_var_新容量_2;
  r19 = cn_var_当前模块->依赖容量;
  r20 = r19 << 1;
  cn_var_新容量_2 = r20;
  r21 = cn_var_新容量_2;
  r22 = r21 == 0;
  if (r22) goto if_then_185; else goto if_merge_186;

  if_merge_184:
  struct 模块依赖* cn_var_依赖_4;
  r32 = cn_var_当前模块->依赖列表;
  r33 = cn_var_当前模块->依赖数量;
  r34 = (struct 模块依赖*)cn_rt_array_get_element(r32, r33, 8);
  cn_var_依赖_4 = r35;
  r36 = cn_var_依赖模块_1->标识符;
  r37 = cn_var_当前模块->依赖数量;
  r38 = r37 + 1;
  return 1;

  if_then_185:
  cn_var_新容量_2 = 4;
  goto if_merge_186;

  if_merge_186:
  struct 模块依赖* cn_var_新数组_3;
  r23 = cn_var_当前模块->依赖列表;
  r24 = cn_var_新容量_2;
  r25 = 模块依赖大小();
  r26 = r24 * r25;
  r27 = 重新分配内存(r23, r26);
  cn_var_新数组_3 = (struct 模块依赖*)重新分配内存(cn_var_当前模块->依赖列表, (cn_var_新容量 * 模块依赖大小()));
  r28 = cn_var_新数组_3;
  r29 = r28 == 0;
  if (r29) goto if_then_187; else goto if_merge_188;

  if_then_187:
  return 0;
  goto if_merge_188;

  if_merge_188:
  r30 = cn_var_新数组_3;
  r31 = cn_var_新容量_2;
  goto if_merge_184;
  return 0;
}

_Bool 处理所有导入语句(struct 模块加载器* cn_var_加载器, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r13;
  struct 模块加载器* r2;
  struct 模块元数据* r4;
  struct 程序节点* r6;
  struct 程序节点* r8;
  struct 声明节点列表* r9;
  struct 声明节点列表* r10;
  struct 声明节点* r12;
  struct 声明节点* r14;
  struct 导入声明* r18;
  struct 模块加载器* r19;
  struct 导入声明* r20;
  struct 模块元数据* r21;
  struct 声明节点列表* r24;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r15;
  _Bool r17;
  _Bool r22;
  _Bool r23;
  enum 节点类型 r16;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_194; else goto logic_rhs_193;

  if_then_189:
  return 0;
  goto if_merge_190;

  if_merge_190:
  struct 程序节点* cn_var_程序_0;
  r8 = cn_var_当前模块->程序节点;
  cn_var_程序_0 = r8;
  struct 声明节点列表* cn_var_声明列表_1;
  r9 = cn_var_程序_0->声明列表;
  cn_var_声明列表_1 = r9;
  goto while_cond_195;

  logic_rhs_191:
  r6 = cn_var_当前模块->程序节点;
  r7 = r6 == 0;
  goto logic_merge_192;

  logic_merge_192:
  if (r7) goto if_then_189; else goto if_merge_190;

  logic_rhs_193:
  r4 = cn_var_当前模块;
  r5 = r4 == 0;
  goto logic_merge_194;

  logic_merge_194:
  if (r5) goto logic_merge_192; else goto logic_rhs_191;

  while_cond_195:
  r10 = cn_var_声明列表_1;
  r11 = r10 != 0;
  if (r11) goto while_body_196; else goto while_exit_197;

  while_body_196:
  struct 声明节点* cn_var_声明_2;
  r12 = cn_var_声明列表_1->节点;
  cn_var_声明_2 = r12;
  r14 = cn_var_声明_2;
  r15 = r14 != 0;
  if (r15) goto logic_rhs_200; else goto logic_merge_201;

  while_exit_197:
  return 1;

  if_then_198:
  struct 导入声明* cn_var_导入节点_3;
  r18 = cn_var_声明_2->作为导入语句;
  cn_var_导入节点_3 = r18;
  r19 = cn_var_加载器;
  r20 = cn_var_导入节点_3;
  r21 = cn_var_当前模块;
  r22 = 处理导入语句(r19, r20, r21);
  r23 = !r22;
  if (r23) goto if_then_202; else goto if_merge_203;

  if_merge_199:
  r24 = cn_var_声明列表_1->下一个;
  cn_var_声明列表_1 = r24;
  goto while_cond_195;

  logic_rhs_200:
  r16 = cn_var_声明_2->类型;
  r17 = r16 == 节点类型_导入声明;
  goto logic_merge_201;

  logic_merge_201:
  if (r17) goto if_then_198; else goto if_merge_199;

  if_then_202:
  goto if_merge_203;

  if_merge_203:
  goto if_merge_199;
  return 0;
}

_Bool 收集导出符号(struct 模块元数据* cn_var_元数据) {
  long long r0, r10, r14, r15, r17, r19, r20, r24, r29, r34, r42, r44, r50, r52, r53, r61, r62, r63, r64;
  char* r43;
  char* r51;
  char* r59;
  char* r60;
  struct 模块元数据* r1;
  struct 程序节点* r3;
  struct 程序节点* r5;
  struct 声明节点列表* r6;
  struct 声明节点列表* r7;
  struct 声明节点* r9;
  struct 声明节点* r11;
  struct 声明节点列表* r16;
  void* r21;
  struct 导出符号* r22;
  struct 声明节点列表* r25;
  struct 声明节点列表* r26;
  struct 声明节点* r28;
  struct 声明节点* r30;
  struct 导出符号* r33;
  struct 导出符号* r36;
  struct 函数声明* r39;
  struct 函数声明* r40;
  struct 变量声明* r47;
  struct 变量声明* r48;
  struct 变量声明* r56;
  struct 变量声明* r57;
  struct 声明节点列表* r65;
  _Bool r2;
  _Bool r4;
  _Bool r8;
  _Bool r12;
  _Bool r13;
  _Bool r18;
  _Bool r23;
  _Bool r27;
  _Bool r31;
  _Bool r32;
  _Bool r38;
  _Bool r41;
  _Bool r46;
  _Bool r49;
  _Bool r55;
  _Bool r58;
  struct 导出符号 r35;
  enum 节点类型 r37;
  enum 节点类型 r45;
  enum 节点类型 r54;

  entry:
  r1 = cn_var_元数据;
  r2 = r1 == 0;
  if (r2) goto logic_merge_207; else goto logic_rhs_206;

  if_then_204:
  return 0;
  goto if_merge_205;

  if_merge_205:
  struct 程序节点* cn_var_程序_0;
  r5 = cn_var_元数据->程序节点;
  cn_var_程序_0 = r5;
  long long cn_var_符号数量_1;
  cn_var_符号数量_1 = 0;
  struct 声明节点列表* cn_var_声明列表_2;
  r6 = cn_var_程序_0->声明列表;
  cn_var_声明列表_2 = r6;
  goto while_cond_208;

  logic_rhs_206:
  r3 = cn_var_元数据->程序节点;
  r4 = r3 == 0;
  goto logic_merge_207;

  logic_merge_207:
  if (r4) goto if_then_204; else goto if_merge_205;

  while_cond_208:
  r7 = cn_var_声明列表_2;
  r8 = r7 != 0;
  if (r8) goto while_body_209; else goto while_exit_210;

  while_body_209:
  struct 声明节点* cn_var_声明_3;
  r9 = cn_var_声明列表_2->节点;
  cn_var_声明_3 = r9;
  r11 = cn_var_声明_3;
  r12 = r11 != 0;
  if (r12) goto logic_rhs_213; else goto logic_merge_214;

  while_exit_210:
  r17 = cn_var_符号数量_1;
  r18 = r17 == 0;
  if (r18) goto if_then_215; else goto if_merge_216;

  if_then_211:
  r14 = cn_var_符号数量_1;
  r15 = r14 + 1;
  cn_var_符号数量_1 = r15;
  goto if_merge_212;

  if_merge_212:
  r16 = cn_var_声明列表_2->下一个;
  cn_var_声明列表_2 = r16;
  goto while_cond_208;

  logic_rhs_213:
  r13 = cn_var_声明_3->是否公开;
  goto logic_merge_214;

  logic_merge_214:
  if (r13) goto if_then_211; else goto if_merge_212;

  if_then_215:
  return 1;
  goto if_merge_216;

  if_merge_216:
  r19 = cn_var_符号数量_1;
  r20 = 导出符号大小();
  r21 = 分配清零内存(r19, r20);
  r22 = cn_var_元数据->导出符号表;
  r23 = r22 == 0;
  if (r23) goto if_then_217; else goto if_merge_218;

  if_then_217:
  return 0;
  goto if_merge_218;

  if_merge_218:
  r24 = cn_var_符号数量_1;
  r25 = cn_var_程序_0->声明列表;
  cn_var_声明列表_2 = r25;
  long long cn_var_当前索引_4;
  cn_var_当前索引_4 = 0;
  goto while_cond_219;

  while_cond_219:
  r26 = cn_var_声明列表_2;
  r27 = r26 != 0;
  if (r27) goto while_body_220; else goto while_exit_221;

  while_body_220:
  struct 声明节点* cn_var_声明_5;
  r28 = cn_var_声明列表_2->节点;
  cn_var_声明_5 = r28;
  r30 = cn_var_声明_5;
  r31 = r30 != 0;
  if (r31) goto logic_rhs_224; else goto logic_merge_225;

  while_exit_221:
  return 1;

  if_then_222:
  struct 导出符号* cn_var_符号_6;
  r33 = cn_var_元数据->导出符号表;
  r34 = cn_var_当前索引_4;
  r35 = (struct 导出符号*)cn_rt_array_get_element(r33, r34, 8);
  cn_var_符号_6 = r36;
  r37 = cn_var_声明_5->类型;
  r38 = r37 == 节点类型_函数声明;
  if (r38) goto if_then_226; else goto if_else_227;

  if_merge_223:
  r65 = cn_var_声明列表_2->下一个;
  cn_var_声明列表_2 = r65;
  goto while_cond_219;

  logic_rhs_224:
  r32 = cn_var_声明_5->是否公开;
  goto logic_merge_225;

  logic_merge_225:
  if (r32) goto if_then_222; else goto if_merge_223;

  if_then_226:
  struct 函数声明* cn_var_函数声明_7;
  r39 = cn_var_声明_5->作为函数声明;
  cn_var_函数声明_7 = r39;
  r40 = cn_var_函数声明_7;
  r41 = r40 != 0;
  if (r41) goto if_then_229; else goto if_merge_230;

  if_else_227:
  r45 = cn_var_声明_5->类型;
  r46 = r45 == 节点类型_变量声明;
  if (r46) goto if_then_231; else goto if_else_232;

  if_merge_228:
  r62 = cn_var_当前索引_4;
  r63 = r62 + 1;
  cn_var_当前索引_4 = r63;
  r64 = cn_var_当前索引_4;
  goto if_merge_223;

  if_then_229:
  r42 = cn_var_函数声明_7->名称;
  r43 = cn_var_函数声明_7->名称;
  r44 = 获取字符串长度(r43);
  goto if_merge_230;

  if_merge_230:
  goto if_merge_228;

  if_then_231:
  struct 变量声明* cn_var_变量声明_8;
  r47 = cn_var_声明_5->作为变量声明;
  cn_var_变量声明_8 = r47;
  r48 = cn_var_变量声明_8;
  r49 = r48 != 0;
  if (r49) goto if_then_234; else goto if_merge_235;

  if_else_232:
  r54 = cn_var_声明_5->类型;
  r55 = r54 == 节点类型_变量声明;
  if (r55) goto if_then_236; else goto if_merge_237;

  if_merge_233:
  goto if_merge_228;

  if_then_234:
  r50 = cn_var_变量声明_8->名称;
  r51 = cn_var_变量声明_8->名称;
  r52 = 获取字符串长度(r51);
  r53 = cn_var_变量声明_8->是常量;
  goto if_merge_235;

  if_merge_235:
  goto if_merge_233;

  if_then_236:
  struct 变量声明* cn_var_常量声明_9;
  r56 = cn_var_声明_5->作为常量声明;
  cn_var_常量声明_9 = r56;
  r57 = cn_var_常量声明_9;
  r58 = r57 != 0;
  if (r58) goto if_then_238; else goto if_merge_239;

  if_merge_237:
  goto if_merge_233;

  if_then_238:
  r59 = cn_var_常量声明_9->名称;
  r60 = cn_var_常量声明_9->名称;
  r61 = 获取字符串长度(r60);
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

void 报告错误(struct 诊断集合* cn_var_诊断集合指针, enum 诊断错误码 cn_var_错误码, struct 源位置 cn_var_位置, const char* cn_var_消息) {
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

