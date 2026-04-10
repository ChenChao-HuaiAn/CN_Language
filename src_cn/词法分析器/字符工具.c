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
  if (r1) goto if_then_204; else goto if_merge_205;

  if_then_204:
  return 1;
  goto if_merge_205;

  if_merge_205:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_206; else goto if_merge_207;

  if_then_206:
  return 1;
  goto if_merge_207;

  if_merge_207:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_208; else goto if_merge_209;

  if_then_208:
  return 1;
  goto if_merge_209;

  if_merge_209:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_210; else goto if_merge_211;

  if_then_210:
  return 1;
  goto if_merge_211;

  if_merge_211:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r1, r2, r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_212; else goto if_merge_213;

  if_then_212:
  return 1;
  goto if_merge_213;

  if_merge_213:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_214; else goto if_merge_215;

  if_then_214:
  return 1;
  goto if_merge_215;

  if_merge_215:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_218; else goto logic_merge_219;

  if_then_216:
  return 1;
  goto if_merge_217;

  if_merge_217:
  return 0;

  logic_rhs_218:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_219;

  logic_merge_219:
  if (r4) goto if_then_216; else goto if_merge_217;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_222; else goto logic_merge_223;

  if_then_220:
  return 1;
  goto if_merge_221;

  if_merge_221:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_226; else goto logic_merge_227;

  logic_rhs_222:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_223;

  logic_merge_223:
  if (r4) goto if_then_220; else goto if_merge_221;

  if_then_224:
  return 1;
  goto if_merge_225;

  if_merge_225:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_230; else goto logic_merge_231;

  logic_rhs_226:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_227;

  logic_merge_227:
  if (r9) goto if_then_224; else goto if_merge_225;

  if_then_228:
  return 1;
  goto if_merge_229;

  if_merge_229:
  return 0;

  logic_rhs_230:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_231;

  logic_merge_231:
  if (r14) goto if_then_228; else goto if_merge_229;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_235; else goto logic_rhs_234;

  if_then_232:
  return 1;
  goto if_merge_233;

  if_merge_233:
  return 0;

  logic_rhs_234:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_235;

  logic_merge_235:
  if (r4) goto if_then_232; else goto if_merge_233;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_238; else goto logic_merge_239;

  if_then_236:
  return 1;
  goto if_merge_237;

  if_merge_237:
  return 0;

  logic_rhs_238:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_239;

  logic_merge_239:
  if (r4) goto if_then_236; else goto if_merge_237;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_242; else goto logic_merge_243;

  if_then_240:
  return 1;
  goto if_merge_241;

  if_merge_241:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_246; else goto logic_merge_247;

  logic_rhs_242:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_243;

  logic_merge_243:
  if (r4) goto if_then_240; else goto if_merge_241;

  if_then_244:
  return 1;
  goto if_merge_245;

  if_merge_245:
  return 0;

  logic_rhs_246:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_247;

  logic_merge_247:
  if (r9) goto if_then_244; else goto if_merge_245;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_248; else goto if_merge_249;

  if_then_248:
  return 1;
  goto if_merge_249;

  if_merge_249:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_250; else goto if_merge_251;

  if_then_250:
  return 1;
  goto if_merge_251;

  if_merge_251:
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
  if (r3) goto logic_rhs_254; else goto logic_merge_255;

  if_then_252:
  return 1;
  goto if_merge_253;

  if_merge_253:
  return 0;

  logic_rhs_254:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_255;

  logic_merge_255:
  if (r5) goto if_then_252; else goto if_merge_253;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_256; else goto if_merge_257;

  if_then_256:
  return 1;
  goto if_merge_257;

  if_merge_257:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_258; else goto if_merge_259;

  if_then_258:
  return 1;
  goto if_merge_259;

  if_merge_259:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_260; else goto if_merge_261;

  if_then_260:
  return 1;
  goto if_merge_261;

  if_merge_261:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r1, r2, r4, r5, r6;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_262; else goto if_merge_263;

  if_then_262:
  return 1;
  goto if_merge_263;

  if_merge_263:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_264; else goto if_merge_265;

  if_then_264:
  return 1;
  goto if_merge_265;

  if_merge_265:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_266; else goto if_merge_267;

  if_then_266:
  return 1;
  goto if_merge_267;

  if_merge_267:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21, r22, r23, r24, r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_268; else goto if_merge_269;

  if_then_268:
  return 1;
  goto if_merge_269;

  if_merge_269:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_270; else goto if_merge_271;

  if_then_270:
  return 1;
  goto if_merge_271;

  if_merge_271:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_272; else goto if_merge_273;

  if_then_272:
  return 1;
  goto if_merge_273;

  if_merge_273:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_274; else goto if_merge_275;

  if_then_274:
  return 1;
  goto if_merge_275;

  if_merge_275:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_276; else goto if_merge_277;

  if_then_276:
  return 1;
  goto if_merge_277;

  if_merge_277:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_278; else goto if_merge_279;

  if_then_278:
  return 1;
  goto if_merge_279;

  if_merge_279:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_280; else goto if_merge_281;

  if_then_280:
  return 1;
  goto if_merge_281;

  if_merge_281:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_282; else goto if_merge_283;

  if_then_282:
  return 1;
  goto if_merge_283;

  if_merge_283:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_284; else goto if_merge_285;

  if_then_284:
  return 1;
  goto if_merge_285;

  if_merge_285:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_286; else goto if_merge_287;

  if_then_286:
  return 1;
  goto if_merge_287;

  if_merge_287:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_288; else goto if_merge_289;

  if_then_288:
  return 1;
  goto if_merge_289;

  if_merge_289:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_290; else goto if_merge_291;

  if_then_290:
  return 1;
  goto if_merge_291;

  if_merge_291:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_292; else goto if_merge_293;

  if_then_292:
  return 1;
  goto if_merge_293;

  if_merge_293:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17, r18, r19, r20, r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_294; else goto if_merge_295;

  if_then_294:
  return 1;
  goto if_merge_295;

  if_merge_295:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_296; else goto if_merge_297;

  if_then_296:
  return 1;
  goto if_merge_297;

  if_merge_297:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_298; else goto if_merge_299;

  if_then_298:
  return 1;
  goto if_merge_299;

  if_merge_299:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_300; else goto if_merge_301;

  if_then_300:
  return 1;
  goto if_merge_301;

  if_merge_301:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_302; else goto if_merge_303;

  if_then_302:
  return 1;
  goto if_merge_303;

  if_merge_303:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_304; else goto if_merge_305;

  if_then_304:
  return 1;
  goto if_merge_305;

  if_merge_305:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_306; else goto if_merge_307;

  if_then_306:
  return 1;
  goto if_merge_307;

  if_merge_307:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_308; else goto if_merge_309;

  if_then_308:
  return 1;
  goto if_merge_309;

  if_merge_309:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_310; else goto if_merge_311;

  if_then_310:
  return 1;
  goto if_merge_311;

  if_merge_311:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_312; else goto if_merge_313;

  if_then_312:
  return 1;
  goto if_merge_313;

  if_merge_313:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_314; else goto if_merge_315;

  if_then_314:
  return 1;
  goto if_merge_315;

  if_merge_315:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_318; else goto logic_merge_319;

  if_then_316:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_317;

  if_merge_317:
  r8 = cn_var_c;
  return r8;

  logic_rhs_318:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_319;

  logic_merge_319:
  if (r4) goto if_then_316; else goto if_merge_317;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r2, r3, r4, r5, r6, r7, r8;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_322; else goto logic_merge_323;

  if_then_320:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_321;

  if_merge_321:
  r8 = cn_var_c;
  return r8;

  logic_rhs_322:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_323;

  logic_merge_323:
  if (r4) goto if_then_320; else goto if_merge_321;
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
  if (r3) goto logic_rhs_326; else goto logic_merge_327;

  if_then_324:
  return 1;
  goto if_merge_325;

  if_merge_325:
  return 0;

  logic_rhs_326:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_327;

  logic_merge_327:
  if (r5) goto if_then_324; else goto if_merge_325;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_330; else goto logic_merge_331;

  if_then_328:
  return 1;
  goto if_merge_329;

  if_merge_329:
  return 0;

  logic_rhs_330:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_333; else goto logic_rhs_332;

  logic_merge_331:
  if (r7) goto if_then_328; else goto if_merge_329;

  logic_rhs_332:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_333;

  logic_merge_333:
  goto logic_merge_331;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_336; else goto logic_merge_337;

  if_then_334:
  return 1;
  goto if_merge_335;

  if_merge_335:
  return 0;

  logic_rhs_336:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_339; else goto logic_rhs_338;

  logic_merge_337:
  if (r7) goto if_then_334; else goto if_merge_335;

  logic_rhs_338:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_339;

  logic_merge_339:
  goto logic_merge_337;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4, r5, r6, r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_342; else goto logic_merge_343;

  if_then_340:
  return 1;
  goto if_merge_341;

  if_merge_341:
  return 0;

  logic_rhs_342:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_345; else goto logic_rhs_344;

  logic_merge_343:
  if (r7) goto if_then_340; else goto if_merge_341;

  logic_rhs_344:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_345;

  logic_merge_345:
  goto logic_merge_343;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_348; else goto logic_merge_349;

  if_then_346:
  return 1;
  goto if_merge_347;

  if_merge_347:
  return 0;

  logic_rhs_348:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_349;

  logic_merge_349:
  if (r4) goto if_then_346; else goto if_merge_347;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_352; else goto logic_merge_353;

  if_then_350:
  return 1;
  goto if_merge_351;

  if_merge_351:
  return 0;

  logic_rhs_352:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_353;

  logic_merge_353:
  if (r4) goto if_then_350; else goto if_merge_351;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r2, r3, r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_356; else goto logic_merge_357;

  if_then_354:
  return 1;
  goto if_merge_355;

  if_merge_355:
  return 0;

  logic_rhs_356:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_357;

  logic_merge_357:
  if (r4) goto if_then_354; else goto if_merge_355;
  return 0;
}

