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
enum 语法恢复策略 {
    语法恢复策略_同步到关键字 = 4,
    语法恢复策略_同步到大括号 = 3,
    语法恢复策略_同步到分号 = 2,
    语法恢复策略_跳过词元 = 1,
    语法恢复策略_不恢复 = 0
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

// Struct Definitions - 从导入模块
struct 关键字条目;
struct 关键字条目 {
    char* 名称;
    long long 类型;
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

// Forward Declarations - 从导入模块
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
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
void 初始化关键字表(void);
struct 关键字条目* 获取关键字表(void);
long long 关键字总数(void);
_Bool 是关键字字符串(char*);
enum 词元类型枚举 查找关键字(char*);
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
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);

// Global Variables

// Forward Declarations
struct 表达式节点* 解析表达式(struct 解析器*);
struct 表达式节点* 解析赋值表达式(struct 解析器*);
struct 表达式节点* 解析三元表达式(struct 解析器*);
struct 表达式节点* 解析逻辑或表达式(struct 解析器*);
struct 表达式节点* 解析逻辑与表达式(struct 解析器*);
struct 表达式节点* 解析按位或表达式(struct 解析器*);
struct 表达式节点* 解析按位异或表达式(struct 解析器*);
struct 表达式节点* 解析按位与表达式(struct 解析器*);
struct 表达式节点* 解析相等表达式(struct 解析器*);
struct 表达式节点* 解析比较表达式(struct 解析器*);
struct 表达式节点* 解析位移表达式(struct 解析器*);
struct 表达式节点* 解析加减表达式(struct 解析器*);
struct 表达式节点* 解析乘除表达式(struct 解析器*);
struct 表达式节点* 解析一元表达式(struct 解析器*);
struct 表达式节点* 解析后缀表达式(struct 解析器*);
struct 表达式节点* 解析基础表达式(struct 解析器*);
struct 表达式节点* 解析字面量(struct 解析器*);
struct 表达式节点* 解析标识符表达式(struct 解析器*);
struct 表达式节点* 解析括号表达式(struct 解析器*);
struct 表达式列表* 解析函数调用参数(struct 解析器*);
struct 表达式节点* 解析表达式(struct 解析器*);
struct 表达式节点* 解析赋值表达式(struct 解析器*);
struct 表达式节点* 解析三元表达式(struct 解析器*);
struct 表达式节点* 解析逻辑或表达式(struct 解析器*);
struct 表达式节点* 解析逻辑与表达式(struct 解析器*);
struct 表达式节点* 解析按位或表达式(struct 解析器*);
struct 表达式节点* 解析按位异或表达式(struct 解析器*);
struct 表达式节点* 解析按位与表达式(struct 解析器*);
struct 表达式节点* 解析相等表达式(struct 解析器*);
struct 表达式节点* 解析比较表达式(struct 解析器*);
struct 表达式节点* 解析位移表达式(struct 解析器*);
struct 表达式节点* 解析加减表达式(struct 解析器*);
struct 表达式节点* 解析乘除表达式(struct 解析器*);
struct 表达式节点* 解析一元表达式(struct 解析器*);
struct 表达式节点* 解析后缀表达式(struct 解析器*);
struct 表达式节点* 解析基础表达式(struct 解析器*);
struct 表达式节点* 解析字面量(struct 解析器*);
struct 表达式节点* 解析标识符表达式(struct 解析器*);
struct 表达式节点* 解析括号表达式(struct 解析器*);
struct 表达式列表* 解析函数调用参数(struct 解析器*);

struct 表达式节点* 解析表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析赋值表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例);
struct 表达式节点* 解析标识符表达式(struct 解析器* cn_var_实例);
struct 表达式节点* 解析括号表达式(struct 解析器* cn_var_实例);
struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例);
struct 表达式节点* 解析表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 表达式节点* r1;

  entry:
  r0 = cn_var_实例;
  r1 = 解析赋值表达式(r0);
  return r1;
}

struct 表达式节点* 解析赋值表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1022; else goto if_merge_1023;

  if_then_1022:
  return 0;
  goto if_merge_1023;

  if_merge_1023:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析三元表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1024; else goto if_merge_1025;

  if_then_1024:
  return 0;
  goto if_merge_1025;

  if_merge_1025:
  r6 = cn_var_实例;
  r7 = 检查(r6, 51);
  if (r7) goto if_then_1026; else goto if_merge_1027;

  if_then_1026:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析赋值表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_赋值节点_2;
  创建赋值表达式();
  cn_var_赋值节点_2 = /* NONE */;
  r11 = cn_var_赋值节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1028; else goto if_merge_1029;

  if_merge_1027:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1028:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1029;

  if_merge_1029:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_赋值节点_2;
  return r16;
  goto if_merge_1027;
  return NULL;
}

struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r15;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 表达式节点* r9;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r7;
  _Bool r11;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1030; else goto if_merge_1031;

  if_then_1030:
  return 0;
  goto if_merge_1031;

  if_merge_1031:
  struct 表达式节点* cn_var_条件_0;
  r2 = cn_var_实例;
  r3 = 解析逻辑或表达式(r2);
  cn_var_条件_0 = r3;
  r4 = cn_var_条件_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1032; else goto if_merge_1033;

  if_then_1032:
  return 0;
  goto if_merge_1033;

  if_merge_1033:
  r6 = cn_var_实例;
  r7 = 匹配(r6, 79);
  if (r7) goto if_then_1034; else goto if_merge_1035;

  if_then_1034:
  struct 表达式节点* cn_var_真值_1;
  r8 = cn_var_实例;
  r9 = 解析表达式(r8);
  cn_var_真值_1 = r9;
  r10 = cn_var_实例;
  r11 = 期望(r10, 78);
  struct 表达式节点* cn_var_假值_2;
  r12 = cn_var_实例;
  r13 = 解析三元表达式(r12);
  cn_var_假值_2 = r13;
  struct 表达式节点* cn_var_三元节点_3;
  创建三元表达式();
  cn_var_三元节点_3 = /* NONE */;
  r14 = cn_var_三元节点_3;
  r15 = r14 == 0;
  if (r15) goto if_then_1036; else goto if_merge_1037;

  if_merge_1035:
  r21 = cn_var_条件_0;
  return r21;

  if_then_1036:
  r16 = cn_var_条件_0;
  return r16;
  goto if_merge_1037;

  if_merge_1037:
  r17 = cn_var_条件_0;
  r18 = cn_var_真值_1;
  r19 = cn_var_假值_2;
  r20 = cn_var_三元节点_3;
  return r20;
  goto if_merge_1035;
  return NULL;
}

struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1038; else goto if_merge_1039;

  if_then_1038:
  return 0;
  goto if_merge_1039;

  if_merge_1039:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析逻辑与表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1040; else goto if_merge_1041;

  if_then_1040:
  return 0;
  goto if_merge_1041;

  if_merge_1041:
  goto while_cond_1042;

  while_cond_1042:
  r6 = cn_var_实例;
  r7 = 检查(r6, 58);
  if (r7) goto while_body_1043; else goto while_exit_1044;

  while_body_1043:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析逻辑与表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r11 = cn_var_二元节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1045; else goto if_merge_1046;

  while_exit_1044:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1045:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1046;

  if_merge_1046:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_二元节点_2;
  cn_var_左值_0 = r16;
  goto while_cond_1042;
  return NULL;
}

struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1047; else goto if_merge_1048;

  if_then_1047:
  return 0;
  goto if_merge_1048;

  if_merge_1048:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位或表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1049; else goto if_merge_1050;

  if_then_1049:
  return 0;
  goto if_merge_1050;

  if_merge_1050:
  goto while_cond_1051;

  while_cond_1051:
  r6 = cn_var_实例;
  r7 = 检查(r6, 57);
  if (r7) goto while_body_1052; else goto while_exit_1053;

  while_body_1052:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位或表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r11 = cn_var_二元节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1054; else goto if_merge_1055;

  while_exit_1053:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1054:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1055;

  if_merge_1055:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_二元节点_2;
  cn_var_左值_0 = r16;
  goto while_cond_1051;
  return NULL;
}

struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1056; else goto if_merge_1057;

  if_then_1056:
  return 0;
  goto if_merge_1057;

  if_merge_1057:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位异或表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1058; else goto if_merge_1059;

  if_then_1058:
  return 0;
  goto if_merge_1059;

  if_merge_1059:
  goto while_cond_1060;

  while_cond_1060:
  r6 = cn_var_实例;
  r7 = 检查(r6, 61);
  if (r7) goto while_body_1061; else goto while_exit_1062;

  while_body_1061:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位异或表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r11 = cn_var_二元节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1063; else goto if_merge_1064;

  while_exit_1062:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1063:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1064;

  if_merge_1064:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_二元节点_2;
  cn_var_左值_0 = r16;
  goto while_cond_1060;
  return NULL;
}

struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1065; else goto if_merge_1066;

  if_then_1065:
  return 0;
  goto if_merge_1066;

  if_merge_1066:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位与表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1067; else goto if_merge_1068;

  if_then_1067:
  return 0;
  goto if_merge_1068;

  if_merge_1068:
  goto while_cond_1069;

  while_cond_1069:
  r6 = cn_var_实例;
  r7 = 检查(r6, 62);
  if (r7) goto while_body_1070; else goto while_exit_1071;

  while_body_1070:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位与表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r11 = cn_var_二元节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1072; else goto if_merge_1073;

  while_exit_1071:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1072:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1073;

  if_merge_1073:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_二元节点_2;
  cn_var_左值_0 = r16;
  goto while_cond_1069;
  return NULL;
}

struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r12;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1074; else goto if_merge_1075;

  if_then_1074:
  return 0;
  goto if_merge_1075;

  if_merge_1075:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析相等表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1076; else goto if_merge_1077;

  if_then_1076:
  return 0;
  goto if_merge_1077;

  if_merge_1077:
  goto while_cond_1078;

  while_cond_1078:
  r6 = cn_var_实例;
  r7 = 检查(r6, 60);
  if (r7) goto while_body_1079; else goto while_exit_1080;

  while_body_1079:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析相等表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r11 = cn_var_二元节点_2;
  r12 = r11 == 0;
  if (r12) goto if_then_1081; else goto if_merge_1082;

  while_exit_1080:
  r17 = cn_var_左值_0;
  return r17;

  if_then_1081:
  r13 = cn_var_左值_0;
  return r13;
  goto if_merge_1082;

  if_merge_1082:
  r14 = cn_var_左值_0;
  r15 = cn_var_右值_1;
  r16 = cn_var_二元节点_2;
  cn_var_左值_0 = r16;
  goto while_cond_1078;
  return NULL;
}

struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r17;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r22;
  struct 表达式节点* r23;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  enum 二元运算符 r21;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1083; else goto if_merge_1084;

  if_then_1083:
  return 0;
  goto if_merge_1084;

  if_merge_1084:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析比较表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1085; else goto if_merge_1086;

  if_then_1085:
  return 0;
  goto if_merge_1086;

  if_merge_1086:
  goto while_cond_1087;

  while_cond_1087:
  r7 = cn_var_实例;
  r8 = 检查(r7, 50);
  if (r8) goto logic_merge_1091; else goto logic_rhs_1090;

  while_body_1088:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 5;
  r11 = cn_var_实例;
  r12 = 检查(r11, 52);
  if (r12) goto if_then_1092; else goto if_merge_1093;

  while_exit_1089:
  r23 = cn_var_左值_0;
  return r23;

  logic_rhs_1090:
  r9 = cn_var_实例;
  r10 = 检查(r9, 52);
  goto logic_merge_1091;

  logic_merge_1091:
  if (r10) goto while_body_1088; else goto while_exit_1089;

  if_then_1092:
  cn_var_运算符_1 = 6;
  goto if_merge_1093;

  if_merge_1093:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析比较表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r16 = cn_var_二元节点_3;
  r17 = r16 == 0;
  if (r17) goto if_then_1094; else goto if_merge_1095;

  if_then_1094:
  r18 = cn_var_左值_0;
  return r18;
  goto if_merge_1095;

  if_merge_1095:
  r19 = cn_var_左值_0;
  r20 = cn_var_右值_2;
  r21 = cn_var_运算符_1;
  r22 = cn_var_二元节点_3;
  cn_var_左值_0 = r22;
  goto while_cond_1087;
  return NULL;
}

struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r7, r8, r27;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r15;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 解析器* r23;
  struct 解析器* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r26;
  struct 表达式节点* r28;
  struct 表达式节点* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r32;
  struct 表达式节点* r33;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r20;
  _Bool r22;
  enum 二元运算符 r31;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1096; else goto if_merge_1097;

  if_then_1096:
  return 0;
  goto if_merge_1097;

  if_merge_1097:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析位移表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1098; else goto if_merge_1099;

  if_then_1098:
  return 0;
  goto if_merge_1099;

  if_merge_1099:
  goto while_cond_1100;

  while_cond_1100:
  r9 = cn_var_实例;
  r10 = 检查(r9, 53);
  if (r10) goto logic_merge_1108; else goto logic_rhs_1107;

  while_body_1101:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 7;
  r17 = cn_var_实例;
  r18 = 检查(r17, 54);
  if (r18) goto if_then_1109; else goto if_else_1110;

  while_exit_1102:
  r33 = cn_var_左值_0;
  return r33;

  logic_rhs_1103:
  r15 = cn_var_实例;
  r16 = 检查(r15, 56);
  goto logic_merge_1104;

  logic_merge_1104:
  if (r16) goto while_body_1101; else goto while_exit_1102;

  logic_rhs_1105:
  r13 = cn_var_实例;
  r14 = 检查(r13, 55);
  goto logic_merge_1106;

  logic_merge_1106:
  if (r14) goto logic_merge_1104; else goto logic_rhs_1103;

  logic_rhs_1107:
  r11 = cn_var_实例;
  r12 = 检查(r11, 54);
  goto logic_merge_1108;

  logic_merge_1108:
  if (r12) goto logic_merge_1106; else goto logic_rhs_1105;

  if_then_1109:
  cn_var_运算符_1 = 9;
  goto if_merge_1111;

  if_else_1110:
  r19 = cn_var_实例;
  r20 = 检查(r19, 55);
  if (r20) goto if_then_1112; else goto if_else_1113;

  if_merge_1111:
  r23 = cn_var_实例;
  前进词元(r23);
  struct 表达式节点* cn_var_右值_2;
  r24 = cn_var_实例;
  r25 = 解析位移表达式(r24);
  cn_var_右值_2 = r25;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r26 = cn_var_二元节点_3;
  r27 = r26 == 0;
  if (r27) goto if_then_1117; else goto if_merge_1118;

  if_then_1112:
  cn_var_运算符_1 = 8;
  goto if_merge_1114;

  if_else_1113:
  r21 = cn_var_实例;
  r22 = 检查(r21, 56);
  if (r22) goto if_then_1115; else goto if_merge_1116;

  if_merge_1114:
  goto if_merge_1111;

  if_then_1115:
  cn_var_运算符_1 = 10;
  goto if_merge_1116;

  if_merge_1116:
  goto if_merge_1114;

  if_then_1117:
  r28 = cn_var_左值_0;
  return r28;
  goto if_merge_1118;

  if_merge_1118:
  r29 = cn_var_左值_0;
  r30 = cn_var_右值_2;
  r31 = cn_var_运算符_1;
  r32 = cn_var_二元节点_3;
  cn_var_左值_0 = r32;
  goto while_cond_1100;
  return NULL;
}

struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r17;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r22;
  struct 表达式节点* r23;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  enum 二元运算符 r21;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1119; else goto if_merge_1120;

  if_then_1119:
  return 0;
  goto if_merge_1120;

  if_merge_1120:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析加减表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1121; else goto if_merge_1122;

  if_then_1121:
  return 0;
  goto if_merge_1122;

  if_merge_1122:
  goto while_cond_1123;

  while_cond_1123:
  r7 = cn_var_实例;
  r8 = 检查(r7, 64);
  if (r8) goto logic_merge_1127; else goto logic_rhs_1126;

  while_body_1124:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 14;
  r11 = cn_var_实例;
  r12 = 检查(r11, 65);
  if (r12) goto if_then_1128; else goto if_merge_1129;

  while_exit_1125:
  r23 = cn_var_左值_0;
  return r23;

  logic_rhs_1126:
  r9 = cn_var_实例;
  r10 = 检查(r9, 65);
  goto logic_merge_1127;

  logic_merge_1127:
  if (r10) goto while_body_1124; else goto while_exit_1125;

  if_then_1128:
  cn_var_运算符_1 = 15;
  goto if_merge_1129;

  if_merge_1129:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析加减表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r16 = cn_var_二元节点_3;
  r17 = r16 == 0;
  if (r17) goto if_then_1130; else goto if_merge_1131;

  if_then_1130:
  r18 = cn_var_左值_0;
  return r18;
  goto if_merge_1131;

  if_merge_1131:
  r19 = cn_var_左值_0;
  r20 = cn_var_右值_2;
  r21 = cn_var_运算符_1;
  r22 = cn_var_二元节点_3;
  cn_var_左值_0 = r22;
  goto while_cond_1123;
  return NULL;
}

struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r17;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r22;
  struct 表达式节点* r23;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  enum 二元运算符 r21;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1132; else goto if_merge_1133;

  if_then_1132:
  return 0;
  goto if_merge_1133;

  if_merge_1133:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析乘除表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1134; else goto if_merge_1135;

  if_then_1134:
  return 0;
  goto if_merge_1135;

  if_merge_1135:
  goto while_cond_1136;

  while_cond_1136:
  r7 = cn_var_实例;
  r8 = 检查(r7, 45);
  if (r8) goto logic_merge_1140; else goto logic_rhs_1139;

  while_body_1137:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = /* NONE */;
  r11 = cn_var_实例;
  r12 = 检查(r11, 46);
  if (r12) goto if_then_1141; else goto if_merge_1142;

  while_exit_1138:
  r23 = cn_var_左值_0;
  return r23;

  logic_rhs_1139:
  r9 = cn_var_实例;
  r10 = 检查(r9, 46);
  goto logic_merge_1140;

  logic_merge_1140:
  if (r10) goto while_body_1137; else goto while_exit_1138;

  if_then_1141:
  cn_var_运算符_1 = /* NONE */;
  goto if_merge_1142;

  if_merge_1142:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析乘除表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r16 = cn_var_二元节点_3;
  r17 = r16 == 0;
  if (r17) goto if_then_1143; else goto if_merge_1144;

  if_then_1143:
  r18 = cn_var_左值_0;
  return r18;
  goto if_merge_1144;

  if_merge_1144:
  r19 = cn_var_左值_0;
  r20 = cn_var_右值_2;
  r21 = cn_var_运算符_1;
  r22 = cn_var_二元节点_3;
  cn_var_左值_0 = r22;
  goto while_cond_1136;
  return NULL;
}

struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r7, r22;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r14;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 解析器* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  enum 二元运算符 r26;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1145; else goto if_merge_1146;

  if_then_1145:
  return 0;
  goto if_merge_1146;

  if_merge_1146:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析一元表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1147; else goto if_merge_1148;

  if_then_1147:
  return 0;
  goto if_merge_1148;

  if_merge_1148:
  goto while_cond_1149;

  while_cond_1149:
  r8 = cn_var_实例;
  r9 = 检查(r8, 47);
  if (r9) goto logic_merge_1155; else goto logic_rhs_1154;

  while_body_1150:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = /* NONE */;
  r14 = cn_var_实例;
  r15 = 检查(r14, 48);
  if (r15) goto if_then_1156; else goto if_else_1157;

  while_exit_1151:
  r28 = cn_var_左值_0;
  return r28;

  logic_rhs_1152:
  r12 = cn_var_实例;
  r13 = 检查(r12, 49);
  goto logic_merge_1153;

  logic_merge_1153:
  if (r13) goto while_body_1150; else goto while_exit_1151;

  logic_rhs_1154:
  r10 = cn_var_实例;
  r11 = 检查(r10, 48);
  goto logic_merge_1155;

  logic_merge_1155:
  if (r11) goto logic_merge_1153; else goto logic_rhs_1152;

  if_then_1156:
  cn_var_运算符_1 = /* NONE */;
  goto if_merge_1158;

  if_else_1157:
  r16 = cn_var_实例;
  r17 = 检查(r16, 49);
  if (r17) goto if_then_1159; else goto if_merge_1160;

  if_merge_1158:
  r18 = cn_var_实例;
  前进词元(r18);
  struct 表达式节点* cn_var_右值_2;
  r19 = cn_var_实例;
  r20 = 解析一元表达式(r19);
  cn_var_右值_2 = r20;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r21 = cn_var_二元节点_3;
  r22 = r21 == 0;
  if (r22) goto if_then_1161; else goto if_merge_1162;

  if_then_1159:
  cn_var_运算符_1 = 4;
  goto if_merge_1160;

  if_merge_1160:
  goto if_merge_1158;

  if_then_1161:
  r23 = cn_var_左值_0;
  return r23;
  goto if_merge_1162;

  if_merge_1162:
  r24 = cn_var_左值_0;
  r25 = cn_var_右值_2;
  r26 = cn_var_运算符_1;
  r27 = cn_var_二元节点_3;
  cn_var_左值_0 = r27;
  goto while_cond_1149;
  return NULL;
}

struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r5, r6, r7, r38;
  struct 解析器* r0;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r14;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 解析器* r20;
  struct 解析器* r22;
  struct 解析器* r24;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 解析器* r30;
  struct 解析器* r32;
  struct 解析器* r34;
  struct 解析器* r35;
  struct 表达式节点* r36;
  struct 表达式节点* r37;
  struct 表达式节点* r39;
  struct 表达式节点* r40;
  struct 表达式节点* r42;
  struct 解析器* r43;
  struct 表达式节点* r44;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  _Bool r21;
  _Bool r23;
  _Bool r25;
  _Bool r27;
  _Bool r29;
  _Bool r31;
  _Bool r33;
  enum 一元运算符 r41;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1163; else goto if_merge_1164;

  if_then_1163:
  return 0;
  goto if_merge_1164;

  if_merge_1164:
  r8 = cn_var_实例;
  r9 = 检查(r8, 59);
  if (r9) goto logic_merge_1178; else goto logic_rhs_1177;

  if_then_1165:
  enum 一元运算符 cn_var_运算符_0;
  cn_var_运算符_0 = 0;
  r22 = cn_var_实例;
  r23 = 检查(r22, 46);
  if (r23) goto if_then_1179; else goto if_else_1180;

  if_merge_1166:
  r43 = cn_var_实例;
  r44 = 解析后缀表达式(r43);
  return r44;

  logic_rhs_1167:
  r20 = cn_var_实例;
  r21 = 检查(r20, 67);
  goto logic_merge_1168;

  logic_merge_1168:
  if (r21) goto if_then_1165; else goto if_merge_1166;

  logic_rhs_1169:
  r18 = cn_var_实例;
  r19 = 检查(r18, 66);
  goto logic_merge_1170;

  logic_merge_1170:
  if (r19) goto logic_merge_1168; else goto logic_rhs_1167;

  logic_rhs_1171:
  r16 = cn_var_实例;
  r17 = 检查(r16, 60);
  goto logic_merge_1172;

  logic_merge_1172:
  if (r17) goto logic_merge_1170; else goto logic_rhs_1169;

  logic_rhs_1173:
  r14 = cn_var_实例;
  r15 = 检查(r14, 47);
  goto logic_merge_1174;

  logic_merge_1174:
  if (r15) goto logic_merge_1172; else goto logic_rhs_1171;

  logic_rhs_1175:
  r12 = cn_var_实例;
  r13 = 检查(r12, 63);
  goto logic_merge_1176;

  logic_merge_1176:
  if (r13) goto logic_merge_1174; else goto logic_rhs_1173;

  logic_rhs_1177:
  r10 = cn_var_实例;
  r11 = 检查(r10, 46);
  goto logic_merge_1178;

  logic_merge_1178:
  if (r11) goto logic_merge_1176; else goto logic_rhs_1175;

  if_then_1179:
  cn_var_运算符_0 = /* NONE */;
  goto if_merge_1181;

  if_else_1180:
  r24 = cn_var_实例;
  r25 = 检查(r24, 63);
  if (r25) goto if_then_1182; else goto if_else_1183;

  if_merge_1181:
  r34 = cn_var_实例;
  前进词元(r34);
  struct 表达式节点* cn_var_操作数_1;
  r35 = cn_var_实例;
  r36 = 解析一元表达式(r35);
  cn_var_操作数_1 = r36;
  struct 表达式节点* cn_var_一元节点_2;
  创建一元表达式();
  cn_var_一元节点_2 = /* NONE */;
  r37 = cn_var_一元节点_2;
  r38 = r37 == 0;
  if (r38) goto if_then_1196; else goto if_merge_1197;

  if_then_1182:
  cn_var_运算符_0 = /* NONE */;
  goto if_merge_1184;

  if_else_1183:
  r26 = cn_var_实例;
  r27 = 检查(r26, 47);
  if (r27) goto if_then_1185; else goto if_else_1186;

  if_merge_1184:
  goto if_merge_1181;

  if_then_1185:
  cn_var_运算符_0 = 3;
  goto if_merge_1187;

  if_else_1186:
  r28 = cn_var_实例;
  r29 = 检查(r28, 60);
  if (r29) goto if_then_1188; else goto if_else_1189;

  if_merge_1187:
  goto if_merge_1184;

  if_then_1188:
  cn_var_运算符_0 = 2;
  goto if_merge_1190;

  if_else_1189:
  r30 = cn_var_实例;
  r31 = 检查(r30, 66);
  if (r31) goto if_then_1191; else goto if_else_1192;

  if_merge_1190:
  goto if_merge_1187;

  if_then_1191:
  cn_var_运算符_0 = /* NONE */;
  goto if_merge_1193;

  if_else_1192:
  r32 = cn_var_实例;
  r33 = 检查(r32, 67);
  if (r33) goto if_then_1194; else goto if_merge_1195;

  if_merge_1193:
  goto if_merge_1190;

  if_then_1194:
  cn_var_运算符_0 = /* NONE */;
  goto if_merge_1195;

  if_merge_1195:
  goto if_merge_1193;

  if_then_1196:
  r39 = cn_var_操作数_1;
  return r39;
  goto if_merge_1197;

  if_merge_1197:
  r40 = cn_var_操作数_1;
  r41 = cn_var_运算符_0;
  r42 = cn_var_一元节点_2;
  return r42;
  goto if_merge_1166;
  return NULL;
}

struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r14, r28, r42, r55, r64, r72;
  char* r24;
  char* r31;
  char* r38;
  char* r45;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式列表* r10;
  struct 解析器* r11;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r18;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r32;
  struct 解析器* r33;
  struct 解析器* r35;
  struct 解析器* r36;
  struct 解析器* r39;
  struct 表达式节点* r41;
  struct 表达式节点* r43;
  struct 表达式节点* r44;
  struct 表达式节点* r46;
  struct 解析器* r47;
  struct 解析器* r49;
  struct 解析器* r50;
  struct 表达式节点* r51;
  struct 解析器* r52;
  struct 表达式节点* r54;
  struct 表达式节点* r56;
  struct 表达式节点* r57;
  struct 表达式节点* r58;
  struct 表达式节点* r59;
  struct 解析器* r60;
  struct 解析器* r62;
  struct 表达式节点* r63;
  struct 表达式节点* r65;
  struct 表达式节点* r66;
  struct 表达式节点* r67;
  struct 解析器* r68;
  struct 解析器* r70;
  struct 表达式节点* r71;
  struct 表达式节点* r73;
  struct 表达式节点* r74;
  struct 表达式节点* r75;
  struct 表达式节点* r76;
  _Bool r7;
  _Bool r12;
  _Bool r20;
  _Bool r26;
  _Bool r34;
  _Bool r40;
  _Bool r48;
  _Bool r53;
  _Bool r61;
  _Bool r69;
  struct 参数 r17;
  struct 词元 r23;
  struct 词元 r37;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1198; else goto if_merge_1199;

  if_then_1198:
  return 0;
  goto if_merge_1199;

  if_merge_1199:
  struct 表达式节点* cn_var_表达式_0;
  r2 = cn_var_实例;
  r3 = 解析基础表达式(r2);
  cn_var_表达式_0 = r3;
  r4 = cn_var_表达式_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1200; else goto if_merge_1201;

  if_then_1200:
  return 0;
  goto if_merge_1201;

  if_merge_1201:
  goto while_cond_1202;

  while_cond_1202:
  if (1) goto while_body_1203; else goto while_exit_1204;

  while_body_1203:
  r6 = cn_var_实例;
  r7 = 检查(r6, 69);
  if (r7) goto if_then_1205; else goto if_else_1206;

  while_exit_1204:
  r76 = cn_var_表达式_0;
  return r76;

  if_then_1205:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式列表* cn_var_参数_1;
  r9 = cn_var_实例;
  r10 = 解析函数调用参数(r9);
  cn_var_参数_1 = r10;
  r11 = cn_var_实例;
  r12 = 期望(r11, 70);
  struct 表达式节点* cn_var_调用节点_2;
  创建函数调用表达式();
  cn_var_调用节点_2 = /* NONE */;
  r13 = cn_var_调用节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1208; else goto if_merge_1209;

  if_else_1206:
  r19 = cn_var_实例;
  r20 = 检查(r19, 77);
  if (r20) goto if_then_1210; else goto if_else_1211;

  if_merge_1207:
  goto while_cond_1202;

  if_then_1208:
  r15 = cn_var_表达式_0;
  return r15;
  goto if_merge_1209;

  if_merge_1209:
  r16 = cn_var_表达式_0;
  r17 = cn_var_参数_1;
  r18 = cn_var_调用节点_2;
  cn_var_表达式_0 = r18;
  goto if_merge_1207;

  if_then_1210:
  r21 = cn_var_实例;
  前进词元(r21);
  char* cn_var_成员名_3;
  r22 = cn_var_实例;
  r23 = r22->当前词元;
  r24 = r23.值;
  cn_var_成员名_3 = r24;
  r25 = cn_var_实例;
  r26 = 期望(r25, 40);
  struct 表达式节点* cn_var_成员节点_4;
  创建成员访问表达式();
  cn_var_成员节点_4 = /* NONE */;
  r27 = cn_var_成员节点_4;
  r28 = r27 == 0;
  if (r28) goto if_then_1213; else goto if_merge_1214;

  if_else_1211:
  r33 = cn_var_实例;
  r34 = 检查(r33, 68);
  if (r34) goto if_then_1215; else goto if_else_1216;

  if_merge_1212:
  goto if_merge_1207;

  if_then_1213:
  r29 = cn_var_表达式_0;
  return r29;
  goto if_merge_1214;

  if_merge_1214:
  r30 = cn_var_表达式_0;
  r31 = cn_var_成员名_3;
  r32 = cn_var_成员节点_4;
  cn_var_表达式_0 = r32;
  goto if_merge_1212;

  if_then_1215:
  r35 = cn_var_实例;
  前进词元(r35);
  char* cn_var_成员名_5;
  r36 = cn_var_实例;
  r37 = r36->当前词元;
  r38 = r37.值;
  cn_var_成员名_5 = r38;
  r39 = cn_var_实例;
  r40 = 期望(r39, 40);
  struct 表达式节点* cn_var_成员节点_6;
  创建成员访问表达式();
  cn_var_成员节点_6 = /* NONE */;
  r41 = cn_var_成员节点_6;
  r42 = r41 == 0;
  if (r42) goto if_then_1218; else goto if_merge_1219;

  if_else_1216:
  r47 = cn_var_实例;
  r48 = 检查(r47, 73);
  if (r48) goto if_then_1220; else goto if_else_1221;

  if_merge_1217:
  goto if_merge_1212;

  if_then_1218:
  r43 = cn_var_表达式_0;
  return r43;
  goto if_merge_1219;

  if_merge_1219:
  r44 = cn_var_表达式_0;
  r45 = cn_var_成员名_5;
  r46 = cn_var_成员节点_6;
  cn_var_表达式_0 = r46;
  goto if_merge_1217;

  if_then_1220:
  r49 = cn_var_实例;
  前进词元(r49);
  struct 表达式节点* cn_var_索引_7;
  r50 = cn_var_实例;
  r51 = 解析表达式(r50);
  cn_var_索引_7 = r51;
  r52 = cn_var_实例;
  r53 = 期望(r52, 74);
  struct 表达式节点* cn_var_数组节点_8;
  创建数组访问表达式();
  cn_var_数组节点_8 = /* NONE */;
  r54 = cn_var_数组节点_8;
  r55 = r54 == 0;
  if (r55) goto if_then_1223; else goto if_merge_1224;

  if_else_1221:
  r60 = cn_var_实例;
  r61 = 检查(r60, 66);
  if (r61) goto if_then_1225; else goto if_else_1226;

  if_merge_1222:
  goto if_merge_1217;

  if_then_1223:
  r56 = cn_var_表达式_0;
  return r56;
  goto if_merge_1224;

  if_merge_1224:
  r57 = cn_var_表达式_0;
  r58 = cn_var_索引_7;
  r59 = cn_var_数组节点_8;
  cn_var_表达式_0 = r59;
  goto if_merge_1222;

  if_then_1225:
  r62 = cn_var_实例;
  前进词元(r62);
  struct 表达式节点* cn_var_一元节点_9;
  创建一元表达式();
  cn_var_一元节点_9 = /* NONE */;
  r63 = cn_var_一元节点_9;
  r64 = r63 == 0;
  if (r64) goto if_then_1228; else goto if_merge_1229;

  if_else_1226:
  r68 = cn_var_实例;
  r69 = 检查(r68, 67);
  if (r69) goto if_then_1230; else goto if_else_1231;

  if_merge_1227:
  goto if_merge_1222;

  if_then_1228:
  r65 = cn_var_表达式_0;
  return r65;
  goto if_merge_1229;

  if_merge_1229:
  r66 = cn_var_表达式_0;
  r67 = cn_var_一元节点_9;
  cn_var_表达式_0 = r67;
  goto if_merge_1227;

  if_then_1230:
  r70 = cn_var_实例;
  前进词元(r70);
  struct 表达式节点* cn_var_一元节点_10;
  创建一元表达式();
  cn_var_一元节点_10 = /* NONE */;
  r71 = cn_var_一元节点_10;
  r72 = r71 == 0;
  if (r72) goto if_then_1233; else goto if_merge_1234;

  if_else_1231:
  goto while_exit_1204;
  goto if_merge_1232;

  if_merge_1232:
  goto if_merge_1227;

  if_then_1233:
  r73 = cn_var_表达式_0;
  return r73;
  goto if_merge_1234;

  if_merge_1234:
  r74 = cn_var_表达式_0;
  r75 = cn_var_一元节点_10;
  cn_var_表达式_0 = r75;
  goto if_merge_1232;
  return NULL;
}

struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r15;
  char* r43;
  char* r46;
  struct 解析器* r0;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 表达式节点* r14;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 解析器* r20;
  struct 表达式节点* r21;
  struct 解析器* r22;
  struct 解析器* r24;
  struct 解析器* r25;
  struct 解析器* r27;
  struct 表达式节点* r28;
  struct 解析器* r29;
  struct 解析器* r31;
  struct 表达式节点* r32;
  struct 解析器* r33;
  struct 解析器* r35;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 解析器* r39;
  struct 解析器* r41;
  struct 解析器* r44;
  struct 解析器* r47;
  _Bool r6;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r17;
  _Bool r19;
  _Bool r23;
  _Bool r26;
  _Bool r30;
  _Bool r34;
  _Bool r37;
  _Bool r40;
  _Bool r45;
  struct 词元 r42;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1235; else goto if_merge_1236;

  if_then_1235:
  return 0;
  goto if_merge_1236;

  if_merge_1236:
  r5 = cn_var_实例;
  r6 = 检查(r5, 41);
  if (r6) goto logic_merge_1244; else goto logic_rhs_1243;

  if_then_1237:
  r13 = cn_var_实例;
  r14 = 解析字面量(r13);
  return r14;
  goto if_merge_1238;

  if_merge_1238:
  r16 = cn_var_实例;
  r17 = 检查(r16, 24);
  if (r17) goto logic_merge_1248; else goto logic_rhs_1247;

  logic_rhs_1239:
  r11 = cn_var_实例;
  r12 = 检查(r11, 44);
  goto logic_merge_1240;

  logic_merge_1240:
  if (r12) goto if_then_1237; else goto if_merge_1238;

  logic_rhs_1241:
  r9 = cn_var_实例;
  r10 = 检查(r9, 43);
  goto logic_merge_1242;

  logic_merge_1242:
  if (r10) goto logic_merge_1240; else goto logic_rhs_1239;

  logic_rhs_1243:
  r7 = cn_var_实例;
  r8 = 检查(r7, 42);
  goto logic_merge_1244;

  logic_merge_1244:
  if (r8) goto logic_merge_1242; else goto logic_rhs_1241;

  if_then_1245:
  r20 = cn_var_实例;
  r21 = 解析字面量(r20);
  return r21;
  goto if_merge_1246;

  if_merge_1246:
  r22 = cn_var_实例;
  r23 = 检查(r22, 26);
  if (r23) goto if_then_1249; else goto if_merge_1250;

  logic_rhs_1247:
  r18 = cn_var_实例;
  r19 = 检查(r18, 25);
  goto logic_merge_1248;

  logic_merge_1248:
  if (r19) goto if_then_1245; else goto if_merge_1246;

  if_then_1249:
  r24 = cn_var_实例;
  前进词元(r24);
  创建空值表达式();
  return;
  goto if_merge_1250;

  if_merge_1250:
  r25 = cn_var_实例;
  r26 = 检查(r25, 40);
  if (r26) goto if_then_1251; else goto if_merge_1252;

  if_then_1251:
  r27 = cn_var_实例;
  r28 = 解析标识符表达式(r27);
  return r28;
  goto if_merge_1252;

  if_merge_1252:
  r29 = cn_var_实例;
  r30 = 检查(r29, 69);
  if (r30) goto if_then_1253; else goto if_merge_1254;

  if_then_1253:
  r31 = cn_var_实例;
  r32 = 解析括号表达式(r31);
  return r32;
  goto if_merge_1254;

  if_merge_1254:
  r33 = cn_var_实例;
  r34 = 检查(r33, 34);
  if (r34) goto if_then_1255; else goto if_merge_1256;

  if_then_1255:
  r35 = cn_var_实例;
  前进词元(r35);
  创建自身表达式();
  return;
  goto if_merge_1256;

  if_merge_1256:
  r36 = cn_var_实例;
  r37 = 检查(r36, 35);
  if (r37) goto if_then_1257; else goto if_merge_1258;

  if_then_1257:
  r38 = cn_var_实例;
  前进词元(r38);
  r39 = cn_var_实例;
  r40 = 期望(r39, 77);
  char* cn_var_成员名_0;
  r41 = cn_var_实例;
  r42 = r41->当前词元;
  r43 = r42.值;
  cn_var_成员名_0 = r43;
  r44 = cn_var_实例;
  r45 = 期望(r44, 40);
  r46 = cn_var_成员名_0;
  创建基类访问表达式(r46);
  return;
  goto if_merge_1258;

  if_merge_1258:
  r47 = cn_var_实例;
  报告错误(r47, 17, "期望表达式");
  return 0;
}

struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例) {
  long long r1, r3, r13, r16;
  char* r22;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 解析器* r4;
  struct 解析器* r14;
  struct 表达式节点* r15;
  struct 解析器* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 表达式节点* r24;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  struct 词元 r5;
  struct 词元 r21;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1259; else goto if_merge_1260;

  if_then_1259:
  return 0;
  goto if_merge_1260;

  if_merge_1260:
  struct 表达式节点* cn_var_节点_0;
  创建字面量表达式();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1261; else goto if_merge_1262;

  if_then_1261:
  return 0;
  goto if_merge_1262;

  if_merge_1262:
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.类型;
  r7 = r6 == 41;
  if (r7) goto case_body_1264; else goto switch_check_1271;

  switch_check_1271:
  r8 = r6 == 42;
  if (r8) goto case_body_1265; else goto switch_check_1272;

  switch_check_1272:
  r9 = r6 == 43;
  if (r9) goto case_body_1266; else goto switch_check_1273;

  switch_check_1273:
  r10 = r6 == 44;
  if (r10) goto case_body_1267; else goto switch_check_1274;

  switch_check_1274:
  r11 = r6 == 24;
  if (r11) goto case_body_1268; else goto switch_check_1275;

  switch_check_1275:
  r12 = r6 == 25;
  if (r12) goto case_body_1269; else goto case_default_1270;

  case_body_1264:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1265:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1266:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1267:
  goto switch_merge_1263;
  goto switch_merge_1263;

  case_body_1268:
  r13 = cn_var_布尔字面量;
  r14 = cn_var_实例;
  前进词元(r14);
  r15 = cn_var_节点_0;
  return r15;
  goto switch_merge_1263;

  case_body_1269:
  r16 = cn_var_布尔字面量;
  r17 = cn_var_实例;
  前进词元(r17);
  r18 = cn_var_节点_0;
  return r18;
  goto switch_merge_1263;

  case_default_1270:
  r19 = cn_var_节点_0;
  释放表达式节点(r19);
  return 0;
  goto switch_merge_1263;

  switch_merge_1263:
  r20 = cn_var_实例;
  r21 = r20->当前词元;
  r22 = r21.值;
  r23 = cn_var_实例;
  前进词元(r23);
  r24 = cn_var_节点_0;
  return r24;
}

struct 表达式节点* 解析标识符表达式(struct 解析器* cn_var_实例) {
  long long r1, r3;
  char* r6;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 词元 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1276; else goto if_merge_1277;

  if_then_1276:
  return 0;
  goto if_merge_1277;

  if_merge_1277:
  struct 表达式节点* cn_var_节点_0;
  创建标识符表达式();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1278; else goto if_merge_1279;

  if_then_1278:
  return 0;
  goto if_merge_1279;

  if_merge_1279:
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  前进词元(r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 表达式节点* 解析括号表达式(struct 解析器* cn_var_实例) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r4;
  struct 表达式节点* r5;
  struct 解析器* r6;
  struct 表达式节点* r8;
  _Bool r3;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1280; else goto if_merge_1281;

  if_then_1280:
  return 0;
  goto if_merge_1281;

  if_merge_1281:
  r2 = cn_var_实例;
  r3 = 期望(r2, 69);
  struct 表达式节点* cn_var_表达式_0;
  r4 = cn_var_实例;
  r5 = 解析表达式(r4);
  cn_var_表达式_0 = r5;
  r6 = cn_var_实例;
  r7 = 期望(r6, 70);
  r8 = cn_var_表达式_0;
  return r8;
}

struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r10, r15;
  struct 解析器* r0;
  struct 表达式列表* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 表达式列表* r11;
  struct 解析器* r13;
  struct 表达式列表* r16;
  _Bool r5;
  _Bool r14;
  struct 参数 r9;
  struct 参数 r12;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1282; else goto if_merge_1283;

  if_then_1282:
  return 0;
  goto if_merge_1283;

  if_merge_1283:
  struct 表达式列表* cn_var_列表_0;
  创建表达式列表();
  cn_var_列表_0 = /* NONE */;
  r2 = cn_var_列表_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1284; else goto if_merge_1285;

  if_then_1284:
  return 0;
  goto if_merge_1285;

  if_merge_1285:
  goto while_cond_1286;

  while_cond_1286:
  r4 = cn_var_实例;
  r5 = 检查(r4, 70);
  r6 = !r5;
  if (r6) goto while_body_1287; else goto while_exit_1288;

  while_body_1287:
  struct 表达式节点* cn_var_参数_1;
  r7 = cn_var_实例;
  r8 = 解析表达式(r7);
  cn_var_参数_1 = r8;
  r9 = cn_var_参数_1;
  r10 = r9 != 0;
  if (r10) goto if_then_1289; else goto if_merge_1290;

  while_exit_1288:
  r16 = cn_var_列表_0;
  return r16;

  if_then_1289:
  r11 = cn_var_列表_0;
  r12 = cn_var_参数_1;
  表达式列表添加(r11, r12);
  goto if_merge_1290;

  if_merge_1290:
  r13 = cn_var_实例;
  r14 = 匹配(r13, 76);
  r15 = !r14;
  if (r15) goto if_then_1291; else goto if_merge_1292;

  if_then_1291:
  goto while_exit_1288;
  goto if_merge_1292;

  if_merge_1292:
  goto while_cond_1286;
  return NULL;
}

