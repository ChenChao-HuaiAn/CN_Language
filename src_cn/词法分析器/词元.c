#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cnrt.h"
#include "cnlang/runtime/system_api.h"

// CN Language Enum Definitions
enum 词元类型枚举 {
    关键字_如果,
    关键字_否则,
    关键字_当,
    关键字_循环,
    关键字_返回,
    关键字_中断,
    关键字_继续,
    关键字_选择,
    关键字_情况,
    关键字_默认,
    关键字_整数,
    关键字_小数,
    关键字_字符串,
    关键字_布尔,
    关键字_空类型,
    关键字_结构体,
    关键字_枚举,
    关键字_函数,
    关键字_变量,
    关键字_导入,
    关键字_从,
    关键字_公开,
    关键字_私有,
    关键字_静态,
    关键字_真,
    关键字_假,
    关键字_无,
    关键字_类,
    关键字_接口,
    关键字_保护,
    关键字_虚拟,
    关键字_重写,
    关键字_抽象,
    关键字_实现,
    关键字_自身,
    关键字_基类,
    关键字_尝试,
    关键字_捕获,
    关键字_抛出,
    关键字_最终,
    标识符,
    整数字面量,
    浮点字面量,
    字符串字面量,
    字符字面量,
    加号,
    减号,
    星号,
    斜杠,
    百分号,
    等于,
    赋值,
    不等于,
    小于,
    小于等于,
    大于,
    大于等于,
    逻辑与,
    逻辑或,
    逻辑非,
    按位与,
    按位或,
    按位异或,
    按位取反,
    左移,
    右移,
    自增,
    自减,
    箭头,
    左括号,
    右括号,
    左大括号,
    右大括号,
    左方括号,
    右方括号,
    分号,
    逗号,
    点,
    冒号,
    问号,
    结束,
    错误
};

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
  _Bool r27;
  _Bool r29;
  _Bool r31;
  _Bool r33;
  _Bool r35;
  _Bool r37;
  _Bool r39;
  _Bool r41;
  _Bool r43;
  _Bool r45;
  _Bool r47;
  _Bool r49;
  _Bool r51;
  _Bool r53;
  _Bool r55;
  _Bool r57;
  _Bool r59;
  _Bool r61;
  _Bool r63;
  _Bool r65;
  _Bool r67;
  _Bool r69;
  _Bool r71;
  _Bool r73;
  _Bool r75;
  _Bool r77;
  _Bool r79;
  _Bool r81;
  _Bool r83;
  _Bool r85;
  _Bool r87;
  _Bool r89;
  _Bool r91;
  _Bool r93;
  _Bool r95;
  _Bool r97;
  _Bool r99;
  _Bool r101;
  _Bool r103;
  _Bool r105;
  _Bool r107;
  _Bool r109;
  _Bool r111;
  _Bool r113;
  _Bool r115;
  _Bool r117;
  _Bool r119;
  _Bool r121;
  _Bool r123;
  _Bool r125;
  _Bool r127;
  _Bool r129;
  _Bool r131;
  _Bool r133;
  _Bool r135;
  _Bool r137;
  _Bool r139;
  _Bool r141;
  _Bool r143;
  _Bool r145;
  _Bool r147;
  _Bool r149;
  _Bool r151;
  _Bool r153;
  _Bool r155;
  _Bool r157;
  _Bool r159;
  _Bool r161;
  _Bool r163;
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
  if (r1) goto if_then_0; else goto if_merge_1;

  if_then_0:
  return "关键字_如果";
  goto if_merge_1;

  if_merge_1:
  r2 = cn_var_类型;
  r3 = r2 == 1;
  if (r3) goto if_then_2; else goto if_merge_3;

  if_then_2:
  return "关键字_否则";
  goto if_merge_3;

  if_merge_3:
  r4 = cn_var_类型;
  r5 = r4 == 2;
  if (r5) goto if_then_4; else goto if_merge_5;

  if_then_4:
  return "关键字_当";
  goto if_merge_5;

  if_merge_5:
  r6 = cn_var_类型;
  r7 = r6 == 3;
  if (r7) goto if_then_6; else goto if_merge_7;

  if_then_6:
  return "关键字_循环";
  goto if_merge_7;

  if_merge_7:
  r8 = cn_var_类型;
  r9 = r8 == 4;
  if (r9) goto if_then_8; else goto if_merge_9;

  if_then_8:
  return "关键字_返回";
  goto if_merge_9;

  if_merge_9:
  r10 = cn_var_类型;
  r11 = r10 == 5;
  if (r11) goto if_then_10; else goto if_merge_11;

  if_then_10:
  return "关键字_中断";
  goto if_merge_11;

  if_merge_11:
  r12 = cn_var_类型;
  r13 = r12 == 6;
  if (r13) goto if_then_12; else goto if_merge_13;

  if_then_12:
  return "关键字_继续";
  goto if_merge_13;

  if_merge_13:
  r14 = cn_var_类型;
  r15 = r14 == 7;
  if (r15) goto if_then_14; else goto if_merge_15;

  if_then_14:
  return "关键字_选择";
  goto if_merge_15;

  if_merge_15:
  r16 = cn_var_类型;
  r17 = r16 == 8;
  if (r17) goto if_then_16; else goto if_merge_17;

  if_then_16:
  return "关键字_情况";
  goto if_merge_17;

  if_merge_17:
  r18 = cn_var_类型;
  r19 = r18 == 9;
  if (r19) goto if_then_18; else goto if_merge_19;

  if_then_18:
  return "关键字_默认";
  goto if_merge_19;

  if_merge_19:
  r20 = cn_var_类型;
  r21 = r20 == 10;
  if (r21) goto if_then_20; else goto if_merge_21;

  if_then_20:
  return "关键字_整数";
  goto if_merge_21;

  if_merge_21:
  r22 = cn_var_类型;
  r23 = r22 == 11;
  if (r23) goto if_then_22; else goto if_merge_23;

  if_then_22:
  return "关键字_小数";
  goto if_merge_23;

  if_merge_23:
  r24 = cn_var_类型;
  r25 = r24 == 12;
  if (r25) goto if_then_24; else goto if_merge_25;

  if_then_24:
  return "关键字_字符串";
  goto if_merge_25;

  if_merge_25:
  r26 = cn_var_类型;
  r27 = r26 == 13;
  if (r27) goto if_then_26; else goto if_merge_27;

  if_then_26:
  return "关键字_布尔";
  goto if_merge_27;

  if_merge_27:
  r28 = cn_var_类型;
  r29 = r28 == 14;
  if (r29) goto if_then_28; else goto if_merge_29;

  if_then_28:
  return "关键字_空类型";
  goto if_merge_29;

  if_merge_29:
  r30 = cn_var_类型;
  r31 = r30 == 15;
  if (r31) goto if_then_30; else goto if_merge_31;

  if_then_30:
  return "关键字_结构体";
  goto if_merge_31;

  if_merge_31:
  r32 = cn_var_类型;
  r33 = r32 == 16;
  if (r33) goto if_then_32; else goto if_merge_33;

  if_then_32:
  return "关键字_枚举";
  goto if_merge_33;

  if_merge_33:
  r34 = cn_var_类型;
  r35 = r34 == 17;
  if (r35) goto if_then_34; else goto if_merge_35;

  if_then_34:
  return "关键字_函数";
  goto if_merge_35;

  if_merge_35:
  r36 = cn_var_类型;
  r37 = r36 == 18;
  if (r37) goto if_then_36; else goto if_merge_37;

  if_then_36:
  return "关键字_变量";
  goto if_merge_37;

  if_merge_37:
  r38 = cn_var_类型;
  r39 = r38 == 19;
  if (r39) goto if_then_38; else goto if_merge_39;

  if_then_38:
  return "关键字_导入";
  goto if_merge_39;

  if_merge_39:
  r40 = cn_var_类型;
  r41 = r40 == 20;
  if (r41) goto if_then_40; else goto if_merge_41;

  if_then_40:
  return "关键字_从";
  goto if_merge_41;

  if_merge_41:
  r42 = cn_var_类型;
  r43 = r42 == 21;
  if (r43) goto if_then_42; else goto if_merge_43;

  if_then_42:
  return "关键字_公开";
  goto if_merge_43;

  if_merge_43:
  r44 = cn_var_类型;
  r45 = r44 == 22;
  if (r45) goto if_then_44; else goto if_merge_45;

  if_then_44:
  return "关键字_私有";
  goto if_merge_45;

  if_merge_45:
  r46 = cn_var_类型;
  r47 = r46 == 23;
  if (r47) goto if_then_46; else goto if_merge_47;

  if_then_46:
  return "关键字_静态";
  goto if_merge_47;

  if_merge_47:
  r48 = cn_var_类型;
  r49 = r48 == 24;
  if (r49) goto if_then_48; else goto if_merge_49;

  if_then_48:
  return "关键字_真";
  goto if_merge_49;

  if_merge_49:
  r50 = cn_var_类型;
  r51 = r50 == 25;
  if (r51) goto if_then_50; else goto if_merge_51;

  if_then_50:
  return "关键字_假";
  goto if_merge_51;

  if_merge_51:
  r52 = cn_var_类型;
  r53 = r52 == 26;
  if (r53) goto if_then_52; else goto if_merge_53;

  if_then_52:
  return "关键字_无";
  goto if_merge_53;

  if_merge_53:
  r54 = cn_var_类型;
  r55 = r54 == 27;
  if (r55) goto if_then_54; else goto if_merge_55;

  if_then_54:
  return "关键字_类";
  goto if_merge_55;

  if_merge_55:
  r56 = cn_var_类型;
  r57 = r56 == 28;
  if (r57) goto if_then_56; else goto if_merge_57;

  if_then_56:
  return "关键字_接口";
  goto if_merge_57;

  if_merge_57:
  r58 = cn_var_类型;
  r59 = r58 == 29;
  if (r59) goto if_then_58; else goto if_merge_59;

  if_then_58:
  return "关键字_保护";
  goto if_merge_59;

  if_merge_59:
  r60 = cn_var_类型;
  r61 = r60 == 30;
  if (r61) goto if_then_60; else goto if_merge_61;

  if_then_60:
  return "关键字_虚拟";
  goto if_merge_61;

  if_merge_61:
  r62 = cn_var_类型;
  r63 = r62 == 31;
  if (r63) goto if_then_62; else goto if_merge_63;

  if_then_62:
  return "关键字_重写";
  goto if_merge_63;

  if_merge_63:
  r64 = cn_var_类型;
  r65 = r64 == 32;
  if (r65) goto if_then_64; else goto if_merge_65;

  if_then_64:
  return "关键字_抽象";
  goto if_merge_65;

  if_merge_65:
  r66 = cn_var_类型;
  r67 = r66 == 33;
  if (r67) goto if_then_66; else goto if_merge_67;

  if_then_66:
  return "关键字_实现";
  goto if_merge_67;

  if_merge_67:
  r68 = cn_var_类型;
  r69 = r68 == 34;
  if (r69) goto if_then_68; else goto if_merge_69;

  if_then_68:
  return "关键字_自身";
  goto if_merge_69;

  if_merge_69:
  r70 = cn_var_类型;
  r71 = r70 == 35;
  if (r71) goto if_then_70; else goto if_merge_71;

  if_then_70:
  return "关键字_基类";
  goto if_merge_71;

  if_merge_71:
  r72 = cn_var_类型;
  r73 = r72 == 36;
  if (r73) goto if_then_72; else goto if_merge_73;

  if_then_72:
  return "关键字_尝试";
  goto if_merge_73;

  if_merge_73:
  r74 = cn_var_类型;
  r75 = r74 == 37;
  if (r75) goto if_then_74; else goto if_merge_75;

  if_then_74:
  return "关键字_捕获";
  goto if_merge_75;

  if_merge_75:
  r76 = cn_var_类型;
  r77 = r76 == 38;
  if (r77) goto if_then_76; else goto if_merge_77;

  if_then_76:
  return "关键字_抛出";
  goto if_merge_77;

  if_merge_77:
  r78 = cn_var_类型;
  r79 = r78 == 39;
  if (r79) goto if_then_78; else goto if_merge_79;

  if_then_78:
  return "关键字_最终";
  goto if_merge_79;

  if_merge_79:
  r80 = cn_var_类型;
  r81 = r80 == 40;
  if (r81) goto if_then_80; else goto if_merge_81;

  if_then_80:
  return "标识符";
  goto if_merge_81;

  if_merge_81:
  r82 = cn_var_类型;
  r83 = r82 == 41;
  if (r83) goto if_then_82; else goto if_merge_83;

  if_then_82:
  return "整数字面量";
  goto if_merge_83;

  if_merge_83:
  r84 = cn_var_类型;
  r85 = r84 == 42;
  if (r85) goto if_then_84; else goto if_merge_85;

  if_then_84:
  return "浮点字面量";
  goto if_merge_85;

  if_merge_85:
  r86 = cn_var_类型;
  r87 = r86 == 43;
  if (r87) goto if_then_86; else goto if_merge_87;

  if_then_86:
  return "字符串字面量";
  goto if_merge_87;

  if_merge_87:
  r88 = cn_var_类型;
  r89 = r88 == 44;
  if (r89) goto if_then_88; else goto if_merge_89;

  if_then_88:
  return "字符字面量";
  goto if_merge_89;

  if_merge_89:
  r90 = cn_var_类型;
  r91 = r90 == 45;
  if (r91) goto if_then_90; else goto if_merge_91;

  if_then_90:
  return "加号";
  goto if_merge_91;

  if_merge_91:
  r92 = cn_var_类型;
  r93 = r92 == 46;
  if (r93) goto if_then_92; else goto if_merge_93;

  if_then_92:
  return "减号";
  goto if_merge_93;

  if_merge_93:
  r94 = cn_var_类型;
  r95 = r94 == 47;
  if (r95) goto if_then_94; else goto if_merge_95;

  if_then_94:
  return "星号";
  goto if_merge_95;

  if_merge_95:
  r96 = cn_var_类型;
  r97 = r96 == 48;
  if (r97) goto if_then_96; else goto if_merge_97;

  if_then_96:
  return "斜杠";
  goto if_merge_97;

  if_merge_97:
  r98 = cn_var_类型;
  r99 = r98 == 49;
  if (r99) goto if_then_98; else goto if_merge_99;

  if_then_98:
  return "百分号";
  goto if_merge_99;

  if_merge_99:
  r100 = cn_var_类型;
  r101 = r100 == 50;
  if (r101) goto if_then_100; else goto if_merge_101;

  if_then_100:
  return "等于";
  goto if_merge_101;

  if_merge_101:
  r102 = cn_var_类型;
  r103 = r102 == 51;
  if (r103) goto if_then_102; else goto if_merge_103;

  if_then_102:
  return "赋值";
  goto if_merge_103;

  if_merge_103:
  r104 = cn_var_类型;
  r105 = r104 == 52;
  if (r105) goto if_then_104; else goto if_merge_105;

  if_then_104:
  return "不等于";
  goto if_merge_105;

  if_merge_105:
  r106 = cn_var_类型;
  r107 = r106 == 53;
  if (r107) goto if_then_106; else goto if_merge_107;

  if_then_106:
  return "小于";
  goto if_merge_107;

  if_merge_107:
  r108 = cn_var_类型;
  r109 = r108 == 54;
  if (r109) goto if_then_108; else goto if_merge_109;

  if_then_108:
  return "小于等于";
  goto if_merge_109;

  if_merge_109:
  r110 = cn_var_类型;
  r111 = r110 == 55;
  if (r111) goto if_then_110; else goto if_merge_111;

  if_then_110:
  return "大于";
  goto if_merge_111;

  if_merge_111:
  r112 = cn_var_类型;
  r113 = r112 == 56;
  if (r113) goto if_then_112; else goto if_merge_113;

  if_then_112:
  return "大于等于";
  goto if_merge_113;

  if_merge_113:
  r114 = cn_var_类型;
  r115 = r114 == 57;
  if (r115) goto if_then_114; else goto if_merge_115;

  if_then_114:
  return "逻辑与";
  goto if_merge_115;

  if_merge_115:
  r116 = cn_var_类型;
  r117 = r116 == 58;
  if (r117) goto if_then_116; else goto if_merge_117;

  if_then_116:
  return "逻辑或";
  goto if_merge_117;

  if_merge_117:
  r118 = cn_var_类型;
  r119 = r118 == 59;
  if (r119) goto if_then_118; else goto if_merge_119;

  if_then_118:
  return "逻辑非";
  goto if_merge_119;

  if_merge_119:
  r120 = cn_var_类型;
  r121 = r120 == 60;
  if (r121) goto if_then_120; else goto if_merge_121;

  if_then_120:
  return "按位与";
  goto if_merge_121;

  if_merge_121:
  r122 = cn_var_类型;
  r123 = r122 == 61;
  if (r123) goto if_then_122; else goto if_merge_123;

  if_then_122:
  return "按位或";
  goto if_merge_123;

  if_merge_123:
  r124 = cn_var_类型;
  r125 = r124 == 62;
  if (r125) goto if_then_124; else goto if_merge_125;

  if_then_124:
  return "按位异或";
  goto if_merge_125;

  if_merge_125:
  r126 = cn_var_类型;
  r127 = r126 == 63;
  if (r127) goto if_then_126; else goto if_merge_127;

  if_then_126:
  return "按位取反";
  goto if_merge_127;

  if_merge_127:
  r128 = cn_var_类型;
  r129 = r128 == 64;
  if (r129) goto if_then_128; else goto if_merge_129;

  if_then_128:
  return "左移";
  goto if_merge_129;

  if_merge_129:
  r130 = cn_var_类型;
  r131 = r130 == 65;
  if (r131) goto if_then_130; else goto if_merge_131;

  if_then_130:
  return "右移";
  goto if_merge_131;

  if_merge_131:
  r132 = cn_var_类型;
  r133 = r132 == 66;
  if (r133) goto if_then_132; else goto if_merge_133;

  if_then_132:
  return "自增";
  goto if_merge_133;

  if_merge_133:
  r134 = cn_var_类型;
  r135 = r134 == 67;
  if (r135) goto if_then_134; else goto if_merge_135;

  if_then_134:
  return "自减";
  goto if_merge_135;

  if_merge_135:
  r136 = cn_var_类型;
  r137 = r136 == 68;
  if (r137) goto if_then_136; else goto if_merge_137;

  if_then_136:
  return "箭头";
  goto if_merge_137;

  if_merge_137:
  r138 = cn_var_类型;
  r139 = r138 == 69;
  if (r139) goto if_then_138; else goto if_merge_139;

  if_then_138:
  return "左括号";
  goto if_merge_139;

  if_merge_139:
  r140 = cn_var_类型;
  r141 = r140 == 70;
  if (r141) goto if_then_140; else goto if_merge_141;

  if_then_140:
  return "右括号";
  goto if_merge_141;

  if_merge_141:
  r142 = cn_var_类型;
  r143 = r142 == 71;
  if (r143) goto if_then_142; else goto if_merge_143;

  if_then_142:
  return "左大括号";
  goto if_merge_143;

  if_merge_143:
  r144 = cn_var_类型;
  r145 = r144 == 72;
  if (r145) goto if_then_144; else goto if_merge_145;

  if_then_144:
  return "右大括号";
  goto if_merge_145;

  if_merge_145:
  r146 = cn_var_类型;
  r147 = r146 == 73;
  if (r147) goto if_then_146; else goto if_merge_147;

  if_then_146:
  return "左方括号";
  goto if_merge_147;

  if_merge_147:
  r148 = cn_var_类型;
  r149 = r148 == 74;
  if (r149) goto if_then_148; else goto if_merge_149;

  if_then_148:
  return "右方括号";
  goto if_merge_149;

  if_merge_149:
  r150 = cn_var_类型;
  r151 = r150 == 75;
  if (r151) goto if_then_150; else goto if_merge_151;

  if_then_150:
  return "分号";
  goto if_merge_151;

  if_merge_151:
  r152 = cn_var_类型;
  r153 = r152 == 76;
  if (r153) goto if_then_152; else goto if_merge_153;

  if_then_152:
  return "逗号";
  goto if_merge_153;

  if_merge_153:
  r154 = cn_var_类型;
  r155 = r154 == 77;
  if (r155) goto if_then_154; else goto if_merge_155;

  if_then_154:
  return "点";
  goto if_merge_155;

  if_merge_155:
  r156 = cn_var_类型;
  r157 = r156 == 78;
  if (r157) goto if_then_156; else goto if_merge_157;

  if_then_156:
  return "冒号";
  goto if_merge_157;

  if_merge_157:
  r158 = cn_var_类型;
  r159 = r158 == 79;
  if (r159) goto if_then_158; else goto if_merge_159;

  if_then_158:
  return "问号";
  goto if_merge_159;

  if_merge_159:
  r160 = cn_var_类型;
  r161 = r160 == 80;
  if (r161) goto if_then_160; else goto if_merge_161;

  if_then_160:
  return "结束";
  goto if_merge_161;

  if_merge_161:
  r162 = cn_var_类型;
  r163 = r162 == 81;
  if (r163) goto if_then_162; else goto if_merge_163;

  if_then_162:
  return "错误";
  goto if_merge_163;

  if_merge_163:
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
  long long r0, r5, r10, r15, r20, r25;
  _Bool r2;
  _Bool r4;
  _Bool r7;
  _Bool r9;
  _Bool r12;
  _Bool r14;
  _Bool r17;
  _Bool r19;
  _Bool r22;
  _Bool r24;
  _Bool r27;
  _Bool r29;
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
  if (r2) goto logic_rhs_166; else goto logic_merge_167;

  if_then_164:
  return 1;
  goto if_merge_165;

  if_merge_165:
  r6 = cn_var_类型;
  r7 = r6 >= 10;
  if (r7) goto logic_rhs_170; else goto logic_merge_171;

  logic_rhs_166:
  r3 = cn_var_类型;
  r4 = r3 <= 9;
  goto logic_merge_167;

  logic_merge_167:
  if (r4) goto if_then_164; else goto if_merge_165;

  if_then_168:
  return 1;
  goto if_merge_169;

  if_merge_169:
  r11 = cn_var_类型;
  r12 = r11 >= 17;
  if (r12) goto logic_rhs_174; else goto logic_merge_175;

  logic_rhs_170:
  r8 = cn_var_类型;
  r9 = r8 <= 16;
  goto logic_merge_171;

  logic_merge_171:
  if (r9) goto if_then_168; else goto if_merge_169;

  if_then_172:
  return 1;
  goto if_merge_173;

  if_merge_173:
  r16 = cn_var_类型;
  r17 = r16 >= 24;
  if (r17) goto logic_rhs_178; else goto logic_merge_179;

  logic_rhs_174:
  r13 = cn_var_类型;
  r14 = r13 <= 23;
  goto logic_merge_175;

  logic_merge_175:
  if (r14) goto if_then_172; else goto if_merge_173;

  if_then_176:
  return 1;
  goto if_merge_177;

  if_merge_177:
  r21 = cn_var_类型;
  r22 = r21 >= 27;
  if (r22) goto logic_rhs_182; else goto logic_merge_183;

  logic_rhs_178:
  r18 = cn_var_类型;
  r19 = r18 <= 26;
  goto logic_merge_179;

  logic_merge_179:
  if (r19) goto if_then_176; else goto if_merge_177;

  if_then_180:
  return 1;
  goto if_merge_181;

  if_merge_181:
  r26 = cn_var_类型;
  r27 = r26 >= 36;
  if (r27) goto logic_rhs_186; else goto logic_merge_187;

  logic_rhs_182:
  r23 = cn_var_类型;
  r24 = r23 <= 35;
  goto logic_merge_183;

  logic_merge_183:
  if (r24) goto if_then_180; else goto if_merge_181;

  if_then_184:
  return 1;
  goto if_merge_185;

  if_merge_185:
  return 0;

  logic_rhs_186:
  r28 = cn_var_类型;
  r29 = r28 <= 39;
  goto logic_merge_187;

  logic_merge_187:
  if (r29) goto if_then_184; else goto if_merge_185;
  return 0;
}

_Bool 是字面量(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  _Bool r2;
  _Bool r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 40;
  if (r2) goto logic_rhs_188; else goto logic_merge_189;

  logic_rhs_188:
  r3 = cn_var_类型;
  r4 = r3 <= 44;
  goto logic_merge_189;

  logic_merge_189:
  return r4;
}

_Bool 是运算符(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  _Bool r2;
  _Bool r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 45;
  if (r2) goto logic_rhs_190; else goto logic_merge_191;

  logic_rhs_190:
  r3 = cn_var_类型;
  r4 = r3 <= 68;
  goto logic_merge_191;

  logic_merge_191:
  return r4;
}

_Bool 是分隔符(enum 词元类型枚举 cn_var_类型) {
  long long r0;
  _Bool r2;
  _Bool r4;
  enum 词元类型枚举 r1;
  enum 词元类型枚举 r3;

  entry:
  r1 = cn_var_类型;
  r2 = r1 >= 69;
  if (r2) goto logic_rhs_192; else goto logic_merge_193;

  logic_rhs_192:
  r3 = cn_var_类型;
  r4 = r3 <= 79;
  goto logic_merge_193;

  logic_merge_193:
  return r4;
}

