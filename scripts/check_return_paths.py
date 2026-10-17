#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_return_paths.py —— 函数返回路径全量审计（968·「按 return 语句全量枚举」）。

背景（用户 10-17 令·语法形态抓捕的反面）：此前 15 处缺陷抓捕按语法形态零散抓
（switch/else/查表），共同本质=**函数在失败/未知路径返回正常类型值**——检查单位
应为每条 return。本工具把「全量枚举两套编译器所有函数的全部 return/返回 + 按语境
分级」机械化为嫌疑池，供逐条人判三选一（业务值→miss_registry.json 登记豁免 /
清偿为报错确定性失败 / 改结果化 API）。

扫描面：宿主 src/**/*.cpp,hpp 与 v2 CN语言编译器v2/**/*.cn（两套编译器同规·词法共用）。
实现=注释/字符串感知词法器 + 大括号块栈 + 函数头启发式（跨行参数表拼接识别）——
**不建完整语法树**（诚实边界②）。

分级（任务书 968 规则·档=线索优先级非缺陷定论）：
  高 = 返字面量 ∧（else/elseif 块内 ∨ 错误分支内 ∨ 查表循环后）——错误分支=if 条件含
       错误信号（nullptr/取反/empty/end/miss/未找到/失败/== -1/< 0/>= size()）；
  中 = 返字面量 ∧（catch 块内 ∨ 失败标签后——label 名含 fail/err/失败/错误）；
  低 = 其余（非字面量 return、唯一 return、递归终止候选、普通业务分支=多为业务值）。
  子标注：唯一 return 函数 / 递归终止候选（函数内自调用 + return 在 if 块内）/ 空 return。
统计口径（与 968 基线 118e17d4 对照）：词法级 return/返回 语句（注释与字符串内的字面
出现不计）；「返常量」=单字面量（数字/布尔/空指针/真/假——字符串字面量单列）。
基线：宿主 3678 return/1613 返常量·v2 3704 返回/1617 返常量（各约 44%）。

诚实边界（不假装全覆盖·不判死）：
  ① 语境分级=机械共现（else 块/错误分支/循环后），不证明该 return 真被静默消费——
     处置判据须人读调用链（010 系教训：文案与根因偏离）。② 函数边界=启发式——宏体/
     初始化列表带嵌套大括号的构造等极端形态可漏（游离 return 计数=扫描面健康度指示，
     >0 即提示漏检）；函数名/条件为近似文本，仅供分组。③ 查表循环后不区分循环是否
     真的消费查表结果；错误信号词面不覆盖全部语义。④ `return 0` 的语义（业务值/默认值）
     机器不可判——这正是本清单纯人判的原因。⑤ 本工具只读（登记动作走
     scripts/miss_registry.json·来源字段=968）。
与 #950 合流：两侧输出同一「文件/行/函数/档/理由/登记」六字段；本件=「检查单位=每条
return」的宽口径（950 是「miss 判定形态」的窄口径）。清单文档 docs/返回路径嫌疑池-968.md。

用法：
  python3 scripts/check_return_paths.py                # 双侧全量审计（报告式·rc=0）
  python3 scripts/check_return_paths.py --only 宿主    # 只审宿主（v2 同理）
  python3 scripts/check_return_paths.py --limit 60     # 清单截断（默认 40·0=全列）
  python3 scripts/check_return_paths.py --json         # 机读（stdout）
  python3 scripts/check_return_paths.py --md [文件]    # md 输出（缺省 stdout）
  python3 scripts/check_return_paths.py --strict       # 高>0 → rc=1（专项轮可选）
  python3 scripts/check_return_paths.py --selftest     # 自测（临时样例树正反两态）

退出码：0=报告完成（--strict 且高>0 → 1）；2=用法/环境错误。
"""

# 兼容 python3.8
from __future__ import annotations

import argparse
import bisect
import io
import json
import re
import sys
import tempfile
from pathlib import Path

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

仓库根 = Path(__file__).resolve().parent.parent
登记表路径 = 仓库根 / "scripts" / "miss_registry.json"

# ============ 常量 ============

标识符正则 = re.compile(r"[A-Za-z_\u4e00-\u9fff][A-Za-z0-9_\u4e00-\u9fff]*")
数字正则 = re.compile(r"[0-9][0-9A-Za-z_\.]*")
# 多字符运算符（最长优先）
多字符符 = ("<<=", ">>=", "...", "::", "->", "==", "!=", "<=", ">=", "&&", "||",
         "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>")
单字符符 = set("+-*/%<>=!&|^~?:.,;()[]{}")

控制词 = {"if", "while", "for", "switch", "catch", "do", "else", "return", "sizeof",
        "decltype", "template", "throw", "new", "delete", "case", "default",
        "try", "namespace", "struct", "class", "enum", "union", "extern"}
v2控制词 = {"如果", "否则", "当", "循环", "遍历", "选择", "情况", "默认", "返回",
          "中断", "继续"}
宿主返回词 = "return"
v2返回词 = "返回"
返回词们 = (宿主返回词, v2返回词)

# 错误信号（if 条件文本内匹配·错误分支判定）
错误信号 = re.compile(
    r"==\s*nullptr|!\s*[\w\u4e00-\u9fff(]|\.empty\s*\(\s*\)|\.end\s*\(\s*\)"
    r"|==\s*-1\b|<\s*0\b|>=\s*[\w.]+\.size\s*\(|>=\s*[\w.]+\.大小\s*\(|miss\b"
    r"|未找到|找不到|查不到|未命中|失败|错误|空指针|!=.*(?:正常|有值)")

# 查表调用（循环体内判定）
查表调用 = re.compile(
    r"(?<![A-Za-z0-9_])[A-Za-z_\u4e00-\u9fff]*(?:find|lookup|search|查|查询)"
    r"[A-Za-z0-9_\u4e00-\u9fff]*(?=\s*\()")

# 失败标签名特征
失败标签 = re.compile(r"fail|err|失败|错误|异常")
标签排除词 = {"case", "default", "public", "private", "protected", "else", "情况",
           "默认", "公开", "私有", "保护"}

字面量布尔 = {"true", "false", "nullptr", "真", "假", "空指针"}


# ============ 一、词法器（注释/字符串感知） ============

def 词法(文本: str):
    """切 token；返回 [(文本, 行号)]。注释跳过（行号连续维护）；字符串/字符整体成 token。"""
    tokens = []
    行号 = 1
    i = 0
    n = len(文本)
    while i < n:
        c = 文本[i]
        if c == "\n":
            行号 += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            if j < 0:
                break
            i = j
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            if j < 0:
                行号 += 文本.count("\n", i)
                break
            行号 += 文本.count("\n", i, j)
            i = j + 2
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if 文本[j] == "\\":
                    j += 2
                    continue
                if 文本[j] == '"':
                    j += 1
                    break
                j += 1
            tokens.append((文本[i:min(j, n)], 行号))
            行号 += 文本.count("\n", i, j)
            i = j
            continue
        if c == "'":
            j = i + 1
            while j < n:
                if 文本[j] == "\\":
                    j += 2
                    continue
                if 文本[j] == "'":
                    j += 1
                    break
                if 文本[j] == "\n":
                    break
                j += 1
            if 0 < j - i <= 8:
                tokens.append((文本[i:j], 行号))
                i = j
                continue
            tokens.append(("'", 行号))
            i += 1
            continue
        m = 标识符正则.match(文本, i)
        if m and (c.isascii() or ord(c) >= 0x4e00):
            tokens.append((m.group(0), 行号))
            i = m.end()
            continue
        m = 数字正则.match(文本, i)
        if m:
            tokens.append((m.group(0), 行号))
            i = m.end()
            continue
        命中 = None
        for op in 多字符符:
            if 文本.startswith(op, i):
                命中 = op
                break
        if 命中:
            tokens.append((命中, 行号))
            i += len(命中)
        elif c in 单字符符:
            tokens.append((c, 行号))
            i += 1
        else:
            i += 1  # 未知字节（容错跳过）
    return tokens


def 是标识符(t: str) -> bool:
    if not t:
        return False
    c = t[0]
    return (c.isascii() and (c.isalpha() or c == "_")) or ord(c) >= 0x4e00


def 是数字(t: str) -> bool:
    return bool(t) and t[0].isdigit()


# ============ 二、配对与块分类 ============

def 建配对(tokens: list):
    """括号配对表：下标 → 配对下标（()/[]/{} 三类·失配容错丢弃）。"""
    栈 = []
    配对 = {}
    for i, (t, _行) in enumerate(tokens):
        if t in ("(", "[", "{"):
            栈.append((t, i))
        elif t in (")", "]", "}"):
            期望 = {")": "(", "]": "[", "}": "{"}[t]
            if 栈 and 栈[-1][0] == 期望:
                _o, oi = 栈.pop()
                配对[oi] = i
                配对[i] = oi
    return 配对


def 回看头段(tokens: list, 配对表: dict, i: int):
    """从 `{`（下标 i）向前扫头段；返回 (flat, 起点)。

    flat=近→远序列：('组', 开, 闭) 或 ('顶', 文本)；遇 `;`/`{`/`}` 停。
    嵌套组整体跳过（方括号组也跳·不影响判定）。"""
    flat = []
    j = i - 1
    while j >= 0:
        t = tokens[j][0]
        if t in (")", "]"):
            p = 配对表.get(j)
            if p is None:
                j -= 1
                continue
            if t == ")":
                flat.append(("组", p, j))
            j = p - 1
            continue
        if t in (";", "{", "}"):
            break
        flat.append(("顶", t))
        j -= 1
    return flat, j + 1


def 取名字(tokens: list, 配对表: dict, 开: int) -> str:
    """参数组开括号（下标 开）左侧取函数/控制词名。

    跳过模板实参区；operator 重载（()/[]/符号）特判返回 "operator"。"""
    p = 开 - 1
    while p >= 0:
        t = tokens[p][0]
        if 是标识符(t):
            return t
        if t == ">":
            深度 = 1
            p -= 1
            while p >= 0 and 深度:
                if tokens[p][0] == ">":
                    深度 += 1
                elif tokens[p][0] == "<":
                    深度 -= 1
                p -= 1
            continue
        if t in ("::", "~", "<"):
            p -= 1
            continue
        if t == ")":
            # operator()(...) 形态：`)` 配对到 `(`，再前是 operator
            q = 配对表.get(p)
            if q is not None and q - 1 >= 0 and tokens[q - 1][0] == "operator":
                return "operator"
            return ""
        if t == "]":
            # operator[](...) 形态：`]` 配对到 `[`，再前是 operator
            q = 配对表.get(p)
            if q is not None and q - 1 >= 0 and tokens[q - 1][0] == "operator":
                return "operator"
            return ""
        if p - 1 >= 0 and tokens[p - 1][0] == "operator":
            # operator== / operator= / operator+ 等符号形态
            return "operator"
        return ""
    return ""


def 分类块(tokens: list, 配对表: dict, i: int):
    """`{`（下标 i）的块分类：字典 {类型, 名, 条件}。

    类型：函数/if/elseif/else/循环/switch/catch/try/普通。
    规则（诚实边界②）：头段回看——顶层 `:` 仅当「近侧为初始化元素段」时才是初始化
    列表冒号（`公开:`/`情况 1:` 等不吞组）；参数组=冒号远侧最近的组（无冒号=最近的组），
    名字=参数组开括号前一个标识符（跳过模板实参）。"""
    flat, _起 = 回看头段(tokens, 配对表, i)
    # 初始化列表冒号判定：近侧（`:` 到 `{` 之间的顶层 token）须是初始化元素段形态——
    # ①仅含 组/标识符/,/::/</>/*/& ②不得有「相邻双标识符」（`public: void f(..)` /
    # `公开: 函数 f(..)` 这类声明头近侧必现相邻标识符，即被排除·不吞参数组）
    冒号位 = None
    for k, 项 in enumerate(flat):
        if 项[0] == "顶" and 项[1] == ":":
            近侧 = flat[:k]
            合法 = bool(近侧)
            前是标识符 = False
            for 项2 in 近侧:
                if 项2[0] == "组":
                    前是标识符 = False
                    continue
                t2 = 项2[1]
                if t2 in (",", "::", "<", ">", "*", "&"):
                    前是标识符 = False
                    continue
                if 是标识符(t2):
                    if 前是标识符:
                        合法 = False
                        break
                    前是标识符 = True
                    continue
                合法 = False
                break
            if 合法:
                冒号位 = k
    # 参数组 = 冒号远侧（或全部）里最近的组；无组 → do/try/else/普通
    组索引 = [k for k, 项 in enumerate(flat)
             if 项[0] == "组" and (冒号位 is None or k > 冒号位)]
    if not 组索引:
        顶们 = [项[1] for 项 in flat if 项[0] == "顶"]
        if "do" in 顶们:
            return {"类型": "循环", "名": "do", "条件": ""}
        if "try" in 顶们:
            return {"类型": "try", "名": "try", "条件": ""}
        if "else" in 顶们 or "否则" in 顶们:
            return {"类型": "else", "名": "else", "条件": ""}
        return {"类型": "普通", "名": "", "条件": ""}
    _k, (_, 开, 闭) = min(((k, 项) for k, 项 in enumerate(flat)
                         if 项[0] == "组" and (冒号位 is None or k > 冒号位)),
                        key=lambda kv: kv[0])
    名 = 取名字(tokens, 配对表, 开)
    条件文本 = "".join(tokens[t][0] for t in range(开 + 1, 闭))
    if 名 in 控制词 or 名 in v2控制词:
        if 名 in ("if", "如果"):
            顶们 = [项[1] for 项 in flat if 项[0] == "顶"]
            类 = "elseif" if ("else" in 顶们 or "否则" in 顶们) else "if"
            return {"类型": 类, "名": 名, "条件": 条件文本}
        if 名 in ("while", "for", "do", "当", "循环", "遍历"):
            return {"类型": "循环", "名": 名, "条件": 条件文本}
        if 名 in ("switch", "选择"):
            return {"类型": "switch", "名": 名, "条件": 条件文本}
        if 名 == "catch":
            return {"类型": "catch", "名": 名, "条件": 条件文本}
        return {"类型": "普通", "名": 名, "条件": ""}
    if not 名:
        return {"类型": "普通", "名": "", "条件": ""}
    return {"类型": "函数", "名": 名, "条件": ""}


def 收集返回值(tokens: list, 配对表: dict, i: int):
    """i=return/返回 token；返回 (值 token 列表, 结束下标)。

    值=return 后到深度 0 的 `;`（() [] 深度核算；`{...}` 整体跳过）；空 return=[]。"""
    值 = []
    深度 = 0
    j = i + 1
    n = len(tokens)
    while j < n:
        t = tokens[j][0]
        if t in ("(", "["):
            深度 += 1
        elif t in (")", "]"):
            深度 -= 1
            if 深度 < 0:
                return [], j   # 未遇分号（容错）
        elif t == "{":
            配对j = 配对表.get(j)
            if 配对j is not None:
                值.append(tokens[j])
                j = 配对j + 1
                continue
            深度 += 1
        elif t == "}":
            深度 -= 1
            if 深度 < 0:
                return [], j
        elif t == ";" and 深度 == 0:
            return 值, j
        值.append(tokens[j])
        j += 1
    return 值, n


# ============ 三、函数与 return 枚举 ============

def 枚举(tokens: list, 配对表: dict):
    """块栈一遍扫描：返回 (函数们, 返回们, 游离return数, 循环块们)。

    函数们=[{名,简名,起,止,返回数,自调用}], 返回们=[{位置,行,值tok们,字面量,块链,条件们,函数}],
    循环块们=[{起,止,函数,含查表}]。"""
    栈 = []
    函数们 = []
    返回们 = []
    循环们 = []
    游离 = 0
    for i, (t, 行) in enumerate(tokens):
        if t == "{":
            信息 = 分类块(tokens, 配对表, i)
            函数 = None
            for 块 in reversed(栈):
                if 块["类型"] == "函数":
                    函数 = 块
                    break
            块 = {"类型": 信息["类型"], "名": 信息["名"], "条件": 信息["条件"],
                 "起": i, "止": None, "函数": 函数}
            栈.append(块)
            if 信息["类型"] == "函数":
                块["简名"] = 信息["名"].split("::")[-1] if "::" in 信息["名"] else 信息["名"]
                块["返回数"] = 0
                块["自调用"] = False
                函数们.append(块)
            elif 信息["类型"] == "循环":
                循环们.append(块)
            continue
        if t == "}":
            if 栈:
                块 = 栈.pop()
                块["止"] = i
            continue
        if t in 返回词们:
            # 归属函数
            函数 = None
            块链 = []
            for 块 in reversed(栈):
                块链.append(块)
                if 函数 is None and 块["类型"] == "函数":
                    函数 = 块
            if 函数 is None:
                游离 += 1
                continue
            值, 结束 = 收集返回值(tokens, 配对表, i)
            字面量 = 判字面量(值)
            条件们 = []
            for 块 in 块链[:-1] if 块链 and 块链[-1]["类型"] == "函数" else 块链:
                if 块["类型"] in ("if", "elseif"):
                    条件们.append(块["条件"])
            # 无块单语句 if：`if (...) return ...;`（return 前是 `)`）
            if i > 0 and tokens[i - 1][0] == ")":
                p = 配对表.get(i - 1)
                if p is not None and p - 1 >= 0 and tokens[p - 1][0] in ("if", "如果"):
                    条件们.append("".join(tokens[k][0] for k in range(p + 1, i - 1)))
            返回们.append({"位置": i, "行": 行, "值tok们": 值, "字面量": 字面量,
                        "块链": 块链, "条件们": 条件们, "函数": 函数})
            函数["返回数"] += 1
            continue
        # 自调用检测（函数体内：标识符==函数简名 且后随 '('）
        if 是标识符(t) and 栈:
            函数 = None
            for 块 in reversed(栈):
                if 块["类型"] == "函数":
                    函数 = 块
                    break
            if 函数 and t == 函数.get("简名") and 函数["起"] < i \
                    and i + 1 < len(tokens) and tokens[i + 1][0] == "(":
                函数["自调用"] = True
    # 循环块含查表判定
    for 循环 in 循环们:
        止 = 循环["止"] if 循环["止"] is not None else len(tokens) - 1
        段 = "".join(tokens[k][0] + " " for k in range(循环["起"], 止 + 1))
        循环["含查表"] = bool(查表调用.search(段))
    # 函数块补止与简名
    for 函数 in 函数们:
        if 函数["止"] is None:
            函数["止"] = len(tokens) - 1
        if "简名" not in 函数:
            函数["简名"] = 函数["名"]
    return 函数们, 返回们, 游离, 循环们


def 判字面量(值: list):
    """值 token 列表 → 字面量类别（'数字'/'字符串'/'字符'/'布尔空'/'') 。"""
    if not 值:
        return ""
    t0 = 值[0][0]
    if len(值) == 1:
        if 是数字(t0):
            return "数字"
        if t0.startswith('"'):
            return "字符串"
        if t0.startswith("'"):
            return "字符"
        if t0 in 字面量布尔:
            return "布尔空"
        return ""
    if len(值) == 2 and t0 in ("-", "+") and 是数字(值[1][0]):
        return "数字"
    return ""


# ============ 四、语境分级 ============

def 找失败标签(tokens: list, 函数: dict):
    """函数体内失败标签位置列表（label 名含 fail/err/失败/错误）。"""
    位们 = []
    for i in range(函数["起"], min(函数["止"] + 1, len(tokens))):
        t = tokens[i][0]
        if not 是标识符(t) or t in 标签排除词:
            continue
        if i + 1 < len(tokens) and tokens[i + 1][0] == ":":
            前 = tokens[i - 1][0] if i > 0 else ""
            if 前 == ":" or 前 == "::":
                continue
            if 失败标签.search(t):
                位们.append(i)
    return 位们


def 分级(返回们: list, 函数们: list, 循环们: list, tokens: list):
    """逐条 return 定档（高/中/低）+ 子标注（原地写入 返回们）。"""
    for r in 返回们:
        if r["函数"] is not None and "失败标签们" not in r["函数"]:
            r["函数"]["失败标签们"] = 找失败标签(tokens, r["函数"])
        块链 = [b["类型"] for b in r["块链"]]
        在else = any(b in ("else", "elseif") for b in 块链)
        在错误分支 = any(错误信号.search(c) for c in r["条件们"])
        在catch = "catch" in 块链
        在try = "try" in 块链
        # 无块 else：`else return 0;`（return 前是 else）
        位置 = r["位置"]
        if 位置 > 0 and tokens[位置 - 1][0] == "else":
            在else = True
        if 位置 > 0 and tokens[位置 - 1][0] == "否则":
            在else = True
        # 查表循环后（函数内·循环止 < return 位置）
        循环后 = False
        for 循环 in 循环们:
            if 循环.get("函数") is r["函数"] and 循环.get("含查表") \
                    and 循环["止"] is not None and 循环["止"] < 位置:
                循环后 = True
                break
        # 失败标签后
        标签后 = False
        for 位置2 in r["函数"]["失败标签们"]:
            if 位置2 < 位置:
                标签后 = True
                break
        字面量 = r["字面量"]
        理由 = []
        if 在else:
            理由.append("else 块内")
        if 在错误分支:
            理由.append("错误分支（条件=" + "|".join(c[:40] for c in r["条件们"] if 错误信号.search(c))[:60] + "）")
        if 循环后:
            理由.append("查表循环后")
        if 在catch:
            理由.append("catch 块内")
        if 在try:
            理由.append("try 块内")
        if 标签后:
            理由.append("失败标签后")
        if not 理由:
            理由.append("无失败语境特征")
        if 字面量:
            理由.append("返字面量=" + 字面量)
        else:
            理由.append("非字面量返回")
        子 = []
        if r["函数"]["返回数"] == 1:
            子.append("唯一 return")
        if 在if块 := any(b in ("if", "elseif") for b in 块链):
            if r["函数"].get("自调用"):
                子.append("递归终止候选")
        if not 字面量:
            子.append("空返回" if not r["值tok们"] else "")
        子 = [x for x in 子 if x]
        if 字面量:
            if 在else or 在错误分支 or 循环后:
                档 = "高"
            elif 在catch or 标签后:
                档 = "中"
            else:
                档 = "低"
        else:
            档 = "低"
        r["档"] = 档
        r["理由"] = 理由
        r["子标注"] = 子
    return None


# ============ 五、扫描整侧 ============

def 扫描一面(根: Path, 后缀集, 显示前缀: str, 条目们: list):
    """扫描一侧。返回 (文件数, 失败, 返回们, 统计)。"""
    文件数 = 0
    失败 = 0
    全部返回 = []
    函数总数 = 0
    游离总 = 0
    对照基线 = {"有值": 0, "空": 0, "数字": 0, "字符串": 0, "字符": 0, "布尔空": 0}
    for p in sorted(根.rglob("*")):
        if not p.is_file() or p.suffix not in 后缀集:
            continue
        try:
            文本 = p.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            失败 += 1
            continue
        相对 = 显示前缀 + "/" + p.relative_to(根).as_posix()
        文件数 += 1
        源行们 = 文本.split("\n")
        tokens = 词法(文本)
        配对表 = 建配对(tokens)
        函数们, 返回们, 游离, 循环们 = 枚举(tokens, 配对表)
        函数总数 += len(函数们)
        游离总 += 游离
        分级(返回们, 函数们, 循环们, tokens)
        for r in 返回们:
            r["文件"] = 相对
            r["函数名"] = "≈" + (r["函数"]["名"] or "?")
            r["锚"] = "".join(t[0] for t in r["值tok们"])[:80] or "（空返回）"
            r["行文本"] = 源行们[r["行"] - 1].strip()[:160] if 0 < r["行"] <= len(源行们) else ""
            r["值类"] = r["字面量"]
            if r["值tok们"]:
                对照基线["有值"] += 1
                if r["字面量"]:
                    对照基线[r["字面量"]] += 1
            else:
                对照基线["空"] += 1
            条 = 登记命中(r, 条目们)
            r["登记"] = {"类别": 条.get("类别", ""), "静默风险": 条.get("静默风险", "")} \
                if 条 else None
        全部返回.extend(返回们)
    return 文件数, 失败, 全部返回, {
        "函数总数": 函数总数, "游离return": 游离总, "对照基线": 对照基线}


def 归一锚(文本: str) -> str:
    return re.sub(r"\s+", "", 文本)


def 登记命中(r: dict, 条目们: list):
    """候选×登记条目对账：同文件且（行文本锚互为子串 ∨ 函数名等价）→ 返回条目。

    行文本锚=return 所在源码行文本（候选锚=返回值文本过短无鉴别力·不作对账面）；
    函数名等价=去 ≈ 与类名前缀后精确相等（防 varSlotOf/varSlotOffset 前缀误配）。"""
    行文本 = 归一锚(r.get("行文本", ""))
    简名 = (r.get("函数名", "") or "").lstrip("≈").split("::")[-1]
    for 条 in 条目们:
        if 条.get("文件") != r["文件"]:
            continue
        条锚 = 归一锚(条.get("锚文本", "") or 条.get("锚", ""))
        if 条锚 and 条锚 in 行文本:
            return 条
        条简 = (条.get("函数", "") or "").split("::")[-1]
        if 简名 and 条简 == 简名:
            return 条
    return None


def 读登记表(路径: Path):
    if not 路径.exists():
        return [], "缺失"
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        return [], f"损坏({e.__class__.__name__})"
    if not isinstance(数据, dict):
        return [], "损坏(非对象)"
    return 数据.get("条目", []), "就绪"


# ============ 六、报告 ============

def 全库扫描(要宿主: bool, 要v2: bool):
    条目们, 登记态 = 读登记表(登记表路径)
    面 = {}
    if 要宿主:
        base = 仓库根 / "src"
        if base.exists():
            面["宿主"] = 扫描一面(base, {".cpp", ".hpp"}, "src", 条目们)
    if 要v2:
        base = 仓库根 / "CN语言编译器v2"
        if base.exists():
            面["v2"] = 扫描一面(base, {".cn"}, "CN语言编译器v2", 条目们)
    分组 = {"高": [], "中": [], "低": []}
    登记数 = 0
    for 侧, (_f, _x, 返回们, _统) in 面.items():
        for r in 返回们:
            if r["登记"]:
                登记数 += 1
            分组[r["档"]].append({"侧": 侧, **r})
    for k in 分组:
        分组[k].sort(key=lambda r: (r["文件"], r["行"]))
    return 面, {"登记态": 登记态, "登记数": 登记数, **分组}


def 统计面(面: dict):
    统 = {}
    for 侧, (文件数, 失败, 返回们, 概) in 面.items():
        档计 = {"高": 0, "中": 0, "低": 0}
        for r in 返回们:
            档计[r["档"]] += 1
        统[侧] = {"文件": 文件数, "读取失败": 失败, "return总数": len(返回们),
                 "有值": 概["对照基线"]["有值"], "空返回": 概["对照基线"]["空"],
                 "返数字": 概["对照基线"]["数字"], "返字符串": 概["对照基线"]["字符串"],
                 "返字符": 概["对照基线"]["字符"], "返布尔空": 概["对照基线"]["布尔空"],
                 "函数总数": 概["函数总数"], "游离return": 概["游离return"],
                 "高": 档计["高"], "中": 档计["中"], "低": 档计["低"]}
    return 统


def 打印返回行(r: dict, 缩进: str = "  "):
    print(f"{缩进}[{r['档']}] {r['文件']}:{r['行']}  {r['函数名']}"
          + ("（" + "·".join(r["子标注"]) + "）" if r["子标注"] else ""))
    if r.get("行文本"):
        print(f"{缩进}    行：{r['行文本'][:110]}")
    print(f"{缩进}    返回值：{r['锚']}")
    print(f"{缩进}    理由：" + "；".join(r["理由"]))
    if r["登记"]:
        print(f"{缩进}    已登记：{r['登记']['类别']}")


def 打印报告(面: dict, 合流, limit: int):
    统 = 统计面(面)
    print("== 函数返回路径全量审计（check_return_paths.py·968）==")
    for 侧, s in 统.items():
        print(f"[{侧}] 文件 {s['文件']}（失败 {s['读取失败']}）；函数 {s['函数总数']}；"
              f"return {s['return总数']}（有值 {s['有值']}·空 {s['空返回']}·游离 {s['游离return']}）")
        print(f"    返字面量：数字 {s['返数字']} / 字符串 {s['返字符串']} / 字符 {s['返字符']}"
              f" / 布尔空指针 {s['返布尔空']}（基线口径「返常量」=数字+布尔空）")
        print(f"    分级：高 {s['高']} / 中 {s['中']} / 低 {s['低']}")
    print(f"登记表：{合流['登记态']}；对账命中 {合流['登记数']} 条")
    print("【档义】高=返字面量且 else/错误分支/查表循环后（几乎必为默认值）；"
          "中=返字面量且 catch/失败标签后；低=其余（含唯一 return/递归终止=多为业务值）。")
    高 = 合流["高"]
    上限 = len(高) if limit == 0 else min(limit, len(高))
    print(f"\n【高嫌疑清单】（{len(高)} 条·展示前 {上限}）")
    for r in 高[:上限]:
        打印返回行(r)
    if 上限 < len(高):
        print(f"  …（其余 {len(高) - 上限} 条用 --limit 0 全列 / --md 输出全量）")
    print(f"\n【中嫌疑】{len(合流['中'])} 条 / 【低嫌疑】{len(合流['低'])} 条（--md 全列）")
    print("\n提示：高嫌疑≠缺陷——档=语境线索；逐条人判（业务值→登记豁免/清偿为报错/"
          "改结果化 API）。登记 scripts/miss_registry.json 后重跑自动对账。"
          "清单文档=docs/返回路径嫌疑池-968.md。")


def 高按目录(高列表: list):
    计数 = {}
    for r in 高列表:
        段 = r["文件"].split("/")
        键 = "/".join(段[:3]) if len(段) > 3 else "/".join(段[:2])
        计数[键] = 计数.get(键, 0) + 1
    return sorted(计数.items(), key=lambda kv: (-kv[1], kv[0]))


def 生成md(面: dict, 合流, limit: int) -> str:
    """md 呈现：统计表+目录分布+紧凑清单（高=limit 全列/截断；中全列；低截前 200）。

    低档（多为业务值面）清单价值限于复核抽样——全量始终可经 --json 取。"""
    统 = 统计面(面)
    行 = []
    行.append("# 返回路径嫌疑池（#968·按 return 语句全量枚举）")
    行.append("")
    行.append("> 自动生成：`python3 scripts/check_return_paths.py --limit 0 --md`"
              "（档=语境线索非缺陷定论；逐条人判后登记 `scripts/miss_registry.json`）。")
    行.append("")
    行.append("## 统计（双侧对比·与 968 基线对照）")
    行.append("")
    行.append("| 侧 | 文件 | 函数 | return 总 | 有值 | 空 | 游离 | 返数字 | 返字符串 | "
              "返布尔空 | 高 | 中 | 低 |")
    行.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for 侧, s in 统.items():
        行.append(f"| {侧} | {s['文件']} | {s['函数总数']} | {s['return总数']} | {s['有值']} | "
                f"{s['空返回']} | {s['游离return']} | {s['返数字']} | {s['返字符串']} | "
                f"{s['返布尔空']} | {s['高']} | {s['中']} | {s['低']} |")
    行.append("")
    对照段 = []
    for 侧, s in 统.items():
        常量 = s["返数字"] + s["返布尔空"]
        对照段.append(f"{侧} {s['return总数']} 总（有值 {s['有值']}·空 {s['空返回']}·"
                    f"返常量 {常量}）")
    行.append("口径：词法级 return/返回 语句（注释/字符串内不计；游离=不在任何函数内·"
              "应为 0=扫描面健康度指示）；返常量（基线口径）=数字+布尔空指针。"
              "基线 118e17d4：宿主 3678 return/1613 返常量·v2 3704 返回/1617 返常量。"
              "本树实测：" + "；".join(对照段) + "——同量级=扫描面口径正确。")
    行.append("")
    行.append(f"登记表：{合流['登记态']}；对账命中 {合流['登记数']} 条。")
    行.append("")
    行.append("### 高嫌疑目录分布（清偿批次规划输入）")
    行.append("")
    行.append("| 目录 | 条数 |")
    行.append("|---|---|")
    for 键, n in 高按目录(合流["高"]):
        行.append(f"| {键} | {n} |")
    行.append("")
    行.append("## 高嫌疑清单（返字面量且 else/错误分支/查表循环后）")
    行.append("")
    上限 = len(合流["高"]) if limit == 0 else min(limit, len(合流["高"]))
    for r in 合流["高"][:上限]:
        子 = "（" + "·".join(r["子标注"]) + "）" if r["子标注"] else ""
        行.append(f"- **{r['文件']}:{r['行']}** {r['函数名']}{子} — 返 `{r['锚'][:40]}` — "
                + "；".join(r["理由"]))
        if r["登记"]:
            行.append(f"  - 已登记：{r['登记']['类别']}")
    if 上限 < len(合流["高"]):
        行.append(f"（其余 {len(合流['高']) - 上限} 条用 --limit 0 全列）")
    行.append("")
    行.append("## 中嫌疑清单（返字面量且 catch/失败标签后）")
    行.append("")
    for r in 合流["中"]:
        行.append(f"- [{r['文件']}:{r['行']}] {r['函数名']} — 返 `{r['锚'][:40]}` — "
                + "；".join(r["理由"]))
    行.append("")
    行.append("## 低嫌疑清单（其余·多为业务值·抽样前 200）")
    行.append("")
    for r in 合流["低"][:200]:
        子 = "（" + "·".join(r["子标注"]) + "）" if r["子标注"] else ""
        行.append(f"- [{r['文件']}:{r['行']}] {r['函数名']}{子} — 返 `{r['锚'][:40]}` — "
                + "；".join(r["理由"]))
    if len(合流["低"]) > 200:
        行.append(f"- …（其余 {len(合流['低']) - 200} 条用 --json 取全量）")
    行.append("")
    行.append("## 清偿批次规划建议")
    行.append("")
    行.append("1. 高档=语境线索池（最接近「失败/未知路径返回正常类型值」的 15 处抓捕"
              "同族面）；逐条人判三选一：业务值→登记豁免 / 清债为报错确定性失败 / "
              "改结果化 API（结果<值,错误>）。")
    行.append("2. 批次序建议：按上「高嫌疑目录分布」逐目录成批；查表循环后/错误分支"
              "两类优先（852/648 已清面同簇——逐条先核消费点防重复清偿）。")
    行.append("3. 与 #950（miss 形态窄口径）的差异：本件=每条 return 的宽口径枚举"
              "（含 else 正常分支枚举误报面——正是「嫌疑池」语义）；两清单互补不互斥。")
    行.append("4. 本轮止于「工具+清单」——存量逐条裁决与清偿是后续批次；"
              "登记表示范条目 5 条（含 1 条待裁决），防账目灌水。")
    行.append("")
    return "\n".join(行)


def 返回干净(r: dict):
    return {k: v for k, v in r.items()
            if k not in ("值tok们", "块链", "函数", "条件们")}


def 转json(面: dict, 合流) -> str:
    统 = 统计面(面)
    输出 = {"工具": "check_return_paths.py", "任务": "968",
          "统计": 统, "登记态": 合流["登记态"], "登记命中": 合流["登记数"],
          "高": [返回干净(r) for r in 合流["高"]],
          "中": [返回干净(r) for r in 合流["中"]],
          "低": [返回干净(r) for r in 合流["低"]]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 七、自测（正反两态·零触碰真实树） ============

样例宿主 = (
    "// 高甲：else 块返常量\n"
    "int 分类(int 模式) {\n"
    "    if (模式 > 0) { return 模式 + 1; } else { return 0; }\n"
    "}\n"
    "// 高乙：错误分支返常量\n"
    "int 求宽(表& t, 字符串 k) {\n"
    "    auto it = t.find(k);\n"
    "    if (it == t.end()) return 8;\n"
    "    return 宽度(it);\n"
    "}\n"
    "// 高丙：查表循环后返常量\n"
    "int 查表后(向量 v) {\n"
    "    for (auto x : v) { auto y = v.find(x); 用(y); }\n"
    "    return 0;\n"
    "}\n"
    "// 中：catch 块内返常量\n"
    "int 中样例() {\n"
    "    try { 危险(); } catch (...) { return -1; }\n"
    "}\n"
    "// 低：唯一 return 简单函数（业务值）\n"
    "int 常量() { return 42; }\n"
    "// 低：非常量返回\n"
    "int 业务(表& t) {\n"
    "    if (查(t)) { return 值(t); }\n"
    "    return 数(t);\n"
    "}\n"
)

样例v2 = (
    "// v2 样例：错误分支返常量（高）+ 普通返回（低）\n"
    "函数 取宽(字符串 k) -> 整64 {\n"
    "    如果 (k == 空指针) { 返回 0; }\n"
    "    当 (k != 空指针) { 返回 1; }\n"
    "    返回 5;\n"
    "}\n"
)


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    with tempfile.TemporaryDirectory(prefix="crp968_") as td:
        根 = Path(td)
        (根 / "src").mkdir()
        (根 / "src" / "样例.cpp").write_text(样例宿主, encoding="utf-8")
        (根 / "CN语言编译器v2").mkdir()
        (根 / "CN语言编译器v2" / "源.cn").write_text(样例v2, encoding="utf-8")
        文件数, _失败, 返回们, 概 = 扫描一面(根 / "src", {".cpp"}, "src", [])
        断言(文件数 == 1, "文件计数")
        断言(概["函数总数"] == 6, f"函数枚举=6: {概['函数总数']}")
        断言(概["游离return"] == 0, f"游离 return=0: {概['游离return']}")
        断言(len(返回们) == 9, f"return 总数=9: {len(返回们)}")
        档map = {}
        for r in 返回们:
            档map.setdefault(r["档"], []).append((r["行"], r["锚"], r["理由"]))
        断言(len(档map.get("高", [])) == 3, f"高=3: {档map.get('高')}")
        断言(len(档map.get("中", [])) == 1, f"中=1: {档map.get('中')}")
        断言(len(档map.get("低", [])) == 5, f"低=5: {档map.get('低')}")
        高理由 = " ".join(" ".join(x[2]) for x in 档map.get("高", []))
        断言("else" in 高理由, "else 块内理由")
        断言("错误分支" in 高理由, "错误分支理由")
        断言("查表循环后" in 高理由, "查表循环后理由")
        中理由 = " ".join(" ".join(x[2]) for x in 档map.get("中", []))
        断言("catch" in 中理由, "catch 理由")
        低子 = [s for r in 返回们 if r["档"] == "低" for s in r["子标注"]]
        断言("唯一 return" in 低子, "唯一 return 子标注")
        # 空返回与字面量分类
        值类 = [r["值类"] for r in 返回们]
        断言("数字" in 值类, "数字字面量分类")
        文件数2, _f2, 返回们2, 概2 = 扫描一面(根 / "CN语言编译器v2", {".cn"},
                                        "CN语言编译器v2", [])
        断言(len(返回们2) == 3, f"v2 return=3: {len(返回们2)}")
        v2高 = [r for r in 返回们2 if r["档"] == "高"]
        断言(len(v2高) == 1, f"v2 高=1（错误分支）: {[(r['行'], r['档'], r['理由']) for r in 返回们2]}")
        # 登记对账
        条 = {"文件": "src/样例.cpp", "锚": "8", "类别": "业务值"}
        hit = [r for r in 返回们 if r["锚"] == "8"]
        断言(hit and 登记命中(hit[0], [条]) is not None, "登记锚对账")
        断言(登记命中(hit[0], [{"文件": "x.cpp", "锚": "8"}]) is None, "异文件不误配")
    if 失败:
        print("== 返回路径审计器自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 返回路径审计器自测全过（{通过[0]} 项断言·正反两态齐备）==")
    return 0


# ============ 八、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="函数返回路径全量审计（968·按 return 语句全量枚举+语境分级）")
    解析器.add_argument("--only", choices=["宿主", "v2"], help="只审指定侧")
    解析器.add_argument("--limit", type=int, default=40, help="清单截断条数（0=全列）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--md", nargs="?", const="-", default=None, metavar="文件",
                        help="md 输出（缺省 stdout；给路径=写文件）")
    解析器.add_argument("--strict", action="store_true", help="高>0 → rc=1")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    要宿主, 要v2 = True, True
    if 参数.only == "宿主":
        要v2 = False
    elif 参数.only == "v2":
        要宿主, 要v2 = False, True

    面, 合流 = 全库扫描(要宿主, 要v2)
    if not 面:
        print("[错误] 扫描面为空（src/ 与 CN语言编译器v2/ 都不存在？）")
        return 2

    if 参数.json:
        print(转json(面, 合流))
    elif 参数.md is not None:
        文本 = 生成md(面, 合流, 参数.limit)
        if 参数.md == "-":
            print(文本)
        else:
            Path(参数.md).write_text(文本 + "\n", encoding="utf-8")
            print(f"[输出] {参数.md}")
    else:
        打印报告(面, 合流, 参数.limit)

    if 参数.strict and len(合流["高"]):
        print(f"[--strict] 高嫌疑 {len(合流['高'])} 条 → rc=1", flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
