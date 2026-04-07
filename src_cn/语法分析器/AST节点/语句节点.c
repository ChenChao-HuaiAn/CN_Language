#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块
struct 类型节点;
struct 表达式节点;
struct 节点类型 {
};
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct 表达式节点;
struct 表达式节点 {
    struct 节点类型 类型;
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

// Forward Declarations - 从导入模块

// CN Language Global Struct Forward Declarations
struct 表达式语句;
struct 块语句;
struct 如果语句;
struct 当语句;
struct 循环语句;
struct 返回语句;
struct 中断语句;
struct 继续语句;
struct 情况分支;
struct 选择语句;
struct 捕获子句;
struct 尝试语句;
struct 抛出语句;
struct 最终语句;
struct 语句节点;

// CN Language Global Struct Definitions
struct 表达式语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 表达式;
};

struct 块语句 {
    struct AST节点 节点基类;
    struct 语句节点** 语句;
    long long 语句个数;
};

struct 如果语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 真分支;
    struct 语句节点* 假分支;
};

struct 当语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 块语句* 循环体;
};

struct 循环语句 {
    struct AST节点 节点基类;
    struct 语句节点* 初始化;
    struct 表达式节点* 条件;
    struct 表达式节点* 更新;
    struct 块语句* 循环体;
};

struct 返回语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 返回值;
};

struct 中断语句 {
    struct AST节点 节点基类;
};

struct 继续语句 {
    struct AST节点 节点基类;
};

struct 情况分支 {
    struct 表达式节点* 匹配值;
    struct 块语句* 语句体;
};

struct 选择语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 选择值;
    struct 情况分支** 情况分支列表;
    long long 情况个数;
    struct 块语句* 默认分支;
};

struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    struct 块语句* 语句体;
};

struct 尝试语句 {
    struct AST节点 节点基类;
    struct 块语句* 尝试块;
    struct 捕获子句** 捕获子句列表;
    long long 捕获个数;
    struct 块语句* 最终块;
};

struct 抛出语句 {
    struct AST节点 节点基类;
    struct 表达式节点* 异常表达式;
    char* 异常类型;
    char* 消息;
};

struct 最终语句 {
    struct AST节点 节点基类;
    struct 块语句* 语句体;
};

struct 语句节点 {
    struct 节点类型 类型;
    struct 源位置 位置;
};

// Global Variables

// Forward Declarations

