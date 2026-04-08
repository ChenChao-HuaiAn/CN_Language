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
struct 表达式节点;
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    long long* 语句体;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    long long* 语句体;
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
struct 符号表管理器;
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};
struct 符号;
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
struct 符号标志;
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
struct 作用域;
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
struct 符号表管理器;
struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};

// Forward Declarations - 从导入模块
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
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, char*, char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, char*);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, char*);
struct 源位置 创建未知位置(void);
struct 源位置 创建源位置(char*, long long, long long);
void 清空诊断集合(struct 诊断集合*);
void 打印所有诊断(struct 诊断集合*);
void 打印诊断信息(struct 诊断信息*);
char* 获取严重级别字符串(enum 诊断严重级别);
_Bool 应该继续(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
_Bool 有错误(struct 诊断集合*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, char*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 释放诊断集合(struct 诊断集合*);
struct 诊断集合* 创建诊断集合(long long);
void* 数组获取(void*, long long);
long long 获取位置参数个数(void);
char* 获取位置参数(long long);
long long 选项存在(char*);
char* 查找选项(char*);
char* 获取程序名称(void);
char* 获取参数(long long);
long long 获取参数个数(void);
long long 求最小值(long long, long long);
long long 求最大值(long long, long long);
long long 获取绝对值(long long);
char* 读取行(void);
long long 字符串格式化(char*, long long, char*);
char* 字符串格式(char*);
long long 类型大小(long long);
void* 分配内存数组(long long, long long);
struct 符号* 创建变量符号(char*, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建函数符号(char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建参数符号(char*, struct 类型节点*, struct 源位置);
struct 符号* 创建结构体符号(char*, struct 源位置);
struct 符号* 创建枚举符号(char*, struct 源位置);
struct 符号* 创建枚举成员符号(char*, long long, _Bool, struct 源位置);
struct 符号* 创建类符号(char*, struct 源位置, _Bool);
struct 符号* 创建接口符号(char*, struct 源位置);
struct 作用域* 创建作用域(enum 作用域类型, char*, struct 作用域*);
void 销毁作用域(struct 作用域*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁符号表管理器(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, char*, struct 符号*);
void 离开作用域(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
_Bool 在循环体内(struct 符号表管理器*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, char*);
struct 符号* 查找符号(struct 符号表管理器*, char*);
struct 符号* 查找全局符号(struct 符号表管理器*, char*);
struct 符号* 在作用域查找符号(struct 作用域*, char*);
struct 符号* 查找类成员(struct 符号*, char*);
char* 获取符号类型名称(enum 符号类型);
char* 获取作用域类型名称(enum 作用域类型);
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;
void 程序添加声明(struct 程序节点*, struct 声明节点*);
struct 程序节点* 创建程序节点(void);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);

// CN Language Enum Definitions
enum IR值类型 {
    IR值类型_无类型,
    IR值类型_整数类型,
    IR值类型_小数类型,
    IR值类型_布尔类型,
    IR值类型_字符串类型,
    IR值类型_指针类型,
    IR值类型_数组类型,
    IR值类型_结构体类型,
    IR值类型_函数类型,
    IR值类型_空值类型,
    IR值类型_标签类型
};

enum IR指令种类 {
    IR指令种类_加法指令,
    IR指令种类_减法指令,
    IR指令种类_乘法指令,
    IR指令种类_除法指令,
    IR指令种类_取模指令,
    IR指令种类_与指令,
    IR指令种类_或指令,
    IR指令种类_异或指令,
    IR指令种类_左移指令,
    IR指令种类_右移指令,
    IR指令种类_负号指令,
    IR指令种类_取反指令,
    IR指令种类_逻辑非指令,
    IR指令种类_相等指令,
    IR指令种类_不等指令,
    IR指令种类_小于指令,
    IR指令种类_小于等于指令,
    IR指令种类_大于指令,
    IR指令种类_大于等于指令,
    IR指令种类_分配指令,
    IR指令种类_加载指令,
    IR指令种类_存储指令,
    IR指令种类_移动指令,
    IR指令种类_取地址指令,
    IR指令种类_解引用指令,
    IR指令种类_标签指令,
    IR指令种类_跳转指令,
    IR指令种类_条件跳转指令,
    IR指令种类_调用指令,
    IR指令种类_返回指令,
    IR指令种类_成员指针指令,
    IR指令种类_成员访问指令,
    IR指令种类_结构体初始化指令,
    IR指令种类_PHI指令,
    IR指令种类_选择指令
};

enum IR操作数种类 {
    IR操作数种类_无操作数,
    IR操作数种类_虚拟寄存器,
    IR操作数种类_整数常量,
    IR操作数种类_小数常量,
    IR操作数种类_字符串常量,
    IR操作数种类_全局符号,
    IR操作数种类_基本块标签,
    IR操作数种类_AST表达式
};

enum IR操作码 {
    IR操作码_加法指令,
    IR操作码_减法指令,
    IR操作码_乘法指令,
    IR操作码_除法指令,
    IR操作码_取模指令,
    IR操作码_与指令,
    IR操作码_或指令,
    IR操作码_异或指令,
    IR操作码_左移指令,
    IR操作码_右移指令,
    IR操作码_负号指令,
    IR操作码_取反指令,
    IR操作码_逻辑非指令,
    IR操作码_相等指令,
    IR操作码_不等指令,
    IR操作码_小于指令,
    IR操作码_小于等于指令,
    IR操作码_大于指令,
    IR操作码_大于等于指令,
    IR操作码_分配指令,
    IR操作码_加载指令,
    IR操作码_存储指令,
    IR操作码_移动指令,
    IR操作码_取地址指令,
    IR操作码_解引用指令,
    IR操作码_标签指令,
    IR操作码_跳转指令,
    IR操作码_条件跳转指令,
    IR操作码_调用指令,
    IR操作码_返回指令,
    IR操作码_成员指针指令,
    IR操作码_成员访问指令,
    IR操作码_结构体初始化指令,
    IR操作码_PHI指令,
    IR操作码_选择指令
};

enum 编译模式 {
    编译模式_宿主模式,
    编译模式_目标模式
};

// CN Language Global Struct Forward Declarations
struct IR操作数;
struct IR指令;
struct 基本块节点;
struct 基本块;
struct 静态变量;
struct IR函数;
struct 目标三元组;
struct 全局变量;
struct IR模块;

// CN Language Global Struct Definitions
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

struct 基本块节点 {
    struct 基本块* 块;
    struct 基本块节点* 下一个;
};

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

struct 静态变量 {
    char* 名称;
    long long 名称长度;
    struct 类型节点* 类型信息;
    struct IR操作数 初始值;
    struct 静态变量* 下一个;
};

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

struct 目标三元组 {
    char* 架构;
    char* 厂商;
    char* 系统;
    char* 环境;
};

struct 全局变量 {
    char* 名称;
    struct 类型节点* 类型信息;
    struct IR操作数 初始值;
    _Bool 是常量;
    struct 全局变量* 下一个;
};

struct IR模块 {
    struct IR函数* 首个函数;
    struct IR函数* 末个函数;
    struct 全局变量* 首个全局;
    struct 全局变量* 末个全局;
    struct 目标三元组 目标信息;
    enum 编译模式 编译模式值;
};

// Global Variables

// Forward Declarations
struct IR模块* 创建IR模块();
void 释放IR模块(struct IR模块*);
struct IR函数* 创建IR函数(char*, struct 类型节点*);
void 释放IR函数(struct IR函数*);
void 添加函数参数(struct IR函数*, struct IR操作数);
void 添加局部变量(struct IR函数*, struct IR操作数);
void 添加基本块(struct IR函数*, struct 基本块*);
void 添加静态变量(struct IR函数*, struct 静态变量*);
long long 分配寄存器(struct IR函数*);
struct 基本块* 创建基本块(char*);
void 释放基本块(struct 基本块*);
void 释放基本块列表(struct 基本块节点*);
void 添加指令(struct 基本块*, struct IR指令*);
void 连接基本块(struct 基本块*, struct 基本块*);
struct IR指令* 创建IR指令(enum IR操作码, struct IR操作数, struct IR操作数, struct IR操作数);
void 释放IR指令(struct IR指令*);
struct IR操作数 空操作数();
struct IR操作数 寄存器操作数(long long, struct 类型节点*);
struct IR操作数 整数常量操作数(long long, struct 类型节点*);
struct IR操作数 小数常量操作数(double, struct 类型节点*);
struct IR操作数 字符串常量操作数(char*, struct 类型节点*);
struct IR操作数 全局符号操作数(char*, struct 类型节点*);
struct IR操作数 标签操作数(struct 基本块*);
struct 全局变量* 创建全局变量(char*, struct 类型节点*, struct IR操作数, _Bool);
void 添加全局变量(struct IR模块*, struct 全局变量*);
void 添加函数(struct IR模块*, struct IR函数*);
struct 静态变量* 创建静态变量(char*, struct 类型节点*, struct IR操作数);

struct IR模块* 创建IR模块() {
  long long r0, r5;
  void* r1;
  struct IR模块* r2;
  struct IR模块* r3;
  void* r6;
  struct IR模块* r7;
  struct 目标三元组 r4;

  entry:
  struct IR模块* cn_var_模块_0;
  r0 = sizeof(struct IR模块);
  r1 = 分配内存(r0);
  cn_var_模块_0 = r1;
  r2 = cn_var_模块_0;
  if (r2) goto if_then_2768; else goto if_merge_2769;

  if_then_2768:
  r3 = cn_var_模块_0;
  r4 = r3->目标信息;
  r5 = sizeof(struct 目标三元组);
  r6 = 设置内存(r4, 0, r5);
  goto if_merge_2769;

  if_merge_2769:
  r7 = cn_var_模块_0;
  return r7;
}

void 释放IR模块(struct IR模块* cn_var_模块) {
  long long r1;
  char* r8;
  char* r10;
  struct IR模块* r0;
  struct IR模块* r2;
  struct 全局变量* r3;
  struct 全局变量* r4;
  struct 全局变量* r5;
  struct 全局变量* r6;
  struct 全局变量* r7;
  struct 全局变量* r9;
  struct 全局变量* r11;
  struct 全局变量* r12;
  struct IR模块* r13;
  struct IR函数* r14;
  struct IR函数* r15;
  struct IR函数* r16;
  struct IR函数* r17;
  struct IR函数* r18;
  struct IR函数* r19;
  struct IR模块* r20;

  entry:
  r0 = cn_var_模块;
  r1 = !r0;
  if (r1) goto if_then_2770; else goto if_merge_2771;

  if_then_2770:
  return;
  goto if_merge_2771;

  if_merge_2771:
  struct 全局变量* cn_var_全局_0;
  r2 = cn_var_模块;
  r3 = r2->首个全局;
  cn_var_全局_0 = r3;
  goto while_cond_2772;

  while_cond_2772:
  r4 = cn_var_全局_0;
  if (r4) goto while_body_2773; else goto while_exit_2774;

  while_body_2773:
  struct 全局变量* cn_var_下个_1;
  r5 = cn_var_全局_0;
  r6 = r5->下一个;
  cn_var_下个_1 = r6;
  r7 = cn_var_全局_0;
  r8 = r7->名称;
  if (r8) goto if_then_2775; else goto if_merge_2776;

  while_exit_2774:
  struct IR函数* cn_var_函数指针_2;
  r13 = cn_var_模块;
  r14 = r13->首个函数;
  cn_var_函数指针_2 = r14;
  goto while_cond_2777;

  if_then_2775:
  r9 = cn_var_全局_0;
  r10 = r9->名称;
  释放内存(r10);
  goto if_merge_2776;

  if_merge_2776:
  r11 = cn_var_全局_0;
  释放内存(r11);
  r12 = cn_var_下个_1;
  cn_var_全局_0 = r12;
  goto while_cond_2772;

  while_cond_2777:
  r15 = cn_var_函数指针_2;
  if (r15) goto while_body_2778; else goto while_exit_2779;

  while_body_2778:
  struct IR函数* cn_var_下个函数_3;
  r16 = cn_var_函数指针_2;
  r17 = r16->下一个;
  cn_var_下个函数_3 = r17;
  r18 = cn_var_函数指针_2;
  释放IR函数(r18);
  r19 = cn_var_下个函数_3;
  cn_var_函数指针_2 = r19;
  goto while_cond_2777;

  while_exit_2779:
  r20 = cn_var_模块;
  释放内存(r20);
  return;
}

struct IR函数* 创建IR函数(char* cn_var_名称, struct 类型节点* cn_var_返回类型) {
  long long r0;
  char* r3;
  char* r4;
  void* r1;
  struct IR函数* r2;
  struct 类型节点* r5;
  struct IR函数* r6;

  entry:
  struct IR函数* cn_var_函数指针_0;
  r0 = sizeof(struct IR函数);
  r1 = 分配内存(r0);
  cn_var_函数指针_0 = r1;
  r2 = cn_var_函数指针_0;
  if (r2) goto if_then_2780; else goto if_merge_2781;

  if_then_2780:
  r3 = cn_var_名称;
  r4 = 复制字符串(r3);
  r5 = cn_var_返回类型;
  goto if_merge_2781;

  if_merge_2781:
  r6 = cn_var_函数指针_0;
  return r6;
}

void 释放IR函数(struct IR函数* cn_var_函数指针) {
  long long r1;
  char* r8;
  char* r10;
  char* r29;
  char* r31;
  void* r21;
  void* r23;
  void* r25;
  void* r27;
  struct IR函数* r0;
  struct IR函数* r2;
  struct 静态变量* r3;
  struct 静态变量* r4;
  struct 静态变量* r5;
  struct 静态变量* r6;
  struct 静态变量* r7;
  struct 静态变量* r9;
  struct 静态变量* r11;
  struct 静态变量* r12;
  struct IR函数* r13;
  struct 基本块* r14;
  struct 基本块* r15;
  struct 基本块* r16;
  struct 基本块* r17;
  struct 基本块* r18;
  struct 基本块* r19;
  struct IR函数* r20;
  struct IR函数* r22;
  struct IR函数* r24;
  struct IR函数* r26;
  struct IR函数* r28;
  struct IR函数* r30;
  struct IR函数* r32;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_2782; else goto if_merge_2783;

  if_then_2782:
  return;
  goto if_merge_2783;

  if_merge_2783:
  struct 静态变量* cn_var_当前静态_0;
  r2 = cn_var_函数指针;
  r3 = r2->首个静态变量;
  cn_var_当前静态_0 = r3;
  goto while_cond_2784;

  while_cond_2784:
  r4 = cn_var_当前静态_0;
  if (r4) goto while_body_2785; else goto while_exit_2786;

  while_body_2785:
  struct 静态变量* cn_var_下个_1;
  r5 = cn_var_当前静态_0;
  r6 = r5->下一个;
  cn_var_下个_1 = r6;
  r7 = cn_var_当前静态_0;
  r8 = r7->名称;
  if (r8) goto if_then_2787; else goto if_merge_2788;

  while_exit_2786:
  struct 基本块* cn_var_块_2;
  r13 = cn_var_函数指针;
  r14 = r13->首个块;
  cn_var_块_2 = r14;
  goto while_cond_2789;

  if_then_2787:
  r9 = cn_var_当前静态_0;
  r10 = r9->名称;
  释放内存(r10);
  goto if_merge_2788;

  if_merge_2788:
  r11 = cn_var_当前静态_0;
  释放内存(r11);
  r12 = cn_var_下个_1;
  cn_var_当前静态_0 = r12;
  goto while_cond_2784;

  while_cond_2789:
  r15 = cn_var_块_2;
  if (r15) goto while_body_2790; else goto while_exit_2791;

  while_body_2790:
  struct 基本块* cn_var_下个块_3;
  r16 = cn_var_块_2;
  r17 = r16->下一个;
  cn_var_下个块_3 = r17;
  r18 = cn_var_块_2;
  释放基本块(r18);
  r19 = cn_var_下个块_3;
  cn_var_块_2 = r19;
  goto while_cond_2789;

  while_exit_2791:
  r20 = cn_var_函数指针;
  r21 = r20->参数列表;
  if (r21) goto if_then_2792; else goto if_merge_2793;

  if_then_2792:
  r22 = cn_var_函数指针;
  r23 = r22->参数列表;
  释放内存(r23);
  goto if_merge_2793;

  if_merge_2793:
  r24 = cn_var_函数指针;
  r25 = r24->局部变量;
  if (r25) goto if_then_2794; else goto if_merge_2795;

  if_then_2794:
  r26 = cn_var_函数指针;
  r27 = r26->局部变量;
  释放内存(r27);
  goto if_merge_2795;

  if_merge_2795:
  r28 = cn_var_函数指针;
  r29 = r28->名称;
  if (r29) goto if_then_2796; else goto if_merge_2797;

  if_then_2796:
  r30 = cn_var_函数指针;
  r31 = r30->名称;
  释放内存(r31);
  goto if_merge_2797;

  if_merge_2797:
  r32 = cn_var_函数指针;
  释放内存(r32);
  return;
}

void 添加函数参数(struct IR函数* cn_var_函数指针, struct IR操作数 cn_var_参数) {
  long long r1, r5, r6, r7, r8, r14, r16, r17;
  void* r3;
  void* r12;
  struct IR函数* r0;
  struct IR函数* r2;
  struct IR函数* r4;
  void* r9;
  struct IR函数* r11;
  struct IR函数* r13;
  struct IR函数* r15;
  struct 参数 r10;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_2798; else goto if_merge_2799;

  if_then_2798:
  return;
  goto if_merge_2799;

  if_merge_2799:
  r2 = cn_var_函数指针;
  r3 = r2->参数列表;
  r4 = cn_var_函数指针;
  r5 = r4->参数数量;
  r6 = r5 + 1;
  r7 = sizeof(struct IR操作数);
  r8 = r6 * r7;
  r9 = 重新分配内存(r3, r8);
  r10 = cn_var_参数;
  r11 = cn_var_函数指针;
  r12 = r11->参数列表;
  r13 = cn_var_函数指针;
  r14 = r13->参数数量;
    { long long _tmp_r11 = r10; cn_rt_array_set_element(r12, r14, &_tmp_r11, 8); }
  r15 = cn_var_函数指针;
  r16 = r15->参数数量;
  r17 = r16 + 1;
  return;
}

void 添加局部变量(struct IR函数* cn_var_函数指针, struct IR操作数 cn_var_局部变量值) {
  long long r1, r5, r6, r7, r8, r14, r16, r17;
  void* r3;
  void* r12;
  struct IR函数* r0;
  struct IR函数* r2;
  struct IR函数* r4;
  void* r9;
  struct IR函数* r11;
  struct IR函数* r13;
  struct IR函数* r15;
  struct IR操作数 r10;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_2800; else goto if_merge_2801;

  if_then_2800:
  return;
  goto if_merge_2801;

  if_merge_2801:
  r2 = cn_var_函数指针;
  r3 = r2->局部变量;
  r4 = cn_var_函数指针;
  r5 = r4->局部变量数量;
  r6 = r5 + 1;
  r7 = sizeof(struct IR操作数);
  r8 = r6 * r7;
  r9 = 重新分配内存(r3, r8);
  r10 = cn_var_局部变量值;
  r11 = cn_var_函数指针;
  r12 = r11->局部变量;
  r13 = cn_var_函数指针;
  r14 = r13->局部变量数量;
    { long long _tmp_r12 = r10; cn_rt_array_set_element(r12, r14, &_tmp_r12, 8); }
  r15 = cn_var_函数指针;
  r16 = r15->局部变量数量;
  r17 = r16 + 1;
  return;
}

void 添加基本块(struct IR函数* cn_var_函数指针, struct 基本块* cn_var_块) {
  long long r0, r2, r4, r7;
  struct IR函数* r1;
  struct 基本块* r3;
  struct IR函数* r5;
  struct 基本块* r6;
  struct 基本块* r8;
  struct 基本块* r9;
  struct 基本块* r10;
  struct IR函数* r11;
  struct 基本块* r12;
  struct 基本块* r13;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_2805; else goto logic_rhs_2804;

  if_then_2802:
  return;
  goto if_merge_2803;

  if_merge_2803:
  r5 = cn_var_函数指针;
  r6 = r5->首个块;
  r7 = !r6;
  if (r7) goto if_then_2806; else goto if_else_2807;

  logic_rhs_2804:
  r3 = cn_var_块;
  r4 = !r3;
  goto logic_merge_2805;

  logic_merge_2805:
  if (r4) goto if_then_2802; else goto if_merge_2803;

  if_then_2806:
  r8 = cn_var_块;
  r9 = cn_var_块;
  goto if_merge_2808;

  if_else_2807:
  r10 = cn_var_块;
  r11 = cn_var_函数指针;
  r12 = r11->末个块;
  r13 = cn_var_块;
  goto if_merge_2808;

  if_merge_2808:
  return;
}

void 添加静态变量(struct IR函数* cn_var_函数指针, struct 静态变量* cn_var_静态变量指针) {
  long long r0, r2, r4, r7;
  struct IR函数* r1;
  struct 静态变量* r3;
  struct IR函数* r5;
  struct 静态变量* r6;
  struct 静态变量* r8;
  struct 静态变量* r9;
  struct 静态变量* r10;
  struct 静态变量* r11;

  entry:
  r1 = cn_var_函数指针;
  r2 = !r1;
  if (r2) goto logic_merge_2812; else goto logic_rhs_2811;

  if_then_2809:
  return;
  goto if_merge_2810;

  if_merge_2810:
  r5 = cn_var_函数指针;
  r6 = r5->首个静态变量;
  r7 = !r6;
  if (r7) goto if_then_2813; else goto if_else_2814;

  logic_rhs_2811:
  r3 = cn_var_静态变量指针;
  r4 = !r3;
  goto logic_merge_2812;

  logic_merge_2812:
  if (r4) goto if_then_2809; else goto if_merge_2810;

  if_then_2813:
  r8 = cn_var_静态变量指针;
  r9 = cn_var_静态变量指针;
  goto if_merge_2815;

  if_else_2814:
  r10 = cn_var_静态变量指针;
  r11 = cn_var_静态变量指针;
  goto if_merge_2815;

  if_merge_2815:
  return;
}

long long 分配寄存器(struct IR函数* cn_var_函数指针) {
  long long r1, r2, r4, r6, r7, r8;
  struct IR函数* r0;
  struct IR函数* r3;
  struct IR函数* r5;

  entry:
  r0 = cn_var_函数指针;
  r1 = !r0;
  if (r1) goto if_then_2816; else goto if_merge_2817;

  if_then_2816:
  r2 = -1;
  return r2;
  goto if_merge_2817;

  if_merge_2817:
  long long cn_var_编号_0;
  r3 = cn_var_函数指针;
  r4 = r3->下个寄存器编号;
  cn_var_编号_0 = r4;
  r5 = cn_var_函数指针;
  r6 = r5->下个寄存器编号;
  r7 = r6 + 1;
  r8 = cn_var_编号_0;
  return r8;
}

struct 基本块* 创建基本块(char* cn_var_名称提示) {
  long long r0, r6;
  char* r3;
  char* r4;
  char* r5;
  void* r1;
  struct 基本块* r2;
  struct 基本块* r7;

  entry:
  struct 基本块* cn_var_块_0;
  r0 = sizeof(struct 基本块);
  r1 = 分配内存(r0);
  cn_var_块_0 = r1;
  r2 = cn_var_块_0;
  if (r2) goto if_then_2818; else goto if_merge_2819;

  if_then_2818:
  r3 = cn_var_名称提示;
  r4 = cn_var_名称提示;
  r5 = 复制字符串(r4);
  r6 = (r3 ? r5 : 0);
  goto if_merge_2819;

  if_merge_2819:
  r7 = cn_var_块_0;
  return r7;
}

void 释放基本块(struct 基本块* cn_var_块) {
  long long r1;
  char* r14;
  char* r16;
  struct 基本块* r0;
  struct 基本块* r2;
  struct IR指令* r3;
  struct IR指令* r4;
  struct IR指令* r5;
  struct IR指令* r6;
  struct IR指令* r7;
  struct IR指令* r8;
  struct 基本块* r9;
  struct 基本块节点* r10;
  struct 基本块* r11;
  struct 基本块节点* r12;
  struct 基本块* r13;
  struct 基本块* r15;
  struct 基本块* r17;

  entry:
  r0 = cn_var_块;
  r1 = !r0;
  if (r1) goto if_then_2820; else goto if_merge_2821;

  if_then_2820:
  return;
  goto if_merge_2821;

  if_merge_2821:
  struct IR指令* cn_var_指令_0;
  r2 = cn_var_块;
  r3 = r2->首条指令;
  cn_var_指令_0 = r3;
  goto while_cond_2822;

  while_cond_2822:
  r4 = cn_var_指令_0;
  if (r4) goto while_body_2823; else goto while_exit_2824;

  while_body_2823:
  struct IR指令* cn_var_下条_1;
  r5 = cn_var_指令_0;
  r6 = r5->下一条;
  cn_var_下条_1 = r6;
  r7 = cn_var_指令_0;
  释放IR指令(r7);
  r8 = cn_var_下条_1;
  cn_var_指令_0 = r8;
  goto while_cond_2822;

  while_exit_2824:
  r9 = cn_var_块;
  r10 = r9->前驱列表;
  释放基本块列表(r10);
  r11 = cn_var_块;
  r12 = r11->后继列表;
  释放基本块列表(r12);
  r13 = cn_var_块;
  r14 = r13->名称;
  if (r14) goto if_then_2825; else goto if_merge_2826;

  if_then_2825:
  r15 = cn_var_块;
  r16 = r15->名称;
  释放内存(r16);
  goto if_merge_2826;

  if_merge_2826:
  r17 = cn_var_块;
  释放内存(r17);
  return;
}

void 释放基本块列表(struct 基本块节点* cn_var_列表) {
  struct 基本块节点* r0;
  struct 基本块节点* r1;
  struct 基本块节点* r2;
  struct 基本块节点* r3;
  struct 基本块节点* r4;

  entry:
  goto while_cond_2827;

  while_cond_2827:
  r0 = cn_var_列表;
  if (r0) goto while_body_2828; else goto while_exit_2829;

  while_body_2828:
  struct 基本块节点* cn_var_下个_0;
  r1 = cn_var_列表;
  r2 = r1->下一个;
  cn_var_下个_0 = r2;
  r3 = cn_var_列表;
  释放内存(r3);
  r4 = cn_var_下个_0;
  cn_var_列表 = r4;
  goto while_cond_2827;

  while_exit_2829:
  return;
}

void 添加指令(struct 基本块* cn_var_块, struct IR指令* cn_var_指令) {
  long long r0, r2, r4, r7;
  struct 基本块* r1;
  struct IR指令* r3;
  struct 基本块* r5;
  struct IR指令* r6;
  struct IR指令* r8;
  struct IR指令* r9;
  struct IR指令* r10;
  struct 基本块* r11;
  struct IR指令* r12;
  struct IR指令* r13;

  entry:
  r1 = cn_var_块;
  r2 = !r1;
  if (r2) goto logic_merge_2833; else goto logic_rhs_2832;

  if_then_2830:
  return;
  goto if_merge_2831;

  if_merge_2831:
  r5 = cn_var_块;
  r6 = r5->首条指令;
  r7 = !r6;
  if (r7) goto if_then_2834; else goto if_else_2835;

  logic_rhs_2832:
  r3 = cn_var_指令;
  r4 = !r3;
  goto logic_merge_2833;

  logic_merge_2833:
  if (r4) goto if_then_2830; else goto if_merge_2831;

  if_then_2834:
  r8 = cn_var_指令;
  r9 = cn_var_指令;
  goto if_merge_2836;

  if_else_2835:
  r10 = cn_var_指令;
  r11 = cn_var_块;
  r12 = r11->末条指令;
  r13 = cn_var_指令;
  goto if_merge_2836;

  if_merge_2836:
  return;
}

void 连接基本块(struct 基本块* cn_var_源块, struct 基本块* cn_var_目标块) {
  long long r0, r2, r4, r5, r11;
  struct 基本块* r1;
  struct 基本块* r3;
  void* r6;
  struct 基本块* r7;
  struct 基本块* r8;
  struct 基本块节点* r9;
  struct 基本块节点* r10;
  void* r12;
  struct 基本块* r13;
  struct 基本块* r14;
  struct 基本块节点* r15;
  struct 基本块节点* r16;

  entry:
  r1 = cn_var_源块;
  r2 = !r1;
  if (r2) goto logic_merge_2840; else goto logic_rhs_2839;

  if_then_2837:
  return;
  goto if_merge_2838;

  if_merge_2838:
  struct 基本块节点* cn_var_后继节点_0;
  r5 = sizeof(struct 基本块节点);
  r6 = 分配内存(r5);
  cn_var_后继节点_0 = r6;
  r7 = cn_var_目标块;
  r8 = cn_var_源块;
  r9 = r8->后继列表;
  r10 = cn_var_后继节点_0;
  struct 基本块节点* cn_var_前驱节点_1;
  r11 = sizeof(struct 基本块节点);
  r12 = 分配内存(r11);
  cn_var_前驱节点_1 = r12;
  r13 = cn_var_源块;
  r14 = cn_var_目标块;
  r15 = r14->前驱列表;
  r16 = cn_var_前驱节点_1;

  logic_rhs_2839:
  r3 = cn_var_目标块;
  r4 = !r3;
  goto logic_merge_2840;

  logic_merge_2840:
  if (r4) goto if_then_2837; else goto if_merge_2838;
  return;
}

struct IR指令* 创建IR指令(enum IR操作码 cn_var_操作码, struct IR操作数 cn_var_目标, struct IR操作数 cn_var_源1, struct IR操作数 cn_var_源2) {
  long long r0;
  void* r1;
  struct IR指令* r2;
  struct IR指令* r7;
  struct IR操作数 r4;
  struct IR操作数 r5;
  struct IR操作数 r6;
  enum IR操作码 r3;

  entry:
  struct IR指令* cn_var_指令_0;
  r0 = sizeof(struct IR指令);
  r1 = 分配内存(r0);
  cn_var_指令_0 = r1;
  r2 = cn_var_指令_0;
  if (r2) goto if_then_2841; else goto if_merge_2842;

  if_then_2841:
  r3 = cn_var_操作码;
  r4 = cn_var_目标;
  r5 = cn_var_源1;
  r6 = cn_var_源2;
  goto if_merge_2842;

  if_merge_2842:
  r7 = cn_var_指令_0;
  return r7;
}

void 释放IR指令(struct IR指令* cn_var_指令) {
  long long r1;
  void* r3;
  void* r5;
  struct IR指令* r0;
  struct IR指令* r2;
  struct IR指令* r4;
  struct IR指令* r6;

  entry:
  r0 = cn_var_指令;
  r1 = !r0;
  if (r1) goto if_then_2843; else goto if_merge_2844;

  if_then_2843:
  return;
  goto if_merge_2844;

  if_merge_2844:
  r2 = cn_var_指令;
  r3 = r2->额外参数;
  if (r3) goto if_then_2845; else goto if_merge_2846;

  if_then_2845:
  r4 = cn_var_指令;
  r5 = r4->额外参数;
  释放内存(r5);
  goto if_merge_2846;

  if_merge_2846:
  r6 = cn_var_指令;
  释放内存(r6);
  return;
}

struct IR操作数 空操作数() {
  struct IR操作数 r0;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_操作数_0;
  return r0;
}

struct IR操作数 寄存器操作数(long long cn_var_编号, struct 类型节点* cn_var_类型) {
  long long r0;
  struct 类型节点* r1;
  struct IR操作数 r2;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_编号;
  r1 = cn_var_类型;
  r2 = cn_var_操作数_0;
  return r2;
}

struct IR操作数 整数常量操作数(long long cn_var_值, struct 类型节点* cn_var_类型) {
  long long r0;
  struct 类型节点* r1;
  struct IR操作数 r2;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_值;
  r1 = cn_var_类型;
  r2 = cn_var_操作数_0;
  return r2;
}

struct IR操作数 小数常量操作数(double cn_var_值, struct 类型节点* cn_var_类型) {
  struct 类型节点* r1;
  double r0;
  struct IR操作数 r2;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_值;
  r1 = cn_var_类型;
  r2 = cn_var_操作数_0;
  return r2;
}

struct IR操作数 字符串常量操作数(char* cn_var_值, struct 类型节点* cn_var_类型) {
  char* r0;
  struct 类型节点* r1;
  struct IR操作数 r2;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_值;
  r1 = cn_var_类型;
  r2 = cn_var_操作数_0;
  return r2;
}

struct IR操作数 全局符号操作数(char* cn_var_名称, struct 类型节点* cn_var_类型) {
  char* r0;
  struct 类型节点* r1;
  struct IR操作数 r2;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_名称;
  r1 = cn_var_类型;
  r2 = cn_var_操作数_0;
  return r2;
}

struct IR操作数 标签操作数(struct 基本块* cn_var_块) {
  struct 基本块* r0;
  struct IR操作数 r1;

  entry:
  struct IR操作数 cn_var_操作数_0;
  r0 = cn_var_块;
  r1 = cn_var_操作数_0;
  return r1;
}

struct 全局变量* 创建全局变量(char* cn_var_名称, struct 类型节点* cn_var_类型, struct IR操作数 cn_var_初始值, _Bool cn_var_是常量) {
  long long r0;
  char* r3;
  char* r4;
  void* r1;
  struct 全局变量* r2;
  struct 类型节点* r5;
  struct 全局变量* r8;
  _Bool r7;
  struct IR操作数 r6;

  entry:
  struct 全局变量* cn_var_全局变量指针_0;
  r0 = sizeof(struct 全局变量);
  r1 = 分配内存(r0);
  cn_var_全局变量指针_0 = r1;
  r2 = cn_var_全局变量指针_0;
  if (r2) goto if_then_2847; else goto if_merge_2848;

  if_then_2847:
  r3 = cn_var_名称;
  r4 = 复制字符串(r3);
  r5 = cn_var_类型;
  r6 = cn_var_初始值;
  r7 = cn_var_是常量;
  goto if_merge_2848;

  if_merge_2848:
  r8 = cn_var_全局变量指针_0;
  return r8;
}

void 添加全局变量(struct IR模块* cn_var_模块, struct 全局变量* cn_var_全局变量指针) {
  long long r0, r2, r4, r7;
  struct IR模块* r1;
  struct 全局变量* r3;
  struct IR模块* r5;
  struct 全局变量* r6;
  struct 全局变量* r8;
  struct 全局变量* r9;
  struct 全局变量* r10;
  struct 全局变量* r11;

  entry:
  r1 = cn_var_模块;
  r2 = !r1;
  if (r2) goto logic_merge_2852; else goto logic_rhs_2851;

  if_then_2849:
  return;
  goto if_merge_2850;

  if_merge_2850:
  r5 = cn_var_模块;
  r6 = r5->首个全局;
  r7 = !r6;
  if (r7) goto if_then_2853; else goto if_else_2854;

  logic_rhs_2851:
  r3 = cn_var_全局变量指针;
  r4 = !r3;
  goto logic_merge_2852;

  logic_merge_2852:
  if (r4) goto if_then_2849; else goto if_merge_2850;

  if_then_2853:
  r8 = cn_var_全局变量指针;
  r9 = cn_var_全局变量指针;
  goto if_merge_2855;

  if_else_2854:
  r10 = cn_var_全局变量指针;
  r11 = cn_var_全局变量指针;
  goto if_merge_2855;

  if_merge_2855:
  return;
}

void 添加函数(struct IR模块* cn_var_模块, struct IR函数* cn_var_函数指针) {
  long long r0, r2, r4, r7;
  struct IR模块* r1;
  struct IR函数* r3;
  struct IR模块* r5;
  struct IR函数* r6;
  struct IR函数* r8;
  struct IR函数* r9;
  struct IR函数* r10;
  struct IR函数* r11;

  entry:
  r1 = cn_var_模块;
  r2 = !r1;
  if (r2) goto logic_merge_2859; else goto logic_rhs_2858;

  if_then_2856:
  return;
  goto if_merge_2857;

  if_merge_2857:
  r5 = cn_var_模块;
  r6 = r5->首个函数;
  r7 = !r6;
  if (r7) goto if_then_2860; else goto if_else_2861;

  logic_rhs_2858:
  r3 = cn_var_函数指针;
  r4 = !r3;
  goto logic_merge_2859;

  logic_merge_2859:
  if (r4) goto if_then_2856; else goto if_merge_2857;

  if_then_2860:
  r8 = cn_var_函数指针;
  r9 = cn_var_函数指针;
  goto if_merge_2862;

  if_else_2861:
  r10 = cn_var_函数指针;
  r11 = cn_var_函数指针;
  goto if_merge_2862;

  if_merge_2862:
  return;
}

struct 静态变量* 创建静态变量(char* cn_var_名称, struct 类型节点* cn_var_类型, struct IR操作数 cn_var_初始值) {
  long long r0, r6;
  char* r3;
  char* r4;
  char* r5;
  void* r1;
  struct 静态变量* r2;
  struct 类型节点* r7;
  struct 静态变量* r9;
  struct IR操作数 r8;

  entry:
  struct 静态变量* cn_var_静态变量指针_0;
  r0 = sizeof(struct 静态变量);
  r1 = 分配内存(r0);
  cn_var_静态变量指针_0 = r1;
  r2 = cn_var_静态变量指针_0;
  if (r2) goto if_then_2863; else goto if_merge_2864;

  if_then_2863:
  r3 = cn_var_名称;
  r4 = 复制字符串(r3);
  r5 = cn_var_名称;
  r6 = 获取字符串长度(r5);
  r7 = cn_var_类型;
  r8 = cn_var_初始值;
  goto if_merge_2864;

  if_merge_2864:
  r9 = cn_var_静态变量指针_0;
  return r9;
}

