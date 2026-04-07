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
struct 扫描器;
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};
struct 诊断集合;
struct 语法错误恢复上下文 {
    long long 连续错误计数;
    long long 最大连续错误;
    long long 最后错误行号;
    long long 级联抑制计数;
    _Bool 抑制级联错误;
    long long 恢复尝试次数;
    long long 最大恢复次数;
};
struct 解析器;
struct 解析器 {
    struct 词元 当前词元;
    struct 词元 下一个词元;
    struct 扫描器* 扫描器实例;
    struct 诊断集合* 诊断集合指针;
    long long 错误计数;
    long long 恢复计数;
    struct 语法错误恢复上下文 恢复上下文;
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
enum 模块类型 {
    模块类型_文件模块,
    模块类型_包模块,
    模块类型_内联模块,
    模块类型_内建模块
};

enum 模块加载状态 {
    模块加载状态_未加载,
    模块加载状态_加载中,
    模块加载状态_已加载,
    模块加载状态_加载失败
};

// CN Language Global Struct Forward Declarations
struct 模块标识符;
struct 导出符号;
struct 模块依赖;
struct 模块元数据;
struct 模块缓存条目;
struct 模块加载器;

// CN Language Global Struct Definitions
struct 模块标识符 {
    char* 完全限定名;
    long long 完全限定名长度;
    char** 路径段列表;
    long long 路径段数量;
};

struct 导出符号 {
    char* 名称;
    long long 名称长度;
    _Bool 是函数;
    _Bool 是常量;
    void* 符号信息;
};

struct 模块依赖 {
    struct 模块标识符* 模块标识;
    _Bool 是必需;
    _Bool 是循环;
};

struct 模块元数据 {
    struct 模块标识符* 标识符;
    enum 模块类型 类型;
    enum 模块加载状态 状态;
    char* 文件路径;
    long long 文件路径长度;
    struct 导出符号* 导出符号表;
    long long 导出符号数量;
    long long 导出符号容量;
    struct 模块依赖* 依赖列表;
    long long 依赖数量;
    long long 依赖容量;
    struct 程序节点* 程序节点;
    struct 符号表管理器* 符号表;
};

struct 模块缓存条目 {
    char* 模块名;
    struct 模块元数据* 元数据;
    struct 模块缓存条目* 下一个;
};

struct 模块加载器 {
    char** 搜索路径列表;
    long long 搜索路径数量;
    struct 模块缓存条目** 缓存列表;
    long long 缓存数量;
    struct 诊断集合* 诊断集合;
    struct 模块元数据* 当前模块;
    long long 当前依赖深度;
    long long 加载成功数;
    long long 加载失败数;
};

// Global Variables
long long cn_var_最大模块路径长度 = 1024;
long long cn_var_最大搜索路径数量 = 32;
long long cn_var_最大模块缓存数量 = 256;
long long cn_var_最大依赖深度 = 16;

// Forward Declarations
struct 模块加载器* 创建模块加载器(struct 诊断集合*);
void 销毁模块加载器(struct 模块加载器*);
_Bool 添加搜索路径(struct 模块加载器*, char*);
void 设置默认搜索路径(struct 模块加载器*, char*);
struct 模块标识符* 解析模块标识符(char*);
void 销毁模块标识符(struct 模块标识符*);
_Bool 解析模块文件路径(struct 模块加载器*, struct 模块标识符*, char*, long long);
long long 计算哈希值(char*);
struct 模块元数据* 从缓存获取模块(struct 模块加载器*, char*);
_Bool 添加模块到缓存(struct 模块加载器*, char*, struct 模块元数据*);
_Bool 读取文件内容(char*, char**, long long*);
struct 模块元数据* 创建模块元数据(struct 模块标识符*);
void 销毁模块元数据(struct 模块元数据*);
struct 模块元数据* 加载模块核心(struct 模块加载器*, char*);
struct 模块元数据* 加载模块(struct 模块加载器*, char*);
_Bool 处理导入语句(struct 模块加载器*, struct 导入语句节点*, struct 模块元数据*);
_Bool 处理所有导入语句(struct 模块加载器*, struct 模块元数据*);
_Bool 收集导出符号(struct 模块元数据*);
long long 模块加载器大小();
long long 模块标识符大小();
long long 模块元数据大小();
long long 模块缓存条目大小();
long long 模块依赖大小();
long long 导出符号大小();
long long 字符串指针大小();
long long 获取文件大小(void*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置*, char*);

struct 模块加载器* 创建模块加载器(struct 诊断集合* cn_var_诊断集合指针) {
  long long r1;
  struct 诊断集合* r2;

  entry:
  struct 模块加载器* cn_var_加载器_0;
  模块加载器大小();
  分配清零内存(1, /* NONE */);
  cn_var_加载器_0 = /* NONE */;
  r0 = cn_var_加载器_0;
  r1 = r0 == 0;
  if (r1) goto if_then_3518; else goto if_merge_3519;

  if_then_3518:
  return 0;
  goto if_merge_3519;

  if_merge_3519:
  r2 = cn_var_诊断集合指针;
  r3 = cn_var_加载器_0;
  return r3;
}

void 销毁模块加载器(struct 模块加载器* cn_var_加载器) {
  long long r1, r2, r4, r5, r7, r8, r9, r10, r12, r13, r14, r15, r16, r17, r18, r19, r21, r22, r23, r25, r32, r33;
  char* r29;
  struct 模块缓存条目* r24;
  struct 模块缓存条目* r26;
  struct 模块缓存条目* r27;
  struct 模块缓存条目* r28;
  struct 模块缓存条目* r30;
  struct 模块缓存条目* r31;

  entry:
  r0 = cn_var_加载器;
  r1 = r0 == 0;
  if (r1) goto if_then_3520; else goto if_merge_3521;

  if_then_3520:
  return;
  goto if_merge_3521;

  if_merge_3521:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3522;

  for_cond_3522:
  r2 = cn_var_i_0;
  r3 = cn_var_加载器;
  r4 = r3.搜索路径数量;
  r5 = r2 < r4;
  if (r5) goto for_body_3523; else goto for_exit_3525;

  for_body_3523:
  r6 = cn_var_加载器;
  r7 = r6.搜索路径列表;
  r8 = cn_var_i_0;
  r9 = *(void*)cn_rt_array_get_element(r7, r8, 8);
  r10 = r9 != 0;
  if (r10) goto if_then_3526; else goto if_merge_3527;

  for_update_3524:
  r15 = cn_var_i_0;
  r16 = r15 + 1;
  cn_var_i_0 = r16;
  goto for_cond_3522;

  for_exit_3525:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_3528;

  if_then_3526:
  r11 = cn_var_加载器;
  r12 = r11.搜索路径列表;
  r13 = cn_var_i_0;
  r14 = *(void*)cn_rt_array_get_element(r12, r13, 8);
  释放内存(r14);
  goto if_merge_3527;

  if_merge_3527:
  goto for_update_3524;

  for_cond_3528:
  r17 = cn_var_i_1;
  r18 = cn_var_最大模块缓存数量;
  r19 = r17 < r18;
  if (r19) goto for_body_3529; else goto for_exit_3531;

  for_body_3529:
  struct 模块缓存条目* cn_var_条目_2;
  r20 = cn_var_加载器;
  r21 = r20.缓存列表;
  r22 = cn_var_i_1;
  r23 = *(void*)cn_rt_array_get_element(r21, r22, 8);
  cn_var_条目_2 = r23;
  goto while_cond_3532;

  for_update_3530:
  r32 = cn_var_i_1;
  r33 = r32 + 1;
  cn_var_i_1 = r33;
  goto for_cond_3528;

  for_exit_3531:
  r34 = cn_var_加载器;
  释放内存(r34);

  while_cond_3532:
  r24 = cn_var_条目_2;
  r25 = r24 != 0;
  if (r25) goto while_body_3533; else goto while_exit_3534;

  while_body_3533:
  struct 模块缓存条目* cn_var_下一个_3;
  r26 = cn_var_条目_2;
  r27 = r26->下一个;
  cn_var_下一个_3 = r27;
  r28 = cn_var_条目_2;
  r29 = r28->模块名;
  释放内存(r29);
  r30 = cn_var_条目_2;
  释放内存(r30);
  r31 = cn_var_下一个_3;
  cn_var_条目_2 = r31;
  goto while_cond_3532;

  while_exit_3534:
  goto for_update_3530;
  return;
}

_Bool 添加搜索路径(struct 模块加载器* cn_var_加载器, char* cn_var_路径) {
  long long r0, r2, r4, r6, r7, r8, r10, r11, r13, r18, r20, r22, r23;
  char* r3;
  char* r9;
  char* r12;
  char* r14;
  char* r15;
  char* r16;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3538; else goto logic_rhs_3537;

  if_then_3535:
  return 0;
  goto if_merge_3536;

  if_merge_3536:
  r5 = cn_var_加载器;
  r6 = r5.搜索路径数量;
  r7 = cn_var_最大搜索路径数量;
  r8 = r6 >= r7;
  if (r8) goto if_then_3539; else goto if_merge_3540;

  logic_rhs_3537:
  r3 = cn_var_路径;
  r4 = r3 == 0;
  goto logic_merge_3538;

  logic_merge_3538:
  if (r4) goto if_then_3535; else goto if_merge_3536;

  if_then_3539:
  return 0;
  goto if_merge_3540;

  if_merge_3540:
  long long cn_var_路径长度_0;
  r9 = cn_var_路径;
  获取字符串长度(r9);
  cn_var_路径长度_0 = /* NONE */;
  char* cn_var_路径副本_1;
  r10 = cn_var_路径长度_0;
  r11 = r10 + 1;
  分配内存(r11);
  cn_var_路径副本_1 = /* NONE */;
  r12 = cn_var_路径副本_1;
  r13 = r12 == 0;
  if (r13) goto if_then_3541; else goto if_merge_3542;

  if_then_3541:
  return 0;
  goto if_merge_3542;

  if_merge_3542:
  r14 = cn_var_路径副本_1;
  r15 = cn_var_路径;
  复制字符串(r14, r15);
  r16 = cn_var_路径副本_1;
  r17 = cn_var_加载器;
  r18 = r17.搜索路径列表;
  r19 = cn_var_加载器;
  r20 = r19.搜索路径数量;
    { long long _tmp_r15 = r16; cn_rt_array_set_element(r18, r20, &_tmp_r15, 8); }
  r21 = cn_var_加载器;
  r22 = r21.搜索路径数量;
  r23 = r22 + 1;
  return 1;
}

void 设置默认搜索路径(struct 模块加载器* cn_var_加载器, char* cn_var_源文件目录) {
  long long r1;
  char* r3;

  entry:
  r0 = cn_var_加载器;
  r1 = r0 == 0;
  if (r1) goto if_then_3543; else goto if_merge_3544;

  if_then_3543:
  return;
  goto if_merge_3544;

  if_merge_3544:
  r2 = cn_var_加载器;
  r3 = cn_var_源文件目录;
  添加搜索路径(r2, r3);
  r4 = cn_var_加载器;
  添加搜索路径(r4, ".");
  r5 = cn_var_加载器;
  添加搜索路径(r5, "标准库");
  r6 = cn_var_加载器;
  添加搜索路径(r6, "stdlib");
  return;
}

struct 模块标识符* 解析模块标识符(char* cn_var_模块名) {
  long long r1, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r16, r17, r18, r19, r21, r22, r23, r24, r25, r26, r27, r28, r29, r30, r31, r32, r33, r34, r35, r36, r38, r39, r40, r42, r43, r44, r46, r48, r49, r51, r52, r53, r54, r55, r56, r57, r58, r59, r60, r61, r65, r66, r67, r68, r70, r71, r73, r74, r75, r76, r77;
  char* r0;
  char* r2;
  char* r15;
  char* r20;
  char* r37;
  char* r41;
  char* r50;
  char* r62;
  char* r64;
  char* r69;
  void* r45;
  void* r47;
  void* r63;
  void* r72;

  entry:
  r0 = cn_var_模块名;
  r1 = r0 == 0;
  if (r1) goto if_then_3545; else goto if_merge_3546;

  if_then_3545:
  return 0;
  goto if_merge_3546;

  if_merge_3546:
  long long cn_var_名称长度_0;
  r2 = cn_var_模块名;
  获取字符串长度(r2);
  cn_var_名称长度_0 = /* NONE */;
  r4 = cn_var_名称长度_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_3550; else goto logic_rhs_3549;

  if_then_3547:
  return 0;
  goto if_merge_3548;

  if_merge_3548:
  struct 模块标识符* cn_var_标识符_1;
  模块标识符大小();
  分配清零内存(1, /* NONE */);
  cn_var_标识符_1 = /* NONE */;
  r9 = 0;
  if (r9) goto if_then_3551; else goto if_merge_3552;

  logic_rhs_3549:
  r6 = cn_var_名称长度_0;
  r7 = cn_var_最大模块路径长度;
  r8 = r6 >= r7;
  goto logic_merge_3550;

  logic_merge_3550:
  if (r8) goto if_then_3547; else goto if_merge_3548;

  if_then_3551:
  return 0;
  goto if_merge_3552;

  if_merge_3552:
  r10 = cn_var_名称长度_0;
  r11 = r10 + 1;
  分配内存(r11);
  r12 = 40.完全限定名;
  r13 = r12 == 0;
  if (r13) goto if_then_3553; else goto if_merge_3554;

  if_then_3553:
  释放内存(40);
  return 0;
  goto if_merge_3554;

  if_merge_3554:
  r14 = 40.完全限定名;
  r15 = cn_var_模块名;
  复制字符串(r14, r15);
  r16 = cn_var_名称长度_0;
  long long cn_var_段数量_2;
  cn_var_段数量_2 = 1;
  long long cn_var_i_3;
  cn_var_i_3 = 0;
  goto for_cond_3555;

  for_cond_3555:
  r17 = cn_var_i_3;
  r18 = cn_var_名称长度_0;
  r19 = r17 < r18;
  if (r19) goto for_body_3556; else goto for_exit_3558;

  for_body_3556:
  r20 = cn_var_模块名;
  r21 = cn_var_i_3;
  r22 = *(void*)cn_rt_array_get_element(r20, r21, 8);
  r23 = r22 == 46;
  if (r23) goto if_then_3559; else goto if_merge_3560;

  for_update_3557:
  r26 = cn_var_i_3;
  r27 = r26 + 1;
  cn_var_i_3 = r27;
  goto for_cond_3555;

  for_exit_3558:
  r28 = cn_var_段数量_2;
  字符串指针大小();
  分配清零内存(r28, /* NONE */);
  r29 = 40.路径段列表;
  r30 = r29 == 0;
  if (r30) goto if_then_3561; else goto if_merge_3562;

  if_then_3559:
  r24 = cn_var_段数量_2;
  r25 = r24 + 1;
  cn_var_段数量_2 = r25;
  goto if_merge_3560;

  if_merge_3560:
  goto for_update_3557;

  if_then_3561:
  r31 = 40.完全限定名;
  释放内存(r31);
  释放内存(40);
  return 0;
  goto if_merge_3562;

  if_merge_3562:
  r32 = cn_var_段数量_2;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 1024);
  long long cn_var_缓冲区位置_4;
  cn_var_缓冲区位置_4 = 0;
  long long cn_var_当前段索引_5;
  cn_var_当前段索引_5 = 0;
  long long cn_var_i_6;
  cn_var_i_6 = 0;
  goto for_cond_3563;

  for_cond_3563:
  r33 = cn_var_i_6;
  r34 = cn_var_名称长度_0;
  r35 = r33 <= r34;
  if (r35) goto for_body_3564; else goto for_exit_3566;

  for_body_3564:
  r37 = cn_var_模块名;
  r38 = cn_var_i_6;
  r39 = *(void*)cn_rt_array_get_element(r37, r38, 8);
  r40 = r39 == 46;
  if (r40) goto logic_merge_3571; else goto logic_rhs_3570;

  for_update_3565:
  r76 = cn_var_i_6;
  r77 = r76 + 1;
  cn_var_i_6 = r77;
  goto for_cond_3563;

  for_exit_3566:
  return 40;

  if_then_3567:
  r45 = cn_var_缓冲区;
  r46 = cn_var_缓冲区位置_4;
    { long long _tmp_i11 = 0; cn_rt_array_set_element(r45, r46, &_tmp_i11, 8); }
  long long cn_var_段长度_7;
  r47 = cn_var_缓冲区;
  获取字符串长度(r47);
  cn_var_段长度_7 = /* NONE */;
  char* cn_var_段副本_8;
  r48 = cn_var_段长度_7;
  r49 = r48 + 1;
  分配内存(r49);
  cn_var_段副本_8 = /* NONE */;
  r50 = cn_var_段副本_8;
  r51 = r50 == 0;
  if (r51) goto if_then_3572; else goto if_merge_3573;

  if_else_3568:
  r69 = cn_var_模块名;
  r70 = cn_var_i_6;
  r71 = *(void*)cn_rt_array_get_element(r69, r70, 8);
  r72 = cn_var_缓冲区;
  r73 = cn_var_缓冲区位置_4;
    { long long _tmp_r16 = r71; cn_rt_array_set_element(r72, r73, &_tmp_r16, 8); }
  r74 = cn_var_缓冲区位置_4;
  r75 = r74 + 1;
  cn_var_缓冲区位置_4 = r75;
  goto if_merge_3569;

  if_merge_3569:
  goto for_update_3565;

  logic_rhs_3570:
  r41 = cn_var_模块名;
  r42 = cn_var_i_6;
  r43 = *(void*)cn_rt_array_get_element(r41, r42, 8);
  r44 = r43 == 0;
  goto logic_merge_3571;

  logic_merge_3571:
  if (r44) goto if_then_3567; else goto if_else_3568;

  if_then_3572:
  long long cn_var_j_9;
  cn_var_j_9 = 0;
  goto for_cond_3574;

  if_merge_3573:
  r62 = cn_var_段副本_8;
  r63 = cn_var_缓冲区;
  复制字符串(r62, r63);
  r64 = cn_var_段副本_8;
  r65 = 40.路径段列表;
  r66 = cn_var_当前段索引_5;
    { long long _tmp_r17 = r64; cn_rt_array_set_element(r65, r66, &_tmp_r17, 8); }
  r67 = cn_var_当前段索引_5;
  r68 = r67 + 1;
  cn_var_当前段索引_5 = r68;
  cn_var_缓冲区位置_4 = 0;
  goto if_merge_3569;

  for_cond_3574:
  r52 = cn_var_j_9;
  r53 = cn_var_当前段索引_5;
  r54 = r52 < r53;
  if (r54) goto for_body_3575; else goto for_exit_3577;

  for_body_3575:
  r55 = 40.路径段列表;
  r56 = cn_var_j_9;
  r57 = *(void*)cn_rt_array_get_element(r55, r56, 8);
  释放内存(r57);
  goto for_update_3576;

  for_update_3576:
  r58 = cn_var_j_9;
  r59 = r58 + 1;
  cn_var_j_9 = r59;
  goto for_cond_3574;

  for_exit_3577:
  r60 = 40.路径段列表;
  释放内存(r60);
  r61 = 40.完全限定名;
  释放内存(r61);
  释放内存(40);
  return 0;
  goto if_merge_3573;
  return NULL;
}

void 销毁模块标识符(struct 模块标识符* cn_var_标识符) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18;

  entry:
  r0 = 0;
  if (r0) goto if_then_3578; else goto if_merge_3579;

  if_then_3578:
  return;
  goto if_merge_3579;

  if_merge_3579:
  r1 = 40.完全限定名;
  r2 = r1 != 0;
  if (r2) goto if_then_3580; else goto if_merge_3581;

  if_then_3580:
  r3 = 40.完全限定名;
  释放内存(r3);
  goto if_merge_3581;

  if_merge_3581:
  r4 = 40.路径段列表;
  r5 = r4 != 0;
  if (r5) goto if_then_3582; else goto if_merge_3583;

  if_then_3582:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3584;

  if_merge_3583:
  释放内存(40);

  for_cond_3584:
  r6 = cn_var_i_0;
  r7 = 40.路径段数量;
  r8 = r6 < r7;
  if (r8) goto for_body_3585; else goto for_exit_3587;

  for_body_3585:
  r9 = 40.路径段列表;
  r10 = cn_var_i_0;
  r11 = *(void*)cn_rt_array_get_element(r9, r10, 8);
  r12 = r11 != 0;
  if (r12) goto if_then_3588; else goto if_merge_3589;

  for_update_3586:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_3584;

  for_exit_3587:
  r18 = 40.路径段列表;
  释放内存(r18);
  goto if_merge_3583;

  if_then_3588:
  r13 = 40.路径段列表;
  r14 = cn_var_i_0;
  r15 = *(void*)cn_rt_array_get_element(r13, r14, 8);
  释放内存(r15);
  goto if_merge_3589;

  if_merge_3589:
  goto for_update_3586;
  return;
}

_Bool 解析模块文件路径(struct 模块加载器* cn_var_加载器, struct 模块标识符* cn_var_标识符, char* cn_var_输出路径, long long cn_var_输出路径大小) {
  long long r0, r1, r3, r4, r6, r7, r9, r10, r12, r13, r14, r18, r19, r20, r21, r22, r24, r25, r26, r27, r28, r29, r32, r33, r35, r36, r37, r38, r39, r40, r42, r43, r44, r45, r47, r50, r52, r53, r54, r57, r58;
  char* r5;
  char* r15;
  char* r17;
  char* r30;
  char* r34;
  char* r55;
  void* r16;
  void* r23;
  void* r31;
  void* r41;
  void* r46;
  void* r48;
  void* r56;
  void* r49;
  void* r51;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_3595; else goto logic_rhs_3594;

  if_then_3590:
  return 0;
  goto if_merge_3591;

  if_merge_3591:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_3596;

  logic_rhs_3592:
  r5 = cn_var_输出路径;
  r6 = r5 == 0;
  goto logic_merge_3593;

  logic_merge_3593:
  if (r6) goto if_then_3590; else goto if_merge_3591;

  logic_rhs_3594:
  r4 = 0;
  goto logic_merge_3595;

  logic_merge_3595:
  if (r4) goto logic_merge_3593; else goto logic_rhs_3592;

  for_cond_3596:
  r7 = cn_var_i_0;
  r8 = cn_var_加载器;
  r9 = r8.搜索路径数量;
  r10 = r7 < r9;
  if (r10) goto for_body_3597; else goto for_exit_3599;

  for_body_3597:
  char* cn_var_搜索路径_1;
  r11 = cn_var_加载器;
  r12 = r11.搜索路径列表;
  r13 = cn_var_i_0;
  r14 = *(void*)cn_rt_array_get_element(r12, r13, 8);
  cn_var_搜索路径_1 = r14;
  char* cn_var_路径缓冲区;
  cn_var_路径缓冲区 = cn_rt_array_alloc(8, 1024);
  long long cn_var_路径位置_2;
  cn_var_路径位置_2 = 0;
  long long cn_var_搜索路径长度_3;
  r15 = cn_var_搜索路径_1;
  获取字符串长度(r15);
  cn_var_搜索路径长度_3 = /* NONE */;
  r16 = cn_var_路径缓冲区;
  r17 = cn_var_搜索路径_1;
  r18 = cn_var_搜索路径长度_3;
  复制内存(r16, r17, r18);
  r19 = cn_var_搜索路径长度_3;
  cn_var_路径位置_2 = r19;
  long long cn_var_j_4;
  cn_var_j_4 = 0;
  goto for_cond_3600;

  for_update_3598:
  r57 = cn_var_i_0;
  r58 = r57 + 1;
  cn_var_i_0 = r58;
  goto for_cond_3596;

  for_exit_3599:
  return 0;

  for_cond_3600:
  r20 = cn_var_j_4;
  r21 = 40.路径段数量;
  r22 = r20 < r21;
  if (r22) goto for_body_3601; else goto for_exit_3603;

  for_body_3601:
  r23 = cn_var_路径缓冲区;
  r24 = cn_var_路径位置_2;
    { long long _tmp_i12 = 47; cn_rt_array_set_element(r23, r24, &_tmp_i12, 8); }
  r25 = cn_var_路径位置_2;
  r26 = r25 + 1;
  cn_var_路径位置_2 = r26;
  char* cn_var_段_5;
  r27 = 40.路径段列表;
  r28 = cn_var_j_4;
  r29 = *(void*)cn_rt_array_get_element(r27, r28, 8);
  cn_var_段_5 = r29;
  long long cn_var_段长度_6;
  r30 = cn_var_段_5;
  获取字符串长度(r30);
  cn_var_段长度_6 = /* NONE */;
  r31 = cn_var_路径缓冲区;
  r32 = cn_var_路径位置_2;
  r33 = r31 + r32;
  r34 = cn_var_段_5;
  r35 = cn_var_段长度_6;
  复制内存(r33, r34, r35);
  r36 = cn_var_路径位置_2;
  r37 = cn_var_段长度_6;
  r38 = r36 + r37;
  cn_var_路径位置_2 = r38;
  goto for_update_3602;

  for_update_3602:
  r39 = cn_var_j_4;
  r40 = r39 + 1;
  cn_var_j_4 = r40;
  goto for_cond_3600;

  for_exit_3603:
  r41 = cn_var_路径缓冲区;
  r42 = cn_var_路径位置_2;
  r43 = r41 + r42;
  复制字符串(r43, ".cn");
  r44 = cn_var_路径位置_2;
  r45 = r44 + 3;
  cn_var_路径位置_2 = r45;
  r46 = cn_var_路径缓冲区;
  r47 = cn_var_路径位置_2;
    { long long _tmp_i13 = 0; cn_rt_array_set_element(r46, r47, &_tmp_i13, 8); }
  void* cn_var_文件_7;
  r48 = cn_var_路径缓冲区;
  打开文件(r48, "r");
  cn_var_文件_7 = /* NONE */;
  r49 = cn_var_文件_7;
  r50 = r49 != 0;
  if (r50) goto if_then_3604; else goto if_merge_3605;

  if_then_3604:
  r51 = cn_var_文件_7;
  关闭文件(r51);
  r52 = cn_var_路径位置_2;
  r53 = cn_var_输出路径大小;
  r54 = r52 < r53;
  if (r54) goto if_then_3606; else goto if_merge_3607;

  if_merge_3605:
  goto for_update_3598;

  if_then_3606:
  r55 = cn_var_输出路径;
  r56 = cn_var_路径缓冲区;
  复制字符串(r55, r56);
  return 1;
  goto if_merge_3607;

  if_merge_3607:
  goto if_merge_3605;
  return 0;
}

long long 计算哈希值(char* cn_var_字符串指针) {
  long long r1, r2, r4, r5, r6, r7, r8, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19;
  char* r0;
  char* r3;
  char* r9;

  entry:
  long long cn_var_哈希_0;
  cn_var_哈希_0 = 0;
  r0 = cn_var_字符串指针;
  r1 = r0 == 0;
  if (r1) goto if_then_3608; else goto if_merge_3609;

  if_then_3608:
  r2 = cn_var_哈希_0;
  return r2;
  goto if_merge_3609;

  if_merge_3609:
  long long cn_var_i_1;
  cn_var_i_1 = 0;
  goto for_cond_3610;

  for_cond_3610:
  r3 = cn_var_字符串指针;
  r4 = cn_var_i_1;
  r5 = *(void*)cn_rt_array_get_element(r3, r4, 8);
  r6 = r5 != 0;
  if (r6) goto for_body_3611; else goto for_exit_3613;

  for_body_3611:
  r7 = cn_var_哈希_0;
  r8 = r7 * 31;
  r9 = cn_var_字符串指针;
  r10 = cn_var_i_1;
  r11 = *(void*)cn_rt_array_get_element(r9, r10, 8);
  r12 = r8 + r11;
  cn_var_哈希_0 = r12;
  goto for_update_3612;

  for_update_3612:
  r13 = cn_var_i_1;
  r14 = r13 + 1;
  cn_var_i_1 = r14;
  goto for_cond_3610;

  for_exit_3613:
  r15 = cn_var_哈希_0;
  r16 = r15 < 0;
  if (r16) goto if_then_3614; else goto if_merge_3615;

  if_then_3614:
  r17 = cn_var_哈希_0;
  r18 = -r17;
  cn_var_哈希_0 = r18;
  goto if_merge_3615;

  if_merge_3615:
  r19 = cn_var_哈希_0;
  return r19;
}

struct 模块元数据* 从缓存获取模块(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r2, r4, r6, r7, r8, r10, r11, r12, r14, r18;
  char* r3;
  char* r5;
  char* r16;
  char* r17;
  struct 模块缓存条目* r13;
  struct 模块缓存条目* r15;
  struct 模块缓存条目* r19;
  struct 模块元数据* r20;
  struct 模块缓存条目* r21;
  struct 模块缓存条目* r22;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3619; else goto logic_rhs_3618;

  if_then_3616:
  return 0;
  goto if_merge_3617;

  if_merge_3617:
  long long cn_var_哈希_0;
  r5 = cn_var_模块名;
  计算哈希值(r5);
  cn_var_哈希_0 = /* NONE */;
  long long cn_var_索引_1;
  r6 = cn_var_哈希_0;
  r7 = cn_var_最大模块缓存数量;
  r8 = r6 % r7;
  cn_var_索引_1 = r8;
  struct 模块缓存条目* cn_var_条目_2;
  r9 = cn_var_加载器;
  r10 = r9.缓存列表;
  r11 = cn_var_索引_1;
  r12 = *(void*)cn_rt_array_get_element(r10, r11, 8);
  cn_var_条目_2 = r12;
  goto while_cond_3620;

  logic_rhs_3618:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_3619;

  logic_merge_3619:
  if (r4) goto if_then_3616; else goto if_merge_3617;

  while_cond_3620:
  r13 = cn_var_条目_2;
  r14 = r13 != 0;
  if (r14) goto while_body_3621; else goto while_exit_3622;

  while_body_3621:
  r15 = cn_var_条目_2;
  r16 = r15->模块名;
  r17 = cn_var_模块名;
  比较字符串(r16, r17);
  r18 = /* NONE */ == 0;
  if (r18) goto if_then_3623; else goto if_merge_3624;

  while_exit_3622:
  return 0;

  if_then_3623:
  r19 = cn_var_条目_2;
  r20 = r19->元数据;
  return r20;
  goto if_merge_3624;

  if_merge_3624:
  r21 = cn_var_条目_2;
  r22 = r21->下一个;
  cn_var_条目_2 = r22;
  goto while_cond_3620;
  return NULL;
}

_Bool 添加模块到缓存(struct 模块加载器* cn_var_加载器, char* cn_var_模块名, struct 模块元数据* cn_var_元数据) {
  long long r0, r1, r3, r5, r7, r9, r10, r11, r13, r15, r16, r19, r26, r27, r28, r31, r32, r34, r35;
  char* r4;
  char* r8;
  char* r14;
  char* r18;
  char* r22;
  char* r23;
  struct 模块元数据* r6;
  struct 模块缓存条目* r12;
  struct 模块缓存条目* r17;
  struct 模块缓存条目* r20;
  struct 模块缓存条目* r21;
  struct 模块元数据* r24;
  struct 模块缓存条目* r29;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_3630; else goto logic_rhs_3629;

  if_then_3625:
  return 0;
  goto if_merge_3626;

  if_merge_3626:
  long long cn_var_哈希_0;
  r8 = cn_var_模块名;
  计算哈希值(r8);
  cn_var_哈希_0 = /* NONE */;
  long long cn_var_索引_1;
  r9 = cn_var_哈希_0;
  r10 = cn_var_最大模块缓存数量;
  r11 = r9 % r10;
  cn_var_索引_1 = r11;
  struct 模块缓存条目* cn_var_新条目_2;
  模块缓存条目大小();
  分配清零内存(1, /* NONE */);
  cn_var_新条目_2 = /* NONE */;
  r12 = cn_var_新条目_2;
  r13 = r12 == 0;
  if (r13) goto if_then_3631; else goto if_merge_3632;

  logic_rhs_3627:
  r6 = cn_var_元数据;
  r7 = r6 == 0;
  goto logic_merge_3628;

  logic_merge_3628:
  if (r7) goto if_then_3625; else goto if_merge_3626;

  logic_rhs_3629:
  r4 = cn_var_模块名;
  r5 = r4 == 0;
  goto logic_merge_3630;

  logic_merge_3630:
  if (r5) goto logic_merge_3628; else goto logic_rhs_3627;

  if_then_3631:
  return 0;
  goto if_merge_3632;

  if_merge_3632:
  long long cn_var_名称长度_3;
  r14 = cn_var_模块名;
  获取字符串长度(r14);
  cn_var_名称长度_3 = /* NONE */;
  r15 = cn_var_名称长度_3;
  r16 = r15 + 1;
  分配内存(r16);
  r17 = cn_var_新条目_2;
  r18 = r17->模块名;
  r19 = r18 == 0;
  if (r19) goto if_then_3633; else goto if_merge_3634;

  if_then_3633:
  r20 = cn_var_新条目_2;
  释放内存(r20);
  return 0;
  goto if_merge_3634;

  if_merge_3634:
  r21 = cn_var_新条目_2;
  r22 = r21->模块名;
  r23 = cn_var_模块名;
  复制字符串(r22, r23);
  r24 = cn_var_元数据;
  r25 = cn_var_加载器;
  r26 = r25.缓存列表;
  r27 = cn_var_索引_1;
  r28 = *(void*)cn_rt_array_get_element(r26, r27, 8);
  r29 = cn_var_新条目_2;
  r30 = cn_var_加载器;
  r31 = r30.缓存列表;
  r32 = cn_var_索引_1;
    { long long _tmp_r18 = r29; cn_rt_array_set_element(r31, r32, &_tmp_r18, 8); }
  r33 = cn_var_加载器;
  r34 = r33.缓存数量;
  r35 = r34 + 1;
  return 1;
}

_Bool 读取文件内容(char* cn_var_文件路径, char** cn_var_输出缓冲区, long long* cn_var_输出大小) {
  long long r0, r1, r3, r5, r7, r10, r12, r13, r15, r16, r18, r22, r24, r25, r26, r29, r31;
  char* r2;
  char* r8;
  char* r17;
  char* r21;
  char* r27;
  char* r28;
  char* r30;
  long long* r6;
  void* r9;
  void* r11;
  void* r14;
  void* r19;
  void* r20;
  void* r23;
  struct 输出缓冲区 r4;

  entry:
  r2 = cn_var_文件路径;
  r3 = r2 == 0;
  if (r3) goto logic_merge_3640; else goto logic_rhs_3639;

  if_then_3635:
  return 0;
  goto if_merge_3636;

  if_merge_3636:
  void* cn_var_文件_0;
  r8 = cn_var_文件路径;
  打开文件(r8, "rb");
  cn_var_文件_0 = /* NONE */;
  r9 = cn_var_文件_0;
  r10 = r9 == 0;
  if (r10) goto if_then_3641; else goto if_merge_3642;

  logic_rhs_3637:
  r6 = cn_var_输出大小;
  r7 = r6 == 0;
  goto logic_merge_3638;

  logic_merge_3638:
  if (r7) goto if_then_3635; else goto if_merge_3636;

  logic_rhs_3639:
  r4 = cn_var_输出缓冲区;
  r5 = r4 == 0;
  goto logic_merge_3640;

  logic_merge_3640:
  if (r5) goto logic_merge_3638; else goto logic_rhs_3637;

  if_then_3641:
  return 0;
  goto if_merge_3642;

  if_merge_3642:
  long long cn_var_文件大小_1;
  r11 = cn_var_文件_0;
  获取文件大小(r11);
  cn_var_文件大小_1 = /* NONE */;
  r12 = cn_var_文件大小_1;
  r13 = r12 <= 0;
  if (r13) goto if_then_3643; else goto if_merge_3644;

  if_then_3643:
  r14 = cn_var_文件_0;
  关闭文件(r14);
  return 0;
  goto if_merge_3644;

  if_merge_3644:
  char* cn_var_缓冲区_2;
  r15 = cn_var_文件大小_1;
  r16 = r15 + 1;
  分配内存(r16);
  cn_var_缓冲区_2 = /* NONE */;
  r17 = cn_var_缓冲区_2;
  r18 = r17 == 0;
  if (r18) goto if_then_3645; else goto if_merge_3646;

  if_then_3645:
  r19 = cn_var_文件_0;
  关闭文件(r19);
  return 0;
  goto if_merge_3646;

  if_merge_3646:
  long long cn_var_读取字节数_3;
  r20 = cn_var_文件_0;
  r21 = cn_var_缓冲区_2;
  r22 = cn_var_文件大小_1;
  读取文件(r20, r21, r22);
  cn_var_读取字节数_3 = /* NONE */;
  r23 = cn_var_文件_0;
  关闭文件(r23);
  r24 = cn_var_读取字节数_3;
  r25 = cn_var_文件大小_1;
  r26 = r24 != r25;
  if (r26) goto if_then_3647; else goto if_merge_3648;

  if_then_3647:
  r27 = cn_var_缓冲区_2;
  释放内存(r27);
  return 0;
  goto if_merge_3648;

  if_merge_3648:
  r28 = cn_var_缓冲区_2;
  r29 = cn_var_文件大小_1;
    { long long _tmp_i14 = 0; cn_rt_array_set_element(r28, r29, &_tmp_i14, 8); }
  r30 = cn_var_缓冲区_2;
  r31 = cn_var_文件大小_1;
  return 1;
}

struct 模块元数据* 创建模块元数据(struct 模块标识符* cn_var_标识符) {
  long long r0, r2;
  struct 模块元数据* r1;
  struct 模块元数据* r3;

  entry:
  r0 = 0;
  if (r0) goto if_then_3649; else goto if_merge_3650;

  if_then_3649:
  return 0;
  goto if_merge_3650;

  if_merge_3650:
  struct 模块元数据* cn_var_元数据_0;
  模块元数据大小();
  分配清零内存(1, /* NONE */);
  cn_var_元数据_0 = /* NONE */;
  r1 = cn_var_元数据_0;
  r2 = r1 == 0;
  if (r2) goto if_then_3651; else goto if_merge_3652;

  if_then_3651:
  return 0;
  goto if_merge_3652;

  if_merge_3652:
  r3 = cn_var_元数据_0;
  return r3;
}

void 销毁模块元数据(struct 模块元数据* cn_var_元数据) {
  long long r1, r4, r9, r14, r19;
  char* r8;
  char* r11;
  struct 模块元数据* r0;
  struct 模块元数据* r2;
  struct 模块标识符* r3;
  struct 模块元数据* r5;
  struct 模块标识符* r6;
  struct 模块元数据* r7;
  struct 模块元数据* r10;
  struct 模块元数据* r12;
  struct 导出符号* r13;
  struct 模块元数据* r15;
  struct 导出符号* r16;
  struct 模块元数据* r17;
  struct 模块依赖* r18;
  struct 模块元数据* r20;
  struct 模块依赖* r21;
  struct 模块元数据* r22;

  entry:
  r0 = cn_var_元数据;
  r1 = r0 == 0;
  if (r1) goto if_then_3653; else goto if_merge_3654;

  if_then_3653:
  return;
  goto if_merge_3654;

  if_merge_3654:
  r2 = cn_var_元数据;
  r3 = r2->标识符;
  r4 = r3 != 0;
  if (r4) goto if_then_3655; else goto if_merge_3656;

  if_then_3655:
  r5 = cn_var_元数据;
  r6 = r5->标识符;
  销毁模块标识符(r6);
  goto if_merge_3656;

  if_merge_3656:
  r7 = cn_var_元数据;
  r8 = r7->文件路径;
  r9 = r8 != 0;
  if (r9) goto if_then_3657; else goto if_merge_3658;

  if_then_3657:
  r10 = cn_var_元数据;
  r11 = r10->文件路径;
  释放内存(r11);
  goto if_merge_3658;

  if_merge_3658:
  r12 = cn_var_元数据;
  r13 = r12->导出符号表;
  r14 = r13 != 0;
  if (r14) goto if_then_3659; else goto if_merge_3660;

  if_then_3659:
  r15 = cn_var_元数据;
  r16 = r15->导出符号表;
  释放内存(r16);
  goto if_merge_3660;

  if_merge_3660:
  r17 = cn_var_元数据;
  r18 = r17->依赖列表;
  r19 = r18 != 0;
  if (r19) goto if_then_3661; else goto if_merge_3662;

  if_then_3661:
  r20 = cn_var_元数据;
  r21 = r20->依赖列表;
  释放内存(r21);
  goto if_merge_3662;

  if_merge_3662:
  r22 = cn_var_元数据;
  释放内存(r22);
  return;
}

struct 模块元数据* 加载模块核心(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r2, r4, r8, r11, r12, r13, r15, r17, r19, r21, r24, r26, r29, r30, r32, r33, r36, r39, r40, r44, r47, r48, r49, r50, r52, r55, r56, r58, r60, r64, r65, r68, r70, r75, r76, r79, r85, r86, r95, r96;
  char* r3;
  char* r6;
  char* r16;
  char* r35;
  char* r42;
  char* r46;
  char* r57;
  char* r61;
  char* r72;
  char* r82;
  char* r90;
  char* r92;
  void* r23;
  void* r31;
  void* r43;
  void* r45;
  struct 模块元数据* r7;
  struct 模块元数据* r9;
  struct 模块元数据* r20;
  struct 模块元数据* r27;
  struct 模块元数据* r34;
  struct 模块元数据* r37;
  struct 模块元数据* r41;
  struct 模块元数据* r53;
  struct 扫描器* r59;
  struct 模块元数据* r62;
  struct 扫描器* r66;
  struct 解析器* r69;
  struct 扫描器* r71;
  struct 模块元数据* r73;
  struct 解析器* r77;
  struct 程序节点* r78;
  struct 解析器* r80;
  struct 扫描器* r81;
  struct 模块元数据* r83;
  struct 程序节点* r87;
  struct 解析器* r88;
  struct 扫描器* r89;
  struct 模块元数据* r93;
  struct 模块元数据* r97;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3666; else goto logic_rhs_3665;

  if_then_3663:
  return 0;
  goto if_merge_3664;

  if_merge_3664:
  struct 模块元数据* cn_var_缓存模块_0;
  r5 = cn_var_加载器;
  r6 = cn_var_模块名;
  从缓存获取模块(r5, r6);
  cn_var_缓存模块_0 = /* NONE */;
  r7 = cn_var_缓存模块_0;
  r8 = r7 != 0;
  if (r8) goto if_then_3667; else goto if_merge_3668;

  logic_rhs_3665:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_3666;

  logic_merge_3666:
  if (r4) goto if_then_3663; else goto if_merge_3664;

  if_then_3667:
  r9 = cn_var_缓存模块_0;
  return r9;
  goto if_merge_3668;

  if_merge_3668:
  r10 = cn_var_加载器;
  r11 = r10.当前依赖深度;
  r12 = cn_var_最大依赖深度;
  r13 = r11 >= r12;
  if (r13) goto if_then_3669; else goto if_merge_3670;

  if_then_3669:
  r14 = cn_var_加载器;
  r15 = r14.诊断集合;
  报告错误(r15, -842150451, 0, "模块依赖深度超过限制");
  return 0;
  goto if_merge_3670;

  if_merge_3670:
  struct 模块标识符* cn_var_标识符_1;
  r16 = cn_var_模块名;
  解析模块标识符(r16);
  cn_var_标识符_1 = /* NONE */;
  r17 = 0;
  if (r17) goto if_then_3671; else goto if_merge_3672;

  if_then_3671:
  r18 = cn_var_加载器;
  r19 = r18.诊断集合;
  报告错误(r19, -842150451, 0, "无法解析模块标识符");
  return 0;
  goto if_merge_3672;

  if_merge_3672:
  struct 模块元数据* cn_var_元数据_2;
  创建模块元数据(40);
  cn_var_元数据_2 = /* NONE */;
  r20 = cn_var_元数据_2;
  r21 = r20 == 0;
  if (r21) goto if_then_3673; else goto if_merge_3674;

  if_then_3673:
  销毁模块标识符(40);
  return 0;
  goto if_merge_3674;

  if_merge_3674:
  char* cn_var_文件路径缓冲区;
  cn_var_文件路径缓冲区 = cn_rt_array_alloc(8, 1024);
  r22 = cn_var_加载器;
  r23 = cn_var_文件路径缓冲区;
  解析模块文件路径(r22, 40, r23, 1024);
  r24 = !/* NONE */;
  if (r24) goto if_then_3675; else goto if_merge_3676;

  if_then_3675:
  r25 = cn_var_加载器;
  r26 = r25.诊断集合;
  报告错误(r26, -842150451, 0, "无法找到模块文件");
  r27 = cn_var_元数据_2;
  销毁模块元数据(r27);
  r28 = cn_var_加载器;
  r29 = r28.加载失败数;
  r30 = r29 + 1;
  return 0;
  goto if_merge_3676;

  if_merge_3676:
  long long cn_var_路径长度_3;
  r31 = cn_var_文件路径缓冲区;
  获取字符串长度(r31);
  cn_var_路径长度_3 = /* NONE */;
  r32 = cn_var_路径长度_3;
  r33 = r32 + 1;
  分配内存(r33);
  r34 = cn_var_元数据_2;
  r35 = r34->文件路径;
  r36 = r35 == 0;
  if (r36) goto if_then_3677; else goto if_merge_3678;

  if_then_3677:
  r37 = cn_var_元数据_2;
  销毁模块元数据(r37);
  r38 = cn_var_加载器;
  r39 = r38.加载失败数;
  r40 = r39 + 1;
  return 0;
  goto if_merge_3678;

  if_merge_3678:
  r41 = cn_var_元数据_2;
  r42 = r41->文件路径;
  r43 = cn_var_文件路径缓冲区;
  复制字符串(r42, r43);
  r44 = cn_var_路径长度_3;
  char* cn_var_源码内容_4;
  cn_var_源码内容_4 = 0;
  long long cn_var_源码大小_5;
  cn_var_源码大小_5 = 0;
  r45 = cn_var_文件路径缓冲区;
  r46 = cn_var_源码内容_4;
  r47 = &cn_var_源码内容_4;
  r48 = cn_var_源码大小_5;
  r49 = &cn_var_源码大小_5;
  读取文件内容(r45, r47, r49);
  r50 = !/* NONE */;
  if (r50) goto if_then_3679; else goto if_merge_3680;

  if_then_3679:
  r51 = cn_var_加载器;
  r52 = r51.诊断集合;
  报告错误(r52, -842150451, 0, "无法读取模块文件");
  r53 = cn_var_元数据_2;
  销毁模块元数据(r53);
  r54 = cn_var_加载器;
  r55 = r54.加载失败数;
  r56 = r55 + 1;
  return 0;
  goto if_merge_3680;

  if_merge_3680:
  struct 扫描器* cn_var_扫描器实例_6;
  r57 = cn_var_源码内容_4;
  r58 = cn_var_源码大小_5;
  创建扫描器(r57, r58);
  cn_var_扫描器实例_6 = /* NONE */;
  r59 = cn_var_扫描器实例_6;
  r60 = r59 == 0;
  if (r60) goto if_then_3681; else goto if_merge_3682;

  if_then_3681:
  r61 = cn_var_源码内容_4;
  释放内存(r61);
  r62 = cn_var_元数据_2;
  销毁模块元数据(r62);
  r63 = cn_var_加载器;
  r64 = r63.加载失败数;
  r65 = r64 + 1;
  return 0;
  goto if_merge_3682;

  if_merge_3682:
  struct 解析器* cn_var_解析器实例_7;
  r66 = cn_var_扫描器实例_6;
  r67 = cn_var_加载器;
  r68 = r67.诊断集合;
  创建解析器(r66, r68);
  cn_var_解析器实例_7 = /* NONE */;
  r69 = cn_var_解析器实例_7;
  r70 = r69 == 0;
  if (r70) goto if_then_3683; else goto if_merge_3684;

  if_then_3683:
  r71 = cn_var_扫描器实例_6;
  销毁扫描器(r71);
  r72 = cn_var_源码内容_4;
  释放内存(r72);
  r73 = cn_var_元数据_2;
  销毁模块元数据(r73);
  r74 = cn_var_加载器;
  r75 = r74.加载失败数;
  r76 = r75 + 1;
  return 0;
  goto if_merge_3684;

  if_merge_3684:
  struct 程序节点* cn_var_程序_8;
  r77 = cn_var_解析器实例_7;
  解析程序(r77);
  cn_var_程序_8 = /* NONE */;
  r78 = cn_var_程序_8;
  r79 = r78 == 0;
  if (r79) goto if_then_3685; else goto if_merge_3686;

  if_then_3685:
  r80 = cn_var_解析器实例_7;
  销毁解析器(r80);
  r81 = cn_var_扫描器实例_6;
  销毁扫描器(r81);
  r82 = cn_var_源码内容_4;
  释放内存(r82);
  r83 = cn_var_元数据_2;
  销毁模块元数据(r83);
  r84 = cn_var_加载器;
  r85 = r84.加载失败数;
  r86 = r85 + 1;
  return 0;
  goto if_merge_3686;

  if_merge_3686:
  r87 = cn_var_程序_8;
  r88 = cn_var_解析器实例_7;
  销毁解析器(r88);
  r89 = cn_var_扫描器实例_6;
  销毁扫描器(r89);
  r90 = cn_var_源码内容_4;
  释放内存(r90);
  r91 = cn_var_加载器;
  r92 = cn_var_模块名;
  r93 = cn_var_元数据_2;
  添加模块到缓存(r91, r92, r93);
  r94 = cn_var_加载器;
  r95 = r94.加载成功数;
  r96 = r95 + 1;
  r97 = cn_var_元数据_2;
  return r97;
}

struct 模块元数据* 加载模块(struct 模块加载器* cn_var_加载器, char* cn_var_模块名) {
  long long r0, r2, r4, r6, r8, r10, r11, r15;
  char* r3;
  char* r13;
  struct 模块元数据* r14;
  struct 模块元数据* r16;

  entry:
  r1 = cn_var_加载器;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3690; else goto logic_rhs_3689;

  if_then_3687:
  return 0;
  goto if_merge_3688;

  if_merge_3688:
  struct 模块元数据* cn_var_保存当前模块_0;
  r5 = cn_var_加载器;
  r6 = r5.当前模块;
  cn_var_保存当前模块_0 = r6;
  long long cn_var_保存依赖深度_1;
  r7 = cn_var_加载器;
  r8 = r7.当前依赖深度;
  cn_var_保存依赖深度_1 = r8;
  r9 = cn_var_加载器;
  r10 = r9.当前依赖深度;
  r11 = r10 + 1;
  struct 模块元数据* cn_var_结果_2;
  r12 = cn_var_加载器;
  r13 = cn_var_模块名;
  加载模块核心(r12, r13);
  cn_var_结果_2 = /* NONE */;
  r14 = cn_var_保存当前模块_0;
  r15 = cn_var_保存依赖深度_1;
  r16 = cn_var_结果_2;
  return r16;

  logic_rhs_3689:
  r3 = cn_var_模块名;
  r4 = r3 == 0;
  goto logic_merge_3690;

  logic_merge_3690:
  if (r4) goto if_then_3687; else goto if_merge_3688;
  return NULL;
}

_Bool 处理导入语句(struct 模块加载器* cn_var_加载器, struct 导入语句节点* cn_var_导入节点, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r3, r5, r7, r9, r11, r15, r17, r19, r20, r22, r23, r24, r25, r28, r29, r31, r33, r37, r38, r39, r43, r44;
  char* r10;
  char* r13;
  struct 导入语句节点* r4;
  struct 模块元数据* r6;
  struct 导入语句节点* r8;
  struct 模块元数据* r14;
  struct 模块元数据* r16;
  struct 模块元数据* r18;
  struct 模块元数据* r21;
  struct 模块元数据* r26;
  struct 模块依赖* r27;
  struct 模块依赖* r30;
  struct 模块依赖* r32;
  struct 模块元数据* r34;
  struct 模块依赖* r35;
  struct 模块元数据* r36;
  struct 模块元数据* r40;
  struct 模块标识符* r41;
  struct 模块元数据* r42;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_3696; else goto logic_rhs_3695;

  if_then_3691:
  return 0;
  goto if_merge_3692;

  if_merge_3692:
  char* cn_var_模块名_0;
  r8 = cn_var_导入节点;
  r9 = r8->模块名;
  cn_var_模块名_0 = r9;
  r10 = cn_var_模块名_0;
  r11 = r10 == 0;
  if (r11) goto if_then_3697; else goto if_merge_3698;

  logic_rhs_3693:
  r6 = cn_var_当前模块;
  r7 = r6 == 0;
  goto logic_merge_3694;

  logic_merge_3694:
  if (r7) goto if_then_3691; else goto if_merge_3692;

  logic_rhs_3695:
  r4 = cn_var_导入节点;
  r5 = r4 == 0;
  goto logic_merge_3696;

  logic_merge_3696:
  if (r5) goto logic_merge_3694; else goto logic_rhs_3693;

  if_then_3697:
  return 0;
  goto if_merge_3698;

  if_merge_3698:
  struct 模块元数据* cn_var_依赖模块_1;
  r12 = cn_var_加载器;
  r13 = cn_var_模块名_0;
  加载模块(r12, r13);
  cn_var_依赖模块_1 = /* NONE */;
  r14 = cn_var_依赖模块_1;
  r15 = r14 == 0;
  if (r15) goto if_then_3699; else goto if_merge_3700;

  if_then_3699:
  return 0;
  goto if_merge_3700;

  if_merge_3700:
  r16 = cn_var_当前模块;
  r17 = r16->依赖数量;
  r18 = cn_var_当前模块;
  r19 = r18->依赖容量;
  r20 = r17 >= r19;
  if (r20) goto if_then_3701; else goto if_merge_3702;

  if_then_3701:
  long long cn_var_新容量_2;
  r21 = cn_var_当前模块;
  r22 = r21->依赖容量;
  r23 = r22 << 1;
  cn_var_新容量_2 = r23;
  r24 = cn_var_新容量_2;
  r25 = r24 == 0;
  if (r25) goto if_then_3703; else goto if_merge_3704;

  if_merge_3702:
  struct 模块依赖* cn_var_依赖_4;
  r34 = cn_var_当前模块;
  r35 = r34->依赖列表;
  r36 = cn_var_当前模块;
  r37 = r36->依赖数量;
  r38 = *(void*)cn_rt_array_get_element(r35, r37, 8);
  cn_var_依赖_4 = r39;
  r40 = cn_var_依赖模块_1;
  r41 = r40->标识符;
  r42 = cn_var_当前模块;
  r43 = r42->依赖数量;
  r44 = r43 + 1;
  return 1;

  if_then_3703:
  cn_var_新容量_2 = 4;
  goto if_merge_3704;

  if_merge_3704:
  struct 模块依赖* cn_var_新数组_3;
  r26 = cn_var_当前模块;
  r27 = r26->依赖列表;
  r28 = cn_var_新容量_2;
  模块依赖大小();
  r29 = r28 * /* NONE */;
  重新分配内存(r27, r29);
  cn_var_新数组_3 = /* NONE */;
  r30 = cn_var_新数组_3;
  r31 = r30 == 0;
  if (r31) goto if_then_3705; else goto if_merge_3706;

  if_then_3705:
  return 0;
  goto if_merge_3706;

  if_merge_3706:
  r32 = cn_var_新数组_3;
  r33 = cn_var_新容量_2;
  goto if_merge_3702;
  return 0;
}

_Bool 处理所有导入语句(struct 模块加载器* cn_var_加载器, struct 模块元数据* cn_var_当前模块) {
  long long r0, r1, r3, r5, r8, r12, r14, r16, r17, r19, r23, r24, r26, r30, r32;
  struct 模块元数据* r4;
  struct 模块元数据* r6;
  long long* r7;
  struct 模块元数据* r9;
  long long* r10;
  struct 程序节点* r11;
  struct 声明节点列表* r13;
  struct 声明节点列表* r15;
  struct 声明节点* r18;
  struct 声明节点* r20;
  struct 声明节点* r25;
  struct 导入语句节点* r28;
  struct 模块元数据* r29;
  struct 声明节点列表* r31;
  enum 节点类型 r21;
  enum 节点类型 r22;

  entry:
  r2 = cn_var_加载器;
  r3 = r2 == 0;
  if (r3) goto logic_merge_3712; else goto logic_rhs_3711;

  if_then_3707:
  return 0;
  goto if_merge_3708;

  if_merge_3708:
  struct 程序节点* cn_var_程序_0;
  r9 = cn_var_当前模块;
  r10 = r9->程序节点;
  cn_var_程序_0 = r10;
  struct 声明节点列表* cn_var_声明列表_1;
  r11 = cn_var_程序_0;
  r12 = r11->声明列表;
  cn_var_声明列表_1 = r12;
  goto while_cond_3713;

  logic_rhs_3709:
  r6 = cn_var_当前模块;
  r7 = r6->程序节点;
  r8 = r7 == 0;
  goto logic_merge_3710;

  logic_merge_3710:
  if (r8) goto if_then_3707; else goto if_merge_3708;

  logic_rhs_3711:
  r4 = cn_var_当前模块;
  r5 = r4 == 0;
  goto logic_merge_3712;

  logic_merge_3712:
  if (r5) goto logic_merge_3710; else goto logic_rhs_3709;

  while_cond_3713:
  r13 = cn_var_声明列表_1;
  r14 = r13 != 0;
  if (r14) goto while_body_3714; else goto while_exit_3715;

  while_body_3714:
  struct 声明节点* cn_var_声明_2;
  r15 = cn_var_声明列表_1;
  r16 = r15->节点;
  cn_var_声明_2 = r16;
  r18 = cn_var_声明_2;
  r19 = r18 != 0;
  if (r19) goto logic_rhs_3718; else goto logic_merge_3719;

  while_exit_3715:
  return 1;

  if_then_3716:
  struct 导入语句节点* cn_var_导入节点_3;
  r25 = cn_var_声明_2;
  r26 = r25->作为导入语句;
  cn_var_导入节点_3 = r26;
  r27 = cn_var_加载器;
  r28 = cn_var_导入节点_3;
  r29 = cn_var_当前模块;
  处理导入语句(r27, r28, r29);
  r30 = !/* NONE */;
  if (r30) goto if_then_3720; else goto if_merge_3721;

  if_merge_3717:
  r31 = cn_var_声明列表_1;
  r32 = r31->下一个;
  cn_var_声明列表_1 = r32;
  goto while_cond_3713;

  logic_rhs_3718:
  r20 = cn_var_声明_2;
  r21 = r20->类型;
  r22 = cn_var_节点类型;
  r23 = r22.导入声明;
  r24 = r21 == r23;
  goto logic_merge_3719;

  logic_merge_3719:
  if (r24) goto if_then_3716; else goto if_merge_3717;

  if_then_3720:
  goto if_merge_3721;

  if_merge_3721:
  goto if_merge_3717;
  return 0;
}

_Bool 收集导出符号(struct 模块元数据* cn_var_元数据) {
  long long r0, r2, r5, r9, r11, r13, r14, r16, r18, r19, r20, r22, r23, r24, r25, r28, r29, r31, r33, r35, r36, r38, r40, r43, r44, r45, r49, r50, r52, r53, r54, r55, r59, r60, r62, r63, r64, r65, r66, r70, r71, r73, r75, r77, r79, r80, r81, r82, r84;
  struct 模块元数据* r1;
  struct 模块元数据* r3;
  long long* r4;
  struct 模块元数据* r6;
  long long* r7;
  struct 程序节点* r8;
  struct 声明节点列表* r10;
  struct 声明节点列表* r12;
  struct 声明节点* r15;
  struct 声明节点* r17;
  struct 声明节点列表* r21;
  struct 模块元数据* r26;
  struct 导出符号* r27;
  struct 程序节点* r30;
  struct 声明节点列表* r32;
  struct 声明节点列表* r34;
  struct 声明节点* r37;
  struct 声明节点* r39;
  struct 模块元数据* r41;
  struct 导出符号* r42;
  struct 声明节点* r46;
  struct 声明节点* r51;
  struct 声明节点* r56;
  struct 声明节点* r61;
  struct 声明节点* r67;
  struct 声明节点* r72;
  struct 常量声明节点* r74;
  struct 常量声明节点* r76;
  struct 常量声明节点* r78;
  struct 声明节点列表* r83;
  enum 节点类型 r47;
  enum 节点类型 r48;
  enum 节点类型 r57;
  enum 节点类型 r58;
  enum 节点类型 r68;
  enum 节点类型 r69;

  entry:
  r1 = cn_var_元数据;
  r2 = r1 == 0;
  if (r2) goto logic_merge_3725; else goto logic_rhs_3724;

  if_then_3722:
  return 0;
  goto if_merge_3723;

  if_merge_3723:
  struct 程序节点* cn_var_程序_0;
  r6 = cn_var_元数据;
  r7 = r6->程序节点;
  cn_var_程序_0 = r7;
  long long cn_var_符号数量_1;
  cn_var_符号数量_1 = 0;
  struct 声明节点列表* cn_var_声明列表_2;
  r8 = cn_var_程序_0;
  r9 = r8->声明列表;
  cn_var_声明列表_2 = r9;
  goto while_cond_3726;

  logic_rhs_3724:
  r3 = cn_var_元数据;
  r4 = r3->程序节点;
  r5 = r4 == 0;
  goto logic_merge_3725;

  logic_merge_3725:
  if (r5) goto if_then_3722; else goto if_merge_3723;

  while_cond_3726:
  r10 = cn_var_声明列表_2;
  r11 = r10 != 0;
  if (r11) goto while_body_3727; else goto while_exit_3728;

  while_body_3727:
  struct 声明节点* cn_var_声明_3;
  r12 = cn_var_声明列表_2;
  r13 = r12->节点;
  cn_var_声明_3 = r13;
  r15 = cn_var_声明_3;
  r16 = r15 != 0;
  if (r16) goto logic_rhs_3731; else goto logic_merge_3732;

  while_exit_3728:
  r23 = cn_var_符号数量_1;
  r24 = r23 == 0;
  if (r24) goto if_then_3733; else goto if_merge_3734;

  if_then_3729:
  r19 = cn_var_符号数量_1;
  r20 = r19 + 1;
  cn_var_符号数量_1 = r20;
  goto if_merge_3730;

  if_merge_3730:
  r21 = cn_var_声明列表_2;
  r22 = r21->下一个;
  cn_var_声明列表_2 = r22;
  goto while_cond_3726;

  logic_rhs_3731:
  r17 = cn_var_声明_3;
  r18 = r17->是否公开;
  goto logic_merge_3732;

  logic_merge_3732:
  if (r18) goto if_then_3729; else goto if_merge_3730;

  if_then_3733:
  return 1;
  goto if_merge_3734;

  if_merge_3734:
  r25 = cn_var_符号数量_1;
  导出符号大小();
  分配清零内存(r25, /* NONE */);
  r26 = cn_var_元数据;
  r27 = r26->导出符号表;
  r28 = r27 == 0;
  if (r28) goto if_then_3735; else goto if_merge_3736;

  if_then_3735:
  return 0;
  goto if_merge_3736;

  if_merge_3736:
  r29 = cn_var_符号数量_1;
  r30 = cn_var_程序_0;
  r31 = r30->声明列表;
  cn_var_声明列表_2 = r31;
  long long cn_var_当前索引_4;
  cn_var_当前索引_4 = 0;
  goto while_cond_3737;

  while_cond_3737:
  r32 = cn_var_声明列表_2;
  r33 = r32 != 0;
  if (r33) goto while_body_3738; else goto while_exit_3739;

  while_body_3738:
  struct 声明节点* cn_var_声明_5;
  r34 = cn_var_声明列表_2;
  r35 = r34->节点;
  cn_var_声明_5 = r35;
  r37 = cn_var_声明_5;
  r38 = r37 != 0;
  if (r38) goto logic_rhs_3742; else goto logic_merge_3743;

  while_exit_3739:
  return 1;

  if_then_3740:
  struct 导出符号* cn_var_符号_6;
  r41 = cn_var_元数据;
  r42 = r41->导出符号表;
  r43 = cn_var_当前索引_4;
  r44 = *(void*)cn_rt_array_get_element(r42, r43, 8);
  cn_var_符号_6 = r45;
  r46 = cn_var_声明_5;
  r47 = r46->类型;
  r48 = cn_var_节点类型;
  r49 = r48.函数声明;
  r50 = r47 == r49;
  if (r50) goto if_then_3744; else goto if_else_3745;

  if_merge_3741:
  r83 = cn_var_声明列表_2;
  r84 = r83->下一个;
  cn_var_声明列表_2 = r84;
  goto while_cond_3737;

  logic_rhs_3742:
  r39 = cn_var_声明_5;
  r40 = r39->是否公开;
  goto logic_merge_3743;

  logic_merge_3743:
  if (r40) goto if_then_3740; else goto if_merge_3741;

  if_then_3744:
  struct 函数声明节点* cn_var_函数声明_7;
  r51 = cn_var_声明_5;
  r52 = r51->作为函数声明;
  cn_var_函数声明_7 = r52;
  r53 = 1;
  if (r53) goto if_then_3747; else goto if_merge_3748;

  if_else_3745:
  r56 = cn_var_声明_5;
  r57 = r56->类型;
  r58 = cn_var_节点类型;
  r59 = r58.变量声明;
  r60 = r57 == r59;
  if (r60) goto if_then_3749; else goto if_else_3750;

  if_merge_3746:
  r80 = cn_var_当前索引_4;
  r81 = r80 + 1;
  cn_var_当前索引_4 = r81;
  r82 = cn_var_当前索引_4;
  goto if_merge_3741;

  if_then_3747:
  r54 = -842150451.函数名;
  r55 = -842150451.函数名;
  获取字符串长度(r55);
  goto if_merge_3748;

  if_merge_3748:
  goto if_merge_3746;

  if_then_3749:
  struct 变量声明节点* cn_var_变量声明_8;
  r61 = cn_var_声明_5;
  r62 = r61->作为变量声明;
  cn_var_变量声明_8 = r62;
  r63 = 1;
  if (r63) goto if_then_3752; else goto if_merge_3753;

  if_else_3750:
  r67 = cn_var_声明_5;
  r68 = r67->类型;
  r69 = cn_var_节点类型;
  r70 = r69.变量声明;
  r71 = r68 == r70;
  if (r71) goto if_then_3754; else goto if_merge_3755;

  if_merge_3751:
  goto if_merge_3746;

  if_then_3752:
  r64 = -842150451.变量名;
  r65 = -842150451.变量名;
  获取字符串长度(r65);
  r66 = -842150451.是常量;
  goto if_merge_3753;

  if_merge_3753:
  goto if_merge_3751;

  if_then_3754:
  struct 常量声明节点* cn_var_常量声明_9;
  r72 = cn_var_声明_5;
  r73 = r72->作为常量声明;
  cn_var_常量声明_9 = r73;
  r74 = cn_var_常量声明_9;
  r75 = r74 != 0;
  if (r75) goto if_then_3756; else goto if_merge_3757;

  if_merge_3755:
  goto if_merge_3751;

  if_then_3756:
  r76 = cn_var_常量声明_9;
  r77 = r76->常量名;
  r78 = cn_var_常量声明_9;
  r79 = r78->常量名;
  获取字符串长度(r79);
  goto if_merge_3757;

  if_merge_3757:
  goto if_merge_3755;
  return 0;
}

long long 模块加载器大小() {

  entry:
  return 1024;
}

long long 模块标识符大小() {

  entry:
  return 32;
}

long long 模块元数据大小() {

  entry:
  return 128;
}

long long 模块缓存条目大小() {

  entry:
  return 24;
}

long long 模块依赖大小() {

  entry:
  return 16;
}

long long 导出符号大小() {

  entry:
  return 32;
}

long long 字符串指针大小() {

  entry:
  return 8;
}

long long 获取文件大小(void* cn_var_文件) {
  long long r1, r5;
  void* r0;
  void* r2;
  void* r3;
  void* r4;

  entry:
  r0 = cn_var_文件;
  r1 = r0 == 0;
  if (r1) goto if_then_3758; else goto if_merge_3759;

  if_then_3758:
  return 0;
  goto if_merge_3759;

  if_merge_3759:
  r2 = cn_var_文件;
  文件定位(r2, 0, 2);
  long long cn_var_大小_0;
  r3 = cn_var_文件;
  获取文件位置(r3);
  cn_var_大小_0 = /* NONE */;
  r4 = cn_var_文件;
  文件定位(r4, 0, 0);
  r5 = cn_var_大小_0;
  return r5;
}

void 报告错误(struct 诊断集合* cn_var_诊断集合指针, enum 诊断错误码 cn_var_错误码, struct 源位置* cn_var_位置, char* cn_var_消息) {
  long long r1;
  char* r0;
  char* r2;

  entry:
  r0 = cn_var_消息;
  r1 = r0 != 0;
  if (r1) goto if_then_3760; else goto if_merge_3761;

  if_then_3760:
  r2 = cn_var_消息;
  打印格式("错误: %s\n", r2);
  goto if_merge_3761;

  if_merge_3761:
  return;
}

