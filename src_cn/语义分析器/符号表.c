#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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

// Forward Declarations - 从导入模块
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

enum 可见性 {
    可见性_默认可见,
    可见性_公开可见,
    可见性_私有可见,
    可见性_保护可见
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

// Forward Declarations
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
struct 符号表管理器* 创建符号表管理器();
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

struct 符号* 创建变量符号(char* cn_var_名称, struct 类型节点* cn_var_类型, struct 源位置 cn_var_位置, struct 符号标志 cn_var_标志) {
  long long r0, r2;
  char* r3;
  struct 类型节点* r4;
  struct 符号* r7;
  struct 源位置 r5;
  struct 符号标志 r6;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.变量符号;
  r3 = cn_var_名称;
  r4 = cn_var_类型;
  r5 = cn_var_位置;
  r6 = cn_var_标志;
  r7 = cn_var_符号指针_0;
  return r7;
}

struct 符号* 创建函数符号(char* cn_var_名称, struct 参数** cn_var_参数列表, long long cn_var_参数个数, struct 类型节点* cn_var_返回类型, struct 源位置 cn_var_位置, struct 符号标志 cn_var_标志) {
  long long r0, r2, r5;
  char* r3;
  struct 参数** r4;
  struct 类型节点* r6;
  struct 符号* r9;
  struct 源位置 r7;
  struct 符号标志 r8;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.函数符号;
  r3 = cn_var_名称;
  r4 = cn_var_参数列表;
  r5 = cn_var_参数个数;
  r6 = cn_var_返回类型;
  r7 = cn_var_位置;
  r8 = cn_var_标志;
  r9 = cn_var_符号指针_0;
  return r9;
}

struct 符号* 创建参数符号(char* cn_var_名称, struct 类型节点* cn_var_类型, struct 源位置 cn_var_位置) {
  long long r0, r2;
  char* r3;
  struct 类型节点* r4;
  struct 符号* r6;
  struct 源位置 r5;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.参数符号;
  r3 = cn_var_名称;
  r4 = cn_var_类型;
  r5 = cn_var_位置;
  r6 = cn_var_符号指针_0;
  return r6;
}

struct 符号* 创建结构体符号(char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0, r2;
  char* r3;
  struct 符号* r5;
  struct 源位置 r4;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.结构体符号;
  r3 = cn_var_名称;
  r4 = cn_var_位置;
  r5 = cn_var_符号指针_0;
  return r5;
}

struct 符号* 创建枚举符号(char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0, r2;
  char* r3;
  struct 符号* r5;
  struct 源位置 r4;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.枚举符号;
  r3 = cn_var_名称;
  r4 = cn_var_位置;
  r5 = cn_var_符号指针_0;
  return r5;
}

struct 符号* 创建枚举成员符号(char* cn_var_名称, long long cn_var_值, _Bool cn_var_有显式值, struct 源位置 cn_var_位置) {
  long long r0, r2, r4;
  char* r3;
  struct 符号* r7;
  _Bool r5;
  struct 源位置 r6;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.枚举成员符号;
  r3 = cn_var_名称;
  r4 = cn_var_值;
  r5 = cn_var_有显式值;
  r6 = cn_var_位置;
  r7 = cn_var_符号指针_0;
  return r7;
}

struct 符号* 创建类符号(char* cn_var_名称, struct 源位置 cn_var_位置, _Bool cn_var_是抽象) {
  long long r0, r2;
  char* r3;
  struct 符号* r6;
  _Bool r5;
  struct 源位置 r4;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.类符号;
  r3 = cn_var_名称;
  r4 = cn_var_位置;
  r5 = cn_var_是抽象;
  r6 = cn_var_符号指针_0;
  return r6;
}

struct 符号* 创建接口符号(char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r0, r2;
  char* r3;
  struct 符号* r5;
  struct 源位置 r4;
  enum 符号类型 r1;

  entry:
  struct 符号* cn_var_符号指针_0;
  r0 = cn_var_符号大小;
  分配内存(r0);
  cn_var_符号指针_0 = /* NONE */;
  r1 = cn_var_符号类型;
  r2 = r1.接口符号;
  r3 = cn_var_名称;
  r4 = cn_var_位置;
  r5 = cn_var_符号指针_0;
  return r5;
}

struct 作用域* 创建作用域(enum 作用域类型 cn_var_类型, char* cn_var_名称, struct 作用域* cn_var_父作用域) {
  long long r0, r4, r5, r6, r7, r8, r9;
  char* r2;
  struct 作用域* r3;
  struct 作用域* r10;
  enum 作用域类型 r1;

  entry:
  struct 作用域* cn_var_作用域指针_0;
  r0 = cn_var_作用域大小;
  分配清零内存(1, r0);
  cn_var_作用域指针_0 = /* NONE */;
  r1 = cn_var_类型;
  r2 = cn_var_名称;
  r3 = cn_var_父作用域;
  r4 = cn_var_初始符号容量;
  r5 = cn_var_初始符号容量;
  r6 = cn_var_符号指针大小;
  分配清零内存(r5, r6);
  r7 = cn_var_初始子作用域容量;
  r8 = cn_var_初始子作用域容量;
  r9 = cn_var_作用域指针大小;
  分配清零内存(r8, r9);
  r10 = cn_var_作用域指针_0;
  return r10;
}

void 销毁作用域(struct 作用域* cn_var_作用域指针) {
  long long r1, r2, r4, r5, r8, r9, r10, r11, r12, r14, r15, r18, r19, r20, r21;
  void* r7;
  void* r17;
  void* r23;
  void* r25;
  struct 作用域* r0;
  struct 作用域* r3;
  struct 作用域* r6;
  struct 作用域* r13;
  struct 作用域* r16;
  struct 作用域* r22;
  struct 作用域* r24;
  struct 作用域* r26;

  entry:
  r0 = cn_var_作用域指针;
  r1 = r0 == 0;
  if (r1) goto if_then_1705; else goto if_merge_1706;

  if_then_1705:
  return;
  goto if_merge_1706;

  if_merge_1706:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1707;

  for_cond_1707:
  r2 = cn_var_i_0;
  r3 = cn_var_作用域指针;
  r4 = r3->子作用域数量;
  r5 = r2 < r4;
  if (r5) goto for_body_1708; else goto for_exit_1710;

  for_body_1708:
  r6 = cn_var_作用域指针;
  r7 = r6->子作用域列表;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  销毁作用域(r9);
  goto for_update_1709;

  for_update_1709:
  r10 = cn_var_i_0;
  r11 = r10 + 1;
  cn_var_i_0 = r11;
  goto for_cond_1707;

  for_exit_1710:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_1711;

  for_cond_1711:
  r12 = cn_var_i_1;
  r13 = cn_var_作用域指针;
  r14 = r13->符号数量;
  r15 = r12 < r14;
  if (r15) goto for_body_1712; else goto for_exit_1714;

  for_body_1712:
  r16 = cn_var_作用域指针;
  r17 = r16->符号表;
  r18 = cn_var_i_1;
  r19 = *(void*)cn_rt_array_get_element(r17, r18, 8);
  释放内存(r19);
  goto for_update_1713;

  for_update_1713:
  r20 = cn_var_i_1;
  r21 = r20 + 1;
  cn_var_i_1 = r21;
  goto for_cond_1711;

  for_exit_1714:
  r22 = cn_var_作用域指针;
  r23 = r22->符号表;
  释放内存(r23);
  r24 = cn_var_作用域指针;
  r25 = r24->子作用域列表;
  释放内存(r25);
  r26 = cn_var_作用域指针;
  释放内存(r26);
  return;
}

struct 符号表管理器* 创建符号表管理器() {
  long long r0, r2;
  struct 符号表管理器* r3;
  struct 作用域* r4;
  struct 符号表管理器* r5;
  enum 作用域类型 r1;

  entry:
  struct 符号表管理器* cn_var_管理器_0;
  r0 = cn_var_符号表管理器大小;
  分配清零内存(1, r0);
  cn_var_管理器_0 = /* NONE */;
  r1 = cn_var_作用域类型;
  r2 = r1.全局作用域;
  创建作用域(r2, "全局", 0);
  r3 = cn_var_管理器_0;
  r4 = r3->全局作用域;
  r5 = cn_var_管理器_0;
  return r5;
}

void 销毁符号表管理器(struct 符号表管理器* cn_var_管理器) {
  long long r1;
  struct 符号表管理器* r0;
  struct 符号表管理器* r2;
  struct 作用域* r3;
  struct 符号表管理器* r4;

  entry:
  r0 = cn_var_管理器;
  r1 = r0 == 0;
  if (r1) goto if_then_1715; else goto if_merge_1716;

  if_then_1715:
  return;
  goto if_merge_1716;

  if_merge_1716:
  r2 = cn_var_管理器;
  r3 = r2->全局作用域;
  销毁作用域(r3);
  r4 = cn_var_管理器;
  释放内存(r4);
  return;
}

void 进入作用域(struct 符号表管理器* cn_var_管理器, enum 作用域类型 cn_var_类型, char* cn_var_名称, struct 符号* cn_var_关联符号) {
  long long r7, r10, r11, r14, r15, r21, r22, r23, r24, r30, r33, r34, r35, r37, r38;
  char* r1;
  void* r18;
  void* r27;
  struct 符号表管理器* r2;
  struct 作用域* r3;
  struct 符号* r4;
  struct 符号表管理器* r5;
  struct 作用域* r6;
  struct 符号表管理器* r8;
  struct 作用域* r9;
  struct 符号表管理器* r12;
  struct 作用域* r13;
  struct 符号表管理器* r16;
  struct 作用域* r17;
  struct 符号表管理器* r19;
  struct 作用域* r20;
  struct 符号表管理器* r25;
  struct 作用域* r26;
  struct 符号表管理器* r28;
  struct 作用域* r29;
  struct 符号表管理器* r31;
  struct 作用域* r32;
  struct 符号表管理器* r36;
  enum 作用域类型 r0;

  entry:
  void cn_var_新作用域_0;
  r0 = cn_var_类型;
  r1 = cn_var_名称;
  r2 = cn_var_管理器;
  r3 = r2->当前作用域;
  创建作用域(r0, r1, r3);
  cn_var_新作用域_0 = /* NONE */;
  r4 = cn_var_关联符号;
  r5 = cn_var_管理器;
  r6 = r5->当前作用域;
  r7 = r6->子作用域数量;
  r8 = cn_var_管理器;
  r9 = r8->当前作用域;
  r10 = r9->子作用域容量;
  r11 = r7 >= r10;
  if (r11) goto if_then_1717; else goto if_merge_1718;

  if_then_1717:
  r12 = cn_var_管理器;
  r13 = r12->当前作用域;
  r14 = r13->子作用域容量;
  r15 = r14 << 1;
  r16 = cn_var_管理器;
  r17 = r16->当前作用域;
  r18 = r17->子作用域列表;
  r19 = cn_var_管理器;
  r20 = r19->当前作用域;
  r21 = r20->子作用域容量;
  r22 = cn_var_作用域指针大小;
  r23 = r21 * r22;
  重新分配内存(r18, r23);
  goto if_merge_1718;

  if_merge_1718:
  r24 = cn_var_新作用域_0;
  r25 = cn_var_管理器;
  r26 = r25->当前作用域;
  r27 = r26->子作用域列表;
  r28 = cn_var_管理器;
  r29 = r28->当前作用域;
  r30 = r29->子作用域数量;
    { long long _tmp_r2 = r24; cn_rt_array_set_element(r27, r30, &_tmp_r2, 8); }
  r31 = cn_var_管理器;
  r32 = r31->当前作用域;
  r33 = r32->子作用域数量;
  r34 = r33 + 1;
  r35 = cn_var_新作用域_0;
  r36 = cn_var_管理器;
  r37 = r36->作用域深度;
  r38 = r37 + 1;
  return;
}

void 离开作用域(struct 符号表管理器* cn_var_管理器) {
  long long r3, r8, r9;
  struct 符号表管理器* r0;
  struct 作用域* r1;
  struct 作用域* r2;
  struct 符号表管理器* r4;
  struct 作用域* r5;
  struct 作用域* r6;
  struct 符号表管理器* r7;

  entry:
  r0 = cn_var_管理器;
  r1 = r0->当前作用域;
  r2 = r1->父作用域;
  r3 = r2 != 0;
  if (r3) goto if_then_1719; else goto if_merge_1720;

  if_then_1719:
  r4 = cn_var_管理器;
  r5 = r4->当前作用域;
  r6 = r5->父作用域;
  r7 = cn_var_管理器;
  r8 = r7->作用域深度;
  r9 = r8 - 1;
  goto if_merge_1720;

  if_merge_1720:
  return;
}

void 设置循环作用域(struct 符号表管理器* cn_var_管理器) {

  entry:
  return;
}

_Bool 在循环体内(struct 符号表管理器* cn_var_管理器) {
  long long r2, r3, r4, r5, r6, r7;
  struct 符号表管理器* r0;
  struct 作用域* r1;

  entry:
  void cn_var_作用域指针_0;
  r0 = cn_var_管理器;
  r1 = r0->当前作用域;
  cn_var_作用域指针_0 = r1;
  goto while_cond_1721;

  while_cond_1721:
  r2 = cn_var_作用域指针_0;
  r3 = r2 != 0;
  if (r3) goto while_body_1722; else goto while_exit_1723;

  while_body_1722:
  r4 = cn_var_作用域指针_0;
  r5 = r4.是循环体;
  if (r5) goto if_then_1724; else goto if_merge_1725;

  while_exit_1723:
  return 0;

  if_then_1724:
  return 1;
  goto if_merge_1725;

  if_merge_1725:
  r6 = cn_var_作用域指针_0;
  r7 = r6.父作用域;
  cn_var_作用域指针_0 = r7;
  goto while_cond_1721;
  return 0;
}

_Bool 插入符号(struct 符号表管理器* cn_var_管理器, struct 符号* cn_var_符号指针) {
  long long r3, r4, r6, r7, r10, r13, r14, r17, r18, r24, r25, r26, r33, r36, r37;
  char* r2;
  void* r21;
  void* r30;
  struct 符号表管理器* r0;
  struct 符号* r1;
  struct 符号表管理器* r5;
  struct 符号表管理器* r8;
  struct 作用域* r9;
  struct 符号表管理器* r11;
  struct 作用域* r12;
  struct 符号表管理器* r15;
  struct 作用域* r16;
  struct 符号表管理器* r19;
  struct 作用域* r20;
  struct 符号表管理器* r22;
  struct 作用域* r23;
  struct 符号* r27;
  struct 符号表管理器* r28;
  struct 作用域* r29;
  struct 符号表管理器* r31;
  struct 作用域* r32;
  struct 符号表管理器* r34;
  struct 作用域* r35;
  struct 符号表管理器* r38;
  struct 作用域* r39;

  entry:
  void cn_var_已存在_0;
  r0 = cn_var_管理器;
  r1 = cn_var_符号指针;
  r2 = r1->名称;
  查找当前作用域符号(r0, r2);
  cn_var_已存在_0 = /* NONE */;
  r3 = cn_var_已存在_0;
  r4 = r3 != 0;
  if (r4) goto if_then_1726; else goto if_merge_1727;

  if_then_1726:
  r5 = cn_var_管理器;
  r6 = r5->错误计数;
  r7 = r6 + 1;
  return 0;
  goto if_merge_1727;

  if_merge_1727:
  r8 = cn_var_管理器;
  r9 = r8->当前作用域;
  r10 = r9->符号数量;
  r11 = cn_var_管理器;
  r12 = r11->当前作用域;
  r13 = r12->符号容量;
  r14 = r10 >= r13;
  if (r14) goto if_then_1728; else goto if_merge_1729;

  if_then_1728:
  r15 = cn_var_管理器;
  r16 = r15->当前作用域;
  r17 = r16->符号容量;
  r18 = r17 << 1;
  r19 = cn_var_管理器;
  r20 = r19->当前作用域;
  r21 = r20->符号表;
  r22 = cn_var_管理器;
  r23 = r22->当前作用域;
  r24 = r23->符号容量;
  r25 = cn_var_符号指针大小;
  r26 = r24 * r25;
  重新分配内存(r21, r26);
  goto if_merge_1729;

  if_merge_1729:
  r27 = cn_var_符号指针;
  r28 = cn_var_管理器;
  r29 = r28->当前作用域;
  r30 = r29->符号表;
  r31 = cn_var_管理器;
  r32 = r31->当前作用域;
  r33 = r32->符号数量;
    { long long _tmp_r3 = r27; cn_rt_array_set_element(r30, r33, &_tmp_r3, 8); }
  r34 = cn_var_管理器;
  r35 = r34->当前作用域;
  r36 = r35->符号数量;
  r37 = r36 + 1;
  r38 = cn_var_管理器;
  r39 = r38->当前作用域;
  return 1;
}

_Bool 在作用域插入符号(struct 作用域* cn_var_目标作用域, struct 符号* cn_var_符号指针) {
  long long r0, r2, r3, r6, r7, r8, r11, r12, r13, r15, r17, r18, r20, r21, r25, r26, r27, r32, r34, r35;
  char* r10;
  void* r5;
  void* r23;
  void* r30;
  struct 作用域* r1;
  struct 作用域* r4;
  struct 符号* r9;
  struct 作用域* r14;
  struct 作用域* r16;
  struct 作用域* r19;
  struct 作用域* r22;
  struct 作用域* r24;
  struct 符号* r28;
  struct 作用域* r29;
  struct 作用域* r31;
  struct 作用域* r33;
  struct 作用域* r36;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1730;

  for_cond_1730:
  r0 = cn_var_i_0;
  r1 = cn_var_目标作用域;
  r2 = r1->符号数量;
  r3 = r0 < r2;
  if (r3) goto for_body_1731; else goto for_exit_1733;

  for_body_1731:
  r4 = cn_var_目标作用域;
  r5 = r4->符号表;
  r6 = cn_var_i_0;
  r7 = *(void*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7.名称;
  r9 = cn_var_符号指针;
  r10 = r9->名称;
  比较字符串(r8, r10);
  r11 = /* NONE */ == 0;
  if (r11) goto if_then_1734; else goto if_merge_1735;

  for_update_1732:
  r12 = cn_var_i_0;
  r13 = r12 + 1;
  cn_var_i_0 = r13;
  goto for_cond_1730;

  for_exit_1733:
  r14 = cn_var_目标作用域;
  r15 = r14->符号数量;
  r16 = cn_var_目标作用域;
  r17 = r16->符号容量;
  r18 = r15 >= r17;
  if (r18) goto if_then_1736; else goto if_merge_1737;

  if_then_1734:
  return 0;
  goto if_merge_1735;

  if_merge_1735:
  goto for_update_1732;

  if_then_1736:
  r19 = cn_var_目标作用域;
  r20 = r19->符号容量;
  r21 = r20 << 1;
  r22 = cn_var_目标作用域;
  r23 = r22->符号表;
  r24 = cn_var_目标作用域;
  r25 = r24->符号容量;
  r26 = cn_var_符号指针大小;
  r27 = r25 * r26;
  重新分配内存(r23, r27);
  goto if_merge_1737;

  if_merge_1737:
  r28 = cn_var_符号指针;
  r29 = cn_var_目标作用域;
  r30 = r29->符号表;
  r31 = cn_var_目标作用域;
  r32 = r31->符号数量;
    { long long _tmp_r4 = r28; cn_rt_array_set_element(r30, r32, &_tmp_r4, 8); }
  r33 = cn_var_目标作用域;
  r34 = r33->符号数量;
  r35 = r34 + 1;
  r36 = cn_var_目标作用域;
  return 1;
}

struct 符号* 查找当前作用域符号(struct 符号表管理器* cn_var_管理器, char* cn_var_名称) {
  long long r0, r3, r4, r8, r9, r10, r12, r16, r17, r18, r19;
  char* r11;
  void* r7;
  void* r15;
  struct 符号表管理器* r1;
  struct 作用域* r2;
  struct 符号表管理器* r5;
  struct 作用域* r6;
  struct 符号表管理器* r13;
  struct 作用域* r14;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1738;

  for_cond_1738:
  r0 = cn_var_i_0;
  r1 = cn_var_管理器;
  r2 = r1->当前作用域;
  r3 = r2->符号数量;
  r4 = r0 < r3;
  if (r4) goto for_body_1739; else goto for_exit_1741;

  for_body_1739:
  r5 = cn_var_管理器;
  r6 = r5->当前作用域;
  r7 = r6->符号表;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  r10 = r9.名称;
  r11 = cn_var_名称;
  比较字符串(r10, r11);
  r12 = /* NONE */ == 0;
  if (r12) goto if_then_1742; else goto if_merge_1743;

  for_update_1740:
  r18 = cn_var_i_0;
  r19 = r18 + 1;
  cn_var_i_0 = r19;
  goto for_cond_1738;

  for_exit_1741:
  return 0;

  if_then_1742:
  r13 = cn_var_管理器;
  r14 = r13->当前作用域;
  r15 = r14->符号表;
  r16 = cn_var_i_0;
  r17 = *(void*)cn_rt_array_get_element(r15, r16, 8);
  return r17;
  goto if_merge_1743;

  if_merge_1743:
  goto for_update_1740;
  return NULL;
}

struct 符号* 查找符号(struct 符号表管理器* cn_var_管理器, char* cn_var_名称) {
  long long r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r14, r15, r16, r17, r18, r19, r20, r21, r22;
  char* r13;
  struct 符号表管理器* r0;
  struct 作用域* r1;

  entry:
  void cn_var_作用域指针_0;
  r0 = cn_var_管理器;
  r1 = r0->当前作用域;
  cn_var_作用域指针_0 = r1;
  goto while_cond_1744;

  while_cond_1744:
  r2 = cn_var_作用域指针_0;
  r3 = r2 != 0;
  if (r3) goto while_body_1745; else goto while_exit_1746;

  while_body_1745:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_1747;

  while_exit_1746:
  return 0;

  for_cond_1747:
  r4 = cn_var_i_1;
  r5 = cn_var_作用域指针_0;
  r6 = r5.符号数量;
  r7 = r4 < r6;
  if (r7) goto for_body_1748; else goto for_exit_1750;

  for_body_1748:
  r8 = cn_var_作用域指针_0;
  r9 = r8.符号表;
  r10 = cn_var_i_1;
  r11 = *(void*)cn_rt_array_get_element(r9, r10, 8);
  r12 = r11.名称;
  r13 = cn_var_名称;
  比较字符串(r12, r13);
  r14 = /* NONE */ == 0;
  if (r14) goto if_then_1751; else goto if_merge_1752;

  for_update_1749:
  r19 = cn_var_i_1;
  r20 = r19 + 1;
  cn_var_i_1 = r20;
  goto for_cond_1747;

  for_exit_1750:
  r21 = cn_var_作用域指针_0;
  r22 = r21.父作用域;
  cn_var_作用域指针_0 = r22;
  goto while_cond_1744;

  if_then_1751:
  r15 = cn_var_作用域指针_0;
  r16 = r15.符号表;
  r17 = cn_var_i_1;
  r18 = *(void*)cn_rt_array_get_element(r16, r17, 8);
  return r18;
  goto if_merge_1752;

  if_merge_1752:
  goto for_update_1749;
  return NULL;
}

struct 符号* 查找全局符号(struct 符号表管理器* cn_var_管理器, char* cn_var_名称) {
  long long r0, r3, r4, r8, r9, r10, r12, r16, r17, r18, r19;
  char* r11;
  void* r7;
  void* r15;
  struct 符号表管理器* r1;
  struct 作用域* r2;
  struct 符号表管理器* r5;
  struct 作用域* r6;
  struct 符号表管理器* r13;
  struct 作用域* r14;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1753;

  for_cond_1753:
  r0 = cn_var_i_0;
  r1 = cn_var_管理器;
  r2 = r1->全局作用域;
  r3 = r2->符号数量;
  r4 = r0 < r3;
  if (r4) goto for_body_1754; else goto for_exit_1756;

  for_body_1754:
  r5 = cn_var_管理器;
  r6 = r5->全局作用域;
  r7 = r6->符号表;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  r10 = r9.名称;
  r11 = cn_var_名称;
  比较字符串(r10, r11);
  r12 = /* NONE */ == 0;
  if (r12) goto if_then_1757; else goto if_merge_1758;

  for_update_1755:
  r18 = cn_var_i_0;
  r19 = r18 + 1;
  cn_var_i_0 = r19;
  goto for_cond_1753;

  for_exit_1756:
  return 0;

  if_then_1757:
  r13 = cn_var_管理器;
  r14 = r13->全局作用域;
  r15 = r14->符号表;
  r16 = cn_var_i_0;
  r17 = *(void*)cn_rt_array_get_element(r15, r16, 8);
  return r17;
  goto if_merge_1758;

  if_merge_1758:
  goto for_update_1755;
  return NULL;
}

struct 符号* 在作用域查找符号(struct 作用域* cn_var_目标作用域, char* cn_var_名称) {
  long long r0, r2, r3, r6, r7, r8, r10, r13, r14, r15, r16;
  char* r9;
  void* r5;
  void* r12;
  struct 作用域* r1;
  struct 作用域* r4;
  struct 作用域* r11;

  entry:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1759;

  for_cond_1759:
  r0 = cn_var_i_0;
  r1 = cn_var_目标作用域;
  r2 = r1->符号数量;
  r3 = r0 < r2;
  if (r3) goto for_body_1760; else goto for_exit_1762;

  for_body_1760:
  r4 = cn_var_目标作用域;
  r5 = r4->符号表;
  r6 = cn_var_i_0;
  r7 = *(void*)cn_rt_array_get_element(r5, r6, 8);
  r8 = r7.名称;
  r9 = cn_var_名称;
  比较字符串(r8, r9);
  r10 = /* NONE */ == 0;
  if (r10) goto if_then_1763; else goto if_merge_1764;

  for_update_1761:
  r15 = cn_var_i_0;
  r16 = r15 + 1;
  cn_var_i_0 = r16;
  goto for_cond_1759;

  for_exit_1762:
  return 0;

  if_then_1763:
  r11 = cn_var_目标作用域;
  r12 = r11->符号表;
  r13 = cn_var_i_0;
  r14 = *(void*)cn_rt_array_get_element(r12, r13, 8);
  return r14;
  goto if_merge_1764;

  if_merge_1764:
  goto for_update_1761;
  return NULL;
}

struct 符号* 查找类成员(struct 符号* cn_var_类符号, char* cn_var_成员名) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23;

  entry:
  goto tail_rec_loop;
  return NULL;
}

char* 获取符号类型名称(enum 符号类型 cn_var_类型) {
  long long r2, r5, r8, r11, r14, r17, r20, r23, r26, r29, r32, r35;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r33;
  _Bool r36;
  enum 符号类型 r0;
  enum 符号类型 r1;
  enum 符号类型 r4;
  enum 符号类型 r7;
  enum 符号类型 r10;
  enum 符号类型 r13;
  enum 符号类型 r16;
  enum 符号类型 r19;
  enum 符号类型 r22;
  enum 符号类型 r25;
  enum 符号类型 r28;
  enum 符号类型 r31;
  enum 符号类型 r34;

  entry:
  r0 = cn_var_类型;
  r1 = cn_var_符号类型;
  r2 = r1.变量符号;
  r3 = r0 == r2;
  if (r3) goto case_body_1778; else goto switch_check_1791;

  switch_check_1791:
  r4 = cn_var_符号类型;
  r5 = r4.函数符号;
  r6 = r0 == r5;
  if (r6) goto case_body_1779; else goto switch_check_1792;

  switch_check_1792:
  r7 = cn_var_符号类型;
  r8 = r7.参数符号;
  r9 = r0 == r8;
  if (r9) goto case_body_1780; else goto switch_check_1793;

  switch_check_1793:
  r10 = cn_var_符号类型;
  r11 = r10.结构体符号;
  r12 = r0 == r11;
  if (r12) goto case_body_1781; else goto switch_check_1794;

  switch_check_1794:
  r13 = cn_var_符号类型;
  r14 = r13.枚举符号;
  r15 = r0 == r14;
  if (r15) goto case_body_1782; else goto switch_check_1795;

  switch_check_1795:
  r16 = cn_var_符号类型;
  r17 = r16.枚举成员符号;
  r18 = r0 == r17;
  if (r18) goto case_body_1783; else goto switch_check_1796;

  switch_check_1796:
  r19 = cn_var_符号类型;
  r20 = r19.类符号;
  r21 = r0 == r20;
  if (r21) goto case_body_1784; else goto switch_check_1797;

  switch_check_1797:
  r22 = cn_var_符号类型;
  r23 = r22.接口符号;
  r24 = r0 == r23;
  if (r24) goto case_body_1785; else goto switch_check_1798;

  switch_check_1798:
  r25 = cn_var_符号类型;
  r26 = r25.类成员符号;
  r27 = r0 == r26;
  if (r27) goto case_body_1786; else goto switch_check_1799;

  switch_check_1799:
  r28 = cn_var_符号类型;
  r29 = r28.模块符号;
  r30 = r0 == r29;
  if (r30) goto case_body_1787; else goto switch_check_1800;

  switch_check_1800:
  r31 = cn_var_符号类型;
  r32 = r31.导入符号;
  r33 = r0 == r32;
  if (r33) goto case_body_1788; else goto switch_check_1801;

  switch_check_1801:
  r34 = cn_var_符号类型;
  r35 = r34.类型参数符号;
  r36 = r0 == r35;
  if (r36) goto case_body_1789; else goto case_default_1790;

  case_body_1778:
  return "变量";
  goto switch_merge_1777;

  case_body_1779:
  return "函数";
  goto switch_merge_1777;

  case_body_1780:
  return "参数";
  goto switch_merge_1777;

  case_body_1781:
  return "结构体";
  goto switch_merge_1777;

  case_body_1782:
  return "枚举";
  goto switch_merge_1777;

  case_body_1783:
  return "枚举成员";
  goto switch_merge_1777;

  case_body_1784:
  return "类";
  goto switch_merge_1777;

  case_body_1785:
  return "接口";
  goto switch_merge_1777;

  case_body_1786:
  return "类成员";
  goto switch_merge_1777;

  case_body_1787:
  return "模块";
  goto switch_merge_1777;

  case_body_1788:
  return "导入";
  goto switch_merge_1777;

  case_body_1789:
  return "类型参数";
  goto switch_merge_1777;

  case_default_1790:
  return "未知";
  goto switch_merge_1777;

  switch_merge_1777:
  return NULL;
}

char* 获取作用域类型名称(enum 作用域类型 cn_var_类型) {
  long long r2, r5, r8, r11, r14, r17, r20, r23;
  _Bool r3;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  enum 作用域类型 r0;
  enum 作用域类型 r1;
  enum 作用域类型 r4;
  enum 作用域类型 r7;
  enum 作用域类型 r10;
  enum 作用域类型 r13;
  enum 作用域类型 r16;
  enum 作用域类型 r19;
  enum 作用域类型 r22;

  entry:
  r0 = cn_var_类型;
  r1 = cn_var_作用域类型;
  r2 = r1.全局作用域;
  r3 = r0 == r2;
  if (r3) goto case_body_1803; else goto switch_check_1812;

  switch_check_1812:
  r4 = cn_var_作用域类型;
  r5 = r4.函数作用域;
  r6 = r0 == r5;
  if (r6) goto case_body_1804; else goto switch_check_1813;

  switch_check_1813:
  r7 = cn_var_作用域类型;
  r8 = r7.块作用域;
  r9 = r0 == r8;
  if (r9) goto case_body_1805; else goto switch_check_1814;

  switch_check_1814:
  r10 = cn_var_作用域类型;
  r11 = r10.类作用域;
  r12 = r0 == r11;
  if (r12) goto case_body_1806; else goto switch_check_1815;

  switch_check_1815:
  r13 = cn_var_作用域类型;
  r14 = r13.结构体作用域;
  r15 = r0 == r14;
  if (r15) goto case_body_1807; else goto switch_check_1816;

  switch_check_1816:
  r16 = cn_var_作用域类型;
  r17 = r16.枚举作用域;
  r18 = r0 == r17;
  if (r18) goto case_body_1808; else goto switch_check_1817;

  switch_check_1817:
  r19 = cn_var_作用域类型;
  r20 = r19.模块作用域;
  r21 = r0 == r20;
  if (r21) goto case_body_1809; else goto switch_check_1818;

  switch_check_1818:
  r22 = cn_var_作用域类型;
  r23 = r22.循环作用域;
  r24 = r0 == r23;
  if (r24) goto case_body_1810; else goto case_default_1811;

  case_body_1803:
  return "全局";
  goto switch_merge_1802;

  case_body_1804:
  return "函数";
  goto switch_merge_1802;

  case_body_1805:
  return "块";
  goto switch_merge_1802;

  case_body_1806:
  return "类";
  goto switch_merge_1802;

  case_body_1807:
  return "结构体";
  goto switch_merge_1802;

  case_body_1808:
  return "枚举";
  goto switch_merge_1802;

  case_body_1809:
  return "模块";
  goto switch_merge_1802;

  case_body_1810:
  return "循环";
  goto switch_merge_1802;

  case_default_1811:
  return "未知";
  goto switch_merge_1802;

  switch_merge_1802:
  return NULL;
}

_Bool 检查符号可访问性(struct 符号* cn_var_符号指针, struct 作用域* cn_var_访问者作用域) {
  long long r9, r14, r15, r16, r19, r20, r21;
  struct 符号* r0;
  struct 符号* r3;
  struct 符号* r6;
  struct 作用域* r7;
  struct 作用域* r8;
  struct 符号* r10;
  struct 作用域* r13;
  struct 符号* r17;
  struct 作用域* r18;
  _Bool r2;
  _Bool r5;
  _Bool r12;
  struct 符号标志 r1;
  struct 符号标志 r4;
  struct 符号标志 r11;

  entry:
  r0 = cn_var_符号指针;
  r1 = r0->标志;
  r2 = r1.是公开;
  if (r2) goto if_then_1819; else goto if_merge_1820;

  if_then_1819:
  return 1;
  goto if_merge_1820;

  if_merge_1820:
  r3 = cn_var_符号指针;
  r4 = r3->标志;
  r5 = r4.是私有;
  if (r5) goto if_then_1821; else goto if_merge_1822;

  if_then_1821:
  r6 = cn_var_符号指针;
  r7 = r6->所属作用域;
  r8 = cn_var_访问者作用域;
  r9 = r7 == r8;
  return r9;
  goto if_merge_1822;

  if_merge_1822:
  r10 = cn_var_符号指针;
  r11 = r10->标志;
  r12 = r11.是保护;
  if (r12) goto if_then_1823; else goto if_merge_1824;

  if_then_1823:
  void cn_var_当前_0;
  r13 = cn_var_访问者作用域;
  cn_var_当前_0 = r13;
  goto while_cond_1825;

  if_merge_1824:
  return 1;

  while_cond_1825:
  r14 = cn_var_当前_0;
  r15 = r14 != 0;
  if (r15) goto while_body_1826; else goto while_exit_1827;

  while_body_1826:
  r16 = cn_var_当前_0;
  r17 = cn_var_符号指针;
  r18 = r17->所属作用域;
  r19 = r16 == r18;
  if (r19) goto if_then_1828; else goto if_merge_1829;

  while_exit_1827:
  return 0;
  goto if_merge_1824;

  if_then_1828:
  return 1;
  goto if_merge_1829;

  if_merge_1829:
  r20 = cn_var_当前_0;
  r21 = r20.父作用域;
  cn_var_当前_0 = r21;
  goto while_cond_1825;
  return 0;
}

