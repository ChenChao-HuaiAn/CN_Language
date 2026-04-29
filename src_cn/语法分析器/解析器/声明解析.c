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
enum 语法恢复策略 {
    语法恢复策略_不恢复 = 0,
    语法恢复策略_跳过词元 = 1,
    语法恢复策略_同步到分号 = 2,
    语法恢复策略_同步到大括号 = 3,
    语法恢复策略_同步到关键字 = 4
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

// Struct Definitions - 从导入模块
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
struct 关键字条目;
struct 关键字条目 {
    char* 名称;
    long long 类型;
};

// Forward Declarations - 从导入模块
extern long long cn_var_最大错误恢复次数;
extern long long cn_var_解析器大小;
_Bool 是否类型关键字(enum 词元类型枚举);
_Bool 同步恢复(struct 解析器*);
void 报告错误期望(struct 解析器*, enum 词元类型枚举);
_Bool 匹配多种(struct 解析器*, enum 词元类型枚举*, long long);
_Bool 匹配(struct 解析器*, enum 词元类型枚举);
_Bool 检查(struct 解析器*, enum 词元类型枚举);
_Bool 期望(struct 解析器*, enum 词元类型枚举);
void 前进词元(struct 解析器*);
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
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
void 初始化关键字表(void);
struct 关键字条目* 获取关键字表(void);
long long 关键字总数(void);
_Bool 是关键字字符串(const char*);
enum 词元类型枚举 查找关键字(const char*);
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
struct 扫描器* 创建扫描器(const char*, long long);
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
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, const char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);

// Global Variables

// Forward Declarations
struct 声明节点* 解析顶层声明(struct 解析器*);
struct 声明节点* 解析函数声明(struct 解析器*);
struct 参数列表* 解析参数列表(struct 解析器*);
struct 参数节点* 解析参数(struct 解析器*);
struct 声明节点* 解析变量声明(struct 解析器*);
struct 声明节点* 解析结构体声明(struct 解析器*);
struct 声明节点* 解析枚举声明(struct 解析器*);
struct 声明节点* 解析类声明(struct 解析器*);
struct 声明节点* 解析接口声明(struct 解析器*);
struct 声明节点* 解析模块声明(struct 解析器*);
struct 声明节点* 解析导入声明(struct 解析器*);
enum 可见性 解析可见性修饰符(struct 解析器*);
struct 声明节点* 解析顶层声明(struct 解析器*);
struct 声明节点* 解析函数声明(struct 解析器*);
struct 参数列表* 解析参数列表(struct 解析器*);
struct 参数节点* 解析参数(struct 解析器*);
struct 声明节点* 解析变量声明(struct 解析器*);
struct 声明节点* 解析结构体声明(struct 解析器*);
struct 声明节点* 解析枚举声明(struct 解析器*);
struct 声明节点* 解析类声明(struct 解析器*);
struct 声明节点* 解析接口声明(struct 解析器*);
struct 声明节点* 解析模块声明(struct 解析器*);
struct 声明节点* 解析导入声明(struct 解析器*);
enum 可见性 解析可见性修饰符(struct 解析器*);

// Extern Declarations - 跨模块调用的函数（ANSI原型风格）
extern void* 创建函数声明(void);
extern void* 解析类型(void*);
extern void* 解析块语句(void*);
extern void* 创建参数列表(void);
extern void* 参数列表添加(void*, void*);
extern void* 创建参数节点(void);
extern void* 创建变量声明(void);
extern void* 字符串转整数(const char*);
extern void* 数组大小列表添加(void*, void*);
extern void* 解析表达式(void*);
extern void* 创建结构体声明(void);
extern void* 结构体添加成员(void*, void*);
extern void* 创建枚举声明(void);
extern void* 创建枚举成员(void);
extern void* 枚举添加成员(void*, void*);
extern void* 创建类声明(void);
extern void* 类添加实现接口(void*, void*);
extern void* 类添加方法(void*, void*);
extern void* 类添加字段(void*, void*);
extern void* 创建接口声明(void);
extern void* 接口添加方法(void*, void*);
extern void* 创建模块声明(void);
extern void* 创建导入声明(void);
extern void* 导入添加成员(void*, void*);

struct 声明节点* 解析顶层声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析函数声明(struct 解析器* cn_var_实例);
struct 参数列表* 解析参数列表(struct 解析器* cn_var_实例);
struct 参数节点* 解析参数(struct 解析器* cn_var_实例);
struct 声明节点* 解析变量声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析结构体声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析枚举声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析类声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析接口声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析模块声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析导入声明(struct 解析器* cn_var_实例);
enum 可见性 解析可见性修饰符(struct 解析器* cn_var_实例);
struct 声明节点* 解析顶层声明(struct 解析器* cn_var_实例) {
  long long r1;
  char* r39;
  char* r40;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r16;
  struct 声明节点* r17;
  struct 解析器* r18;
  struct 声明节点* r19;
  struct 解析器* r20;
  struct 声明节点* r21;
  struct 解析器* r22;
  struct 声明节点* r23;
  struct 解析器* r24;
  struct 声明节点* r25;
  struct 解析器* r26;
  struct 声明节点* r27;
  struct 解析器* r28;
  struct 声明节点* r29;
  struct 解析器* r30;
  struct 声明节点* r31;
  struct 解析器* r35;
  struct 声明节点* r36;
  struct 解析器* r37;
  struct 解析器* r41;
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
  _Bool r34;
  struct 词元 r4;
  struct 词元 r32;
  struct 词元 r38;
  enum 可见性 r3;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r33;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1413; else goto if_merge_1414;

  if_then_1413:
  return 0;
  goto if_merge_1414;

  if_merge_1414:
  enum 可见性 cn_var_当前可见性_0;
  r2 = cn_var_实例;
  r3 = 解析可见性修饰符(r2);
  cn_var_当前可见性_0 = r3;
  r4 = cn_var_实例->当前词元;
  r5 = r4.类型;
  r6 = r5 == 词元类型枚举_关键字_函数;
  if (r6) goto case_body_1416; else goto switch_check_1427;

  switch_check_1427:
  r7 = r5 == 词元类型枚举_关键字_结构体;
  if (r7) goto case_body_1417; else goto switch_check_1428;

  switch_check_1428:
  r8 = r5 == 词元类型枚举_关键字_枚举;
  if (r8) goto case_body_1418; else goto switch_check_1429;

  switch_check_1429:
  r9 = r5 == 词元类型枚举_关键字_类;
  if (r9) goto case_body_1419; else goto switch_check_1430;

  switch_check_1430:
  r10 = r5 == 词元类型枚举_关键字_接口;
  if (r10) goto case_body_1420; else goto switch_check_1431;

  switch_check_1431:
  r11 = r5 == /* NONE */;
  if (r11) goto case_body_1421; else goto switch_check_1432;

  switch_check_1432:
  r12 = r5 == 词元类型枚举_关键字_导入;
  if (r12) goto case_body_1422; else goto switch_check_1433;

  switch_check_1433:
  r13 = r5 == 词元类型枚举_关键字_变量;
  if (r13) goto case_body_1423; else goto switch_check_1434;

  switch_check_1434:
  r14 = r5 == /* NONE */;
  if (r14) goto case_body_1424; else goto switch_check_1435;

  switch_check_1435:
  r15 = r5 == 词元类型枚举_关键字_静态;
  if (r15) goto case_body_1425; else goto case_default_1426;

  case_body_1416:
  r16 = cn_var_实例;
  r17 = 解析函数声明(r16);
  return r17;
  goto switch_merge_1415;

  case_body_1417:
  r18 = cn_var_实例;
  r19 = 解析结构体声明(r18);
  return r19;
  goto switch_merge_1415;

  case_body_1418:
  r20 = cn_var_实例;
  r21 = 解析枚举声明(r20);
  return r21;
  goto switch_merge_1415;

  case_body_1419:
  r22 = cn_var_实例;
  r23 = 解析类声明(r22);
  return r23;
  goto switch_merge_1415;

  case_body_1420:
  r24 = cn_var_实例;
  r25 = 解析接口声明(r24);
  return r25;
  goto switch_merge_1415;

  case_body_1421:
  r26 = cn_var_实例;
  r27 = 解析模块声明(r26);
  return r27;
  goto switch_merge_1415;

  case_body_1422:
  r28 = cn_var_实例;
  r29 = 解析导入声明(r28);
  return r29;
  goto switch_merge_1415;

  case_body_1423:
  goto switch_merge_1415;

  case_body_1424:
  goto switch_merge_1415;

  case_body_1425:
  r30 = cn_var_实例;
  r31 = 解析变量声明(r30);
  return r31;
  goto switch_merge_1415;

  case_default_1426:
  r32 = cn_var_实例->当前词元;
  r33 = r32.类型;
  r34 = 是否类型关键字(r33);
  if (r34) goto if_then_1436; else goto if_merge_1437;

  if_then_1436:
  r35 = cn_var_实例;
  r36 = 解析变量声明(r35);
  return r36;
  goto if_merge_1437;

  if_merge_1437:
  r37 = cn_var_实例;
  r38 = cn_var_实例->当前词元;
  r39 = r38.值;
  r40 = "意外的词元: " + r39;
  报告错误(r37, 诊断错误码_语法_期望标记, r40);
  r41 = cn_var_实例;
  前进词元(r41);
  return 0;
  goto switch_merge_1415;

  switch_merge_1415:
  return NULL;
}

struct 声明节点* 解析函数声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r4, r5, r6;
  char* r25;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 解析器* r30;
  struct 参数列表* r31;
  struct 解析器* r32;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r37;
  struct 类型节点* r38;
  struct 解析器* r39;
  struct 解析器* r41;
  struct 块语句节点* r42;
  struct 声明节点* r43;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r17;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r23;
  _Bool r27;
  _Bool r29;
  _Bool r33;
  _Bool r35;
  _Bool r40;
  struct 词元 r15;
  struct 词元 r24;
  enum 词元类型枚举 r16;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1438; else goto if_merge_1439;

  if_then_1438:
  return 0;
  goto if_merge_1439;

  if_merge_1439:
  struct 声明节点* cn_var_节点_0;
  创建函数声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1440; else goto if_merge_1441;

  if_then_1440:
  return 0;
  goto if_merge_1441;

  if_merge_1441:
  goto while_cond_1442;

  while_cond_1442:
  r7 = cn_var_实例;
  r8 = 检查(r7, 词元类型枚举_关键字_重写);
  if (r8) goto logic_merge_1450; else goto logic_rhs_1449;

  while_body_1443:
  r15 = cn_var_实例->当前词元;
  r16 = r15.类型;
  r17 = r16 == 词元类型枚举_关键字_重写;
  if (r17) goto case_body_1452; else goto switch_check_1456;

  while_exit_1444:
  r22 = cn_var_实例;
  r23 = 期望(r22, 词元类型枚举_关键字_函数);
  r24 = cn_var_实例->当前词元;
  r25 = r24.值;
  r26 = cn_var_实例;
  r27 = 期望(r26, 词元类型枚举_标识符);
  r28 = cn_var_实例;
  r29 = 期望(r28, 词元类型枚举_左括号);
  r30 = cn_var_实例;
  r31 = 解析参数列表(r30);
  r32 = cn_var_实例;
  r33 = 期望(r32, 词元类型枚举_右括号);
  r34 = cn_var_实例;
  r35 = 检查(r34, 词元类型枚举_箭头);
  if (r35) goto if_then_1459; else goto if_merge_1460;

  logic_rhs_1445:
  r13 = cn_var_实例;
  r14 = 检查(r13, 词元类型枚举_关键字_抽象);
  goto logic_merge_1446;

  logic_merge_1446:
  if (r14) goto while_body_1443; else goto while_exit_1444;

  logic_rhs_1447:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_关键字_静态);
  goto logic_merge_1448;

  logic_merge_1448:
  if (r12) goto logic_merge_1446; else goto logic_rhs_1445;

  logic_rhs_1449:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_关键字_虚拟);
  goto logic_merge_1450;

  logic_merge_1450:
  if (r10) goto logic_merge_1448; else goto logic_rhs_1447;

  switch_check_1456:
  r18 = r16 == 词元类型枚举_关键字_虚拟;
  if (r18) goto case_body_1453; else goto switch_check_1457;

  switch_check_1457:
  r19 = r16 == 词元类型枚举_关键字_静态;
  if (r19) goto case_body_1454; else goto switch_check_1458;

  switch_check_1458:
  r20 = r16 == 词元类型枚举_关键字_抽象;
  if (r20) goto case_body_1455; else goto switch_merge_1451;

  case_body_1452:
  goto switch_merge_1451;
  goto switch_merge_1451;

  case_body_1453:
  goto switch_merge_1451;
  goto switch_merge_1451;

  case_body_1454:
  goto switch_merge_1451;
  goto switch_merge_1451;

  case_body_1455:
  goto switch_merge_1451;
  goto switch_merge_1451;

  switch_merge_1451:
  r21 = cn_var_实例;
  前进词元(r21);
  goto while_cond_1442;

  if_then_1459:
  r36 = cn_var_实例;
  前进词元(r36);
  r37 = cn_var_实例;
  r38 = 解析类型(r37);
  goto if_merge_1460;

  if_merge_1460:
  r39 = cn_var_实例;
  r40 = 检查(r39, 词元类型枚举_左大括号);
  if (r40) goto if_then_1461; else goto if_else_1462;

  if_then_1461:
  r41 = cn_var_实例;
  r42 = 解析块语句(r41);
  goto if_merge_1463;

  if_else_1462:
  goto if_merge_1463;

  if_merge_1463:
  r43 = cn_var_节点_0;
  return r43;
}

struct 参数列表* 解析参数列表(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r10, r15;
  struct 解析器* r0;
  struct 参数列表* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 参数节点* r8;
  struct 参数节点* r9;
  struct 参数列表* r11;
  struct 参数节点* r12;
  struct 解析器* r13;
  struct 参数列表* r16;
  _Bool r5;
  _Bool r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1464; else goto if_merge_1465;

  if_then_1464:
  return 0;
  goto if_merge_1465;

  if_merge_1465:
  struct 参数列表* cn_var_列表_0;
  创建参数列表();
  r2 = cn_var_列表_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1466; else goto if_merge_1467;

  if_then_1466:
  return 0;
  goto if_merge_1467;

  if_merge_1467:
  goto while_cond_1468;

  while_cond_1468:
  r4 = cn_var_实例;
  r5 = 检查(r4, 词元类型枚举_右括号);
  r6 = !r5;
  if (r6) goto while_body_1469; else goto while_exit_1470;

  while_body_1469:
  struct 参数节点* cn_var_参数_1;
  r7 = cn_var_实例;
  r8 = 解析参数(r7);
  cn_var_参数_1 = r8;
  r9 = cn_var_参数_1;
  r10 = r9 != 0;
  if (r10) goto if_then_1471; else goto if_merge_1472;

  while_exit_1470:
  r16 = cn_var_列表_0;
  return r16;

  if_then_1471:
  r11 = cn_var_列表_0;
  r12 = cn_var_参数_1;
  参数列表添加(r11, r12);
  goto if_merge_1472;

  if_merge_1472:
  r13 = cn_var_实例;
  r14 = 匹配(r13, 词元类型枚举_逗号);
  r15 = !r14;
  if (r15) goto if_then_1473; else goto if_merge_1474;

  if_then_1473:
  goto while_exit_1470;
  goto if_merge_1474;

  if_merge_1474:
  goto while_cond_1468;
  return NULL;
}

struct 参数节点* 解析参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r13, r14;
  char* r7;
  struct 解析器* r0;
  struct 参数节点* r2;
  struct 解析器* r4;
  struct 类型节点* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r17;
  struct 解析器* r18;
  struct 参数节点* r20;
  _Bool r9;
  _Bool r11;
  _Bool r16;
  _Bool r19;
  struct 词元 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1475; else goto if_merge_1476;

  if_then_1475:
  return 0;
  goto if_merge_1476;

  if_merge_1476:
  struct 参数节点* cn_var_参数_0;
  创建参数节点();
  r2 = cn_var_参数_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1477; else goto if_merge_1478;

  if_then_1477:
  return 0;
  goto if_merge_1478;

  if_merge_1478:
  r4 = cn_var_实例;
  r5 = 解析类型(r4);
  r6 = cn_var_实例->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = 期望(r8, 词元类型枚举_标识符);
  goto while_cond_1479;

  while_cond_1479:
  r10 = cn_var_实例;
  r11 = 检查(r10, 词元类型枚举_左方括号);
  if (r11) goto while_body_1480; else goto while_exit_1481;

  while_body_1480:
  r12 = cn_var_实例;
  前进词元(r12);
  r13 = cn_var_参数_0.数组维度;
  r14 = r13 + 1;
  r15 = cn_var_实例;
  r16 = 检查(r15, 词元类型枚举_整数字面量);
  if (r16) goto if_then_1482; else goto if_merge_1483;

  while_exit_1481:
  r20 = cn_var_参数_0;
  return r20;

  if_then_1482:
  r17 = cn_var_实例;
  前进词元(r17);
  goto if_merge_1483;

  if_merge_1483:
  r18 = cn_var_实例;
  r19 = 期望(r18, 词元类型枚举_右方括号);
  goto while_cond_1479;
  return NULL;
}

struct 声明节点* 解析变量声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r25, r26, r27, r30, r31;
  char* r19;
  char* r33;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r6;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r13;
  struct 类型节点* r14;
  struct 解析器* r15;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r22;
  struct 解析器* r24;
  struct 解析器* r28;
  struct 解析器* r34;
  struct 解析器* r35;
  struct 解析器* r37;
  struct 解析器* r39;
  struct 表达式节点* r40;
  struct 解析器* r41;
  struct 声明节点* r43;
  _Bool r5;
  _Bool r8;
  _Bool r12;
  _Bool r16;
  _Bool r21;
  _Bool r23;
  _Bool r29;
  _Bool r36;
  _Bool r38;
  _Bool r42;
  struct 词元 r10;
  struct 词元 r18;
  struct 词元 r32;
  enum 词元类型枚举 r11;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1484; else goto if_merge_1485;

  if_then_1484:
  return 0;
  goto if_merge_1485;

  if_merge_1485:
  struct 声明节点* cn_var_节点_0;
  创建变量声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1486; else goto if_merge_1487;

  if_then_1486:
  return 0;
  goto if_merge_1487;

  if_merge_1487:
  r4 = cn_var_实例;
  r5 = 检查(r4, 词元类型枚举_关键字_静态);
  if (r5) goto if_then_1488; else goto if_merge_1489;

  if_then_1488:
  r6 = cn_var_实例;
  前进词元(r6);
  goto if_merge_1489;

  if_merge_1489:
  r7 = cn_var_实例;
  r8 = 检查(r7, /* NONE */);
  if (r8) goto if_then_1490; else goto if_merge_1491;

  if_then_1490:
  r9 = cn_var_实例;
  前进词元(r9);
  goto if_merge_1491;

  if_merge_1491:
  r10 = cn_var_实例->当前词元;
  r11 = r10.类型;
  r12 = 是否类型关键字(r11);
  if (r12) goto if_then_1492; else goto if_else_1493;

  if_then_1492:
  r13 = cn_var_实例;
  r14 = 解析类型(r13);
  goto if_merge_1494;

  if_else_1493:
  r15 = cn_var_实例;
  r16 = 检查(r15, 词元类型枚举_关键字_变量);
  if (r16) goto if_then_1495; else goto if_merge_1496;

  if_merge_1494:
  r18 = cn_var_实例->当前词元;
  r19 = r18.值;
  r20 = cn_var_实例;
  r21 = 期望(r20, 词元类型枚举_标识符);
  goto while_cond_1497;

  if_then_1495:
  r17 = cn_var_实例;
  前进词元(r17);
  goto if_merge_1496;

  if_merge_1496:
  goto if_merge_1494;

  while_cond_1497:
  r22 = cn_var_实例;
  r23 = 检查(r22, 词元类型枚举_左方括号);
  if (r23) goto while_body_1498; else goto while_exit_1499;

  while_body_1498:
  r24 = cn_var_实例;
  前进词元(r24);
  r25 = cn_var_节点_0->变量声明;
  r26 = r25.数组维度;
  r27 = r26 + 1;
  r28 = cn_var_实例;
  r29 = 检查(r28, 词元类型枚举_整数字面量);
  if (r29) goto if_then_1500; else goto if_merge_1501;

  while_exit_1499:
  r37 = cn_var_实例;
  r38 = 匹配(r37, 词元类型枚举_赋值);
  if (r38) goto if_then_1502; else goto if_merge_1503;

  if_then_1500:
  r30 = cn_var_节点_0->变量声明;
  r31 = r30.数组大小;
  r32 = cn_var_实例->当前词元;
  r33 = r32.值;
  字符串转整数(r33);
  数组大小列表添加(r31, /* NONE */);
  r34 = cn_var_实例;
  前进词元(r34);
  goto if_merge_1501;

  if_merge_1501:
  r35 = cn_var_实例;
  r36 = 期望(r35, 词元类型枚举_右方括号);
  goto while_cond_1497;

  if_then_1502:
  r39 = cn_var_实例;
  r40 = 解析表达式(r39);
  goto if_merge_1503;

  if_merge_1503:
  r41 = cn_var_实例;
  r42 = 期望(r41, 词元类型枚举_分号);
  r43 = cn_var_节点_0;
  return r43;
}

struct 声明节点* 解析结构体声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r13, r17;
  char* r6;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 声明节点* r15;
  struct 声明节点* r16;
  struct 声明节点* r18;
  struct 声明节点* r19;
  struct 解析器* r20;
  struct 声明节点* r22;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r21;
  struct 词元 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1504; else goto if_merge_1505;

  if_then_1504:
  return 0;
  goto if_merge_1505;

  if_merge_1505:
  struct 声明节点* cn_var_节点_0;
  创建结构体声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1506; else goto if_merge_1507;

  if_then_1506:
  return 0;
  goto if_merge_1507;

  if_merge_1507:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  r8 = 期望(r7, 词元类型枚举_标识符);
  r9 = cn_var_实例;
  r10 = 期望(r9, 词元类型枚举_左大括号);
  goto while_cond_1508;

  while_cond_1508:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_右大括号);
  r13 = !r12;
  if (r13) goto while_body_1509; else goto while_exit_1510;

  while_body_1509:
  struct 声明节点* cn_var_成员_1;
  r14 = cn_var_实例;
  r15 = 解析变量声明(r14);
  cn_var_成员_1 = r15;
  r16 = cn_var_成员_1;
  r17 = r16 != 0;
  if (r17) goto if_then_1511; else goto if_merge_1512;

  while_exit_1510:
  r20 = cn_var_实例;
  r21 = 期望(r20, 词元类型枚举_右大括号);
  r22 = cn_var_节点_0;
  return r22;

  if_then_1511:
  r18 = cn_var_节点_0;
  r19 = cn_var_成员_1;
  结构体添加成员(r18, r19);
  goto if_merge_1512;

  if_merge_1512:
  goto while_cond_1508;
  return NULL;
}

struct 声明节点* 解析枚举声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r13, r15, r24, r27, r28, r29;
  char* r6;
  char* r17;
  char* r23;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 枚举成员* r14;
  struct 解析器* r18;
  struct 解析器* r20;
  struct 解析器* r25;
  struct 声明节点* r30;
  struct 枚举成员* r31;
  struct 解析器* r32;
  struct 解析器* r34;
  struct 声明节点* r36;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r19;
  _Bool r21;
  _Bool r26;
  _Bool r33;
  _Bool r35;
  struct 词元 r5;
  struct 词元 r16;
  struct 词元 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1513; else goto if_merge_1514;

  if_then_1513:
  return 0;
  goto if_merge_1514;

  if_merge_1514:
  struct 声明节点* cn_var_节点_0;
  创建枚举声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1515; else goto if_merge_1516;

  if_then_1515:
  return 0;
  goto if_merge_1516;

  if_merge_1516:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  r8 = 期望(r7, 词元类型枚举_标识符);
  r9 = cn_var_实例;
  r10 = 期望(r9, 词元类型枚举_左大括号);
  long long cn_var_当前值_1;
  cn_var_当前值_1 = 0;
  goto while_cond_1517;

  while_cond_1517:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_右大括号);
  r13 = !r12;
  if (r13) goto while_body_1518; else goto while_exit_1519;

  while_body_1518:
  struct 枚举成员* cn_var_成员_2;
  创建枚举成员();
  r14 = cn_var_成员_2;
  r15 = r14 == 0;
  if (r15) goto if_then_1520; else goto if_merge_1521;

  while_exit_1519:
  r34 = cn_var_实例;
  r35 = 期望(r34, 词元类型枚举_右大括号);
  r36 = cn_var_节点_0;
  return r36;

  if_then_1520:
  goto while_exit_1519;
  goto if_merge_1521;

  if_merge_1521:
  r16 = cn_var_实例->当前词元;
  r17 = r16.值;
  r18 = cn_var_实例;
  r19 = 期望(r18, 词元类型枚举_标识符);
  r20 = cn_var_实例;
  r21 = 匹配(r20, 词元类型枚举_赋值);
  if (r21) goto if_then_1522; else goto if_else_1523;

  if_then_1522:
  r22 = cn_var_实例->当前词元;
  r23 = r22.值;
  字符串转整数(r23);
  r24 = cn_var_成员_2->值;
  cn_var_当前值_1 = r24;
  r25 = cn_var_实例;
  r26 = 期望(r25, 词元类型枚举_整数字面量);
  goto if_merge_1524;

  if_else_1523:
  r27 = cn_var_当前值_1;
  goto if_merge_1524;

  if_merge_1524:
  r28 = cn_var_当前值_1;
  r29 = r28 + 1;
  cn_var_当前值_1 = r29;
  r30 = cn_var_节点_0;
  r31 = cn_var_成员_2;
  枚举添加成员(r30, r31);
  r32 = cn_var_实例;
  r33 = 匹配(r32, 词元类型枚举_逗号);
  goto while_cond_1517;
  return NULL;
}

struct 声明节点* 解析类声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r30, r35, r43, r50;
  char* r10;
  char* r16;
  char* r23;
  char* r27;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r6;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 解析器* r24;
  struct 声明节点* r26;
  struct 解析器* r28;
  struct 解析器* r31;
  struct 解析器* r33;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 解析器* r40;
  struct 声明节点* r41;
  struct 声明节点* r42;
  struct 声明节点* r45;
  struct 声明节点* r46;
  struct 解析器* r47;
  struct 声明节点* r48;
  struct 声明节点* r49;
  struct 声明节点* r52;
  struct 声明节点* r53;
  struct 解析器* r54;
  struct 声明节点* r56;
  _Bool r5;
  _Bool r8;
  _Bool r12;
  _Bool r14;
  _Bool r18;
  _Bool r20;
  _Bool r25;
  _Bool r29;
  _Bool r32;
  _Bool r34;
  _Bool r39;
  _Bool r55;
  struct 词元 r9;
  struct 词元 r15;
  struct 词元 r22;
  enum 可见性 r44;
  enum 可见性 r51;
  enum 可见性 r37;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1525; else goto if_merge_1526;

  if_then_1525:
  return 0;
  goto if_merge_1526;

  if_merge_1526:
  struct 声明节点* cn_var_节点_0;
  创建类声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1527; else goto if_merge_1528;

  if_then_1527:
  return 0;
  goto if_merge_1528;

  if_merge_1528:
  r4 = cn_var_实例;
  r5 = 检查(r4, 词元类型枚举_关键字_抽象);
  if (r5) goto if_then_1529; else goto if_merge_1530;

  if_then_1529:
  r6 = cn_var_实例;
  前进词元(r6);
  goto if_merge_1530;

  if_merge_1530:
  r7 = cn_var_实例;
  r8 = 期望(r7, 词元类型枚举_关键字_类);
  r9 = cn_var_实例->当前词元;
  r10 = r9.值;
  r11 = cn_var_实例;
  r12 = 期望(r11, 词元类型枚举_标识符);
  r13 = cn_var_实例;
  r14 = 匹配(r13, 词元类型枚举_冒号);
  if (r14) goto if_then_1531; else goto if_merge_1532;

  if_then_1531:
  r15 = cn_var_实例->当前词元;
  r16 = r15.值;
  r17 = cn_var_实例;
  r18 = 期望(r17, 词元类型枚举_标识符);
  goto if_merge_1532;

  if_merge_1532:
  r19 = cn_var_实例;
  r20 = 检查(r19, 词元类型枚举_关键字_实现);
  if (r20) goto if_then_1533; else goto if_merge_1534;

  if_then_1533:
  r21 = cn_var_实例;
  前进词元(r21);
  goto while_cond_1535;

  if_merge_1534:
  r31 = cn_var_实例;
  r32 = 期望(r31, 词元类型枚举_左大括号);
  goto while_cond_1540;

  while_cond_1535:
  if (1) goto while_body_1536; else goto while_exit_1537;

  while_body_1536:
  char* cn_var_接口名_1;
  r22 = cn_var_实例->当前词元;
  r23 = r22.值;
  cn_var_接口名_1 = r23;
  r24 = cn_var_实例;
  r25 = 期望(r24, 词元类型枚举_标识符);
  r26 = cn_var_节点_0;
  r27 = cn_var_接口名_1;
  类添加实现接口(r26, r27);
  r28 = cn_var_实例;
  r29 = 匹配(r28, 词元类型枚举_逗号);
  r30 = !r29;
  if (r30) goto if_then_1538; else goto if_merge_1539;

  while_exit_1537:
  goto if_merge_1534;

  if_then_1538:
  goto while_exit_1537;
  goto if_merge_1539;

  if_merge_1539:
  goto while_cond_1535;

  while_cond_1540:
  r33 = cn_var_实例;
  r34 = 检查(r33, 词元类型枚举_右大括号);
  r35 = !r34;
  if (r35) goto while_body_1541; else goto while_exit_1542;

  while_body_1541:
  enum 可见性 cn_var_成员可见性_2;
  r36 = cn_var_实例;
  r37 = 解析可见性修饰符(r36);
  cn_var_成员可见性_2 = r37;
  r38 = cn_var_实例;
  r39 = 检查(r38, 词元类型枚举_关键字_函数);
  if (r39) goto if_then_1543; else goto if_else_1544;

  while_exit_1542:
  r54 = cn_var_实例;
  r55 = 期望(r54, 词元类型枚举_右大括号);
  r56 = cn_var_节点_0;
  return r56;

  if_then_1543:
  struct 声明节点* cn_var_方法_3;
  r40 = cn_var_实例;
  r41 = 解析函数声明(r40);
  cn_var_方法_3 = r41;
  r42 = cn_var_方法_3;
  r43 = r42 != 0;
  if (r43) goto if_then_1546; else goto if_merge_1547;

  if_else_1544:
  struct 声明节点* cn_var_字段_4;
  r47 = cn_var_实例;
  r48 = 解析变量声明(r47);
  cn_var_字段_4 = r48;
  r49 = cn_var_字段_4;
  r50 = r49 != 0;
  if (r50) goto if_then_1548; else goto if_merge_1549;

  if_merge_1545:
  goto while_cond_1540;

  if_then_1546:
  r44 = cn_var_成员可见性_2;
  r45 = cn_var_节点_0;
  r46 = cn_var_方法_3;
  类添加方法(r45, r46);
  goto if_merge_1547;

  if_merge_1547:
  goto if_merge_1545;

  if_then_1548:
  r51 = cn_var_成员可见性_2;
  r52 = cn_var_节点_0;
  r53 = cn_var_字段_4;
  类添加字段(r52, r53);
  goto if_merge_1549;

  if_merge_1549:
  goto if_merge_1545;
  return NULL;
}

struct 声明节点* 解析接口声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r13, r19;
  char* r6;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r16;
  struct 声明节点* r17;
  struct 声明节点* r18;
  struct 声明节点* r20;
  struct 声明节点* r21;
  struct 解析器* r22;
  struct 解析器* r23;
  struct 声明节点* r25;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r15;
  _Bool r24;
  struct 词元 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1550; else goto if_merge_1551;

  if_then_1550:
  return 0;
  goto if_merge_1551;

  if_merge_1551:
  struct 声明节点* cn_var_节点_0;
  创建接口声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1552; else goto if_merge_1553;

  if_then_1552:
  return 0;
  goto if_merge_1553;

  if_merge_1553:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  r8 = 期望(r7, 词元类型枚举_标识符);
  r9 = cn_var_实例;
  r10 = 期望(r9, 词元类型枚举_左大括号);
  goto while_cond_1554;

  while_cond_1554:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_右大括号);
  r13 = !r12;
  if (r13) goto while_body_1555; else goto while_exit_1556;

  while_body_1555:
  r14 = cn_var_实例;
  r15 = 检查(r14, 词元类型枚举_关键字_函数);
  if (r15) goto if_then_1557; else goto if_else_1558;

  while_exit_1556:
  r23 = cn_var_实例;
  r24 = 期望(r23, 词元类型枚举_右大括号);
  r25 = cn_var_节点_0;
  return r25;

  if_then_1557:
  struct 声明节点* cn_var_方法_1;
  r16 = cn_var_实例;
  r17 = 解析函数声明(r16);
  cn_var_方法_1 = r17;
  r18 = cn_var_方法_1;
  r19 = r18 != 0;
  if (r19) goto if_then_1560; else goto if_merge_1561;

  if_else_1558:
  r22 = cn_var_实例;
  前进词元(r22);
  goto if_merge_1559;

  if_merge_1559:
  goto while_cond_1554;

  if_then_1560:
  r20 = cn_var_节点_0;
  r21 = cn_var_方法_1;
  接口添加方法(r20, r21);
  goto if_merge_1561;

  if_merge_1561:
  goto if_merge_1559;
  return NULL;
}

struct 声明节点* 解析模块声明(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1562; else goto if_merge_1563;

  if_then_1562:
  return 0;
  goto if_merge_1563;

  if_merge_1563:
  struct 声明节点* cn_var_节点_0;
  创建模块声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1564; else goto if_merge_1565;

  if_then_1564:
  return 0;
  goto if_merge_1565;

  if_merge_1565:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  报告错误(r5, 诊断错误码_语法_预留特性, "模块关键字暂不支持，请使用文件即模块");
  return 0;
}

struct 声明节点* 解析导入声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r14, r23;
  char* r6;
  char* r16;
  char* r20;
  char* r30;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r12;
  struct 解析器* r17;
  struct 声明节点* r19;
  struct 解析器* r21;
  struct 解析器* r24;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 解析器* r31;
  struct 解析器* r33;
  struct 解析器* r35;
  struct 声明节点* r37;
  _Bool r8;
  _Bool r10;
  _Bool r13;
  _Bool r18;
  _Bool r22;
  _Bool r25;
  _Bool r27;
  _Bool r32;
  _Bool r34;
  _Bool r36;
  struct 词元 r5;
  struct 词元 r15;
  struct 词元 r29;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1566; else goto if_merge_1567;

  if_then_1566:
  return 0;
  goto if_merge_1567;

  if_merge_1567:
  struct 声明节点* cn_var_节点_0;
  创建导入声明();
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1568; else goto if_merge_1569;

  if_then_1568:
  return 0;
  goto if_merge_1569;

  if_merge_1569:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  r8 = 期望(r7, 词元类型枚举_标识符);
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_左大括号);
  if (r10) goto if_then_1570; else goto if_merge_1571;

  if_then_1570:
  r11 = cn_var_实例;
  前进词元(r11);
  goto while_cond_1572;

  if_merge_1571:
  r26 = cn_var_实例;
  r27 = 检查(r26, 词元类型枚举_左括号);
  if (r27) goto if_then_1577; else goto if_merge_1578;

  while_cond_1572:
  r12 = cn_var_实例;
  r13 = 检查(r12, 词元类型枚举_右大括号);
  r14 = !r13;
  if (r14) goto while_body_1573; else goto while_exit_1574;

  while_body_1573:
  char* cn_var_成员名_1;
  r15 = cn_var_实例->当前词元;
  r16 = r15.值;
  cn_var_成员名_1 = r16;
  r17 = cn_var_实例;
  r18 = 期望(r17, 词元类型枚举_标识符);
  r19 = cn_var_节点_0;
  r20 = cn_var_成员名_1;
  导入添加成员(r19, r20);
  r21 = cn_var_实例;
  r22 = 匹配(r21, 词元类型枚举_逗号);
  r23 = !r22;
  if (r23) goto if_then_1575; else goto if_merge_1576;

  while_exit_1574:
  r24 = cn_var_实例;
  r25 = 期望(r24, 词元类型枚举_右大括号);
  goto if_merge_1571;

  if_then_1575:
  goto while_exit_1574;
  goto if_merge_1576;

  if_merge_1576:
  goto while_cond_1572;

  if_then_1577:
  r28 = cn_var_实例;
  前进词元(r28);
  r29 = cn_var_实例->当前词元;
  r30 = r29.值;
  r31 = cn_var_实例;
  r32 = 期望(r31, 词元类型枚举_标识符);
  r33 = cn_var_实例;
  r34 = 期望(r33, 词元类型枚举_右括号);
  goto if_merge_1578;

  if_merge_1578:
  r35 = cn_var_实例;
  r36 = 期望(r35, 词元类型枚举_分号);
  r37 = cn_var_节点_0;
  return r37;
}

enum 可见性 解析可见性修饰符(struct 解析器* cn_var_实例) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 解析器* r8;
  struct 解析器* r10;
  _Bool r3;
  _Bool r6;
  _Bool r9;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1579; else goto if_merge_1580;

  if_then_1579:
  return 可见性_可见性_私有;
  goto if_merge_1580;

  if_merge_1580:
  r2 = cn_var_实例;
  r3 = 检查(r2, 词元类型枚举_关键字_公开);
  if (r3) goto if_then_1581; else goto if_merge_1582;

  if_then_1581:
  r4 = cn_var_实例;
  前进词元(r4);
  return 可见性_可见性_公开;
  goto if_merge_1582;

  if_merge_1582:
  r5 = cn_var_实例;
  r6 = 检查(r5, 词元类型枚举_关键字_私有);
  if (r6) goto if_then_1583; else goto if_merge_1584;

  if_then_1583:
  r7 = cn_var_实例;
  前进词元(r7);
  return 可见性_可见性_私有;
  goto if_merge_1584;

  if_merge_1584:
  r8 = cn_var_实例;
  r9 = 检查(r8, 词元类型枚举_关键字_保护);
  if (r9) goto if_then_1585; else goto if_merge_1586;

  if_then_1585:
  r10 = cn_var_实例;
  前进词元(r10);
  return 可见性_可见性_默认;
  goto if_merge_1586;

  if_merge_1586:
  return 可见性_可见性_私有;
}

