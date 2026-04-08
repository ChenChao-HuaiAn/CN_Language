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
enum 编译模式 {
    编译模式_目标模式 = 1,
    编译模式_宿主模式 = 0
};
enum IR操作码 {
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
enum C类型种类 {
    C类型种类_C函数指针类型 = 13,
    C类型种类_C枚举类型 = 12,
    C类型种类_C联合体类型 = 11,
    C类型种类_C结构体类型 = 10,
    C类型种类_C数组类型 = 9,
    C类型种类_C指针类型 = 8,
    C类型种类_C字符串类型 = 7,
    C类型种类_C布尔类型 = 6,
    C类型种类_C字符类型 = 5,
    C类型种类_C浮点类型 = 4,
    C类型种类_C小数类型 = 3,
    C类型种类_C长整数类型 = 2,
    C类型种类_C整数类型 = 1,
    C类型种类_C空类型 = 0
};
enum 模块加载状态 {
    模块加载状态_加载失败 = 3,
    模块加载状态_已加载 = 2,
    模块加载状态_加载中 = 1,
    模块加载状态_未加载 = 0
};
enum 模块类型 {
    模块类型_内建模块 = 3,
    模块类型_内联模块 = 2,
    模块类型_包模块 = 1,
    模块类型_文件模块 = 0
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
struct 表达式节点;
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
    long long* 语句体;
};
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    long long* 语句体;
};
struct 声明节点;
struct 声明节点列表;
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
    long long* 作为函数声明;
    long long* 作为变量声明;
    long long* 作为常量声明;
    long long* 作为结构体声明;
    long long* 作为枚举声明;
    long long* 作为类声明;
    long long* 作为接口声明;
    long long* 作为导入语句;
    long long* 作为模板函数声明;
    long long* 作为模板结构体声明;
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
    long long* 字段;
    long long* 方法;
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
struct 语义检查上下文;
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
struct 作用域构建器;
struct 作用域构建器 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
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
    struct IR操作数** 参数列表;
    long long 参数数量;
    struct IR操作数** 局部变量;
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
    struct IR操作数** 额外参数;
    long long 额外参数数量;
    struct IR指令* 下一条;
    struct IR指令* 上一条;
    struct 源位置 指令位置;
};
struct 静态变量信息;
struct 局部变量映射;
struct IR生成上下文;
struct IR生成上下文 {
    struct IR模块* 模块;
    struct IR函数* 当前函数;
    struct 基本块* 当前块;
    struct 基本块* 循环退出块;
    struct 基本块* 循环继续块;
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    struct 静态变量信息* 静态变量列表;
    struct 局部变量映射* 局部变量映射表;
};
struct 静态变量信息;
struct 静态变量信息 {
    char* 名称;
    long long 名称长度;
    struct 静态变量信息* 下一个;
};
struct 局部变量映射;
struct 局部变量映射 {
    char* 原始名称;
    char* 唯一名称;
    struct 局部变量映射* 下一个;
};
struct 输出缓冲区;
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
struct 模块代码生成上下文;
struct 模块代码生成上下文 {
    struct IR模块* 模块;
    struct 输出缓冲区* 头文件缓冲区;
    struct 输出缓冲区* 源文件缓冲区;
    struct 输出缓冲区* 前向声明缓冲区;
    struct 输出缓冲区* 结构体定义缓冲区;
    struct 输出缓冲区* 枚举定义缓冲区;
    struct 输出缓冲区* 全局变量缓冲区;
    struct 输出缓冲区* 函数声明缓冲区;
    struct 输出缓冲区* 函数定义缓冲区;
    struct 代码生成选项 选项;
    char* 模块名称;
    char* 输出文件名;
    long long 生成的结构体数量;
    long long 生成的枚举数量;
    long long 生成的全局变量数量;
    long long 生成的函数数量;
    long long 错误计数;
    long long 警告计数;
};
struct 输出缓冲区;
struct 输出缓冲区 {
    char* 数据;
    long long 容量;
    long long 长度;
    long long 缩进级别;
};
struct C代码生成上下文;
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
struct 模块元数据;
struct 模块加载器;
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
struct 模块缓存条目;
struct 模块缓存条目;
struct 模块缓存条目 {
    char* 模块名;
    struct 模块元数据* 元数据;
    struct 模块缓存条目* 下一个;
};
struct 模块标识符;
struct 导出符号;
struct 模块依赖;
struct 模块元数据;
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
    long long* 程序节点;
    struct 符号表管理器* 符号表;
};
struct 模块依赖;
struct 模块依赖 {
    struct 模块标识符* 模块标识;
    _Bool 是必需;
    _Bool 是循环;
};
struct 导出符号;
struct 导出符号 {
    char* 名称;
    long long 名称长度;
    _Bool 是函数;
    _Bool 是常量;
    void* 符号信息;
};
struct 模块标识符;
struct 模块标识符 {
    char* 完全限定名;
    long long 完全限定名长度;
    char** 路径段列表;
    long long 路径段数量;
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
long long 语义检查上下文大小(void);
_Bool 是常量表达式(struct 表达式节点*);
struct 类型信息* 检查三元表达式(struct 语义检查上下文*, struct 三元表达式*);
struct 类型信息* 检查赋值表达式(struct 语义检查上下文*, struct 赋值表达式*);
struct 类型信息* 检查数组访问表达式(struct 语义检查上下文*, struct 数组访问表达式*);
struct 类型信息* 检查成员访问表达式(struct 语义检查上下文*, struct 成员访问表达式*);
struct 类型信息* 检查函数调用表达式(struct 语义检查上下文*, struct 函数调用表达式*);
struct 类型信息* 检查标识符表达式(struct 语义检查上下文*, struct 标识符表达式*);
struct 类型信息* 检查字面量表达式(struct 语义检查上下文*, struct 字面量表达式*);
struct 类型信息* 检查一元表达式(struct 语义检查上下文*, struct 一元表达式*);
struct 类型信息* 检查二元表达式(struct 语义检查上下文*, struct 二元表达式*);
struct 类型信息* 检查表达式(struct 语义检查上下文*, struct 表达式节点*);
void 检查抛出语句(struct 语义检查上下文*, struct 抛出语句*);
void 检查捕获语句(struct 语义检查上下文*, struct 捕获语句*);
void 检查尝试语句(struct 语义检查上下文*, struct 尝试语句*);
void 检查情况语句(struct 语义检查上下文*, struct 情况语句*);
void 检查选择语句(struct 语义检查上下文*, struct 选择语句*);
void 检查继续语句(struct 语义检查上下文*, struct 继续语句*);
void 检查中断语句(struct 语义检查上下文*, struct 中断语句*);
void 检查返回语句(struct 语义检查上下文*, struct 返回语句*);
void 检查循环语句(struct 语义检查上下文*, struct 循环语句*);
void 检查当语句(struct 语义检查上下文*, struct 当语句*);
void 检查如果语句(struct 语义检查上下文*, struct 如果语句*);
void 检查块语句(struct 语义检查上下文*, struct 块语句*);
void 检查语句(struct 语义检查上下文*, struct 语句节点*);
void 检查接口声明(struct 语义检查上下文*, struct 接口声明*);
void 检查类成员(struct 语义检查上下文*, struct 类成员*);
void 检查类声明(struct 语义检查上下文*, struct 类声明*);
void 检查枚举声明(struct 语义检查上下文*, struct 枚举声明*);
void 检查结构体声明(struct 语义检查上下文*, struct 结构体声明*);
void 检查变量声明(struct 语义检查上下文*, struct 变量声明*);
void 检查参数(struct 语义检查上下文*, struct 参数*);
void 检查函数声明(struct 语义检查上下文*, struct 函数声明*);
void 检查声明(struct 语义检查上下文*, struct 声明节点*);
void 检查程序(struct 语义检查上下文*, struct 程序节点*);
_Bool 执行语义检查(struct 语义检查上下文*, struct 程序节点*);
void 销毁语义检查上下文(struct 语义检查上下文*);
struct 语义检查上下文* 创建语义检查上下文(struct 诊断集合*);
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
void 构建情况语句作用域(struct 作用域构建器*, struct 情况语句*);
void 构建返回语句作用域(struct 作用域构建器*, struct 返回语句*);
void 构建尝试语句作用域(struct 作用域构建器*, struct 尝试语句*);
void 构建捕获语句作用域(struct 作用域构建器*, struct 捕获语句*);
void 构建表达式作用域(struct 作用域构建器*, struct 表达式节点*);
void 构建二元表达式作用域(struct 作用域构建器*, struct 二元表达式*);
void 构建一元表达式作用域(struct 作用域构建器*, struct 一元表达式*);
void 构建函数调用表达式作用域(struct 作用域构建器*, struct 函数调用表达式*);
void 构建成员访问表达式作用域(struct 作用域构建器*, struct 成员访问表达式*);
void 构建数组访问表达式作用域(struct 作用域构建器*, struct 数组访问表达式*);
void 构建赋值表达式作用域(struct 作用域构建器*, struct 赋值表达式*);
void 构建三元表达式作用域(struct 作用域构建器*, struct 三元表达式*);
void 构建标识符表达式作用域(struct 作用域构建器*, struct 标识符表达式*);
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
struct 静态变量* 创建静态变量(char*, struct 类型节点*, struct IR操作数);
void 添加函数(struct IR模块*, struct IR函数*);
void 添加全局变量(struct IR模块*, struct 全局变量*);
struct 全局变量* 创建全局变量(char*, struct 类型节点*, struct IR操作数, _Bool);
struct IR操作数 标签操作数(struct 基本块*);
struct IR操作数 全局符号操作数(char*, struct 类型节点*);
struct IR操作数 字符串常量操作数(char*, struct 类型节点*);
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
struct 基本块* 创建基本块(char*);
long long 分配寄存器(struct IR函数*);
void 添加静态变量(struct IR函数*, struct 静态变量*);
void 添加基本块(struct IR函数*, struct 基本块*);
void 添加局部变量(struct IR函数*, struct IR操作数);
void 添加函数参数(struct IR函数*, struct IR操作数);
void 释放IR函数(struct IR函数*);
struct IR函数* 创建IR函数(char*, struct 类型节点*);
void 释放IR模块(struct IR模块*);
struct IR模块* 创建IR模块(void);
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_基本块计数器;
extern long long cn_var_局部变量计数器;
void 生成全局变量IR(struct IR生成上下文*, struct 声明节点*);
void 生成程序IR(struct IR生成上下文*, struct 程序节点*);
void 生成函数IR(struct IR生成上下文*, struct 声明节点*);
void 生成块语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成继续语句IR(struct IR生成上下文*);
void 生成中断语句IR(struct IR生成上下文*);
void 生成返回语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成循环IR(struct IR生成上下文*, struct 语句节点*);
void 生成当循环IR(struct IR生成上下文*, struct 语句节点*);
void 生成如果语句IR(struct IR生成上下文*, struct 语句节点*);
void 生成变量声明IR(struct IR生成上下文*, struct 语句节点*);
void 生成语句IR(struct IR生成上下文*, struct 语句节点*);
struct IR操作数 生成解引用IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成取地址IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成赋值IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成数组访问IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成成员访问IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成函数调用IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成一元运算IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成二元运算IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成标识符IR(struct IR生成上下文*, struct 表达式节点*);
struct IR操作数 生成表达式IR(struct IR生成上下文*, struct 表达式节点*);
enum IR操作码 二元运算符转IR(enum 二元运算符);
void 释放IR生成上下文(struct IR生成上下文*);
struct IR生成上下文* 创建IR生成上下文(void);
void 释放局部变量映射表(struct IR生成上下文*);
void 添加局部变量映射(struct IR生成上下文*, char*, char*);
char* 查找局部变量唯一名(struct IR生成上下文*, char*);
void 切换基本块(struct IR生成上下文*, struct 基本块*);
void 发射指令(struct IR生成上下文*, struct IR指令*);
long long 分配虚拟寄存器(struct IR生成上下文*);
char* 生成块名称(char*);
char* 生成唯一变量名(char*);
void 重置局部变量计数器(void);
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
long long 获取生成函数数量(struct 模块代码生成上下文*);
long long 获取生成全局变量数量(struct 模块代码生成上下文*);
long long 获取生成枚举数量(struct 模块代码生成上下文*);
long long 获取生成结构体数量(struct 模块代码生成上下文*);
_Bool 写入代码文件(struct 模块代码生成上下文*, char*);
char* 获取源文件内容(struct 模块代码生成上下文*);
char* 获取头文件内容(struct 模块代码生成上下文*);
void 生成模块代码(struct 模块代码生成上下文*);
void 释放模块代码生成上下文(struct 模块代码生成上下文*);
struct 模块代码生成上下文* 创建模块代码生成上下文(struct IR模块*, char*, struct 代码生成选项);
void 生成函数声明(struct IR函数*, struct 输出缓冲区*);
void 生成全局变量定义(struct 全局变量*, struct 输出缓冲区*);
void 生成全局变量声明(struct 全局变量*, struct 输出缓冲区*);
void 生成类前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成类定义(struct 类型信息*, struct 输出缓冲区*);
void 生成枚举前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成枚举定义(struct 类型信息*, struct 符号*, struct 输出缓冲区*);
void 生成结构体前向声明(struct 类型信息*, struct 输出缓冲区*);
void 生成结构体定义(struct 类型信息*, struct 输出缓冲区*);
void 生成源文件开头(struct 模块代码生成上下文*);
void 生成头文件结尾(struct 模块代码生成上下文*);
void 生成头文件开头(struct 模块代码生成上下文*);
char* 生成头文件保护宏(char*, struct 输出缓冲区*);
struct 输出缓冲区* 创建输出缓冲区(long long);
void 释放输出缓冲区(struct 输出缓冲区*);
_Bool 扩展缓冲区(struct 输出缓冲区*, long long);
_Bool 追加字符串(struct 输出缓冲区*, char*);
_Bool 追加字符(struct 输出缓冲区*, long long);
_Bool 追加整数(struct 输出缓冲区*, long long);
void 追加缩进(struct 输出缓冲区*, long long);
void 追加换行(struct 输出缓冲区*);
void 增加缩进(struct 输出缓冲区*);
void 减少缩进(struct 输出缓冲区*);
char* 类型到C类型(struct 类型信息*, struct 输出缓冲区*);
char* 生成类型声明(struct 类型信息*, char*, struct 输出缓冲区*);
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
struct 代码生成选项 获取默认生成选项(void);
void 格式化整数(char*, long long, long long);
void 追加小数(struct 输出缓冲区*, double);
extern long long cn_var_默认缓冲区大小;
extern long long cn_var_最大缩进级别;
extern long long cn_var_最大变量数量;
extern long long cn_var_最大字符串数量;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
extern long long cn_var_整数大小;
extern long long cn_var_小数大小;
extern long long cn_var_布尔大小;
extern long long cn_var_字符串大小;
extern long long cn_var_指针大小;
extern long long cn_var_最大依赖深度;
extern long long cn_var_最大模块缓存数量;
extern long long cn_var_最大搜索路径数量;
extern long long cn_var_最大模块路径长度;
long long 获取文件大小(void*);
long long 字符串指针大小(void);
long long 导出符号大小(void);
long long 模块依赖大小(void);
long long 模块缓存条目大小(void);
long long 模块元数据大小(void);
long long 模块标识符大小(void);
long long 模块加载器大小(void);
_Bool 收集导出符号(struct 模块元数据*);
_Bool 处理所有导入语句(struct 模块加载器*, struct 模块元数据*);
_Bool 处理导入语句(struct 模块加载器*, struct 导入语句节点*, struct 模块元数据*);
struct 模块元数据* 加载模块(struct 模块加载器*, char*);
struct 模块元数据* 加载模块核心(struct 模块加载器*, char*);
void 销毁模块元数据(struct 模块元数据*);
struct 模块元数据* 创建模块元数据(struct 模块标识符*);
_Bool 读取文件内容(char*, char**, long long*);
_Bool 添加模块到缓存(struct 模块加载器*, char*, struct 模块元数据*);
struct 模块元数据* 从缓存获取模块(struct 模块加载器*, char*);
long long 计算哈希值(char*);
_Bool 解析模块文件路径(struct 模块加载器*, struct 模块标识符*, char*, long long);
void 销毁模块标识符(struct 模块标识符*);
struct 模块标识符* 解析模块标识符(char*);
void 设置默认搜索路径(struct 模块加载器*, char*);
_Bool 添加搜索路径(struct 模块加载器*, char*);
void 销毁模块加载器(struct 模块加载器*);
struct 模块加载器* 创建模块加载器(struct 诊断集合*);
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
extern long long cn_var_最大错误恢复次数;
extern long long cn_var_解析器大小;
_Bool 初始化错误恢复(struct 语法错误恢复上下文*);
void 销毁错误恢复上下文(struct 语法错误恢复上下文*);
long long 是同步点(enum 词元类型枚举, enum 同步点类型);
long long 是任意同步点(enum 词元类型枚举);
long long 跳转到同步点(struct 语法错误恢复上下文*, enum 词元类型枚举, enum 同步点类型);
_Bool 应该抑制错误(struct 语法错误恢复上下文*);
void 记录错误(struct 语法错误恢复上下文*, long long);
void 重置错误计数(struct 语法错误恢复上下文*);
enum 语法恢复策略 选择恢复策略(long long);
char* 同步点类型名称(enum 同步点类型);
char* 恢复策略名称(enum 语法恢复策略);
extern long long cn_var_默认最大连续错误;
extern long long cn_var_默认最大恢复次数;
extern long long cn_var_语法错误恢复上下文大小;
struct 类型节点* 解析函数类型(struct 解析器*);
struct 类型节点* 解析数组类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析指针类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析基础类型(struct 解析器*);
struct 类型节点* 解析类型(struct 解析器*);
struct 表达式列表* 解析函数调用参数(struct 解析器*);
struct 表达式节点* 解析括号表达式(struct 解析器*);
struct 表达式节点* 解析标识符表达式(struct 解析器*);
struct 表达式节点* 解析字面量(struct 解析器*);
struct 表达式节点* 解析基础表达式(struct 解析器*);
struct 表达式节点* 解析后缀表达式(struct 解析器*);
struct 表达式节点* 解析一元表达式(struct 解析器*);
struct 表达式节点* 解析乘除表达式(struct 解析器*);
struct 表达式节点* 解析加减表达式(struct 解析器*);
struct 表达式节点* 解析位移表达式(struct 解析器*);
struct 表达式节点* 解析比较表达式(struct 解析器*);
struct 表达式节点* 解析相等表达式(struct 解析器*);
struct 表达式节点* 解析按位与表达式(struct 解析器*);
struct 表达式节点* 解析按位异或表达式(struct 解析器*);
struct 表达式节点* 解析按位或表达式(struct 解析器*);
struct 表达式节点* 解析逻辑与表达式(struct 解析器*);
struct 表达式节点* 解析逻辑或表达式(struct 解析器*);
struct 表达式节点* 解析三元表达式(struct 解析器*);
struct 表达式节点* 解析赋值表达式(struct 解析器*);
struct 表达式节点* 解析表达式(struct 解析器*);
struct 语句节点* 解析变量声明语句(struct 解析器*);
struct 语句节点* 解析表达式语句(struct 解析器*);
struct 语句节点* 解析抛出语句(struct 解析器*);
struct 语句节点* 解析尝试语句(struct 解析器*);
void* 解析情况列表(struct 解析器*);
struct 语句节点* 解析选择语句(struct 解析器*);
struct 语句节点* 解析继续语句(struct 解析器*);
struct 语句节点* 解析中断语句(struct 解析器*);
struct 语句节点* 解析返回语句(struct 解析器*);
struct 语句节点* 解析循环语句(struct 解析器*);
struct 语句节点* 解析当语句(struct 解析器*);
struct 语句节点* 解析如果语句(struct 解析器*);
struct 块语句节点* 解析块语句(struct 解析器*);
struct 语句节点* 解析语句(struct 解析器*);
enum 可见性 解析可见性修饰符(struct 解析器*);
struct 声明节点* 解析导入声明(struct 解析器*);
struct 声明节点* 解析模块声明(struct 解析器*);
struct 声明节点* 解析接口声明(struct 解析器*);
struct 声明节点* 解析类声明(struct 解析器*);
struct 声明节点* 解析枚举声明(struct 解析器*);
struct 声明节点* 解析结构体声明(struct 解析器*);
struct 声明节点* 解析变量声明(struct 解析器*);
struct 参数节点* 解析参数(struct 解析器*);
struct 参数列表* 解析参数列表(struct 解析器*);
struct 声明节点* 解析函数声明(struct 解析器*);
struct 声明节点* 解析顶层声明(struct 解析器*);
struct 解析器* 创建解析器(struct 扫描器*, struct 诊断集合*);
void 销毁解析器(struct 解析器*);
struct 程序节点* 解析程序(struct 解析器*);
void 前进词元(struct 解析器*);
_Bool 期望(struct 解析器*, enum 词元类型枚举);
_Bool 检查(struct 解析器*, enum 词元类型枚举);
_Bool 匹配(struct 解析器*, enum 词元类型枚举);
_Bool 匹配多种(struct 解析器*, enum 词元类型枚举*, long long);
void 报告错误期望(struct 解析器*, enum 词元类型枚举);
_Bool 同步恢复(struct 解析器*);
_Bool 是否类型关键字(enum 词元类型枚举);
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
void 初始化关键字表(void);
struct 关键字条目* 获取关键字表(void);
long long 关键字总数(void);
_Bool 是关键字字符串(char*);
enum 词元类型枚举 查找关键字(char*);
_Bool 是块注释结束(long long, long long);
_Bool 是块注释开始(long long, long long);
_Bool 是单行注释开始(long long, long long);
_Bool 是八进制前缀(long long, long long);
_Bool 是二进制前缀(long long, long long);
_Bool 是十六进制前缀(long long, long long);
_Bool 是可打印字符(long long);
long long 转大写(long long);
long long 转小写(long long);
_Bool 是分隔符字符(long long);
_Bool 是运算符字符(long long);
_Bool 是标识符字符(long long);
_Bool 是标识符开头(long long);
_Bool 是中文字符(long long);
_Bool 是字母或数字(long long);
_Bool 是字母(long long);
_Bool 是八进制数字(long long);
_Bool 是二进制数字(long long);
_Bool 是十六进制数字(long long);
_Bool 是数字(long long);
_Bool 是换行符(long long);
_Bool 是空白字符(long long);
struct 扫描器* 创建扫描器(char*, long long);
void 销毁扫描器(struct 扫描器*);
struct 词元 下一个词元(struct 扫描器*);
struct 词元 预览词元(struct 扫描器*);
long long 当前字符(struct 扫描器*);
long long 预览字符(struct 扫描器*, long long);
long long 前进(struct 扫描器*);
_Bool 是否结束(struct 扫描器*);
void 跳过空白字符(struct 扫描器*);
void 跳过单行注释(struct 扫描器*);
_Bool 跳过块注释(struct 扫描器*);
struct 词元 扫描标识符(struct 扫描器*);
struct 词元 扫描数字(struct 扫描器*);
struct 词元 扫描字符串(struct 扫描器*);
struct 词元 扫描字符(struct 扫描器*);
struct 词元 扫描运算符(struct 扫描器*);
extern long long cn_var_最大标识符长度;
extern long long cn_var_最大字符串长度;
extern long long cn_var_最大数字长度;
extern long long cn_var_扫描器大小;

// CN Language Enum Definitions
enum 编译阶段 {
    编译阶段_阶段_词法分析,
    编译阶段_阶段_语法分析,
    编译阶段_阶段_语义分析,
    编译阶段_阶段_IR生成,
    编译阶段_阶段_代码生成
};

enum 输出格式 {
    输出格式_格式_C代码,
    输出格式_格式_IR,
    输出格式_格式_AST,
    输出格式_格式_目标代码
};

enum 优化级别 {
    优化级别_优化_无 = 0,
    优化级别_优化_基础 = 1,
    优化级别_优化_标准 = 2,
    优化级别_优化_激进 = 3
};

// CN Language Global Struct Forward Declarations
struct 编译选项;
struct 编译上下文;

// CN Language Global Struct Definitions
struct 编译选项 {
    char* 输入文件路径;
    char* 输出文件路径;
    char* 输出目录;
    enum 编译阶段 停止阶段;
    enum 输出格式 输出格式;
    enum 优化级别 优化级别;
    _Bool 显示版本;
    _Bool 显示帮助;
    _Bool 详细输出;
    _Bool 仅语法检查;
    _Bool 生成调试信息;
    _Bool 启用警告;
    _Bool 严格模式;
    char** 搜索路径列表;
    long long 搜索路径数量;
};

struct 编译上下文 {
    struct 编译选项 选项;
    struct 诊断集合* 诊断集合;
    struct 模块加载器* 模块加载器;
    struct 扫描器* 扫描器实例;
    struct 解析器* 解析器实例;
    struct 程序节点* 程序节点;
    struct 语义检查上下文* 语义上下文;
    struct IR模块* IR模块;
    struct 模块代码生成上下文* 代码生成上下文;
    long long 词法错误数;
    long long 语法错误数;
    long long 语义错误数;
    long long 总错误数;
    long long 总警告数;
    _Bool 编译成功;
};

// Global Variables
char* cn_var_编译器版本 = "0.1.0";
char* cn_var_编译器名称 = "CN语言自托管编译器";
long long cn_var_最大参数数量 = 64;
long long cn_var_最大文件路径长度 = 1024;
long long cn_var_最大输出路径长度 = 1024;

// Forward Declarations
void 初始化编译选项(struct 编译选项*);
_Bool 解析命令行参数(long long, char**, struct 编译选项*);
void 显示版本信息();
void 显示帮助信息();
struct 编译上下文* 创建编译上下文(struct 编译选项*);
void 销毁编译上下文(struct 编译上下文*);
_Bool 执行词法分析(struct 编译上下文*, char*, long long);
_Bool 执行语法分析(struct 编译上下文*);
_Bool 执行语义分析(struct 编译上下文*);
_Bool 执行IR生成(struct 编译上下文*);
_Bool 执行代码生成(struct 编译上下文*);
_Bool 编译文件(struct 编译上下文*);
_Bool 读取文件内容到缓冲区(char*, char**, long long*);
long long 编译上下文大小();
long long 编译选项大小();
long long main(long long, char**);

void 初始化编译选项(struct 编译选项* cn_var_选项) {
  struct 编译选项* r0;
  _Bool r1;

  entry:
  r0 = cn_var_选项;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return;
  goto if_merge_1;

  if_merge_1:
  return;
}

_Bool 解析命令行参数(long long cn_var_参数数量, char** cn_var_参数数组, struct 编译选项* cn_var_选项) {
  long long r0, r6, r7, r10, r14, r16, r19, r21, r23, r26, r29, r31, r32, r33, r36, r37, r39, r40, r42, r44, r45, r46, r47, r50, r53, r54, r59, r61, r62, r63, r64, r66, r69, r72, r75, r84, r85;
  char* r11;
  char* r12;
  char* r15;
  char* r18;
  char* r22;
  char* r25;
  char* r28;
  char* r38;
  char* r41;
  char* r55;
  char* r65;
  char* r68;
  char* r71;
  char* r74;
  char* r77;
  char* r83;
  void* r57;
  char** r1;
  struct 编译选项* r3;
  struct 编译选项* r5;
  char** r9;
  char** r35;
  struct 编译选项* r49;
  char** r52;
  struct 编译选项* r56;
  struct 编译选项* r58;
  struct 编译选项* r60;
  struct 编译选项* r80;
  char* r81;
  _Bool r2;
  _Bool r4;
  _Bool r8;
  _Bool r13;
  _Bool r17;
  _Bool r20;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r34;
  _Bool r43;
  _Bool r48;
  _Bool r51;
  _Bool r67;
  _Bool r70;
  _Bool r73;
  _Bool r76;
  _Bool r79;
  _Bool r82;
  char r78;

  entry:
  r1 = cn_var_参数数组;
  r2 = r1 == 0;
  if (r2) goto logic_merge_5; else goto logic_rhs_4;

  if_then_2:
  return 0;
  goto if_merge_3;

  if_merge_3:
  r5 = cn_var_选项;
  初始化编译选项(r5);
  long long cn_var_i_0;
  cn_var_i_0 = 1;
  goto for_cond_6;

  logic_rhs_4:
  r3 = cn_var_选项;
  r4 = r3 == 0;
  goto logic_merge_5;

  logic_merge_5:
  if (r4) goto if_then_2; else goto if_merge_3;

  for_cond_6:
  r6 = cn_var_i_0;
  r7 = cn_var_参数数量;
  r8 = r6 < r7;
  if (r8) goto for_body_7; else goto for_exit_9;

  for_body_7:
  char* cn_var_参数_1;
  r9 = cn_var_参数数组;
  r10 = cn_var_i_0;
  r11 = *(char**)cn_rt_array_get_element(r9, r10, 8);
  cn_var_参数_1 = r11;
  r12 = cn_var_参数_1;
  r13 = r12 == 0;
  if (r13) goto if_then_10; else goto if_merge_11;

  for_update_8:
  r84 = cn_var_i_0;
  r85 = r84 + 1;
  cn_var_i_0 = r85;
  goto for_cond_6;

  for_exit_9:
  return 1;

  if_then_10:
  goto for_update_8;
  goto if_merge_11;

  if_merge_11:
  r15 = cn_var_参数_1;
  r16 = 比较字符串(r15, "--version");
  r17 = r16 == 0;
  if (r17) goto logic_merge_15; else goto logic_rhs_14;

  if_then_12:
  return 1;
  goto if_merge_13;

  if_merge_13:
  r22 = cn_var_参数_1;
  r23 = 比较字符串(r22, "--help");
  r24 = r23 == 0;
  if (r24) goto logic_merge_19; else goto logic_rhs_18;

  logic_rhs_14:
  r18 = cn_var_参数_1;
  r19 = 比较字符串(r18, "-v");
  r20 = r19 == 0;
  goto logic_merge_15;

  logic_merge_15:
  if (r20) goto if_then_12; else goto if_merge_13;

  if_then_16:
  return 1;
  goto if_merge_17;

  if_merge_17:
  r28 = cn_var_参数_1;
  r29 = 比较字符串(r28, "-o");
  r30 = r29 == 0;
  if (r30) goto if_then_20; else goto if_merge_21;

  logic_rhs_18:
  r25 = cn_var_参数_1;
  r26 = 比较字符串(r25, "-h");
  r27 = r26 == 0;
  goto logic_merge_19;

  logic_merge_19:
  if (r27) goto if_then_16; else goto if_merge_17;

  if_then_20:
  r31 = cn_var_i_0;
  r32 = r31 + 1;
  r33 = cn_var_参数数量;
  r34 = r32 < r33;
  if (r34) goto if_then_22; else goto if_merge_23;

  if_merge_21:
  r41 = cn_var_参数_1;
  r42 = 比较字符串(r41, "-I");
  r43 = r42 == 0;
  if (r43) goto if_then_24; else goto if_merge_25;

  if_then_22:
  r35 = cn_var_参数数组;
  r36 = cn_var_i_0;
  r37 = r36 + 1;
  r38 = *(char**)cn_rt_array_get_element(r35, r37, 8);
  r39 = cn_var_i_0;
  r40 = r39 + 1;
  cn_var_i_0 = r40;
  goto if_merge_23;

  if_merge_23:
  goto for_update_8;
  goto if_merge_21;

  if_then_24:
  r45 = cn_var_i_0;
  r46 = r45 + 1;
  r47 = cn_var_参数数量;
  r48 = r46 < r47;
  if (r48) goto logic_rhs_28; else goto logic_merge_29;

  if_merge_25:
  r65 = cn_var_参数_1;
  r66 = 比较字符串(r65, "--verbose");
  r67 = r66 == 0;
  if (r67) goto if_then_30; else goto if_merge_31;

  if_then_26:
  r52 = cn_var_参数数组;
  r53 = cn_var_i_0;
  r54 = r53 + 1;
  r55 = *(char**)cn_rt_array_get_element(r52, r54, 8);
  r56 = cn_var_选项;
  r57 = r56->搜索路径列表;
  r58 = cn_var_选项;
  r59 = r58->搜索路径数量;
    { char* _tmp_r0 = r55; cn_rt_array_set_element(r57, r59, &_tmp_r0, 8); }
  r60 = cn_var_选项;
  r61 = r60->搜索路径数量;
  r62 = r61 + 1;
  r63 = cn_var_i_0;
  r64 = r63 + 1;
  cn_var_i_0 = r64;
  goto if_merge_27;

  if_merge_27:
  goto for_update_8;
  goto if_merge_25;

  logic_rhs_28:
  r49 = cn_var_选项;
  r50 = r49->搜索路径数量;
  r51 = r50 < 64;
  goto logic_merge_29;

  logic_merge_29:
  if (r51) goto if_then_26; else goto if_merge_27;

  if_then_30:
  goto for_update_8;
  goto if_merge_31;

  if_merge_31:
  r68 = cn_var_参数_1;
  r69 = 比较字符串(r68, "--syntax-only");
  r70 = r69 == 0;
  if (r70) goto if_then_32; else goto if_merge_33;

  if_then_32:
  goto for_update_8;
  goto if_merge_33;

  if_merge_33:
  r71 = cn_var_参数_1;
  r72 = 比较字符串(r71, "-O0");
  r73 = r72 == 0;
  if (r73) goto if_then_34; else goto if_merge_35;

  if_then_34:
  goto for_update_8;
  goto if_merge_35;

  if_merge_35:
  r74 = cn_var_参数_1;
  r75 = 比较字符串(r74, "-O2");
  r76 = r75 == 0;
  if (r76) goto if_then_36; else goto if_merge_37;

  if_then_36:
  goto for_update_8;
  goto if_merge_37;

  if_merge_37:
  r77 = cn_var_参数_1;
  r78 = *(char*)cn_rt_array_get_element(r77, 0, 8);
  r79 = r78 != 45;
  if (r79) goto if_then_38; else goto if_merge_39;

  if_then_38:
  r80 = cn_var_选项;
  r81 = r80->输入文件路径;
  r82 = r81 == 0;
  if (r82) goto if_then_40; else goto if_merge_41;

  if_merge_39:
  goto for_update_8;

  if_then_40:
  r83 = cn_var_参数_1;
  goto if_merge_41;

  if_merge_41:
  goto for_update_8;
  goto if_merge_39;
  return 0;
}

void 显示版本信息() {
  long long r2, r3;
  char* r0;
  char* r1;

  entry:
  r0 = cn_var_编译器名称;
  r1 = cn_var_编译器版本;
  r2 = 打印格式("%s 版本 %s\n", r0, r1);
  r3 = 打印格式("版权所有 (C) 2026 CN语言项目\n");
  return;
}

void 显示帮助信息() {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r0 = 打印格式("用法: cnc [选项] <输入文件>\n");
  r1 = 打印格式("\n选项:\n");
  r2 = 打印格式("  -h, --help          显示帮助信息\n");
  r3 = 打印格式("  -v, --version       显示版本信息\n");
  r4 = 打印格式("  -o <文件>           指定输出文件\n");
  r5 = 打印格式("  -I <路径>           添加模块搜索路径\n");
  r6 = 打印格式("  --verbose           详细输出模式\n");
  r7 = 打印格式("  --syntax-only       仅进行语法检查\n");
  r8 = 打印格式("  -O0, -O2            优化级别\n");
  return;
}

struct 编译上下文* 创建编译上下文(struct 编译选项* cn_var_选项) {
  long long r2, r12, r28, r30, r36, r39, r40;
  char* r37;
  void* r35;
  struct 编译选项* r0;
  void* r3;
  struct 编译上下文* r4;
  struct 编译上下文* r6;
  struct 编译选项* r8;
  struct 编译上下文* r9;
  struct 编译选项* r11;
  void* r13;
  struct 诊断集合* r14;
  struct 编译上下文* r15;
  struct 诊断集合* r16;
  struct 编译上下文* r18;
  struct 编译上下文* r19;
  struct 诊断集合* r20;
  struct 模块加载器* r21;
  struct 编译上下文* r22;
  struct 模块加载器* r23;
  struct 编译上下文* r25;
  struct 诊断集合* r26;
  struct 编译上下文* r27;
  struct 编译选项* r29;
  struct 编译上下文* r32;
  struct 模块加载器* r33;
  struct 编译选项* r34;
  struct 编译上下文* r41;
  _Bool r1;
  _Bool r5;
  _Bool r17;
  _Bool r24;
  _Bool r31;
  _Bool r38;
  struct 编译选项 r7;
  struct 编译选项 r10;

  entry:
  r0 = cn_var_选项;
  r1 = r0 == 0;
  if (r1) goto if_then_42; else goto if_merge_43;

  if_then_42:
  return 0;
  goto if_merge_43;

  if_merge_43:
  struct 编译上下文* cn_var_上下文_0;
  r2 = 编译上下文大小();
  r3 = 分配清零内存(1, r2);
  cn_var_上下文_0 = r3;
  r4 = cn_var_上下文_0;
  r5 = r4 == 0;
  if (r5) goto if_then_44; else goto if_merge_45;

  if_then_44:
  return 0;
  goto if_merge_45;

  if_merge_45:
  r6 = cn_var_上下文_0;
  r7 = r6->选项;
  r9 = cn_var_上下文_0;
  r10 = r9->选项;
  r8 = &r10;
  r11 = cn_var_选项;
  r12 = 编译选项大小();
  r13 = 复制内存(r8, r11, r12);
  r14 = 创建诊断集合(16);
  r15 = cn_var_上下文_0;
  r16 = r15->诊断集合;
  r17 = r16 == 0;
  if (r17) goto if_then_46; else goto if_merge_47;

  if_then_46:
  r18 = cn_var_上下文_0;
  释放内存(r18);
  return 0;
  goto if_merge_47;

  if_merge_47:
  r19 = cn_var_上下文_0;
  r20 = r19->诊断集合;
  r21 = 创建模块加载器(r20);
  r22 = cn_var_上下文_0;
  r23 = r22->模块加载器;
  r24 = r23 == 0;
  if (r24) goto if_then_48; else goto if_merge_49;

  if_then_48:
  r25 = cn_var_上下文_0;
  r26 = r25->诊断集合;
  释放诊断集合(r26);
  r27 = cn_var_上下文_0;
  释放内存(r27);
  return 0;
  goto if_merge_49;

  if_merge_49:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_50;

  for_cond_50:
  r28 = cn_var_i_1;
  r29 = cn_var_选项;
  r30 = r29->搜索路径数量;
  r31 = r28 < r30;
  if (r31) goto for_body_51; else goto for_exit_53;

  for_body_51:
  r32 = cn_var_上下文_0;
  r33 = r32->模块加载器;
  r34 = cn_var_选项;
  r35 = r34->搜索路径列表;
  r36 = cn_var_i_1;
  r37 = *(char**)cn_rt_array_get_element(r35, r36, 8);
  r38 = 添加搜索路径(r33, r37);
  goto for_update_52;

  for_update_52:
  r39 = cn_var_i_1;
  r40 = r39 + 1;
  cn_var_i_1 = r40;
  goto for_cond_50;

  for_exit_53:
  r41 = cn_var_上下文_0;
  return r41;
}

void 销毁编译上下文(struct 编译上下文* cn_var_上下文) {
  struct 编译上下文* r0;
  struct 编译上下文* r2;
  struct 解析器* r3;
  struct 编译上下文* r5;
  struct 解析器* r6;
  struct 编译上下文* r7;
  struct 扫描器* r8;
  struct 编译上下文* r10;
  struct 扫描器* r11;
  struct 编译上下文* r12;
  struct 模块加载器* r13;
  struct 编译上下文* r15;
  struct 模块加载器* r16;
  struct 编译上下文* r17;
  struct 诊断集合* r18;
  struct 编译上下文* r20;
  struct 诊断集合* r21;
  struct 编译上下文* r22;
  _Bool r1;
  _Bool r4;
  _Bool r9;
  _Bool r14;
  _Bool r19;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_54; else goto if_merge_55;

  if_then_54:
  return;
  goto if_merge_55;

  if_merge_55:
  r2 = cn_var_上下文;
  r3 = r2->解析器实例;
  r4 = r3 != 0;
  if (r4) goto if_then_56; else goto if_merge_57;

  if_then_56:
  r5 = cn_var_上下文;
  r6 = r5->解析器实例;
  销毁解析器(r6);
  goto if_merge_57;

  if_merge_57:
  r7 = cn_var_上下文;
  r8 = r7->扫描器实例;
  r9 = r8 != 0;
  if (r9) goto if_then_58; else goto if_merge_59;

  if_then_58:
  r10 = cn_var_上下文;
  r11 = r10->扫描器实例;
  销毁扫描器(r11);
  goto if_merge_59;

  if_merge_59:
  r12 = cn_var_上下文;
  r13 = r12->模块加载器;
  r14 = r13 != 0;
  if (r14) goto if_then_60; else goto if_merge_61;

  if_then_60:
  r15 = cn_var_上下文;
  r16 = r15->模块加载器;
  销毁模块加载器(r16);
  goto if_merge_61;

  if_merge_61:
  r17 = cn_var_上下文;
  r18 = r17->诊断集合;
  r19 = r18 != 0;
  if (r19) goto if_then_62; else goto if_merge_63;

  if_then_62:
  r20 = cn_var_上下文;
  r21 = r20->诊断集合;
  释放诊断集合(r21);
  goto if_merge_63;

  if_merge_63:
  r22 = cn_var_上下文;
  释放内存(r22);
  return;
}

_Bool 执行词法分析(struct 编译上下文* cn_var_上下文, char* cn_var_源码, long long cn_var_源码长度) {
  long long r0, r8, r10, r15;
  char* r3;
  char* r9;
  struct 编译上下文* r1;
  struct 编译上下文* r5;
  struct 扫描器* r11;
  struct 编译上下文* r12;
  struct 扫描器* r13;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r14;
  struct 编译选项 r6;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_67; else goto logic_rhs_66;

  if_then_64:
  return 0;
  goto if_merge_65;

  if_merge_65:
  r5 = cn_var_上下文;
  r6 = r5->选项;
  r7 = r6.详细输出;
  if (r7) goto if_then_68; else goto if_merge_69;

  logic_rhs_66:
  r3 = cn_var_源码;
  r4 = r3 == 0;
  goto logic_merge_67;

  logic_merge_67:
  if (r4) goto if_then_64; else goto if_merge_65;

  if_then_68:
  r8 = 打印格式("正在执行词法分析...\n");
  goto if_merge_69;

  if_merge_69:
  r9 = cn_var_源码;
  r10 = cn_var_源码长度;
  r11 = 创建扫描器(r9, r10);
  r12 = cn_var_上下文;
  r13 = r12->扫描器实例;
  r14 = r13 == 0;
  if (r14) goto if_then_70; else goto if_merge_71;

  if_then_70:
  r15 = 打印格式("错误: 无法创建词法分析器\n");
  return 0;
  goto if_merge_71;

  if_merge_71:
  return 1;
}

_Bool 执行语法分析(struct 编译上下文* cn_var_上下文) {
  long long r0, r9, r18, r25;
  struct 编译上下文* r1;
  struct 编译上下文* r3;
  struct 扫描器* r4;
  struct 编译上下文* r6;
  struct 编译上下文* r10;
  struct 扫描器* r11;
  struct 编译上下文* r12;
  struct 诊断集合* r13;
  struct 解析器* r14;
  struct 编译上下文* r15;
  struct 解析器* r16;
  struct 编译上下文* r19;
  struct 解析器* r20;
  struct 程序节点* r21;
  struct 编译上下文* r22;
  struct 程序节点* r23;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r17;
  _Bool r24;
  struct 编译选项 r7;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_75; else goto logic_rhs_74;

  if_then_72:
  return 0;
  goto if_merge_73;

  if_merge_73:
  r6 = cn_var_上下文;
  r7 = r6->选项;
  r8 = r7.详细输出;
  if (r8) goto if_then_76; else goto if_merge_77;

  logic_rhs_74:
  r3 = cn_var_上下文;
  r4 = r3->扫描器实例;
  r5 = r4 == 0;
  goto logic_merge_75;

  logic_merge_75:
  if (r5) goto if_then_72; else goto if_merge_73;

  if_then_76:
  r9 = 打印格式("正在执行语法分析...\n");
  goto if_merge_77;

  if_merge_77:
  r10 = cn_var_上下文;
  r11 = r10->扫描器实例;
  r12 = cn_var_上下文;
  r13 = r12->诊断集合;
  r14 = 创建解析器(r11, r13);
  r15 = cn_var_上下文;
  r16 = r15->解析器实例;
  r17 = r16 == 0;
  if (r17) goto if_then_78; else goto if_merge_79;

  if_then_78:
  r18 = 打印格式("错误: 无法创建语法分析器\n");
  return 0;
  goto if_merge_79;

  if_merge_79:
  r19 = cn_var_上下文;
  r20 = r19->解析器实例;
  r21 = 解析程序(r20);
  r22 = cn_var_上下文;
  r23 = r22->程序节点;
  r24 = r23 == 0;
  if (r24) goto if_then_80; else goto if_merge_81;

  if_then_80:
  r25 = 打印格式("错误: 语法分析失败\n");
  return 0;
  goto if_merge_81;

  if_merge_81:
  return 1;
}

_Bool 执行语义分析(struct 编译上下文* cn_var_上下文) {
  long long r0, r9, r16, r24;
  struct 编译上下文* r1;
  struct 编译上下文* r3;
  struct 程序节点* r4;
  struct 编译上下文* r6;
  struct 编译上下文* r10;
  struct 诊断集合* r11;
  struct 语义检查上下文* r12;
  struct 编译上下文* r13;
  struct 语义检查上下文* r14;
  struct 编译上下文* r17;
  struct 语义检查上下文* r18;
  struct 编译上下文* r19;
  struct 程序节点* r20;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r15;
  _Bool r21;
  _Bool r22;
  _Bool r23;
  struct 编译选项 r7;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_85; else goto logic_rhs_84;

  if_then_82:
  return 0;
  goto if_merge_83;

  if_merge_83:
  r6 = cn_var_上下文;
  r7 = r6->选项;
  r8 = r7.详细输出;
  if (r8) goto if_then_86; else goto if_merge_87;

  logic_rhs_84:
  r3 = cn_var_上下文;
  r4 = r3->程序节点;
  r5 = r4 == 0;
  goto logic_merge_85;

  logic_merge_85:
  if (r5) goto if_then_82; else goto if_merge_83;

  if_then_86:
  r9 = 打印格式("正在执行语义分析...\n");
  goto if_merge_87;

  if_merge_87:
  r10 = cn_var_上下文;
  r11 = r10->诊断集合;
  r12 = 创建语义检查上下文(r11);
  r13 = cn_var_上下文;
  r14 = r13->语义上下文;
  r15 = r14 == 0;
  if (r15) goto if_then_88; else goto if_merge_89;

  if_then_88:
  r16 = 打印格式("错误: 无法创建语义分析器\n");
  return 0;
  goto if_merge_89;

  if_merge_89:
  _Bool cn_var_检查结果_0;
  r17 = cn_var_上下文;
  r18 = r17->语义上下文;
  r19 = cn_var_上下文;
  r20 = r19->程序节点;
  r21 = 执行语义检查(r18, r20);
  cn_var_检查结果_0 = r21;
  r22 = cn_var_检查结果_0;
  r23 = !r22;
  if (r23) goto if_then_90; else goto if_merge_91;

  if_then_90:
  r24 = 打印格式("错误: 语义分析失败\n");
  return 0;
  goto if_merge_91;

  if_merge_91:
  return 1;
}

_Bool 执行IR生成(struct 编译上下文* cn_var_上下文) {
  long long r0, r9, r14, r17, r33;
  struct 编译上下文* r1;
  struct 编译上下文* r3;
  struct 程序节点* r4;
  struct 编译上下文* r6;
  struct IR模块* r10;
  struct 编译上下文* r11;
  struct IR模块* r12;
  struct IR生成上下文* r16;
  void* r18;
  struct 编译上下文* r19;
  struct IR模块* r20;
  struct 编译上下文* r21;
  struct 语义检查上下文* r22;
  struct 符号表管理器* r23;
  struct 作用域* r24;
  struct IR生成上下文* r26;
  struct 编译上下文* r27;
  struct 程序节点* r28;
  struct 编译上下文* r29;
  struct IR模块* r30;
  struct IR函数* r31;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r13;
  _Bool r32;
  struct 编译选项 r7;
  struct IR生成上下文 r15;
  struct IR生成上下文 r25;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_95; else goto logic_rhs_94;

  if_then_92:
  return 0;
  goto if_merge_93;

  if_merge_93:
  r6 = cn_var_上下文;
  r7 = r6->选项;
  r8 = r7.详细输出;
  if (r8) goto if_then_96; else goto if_merge_97;

  logic_rhs_94:
  r3 = cn_var_上下文;
  r4 = r3->程序节点;
  r5 = r4 == 0;
  goto logic_merge_95;

  logic_merge_95:
  if (r5) goto if_then_92; else goto if_merge_93;

  if_then_96:
  r9 = 打印格式("正在生成中间表示(IR)...\n");
  goto if_merge_97;

  if_merge_97:
  r10 = 创建IR模块();
  r11 = cn_var_上下文;
  r12 = r11->IR模块;
  r13 = r12 == 0;
  if (r13) goto if_then_98; else goto if_merge_99;

  if_then_98:
  r14 = 打印格式("错误: 无法创建IR模块\n");
  return 0;
  goto if_merge_99;

  if_merge_99:
  struct IR生成上下文 cn_var_IR上下文_0;
  r15 = cn_var_IR上下文_0;
  r16 = &cn_var_IR上下文_0;
  r17 = sizeof(struct IR生成上下文);
  r18 = 设置内存(r16, 0, r17);
  r19 = cn_var_上下文;
  r20 = r19->IR模块;
  r21 = cn_var_上下文;
  r22 = r21->语义上下文;
  r23 = r22->符号表;
  r24 = r23->全局作用域;
  r25 = cn_var_IR上下文_0;
  r26 = &cn_var_IR上下文_0;
  r27 = cn_var_上下文;
  r28 = r27->程序节点;
  生成程序IR(r26, r28);
  r29 = cn_var_上下文;
  r30 = r29->IR模块;
  r31 = r30->首个函数;
  r32 = r31 == 0;
  if (r32) goto if_then_100; else goto if_merge_101;

  if_then_100:
  r33 = 打印格式("警告: IR模块没有生成任何函数\n");
  goto if_merge_101;

  if_merge_101:
  return 1;
}

_Bool 执行代码生成(struct 编译上下文* cn_var_上下文) {
  long long r0, r9, r29, r30, r31, r32, r35, r39, r40, r41, r48, r61, r68;
  char* r18;
  char* r27;
  char* r43;
  char* r45;
  void* r14;
  void* r23;
  void* r28;
  void* r34;
  void* r38;
  void* r42;
  void* r44;
  void* r55;
  void* r67;
  struct 编译上下文* r1;
  struct 编译上下文* r3;
  struct IR模块* r4;
  struct 编译上下文* r6;
  struct 编译上下文* r10;
  char* r12;
  struct 编译上下文* r15;
  char* r17;
  struct 编译上下文* r19;
  char* r21;
  struct 编译上下文* r24;
  char* r26;
  struct 代码生成选项* r47;
  void* r49;
  struct 编译上下文* r50;
  struct 编译上下文* r53;
  struct IR模块* r54;
  struct 模块代码生成上下文* r57;
  struct 编译上下文* r58;
  struct 模块代码生成上下文* r59;
  struct 编译上下文* r62;
  struct 模块代码生成上下文* r63;
  struct 编译上下文* r64;
  _Bool r2;
  _Bool r5;
  _Bool r8;
  _Bool r13;
  _Bool r22;
  _Bool r33;
  _Bool r37;
  _Bool r52;
  _Bool r60;
  _Bool r66;
  char r36;
  struct 编译选项 r7;
  struct 编译选项 r11;
  struct 编译选项 r16;
  struct 编译选项 r20;
  struct 编译选项 r25;
  struct 代码生成选项 r46;
  struct 编译选项 r51;
  struct 代码生成选项 r56;
  struct 编译选项 r65;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_105; else goto logic_rhs_104;

  if_then_102:
  return 0;
  goto if_merge_103;

  if_merge_103:
  r6 = cn_var_上下文;
  r7 = r6->选项;
  r8 = r7.详细输出;
  if (r8) goto if_then_106; else goto if_merge_107;

  logic_rhs_104:
  r3 = cn_var_上下文;
  r4 = r3->IR模块;
  r5 = r4 == 0;
  goto logic_merge_105;

  logic_merge_105:
  if (r5) goto if_then_102; else goto if_merge_103;

  if_then_106:
  r9 = 打印格式("正在生成代码...\n");
  goto if_merge_107;

  if_merge_107:
  char* cn_var_输出路径;
  cn_var_输出路径 = cn_rt_array_alloc(8, 1024);
  r10 = cn_var_上下文;
  r11 = r10->选项;
  r12 = r11.输出文件路径;
  r13 = r12 != 0;
  if (r13) goto if_then_108; else goto if_else_109;

  if_then_108:
  r14 = cn_var_输出路径;
  r15 = cn_var_上下文;
  r16 = r15->选项;
  r17 = r16.输出文件路径;
  r18 = 复制字符串(r14, r17);
  goto if_merge_110;

  if_else_109:
  r19 = cn_var_上下文;
  r20 = r19->选项;
  r21 = r20.输入文件路径;
  r22 = r21 != 0;
  if (r22) goto if_then_111; else goto if_else_112;

  if_merge_110:
  struct 代码生成选项 cn_var_生成选项_2;
  r46 = cn_var_生成选项_2;
  r47 = &cn_var_生成选项_2;
  r48 = sizeof(struct 代码生成选项);
  r49 = 设置内存(r47, 0, r48);
  r50 = cn_var_上下文;
  r51 = r50->选项;
  r52 = r51.详细输出;
  r53 = cn_var_上下文;
  r54 = r53->IR模块;
  r55 = cn_var_输出路径;
  r56 = cn_var_生成选项_2;
  r57 = 创建模块代码生成上下文(r54, r55, r56);
  r58 = cn_var_上下文;
  r59 = r58->代码生成上下文;
  r60 = r59 == 0;
  if (r60) goto if_then_120; else goto if_merge_121;

  if_then_111:
  r23 = cn_var_输出路径;
  r24 = cn_var_上下文;
  r25 = r24->选项;
  r26 = r25.输入文件路径;
  r27 = 复制字符串(r23, r26);
  long long cn_var_长度_0;
  r28 = cn_var_输出路径;
  r29 = 获取字符串长度(r28);
  cn_var_长度_0 = r29;
  long long cn_var_i_1;
  r30 = cn_var_长度_0;
  r31 = r30 - 1;
  cn_var_i_1 = r31;
  goto for_cond_114;

  if_else_112:
  r44 = cn_var_输出路径;
  r45 = 复制字符串(r44, "output.c");
  goto if_merge_113;

  if_merge_113:
  goto if_merge_110;

  for_cond_114:
  r32 = cn_var_i_1;
  r33 = r32 >= 0;
  if (r33) goto for_body_115; else goto for_exit_117;

  for_body_115:
  r34 = cn_var_输出路径;
  r35 = cn_var_i_1;
  r36 = *(char*)cn_rt_array_get_element(r34, r35, 8);
  r37 = r36 == 46;
  if (r37) goto if_then_118; else goto if_merge_119;

  for_update_116:
  r40 = cn_var_i_1;
  r41 = r40 - 1;
  cn_var_i_1 = r41;
  goto for_cond_114;

  for_exit_117:
  r42 = cn_var_输出路径;
  r43 = 连接字符串(r42, ".c");
  goto if_merge_113;

  if_then_118:
  r38 = cn_var_输出路径;
  r39 = cn_var_i_1;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r38, r39, &_tmp_i0, 8); }
  goto for_exit_117;
  goto if_merge_119;

  if_merge_119:
  goto for_update_116;

  if_then_120:
  r61 = 打印格式("错误: 无法创建代码生成上下文\n");
  return 0;
  goto if_merge_121;

  if_merge_121:
  r62 = cn_var_上下文;
  r63 = r62->代码生成上下文;
  生成模块代码(r63);
  r64 = cn_var_上下文;
  r65 = r64->选项;
  r66 = r65.详细输出;
  if (r66) goto if_then_122; else goto if_merge_123;

  if_then_122:
  r67 = cn_var_输出路径;
  r68 = 打印格式("代码生成完成: %s\n", r67);
  goto if_merge_123;

  if_merge_123:
  return 1;
}

_Bool 编译文件(struct 编译上下文* cn_var_上下文) {
  long long r0, r13, r19, r26, r29;
  char* r17;
  char* r28;
  char* r32;
  char* r37;
  char* r41;
  char* r46;
  char* r50;
  char* r55;
  char* r59;
  char* r64;
  char* r68;
  char* r69;
  struct 编译上下文* r1;
  struct 编译上下文* r3;
  char* r5;
  struct 编译上下文* r7;
  struct 编译上下文* r10;
  char* r12;
  struct 编译上下文* r14;
  char* r16;
  char** r18;
  long long* r20;
  struct 编译上下文* r23;
  char* r25;
  struct 编译上下文* r27;
  struct 编译上下文* r33;
  struct 编译上下文* r38;
  struct 编译上下文* r42;
  struct 编译上下文* r47;
  struct 编译上下文* r51;
  struct 编译上下文* r56;
  struct 编译上下文* r60;
  struct 编译上下文* r65;
  _Bool r2;
  _Bool r6;
  _Bool r9;
  _Bool r21;
  _Bool r22;
  _Bool r30;
  _Bool r31;
  _Bool r36;
  _Bool r39;
  _Bool r40;
  _Bool r45;
  _Bool r48;
  _Bool r49;
  _Bool r54;
  _Bool r57;
  _Bool r58;
  _Bool r63;
  _Bool r66;
  _Bool r67;
  struct 编译选项 r4;
  struct 编译选项 r8;
  struct 编译选项 r11;
  struct 编译选项 r15;
  struct 编译选项 r24;
  struct 编译选项 r34;
  struct 编译选项 r43;
  struct 编译选项 r52;
  struct 编译选项 r61;
  enum 编译阶段 r35;
  enum 编译阶段 r44;
  enum 编译阶段 r53;
  enum 编译阶段 r62;

  entry:
  r1 = cn_var_上下文;
  r2 = r1 == 0;
  if (r2) goto logic_merge_127; else goto logic_rhs_126;

  if_then_124:
  return 0;
  goto if_merge_125;

  if_merge_125:
  r7 = cn_var_上下文;
  r8 = r7->选项;
  r9 = r8.详细输出;
  if (r9) goto if_then_128; else goto if_merge_129;

  logic_rhs_126:
  r3 = cn_var_上下文;
  r4 = r3->选项;
  r5 = r4.输入文件路径;
  r6 = r5 == 0;
  goto logic_merge_127;

  logic_merge_127:
  if (r6) goto if_then_124; else goto if_merge_125;

  if_then_128:
  r10 = cn_var_上下文;
  r11 = r10->选项;
  r12 = r11.输入文件路径;
  r13 = 打印格式("正在编译: %s\n", r12);
  goto if_merge_129;

  if_merge_129:
  char* cn_var_源码_0;
  cn_var_源码_0 = 0;
  long long cn_var_源码长度_1;
  cn_var_源码长度_1 = 0;
  r14 = cn_var_上下文;
  r15 = r14->选项;
  r16 = r15.输入文件路径;
  r17 = cn_var_源码_0;
  r18 = &cn_var_源码_0;
  r19 = cn_var_源码长度_1;
  r20 = &cn_var_源码长度_1;
  r21 = 读取文件内容到缓冲区(r16, r18, r20);
  r22 = !r21;
  if (r22) goto if_then_130; else goto if_merge_131;

  if_then_130:
  r23 = cn_var_上下文;
  r24 = r23->选项;
  r25 = r24.输入文件路径;
  r26 = 打印格式("错误: 无法读取文件: %s\n", r25);
  return 0;
  goto if_merge_131;

  if_merge_131:
  r27 = cn_var_上下文;
  r28 = cn_var_源码_0;
  r29 = cn_var_源码长度_1;
  r30 = 执行词法分析(r27, r28, r29);
  r31 = !r30;
  if (r31) goto if_then_132; else goto if_merge_133;

  if_then_132:
  r32 = cn_var_源码_0;
  释放内存(r32);
  return 0;
  goto if_merge_133;

  if_merge_133:
  r33 = cn_var_上下文;
  r34 = r33->选项;
  r35 = r34.停止阶段;
  r36 = r35 == 0;
  if (r36) goto if_then_134; else goto if_merge_135;

  if_then_134:
  r37 = cn_var_源码_0;
  释放内存(r37);
  return 1;
  goto if_merge_135;

  if_merge_135:
  r38 = cn_var_上下文;
  r39 = 执行语法分析(r38);
  r40 = !r39;
  if (r40) goto if_then_136; else goto if_merge_137;

  if_then_136:
  r41 = cn_var_源码_0;
  释放内存(r41);
  return 0;
  goto if_merge_137;

  if_merge_137:
  r42 = cn_var_上下文;
  r43 = r42->选项;
  r44 = r43.停止阶段;
  r45 = r44 == 1;
  if (r45) goto if_then_138; else goto if_merge_139;

  if_then_138:
  r46 = cn_var_源码_0;
  释放内存(r46);
  return 1;
  goto if_merge_139;

  if_merge_139:
  r47 = cn_var_上下文;
  r48 = 执行语义分析(r47);
  r49 = !r48;
  if (r49) goto if_then_140; else goto if_merge_141;

  if_then_140:
  r50 = cn_var_源码_0;
  释放内存(r50);
  return 0;
  goto if_merge_141;

  if_merge_141:
  r51 = cn_var_上下文;
  r52 = r51->选项;
  r53 = r52.停止阶段;
  r54 = r53 == 2;
  if (r54) goto if_then_142; else goto if_merge_143;

  if_then_142:
  r55 = cn_var_源码_0;
  释放内存(r55);
  return 1;
  goto if_merge_143;

  if_merge_143:
  r56 = cn_var_上下文;
  r57 = 执行IR生成(r56);
  r58 = !r57;
  if (r58) goto if_then_144; else goto if_merge_145;

  if_then_144:
  r59 = cn_var_源码_0;
  释放内存(r59);
  return 0;
  goto if_merge_145;

  if_merge_145:
  r60 = cn_var_上下文;
  r61 = r60->选项;
  r62 = r61.停止阶段;
  r63 = r62 == 3;
  if (r63) goto if_then_146; else goto if_merge_147;

  if_then_146:
  r64 = cn_var_源码_0;
  释放内存(r64);
  return 1;
  goto if_merge_147;

  if_merge_147:
  r65 = cn_var_上下文;
  r66 = 执行代码生成(r65);
  r67 = !r66;
  if (r67) goto if_then_148; else goto if_merge_149;

  if_then_148:
  r68 = cn_var_源码_0;
  释放内存(r68);
  return 0;
  goto if_merge_149;

  if_merge_149:
  r69 = cn_var_源码_0;
  释放内存(r69);
  return 1;
}

_Bool 读取文件内容到缓冲区(char* cn_var_文件路径, char** cn_var_缓冲区, long long* cn_var_大小) {
  long long r0, r1, r13, r15, r17, r18, r21, r22, r29, r30, r32, r33, r37, r39;
  char* r2;
  char* r8;
  char* r24;
  char* r28;
  char* r35;
  char* r36;
  char* r38;
  char** r4;
  long long* r6;
  void* r9;
  void* r10;
  void* r12;
  void* r14;
  void* r16;
  void* r20;
  void* r23;
  void* r26;
  void* r27;
  void* r31;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r19;
  _Bool r25;
  _Bool r34;

  entry:
  r2 = cn_var_文件路径;
  r3 = r2 == 0;
  if (r3) goto logic_merge_155; else goto logic_rhs_154;

  if_then_150:
  return 0;
  goto if_merge_151;

  if_merge_151:
  void* cn_var_文件_0;
  r8 = cn_var_文件路径;
  r9 = 打开文件(r8, "rb");
  cn_var_文件_0 = r9;
  r10 = cn_var_文件_0;
  r11 = r10 == 0;
  if (r11) goto if_then_156; else goto if_merge_157;

  logic_rhs_152:
  r6 = cn_var_大小;
  r7 = r6 == 0;
  goto logic_merge_153;

  logic_merge_153:
  if (r7) goto if_then_150; else goto if_merge_151;

  logic_rhs_154:
  r4 = cn_var_缓冲区;
  r5 = r4 == 0;
  goto logic_merge_155;

  logic_merge_155:
  if (r5) goto logic_merge_153; else goto logic_rhs_152;

  if_then_156:
  return 0;
  goto if_merge_157;

  if_merge_157:
  r12 = cn_var_文件_0;
  r13 = 文件定位(r12, 0, 2);
  long long cn_var_文件大小_1;
  r14 = cn_var_文件_0;
  r15 = 获取文件位置(r14);
  cn_var_文件大小_1 = r15;
  r16 = cn_var_文件_0;
  r17 = 文件定位(r16, 0, 0);
  r18 = cn_var_文件大小_1;
  r19 = r18 <= 0;
  if (r19) goto if_then_158; else goto if_merge_159;

  if_then_158:
  r20 = cn_var_文件_0;
  关闭文件(r20);
  return 0;
  goto if_merge_159;

  if_merge_159:
  char* cn_var_内容_2;
  r21 = cn_var_文件大小_1;
  r22 = r21 + 1;
  r23 = 分配内存(r22);
  cn_var_内容_2 = r23;
  r24 = cn_var_内容_2;
  r25 = r24 == 0;
  if (r25) goto if_then_160; else goto if_merge_161;

  if_then_160:
  r26 = cn_var_文件_0;
  关闭文件(r26);
  return 0;
  goto if_merge_161;

  if_merge_161:
  long long cn_var_读取字节数_3;
  r27 = cn_var_文件_0;
  r28 = cn_var_内容_2;
  r29 = cn_var_文件大小_1;
  r30 = 读取文件(r27, r28, r29);
  cn_var_读取字节数_3 = r30;
  r31 = cn_var_文件_0;
  关闭文件(r31);
  r32 = cn_var_读取字节数_3;
  r33 = cn_var_文件大小_1;
  r34 = r32 != r33;
  if (r34) goto if_then_162; else goto if_merge_163;

  if_then_162:
  r35 = cn_var_内容_2;
  释放内存(r35);
  return 0;
  goto if_merge_163;

  if_merge_163:
  r36 = cn_var_内容_2;
  r37 = cn_var_文件大小_1;
    { long long _tmp_i1 = 0; cn_rt_array_set_element(r36, r37, &_tmp_i1, 8); }
  r38 = cn_var_内容_2;
  r39 = cn_var_文件大小_1;
  return 1;
}

long long 编译上下文大小() {

  entry:
  return 512;
}

long long 编译选项大小() {

  entry:
  return 256;
}

long long main(long long cn_var_参数数量, char** cn_var_参数数组) {
  cn_rt_init();
  long long r0, r6, r14, r20, r25, r27, r28, r30, r31;
  char** r1;
  struct 编译选项* r3;
  char* r12;
  struct 编译选项* r16;
  struct 编译上下文* r17;
  struct 编译上下文* r18;
  struct 编译上下文* r21;
  struct 编译上下文* r26;
  struct 编译上下文* r29;
  struct 编译上下文* r32;
  _Bool r4;
  _Bool r5;
  _Bool r8;
  _Bool r10;
  _Bool r13;
  _Bool r19;
  _Bool r22;
  _Bool r24;
  _Bool r33;
  struct 编译选项 r2;
  struct 编译选项 r7;
  struct 编译选项 r9;
  struct 编译选项 r11;
  struct 编译选项 r15;
  struct 编译选项 r23;

  entry:
  struct 编译选项 cn_var_选项_0;
  r0 = cn_var_参数数量;
  r1 = cn_var_参数数组;
  r2 = cn_var_选项_0;
  r3 = &cn_var_选项_0;
  r4 = 解析命令行参数(r0, r1, r3);
  r5 = !r4;
  if (r5) goto if_then_164; else goto if_merge_165;

  if_then_164:
  r6 = 打印格式("错误: 无法解析命令行参数\n");
  return 1;
  goto if_merge_165;

  if_merge_165:
  r7 = cn_var_选项_0;
  r8 = r7.显示版本;
  if (r8) goto if_then_166; else goto if_merge_167;

  if_then_166:
  显示版本信息();
  return 0;
  goto if_merge_167;

  if_merge_167:
  r9 = cn_var_选项_0;
  r10 = r9.显示帮助;
  if (r10) goto if_then_168; else goto if_merge_169;

  if_then_168:
  显示帮助信息();
  return 0;
  goto if_merge_169;

  if_merge_169:
  r11 = cn_var_选项_0;
  r12 = r11.输入文件路径;
  r13 = r12 == 0;
  if (r13) goto if_then_170; else goto if_merge_171;

  if_then_170:
  r14 = 打印格式("错误: 未指定输入文件\n");
  显示帮助信息();
  return 1;
  goto if_merge_171;

  if_merge_171:
  struct 编译上下文* cn_var_上下文_1;
  r15 = cn_var_选项_0;
  r16 = &cn_var_选项_0;
  r17 = 创建编译上下文(r16);
  cn_var_上下文_1 = r17;
  r18 = cn_var_上下文_1;
  r19 = r18 == 0;
  if (r19) goto if_then_172; else goto if_merge_173;

  if_then_172:
  r20 = 打印格式("错误: 无法创建编译上下文\n");
  return 1;
  goto if_merge_173;

  if_merge_173:
  _Bool cn_var_编译结果_2;
  r21 = cn_var_上下文_1;
  r22 = 编译文件(r21);
  cn_var_编译结果_2 = r22;
  r23 = cn_var_选项_0;
  r24 = r23.详细输出;
  if (r24) goto if_then_174; else goto if_merge_175;

  if_then_174:
  r25 = 打印格式("\n编译统计:\n");
  r26 = cn_var_上下文_1;
  r27 = r26->总错误数;
  r28 = 打印格式("  总错误数: %d\n", r27);
  r29 = cn_var_上下文_1;
  r30 = r29->总警告数;
  r31 = 打印格式("  总警告数: %d\n", r30);
  goto if_merge_175;

  if_merge_175:
  r32 = cn_var_上下文_1;
  销毁编译上下文(r32);
  r33 = cn_var_编译结果_2;
  if (r33) goto if_then_176; else goto if_else_177;

  if_then_176:
  return 0;
  goto if_merge_178;

  if_else_177:
  return 1;
  goto if_merge_178;

  if_merge_178:
  return 0;
  cn_rt_exit();
}

