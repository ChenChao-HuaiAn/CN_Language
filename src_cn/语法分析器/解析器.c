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

// Global Variables

// Forward Declarations
struct 解析器* 创建解析器(struct 扫描器*, struct 诊断集合*);
void 销毁解析器(struct 解析器*);
struct 程序节点* 解析程序(struct 解析器*);
void 前进词元(struct 解析器*);
_Bool 期望(struct 解析器*, enum 词元类型枚举);
_Bool 检查(struct 解析器*, enum 词元类型枚举);
_Bool 匹配(struct 解析器*, enum 词元类型枚举);
_Bool 匹配多种(struct 解析器*, enum 词元类型枚举*, long long);
void 报告错误(struct 解析器*, enum 诊断错误码, char*);
void 报告错误期望(struct 解析器*, enum 词元类型枚举);
_Bool 同步恢复(struct 解析器*);
_Bool 是否类型关键字(enum 词元类型枚举);

struct 解析器* 创建解析器(struct 扫描器* cn_var_扫描器实例, struct 诊断集合* cn_var_诊断集合指针) {
  long long r1, r2, r4, r9, r13;
  struct 扫描器* r0;
  struct 解析器* r3;
  struct 扫描器* r5;
  struct 诊断集合* r6;
  struct 解析器* r7;
  struct 解析器* r10;
  struct 解析器* r14;
  struct 扫描器* r15;
  struct 扫描器* r16;
  struct 解析器* r17;
  _Bool r12;
  struct 语法错误恢复上下文 r8;
  struct 语法错误恢复上下文 r11;

  entry:
  r0 = cn_var_扫描器实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1615; else goto if_merge_1616;

  if_then_1615:
  return 0;
  goto if_merge_1616;

  if_merge_1616:
  struct 解析器* cn_var_实例_0;
  r2 = cn_var_解析器大小;
  分配内存(r2);
  cn_var_实例_0 = /* NONE */;
  r3 = cn_var_实例_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1617; else goto if_merge_1618;

  if_then_1617:
  return 0;
  goto if_merge_1618;

  if_merge_1618:
  r5 = cn_var_扫描器实例;
  r6 = cn_var_诊断集合指针;
  _Bool cn_var_初始化结果_1;
  r7 = cn_var_实例_0;
  r8 = r7->恢复上下文;
  r10 = cn_var_实例_0;
  r11 = r10->恢复上下文;
  r9 = &r11;
  初始化错误恢复(r9);
  cn_var_初始化结果_1 = /* NONE */;
  r12 = cn_var_初始化结果_1;
  r13 = r12 == 0;
  if (r13) goto if_then_1619; else goto if_merge_1620;

  if_then_1619:
  r14 = cn_var_实例_0;
  释放内存(r14);
  return 0;
  goto if_merge_1620;

  if_merge_1620:
  r15 = cn_var_扫描器实例;
  下一个词元(r15);
  r16 = cn_var_扫描器实例;
  下一个词元(r16);
  r17 = cn_var_实例_0;
  return r17;
}

void 销毁解析器(struct 解析器* cn_var_实例) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;

  entry:
  r0 = cn_var_实例;
  r1 = r0 != 0;
  if (r1) goto if_then_1621; else goto if_merge_1622;

  if_then_1621:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_1622;

  if_merge_1622:
  return;
}

struct 程序节点* 解析程序(struct 解析器* cn_var_实例) {
  long long r1, r3, r8, r9, r12;
  struct 解析器* r0;
  struct 程序节点* r2;
  struct 解析器* r4;
  struct 解析器* r10;
  struct 声明节点* r11;
  struct 程序节点* r13;
  struct 声明节点* r14;
  struct 程序节点* r15;
  struct 词元 r5;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r7;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1623; else goto if_merge_1624;

  if_then_1623:
  return 0;
  goto if_merge_1624;

  if_merge_1624:
  struct 程序节点* cn_var_程序_0;
  创建程序节点();
  cn_var_程序_0 = /* NONE */;
  r2 = cn_var_程序_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1625; else goto if_merge_1626;

  if_then_1625:
  return 0;
  goto if_merge_1626;

  if_merge_1626:
  goto while_cond_1627;

  while_cond_1627:
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.类型;
  r7 = cn_var_词元类型枚举;
  r8 = r7.结束;
  r9 = r6 != r8;
  if (r9) goto while_body_1628; else goto while_exit_1629;

  while_body_1628:
  struct 声明节点* cn_var_声明_1;
  r10 = cn_var_实例;
  解析顶层声明(r10);
  cn_var_声明_1 = /* NONE */;
  r11 = cn_var_声明_1;
  r12 = r11 != 0;
  if (r12) goto if_then_1630; else goto if_merge_1631;

  while_exit_1629:
  r15 = cn_var_程序_0;
  return r15;

  if_then_1630:
  r13 = cn_var_程序_0;
  r14 = cn_var_声明_1;
  程序添加声明(r13, r14);
  goto if_merge_1631;

  if_merge_1631:
  goto while_cond_1627;
  return NULL;
}

void 前进词元(struct 解析器* cn_var_实例) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r4;
  struct 扫描器* r5;
  struct 词元 r3;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1632; else goto if_merge_1633;

  if_then_1632:
  return;
  goto if_merge_1633;

  if_merge_1633:
  r2 = cn_var_实例;
  r3 = r2->下一个词元;
  r4 = cn_var_实例;
  r5 = r4->扫描器实例;
  下一个词元(r5);
  return;
}

_Bool 期望(struct 解析器* cn_var_实例, enum 词元类型枚举 cn_var_类型) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1634; else goto if_merge_1635;

  if_then_1634:
  return 0;
  goto if_merge_1635;

  if_merge_1635:
  r2 = cn_var_实例;
  r3 = cn_var_类型;
  检查(r2, r3);
  goto if_then_1636;

  if_then_1636:
  r4 = cn_var_实例;
  前进词元(r4);
  return 1;
  goto if_merge_1637;

  if_merge_1637:
  r5 = cn_var_实例;
  r6 = cn_var_类型;
  报告错误期望(r5, r6);
  return 0;
}

_Bool 检查(struct 解析器* cn_var_实例, enum 词元类型枚举 cn_var_类型) {
  long long r1, r6;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 词元 r3;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r5;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1638; else goto if_merge_1639;

  if_then_1638:
  return 0;
  goto if_merge_1639;

  if_merge_1639:
  r2 = cn_var_实例;
  r3 = r2->当前词元;
  r4 = r3.类型;
  r5 = cn_var_类型;
  r6 = r4 == r5;
  return r6;
}

_Bool 匹配(struct 解析器* cn_var_实例, enum 词元类型枚举 cn_var_类型) {
  struct 解析器* r0;
  struct 解析器* r2;
  enum 词元类型枚举 r1;

  entry:
  r0 = cn_var_实例;
  r1 = cn_var_类型;
  检查(r0, r1);
  goto if_then_1640;

  if_then_1640:
  r2 = cn_var_实例;
  前进词元(r2);
  return 1;
  goto if_merge_1641;

  if_merge_1641:
  return 0;
}

_Bool 匹配多种(struct 解析器* cn_var_实例, enum 词元类型枚举* cn_var_类型数组, long long cn_var_数量) {
  long long r0, r1, r3, r5, r6, r7, r8, r9, r10, r13, r14, r16, r17;
  struct 解析器* r2;
  enum 词元类型枚举* r4;
  struct 解析器* r11;
  enum 词元类型枚举* r12;
  struct 解析器* r15;

  entry:
  r2 = cn_var_实例;
  r3 = r2 == 0;
  if (r3) goto logic_merge_1647; else goto logic_rhs_1646;

  if_then_1642:
  return 0;
  goto if_merge_1643;

  if_merge_1643:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_1648;

  logic_rhs_1644:
  r6 = cn_var_数量;
  r7 = r6 <= 0;
  goto logic_merge_1645;

  logic_merge_1645:
  if (r7) goto if_then_1642; else goto if_merge_1643;

  logic_rhs_1646:
  r4 = cn_var_类型数组;
  r5 = r4 == 0;
  goto logic_merge_1647;

  logic_merge_1647:
  if (r5) goto logic_merge_1645; else goto logic_rhs_1644;

  for_cond_1648:
  r8 = cn_var_i_0;
  r9 = cn_var_数量;
  r10 = r8 < r9;
  if (r10) goto for_body_1649; else goto for_exit_1651;

  for_body_1649:
  r11 = cn_var_实例;
  r12 = cn_var_类型数组;
  r13 = cn_var_i_0;
  r14 = *(void*)cn_rt_array_get_element(r12, r13, 8);
  检查(r11, r14);
  goto if_then_1652;

  for_update_1650:
  r16 = cn_var_i_0;
  r17 = r16 + 1;
  cn_var_i_0 = r17;
  goto for_cond_1648;

  for_exit_1651:
  return 0;

  if_then_1652:
  r15 = cn_var_实例;
  前进词元(r15);
  return 1;
  goto if_merge_1653;

  if_merge_1653:
  goto for_update_1650;
  return 0;
}

void 报告错误(struct 解析器* cn_var_实例, enum 诊断错误码 cn_var_错误码, char* cn_var_消息) {
  long long r1, r3, r4, r7, r9, r13, r16;
  char* r17;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 诊断集合* r6;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r18;
  struct 诊断集合* r19;
  struct 词元 r12;
  struct 词元 r15;
  struct 诊断信息 r20;
  enum 诊断严重级别 r8;
  enum 诊断错误码 r10;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1654; else goto if_merge_1655;

  if_then_1654:
  return;
  goto if_merge_1655;

  if_merge_1655:
  r2 = cn_var_实例;
  r3 = r2->错误计数;
  r4 = r3 + 1;
  r5 = cn_var_实例;
  r6 = r5->诊断集合指针;
  r7 = r6 != 0;
  if (r7) goto if_then_1656; else goto if_merge_1657;

  if_then_1656:
  struct 诊断信息 cn_var_信息_0;
  r8 = cn_var_诊断严重级别;
  r9 = r8.错误;
  r10 = cn_var_错误码;
  创建未知位置();
  r11 = cn_var_实例;
  r12 = r11->当前词元;
  r13 = r12.行号;
  r14 = cn_var_实例;
  r15 = r14->当前词元;
  r16 = r15.列号;
  r17 = cn_var_消息;
  r18 = cn_var_实例;
  r19 = r18->诊断集合指针;
  r20 = cn_var_信息_0;
  诊断集合添加(r19, r20);
  goto if_merge_1657;

  if_merge_1657:
  return;
}

void 报告错误期望(struct 解析器* cn_var_实例, enum 词元类型枚举 cn_var_期望类型) {
  long long r1, r4;
  struct 解析器* r0;
  struct 解析器* r2;
  enum 诊断错误码 r3;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1658; else goto if_merge_1659;

  if_then_1658:
  return;
  goto if_merge_1659;

  if_merge_1659:
  r2 = cn_var_实例;
  r3 = cn_var_诊断错误码;
  r4 = r3.语法_期望标记;
  报告错误(r2, r4, "期望的词元类型不匹配");
  return;
}

_Bool 同步恢复(struct 解析器* cn_var_实例) {
  long long r1, r4, r8, r9, r11, r12, r13, r16, r21, r26, r27, r33, r38, r41, r46, r49, r55, r56, r57, r58, r59, r60, r61, r64, r65, r68, r69, r72, r73, r76, r77, r80, r81, r84, r85, r88, r89, r92, r93, r96;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 解析器* r10;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r22;
  struct 解析器* r28;
  struct 解析器* r31;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r39;
  struct 解析器* r42;
  struct 解析器* r44;
  struct 解析器* r47;
  struct 解析器* r50;
  struct 解析器* r52;
  struct 解析器* r94;
  struct 解析器* r97;
  struct 解析器* r99;
  struct 语法错误恢复上下文 r3;
  struct 语法错误恢复上下文 r6;
  struct 语法错误恢复上下文 r15;
  struct 语法错误恢复上下文 r18;
  struct 词元 r20;
  struct 词元 r23;
  struct 词元 r29;
  struct 语法错误恢复上下文 r32;
  struct 语法错误恢复上下文 r35;
  struct 语法错误恢复上下文 r40;
  struct 语法错误恢复上下文 r43;
  struct 语法错误恢复上下文 r48;
  struct 语法错误恢复上下文 r51;
  struct 词元 r53;
  enum 词元类型枚举 r62;
  enum 词元类型枚举 r66;
  enum 词元类型枚举 r70;
  enum 词元类型枚举 r74;
  enum 词元类型枚举 r78;
  enum 词元类型枚举 r82;
  enum 词元类型枚举 r86;
  enum 词元类型枚举 r90;
  struct 语法错误恢复上下文 r95;
  struct 语法错误恢复上下文 r98;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r30;
  enum 词元类型枚举 r37;
  enum 词元类型枚举 r45;
  enum 词元类型枚举 r54;
  enum 词元类型枚举 r63;
  enum 词元类型枚举 r67;
  enum 词元类型枚举 r71;
  enum 词元类型枚举 r75;
  enum 词元类型枚举 r79;
  enum 词元类型枚举 r83;
  enum 词元类型枚举 r87;
  enum 词元类型枚举 r91;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1660; else goto if_merge_1661;

  if_then_1660:
  return 0;
  goto if_merge_1661;

  if_merge_1661:
  r2 = cn_var_实例;
  r3 = r2->恢复上下文;
  r5 = cn_var_实例;
  r6 = r5->恢复上下文;
  r4 = &r6;
  应该抑制错误(r4);
  goto if_then_1662;

  if_then_1662:
  return 0;
  goto if_merge_1663;

  if_merge_1663:
  r7 = cn_var_实例;
  r8 = r7->恢复计数;
  r9 = r8 + 1;
  r10 = cn_var_实例;
  r11 = r10->恢复计数;
  r12 = cn_var_最大错误恢复次数;
  r13 = r11 > r12;
  if (r13) goto if_then_1664; else goto if_merge_1665;

  if_then_1664:
  return 0;
  goto if_merge_1665;

  if_merge_1665:
  r14 = cn_var_实例;
  r15 = r14->恢复上下文;
  r17 = cn_var_实例;
  r18 = r17->恢复上下文;
  r16 = &r18;
  r19 = cn_var_实例;
  r20 = r19->当前词元;
  r21 = r20.行号;
  记录错误(r16, r21);
  goto while_cond_1666;

  while_cond_1666:
  r22 = cn_var_实例;
  r23 = r22->当前词元;
  r24 = r23.类型;
  r25 = cn_var_词元类型枚举;
  r26 = r25.结束;
  r27 = r24 != r26;
  if (r27) goto while_body_1667; else goto while_exit_1668;

  while_body_1667:
  r28 = cn_var_实例;
  r29 = r28->当前词元;
  r30 = r29.类型;
  是任意同步点(r30);
  goto if_then_1669;

  while_exit_1668:
  return 0;

  if_then_1669:
  r31 = cn_var_实例;
  r32 = r31->恢复上下文;
  r34 = cn_var_实例;
  r35 = r34->恢复上下文;
  r33 = &r35;
  重置错误计数(r33);
  return 1;
  goto if_merge_1670;

  if_merge_1670:
  r36 = cn_var_实例;
  r37 = cn_var_词元类型枚举;
  r38 = r37.分号;
  匹配(r36, r38);
  goto if_then_1671;

  if_then_1671:
  r39 = cn_var_实例;
  r40 = r39->恢复上下文;
  r42 = cn_var_实例;
  r43 = r42->恢复上下文;
  r41 = &r43;
  重置错误计数(r41);
  return 1;
  goto if_merge_1672;

  if_merge_1672:
  r44 = cn_var_实例;
  r45 = cn_var_词元类型枚举;
  r46 = r45.右大括号;
  检查(r44, r46);
  goto if_then_1673;

  if_then_1673:
  r47 = cn_var_实例;
  r48 = r47->恢复上下文;
  r50 = cn_var_实例;
  r51 = r50->恢复上下文;
  r49 = &r51;
  重置错误计数(r49);
  return 1;
  goto if_merge_1674;

  if_merge_1674:
  enum 词元类型枚举 cn_var_类型_0;
  r52 = cn_var_实例;
  r53 = r52->当前词元;
  r54 = r53.类型;
  cn_var_类型_0 = r54;
  r62 = cn_var_类型_0;
  r63 = cn_var_词元类型枚举;
  r64 = r63.关键字_函数;
  r65 = r62 == r64;
  if (r65) goto logic_merge_1690; else goto logic_rhs_1689;

  if_then_1675:
  r94 = cn_var_实例;
  r95 = r94->恢复上下文;
  r97 = cn_var_实例;
  r98 = r97->恢复上下文;
  r96 = &r98;
  重置错误计数(r96);
  return 1;
  goto if_merge_1676;

  if_merge_1676:
  r99 = cn_var_实例;
  前进词元(r99);
  goto while_cond_1666;

  logic_rhs_1677:
  r90 = cn_var_类型_0;
  r91 = cn_var_词元类型枚举;
  r92 = r91.关键字_私有;
  r93 = r90 == r92;
  goto logic_merge_1678;

  logic_merge_1678:
  if (r93) goto if_then_1675; else goto if_merge_1676;

  logic_rhs_1679:
  r86 = cn_var_类型_0;
  r87 = cn_var_词元类型枚举;
  r88 = r87.关键字_公开;
  r89 = r86 == r88;
  goto logic_merge_1680;

  logic_merge_1680:
  if (r89) goto logic_merge_1678; else goto logic_rhs_1677;

  logic_rhs_1681:
  r82 = cn_var_类型_0;
  r83 = cn_var_词元类型枚举;
  r84 = r83.关键字_导入;
  r85 = r82 == r84;
  goto logic_merge_1682;

  logic_merge_1682:
  if (r85) goto logic_merge_1680; else goto logic_rhs_1679;

  logic_rhs_1683:
  r78 = cn_var_类型_0;
  r79 = cn_var_词元类型枚举;
  r80 = r79.关键字_接口;
  r81 = r78 == r80;
  goto logic_merge_1684;

  logic_merge_1684:
  if (r81) goto logic_merge_1682; else goto logic_rhs_1681;

  logic_rhs_1685:
  r74 = cn_var_类型_0;
  r75 = cn_var_词元类型枚举;
  r76 = r75.关键字_枚举;
  r77 = r74 == r76;
  goto logic_merge_1686;

  logic_merge_1686:
  if (r77) goto logic_merge_1684; else goto logic_rhs_1683;

  logic_rhs_1687:
  r70 = cn_var_类型_0;
  r71 = cn_var_词元类型枚举;
  r72 = r71.关键字_结构体;
  r73 = r70 == r72;
  goto logic_merge_1688;

  logic_merge_1688:
  if (r73) goto logic_merge_1686; else goto logic_rhs_1685;

  logic_rhs_1689:
  r66 = cn_var_类型_0;
  r67 = cn_var_词元类型枚举;
  r68 = r67.关键字_类;
  r69 = r66 == r68;
  goto logic_merge_1690;

  logic_merge_1690:
  if (r69) goto logic_merge_1688; else goto logic_rhs_1687;
  return 0;
}

_Bool 是否类型关键字(enum 词元类型枚举 cn_var_类型) {
  long long r0, r1, r2, r3, r4, r5, r6, r9, r10, r13, r14, r17, r18, r21, r22, r25, r26, r29, r30, r33, r34, r37, r38;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r36;

  entry:
  r7 = cn_var_类型;
  r8 = cn_var_词元类型枚举;
  r9 = r8.关键字_整数;
  r10 = r7 == r9;
  if (r10) goto logic_merge_1704; else goto logic_rhs_1703;

  logic_rhs_1691:
  r35 = cn_var_类型;
  r36 = cn_var_词元类型枚举;
  r37 = r36.标识符;
  r38 = r35 == r37;
  goto logic_merge_1692;

  logic_merge_1692:
  return r38;

  logic_rhs_1693:
  r31 = cn_var_类型;
  r32 = cn_var_词元类型枚举;
  r33 = r32.关键字_枚举;
  r34 = r31 == r33;
  goto logic_merge_1694;

  logic_merge_1694:
  if (r34) goto logic_merge_1692; else goto logic_rhs_1691;

  logic_rhs_1695:
  r27 = cn_var_类型;
  r28 = cn_var_词元类型枚举;
  r29 = r28.关键字_结构体;
  r30 = r27 == r29;
  goto logic_merge_1696;

  logic_merge_1696:
  if (r30) goto logic_merge_1694; else goto logic_rhs_1693;

  logic_rhs_1697:
  r23 = cn_var_类型;
  r24 = cn_var_词元类型枚举;
  r25 = r24.关键字_空类型;
  r26 = r23 == r25;
  goto logic_merge_1698;

  logic_merge_1698:
  if (r26) goto logic_merge_1696; else goto logic_rhs_1695;

  logic_rhs_1699:
  r19 = cn_var_类型;
  r20 = cn_var_词元类型枚举;
  r21 = r20.关键字_布尔;
  r22 = r19 == r21;
  goto logic_merge_1700;

  logic_merge_1700:
  if (r22) goto logic_merge_1698; else goto logic_rhs_1697;

  logic_rhs_1701:
  r15 = cn_var_类型;
  r16 = cn_var_词元类型枚举;
  r17 = r16.关键字_字符串;
  r18 = r15 == r17;
  goto logic_merge_1702;

  logic_merge_1702:
  if (r18) goto logic_merge_1700; else goto logic_rhs_1699;

  logic_rhs_1703:
  r11 = cn_var_类型;
  r12 = cn_var_词元类型枚举;
  r13 = r12.关键字_小数;
  r14 = r11 == r13;
  goto logic_merge_1704;

  logic_merge_1704:
  if (r14) goto logic_merge_1702; else goto logic_rhs_1701;
  return 0;
}

