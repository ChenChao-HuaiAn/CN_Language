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
  long long r0, r2, r4, r6;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r7;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 32;
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return 1;
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_c;
  r3 = r2 == 9;
  if (r3) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return 1;
  goto if_merge_3;

  if_merge_3:
  r4 = cn_var_c;
  r5 = r4 == 10;
  if (r5) goto if_then_4; else goto if_merge_5;

  if_then_4:
  return 1;
  goto if_merge_5;

  if_merge_5:
  r6 = cn_var_c;
  r7 = r6 == 13;
  if (r7) goto if_then_6; else goto if_merge_7;

  if_then_6:
  return 1;
  goto if_merge_7;

  if_merge_7:
  return 0;
}

_Bool 是换行符(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 10;
  if (r1) goto if_then_8; else goto if_merge_9;

  if_then_8:
  return 1;
  goto if_merge_9;

  if_merge_9:
  r2 = cn_var_c;
  r3 = r2 == 13;
  if (r3) goto if_then_10; else goto if_merge_11;

  if_then_10:
  return 1;
  goto if_merge_11;

  if_merge_11:
  return 0;
}

_Bool 是数字(long long cn_var_c) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_14; else goto logic_merge_15;

  if_then_12:
  return 1;
  goto if_merge_13;

  if_merge_13:
  return 0;

  logic_rhs_14:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_15;

  logic_merge_15:
  if (r4) goto if_then_12; else goto if_merge_13;
  return 0;
}

_Bool 是十六进制数字(long long cn_var_c) {
  long long r0, r1, r3, r5, r6, r8, r10, r11, r13;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r9;
  _Bool r12;
  _Bool r14;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_18; else goto logic_merge_19;

  if_then_16:
  return 1;
  goto if_merge_17;

  if_merge_17:
  r6 = cn_var_c;
  r7 = r6 >= 97;
  if (r7) goto logic_rhs_22; else goto logic_merge_23;

  logic_rhs_18:
  r3 = cn_var_c;
  r4 = r3 <= 57;
  goto logic_merge_19;

  logic_merge_19:
  if (r4) goto if_then_16; else goto if_merge_17;

  if_then_20:
  return 1;
  goto if_merge_21;

  if_merge_21:
  r11 = cn_var_c;
  r12 = r11 >= 65;
  if (r12) goto logic_rhs_26; else goto logic_merge_27;

  logic_rhs_22:
  r8 = cn_var_c;
  r9 = r8 <= 102;
  goto logic_merge_23;

  logic_merge_23:
  if (r9) goto if_then_20; else goto if_merge_21;

  if_then_24:
  return 1;
  goto if_merge_25;

  if_merge_25:
  return 0;

  logic_rhs_26:
  r13 = cn_var_c;
  r14 = r13 <= 70;
  goto logic_merge_27;

  logic_merge_27:
  if (r14) goto if_then_24; else goto if_merge_25;
  return 0;
}

_Bool 是二进制数字(long long cn_var_c) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 == 48;
  if (r2) goto logic_merge_31; else goto logic_rhs_30;

  if_then_28:
  return 1;
  goto if_merge_29;

  if_merge_29:
  return 0;

  logic_rhs_30:
  r3 = cn_var_c;
  r4 = r3 == 49;
  goto logic_merge_31;

  logic_merge_31:
  if (r4) goto if_then_28; else goto if_merge_29;
  return 0;
}

_Bool 是八进制数字(long long cn_var_c) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 48;
  if (r2) goto logic_rhs_34; else goto logic_merge_35;

  if_then_32:
  return 1;
  goto if_merge_33;

  if_merge_33:
  return 0;

  logic_rhs_34:
  r3 = cn_var_c;
  r4 = r3 <= 55;
  goto logic_merge_35;

  logic_merge_35:
  if (r4) goto if_then_32; else goto if_merge_33;
  return 0;
}

_Bool 是字母(long long cn_var_c) {
  long long r0, r1, r3, r5, r6, r8;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r9;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_38; else goto logic_merge_39;

  if_then_36:
  return 1;
  goto if_merge_37;

  if_merge_37:
  r6 = cn_var_c;
  r7 = r6 >= 65;
  if (r7) goto logic_rhs_42; else goto logic_merge_43;

  logic_rhs_38:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_39;

  logic_merge_39:
  if (r4) goto if_then_36; else goto if_merge_37;

  if_then_40:
  return 1;
  goto if_merge_41;

  if_merge_41:
  return 0;

  logic_rhs_42:
  r8 = cn_var_c;
  r9 = r8 <= 90;
  goto logic_merge_43;

  logic_merge_43:
  if (r9) goto if_then_40; else goto if_merge_41;
  return 0;
}

_Bool 是字母或数字(long long cn_var_c) {
  long long r0, r2;
  _Bool r1;
  _Bool r3;

  entry:
  r0 = cn_var_c;
  r1 = 是字母(r0);
  if (r1) goto if_then_44; else goto if_merge_45;

  if_then_44:
  return 1;
  goto if_merge_45;

  if_merge_45:
  r2 = cn_var_c;
  r3 = 是数字(r2);
  if (r3) goto if_then_46; else goto if_merge_47;

  if_then_46:
  return 1;
  goto if_merge_47;

  if_merge_47:
  return 0;
}

_Bool 是中文字符(long long cn_var_c) {
  long long r0, r1, r2, r4;
  _Bool r3;
  _Bool r5;

  entry:
  long long cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 228;
  if (r3) goto logic_rhs_50; else goto logic_merge_51;

  if_then_48:
  return 1;
  goto if_merge_49;

  if_merge_49:
  return 0;

  logic_rhs_50:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 233;
  goto logic_merge_51;

  logic_merge_51:
  if (r5) goto if_then_48; else goto if_merge_49;
  return 0;
}

_Bool 是标识符开头(long long cn_var_c) {
  long long r0, r2, r4, r5;
  _Bool r1;
  _Bool r3;
  _Bool r6;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_52; else goto if_merge_53;

  if_then_52:
  return 1;
  goto if_merge_53;

  if_merge_53:
  r2 = cn_var_c;
  r3 = 是字母(r2);
  if (r3) goto if_then_54; else goto if_merge_55;

  if_then_54:
  return 1;
  goto if_merge_55;

  if_merge_55:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_56; else goto if_merge_57;

  if_then_56:
  return 1;
  goto if_merge_57;

  if_merge_57:
  return 0;
}

_Bool 是标识符字符(long long cn_var_c) {
  long long r0, r2, r4, r5;
  _Bool r1;
  _Bool r3;
  _Bool r6;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 95;
  if (r1) goto if_then_58; else goto if_merge_59;

  if_then_58:
  return 1;
  goto if_merge_59;

  if_merge_59:
  r2 = cn_var_c;
  r3 = 是字母或数字(r2);
  if (r3) goto if_then_60; else goto if_merge_61;

  if_then_60:
  return 1;
  goto if_merge_61;

  if_merge_61:
  long long cn_var_整数值_0;
  r4 = cn_var_c;
  cn_var_整数值_0 = r4;
  r5 = cn_var_整数值_0;
  r6 = r5 >= 128;
  if (r6) goto if_then_62; else goto if_merge_63;

  if_then_62:
  return 1;
  goto if_merge_63;

  if_merge_63:
  return 0;
}

_Bool 是运算符字符(long long cn_var_c) {
  long long r0, r2, r4, r6, r8, r10, r12, r14, r16, r18, r20, r22, r24;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  _Bool r21;
  _Bool r23;
  _Bool r25;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 43;
  if (r1) goto if_then_64; else goto if_merge_65;

  if_then_64:
  return 1;
  goto if_merge_65;

  if_merge_65:
  r2 = cn_var_c;
  r3 = r2 == 45;
  if (r3) goto if_then_66; else goto if_merge_67;

  if_then_66:
  return 1;
  goto if_merge_67;

  if_merge_67:
  r4 = cn_var_c;
  r5 = r4 == 42;
  if (r5) goto if_then_68; else goto if_merge_69;

  if_then_68:
  return 1;
  goto if_merge_69;

  if_merge_69:
  r6 = cn_var_c;
  r7 = r6 == 47;
  if (r7) goto if_then_70; else goto if_merge_71;

  if_then_70:
  return 1;
  goto if_merge_71;

  if_merge_71:
  r8 = cn_var_c;
  r9 = r8 == 37;
  if (r9) goto if_then_72; else goto if_merge_73;

  if_then_72:
  return 1;
  goto if_merge_73;

  if_merge_73:
  r10 = cn_var_c;
  r11 = r10 == 61;
  if (r11) goto if_then_74; else goto if_merge_75;

  if_then_74:
  return 1;
  goto if_merge_75;

  if_merge_75:
  r12 = cn_var_c;
  r13 = r12 == 33;
  if (r13) goto if_then_76; else goto if_merge_77;

  if_then_76:
  return 1;
  goto if_merge_77;

  if_merge_77:
  r14 = cn_var_c;
  r15 = r14 == 60;
  if (r15) goto if_then_78; else goto if_merge_79;

  if_then_78:
  return 1;
  goto if_merge_79;

  if_merge_79:
  r16 = cn_var_c;
  r17 = r16 == 62;
  if (r17) goto if_then_80; else goto if_merge_81;

  if_then_80:
  return 1;
  goto if_merge_81;

  if_merge_81:
  r18 = cn_var_c;
  r19 = r18 == 38;
  if (r19) goto if_then_82; else goto if_merge_83;

  if_then_82:
  return 1;
  goto if_merge_83;

  if_merge_83:
  r20 = cn_var_c;
  r21 = r20 == 124;
  if (r21) goto if_then_84; else goto if_merge_85;

  if_then_84:
  return 1;
  goto if_merge_85;

  if_merge_85:
  r22 = cn_var_c;
  r23 = r22 == 94;
  if (r23) goto if_then_86; else goto if_merge_87;

  if_then_86:
  return 1;
  goto if_merge_87;

  if_merge_87:
  r24 = cn_var_c;
  r25 = r24 == 126;
  if (r25) goto if_then_88; else goto if_merge_89;

  if_then_88:
  return 1;
  goto if_merge_89;

  if_merge_89:
  return 0;
}

_Bool 是分隔符字符(long long cn_var_c) {
  long long r0, r2, r4, r6, r8, r10, r12, r14, r16, r18, r20;
  _Bool r1;
  _Bool r3;
  _Bool r5;
  _Bool r7;
  _Bool r9;
  _Bool r11;
  _Bool r13;
  _Bool r15;
  _Bool r17;
  _Bool r19;
  _Bool r21;

  entry:
  r0 = cn_var_c;
  r1 = r0 == 40;
  if (r1) goto if_then_90; else goto if_merge_91;

  if_then_90:
  return 1;
  goto if_merge_91;

  if_merge_91:
  r2 = cn_var_c;
  r3 = r2 == 41;
  if (r3) goto if_then_92; else goto if_merge_93;

  if_then_92:
  return 1;
  goto if_merge_93;

  if_merge_93:
  r4 = cn_var_c;
  r5 = r4 == 123;
  if (r5) goto if_then_94; else goto if_merge_95;

  if_then_94:
  return 1;
  goto if_merge_95;

  if_merge_95:
  r6 = cn_var_c;
  r7 = r6 == 125;
  if (r7) goto if_then_96; else goto if_merge_97;

  if_then_96:
  return 1;
  goto if_merge_97;

  if_merge_97:
  r8 = cn_var_c;
  r9 = r8 == 91;
  if (r9) goto if_then_98; else goto if_merge_99;

  if_then_98:
  return 1;
  goto if_merge_99;

  if_merge_99:
  r10 = cn_var_c;
  r11 = r10 == 93;
  if (r11) goto if_then_100; else goto if_merge_101;

  if_then_100:
  return 1;
  goto if_merge_101;

  if_merge_101:
  r12 = cn_var_c;
  r13 = r12 == 59;
  if (r13) goto if_then_102; else goto if_merge_103;

  if_then_102:
  return 1;
  goto if_merge_103;

  if_merge_103:
  r14 = cn_var_c;
  r15 = r14 == 44;
  if (r15) goto if_then_104; else goto if_merge_105;

  if_then_104:
  return 1;
  goto if_merge_105;

  if_merge_105:
  r16 = cn_var_c;
  r17 = r16 == 46;
  if (r17) goto if_then_106; else goto if_merge_107;

  if_then_106:
  return 1;
  goto if_merge_107;

  if_merge_107:
  r18 = cn_var_c;
  r19 = r18 == 58;
  if (r19) goto if_then_108; else goto if_merge_109;

  if_then_108:
  return 1;
  goto if_merge_109;

  if_merge_109:
  r20 = cn_var_c;
  r21 = r20 == 63;
  if (r21) goto if_then_110; else goto if_merge_111;

  if_then_110:
  return 1;
  goto if_merge_111;

  if_merge_111:
  return 0;
}

long long 转小写(long long cn_var_c) {
  long long r0, r1, r3, r5, r6, r7, r8;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 65;
  if (r2) goto logic_rhs_114; else goto logic_merge_115;

  if_then_112:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 + r6;
  return r7;
  goto if_merge_113;

  if_merge_113:
  r8 = cn_var_c;
  return r8;

  logic_rhs_114:
  r3 = cn_var_c;
  r4 = r3 <= 90;
  goto logic_merge_115;

  logic_merge_115:
  if (r4) goto if_then_112; else goto if_merge_113;
  return 0;
}

long long 转大写(long long cn_var_c) {
  long long r0, r1, r3, r5, r6, r7, r8;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c;
  r2 = r1 >= 97;
  if (r2) goto logic_rhs_118; else goto logic_merge_119;

  if_then_116:
  r5 = cn_var_c;
  r6 = 32;
  r7 = r5 - r6;
  return r7;
  goto if_merge_117;

  if_merge_117:
  r8 = cn_var_c;
  return r8;

  logic_rhs_118:
  r3 = cn_var_c;
  r4 = r3 <= 122;
  goto logic_merge_119;

  logic_merge_119:
  if (r4) goto if_then_116; else goto if_merge_117;
  return 0;
}

_Bool 是可打印字符(long long cn_var_c) {
  long long r0, r1, r2, r4;
  _Bool r3;
  _Bool r5;

  entry:
  long long cn_var_整数值_0;
  r0 = cn_var_c;
  cn_var_整数值_0 = r0;
  r2 = cn_var_整数值_0;
  r3 = r2 >= 32;
  if (r3) goto logic_rhs_122; else goto logic_merge_123;

  if_then_120:
  return 1;
  goto if_merge_121;

  if_merge_121:
  return 0;

  logic_rhs_122:
  r4 = cn_var_整数值_0;
  r5 = r4 <= 126;
  goto logic_merge_123;

  logic_merge_123:
  if (r5) goto if_then_120; else goto if_merge_121;
  return 0;
}

_Bool 是十六进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3, r4, r6;
  _Bool r2;
  _Bool r5;
  _Bool r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_126; else goto logic_merge_127;

  if_then_124:
  return 1;
  goto if_merge_125;

  if_merge_125:
  return 0;

  logic_rhs_126:
  r4 = cn_var_c2;
  r5 = r4 == 120;
  if (r5) goto logic_merge_129; else goto logic_rhs_128;

  logic_merge_127:
  if (r7) goto if_then_124; else goto if_merge_125;

  logic_rhs_128:
  r6 = cn_var_c2;
  r7 = r6 == 88;
  goto logic_merge_129;

  logic_merge_129:
  goto logic_merge_127;
  return 0;
}

_Bool 是二进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3, r4, r6;
  _Bool r2;
  _Bool r5;
  _Bool r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_132; else goto logic_merge_133;

  if_then_130:
  return 1;
  goto if_merge_131;

  if_merge_131:
  return 0;

  logic_rhs_132:
  r4 = cn_var_c2;
  r5 = r4 == 98;
  if (r5) goto logic_merge_135; else goto logic_rhs_134;

  logic_merge_133:
  if (r7) goto if_then_130; else goto if_merge_131;

  logic_rhs_134:
  r6 = cn_var_c2;
  r7 = r6 == 66;
  goto logic_merge_135;

  logic_merge_135:
  goto logic_merge_133;
  return 0;
}

_Bool 是八进制前缀(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3, r4, r6;
  _Bool r2;
  _Bool r5;
  _Bool r7;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 48;
  if (r2) goto logic_rhs_138; else goto logic_merge_139;

  if_then_136:
  return 1;
  goto if_merge_137;

  if_merge_137:
  return 0;

  logic_rhs_138:
  r4 = cn_var_c2;
  r5 = r4 == 111;
  if (r5) goto logic_merge_141; else goto logic_rhs_140;

  logic_merge_139:
  if (r7) goto if_then_136; else goto if_merge_137;

  logic_rhs_140:
  r6 = cn_var_c2;
  r7 = r6 == 79;
  goto logic_merge_141;

  logic_merge_141:
  goto logic_merge_139;
  return 0;
}

_Bool 是单行注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_144; else goto logic_merge_145;

  if_then_142:
  return 1;
  goto if_merge_143;

  if_merge_143:
  return 0;

  logic_rhs_144:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_145;

  logic_merge_145:
  if (r4) goto if_then_142; else goto if_merge_143;
  return 0;
}

_Bool 是块注释开始(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 47;
  if (r2) goto logic_rhs_148; else goto logic_merge_149;

  if_then_146:
  return 1;
  goto if_merge_147;

  if_merge_147:
  return 0;

  logic_rhs_148:
  r3 = cn_var_c2;
  r4 = r3 == 42;
  goto logic_merge_149;

  logic_merge_149:
  if (r4) goto if_then_146; else goto if_merge_147;
  return 0;
}

_Bool 是块注释结束(long long cn_var_c1, long long cn_var_c2) {
  long long r0, r1, r3;
  _Bool r2;
  _Bool r4;

  entry:
  r1 = cn_var_c1;
  r2 = r1 == 42;
  if (r2) goto logic_rhs_152; else goto logic_merge_153;

  if_then_150:
  return 1;
  goto if_merge_151;

  if_merge_151:
  return 0;

  logic_rhs_152:
  r3 = cn_var_c2;
  r4 = r3 == 47;
  goto logic_merge_153;

  logic_merge_153:
  if (r4) goto if_then_150; else goto if_merge_151;
  return 0;
}

