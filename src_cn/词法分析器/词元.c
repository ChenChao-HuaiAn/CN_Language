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
  if (r1) goto if_then_90; else goto if_merge_91;

  if_then_90:
  return "关键字_如果";
  goto if_merge_91;

  if_merge_91:
  r2 = cn_var_类型;
  r3 = r2 == 0;
  if (r3) goto if_then_92; else goto if_merge_93;

  if_then_92:
  return "关键字_否则";
  goto if_merge_93;

  if_merge_93:
  r4 = cn_var_类型;
  r5 = r4 == 0;
  if (r5) goto if_then_94; else goto if_merge_95;

  if_then_94:
  return "关键字_当";
  goto if_merge_95;

  if_merge_95:
  r6 = cn_var_类型;
  r7 = r6 == 0;
  if (r7) goto if_then_96; else goto if_merge_97;

  if_then_96:
  return "关键字_循环";
  goto if_merge_97;

  if_merge_97:
  r8 = cn_var_类型;
  r9 = r8 == 0;
  if (r9) goto if_then_98; else goto if_merge_99;

  if_then_98:
  return "关键字_返回";
  goto if_merge_99;

  if_merge_99:
  r10 = cn_var_类型;
  r11 = r10 == 0;
  if (r11) goto if_then_100; else goto if_merge_101;

  if_then_100:
  return "关键字_中断";
  goto if_merge_101;

  if_merge_101:
  r12 = cn_var_类型;
  r13 = r12 == 0;
  if (r13) goto if_then_102; else goto if_merge_103;

  if_then_102:
  return "关键字_继续";
  goto if_merge_103;

  if_merge_103:
  r14 = cn_var_类型;
  r15 = r14 == 0;
  if (r15) goto if_then_104; else goto if_merge_105;

  if_then_104:
  return "关键字_选择";
  goto if_merge_105;

  if_merge_105:
  r16 = cn_var_类型;
  r17 = r16 == 0;
  if (r17) goto if_then_106; else goto if_merge_107;

  if_then_106:
  return "关键字_情况";
  goto if_merge_107;

  if_merge_107:
  r18 = cn_var_类型;
  r19 = r18 == 0;
  if (r19) goto if_then_108; else goto if_merge_109;

  if_then_108:
  return "关键字_默认";
  goto if_merge_109;

  if_merge_109:
  r20 = cn_var_类型;
  r21 = r20 == 0;
  if (r21) goto if_then_110; else goto if_merge_111;

  if_then_110:
  return "关键字_整数";
  goto if_merge_111;

  if_merge_111:
  r22 = cn_var_类型;
  r23 = r22 == 0;
  if (r23) goto if_then_112; else goto if_merge_113;

  if_then_112:
  return "关键字_小数";
  goto if_merge_113;

  if_merge_113:
  r24 = cn_var_类型;
  r25 = r24 == 0;
  if (r25) goto if_then_114; else goto if_merge_115;

  if_then_114:
  return "关键字_字符串";
  goto if_merge_115;

  if_merge_115:
  r26 = cn_var_类型;
  r27 = r26 == 0;
  if (r27) goto if_then_116; else goto if_merge_117;

  if_then_116:
  return "关键字_布尔";
  goto if_merge_117;

  if_merge_117:
  r28 = cn_var_类型;
  r29 = r28 == 0;
  if (r29) goto if_then_118; else goto if_merge_119;

  if_then_118:
  return "关键字_空类型";
  goto if_merge_119;

  if_merge_119:
  r30 = cn_var_类型;
  r31 = r30 == 0;
  if (r31) goto if_then_120; else goto if_merge_121;

  if_then_120:
  return "关键字_结构体";
  goto if_merge_121;

  if_merge_121:
  r32 = cn_var_类型;
  r33 = r32 == 0;
  if (r33) goto if_then_122; else goto if_merge_123;

  if_then_122:
  return "关键字_枚举";
  goto if_merge_123;

  if_merge_123:
  r34 = cn_var_类型;
  r35 = r34 == 0;
  if (r35) goto if_then_124; else goto if_merge_125;

  if_then_124:
  return "关键字_函数";
  goto if_merge_125;

  if_merge_125:
  r36 = cn_var_类型;
  r37 = r36 == 0;
  if (r37) goto if_then_126; else goto if_merge_127;

  if_then_126:
  return "关键字_变量";
  goto if_merge_127;

  if_merge_127:
  r38 = cn_var_类型;
  r39 = r38 == 0;
  if (r39) goto if_then_128; else goto if_merge_129;

  if_then_128:
  return "关键字_导入";
  goto if_merge_129;

  if_merge_129:
  r40 = cn_var_类型;
  r41 = r40 == 0;
  if (r41) goto if_then_130; else goto if_merge_131;

  if_then_130:
  return "关键字_从";
  goto if_merge_131;

  if_merge_131:
  r42 = cn_var_类型;
  r43 = r42 == 0;
  if (r43) goto if_then_132; else goto if_merge_133;

  if_then_132:
  return "关键字_公开";
  goto if_merge_133;

  if_merge_133:
  r44 = cn_var_类型;
  r45 = r44 == 0;
  if (r45) goto if_then_134; else goto if_merge_135;

  if_then_134:
  return "关键字_私有";
  goto if_merge_135;

  if_merge_135:
  r46 = cn_var_类型;
  r47 = r46 == 0;
  if (r47) goto if_then_136; else goto if_merge_137;

  if_then_136:
  return "关键字_静态";
  goto if_merge_137;

  if_merge_137:
  r48 = cn_var_类型;
  r49 = r48 == 0;
  if (r49) goto if_then_138; else goto if_merge_139;

  if_then_138:
  return "关键字_真";
  goto if_merge_139;

  if_merge_139:
  r50 = cn_var_类型;
  r51 = r50 == 0;
  if (r51) goto if_then_140; else goto if_merge_141;

  if_then_140:
  return "关键字_假";
  goto if_merge_141;

  if_merge_141:
  r52 = cn_var_类型;
  r53 = r52 == 0;
  if (r53) goto if_then_142; else goto if_merge_143;

  if_then_142:
  return "关键字_无";
  goto if_merge_143;

  if_merge_143:
  r54 = cn_var_类型;
  r55 = r54 == 0;
  if (r55) goto if_then_144; else goto if_merge_145;

  if_then_144:
  return "关键字_类";
  goto if_merge_145;

  if_merge_145:
  r56 = cn_var_类型;
  r57 = r56 == 0;
  if (r57) goto if_then_146; else goto if_merge_147;

  if_then_146:
  return "关键字_接口";
  goto if_merge_147;

  if_merge_147:
  r58 = cn_var_类型;
  r59 = r58 == 0;
  if (r59) goto if_then_148; else goto if_merge_149;

  if_then_148:
  return "关键字_保护";
  goto if_merge_149;

  if_merge_149:
  r60 = cn_var_类型;
  r61 = r60 == 0;
  if (r61) goto if_then_150; else goto if_merge_151;

  if_then_150:
  return "关键字_虚拟";
  goto if_merge_151;

  if_merge_151:
  r62 = cn_var_类型;
  r63 = r62 == 0;
  if (r63) goto if_then_152; else goto if_merge_153;

  if_then_152:
  return "关键字_重写";
  goto if_merge_153;

  if_merge_153:
  r64 = cn_var_类型;
  r65 = r64 == 0;
  if (r65) goto if_then_154; else goto if_merge_155;

  if_then_154:
  return "关键字_抽象";
  goto if_merge_155;

  if_merge_155:
  r66 = cn_var_类型;
  r67 = r66 == 0;
  if (r67) goto if_then_156; else goto if_merge_157;

  if_then_156:
  return "关键字_实现";
  goto if_merge_157;

  if_merge_157:
  r68 = cn_var_类型;
  r69 = r68 == 0;
  if (r69) goto if_then_158; else goto if_merge_159;

  if_then_158:
  return "关键字_自身";
  goto if_merge_159;

  if_merge_159:
  r70 = cn_var_类型;
  r71 = r70 == 0;
  if (r71) goto if_then_160; else goto if_merge_161;

  if_then_160:
  return "关键字_基类";
  goto if_merge_161;

  if_merge_161:
  r72 = cn_var_类型;
  r73 = r72 == 0;
  if (r73) goto if_then_162; else goto if_merge_163;

  if_then_162:
  return "关键字_尝试";
  goto if_merge_163;

  if_merge_163:
  r74 = cn_var_类型;
  r75 = r74 == 0;
  if (r75) goto if_then_164; else goto if_merge_165;

  if_then_164:
  return "关键字_捕获";
  goto if_merge_165;

  if_merge_165:
  r76 = cn_var_类型;
  r77 = r76 == 0;
  if (r77) goto if_then_166; else goto if_merge_167;

  if_then_166:
  return "关键字_抛出";
  goto if_merge_167;

  if_merge_167:
  r78 = cn_var_类型;
  r79 = r78 == 0;
  if (r79) goto if_then_168; else goto if_merge_169;

  if_then_168:
  return "关键字_最终";
  goto if_merge_169;

  if_merge_169:
  r80 = cn_var_类型;
  r81 = r80 == 0;
  if (r81) goto if_then_170; else goto if_merge_171;

  if_then_170:
  return "标识符";
  goto if_merge_171;

  if_merge_171:
  r82 = cn_var_类型;
  r83 = r82 == 0;
  if (r83) goto if_then_172; else goto if_merge_173;

  if_then_172:
  return "整数字面量";
  goto if_merge_173;

  if_merge_173:
  r84 = cn_var_类型;
  r85 = r84 == 0;
  if (r85) goto if_then_174; else goto if_merge_175;

  if_then_174:
  return "浮点字面量";
  goto if_merge_175;

  if_merge_175:
  r86 = cn_var_类型;
  r87 = r86 == 0;
  if (r87) goto if_then_176; else goto if_merge_177;

  if_then_176:
  return "字符串字面量";
  goto if_merge_177;

  if_merge_177:
  r88 = cn_var_类型;
  r89 = r88 == 0;
  if (r89) goto if_then_178; else goto if_merge_179;

  if_then_178:
  return "字符字面量";
  goto if_merge_179;

  if_merge_179:
  r90 = cn_var_类型;
  r91 = r90 == 0;
  if (r91) goto if_then_180; else goto if_merge_181;

  if_then_180:
  return "加号";
  goto if_merge_181;

  if_merge_181:
  r92 = cn_var_类型;
  r93 = r92 == 0;
  if (r93) goto if_then_182; else goto if_merge_183;

  if_then_182:
  return "减号";
  goto if_merge_183;

  if_merge_183:
  r94 = cn_var_类型;
  r95 = r94 == 0;
  if (r95) goto if_then_184; else goto if_merge_185;

  if_then_184:
  return "星号";
  goto if_merge_185;

  if_merge_185:
  r96 = cn_var_类型;
  r97 = r96 == 0;
  if (r97) goto if_then_186; else goto if_merge_187;

  if_then_186:
  return "斜杠";
  goto if_merge_187;

  if_merge_187:
  r98 = cn_var_类型;
  r99 = r98 == 0;
  if (r99) goto if_then_188; else goto if_merge_189;

  if_then_188:
  return "百分号";
  goto if_merge_189;

  if_merge_189:
  r100 = cn_var_类型;
  r101 = r100 == 0;
  if (r101) goto if_then_190; else goto if_merge_191;

  if_then_190:
  return "等于";
  goto if_merge_191;

  if_merge_191:
  r102 = cn_var_类型;
  r103 = r102 == 0;
  if (r103) goto if_then_192; else goto if_merge_193;

  if_then_192:
  return "赋值";
  goto if_merge_193;

  if_merge_193:
  r104 = cn_var_类型;
  r105 = r104 == 0;
  if (r105) goto if_then_194; else goto if_merge_195;

  if_then_194:
  return "不等于";
  goto if_merge_195;

  if_merge_195:
  r106 = cn_var_类型;
  r107 = r106 == 0;
  if (r107) goto if_then_196; else goto if_merge_197;

  if_then_196:
  return "小于";
  goto if_merge_197;

  if_merge_197:
  r108 = cn_var_类型;
  r109 = r108 == 0;
  if (r109) goto if_then_198; else goto if_merge_199;

  if_then_198:
  return "小于等于";
  goto if_merge_199;

  if_merge_199:
  r110 = cn_var_类型;
  r111 = r110 == 0;
  if (r111) goto if_then_200; else goto if_merge_201;

  if_then_200:
  return "大于";
  goto if_merge_201;

  if_merge_201:
  r112 = cn_var_类型;
  r113 = r112 == 0;
  if (r113) goto if_then_202; else goto if_merge_203;

  if_then_202:
  return "大于等于";
  goto if_merge_203;

  if_merge_203:
  r114 = cn_var_类型;
  r115 = r114 == 0;
  if (r115) goto if_then_204; else goto if_merge_205;

  if_then_204:
  return "逻辑与";
  goto if_merge_205;

  if_merge_205:
  r116 = cn_var_类型;
  r117 = r116 == 0;
  if (r117) goto if_then_206; else goto if_merge_207;

  if_then_206:
  return "逻辑或";
  goto if_merge_207;

  if_merge_207:
  r118 = cn_var_类型;
  r119 = r118 == 0;
  if (r119) goto if_then_208; else goto if_merge_209;

  if_then_208:
  return "逻辑非";
  goto if_merge_209;

  if_merge_209:
  r120 = cn_var_类型;
  r121 = r120 == 0;
  if (r121) goto if_then_210; else goto if_merge_211;

  if_then_210:
  return "按位与";
  goto if_merge_211;

  if_merge_211:
  r122 = cn_var_类型;
  r123 = r122 == 0;
  if (r123) goto if_then_212; else goto if_merge_213;

  if_then_212:
  return "按位或";
  goto if_merge_213;

  if_merge_213:
  r124 = cn_var_类型;
  r125 = r124 == 0;
  if (r125) goto if_then_214; else goto if_merge_215;

  if_then_214:
  return "按位异或";
  goto if_merge_215;

  if_merge_215:
  r126 = cn_var_类型;
  r127 = r126 == 0;
  if (r127) goto if_then_216; else goto if_merge_217;

  if_then_216:
  return "按位取反";
  goto if_merge_217;

  if_merge_217:
  r128 = cn_var_类型;
  r129 = r128 == 0;
  if (r129) goto if_then_218; else goto if_merge_219;

  if_then_218:
  return "左移";
  goto if_merge_219;

  if_merge_219:
  r130 = cn_var_类型;
  r131 = r130 == 0;
  if (r131) goto if_then_220; else goto if_merge_221;

  if_then_220:
  return "右移";
  goto if_merge_221;

  if_merge_221:
  r132 = cn_var_类型;
  r133 = r132 == 0;
  if (r133) goto if_then_222; else goto if_merge_223;

  if_then_222:
  return "自增";
  goto if_merge_223;

  if_merge_223:
  r134 = cn_var_类型;
  r135 = r134 == 0;
  if (r135) goto if_then_224; else goto if_merge_225;

  if_then_224:
  return "自减";
  goto if_merge_225;

  if_merge_225:
  r136 = cn_var_类型;
  r137 = r136 == 0;
  if (r137) goto if_then_226; else goto if_merge_227;

  if_then_226:
  return "箭头";
  goto if_merge_227;

  if_merge_227:
  r138 = cn_var_类型;
  r139 = r138 == 0;
  if (r139) goto if_then_228; else goto if_merge_229;

  if_then_228:
  return "左括号";
  goto if_merge_229;

  if_merge_229:
  r140 = cn_var_类型;
  r141 = r140 == 0;
  if (r141) goto if_then_230; else goto if_merge_231;

  if_then_230:
  return "右括号";
  goto if_merge_231;

  if_merge_231:
  r142 = cn_var_类型;
  r143 = r142 == 0;
  if (r143) goto if_then_232; else goto if_merge_233;

  if_then_232:
  return "左大括号";
  goto if_merge_233;

  if_merge_233:
  r144 = cn_var_类型;
  r145 = r144 == 0;
  if (r145) goto if_then_234; else goto if_merge_235;

  if_then_234:
  return "右大括号";
  goto if_merge_235;

  if_merge_235:
  r146 = cn_var_类型;
  r147 = r146 == 0;
  if (r147) goto if_then_236; else goto if_merge_237;

  if_then_236:
  return "左方括号";
  goto if_merge_237;

  if_merge_237:
  r148 = cn_var_类型;
  r149 = r148 == 0;
  if (r149) goto if_then_238; else goto if_merge_239;

  if_then_238:
  return "右方括号";
  goto if_merge_239;

  if_merge_239:
  r150 = cn_var_类型;
  r151 = r150 == 0;
  if (r151) goto if_then_240; else goto if_merge_241;

  if_then_240:
  return "分号";
  goto if_merge_241;

  if_merge_241:
  r152 = cn_var_类型;
  r153 = r152 == 0;
  if (r153) goto if_then_242; else goto if_merge_243;

  if_then_242:
  return "逗号";
  goto if_merge_243;

  if_merge_243:
  r154 = cn_var_类型;
  r155 = r154 == 0;
  if (r155) goto if_then_244; else goto if_merge_245;

  if_then_244:
  return "点";
  goto if_merge_245;

  if_merge_245:
  r156 = cn_var_类型;
  r157 = r156 == 0;
  if (r157) goto if_then_246; else goto if_merge_247;

  if_then_246:
  return "冒号";
  goto if_merge_247;

  if_merge_247:
  r158 = cn_var_类型;
  r159 = r158 == 0;
  if (r159) goto if_then_248; else goto if_merge_249;

  if_then_248:
  return "问号";
  goto if_merge_249;

  if_merge_249:
  r160 = cn_var_类型;
  r161 = r160 == 0;
  if (r161) goto if_then_250; else goto if_merge_251;

  if_then_250:
  return "结束";
  goto if_merge_251;

  if_merge_251:
  r162 = cn_var_类型;
  r163 = r162 == 0;
  if (r163) goto if_then_252; else goto if_merge_253;

  if_then_252:
  return "错误";
  goto if_merge_253;

  if_merge_253:
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
  if (r2) goto logic_rhs_256; else goto logic_merge_257;

  if_then_254:
  return 1;
  goto if_merge_255;

  if_merge_255:
  r6 = cn_var_类型;
  r7 = r6 >= 0;
  if (r7) goto logic_rhs_260; else goto logic_merge_261;

  logic_rhs_256:
  r3 = cn_var_类型;
  r4 = r3 <= 0;
  goto logic_merge_257;

  logic_merge_257:
  if (r4) goto if_then_254; else goto if_merge_255;

  if_then_258:
  return 1;
  goto if_merge_259;

  if_merge_259:
  r11 = cn_var_类型;
  r12 = r11 >= 0;
  if (r12) goto logic_rhs_264; else goto logic_merge_265;

  logic_rhs_260:
  r8 = cn_var_类型;
  r9 = r8 <= 0;
  goto logic_merge_261;

  logic_merge_261:
  if (r9) goto if_then_258; else goto if_merge_259;

  if_then_262:
  return 1;
  goto if_merge_263;

  if_merge_263:
  r16 = cn_var_类型;
  r17 = r16 >= 0;
  if (r17) goto logic_rhs_268; else goto logic_merge_269;

  logic_rhs_264:
  r13 = cn_var_类型;
  r14 = r13 <= 0;
  goto logic_merge_265;

  logic_merge_265:
  if (r14) goto if_then_262; else goto if_merge_263;

  if_then_266:
  return 1;
  goto if_merge_267;

  if_merge_267:
  r21 = cn_var_类型;
  r22 = r21 >= 0;
  if (r22) goto logic_rhs_272; else goto logic_merge_273;

  logic_rhs_268:
  r18 = cn_var_类型;
  r19 = r18 <= 0;
  goto logic_merge_269;

  logic_merge_269:
  if (r19) goto if_then_266; else goto if_merge_267;

  if_then_270:
  return 1;
  goto if_merge_271;

  if_merge_271:
  r26 = cn_var_类型;
  r27 = r26 >= 0;
  if (r27) goto logic_rhs_276; else goto logic_merge_277;

  logic_rhs_272:
  r23 = cn_var_类型;
  r24 = r23 <= 0;
  goto logic_merge_273;

  logic_merge_273:
  if (r24) goto if_then_270; else goto if_merge_271;

  if_then_274:
  return 1;
  goto if_merge_275;

  if_merge_275:
  return 0;

  logic_rhs_276:
  r28 = cn_var_类型;
  r29 = r28 <= 0;
  goto logic_merge_277;

  logic_merge_277:
  if (r29) goto if_then_274; else goto if_merge_275;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 0;
  if (r2) goto logic_rhs_278; else goto logic_merge_279;

  logic_rhs_278:
  r3 = cn_var_类型;
  r4 = r3 <= 0;
  goto logic_merge_279;

  logic_merge_279:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 0;
  if (r2) goto logic_rhs_280; else goto logic_merge_281;

  logic_rhs_280:
  r3 = cn_var_类型;
  r4 = r3 <= 0;
  goto logic_merge_281;

  logic_merge_281:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0, r2, r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 0;
  if (r2) goto logic_rhs_282; else goto logic_merge_283;

  logic_rhs_282:
  r3 = cn_var_类型;
  r4 = r3 <= 0;
  goto logic_merge_283;

  logic_merge_283:
  return r4;
}

