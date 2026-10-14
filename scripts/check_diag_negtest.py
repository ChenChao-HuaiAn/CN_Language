#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_diag_negtest.py —— 诊断↔负测对账扫描（728n·「闸门验收两态」机械化对账面）。

病根（教训 #146 同族·193 实录）：宿主编译器某处注释写着「语义层已报错、此处防御
跳过」——注释声称检查存在，实际语义层零检查，防线交接断链存活很久才被探针戳穿。
审计结论：「检查点交接」没有验收环节——新增/交接一处语义闸门（编译期拒绝/诊断/
防线）时，验收必须制度化地包含「真实拿一个坏样本喂进去、确认被拦住」的正反两态
证据（正：坏样本被拦+文案对；反：好样本不被误伤）。本工具=该验收的**机械对账面**：
新增/改动的诊断文案应与 tests/e2e 负测锚池（run_e2e 三通道的锚）对得上账。

诚实边界（不假装全覆盖）：
  ① 本工具只做「诊断文案 ↔ 负测锚」的**存在性对账**（静态文本层面）——不证明坏
     样本运行期真被拦截。运行证据仍须按 ai-code-quality-guard skill「闸门验收两态」
     清单现场跑一遍（编译坏样本看诊断原文+退出码）；对账命中仅说明「有负测锚着落」。
  ② 文案动态拼接：按「单个静态段内部」的最长公共子串计算——锚跨变量缝时运行期
     不可能命中（降级为近似/弱档供人复查）；跨相邻字面量缝（C++ 自动连接）时实际
     可命中（同为近似档·需人判缝的性质）。
  ③ v2 IO::打印到错误 通道混有进度日志（[p6h]/[ir1]/[cg1] 等——非诊断拦截），本
     工具不做文本语义分类（避免假判），缺口清单可能含日志类噪音，人读时忽略。
  ④ 交接型防线（注释声称已拦）：机械无判据（自然语言声称），列为人工清单项——
     「禁信注释·必须现场证明」见 skill 闸门验收两态 ③。
  ⑤ 短文案（静态段 <4 字符）无法对账判档；2~3 字符的短锚（如「重载」）判弱命中
     提示复查（锚过泛·可误锚）。这两类均不视为机械可判面。

锚池来源（与 tests/e2e/run_e2e.py 负测三通道同源）：
  期望编译失败.txt（整文件=锚）/ 期望check失败.txt（跳过首行被检文件名）/
  双编译对照.txt 与 v2闭环.txt 的「诊断锚=」/ 非注释非空行 / 负测用例的 *.expected 行。
负测判定：目录含 期望编译失败.txt 或 期望check失败.txt，或配置含「退出码=负」/
「预期编译失败=是」（此时该目录 *.expected 行才有诊断锚义·正测 expected 不入池）。

档位（对每条诊断取最佳锚）：
  强命中 = 某锚(≥4字符)为某静态段的子串——run_e2e 子串匹配必然命中（前提该诊断路径被触发）
  近似   = 最长公共子串 ≥6 且非子串（疑跨缝·需人判相邻字面量/变量缝）
  弱     = 4≤LCS<6，或短锚(2~3字符)为段子串（锚过泛·证据强度低）
  缺口   = LCS<4（无任何锚着落=验收缺口·应补负测或按模板现场补证据）

用法：
  python scripts/check_diag_negtest.py                  # 全库盘点（报告式·rc=0）
  python scripts/check_diag_negtest.py --diff           # 本分支相对 develop 新增诊断对账（验收主用法）
  python scripts/check_diag_negtest.py --diff <提交/分支>  # 指定基线的增量对账
  python scripts/check_diag_negtest.py --diff --strict  # 增量对账：存在缺口 → rc=1（收工自查）
  python scripts/check_diag_negtest.py --all            # 全库盘点时全列缺口清单
  python scripts/check_diag_negtest.py --selftest       # 探测器自测（正/负样本·零触碰真实树）

退出码：0=报告完成（--strict 且增量有缺口=1）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）：泛型下标注解与 X|None 在 3.8 会炸
from __future__ import annotations

import argparse
import bisect
import io
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

仓库根 = Path(__file__).resolve().parent.parent

# ============ 常量与数据形态 ============

# 诊断通道模式（扫描面登记处——新增报告通道须在此登记，否则对账面有盲区）：
#   宿主三通道：diagnostics_.report（语义/IR 主通道·302 处）、reportErrorHere/
#     reportError（parser 便捷方法·67+约 35 处调用·定义行无文案自动滤除）。
#   v2 两通道：表诊断(诊断,（语义层诊断缓冲·168 处）、IO::打印到错误（语法/IR/
#     主入口直发·139 处·含进度日志见边界③）。
通道模式 = [
    ("宿主", re.compile(r"diagnostics_\.report\s*\(")),
    ("宿主", re.compile(r"reportErrorHere\s*\(")),
    ("宿主", re.compile(r"(?<![A-Za-z0-9_])reportError\s*\(")),
    ("v2", re.compile(r"表诊断\s*\(\s*诊断\s*,")),
    ("v2", re.compile(r"IO::打印到错误\s*\(")),
]

# 源树扫描面：后缀过滤（.hpp 里的声明行无字符串段·自动滤除）
扫描树 = (("src", (".cpp", ".hpp")), ("CN语言编译器v2", (".cn",)))

档缺口, 档弱, 档近似, 档强 = 0, 1, 2, 3
档名 = {档缺口: "缺口", 档弱: "弱", 档近似: "近似", 档强: "强"}
档标 = {档缺口: "✘", 档弱: "⚠", 档近似: "≈", 档强: "✔"}

LCS_近似阈值 = 6      # ≥6 判近似档
LCS_下限 = 4          # <4 一律缺口（4-gram 预筛保证：无公共 4-gram ⇒ LCS<4）
短锚阈值 = 4          # 锚 <4 字符的子串命中也只判弱档

# 通道前缀（[parse]/[错误]/[词法] 等）在 LCS 档计算前剥离——前缀是通道名而非
# 文案特征；不剥会让「[parse] 」8 字符公共前缀把无关文案顶成近似假信号。
前缀正则 = re.compile(r"^\[[^\]\n]{1,10}\]\s*")


def 剥前缀(s: str) -> str:
    return 前缀正则.sub("", s, count=1)


# ============ 一、静态文本提取（字符串感知状态机） ============

def 读字符串(文本: str, 起点: int):
    """读双引号字符串（起点=引号后一位）；返回 (内容, 引号后一位)。
    转义还原：\\" → "、\\\\ → \\；其余 \\x 保留原样两字符（锚匹配无害）。"""
    出 = []
    i = 起点
    n = len(文本)
    while i < n:
        c = 文本[i]
        if c == "\\" and i + 1 < n:
            nxt = 文本[i + 1]
            if nxt in ('"', "\\"):
                出.append(nxt)
            else:
                出.append(c)
                出.append(nxt)
            i += 2
            continue
        if c == '"':
            return "".join(出), i + 1
        出.append(c)
        i += 1
    return "".join(出), n  # 未闭合容错（截断文本）


def 跳字符字面量(文本: str, 起点: int) -> int:
    """跳过 C++/CN 字符字面量 'x'（起点=撇号）。短窗口（同行 ≤4 字符）判定，
    防数字分隔符 1'000 型撇号误吞——超窗即按普通字符处理。"""
    i = 起点 + 1
    n = len(文本)
    限 = min(n, 起点 + 6)
    while i < 限:
        c = 文本[i]
        if c == "\\":
            i += 2
            continue
        if c == "'":
            return i + 1
        if c == "\n":
            break
        i += 1
    return 起点 + 1  # 非字面量：单字符跳过


def 收集段(文本: str, 括号位: int):
    """从 括号位（'('）起做字符串感知括号配对；返回 (字符串段列表, 结束位|None)。

    状态机：代码态（括号计数）/行注释 // /块注释 /* */ /双引号字符串/字符字面量；
    段=调用内每个字符串字面量的内容（相邻字面量与拼接缝保持分段——对账时跨缝
    降档为近似，见边界②）。"""
    段 = []
    深度 = 0
    i = 括号位
    n = len(文本)
    while i < n:
        c = 文本[i]
        if c == "(":
            深度 += 1
            i += 1
        elif c == ")":
            深度 -= 1
            if 深度 == 0:
                return 段, i
            i += 1
        elif c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            i = n if j < 0 else j
        elif c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif c == '"':
            内容, i = 读字符串(文本, i + 1)
            段.append(内容)
        elif c == "'":
            i = 跳字符字面量(文本, i)
        else:
            i += 1
    return 段, None


def 注释区间表(文本: str):
    """标出注释区间 [(起, 止)]——字符串态优先（字符串内 // /* 不算注释）。
    用于丢弃落在注释内的调用匹配（注释里议论代码=常见，误提取会污染对账面）。"""
    区间 = []
    i = 0
    n = len(文本)
    while i < n:
        c = 文本[i]
        if c == '"':
            _, i = 读字符串(文本, i + 1)
        elif c == "'":
            i = 跳字符字面量(文本, i)
        elif c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            j = n if j < 0 else j
            区间.append((i, j))
            i = j
        elif c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            j = n if j < 0 else j + 2
            区间.append((i, j))
            i = j
        else:
            i += 1
    return 区间


def 在注释内(位置: int, 区间: list, 起表: list) -> bool:
    i = bisect.bisect_right(起表, 位置) - 1
    return i >= 0 and 区间[i][0] <= 位置 < 区间[i][1]


def 扫描文本(文本: str, 文件: str):
    """扫描单文件全部诊断调用；返回 [诊断]（文件/行/通道/段）。

    注释内与定义行（参数表无字符串段）自动滤除；段全空的调用不记录。"""
    记录 = []
    区间们 = 注释区间表(文本)
    起表 = [起 for 起, _ in 区间们]
    for 通道, 正则 in 通道模式:
        for m in 正则.finditer(文本):
            if 在注释内(m.start(), 区间们, 起表):
                continue
            括号位 = 文本.find("(", m.start())
            if 括号位 < 0:
                continue
            段们, _ = 收集段(文本, 括号位)
            段们 = [一段 for 一段 in 段们 if 一段.strip()]
            if not 段们:
                continue
            行号 = 文本.count("\n", 0, m.start()) + 1
            记录.append({"文件": 文件, "行": 行号, "通道": 通道, "段": 段们})
    return 记录


def 迭代源文件(根: Path):
    for 目录, 后缀们 in 扫描树:
        base = 根 / 目录
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if p.is_file() and p.suffix in 后缀们:
                yield p


def 收集全库诊断(根: Path):
    """全库扫描诊断调用；返回 (记录列表, 文件数, 失败行数)。"""
    记录 = []
    文件数 = 0
    失败 = 0
    for p in 迭代源文件(根):
        try:
            文本 = p.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            失败 += 1
            continue
        文件数 += 1
        相对 = p.relative_to(根).as_posix()
        记录.extend(扫描文本(文本, 相对))
    return 记录, 文件数, 失败


# ============ 二、锚池提取（run_e2e 负测三通道同源） ============

def 有效行们(文件: Path):
    return [行.strip() for 行 in 文件.read_text(encoding="utf-8").splitlines()
            if 行.strip() and not 行.strip().startswith("#")]


def 收集锚池(根: Path):
    """收集负测锚池；返回 (锚列表, 负测用例数, 忽略的短锚数)。

    锚 = {"文本", "来源"}；同一锚文本多来源保留首见（报告只展示一个来源）。"""
    锚们 = []
    用例数 = 0
    忽略 = 0
    e2e = 根 / "tests" / "e2e"
    if not e2e.exists():
        return 锚们, 0, 0
    for 目录 in sorted(p for p in e2e.iterdir() if p.is_dir()):
        负 = False
        f = 目录 / "期望编译失败.txt"
        if f.exists():
            负 = True
            for 行 in 有效行们(f):
                锚们.append({"文本": 行, "来源": f})
        f = 目录 / "期望check失败.txt"
        if f.exists():
            负 = True
            行们 = 有效行们(f)  # 首行=被检文件名（非锚）→ 跳过
            for 行 in 行们[1:]:
                锚们.append({"文本": 行, "来源": f})
        for 配置名 in ("双编译对照.txt", "v2闭环.txt"):
            f = 目录 / 配置名
            if not f.exists():
                continue
            for 行 in f.read_text(encoding="utf-8").splitlines():
                行 = 行.strip()
                if 行.startswith("诊断锚="):
                    v = 行[len("诊断锚="):].strip()
                    if v:
                        锚们.append({"文本": v, "来源": f})
                elif 行 in ("退出码=负", "预期编译失败=是"):
                    负 = True
        if 负:
            用例数 += 1
            for f in sorted(目录.glob("*.expected")):
                for 行 in 有效行们(f):
                    锚们.append({"文本": 行, "来源": f})
    # 短锚（<2 字符如「?」）无对账意义——单列计忽略
    保留 = []
    见 = set()
    for 锚 in 锚们:
        if len(锚["文本"]) < 2:
            忽略 += 1
            continue
        if 锚["文本"] in 见:
            continue
        见.add(锚["文本"])
        保留.append(锚)
    return 保留, 用例数, 忽略


# ============ 三、对账判定（4-gram 倒排预筛 + 分段 LCS） ============

def 建四元组位置表(s: str):
    表 = {}
    for i in range(len(s) - 3):
        表.setdefault(s[i:i + 4], []).append(i)
    return 表


def 最长公共子串长(a: str, b: str, b位置表=None) -> int:
    """a×b 最长公共子串长度。

    仅对 ≥4 精确（档位下限即 4；<4 返回 0 视为缺口——预筛已保证无公共 4-gram
    时确实 <4）。算法=遍历 a 的全部 4-gram 在 b 中的出现位，向两侧延展取最长；
    任何 ≥4 的公共子串必含一个 4-gram 匹配，故结果精确。"""
    if len(a) < 4 or len(b) < 4:
        return 0
    位置表 = b位置表 if b位置表 is not None else 建四元组位置表(b)
    最优 = 0
    n = len(a)
    nb = len(b)
    for i in range(n - 3):
        位们 = 位置表.get(a[i:i + 4])
        if not 位们:
            continue
        for j in 位们:
            k = 4
            while i + k < n and j + k < nb and a[i + k] == b[j + k]:
                k += 1
            m = 0
            while i - m - 1 >= 0 and j - m - 1 >= 0 and a[i - m - 1] == b[j - m - 1]:
                m += 1
            if k + m > 最优:
                最优 = k + m
    return 最优


def 建锚索引(锚池: list):
    """4-gram 倒排：gram → 锚下标列表；附锚的 LCS 用位置表缓存。

    文本一律以剥前缀后的「键文本」建索引（前缀非文案特征）；键文本 <4 的短锚
    无线可排——单独成表直接子串扫描（锚池里此类约十余条，逐条扫无性能问题）；
    对账时命中也只判弱档。"""
    倒排 = {}
    位置表缓存 = {}
    短锚们 = []
    for idx, 锚 in enumerate(锚池):
        文本 = 锚["文本"]
        键 = 剥前缀(文本)
        if len(键) < LCS_下限:
            短锚们.append((idx, 文本))
            continue
        位置表缓存[idx] = 建四元组位置表(键)
        for gram in set(位置表缓存[idx]):
            倒排.setdefault(gram, []).append(idx)
    return 倒排, 位置表缓存, 短锚们


def 对账一条(段们: list, 倒排: dict, 位置表缓存: dict, 短锚们: list, 锚池: list):
    """对一条诊断的静态段列表求最佳锚；返回 (档位, 锚文本, 来源, 匹配长度)。

    强档（≥4 字符锚完整包含于某段）即最高档——命中后立即返回（性能面）。"""
    最佳 = (档缺口, "", "", 0)
    # 短锚表直扫（子串命中判弱档——锚过泛·证据强度低）
    for idx, 文本 in 短锚们:
        if any(文本 in 段 for 段 in 段们):
            档 = 档弱
            if 档 > 最佳[0] or (档 == 最佳[0] and len(文本) > 最佳[3]):
                最佳 = (档, 文本, 锚池[idx]["来源"], len(文本))
    剥段们 = [剥前缀(段) for 段 in 段们]
    候选 = set()
    for 段 in 段们:
        for i in range(len(段) - 3):
            位们 = 倒排.get(段[i:i + 4])
            if 位们:
                候选.update(位们)
    for idx in sorted(候选):
        锚 = 锚池[idx]
        锚文本 = 锚["文本"]
        键 = 剥前缀(锚文本)
        for 段 in 段们:
            if 锚文本 in 段:
                档 = 档强
                if 档 > 最佳[0] or (档 == 最佳[0] and len(锚文本) > 最佳[3]):
                    最佳 = (档, 锚文本, 锚["来源"], len(锚文本))
                return 最佳  # 强档已最优·立即收（性能面）
        for 剥段 in 剥段们:
            if len(剥段) < LCS_下限 or len(键) < LCS_下限:
                continue
            L = 最长公共子串长(剥段, 键, 位置表缓存[idx])
            if L >= LCS_近似阈值:
                档 = 档近似
            elif L >= LCS_下限:
                档 = 档弱
            else:
                continue
            if 档 > 最佳[0] or (档 == 最佳[0] and L > 最佳[3]):
                最佳 = (档, 锚文本, 锚["来源"], L)
    return 最佳


def 对账一记录(记录: dict, 索引, 锚池: list):
    return 对账一条(记录["段"], *索引, 锚池)


# ============ 四、增量（--diff）新增区间解析 ============

正则_新文件 = re.compile(r"^\+\+\+ b/(.+)$")
正则_hunk = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


def 解析diff新增区间(文本: str) -> dict:
    """git diff -U0 输出 → {相对路径: [(起行, 止行)]}（新版本侧·纯删除 hunk 忽略）。"""
    结果 = {}
    当前 = None
    for 行 in 文本.splitlines():
        m = 正则_新文件.match(行)
        if m:
            当前 = m.group(1)
            结果.setdefault(当前, [])
            continue
        h = 正则_hunk.match(行)
        if h and 当前 is not None:
            c = int(h.group(1))
            d = int(h.group(2) or "1")
            if d > 0:
                结果[当前].append((c, c + d - 1))
    return {路径: 区间们 for 路径, 区间们 in 结果.items() if 区间们}


def 取新增区间(根: Path, 基线: str):
    """git diff 取扫描面新增行区间；返回 dict 或 None（git 失败）。"""
    r = subprocess.run(
        ["git", "-c", "core.quotepath=false", "diff", "-U0", 基线,
         "--", "src", "CN语言编译器v2"],
        cwd=str(根), capture_output=True, text=True)
    if r.returncode != 0:
        print("[错误] git diff 失败（基线 " + 基线 + "）: "
              + (r.stderr or "").strip()[:300])
        return None
    return 解析diff新增区间(r.stdout)


def 解析基线(根: Path, 参数值: str) -> str:
    if 参数值 and 参数值 != "__自动__":
        return 参数值
    r = subprocess.run(["git", "merge-base", "HEAD", "gitcode/develop"],
                       cwd=str(根), capture_output=True, text=True)
    if r.returncode == 0 and r.stdout.strip():
        return r.stdout.strip()
    return "develop"


# ============ 五、报告输出 ============

def 文案头(段们: list, 上限: int = 60) -> str:
    全部 = "".join(段们)
    return 全部[:上限] + ("…" if len(全部) > 上限 else "")


def 打印全库盘点(记录们: list, 索引, 锚池: list, 用例数: int, 忽略锚: int,
                 全列: bool) -> dict:
    统计 = {("宿主", 档): 0 for 档 in 档名}
    统计.update({("v2", 档): 0 for 档 in 档名})
    缺口清单 = []
    for 记录 in 记录们:
        档, 锚文本, 来源, _ = 对账一记录(记录, 索引, 锚池)
        统计[(记录["通道"], 档)] += 1
        if 档 == 档缺口:
            缺口清单.append(记录)
    print("== 诊断↔负测对账·全库盘点（check_diag_negtest.py·728n）==")
    print(f"扫描面：{len(记录们)} 条诊断调用；锚池 {len(锚池)} 条"
          f"（覆盖负测用例 {用例数} 个；忽略短锚 {忽略锚} 条）")
    for 通道 in ("宿主", "v2"):
        条 = sum(统计[(通道, 档)] for 档 in 档名)
        分 = " / ".join(f"{档名[档]} {统计[(通道, 档)]}" for 档 in (档强, 档近似, 档弱, 档缺口))
        print(f"  [{通道}] {条} 条：{分}")
    print("【档义】强=锚为静态段子串（run_e2e 必然命中）；近似=疑跨拼接缝待判；"
          "弱=短锚/LCS 4~5；缺口=无锚着落。")
    print(f"\n缺口清单（{len(缺口清单)} 条"
          + ("·--all 全列" if not 全列 else "·全列") + "）：")
    上限 = len(缺口清单) if 全列 else min(40, len(缺口清单))
    for 记录 in 缺口清单[:上限]:
        print(f"  {记录['文件']}:{记录['行']}  「{文案头(记录['段'])}」")
    if 上限 < len(缺口清单):
        print(f"  …（其余 {len(缺口清单) - 上限} 条用 --all 全列）")
    print("\n提示：缺口≠缺陷——既有存量面允许缺；新增/交接闸门请用 --diff 对账"
          "并按 skill「闸门验收两态」现场补证据。")
    return 统计


def 打印增量对账(记录们: list, 索引, 锚池: list, 基线: str) -> dict:
    print("== 诊断↔负测对账·增量（--diff·验收主用法）==")
    print(f"基线：{基线}")
    if not 记录们:
        print("本增量无新增/改动诊断调用（diff 只认新增行）——若本轮确实新增闸门，"
              "检查路径是否在扫描面（src/ 与 CN语言编译器v2/）。")
        return {档: 0 for 档 in 档名}
    统计 = {档: 0 for 档 in 档名}
    for 记录 in 记录们:
        档, 锚文本, 来源, 长度 = 对账一记录(记录, 索引, 锚池)
        统计[档] += 1
        行 = (f"  {档标[档]}{档名[档]:<2} {记录['文件']}:{记录['行']}  "
              f"「{文案头(记录['段'])}」")
        print(行)
        if 档 >= 档近似:
            来源段 = 来源
            if 来源段 and Path(来源段).is_absolute():
                try:
                    来源段 = Path(来源段).relative_to(仓库根).as_posix()
                except ValueError:
                    来源段 = str(来源段)
            print(f"        ← 锚[{锚文本}]（匹配 {长度} 字符）@ {来源段}")
    print(f"\n[增量结论] 新增诊断 {len(记录们)} 条：强 {统计[档强]} / 近似 "
          f"{统计[档近似]} / 弱 {统计[档弱]} / 缺口 {统计[档缺口]}")
    if 统计[档缺口]:
        print("  ⚠ 存在缺口：请补负测锚（tests/e2e 负测用例）或按 skill「闸门验收"
              "两态」现场补正反两态证据。")
    if 统计[档近似] or 统计[档弱]:
        print("  ⚠ 近似/弱档：人判拼接缝性质与锚泛化度（≈=疑跨缝；⚠=短锚/短共享段）。")
    return 统计


# ============ 六、自测（探测器自身也是被测对象·110-a 教训） ============
# 分三段函数（单函数 ≤100 行纪律·728n）；断言回调=（条件, 说明）。

def 自测段_提取(断言):
    """静态提取面：多行拼接/转义/注释过滤/v2 通道。"""
    # 1. 宿主提取：多行拼接 + 变量缝分段
    代码 = ('diagnostics_.report(DiagnosticLevel::Error, loc,\n'
            '                    "类 \'" + name + "\' 重复定义字段");')
    段, 末 = 收集段(代码, 代码.index("("))
    断言(段 == ["类 '", "' 重复定义字段"], f"宿主多行拼接段提取: {段!r}")
    断言(末 is not None, "括号配对闭合")

    # 2. 转义与内嵌括号
    代码 = 'reportErrorHere("预期 \'{\'（含转义 \\" 引号）");'
    段, _ = 收集段(代码, 代码.index("("))
    断言(段 == ['预期 \'{\'（含转义 " 引号）'], f"转义/括号: {段!r}")

    # 3. 注释内假调用不提取 + 字符串内括号不错配
    文本 = ('// diagnostics_.report(Error, loc, "假注释文案");\n'
            '/* reportErrorHere("块注释假文案"); */\n'
            'diagnostics_.report(Error, loc, "真文案（含）括号)");\n')
    记录 = 扫描文本(文本, "假.cpp")
    断言(len(记录) == 1 and 记录[0]["段"] == ["真文案（含）括号)"]
         and 记录[0]["行"] == 3, f"注释过滤与行号: {记录!r}")

    # 4. v2 提取：表诊断与 IO::打印到错误
    文本 = ('表诊断(诊断, "[错误] 借出视图 " + 视图文本 + " 在容器失效点之后仍被使用（行17）");\n'
            'IO::打印到错误("[parse] " + 类别 + "声明体以 \'}\' 自终结，其后不应有分号 \';\'（行=" + X + "）\\n");\n')
    记录 = 扫描文本(文本, "假.cn")
    断言(len(记录) == 2 and 记录[0]["通道"] == "v2", f"v2 提取: {记录!r}")
    断言(记录[1]["段"][1].startswith("声明体以 '}' 自终结"), f"v2 段2: {记录[1]['段']!r}")


def 自测段_对账(断言):
    """对账判定面：四档/LCS/前缀剥离。"""
    # 5. 对账四档（内存锚池）
    锚池 = [
        {"文本": "重复定义字段", "来源": "锚A"},      # 强（长锚子串）
        {"文本": "声明体以 '}' 自终结，其后不应有分号（行=", "来源": "锚B"},  # 近似（跨缝）
        {"文本": "借出视图 已转移", "来源": "锚C"},    # 弱（LCS 5 跨缝）
        {"文本": "重载", "来源": "锚D"},              # 短锚（弱·短锚表直扫）
    ]
    索引 = 建锚索引(锚池)
    档, 锚文本, _, L = 对账一条(["类 '", "' 重复定义字段"], *索引, 锚池)
    断言(档 == 档强 and 锚文本 == "重复定义字段", f"四档-强: {(档, 锚文本, L)}")
    档, 锚文本, _, L = 对账一条(["声明体以 '}' 自终结，其后不应有分号 ';'（行=",
                              "）"], *索引, 锚池)
    断言(档 == 档近似, f"四档-近似: {(档, 锚文本, L)}")
    档, 锚文本, _, L = 对账一条(["[错误] 借出视图 ", " 已转移，不能继续使用"],
                              *索引, 锚池)
    断言(档 == 档弱, f"四档-弱: {(档, 锚文本, L)}")
    档, 锚文本, _, L = 对账一条(["重载期望 ", " 个右操作数"], *索引, 锚池)
    断言(档 == 档弱 and 锚文本 == "重载", f"四档-短锚弱: {(档, 锚文本, L)}")
    档, _, _, _ = 对账一条(["完全无关的诊断文案"], *索引, 锚池)
    断言(档 == 档缺口, f"四档-缺口: {档}")

    # 6. LCS 精确性与 <4 归零（档位下限 4 之下不细究）
    断言(最长公共子串长("abc甲丁乙丙丁", "丁乙丙丁x") == 4, "LCS 基本")
    断言(最长公共子串长("abcdef", "zabcz") == 0, "LCS 3 <4 → 归零")
    断言(最长公共子串长("甲乙丙丁戊己", "丙丁戊己庚") == 4, "LCS 汉字")
    断言(最长公共子串长("数组下标越界", "编译期下标越界") == 4, "LCS 下标越界=4")

    # 6b. 通道前缀剥离（[parse]/[错误] 前缀非文案特征·不得顶出假近似）
    断言(剥前缀("[parse] 声明体尾分号") == "声明体尾分号", "剥前缀 [parse]")
    断言(剥前缀("[错误] 借出视图") == "借出视图", "剥前缀 [错误]")
    断言(剥前缀("无前缀文案") == "无前缀文案", "剥前缀 无前缀原样")
    锚池 = [{"文本": "[parse] -> 成员访问已废除（统一 .，指针自动解引用一级）", "来源": "锚P"}]
    索引 = 建锚索引(锚池)
    档, _, _, _ = 对账一条(["[parse] 静态/全局声明缺少类型名（行="], *索引, 锚池)
    断言(档 == 档缺口, f"前缀假近似应消除: {档}")


def 自测段_增量与端到端(断言):
    """diff 区间解析 + 临时迷你仓库端到端全流程。"""
    # 7. diff 新增区间解析（含纯删除 hunk 忽略 / 多 hunk）
    差异文本 = ('diff --git a/src/a.cpp b/src/a.cpp\n'
              '--- a/src/a.cpp\n'
              '+++ b/src/a.cpp\n'
              '@@ -10 +10,0 @@\n'
              '-删行\n'
              '@@ -20,0 +21,3 @@\n'
              '+甲\n+乙\n+丙\n'
              'diff --git a/CN语言编译器v2/主.cn b/CN语言编译器v2/主.cn\n'
              '--- a/CN语言编译器v2/主.cn\n'
              '+++ b/CN语言编译器v2/主.cn\n'
              '@@ -5 +5,2 @@\n')
    区间 = 解析diff新增区间(差异文本)
    断言(区间 == {"src/a.cpp": [(21, 23)], "CN语言编译器v2/主.cn": [(5, 6)]},
         f"diff 区间解析: {区间!r}")

    # 8. 端到端：临时迷你仓库（源码 + 负测锚）走全流程
    with tempfile.TemporaryDirectory() as 临时:
        根 = Path(临时)
        (根 / "src").mkdir()
        (根 / "src" / "甲.cpp").write_text(
            'void f() {\n'
            '  diagnostics_.report(Error, loc, "甲检查未通过：要素缺失");\n'
            '  diagnostics_.report(Error, loc, "乙检查缺锚文案");\n'
            '}\n', encoding="utf-8")
        (根 / "CN语言编译器v2").mkdir()
        (根 / "CN语言编译器v2" / "主.cn").write_text(
            'IO::打印到错误("[甲] " + 名 + "检查未通过：要素缺失（行=" + 行 + "）\\n");\n',
            encoding="utf-8")
        用例 = 根 / "tests" / "e2e" / "001_甲检查"
        用例.mkdir(parents=True)
        (用例 / "期望编译失败.txt").write_text("甲检查未通过", encoding="utf-8")
        记录们, 文件数, _ = 收集全库诊断(根)
        锚池, 用例数, 忽略 = 收集锚池(根)
        断言(文件数 == 2 and len(记录们) == 3 and len(锚池) == 1 and 用例数 == 1,
             f"端到端收集: 文件{文件数} 记录{len(记录们)} 锚{len(锚池)}")
        索引 = 建锚索引(锚池)
        结果 = [对账一条(记录["段"], *索引, 锚池)[0] for 记录 in 记录们]
        断言(sorted(结果) == [档缺口, 档弱, 档强], f"端到端对账档位: {结果}")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    for 段函数 in (自测段_提取, 自测段_对账, 自测段_增量与端到端):
        段函数(断言)
    if 失败:
        print("== 探测器自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 探测器自测全过（{通过[0]} 项断言·正/负样本齐备）==")
    return 0


# ============ 七、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="诊断↔负测对账扫描（728n·闸门验收两态的机械对账面）")
    解析器.add_argument("--diff", nargs="?", const="__自动__", metavar="基线",
                        help="增量对账：本分支相对基线（默认 merge-base HEAD gitcode/develop）"
                             "新增诊断逐条对账")
    解析器.add_argument("--strict", action="store_true",
                        help="增量对账存在缺口时 rc=1（收工自查用）")
    解析器.add_argument("--all", action="store_true", help="全库盘点全列缺口清单")
    解析器.add_argument("--selftest", action="store_true", help="探测器自测（零触碰真实树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    锚池, 用例数, 忽略锚 = 收集锚池(仓库根)
    索引 = 建锚索引(锚池)

    if 参数.diff is not None:
        基线 = 解析基线(仓库根, 参数.diff)
        区间表 = 取新增区间(仓库根, 基线)
        if 区间表 is None:
            return 2
        记录们 = []
        for 相对, 区间们 in sorted(区间表.items()):
            文件 = 仓库根 / 相对
            if not 文件.exists():
                continue
            try:
                文本 = 文件.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            for 记录 in 扫描文本(文本, 相对):
                if any(起 <= 记录["行"] <= 止 for 起, 止 in 区间们):
                    记录们.append(记录)
        统计 = 打印增量对账(记录们, 索引, 锚池, 基线)
        if 参数.strict and 统计[档缺口]:
            print("[--strict] 增量缺口未清 → rc=1", flush=True)
            return 1
        return 0

    记录们, 文件数, 失败数 = 收集全库诊断(仓库根)
    if 失败数:
        print(f"[警告] {失败数} 个源文件读取失败（已跳过）")
    打印全库盘点(记录们, 索引, 锚池, 用例数, 忽略锚, 参数.all)
    return 0


if __name__ == "__main__":
    sys.exit(main())
