#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块
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
struct 声明节点;
struct 声明节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};

// Forward Declarations - 从导入模块

// CN Language Global Struct Forward Declarations
struct 程序节点;

// CN Language Global Struct Definitions
struct 程序节点 {
    struct AST节点 节点基类;
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

// Global Variables

// Forward Declarations
struct 程序节点* 创建程序节点();
void 程序添加声明(struct 程序节点*, struct 声明节点*);

struct 程序节点* 创建程序节点() {
  long long r1, r3;
  struct 程序节点* r0;
  struct 程序节点* r4;
  enum 节点类型 r2;

  entry:
  struct 程序节点* cn_var_节点_0;
  程序节点大小();
  分配内存(/* NONE */);
  cn_var_节点_0 = /* NONE */;
  r0 = cn_var_节点_0;
  r1 = r0 == 0;
  if (r1) goto if_then_758; else goto if_merge_759;

  if_then_758:
  return 0;
  goto if_merge_759;

  if_merge_759:
  r2 = cn_var_节点类型;
  r3 = r2.程序节点;
  r4 = cn_var_节点_0;
  return r4;
}

void 程序添加声明(struct 程序节点* cn_var_程序, struct 声明节点* cn_var_声明) {
  long long r0, r2, r4;
  struct 程序节点* r1;
  struct 声明节点* r3;

  entry:
  r1 = cn_var_程序;
  r2 = r1 == 0;
  if (r2) goto logic_merge_763; else goto logic_rhs_762;

  if_then_760:
  return;
  goto if_merge_761;

  if_merge_761:

  logic_rhs_762:
  r3 = cn_var_声明;
  r4 = r3 == 0;
  goto logic_merge_763;

  logic_merge_763:
  if (r4) goto if_then_760; else goto if_merge_761;
  return;
}

