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

// Global Variables

// Forward Declarations
struct 声明节点* 解析顶层声明(struct 解析器*);
struct 声明节点* 解析函数声明(struct 解析器*);
struct 参数列表* 解析参数列表(struct 解析器*);
struct 参数节点* 解析参数(struct 解析器*);
struct 声明节点* 解析变量声明(struct 解析器*);
struct 声明节点* 解析结构体声明(struct 解析器*);
struct 声明节点* 解析枚举声明(struct 解析器*);
struct 声明节点* 解析类声明(struct 解析器*);
struct 声明节点* 解析接口声明(struct 解析器*);
struct 声明节点* 解析模块声明(struct 解析器*);
struct 声明节点* 解析导入声明(struct 解析器*);
enum 可见性 解析可见性修饰符(struct 解析器*);
struct 声明节点* 解析顶层声明(struct 解析器*);
struct 声明节点* 解析函数声明(struct 解析器*);
struct 参数列表* 解析参数列表(struct 解析器*);
struct 参数节点* 解析参数(struct 解析器*);
struct 声明节点* 解析变量声明(struct 解析器*);
struct 声明节点* 解析结构体声明(struct 解析器*);
struct 声明节点* 解析枚举声明(struct 解析器*);
struct 声明节点* 解析类声明(struct 解析器*);
struct 声明节点* 解析接口声明(struct 解析器*);
struct 声明节点* 解析模块声明(struct 解析器*);
struct 声明节点* 解析导入声明(struct 解析器*);
enum 可见性 解析可见性修饰符(struct 解析器*);

struct 声明节点* 解析顶层声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析函数声明(struct 解析器* cn_var_实例);
struct 参数列表* 解析参数列表(struct 解析器* cn_var_实例);
struct 参数节点* 解析参数(struct 解析器* cn_var_实例);
struct 声明节点* 解析变量声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析结构体声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析枚举声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析类声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析接口声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析模块声明(struct 解析器* cn_var_实例);
struct 声明节点* 解析导入声明(struct 解析器* cn_var_实例);
enum 可见性 解析可见性修饰符(struct 解析器* cn_var_实例);
struct 声明节点* 解析顶层声明(struct 解析器* cn_var_实例) {
  long long r1, r7, r10, r13, r16, r19, r21, r24, r27, r30, r33, r49, r53;
  char* r52;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r3;
  struct 解析器* r35;
  struct 解析器* r36;
  struct 解析器* r37;
  struct 解析器* r38;
  struct 解析器* r39;
  struct 解析器* r40;
  struct 解析器* r41;
  struct 解析器* r42;
  struct 解析器* r43;
  struct 解析器* r46;
  struct 解析器* r47;
  struct 解析器* r50;
  struct 解析器* r54;
  _Bool r8;
  _Bool r11;
  _Bool r14;
  _Bool r17;
  _Bool r20;
  _Bool r22;
  _Bool r25;
  _Bool r28;
  _Bool r31;
  _Bool r34;
  struct 词元 r4;
  struct 词元 r44;
  struct 词元 r51;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r29;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r45;
  enum 诊断错误码 r48;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1441; else goto if_merge_1442;

  if_then_1441:
  return 0;
  goto if_merge_1442;

  if_merge_1442:
  struct 可见性 cn_var_当前可见性_0;
  r2 = cn_var_实例;
  解析可见性修饰符(r2);
  cn_var_当前可见性_0 = /* NONE */;
  r3 = cn_var_实例;
  r4 = r3->当前词元;
  r5 = r4.类型;
  r6 = cn_var_词元类型枚举;
  r7 = r6.关键字_函数;
  r8 = r5 == r7;
  if (r8) goto case_body_1444; else goto switch_check_1455;

  switch_check_1455:
  r9 = cn_var_词元类型枚举;
  r10 = r9.关键字_结构体;
  r11 = r5 == r10;
  if (r11) goto case_body_1445; else goto switch_check_1456;

  switch_check_1456:
  r12 = cn_var_词元类型枚举;
  r13 = r12.关键字_枚举;
  r14 = r5 == r13;
  if (r14) goto case_body_1446; else goto switch_check_1457;

  switch_check_1457:
  r15 = cn_var_词元类型枚举;
  r16 = r15.关键字_类;
  r17 = r5 == r16;
  if (r17) goto case_body_1447; else goto switch_check_1458;

  switch_check_1458:
  r18 = cn_var_词元类型枚举;
  r19 = r18.关键字_接口;
  r20 = r5 == r19;
  if (r20) goto case_body_1448; else goto switch_check_1459;

  switch_check_1459:
  r21 = cn_var_关键字_模块;
  r22 = r5 == r21;
  if (r22) goto case_body_1449; else goto switch_check_1460;

  switch_check_1460:
  r23 = cn_var_词元类型枚举;
  r24 = r23.关键字_导入;
  r25 = r5 == r24;
  if (r25) goto case_body_1450; else goto switch_check_1461;

  switch_check_1461:
  r26 = cn_var_词元类型枚举;
  r27 = r26.关键字_变量;
  r28 = r5 == r27;
  if (r28) goto case_body_1451; else goto switch_check_1462;

  switch_check_1462:
  r29 = cn_var_词元类型枚举;
  r30 = r29.关键字_常量;
  r31 = r5 == r30;
  if (r31) goto case_body_1452; else goto switch_check_1463;

  switch_check_1463:
  r32 = cn_var_词元类型枚举;
  r33 = r32.关键字_静态;
  r34 = r5 == r33;
  if (r34) goto case_body_1453; else goto case_default_1454;

  case_body_1444:
  r35 = cn_var_实例;
  解析函数声明(r35);
  return;
  goto switch_merge_1443;

  case_body_1445:
  r36 = cn_var_实例;
  解析结构体声明(r36);
  return;
  goto switch_merge_1443;

  case_body_1446:
  r37 = cn_var_实例;
  解析枚举声明(r37);
  return;
  goto switch_merge_1443;

  case_body_1447:
  r38 = cn_var_实例;
  解析类声明(r38);
  return;
  goto switch_merge_1443;

  case_body_1448:
  r39 = cn_var_实例;
  解析接口声明(r39);
  return;
  goto switch_merge_1443;

  case_body_1449:
  r40 = cn_var_实例;
  解析模块声明(r40);
  return;
  goto switch_merge_1443;

  case_body_1450:
  r41 = cn_var_实例;
  解析导入声明(r41);
  return;
  goto switch_merge_1443;

  case_body_1451:
  goto switch_merge_1443;

  case_body_1452:
  goto switch_merge_1443;

  case_body_1453:
  r42 = cn_var_实例;
  解析变量声明(r42);
  return;
  goto switch_merge_1443;

  case_default_1454:
  r43 = cn_var_实例;
  r44 = r43->当前词元;
  r45 = r44.类型;
  是否类型关键字(r45);
  goto if_then_1464;

  if_then_1464:
  r46 = cn_var_实例;
  解析变量声明(r46);
  return;
  goto if_merge_1465;

  if_merge_1465:
  r47 = cn_var_实例;
  r48 = cn_var_诊断错误码;
  r49 = r48.语法_期望标记;
  r50 = cn_var_实例;
  r51 = r50->当前词元;
  r52 = r51.值;
  r53 = "意外的词元: " + r52;
  报告错误(r47, r49, r53);
  r54 = cn_var_实例;
  前进词元(r54);
  return 0;
  goto switch_merge_1443;

  switch_merge_1443:
  return NULL;
}

struct 声明节点* 解析函数声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r4, r5, r6, r9, r12, r15, r18, r23, r26, r29, r32, r37, r43, r46, r50, r53, r58;
  char* r40;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r7;
  struct 解析器* r10;
  struct 解析器* r13;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 解析器* r34;
  struct 解析器* r35;
  struct 解析器* r38;
  struct 解析器* r41;
  struct 解析器* r44;
  struct 解析器* r47;
  struct 解析器* r48;
  struct 解析器* r51;
  struct 解析器* r54;
  struct 解析器* r55;
  struct 解析器* r56;
  struct 解析器* r59;
  struct 声明节点* r60;
  _Bool r24;
  _Bool r27;
  _Bool r30;
  _Bool r33;
  struct 词元 r20;
  struct 词元 r39;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r22;
  enum 词元类型枚举 r25;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r31;
  enum 词元类型枚举 r36;
  enum 词元类型枚举 r42;
  enum 词元类型枚举 r45;
  enum 词元类型枚举 r49;
  enum 词元类型枚举 r52;
  enum 词元类型枚举 r57;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1466; else goto if_merge_1467;

  if_then_1466:
  return 0;
  goto if_merge_1467;

  if_merge_1467:
  struct 声明节点* cn_var_节点_0;
  创建函数声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1468; else goto if_merge_1469;

  if_then_1468:
  return 0;
  goto if_merge_1469;

  if_merge_1469:
  goto while_cond_1470;

  while_cond_1470:
  r7 = cn_var_实例;
  r8 = cn_var_词元类型枚举;
  r9 = r8.关键字_重写;
  检查(r7, r9);
  goto logic_merge_1478;

  while_body_1471:
  r19 = cn_var_实例;
  r20 = r19->当前词元;
  r21 = r20.类型;
  r22 = cn_var_词元类型枚举;
  r23 = r22.关键字_重写;
  r24 = r21 == r23;
  if (r24) goto case_body_1480; else goto switch_check_1484;

  while_exit_1472:
  r35 = cn_var_实例;
  r36 = cn_var_词元类型枚举;
  r37 = r36.关键字_函数;
  期望(r35, r37);
  r38 = cn_var_实例;
  r39 = r38->当前词元;
  r40 = r39.值;
  r41 = cn_var_实例;
  r42 = cn_var_词元类型枚举;
  r43 = r42.标识符;
  期望(r41, r43);
  r44 = cn_var_实例;
  r45 = cn_var_词元类型枚举;
  r46 = r45.左括号;
  期望(r44, r46);
  r47 = cn_var_实例;
  解析参数列表(r47);
  r48 = cn_var_实例;
  r49 = cn_var_词元类型枚举;
  r50 = r49.右括号;
  期望(r48, r50);
  r51 = cn_var_实例;
  r52 = cn_var_词元类型枚举;
  r53 = r52.箭头;
  检查(r51, r53);
  goto if_then_1487;

  logic_rhs_1473:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.关键字_抽象;
  检查(r16, r18);
  goto logic_merge_1474;

  logic_merge_1474:
  goto while_body_1471;

  logic_rhs_1475:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.关键字_静态;
  检查(r13, r15);
  goto logic_merge_1476;

  logic_merge_1476:
  goto logic_merge_1474;

  logic_rhs_1477:
  r10 = cn_var_实例;
  r11 = cn_var_词元类型枚举;
  r12 = r11.关键字_虚拟;
  检查(r10, r12);
  goto logic_merge_1478;

  logic_merge_1478:
  goto logic_merge_1476;

  switch_check_1484:
  r25 = cn_var_词元类型枚举;
  r26 = r25.关键字_虚拟;
  r27 = r21 == r26;
  if (r27) goto case_body_1481; else goto switch_check_1485;

  switch_check_1485:
  r28 = cn_var_词元类型枚举;
  r29 = r28.关键字_静态;
  r30 = r21 == r29;
  if (r30) goto case_body_1482; else goto switch_check_1486;

  switch_check_1486:
  r31 = cn_var_词元类型枚举;
  r32 = r31.关键字_抽象;
  r33 = r21 == r32;
  if (r33) goto case_body_1483; else goto switch_merge_1479;

  case_body_1480:
  goto switch_merge_1479;
  goto switch_merge_1479;

  case_body_1481:
  goto switch_merge_1479;
  goto switch_merge_1479;

  case_body_1482:
  goto switch_merge_1479;
  goto switch_merge_1479;

  case_body_1483:
  goto switch_merge_1479;
  goto switch_merge_1479;

  switch_merge_1479:
  r34 = cn_var_实例;
  前进词元(r34);
  goto while_cond_1470;

  if_then_1487:
  r54 = cn_var_实例;
  前进词元(r54);
  r55 = cn_var_实例;
  解析类型(r55);
  goto if_merge_1488;

  if_merge_1488:
  r56 = cn_var_实例;
  r57 = cn_var_词元类型枚举;
  r58 = r57.左大括号;
  检查(r56, r58);
  goto if_then_1489;

  if_then_1489:
  r59 = cn_var_实例;
  解析块语句(r59);
  goto if_merge_1491;

  if_else_1490:
  goto if_merge_1491;

  if_merge_1491:
  r60 = cn_var_节点_0;
  return r60;
}

struct 参数列表* 解析参数列表(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r7, r10, r15, r16;
  struct 解析器* r0;
  struct 参数列表* r2;
  struct 解析器* r4;
  struct 解析器* r8;
  struct 参数列表* r11;
  struct 解析器* r13;
  struct 参数列表* r17;
  struct 参数 r9;
  struct 参数 r12;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1492; else goto if_merge_1493;

  if_then_1492:
  return 0;
  goto if_merge_1493;

  if_merge_1493:
  struct 参数列表* cn_var_列表_0;
  创建参数列表();
  cn_var_列表_0 = /* NONE */;
  r2 = cn_var_列表_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1494; else goto if_merge_1495;

  if_then_1494:
  return 0;
  goto if_merge_1495;

  if_merge_1495:
  goto while_cond_1496;

  while_cond_1496:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.右括号;
  检查(r4, r6);
  r7 = !/* NONE */;
  if (r7) goto while_body_1497; else goto while_exit_1498;

  while_body_1497:
  struct 参数节点* cn_var_参数_1;
  r8 = cn_var_实例;
  解析参数(r8);
  cn_var_参数_1 = /* NONE */;
  r9 = cn_var_参数_1;
  r10 = r9 != 0;
  if (r10) goto if_then_1499; else goto if_merge_1500;

  while_exit_1498:
  r17 = cn_var_列表_0;
  return r17;

  if_then_1499:
  r11 = cn_var_列表_0;
  r12 = cn_var_参数_1;
  参数列表添加(r11, r12);
  goto if_merge_1500;

  if_merge_1500:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.逗号;
  匹配(r13, r15);
  r16 = !/* NONE */;
  if (r16) goto if_then_1501; else goto if_merge_1502;

  if_then_1501:
  goto while_exit_1498;
  goto if_merge_1502;

  if_merge_1502:
  goto while_cond_1496;
  return NULL;
}

struct 参数节点* 解析参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r16, r17, r20, r24;
  char* r7;
  struct 解析器* r0;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r18;
  struct 解析器* r21;
  struct 解析器* r22;
  struct 参数 r2;
  struct 词元 r6;
  struct 参数 r15;
  struct 参数 r25;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r23;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1503; else goto if_merge_1504;

  if_then_1503:
  return 0;
  goto if_merge_1504;

  if_merge_1504:
  struct 参数节点* cn_var_参数_0;
  创建参数节点();
  cn_var_参数_0 = /* NONE */;
  r2 = cn_var_参数_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1505; else goto if_merge_1506;

  if_then_1505:
  return 0;
  goto if_merge_1506;

  if_merge_1506:
  r4 = cn_var_实例;
  解析类型(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.标识符;
  期望(r8, r10);
  goto while_cond_1507;

  while_cond_1507:
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.左方括号;
  检查(r11, r13);
  goto while_body_1508;

  while_body_1508:
  r14 = cn_var_实例;
  前进词元(r14);
  r15 = cn_var_参数_0;
  r16 = r15.数组维度;
  r17 = r16 + 1;
  r18 = cn_var_实例;
  r19 = cn_var_词元类型枚举;
  r20 = r19.整数字面量;
  检查(r18, r20);
  goto if_then_1510;

  while_exit_1509:
  r25 = cn_var_参数_0;
  return r25;

  if_then_1510:
  r21 = cn_var_实例;
  前进词元(r21);
  goto if_merge_1511;

  if_merge_1511:
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.右方括号;
  期望(r22, r24);
  goto while_cond_1507;
  return NULL;
}

struct 声明节点* 解析变量声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r10, r18, r25, r28, r31, r32, r33, r36, r38, r39, r46, r49, r53;
  char* r22;
  char* r42;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 解析器* r29;
  struct 声明节点* r30;
  struct 解析器* r34;
  struct 声明节点* r37;
  struct 解析器* r40;
  struct 解析器* r43;
  struct 解析器* r44;
  struct 解析器* r47;
  struct 解析器* r50;
  struct 解析器* r51;
  struct 声明节点* r54;
  struct 词元 r13;
  struct 词元 r21;
  struct 词元 r41;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r17;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r45;
  enum 词元类型枚举 r48;
  enum 词元类型枚举 r52;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1512; else goto if_merge_1513;

  if_then_1512:
  return 0;
  goto if_merge_1513;

  if_merge_1513:
  struct 声明节点* cn_var_节点_0;
  创建变量声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1514; else goto if_merge_1515;

  if_then_1514:
  return 0;
  goto if_merge_1515;

  if_merge_1515:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.关键字_静态;
  检查(r4, r6);
  goto if_then_1516;

  if_then_1516:
  r7 = cn_var_实例;
  前进词元(r7);
  goto if_merge_1517;

  if_merge_1517:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.关键字_常量;
  检查(r8, r10);
  goto if_then_1518;

  if_then_1518:
  r11 = cn_var_实例;
  前进词元(r11);
  goto if_merge_1519;

  if_merge_1519:
  r12 = cn_var_实例;
  r13 = r12->当前词元;
  r14 = r13.类型;
  是否类型关键字(r14);
  goto if_then_1520;

  if_then_1520:
  r15 = cn_var_实例;
  解析类型(r15);
  goto if_merge_1522;

  if_else_1521:
  r16 = cn_var_实例;
  r17 = cn_var_词元类型枚举;
  r18 = r17.关键字_变量;
  检查(r16, r18);
  goto if_then_1523;

  if_merge_1522:
  r20 = cn_var_实例;
  r21 = r20->当前词元;
  r22 = r21.值;
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.标识符;
  期望(r23, r25);
  goto while_cond_1525;

  if_then_1523:
  r19 = cn_var_实例;
  前进词元(r19);
  goto if_merge_1524;

  if_merge_1524:
  goto if_merge_1522;

  while_cond_1525:
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.左方括号;
  检查(r26, r28);
  goto while_body_1526;

  while_body_1526:
  r29 = cn_var_实例;
  前进词元(r29);
  r30 = cn_var_节点_0;
  r31 = r30->变量声明;
  r32 = r31.数组维度;
  r33 = r32 + 1;
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.整数字面量;
  检查(r34, r36);
  goto if_then_1528;

  while_exit_1527:
  r47 = cn_var_实例;
  r48 = cn_var_词元类型枚举;
  r49 = r48.赋值;
  匹配(r47, r49);
  goto if_then_1530;

  if_then_1528:
  r37 = cn_var_节点_0;
  r38 = r37->变量声明;
  r39 = r38.数组大小;
  r40 = cn_var_实例;
  r41 = r40->当前词元;
  r42 = r41.值;
  字符串转整数(r42);
  数组大小列表添加(r39, /* NONE */);
  r43 = cn_var_实例;
  前进词元(r43);
  goto if_merge_1529;

  if_merge_1529:
  r44 = cn_var_实例;
  r45 = cn_var_词元类型枚举;
  r46 = r45.右方括号;
  期望(r44, r46);
  goto while_cond_1525;

  if_then_1530:
  r50 = cn_var_实例;
  解析表达式(r50);
  goto if_merge_1531;

  if_merge_1531:
  r51 = cn_var_实例;
  r52 = cn_var_词元类型枚举;
  r53 = r52.分号;
  期望(r51, r53);
  r54 = cn_var_节点_0;
  return r54;
}

struct 声明节点* 解析结构体声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r16, r17, r20, r25;
  char* r7;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r18;
  struct 声明节点* r19;
  struct 声明节点* r21;
  struct 声明节点* r22;
  struct 解析器* r23;
  struct 声明节点* r26;
  struct 词元 r6;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r24;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1532; else goto if_merge_1533;

  if_then_1532:
  return 0;
  goto if_merge_1533;

  if_merge_1533:
  struct 声明节点* cn_var_节点_0;
  创建结构体声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1534; else goto if_merge_1535;

  if_then_1534:
  return 0;
  goto if_merge_1535;

  if_merge_1535:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.标识符;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.左大括号;
  期望(r11, r13);
  goto while_cond_1536;

  while_cond_1536:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.右大括号;
  检查(r14, r16);
  r17 = !/* NONE */;
  if (r17) goto while_body_1537; else goto while_exit_1538;

  while_body_1537:
  struct 声明节点* cn_var_成员_1;
  r18 = cn_var_实例;
  解析变量声明(r18);
  cn_var_成员_1 = /* NONE */;
  r19 = cn_var_成员_1;
  r20 = r19 != 0;
  if (r20) goto if_then_1539; else goto if_merge_1540;

  while_exit_1538:
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.右大括号;
  期望(r23, r25);
  r26 = cn_var_节点_0;
  return r26;

  if_then_1539:
  r21 = cn_var_节点_0;
  r22 = cn_var_成员_1;
  结构体添加成员(r21, r22);
  goto if_merge_1540;

  if_merge_1540:
  goto while_cond_1536;
  return NULL;
}

struct 声明节点* 解析枚举声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r16, r17, r19, r25, r28, r33, r36, r37, r38, r39, r44, r47;
  char* r7;
  char* r22;
  char* r31;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 枚举成员* r18;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 解析器* r29;
  struct 枚举成员* r32;
  struct 解析器* r34;
  struct 声明节点* r40;
  struct 枚举成员* r41;
  struct 解析器* r42;
  struct 解析器* r45;
  struct 声明节点* r48;
  struct 词元 r6;
  struct 词元 r21;
  struct 词元 r30;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r43;
  enum 词元类型枚举 r46;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1541; else goto if_merge_1542;

  if_then_1541:
  return 0;
  goto if_merge_1542;

  if_merge_1542:
  struct 声明节点* cn_var_节点_0;
  创建枚举声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1543; else goto if_merge_1544;

  if_then_1543:
  return 0;
  goto if_merge_1544;

  if_merge_1544:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.标识符;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.左大括号;
  期望(r11, r13);
  long long cn_var_当前值_1;
  cn_var_当前值_1 = 0;
  goto while_cond_1545;

  while_cond_1545:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.右大括号;
  检查(r14, r16);
  r17 = !/* NONE */;
  if (r17) goto while_body_1546; else goto while_exit_1547;

  while_body_1546:
  struct 枚举成员* cn_var_成员_2;
  创建枚举成员();
  cn_var_成员_2 = /* NONE */;
  r18 = cn_var_成员_2;
  r19 = r18 == 0;
  if (r19) goto if_then_1548; else goto if_merge_1549;

  while_exit_1547:
  r45 = cn_var_实例;
  r46 = cn_var_词元类型枚举;
  r47 = r46.右大括号;
  期望(r45, r47);
  r48 = cn_var_节点_0;
  return r48;

  if_then_1548:
  goto while_exit_1547;
  goto if_merge_1549;

  if_merge_1549:
  r20 = cn_var_实例;
  r21 = r20->当前词元;
  r22 = r21.值;
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.标识符;
  期望(r23, r25);
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.赋值;
  匹配(r26, r28);
  goto if_then_1550;

  if_then_1550:
  r29 = cn_var_实例;
  r30 = r29->当前词元;
  r31 = r30.值;
  字符串转整数(r31);
  r32 = cn_var_成员_2;
  r33 = r32->值;
  cn_var_当前值_1 = r33;
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.整数字面量;
  期望(r34, r36);
  goto if_merge_1552;

  if_else_1551:
  r37 = cn_var_当前值_1;
  goto if_merge_1552;

  if_merge_1552:
  r38 = cn_var_当前值_1;
  r39 = r38 + 1;
  cn_var_当前值_1 = r39;
  r40 = cn_var_节点_0;
  r41 = cn_var_成员_2;
  枚举添加成员(r40, r41);
  r42 = cn_var_实例;
  r43 = cn_var_词元类型枚举;
  r44 = r43.逗号;
  匹配(r42, r44);
  goto while_cond_1545;
  return NULL;
}

struct 声明节点* 解析类声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r10, r16, r19, r25, r28, r35, r40, r41, r44, r47, r48, r52, r55, r61, r67;
  char* r13;
  char* r22;
  char* r32;
  char* r37;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 解析器* r29;
  struct 解析器* r30;
  struct 解析器* r33;
  struct 声明节点* r36;
  struct 解析器* r38;
  struct 解析器* r42;
  struct 解析器* r45;
  struct 解析器* r49;
  struct 解析器* r50;
  struct 解析器* r53;
  struct 声明节点* r54;
  struct 声明节点* r57;
  struct 声明节点* r58;
  struct 解析器* r59;
  struct 声明节点* r60;
  struct 声明节点* r63;
  struct 声明节点* r64;
  struct 解析器* r65;
  struct 声明节点* r68;
  struct 词元 r12;
  struct 词元 r21;
  struct 词元 r31;
  struct 可见性 r56;
  struct 可见性 r62;
  enum 词元类型枚举 r5;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r27;
  enum 词元类型枚举 r34;
  enum 词元类型枚举 r39;
  enum 词元类型枚举 r43;
  enum 词元类型枚举 r46;
  enum 词元类型枚举 r51;
  enum 词元类型枚举 r66;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1553; else goto if_merge_1554;

  if_then_1553:
  return 0;
  goto if_merge_1554;

  if_merge_1554:
  struct 声明节点* cn_var_节点_0;
  创建类声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1555; else goto if_merge_1556;

  if_then_1555:
  return 0;
  goto if_merge_1556;

  if_merge_1556:
  r4 = cn_var_实例;
  r5 = cn_var_词元类型枚举;
  r6 = r5.关键字_抽象;
  检查(r4, r6);
  goto if_then_1557;

  if_then_1557:
  r7 = cn_var_实例;
  前进词元(r7);
  goto if_merge_1558;

  if_merge_1558:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.关键字_类;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = r11->当前词元;
  r13 = r12.值;
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.标识符;
  期望(r14, r16);
  r17 = cn_var_实例;
  r18 = cn_var_词元类型枚举;
  r19 = r18.冒号;
  匹配(r17, r19);
  goto if_then_1559;

  if_then_1559:
  r20 = cn_var_实例;
  r21 = r20->当前词元;
  r22 = r21.值;
  r23 = cn_var_实例;
  r24 = cn_var_词元类型枚举;
  r25 = r24.标识符;
  期望(r23, r25);
  goto if_merge_1560;

  if_merge_1560:
  r26 = cn_var_实例;
  r27 = cn_var_词元类型枚举;
  r28 = r27.关键字_实现;
  检查(r26, r28);
  goto if_then_1561;

  if_then_1561:
  r29 = cn_var_实例;
  前进词元(r29);
  goto while_cond_1563;

  if_merge_1562:
  r42 = cn_var_实例;
  r43 = cn_var_词元类型枚举;
  r44 = r43.左大括号;
  期望(r42, r44);
  goto while_cond_1568;

  while_cond_1563:
  if (1) goto while_body_1564; else goto while_exit_1565;

  while_body_1564:
  char* cn_var_接口名_1;
  r30 = cn_var_实例;
  r31 = r30->当前词元;
  r32 = r31.值;
  cn_var_接口名_1 = r32;
  r33 = cn_var_实例;
  r34 = cn_var_词元类型枚举;
  r35 = r34.标识符;
  期望(r33, r35);
  r36 = cn_var_节点_0;
  r37 = cn_var_接口名_1;
  类添加实现接口(r36, r37);
  r38 = cn_var_实例;
  r39 = cn_var_词元类型枚举;
  r40 = r39.逗号;
  匹配(r38, r40);
  r41 = !/* NONE */;
  if (r41) goto if_then_1566; else goto if_merge_1567;

  while_exit_1565:
  goto if_merge_1562;

  if_then_1566:
  goto while_exit_1565;
  goto if_merge_1567;

  if_merge_1567:
  goto while_cond_1563;

  while_cond_1568:
  r45 = cn_var_实例;
  r46 = cn_var_词元类型枚举;
  r47 = r46.右大括号;
  检查(r45, r47);
  r48 = !/* NONE */;
  if (r48) goto while_body_1569; else goto while_exit_1570;

  while_body_1569:
  struct 可见性 cn_var_成员可见性_2;
  r49 = cn_var_实例;
  解析可见性修饰符(r49);
  cn_var_成员可见性_2 = /* NONE */;
  r50 = cn_var_实例;
  r51 = cn_var_词元类型枚举;
  r52 = r51.关键字_函数;
  检查(r50, r52);
  goto if_then_1571;

  while_exit_1570:
  r65 = cn_var_实例;
  r66 = cn_var_词元类型枚举;
  r67 = r66.右大括号;
  期望(r65, r67);
  r68 = cn_var_节点_0;
  return r68;

  if_then_1571:
  struct 声明节点* cn_var_方法_3;
  r53 = cn_var_实例;
  解析函数声明(r53);
  cn_var_方法_3 = /* NONE */;
  r54 = cn_var_方法_3;
  r55 = r54 != 0;
  if (r55) goto if_then_1574; else goto if_merge_1575;

  if_else_1572:
  struct 声明节点* cn_var_字段_4;
  r59 = cn_var_实例;
  解析变量声明(r59);
  cn_var_字段_4 = /* NONE */;
  r60 = cn_var_字段_4;
  r61 = r60 != 0;
  if (r61) goto if_then_1576; else goto if_merge_1577;

  if_merge_1573:
  goto while_cond_1568;

  if_then_1574:
  r56 = cn_var_成员可见性_2;
  r57 = cn_var_节点_0;
  r58 = cn_var_方法_3;
  类添加方法(r57, r58);
  goto if_merge_1575;

  if_merge_1575:
  goto if_merge_1573;

  if_then_1576:
  r62 = cn_var_成员可见性_2;
  r63 = cn_var_节点_0;
  r64 = cn_var_字段_4;
  类添加字段(r63, r64);
  goto if_merge_1577;

  if_merge_1577:
  goto if_merge_1573;
  return NULL;
}

struct 声明节点* 解析接口声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r16, r17, r20, r23, r29;
  char* r7;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r18;
  struct 解析器* r21;
  struct 声明节点* r22;
  struct 声明节点* r24;
  struct 声明节点* r25;
  struct 解析器* r26;
  struct 解析器* r27;
  struct 声明节点* r30;
  struct 词元 r6;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r15;
  enum 词元类型枚举 r19;
  enum 词元类型枚举 r28;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1578; else goto if_merge_1579;

  if_then_1578:
  return 0;
  goto if_merge_1579;

  if_merge_1579:
  struct 声明节点* cn_var_节点_0;
  创建接口声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1580; else goto if_merge_1581;

  if_then_1580:
  return 0;
  goto if_merge_1581;

  if_merge_1581:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.标识符;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.左大括号;
  期望(r11, r13);
  goto while_cond_1582;

  while_cond_1582:
  r14 = cn_var_实例;
  r15 = cn_var_词元类型枚举;
  r16 = r15.右大括号;
  检查(r14, r16);
  r17 = !/* NONE */;
  if (r17) goto while_body_1583; else goto while_exit_1584;

  while_body_1583:
  r18 = cn_var_实例;
  r19 = cn_var_词元类型枚举;
  r20 = r19.关键字_函数;
  检查(r18, r20);
  goto if_then_1585;

  while_exit_1584:
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.右大括号;
  期望(r27, r29);
  r30 = cn_var_节点_0;
  return r30;

  if_then_1585:
  struct 声明节点* cn_var_方法_1;
  r21 = cn_var_实例;
  解析函数声明(r21);
  cn_var_方法_1 = /* NONE */;
  r22 = cn_var_方法_1;
  r23 = r22 != 0;
  if (r23) goto if_then_1588; else goto if_merge_1589;

  if_else_1586:
  r26 = cn_var_实例;
  前进词元(r26);
  goto if_merge_1587;

  if_merge_1587:
  goto while_cond_1582;

  if_then_1588:
  r24 = cn_var_节点_0;
  r25 = cn_var_方法_1;
  接口添加方法(r24, r25);
  goto if_merge_1589;

  if_merge_1589:
  goto if_merge_1587;
  return NULL;
}

struct 声明节点* 解析模块声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r7;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  enum 诊断错误码 r6;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1590; else goto if_merge_1591;

  if_then_1590:
  return 0;
  goto if_merge_1591;

  if_merge_1591:
  struct 声明节点* cn_var_节点_0;
  创建模块声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1592; else goto if_merge_1593;

  if_then_1592:
  return 0;
  goto if_merge_1593;

  if_merge_1593:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = cn_var_诊断错误码;
  r7 = r6.语法_预留特性;
  报告错误(r5, r7, "模块关键字暂不支持，请使用文件即模块");
  return 0;
}

struct 声明节点* 解析导入声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r10, r13, r17, r18, r24, r29, r30, r33, r36, r43, r46, r49;
  char* r7;
  char* r21;
  char* r26;
  char* r40;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r14;
  struct 解析器* r15;
  struct 解析器* r19;
  struct 解析器* r22;
  struct 声明节点* r25;
  struct 解析器* r27;
  struct 解析器* r31;
  struct 解析器* r34;
  struct 解析器* r37;
  struct 解析器* r38;
  struct 解析器* r41;
  struct 解析器* r44;
  struct 解析器* r47;
  struct 声明节点* r50;
  struct 词元 r6;
  struct 词元 r20;
  struct 词元 r39;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r35;
  enum 词元类型枚举 r42;
  enum 词元类型枚举 r45;
  enum 词元类型枚举 r48;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1594; else goto if_merge_1595;

  if_then_1594:
  return 0;
  goto if_merge_1595;

  if_merge_1595:
  struct 声明节点* cn_var_节点_0;
  创建导入声明();
  cn_var_节点_0 = /* NONE */;
  r2 = cn_var_节点_0;
  r3 = r2 == 0;
  if (r3) goto if_then_1596; else goto if_merge_1597;

  if_then_1596:
  return 0;
  goto if_merge_1597;

  if_merge_1597:
  r4 = cn_var_实例;
  前进词元(r4);
  r5 = cn_var_实例;
  r6 = r5->当前词元;
  r7 = r6.值;
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.标识符;
  期望(r8, r10);
  r11 = cn_var_实例;
  r12 = cn_var_词元类型枚举;
  r13 = r12.左大括号;
  检查(r11, r13);
  goto if_then_1598;

  if_then_1598:
  r14 = cn_var_实例;
  前进词元(r14);
  goto while_cond_1600;

  if_merge_1599:
  r34 = cn_var_实例;
  r35 = cn_var_词元类型枚举;
  r36 = r35.左括号;
  检查(r34, r36);
  goto if_then_1605;

  while_cond_1600:
  r15 = cn_var_实例;
  r16 = cn_var_词元类型枚举;
  r17 = r16.右大括号;
  检查(r15, r17);
  r18 = !/* NONE */;
  if (r18) goto while_body_1601; else goto while_exit_1602;

  while_body_1601:
  char* cn_var_成员名_1;
  r19 = cn_var_实例;
  r20 = r19->当前词元;
  r21 = r20.值;
  cn_var_成员名_1 = r21;
  r22 = cn_var_实例;
  r23 = cn_var_词元类型枚举;
  r24 = r23.标识符;
  期望(r22, r24);
  r25 = cn_var_节点_0;
  r26 = cn_var_成员名_1;
  导入添加成员(r25, r26);
  r27 = cn_var_实例;
  r28 = cn_var_词元类型枚举;
  r29 = r28.逗号;
  匹配(r27, r29);
  r30 = !/* NONE */;
  if (r30) goto if_then_1603; else goto if_merge_1604;

  while_exit_1602:
  r31 = cn_var_实例;
  r32 = cn_var_词元类型枚举;
  r33 = r32.右大括号;
  期望(r31, r33);
  goto if_merge_1599;

  if_then_1603:
  goto while_exit_1602;
  goto if_merge_1604;

  if_merge_1604:
  goto while_cond_1600;

  if_then_1605:
  r37 = cn_var_实例;
  前进词元(r37);
  r38 = cn_var_实例;
  r39 = r38->当前词元;
  r40 = r39.值;
  r41 = cn_var_实例;
  r42 = cn_var_词元类型枚举;
  r43 = r42.标识符;
  期望(r41, r43);
  r44 = cn_var_实例;
  r45 = cn_var_词元类型枚举;
  r46 = r45.右括号;
  期望(r44, r46);
  goto if_merge_1606;

  if_merge_1606:
  r47 = cn_var_实例;
  r48 = cn_var_词元类型枚举;
  r49 = r48.分号;
  期望(r47, r49);
  r50 = cn_var_节点_0;
  return r50;
}

enum 可见性 解析可见性修饰符(struct 解析器* cn_var_实例) {
  long long r1, r2, r5, r7, r10, r12, r15, r17, r18;
  struct 解析器* r0;
  struct 解析器* r3;
  struct 解析器* r6;
  struct 解析器* r8;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r16;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r9;
  enum 词元类型枚举 r14;

  entry:
  r0 = cn_var_实例;
  r1 = r0 == 0;
  if (r1) goto if_then_1607; else goto if_merge_1608;

  if_then_1607:
  r2 = cn_var_私有可见;
  return r2;
  goto if_merge_1608;

  if_merge_1608:
  r3 = cn_var_实例;
  r4 = cn_var_词元类型枚举;
  r5 = r4.关键字_公开;
  检查(r3, r5);
  goto if_then_1609;

  if_then_1609:
  r6 = cn_var_实例;
  前进词元(r6);
  r7 = cn_var_公开可见;
  return r7;
  goto if_merge_1610;

  if_merge_1610:
  r8 = cn_var_实例;
  r9 = cn_var_词元类型枚举;
  r10 = r9.关键字_私有;
  检查(r8, r10);
  goto if_then_1611;

  if_then_1611:
  r11 = cn_var_实例;
  前进词元(r11);
  r12 = cn_var_私有可见;
  return r12;
  goto if_merge_1612;

  if_merge_1612:
  r13 = cn_var_实例;
  r14 = cn_var_词元类型枚举;
  r15 = r14.关键字_保护;
  检查(r13, r15);
  goto if_then_1613;

  if_then_1613:
  r16 = cn_var_实例;
  前进词元(r16);
  r17 = cn_var_默认可见;
  return r17;
  goto if_merge_1614;

  if_merge_1614:
  r18 = cn_var_私有可见;
  return r18;
}

