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
struct 源位置 创建未知位置();
void 报告重复符号错误(struct 诊断集合*, struct 源位置, char*);
void 报告未定义标识符错误(struct 诊断集合*, struct 源位置, char*);
void 报告类型不匹配错误(struct 诊断集合*, struct 源位置, char*, char*);
void 报告缺少返回语句错误(struct 诊断集合*, struct 源位置, char*);
void 报告常量缺少初始化错误(struct 诊断集合*, struct 源位置, char*);
void 报告期望标记错误(struct 诊断集合*, struct 源位置, char*);
void 报告未终止字符串错误(struct 诊断集合*, struct 源位置);
void 报告无效字符错误(struct 诊断集合*, struct 源位置, char*);
void 诊断集合添加(struct 诊断集合*, struct 诊断信息);

struct 诊断集合* 创建诊断集合(long long cn_var_初始容量) {
  long long r0, r1, r4, r5, r9, r11;
  void* r2;
  struct 诊断集合* r3;
  void* r6;
  struct 诊断集合* r7;
  struct 诊断信息* r8;
  struct 诊断集合* r10;
  struct 诊断集合* r12;

  entry:
  r0 = cn_var_初始容量;
  r1 = r0 <= 0;
  if (r1) goto if_then_829; else goto if_merge_830;

  if_then_829:
  cn_var_初始容量 = 4;
  goto if_merge_830;

  if_merge_830:
  struct 诊断集合* cn_var_集合_0;
  r2 = 分配内存(32);
  cn_var_集合_0 = r2;
  r3 = cn_var_集合_0;
  r4 = r3 == 0;
  if (r4) goto if_then_831; else goto if_merge_832;

  if_then_831:
  return 0;
  goto if_merge_832;

  if_merge_832:
  r5 = cn_var_初始容量;
  r6 = 分配内存数组(48, r5);
  r7 = cn_var_集合_0;
  r8 = r7->诊断数组;
  r9 = r8 == 0;
  if (r9) goto if_then_833; else goto if_merge_834;

  if_then_833:
  r10 = cn_var_集合_0;
  释放内存(r10);
  return 0;
  goto if_merge_834;

  if_merge_834:
  r11 = cn_var_初始容量;
  r12 = cn_var_集合_0;
  return r12;
}

void 释放诊断集合(struct 诊断集合* cn_var_集合) {
  long long r1, r4;
  struct 诊断集合* r0;
  struct 诊断集合* r2;
  struct 诊断信息* r3;
  struct 诊断集合* r5;
  struct 诊断信息* r6;
  struct 诊断集合* r7;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_835; else goto if_merge_836;

  if_then_835:
  return;
  goto if_merge_836;

  if_merge_836:
  r2 = cn_var_集合;
  r3 = r2->诊断数组;
  r4 = r3 != 0;
  if (r4) goto if_then_837; else goto if_merge_838;

  if_then_837:
  r5 = cn_var_集合;
  r6 = r5->诊断数组;
  释放内存(r6);
  goto if_merge_838;

  if_merge_838:
  r7 = cn_var_集合;
  释放内存(r7);
  return;
}

_Bool 扩展诊断数组(struct 诊断集合* cn_var_集合) {
  long long r1, r3, r4, r5, r6, r9, r10, r13, r15;
  struct 诊断集合* r0;
  struct 诊断集合* r2;
  struct 诊断集合* r7;
  struct 诊断信息* r8;
  void* r11;
  struct 诊断信息* r12;
  struct 诊断信息* r14;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_839; else goto if_merge_840;

  if_then_839:
  return 0;
  goto if_merge_840;

  if_merge_840:
  long long cn_var_新容量_0;
  r2 = cn_var_集合;
  r3 = r2->容量;
  r4 = r3 << 1;
  cn_var_新容量_0 = r4;
  r5 = cn_var_新容量_0;
  r6 = r5 <= 0;
  if (r6) goto if_then_841; else goto if_merge_842;

  if_then_841:
  cn_var_新容量_0 = 4;
  goto if_merge_842;

  if_merge_842:
  struct 诊断信息* cn_var_新数组_1;
  r7 = cn_var_集合;
  r8 = r7->诊断数组;
  r9 = cn_var_新容量_0;
  r10 = r9 * 48;
  r11 = 重新分配内存(r8, r10);
  cn_var_新数组_1 = r11;
  r12 = cn_var_新数组_1;
  r13 = r12 == 0;
  if (r13) goto if_then_843; else goto if_merge_844;

  if_then_843:
  return 0;
  goto if_merge_844;

  if_merge_844:
  r14 = cn_var_新数组_1;
  r15 = cn_var_新容量_0;
  return 1;
}

void 报告诊断(struct 诊断集合* cn_var_集合, enum 诊断严重级别 cn_var_级别, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, char* cn_var_消息) {
  long long r1, r3, r5, r6, r9, r13, r14, r20, r21, r23, r25, r26, r28, r29;
  char* r18;
  struct 诊断集合* r0;
  struct 诊断集合* r2;
  struct 诊断集合* r4;
  struct 诊断集合* r7;
  struct 诊断集合* r10;
  struct 诊断信息* r11;
  struct 诊断集合* r12;
  struct 诊断集合* r19;
  struct 诊断集合* r24;
  struct 诊断集合* r27;
  _Bool r8;
  struct 源位置 r17;
  enum 诊断严重级别 r15;
  enum 诊断错误码 r16;
  enum 诊断严重级别 r22;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_845; else goto if_merge_846;

  if_then_845:
  return;
  goto if_merge_846;

  if_merge_846:
  r2 = cn_var_集合;
  r3 = r2->长度;
  r4 = cn_var_集合;
  r5 = r4->容量;
  r6 = r3 >= r5;
  if (r6) goto if_then_847; else goto if_merge_848;

  if_then_847:
  r7 = cn_var_集合;
  r8 = 扩展诊断数组(r7);
  r9 = r8 == 0;
  if (r9) goto if_then_849; else goto if_merge_850;

  if_merge_848:
  struct 诊断信息* cn_var_信息_0;
  r10 = cn_var_集合;
  r11 = r10->诊断数组;
  r12 = cn_var_集合;
  r13 = r12->长度;
  r14 = r11 + r13;
  cn_var_信息_0 = r14;
  r15 = cn_var_级别;
  r16 = cn_var_代码;
  r17 = cn_var_位置;
  r18 = cn_var_消息;
  r19 = cn_var_集合;
  r20 = r19->长度;
  r21 = r20 + 1;
  r22 = cn_var_级别;
  r23 = r22 == 0;
  if (r23) goto if_then_851; else goto if_else_852;

  if_then_849:
  return;
  goto if_merge_850;

  if_merge_850:
  goto if_merge_848;

  if_then_851:
  r24 = cn_var_集合;
  r25 = r24->错误计数;
  r26 = r25 + 1;
  goto if_merge_853;

  if_else_852:
  r27 = cn_var_集合;
  r28 = r27->警告计数;
  r29 = r28 + 1;
  goto if_merge_853;

  if_merge_853:
  return;
}

void 报告错误(struct 诊断集合* cn_var_集合, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, char* cn_var_消息) {
  char* r3;
  struct 诊断集合* r0;
  struct 源位置 r2;
  enum 诊断错误码 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_代码;
  r2 = cn_var_位置;
  r3 = cn_var_消息;
  报告诊断(r0, 0, r1, r2, r3);
  return;
}

void 报告警告(struct 诊断集合* cn_var_集合, enum 诊断错误码 cn_var_代码, struct 源位置 cn_var_位置, char* cn_var_消息) {
  char* r3;
  struct 诊断集合* r0;
  struct 源位置 r2;
  enum 诊断错误码 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_代码;
  r2 = cn_var_位置;
  r3 = cn_var_消息;
  报告诊断(r0, 0, r1, r2, r3);
  return;
}

_Bool 有错误(struct 诊断集合* cn_var_集合) {
  long long r1, r3, r4;
  struct 诊断集合* r0;
  struct 诊断集合* r2;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_854; else goto if_merge_855;

  if_then_854:
  return 0;
  goto if_merge_855;

  if_merge_855:
  r2 = cn_var_集合;
  r3 = r2->错误计数;
  r4 = r3 > 0;
  return r4;
}

long long 获取错误计数(struct 诊断集合* cn_var_集合) {
  long long r1, r3;
  struct 诊断集合* r0;
  struct 诊断集合* r2;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_856; else goto if_merge_857;

  if_then_856:
  return 0;
  goto if_merge_857;

  if_merge_857:
  r2 = cn_var_集合;
  r3 = r2->错误计数;
  return r3;
}

long long 获取警告计数(struct 诊断集合* cn_var_集合) {
  long long r1, r3;
  struct 诊断集合* r0;
  struct 诊断集合* r2;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_858; else goto if_merge_859;

  if_then_858:
  return 0;
  goto if_merge_859;

  if_merge_859:
  r2 = cn_var_集合;
  r3 = r2->警告计数;
  return r3;
}

_Bool 应该继续(struct 诊断集合* cn_var_集合) {
  long long r1, r3, r4, r6, r8, r9;
  struct 诊断集合* r0;
  struct 诊断集合* r2;
  struct 诊断集合* r5;
  struct 诊断集合* r7;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_860; else goto if_merge_861;

  if_then_860:
  return 1;
  goto if_merge_861;

  if_merge_861:
  r2 = cn_var_集合;
  r3 = r2->最大错误数;
  r4 = r3 > 0;
  if (r4) goto if_then_862; else goto if_merge_863;

  if_then_862:
  r5 = cn_var_集合;
  r6 = r5->错误计数;
  r7 = cn_var_集合;
  r8 = r7->最大错误数;
  r9 = r6 < r8;
  return r9;
  goto if_merge_863;

  if_merge_863:
  return 1;
}

char* 获取严重级别字符串(enum 诊断严重级别 cn_var_级别) {
  long long r1, r3;
  enum 诊断严重级别 r0;
  enum 诊断严重级别 r2;

  entry:
  r0 = cn_var_级别;
  r1 = r0 == 0;
  if (r1) goto if_then_864; else goto if_else_865;

  if_then_864:
  return "错误";
  goto if_merge_866;

  if_else_865:
  r2 = cn_var_级别;
  r3 = r2 == 0;
  if (r3) goto if_then_867; else goto if_else_868;

  if_merge_866:

  if_then_867:
  return "警告";
  goto if_merge_869;

  if_else_868:
  return "未知";
  goto if_merge_869;

  if_merge_869:
  goto if_merge_866;
  return NULL;
}

void 打印诊断信息(struct 诊断信息* cn_var_信息) {
  long long r1, r8, r14, r17;
  char* r4;
  char* r7;
  char* r11;
  char* r18;
  char* r20;
  char* r21;
  char* r23;
  struct 诊断信息* r0;
  struct 诊断信息* r2;
  struct 诊断信息* r5;
  struct 诊断信息* r9;
  struct 诊断信息* r12;
  struct 诊断信息* r15;
  struct 诊断信息* r19;
  struct 诊断信息* r22;
  struct 源位置 r6;
  struct 源位置 r10;
  struct 源位置 r13;
  struct 源位置 r16;
  enum 诊断严重级别 r3;

  entry:
  r0 = cn_var_信息;
  r1 = r0 == 0;
  if (r1) goto if_then_870; else goto if_merge_871;

  if_then_870:
  return;
  goto if_merge_871;

  if_merge_871:
  char* cn_var_级别字符串_0;
  r2 = cn_var_信息;
  r3 = r2->严重级别;
  r4 = 获取严重级别字符串(r3);
  cn_var_级别字符串_0 = r4;
  r5 = cn_var_信息;
  r6 = r5->位置;
  r7 = r6.文件名;
  r8 = r7 != 0;
  if (r8) goto if_then_872; else goto if_else_873;

  if_then_872:
  r9 = cn_var_信息;
  r10 = r9->位置;
  r11 = r10.文件名;
  r12 = cn_var_信息;
  r13 = r12->位置;
  r14 = r13.行号;
  r15 = cn_var_信息;
  r16 = r15->位置;
  r17 = r16.列号;
  r18 = cn_var_级别字符串_0;
  r19 = cn_var_信息;
  r20 = r19->消息;
  打印格式("%s:%d:%d: %s: %s\n", r11, r14, r17, r18, r20);
  goto if_merge_874;

  if_else_873:
  r21 = cn_var_级别字符串_0;
  r22 = cn_var_信息;
  r23 = r22->消息;
  打印格式("%s: %s\n", r21, r23);
  goto if_merge_874;

  if_merge_874:
  return;
}

void 打印所有诊断(struct 诊断集合* cn_var_集合) {
  long long r1, r2, r4, r5, r8, r9, r10, r11, r13, r15;
  struct 诊断集合* r0;
  struct 诊断集合* r3;
  struct 诊断集合* r6;
  struct 诊断信息* r7;
  struct 诊断集合* r12;
  struct 诊断集合* r14;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_875; else goto if_merge_876;

  if_then_875:
  return;
  goto if_merge_876;

  if_merge_876:
  long long cn_var_i_0;
  cn_var_i_0 = 0;
  goto for_cond_877;

  for_cond_877:
  r2 = cn_var_i_0;
  r3 = cn_var_集合;
  r4 = r3->长度;
  r5 = r2 < r4;
  if (r5) goto for_body_878; else goto for_exit_880;

  for_body_878:
  r6 = cn_var_集合;
  r7 = r6->诊断数组;
  r8 = cn_var_i_0;
  r9 = r7 + r8;
  打印诊断信息(r9);
  goto for_update_879;

  for_update_879:
  r10 = cn_var_i_0;
  r11 = r10 + 1;
  cn_var_i_0 = r11;
  goto for_cond_877;

  for_exit_880:
  r12 = cn_var_集合;
  r13 = r12->错误计数;
  r14 = cn_var_集合;
  r15 = r14->警告计数;
  打印格式("\n编译完成: %d 个错误, %d 个警告\n", r13, r15);
  return;
}

void 清空诊断集合(struct 诊断集合* cn_var_集合) {
  long long r1;
  struct 诊断集合* r0;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_881; else goto if_merge_882;

  if_then_881:
  return;
  goto if_merge_882;

  if_merge_882:
  return;
}

struct 源位置 创建源位置(char* cn_var_文件名, long long cn_var_行号, long long cn_var_列号) {
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

void 报告重复符号错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_符号名) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 报告未定义标识符错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_标识符名) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 报告类型不匹配错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_期望类型, char* cn_var_实际类型) {
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
  报告错误(r2, 0, r3, r4);
  return;
}

void 报告缺少返回语句错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_函数名) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 报告常量缺少初始化错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_常量名) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 报告期望标记错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_期望的标记) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 报告未终止字符串错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置) {
  struct 诊断集合* r0;
  struct 源位置 r1;

  entry:
  r0 = cn_var_集合;
  r1 = cn_var_位置;
  报告错误(r0, 0, r1, "字符串未正确终止");
  return;
}

void 报告无效字符错误(struct 诊断集合* cn_var_集合, struct 源位置 cn_var_位置, char* cn_var_无效字符) {
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
  报告错误(r1, 0, r2, r3);
  return;
}

void 诊断集合添加(struct 诊断集合* cn_var_集合, struct 诊断信息 cn_var_信息) {
  long long r1, r3, r5, r6, r9, r14, r16, r17, r20, r22, r23, r25, r26;
  struct 诊断集合* r0;
  struct 诊断集合* r2;
  struct 诊断集合* r4;
  struct 诊断集合* r7;
  struct 诊断集合* r11;
  struct 诊断信息* r12;
  struct 诊断集合* r13;
  struct 诊断集合* r15;
  struct 诊断集合* r21;
  struct 诊断集合* r24;
  _Bool r8;
  struct 诊断信息 r10;
  struct 诊断信息 r18;
  enum 诊断严重级别 r19;

  entry:
  r0 = cn_var_集合;
  r1 = r0 == 0;
  if (r1) goto if_then_883; else goto if_merge_884;

  if_then_883:
  return;
  goto if_merge_884;

  if_merge_884:
  r2 = cn_var_集合;
  r3 = r2->长度;
  r4 = cn_var_集合;
  r5 = r4->容量;
  r6 = r3 >= r5;
  if (r6) goto if_then_885; else goto if_merge_886;

  if_then_885:
  r7 = cn_var_集合;
  r8 = 扩展诊断数组(r7);
  r9 = r8 == 0;
  if (r9) goto if_then_887; else goto if_merge_888;

  if_merge_886:
  r10 = cn_var_信息;
  r11 = cn_var_集合;
  r12 = r11->诊断数组;
  r13 = cn_var_集合;
  r14 = r13->长度;
    { long long _tmp_r4 = r10; cn_rt_array_set_element(r12, r14, &_tmp_r4, 8); }
  r15 = cn_var_集合;
  r16 = r15->长度;
  r17 = r16 + 1;
  r18 = cn_var_信息;
  r19 = r18.严重级别;
  r20 = r19 == 0;
  if (r20) goto if_then_889; else goto if_else_890;

  if_then_887:
  return;
  goto if_merge_888;

  if_merge_888:
  goto if_merge_886;

  if_then_889:
  r21 = cn_var_集合;
  r22 = r21->错误计数;
  r23 = r22 + 1;
  goto if_merge_891;

  if_else_890:
  r24 = cn_var_集合;
  r25 = r24->警告计数;
  r26 = r25 + 1;
  goto if_merge_891;

  if_merge_891:
  return;
}

