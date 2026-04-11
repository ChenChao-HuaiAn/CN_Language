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
  if (r1) goto if_then_97; else goto if_merge_98;

  if_then_97:
  return "关键字_如果";
  goto if_merge_98;

  if_merge_98:
  r2 = cn_var_类型;
  r3 = r2 == 1;
  if (r3) goto if_then_99; else goto if_merge_100;

  if_then_99:
  return "关键字_否则";
  goto if_merge_100;

  if_merge_100:
  r4 = cn_var_类型;
  r5 = r4 == 2;
  if (r5) goto if_then_101; else goto if_merge_102;

  if_then_101:
  return "关键字_当";
  goto if_merge_102;

  if_merge_102:
  r6 = cn_var_类型;
  r7 = r6 == 3;
  if (r7) goto if_then_103; else goto if_merge_104;

  if_then_103:
  return "关键字_循环";
  goto if_merge_104;

  if_merge_104:
  r8 = cn_var_类型;
  r9 = r8 == 4;
  if (r9) goto if_then_105; else goto if_merge_106;

  if_then_105:
  return "关键字_返回";
  goto if_merge_106;

  if_merge_106:
  r10 = cn_var_类型;
  r11 = r10 == 5;
  if (r11) goto if_then_107; else goto if_merge_108;

  if_then_107:
  return "关键字_中断";
  goto if_merge_108;

  if_merge_108:
  r12 = cn_var_类型;
  r13 = r12 == 6;
  if (r13) goto if_then_109; else goto if_merge_110;

  if_then_109:
  return "关键字_继续";
  goto if_merge_110;

  if_merge_110:
  r14 = cn_var_类型;
  r15 = r14 == 7;
  if (r15) goto if_then_111; else goto if_merge_112;

  if_then_111:
  return "关键字_选择";
  goto if_merge_112;

  if_merge_112:
  r16 = cn_var_类型;
  r17 = r16 == 8;
  if (r17) goto if_then_113; else goto if_merge_114;

  if_then_113:
  return "关键字_情况";
  goto if_merge_114;

  if_merge_114:
  r18 = cn_var_类型;
  r19 = r18 == 9;
  if (r19) goto if_then_115; else goto if_merge_116;

  if_then_115:
  return "关键字_默认";
  goto if_merge_116;

  if_merge_116:
  r20 = cn_var_类型;
  r21 = r20 == 10;
  if (r21) goto if_then_117; else goto if_merge_118;

  if_then_117:
  return "关键字_整数";
  goto if_merge_118;

  if_merge_118:
  r22 = cn_var_类型;
  r23 = r22 == 11;
  if (r23) goto if_then_119; else goto if_merge_120;

  if_then_119:
  return "关键字_小数";
  goto if_merge_120;

  if_merge_120:
  r24 = cn_var_类型;
  r25 = r24 == 12;
  if (r25) goto if_then_121; else goto if_merge_122;

  if_then_121:
  return "关键字_字符串";
  goto if_merge_122;

  if_merge_122:
  r26 = cn_var_类型;
  r27 = r26 == 13;
  if (r27) goto if_then_123; else goto if_merge_124;

  if_then_123:
  return "关键字_布尔";
  goto if_merge_124;

  if_merge_124:
  r28 = cn_var_类型;
  r29 = r28 == 14;
  if (r29) goto if_then_125; else goto if_merge_126;

  if_then_125:
  return "关键字_空类型";
  goto if_merge_126;

  if_merge_126:
  r30 = cn_var_类型;
  r31 = r30 == 15;
  if (r31) goto if_then_127; else goto if_merge_128;

  if_then_127:
  return "关键字_结构体";
  goto if_merge_128;

  if_merge_128:
  r32 = cn_var_类型;
  r33 = r32 == 16;
  if (r33) goto if_then_129; else goto if_merge_130;

  if_then_129:
  return "关键字_枚举";
  goto if_merge_130;

  if_merge_130:
  r34 = cn_var_类型;
  r35 = r34 == 17;
  if (r35) goto if_then_131; else goto if_merge_132;

  if_then_131:
  return "关键字_函数";
  goto if_merge_132;

  if_merge_132:
  r36 = cn_var_类型;
  r37 = r36 == 18;
  if (r37) goto if_then_133; else goto if_merge_134;

  if_then_133:
  return "关键字_变量";
  goto if_merge_134;

  if_merge_134:
  r38 = cn_var_类型;
  r39 = r38 == 19;
  if (r39) goto if_then_135; else goto if_merge_136;

  if_then_135:
  return "关键字_导入";
  goto if_merge_136;

  if_merge_136:
  r40 = cn_var_类型;
  r41 = r40 == 20;
  if (r41) goto if_then_137; else goto if_merge_138;

  if_then_137:
  return "关键字_从";
  goto if_merge_138;

  if_merge_138:
  r42 = cn_var_类型;
  r43 = r42 == 21;
  if (r43) goto if_then_139; else goto if_merge_140;

  if_then_139:
  return "关键字_公开";
  goto if_merge_140;

  if_merge_140:
  r44 = cn_var_类型;
  r45 = r44 == 22;
  if (r45) goto if_then_141; else goto if_merge_142;

  if_then_141:
  return "关键字_私有";
  goto if_merge_142;

  if_merge_142:
  r46 = cn_var_类型;
  r47 = r46 == 23;
  if (r47) goto if_then_143; else goto if_merge_144;

  if_then_143:
  return "关键字_静态";
  goto if_merge_144;

  if_merge_144:
  r48 = cn_var_类型;
  r49 = r48 == 24;
  if (r49) goto if_then_145; else goto if_merge_146;

  if_then_145:
  return "关键字_真";
  goto if_merge_146;

  if_merge_146:
  r50 = cn_var_类型;
  r51 = r50 == 25;
  if (r51) goto if_then_147; else goto if_merge_148;

  if_then_147:
  return "关键字_假";
  goto if_merge_148;

  if_merge_148:
  r52 = cn_var_类型;
  r53 = r52 == 26;
  if (r53) goto if_then_149; else goto if_merge_150;

  if_then_149:
  return "关键字_无";
  goto if_merge_150;

  if_merge_150:
  r54 = cn_var_类型;
  r55 = r54 == 27;
  if (r55) goto if_then_151; else goto if_merge_152;

  if_then_151:
  return "关键字_类";
  goto if_merge_152;

  if_merge_152:
  r56 = cn_var_类型;
  r57 = r56 == 28;
  if (r57) goto if_then_153; else goto if_merge_154;

  if_then_153:
  return "关键字_接口";
  goto if_merge_154;

  if_merge_154:
  r58 = cn_var_类型;
  r59 = r58 == 29;
  if (r59) goto if_then_155; else goto if_merge_156;

  if_then_155:
  return "关键字_保护";
  goto if_merge_156;

  if_merge_156:
  r60 = cn_var_类型;
  r61 = r60 == 30;
  if (r61) goto if_then_157; else goto if_merge_158;

  if_then_157:
  return "关键字_虚拟";
  goto if_merge_158;

  if_merge_158:
  r62 = cn_var_类型;
  r63 = r62 == 31;
  if (r63) goto if_then_159; else goto if_merge_160;

  if_then_159:
  return "关键字_重写";
  goto if_merge_160;

  if_merge_160:
  r64 = cn_var_类型;
  r65 = r64 == 32;
  if (r65) goto if_then_161; else goto if_merge_162;

  if_then_161:
  return "关键字_抽象";
  goto if_merge_162;

  if_merge_162:
  r66 = cn_var_类型;
  r67 = r66 == 33;
  if (r67) goto if_then_163; else goto if_merge_164;

  if_then_163:
  return "关键字_实现";
  goto if_merge_164;

  if_merge_164:
  r68 = cn_var_类型;
  r69 = r68 == 34;
  if (r69) goto if_then_165; else goto if_merge_166;

  if_then_165:
  return "关键字_自身";
  goto if_merge_166;

  if_merge_166:
  r70 = cn_var_类型;
  r71 = r70 == 35;
  if (r71) goto if_then_167; else goto if_merge_168;

  if_then_167:
  return "关键字_基类";
  goto if_merge_168;

  if_merge_168:
  r72 = cn_var_类型;
  r73 = r72 == 36;
  if (r73) goto if_then_169; else goto if_merge_170;

  if_then_169:
  return "关键字_尝试";
  goto if_merge_170;

  if_merge_170:
  r74 = cn_var_类型;
  r75 = r74 == 37;
  if (r75) goto if_then_171; else goto if_merge_172;

  if_then_171:
  return "关键字_捕获";
  goto if_merge_172;

  if_merge_172:
  r76 = cn_var_类型;
  r77 = r76 == 38;
  if (r77) goto if_then_173; else goto if_merge_174;

  if_then_173:
  return "关键字_抛出";
  goto if_merge_174;

  if_merge_174:
  r78 = cn_var_类型;
  r79 = r78 == 39;
  if (r79) goto if_then_175; else goto if_merge_176;

  if_then_175:
  return "关键字_最终";
  goto if_merge_176;

  if_merge_176:
  r80 = cn_var_类型;
  r81 = r80 == 40;
  if (r81) goto if_then_177; else goto if_merge_178;

  if_then_177:
  return "标识符";
  goto if_merge_178;

  if_merge_178:
  r82 = cn_var_类型;
  r83 = r82 == 41;
  if (r83) goto if_then_179; else goto if_merge_180;

  if_then_179:
  return "整数字面量";
  goto if_merge_180;

  if_merge_180:
  r84 = cn_var_类型;
  r85 = r84 == 42;
  if (r85) goto if_then_181; else goto if_merge_182;

  if_then_181:
  return "浮点字面量";
  goto if_merge_182;

  if_merge_182:
  r86 = cn_var_类型;
  r87 = r86 == 43;
  if (r87) goto if_then_183; else goto if_merge_184;

  if_then_183:
  return "字符串字面量";
  goto if_merge_184;

  if_merge_184:
  r88 = cn_var_类型;
  r89 = r88 == 44;
  if (r89) goto if_then_185; else goto if_merge_186;

  if_then_185:
  return "字符字面量";
  goto if_merge_186;

  if_merge_186:
  r90 = cn_var_类型;
  r91 = r90 == 45;
  if (r91) goto if_then_187; else goto if_merge_188;

  if_then_187:
  return "加号";
  goto if_merge_188;

  if_merge_188:
  r92 = cn_var_类型;
  r93 = r92 == 46;
  if (r93) goto if_then_189; else goto if_merge_190;

  if_then_189:
  return "减号";
  goto if_merge_190;

  if_merge_190:
  r94 = cn_var_类型;
  r95 = r94 == 47;
  if (r95) goto if_then_191; else goto if_merge_192;

  if_then_191:
  return "星号";
  goto if_merge_192;

  if_merge_192:
  r96 = cn_var_类型;
  r97 = r96 == 48;
  if (r97) goto if_then_193; else goto if_merge_194;

  if_then_193:
  return "斜杠";
  goto if_merge_194;

  if_merge_194:
  r98 = cn_var_类型;
  r99 = r98 == 49;
  if (r99) goto if_then_195; else goto if_merge_196;

  if_then_195:
  return "百分号";
  goto if_merge_196;

  if_merge_196:
  r100 = cn_var_类型;
  r101 = r100 == 50;
  if (r101) goto if_then_197; else goto if_merge_198;

  if_then_197:
  return "等于";
  goto if_merge_198;

  if_merge_198:
  r102 = cn_var_类型;
  r103 = r102 == 51;
  if (r103) goto if_then_199; else goto if_merge_200;

  if_then_199:
  return "赋值";
  goto if_merge_200;

  if_merge_200:
  r104 = cn_var_类型;
  r105 = r104 == 52;
  if (r105) goto if_then_201; else goto if_merge_202;

  if_then_201:
  return "不等于";
  goto if_merge_202;

  if_merge_202:
  r106 = cn_var_类型;
  r107 = r106 == 53;
  if (r107) goto if_then_203; else goto if_merge_204;

  if_then_203:
  return "小于";
  goto if_merge_204;

  if_merge_204:
  r108 = cn_var_类型;
  r109 = r108 == 54;
  if (r109) goto if_then_205; else goto if_merge_206;

  if_then_205:
  return "小于等于";
  goto if_merge_206;

  if_merge_206:
  r110 = cn_var_类型;
  r111 = r110 == 55;
  if (r111) goto if_then_207; else goto if_merge_208;

  if_then_207:
  return "大于";
  goto if_merge_208;

  if_merge_208:
  r112 = cn_var_类型;
  r113 = r112 == 56;
  if (r113) goto if_then_209; else goto if_merge_210;

  if_then_209:
  return "大于等于";
  goto if_merge_210;

  if_merge_210:
  r114 = cn_var_类型;
  r115 = r114 == 57;
  if (r115) goto if_then_211; else goto if_merge_212;

  if_then_211:
  return "逻辑与";
  goto if_merge_212;

  if_merge_212:
  r116 = cn_var_类型;
  r117 = r116 == 58;
  if (r117) goto if_then_213; else goto if_merge_214;

  if_then_213:
  return "逻辑或";
  goto if_merge_214;

  if_merge_214:
  r118 = cn_var_类型;
  r119 = r118 == 59;
  if (r119) goto if_then_215; else goto if_merge_216;

  if_then_215:
  return "逻辑非";
  goto if_merge_216;

  if_merge_216:
  r120 = cn_var_类型;
  r121 = r120 == 60;
  if (r121) goto if_then_217; else goto if_merge_218;

  if_then_217:
  return "按位与";
  goto if_merge_218;

  if_merge_218:
  r122 = cn_var_类型;
  r123 = r122 == 61;
  if (r123) goto if_then_219; else goto if_merge_220;

  if_then_219:
  return "按位或";
  goto if_merge_220;

  if_merge_220:
  r124 = cn_var_类型;
  r125 = r124 == 62;
  if (r125) goto if_then_221; else goto if_merge_222;

  if_then_221:
  return "按位异或";
  goto if_merge_222;

  if_merge_222:
  r126 = cn_var_类型;
  r127 = r126 == 63;
  if (r127) goto if_then_223; else goto if_merge_224;

  if_then_223:
  return "按位取反";
  goto if_merge_224;

  if_merge_224:
  r128 = cn_var_类型;
  r129 = r128 == 64;
  if (r129) goto if_then_225; else goto if_merge_226;

  if_then_225:
  return "左移";
  goto if_merge_226;

  if_merge_226:
  r130 = cn_var_类型;
  r131 = r130 == 65;
  if (r131) goto if_then_227; else goto if_merge_228;

  if_then_227:
  return "右移";
  goto if_merge_228;

  if_merge_228:
  r132 = cn_var_类型;
  r133 = r132 == 66;
  if (r133) goto if_then_229; else goto if_merge_230;

  if_then_229:
  return "自增";
  goto if_merge_230;

  if_merge_230:
  r134 = cn_var_类型;
  r135 = r134 == 67;
  if (r135) goto if_then_231; else goto if_merge_232;

  if_then_231:
  return "自减";
  goto if_merge_232;

  if_merge_232:
  r136 = cn_var_类型;
  r137 = r136 == 68;
  if (r137) goto if_then_233; else goto if_merge_234;

  if_then_233:
  return "箭头";
  goto if_merge_234;

  if_merge_234:
  r138 = cn_var_类型;
  r139 = r138 == 69;
  if (r139) goto if_then_235; else goto if_merge_236;

  if_then_235:
  return "左括号";
  goto if_merge_236;

  if_merge_236:
  r140 = cn_var_类型;
  r141 = r140 == 70;
  if (r141) goto if_then_237; else goto if_merge_238;

  if_then_237:
  return "右括号";
  goto if_merge_238;

  if_merge_238:
  r142 = cn_var_类型;
  r143 = r142 == 71;
  if (r143) goto if_then_239; else goto if_merge_240;

  if_then_239:
  return "左大括号";
  goto if_merge_240;

  if_merge_240:
  r144 = cn_var_类型;
  r145 = r144 == 72;
  if (r145) goto if_then_241; else goto if_merge_242;

  if_then_241:
  return "右大括号";
  goto if_merge_242;

  if_merge_242:
  r146 = cn_var_类型;
  r147 = r146 == 73;
  if (r147) goto if_then_243; else goto if_merge_244;

  if_then_243:
  return "左方括号";
  goto if_merge_244;

  if_merge_244:
  r148 = cn_var_类型;
  r149 = r148 == 74;
  if (r149) goto if_then_245; else goto if_merge_246;

  if_then_245:
  return "右方括号";
  goto if_merge_246;

  if_merge_246:
  r150 = cn_var_类型;
  r151 = r150 == 75;
  if (r151) goto if_then_247; else goto if_merge_248;

  if_then_247:
  return "分号";
  goto if_merge_248;

  if_merge_248:
  r152 = cn_var_类型;
  r153 = r152 == 76;
  if (r153) goto if_then_249; else goto if_merge_250;

  if_then_249:
  return "逗号";
  goto if_merge_250;

  if_merge_250:
  r154 = cn_var_类型;
  r155 = r154 == 77;
  if (r155) goto if_then_251; else goto if_merge_252;

  if_then_251:
  return "点";
  goto if_merge_252;

  if_merge_252:
  r156 = cn_var_类型;
  r157 = r156 == 78;
  if (r157) goto if_then_253; else goto if_merge_254;

  if_then_253:
  return "冒号";
  goto if_merge_254;

  if_merge_254:
  r158 = cn_var_类型;
  r159 = r158 == 79;
  if (r159) goto if_then_255; else goto if_merge_256;

  if_then_255:
  return "问号";
  goto if_merge_256;

  if_merge_256:
  r160 = cn_var_类型;
  r161 = r160 == 80;
  if (r161) goto if_then_257; else goto if_merge_258;

  if_then_257:
  return "结束";
  goto if_merge_258;

  if_merge_258:
  r162 = cn_var_类型;
  r163 = r162 == 81;
  if (r163) goto if_then_259; else goto if_merge_260;

  if_then_259:
  return "错误";
  goto if_merge_260;

  if_merge_260:
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
  if (r2) goto logic_rhs_263; else goto logic_merge_264;

  if_then_261:
  return 1;
  goto if_merge_262;

  if_merge_262:
  r6 = cn_var_类型;
  r7 = r6 >= 10;
  if (r7) goto logic_rhs_267; else goto logic_merge_268;

  logic_rhs_263:
  r3 = cn_var_类型;
  r4 = r3 <= 9;
  goto logic_merge_264;

  logic_merge_264:
  if (r4) goto if_then_261; else goto if_merge_262;

  if_then_265:
  return 1;
  goto if_merge_266;

  if_merge_266:
  r11 = cn_var_类型;
  r12 = r11 >= 17;
  if (r12) goto logic_rhs_271; else goto logic_merge_272;

  logic_rhs_267:
  r8 = cn_var_类型;
  r9 = r8 <= 16;
  goto logic_merge_268;

  logic_merge_268:
  if (r9) goto if_then_265; else goto if_merge_266;

  if_then_269:
  return 1;
  goto if_merge_270;

  if_merge_270:
  r16 = cn_var_类型;
  r17 = r16 >= 24;
  if (r17) goto logic_rhs_275; else goto logic_merge_276;

  logic_rhs_271:
  r13 = cn_var_类型;
  r14 = r13 <= 23;
  goto logic_merge_272;

  logic_merge_272:
  if (r14) goto if_then_269; else goto if_merge_270;

  if_then_273:
  return 1;
  goto if_merge_274;

  if_merge_274:
  r21 = cn_var_类型;
  r22 = r21 >= 27;
  if (r22) goto logic_rhs_279; else goto logic_merge_280;

  logic_rhs_275:
  r18 = cn_var_类型;
  r19 = r18 <= 26;
  goto logic_merge_276;

  logic_merge_276:
  if (r19) goto if_then_273; else goto if_merge_274;

  if_then_277:
  return 1;
  goto if_merge_278;

  if_merge_278:
  r26 = cn_var_类型;
  r27 = r26 >= 36;
  if (r27) goto logic_rhs_283; else goto logic_merge_284;

  logic_rhs_279:
  r23 = cn_var_类型;
  r24 = r23 <= 35;
  goto logic_merge_280;

  logic_merge_280:
  if (r24) goto if_then_277; else goto if_merge_278;

  if_then_281:
  return 1;
  goto if_merge_282;

  if_merge_282:
  return 0;

  logic_rhs_283:
  r28 = cn_var_类型;
  r29 = r28 <= 39;
  goto logic_merge_284;

  logic_merge_284:
  if (r29) goto if_then_281; else goto if_merge_282;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 40;
  if (r2) goto logic_rhs_285; else goto logic_merge_286;

  logic_rhs_285:
  r3 = cn_var_类型;
  r4 = r3 <= 44;
  goto logic_merge_286;

  logic_merge_286:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 45;
  if (r2) goto logic_rhs_287; else goto logic_merge_288;

  logic_rhs_287:
  r3 = cn_var_类型;
  r4 = r3 <= 68;
  goto logic_merge_288;

  logic_merge_288:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 69;
  if (r2) goto logic_rhs_289; else goto logic_merge_290;

  logic_rhs_289:
  r3 = cn_var_类型;
  r4 = r3 <= 79;
  goto logic_merge_290;

  logic_merge_290:
  return r4;
}

