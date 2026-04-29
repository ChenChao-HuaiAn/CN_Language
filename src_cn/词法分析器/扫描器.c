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
  if (r1) goto if_then_602; else goto if_merge_603;

  if_then_602:
  return 0;
  goto if_merge_603;

  if_merge_603:
  struct 扫描器* cn_var_实例_0;
  r2 = cn_var_扫描器大小;
  r3 = 分配内存(r2);
  cn_var_实例_0 = r3;
  r4 = cn_var_实例_0;
  r5 = r4 == 0;
  if (r5) goto if_then_604; else goto if_merge_605;

  if_then_604:
  return 0;
  goto if_merge_605;

  if_merge_605:
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
  if (r1) goto if_then_606; else goto if_merge_607;

  if_then_606:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_607;

  if_merge_607:
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
  if (r1) goto if_then_608; else goto if_merge_609;

  if_then_608:
  r2 = cn_var_结果_0;
  return r2;
  goto if_merge_609;

  if_merge_609:
  r3 = cn_var_实例;
  跳过空白字符(r3);
  long long cn_var_起始位置_1;
  r4 = cn_var_实例->位置;
  cn_var_起始位置_1 = r4;
  long long cn_var_起始行号_2;
  r5 = cn_var_实例->行号;
  cn_var_起始行号_2 = r5;
  long long cn_var_起始列号_3;
  r6 = cn_var_实例->列号;
  cn_var_起始列号_3 = r6;
  r7 = cn_var_实例;
  r8 = 是否结束(r7);
  if (r8) goto if_then_610; else goto if_merge_611;

  if_then_610:
  r9 = cn_var_起始行号_2;
  r10 = cn_var_起始列号_3;
  r11 = cn_var_结果_0;
  return r11;
  goto if_merge_611;

  if_merge_611:
  long long cn_var_c_4;
  r12 = cn_var_实例;
  r13 = 当前字符(r12);
  cn_var_c_4 = r13;
  r14 = cn_var_c_4;
  r15 = 是标识符开头(r14);
  if (r15) goto if_then_612; else goto if_merge_613;

  if_then_612:
  r16 = cn_var_实例;
  r17 = 扫描标识符(r16);
  return r17;
  goto if_merge_613;

  if_merge_613:
  r18 = cn_var_c_4;
  r19 = 是数字(r18);
  if (r19) goto if_then_614; else goto if_merge_615;

  if_then_614:
  r20 = cn_var_实例;
  r21 = 扫描数字(r20);
  return r21;
  goto if_merge_615;

  if_merge_615:
  r22 = cn_var_c_4;
  r23 = r22 == 34;
  if (r23) goto if_then_616; else goto if_merge_617;

  if_then_616:
  r24 = cn_var_实例;
  r25 = 扫描字符串(r24);
  return r25;
  goto if_merge_617;

  if_merge_617:
  r26 = cn_var_c_4;
  r27 = r26 == 39;
  if (r27) goto if_then_618; else goto if_merge_619;

  if_then_618:
  r28 = cn_var_实例;
  r29 = 扫描字符(r28);
  return r29;
  goto if_merge_619;

  if_merge_619:
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
  if (r1) goto if_then_620; else goto if_merge_621;

  if_then_620:
  struct 词元 cn_var_空词元_0;
  r2 = cn_var_空词元_0;
  return r2;
  goto if_merge_621;

  if_merge_621:
  long long cn_var_保存位置_1;
  r3 = cn_var_实例->位置;
  cn_var_保存位置_1 = r3;
  long long cn_var_保存行号_2;
  r4 = cn_var_实例->行号;
  cn_var_保存行号_2 = r4;
  long long cn_var_保存列号_3;
  r5 = cn_var_实例->列号;
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
  long long r0, r2, r3, r4, r5, r7;
  char* r6;
  struct 扫描器* r1;
  void* r8;
  char r9;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_625; else goto logic_rhs_624;

  if_then_622:
  return 0;
  goto if_merge_623;

  if_merge_623:
  r6 = cn_var_实例->源码;
  r7 = cn_var_实例->位置;
  r8 = &r6[r7];
  r9 = *r8;
  return r9;

  logic_rhs_624:
  r3 = cn_var_实例->位置;
  r4 = cn_var_实例->源码长度;
  r5 = r3 >= r4;
  goto logic_merge_625;

  logic_merge_625:
  if (r5) goto if_then_622; else goto if_merge_623;
  return 0;
}

long long 预览字符(struct 扫描器* cn_var_实例, long long cn_var_偏移) {
  long long r0, r1, r2, r3, r4, r6, r7, r8, r9, r10, r11, r13;
  char* r12;
  struct 扫描器* r5;
  void* r14;
  char r15;

  entry:
  long long cn_var_目标位置_0;
  r0 = cn_var_实例->位置;
  r1 = cn_var_偏移;
  r2 = r0 + r1;
  cn_var_目标位置_0 = r2;
  r5 = cn_var_实例;
  r6 = r5 == 0;
  if (r6) goto logic_merge_631; else goto logic_rhs_630;

  if_then_626:
  return 0;
  goto if_merge_627;

  if_merge_627:
  r12 = cn_var_实例->源码;
  r13 = cn_var_目标位置_0;
  r14 = &r12[r13];
  r15 = *r14;
  return r15;

  logic_rhs_628:
  r9 = cn_var_目标位置_0;
  r10 = cn_var_实例->源码长度;
  r11 = r9 >= r10;
  goto logic_merge_629;

  logic_merge_629:
  if (r11) goto if_then_626; else goto if_merge_627;

  logic_rhs_630:
  r7 = cn_var_目标位置_0;
  r8 = r7 < 0;
  goto logic_merge_631;

  logic_merge_631:
  if (r8) goto logic_merge_629; else goto logic_rhs_628;
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
  if (r3) goto if_then_632; else goto if_merge_633;

  if_then_632:
  r4 = cn_var_c_0;
  return r4;
  goto if_merge_633;

  if_merge_633:
  r5 = cn_var_实例->位置;
  r6 = r5 + 1;
  r7 = cn_var_c_0;
  r8 = r7 == 10;
  if (r8) goto if_then_634; else goto if_else_635;

  if_then_634:
  r9 = cn_var_实例->行号;
  r10 = r9 + 1;
  goto if_merge_636;

  if_else_635:
  r11 = cn_var_实例->列号;
  r12 = r11 + 1;
  goto if_merge_636;

  if_merge_636:
  r13 = cn_var_c_0;
  return r13;
}

_Bool 是否结束(struct 扫描器* cn_var_实例) {
  long long r1, r2, r3, r4;
  struct 扫描器* r0;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_637; else goto if_merge_638;

  if_then_637:
  return 1;
  goto if_merge_638;

  if_merge_638:
  r2 = cn_var_实例->位置;
  r3 = cn_var_实例->源码长度;
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
  goto while_cond_639;

  while_cond_639:
  if (1) goto while_body_640; else goto while_exit_641;

  while_body_640:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = 是空白字符(r2);
  if (r3) goto if_then_642; else goto if_merge_643;

  while_exit_641:

  if_then_642:
  r4 = cn_var_实例;
  r5 = 前进(r4);
  goto while_cond_639;
  goto if_merge_643;

  if_merge_643:
  r6 = cn_var_c_0;
  r7 = cn_var_实例;
  r8 = 预览字符(r7, 1);
  r9 = 是单行注释开始(r6, r8);
  if (r9) goto if_then_644; else goto if_merge_645;

  if_then_644:
  r10 = cn_var_实例;
  跳过单行注释(r10);
  goto while_cond_639;
  goto if_merge_645;

  if_merge_645:
  r11 = cn_var_c_0;
  r12 = cn_var_实例;
  r13 = 预览字符(r12, 1);
  r14 = 是块注释开始(r11, r13);
  if (r14) goto if_then_646; else goto if_merge_647;

  if_then_646:
  r15 = cn_var_实例;
  r16 = 跳过块注释(r15);
  goto while_cond_639;
  goto if_merge_647;

  if_merge_647:
  goto while_exit_641;
  goto while_cond_639;
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
  goto while_cond_648;

  while_cond_648:
  if (1) goto while_body_649; else goto while_exit_650;

  while_body_649:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r7 = cn_var_c_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_654; else goto logic_rhs_653;

  while_exit_650:

  if_then_651:
  goto while_exit_650;
  goto if_merge_652;

  if_merge_652:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_648;

  logic_rhs_653:
  r9 = cn_var_c_0;
  r10 = r9 == 10;
  goto logic_merge_654;

  logic_merge_654:
  if (r10) goto if_then_651; else goto if_merge_652;
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
  goto while_cond_655;

  while_cond_655:
  if (1) goto while_body_656; else goto while_exit_657;

  while_body_656:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r6 = cn_var_c_0;
  r7 = r6 == 0;
  if (r7) goto if_then_658; else goto if_merge_659;

  while_exit_657:

  if_then_658:
  return 0;
  goto if_merge_659;

  if_merge_659:
  r9 = cn_var_c_0;
  r10 = r9 == 42;
  if (r10) goto logic_rhs_662; else goto logic_merge_663;

  if_then_660:
  r14 = cn_var_实例;
  r15 = 前进(r14);
  r16 = cn_var_实例;
  r17 = 前进(r16);
  return 1;
  goto if_merge_661;

  if_merge_661:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_655;

  logic_rhs_662:
  r11 = cn_var_实例;
  r12 = 预览字符(r11, 1);
  r13 = r12 == 47;
  goto logic_merge_663;

  logic_merge_663:
  if (r13) goto if_then_660; else goto if_merge_661;
  return 0;
}

struct 词元 扫描标识符(struct 扫描器* cn_var_实例) {
  long long r0, r1, r2, r4, r5, r7, r9, r10, r11, r12, r13, r14, r15, r16, r17, r20, r22, r25, r30, r31, r32;
  char* r19;
  char* r21;
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
  r0 = cn_var_实例->位置;
  cn_var_起始位置_1 = r0;
  long long cn_var_起始行号_2;
  r1 = cn_var_实例->行号;
  cn_var_起始行号_2 = r1;
  long long cn_var_起始列号_3;
  r2 = cn_var_实例->列号;
  cn_var_起始列号_3 = r2;
  goto while_cond_664;

  while_cond_664:
  if (1) goto while_body_665; else goto while_exit_666;

  while_body_665:
  long long cn_var_c_4;
  r3 = cn_var_实例;
  r4 = 当前字符(r3);
  cn_var_c_4 = r4;
  r5 = cn_var_c_4;
  r6 = 是标识符字符(r5);
  r7 = !r6;
  if (r7) goto if_then_667; else goto if_merge_668;

  while_exit_666:
  long long cn_var_长度_5;
  r10 = cn_var_实例->位置;
  r11 = cn_var_起始位置_1;
  r12 = r10 - r11;
  cn_var_长度_5 = r12;
  char* cn_var_缓冲区;
  cn_var_缓冲区 = cn_rt_array_alloc(8, 256);
  r13 = cn_var_长度_5;
  r14 = cn_var_最大标识符长度;
  r15 = r13 >= r14;
  if (r15) goto if_then_669; else goto if_merge_670;

  if_then_667:
  goto while_exit_666;
  goto if_merge_668;

  if_merge_668:
  r8 = cn_var_实例;
  r9 = 前进(r8);
  goto while_cond_664;

  if_then_669:
  r16 = cn_var_最大标识符长度;
  r17 = r16 - 1;
  cn_var_长度_5 = r17;
  goto if_merge_670;

  if_merge_670:
  r18 = cn_var_缓冲区;
  r19 = cn_var_实例->源码;
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
  r0 = cn_var_实例->位置;
  cn_var_起始位置_1 = r0;
  long long cn_var_起始行号_2;
  r1 = cn_var_实例->行号;
  cn_var_起始行号_2 = r1;
  long long cn_var_起始列号_3;
  r2 = cn_var_实例->列号;
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
  if (r8) goto if_then_671; else goto if_merge_672;

  if_then_671:
  r9 = cn_var_实例;
  r10 = 前进(r9);
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_673;

  if_merge_672:
  r27 = cn_var_c_5;
  r28 = cn_var_实例;
  r29 = 预览字符(r28, 1);
  r30 = 是二进制前缀(r27, r29);
  if (r30) goto if_then_678; else goto if_merge_679;

  while_cond_673:
  if (1) goto while_body_674; else goto while_exit_675;

  while_body_674:
  r13 = cn_var_实例;
  r14 = 当前字符(r13);
  cn_var_c_5 = r14;
  r15 = cn_var_c_5;
  r16 = 是十六进制数字(r15);
  r17 = !r16;
  if (r17) goto if_then_676; else goto if_merge_677;

  while_exit_675:
  long long cn_var_长度_6;
  r20 = cn_var_实例->位置;
  r21 = cn_var_起始位置_1;
  r22 = r20 - r21;
  cn_var_长度_6 = r22;
  r23 = cn_var_起始行号_2;
  r24 = cn_var_起始列号_3;
  r25 = cn_var_长度_6;
  r26 = cn_var_结果_0;
  return r26;
  goto if_merge_672;

  if_then_676:
  goto while_exit_675;
  goto if_merge_677;

  if_merge_677:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_673;

  if_then_678:
  r31 = cn_var_实例;
  r32 = 前进(r31);
  r33 = cn_var_实例;
  r34 = 前进(r33);
  goto while_cond_680;

  if_merge_679:
  r49 = cn_var_c_5;
  r50 = cn_var_实例;
  r51 = 预览字符(r50, 1);
  r52 = 是八进制前缀(r49, r51);
  if (r52) goto if_then_685; else goto if_merge_686;

  while_cond_680:
  if (1) goto while_body_681; else goto while_exit_682;

  while_body_681:
  r35 = cn_var_实例;
  r36 = 当前字符(r35);
  cn_var_c_5 = r36;
  r37 = cn_var_c_5;
  r38 = 是二进制数字(r37);
  r39 = !r38;
  if (r39) goto if_then_683; else goto if_merge_684;

  while_exit_682:
  long long cn_var_长度_7;
  r42 = cn_var_实例->位置;
  r43 = cn_var_起始位置_1;
  r44 = r42 - r43;
  cn_var_长度_7 = r44;
  r45 = cn_var_起始行号_2;
  r46 = cn_var_起始列号_3;
  r47 = cn_var_长度_7;
  r48 = cn_var_结果_0;
  return r48;
  goto if_merge_679;

  if_then_683:
  goto while_exit_682;
  goto if_merge_684;

  if_merge_684:
  r40 = cn_var_实例;
  r41 = 前进(r40);
  goto while_cond_680;

  if_then_685:
  r53 = cn_var_实例;
  r54 = 前进(r53);
  r55 = cn_var_实例;
  r56 = 前进(r55);
  goto while_cond_687;

  if_merge_686:
  goto while_cond_692;

  while_cond_687:
  if (1) goto while_body_688; else goto while_exit_689;

  while_body_688:
  r57 = cn_var_实例;
  r58 = 当前字符(r57);
  cn_var_c_5 = r58;
  r59 = cn_var_c_5;
  r60 = 是八进制数字(r59);
  r61 = !r60;
  if (r61) goto if_then_690; else goto if_merge_691;

  while_exit_689:
  long long cn_var_长度_8;
  r64 = cn_var_实例->位置;
  r65 = cn_var_起始位置_1;
  r66 = r64 - r65;
  cn_var_长度_8 = r66;
  r67 = cn_var_起始行号_2;
  r68 = cn_var_起始列号_3;
  r69 = cn_var_长度_8;
  r70 = cn_var_结果_0;
  return r70;
  goto if_merge_686;

  if_then_690:
  goto while_exit_689;
  goto if_merge_691;

  if_merge_691:
  r62 = cn_var_实例;
  r63 = 前进(r62);
  goto while_cond_687;

  while_cond_692:
  if (1) goto while_body_693; else goto while_exit_694;

  while_body_693:
  r71 = cn_var_实例;
  r72 = 当前字符(r71);
  cn_var_c_5 = r72;
  r73 = cn_var_c_5;
  r74 = 是数字(r73);
  r75 = !r74;
  if (r75) goto if_then_695; else goto if_merge_696;

  while_exit_694:
  r78 = cn_var_实例;
  r79 = 当前字符(r78);
  cn_var_c_5 = r79;
  r81 = cn_var_c_5;
  r82 = r81 == 46;
  if (r82) goto logic_rhs_699; else goto logic_merge_700;

  if_then_695:
  goto while_exit_694;
  goto if_merge_696;

  if_merge_696:
  r76 = cn_var_实例;
  r77 = 前进(r76);
  goto while_cond_692;

  if_then_697:
  cn_var_是浮点数_4 = 1;
  r86 = cn_var_实例;
  r87 = 前进(r86);
  goto while_cond_701;

  if_merge_698:
  r95 = cn_var_实例;
  r96 = 当前字符(r95);
  cn_var_c_5 = r96;
  r98 = cn_var_c_5;
  r99 = r98 == 101;
  if (r99) goto logic_merge_709; else goto logic_rhs_708;

  logic_rhs_699:
  r83 = cn_var_实例;
  r84 = 预览字符(r83, 1);
  r85 = 是数字(r84);
  goto logic_merge_700;

  logic_merge_700:
  if (r85) goto if_then_697; else goto if_merge_698;

  while_cond_701:
  if (1) goto while_body_702; else goto while_exit_703;

  while_body_702:
  r88 = cn_var_实例;
  r89 = 当前字符(r88);
  cn_var_c_5 = r89;
  r90 = cn_var_c_5;
  r91 = 是数字(r90);
  r92 = !r91;
  if (r92) goto if_then_704; else goto if_merge_705;

  while_exit_703:
  goto if_merge_698;

  if_then_704:
  goto while_exit_703;
  goto if_merge_705;

  if_merge_705:
  r93 = cn_var_实例;
  r94 = 前进(r93);
  goto while_cond_701;

  if_then_706:
  cn_var_是浮点数_4 = 1;
  r102 = cn_var_实例;
  r103 = 前进(r102);
  r104 = cn_var_实例;
  r105 = 当前字符(r104);
  cn_var_c_5 = r105;
  r107 = cn_var_c_5;
  r108 = r107 == 43;
  if (r108) goto logic_merge_713; else goto logic_rhs_712;

  if_merge_707:
  long long cn_var_长度_9;
  r120 = cn_var_实例->位置;
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

  logic_rhs_708:
  r100 = cn_var_c_5;
  r101 = r100 == 69;
  goto logic_merge_709;

  logic_merge_709:
  if (r101) goto if_then_706; else goto if_merge_707;

  if_then_710:
  r111 = cn_var_实例;
  r112 = 前进(r111);
  goto if_merge_711;

  if_merge_711:
  goto while_cond_714;

  logic_rhs_712:
  r109 = cn_var_c_5;
  r110 = r109 == 45;
  goto logic_merge_713;

  logic_merge_713:
  if (r110) goto if_then_710; else goto if_merge_711;

  while_cond_714:
  if (1) goto while_body_715; else goto while_exit_716;

  while_body_715:
  r113 = cn_var_实例;
  r114 = 当前字符(r113);
  cn_var_c_5 = r114;
  r115 = cn_var_c_5;
  r116 = 是数字(r115);
  r117 = !r116;
  if (r117) goto if_then_717; else goto if_merge_718;

  while_exit_716:
  goto if_merge_707;

  if_then_717:
  goto while_exit_716;
  goto if_merge_718;

  if_merge_718:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  goto while_cond_714;
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
  r0 = cn_var_实例->行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例->列号;
  cn_var_起始列号_2 = r1;
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_719;

  while_cond_719:
  if (1) goto while_body_720; else goto while_exit_721;

  while_body_720:
  long long cn_var_c_3;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_3 = r5;
  r6 = cn_var_c_3;
  r7 = r6 == 0;
  if (r7) goto if_then_722; else goto if_merge_723;

  while_exit_721:
  r27 = cn_var_起始行号_1;
  r28 = cn_var_起始列号_2;
  r29 = cn_var_实例->位置;
  r30 = cn_var_起始列号_2;
  r31 = r29 - r30;
  r32 = cn_var_结果_0;
  return r32;

  if_then_722:
  r8 = cn_var_起始行号_1;
  r9 = cn_var_起始列号_2;
  r10 = cn_var_结果_0;
  return r10;
  goto if_merge_723;

  if_merge_723:
  r11 = cn_var_c_3;
  r12 = r11 == 34;
  if (r12) goto if_then_724; else goto if_merge_725;

  if_then_724:
  r13 = cn_var_实例;
  r14 = 前进(r13);
  goto while_exit_721;
  goto if_merge_725;

  if_merge_725:
  r15 = cn_var_c_3;
  r16 = r15 == 92;
  if (r16) goto if_then_726; else goto if_merge_727;

  if_then_726:
  r17 = cn_var_实例;
  r18 = 前进(r17);
  r19 = cn_var_实例;
  r20 = 当前字符(r19);
  cn_var_c_3 = r20;
  r21 = cn_var_c_3;
  r22 = r21 != 0;
  if (r22) goto if_then_728; else goto if_merge_729;

  if_merge_727:
  r25 = cn_var_实例;
  r26 = 前进(r25);
  goto while_cond_719;

  if_then_728:
  r23 = cn_var_实例;
  r24 = 前进(r23);
  goto if_merge_729;

  if_merge_729:
  goto while_cond_719;
  goto if_merge_727;
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
  r0 = cn_var_实例->行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例->列号;
  cn_var_起始列号_2 = r1;
  r2 = cn_var_实例;
  r3 = 前进(r2);
  long long cn_var_c_3;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_3 = r5;
  r7 = cn_var_c_3;
  r8 = r7 == 0;
  if (r8) goto logic_merge_733; else goto logic_rhs_732;

  if_then_730:
  r11 = cn_var_起始行号_1;
  r12 = cn_var_起始列号_2;
  r13 = cn_var_结果_0;
  return r13;
  goto if_merge_731;

  if_merge_731:
  r14 = cn_var_c_3;
  r15 = r14 == 92;
  if (r15) goto if_then_734; else goto if_else_735;

  logic_rhs_732:
  r9 = cn_var_c_3;
  r10 = r9 == 39;
  goto logic_merge_733;

  logic_merge_733:
  if (r10) goto if_then_730; else goto if_merge_731;

  if_then_734:
  r16 = cn_var_实例;
  r17 = 前进(r16);
  r18 = cn_var_实例;
  r19 = 当前字符(r18);
  cn_var_c_3 = r19;
  r20 = cn_var_c_3;
  r21 = r20 != 0;
  if (r21) goto if_then_737; else goto if_merge_738;

  if_else_735:
  r24 = cn_var_实例;
  r25 = 前进(r24);
  goto if_merge_736;

  if_merge_736:
  r26 = cn_var_实例;
  r27 = 当前字符(r26);
  cn_var_c_3 = r27;
  r28 = cn_var_c_3;
  r29 = r28 != 39;
  if (r29) goto if_then_739; else goto if_merge_740;

  if_then_737:
  r22 = cn_var_实例;
  r23 = 前进(r22);
  goto if_merge_738;

  if_merge_738:
  goto if_merge_736;

  if_then_739:
  r30 = cn_var_起始行号_1;
  r31 = cn_var_起始列号_2;
  r32 = cn_var_结果_0;
  return r32;
  goto if_merge_740;

  if_merge_740:
  r33 = cn_var_实例;
  r34 = 前进(r33);
  r35 = cn_var_起始行号_1;
  r36 = cn_var_起始列号_2;
  r37 = cn_var_实例->位置;
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
  r0 = cn_var_实例->行号;
  cn_var_起始行号_1 = r0;
  long long cn_var_起始列号_2;
  r1 = cn_var_实例->列号;
  cn_var_起始列号_2 = r1;
  long long cn_var_c_3;
  r2 = cn_var_实例;
  r3 = 当前字符(r2);
  cn_var_c_3 = r3;
  enum 词元类型枚举 cn_var_类型_4;
  cn_var_类型_4 = 词元类型枚举_错误;
  r4 = cn_var_c_3;
  r5 = r4 == 43;
  if (r5) goto case_body_742; else goto switch_check_767;

  switch_check_767:
  r6 = r4 == 45;
  if (r6) goto case_body_743; else goto switch_check_768;

  switch_check_768:
  r7 = r4 == 42;
  if (r7) goto case_body_744; else goto switch_check_769;

  switch_check_769:
  r8 = r4 == 47;
  if (r8) goto case_body_745; else goto switch_check_770;

  switch_check_770:
  r9 = r4 == 37;
  if (r9) goto case_body_746; else goto switch_check_771;

  switch_check_771:
  r10 = r4 == 61;
  if (r10) goto case_body_747; else goto switch_check_772;

  switch_check_772:
  r11 = r4 == 33;
  if (r11) goto case_body_748; else goto switch_check_773;

  switch_check_773:
  r12 = r4 == 60;
  if (r12) goto case_body_749; else goto switch_check_774;

  switch_check_774:
  r13 = r4 == 62;
  if (r13) goto case_body_750; else goto switch_check_775;

  switch_check_775:
  r14 = r4 == 38;
  if (r14) goto case_body_751; else goto switch_check_776;

  switch_check_776:
  r15 = r4 == 124;
  if (r15) goto case_body_752; else goto switch_check_777;

  switch_check_777:
  r16 = r4 == 94;
  if (r16) goto case_body_753; else goto switch_check_778;

  switch_check_778:
  r17 = r4 == 126;
  if (r17) goto case_body_754; else goto switch_check_779;

  switch_check_779:
  r18 = r4 == 40;
  if (r18) goto case_body_755; else goto switch_check_780;

  switch_check_780:
  r19 = r4 == 41;
  if (r19) goto case_body_756; else goto switch_check_781;

  switch_check_781:
  r20 = r4 == 123;
  if (r20) goto case_body_757; else goto switch_check_782;

  switch_check_782:
  r21 = r4 == 125;
  if (r21) goto case_body_758; else goto switch_check_783;

  switch_check_783:
  r22 = r4 == 91;
  if (r22) goto case_body_759; else goto switch_check_784;

  switch_check_784:
  r23 = r4 == 93;
  if (r23) goto case_body_760; else goto switch_check_785;

  switch_check_785:
  r24 = r4 == 59;
  if (r24) goto case_body_761; else goto switch_check_786;

  switch_check_786:
  r25 = r4 == 44;
  if (r25) goto case_body_762; else goto switch_check_787;

  switch_check_787:
  r26 = r4 == 46;
  if (r26) goto case_body_763; else goto switch_check_788;

  switch_check_788:
  r27 = r4 == 58;
  if (r27) goto case_body_764; else goto switch_check_789;

  switch_check_789:
  r28 = r4 == 63;
  if (r28) goto case_body_765; else goto case_default_766;

  case_body_742:
  r29 = cn_var_实例;
  r30 = 前进(r29);
  r31 = cn_var_实例;
  r32 = 当前字符(r31);
  r33 = r32 == 43;
  if (r33) goto if_then_790; else goto if_else_791;

  if_then_790:
  r34 = cn_var_实例;
  r35 = 前进(r34);
  cn_var_类型_4 = 词元类型枚举_自增;
  goto if_merge_792;

  if_else_791:
  cn_var_类型_4 = 词元类型枚举_加号;
  goto if_merge_792;

  if_merge_792:
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_743:
  r36 = cn_var_实例;
  r37 = 前进(r36);
  r38 = cn_var_实例;
  r39 = 当前字符(r38);
  r40 = r39 == 62;
  if (r40) goto if_then_793; else goto if_else_794;

  if_then_793:
  r41 = cn_var_实例;
  r42 = 前进(r41);
  cn_var_类型_4 = 词元类型枚举_箭头;
  goto if_merge_795;

  if_else_794:
  r43 = cn_var_实例;
  r44 = 当前字符(r43);
  r45 = r44 == 45;
  if (r45) goto if_then_796; else goto if_else_797;

  if_merge_795:
  goto switch_merge_741;
  goto switch_merge_741;

  if_then_796:
  r46 = cn_var_实例;
  r47 = 前进(r46);
  cn_var_类型_4 = 词元类型枚举_自减;
  goto if_merge_798;

  if_else_797:
  cn_var_类型_4 = 词元类型枚举_减号;
  goto if_merge_798;

  if_merge_798:
  goto if_merge_795;

  case_body_744:
  r48 = cn_var_实例;
  r49 = 前进(r48);
  cn_var_类型_4 = 词元类型枚举_星号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_745:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  cn_var_类型_4 = 词元类型枚举_斜杠;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_746:
  r52 = cn_var_实例;
  r53 = 前进(r52);
  cn_var_类型_4 = 词元类型枚举_百分号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_747:
  r54 = cn_var_实例;
  r55 = 前进(r54);
  r56 = cn_var_实例;
  r57 = 当前字符(r56);
  r58 = r57 == 61;
  if (r58) goto if_then_799; else goto if_else_800;

  if_then_799:
  r59 = cn_var_实例;
  r60 = 前进(r59);
  cn_var_类型_4 = 词元类型枚举_等于;
  goto if_merge_801;

  if_else_800:
  cn_var_类型_4 = 词元类型枚举_赋值;
  goto if_merge_801;

  if_merge_801:
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_748:
  r61 = cn_var_实例;
  r62 = 前进(r61);
  r63 = cn_var_实例;
  r64 = 当前字符(r63);
  r65 = r64 == 61;
  if (r65) goto if_then_802; else goto if_else_803;

  if_then_802:
  r66 = cn_var_实例;
  r67 = 前进(r66);
  cn_var_类型_4 = 词元类型枚举_不等于;
  goto if_merge_804;

  if_else_803:
  cn_var_类型_4 = 词元类型枚举_逻辑非;
  goto if_merge_804;

  if_merge_804:
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_749:
  r68 = cn_var_实例;
  r69 = 前进(r68);
  r70 = cn_var_实例;
  r71 = 当前字符(r70);
  r72 = r71 == 60;
  if (r72) goto if_then_805; else goto if_else_806;

  if_then_805:
  r73 = cn_var_实例;
  r74 = 前进(r73);
  cn_var_类型_4 = 词元类型枚举_左移;
  goto if_merge_807;

  if_else_806:
  r75 = cn_var_实例;
  r76 = 当前字符(r75);
  r77 = r76 == 61;
  if (r77) goto if_then_808; else goto if_else_809;

  if_merge_807:
  goto switch_merge_741;
  goto switch_merge_741;

  if_then_808:
  r78 = cn_var_实例;
  r79 = 前进(r78);
  cn_var_类型_4 = 词元类型枚举_小于等于;
  goto if_merge_810;

  if_else_809:
  cn_var_类型_4 = 词元类型枚举_小于;
  goto if_merge_810;

  if_merge_810:
  goto if_merge_807;

  case_body_750:
  r80 = cn_var_实例;
  r81 = 前进(r80);
  r82 = cn_var_实例;
  r83 = 当前字符(r82);
  r84 = r83 == 62;
  if (r84) goto if_then_811; else goto if_else_812;

  if_then_811:
  r85 = cn_var_实例;
  r86 = 前进(r85);
  cn_var_类型_4 = 词元类型枚举_右移;
  goto if_merge_813;

  if_else_812:
  r87 = cn_var_实例;
  r88 = 当前字符(r87);
  r89 = r88 == 61;
  if (r89) goto if_then_814; else goto if_else_815;

  if_merge_813:
  goto switch_merge_741;
  goto switch_merge_741;

  if_then_814:
  r90 = cn_var_实例;
  r91 = 前进(r90);
  cn_var_类型_4 = 词元类型枚举_大于等于;
  goto if_merge_816;

  if_else_815:
  cn_var_类型_4 = 词元类型枚举_大于;
  goto if_merge_816;

  if_merge_816:
  goto if_merge_813;

  case_body_751:
  r92 = cn_var_实例;
  r93 = 前进(r92);
  r94 = cn_var_实例;
  r95 = 当前字符(r94);
  r96 = r95 == 38;
  if (r96) goto if_then_817; else goto if_else_818;

  if_then_817:
  r97 = cn_var_实例;
  r98 = 前进(r97);
  cn_var_类型_4 = 词元类型枚举_逻辑与;
  goto if_merge_819;

  if_else_818:
  cn_var_类型_4 = 词元类型枚举_按位与;
  goto if_merge_819;

  if_merge_819:
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_752:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  r101 = cn_var_实例;
  r102 = 当前字符(r101);
  r103 = r102 == 124;
  if (r103) goto if_then_820; else goto if_else_821;

  if_then_820:
  r104 = cn_var_实例;
  r105 = 前进(r104);
  cn_var_类型_4 = 词元类型枚举_逻辑或;
  goto if_merge_822;

  if_else_821:
  cn_var_类型_4 = 词元类型枚举_按位或;
  goto if_merge_822;

  if_merge_822:
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_753:
  r106 = cn_var_实例;
  r107 = 前进(r106);
  cn_var_类型_4 = 词元类型枚举_按位异或;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_754:
  r108 = cn_var_实例;
  r109 = 前进(r108);
  cn_var_类型_4 = 词元类型枚举_按位取反;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_755:
  r110 = cn_var_实例;
  r111 = 前进(r110);
  cn_var_类型_4 = 词元类型枚举_左括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_756:
  r112 = cn_var_实例;
  r113 = 前进(r112);
  cn_var_类型_4 = 词元类型枚举_右括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_757:
  r114 = cn_var_实例;
  r115 = 前进(r114);
  cn_var_类型_4 = 词元类型枚举_左大括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_758:
  r116 = cn_var_实例;
  r117 = 前进(r116);
  cn_var_类型_4 = 词元类型枚举_右大括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_759:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  cn_var_类型_4 = 词元类型枚举_左方括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_760:
  r120 = cn_var_实例;
  r121 = 前进(r120);
  cn_var_类型_4 = 词元类型枚举_右方括号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_761:
  r122 = cn_var_实例;
  r123 = 前进(r122);
  cn_var_类型_4 = 词元类型枚举_分号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_762:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  cn_var_类型_4 = 词元类型枚举_逗号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_763:
  r126 = cn_var_实例;
  r127 = 前进(r126);
  cn_var_类型_4 = 词元类型枚举_点;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_764:
  r128 = cn_var_实例;
  r129 = 前进(r128);
  cn_var_类型_4 = 词元类型枚举_冒号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_body_765:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  cn_var_类型_4 = 词元类型枚举_问号;
  goto switch_merge_741;
  goto switch_merge_741;

  case_default_766:
  r132 = cn_var_实例;
  r133 = 前进(r132);
  cn_var_类型_4 = 词元类型枚举_错误;
  goto switch_merge_741;
  goto switch_merge_741;

  switch_merge_741:
  r134 = cn_var_类型_4;
  r135 = cn_var_起始行号_1;
  r136 = cn_var_起始列号_2;
  r137 = cn_var_实例->位置;
  r138 = cn_var_起始列号_2;
  r139 = r137 - r138;
  r140 = cn_var_结果_0;
  return r140;
}

