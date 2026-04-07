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
  long long r1, r2, r4, r6;
  char* r0;
  char* r5;
  struct 扫描器* r3;
  struct 扫描器* r7;

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
  分配内存(r2);
  cn_var_实例_0 = /* NONE */;
  r3 = cn_var_实例_0;
  r4 = r3 == 0;
  if (r4) goto if_then_539; else goto if_merge_540;

  if_then_539:
  return 0;
  goto if_merge_540;

  if_merge_540:
  r5 = cn_var_源码内容;
  r6 = cn_var_长度;
  r7 = cn_var_实例_0;
  return r7;
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
  long long r1, r3, r7, r9, r11, r14, r15, r16, r19, r21, r23, r24, r26, r27;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r6;
  struct 扫描器* r8;
  struct 扫描器* r10;
  struct 扫描器* r12;
  struct 扫描器* r18;
  struct 扫描器* r20;
  struct 扫描器* r22;
  struct 扫描器* r25;
  struct 扫描器* r28;
  struct 扫描器* r29;
  struct 词元 r4;
  struct 词元 r17;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r13;

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
  是否结束(r12);
  goto if_then_545;

  if_then_545:
  r13 = cn_var_词元类型枚举;
  r14 = r13.结束;
  r15 = cn_var_起始行号_2;
  r16 = cn_var_起始列号_3;
  r17 = cn_var_结果_0;
  return r17;
  goto if_merge_546;

  if_merge_546:
  long long cn_var_c_4;
  r18 = cn_var_实例;
  当前字符(r18);
  cn_var_c_4 = /* NONE */;
  r19 = cn_var_c_4;
  是标识符开头(r19);
  goto if_then_547;

  if_then_547:
  r20 = cn_var_实例;
  扫描标识符(r20);
  return;
  goto if_merge_548;

  if_merge_548:
  r21 = cn_var_c_4;
  是数字(r21);
  goto if_then_549;

  if_then_549:
  r22 = cn_var_实例;
  扫描数字(r22);
  return;
  goto if_merge_550;

  if_merge_550:
  r23 = cn_var_c_4;
  r24 = r23 == 34;
  if (r24) goto if_then_551; else goto if_merge_552;

  if_then_551:
  r25 = cn_var_实例;
  扫描字符串(r25);
  return;
  goto if_merge_552;

  if_merge_552:
  r26 = cn_var_c_4;
  r27 = r26 == 39;
  if (r27) goto if_then_553; else goto if_merge_554;

  if_then_553:
  r28 = cn_var_实例;
  扫描字符(r28);
  return;
  goto if_merge_554;

  if_merge_554:
  r29 = cn_var_实例;
  扫描运算符(r29);
  return;
}

struct 词元 预览词元(struct 扫描器* cn_var_实例) {
  long long r1, r3, r6, r8, r10, r12, r13, r14;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r7;
  struct 扫描器* r9;
  struct 扫描器* r11;
  struct 词元 r4;
  struct 词元 r15;
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
  下一个词元(r11);
  cn_var_结果_4 = /* NONE */;
  r12 = cn_var_保存位置_1;
  r13 = cn_var_保存行号_2;
  r14 = cn_var_保存列号_3;
  r15 = cn_var_结果_4;
  return r15;
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
  long long r1, r2, r3, r5, r6, r7, r8, r10, r11, r13, r14, r15;
  struct 扫描器* r0;
  struct 扫描器* r4;
  struct 扫描器* r9;
  struct 扫描器* r12;

  entry:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  当前字符(r0);
  cn_var_c_0 = /* NONE */;
  r1 = cn_var_c_0;
  r2 = r1 == 0;
  if (r2) goto if_then_567; else goto if_merge_568;

  if_then_567:
  r3 = cn_var_c_0;
  return r3;
  goto if_merge_568;

  if_merge_568:
  r4 = cn_var_实例;
  r5 = r4->位置;
  r6 = r5 + 1;
  r7 = cn_var_c_0;
  r8 = r7 == 10;
  if (r8) goto if_then_569; else goto if_else_570;

  if_then_569:
  r9 = cn_var_实例;
  r10 = r9->行号;
  r11 = r10 + 1;
  goto if_merge_571;

  if_else_570:
  r12 = cn_var_实例;
  r13 = r12->列号;
  r14 = r13 + 1;
  goto if_merge_571;

  if_merge_571:
  r15 = cn_var_c_0;
  return r15;
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
  long long r1, r3, r6;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r5;
  struct 扫描器* r7;
  struct 扫描器* r8;

  entry:
  goto while_cond_574;

  while_cond_574:
  if (1) goto while_body_575; else goto while_exit_576;

  while_body_575:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  当前字符(r0);
  cn_var_c_0 = /* NONE */;
  r1 = cn_var_c_0;
  是空白字符(r1);
  goto if_then_577;

  while_exit_576:

  if_then_577:
  r2 = cn_var_实例;
  前进(r2);
  goto while_cond_574;
  goto if_merge_578;

  if_merge_578:
  r3 = cn_var_c_0;
  r4 = cn_var_实例;
  预览字符(r4, 1);
  是单行注释开始(r3, /* NONE */);
  goto if_then_579;

  if_then_579:
  r5 = cn_var_实例;
  跳过单行注释(r5);
  goto while_cond_574;
  goto if_merge_580;

  if_merge_580:
  r6 = cn_var_c_0;
  r7 = cn_var_实例;
  预览字符(r7, 1);
  是块注释开始(r6, /* NONE */);
  goto if_then_581;

  if_then_581:
  r8 = cn_var_实例;
  跳过块注释(r8);
  goto while_cond_574;
  goto if_merge_582;

  if_merge_582:
  goto while_exit_576;
  goto while_cond_574;
  return;
}

void 跳过单行注释(struct 扫描器* cn_var_实例) {
  long long r3, r4, r5, r6, r7;
  struct 扫描器* r0;
  struct 扫描器* r1;
  struct 扫描器* r2;
  struct 扫描器* r8;

  entry:
  r0 = cn_var_实例;
  前进(r0);
  r1 = cn_var_实例;
  前进(r1);
  goto while_cond_583;

  while_cond_583:
  if (1) goto while_body_584; else goto while_exit_585;

  while_body_584:
  long long cn_var_c_0;
  r2 = cn_var_实例;
  当前字符(r2);
  cn_var_c_0 = /* NONE */;
  r4 = cn_var_c_0;
  r5 = r4 == 0;
  if (r5) goto logic_merge_589; else goto logic_rhs_588;

  while_exit_585:

  if_then_586:
  goto while_exit_585;
  goto if_merge_587;

  if_merge_587:
  r8 = cn_var_实例;
  前进(r8);
  goto while_cond_583;

  logic_rhs_588:
  r6 = cn_var_c_0;
  r7 = r6 == 10;
  goto logic_merge_589;

  logic_merge_589:
  if (r7) goto if_then_586; else goto if_merge_587;
  return;
}

_Bool 跳过块注释(struct 扫描器* cn_var_实例) {
  long long r3, r4, r5, r6, r7, r9;
  struct 扫描器* r0;
  struct 扫描器* r1;
  struct 扫描器* r2;
  struct 扫描器* r8;
  struct 扫描器* r10;
  struct 扫描器* r11;
  struct 扫描器* r12;

  entry:
  r0 = cn_var_实例;
  前进(r0);
  r1 = cn_var_实例;
  前进(r1);
  goto while_cond_590;

  while_cond_590:
  if (1) goto while_body_591; else goto while_exit_592;

  while_body_591:
  long long cn_var_c_0;
  r2 = cn_var_实例;
  当前字符(r2);
  cn_var_c_0 = /* NONE */;
  r3 = cn_var_c_0;
  r4 = r3 == 0;
  if (r4) goto if_then_593; else goto if_merge_594;

  while_exit_592:

  if_then_593:
  return 0;
  goto if_merge_594;

  if_merge_594:
  r6 = cn_var_c_0;
  r7 = r6 == 42;
  if (r7) goto logic_rhs_597; else goto logic_merge_598;

  if_then_595:
  r10 = cn_var_实例;
  前进(r10);
  r11 = cn_var_实例;
  前进(r11);
  return 1;
  goto if_merge_596;

  if_merge_596:
  r12 = cn_var_实例;
  前进(r12);
  goto while_cond_590;

  logic_rhs_597:
  r8 = cn_var_实例;
  预览字符(r8, 1);
  r9 = /* NONE */ == 47;
  goto logic_merge_598;

  logic_merge_598:
  if (r9) goto if_then_595; else goto if_merge_596;
  return 0;
}

struct 词元 扫描标识符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r11, r12, r13, r14, r15, r16, r17, r18, r22, r23, r24, r26, r30, r31, r32;
  char* r21;
  void* r19;
  void* r25;
  void* r27;
  void* r29;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r9;
  struct 扫描器* r10;
  struct 扫描器* r20;
  enum 词元类型枚举 r28;
  struct 词元 r33;

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
  当前字符(r6);
  cn_var_c_4 = /* NONE */;
  r7 = cn_var_c_4;
  是标识符字符(r7);
  r8 = !/* NONE */;
  if (r8) goto if_then_602; else goto if_merge_603;

  while_exit_601:
  long long cn_var_长度_5;
  r10 = cn_var_实例;
  r11 = r10->位置;
  r12 = cn_var_起始位置_1;
  r13 = r11 - r12;
  cn_var_长度_5 = r13;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 256);
  r14 = cn_var_长度_5;
  r15 = cn_var_最大标识符长度;
  r16 = r14 >= r15;
  if (r16) goto if_then_604; else goto if_merge_605;

  if_then_602:
  goto while_exit_601;
  goto if_merge_603;

  if_merge_603:
  r9 = cn_var_实例;
  前进(r9);
  goto while_cond_599;

  if_then_604:
  r17 = cn_var_最大标识符长度;
  r18 = r17 - 1;
  cn_var_长度_5 = r18;
  goto if_merge_605;

  if_merge_605:
  r19 = cn_var_缓冲区;
  r20 = cn_var_实例;
  r21 = r20->源码;
  r22 = cn_var_起始位置_1;
  r23 = r21 + r22;
  r24 = cn_var_长度_5;
  限长复制字符串(r19, r23, r24);
  r25 = cn_var_缓冲区;
  r26 = cn_var_长度_5;
    { long long _tmp_i2 = 0; cn_rt_array_set_element(r25, r26, &_tmp_i2, 8); }
  enum 词元类型枚举 cn_var_类型_6;
  r27 = cn_var_缓冲区;
  查找关键字(r27);
  cn_var_类型_6 = /* NONE */;
  r28 = cn_var_类型_6;
  r29 = cn_var_缓冲区;
  r30 = cn_var_起始行号_2;
  r31 = cn_var_起始列号_3;
  r32 = cn_var_长度_5;
  r33 = cn_var_结果_0;
  return r33;
}

struct 词元 扫描数字(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r12, r13, r16, r17, r18, r20, r21, r22, r23, r25, r30, r31, r34, r35, r36, r38, r39, r40, r41, r43, r48, r49, r52, r53, r54, r56, r57, r58, r59, r62, r63, r66, r67, r68, r72, r73, r76, r77, r78, r79, r80, r83, r84, r85, r86, r87, r90, r91, r94, r95, r96, r99, r101, r102, r103, r104, r105;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r8;
  struct 扫描器* r9;
  struct 扫描器* r10;
  struct 扫描器* r11;
  struct 扫描器* r14;
  struct 扫描器* r15;
  struct 扫描器* r26;
  struct 扫描器* r27;
  struct 扫描器* r28;
  struct 扫描器* r29;
  struct 扫描器* r32;
  struct 扫描器* r33;
  struct 扫描器* r44;
  struct 扫描器* r45;
  struct 扫描器* r46;
  struct 扫描器* r47;
  struct 扫描器* r50;
  struct 扫描器* r51;
  struct 扫描器* r61;
  struct 扫描器* r64;
  struct 扫描器* r65;
  struct 扫描器* r69;
  struct 扫描器* r70;
  struct 扫描器* r71;
  struct 扫描器* r74;
  struct 扫描器* r75;
  struct 扫描器* r81;
  struct 扫描器* r82;
  struct 扫描器* r88;
  struct 扫描器* r89;
  struct 扫描器* r92;
  struct 扫描器* r93;
  _Bool r97;
  struct 词元 r24;
  struct 词元 r42;
  struct 词元 r60;
  struct 词元 r106;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r37;
  enum 词元类型枚举 r55;
  enum 词元类型枚举 r98;
  enum 词元类型枚举 r100;

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
  当前字符(r6);
  cn_var_c_5 = /* NONE */;
  r7 = cn_var_c_5;
  r8 = cn_var_实例;
  预览字符(r8, 1);
  是十六进制前缀(r7, /* NONE */);
  goto if_then_606;

  if_then_606:
  r9 = cn_var_实例;
  前进(r9);
  r10 = cn_var_实例;
  前进(r10);
  goto while_cond_608;

  if_merge_607:
  r25 = cn_var_c_5;
  r26 = cn_var_实例;
  预览字符(r26, 1);
  是二进制前缀(r25, /* NONE */);
  goto if_then_613;

  while_cond_608:
  if (1) goto while_body_609; else goto while_exit_610;

  while_body_609:
  r11 = cn_var_实例;
  当前字符(r11);
  cn_var_c_5 = /* NONE */;
  r12 = cn_var_c_5;
  是十六进制数字(r12);
  r13 = !/* NONE */;
  if (r13) goto if_then_611; else goto if_merge_612;

  while_exit_610:
  long long cn_var_长度_6;
  r15 = cn_var_实例;
  r16 = r15->位置;
  r17 = cn_var_起始位置_1;
  r18 = r16 - r17;
  cn_var_长度_6 = r18;
  r19 = cn_var_词元类型枚举;
  r20 = r19.整数字面量;
  r21 = cn_var_起始行号_2;
  r22 = cn_var_起始列号_3;
  r23 = cn_var_长度_6;
  r24 = cn_var_结果_0;
  return r24;
  goto if_merge_607;

  if_then_611:
  goto while_exit_610;
  goto if_merge_612;

  if_merge_612:
  r14 = cn_var_实例;
  前进(r14);
  goto while_cond_608;

  if_then_613:
  r27 = cn_var_实例;
  前进(r27);
  r28 = cn_var_实例;
  前进(r28);
  goto while_cond_615;

  if_merge_614:
  r43 = cn_var_c_5;
  r44 = cn_var_实例;
  预览字符(r44, 1);
  是八进制前缀(r43, /* NONE */);
  goto if_then_620;

  while_cond_615:
  if (1) goto while_body_616; else goto while_exit_617;

  while_body_616:
  r29 = cn_var_实例;
  当前字符(r29);
  cn_var_c_5 = /* NONE */;
  r30 = cn_var_c_5;
  是二进制数字(r30);
  r31 = !/* NONE */;
  if (r31) goto if_then_618; else goto if_merge_619;

  while_exit_617:
  long long cn_var_长度_7;
  r33 = cn_var_实例;
  r34 = r33->位置;
  r35 = cn_var_起始位置_1;
  r36 = r34 - r35;
  cn_var_长度_7 = r36;
  r37 = cn_var_词元类型枚举;
  r38 = r37.整数字面量;
  r39 = cn_var_起始行号_2;
  r40 = cn_var_起始列号_3;
  r41 = cn_var_长度_7;
  r42 = cn_var_结果_0;
  return r42;
  goto if_merge_614;

  if_then_618:
  goto while_exit_617;
  goto if_merge_619;

  if_merge_619:
  r32 = cn_var_实例;
  前进(r32);
  goto while_cond_615;

  if_then_620:
  r45 = cn_var_实例;
  前进(r45);
  r46 = cn_var_实例;
  前进(r46);
  goto while_cond_622;

  if_merge_621:
  goto while_cond_627;

  while_cond_622:
  if (1) goto while_body_623; else goto while_exit_624;

  while_body_623:
  r47 = cn_var_实例;
  当前字符(r47);
  cn_var_c_5 = /* NONE */;
  r48 = cn_var_c_5;
  是八进制数字(r48);
  r49 = !/* NONE */;
  if (r49) goto if_then_625; else goto if_merge_626;

  while_exit_624:
  long long cn_var_长度_8;
  r51 = cn_var_实例;
  r52 = r51->位置;
  r53 = cn_var_起始位置_1;
  r54 = r52 - r53;
  cn_var_长度_8 = r54;
  r55 = cn_var_词元类型枚举;
  r56 = r55.整数字面量;
  r57 = cn_var_起始行号_2;
  r58 = cn_var_起始列号_3;
  r59 = cn_var_长度_8;
  r60 = cn_var_结果_0;
  return r60;
  goto if_merge_621;

  if_then_625:
  goto while_exit_624;
  goto if_merge_626;

  if_merge_626:
  r50 = cn_var_实例;
  前进(r50);
  goto while_cond_622;

  while_cond_627:
  if (1) goto while_body_628; else goto while_exit_629;

  while_body_628:
  r61 = cn_var_实例;
  当前字符(r61);
  cn_var_c_5 = /* NONE */;
  r62 = cn_var_c_5;
  是数字(r62);
  r63 = !/* NONE */;
  if (r63) goto if_then_630; else goto if_merge_631;

  while_exit_629:
  r65 = cn_var_实例;
  当前字符(r65);
  cn_var_c_5 = /* NONE */;
  r67 = cn_var_c_5;
  r68 = r67 == 46;
  if (r68) goto logic_rhs_634; else goto logic_merge_635;

  if_then_630:
  goto while_exit_629;
  goto if_merge_631;

  if_merge_631:
  r64 = cn_var_实例;
  前进(r64);
  goto while_cond_627;

  if_then_632:
  cn_var_是浮点数_4 = 1;
  r70 = cn_var_实例;
  前进(r70);
  goto while_cond_636;

  if_merge_633:
  r75 = cn_var_实例;
  当前字符(r75);
  cn_var_c_5 = /* NONE */;
  r77 = cn_var_c_5;
  r78 = r77 == 101;
  if (r78) goto logic_merge_644; else goto logic_rhs_643;

  logic_rhs_634:
  r69 = cn_var_实例;
  预览字符(r69, 1);
  是数字(/* NONE */);
  goto logic_merge_635;

  logic_merge_635:
  goto if_then_632;

  while_cond_636:
  if (1) goto while_body_637; else goto while_exit_638;

  while_body_637:
  r71 = cn_var_实例;
  当前字符(r71);
  cn_var_c_5 = /* NONE */;
  r72 = cn_var_c_5;
  是数字(r72);
  r73 = !/* NONE */;
  if (r73) goto if_then_639; else goto if_merge_640;

  while_exit_638:
  goto if_merge_633;

  if_then_639:
  goto while_exit_638;
  goto if_merge_640;

  if_merge_640:
  r74 = cn_var_实例;
  前进(r74);
  goto while_cond_636;

  if_then_641:
  cn_var_是浮点数_4 = 1;
  r81 = cn_var_实例;
  前进(r81);
  r82 = cn_var_实例;
  当前字符(r82);
  cn_var_c_5 = /* NONE */;
  r84 = cn_var_c_5;
  r85 = r84 == 43;
  if (r85) goto logic_merge_648; else goto logic_rhs_647;

  if_merge_642:
  long long cn_var_长度_9;
  r93 = cn_var_实例;
  r94 = r93->位置;
  r95 = cn_var_起始位置_1;
  r96 = r94 - r95;
  cn_var_长度_9 = r96;
  r97 = cn_var_是浮点数_4;
  r98 = cn_var_词元类型枚举;
  r99 = r98.浮点字面量;
  r100 = cn_var_词元类型枚举;
  r101 = r100.整数字面量;
  r102 = (r97 ? r99 : r101);
  r103 = cn_var_起始行号_2;
  r104 = cn_var_起始列号_3;
  r105 = cn_var_长度_9;
  r106 = cn_var_结果_0;
  return r106;

  logic_rhs_643:
  r79 = cn_var_c_5;
  r80 = r79 == 69;
  goto logic_merge_644;

  logic_merge_644:
  if (r80) goto if_then_641; else goto if_merge_642;

  if_then_645:
  r88 = cn_var_实例;
  前进(r88);
  goto if_merge_646;

  if_merge_646:
  goto while_cond_649;

  logic_rhs_647:
  r86 = cn_var_c_5;
  r87 = r86 == 45;
  goto logic_merge_648;

  logic_merge_648:
  if (r87) goto if_then_645; else goto if_merge_646;

  while_cond_649:
  if (1) goto while_body_650; else goto while_exit_651;

  while_body_650:
  r89 = cn_var_实例;
  当前字符(r89);
  cn_var_c_5 = /* NONE */;
  r90 = cn_var_c_5;
  是数字(r90);
  r91 = !/* NONE */;
  if (r91) goto if_then_652; else goto if_merge_653;

  while_exit_651:
  goto if_merge_642;

  if_then_652:
  goto while_exit_651;
  goto if_merge_653;

  if_merge_653:
  r92 = cn_var_实例;
  前进(r92);
  goto while_cond_649;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符串(struct 扫描器* cn_var_实例) {
  long long r1, r3, r6, r7, r9, r10, r11, r13, r14, r16, r17, r20, r21, r25, r26, r27, r29, r30, r31;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r5;
  struct 扫描器* r15;
  struct 扫描器* r18;
  struct 扫描器* r19;
  struct 扫描器* r22;
  struct 扫描器* r23;
  struct 扫描器* r28;
  struct 词元 r12;
  struct 词元 r32;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r24;

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
  前进(r4);
  goto while_cond_654;

  while_cond_654:
  if (1) goto while_body_655; else goto while_exit_656;

  while_body_655:
  long long cn_var_c_3;
  r5 = cn_var_实例;
  当前字符(r5);
  cn_var_c_3 = /* NONE */;
  r6 = cn_var_c_3;
  r7 = r6 == 0;
  if (r7) goto if_then_657; else goto if_merge_658;

  while_exit_656:
  r24 = cn_var_词元类型枚举;
  r25 = r24.字符串字面量;
  r26 = cn_var_起始行号_1;
  r27 = cn_var_起始列号_2;
  r28 = cn_var_实例;
  r29 = r28->位置;
  r30 = cn_var_起始列号_2;
  r31 = r29 - r30;
  r32 = cn_var_结果_0;
  return r32;

  if_then_657:
  r8 = cn_var_词元类型枚举;
  r9 = r8.错误;
  r10 = cn_var_起始行号_1;
  r11 = cn_var_起始列号_2;
  r12 = cn_var_结果_0;
  return r12;
  goto if_merge_658;

  if_merge_658:
  r13 = cn_var_c_3;
  r14 = r13 == 34;
  if (r14) goto if_then_659; else goto if_merge_660;

  if_then_659:
  r15 = cn_var_实例;
  前进(r15);
  goto while_exit_656;
  goto if_merge_660;

  if_merge_660:
  r16 = cn_var_c_3;
  r17 = r16 == 92;
  if (r17) goto if_then_661; else goto if_merge_662;

  if_then_661:
  r18 = cn_var_实例;
  前进(r18);
  r19 = cn_var_实例;
  当前字符(r19);
  cn_var_c_3 = /* NONE */;
  r20 = cn_var_c_3;
  r21 = r20 != 0;
  if (r21) goto if_then_663; else goto if_merge_664;

  if_merge_662:
  r23 = cn_var_实例;
  前进(r23);
  goto while_cond_654;

  if_then_663:
  r22 = cn_var_实例;
  前进(r22);
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
  long long r1, r3, r6, r7, r8, r9, r10, r12, r13, r14, r16, r17, r20, r21, r25, r26, r28, r29, r30, r34, r35, r36, r38, r39, r40;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r5;
  struct 扫描器* r18;
  struct 扫描器* r19;
  struct 扫描器* r22;
  struct 扫描器* r23;
  struct 扫描器* r24;
  struct 扫描器* r32;
  struct 扫描器* r37;
  struct 词元 r15;
  struct 词元 r31;
  struct 词元 r41;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r33;

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
  前进(r4);
  long long cn_var_c_3;
  r5 = cn_var_实例;
  当前字符(r5);
  cn_var_c_3 = /* NONE */;
  r7 = cn_var_c_3;
  r8 = r7 == 0;
  if (r8) goto logic_merge_668; else goto logic_rhs_667;

  if_then_665:
  r11 = cn_var_词元类型枚举;
  r12 = r11.错误;
  r13 = cn_var_起始行号_1;
  r14 = cn_var_起始列号_2;
  r15 = cn_var_结果_0;
  return r15;
  goto if_merge_666;

  if_merge_666:
  r16 = cn_var_c_3;
  r17 = r16 == 92;
  if (r17) goto if_then_669; else goto if_else_670;

  logic_rhs_667:
  r9 = cn_var_c_3;
  r10 = r9 == 39;
  goto logic_merge_668;

  logic_merge_668:
  if (r10) goto if_then_665; else goto if_merge_666;

  if_then_669:
  r18 = cn_var_实例;
  前进(r18);
  r19 = cn_var_实例;
  当前字符(r19);
  cn_var_c_3 = /* NONE */;
  r20 = cn_var_c_3;
  r21 = r20 != 0;
  if (r21) goto if_then_672; else goto if_merge_673;

  if_else_670:
  r23 = cn_var_实例;
  前进(r23);
  goto if_merge_671;

  if_merge_671:
  r24 = cn_var_实例;
  当前字符(r24);
  cn_var_c_3 = /* NONE */;
  r25 = cn_var_c_3;
  r26 = r25 != 39;
  if (r26) goto if_then_674; else goto if_merge_675;

  if_then_672:
  r22 = cn_var_实例;
  前进(r22);
  goto if_merge_673;

  if_merge_673:
  goto if_merge_671;

  if_then_674:
  r27 = cn_var_词元类型枚举;
  r28 = r27.错误;
  r29 = cn_var_起始行号_1;
  r30 = cn_var_起始列号_2;
  r31 = cn_var_结果_0;
  return r31;
  goto if_merge_675;

  if_merge_675:
  r32 = cn_var_实例;
  前进(r32);
  r33 = cn_var_词元类型枚举;
  r34 = r33.字符字面量;
  r35 = cn_var_起始行号_1;
  r36 = cn_var_起始列号_2;
  r37 = cn_var_实例;
  r38 = r37->位置;
  r39 = cn_var_起始列号_2;
  r40 = r38 - r39;
  r41 = cn_var_结果_0;
  return r41;
}

struct 词元 扫描运算符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r6, r7, r34, r38, r41, r48, r52, r56, r59, r63, r66, r70, r74, r91, r93, r94, r96, r97, r98;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r32;
  struct 扫描器* r33;
  struct 扫描器* r35;
  struct 扫描器* r36;
  struct 扫描器* r37;
  struct 扫描器* r39;
  struct 扫描器* r40;
  struct 扫描器* r42;
  struct 扫描器* r43;
  struct 扫描器* r44;
  struct 扫描器* r45;
  struct 扫描器* r46;
  struct 扫描器* r47;
  struct 扫描器* r49;
  struct 扫描器* r50;
  struct 扫描器* r51;
  struct 扫描器* r53;
  struct 扫描器* r54;
  struct 扫描器* r55;
  struct 扫描器* r57;
  struct 扫描器* r58;
  struct 扫描器* r60;
  struct 扫描器* r61;
  struct 扫描器* r62;
  struct 扫描器* r64;
  struct 扫描器* r65;
  struct 扫描器* r67;
  struct 扫描器* r68;
  struct 扫描器* r69;
  struct 扫描器* r71;
  struct 扫描器* r72;
  struct 扫描器* r73;
  struct 扫描器* r75;
  struct 扫描器* r76;
  struct 扫描器* r77;
  struct 扫描器* r78;
  struct 扫描器* r79;
  struct 扫描器* r80;
  struct 扫描器* r81;
  struct 扫描器* r82;
  struct 扫描器* r83;
  struct 扫描器* r84;
  struct 扫描器* r85;
  struct 扫描器* r86;
  struct 扫描器* r87;
  struct 扫描器* r88;
  struct 扫描器* r89;
  struct 扫描器* r95;
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
  enum 词元类型枚举 r92;
  struct 词元 r99;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r90;

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
  当前字符(r4);
  cn_var_c_3 = /* NONE */;
  enum 词元类型枚举 cn_var_类型_4;
  r5 = cn_var_词元类型枚举;
  r6 = r5.错误;
  cn_var_类型_4 = r6;
  r7 = cn_var_c_3;
  r8 = r7 == 43;
  if (r8) goto case_body_677; else goto switch_check_702;

  switch_check_702:
  r9 = r7 == 45;
  if (r9) goto case_body_678; else goto switch_check_703;

  switch_check_703:
  r10 = r7 == 42;
  if (r10) goto case_body_679; else goto switch_check_704;

  switch_check_704:
  r11 = r7 == 47;
  if (r11) goto case_body_680; else goto switch_check_705;

  switch_check_705:
  r12 = r7 == 37;
  if (r12) goto case_body_681; else goto switch_check_706;

  switch_check_706:
  r13 = r7 == 61;
  if (r13) goto case_body_682; else goto switch_check_707;

  switch_check_707:
  r14 = r7 == 33;
  if (r14) goto case_body_683; else goto switch_check_708;

  switch_check_708:
  r15 = r7 == 60;
  if (r15) goto case_body_684; else goto switch_check_709;

  switch_check_709:
  r16 = r7 == 62;
  if (r16) goto case_body_685; else goto switch_check_710;

  switch_check_710:
  r17 = r7 == 38;
  if (r17) goto case_body_686; else goto switch_check_711;

  switch_check_711:
  r18 = r7 == 124;
  if (r18) goto case_body_687; else goto switch_check_712;

  switch_check_712:
  r19 = r7 == 94;
  if (r19) goto case_body_688; else goto switch_check_713;

  switch_check_713:
  r20 = r7 == 126;
  if (r20) goto case_body_689; else goto switch_check_714;

  switch_check_714:
  r21 = r7 == 40;
  if (r21) goto case_body_690; else goto switch_check_715;

  switch_check_715:
  r22 = r7 == 41;
  if (r22) goto case_body_691; else goto switch_check_716;

  switch_check_716:
  r23 = r7 == 123;
  if (r23) goto case_body_692; else goto switch_check_717;

  switch_check_717:
  r24 = r7 == 125;
  if (r24) goto case_body_693; else goto switch_check_718;

  switch_check_718:
  r25 = r7 == 91;
  if (r25) goto case_body_694; else goto switch_check_719;

  switch_check_719:
  r26 = r7 == 93;
  if (r26) goto case_body_695; else goto switch_check_720;

  switch_check_720:
  r27 = r7 == 59;
  if (r27) goto case_body_696; else goto switch_check_721;

  switch_check_721:
  r28 = r7 == 44;
  if (r28) goto case_body_697; else goto switch_check_722;

  switch_check_722:
  r29 = r7 == 46;
  if (r29) goto case_body_698; else goto switch_check_723;

  switch_check_723:
  r30 = r7 == 58;
  if (r30) goto case_body_699; else goto switch_check_724;

  switch_check_724:
  r31 = r7 == 63;
  if (r31) goto case_body_700; else goto case_default_701;

  case_body_677:
  r32 = cn_var_实例;
  前进(r32);
  r33 = cn_var_实例;
  当前字符(r33);
  r34 = /* NONE */ == 43;
  if (r34) goto if_then_725; else goto if_else_726;

  if_then_725:
  r35 = cn_var_实例;
  前进(r35);
  cn_var_类型_4 = 66;
  goto if_merge_727;

  if_else_726:
  cn_var_类型_4 = 45;
  goto if_merge_727;

  if_merge_727:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_678:
  r36 = cn_var_实例;
  前进(r36);
  r37 = cn_var_实例;
  当前字符(r37);
  r38 = /* NONE */ == 62;
  if (r38) goto if_then_728; else goto if_else_729;

  if_then_728:
  r39 = cn_var_实例;
  前进(r39);
  cn_var_类型_4 = 68;
  goto if_merge_730;

  if_else_729:
  r40 = cn_var_实例;
  当前字符(r40);
  r41 = /* NONE */ == 45;
  if (r41) goto if_then_731; else goto if_else_732;

  if_merge_730:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_731:
  r42 = cn_var_实例;
  前进(r42);
  cn_var_类型_4 = 67;
  goto if_merge_733;

  if_else_732:
  cn_var_类型_4 = 46;
  goto if_merge_733;

  if_merge_733:
  goto if_merge_730;

  case_body_679:
  r43 = cn_var_实例;
  前进(r43);
  cn_var_类型_4 = 47;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_680:
  r44 = cn_var_实例;
  前进(r44);
  cn_var_类型_4 = 48;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_681:
  r45 = cn_var_实例;
  前进(r45);
  cn_var_类型_4 = 49;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_682:
  r46 = cn_var_实例;
  前进(r46);
  r47 = cn_var_实例;
  当前字符(r47);
  r48 = /* NONE */ == 61;
  if (r48) goto if_then_734; else goto if_else_735;

  if_then_734:
  r49 = cn_var_实例;
  前进(r49);
  cn_var_类型_4 = 50;
  goto if_merge_736;

  if_else_735:
  cn_var_类型_4 = 51;
  goto if_merge_736;

  if_merge_736:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_683:
  r50 = cn_var_实例;
  前进(r50);
  r51 = cn_var_实例;
  当前字符(r51);
  r52 = /* NONE */ == 61;
  if (r52) goto if_then_737; else goto if_else_738;

  if_then_737:
  r53 = cn_var_实例;
  前进(r53);
  cn_var_类型_4 = 52;
  goto if_merge_739;

  if_else_738:
  cn_var_类型_4 = 59;
  goto if_merge_739;

  if_merge_739:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_684:
  r54 = cn_var_实例;
  前进(r54);
  r55 = cn_var_实例;
  当前字符(r55);
  r56 = /* NONE */ == 60;
  if (r56) goto if_then_740; else goto if_else_741;

  if_then_740:
  r57 = cn_var_实例;
  前进(r57);
  cn_var_类型_4 = 64;
  goto if_merge_742;

  if_else_741:
  r58 = cn_var_实例;
  当前字符(r58);
  r59 = /* NONE */ == 61;
  if (r59) goto if_then_743; else goto if_else_744;

  if_merge_742:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_743:
  r60 = cn_var_实例;
  前进(r60);
  cn_var_类型_4 = 54;
  goto if_merge_745;

  if_else_744:
  cn_var_类型_4 = 53;
  goto if_merge_745;

  if_merge_745:
  goto if_merge_742;

  case_body_685:
  r61 = cn_var_实例;
  前进(r61);
  r62 = cn_var_实例;
  当前字符(r62);
  r63 = /* NONE */ == 62;
  if (r63) goto if_then_746; else goto if_else_747;

  if_then_746:
  r64 = cn_var_实例;
  前进(r64);
  cn_var_类型_4 = 65;
  goto if_merge_748;

  if_else_747:
  r65 = cn_var_实例;
  当前字符(r65);
  r66 = /* NONE */ == 61;
  if (r66) goto if_then_749; else goto if_else_750;

  if_merge_748:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_749:
  r67 = cn_var_实例;
  前进(r67);
  cn_var_类型_4 = 56;
  goto if_merge_751;

  if_else_750:
  cn_var_类型_4 = 55;
  goto if_merge_751;

  if_merge_751:
  goto if_merge_748;

  case_body_686:
  r68 = cn_var_实例;
  前进(r68);
  r69 = cn_var_实例;
  当前字符(r69);
  r70 = /* NONE */ == 38;
  if (r70) goto if_then_752; else goto if_else_753;

  if_then_752:
  r71 = cn_var_实例;
  前进(r71);
  cn_var_类型_4 = 57;
  goto if_merge_754;

  if_else_753:
  cn_var_类型_4 = 60;
  goto if_merge_754;

  if_merge_754:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_687:
  r72 = cn_var_实例;
  前进(r72);
  r73 = cn_var_实例;
  当前字符(r73);
  r74 = /* NONE */ == 124;
  if (r74) goto if_then_755; else goto if_else_756;

  if_then_755:
  r75 = cn_var_实例;
  前进(r75);
  cn_var_类型_4 = 58;
  goto if_merge_757;

  if_else_756:
  cn_var_类型_4 = 61;
  goto if_merge_757;

  if_merge_757:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_688:
  r76 = cn_var_实例;
  前进(r76);
  cn_var_类型_4 = 62;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_689:
  r77 = cn_var_实例;
  前进(r77);
  cn_var_类型_4 = 63;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_690:
  r78 = cn_var_实例;
  前进(r78);
  cn_var_类型_4 = 69;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_691:
  r79 = cn_var_实例;
  前进(r79);
  cn_var_类型_4 = 70;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_692:
  r80 = cn_var_实例;
  前进(r80);
  cn_var_类型_4 = 71;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_693:
  r81 = cn_var_实例;
  前进(r81);
  cn_var_类型_4 = 72;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_694:
  r82 = cn_var_实例;
  前进(r82);
  cn_var_类型_4 = 73;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_695:
  r83 = cn_var_实例;
  前进(r83);
  cn_var_类型_4 = 74;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_696:
  r84 = cn_var_实例;
  前进(r84);
  cn_var_类型_4 = 75;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_697:
  r85 = cn_var_实例;
  前进(r85);
  cn_var_类型_4 = 76;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_698:
  r86 = cn_var_实例;
  前进(r86);
  cn_var_类型_4 = 77;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_699:
  r87 = cn_var_实例;
  前进(r87);
  cn_var_类型_4 = 78;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_700:
  r88 = cn_var_实例;
  前进(r88);
  cn_var_类型_4 = 79;
  goto switch_merge_676;
  goto switch_merge_676;

  case_default_701:
  r89 = cn_var_实例;
  前进(r89);
  r90 = cn_var_词元类型枚举;
  r91 = r90.错误;
  cn_var_类型_4 = r91;
  goto switch_merge_676;
  goto switch_merge_676;

  switch_merge_676:
  r92 = cn_var_类型_4;
  r93 = cn_var_起始行号_1;
  r94 = cn_var_起始列号_2;
  r95 = cn_var_实例;
  r96 = r95->位置;
  r97 = cn_var_起始列号_2;
  r98 = r96 - r97;
  r99 = cn_var_结果_0;
  return r99;
}

