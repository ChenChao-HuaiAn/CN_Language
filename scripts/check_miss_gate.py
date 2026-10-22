#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_miss_gate.py —— 静默值登记制硬拦门禁（882·648 立法精确化配套·用户两道令落地件）。

用户两道令（2026-10-17·台账 #882）：①登记制——默认值/豁免面禁散写，统一登记
  scripts/miss_registry.json（文件/函数/锚文本/类别/来源/静默风险/原因/登记人），代码侧
  协议化注释「// 静默豁免<类别>: 原因」（对齐 Google TODO(username) 强制格式先例）；
  ②L1 硬拦（非报告式）——三红任一命中=gate_quick 静态段直接拦提交（exit≠0），无逃生门
  flag；临时豁免唯一合法通道=登记表补条（+协议注释）。

四面×四红（执行面）：
  面①「静默形态」=复用了 950 扫描核（scripts/check_miss_defaults.py）：miss 判定→返默认值
    形态候选；面②「else业务」=else/否则 块含业务语句（用户 else 新规：else 降格为非法输入
    拦截分支·块内只许 报错/打印到错误/返回错误码/断言）；面③「单边比较」=分支条件
    （if/如果/else if/否则如果）单边无限区间（一侧字面量的 >= > < <= 比较·无反向第二比较
    即红）；面④「红旗词面」=648 驳回词注释（防御性兜底/保守回退/miss 即默认/查不到按/
    兜底默认）无登记。红①新增静默面无登记 ②漏登记（注释无条）③陈旧（条锚消失）
    ④红旗词面无登记；另：协议注释类别不在既控枚举=红（格式强制）。

冻结基线（只减不增·借鉴 check_layer_deps.py 先例）：scripts/miss_freeze_baseline.json =
  各面位置快照（键=「文件::归一锚文本」·同键重复=合法计数）+生成 sha。位置存在于基线=放行
  （存量）；新增位置=红（除非该位置有协议注释且登记表有条）；基线位置消失=自动消减提示。
  首次上线=以当前实测为准生成基线；存量清偿挂 #950/#891，清后 --update-baseline 收账。

诚实边界（不假装全覆盖·机械筛+人判+登记留痕闭环）：
  ①语义级默认值机器不可全识（`return 0` 千种语境）；本门禁抓「留痕」核「登记」（业界上限）。
  ②else 内容分类器=机械白名单（报错名子串表/断言/负数码/假nullptr/空指针）；不可判形态
     收严为红+登记例外通道（禁放宽）。③单边比较只扫分支条件（循环头天然不扫）；指针判空
     （nullptr/空指针/「*」前缀操作数）排除；双边界须同一操作数上界+下界同现。
  ④位置键=文件+归一锚文本·计数只减不增；文本漂移使旧键消失、新键判新增（收严取向·修理=
     登记豁免，或确认无误报后 --update-baseline 收账）。⑤性能：不调 950 收函数头们（省
     8.5s·不用函数归属字段）；950 else-邻接窗口退化为 ±5 行（多收=更严·已并入基线）。
  ⑥本工具只读不写（--update-baseline 除外）；登记动作=人读代码后手写 miss_registry.json。

用法：
  python3 scripts/check_miss_gate.py                    # 门禁（rc=1=有红·gate_quick 静态段调用）
  python3 scripts/check_miss_gate.py --report           # 报告模式（全列在测/消减/放行对照·rc=0）
  python3 scripts/check_miss_gate.py --json             # 机读输出（stdout）
  python3 scripts/check_miss_gate.py --update-baseline  # 以当前实测重写冻结基线（清账后收基线）
  python3 scripts/check_miss_gate.py --selftest         # 自测（临时样例树·正反两态+三红各一例）

退出码：0=绿（无红）；1=红（有红·L1 硬拦）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import bisect
import json
import re
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
仓库根 = 脚本目录.parent

# 复用既有扫描核（纪律二·重复归纳：950 形态/留痕扫描 + 968 词法器；不重复实现）。
# 注意：两模块模块级各自把 sys.stdout/stderr 重包为 utf-8 TextIOWrapper——必须保留被
# 替换下来的包装器引用（_保链），否则其被子引用回收时 __del__ 会 close 底层 buffer，
# 使后一个包装器写失败（ValueError: I/O operation on closed file·882 轮实证）。
import check_miss_defaults as 核950          # noqa: E402
_保链 = [sys.stdout, sys.stderr]
import check_return_paths as 核968           # noqa: E402
_保链 += [sys.stdout, sys.stderr]

登记表文件 = 仓库根 / "scripts" / "miss_registry.json"
基线文件 = 仓库根 / "scripts" / "miss_freeze_baseline.json"

# ============ 常量与判据表 ============

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")
面名们 = ("静默形态", "else业务", "单边比较", "红旗词面")

# 协议化注释（登记制·代码侧）：// 静默豁免<类别>: 原因——注释段须以「静默豁免」开头
协议注释 = re.compile(r"^静默豁免\s*([A-Za-z\u4e00-\u9fff]{1,16})\s*[:：]\s*(.+)$")
# 类别枚举（648 登记制台账口径 + miss_registry.json 现枚举的并集·改枚举=改门禁）
豁免类别枚举 = {"上游已诊断", "上游已诊断恢复", "闭集穷举", "运行时API",
              "业务值", "确权不求值", "待裁决"}

# else 块内容分类器（用户 else 新规·机械白名单）
else模式 = re.compile(
    r"(?<![A-Za-z0-9_])else(?![A-Za-z0-9_])|否则如果|(?<![\u4e00-\u9fff])否则")
语句控制词 = {"if", "for", "while", "do", "switch", "try", "如果", "循环", "当",
           "遍历", "选择", "情况", "否则"}
# 终止/断言类首 token（合法）
首token终止 = {"assert", "static_assert", "CN_ASSERT", "throw", "abort",
            "断言", "不可达", "panic", "Panic"}
# 报错/警告调用名子串表（合法·命中即视为报错调用）
报错调用名 = ("report", "诊断", "错误", "警告", "error", "Error", "warn", "Warn",
           "panic", "Panic", "abort", "断言", "assert", "报错",
           "打印到错误", "打印警告", "fatal", "Fatal", "失败", "运行时报错")
# 返回「错误码」族（合法）：负数字面量 / false / 假 / nullptr / 空指针
返回错误码词 = {"false", "假", "nullptr", "空指针"}

# 单边开区间判据（用户裁决二·机械判据）
分支条件模式 = re.compile(
    r"(?<![A-Za-z0-9_\u4e00-\u9fff#])(?:if|如果)\s*\(|否则如果\s*\(")
区间比较 = re.compile(r"(>=|<=|>|<)")
下界方向 = {">", ">="}
字面量正则 = re.compile(
    r"^-?(?:0[xX][0-9a-fA-F]+|0[bB][01]+|[0-9]+(?:\.[0-9]+)?)[fFuUlL]*$")
标识符正则 = re.compile(r"^[A-Za-z_\u4e00-\u9fff][\w\u4e00-\u9fff]*$")

条件跨行上限 = 400     # 条件文本长度上限（超出跳过·防病态）
括号回溯上限 = 8       # 分支模式向前找 `(` 的窗口


# ============ 一、掩码（注释/字符串内容抹空·长度与换行保持·偏移可复用） ============

def 串尾(文本: str, i: int, 跨行断: bool) -> int:
    """i 处为引号 `"`/`'`；返回字符串段结束下标（含结束引号）。

    跨行断=True（全量掩码）时未闭合字符串扫到行尾即止；False（单行）扫到文尾。"""
    引, j, n = 文本[i], i + 1, len(文本)
    while j < n:
        if 文本[j] == "\\" and 引 == '"':
            j += 2
            continue
        if 文本[j] == 引:
            return j + 1
        if 跨行断 and 文本[j] == "\n":
            return j
        j += 1
    return n


def 掩码单行(行: str) -> str:
    """单行掩码（无跨行块注释态的快速路径）：行尾 // 注释与字符串整体抹空格。"""
    if "/" not in 行 and '"' not in 行 and "'" not in 行:
        return 行
    掩 = list(行)
    i, n = 0, len(行)
    while i < n:
        c = 行[i]
        if c == "/" and i + 1 < n and 行[i + 1] == "/":
            for k in range(i, n):
                掩[k] = " "
            break
        if c == '"' or c == "'":
            j = 串尾(行, i, False)
            for k in range(i, min(j, n)):
                掩[k] = " "
            i = j
            continue
        i += 1
    return "".join(掩)


def 掩码(文本: str) -> str:
    """注释与字符串段整体抹空格（换行保留·长度保持=偏移/行号与原文一致）。

    无跨行 `/*` 时走逐行快速路径（性能）；掩码上扫描的 if/else 假命中天然消失。"""
    if "/*" not in 文本:
        return "\n".join(掩码单行(行) for 行 in 文本.split("\n"))
    掩 = list(文本)
    n = len(文本)
    i = 0
    while i < n:
        c = 文本[i]
        if c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                if 掩[k] != "\n":
                    掩[k] = " "
            i = j
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if 掩[k] != "\n":
                    掩[k] = " "
            i = j
            continue
        if c == '"' or c == "'":
            j = 串尾(文本, i, True)
            for k in range(i, min(j, n)):
                if 掩[k] != "\n":
                    掩[k] = " "
            i = j
            continue
        i += 1
    return "".join(掩)


def 建行首表(文本: str):
    """行首偏移表（偏移→行号 bisect 用·re.finditer 走 C 层提速）。"""
    表 = [0]
    表.extend(m.start() + 1 for m in re.finditer("\n", 文本))
    return 表


def 取行号(行首表: list, 偏移: int) -> int:
    return bisect.bisect_right(行首表, 偏移)


def 配对括号(文本: str, 开位: int):
    """掩码文本上 `(`/`{` 起配对；返回配对下标或 None（不设行限·防病态用长度差守卫）。"""
    对 = {"(": ")", "{": "}", "[": "]"}
    期望 = 对.get(文本[开位])
    if 期望 is None:
        return None
    深 = 0
    for i in range(开位, len(文本)):
        if 文本[i] == 文本[开位]:
            深 += 1
        elif 文本[i] == 期望:
            深 -= 1
            if 深 == 0:
                return i
    return None


# ============ 二、else 块扫描（用户 else 新规·机械分类器） ============

def 切语句(toks: list):
    """词法 token 序列→语句列表（顶层 `;`/控制结构体结束切分·嵌套括号并入）。"""
    语句们 = []
    当前 = []
    深 = 0
    for tok, _行 in toks:
        if tok in ("{", "(", "["):
            深 += 1
            当前.append(tok)
            continue
        if tok in ("}", ")", "]"):
            深 = max(0, 深 - 1)
            当前.append(tok)
            if 深 == 0 and tok == "}" and 当前 and 当前[0] in 语句控制词:
                语句们.append(当前)
                当前 = []
            continue
        if tok == ";" and 深 == 0:
            语句们.append(当前)
            当前 = []
            continue
        当前.append(tok)
    if 当前:
        语句们.append(当前)
    return 语句们


def 调用名们(语: list):
    """语句 token 里的调用名（标识符紧跟 `(`·限定名前缀自动取末段）。"""
    return [t for i, t in enumerate(语[:-1])
            if 语[i + 1] == "(" and 标识符正则.match(t)]


def 分类语句(语: list) -> str:
    """语句分类：报错|返回错误码|返回值|其他|空（用户 else 白名单机械版）。"""
    if not 语:
        return "空"
    首 = 语[0]
    if 首 in ("return", "返回"):
        if len(语) == 1:
            return "返回错误码"          # 空返回=终止动作
        值 = 语[1:]
        if 值[0] in 返回错误码词:
            return "返回错误码"          # false/假/nullptr/空指针=错误传播惯习
        if 值[0] == "-" and len(值) > 1 and 值[1][:1].isdigit():
            return "返回错误码"          # -1 等负数字面量=错误码
        return "返回值"
    if 首 in 首token终止:
        return "报错"
    if any(t in ("cerr", "stderr") for t in 语):
        return "报错"                    # 打印到标准错误=合法
    if any(子串 in 名 or 名 in 子串 for 名 in 调用名们(语)
           for 子串 in 报错调用名):
        return "报错"
    return "其他"


def 判else块源(块源: str):
    """else 块源文本→(是否红, 首违语句文本)。状态机：无「其他」且
    （无「返回值」或 有「报错」）→ 合法（648 报诊断+返回收尾模式放行）。"""
    归一 = re.sub(r"\s+", "", 块源)
    if ".value_or(" in 归一 or ".value(" in 归一:
        # 1360 结果化消费点豁免：块内每条语句均为 Optional 取值（.value()/.value_or(）
        # =类型强制通道的取值赋值（显形默认/坏值 abort）·非业务分支
        语句们 = [x for x in re.split(r"[;{}]", 归一) if x.strip()]
        if 语句们 and all((".value(" in x or ".value_or(" in x) for x in 语句们):
            return False, ""
    语们 = 切语句(核968.词法(块源))
    有报错 = False
    返回任意 = None
    for 语 in 语们:
        类 = 分类语句(语)
        if 类 == "报错":
            有报错 = True
        elif 类 == "返回值" and 返回任意 is None:
            返回任意 = "".join(语)
        elif 类 == "其他":
            return True, "".join(语)
    if 返回任意 is not None and not 有报错:
        return True, 返回任意
    return False, ""


def 扫else块(相对: str, 掩: str, 原文: str, 行首表: list):
    """全文件 else/否则 块扫描 → 红点列表（else-if/否则如果 是链延续·跳过）。"""
    红点 = []
    行们 = 原文.split("\n")
    for m in else模式.finditer(掩):
        if m.start() > 0 and 掩[m.start() - 1] == "#":
            continue                     # 预处理 #else/#endif 行（非语言层 else·969 实测盲区）
        j = m.end()
        限 = min(len(掩), j + 200)
        while j < 限 and 掩[j] in " \t\r\n":
            j += 1
        if 掩.startswith("如果", j):
            continue                     # 否则 如果：链延续
        if 掩.startswith("if", j) and not 掩[j + 2:j + 3].isalnum():
            continue                     # else if：链延续
        首 = 掩[j] if j < 限 else ""
        if 首 == "{":
            闭 = 配对括号(掩, j)
            if 闭 is None:
                continue
            源 = 原文[j + 1:闭]
        elif 首 and 首 != ";":
            k = 掩.find(";", j)
            if k < 0 or k - j > 条件跨行上限:
                continue
            源 = 原文[j:k]                # 单语句 else（无块）
        else:
            continue
        红, 违 = 判else块源(源)
        if 红:
            行 = 取行号(行首表, m.start())
            行文 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
            红点.append({"面": "else业务", "文件": 相对, "行": 行,
                       "锚": 行文 + " │ " + 违[:80]})
    return 红点


# ============ 三、单边比较扫描（用户单边开区间禁令·机械判据） ============

def 是字面量(s: str) -> bool:
    t = re.sub(r"\s+", "", s)
    if not t:
        return False
    if 字面量正则.match(t):
        return True
    return len(t) >= 2 and t[0] == t[-1] and t[0] in "\"'"


def 跳过配对(文本: str, k: int, 向: int, 配对符: str, 嵌套符: str) -> int:
    """k 处为 嵌套符（`(`/`[` 一方）；沿 向 越过其 配对符，返回 配对符 前一位（未配对=-1）。"""
    深 = 1
    k += 向
    while 0 <= k < len(文本):
        if 文本[k] == 嵌套符:
            深 += 1
        elif 文本[k] == 配对符:
            深 -= 1
            if 深 == 0:
                return k + 向
        k += 向
    return -1


def 截操作数(文本: str, pos: int, 向左: bool):
    """比较符一侧的操作数文本（扫到顶层分隔符为止·括号组整体回跳）。

    向左：从 pos-1 向左扫；遇 `)`/`]` 回跳到配对开符前继续（函数调用/下标整体保留），
    遇顶层分隔符（逗号/分号/括号/与或符/且或）停。向右对称。"""
    括号对 = {")": "(", "]": "[", "(": ")", "[": "]"}   # 键=遇见的括号·值=其配对符
    k = pos - 1 if 向左 else pos
    向 = -1 if 向左 else 1
    n = len(文本)
    越界 = (lambda x: x < 0) if 向左 else (lambda x: x >= n)
    while not 越界(k):
        c = 文本[k]
        # 本方向的「闭侧括号」（向左 `)`/`]`·向右 `(`/`[`）=回跳配对；反侧=顶层括号停
        if c in 括号对 and (c in ")]") == 向左:
            k = 跳过配对(文本, k, 向, 括号对[c], c)
            continue
        if c in "()[]{};,?:" or c == "且" or c == "或":
            break
        if c == "&" and 0 <= k + 向 < n and 文本[k + 向] == "&":
            break
        if c == "|" and 0 <= k + 向 < n and 文本[k + 向] == "|":
            break
        k += 向
    return 文本[k + 1:pos] if 向左 else 文本[pos:k]


def 分析条件(条: str):
    """条件文本→红原因串或 ""（单边无限区间=红·双边界/无字面量比较=绿）。

    判据（用户单边开区间禁令·机械版）：存在「一侧为字面量的区间比较」且其操作数
    无反向第二比较（同一操作数的上界+下界同时出现）→ 红；否则绿。指针判空
    （nullptr/空指针/「*」前缀操作数）排除。"""
    撤 = 条.replace("->", "  ").replace("<<", "  ").replace(">>", "  ")
    if "nullptr" in 撤 or "空指针" in 撤:
        return ""                        # 指针判空排除（区间符下的空指针比较不判）
    if ".value()" in 条 or ".值(" in 条:
        return ""                        # 1360：Optional 取值接收者的有效性守卫=契约边界检查（结果化消费点豁免）
    记录 = {}                            # 操作数归一 → {"向": 方向集, "字面量": bool}
    for m in 区间比较.finditer(撤):
        左 = 截操作数(撤, m.start(), True)
        右 = 截操作数(撤, m.end(), False)
        左字, 右字 = 是字面量(左), 是字面量(右)
        if 左字 and 右字:
            continue                     # 常量间比较（不可能·防御）
        if 右字:
            侧 = 左                       # 右字面量→操作数在左
        elif 左字:
            侧 = 右                       # 左字面量（90 <= 分数 型）→操作数在右
        else:
            侧 = 左                       # 双变量比较→取左（只贡献方向）
        侧键 = re.sub(r"\s+", "", 侧)
        if not 侧键:
            continue
        if re.match(r"^\*", 侧键) or 侧键 in ("nullptr", "空指针", "无"):
            continue                     # 指针解引用/空值操作数排除
        e = 记录.setdefault(侧键, {"向": set(), "字面量": False})
        运向 = "下" if m.group(1) in 下界方向 else "上"
        if 左字:
            运向 = "上" if 运向 == "下" else "下"   # 字面量在左·对右操作数方向反转
        e["向"].add(运向)
        e["字面量"] = e["字面量"] or 左字 or 右字
    for 侧键, e in 记录.items():
        if not e["字面量"]:
            continue                     # 红前提=至少一侧为字面量（双变量界不判）
        if len(e["向"]) >= 2:
            continue                     # 同操作数上界+下界=双边界闭区间（合法）
        return f"单边无限区间（{侧键} 仅{('下' if '下' in e['向'] else '上')}界·无反向第二比较）"
    return ""


def 扫单边比较(相对: str, 掩: str, 行首表: list):
    """分支条件单边开区间扫描（循环头天然不扫·见模块 docstring 边界③）。"""
    红点 = []
    for m in 分支条件模式.finditer(掩):
        开位m = 掩.find("(", m.start(), m.start() + 括号回溯上限 + 8)
        if 开位m < 0:
            continue
        闭 = 配对括号(掩, 开位m)
        if 闭 is None or 闭 - 开位m > 条件跨行上限:
            continue
        条 = 掩[开位m + 1:闭]
        原因 = 分析条件(条)
        if not 原因:
            continue
        行 = 取行号(行首表, m.start())
        行起 = 行首表[行 - 1] if 行 - 1 < len(行首表) else 0
        行终 = 掩.find("\n", 行起)
        行文 = 掩[行起:行终 if 行终 >= 0 else len(掩)].strip()
        if "std::nullopt" in 行文 or "返回 无" in 行文:
            continue           # 1360：分支返回 nullopt=结果化否定回答通道（守卫·非业务区间）
        红点.append({"面": "单边比较", "文件": 相对, "行": 行,
                   "锚": 行文[:120], "条件": re.sub(r"\s+", "", 条)[:80], "因": 原因})
    return 红点


# ============ 四、协议注释扫描（登记制·代码侧留痕） ============

def 扫协议注释(相对: str, 行们: list):
    """全文件协议注释点 → [{文件,行,锚定行,锚,类别,原因,已登记(后填)}]。

    锚定规则：注释行代码非空→锚=该行代码；注释独占一行→锚=下一行代码文本
    （协议注释应写在豁免代码同行或上一行）。"""
    点们 = []
    for k, (行号, 代码, 注释) in enumerate(行们):
        if not 注释:
            continue
        注释净 = re.sub(r"^(?://+|/\*+|\*+|\*/)\s*",
                      "", 注释.strip()).strip()
        m = 协议注释.match(注释净)
        if not m:
            continue
        类别, 原因 = m.group(1), m.group(2).strip()
        if not 原因:
            原因 = "（未写原因）"
        锚, 锚定行 = 代码.strip(), 行号
        if not 锚:                       # 注释独占行→锚=下一行代码（最近 3 行内）
            for 行号2, 代码2, _c2 in 行们[k + 1:k + 4]:
                if 代码2.strip():
                    锚, 锚定行 = 代码2.strip(), 行号2
                    break
        点们.append({"文件": 相对, "行": 行号, "锚定行": 锚定行, "锚": 锚,
                   "类别": 类别, "原因": 原因, "已登记": False})
    return 点们


# ============ 五、登记表/基线读写与对账 ============

def 归一(文本: str) -> str:
    return re.sub(r"\s+", "", 文本 or "")


def 条锚(条: dict) -> str:
    return 归一(条.get("锚文本", "") or 条.get("锚", ""))


def 锚相容(甲: str, 乙: str) -> bool:
    """协议注释锚与红点锚的关联判定（归一后双向子串·防跨行组合锚失配）。"""
    a, b = 归一(甲), 归一(乙)
    return bool(a and b) and (a in b or b in a)


def 条命中(文件: str, 锚: str, 条目们: list):
    """锚对账：同文件且（条锚 in 锚 ∨ 锚 in 条锚）→ 条目（双向子串·防前缀误配）。"""
    锚n = 归一(锚)
    for 条 in 条目们:
        if 锚n and 条.get("文件") == 文件:
            a = 条锚(条)
            if a and (a in 锚n or 锚n in a):
                return 条
    return None


def 读基线(路径: Path):
    """读冻结基线；缺失=异常（门禁拒跑·防绕过）；损坏=异常。

    基线存各面键列表（键=「文件::归一锚」·同键重复出现=该位置的合法计数）——
    判定按计数只减不增（同文件同锚复制出第二处也算新增·防误放）。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁] 基线缺失：{路径.name}——先跑 "
                         f"python3 scripts/check_miss_gate.py --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁] 基线损坏（{e.__class__.__name__}）：{路径.name}")
    return {面: Counter(数据.get("面", {}).get(面, [])) for 面 in 面名们}


# ============ 六、全库扫描 ============

def 扫文件(相对: str, 文本: str, v2: bool):
    """单文件四面扫描。返回 (面点dict, 协议点们)。

    性能（882）：不调 950 收函数头们（正则拼接占全库 8.5s）——不用候选的函数/返回
    类型归属字段；950 else-邻接窗口退化为 ±5 行（多收候选=更严·差异面并入基线）。"""
    行们 = 核950.拆行(文本)
    面点 = {面: [] for 面 in 面名们}
    候选, _弱 = 核950.扫形态(相对, 行们, [])
    for c in 候选:
        面点["静默形态"].append({"面": "静默形态", "文件": 相对, "行": c["行"],
                              "锚": c["锚"]})
    for h in 核950.扫留痕(相对, 行们):
        if h["类"] == "驳回词":
            面点["红旗词面"].append({"面": "红旗词面", "文件": 相对, "行": h["行"],
                                 "锚": h["注释"], "词": h["词"]})
    掩 = 掩码(文本)
    行首表 = 建行首表(文本)
    面点["else业务"] = 扫else块(相对, 掩, 文本, 行首表)
    面点["单边比较"] = 扫单边比较(相对, 掩, 行首表)
    协议点们 = 扫协议注释(相对, 行们)
    return 面点, 协议点们


def 全库扫描(根: Path):
    """扫 src/ 与 CN语言编译器v2/（882 扫描面=编译器实现代码·两侧同规）。"""
    面点 = {面: [] for 面 in 面名们}
    协议点们 = []
    文件们 = {}                          # 相对路径 → 归一全文（陈旧检测用）
    统计 = {"文件": 0, "读取失败": 0}
    for 前缀 in 扫描前缀:
        base = 根 / 前缀.rstrip("/")
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if not p.is_file() or p.suffix not in 扫描后缀:
                continue
            相对 = 前缀 + p.relative_to(base).as_posix()
            try:
                文本 = p.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                统计["读取失败"] += 1
                continue
            统计["文件"] += 1
            文件们[相对] = 归一(文本)
            面点此, 协议此 = 扫文件(相对, 文本, p.suffix == ".cn")
            for 面 in 面名们:
                面点[面].extend(面点此[面])
            协议点们.extend(协议此)
    return {"面点": 面点, "协议点": 协议点们, "文件文本": 文件们, "统计": 统计}


def 建点键(点: dict) -> str:
    return 点["文件"] + "::" + 归一(点["锚"])


# ============ 七、判定引擎（四红） ============

def 判定(扫描果: dict, 条目们: list, 基线: dict):
    """返回 {"红":[], "豁免":[], "消减":{}, "统计":{}}。"""
    红们 = []
    豁免们 = []
    消减 = {}
    面统 = {}
    # 协议点登记对账（先做·红点豁免判定依赖）
    for 协议 in 扫描果["协议点"]:
        if 协议["类别"] not in 豁免类别枚举:
            红们.append({"种类": "协议注释类别非法", "文件": 协议["文件"], "行": 协议["行"],
                       "锚": 协议["锚"],
                       "说明": f"类别「{协议['类别']}」不在枚举 {sorted(豁免类别枚举)}"})
        else:
            条 = 条命中(协议["文件"], 协议["锚"], 条目们)
            if 条 is None:
                红们.append({"种类": "漏登记", "文件": 协议["文件"], "行": 协议["行"],
                           "锚": 协议["锚"],
                           "说明": "代码有协议注释而登记表无条——补 miss_registry.json 条目"})
                continue
            # 1238 批准制（2026-10-20 用户立法）：登记条须带批准状态才放行——
            #   已批/存量过渡=绿；缺批准/呈报中/其他=红「登记无批准」（新兜底先呈报后编码）；
            #   已毙=红「已毙未清偿」（毙而锚仍在代码）。
            批 = (条.get("批准") or "").strip()
            if 批 == "已毙":
                红们.append({"种类": "已毙未清偿", "文件": 协议["文件"], "行": 协议["行"],
                           "锚": 协议["锚"],
                           "说明": "登记条已判「已毙」而兜底代码仍在——按兜底禁令改报内部错误，"
                                 "清偿后删条或改批准状态"})
            elif 批 not in ("已批", "存量过渡"):
                红们.append({"种类": "登记无批准", "文件": 协议["文件"], "行": 协议["行"],
                           "锚": 协议["锚"],
                           "说明": f"登记条批准状态={批 or '（缺字段）'}≠已批/存量过渡——"
                                 f"兜底禁令（#1238）：新登记须呈报用户批准后才放行"})
            else:
                协议["已登记"] = True
    # 面点×基线×豁免（基线=计数语义·只减不增）
    for 面 in 面名们:
        点们 = 扫描果["面点"][面]
        现计 = {}
        for 点 in 点们:
            现计.setdefault(建点键(点), []).append(点)
        基计 = 基线.get(面, {})
        新增 = []
        for 键 in sorted(现计):
            b = 基计.get(键, 0)
            点列 = sorted(现计[键], key=lambda x: (x["行"], x["锚"]))
            if len(点列) > b:
                新增.extend(点列[b:])       # 前 b 个视作基线存量·超出的才是新增
        减 = []
        for 键, b in sorted(基计.items()):
            c = len(现计.get(键, []))
            if c < b:
                减.extend([键] * (b - c))
        消减[面] = 减
        放行数 = 0
        红数 = 0
        for 点 in 新增:
            同行协议 = None
            for 协议 in 扫描果["协议点"]:
                if 协议["文件"] == 点["文件"] and 协议["锚定行"] == 点["行"] \
                        and 锚相容(协议["锚"], 点["锚"]):
                    同行协议 = 协议
                    break
            if 同行协议 is not None and 同行协议["已登记"]:
                放行数 += 1
                豁免们.append({"面": 面, "文件": 点["文件"], "行": 点["行"],
                            "锚": 点["锚"][:100], "豁免": "协议注释+登记条目"})
                continue
            if 同行协议 is not None:
                # 「漏登记/类别非法」红已由协议点检查覆盖——本红点不重复报
                continue
            条 = 条命中(点["文件"], 点["锚"], 条目们)
            if 条 is None and 点.get("条件"):
                条 = 条命中(点["文件"], 点["条件"], 条目们)
            if 条 is not None:
                红们.append({"种类": "新增位置缺协议注释", "文件": 点["文件"],
                           "行": 点["行"], "锚": 点["锚"][:120],
                           "说明": f"{面}：登记条目在但代码无协议注释——补"
                                 f"「// 静默豁免<类别>: 原因」"})
            else:
                红们.append({"种类": "新增" + 面, "文件": 点["文件"], "行": 点["行"],
                           "锚": 点["锚"][:120],
                           "说明": f"新增位置无登记（{面}）——清偿或走登记通道"
                                 f"（miss_registry.json 补条+代码协议注释）"})
            红数 += 1
        面统[面] = {"现测": len(点们), "基线": sum(基计.values()), "新增": len(新增),
                   "豁免放行": 放行数, "红": 红数, "消减": len(减)}
    # 陈旧（登记表有条而代码锚文本消失）
    陈旧 = []
    for 条 in 条目们:
        f = 条.get("文件", "")
        if f not in 扫描果["文件文本"]:
            陈旧.append({"种类": "陈旧", "文件": f, "行": 0, "锚": 条锚(条)[:120],
                       "说明": "登记条目指向的文件不存在——删条或恢复文件"})
            continue
        段们 = [段 for 段 in re.split(r"[│|]", 条.get("锚文本", "") or 条.get("锚", ""))
               if 归一(段)]
        for 段 in 段们:
            if 归一(段) not in 扫描果["文件文本"][f]:
                陈旧.append({"种类": "陈旧", "文件": f, "行": 0, "锚": 归一(段)[:120],
                           "说明": "登记条目锚文本在代码中消失——改代码须同步修表"
                                 "（改锚或删条）"})
                break
    红们.extend(陈旧)
    批准统 = {"已批": 0, "存量过渡": 0, "已毙": 0, "未批准": 0}
    for 条 in 条目们:
        批 = (条.get("批准") or "").strip()
        if 批 in 批准统:
            批准统[批] += 1
        else:
            批准统["未批准"] += 1
    return {"红": 红们, "豁免": 豁免们, "消减": 消减, "面统": 面统,
          "陈旧数": len(陈旧), "条目数": len(条目们),
          "协议点数": len(扫描果["协议点"]),
          "漏登记数": sum(1 for r in 红们 if r["种类"] == "漏登记"),
          "批准统": 批准统}


# ============ 八、报告输出 ============

def 打印批准档案(条目们: list):
    """1238 批准档案：按批准状态全列登记条（用户逐条审计面）。"""
    组 = {"已批": [], "存量过渡": [], "呈报中": [], "已毙": [], "未批准": []}
    for 条 in 条目们:
        批 = (条.get("批准") or "").strip()
        组.setdefault(批 if 批 in 组 else "未批准", []).append(条)
    print("== 兜底禁令批准档案（#1238·2026-10-20 用户立法）==")
    print(f"登记条 {len(条目们)} 条：已批 {len(组['已批'])} · 存量过渡 {len(组['存量过渡'])}"
          f" · 呈报中 {len(组['呈报中'])} · 已毙 {len(组['已毙'])} · 未批准 {len(组['未批准'])}")
    for 态 in ("已毙", "未批准", "呈报中", "存量过渡", "已批"):
        们 = 组[态]
        if not 们:
            continue
        print(f"\n【{态}】（{len(们)} 条）")
        for 条 in 们:
            print(f"  · {条.get('文件', '?')}::{条.get('函数', '?')}")
            print(f"      锚：{条锚(条)[:100]}")
            print(f"      类别：{条.get('类别', '?')} · 原因：{(条.get('原因') or '')[:90]}")



def 打印红们(红们: list):
    排序 = sorted(红们, key=lambda r: (r["种类"], r["文件"], r["行"]))
    print(f"\n【红清单】（{len(排序)} 条·L1 硬拦）")
    for r in 排序:
        位 = f"{r['文件']}:{r['行']}" if r.get("行") else r["文件"]
        print(f"  [×] ({r['种类']}) {位}")
        if r.get("锚"):
            print(f"      锚：{r['锚'][:110]}")
        print(f"      处置：{r['说明']}")


def 打印报告(扫描果: dict, 判定果: dict, 详列: bool):
    统计 = 扫描果["统计"]
    print("== 静默值登记制硬拦门禁（check_miss_gate.py·882·648 立法精确化）==")
    print(f"扫描面：src/ + CN语言编译器v2/（文件 {统计['文件']}·读取失败 {统计['读取失败']}）"
          f"；登记表条目 {判定果['条目数']} 条；协议注释点 {判定果['协议点数']} 处")
    批 = 判定果.get("批准统", {})
    if 批:
        print(f"【1238 批准制】已批 {批.get('已批', 0)} · 存量过渡 {批.get('存量过渡', 0)} · "
              f"已毙 {批.get('已毙', 0)} · 未批准 {批.get('未批准', 0)}"
              f"（未批准/已毙条=红通道·存量过渡=逐条呈报宽限·批完翻开关）")
    print("【面级统计】面 | 现测 | 基线 | 新增 | 豁免放行 | 红 | 消减")
    for 面 in 面名们:
        u = 判定果["面统"][面]
        print(f"  {面} | {u['现测']} | {u['基线']} | {u['新增']} | {u['豁免放行']} | "
              f"{u['红']} | {u['消减']}")
    for 面 in 面名们:
        减 = 判定果["消减"].get(面, [])
        if 减:
            print(f"  [↓] {面} 基线位置消失 {len(减)} 处（自动消减·确认无误报后跑 "
                  f"--update-baseline 收基线）：{减[:3]}…")
    if 判定果["豁免"]:
        print(f"\n【豁免放行】（{len(判定果['豁免'])} 条·协议注释+登记条目："
              f"登记制正常通道·正态）")
        for h in 判定果["豁免"][:10]:
            print(f"  [✓] {h['文件']}:{h['行']}（{h['面']}）{h['锚'][:80]}")
        if len(判定果["豁免"]) > 10:
            print(f"  …（其余 {len(判定果['豁免']) - 10} 条）")
    if 详列:
        print("\n【在测位置明细】（--report·全列）")
        for 面 in 面名们:
            print(f"-- {面}（{len(扫描果['面点'][面])}）--")
            for 点 in sorted(扫描果["面点"][面], key=lambda x: (x["文件"], x["行"])):
                print(f"  {点['文件']}:{点['行']}  {点['锚'][:100]}")
        print("\n【协议注释点】")
        for 协议 in 扫描果["协议点"]:
            print(f"  {协议['文件']}:{协议['行']} 锚定={协议['锚定行']} "
                  f"类别={协议['类别']} 已登记={协议['已登记']} 原因={协议['原因'][:60]}")
    if 判定果["红"]:
        打印红们(判定果["红"])
        print(f"\n[门禁·红] {len(判定果['红'])} 条——L1 硬拦（无逃生门 flag）；"
              f"唯一合法通道=登记表补条+代码协议注释，或原地清偿。")
    else:
        print("\n[门禁·绿] 四面零红（存量冻结只减不增·新增面严）")


def 转json(判定果: dict, 扫描果: dict) -> str:
    输出 = {"工具": "check_miss_gate.py", "任务": "882",
          "统计": 扫描果["统计"], "面统": 判定果["面统"],
          "条目数": 判定果["条目数"], "协议点数": 判定果["协议点数"],
          "豁免": 判定果["豁免"], "消减": {k: v[:50] for k, v in 判定果["消减"].items()},
          "红": 判定果["红"]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 九、基线生成 ============

def 写基线(路径: Path, 扫描果: dict) -> dict:
    面 = {名: sorted(建点键(点) for 点 in 扫描果["面点"][名]) for 名 in 面名们}
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=路径.parent.parent,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_miss_gate.py",
        "说明": ("882 静默门禁冻结基线（只减不增）：全库各面位置快照（键=「文件::归一锚文本」）。"
               "位置存在于基线=放行（存量）；新增位置=红（除非有协议注释+登记条目）；"
               "位置消失=自动消减提示（确认无误报后跑 --update-baseline 收基线）。"
               "存量清偿挂 #950/#891 逐批清。手改基线=绕过门禁行为·禁。"),
        "生成sha": sha,
        "面": 面,
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1), encoding="utf-8")
    return {名: len(面[名]) for 名 in 面名们}


# ============ 十、自测（正反两态+三红各一例·零触碰真实树） ============

样例静默 = """// 静默样例：查表 miss 返默认值（新增即红反态）
int 查宽(表& t, 字符串 k) {
    auto it = t.find(k);
    if (it == t.end()) return 0;
    return it->second;
}
"""

样例合格 = """// 合格样例：业务形态（无红旗注释·无静默豁免需求）
int 求和(int a, int b) {
    int 和 = a + b;
    return 和;
}
"""


def 自测建场景(保活: list, 文件们: dict, 条目们=None):
    """独立临时树 + 以当前实测生成基线。返回 (根, 表, 基)。"""
    td = tempfile.TemporaryDirectory(prefix="cmg882_")
    保活.append(td)
    根 = Path(td.name)
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")
    表 = 根 / "registry.json"
    自测写表(表, 条目们 or [])
    基 = 根 / "baseline.json"
    写基线(基, 全库扫描(根))
    return 根, 表, 基


def 自测写表(表: Path, 条目们: list):
    表.write_text(json.dumps({"版本": 1, "条目": 条目们}, ensure_ascii=False),
                 encoding="utf-8")


def 自测跑(根: Path, 表: Path, 基: Path):
    """临时树门禁调用：返回 (rc, 判定果, 扫描果)。"""
    扫描果 = 全库扫描(根)
    条目们, _态 = 核950.读登记表(表)
    果 = 判定(扫描果, 条目们, 读基线(基))
    return (1 if 果["红"] else 0), 果, 扫描果


def 自测场景一(断言, 保活):
    """场景一：静默形态基线与四红演化（同一棵树顺序演化）。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/样例.cpp": 样例静默, "src/合格.cpp": 样例合格,
        "CN语言编译器v2/源.cn":
        "// v2 样例\n函数 取宽(字符串 k) -> 整64 {\n    返回 字符串长度(k);\n}\n"})
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"基线生成后存量放行=绿: 红={果['红']!r}")
    断言(果["面统"]["静默形态"]["现测"] >= 1, "静默形态现测≥1（扫描面健康度）")
    样例 = 根 / "src" / "样例.cpp"
    查高头 = ("int 查高(表& t, 字符串 k) {\n"
            "    auto it = t.find(k);\n"
            "    if (it == t.end()) return 8;%s\n"
            "    return it->second;\n"
            "}\n")
    样例.write_text(样例静默 + (查高头 % ""), encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增静默形态" in 红种们(果), f"反态①新位置无登记默认值真拦: {红种们(果)}")
    样例.write_text(样例静默 + (查高头 % "  // 静默豁免业务值: 自测"), encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "漏登记" in 红种们(果), f"反态②红·漏登记（注释无条）: {红种们(果)}")
    # 1238 批准制反态：登记条缺「批准」字段=红「登记无批准」（新兜底先呈报后编码）
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "查高",
                "锚文本": "if (it == t.end()) return 8;", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测登记", "登记人": "selftest"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "登记无批准" in 红种们(果), f"反态⑥红·登记无批准（1238）: {红种们(果)}")
    # 存量过渡=宽限绿
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "查高",
                "锚文本": "if (it == t.end()) return 8;", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测登记", "登记人": "selftest",
                "批准": "存量过渡"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态①协议注释+登记条（存量过渡）=放行: 红={果['红']!r}")
    断言(果["批准统"]["存量过渡"] == 1, "批准统计含存量过渡")
    # 1238 批准制反态：已毙而锚仍在=红「已毙未清偿」
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "查高",
                "锚文本": "if (it == t.end()) return 8;", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测登记", "登记人": "selftest",
                "批准": "已毙"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "已毙未清偿" in 红种们(果), f"反态⑦红·已毙未清偿（1238）: {红种们(果)}")
    # 已批=绿
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "查高",
                "锚文本": "if (it == t.end()) return 8;", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测登记", "登记人": "selftest",
                "批准": "已批"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态③批准=已批=放行: 红={果['红']!r}")
    断言(any(h["面"] == "静默形态" for h in 果["豁免"]), "豁免放行列表含静默形态")
    自测写表(表, [{"文件": "src/样例.cpp", "函数": "查高",
                "锚文本": "if (it == t.end()) return 999;", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测陈旧", "登记人": "selftest"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "陈旧" in 红种们(果), f"反态③红·陈旧（锚消失）: {红种们(果)}")
    自测写表(表, [])
    样例.write_text(样例静默 + ("int 兜底(表& t, 字符串 k) {\n"
                            "    // 查不到按 0 处理（兜底默认）\n"
                            "    return 0;\n"
                            "}\n"), encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言("新增红旗词面" in 红种们(果), f"反态④红·红旗词面无登记: {红种们(果)}")
    样例.write_text(样例合格, encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0 and 果["消减"]["静默形态"], f"正态②基线位置消失=自动消减不红: {果['消减']}")


def 红种们(果) -> set:
    return {r["种类"] for r in 果["红"]}


def 自测场景结果化(断言, 保活):
    """场景结果化（1360）：结果/可选 消费点惯用法=豁免绿·裸哨兵新位置仍真拦。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/样例.cpp": 样例合格, "CN语言编译器v2/源.cn": "// v2 占位\n"})
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"基线绿: 红={果['红']!r}")
    样例 = 根 / "src" / "结果化.cpp"
    样例1360 = """std::optional<int> 取长(const std::string& s) {
    if (s.empty()) return std::nullopt;
    return (int)s.size();
}
int 用长(const std::string& s) {
    auto len = 取长(s);
    if (!len.has_value()) return false;
    if (len.value() <= 0) return 0;
    return len.value();
}
std::string 兜(std::optional<int> o) {
    std::string out;
    if (o.has_value()) { out = "y"; } else { out = std::to_string(o.value_or(0)); }
    return out;
}"""
    样例.write_text(样例1360, encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"1360 结果化消费点=豁免绿: 红={果['红']!r}")
    样例.write_text(样例1360 + """int 查宽2(表& t, std::string k) {
    auto it = t.find(k);
    if (it == t.end()) return 0;
    return it->second;
}""", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增静默形态" in 红种们(果), f"1360 反态·裸哨兵新位置仍真拦: {红种们(果)}")


def 自测场景二(断言, 保活):
    """场景二：else 块业务语句（用户 else 新规）与 648 报诊断收尾模式放行。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/合格.cpp": 样例合格,
        "src/else样.cpp": "int 分档(int x) {\n    if (x > 0) { return 1; } else { x = 2; }\n    return x;\n}\n"})
    新 = 根 / "src" / "else样新.cpp"
    新.write_text("int 校验(int x) {\n    if (x > 0) { return 1; } else { 打印结果(x); }\n"
                "    return 0;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增else业务" in 红种们(果), f"反态⑤else 块业务语句=红: {红种们(果)}")
    新.write_text("int 校验(int x, 诊断器& 诊断) {\n    if (x > 0) { return 1; } else {\n"
                "        诊断.report(Error, \"非法输入\");  return 0;\n    }\n"
                "    return 0;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言("新增else业务" not in 红种们(果), f"正态③报诊断+返回收尾模式放行: {红种们(果)}")


def 自测场景三(断言, 保活):
    """场景三：单边开区间（用户裁决二）与豁免通道、双边界闭区间。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/区间样.cpp": "int 评级(int 分数) {\n    if (分数 >= 90) { return 1; }\n    return 0;\n}\n"})
    样 = 根 / "src" / "区间样.cpp"
    样.write_text("int 评级(int 分数) {\n    if (分数 >= 95) { return 1; }\n    return 0;\n}\n",
                encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增单边比较" in 红种们(果), f"反态⑥单边开区间=红: {红种们(果)}")
    样.write_text("int 评级(int 分数) {\n    if (分数 >= 95) { return 1; }"
                "  // 静默豁免业务值: 自测区间\n    return 0;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "漏登记" in 红种们(果), f"反态⑦单边区间注释无表条=漏登记: {红种们(果)}")
    自测写表(表, [{"文件": "src/区间样.cpp", "函数": "评级",
                "锚文本": "分数 >= 95", "来源": "882selftest",
                "类别": "业务值", "静默风险": "低", "原因": "自测区间豁免", "登记人": "selftest",
                "批准": "存量过渡"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态④单边区间 协议注释+条件文本锚条=放行: {果['红']!r}")
    根4, 表4, 基4 = 自测建场景(保活, {
        "src/双边界.cpp":
        "int 评级(int 分数) {\n    if (分数 >= 90 && 分数 <= 100) { return 1; }\n    return 0;\n}\n"})
    rc, 果, _s = 自测跑(根4, 表4, 基4)
    断言(rc == 0, f"正态⑤双边界闭区间不红: {红种们(果)}")


def 自测场景四(断言, 保活):
    """场景四：协议注释格式强制（类别枚举）+ 基线缺失拒跑（防绕过）。"""
    根5, 表5, _基5 = 自测建场景(保活, {
        "src/类别样.cpp":
        "int 空串长(const char* s, int n) {\n    if (s == nullptr) return 0;\n    return n;\n}\n"})
    (根5 / "src" / "类别样.cpp").write_text(
        "int 空串长(const char* s, int n) {\n    if (s == nullptr) return 0;\n"
        "    if (!s) return 0;  // 静默豁免随便写: 类别不在枚举检查行\n    return n;\n}\n",
        encoding="utf-8")
    rc, 果, _s = 自测跑(根5, 表5, 根5 / "baseline.json")
    断言("协议注释类别非法" in 红种们(果), f"反态⑧协议注释类别非法=红: {红种们(果)}")
    try:
        读基线(根5 / "不存在.json")
        断言(False, "基线缺失应拒跑")
    except SystemExit:
        断言(True, "基线缺失=拒跑（防绕过）")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    保活 = []                            # TemporaryDirectory 保活（防提前清理）
    for 场景 in (自测场景一, 自测场景结果化, 自测场景二, 自测场景三, 自测场景四):
        try:
            场景(断言, 保活)
        except Exception as e:           # 场景异常=自测失败（显形不静默）
            失败.append(f"{场景.__name__} 异常：{e!r}")
    if 失败:
        print("== 门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 门禁自测全过（{通过[0]} 项断言·反态真拦/正态放行/三红各一例齐备）==")
    return 0


# ============ 十一、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="静默值登记制硬拦门禁（882·三红+基线冻结·L1 硬拦）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--registry", default=str(登记表文件), help="登记表路径")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（全列·rc=0 不拦）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--update-baseline", action="store_true",
                      help="以当前实测重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    解析器.add_argument("--approval", action="store_true",
                        help="批准档案模式（1238：按批准状态全列登记条·rc=0）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    if 参数.approval:
        条目们, 表态 = 核950.读登记表(Path(参数.registry))
        if 表态 != "就绪":
            print(f"[红] 登记表状态异常：{表态}（{参数.registry}）")
            return 1
        打印批准档案(条目们)
        return 0
    扫描果 = 全库扫描(根)
    if 参数.update_baseline:
        统计 = 写基线(Path(参数.baseline), 扫描果)
        print(f"[基线] 已重写 {Path(参数.baseline).name}："
              + " · ".join(f"{面} {统计[面]}" for 面 in 面名们))
        return 0
    条目们, 表态 = 核950.读登记表(Path(参数.registry))
    if 表态 != "就绪":
        print(f"[门禁·红] 登记表状态异常：{表态}（{参数.registry}）")
        return 1
    基路径 = Path(参数.baseline)
    if 参数.report and not 基路径.exists():
        基线 = {面: Counter() for 面 in 面名们}
        print(f"[提示] 基线缺失（{基路径.name}）——报告模式按空基线全量展示（不拦）")
    else:
        基线 = 读基线(基路径)
    判定果 = 判定(扫描果, 条目们, 基线)

    if 参数.json:
        print(转json(判定果, 扫描果))
    else:
        打印报告(扫描果, 判定果, 详列=参数.report)
    if 参数.report:
        return 0
    return 1 if 判定果["红"] else 0


if __name__ == "__main__":
    sys.exit(main())
