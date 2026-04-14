#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型枚举 {
    词元类型枚举_关键字_如果,
    词元类型枚举_关键字_否则,
    词元类型枚举_关键字_当,
    词元类型枚举_关键字_循环,
    词元类型枚举_关键字_返回,
    词元类型枚举_关键字_中断,
    词元类型枚举_关键字_继续,
    词元类型枚举_关键字_选择,
    词元类型枚举_关键字_情况,
    词元类型枚举_关键字_默认,
    词元类型枚举_关键字_整数,
    词元类型枚举_关键字_小数,
    词元类型枚举_关键字_字符串,
    词元类型枚举_关键字_布尔,
    词元类型枚举_关键字_空类型,
    词元类型枚举_关键字_结构体,
    词元类型枚举_关键字_枚举,
    词元类型枚举_关键字_函数,
    词元类型枚举_关键字_变量,
    词元类型枚举_关键字_导入,
    词元类型枚举_关键字_从,
    词元类型枚举_关键字_公开,
    词元类型枚举_关键字_私有,
    词元类型枚举_关键字_静态,
    词元类型枚举_关键字_真,
    词元类型枚举_关键字_假,
    词元类型枚举_关键字_无,
    词元类型枚举_关键字_类,
    词元类型枚举_关键字_接口,
    词元类型枚举_关键字_保护,
    词元类型枚举_关键字_虚拟,
    词元类型枚举_关键字_重写,
    词元类型枚举_关键字_抽象,
    词元类型枚举_关键字_实现,
    词元类型枚举_关键字_自身,
    词元类型枚举_关键字_基类,
    词元类型枚举_关键字_尝试,
    词元类型枚举_关键字_捕获,
    词元类型枚举_关键字_抛出,
    词元类型枚举_关键字_最终,
    词元类型枚举_标识符,
    词元类型枚举_整数字面量,
    词元类型枚举_浮点字面量,
    词元类型枚举_字符串字面量,
    词元类型枚举_字符字面量,
    词元类型枚举_加号,
    词元类型枚举_减号,
    词元类型枚举_星号,
    词元类型枚举_斜杠,
    词元类型枚举_百分号,
    词元类型枚举_等于,
    词元类型枚举_赋值,
    词元类型枚举_不等于,
    词元类型枚举_小于,
    词元类型枚举_小于等于,
    词元类型枚举_大于,
    词元类型枚举_大于等于,
    词元类型枚举_逻辑与,
    词元类型枚举_逻辑或,
    词元类型枚举_逻辑非,
    词元类型枚举_按位与,
    词元类型枚举_按位或,
    词元类型枚举_按位异或,
    词元类型枚举_按位取反,
    词元类型枚举_左移,
    词元类型枚举_右移,
    词元类型枚举_自增,
    词元类型枚举_自减,
    词元类型枚举_箭头,
    词元类型枚举_左括号,
    词元类型枚举_右括号,
    词元类型枚举_左大括号,
    词元类型枚举_右大括号,
    词元类型枚举_左方括号,
    词元类型枚举_右方括号,
    词元类型枚举_分号,
    词元类型枚举_逗号,
    词元类型枚举_点,
    词元类型枚举_冒号,
    词元类型枚举_问号,
    词元类型枚举_结束,
    词元类型枚举_错误
};

// CN Language Global Struct Forward Declarations
struct 词元;

// CN Language Global Struct Definitions
struct 词元 {
    enum 词元类型枚举 类型;
    char* 值;
    long long 行号;
    long long 列号;
    long long 长度;
};

// Global Variables

// Forward Declarations
char* 词元类型名称(enum 词元类型枚举);
struct 词元 创建词元(enum 词元类型枚举, const char*, long long, long long, long long);
_Bool 是关键字(enum 词元类型枚举);
_Bool 是字面量(enum 词元类型枚举);
_Bool 是运算符(enum 词元类型枚举);
_Bool 是分隔符(enum 词元类型枚举);

char* 词元类型名称(enum 词元类型枚举 cn_var_类型) {
  long long r1, r3, r5, r7, r9, r11, r13, r15, r17, r19, r21, r23, r25, r27, r29, r31, r33, r35, r37, r39, r41, r43, r45, r47, r49, r51, r53, r55, r57, r59, r61, r63, r65, r67, r69, r71, r73, r75, r77, r79, r81, r83, r85, r87, r89, r91, r93, r95, r97, r99, r101, r103, r105, r107, r109, r111, r113, r115, r117, r119, r121, r123, r125, r127, r129, r131, r133, r135, r137, r139, r141, r143, r145, r147, r149, r151, r153, r155, r157, r159, r161, r163;
  enum 词元类型枚举 r0;
  enum 词元类型枚举 r2;
  enum 词元类型枚举 r4;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r10;
  enum 词元类型枚举 r12;
  enum 词元类型枚举 r14;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r20;
  enum 词元类型枚举 r22;
  enum 词元类型枚举 r24;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r28;
  enum 词元类型枚举 r30;
  enum 词元类型枚举 r32;
  enum 词元类型枚举 r34;
  enum 词元类型枚举 r36;
  enum 词元类型枚举 r38;
  enum 词元类型枚举 r40;
  enum 词元类型枚举 r42;
  enum 词元类型枚举 r44;
  enum 词元类型枚举 r46;
  enum 词元类型枚举 r48;
  enum 词元类型枚举 r50;
  enum 词元类型枚举 r52;
  enum 词元类型枚举 r54;
  enum 词元类型枚举 r56;
  enum 词元类型枚举 r58;
  enum 词元类型枚举 r60;
  enum 词元类型枚举 r62;
  enum 词元类型枚举 r64;
  enum 词元类型枚举 r66;
  enum 词元类型枚举 r68;
  enum 词元类型枚举 r70;
  enum 词元类型枚举 r72;
  enum 词元类型枚举 r74;
  enum 词元类型枚举 r76;
  enum 词元类型枚举 r78;
  enum 词元类型枚举 r80;
  enum 词元类型枚举 r82;
  enum 词元类型枚举 r84;
  enum 词元类型枚举 r86;
  enum 词元类型枚举 r88;
  enum 词元类型枚举 r90;
  enum 词元类型枚举 r92;
  enum 词元类型枚举 r94;
  enum 词元类型枚举 r96;
  enum 词元类型枚举 r98;
  enum 词元类型枚举 r100;
  enum 词元类型枚举 r102;
  enum 词元类型枚举 r104;
  enum 词元类型枚举 r106;
  enum 词元类型枚举 r108;
  enum 词元类型枚举 r110;
  enum 词元类型枚举 r112;
  enum 词元类型枚举 r114;
  enum 词元类型枚举 r116;
  enum 词元类型枚举 r118;
  enum 词元类型枚举 r120;
  enum 词元类型枚举 r122;
  enum 词元类型枚举 r124;
  enum 词元类型枚举 r126;
  enum 词元类型枚举 r128;
  enum 词元类型枚举 r130;
  enum 词元类型枚举 r132;
  enum 词元类型枚举 r134;
  enum 词元类型枚举 r136;
  enum 词元类型枚举 r138;
  enum 词元类型枚举 r140;
  enum 词元类型枚举 r142;
  enum 词元类型枚举 r144;
  enum 词元类型枚举 r146;
  enum 词元类型枚举 r148;
  enum 词元类型枚举 r150;
  enum 词元类型枚举 r152;
  enum 词元类型枚举 r154;
  enum 词元类型枚举 r156;
  enum 词元类型枚举 r158;
  enum 词元类型枚举 r160;
  enum 词元类型枚举 r162;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 词元类型枚举_关键字_如果;
  if (r1) goto if_then_326; else goto if_merge_327;

  if_then_326:
  return "关键字_如果";
  goto if_merge_327;

  if_merge_327:
  r2 = cn_var_类型;
  r3 = r2 == 词元类型枚举_关键字_否则;
  if (r3) goto if_then_328; else goto if_merge_329;

  if_then_328:
  return "关键字_否则";
  goto if_merge_329;

  if_merge_329:
  r4 = cn_var_类型;
  r5 = r4 == 词元类型枚举_关键字_当;
  if (r5) goto if_then_330; else goto if_merge_331;

  if_then_330:
  return "关键字_当";
  goto if_merge_331;

  if_merge_331:
  r6 = cn_var_类型;
  r7 = r6 == 词元类型枚举_关键字_循环;
  if (r7) goto if_then_332; else goto if_merge_333;

  if_then_332:
  return "关键字_循环";
  goto if_merge_333;

  if_merge_333:
  r8 = cn_var_类型;
  r9 = r8 == 词元类型枚举_关键字_返回;
  if (r9) goto if_then_334; else goto if_merge_335;

  if_then_334:
  return "关键字_返回";
  goto if_merge_335;

  if_merge_335:
  r10 = cn_var_类型;
  r11 = r10 == 词元类型枚举_关键字_中断;
  if (r11) goto if_then_336; else goto if_merge_337;

  if_then_336:
  return "关键字_中断";
  goto if_merge_337;

  if_merge_337:
  r12 = cn_var_类型;
  r13 = r12 == 词元类型枚举_关键字_继续;
  if (r13) goto if_then_338; else goto if_merge_339;

  if_then_338:
  return "关键字_继续";
  goto if_merge_339;

  if_merge_339:
  r14 = cn_var_类型;
  r15 = r14 == 词元类型枚举_关键字_选择;
  if (r15) goto if_then_340; else goto if_merge_341;

  if_then_340:
  return "关键字_选择";
  goto if_merge_341;

  if_merge_341:
  r16 = cn_var_类型;
  r17 = r16 == 词元类型枚举_关键字_情况;
  if (r17) goto if_then_342; else goto if_merge_343;

  if_then_342:
  return "关键字_情况";
  goto if_merge_343;

  if_merge_343:
  r18 = cn_var_类型;
  r19 = r18 == 词元类型枚举_关键字_默认;
  if (r19) goto if_then_344; else goto if_merge_345;

  if_then_344:
  return "关键字_默认";
  goto if_merge_345;

  if_merge_345:
  r20 = cn_var_类型;
  r21 = r20 == 词元类型枚举_关键字_整数;
  if (r21) goto if_then_346; else goto if_merge_347;

  if_then_346:
  return "关键字_整数";
  goto if_merge_347;

  if_merge_347:
  r22 = cn_var_类型;
  r23 = r22 == 词元类型枚举_关键字_小数;
  if (r23) goto if_then_348; else goto if_merge_349;

  if_then_348:
  return "关键字_小数";
  goto if_merge_349;

  if_merge_349:
  r24 = cn_var_类型;
  r25 = r24 == 词元类型枚举_关键字_字符串;
  if (r25) goto if_then_350; else goto if_merge_351;

  if_then_350:
  return "关键字_字符串";
  goto if_merge_351;

  if_merge_351:
  r26 = cn_var_类型;
  r27 = r26 == 词元类型枚举_关键字_布尔;
  if (r27) goto if_then_352; else goto if_merge_353;

  if_then_352:
  return "关键字_布尔";
  goto if_merge_353;

  if_merge_353:
  r28 = cn_var_类型;
  r29 = r28 == 词元类型枚举_关键字_空类型;
  if (r29) goto if_then_354; else goto if_merge_355;

  if_then_354:
  return "关键字_空类型";
  goto if_merge_355;

  if_merge_355:
  r30 = cn_var_类型;
  r31 = r30 == 词元类型枚举_关键字_结构体;
  if (r31) goto if_then_356; else goto if_merge_357;

  if_then_356:
  return "关键字_结构体";
  goto if_merge_357;

  if_merge_357:
  r32 = cn_var_类型;
  r33 = r32 == 词元类型枚举_关键字_枚举;
  if (r33) goto if_then_358; else goto if_merge_359;

  if_then_358:
  return "关键字_枚举";
  goto if_merge_359;

  if_merge_359:
  r34 = cn_var_类型;
  r35 = r34 == 词元类型枚举_关键字_函数;
  if (r35) goto if_then_360; else goto if_merge_361;

  if_then_360:
  return "关键字_函数";
  goto if_merge_361;

  if_merge_361:
  r36 = cn_var_类型;
  r37 = r36 == 词元类型枚举_关键字_变量;
  if (r37) goto if_then_362; else goto if_merge_363;

  if_then_362:
  return "关键字_变量";
  goto if_merge_363;

  if_merge_363:
  r38 = cn_var_类型;
  r39 = r38 == 词元类型枚举_关键字_导入;
  if (r39) goto if_then_364; else goto if_merge_365;

  if_then_364:
  return "关键字_导入";
  goto if_merge_365;

  if_merge_365:
  r40 = cn_var_类型;
  r41 = r40 == 词元类型枚举_关键字_从;
  if (r41) goto if_then_366; else goto if_merge_367;

  if_then_366:
  return "关键字_从";
  goto if_merge_367;

  if_merge_367:
  r42 = cn_var_类型;
  r43 = r42 == 词元类型枚举_关键字_公开;
  if (r43) goto if_then_368; else goto if_merge_369;

  if_then_368:
  return "关键字_公开";
  goto if_merge_369;

  if_merge_369:
  r44 = cn_var_类型;
  r45 = r44 == 词元类型枚举_关键字_私有;
  if (r45) goto if_then_370; else goto if_merge_371;

  if_then_370:
  return "关键字_私有";
  goto if_merge_371;

  if_merge_371:
  r46 = cn_var_类型;
  r47 = r46 == 词元类型枚举_关键字_静态;
  if (r47) goto if_then_372; else goto if_merge_373;

  if_then_372:
  return "关键字_静态";
  goto if_merge_373;

  if_merge_373:
  r48 = cn_var_类型;
  r49 = r48 == 词元类型枚举_关键字_真;
  if (r49) goto if_then_374; else goto if_merge_375;

  if_then_374:
  return "关键字_真";
  goto if_merge_375;

  if_merge_375:
  r50 = cn_var_类型;
  r51 = r50 == 词元类型枚举_关键字_假;
  if (r51) goto if_then_376; else goto if_merge_377;

  if_then_376:
  return "关键字_假";
  goto if_merge_377;

  if_merge_377:
  r52 = cn_var_类型;
  r53 = r52 == 词元类型枚举_关键字_无;
  if (r53) goto if_then_378; else goto if_merge_379;

  if_then_378:
  return "关键字_无";
  goto if_merge_379;

  if_merge_379:
  r54 = cn_var_类型;
  r55 = r54 == 词元类型枚举_关键字_类;
  if (r55) goto if_then_380; else goto if_merge_381;

  if_then_380:
  return "关键字_类";
  goto if_merge_381;

  if_merge_381:
  r56 = cn_var_类型;
  r57 = r56 == 词元类型枚举_关键字_接口;
  if (r57) goto if_then_382; else goto if_merge_383;

  if_then_382:
  return "关键字_接口";
  goto if_merge_383;

  if_merge_383:
  r58 = cn_var_类型;
  r59 = r58 == 词元类型枚举_关键字_保护;
  if (r59) goto if_then_384; else goto if_merge_385;

  if_then_384:
  return "关键字_保护";
  goto if_merge_385;

  if_merge_385:
  r60 = cn_var_类型;
  r61 = r60 == 词元类型枚举_关键字_虚拟;
  if (r61) goto if_then_386; else goto if_merge_387;

  if_then_386:
  return "关键字_虚拟";
  goto if_merge_387;

  if_merge_387:
  r62 = cn_var_类型;
  r63 = r62 == 词元类型枚举_关键字_重写;
  if (r63) goto if_then_388; else goto if_merge_389;

  if_then_388:
  return "关键字_重写";
  goto if_merge_389;

  if_merge_389:
  r64 = cn_var_类型;
  r65 = r64 == 词元类型枚举_关键字_抽象;
  if (r65) goto if_then_390; else goto if_merge_391;

  if_then_390:
  return "关键字_抽象";
  goto if_merge_391;

  if_merge_391:
  r66 = cn_var_类型;
  r67 = r66 == 词元类型枚举_关键字_实现;
  if (r67) goto if_then_392; else goto if_merge_393;

  if_then_392:
  return "关键字_实现";
  goto if_merge_393;

  if_merge_393:
  r68 = cn_var_类型;
  r69 = r68 == 词元类型枚举_关键字_自身;
  if (r69) goto if_then_394; else goto if_merge_395;

  if_then_394:
  return "关键字_自身";
  goto if_merge_395;

  if_merge_395:
  r70 = cn_var_类型;
  r71 = r70 == 词元类型枚举_关键字_基类;
  if (r71) goto if_then_396; else goto if_merge_397;

  if_then_396:
  return "关键字_基类";
  goto if_merge_397;

  if_merge_397:
  r72 = cn_var_类型;
  r73 = r72 == 词元类型枚举_关键字_尝试;
  if (r73) goto if_then_398; else goto if_merge_399;

  if_then_398:
  return "关键字_尝试";
  goto if_merge_399;

  if_merge_399:
  r74 = cn_var_类型;
  r75 = r74 == 词元类型枚举_关键字_捕获;
  if (r75) goto if_then_400; else goto if_merge_401;

  if_then_400:
  return "关键字_捕获";
  goto if_merge_401;

  if_merge_401:
  r76 = cn_var_类型;
  r77 = r76 == 词元类型枚举_关键字_抛出;
  if (r77) goto if_then_402; else goto if_merge_403;

  if_then_402:
  return "关键字_抛出";
  goto if_merge_403;

  if_merge_403:
  r78 = cn_var_类型;
  r79 = r78 == 词元类型枚举_关键字_最终;
  if (r79) goto if_then_404; else goto if_merge_405;

  if_then_404:
  return "关键字_最终";
  goto if_merge_405;

  if_merge_405:
  r80 = cn_var_类型;
  r81 = r80 == 词元类型枚举_标识符;
  if (r81) goto if_then_406; else goto if_merge_407;

  if_then_406:
  return "标识符";
  goto if_merge_407;

  if_merge_407:
  r82 = cn_var_类型;
  r83 = r82 == 词元类型枚举_整数字面量;
  if (r83) goto if_then_408; else goto if_merge_409;

  if_then_408:
  return "整数字面量";
  goto if_merge_409;

  if_merge_409:
  r84 = cn_var_类型;
  r85 = r84 == 词元类型枚举_浮点字面量;
  if (r85) goto if_then_410; else goto if_merge_411;

  if_then_410:
  return "浮点字面量";
  goto if_merge_411;

  if_merge_411:
  r86 = cn_var_类型;
  r87 = r86 == 词元类型枚举_字符串字面量;
  if (r87) goto if_then_412; else goto if_merge_413;

  if_then_412:
  return "字符串字面量";
  goto if_merge_413;

  if_merge_413:
  r88 = cn_var_类型;
  r89 = r88 == 词元类型枚举_字符字面量;
  if (r89) goto if_then_414; else goto if_merge_415;

  if_then_414:
  return "字符字面量";
  goto if_merge_415;

  if_merge_415:
  r90 = cn_var_类型;
  r91 = r90 == 词元类型枚举_加号;
  if (r91) goto if_then_416; else goto if_merge_417;

  if_then_416:
  return "加号";
  goto if_merge_417;

  if_merge_417:
  r92 = cn_var_类型;
  r93 = r92 == 词元类型枚举_减号;
  if (r93) goto if_then_418; else goto if_merge_419;

  if_then_418:
  return "减号";
  goto if_merge_419;

  if_merge_419:
  r94 = cn_var_类型;
  r95 = r94 == 词元类型枚举_星号;
  if (r95) goto if_then_420; else goto if_merge_421;

  if_then_420:
  return "星号";
  goto if_merge_421;

  if_merge_421:
  r96 = cn_var_类型;
  r97 = r96 == 词元类型枚举_斜杠;
  if (r97) goto if_then_422; else goto if_merge_423;

  if_then_422:
  return "斜杠";
  goto if_merge_423;

  if_merge_423:
  r98 = cn_var_类型;
  r99 = r98 == 词元类型枚举_百分号;
  if (r99) goto if_then_424; else goto if_merge_425;

  if_then_424:
  return "百分号";
  goto if_merge_425;

  if_merge_425:
  r100 = cn_var_类型;
  r101 = r100 == 词元类型枚举_等于;
  if (r101) goto if_then_426; else goto if_merge_427;

  if_then_426:
  return "等于";
  goto if_merge_427;

  if_merge_427:
  r102 = cn_var_类型;
  r103 = r102 == 词元类型枚举_赋值;
  if (r103) goto if_then_428; else goto if_merge_429;

  if_then_428:
  return "赋值";
  goto if_merge_429;

  if_merge_429:
  r104 = cn_var_类型;
  r105 = r104 == 词元类型枚举_不等于;
  if (r105) goto if_then_430; else goto if_merge_431;

  if_then_430:
  return "不等于";
  goto if_merge_431;

  if_merge_431:
  r106 = cn_var_类型;
  r107 = r106 == 词元类型枚举_小于;
  if (r107) goto if_then_432; else goto if_merge_433;

  if_then_432:
  return "小于";
  goto if_merge_433;

  if_merge_433:
  r108 = cn_var_类型;
  r109 = r108 == 词元类型枚举_小于等于;
  if (r109) goto if_then_434; else goto if_merge_435;

  if_then_434:
  return "小于等于";
  goto if_merge_435;

  if_merge_435:
  r110 = cn_var_类型;
  r111 = r110 == 词元类型枚举_大于;
  if (r111) goto if_then_436; else goto if_merge_437;

  if_then_436:
  return "大于";
  goto if_merge_437;

  if_merge_437:
  r112 = cn_var_类型;
  r113 = r112 == 词元类型枚举_大于等于;
  if (r113) goto if_then_438; else goto if_merge_439;

  if_then_438:
  return "大于等于";
  goto if_merge_439;

  if_merge_439:
  r114 = cn_var_类型;
  r115 = r114 == 词元类型枚举_逻辑与;
  if (r115) goto if_then_440; else goto if_merge_441;

  if_then_440:
  return "逻辑与";
  goto if_merge_441;

  if_merge_441:
  r116 = cn_var_类型;
  r117 = r116 == 词元类型枚举_逻辑或;
  if (r117) goto if_then_442; else goto if_merge_443;

  if_then_442:
  return "逻辑或";
  goto if_merge_443;

  if_merge_443:
  r118 = cn_var_类型;
  r119 = r118 == 词元类型枚举_逻辑非;
  if (r119) goto if_then_444; else goto if_merge_445;

  if_then_444:
  return "逻辑非";
  goto if_merge_445;

  if_merge_445:
  r120 = cn_var_类型;
  r121 = r120 == 词元类型枚举_按位与;
  if (r121) goto if_then_446; else goto if_merge_447;

  if_then_446:
  return "按位与";
  goto if_merge_447;

  if_merge_447:
  r122 = cn_var_类型;
  r123 = r122 == 词元类型枚举_按位或;
  if (r123) goto if_then_448; else goto if_merge_449;

  if_then_448:
  return "按位或";
  goto if_merge_449;

  if_merge_449:
  r124 = cn_var_类型;
  r125 = r124 == 词元类型枚举_按位异或;
  if (r125) goto if_then_450; else goto if_merge_451;

  if_then_450:
  return "按位异或";
  goto if_merge_451;

  if_merge_451:
  r126 = cn_var_类型;
  r127 = r126 == 词元类型枚举_按位取反;
  if (r127) goto if_then_452; else goto if_merge_453;

  if_then_452:
  return "按位取反";
  goto if_merge_453;

  if_merge_453:
  r128 = cn_var_类型;
  r129 = r128 == 词元类型枚举_左移;
  if (r129) goto if_then_454; else goto if_merge_455;

  if_then_454:
  return "左移";
  goto if_merge_455;

  if_merge_455:
  r130 = cn_var_类型;
  r131 = r130 == 词元类型枚举_右移;
  if (r131) goto if_then_456; else goto if_merge_457;

  if_then_456:
  return "右移";
  goto if_merge_457;

  if_merge_457:
  r132 = cn_var_类型;
  r133 = r132 == 词元类型枚举_自增;
  if (r133) goto if_then_458; else goto if_merge_459;

  if_then_458:
  return "自增";
  goto if_merge_459;

  if_merge_459:
  r134 = cn_var_类型;
  r135 = r134 == 词元类型枚举_自减;
  if (r135) goto if_then_460; else goto if_merge_461;

  if_then_460:
  return "自减";
  goto if_merge_461;

  if_merge_461:
  r136 = cn_var_类型;
  r137 = r136 == 词元类型枚举_箭头;
  if (r137) goto if_then_462; else goto if_merge_463;

  if_then_462:
  return "箭头";
  goto if_merge_463;

  if_merge_463:
  r138 = cn_var_类型;
  r139 = r138 == 词元类型枚举_左括号;
  if (r139) goto if_then_464; else goto if_merge_465;

  if_then_464:
  return "左括号";
  goto if_merge_465;

  if_merge_465:
  r140 = cn_var_类型;
  r141 = r140 == 词元类型枚举_右括号;
  if (r141) goto if_then_466; else goto if_merge_467;

  if_then_466:
  return "右括号";
  goto if_merge_467;

  if_merge_467:
  r142 = cn_var_类型;
  r143 = r142 == 词元类型枚举_左大括号;
  if (r143) goto if_then_468; else goto if_merge_469;

  if_then_468:
  return "左大括号";
  goto if_merge_469;

  if_merge_469:
  r144 = cn_var_类型;
  r145 = r144 == 词元类型枚举_右大括号;
  if (r145) goto if_then_470; else goto if_merge_471;

  if_then_470:
  return "右大括号";
  goto if_merge_471;

  if_merge_471:
  r146 = cn_var_类型;
  r147 = r146 == 词元类型枚举_左方括号;
  if (r147) goto if_then_472; else goto if_merge_473;

  if_then_472:
  return "左方括号";
  goto if_merge_473;

  if_merge_473:
  r148 = cn_var_类型;
  r149 = r148 == 词元类型枚举_右方括号;
  if (r149) goto if_then_474; else goto if_merge_475;

  if_then_474:
  return "右方括号";
  goto if_merge_475;

  if_merge_475:
  r150 = cn_var_类型;
  r151 = r150 == 词元类型枚举_分号;
  if (r151) goto if_then_476; else goto if_merge_477;

  if_then_476:
  return "分号";
  goto if_merge_477;

  if_merge_477:
  r152 = cn_var_类型;
  r153 = r152 == 词元类型枚举_逗号;
  if (r153) goto if_then_478; else goto if_merge_479;

  if_then_478:
  return "逗号";
  goto if_merge_479;

  if_merge_479:
  r154 = cn_var_类型;
  r155 = r154 == 词元类型枚举_点;
  if (r155) goto if_then_480; else goto if_merge_481;

  if_then_480:
  return "点";
  goto if_merge_481;

  if_merge_481:
  r156 = cn_var_类型;
  r157 = r156 == 词元类型枚举_冒号;
  if (r157) goto if_then_482; else goto if_merge_483;

  if_then_482:
  return "冒号";
  goto if_merge_483;

  if_merge_483:
  r158 = cn_var_类型;
  r159 = r158 == 词元类型枚举_问号;
  if (r159) goto if_then_484; else goto if_merge_485;

  if_then_484:
  return "问号";
  goto if_merge_485;

  if_merge_485:
  r160 = cn_var_类型;
  r161 = r160 == 词元类型枚举_结束;
  if (r161) goto if_then_486; else goto if_merge_487;

  if_then_486:
  return "结束";
  goto if_merge_487;

  if_merge_487:
  r162 = cn_var_类型;
  r163 = r162 == 词元类型枚举_错误;
  if (r163) goto if_then_488; else goto if_merge_489;

  if_then_488:
  return "错误";
  goto if_merge_489;

  if_merge_489:
  return "未知词元类型";
}

struct 词元 创建词元(enum 词元类型枚举 cn_var_类型, const char* cn_var_值, long long cn_var_行号, long long cn_var_列号, long long cn_var_长度值) {
  long long r2, r3, r4;
  char* r1;
  struct 词元 r5;
  enum 词元类型枚举 r0;

  entry:
  struct 词元 cn_var_新词元_0;
  r0 = cn_var_类型;
  r1 = cn_var_值;
  r2 = cn_var_行号;
  r3 = cn_var_列号;
  r4 = cn_var_长度值;
  r5 = cn_var_新词元_0;
  return r5;
}

_Bool 是关键字(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4, r5, r7, r9, r10, r12, r14, r15, r17, r19, r20, r22, r24, r25, r27, r29;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;
  enum 词元类型枚举 r6;
  enum 词元类型枚举 r8;
  enum 词元类型枚举 r11;
  enum 词元类型枚举 r13;
  enum 词元类型枚举 r16;
  enum 词元类型枚举 r18;
  enum 词元类型枚举 r21;
  enum 词元类型枚举 r23;
  enum 词元类型枚举 r26;
  enum 词元类型枚举 r28;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_关键字_如果;
  if (r2) goto logic_rhs_492; else goto logic_merge_493;

  if_then_490:
  return 1;
  goto if_merge_491;

  if_merge_491:
  r6 = cn_var_类型;
  r7 = r6 >= 词元类型枚举_关键字_整数;
  if (r7) goto logic_rhs_496; else goto logic_merge_497;

  logic_rhs_492:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_关键字_默认;
  goto logic_merge_493;

  logic_merge_493:
  if (r4) goto if_then_490; else goto if_merge_491;

  if_then_494:
  return 1;
  goto if_merge_495;

  if_merge_495:
  r11 = cn_var_类型;
  r12 = r11 >= 词元类型枚举_关键字_函数;
  if (r12) goto logic_rhs_500; else goto logic_merge_501;

  logic_rhs_496:
  r8 = cn_var_类型;
  r9 = r8 <= 词元类型枚举_关键字_枚举;
  goto logic_merge_497;

  logic_merge_497:
  if (r9) goto if_then_494; else goto if_merge_495;

  if_then_498:
  return 1;
  goto if_merge_499;

  if_merge_499:
  r16 = cn_var_类型;
  r17 = r16 >= 词元类型枚举_关键字_真;
  if (r17) goto logic_rhs_504; else goto logic_merge_505;

  logic_rhs_500:
  r13 = cn_var_类型;
  r14 = r13 <= 词元类型枚举_关键字_静态;
  goto logic_merge_501;

  logic_merge_501:
  if (r14) goto if_then_498; else goto if_merge_499;

  if_then_502:
  return 1;
  goto if_merge_503;

  if_merge_503:
  r21 = cn_var_类型;
  r22 = r21 >= 词元类型枚举_关键字_类;
  if (r22) goto logic_rhs_508; else goto logic_merge_509;

  logic_rhs_504:
  r18 = cn_var_类型;
  r19 = r18 <= 词元类型枚举_关键字_无;
  goto logic_merge_505;

  logic_merge_505:
  if (r19) goto if_then_502; else goto if_merge_503;

  if_then_506:
  return 1;
  goto if_merge_507;

  if_merge_507:
  r26 = cn_var_类型;
  r27 = r26 >= 词元类型枚举_关键字_尝试;
  if (r27) goto logic_rhs_512; else goto logic_merge_513;

  logic_rhs_508:
  r23 = cn_var_类型;
  r24 = r23 <= 词元类型枚举_关键字_基类;
  goto logic_merge_509;

  logic_merge_509:
  if (r24) goto if_then_506; else goto if_merge_507;

  if_then_510:
  return 1;
  goto if_merge_511;

  if_merge_511:
  return 0;

  logic_rhs_512:
  r28 = cn_var_类型;
  r29 = r28 <= 词元类型枚举_关键字_最终;
  goto logic_merge_513;

  logic_merge_513:
  if (r29) goto if_then_510; else goto if_merge_511;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_标识符;
  if (r2) goto logic_rhs_514; else goto logic_merge_515;

  logic_rhs_514:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_字符字面量;
  goto logic_merge_515;

  logic_merge_515:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_加号;
  if (r2) goto logic_rhs_516; else goto logic_merge_517;

  logic_rhs_516:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_箭头;
  goto logic_merge_517;

  logic_merge_517:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_左括号;
  if (r2) goto logic_rhs_518; else goto logic_merge_519;

  logic_rhs_518:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_问号;
  goto logic_merge_519;

  logic_merge_519:
  return r4;
}

