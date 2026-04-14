#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 诊断严重级别 {
    诊断严重级别_诊断_错误 = 0,
    诊断严重级别_诊断_警告 = 1
};

enum 诊断错误码 {
    诊断错误码_未知错误 = 0,
    诊断错误码_词法_未终止字符串 = 1,
    诊断错误码_词法_无效字符 = 2,
    诊断错误码_词法_无效十六进制 = 3,
    诊断错误码_词法_无效二进制 = 4,
    诊断错误码_词法_无效八进制 = 5,
    诊断错误码_词法_字面量溢出 = 6,
    诊断错误码_词法_未终止块注释 = 7,
    诊断错误码_语法_期望标记 = 11,
    诊断错误码_语法_预留特性 = 12,
    诊断错误码_语法_无效函数名 = 13,
    诊断错误码_语法_无效参数 = 14,
    诊断错误码_语法_无效变量声明 = 15,
    诊断错误码_语法_无效比较运算符 = 16,
    诊断错误码_语法_无效表达式 = 17,
    诊断错误码_语义_重复符号 = 21,
    诊断错误码_语义_未定义标识符 = 22,
    诊断错误码_语义_类型不匹配 = 23,
    诊断错误码_语义_参数数量不匹配 = 24,
    诊断错误码_语义_参数类型不匹配 = 25,
    诊断错误码_语义_返回语句在函数外 = 26,
    诊断错误码_语义_中断继续在循环外 = 27,
    诊断错误码_语义_缺少返回语句 = 28,
    诊断错误码_语义_无效赋值 = 29,
    诊断错误码_语义_不可调用 = 30,
    诊断错误码_语义_非结构体类型 = 31,
    诊断错误码_语义_成员未找到 = 32,
    诊断错误码_语义_访问被拒绝 = 33,
    诊断错误码_语义_常量缺少初始化 = 34,
    诊断错误码_语义_常量非常量初始化 = 35,
    诊断错误码_语义_开关非常量情况 = 36,
    诊断错误码_语义_开关重复情况 = 37,
    诊断错误码_语义_静态非常量初始化 = 38,
    诊断错误码_语义_静态空类型 = 39,
    诊断错误码_语义_无效自身使用 = 40,
    诊断错误码_语义_抽象类实例化 = 41,
    诊断错误码_语义_纯虚函数未实现 = 42,
    诊断错误码_语义_纯虚函数调用 = 43
};

// CN Language Global Struct Forward Declarations
struct 源位置;
struct 诊断信息;
struct 诊断集合;

// CN Language Global Struct Definitions
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};

struct 诊断信息 {
    enum 诊断严重级别 严重级别;
    enum 诊断错误码 错误码;
    struct 源位置 位置;
    char* 消息;
};

struct 诊断集合 {
    struct 诊断信息* 诊断数组;
    long long 容量;
    long long 长度;
    long long 错误计数;
    long long 警告计数;
    long long 最大错误数;
};

// Global Variables

// Forward Declarations
struct 诊断集合* 创建诊断集合(long long);
void 释放诊断集合(struct 诊断集合*);
_Bool 扩展诊断数组(struct 诊断集合*);
void 报告诊断(struct 诊断集合*, enum 诊断严重级别, enum 诊断错误码, struct 源位置, const char*);
void 报告错误(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
void 报告警告(struct 诊断集合*, enum 诊断错误码, struct 源位置, const char*);
_Bool 有错误(struct 诊断集合*);
long long 获取错误计数(struct 诊断集合*);
long long 获取警告计数(struct 诊断集合*);
_Bool 应该继续(struct 诊断集合*);
char* 获取严重级别字符串(enum 诊断严重级别);
void 打印诊断信息(struct 诊断信息*);
void 打印所有诊断(struct 诊断集合*);
void 清空诊断集合(struct 诊断集合*);
struct 源位置 创建源位置(const char*, long long, long long);
struct 源位置 创建未知位置();
void 报告重复符号错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, const char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, const char*, const char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, const char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, const char*);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, const char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, const char*);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);

struct 诊断集合* 创建诊断集合(long long cn_var_初始容量) {
  long long r0, r1, r4, r5, r7, r8, r10;
  void* r2;
  struct 诊断集合* r3;
  void* r6;
  struct 诊断集合* r9;
  struct 诊断集合* r11;

  entry:
  r0 = cn_var_初始容量;
  r1 = r0 <= 0;
  if (r1) goto if_then_815; else goto if_merge_816;

  if_then_815:
  cn_var_初始容量 = 4;
  goto if_merge_816;

  if_merge_816:
  struct 诊断集合* cn_var_集合_0;
  r2 = 分配内存(32);
  cn_var_集合_0 = r2;
  r3 = cn_var_集合_0;
  r4 = r3 == 0;
  if (r4) goto if_then_817; else goto if_merge_818;

  if_then_817:
  return 0;
  goto if_merge_818;

  if_merge_818:
  r5 = cn_var_初始容量;
  r6 = 分配内存数组(48, r5);
  r7 = cn_var_集合_0->诊断数组;
  r8 = r7 == 0;
  if (r8) goto if_then_819; else goto if_merge_820;

  if_then_819:
  r9 = cn_var_集合_0;
  释放内存(r9);
  return 0;
  goto if_merge_820;

  if_merge_820:
  r10 = cn_var_初始容量;
  r11 = cn_var_集合_0;
  return r11;
}

void 释放诊断集合(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r3, r4;
  struct 诊断集合* r0;
  struct 诊断集合* r5;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_821; else goto if_merge_822;

  if_then_821:
  return;
  goto if_merge_822;

  if_merge_822:
  r2 = cn_var_集合.诊断数组;
  r3 = r2 != 0;
  if (r3) goto if_then_823; else goto if_merge_824;

  if_then_823:
  r4 = cn_var_集合.诊断数组;
  释放内存(r4);
  goto if_merge_824;

  if_merge_824:
  r5 = cn_var_集合;
  释放内存(r5);
  return;
}

_Bool 扩展诊断数组(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r11, r13;
  struct 诊断集合* r0;
  void* r9;
  struct 诊断信息* r10;
  struct 诊断信息* r12;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_825; else goto if_merge_826;

  if_then_825:
  return 0;
  goto if_merge_826;

  if_merge_826:
  long long cn_var_新容量_0;
  r2 = cn_var_集合.容量;
  r3 = r2 << 1;
  cn_var_新容量_0 = r3;
  r4 = cn_var_新容量_0;
  r5 = r4 <= 0;
  if (r5) goto if_then_827; else goto if_merge_828;

  if_then_827:
  cn_var_新容量_0 = 4;
  goto if_merge_828;

  if_merge_828:
  struct 诊断信息* cn_var_新数组_1;
  r6 = cn_var_集合.诊断数组;
  r7 = cn_var_新容量_0;
  r8 = r7 * 48;
  r9 = 重新分配内存(r6, r8);
  cn_var_新数组_1 = r9;
  r10 = cn_var_新数组_1;
  r11 = r10 == 0;
  if (r11) goto if_then_829; else goto if_merge_830;

  if_then_829:
  return 0;
  goto if_merge_830;

  if_merge_830:
  r12 = cn_var_新数组_1;
  r13 = cn_var_新容量_0;
  return 1;
}

void 报告诊断(struct 诊断集合* cn_var_集合, enum 诊断严重级别 cn_var_级别, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, const char* cn_var_消息) {
  long long r1, r2, r3, r4, r7, r8, r9, r10, r15, r16, r18, r19, r20, r21, r22;
  char* r14;
  struct 诊断集合* r0;
  struct 诊断集合* r5;
  _Bool r6;
  struct 源位置 r13;
  enum 诊断严重级别 r11;
  enum 诊断错误码 r12;
  enum 诊断严重级别 r17;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_831; else goto if_merge_832;

  if_then_831:
  return;
  goto if_merge_832;

  if_merge_832:
  r2 = cn_var_集合.长度;
  r3 = cn_var_集合.容量;
  r4 = r2 >= r3;
  if (r4) goto if_then_833; else goto if_merge_834;

  if_then_833:
  r5 = cn_var_集合;
  r6 = 扩展诊断数组(r5);
  r7 = r6 == 0;
  if (r7) goto if_then_835; else goto if_merge_836;

  if_merge_834:
  struct 诊断信息* cn_var_信息_0;
  r8 = cn_var_集合.诊断数组;
  r9 = cn_var_集合.长度;
  r10 = r8 + r9;
  cn_var_信息_0 = r10;
  r11 = cn_var_级别;
  r12 = cn_var_代码;
  r13 = cn_var_位置;
  r14 = cn_var_消息;
  r15 = cn_var_集合.长度;
  r16 = r15 + 1;
  r17 = cn_var_级别;
  r18 = r17 == 诊断严重级别_诊断_错误;
  if (r18) goto if_then_837; else goto if_else_838;

  if_then_835:
  return;
  goto if_merge_836;

  if_merge_836:
  goto if_merge_834;

  if_then_837:
  r19 = cn_var_集合.错误计数;
  r20 = r19 + 1;
  goto if_merge_839;

  if_else_838:
  r21 = cn_var_集合.警告计数;
  r22 = r21 + 1;
  goto if_merge_839;

  if_merge_839:
  return;
}

void 报告错误(struct 诊断集合* cn_var_集合, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, const char* cn_var_消息) {
  char* r3;
  struct 诊断集合* r0;
  struct 源位置 r2;
  enum 诊断错误码 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_代码;
  r2 = cn_var_位置;
  r3 = cn_var_消息;
  报告诊断(r0, 诊断严重级别_诊断_错误, r1, r2, r3);
  return;
}

void 报告警告(struct 诊断集合* cn_var_集合, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, const char* cn_var_消息) {
  char* r3;
  struct 诊断集合* r0;
  struct 源位置 r2;
  enum 诊断错误码 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_代码;
  r2 = cn_var_位置;
  r3 = cn_var_消息;
  报告诊断(r0, 诊断严重级别_诊断_警告, r1, r2, r3);
  return;
}

_Bool 有错误(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r3;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_840; else goto if_merge_841;

  if_then_840:
  return 0;
  goto if_merge_841;

  if_merge_841:
  r2 = cn_var_集合.错误计数;
  r3 = r2 > 0;
  return r3;
}

long long 获取错误计数(struct 诊断集合* cn_var_集合) {
  long long r1, r2;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_842; else goto if_merge_843;

  if_then_842:
  return 0;
  goto if_merge_843;

  if_merge_843:
  r2 = cn_var_集合.错误计数;
  return r2;
}

long long 获取警告计数(struct 诊断集合* cn_var_集合) {
  long long r1, r2;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_844; else goto if_merge_845;

  if_then_844:
  return 0;
  goto if_merge_845;

  if_merge_845:
  r2 = cn_var_集合.警告计数;
  return r2;
}

_Bool 应该继续(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r3, r4, r5, r6;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_846; else goto if_merge_847;

  if_then_846:
  return 1;
  goto if_merge_847;

  if_merge_847:
  r2 = cn_var_集合.最大错误数;
  r3 = r2 > 0;
  if (r3) goto if_then_848; else goto if_merge_849;

  if_then_848:
  r4 = cn_var_集合.错误计数;
  r5 = cn_var_集合.最大错误数;
  r6 = r4 < r5;
  return r6;
  goto if_merge_849;

  if_merge_849:
  return 1;
}

char* 获取严重级别字符串(enum 诊断严重级别 cn_var_级别) {
  long long r1, r3;
  enum 诊断严重级别 r0;
  enum 诊断严重级别 r2;

  entry:
  r0 = cn_var_级别;
  r1 = r0 == 诊断严重级别_诊断_错误;
  if (r1) goto if_then_850; else goto if_else_851;

  if_then_850:
  return "错误";
  goto if_merge_852;

  if_else_851:
  r2 = cn_var_级别;
  r3 = r2 == 诊断严重级别_诊断_警告;
  if (r3) goto if_then_853; else goto if_else_854;

  if_merge_852:

  if_then_853:
  return "警告";
  goto if_merge_855;

  if_else_854:
  return "未知";
  goto if_merge_855;

  if_merge_855:
  goto if_merge_852;
  return NULL;
}

void 打印诊断信息(struct 诊断信息* cn_var_信息) {
  long long r1, r2, r4, r5, r6, r7, r8, r9, r10, r11, r12, r14, r16;
  char* r3;
  char* r13;
  char* r15;
  struct 诊断信息* r0;

  entry:
  r0 = cn_var_信息;
  r1 = r0 == 0;
  if (r1) goto if_then_856; else goto if_merge_857;

  if_then_856:
  return;
  goto if_merge_857;

  if_merge_857:
  char* cn_var_级别字符串_0;
  r2 = cn_var_信息.严重级别;
  r3 = 获取严重级别字符串(r2);
  cn_var_级别字符串_0 = r3;
  r4 = cn_var_信息.位置;
  r5 = r4.文件名;
  r6 = r5 != 0;
  if (r6) goto if_then_858; else goto if_else_859;

  if_then_858:
  r7 = cn_var_信息.位置;
  r8 = r7.文件名;
  r9 = cn_var_信息.位置;
  r10 = r9.行号;
  r11 = cn_var_信息.位置;
  r12 = r11.列号;
  r13 = cn_var_级别字符串_0;
  r14 = cn_var_信息.消息;
  打印格式("%s:%d:%d: %s: %s\n", r8, r10, r12, r13, r14);
  goto if_merge_860;

  if_else_859:
  r15 = cn_var_级别字符串_0;
  r16 = cn_var_信息.消息;
  打印格式("%s: %s\n", r15, r16);
  goto if_merge_860;

  if_merge_860:
  return;
}

void 打印所有诊断(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_861; else goto if_merge_862;

  if_then_861:
  return;
  goto if_merge_862;

  if_merge_862:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_863;

  for_cond_863:
  r2 = cn_var_i_0;
  r3 = cn_var_集合.长度;
  r4 = r2 < r3;
  if (r4) goto for_body_864; else goto for_exit_866;

  for_body_864:
  r5 = cn_var_集合.诊断数组;
  r6 = cn_var_i_0;
  r7 = r5 + r6;
  打印诊断信息(r7);
  goto for_update_865;

  for_update_865:
  r8 = cn_var_i_0;
  r9 = r8 + 1;
  cn_var_i_0 = r9;
  goto for_cond_863;

  for_exit_866:
  r10 = cn_var_集合.错误计数;
  r11 = cn_var_集合.警告计数;
  打印格式("\n编译完成: %d 个错误, %d 个警告\n", r10, r11);
  return;
}

void 清空诊断集合(struct 诊断集合* cn_var_集合) {
  long long r1;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_867; else goto if_merge_868;

  if_then_867:
  return;
  goto if_merge_868;

  if_merge_868:
  return;
}

struct 源位置 创建源位置(const char* cn_var_文件名, long long cn_var_行号, long long cn_var_列号) {
  long long r1, r2;
  char* r0;
  struct 源位置 r3;

  entry:
  struct 源位置 cn_var_位置_0;
  r0 = cn_var_文件名;
  r1 = cn_var_行号;
  r2 = cn_var_列号;
  r3 = cn_var_位置_0;
  return r3;
}

struct 源位置 创建未知位置() {
  struct 源位置 r0;

  entry:
  r0 = 创建源位置(0, 0, 0);
  return r0;
}

void 报告重复符号错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_符号名) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_符号名;
  字符串格式("重复定义的符号: %s", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_语义_重复符号, r2, r3);
  return;
}

void 报告未定义标识符错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_标识符名) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_标识符名;
  字符串格式("未定义的标识符: %s", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_语义_未定义标识符, r2, r3);
  return;
}

void 报告类型不匹配错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_期望类型, const char* cn_var_实际类型) {
  char* r0;
  char* r1;
  char* r4;
  struct 诊断集合* r2;
  struct 源位置 r3;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_期望类型;
  r1 = cn_var_实际类型;
  字符串格式("类型不匹配: 期望 %s, 实际 %s", r0, r1);
  cn_var_消息_0 = /* NONE */;
  r2 = cn_var_集合;
  r3 = cn_var_位置;
  r4 = cn_var_消息_0;
  报告错误(r2, 诊断错误码_语义_类型不匹配, r3, r4);
  return;
}

void 报告缺少返回语句错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_函数名) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_函数名;
  字符串格式("函数 '%s' 缺少返回语句", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_语义_缺少返回语句, r2, r3);
  return;
}

void 报告常量缺少初始化错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_常量名) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_常量名;
  字符串格式("常量 '%s' 缺少初始化表达式", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_语义_常量缺少初始化, r2, r3);
  return;
}

void 报告期望标记错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_期望的标记) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_期望的标记;
  字符串格式("期望 %s", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_语法_期望标记, r2, r3);
  return;
}

void 报告未终止字符串错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置) {
  struct 诊断集合* r0;
  struct 源位置 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_位置;
  报告错误(r0, 诊断错误码_词法_未终止字符串, r1, "字符串未正确终止");
  return;
}

void 报告无效字符错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, const char* cn_var_无效字符) {
  char* r0;
  char* r3;
  struct 诊断集合* r1;
  struct 源位置 r2;

  entry:
  char* cn_var_消息_0;
  r0 = cn_var_无效字符;
  字符串格式("无效字符: %s", r0);
  cn_var_消息_0 = /* NONE */;
  r1 = cn_var_集合;
  r2 = cn_var_位置;
  r3 = cn_var_消息_0;
  报告错误(r1, 诊断错误码_词法_无效字符, r2, r3);
  return;
}

void 诊断集合添加(struct 诊断集合* cn_var_集合, struct 诊断信息 cn_var_信息) {
  long long r1, r2, r3, r4, r7, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18;
  struct 诊断集合* r0;
  struct 诊断集合* r5;
  _Bool r6;
  struct 诊断信息 r8;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_869; else goto if_merge_870;

  if_then_869:
  return;
  goto if_merge_870;

  if_merge_870:
  r2 = cn_var_集合.长度;
  r3 = cn_var_集合.容量;
  r4 = r2 >= r3;
  if (r4) goto if_then_871; else goto if_merge_872;

  if_then_871:
  r5 = cn_var_集合;
  r6 = 扩展诊断数组(r5);
  r7 = r6 == 0;
  if (r7) goto if_then_873; else goto if_merge_874;

  if_merge_872:
  r8 = cn_var_信息;
  r9 = cn_var_集合.诊断数组;
  r10 = cn_var_集合.长度;
    { long long _tmp_r1 = r8; cn_rt_array_set_element(r9, r10, &_tmp_r1, 8); }
  r11 = cn_var_集合.长度;
  r12 = r11 + 1;
  r13 = cn_var_信息.严重级别;
  r14 = r13 == 诊断严重级别_诊断_错误;
  if (r14) goto if_then_875; else goto if_else_876;

  if_then_873:
  return;
  goto if_merge_874;

  if_merge_874:
  goto if_merge_872;

  if_then_875:
  r15 = cn_var_集合.错误计数;
  r16 = r15 + 1;
  goto if_merge_877;

  if_else_876:
  r17 = cn_var_集合.警告计数;
  r18 = r17 + 1;
  goto if_merge_877;

  if_merge_877:
  return;
}

