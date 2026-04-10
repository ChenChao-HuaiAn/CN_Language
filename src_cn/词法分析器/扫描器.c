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
_Bool 是关键字字符串(char*);
enum 词元类型枚举 查找关键字(char*);
void* 分配内存数组(long long, long long);
long long 类型大小(long long);
char* 复制字符串副本(char*);
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
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
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
  if (r1) goto if_then_448; else goto if_merge_449;

  if_then_448:
  return 0;
  goto if_merge_449;

  if_merge_449:
  struct 扫描器* cn_var_实例_0;
  r2 = cn_var_扫描器大小;
  r3 = 分配内存(r2);
  cn_var_实例_0 = r3;
  r4 = cn_var_实例_0;
  r5 = r4 == 0;
  if (r5) goto if_then_450; else goto if_merge_451;

  if_then_450:
  return 0;
  goto if_merge_451;

  if_merge_451:
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
  if (r1) goto if_then_452; else goto if_merge_453;

  if_then_452:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_453;

  if_merge_453:
  return;
}

struct 词元 下一个词元(struct 扫描器* cn_var_实例) {
  long long r1, r5, r7, r9, r12, r13, r16, r17, r21, r25, r26, r29, r30;
  struct 扫描器* r0;
  struct 扫描器* r3;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r8;
  struct 扫描器* r10;
  struct 扫描器* r15;
  struct 扫描器* r19;
  struct 扫描器* r23;
  struct 扫描器* r27;
  struct 扫描器* r31;
  struct 扫描器* r33;
  _Bool r11;
  _Bool r18;
  _Bool r22;
  struct 词元 r2;
  struct 词元 r14;
  struct 词元 r20;
  struct 词元 r24;
  struct 词元 r28;
  struct 词元 r32;
  struct 词元 r34;

  entry:
  struct 词元 cn_var_结果_0;
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_454; else goto if_merge_455;

  if_then_454:
  r2 = cn_var_结果_0;
  return r2;
  goto if_merge_455;

  if_merge_455:
  r3 = cn_var_实例;
  跳过空白字符(r3);
  long long cn_var_起始位置_1;
  r4 = cn_var_实例;
  r5 = r4->位置;
  cn_var_起始位置_1 = r5;
  long long cn_var_起始行号_2;
  r6 = cn_var_实例;
  r7 = r6->行号;
  cn_var_起始行号_2 = r7;
  long long cn_var_起始列号_3;
  r8 = cn_var_实例;
  r9 = r8->列号;
  cn_var_起始列号_3 = r9;
  r10 = cn_var_实例;
  r11 = 是否结束(r10);
  if (r11) goto if_then_456; else goto if_merge_457;

  if_then_456:
  r12 = cn_var_起始行号_2;
  r13 = cn_var_起始列号_3;
  r14 = cn_var_结果_0;
  return r14;
  goto if_merge_457;

  if_merge_457:
  long long cn_var_c_4;
  r15 = cn_var_实例;
  r16 = 当前字符(r15);
  cn_var_c_4 = r16;
  r17 = cn_var_c_4;
  r18 = 是标识符开头(r17);
  if (r18) goto if_then_458; else goto if_merge_459;

  if_then_458:
  r19 = cn_var_实例;
  r20 = 扫描标识符(r19);
  return r20;
  goto if_merge_459;

  if_merge_459:
  r21 = cn_var_c_4;
  r22 = 是数字(r21);
  if (r22) goto if_then_460; else goto if_merge_461;

  if_then_460:
  r23 = cn_var_实例;
  r24 = 扫描数字(r23);
  return r24;
  goto if_merge_461;

  if_merge_461:
  r25 = cn_var_c_4;
  r26 = r25 == 34;
  if (r26) goto if_then_462; else goto if_merge_463;

  if_then_462:
  r27 = cn_var_实例;
  r28 = 扫描字符串(r27);
  return r28;
  goto if_merge_463;

  if_merge_463:
  r29 = cn_var_c_4;
  r30 = r29 == 39;
  if (r30) goto if_then_464; else goto if_merge_465;

  if_then_464:
  r31 = cn_var_实例;
  r32 = 扫描字符(r31);
  return r32;
  goto if_merge_465;

  if_merge_465:
  r33 = cn_var_实例;
  r34 = 扫描运算符(r33);
  return r34;
}

struct 词元 预览词元(struct 扫描器* cn_var_实例) {
  long long r1, r4, r6, r8, r11, r12, r13;
  struct 扫描器* r0;
  struct 扫描器* r3;
  struct 扫描器* r5;
  struct 扫描器* r7;
  struct 扫描器* r9;
  struct 词元 r2;
  struct 词元 r10;
  struct 词元 r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_466; else goto if_merge_467;

  if_then_466:
  struct 词元 cn_var_空词元_0;
  r2 = cn_var_空词元_0;
  return r2;
  goto if_merge_467;

  if_merge_467:
  long long cn_var_保存位置_1;
  r3 = cn_var_实例;
  r4 = r3->位置;
  cn_var_保存位置_1 = r4;
  long long cn_var_保存行号_2;
  r5 = cn_var_实例;
  r6 = r5->行号;
  cn_var_保存行号_2 = r6;
  long long cn_var_保存列号_3;
  r7 = cn_var_实例;
  r8 = r7->列号;
  cn_var_保存列号_3 = r8;
  struct 词元 cn_var_结果_4;
  r9 = cn_var_实例;
  r10 = 下一个词元(r9);
  cn_var_结果_4 = r10;
  r11 = cn_var_保存位置_1;
  r12 = cn_var_保存行号_2;
  r13 = cn_var_保存列号_3;
  r14 = cn_var_结果_4;
  return r14;
}

long long 当前字符(struct 扫描器* cn_var_实例) {
  long long r0, r2, r4, r6, r7, r9, r11;
  struct 扫描器* r1;
  struct 扫描器* r3;
  struct 扫描器* r5;
  struct 扫描器* r8;
  struct 扫描器* r10;
  void* r12;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_471; else goto logic_rhs_470;

  if_then_468:
  return 0;
  goto if_merge_469;

  if_merge_469:
  r8 = cn_var_实例;
  r9 = r8->源码;
  r10 = cn_var_实例;
  r11 = r10->位置;
  r12 = *(void**)cn_rt_array_get_element(r9, r11, 8);
  return r12;

  logic_rhs_470:
  r3 = cn_var_实例;
  r4 = r3->位置;
  r5 = cn_var_实例;
  r6 = r5->源码长度;
  r7 = r4 >= r6;
  goto logic_merge_471;

  logic_merge_471:
  if (r7) goto if_then_468; else goto if_merge_469;
  return 0;
}

long long 预览字符(struct 扫描器* cn_var_实例, long long cn_var_偏移) {
  long long r1, r2, r3, r4, r5, r7, r8, r9, r10, r12, r13, r15, r16;
  struct 扫描器* r0;
  struct 扫描器* r6;
  struct 扫描器* r11;
  struct 扫描器* r14;
  void* r17;

  entry:
  long long cn_var_目标位置_0;
  r0 = cn_var_实例;
  r1 = r0->位置;
  r2 = cn_var_偏移;
  r3 = r1 + r2;
  cn_var_目标位置_0 = r3;
  r6 = cn_var_实例;
  r7 = r6 == 0;
  if (r7) goto logic_merge_477; else goto logic_rhs_476;

  if_then_472:
  return 0;
  goto if_merge_473;

  if_merge_473:
  r14 = cn_var_实例;
  r15 = r14->源码;
  r16 = cn_var_目标位置_0;
  r17 = *(void**)cn_rt_array_get_element(r15, r16, 8);
  return r17;

  logic_rhs_474:
  r10 = cn_var_目标位置_0;
  r11 = cn_var_实例;
  r12 = r11->源码长度;
  r13 = r10 >= r12;
  goto logic_merge_475;

  logic_merge_475:
  if (r13) goto if_then_472; else goto if_merge_473;

  logic_rhs_476:
  r8 = cn_var_目标位置_0;
  r9 = r8 < 0;
  goto logic_merge_477;

  logic_merge_477:
  if (r9) goto logic_merge_475; else goto logic_rhs_474;
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
  if (r3) goto if_then_478; else goto if_merge_479;

  if_then_478:
  r4 = cn_var_c_0;
  return r4;
  goto if_merge_479;

  if_merge_479:
  r5 = cn_var_实例;
  r6 = r5->位置;
  r7 = r6 + 1;
  r8 = cn_var_c_0;
  r9 = r8 == 10;
  if (r9) goto if_then_480; else goto if_else_481;

  if_then_480:
  r10 = cn_var_实例;
  r11 = r10->行号;
  r12 = r11 + 1;
  goto if_merge_482;

  if_else_481:
  r13 = cn_var_实例;
  r14 = r13->列号;
  r15 = r14 + 1;
  goto if_merge_482;

  if_merge_482:
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
  if (r1) goto if_then_483; else goto if_merge_484;

  if_then_483:
  return 1;
  goto if_merge_484;

  if_merge_484:
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
  goto while_cond_485;

  while_cond_485:
  if (1) goto while_body_486; else goto while_exit_487;

  while_body_486:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = 是空白字符(r2);
  if (r3) goto if_then_488; else goto if_merge_489;

  while_exit_487:

  if_then_488:
  r4 = cn_var_实例;
  r5 = 前进(r4);
  goto while_cond_485;
  goto if_merge_489;

  if_merge_489:
  r6 = cn_var_c_0;
  r7 = cn_var_实例;
  r8 = 预览字符(r7, 1);
  r9 = 是单行注释开始(r6, r8);
  if (r9) goto if_then_490; else goto if_merge_491;

  if_then_490:
  r10 = cn_var_实例;
  跳过单行注释(r10);
  goto while_cond_485;
  goto if_merge_491;

  if_merge_491:
  r11 = cn_var_c_0;
  r12 = cn_var_实例;
  r13 = 预览字符(r12, 1);
  r14 = 是块注释开始(r11, r13);
  if (r14) goto if_then_492; else goto if_merge_493;

  if_then_492:
  r15 = cn_var_实例;
  r16 = 跳过块注释(r15);
  goto while_cond_485;
  goto if_merge_493;

  if_merge_493:
  goto while_exit_487;
  goto while_cond_485;
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
  goto while_cond_494;

  while_cond_494:
  if (1) goto while_body_495; else goto while_exit_496;

  while_body_495:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r7 = cn_var_c_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_500; else goto logic_rhs_499;

  while_exit_496:

  if_then_497:
  goto while_exit_496;
  goto if_merge_498;

  if_merge_498:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_494;

  logic_rhs_499:
  r9 = cn_var_c_0;
  r10 = r9 == 10;
  goto logic_merge_500;

  logic_merge_500:
  if (r10) goto if_then_497; else goto if_merge_498;
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
  goto while_cond_501;

  while_cond_501:
  if (1) goto while_body_502; else goto while_exit_503;

  while_body_502:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r6 = cn_var_c_0;
  r7 = r6 == 0;
  if (r7) goto if_then_504; else goto if_merge_505;

  while_exit_503:

  if_then_504:
  return 0;
  goto if_merge_505;

  if_merge_505:
  r9 = cn_var_c_0;
  r10 = r9 == 42;
  if (r10) goto logic_rhs_508; else goto logic_merge_509;

  if_then_506:
  r14 = cn_var_实例;
  r15 = 前进(r14);
  r16 = cn_var_实例;
  r17 = 前进(r16);
  return 1;
  goto if_merge_507;

  if_merge_507:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_501;

  logic_rhs_508:
  r11 = cn_var_实例;
  r12 = 预览字符(r11, 1);
  r13 = r12 == 47;
  goto logic_merge_509;

  logic_merge_509:
  if (r13) goto if_then_506; else goto if_merge_507;
  return 0;
}

struct 词元 扫描标识符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r10, r12, r14, r15, r16, r17, r18, r19, r20, r21, r24, r25, r26, r27, r30, r35, r36, r37;
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
  goto while_cond_510;

  while_cond_510:
  if (1) goto while_body_511; else goto while_exit_512;

  while_body_511:
  long long cn_var_c_4;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_4 = r7;
  r8 = cn_var_c_4;
  r9 = 是标识符字符(r8);
  r10 = !r9;
  if (r10) goto if_then_513; else goto if_merge_514;

  while_exit_512:
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
  if (r19) goto if_then_515; else goto if_merge_516;

  if_then_513:
  goto while_exit_512;
  goto if_merge_514;

  if_merge_514:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_510;

  if_then_515:
  r20 = cn_var_最大标识符长度;
  r21 = r20 - 1;
  cn_var_长度_5 = r21;
  goto if_merge_516;

  if_merge_516:
  r22 = cn_var_缓冲区;
  r23 = cn_var_实例;
  r24 = r23->源码;
  r25 = cn_var_起始位置_1;
  r26 = r24 + r25;
  r27 = cn_var_长度_5;
  r28 = 限长复制字符串(r22, r26, r27);
  r29 = cn_var_缓冲区;
  r30 = cn_var_长度_5;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r29, r30, &_tmp_i0, 8); }
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
  long long r1, r3, r5, r7, r8, r10, r13, r15, r17, r18, r20, r22, r24, r25, r26, r27, r28, r29, r31, r33, r36, r38, r40, r41, r43, r45, r47, r48, r49, r50, r51, r52, r54, r56, r59, r61, r63, r64, r66, r68, r70, r71, r72, r73, r74, r75, r78, r79, r81, r83, r85, r86, r87, r88, r90, r93, r95, r96, r98, r100, r102, r103, r104, r105, r106, r107, r109, r111, r112, r113, r114, r115, r116, r118, r120, r121, r123, r125, r127, r128, r129, r131, r132, r133, r134;
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
  struct 扫描器* r32;
  struct 扫描器* r35;
  struct 扫描器* r37;
  struct 扫描器* r39;
  struct 扫描器* r44;
  struct 扫描器* r46;
  struct 扫描器* r55;
  struct 扫描器* r58;
  struct 扫描器* r60;
  struct 扫描器* r62;
  struct 扫描器* r67;
  struct 扫描器* r69;
  struct 扫描器* r77;
  struct 扫描器* r82;
  struct 扫描器* r84;
  struct 扫描器* r89;
  struct 扫描器* r92;
  struct 扫描器* r94;
  struct 扫描器* r99;
  struct 扫描器* r101;
  struct 扫描器* r108;
  struct 扫描器* r110;
  struct 扫描器* r117;
  struct 扫描器* r119;
  struct 扫描器* r124;
  struct 扫描器* r126;
  _Bool r11;
  _Bool r19;
  _Bool r34;
  _Bool r42;
  _Bool r57;
  _Bool r65;
  _Bool r80;
  _Bool r91;
  _Bool r97;
  _Bool r122;
  _Bool r130;
  struct 词元 r30;
  struct 词元 r53;
  struct 词元 r76;
  struct 词元 r135;

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
  if (r11) goto if_then_517; else goto if_merge_518;

  if_then_517:
  r12 = cn_var_实例;
  r13 = 前进(r12);
  r14 = cn_var_实例;
  r15 = 前进(r14);
  goto while_cond_519;

  if_merge_518:
  r31 = cn_var_c_5;
  r32 = cn_var_实例;
  r33 = 预览字符(r32, 1);
  r34 = 是二进制前缀(r31, r33);
  if (r34) goto if_then_524; else goto if_merge_525;

  while_cond_519:
  if (1) goto while_body_520; else goto while_exit_521;

  while_body_520:
  r16 = cn_var_实例;
  r17 = 当前字符(r16);
  cn_var_c_5 = r17;
  r18 = cn_var_c_5;
  r19 = 是十六进制数字(r18);
  r20 = !r19;
  if (r20) goto if_then_522; else goto if_merge_523;

  while_exit_521:
  long long cn_var_长度_6;
  r23 = cn_var_实例;
  r24 = r23->位置;
  r25 = cn_var_起始位置_1;
  r26 = r24 - r25;
  cn_var_长度_6 = r26;
  r27 = cn_var_起始行号_2;
  r28 = cn_var_起始列号_3;
  r29 = cn_var_长度_6;
  r30 = cn_var_结果_0;
  return r30;
  goto if_merge_518;

  if_then_522:
  goto while_exit_521;
  goto if_merge_523;

  if_merge_523:
  r21 = cn_var_实例;
  r22 = 前进(r21);
  goto while_cond_519;

  if_then_524:
  r35 = cn_var_实例;
  r36 = 前进(r35);
  r37 = cn_var_实例;
  r38 = 前进(r37);
  goto while_cond_526;

  if_merge_525:
  r54 = cn_var_c_5;
  r55 = cn_var_实例;
  r56 = 预览字符(r55, 1);
  r57 = 是八进制前缀(r54, r56);
  if (r57) goto if_then_531; else goto if_merge_532;

  while_cond_526:
  if (1) goto while_body_527; else goto while_exit_528;

  while_body_527:
  r39 = cn_var_实例;
  r40 = 当前字符(r39);
  cn_var_c_5 = r40;
  r41 = cn_var_c_5;
  r42 = 是二进制数字(r41);
  r43 = !r42;
  if (r43) goto if_then_529; else goto if_merge_530;

  while_exit_528:
  long long cn_var_长度_7;
  r46 = cn_var_实例;
  r47 = r46->位置;
  r48 = cn_var_起始位置_1;
  r49 = r47 - r48;
  cn_var_长度_7 = r49;
  r50 = cn_var_起始行号_2;
  r51 = cn_var_起始列号_3;
  r52 = cn_var_长度_7;
  r53 = cn_var_结果_0;
  return r53;
  goto if_merge_525;

  if_then_529:
  goto while_exit_528;
  goto if_merge_530;

  if_merge_530:
  r44 = cn_var_实例;
  r45 = 前进(r44);
  goto while_cond_526;

  if_then_531:
  r58 = cn_var_实例;
  r59 = 前进(r58);
  r60 = cn_var_实例;
  r61 = 前进(r60);
  goto while_cond_533;

  if_merge_532:
  goto while_cond_538;

  while_cond_533:
  if (1) goto while_body_534; else goto while_exit_535;

  while_body_534:
  r62 = cn_var_实例;
  r63 = 当前字符(r62);
  cn_var_c_5 = r63;
  r64 = cn_var_c_5;
  r65 = 是八进制数字(r64);
  r66 = !r65;
  if (r66) goto if_then_536; else goto if_merge_537;

  while_exit_535:
  long long cn_var_长度_8;
  r69 = cn_var_实例;
  r70 = r69->位置;
  r71 = cn_var_起始位置_1;
  r72 = r70 - r71;
  cn_var_长度_8 = r72;
  r73 = cn_var_起始行号_2;
  r74 = cn_var_起始列号_3;
  r75 = cn_var_长度_8;
  r76 = cn_var_结果_0;
  return r76;
  goto if_merge_532;

  if_then_536:
  goto while_exit_535;
  goto if_merge_537;

  if_merge_537:
  r67 = cn_var_实例;
  r68 = 前进(r67);
  goto while_cond_533;

  while_cond_538:
  if (1) goto while_body_539; else goto while_exit_540;

  while_body_539:
  r77 = cn_var_实例;
  r78 = 当前字符(r77);
  cn_var_c_5 = r78;
  r79 = cn_var_c_5;
  r80 = 是数字(r79);
  r81 = !r80;
  if (r81) goto if_then_541; else goto if_merge_542;

  while_exit_540:
  r84 = cn_var_实例;
  r85 = 当前字符(r84);
  cn_var_c_5 = r85;
  r87 = cn_var_c_5;
  r88 = r87 == 46;
  if (r88) goto logic_rhs_545; else goto logic_merge_546;

  if_then_541:
  goto while_exit_540;
  goto if_merge_542;

  if_merge_542:
  r82 = cn_var_实例;
  r83 = 前进(r82);
  goto while_cond_538;

  if_then_543:
  cn_var_是浮点数_4 = 1;
  r92 = cn_var_实例;
  r93 = 前进(r92);
  goto while_cond_547;

  if_merge_544:
  r101 = cn_var_实例;
  r102 = 当前字符(r101);
  cn_var_c_5 = r102;
  r104 = cn_var_c_5;
  r105 = r104 == 101;
  if (r105) goto logic_merge_555; else goto logic_rhs_554;

  logic_rhs_545:
  r89 = cn_var_实例;
  r90 = 预览字符(r89, 1);
  r91 = 是数字(r90);
  goto logic_merge_546;

  logic_merge_546:
  if (r91) goto if_then_543; else goto if_merge_544;

  while_cond_547:
  if (1) goto while_body_548; else goto while_exit_549;

  while_body_548:
  r94 = cn_var_实例;
  r95 = 当前字符(r94);
  cn_var_c_5 = r95;
  r96 = cn_var_c_5;
  r97 = 是数字(r96);
  r98 = !r97;
  if (r98) goto if_then_550; else goto if_merge_551;

  while_exit_549:
  goto if_merge_544;

  if_then_550:
  goto while_exit_549;
  goto if_merge_551;

  if_merge_551:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  goto while_cond_547;

  if_then_552:
  cn_var_是浮点数_4 = 1;
  r108 = cn_var_实例;
  r109 = 前进(r108);
  r110 = cn_var_实例;
  r111 = 当前字符(r110);
  cn_var_c_5 = r111;
  r113 = cn_var_c_5;
  r114 = r113 == 43;
  if (r114) goto logic_merge_559; else goto logic_rhs_558;

  if_merge_553:
  long long cn_var_长度_9;
  r126 = cn_var_实例;
  r127 = r126->位置;
  r128 = cn_var_起始位置_1;
  r129 = r127 - r128;
  cn_var_长度_9 = r129;
  r130 = cn_var_是浮点数_4;
  r131 = (r130 ? 42 : 41);
  r132 = cn_var_起始行号_2;
  r133 = cn_var_起始列号_3;
  r134 = cn_var_长度_9;
  r135 = cn_var_结果_0;
  return r135;

  logic_rhs_554:
  r106 = cn_var_c_5;
  r107 = r106 == 69;
  goto logic_merge_555;

  logic_merge_555:
  if (r107) goto if_then_552; else goto if_merge_553;

  if_then_556:
  r117 = cn_var_实例;
  r118 = 前进(r117);
  goto if_merge_557;

  if_merge_557:
  goto while_cond_560;

  logic_rhs_558:
  r115 = cn_var_c_5;
  r116 = r115 == 45;
  goto logic_merge_559;

  logic_merge_559:
  if (r116) goto if_then_556; else goto if_merge_557;

  while_cond_560:
  if (1) goto while_body_561; else goto while_exit_562;

  while_body_561:
  r119 = cn_var_实例;
  r120 = 当前字符(r119);
  cn_var_c_5 = r120;
  r121 = cn_var_c_5;
  r122 = 是数字(r121);
  r123 = !r122;
  if (r123) goto if_then_563; else goto if_merge_564;

  while_exit_562:
  goto if_merge_553;

  if_then_563:
  goto while_exit_562;
  goto if_merge_564;

  if_merge_564:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  goto while_cond_560;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符串(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r9, r10, r11, r13, r14, r16, r17, r18, r20, r22, r23, r24, r26, r28, r29, r30, r32, r33, r34;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r15;
  struct 扫描器* r19;
  struct 扫描器* r21;
  struct 扫描器* r25;
  struct 扫描器* r27;
  struct 扫描器* r31;
  struct 词元 r12;
  struct 词元 r35;

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
  goto while_cond_565;

  while_cond_565:
  if (1) goto while_body_566; else goto while_exit_567;

  while_body_566:
  long long cn_var_c_3;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_3 = r7;
  r8 = cn_var_c_3;
  r9 = r8 == 0;
  if (r9) goto if_then_568; else goto if_merge_569;

  while_exit_567:
  r29 = cn_var_起始行号_1;
  r30 = cn_var_起始列号_2;
  r31 = cn_var_实例;
  r32 = r31->位置;
  r33 = cn_var_起始列号_2;
  r34 = r32 - r33;
  r35 = cn_var_结果_0;
  return r35;

  if_then_568:
  r10 = cn_var_起始行号_1;
  r11 = cn_var_起始列号_2;
  r12 = cn_var_结果_0;
  return r12;
  goto if_merge_569;

  if_merge_569:
  r13 = cn_var_c_3;
  r14 = r13 == 34;
  if (r14) goto if_then_570; else goto if_merge_571;

  if_then_570:
  r15 = cn_var_实例;
  r16 = 前进(r15);
  goto while_exit_567;
  goto if_merge_571;

  if_merge_571:
  r17 = cn_var_c_3;
  r18 = r17 == 92;
  if (r18) goto if_then_572; else goto if_merge_573;

  if_then_572:
  r19 = cn_var_实例;
  r20 = 前进(r19);
  r21 = cn_var_实例;
  r22 = 当前字符(r21);
  cn_var_c_3 = r22;
  r23 = cn_var_c_3;
  r24 = r23 != 0;
  if (r24) goto if_then_574; else goto if_merge_575;

  if_merge_573:
  r27 = cn_var_实例;
  r28 = 前进(r27);
  goto while_cond_565;

  if_then_574:
  r25 = cn_var_实例;
  r26 = 前进(r25);
  goto if_merge_575;

  if_merge_575:
  goto while_cond_565;
  goto if_merge_573;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r9, r10, r11, r12, r13, r14, r16, r17, r19, r21, r22, r23, r25, r27, r29, r30, r31, r32, r33, r36, r37, r38, r40, r41, r42;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r18;
  struct 扫描器* r20;
  struct 扫描器* r24;
  struct 扫描器* r26;
  struct 扫描器* r28;
  struct 扫描器* r35;
  struct 扫描器* r39;
  struct 词元 r15;
  struct 词元 r34;
  struct 词元 r43;

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
  if (r10) goto logic_merge_579; else goto logic_rhs_578;

  if_then_576:
  r13 = cn_var_起始行号_1;
  r14 = cn_var_起始列号_2;
  r15 = cn_var_结果_0;
  return r15;
  goto if_merge_577;

  if_merge_577:
  r16 = cn_var_c_3;
  r17 = r16 == 92;
  if (r17) goto if_then_580; else goto if_else_581;

  logic_rhs_578:
  r11 = cn_var_c_3;
  r12 = r11 == 39;
  goto logic_merge_579;

  logic_merge_579:
  if (r12) goto if_then_576; else goto if_merge_577;

  if_then_580:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  r20 = cn_var_实例;
  r21 = 当前字符(r20);
  cn_var_c_3 = r21;
  r22 = cn_var_c_3;
  r23 = r22 != 0;
  if (r23) goto if_then_583; else goto if_merge_584;

  if_else_581:
  r26 = cn_var_实例;
  r27 = 前进(r26);
  goto if_merge_582;

  if_merge_582:
  r28 = cn_var_实例;
  r29 = 当前字符(r28);
  cn_var_c_3 = r29;
  r30 = cn_var_c_3;
  r31 = r30 != 39;
  if (r31) goto if_then_585; else goto if_merge_586;

  if_then_583:
  r24 = cn_var_实例;
  r25 = 前进(r24);
  goto if_merge_584;

  if_merge_584:
  goto if_merge_582;

  if_then_585:
  r32 = cn_var_起始行号_1;
  r33 = cn_var_起始列号_2;
  r34 = cn_var_结果_0;
  return r34;
  goto if_merge_586;

  if_merge_586:
  r35 = cn_var_实例;
  r36 = 前进(r35);
  r37 = cn_var_起始行号_1;
  r38 = cn_var_起始列号_2;
  r39 = cn_var_实例;
  r40 = r39->位置;
  r41 = cn_var_起始列号_2;
  r42 = r40 - r41;
  r43 = cn_var_结果_0;
  return r43;
}

struct 词元 扫描运算符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6, r32, r34, r35, r37, r39, r41, r42, r44, r46, r47, r49, r51, r53, r55, r57, r59, r60, r62, r64, r66, r67, r69, r71, r73, r74, r76, r78, r79, r81, r83, r85, r86, r88, r90, r91, r93, r95, r97, r98, r100, r102, r104, r105, r107, r109, r111, r113, r115, r117, r119, r121, r123, r125, r127, r129, r131, r133, r135, r137, r138, r140, r141, r142;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r31;
  struct 扫描器* r33;
  struct 扫描器* r36;
  struct 扫描器* r38;
  struct 扫描器* r40;
  struct 扫描器* r43;
  struct 扫描器* r45;
  struct 扫描器* r48;
  struct 扫描器* r50;
  struct 扫描器* r52;
  struct 扫描器* r54;
  struct 扫描器* r56;
  struct 扫描器* r58;
  struct 扫描器* r61;
  struct 扫描器* r63;
  struct 扫描器* r65;
  struct 扫描器* r68;
  struct 扫描器* r70;
  struct 扫描器* r72;
  struct 扫描器* r75;
  struct 扫描器* r77;
  struct 扫描器* r80;
  struct 扫描器* r82;
  struct 扫描器* r84;
  struct 扫描器* r87;
  struct 扫描器* r89;
  struct 扫描器* r92;
  struct 扫描器* r94;
  struct 扫描器* r96;
  struct 扫描器* r99;
  struct 扫描器* r101;
  struct 扫描器* r103;
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
  struct 扫描器* r134;
  struct 扫描器* r139;
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
  _Bool r29;
  _Bool r30;
  enum 词元类型枚举 r136;
  struct 词元 r143;

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
  cn_var_类型_4 = 81;
  r6 = cn_var_c_3;
  r7 = r6 == 43;
  if (r7) goto case_body_588; else goto switch_check_613;

  switch_check_613:
  r8 = r6 == 45;
  if (r8) goto case_body_589; else goto switch_check_614;

  switch_check_614:
  r9 = r6 == 42;
  if (r9) goto case_body_590; else goto switch_check_615;

  switch_check_615:
  r10 = r6 == 47;
  if (r10) goto case_body_591; else goto switch_check_616;

  switch_check_616:
  r11 = r6 == 37;
  if (r11) goto case_body_592; else goto switch_check_617;

  switch_check_617:
  r12 = r6 == 61;
  if (r12) goto case_body_593; else goto switch_check_618;

  switch_check_618:
  r13 = r6 == 33;
  if (r13) goto case_body_594; else goto switch_check_619;

  switch_check_619:
  r14 = r6 == 60;
  if (r14) goto case_body_595; else goto switch_check_620;

  switch_check_620:
  r15 = r6 == 62;
  if (r15) goto case_body_596; else goto switch_check_621;

  switch_check_621:
  r16 = r6 == 38;
  if (r16) goto case_body_597; else goto switch_check_622;

  switch_check_622:
  r17 = r6 == 124;
  if (r17) goto case_body_598; else goto switch_check_623;

  switch_check_623:
  r18 = r6 == 94;
  if (r18) goto case_body_599; else goto switch_check_624;

  switch_check_624:
  r19 = r6 == 126;
  if (r19) goto case_body_600; else goto switch_check_625;

  switch_check_625:
  r20 = r6 == 40;
  if (r20) goto case_body_601; else goto switch_check_626;

  switch_check_626:
  r21 = r6 == 41;
  if (r21) goto case_body_602; else goto switch_check_627;

  switch_check_627:
  r22 = r6 == 123;
  if (r22) goto case_body_603; else goto switch_check_628;

  switch_check_628:
  r23 = r6 == 125;
  if (r23) goto case_body_604; else goto switch_check_629;

  switch_check_629:
  r24 = r6 == 91;
  if (r24) goto case_body_605; else goto switch_check_630;

  switch_check_630:
  r25 = r6 == 93;
  if (r25) goto case_body_606; else goto switch_check_631;

  switch_check_631:
  r26 = r6 == 59;
  if (r26) goto case_body_607; else goto switch_check_632;

  switch_check_632:
  r27 = r6 == 44;
  if (r27) goto case_body_608; else goto switch_check_633;

  switch_check_633:
  r28 = r6 == 46;
  if (r28) goto case_body_609; else goto switch_check_634;

  switch_check_634:
  r29 = r6 == 58;
  if (r29) goto case_body_610; else goto switch_check_635;

  switch_check_635:
  r30 = r6 == 63;
  if (r30) goto case_body_611; else goto case_default_612;

  case_body_588:
  r31 = cn_var_实例;
  r32 = 前进(r31);
  r33 = cn_var_实例;
  r34 = 当前字符(r33);
  r35 = r34 == 43;
  if (r35) goto if_then_636; else goto if_else_637;

  if_then_636:
  r36 = cn_var_实例;
  r37 = 前进(r36);
  cn_var_类型_4 = 0;
  goto if_merge_638;

  if_else_637:
  cn_var_类型_4 = 0;
  goto if_merge_638;

  if_merge_638:
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_589:
  r38 = cn_var_实例;
  r39 = 前进(r38);
  r40 = cn_var_实例;
  r41 = 当前字符(r40);
  r42 = r41 == 62;
  if (r42) goto if_then_639; else goto if_else_640;

  if_then_639:
  r43 = cn_var_实例;
  r44 = 前进(r43);
  cn_var_类型_4 = 0;
  goto if_merge_641;

  if_else_640:
  r45 = cn_var_实例;
  r46 = 当前字符(r45);
  r47 = r46 == 45;
  if (r47) goto if_then_642; else goto if_else_643;

  if_merge_641:
  goto switch_merge_587;
  goto switch_merge_587;

  if_then_642:
  r48 = cn_var_实例;
  r49 = 前进(r48);
  cn_var_类型_4 = 0;
  goto if_merge_644;

  if_else_643:
  cn_var_类型_4 = 0;
  goto if_merge_644;

  if_merge_644:
  goto if_merge_641;

  case_body_590:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_591:
  r52 = cn_var_实例;
  r53 = 前进(r52);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_592:
  r54 = cn_var_实例;
  r55 = 前进(r54);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_593:
  r56 = cn_var_实例;
  r57 = 前进(r56);
  r58 = cn_var_实例;
  r59 = 当前字符(r58);
  r60 = r59 == 61;
  if (r60) goto if_then_645; else goto if_else_646;

  if_then_645:
  r61 = cn_var_实例;
  r62 = 前进(r61);
  cn_var_类型_4 = 0;
  goto if_merge_647;

  if_else_646:
  cn_var_类型_4 = 0;
  goto if_merge_647;

  if_merge_647:
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_594:
  r63 = cn_var_实例;
  r64 = 前进(r63);
  r65 = cn_var_实例;
  r66 = 当前字符(r65);
  r67 = r66 == 61;
  if (r67) goto if_then_648; else goto if_else_649;

  if_then_648:
  r68 = cn_var_实例;
  r69 = 前进(r68);
  cn_var_类型_4 = 0;
  goto if_merge_650;

  if_else_649:
  cn_var_类型_4 = 0;
  goto if_merge_650;

  if_merge_650:
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_595:
  r70 = cn_var_实例;
  r71 = 前进(r70);
  r72 = cn_var_实例;
  r73 = 当前字符(r72);
  r74 = r73 == 60;
  if (r74) goto if_then_651; else goto if_else_652;

  if_then_651:
  r75 = cn_var_实例;
  r76 = 前进(r75);
  cn_var_类型_4 = 0;
  goto if_merge_653;

  if_else_652:
  r77 = cn_var_实例;
  r78 = 当前字符(r77);
  r79 = r78 == 61;
  if (r79) goto if_then_654; else goto if_else_655;

  if_merge_653:
  goto switch_merge_587;
  goto switch_merge_587;

  if_then_654:
  r80 = cn_var_实例;
  r81 = 前进(r80);
  cn_var_类型_4 = 0;
  goto if_merge_656;

  if_else_655:
  cn_var_类型_4 = 0;
  goto if_merge_656;

  if_merge_656:
  goto if_merge_653;

  case_body_596:
  r82 = cn_var_实例;
  r83 = 前进(r82);
  r84 = cn_var_实例;
  r85 = 当前字符(r84);
  r86 = r85 == 62;
  if (r86) goto if_then_657; else goto if_else_658;

  if_then_657:
  r87 = cn_var_实例;
  r88 = 前进(r87);
  cn_var_类型_4 = 0;
  goto if_merge_659;

  if_else_658:
  r89 = cn_var_实例;
  r90 = 当前字符(r89);
  r91 = r90 == 61;
  if (r91) goto if_then_660; else goto if_else_661;

  if_merge_659:
  goto switch_merge_587;
  goto switch_merge_587;

  if_then_660:
  r92 = cn_var_实例;
  r93 = 前进(r92);
  cn_var_类型_4 = 0;
  goto if_merge_662;

  if_else_661:
  cn_var_类型_4 = 0;
  goto if_merge_662;

  if_merge_662:
  goto if_merge_659;

  case_body_597:
  r94 = cn_var_实例;
  r95 = 前进(r94);
  r96 = cn_var_实例;
  r97 = 当前字符(r96);
  r98 = r97 == 38;
  if (r98) goto if_then_663; else goto if_else_664;

  if_then_663:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  cn_var_类型_4 = 0;
  goto if_merge_665;

  if_else_664:
  cn_var_类型_4 = 0;
  goto if_merge_665;

  if_merge_665:
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_598:
  r101 = cn_var_实例;
  r102 = 前进(r101);
  r103 = cn_var_实例;
  r104 = 当前字符(r103);
  r105 = r104 == 124;
  if (r105) goto if_then_666; else goto if_else_667;

  if_then_666:
  r106 = cn_var_实例;
  r107 = 前进(r106);
  cn_var_类型_4 = 0;
  goto if_merge_668;

  if_else_667:
  cn_var_类型_4 = 0;
  goto if_merge_668;

  if_merge_668:
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_599:
  r108 = cn_var_实例;
  r109 = 前进(r108);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_600:
  r110 = cn_var_实例;
  r111 = 前进(r110);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_601:
  r112 = cn_var_实例;
  r113 = 前进(r112);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_602:
  r114 = cn_var_实例;
  r115 = 前进(r114);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_603:
  r116 = cn_var_实例;
  r117 = 前进(r116);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_604:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_605:
  r120 = cn_var_实例;
  r121 = 前进(r120);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_606:
  r122 = cn_var_实例;
  r123 = 前进(r122);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_607:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_608:
  r126 = cn_var_实例;
  r127 = 前进(r126);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_609:
  r128 = cn_var_实例;
  r129 = 前进(r128);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_610:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_body_611:
  r132 = cn_var_实例;
  r133 = 前进(r132);
  cn_var_类型_4 = 0;
  goto switch_merge_587;
  goto switch_merge_587;

  case_default_612:
  r134 = cn_var_实例;
  r135 = 前进(r134);
  cn_var_类型_4 = 81;
  goto switch_merge_587;
  goto switch_merge_587;

  switch_merge_587:
  r136 = cn_var_类型_4;
  r137 = cn_var_起始行号_1;
  r138 = cn_var_起始列号_2;
  r139 = cn_var_实例;
  r140 = r139->位置;
  r141 = cn_var_起始列号_2;
  r142 = r140 - r141;
  r143 = cn_var_结果_0;
  return r143;
}

