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
  enum 词元类型枚举 r0;
  long long r1;
  enum 词元类型枚举 r2;
  long long r3;
  enum 词元类型枚举 r4;
  long long r5;
  enum 词元类型枚举 r6;
  long long r7;
  enum 词元类型枚举 r8;
  long long r9;
  enum 词元类型枚举 r10;
  long long r11;
  enum 词元类型枚举 r12;
  long long r13;
  enum 词元类型枚举 r14;
  long long r15;
  enum 词元类型枚举 r16;
  long long r17;
  enum 词元类型枚举 r18;
  long long r19;
  enum 词元类型枚举 r20;
  long long r21;
  enum 词元类型枚举 r22;
  long long r23;
  enum 词元类型枚举 r24;
  long long r25;
  enum 词元类型枚举 r26;
  long long r27;
  enum 词元类型枚举 r28;
  long long r29;
  enum 词元类型枚举 r30;
  long long r31;
  enum 词元类型枚举 r32;
  long long r33;
  enum 词元类型枚举 r34;
  long long r35;
  enum 词元类型枚举 r36;
  long long r37;
  enum 词元类型枚举 r38;
  long long r39;
  enum 词元类型枚举 r40;
  long long r41;
  enum 词元类型枚举 r42;
  long long r43;
  enum 词元类型枚举 r44;
  long long r45;
  enum 词元类型枚举 r46;
  long long r47;
  enum 词元类型枚举 r48;
  long long r49;
  enum 词元类型枚举 r50;
  long long r51;
  enum 词元类型枚举 r52;
  long long r53;
  enum 词元类型枚举 r54;
  long long r55;
  enum 词元类型枚举 r56;
  long long r57;
  enum 词元类型枚举 r58;
  long long r59;
  enum 词元类型枚举 r60;
  long long r61;
  enum 词元类型枚举 r62;
  long long r63;
  enum 词元类型枚举 r64;
  long long r65;
  enum 词元类型枚举 r66;
  long long r67;
  enum 词元类型枚举 r68;
  long long r69;
  enum 词元类型枚举 r70;
  long long r71;
  enum 词元类型枚举 r72;
  long long r73;
  enum 词元类型枚举 r74;
  long long r75;
  enum 词元类型枚举 r76;
  long long r77;
  enum 词元类型枚举 r78;
  long long r79;
  enum 词元类型枚举 r80;
  long long r81;
  enum 词元类型枚举 r82;
  long long r83;
  enum 词元类型枚举 r84;
  long long r85;
  enum 词元类型枚举 r86;
  long long r87;
  enum 词元类型枚举 r88;
  long long r89;
  enum 词元类型枚举 r90;
  long long r91;
  enum 词元类型枚举 r92;
  long long r93;
  enum 词元类型枚举 r94;
  long long r95;
  enum 词元类型枚举 r96;
  long long r97;
  enum 词元类型枚举 r98;
  long long r99;
  enum 词元类型枚举 r100;
  long long r101;
  enum 词元类型枚举 r102;
  long long r103;
  enum 词元类型枚举 r104;
  long long r105;
  enum 词元类型枚举 r106;
  long long r107;
  enum 词元类型枚举 r108;
  long long r109;
  enum 词元类型枚举 r110;
  long long r111;
  enum 词元类型枚举 r112;
  long long r113;
  enum 词元类型枚举 r114;
  long long r115;
  enum 词元类型枚举 r116;
  long long r117;
  enum 词元类型枚举 r118;
  long long r119;
  enum 词元类型枚举 r120;
  long long r121;
  enum 词元类型枚举 r122;
  long long r123;
  enum 词元类型枚举 r124;
  long long r125;
  enum 词元类型枚举 r126;
  long long r127;
  enum 词元类型枚举 r128;
  long long r129;
  enum 词元类型枚举 r130;
  long long r131;
  enum 词元类型枚举 r132;
  long long r133;
  enum 词元类型枚举 r134;
  long long r135;
  enum 词元类型枚举 r136;
  long long r137;
  enum 词元类型枚举 r138;
  long long r139;
  enum 词元类型枚举 r140;
  long long r141;
  enum 词元类型枚举 r142;
  long long r143;
  enum 词元类型枚举 r144;
  long long r145;
  enum 词元类型枚举 r146;
  long long r147;
  enum 词元类型枚举 r148;
  long long r149;
  enum 词元类型枚举 r150;
  long long r151;
  enum 词元类型枚举 r152;
  long long r153;
  enum 词元类型枚举 r154;
  long long r155;
  enum 词元类型枚举 r156;
  long long r157;
  enum 词元类型枚举 r158;
  long long r159;
  enum 词元类型枚举 r160;
  long long r161;
  enum 词元类型枚举 r162;
  long long r163;

  entry:
  r0 = cn_var_类型;
  r1 = r0 == 词元类型枚举_关键字_如果;
  if (r1) goto if_then_281; else goto if_merge_282;

  if_then_281:
  return "关键字_如果";
  goto if_merge_282;

  if_merge_282:
  r2 = cn_var_类型;
  r3 = r2 == 词元类型枚举_关键字_否则;
  if (r3) goto if_then_283; else goto if_merge_284;

  if_then_283:
  return "关键字_否则";
  goto if_merge_284;

  if_merge_284:
  r4 = cn_var_类型;
  r5 = r4 == 词元类型枚举_关键字_当;
  if (r5) goto if_then_285; else goto if_merge_286;

  if_then_285:
  return "关键字_当";
  goto if_merge_286;

  if_merge_286:
  r6 = cn_var_类型;
  r7 = r6 == 词元类型枚举_关键字_循环;
  if (r7) goto if_then_287; else goto if_merge_288;

  if_then_287:
  return "关键字_循环";
  goto if_merge_288;

  if_merge_288:
  r8 = cn_var_类型;
  r9 = r8 == 词元类型枚举_关键字_返回;
  if (r9) goto if_then_289; else goto if_merge_290;

  if_then_289:
  return "关键字_返回";
  goto if_merge_290;

  if_merge_290:
  r10 = cn_var_类型;
  r11 = r10 == 词元类型枚举_关键字_中断;
  if (r11) goto if_then_291; else goto if_merge_292;

  if_then_291:
  return "关键字_中断";
  goto if_merge_292;

  if_merge_292:
  r12 = cn_var_类型;
  r13 = r12 == 词元类型枚举_关键字_继续;
  if (r13) goto if_then_293; else goto if_merge_294;

  if_then_293:
  return "关键字_继续";
  goto if_merge_294;

  if_merge_294:
  r14 = cn_var_类型;
  r15 = r14 == 词元类型枚举_关键字_选择;
  if (r15) goto if_then_295; else goto if_merge_296;

  if_then_295:
  return "关键字_选择";
  goto if_merge_296;

  if_merge_296:
  r16 = cn_var_类型;
  r17 = r16 == 词元类型枚举_关键字_情况;
  if (r17) goto if_then_297; else goto if_merge_298;

  if_then_297:
  return "关键字_情况";
  goto if_merge_298;

  if_merge_298:
  r18 = cn_var_类型;
  r19 = r18 == 词元类型枚举_关键字_默认;
  if (r19) goto if_then_299; else goto if_merge_300;

  if_then_299:
  return "关键字_默认";
  goto if_merge_300;

  if_merge_300:
  r20 = cn_var_类型;
  r21 = r20 == 词元类型枚举_关键字_整数;
  if (r21) goto if_then_301; else goto if_merge_302;

  if_then_301:
  return "关键字_整数";
  goto if_merge_302;

  if_merge_302:
  r22 = cn_var_类型;
  r23 = r22 == 词元类型枚举_关键字_小数;
  if (r23) goto if_then_303; else goto if_merge_304;

  if_then_303:
  return "关键字_小数";
  goto if_merge_304;

  if_merge_304:
  r24 = cn_var_类型;
  r25 = r24 == 词元类型枚举_关键字_字符串;
  if (r25) goto if_then_305; else goto if_merge_306;

  if_then_305:
  return "关键字_字符串";
  goto if_merge_306;

  if_merge_306:
  r26 = cn_var_类型;
  r27 = r26 == 词元类型枚举_关键字_布尔;
  if (r27) goto if_then_307; else goto if_merge_308;

  if_then_307:
  return "关键字_布尔";
  goto if_merge_308;

  if_merge_308:
  r28 = cn_var_类型;
  r29 = r28 == 词元类型枚举_关键字_空类型;
  if (r29) goto if_then_309; else goto if_merge_310;

  if_then_309:
  return "关键字_空类型";
  goto if_merge_310;

  if_merge_310:
  r30 = cn_var_类型;
  r31 = r30 == 词元类型枚举_关键字_结构体;
  if (r31) goto if_then_311; else goto if_merge_312;

  if_then_311:
  return "关键字_结构体";
  goto if_merge_312;

  if_merge_312:
  r32 = cn_var_类型;
  r33 = r32 == 词元类型枚举_关键字_枚举;
  if (r33) goto if_then_313; else goto if_merge_314;

  if_then_313:
  return "关键字_枚举";
  goto if_merge_314;

  if_merge_314:
  r34 = cn_var_类型;
  r35 = r34 == 词元类型枚举_关键字_函数;
  if (r35) goto if_then_315; else goto if_merge_316;

  if_then_315:
  return "关键字_函数";
  goto if_merge_316;

  if_merge_316:
  r36 = cn_var_类型;
  r37 = r36 == 词元类型枚举_关键字_变量;
  if (r37) goto if_then_317; else goto if_merge_318;

  if_then_317:
  return "关键字_变量";
  goto if_merge_318;

  if_merge_318:
  r38 = cn_var_类型;
  r39 = r38 == 词元类型枚举_关键字_导入;
  if (r39) goto if_then_319; else goto if_merge_320;

  if_then_319:
  return "关键字_导入";
  goto if_merge_320;

  if_merge_320:
  r40 = cn_var_类型;
  r41 = r40 == 词元类型枚举_关键字_从;
  if (r41) goto if_then_321; else goto if_merge_322;

  if_then_321:
  return "关键字_从";
  goto if_merge_322;

  if_merge_322:
  r42 = cn_var_类型;
  r43 = r42 == 词元类型枚举_关键字_公开;
  if (r43) goto if_then_323; else goto if_merge_324;

  if_then_323:
  return "关键字_公开";
  goto if_merge_324;

  if_merge_324:
  r44 = cn_var_类型;
  r45 = r44 == 词元类型枚举_关键字_私有;
  if (r45) goto if_then_325; else goto if_merge_326;

  if_then_325:
  return "关键字_私有";
  goto if_merge_326;

  if_merge_326:
  r46 = cn_var_类型;
  r47 = r46 == 词元类型枚举_关键字_静态;
  if (r47) goto if_then_327; else goto if_merge_328;

  if_then_327:
  return "关键字_静态";
  goto if_merge_328;

  if_merge_328:
  r48 = cn_var_类型;
  r49 = r48 == 词元类型枚举_关键字_真;
  if (r49) goto if_then_329; else goto if_merge_330;

  if_then_329:
  return "关键字_真";
  goto if_merge_330;

  if_merge_330:
  r50 = cn_var_类型;
  r51 = r50 == 词元类型枚举_关键字_假;
  if (r51) goto if_then_331; else goto if_merge_332;

  if_then_331:
  return "关键字_假";
  goto if_merge_332;

  if_merge_332:
  r52 = cn_var_类型;
  r53 = r52 == 词元类型枚举_关键字_无;
  if (r53) goto if_then_333; else goto if_merge_334;

  if_then_333:
  return "关键字_无";
  goto if_merge_334;

  if_merge_334:
  r54 = cn_var_类型;
  r55 = r54 == 词元类型枚举_关键字_类;
  if (r55) goto if_then_335; else goto if_merge_336;

  if_then_335:
  return "关键字_类";
  goto if_merge_336;

  if_merge_336:
  r56 = cn_var_类型;
  r57 = r56 == 词元类型枚举_关键字_接口;
  if (r57) goto if_then_337; else goto if_merge_338;

  if_then_337:
  return "关键字_接口";
  goto if_merge_338;

  if_merge_338:
  r58 = cn_var_类型;
  r59 = r58 == 词元类型枚举_关键字_保护;
  if (r59) goto if_then_339; else goto if_merge_340;

  if_then_339:
  return "关键字_保护";
  goto if_merge_340;

  if_merge_340:
  r60 = cn_var_类型;
  r61 = r60 == 词元类型枚举_关键字_虚拟;
  if (r61) goto if_then_341; else goto if_merge_342;

  if_then_341:
  return "关键字_虚拟";
  goto if_merge_342;

  if_merge_342:
  r62 = cn_var_类型;
  r63 = r62 == 词元类型枚举_关键字_重写;
  if (r63) goto if_then_343; else goto if_merge_344;

  if_then_343:
  return "关键字_重写";
  goto if_merge_344;

  if_merge_344:
  r64 = cn_var_类型;
  r65 = r64 == 词元类型枚举_关键字_抽象;
  if (r65) goto if_then_345; else goto if_merge_346;

  if_then_345:
  return "关键字_抽象";
  goto if_merge_346;

  if_merge_346:
  r66 = cn_var_类型;
  r67 = r66 == 词元类型枚举_关键字_实现;
  if (r67) goto if_then_347; else goto if_merge_348;

  if_then_347:
  return "关键字_实现";
  goto if_merge_348;

  if_merge_348:
  r68 = cn_var_类型;
  r69 = r68 == 词元类型枚举_关键字_自身;
  if (r69) goto if_then_349; else goto if_merge_350;

  if_then_349:
  return "关键字_自身";
  goto if_merge_350;

  if_merge_350:
  r70 = cn_var_类型;
  r71 = r70 == 词元类型枚举_关键字_基类;
  if (r71) goto if_then_351; else goto if_merge_352;

  if_then_351:
  return "关键字_基类";
  goto if_merge_352;

  if_merge_352:
  r72 = cn_var_类型;
  r73 = r72 == 词元类型枚举_关键字_尝试;
  if (r73) goto if_then_353; else goto if_merge_354;

  if_then_353:
  return "关键字_尝试";
  goto if_merge_354;

  if_merge_354:
  r74 = cn_var_类型;
  r75 = r74 == 词元类型枚举_关键字_捕获;
  if (r75) goto if_then_355; else goto if_merge_356;

  if_then_355:
  return "关键字_捕获";
  goto if_merge_356;

  if_merge_356:
  r76 = cn_var_类型;
  r77 = r76 == 词元类型枚举_关键字_抛出;
  if (r77) goto if_then_357; else goto if_merge_358;

  if_then_357:
  return "关键字_抛出";
  goto if_merge_358;

  if_merge_358:
  r78 = cn_var_类型;
  r79 = r78 == 词元类型枚举_关键字_最终;
  if (r79) goto if_then_359; else goto if_merge_360;

  if_then_359:
  return "关键字_最终";
  goto if_merge_360;

  if_merge_360:
  r80 = cn_var_类型;
  r81 = r80 == 词元类型枚举_标识符;
  if (r81) goto if_then_361; else goto if_merge_362;

  if_then_361:
  return "标识符";
  goto if_merge_362;

  if_merge_362:
  r82 = cn_var_类型;
  r83 = r82 == 词元类型枚举_整数字面量;
  if (r83) goto if_then_363; else goto if_merge_364;

  if_then_363:
  return "整数字面量";
  goto if_merge_364;

  if_merge_364:
  r84 = cn_var_类型;
  r85 = r84 == 词元类型枚举_浮点字面量;
  if (r85) goto if_then_365; else goto if_merge_366;

  if_then_365:
  return "浮点字面量";
  goto if_merge_366;

  if_merge_366:
  r86 = cn_var_类型;
  r87 = r86 == 词元类型枚举_字符串字面量;
  if (r87) goto if_then_367; else goto if_merge_368;

  if_then_367:
  return "字符串字面量";
  goto if_merge_368;

  if_merge_368:
  r88 = cn_var_类型;
  r89 = r88 == 词元类型枚举_字符字面量;
  if (r89) goto if_then_369; else goto if_merge_370;

  if_then_369:
  return "字符字面量";
  goto if_merge_370;

  if_merge_370:
  r90 = cn_var_类型;
  r91 = r90 == 词元类型枚举_加号;
  if (r91) goto if_then_371; else goto if_merge_372;

  if_then_371:
  return "加号";
  goto if_merge_372;

  if_merge_372:
  r92 = cn_var_类型;
  r93 = r92 == 词元类型枚举_减号;
  if (r93) goto if_then_373; else goto if_merge_374;

  if_then_373:
  return "减号";
  goto if_merge_374;

  if_merge_374:
  r94 = cn_var_类型;
  r95 = r94 == 词元类型枚举_星号;
  if (r95) goto if_then_375; else goto if_merge_376;

  if_then_375:
  return "星号";
  goto if_merge_376;

  if_merge_376:
  r96 = cn_var_类型;
  r97 = r96 == 词元类型枚举_斜杠;
  if (r97) goto if_then_377; else goto if_merge_378;

  if_then_377:
  return "斜杠";
  goto if_merge_378;

  if_merge_378:
  r98 = cn_var_类型;
  r99 = r98 == 词元类型枚举_百分号;
  if (r99) goto if_then_379; else goto if_merge_380;

  if_then_379:
  return "百分号";
  goto if_merge_380;

  if_merge_380:
  r100 = cn_var_类型;
  r101 = r100 == 词元类型枚举_等于;
  if (r101) goto if_then_381; else goto if_merge_382;

  if_then_381:
  return "等于";
  goto if_merge_382;

  if_merge_382:
  r102 = cn_var_类型;
  r103 = r102 == 词元类型枚举_赋值;
  if (r103) goto if_then_383; else goto if_merge_384;

  if_then_383:
  return "赋值";
  goto if_merge_384;

  if_merge_384:
  r104 = cn_var_类型;
  r105 = r104 == 词元类型枚举_不等于;
  if (r105) goto if_then_385; else goto if_merge_386;

  if_then_385:
  return "不等于";
  goto if_merge_386;

  if_merge_386:
  r106 = cn_var_类型;
  r107 = r106 == 词元类型枚举_小于;
  if (r107) goto if_then_387; else goto if_merge_388;

  if_then_387:
  return "小于";
  goto if_merge_388;

  if_merge_388:
  r108 = cn_var_类型;
  r109 = r108 == 词元类型枚举_小于等于;
  if (r109) goto if_then_389; else goto if_merge_390;

  if_then_389:
  return "小于等于";
  goto if_merge_390;

  if_merge_390:
  r110 = cn_var_类型;
  r111 = r110 == 词元类型枚举_大于;
  if (r111) goto if_then_391; else goto if_merge_392;

  if_then_391:
  return "大于";
  goto if_merge_392;

  if_merge_392:
  r112 = cn_var_类型;
  r113 = r112 == 词元类型枚举_大于等于;
  if (r113) goto if_then_393; else goto if_merge_394;

  if_then_393:
  return "大于等于";
  goto if_merge_394;

  if_merge_394:
  r114 = cn_var_类型;
  r115 = r114 == 词元类型枚举_逻辑与;
  if (r115) goto if_then_395; else goto if_merge_396;

  if_then_395:
  return "逻辑与";
  goto if_merge_396;

  if_merge_396:
  r116 = cn_var_类型;
  r117 = r116 == 词元类型枚举_逻辑或;
  if (r117) goto if_then_397; else goto if_merge_398;

  if_then_397:
  return "逻辑或";
  goto if_merge_398;

  if_merge_398:
  r118 = cn_var_类型;
  r119 = r118 == 词元类型枚举_逻辑非;
  if (r119) goto if_then_399; else goto if_merge_400;

  if_then_399:
  return "逻辑非";
  goto if_merge_400;

  if_merge_400:
  r120 = cn_var_类型;
  r121 = r120 == 词元类型枚举_按位与;
  if (r121) goto if_then_401; else goto if_merge_402;

  if_then_401:
  return "按位与";
  goto if_merge_402;

  if_merge_402:
  r122 = cn_var_类型;
  r123 = r122 == 词元类型枚举_按位或;
  if (r123) goto if_then_403; else goto if_merge_404;

  if_then_403:
  return "按位或";
  goto if_merge_404;

  if_merge_404:
  r124 = cn_var_类型;
  r125 = r124 == 词元类型枚举_按位异或;
  if (r125) goto if_then_405; else goto if_merge_406;

  if_then_405:
  return "按位异或";
  goto if_merge_406;

  if_merge_406:
  r126 = cn_var_类型;
  r127 = r126 == 词元类型枚举_按位取反;
  if (r127) goto if_then_407; else goto if_merge_408;

  if_then_407:
  return "按位取反";
  goto if_merge_408;

  if_merge_408:
  r128 = cn_var_类型;
  r129 = r128 == 词元类型枚举_左移;
  if (r129) goto if_then_409; else goto if_merge_410;

  if_then_409:
  return "左移";
  goto if_merge_410;

  if_merge_410:
  r130 = cn_var_类型;
  r131 = r130 == 词元类型枚举_右移;
  if (r131) goto if_then_411; else goto if_merge_412;

  if_then_411:
  return "右移";
  goto if_merge_412;

  if_merge_412:
  r132 = cn_var_类型;
  r133 = r132 == 词元类型枚举_自增;
  if (r133) goto if_then_413; else goto if_merge_414;

  if_then_413:
  return "自增";
  goto if_merge_414;

  if_merge_414:
  r134 = cn_var_类型;
  r135 = r134 == 词元类型枚举_自减;
  if (r135) goto if_then_415; else goto if_merge_416;

  if_then_415:
  return "自减";
  goto if_merge_416;

  if_merge_416:
  r136 = cn_var_类型;
  r137 = r136 == 词元类型枚举_箭头;
  if (r137) goto if_then_417; else goto if_merge_418;

  if_then_417:
  return "箭头";
  goto if_merge_418;

  if_merge_418:
  r138 = cn_var_类型;
  r139 = r138 == 词元类型枚举_左括号;
  if (r139) goto if_then_419; else goto if_merge_420;

  if_then_419:
  return "左括号";
  goto if_merge_420;

  if_merge_420:
  r140 = cn_var_类型;
  r141 = r140 == 词元类型枚举_右括号;
  if (r141) goto if_then_421; else goto if_merge_422;

  if_then_421:
  return "右括号";
  goto if_merge_422;

  if_merge_422:
  r142 = cn_var_类型;
  r143 = r142 == 词元类型枚举_左大括号;
  if (r143) goto if_then_423; else goto if_merge_424;

  if_then_423:
  return "左大括号";
  goto if_merge_424;

  if_merge_424:
  r144 = cn_var_类型;
  r145 = r144 == 词元类型枚举_右大括号;
  if (r145) goto if_then_425; else goto if_merge_426;

  if_then_425:
  return "右大括号";
  goto if_merge_426;

  if_merge_426:
  r146 = cn_var_类型;
  r147 = r146 == 词元类型枚举_左方括号;
  if (r147) goto if_then_427; else goto if_merge_428;

  if_then_427:
  return "左方括号";
  goto if_merge_428;

  if_merge_428:
  r148 = cn_var_类型;
  r149 = r148 == 词元类型枚举_右方括号;
  if (r149) goto if_then_429; else goto if_merge_430;

  if_then_429:
  return "右方括号";
  goto if_merge_430;

  if_merge_430:
  r150 = cn_var_类型;
  r151 = r150 == 词元类型枚举_分号;
  if (r151) goto if_then_431; else goto if_merge_432;

  if_then_431:
  return "分号";
  goto if_merge_432;

  if_merge_432:
  r152 = cn_var_类型;
  r153 = r152 == 词元类型枚举_逗号;
  if (r153) goto if_then_433; else goto if_merge_434;

  if_then_433:
  return "逗号";
  goto if_merge_434;

  if_merge_434:
  r154 = cn_var_类型;
  r155 = r154 == 词元类型枚举_点;
  if (r155) goto if_then_435; else goto if_merge_436;

  if_then_435:
  return "点";
  goto if_merge_436;

  if_merge_436:
  r156 = cn_var_类型;
  r157 = r156 == 词元类型枚举_冒号;
  if (r157) goto if_then_437; else goto if_merge_438;

  if_then_437:
  return "冒号";
  goto if_merge_438;

  if_merge_438:
  r158 = cn_var_类型;
  r159 = r158 == 词元类型枚举_问号;
  if (r159) goto if_then_439; else goto if_merge_440;

  if_then_439:
  return "问号";
  goto if_merge_440;

  if_merge_440:
  r160 = cn_var_类型;
  r161 = r160 == 词元类型枚举_结束;
  if (r161) goto if_then_441; else goto if_merge_442;

  if_then_441:
  return "结束";
  goto if_merge_442;

  if_merge_442:
  r162 = cn_var_类型;
  r163 = r162 == 词元类型枚举_错误;
  if (r163) goto if_then_443; else goto if_merge_444;

  if_then_443:
  return "错误";
  goto if_merge_444;

  if_merge_444:
  return "未知词元类型";
}

struct 词元 创建词元(enum 词元类型枚举 cn_var_类型, const char* cn_var_值, long long cn_var_行号, long long cn_var_列号, long long cn_var_长度值) {
  enum 词元类型枚举 r0;
  char* r1;
  long long r2;
  long long r3;
  long long r4;
  struct 词元 r5;

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
  long long r0, r5, r10, r15, r20, r25;
  enum 词元类型枚举 r1;
  long long r2;
  enum 词元类型枚举 r3;
  long long r4;
  enum 词元类型枚举 r6;
  long long r7;
  enum 词元类型枚举 r8;
  long long r9;
  enum 词元类型枚举 r11;
  long long r12;
  enum 词元类型枚举 r13;
  long long r14;
  enum 词元类型枚举 r16;
  long long r17;
  enum 词元类型枚举 r18;
  long long r19;
  enum 词元类型枚举 r21;
  long long r22;
  enum 词元类型枚举 r23;
  long long r24;
  enum 词元类型枚举 r26;
  long long r27;
  enum 词元类型枚举 r28;
  long long r29;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_关键字_如果;
  if (r2) goto logic_rhs_447; else goto logic_merge_448;

  if_then_445:
  return 1;
  goto if_merge_446;

  if_merge_446:
  r6 = cn_var_类型;
  r7 = r6 >= 词元类型枚举_关键字_整数;
  if (r7) goto logic_rhs_451; else goto logic_merge_452;

  logic_rhs_447:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_关键字_默认;
  goto logic_merge_448;

  logic_merge_448:
  if (r4) goto if_then_445; else goto if_merge_446;

  if_then_449:
  return 1;
  goto if_merge_450;

  if_merge_450:
  r11 = cn_var_类型;
  r12 = r11 >= 词元类型枚举_关键字_函数;
  if (r12) goto logic_rhs_455; else goto logic_merge_456;

  logic_rhs_451:
  r8 = cn_var_类型;
  r9 = r8 <= 词元类型枚举_关键字_枚举;
  goto logic_merge_452;

  logic_merge_452:
  if (r9) goto if_then_449; else goto if_merge_450;

  if_then_453:
  return 1;
  goto if_merge_454;

  if_merge_454:
  r16 = cn_var_类型;
  r17 = r16 >= 词元类型枚举_关键字_真;
  if (r17) goto logic_rhs_459; else goto logic_merge_460;

  logic_rhs_455:
  r13 = cn_var_类型;
  r14 = r13 <= 词元类型枚举_关键字_静态;
  goto logic_merge_456;

  logic_merge_456:
  if (r14) goto if_then_453; else goto if_merge_454;

  if_then_457:
  return 1;
  goto if_merge_458;

  if_merge_458:
  r21 = cn_var_类型;
  r22 = r21 >= 词元类型枚举_关键字_类;
  if (r22) goto logic_rhs_463; else goto logic_merge_464;

  logic_rhs_459:
  r18 = cn_var_类型;
  r19 = r18 <= 词元类型枚举_关键字_无;
  goto logic_merge_460;

  logic_merge_460:
  if (r19) goto if_then_457; else goto if_merge_458;

  if_then_461:
  return 1;
  goto if_merge_462;

  if_merge_462:
  r26 = cn_var_类型;
  r27 = r26 >= 词元类型枚举_关键字_尝试;
  if (r27) goto logic_rhs_467; else goto logic_merge_468;

  logic_rhs_463:
  r23 = cn_var_类型;
  r24 = r23 <= 词元类型枚举_关键字_基类;
  goto logic_merge_464;

  logic_merge_464:
  if (r24) goto if_then_461; else goto if_merge_462;

  if_then_465:
  return 1;
  goto if_merge_466;

  if_merge_466:
  return 0;

  logic_rhs_467:
  r28 = cn_var_类型;
  r29 = r28 <= 词元类型枚举_关键字_最终;
  goto logic_merge_468;

  logic_merge_468:
  if (r29) goto if_then_465; else goto if_merge_466;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  enum 词元类型枚举 r1;
  long long r2;
  enum 词元类型枚举 r3;
  long long r4;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_标识符;
  if (r2) goto logic_rhs_469; else goto logic_merge_470;

  logic_rhs_469:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_字符字面量;
  goto logic_merge_470;

  logic_merge_470:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  enum 词元类型枚举 r1;
  long long r2;
  enum 词元类型枚举 r3;
  long long r4;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_加号;
  if (r2) goto logic_rhs_471; else goto logic_merge_472;

  logic_rhs_471:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_箭头;
  goto logic_merge_472;

  logic_merge_472:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  enum 词元类型枚举 r1;
  long long r2;
  enum 词元类型枚举 r3;
  long long r4;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 词元类型枚举_左括号;
  if (r2) goto logic_rhs_473; else goto logic_merge_474;

  logic_rhs_473:
  r3 = cn_var_类型;
  r4 = r3 <= 词元类型枚举_问号;
  goto logic_merge_474;

  logic_merge_474:
  return r4;
}

