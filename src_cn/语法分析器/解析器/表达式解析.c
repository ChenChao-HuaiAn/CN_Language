#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
enum 逻辑运算符 {
    逻辑运算符_逻辑_或 = 1,
    逻辑运算符_逻辑_与 = 0
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
enum 字面量类型 {
    字面量类型_字面量_字符 = 5,
    字面量类型_字面量_空 = 4,
    字面量类型_字面量_布尔 = 3,
    字面量类型_字面量_字符串 = 2,
    字面量类型_字面量_浮点 = 1,
    字面量类型_字面量_整数 = 0
};
enum 可见性 {
    可见性_可见性_私有 = 2,
    可见性_可见性_公开 = 1,
    可见性_可见性_默认 = 0
};
enum 导入类型 {
    导入类型_导入_从包导入模块 = 5,
    导入类型_导入_从模块通配符 = 4,
    导入类型_导入_从模块选择性 = 3,
    导入类型_导入_别名 = 2,
    导入类型_导入_选择性 = 1,
    导入类型_导入_全量 = 0
};

// Struct Definitions - 从导入模块
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
struct 声明节点列表;
struct AST节点 {
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
struct 类型节点;
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
};
struct 表达式节点;
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
struct 块语句;
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
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 结构体声明;
struct 结构体声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 结构体成员** 成员;
    long long 成员个数;
};
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 枚举声明;
struct 枚举声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 枚举成员** 成员;
    long long 成员个数;
};
struct 节点类型 {
};
struct 类成员;
struct 类成员 {
    char* 名称;
    struct 节点类型 类型;
    struct 变量声明* 字段;
    struct 函数声明* 方法;
    enum 可见性 可见性;
    _Bool 是静态;
    _Bool 是虚拟;
    _Bool 是重写;
    _Bool 是抽象;
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
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
};
struct 接口声明;
struct 接口声明 {
    struct AST节点 节点基类;
    char* 名称;
    struct 接口方法** 方法;
    long long 方法个数;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
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
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
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
struct 声明节点;
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct 声明节点;
struct 声明节点 {
    struct 节点类型 类型;
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
struct 表达式语句;
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};
struct 语句节点;
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
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};
struct 选择语句;
struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
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
struct 逻辑表达式;
struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    enum 逻辑运算符 运算符;
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 结构体字面量表达式;
struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};
struct 模板实例化表达式;
struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};

// Forward Declarations - 从导入模块
struct 程序节点* 创建程序节点(void);
void 程序添加声明(struct 程序节点*, struct 声明节点*);

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
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 赋值运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.赋值;
  r9 = 检查(r6, r8);
  if (r9) goto if_then_1026; else goto if_merge_1027;

  if_then_1026:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析赋值表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_赋值节点_2;
  创建赋值表达式();
  cn_var_赋值节点_2 = /* NONE */;
  r13 = cn_var_赋值节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1028; else goto if_merge_1029;

  if_merge_1027:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1028:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1029;

  if_merge_1029:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_赋值运算符;
  r19 = r18.简单赋值;
  r20 = cn_var_赋值节点_2;
  return r20;
  goto if_merge_1027;
  return NULL;
}

struct 表达式节点* 解析三元表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 表达式节点* r11;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r18;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  struct 表达式节点* r22;
  struct 表达式节点* r23;
  struct 表达式节点* r24;
  struct 表达式节点* r25;
  _Bool r9;
  _Bool r15;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r13;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.问号;
  r9 = 匹配(r6, r8);
  if (r9) goto if_then_1034; else goto if_merge_1035;

  if_then_1034:
  struct 表达式节点* cn_var_真值_1;
  r10 = cn_var_实例;
  r11 = 解析表达式(r10);
  cn_var_真值_1 = r11;
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.冒号;
  r15 = 期望(r12, r14);
  struct 表达式节点* cn_var_假值_2;
  r16 = cn_var_实例;
  r17 = 解析三元表达式(r16);
  cn_var_假值_2 = r17;
  struct 表达式节点* cn_var_三元节点_3;
  创建三元表达式();
  cn_var_三元节点_3 = /* NONE */;
  r18 = cn_var_三元节点_3;
  r19 = r18 == 0;
  if (r19) goto if_then_1036; else goto if_merge_1037;

  if_merge_1035:
  r25 = cn_var_条件_0;
  return r25;

  if_then_1036:
  r20 = cn_var_条件_0;
  return r20;
  goto if_merge_1037;

  if_merge_1037:
  r21 = cn_var_条件_0;
  r22 = cn_var_真值_1;
  r23 = cn_var_假值_2;
  r24 = cn_var_三元节点_3;
  return r24;
  goto if_merge_1035;
  return NULL;
}

struct 表达式节点* 解析逻辑或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 二元运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.逻辑或;
  r9 = 检查(r6, r8);
  if (r9) goto while_body_1043; else goto while_exit_1044;

  while_body_1043:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析逻辑与表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r13 = cn_var_二元节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1045; else goto if_merge_1046;

  while_exit_1044:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1045:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1046;

  if_merge_1046:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_二元运算符;
  r19 = r18.逻辑或;
  r20 = cn_var_二元节点_2;
  cn_var_左值_0 = r20;
  goto while_cond_1042;
  return NULL;
}

struct 表达式节点* 解析逻辑与表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 二元运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.逻辑与;
  r9 = 检查(r6, r8);
  if (r9) goto while_body_1052; else goto while_exit_1053;

  while_body_1052:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析按位或表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r13 = cn_var_二元节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1054; else goto if_merge_1055;

  while_exit_1053:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1054:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1055;

  if_merge_1055:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_二元运算符;
  r19 = r18.逻辑与;
  r20 = cn_var_二元节点_2;
  cn_var_左值_0 = r20;
  goto while_cond_1051;
  return NULL;
}

struct 表达式节点* 解析按位或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 二元运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.按位或;
  r9 = 检查(r6, r8);
  if (r9) goto while_body_1061; else goto while_exit_1062;

  while_body_1061:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析按位异或表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r13 = cn_var_二元节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1063; else goto if_merge_1064;

  while_exit_1062:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1063:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1064;

  if_merge_1064:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_二元运算符;
  r19 = r18.按位或;
  r20 = cn_var_二元节点_2;
  cn_var_左值_0 = r20;
  goto while_cond_1060;
  return NULL;
}

struct 表达式节点* 解析按位异或表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 二元运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.按位异或;
  r9 = 检查(r6, r8);
  if (r9) goto while_body_1070; else goto while_exit_1071;

  while_body_1070:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析按位与表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r13 = cn_var_二元节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1072; else goto if_merge_1073;

  while_exit_1071:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1072:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1073;

  if_merge_1073:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_二元运算符;
  r19 = r18.按位异或;
  r20 = cn_var_二元节点_2;
  cn_var_左值_0 = r20;
  goto while_cond_1069;
  return NULL;
}

struct 表达式节点* 解析按位与表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r14, r19;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式节点* r12;
  struct 表达式节点* r13;
  struct 表达式节点* r15;
  struct 表达式节点* r16;
  struct 表达式节点* r17;
  struct 表达式节点* r20;
  struct 表达式节点* r21;
  _Bool r9;
  enum 词元类型枚举 r7;
  enum 二元运算符 r18;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.按位与;
  r9 = 检查(r6, r8);
  if (r9) goto while_body_1079; else goto while_exit_1080;

  while_body_1079:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式节点* cn_var_右值_1;
  r11 = cn_var_实例;
  r12 = 解析相等表达式(r11);
  cn_var_右值_1 = r12;
  struct 表达式节点* cn_var_二元节点_2;
  创建二元表达式();
  cn_var_二元节点_2 = /* NONE */;
  r13 = cn_var_二元节点_2;
  r14 = r13 == 0;
  if (r14) goto if_then_1081; else goto if_merge_1082;

  while_exit_1080:
  r21 = cn_var_左值_0;
  return r21;

  if_then_1081:
  r15 = cn_var_左值_0;
  return r15;
  goto if_merge_1082;

  if_merge_1082:
  r16 = cn_var_左值_0;
  r17 = cn_var_右值_1;
  r18 = cn_var_二元运算符;
  r19 = r18.按位与;
  r20 = cn_var_二元节点_2;
  cn_var_左值_0 = r20;
  goto while_cond_1078;
  return NULL;
}

struct 表达式节点* 解析相等表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r9, r13, r16, r19, r22, r27;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r17;
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
  _Bool r14;
  _Bool r20;
  enum 二元运算符 r31;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r12;
  enum 二元运算符 r15;
  enum 词元类型枚举 r18;
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
  r8 = cn_var_词元类型枚举;
  r9 = r8.等于;
  r10 = 检查(r7, r9);
  if (r10) goto logic_merge_1091; else goto logic_rhs_1090;

  while_body_1088:
  enum 二元运算符 cn_var_运算符_1;
  r15 = cn_var_二元运算符;
  r16 = r15.等于;
  cn_var_运算符_1 = r16;
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.不等于;
  r20 = 检查(r17, r19);
  if (r20) goto if_then_1092; else goto if_merge_1093;

  while_exit_1089:
  r33 = cn_var_左值_0;
  return r33;

  logic_rhs_1090:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.不等于;
  r14 = 检查(r11, r13);
  goto logic_merge_1091;

  logic_merge_1091:
  if (r14) goto while_body_1088; else goto while_exit_1089;

  if_then_1092:
  r21 = cn_var_二元运算符;
  r22 = r21.不等于;
  cn_var_运算符_1 = r22;
  goto if_merge_1093;

  if_merge_1093:
  r23 = cn_var_实例;
  前进词元(r23);
  struct 表达式节点* cn_var_右值_2;
  r24 = cn_var_实例;
  r25 = 解析比较表达式(r24);
  cn_var_右值_2 = r25;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r26 = cn_var_二元节点_3;
  r27 = r26 == 0;
  if (r27) goto if_then_1094; else goto if_merge_1095;

  if_then_1094:
  r28 = cn_var_左值_0;
  return r28;
  goto if_merge_1095;

  if_merge_1095:
  r29 = cn_var_左值_0;
  r30 = cn_var_右值_2;
  r31 = cn_var_运算符_1;
  r32 = cn_var_二元节点_3;
  cn_var_左值_0 = r32;
  goto while_cond_1087;
  return NULL;
}

struct 表达式节点* 解析比较表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r7, r8, r11, r15, r19, r23, r26, r29, r32, r35, r38, r41, r44, r49;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r9;
  struct 解析器* r13;
  struct 解析器* r17;
  struct 解析器* r21;
  struct 解析器* r27;
  struct 解析器* r33;
  struct 解析器* r39;
  struct 解析器* r45;
  struct 解析器* r46;
  struct 表达式节点* r47;
  struct 表达式节点* r48;
  struct 表达式节点* r50;
  struct 表达式节点* r51;
  struct 表达式节点* r52;
  struct 表达式节点* r54;
  struct 表达式节点* r55;
  _Bool r12;
  _Bool r16;
  _Bool r20;
  _Bool r24;
  _Bool r30;
  _Bool r36;
  _Bool r42;
  enum 二元运算符 r53;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r22;
  enum 二元运算符 r25;
  enum 词元类型枚举 r28;
  enum 二元运算符 r31;
  enum 词元类型枚举 r34;
  enum 二元运算符 r37;
  enum 词元类型枚举 r40;
  enum 二元运算符 r43;

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
  r10 = cn_var_词元类型枚举;
  r11 = r10.小于;
  r12 = 检查(r9, r11);
  if (r12) goto logic_merge_1108; else goto logic_rhs_1107;

  while_body_1101:
  enum 二元运算符 cn_var_运算符_1;
  r25 = cn_var_二元运算符;
  r26 = r25.小于;
  cn_var_运算符_1 = r26;
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.小于等于;
  r30 = 检查(r27, r29);
  if (r30) goto if_then_1109; else goto if_else_1110;

  while_exit_1102:
  r55 = cn_var_左值_0;
  return r55;

  logic_rhs_1103:
  r21 = cn_var_实例;
  r22 = cn_var_词元类型枚举;
  r23 = r22.大于等于;
  r24 = 检查(r21, r23);
  goto logic_merge_1104;

  logic_merge_1104:
  if (r24) goto while_body_1101; else goto while_exit_1102;

  logic_rhs_1105:
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.大于;
  r20 = 检查(r17, r19);
  goto logic_merge_1106;

  logic_merge_1106:
  if (r20) goto logic_merge_1104; else goto logic_rhs_1103;

  logic_rhs_1107:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.小于等于;
  r16 = 检查(r13, r15);
  goto logic_merge_1108;

  logic_merge_1108:
  if (r16) goto logic_merge_1106; else goto logic_rhs_1105;

  if_then_1109:
  r31 = cn_var_二元运算符;
  r32 = r31.小于等于;
  cn_var_运算符_1 = r32;
  goto if_merge_1111;

  if_else_1110:
  r33 = cn_var_实例;
  r34 = cn_var_词元类型枚举;
  r35 = r34.大于;
  r36 = 检查(r33, r35);
  if (r36) goto if_then_1112; else goto if_else_1113;

  if_merge_1111:
  r45 = cn_var_实例;
  前进词元(r45);
  struct 表达式节点* cn_var_右值_2;
  r46 = cn_var_实例;
  r47 = 解析位移表达式(r46);
  cn_var_右值_2 = r47;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r48 = cn_var_二元节点_3;
  r49 = r48 == 0;
  if (r49) goto if_then_1117; else goto if_merge_1118;

  if_then_1112:
  r37 = cn_var_二元运算符;
  r38 = r37.大于;
  cn_var_运算符_1 = r38;
  goto if_merge_1114;

  if_else_1113:
  r39 = cn_var_实例;
  r40 = cn_var_词元类型枚举;
  r41 = r40.大于等于;
  r42 = 检查(r39, r41);
  if (r42) goto if_then_1115; else goto if_merge_1116;

  if_merge_1114:
  goto if_merge_1111;

  if_then_1115:
  r43 = cn_var_二元运算符;
  r44 = r43.大于等于;
  cn_var_运算符_1 = r44;
  goto if_merge_1116;

  if_merge_1116:
  goto if_merge_1114;

  if_then_1117:
  r50 = cn_var_左值_0;
  return r50;
  goto if_merge_1118;

  if_merge_1118:
  r51 = cn_var_左值_0;
  r52 = cn_var_右值_2;
  r53 = cn_var_运算符_1;
  r54 = cn_var_二元节点_3;
  cn_var_左值_0 = r54;
  goto while_cond_1100;
  return NULL;
}

struct 表达式节点* 解析位移表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r9, r13, r16, r19, r22, r27;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r17;
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
  _Bool r14;
  _Bool r20;
  enum 二元运算符 r31;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r12;
  enum 二元运算符 r15;
  enum 词元类型枚举 r18;
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
  r8 = cn_var_词元类型枚举;
  r9 = r8.左移;
  r10 = 检查(r7, r9);
  if (r10) goto logic_merge_1127; else goto logic_rhs_1126;

  while_body_1124:
  enum 二元运算符 cn_var_运算符_1;
  r15 = cn_var_二元运算符;
  r16 = r15.左移;
  cn_var_运算符_1 = r16;
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.右移;
  r20 = 检查(r17, r19);
  if (r20) goto if_then_1128; else goto if_merge_1129;

  while_exit_1125:
  r33 = cn_var_左值_0;
  return r33;

  logic_rhs_1126:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.右移;
  r14 = 检查(r11, r13);
  goto logic_merge_1127;

  logic_merge_1127:
  if (r14) goto while_body_1124; else goto while_exit_1125;

  if_then_1128:
  r21 = cn_var_二元运算符;
  r22 = r21.右移;
  cn_var_运算符_1 = r22;
  goto if_merge_1129;

  if_merge_1129:
  r23 = cn_var_实例;
  前进词元(r23);
  struct 表达式节点* cn_var_右值_2;
  r24 = cn_var_实例;
  r25 = 解析加减表达式(r24);
  cn_var_右值_2 = r25;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r26 = cn_var_二元节点_3;
  r27 = r26 == 0;
  if (r27) goto if_then_1130; else goto if_merge_1131;

  if_then_1130:
  r28 = cn_var_左值_0;
  return r28;
  goto if_merge_1131;

  if_merge_1131:
  r29 = cn_var_左值_0;
  r30 = cn_var_右值_2;
  r31 = cn_var_运算符_1;
  r32 = cn_var_二元节点_3;
  cn_var_左值_0 = r32;
  goto while_cond_1123;
  return NULL;
}

struct 表达式节点* 解析加减表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r9, r13, r16, r19, r22, r27;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r17;
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
  _Bool r14;
  _Bool r20;
  enum 二元运算符 r31;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r12;
  enum 二元运算符 r15;
  enum 词元类型枚举 r18;
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
  r8 = cn_var_词元类型枚举;
  r9 = r8.加号;
  r10 = 检查(r7, r9);
  if (r10) goto logic_merge_1140; else goto logic_rhs_1139;

  while_body_1137:
  enum 二元运算符 cn_var_运算符_1;
  r15 = cn_var_二元运算符;
  r16 = r15.加法;
  cn_var_运算符_1 = r16;
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.减号;
  r20 = 检查(r17, r19);
  if (r20) goto if_then_1141; else goto if_merge_1142;

  while_exit_1138:
  r33 = cn_var_左值_0;
  return r33;

  logic_rhs_1139:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.减号;
  r14 = 检查(r11, r13);
  goto logic_merge_1140;

  logic_merge_1140:
  if (r14) goto while_body_1137; else goto while_exit_1138;

  if_then_1141:
  r21 = cn_var_二元运算符;
  r22 = r21.减法;
  cn_var_运算符_1 = r22;
  goto if_merge_1142;

  if_merge_1142:
  r23 = cn_var_实例;
  前进词元(r23);
  struct 表达式节点* cn_var_右值_2;
  r24 = cn_var_实例;
  r25 = 解析乘除表达式(r24);
  cn_var_右值_2 = r25;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r26 = cn_var_二元节点_3;
  r27 = r26 == 0;
  if (r27) goto if_then_1143; else goto if_merge_1144;

  if_then_1143:
  r28 = cn_var_左值_0;
  return r28;
  goto if_merge_1144;

  if_merge_1144:
  r29 = cn_var_左值_0;
  r30 = cn_var_右值_2;
  r31 = cn_var_运算符_1;
  r32 = cn_var_二元节点_3;
  cn_var_左值_0 = r32;
  goto while_cond_1136;
  return NULL;
}

struct 表达式节点* 解析乘除表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r6, r7, r10, r14, r18, r21, r24, r27, r30, r33, r38;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r8;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 解析器* r22;
  struct 解析器* r28;
  struct 解析器* r34;
  struct 解析器* r35;
  struct 表达式节点* r36;
  struct 表达式节点* r37;
  struct 表达式节点* r39;
  struct 表达式节点* r40;
  struct 表达式节点* r41;
  struct 表达式节点* r43;
  struct 表达式节点* r44;
  _Bool r11;
  _Bool r15;
  _Bool r19;
  _Bool r25;
  _Bool r31;
  enum 二元运算符 r42;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r17;
  enum 二元运算符 r20;
  enum 词元类型枚举 r23;
  enum 二元运算符 r26;
  enum 词元类型枚举 r29;
  enum 二元运算符 r32;

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
  r9 = cn_var_词元类型枚举;
  r10 = r9.星号;
  r11 = 检查(r8, r10);
  if (r11) goto logic_merge_1155; else goto logic_rhs_1154;

  while_body_1150:
  enum 二元运算符 cn_var_运算符_1;
  r20 = cn_var_二元运算符;
  r21 = r20.乘法;
  cn_var_运算符_1 = r21;
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.斜杠;
  r25 = 检查(r22, r24);
  if (r25) goto if_then_1156; else goto if_else_1157;

  while_exit_1151:
  r44 = cn_var_左值_0;
  return r44;

  logic_rhs_1152:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.百分号;
  r19 = 检查(r16, r18);
  goto logic_merge_1153;

  logic_merge_1153:
  if (r19) goto while_body_1150; else goto while_exit_1151;

  logic_rhs_1154:
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.斜杠;
  r15 = 检查(r12, r14);
  goto logic_merge_1155;

  logic_merge_1155:
  if (r15) goto logic_merge_1153; else goto logic_rhs_1152;

  if_then_1156:
  r26 = cn_var_二元运算符;
  r27 = r26.除法;
  cn_var_运算符_1 = r27;
  goto if_merge_1158;

  if_else_1157:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.百分号;
  r31 = 检查(r28, r30);
  if (r31) goto if_then_1159; else goto if_merge_1160;

  if_merge_1158:
  r34 = cn_var_实例;
  前进词元(r34);
  struct 表达式节点* cn_var_右值_2;
  r35 = cn_var_实例;
  r36 = 解析一元表达式(r35);
  cn_var_右值_2 = r36;
  struct 表达式节点* cn_var_二元节点_3;
  创建二元表达式();
  cn_var_二元节点_3 = /* NONE */;
  r37 = cn_var_二元节点_3;
  r38 = r37 == 0;
  if (r38) goto if_then_1161; else goto if_merge_1162;

  if_then_1159:
  r32 = cn_var_二元运算符;
  r33 = r32.取模;
  cn_var_运算符_1 = r33;
  goto if_merge_1160;

  if_merge_1160:
  goto if_merge_1158;

  if_then_1161:
  r39 = cn_var_左值_0;
  return r39;
  goto if_merge_1162;

  if_merge_1162:
  r40 = cn_var_左值_0;
  r41 = cn_var_右值_2;
  r42 = cn_var_运算符_1;
  r43 = cn_var_二元节点_3;
  cn_var_左值_0 = r43;
  goto while_cond_1149;
  return NULL;
}

struct 表达式节点* 解析一元表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r5, r6, r7, r10, r14, r18, r22, r26, r30, r34, r37, r40, r43, r46, r49, r52, r55, r58, r61, r64, r67, r70, r73, r78;
  struct 解析器* r0;
  struct 解析器* r8;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 解析器* r20;
  struct 解析器* r24;
  struct 解析器* r28;
  struct 解析器* r32;
  struct 解析器* r38;
  struct 解析器* r44;
  struct 解析器* r50;
  struct 解析器* r56;
  struct 解析器* r62;
  struct 解析器* r68;
  struct 解析器* r74;
  struct 解析器* r75;
  struct 表达式节点* r76;
  struct 表达式节点* r77;
  struct 表达式节点* r79;
  struct 表达式节点* r80;
  struct 表达式节点* r82;
  struct 解析器* r83;
  struct 表达式节点* r84;
  _Bool r11;
  _Bool r15;
  _Bool r19;
  _Bool r23;
  _Bool r27;
  _Bool r31;
  _Bool r35;
  _Bool r41;
  _Bool r47;
  _Bool r53;
  _Bool r59;
  _Bool r65;
  _Bool r71;
  enum 一元运算符 r81;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r33;
  enum 一元运算符 r36;
  enum 词元类型枚举 r39;
  enum 一元运算符 r42;
  enum 词元类型枚举 r45;
  enum 一元运算符 r48;
  enum 词元类型枚举 r51;
  enum 一元运算符 r54;
  enum 词元类型枚举 r57;
  enum 一元运算符 r60;
  enum 词元类型枚举 r63;
  enum 一元运算符 r66;
  enum 词元类型枚举 r69;
  enum 一元运算符 r72;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1163; else goto if_merge_1164;

  if_then_1163:
  return 0;
  goto if_merge_1164;

  if_merge_1164:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.逻辑非;
  r11 = 检查(r8, r10);
  if (r11) goto logic_merge_1178; else goto logic_rhs_1177;

  if_then_1165:
  enum 一元运算符 cn_var_运算符_0;
  r36 = cn_var_一元运算符;
  r37 = r36.逻辑非;
  cn_var_运算符_0 = r37;
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.减号;
  r41 = 检查(r38, r40);
  if (r41) goto if_then_1179; else goto if_else_1180;

  if_merge_1166:
  r83 = cn_var_实例;
  r84 = 解析后缀表达式(r83);
  return r84;

  logic_rhs_1167:
  r32 = cn_var_实例;
  r33 = cn_var_词元类型枚举;
  r34 = r33.自减;
  r35 = 检查(r32, r34);
  goto logic_merge_1168;

  logic_merge_1168:
  if (r35) goto if_then_1165; else goto if_merge_1166;

  logic_rhs_1169:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.自增;
  r31 = 检查(r28, r30);
  goto logic_merge_1170;

  logic_merge_1170:
  if (r31) goto logic_merge_1168; else goto logic_rhs_1167;

  logic_rhs_1171:
  r24 = cn_var_实例;
  r25 = cn_var_词元类型枚举;
  r26 = r25.按位与;
  r27 = 检查(r24, r26);
  goto logic_merge_1172;

  logic_merge_1172:
  if (r27) goto logic_merge_1170; else goto logic_rhs_1169;

  logic_rhs_1173:
  r20 = cn_var_实例;
  r21 = cn_var_词元类型枚举;
  r22 = r21.星号;
  r23 = 检查(r20, r22);
  goto logic_merge_1174;

  logic_merge_1174:
  if (r23) goto logic_merge_1172; else goto logic_rhs_1171;

  logic_rhs_1175:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.按位取反;
  r19 = 检查(r16, r18);
  goto logic_merge_1176;

  logic_merge_1176:
  if (r19) goto logic_merge_1174; else goto logic_rhs_1173;

  logic_rhs_1177:
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.减号;
  r15 = 检查(r12, r14);
  goto logic_merge_1178;

  logic_merge_1178:
  if (r15) goto logic_merge_1176; else goto logic_rhs_1175;

  if_then_1179:
  r42 = cn_var_一元运算符;
  r43 = r42.负号;
  cn_var_运算符_0 = r43;
  goto if_merge_1181;

  if_else_1180:
  r44 = cn_var_实例;
  r45 = cn_var_词元类型枚举;
  r46 = r45.按位取反;
  r47 = 检查(r44, r46);
  if (r47) goto if_then_1182; else goto if_else_1183;

  if_merge_1181:
  r74 = cn_var_实例;
  前进词元(r74);
  struct 表达式节点* cn_var_操作数_1;
  r75 = cn_var_实例;
  r76 = 解析一元表达式(r75);
  cn_var_操作数_1 = r76;
  struct 表达式节点* cn_var_一元节点_2;
  创建一元表达式();
  cn_var_一元节点_2 = /* NONE */;
  r77 = cn_var_一元节点_2;
  r78 = r77 == 0;
  if (r78) goto if_then_1196; else goto if_merge_1197;

  if_then_1182:
  r48 = cn_var_一元运算符;
  r49 = r48.按位取反;
  cn_var_运算符_0 = r49;
  goto if_merge_1184;

  if_else_1183:
  r50 = cn_var_实例;
  r51 = cn_var_词元类型枚举;
  r52 = r51.星号;
  r53 = 检查(r50, r52);
  if (r53) goto if_then_1185; else goto if_else_1186;

  if_merge_1184:
  goto if_merge_1181;

  if_then_1185:
  r54 = cn_var_一元运算符;
  r55 = r54.解引用;
  cn_var_运算符_0 = r55;
  goto if_merge_1187;

  if_else_1186:
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.按位与;
  r59 = 检查(r56, r58);
  if (r59) goto if_then_1188; else goto if_else_1189;

  if_merge_1187:
  goto if_merge_1184;

  if_then_1188:
  r60 = cn_var_一元运算符;
  r61 = r60.取地址;
  cn_var_运算符_0 = r61;
  goto if_merge_1190;

  if_else_1189:
  r62 = cn_var_实例;
  r63 = cn_var_词元类型枚举;
  r64 = r63.自增;
  r65 = 检查(r62, r64);
  if (r65) goto if_then_1191; else goto if_else_1192;

  if_merge_1190:
  goto if_merge_1187;

  if_then_1191:
  r66 = cn_var_一元运算符;
  r67 = r66.前缀自增;
  cn_var_运算符_0 = r67;
  goto if_merge_1193;

  if_else_1192:
  r68 = cn_var_实例;
  r69 = cn_var_词元类型枚举;
  r70 = r69.自减;
  r71 = 检查(r68, r70);
  if (r71) goto if_then_1194; else goto if_merge_1195;

  if_merge_1193:
  goto if_merge_1190;

  if_then_1194:
  r72 = cn_var_一元运算符;
  r73 = r72.前缀自减;
  cn_var_运算符_0 = r73;
  goto if_merge_1195;

  if_merge_1195:
  goto if_merge_1193;

  if_then_1196:
  r79 = cn_var_操作数_1;
  return r79;
  goto if_merge_1197;

  if_merge_1197:
  r80 = cn_var_操作数_1;
  r81 = cn_var_运算符_0;
  r82 = cn_var_一元节点_2;
  return r82;
  goto if_merge_1166;
  return NULL;
}

struct 表达式节点* 解析后缀表达式(struct 解析器* cn_var_实例) {
  long long r1, r5, r8, r15, r18, r25, r33, r36, r43, r51, r54, r61, r68, r71, r78, r82, r86, r90, r94, r98;
  char* r30;
  char* r39;
  char* r48;
  char* r57;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 表达式节点* r3;
  struct 表达式节点* r4;
  struct 解析器* r6;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 表达式列表* r12;
  struct 解析器* r13;
  struct 表达式节点* r17;
  struct 表达式节点* r19;
  struct 表达式节点* r20;
  struct 表达式节点* r22;
  struct 解析器* r23;
  struct 解析器* r27;
  struct 解析器* r28;
  struct 解析器* r31;
  struct 表达式节点* r35;
  struct 表达式节点* r37;
  struct 表达式节点* r38;
  struct 表达式节点* r40;
  struct 解析器* r41;
  struct 解析器* r45;
  struct 解析器* r46;
  struct 解析器* r49;
  struct 表达式节点* r53;
  struct 表达式节点* r55;
  struct 表达式节点* r56;
  struct 表达式节点* r58;
  struct 解析器* r59;
  struct 解析器* r63;
  struct 解析器* r64;
  struct 表达式节点* r65;
  struct 解析器* r66;
  struct 表达式节点* r70;
  struct 表达式节点* r72;
  struct 表达式节点* r73;
  struct 表达式节点* r74;
  struct 表达式节点* r75;
  struct 解析器* r76;
  struct 解析器* r80;
  struct 表达式节点* r81;
  struct 表达式节点* r83;
  struct 表达式节点* r84;
  struct 表达式节点* r87;
  struct 解析器* r88;
  struct 解析器* r92;
  struct 表达式节点* r93;
  struct 表达式节点* r95;
  struct 表达式节点* r96;
  struct 表达式节点* r99;
  struct 表达式节点* r100;
  _Bool r9;
  _Bool r16;
  _Bool r26;
  _Bool r34;
  _Bool r44;
  _Bool r52;
  _Bool r62;
  _Bool r69;
  _Bool r79;
  _Bool r91;
  struct 参数 r21;
  struct 词元 r29;
  struct 词元 r47;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r42;
  enum 词元类型枚举 r50;
  enum 词元类型枚举 r60;
  enum 词元类型枚举 r67;
  enum 词元类型枚举 r77;
  enum 一元运算符 r85;
  enum 词元类型枚举 r89;
  enum 一元运算符 r97;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.左括号;
  r9 = 检查(r6, r8);
  if (r9) goto if_then_1205; else goto if_else_1206;

  while_exit_1204:
  r100 = cn_var_表达式_0;
  return r100;

  if_then_1205:
  r10 = cn_var_实例;
  前进词元(r10);
  struct 表达式列表* cn_var_参数_1;
  r11 = cn_var_实例;
  r12 = 解析函数调用参数(r11);
  cn_var_参数_1 = r12;
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.右括号;
  r16 = 期望(r13, r15);
  struct 表达式节点* cn_var_调用节点_2;
  创建函数调用表达式();
  cn_var_调用节点_2 = /* NONE */;
  r17 = cn_var_调用节点_2;
  r18 = r17 == 0;
  if (r18) goto if_then_1208; else goto if_merge_1209;

  if_else_1206:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.点;
  r26 = 检查(r23, r25);
  if (r26) goto if_then_1210; else goto if_else_1211;

  if_merge_1207:
  goto while_cond_1202;

  if_then_1208:
  r19 = cn_var_表达式_0;
  return r19;
  goto if_merge_1209;

  if_merge_1209:
  r20 = cn_var_表达式_0;
  r21 = cn_var_参数_1;
  r22 = cn_var_调用节点_2;
  cn_var_表达式_0 = r22;
  goto if_merge_1207;

  if_then_1210:
  r27 = cn_var_实例;
  前进词元(r27);
  char* cn_var_成员名_3;
  r28 = cn_var_实例;
  r29 = r28->当前词元;
  r30 = r29.值;
  cn_var_成员名_3 = r30;
  r31 = cn_var_实例;
  r32 = cn_var_词元类型枚举;
  r33 = r32.标识符;
  r34 = 期望(r31, r33);
  struct 表达式节点* cn_var_成员节点_4;
  创建成员访问表达式();
  cn_var_成员节点_4 = /* NONE */;
  r35 = cn_var_成员节点_4;
  r36 = r35 == 0;
  if (r36) goto if_then_1213; else goto if_merge_1214;

  if_else_1211:
  r41 = cn_var_实例;
  r42 = cn_var_词元类型枚举;
  r43 = r42.箭头;
  r44 = 检查(r41, r43);
  if (r44) goto if_then_1215; else goto if_else_1216;

  if_merge_1212:
  goto if_merge_1207;

  if_then_1213:
  r37 = cn_var_表达式_0;
  return r37;
  goto if_merge_1214;

  if_merge_1214:
  r38 = cn_var_表达式_0;
  r39 = cn_var_成员名_3;
  r40 = cn_var_成员节点_4;
  cn_var_表达式_0 = r40;
  goto if_merge_1212;

  if_then_1215:
  r45 = cn_var_实例;
  前进词元(r45);
  char* cn_var_成员名_5;
  r46 = cn_var_实例;
  r47 = r46->当前词元;
  r48 = r47.值;
  cn_var_成员名_5 = r48;
  r49 = cn_var_实例;
  r50 = cn_var_词元类型枚举;
  r51 = r50.标识符;
  r52 = 期望(r49, r51);
  struct 表达式节点* cn_var_成员节点_6;
  创建成员访问表达式();
  cn_var_成员节点_6 = /* NONE */;
  r53 = cn_var_成员节点_6;
  r54 = r53 == 0;
  if (r54) goto if_then_1218; else goto if_merge_1219;

  if_else_1216:
  r59 = cn_var_实例;
  r60 = cn_var_词元类型枚举;
  r61 = r60.左方括号;
  r62 = 检查(r59, r61);
  if (r62) goto if_then_1220; else goto if_else_1221;

  if_merge_1217:
  goto if_merge_1212;

  if_then_1218:
  r55 = cn_var_表达式_0;
  return r55;
  goto if_merge_1219;

  if_merge_1219:
  r56 = cn_var_表达式_0;
  r57 = cn_var_成员名_5;
  r58 = cn_var_成员节点_6;
  cn_var_表达式_0 = r58;
  goto if_merge_1217;

  if_then_1220:
  r63 = cn_var_实例;
  前进词元(r63);
  struct 表达式节点* cn_var_索引_7;
  r64 = cn_var_实例;
  r65 = 解析表达式(r64);
  cn_var_索引_7 = r65;
  r66 = cn_var_实例;
  r67 = cn_var_词元类型枚举;
  r68 = r67.右方括号;
  r69 = 期望(r66, r68);
  struct 表达式节点* cn_var_数组节点_8;
  创建数组访问表达式();
  cn_var_数组节点_8 = /* NONE */;
  r70 = cn_var_数组节点_8;
  r71 = r70 == 0;
  if (r71) goto if_then_1223; else goto if_merge_1224;

  if_else_1221:
  r76 = cn_var_实例;
  r77 = cn_var_词元类型枚举;
  r78 = r77.自增;
  r79 = 检查(r76, r78);
  if (r79) goto if_then_1225; else goto if_else_1226;

  if_merge_1222:
  goto if_merge_1217;

  if_then_1223:
  r72 = cn_var_表达式_0;
  return r72;
  goto if_merge_1224;

  if_merge_1224:
  r73 = cn_var_表达式_0;
  r74 = cn_var_索引_7;
  r75 = cn_var_数组节点_8;
  cn_var_表达式_0 = r75;
  goto if_merge_1222;

  if_then_1225:
  r80 = cn_var_实例;
  前进词元(r80);
  struct 表达式节点* cn_var_一元节点_9;
  创建一元表达式();
  cn_var_一元节点_9 = /* NONE */;
  r81 = cn_var_一元节点_9;
  r82 = r81 == 0;
  if (r82) goto if_then_1228; else goto if_merge_1229;

  if_else_1226:
  r88 = cn_var_实例;
  r89 = cn_var_词元类型枚举;
  r90 = r89.自减;
  r91 = 检查(r88, r90);
  if (r91) goto if_then_1230; else goto if_else_1231;

  if_merge_1227:
  goto if_merge_1222;

  if_then_1228:
  r83 = cn_var_表达式_0;
  return r83;
  goto if_merge_1229;

  if_merge_1229:
  r84 = cn_var_表达式_0;
  r85 = cn_var_一元运算符;
  r86 = r85.后缀自增;
  r87 = cn_var_一元节点_9;
  cn_var_表达式_0 = r87;
  goto if_merge_1227;

  if_then_1230:
  r92 = cn_var_实例;
  前进词元(r92);
  struct 表达式节点* cn_var_一元节点_10;
  创建一元表达式();
  cn_var_一元节点_10 = /* NONE */;
  r93 = cn_var_一元节点_10;
  r94 = r93 == 0;
  if (r94) goto if_then_1233; else goto if_merge_1234;

  if_else_1231:
  goto while_exit_1204;
  goto if_merge_1232;

  if_merge_1232:
  goto if_merge_1227;

  if_then_1233:
  r95 = cn_var_表达式_0;
  return r95;
  goto if_merge_1234;

  if_merge_1234:
  r96 = cn_var_表达式_0;
  r97 = cn_var_一元运算符;
  r98 = r97.后缀自减;
  r99 = cn_var_一元节点_10;
  cn_var_表达式_0 = r99;
  goto if_merge_1232;
  return NULL;
}

struct 表达式节点* 解析基础表达式(struct 解析器* cn_var_实例) {
  long long r1, r2, r3, r4, r7, r11, r15, r19, r23, r26, r30, r36, r41, r47, r53, r58, r63, r70, r75;
  char* r67;
  char* r72;
  struct 解析器* r0;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 解析器* r13;
  struct 解析器* r17;
  struct 解析器* r21;
  struct 表达式节点* r22;
  struct 解析器* r24;
  struct 解析器* r28;
  struct 解析器* r32;
  struct 表达式节点* r33;
  struct 解析器* r34;
  struct 解析器* r38;
  struct 解析器* r39;
  struct 解析器* r43;
  struct 表达式节点* r44;
  struct 解析器* r45;
  struct 解析器* r49;
  struct 表达式节点* r50;
  struct 解析器* r51;
  struct 解析器* r55;
  struct 解析器* r56;
  struct 解析器* r60;
  struct 解析器* r61;
  struct 解析器* r65;
  struct 解析器* r68;
  struct 解析器* r73;
  _Bool r8;
  _Bool r12;
  _Bool r16;
  _Bool r20;
  _Bool r27;
  _Bool r31;
  _Bool r37;
  _Bool r42;
  _Bool r48;
  _Bool r54;
  _Bool r59;
  _Bool r64;
  _Bool r71;
  struct 词元 r66;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r40;
  enum 词元类型枚举 r46;
  enum 词元类型枚举 r52;
  enum 词元类型枚举 r57;
  enum 词元类型枚举 r62;
  enum 词元类型枚举 r69;
  enum 诊断错误码 r74;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1235; else goto if_merge_1236;

  if_then_1235:
  return 0;
  goto if_merge_1236;

  if_merge_1236:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.整数字面量;
  r8 = 检查(r5, r7);
  if (r8) goto logic_merge_1244; else goto logic_rhs_1243;

  if_then_1237:
  r21 = cn_var_实例;
  r22 = 解析字面量(r21);
  return r22;
  goto if_merge_1238;

  if_merge_1238:
  r24 = cn_var_实例;
  r25 = cn_var_词元类型枚举;
  r26 = r25.关键字_真;
  r27 = 检查(r24, r26);
  if (r27) goto logic_merge_1248; else goto logic_rhs_1247;

  logic_rhs_1239:
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.字符字面量;
  r20 = 检查(r17, r19);
  goto logic_merge_1240;

  logic_merge_1240:
  if (r20) goto if_then_1237; else goto if_merge_1238;

  logic_rhs_1241:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.字符串字面量;
  r16 = 检查(r13, r15);
  goto logic_merge_1242;

  logic_merge_1242:
  if (r16) goto logic_merge_1240; else goto logic_rhs_1239;

  logic_rhs_1243:
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.浮点字面量;
  r12 = 检查(r9, r11);
  goto logic_merge_1244;

  logic_merge_1244:
  if (r12) goto logic_merge_1242; else goto logic_rhs_1241;

  if_then_1245:
  r32 = cn_var_实例;
  r33 = 解析字面量(r32);
  return r33;
  goto if_merge_1246;

  if_merge_1246:
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.关键字_无;
  r37 = 检查(r34, r36);
  if (r37) goto if_then_1249; else goto if_merge_1250;

  logic_rhs_1247:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.关键字_假;
  r31 = 检查(r28, r30);
  goto logic_merge_1248;

  logic_merge_1248:
  if (r31) goto if_then_1245; else goto if_merge_1246;

  if_then_1249:
  r38 = cn_var_实例;
  前进词元(r38);
  创建空值表达式();
  return;
  goto if_merge_1250;

  if_merge_1250:
  r39 = cn_var_实例;
  r40 = cn_var_词元类型枚举;
  r41 = r40.标识符;
  r42 = 检查(r39, r41);
  if (r42) goto if_then_1251; else goto if_merge_1252;

  if_then_1251:
  r43 = cn_var_实例;
  r44 = 解析标识符表达式(r43);
  return r44;
  goto if_merge_1252;

  if_merge_1252:
  r45 = cn_var_实例;
  r46 = cn_var_词元类型枚举;
  r47 = r46.左括号;
  r48 = 检查(r45, r47);
  if (r48) goto if_then_1253; else goto if_merge_1254;

  if_then_1253:
  r49 = cn_var_实例;
  r50 = 解析括号表达式(r49);
  return r50;
  goto if_merge_1254;

  if_merge_1254:
  r51 = cn_var_实例;
  r52 = cn_var_词元类型枚举;
  r53 = r52.关键字_自身;
  r54 = 检查(r51, r53);
  if (r54) goto if_then_1255; else goto if_merge_1256;

  if_then_1255:
  r55 = cn_var_实例;
  前进词元(r55);
  创建自身表达式();
  return;
  goto if_merge_1256;

  if_merge_1256:
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.关键字_基类;
  r59 = 检查(r56, r58);
  if (r59) goto if_then_1257; else goto if_merge_1258;

  if_then_1257:
  r60 = cn_var_实例;
  前进词元(r60);
  r61 = cn_var_实例;
  r62 = cn_var_词元类型枚举;
  r63 = r62.点;
  r64 = 期望(r61, r63);
  char* cn_var_成员名_0;
  r65 = cn_var_实例;
  r66 = r65->当前词元;
  r67 = r66.值;
  cn_var_成员名_0 = r67;
  r68 = cn_var_实例;
  r69 = cn_var_词元类型枚举;
  r70 = r69.标识符;
  r71 = 期望(r68, r70);
  r72 = cn_var_成员名_0;
  创建基类访问表达式(r72);
  return;
  goto if_merge_1258;

  if_merge_1258:
  r73 = cn_var_实例;
  r74 = cn_var_诊断错误码;
  r75 = r74.语法_无效表达式;
  报告错误(r73, r75, "期望表达式");
  return 0;
}

struct 表达式节点* 解析字面量(struct 解析器* cn_var_实例) {
  long long r1, r3, r8, r11, r14, r17, r20, r23, r25, r28;
  char* r34;
  struct 解析器* r0;
  struct 表达式节点* r2;
  struct 解析器* r4;
  struct 解析器* r26;
  struct 表达式节点* r27;
  struct 解析器* r29;
  struct 表达式节点* r30;
  struct 表达式节点* r31;
  struct 解析器* r32;
  struct 解析器* r35;
  struct 表达式节点* r36;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  struct 词元 r5;
  struct 词元 r33;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r22;

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
  r7 = cn_var_词元类型枚举;
  r8 = r7.整数字面量;
  r9 = r6 == r8;
  if (r9) goto case_body_1264; else goto switch_check_1271;

  switch_check_1271:
  r10 = cn_var_词元类型枚举;
  r11 = r10.浮点字面量;
  r12 = r6 == r11;
  if (r12) goto case_body_1265; else goto switch_check_1272;

  switch_check_1272:
  r13 = cn_var_词元类型枚举;
  r14 = r13.字符串字面量;
  r15 = r6 == r14;
  if (r15) goto case_body_1266; else goto switch_check_1273;

  switch_check_1273:
  r16 = cn_var_词元类型枚举;
  r17 = r16.字符字面量;
  r18 = r6 == r17;
  if (r18) goto case_body_1267; else goto switch_check_1274;

  switch_check_1274:
  r19 = cn_var_词元类型枚举;
  r20 = r19.关键字_真;
  r21 = r6 == r20;
  if (r21) goto case_body_1268; else goto switch_check_1275;

  switch_check_1275:
  r22 = cn_var_词元类型枚举;
  r23 = r22.关键字_假;
  r24 = r6 == r23;
  if (r24) goto case_body_1269; else goto case_default_1270;

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
  r25 = cn_var_布尔字面量;
  r26 = cn_var_实例;
  前进词元(r26);
  r27 = cn_var_节点_0;
  return r27;
  goto switch_merge_1263;

  case_body_1269:
  r28 = cn_var_布尔字面量;
  r29 = cn_var_实例;
  前进词元(r29);
  r30 = cn_var_节点_0;
  return r30;
  goto switch_merge_1263;

  case_default_1270:
  r31 = cn_var_节点_0;
  释放表达式节点(r31);
  return 0;
  goto switch_merge_1263;

  switch_merge_1263:
  r32 = cn_var_实例;
  r33 = r32->当前词元;
  r34 = r33.值;
  r35 = cn_var_实例;
  前进词元(r35);
  r36 = cn_var_节点_0;
  return r36;
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
  long long r1, r4, r10;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r6;
  struct 表达式节点* r7;
  struct 解析器* r8;
  struct 表达式节点* r12;
  _Bool r5;
  _Bool r11;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r9;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1280; else goto if_merge_1281;

  if_then_1280:
  return 0;
  goto if_merge_1281;

  if_merge_1281:
  r2 = cn_var_实例;
  r3 = cn_var_词元类型枚举;
  r4 = r3.左括号;
  r5 = 期望(r2, r4);
  struct 表达式节点* cn_var_表达式_0;
  r6 = cn_var_实例;
  r7 = 解析表达式(r6);
  cn_var_表达式_0 = r7;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.右括号;
  r11 = 期望(r8, r10);
  r12 = cn_var_表达式_0;
  return r12;
}

struct 表达式列表* 解析函数调用参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r8, r12, r17, r19;
  struct 解析器* r0;
  struct 表达式列表* r2;
  struct 解析器* r4;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 表达式列表* r13;
  struct 解析器* r15;
  struct 表达式列表* r20;
  _Bool r7;
  _Bool r18;
  struct 参数 r11;
  struct 参数 r14;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r16;

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
  r5 = cn_var_词元类型枚举;
  r6 = r5.右括号;
  r7 = 检查(r4, r6);
  r8 = !r7;
  if (r8) goto while_body_1287; else goto while_exit_1288;

  while_body_1287:
  struct 表达式节点* cn_var_参数_1;
  r9 = cn_var_实例;
  r10 = 解析表达式(r9);
  cn_var_参数_1 = r10;
  r11 = cn_var_参数_1;
  r12 = r11 != 0;
  if (r12) goto if_then_1289; else goto if_merge_1290;

  while_exit_1288:
  r20 = cn_var_列表_0;
  return r20;

  if_then_1289:
  r13 = cn_var_列表_0;
  r14 = cn_var_参数_1;
  表达式列表添加(r13, r14);
  goto if_merge_1290;

  if_merge_1290:
  r15 = cn_var_实例;
  r16 = cn_var_词元类型枚举;
  r17 = r16.逗号;
  r18 = 匹配(r15, r17);
  r19 = !r18;
  if (r19) goto if_then_1291; else goto if_merge_1292;

  if_then_1291:
  goto while_exit_1288;
  goto if_merge_1292;

  if_merge_1292:
  goto while_cond_1286;
  return NULL;
}

