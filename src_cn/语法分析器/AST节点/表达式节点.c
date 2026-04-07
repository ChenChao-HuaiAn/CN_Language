#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块

// CN Language Global Struct Forward Declarations
struct 二元表达式;
struct 一元表达式;
struct 逻辑表达式;
struct 字面量表达式;
struct 标识符表达式;
struct 函数调用表达式;
struct 成员访问表达式;
struct 数组访问表达式;
struct 赋值表达式;
struct 三元表达式;
struct 数组字面量表达式;
struct 结构体字段初始化;
struct 结构体字面量表达式;
struct 模板实例化表达式;
struct 表达式节点;

// CN Language Global Struct Definitions
struct 二元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    struct 二元运算符 运算符;
};

struct 一元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 操作数;
    struct 一元运算符 运算符;
    _Bool 是前缀;
};

struct 逻辑表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 左操作数;
    struct 表达式节点* 右操作数;
    struct 逻辑运算符 运算符;
};

struct 字面量表达式 {
    struct AST节点 节点基类;
    struct 字面量类型 类型;
    char* 值;
    long long 整数值;
    double 浮点值;
    _Bool 布尔值;
};

struct 标识符表达式 {
    struct AST节点 节点基类;
    char* 名称;
};

struct 函数调用表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 被调函数;
    struct 表达式节点** 参数;
    long long 参数个数;
};

struct 成员访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 对象;
    char* 成员名;
    _Bool 是指针访问;
    _Bool 是静态成员;
    char* 类名;
};

struct 数组访问表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 数组;
    struct 表达式节点* 索引;
};

struct 赋值表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 目标;
    struct 表达式节点* 值;
    struct 赋值运算符 运算符;
};

struct 三元表达式 {
    struct AST节点 节点基类;
    struct 表达式节点* 条件;
    struct 表达式节点* 真值;
    struct 表达式节点* 假值;
};

struct 数组字面量表达式 {
    struct AST节点 节点基类;
    struct 表达式节点** 元素;
    long long 元素个数;
};

struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};

struct 结构体字面量表达式 {
    struct AST节点 节点基类;
    char* 结构体名;
    struct 结构体字段初始化** 字段;
    long long 字段个数;
};

struct 模板实例化表达式 {
    struct AST节点 节点基类;
    char* 模板名;
    struct 类型节点** 类型参数;
    long long 参数个数;
};

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

// Global Variables

// Forward Declarations

