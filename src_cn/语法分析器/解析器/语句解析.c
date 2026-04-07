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
  struct 块语句节点* r45;
  struct 解析器* r46;
  struct 语句节点* r47;
  struct 解析器* r48;
  struct 语句节点* r49;
  struct 解析器* r50;
  struct 语句节点* r51;
  struct 解析器* r52;
  struct 语句节点* r53;
  struct 解析器* r54;
  struct 语句节点* r55;
  struct 解析器* r56;
  struct 语句节点* r57;
  struct 解析器* r58;
  struct 语句节点* r59;
  struct 解析器* r60;
  struct 语句节点* r61;
  struct 解析器* r62;
  struct 语句节点* r63;
  struct 解析器* r64;
  struct 语句节点* r65;
  struct 解析器* r66;
  struct 解析器* r70;
  struct 语句节点* r71;
  struct 解析器* r72;
  struct 语句节点* r73;
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
  _Bool r69;
  struct 词元 r3;
  struct 词元 r67;
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
  enum 词元类型枚举 r68;

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
  r45 = 解析块语句(r44);
  return r45;
  goto switch_merge_1295;

  case_body_1297:
  r46 = cn_var_实例;
  r47 = 解析如果语句(r46);
  return r47;
  goto switch_merge_1295;

  case_body_1298:
  r48 = cn_var_实例;
  r49 = 解析当语句(r48);
  return r49;
  goto switch_merge_1295;

  case_body_1299:
  r50 = cn_var_实例;
  r51 = 解析循环语句(r50);
  return r51;
  goto switch_merge_1295;

  case_body_1300:
  r52 = cn_var_实例;
  r53 = 解析返回语句(r52);
  return r53;
  goto switch_merge_1295;

  case_body_1301:
  r54 = cn_var_实例;
  r55 = 解析中断语句(r54);
  return r55;
  goto switch_merge_1295;

  case_body_1302:
  r56 = cn_var_实例;
  r57 = 解析继续语句(r56);
  return r57;
  goto switch_merge_1295;

  case_body_1303:
  r58 = cn_var_实例;
  r59 = 解析选择语句(r58);
  return r59;
  goto switch_merge_1295;

  case_body_1304:
  r60 = cn_var_实例;
  r61 = 解析尝试语句(r60);
  return r61;
  goto switch_merge_1295;

  case_body_1305:
  r62 = cn_var_实例;
  r63 = 解析抛出语句(r62);
  return r63;
  goto switch_merge_1295;

  case_body_1306:
  goto switch_merge_1295;

  case_body_1307:
  goto switch_merge_1295;

  case_body_1308:
  r64 = cn_var_实例;
  r65 = 解析变量声明语句(r64);
  return r65;
  goto switch_merge_1295;

  case_default_1309:
  r66 = cn_var_实例;
  r67 = r66->当前词元;
  r68 = r67.类型;
  r69 = 是否类型关键字(r68);
  if (r69) goto if_then_1322; else goto if_merge_1323;

  if_then_1322:
  r70 = cn_var_实例;
  r71 = 解析变量声明语句(r70);
  return r71;
  goto if_merge_1323;

  if_merge_1323:
  r72 = cn_var_实例;
  r73 = 解析表达式语句(r72);
  return r73;
  goto switch_merge_1295;

  switch_merge_1295:
  return NULL;
}

struct 块语句节点* 解析块语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r8, r11, r13, r16, r18, r22, r27;
  struct 解析器* r0;
  struct 块语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r9;
  struct 解析器* r14;
  struct 解析器* r19;
  struct 语句节点* r20;
  struct 语句节点* r21;
  struct 块语句节点* r23;
  struct 语句节点* r24;
  struct 解析器* r25;
  struct 块语句节点* r29;
  _Bool r7;
  _Bool r12;
  _Bool r17;
  _Bool r28;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r26;

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
  r7 = 期望(r4, r6);
  goto while_cond_1328;

  while_cond_1328:
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.右大括号;
  r12 = 检查(r9, r11);
  r13 = !r12;
  if (r13) goto logic_rhs_1331; else goto logic_merge_1332;

  while_body_1329:
  struct 语句节点* cn_var_语句_1;
  r19 = cn_var_实例;
  r20 = 解析语句(r19);
  cn_var_语句_1 = r20;
  r21 = cn_var_语句_1;
  r22 = r21 != 0;
  if (r22) goto if_then_1333; else goto if_merge_1334;

  while_exit_1330:
  r25 = cn_var_实例;
  r26 = cn_var_词元类型枚举;
  r27 = r26.右大括号;
  r28 = 期望(r25, r27);
  r29 = cn_var_块节点_0;
  return r29;

  logic_rhs_1331:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.结束;
  r17 = 检查(r14, r16);
  r18 = !r17;
  goto logic_merge_1332;

  logic_merge_1332:
  if (r18) goto while_body_1329; else goto while_exit_1330;

  if_then_1333:
  r23 = cn_var_块节点_0;
  r24 = cn_var_语句_1;
  块语句添加(r23, r24);
  goto if_merge_1334;

  if_merge_1334:
  goto while_cond_1328;
  return NULL;
}

struct 语句节点* 解析如果语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r13, r19, r24;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 解析器* r11;
  struct 解析器* r15;
  struct 块语句节点* r16;
  struct 解析器* r17;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 解析器* r26;
  struct 语句节点* r27;
  struct 解析器* r28;
  struct 块语句节点* r29;
  struct 语句节点* r30;
  _Bool r8;
  _Bool r14;
  _Bool r20;
  _Bool r25;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r23;

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
  r8 = 期望(r5, r7);
  r9 = cn_var_实例;
  r10 = 解析表达式(r9);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.右括号;
  r14 = 期望(r11, r13);
  r15 = cn_var_实例;
  r16 = 解析块语句(r15);
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.关键字_否则;
  r20 = 检查(r17, r19);
  if (r20) goto if_then_1339; else goto if_merge_1340;

  if_then_1339:
  r21 = cn_var_实例;
  前进词元(r21);
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.关键字_如果;
  r25 = 检查(r22, r24);
  if (r25) goto if_then_1341; else goto if_else_1342;

  if_merge_1340:
  r30 = cn_var_节点_0;
  return r30;

  if_then_1341:
  r26 = cn_var_实例;
  r27 = 解析如果语句(r26);
  goto if_merge_1343;

  if_else_1342:
  r28 = cn_var_实例;
  r29 = 解析块语句(r28);
  goto if_merge_1343;

  if_merge_1343:
  goto if_merge_1340;
  return NULL;
}

struct 语句节点* 解析当语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r13;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 解析器* r11;
  struct 解析器* r15;
  struct 块语句节点* r16;
  struct 语句节点* r17;
  _Bool r8;
  _Bool r14;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r12;

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
  r8 = 期望(r5, r7);
  r9 = cn_var_实例;
  r10 = 解析表达式(r9);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.右括号;
  r14 = 期望(r11, r13);
  r15 = cn_var_实例;
  r16 = 解析块语句(r15);
  r17 = cn_var_节点_0;
  return r17;
}

struct 语句节点* 解析循环语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r11, r13, r14, r21, r30, r32, r37, r41, r43, r48;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 解析器* r15;
  struct 解析器* r19;
  struct 解析器* r23;
  struct 语句节点* r24;
  struct 解析器* r25;
  struct 语句节点* r26;
  struct 解析器* r27;
  struct 解析器* r28;
  struct 解析器* r33;
  struct 表达式节点* r34;
  struct 解析器* r35;
  struct 解析器* r39;
  struct 解析器* r44;
  struct 表达式节点* r45;
  struct 解析器* r46;
  struct 解析器* r50;
  struct 块语句节点* r51;
  struct 语句节点* r52;
  _Bool r8;
  _Bool r12;
  _Bool r18;
  _Bool r22;
  _Bool r31;
  _Bool r38;
  _Bool r42;
  _Bool r49;
  struct 词元 r16;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r36;
  enum 词元类型枚举 r40;
  enum 词元类型枚举 r47;

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
  r8 = 期望(r5, r7);
  r9 = cn_var_实例;
  r10 = cn_var_词元类型枚举;
  r11 = r10.分号;
  r12 = 检查(r9, r11);
  r13 = !r12;
  if (r13) goto if_then_1352; else goto if_else_1353;

  if_then_1352:
  r15 = cn_var_实例;
  r16 = r15->当前词元;
  r17 = r16.类型;
  r18 = 是否类型关键字(r17);
  if (r18) goto logic_merge_1359; else goto logic_rhs_1358;

  if_else_1353:
  r27 = cn_var_实例;
  前进词元(r27);
  goto if_merge_1354;

  if_merge_1354:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.分号;
  r31 = 检查(r28, r30);
  r32 = !r31;
  if (r32) goto if_then_1360; else goto if_merge_1361;

  if_then_1355:
  r23 = cn_var_实例;
  r24 = 解析变量声明语句(r23);
  goto if_merge_1357;

  if_else_1356:
  r25 = cn_var_实例;
  r26 = 解析表达式语句(r25);
  goto if_merge_1357;

  if_merge_1357:
  goto if_merge_1354;

  logic_rhs_1358:
  r19 = cn_var_实例;
  r20 = cn_var_词元类型枚举;
  r21 = r20.关键字_变量;
  r22 = 检查(r19, r21);
  goto logic_merge_1359;

  logic_merge_1359:
  if (r22) goto if_then_1355; else goto if_else_1356;

  if_then_1360:
  r33 = cn_var_实例;
  r34 = 解析表达式(r33);
  goto if_merge_1361;

  if_merge_1361:
  r35 = cn_var_实例;
  r36 = cn_var_词元类型枚举;
  r37 = r36.分号;
  r38 = 期望(r35, r37);
  r39 = cn_var_实例;
  r40 = cn_var_词元类型枚举;
  r41 = r40.右括号;
  r42 = 检查(r39, r41);
  r43 = !r42;
  if (r43) goto if_then_1362; else goto if_merge_1363;

  if_then_1362:
  r44 = cn_var_实例;
  r45 = 解析表达式(r44);
  goto if_merge_1363;

  if_merge_1363:
  r46 = cn_var_实例;
  r47 = cn_var_词元类型枚举;
  r48 = r47.右括号;
  r49 = 期望(r46, r48);
  r50 = cn_var_实例;
  r51 = 解析块语句(r50);
  r52 = cn_var_节点_0;
  return r52;
}

struct 语句节点* 解析返回语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r9, r14;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r10;
  struct 表达式节点* r11;
  struct 解析器* r12;
  struct 语句节点* r16;
  _Bool r8;
  _Bool r15;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r13;

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
  r8 = 检查(r5, r7);
  r9 = !r8;
  if (r9) goto if_then_1368; else goto if_merge_1369;

  if_then_1368:
  r10 = cn_var_实例;
  r11 = 解析表达式(r10);
  goto if_merge_1369;

  if_merge_1369:
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.分号;
  r15 = 期望(r12, r14);
  r16 = cn_var_节点_0;
  return r16;
}

struct 语句节点* 解析中断语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r9;
  _Bool r8;
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
  r8 = 期望(r5, r7);
  r9 = cn_var_节点_0;
  return r9;
}

struct 语句节点* 解析继续语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 语句节点* r9;
  _Bool r8;
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
  r8 = 期望(r5, r7);
  r9 = cn_var_节点_0;
  return r9;
}

struct 语句节点* 解析选择语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r7, r13, r17, r23;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r9;
  struct 表达式节点* r10;
  struct 解析器* r11;
  struct 解析器* r15;
  struct 解析器* r19;
  void* r20;
  struct 解析器* r21;
  struct 语句节点* r25;
  _Bool r8;
  _Bool r14;
  _Bool r18;
  _Bool r24;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r22;

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
  r8 = 期望(r5, r7);
  r9 = cn_var_实例;
  r10 = 解析表达式(r9);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.右括号;
  r14 = 期望(r11, r13);
  r15 = cn_var_实例;
  r16 = cn_var_词元类型枚举;
  r17 = r16.左大括号;
  r18 = 期望(r15, r17);
  r19 = cn_var_实例;
  r20 = 解析情况列表(r19);
  r21 = cn_var_实例;
  r22 = cn_var_词元类型枚举;
  r23 = r22.右大括号;
  r24 = 期望(r21, r23);
  r25 = cn_var_节点_0;
  return r25;
}

void* 解析情况列表(struct 解析器* cn_var_实例) {
  long long r1, r4, r7, r9, r12, r17, r20, r21, r22, r25, r27, r30, r32, r35, r37, r41, r46, r49, r53, r58, r60, r61, r64, r66, r69, r71, r74, r76, r80, r85;
  struct 解析器* r0;
  void* r2;
  void* r3;
  struct 解析器* r5;
  struct 解析器* r10;
  struct 解析器* r14;
  struct 解析器* r15;
  struct 情况分支* r19;
  struct 解析器* r23;
  struct 解析器* r28;
  struct 解析器* r33;
  struct 解析器* r38;
  struct 语句节点* r39;
  struct 语句节点* r40;
  struct 情况分支* r42;
  struct 语句节点* r43;
  void* r44;
  struct 情况分支* r45;
  struct 解析器* r47;
  struct 解析器* r51;
  struct 情况分支* r52;
  struct 解析器* r54;
  struct 表达式节点* r55;
  struct 解析器* r56;
  struct 解析器* r62;
  struct 解析器* r67;
  struct 解析器* r72;
  struct 解析器* r77;
  struct 语句节点* r78;
  struct 语句节点* r79;
  struct 情况分支* r81;
  struct 语句节点* r82;
  void* r83;
  struct 情况分支* r84;
  void* r86;
  _Bool r8;
  _Bool r13;
  _Bool r18;
  _Bool r26;
  _Bool r31;
  _Bool r36;
  _Bool r50;
  _Bool r59;
  _Bool r65;
  _Bool r70;
  _Bool r75;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r34;
  enum 词元类型枚举 r48;
  enum 词元类型枚举 r57;
  enum 词元类型枚举 r63;
  enum 词元类型枚举 r68;
  enum 词元类型枚举 r73;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1382; else goto if_merge_1383;

  if_then_1382:
  return 0;
  goto if_merge_1383;

  if_merge_1383:
  void* cn_var_数组_0;
  r2 = 创建数组(4);
  cn_var_数组_0 = r2;
  r3 = cn_var_数组_0;
  r4 = r3 == 0;
  if (r4) goto if_then_1384; else goto if_merge_1385;

  if_then_1384:
  return 0;
  goto if_merge_1385;

  if_merge_1385:
  goto while_cond_1386;

  while_cond_1386:
  r5 = cn_var_实例;
  r6 = cn_var_词元类型枚举;
  r7 = r6.右大括号;
  r8 = 检查(r5, r7);
  r9 = !r8;
  if (r9) goto while_body_1387; else goto while_exit_1388;

  while_body_1387:
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.关键字_默认;
  r13 = 检查(r10, r12);
  if (r13) goto if_then_1389; else goto if_else_1390;

  while_exit_1388:
  r86 = cn_var_数组_0;
  return r86;

  if_then_1389:
  r14 = cn_var_实例;
  前进词元(r14);
  r15 = cn_var_实例;
  r16 = cn_var_词元类型枚举;
  r17 = r16.冒号;
  r18 = 期望(r15, r17);
  struct 情况分支* cn_var_默认分支_1;
  创建情况分支();
  cn_var_默认分支_1 = /* NONE */;
  r19 = cn_var_默认分支_1;
  r20 = r19 != 0;
  if (r20) goto if_then_1392; else goto if_merge_1393;

  if_else_1390:
  r47 = cn_var_实例;
  r48 = cn_var_词元类型枚举;
  r49 = r48.关键字_情况;
  r50 = 检查(r47, r49);
  if (r50) goto if_then_1403; else goto if_else_1404;

  if_merge_1391:
  goto while_cond_1386;

  if_then_1392:
  goto while_cond_1394;

  if_merge_1393:
  goto if_merge_1391;

  while_cond_1394:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.右大括号;
  r26 = 检查(r23, r25);
  r27 = !r26;
  if (r27) goto logic_rhs_1399; else goto logic_merge_1400;

  while_body_1395:
  struct 语句节点* cn_var_语句_2;
  r38 = cn_var_实例;
  r39 = 解析语句(r38);
  cn_var_语句_2 = r39;
  r40 = cn_var_语句_2;
  r41 = r40 != 0;
  if (r41) goto if_then_1401; else goto if_merge_1402;

  while_exit_1396:
  r44 = cn_var_数组_0;
  r45 = cn_var_默认分支_1;
  r46 = 数组添加(r44, r45);
  goto if_merge_1393;

  logic_rhs_1397:
  r33 = cn_var_实例;
  r34 = cn_var_词元类型枚举;
  r35 = r34.关键字_默认;
  r36 = 检查(r33, r35);
  r37 = !r36;
  goto logic_merge_1398;

  logic_merge_1398:
  if (r37) goto while_body_1395; else goto while_exit_1396;

  logic_rhs_1399:
  r28 = cn_var_实例;
  r29 = cn_var_词元类型枚举;
  r30 = r29.关键字_情况;
  r31 = 检查(r28, r30);
  r32 = !r31;
  goto logic_merge_1400;

  logic_merge_1400:
  if (r32) goto logic_rhs_1397; else goto logic_merge_1398;

  if_then_1401:
  r42 = cn_var_默认分支_1;
  r43 = cn_var_语句_2;
  情况分支添加语句(r42, r43);
  goto if_merge_1402;

  if_merge_1402:
  goto while_cond_1394;

  if_then_1403:
  r51 = cn_var_实例;
  前进词元(r51);
  struct 情况分支* cn_var_当前情况_3;
  创建情况分支();
  cn_var_当前情况_3 = /* NONE */;
  r52 = cn_var_当前情况_3;
  r53 = r52 != 0;
  if (r53) goto if_then_1406; else goto if_merge_1407;

  if_else_1404:
  goto while_exit_1388;
  goto if_merge_1405;

  if_merge_1405:
  goto if_merge_1391;

  if_then_1406:
  r54 = cn_var_实例;
  r55 = 解析表达式(r54);
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.冒号;
  r59 = 期望(r56, r58);
  goto while_cond_1408;

  if_merge_1407:
  goto if_merge_1405;

  while_cond_1408:
  r62 = cn_var_实例;
  r63 = cn_var_词元类型枚举;
  r64 = r63.右大括号;
  r65 = 检查(r62, r64);
  r66 = !r65;
  if (r66) goto logic_rhs_1413; else goto logic_merge_1414;

  while_body_1409:
  struct 语句节点* cn_var_语句_4;
  r77 = cn_var_实例;
  r78 = 解析语句(r77);
  cn_var_语句_4 = r78;
  r79 = cn_var_语句_4;
  r80 = r79 != 0;
  if (r80) goto if_then_1415; else goto if_merge_1416;

  while_exit_1410:
  r83 = cn_var_数组_0;
  r84 = cn_var_当前情况_3;
  r85 = 数组添加(r83, r84);
  goto if_merge_1407;

  logic_rhs_1411:
  r72 = cn_var_实例;
  r73 = cn_var_词元类型枚举;
  r74 = r73.关键字_默认;
  r75 = 检查(r72, r74);
  r76 = !r75;
  goto logic_merge_1412;

  logic_merge_1412:
  if (r76) goto while_body_1409; else goto while_exit_1410;

  logic_rhs_1413:
  r67 = cn_var_实例;
  r68 = cn_var_词元类型枚举;
  r69 = r68.关键字_情况;
  r70 = 检查(r67, r69);
  r71 = !r70;
  goto logic_merge_1414;

  logic_merge_1414:
  if (r71) goto logic_rhs_1411; else goto logic_merge_1412;

  if_then_1415:
  r81 = cn_var_当前情况_3;
  r82 = cn_var_语句_4;
  情况分支添加语句(r81, r82);
  goto if_merge_1416;

  if_merge_1416:
  goto while_cond_1408;
  return NULL;
}

struct 语句节点* 解析尝试语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r9, r14, r23, r27, r33;
  char* r20;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 块语句节点* r6;
  struct 解析器* r7;
  struct 解析器* r11;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 类型节点* r17;
  struct 解析器* r18;
  struct 解析器* r21;
  struct 解析器* r25;
  struct 解析器* r29;
  struct 块语句节点* r30;
  struct 解析器* r31;
  struct 解析器* r35;
  struct 解析器* r36;
  struct 块语句节点* r37;
  struct 语句节点* r38;
  _Bool r10;
  _Bool r15;
  _Bool r24;
  _Bool r28;
  _Bool r34;
  struct 词元 r19;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r22;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r32;

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
  r6 = 解析块语句(r5);
  r7 = cn_var_实例;
  r8 = cn_var_词元类型枚举;
  r9 = r8.关键字_捕获;
  r10 = 检查(r7, r9);
  if (r10) goto if_then_1421; else goto if_merge_1422;

  if_then_1421:
  r11 = cn_var_实例;
  前进词元(r11);
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.左括号;
  r15 = 期望(r12, r14);
  r16 = cn_var_实例;
  r17 = 解析类型(r16);
  r18 = cn_var_实例;
  r19 = r18->当前词元;
  r20 = r19.值;
  r21 = cn_var_实例;
  r22 = cn_var_词元类型枚举;
  r23 = r22.标识符;
  r24 = 期望(r21, r23);
  r25 = cn_var_实例;
  r26 = cn_var_词元类型枚举;
  r27 = r26.右括号;
  r28 = 期望(r25, r27);
  r29 = cn_var_实例;
  r30 = 解析块语句(r29);
  goto if_merge_1422;

  if_merge_1422:
  r31 = cn_var_实例;
  r32 = cn_var_词元类型枚举;
  r33 = r32.关键字_最终;
  r34 = 检查(r31, r33);
  if (r34) goto if_then_1423; else goto if_merge_1424;

  if_then_1423:
  r35 = cn_var_实例;
  前进词元(r35);
  r36 = cn_var_实例;
  r37 = 解析块语句(r36);
  goto if_merge_1424;

  if_merge_1424:
  r38 = cn_var_节点_0;
  return r38;
}

struct 语句节点* 解析抛出语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r14, r22;
  char* r7;
  char* r18;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r12;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 语句节点* r24;
  _Bool r11;
  _Bool r15;
  _Bool r23;
  struct 词元 r6;
  struct 词元 r17;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r21;

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
  r11 = 期望(r8, r10);
  r12 = cn_var_实例;
  r13 = cn_var_词元类型枚举;
  r14 = r13.字符串字面量;
  r15 = 检查(r12, r14);
  if (r15) goto if_then_1429; else goto if_merge_1430;

  if_then_1429:
  r16 = cn_var_实例;
  r17 = r16->当前词元;
  r18 = r17.值;
  r19 = cn_var_实例;
  前进词元(r19);
  goto if_merge_1430;

  if_merge_1430:
  r20 = cn_var_实例;
  r21 = cn_var_词元类型枚举;
  r22 = r21.分号;
  r23 = 期望(r20, r22);
  r24 = cn_var_节点_0;
  return r24;
}

struct 语句节点* 解析表达式语句(struct 解析器* cn_var_实例) {
  long long r1, r3, r8;
  struct 解析器* r0;
  struct 语句节点* r2;
  struct 解析器* r4;
  struct 表达式节点* r5;
  struct 解析器* r6;
  struct 语句节点* r10;
  _Bool r9;
  enum 词元类型枚举 r7;

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
  r5 = 解析表达式(r4);
  r6 = cn_var_实例;
  r7 = cn_var_词元类型枚举;
  r8 = r7.分号;
  r9 = 期望(r6, r8);
  r10 = cn_var_节点_0;
  return r10;
}

struct 语句节点* 解析变量声明语句(struct 解析器* cn_var_实例) {
  long long r1, r5, r7;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 声明节点* r3;
  struct 声明节点* r4;
  struct 语句节点* r6;
  struct 声明节点* r8;
  struct 语句节点* r9;

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
  r3 = 解析变量声明(r2);
  cn_var_声明_0 = r3;
  r4 = cn_var_声明_0;
  r5 = r4 == 0;
  if (r5) goto if_then_1437; else goto if_merge_1438;

  if_then_1437:
  return 0;
  goto if_merge_1438;

  if_merge_1438:
  struct 语句节点* cn_var_节点_1;
  创建声明语句();
  cn_var_节点_1 = /* NONE */;
  r6 = cn_var_节点_1;
  r7 = r6 == 0;
  if (r7) goto if_then_1439; else goto if_merge_1440;

  if_then_1439:
  return 0;
  goto if_merge_1440;

  if_merge_1440:
  r8 = cn_var_声明_0;
  r9 = cn_var_节点_1;
  return r9;
}

