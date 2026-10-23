#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 生成器（支柱二·plans/026 §2.2·245-a）

按规范文法生成随机但合法的 CN 程序（确定性种子可复现）。
首期生成面：整32/整64 变量、算术（+ - * / %）、位运算（& | ^ << >>）、
比较（== != < <= > >=）、逻辑（&& || !）、三元、打印行。
1022 扩面（#1398）：`--族 混合`（默认）按 seed 轮转「枚举数组/重载后缀/
默认参数/转义字符串」四族——每 5 个 seed 覆盖标量面+四族。四族用于让
#1393~#1397（v2 枚举宽度/重载后缀/默认参数/NUL 转义）在自动对拍轨道显形，
修复前对拍与矩阵轨道对这些族预期红（红在明处·勿误判工具回归）。

「只生成合法程序」契约（847 收口）由二层保护承担：
  ①移位量域保护：移位量=操作数类型位宽域内小字面量（474 立法）；
  ②常量折叠溢出保护：常量链折叠值镜像编译器预演域（_算术折叠/…）——
    超域节点主动修正为恒合法形态（同除数非零保护先例）。
自校验：python scripts/cnsmith_gen.py --selftest [--编译 target/cn.exe]
  （移位域+折叠域+采样生成面静态三断言；给 --编译 时加样本全编译通过
  + 危险表达式全被拒双向对拍）。

用法：python scripts/cnsmith_gen.py --seed 1 --count 10 --out target/cnsmith
"""
import argparse
import os
import random
import re
import shutil
import subprocess

NL = "\n"

# 折叠预演域（847）：镜像编译器常量折叠/溢出预演口径
#   （semantic_expr_binary.cpp reportConstOverflow + model/semantic_helpers.hpp
#   cnFoldConstIntExpr505——int64 承载闭包 + 结果类型域预演）
INT32_MIN = -2147483648
INT32_MAX = 2147483647
INT64_MIN = -9223372036854775808
INT64_MAX = 9223372036854775807

# 474 立法：移位量合法域 [0, 操作数类型位宽)——k ≥ 位宽=编译期硬错误
类型位宽 = {"整32": 32, "整64": 64}


def _超整32(值):
    """值超整32 域判据（编译器预演升格条件——reportConstOverflow promotedType 同口径）。"""
    return 值 < INT32_MIN or 值 > INT32_MAX


def _算术求值(op, a, b):
    """+ - * 求值（Python 大整数；闭集三算子显式枚举）。"""
    if op == "+":
        return a + b
    if op == "-":
        return a - b
    if op == "*":
        return a * b
    raise AssertionError("闭集外算术算符：%s" % op)   # 穷举断言（miss 不可静默）


def _截断除(a, b):
    """向零截断除法（与宿主常量折叠 C++ '/' 口径一致；生成面除数恒 1..9 正数）。"""
    return a // b if a >= 0 else -((-a) // b)


def _域断言(值):
    """（847 自校验）折叠值承载域不变量：可折叠值必落 int64 闭包域。

    判据保证（<< 超位域不可折叠/修正后值域内）失效时立即显形——防折叠
    模拟与编译器口径偏离静默产出非法样本。"""
    assert INT64_MIN <= 值 <= INT64_MAX, "折叠值超 int64 承载域：%d" % 值
    return 值


# =====================================================================
# 1022 扩面（#1398）：四族结构程序生成
#   背景（2026-10-22 采样轮实证）：原随机面只产 主() 内整32/整64 标量语句，
#   枚举数组/重载后缀/默认参数表达式/转义字符串四族零覆盖——自动三轨道全绿
#   而 22 项红全靠手写探针。本段按 seed 轮转四族（每 5 个 seed 覆盖标量面+四族）。
#   合法性契约=宿主编译通过；v2 侧分叉=对拍轨道命中面（#1393~#1397 锁面）。
#   ⚠ 修复前对拍/矩阵轨道对这些族预期红（红在明处，勿误判工具回归 revert）。
# =====================================================================
族名们 = ("枚举数组", "重载后缀", "默认参数", "转义字符串")


def 族程序(族名, seed):
    """四族完整程序生成（确定性：族内 rng=seed*1009+族序）。

    返回源码带 `// 族=<名>` 标记（selftest 覆盖断言消费）。每族两变体，
    全部为宿主可编译的合法程序；值取小域确定形态，避免常量折叠/移位保护面。
    """
    rng = random.Random(seed * 1009 + 族名们.index(族名))
    if 族名 == "枚举数组":
        if rng.random() < 0.5:
            return r'''// 族=枚举数组
枚举 颜色 { 红, 绿, 蓝 }
函数 枚举和(颜色[3] 表) -> 整32 {
    返回 整32(表[0]) + 整32(表[1]) + 整32(表[2]);
}
函数 主() -> 整32 {
    颜色[3] 表 = {颜色.红, 颜色.绿, 颜色.蓝};
    打印(整32(表[0]), " ", 整32(表[1]), " ", 整32(表[2]));
    打印(枚举和(表));
    返回 0;
}
'''
        return r'''// 族=枚举数组
枚举 方向 { 上 = -1, 中 = 0, 下 = 1 }
函数 主() -> 整32 {
    方向[3] 表;
    表[0] = 方向.上;
    表[1] = 方向.中;
    表[2] = 方向.下;
    循环 (整32 i = 0; i < 3; i++) {
        打印(整32(表[i]), " ");
    }
    返回 0;
}
'''
    if 族名 == "重载后缀":
        if rng.random() < 0.5:
            return r'''// 族=重载后缀
函数 取(整32 x) -> 整32 { 返回 x + 1; }
函数 取(整64 x) -> 整64 { 返回 x + 2L; }
函数 述(浮32 x) -> 整32 { 返回 1; }
函数 述(浮64 x) -> 整32 { 返回 2; }
函数 主() -> 整32 {
    打印(取(1), " ", 取(1L));
    打印(述(1.5f), " ", 述(1.5));
    返回 0;
}
'''
        return r'''// 族=重载后缀
函数 选(正32 x) -> 整32 { 返回 1; }
函数 选(正64 x) -> 整32 { 返回 2; }
函数 取(整32 x) -> 整32 { 返回 x + 1; }
函数 取(整64 x) -> 整64 { 返回 x + 2L; }
函数 主() -> 整32 {
    打印(选(1U), " ", 选(1UL));
    打印(取(1), " ", 取(1L));
    返回 0;
}
'''
    if 族名 == "默认参数":
        if rng.random() < 0.5:
            return r'''// 族=默认参数
常量 基 = 5;
函数 算(整32 a = 1 + 2, 整32 b = 基) -> 整32 {
    返回 a + b;
}
函数 主() -> 整32 {
    打印(算());
    打印(算(10));
    返回 0;
}
'''
        return r'''// 族=默认参数
类 盒 {
公开:
    整32 值;
    函数 盒(整32 n = 2 + 3) { 值 = n; }
}
函数 主() -> 整32 {
    盒 b = 盒();
    打印(b.值);
    返回 0;
}
'''
    if 族名 == "转义字符串":
        if rng.random() < 0.5:
            return r'''// 族=转义字符串
函数 主() -> 整32 {
    字符串 s = "a\0b";
    打印(字符串长度(s), " ", s[0]);
    字符串 r = 原始"a\tb";
    打印(字符串长度(r));
    返回 0;
}
'''
        return r'''// 族=转义字符串
函数 主() -> 整32 {
    字符串 甲 = 多行"""第一行
第二行""";
    字符串 乙 = 原始多行"""甲\n乙""";
    打印(字符串长度(甲), " ", 字符串长度(乙));
    返回 0;
}
'''
    raise AssertionError("闭集外族名：%s" % 族名)   # 穷举断言（miss 不可静默）


class Gen:
    def __init__(self, seed):
        self.seed = seed
        self.rng = random.Random(seed)
        self.vars = []          # (名称, 类型) 整32/整64
        self.flt_vars = []      # (名称,) 浮64（1306 推断声明面产物·供后续语句复用）
        self.tmp = 0
        # 847 自校验审计面：常量折叠超域被修正为安全形态的「原表达式」文本
        # （selftest 编译对拍用——每个原表达式应被编译器真实拒绝）
        self.审计危险 = []

    def new_name(self, typ):
        self.tmp += 1
        return "变%d" % self.tmp

    def declare(self, name, typ):
        self.vars.append((name, typ))

    def expr(self, depth, typ="整32"):
        """生成一个 typ 类型表达式；返回 (文本, 折叠值)。

        折叠值=镜像编译器常量折叠器（cnFoldConstIntExpr505）的可折叠量：
        int=常量闭包值（字面量/一元负/算术/除模/移位），None=不可折叠
        （变量/显式转换/位运算/比较/逻辑/三元——折叠器不进这些节点）。
        仅 + - * 节点消费之做溢出预演保护（_算术折叠）——847 契约：
        生成的样本对当前编译器全部合法。"""
        if depth <= 0 or self.rng.random() < 0.35:
            return self.atom(typ)
        kind = self.rng.randrange(6)
        if typ == "整32" and kind == 0:
            # 算术二元（折叠值跟踪见 _算术折叠——847 防常量折叠溢出拒绝）
            op = self.rng.choice(["+", "-", "*"])
            lt, lv = self.expr(depth - 1)
            rt, rv = self.expr(depth - 1)
            return self._算术折叠(op, lt, rt, lv, rv)
        if typ == "整32" and kind == 1:
            # 位运算/移位；移位量=域内小字面量（474 执法保护——同除数非零
            #   保护先例。167 轨道入口实测：原通用 expr 生成常量折叠后超宽移位量→
            #   宿主 474 执法拒 16/24=采样面大面积无效，违背「随机但合法」本职）
            op = self.rng.choice(["&", "|", "^", "<<", ">>"])
            if op in ("<<", ">>"):
                # 840 轮修（生成面缺陷·任务 474 同族）：移位量须在类型位宽域内
                #   [0,32)——原实现右操作数为任意表达式，可产 513/-415 超宽移位量
                #   → 编译期诊断拒绝（s6/s8 实测 build_err）：「生成合法程序」承诺
                #   被破坏，合法轨产出非法样本污染差分信噪比。右操作数=域内小字面量。
                #   （167 独立同处发现·两源互证：合并取 840 注释版·代码两侧全同）
                #   847 收口：域按目标类型位宽取 [0, 类型位宽)——当前移位生成面
                #   =整32（分支条件），randint 参数 (0,31) 与修复前逐位一致。
                lt, lv = self.expr(depth - 1)
                k = self.rng.randint(0, 类型位宽[typ] - 1)
                return self._移位折叠(op, lt, lv, k)
            lt, _ = self.expr(depth - 1)
            rt, _ = self.expr(depth - 1)
            # 位运算不进折叠闭包（cnFoldConstIntExpr505 default——不可折叠）
            return "(%s %s %s)" % (lt, op, rt), None
        if kind == 2 and depth >= 2:
            # 除法/取模（除数恒非零保护：字面量 1..9）
            op = self.rng.choice(["/", "%"])
            lt, lv = self.expr(depth - 1)
            rv = self.rng.randint(1, 9)
            return self._除模折叠(op, lt, lv, rv)
        if typ == "整32" and kind == 3:
            # 比较产生布尔 -> 再转整（三元取整分支）；比较不进折叠闭包
            ops = ["==", "!=", "<", "<=", ">", ">="]
            lt, _ = self.expr(depth - 1)
            cmp_op = self.rng.choice(ops)
            rt, _ = self.expr(depth - 1)
            b = "(%s %s %s)" % (lt, cmp_op, rt)
            return "(%s ? 1 : 0)" % b, None
        if typ == "整32" and kind == 4 and depth >= 2:
            # 逻辑组合（操作数=比较表达式·布尔）转 0/1；逻辑/三元不进折叠闭包
            ops = ["==", "!=", "<", "<=", ">", ">="]
            op = self.rng.choice(["&&", "||"])
            cmp_op = self.rng.choice(ops)
            l1, _ = self.expr(depth - 2)
            r1, _ = self.expr(depth - 2)
            lhs = "(%s %s %s)" % (l1, cmp_op, r1)
            cmp_op2 = self.rng.choice(ops)
            l2, _ = self.expr(depth - 2)
            r2, _ = self.expr(depth - 2)
            rhs = "(%s %s %s)" % (l2, cmp_op2, r2)
            return "((%s %s %s) ? 1 : 0)" % (lhs, op, rhs), None
        return self.atom(typ)

    def atom(self, typ):
        """原子表达式；返回 (文本, 折叠值)。"""
        r = self.rng.random()
        if self.vars and r < 0.4:
            # 复用已声明变量（类型匹配优先，类型不同做显式转换保证合法）
            name, vt = self.rng.choice(self.vars)
            if vt == typ:
                return name, None
            if typ == "整32":
                return "整32(%s)" % name, None    # 显式转换不可折叠
            return "整64(%s)" % name, None
        if typ == "整64":
            # 整64 字面量不进整32 折叠跟踪（整64 无算术生成面——值无消费点）
            return "%dL" % self.rng.randint(-100000, 100000), None
        v = self.rng.randint(-1000, 1000)
        return str(v), v

    def program(self, stmt_n=14):
        # 1339 扩轴（#1337/#1338 遮蔽族+1390 备注证据链）：全局变量声明带+辅助
        #   读/改函数+主函数内同名遮蔽+循环/如果控制流——覆盖「全局交互/遮蔽
        #   写读/跨函数可见性/Mem2Reg 跨块 φ」结构性盲区。门控=self.seed 奇偶
        #   （偶=新轴·奇=原路径 rng 消费序逐字节不变=旧面零回归）。
        新轴 = (self.seed % 2 == 0)
        out = []
        if 新轴:
            for gi in range(1, self.rng.randint(2, 4)):
                out.append("整32 全%d = %d;" % (gi, self.rng.randint(0, 100)))
            out.append("函数 读全1() -> 整32 { 返回 全1; }")
            out.append("不安全 函数 改全1(整32 v) -> 空类型 { 全1 = v; }")
        out.append("函数 主() -> 整32 {")
        out.append('    打印行("=== CN-Smith 采样 ===");')
        if 新轴:
            out.append("    整32 全1 = %d;" % self.rng.randint(200, 300))
            out.append('    打印行("遮蔽局部全1 = ", 全1);')
        for _ in range(stmt_n):
            out.append(self.statement())
        if 新轴:
            out.append("    循环 (整32 轮 = 0; 轮 < 3; 轮 += 1) {")
            out.append("        改全1(读全1() + 轮);")
            out.append("        如果 (轮 == 1) {")
            out.append('            打印行("轮1 读全1 = ", 读全1());')
            out.append("        } 否则 {")
            out.append('            打印行("轮非1 跳过");')
            out.append("        }")
            out.append("    }")
        out.append('    打印行("done");')
        out.append("    返回 0;")
        out.append("}")
        return NL.join(out)

    def program_mixed(self, stmt_n=14):
        """混合面（1022·#1398）：seed%5==0 用原标量面，1~4 依次四族。

        连续 5 个 seed 覆盖「标量+枚举数组+重载后缀+默认参数+转义字符串」；
        全部面均遵守「只生成合法程序」契约（合法性=宿主编译通过）。"""
        序 = self.seed % 5
        if 序 == 0:
            return self.program(stmt_n)
        return 族程序(族名们[序 - 1], self.seed)

    def statement(self):
        if self.rng.random() < 0.2:
            return self._推断语句()
        typ = self.rng.choice(["整32", "整32", "整32", "整64"])
        name = self.new_name(typ)
        val, _ = self.expr(self.rng.randint(2, 5), typ)  # 先生成值（未登记，避免自引用）
        self.declare(name, typ)
        head = "整64" if typ == "整64" else "整32"
        line = "    %s %s = %s;" % (head, name, val)
        if self.rng.random() < 0.6:
            line += NL + '    打印行("%s = ", %s);' % (name, name)
        return line

    def _推断语句(self):
        """推断声明语句（1306 扩面·#1304/#1305 形态域）：变量 名 = <表达式>;

        三型（宿主全部合法——合法性契约=宿主编译通过；对拍轨道中 v2 拒=
        分叉命中即扩面目的，非生成器缺陷）：
        ①拼接型：字符串字面量 + 整值 + 字面量 + 浮值 + 字面量 混链
          （001 §253 数值->字符串自动转换承诺面——v2 推导通道曾全拒）；
        ②浮整混算型：整值×浮字面量 两序（宿主 usual arithmetic 提升浮64）；
        ③三元跨类型：整条件 ? 整分支 : 浮分支（宿主提升合并·#1305 形态域）。
        浮64 结果入 flt_vars 复用池；字符串结果打印消费即止（不入池）。"""
        name = self.new_name("推断")
        kind = self.rng.randrange(3)
        if kind == 0:
            if self.vars and self.rng.random() < 0.6:
                整段 = self.rng.choice(self.vars)[0]
            else:
                整段 = str(self.rng.randint(0, 999))
            if self.flt_vars and self.rng.random() < 0.4:
                浮段 = self.rng.choice(self.flt_vars)
            else:
                浮段 = self.rng.choice(["1.5", "2.25", "0.5", "3.5"])
            line = '    变量 %s = "%s" + %s + "%s" + %s + "%s";' % (
                name, self.rng.choice(["前", "甲", "x", "v"]), 整段,
                self.rng.choice(["中", "乙", "-", "."]), 浮段,
                self.rng.choice(["尾", "end", "!", "9"]))
        elif kind == 1:
            整段 = self.rng.choice(self.vars)[0] if self.vars else \
                str(self.rng.randint(1, 99))
            flt = self.rng.choice(["1.5", "2.5", "0.25", "4.0"])
            op = self.rng.choice(["+", "-", "*"])
            if self.rng.random() < 0.5:
                line = "    变量 %s = %s %s %s;" % (name, 整段, op, flt)
            else:
                line = "    变量 %s = %s %s %s;" % (name, flt, op, 整段)
            self.flt_vars.append(name)
        else:
            条件 = self.rng.choice(self.vars)[0] if self.vars else \
                str(self.rng.randint(0, 9))
            line = "    变量 %s = (%s > 0) ? 1 : 2.5;" % (name, 条件)
            self.flt_vars.append(name)
        line += NL + '    打印行("%s = ", %s);' % (name, name)
        return line

    # ------------------------------------------------------------------
    # 常量折叠溢出保护（847 收口：生成器「只生成合法程序」契约）
    #   病根实录（2026-10-17 复核新 seed 120 样本）：3 个样本被「编译期整数
    #   溢出」拒绝——常量链折叠超整32 域（((-379*-130)*411)*110、
    #   ((767<<17)*494) 等；001 §1.1a①/Rust debug 对齐硬错误）。840/167 只修了
    #   移位量域，未防折叠超域——契约仍破 2.5%。
    #   修法：expr 递归携带折叠值（镜像 cnFoldConstIntExpr505 闭包——
    #   字面量/一元负/算术/除模/移位可折叠，位/比较/逻辑/三元不可折叠），
    #   + - * 节点按编译器预演口径预检（reportConstOverflow：任一侧折叠值超
    #   整32 域→按整64 域预演，否则整32 域；结果超域=必被拒），超域节点主动
    #   修正为恒合法形态=右操作数置 0（同除数非零保护先例；修正不消耗 rng
    #   ——同 seed 下未触发修正的样本产出与修复前逐字节一致）。
    # ------------------------------------------------------------------

    def _算术折叠(self, op, lt, rt, lv, rv):
        """+ - * 节点文本+折叠值；折叠结果超预演域时主动修正（右操作数置 0）。"""
        if lv is None or rv is None:
            return "(%s %s %s)" % (lt, op, rt), None
        域上界 = INT64_MAX if (_超整32(lv) or _超整32(rv)) else INT32_MAX
        结果 = _算术求值(op, lv, rv)
        if -域上界 - 1 <= 结果 <= 域上界:
            return "(%s %s %s)" % (lt, op, rt), _域断言(结果)
        self.审计危险.append("(%s %s %s)" % (lt, op, rt))
        修正值 = _算术求值(op, lv, 0)
        return "(%s %s 0)" % (lt, op), _域断言(修正值)

    def _移位折叠(self, op, lt, lv, k):
        """<< >> 节点文本+折叠值。

        镜像折叠器：<< 仅当左值非负且不超 int64 位域才可折叠（负值左移/
        超位域=不可折叠——该形态编译器预演不消费，实测合法）；>> 恒算术
        右移可折叠（k∈[0,32) 域内）。"""
        if op == "<<":
            if lv is not None and lv >= 0 and lv <= (INT64_MAX >> k):
                return "(%s << %d)" % (lt, k), _域断言(lv << k)
            return "(%s << %d)" % (lt, k), None
        if lv is None:
            return "(%s >> %d)" % (lt, k), None
        return "(%s >> %d)" % (lt, k), _域断言(lv >> k)

    def _除模折叠(self, op, lt, lv, rv):
        """除/取模节点文本+折叠值（截断除法·余数符号同被除数——与宿主折叠口径一致）。

        除数恒 1..9 正数：除零/INT_MIN÷-1 溢出形态不可达；结果绝对值不超
        被除数→恒在 int64 承载域。"""
        if lv is None:
            return "(%s %s %d)" % (lt, op, rv), None
        q = _截断除(lv, rv)
        if op == "/":
            return "(%s / %d)" % (lt, rv), _域断言(q)
        return "(%s %% %d)" % (lt, rv), _域断言(lv - q * rv)


# 负向注入（246-a）：对合法程序注入一个随机语法/结构错误——
#   验证编译器「必有诊断、永不崩溃」（T6 族机械化）。
#   注入器=字符串级手术（生成后再破坏），不追求语义多样性（覆盖语法层防线）。
#   T28①根治（303-a）：①不再追加「# 注入类型」标记行（`#` 是预处理指令起始，
#   会掩盖真注入错误=判据不纯——注入元数据改落 <样本>.cn.neg 伴随文件）；
#   ②必真注入=在含目标特征的行中选（重试选行），全特征行缺失时换注入类型，
#   全类型不可注入则跳过该样本并计数（禁止静默假注入=样本仍合法）。
INJECTIONS = [
    "删分号",       # 语句缺少分号
    "缺右括号",     # 括号不闭合
    "缺右花括号",   # 块不闭合
    "非法字符",     # 词法层未知符号
    "截断文件",     # 结构突然中断
]


def inject_error(src, rng):
    """注入一个随机语法错误。返回 (源码|None, 注入类型|None, 注入行号 1 基|0)。

    必真注入保证：只在含目标特征的候选行中随机选（该行注入后必然语法非法）；
    候选为空则换下一个注入类型（随机序）；全部类型不可注入返回 None（调用方跳过）。"""
    kinds = INJECTIONS[:]
    rng.shuffle(kinds)
    lines = src.split(NL)
    for kind in kinds:
        out = _尝试注入(lines, kind, rng)
        if out is not None:
            return NL.join(out[0]), kind, out[1]
    return None, None, 0


def _尝试注入(lines, kind, rng):
    """单类型注入尝试：返回 (新行列表, 注入行号) 或 None（无可注入行）。

    1022（#1398 随批根治）：候选行一律排除注释行（`//`）与预处理行（`#`）——
    原实现只按「非空」选行，族程序首行 `// 族=…` 使「非法字符」注入追加进
    注释（样本仍合法 → 宿主误接受假命中）；「截断文件」也可能只删末尾空行。
    注入契约=必真破坏语法：候选必须落在代码行。"""
    out = list(lines)
    代码行 = [i for i, l in enumerate(out)
              if l.strip() and not l.strip().startswith("//")
              and not l.strip().startswith("#")]
    if kind == "删分号":
        cand = [i for i in 代码行 if ";" in out[i]]
        if not cand:
            return None
        i = rng.choice(cand)
        out[i] = out[i].replace(";", "", 1)
    elif kind == "缺右括号":
        cand = [i for i in 代码行 if ")" in out[i]]
        if not cand:
            return None
        i = rng.choice(cand)
        j = out[i].rfind(")")
        out[i] = out[i][:j] + out[i][j + 1:]
    elif kind == "缺右花括号":
        cand = [i for i, l in enumerate(out) if l.strip() == "}"]
        if not cand:
            return None
        i = rng.choice(cand)
        out.pop(i)
    elif kind == "非法字符":
        if not 代码行:
            return None
        i = rng.choice(代码行)
        out[i] = out[i] + " @@"
    elif kind == "截断文件":
        # 截断点=后半段代码行（排除注释/空行——只删空串不算截断）
        cand = [i for i in 代码行 if i >= len(out) // 2]
        if not cand:
            return None
        i = rng.choice(cand)
        out = out[:i]
    else:
        return None
    return out, i + 1


# =====================================================================
# 自校验（847 收口·判定契约=全绿 0/红 1——对齐 cnsmith_run 判定风格）
# =====================================================================


def selftest(编译路径=None, 目标=None):
    """生成器自校验：移位域（474）+折叠域（847 同族）+采样生成面。

    静态面（零依赖）：
      ①样本面=847 实录 12 样本 seed（1..12）+3 个实录危险 seed 采样；
      ②全部移位量 ∈ [0, 32)（474——防 840 缺陷重演锁面）；
      ③折叠值不变量（生成期 _域断言 内建·超域即异常显形）；
      ④生成面未退化+折叠保护审计面有捕获（miss 不可静默）。
    编译对拍（--编译 <cn.exe>）：⑤样本全编译通过；⑥审计危险表达式全被拒
      （折叠模拟与编译器口径双向闭环——偏差即显形）。
    """
    通过, 失败 = [], []
    语句数 = 14   # 对齐 CLI 默认 --stmts（实录样本生成口径）

    def 签(名, 条件, 详情=""):
        if 条件:
            通过.append(名)
        else:
            失败.append((名, 详情))

    # 实录危险 seed（2026-10-17 复核：s1019/s251015000020/s251017000004 曾被
    #   「编译期整数溢出」拒绝——常量折叠超域）恒入锁面，修复回归即 selftest 红
    种子们 = list(range(1, 13)) + [1019, 251015000020, 251017000004]
    样本们 = []
    审计 = []
    for seed in 种子们:
        g = Gen(seed)
        样本们.append((seed, g.program(语句数)))   # 语句数=CLI 默认（实录口径）
        审计.extend(g.审计危险)

    移位量们 = []
    for _seed, src in 样本们:
        移位量们.extend(int(t) for t in re.findall(r"(?:<<|>>)\s*(-?\d+)", src))
    签("样本数=%d（12 实录 seed+3 危险 seed）" % len(样本们), len(样本们) == 15)
    签("移位量全部 ∈ [0,32)（474·%d 个）" % len(移位量们),
       len(移位量们) > 0 and all(0 <= v < 32 for v in 移位量们))
    文本全集 = NL.join(src for _s, src in 样本们)
    签("生成面未退化（移位/算术/除模均有产出）",
       bool(re.search(r"<<|>>", 文本全集))
       and bool(re.search(r" [\+\-\*] ", 文本全集))
       and bool(re.search(r" [/%] ", 文本全集)))
    # 847 折叠保护确定性锁面（1306：推断语句消耗随机流致 15 实录 seed 采样
    #   漂移·「随机面审计>=1」断言概率化失效——847 注释实录危险式直接单元化，
    #   同 seed 恒绿；随机面审计列表继续积累供编译对拍面双向对拍）
    g847 = Gen(847)
    t1_847, v1_847 = g847._算术折叠("*", "(-379 * -130)", "411",
                                    -379 * -130, 411)
    t2_847, v2_847 = g847._算术折叠("*", t1_847, "110", v1_847, 110)
    签("折叠保护修正确定面（847 实录式超域→右置 0+审计捕获）",
       v1_847 == 49270 * 411 and v2_847 == 0
       and len(g847.审计危险) == 1 and t2_847.endswith("* 0)"))
    审计.extend(g847.审计危险)   # 编译对拍面恒有实录超域式（非空锁）
    # 1306 扩面锁：推断声明三型（拼接/浮整混算/三元跨类）均有产出——
    #   形态域即 #1304/#1305 命中域，生成面回退=自动面重新致盲
    推断语句 = re.findall(r"变量 \S+ = [^\n]+", 文本全集)
    签("推断声明有产出（%d 条）" % len(推断语句),
       len(推断语句) >= 15, "样本集推断语句过少")
    签("推断声明三型覆盖（拼接/浮整混算/三元跨类）",
       bool(re.search(r'变量 \S+ = "[^"]*" \+', 文本全集))
       and bool(re.search(r"变量 \S+ = \S+ [\+\-\*] \d+\.\d+", 文本全集))
       and bool(re.search(r"变量 \S+ = \(\S+ > 0\) \? 1 : 2\.5", 文本全集)),
       "三型缺一")

    # 1022 四族扩面锁（#1398）：混合面 20 seed 覆盖四族+标量回退；族结构完整
    #   （删族=自动面重新致盲 → 本轮 #1393~#1397 永不再显形）
    混合样本 = [(seed, Gen(seed).program_mixed(语句数)) for seed in range(1, 21)]
    混合文本 = NL.join(src for _s, src in 混合样本)
    缺族 = [族 for 族 in 族名们 if ("// 族=" + 族) not in 混合文本]
    签("1022 四族生成面覆盖（%d/4）" % (len(族名们) - len(缺族)), not 缺族,
       "缺 %s" % 缺族)
    签("1022 混合面保留标量回退（seed%%5==0）",
       any("=== CN-Smith 采样 ===" in src and "// 族=" not in src
           for _s, src in 混合样本))
    族样本 = [(s, src) for s, src in 混合样本 if "// 族=" in src]
    签("1022 族程序结构完整（主函数唯一·%d 件）" % len(族样本),
       len(族样本) == 16 and all(src.count("函数 主()") == 1
                                 for _s, src in 族样本))
    # 1022 注入器候选行锁（#1398 随批）：族程序首行注释不得吞掉注入；
    #   截断点必须是代码行（原实现两缺陷=样本实际仍合法→宿主误接受假命中）
    注入坏 = []
    for seed in range(1, 5):
        基 = Gen(seed).program_mixed(语句数)
        基行们 = 基.split(NL)
        for k in range(8):
            注入, kind, 行号 = inject_error(基, random.Random(seed * 7 + k))
            if 注入 is None:
                注入坏.append((seed, kind, "跳过"))
                continue
            原行 = 基行们[行号 - 1] if 0 < 行号 <= len(基行们) else ""
            if kind == "非法字符" and 原行.strip().startswith("//"):
                注入坏.append((seed, kind, "注释行"))
            if kind == "截断文件" and not 原行.strip():
                注入坏.append((seed, kind, "空行"))
    签("1022 注入器候选行锁（注释不吞注入·截断必删代码行）", not 注入坏,
       str(注入坏[:5]))

    if 编译路径 is not None:
        目标 = 目标 or ("win-x64" if os.name == "nt" else "linux-x64")
        临时 = os.path.join("target", "cnsmith_selftest_tmp")
        os.makedirs(临时, exist_ok=True)
        try:
            过, 坏 = 0, []
            for seed, src in 样本们:
                源 = os.path.join(临时, "s%d.cn" % seed)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write(src)
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                if r.returncode == 0:
                    过 += 1
                else:
                    坏.append(seed)
            签("样本全部编译通过（%d/%d）" % (过, len(样本们)),
               过 == len(样本们), "被拒 seed=%s" % 坏)

            # 1022 四族面编译锁：seed 1~4 各命中一族（宿主对默认参数常量算式
            #   放行但值错——合法性契约只要求编译通过，值错由对拍轨道显形）
            族坏 = []
            for seed in range(1, 5):
                源 = os.path.join(临时, "f%d.cn" % seed)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write(Gen(seed).program_mixed(语句数))
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                if r.returncode != 0:
                    族坏.append(seed)
            签("1022 四族混合样本编译通过（%d/4）" % (4 - len(族坏)), not 族坏,
               "被拒 seed=%s" % 族坏)

            帽子 = list(dict.fromkeys(审计))   # 去重（多样本可能同式）
            拒 = 0
            for i, 表达式 in enumerate(帽子):
                源 = os.path.join(临时, "d%d.cn" % i)
                with open(源, "w", encoding="utf-8", newline="") as f:
                    f.write("函数 主() -> 整32 {\n    整32 探针值 = %s;\n"
                            "    打印(探针值);\n    返回 0;\n}\n" % 表达式)
                r = subprocess.run(
                    [编译路径, "build", 源, "--target", 目标, "-O0",
                     "--output", 源[:-3] + ".exe"],
                    capture_output=True, timeout=120)
                输出 = (r.stdout + r.stderr).decode("utf-8", "replace")
                if r.returncode != 0 and "编译期整数溢出" in 输出:
                    拒 += 1
            签("危险表达式全被编译器拒（%d/%d·双向对拍）" % (拒, len(帽子)),
               len(帽子) >= 1 and 拒 == len(帽子))
        finally:
            shutil.rmtree(临时, ignore_errors=True)

    for 名 in 通过:
        print("  [通过] %s" % 名)
    for 名, 详情 in 失败:
        print("  [失败] %s%s" % (名, ("—— " + 详情) if 详情 else ""))
    print("selftest: %d 通过 / %d 失败" % (len(通过), len(失败)))
    return 1 if 失败 else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=10)
    ap.add_argument("--out", default="target/cnsmith")
    ap.add_argument("--stmts", type=int, default=14)
    ap.add_argument("--族", choices=("混合", "标量"), default="混合",
                    help="程序面：混合=每5个seed覆盖标量+四族（1022扩面·默认）；"
                         "标量=原整32/整64面")
    ap.add_argument("--negative", action="store_true",
                    help="负向注入模式：生成非法程序（验证必有诊断不崩溃）")
    ap.add_argument("--selftest", action="store_true",
                    help="生成器自校验（移位域/折叠域/采样面·零编译依赖）")
    ap.add_argument("--编译", default=None,
                    help="selftest 编译对拍用 cn.exe 路径（可选·增强面）")
    ap.add_argument("--目标", default=None,
                    help="selftest 编译对拍目标平台（默认按本机探测）")
    a = ap.parse_args()
    if a.selftest:
        return selftest(a.编译, a.目标)
    os.makedirs(a.out, exist_ok=True)
    跳过 = 0
    分布 = {}
    for i in range(a.count):
        seed = a.seed + i
        g = Gen(seed)
        src = g.program(a.stmts) if a.族 == "标量" else g.program_mixed(a.stmts)
        if a.negative:
            rng = random.Random(seed * 31 + 7)
            src, kind, 行号 = inject_error(src, rng)
            if src is None:
                跳过 += 1          # 全类型不可注入（合法程序行集过小）——不产假注入样本
                continue
            分布[kind] = 分布.get(kind, 0) + 1
            with open(os.path.join(a.out, "s%d.cn.neg" % seed), "w",
                      encoding="utf-8", newline="") as f:
                f.write("%s\t%d\n" % (kind, 行号))
        with open(os.path.join(a.out, "s%d.cn" % seed), "w",
                  encoding="utf-8", newline="") as f:
            f.write(src)
    mode = "负向注入" if a.negative else "合法"
    print("生成 %d 个程序（%s） -> %s" % (a.count - 跳过, mode, a.out))
    if a.negative:
        print("  注入类型分布: %s ｜ 跳过（无可注入行）%d" %
              (dict(sorted(分布.items())), 跳过))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
