#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Enum Definitions - 从导入模块
enum 词元类型枚举 {
    词元_错误 = 81,
    词元_结束 = 80,
    词元_问号 = 79,
    词元_冒号 = 78,
    词元_点 = 77,
    词元_逗号 = 76,
    词元_分号 = 75,
    词元_右方括号 = 74,
    词元_左方括号 = 73,
    词元_右大括号 = 72,
    词元_左大括号 = 71,
    词元_右括号 = 70,
    词元_左括号 = 69,
    词元_箭头 = 68,
    词元_自减 = 67,
    词元_自增 = 66,
    词元_右移 = 65,
    词元_左移 = 64,
    词元_按位取反 = 63,
    词元_按位异或 = 62,
    词元_按位或 = 61,
    词元_按位与 = 60,
    词元_逻辑非 = 59,
    词元_逻辑或 = 58,
    词元_逻辑与 = 57,
    词元_大于等于 = 56,
    词元_大于 = 55,
    词元_小于等于 = 54,
    词元_小于 = 53,
    词元_不等于 = 52,
    词元_赋值 = 51,
    词元_等于 = 50,
    词元_百分号 = 49,
    词元_斜杠 = 48,
    词元_星号 = 47,
    词元_减号 = 46,
    词元_加号 = 45,
    词元_字符字面量 = 44,
    词元_字符串字面量 = 43,
    词元_浮点字面量 = 42,
    词元_整数字面量 = 41,
    词元_标识符 = 40,
    关键字_最终 = 39,
    关键字_抛出 = 38,
    关键字_捕获 = 37,
    关键字_尝试 = 36,
    关键字_基类 = 35,
    关键字_自身 = 34,
    关键字_实现 = 33,
    关键字_抽象 = 32,
    关键字_重写 = 31,
    关键字_虚拟 = 30,
    关键字_保护 = 29,
    关键字_接口 = 28,
    关键字_类 = 27,
    关键字_无 = 26,
    关键字_假 = 25,
    关键字_真 = 24,
    关键字_静态 = 23,
    关键字_私有 = 22,
    关键字_公开 = 21,
    关键字_从 = 20,
    关键字_导入 = 19,
    关键字_变量 = 18,
    关键字_函数 = 17,
    关键字_枚举 = 16,
    关键字_结构体 = 15,
    关键字_空类型 = 14,
    关键字_布尔 = 13,
    关键字_字符串 = 12,
    关键字_小数 = 11,
    关键字_整数 = 10,
    关键字_默认 = 9,
    关键字_情况 = 8,
    关键字_选择 = 7,
    关键字_继续 = 6,
    关键字_中断 = 5,
    关键字_返回 = 4,
    关键字_循环 = 3,
    关键字_当 = 2,
    关键字_否则 = 1,
    关键字_如果 = 0
};

// Struct Definitions - 从导入模块
struct 词元;
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};
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
  long long r2, r7;
  char* r0;
  char* r6;
  void* r3;
  struct 扫描器* r4;
  struct 扫描器* r8;
  _Bool r1;
  _Bool r5;

  entry:
  r0 = cn_var_源码内容;
  r1 = r0 == 0;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 0;
  goto if_merge_1;

  if_merge_1:
  struct 扫描器* cn_var_实例_0;
  r2 = cn_var_扫描器大小;
  r3 = 分配内存(r2);
  cn_var_实例_0 = r3;
  r4 = cn_var_实例_0;
  r5 = r4 == 0;
  if (r5) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return 0;
  goto if_merge_3;

  if_merge_3:
  r6 = cn_var_源码内容;
  r7 = cn_var_长度;
  r8 = cn_var_实例_0;
  return r8;
}

void 销毁扫描器(struct 扫描器* cn_var_实例) {
  struct 扫描器* r0;
  struct 扫描器* r2;
  _Bool r1;

  entry:
  r0 = cn_var_实例;
  r1 = r0 != 0;
  if (r1) goto if_then_4; else goto if_merge_5;

  if_then_4:
  r2 = cn_var_实例;
  释放内存(r2);
  goto if_merge_5;

  if_merge_5:
  return;
}

struct 词元 下一个词元(struct 扫描器* cn_var_实例) {
  long long r5, r7, r9, r12, r13, r16, r17, r21, r25, r29;
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
  _Bool r1;
  _Bool r11;
  _Bool r18;
  _Bool r22;
  _Bool r26;
  _Bool r30;
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
  if (r1) goto if_then_6; else goto if_merge_7;

  if_then_6:
  r2 = cn_var_结果_0;
  return r2;
  goto if_merge_7;

  if_merge_7:
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
  if (r11) goto if_then_8; else goto if_merge_9;

  if_then_8:
  r12 = cn_var_起始行号_2;
  r13 = cn_var_起始列号_3;
  r14 = cn_var_结果_0;
  return r14;
  goto if_merge_9;

  if_merge_9:
  long long cn_var_c_4;
  r15 = cn_var_实例;
  r16 = 当前字符(r15);
  cn_var_c_4 = r16;
  r17 = cn_var_c_4;
  r18 = 是标识符开头(r17);
  if (r18) goto if_then_10; else goto if_merge_11;

  if_then_10:
  r19 = cn_var_实例;
  r20 = 扫描标识符(r19);
  return r20;
  goto if_merge_11;

  if_merge_11:
  r21 = cn_var_c_4;
  r22 = 是数字(r21);
  if (r22) goto if_then_12; else goto if_merge_13;

  if_then_12:
  r23 = cn_var_实例;
  r24 = 扫描数字(r23);
  return r24;
  goto if_merge_13;

  if_merge_13:
  r25 = cn_var_c_4;
  r26 = r25 == 34;
  if (r26) goto if_then_14; else goto if_merge_15;

  if_then_14:
  r27 = cn_var_实例;
  r28 = 扫描字符串(r27);
  return r28;
  goto if_merge_15;

  if_merge_15:
  r29 = cn_var_c_4;
  r30 = r29 == 39;
  if (r30) goto if_then_16; else goto if_merge_17;

  if_then_16:
  r31 = cn_var_实例;
  r32 = 扫描字符(r31);
  return r32;
  goto if_merge_17;

  if_merge_17:
  r33 = cn_var_实例;
  r34 = 扫描运算符(r33);
  return r34;
}

struct 词元 预览词元(struct 扫描器* cn_var_实例) {
  long long r4, r6, r8, r11, r12, r13;
  struct 扫描器* r0;
  struct 扫描器* r3;
  struct 扫描器* r5;
  struct 扫描器* r7;
  struct 扫描器* r9;
  _Bool r1;
  struct 词元 r2;
  struct 词元 r10;
  struct 词元 r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_18; else goto if_merge_19;

  if_then_18:
  struct 词元 cn_var_空词元_0;
  r2 = cn_var_空词元_0;
  return r2;
  goto if_merge_19;

  if_merge_19:
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
  long long r0, r4, r6, r11;
  char* r9;
  struct 扫描器* r1;
  struct 扫描器* r3;
  struct 扫描器* r5;
  struct 扫描器* r8;
  struct 扫描器* r10;
  _Bool r2;
  _Bool r7;
  char r12;

  entry:
  r1 = cn_var_实例;
  r2 = r1 == 0;
  if (r2) goto logic_merge_23; else goto logic_rhs_22;

  if_then_20:
  return 0;
  goto if_merge_21;

  if_merge_21:
  r8 = cn_var_实例;
  r9 = r8->源码;
  r10 = cn_var_实例;
  r11 = r10->位置;
  r12 = *(char*)cn_rt_array_get_element(r9, r11, 8);
  return r12;

  logic_rhs_22:
  r3 = cn_var_实例;
  r4 = r3->位置;
  r5 = cn_var_实例;
  r6 = r5->源码长度;
  r7 = r4 >= r6;
  goto logic_merge_23;

  logic_merge_23:
  if (r7) goto if_then_20; else goto if_merge_21;
  return 0;
}

long long 预览字符(struct 扫描器* cn_var_实例, long long cn_var_偏移) {
  long long r1, r2, r3, r4, r5, r8, r10, r12, r16;
  char* r15;
  struct 扫描器* r0;
  struct 扫描器* r6;
  struct 扫描器* r11;
  struct 扫描器* r14;
  _Bool r7;
  _Bool r9;
  _Bool r13;
  char r17;

  entry:
  long long cn_var_目标位置_0;
  r0 = cn_var_实例;
  r1 = r0->位置;
  r2 = cn_var_偏移;
  r3 = r1 + r2;
  cn_var_目标位置_0 = r3;
  r6 = cn_var_实例;
  r7 = r6 == 0;
  if (r7) goto logic_merge_29; else goto logic_rhs_28;

  if_then_24:
  return 0;
  goto if_merge_25;

  if_merge_25:
  r14 = cn_var_实例;
  r15 = r14->源码;
  r16 = cn_var_目标位置_0;
  r17 = *(char*)cn_rt_array_get_element(r15, r16, 8);
  return r17;

  logic_rhs_26:
  r10 = cn_var_目标位置_0;
  r11 = cn_var_实例;
  r12 = r11->源码长度;
  r13 = r10 >= r12;
  goto logic_merge_27;

  logic_merge_27:
  if (r13) goto if_then_24; else goto if_merge_25;

  logic_rhs_28:
  r8 = cn_var_目标位置_0;
  r9 = r8 < 0;
  goto logic_merge_29;

  logic_merge_29:
  if (r9) goto logic_merge_27; else goto logic_rhs_26;
  return 0;
}

long long 前进(struct 扫描器* cn_var_实例) {
  long long r1, r2, r4, r6, r7, r8, r11, r12, r14, r15, r16;
  struct 扫描器* r0;
  struct 扫描器* r5;
  struct 扫描器* r10;
  struct 扫描器* r13;
  _Bool r3;
  _Bool r9;

  entry:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = r2 == 0;
  if (r3) goto if_then_30; else goto if_merge_31;

  if_then_30:
  r4 = cn_var_c_0;
  return r4;
  goto if_merge_31;

  if_merge_31:
  r5 = cn_var_实例;
  r6 = r5->位置;
  r7 = r6 + 1;
  r8 = cn_var_c_0;
  r9 = r8 == 10;
  if (r9) goto if_then_32; else goto if_else_33;

  if_then_32:
  r10 = cn_var_实例;
  r11 = r10->行号;
  r12 = r11 + 1;
  goto if_merge_34;

  if_else_33:
  r13 = cn_var_实例;
  r14 = r13->列号;
  r15 = r14 + 1;
  goto if_merge_34;

  if_merge_34:
  r16 = cn_var_c_0;
  return r16;
}

_Bool 是否结束(struct 扫描器* cn_var_实例) {
  long long r3, r5;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  _Bool r1;
  _Bool r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_35; else goto if_merge_36;

  if_then_35:
  return 1;
  goto if_merge_36;

  if_merge_36:
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
  goto while_cond_37;

  while_cond_37:
  if (1) goto while_body_38; else goto while_exit_39;

  while_body_38:
  long long cn_var_c_0;
  r0 = cn_var_实例;
  r1 = 当前字符(r0);
  cn_var_c_0 = r1;
  r2 = cn_var_c_0;
  r3 = 是空白字符(r2);
  if (r3) goto if_then_40; else goto if_merge_41;

  while_exit_39:

  if_then_40:
  r4 = cn_var_实例;
  r5 = 前进(r4);
  goto while_cond_37;
  goto if_merge_41;

  if_merge_41:
  r6 = cn_var_c_0;
  r7 = cn_var_实例;
  r8 = 预览字符(r7, 1);
  r9 = 是单行注释开始(r6, r8);
  if (r9) goto if_then_42; else goto if_merge_43;

  if_then_42:
  r10 = cn_var_实例;
  跳过单行注释(r10);
  goto while_cond_37;
  goto if_merge_43;

  if_merge_43:
  r11 = cn_var_c_0;
  r12 = cn_var_实例;
  r13 = 预览字符(r12, 1);
  r14 = 是块注释开始(r11, r13);
  if (r14) goto if_then_44; else goto if_merge_45;

  if_then_44:
  r15 = cn_var_实例;
  r16 = 跳过块注释(r15);
  goto while_cond_37;
  goto if_merge_45;

  if_merge_45:
  goto while_exit_39;
  goto while_cond_37;
  return;
}

void 跳过单行注释(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6, r7, r9, r12;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r11;
  _Bool r8;
  _Bool r10;

  entry:
  r0 = cn_var_实例;
  r1 = 前进(r0);
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_46;

  while_cond_46:
  if (1) goto while_body_47; else goto while_exit_48;

  while_body_47:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r7 = cn_var_c_0;
  r8 = r7 == 0;
  if (r8) goto logic_merge_52; else goto logic_rhs_51;

  while_exit_48:

  if_then_49:
  goto while_exit_48;
  goto if_merge_50;

  if_merge_50:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_46;

  logic_rhs_51:
  r9 = cn_var_c_0;
  r10 = r9 == 10;
  goto logic_merge_52;

  logic_merge_52:
  if (r10) goto if_then_49; else goto if_merge_50;
  return;
}

_Bool 跳过块注释(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r6, r8, r9, r12, r15, r17, r19;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r11;
  struct 扫描器* r14;
  struct 扫描器* r16;
  struct 扫描器* r18;
  _Bool r7;
  _Bool r10;
  _Bool r13;

  entry:
  r0 = cn_var_实例;
  r1 = 前进(r0);
  r2 = cn_var_实例;
  r3 = 前进(r2);
  goto while_cond_53;

  while_cond_53:
  if (1) goto while_body_54; else goto while_exit_55;

  while_body_54:
  long long cn_var_c_0;
  r4 = cn_var_实例;
  r5 = 当前字符(r4);
  cn_var_c_0 = r5;
  r6 = cn_var_c_0;
  r7 = r6 == 0;
  if (r7) goto if_then_56; else goto if_merge_57;

  while_exit_55:

  if_then_56:
  return 0;
  goto if_merge_57;

  if_merge_57:
  r9 = cn_var_c_0;
  r10 = r9 == 42;
  if (r10) goto logic_rhs_60; else goto logic_merge_61;

  if_then_58:
  r14 = cn_var_实例;
  r15 = 前进(r14);
  r16 = cn_var_实例;
  r17 = 前进(r16);
  return 1;
  goto if_merge_59;

  if_merge_59:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  goto while_cond_53;

  logic_rhs_60:
  r11 = cn_var_实例;
  r12 = 预览字符(r11, 1);
  r13 = r12 == 47;
  goto logic_merge_61;

  logic_merge_61:
  if (r13) goto if_then_58; else goto if_merge_59;
  return 0;
}

struct 词元 扫描标识符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r12, r14, r15, r16, r17, r18, r20, r21, r25, r28, r31, r36, r37, r38;
  char* r24;
  char* r26;
  char* r27;
  char* r29;
  void* r22;
  void* r30;
  void* r32;
  void* r35;
  struct 扫描器* r0;
  struct 扫描器* r2;
  struct 扫描器* r4;
  struct 扫描器* r6;
  struct 扫描器* r11;
  struct 扫描器* r13;
  struct 扫描器* r23;
  _Bool r9;
  _Bool r10;
  _Bool r19;
  struct 词元 r39;
  enum 词元类型枚举 r33;
  enum 词元类型枚举 r34;

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
  goto while_cond_62;

  while_cond_62:
  if (1) goto while_body_63; else goto while_exit_64;

  while_body_63:
  long long cn_var_c_4;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_4 = r7;
  r8 = cn_var_c_4;
  r9 = 是标识符字符(r8);
  r10 = !r9;
  if (r10) goto if_then_65; else goto if_merge_66;

  while_exit_64:
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
  if (r19) goto if_then_67; else goto if_merge_68;

  if_then_65:
  goto while_exit_64;
  goto if_merge_66;

  if_merge_66:
  r11 = cn_var_实例;
  r12 = 前进(r11);
  goto while_cond_62;

  if_then_67:
  r20 = cn_var_最大标识符长度;
  r21 = r20 - 1;
  cn_var_长度_5 = r21;
  goto if_merge_68;

  if_merge_68:
  r22 = cn_var_缓冲区;
  r23 = cn_var_实例;
  r24 = r23->源码;
  r25 = cn_var_起始位置_1;
  r26 = cn_rt_int_to_string(r25);
  r27 = cn_rt_string_concat(r24, r26);
  r28 = cn_var_长度_5;
  r29 = 限长复制字符串(r22, r27, r28);
  r30 = cn_var_缓冲区;
  r31 = cn_var_长度_5;
    { long long _tmp_i0 = 0; cn_rt_array_set_element(r30, r31, &_tmp_i0, 8); }
  enum 词元类型枚举 cn_var_类型_6;
  r32 = cn_var_缓冲区;
  r33 = 查找关键字(r32);
  cn_var_类型_6 = r33;
  r34 = cn_var_类型_6;
  r35 = cn_var_缓冲区;
  r36 = cn_var_起始行号_2;
  r37 = cn_var_起始列号_3;
  r38 = cn_var_长度_5;
  r39 = cn_var_结果_0;
  return r39;
}

struct 词元 扫描数字(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r10, r13, r15, r17, r18, r22, r24, r25, r26, r27, r28, r29, r31, r33, r36, r38, r40, r41, r45, r47, r48, r49, r50, r51, r52, r54, r56, r59, r61, r63, r64, r68, r70, r71, r72, r73, r74, r75, r78, r79, r83, r85, r86, r87, r90, r93, r95, r96, r100, r102, r103, r104, r106, r109, r111, r112, r113, r115, r118, r120, r121, r125, r127, r128, r129, r131, r132, r133, r134;
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
  _Bool r20;
  _Bool r34;
  _Bool r42;
  _Bool r43;
  _Bool r57;
  _Bool r65;
  _Bool r66;
  _Bool r80;
  _Bool r81;
  _Bool r88;
  _Bool r91;
  _Bool r97;
  _Bool r98;
  _Bool r105;
  _Bool r107;
  _Bool r114;
  _Bool r116;
  _Bool r122;
  _Bool r123;
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
  if (r11) goto if_then_69; else goto if_merge_70;

  if_then_69:
  r12 = cn_var_实例;
  r13 = 前进(r12);
  r14 = cn_var_实例;
  r15 = 前进(r14);
  goto while_cond_71;

  if_merge_70:
  r31 = cn_var_c_5;
  r32 = cn_var_实例;
  r33 = 预览字符(r32, 1);
  r34 = 是二进制前缀(r31, r33);
  if (r34) goto if_then_76; else goto if_merge_77;

  while_cond_71:
  if (1) goto while_body_72; else goto while_exit_73;

  while_body_72:
  r16 = cn_var_实例;
  r17 = 当前字符(r16);
  cn_var_c_5 = r17;
  r18 = cn_var_c_5;
  r19 = 是十六进制数字(r18);
  r20 = !r19;
  if (r20) goto if_then_74; else goto if_merge_75;

  while_exit_73:
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
  goto if_merge_70;

  if_then_74:
  goto while_exit_73;
  goto if_merge_75;

  if_merge_75:
  r21 = cn_var_实例;
  r22 = 前进(r21);
  goto while_cond_71;

  if_then_76:
  r35 = cn_var_实例;
  r36 = 前进(r35);
  r37 = cn_var_实例;
  r38 = 前进(r37);
  goto while_cond_78;

  if_merge_77:
  r54 = cn_var_c_5;
  r55 = cn_var_实例;
  r56 = 预览字符(r55, 1);
  r57 = 是八进制前缀(r54, r56);
  if (r57) goto if_then_83; else goto if_merge_84;

  while_cond_78:
  if (1) goto while_body_79; else goto while_exit_80;

  while_body_79:
  r39 = cn_var_实例;
  r40 = 当前字符(r39);
  cn_var_c_5 = r40;
  r41 = cn_var_c_5;
  r42 = 是二进制数字(r41);
  r43 = !r42;
  if (r43) goto if_then_81; else goto if_merge_82;

  while_exit_80:
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
  goto if_merge_77;

  if_then_81:
  goto while_exit_80;
  goto if_merge_82;

  if_merge_82:
  r44 = cn_var_实例;
  r45 = 前进(r44);
  goto while_cond_78;

  if_then_83:
  r58 = cn_var_实例;
  r59 = 前进(r58);
  r60 = cn_var_实例;
  r61 = 前进(r60);
  goto while_cond_85;

  if_merge_84:
  goto while_cond_90;

  while_cond_85:
  if (1) goto while_body_86; else goto while_exit_87;

  while_body_86:
  r62 = cn_var_实例;
  r63 = 当前字符(r62);
  cn_var_c_5 = r63;
  r64 = cn_var_c_5;
  r65 = 是八进制数字(r64);
  r66 = !r65;
  if (r66) goto if_then_88; else goto if_merge_89;

  while_exit_87:
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
  goto if_merge_84;

  if_then_88:
  goto while_exit_87;
  goto if_merge_89;

  if_merge_89:
  r67 = cn_var_实例;
  r68 = 前进(r67);
  goto while_cond_85;

  while_cond_90:
  if (1) goto while_body_91; else goto while_exit_92;

  while_body_91:
  r77 = cn_var_实例;
  r78 = 当前字符(r77);
  cn_var_c_5 = r78;
  r79 = cn_var_c_5;
  r80 = 是数字(r79);
  r81 = !r80;
  if (r81) goto if_then_93; else goto if_merge_94;

  while_exit_92:
  r84 = cn_var_实例;
  r85 = 当前字符(r84);
  cn_var_c_5 = r85;
  r87 = cn_var_c_5;
  r88 = r87 == 46;
  if (r88) goto logic_rhs_97; else goto logic_merge_98;

  if_then_93:
  goto while_exit_92;
  goto if_merge_94;

  if_merge_94:
  r82 = cn_var_实例;
  r83 = 前进(r82);
  goto while_cond_90;

  if_then_95:
  cn_var_是浮点数_4 = 1;
  r92 = cn_var_实例;
  r93 = 前进(r92);
  goto while_cond_99;

  if_merge_96:
  r101 = cn_var_实例;
  r102 = 当前字符(r101);
  cn_var_c_5 = r102;
  r104 = cn_var_c_5;
  r105 = r104 == 101;
  if (r105) goto logic_merge_107; else goto logic_rhs_106;

  logic_rhs_97:
  r89 = cn_var_实例;
  r90 = 预览字符(r89, 1);
  r91 = 是数字(r90);
  goto logic_merge_98;

  logic_merge_98:
  if (r91) goto if_then_95; else goto if_merge_96;

  while_cond_99:
  if (1) goto while_body_100; else goto while_exit_101;

  while_body_100:
  r94 = cn_var_实例;
  r95 = 当前字符(r94);
  cn_var_c_5 = r95;
  r96 = cn_var_c_5;
  r97 = 是数字(r96);
  r98 = !r97;
  if (r98) goto if_then_102; else goto if_merge_103;

  while_exit_101:
  goto if_merge_96;

  if_then_102:
  goto while_exit_101;
  goto if_merge_103;

  if_merge_103:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  goto while_cond_99;

  if_then_104:
  cn_var_是浮点数_4 = 1;
  r108 = cn_var_实例;
  r109 = 前进(r108);
  r110 = cn_var_实例;
  r111 = 当前字符(r110);
  cn_var_c_5 = r111;
  r113 = cn_var_c_5;
  r114 = r113 == 43;
  if (r114) goto logic_merge_111; else goto logic_rhs_110;

  if_merge_105:
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

  logic_rhs_106:
  r106 = cn_var_c_5;
  r107 = r106 == 69;
  goto logic_merge_107;

  logic_merge_107:
  if (r107) goto if_then_104; else goto if_merge_105;

  if_then_108:
  r117 = cn_var_实例;
  r118 = 前进(r117);
  goto if_merge_109;

  if_merge_109:
  goto while_cond_112;

  logic_rhs_110:
  r115 = cn_var_c_5;
  r116 = r115 == 45;
  goto logic_merge_111;

  logic_merge_111:
  if (r116) goto if_then_108; else goto if_merge_109;

  while_cond_112:
  if (1) goto while_body_113; else goto while_exit_114;

  while_body_113:
  r119 = cn_var_实例;
  r120 = 当前字符(r119);
  cn_var_c_5 = r120;
  r121 = cn_var_c_5;
  r122 = 是数字(r121);
  r123 = !r122;
  if (r123) goto if_then_115; else goto if_merge_116;

  while_exit_114:
  goto if_merge_105;

  if_then_115:
  goto while_exit_114;
  goto if_merge_116;

  if_merge_116:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  goto while_cond_112;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符串(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r10, r11, r13, r16, r17, r20, r22, r23, r26, r28, r29, r30, r32, r33, r34;
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
  _Bool r9;
  _Bool r14;
  _Bool r18;
  _Bool r24;
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
  goto while_cond_117;

  while_cond_117:
  if (1) goto while_body_118; else goto while_exit_119;

  while_body_118:
  long long cn_var_c_3;
  r6 = cn_var_实例;
  r7 = 当前字符(r6);
  cn_var_c_3 = r7;
  r8 = cn_var_c_3;
  r9 = r8 == 0;
  if (r9) goto if_then_120; else goto if_merge_121;

  while_exit_119:
  r29 = cn_var_起始行号_1;
  r30 = cn_var_起始列号_2;
  r31 = cn_var_实例;
  r32 = r31->位置;
  r33 = cn_var_起始列号_2;
  r34 = r32 - r33;
  r35 = cn_var_结果_0;
  return r35;

  if_then_120:
  r10 = cn_var_起始行号_1;
  r11 = cn_var_起始列号_2;
  r12 = cn_var_结果_0;
  return r12;
  goto if_merge_121;

  if_merge_121:
  r13 = cn_var_c_3;
  r14 = r13 == 34;
  if (r14) goto if_then_122; else goto if_merge_123;

  if_then_122:
  r15 = cn_var_实例;
  r16 = 前进(r15);
  goto while_exit_119;
  goto if_merge_123;

  if_merge_123:
  r17 = cn_var_c_3;
  r18 = r17 == 92;
  if (r18) goto if_then_124; else goto if_merge_125;

  if_then_124:
  r19 = cn_var_实例;
  r20 = 前进(r19);
  r21 = cn_var_实例;
  r22 = 当前字符(r21);
  cn_var_c_3 = r22;
  r23 = cn_var_c_3;
  r24 = r23 != 0;
  if (r24) goto if_then_126; else goto if_merge_127;

  if_merge_125:
  r27 = cn_var_实例;
  r28 = 前进(r27);
  goto while_cond_117;

  if_then_126:
  r25 = cn_var_实例;
  r26 = 前进(r25);
  goto if_merge_127;

  if_merge_127:
  goto while_cond_117;
  goto if_merge_125;
  {
    static struct 词元 _zero = {0};
    return _zero;
  }
}

struct 词元 扫描字符(struct 扫描器* cn_var_实例) {
  long long r1, r3, r5, r7, r8, r9, r11, r13, r14, r16, r19, r21, r22, r25, r27, r29, r30, r32, r33, r36, r37, r38, r40, r41, r42;
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
  _Bool r10;
  _Bool r12;
  _Bool r17;
  _Bool r23;
  _Bool r31;
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
  if (r10) goto logic_merge_131; else goto logic_rhs_130;

  if_then_128:
  r13 = cn_var_起始行号_1;
  r14 = cn_var_起始列号_2;
  r15 = cn_var_结果_0;
  return r15;
  goto if_merge_129;

  if_merge_129:
  r16 = cn_var_c_3;
  r17 = r16 == 92;
  if (r17) goto if_then_132; else goto if_else_133;

  logic_rhs_130:
  r11 = cn_var_c_3;
  r12 = r11 == 39;
  goto logic_merge_131;

  logic_merge_131:
  if (r12) goto if_then_128; else goto if_merge_129;

  if_then_132:
  r18 = cn_var_实例;
  r19 = 前进(r18);
  r20 = cn_var_实例;
  r21 = 当前字符(r20);
  cn_var_c_3 = r21;
  r22 = cn_var_c_3;
  r23 = r22 != 0;
  if (r23) goto if_then_135; else goto if_merge_136;

  if_else_133:
  r26 = cn_var_实例;
  r27 = 前进(r26);
  goto if_merge_134;

  if_merge_134:
  r28 = cn_var_实例;
  r29 = 当前字符(r28);
  cn_var_c_3 = r29;
  r30 = cn_var_c_3;
  r31 = r30 != 39;
  if (r31) goto if_then_137; else goto if_merge_138;

  if_then_135:
  r24 = cn_var_实例;
  r25 = 前进(r24);
  goto if_merge_136;

  if_merge_136:
  goto if_merge_134;

  if_then_137:
  r32 = cn_var_起始行号_1;
  r33 = cn_var_起始列号_2;
  r34 = cn_var_结果_0;
  return r34;
  goto if_merge_138;

  if_merge_138:
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
  long long r1, r3, r5, r6, r32, r34, r37, r39, r41, r44, r46, r49, r51, r53, r55, r57, r59, r62, r64, r66, r69, r71, r73, r76, r78, r81, r83, r85, r88, r90, r93, r95, r97, r100, r102, r104, r107, r109, r111, r113, r115, r117, r119, r121, r123, r125, r127, r129, r131, r133, r135, r137, r138, r140, r141, r142;
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
  _Bool r35;
  _Bool r42;
  _Bool r47;
  _Bool r60;
  _Bool r67;
  _Bool r74;
  _Bool r79;
  _Bool r86;
  _Bool r91;
  _Bool r98;
  _Bool r105;
  struct 词元 r143;
  enum 词元类型枚举 r136;

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
  if (r7) goto case_body_140; else goto switch_check_165;

  switch_check_165:
  r8 = r6 == 45;
  if (r8) goto case_body_141; else goto switch_check_166;

  switch_check_166:
  r9 = r6 == 42;
  if (r9) goto case_body_142; else goto switch_check_167;

  switch_check_167:
  r10 = r6 == 47;
  if (r10) goto case_body_143; else goto switch_check_168;

  switch_check_168:
  r11 = r6 == 37;
  if (r11) goto case_body_144; else goto switch_check_169;

  switch_check_169:
  r12 = r6 == 61;
  if (r12) goto case_body_145; else goto switch_check_170;

  switch_check_170:
  r13 = r6 == 33;
  if (r13) goto case_body_146; else goto switch_check_171;

  switch_check_171:
  r14 = r6 == 60;
  if (r14) goto case_body_147; else goto switch_check_172;

  switch_check_172:
  r15 = r6 == 62;
  if (r15) goto case_body_148; else goto switch_check_173;

  switch_check_173:
  r16 = r6 == 38;
  if (r16) goto case_body_149; else goto switch_check_174;

  switch_check_174:
  r17 = r6 == 124;
  if (r17) goto case_body_150; else goto switch_check_175;

  switch_check_175:
  r18 = r6 == 94;
  if (r18) goto case_body_151; else goto switch_check_176;

  switch_check_176:
  r19 = r6 == 126;
  if (r19) goto case_body_152; else goto switch_check_177;

  switch_check_177:
  r20 = r6 == 40;
  if (r20) goto case_body_153; else goto switch_check_178;

  switch_check_178:
  r21 = r6 == 41;
  if (r21) goto case_body_154; else goto switch_check_179;

  switch_check_179:
  r22 = r6 == 123;
  if (r22) goto case_body_155; else goto switch_check_180;

  switch_check_180:
  r23 = r6 == 125;
  if (r23) goto case_body_156; else goto switch_check_181;

  switch_check_181:
  r24 = r6 == 91;
  if (r24) goto case_body_157; else goto switch_check_182;

  switch_check_182:
  r25 = r6 == 93;
  if (r25) goto case_body_158; else goto switch_check_183;

  switch_check_183:
  r26 = r6 == 59;
  if (r26) goto case_body_159; else goto switch_check_184;

  switch_check_184:
  r27 = r6 == 44;
  if (r27) goto case_body_160; else goto switch_check_185;

  switch_check_185:
  r28 = r6 == 46;
  if (r28) goto case_body_161; else goto switch_check_186;

  switch_check_186:
  r29 = r6 == 58;
  if (r29) goto case_body_162; else goto switch_check_187;

  switch_check_187:
  r30 = r6 == 63;
  if (r30) goto case_body_163; else goto case_default_164;

  case_body_140:
  r31 = cn_var_实例;
  r32 = 前进(r31);
  r33 = cn_var_实例;
  r34 = 当前字符(r33);
  r35 = r34 == 43;
  if (r35) goto if_then_188; else goto if_else_189;

  if_then_188:
  r36 = cn_var_实例;
  r37 = 前进(r36);
  cn_var_类型_4 = 66;
  goto if_merge_190;

  if_else_189:
  cn_var_类型_4 = 45;
  goto if_merge_190;

  if_merge_190:
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_141:
  r38 = cn_var_实例;
  r39 = 前进(r38);
  r40 = cn_var_实例;
  r41 = 当前字符(r40);
  r42 = r41 == 62;
  if (r42) goto if_then_191; else goto if_else_192;

  if_then_191:
  r43 = cn_var_实例;
  r44 = 前进(r43);
  cn_var_类型_4 = 68;
  goto if_merge_193;

  if_else_192:
  r45 = cn_var_实例;
  r46 = 当前字符(r45);
  r47 = r46 == 45;
  if (r47) goto if_then_194; else goto if_else_195;

  if_merge_193:
  goto switch_merge_139;
  goto switch_merge_139;

  if_then_194:
  r48 = cn_var_实例;
  r49 = 前进(r48);
  cn_var_类型_4 = 67;
  goto if_merge_196;

  if_else_195:
  cn_var_类型_4 = 46;
  goto if_merge_196;

  if_merge_196:
  goto if_merge_193;

  case_body_142:
  r50 = cn_var_实例;
  r51 = 前进(r50);
  cn_var_类型_4 = 47;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_143:
  r52 = cn_var_实例;
  r53 = 前进(r52);
  cn_var_类型_4 = 48;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_144:
  r54 = cn_var_实例;
  r55 = 前进(r54);
  cn_var_类型_4 = 49;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_145:
  r56 = cn_var_实例;
  r57 = 前进(r56);
  r58 = cn_var_实例;
  r59 = 当前字符(r58);
  r60 = r59 == 61;
  if (r60) goto if_then_197; else goto if_else_198;

  if_then_197:
  r61 = cn_var_实例;
  r62 = 前进(r61);
  cn_var_类型_4 = 50;
  goto if_merge_199;

  if_else_198:
  cn_var_类型_4 = 51;
  goto if_merge_199;

  if_merge_199:
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_146:
  r63 = cn_var_实例;
  r64 = 前进(r63);
  r65 = cn_var_实例;
  r66 = 当前字符(r65);
  r67 = r66 == 61;
  if (r67) goto if_then_200; else goto if_else_201;

  if_then_200:
  r68 = cn_var_实例;
  r69 = 前进(r68);
  cn_var_类型_4 = 52;
  goto if_merge_202;

  if_else_201:
  cn_var_类型_4 = 59;
  goto if_merge_202;

  if_merge_202:
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_147:
  r70 = cn_var_实例;
  r71 = 前进(r70);
  r72 = cn_var_实例;
  r73 = 当前字符(r72);
  r74 = r73 == 60;
  if (r74) goto if_then_203; else goto if_else_204;

  if_then_203:
  r75 = cn_var_实例;
  r76 = 前进(r75);
  cn_var_类型_4 = 64;
  goto if_merge_205;

  if_else_204:
  r77 = cn_var_实例;
  r78 = 当前字符(r77);
  r79 = r78 == 61;
  if (r79) goto if_then_206; else goto if_else_207;

  if_merge_205:
  goto switch_merge_139;
  goto switch_merge_139;

  if_then_206:
  r80 = cn_var_实例;
  r81 = 前进(r80);
  cn_var_类型_4 = 54;
  goto if_merge_208;

  if_else_207:
  cn_var_类型_4 = 53;
  goto if_merge_208;

  if_merge_208:
  goto if_merge_205;

  case_body_148:
  r82 = cn_var_实例;
  r83 = 前进(r82);
  r84 = cn_var_实例;
  r85 = 当前字符(r84);
  r86 = r85 == 62;
  if (r86) goto if_then_209; else goto if_else_210;

  if_then_209:
  r87 = cn_var_实例;
  r88 = 前进(r87);
  cn_var_类型_4 = 65;
  goto if_merge_211;

  if_else_210:
  r89 = cn_var_实例;
  r90 = 当前字符(r89);
  r91 = r90 == 61;
  if (r91) goto if_then_212; else goto if_else_213;

  if_merge_211:
  goto switch_merge_139;
  goto switch_merge_139;

  if_then_212:
  r92 = cn_var_实例;
  r93 = 前进(r92);
  cn_var_类型_4 = 56;
  goto if_merge_214;

  if_else_213:
  cn_var_类型_4 = 55;
  goto if_merge_214;

  if_merge_214:
  goto if_merge_211;

  case_body_149:
  r94 = cn_var_实例;
  r95 = 前进(r94);
  r96 = cn_var_实例;
  r97 = 当前字符(r96);
  r98 = r97 == 38;
  if (r98) goto if_then_215; else goto if_else_216;

  if_then_215:
  r99 = cn_var_实例;
  r100 = 前进(r99);
  cn_var_类型_4 = 57;
  goto if_merge_217;

  if_else_216:
  cn_var_类型_4 = 60;
  goto if_merge_217;

  if_merge_217:
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_150:
  r101 = cn_var_实例;
  r102 = 前进(r101);
  r103 = cn_var_实例;
  r104 = 当前字符(r103);
  r105 = r104 == 124;
  if (r105) goto if_then_218; else goto if_else_219;

  if_then_218:
  r106 = cn_var_实例;
  r107 = 前进(r106);
  cn_var_类型_4 = 58;
  goto if_merge_220;

  if_else_219:
  cn_var_类型_4 = 61;
  goto if_merge_220;

  if_merge_220:
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_151:
  r108 = cn_var_实例;
  r109 = 前进(r108);
  cn_var_类型_4 = 62;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_152:
  r110 = cn_var_实例;
  r111 = 前进(r110);
  cn_var_类型_4 = 63;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_153:
  r112 = cn_var_实例;
  r113 = 前进(r112);
  cn_var_类型_4 = 69;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_154:
  r114 = cn_var_实例;
  r115 = 前进(r114);
  cn_var_类型_4 = 70;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_155:
  r116 = cn_var_实例;
  r117 = 前进(r116);
  cn_var_类型_4 = 71;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_156:
  r118 = cn_var_实例;
  r119 = 前进(r118);
  cn_var_类型_4 = 72;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_157:
  r120 = cn_var_实例;
  r121 = 前进(r120);
  cn_var_类型_4 = 73;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_158:
  r122 = cn_var_实例;
  r123 = 前进(r122);
  cn_var_类型_4 = 74;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_159:
  r124 = cn_var_实例;
  r125 = 前进(r124);
  cn_var_类型_4 = 75;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_160:
  r126 = cn_var_实例;
  r127 = 前进(r126);
  cn_var_类型_4 = 76;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_161:
  r128 = cn_var_实例;
  r129 = 前进(r128);
  cn_var_类型_4 = 77;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_162:
  r130 = cn_var_实例;
  r131 = 前进(r130);
  cn_var_类型_4 = 78;
  goto switch_merge_139;
  goto switch_merge_139;

  case_body_163:
  r132 = cn_var_实例;
  r133 = 前进(r132);
  cn_var_类型_4 = 79;
  goto switch_merge_139;
  goto switch_merge_139;

  case_default_164:
  r134 = cn_var_实例;
  r135 = 前进(r134);
  cn_var_类型_4 = 81;
  goto switch_merge_139;
  goto switch_merge_139;

  switch_merge_139:
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

