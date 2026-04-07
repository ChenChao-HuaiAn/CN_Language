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
struct 扫描器;
struct 扫描器 {
    char* 源码;
    long long 位置;
    long long 行号;
    long long 列号;
    long long 源码长度;
};
enum 节点类型 {
    节点类型_接口类型 = 43,
    节点类型_类类型 = 42,
    节点类型_枚举类型 = 41,
    节点类型_结构体类型 = 40,
    节点类型_函数类型 = 39,
    节点类型_数组类型 = 38,
    节点类型_指针类型 = 37,
    节点类型_基础类型 = 36,
    节点类型_模板实例化表达式 = 35,
    节点类型_逻辑表达式 = 34,
    节点类型_结构体字面量表达式 = 33,
    节点类型_数组字面量表达式 = 32,
    节点类型_三元表达式 = 31,
    节点类型_赋值表达式 = 30,
    节点类型_数组访问表达式 = 29,
    节点类型_成员访问表达式 = 28,
    节点类型_函数调用表达式 = 27,
    节点类型_标识符表达式 = 26,
    节点类型_字面量表达式 = 25,
    节点类型_一元表达式 = 24,
    节点类型_二元表达式 = 23,
    节点类型_最终语句 = 22,
    节点类型_抛出语句 = 21,
    节点类型_尝试语句 = 20,
    节点类型_选择语句 = 19,
    节点类型_继续语句 = 18,
    节点类型_中断语句 = 17,
    节点类型_返回语句 = 16,
    节点类型_循环语句 = 15,
    节点类型_当语句 = 14,
    节点类型_如果语句 = 13,
    节点类型_块语句 = 12,
    节点类型_表达式语句 = 11,
    节点类型_模板结构体声明 = 10,
    节点类型_模板函数声明 = 9,
    节点类型_接口声明 = 8,
    节点类型_类声明 = 7,
    节点类型_导入声明 = 6,
    节点类型_模块声明 = 5,
    节点类型_枚举声明 = 4,
    节点类型_结构体声明 = 3,
    节点类型_变量声明 = 2,
    节点类型_函数声明 = 1,
    节点类型_程序节点 = 0
};
struct 源位置 {
    char* 文件名;
    long long 行号;
    long long 列号;
};
struct AST节点;
struct AST节点 {
    enum 节点类型 类型;
    struct 源位置 位置;
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

// Global Variables

// Forward Declarations
struct 语句节点* 解析语句(struct 解析器*);
struct 块语句节点* 解析块语句(struct 解析器*);
struct 语句节点* 解析如果语句(struct 解析器*);
struct 语句节点* 解析当语句(struct 解析器*);
struct 语句节点* 解析循环语句(struct 解析器*);
struct 语句节点* 解析返回语句(struct 解析器*);
struct 语句节点* 解析中断语句(struct 解析器*);
struct 语句节点* 解析继续语句(struct 解析器*);
struct 语句节点* 解析选择语句(struct 解析器*);
void* 解析情况列表(struct 解析器*);
struct 语句节点* 解析尝试语句(struct 解析器*);
struct 语句节点* 解析抛出语句(struct 解析器*);
struct 语句节点* 解析表达式语句(struct 解析器*);
struct 语句节点* 解析变量声明语句(struct 解析器*);
struct 语句节点* 解析语句(struct 解析器*);
struct 块语句节点* 解析块语句(struct 解析器*);
struct 语句节点* 解析如果语句(struct 解析器*);
struct 语句节点* 解析当语句(struct 解析器*);
struct 语句节点* 解析循环语句(struct 解析器*);
struct 语句节点* 解析返回语句(struct 解析器*);
struct 语句节点* 解析中断语句(struct 解析器*);
struct 语句节点* 解析继续语句(struct 解析器*);
struct 语句节点* 解析选择语句(struct 解析器*);
void* 解析情况列表(struct 解析器*);
struct 语句节点* 解析尝试语句(struct 解析器*);
struct 语句节点* 解析抛出语句(struct 解析器*);
struct 语句节点* 解析表达式语句(struct 解析器*);
struct 语句节点* 解析变量声明语句(struct 解析器*);

struct 语句节点* 解析语句(struct 解析器* cn_var_实例);
struct 块语句节点* 解析块语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析如果语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析当语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析循环语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析返回语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析中断语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析继续语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析选择语句(struct 解析器* cn_var_实例);
void* 解析情况列表(struct 解析器* cn_var_实例);
struct 语句节点* 解析尝试语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析抛出语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析表达式语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析变量声明语句(struct 解析器* cn_var_实例);
struct 语句节点* 解析语句(struct 解析器* cn_var_实例) {
  long long r1, r6, r9, r12, r15, r18, r21, r24, r27, r30, r33, r36, r39, r42;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r44;
  struct 解析器* r45;
  struct 解析器* r46;
  struct 解析器* r47;
  struct 解析器* r48;
  struct 解析器* r49;
  struct 解析器* r50;
  struct 解析器* r51;
  struct 解析器* r52;
  struct 解析器* r53;
  struct 解析器* r54;
  struct 解析器* r55;
  struct 解析器* r58;
  struct 解析器* r59;
  _Bool r7;
  _Bool r10;
  _Bool r13;
  _Bool r16;
  _Bool r19;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;
  _Bool r34;
  _Bool r37;
  _Bool r40;
  _Bool r43;
  struct 词元 r3;
  struct 词元 r56;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r38;
  enum 词元类型枚举 r41;
  enum 词元类型枚举 r57;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1293; else goto if_merge_1294;

  if_then_1293:
  return 0;
  goto if_merge_1294;

  if_merge_1294:
  r2 = cn_var_实例;
  r3 = r2->当前词元;
  r4 = r3.类型;
  r5 = cn_var_词元类型枚举;
  r6 = r5.左大括号;
  r7 = r4 == r6;
  if (r7) goto case_body_1296; else goto switch_check_1310;

  switch_check_1310:
  r8 = cn_var_词元类型枚举;
  r9 = r8.关键字_如果;
  r10 = r4 == r9;
  if (r10) goto case_body_1297; else goto switch_check_1311;

  switch_check_1311:
  r11 = cn_var_词元类型枚举;
  r12 = r11.关键字_当;
  r13 = r4 == r12;
  if (r13) goto case_body_1298; else goto switch_check_1312;

  switch_check_1312:
  r14 = cn_var_词元类型枚举;
  r15 = r14.关键字_循环;
  r16 = r4 == r15;
  if (r16) goto case_body_1299; else goto switch_check_1313;

  switch_check_1313:
  r17 = cn_var_词元类型枚举;
  r18 = r17.关键字_返回;
  r19 = r4 == r18;
  if (r19) goto case_body_1300; else goto switch_check_1314;

  switch_check_1314:
  r20 = cn_var_词元类型枚举;
  r21 = r20.关键字_中断;
  r22 = r4 == r21;
  if (r22) goto case_body_1301; else goto switch_check_1315;

  switch_check_1315:
  r23 = cn_var_词元类型枚举;
  r24 = r23.关键字_继续;
  r25 = r4 == r24;
  if (r25) goto case_body_1302; else goto switch_check_1316;

  switch_check_1316:
  r26 = cn_var_词元类型枚举;
  r27 = r26.关键字_选择;
  r28 = r4 == r27;
  if (r28) goto case_body_1303; else goto switch_check_1317;

  switch_check_1317:
  r29 = cn_var_词元类型枚举;
  r30 = r29.关键字_尝试;
  r31 = r4 == r30;
  if (r31) goto case_body_1304; else goto switch_check_1318;

  switch_check_1318:
  r32 = cn_var_词元类型枚举;
  r33 = r32.关键字_抛出;
  r34 = r4 == r33;
  if (r34) goto case_body_1305; else goto switch_check_1319;

  switch_check_1319:
  r35 = cn_var_词元类型枚举;
  r36 = r35.关键字_变量;
  r37 = r4 == r36;
  if (r37) goto case_body_1306; else goto switch_check_1320;

  switch_check_1320:
  r38 = cn_var_词元类型枚举;
  r39 = r38.关键字_常量;
  r40 = r4 == r39;
  if (r40) goto case_body_1307; else goto switch_check_1321;

  switch_check_1321:
  r41 = cn_var_词元类型枚举;
  r42 = r41.关键字_静态;
  r43 = r4 == r42;
  if (r43) goto case_body_1308; else goto case_default_1309;

  case_body_1296:
  r44 = cn_var_实例;
  解析块语句(r44);
  return;
  goto switch_merge_1295;

  case_body_1297:
  r45 = cn_var_实例;
  解析如果语句(r45);
  return;
  goto switch_merge_1295;

  case_body_1298:
  r46 = cn_var_实例;
  解析当语句(r46);
  return;
  goto switch_merge_1295;

  case_body_1299:
  r47 = cn_var_实例;
  解析循环语句(r47);
  return;
  goto switch_merge_1295;

  case_body_1300:
  r48 = cn_var_实例;
  解析返回语句(r48);
  return;
  goto switch_merge_1295;

  case_body_1301:
  r49 = cn_var_实例;
  解析中断语句(r49);
  return;
  goto switch_merge_1295;

  case_body_1302:
  r50 = cn_var_实例;
  解析继续语句(r50);
  return;
  goto switch_merge_1295;

  case_body_1303:
  r51 = cn_var_实例;
  解析选择语句(r51);
  return;
  goto switch_merge_1295;

  case_body_1304:
  r52 = cn_var_实例;
  解析尝试语句(r52);
  return;
  goto switch_merge_1295;

  case_body_1305:
  r53 = cn_var_实例;
  解析抛出语句(r53);
  return;
  goto switch_merge_1295;

  case_body_1306:
  goto switch_merge_1295;

  case_body_1307:
  goto switch_merge_1295;

  case_body_1308:
  r54 = cn_var_实例;
  解析变量声明语句(r54);
  return;
  goto switch_merge_1295;

  case_default_1309:
  r55 = cn_var_实例;
  r56 = r55->当前词元;
  r57 = r56.类型;
  是否类型关键字(r57);
  goto if_then_1322;

  if_then_1322:
  r58 = cn_var_实例;
  解析变量声明语句(r58);
  return;
  goto if_merge_1323;

  if_merge_1323:
  r59 = cn_var_实例;
  解析表达式语句(r59);
  return;
  goto switch_merge_1295;

  switch_merge_1295:
  return NULL;
}

struct 块语句节点* 解析块语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r7, r10, r11, r14, r15, r18, r23;
  struct 解析器* r0;
  struct 块语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r8;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 语句节点* r17;
  struct 块语句节点* r19;
  struct 语句节点* r20;
  struct 解析器* r21;
  struct 块语句节点* r24;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r22;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1324; else goto if_merge_1325;

  if_then_1324:
  return 0;
  goto if_merge_1325;

  if_merge_1325:
  struct 块语句节点* cn_var_块节点_0;
  创建块语句();
  cn_var_块节点_0 = /* NONE */;
  r2 = cn_var_块节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1326; else goto if_merge_1327;

  if_then_1326:
  return 0;
  goto if_merge_1327;

  if_merge_1327:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.左大括号;
  期望(r4, r6);
  goto while_cond_1328;

  while_cond_1328:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.右大括号;
  检查(r8, r10);
  r11 = !/* NONE */;
  if (r11) goto logic_rhs_1331; else goto logic_merge_1332;

  while_body_1329:
  struct 语句节点* cn_var_语句_1;
  r16 = cn_var_实例;
  解析语句(r16);
  cn_var_语句_1 = /* NONE */;
  r17 = cn_var_语句_1;
  r18 = r17 != 0;
  if (r18) goto if_then_1333; else goto if_merge_1334;

  while_exit_1330:
  r21 = cn_var_实例;
  r22 = cn_var_词元类型枚举;
  r23 = r22.右大括号;
  期望(r21, r23);
  r24 = cn_var_块节点_0;
  return r24;

  logic_rhs_1331:
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.结束;
  检查(r12, r14);
  r15 = !/* NONE */;
  goto logic_merge_1332;

  logic_merge_1332:
  if (r15) goto while_body_1329; else goto while_exit_1330;

  if_then_1333:
  r19 = cn_var_块节点_0;
  r20 = cn_var_语句_1;
  块语句添加(r19, r20);
  goto if_merge_1334;

  if_merge_1334:
  goto while_cond_1328;
  return NULL;
}

struct 语句节点* 解析如果语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r11, r15, r19;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 解析器* r12;
  struct 解析器* r13;
  struct 解析器* r16;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r21;
  struct 语句节点* r22;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r18;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1335; else goto if_merge_1336;

  if_then_1335:
  return 0;
  goto if_merge_1336;

  if_merge_1336:
  struct 语句节点* cn_var_节点_0;
  创建如果语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1337; else goto if_merge_1338;

  if_then_1337:
  return 0;
  goto if_merge_1338;

  if_merge_1338:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左括号;
  期望(r5, r7);
  r8 = cn_var_实例;
  解析表达式(r8);
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.右括号;
  期望(r9, r11);
  r12 = cn_var_实例;
  解析块语句(r12);
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.关键字_否则;
  检查(r13, r15);
  goto if_then_1339;

  if_then_1339:
  r16 = cn_var_实例;
  前进词元(r16);
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.关键字_如果;
  检查(r17, r19);
  goto if_then_1341;

  if_merge_1340:
  r22 = cn_var_节点_0;
  return r22;

  if_then_1341:
  r20 = cn_var_实例;
  解析如果语句(r20);
  goto if_merge_1343;

  if_else_1342:
  r21 = cn_var_实例;
  解析块语句(r21);
  goto if_merge_1343;

  if_merge_1343:
  goto if_merge_1340;
  return NULL;
}

struct 语句节点* 解析当语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r11;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 解析器* r12;
  struct 语句节点* r13;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1344; else goto if_merge_1345;

  if_then_1344:
  return 0;
  goto if_merge_1345;

  if_merge_1345:
  struct 语句节点* cn_var_节点_0;
  创建当语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1346; else goto if_merge_1347;

  if_then_1346:
  return 0;
  goto if_merge_1347;

  if_merge_1347:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左括号;
  期望(r5, r7);
  r8 = cn_var_实例;
  解析表达式(r8);
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.右括号;
  期望(r9, r11);
  r12 = cn_var_实例;
  解析块语句(r12);
  r13 = cn_var_节点_0;
  return r13;
}

struct 语句节点* 解析循环语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r10, r11, r12, r18, r24, r25, r29, r32, r33, r37;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r13;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r26;
  struct 解析器* r27;
  struct 解析器* r30;
  struct 解析器* r34;
  struct 解析器* r35;
  struct 解析器* r38;
  struct 语句节点* r39;
  struct 词元 r14;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r36;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1348; else goto if_merge_1349;

  if_then_1348:
  return 0;
  goto if_merge_1349;

  if_merge_1349:
  struct 语句节点* cn_var_节点_0;
  创建循环语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1350; else goto if_merge_1351;

  if_then_1350:
  return 0;
  goto if_merge_1351;

  if_merge_1351:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左括号;
  期望(r5, r7);
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.分号;
  检查(r8, r10);
  r11 = !/* NONE */;
  if (r11) goto if_then_1352; else goto if_else_1353;

  if_then_1352:
  r13 = cn_var_实例;
  r14 = r13->当前词元;
  r15 = r14.类型;
  是否类型关键字(r15);
  goto logic_merge_1359;

  if_else_1353:
  r21 = cn_var_实例;
  前进词元(r21);
  goto if_merge_1354;

  if_merge_1354:
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.分号;
  检查(r22, r24);
  r25 = !/* NONE */;
  if (r25) goto if_then_1360; else goto if_merge_1361;

  if_then_1355:
  r19 = cn_var_实例;
  解析变量声明语句(r19);
  goto if_merge_1357;

  if_else_1356:
  r20 = cn_var_实例;
  解析表达式语句(r20);
  goto if_merge_1357;

  if_merge_1357:
  goto if_merge_1354;

  logic_rhs_1358:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.关键字_变量;
  检查(r16, r18);
  goto logic_merge_1359;

  logic_merge_1359:
  goto if_then_1355;

  if_then_1360:
  r26 = cn_var_实例;
  解析表达式(r26);
  goto if_merge_1361;

  if_merge_1361:
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.分号;
  期望(r27, r29);
  r30 = cn_var_实例;
  r31 = cn_var_词元类型枚举;
  r32 = r31.右括号;
  检查(r30, r32);
  r33 = !/* NONE */;
  if (r33) goto if_then_1362; else goto if_merge_1363;

  if_then_1362:
  r34 = cn_var_实例;
  解析表达式(r34);
  goto if_merge_1363;

  if_merge_1363:
  r35 = cn_var_实例;
  r36 = cn_var_词元类型枚举;
  r37 = r36.右括号;
  期望(r35, r37);
  r38 = cn_var_实例;
  解析块语句(r38);
  r39 = cn_var_节点_0;
  return r39;
}

struct 语句节点* 解析返回语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r8, r12;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 解析器* r10;
  struct 语句节点* r13;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r11;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1364; else goto if_merge_1365;

  if_then_1364:
  return 0;
  goto if_merge_1365;

  if_merge_1365:
  struct 语句节点* cn_var_节点_0;
  创建返回语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1366; else goto if_merge_1367;

  if_then_1366:
  return 0;
  goto if_merge_1367;

  if_merge_1367:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.分号;
  检查(r5, r7);
  r8 = !/* NONE */;
  if (r8) goto if_then_1368; else goto if_merge_1369;

  if_then_1368:
  r9 = cn_var_实例;
  解析表达式(r9);
  goto if_merge_1369;

  if_merge_1369:
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.分号;
  期望(r10, r12);
  r13 = cn_var_节点_0;
  return r13;
}

struct 语句节点* 解析中断语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r8;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1370; else goto if_merge_1371;

  if_then_1370:
  return 0;
  goto if_merge_1371;

  if_merge_1371:
  struct 语句节点* cn_var_节点_0;
  创建中断语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1372; else goto if_merge_1373;

  if_then_1372:
  return 0;
  goto if_merge_1373;

  if_merge_1373:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.分号;
  期望(r5, r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 语句节点* 解析继续语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r8;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1374; else goto if_merge_1375;

  if_then_1374:
  return 0;
  goto if_merge_1375;

  if_merge_1375:
  struct 语句节点* cn_var_节点_0;
  创建继续语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1376; else goto if_merge_1377;

  if_then_1376:
  return 0;
  goto if_merge_1377;

  if_merge_1377:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.分号;
  期望(r5, r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 语句节点* 解析选择语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r11, r14, r18;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r9;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r16;
  struct 语句节点* r19;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r17;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1378; else goto if_merge_1379;

  if_then_1378:
  return 0;
  goto if_merge_1379;

  if_merge_1379:
  struct 语句节点* cn_var_节点_0;
  创建选择语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1380; else goto if_merge_1381;

  if_then_1380:
  return 0;
  goto if_merge_1381;

  if_merge_1381:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.左括号;
  期望(r5, r7);
  r8 = cn_var_实例;
  解析表达式(r8);
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.右括号;
  期望(r9, r11);
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.左大括号;
  期望(r12, r14);
  r15 = cn_var_实例;
  解析情况列表(r15);
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.右大括号;
  期望(r16, r18);
  r19 = cn_var_节点_0;
  return r19;
}

void* 解析情况列表(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r7, r10, r14, r16, r17, r18, r21, r22, r25, r26, r29, r30, r33, r40, r43, r47, r48, r49, r52, r53, r56, r57, r60, r61, r64;
  struct 解析器* r0;
  void* r2;
  struct 解析器* r4;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r12;
  struct 情况分支* r15;
  struct 解析器* r19;
  struct 解析器* r23;
  struct 解析器* r27;
  struct 解析器* r31;
  struct 语句节点* r32;
  struct 情况分支* r34;
  struct 语句节点* r35;
  void* r36;
  struct 情况分支* r37;
  struct 解析器* r38;
  struct 解析器* r41;
  struct 情况分支* r42;
  struct 解析器* r44;
  struct 解析器* r45;
  struct 解析器* r50;
  struct 解析器* r54;
  struct 解析器* r58;
  struct 解析器* r62;
  struct 语句节点* r63;
  struct 情况分支* r65;
  struct 语句节点* r66;
  void* r67;
  struct 情况分支* r68;
  void* r69;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r39;
  enum 词元类型枚举 r46;
  enum 词元类型枚举 r51;
  enum 词元类型枚举 r55;
  enum 词元类型枚举 r59;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1382; else goto if_merge_1383;

  if_then_1382:
  return 0;
  goto if_merge_1383;

  if_merge_1383:
  void* cn_var_数组_0;
  创建数组(4);
  cn_var_数组_0 = /* NONE */;
  r2 = cn_var_数组_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1384; else goto if_merge_1385;

  if_then_1384:
  return 0;
  goto if_merge_1385;

  if_merge_1385:
  goto while_cond_1386;

  while_cond_1386:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.右大括号;
  检查(r4, r6);
  r7 = !/* NONE */;
  if (r7) goto while_body_1387; else goto while_exit_1388;

  while_body_1387:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.关键字_默认;
  检查(r8, r10);
  goto if_then_1389;

  while_exit_1388:
  r69 = cn_var_数组_0;
  return r69;

  if_then_1389:
  r11 = cn_var_实例;
  前进词元(r11);
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.冒号;
  期望(r12, r14);
  struct 情况分支* cn_var_默认分支_1;
  创建情况分支();
  cn_var_默认分支_1 = /* NONE */;
  r15 = cn_var_默认分支_1;
  r16 = r15 != 0;
  if (r16) goto if_then_1392; else goto if_merge_1393;

  if_else_1390:
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.关键字_情况;
  检查(r38, r40);
  goto if_then_1403;

  if_merge_1391:
  goto while_cond_1386;

  if_then_1392:
  goto while_cond_1394;

  if_merge_1393:
  goto if_merge_1391;

  while_cond_1394:
  r19 = cn_var_实例;
  r20 = cn_var_词元类型枚举;
  r21 = r20.右大括号;
  检查(r19, r21);
  r22 = !/* NONE */;
  if (r22) goto logic_rhs_1399; else goto logic_merge_1400;

  while_body_1395:
  struct 语句节点* cn_var_语句_2;
  r31 = cn_var_实例;
  解析语句(r31);
  cn_var_语句_2 = /* NONE */;
  r32 = cn_var_语句_2;
  r33 = r32 != 0;
  if (r33) goto if_then_1401; else goto if_merge_1402;

  while_exit_1396:
  r36 = cn_var_数组_0;
  r37 = cn_var_默认分支_1;
  数组添加(r36, r37);
  goto if_merge_1393;

  logic_rhs_1397:
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.关键字_默认;
  检查(r27, r29);
  r30 = !/* NONE */;
  goto logic_merge_1398;

  logic_merge_1398:
  if (r30) goto while_body_1395; else goto while_exit_1396;

  logic_rhs_1399:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.关键字_情况;
  检查(r23, r25);
  r26 = !/* NONE */;
  goto logic_merge_1400;

  logic_merge_1400:
  if (r26) goto logic_rhs_1397; else goto logic_merge_1398;

  if_then_1401:
  r34 = cn_var_默认分支_1;
  r35 = cn_var_语句_2;
  情况分支添加语句(r34, r35);
  goto if_merge_1402;

  if_merge_1402:
  goto while_cond_1394;

  if_then_1403:
  r41 = cn_var_实例;
  前进词元(r41);
  struct 情况分支* cn_var_当前情况_3;
  创建情况分支();
  cn_var_当前情况_3 = /* NONE */;
  r42 = cn_var_当前情况_3;
  r43 = r42 != 0;
  if (r43) goto if_then_1406; else goto if_merge_1407;

  if_else_1404:
  goto while_exit_1388;
  goto if_merge_1405;

  if_merge_1405:
  goto if_merge_1391;

  if_then_1406:
  r44 = cn_var_实例;
  解析表达式(r44);
  r45 = cn_var_实例;
  r46 = cn_var_词元类型枚举;
  r47 = r46.冒号;
  期望(r45, r47);
  goto while_cond_1408;

  if_merge_1407:
  goto if_merge_1405;

  while_cond_1408:
  r50 = cn_var_实例;
  r51 = cn_var_词元类型枚举;
  r52 = r51.右大括号;
  检查(r50, r52);
  r53 = !/* NONE */;
  if (r53) goto logic_rhs_1413; else goto logic_merge_1414;

  while_body_1409:
  struct 语句节点* cn_var_语句_4;
  r62 = cn_var_实例;
  解析语句(r62);
  cn_var_语句_4 = /* NONE */;
  r63 = cn_var_语句_4;
  r64 = r63 != 0;
  if (r64) goto if_then_1415; else goto if_merge_1416;

  while_exit_1410:
  r67 = cn_var_数组_0;
  r68 = cn_var_当前情况_3;
  数组添加(r67, r68);
  goto if_merge_1407;

  logic_rhs_1411:
  r58 = cn_var_实例;
  r59 = cn_var_词元类型枚举;
  r60 = r59.关键字_默认;
  检查(r58, r60);
  r61 = !/* NONE */;
  goto logic_merge_1412;

  logic_merge_1412:
  if (r61) goto while_body_1409; else goto while_exit_1410;

  logic_rhs_1413:
  r54 = cn_var_实例;
  r55 = cn_var_词元类型枚举;
  r56 = r55.关键字_情况;
  检查(r54, r56);
  r57 = !/* NONE */;
  goto logic_merge_1414;

  logic_merge_1414:
  if (r57) goto logic_rhs_1411; else goto logic_merge_1412;

  if_then_1415:
  r65 = cn_var_当前情况_3;
  r66 = cn_var_语句_4;
  情况分支添加语句(r65, r66);
  goto if_merge_1416;

  if_merge_1416:
  goto while_cond_1408;
  return NULL;
}

struct 语句节点* 解析尝试语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r8, r12, r19, r22, r26;
  char* r16;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r6;
  struct 解析器* r9;
  struct 解析器* r10;
  struct 解析器* r13;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r24;
  struct 解析器* r27;
  struct 解析器* r28;
  struct 语句节点* r29;
  struct 词元 r15;
  enum 词元类型枚举 r7;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r25;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1417; else goto if_merge_1418;

  if_then_1417:
  return 0;
  goto if_merge_1418;

  if_merge_1418:
  struct 语句节点* cn_var_节点_0;
  创建尝试语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1419; else goto if_merge_1420;

  if_then_1419:
  return 0;
  goto if_merge_1420;

  if_merge_1420:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  解析块语句(r5);
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.关键字_捕获;
  检查(r6, r8);
  goto if_then_1421;

  if_then_1421:
  r9 = cn_var_实例;
  前进词元(r9);
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.左括号;
  期望(r10, r12);
  r13 = cn_var_实例;
  解析类型(r13);
  r14 = cn_var_实例;
  r15 = r14->当前词元;
  r16 = r15.值;
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.标识符;
  期望(r17, r19);
  r20 = cn_var_实例;
  r21 = cn_var_词元类型枚举;
  r22 = r21.右括号;
  期望(r20, r22);
  r23 = cn_var_实例;
  解析块语句(r23);
  goto if_merge_1422;

  if_merge_1422:
  r24 = cn_var_实例;
  r25 = cn_var_词元类型枚举;
  r26 = r25.关键字_最终;
  检查(r24, r26);
  goto if_then_1423;

  if_then_1423:
  r27 = cn_var_实例;
  前进词元(r27);
  r28 = cn_var_实例;
  解析块语句(r28);
  goto if_merge_1424;

  if_merge_1424:
  r29 = cn_var_节点_0;
  return r29;
}

struct 语句节点* 解析抛出语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r20;
  char* r7;
  char* r16;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r18;
  struct 语句节点* r21;
  struct 词元 r6;
  struct 词元 r15;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r19;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1425; else goto if_merge_1426;

  if_then_1425:
  return 0;
  goto if_merge_1426;

  if_merge_1426:
  struct 语句节点* cn_var_节点_0;
  创建抛出语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1427; else goto if_merge_1428;

  if_then_1427:
  return 0;
  goto if_merge_1428;

  if_merge_1428:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.字符串字面量;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.字符串字面量;
  检查(r11, r13);
  goto if_then_1429;

  if_then_1429:
  r14 = cn_var_实例;
  r15 = r14->当前词元;
  r16 = r15.值;
  r17 = cn_var_实例;
  前进词元(r17);
  goto if_merge_1430;

  if_merge_1430:
  r18 = cn_var_实例;
  r19 = cn_var_词元类型枚举;
  r20 = r19.分号;
  期望(r18, r20);
  r21 = cn_var_节点_0;
  return r21;
}

struct 语句节点* 解析表达式语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r8;
  enum 词元类型枚举 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1431; else goto if_merge_1432;

  if_then_1431:
  return 0;
  goto if_merge_1432;

  if_merge_1432:
  struct 语句节点* cn_var_节点_0;
  创建表达式语句();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1433; else goto if_merge_1434;

  if_then_1433:
  return 0;
  goto if_merge_1434;

  if_merge_1434:
  r4 = cn_var_实例;
  解析表达式(r4);
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.分号;
  期望(r5, r7);
  r8 = cn_var_节点_0;
  return r8;
}

struct 语句节点* 解析变量声明语句(struct 解析器* cn_var_实例) {
  long long r1, r4, r6;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 声明节点* r3;
  struct 语句节点* r5;
  struct 声明节点* r7;
  struct 语句节点* r8;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1435; else goto if_merge_1436;

  if_then_1435:
  return 0;
  goto if_merge_1436;

  if_merge_1436:
  struct 声明节点* cn_var_声明_0;
  r2 = cn_var_实例;
  解析变量声明(r2);
  cn_var_声明_0 = /* NONE */;
  r3 = cn_var_声明_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1437; else goto if_merge_1438;

  if_then_1437:
  return 0;
  goto if_merge_1438;

  if_merge_1438:
  struct 语句节点* cn_var_节点_1;
  创建声明语句();
  cn_var_节点_1 = /* NONE */;
  r5 = cn_var_节点_1;
  r6 = r5 == 0;
  if (r6) goto if_then_1439; else goto if_merge_1440;

  if_then_1439:
  return 0;
  goto if_merge_1440;

  if_merge_1440:
  r7 = cn_var_声明_0;
  r8 = cn_var_节点_1;
  return r8;
}

