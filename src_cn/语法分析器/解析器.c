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
struct 源位置;
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
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
  long long r1, r2, r5, r10, r15;
  struct 扫描器* r0;
  void* r3;
  struct 解析器* r4;
  struct 扫描器* r6;
  struct 诊断集合* r7;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r16;
  struct 扫描器* r17;
  struct 扫描器* r19;
  struct 解析器* r21;
  _Bool r13;
  _Bool r14;
  struct 语法错误恢复上下文 r9;
  struct 语法错误恢复上下文 r12;
  struct 词元 r18;
  struct 词元 r20;

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
  r3 = 分配内存(r2);
  cn_var_实例_0 = r3;
  r4 = cn_var_实例_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1617; else goto if_merge_1618;

  if_then_1617:
  return 0;
  goto if_merge_1618;

  if_merge_1618:
  r6 = cn_var_扫描器实例;
  r7 = cn_var_诊断集合指针;
  _Bool cn_var_初始化结果_1;
  r8 = cn_var_实例_0;
  r9 = r8->恢复上下文;
  r11 = cn_var_实例_0;
  r12 = r11->恢复上下文;
  r10 = &r12;
  r13 = 初始化错误恢复(r10);
  cn_var_初始化结果_1 = r13;
  r14 = cn_var_初始化结果_1;
  r15 = r14 == 0;
  if (r15) goto if_then_1619; else goto if_merge_1620;

  if_then_1619:
  r16 = cn_var_实例_0;
  释放内存(r16);
  return 0;
  goto if_merge_1620;

  if_merge_1620:
  r17 = cn_var_扫描器实例;
  r18 = 下一个词元(r17);
  r19 = cn_var_扫描器实例;
  r20 = 下一个词元(r19);
  r21 = cn_var_实例_0;
  return r21;
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
  long long r1, r4, r9, r10, r14;
  struct 解析器* r0;
  struct 程序节点* r2;
  struct 程序节点* r3;
  struct 解析器* r5;
  struct 解析器* r11;
  struct 声明节点* r12;
  struct 声明节点* r13;
  struct 程序节点* r15;
  struct 声明节点* r16;
  struct 程序节点* r17;
  struct 词元 r6;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r8;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1623; else goto if_merge_1624;

  if_then_1623:
  return 0;
  goto if_merge_1624;

  if_merge_1624:
  struct 程序节点* cn_var_程序_0;
  r2 = 创建程序节点();
  cn_var_程序_0 = r2;
  r3 = cn_var_程序_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1625; else goto if_merge_1626;

  if_then_1625:
  return 0;
  goto if_merge_1626;

  if_merge_1626:
  goto while_cond_1627;

  while_cond_1627:
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.类型;
  r8 = cn_var_词元类型枚举;
  r9 = r8.结束;
  r10 = r7 != r9;
  if (r10) goto while_body_1628; else goto while_exit_1629;

  while_body_1628:
  struct 声明节点* cn_var_声明_1;
  r11 = cn_var_实例;
  r12 = 解析顶层声明(r11);
  cn_var_声明_1 = r12;
  r13 = cn_var_声明_1;
  r14 = r13 != 0;
  if (r14) goto if_then_1630; else goto if_merge_1631;

  while_exit_1629:
  r17 = cn_var_程序_0;
  return r17;

  if_then_1630:
  r15 = cn_var_程序_0;
  r16 = cn_var_声明_1;
  程序添加声明(r15, r16);
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
  struct 词元 r6;

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
  r6 = 下一个词元(r5);
  return;
}

_Bool 期望(struct 解析器* cn_var_实例, enum 词元类型枚举 cn_var_类型) {
  long long r1;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 解析器* r6;
  _Bool r4;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r7;

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
  r4 = 检查(r2, r3);
  if (r4) goto if_then_1636; else goto if_merge_1637;

  if_then_1636:
  r5 = cn_var_实例;
  前进词元(r5);
  return 1;
  goto if_merge_1637;

  if_merge_1637:
  r6 = cn_var_实例;
  r7 = cn_var_类型;
  报告错误期望(r6, r7);
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
  struct 解析器* r3;
  _Bool r2;
  enum 词元类型枚举 r1;

  entry:
  r0 = cn_var_实例;
  r1 = cn_var_类型;
  r2 = 检查(r0, r1);
  if (r2) goto if_then_1640; else goto if_merge_1641;

  if_then_1640:
  r3 = cn_var_实例;
  前进词元(r3);
  return 1;
  goto if_merge_1641;

  if_merge_1641:
  return 0;
}

_Bool 匹配多种(struct 解析器* cn_var_实例, enum 词元类型枚举* cn_var_类型数组, long long cn_var_数量) {
  long long r0, r1, r3, r5, r6, r7, r8, r9, r10, r13, r14, r17, r18;
  struct 解析器* r2;
  enum 词元类型枚举* r4;
  struct 解析器* r11;
  enum 词元类型枚举* r12;
  struct 解析器* r16;
  _Bool r15;

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
  r15 = 检查(r11, r14);
  if (r15) goto if_then_1652; else goto if_merge_1653;

  for_update_1650:
  r17 = cn_var_i_0;
  r18 = r17 + 1;
  cn_var_i_0 = r18;
  goto for_cond_1648;

  for_exit_1651:
  return 0;

  if_then_1652:
  r16 = cn_var_实例;
  前进词元(r16);
  return 1;
  goto if_merge_1653;

  if_merge_1653:
  goto for_update_1650;
  return 0;
}

void 报告错误(struct 解析器* cn_var_实例, enum 诊断错误码 cn_var_错误码, char* cn_var_消息) {
  long long r1, r3, r4, r7, r9, r14, r17;
  char* r18;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 诊断集合* r6;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r19;
  struct 诊断集合* r20;
  struct 源位置 r11;
  struct 词元 r13;
  struct 词元 r16;
  struct 诊断信息 r21;
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
  r11 = 创建未知位置();
  r12 = cn_var_实例;
  r13 = r12->当前词元;
  r14 = r13.行号;
  r15 = cn_var_实例;
  r16 = r15->当前词元;
  r17 = r16.列号;
  r18 = cn_var_消息;
  r19 = cn_var_实例;
  r20 = r19->诊断集合指针;
  r21 = cn_var_信息_0;
  诊断集合添加(r20, r21);
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
  long long r1, r4, r9, r10, r12, r13, r14, r17, r22, r27, r28, r32, r35, r40, r44, r49, r53, r59, r60, r61, r62, r63, r64, r65, r68, r69, r72, r73, r76, r77, r80, r81, r84, r85, r88, r89, r92, r93, r96, r97, r100;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r15;
  struct 解析器* r18;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r29;
  struct 解析器* r33;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 解析器* r42;
  struct 解析器* r45;
  struct 解析器* r47;
  struct 解析器* r51;
  struct 解析器* r54;
  struct 解析器* r56;
  struct 解析器* r98;
  struct 解析器* r101;
  struct 解析器* r103;
  _Bool r7;
  _Bool r41;
  _Bool r50;
  struct 语法错误恢复上下文 r3;
  struct 语法错误恢复上下文 r6;
  struct 语法错误恢复上下文 r16;
  struct 语法错误恢复上下文 r19;
  struct 词元 r21;
  struct 词元 r24;
  struct 词元 r30;
  struct 语法错误恢复上下文 r34;
  struct 语法错误恢复上下文 r37;
  struct 语法错误恢复上下文 r43;
  struct 语法错误恢复上下文 r46;
  struct 语法错误恢复上下文 r52;
  struct 语法错误恢复上下文 r55;
  struct 词元 r57;
  enum 词元类型枚举 r66;
  enum 词元类型枚举 r70;
  enum 词元类型枚举 r74;
  enum 词元类型枚举 r78;
  enum 词元类型枚举 r82;
  enum 词元类型枚举 r86;
  enum 词元类型枚举 r90;
  enum 词元类型枚举 r94;
  struct 语法错误恢复上下文 r99;
  struct 语法错误恢复上下文 r102;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r39;
  enum 词元类型枚举 r48;
  enum 词元类型枚举 r58;
  enum 词元类型枚举 r67;
  enum 词元类型枚举 r71;
  enum 词元类型枚举 r75;
  enum 词元类型枚举 r79;
  enum 词元类型枚举 r83;
  enum 词元类型枚举 r87;
  enum 词元类型枚举 r91;
  enum 词元类型枚举 r95;

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
  r7 = 应该抑制错误(r4);
  if (r7) goto if_then_1662; else goto if_merge_1663;

  if_then_1662:
  return 0;
  goto if_merge_1663;

  if_merge_1663:
  r8 = cn_var_实例;
  r9 = r8->恢复计数;
  r10 = r9 + 1;
  r11 = cn_var_实例;
  r12 = r11->恢复计数;
  r13 = cn_var_最大错误恢复次数;
  r14 = r12 > r13;
  if (r14) goto if_then_1664; else goto if_merge_1665;

  if_then_1664:
  return 0;
  goto if_merge_1665;

  if_merge_1665:
  r15 = cn_var_实例;
  r16 = r15->恢复上下文;
  r18 = cn_var_实例;
  r19 = r18->恢复上下文;
  r17 = &r19;
  r20 = cn_var_实例;
  r21 = r20->当前词元;
  r22 = r21.行号;
  记录错误(r17, r22);
  goto while_cond_1666;

  while_cond_1666:
  r23 = cn_var_实例;
  r24 = r23->当前词元;
  r25 = r24.类型;
  r26 = cn_var_词元类型枚举;
  r27 = r26.结束;
  r28 = r25 != r27;
  if (r28) goto while_body_1667; else goto while_exit_1668;

  while_body_1667:
  r29 = cn_var_实例;
  r30 = r29->当前词元;
  r31 = r30.类型;
  r32 = 是任意同步点(r31);
  if (r32) goto if_then_1669; else goto if_merge_1670;

  while_exit_1668:
  return 0;

  if_then_1669:
  r33 = cn_var_实例;
  r34 = r33->恢复上下文;
  r36 = cn_var_实例;
  r37 = r36->恢复上下文;
  r35 = &r37;
  重置错误计数(r35);
  return 1;
  goto if_merge_1670;

  if_merge_1670:
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.分号;
  r41 = 匹配(r38, r40);
  if (r41) goto if_then_1671; else goto if_merge_1672;

  if_then_1671:
  r42 = cn_var_实例;
  r43 = r42->恢复上下文;
  r45 = cn_var_实例;
  r46 = r45->恢复上下文;
  r44 = &r46;
  重置错误计数(r44);
  return 1;
  goto if_merge_1672;

  if_merge_1672:
  r47 = cn_var_实例;
  r48 = cn_var_词元类型枚举;
  r49 = r48.右大括号;
  r50 = 检查(r47, r49);
  if (r50) goto if_then_1673; else goto if_merge_1674;

  if_then_1673:
  r51 = cn_var_实例;
  r52 = r51->恢复上下文;
  r54 = cn_var_实例;
  r55 = r54->恢复上下文;
  r53 = &r55;
  重置错误计数(r53);
  return 1;
  goto if_merge_1674;

  if_merge_1674:
  enum 词元类型枚举 cn_var_类型_0;
  r56 = cn_var_实例;
  r57 = r56->当前词元;
  r58 = r57.类型;
  cn_var_类型_0 = r58;
  r66 = cn_var_类型_0;
  r67 = cn_var_词元类型枚举;
  r68 = r67.关键字_函数;
  r69 = r66 == r68;
  if (r69) goto logic_merge_1690; else goto logic_rhs_1689;

  if_then_1675:
  r98 = cn_var_实例;
  r99 = r98->恢复上下文;
  r101 = cn_var_实例;
  r102 = r101->恢复上下文;
  r100 = &r102;
  重置错误计数(r100);
  return 1;
  goto if_merge_1676;

  if_merge_1676:
  r103 = cn_var_实例;
  前进词元(r103);
  goto while_cond_1666;

  logic_rhs_1677:
  r94 = cn_var_类型_0;
  r95 = cn_var_词元类型枚举;
  r96 = r95.关键字_私有;
  r97 = r94 == r96;
  goto logic_merge_1678;

  logic_merge_1678:
  if (r97) goto if_then_1675; else goto if_merge_1676;

  logic_rhs_1679:
  r90 = cn_var_类型_0;
  r91 = cn_var_词元类型枚举;
  r92 = r91.关键字_公开;
  r93 = r90 == r92;
  goto logic_merge_1680;

  logic_merge_1680:
  if (r93) goto logic_merge_1678; else goto logic_rhs_1677;

  logic_rhs_1681:
  r86 = cn_var_类型_0;
  r87 = cn_var_词元类型枚举;
  r88 = r87.关键字_导入;
  r89 = r86 == r88;
  goto logic_merge_1682;

  logic_merge_1682:
  if (r89) goto logic_merge_1680; else goto logic_rhs_1679;

  logic_rhs_1683:
  r82 = cn_var_类型_0;
  r83 = cn_var_词元类型枚举;
  r84 = r83.关键字_接口;
  r85 = r82 == r84;
  goto logic_merge_1684;

  logic_merge_1684:
  if (r85) goto logic_merge_1682; else goto logic_rhs_1681;

  logic_rhs_1685:
  r78 = cn_var_类型_0;
  r79 = cn_var_词元类型枚举;
  r80 = r79.关键字_枚举;
  r81 = r78 == r80;
  goto logic_merge_1686;

  logic_merge_1686:
  if (r81) goto logic_merge_1684; else goto logic_rhs_1683;

  logic_rhs_1687:
  r74 = cn_var_类型_0;
  r75 = cn_var_词元类型枚举;
  r76 = r75.关键字_结构体;
  r77 = r74 == r76;
  goto logic_merge_1688;

  logic_merge_1688:
  if (r77) goto logic_merge_1686; else goto logic_rhs_1685;

  logic_rhs_1689:
  r70 = cn_var_类型_0;
  r71 = cn_var_词元类型枚举;
  r72 = r71.关键字_类;
  r73 = r70 == r72;
  goto logic_merge_1690;

  logic_merge_1690:
  if (r73) goto logic_merge_1688; else goto logic_rhs_1687;
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

