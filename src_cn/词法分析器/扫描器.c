#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块

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

// Forward Declarations - 从导入模块
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

// CN Language Global Struct Forward Declarations
struct 扫描器;

// CN Language Global Struct Definitions
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};

// Global Variables
long long cn_var_最大标识符长度 = 256;
long long cn_var_最大字符串长度 = 4096;
long long cn_var_最大数字长度 = 64;
long long cn_var_扫描器大小 = 24;

// Forward Declarations
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

struct 扫描器* 创建扫描器(char* cn_var_源码内容, long long cn_var_长度) {
  long long r1, r2, r5, r7;
  char* r0;
  char* r6;
  void* r3;
  struct 扫描器* r4;
  struct 扫描器* r8;

  entry:
  r0 = cn_var_源码内容;
  r1 = r0 == 0;
  if (r1) goto if_then_537; else goto if_merge_538;

  if_then_537:
  return 0;
  goto if_merge_538;

  if_merge_538:
  struct 扫描器* cn_var_实例_0;
  r2 = cn_var_扫描器大小;
  r3 = 分配内存(r2);
  cn_var_实例_0 = r3;
  r4 = cn_var_实例_0;
  r5 = r4 == 0;
  if (r5) goto if_then_539; else goto if_merge_540;

  if_then_539:
  return 0;
  goto if_merge_540;

  if_merge_540:
  r6 = cn_var_源码内容;
  r7 = cn_var_长度;
  r8 = cn_var_实例_0;
  return r8;
}

void 销毁扫描器(struct 扫描器* cn_var_实例) {
  long long r1;
  struct 扫描器* r0;
  struct 扫描器* r2;

  entry:
  r0 = cn_var_实例;
  r1 = r0 != 0;
  if (r1) goto if_then_541; else goto if_merge_542;

  if_then_541:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_542;

  if_merge_542:
  return;
}

struct 词元 下一个词元(struct 扫描器* cn_var_实例) {
  long long r1, r3, r7, r9, r11, r15, r16, r17, r20, r21, r25, r29, r30, r33, r34;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r6;
  struct 扫描器* r8;
  struct 扫描器* r10;
  struct 扫描器* r12;
  struct 扫描器* r19;
  struct 扫描器* r23;
  struct 扫描器* r27;
  struct 扫描器* r31;
  struct 扫描器* r35;
  struct 扫描器* r37;
  _Bool r13;
  _Bool r22;
  _Bool r26;
  struct 词元 r4;
  struct 词元 r18;
  struct 词元 r24;
  struct 词元 r28;
  struct 词元 r32;
  struct 词元 r36;
  struct 词元 r38;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r14;

  entry:
  struct 词元 cn_var_结果_0;
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_543; else goto if_merge_544;

  if_then_543:
  r2 = cn_var_词元类型枚举;
  r3 = r2.错误;
  r4 = cn_var_结果_0;
  return r4;
  goto if_merge_544;

  if_merge_544:
  r5 = cn_var_实例;
  跳过空白字符(r5);
  long long cn_var_起始位置_1;
  r6 = cn_var_实例;
  r7 = r6->位置;
  cn_var_起始位置_1 = r7;
  long long cn_var_起始行号_2;
  r8 = cn_var_实例;
  r9 = r8->行号;
  cn_var_起始行号_2 = r9;
  long long cn_var_起始列号_3;
  r10 = cn_var_实例;
  r11 = r10->列号;
  cn_var_起始列号_3 = r11;
  r12 = cn_var_实例;
  r13 = 是否结束(r12);
  if (r13) goto if_then_545; else goto if_merge_546;

  if_then_545:
  r14 = cn_var_词元类型枚举;
  r15 = r14.结束;
  r16 = cn_var_起始行号_2;
  r17 = cn_var_起始列号_3;
  r18 = cn_var_结果_0;
  return r18;
  goto if_merge_546;

  if_merge_546:
  long long cn_var_c_4;
  r19 = cn_var_实例;
  r20 = 当前字符(r19);
  cn_var_c_4 = r20;
  r21 = cn_var_c_4;
  r22 = 是标识符开头(r21);
  if (r22) goto if_then_547; else goto if_merge_548;

  if_then_547:
  r23 = cn_var_实例;
  r24 = 扫描标识符(r23);
  return r24;
  goto if_merge_548;

  if_merge_548:
  r25 = cn_var_c_4;
  r26 = 是数字(r25);
  if (r26) goto if_then_549; else goto if_merge_550;

  if_then_549:
  r27 = cn_var_实例;
  r28 = 扫描数字(r27);
  return r28;
  goto if_merge_550;

  if_merge_550:
  r29 = cn_var_c_4;
  r30 = r29 == 34;
  if (r30) goto if_then_551; else goto if_merge_552;

  if_then_551:
  r31 = cn_var_实例;
  r32 = 扫描字符串(r31);
  return r32;
  goto if_merge_552;

  if_merge_552:
  r33 = cn_var_c_4;
  r34 = r33 == 39;
  if (r34) goto if_then_553; else goto if_merge_554;

  if_then_553:
  r35 = cn_var_实例;
  r36 = 扫描字符(r35);
  return r36;
  goto if_merge_554;

  if_merge_554:
  r37 = cn_var_实例;
  r38 = 扫描运算符(r37);
  return r38;
}

struct 词元 预览词元(struct 扫描器* cn_var_实例) {
  long long r1, r3, r6, r8, r10, r13, r14, r15;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r7;
  struct 扫描器* r9;
  struct 扫描器* r11;
  struct 词元 r4;
  struct 词元 r12;
  struct 词元 r16;
  enum 词元类型枚举 r2;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_555; else goto if_merge_556;

  if_then_555:
  struct 词元 cn_var_空词元_0;
  r2 = cn_var_词元类型枚举;
  r3 = r2.错误;
  r4 = cn_var_空词元_0;
  return r4;
  goto if_merge_556;

  if_merge_556:
  long long cn_var_保存位置_1;
  r5 = cn_var_实例;
  r6 = r5->位置;
  cn_var_保存位置_1 = r6;
  long long cn_var_保存行号_2;
  r7 = cn_var_实例;
  r8 = r7->行号;
  cn_var_保存行号_2 = r8;
  long long cn_var_保存列号_3;
  r9 = cn_var_实例;
  r10 = r9->列号;
  cn_var_保存列号_3 = r10;
  struct 词元 cn_var_结果_4;
  r11 = cn_var_实例;
  r12 = 下一个词元(r11);
  cn_var_结果_4 = r12;
  r13 = cn_var_保存位置_1;
  r14 = cn_var_保存行号_2;
  r15 = cn_var_保存列号_3;
  r16 = cn_var_结果_4;
  return r16;
}

long long 当前字符(struct 扫描器* cn_var_实例) {
  long long r0, r2, r4, r6, r7, r11, r12;
  char* r9;
  struct 扫描器* r1;
  struct 扫描器* r3;
  struct 扫描器* r5;
  struct 扫描器* r8;
  struct 扫描器* r10;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_560; else goto logic_rhs_559;

  if_then_557:
  return 0;
  goto if_merge_558;

  if_merge_558:
  r8 = cn_var_实例;
  r9 = r8->源码;
  r10 = cn_var_实例;
  r11 = r10->位置;
  r12 = *(void*)cn_rt_array_get_element(r9, r11, 8);
  return r12;

  logic_rhs_559:
  r3 = cn_var_实例;
  r4 = r3->位置;
  r5 = cn_var_实例;
  r6 = r5->源码长度;
  r7 = r4 >= r6;
  goto logic_merge_560;

  logic_merge_560:
  if (r7) goto if_then_557; else goto if_merge_558;
  return 0;
}

long long 预览字符(struct 扫描器* cn_var_实例, long long cn_var_偏移) {
  long long r1, r2, r3, r4, r5, r7, r8, r9, r10, r12, r13, r16, r17;
  char* r15;
  struct 扫描器* r0;
  struct 扫描器* r6;
  struct 扫描器* r11;
  struct 扫描器* r14;

  entry:
  long long cn_var_目标位置_0;
  r0 = cn_var_实例;
  r1 = r0->位置;
  r2 = cn_var_偏移;
  r3 = r1 + r2;
  cn_var_目标位置_0 = r3;
  r6 = cn_var_实例;
  r7 = r6 == 0;
  if (r7) goto logic_merge_566; else goto logic_rhs_565;

  if_then_561:
  return 0;
  goto if_merge_562;

  if_merge_562:
  r14 = cn_var_实例;
  r15 = r14->源码;
  r16 = cn_var_目标位置_0;
  r17 = *(void*)cn_rt_array_get_element(r15, r16, 8);
  return r17;

  logic_rhs_563:
  r10 = cn_var_目标位置_0;
  r11 = cn_var_实例;
  r12 = r11->源码长度;
  r13 = r10 >= r12;
  goto logic_merge_564;

  logic_merge_564:
  if (r13) goto if_then_561; else goto if_merge_562;

  logic_rhs_565:
  r8 = cn_var_目标位置_0;
  r9 = r8 < 0;
  goto logic_merge_566;

  logic_merge_566:
  if (r9) goto logic_merge_564; else goto logic_rhs_563;
  return 0;
}

long long 前进(struct 扫描器* cn_var_实例) {
  long long r1, r2, r3, r4, r6, r7, r8, r9, r11, r12, r14, r15, r16;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r10;
  struct 扫描器* r13;

  entry:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = r2 == 0;
  if (r3) goto if_then_567; else goto if_merge_568;

  if_then_567:
  r4 = cn_var_c_0;
  return r4;
  goto if_merge_568;

  if_merge_568:
  r5 = cn_var_实例;
  r6 = r5->位置;
  r7 = r6 + 1;
  r8 = cn_var_c_0;
  r9 = r8 == 10;
  if (r9) goto if_then_569; else goto if_else_570;

  if_then_569:
  r10 = cn_var_实例;
  r11 = r10->行号;
  r12 = r11 + 1;
  goto if_merge_571;

  if_else_570:
  r13 = cn_var_实例;
  r14 = r13->列号;
  r15 = r14 + 1;
  goto if_merge_571;

  if_merge_571:
  r16 = cn_var_c_0;
  return r16;
}

_Bool 是否结束(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_572; else goto if_merge_573;

  if_then_572:
  return 1;
  goto if_merge_573;

  if_merge_573:
  r2 = cn_var_实例;
  r3 = r2->位置;
  r4 = cn_var_实例;
  r5 = r4->源码长度;
  r6 = r3 >= r5;
  return r6;
}

void 跳过空白字符(struct 扫描器* cn_var_实例) {
  long long r1, r2, r5, r6, r8, r11, r13;
  struct 扫描器* r0;
  struct 扫描器* r4;
  struct 扫描器* r7;
  struct 扫描器* r10;
  struct 扫描器* r12;
  struct 扫描器* r15;
  _Bool r3;
  _Bool r9;
  _Bool r14;
  _Bool r16;

  entry:
  goto while_cond_574;

  while_cond_574:
  if (1) goto while_body_575; else goto while_exit_576;

  while_body_575:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = 是空白字符(r2);
  if (r3) goto if_then_577; else goto if_merge_578;

  while_exit_576:

  if_then_577:
  r4 = cn_var_实例;
  r5 = 前进(r4);
  goto while_cond_574;
  goto if_merge_578;

  if_merge_578:
  r6 = cn_var_c_0;
  r7 = cn_var_实例;
  r8 = 预览字符(r7, 1);
  r9 = 是单行注释开始(r6, r8);
  if (r9) goto if_then_579; else goto if_merge_580;

  if_then_579:
  r10 = cn_var_实例;
  跳过单行注释(r10);
  goto while_cond_574;
  goto if_merge_580;

  if_merge_580:
  r11 = cn_var_c_0;
  r12 = cn_var_实例;
  r13 = 预览字符(r12, 1);
  r14 = 是块注释开始(r11, r13);
  if (r14) goto if_then_581; else goto if_merge_582;

  if_then_581:
  r15 = cn_var_实例;
  r16 = 跳过块注释(r15);
  goto while_cond_574;
  goto if_merge_582;

  if_merge_582:
  goto while_exit_576;
  goto while_cond_574;
  return;
}

void 跳过单行注释(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6, r7, r8, r9, r10, r12;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r11;

  entry:
  r0 = cn_var_实例;
  r1 = 前进(r0);
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_583;

  while_cond_583:
  if (1) goto while_body_584; else goto while_exit_585;

  while_body_584:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r7 = cn_var_c_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_589; else goto logic_rhs_588;

  while_exit_585:

  if_then_586:
  goto while_exit_585;
  goto if_merge_587;

  if_merge_587:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_583;

  logic_rhs_588:
  r9 = cn_var_c_0;
  r10 = r9 == 10;
  goto logic_merge_589;

  logic_merge_589:
  if (r10) goto if_then_586; else goto if_merge_587;
  return;
}

_Bool 跳过块注释(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6, r7, r8, r9, r10, r12, r13, r15, r17, r19;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r11;
  struct 扫描器* r14;
  struct 扫描器* r16;
  struct 扫描器* r18;

  entry:
  r0 = cn_var_实例;
  r1 = 前进(r0);
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_590;

  while_cond_590:
  if (1) goto while_body_591; else goto while_exit_592;

  while_body_591:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r6 = cn_var_c_0;
  r7 = r6 == 0;
  if (r7) goto if_then_593; else goto if_merge_594;

  while_exit_592:

  if_then_593:
  return 0;
  goto if_merge_594;

  if_merge_594:
  r9 = cn_var_c_0;
  r10 = r9 == 42;
  if (r10) goto logic_rhs_597; else goto logic_merge_598;

  if_then_595:
  r14 = cn_var_实例;
  r15 = 前进(r14);
  r16 = cn_var_实例;
  r17 = 前进(r16);
  return 1;
  goto if_merge_596;

  if_merge_596:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_590;

  logic_rhs_597:
  r11 = cn_var_实例;
  r12 = 预览字符(r11, 1);
  r13 = r12 == 47;
  goto logic_merge_598;

  logic_merge_598:
  if (r13) goto if_then_595; else goto if_merge_596;
  return 0;
}

struct 词元 扫描标识符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r10, r12, r14, r15, r16, r17, r18, r19, r20, r21, r25, r26, r27, r30, r35, r36, r37;
  char* r24;
  char* r28;
  void* r22;
  void* r29;
  void* r31;
  void* r34;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r11;
  struct 扫描器* r13;
  struct 扫描器* r23;
  _Bool r9;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r33;
  struct 词元 r38;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始位置_1;
  r0 = cn_var_实例;
  r1 = r0->位置;
  cn_var_起始位置_1 = r1;
  long long cn_var_起始行号_2;
  r2 = cn_var_实例;
  r3 = r2->行号;
  cn_var_起始行号_2 = r3;
  long long cn_var_起始列号_3;
  r4 = cn_var_实例;
  r5 = r4->列号;
  cn_var_起始列号_3 = r5;
  goto while_cond_599;

  while_cond_599:
  if (1) goto while_body_600; else goto while_exit_601;

  while_body_600:
  long long cn_var_c_4;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_4 = r7;
  r8 = cn_var_c_4;
  r9 = 是标识符字符(r8);
  r10 = !r9;
  if (r10) goto if_then_602; else goto if_merge_603;

  while_exit_601:
  long long cn_var_长度_5;
  r13 = cn_var_实例;
  r14 = r13->位置;
  r15 = cn_var_起始位置_1;
  r16 = r14 - r15;
  cn_var_长度_5 = r16;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 256);
  r17 = cn_var_长度_5;
  r18 = cn_var_最大标识符长度;
  r19 = r17 >= r18;
  if (r19) goto if_then_604; else goto if_merge_605;

  if_then_602:
  goto while_exit_601;
  goto if_merge_603;

  if_merge_603:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_599;

  if_then_604:
  r20 = cn_var_最大标识符长度;
  r21 = r20 - 1;
  cn_var_长度_5 = r21;
  goto if_merge_605;

  if_merge_605:
  r22 = cn_var_缓冲区;
  r23 = cn_var_实例;
  r24 = r23->源码;
  r25 = cn_var_起始位置_1;
  r26 = r24 + r25;
  r27 = cn_var_长度_5;
  r28 = 限长复制字符串(r22, r26, r27);
  r29 = cn_var_缓冲区;
  r30 = cn_var_长度_5;
    { long long _tmp_i2 = 0; cn_rt_array_set_element(r29, r30, &_tmp_i2, 8); }
  enum 词元类型枚举 cn_var_类型_6;
  r31 = cn_var_缓冲区;
  r32 = 查找关键字(r31);
  cn_var_类型_6 = r32;
  r33 = cn_var_类型_6;
  r34 = cn_var_缓冲区;
  r35 = cn_var_起始行号_2;
  r36 = cn_var_起始列号_3;
  r37 = cn_var_长度_5;
  r38 = cn_var_结果_0;
  return r38;
}

struct 词元 扫描数字(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r10, r13, r15, r17, r18, r20, r22, r24, r25, r26, r28, r29, r30, r31, r33, r35, r38, r40, r42, r43, r45, r47, r49, r50, r51, r53, r54, r55, r56, r58, r60, r63, r65, r67, r68, r70, r72, r74, r75, r76, r78, r79, r80, r81, r84, r85, r87, r89, r91, r92, r93, r94, r96, r99, r101, r102, r104, r106, r108, r109, r110, r111, r112, r113, r115, r117, r118, r119, r120, r121, r122, r124, r126, r127, r129, r131, r133, r134, r135, r138, r140, r141, r142, r143, r144;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r9;
  struct 扫描器* r12;
  struct 扫描器* r14;
  struct 扫描器* r16;
  struct 扫描器* r21;
  struct 扫描器* r23;
  struct 扫描器* r34;
  struct 扫描器* r37;
  struct 扫描器* r39;
  struct 扫描器* r41;
  struct 扫描器* r46;
  struct 扫描器* r48;
  struct 扫描器* r59;
  struct 扫描器* r62;
  struct 扫描器* r64;
  struct 扫描器* r66;
  struct 扫描器* r71;
  struct 扫描器* r73;
  struct 扫描器* r83;
  struct 扫描器* r88;
  struct 扫描器* r90;
  struct 扫描器* r95;
  struct 扫描器* r98;
  struct 扫描器* r100;
  struct 扫描器* r105;
  struct 扫描器* r107;
  struct 扫描器* r114;
  struct 扫描器* r116;
  struct 扫描器* r123;
  struct 扫描器* r125;
  struct 扫描器* r130;
  struct 扫描器* r132;
  _Bool r11;
  _Bool r19;
  _Bool r36;
  _Bool r44;
  _Bool r61;
  _Bool r69;
  _Bool r86;
  _Bool r97;
  _Bool r103;
  _Bool r128;
  _Bool r136;
  struct 词元 r32;
  struct 词元 r57;
  struct 词元 r82;
  struct 词元 r145;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r52;
  enum 词元类型枚举 r77;
  enum 词元类型枚举 r137;
  enum 词元类型枚举 r139;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始位置_1;
  r0 = cn_var_实例;
  r1 = r0->位置;
  cn_var_起始位置_1 = r1;
  long long cn_var_起始行号_2;
  r2 = cn_var_实例;
  r3 = r2->行号;
  cn_var_起始行号_2 = r3;
  long long cn_var_起始列号_3;
  r4 = cn_var_实例;
  r5 = r4->列号;
  cn_var_起始列号_3 = r5;
  _Bool cn_var_是浮点数_4;
  cn_var_是浮点数_4 = 0;
  long long cn_var_c_5;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_5 = r7;
  r8 = cn_var_c_5;
  r9 = cn_var_实例;
  r10 = 预览字符(r9, 1);
  r11 = 是十六进制前缀(r8, r10);
  if (r11) goto if_then_606; else goto if_merge_607;

  if_then_606:
  r12 = cn_var_实例;
  r13 = 前进(r12);
  r14 = cn_var_实例;
  r15 = 前进(r14);
  goto while_cond_608;

  if_merge_607:
  r33 = cn_var_c_5;
  r34 = cn_var_实例;
  r35 = 预览字符(r34, 1);
  r36 = 是二进制前缀(r33, r35);
  if (r36) goto if_then_613; else goto if_merge_614;

  while_cond_608:
  if (1) goto while_body_609; else goto while_exit_610;

  while_body_609:
  r16 = cn_var_实例;
  r17 = 当前字符(r16);
  cn_var_c_5 = r17;
  r18 = cn_var_c_5;
  r19 = 是十六进制数字(r18);
  r20 = !r19;
  if (r20) goto if_then_611; else goto if_merge_612;

  while_exit_610:
  long long cn_var_长度_6;
  r23 = cn_var_实例;
  r24 = r23->位置;
  r25 = cn_var_起始位置_1;
  r26 = r24 - r25;
  cn_var_长度_6 = r26;
  r27 = cn_var_词元类型枚举;
  r28 = r27.整数字面量;
  r29 = cn_var_起始行号_2;
  r30 = cn_var_起始列号_3;
  r31 = cn_var_长度_6;
  r32 = cn_var_结果_0;
  return r32;
  goto if_merge_607;

  if_then_611:
  goto while_exit_610;
  goto if_merge_612;

  if_merge_612:
  r21 = cn_var_实例;
  r22 = 前进(r21);
  goto while_cond_608;

  if_then_613:
  r37 = cn_var_实例;
  r38 = 前进(r37);
  r39 = cn_var_实例;
  r40 = 前进(r39);
  goto while_cond_615;

  if_merge_614:
  r58 = cn_var_c_5;
  r59 = cn_var_实例;
  r60 = 预览字符(r59, 1);
  r61 = 是八进制前缀(r58, r60);
  if (r61) goto if_then_620; else goto if_merge_621;

  while_cond_615:
  if (1) goto while_body_616; else goto while_exit_617;

  while_body_616:
  r41 = cn_var_实例;
  r42 = 当前字符(r41);
  cn_var_c_5 = r42;
  r43 = cn_var_c_5;
  r44 = 是二进制数字(r43);
  r45 = !r44;
  if (r45) goto if_then_618; else goto if_merge_619;

  while_exit_617:
  long long cn_var_长度_7;
  r48 = cn_var_实例;
  r49 = r48->位置;
  r50 = cn_var_起始位置_1;
  r51 = r49 - r50;
  cn_var_长度_7 = r51;
  r52 = cn_var_词元类型枚举;
  r53 = r52.整数字面量;
  r54 = cn_var_起始行号_2;
  r55 = cn_var_起始列号_3;
  r56 = cn_var_长度_7;
  r57 = cn_var_结果_0;
  return r57;
  goto if_merge_614;

  if_then_618:
  goto while_exit_617;
  goto if_merge_619;

  if_merge_619:
  r46 = cn_var_实例;
  r47 = 前进(r46);
  goto while_cond_615;

  if_then_620:
  r62 = cn_var_实例;
  r63 = 前进(r62);
  r64 = cn_var_实例;
  r65 = 前进(r64);
  goto while_cond_622;

  if_merge_621:
  goto while_cond_627;

  while_cond_622:
  if (1) goto while_body_623; else goto while_exit_624;

  while_body_623:
  r66 = cn_var_实例;
  r67 = 当前字符(r66);
  cn_var_c_5 = r67;
  r68 = cn_var_c_5;
  r69 = 是八进制数字(r68);
  r70 = !r69;
  if (r70) goto if_then_625; else goto if_merge_626;

  while_exit_624:
  long long cn_var_长度_8;
  r73 = cn_var_实例;
  r74 = r73->位置;
  r75 = cn_var_起始位置_1;
  r76 = r74 - r75;
  cn_var_长度_8 = r76;
  r77 = cn_var_词元类型枚举;
  r78 = r77.整数字面量;
  r79 = cn_var_起始行号_2;
  r80 = cn_var_起始列号_3;
  r81 = cn_var_长度_8;
  r82 = cn_var_结果_0;
  return r82;
  goto if_merge_621;

  if_then_625:
  goto while_exit_624;
  goto if_merge_626;

  if_merge_626:
  r71 = cn_var_实例;
  r72 = 前进(r71);
  goto while_cond_622;

  while_cond_627:
  if (1) goto while_body_628; else goto while_exit_629;

  while_body_628:
  r83 = cn_var_实例;
  r84 = 当前字符(r83);
  cn_var_c_5 = r84;
  r85 = cn_var_c_5;
  r86 = 是数字(r85);
  r87 = !r86;
  if (r87) goto if_then_630; else goto if_merge_631;

  while_exit_629:
  r90 = cn_var_实例;
  r91 = 当前字符(r90);
  cn_var_c_5 = r91;
  r93 = cn_var_c_5;
  r94 = r93 == 46;
  if (r94) goto logic_rhs_634; else goto logic_merge_635;

  if_then_630:
  goto while_exit_629;
  goto if_merge_631;

  if_merge_631:
  r88 = cn_var_实例;
  r89 = 前进(r88);
  goto while_cond_627;

  if_then_632:
  cn_var_是浮点数_4 = 1;
  r98 = cn_var_实例;
  r99 = 前进(r98);
  goto while_cond_636;

  if_merge_633:
  r107 = cn_var_实例;
  r108 = 当前字符(r107);
  cn_var_c_5 = r108;
  r110 = cn_var_c_5;
  r111 = r110 == 101;
  if (r111) goto logic_merge_644; else goto logic_rhs_643;

  logic_rhs_634:
  r95 = cn_var_实例;
  r96 = 预览字符(r95, 1);
  r97 = 是数字(r96);
  goto logic_merge_635;

  logic_merge_635:
  if (r97) goto if_then_632; else goto if_merge_633;

  while_cond_636:
  if (1) goto while_body_637; else goto while_exit_638;

  while_body_637:
  r100 = cn_var_实例;
  r101 = 当前字符(r100);
  cn_var_c_5 = r101;
  r102 = cn_var_c_5;
  r103 = 是数字(r102);
  r104 = !r103;
  if (r104) goto if_then_639; else goto if_merge_640;

  while_exit_638:
  goto if_merge_633;

  if_then_639:
  goto while_exit_638;
  goto if_merge_640;

  if_merge_640:
  r105 = cn_var_实例;
  r106 = 前进(r105);
  goto while_cond_636;

  if_then_641:
  cn_var_是浮点数_4 = 1;
  r114 = cn_var_实例;
  r115 = 前进(r114);
  r116 = cn_var_实例;
  r117 = 当前字符(r116);
  cn_var_c_5 = r117;
  r119 = cn_var_c_5;
  r120 = r119 == 43;
  if (r120) goto logic_merge_648; else goto logic_rhs_647;

  if_merge_642:
  long long cn_var_长度_9;
  r132 = cn_var_实例;
  r133 = r132->位置;
  r134 = cn_var_起始位置_1;
  r135 = r133 - r134;
  cn_var_长度_9 = r135;
  r136 = cn_var_是浮点数_4;
  r137 = cn_var_词元类型枚举;
  r138 = r137.浮点字面量;
  r139 = cn_var_词元类型枚举;
  r140 = r139.整数字面量;
  r141 = (r136 ? r138 : r140);
  r142 = cn_var_起始行号_2;
  r143 = cn_var_起始列号_3;
  r144 = cn_var_长度_9;
  r145 = cn_var_结果_0;
  return r145;

  logic_rhs_643:
  r112 = cn_var_c_5;
  r113 = r112 == 69;
  goto logic_merge_644;

  logic_merge_644:
  if (r113) goto if_then_641; else goto if_merge_642;

  if_then_645:
  r123 = cn_var_实例;
  r124 = 前进(r123);
  goto if_merge_646;

  if_merge_646:
  goto while_cond_649;

  logic_rhs_647:
  r121 = cn_var_c_5;
  r122 = r121 == 45;
  goto logic_merge_648;

  logic_merge_648:
  if (r122) goto if_then_645; else goto if_merge_646;

  while_cond_649:
  if (1) goto while_body_650; else goto while_exit_651;

  while_body_650:
  r125 = cn_var_实例;
  r126 = 当前字符(r125);
  cn_var_c_5 = r126;
  r127 = cn_var_c_5;
  r128 = 是数字(r127);
  r129 = !r128;
  if (r129) goto if_then_652; else goto if_merge_653;

  while_exit_651:
  goto if_merge_642;

  if_then_652:
  goto while_exit_651;
  goto if_merge_653;

  if_merge_653:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  goto while_cond_649;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符串(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r9, r11, r12, r13, r15, r16, r18, r19, r20, r22, r24, r25, r26, r28, r30, r32, r33, r34, r36, r37, r38;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r17;
  struct 扫描器* r21;
  struct 扫描器* r23;
  struct 扫描器* r27;
  struct 扫描器* r29;
  struct 扫描器* r35;
  struct 词元 r14;
  struct 词元 r39;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r31;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例;
  r1 = r0->行号;
  cn_var_起始行号_1 = r1;
  long long cn_var_起始列号_2;
  r2 = cn_var_实例;
  r3 = r2->列号;
  cn_var_起始列号_2 = r3;
  r4 = cn_var_实例;
  r5 = 前进(r4);
  goto while_cond_654;

  while_cond_654:
  if (1) goto while_body_655; else goto while_exit_656;

  while_body_655:
  long long cn_var_c_3;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_3 = r7;
  r8 = cn_var_c_3;
  r9 = r8 == 0;
  if (r9) goto if_then_657; else goto if_merge_658;

  while_exit_656:
  r31 = cn_var_词元类型枚举;
  r32 = r31.字符串字面量;
  r33 = cn_var_起始行号_1;
  r34 = cn_var_起始列号_2;
  r35 = cn_var_实例;
  r36 = r35->位置;
  r37 = cn_var_起始列号_2;
  r38 = r36 - r37;
  r39 = cn_var_结果_0;
  return r39;

  if_then_657:
  r10 = cn_var_词元类型枚举;
  r11 = r10.错误;
  r12 = cn_var_起始行号_1;
  r13 = cn_var_起始列号_2;
  r14 = cn_var_结果_0;
  return r14;
  goto if_merge_658;

  if_merge_658:
  r15 = cn_var_c_3;
  r16 = r15 == 34;
  if (r16) goto if_then_659; else goto if_merge_660;

  if_then_659:
  r17 = cn_var_实例;
  r18 = 前进(r17);
  goto while_exit_656;
  goto if_merge_660;

  if_merge_660:
  r19 = cn_var_c_3;
  r20 = r19 == 92;
  if (r20) goto if_then_661; else goto if_merge_662;

  if_then_661:
  r21 = cn_var_实例;
  r22 = 前进(r21);
  r23 = cn_var_实例;
  r24 = 当前字符(r23);
  cn_var_c_3 = r24;
  r25 = cn_var_c_3;
  r26 = r25 != 0;
  if (r26) goto if_then_663; else goto if_merge_664;

  if_merge_662:
  r29 = cn_var_实例;
  r30 = 前进(r29);
  goto while_cond_654;

  if_then_663:
  r27 = cn_var_实例;
  r28 = 前进(r27);
  goto if_merge_664;

  if_merge_664:
  goto while_cond_654;
  goto if_merge_662;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r9, r10, r11, r12, r14, r15, r16, r18, r19, r21, r23, r24, r25, r27, r29, r31, r32, r33, r35, r36, r37, r40, r42, r43, r44, r46, r47, r48;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r20;
  struct 扫描器* r22;
  struct 扫描器* r26;
  struct 扫描器* r28;
  struct 扫描器* r30;
  struct 扫描器* r39;
  struct 扫描器* r45;
  struct 词元 r17;
  struct 词元 r38;
  struct 词元 r49;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r34;
  enum 词元类型枚举 r41;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例;
  r1 = r0->行号;
  cn_var_起始行号_1 = r1;
  long long cn_var_起始列号_2;
  r2 = cn_var_实例;
  r3 = r2->列号;
  cn_var_起始列号_2 = r3;
  r4 = cn_var_实例;
  r5 = 前进(r4);
  long long cn_var_c_3;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_3 = r7;
  r9 = cn_var_c_3;
  r10 = r9 == 0;
  if (r10) goto logic_merge_668; else goto logic_rhs_667;

  if_then_665:
  r13 = cn_var_词元类型枚举;
  r14 = r13.错误;
  r15 = cn_var_起始行号_1;
  r16 = cn_var_起始列号_2;
  r17 = cn_var_结果_0;
  return r17;
  goto if_merge_666;

  if_merge_666:
  r18 = cn_var_c_3;
  r19 = r18 == 92;
  if (r19) goto if_then_669; else goto if_else_670;

  logic_rhs_667:
  r11 = cn_var_c_3;
  r12 = r11 == 39;
  goto logic_merge_668;

  logic_merge_668:
  if (r12) goto if_then_665; else goto if_merge_666;

  if_then_669:
  r20 = cn_var_实例;
  r21 = 前进(r20);
  r22 = cn_var_实例;
  r23 = 当前字符(r22);
  cn_var_c_3 = r23;
  r24 = cn_var_c_3;
  r25 = r24 != 0;
  if (r25) goto if_then_672; else goto if_merge_673;

  if_else_670:
  r28 = cn_var_实例;
  r29 = 前进(r28);
  goto if_merge_671;

  if_merge_671:
  r30 = cn_var_实例;
  r31 = 当前字符(r30);
  cn_var_c_3 = r31;
  r32 = cn_var_c_3;
  r33 = r32 != 39;
  if (r33) goto if_then_674; else goto if_merge_675;

  if_then_672:
  r26 = cn_var_实例;
  r27 = 前进(r26);
  goto if_merge_673;

  if_merge_673:
  goto if_merge_671;

  if_then_674:
  r34 = cn_var_词元类型枚举;
  r35 = r34.错误;
  r36 = cn_var_起始行号_1;
  r37 = cn_var_起始列号_2;
  r38 = cn_var_结果_0;
  return r38;
  goto if_merge_675;

  if_merge_675:
  r39 = cn_var_实例;
  r40 = 前进(r39);
  r41 = cn_var_词元类型枚举;
  r42 = r41.字符字面量;
  r43 = cn_var_起始行号_1;
  r44 = cn_var_起始列号_2;
  r45 = cn_var_实例;
  r46 = r45->位置;
  r47 = cn_var_起始列号_2;
  r48 = r46 - r47;
  r49 = cn_var_结果_0;
  return r49;
}

struct 词元 扫描运算符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r34, r36, r37, r39, r41, r43, r44, r46, r48, r49, r51, r53, r55, r57, r59, r61, r62, r64, r66, r68, r69, r71, r73, r75, r76, r78, r80, r81, r83, r85, r87, r88, r90, r92, r93, r95, r97, r99, r100, r102, r104, r106, r107, r109, r111, r113, r115, r117, r119, r121, r123, r125, r127, r129, r131, r133, r135, r137, r139, r141, r142, r144, r145, r146;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r33;
  struct 扫描器* r35;
  struct 扫描器* r38;
  struct 扫描器* r40;
  struct 扫描器* r42;
  struct 扫描器* r45;
  struct 扫描器* r47;
  struct 扫描器* r50;
  struct 扫描器* r52;
  struct 扫描器* r54;
  struct 扫描器* r56;
  struct 扫描器* r58;
  struct 扫描器* r60;
  struct 扫描器* r63;
  struct 扫描器* r65;
  struct 扫描器* r67;
  struct 扫描器* r70;
  struct 扫描器* r72;
  struct 扫描器* r74;
  struct 扫描器* r77;
  struct 扫描器* r79;
  struct 扫描器* r82;
  struct 扫描器* r84;
  struct 扫描器* r86;
  struct 扫描器* r89;
  struct 扫描器* r91;
  struct 扫描器* r94;
  struct 扫描器* r96;
  struct 扫描器* r98;
  struct 扫描器* r101;
  struct 扫描器* r103;
  struct 扫描器* r105;
  struct 扫描器* r108;
  struct 扫描器* r110;
  struct 扫描器* r112;
  struct 扫描器* r114;
  struct 扫描器* r116;
  struct 扫描器* r118;
  struct 扫描器* r120;
  struct 扫描器* r122;
  struct 扫描器* r124;
  struct 扫描器* r126;
  struct 扫描器* r128;
  struct 扫描器* r130;
  struct 扫描器* r132;
  struct 扫描器* r134;
  struct 扫描器* r136;
  struct 扫描器* r143;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r12;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r21;
  _Bool r22;
  _Bool r23;
  _Bool r24;
  _Bool r25;
  _Bool r26;
  _Bool r27;
  _Bool r28;
  _Bool r29;
  _Bool r30;
  _Bool r31;
  _Bool r32;
  enum 词元类型枚举 r140;
  struct 词元 r147;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r138;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例;
  r1 = r0->行号;
  cn_var_起始行号_1 = r1;
  long long cn_var_起始列号_2;
  r2 = cn_var_实例;
  r3 = r2->列号;
  cn_var_起始列号_2 = r3;
  long long cn_var_c_3;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_3 = r5;
  enum 词元类型枚举 cn_var_类型_4;
  r6 = cn_var_词元类型枚举;
  r7 = r6.错误;
  cn_var_类型_4 = r7;
  r8 = cn_var_c_3;
  r9 = r8 == 43;
  if (r9) goto case_body_677; else goto switch_check_702;

  switch_check_702:
  r10 = r8 == 45;
  if (r10) goto case_body_678; else goto switch_check_703;

  switch_check_703:
  r11 = r8 == 42;
  if (r11) goto case_body_679; else goto switch_check_704;

  switch_check_704:
  r12 = r8 == 47;
  if (r12) goto case_body_680; else goto switch_check_705;

  switch_check_705:
  r13 = r8 == 37;
  if (r13) goto case_body_681; else goto switch_check_706;

  switch_check_706:
  r14 = r8 == 61;
  if (r14) goto case_body_682; else goto switch_check_707;

  switch_check_707:
  r15 = r8 == 33;
  if (r15) goto case_body_683; else goto switch_check_708;

  switch_check_708:
  r16 = r8 == 60;
  if (r16) goto case_body_684; else goto switch_check_709;

  switch_check_709:
  r17 = r8 == 62;
  if (r17) goto case_body_685; else goto switch_check_710;

  switch_check_710:
  r18 = r8 == 38;
  if (r18) goto case_body_686; else goto switch_check_711;

  switch_check_711:
  r19 = r8 == 124;
  if (r19) goto case_body_687; else goto switch_check_712;

  switch_check_712:
  r20 = r8 == 94;
  if (r20) goto case_body_688; else goto switch_check_713;

  switch_check_713:
  r21 = r8 == 126;
  if (r21) goto case_body_689; else goto switch_check_714;

  switch_check_714:
  r22 = r8 == 40;
  if (r22) goto case_body_690; else goto switch_check_715;

  switch_check_715:
  r23 = r8 == 41;
  if (r23) goto case_body_691; else goto switch_check_716;

  switch_check_716:
  r24 = r8 == 123;
  if (r24) goto case_body_692; else goto switch_check_717;

  switch_check_717:
  r25 = r8 == 125;
  if (r25) goto case_body_693; else goto switch_check_718;

  switch_check_718:
  r26 = r8 == 91;
  if (r26) goto case_body_694; else goto switch_check_719;

  switch_check_719:
  r27 = r8 == 93;
  if (r27) goto case_body_695; else goto switch_check_720;

  switch_check_720:
  r28 = r8 == 59;
  if (r28) goto case_body_696; else goto switch_check_721;

  switch_check_721:
  r29 = r8 == 44;
  if (r29) goto case_body_697; else goto switch_check_722;

  switch_check_722:
  r30 = r8 == 46;
  if (r30) goto case_body_698; else goto switch_check_723;

  switch_check_723:
  r31 = r8 == 58;
  if (r31) goto case_body_699; else goto switch_check_724;

  switch_check_724:
  r32 = r8 == 63;
  if (r32) goto case_body_700; else goto case_default_701;

  case_body_677:
  r33 = cn_var_实例;
  r34 = 前进(r33);
  r35 = cn_var_实例;
  r36 = 当前字符(r35);
  r37 = r36 == 43;
  if (r37) goto if_then_725; else goto if_else_726;

  if_then_725:
  r38 = cn_var_实例;
  r39 = 前进(r38);
  cn_var_类型_4 = 66;
  goto if_merge_727;

  if_else_726:
  cn_var_类型_4 = 45;
  goto if_merge_727;

  if_merge_727:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_678:
  r40 = cn_var_实例;
  r41 = 前进(r40);
  r42 = cn_var_实例;
  r43 = 当前字符(r42);
  r44 = r43 == 62;
  if (r44) goto if_then_728; else goto if_else_729;

  if_then_728:
  r45 = cn_var_实例;
  r46 = 前进(r45);
  cn_var_类型_4 = 68;
  goto if_merge_730;

  if_else_729:
  r47 = cn_var_实例;
  r48 = 当前字符(r47);
  r49 = r48 == 45;
  if (r49) goto if_then_731; else goto if_else_732;

  if_merge_730:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_731:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  cn_var_类型_4 = 67;
  goto if_merge_733;

  if_else_732:
  cn_var_类型_4 = 46;
  goto if_merge_733;

  if_merge_733:
  goto if_merge_730;

  case_body_679:
  r52 = cn_var_实例;
  r53 = 前进(r52);
  cn_var_类型_4 = 47;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_680:
  r54 = cn_var_实例;
  r55 = 前进(r54);
  cn_var_类型_4 = 48;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_681:
  r56 = cn_var_实例;
  r57 = 前进(r56);
  cn_var_类型_4 = 49;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_682:
  r58 = cn_var_实例;
  r59 = 前进(r58);
  r60 = cn_var_实例;
  r61 = 当前字符(r60);
  r62 = r61 == 61;
  if (r62) goto if_then_734; else goto if_else_735;

  if_then_734:
  r63 = cn_var_实例;
  r64 = 前进(r63);
  cn_var_类型_4 = 50;
  goto if_merge_736;

  if_else_735:
  cn_var_类型_4 = 51;
  goto if_merge_736;

  if_merge_736:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_683:
  r65 = cn_var_实例;
  r66 = 前进(r65);
  r67 = cn_var_实例;
  r68 = 当前字符(r67);
  r69 = r68 == 61;
  if (r69) goto if_then_737; else goto if_else_738;

  if_then_737:
  r70 = cn_var_实例;
  r71 = 前进(r70);
  cn_var_类型_4 = 52;
  goto if_merge_739;

  if_else_738:
  cn_var_类型_4 = 59;
  goto if_merge_739;

  if_merge_739:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_684:
  r72 = cn_var_实例;
  r73 = 前进(r72);
  r74 = cn_var_实例;
  r75 = 当前字符(r74);
  r76 = r75 == 60;
  if (r76) goto if_then_740; else goto if_else_741;

  if_then_740:
  r77 = cn_var_实例;
  r78 = 前进(r77);
  cn_var_类型_4 = 64;
  goto if_merge_742;

  if_else_741:
  r79 = cn_var_实例;
  r80 = 当前字符(r79);
  r81 = r80 == 61;
  if (r81) goto if_then_743; else goto if_else_744;

  if_merge_742:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_743:
  r82 = cn_var_实例;
  r83 = 前进(r82);
  cn_var_类型_4 = 54;
  goto if_merge_745;

  if_else_744:
  cn_var_类型_4 = 53;
  goto if_merge_745;

  if_merge_745:
  goto if_merge_742;

  case_body_685:
  r84 = cn_var_实例;
  r85 = 前进(r84);
  r86 = cn_var_实例;
  r87 = 当前字符(r86);
  r88 = r87 == 62;
  if (r88) goto if_then_746; else goto if_else_747;

  if_then_746:
  r89 = cn_var_实例;
  r90 = 前进(r89);
  cn_var_类型_4 = 65;
  goto if_merge_748;

  if_else_747:
  r91 = cn_var_实例;
  r92 = 当前字符(r91);
  r93 = r92 == 61;
  if (r93) goto if_then_749; else goto if_else_750;

  if_merge_748:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_749:
  r94 = cn_var_实例;
  r95 = 前进(r94);
  cn_var_类型_4 = 56;
  goto if_merge_751;

  if_else_750:
  cn_var_类型_4 = 55;
  goto if_merge_751;

  if_merge_751:
  goto if_merge_748;

  case_body_686:
  r96 = cn_var_实例;
  r97 = 前进(r96);
  r98 = cn_var_实例;
  r99 = 当前字符(r98);
  r100 = r99 == 38;
  if (r100) goto if_then_752; else goto if_else_753;

  if_then_752:
  r101 = cn_var_实例;
  r102 = 前进(r101);
  cn_var_类型_4 = 57;
  goto if_merge_754;

  if_else_753:
  cn_var_类型_4 = 60;
  goto if_merge_754;

  if_merge_754:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_687:
  r103 = cn_var_实例;
  r104 = 前进(r103);
  r105 = cn_var_实例;
  r106 = 当前字符(r105);
  r107 = r106 == 124;
  if (r107) goto if_then_755; else goto if_else_756;

  if_then_755:
  r108 = cn_var_实例;
  r109 = 前进(r108);
  cn_var_类型_4 = 58;
  goto if_merge_757;

  if_else_756:
  cn_var_类型_4 = 61;
  goto if_merge_757;

  if_merge_757:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_688:
  r110 = cn_var_实例;
  r111 = 前进(r110);
  cn_var_类型_4 = 62;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_689:
  r112 = cn_var_实例;
  r113 = 前进(r112);
  cn_var_类型_4 = 63;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_690:
  r114 = cn_var_实例;
  r115 = 前进(r114);
  cn_var_类型_4 = 69;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_691:
  r116 = cn_var_实例;
  r117 = 前进(r116);
  cn_var_类型_4 = 70;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_692:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  cn_var_类型_4 = 71;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_693:
  r120 = cn_var_实例;
  r121 = 前进(r120);
  cn_var_类型_4 = 72;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_694:
  r122 = cn_var_实例;
  r123 = 前进(r122);
  cn_var_类型_4 = 73;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_695:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  cn_var_类型_4 = 74;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_696:
  r126 = cn_var_实例;
  r127 = 前进(r126);
  cn_var_类型_4 = 75;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_697:
  r128 = cn_var_实例;
  r129 = 前进(r128);
  cn_var_类型_4 = 76;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_698:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  cn_var_类型_4 = 77;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_699:
  r132 = cn_var_实例;
  r133 = 前进(r132);
  cn_var_类型_4 = 78;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_700:
  r134 = cn_var_实例;
  r135 = 前进(r134);
  cn_var_类型_4 = 79;
  goto switch_merge_676;
  goto switch_merge_676;

  case_default_701:
  r136 = cn_var_实例;
  r137 = 前进(r136);
  r138 = cn_var_词元类型枚举;
  r139 = r138.错误;
  cn_var_类型_4 = r139;
  goto switch_merge_676;
  goto switch_merge_676;

  switch_merge_676:
  r140 = cn_var_类型_4;
  r141 = cn_var_起始行号_1;
  r142 = cn_var_起始列号_2;
  r143 = cn_var_实例;
  r144 = r143->位置;
  r145 = cn_var_起始列号_2;
  r146 = r144 - r145;
  r147 = cn_var_结果_0;
  return r147;
}

