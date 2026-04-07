#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 节点类型 {
    节点类型_程序节点,
    节点类型_函数声明,
    节点类型_变量声明,
    节点类型_结构体声明,
    节点类型_枚举声明,
    节点类型_模块声明,
    节点类型_导入声明,
    节点类型_类声明,
    节点类型_接口声明,
    节点类型_模板函数声明,
    节点类型_模板结构体声明,
    节点类型_表达式语句,
    节点类型_块语句,
    节点类型_如果语句,
    节点类型_当语句,
    节点类型_循环语句,
    节点类型_返回语句,
    节点类型_中断语句,
    节点类型_继续语句,
    节点类型_选择语句,
    节点类型_尝试语句,
    节点类型_抛出语句,
    节点类型_最终语句,
    节点类型_二元表达式,
    节点类型_一元表达式,
    节点类型_字面量表达式,
    节点类型_标识符表达式,
    节点类型_函数调用表达式,
    节点类型_成员访问表达式,
    节点类型_数组访问表达式,
    节点类型_赋值表达式,
    节点类型_三元表达式,
    节点类型_数组字面量表达式,
    节点类型_结构体字面量表达式,
    节点类型_逻辑表达式,
    节点类型_模板实例化表达式,
    节点类型_基础类型,
    节点类型_指针类型,
    节点类型_数组类型,
    节点类型_函数类型,
    节点类型_结构体类型,
    节点类型_枚举类型,
    节点类型_类类型,
    节点类型_接口类型
};

// CN Language Global Struct Forward Declarations
struct 源位置;
struct AST节点;
struct 类型节点;

// CN Language Global Struct Definitions
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};

struct AST节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};

struct 类型节点 {
    enum 节点类型 类型;
    char* 名称;
    struct 类型节点* 元素类型;
    long long 指针层级;
    long long 数组维度;
    long long 数组大小;
    _Bool 是常量;
};

// Global Variables

// Forward Declarations

