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
  if (r1) goto if_then_294; else goto if_merge_295;

  if_then_294:
  return 1;
  goto if_merge_295;

  if_merge_295:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_296; else goto if_merge_297;

  if_then_296:
  return 1;
  goto if_merge_297;

  if_merge_297:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_298; else goto if_merge_299;

  if_then_298:
  return 1;
  goto if_merge_299;

  if_merge_299:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_300; else goto if_merge_301;

  if_then_300:
  return 1;
  goto if_merge_301;

  if_merge_301:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_302; else goto if_merge_303;

  if_then_302:
  return 1;
  goto if_merge_303;

  if_merge_303:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_304; else goto if_merge_305;

  if_then_304:
  return 1;
  goto if_merge_305;

  if_merge_305:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_308; else goto logic_merge_309;

  if_then_306:
  return 1;
  goto if_merge_307;

  if_merge_307:
  return 0;

  logic_rhs_308:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_309;

  logic_merge_309:
  if (r4) goto if_then_306; else goto if_merge_307;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_312; else goto logic_merge_313;

  if_then_310:
  return 1;
  goto if_merge_311;

  if_merge_311:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_316; else goto logic_merge_317;

  logic_rhs_312:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_313;

  logic_merge_313:
  if (r4) goto if_then_310; else goto if_merge_311;

  if_then_314:
  return 1;
  goto if_merge_315;

  if_merge_315:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_320; else goto logic_merge_321;

  logic_rhs_316:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_317;

  logic_merge_317:
  if (r9) goto if_then_314; else goto if_merge_315;

  if_then_318:
  return 1;
  goto if_merge_319;

  if_merge_319:
  return 0;

  logic_rhs_320:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_321;

  logic_merge_321:
  if (r14) goto if_then_318; else goto if_merge_319;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_325; else goto logic_rhs_324;

  if_then_322:
  return 1;
  goto if_merge_323;

  if_merge_323:
  return 0;

  logic_rhs_324:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_325;

  logic_merge_325:
  if (r4) goto if_then_322; else goto if_merge_323;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_328; else goto logic_merge_329;

  if_then_326:
  return 1;
  goto if_merge_327;

  if_merge_327:
  return 0;

  logic_rhs_328:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_329;

  logic_merge_329:
  if (r4) goto if_then_326; else goto if_merge_327;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_332; else goto logic_merge_333;

  if_then_330:
  return 1;
  goto if_merge_331;

  if_merge_331:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_336; else goto logic_merge_337;

  logic_rhs_332:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_333;

  logic_merge_333:
  if (r4) goto if_then_330; else goto if_merge_331;

  if_then_334:
  return 1;
  goto if_merge_335;

  if_merge_335:
  return 0;

  logic_rhs_336:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_337;

  logic_merge_337:
  if (r9) goto if_then_334; else goto if_merge_335;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_338; else goto if_merge_339;

  if_then_338:
  return 1;
  goto if_merge_339;

  if_merge_339:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_340; else goto if_merge_341;

  if_then_340:
  return 1;
  goto if_merge_341;

  if_merge_341:
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
  if (r3) goto logic_rhs_344; else goto logic_merge_345;

  if_then_342:
  return 1;
  goto if_merge_343;

  if_merge_343:
  return 0;

  logic_rhs_344:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_345;

  logic_merge_345:
  if (r5) goto if_then_342; else goto if_merge_343;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_346; else goto if_merge_347;

  if_then_346:
  return 1;
  goto if_merge_347;

  if_merge_347:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_348; else goto if_merge_349;

  if_then_348:
  return 1;
  goto if_merge_349;

  if_merge_349:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_350; else goto if_merge_351;

  if_then_350:
  return 1;
  goto if_merge_351;

  if_merge_351:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_352; else goto if_merge_353;

  if_then_352:
  return 1;
  goto if_merge_353;

  if_merge_353:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_354; else goto if_merge_355;

  if_then_354:
  return 1;
  goto if_merge_355;

  if_merge_355:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_356; else goto if_merge_357;

  if_then_356:
  return 1;
  goto if_merge_357;

  if_merge_357:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_358; else goto if_merge_359;

  if_then_358:
  return 1;
  goto if_merge_359;

  if_merge_359:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_360; else goto if_merge_361;

  if_then_360:
  return 1;
  goto if_merge_361;

  if_merge_361:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_362; else goto if_merge_363;

  if_then_362:
  return 1;
  goto if_merge_363;

  if_merge_363:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_364; else goto if_merge_365;

  if_then_364:
  return 1;
  goto if_merge_365;

  if_merge_365:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_366; else goto if_merge_367;

  if_then_366:
  return 1;
  goto if_merge_367;

  if_merge_367:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_368; else goto if_merge_369;

  if_then_368:
  return 1;
  goto if_merge_369;

  if_merge_369:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_370; else goto if_merge_371;

  if_then_370:
  return 1;
  goto if_merge_371;

  if_merge_371:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_372; else goto if_merge_373;

  if_then_372:
  return 1;
  goto if_merge_373;

  if_merge_373:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_374; else goto if_merge_375;

  if_then_374:
  return 1;
  goto if_merge_375;

  if_merge_375:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_376; else goto if_merge_377;

  if_then_376:
  return 1;
  goto if_merge_377;

  if_merge_377:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_378; else goto if_merge_379;

  if_then_378:
  return 1;
  goto if_merge_379;

  if_merge_379:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_380; else goto if_merge_381;

  if_then_380:
  return 1;
  goto if_merge_381;

  if_merge_381:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_382; else goto if_merge_383;

  if_then_382:
  return 1;
  goto if_merge_383;

  if_merge_383:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_384; else goto if_merge_385;

  if_then_384:
  return 1;
  goto if_merge_385;

  if_merge_385:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_386; else goto if_merge_387;

  if_then_386:
  return 1;
  goto if_merge_387;

  if_merge_387:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_388; else goto if_merge_389;

  if_then_388:
  return 1;
  goto if_merge_389;

  if_merge_389:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_390; else goto if_merge_391;

  if_then_390:
  return 1;
  goto if_merge_391;

  if_merge_391:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_392; else goto if_merge_393;

  if_then_392:
  return 1;
  goto if_merge_393;

  if_merge_393:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_394; else goto if_merge_395;

  if_then_394:
  return 1;
  goto if_merge_395;

  if_merge_395:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_396; else goto if_merge_397;

  if_then_396:
  return 1;
  goto if_merge_397;

  if_merge_397:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_398; else goto if_merge_399;

  if_then_398:
  return 1;
  goto if_merge_399;

  if_merge_399:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_400; else goto if_merge_401;

  if_then_400:
  return 1;
  goto if_merge_401;

  if_merge_401:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_402; else goto if_merge_403;

  if_then_402:
  return 1;
  goto if_merge_403;

  if_merge_403:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_404; else goto if_merge_405;

  if_then_404:
  return 1;
  goto if_merge_405;

  if_merge_405:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_408; else goto logic_merge_409;

  if_then_406:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_407;

  if_merge_407:
  r8 = cn_var_c;
  return r8;

  logic_rhs_408:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_409;

  logic_merge_409:
  if (r4) goto if_then_406; else goto if_merge_407;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_412; else goto logic_merge_413;

  if_then_410:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_411;

  if_merge_411:
  r8 = cn_var_c;
  return r8;

  logic_rhs_412:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_413;

  logic_merge_413:
  if (r4) goto if_then_410; else goto if_merge_411;
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
  if (r3) goto logic_rhs_416; else goto logic_merge_417;

  if_then_414:
  return 1;
  goto if_merge_415;

  if_merge_415:
  return 0;

  logic_rhs_416:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_417;

  logic_merge_417:
  if (r5) goto if_then_414; else goto if_merge_415;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_420; else goto logic_merge_421;

  if_then_418:
  return 1;
  goto if_merge_419;

  if_merge_419:
  return 0;

  logic_rhs_420:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_423; else goto logic_rhs_422;

  logic_merge_421:
  if (r7) goto if_then_418; else goto if_merge_419;

  logic_rhs_422:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_423;

  logic_merge_423:
  goto logic_merge_421;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_426; else goto logic_merge_427;

  if_then_424:
  return 1;
  goto if_merge_425;

  if_merge_425:
  return 0;

  logic_rhs_426:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_429; else goto logic_rhs_428;

  logic_merge_427:
  if (r7) goto if_then_424; else goto if_merge_425;

  logic_rhs_428:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_429;

  logic_merge_429:
  goto logic_merge_427;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_432; else goto logic_merge_433;

  if_then_430:
  return 1;
  goto if_merge_431;

  if_merge_431:
  return 0;

  logic_rhs_432:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_435; else goto logic_rhs_434;

  logic_merge_433:
  if (r7) goto if_then_430; else goto if_merge_431;

  logic_rhs_434:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_435;

  logic_merge_435:
  goto logic_merge_433;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_438; else goto logic_merge_439;

  if_then_436:
  return 1;
  goto if_merge_437;

  if_merge_437:
  return 0;

  logic_rhs_438:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_439;

  logic_merge_439:
  if (r4) goto if_then_436; else goto if_merge_437;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_442; else goto logic_merge_443;

  if_then_440:
  return 1;
  goto if_merge_441;

  if_merge_441:
  return 0;

  logic_rhs_442:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_443;

  logic_merge_443:
  if (r4) goto if_then_440; else goto if_merge_441;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_446; else goto logic_merge_447;

  if_then_444:
  return 1;
  goto if_merge_445;

  if_merge_445:
  return 0;

  logic_rhs_446:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_447;

  logic_merge_447:
  if (r4) goto if_then_444; else goto if_merge_445;
  return 0;
}

