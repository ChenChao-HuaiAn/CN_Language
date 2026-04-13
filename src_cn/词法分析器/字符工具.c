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
  if (r1) goto if_then_448; else goto if_merge_449;

  if_then_448:
  return 1;
  goto if_merge_449;

  if_merge_449:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_450; else goto if_merge_451;

  if_then_450:
  return 1;
  goto if_merge_451;

  if_merge_451:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_452; else goto if_merge_453;

  if_then_452:
  return 1;
  goto if_merge_453;

  if_merge_453:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_454; else goto if_merge_455;

  if_then_454:
  return 1;
  goto if_merge_455;

  if_merge_455:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_456; else goto if_merge_457;

  if_then_456:
  return 1;
  goto if_merge_457;

  if_merge_457:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_458; else goto if_merge_459;

  if_then_458:
  return 1;
  goto if_merge_459;

  if_merge_459:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_462; else goto logic_merge_463;

  if_then_460:
  return 1;
  goto if_merge_461;

  if_merge_461:
  return 0;

  logic_rhs_462:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_463;

  logic_merge_463:
  if (r4) goto if_then_460; else goto if_merge_461;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_466; else goto logic_merge_467;

  if_then_464:
  return 1;
  goto if_merge_465;

  if_merge_465:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_470; else goto logic_merge_471;

  logic_rhs_466:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_467;

  logic_merge_467:
  if (r4) goto if_then_464; else goto if_merge_465;

  if_then_468:
  return 1;
  goto if_merge_469;

  if_merge_469:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_474; else goto logic_merge_475;

  logic_rhs_470:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_471;

  logic_merge_471:
  if (r9) goto if_then_468; else goto if_merge_469;

  if_then_472:
  return 1;
  goto if_merge_473;

  if_merge_473:
  return 0;

  logic_rhs_474:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_475;

  logic_merge_475:
  if (r14) goto if_then_472; else goto if_merge_473;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_479; else goto logic_rhs_478;

  if_then_476:
  return 1;
  goto if_merge_477;

  if_merge_477:
  return 0;

  logic_rhs_478:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_479;

  logic_merge_479:
  if (r4) goto if_then_476; else goto if_merge_477;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_482; else goto logic_merge_483;

  if_then_480:
  return 1;
  goto if_merge_481;

  if_merge_481:
  return 0;

  logic_rhs_482:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_483;

  logic_merge_483:
  if (r4) goto if_then_480; else goto if_merge_481;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_486; else goto logic_merge_487;

  if_then_484:
  return 1;
  goto if_merge_485;

  if_merge_485:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_490; else goto logic_merge_491;

  logic_rhs_486:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_487;

  logic_merge_487:
  if (r4) goto if_then_484; else goto if_merge_485;

  if_then_488:
  return 1;
  goto if_merge_489;

  if_merge_489:
  return 0;

  logic_rhs_490:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_491;

  logic_merge_491:
  if (r9) goto if_then_488; else goto if_merge_489;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_492; else goto if_merge_493;

  if_then_492:
  return 1;
  goto if_merge_493;

  if_merge_493:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_494; else goto if_merge_495;

  if_then_494:
  return 1;
  goto if_merge_495;

  if_merge_495:
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
  if (r3) goto logic_rhs_498; else goto logic_merge_499;

  if_then_496:
  return 1;
  goto if_merge_497;

  if_merge_497:
  return 0;

  logic_rhs_498:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_499;

  logic_merge_499:
  if (r5) goto if_then_496; else goto if_merge_497;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_500; else goto if_merge_501;

  if_then_500:
  return 1;
  goto if_merge_501;

  if_merge_501:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_502; else goto if_merge_503;

  if_then_502:
  return 1;
  goto if_merge_503;

  if_merge_503:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_504; else goto if_merge_505;

  if_then_504:
  return 1;
  goto if_merge_505;

  if_merge_505:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_506; else goto if_merge_507;

  if_then_506:
  return 1;
  goto if_merge_507;

  if_merge_507:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_508; else goto if_merge_509;

  if_then_508:
  return 1;
  goto if_merge_509;

  if_merge_509:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_510; else goto if_merge_511;

  if_then_510:
  return 1;
  goto if_merge_511;

  if_merge_511:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_512; else goto if_merge_513;

  if_then_512:
  return 1;
  goto if_merge_513;

  if_merge_513:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_514; else goto if_merge_515;

  if_then_514:
  return 1;
  goto if_merge_515;

  if_merge_515:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_516; else goto if_merge_517;

  if_then_516:
  return 1;
  goto if_merge_517;

  if_merge_517:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_518; else goto if_merge_519;

  if_then_518:
  return 1;
  goto if_merge_519;

  if_merge_519:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_520; else goto if_merge_521;

  if_then_520:
  return 1;
  goto if_merge_521;

  if_merge_521:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_522; else goto if_merge_523;

  if_then_522:
  return 1;
  goto if_merge_523;

  if_merge_523:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_524; else goto if_merge_525;

  if_then_524:
  return 1;
  goto if_merge_525;

  if_merge_525:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_526; else goto if_merge_527;

  if_then_526:
  return 1;
  goto if_merge_527;

  if_merge_527:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_528; else goto if_merge_529;

  if_then_528:
  return 1;
  goto if_merge_529;

  if_merge_529:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_530; else goto if_merge_531;

  if_then_530:
  return 1;
  goto if_merge_531;

  if_merge_531:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_532; else goto if_merge_533;

  if_then_532:
  return 1;
  goto if_merge_533;

  if_merge_533:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_534; else goto if_merge_535;

  if_then_534:
  return 1;
  goto if_merge_535;

  if_merge_535:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_536; else goto if_merge_537;

  if_then_536:
  return 1;
  goto if_merge_537;

  if_merge_537:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_538; else goto if_merge_539;

  if_then_538:
  return 1;
  goto if_merge_539;

  if_merge_539:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_540; else goto if_merge_541;

  if_then_540:
  return 1;
  goto if_merge_541;

  if_merge_541:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_542; else goto if_merge_543;

  if_then_542:
  return 1;
  goto if_merge_543;

  if_merge_543:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_544; else goto if_merge_545;

  if_then_544:
  return 1;
  goto if_merge_545;

  if_merge_545:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_546; else goto if_merge_547;

  if_then_546:
  return 1;
  goto if_merge_547;

  if_merge_547:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_548; else goto if_merge_549;

  if_then_548:
  return 1;
  goto if_merge_549;

  if_merge_549:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_550; else goto if_merge_551;

  if_then_550:
  return 1;
  goto if_merge_551;

  if_merge_551:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_552; else goto if_merge_553;

  if_then_552:
  return 1;
  goto if_merge_553;

  if_merge_553:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_554; else goto if_merge_555;

  if_then_554:
  return 1;
  goto if_merge_555;

  if_merge_555:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_556; else goto if_merge_557;

  if_then_556:
  return 1;
  goto if_merge_557;

  if_merge_557:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_558; else goto if_merge_559;

  if_then_558:
  return 1;
  goto if_merge_559;

  if_merge_559:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_562; else goto logic_merge_563;

  if_then_560:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_561;

  if_merge_561:
  r8 = cn_var_c;
  return r8;

  logic_rhs_562:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_563;

  logic_merge_563:
  if (r4) goto if_then_560; else goto if_merge_561;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_566; else goto logic_merge_567;

  if_then_564:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_565;

  if_merge_565:
  r8 = cn_var_c;
  return r8;

  logic_rhs_566:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_567;

  logic_merge_567:
  if (r4) goto if_then_564; else goto if_merge_565;
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
  if (r3) goto logic_rhs_570; else goto logic_merge_571;

  if_then_568:
  return 1;
  goto if_merge_569;

  if_merge_569:
  return 0;

  logic_rhs_570:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_571;

  logic_merge_571:
  if (r5) goto if_then_568; else goto if_merge_569;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_574; else goto logic_merge_575;

  if_then_572:
  return 1;
  goto if_merge_573;

  if_merge_573:
  return 0;

  logic_rhs_574:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_577; else goto logic_rhs_576;

  logic_merge_575:
  if (r7) goto if_then_572; else goto if_merge_573;

  logic_rhs_576:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_577;

  logic_merge_577:
  goto logic_merge_575;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_580; else goto logic_merge_581;

  if_then_578:
  return 1;
  goto if_merge_579;

  if_merge_579:
  return 0;

  logic_rhs_580:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_583; else goto logic_rhs_582;

  logic_merge_581:
  if (r7) goto if_then_578; else goto if_merge_579;

  logic_rhs_582:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_583;

  logic_merge_583:
  goto logic_merge_581;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_586; else goto logic_merge_587;

  if_then_584:
  return 1;
  goto if_merge_585;

  if_merge_585:
  return 0;

  logic_rhs_586:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_589; else goto logic_rhs_588;

  logic_merge_587:
  if (r7) goto if_then_584; else goto if_merge_585;

  logic_rhs_588:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_589;

  logic_merge_589:
  goto logic_merge_587;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_592; else goto logic_merge_593;

  if_then_590:
  return 1;
  goto if_merge_591;

  if_merge_591:
  return 0;

  logic_rhs_592:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_593;

  logic_merge_593:
  if (r4) goto if_then_590; else goto if_merge_591;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_596; else goto logic_merge_597;

  if_then_594:
  return 1;
  goto if_merge_595;

  if_merge_595:
  return 0;

  logic_rhs_596:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_597;

  logic_merge_597:
  if (r4) goto if_then_594; else goto if_merge_595;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_600; else goto logic_merge_601;

  if_then_598:
  return 1;
  goto if_merge_599;

  if_merge_599:
  return 0;

  logic_rhs_600:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_601;

  logic_merge_601:
  if (r4) goto if_then_598; else goto if_merge_599;
  return 0;
}

