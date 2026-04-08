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
struct 词元 创建词元(enum 词元类型枚举, char*, long long, long long, long long);
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
  r1 = r0 == 0;
  if (r1) goto if_then_221; else goto if_merge_222;

  if_then_221:
  return "关键字_如果";
  goto if_merge_222;

  if_merge_222:
  r2 = cn_var_类型;
  r3 = r2 == 1;
  if (r3) goto if_then_223; else goto if_merge_224;

  if_then_223:
  return "关键字_否则";
  goto if_merge_224;

  if_merge_224:
  r4 = cn_var_类型;
  r5 = r4 == 2;
  if (r5) goto if_then_225; else goto if_merge_226;

  if_then_225:
  return "关键字_当";
  goto if_merge_226;

  if_merge_226:
  r6 = cn_var_类型;
  r7 = r6 == 3;
  if (r7) goto if_then_227; else goto if_merge_228;

  if_then_227:
  return "关键字_循环";
  goto if_merge_228;

  if_merge_228:
  r8 = cn_var_类型;
  r9 = r8 == 4;
  if (r9) goto if_then_229; else goto if_merge_230;

  if_then_229:
  return "关键字_返回";
  goto if_merge_230;

  if_merge_230:
  r10 = cn_var_类型;
  r11 = r10 == 5;
  if (r11) goto if_then_231; else goto if_merge_232;

  if_then_231:
  return "关键字_中断";
  goto if_merge_232;

  if_merge_232:
  r12 = cn_var_类型;
  r13 = r12 == 6;
  if (r13) goto if_then_233; else goto if_merge_234;

  if_then_233:
  return "关键字_继续";
  goto if_merge_234;

  if_merge_234:
  r14 = cn_var_类型;
  r15 = r14 == 7;
  if (r15) goto if_then_235; else goto if_merge_236;

  if_then_235:
  return "关键字_选择";
  goto if_merge_236;

  if_merge_236:
  r16 = cn_var_类型;
  r17 = r16 == 8;
  if (r17) goto if_then_237; else goto if_merge_238;

  if_then_237:
  return "关键字_情况";
  goto if_merge_238;

  if_merge_238:
  r18 = cn_var_类型;
  r19 = r18 == 9;
  if (r19) goto if_then_239; else goto if_merge_240;

  if_then_239:
  return "关键字_默认";
  goto if_merge_240;

  if_merge_240:
  r20 = cn_var_类型;
  r21 = r20 == 10;
  if (r21) goto if_then_241; else goto if_merge_242;

  if_then_241:
  return "关键字_整数";
  goto if_merge_242;

  if_merge_242:
  r22 = cn_var_类型;
  r23 = r22 == 11;
  if (r23) goto if_then_243; else goto if_merge_244;

  if_then_243:
  return "关键字_小数";
  goto if_merge_244;

  if_merge_244:
  r24 = cn_var_类型;
  r25 = r24 == 12;
  if (r25) goto if_then_245; else goto if_merge_246;

  if_then_245:
  return "关键字_字符串";
  goto if_merge_246;

  if_merge_246:
  r26 = cn_var_类型;
  r27 = r26 == 13;
  if (r27) goto if_then_247; else goto if_merge_248;

  if_then_247:
  return "关键字_布尔";
  goto if_merge_248;

  if_merge_248:
  r28 = cn_var_类型;
  r29 = r28 == 14;
  if (r29) goto if_then_249; else goto if_merge_250;

  if_then_249:
  return "关键字_空类型";
  goto if_merge_250;

  if_merge_250:
  r30 = cn_var_类型;
  r31 = r30 == 15;
  if (r31) goto if_then_251; else goto if_merge_252;

  if_then_251:
  return "关键字_结构体";
  goto if_merge_252;

  if_merge_252:
  r32 = cn_var_类型;
  r33 = r32 == 16;
  if (r33) goto if_then_253; else goto if_merge_254;

  if_then_253:
  return "关键字_枚举";
  goto if_merge_254;

  if_merge_254:
  r34 = cn_var_类型;
  r35 = r34 == 17;
  if (r35) goto if_then_255; else goto if_merge_256;

  if_then_255:
  return "关键字_函数";
  goto if_merge_256;

  if_merge_256:
  r36 = cn_var_类型;
  r37 = r36 == 18;
  if (r37) goto if_then_257; else goto if_merge_258;

  if_then_257:
  return "关键字_变量";
  goto if_merge_258;

  if_merge_258:
  r38 = cn_var_类型;
  r39 = r38 == 19;
  if (r39) goto if_then_259; else goto if_merge_260;

  if_then_259:
  return "关键字_导入";
  goto if_merge_260;

  if_merge_260:
  r40 = cn_var_类型;
  r41 = r40 == 20;
  if (r41) goto if_then_261; else goto if_merge_262;

  if_then_261:
  return "关键字_从";
  goto if_merge_262;

  if_merge_262:
  r42 = cn_var_类型;
  r43 = r42 == 21;
  if (r43) goto if_then_263; else goto if_merge_264;

  if_then_263:
  return "关键字_公开";
  goto if_merge_264;

  if_merge_264:
  r44 = cn_var_类型;
  r45 = r44 == 22;
  if (r45) goto if_then_265; else goto if_merge_266;

  if_then_265:
  return "关键字_私有";
  goto if_merge_266;

  if_merge_266:
  r46 = cn_var_类型;
  r47 = r46 == 23;
  if (r47) goto if_then_267; else goto if_merge_268;

  if_then_267:
  return "关键字_静态";
  goto if_merge_268;

  if_merge_268:
  r48 = cn_var_类型;
  r49 = r48 == 24;
  if (r49) goto if_then_269; else goto if_merge_270;

  if_then_269:
  return "关键字_真";
  goto if_merge_270;

  if_merge_270:
  r50 = cn_var_类型;
  r51 = r50 == 25;
  if (r51) goto if_then_271; else goto if_merge_272;

  if_then_271:
  return "关键字_假";
  goto if_merge_272;

  if_merge_272:
  r52 = cn_var_类型;
  r53 = r52 == 26;
  if (r53) goto if_then_273; else goto if_merge_274;

  if_then_273:
  return "关键字_无";
  goto if_merge_274;

  if_merge_274:
  r54 = cn_var_类型;
  r55 = r54 == 27;
  if (r55) goto if_then_275; else goto if_merge_276;

  if_then_275:
  return "关键字_类";
  goto if_merge_276;

  if_merge_276:
  r56 = cn_var_类型;
  r57 = r56 == 28;
  if (r57) goto if_then_277; else goto if_merge_278;

  if_then_277:
  return "关键字_接口";
  goto if_merge_278;

  if_merge_278:
  r58 = cn_var_类型;
  r59 = r58 == 29;
  if (r59) goto if_then_279; else goto if_merge_280;

  if_then_279:
  return "关键字_保护";
  goto if_merge_280;

  if_merge_280:
  r60 = cn_var_类型;
  r61 = r60 == 30;
  if (r61) goto if_then_281; else goto if_merge_282;

  if_then_281:
  return "关键字_虚拟";
  goto if_merge_282;

  if_merge_282:
  r62 = cn_var_类型;
  r63 = r62 == 31;
  if (r63) goto if_then_283; else goto if_merge_284;

  if_then_283:
  return "关键字_重写";
  goto if_merge_284;

  if_merge_284:
  r64 = cn_var_类型;
  r65 = r64 == 32;
  if (r65) goto if_then_285; else goto if_merge_286;

  if_then_285:
  return "关键字_抽象";
  goto if_merge_286;

  if_merge_286:
  r66 = cn_var_类型;
  r67 = r66 == 33;
  if (r67) goto if_then_287; else goto if_merge_288;

  if_then_287:
  return "关键字_实现";
  goto if_merge_288;

  if_merge_288:
  r68 = cn_var_类型;
  r69 = r68 == 34;
  if (r69) goto if_then_289; else goto if_merge_290;

  if_then_289:
  return "关键字_自身";
  goto if_merge_290;

  if_merge_290:
  r70 = cn_var_类型;
  r71 = r70 == 35;
  if (r71) goto if_then_291; else goto if_merge_292;

  if_then_291:
  return "关键字_基类";
  goto if_merge_292;

  if_merge_292:
  r72 = cn_var_类型;
  r73 = r72 == 36;
  if (r73) goto if_then_293; else goto if_merge_294;

  if_then_293:
  return "关键字_尝试";
  goto if_merge_294;

  if_merge_294:
  r74 = cn_var_类型;
  r75 = r74 == 37;
  if (r75) goto if_then_295; else goto if_merge_296;

  if_then_295:
  return "关键字_捕获";
  goto if_merge_296;

  if_merge_296:
  r76 = cn_var_类型;
  r77 = r76 == 38;
  if (r77) goto if_then_297; else goto if_merge_298;

  if_then_297:
  return "关键字_抛出";
  goto if_merge_298;

  if_merge_298:
  r78 = cn_var_类型;
  r79 = r78 == 39;
  if (r79) goto if_then_299; else goto if_merge_300;

  if_then_299:
  return "关键字_最终";
  goto if_merge_300;

  if_merge_300:
  r80 = cn_var_类型;
  r81 = r80 == 40;
  if (r81) goto if_then_301; else goto if_merge_302;

  if_then_301:
  return "标识符";
  goto if_merge_302;

  if_merge_302:
  r82 = cn_var_类型;
  r83 = r82 == 41;
  if (r83) goto if_then_303; else goto if_merge_304;

  if_then_303:
  return "整数字面量";
  goto if_merge_304;

  if_merge_304:
  r84 = cn_var_类型;
  r85 = r84 == 42;
  if (r85) goto if_then_305; else goto if_merge_306;

  if_then_305:
  return "浮点字面量";
  goto if_merge_306;

  if_merge_306:
  r86 = cn_var_类型;
  r87 = r86 == 43;
  if (r87) goto if_then_307; else goto if_merge_308;

  if_then_307:
  return "字符串字面量";
  goto if_merge_308;

  if_merge_308:
  r88 = cn_var_类型;
  r89 = r88 == 44;
  if (r89) goto if_then_309; else goto if_merge_310;

  if_then_309:
  return "字符字面量";
  goto if_merge_310;

  if_merge_310:
  r90 = cn_var_类型;
  r91 = r90 == 45;
  if (r91) goto if_then_311; else goto if_merge_312;

  if_then_311:
  return "加号";
  goto if_merge_312;

  if_merge_312:
  r92 = cn_var_类型;
  r93 = r92 == 46;
  if (r93) goto if_then_313; else goto if_merge_314;

  if_then_313:
  return "减号";
  goto if_merge_314;

  if_merge_314:
  r94 = cn_var_类型;
  r95 = r94 == 47;
  if (r95) goto if_then_315; else goto if_merge_316;

  if_then_315:
  return "星号";
  goto if_merge_316;

  if_merge_316:
  r96 = cn_var_类型;
  r97 = r96 == 48;
  if (r97) goto if_then_317; else goto if_merge_318;

  if_then_317:
  return "斜杠";
  goto if_merge_318;

  if_merge_318:
  r98 = cn_var_类型;
  r99 = r98 == 49;
  if (r99) goto if_then_319; else goto if_merge_320;

  if_then_319:
  return "百分号";
  goto if_merge_320;

  if_merge_320:
  r100 = cn_var_类型;
  r101 = r100 == 50;
  if (r101) goto if_then_321; else goto if_merge_322;

  if_then_321:
  return "等于";
  goto if_merge_322;

  if_merge_322:
  r102 = cn_var_类型;
  r103 = r102 == 51;
  if (r103) goto if_then_323; else goto if_merge_324;

  if_then_323:
  return "赋值";
  goto if_merge_324;

  if_merge_324:
  r104 = cn_var_类型;
  r105 = r104 == 52;
  if (r105) goto if_then_325; else goto if_merge_326;

  if_then_325:
  return "不等于";
  goto if_merge_326;

  if_merge_326:
  r106 = cn_var_类型;
  r107 = r106 == 53;
  if (r107) goto if_then_327; else goto if_merge_328;

  if_then_327:
  return "小于";
  goto if_merge_328;

  if_merge_328:
  r108 = cn_var_类型;
  r109 = r108 == 54;
  if (r109) goto if_then_329; else goto if_merge_330;

  if_then_329:
  return "小于等于";
  goto if_merge_330;

  if_merge_330:
  r110 = cn_var_类型;
  r111 = r110 == 55;
  if (r111) goto if_then_331; else goto if_merge_332;

  if_then_331:
  return "大于";
  goto if_merge_332;

  if_merge_332:
  r112 = cn_var_类型;
  r113 = r112 == 56;
  if (r113) goto if_then_333; else goto if_merge_334;

  if_then_333:
  return "大于等于";
  goto if_merge_334;

  if_merge_334:
  r114 = cn_var_类型;
  r115 = r114 == 57;
  if (r115) goto if_then_335; else goto if_merge_336;

  if_then_335:
  return "逻辑与";
  goto if_merge_336;

  if_merge_336:
  r116 = cn_var_类型;
  r117 = r116 == 58;
  if (r117) goto if_then_337; else goto if_merge_338;

  if_then_337:
  return "逻辑或";
  goto if_merge_338;

  if_merge_338:
  r118 = cn_var_类型;
  r119 = r118 == 59;
  if (r119) goto if_then_339; else goto if_merge_340;

  if_then_339:
  return "逻辑非";
  goto if_merge_340;

  if_merge_340:
  r120 = cn_var_类型;
  r121 = r120 == 60;
  if (r121) goto if_then_341; else goto if_merge_342;

  if_then_341:
  return "按位与";
  goto if_merge_342;

  if_merge_342:
  r122 = cn_var_类型;
  r123 = r122 == 61;
  if (r123) goto if_then_343; else goto if_merge_344;

  if_then_343:
  return "按位或";
  goto if_merge_344;

  if_merge_344:
  r124 = cn_var_类型;
  r125 = r124 == 62;
  if (r125) goto if_then_345; else goto if_merge_346;

  if_then_345:
  return "按位异或";
  goto if_merge_346;

  if_merge_346:
  r126 = cn_var_类型;
  r127 = r126 == 63;
  if (r127) goto if_then_347; else goto if_merge_348;

  if_then_347:
  return "按位取反";
  goto if_merge_348;

  if_merge_348:
  r128 = cn_var_类型;
  r129 = r128 == 64;
  if (r129) goto if_then_349; else goto if_merge_350;

  if_then_349:
  return "左移";
  goto if_merge_350;

  if_merge_350:
  r130 = cn_var_类型;
  r131 = r130 == 65;
  if (r131) goto if_then_351; else goto if_merge_352;

  if_then_351:
  return "右移";
  goto if_merge_352;

  if_merge_352:
  r132 = cn_var_类型;
  r133 = r132 == 66;
  if (r133) goto if_then_353; else goto if_merge_354;

  if_then_353:
  return "自增";
  goto if_merge_354;

  if_merge_354:
  r134 = cn_var_类型;
  r135 = r134 == 67;
  if (r135) goto if_then_355; else goto if_merge_356;

  if_then_355:
  return "自减";
  goto if_merge_356;

  if_merge_356:
  r136 = cn_var_类型;
  r137 = r136 == 68;
  if (r137) goto if_then_357; else goto if_merge_358;

  if_then_357:
  return "箭头";
  goto if_merge_358;

  if_merge_358:
  r138 = cn_var_类型;
  r139 = r138 == 69;
  if (r139) goto if_then_359; else goto if_merge_360;

  if_then_359:
  return "左括号";
  goto if_merge_360;

  if_merge_360:
  r140 = cn_var_类型;
  r141 = r140 == 70;
  if (r141) goto if_then_361; else goto if_merge_362;

  if_then_361:
  return "右括号";
  goto if_merge_362;

  if_merge_362:
  r142 = cn_var_类型;
  r143 = r142 == 71;
  if (r143) goto if_then_363; else goto if_merge_364;

  if_then_363:
  return "左大括号";
  goto if_merge_364;

  if_merge_364:
  r144 = cn_var_类型;
  r145 = r144 == 72;
  if (r145) goto if_then_365; else goto if_merge_366;

  if_then_365:
  return "右大括号";
  goto if_merge_366;

  if_merge_366:
  r146 = cn_var_类型;
  r147 = r146 == 73;
  if (r147) goto if_then_367; else goto if_merge_368;

  if_then_367:
  return "左方括号";
  goto if_merge_368;

  if_merge_368:
  r148 = cn_var_类型;
  r149 = r148 == 74;
  if (r149) goto if_then_369; else goto if_merge_370;

  if_then_369:
  return "右方括号";
  goto if_merge_370;

  if_merge_370:
  r150 = cn_var_类型;
  r151 = r150 == 75;
  if (r151) goto if_then_371; else goto if_merge_372;

  if_then_371:
  return "分号";
  goto if_merge_372;

  if_merge_372:
  r152 = cn_var_类型;
  r153 = r152 == 76;
  if (r153) goto if_then_373; else goto if_merge_374;

  if_then_373:
  return "逗号";
  goto if_merge_374;

  if_merge_374:
  r154 = cn_var_类型;
  r155 = r154 == 77;
  if (r155) goto if_then_375; else goto if_merge_376;

  if_then_375:
  return "点";
  goto if_merge_376;

  if_merge_376:
  r156 = cn_var_类型;
  r157 = r156 == 78;
  if (r157) goto if_then_377; else goto if_merge_378;

  if_then_377:
  return "冒号";
  goto if_merge_378;

  if_merge_378:
  r158 = cn_var_类型;
  r159 = r158 == 79;
  if (r159) goto if_then_379; else goto if_merge_380;

  if_then_379:
  return "问号";
  goto if_merge_380;

  if_merge_380:
  r160 = cn_var_类型;
  r161 = r160 == 80;
  if (r161) goto if_then_381; else goto if_merge_382;

  if_then_381:
  return "结束";
  goto if_merge_382;

  if_merge_382:
  r162 = cn_var_类型;
  r163 = r162 == 81;
  if (r163) goto if_then_383; else goto if_merge_384;

  if_then_383:
  return "错误";
  goto if_merge_384;

  if_merge_384:
  return "未知词元类型";
}

struct 词元 创建词元(enum 词元类型枚举 cn_var_类型, char* cn_var_值, long long cn_var_行号, long long cn_var_列号, long long cn_var_长度值) {
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
  r2 = r1 >= 0;
  if (r2) goto logic_rhs_387; else goto logic_merge_388;

  if_then_385:
  return 1;
  goto if_merge_386;

  if_merge_386:
  r6 = cn_var_类型;
  r7 = r6 >= 10;
  if (r7) goto logic_rhs_391; else goto logic_merge_392;

  logic_rhs_387:
  r3 = cn_var_类型;
  r4 = r3 <= 9;
  goto logic_merge_388;

  logic_merge_388:
  if (r4) goto if_then_385; else goto if_merge_386;

  if_then_389:
  return 1;
  goto if_merge_390;

  if_merge_390:
  r11 = cn_var_类型;
  r12 = r11 >= 17;
  if (r12) goto logic_rhs_395; else goto logic_merge_396;

  logic_rhs_391:
  r8 = cn_var_类型;
  r9 = r8 <= 16;
  goto logic_merge_392;

  logic_merge_392:
  if (r9) goto if_then_389; else goto if_merge_390;

  if_then_393:
  return 1;
  goto if_merge_394;

  if_merge_394:
  r16 = cn_var_类型;
  r17 = r16 >= 24;
  if (r17) goto logic_rhs_399; else goto logic_merge_400;

  logic_rhs_395:
  r13 = cn_var_类型;
  r14 = r13 <= 23;
  goto logic_merge_396;

  logic_merge_396:
  if (r14) goto if_then_393; else goto if_merge_394;

  if_then_397:
  return 1;
  goto if_merge_398;

  if_merge_398:
  r21 = cn_var_类型;
  r22 = r21 >= 27;
  if (r22) goto logic_rhs_403; else goto logic_merge_404;

  logic_rhs_399:
  r18 = cn_var_类型;
  r19 = r18 <= 26;
  goto logic_merge_400;

  logic_merge_400:
  if (r19) goto if_then_397; else goto if_merge_398;

  if_then_401:
  return 1;
  goto if_merge_402;

  if_merge_402:
  r26 = cn_var_类型;
  r27 = r26 >= 36;
  if (r27) goto logic_rhs_407; else goto logic_merge_408;

  logic_rhs_403:
  r23 = cn_var_类型;
  r24 = r23 <= 35;
  goto logic_merge_404;

  logic_merge_404:
  if (r24) goto if_then_401; else goto if_merge_402;

  if_then_405:
  return 1;
  goto if_merge_406;

  if_merge_406:
  return 0;

  logic_rhs_407:
  r28 = cn_var_类型;
  r29 = r28 <= 39;
  goto logic_merge_408;

  logic_merge_408:
  if (r29) goto if_then_405; else goto if_merge_406;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 40;
  if (r2) goto logic_rhs_409; else goto logic_merge_410;

  logic_rhs_409:
  r3 = cn_var_类型;
  r4 = r3 <= 44;
  goto logic_merge_410;

  logic_merge_410:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 45;
  if (r2) goto logic_rhs_411; else goto logic_merge_412;

  logic_rhs_411:
  r3 = cn_var_类型;
  r4 = r3 <= 68;
  goto logic_merge_412;

  logic_merge_412:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 69;
  if (r2) goto logic_rhs_413; else goto logic_merge_414;

  logic_rhs_413:
  r3 = cn_var_类型;
  r4 = r3 <= 79;
  goto logic_merge_414;

  logic_merge_414:
  return r4;
}

