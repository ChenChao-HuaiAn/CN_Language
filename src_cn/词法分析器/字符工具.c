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
  if (r1) goto if_then_425; else goto if_merge_426;

  if_then_425:
  return 1;
  goto if_merge_426;

  if_merge_426:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_427; else goto if_merge_428;

  if_then_427:
  return 1;
  goto if_merge_428;

  if_merge_428:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_429; else goto if_merge_430;

  if_then_429:
  return 1;
  goto if_merge_430;

  if_merge_430:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_431; else goto if_merge_432;

  if_then_431:
  return 1;
  goto if_merge_432;

  if_merge_432:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_433; else goto if_merge_434;

  if_then_433:
  return 1;
  goto if_merge_434;

  if_merge_434:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_435; else goto if_merge_436;

  if_then_435:
  return 1;
  goto if_merge_436;

  if_merge_436:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_439; else goto logic_merge_440;

  if_then_437:
  return 1;
  goto if_merge_438;

  if_merge_438:
  return 0;

  logic_rhs_439:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_440;

  logic_merge_440:
  if (r4) goto if_then_437; else goto if_merge_438;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_443; else goto logic_merge_444;

  if_then_441:
  return 1;
  goto if_merge_442;

  if_merge_442:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_447; else goto logic_merge_448;

  logic_rhs_443:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_444;

  logic_merge_444:
  if (r4) goto if_then_441; else goto if_merge_442;

  if_then_445:
  return 1;
  goto if_merge_446;

  if_merge_446:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_451; else goto logic_merge_452;

  logic_rhs_447:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_448;

  logic_merge_448:
  if (r9) goto if_then_445; else goto if_merge_446;

  if_then_449:
  return 1;
  goto if_merge_450;

  if_merge_450:
  return 0;

  logic_rhs_451:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_452;

  logic_merge_452:
  if (r14) goto if_then_449; else goto if_merge_450;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_456; else goto logic_rhs_455;

  if_then_453:
  return 1;
  goto if_merge_454;

  if_merge_454:
  return 0;

  logic_rhs_455:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_456;

  logic_merge_456:
  if (r4) goto if_then_453; else goto if_merge_454;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_459; else goto logic_merge_460;

  if_then_457:
  return 1;
  goto if_merge_458;

  if_merge_458:
  return 0;

  logic_rhs_459:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_460;

  logic_merge_460:
  if (r4) goto if_then_457; else goto if_merge_458;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_463; else goto logic_merge_464;

  if_then_461:
  return 1;
  goto if_merge_462;

  if_merge_462:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_467; else goto logic_merge_468;

  logic_rhs_463:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_464;

  logic_merge_464:
  if (r4) goto if_then_461; else goto if_merge_462;

  if_then_465:
  return 1;
  goto if_merge_466;

  if_merge_466:
  return 0;

  logic_rhs_467:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_468;

  logic_merge_468:
  if (r9) goto if_then_465; else goto if_merge_466;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_469; else goto if_merge_470;

  if_then_469:
  return 1;
  goto if_merge_470;

  if_merge_470:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_471; else goto if_merge_472;

  if_then_471:
  return 1;
  goto if_merge_472;

  if_merge_472:
  return 0;
}

_Bool 是中文字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  void cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 228;
  if (r3) goto logic_rhs_475; else goto logic_merge_476;

  if_then_473:
  return 1;
  goto if_merge_474;

  if_merge_474:
  return 0;

  logic_rhs_475:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_476;

  logic_merge_476:
  if (r5) goto if_then_473; else goto if_merge_474;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_477; else goto if_merge_478;

  if_then_477:
  return 1;
  goto if_merge_478;

  if_merge_478:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_479; else goto if_merge_480;

  if_then_479:
  return 1;
  goto if_merge_480;

  if_merge_480:
  void cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_481; else goto if_merge_482;

  if_then_481:
  return 1;
  goto if_merge_482;

  if_merge_482:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_483; else goto if_merge_484;

  if_then_483:
  return 1;
  goto if_merge_484;

  if_merge_484:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_485; else goto if_merge_486;

  if_then_485:
  return 1;
  goto if_merge_486;

  if_merge_486:
  void cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_487; else goto if_merge_488;

  if_then_487:
  return 1;
  goto if_merge_488;

  if_merge_488:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_489; else goto if_merge_490;

  if_then_489:
  return 1;
  goto if_merge_490;

  if_merge_490:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_491; else goto if_merge_492;

  if_then_491:
  return 1;
  goto if_merge_492;

  if_merge_492:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_493; else goto if_merge_494;

  if_then_493:
  return 1;
  goto if_merge_494;

  if_merge_494:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_495; else goto if_merge_496;

  if_then_495:
  return 1;
  goto if_merge_496;

  if_merge_496:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_497; else goto if_merge_498;

  if_then_497:
  return 1;
  goto if_merge_498;

  if_merge_498:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_499; else goto if_merge_500;

  if_then_499:
  return 1;
  goto if_merge_500;

  if_merge_500:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_501; else goto if_merge_502;

  if_then_501:
  return 1;
  goto if_merge_502;

  if_merge_502:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_503; else goto if_merge_504;

  if_then_503:
  return 1;
  goto if_merge_504;

  if_merge_504:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_505; else goto if_merge_506;

  if_then_505:
  return 1;
  goto if_merge_506;

  if_merge_506:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_507; else goto if_merge_508;

  if_then_507:
  return 1;
  goto if_merge_508;

  if_merge_508:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_509; else goto if_merge_510;

  if_then_509:
  return 1;
  goto if_merge_510;

  if_merge_510:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_511; else goto if_merge_512;

  if_then_511:
  return 1;
  goto if_merge_512;

  if_merge_512:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_513; else goto if_merge_514;

  if_then_513:
  return 1;
  goto if_merge_514;

  if_merge_514:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_515; else goto if_merge_516;

  if_then_515:
  return 1;
  goto if_merge_516;

  if_merge_516:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_517; else goto if_merge_518;

  if_then_517:
  return 1;
  goto if_merge_518;

  if_merge_518:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_519; else goto if_merge_520;

  if_then_519:
  return 1;
  goto if_merge_520;

  if_merge_520:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_521; else goto if_merge_522;

  if_then_521:
  return 1;
  goto if_merge_522;

  if_merge_522:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_523; else goto if_merge_524;

  if_then_523:
  return 1;
  goto if_merge_524;

  if_merge_524:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_525; else goto if_merge_526;

  if_then_525:
  return 1;
  goto if_merge_526;

  if_merge_526:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_527; else goto if_merge_528;

  if_then_527:
  return 1;
  goto if_merge_528;

  if_merge_528:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_529; else goto if_merge_530;

  if_then_529:
  return 1;
  goto if_merge_530;

  if_merge_530:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_531; else goto if_merge_532;

  if_then_531:
  return 1;
  goto if_merge_532;

  if_merge_532:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_533; else goto if_merge_534;

  if_then_533:
  return 1;
  goto if_merge_534;

  if_merge_534:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_535; else goto if_merge_536;

  if_then_535:
  return 1;
  goto if_merge_536;

  if_merge_536:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_539; else goto logic_merge_540;

  if_then_537:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_538;

  if_merge_538:
  r8 = cn_var_c;
  return r8;

  logic_rhs_539:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_540;

  logic_merge_540:
  if (r4) goto if_then_537; else goto if_merge_538;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_543; else goto logic_merge_544;

  if_then_541:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_542;

  if_merge_542:
  r8 = cn_var_c;
  return r8;

  logic_rhs_543:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_544;

  logic_merge_544:
  if (r4) goto if_then_541; else goto if_merge_542;
  return 0;
}

_Bool 是可打印字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  void cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 32;
  if (r3) goto logic_rhs_547; else goto logic_merge_548;

  if_then_545:
  return 1;
  goto if_merge_546;

  if_merge_546:
  return 0;

  logic_rhs_547:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_548;

  logic_merge_548:
  if (r5) goto if_then_545; else goto if_merge_546;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_551; else goto logic_merge_552;

  if_then_549:
  return 1;
  goto if_merge_550;

  if_merge_550:
  return 0;

  logic_rhs_551:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_554; else goto logic_rhs_553;

  logic_merge_552:
  if (r7) goto if_then_549; else goto if_merge_550;

  logic_rhs_553:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_554;

  logic_merge_554:
  goto logic_merge_552;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_557; else goto logic_merge_558;

  if_then_555:
  return 1;
  goto if_merge_556;

  if_merge_556:
  return 0;

  logic_rhs_557:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_560; else goto logic_rhs_559;

  logic_merge_558:
  if (r7) goto if_then_555; else goto if_merge_556;

  logic_rhs_559:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_560;

  logic_merge_560:
  goto logic_merge_558;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_563; else goto logic_merge_564;

  if_then_561:
  return 1;
  goto if_merge_562;

  if_merge_562:
  return 0;

  logic_rhs_563:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_566; else goto logic_rhs_565;

  logic_merge_564:
  if (r7) goto if_then_561; else goto if_merge_562;

  logic_rhs_565:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_566;

  logic_merge_566:
  goto logic_merge_564;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_569; else goto logic_merge_570;

  if_then_567:
  return 1;
  goto if_merge_568;

  if_merge_568:
  return 0;

  logic_rhs_569:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_570;

  logic_merge_570:
  if (r4) goto if_then_567; else goto if_merge_568;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_573; else goto logic_merge_574;

  if_then_571:
  return 1;
  goto if_merge_572;

  if_merge_572:
  return 0;

  logic_rhs_573:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_574;

  logic_merge_574:
  if (r4) goto if_then_571; else goto if_merge_572;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_577; else goto logic_merge_578;

  if_then_575:
  return 1;
  goto if_merge_576;

  if_merge_576:
  return 0;

  logic_rhs_577:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_578;

  logic_merge_578:
  if (r4) goto if_then_575; else goto if_merge_576;
  return 0;
}

