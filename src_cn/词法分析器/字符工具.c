#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// Global Variables

// Forward Declarations
_Bool 是空白字符(long long);
_Bool 是换行符(long long);
_Bool 是数字(long long);
_Bool 是十六进制数字(long long);
_Bool 是二进制数字(long long);
_Bool 是八进制数字(long long);
_Bool 是字母(long long);
_Bool 是字母或数字(long long);
_Bool 是中文字符(long long);
_Bool 是标识符开头(long long);
_Bool 是标识符字符(long long);
_Bool 是运算符字符(long long);
_Bool 是分隔符字符(long long);
long long 转小写(long long);
long long 转大写(long long);
_Bool 是可打印字符(long long);
_Bool 是十六进制前缀(long long, long long);
_Bool 是二进制前缀(long long, long long);
_Bool 是八进制前缀(long long, long long);
_Bool 是单行注释开始(long long, long long);
_Bool 是块注释开始(long long, long long);
_Bool 是块注释结束(long long, long long);

_Bool 是空白字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 32;
  if (r1) goto if_then_483; else goto if_merge_484;

  if_then_483:
  return 1;
  goto if_merge_484;

  if_merge_484:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_485; else goto if_merge_486;

  if_then_485:
  return 1;
  goto if_merge_486;

  if_merge_486:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_487; else goto if_merge_488;

  if_then_487:
  return 1;
  goto if_merge_488;

  if_merge_488:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_489; else goto if_merge_490;

  if_then_489:
  return 1;
  goto if_merge_490;

  if_merge_490:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_491; else goto if_merge_492;

  if_then_491:
  return 1;
  goto if_merge_492;

  if_merge_492:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_493; else goto if_merge_494;

  if_then_493:
  return 1;
  goto if_merge_494;

  if_merge_494:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_497; else goto logic_merge_498;

  if_then_495:
  return 1;
  goto if_merge_496;

  if_merge_496:
  return 0;

  logic_rhs_497:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_498;

  logic_merge_498:
  if (r4) goto if_then_495; else goto if_merge_496;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_501; else goto logic_merge_502;

  if_then_499:
  return 1;
  goto if_merge_500;

  if_merge_500:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_505; else goto logic_merge_506;

  logic_rhs_501:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_502;

  logic_merge_502:
  if (r4) goto if_then_499; else goto if_merge_500;

  if_then_503:
  return 1;
  goto if_merge_504;

  if_merge_504:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_509; else goto logic_merge_510;

  logic_rhs_505:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_506;

  logic_merge_506:
  if (r9) goto if_then_503; else goto if_merge_504;

  if_then_507:
  return 1;
  goto if_merge_508;

  if_merge_508:
  return 0;

  logic_rhs_509:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_510;

  logic_merge_510:
  if (r14) goto if_then_507; else goto if_merge_508;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_514; else goto logic_rhs_513;

  if_then_511:
  return 1;
  goto if_merge_512;

  if_merge_512:
  return 0;

  logic_rhs_513:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_514;

  logic_merge_514:
  if (r4) goto if_then_511; else goto if_merge_512;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_517; else goto logic_merge_518;

  if_then_515:
  return 1;
  goto if_merge_516;

  if_merge_516:
  return 0;

  logic_rhs_517:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_518;

  logic_merge_518:
  if (r4) goto if_then_515; else goto if_merge_516;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_521; else goto logic_merge_522;

  if_then_519:
  return 1;
  goto if_merge_520;

  if_merge_520:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_525; else goto logic_merge_526;

  logic_rhs_521:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_522;

  logic_merge_522:
  if (r4) goto if_then_519; else goto if_merge_520;

  if_then_523:
  return 1;
  goto if_merge_524;

  if_merge_524:
  return 0;

  logic_rhs_525:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_526;

  logic_merge_526:
  if (r9) goto if_then_523; else goto if_merge_524;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_527; else goto if_merge_528;

  if_then_527:
  return 1;
  goto if_merge_528;

  if_merge_528:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_529; else goto if_merge_530;

  if_then_529:
  return 1;
  goto if_merge_530;

  if_merge_530:
  return 0;
}

_Bool 是中文字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  long long cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 228;
  if (r3) goto logic_rhs_533; else goto logic_merge_534;

  if_then_531:
  return 1;
  goto if_merge_532;

  if_merge_532:
  return 0;

  logic_rhs_533:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_534;

  logic_merge_534:
  if (r5) goto if_then_531; else goto if_merge_532;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_535; else goto if_merge_536;

  if_then_535:
  return 1;
  goto if_merge_536;

  if_merge_536:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_537; else goto if_merge_538;

  if_then_537:
  return 1;
  goto if_merge_538;

  if_merge_538:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_539; else goto if_merge_540;

  if_then_539:
  return 1;
  goto if_merge_540;

  if_merge_540:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_541; else goto if_merge_542;

  if_then_541:
  return 1;
  goto if_merge_542;

  if_merge_542:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_543; else goto if_merge_544;

  if_then_543:
  return 1;
  goto if_merge_544;

  if_merge_544:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_545; else goto if_merge_546;

  if_then_545:
  return 1;
  goto if_merge_546;

  if_merge_546:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_547; else goto if_merge_548;

  if_then_547:
  return 1;
  goto if_merge_548;

  if_merge_548:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_549; else goto if_merge_550;

  if_then_549:
  return 1;
  goto if_merge_550;

  if_merge_550:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_551; else goto if_merge_552;

  if_then_551:
  return 1;
  goto if_merge_552;

  if_merge_552:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_553; else goto if_merge_554;

  if_then_553:
  return 1;
  goto if_merge_554;

  if_merge_554:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_555; else goto if_merge_556;

  if_then_555:
  return 1;
  goto if_merge_556;

  if_merge_556:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_557; else goto if_merge_558;

  if_then_557:
  return 1;
  goto if_merge_558;

  if_merge_558:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_559; else goto if_merge_560;

  if_then_559:
  return 1;
  goto if_merge_560;

  if_merge_560:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_561; else goto if_merge_562;

  if_then_561:
  return 1;
  goto if_merge_562;

  if_merge_562:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_563; else goto if_merge_564;

  if_then_563:
  return 1;
  goto if_merge_564;

  if_merge_564:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_565; else goto if_merge_566;

  if_then_565:
  return 1;
  goto if_merge_566;

  if_merge_566:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_567; else goto if_merge_568;

  if_then_567:
  return 1;
  goto if_merge_568;

  if_merge_568:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_569; else goto if_merge_570;

  if_then_569:
  return 1;
  goto if_merge_570;

  if_merge_570:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_571; else goto if_merge_572;

  if_then_571:
  return 1;
  goto if_merge_572;

  if_merge_572:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_573; else goto if_merge_574;

  if_then_573:
  return 1;
  goto if_merge_574;

  if_merge_574:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_575; else goto if_merge_576;

  if_then_575:
  return 1;
  goto if_merge_576;

  if_merge_576:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_577; else goto if_merge_578;

  if_then_577:
  return 1;
  goto if_merge_578;

  if_merge_578:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_579; else goto if_merge_580;

  if_then_579:
  return 1;
  goto if_merge_580;

  if_merge_580:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_581; else goto if_merge_582;

  if_then_581:
  return 1;
  goto if_merge_582;

  if_merge_582:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_583; else goto if_merge_584;

  if_then_583:
  return 1;
  goto if_merge_584;

  if_merge_584:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_585; else goto if_merge_586;

  if_then_585:
  return 1;
  goto if_merge_586;

  if_merge_586:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_587; else goto if_merge_588;

  if_then_587:
  return 1;
  goto if_merge_588;

  if_merge_588:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_589; else goto if_merge_590;

  if_then_589:
  return 1;
  goto if_merge_590;

  if_merge_590:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_591; else goto if_merge_592;

  if_then_591:
  return 1;
  goto if_merge_592;

  if_merge_592:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_593; else goto if_merge_594;

  if_then_593:
  return 1;
  goto if_merge_594;

  if_merge_594:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_597; else goto logic_merge_598;

  if_then_595:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_596;

  if_merge_596:
  r8 = cn_var_c;
  return r8;

  logic_rhs_597:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_598;

  logic_merge_598:
  if (r4) goto if_then_595; else goto if_merge_596;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_601; else goto logic_merge_602;

  if_then_599:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_600;

  if_merge_600:
  r8 = cn_var_c;
  return r8;

  logic_rhs_601:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_602;

  logic_merge_602:
  if (r4) goto if_then_599; else goto if_merge_600;
  return 0;
}

_Bool 是可打印字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  long long cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 32;
  if (r3) goto logic_rhs_605; else goto logic_merge_606;

  if_then_603:
  return 1;
  goto if_merge_604;

  if_merge_604:
  return 0;

  logic_rhs_605:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_606;

  logic_merge_606:
  if (r5) goto if_then_603; else goto if_merge_604;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_609; else goto logic_merge_610;

  if_then_607:
  return 1;
  goto if_merge_608;

  if_merge_608:
  return 0;

  logic_rhs_609:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_612; else goto logic_rhs_611;

  logic_merge_610:
  if (r7) goto if_then_607; else goto if_merge_608;

  logic_rhs_611:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_612;

  logic_merge_612:
  goto logic_merge_610;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_615; else goto logic_merge_616;

  if_then_613:
  return 1;
  goto if_merge_614;

  if_merge_614:
  return 0;

  logic_rhs_615:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_618; else goto logic_rhs_617;

  logic_merge_616:
  if (r7) goto if_then_613; else goto if_merge_614;

  logic_rhs_617:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_618;

  logic_merge_618:
  goto logic_merge_616;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_621; else goto logic_merge_622;

  if_then_619:
  return 1;
  goto if_merge_620;

  if_merge_620:
  return 0;

  logic_rhs_621:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_624; else goto logic_rhs_623;

  logic_merge_622:
  if (r7) goto if_then_619; else goto if_merge_620;

  logic_rhs_623:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_624;

  logic_merge_624:
  goto logic_merge_622;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_627; else goto logic_merge_628;

  if_then_625:
  return 1;
  goto if_merge_626;

  if_merge_626:
  return 0;

  logic_rhs_627:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_628;

  logic_merge_628:
  if (r4) goto if_then_625; else goto if_merge_626;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_631; else goto logic_merge_632;

  if_then_629:
  return 1;
  goto if_merge_630;

  if_merge_630:
  return 0;

  logic_rhs_631:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_632;

  logic_merge_632:
  if (r4) goto if_then_629; else goto if_merge_630;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_635; else goto logic_merge_636;

  if_then_633:
  return 1;
  goto if_merge_634;

  if_merge_634:
  return 0;

  logic_rhs_635:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_636;

  logic_merge_636:
  if (r4) goto if_then_633; else goto if_merge_634;
  return 0;
}

