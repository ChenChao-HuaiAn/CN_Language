#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
enum 诊断严重级别 {
    诊断严重级别_诊断_警告 = 1,
    诊断严重级别_诊断_错误 = 0
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

// Struct Definitions - 从导入模块
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
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
struct 作用域;
struct 符号表管理器;
struct 符号表管理器 {
    struct 作用域* 全局作用域;
    struct 作用域* 当前作用域;
    long long 作用域深度;
    long long 错误计数;
};
struct 类型节点;
struct 符号;
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
struct 表达式节点;
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
struct 结构体字段初始化;
struct 结构体字段初始化 {
    char* 字段名;
    struct 表达式节点* 值;
};
struct 语句节点;
struct 语句节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 捕获子句;
struct 捕获子句 {
    char* 异常类型;
    char* 变量名;
    long long* 语句体;
};
struct 情况分支;
struct 情况分支 {
    struct 表达式节点* 匹配值;
    long long* 语句体;
};
struct 声明节点;
struct 声明节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
};
struct 导入成员;
struct 导入成员 {
    char* 名称;
    char* 别名;
};
struct 模板参数;
struct 模板参数 {
    char* 名称;
    struct 类型节点* 约束;
    struct 类型节点* 默认类型;
};
struct 接口方法;
struct 接口方法 {
    char* 名称;
    struct 参数** 参数列表;
    long long 参数个数;
    struct 类型节点* 返回类型;
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
struct 枚举成员;
struct 枚举成员 {
    char* 名称;
    long long 值;
    _Bool 有显式值;
};
struct 结构体成员;
struct 结构体成员 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    enum 可见性 可见性;
};
struct 参数;
struct 参数 {
    char* 名称;
    struct 类型节点* 类型;
    _Bool 是常量;
    _Bool 是数组;
    long long 数组维度;
};
struct 类型信息;
struct 类型推断上下文;
struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};

// Forward Declarations - 从导入模块
extern long long cn_var_初始子作用域容量;
extern long long cn_var_初始符号容量;
_Bool 检查符号可访问性(struct 符号*, struct 作用域*);
char* 获取作用域类型名称(enum 作用域类型);
char* 获取符号类型名称(enum 符号类型);
struct 符号* 查找类成员(struct 符号*, char*);
struct 符号* 在作用域查找符号(struct 作用域*, char*);
struct 符号* 查找全局符号(struct 符号表管理器*, char*);
struct 符号* 查找符号(struct 符号表管理器*, char*);
struct 符号* 查找当前作用域符号(struct 符号表管理器*, char*);
_Bool 在作用域插入符号(struct 作用域*, struct 符号*);
_Bool 插入符号(struct 符号表管理器*, struct 符号*);
_Bool 在循环体内(struct 符号表管理器*);
void 设置循环作用域(struct 符号表管理器*);
void 离开作用域(struct 符号表管理器*);
void 进入作用域(struct 符号表管理器*, enum 作用域类型, char*, struct 符号*);
void 销毁符号表管理器(struct 符号表管理器*);
struct 符号表管理器* 创建符号表管理器(void);
void 销毁作用域(struct 作用域*);
struct 作用域* 创建作用域(enum 作用域类型, char*, struct 作用域*);
struct 符号* 创建接口符号(char*, struct 源位置);
struct 符号* 创建类符号(char*, struct 源位置, _Bool);
struct 符号* 创建枚举成员符号(char*, long long, _Bool, struct 源位置);
struct 符号* 创建枚举符号(char*, struct 源位置);
struct 符号* 创建结构体符号(char*, struct 源位置);
struct 符号* 创建参数符号(char*, struct 类型节点*, struct 源位置);
struct 符号* 创建函数符号(char*, struct 参数**, long long, struct 类型节点*, struct 源位置, struct 符号标志);
struct 符号* 创建变量符号(char*, struct 类型节点*, struct 源位置, struct 符号标志);
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 字符串格式(char*);
long long 字符串格式化(char*, long long, char*);
char* 读取行(void);
long long 获取绝对值(long long);
long long 求最大值(long long, long long);
long long 求最小值(long long, long long);
long long 获取参数个数(void);
char* 获取参数(long long);
char* 获取程序名称(void);
char* 查找选项(char*);
long long 选项存在(char*);
char* 获取位置参数(long long);
long long 获取位置参数个数(void);
void* 数组获取(void*, long long);
struct 诊断集合* 创建诊断集合(long long);
void 释放诊断集合(struct 诊断集合*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, char*);
_Bool 有错误(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
_Bool 应该继续(struct 诊断集合*);
char* 获取严重级别字符串(enum 诊断严重级别);
void 打印诊断信息(struct 诊断信息*);
void 打印所有诊断(struct 诊断集合*);
void 清空诊断集合(struct 诊断集合*);
struct 源位置 创建源位置(char*, long long, long long);
struct 源位置 创建未知位置(void);
void 报告重复符号错误(struct 诊断集合*, struct 源位置, char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, char*, char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, char*);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, char*);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);
void 程序添加声明(struct 程序节点*, struct 声明节点*);
struct 程序节点* 创建程序节点(void);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
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
extern long long cn_var_初始符号容量;
extern long long cn_var_初始子作用域容量;

// CN Language Global Struct Forward Declarations
struct 作用域构建器;

// CN Language Global Struct Definitions
struct 作用域构建器 {
    struct 符号表管理器* 符号表;
    struct 类型推断上下文* 类型上下文;
    struct 诊断集合* 诊断器;
    long long 错误计数;
};

// Global Variables

// Forward Declarations
struct 作用域构建器* 创建作用域构建器(struct 诊断集合*);
void 销毁作用域构建器(struct 作用域构建器*);
void 构建程序作用域(struct 作用域构建器*, struct 程序节点*);
void 构建声明作用域(struct 作用域构建器*, struct 声明节点*);
void 构建函数声明作用域(struct 作用域构建器*, struct 函数声明*);
struct 符号标志 创建符号标志(struct 函数声明*);
void 构建参数作用域(struct 作用域构建器*, struct 参数*);
void 构建变量声明作用域(struct 作用域构建器*, struct 变量声明*);
void 构建结构体声明作用域(struct 作用域构建器*, struct 结构体声明*);
struct 符号标志 创建成员标志(struct 结构体成员*);
void 构建枚举声明作用域(struct 作用域构建器*, struct 枚举声明*);
void 构建类声明作用域(struct 作用域构建器*, struct 类声明*);
void 构建类成员作用域(struct 作用域构建器*, struct 类成员*, struct 符号*, long long);
void 构建接口声明作用域(struct 作用域构建器*, struct 接口声明*);
void 构建导入声明作用域(struct 作用域构建器*, struct 导入声明*);
void 构建语句作用域(struct 作用域构建器*, struct 语句节点*);
void 构建块语句作用域(struct 作用域构建器*, struct 块语句*);
void 构建如果语句作用域(struct 作用域构建器*, struct 如果语句*);
void 构建当语句作用域(struct 作用域构建器*, struct 当语句*);
void 构建循环语句作用域(struct 作用域构建器*, struct 循环语句*);
void 构建选择语句作用域(struct 作用域构建器*, struct 选择语句*);
void 构建情况语句作用域(struct 作用域构建器*, struct 情况语句*);
void 构建返回语句作用域(struct 作用域构建器*, struct 返回语句*);
void 构建尝试语句作用域(struct 作用域构建器*, struct 尝试语句*);
void 构建捕获语句作用域(struct 作用域构建器*, struct 捕获语句*);
void 构建表达式作用域(struct 作用域构建器*, struct 表达式节点*);
void 构建二元表达式作用域(struct 作用域构建器*, struct 二元表达式*);
void 构建一元表达式作用域(struct 作用域构建器*, struct 一元表达式*);
void 构建函数调用表达式作用域(struct 作用域构建器*, struct 函数调用表达式*);
void 构建成员访问表达式作用域(struct 作用域构建器*, struct 成员访问表达式*);
void 构建数组访问表达式作用域(struct 作用域构建器*, struct 数组访问表达式*);
void 构建赋值表达式作用域(struct 作用域构建器*, struct 赋值表达式*);
void 构建三元表达式作用域(struct 作用域构建器*, struct 三元表达式*);
void 构建标识符表达式作用域(struct 作用域构建器*, struct 标识符表达式*);
void 报告重复符号错误(struct 作用域构建器*, char*, struct 源位置);
void 报告未定义标识符错误(struct 作用域构建器*, char*, struct 源位置);

struct 作用域构建器* 创建作用域构建器(struct 诊断集合* cn_var_诊断器) {
  long long r0, r1;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 诊断集合* r4;
  struct 作用域构建器* r5;

  entry:
  struct 作用域构建器* cn_var_构建器_0;
  r0 = cn_var_作用域构建器大小;
  分配清零内存(1, r0);
  cn_var_构建器_0 = /* NONE */;
  创建符号表管理器();
  r1 = cn_var_类型推断上下文大小;
  分配清零内存(1, r1);
  r2 = cn_var_构建器_0;
  r3 = r2->符号表;
  r4 = cn_var_诊断器;
  r5 = cn_var_构建器_0;
  return r5;
}

void 销毁作用域构建器(struct 作用域构建器* cn_var_构建器) {
  long long r1;
  struct 作用域构建器* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 作用域构建器* r4;
  struct 类型推断上下文* r5;
  struct 作用域构建器* r6;

  entry:
  r0 = cn_var_构建器;
  r1 = r0 == 0;
  if (r1) goto if_then_2068; else goto if_merge_2069;

  if_then_2068:
  return;
  goto if_merge_2069;

  if_merge_2069:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  销毁符号表管理器(r3);
  r4 = cn_var_构建器;
  r5 = r4->类型上下文;
  释放内存(r5);
  r6 = cn_var_构建器;
  释放内存(r6);
  return;
}

void 构建程序作用域(struct 作用域构建器* cn_var_构建器, struct 程序节点* cn_var_程序) {
  long long r1, r2, r4, r5, r8, r9, r10, r11, r12;
  struct 程序节点* r0;
  struct 程序节点* r3;
  struct 作用域构建器* r6;
  struct 程序节点* r7;

  entry:
  r0 = cn_var_程序;
  r1 = r0 == 0;
  if (r1) goto if_then_2070; else goto if_merge_2071;

  if_then_2070:
  return;
  goto if_merge_2071;

  if_merge_2071:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2072;

  for_cond_2072:
  r2 = cn_var_i_0;
  r3 = cn_var_程序;
  r4 = r3->声明个数;
  r5 = r2 < r4;
  if (r5) goto for_body_2073; else goto for_exit_2075;

  for_body_2073:
  r6 = cn_var_构建器;
  r7 = cn_var_程序;
  r8 = r7->声明列表;
  r9 = cn_var_i_0;
  r10 = *(void*)cn_rt_array_get_element(r8, r9, 8);
  构建声明作用域(r6, r10);
  goto for_update_2074;

  for_update_2074:
  r11 = cn_var_i_0;
  r12 = r11 + 1;
  cn_var_i_0 = r12;
  goto for_cond_2072;

  for_exit_2075:
  return;
}

void 构建声明作用域(struct 作用域构建器* cn_var_构建器, struct 声明节点* cn_var_声明) {
  long long r1, r3, r4, r6, r9, r12, r15, r18, r21, r24;
  struct 声明节点* r0;
  struct 声明节点* r2;
  struct 作用域构建器* r26;
  struct 声明节点* r27;
  struct 作用域构建器* r28;
  struct 声明节点* r29;
  struct 作用域构建器* r30;
  struct 声明节点* r31;
  struct 作用域构建器* r32;
  struct 声明节点* r33;
  struct 作用域构建器* r34;
  struct 声明节点* r35;
  struct 作用域构建器* r36;
  struct 声明节点* r37;
  struct 作用域构建器* r38;
  struct 声明节点* r39;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  enum 节点类型 r5;
  enum 节点类型 r8;
  enum 节点类型 r11;
  enum 节点类型 r14;
  enum 节点类型 r17;
  enum 节点类型 r20;
  enum 节点类型 r23;

  entry:
  r0 = cn_var_声明;
  r1 = r0 == 0;
  if (r1) goto if_then_2076; else goto if_merge_2077;

  if_then_2076:
  return;
  goto if_merge_2077;

  if_merge_2077:
  r2 = cn_var_声明;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = cn_var_节点类型;
  r6 = r5.函数声明;
  r7 = r4 == r6;
  if (r7) goto case_body_2079; else goto switch_check_2087;

  switch_check_2087:
  r8 = cn_var_节点类型;
  r9 = r8.变量声明;
  r10 = r4 == r9;
  if (r10) goto case_body_2080; else goto switch_check_2088;

  switch_check_2088:
  r11 = cn_var_节点类型;
  r12 = r11.结构体声明;
  r13 = r4 == r12;
  if (r13) goto case_body_2081; else goto switch_check_2089;

  switch_check_2089:
  r14 = cn_var_节点类型;
  r15 = r14.枚举声明;
  r16 = r4 == r15;
  if (r16) goto case_body_2082; else goto switch_check_2090;

  switch_check_2090:
  r17 = cn_var_节点类型;
  r18 = r17.类声明;
  r19 = r4 == r18;
  if (r19) goto case_body_2083; else goto switch_check_2091;

  switch_check_2091:
  r20 = cn_var_节点类型;
  r21 = r20.接口声明;
  r22 = r4 == r21;
  if (r22) goto case_body_2084; else goto switch_check_2092;

  switch_check_2092:
  r23 = cn_var_节点类型;
  r24 = r23.导入声明;
  r25 = r4 == r24;
  if (r25) goto case_body_2085; else goto case_default_2086;

  case_body_2079:
  r26 = cn_var_构建器;
  r27 = cn_var_声明;
  构建函数声明作用域(r26, (struct 函数声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2080:
  r28 = cn_var_构建器;
  r29 = cn_var_声明;
  构建变量声明作用域(r28, (struct 变量声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2081:
  r30 = cn_var_构建器;
  r31 = cn_var_声明;
  构建结构体声明作用域(r30, (struct 结构体声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2082:
  r32 = cn_var_构建器;
  r33 = cn_var_声明;
  构建枚举声明作用域(r32, (struct 枚举声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2083:
  r34 = cn_var_构建器;
  r35 = cn_var_声明;
  构建类声明作用域(r34, (struct 类声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2084:
  r36 = cn_var_构建器;
  r37 = cn_var_声明;
  构建接口声明作用域(r36, (struct 接口声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_body_2085:
  r38 = cn_var_构建器;
  r39 = cn_var_声明;
  构建导入声明作用域(r38, (struct 导入声明*)cn_var_声明);
  goto switch_merge_2078;
  goto switch_merge_2078;

  case_default_2086:
  goto switch_merge_2078;
  goto switch_merge_2078;

  switch_merge_2078:
  return;
}

void 构建函数声明作用域(struct 作用域构建器* cn_var_构建器, struct 函数声明* cn_var_函数节点) {
  long long r1, r4, r6, r8, r10, r12, r13, r14, r17, r20, r22, r23, r27, r29, r33, r35, r36, r39, r40, r41, r42, r43, r45, r46, r49;
  struct 函数声明* r0;
  struct 函数声明* r2;
  struct 函数声明* r3;
  struct 函数声明* r5;
  struct 函数声明* r7;
  struct 函数声明* r9;
  struct 函数声明* r11;
  struct 作用域构建器* r15;
  struct 符号表管理器* r16;
  struct 作用域构建器* r18;
  struct 函数声明* r19;
  struct 函数声明* r21;
  struct 作用域构建器* r24;
  struct 符号表管理器* r25;
  struct 函数声明* r28;
  struct 作用域构建器* r30;
  struct 符号表管理器* r31;
  struct 作用域* r32;
  struct 函数声明* r34;
  struct 作用域构建器* r37;
  struct 函数声明* r38;
  struct 函数声明* r44;
  struct 作用域构建器* r47;
  struct 函数声明* r48;
  struct 作用域构建器* r50;
  struct 符号表管理器* r51;
  enum 作用域类型 r26;

  entry:
  r0 = cn_var_函数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2093; else goto if_merge_2094;

  if_then_2093:
  return;
  goto if_merge_2094;

  if_merge_2094:
  void cn_var_标志_0;
  r2 = cn_var_函数节点;
  创建符号标志(r2);
  cn_var_标志_0 = /* NONE */;
  void cn_var_函数符号_1;
  r3 = cn_var_函数节点;
  r4 = r3->名称;
  r5 = cn_var_函数节点;
  r6 = r5->参数列表;
  r7 = cn_var_函数节点;
  r8 = r7->参数个数;
  r9 = cn_var_函数节点;
  r10 = r9->返回类型;
  r11 = cn_var_函数节点;
  r12 = r11->节点基类;
  r13 = r12.位置;
  r14 = cn_var_标志_0;
  创建函数符号(r4, r6, r8, r10, r13, r14);
  cn_var_函数符号_1 = /* NONE */;
  r15 = cn_var_构建器;
  r16 = r15->符号表;
  插入符号(r16, 1);
  r17 = /* NONE */ == 0;
  if (r17) goto if_then_2095; else goto if_merge_2096;

  if_then_2095:
  r18 = cn_var_构建器;
  r19 = cn_var_函数节点;
  r20 = r19->名称;
  r21 = cn_var_函数节点;
  r22 = r21->节点基类;
  r23 = r22.位置;
  报告重复符号错误(r18, r20, r23);
  return;
  goto if_merge_2096;

  if_merge_2096:
  r24 = cn_var_构建器;
  r25 = r24->符号表;
  r26 = cn_var_作用域类型;
  r27 = r26.函数作用域;
  r28 = cn_var_函数节点;
  r29 = r28->名称;
  进入作用域(r25, r27, r29, 1);
  r30 = cn_var_构建器;
  r31 = r30->符号表;
  r32 = r31->当前作用域;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2097;

  for_cond_2097:
  r33 = cn_var_i_2;
  r34 = cn_var_函数节点;
  r35 = r34->参数个数;
  r36 = r33 < r35;
  if (r36) goto for_body_2098; else goto for_exit_2100;

  for_body_2098:
  r37 = cn_var_构建器;
  r38 = cn_var_函数节点;
  r39 = r38->参数列表;
  r40 = cn_var_i_2;
  r41 = *(void*)cn_rt_array_get_element(r39, r40, 8);
  构建参数作用域(r37, r41);
  goto for_update_2099;

  for_update_2099:
  r42 = cn_var_i_2;
  r43 = r42 + 1;
  cn_var_i_2 = r43;
  goto for_cond_2097;

  for_exit_2100:
  r44 = cn_var_函数节点;
  r45 = r44->函数体;
  r46 = r45 != 0;
  if (r46) goto if_then_2101; else goto if_merge_2102;

  if_then_2101:
  r47 = cn_var_构建器;
  r48 = cn_var_函数节点;
  r49 = r48->函数体;
  构建语句作用域(r47, (struct 语句节点*)cn_var_函数节点.函数体);
  goto if_merge_2102;

  if_merge_2102:
  r50 = cn_var_构建器;
  r51 = r50->符号表;
  离开作用域(r51);
  return;
}

struct 符号标志 创建符号标志(struct 函数声明* cn_var_函数节点) {
  long long r1, r3, r5, r7, r8;
  struct 函数声明* r0;
  struct 函数声明* r2;
  struct 函数声明* r4;
  struct 函数声明* r6;

  entry:
  void cn_var_标志_0;
  r0 = cn_var_函数节点;
  r1 = r0->是静态;
  r2 = cn_var_函数节点;
  r3 = r2->是虚拟;
  r4 = cn_var_函数节点;
  r5 = r4->是重写;
  r6 = cn_var_函数节点;
  r7 = r6->是抽象;
  r8 = cn_var_标志_0;
  return r8;
}

void 构建参数作用域(struct 作用域构建器* cn_var_构建器, struct 参数* cn_var_参数节点) {
  long long r1, r7, r10, r15;
  char* r3;
  char* r13;
  struct 参数* r0;
  struct 参数* r2;
  struct 参数* r4;
  struct 类型节点* r5;
  struct 参数* r6;
  struct 作用域构建器* r8;
  struct 符号表管理器* r9;
  struct 作用域构建器* r11;
  struct 参数* r12;
  struct 参数* r14;

  entry:
  r0 = cn_var_参数节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2103; else goto if_merge_2104;

  if_then_2103:
  return;
  goto if_merge_2104;

  if_merge_2104:
  void cn_var_参数符号_0;
  r2 = cn_var_参数节点;
  r3 = r2->名称;
  r4 = cn_var_参数节点;
  r5 = r4->类型;
  r6 = cn_var_参数节点;
  r7 = r6->定义位置;
  创建参数符号(r3, r5, r7);
  cn_var_参数符号_0 = /* NONE */;
  r8 = cn_var_构建器;
  r9 = r8->符号表;
  插入符号(r9, 2);
  r10 = /* NONE */ == 0;
  if (r10) goto if_then_2105; else goto if_merge_2106;

  if_then_2105:
  r11 = cn_var_构建器;
  r12 = cn_var_参数节点;
  r13 = r12->名称;
  r14 = cn_var_参数节点;
  r15 = r14->定义位置;
  报告重复符号错误(r11, r13, r15);
  goto if_merge_2106;

  if_merge_2106:
  return;
}

void 构建变量声明作用域(struct 作用域构建器* cn_var_构建器, struct 变量声明* cn_var_变量节点) {
  long long r1, r3, r5, r7, r8, r10, r12, r14, r15, r16, r19, r22, r24, r25, r27, r28, r31;
  struct 变量声明* r0;
  struct 变量声明* r2;
  struct 变量声明* r4;
  struct 变量声明* r6;
  struct 变量声明* r9;
  struct 变量声明* r11;
  struct 变量声明* r13;
  struct 作用域构建器* r17;
  struct 符号表管理器* r18;
  struct 作用域构建器* r20;
  struct 变量声明* r21;
  struct 变量声明* r23;
  struct 变量声明* r26;
  struct 作用域构建器* r29;
  struct 变量声明* r30;

  entry:
  r0 = cn_var_变量节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2107; else goto if_merge_2108;

  if_then_2107:
  return;
  goto if_merge_2108;

  if_merge_2108:
  void cn_var_标志_0;
  r2 = cn_var_变量节点;
  r3 = r2->是常量;
  r4 = cn_var_变量节点;
  r5 = r4->是静态;
  r6 = cn_var_变量节点;
  r7 = r6->初始值;
  r8 = r7 != 0;
  void cn_var_变量符号_1;
  r9 = cn_var_变量节点;
  r10 = r9->名称;
  r11 = cn_var_变量节点;
  r12 = r11->类型;
  r13 = cn_var_变量节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  r16 = cn_var_标志_0;
  创建变量符号(r10, r12, r15, r16);
  cn_var_变量符号_1 = /* NONE */;
  r17 = cn_var_构建器;
  r18 = r17->符号表;
  插入符号(r18, 0);
  r19 = /* NONE */ == 0;
  if (r19) goto if_then_2109; else goto if_merge_2110;

  if_then_2109:
  r20 = cn_var_构建器;
  r21 = cn_var_变量节点;
  r22 = r21->名称;
  r23 = cn_var_变量节点;
  r24 = r23->节点基类;
  r25 = r24.位置;
  报告重复符号错误(r20, r22, r25);
  return;
  goto if_merge_2110;

  if_merge_2110:
  r26 = cn_var_变量节点;
  r27 = r26->初始值;
  r28 = r27 != 0;
  if (r28) goto if_then_2111; else goto if_merge_2112;

  if_then_2111:
  r29 = cn_var_构建器;
  r30 = cn_var_变量节点;
  r31 = r30->初始值;
  构建表达式作用域(r29, r31);
  goto if_merge_2112;

  if_merge_2112:
  return;
}

void 构建结构体声明作用域(struct 作用域构建器* cn_var_构建器, struct 结构体声明* cn_var_结构体节点) {
  long long r1, r3, r5, r6, r9, r12, r14, r15, r19, r21, r26, r28, r29, r30, r32, r33, r35, r36, r37, r38, r39, r40, r41, r43, r44, r45, r46, r47, r48, r51, r52, r53;
  struct 结构体声明* r0;
  struct 结构体声明* r2;
  struct 结构体声明* r4;
  struct 作用域构建器* r7;
  struct 符号表管理器* r8;
  struct 作用域构建器* r10;
  struct 结构体声明* r11;
  struct 结构体声明* r13;
  struct 作用域构建器* r16;
  struct 符号表管理器* r17;
  struct 结构体声明* r20;
  struct 作用域构建器* r22;
  struct 符号表管理器* r23;
  struct 作用域* r24;
  struct 结构体声明* r25;
  struct 结构体声明* r27;
  struct 结构体声明* r31;
  struct 结构体声明* r34;
  struct 结构体声明* r42;
  struct 作用域构建器* r49;
  struct 符号表管理器* r50;
  struct 作用域构建器* r54;
  struct 符号表管理器* r55;
  enum 作用域类型 r18;

  entry:
  r0 = cn_var_结构体节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2113; else goto if_merge_2114;

  if_then_2113:
  return;
  goto if_merge_2114;

  if_merge_2114:
  void cn_var_结构体符号_0;
  r2 = cn_var_结构体节点;
  r3 = r2->名称;
  r4 = cn_var_结构体节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  创建结构体符号(r3, r6);
  cn_var_结构体符号_0 = /* NONE */;
  r7 = cn_var_构建器;
  r8 = r7->符号表;
  插入符号(r8, 3);
  r9 = /* NONE */ == 0;
  if (r9) goto if_then_2115; else goto if_merge_2116;

  if_then_2115:
  r10 = cn_var_构建器;
  r11 = cn_var_结构体节点;
  r12 = r11->名称;
  r13 = cn_var_结构体节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  报告重复符号错误(r10, r12, r15);
  return;
  goto if_merge_2116;

  if_merge_2116:
  r16 = cn_var_构建器;
  r17 = r16->符号表;
  r18 = cn_var_作用域类型;
  r19 = r18.结构体作用域;
  r20 = cn_var_结构体节点;
  r21 = r20->名称;
  进入作用域(r17, r19, r21, 3);
  r22 = cn_var_构建器;
  r23 = r22->符号表;
  r24 = r23->当前作用域;
  r25 = cn_var_结构体节点;
  r26 = r25->成员个数;
  r27 = cn_var_结构体节点;
  r28 = r27->成员个数;
  r29 = cn_var_符号指针大小;
  分配清零内存(r28, r29);
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2117;

  for_cond_2117:
  r30 = cn_var_i_1;
  r31 = cn_var_结构体节点;
  r32 = r31->成员个数;
  r33 = r30 < r32;
  if (r33) goto for_body_2118; else goto for_exit_2120;

  for_body_2118:
  void cn_var_成员_2;
  r34 = cn_var_结构体节点;
  r35 = r34->成员;
  r36 = cn_var_i_1;
  r37 = *(void*)cn_rt_array_get_element(r35, r36, 8);
  cn_var_成员_2 = r37;
  void cn_var_成员符号_3;
  r38 = cn_var_成员_2;
  r39 = r38.名称;
  r40 = cn_var_成员_2;
  r41 = r40.类型;
  r42 = cn_var_结构体节点;
  r43 = r42->节点基类;
  r44 = r43.位置;
  r45 = cn_var_成员_2;
  创建成员标志(r45);
  创建变量符号(r39, r41, r44, /* NONE */);
  cn_var_成员符号_3 = /* NONE */;
  r46 = cn_var_成员符号_3;
  r47 = 3.成员列表;
  r48 = cn_var_i_1;
    { long long _tmp_r6 = r46; cn_rt_array_set_element(r47, r48, &_tmp_r6, 8); }
  r49 = cn_var_构建器;
  r50 = r49->符号表;
  r51 = cn_var_成员符号_3;
  插入符号(r50, r51);
  goto for_update_2119;

  for_update_2119:
  r52 = cn_var_i_1;
  r53 = r52 + 1;
  cn_var_i_1 = r53;
  goto for_cond_2117;

  for_exit_2120:
  r54 = cn_var_构建器;
  r55 = r54->符号表;
  离开作用域(r55);
  return;
}

struct 符号标志 创建成员标志(struct 结构体成员* cn_var_成员) {
  long long r5, r6, r10, r11, r15, r16, r17;
  struct 结构体成员* r0;
  struct 结构体成员* r2;
  struct 结构体成员* r7;
  struct 结构体成员* r12;
  _Bool r1;
  enum 可见性 r3;
  enum 可见性 r4;
  enum 可见性 r8;
  enum 可见性 r9;
  enum 可见性 r13;
  enum 可见性 r14;

  entry:
  void cn_var_标志_0;
  r0 = cn_var_成员;
  r1 = r0->是常量;
  r2 = cn_var_成员;
  r3 = r2->可见性;
  r4 = cn_var_可见性;
  r5 = r4.公开可见;
  r6 = r3 == r5;
  r7 = cn_var_成员;
  r8 = r7->可见性;
  r9 = cn_var_可见性;
  r10 = r9.私有可见;
  r11 = r8 == r10;
  r12 = cn_var_成员;
  r13 = r12->可见性;
  r14 = cn_var_可见性;
  r15 = r14.保护可见;
  r16 = r13 == r15;
  r17 = cn_var_标志_0;
  return r17;
}

void 构建枚举声明作用域(struct 作用域构建器* cn_var_构建器, struct 枚举声明* cn_var_枚举节点) {
  long long r1, r3, r5, r6, r9, r12, r14, r15, r19, r21, r26, r28, r29, r30, r32, r33, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r48, r49, r50, r51, r52, r55, r56, r57, r58, r59;
  struct 枚举声明* r0;
  struct 枚举声明* r2;
  struct 枚举声明* r4;
  struct 作用域构建器* r7;
  struct 符号表管理器* r8;
  struct 作用域构建器* r10;
  struct 枚举声明* r11;
  struct 枚举声明* r13;
  struct 作用域构建器* r16;
  struct 符号表管理器* r17;
  struct 枚举声明* r20;
  struct 作用域构建器* r22;
  struct 符号表管理器* r23;
  struct 作用域* r24;
  struct 枚举声明* r25;
  struct 枚举声明* r27;
  struct 枚举声明* r31;
  struct 枚举声明* r34;
  struct 枚举声明* r47;
  struct 作用域构建器* r53;
  struct 符号表管理器* r54;
  struct 作用域构建器* r60;
  struct 符号表管理器* r61;
  enum 作用域类型 r18;

  entry:
  r0 = cn_var_枚举节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2121; else goto if_merge_2122;

  if_then_2121:
  return;
  goto if_merge_2122;

  if_merge_2122:
  void cn_var_枚举符号_0;
  r2 = cn_var_枚举节点;
  r3 = r2->名称;
  r4 = cn_var_枚举节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  创建枚举符号(r3, r6);
  cn_var_枚举符号_0 = /* NONE */;
  r7 = cn_var_构建器;
  r8 = r7->符号表;
  插入符号(r8, 4);
  r9 = /* NONE */ == 0;
  if (r9) goto if_then_2123; else goto if_merge_2124;

  if_then_2123:
  r10 = cn_var_构建器;
  r11 = cn_var_枚举节点;
  r12 = r11->名称;
  r13 = cn_var_枚举节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  报告重复符号错误(r10, r12, r15);
  return;
  goto if_merge_2124;

  if_merge_2124:
  r16 = cn_var_构建器;
  r17 = r16->符号表;
  r18 = cn_var_作用域类型;
  r19 = r18.枚举作用域;
  r20 = cn_var_枚举节点;
  r21 = r20->名称;
  进入作用域(r17, r19, r21, 4);
  r22 = cn_var_构建器;
  r23 = r22->符号表;
  r24 = r23->当前作用域;
  r25 = cn_var_枚举节点;
  r26 = r25->成员个数;
  r27 = cn_var_枚举节点;
  r28 = r27->成员个数;
  r29 = cn_var_符号指针大小;
  分配清零内存(r28, r29);
  long long cn_var_当前值_1;
  cn_var_当前值_1 = 0;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2125;

  for_cond_2125:
  r30 = cn_var_i_2;
  r31 = cn_var_枚举节点;
  r32 = r31->成员个数;
  r33 = r30 < r32;
  if (r33) goto for_body_2126; else goto for_exit_2128;

  for_body_2126:
  void cn_var_成员_3;
  r34 = cn_var_枚举节点;
  r35 = r34->成员;
  r36 = cn_var_i_2;
  r37 = *(void*)cn_rt_array_get_element(r35, r36, 8);
  cn_var_成员_3 = r37;
  r38 = cn_var_成员_3;
  r39 = r38.有显式值;
  if (r39) goto if_then_2129; else goto if_merge_2130;

  for_update_2127:
  r58 = cn_var_i_2;
  r59 = r58 + 1;
  cn_var_i_2 = r59;
  goto for_cond_2125;

  for_exit_2128:
  r60 = cn_var_构建器;
  r61 = r60->符号表;
  离开作用域(r61);

  if_then_2129:
  r40 = cn_var_成员_3;
  r41 = r40.值;
  cn_var_当前值_1 = r41;
  goto if_merge_2130;

  if_merge_2130:
  void cn_var_成员符号_4;
  r42 = cn_var_成员_3;
  r43 = r42.名称;
  r44 = cn_var_当前值_1;
  r45 = cn_var_成员_3;
  r46 = r45.有显式值;
  r47 = cn_var_枚举节点;
  r48 = r47->节点基类;
  r49 = r48.位置;
  创建枚举成员符号(r43, r44, r46, r49);
  cn_var_成员符号_4 = /* NONE */;
  r50 = cn_var_成员符号_4;
  r51 = 4.成员列表;
  r52 = cn_var_i_2;
    { long long _tmp_r7 = r50; cn_rt_array_set_element(r51, r52, &_tmp_r7, 8); }
  r53 = cn_var_构建器;
  r54 = r53->符号表;
  r55 = cn_var_成员符号_4;
  插入符号(r54, r55);
  r56 = cn_var_当前值_1;
  r57 = r56 + 1;
  cn_var_当前值_1 = r57;
  goto for_update_2127;
  return;
}

void 构建类声明作用域(struct 作用域构建器* cn_var_构建器, struct 类声明* cn_var_类节点) {
  long long r1, r3, r5, r6, r8, r11, r14, r16, r17, r18, r20, r21, r23, r24, r28, r29, r30, r31, r32, r33, r35, r36, r37, r39, r40, r42, r44, r45, r46, r48, r49, r53, r54, r55, r56, r57, r58, r60, r61, r62, r63, r64, r65, r69, r71, r76, r78, r79, r80, r82, r83, r86, r87, r88, r89, r90, r91;
  struct 类声明* r0;
  struct 类声明* r2;
  struct 类声明* r4;
  struct 类声明* r7;
  struct 作用域构建器* r9;
  struct 符号表管理器* r10;
  struct 作用域构建器* r12;
  struct 类声明* r13;
  struct 类声明* r15;
  struct 类声明* r19;
  struct 类声明* r22;
  struct 作用域构建器* r25;
  struct 符号表管理器* r26;
  struct 类声明* r27;
  struct 类声明* r38;
  struct 类声明* r41;
  struct 类声明* r43;
  struct 类声明* r47;
  struct 作用域构建器* r50;
  struct 符号表管理器* r51;
  struct 类声明* r52;
  struct 作用域构建器* r66;
  struct 符号表管理器* r67;
  struct 类声明* r70;
  struct 作用域构建器* r72;
  struct 符号表管理器* r73;
  struct 作用域* r74;
  struct 类声明* r75;
  struct 类声明* r77;
  struct 类声明* r81;
  struct 作用域构建器* r84;
  struct 类声明* r85;
  struct 作用域构建器* r92;
  struct 符号表管理器* r93;
  enum 符号类型 r34;
  enum 符号类型 r59;
  enum 作用域类型 r68;

  entry:
  r0 = cn_var_类节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2131; else goto if_merge_2132;

  if_then_2131:
  return;
  goto if_merge_2132;

  if_merge_2132:
  void cn_var_类符号_0;
  r2 = cn_var_类节点;
  r3 = r2->名称;
  r4 = cn_var_类节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  r7 = cn_var_类节点;
  r8 = r7->是抽象;
  创建类符号(r3, r6, r8);
  cn_var_类符号_0 = /* NONE */;
  r9 = cn_var_构建器;
  r10 = r9->符号表;
  插入符号(r10, 6);
  r11 = /* NONE */ == 0;
  if (r11) goto if_then_2133; else goto if_merge_2134;

  if_then_2133:
  r12 = cn_var_构建器;
  r13 = cn_var_类节点;
  r14 = r13->名称;
  r15 = cn_var_类节点;
  r16 = r15->节点基类;
  r17 = r16.位置;
  报告重复符号错误(r12, r14, r17);
  return;
  goto if_merge_2134;

  if_merge_2134:
  r19 = cn_var_类节点;
  r20 = r19->基类名称;
  r21 = r20 != 0;
  if (r21) goto logic_rhs_2137; else goto logic_merge_2138;

  if_then_2135:
  void cn_var_父类符号_1;
  r25 = cn_var_构建器;
  r26 = r25->符号表;
  r27 = cn_var_类节点;
  r28 = r27->基类名称;
  查找符号(r26, r28);
  cn_var_父类符号_1 = /* NONE */;
  r30 = cn_var_父类符号_1;
  r31 = r30 != 0;
  if (r31) goto logic_rhs_2141; else goto logic_merge_2142;

  if_merge_2136:
  r38 = cn_var_类节点;
  r39 = r38->接口个数;
  r40 = r39 > 0;
  if (r40) goto if_then_2143; else goto if_merge_2144;

  logic_rhs_2137:
  r22 = cn_var_类节点;
  r23 = r22->基类名称;
  获取字符串长度(r23);
  r24 = /* NONE */ > 0;
  goto logic_merge_2138;

  logic_merge_2138:
  if (r24) goto if_then_2135; else goto if_merge_2136;

  if_then_2139:
  r37 = cn_var_父类符号_1;
  goto if_merge_2140;

  if_merge_2140:
  goto if_merge_2136;

  logic_rhs_2141:
  r32 = cn_var_父类符号_1;
  r33 = r32.种类;
  r34 = cn_var_符号类型;
  r35 = r34.类符号;
  r36 = r33 == r35;
  goto logic_merge_2142;

  logic_merge_2142:
  if (r36) goto if_then_2139; else goto if_merge_2140;

  if_then_2143:
  r41 = cn_var_类节点;
  r42 = r41->接口个数;
  r43 = cn_var_类节点;
  r44 = r43->接口个数;
  r45 = cn_var_符号指针大小;
  分配清零内存(r44, r45);
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_2145;

  if_merge_2144:
  r66 = cn_var_构建器;
  r67 = r66->符号表;
  r68 = cn_var_作用域类型;
  r69 = r68.类作用域;
  r70 = cn_var_类节点;
  r71 = r70->名称;
  进入作用域(r67, r69, r71, 6);
  r72 = cn_var_构建器;
  r73 = r72->符号表;
  r74 = r73->当前作用域;
  r75 = cn_var_类节点;
  r76 = r75->成员个数;
  r77 = cn_var_类节点;
  r78 = r77->成员个数;
  r79 = cn_var_符号指针大小;
  分配清零内存(r78, r79);
  long long cn_var_i_4;
  cn_var_i_4 = 0;
  goto for_cond_2153;

  for_cond_2145:
  r46 = cn_var_i_2;
  r47 = cn_var_类节点;
  r48 = r47->接口个数;
  r49 = r46 < r48;
  if (r49) goto for_body_2146; else goto for_exit_2148;

  for_body_2146:
  void cn_var_接口符号_3;
  r50 = cn_var_构建器;
  r51 = r50->符号表;
  r52 = cn_var_类节点;
  r53 = r52->实现接口;
  r54 = cn_var_i_2;
  r55 = *(void*)cn_rt_array_get_element(r53, r54, 8);
  查找符号(r51, r55);
  cn_var_接口符号_3 = /* NONE */;
  r57 = 1;
  if (r57) goto logic_rhs_2151; else goto logic_merge_2152;

  for_update_2147:
  r64 = cn_var_i_2;
  r65 = r64 + 1;
  cn_var_i_2 = r65;
  goto for_cond_2145;

  for_exit_2148:
  goto if_merge_2144;

  if_then_2149:
  r62 = 6.实现接口;
  r63 = cn_var_i_2;
    { long long _tmp_i3 = 7; cn_rt_array_set_element(r62, r63, &_tmp_i3, 8); }
  goto if_merge_2150;

  if_merge_2150:
  goto for_update_2147;

  logic_rhs_2151:
  r58 = 7.种类;
  r59 = cn_var_符号类型;
  r60 = r59.接口符号;
  r61 = r58 == r60;
  goto logic_merge_2152;

  logic_merge_2152:
  if (r61) goto if_then_2149; else goto if_merge_2150;

  for_cond_2153:
  r80 = cn_var_i_4;
  r81 = cn_var_类节点;
  r82 = r81->成员个数;
  r83 = r80 < r82;
  if (r83) goto for_body_2154; else goto for_exit_2156;

  for_body_2154:
  r84 = cn_var_构建器;
  r85 = cn_var_类节点;
  r86 = r85->成员;
  r87 = cn_var_i_4;
  r88 = *(void*)cn_rt_array_get_element(r86, r87, 8);
  r89 = cn_var_i_4;
  构建类成员作用域(r84, r88, 6, r89);
  goto for_update_2155;

  for_update_2155:
  r90 = cn_var_i_4;
  r91 = r90 + 1;
  cn_var_i_4 = r91;
  goto for_cond_2153;

  for_exit_2156:
  r92 = cn_var_构建器;
  r93 = r92->符号表;
  离开作用域(r93);
  return;
}

void 构建类成员作用域(struct 作用域构建器* cn_var_构建器, struct 类成员* cn_var_成员节点, struct 符号* cn_var_类符号, long long cn_var_索引) {
  long long r1, r5, r6, r12, r13, r17, r18, r22, r23, r28, r31, r32, r33, r35, r39, r40, r52, r53, r57, r58, r62, r63, r68, r71, r74, r77, r78, r79, r81, r82, r83, r84, r85, r86, r89;
  char* r25;
  char* r65;
  struct 类成员* r0;
  struct 类成员* r2;
  struct 类成员* r7;
  struct 类成员* r9;
  struct 类成员* r14;
  struct 类成员* r19;
  struct 类成员* r24;
  struct 类成员* r26;
  long long* r27;
  struct 类成员* r29;
  long long* r30;
  struct 类成员* r36;
  struct 类成员* r41;
  struct 类成员* r43;
  struct 类成员* r45;
  struct 类成员* r47;
  struct 类成员* r49;
  struct 类成员* r54;
  struct 类成员* r59;
  struct 类成员* r64;
  struct 类成员* r66;
  long long* r67;
  struct 类成员* r69;
  long long* r70;
  struct 类成员* r72;
  long long* r73;
  struct 类成员* r75;
  long long* r76;
  struct 作用域构建器* r87;
  struct 符号表管理器* r88;
  _Bool r8;
  _Bool r42;
  _Bool r44;
  _Bool r46;
  _Bool r48;
  enum 节点类型 r3;
  enum 节点类型 r4;
  enum 可见性 r10;
  enum 可见性 r11;
  enum 可见性 r15;
  enum 可见性 r16;
  enum 可见性 r20;
  enum 可见性 r21;
  enum 符号类型 r34;
  enum 节点类型 r37;
  enum 节点类型 r38;
  enum 可见性 r50;
  enum 可见性 r51;
  enum 可见性 r55;
  enum 可见性 r56;
  enum 可见性 r60;
  enum 可见性 r61;
  enum 符号类型 r80;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2157; else goto if_merge_2158;

  if_then_2157:
  return;
  goto if_merge_2158;

  if_merge_2158:
  void cn_var_成员符号_0;
  r2 = cn_var_成员节点;
  r3 = r2->类型;
  r4 = cn_var_节点类型;
  r5 = r4.变量声明;
  r6 = r3 == r5;
  if (r6) goto if_then_2159; else goto if_else_2160;

  if_then_2159:
  void cn_var_标志_1;
  r7 = cn_var_成员节点;
  r8 = r7->是静态;
  r9 = cn_var_成员节点;
  r10 = r9->可见性;
  r11 = cn_var_可见性;
  r12 = r11.公开可见;
  r13 = r10 == r12;
  r14 = cn_var_成员节点;
  r15 = r14->可见性;
  r16 = cn_var_可见性;
  r17 = r16.私有可见;
  r18 = r15 == r17;
  r19 = cn_var_成员节点;
  r20 = r19->可见性;
  r21 = cn_var_可见性;
  r22 = r21.保护可见;
  r23 = r20 == r22;
  r24 = cn_var_成员节点;
  r25 = r24->名称;
  r26 = cn_var_成员节点;
  r27 = r26->字段;
  r28 = r27->类型;
  r29 = cn_var_成员节点;
  r30 = r29->字段;
  r31 = r30->节点基类;
  r32 = r31.位置;
  r33 = cn_var_标志_1;
  创建变量符号(r25, r28, r32, r33);
  cn_var_成员符号_0 = /* NONE */;
  r34 = cn_var_符号类型;
  r35 = r34.类成员符号;
  goto if_merge_2161;

  if_else_2160:
  r36 = cn_var_成员节点;
  r37 = r36->类型;
  r38 = cn_var_节点类型;
  r39 = r38.函数声明;
  r40 = r37 == r39;
  if (r40) goto if_then_2162; else goto if_merge_2163;

  if_merge_2161:
  r82 = cn_var_成员符号_0;
  r83 = r82 != 0;
  if (r83) goto if_then_2164; else goto if_merge_2165;

  if_then_2162:
  void cn_var_标志_2;
  r41 = cn_var_成员节点;
  r42 = r41->是静态;
  r43 = cn_var_成员节点;
  r44 = r43->是虚拟;
  r45 = cn_var_成员节点;
  r46 = r45->是重写;
  r47 = cn_var_成员节点;
  r48 = r47->是抽象;
  r49 = cn_var_成员节点;
  r50 = r49->可见性;
  r51 = cn_var_可见性;
  r52 = r51.公开可见;
  r53 = r50 == r52;
  r54 = cn_var_成员节点;
  r55 = r54->可见性;
  r56 = cn_var_可见性;
  r57 = r56.私有可见;
  r58 = r55 == r57;
  r59 = cn_var_成员节点;
  r60 = r59->可见性;
  r61 = cn_var_可见性;
  r62 = r61.保护可见;
  r63 = r60 == r62;
  r64 = cn_var_成员节点;
  r65 = r64->名称;
  r66 = cn_var_成员节点;
  r67 = r66->方法;
  r68 = r67->参数列表;
  r69 = cn_var_成员节点;
  r70 = r69->方法;
  r71 = r70->参数个数;
  r72 = cn_var_成员节点;
  r73 = r72->方法;
  r74 = r73->返回类型;
  r75 = cn_var_成员节点;
  r76 = r75->方法;
  r77 = r76->节点基类;
  r78 = r77.位置;
  r79 = cn_var_标志_2;
  创建函数符号(r65, r68, r71, r74, r78, r79);
  cn_var_成员符号_0 = /* NONE */;
  r80 = cn_var_符号类型;
  r81 = r80.类成员符号;
  goto if_merge_2163;

  if_merge_2163:
  goto if_merge_2161;

  if_then_2164:
  r84 = cn_var_成员符号_0;
  r85 = 6.成员列表;
  r86 = cn_var_索引;
    { long long _tmp_r8 = r84; cn_rt_array_set_element(r85, r86, &_tmp_r8, 8); }
  r87 = cn_var_构建器;
  r88 = r87->符号表;
  r89 = cn_var_成员符号_0;
  插入符号(r88, r89);
  goto if_merge_2165;

  if_merge_2165:
  return;
}

void 构建接口声明作用域(struct 作用域构建器* cn_var_构建器, struct 接口声明* cn_var_接口节点) {
  long long r1, r3, r5, r6, r9, r12, r14, r15, r19, r21, r26, r28, r29, r30, r32, r33, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r47, r48, r49, r51, r52, r53, r54, r57, r58, r59;
  struct 接口声明* r0;
  struct 接口声明* r2;
  struct 接口声明* r4;
  struct 作用域构建器* r7;
  struct 符号表管理器* r8;
  struct 作用域构建器* r10;
  struct 接口声明* r11;
  struct 接口声明* r13;
  struct 作用域构建器* r16;
  struct 符号表管理器* r17;
  struct 接口声明* r20;
  struct 作用域构建器* r22;
  struct 符号表管理器* r23;
  struct 作用域* r24;
  struct 接口声明* r25;
  struct 接口声明* r27;
  struct 接口声明* r31;
  struct 接口声明* r34;
  struct 接口声明* r46;
  struct 作用域构建器* r55;
  struct 符号表管理器* r56;
  struct 作用域构建器* r60;
  struct 符号表管理器* r61;
  enum 作用域类型 r18;
  enum 符号类型 r50;

  entry:
  r0 = cn_var_接口节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2166; else goto if_merge_2167;

  if_then_2166:
  return;
  goto if_merge_2167;

  if_merge_2167:
  void cn_var_接口符号_0;
  r2 = cn_var_接口节点;
  r3 = r2->名称;
  r4 = cn_var_接口节点;
  r5 = r4->节点基类;
  r6 = r5.位置;
  创建接口符号(r3, r6);
  cn_var_接口符号_0 = /* NONE */;
  r7 = cn_var_构建器;
  r8 = r7->符号表;
  插入符号(r8, 7);
  r9 = /* NONE */ == 0;
  if (r9) goto if_then_2168; else goto if_merge_2169;

  if_then_2168:
  r10 = cn_var_构建器;
  r11 = cn_var_接口节点;
  r12 = r11->名称;
  r13 = cn_var_接口节点;
  r14 = r13->节点基类;
  r15 = r14.位置;
  报告重复符号错误(r10, r12, r15);
  return;
  goto if_merge_2169;

  if_merge_2169:
  r16 = cn_var_构建器;
  r17 = r16->符号表;
  r18 = cn_var_作用域类型;
  r19 = r18.类作用域;
  r20 = cn_var_接口节点;
  r21 = r20->名称;
  进入作用域(r17, r19, r21, 7);
  r22 = cn_var_构建器;
  r23 = r22->符号表;
  r24 = r23->当前作用域;
  r25 = cn_var_接口节点;
  r26 = r25->方法个数;
  r27 = cn_var_接口节点;
  r28 = r27->方法个数;
  r29 = cn_var_符号指针大小;
  分配清零内存(r28, r29);
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_2170;

  for_cond_2170:
  r30 = cn_var_i_1;
  r31 = cn_var_接口节点;
  r32 = r31->方法个数;
  r33 = r30 < r32;
  if (r33) goto for_body_2171; else goto for_exit_2173;

  for_body_2171:
  void cn_var_方法_2;
  r34 = cn_var_接口节点;
  r35 = r34->方法列表;
  r36 = cn_var_i_1;
  r37 = *(void*)cn_rt_array_get_element(r35, r36, 8);
  cn_var_方法_2 = r37;
  void cn_var_标志_3;
  void cn_var_方法符号_4;
  r38 = cn_var_方法_2;
  r39 = r38.名称;
  r40 = cn_var_方法_2;
  r41 = r40.参数列表;
  r42 = cn_var_方法_2;
  r43 = r42.参数个数;
  r44 = cn_var_方法_2;
  r45 = r44.返回类型;
  r46 = cn_var_接口节点;
  r47 = r46->节点基类;
  r48 = r47.位置;
  r49 = cn_var_标志_3;
  创建函数符号(r39, r41, r43, r45, r48, r49);
  cn_var_方法符号_4 = /* NONE */;
  r50 = cn_var_符号类型;
  r51 = r50.类成员符号;
  r52 = cn_var_方法符号_4;
  r53 = 7.成员列表;
  r54 = cn_var_i_1;
    { long long _tmp_r9 = r52; cn_rt_array_set_element(r53, r54, &_tmp_r9, 8); }
  r55 = cn_var_构建器;
  r56 = r55->符号表;
  r57 = cn_var_方法符号_4;
  插入符号(r56, r57);
  goto for_update_2172;

  for_update_2172:
  r58 = cn_var_i_1;
  r59 = r58 + 1;
  cn_var_i_1 = r59;
  goto for_cond_2170;

  for_exit_2173:
  r60 = cn_var_构建器;
  r61 = r60->符号表;
  离开作用域(r61);
  return;
}

void 构建导入声明作用域(struct 作用域构建器* cn_var_构建器, struct 导入声明* cn_var_导入节点) {
  long long r1, r2, r4, r6, r8, r9;
  struct 导入声明* r0;
  struct 导入声明* r5;
  struct 导入声明* r7;
  struct 作用域构建器* r10;
  struct 符号表管理器* r11;
  enum 符号类型 r3;

  entry:
  r0 = cn_var_导入节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2174; else goto if_merge_2175;

  if_then_2174:
  return;
  goto if_merge_2175;

  if_merge_2175:
  struct 符号* cn_var_导入符号_0;
  r2 = cn_var_符号大小;
  分配清零内存(1, r2);
  cn_var_导入符号_0 = /* NONE */;
  r3 = cn_var_符号类型;
  r4 = r3.导入符号;
  r5 = cn_var_导入节点;
  r6 = r5->模块名;
  r7 = cn_var_导入节点;
  r8 = r7->节点基类;
  r9 = r8.位置;
  r10 = cn_var_构建器;
  r11 = r10->符号表;
  插入符号(r11, 10);
  return;
}

void 构建语句作用域(struct 作用域构建器* cn_var_构建器, struct 语句节点* cn_var_语句) {
  long long r1, r3, r4, r6, r9, r12, r15, r18, r21, r24, r27, r41;
  struct 语句节点* r0;
  struct 语句节点* r2;
  struct 作用域构建器* r29;
  struct 语句节点* r30;
  struct 作用域构建器* r31;
  struct 语句节点* r32;
  struct 作用域构建器* r33;
  struct 语句节点* r34;
  struct 作用域构建器* r35;
  struct 语句节点* r36;
  struct 作用域构建器* r37;
  struct 语句节点* r38;
  struct 作用域构建器* r39;
  struct 语句节点* r40;
  struct 作用域构建器* r42;
  struct 语句节点* r43;
  struct 作用域构建器* r44;
  struct 语句节点* r45;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  enum 节点类型 r5;
  enum 节点类型 r8;
  enum 节点类型 r11;
  enum 节点类型 r14;
  enum 节点类型 r17;
  enum 节点类型 r20;
  enum 节点类型 r23;
  enum 节点类型 r26;

  entry:
  r0 = cn_var_语句;
  r1 = r0 == 0;
  if (r1) goto if_then_2176; else goto if_merge_2177;

  if_then_2176:
  return;
  goto if_merge_2177;

  if_merge_2177:
  r2 = cn_var_语句;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = cn_var_节点类型;
  r6 = r5.块语句;
  r7 = r4 == r6;
  if (r7) goto case_body_2179; else goto switch_check_2188;

  switch_check_2188:
  r8 = cn_var_节点类型;
  r9 = r8.如果语句;
  r10 = r4 == r9;
  if (r10) goto case_body_2180; else goto switch_check_2189;

  switch_check_2189:
  r11 = cn_var_节点类型;
  r12 = r11.当语句;
  r13 = r4 == r12;
  if (r13) goto case_body_2181; else goto switch_check_2190;

  switch_check_2190:
  r14 = cn_var_节点类型;
  r15 = r14.循环语句;
  r16 = r4 == r15;
  if (r16) goto case_body_2182; else goto switch_check_2191;

  switch_check_2191:
  r17 = cn_var_节点类型;
  r18 = r17.选择语句;
  r19 = r4 == r18;
  if (r19) goto case_body_2183; else goto switch_check_2192;

  switch_check_2192:
  r20 = cn_var_节点类型;
  r21 = r20.表达式语句;
  r22 = r4 == r21;
  if (r22) goto case_body_2184; else goto switch_check_2193;

  switch_check_2193:
  r23 = cn_var_节点类型;
  r24 = r23.返回语句;
  r25 = r4 == r24;
  if (r25) goto case_body_2185; else goto switch_check_2194;

  switch_check_2194:
  r26 = cn_var_节点类型;
  r27 = r26.尝试语句;
  r28 = r4 == r27;
  if (r28) goto case_body_2186; else goto case_default_2187;

  case_body_2179:
  r29 = cn_var_构建器;
  r30 = cn_var_语句;
  构建块语句作用域(r29, (struct 块语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2180:
  r31 = cn_var_构建器;
  r32 = cn_var_语句;
  构建如果语句作用域(r31, (struct 如果语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2181:
  r33 = cn_var_构建器;
  r34 = cn_var_语句;
  构建当语句作用域(r33, (struct 当语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2182:
  r35 = cn_var_构建器;
  r36 = cn_var_语句;
  构建循环语句作用域(r35, (struct 循环语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2183:
  r37 = cn_var_构建器;
  r38 = cn_var_语句;
  构建选择语句作用域(r37, (struct 选择语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2184:
  r39 = cn_var_构建器;
  r40 = cn_var_语句;
  r41 = (struct 表达式语句*)cn_var_语句->表达式;
  构建表达式作用域(r39, r41);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2185:
  r42 = cn_var_构建器;
  r43 = cn_var_语句;
  构建返回语句作用域(r42, (struct 返回语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_body_2186:
  r44 = cn_var_构建器;
  r45 = cn_var_语句;
  构建尝试语句作用域(r44, (struct 尝试语句*)cn_var_语句);
  goto switch_merge_2178;
  goto switch_merge_2178;

  case_default_2187:
  goto switch_merge_2178;
  goto switch_merge_2178;

  switch_merge_2178:
  return;
}

void 构建块语句作用域(struct 作用域构建器* cn_var_构建器, struct 块语句* cn_var_块节点) {
  long long r1, r5, r6, r8, r9, r12, r13, r14, r15, r16;
  struct 块语句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 块语句* r7;
  struct 作用域构建器* r10;
  struct 块语句* r11;
  struct 作用域构建器* r17;
  struct 符号表管理器* r18;
  enum 作用域类型 r4;

  entry:
  r0 = cn_var_块节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2195; else goto if_merge_2196;

  if_then_2195:
  return;
  goto if_merge_2196;

  if_merge_2196:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  r4 = cn_var_作用域类型;
  r5 = r4.块作用域;
  进入作用域(r3, r5, "块", 0);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2197;

  for_cond_2197:
  r6 = cn_var_i_0;
  r7 = cn_var_块节点;
  r8 = r7->语句个数;
  r9 = r6 < r8;
  if (r9) goto for_body_2198; else goto for_exit_2200;

  for_body_2198:
  r10 = cn_var_构建器;
  r11 = cn_var_块节点;
  r12 = r11->语句列表;
  r13 = cn_var_i_0;
  r14 = *(void*)cn_rt_array_get_element(r12, r13, 8);
  构建语句作用域(r10, r14);
  goto for_update_2199;

  for_update_2199:
  r15 = cn_var_i_0;
  r16 = r15 + 1;
  cn_var_i_0 = r16;
  goto for_cond_2197;

  for_exit_2200:
  r17 = cn_var_构建器;
  r18 = r17->符号表;
  离开作用域(r18);
  return;
}

void 构建如果语句作用域(struct 作用域构建器* cn_var_构建器, struct 如果语句* cn_var_如果节点) {
  long long r1, r4, r6, r7, r10, r12, r13, r16;
  struct 如果语句* r0;
  struct 作用域构建器* r2;
  struct 如果语句* r3;
  struct 如果语句* r5;
  struct 作用域构建器* r8;
  struct 如果语句* r9;
  struct 如果语句* r11;
  struct 作用域构建器* r14;
  struct 如果语句* r15;

  entry:
  r0 = cn_var_如果节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2201; else goto if_merge_2202;

  if_then_2201:
  return;
  goto if_merge_2202;

  if_merge_2202:
  r2 = cn_var_构建器;
  r3 = cn_var_如果节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_如果节点;
  r6 = r5->真分支;
  r7 = r6 != 0;
  if (r7) goto if_then_2203; else goto if_merge_2204;

  if_then_2203:
  r8 = cn_var_构建器;
  r9 = cn_var_如果节点;
  r10 = r9->真分支;
  构建语句作用域(r8, r10);
  goto if_merge_2204;

  if_merge_2204:
  r11 = cn_var_如果节点;
  r12 = r11->假分支;
  r13 = r12 != 0;
  if (r13) goto if_then_2205; else goto if_merge_2206;

  if_then_2205:
  r14 = cn_var_构建器;
  r15 = cn_var_如果节点;
  r16 = r15->假分支;
  构建语句作用域(r14, r16);
  goto if_merge_2206;

  if_merge_2206:
  return;
}

void 构建当语句作用域(struct 作用域构建器* cn_var_构建器, struct 当语句* cn_var_当节点) {
  long long r1, r4, r8, r12, r13, r16;
  struct 当语句* r0;
  struct 作用域构建器* r2;
  struct 当语句* r3;
  struct 作用域构建器* r5;
  struct 符号表管理器* r6;
  struct 作用域构建器* r9;
  struct 符号表管理器* r10;
  struct 当语句* r11;
  struct 作用域构建器* r14;
  struct 当语句* r15;
  struct 作用域构建器* r17;
  struct 符号表管理器* r18;
  enum 作用域类型 r7;

  entry:
  r0 = cn_var_当节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2207; else goto if_merge_2208;

  if_then_2207:
  return;
  goto if_merge_2208;

  if_merge_2208:
  r2 = cn_var_构建器;
  r3 = cn_var_当节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = r5->符号表;
  r7 = cn_var_作用域类型;
  r8 = r7.循环作用域;
  进入作用域(r6, r8, "当循环", 0);
  r9 = cn_var_构建器;
  r10 = r9->符号表;
  设置循环作用域(r10);
  r11 = cn_var_当节点;
  r12 = r11->循环体;
  r13 = r12 != 0;
  if (r13) goto if_then_2209; else goto if_merge_2210;

  if_then_2209:
  r14 = cn_var_构建器;
  r15 = cn_var_当节点;
  r16 = r15->循环体;
  构建语句作用域(r14, r16);
  goto if_merge_2210;

  if_merge_2210:
  r17 = cn_var_构建器;
  r18 = r17->符号表;
  离开作用域(r18);
  return;
}

void 构建循环语句作用域(struct 作用域构建器* cn_var_构建器, struct 循环语句* cn_var_循环节点) {
  long long r1, r5, r9, r10, r13, r15, r16, r19, r21, r22, r25, r27, r28, r31;
  struct 循环语句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 作用域构建器* r6;
  struct 符号表管理器* r7;
  struct 循环语句* r8;
  struct 作用域构建器* r11;
  struct 循环语句* r12;
  struct 循环语句* r14;
  struct 作用域构建器* r17;
  struct 循环语句* r18;
  struct 循环语句* r20;
  struct 作用域构建器* r23;
  struct 循环语句* r24;
  struct 循环语句* r26;
  struct 作用域构建器* r29;
  struct 循环语句* r30;
  struct 作用域构建器* r32;
  struct 符号表管理器* r33;
  enum 作用域类型 r4;

  entry:
  r0 = cn_var_循环节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2211; else goto if_merge_2212;

  if_then_2211:
  return;
  goto if_merge_2212;

  if_merge_2212:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  r4 = cn_var_作用域类型;
  r5 = r4.循环作用域;
  进入作用域(r3, r5, "循环", 0);
  r6 = cn_var_构建器;
  r7 = r6->符号表;
  设置循环作用域(r7);
  r8 = cn_var_循环节点;
  r9 = r8->初始化;
  r10 = r9 != 0;
  if (r10) goto if_then_2213; else goto if_merge_2214;

  if_then_2213:
  r11 = cn_var_构建器;
  r12 = cn_var_循环节点;
  r13 = r12->初始化;
  构建语句作用域(r11, r13);
  goto if_merge_2214;

  if_merge_2214:
  r14 = cn_var_循环节点;
  r15 = r14->条件;
  r16 = r15 != 0;
  if (r16) goto if_then_2215; else goto if_merge_2216;

  if_then_2215:
  r17 = cn_var_构建器;
  r18 = cn_var_循环节点;
  r19 = r18->条件;
  构建表达式作用域(r17, r19);
  goto if_merge_2216;

  if_merge_2216:
  r20 = cn_var_循环节点;
  r21 = r20->更新;
  r22 = r21 != 0;
  if (r22) goto if_then_2217; else goto if_merge_2218;

  if_then_2217:
  r23 = cn_var_构建器;
  r24 = cn_var_循环节点;
  r25 = r24->更新;
  构建表达式作用域(r23, r25);
  goto if_merge_2218;

  if_merge_2218:
  r26 = cn_var_循环节点;
  r27 = r26->循环体;
  r28 = r27 != 0;
  if (r28) goto if_then_2219; else goto if_merge_2220;

  if_then_2219:
  r29 = cn_var_构建器;
  r30 = cn_var_循环节点;
  r31 = r30->循环体;
  构建语句作用域(r29, r31);
  goto if_merge_2220;

  if_merge_2220:
  r32 = cn_var_构建器;
  r33 = r32->符号表;
  离开作用域(r33);
  return;
}

void 构建选择语句作用域(struct 作用域构建器* cn_var_构建器, struct 选择语句* cn_var_选择节点) {
  long long r1, r4, r5, r7, r8, r11, r12, r13, r14, r15, r17, r18, r21;
  struct 选择语句* r0;
  struct 作用域构建器* r2;
  struct 选择语句* r3;
  struct 选择语句* r6;
  struct 作用域构建器* r9;
  struct 选择语句* r10;
  struct 选择语句* r16;
  struct 作用域构建器* r19;
  struct 选择语句* r20;

  entry:
  r0 = cn_var_选择节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2221; else goto if_merge_2222;

  if_then_2221:
  return;
  goto if_merge_2222;

  if_merge_2222:
  r2 = cn_var_构建器;
  r3 = cn_var_选择节点;
  r4 = r3->选择值;
  构建表达式作用域(r2, r4);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2223;

  for_cond_2223:
  r5 = cn_var_i_0;
  r6 = cn_var_选择节点;
  r7 = r6->情况个数;
  r8 = r5 < r7;
  if (r8) goto for_body_2224; else goto for_exit_2226;

  for_body_2224:
  r9 = cn_var_构建器;
  r10 = cn_var_选择节点;
  r11 = r10->情况列表;
  r12 = cn_var_i_0;
  r13 = *(void*)cn_rt_array_get_element(r11, r12, 8);
  构建情况语句作用域(r9, r13);
  goto for_update_2225;

  for_update_2225:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_2223;

  for_exit_2226:
  r16 = cn_var_选择节点;
  r17 = r16->默认分支;
  r18 = r17 != 0;
  if (r18) goto if_then_2227; else goto if_merge_2228;

  if_then_2227:
  r19 = cn_var_构建器;
  r20 = cn_var_选择节点;
  r21 = r20->默认分支;
  构建语句作用域(r19, r21);
  goto if_merge_2228;

  if_merge_2228:
  return;
}

void 构建情况语句作用域(struct 作用域构建器* cn_var_构建器, struct 情况语句* cn_var_情况节点) {
  long long r1, r4, r5, r7, r8, r11, r12, r13, r14, r15;
  struct 情况语句* r0;
  struct 作用域构建器* r2;
  struct 情况语句* r3;
  struct 情况语句* r6;
  struct 作用域构建器* r9;
  struct 情况语句* r10;

  entry:
  r0 = cn_var_情况节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2229; else goto if_merge_2230;

  if_then_2229:
  return;
  goto if_merge_2230;

  if_merge_2230:
  r2 = cn_var_构建器;
  r3 = cn_var_情况节点;
  r4 = r3->值;
  构建表达式作用域(r2, r4);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2231;

  for_cond_2231:
  r5 = cn_var_i_0;
  r6 = cn_var_情况节点;
  r7 = r6->语句个数;
  r8 = r5 < r7;
  if (r8) goto for_body_2232; else goto for_exit_2234;

  for_body_2232:
  r9 = cn_var_构建器;
  r10 = cn_var_情况节点;
  r11 = r10->语句列表;
  r12 = cn_var_i_0;
  r13 = *(void*)cn_rt_array_get_element(r11, r12, 8);
  构建语句作用域(r9, r13);
  goto for_update_2233;

  for_update_2233:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_2231;

  for_exit_2234:
  return;
}

void 构建返回语句作用域(struct 作用域构建器* cn_var_构建器, struct 返回语句* cn_var_返回节点) {
  long long r1, r3, r4, r7;
  struct 返回语句* r0;
  struct 返回语句* r2;
  struct 作用域构建器* r5;
  struct 返回语句* r6;

  entry:
  r0 = cn_var_返回节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2235; else goto if_merge_2236;

  if_then_2235:
  return;
  goto if_merge_2236;

  if_merge_2236:
  r2 = cn_var_返回节点;
  r3 = r2->返回值;
  r4 = r3 != 0;
  if (r4) goto if_then_2237; else goto if_merge_2238;

  if_then_2237:
  r5 = cn_var_构建器;
  r6 = cn_var_返回节点;
  r7 = r6->返回值;
  构建表达式作用域(r5, r7);
  goto if_merge_2238;

  if_merge_2238:
  return;
}

void 构建尝试语句作用域(struct 作用域构建器* cn_var_构建器, struct 尝试语句* cn_var_尝试节点) {
  long long r1, r3, r4, r7, r8, r10, r11, r14, r15, r16, r17, r18, r20, r21, r24;
  struct 尝试语句* r0;
  struct 尝试语句* r2;
  struct 作用域构建器* r5;
  struct 尝试语句* r6;
  struct 尝试语句* r9;
  struct 作用域构建器* r12;
  struct 尝试语句* r13;
  struct 尝试语句* r19;
  struct 作用域构建器* r22;
  struct 尝试语句* r23;

  entry:
  r0 = cn_var_尝试节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2239; else goto if_merge_2240;

  if_then_2239:
  return;
  goto if_merge_2240;

  if_merge_2240:
  r2 = cn_var_尝试节点;
  r3 = r2->尝试块;
  r4 = r3 != 0;
  if (r4) goto if_then_2241; else goto if_merge_2242;

  if_then_2241:
  r5 = cn_var_构建器;
  r6 = cn_var_尝试节点;
  r7 = r6->尝试块;
  构建语句作用域(r5, r7);
  goto if_merge_2242;

  if_merge_2242:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2243;

  for_cond_2243:
  r8 = cn_var_i_0;
  r9 = cn_var_尝试节点;
  r10 = r9->捕获个数;
  r11 = r8 < r10;
  if (r11) goto for_body_2244; else goto for_exit_2246;

  for_body_2244:
  r12 = cn_var_构建器;
  r13 = cn_var_尝试节点;
  r14 = r13->捕获列表;
  r15 = cn_var_i_0;
  r16 = *(void*)cn_rt_array_get_element(r14, r15, 8);
  构建捕获语句作用域(r12, r16);
  goto for_update_2245;

  for_update_2245:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_2243;

  for_exit_2246:
  r19 = cn_var_尝试节点;
  r20 = r19->最终块;
  r21 = r20 != 0;
  if (r21) goto if_then_2247; else goto if_merge_2248;

  if_then_2247:
  r22 = cn_var_构建器;
  r23 = cn_var_尝试节点;
  r24 = r23->最终块;
  构建语句作用域(r22, r24);
  goto if_merge_2248;

  if_merge_2248:
  return;
}

void 构建捕获语句作用域(struct 作用域构建器* cn_var_构建器, struct 捕获语句* cn_var_捕获节点) {
  long long r1, r5, r7, r8, r11, r13, r14, r17;
  struct 捕获语句* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 捕获语句* r6;
  struct 作用域构建器* r9;
  struct 捕获语句* r10;
  struct 捕获语句* r12;
  struct 作用域构建器* r15;
  struct 捕获语句* r16;
  struct 作用域构建器* r18;
  struct 符号表管理器* r19;
  enum 作用域类型 r4;

  entry:
  r0 = cn_var_捕获节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2249; else goto if_merge_2250;

  if_then_2249:
  return;
  goto if_merge_2250;

  if_merge_2250:
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  r4 = cn_var_作用域类型;
  r5 = r4.块作用域;
  进入作用域(r3, r5, "捕获", 0);
  r6 = cn_var_捕获节点;
  r7 = r6->异常变量;
  r8 = r7 != 0;
  if (r8) goto if_then_2251; else goto if_merge_2252;

  if_then_2251:
  r9 = cn_var_构建器;
  r10 = cn_var_捕获节点;
  r11 = r10->异常变量;
  构建变量声明作用域(r9, r11);
  goto if_merge_2252;

  if_merge_2252:
  r12 = cn_var_捕获节点;
  r13 = r12->捕获体;
  r14 = r13 != 0;
  if (r14) goto if_then_2253; else goto if_merge_2254;

  if_then_2253:
  r15 = cn_var_构建器;
  r16 = cn_var_捕获节点;
  r17 = r16->捕获体;
  构建语句作用域(r15, r17);
  goto if_merge_2254;

  if_merge_2254:
  r18 = cn_var_构建器;
  r19 = r18->符号表;
  离开作用域(r19);
  return;
}

void 构建表达式作用域(struct 作用域构建器* cn_var_构建器, struct 表达式节点* cn_var_表达式) {
  long long r1, r3, r4, r6, r9, r12, r15, r18, r21, r24, r27;
  struct 表达式节点* r0;
  struct 表达式节点* r2;
  struct 作用域构建器* r29;
  struct 表达式节点* r30;
  struct 作用域构建器* r31;
  struct 表达式节点* r32;
  struct 作用域构建器* r33;
  struct 表达式节点* r34;
  struct 作用域构建器* r35;
  struct 表达式节点* r36;
  struct 作用域构建器* r37;
  struct 表达式节点* r38;
  struct 作用域构建器* r39;
  struct 表达式节点* r40;
  struct 作用域构建器* r41;
  struct 表达式节点* r42;
  struct 作用域构建器* r43;
  struct 表达式节点* r44;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  enum 节点类型 r5;
  enum 节点类型 r8;
  enum 节点类型 r11;
  enum 节点类型 r14;
  enum 节点类型 r17;
  enum 节点类型 r20;
  enum 节点类型 r23;
  enum 节点类型 r26;

  entry:
  r0 = cn_var_表达式;
  r1 = r0 == 0;
  if (r1) goto if_then_2255; else goto if_merge_2256;

  if_then_2255:
  return;
  goto if_merge_2256;

  if_merge_2256:
  r2 = cn_var_表达式;
  r3 = r2->节点基类;
  r4 = r3.类型;
  r5 = cn_var_节点类型;
  r6 = r5.二元表达式;
  r7 = r4 == r6;
  if (r7) goto case_body_2258; else goto switch_check_2267;

  switch_check_2267:
  r8 = cn_var_节点类型;
  r9 = r8.一元表达式;
  r10 = r4 == r9;
  if (r10) goto case_body_2259; else goto switch_check_2268;

  switch_check_2268:
  r11 = cn_var_节点类型;
  r12 = r11.函数调用表达式;
  r13 = r4 == r12;
  if (r13) goto case_body_2260; else goto switch_check_2269;

  switch_check_2269:
  r14 = cn_var_节点类型;
  r15 = r14.成员访问表达式;
  r16 = r4 == r15;
  if (r16) goto case_body_2261; else goto switch_check_2270;

  switch_check_2270:
  r17 = cn_var_节点类型;
  r18 = r17.数组访问表达式;
  r19 = r4 == r18;
  if (r19) goto case_body_2262; else goto switch_check_2271;

  switch_check_2271:
  r20 = cn_var_节点类型;
  r21 = r20.赋值表达式;
  r22 = r4 == r21;
  if (r22) goto case_body_2263; else goto switch_check_2272;

  switch_check_2272:
  r23 = cn_var_节点类型;
  r24 = r23.三元表达式;
  r25 = r4 == r24;
  if (r25) goto case_body_2264; else goto switch_check_2273;

  switch_check_2273:
  r26 = cn_var_节点类型;
  r27 = r26.标识符表达式;
  r28 = r4 == r27;
  if (r28) goto case_body_2265; else goto case_default_2266;

  case_body_2258:
  r29 = cn_var_构建器;
  r30 = cn_var_表达式;
  构建二元表达式作用域(r29, (struct 二元表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2259:
  r31 = cn_var_构建器;
  r32 = cn_var_表达式;
  构建一元表达式作用域(r31, (struct 一元表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2260:
  r33 = cn_var_构建器;
  r34 = cn_var_表达式;
  构建函数调用表达式作用域(r33, (struct 函数调用表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2261:
  r35 = cn_var_构建器;
  r36 = cn_var_表达式;
  构建成员访问表达式作用域(r35, (struct 成员访问表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2262:
  r37 = cn_var_构建器;
  r38 = cn_var_表达式;
  构建数组访问表达式作用域(r37, (struct 数组访问表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2263:
  r39 = cn_var_构建器;
  r40 = cn_var_表达式;
  构建赋值表达式作用域(r39, (struct 赋值表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2264:
  r41 = cn_var_构建器;
  r42 = cn_var_表达式;
  构建三元表达式作用域(r41, (struct 三元表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_body_2265:
  r43 = cn_var_构建器;
  r44 = cn_var_表达式;
  构建标识符表达式作用域(r43, (struct 标识符表达式*)cn_var_表达式);
  goto switch_merge_2257;
  goto switch_merge_2257;

  case_default_2266:
  goto switch_merge_2257;
  goto switch_merge_2257;

  switch_merge_2257:
  return;
}

void 构建二元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 二元表达式* cn_var_二元节点) {
  long long r1, r4, r7;
  struct 二元表达式* r0;
  struct 作用域构建器* r2;
  struct 二元表达式* r3;
  struct 作用域构建器* r5;
  struct 二元表达式* r6;

  entry:
  r0 = cn_var_二元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2274; else goto if_merge_2275;

  if_then_2274:
  return;
  goto if_merge_2275;

  if_merge_2275:
  r2 = cn_var_构建器;
  r3 = cn_var_二元节点;
  r4 = r3->左操作数;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_二元节点;
  r7 = r6->右操作数;
  构建表达式作用域(r5, r7);
  return;
}

void 构建一元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 一元表达式* cn_var_一元节点) {
  long long r1, r4;
  struct 一元表达式* r0;
  struct 作用域构建器* r2;
  struct 一元表达式* r3;

  entry:
  r0 = cn_var_一元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2276; else goto if_merge_2277;

  if_then_2276:
  return;
  goto if_merge_2277;

  if_merge_2277:
  r2 = cn_var_构建器;
  r3 = cn_var_一元节点;
  r4 = r3->操作数;
  构建表达式作用域(r2, r4);
  return;
}

void 构建函数调用表达式作用域(struct 作用域构建器* cn_var_构建器, struct 函数调用表达式* cn_var_调用节点) {
  long long r1, r4, r5, r7, r8, r11, r12, r13, r14, r15;
  struct 函数调用表达式* r0;
  struct 作用域构建器* r2;
  struct 函数调用表达式* r3;
  struct 函数调用表达式* r6;
  struct 作用域构建器* r9;
  struct 函数调用表达式* r10;

  entry:
  r0 = cn_var_调用节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2278; else goto if_merge_2279;

  if_then_2278:
  return;
  goto if_merge_2279;

  if_merge_2279:
  r2 = cn_var_构建器;
  r3 = cn_var_调用节点;
  r4 = r3->被调函数;
  构建表达式作用域(r2, r4);
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_2280;

  for_cond_2280:
  r5 = cn_var_i_0;
  r6 = cn_var_调用节点;
  r7 = r6->参数个数;
  r8 = r5 < r7;
  if (r8) goto for_body_2281; else goto for_exit_2283;

  for_body_2281:
  r9 = cn_var_构建器;
  r10 = cn_var_调用节点;
  r11 = r10->参数;
  r12 = cn_var_i_0;
  r13 = *(void*)cn_rt_array_get_element(r11, r12, 8);
  构建表达式作用域(r9, r13);
  goto for_update_2282;

  for_update_2282:
  r14 = cn_var_i_0;
  r15 = r14 + 1;
  cn_var_i_0 = r15;
  goto for_cond_2280;

  for_exit_2283:
  return;
}

void 构建成员访问表达式作用域(struct 作用域构建器* cn_var_构建器, struct 成员访问表达式* cn_var_成员节点) {
  long long r1, r4;
  struct 成员访问表达式* r0;
  struct 作用域构建器* r2;
  struct 成员访问表达式* r3;

  entry:
  r0 = cn_var_成员节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2284; else goto if_merge_2285;

  if_then_2284:
  return;
  goto if_merge_2285;

  if_merge_2285:
  r2 = cn_var_构建器;
  r3 = cn_var_成员节点;
  r4 = r3->对象;
  构建表达式作用域(r2, r4);
  return;
}

void 构建数组访问表达式作用域(struct 作用域构建器* cn_var_构建器, struct 数组访问表达式* cn_var_数组节点) {
  long long r1, r4, r7;
  struct 数组访问表达式* r0;
  struct 作用域构建器* r2;
  struct 数组访问表达式* r3;
  struct 作用域构建器* r5;
  struct 数组访问表达式* r6;

  entry:
  r0 = cn_var_数组节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2286; else goto if_merge_2287;

  if_then_2286:
  return;
  goto if_merge_2287;

  if_merge_2287:
  r2 = cn_var_构建器;
  r3 = cn_var_数组节点;
  r4 = r3->数组;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_数组节点;
  r7 = r6->索引;
  构建表达式作用域(r5, r7);
  return;
}

void 构建赋值表达式作用域(struct 作用域构建器* cn_var_构建器, struct 赋值表达式* cn_var_赋值节点) {
  long long r1, r4, r7;
  struct 赋值表达式* r0;
  struct 作用域构建器* r2;
  struct 赋值表达式* r3;
  struct 作用域构建器* r5;
  struct 赋值表达式* r6;

  entry:
  r0 = cn_var_赋值节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2288; else goto if_merge_2289;

  if_then_2288:
  return;
  goto if_merge_2289;

  if_merge_2289:
  r2 = cn_var_构建器;
  r3 = cn_var_赋值节点;
  r4 = r3->目标;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_赋值节点;
  r7 = r6->值;
  构建表达式作用域(r5, r7);
  return;
}

void 构建三元表达式作用域(struct 作用域构建器* cn_var_构建器, struct 三元表达式* cn_var_三元节点) {
  long long r1, r4, r7, r10;
  struct 三元表达式* r0;
  struct 作用域构建器* r2;
  struct 三元表达式* r3;
  struct 作用域构建器* r5;
  struct 三元表达式* r6;
  struct 作用域构建器* r8;
  struct 三元表达式* r9;

  entry:
  r0 = cn_var_三元节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2290; else goto if_merge_2291;

  if_then_2290:
  return;
  goto if_merge_2291;

  if_merge_2291:
  r2 = cn_var_构建器;
  r3 = cn_var_三元节点;
  r4 = r3->条件;
  构建表达式作用域(r2, r4);
  r5 = cn_var_构建器;
  r6 = cn_var_三元节点;
  r7 = r6->真值;
  构建表达式作用域(r5, r7);
  r8 = cn_var_构建器;
  r9 = cn_var_三元节点;
  r10 = r9->假值;
  构建表达式作用域(r8, r10);
  return;
}

void 构建标识符表达式作用域(struct 作用域构建器* cn_var_构建器, struct 标识符表达式* cn_var_标识符节点) {
  long long r1, r5, r6, r7, r10, r12, r13;
  struct 标识符表达式* r0;
  struct 作用域构建器* r2;
  struct 符号表管理器* r3;
  struct 标识符表达式* r4;
  struct 作用域构建器* r8;
  struct 标识符表达式* r9;
  struct 标识符表达式* r11;

  entry:
  r0 = cn_var_标识符节点;
  r1 = r0 == 0;
  if (r1) goto if_then_2292; else goto if_merge_2293;

  if_then_2292:
  return;
  goto if_merge_2293;

  if_merge_2293:
  void cn_var_符号指针_0;
  r2 = cn_var_构建器;
  r3 = r2->符号表;
  r4 = cn_var_标识符节点;
  r5 = r4->名称;
  查找符号(r3, r5);
  cn_var_符号指针_0 = /* NONE */;
  r6 = cn_var_符号指针_0;
  r7 = r6 == 0;
  if (r7) goto if_then_2294; else goto if_else_2295;

  if_then_2294:
  r8 = cn_var_构建器;
  r9 = cn_var_标识符节点;
  r10 = r9->名称;
  r11 = cn_var_标识符节点;
  r12 = r11->节点基类;
  r13 = r12.位置;
  报告未定义标识符错误(r8, r10, r13);
  goto if_merge_2296;

  if_else_2295:
  goto if_merge_2296;

  if_merge_2296:
  return;
}

void 报告重复符号错误(struct 作用域构建器* cn_var_构建器, char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r1, r2;
  struct 作用域构建器* r0;

  entry:
  r0 = cn_var_构建器;
  r1 = r0->错误计数;
  r2 = r1 + 1;
  return;
}

void 报告未定义标识符错误(struct 作用域构建器* cn_var_构建器, char* cn_var_名称, struct 源位置 cn_var_位置) {
  long long r1, r2;
  struct 作用域构建器* r0;

  entry:
  r0 = cn_var_构建器;
  r1 = r0->错误计数;
  r2 = r1 + 1;
  return;
}

