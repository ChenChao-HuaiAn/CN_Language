#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

// Struct Definitions - 从导入模块
struct 函数声明;
struct 变量声明;
struct 结构体声明;
struct 枚举声明;
struct 类声明;
struct 接口声明;
struct 导入声明;
struct 模板函数声明;
struct 模板结构体声明;
struct 声明节点;
struct 节点类型 {
};
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

// Forward Declarations - 从导入模块

// CN Language Global Struct Forward Declarations
struct 程序节点;

// CN Language Global Struct Definitions
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

// Global Variables

// Forward Declarations
struct 程序节点* 创建程序节点();
void 程序添加声明(struct 程序节点*, struct 声明节点*);

struct 程序节点* 创建程序节点() {
  long long r2, r3, r4;
  void* r0;
  struct 程序节点* r1;
  struct 程序节点* r5;

  entry:
  struct 程序节点* cn_var_节点_0;
  程序节点大小();
  r0 = 分配内存(/* NONE */);
  cn_var_节点_0 = r0;
  r1 = cn_var_节点_0;
  r2 = r1 == 0;
  if (r2) goto if_then_758; else goto if_merge_759;

  if_then_758:
  return 0;
  goto if_merge_759;

  if_merge_759:
  r3 = cn_var_节点类型;
  r4 = r3.程序节点;
  r5 = cn_var_节点_0;
  return r5;
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

