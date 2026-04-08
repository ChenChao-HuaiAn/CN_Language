#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
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
enum 同步点类型 {
    同步点类型_同步点_声明关键字 = 7,
    同步点类型_同步点_类型关键字 = 6,
    同步点类型_同步点_关键字 = 5,
    同步点类型_同步点_逗号 = 4,
    同步点类型_同步点_右中括号 = 3,
    同步点类型_同步点_右小括号 = 2,
    同步点类型_同步点_右大括号 = 1,
    同步点类型_同步点_分号 = 0
};
enum 语法恢复策略 {
    语法恢复策略_同步到关键字 = 4,
    语法恢复策略_同步到大括号 = 3,
    语法恢复策略_同步到分号 = 2,
    语法恢复策略_跳过词元 = 1,
    语法恢复策略_不恢复 = 0
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

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块
extern long long cn_var_最大错误恢复次数;
extern long long cn_var_解析器大小;
_Bool 初始化错误恢复(struct 语法错误恢复上下文*);
void 销毁错误恢复上下文(struct 语法错误恢复上下文*);
long long 是同步点(enum 词元类型枚举, enum 同步点类型);
long long 是任意同步点(enum 词元类型枚举);
long long 跳转到同步点(struct 语法错误恢复上下文*, enum 词元类型枚举, enum 同步点类型);
_Bool 应该抑制错误(struct 语法错误恢复上下文*);
void 记录错误(struct 语法错误恢复上下文*, long long);
void 重置错误计数(struct 语法错误恢复上下文*);
enum 语法恢复策略 选择恢复策略(long long);
char* 同步点类型名称(enum 同步点类型);
char* 恢复策略名称(enum 语法恢复策略);
extern long long cn_var_默认最大连续错误;
extern long long cn_var_默认最大恢复次数;
extern long long cn_var_语法错误恢复上下文大小;
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
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
void 初始化关键字表(void);
struct 关键字条目* 获取关键字表(void);
long long 关键字总数(void);
_Bool 是关键字字符串(char*);
enum 词元类型枚举 查找关键字(char*);
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
_Bool 是块注释结束(long long, long long);
_Bool 是块注释开始(long long, long long);
_Bool 是单行注释开始(long long, long long);
_Bool 是八进制前缀(long long, long long);
_Bool 是二进制前缀(long long, long long);
_Bool 是十六进制前缀(long long, long long);
_Bool 是可打印字符(long long);
long long 转大写(long long);
long long 转小写(long long);
_Bool 是分隔符字符(long long);
_Bool 是运算符字符(long long);
_Bool 是标识符字符(long long);
_Bool 是标识符开头(long long);
_Bool 是中文字符(long long);
_Bool 是字母或数字(long long);
_Bool 是字母(long long);
_Bool 是八进制数字(long long);
_Bool 是二进制数字(long long);
_Bool 是十六进制数字(long long);
_Bool 是数字(long long);
_Bool 是换行符(long long);
_Bool 是空白字符(long long);
struct 扫描器* 创建扫描器(char*, long long);
void 销毁扫描器(struct 扫描器*);
struct 词元 下一个词元(struct 扫描器*);
struct 词元 预览词元(struct 扫描器*);
long long 当前字符(struct 扫描器*);
long long 预览字符(struct 扫描器*, long long);
long long 前进(struct 扫描器*);
_Bool 是否结束(struct 扫描器*);
void 跳过空白字符(struct 扫描器*);
void 跳过单行注释(struct 扫描器*);
_Bool 跳过块注释(struct 扫描器*);
struct 词元 扫描标识符(struct 扫描器*);
struct 词元 扫描数字(struct 扫描器*);
struct 词元 扫描字符串(struct 扫描器*);
struct 词元 扫描字符(struct 扫描器*);
struct 词元 扫描运算符(struct 扫描器*);
extern long long cn_var_最大标识符长度;
extern long long cn_var_最大字符串长度;
extern long long cn_var_最大数字长度;
extern long long cn_var_扫描器大小;
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);

// Global Variables

// Forward Declarations
struct 语句节点* 解析语句(struct 解析器*);
struct 块语句节点* 解析块语句(struct 解析器*);
struct 语句节点* 解析如果语句(struct 解析器*);
struct 语句节点* 解析当语句(struct 解析器*);
struct 语句节点* 解析循环语句(struct 解析器*);
struct 语句节点* 解析返回语句(struct 解析器*);
struct 语句节点* 解析中断语句(struct 解析器*);
struct 语句节点* 解析继续语句(struct 解析器*);
struct 语句节点* 解析选择语句(struct 解析器*);
void* 解析情况列表(struct 解析器*);
struct 语句节点* 解析尝试语句(struct 解析器*);
struct 语句节点* 解析抛出语句(struct 解析器*);
struct 语句节点* 解析表达式语句(struct 解析器*);
struct 语句节点* 解析变量声明语句(struct 解析器*);
struct 语句节点* 解析语句(struct 解析器*);
struct 块语句节点* 解析块语句(struct 解析器*);
struct 语句节点* 解析如果语句(struct 解析器*);
struct 语句节点* 解析当语句(struct 解析器*);
struct 语句节点* 解析循环语句(struct 解析器*);
struct 语句节点* 解析返回语句(struct 解析器*);
struct 语句节点* 解析中断语句(struct 解析器*);
struct 语句节点* 解析继续语句(struct 解析器*);
struct 语句节点* 解析选择语句(struct 解析器*);
void* 解析情况列表(struct 解析器*);
struct 语句节点* 解析尝试语句(struct 解析器*);
struct 语句节点* 解析抛出语句(struct 解析器*);
struct 语句节点* 解析表达式语句(struct 解析器*);
struct 语句节点* 解析变量声明语句(struct 解析器*);

struct 语句节点* 解析语句(struct 解析器* cn_var_实例);
struct 块语句节点* 解析块语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析如果语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析当语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析循环语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析返回语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析中断语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析继续语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析选择语句(struct 解析器* cn_var_实例);
void* 解析情况列表(struct 解析器* cn_var_实例);
struct 语句节点* 解析尝试语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析抛出语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析表达式语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析变量声明语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析语句(struct 解析器* cn_var_实例) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r18;
  struct 块语句节点* r19;
  struct 解析器* r20;
  struct 语句节点* r21;
  struct 解析器* r22;
  struct 语句节点* r23;
  struct 解析器* r24;
  struct 语句节点* r25;
  struct 解析器* r26;
  struct 语句节点* r27;
  struct 解析器* r28;
  struct 语句节点* r29;
  struct 解析器* r30;
  struct 语句节点* r31;
  struct 解析器* r32;
  struct 语句节点* r33;
  struct 解析器* r34;
  struct 语句节点* r35;
  struct 解析器* r36;
  struct 语句节点* r37;
  struct 解析器* r38;
  struct 语句节点* r39;
  struct 解析器* r40;
  struct 解析器* r44;
  struct 语句节点* r45;
  struct 解析器* r46;
  struct 语句节点* r47;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r43;
  struct 词元 r3;
  struct 词元 r41;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r42;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1293; else goto if_merge_1294;

  if_then_1293:
  return 0;
  goto if_merge_1294;

  if_merge_1294:
  r2 = cn_var_实例;
  r3 = r2->当前词元;
  r4 = r3.类型;
  r5 = r4 == 71;
  if (r5) goto case_body_1296; else goto switch_check_1310;

  switch_check_1310:
  r6 = r4 == 0;
  if (r6) goto case_body_1297; else goto switch_check_1311;

  switch_check_1311:
  r7 = r4 == 2;
  if (r7) goto case_body_1298; else goto switch_check_1312;

  switch_check_1312:
  r8 = r4 == 3;
  if (r8) goto case_body_1299; else goto switch_check_1313;

  switch_check_1313:
  r9 = r4 == 4;
  if (r9) goto case_body_1300; else goto switch_check_1314;

  switch_check_1314:
  r10 = r4 == 5;
  if (r10) goto case_body_1301; else goto switch_check_1315;

  switch_check_1315:
  r11 = r4 == 6;
  if (r11) goto case_body_1302; else goto switch_check_1316;

  switch_check_1316:
  r12 = r4 == 7;
  if (r12) goto case_body_1303; else goto switch_check_1317;

  switch_check_1317:
  r13 = r4 == 36;
  if (r13) goto case_body_1304; else goto switch_check_1318;

  switch_check_1318:
  r14 = r4 == 38;
  if (r14) goto case_body_1305; else goto switch_check_1319;

  switch_check_1319:
  r15 = r4 == 18;
  if (r15) goto case_body_1306; else goto switch_check_1320;

  switch_check_1320:
  r16 = r4 == /* NONE */;
  if (r16) goto case_body_1307; else goto switch_check_1321;

  switch_check_1321:
  r17 = r4 == 23;
  if (r17) goto case_body_1308; else goto case_default_1309;

  case_body_1296:
  r18 = cn_var_实例;
  r19 = 解析块语句(r18);
  return r19;
  goto switch_merge_1295;

  case_body_1297:
  r20 = cn_var_实例;
  r21 = 解析如果语句(r20);
  return r21;
  goto switch_merge_1295;

  case_body_1298:
  r22 = cn_var_实例;
  r23 = 解析当语句(r22);
  return r23;
  goto switch_merge_1295;

  case_body_1299:
  r24 = cn_var_实例;
  r25 = 解析循环语句(r24);
  return r25;
  goto switch_merge_1295;

  case_body_1300:
  r26 = cn_var_实例;
  r27 = 解析返回语句(r26);
  return r27;
  goto switch_merge_1295;

  case_body_1301:
  r28 = cn_var_实例;
  r29 = 解析中断语句(r28);
  return r29;
  goto switch_merge_1295;

  case_body_1302:
  r30 = cn_var_实例;
  r31 = 解析继续语句(r30);
  return r31;
  goto switch_merge_1295;

  case_body_1303:
  r32 = cn_var_实例;
  r33 = 解析选择语句(r32);
  return r33;
  goto switch_merge_1295;

  case_body_1304:
  r34 = cn_var_实例;
  r35 = 解析尝试语句(r34);
  return r35;
  goto switch_merge_1295;

  case_body_1305:
  r36 = cn_var_实例;
  r37 = 解析抛出语句(r36);
  return r37;
  goto switch_merge_1295;

  case_body_1306:
  goto switch_merge_1295;

  case_body_1307:
  goto switch_merge_1295;

  case_body_1308:
  r38 = cn_var_实例;
  r39 = 解析变量声明语句(r38);
  return r39;
  goto switch_merge_1295;

  case_default_1309:
  r40 = cn_var_实例;
  r41 = r40->当前词元;
  r42 = r41.类型;
  r43 = 是否类型关键字(r42);
  if (r43) goto if_then_1322; else goto if_merge_1323;

  if_then_1322:
  r44 = cn_var_实例;
  r45 = 解析变量声明语句(r44);
  return r45;
  goto if_merge_1323;

  if_merge_1323:
  r46 = cn_var_实例;
  r47 = 解析表达式语句(r46);
  return r47;
  goto switch_merge_1295;

  switch_merge_1295:
  return NULL;
}

struct 块语句节点* 解析块语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r9, r12, r16;
  struct 解析器* r0;
  struct 块语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r10;
  struct 解析器* r13;
  struct 语句节点* r14;
  struct 语句节点* r15;
  struct 块语句节点* r17;
  struct 语句节点* r18;
  struct 解析器* r19;
  struct 块语句节点* r21;
  _Bool r5;
  _Bool r8;
  _Bool r11;
  _Bool r20;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1324; else goto if_merge_1325;

  if_then_1324:
  return 0;
  goto if_merge_1325;

  if_merge_1325:
  struct 块语句节点* cn_var_块节点_0;
  创建块语句();
  cn_var_块节点_0 = /* NONE */;
  r2 = cn_var_块节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1326; else goto if_merge_1327;

  if_then_1326:
  return 0;
  goto if_merge_1327;

  if_merge_1327:
  r4 = cn_var_实例;
  r5 = 期望(r4, 71);
  goto while_cond_1328;

  while_cond_1328:
  r7 = cn_var_实例;
  r8 = 检查(r7, 72);
  r9 = !r8;
  if (r9) goto logic_rhs_1331; else goto logic_merge_1332;

  while_body_1329:
  struct 语句节点* cn_var_语句_1;
  r13 = cn_var_实例;
  r14 = 解析语句(r13);
  cn_var_语句_1 = r14;
  r15 = cn_var_语句_1;
  r16 = r15 != 0;
  if (r16) goto if_then_1333; else goto if_merge_1334;

  while_exit_1330:
  r19 = cn_var_实例;
  r20 = 期望(r19, 72);
  r21 = cn_var_块节点_0;
  return r21;

  logic_rhs_1331:
  r10 = cn_var_实例;
  r11 = 检查(r10, 80);
  r12 = !r11;
  goto logic_merge_1332;

  logic_merge_1332:
  if (r12) goto while_body_1329; else goto while_exit_1330;

  if_then_1333:
  r17 = cn_var_块节点_0;
  r18 = cn_var_语句_1;
  块语句添加(r17, r18);
  goto if_merge_1334;

  if_merge_1334:
  goto while_cond_1328;
  return NULL;
}

struct 语句节点* 解析如果语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 块语句节点* r12;
  struct 解析器* r13;
  struct 解析器* r15;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 语句节点* r19;
  struct 解析器* r20;
  struct 块语句节点* r21;
  struct 语句节点* r22;
  _Bool r6;
  _Bool r10;
  _Bool r14;
  _Bool r17;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1335; else goto if_merge_1336;

  if_then_1335:
  return 0;
  goto if_merge_1336;

  if_merge_1336:
  struct 语句节点* cn_var_节点_0;
  创建如果语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1337; else goto if_merge_1338;

  if_then_1337:
  return 0;
  goto if_merge_1338;

  if_merge_1338:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 69);
  r7 = cn_var_实例;
  r8 = 解析表达式(r7);
  r9 = cn_var_实例;
  r10 = 期望(r9, 70);
  r11 = cn_var_实例;
  r12 = 解析块语句(r11);
  r13 = cn_var_实例;
  r14 = 检查(r13, 1);
  if (r14) goto if_then_1339; else goto if_merge_1340;

  if_then_1339:
  r15 = cn_var_实例;
  前进词元(r15);
  r16 = cn_var_实例;
  r17 = 检查(r16, 0);
  if (r17) goto if_then_1341; else goto if_else_1342;

  if_merge_1340:
  r22 = cn_var_节点_0;
  return r22;

  if_then_1341:
  r18 = cn_var_实例;
  r19 = 解析如果语句(r18);
  goto if_merge_1343;

  if_else_1342:
  r20 = cn_var_实例;
  r21 = 解析块语句(r20);
  goto if_merge_1343;

  if_merge_1343:
  goto if_merge_1340;
  return NULL;
}

struct 语句节点* 解析当语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 块语句节点* r12;
  struct 语句节点* r13;
  _Bool r6;
  _Bool r10;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1344; else goto if_merge_1345;

  if_then_1344:
  return 0;
  goto if_merge_1345;

  if_merge_1345:
  struct 语句节点* cn_var_节点_0;
  创建当语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1346; else goto if_merge_1347;

  if_then_1346:
  return 0;
  goto if_merge_1347;

  if_merge_1347:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 69);
  r7 = cn_var_实例;
  r8 = 解析表达式(r7);
  r9 = cn_var_实例;
  r10 = 期望(r9, 70);
  r11 = cn_var_实例;
  r12 = 解析块语句(r11);
  r13 = cn_var_节点_0;
  return r13;
}

struct 语句节点* 解析循环语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r9, r10, r24, r31;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r15;
  struct 解析器* r17;
  struct 语句节点* r18;
  struct 解析器* r19;
  struct 语句节点* r20;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r25;
  struct 表达式节点* r26;
  struct 解析器* r27;
  struct 解析器* r29;
  struct 解析器* r32;
  struct 表达式节点* r33;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 块语句节点* r37;
  struct 语句节点* r38;
  _Bool r6;
  _Bool r8;
  _Bool r14;
  _Bool r16;
  _Bool r23;
  _Bool r28;
  _Bool r30;
  _Bool r35;
  struct 词元 r12;
  enum 词元类型枚举 r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1348; else goto if_merge_1349;

  if_then_1348:
  return 0;
  goto if_merge_1349;

  if_merge_1349:
  struct 语句节点* cn_var_节点_0;
  创建循环语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1350; else goto if_merge_1351;

  if_then_1350:
  return 0;
  goto if_merge_1351;

  if_merge_1351:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 69);
  r7 = cn_var_实例;
  r8 = 检查(r7, 75);
  r9 = !r8;
  if (r9) goto if_then_1352; else goto if_else_1353;

  if_then_1352:
  r11 = cn_var_实例;
  r12 = r11->当前词元;
  r13 = r12.类型;
  r14 = 是否类型关键字(r13);
  if (r14) goto logic_merge_1359; else goto logic_rhs_1358;

  if_else_1353:
  r21 = cn_var_实例;
  前进词元(r21);
  goto if_merge_1354;

  if_merge_1354:
  r22 = cn_var_实例;
  r23 = 检查(r22, 75);
  r24 = !r23;
  if (r24) goto if_then_1360; else goto if_merge_1361;

  if_then_1355:
  r17 = cn_var_实例;
  r18 = 解析变量声明语句(r17);
  goto if_merge_1357;

  if_else_1356:
  r19 = cn_var_实例;
  r20 = 解析表达式语句(r19);
  goto if_merge_1357;

  if_merge_1357:
  goto if_merge_1354;

  logic_rhs_1358:
  r15 = cn_var_实例;
  r16 = 检查(r15, 18);
  goto logic_merge_1359;

  logic_merge_1359:
  if (r16) goto if_then_1355; else goto if_else_1356;

  if_then_1360:
  r25 = cn_var_实例;
  r26 = 解析表达式(r25);
  goto if_merge_1361;

  if_merge_1361:
  r27 = cn_var_实例;
  r28 = 期望(r27, 75);
  r29 = cn_var_实例;
  r30 = 检查(r29, 70);
  r31 = !r30;
  if (r31) goto if_then_1362; else goto if_merge_1363;

  if_then_1362:
  r32 = cn_var_实例;
  r33 = 解析表达式(r32);
  goto if_merge_1363;

  if_merge_1363:
  r34 = cn_var_实例;
  r35 = 期望(r34, 70);
  r36 = cn_var_实例;
  r37 = 解析块语句(r36);
  r38 = cn_var_节点_0;
  return r38;
}

struct 语句节点* 解析返回语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 表达式节点* r9;
  struct 解析器* r10;
  struct 语句节点* r12;
  _Bool r6;
  _Bool r11;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1364; else goto if_merge_1365;

  if_then_1364:
  return 0;
  goto if_merge_1365;

  if_merge_1365:
  struct 语句节点* cn_var_节点_0;
  创建返回语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1366; else goto if_merge_1367;

  if_then_1366:
  return 0;
  goto if_merge_1367;

  if_merge_1367:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 检查(r5, 75);
  r7 = !r6;
  if (r7) goto if_then_1368; else goto if_merge_1369;

  if_then_1368:
  r8 = cn_var_实例;
  r9 = 解析表达式(r8);
  goto if_merge_1369;

  if_merge_1369:
  r10 = cn_var_实例;
  r11 = 期望(r10, 75);
  r12 = cn_var_节点_0;
  return r12;
}

struct 语句节点* 解析中断语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r7;
  _Bool r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1370; else goto if_merge_1371;

  if_then_1370:
  return 0;
  goto if_merge_1371;

  if_merge_1371:
  struct 语句节点* cn_var_节点_0;
  创建中断语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1372; else goto if_merge_1373;

  if_then_1372:
  return 0;
  goto if_merge_1373;

  if_merge_1373:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 75);
  r7 = cn_var_节点_0;
  return r7;
}

struct 语句节点* 解析继续语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r7;
  _Bool r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1374; else goto if_merge_1375;

  if_then_1374:
  return 0;
  goto if_merge_1375;

  if_merge_1375:
  struct 语句节点* cn_var_节点_0;
  创建继续语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1376; else goto if_merge_1377;

  if_then_1376:
  return 0;
  goto if_merge_1377;

  if_merge_1377:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 75);
  r7 = cn_var_节点_0;
  return r7;
}

struct 语句节点* 解析选择语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 表达式节点* r8;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  void* r14;
  struct 解析器* r15;
  struct 语句节点* r17;
  _Bool r6;
  _Bool r10;
  _Bool r12;
  _Bool r16;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1378; else goto if_merge_1379;

  if_then_1378:
  return 0;
  goto if_merge_1379;

  if_merge_1379:
  struct 语句节点* cn_var_节点_0;
  创建选择语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1380; else goto if_merge_1381;

  if_then_1380:
  return 0;
  goto if_merge_1381;

  if_merge_1381:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 期望(r5, 69);
  r7 = cn_var_实例;
  r8 = 解析表达式(r7);
  r9 = cn_var_实例;
  r10 = 期望(r9, 70);
  r11 = cn_var_实例;
  r12 = 期望(r11, 71);
  r13 = cn_var_实例;
  r14 = 解析情况列表(r13);
  r15 = cn_var_实例;
  r16 = 期望(r15, 72);
  r17 = cn_var_节点_0;
  return r17;
}

void* 解析情况列表(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r14, r15, r16, r19, r22, r25, r29, r34, r39, r44, r45, r48, r51, r54, r58, r63;
  struct 解析器* r0;
  void* r2;
  void* r3;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r11;
  struct 情况分支* r13;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 语句节点* r27;
  struct 语句节点* r28;
  struct 情况分支* r30;
  struct 语句节点* r31;
  void* r32;
  struct 情况分支* r33;
  struct 解析器* r35;
  struct 解析器* r37;
  struct 情况分支* r38;
  struct 解析器* r40;
  struct 表达式节点* r41;
  struct 解析器* r42;
  struct 解析器* r46;
  struct 解析器* r49;
  struct 解析器* r52;
  struct 解析器* r55;
  struct 语句节点* r56;
  struct 语句节点* r57;
  struct 情况分支* r59;
  struct 语句节点* r60;
  void* r61;
  struct 情况分支* r62;
  void* r64;
  _Bool r6;
  _Bool r9;
  _Bool r12;
  _Bool r18;
  _Bool r21;
  _Bool r24;
  _Bool r36;
  _Bool r43;
  _Bool r47;
  _Bool r50;
  _Bool r53;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1382; else goto if_merge_1383;

  if_then_1382:
  return 0;
  goto if_merge_1383;

  if_merge_1383:
  void* cn_var_数组_0;
  r2 = 创建数组(4);
  cn_var_数组_0 = r2;
  r3 = cn_var_数组_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1384; else goto if_merge_1385;

  if_then_1384:
  return 0;
  goto if_merge_1385;

  if_merge_1385:
  goto while_cond_1386;

  while_cond_1386:
  r5 = cn_var_实例;
  r6 = 检查(r5, 72);
  r7 = !r6;
  if (r7) goto while_body_1387; else goto while_exit_1388;

  while_body_1387:
  r8 = cn_var_实例;
  r9 = 检查(r8, 9);
  if (r9) goto if_then_1389; else goto if_else_1390;

  while_exit_1388:
  r64 = cn_var_数组_0;
  return r64;

  if_then_1389:
  r10 = cn_var_实例;
  前进词元(r10);
  r11 = cn_var_实例;
  r12 = 期望(r11, 78);
  struct 情况分支* cn_var_默认分支_1;
  创建情况分支();
  cn_var_默认分支_1 = /* NONE */;
  r13 = cn_var_默认分支_1;
  r14 = r13 != 0;
  if (r14) goto if_then_1392; else goto if_merge_1393;

  if_else_1390:
  r35 = cn_var_实例;
  r36 = 检查(r35, 8);
  if (r36) goto if_then_1403; else goto if_else_1404;

  if_merge_1391:
  goto while_cond_1386;

  if_then_1392:
  goto while_cond_1394;

  if_merge_1393:
  goto if_merge_1391;

  while_cond_1394:
  r17 = cn_var_实例;
  r18 = 检查(r17, 72);
  r19 = !r18;
  if (r19) goto logic_rhs_1399; else goto logic_merge_1400;

  while_body_1395:
  struct 语句节点* cn_var_语句_2;
  r26 = cn_var_实例;
  r27 = 解析语句(r26);
  cn_var_语句_2 = r27;
  r28 = cn_var_语句_2;
  r29 = r28 != 0;
  if (r29) goto if_then_1401; else goto if_merge_1402;

  while_exit_1396:
  r32 = cn_var_数组_0;
  r33 = cn_var_默认分支_1;
  r34 = 数组添加(r32, r33);
  goto if_merge_1393;

  logic_rhs_1397:
  r23 = cn_var_实例;
  r24 = 检查(r23, 9);
  r25 = !r24;
  goto logic_merge_1398;

  logic_merge_1398:
  if (r25) goto while_body_1395; else goto while_exit_1396;

  logic_rhs_1399:
  r20 = cn_var_实例;
  r21 = 检查(r20, 8);
  r22 = !r21;
  goto logic_merge_1400;

  logic_merge_1400:
  if (r22) goto logic_rhs_1397; else goto logic_merge_1398;

  if_then_1401:
  r30 = cn_var_默认分支_1;
  r31 = cn_var_语句_2;
  情况分支添加语句(r30, r31);
  goto if_merge_1402;

  if_merge_1402:
  goto while_cond_1394;

  if_then_1403:
  r37 = cn_var_实例;
  前进词元(r37);
  struct 情况分支* cn_var_当前情况_3;
  创建情况分支();
  cn_var_当前情况_3 = /* NONE */;
  r38 = cn_var_当前情况_3;
  r39 = r38 != 0;
  if (r39) goto if_then_1406; else goto if_merge_1407;

  if_else_1404:
  goto while_exit_1388;
  goto if_merge_1405;

  if_merge_1405:
  goto if_merge_1391;

  if_then_1406:
  r40 = cn_var_实例;
  r41 = 解析表达式(r40);
  r42 = cn_var_实例;
  r43 = 期望(r42, 78);
  goto while_cond_1408;

  if_merge_1407:
  goto if_merge_1405;

  while_cond_1408:
  r46 = cn_var_实例;
  r47 = 检查(r46, 72);
  r48 = !r47;
  if (r48) goto logic_rhs_1413; else goto logic_merge_1414;

  while_body_1409:
  struct 语句节点* cn_var_语句_4;
  r55 = cn_var_实例;
  r56 = 解析语句(r55);
  cn_var_语句_4 = r56;
  r57 = cn_var_语句_4;
  r58 = r57 != 0;
  if (r58) goto if_then_1415; else goto if_merge_1416;

  while_exit_1410:
  r61 = cn_var_数组_0;
  r62 = cn_var_当前情况_3;
  r63 = 数组添加(r61, r62);
  goto if_merge_1407;

  logic_rhs_1411:
  r52 = cn_var_实例;
  r53 = 检查(r52, 9);
  r54 = !r53;
  goto logic_merge_1412;

  logic_merge_1412:
  if (r54) goto while_body_1409; else goto while_exit_1410;

  logic_rhs_1413:
  r49 = cn_var_实例;
  r50 = 检查(r49, 8);
  r51 = !r50;
  goto logic_merge_1414;

  logic_merge_1414:
  if (r51) goto logic_rhs_1411; else goto logic_merge_1412;

  if_then_1415:
  r59 = cn_var_当前情况_3;
  r60 = cn_var_语句_4;
  情况分支添加语句(r59, r60);
  goto if_merge_1416;

  if_merge_1416:
  goto while_cond_1408;
  return NULL;
}

struct 语句节点* 解析尝试语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  char* r16;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 块语句节点* r6;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 类型节点* r13;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 块语句节点* r22;
  struct 解析器* r23;
  struct 解析器* r25;
  struct 解析器* r26;
  struct 块语句节点* r27;
  struct 语句节点* r28;
  _Bool r8;
  _Bool r11;
  _Bool r18;
  _Bool r20;
  _Bool r24;
  struct 词元 r15;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1417; else goto if_merge_1418;

  if_then_1417:
  return 0;
  goto if_merge_1418;

  if_merge_1418:
  struct 语句节点* cn_var_节点_0;
  创建尝试语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1419; else goto if_merge_1420;

  if_then_1419:
  return 0;
  goto if_merge_1420;

  if_merge_1420:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = 解析块语句(r5);
  r7 = cn_var_实例;
  r8 = 检查(r7, 37);
  if (r8) goto if_then_1421; else goto if_merge_1422;

  if_then_1421:
  r9 = cn_var_实例;
  前进词元(r9);
  r10 = cn_var_实例;
  r11 = 期望(r10, 69);
  r12 = cn_var_实例;
  r13 = 解析类型(r12);
  r14 = cn_var_实例;
  r15 = r14->当前词元;
  r16 = r15.值;
  r17 = cn_var_实例;
  r18 = 期望(r17, 40);
  r19 = cn_var_实例;
  r20 = 期望(r19, 70);
  r21 = cn_var_实例;
  r22 = 解析块语句(r21);
  goto if_merge_1422;

  if_merge_1422:
  r23 = cn_var_实例;
  r24 = 检查(r23, 39);
  if (r24) goto if_then_1423; else goto if_merge_1424;

  if_then_1423:
  r25 = cn_var_实例;
  前进词元(r25);
  r26 = cn_var_实例;
  r27 = 解析块语句(r26);
  goto if_merge_1424;

  if_merge_1424:
  r28 = cn_var_节点_0;
  return r28;
}

struct 语句节点* 解析抛出语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  char* r7;
  char* r14;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r16;
  struct 语句节点* r18;
  _Bool r9;
  _Bool r11;
  _Bool r17;
  struct 词元 r6;
  struct 词元 r13;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1425; else goto if_merge_1426;

  if_then_1425:
  return 0;
  goto if_merge_1426;

  if_merge_1426:
  struct 语句节点* cn_var_节点_0;
  创建抛出语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1427; else goto if_merge_1428;

  if_then_1427:
  return 0;
  goto if_merge_1428;

  if_merge_1428:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = 期望(r8, 43);
  r10 = cn_var_实例;
  r11 = 检查(r10, 43);
  if (r11) goto if_then_1429; else goto if_merge_1430;

  if_then_1429:
  r12 = cn_var_实例;
  r13 = r12->当前词元;
  r14 = r13.值;
  r15 = cn_var_实例;
  前进词元(r15);
  goto if_merge_1430;

  if_merge_1430:
  r16 = cn_var_实例;
  r17 = 期望(r16, 75);
  r18 = cn_var_节点_0;
  return r18;
}

struct 语句节点* 解析表达式语句(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 表达式节点* r5;
  struct 解析器* r6;
  struct 语句节点* r8;
  _Bool r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1431; else goto if_merge_1432;

  if_then_1431:
  return 0;
  goto if_merge_1432;

  if_merge_1432:
  struct 语句节点* cn_var_节点_0;
  创建表达式语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1433; else goto if_merge_1434;

  if_then_1433:
  return 0;
  goto if_merge_1434;

  if_merge_1434:
  r4 = cn_var_实例;
  r5 = 解析表达式(r4);
  r6 = cn_var_实例;
  r7 = 期望(r6, 75);
  r8 = cn_var_节点_0;
  return r8;
}

struct 语句节点* 解析变量声明语句(struct 解析器* cn_var_实例) {
  long long r1, r5, r7;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 声明节点* r3;
  struct 声明节点* r4;
  struct 语句节点* r6;
  struct 声明节点* r8;
  struct 语句节点* r9;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1435; else goto if_merge_1436;

  if_then_1435:
  return 0;
  goto if_merge_1436;

  if_merge_1436:
  struct 声明节点* cn_var_声明_0;
  r2 = cn_var_实例;
  r3 = 解析变量声明(r2);
  cn_var_声明_0 = r3;
  r4 = cn_var_声明_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1437; else goto if_merge_1438;

  if_then_1437:
  return 0;
  goto if_merge_1438;

  if_merge_1438:
  struct 语句节点* cn_var_节点_1;
  创建声明语句();
  cn_var_节点_1 = /* NONE */;
  r6 = cn_var_节点_1;
  r7 = r6 == 0;
  if (r7) goto if_then_1439; else goto if_merge_1440;

  if_then_1439:
  return 0;
  goto if_merge_1440;

  if_merge_1440:
  r8 = cn_var_声明_0;
  r9 = cn_var_节点_1;
  return r9;
}

