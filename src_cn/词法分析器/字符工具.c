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
  if (r1) goto if_then_383; else goto if_merge_384;

  if_then_383:
  return 1;
  goto if_merge_384;

  if_merge_384:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_385; else goto if_merge_386;

  if_then_385:
  return 1;
  goto if_merge_386;

  if_merge_386:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_387; else goto if_merge_388;

  if_then_387:
  return 1;
  goto if_merge_388;

  if_merge_388:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_389; else goto if_merge_390;

  if_then_389:
  return 1;
  goto if_merge_390;

  if_merge_390:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_391; else goto if_merge_392;

  if_then_391:
  return 1;
  goto if_merge_392;

  if_merge_392:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_393; else goto if_merge_394;

  if_then_393:
  return 1;
  goto if_merge_394;

  if_merge_394:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_397; else goto logic_merge_398;

  if_then_395:
  return 1;
  goto if_merge_396;

  if_merge_396:
  return 0;

  logic_rhs_397:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_398;

  logic_merge_398:
  if (r4) goto if_then_395; else goto if_merge_396;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_401; else goto logic_merge_402;

  if_then_399:
  return 1;
  goto if_merge_400;

  if_merge_400:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_405; else goto logic_merge_406;

  logic_rhs_401:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_402;

  logic_merge_402:
  if (r4) goto if_then_399; else goto if_merge_400;

  if_then_403:
  return 1;
  goto if_merge_404;

  if_merge_404:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_409; else goto logic_merge_410;

  logic_rhs_405:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_406;

  logic_merge_406:
  if (r9) goto if_then_403; else goto if_merge_404;

  if_then_407:
  return 1;
  goto if_merge_408;

  if_merge_408:
  return 0;

  logic_rhs_409:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_410;

  logic_merge_410:
  if (r14) goto if_then_407; else goto if_merge_408;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_414; else goto logic_rhs_413;

  if_then_411:
  return 1;
  goto if_merge_412;

  if_merge_412:
  return 0;

  logic_rhs_413:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_414;

  logic_merge_414:
  if (r4) goto if_then_411; else goto if_merge_412;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_417; else goto logic_merge_418;

  if_then_415:
  return 1;
  goto if_merge_416;

  if_merge_416:
  return 0;

  logic_rhs_417:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_418;

  logic_merge_418:
  if (r4) goto if_then_415; else goto if_merge_416;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_421; else goto logic_merge_422;

  if_then_419:
  return 1;
  goto if_merge_420;

  if_merge_420:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_425; else goto logic_merge_426;

  logic_rhs_421:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_422;

  logic_merge_422:
  if (r4) goto if_then_419; else goto if_merge_420;

  if_then_423:
  return 1;
  goto if_merge_424;

  if_merge_424:
  return 0;

  logic_rhs_425:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_426;

  logic_merge_426:
  if (r9) goto if_then_423; else goto if_merge_424;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r1;

  entry:
  r0 = cn_var_c;
  是字母(r0);
  goto if_then_427;

  if_then_427:
  return 1;
  goto if_merge_428;

  if_merge_428:
  r1 = cn_var_c;
  是数字(r1);
  goto if_then_429;

  if_then_429:
  return 1;
  goto if_merge_430;

  if_merge_430:
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
  if (r3) goto logic_rhs_433; else goto logic_merge_434;

  if_then_431:
  return 1;
  goto if_merge_432;

  if_merge_432:
  return 0;

  logic_rhs_433:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_434;

  logic_merge_434:
  if (r5) goto if_then_431; else goto if_merge_432;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_435; else goto if_merge_436;

  if_then_435:
  return 1;
  goto if_merge_436;

  if_merge_436:
  r2 = cn_var_c;
  是字母(r2);
  goto if_then_437;

  if_then_437:
  return 1;
  goto if_merge_438;

  if_merge_438:
  void cn_var_整数值_0;
  r3 = cn_var_c;
  cn_var_整数值_0 = r3;
  r4 = cn_var_整数值_0;
  r5 = r4 >= 128;
  if (r5) goto if_then_439; else goto if_merge_440;

  if_then_439:
  return 1;
  goto if_merge_440;

  if_merge_440:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_441; else goto if_merge_442;

  if_then_441:
  return 1;
  goto if_merge_442;

  if_merge_442:
  r2 = cn_var_c;
  是字母或数字(r2);
  goto if_then_443;

  if_then_443:
  return 1;
  goto if_merge_444;

  if_merge_444:
  void cn_var_整数值_0;
  r3 = cn_var_c;
  cn_var_整数值_0 = r3;
  r4 = cn_var_整数值_0;
  r5 = r4 >= 128;
  if (r5) goto if_then_445; else goto if_merge_446;

  if_then_445:
  return 1;
  goto if_merge_446;

  if_merge_446:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_447; else goto if_merge_448;

  if_then_447:
  return 1;
  goto if_merge_448;

  if_merge_448:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_449; else goto if_merge_450;

  if_then_449:
  return 1;
  goto if_merge_450;

  if_merge_450:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_451; else goto if_merge_452;

  if_then_451:
  return 1;
  goto if_merge_452;

  if_merge_452:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_453; else goto if_merge_454;

  if_then_453:
  return 1;
  goto if_merge_454;

  if_merge_454:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_455; else goto if_merge_456;

  if_then_455:
  return 1;
  goto if_merge_456;

  if_merge_456:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_457; else goto if_merge_458;

  if_then_457:
  return 1;
  goto if_merge_458;

  if_merge_458:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_459; else goto if_merge_460;

  if_then_459:
  return 1;
  goto if_merge_460;

  if_merge_460:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_461; else goto if_merge_462;

  if_then_461:
  return 1;
  goto if_merge_462;

  if_merge_462:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_463; else goto if_merge_464;

  if_then_463:
  return 1;
  goto if_merge_464;

  if_merge_464:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_465; else goto if_merge_466;

  if_then_465:
  return 1;
  goto if_merge_466;

  if_merge_466:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_467; else goto if_merge_468;

  if_then_467:
  return 1;
  goto if_merge_468;

  if_merge_468:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_469; else goto if_merge_470;

  if_then_469:
  return 1;
  goto if_merge_470;

  if_merge_470:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_471; else goto if_merge_472;

  if_then_471:
  return 1;
  goto if_merge_472;

  if_merge_472:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_473; else goto if_merge_474;

  if_then_473:
  return 1;
  goto if_merge_474;

  if_merge_474:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_475; else goto if_merge_476;

  if_then_475:
  return 1;
  goto if_merge_476;

  if_merge_476:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_477; else goto if_merge_478;

  if_then_477:
  return 1;
  goto if_merge_478;

  if_merge_478:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_479; else goto if_merge_480;

  if_then_479:
  return 1;
  goto if_merge_480;

  if_merge_480:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_481; else goto if_merge_482;

  if_then_481:
  return 1;
  goto if_merge_482;

  if_merge_482:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_483; else goto if_merge_484;

  if_then_483:
  return 1;
  goto if_merge_484;

  if_merge_484:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_485; else goto if_merge_486;

  if_then_485:
  return 1;
  goto if_merge_486;

  if_merge_486:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_487; else goto if_merge_488;

  if_then_487:
  return 1;
  goto if_merge_488;

  if_merge_488:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_489; else goto if_merge_490;

  if_then_489:
  return 1;
  goto if_merge_490;

  if_merge_490:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_491; else goto if_merge_492;

  if_then_491:
  return 1;
  goto if_merge_492;

  if_merge_492:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_493; else goto if_merge_494;

  if_then_493:
  return 1;
  goto if_merge_494;

  if_merge_494:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_497; else goto logic_merge_498;

  if_then_495:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_496;

  if_merge_496:
  r8 = cn_var_c;
  return r8;

  logic_rhs_497:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_498;

  logic_merge_498:
  if (r4) goto if_then_495; else goto if_merge_496;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_501; else goto logic_merge_502;

  if_then_499:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_500;

  if_merge_500:
  r8 = cn_var_c;
  return r8;

  logic_rhs_501:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_502;

  logic_merge_502:
  if (r4) goto if_then_499; else goto if_merge_500;
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
  if (r3) goto logic_rhs_505; else goto logic_merge_506;

  if_then_503:
  return 1;
  goto if_merge_504;

  if_merge_504:
  return 0;

  logic_rhs_505:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_506;

  logic_merge_506:
  if (r5) goto if_then_503; else goto if_merge_504;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_509; else goto logic_merge_510;

  if_then_507:
  return 1;
  goto if_merge_508;

  if_merge_508:
  return 0;

  logic_rhs_509:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_512; else goto logic_rhs_511;

  logic_merge_510:
  if (r7) goto if_then_507; else goto if_merge_508;

  logic_rhs_511:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_512;

  logic_merge_512:
  goto logic_merge_510;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_515; else goto logic_merge_516;

  if_then_513:
  return 1;
  goto if_merge_514;

  if_merge_514:
  return 0;

  logic_rhs_515:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_518; else goto logic_rhs_517;

  logic_merge_516:
  if (r7) goto if_then_513; else goto if_merge_514;

  logic_rhs_517:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_518;

  logic_merge_518:
  goto logic_merge_516;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_521; else goto logic_merge_522;

  if_then_519:
  return 1;
  goto if_merge_520;

  if_merge_520:
  return 0;

  logic_rhs_521:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_524; else goto logic_rhs_523;

  logic_merge_522:
  if (r7) goto if_then_519; else goto if_merge_520;

  logic_rhs_523:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_524;

  logic_merge_524:
  goto logic_merge_522;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_527; else goto logic_merge_528;

  if_then_525:
  return 1;
  goto if_merge_526;

  if_merge_526:
  return 0;

  logic_rhs_527:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_528;

  logic_merge_528:
  if (r4) goto if_then_525; else goto if_merge_526;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_531; else goto logic_merge_532;

  if_then_529:
  return 1;
  goto if_merge_530;

  if_merge_530:
  return 0;

  logic_rhs_531:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_532;

  logic_merge_532:
  if (r4) goto if_then_529; else goto if_merge_530;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_535; else goto logic_merge_536;

  if_then_533:
  return 1;
  goto if_merge_534;

  if_merge_534:
  return 0;

  logic_rhs_535:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_536;

  logic_merge_536:
  if (r4) goto if_then_533; else goto if_merge_534;
  return 0;
}

