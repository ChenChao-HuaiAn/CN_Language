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
1740 扩面（#1735/#1737/#1739 锁面）：混合面扩为「标量+七族」——新增
「枚举默认初始化/方法句柄链/结构体拷贝」三族（2026-10-27 采样轮四缺陷全部
由手动探针挖出、自动轨道零覆盖的实证收口）。#1736 聚合比较负形态（预期双侧
一致拒）不入随机合法面，走对拍内置双侧拒绝锁（cnsmith_dual_samples）。
1772 扩面（#1770/#1771 锁面）：混合面再扩为「标量+十族」按 seed%11 轮转——
新增「嵌套构造/多维数组/指针出参」三族（2026-10-28 采样轮 4 项新发现全靠
手动探针挖出、自动轨道零覆盖收口；素材=主树 target/probe_1028 探针
a4/f1/j1/j2 精简改写）。修复未集成 develop 期间新三族对拍轨道预期红
（嵌套构造/指针出参=宿主绿 v2 红；多维数组=宿主 def-init 误拦在案复红）。

「只生成合法程序」契约（847 收口）由二层保护承担：
  ①移位量域保护：移位量=操作数类型位宽域内小字面量（474 立法）；
  ②常量折叠溢出保护：常量链折叠值镜像编译器预演域（_算术折叠/…）——
    超域节点主动修正为恒合法形态（同除数非零保护先例）。
自校验：python scripts/cnsmith_gen.py --selftest [--编译 target/cn.exe]
  （移位域+折叠域+采样生成面静态三断言；给 --编译 时加样本全编译通过
  + 危险表达式全被拒双向对拍。本体拆出至 cnsmith_gen_selftest.py——
  1772 拆出面·本文件触 900 警戒线后按「先拆再加」拆分）。

用法：python scripts/cnsmith_gen.py --seed 1 --count 10 --out target/cnsmith
"""
import argparse
import os
import random
import re

NL = "\n"

# 折叠预演域（847）：镜像编译器常量折叠/溢出预演口径
#   （semantic_expr_binary.cpp reportConstOverflow + model/semantic_helpers.hpp
#   cnFoldConstIntExpr——int64 承载闭包 + 结果类型域预演）
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
# 1022 扩面（#1398）+ 1740 扩面（#1735/#1737/#1739）：族结构程序生成
#   背景（2026-10-22 采样轮实证）：原随机面只产 主() 内整32/整64 标量语句，
#   枚举数组/重载后缀/默认参数表达式/转义字符串四族零覆盖——自动三轨道全绿
#   而 22 项红全靠手写探针。1022 按混合面轮转四族；1740 扩三族（枚举默认
#   初始化/方法句柄链/结构体拷贝——2026-10-27 采样轮四缺陷零自动覆盖收口）。
#   合法性契约=宿主编译通过；v2 侧分叉=对拍轨道命中面（#1393~#1397 与
#   #1735/#1737/#1739 锁面）。
#   ⚠ 修复前对拍/矩阵轨道对这些族预期红（红在明处，勿误判工具回归 revert）。
#   模板本体=cnsmith_gen_families.py（1109 体量红线「先拆再加」拆出面·
#   族名们/族程序 此处 re-export 保接口不变）。
# =====================================================================
from cnsmith_gen_families import 族名们, 族程序  # noqa: E402  （模板拆出面·re-export）


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

        折叠值=镜像编译器常量折叠器（cnFoldConstIntExpr）的可折叠量：
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
            # 位运算不进折叠闭包（cnFoldConstIntExpr default——不可折叠）
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
        """混合面（1022·#1398·1740/1772 扩）：seed%11==0 用原标量面，1~10 依次十族。

        连续 11 个 seed 覆盖「标量+枚举数组+重载后缀+默认参数+转义字符串+
        枚举默认初始化+方法句柄链+结构体拷贝+嵌套构造+多维数组+指针出参」；
        全部面均遵守「只生成合法程序」契约（合法性=宿主编译通过；多维数组族
        在 #1771 集成前为在案复红·诊断锚白名单见 cnsmith_gen_selftest）。"""
        序 = self.seed % 11
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
    #   修法：expr 递归携带折叠值（镜像 cnFoldConstIntExpr 闭包——
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


# ---------------------------------------------------------------------
# 1618 注入点字面量避让：轻量词法扫描（对齐宿主 lexer 词法面）
#   缺陷实录（2026-10-25 采样轮 s20261259）：「非法字符」把 @@ 追加到
#   `字符串 甲 = 多行"""第一行` 行尾——该行行尾处于未闭合三引号字面量
#   体内，@@ 属字符串自由内容 → 样本仍合法 → 宿主 rc=0 放行被误判
#   「负向防线失守」、v2 侧 rc=0 未产 asm 被误判「假绿」（无效负向样本）。
#   修法：注入前先做一次轻量词法扫描，标出字符串/字符字面量 token 内部
#   字符索引，五种注入的候选点一律剔除字面量内位置；某类型在字面量外
#   无候选则按既有机制换类型，全类型不可注入返回 None（调用方跳过样本）。
#   扫描器只服务「注入点避开字面量」判定：保守方向=宁可多剔除不误注入，
#   不复刻宿主诊断细节（与 neg_expect_audit.字符串们 的「内容提取」用途
#   不同——此处是「区间标记」，且须支持 原始/多行 前缀与跨行三引号）。
# ---------------------------------------------------------------------

# 字符串字面量前缀（plans/001 §2.1a·上下文识别非保留字：词后紧跟引号才成前缀）
_字符串前缀们 = ("原始", "多行", "原始多行", "多行原始")


def _是词字(ch):
    """CN 标识符字符判定（中文/字母/数字/下划线——扫描器切词用）。"""
    return ord(ch) > 127 or ch.isalnum() or ch == "_"


def _消费unicode转义(文本, i, 内):
    """消费 \\u{...} 结构（i 指向 'u' 且 文本[i+1]=='{'·调用方先行判定）。

    把 u{...} 全部标入 `内`，含收尾 }（宿主 readStringBody/readChar 的
    \\u 结构感知同构；缺 } 时消费到文本尾——宿主同款 while !isAtEnd）。"""
    while i < len(文本):
        内.add(i)
        ch = 文本[i]
        i += 1
        if ch == "}":
            break
    return i


def _扫转义(文本, i, 内):
    """消费非 raw 字符串态的反斜杠转义（i 指向 \\ 本身·三引号/普通/字符三态共用）。

    \\ 与转义体一并标入 `内`：\\u{...} 结构消费到 }（含）；其余两字符转义
    （含 \\+换行跨行·宿主 readStringBody 同款）。返回新索引。"""
    内.add(i)
    i += 1
    if 文本[i:i + 1] == "u" and 文本[i + 1:i + 2] == "{":
        return _消费unicode转义(文本, i, 内)
    if i < len(文本):
        内.add(i)
        return i + 1
    return i


def _扫代码态(文本, i, 内):
    """代码态扫描：注释跳过/前缀词识别/引号起始。返回 (新i, 新态, raw)。

    - 单行 // 与嵌套块注释 /* */ 整体跳过（宿主 skipWhitespaceAndComments
      同构），注释内引号不参与字面量状态；
    - 词∈前缀集（原始/多行/原始多行/多行原始）且词后紧跟引号才成前缀
      （上下文识别·变量名 原始值 不触字符串态）；前缀字符不入 `内`
      （注入点形态=行尾追加/删 ; ) } /整行截断，均落不到前缀中间）；
    - 多行前缀+单引号=宿主降级普通字符串；无前缀 " 一律普通字符串态。"""
    n = len(文本)
    c = 文本[i]
    if c == "/" and 文本[i + 1:i + 2] == "/":
        while i < n and 文本[i] != "\n":          # 单行注释整体跳过
            i += 1
        return i, "代码", False
    if c == "/" and 文本[i + 1:i + 2] == "*":
        i += 2                                    # 块注释（嵌套）
        深 = 1
        while i < n and 深 > 0:
            if 文本[i:i + 2] == "/*":
                深 += 1
                i += 2
            elif 文本[i:i + 2] == "*/":
                深 -= 1
                i += 2
            else:
                i += 1
        return i, "代码", False
    if _是词字(c):
        词起 = i
        while i < n and _是词字(文本[i]):
            i += 1
        词 = 文本[词起:i]
        if 词 in _字符串前缀们 and 文本[i:i + 1] == '"':
            raw = "原始" in 词
            if "多行" in 词 and 文本[i:i + 3] == '"""':
                内.update(range(i, i + 3))
                return i + 3, "三引号", raw
            内.add(i)
            return i + 1, "普通字符串", raw
        return i, "代码", False
    if c == '"':
        内.add(i)
        return i + 1, "普通字符串", False
    if c == "'":
        内.add(i)
        return i + 1, "字符", False
    return i + 1, "代码", False


def _字面量内索引集(文本):
    """轻量词法扫描：返回字符串/字符字面量 token 内部字符的一维索引集合。

    对齐宿主 src/cn_compiler/lexer/lexer.cpp 词法面（readStringBody/readChar）：
      - 前缀 原始/多行/原始多行/多行原始 后紧跟引号才成前缀（词∉前缀集或
        词后非引号=普通标识符，如变量名 原始值 不触字符串态）；
      - 三引号 \"\"\"...\"\"\" 可跨行；raw（前缀含 原始）不处理转义（多行原义），
        非 raw 多行 \\ 转义消费两字符（\\u{...} 消费到 }）——对齐宿主
        「转义优先于 \"\"\" 终止判定」的检查顺序；
      - 普通 "..." 不跨行（遇裸换行 token 终止=宿主报「不能跨行」），
        \\ 转义（含 \\+换行）继续；
      - 字符 '...' 至多一个转义/普通字符即期待闭引号（宿主 readChar 同构）；
      - 单行注释 // 与嵌套块注释 /* */ 整体跳过，不参与字面量状态。
    跨行字面量把途经换行符一并标内：行尾索引∈集合 ⟺ 行尾处于未闭合
    跨行字面量中间（正是 s20261259 缺陷判定式）；文件尾未闭合时补标
    len(文本) 哨兵使末行行尾判定同式。"""
    内 = set()
    n = len(文本)
    i = 0
    态 = "代码"        # 代码 / 三引号 / 普通字符串 / 字符
    raw = False
    已有体 = False     # 字符字面量体已消费（'X' 的 X·宿主一体即闭语义）
    while i < n:
        c = 文本[i]
        if 态 == "代码":
            i, 态, raw = _扫代码态(文本, i, 内)
            已有体 = False
            continue
        if 态 == "三引号":
            if 文本[i:i + 3] == '"""':
                内.update(range(i, i + 3))
                i += 3
                态 = "代码"
                continue
            if c == "\\" and not raw:
                i = _扫转义(文本, i, 内)
                continue
            内.add(i)                             # 普通字符含换行——跨行
            i += 1
            continue
        if 态 == "普通字符串":
            if c == '"':
                内.add(i)
                i += 1
                态 = "代码"
                continue
            if c == "\n":
                态 = "代码"                       # 裸换行=宿主「不能跨行」终止
                continue
            if c == "\\" and not raw:
                i = _扫转义(文本, i, 内)
                continue
            内.add(i)
            i += 1
            continue
        # 态 == "字符"（宿主 readChar：'X'/'\\X'/'\\u{...}'，一体后即闭引号）
        if c == "'":
            内.add(i)
            i += 1
            态 = "代码"
            continue
        if 已有体:
            态 = "代码"       # 宿主：闭引号不匹配=报「未闭合」token 终止
            continue          # 当前字符不入字面量，回代码态重新处理
        if c == "\\":
            i = _扫转义(文本, i, 内)          # 字符态转义体（\\u 结构/单字符）
        else:
            内.add(i)
            i += 1
        已有体 = True
    if 态 != "代码":
        内.add(n)             # 未闭合字面量延伸到文件尾（末行行尾判定哨兵）
    return 内


def _行内字面量外位置(行, 行基, 内, 目标, 从后往前=False):
    """行内目标字符的字面量外位置；无则 -1（-1=「未找到」哨兵，调用方分流）。

    从后往前=False 取首个（删分号——对齐既有 replace(…,1) 语义）、
    True 取末个（缺右括号——对齐既有 rfind 语义），仅限定字面量外。"""
    列 = 行.rfind(目标) if 从后往前 else 行.find(目标)
    while 列 >= 0:
        if 行基 + 列 not in 内:
            return 列
        列 = 行.rfind(目标, 0, 列) if 从后往前 else 行.find(目标, 列 + 1)
    return -1


def inject_error(src, rng):
    """注入一个随机语法错误。返回 (源码|None, 注入类型|None, 注入行号 1 基|0)。

    必真注入保证：只在含目标特征的候选行中随机选（该行注入后必然语法非法）；
    候选为空则换下一个注入类型（随机序）；全部类型不可注入返回 None（调用方跳过）。
    1618：候选点一律剔除字符串/字符字面量内部位置（先轻量词法扫描一次，
    五类型共享区间集）——字面量内容自由，注入其中=样本仍合法（假注入）。"""
    kinds = INJECTIONS[:]
    rng.shuffle(kinds)
    lines = src.split(NL)
    内 = _字面量内索引集(src)
    行基 = [0]
    for l in lines:
        行基.append(行基[-1] + len(l) + 1)   # +1=行尾换行符（行基[i]..行基[i+1]-1）
    for kind in kinds:
        out = _尝试注入(lines, kind, rng, 内, 行基)
        if out is not None:
            return NL.join(out[0]), kind, out[1]
    return None, None, 0


def _尝试注入(lines, kind, rng, 内, 行基):
    """单类型注入尝试：返回 (新行列表, 注入行号) 或 None（无可注入行）。

    1022（#1398 随批根治）：候选行一律排除注释行（`//`）与预处理行（`#`）——
    原实现只按「非空」选行，族程序首行 `// 族=…` 使「非法字符」注入追加进
    注释（样本仍合法 → 宿主误接受假命中）；「截断文件」也可能只删末尾空行。
    1618：候选点一律再剔除字符串/字符字面量内部位置（`内` 索引集·含跨行
    三引号体与行尾未闭合判定）——s20261259 实录：@@ 追加进多行字符串体
    =样本仍合法。注入契约=必真破坏语法：候选必须落在代码行+代码位。"""
    out = list(lines)
    代码行 = [i for i, l in enumerate(out)
              if l.strip() and not l.strip().startswith("//")
              and not l.strip().startswith("#")]
    if kind == "删分号":
        cand = [(i, _行内字面量外位置(out[i], 行基[i], 内, ";"))
                for i in 代码行]
        cand = [(i, j) for i, j in cand if j >= 0]
        if not cand:
            return None
        i, j = rng.choice(cand)
        out[i] = out[i][:j] + out[i][j + 1:]
    elif kind == "缺右括号":
        cand = [(i, _行内字面量外位置(out[i], 行基[i], 内, ")", 从后往前=True))
                for i in 代码行]
        cand = [(i, j) for i, j in cand if j >= 0]
        if not cand:
            return None
        i, j = rng.choice(cand)
        out[i] = out[i][:j] + out[i][j + 1:]
    elif kind == "缺右花括号":
        cand = [i for i, l in enumerate(out)
                if l.strip() == "}"
                and (行基[i] + l.index("}")) not in 内]
        if not cand:
            return None
        i = rng.choice(cand)
        out.pop(i)
    elif kind == "非法字符":
        cand = [i for i in 代码行 if (行基[i] + len(out[i])) not in 内]
        if not cand:
            return None
        i = rng.choice(cand)
        out[i] = out[i] + " @@"
    elif kind == "截断文件":
        # 截断点=后半段代码行（排除注释/空行——只删空串不算截断；
        # 1618：行首在跨行字面量内的行=字符串体中行，截断它只会截开
        # 字面量本身（未闭合报错），非「结构突然中断」，剔除）
        cand = [i for i in 代码行 if i >= len(out) // 2 and 行基[i] not in 内]
        if not cand:
            return None
        i = rng.choice(cand)
        out = out[:i]
    else:
        return None
    return out, i + 1


# =====================================================================
# 自校验（847 收口·判定契约=全绿 0/红 1）已拆出至 cnsmith_gen_selftest.py
#   （1772 拆出面·本文件触 900 警戒线按「先拆再加」拆分；main 延迟导入
#   防循环——拆出模块反向 import 本模块的 Gen/inject_error）。
# =====================================================================


def selftest(编译路径=None, 目标=None):
    """生成器自校验（实现在 cnsmith_gen_selftest.selftest·接口保位）。"""
    import cnsmith_gen_selftest as 自测   # 延迟导入（防循环·对齐 dual 先例）
    return 自测.selftest(编译路径, 目标)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--count", type=int, default=10)
    ap.add_argument("--out", default="target/cnsmith")
    ap.add_argument("--stmts", type=int, default=14)
    ap.add_argument("--族", choices=("混合", "标量"), default="混合",
                    help="程序面：混合=每11个seed覆盖标量+十族（1022四族+1740三族"
                         "+1772三族·默认）；标量=原整32/整64面")
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
