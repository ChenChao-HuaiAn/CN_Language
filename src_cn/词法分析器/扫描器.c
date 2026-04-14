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
struct 关键字条目;
struct 关键字条目 {
    char* 名称;
    long long 类型;
};

// Forward Declarations - 从导入模块
extern _Bool cn_var_关键字表已初始化;
extern struct 关键字条目* cn_var_关键字表;
extern long long cn_var_关键字表大小;
void 初始化关键字表(void);
struct 关键字条目* 获取关键字表(void);
long long 关键字总数(void);
_Bool 是关键字字符串(const char*);
enum 词元类型枚举 查找关键字(const char*);
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 复制字符串副本(const char*);
char* 字符串格式(const char*);
long long 字符串格式化(const char*, long long, const char*);
char* 读取行(void);
long long 获取绝对值(long long);
long long 求最大值(long long, long long);
long long 求最小值(long long, long long);
long long 获取参数个数(void);
char* 获取参数(long long);
char* 获取程序名称(void);
char* 查找选项(const char*);
long long 选项存在(const char*);
char* 获取位置参数(long long);
long long 获取位置参数个数(void);
void* 数组获取(void*, long long);
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, const char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);
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
struct 扫描器* 创建扫描器(const char*, long long);
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

struct 扫描器* 创建扫描器(const char* cn_var_源码内容, long long cn_var_长度) {
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
  long long r1, r4, r5, r6, r9, r10, r13, r14, r18, r22, r23, r26, r27;
  struct 扫描器* r0;
  struct 扫描器* r3;
  struct 扫描器* r7;
  struct 扫描器* r12;
  struct 扫描器* r16;
  struct 扫描器* r20;
  struct 扫描器* r24;
  struct 扫描器* r28;
  struct 扫描器* r30;
  _Bool r8;
  _Bool r15;
  _Bool r19;
  struct 词元 r2;
  struct 词元 r11;
  struct 词元 r17;
  struct 词元 r21;
  struct 词元 r25;
  struct 词元 r29;
  struct 词元 r31;

  entry:
  struct 词元 cn_var_结果_0;
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_543; else goto if_merge_544;

  if_then_543:
  r2 = cn_var_结果_0;
  return r2;
  goto if_merge_544;

  if_merge_544:
  r3 = cn_var_实例;
  跳过空白字符(r3);
  long long cn_var_起始位置_1;
  r4 = cn_var_实例.位置;
  cn_var_起始位置_1 = r4;
  long long cn_var_起始行号_2;
  r5 = cn_var_实例.行号;
  cn_var_起始行号_2 = r5;
  long long cn_var_起始列号_3;
  r6 = cn_var_实例.列号;
  cn_var_起始列号_3 = r6;
  r7 = cn_var_实例;
  r8 = 是否结束(r7);
  if (r8) goto if_then_545; else goto if_merge_546;

  if_then_545:
  r9 = cn_var_起始行号_2;
  r10 = cn_var_起始列号_3;
  r11 = cn_var_结果_0;
  return r11;
  goto if_merge_546;

  if_merge_546:
  long long cn_var_c_4;
  r12 = cn_var_实例;
  r13 = 当前字符(r12);
  cn_var_c_4 = r13;
  r14 = cn_var_c_4;
  r15 = 是标识符开头(r14);
  if (r15) goto if_then_547; else goto if_merge_548;

  if_then_547:
  r16 = cn_var_实例;
  r17 = 扫描标识符(r16);
  return r17;
  goto if_merge_548;

  if_merge_548:
  r18 = cn_var_c_4;
  r19 = 是数字(r18);
  if (r19) goto if_then_549; else goto if_merge_550;

  if_then_549:
  r20 = cn_var_实例;
  r21 = 扫描数字(r20);
  return r21;
  goto if_merge_550;

  if_merge_550:
  r22 = cn_var_c_4;
  r23 = r22 == 34;
  if (r23) goto if_then_551; else goto if_merge_552;

  if_then_551:
  r24 = cn_var_实例;
  r25 = 扫描字符串(r24);
  return r25;
  goto if_merge_552;

  if_merge_552:
  r26 = cn_var_c_4;
  r27 = r26 == 39;
  if (r27) goto if_then_553; else goto if_merge_554;

  if_then_553:
  r28 = cn_var_实例;
  r29 = 扫描字符(r28);
  return r29;
  goto if_merge_554;

  if_merge_554:
  r30 = cn_var_实例;
  r31 = 扫描运算符(r30);
  return r31;
}

struct 词元 预览词元(struct 扫描器* cn_var_实例) {
  long long r1, r3, r4, r5, r8, r9, r10;
  struct 扫描器* r0;
  struct 扫描器* r6;
  struct 词元 r2;
  struct 词元 r7;
  struct 词元 r11;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_555; else goto if_merge_556;

  if_then_555:
  struct 词元 cn_var_空词元_0;
  r2 = cn_var_空词元_0;
  return r2;
  goto if_merge_556;

  if_merge_556:
  long long cn_var_保存位置_1;
  r3 = cn_var_实例.位置;
  cn_var_保存位置_1 = r3;
  long long cn_var_保存行号_2;
  r4 = cn_var_实例.行号;
  cn_var_保存行号_2 = r4;
  long long cn_var_保存列号_3;
  r5 = cn_var_实例.列号;
  cn_var_保存列号_3 = r5;
  struct 词元 cn_var_结果_4;
  r6 = cn_var_实例;
  r7 = 下一个词元(r6);
  cn_var_结果_4 = r7;
  r8 = cn_var_保存位置_1;
  r9 = cn_var_保存行号_2;
  r10 = cn_var_保存列号_3;
  r11 = cn_var_结果_4;
  return r11;
}

long long 当前字符(struct 扫描器* cn_var_实例) {
  long long r0, r2, r3, r4, r5, r6, r7;
  struct 扫描器* r1;
  void* r8;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_560; else goto logic_rhs_559;

  if_then_557:
  return 0;
  goto if_merge_558;

  if_merge_558:
  r6 = cn_var_实例.源码;
  r7 = cn_var_实例.位置;
  r8 = (void*)cn_rt_array_get_element(r6, r7, 8);
  return r8;

  logic_rhs_559:
  r3 = cn_var_实例.位置;
  r4 = cn_var_实例.源码长度;
  r5 = r3 >= r4;
  goto logic_merge_560;

  logic_merge_560:
  if (r5) goto if_then_557; else goto if_merge_558;
  return 0;
}

long long 预览字符(struct 扫描器* cn_var_实例, long long cn_var_偏移) {
  long long r0, r1, r2, r3, r4, r6, r7, r8, r9, r10, r11, r12, r13;
  struct 扫描器* r5;
  void* r14;

  entry:
  long long cn_var_目标位置_0;
  r0 = cn_var_实例.位置;
  r1 = cn_var_偏移;
  r2 = r0 + r1;
  cn_var_目标位置_0 = r2;
  r5 = cn_var_实例;
  r6 = r5 == 0;
  if (r6) goto logic_merge_566; else goto logic_rhs_565;

  if_then_561:
  return 0;
  goto if_merge_562;

  if_merge_562:
  r12 = cn_var_实例.源码;
  r13 = cn_var_目标位置_0;
  r14 = (void*)cn_rt_array_get_element(r12, r13, 8);
  return r14;

  logic_rhs_563:
  r9 = cn_var_目标位置_0;
  r10 = cn_var_实例.源码长度;
  r11 = r9 >= r10;
  goto logic_merge_564;

  logic_merge_564:
  if (r11) goto if_then_561; else goto if_merge_562;

  logic_rhs_565:
  r7 = cn_var_目标位置_0;
  r8 = r7 < 0;
  goto logic_merge_566;

  logic_merge_566:
  if (r8) goto logic_merge_564; else goto logic_rhs_563;
  return 0;
}

long long 前进(struct 扫描器* cn_var_实例) {
  long long r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13;
  struct 扫描器* r0;

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
  r5 = cn_var_实例.位置;
  r6 = r5 + 1;
  r7 = cn_var_c_0;
  r8 = r7 == 10;
  if (r8) goto if_then_569; else goto if_else_570;

  if_then_569:
  r9 = cn_var_实例.行号;
  r10 = r9 + 1;
  goto if_merge_571;

  if_else_570:
  r11 = cn_var_实例.列号;
  r12 = r11 + 1;
  goto if_merge_571;

  if_merge_571:
  r13 = cn_var_c_0;
  return r13;
}

_Bool 是否结束(struct 扫描器* cn_var_实例) {
  long long r1, r2, r3, r4;
  struct 扫描器* r0;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_572; else goto if_merge_573;

  if_then_572:
  return 1;
  goto if_merge_573;

  if_merge_573:
  r2 = cn_var_实例.位置;
  r3 = cn_var_实例.源码长度;
  r4 = r2 >= r3;
  return r4;
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
  long long r0, r1, r2, r4, r5, r7, r9, r10, r11, r12, r13, r14, r15, r16, r17, r19, r20, r21, r22, r25, r30, r31, r32;
  char* r23;
  char* r18;
  char* r24;
  char* r26;
  char* r29;
  struct 扫描器* r3;
  struct 扫描器* r8;
  _Bool r6;
  enum 词元类型枚举 r28;
  struct 词元 r33;
  enum 词元类型枚举 r27;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始位置_1;
  r0 = cn_var_实例.位置;
  cn_var_起始位置_1 = r0;
  long long cn_var_起始行号_2;
  r1 = cn_var_实例.行号;
  cn_var_起始行号_2 = r1;
  long long cn_var_起始列号_3;
  r2 = cn_var_实例.列号;
  cn_var_起始列号_3 = r2;
  goto while_cond_599;

  while_cond_599:
  if (1) goto while_body_600; else goto while_exit_601;

  while_body_600:
  long long cn_var_c_4;
  r3 = cn_var_实例;
  r4 = 当前字符(r3);
  cn_var_c_4 = r4;
  r5 = cn_var_c_4;
  r6 = 是标识符字符(r5);
  r7 = !r6;
  if (r7) goto if_then_602; else goto if_merge_603;

  while_exit_601:
  long long cn_var_长度_5;
  r10 = cn_var_实例.位置;
  r11 = cn_var_起始位置_1;
  r12 = r10 - r11;
  cn_var_长度_5 = r12;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 256);
  r13 = cn_var_长度_5;
  r14 = cn_var_最大标识符长度;
  r15 = r13 >= r14;
  if (r15) goto if_then_604; else goto if_merge_605;

  if_then_602:
  goto while_exit_601;
  goto if_merge_603;

  if_merge_603:
  r8 = cn_var_实例;
  r9 = 前进(r8);
  goto while_cond_599;

  if_then_604:
  r16 = cn_var_最大标识符长度;
  r17 = r16 - 1;
  cn_var_长度_5 = r17;
  goto if_merge_605;

  if_merge_605:
  r18 = cn_var_缓冲区;
  r19 = cn_var_实例.源码;
  r20 = cn_var_起始位置_1;
  r21 = r19 + r20;
  r22 = cn_var_长度_5;
  r23 = 限长复制字符串(r18, r21, r22);
  r24 = cn_var_缓冲区;
  r25 = cn_var_长度_5;
    { long long _tmp_i1 = 0; cn_rt_array_set_element(r24, r25, &_tmp_i1, 8); }
  enum 词元类型枚举 cn_var_类型_6;
  r26 = cn_var_缓冲区;
  r27 = 查找关键字(r26);
  cn_var_类型_6 = r27;
  r28 = cn_var_类型_6;
  r29 = cn_var_缓冲区;
  r30 = cn_var_起始行号_2;
  r31 = cn_var_起始列号_3;
  r32 = cn_var_长度_5;
  r33 = cn_var_结果_0;
  return r33;
}

struct 词元 扫描数字(struct 扫描器* cn_var_实例) {
  long long r0, r1, r2, r4, r5, r7, r10, r12, r14, r15, r17, r19, r20, r21, r22, r23, r24, r25, r27, r29, r32, r34, r36, r37, r39, r41, r42, r43, r44, r45, r46, r47, r49, r51, r54, r56, r58, r59, r61, r63, r64, r65, r66, r67, r68, r69, r72, r73, r75, r77, r79, r80, r81, r82, r84, r87, r89, r90, r92, r94, r96, r97, r98, r99, r100, r101, r103, r105, r106, r107, r108, r109, r110, r112, r114, r115, r117, r119, r120, r121, r122, r124, r125, r126, r127;
  struct 扫描器* r3;
  struct 扫描器* r6;
  struct 扫描器* r9;
  struct 扫描器* r11;
  struct 扫描器* r13;
  struct 扫描器* r18;
  struct 扫描器* r28;
  struct 扫描器* r31;
  struct 扫描器* r33;
  struct 扫描器* r35;
  struct 扫描器* r40;
  struct 扫描器* r50;
  struct 扫描器* r53;
  struct 扫描器* r55;
  struct 扫描器* r57;
  struct 扫描器* r62;
  struct 扫描器* r71;
  struct 扫描器* r76;
  struct 扫描器* r78;
  struct 扫描器* r83;
  struct 扫描器* r86;
  struct 扫描器* r88;
  struct 扫描器* r93;
  struct 扫描器* r95;
  struct 扫描器* r102;
  struct 扫描器* r104;
  struct 扫描器* r111;
  struct 扫描器* r113;
  struct 扫描器* r118;
  _Bool r8;
  _Bool r16;
  _Bool r30;
  _Bool r38;
  _Bool r52;
  _Bool r60;
  _Bool r74;
  _Bool r85;
  _Bool r91;
  _Bool r116;
  _Bool r123;
  struct 词元 r26;
  struct 词元 r48;
  struct 词元 r70;
  struct 词元 r128;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始位置_1;
  r0 = cn_var_实例.位置;
  cn_var_起始位置_1 = r0;
  long long cn_var_起始行号_2;
  r1 = cn_var_实例.行号;
  cn_var_起始行号_2 = r1;
  long long cn_var_起始列号_3;
  r2 = cn_var_实例.列号;
  cn_var_起始列号_3 = r2;
  _Bool cn_var_是浮点数_4;
  cn_var_是浮点数_4 = 0;
  long long cn_var_c_5;
  r3 = cn_var_实例;
  r4 = 当前字符(r3);
  cn_var_c_5 = r4;
  r5 = cn_var_c_5;
  r6 = cn_var_实例;
  r7 = 预览字符(r6, 1);
  r8 = 是十六进制前缀(r5, r7);
  if (r8) goto if_then_606; else goto if_merge_607;

  if_then_606:
  r9 = cn_var_实例;
  r10 = 前进(r9);
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_608;

  if_merge_607:
  r27 = cn_var_c_5;
  r28 = cn_var_实例;
  r29 = 预览字符(r28, 1);
  r30 = 是二进制前缀(r27, r29);
  if (r30) goto if_then_613; else goto if_merge_614;

  while_cond_608:
  if (1) goto while_body_609; else goto while_exit_610;

  while_body_609:
  r13 = cn_var_实例;
  r14 = 当前字符(r13);
  cn_var_c_5 = r14;
  r15 = cn_var_c_5;
  r16 = 是十六进制数字(r15);
  r17 = !r16;
  if (r17) goto if_then_611; else goto if_merge_612;

  while_exit_610:
  long long cn_var_长度_6;
  r20 = cn_var_实例.位置;
  r21 = cn_var_起始位置_1;
  r22 = r20 - r21;
  cn_var_长度_6 = r22;
  r23 = cn_var_起始行号_2;
  r24 = cn_var_起始列号_3;
  r25 = cn_var_长度_6;
  r26 = cn_var_结果_0;
  return r26;
  goto if_merge_607;

  if_then_611:
  goto while_exit_610;
  goto if_merge_612;

  if_merge_612:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_608;

  if_then_613:
  r31 = cn_var_实例;
  r32 = 前进(r31);
  r33 = cn_var_实例;
  r34 = 前进(r33);
  goto while_cond_615;

  if_merge_614:
  r49 = cn_var_c_5;
  r50 = cn_var_实例;
  r51 = 预览字符(r50, 1);
  r52 = 是八进制前缀(r49, r51);
  if (r52) goto if_then_620; else goto if_merge_621;

  while_cond_615:
  if (1) goto while_body_616; else goto while_exit_617;

  while_body_616:
  r35 = cn_var_实例;
  r36 = 当前字符(r35);
  cn_var_c_5 = r36;
  r37 = cn_var_c_5;
  r38 = 是二进制数字(r37);
  r39 = !r38;
  if (r39) goto if_then_618; else goto if_merge_619;

  while_exit_617:
  long long cn_var_长度_7;
  r42 = cn_var_实例.位置;
  r43 = cn_var_起始位置_1;
  r44 = r42 - r43;
  cn_var_长度_7 = r44;
  r45 = cn_var_起始行号_2;
  r46 = cn_var_起始列号_3;
  r47 = cn_var_长度_7;
  r48 = cn_var_结果_0;
  return r48;
  goto if_merge_614;

  if_then_618:
  goto while_exit_617;
  goto if_merge_619;

  if_merge_619:
  r40 = cn_var_实例;
  r41 = 前进(r40);
  goto while_cond_615;

  if_then_620:
  r53 = cn_var_实例;
  r54 = 前进(r53);
  r55 = cn_var_实例;
  r56 = 前进(r55);
  goto while_cond_622;

  if_merge_621:
  goto while_cond_627;

  while_cond_622:
  if (1) goto while_body_623; else goto while_exit_624;

  while_body_623:
  r57 = cn_var_实例;
  r58 = 当前字符(r57);
  cn_var_c_5 = r58;
  r59 = cn_var_c_5;
  r60 = 是八进制数字(r59);
  r61 = !r60;
  if (r61) goto if_then_625; else goto if_merge_626;

  while_exit_624:
  long long cn_var_长度_8;
  r64 = cn_var_实例.位置;
  r65 = cn_var_起始位置_1;
  r66 = r64 - r65;
  cn_var_长度_8 = r66;
  r67 = cn_var_起始行号_2;
  r68 = cn_var_起始列号_3;
  r69 = cn_var_长度_8;
  r70 = cn_var_结果_0;
  return r70;
  goto if_merge_621;

  if_then_625:
  goto while_exit_624;
  goto if_merge_626;

  if_merge_626:
  r62 = cn_var_实例;
  r63 = 前进(r62);
  goto while_cond_622;

  while_cond_627:
  if (1) goto while_body_628; else goto while_exit_629;

  while_body_628:
  r71 = cn_var_实例;
  r72 = 当前字符(r71);
  cn_var_c_5 = r72;
  r73 = cn_var_c_5;
  r74 = 是数字(r73);
  r75 = !r74;
  if (r75) goto if_then_630; else goto if_merge_631;

  while_exit_629:
  r78 = cn_var_实例;
  r79 = 当前字符(r78);
  cn_var_c_5 = r79;
  r81 = cn_var_c_5;
  r82 = r81 == 46;
  if (r82) goto logic_rhs_634; else goto logic_merge_635;

  if_then_630:
  goto while_exit_629;
  goto if_merge_631;

  if_merge_631:
  r76 = cn_var_实例;
  r77 = 前进(r76);
  goto while_cond_627;

  if_then_632:
  cn_var_是浮点数_4 = 1;
  r86 = cn_var_实例;
  r87 = 前进(r86);
  goto while_cond_636;

  if_merge_633:
  r95 = cn_var_实例;
  r96 = 当前字符(r95);
  cn_var_c_5 = r96;
  r98 = cn_var_c_5;
  r99 = r98 == 101;
  if (r99) goto logic_merge_644; else goto logic_rhs_643;

  logic_rhs_634:
  r83 = cn_var_实例;
  r84 = 预览字符(r83, 1);
  r85 = 是数字(r84);
  goto logic_merge_635;

  logic_merge_635:
  if (r85) goto if_then_632; else goto if_merge_633;

  while_cond_636:
  if (1) goto while_body_637; else goto while_exit_638;

  while_body_637:
  r88 = cn_var_实例;
  r89 = 当前字符(r88);
  cn_var_c_5 = r89;
  r90 = cn_var_c_5;
  r91 = 是数字(r90);
  r92 = !r91;
  if (r92) goto if_then_639; else goto if_merge_640;

  while_exit_638:
  goto if_merge_633;

  if_then_639:
  goto while_exit_638;
  goto if_merge_640;

  if_merge_640:
  r93 = cn_var_实例;
  r94 = 前进(r93);
  goto while_cond_636;

  if_then_641:
  cn_var_是浮点数_4 = 1;
  r102 = cn_var_实例;
  r103 = 前进(r102);
  r104 = cn_var_实例;
  r105 = 当前字符(r104);
  cn_var_c_5 = r105;
  r107 = cn_var_c_5;
  r108 = r107 == 43;
  if (r108) goto logic_merge_648; else goto logic_rhs_647;

  if_merge_642:
  long long cn_var_长度_9;
  r120 = cn_var_实例.位置;
  r121 = cn_var_起始位置_1;
  r122 = r120 - r121;
  cn_var_长度_9 = r122;
  r123 = cn_var_是浮点数_4;
  r124 = (r123 ? 词元类型枚举_浮点字面量 : 词元类型枚举_整数字面量);
  r125 = cn_var_起始行号_2;
  r126 = cn_var_起始列号_3;
  r127 = cn_var_长度_9;
  r128 = cn_var_结果_0;
  return r128;

  logic_rhs_643:
  r100 = cn_var_c_5;
  r101 = r100 == 69;
  goto logic_merge_644;

  logic_merge_644:
  if (r101) goto if_then_641; else goto if_merge_642;

  if_then_645:
  r111 = cn_var_实例;
  r112 = 前进(r111);
  goto if_merge_646;

  if_merge_646:
  goto while_cond_649;

  logic_rhs_647:
  r109 = cn_var_c_5;
  r110 = r109 == 45;
  goto logic_merge_648;

  logic_merge_648:
  if (r110) goto if_then_645; else goto if_merge_646;

  while_cond_649:
  if (1) goto while_body_650; else goto while_exit_651;

  while_body_650:
  r113 = cn_var_实例;
  r114 = 当前字符(r113);
  cn_var_c_5 = r114;
  r115 = cn_var_c_5;
  r116 = 是数字(r115);
  r117 = !r116;
  if (r117) goto if_then_652; else goto if_merge_653;

  while_exit_651:
  goto if_merge_642;

  if_then_652:
  goto while_exit_651;
  goto if_merge_653;

  if_merge_653:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  goto while_cond_649;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符串(struct 扫描器* cn_var_实例) {
  long long r0, r1, r3, r5, r6, r7, r8, r9, r11, r12, r14, r15, r16, r18, r20, r21, r22, r24, r26, r27, r28, r29, r30, r31;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r13;
  struct 扫描器* r17;
  struct 扫描器* r19;
  struct 扫描器* r23;
  struct 扫描器* r25;
  struct 词元 r10;
  struct 词元 r32;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例.行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例.列号;
  cn_var_起始列号_2 = r1;
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_654;

  while_cond_654:
  if (1) goto while_body_655; else goto while_exit_656;

  while_body_655:
  long long cn_var_c_3;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_3 = r5;
  r6 = cn_var_c_3;
  r7 = r6 == 0;
  if (r7) goto if_then_657; else goto if_merge_658;

  while_exit_656:
  r27 = cn_var_起始行号_1;
  r28 = cn_var_起始列号_2;
  r29 = cn_var_实例.位置;
  r30 = cn_var_起始列号_2;
  r31 = r29 - r30;
  r32 = cn_var_结果_0;
  return r32;

  if_then_657:
  r8 = cn_var_起始行号_1;
  r9 = cn_var_起始列号_2;
  r10 = cn_var_结果_0;
  return r10;
  goto if_merge_658;

  if_merge_658:
  r11 = cn_var_c_3;
  r12 = r11 == 34;
  if (r12) goto if_then_659; else goto if_merge_660;

  if_then_659:
  r13 = cn_var_实例;
  r14 = 前进(r13);
  goto while_exit_656;
  goto if_merge_660;

  if_merge_660:
  r15 = cn_var_c_3;
  r16 = r15 == 92;
  if (r16) goto if_then_661; else goto if_merge_662;

  if_then_661:
  r17 = cn_var_实例;
  r18 = 前进(r17);
  r19 = cn_var_实例;
  r20 = 当前字符(r19);
  cn_var_c_3 = r20;
  r21 = cn_var_c_3;
  r22 = r21 != 0;
  if (r22) goto if_then_663; else goto if_merge_664;

  if_merge_662:
  r25 = cn_var_实例;
  r26 = 前进(r25);
  goto while_cond_654;

  if_then_663:
  r23 = cn_var_实例;
  r24 = 前进(r23);
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
  long long r0, r1, r3, r5, r6, r7, r8, r9, r10, r11, r12, r14, r15, r17, r19, r20, r21, r23, r25, r27, r28, r29, r30, r31, r34, r35, r36, r37, r38, r39;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r16;
  struct 扫描器* r18;
  struct 扫描器* r22;
  struct 扫描器* r24;
  struct 扫描器* r26;
  struct 扫描器* r33;
  struct 词元 r13;
  struct 词元 r32;
  struct 词元 r40;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例.行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例.列号;
  cn_var_起始列号_2 = r1;
  r2 = cn_var_实例;
  r3 = 前进(r2);
  long long cn_var_c_3;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_3 = r5;
  r7 = cn_var_c_3;
  r8 = r7 == 0;
  if (r8) goto logic_merge_668; else goto logic_rhs_667;

  if_then_665:
  r11 = cn_var_起始行号_1;
  r12 = cn_var_起始列号_2;
  r13 = cn_var_结果_0;
  return r13;
  goto if_merge_666;

  if_merge_666:
  r14 = cn_var_c_3;
  r15 = r14 == 92;
  if (r15) goto if_then_669; else goto if_else_670;

  logic_rhs_667:
  r9 = cn_var_c_3;
  r10 = r9 == 39;
  goto logic_merge_668;

  logic_merge_668:
  if (r10) goto if_then_665; else goto if_merge_666;

  if_then_669:
  r16 = cn_var_实例;
  r17 = 前进(r16);
  r18 = cn_var_实例;
  r19 = 当前字符(r18);
  cn_var_c_3 = r19;
  r20 = cn_var_c_3;
  r21 = r20 != 0;
  if (r21) goto if_then_672; else goto if_merge_673;

  if_else_670:
  r24 = cn_var_实例;
  r25 = 前进(r24);
  goto if_merge_671;

  if_merge_671:
  r26 = cn_var_实例;
  r27 = 当前字符(r26);
  cn_var_c_3 = r27;
  r28 = cn_var_c_3;
  r29 = r28 != 39;
  if (r29) goto if_then_674; else goto if_merge_675;

  if_then_672:
  r22 = cn_var_实例;
  r23 = 前进(r22);
  goto if_merge_673;

  if_merge_673:
  goto if_merge_671;

  if_then_674:
  r30 = cn_var_起始行号_1;
  r31 = cn_var_起始列号_2;
  r32 = cn_var_结果_0;
  return r32;
  goto if_merge_675;

  if_merge_675:
  r33 = cn_var_实例;
  r34 = 前进(r33);
  r35 = cn_var_起始行号_1;
  r36 = cn_var_起始列号_2;
  r37 = cn_var_实例.位置;
  r38 = cn_var_起始列号_2;
  r39 = r37 - r38;
  r40 = cn_var_结果_0;
  return r40;
}

struct 词元 扫描运算符(struct 扫描器* cn_var_实例) {
  long long r0, r1, r3, r4, r30, r32, r33, r35, r37, r39, r40, r42, r44, r45, r47, r49, r51, r53, r55, r57, r58, r60, r62, r64, r65, r67, r69, r71, r72, r74, r76, r77, r79, r81, r83, r84, r86, r88, r89, r91, r93, r95, r96, r98, r100, r102, r103, r105, r107, r109, r111, r113, r115, r117, r119, r121, r123, r125, r127, r129, r131, r133, r135, r136, r137, r138, r139;
  struct 扫描器* r2;
  struct 扫描器* r29;
  struct 扫描器* r31;
  struct 扫描器* r34;
  struct 扫描器* r36;
  struct 扫描器* r38;
  struct 扫描器* r41;
  struct 扫描器* r43;
  struct 扫描器* r46;
  struct 扫描器* r48;
  struct 扫描器* r50;
  struct 扫描器* r52;
  struct 扫描器* r54;
  struct 扫描器* r56;
  struct 扫描器* r59;
  struct 扫描器* r61;
  struct 扫描器* r63;
  struct 扫描器* r66;
  struct 扫描器* r68;
  struct 扫描器* r70;
  struct 扫描器* r73;
  struct 扫描器* r75;
  struct 扫描器* r78;
  struct 扫描器* r80;
  struct 扫描器* r82;
  struct 扫描器* r85;
  struct 扫描器* r87;
  struct 扫描器* r90;
  struct 扫描器* r92;
  struct 扫描器* r94;
  struct 扫描器* r97;
  struct 扫描器* r99;
  struct 扫描器* r101;
  struct 扫描器* r104;
  struct 扫描器* r106;
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
  enum 词元类型枚举 r134;
  struct 词元 r140;

  entry:
  struct 词元 cn_var_结果_0;
  long long cn_var_起始行号_1;
  r0 = cn_var_实例.行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例.列号;
  cn_var_起始列号_2 = r1;
  long long cn_var_c_3;
  r2 = cn_var_实例;
  r3 = 当前字符(r2);
  cn_var_c_3 = r3;
  enum 词元类型枚举 cn_var_类型_4;
  cn_var_类型_4 = 词元类型枚举_错误;
  r4 = cn_var_c_3;
  r5 = r4 == 43;
  if (r5) goto case_body_677; else goto switch_check_702;

  switch_check_702:
  r6 = r4 == 45;
  if (r6) goto case_body_678; else goto switch_check_703;

  switch_check_703:
  r7 = r4 == 42;
  if (r7) goto case_body_679; else goto switch_check_704;

  switch_check_704:
  r8 = r4 == 47;
  if (r8) goto case_body_680; else goto switch_check_705;

  switch_check_705:
  r9 = r4 == 37;
  if (r9) goto case_body_681; else goto switch_check_706;

  switch_check_706:
  r10 = r4 == 61;
  if (r10) goto case_body_682; else goto switch_check_707;

  switch_check_707:
  r11 = r4 == 33;
  if (r11) goto case_body_683; else goto switch_check_708;

  switch_check_708:
  r12 = r4 == 60;
  if (r12) goto case_body_684; else goto switch_check_709;

  switch_check_709:
  r13 = r4 == 62;
  if (r13) goto case_body_685; else goto switch_check_710;

  switch_check_710:
  r14 = r4 == 38;
  if (r14) goto case_body_686; else goto switch_check_711;

  switch_check_711:
  r15 = r4 == 124;
  if (r15) goto case_body_687; else goto switch_check_712;

  switch_check_712:
  r16 = r4 == 94;
  if (r16) goto case_body_688; else goto switch_check_713;

  switch_check_713:
  r17 = r4 == 126;
  if (r17) goto case_body_689; else goto switch_check_714;

  switch_check_714:
  r18 = r4 == 40;
  if (r18) goto case_body_690; else goto switch_check_715;

  switch_check_715:
  r19 = r4 == 41;
  if (r19) goto case_body_691; else goto switch_check_716;

  switch_check_716:
  r20 = r4 == 123;
  if (r20) goto case_body_692; else goto switch_check_717;

  switch_check_717:
  r21 = r4 == 125;
  if (r21) goto case_body_693; else goto switch_check_718;

  switch_check_718:
  r22 = r4 == 91;
  if (r22) goto case_body_694; else goto switch_check_719;

  switch_check_719:
  r23 = r4 == 93;
  if (r23) goto case_body_695; else goto switch_check_720;

  switch_check_720:
  r24 = r4 == 59;
  if (r24) goto case_body_696; else goto switch_check_721;

  switch_check_721:
  r25 = r4 == 44;
  if (r25) goto case_body_697; else goto switch_check_722;

  switch_check_722:
  r26 = r4 == 46;
  if (r26) goto case_body_698; else goto switch_check_723;

  switch_check_723:
  r27 = r4 == 58;
  if (r27) goto case_body_699; else goto switch_check_724;

  switch_check_724:
  r28 = r4 == 63;
  if (r28) goto case_body_700; else goto case_default_701;

  case_body_677:
  r29 = cn_var_实例;
  r30 = 前进(r29);
  r31 = cn_var_实例;
  r32 = 当前字符(r31);
  r33 = r32 == 43;
  if (r33) goto if_then_725; else goto if_else_726;

  if_then_725:
  r34 = cn_var_实例;
  r35 = 前进(r34);
  cn_var_类型_4 = 词元类型枚举_自增;
  goto if_merge_727;

  if_else_726:
  cn_var_类型_4 = 词元类型枚举_加号;
  goto if_merge_727;

  if_merge_727:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_678:
  r36 = cn_var_实例;
  r37 = 前进(r36);
  r38 = cn_var_实例;
  r39 = 当前字符(r38);
  r40 = r39 == 62;
  if (r40) goto if_then_728; else goto if_else_729;

  if_then_728:
  r41 = cn_var_实例;
  r42 = 前进(r41);
  cn_var_类型_4 = 词元类型枚举_箭头;
  goto if_merge_730;

  if_else_729:
  r43 = cn_var_实例;
  r44 = 当前字符(r43);
  r45 = r44 == 45;
  if (r45) goto if_then_731; else goto if_else_732;

  if_merge_730:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_731:
  r46 = cn_var_实例;
  r47 = 前进(r46);
  cn_var_类型_4 = 词元类型枚举_自减;
  goto if_merge_733;

  if_else_732:
  cn_var_类型_4 = 词元类型枚举_减号;
  goto if_merge_733;

  if_merge_733:
  goto if_merge_730;

  case_body_679:
  r48 = cn_var_实例;
  r49 = 前进(r48);
  cn_var_类型_4 = 词元类型枚举_星号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_680:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  cn_var_类型_4 = 词元类型枚举_斜杠;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_681:
  r52 = cn_var_实例;
  r53 = 前进(r52);
  cn_var_类型_4 = 词元类型枚举_百分号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_682:
  r54 = cn_var_实例;
  r55 = 前进(r54);
  r56 = cn_var_实例;
  r57 = 当前字符(r56);
  r58 = r57 == 61;
  if (r58) goto if_then_734; else goto if_else_735;

  if_then_734:
  r59 = cn_var_实例;
  r60 = 前进(r59);
  cn_var_类型_4 = 词元类型枚举_等于;
  goto if_merge_736;

  if_else_735:
  cn_var_类型_4 = 词元类型枚举_赋值;
  goto if_merge_736;

  if_merge_736:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_683:
  r61 = cn_var_实例;
  r62 = 前进(r61);
  r63 = cn_var_实例;
  r64 = 当前字符(r63);
  r65 = r64 == 61;
  if (r65) goto if_then_737; else goto if_else_738;

  if_then_737:
  r66 = cn_var_实例;
  r67 = 前进(r66);
  cn_var_类型_4 = 词元类型枚举_不等于;
  goto if_merge_739;

  if_else_738:
  cn_var_类型_4 = 词元类型枚举_逻辑非;
  goto if_merge_739;

  if_merge_739:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_684:
  r68 = cn_var_实例;
  r69 = 前进(r68);
  r70 = cn_var_实例;
  r71 = 当前字符(r70);
  r72 = r71 == 60;
  if (r72) goto if_then_740; else goto if_else_741;

  if_then_740:
  r73 = cn_var_实例;
  r74 = 前进(r73);
  cn_var_类型_4 = 词元类型枚举_左移;
  goto if_merge_742;

  if_else_741:
  r75 = cn_var_实例;
  r76 = 当前字符(r75);
  r77 = r76 == 61;
  if (r77) goto if_then_743; else goto if_else_744;

  if_merge_742:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_743:
  r78 = cn_var_实例;
  r79 = 前进(r78);
  cn_var_类型_4 = 词元类型枚举_小于等于;
  goto if_merge_745;

  if_else_744:
  cn_var_类型_4 = 词元类型枚举_小于;
  goto if_merge_745;

  if_merge_745:
  goto if_merge_742;

  case_body_685:
  r80 = cn_var_实例;
  r81 = 前进(r80);
  r82 = cn_var_实例;
  r83 = 当前字符(r82);
  r84 = r83 == 62;
  if (r84) goto if_then_746; else goto if_else_747;

  if_then_746:
  r85 = cn_var_实例;
  r86 = 前进(r85);
  cn_var_类型_4 = 词元类型枚举_右移;
  goto if_merge_748;

  if_else_747:
  r87 = cn_var_实例;
  r88 = 当前字符(r87);
  r89 = r88 == 61;
  if (r89) goto if_then_749; else goto if_else_750;

  if_merge_748:
  goto switch_merge_676;
  goto switch_merge_676;

  if_then_749:
  r90 = cn_var_实例;
  r91 = 前进(r90);
  cn_var_类型_4 = 词元类型枚举_大于等于;
  goto if_merge_751;

  if_else_750:
  cn_var_类型_4 = 词元类型枚举_大于;
  goto if_merge_751;

  if_merge_751:
  goto if_merge_748;

  case_body_686:
  r92 = cn_var_实例;
  r93 = 前进(r92);
  r94 = cn_var_实例;
  r95 = 当前字符(r94);
  r96 = r95 == 38;
  if (r96) goto if_then_752; else goto if_else_753;

  if_then_752:
  r97 = cn_var_实例;
  r98 = 前进(r97);
  cn_var_类型_4 = 词元类型枚举_逻辑与;
  goto if_merge_754;

  if_else_753:
  cn_var_类型_4 = 词元类型枚举_按位与;
  goto if_merge_754;

  if_merge_754:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_687:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  r101 = cn_var_实例;
  r102 = 当前字符(r101);
  r103 = r102 == 124;
  if (r103) goto if_then_755; else goto if_else_756;

  if_then_755:
  r104 = cn_var_实例;
  r105 = 前进(r104);
  cn_var_类型_4 = 词元类型枚举_逻辑或;
  goto if_merge_757;

  if_else_756:
  cn_var_类型_4 = 词元类型枚举_按位或;
  goto if_merge_757;

  if_merge_757:
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_688:
  r106 = cn_var_实例;
  r107 = 前进(r106);
  cn_var_类型_4 = 词元类型枚举_按位异或;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_689:
  r108 = cn_var_实例;
  r109 = 前进(r108);
  cn_var_类型_4 = 词元类型枚举_按位取反;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_690:
  r110 = cn_var_实例;
  r111 = 前进(r110);
  cn_var_类型_4 = 词元类型枚举_左括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_691:
  r112 = cn_var_实例;
  r113 = 前进(r112);
  cn_var_类型_4 = 词元类型枚举_右括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_692:
  r114 = cn_var_实例;
  r115 = 前进(r114);
  cn_var_类型_4 = 词元类型枚举_左大括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_693:
  r116 = cn_var_实例;
  r117 = 前进(r116);
  cn_var_类型_4 = 词元类型枚举_右大括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_694:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  cn_var_类型_4 = 词元类型枚举_左方括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_695:
  r120 = cn_var_实例;
  r121 = 前进(r120);
  cn_var_类型_4 = 词元类型枚举_右方括号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_696:
  r122 = cn_var_实例;
  r123 = 前进(r122);
  cn_var_类型_4 = 词元类型枚举_分号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_697:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  cn_var_类型_4 = 词元类型枚举_逗号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_698:
  r126 = cn_var_实例;
  r127 = 前进(r126);
  cn_var_类型_4 = 词元类型枚举_点;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_699:
  r128 = cn_var_实例;
  r129 = 前进(r128);
  cn_var_类型_4 = 词元类型枚举_冒号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_body_700:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  cn_var_类型_4 = 词元类型枚举_问号;
  goto switch_merge_676;
  goto switch_merge_676;

  case_default_701:
  r132 = cn_var_实例;
  r133 = 前进(r132);
  cn_var_类型_4 = 词元类型枚举_错误;
  goto switch_merge_676;
  goto switch_merge_676;

  switch_merge_676:
  r134 = cn_var_类型_4;
  r135 = cn_var_起始行号_1;
  r136 = cn_var_起始列号_2;
  r137 = cn_var_实例.位置;
  r138 = cn_var_起始列号_2;
  r139 = r137 - r138;
  r140 = cn_var_结果_0;
  return r140;
}

