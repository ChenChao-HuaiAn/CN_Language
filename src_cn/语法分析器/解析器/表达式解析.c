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
struct 类型节点* 解析函数类型(struct 解析器*);
struct 类型节点* 解析数组类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析指针类型(struct 解析器*, struct 类型节点*);
struct 类型节点* 解析基础类型(struct 解析器*);
struct 类型节点* 解析类型(struct 解析器*);
extern struct 源位置 cn_var_空源位置;
extern long long cn_var_语法错误恢复上下文大小;
extern long long cn_var_默认最大恢复次数;
extern long long cn_var_默认最大连续错误;
extern long long cn_var_解析器大小;
extern long long cn_var_最大错误恢复次数;
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
extern long long cn_var_最大标识符长度;
extern long long cn_var_最大字符串长度;
extern long long cn_var_最大数字长度;
extern long long cn_var_扫描器大小;

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
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析三元表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return 0;
  goto if_merge_3;

  if_merge_3:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_赋值);
  if (r7) goto if_then_4; else goto if_merge_5;

  if_then_4:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析赋值表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_赋值节点_2;
  r11 = 创建赋值表达式();
  cn_var_赋值节点_2 = r11;
  r12 = cn_var_赋值节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_6; else goto if_merge_7;

  if_merge_5:
  r18 = cn_var_左值_0;
  return r18;

  if_then_6:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_7;

  if_merge_7:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_赋值节点_2;
  return r17;
  goto if_merge_5;
  return NULL;
}

struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例) {
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
  struct 表达式节点* r15;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r22;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r11;
  _Bool r16;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_8; else goto if_merge_9;

  if_then_8:
  return 0;
  goto if_merge_9;

  if_merge_9:
  struct 表达式节点* cn_var_条件_0;
  r2 = cn_var_实例;
  r3 = 解析逻辑或表达式(r2);
  cn_var_条件_0 = r3;
  r4 = cn_var_条件_0;
  r5 = r4 == 0;
  if (r5) goto if_then_10; else goto if_merge_11;

  if_then_10:
  return 0;
  goto if_merge_11;

  if_merge_11:
  r6 = cn_var_实例;
  r7 = 匹配(r6, 词元类型枚举_问号);
  if (r7) goto if_then_12; else goto if_merge_13;

  if_then_12:
  struct 表达式节点* cn_var_真值_1;
  r8 = cn_var_实例;
  r9 = 解析表达式(r8);
  cn_var_真值_1 = r9;
  r10 = cn_var_实例;
  r11 = 期望(r10, 词元类型枚举_冒号);
  struct 表达式节点* cn_var_假值_2;
  r12 = cn_var_实例;
  r13 = 解析三元表达式(r12);
  cn_var_假值_2 = r13;
  struct 表达式节点* cn_var_三元节点_3;
  r14 = 创建三元表达式();
  cn_var_三元节点_3 = r14;
  r15 = cn_var_三元节点_3;
  r16 = r15 == 0;
  if (r16) goto if_then_14; else goto if_merge_15;

  if_merge_13:
  r22 = cn_var_条件_0;
  return r22;

  if_then_14:
  r17 = cn_var_条件_0;
  return r17;
  goto if_merge_15;

  if_merge_15:
  r18 = cn_var_条件_0;
  r19 = cn_var_真值_1;
  r20 = cn_var_假值_2;
  r21 = cn_var_三元节点_3;
  return r21;
  goto if_merge_13;
  return NULL;
}

struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_16; else goto if_merge_17;

  if_then_16:
  return 0;
  goto if_merge_17;

  if_merge_17:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析逻辑与表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_18; else goto if_merge_19;

  if_then_18:
  return 0;
  goto if_merge_19;

  if_merge_19:
  goto while_cond_20;

  while_cond_20:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_逻辑或);
  if (r7) goto while_body_21; else goto while_exit_22;

  while_body_21:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析逻辑与表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  r11 = 创建二元表达式();
  cn_var_二元节点_2 = r11;
  r12 = cn_var_二元节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_23; else goto if_merge_24;

  while_exit_22:
  r18 = cn_var_左值_0;
  return r18;

  if_then_23:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_24;

  if_merge_24:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_20;
  return NULL;
}

struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_25; else goto if_merge_26;

  if_then_25:
  return 0;
  goto if_merge_26;

  if_merge_26:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位或表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_27; else goto if_merge_28;

  if_then_27:
  return 0;
  goto if_merge_28;

  if_merge_28:
  goto while_cond_29;

  while_cond_29:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_逻辑与);
  if (r7) goto while_body_30; else goto while_exit_31;

  while_body_30:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位或表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  r11 = 创建二元表达式();
  cn_var_二元节点_2 = r11;
  r12 = cn_var_二元节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_32; else goto if_merge_33;

  while_exit_31:
  r18 = cn_var_左值_0;
  return r18;

  if_then_32:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_33;

  if_merge_33:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_29;
  return NULL;
}

struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_34; else goto if_merge_35;

  if_then_34:
  return 0;
  goto if_merge_35;

  if_merge_35:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位异或表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_36; else goto if_merge_37;

  if_then_36:
  return 0;
  goto if_merge_37;

  if_merge_37:
  goto while_cond_38;

  while_cond_38:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_按位或);
  if (r7) goto while_body_39; else goto while_exit_40;

  while_body_39:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位异或表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  r11 = 创建二元表达式();
  cn_var_二元节点_2 = r11;
  r12 = cn_var_二元节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_41; else goto if_merge_42;

  while_exit_40:
  r18 = cn_var_左值_0;
  return r18;

  if_then_41:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_42;

  if_merge_42:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_38;
  return NULL;
}

struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_43; else goto if_merge_44;

  if_then_43:
  return 0;
  goto if_merge_44;

  if_merge_44:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析按位与表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_45; else goto if_merge_46;

  if_then_45:
  return 0;
  goto if_merge_46;

  if_merge_46:
  goto while_cond_47;

  while_cond_47:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_按位异或);
  if (r7) goto while_body_48; else goto while_exit_49;

  while_body_48:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析按位与表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  r11 = 创建二元表达式();
  cn_var_二元节点_2 = r11;
  r12 = cn_var_二元节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_50; else goto if_merge_51;

  while_exit_49:
  r18 = cn_var_左值_0;
  return r18;

  if_then_50:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_51;

  if_merge_51:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_47;
  return NULL;
}

struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式节点* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r14;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_52; else goto if_merge_53;

  if_then_52:
  return 0;
  goto if_merge_53;

  if_merge_53:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析相等表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_54; else goto if_merge_55;

  if_then_54:
  return 0;
  goto if_merge_55;

  if_merge_55:
  goto while_cond_56;

  while_cond_56:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_按位与);
  if (r7) goto while_body_57; else goto while_exit_58;

  while_body_57:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式节点* cn_var_右值_1;
  r9 = cn_var_实例;
  r10 = 解析相等表达式(r9);
  cn_var_右值_1 = r10;
  struct 表达式节点* cn_var_二元节点_2;
  r11 = 创建二元表达式();
  cn_var_二元节点_2 = r11;
  r12 = cn_var_二元节点_2;
  r13 = r12 == 0;
  if (r13) goto if_then_59; else goto if_merge_60;

  while_exit_58:
  r18 = cn_var_左值_0;
  return r18;

  if_then_59:
  r14 = cn_var_左值_0;
  return r14;
  goto if_merge_60;

  if_merge_60:
  r15 = cn_var_左值_0;
  r16 = cn_var_右值_1;
  r17 = cn_var_二元节点_2;
  cn_var_左值_0 = r17;
  goto while_cond_56;
  return NULL;
}

struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例) {
  long long r6;
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
  struct 表达式节点* r17;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  _Bool r1;
  _Bool r5;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r18;
  enum 二元运算符 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_61; else goto if_merge_62;

  if_then_61:
  return 0;
  goto if_merge_62;

  if_merge_62:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析比较表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_63; else goto if_merge_64;

  if_then_63:
  return 0;
  goto if_merge_64;

  if_merge_64:
  goto while_cond_65;

  while_cond_65:
  r7 = cn_var_实例;
  r8 = 检查(r7, 词元类型枚举_等于);
  if (r8) goto logic_merge_69; else goto logic_rhs_68;

  while_body_66:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_二元_等于;
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_不等于);
  if (r12) goto if_then_70; else goto if_merge_71;

  while_exit_67:
  r24 = cn_var_左值_0;
  return r24;

  logic_rhs_68:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_不等于);
  goto logic_merge_69;

  logic_merge_69:
  if (r10) goto while_body_66; else goto while_exit_67;

  if_then_70:
  cn_var_运算符_1 = 二元运算符_二元_不等于;
  goto if_merge_71;

  if_merge_71:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析比较表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  r16 = 创建二元表达式();
  cn_var_二元节点_3 = r16;
  r17 = cn_var_二元节点_3;
  r18 = r17 == 0;
  if (r18) goto if_then_72; else goto if_merge_73;

  if_then_72:
  r19 = cn_var_左值_0;
  return r19;
  goto if_merge_73;

  if_merge_73:
  r20 = cn_var_左值_0;
  r21 = cn_var_右值_2;
  r22 = cn_var_运算符_1;
  r23 = cn_var_二元节点_3;
  cn_var_左值_0 = r23;
  goto while_cond_65;
  return NULL;
}

struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例) {
  long long r6, r7, r8;
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
  struct 表达式节点* r27;
  struct 表达式节点* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r31;
  struct 表达式节点* r33;
  struct 表达式节点* r34;
  _Bool r1;
  _Bool r5;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r16;
  _Bool r18;
  _Bool r20;
  _Bool r22;
  _Bool r28;
  enum 二元运算符 r32;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_74; else goto if_merge_75;

  if_then_74:
  return 0;
  goto if_merge_75;

  if_merge_75:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析位移表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_76; else goto if_merge_77;

  if_then_76:
  return 0;
  goto if_merge_77;

  if_merge_77:
  goto while_cond_78;

  while_cond_78:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_小于);
  if (r10) goto logic_merge_86; else goto logic_rhs_85;

  while_body_79:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_二元_小于;
  r17 = cn_var_实例;
  r18 = 检查(r17, 词元类型枚举_小于等于);
  if (r18) goto if_then_87; else goto if_else_88;

  while_exit_80:
  r34 = cn_var_左值_0;
  return r34;

  logic_rhs_81:
  r15 = cn_var_实例;
  r16 = 检查(r15, 词元类型枚举_大于等于);
  goto logic_merge_82;

  logic_merge_82:
  if (r16) goto while_body_79; else goto while_exit_80;

  logic_rhs_83:
  r13 = cn_var_实例;
  r14 = 检查(r13, 词元类型枚举_大于);
  goto logic_merge_84;

  logic_merge_84:
  if (r14) goto logic_merge_82; else goto logic_rhs_81;

  logic_rhs_85:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_小于等于);
  goto logic_merge_86;

  logic_merge_86:
  if (r12) goto logic_merge_84; else goto logic_rhs_83;

  if_then_87:
  cn_var_运算符_1 = 二元运算符_二元_小于等于;
  goto if_merge_89;

  if_else_88:
  r19 = cn_var_实例;
  r20 = 检查(r19, 词元类型枚举_大于);
  if (r20) goto if_then_90; else goto if_else_91;

  if_merge_89:
  r23 = cn_var_实例;
  前进词元(r23);
  struct 表达式节点* cn_var_右值_2;
  r24 = cn_var_实例;
  r25 = 解析位移表达式(r24);
  cn_var_右值_2 = r25;
  struct 表达式节点* cn_var_二元节点_3;
  r26 = 创建二元表达式();
  cn_var_二元节点_3 = r26;
  r27 = cn_var_二元节点_3;
  r28 = r27 == 0;
  if (r28) goto if_then_95; else goto if_merge_96;

  if_then_90:
  cn_var_运算符_1 = 二元运算符_二元_大于;
  goto if_merge_92;

  if_else_91:
  r21 = cn_var_实例;
  r22 = 检查(r21, 词元类型枚举_大于等于);
  if (r22) goto if_then_93; else goto if_merge_94;

  if_merge_92:
  goto if_merge_89;

  if_then_93:
  cn_var_运算符_1 = 二元运算符_二元_大于等于;
  goto if_merge_94;

  if_merge_94:
  goto if_merge_92;

  if_then_95:
  r29 = cn_var_左值_0;
  return r29;
  goto if_merge_96;

  if_merge_96:
  r30 = cn_var_左值_0;
  r31 = cn_var_右值_2;
  r32 = cn_var_运算符_1;
  r33 = cn_var_二元节点_3;
  cn_var_左值_0 = r33;
  goto while_cond_78;
  return NULL;
}

struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例) {
  long long r6;
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
  struct 表达式节点* r17;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  _Bool r1;
  _Bool r5;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r18;
  enum 二元运算符 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_97; else goto if_merge_98;

  if_then_97:
  return 0;
  goto if_merge_98;

  if_merge_98:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析加减表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_99; else goto if_merge_100;

  if_then_99:
  return 0;
  goto if_merge_100;

  if_merge_100:
  goto while_cond_101;

  while_cond_101:
  r7 = cn_var_实例;
  r8 = 检查(r7, 词元类型枚举_左移);
  if (r8) goto logic_merge_105; else goto logic_rhs_104;

  while_body_102:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_二元_左移;
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_右移);
  if (r12) goto if_then_106; else goto if_merge_107;

  while_exit_103:
  r24 = cn_var_左值_0;
  return r24;

  logic_rhs_104:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_右移);
  goto logic_merge_105;

  logic_merge_105:
  if (r10) goto while_body_102; else goto while_exit_103;

  if_then_106:
  cn_var_运算符_1 = 二元运算符_二元_右移;
  goto if_merge_107;

  if_merge_107:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析加减表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  r16 = 创建二元表达式();
  cn_var_二元节点_3 = r16;
  r17 = cn_var_二元节点_3;
  r18 = r17 == 0;
  if (r18) goto if_then_108; else goto if_merge_109;

  if_then_108:
  r19 = cn_var_左值_0;
  return r19;
  goto if_merge_109;

  if_merge_109:
  r20 = cn_var_左值_0;
  r21 = cn_var_右值_2;
  r22 = cn_var_运算符_1;
  r23 = cn_var_二元节点_3;
  cn_var_左值_0 = r23;
  goto while_cond_101;
  return NULL;
}

struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例) {
  long long r6;
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
  struct 表达式节点* r17;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  _Bool r1;
  _Bool r5;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r18;
  enum 二元运算符 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_110; else goto if_merge_111;

  if_then_110:
  return 0;
  goto if_merge_111;

  if_merge_111:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析乘除表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_112; else goto if_merge_113;

  if_then_112:
  return 0;
  goto if_merge_113;

  if_merge_113:
  goto while_cond_114;

  while_cond_114:
  r7 = cn_var_实例;
  r8 = 检查(r7, 词元类型枚举_加号);
  if (r8) goto logic_merge_118; else goto logic_rhs_117;

  while_body_115:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_二元_加;
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_减号);
  if (r12) goto if_then_119; else goto if_merge_120;

  while_exit_116:
  r24 = cn_var_左值_0;
  return r24;

  logic_rhs_117:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_减号);
  goto logic_merge_118;

  logic_merge_118:
  if (r10) goto while_body_115; else goto while_exit_116;

  if_then_119:
  cn_var_运算符_1 = 二元运算符_二元_减;
  goto if_merge_120;

  if_merge_120:
  r13 = cn_var_实例;
  前进词元(r13);
  struct 表达式节点* cn_var_右值_2;
  r14 = cn_var_实例;
  r15 = 解析乘除表达式(r14);
  cn_var_右值_2 = r15;
  struct 表达式节点* cn_var_二元节点_3;
  r16 = 创建二元表达式();
  cn_var_二元节点_3 = r16;
  r17 = cn_var_二元节点_3;
  r18 = r17 == 0;
  if (r18) goto if_then_121; else goto if_merge_122;

  if_then_121:
  r19 = cn_var_左值_0;
  return r19;
  goto if_merge_122;

  if_merge_122:
  r20 = cn_var_左值_0;
  r21 = cn_var_右值_2;
  r22 = cn_var_运算符_1;
  r23 = cn_var_二元节点_3;
  cn_var_左值_0 = r23;
  goto while_cond_114;
  return NULL;
}

struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例) {
  long long r6, r7;
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
  struct 表达式节点* r22;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  struct 表达式节点* r26;
  struct 表达式节点* r28;
  struct 表达式节点* r29;
  _Bool r1;
  _Bool r5;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r23;
  enum 二元运算符 r27;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_123; else goto if_merge_124;

  if_then_123:
  return 0;
  goto if_merge_124;

  if_merge_124:
  struct 表达式节点* cn_var_左值_0;
  r2 = cn_var_实例;
  r3 = 解析一元表达式(r2);
  cn_var_左值_0 = r3;
  r4 = cn_var_左值_0;
  r5 = r4 == 0;
  if (r5) goto if_then_125; else goto if_merge_126;

  if_then_125:
  return 0;
  goto if_merge_126;

  if_merge_126:
  goto while_cond_127;

  while_cond_127:
  r8 = cn_var_实例;
  r9 = 检查(r8, 词元类型枚举_星号);
  if (r9) goto logic_merge_133; else goto logic_rhs_132;

  while_body_128:
  enum 二元运算符 cn_var_运算符_1;
  cn_var_运算符_1 = 二元运算符_二元_乘;
  r14 = cn_var_实例;
  r15 = 检查(r14, 词元类型枚举_斜杠);
  if (r15) goto if_then_134; else goto if_else_135;

  while_exit_129:
  r29 = cn_var_左值_0;
  return r29;

  logic_rhs_130:
  r12 = cn_var_实例;
  r13 = 检查(r12, 词元类型枚举_百分号);
  goto logic_merge_131;

  logic_merge_131:
  if (r13) goto while_body_128; else goto while_exit_129;

  logic_rhs_132:
  r10 = cn_var_实例;
  r11 = 检查(r10, 词元类型枚举_斜杠);
  goto logic_merge_133;

  logic_merge_133:
  if (r11) goto logic_merge_131; else goto logic_rhs_130;

  if_then_134:
  cn_var_运算符_1 = 二元运算符_二元_除;
  goto if_merge_136;

  if_else_135:
  r16 = cn_var_实例;
  r17 = 检查(r16, 词元类型枚举_百分号);
  if (r17) goto if_then_137; else goto if_merge_138;

  if_merge_136:
  r18 = cn_var_实例;
  前进词元(r18);
  struct 表达式节点* cn_var_右值_2;
  r19 = cn_var_实例;
  r20 = 解析一元表达式(r19);
  cn_var_右值_2 = r20;
  struct 表达式节点* cn_var_二元节点_3;
  r21 = 创建二元表达式();
  cn_var_二元节点_3 = r21;
  r22 = cn_var_二元节点_3;
  r23 = r22 == 0;
  if (r23) goto if_then_139; else goto if_merge_140;

  if_then_137:
  cn_var_运算符_1 = 二元运算符_二元_取模;
  goto if_merge_138;

  if_merge_138:
  goto if_merge_136;

  if_then_139:
  r24 = cn_var_左值_0;
  return r24;
  goto if_merge_140;

  if_merge_140:
  r25 = cn_var_左值_0;
  r26 = cn_var_右值_2;
  r27 = cn_var_运算符_1;
  r28 = cn_var_二元节点_3;
  cn_var_左值_0 = r28;
  goto while_cond_127;
  return NULL;
}

struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例) {
  long long r2, r3, r4, r5, r6, r7;
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
  struct 表达式节点* r38;
  struct 表达式节点* r40;
  struct 表达式节点* r41;
  struct 表达式节点* r43;
  struct 解析器* r44;
  struct 表达式节点* r45;
  _Bool r1;
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
  _Bool r39;
  enum 一元运算符 r42;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_141; else goto if_merge_142;

  if_then_141:
  return 0;
  goto if_merge_142;

  if_merge_142:
  r8 = cn_var_实例;
  r9 = 检查(r8, 词元类型枚举_逻辑非);
  if (r9) goto logic_merge_156; else goto logic_rhs_155;

  if_then_143:
  enum 一元运算符 cn_var_运算符_0;
  cn_var_运算符_0 = 一元运算符_一元_逻辑非;
  r22 = cn_var_实例;
  r23 = 检查(r22, 词元类型枚举_减号);
  if (r23) goto if_then_157; else goto if_else_158;

  if_merge_144:
  r44 = cn_var_实例;
  r45 = 解析后缀表达式(r44);
  return r45;

  logic_rhs_145:
  r20 = cn_var_实例;
  r21 = 检查(r20, 词元类型枚举_自减);
  goto logic_merge_146;

  logic_merge_146:
  if (r21) goto if_then_143; else goto if_merge_144;

  logic_rhs_147:
  r18 = cn_var_实例;
  r19 = 检查(r18, 词元类型枚举_自增);
  goto logic_merge_148;

  logic_merge_148:
  if (r19) goto logic_merge_146; else goto logic_rhs_145;

  logic_rhs_149:
  r16 = cn_var_实例;
  r17 = 检查(r16, 词元类型枚举_按位与);
  goto logic_merge_150;

  logic_merge_150:
  if (r17) goto logic_merge_148; else goto logic_rhs_147;

  logic_rhs_151:
  r14 = cn_var_实例;
  r15 = 检查(r14, 词元类型枚举_星号);
  goto logic_merge_152;

  logic_merge_152:
  if (r15) goto logic_merge_150; else goto logic_rhs_149;

  logic_rhs_153:
  r12 = cn_var_实例;
  r13 = 检查(r12, 词元类型枚举_按位取反);
  goto logic_merge_154;

  logic_merge_154:
  if (r13) goto logic_merge_152; else goto logic_rhs_151;

  logic_rhs_155:
  r10 = cn_var_实例;
  r11 = 检查(r10, 词元类型枚举_减号);
  goto logic_merge_156;

  logic_merge_156:
  if (r11) goto logic_merge_154; else goto logic_rhs_153;

  if_then_157:
  cn_var_运算符_0 = 一元运算符_一元_取负;
  goto if_merge_159;

  if_else_158:
  r24 = cn_var_实例;
  r25 = 检查(r24, 词元类型枚举_按位取反);
  if (r25) goto if_then_160; else goto if_else_161;

  if_merge_159:
  r34 = cn_var_实例;
  前进词元(r34);
  struct 表达式节点* cn_var_操作数_1;
  r35 = cn_var_实例;
  r36 = 解析一元表达式(r35);
  cn_var_操作数_1 = r36;
  struct 表达式节点* cn_var_一元节点_2;
  r37 = 创建一元表达式();
  cn_var_一元节点_2 = r37;
  r38 = cn_var_一元节点_2;
  r39 = r38 == 0;
  if (r39) goto if_then_174; else goto if_merge_175;

  if_then_160:
  cn_var_运算符_0 = 一元运算符_一元_位取反;
  goto if_merge_162;

  if_else_161:
  r26 = cn_var_实例;
  r27 = 检查(r26, 词元类型枚举_星号);
  if (r27) goto if_then_163; else goto if_else_164;

  if_merge_162:
  goto if_merge_159;

  if_then_163:
  cn_var_运算符_0 = 一元运算符_一元_解引用;
  goto if_merge_165;

  if_else_164:
  r28 = cn_var_实例;
  r29 = 检查(r28, 词元类型枚举_按位与);
  if (r29) goto if_then_166; else goto if_else_167;

  if_merge_165:
  goto if_merge_162;

  if_then_166:
  cn_var_运算符_0 = 一元运算符_一元_取地址;
  goto if_merge_168;

  if_else_167:
  r30 = cn_var_实例;
  r31 = 检查(r30, 词元类型枚举_自增);
  if (r31) goto if_then_169; else goto if_else_170;

  if_merge_168:
  goto if_merge_165;

  if_then_169:
  cn_var_运算符_0 = 一元运算符_一元_前置自增;
  goto if_merge_171;

  if_else_170:
  r32 = cn_var_实例;
  r33 = 检查(r32, 词元类型枚举_自减);
  if (r33) goto if_then_172; else goto if_merge_173;

  if_merge_171:
  goto if_merge_168;

  if_then_172:
  cn_var_运算符_0 = 一元运算符_一元_前置自减;
  goto if_merge_173;

  if_merge_173:
  goto if_merge_171;

  if_then_174:
  r40 = cn_var_操作数_1;
  return r40;
  goto if_merge_175;

  if_merge_175:
  r41 = cn_var_操作数_1;
  r42 = cn_var_运算符_0;
  r43 = cn_var_一元节点_2;
  return r43;
  goto if_merge_144;
  return NULL;
}

struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例) {
  char* r24;
  char* r32;
  char* r38;
  char* r46;
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
  struct 表达式节点* r14;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式列表* r18;
  struct 表达式节点* r19;
  struct 解析器* r20;
  struct 解析器* r22;
  struct 解析器* r25;
  struct 表达式节点* r27;
  struct 表达式节点* r28;
  struct 表达式节点* r30;
  struct 表达式节点* r31;
  struct 表达式节点* r33;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r39;
  struct 表达式节点* r41;
  struct 表达式节点* r42;
  struct 表达式节点* r44;
  struct 表达式节点* r45;
  struct 表达式节点* r47;
  struct 解析器* r48;
  struct 解析器* r50;
  struct 解析器* r51;
  struct 表达式节点* r52;
  struct 解析器* r53;
  struct 表达式节点* r55;
  struct 表达式节点* r56;
  struct 表达式节点* r58;
  struct 表达式节点* r59;
  struct 表达式节点* r60;
  struct 表达式节点* r61;
  struct 解析器* r62;
  struct 解析器* r64;
  struct 表达式节点* r65;
  struct 表达式节点* r66;
  struct 表达式节点* r68;
  struct 表达式节点* r69;
  struct 表达式节点* r70;
  struct 解析器* r71;
  struct 解析器* r73;
  struct 表达式节点* r74;
  struct 表达式节点* r75;
  struct 表达式节点* r77;
  struct 表达式节点* r78;
  struct 表达式节点* r79;
  struct 表达式节点* r80;
  _Bool r1;
  _Bool r5;
  _Bool r7;
  _Bool r12;
  _Bool r15;
  _Bool r21;
  _Bool r26;
  _Bool r29;
  _Bool r35;
  _Bool r40;
  _Bool r43;
  _Bool r49;
  _Bool r54;
  _Bool r57;
  _Bool r63;
  _Bool r67;
  _Bool r72;
  _Bool r76;
  struct 词元 r23;
  struct 词元 r37;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_176; else goto if_merge_177;

  if_then_176:
  return 0;
  goto if_merge_177;

  if_merge_177:
  struct 表达式节点* cn_var_表达式_0;
  r2 = cn_var_实例;
  r3 = 解析基础表达式(r2);
  cn_var_表达式_0 = r3;
  r4 = cn_var_表达式_0;
  r5 = r4 == 0;
  if (r5) goto if_then_178; else goto if_merge_179;

  if_then_178:
  return 0;
  goto if_merge_179;

  if_merge_179:
  goto while_cond_180;

  while_cond_180:
  if (1) goto while_body_181; else goto while_exit_182;

  while_body_181:
  r6 = cn_var_实例;
  r7 = 检查(r6, 词元类型枚举_左括号);
  if (r7) goto if_then_183; else goto if_else_184;

  while_exit_182:
  r80 = cn_var_表达式_0;
  return r80;

  if_then_183:
  r8 = cn_var_实例;
  前进词元(r8);
  struct 表达式列表* cn_var_参数_1;
  r9 = cn_var_实例;
  r10 = 解析函数调用参数(r9);
  cn_var_参数_1 = r10;
  r11 = cn_var_实例;
  r12 = 期望(r11, 词元类型枚举_右括号);
  struct 表达式节点* cn_var_调用节点_2;
  r13 = 创建函数调用表达式();
  cn_var_调用节点_2 = r13;
  r14 = cn_var_调用节点_2;
  r15 = r14 == 0;
  if (r15) goto if_then_186; else goto if_merge_187;

  if_else_184:
  r20 = cn_var_实例;
  r21 = 检查(r20, 词元类型枚举_点);
  if (r21) goto if_then_188; else goto if_else_189;

  if_merge_185:
  goto while_cond_180;

  if_then_186:
  r16 = cn_var_表达式_0;
  return r16;
  goto if_merge_187;

  if_merge_187:
  r17 = cn_var_表达式_0;
  r18 = cn_var_参数_1;
  r19 = cn_var_调用节点_2;
  cn_var_表达式_0 = r19;
  goto if_merge_185;

  if_then_188:
  r22 = cn_var_实例;
  前进词元(r22);
  char* cn_var_成员名_3;
  r23 = cn_var_实例->当前词元;
  r24 = r23.值;
  cn_var_成员名_3 = r24;
  r25 = cn_var_实例;
  r26 = 期望(r25, 词元类型枚举_标识符);
  struct 表达式节点* cn_var_成员节点_4;
  r27 = 创建成员访问表达式();
  cn_var_成员节点_4 = r27;
  r28 = cn_var_成员节点_4;
  r29 = r28 == 0;
  if (r29) goto if_then_191; else goto if_merge_192;

  if_else_189:
  r34 = cn_var_实例;
  r35 = 检查(r34, 词元类型枚举_箭头);
  if (r35) goto if_then_193; else goto if_else_194;

  if_merge_190:
  goto if_merge_185;

  if_then_191:
  r30 = cn_var_表达式_0;
  return r30;
  goto if_merge_192;

  if_merge_192:
  r31 = cn_var_表达式_0;
  r32 = cn_var_成员名_3;
  r33 = cn_var_成员节点_4;
  cn_var_表达式_0 = r33;
  goto if_merge_190;

  if_then_193:
  r36 = cn_var_实例;
  前进词元(r36);
  char* cn_var_成员名_5;
  r37 = cn_var_实例->当前词元;
  r38 = r37.值;
  cn_var_成员名_5 = r38;
  r39 = cn_var_实例;
  r40 = 期望(r39, 词元类型枚举_标识符);
  struct 表达式节点* cn_var_成员节点_6;
  r41 = 创建成员访问表达式();
  cn_var_成员节点_6 = r41;
  r42 = cn_var_成员节点_6;
  r43 = r42 == 0;
  if (r43) goto if_then_196; else goto if_merge_197;

  if_else_194:
  r48 = cn_var_实例;
  r49 = 检查(r48, 词元类型枚举_左方括号);
  if (r49) goto if_then_198; else goto if_else_199;

  if_merge_195:
  goto if_merge_190;

  if_then_196:
  r44 = cn_var_表达式_0;
  return r44;
  goto if_merge_197;

  if_merge_197:
  r45 = cn_var_表达式_0;
  r46 = cn_var_成员名_5;
  r47 = cn_var_成员节点_6;
  cn_var_表达式_0 = r47;
  goto if_merge_195;

  if_then_198:
  r50 = cn_var_实例;
  前进词元(r50);
  struct 表达式节点* cn_var_索引_7;
  r51 = cn_var_实例;
  r52 = 解析表达式(r51);
  cn_var_索引_7 = r52;
  r53 = cn_var_实例;
  r54 = 期望(r53, 词元类型枚举_右方括号);
  struct 表达式节点* cn_var_数组节点_8;
  r55 = 创建数组访问表达式();
  cn_var_数组节点_8 = r55;
  r56 = cn_var_数组节点_8;
  r57 = r56 == 0;
  if (r57) goto if_then_201; else goto if_merge_202;

  if_else_199:
  r62 = cn_var_实例;
  r63 = 检查(r62, 词元类型枚举_自增);
  if (r63) goto if_then_203; else goto if_else_204;

  if_merge_200:
  goto if_merge_195;

  if_then_201:
  r58 = cn_var_表达式_0;
  return r58;
  goto if_merge_202;

  if_merge_202:
  r59 = cn_var_表达式_0;
  r60 = cn_var_索引_7;
  r61 = cn_var_数组节点_8;
  cn_var_表达式_0 = r61;
  goto if_merge_200;

  if_then_203:
  r64 = cn_var_实例;
  前进词元(r64);
  struct 表达式节点* cn_var_一元节点_9;
  r65 = 创建一元表达式();
  cn_var_一元节点_9 = r65;
  r66 = cn_var_一元节点_9;
  r67 = r66 == 0;
  if (r67) goto if_then_206; else goto if_merge_207;

  if_else_204:
  r71 = cn_var_实例;
  r72 = 检查(r71, 词元类型枚举_自减);
  if (r72) goto if_then_208; else goto if_else_209;

  if_merge_205:
  goto if_merge_200;

  if_then_206:
  r68 = cn_var_表达式_0;
  return r68;
  goto if_merge_207;

  if_merge_207:
  r69 = cn_var_表达式_0;
  r70 = cn_var_一元节点_9;
  cn_var_表达式_0 = r70;
  goto if_merge_205;

  if_then_208:
  r73 = cn_var_实例;
  前进词元(r73);
  struct 表达式节点* cn_var_一元节点_10;
  r74 = 创建一元表达式();
  cn_var_一元节点_10 = r74;
  r75 = cn_var_一元节点_10;
  r76 = r75 == 0;
  if (r76) goto if_then_211; else goto if_merge_212;

  if_else_209:
  goto while_exit_182;
  goto if_merge_210;

  if_merge_210:
  goto if_merge_205;

  if_then_211:
  r77 = cn_var_表达式_0;
  return r77;
  goto if_merge_212;

  if_merge_212:
  r78 = cn_var_表达式_0;
  r79 = cn_var_一元节点_10;
  cn_var_表达式_0 = r79;
  goto if_merge_210;
  return NULL;
}

struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例) {
  long long r2, r3, r4, r15, r49, r51;
  char* r44;
  char* r47;
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
  struct 表达式节点* r25;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 表达式节点* r29;
  struct 解析器* r30;
  struct 解析器* r32;
  struct 表达式节点* r33;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 表达式节点* r37;
  struct 解析器* r38;
  struct 解析器* r40;
  struct 解析器* r41;
  struct 解析器* r45;
  struct 表达式节点* r48;
  _Bool r1;
  _Bool r6;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r17;
  _Bool r19;
  _Bool r23;
  _Bool r27;
  _Bool r31;
  _Bool r35;
  _Bool r39;
  _Bool r42;
  _Bool r46;
  struct 词元 r43;
  struct 词元 r50;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_213; else goto if_merge_214;

  if_then_213:
  return 0;
  goto if_merge_214;

  if_merge_214:
  r5 = cn_var_实例;
  r6 = 检查(r5, 词元类型枚举_整数字面量);
  if (r6) goto logic_merge_222; else goto logic_rhs_221;

  if_then_215:
  r13 = cn_var_实例;
  r14 = 解析字面量(r13);
  return r14;
  goto if_merge_216;

  if_merge_216:
  r16 = cn_var_实例;
  r17 = 检查(r16, 词元类型枚举_关键字_真);
  if (r17) goto logic_merge_226; else goto logic_rhs_225;

  logic_rhs_217:
  r11 = cn_var_实例;
  r12 = 检查(r11, 词元类型枚举_字符字面量);
  goto logic_merge_218;

  logic_merge_218:
  if (r12) goto if_then_215; else goto if_merge_216;

  logic_rhs_219:
  r9 = cn_var_实例;
  r10 = 检查(r9, 词元类型枚举_字符串字面量);
  goto logic_merge_220;

  logic_merge_220:
  if (r10) goto logic_merge_218; else goto logic_rhs_217;

  logic_rhs_221:
  r7 = cn_var_实例;
  r8 = 检查(r7, 词元类型枚举_浮点字面量);
  goto logic_merge_222;

  logic_merge_222:
  if (r8) goto logic_merge_220; else goto logic_rhs_219;

  if_then_223:
  r20 = cn_var_实例;
  r21 = 解析字面量(r20);
  return r21;
  goto if_merge_224;

  if_merge_224:
  r22 = cn_var_实例;
  r23 = 检查(r22, 词元类型枚举_关键字_无);
  if (r23) goto if_then_227; else goto if_merge_228;

  logic_rhs_225:
  r18 = cn_var_实例;
  r19 = 检查(r18, 词元类型枚举_关键字_假);
  goto logic_merge_226;

  logic_merge_226:
  if (r19) goto if_then_223; else goto if_merge_224;

  if_then_227:
  r24 = cn_var_实例;
  前进词元(r24);
  r25 = 创建空值表达式();
  return r25;
  goto if_merge_228;

  if_merge_228:
  r26 = cn_var_实例;
  r27 = 检查(r26, 词元类型枚举_标识符);
  if (r27) goto if_then_229; else goto if_merge_230;

  if_then_229:
  r28 = cn_var_实例;
  r29 = 解析标识符表达式(r28);
  return r29;
  goto if_merge_230;

  if_merge_230:
  r30 = cn_var_实例;
  r31 = 检查(r30, 词元类型枚举_左括号);
  if (r31) goto if_then_231; else goto if_merge_232;

  if_then_231:
  r32 = cn_var_实例;
  r33 = 解析括号表达式(r32);
  return r33;
  goto if_merge_232;

  if_merge_232:
  r34 = cn_var_实例;
  r35 = 检查(r34, 词元类型枚举_关键字_自身);
  if (r35) goto if_then_233; else goto if_merge_234;

  if_then_233:
  r36 = cn_var_实例;
  前进词元(r36);
  r37 = 创建自身表达式();
  return r37;
  goto if_merge_234;

  if_merge_234:
  r38 = cn_var_实例;
  r39 = 检查(r38, 词元类型枚举_关键字_基类);
  if (r39) goto if_then_235; else goto if_merge_236;

  if_then_235:
  r40 = cn_var_实例;
  前进词元(r40);
  r41 = cn_var_实例;
  r42 = 期望(r41, 词元类型枚举_点);
  char* cn_var_成员名_0;
  r43 = cn_var_实例->当前词元;
  r44 = r43.值;
  cn_var_成员名_0 = r44;
  r45 = cn_var_实例;
  r46 = 期望(r45, 词元类型枚举_标识符);
  r47 = cn_var_成员名_0;
  r48 = 创建基类访问表达式(r47);
  return r48;
  goto if_merge_236;

  if_merge_236:
  r49 = cn_var_实例->诊断集合;
  r50 = cn_var_实例->当前词元;
  r51 = r50.位置;
  报告错误(r49, 诊断错误码_语法_无效表达式, r51, "期望表达式");
  return 0;
}

struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例) {
  char* r19;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 表达式节点* r3;
  struct 解析器* r13;
  struct 表达式节点* r14;
  struct 解析器* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 解析器* r20;
  struct 表达式节点* r21;
  _Bool r1;
  _Bool r4;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  struct 词元 r5;
  struct 词元 r18;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_237; else goto if_merge_238;

  if_then_237:
  return 0;
  goto if_merge_238;

  if_merge_238:
  struct 表达式节点* cn_var_节点_0;
  r2 = 创建字面量表达式();
  cn_var_节点_0 = r2;
  r3 = cn_var_节点_0;
  r4 = r3 == 0;
  if (r4) goto if_then_239; else goto if_merge_240;

  if_then_239:
  return 0;
  goto if_merge_240;

  if_merge_240:
  r5 = cn_var_实例->当前词元;
  r6 = r5.类型;
  r7 = r6 == 词元类型枚举_整数字面量;
  if (r7) goto case_body_242; else goto switch_check_249;

  switch_check_249:
  r8 = r6 == 词元类型枚举_浮点字面量;
  if (r8) goto case_body_243; else goto switch_check_250;

  switch_check_250:
  r9 = r6 == 词元类型枚举_字符串字面量;
  if (r9) goto case_body_244; else goto switch_check_251;

  switch_check_251:
  r10 = r6 == 词元类型枚举_字符字面量;
  if (r10) goto case_body_245; else goto switch_check_252;

  switch_check_252:
  r11 = r6 == 词元类型枚举_关键字_真;
  if (r11) goto case_body_246; else goto switch_check_253;

  switch_check_253:
  r12 = r6 == 词元类型枚举_关键字_假;
  if (r12) goto case_body_247; else goto case_default_248;

  case_body_242:
  goto switch_merge_241;
  goto switch_merge_241;

  case_body_243:
  goto switch_merge_241;
  goto switch_merge_241;

  case_body_244:
  goto switch_merge_241;
  goto switch_merge_241;

  case_body_245:
  goto switch_merge_241;
  goto switch_merge_241;

  case_body_246:
  r13 = cn_var_实例;
  前进词元(r13);
  r14 = cn_var_节点_0;
  return r14;
  goto switch_merge_241;

  case_body_247:
  r15 = cn_var_实例;
  前进词元(r15);
  r16 = cn_var_节点_0;
  return r16;
  goto switch_merge_241;

  case_default_248:
  r17 = cn_var_节点_0;
  释放表达式节点(r17);
  return 0;
  goto switch_merge_241;

  switch_merge_241:
  r18 = cn_var_实例->当前词元;
  r19 = r18.值;
  r20 = cn_var_实例;
  前进词元(r20);
  r21 = cn_var_节点_0;
  return r21;
}

struct 表达式节点* 解析标识符表达式(struct 解析器* cn_var_实例) {
  char* r6;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 表达式节点* r3;
  struct 解析器* r7;
  struct 表达式节点* r8;
  _Bool r1;
  _Bool r4;
  struct 词元 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_254; else goto if_merge_255;

  if_then_254:
  return 0;
  goto if_merge_255;

  if_merge_255:
  struct 表达式节点* cn_var_节点_0;
  r2 = 创建标识符表达式();
  cn_var_节点_0 = r2;
  r3 = cn_var_节点_0;
  r4 = r3 == 0;
  if (r4) goto if_then_256; else goto if_merge_257;

  if_then_256:
  return 0;
  goto if_merge_257;

  if_merge_257:
  r5 = cn_var_实例->当前词元;
  r6 = r5.值;
  r7 = cn_var_实例;
  前进词元(r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 表达式节点* 解析括号表达式(struct 解析器* cn_var_实例) {
  long long r9;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r7;
  struct 类型节点* r8;
  struct 类型节点* r10;
  struct 解析器* r12;
  struct 解析器* r14;
  struct 解析器* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 类型节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 解析器* r22;
  struct 表达式节点* r23;
  struct 解析器* r24;
  struct 表达式节点* r26;
  _Bool r1;
  _Bool r3;
  _Bool r6;
  _Bool r11;
  _Bool r13;
  _Bool r18;
  _Bool r25;
  struct 词元 r4;
  enum 词元类型枚举 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_258; else goto if_merge_259;

  if_then_258:
  return 0;
  goto if_merge_259;

  if_merge_259:
  r2 = cn_var_实例;
  r3 = 期望(r2, 词元类型枚举_左括号);
  r4 = cn_var_实例->当前词元;
  r5 = r4.类型;
  r6 = 是否类型关键字(r5);
  if (r6) goto if_then_260; else goto if_merge_261;

  if_then_260:
  struct 类型节点* cn_var_目标类型_0;
  r7 = cn_var_实例;
  r8 = 解析类型(r7);
  cn_var_目标类型_0 = r8;
  r10 = cn_var_目标类型_0;
  r11 = r10 != 0;
  if (r11) goto logic_rhs_264; else goto logic_merge_265;

  if_merge_261:
  struct 表达式节点* cn_var_表达式_2;
  r22 = cn_var_实例;
  r23 = 解析表达式(r22);
  cn_var_表达式_2 = r23;
  r24 = cn_var_实例;
  r25 = 期望(r24, 词元类型枚举_右括号);
  r26 = cn_var_表达式_2;
  return r26;

  if_then_262:
  r14 = cn_var_实例;
  前进词元(r14);
  struct 表达式节点* cn_var_操作数_1;
  r15 = cn_var_实例;
  r16 = 解析一元表达式(r15);
  cn_var_操作数_1 = r16;
  r17 = cn_var_操作数_1;
  r18 = r17 == 0;
  if (r18) goto if_then_266; else goto if_merge_267;

  if_merge_263:
  goto if_merge_261;

  logic_rhs_264:
  r12 = cn_var_实例;
  r13 = 检查(r12, 词元类型枚举_右括号);
  goto logic_merge_265;

  logic_merge_265:
  if (r13) goto if_then_262; else goto if_merge_263;

  if_then_266:
  return 0;
  goto if_merge_267;

  if_merge_267:
  r19 = cn_var_目标类型_0;
  r20 = cn_var_操作数_1;
  r21 = 创建类型转换表达式(r19, r20);
  return r21;
  goto if_merge_263;
  return NULL;
}

struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例) {
  struct 解析器* r0;
  struct 表达式列表* r2;
  struct 表达式列表* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 表达式节点* r9;
  struct 表达式节点* r10;
  struct 表达式列表* r12;
  struct 表达式节点* r13;
  struct 解析器* r14;
  struct 表达式列表* r17;
  _Bool r1;
  _Bool r4;
  _Bool r6;
  _Bool r7;
  _Bool r11;
  _Bool r15;
  _Bool r16;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_268; else goto if_merge_269;

  if_then_268:
  return 0;
  goto if_merge_269;

  if_merge_269:
  struct 表达式列表* cn_var_列表_0;
  r2 = 创建表达式列表();
  cn_var_列表_0 = r2;
  r3 = cn_var_列表_0;
  r4 = r3 == 0;
  if (r4) goto if_then_270; else goto if_merge_271;

  if_then_270:
  return 0;
  goto if_merge_271;

  if_merge_271:
  goto while_cond_272;

  while_cond_272:
  r5 = cn_var_实例;
  r6 = 检查(r5, 词元类型枚举_右括号);
  r7 = !r6;
  if (r7) goto while_body_273; else goto while_exit_274;

  while_body_273:
  struct 表达式节点* cn_var_参数_1;
  r8 = cn_var_实例;
  r9 = 解析表达式(r8);
  cn_var_参数_1 = r9;
  r10 = cn_var_参数_1;
  r11 = r10 != 0;
  if (r11) goto if_then_275; else goto if_merge_276;

  while_exit_274:
  r17 = cn_var_列表_0;
  return r17;

  if_then_275:
  r12 = cn_var_列表_0;
  r13 = cn_var_参数_1;
  表达式列表添加(r12, r13);
  goto if_merge_276;

  if_merge_276:
  r14 = cn_var_实例;
  r15 = 匹配(r14, 词元类型枚举_逗号);
  r16 = !r15;
  if (r16) goto if_then_277; else goto if_merge_278;

  if_then_277:
  goto while_exit_274;
  goto if_merge_278;

  if_merge_278:
  goto while_cond_272;
  return NULL;
}

