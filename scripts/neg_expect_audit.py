#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""neg_expect_audit.py —— 负测期望反向对账（841）：题库→法条方向清点孤儿负例。

方向（728n 单向对账的反向）：728n check_diag_negtest.py 问「每条诊断文案有没有负测锚」；
本工具问反向问题——**每例负测断言的拒绝行为，在 plans/001 法条与编译器诊断文本里有没有
对应着落**（考卷答案本身错的风险面：期望的拒绝在 001 无规范依据/诊断源无实现痕迹）。

负例枚举（与 run_e2e 负测通道同源·5 通道）：
  期望编译失败.txt（整文=宿主 build 拒绝锚）/ 期望check失败.txt（跳过首行被检文件名）/
  双编译对照.txt·v2闭环.txt 的「诊断锚=」键（宿主侧负编译锚）与负路径 .expected 行
  （v2 侧诊断锚行——由 v2p 实跑输出固化）。判定为负例=目录含 期望编译失败/期望check失败，
  或配置含 预期编译失败=是 / 退出码=负。
锚提取：剥「[p6h]/[sem5]」类 ASCII 通道前缀与「正确诊断:」标签（纯中文方括号标记如
  [安全区边界] = 诊断正文标记，保留）；整行管道（语义错误 N 个…/已解析…等进度与
  汇总行）无对账意义（忽略计数）；装饰变体（[错误] 标记 / （行=N） 注记）剥出正文核；
  <2 字符锚无对账意义（忽略计数）。

两面佐证（文本层·每锚取最佳）：
  ① 法条面 plans/001 全文 + 章节（章节 ID 与 check_spec_coverage/覆盖矩阵同规则，
     便于「该例挂靠单元 ↔ 001 该节」逐条对表）；
  ② 诊断面 编译器源字符串语料（宿主 src/**/*.cpp + v2 CN语言编译器v2/**/*.cn 的字符串
     字面量·字符串感知状态机·注释内文本不算）。

档位（逐例取最佳锚·公共子串口径 3 字符下界）：
  强·法条直陈  某锚核心（≥6 字符）为 001 连续文本子串 —— 规范直陈该表述
  强·诊断实证  某锚核心与某诊断源字符串 LCS ≥6 —— 编译器真实发射该文案（法条无直陈）
  中·词根档    001 或诊断面 LCS 3~5 —— 同词根（列出匹配串供人判）
  短锚·机械不可判 全部锚核心 <4 字符（如「重载」「注释」）—— 锚过泛·机器无法对表
  无锚（仅 rc≠0 断言）负例声明了拒绝但零诊断文本（诊断锚留空/标记无内容）——文面无
             对账基准（最弱断言形态·541 为两侧文案不同构的显式留空）
  无有效锚     全部锚 <2 字符（如「?」）—— 无对账意义
  孤儿负例     有 ≥4 字符锚，但两面均无 ≥3 字符公共串 —— **期望的拒绝在 001 与诊断源
             均无着落**（考卷答案本身错/规范缺口双风险面·本工具头号产出）

零锚负例照收不静默（miss 不可静默同规）：收集面=负例判据目录全量（非「有锚才收」），
  否则该类缺陷从对账面静默消失。

输出：孤儿负例清单（头号）/ 法条无着落清单（001 无 ≥3 字符公共串·诊断实证在位——两义：
  规范表述差 or 规范缺口，列出实证串与挂靠单元供人判）/ 短锚清单 / 分类统计。
覆盖矩阵挂靠单元（/api/coverage·check_spec_coverage 同源）随「法条无着落」面列出，
  并给出该单元 001 节文本与锚的最大公共串（单元对表=有词根 / 单元无文本关联）。

诚实边界（不假装全覆盖·728n 同规）：
  ① 文本层对账 ≠ 语义证明：「法条直陈」只证 001 出现同串文本；001 表述与诊断文案不同的
     合规拒绝由「诊断实证/词根档」兜（列出匹配串），不机械判死。
  ② 诊断实证只证「编译器有意发射该文案」，文案是否与规范一致仍须人读 001。
  ③ 公共子串下界 3 字符（3-gram 预筛保证精确性）；2 字符公共串（如「类型」）过泛不判。
  ④ 孤儿负例=机械筛选面（低误报取向）：处置=人读 001 补法条落点、或修期望文案；确属
     实现级拒绝（语言面无规范义务·如内部错误码探针）由人裁决留痕，本工具不设内置豁免。

用法：
  python3 scripts/neg_expect_audit.py             # 全库对账（报告式·rc=0）
  python3 scripts/neg_expect_audit.py --strict    # 孤儿负例>0 → rc=1（周检挂载点；
                                                  #   973 起已周期化挂载=ci_daemon 兜底轮周期段〔ci_reconcile.py〕）
  python3 scripts/neg_expect_audit.py --json      # 机读报告
  python3 scripts/neg_expect_audit.py --limit 50  # 清单截断（默认 20·0=全列）
  python3 scripts/neg_expect_audit.py --selftest  # 自测（临时样例树正反两态·零触碰真实树）

退出码：0=报告完成（--strict 且孤儿>0 → 1）；2=用法错误。
"""
from __future__ import annotations

import argparse
import io
import json
import re
import sys
import tempfile
from pathlib import Path

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
E2E = 仓库根 / "tests" / "e2e"
规范路径 = 仓库根 / "plans" / "001 CN语言编译器设计规格书.md"
sys.path.insert(0, str(脚本目录))  # check_spec_coverage 复用（覆盖矩阵拉取/解析）

# 诊断源扫描面（新增扫描树在此登记，否则诊断面有盲区）
扫描树 = (("src", (".cpp", ".hpp")), ("CN语言编译器v2", (".cn",)))

# ============ 锚提取 ============

# 通道前缀：含 ASCII 字母/数字的方括号令牌（[p6h]/[sem5]/[ir2]）——纯中文方括号
# （[安全区边界]/[错误]）是诊断正文标记，必须保留（否则锚被吞成空串·自测 265 案）。
通道前缀 = re.compile(r"^(\[(?=[^\]]*[A-Za-z0-9])[^\]\n]{1,12}\]\s*)+")
标签 = re.compile(r"^(正确诊断|错误诊断|警告|已编译|已解析)[:：]\s*")
管道 = re.compile(r"^(语义错误\s*\d+\s*个|已解析[:：]|词法分析|语法分析|IR生成|代码生成)")
负路径键 = ("预期编译失败=是", "预期编译失败=true", "预期编译失败=True",
        "预期编译失败=1", "退出码=负")


def 有效行(路径: Path) -> list:
    """去空行/# 注释行（run_e2e 用例级控制文件同口径）。"""
    try:
        文本 = 路径.read_text(encoding="utf-8-sig")
    except OSError:
        return []
    return [行.strip() for 行 in 文本.splitlines()
            if 行.strip() and not 行.strip().startswith("#")]


def 核心(原文: str) -> str:
    """锚核心：剥通道前缀/标签/首尾标点；整行管道（进度/汇总行）=无对账意义 → ""。

    首尾只剥标点不剥括号（括号装饰由 变体们 处理——此处剥右括号会致括号不配对）。"""
    s = 通道前缀.sub("", 原文.strip())
    s = 标签.sub("", s)
    if 管道.match(s):
        return ""
    return s.strip().strip(" \t，,。.：:；;、")


def 变体们(核心文本: str) -> list:
    """锚核心的判据变体：[核心, 正文核]。

    正文核=再剥纯中文方括号标记（[错误]/[安全区边界]——诊断正文标记非通道名）与
    尾部注记（（行=N）/（退出码 1）类）。实锚常带这两类装饰（728n/801 固化形态：
    `[sem5] 正确诊断: [错误] <正文>（行=N）`），只按核心判会把正文装饰掉不进 001
    直陈面——变体面即为此设（两面皆取变体中最优）。"""
    出 = [核心文本]
    s = re.sub(r"^(\[[^\]\n]{1,12}\]\s*)+", "", 核心文本)
    s = re.sub(r"[（(][^）)]*$", "", s)          # 尾部未闭合括号（人工书写的期望行可能不配对）
    s = re.sub(r"[（(][^）)]{0,30}[）)]\s*$", "", s)
    s = s.strip().strip(" \t，,。.：:；;、")
    if s and s != 核心文本 and len(s) >= 2:
        出.append(s)
    return 出


def 锚提取(用例目录: Path) -> list:
    """负例锚提取（5 通道同源）；返回 [{"来源","原文","核心"}]。
    非负例目录返回 []。"""
    出 = []
    f = 用例目录 / "期望编译失败.txt"
    if f.exists():
        出 += [{"来源": "期望编译失败.txt", "原文": x} for x in 有效行(f)]
    f = 用例目录 / "期望check失败.txt"
    if f.exists():
        行们 = 有效行(f)
        出 += [{"来源": "期望check失败.txt", "原文": x} for x in 行们[1:]]
    for 配置名 in ("双编译对照.txt", "v2闭环.txt"):
        配置 = 用例目录 / 配置名
        if not 配置.exists():
            continue
        负 = False
        try:
            行们 = 配置.read_text(encoding="utf-8-sig").splitlines()
        except OSError:
            行们 = []
        for 行 in 行们:
            s = 行.split("#", 1)[0].strip()
            if s.startswith("诊断锚="):
                v = s[len("诊断锚="):].strip()
                if v:
                    出.append({"来源": 配置名 + ":诊断锚", "原文": v})
            elif s in 负路径键:
                负 = True
        if 负:
            for ff in sorted(用例目录.glob("*.expected")):
                for x in 有效行(ff):
                    出.append({"来源": ff.name, "原文": x})
    for 锚 in 出:
        锚["核心"] = 核心(锚["原文"])
    return 出


def 是负例(用例目录: Path) -> bool:
    """负例判据（标记文件 或 配置负路径键）——与 run_e2e 负测通道同源。"""
    if (用例目录 / "期望编译失败.txt").exists() or (用例目录 / "期望check失败.txt").exists():
        return True
    for 配置名 in ("双编译对照.txt", "v2闭环.txt"):
        配置 = 用例目录 / 配置名
        if not 配置.exists():
            continue
        try:
            行们 = 配置.read_text(encoding="utf-8-sig").splitlines()
        except OSError:
            continue
        for 行 in 行们:
            if 行.split("#", 1)[0].strip() in 负路径键:
                return True
    return False


def 收集负例(根: Path) -> list:
    """tests/e2e 下全部负例用例 → [(目录名, 锚们)]。

    零锚负例（仅 rc≠0 断言·无诊断文本）**照收**——静默丢弃=对账面盲区
    （miss 不可静默）；其分类档=「无锚（仅 rc≠0 断言）」。"""
    出 = []
    if not 根.exists():
        return 出
    for d in sorted(根.iterdir()):
        if not d.is_dir() or d.name == "__pycache__":
            continue
        if not 是负例(d):
            continue
        出.append((d.name, 锚提取(d)))
    return 出


# ============ 001 法条面 ============

_编号正则 = re.compile(r"^(\d+(?:\.\d+)*[A-Za-z]?)\s+(.*)$")


def 读001(路径: Path) -> dict:
    """解析 001 → {"全文", "章节": {uid: {"标题","行","文本"}}, "章节序"}。

    章节 ID 规则与 check_spec_coverage.parse_units 同规（三级标题自带编号或标题名；
    四级+ 自带编号或 parent#标题）——覆盖矩阵单元 ID 可直接取本节文本对表。
    节文本=该标题行起至下一个「层级 ≤ 本节」标题止（含子孙节·locus 对表口径）。"""
    行们 = 路径.read_text(encoding="utf-8").splitlines()
    标题们 = []  # (行号, level, 标题文本, uid)
    parent = None
    for i, 行 in enumerate(行们):
        m = re.match(r"^(#{2,5})\s+(.*?)\s*$", 行)
        if not m:
            continue
        level = len(m.group(1))
        text = m.group(2).replace("**", "").strip()
        nm = _编号正则.match(text)
        if level <= 2:
            parent = None
            uid = nm.group(1) if (level == 2 and nm) else text
        elif level == 3:
            uid = nm.group(1) if nm else (text.split("：")[0] if text.startswith("附录") else text)
            parent = uid
        else:
            uid = nm.group(1) if nm else "%s#%s" % (parent, text)
        标题们.append((i, level, text, uid))
    章节 = {}
    for idx, (行号, level, 标题, uid) in enumerate(标题们):
        止 = len(行们)
        for j in range(idx + 1, len(标题们)):
            if 标题们[j][1] <= level:
                止 = 标题们[j][0]
                break
        章节[uid] = {"标题": 标题, "行": 行号 + 1, "文本": "\n".join(行们[行号:止])}
    return {"全文": "\n".join(行们),
            "章节": 章节, "章节序": [x[3] for x in 标题们]}


# ============ 诊断源语料（字符串感知） ============

def 字符串们(文本: str) -> list:
    """提取双引号字符串字面量内容（注释/字符字面量跳过·转义还原 \\" 与 \\\\）。"""
    出 = []
    i = 0
    n = len(文本)
    while i < n:
        c = 文本[i]
        if c == '"':
            j = i + 1
            buf = []
            while j < n:
                ch = 文本[j]
                if ch == "\\" and j + 1 < n:
                    nxt = 文本[j + 1]
                    buf.append(nxt if nxt in ('"', "\\") else ch)
                    if nxt not in ('"', "\\"):
                        buf.append(nxt)
                    j += 2
                    continue
                if ch == '"':
                    break
                buf.append(ch)
                j += 1
            出.append("".join(buf))
            i = j + 1
        elif c == "'":                 # 字符字面量（短窗·防数字分隔符 1'000 误吞）
            j = i + 1
            while j < min(n, i + 6):
                if 文本[j] == "\\":
                    j += 2
                    continue
                if 文本[j] == "'":
                    j += 1
                    break
                j += 1
            i = j
        elif c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            i = n if j < 0 else j
        elif c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            i = n if j < 0 else j + 2
        else:
            i += 1
    return 出


def 收集诊断语料(根: Path) -> list:
    """诊断源语料 [(相对路径, 串)]（≥4 字符·去重保首见）。"""
    见 = set()
    出 = []
    for 目录, 后缀们 in 扫描树:
        base = 根 / 目录
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if not p.is_file() or p.suffix not in 后缀们:
                continue
            try:
                文本 = p.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            相对 = p.relative_to(根).as_posix()
            for s in 字符串们(文本):
                if len(s) < 4 or s in 见:
                    continue
                见.add(s)
                出.append((相对, s))
    return 出


# ============ 公共子串（3-gram 倒排预筛 + 双侧延展精确） ============

def 建gram表(s: str, k: int = 3) -> dict:
    表 = {}
    for i in range(len(s) - k + 1):
        表.setdefault(s[i:i + k], []).append(i)
    return 表


def 最长公共子串(a: str, b: str, b表=None, 下界: int = 3) -> tuple:
    """a×b 最长公共子串 → (长度, 串)。3-gram 预筛保证 ≥下界 的结果精确
    （任何 ≥3 的公共子串必含一个 3-gram 匹配）；<下界 一律返回 (0, "")。"""
    if len(a) < 下界 or len(b) < 下界:
        return 0, ""
    表 = b表 if b表 is not None else 建gram表(b)
    最优 = 0
    串 = ""
    n = len(a)
    nb = len(b)
    for i in range(n - 2):
        for j in 表.get(a[i:i + 3], []):
            k = 3
            while i + k < n and j + k < nb and a[i + k] == b[j + k]:
                k += 1
            m = 0
            while i - m - 1 >= 0 and j - m - 1 >= 0 and a[i - m - 1] == b[j - m - 1]:
                m += 1
            if k + m > 最优:
                最优 = k + m
                串 = a[i - m:i + k]
    return (最优, 串) if 最优 >= 下界 else (0, "")


# ============ 面构建与逐例对账 ============

def 见gram集合(s: str, k: int = 3) -> set:
    return {s[i:i + k] for i in range(len(s) - k + 1)}


def 公共串(短串: str, 长文本: str, 长gram集: set, 下界: int = 3) -> str:
    """短串 × 长文本 的公共子串抽样（gram 集判存在 + find 定位 + 双侧延展取串）。

    与 最长公共子串 的分工：本函数=「有无 ≥下界 公共串 + 一个实例」（O(len(短串))·
    大文本 locus 面对账用——全文级 LCS 在万字级章节上代价过高）；返回 ""=无。
    find 命中的出现位未必是最大延展位（抽样口径·locus 面只需存在性+样本串）。"""
    if len(短串) < 下界 or len(长文本) < 下界:
        return ""
    for i in range(len(短串) - 下界 + 1):
        g = 短串[i:i + 下界]
        if g not in 长gram集:
            continue
        j = 长文本.find(g)
        if j < 0:
            continue
        k = 下界
        while i + k < len(短串) and j + k < len(长文本) and 短串[i + k] == 长文本[j + k]:
            k += 1
        m = 0
        while i - m - 1 >= 0 and j - m - 1 >= 0 and 短串[i - m - 1] == 长文本[j - m - 1]:
            m += 1
        return 短串[i - m:i + k]
    return ""


def 建面(规范文件: Path, 源根: Path) -> dict:
    """佐证面：001 全文/章节 + 诊断语料倒排（供对账一例消费·自测注入临时面）。"""
    规范 = 读001(规范文件)
    语料 = 收集诊断语料(源根)
    倒排 = {}
    位置表 = {}
    for idx, (_, s) in enumerate(语料):
        t = 建gram表(s)
        位置表[idx] = t
        for g in set(t):
            倒排.setdefault(g, []).append(idx)
    章节gram集 = {uid: 见gram集合(节["文本"]) for uid, 节 in 规范["章节"].items()}
    规范表 = 建gram表(规范["全文"])
    return {"规范": 规范, "规范表": 规范表, "章节gram集": 章节gram集,
            "语料": 语料, "倒排": 倒排, "位置表": 位置表}


def 法条面(面: dict, 核心: str) -> dict:
    """锚 × 001：直陈（≥6 字符核心为全文子串）/ 公共串 + 含匹配串的最小章节。

    节定位=含匹配串的**最小**章节（嵌套章节中取最具体者·父子节均含串时取子）——
    供报告直接指路「该拒绝的 001 落点」。"""
    全文 = 面["规范"]["全文"]
    直陈 = len(核心) >= 6 and 核心 in 全文
    n, s = 最长公共子串(核心, 全文, 面["规范表"], 下界=3)
    节 = ""
    if s:
        候选 = [uid for uid in 面["规范"]["章节序"]
              if s in 面["规范"]["章节"][uid]["文本"]]
        if 候选:
            节 = min(候选, key=lambda u: len(面["规范"]["章节"][u]["文本"]))
    return {"直陈": 直陈, "长": n, "串": s, "节": 节}


def 诊断面(面: dict, 核心: str) -> dict:
    """锚 × 诊断语料：最大公共子串 + 命中文件；短锚（<4）用子串命中。"""
    if len(核心) < 2:
        return {"长": 0, "串": "", "文件": ""}
    if len(核心) < 4:  # 短锚无 3-gram 对账面：子串直扫（弱档候选）
        for 文件, s in 面["语料"]:
            if 核心 in s:
                return {"长": len(核心), "串": 核心, "文件": 文件}
        return {"长": 0, "串": "", "文件": ""}
    候选 = set()
    for i in range(len(核心) - 2):
        for idx in 面["倒排"].get(核心[i:i + 3], []):
            候选.add(idx)
    最佳 = {"长": 0, "串": "", "文件": ""}
    for idx in 候选:
        文件, s = 面["语料"][idx]
        n, ms = 最长公共子串(核心, s, 面["位置表"][idx], 下界=3)
        if n > 最佳["长"]:
            最佳 = {"长": n, "串": ms, "文件": 文件}
    return 最佳


def 对账一例(面: dict, 锚们: list) -> dict:
    """单例对账：逐锚（含变体）取最佳两面佐证 → 分类（档位规则见文件头）。"""
    if not 锚们:
        return {"法条": {"直陈": False, "长": 0, "串": "", "节": ""},
                "诊断": {"长": 0, "串": "", "文件": ""},
                "档": "无锚（仅 rc≠0 断言）", "有效锚": 0, "锚例": []}
    法 = {"直陈": False, "长": 0, "串": "", "节": ""}
    诊 = {"长": 0, "串": "", "文件": ""}
    有长锚 = False
    有效锚 = 0
    for 锚 in 锚们:
        c = 锚["核心"]
        if len(c) < 2:
            continue
        有效锚 += 1
        if len(c) >= 4:
            有长锚 = True
        for 变体 in 变体们(c):
            f = 法条面(面, 变体)
            if (f["直陈"] and not 法["直陈"]) or f["长"] > 法["长"]:
                法 = {"直陈": 法["直陈"] or f["直陈"], "长": f["长"],
                     "串": f["串"], "节": f["节"]}
            d = 诊断面(面, 变体)
            if d["长"] > 诊["长"]:
                诊 = d
    if 法["直陈"]:
        档 = "强·法条直陈"
    elif 诊["长"] >= 6:
        档 = "强·诊断实证"
    elif 法["长"] >= 3 or 诊["长"] >= 3:
        档 = "中·词根档"
    elif 有长锚:
        档 = "孤儿负例"
    elif 有效锚:
        档 = "短锚·机械不可判"
    else:
        档 = "无有效锚"
    锚例 = []
    for x in 锚们:
        if len(x["核心"]) >= 2 and x["核心"] not in 锚例:
            锚例.append(x["核心"])
    return {"法条": 法, "诊断": 诊, "档": 档, "有效锚": 有效锚,
            "锚例": 锚例[:6]}


# ============ 覆盖矩阵挂靠单元（locus 对表面积） ============

def 取挂靠单元() -> dict:
    """覆盖矩阵 → {用例名: [(单元ID, 列名)]}（服务不可达降级空表·报告注黄）。"""
    try:
        import check_spec_coverage
        行们, 豁免 = check_spec_coverage.parse_map()
    except (SystemExit, OSError, ImportError):
        return None
    表 = {}
    for 单元, 列们 in 行们.items():
        for 列, 名们 in 列们.items():
            for 名 in 名们:
                表.setdefault(名, []).append((单元, 列))
    for 名 in 豁免:
        表.setdefault(名, []).append(("(豁免)", "豁免"))
    return 表


def 全库对账(根: Path = None) -> dict:
    根 = 根 or E2E
    面 = 建面(规范路径, 仓库根)
    单元表 = 取挂靠单元()
    例们 = []
    for 名, 锚们 in 收集负例(根):
        判 = 对账一例(面, 锚们)
        判["用例"] = 名
        判["挂靠"] = 单元表.get(名, []) if 单元表 is not None else []
        判["挂靠节串"] = []
        for uid, _ in 判["挂靠"][:4]:
            if uid == "(豁免)":
                判["挂靠节串"].append((uid, None))     # 豁免行无 001 单元面
                continue
            节 = 面["规范"]["章节"].get(uid)
            if 节 is None:                             # 矩阵单元 ID 不在 001（另属 check_spec_coverage 面）
                判["挂靠节串"].append((uid, None))
                continue
            串 = ""
            for 锚 in 锚们:
                for 变体 in 变体们(锚["核心"]):
                    s = 公共串(变体, 节["文本"], 面["章节gram集"][uid], 下界=3)
                    if len(s) > len(串):
                        串 = s
            判["挂靠节串"].append((uid, 串 or ""))
        例们.append(判)
    return {"例们": 例们, "面": 面, "挂靠可用": 单元表 is not None}


# ============ 报告 ============

def 打印报告(报告: dict, 截断: int) -> None:
    例们 = 报告["例们"]
    分组 = {}
    for 判 in 例们:
        分组.setdefault(判["档"], []).append(判)

    def 列(项们, 打印一行):
        for 判 in (项们 if 截断 <= 0 else 项们[:截断]):
            打印一行(判)
        if 截断 > 0 and len(项们) > 截断:
            print("  …（另 %d 例略·--limit 0 全列）" % (len(项们) - 截断))

    print("=== 负测期望反向对账（841）：题库 → 001 法条/诊断源 ===")
    print("负例总数：%d 例（5 通道枚举：期望编译失败/期望check失败/双编译对照/ v2闭环"
          "的负路径与诊断锚）" % len(例们))
    print("分类：" + " ｜ ".join("%s %d" % (k, len(v)) for k, v in sorted(
        分组.items(), key=lambda kv: -len(kv[1]))))
    print("  （基本面义：法条=001 文本佐证·诊断=编译器源字符串实证——诊断实证只证"
          "「确定写过该文案」，不证其与规范一致；判读以两面串样本为准）")
    print("挂靠单元（覆盖矩阵）：%s" % ("可用" if 报告["挂靠可用"] else "不可达（降级）"))
    print()
    孤儿 = 分组.get("孤儿负例", [])
    print("—— 孤儿负例（001 与诊断源均无 ≥3 字符公共串·考卷答案错风险）：%d 例 ——"
          % len(孤儿))
    列(孤儿, lambda 判: print("  [%s] 锚：%s" % (判["用例"], " ｜ ".join(判["锚例"][:4]))))
    if not 孤儿:
        print("  [✓] 无孤儿负例（全部负例至少一面有 ≥3 字符公共串佐证）")
    print()
    无着落 = [判 for 判 in 例们
              if 判["档"] != "强·法条直陈" and 判["法条"]["长"] < 3
              and 判["档"] not in ("短锚·机械不可判", "无有效锚", "孤儿负例",
                               "无锚（仅 rc≠0 断言）")]

    def 挂靠文(判):
        if not 判["挂靠"]:
            return "挂靠：未登记"
        段 = []
        for (uid, 列), (_, 串) in zip(判["挂靠"][:3], 判["挂靠节串"][:3]):
            if 串 is None:
                段.append("%s(%s·无单元面)" % (uid, 列))
            elif 串:
                段.append("%s(%s·节串%r)" % (uid, 列, 串))
            else:
                段.append("%s(%s·节内无词根)" % (uid, 列))
        return "挂靠：" + "、".join(段)

    print("—— 法条无着落（001 无 ≥3 字符公共串·诊断实证在位·两义：表述差/规范缺口）："
          "%d 例 ——" % len(无着落))
    列(无着落, lambda 判: print(
        "  [%s] 诊断实证=%r ｜ 锚：%s ｜ %s" % (
            判["用例"], 判["诊断"]["串"], " ｜ ".join(判["锚例"][:3]), 挂靠文(判))))
    if not 无着落:
        print("  [✓] 无（全部非孤儿负例在 001 均有 ≥3 字符公共串落点）")
    print()
    短锚 = 分组.get("短锚·机械不可判", []) + 分组.get("无有效锚", [])
    print("—— 短锚·机械不可判（锚 <4 字符过泛·须人读）：%d 例 ——" % len(短锚))
    列(短锚, lambda 判: print("  [%s] 锚：%s" % (判["用例"], "、".join(判["锚例"][:3]) or "(无)")))
    if not 短锚:
        print("  [✓] 无")
    print()
    无锚 = 分组.get("无锚（仅 rc≠0 断言）", [])
    print("—— 无锚负例（仅 rc≠0 断言·无诊断文本=文面无对账基准）：%d 例 ——" % len(无锚))
    列(无锚, lambda 判: print("  [%s]" % 判["用例"]))
    if not 无锚:
        print("  [✓] 无")
    print()
    print("—— 词根档（001 有 3~5 字符公共串·人判表述差）：%d 例 ——"
          % len([判 for 判 in 例们 if 判["档"] == "中·词根档" and 判["法条"]["长"] >= 3]))
    样本 = [判 for 判 in 例们 if 判["档"] == "中·词根档" and 判["法条"]["长"] >= 3]
    列(样本, lambda 判: print(
        "  [%s] 法条=%r(节 %s) ｜ 诊断=%r ｜ 锚：%s" % (
            判["用例"], 判["法条"]["串"], 判["法条"]["节"] or "-",
            判["诊断"]["串"], " ｜ ".join(判["锚例"][:3]))))
    if not 样本:
        print("  （无）")
    print()
    if 孤儿 or 无着落:
        print("[黄] 处置：① 孤儿负例——人读 001 核该拒绝有无规范义务：有=补 001 法条落点，"
              "无=修期望文案（考卷与法条对齐）；② 法条无着落——核 001 表述差（补关键词）"
              "或规范缺口（呈报补法条）；③ 处置须按「提及即立项」落看板台账。")
    else:
        print("[✓] 无孤儿负例、无法条无着落。")


# ============ 自测（临时样例树正反两态·零触碰真实树） ============

def _造样例(根: Path) -> Path:
    """临时样例：规范文件 + 诊断源 + 6 例负例目录（正反两态）。返回规范文件路径。"""
    规范 = 根 / "001.md"
    规范.write_text(
        "## 二、关键字体系\n"
        "### 2.1 关键字分类表\n"
        "关键字不得作为标识符使用（示例正例略）。\n"
        "### 2.2 移除的关键字\n"
        "移除词可作标识符。\n"
        "## 三、类型系统\n"
        "### 3.1 默认类型\n"
        "隐式转换规则：窄类型可隐式转换为宽类型。\n"
        "隐式转换宽度不符者拒绝。\n",
        encoding="utf-8")
    源 = 根 / "src"
    (源 / "语义").mkdir(parents=True)
    (源 / "语义" / "a.cpp").write_text(
        'void f() {\n'
        '    // 注释里的串不算：注释专有短语不入面\n'
        '    report("关键字不得作为标识符使用（示例）");\n'
        '    report("窄类型可隐式转换为宽类型");\n'
        '    report("[安全区边界] " + k + " 须在 不安全 函数 内");\n'
        '}\n',
        encoding="utf-8")
    e2e = 根 / "tests" / "e2e"
    e2e.mkdir(parents=True)

    def 建(名, 文件们):
        d = e2e / 名
        d.mkdir(parents=True)
        for 文件名, 内容 in 文件们.items():
            (d / 文件名).write_text(内容, encoding="utf-8")

    # 正例：锚为规范直陈表述（001 有 ≥6 字符同串）
    建("01_直陈正例", {"主.cn": "x", "期望编译失败.txt": "关键字不得作为标识符使用"})
    # 正例：锚为诊断源实证（001 无直陈串·诊断面 LCS ≥6）
    建("02_诊断实证正例", {"主.cn": "x", "双编译对照.txt":
                    "退出码=负\n预期编译失败=是\n诊断锚=[安全区边界] 须在 不安全 函数 内\n"})
    # 反例·孤儿：锚 ≥4 字符且两面皆无 ≥3 字符公共串
    建("03_孤儿反例", {"主.cn": "x", "期望编译失败.txt": "幽灵协议握手失败禁止通行"})
    # 正例：词根档（3~5 字符公共串·不判孤儿）
    建("04_词根正例", {"主.cn": "x", "期望编译失败.txt": "隐式转换"})
    # 反例·短锚：全锚 <4 字符 → 机械不可判（非孤儿）
    建("05_短锚", {"主.cn": "x", "双编译对照.txt":
                "退出码=负\n预期编译失败=是\n诊断锚=重载\n"})
    # 反例·无有效锚：锚 <2 字符
    建("06_无有效锚", {"主.cn": "x", "双编译对照.txt":
                   "退出码=负\n预期编译失败=是\n诊断锚=?\n"})
    # 正例：负路径 .expected 锚行（剥通道前缀/标签/正文装饰后直陈 001）
    建("07_expected正例", {"主.cn": "x", "主.expected":
                     "[sem5] 正确诊断: [错误] 隐式转换宽度不符（行=3）\n"
                     "[sem5] 语义错误 1 个，中止 IR 生成与代码生成（退出码 1）\n",
                     "双编译对照.txt": "退出码=负\n预期编译失败=是\n"})
    # 反例·孤儿：断言短语只在注释中出现（诊断面不入·法条面无）——防注释假实证
    建("08_注释不实证", {"主.cn": "x", "期望编译失败.txt": "注释专有短语不入面"})
    # 反例·无锚：负例声明拒绝但零诊断文本（诊断锚留空）——照收不静默
    建("09_无锚负例", {"主.cn": "x", "双编译对照.txt":
                  "退出码=负\n预期编译失败=是\n诊断锚=\n"})
    return 规范


def selftest() -> int:
    """正反两态自证：档位分类/注释面排除/前缀剥除/短锚分流/挂靠对表。"""
    with tempfile.TemporaryDirectory(prefix="nea841_") as td:
        根 = Path(td)
        规范 = _造样例(根)
        e2e = 根 / "tests" / "e2e"
        面 = 建面(规范, 根)
        断言数 = [0]

        def 断言(条件, 说明):
            assert 条件, "自测失败：" + 说明
            断言数[0] += 1
            print("  ✓ " + 说明)

        例们 = {}
        for 名, 锚们 in 收集负例(e2e):
            判 = 对账一例(面, 锚们)
            判["用例"] = 名
            例们[名] = 判
        # 收集面：08 例锚=纯注释文本、09 例零锚，均照收（不静默）
        断言(len(例们) == 9, "负例枚举 9 例（含零锚负例·静默丢弃防线）")
        断言(例们["09_无锚负例"]["档"] == "无锚（仅 rc≠0 断言）"
           and 例们["09_无锚负例"]["有效锚"] == 0,
           "零锚负例照收并分类（诊断锚= 留空 → 无锚·仅 rc≠0 断言）")
        断言(例们["01_直陈正例"]["档"] == "强·法条直陈",
           "001 直陈锚 → 强·法条直陈")
        断言(例们["02_诊断实证正例"]["档"] == "强·诊断实证",
           "诊断源 LCS≥6 且 001 无直陈 → 强·诊断实证")
        断言(例们["02_诊断实证正例"]["诊断"]["长"] >= 8
           and 例们["02_诊断实证正例"]["诊断"]["文件"].endswith("a.cpp"),
           "诊断实证取双字面量分段的最长分段（[安全区边界] 8 字）")
        断言(核心("[安全区边界] 须在 不安全 函数 内") == "[安全区边界] 须在 不安全 函数 内"
           and 核心("[sem5] 正确诊断: [错误] x") == "[错误] x",
           "锚核心剥除：ASCII 通道前缀剥、纯中文方括号正文保留（265 案防误吞）")
        断言(核心("[sem5] 语义错误 1 个，中止 IR 生成与代码生成（退出码 1）") == "",
           "整行管道（进度/汇总行）忽略·不占对账面")
        断言(变体们("[错误] 隐式转换宽度不符（行=3）")
           == ["[错误] 隐式转换宽度不符（行=3）", "隐式转换宽度不符"],
           "变体：剥离正文装饰（[错误] 标记 + （行=N）注记）得正文核")
        断言(例们["03_孤儿反例"]["档"] == "孤儿负例", "两面无 ≥3 公共串 → 孤儿负例")
        断言(例们["04_词根正例"]["档"] == "中·词根档",
           "3~5 字符公共串（隐式转换）→ 词根档·不误判孤儿")
        断言(例们["05_短锚"]["档"] == "短锚·机械不可判", "全短锚（重载）→ 机械不可判")
        断言(例们["06_无有效锚"]["档"] == "无有效锚", "锚 <2 字符（?）→ 无有效锚")
        断言(例们["07_expected正例"]["档"] == "强·法条直陈"
           and 例们["07_expected正例"]["法条"]["直陈"],
           "负路径 .expected 锚行剥装饰后直陈 001（隐式转换宽度不符·变体判据）")
        断言(not any("注释专有短语不入面" in s for _, s in 面["语料"]),
           "注释内文本不入诊断面（字符串感知·注释里的串不算）")
        断言(例们["08_注释不实证"]["档"] == "孤儿负例",
           "注释专有锚两面皆无着落 → 孤儿（防注释假实证）")
        断言(例们["02_诊断实证正例"]["诊断"]["文件"].endswith("a.cpp"),
           "诊断面实证带文件来源")
        # 兜底：法条面节定位（直陈锚应落在 2.1 节）
        断言(例们["01_直陈正例"]["法条"]["节"] in ("2.1", "2"), "直陈锚定位到节 ID")
        print("自测通过：%d 项断言全绿" % 断言数[0])
        return 0


def main() -> int:
    解析 = argparse.ArgumentParser(
        description="负测期望反向对账（841）：题库→001 法条/诊断源 清点孤儿负例",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    解析.add_argument("--strict", action="store_true", help="孤儿负例>0 → rc=1（周检挂载点）")
    解析.add_argument("--json", action="store_true", help="机读报告")
    解析.add_argument("--limit", type=int, default=20, help="清单截断条数（0=全列·默认 20）")
    解析.add_argument("--selftest", action="store_true", help="自测（临时样例树·零触碰真实树）")
    参数 = 解析.parse_args()
    if 参数.selftest:
        return selftest()
    报告 = 全库对账()
    if 参数.json:
        print(json.dumps({
            "负例数": len(报告["例们"]),
            "挂靠可用": 报告["挂靠可用"],
            "例们": [{"用例": x["用例"], "档": x["档"],
                    "法条直陈": x["法条"]["直陈"], "法条长": x["法条"]["长"],
                    "法条串": x["法条"]["串"], "法条节": x["法条"]["节"],
                    "诊断长": x["诊断"]["长"], "诊断串": x["诊断"]["串"],
                    "诊断文件": x["诊断"]["文件"], "锚例": x["锚例"],
                    "挂靠": ["%s(%s)" % (u, c) for u, c in x["挂靠"]]}
                   for x in 报告["例们"]]}, ensure_ascii=False, indent=1))
    else:
        打印报告(报告, 参数.limit)
    孤儿数 = sum(1 for x in 报告["例们"] if x["档"] == "孤儿负例")
    if 参数.strict and 孤儿数:
        if not 参数.json:
            print("[FAIL] --strict：孤儿负例 %d 例（rc=1）" % 孤儿数)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
