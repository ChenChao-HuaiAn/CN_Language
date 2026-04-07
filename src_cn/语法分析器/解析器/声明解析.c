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
  long long r1, r12, r44;
  char* r43;
  struct 解析器* r0;
  struct 解析器* r2;
  struct 解析器* r4;
  struct 解析器* r18;
  struct 声明节点* r19;
  struct 解析器* r20;
  struct 声明节点* r21;
  struct 解析器* r22;
  struct 声明节点* r23;
  struct 解析器* r24;
  struct 声明节点* r25;
  struct 解析器* r26;
  struct 声明节点* r27;
  struct 解析器* r28;
  struct 声明节点* r29;
  struct 解析器* r30;
  struct 声明节点* r31;
  struct 解析器* r32;
  struct 声明节点* r33;
  struct 解析器* r34;
  struct 解析器* r38;
  struct 声明节点* r39;
  struct 解析器* r40;
  struct 解析器* r41;
  struct 解析器* r45;
  _Bool r7;
  _Bool r8;
  _Bool r9;
  _Bool r10;
  _Bool r11;
  _Bool r13;
  _Bool r14;
  _Bool r15;
  _Bool r16;
  _Bool r17;
  _Bool r37;
  struct 可见性 r3;
  struct 词元 r5;
  struct 词元 r35;
  struct 词元 r42;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r36;

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
  r3 = 解析可见性修饰符(r2);
  cn_var_当前可见性_0 = r3;
  r4 = cn_var_实例;
  r5 = r4->当前词元;
  r6 = r5.类型;
  r7 = r6 == 17;
  if (r7) goto case_body_1444; else goto switch_check_1455;

  switch_check_1455:
  r8 = r6 == 15;
  if (r8) goto case_body_1445; else goto switch_check_1456;

  switch_check_1456:
  r9 = r6 == 16;
  if (r9) goto case_body_1446; else goto switch_check_1457;

  switch_check_1457:
  r10 = r6 == 27;
  if (r10) goto case_body_1447; else goto switch_check_1458;

  switch_check_1458:
  r11 = r6 == 28;
  if (r11) goto case_body_1448; else goto switch_check_1459;

  switch_check_1459:
  r12 = cn_var_关键字_模块;
  r13 = r6 == r12;
  if (r13) goto case_body_1449; else goto switch_check_1460;

  switch_check_1460:
  r14 = r6 == 19;
  if (r14) goto case_body_1450; else goto switch_check_1461;

  switch_check_1461:
  r15 = r6 == 18;
  if (r15) goto case_body_1451; else goto switch_check_1462;

  switch_check_1462:
  r16 = r6 == /* NONE */;
  if (r16) goto case_body_1452; else goto switch_check_1463;

  switch_check_1463:
  r17 = r6 == 23;
  if (r17) goto case_body_1453; else goto case_default_1454;

  case_body_1444:
  r18 = cn_var_实例;
  r19 = 解析函数声明(r18);
  return r19;
  goto switch_merge_1443;

  case_body_1445:
  r20 = cn_var_实例;
  r21 = 解析结构体声明(r20);
  return r21;
  goto switch_merge_1443;

  case_body_1446:
  r22 = cn_var_实例;
  r23 = 解析枚举声明(r22);
  return r23;
  goto switch_merge_1443;

  case_body_1447:
  r24 = cn_var_实例;
  r25 = 解析类声明(r24);
  return r25;
  goto switch_merge_1443;

  case_body_1448:
  r26 = cn_var_实例;
  r27 = 解析接口声明(r26);
  return r27;
  goto switch_merge_1443;

  case_body_1449:
  r28 = cn_var_实例;
  r29 = 解析模块声明(r28);
  return r29;
  goto switch_merge_1443;

  case_body_1450:
  r30 = cn_var_实例;
  r31 = 解析导入声明(r30);
  return r31;
  goto switch_merge_1443;

  case_body_1451:
  goto switch_merge_1443;

  case_body_1452:
  goto switch_merge_1443;

  case_body_1453:
  r32 = cn_var_实例;
  r33 = 解析变量声明(r32);
  return r33;
  goto switch_merge_1443;

  case_default_1454:
  r34 = cn_var_实例;
  r35 = r34->当前词元;
  r36 = r35.类型;
  r37 = 是否类型关键字(r36);
  if (r37) goto if_then_1464; else goto if_merge_1465;

  if_then_1464:
  r38 = cn_var_实例;
  r39 = 解析变量声明(r38);
  return r39;
  goto if_merge_1465;

  if_merge_1465:
  r40 = cn_var_实例;
  r41 = cn_var_实例;
  r42 = r41->当前词元;
  r43 = r42.值;
  r44 = "意外的词元: " + r43;
  报告错误(r40, 11, r44);
  r45 = cn_var_实例;
  前进词元(r45);
  return 0;
  goto switch_merge_1443;

  switch_merge_1443:
  return NULL;
}

struct 声明节点* 解析函数声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r4, r5, r6;
  char* r27;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r15;
  struct 解析器* r22;
  struct 解析器* r23;
  struct 解析器* r25;
  struct 解析器* r28;
  struct 解析器* r30;
  struct 解析器* r32;
  struct 参数列表* r33;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 解析器* r39;
  struct 类型节点* r40;
  struct 解析器* r41;
  struct 解析器* r43;
  struct 块语句节点* r44;
  struct 声明节点* r45;
  _Bool r8;
  _Bool r10;
  _Bool r12;
  _Bool r14;
  _Bool r18;
  _Bool r19;
  _Bool r20;
  _Bool r21;
  _Bool r24;
  _Bool r29;
  _Bool r31;
  _Bool r35;
  _Bool r37;
  _Bool r42;
  struct 词元 r16;
  struct 词元 r26;
  enum 词元类型枚举 r17;

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
  r8 = 检查(r7, 31);
  if (r8) goto logic_merge_1478; else goto logic_rhs_1477;

  while_body_1471:
  r15 = cn_var_实例;
  r16 = r15->当前词元;
  r17 = r16.类型;
  r18 = r17 == 31;
  if (r18) goto case_body_1480; else goto switch_check_1484;

  while_exit_1472:
  r23 = cn_var_实例;
  r24 = 期望(r23, 17);
  r25 = cn_var_实例;
  r26 = r25->当前词元;
  r27 = r26.值;
  r28 = cn_var_实例;
  r29 = 期望(r28, 40);
  r30 = cn_var_实例;
  r31 = 期望(r30, 69);
  r32 = cn_var_实例;
  r33 = 解析参数列表(r32);
  r34 = cn_var_实例;
  r35 = 期望(r34, 70);
  r36 = cn_var_实例;
  r37 = 检查(r36, 68);
  if (r37) goto if_then_1487; else goto if_merge_1488;

  logic_rhs_1473:
  r13 = cn_var_实例;
  r14 = 检查(r13, 32);
  goto logic_merge_1474;

  logic_merge_1474:
  if (r14) goto while_body_1471; else goto while_exit_1472;

  logic_rhs_1475:
  r11 = cn_var_实例;
  r12 = 检查(r11, 23);
  goto logic_merge_1476;

  logic_merge_1476:
  if (r12) goto logic_merge_1474; else goto logic_rhs_1473;

  logic_rhs_1477:
  r9 = cn_var_实例;
  r10 = 检查(r9, 30);
  goto logic_merge_1478;

  logic_merge_1478:
  if (r10) goto logic_merge_1476; else goto logic_rhs_1475;

  switch_check_1484:
  r19 = r17 == 30;
  if (r19) goto case_body_1481; else goto switch_check_1485;

  switch_check_1485:
  r20 = r17 == 23;
  if (r20) goto case_body_1482; else goto switch_check_1486;

  switch_check_1486:
  r21 = r17 == 32;
  if (r21) goto case_body_1483; else goto switch_merge_1479;

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
  r22 = cn_var_实例;
  前进词元(r22);
  goto while_cond_1470;

  if_then_1487:
  r38 = cn_var_实例;
  前进词元(r38);
  r39 = cn_var_实例;
  r40 = 解析类型(r39);
  goto if_merge_1488;

  if_merge_1488:
  r41 = cn_var_实例;
  r42 = 检查(r41, 71);
  if (r42) goto if_then_1489; else goto if_else_1490;

  if_then_1489:
  r43 = cn_var_实例;
  r44 = 解析块语句(r43);
  goto if_merge_1491;

  if_else_1490:
  goto if_merge_1491;

  if_merge_1491:
  r45 = cn_var_节点_0;
  return r45;
}

struct 参数列表* 解析参数列表(struct 解析器* cn_var_实例) {
  long long r1, r3, r6, r10, r15;
  struct 解析器* r0;
  struct 参数列表* r2;
  struct 解析器* r4;
  struct 解析器* r7;
  struct 参数节点* r8;
  struct 参数列表* r11;
  struct 解析器* r13;
  struct 参数列表* r16;
  _Bool r5;
  _Bool r14;
  struct 参数 r9;
  struct 参数 r12;

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
  r5 = 检查(r4, 70);
  r6 = !r5;
  if (r6) goto while_body_1497; else goto while_exit_1498;

  while_body_1497:
  struct 参数节点* cn_var_参数_1;
  r7 = cn_var_实例;
  r8 = 解析参数(r7);
  cn_var_参数_1 = r8;
  r9 = cn_var_参数_1;
  r10 = r9 != 0;
  if (r10) goto if_then_1499; else goto if_merge_1500;

  while_exit_1498:
  r16 = cn_var_列表_0;
  return r16;

  if_then_1499:
  r11 = cn_var_列表_0;
  r12 = cn_var_参数_1;
  参数列表添加(r11, r12);
  goto if_merge_1500;

  if_merge_1500:
  r13 = cn_var_实例;
  r14 = 匹配(r13, 76);
  r15 = !r14;
  if (r15) goto if_then_1501; else goto if_merge_1502;

  if_then_1501:
  goto while_exit_1498;
  goto if_merge_1502;

  if_merge_1502:
  goto while_cond_1496;
  return NULL;
}

struct 参数节点* 解析参数(struct 解析器* cn_var_实例) {
  long long r1, r3, r15, r16;
  char* r8;
  struct 解析器* r0;
  struct 解析器* r4;
  struct 类型节点* r5;
  struct 解析器* r6;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  struct 解析器* r17;
  struct 解析器* r19;
  struct 解析器* r20;
  _Bool r10;
  _Bool r12;
  _Bool r18;
  _Bool r21;
  struct 参数 r2;
  struct 词元 r7;
  struct 参数 r14;
  struct 参数 r22;

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
  r5 = 解析类型(r4);
  r6 = cn_var_实例;
  r7 = r6->当前词元;
  r8 = r7.值;
  r9 = cn_var_实例;
  r10 = 期望(r9, 40);
  goto while_cond_1507;

  while_cond_1507:
  r11 = cn_var_实例;
  r12 = 检查(r11, 73);
  if (r12) goto while_body_1508; else goto while_exit_1509;

  while_body_1508:
  r13 = cn_var_实例;
  前进词元(r13);
  r14 = cn_var_参数_0;
  r15 = r14.数组维度;
  r16 = r15 + 1;
  r17 = cn_var_实例;
  r18 = 检查(r17, 41);
  if (r18) goto if_then_1510; else goto if_merge_1511;

  while_exit_1509:
  r22 = cn_var_参数_0;
  return r22;

  if_then_1510:
  r19 = cn_var_实例;
  前进词元(r19);
  goto if_merge_1511;

  if_merge_1511:
  r20 = cn_var_实例;
  r21 = 期望(r20, 74);
  goto while_cond_1507;
  return NULL;
}

struct 声明节点* 解析变量声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r28, r29, r30, r34, r35;
  char* r21;
  char* r38;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r6;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r10;
  struct 解析器* r14;
  struct 类型节点* r15;
  struct 解析器* r16;
  struct 解析器* r18;
  struct 解析器* r19;
  struct 解析器* r22;
  struct 解析器* r24;
  struct 解析器* r26;
  struct 声明节点* r27;
  struct 解析器* r31;
  struct 声明节点* r33;
  struct 解析器* r36;
  struct 解析器* r39;
  struct 解析器* r40;
  struct 解析器* r42;
  struct 解析器* r44;
  struct 表达式节点* r45;
  struct 解析器* r46;
  struct 声明节点* r48;
  _Bool r5;
  _Bool r8;
  _Bool r13;
  _Bool r17;
  _Bool r23;
  _Bool r25;
  _Bool r32;
  _Bool r41;
  _Bool r43;
  _Bool r47;
  struct 词元 r11;
  struct 词元 r20;
  struct 词元 r37;
  enum 词元类型枚举 r12;

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
  r5 = 检查(r4, 23);
  if (r5) goto if_then_1516; else goto if_merge_1517;

  if_then_1516:
  r6 = cn_var_实例;
  前进词元(r6);
  goto if_merge_1517;

  if_merge_1517:
  r7 = cn_var_实例;
  r8 = 检查(r7, /* NONE */);
  if (r8) goto if_then_1518; else goto if_merge_1519;

  if_then_1518:
  r9 = cn_var_实例;
  前进词元(r9);
  goto if_merge_1519;

  if_merge_1519:
  r10 = cn_var_实例;
  r11 = r10->当前词元;
  r12 = r11.类型;
  r13 = 是否类型关键字(r12);
  if (r13) goto if_then_1520; else goto if_else_1521;

  if_then_1520:
  r14 = cn_var_实例;
  r15 = 解析类型(r14);
  goto if_merge_1522;

  if_else_1521:
  r16 = cn_var_实例;
  r17 = 检查(r16, 18);
  if (r17) goto if_then_1523; else goto if_merge_1524;

  if_merge_1522:
  r19 = cn_var_实例;
  r20 = r19->当前词元;
  r21 = r20.值;
  r22 = cn_var_实例;
  r23 = 期望(r22, 40);
  goto while_cond_1525;

  if_then_1523:
  r18 = cn_var_实例;
  前进词元(r18);
  goto if_merge_1524;

  if_merge_1524:
  goto if_merge_1522;

  while_cond_1525:
  r24 = cn_var_实例;
  r25 = 检查(r24, 73);
  if (r25) goto while_body_1526; else goto while_exit_1527;

  while_body_1526:
  r26 = cn_var_实例;
  前进词元(r26);
  r27 = cn_var_节点_0;
  r28 = r27->变量声明;
  r29 = r28.数组维度;
  r30 = r29 + 1;
  r31 = cn_var_实例;
  r32 = 检查(r31, 41);
  if (r32) goto if_then_1528; else goto if_merge_1529;

  while_exit_1527:
  r42 = cn_var_实例;
  r43 = 匹配(r42, 51);
  if (r43) goto if_then_1530; else goto if_merge_1531;

  if_then_1528:
  r33 = cn_var_节点_0;
  r34 = r33->变量声明;
  r35 = r34.数组大小;
  r36 = cn_var_实例;
  r37 = r36->当前词元;
  r38 = r37.值;
  字符串转整数(r38);
  数组大小列表添加(r35, /* NONE */);
  r39 = cn_var_实例;
  前进词元(r39);
  goto if_merge_1529;

  if_merge_1529:
  r40 = cn_var_实例;
  r41 = 期望(r40, 74);
  goto while_cond_1525;

  if_then_1530:
  r44 = cn_var_实例;
  r45 = 解析表达式(r44);
  goto if_merge_1531;

  if_merge_1531:
  r46 = cn_var_实例;
  r47 = 期望(r46, 75);
  r48 = cn_var_节点_0;
  return r48;
}

struct 声明节点* 解析结构体声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r14, r18;
  char* r7;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 声明节点* r16;
  struct 声明节点* r17;
  struct 声明节点* r19;
  struct 声明节点* r20;
  struct 解析器* r21;
  struct 声明节点* r23;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r22;
  struct 词元 r6;

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
  r9 = 期望(r8, 40);
  r10 = cn_var_实例;
  r11 = 期望(r10, 71);
  goto while_cond_1536;

  while_cond_1536:
  r12 = cn_var_实例;
  r13 = 检查(r12, 72);
  r14 = !r13;
  if (r14) goto while_body_1537; else goto while_exit_1538;

  while_body_1537:
  struct 声明节点* cn_var_成员_1;
  r15 = cn_var_实例;
  r16 = 解析变量声明(r15);
  cn_var_成员_1 = r16;
  r17 = cn_var_成员_1;
  r18 = r17 != 0;
  if (r18) goto if_then_1539; else goto if_merge_1540;

  while_exit_1538:
  r21 = cn_var_实例;
  r22 = 期望(r21, 72);
  r23 = cn_var_节点_0;
  return r23;

  if_then_1539:
  r19 = cn_var_节点_0;
  r20 = cn_var_成员_1;
  结构体添加成员(r19, r20);
  goto if_merge_1540;

  if_merge_1540:
  goto while_cond_1536;
  return NULL;
}

struct 声明节点* 解析枚举声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r14, r16, r28, r31, r32, r33;
  char* r7;
  char* r19;
  char* r26;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 枚举成员* r15;
  struct 解析器* r17;
  struct 解析器* r20;
  struct 解析器* r22;
  struct 解析器* r24;
  struct 枚举成员* r27;
  struct 解析器* r29;
  struct 声明节点* r34;
  struct 枚举成员* r35;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 声明节点* r40;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r21;
  _Bool r23;
  _Bool r30;
  _Bool r37;
  _Bool r39;
  struct 词元 r6;
  struct 词元 r18;
  struct 词元 r25;

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
  r9 = 期望(r8, 40);
  r10 = cn_var_实例;
  r11 = 期望(r10, 71);
  long long cn_var_当前值_1;
  cn_var_当前值_1 = 0;
  goto while_cond_1545;

  while_cond_1545:
  r12 = cn_var_实例;
  r13 = 检查(r12, 72);
  r14 = !r13;
  if (r14) goto while_body_1546; else goto while_exit_1547;

  while_body_1546:
  struct 枚举成员* cn_var_成员_2;
  创建枚举成员();
  cn_var_成员_2 = /* NONE */;
  r15 = cn_var_成员_2;
  r16 = r15 == 0;
  if (r16) goto if_then_1548; else goto if_merge_1549;

  while_exit_1547:
  r38 = cn_var_实例;
  r39 = 期望(r38, 72);
  r40 = cn_var_节点_0;
  return r40;

  if_then_1548:
  goto while_exit_1547;
  goto if_merge_1549;

  if_merge_1549:
  r17 = cn_var_实例;
  r18 = r17->当前词元;
  r19 = r18.值;
  r20 = cn_var_实例;
  r21 = 期望(r20, 40);
  r22 = cn_var_实例;
  r23 = 匹配(r22, 51);
  if (r23) goto if_then_1550; else goto if_else_1551;

  if_then_1550:
  r24 = cn_var_实例;
  r25 = r24->当前词元;
  r26 = r25.值;
  字符串转整数(r26);
  r27 = cn_var_成员_2;
  r28 = r27->值;
  cn_var_当前值_1 = r28;
  r29 = cn_var_实例;
  r30 = 期望(r29, 41);
  goto if_merge_1552;

  if_else_1551:
  r31 = cn_var_当前值_1;
  goto if_merge_1552;

  if_merge_1552:
  r32 = cn_var_当前值_1;
  r33 = r32 + 1;
  cn_var_当前值_1 = r33;
  r34 = cn_var_节点_0;
  r35 = cn_var_成员_2;
  枚举添加成员(r34, r35);
  r36 = cn_var_实例;
  r37 = 匹配(r36, 76);
  goto while_cond_1545;
  return NULL;
}

struct 声明节点* 解析类声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r33, r38, r46, r53;
  char* r11;
  char* r18;
  char* r26;
  char* r30;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r6;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r12;
  struct 解析器* r14;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 解析器* r21;
  struct 解析器* r23;
  struct 解析器* r24;
  struct 解析器* r27;
  struct 声明节点* r29;
  struct 解析器* r31;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r39;
  struct 解析器* r41;
  struct 解析器* r43;
  struct 声明节点* r44;
  struct 声明节点* r45;
  struct 声明节点* r48;
  struct 声明节点* r49;
  struct 解析器* r50;
  struct 声明节点* r51;
  struct 声明节点* r52;
  struct 声明节点* r55;
  struct 声明节点* r56;
  struct 解析器* r57;
  struct 声明节点* r59;
  _Bool r5;
  _Bool r8;
  _Bool r13;
  _Bool r15;
  _Bool r20;
  _Bool r22;
  _Bool r28;
  _Bool r32;
  _Bool r35;
  _Bool r37;
  _Bool r42;
  _Bool r58;
  struct 词元 r10;
  struct 词元 r17;
  struct 词元 r25;
  struct 可见性 r40;
  struct 可见性 r47;
  struct 可见性 r54;

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
  r5 = 检查(r4, 32);
  if (r5) goto if_then_1557; else goto if_merge_1558;

  if_then_1557:
  r6 = cn_var_实例;
  前进词元(r6);
  goto if_merge_1558;

  if_merge_1558:
  r7 = cn_var_实例;
  r8 = 期望(r7, 27);
  r9 = cn_var_实例;
  r10 = r9->当前词元;
  r11 = r10.值;
  r12 = cn_var_实例;
  r13 = 期望(r12, 40);
  r14 = cn_var_实例;
  r15 = 匹配(r14, 78);
  if (r15) goto if_then_1559; else goto if_merge_1560;

  if_then_1559:
  r16 = cn_var_实例;
  r17 = r16->当前词元;
  r18 = r17.值;
  r19 = cn_var_实例;
  r20 = 期望(r19, 40);
  goto if_merge_1560;

  if_merge_1560:
  r21 = cn_var_实例;
  r22 = 检查(r21, 33);
  if (r22) goto if_then_1561; else goto if_merge_1562;

  if_then_1561:
  r23 = cn_var_实例;
  前进词元(r23);
  goto while_cond_1563;

  if_merge_1562:
  r34 = cn_var_实例;
  r35 = 期望(r34, 71);
  goto while_cond_1568;

  while_cond_1563:
  if (1) goto while_body_1564; else goto while_exit_1565;

  while_body_1564:
  char* cn_var_接口名_1;
  r24 = cn_var_实例;
  r25 = r24->当前词元;
  r26 = r25.值;
  cn_var_接口名_1 = r26;
  r27 = cn_var_实例;
  r28 = 期望(r27, 40);
  r29 = cn_var_节点_0;
  r30 = cn_var_接口名_1;
  类添加实现接口(r29, r30);
  r31 = cn_var_实例;
  r32 = 匹配(r31, 76);
  r33 = !r32;
  if (r33) goto if_then_1566; else goto if_merge_1567;

  while_exit_1565:
  goto if_merge_1562;

  if_then_1566:
  goto while_exit_1565;
  goto if_merge_1567;

  if_merge_1567:
  goto while_cond_1563;

  while_cond_1568:
  r36 = cn_var_实例;
  r37 = 检查(r36, 72);
  r38 = !r37;
  if (r38) goto while_body_1569; else goto while_exit_1570;

  while_body_1569:
  struct 可见性 cn_var_成员可见性_2;
  r39 = cn_var_实例;
  r40 = 解析可见性修饰符(r39);
  cn_var_成员可见性_2 = r40;
  r41 = cn_var_实例;
  r42 = 检查(r41, 17);
  if (r42) goto if_then_1571; else goto if_else_1572;

  while_exit_1570:
  r57 = cn_var_实例;
  r58 = 期望(r57, 72);
  r59 = cn_var_节点_0;
  return r59;

  if_then_1571:
  struct 声明节点* cn_var_方法_3;
  r43 = cn_var_实例;
  r44 = 解析函数声明(r43);
  cn_var_方法_3 = r44;
  r45 = cn_var_方法_3;
  r46 = r45 != 0;
  if (r46) goto if_then_1574; else goto if_merge_1575;

  if_else_1572:
  struct 声明节点* cn_var_字段_4;
  r50 = cn_var_实例;
  r51 = 解析变量声明(r50);
  cn_var_字段_4 = r51;
  r52 = cn_var_字段_4;
  r53 = r52 != 0;
  if (r53) goto if_then_1576; else goto if_merge_1577;

  if_merge_1573:
  goto while_cond_1568;

  if_then_1574:
  r47 = cn_var_成员可见性_2;
  r48 = cn_var_节点_0;
  r49 = cn_var_方法_3;
  类添加方法(r48, r49);
  goto if_merge_1575;

  if_merge_1575:
  goto if_merge_1573;

  if_then_1576:
  r54 = cn_var_成员可见性_2;
  r55 = cn_var_节点_0;
  r56 = cn_var_字段_4;
  类添加字段(r55, r56);
  goto if_merge_1577;

  if_merge_1577:
  goto if_merge_1573;
  return NULL;
}

struct 声明节点* 解析接口声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r14, r20;
  char* r7;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r15;
  struct 解析器* r17;
  struct 声明节点* r18;
  struct 声明节点* r19;
  struct 声明节点* r21;
  struct 声明节点* r22;
  struct 解析器* r23;
  struct 解析器* r24;
  struct 声明节点* r26;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r16;
  _Bool r25;
  struct 词元 r6;

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
  r9 = 期望(r8, 40);
  r10 = cn_var_实例;
  r11 = 期望(r10, 71);
  goto while_cond_1582;

  while_cond_1582:
  r12 = cn_var_实例;
  r13 = 检查(r12, 72);
  r14 = !r13;
  if (r14) goto while_body_1583; else goto while_exit_1584;

  while_body_1583:
  r15 = cn_var_实例;
  r16 = 检查(r15, 17);
  if (r16) goto if_then_1585; else goto if_else_1586;

  while_exit_1584:
  r24 = cn_var_实例;
  r25 = 期望(r24, 72);
  r26 = cn_var_节点_0;
  return r26;

  if_then_1585:
  struct 声明节点* cn_var_方法_1;
  r17 = cn_var_实例;
  r18 = 解析函数声明(r17);
  cn_var_方法_1 = r18;
  r19 = cn_var_方法_1;
  r20 = r19 != 0;
  if (r20) goto if_then_1588; else goto if_merge_1589;

  if_else_1586:
  r23 = cn_var_实例;
  前进词元(r23);
  goto if_merge_1587;

  if_merge_1587:
  goto while_cond_1582;

  if_then_1588:
  r21 = cn_var_节点_0;
  r22 = cn_var_方法_1;
  接口添加方法(r21, r22);
  goto if_merge_1589;

  if_merge_1589:
  goto if_merge_1587;
  return NULL;
}

struct 声明节点* 解析模块声明(struct 解析器* cn_var_实例) {
  long long r1, r3;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;

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
  报告错误(r5, 12, "模块关键字暂不支持，请使用文件即模块");
  return 0;
}

struct 声明节点* 解析导入声明(struct 解析器* cn_var_实例) {
  long long r1, r3, r15, r25;
  char* r7;
  char* r18;
  char* r22;
  char* r33;
  struct 解析器* r0;
  struct 声明节点* r2;
  struct 解析器* r4;
  struct 解析器* r5;
  struct 解析器* r8;
  struct 解析器* r10;
  struct 解析器* r12;
  struct 解析器* r13;
  struct 解析器* r16;
  struct 解析器* r19;
  struct 声明节点* r21;
  struct 解析器* r23;
  struct 解析器* r26;
  struct 解析器* r28;
  struct 解析器* r30;
  struct 解析器* r31;
  struct 解析器* r34;
  struct 解析器* r36;
  struct 解析器* r38;
  struct 声明节点* r40;
  _Bool r9;
  _Bool r11;
  _Bool r14;
  _Bool r20;
  _Bool r24;
  _Bool r27;
  _Bool r29;
  _Bool r35;
  _Bool r37;
  _Bool r39;
  struct 词元 r6;
  struct 词元 r17;
  struct 词元 r32;

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
  r9 = 期望(r8, 40);
  r10 = cn_var_实例;
  r11 = 检查(r10, 71);
  if (r11) goto if_then_1598; else goto if_merge_1599;

  if_then_1598:
  r12 = cn_var_实例;
  前进词元(r12);
  goto while_cond_1600;

  if_merge_1599:
  r28 = cn_var_实例;
  r29 = 检查(r28, 69);
  if (r29) goto if_then_1605; else goto if_merge_1606;

  while_cond_1600:
  r13 = cn_var_实例;
  r14 = 检查(r13, 72);
  r15 = !r14;
  if (r15) goto while_body_1601; else goto while_exit_1602;

  while_body_1601:
  char* cn_var_成员名_1;
  r16 = cn_var_实例;
  r17 = r16->当前词元;
  r18 = r17.值;
  cn_var_成员名_1 = r18;
  r19 = cn_var_实例;
  r20 = 期望(r19, 40);
  r21 = cn_var_节点_0;
  r22 = cn_var_成员名_1;
  导入添加成员(r21, r22);
  r23 = cn_var_实例;
  r24 = 匹配(r23, 76);
  r25 = !r24;
  if (r25) goto if_then_1603; else goto if_merge_1604;

  while_exit_1602:
  r26 = cn_var_实例;
  r27 = 期望(r26, 72);
  goto if_merge_1599;

  if_then_1603:
  goto while_exit_1602;
  goto if_merge_1604;

  if_merge_1604:
  goto while_cond_1600;

  if_then_1605:
  r30 = cn_var_实例;
  前进词元(r30);
  r31 = cn_var_实例;
  r32 = r31->当前词元;
  r33 = r32.值;
  r34 = cn_var_实例;
  r35 = 期望(r34, 40);
  r36 = cn_var_实例;
  r37 = 期望(r36, 70);
  goto if_merge_1606;

  if_merge_1606:
  r38 = cn_var_实例;
  r39 = 期望(r38, 75);
  r40 = cn_var_节点_0;
  return r40;
}

enum 可见性 解析可见性修饰符(struct 解析器* cn_var_实例) {
  long long r1, r2, r6, r10, r14, r15;
  struct 解析器* r0;
  struct 解析器* r3;
  struct 解析器* r5;
  struct 解析器* r7;
  struct 解析器* r9;
  struct 解析器* r11;
  struct 解析器* r13;
  _Bool r4;
  _Bool r8;
  _Bool r12;

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
  r4 = 检查(r3, 21);
  if (r4) goto if_then_1609; else goto if_merge_1610;

  if_then_1609:
  r5 = cn_var_实例;
  前进词元(r5);
  r6 = cn_var_公开可见;
  return r6;
  goto if_merge_1610;

  if_merge_1610:
  r7 = cn_var_实例;
  r8 = 检查(r7, 22);
  if (r8) goto if_then_1611; else goto if_merge_1612;

  if_then_1611:
  r9 = cn_var_实例;
  前进词元(r9);
  r10 = cn_var_私有可见;
  return r10;
  goto if_merge_1612;

  if_merge_1612:
  r11 = cn_var_实例;
  r12 = 检查(r11, 29);
  if (r12) goto if_then_1613; else goto if_merge_1614;

  if_then_1613:
  r13 = cn_var_实例;
  前进词元(r13);
  r14 = cn_var_默认可见;
  return r14;
  goto if_merge_1614;

  if_merge_1614:
  r15 = cn_var_私有可见;
  return r15;
}

