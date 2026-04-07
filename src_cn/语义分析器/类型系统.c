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

// CN Language Enum Definitions
enum 类型种类 {
    类型种类_类型_空,
    类型种类_类型_整数,
    类型种类_类型_小数,
    类型种类_类型_布尔,
    类型种类_类型_字符串,
    类型种类_类型_字符,
    类型种类_类型_指针,
    类型种类_类型_数组,
    类型种类_类型_结构体,
    类型种类_类型_枚举,
    类型种类_类型_类,
    类型种类_类型_接口,
    类型种类_类型_函数,
    类型种类_类型_参数,
    类型种类_类型_未定义
};

enum 类型兼容性 {
    类型兼容性_完全兼容,
    类型兼容性_隐式转换,
    类型兼容性_显式转换,
    类型兼容性_不兼容
};

// CN Language Global Struct Forward Declarations
struct 类型信息;
struct 类型推断上下文;

// CN Language Global Struct Definitions
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

struct 类型推断上下文 {
    struct 符号表管理器* 符号表;
    struct 类型信息* 期望类型;
    long long 错误计数;
};

// Global Variables
long long cn_var_整数大小 = 4;
long long cn_var_小数大小 = 8;
long long cn_var_布尔大小 = 1;
long long cn_var_字符串大小 = 8;
long long cn_var_指针大小 = 8;

// Forward Declarations
struct 类型信息* 创建空类型();
struct 类型信息* 创建整数类型(char*, long long, _Bool);
struct 类型信息* 创建小数类型(char*, long long);
struct 类型信息* 创建布尔类型();
struct 类型信息* 创建字符串类型();
struct 类型信息* 创建指针类型(struct 类型信息*);
struct 类型信息* 创建数组类型(struct 类型信息*, long long*, long long);
struct 类型信息* 创建函数类型(struct 类型信息**, long long, struct 类型信息*);
struct 类型信息* 创建结构体类型(char*, struct 符号*);
struct 类型信息* 创建枚举类型(char*, struct 符号*);
struct 类型信息* 创建类类型(char*, struct 符号*);
struct 类型信息* 创建接口类型(char*, struct 符号*);
_Bool 类型相同(struct 类型信息*, struct 类型信息*);
enum 类型兼容性 检查类型兼容性(struct 类型信息*, struct 类型信息*);
_Bool 是派生类(struct 类型信息*, struct 类型信息*);
_Bool 实现接口(struct 类型信息*, struct 类型信息*);
_Bool 是数值类型(struct 类型信息*);
_Bool 是整数类型(struct 类型信息*);
_Bool 是指针类型(struct 类型信息*);
_Bool 是数组类型(struct 类型信息*);
_Bool 是结构体类型(struct 类型信息*);
_Bool 是类类型(struct 类型信息*);
_Bool 是函数类型(struct 类型信息*);
_Bool 是可调用类型(struct 类型信息*);
struct 类型信息* 从类型节点推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从基础类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从指针类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从数组类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从结构体类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从枚举类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从类类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从接口类型推断(struct 类型节点*, struct 类型推断上下文*);
struct 类型信息* 从函数类型推断(struct 类型节点*, struct 类型推断上下文*);
char* 获取类型种类名称(enum 类型种类);

struct 类型信息* 创建空类型() {
  long long r0, r1, r3;
  struct 类型信息* r4;
  enum 类型种类 r2;

  entry:
  void cn_var_内存指针_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_内存指针_0 = /* NONE */;
  struct 类型信息* cn_var_类型_1;
  r1 = cn_var_内存指针_0;
  cn_var_类型_1 = (struct 类型信息*)cn_var_内存指针;
  r2 = cn_var_类型种类;
  r3 = r2.类型_空;
  r4 = cn_var_类型_1;
  return r4;
}

struct 类型信息* 创建整数类型(char* cn_var_名称, long long cn_var_大小, _Bool cn_var_是有符号) {
  long long r0, r2, r4, r5;
  char* r3;
  struct 类型信息* r7;
  _Bool r6;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_整数;
  r3 = cn_var_名称;
  r4 = cn_var_大小;
  r5 = cn_var_大小;
  r6 = cn_var_是有符号;
  r7 = cn_var_类型_0;
  return r7;
}

struct 类型信息* 创建小数类型(char* cn_var_名称, long long cn_var_大小) {
  long long r0, r2, r4, r5;
  char* r3;
  struct 类型信息* r6;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_小数;
  r3 = cn_var_名称;
  r4 = cn_var_大小;
  r5 = cn_var_大小;
  r6 = cn_var_类型_0;
  return r6;
}

struct 类型信息* 创建布尔类型() {
  long long r0, r2, r3;
  struct 类型信息* r4;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_布尔;
  r3 = cn_var_布尔大小;
  r4 = cn_var_类型_0;
  return r4;
}

struct 类型信息* 创建字符串类型() {
  long long r0, r2, r3, r4;
  struct 类型信息* r5;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_字符串;
  r3 = cn_var_字符串大小;
  r4 = cn_var_指针大小;
  r5 = cn_var_类型_0;
  return r5;
}

struct 类型信息* 创建指针类型(struct 类型信息* cn_var_指向类型) {
  long long r0, r2, r5, r6, r7, r8, r11;
  char* r10;
  struct 类型信息* r3;
  struct 类型信息* r4;
  struct 类型信息* r9;
  struct 类型信息* r12;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_指针;
  r3 = cn_var_指向类型;
  r4 = cn_var_指向类型;
  r5 = r4->指针层级;
  r6 = r5 + 1;
  r7 = cn_var_指针大小;
  r8 = cn_var_指针大小;
  r9 = cn_var_指向类型;
  r10 = r9->名称;
  r11 = r10 + "*";
  r12 = cn_var_类型_0;
  return r12;
}

struct 类型信息* 创建数组类型(struct 类型信息* cn_var_元素类型, long long* cn_var_维度大小, long long cn_var_维度) {
  long long r0, r2, r4, r6, r7, r8, r9, r11, r12, r13, r14, r15, r16, r17, r19, r20, r22;
  struct 类型信息* r3;
  long long* r5;
  long long* r10;
  struct 类型信息* r18;
  struct 类型信息* r21;
  struct 类型信息* r23;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_数组;
  r3 = cn_var_元素类型;
  r4 = cn_var_维度;
  r5 = cn_var_维度大小;
  long long cn_var_总数_1;
  cn_var_总数_1 = 1;
  long long cn_var_i_2;
  cn_var_i_2 = 0;
  goto for_cond_1830;

  for_cond_1830:
  r6 = cn_var_i_2;
  r7 = cn_var_维度;
  r8 = r6 < r7;
  if (r8) goto for_body_1831; else goto for_exit_1833;

  for_body_1831:
  r9 = cn_var_总数_1;
  r10 = cn_var_维度大小;
  r11 = cn_var_i_2;
  r12 = *(void*)cn_rt_array_get_element(r10, r11, 8);
  r13 = r9 * r12;
  cn_var_总数_1 = r13;
  goto for_update_1832;

  for_update_1832:
  r14 = cn_var_i_2;
  r15 = r14 + 1;
  cn_var_i_2 = r15;
  goto for_cond_1830;

  for_exit_1833:
  r16 = cn_var_总数_1;
  r17 = cn_var_总数_1;
  r18 = cn_var_元素类型;
  r19 = r18->大小;
  r20 = r17 * r19;
  r21 = cn_var_元素类型;
  r22 = r21->对齐;
  r23 = cn_var_类型_0;
  return r23;
}

struct 类型信息* 创建函数类型(struct 类型信息** cn_var_参数类型列表, long long cn_var_参数个数, struct 类型信息* cn_var_返回类型) {
  long long r0, r2, r4, r6, r7;
  struct 类型信息** r3;
  struct 类型信息* r5;
  struct 类型信息* r8;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_函数;
  r3 = cn_var_参数类型列表;
  r4 = cn_var_参数个数;
  r5 = cn_var_返回类型;
  r6 = cn_var_指针大小;
  r7 = cn_var_指针大小;
  r8 = cn_var_类型_0;
  return r8;
}

struct 类型信息* 创建结构体类型(char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0, r2;
  char* r3;
  struct 符号* r4;
  struct 类型信息* r5;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_结构体;
  r3 = cn_var_名称;
  r4 = cn_var_类型符号;
  r5 = cn_var_类型_0;
  return r5;
}

struct 类型信息* 创建枚举类型(char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0, r2, r5, r6;
  char* r3;
  struct 符号* r4;
  struct 类型信息* r7;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_枚举;
  r3 = cn_var_名称;
  r4 = cn_var_类型符号;
  r5 = cn_var_整数大小;
  r6 = cn_var_整数大小;
  r7 = cn_var_类型_0;
  return r7;
}

struct 类型信息* 创建类类型(char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0, r2;
  char* r3;
  struct 符号* r4;
  struct 类型信息* r5;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_类;
  r3 = cn_var_名称;
  r4 = cn_var_类型符号;
  r5 = cn_var_类型_0;
  return r5;
}

struct 类型信息* 创建接口类型(char* cn_var_名称, struct 符号* cn_var_类型符号) {
  long long r0, r2;
  char* r3;
  struct 符号* r4;
  struct 类型信息* r5;
  enum 类型种类 r1;

  entry:
  struct 类型信息* cn_var_类型_0;
  r0 = sizeof(struct 类型信息);
  分配清零内存(1, r0);
  cn_var_类型_0 = /* NONE */;
  r1 = cn_var_类型种类;
  r2 = r1.类型_接口;
  r3 = cn_var_名称;
  r4 = cn_var_类型符号;
  r5 = cn_var_类型_0;
  return r5;
}

_Bool 类型相同(struct 类型信息* cn_var_类型1, struct 类型信息* cn_var_类型2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r37, r38, r39, r40, r41, r42, r43, r44, r45, r46, r47, r48, r49, r50, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r62, r63, r64, r65, r66, r67, r68, r69, r70, r71, r72, r73, r74, r75, r76, r77, r78, r79, r80, r81, r82, r83, r84, r85, r86, r87, r88, r89, r90, r91, r92, r93, r94, r95, r96, r97, r98, r99, r100, r101, r102, r103, r104, r105;

  entry:
  goto tail_rec_loop;
  return 0;
}

enum 类型兼容性 检查类型兼容性(struct 类型信息* cn_var_源类型, struct 类型信息* cn_var_目标类型) {
  long long r3, r4, r8, r9, r13, r14, r19, r20, r22, r23, r27, r28, r32, r33, r35, r36, r40, r41, r45, r46, r48, r49, r53, r54, r58, r59, r61, r62, r66, r67, r71, r72, r74, r75, r79, r80, r84, r85, r87, r88, r92, r93, r97, r98, r102, r103, r107, r108, r112, r113, r117, r119;
  struct 类型信息* r0;
  struct 类型信息* r1;
  struct 类型信息* r5;
  struct 类型信息* r10;
  struct 类型信息* r15;
  struct 类型信息* r16;
  struct 类型信息* r24;
  struct 类型信息* r29;
  struct 类型信息* r37;
  struct 类型信息* r42;
  struct 类型信息* r50;
  struct 类型信息* r55;
  struct 类型信息* r63;
  struct 类型信息* r68;
  struct 类型信息* r76;
  struct 类型信息* r81;
  struct 类型信息* r89;
  struct 类型信息* r94;
  struct 类型信息* r99;
  struct 类型信息* r100;
  struct 类型信息* r104;
  struct 类型信息* r109;
  struct 类型信息* r114;
  struct 类型信息* r115;
  enum 类型兼容性 r2;
  enum 类型种类 r6;
  enum 类型种类 r7;
  enum 类型种类 r11;
  enum 类型种类 r12;
  enum 类型种类 r17;
  enum 类型种类 r18;
  enum 类型兼容性 r21;
  enum 类型种类 r25;
  enum 类型种类 r26;
  enum 类型种类 r30;
  enum 类型种类 r31;
  enum 类型兼容性 r34;
  enum 类型种类 r38;
  enum 类型种类 r39;
  enum 类型种类 r43;
  enum 类型种类 r44;
  enum 类型兼容性 r47;
  enum 类型种类 r51;
  enum 类型种类 r52;
  enum 类型种类 r56;
  enum 类型种类 r57;
  enum 类型兼容性 r60;
  enum 类型种类 r64;
  enum 类型种类 r65;
  enum 类型种类 r69;
  enum 类型种类 r70;
  enum 类型兼容性 r73;
  enum 类型种类 r77;
  enum 类型种类 r78;
  enum 类型种类 r82;
  enum 类型种类 r83;
  enum 类型兼容性 r86;
  enum 类型种类 r90;
  enum 类型种类 r91;
  enum 类型种类 r95;
  enum 类型种类 r96;
  enum 类型兼容性 r101;
  enum 类型种类 r105;
  enum 类型种类 r106;
  enum 类型种类 r110;
  enum 类型种类 r111;
  enum 类型兼容性 r116;
  enum 类型兼容性 r118;

  entry:
  r0 = cn_var_源类型;
  r1 = cn_var_目标类型;
  类型相同(r0, r1);
  goto if_then_1879;

  if_then_1879:
  r2 = cn_var_类型兼容性;
  r3 = r2.完全兼容;
  return r3;
  goto if_merge_1880;

  if_merge_1880:
  r5 = cn_var_源类型;
  r6 = r5->种类;
  r7 = cn_var_类型种类;
  r8 = r7.类型_指针;
  r9 = r6 == r8;
  if (r9) goto logic_rhs_1883; else goto logic_merge_1884;

  if_then_1881:
  r15 = cn_var_源类型;
  r16 = r15->指向类型;
  r17 = r16->种类;
  r18 = cn_var_类型种类;
  r19 = r18.类型_空;
  r20 = r17 == r19;
  if (r20) goto if_then_1885; else goto if_merge_1886;

  if_merge_1882:
  r24 = cn_var_源类型;
  r25 = r24->种类;
  r26 = cn_var_类型种类;
  r27 = r26.类型_整数;
  r28 = r25 == r27;
  if (r28) goto logic_rhs_1889; else goto logic_merge_1890;

  logic_rhs_1883:
  r10 = cn_var_目标类型;
  r11 = r10->种类;
  r12 = cn_var_类型种类;
  r13 = r12.类型_指针;
  r14 = r11 == r13;
  goto logic_merge_1884;

  logic_merge_1884:
  if (r14) goto if_then_1881; else goto if_merge_1882;

  if_then_1885:
  r21 = cn_var_类型兼容性;
  r22 = r21.隐式转换;
  return r22;
  goto if_merge_1886;

  if_merge_1886:
  goto if_merge_1882;

  if_then_1887:
  r34 = cn_var_类型兼容性;
  r35 = r34.隐式转换;
  return r35;
  goto if_merge_1888;

  if_merge_1888:
  r37 = cn_var_源类型;
  r38 = r37->种类;
  r39 = cn_var_类型种类;
  r40 = r39.类型_整数;
  r41 = r38 == r40;
  if (r41) goto logic_rhs_1893; else goto logic_merge_1894;

  logic_rhs_1889:
  r29 = cn_var_目标类型;
  r30 = r29->种类;
  r31 = cn_var_类型种类;
  r32 = r31.类型_小数;
  r33 = r30 == r32;
  goto logic_merge_1890;

  logic_merge_1890:
  if (r33) goto if_then_1887; else goto if_merge_1888;

  if_then_1891:
  r47 = cn_var_类型兼容性;
  r48 = r47.隐式转换;
  return r48;
  goto if_merge_1892;

  if_merge_1892:
  r50 = cn_var_源类型;
  r51 = r50->种类;
  r52 = cn_var_类型种类;
  r53 = r52.类型_枚举;
  r54 = r51 == r53;
  if (r54) goto logic_rhs_1897; else goto logic_merge_1898;

  logic_rhs_1893:
  r42 = cn_var_目标类型;
  r43 = r42->种类;
  r44 = cn_var_类型种类;
  r45 = r44.类型_整数;
  r46 = r43 == r45;
  goto logic_merge_1894;

  logic_merge_1894:
  if (r46) goto if_then_1891; else goto if_merge_1892;

  if_then_1895:
  r60 = cn_var_类型兼容性;
  r61 = r60.隐式转换;
  return r61;
  goto if_merge_1896;

  if_merge_1896:
  r63 = cn_var_源类型;
  r64 = r63->种类;
  r65 = cn_var_类型种类;
  r66 = r65.类型_整数;
  r67 = r64 == r66;
  if (r67) goto logic_rhs_1901; else goto logic_merge_1902;

  logic_rhs_1897:
  r55 = cn_var_目标类型;
  r56 = r55->种类;
  r57 = cn_var_类型种类;
  r58 = r57.类型_整数;
  r59 = r56 == r58;
  goto logic_merge_1898;

  logic_merge_1898:
  if (r59) goto if_then_1895; else goto if_merge_1896;

  if_then_1899:
  r73 = cn_var_类型兼容性;
  r74 = r73.显式转换;
  return r74;
  goto if_merge_1900;

  if_merge_1900:
  r76 = cn_var_源类型;
  r77 = r76->种类;
  r78 = cn_var_类型种类;
  r79 = r78.类型_小数;
  r80 = r77 == r79;
  if (r80) goto logic_rhs_1905; else goto logic_merge_1906;

  logic_rhs_1901:
  r68 = cn_var_目标类型;
  r69 = r68->种类;
  r70 = cn_var_类型种类;
  r71 = r70.类型_枚举;
  r72 = r69 == r71;
  goto logic_merge_1902;

  logic_merge_1902:
  if (r72) goto if_then_1899; else goto if_merge_1900;

  if_then_1903:
  r86 = cn_var_类型兼容性;
  r87 = r86.显式转换;
  return r87;
  goto if_merge_1904;

  if_merge_1904:
  r89 = cn_var_源类型;
  r90 = r89->种类;
  r91 = cn_var_类型种类;
  r92 = r91.类型_类;
  r93 = r90 == r92;
  if (r93) goto logic_rhs_1909; else goto logic_merge_1910;

  logic_rhs_1905:
  r81 = cn_var_目标类型;
  r82 = r81->种类;
  r83 = cn_var_类型种类;
  r84 = r83.类型_整数;
  r85 = r82 == r84;
  goto logic_merge_1906;

  logic_merge_1906:
  if (r85) goto if_then_1903; else goto if_merge_1904;

  if_then_1907:
  r99 = cn_var_源类型;
  r100 = cn_var_目标类型;
  是派生类(r99, r100);
  goto if_then_1911;

  if_merge_1908:
  r104 = cn_var_源类型;
  r105 = r104->种类;
  r106 = cn_var_类型种类;
  r107 = r106.类型_类;
  r108 = r105 == r107;
  if (r108) goto logic_rhs_1915; else goto logic_merge_1916;

  logic_rhs_1909:
  r94 = cn_var_目标类型;
  r95 = r94->种类;
  r96 = cn_var_类型种类;
  r97 = r96.类型_类;
  r98 = r95 == r97;
  goto logic_merge_1910;

  logic_merge_1910:
  if (r98) goto if_then_1907; else goto if_merge_1908;

  if_then_1911:
  r101 = cn_var_类型兼容性;
  r102 = r101.隐式转换;
  return r102;
  goto if_merge_1912;

  if_merge_1912:
  goto if_merge_1908;

  if_then_1913:
  r114 = cn_var_源类型;
  r115 = cn_var_目标类型;
  实现接口(r114, r115);
  goto if_then_1917;

  if_merge_1914:
  r118 = cn_var_类型兼容性;
  r119 = r118.不兼容;
  return r119;

  logic_rhs_1915:
  r109 = cn_var_目标类型;
  r110 = r109->种类;
  r111 = cn_var_类型种类;
  r112 = r111.类型_接口;
  r113 = r110 == r112;
  goto logic_merge_1916;

  logic_merge_1916:
  if (r113) goto if_then_1913; else goto if_merge_1914;

  if_then_1917:
  r116 = cn_var_类型兼容性;
  r117 = r116.隐式转换;
  return r117;
  goto if_merge_1918;

  if_merge_1918:
  goto if_merge_1914;
  return 0;
}

_Bool 是派生类(struct 类型信息* cn_var_子类, struct 类型信息* cn_var_父类) {
  long long r0, r2, r4, r5, r9, r10, r14, r15, r18, r19, r20, r23, r24, r25;
  struct 类型信息* r1;
  struct 类型信息* r3;
  struct 类型信息* r6;
  struct 类型信息* r11;
  struct 类型信息* r16;
  struct 符号* r17;
  struct 类型信息* r21;
  struct 符号* r22;
  enum 类型种类 r7;
  enum 类型种类 r8;
  enum 类型种类 r12;
  enum 类型种类 r13;

  entry:
  r1 = cn_var_子类;
  r2 = r1 == 0;
  if (r2) goto logic_merge_1922; else goto logic_rhs_1921;

  if_then_1919:
  return 0;
  goto if_merge_1920;

  if_merge_1920:
  r6 = cn_var_子类;
  r7 = r6->种类;
  r8 = cn_var_类型种类;
  r9 = r8.类型_类;
  r10 = r7 != r9;
  if (r10) goto logic_merge_1926; else goto logic_rhs_1925;

  logic_rhs_1921:
  r3 = cn_var_父类;
  r4 = r3 == 0;
  goto logic_merge_1922;

  logic_merge_1922:
  if (r4) goto if_then_1919; else goto if_merge_1920;

  if_then_1923:
  return 0;
  goto if_merge_1924;

  if_merge_1924:
  void cn_var_当前_0;
  r16 = cn_var_子类;
  r17 = r16->类型符号;
  cn_var_当前_0 = r17;
  goto while_cond_1927;

  logic_rhs_1925:
  r11 = cn_var_父类;
  r12 = r11->种类;
  r13 = cn_var_类型种类;
  r14 = r13.类型_类;
  r15 = r12 != r14;
  goto logic_merge_1926;

  logic_merge_1926:
  if (r15) goto if_then_1923; else goto if_merge_1924;

  while_cond_1927:
  r18 = cn_var_当前_0;
  r19 = r18 != 0;
  if (r19) goto while_body_1928; else goto while_exit_1929;

  while_body_1928:
  r20 = cn_var_当前_0;
  r21 = cn_var_父类;
  r22 = r21->类型符号;
  r23 = r20 == r22;
  if (r23) goto if_then_1930; else goto if_merge_1931;

  while_exit_1929:
  return 0;

  if_then_1930:
  return 1;
  goto if_merge_1931;

  if_merge_1931:
  r24 = cn_var_当前_0;
  r25 = r24.父类符号;
  cn_var_当前_0 = r25;
  goto while_cond_1927;
  return 0;
}

_Bool 实现接口(struct 类型信息* cn_var_类类型, struct 类型信息* cn_var_接口类型) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31;

  entry:
  goto tail_rec_loop;
  return 0;
}

_Bool 是数值类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r6, r7, r11, r12;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r8;
  enum 类型种类 r4;
  enum 类型种类 r5;
  enum 类型种类 r9;
  enum 类型种类 r10;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1950; else goto if_merge_1951;

  if_then_1950:
  return 0;
  goto if_merge_1951;

  if_merge_1951:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = cn_var_类型种类;
  r6 = r5.类型_整数;
  r7 = r4 == r6;
  if (r7) goto logic_merge_1953; else goto logic_rhs_1952;

  logic_rhs_1952:
  r8 = cn_var_类型;
  r9 = r8->种类;
  r10 = cn_var_类型种类;
  r11 = r10.类型_小数;
  r12 = r9 == r11;
  goto logic_merge_1953;

  logic_merge_1953:
  return r12;
}

_Bool 是整数类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r6, r7, r11, r12;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r8;
  enum 类型种类 r4;
  enum 类型种类 r5;
  enum 类型种类 r9;
  enum 类型种类 r10;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1954; else goto if_merge_1955;

  if_then_1954:
  return 0;
  goto if_merge_1955;

  if_merge_1955:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = cn_var_类型种类;
  r6 = r5.类型_整数;
  r7 = r4 == r6;
  if (r7) goto logic_merge_1957; else goto logic_rhs_1956;

  logic_rhs_1956:
  r8 = cn_var_类型;
  r9 = r8->种类;
  r10 = cn_var_类型种类;
  r11 = r10.类型_枚举;
  r12 = r9 == r11;
  goto logic_merge_1957;

  logic_merge_1957:
  return r12;
}

_Bool 是指针类型(struct 类型信息* cn_var_类型) {
  long long r1, r5, r6;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;
  enum 类型种类 r4;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1958; else goto if_merge_1959;

  if_then_1958:
  return 0;
  goto if_merge_1959;

  if_merge_1959:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_指针;
  r6 = r3 == r5;
  return r6;
}

_Bool 是数组类型(struct 类型信息* cn_var_类型) {
  long long r1, r5, r6;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;
  enum 类型种类 r4;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1960; else goto if_merge_1961;

  if_then_1960:
  return 0;
  goto if_merge_1961;

  if_merge_1961:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_数组;
  r6 = r3 == r5;
  return r6;
}

_Bool 是结构体类型(struct 类型信息* cn_var_类型) {
  long long r1, r5, r6;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;
  enum 类型种类 r4;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1962; else goto if_merge_1963;

  if_then_1962:
  return 0;
  goto if_merge_1963;

  if_merge_1963:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_结构体;
  r6 = r3 == r5;
  return r6;
}

_Bool 是类类型(struct 类型信息* cn_var_类型) {
  long long r1, r5, r6;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;
  enum 类型种类 r4;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1964; else goto if_merge_1965;

  if_then_1964:
  return 0;
  goto if_merge_1965;

  if_merge_1965:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_类;
  r6 = r3 == r5;
  return r6;
}

_Bool 是函数类型(struct 类型信息* cn_var_类型) {
  long long r1, r5, r6;
  struct 类型信息* r0;
  struct 类型信息* r2;
  enum 类型种类 r3;
  enum 类型种类 r4;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1966; else goto if_merge_1967;

  if_then_1966:
  return 0;
  goto if_merge_1967;

  if_merge_1967:
  r2 = cn_var_类型;
  r3 = r2->种类;
  r4 = cn_var_类型种类;
  r5 = r4.类型_函数;
  r6 = r3 == r5;
  return r6;
}

_Bool 是可调用类型(struct 类型信息* cn_var_类型) {
  long long r1, r2, r6, r7, r8, r12, r13, r18, r19;
  struct 类型信息* r0;
  struct 类型信息* r3;
  struct 类型信息* r9;
  struct 类型信息* r14;
  struct 类型信息* r15;
  enum 类型种类 r4;
  enum 类型种类 r5;
  enum 类型种类 r10;
  enum 类型种类 r11;
  enum 类型种类 r16;
  enum 类型种类 r17;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 0;
  if (r1) goto if_then_1968; else goto if_merge_1969;

  if_then_1968:
  return 0;
  goto if_merge_1969;

  if_merge_1969:
  r3 = cn_var_类型;
  r4 = r3->种类;
  r5 = cn_var_类型种类;
  r6 = r5.类型_函数;
  r7 = r4 == r6;
  if (r7) goto logic_merge_1971; else goto logic_rhs_1970;

  logic_rhs_1970:
  r9 = cn_var_类型;
  r10 = r9->种类;
  r11 = cn_var_类型种类;
  r12 = r11.类型_指针;
  r13 = r10 == r12;
  if (r13) goto logic_rhs_1972; else goto logic_merge_1973;

  logic_merge_1971:
  return r19;

  logic_rhs_1972:
  r14 = cn_var_类型;
  r15 = r14->指向类型;
  r16 = r15->种类;
  r17 = cn_var_类型种类;
  r18 = r17.类型_函数;
  r19 = r16 == r18;
  goto logic_merge_1973;

  logic_merge_1973:
  goto logic_merge_1971;
  return 0;
}

struct 类型信息* 从类型节点推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r1, r5, r8, r11, r14, r17, r20, r23, r26;
  struct 类型节点* r0;
  struct 类型节点* r2;
  struct 类型节点* r28;
  struct 类型推断上下文* r29;
  struct 类型节点* r30;
  struct 类型推断上下文* r31;
  struct 类型节点* r32;
  struct 类型推断上下文* r33;
  struct 类型节点* r34;
  struct 类型推断上下文* r35;
  struct 类型节点* r36;
  struct 类型推断上下文* r37;
  struct 类型节点* r38;
  struct 类型推断上下文* r39;
  struct 类型节点* r40;
  struct 类型推断上下文* r41;
  struct 类型节点* r42;
  struct 类型推断上下文* r43;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r15;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r27;
  enum 节点类型 r3;
  enum 节点类型 r4;
  enum 节点类型 r7;
  enum 节点类型 r10;
  enum 节点类型 r13;
  enum 节点类型 r16;
  enum 节点类型 r19;
  enum 节点类型 r22;
  enum 节点类型 r25;

  entry:
  r0 = cn_var_类型节点指针;
  r1 = r0 == 0;
  if (r1) goto if_then_1974; else goto if_merge_1975;

  if_then_1974:
  return 0;
  goto if_merge_1975;

  if_merge_1975:
  r2 = cn_var_类型节点指针;
  r3 = r2->类型;
  r4 = cn_var_节点类型;
  r5 = r4.基础类型;
  r6 = r3 == r5;
  if (r6) goto case_body_1977; else goto switch_check_1986;

  switch_check_1986:
  r7 = cn_var_节点类型;
  r8 = r7.指针类型;
  r9 = r3 == r8;
  if (r9) goto case_body_1978; else goto switch_check_1987;

  switch_check_1987:
  r10 = cn_var_节点类型;
  r11 = r10.数组类型;
  r12 = r3 == r11;
  if (r12) goto case_body_1979; else goto switch_check_1988;

  switch_check_1988:
  r13 = cn_var_节点类型;
  r14 = r13.结构体类型;
  r15 = r3 == r14;
  if (r15) goto case_body_1980; else goto switch_check_1989;

  switch_check_1989:
  r16 = cn_var_节点类型;
  r17 = r16.枚举类型;
  r18 = r3 == r17;
  if (r18) goto case_body_1981; else goto switch_check_1990;

  switch_check_1990:
  r19 = cn_var_节点类型;
  r20 = r19.类类型;
  r21 = r3 == r20;
  if (r21) goto case_body_1982; else goto switch_check_1991;

  switch_check_1991:
  r22 = cn_var_节点类型;
  r23 = r22.接口类型;
  r24 = r3 == r23;
  if (r24) goto case_body_1983; else goto switch_check_1992;

  switch_check_1992:
  r25 = cn_var_节点类型;
  r26 = r25.函数类型;
  r27 = r3 == r26;
  if (r27) goto case_body_1984; else goto case_default_1985;

  case_body_1977:
  r28 = cn_var_类型节点指针;
  r29 = cn_var_上下文;
  从基础类型推断(r28, r29);
  return;
  goto switch_merge_1976;

  case_body_1978:
  r30 = cn_var_类型节点指针;
  r31 = cn_var_上下文;
  从指针类型推断(r30, r31);
  return;
  goto switch_merge_1976;

  case_body_1979:
  r32 = cn_var_类型节点指针;
  r33 = cn_var_上下文;
  从数组类型推断(r32, r33);
  return;
  goto switch_merge_1976;

  case_body_1980:
  r34 = cn_var_类型节点指针;
  r35 = cn_var_上下文;
  从结构体类型推断(r34, r35);
  return;
  goto switch_merge_1976;

  case_body_1981:
  r36 = cn_var_类型节点指针;
  r37 = cn_var_上下文;
  从枚举类型推断(r36, r37);
  return;
  goto switch_merge_1976;

  case_body_1982:
  r38 = cn_var_类型节点指针;
  r39 = cn_var_上下文;
  从类类型推断(r38, r39);
  return;
  goto switch_merge_1976;

  case_body_1983:
  r40 = cn_var_类型节点指针;
  r41 = cn_var_上下文;
  从接口类型推断(r40, r41);
  return;
  goto switch_merge_1976;

  case_body_1984:
  r42 = cn_var_类型节点指针;
  r43 = cn_var_上下文;
  从函数类型推断(r42, r43);
  return;
  goto switch_merge_1976;

  case_default_1985:
  return 0;
  goto switch_merge_1976;

  switch_merge_1976:
  return NULL;
}

struct 类型信息* 从基础类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r16, r17, r18, r19, r20, r22, r23, r24, r25, r26, r27, r29, r30, r31, r32, r33, r34, r36, r37, r38, r39, r40, r41, r43, r44, r45, r46;
  char* r1;
  struct 类型节点* r0;
  struct 类型推断上下文* r14;
  struct 符号表管理器* r15;
  enum 符号类型 r21;
  enum 符号类型 r28;
  enum 符号类型 r35;
  enum 符号类型 r42;

  entry:
  void cn_var_名称_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->名称;
  cn_var_名称_0 = r1;
  r2 = cn_var_名称_0;
  比较字符串(r2, "整数");
  r3 = /* NONE */ == 0;
  if (r3) goto if_then_1993; else goto if_merge_1994;

  if_then_1993:
  r4 = cn_var_整数大小;
  创建整数类型("整数", r4, 1);
  return;
  goto if_merge_1994;

  if_merge_1994:
  r5 = cn_var_名称_0;
  比较字符串(r5, "小数");
  r6 = /* NONE */ == 0;
  if (r6) goto if_then_1995; else goto if_merge_1996;

  if_then_1995:
  r7 = cn_var_小数大小;
  创建小数类型("小数", r7);
  return;
  goto if_merge_1996;

  if_merge_1996:
  r8 = cn_var_名称_0;
  比较字符串(r8, "布尔");
  r9 = /* NONE */ == 0;
  if (r9) goto if_then_1997; else goto if_merge_1998;

  if_then_1997:
  创建布尔类型();
  return;
  goto if_merge_1998;

  if_merge_1998:
  r10 = cn_var_名称_0;
  比较字符串(r10, "字符串");
  r11 = /* NONE */ == 0;
  if (r11) goto if_then_1999; else goto if_merge_2000;

  if_then_1999:
  创建字符串类型();
  return;
  goto if_merge_2000;

  if_merge_2000:
  r12 = cn_var_名称_0;
  比较字符串(r12, "空类型");
  r13 = /* NONE */ == 0;
  if (r13) goto if_then_2001; else goto if_merge_2002;

  if_then_2001:
  创建空类型();
  return;
  goto if_merge_2002;

  if_merge_2002:
  void cn_var_符号指针_1;
  r14 = cn_var_上下文;
  r15 = r14->符号表;
  r16 = cn_var_名称_0;
  查找符号(r15, r16);
  cn_var_符号指针_1 = /* NONE */;
  r17 = cn_var_符号指针_1;
  r18 = r17 != 0;
  if (r18) goto if_then_2003; else goto if_merge_2004;

  if_then_2003:
  r19 = cn_var_符号指针_1;
  r20 = r19.种类;
  r21 = cn_var_符号类型;
  r22 = r21.结构体符号;
  r23 = r20 == r22;
  if (r23) goto if_then_2005; else goto if_merge_2006;

  if_merge_2004:
  return 0;

  if_then_2005:
  r24 = cn_var_名称_0;
  r25 = cn_var_符号指针_1;
  创建结构体类型(r24, r25);
  return;
  goto if_merge_2006;

  if_merge_2006:
  r26 = cn_var_符号指针_1;
  r27 = r26.种类;
  r28 = cn_var_符号类型;
  r29 = r28.枚举符号;
  r30 = r27 == r29;
  if (r30) goto if_then_2007; else goto if_merge_2008;

  if_then_2007:
  r31 = cn_var_名称_0;
  r32 = cn_var_符号指针_1;
  创建枚举类型(r31, r32);
  return;
  goto if_merge_2008;

  if_merge_2008:
  r33 = cn_var_符号指针_1;
  r34 = r33.种类;
  r35 = cn_var_符号类型;
  r36 = r35.类符号;
  r37 = r34 == r36;
  if (r37) goto if_then_2009; else goto if_merge_2010;

  if_then_2009:
  r38 = cn_var_名称_0;
  r39 = cn_var_符号指针_1;
  创建类类型(r38, r39);
  return;
  goto if_merge_2010;

  if_merge_2010:
  r40 = cn_var_符号指针_1;
  r41 = r40.种类;
  r42 = cn_var_符号类型;
  r43 = r42.接口符号;
  r44 = r41 == r43;
  if (r44) goto if_then_2011; else goto if_merge_2012;

  if_then_2011:
  r45 = cn_var_名称_0;
  r46 = cn_var_符号指针_1;
  创建接口类型(r45, r46);
  return;
  goto if_merge_2012;

  if_merge_2012:
  goto if_merge_2004;
  return NULL;
}

struct 类型信息* 从指针类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3, r4, r5, r6, r8, r9, r10, r11, r12, r13;
  struct 类型节点* r0;
  struct 类型节点* r1;
  struct 类型推断上下文* r2;
  struct 类型节点* r7;

  entry:
  void cn_var_指向类型_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->元素类型;
  r2 = cn_var_上下文;
  从类型节点推断(r1, r2);
  cn_var_指向类型_0 = /* NONE */;
  r3 = cn_var_指向类型_0;
  r4 = r3 == 0;
  if (r4) goto if_then_2013; else goto if_merge_2014;

  if_then_2013:
  return 0;
  goto if_merge_2014;

  if_merge_2014:
  void cn_var_结果_1;
  r5 = cn_var_指向类型_0;
  创建指针类型(r5);
  cn_var_结果_1 = /* NONE */;
  long long cn_var_i_2;
  cn_var_i_2 = 1;
  goto for_cond_2015;

  for_cond_2015:
  r6 = cn_var_i_2;
  r7 = cn_var_类型节点指针;
  r8 = r7->指针层级;
  r9 = r6 < r8;
  if (r9) goto for_body_2016; else goto for_exit_2018;

  for_body_2016:
  r10 = cn_var_结果_1;
  创建指针类型(r10);
  cn_var_结果_1 = /* NONE */;
  goto for_update_2017;

  for_update_2017:
  r11 = cn_var_i_2;
  r12 = r11 + 1;
  cn_var_i_2 = r12;
  goto for_cond_2015;

  for_exit_2018:
  r13 = cn_var_结果_1;
  return r13;
}

struct 类型信息* 从数组类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r3, r4, r6, r7, r8, r10, r12, r15;
  struct 类型节点* r0;
  struct 类型节点* r1;
  struct 类型推断上下文* r2;
  struct 类型节点* r5;
  struct 类型节点* r9;
  long long* r11;
  long long* r13;
  struct 类型节点* r14;

  entry:
  void cn_var_元素类型_0;
  r0 = cn_var_类型节点指针;
  r1 = r0->元素类型;
  r2 = cn_var_上下文;
  从类型节点推断(r1, r2);
  cn_var_元素类型_0 = /* NONE */;
  r3 = cn_var_元素类型_0;
  r4 = r3 == 0;
  if (r4) goto if_then_2019; else goto if_merge_2020;

  if_then_2019:
  return 0;
  goto if_merge_2020;

  if_merge_2020:
  long long* cn_var_维度大小_1;
  r5 = cn_var_类型节点指针;
  r6 = r5->数组维度;
  r7 = cn_var_整数大小;
  r8 = r6 * r7;
  分配内存(r8);
  cn_var_维度大小_1 = /* NONE */;
  r9 = cn_var_类型节点指针;
  r10 = r9->数组大小;
  r11 = cn_var_维度大小_1;
    { long long _tmp_r5 = r10; cn_rt_array_set_element(r11, 0, &_tmp_r5, 8); }
  r12 = cn_var_元素类型_0;
  r13 = cn_var_维度大小_1;
  r14 = cn_var_类型节点指针;
  r15 = r14->数组维度;
  创建数组类型(r12, r13, r15);
  return;
}

struct 类型信息* 从结构体类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r6, r7, r8, r10, r11, r14;
  char* r3;
  char* r13;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 类型节点* r12;
  enum 符号类型 r9;

  entry:
  void cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  查找符号(r1, r3);
  cn_var_符号指针_0 = /* NONE */;
  r5 = cn_var_符号指针_0;
  r6 = r5 == 0;
  if (r6) goto logic_merge_2024; else goto logic_rhs_2023;

  if_then_2021:
  return 0;
  goto if_merge_2022;

  if_merge_2022:
  r12 = cn_var_类型节点指针;
  r13 = r12->名称;
  r14 = cn_var_符号指针_0;
  创建结构体类型(r13, r14);
  return;

  logic_rhs_2023:
  r7 = cn_var_符号指针_0;
  r8 = r7.种类;
  r9 = cn_var_符号类型;
  r10 = r9.结构体符号;
  r11 = r8 != r10;
  goto logic_merge_2024;

  logic_merge_2024:
  if (r11) goto if_then_2021; else goto if_merge_2022;
  return NULL;
}

struct 类型信息* 从枚举类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r6, r7, r8, r10, r11, r14;
  char* r3;
  char* r13;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 类型节点* r12;
  enum 符号类型 r9;

  entry:
  void cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  查找符号(r1, r3);
  cn_var_符号指针_0 = /* NONE */;
  r5 = cn_var_符号指针_0;
  r6 = r5 == 0;
  if (r6) goto logic_merge_2028; else goto logic_rhs_2027;

  if_then_2025:
  return 0;
  goto if_merge_2026;

  if_merge_2026:
  r12 = cn_var_类型节点指针;
  r13 = r12->名称;
  r14 = cn_var_符号指针_0;
  创建枚举类型(r13, r14);
  return;

  logic_rhs_2027:
  r7 = cn_var_符号指针_0;
  r8 = r7.种类;
  r9 = cn_var_符号类型;
  r10 = r9.枚举符号;
  r11 = r8 != r10;
  goto logic_merge_2028;

  logic_merge_2028:
  if (r11) goto if_then_2025; else goto if_merge_2026;
  return NULL;
}

struct 类型信息* 从类类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r6, r7, r8, r10, r11, r14;
  char* r3;
  char* r13;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 类型节点* r12;
  enum 符号类型 r9;

  entry:
  void cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  查找符号(r1, r3);
  cn_var_符号指针_0 = /* NONE */;
  r5 = cn_var_符号指针_0;
  r6 = r5 == 0;
  if (r6) goto logic_merge_2032; else goto logic_rhs_2031;

  if_then_2029:
  return 0;
  goto if_merge_2030;

  if_merge_2030:
  r12 = cn_var_类型节点指针;
  r13 = r12->名称;
  r14 = cn_var_符号指针_0;
  创建类类型(r13, r14);
  return;

  logic_rhs_2031:
  r7 = cn_var_符号指针_0;
  r8 = r7.种类;
  r9 = cn_var_符号类型;
  r10 = r9.类符号;
  r11 = r8 != r10;
  goto logic_merge_2032;

  logic_merge_2032:
  if (r11) goto if_then_2029; else goto if_merge_2030;
  return NULL;
}

struct 类型信息* 从接口类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {
  long long r4, r5, r6, r7, r8, r10, r11, r14;
  char* r3;
  char* r13;
  struct 类型推断上下文* r0;
  struct 符号表管理器* r1;
  struct 类型节点* r2;
  struct 类型节点* r12;
  enum 符号类型 r9;

  entry:
  void cn_var_符号指针_0;
  r0 = cn_var_上下文;
  r1 = r0->符号表;
  r2 = cn_var_类型节点指针;
  r3 = r2->名称;
  查找符号(r1, r3);
  cn_var_符号指针_0 = /* NONE */;
  r5 = cn_var_符号指针_0;
  r6 = r5 == 0;
  if (r6) goto logic_merge_2036; else goto logic_rhs_2035;

  if_then_2033:
  return 0;
  goto if_merge_2034;

  if_merge_2034:
  r12 = cn_var_类型节点指针;
  r13 = r12->名称;
  r14 = cn_var_符号指针_0;
  创建接口类型(r13, r14);
  return;

  logic_rhs_2035:
  r7 = cn_var_符号指针_0;
  r8 = r7.种类;
  r9 = cn_var_符号类型;
  r10 = r9.接口符号;
  r11 = r8 != r10;
  goto logic_merge_2036;

  logic_merge_2036:
  if (r11) goto if_then_2033; else goto if_merge_2034;
  return NULL;
}

struct 类型信息* 从函数类型推断(struct 类型节点* cn_var_类型节点指针, struct 类型推断上下文* cn_var_上下文) {

  entry:
  return 0;
}

char* 获取类型种类名称(enum 类型种类 cn_var_种类) {
  long long r2, r5, r8, r11, r14, r17, r20, r23, r26, r29, r32, r35, r38, r41, r44;
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
  _Bool r39;
  _Bool r42;
  _Bool r45;
  enum 类型种类 r0;
  enum 类型种类 r1;
  enum 类型种类 r4;
  enum 类型种类 r7;
  enum 类型种类 r10;
  enum 类型种类 r13;
  enum 类型种类 r16;
  enum 类型种类 r19;
  enum 类型种类 r22;
  enum 类型种类 r25;
  enum 类型种类 r28;
  enum 类型种类 r31;
  enum 类型种类 r34;
  enum 类型种类 r37;
  enum 类型种类 r40;
  enum 类型种类 r43;

  entry:
  r0 = cn_var_种类;
  r1 = cn_var_类型种类;
  r2 = r1.类型_空;
  r3 = r0 == r2;
  if (r3) goto case_body_2038; else goto switch_check_2054;

  switch_check_2054:
  r4 = cn_var_类型种类;
  r5 = r4.类型_整数;
  r6 = r0 == r5;
  if (r6) goto case_body_2039; else goto switch_check_2055;

  switch_check_2055:
  r7 = cn_var_类型种类;
  r8 = r7.类型_小数;
  r9 = r0 == r8;
  if (r9) goto case_body_2040; else goto switch_check_2056;

  switch_check_2056:
  r10 = cn_var_类型种类;
  r11 = r10.类型_布尔;
  r12 = r0 == r11;
  if (r12) goto case_body_2041; else goto switch_check_2057;

  switch_check_2057:
  r13 = cn_var_类型种类;
  r14 = r13.类型_字符串;
  r15 = r0 == r14;
  if (r15) goto case_body_2042; else goto switch_check_2058;

  switch_check_2058:
  r16 = cn_var_类型种类;
  r17 = r16.类型_字符;
  r18 = r0 == r17;
  if (r18) goto case_body_2043; else goto switch_check_2059;

  switch_check_2059:
  r19 = cn_var_类型种类;
  r20 = r19.类型_指针;
  r21 = r0 == r20;
  if (r21) goto case_body_2044; else goto switch_check_2060;

  switch_check_2060:
  r22 = cn_var_类型种类;
  r23 = r22.类型_数组;
  r24 = r0 == r23;
  if (r24) goto case_body_2045; else goto switch_check_2061;

  switch_check_2061:
  r25 = cn_var_类型种类;
  r26 = r25.类型_结构体;
  r27 = r0 == r26;
  if (r27) goto case_body_2046; else goto switch_check_2062;

  switch_check_2062:
  r28 = cn_var_类型种类;
  r29 = r28.类型_枚举;
  r30 = r0 == r29;
  if (r30) goto case_body_2047; else goto switch_check_2063;

  switch_check_2063:
  r31 = cn_var_类型种类;
  r32 = r31.类型_类;
  r33 = r0 == r32;
  if (r33) goto case_body_2048; else goto switch_check_2064;

  switch_check_2064:
  r34 = cn_var_类型种类;
  r35 = r34.类型_接口;
  r36 = r0 == r35;
  if (r36) goto case_body_2049; else goto switch_check_2065;

  switch_check_2065:
  r37 = cn_var_类型种类;
  r38 = r37.类型_函数;
  r39 = r0 == r38;
  if (r39) goto case_body_2050; else goto switch_check_2066;

  switch_check_2066:
  r40 = cn_var_类型种类;
  r41 = r40.类型_参数;
  r42 = r0 == r41;
  if (r42) goto case_body_2051; else goto switch_check_2067;

  switch_check_2067:
  r43 = cn_var_类型种类;
  r44 = r43.类型_未定义;
  r45 = r0 == r44;
  if (r45) goto case_body_2052; else goto case_default_2053;

  case_body_2038:
  return "空类型";
  goto switch_merge_2037;

  case_body_2039:
  return "整数";
  goto switch_merge_2037;

  case_body_2040:
  return "小数";
  goto switch_merge_2037;

  case_body_2041:
  return "布尔";
  goto switch_merge_2037;

  case_body_2042:
  return "字符串";
  goto switch_merge_2037;

  case_body_2043:
  return "字符";
  goto switch_merge_2037;

  case_body_2044:
  return "指针";
  goto switch_merge_2037;

  case_body_2045:
  return "数组";
  goto switch_merge_2037;

  case_body_2046:
  return "结构体";
  goto switch_merge_2037;

  case_body_2047:
  return "枚举";
  goto switch_merge_2037;

  case_body_2048:
  return "类";
  goto switch_merge_2037;

  case_body_2049:
  return "接口";
  goto switch_merge_2037;

  case_body_2050:
  return "函数";
  goto switch_merge_2037;

  case_body_2051:
  return "类型参数";
  goto switch_merge_2037;

  case_body_2052:
  return "未定义";
  goto switch_merge_2037;

  case_default_2053:
  return "未知";
  goto switch_merge_2037;

  switch_merge_2037:
  return NULL;
}

