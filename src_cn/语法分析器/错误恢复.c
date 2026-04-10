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

// Struct Definitions - 从导入模块

// Forward Declarations - 从导入模块

// CN Language Enum Definitions
enum 同步点类型 {
    同步点类型_同步点_分号,
    同步点类型_同步点_右大括号,
    同步点类型_同步点_右小括号,
    同步点类型_同步点_右中括号,
    同步点类型_同步点_逗号,
    同步点类型_同步点_关键字,
    同步点类型_同步点_类型关键字,
    同步点类型_同步点_声明关键字
};

enum 语法恢复策略 {
    语法恢复策略_不恢复,
    语法恢复策略_跳过词元,
    语法恢复策略_同步到分号,
    语法恢复策略_同步到大括号,
    语法恢复策略_同步到关键字
};

// CN Language Global Struct Forward Declarations
struct 语法错误恢复上下文;

// CN Language Global Struct Definitions
struct 语法错误恢复上下文 {
    long long 连续错误计数;
    long long 最大连续错误;
    long long 最后错误行号;
    long long 级联抑制计数;
    _Bool 抑制级联错误;
    long long 恢复尝试次数;
    long long 最大恢复次数;
};

// Global Variables
long long cn_var_默认最大连续错误 = 3;
long long cn_var_默认最大恢复次数 = 10;
long long cn_var_语法错误恢复上下文大小 = 28;

// Forward Declarations
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

_Bool 初始化错误恢复(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1, r2, r3;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_648; else goto if_merge_649;

  if_then_648:
  return 0;
  goto if_merge_649;

  if_merge_649:
  r2 = cn_var_默认最大连续错误;
  r3 = cn_var_默认最大恢复次数;
  return 1;
}

void 销毁错误恢复上下文(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 != 0;
  if (r1) goto if_then_650; else goto if_merge_651;

  if_then_650:
  goto if_merge_651;

  if_merge_651:
  return;
}

long long 是同步点(enum 词元类型枚举 cn_var_词元类型, enum 同步点类型 cn_var_同步类型) {
  long long r10, r12, r14, r16, r18, r19, r21, r23, r24, r26, r28, r29, r31, r33;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  enum 同步点类型 r0;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r22;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r30;
  enum 词元类型枚举 r32;

  entry:
  r0 = cn_var_同步类型;
  r1 = r0 == 0;
  if (r1) goto case_body_653; else goto switch_check_662;

  switch_check_662:
  r2 = r0 == 1;
  if (r2) goto case_body_654; else goto switch_check_663;

  switch_check_663:
  r3 = r0 == 2;
  if (r3) goto case_body_655; else goto switch_check_664;

  switch_check_664:
  r4 = r0 == 3;
  if (r4) goto case_body_656; else goto switch_check_665;

  switch_check_665:
  r5 = r0 == 4;
  if (r5) goto case_body_657; else goto switch_check_666;

  switch_check_666:
  r6 = r0 == 5;
  if (r6) goto case_body_658; else goto switch_check_667;

  switch_check_667:
  r7 = r0 == 6;
  if (r7) goto case_body_659; else goto switch_check_668;

  switch_check_668:
  r8 = r0 == 7;
  if (r8) goto case_body_660; else goto case_default_661;

  case_body_653:
  r9 = cn_var_词元类型;
  r10 = r9 == 1;
  return r10;
  goto switch_merge_652;

  case_body_654:
  r11 = cn_var_词元类型;
  r12 = r11 == 2;
  return r12;
  goto switch_merge_652;

  case_body_655:
  r13 = cn_var_词元类型;
  r14 = r13 == 3;
  return r14;
  goto switch_merge_652;

  case_body_656:
  r15 = cn_var_词元类型;
  r16 = r15 == 4;
  return r16;
  goto switch_merge_652;

  case_body_657:
  r17 = cn_var_词元类型;
  r18 = r17 == 5;
  return r18;
  goto switch_merge_652;

  case_body_658:
  r20 = cn_var_词元类型;
  r21 = r20 >= 100;
  if (r21) goto logic_rhs_669; else goto logic_merge_670;

  logic_rhs_669:
  r22 = cn_var_词元类型;
  r23 = r22 <= 150;
  goto logic_merge_670;

  logic_merge_670:
  return r23;
  goto switch_merge_652;

  case_body_659:
  r25 = cn_var_词元类型;
  r26 = r25 >= 160;
  if (r26) goto logic_rhs_671; else goto logic_merge_672;

  logic_rhs_671:
  r27 = cn_var_词元类型;
  r28 = r27 <= 170;
  goto logic_merge_672;

  logic_merge_672:
  return r28;
  goto switch_merge_652;

  case_body_660:
  r30 = cn_var_词元类型;
  r31 = r30 >= 180;
  if (r31) goto logic_rhs_673; else goto logic_merge_674;

  logic_rhs_673:
  r32 = cn_var_词元类型;
  r33 = r32 <= 190;
  goto logic_merge_674;

  logic_merge_674:
  return r33;
  goto switch_merge_652;

  case_default_661:
  return 0;
  goto switch_merge_652;

  switch_merge_652:
  return 0;
}

long long 是任意同步点(enum 词元类型枚举 cn_var_词元类型) {
  long long r1, r3, r5, r7, r9, r11, r13, r15;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r14;

  entry:
  r0 = cn_var_词元类型;
  r1 = 是同步点(r0, 0);
  if (r1) goto if_then_675; else goto if_merge_676;

  if_then_675:
  return 1;
  goto if_merge_676;

  if_merge_676:
  r2 = cn_var_词元类型;
  r3 = 是同步点(r2, 1);
  if (r3) goto if_then_677; else goto if_merge_678;

  if_then_677:
  return 1;
  goto if_merge_678;

  if_merge_678:
  r4 = cn_var_词元类型;
  r5 = 是同步点(r4, 2);
  if (r5) goto if_then_679; else goto if_merge_680;

  if_then_679:
  return 1;
  goto if_merge_680;

  if_merge_680:
  r6 = cn_var_词元类型;
  r7 = 是同步点(r6, 3);
  if (r7) goto if_then_681; else goto if_merge_682;

  if_then_681:
  return 1;
  goto if_merge_682;

  if_merge_682:
  r8 = cn_var_词元类型;
  r9 = 是同步点(r8, 4);
  if (r9) goto if_then_683; else goto if_merge_684;

  if_then_683:
  return 1;
  goto if_merge_684;

  if_merge_684:
  r10 = cn_var_词元类型;
  r11 = 是同步点(r10, 5);
  if (r11) goto if_then_685; else goto if_merge_686;

  if_then_685:
  return 1;
  goto if_merge_686;

  if_merge_686:
  r12 = cn_var_词元类型;
  r13 = 是同步点(r12, 6);
  if (r13) goto if_then_687; else goto if_merge_688;

  if_then_687:
  return 1;
  goto if_merge_688;

  if_merge_688:
  r14 = cn_var_词元类型;
  r15 = 是同步点(r14, 7);
  if (r15) goto if_then_689; else goto if_merge_690;

  if_then_689:
  return 1;
  goto if_merge_690;

  if_merge_690:
  return 0;
}

long long 跳转到同步点(struct 语法错误恢复上下文* cn_var_上下文, enum 词元类型枚举 cn_var_当前词元类型, enum 同步点类型 cn_var_目标同步类型) {
  long long r1, r3, r4, r6, r8, r9, r12, r14, r16;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r5;
  struct 语法错误恢复上下文* r7;
  enum 词元类型枚举 r10;
  enum 同步点类型 r11;
  enum 同步点类型 r13;
  enum 词元类型枚举 r15;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_691; else goto if_merge_692;

  if_then_691:
  return 0;
  goto if_merge_692;

  if_merge_692:
  r2 = cn_var_上下文;
  r3 = r2->恢复尝试次数;
  r4 = r3 + 1;
  r5 = cn_var_上下文;
  r6 = r5->恢复尝试次数;
  r7 = cn_var_上下文;
  r8 = r7->最大恢复次数;
  r9 = r6 > r8;
  if (r9) goto if_then_693; else goto if_merge_694;

  if_then_693:
  return 0;
  goto if_merge_694;

  if_merge_694:
  r10 = cn_var_当前词元类型;
  r11 = cn_var_目标同步类型;
  r12 = 是同步点(r10, r11);
  if (r12) goto if_then_695; else goto if_merge_696;

  if_then_695:
  return 1;
  goto if_merge_696;

  if_merge_696:
  r13 = cn_var_目标同步类型;
  r14 = r13 == 0;
  if (r14) goto if_then_697; else goto if_merge_698;

  if_then_697:
  r15 = cn_var_当前词元类型;
  r16 = 是任意同步点(r15);
  if (r16) goto if_then_699; else goto if_merge_700;

  if_merge_698:
  return 0;

  if_then_699:
  return 1;
  goto if_merge_700;

  if_merge_700:
  goto if_merge_698;
  return 0;
}

_Bool 应该抑制错误(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1, r5, r7, r8;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r4;
  struct 语法错误恢复上下文* r6;
  _Bool r3;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_701; else goto if_merge_702;

  if_then_701:
  return 0;
  goto if_merge_702;

  if_merge_702:
  r2 = cn_var_上下文;
  r3 = r2->抑制级联错误;
  if (r3) goto if_then_703; else goto if_merge_704;

  if_then_703:
  return 1;
  goto if_merge_704;

  if_merge_704:
  r4 = cn_var_上下文;
  r5 = r4->连续错误计数;
  r6 = cn_var_上下文;
  r7 = r6->最大连续错误;
  r8 = r5 >= r7;
  if (r8) goto if_then_705; else goto if_merge_706;

  if_then_705:
  return 1;
  goto if_merge_706;

  if_merge_706:
  return 0;
}

void 记录错误(struct 语法错误恢复上下文* cn_var_上下文, long long cn_var_行号) {
  long long r1, r3, r4, r5, r7, r8, r10, r12, r13, r15, r16, r17;
  struct 语法错误恢复上下文* r0;
  struct 语法错误恢复上下文* r2;
  struct 语法错误恢复上下文* r6;
  struct 语法错误恢复上下文* r9;
  struct 语法错误恢复上下文* r11;
  struct 语法错误恢复上下文* r14;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_707; else goto if_merge_708;

  if_then_707:
  return;
  goto if_merge_708;

  if_merge_708:
  r2 = cn_var_上下文;
  r3 = r2->最后错误行号;
  r4 = cn_var_行号;
  r5 = r3 == r4;
  if (r5) goto if_then_709; else goto if_else_710;

  if_then_709:
  r6 = cn_var_上下文;
  r7 = r6->级联抑制计数;
  r8 = r7 + 1;
  r9 = cn_var_上下文;
  r10 = r9->级联抑制计数;
  r11 = cn_var_上下文;
  r12 = r11->最大连续错误;
  r13 = r10 >= r12;
  if (r13) goto if_then_712; else goto if_merge_713;

  if_else_710:
  goto if_merge_711;

  if_merge_711:
  r14 = cn_var_上下文;
  r15 = r14->连续错误计数;
  r16 = r15 + 1;
  r17 = cn_var_行号;

  if_then_712:
  goto if_merge_713;

  if_merge_713:
  goto if_merge_711;
  return;
}

void 重置错误计数(struct 语法错误恢复上下文* cn_var_上下文) {
  long long r1;
  struct 语法错误恢复上下文* r0;

  entry:
  r0 = cn_var_上下文;
  r1 = r0 == 0;
  if (r1) goto if_then_714; else goto if_merge_715;

  if_then_714:
  return;
  goto if_merge_715;

  if_merge_715:
  return;
}

enum 语法恢复策略 选择恢复策略(long long cn_var_错误码) {
  long long r0;
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;

  entry:
  r0 = cn_var_错误码;
  r1 = r0 == 101;
  if (r1) goto case_body_717; else goto switch_check_730;

  switch_check_730:
  r2 = r0 == 102;
  if (r2) goto case_body_718; else goto switch_check_731;

  switch_check_731:
  r3 = r0 == 103;
  if (r3) goto case_body_719; else goto switch_check_732;

  switch_check_732:
  r4 = r0 == 104;
  if (r4) goto case_body_720; else goto switch_check_733;

  switch_check_733:
  r5 = r0 == 105;
  if (r5) goto case_body_721; else goto switch_check_734;

  switch_check_734:
  r6 = r0 == 106;
  if (r6) goto case_body_722; else goto switch_check_735;

  switch_check_735:
  r7 = r0 == 107;
  if (r7) goto case_body_723; else goto switch_check_736;

  switch_check_736:
  r8 = r0 == 108;
  if (r8) goto case_body_724; else goto switch_check_737;

  switch_check_737:
  r9 = r0 == 109;
  if (r9) goto case_body_725; else goto switch_check_738;

  switch_check_738:
  r10 = r0 == 1;
  if (r10) goto case_body_726; else goto switch_check_739;

  switch_check_739:
  r11 = r0 == 2;
  if (r11) goto case_body_727; else goto switch_check_740;

  switch_check_740:
  r12 = r0 == 3;
  if (r12) goto case_body_728; else goto case_default_729;

  case_body_717:
  goto switch_merge_716;

  case_body_718:
  return 2;
  goto switch_merge_716;

  case_body_719:
  goto switch_merge_716;

  case_body_720:
  return 3;
  goto switch_merge_716;

  case_body_721:
  goto switch_merge_716;

  case_body_722:
  goto switch_merge_716;

  case_body_723:
  return 4;
  goto switch_merge_716;

  case_body_724:
  goto switch_merge_716;

  case_body_725:
  return 1;
  goto switch_merge_716;

  case_body_726:
  goto switch_merge_716;

  case_body_727:
  goto switch_merge_716;

  case_body_728:
  return 1;
  goto switch_merge_716;

  case_default_729:
  return 2;
  goto switch_merge_716;

  switch_merge_716:
  return 0;
}

char* 同步点类型名称(enum 同步点类型 cn_var_同步类型) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  _Bool r6;
  _Bool r7;
  _Bool r8;
  enum 同步点类型 r0;

  entry:
  r0 = cn_var_同步类型;
  r1 = r0 == 0;
  if (r1) goto case_body_742; else goto switch_check_751;

  switch_check_751:
  r2 = r0 == 1;
  if (r2) goto case_body_743; else goto switch_check_752;

  switch_check_752:
  r3 = r0 == 2;
  if (r3) goto case_body_744; else goto switch_check_753;

  switch_check_753:
  r4 = r0 == 3;
  if (r4) goto case_body_745; else goto switch_check_754;

  switch_check_754:
  r5 = r0 == 4;
  if (r5) goto case_body_746; else goto switch_check_755;

  switch_check_755:
  r6 = r0 == 5;
  if (r6) goto case_body_747; else goto switch_check_756;

  switch_check_756:
  r7 = r0 == 6;
  if (r7) goto case_body_748; else goto switch_check_757;

  switch_check_757:
  r8 = r0 == 7;
  if (r8) goto case_body_749; else goto case_default_750;

  case_body_742:
  return "分号";
  goto switch_merge_741;

  case_body_743:
  return "右大括号";
  goto switch_merge_741;

  case_body_744:
  return "右小括号";
  goto switch_merge_741;

  case_body_745:
  return "右中括号";
  goto switch_merge_741;

  case_body_746:
  return "逗号";
  goto switch_merge_741;

  case_body_747:
  return "关键字";
  goto switch_merge_741;

  case_body_748:
  return "类型关键字";
  goto switch_merge_741;

  case_body_749:
  return "声明关键字";
  goto switch_merge_741;

  case_default_750:
  return "未知同步点";
  goto switch_merge_741;

  switch_merge_741:
  return NULL;
}

char* 恢复策略名称(enum 语法恢复策略 cn_var_策略) {
  _Bool r1;
  _Bool r2;
  _Bool r3;
  _Bool r4;
  _Bool r5;
  enum 语法恢复策略 r0;

  entry:
  r0 = cn_var_策略;
  r1 = r0 == 0;
  if (r1) goto case_body_759; else goto switch_check_765;

  switch_check_765:
  r2 = r0 == 1;
  if (r2) goto case_body_760; else goto switch_check_766;

  switch_check_766:
  r3 = r0 == 2;
  if (r3) goto case_body_761; else goto switch_check_767;

  switch_check_767:
  r4 = r0 == 3;
  if (r4) goto case_body_762; else goto switch_check_768;

  switch_check_768:
  r5 = r0 == 4;
  if (r5) goto case_body_763; else goto case_default_764;

  case_body_759:
  return "不恢复";
  goto switch_merge_758;

  case_body_760:
  return "跳过词元";
  goto switch_merge_758;

  case_body_761:
  return "同步到分号";
  goto switch_merge_758;

  case_body_762:
  return "同步到大括号";
  goto switch_merge_758;

  case_body_763:
  return "同步到关键字";
  goto switch_merge_758;

  case_default_764:
  return "未知策略";
  goto switch_merge_758;

  switch_merge_758:
  return NULL;
}

