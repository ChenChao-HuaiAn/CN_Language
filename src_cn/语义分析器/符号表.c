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
struct 声明语句;
struct 声明语句 {
    struct AST节点 节点基类;
    struct 声明节点* 声明;
};
struct 表达式语句;
struct 如果语句;
struct 当语句;
struct 循环语句;
struct 返回语句;
struct 中断语句;
struct 继续语句;
struct 选择语句;
struct 尝试语句;
struct 抛出语句;
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

// Forward Declarations - 从导入模块
extern struct 源位置 cn_var_空源位置;
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, const char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, const char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, const char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, const char*, const char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, const char*);
struct 源位置 创建未知位置(void);
struct 源位置 创建源位置(const char*, long long, long long);
void 清空诊断集合(struct 诊断集合*);
void 打印所有诊断(struct 诊断集合*);
void 打印诊断信息(struct 诊断信息*);
char* 获取严重级别字符串(enum 诊断严重级别);
_Bool 应该继续(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
_Bool 有错误(struct 诊断集合*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, const char*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 释放诊断集合(struct 诊断集合*);
struct 诊断集合* 创建诊断集合(long long);
void* 数组获取(void*, long long);
long long 获取位置参数个数(void);
char* 获取位置参数(long long);
long long 选项存在(const char*);
char* 查找选项(const char*);
char* 获取程序名称(void);
char* 获取参数(long long);
long long 获取参数个数(void);
long long 求最小值(long long, long long);
long long 求最大值(long long, long long);
long long 获取绝对值(long long);
char* 读取行(void);
long long 字符串格式化(const char*, long long, const char*);
char* 字符串格式(const char*);
char* 复制字符串副本(const char*);
long long 类型大小(long long);
void* 分配内存数组(long long, long long);

// CN Language Enum Definitions
enum 符号类型 {
    符号类型_变量符号,
    符号类型_函数符号,
    符号类型_参数符号,
    符号类型_结构体符号,
    符号类型_枚举符号,
    符号类型_枚举成员符号,
    符号类型_类符号,
    符号类型_接口符号,
    符号类型_类成员符号,
    符号类型_模块符号,
    符号类型_导入符号,
    符号类型_类型参数符号
};

enum 作用域类型 {
    作用域类型_全局作用域,
    作用域类型_函数作用域,
    作用域类型_块作用域,
    作用域类型_类作用域,
    作用域类型_结构体作用域,
    作用域类型_枚举作用域,
    作用域类型_模块作用域,
    作用域类型_循环作用域
};

// CN Language Global Struct Forward Declarations
struct 符号标志;
struct 作用域;
struct 符号;
struct 符号表管理器;

// CN Language Global Struct Definitions
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

struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};

// Global Variables
long long cn_var_初始符号容量 = 16;
long long cn_var_初始子作用域容量 = 4;
long long cn_var_符号大小 = 120;
long long cn_var_作用域大小 = 80;
long long cn_var_符号表管理器大小 = 32;
long long cn_var_符号指针大小 = 8;
long long cn_var_作用域指针大小 = 8;

// Forward Declarations
struct 符号* 创建变量符号(const char*, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建函数符号(const char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建参数符号(const char*, struct 类型节点*, struct 源位置);
struct 符号* 创建结构体符号(const char*, struct 源位置);
struct 符号* 创建枚举符号(const char*, struct 源位置);
struct 符号* 创建枚举成员符号(const char*, long long, _Bool, struct 源位置);
struct 符号* 创建类符号(const char*, struct 源位置, _Bool);
struct 符号* 创建接口符号(const char*, struct 源位置);
struct 作用域* 创建作用域(enum 作用域类型, const char*, struct 作用域*);
void 销毁作用域(struct 作用域*);
struct 符号表管理器* 创建符号表管理器();
void 销毁符号表管理器(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, const char*, struct 符号*);
void 离开作用域(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
_Bool 在循环体内(struct 符号表管理器*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, const char*);
struct 符号* 查找符号(struct 符号表管理器*, const char*);
struct 符号* 查找全局符号(struct 符号表管理器*, const char*);
struct 符号* 在作用域查找符号(struct 作用域*, const char*);
struct 符号* 查找类成员(struct 符号*, const char*);
char* 获取符号类型名称(enum 符号类型);
char* 获取作用域类型名称(enum 作用域类型);
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);

struct 符号* 创建变量符号(const char* cn_var_名称, struct 类型节点* cn_var_类型, struct 源位置 cn_var_位置, struct 符号标志 cn_var_标志) {
  long long r0;
  char* r2;
  void* r1;
  struct 类型节点* r3;
  struct 符号* r6;
  struct 源位置 r4;
  struct 符号标志 r5;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_类型;
  r4 = cn_var_位置;
  r5 = cn_var_标志;
  r6 = cn_var_符号指针_0;
  return r6;
}

struct 符号* 创建函数符号(const char* cn_var_名称, struct 参数** cn_var_参数列表, long long cn_var_参数个数, struct 类型节点* cn_var_返回类型, struct 源位置 cn_var_位置, struct 符号标志 cn_var_标志) {
  long long r0, r4;
  char* r2;
  void* r1;
  struct 参数** r3;
  struct 类型节点* r5;
  struct 符号* r8;
  struct 源位置 r6;
  struct 符号标志 r7;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_参数列表;
  r4 = cn_var_参数个数;
  r5 = cn_var_返回类型;
  r6 = cn_var_位置;
  r7 = cn_var_标志;
  r8 = cn_var_符号指针_0;
  return r8;
}

struct 符号* 创建参数符号(const char* cn_var_名称, struct 类型节点* cn_var_类型, struct 源位置 cn_var_位置) {
  long long r0;
  char* r2;
  void* r1;
  struct 类型节点* r3;
  struct 符号* r5;
  struct 源位置 r4;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_类型;
  r4 = cn_var_位置;
  r5 = cn_var_符号指针_0;
  return r5;
}

struct 符号* 创建结构体符号(const char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r4;
  struct 源位置 r3;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_位置;
  r4 = cn_var_符号指针_0;
  return r4;
}

struct 符号* 创建枚举符号(const char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r4;
  struct 源位置 r3;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_位置;
  r4 = cn_var_符号指针_0;
  return r4;
}

struct 符号* 创建枚举成员符号(const char* cn_var_名称, long long cn_var_值, _Bool cn_var_有显式值, struct 源位置 cn_var_位置) {
  long long r0, r3;
  char* r2;
  void* r1;
  struct 符号* r6;
  _Bool r4;
  struct 源位置 r5;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_值;
  r4 = cn_var_有显式值;
  r5 = cn_var_位置;
  r6 = cn_var_符号指针_0;
  return r6;
}

struct 符号* 创建类符号(const char* cn_var_名称, struct 源位置 cn_var_位置, _Bool cn_var_是抽象) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r5;
  _Bool r4;
  struct 源位置 r3;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_位置;
  r4 = cn_var_是抽象;
  r5 = cn_var_符号指针_0;
  return r5;
}

struct 符号* 创建接口符号(const char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0;
  char* r2;
  void* r1;
  struct 符号* r4;
  struct 源位置 r3;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  r1 = 分配内存(r0);
  cn_var_符号指针_0 = (struct 符号*)分配内存(cn_var_符号大小);
  r2 = cn_var_名称;
  r3 = cn_var_位置;
  r4 = cn_var_符号指针_0;
  return r4;
}

struct 作用域* 创建作用域(enum 作用域类型 cn_var_类型, const char* cn_var_名称, struct 作用域* cn_var_父作用域) {
  long long r0, r5, r6, r7, r9, r10, r11;
  char* r3;
  void* r1;
  struct 作用域* r4;
  void* r8;
  void* r12;
  struct 作用域* r13;
  enum 作用域类型 r2;

  entry:
  struct 作用域* cn_var_作用域指针_0;
  r0 = cn_var_作用域大小;
  r1 = 分配清零内存(1, r0);
  cn_var_作用域指针_0 = (struct 作用域*)分配清零内存(1, cn_var_作用域大小);
  r2 = cn_var_类型;
  r3 = cn_var_名称;
  r4 = cn_var_父作用域;
  r5 = cn_var_初始符号容量;
  r6 = cn_var_初始符号容量;
  r7 = cn_var_符号指针大小;
  r8 = 分配清零内存(r6, r7);
  r9 = cn_var_初始子作用域容量;
  r10 = cn_var_初始子作用域容量;
  r11 = cn_var_作用域指针大小;
  r12 = 分配清零内存(r10, r11);
  r13 = cn_var_作用域指针_0;
  return r13;
}

void 销毁作用域(struct 作用域* cn_var_作用域指针) {
  long long r1, r2, r3, r4, r6, r8, r9, r10, r11, r12, r14, r16, r17;
  struct 作用域** r5;
  struct 符号** r13;
  struct 符号** r18;
  struct 作用域** r19;
  struct 作用域* r0;
  struct 作用域* r7;
  struct 符号* r15;
  struct 作用域* r20;

  entry:
  r0 = cn_var_作用域指针;
  r1 = r0 == 0;
  if (r1) goto if_then_597; else goto if_merge_598;

  if_then_597:
  return;
  goto if_merge_598;

  if_merge_598:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_599;

  for_cond_599:
  r2 = cn_var_i_0;
  r3 = cn_var_作用域指针->子作用域数量;
  r4 = r2 < r3;
  if (r4) goto for_body_600; else goto for_exit_602;

  for_body_600:
  r5 = cn_var_作用域指针->子作用域列表;
  r6 = cn_var_i_0;
  r7 = (struct 作用域*)cn_rt_array_get_element(r5, r6, 8);
  销毁作用域(r7);
  goto for_update_601;

  for_update_601:
  r8 = cn_var_i_0;
  r9 = r8 + 1;
  cn_var_i_0 = r9;
  goto for_cond_599;

  for_exit_602:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_603;

  for_cond_603:
  r10 = cn_var_i_1;
  r11 = cn_var_作用域指针->符号数量;
  r12 = r10 < r11;
  if (r12) goto for_body_604; else goto for_exit_606;

  for_body_604:
  r13 = cn_var_作用域指针->符号表;
  r14 = cn_var_i_1;
  r15 = (struct 符号*)cn_rt_array_get_element(r13, r14, 8);
  释放内存(r15);
  goto for_update_605;

  for_update_605:
  r16 = cn_var_i_1;
  r17 = r16 + 1;
  cn_var_i_1 = r17;
  goto for_cond_603;

  for_exit_606:
  r18 = cn_var_作用域指针->符号表;
  释放内存(r18);
  r19 = cn_var_作用域指针->子作用域列表;
  释放内存(r19);
  r20 = cn_var_作用域指针;
  释放内存(r20);
  return;
}

struct 符号表管理器* 创建符号表管理器() {
  long long r0;
  void* r1;
  struct 作用域* r2;
  struct 作用域* r3;
  struct 符号表管理器* r4;

  entry:
  struct 符号表管理器* cn_var_管理器_0;
  r0 = cn_var_符号表管理器大小;
  r1 = 分配清零内存(1, r0);
  cn_var_管理器_0 = (struct 符号表管理器*)分配清零内存(1, cn_var_符号表管理器大小);
  r2 = 创建作用域(作用域类型_全局作用域, "全局", 0);
  r3 = cn_var_管理器_0->全局作用域;
  r4 = cn_var_管理器_0;
  return r4;
}

void 销毁符号表管理器(struct 符号表管理器* cn_var_管理器) {
  long long r1;
  struct 符号表管理器* r0;
  struct 作用域* r2;
  struct 符号表管理器* r3;

  entry:
  r0 = cn_var_管理器;
  r1 = r0 == 0;
  if (r1) goto if_then_607; else goto if_merge_608;

  if_then_607:
  return;
  goto if_merge_608;

  if_merge_608:
  r2 = cn_var_管理器->全局作用域;
  销毁作用域(r2);
  r3 = cn_var_管理器;
  释放内存(r3);
  return;
}

void 进入作用域(struct 符号表管理器* cn_var_管理器, enum 作用域类型 cn_var_类型, const char* cn_var_名称, struct 符号* cn_var_关联符号) {
  long long r6, r8, r9, r11, r12, r16, r17, r18, r24, r26, r27, r29, r30;
  char* r1;
  struct 作用域** r14;
  struct 作用域** r22;
  struct 作用域* r2;
  struct 作用域* r3;
  struct 符号* r4;
  struct 作用域* r5;
  struct 作用域* r7;
  struct 作用域* r10;
  struct 作用域* r13;
  struct 作用域* r15;
  void* r19;
  struct 作用域* r20;
  struct 作用域* r21;
  struct 作用域* r23;
  struct 作用域* r25;
  struct 作用域* r28;
  enum 作用域类型 r0;

  entry:
  struct 作用域* cn_var_新作用域_0;
  r0 = cn_var_类型;
  r1 = cn_var_名称;
  r2 = cn_var_管理器->当前作用域;
  r3 = 创建作用域(r0, r1, r2);
  cn_var_新作用域_0 = r3;
  r4 = cn_var_关联符号;
  r5 = cn_var_管理器->当前作用域;
  r6 = r5->子作用域数量;
  r7 = cn_var_管理器->当前作用域;
  r8 = r7->子作用域容量;
  r9 = r6 >= r8;
  if (r9) goto if_then_609; else goto if_merge_610;

  if_then_609:
  r10 = cn_var_管理器->当前作用域;
  r11 = r10->子作用域容量;
  r12 = r11 << 1;
  r13 = cn_var_管理器->当前作用域;
  r14 = r13->子作用域列表;
  r15 = cn_var_管理器->当前作用域;
  r16 = r15->子作用域容量;
  r17 = cn_var_作用域指针大小;
  r18 = r16 * r17;
  r19 = 重新分配内存(r14, r18);
  goto if_merge_610;

  if_merge_610:
  r20 = cn_var_新作用域_0;
  r21 = cn_var_管理器->当前作用域;
  r22 = r21->子作用域列表;
  r23 = cn_var_管理器->当前作用域;
  r24 = r23->子作用域数量;
    { long long _tmp_r0 = r20; cn_rt_array_set_element(r22, r24, &_tmp_r0, 8); }
  r25 = cn_var_管理器->当前作用域;
  r26 = r25->子作用域数量;
  r27 = r26 + 1;
  r28 = cn_var_新作用域_0;
  r29 = cn_var_管理器->作用域深度;
  r30 = r29 + 1;
  return;
}

void 离开作用域(struct 符号表管理器* cn_var_管理器) {
  long long r2, r5, r6;
  struct 作用域* r0;
  struct 作用域* r1;
  struct 作用域* r3;
  struct 作用域* r4;

  entry:
  r0 = cn_var_管理器->当前作用域;
  r1 = r0->父作用域;
  r2 = r1 != 0;
  if (r2) goto if_then_611; else goto if_merge_612;

  if_then_611:
  r3 = cn_var_管理器->当前作用域;
  r4 = r3->父作用域;
  r5 = cn_var_管理器->作用域深度;
  r6 = r5 - 1;
  goto if_merge_612;

  if_merge_612:
  return;
}

void 设置循环作用域(struct 符号表管理器* cn_var_管理器) {

  entry:
  return;
}

_Bool 在循环体内(struct 符号表管理器* cn_var_管理器) {
  long long r2, r3, r4;
  struct 作用域* r0;
  struct 作用域* r1;

  entry:
  long long cn_var_作用域指针_0;
  r0 = cn_var_管理器->当前作用域;
  cn_var_作用域指针_0 = r0;
  goto while_cond_613;

  while_cond_613:
  r1 = cn_var_作用域指针_0;
  r2 = r1 != 0;
  if (r2) goto while_body_614; else goto while_exit_615;

  while_body_614:
  r3 = cn_var_作用域指针_0.是循环体;
  if (r3) goto if_then_616; else goto if_merge_617;

  while_exit_615:
  return 0;

  if_then_616:
  return 1;
  goto if_merge_617;

  if_merge_617:
  r4 = cn_var_作用域指针_0.父作用域;
  cn_var_作用域指针_0 = r4;
  goto while_cond_613;
  return 0;
}

_Bool 插入符号(struct 符号表管理器* cn_var_管理器, struct 符号* cn_var_符号指针) {
  long long r4, r5, r6, r8, r10, r11, r13, r14, r18, r19, r20, r26, r28, r29;
  char* r1;
  struct 符号** r16;
  struct 符号** r24;
  struct 符号表管理器* r0;
  struct 符号* r2;
  struct 符号* r3;
  struct 作用域* r7;
  struct 作用域* r9;
  struct 作用域* r12;
  struct 作用域* r15;
  struct 作用域* r17;
  void* r21;
  struct 符号* r22;
  struct 作用域* r23;
  struct 作用域* r25;
  struct 作用域* r27;
  struct 作用域* r30;

  entry:
  struct 符号* cn_var_已存在_0;
  r0 = cn_var_管理器;
  r1 = cn_var_符号指针->名称;
  r2 = 查找当前作用域符号(r0, r1);
  cn_var_已存在_0 = r2;
  r3 = cn_var_已存在_0;
  r4 = r3 != 0;
  if (r4) goto if_then_618; else goto if_merge_619;

  if_then_618:
  r5 = cn_var_管理器->错误计数;
  r6 = r5 + 1;
  return 0;
  goto if_merge_619;

  if_merge_619:
  r7 = cn_var_管理器->当前作用域;
  r8 = r7->符号数量;
  r9 = cn_var_管理器->当前作用域;
  r10 = r9->符号容量;
  r11 = r8 >= r10;
  if (r11) goto if_then_620; else goto if_merge_621;

  if_then_620:
  r12 = cn_var_管理器->当前作用域;
  r13 = r12->符号容量;
  r14 = r13 << 1;
  r15 = cn_var_管理器->当前作用域;
  r16 = r15->符号表;
  r17 = cn_var_管理器->当前作用域;
  r18 = r17->符号容量;
  r19 = cn_var_符号指针大小;
  r20 = r18 * r19;
  r21 = 重新分配内存(r16, r20);
  goto if_merge_621;

  if_merge_621:
  r22 = cn_var_符号指针;
  r23 = cn_var_管理器->当前作用域;
  r24 = r23->符号表;
  r25 = cn_var_管理器->当前作用域;
  r26 = r25->符号数量;
    { long long _tmp_r1 = r22; cn_rt_array_set_element(r24, r26, &_tmp_r1, 8); }
  r27 = cn_var_管理器->当前作用域;
  r28 = r27->符号数量;
  r29 = r28 + 1;
  r30 = cn_var_管理器->当前作用域;
  return 1;
}

_Bool 在作用域插入符号(struct 作用域* cn_var_目标作用域, struct 符号* cn_var_符号指针) {
  long long r0, r1, r2, r4, r8, r9, r10, r11, r12, r13, r14, r15, r16, r18, r19, r20, r24, r25, r26;
  char* r6;
  char* r7;
  struct 符号** r3;
  struct 符号** r17;
  struct 符号** r23;
  struct 符号* r5;
  void* r21;
  struct 符号* r22;
  struct 作用域* r27;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_622;

  for_cond_622:
  r0 = cn_var_i_0;
  r1 = cn_var_目标作用域->符号数量;
  r2 = r0 < r1;
  if (r2) goto for_body_623; else goto for_exit_625;

  for_body_623:
  r3 = cn_var_目标作用域->符号表;
  r4 = cn_var_i_0;
  r5 = (struct 符号*)cn_rt_array_get_element(r3, r4, 8);
  r6 = r5->名称;
  r7 = cn_var_符号指针->名称;
  r8 = 比较字符串(r6, r7);
  r9 = r8 == 0;
  if (r9) goto if_then_626; else goto if_merge_627;

  for_update_624:
  r10 = cn_var_i_0;
  r11 = r10 + 1;
  cn_var_i_0 = r11;
  goto for_cond_622;

  for_exit_625:
  r12 = cn_var_目标作用域->符号数量;
  r13 = cn_var_目标作用域->符号容量;
  r14 = r12 >= r13;
  if (r14) goto if_then_628; else goto if_merge_629;

  if_then_626:
  return 0;
  goto if_merge_627;

  if_merge_627:
  goto for_update_624;

  if_then_628:
  r15 = cn_var_目标作用域->符号容量;
  r16 = r15 << 1;
  r17 = cn_var_目标作用域->符号表;
  r18 = cn_var_目标作用域->符号容量;
  r19 = cn_var_符号指针大小;
  r20 = r18 * r19;
  r21 = 重新分配内存(r17, r20);
  goto if_merge_629;

  if_merge_629:
  r22 = cn_var_符号指针;
  r23 = cn_var_目标作用域->符号表;
  r24 = cn_var_目标作用域->符号数量;
    { long long _tmp_r2 = r22; cn_rt_array_set_element(r23, r24, &_tmp_r2, 8); }
  r25 = cn_var_目标作用域->符号数量;
  r26 = r25 + 1;
  r27 = cn_var_目标作用域;
  return 1;
}

struct 符号* 查找当前作用域符号(struct 符号表管理器* cn_var_管理器, const char* cn_var_名称) {
  long long r0, r2, r3, r6, r10, r11, r14, r16, r17;
  char* r8;
  char* r9;
  struct 符号** r5;
  struct 符号** r13;
  struct 作用域* r1;
  struct 作用域* r4;
  struct 符号* r7;
  struct 作用域* r12;
  struct 符号* r15;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_630;

  for_cond_630:
  r0 = cn_var_i_0;
  r1 = cn_var_管理器->当前作用域;
  r2 = r1->符号数量;
  r3 = r0 < r2;
  if (r3) goto for_body_631; else goto for_exit_633;

  for_body_631:
  r4 = cn_var_管理器->当前作用域;
  r5 = r4->符号表;
  r6 = cn_var_i_0;
  r7 = (struct 符号*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7->名称;
  r9 = cn_var_名称;
  r10 = 比较字符串(r8, r9);
  r11 = r10 == 0;
  if (r11) goto if_then_634; else goto if_merge_635;

  for_update_632:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_630;

  for_exit_633:
  return 0;

  if_then_634:
  r12 = cn_var_管理器->当前作用域;
  r13 = r12->符号表;
  r14 = cn_var_i_0;
  r15 = (struct 符号*)cn_rt_array_get_element(r13, r14, 8);
  return r15;
  goto if_merge_635;

  if_merge_635:
  goto for_update_632;
  return NULL;
}

struct 符号* 查找符号(struct 符号表管理器* cn_var_管理器, const char* cn_var_名称) {
  long long r2, r3, r4, r5, r6, r7, r9, r11, r12, r13, r14, r16, r17, r18;
  char* r10;
  struct 作用域* r0;
  struct 作用域* r1;
  void* r8;
  void* r15;

  entry:
  long long cn_var_作用域指针_0;
  r0 = cn_var_管理器->当前作用域;
  cn_var_作用域指针_0 = r0;
  goto while_cond_636;

  while_cond_636:
  r1 = cn_var_作用域指针_0;
  r2 = r1 != 0;
  if (r2) goto while_body_637; else goto while_exit_638;

  while_body_637:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_639;

  while_exit_638:
  return 0;

  for_cond_639:
  r3 = cn_var_i_1;
  r4 = cn_var_作用域指针_0.符号数量;
  r5 = r3 < r4;
  if (r5) goto for_body_640; else goto for_exit_642;

  for_body_640:
  r6 = cn_var_作用域指针_0.符号表;
  r7 = cn_var_i_1;
  r8 = (void*)cn_rt_array_get_element(r6, r7, 8);
  r9 = r8->名称;
  r10 = cn_var_名称;
  r11 = 比较字符串(r9, r10);
  r12 = r11 == 0;
  if (r12) goto if_then_643; else goto if_merge_644;

  for_update_641:
  r16 = cn_var_i_1;
  r17 = r16 + 1;
  cn_var_i_1 = r17;
  goto for_cond_639;

  for_exit_642:
  r18 = cn_var_作用域指针_0.父作用域;
  cn_var_作用域指针_0 = r18;
  goto while_cond_636;

  if_then_643:
  r13 = cn_var_作用域指针_0.符号表;
  r14 = cn_var_i_1;
  r15 = (void*)cn_rt_array_get_element(r13, r14, 8);
  return r15;
  goto if_merge_644;

  if_merge_644:
  goto for_update_641;
  return NULL;
}

struct 符号* 查找全局符号(struct 符号表管理器* cn_var_管理器, const char* cn_var_名称) {
  long long r0, r2, r3, r6, r10, r11, r14, r16, r17;
  char* r8;
  char* r9;
  struct 符号** r5;
  struct 符号** r13;
  struct 作用域* r1;
  struct 作用域* r4;
  struct 符号* r7;
  struct 作用域* r12;
  struct 符号* r15;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_645;

  for_cond_645:
  r0 = cn_var_i_0;
  r1 = cn_var_管理器->全局作用域;
  r2 = r1->符号数量;
  r3 = r0 < r2;
  if (r3) goto for_body_646; else goto for_exit_648;

  for_body_646:
  r4 = cn_var_管理器->全局作用域;
  r5 = r4->符号表;
  r6 = cn_var_i_0;
  r7 = (struct 符号*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7->名称;
  r9 = cn_var_名称;
  r10 = 比较字符串(r8, r9);
  r11 = r10 == 0;
  if (r11) goto if_then_649; else goto if_merge_650;

  for_update_647:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_645;

  for_exit_648:
  return 0;

  if_then_649:
  r12 = cn_var_管理器->全局作用域;
  r13 = r12->符号表;
  r14 = cn_var_i_0;
  r15 = (struct 符号*)cn_rt_array_get_element(r13, r14, 8);
  return r15;
  goto if_merge_650;

  if_merge_650:
  goto for_update_647;
  return NULL;
}

struct 符号* 在作用域查找符号(struct 作用域* cn_var_目标作用域, const char* cn_var_名称) {
  long long r0, r1, r2, r4, r8, r9, r11, r13, r14;
  char* r6;
  char* r7;
  struct 符号** r3;
  struct 符号** r10;
  struct 符号* r5;
  struct 符号* r12;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_651;

  for_cond_651:
  r0 = cn_var_i_0;
  r1 = cn_var_目标作用域->符号数量;
  r2 = r0 < r1;
  if (r2) goto for_body_652; else goto for_exit_654;

  for_body_652:
  r3 = cn_var_目标作用域->符号表;
  r4 = cn_var_i_0;
  r5 = (struct 符号*)cn_rt_array_get_element(r3, r4, 8);
  r6 = r5->名称;
  r7 = cn_var_名称;
  r8 = 比较字符串(r6, r7);
  r9 = r8 == 0;
  if (r9) goto if_then_655; else goto if_merge_656;

  for_update_653:
  r13 = cn_var_i_0;
  r14 = r13 + 1;
  cn_var_i_0 = r14;
  goto for_cond_651;

  for_exit_654:
  return 0;

  if_then_655:
  r10 = cn_var_目标作用域->符号表;
  r11 = cn_var_i_0;
  r12 = (struct 符号*)cn_rt_array_get_element(r10, r11, 8);
  return r12;
  goto if_merge_656;

  if_merge_656:
  goto for_update_653;
  return NULL;
}

struct 符号* 查找类成员(struct 符号* cn_var_类符号, const char* cn_var_成员名) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26;

  entry:
  goto tail_rec_loop;
  return NULL;
}

char* 获取符号类型名称(enum 符号类型 cn_var_类型) {
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
  enum 符号类型 r0;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 符号类型_变量符号;
  if (r1) goto case_body_670; else goto switch_check_683;

  switch_check_683:
  r2 = r0 == 符号类型_函数符号;
  if (r2) goto case_body_671; else goto switch_check_684;

  switch_check_684:
  r3 = r0 == 符号类型_参数符号;
  if (r3) goto case_body_672; else goto switch_check_685;

  switch_check_685:
  r4 = r0 == 符号类型_结构体符号;
  if (r4) goto case_body_673; else goto switch_check_686;

  switch_check_686:
  r5 = r0 == 符号类型_枚举符号;
  if (r5) goto case_body_674; else goto switch_check_687;

  switch_check_687:
  r6 = r0 == 符号类型_枚举成员符号;
  if (r6) goto case_body_675; else goto switch_check_688;

  switch_check_688:
  r7 = r0 == 符号类型_类符号;
  if (r7) goto case_body_676; else goto switch_check_689;

  switch_check_689:
  r8 = r0 == 符号类型_接口符号;
  if (r8) goto case_body_677; else goto switch_check_690;

  switch_check_690:
  r9 = r0 == 符号类型_类成员符号;
  if (r9) goto case_body_678; else goto switch_check_691;

  switch_check_691:
  r10 = r0 == 符号类型_模块符号;
  if (r10) goto case_body_679; else goto switch_check_692;

  switch_check_692:
  r11 = r0 == 符号类型_导入符号;
  if (r11) goto case_body_680; else goto switch_check_693;

  switch_check_693:
  r12 = r0 == 符号类型_类型参数符号;
  if (r12) goto case_body_681; else goto case_default_682;

  case_body_670:
  return "变量";
  goto switch_merge_669;

  case_body_671:
  return "函数";
  goto switch_merge_669;

  case_body_672:
  return "参数";
  goto switch_merge_669;

  case_body_673:
  return "结构体";
  goto switch_merge_669;

  case_body_674:
  return "枚举";
  goto switch_merge_669;

  case_body_675:
  return "枚举成员";
  goto switch_merge_669;

  case_body_676:
  return "类";
  goto switch_merge_669;

  case_body_677:
  return "接口";
  goto switch_merge_669;

  case_body_678:
  return "类成员";
  goto switch_merge_669;

  case_body_679:
  return "模块";
  goto switch_merge_669;

  case_body_680:
  return "导入";
  goto switch_merge_669;

  case_body_681:
  return "类型参数";
  goto switch_merge_669;

  case_default_682:
  return "未知";
  goto switch_merge_669;

  switch_merge_669:
  return NULL;
}

char* 获取作用域类型名称(enum 作用域类型 cn_var_类型) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  enum 作用域类型 r0;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 作用域类型_全局作用域;
  if (r1) goto case_body_695; else goto switch_check_704;

  switch_check_704:
  r2 = r0 == 作用域类型_函数作用域;
  if (r2) goto case_body_696; else goto switch_check_705;

  switch_check_705:
  r3 = r0 == 作用域类型_块作用域;
  if (r3) goto case_body_697; else goto switch_check_706;

  switch_check_706:
  r4 = r0 == 作用域类型_类作用域;
  if (r4) goto case_body_698; else goto switch_check_707;

  switch_check_707:
  r5 = r0 == 作用域类型_结构体作用域;
  if (r5) goto case_body_699; else goto switch_check_708;

  switch_check_708:
  r6 = r0 == 作用域类型_枚举作用域;
  if (r6) goto case_body_700; else goto switch_check_709;

  switch_check_709:
  r7 = r0 == 作用域类型_模块作用域;
  if (r7) goto case_body_701; else goto switch_check_710;

  switch_check_710:
  r8 = r0 == 作用域类型_循环作用域;
  if (r8) goto case_body_702; else goto case_default_703;

  case_body_695:
  return "全局";
  goto switch_merge_694;

  case_body_696:
  return "函数";
  goto switch_merge_694;

  case_body_697:
  return "块";
  goto switch_merge_694;

  case_body_698:
  return "类";
  goto switch_merge_694;

  case_body_699:
  return "结构体";
  goto switch_merge_694;

  case_body_700:
  return "枚举";
  goto switch_merge_694;

  case_body_701:
  return "模块";
  goto switch_merge_694;

  case_body_702:
  return "循环";
  goto switch_merge_694;

  case_default_703:
  return "未知";
  goto switch_merge_694;

  switch_merge_694:
  return NULL;
}

_Bool 检查符号可访问性(struct 符号* cn_var_符号指针, struct 作用域* cn_var_访问者作用域) {
  long long r6, r10, r11, r12, r14, r15;
  struct 作用域* r4;
  struct 作用域* r5;
  struct 作用域* r9;
  struct 作用域* r13;
  _Bool r1;
  _Bool r3;
  _Bool r8;
  struct 符号标志 r0;
  struct 符号标志 r2;
  struct 符号标志 r7;

  entry:
  r0 = cn_var_符号指针->标志;
  r1 = r0.是公开;
  if (r1) goto if_then_711; else goto if_merge_712;

  if_then_711:
  return 1;
  goto if_merge_712;

  if_merge_712:
  r2 = cn_var_符号指针->标志;
  r3 = r2.是私有;
  if (r3) goto if_then_713; else goto if_merge_714;

  if_then_713:
  r4 = cn_var_符号指针->所属作用域;
  r5 = cn_var_访问者作用域;
  r6 = r4 == r5;
  return r6;
  goto if_merge_714;

  if_merge_714:
  r7 = cn_var_符号指针->标志;
  r8 = r7.是保护;
  if (r8) goto if_then_715; else goto if_merge_716;

  if_then_715:
  long long cn_var_当前_0;
  r9 = cn_var_访问者作用域;
  cn_var_当前_0 = r9;
  goto while_cond_717;

  if_merge_716:
  return 1;

  while_cond_717:
  r10 = cn_var_当前_0;
  r11 = r10 != 0;
  if (r11) goto while_body_718; else goto while_exit_719;

  while_body_718:
  r12 = cn_var_当前_0;
  r13 = cn_var_符号指针->所属作用域;
  r14 = r12 == r13;
  if (r14) goto if_then_720; else goto if_merge_721;

  while_exit_719:
  return 0;
  goto if_merge_716;

  if_then_720:
  return 1;
  goto if_merge_721;

  if_merge_721:
  r15 = cn_var_当前_0.父作用域;
  cn_var_当前_0 = r15;
  goto while_cond_717;
  return 0;
}

