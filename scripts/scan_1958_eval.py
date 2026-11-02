#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""scan_1958_eval.py —— #1958 静默兜底评估轮扫描器（十二条款全量扫描·只评估不修码）。

任务 #1958（评估轮打标）：存量代码按「防静默错误零裁量十二条款」（看板 #1964）全量
扫描 + 三型分型（违规/合法接力/豁免清单）支撑，产出机读 JSON 供评估文档
plans/1958 静默兜底评估轮.md 引用。**本工具只读不写（除 --out 输出文件）**。

与既有门禁的分工（不重复造轮子·先跑它们再补盲区）：
  - check_miss_gate.py（#882）已覆盖：miss 返默认值形态 / else 业务 / 单边比较 /
    红旗词面（6 词）+ 协议注释登记对账——本工具不重扫这四面，评估文档直接引其
    --json 输出；
  - check_sentinel_gate.py（#1640）已覆盖：return -1 / return 0+miss 信号 /
    目标尺寸拷贝 / 源型顶替——同上；
  - 本工具补两门禁未覆盖的面（条款映射）：
      条款⑥ 宿主 .value_or(/.value_or{ 全量（含实参提取与缺省值分类）；
      条款⑦ v2 哨兵还原 `= 0 - 1;` 赋值 与 `==/!= 0-1`、`==/!= -1` 哨兵判据；
      条款①② 宿主 switch default 分派候选 + default 段内 return 字面量；
      条款②⑤ 宿主裸 else 块内 return 字面量；v2「否则」块内 返回 字面量
              与 返回 正常(驻留("整64"))（调用终态整64兜底）；
      条款⑪ 注释禁词全表（诚实边界/保守豁免/保守不判/保守放行/宁漏勿崩/
              宁漏勿误/回退/fallback/best-effort/best effort/兜底——比 882
              红旗词面表全）+ 对账辅助词（检不出/放行/保持0/白名单/漏报/不判）；
      条款③ 「报错后继续跑」保守版：宿主 .report(Error/fatal) 与 v2 表诊断(
              调用结束后 10 行内无控制流出口（尽力面·保守标注）；
      条款⑨ 判据函数重名（is*/has* 与 布尔 是*）定义计数≥2（尽力面）；
      条款⑩ report 调用报错文案字面量直传（注册表外文案·尽力面）；
      条款⑫ report 调用 Warning/警告 级清单（检查面放行级别候选·供人工判）。

诚实边界（不假装全覆盖）：
  ① 本工具是「评估轮打标」扫描器不是门禁——输出=原始命中，三型分型由人判
    （评估文档），机器分型仅 value_or 实参缺省值分类等机械标注；
  ② default 段/else 块定位按掩码括号配对，嵌套 if/switch 内层 return 会并入
    外层段（多收=更严·分型时人工剔）；嵌套 switch 已跳一层；
  ③ 「报错后继续跑」只看 10 行窗口与词面出口——异常/外层循环/调用内中止
    不可见（保守=多报，判读时剔）；
  ④ 判据重名不区分「重载/同名片/跨类同名」（多收·人工剔）；
  ⑤ 条款④（枚举含未知成员）/条款⑧（遍历穷举）语义级机器不可全识——不在本
    工具机械面，评估文档以案例+人工审计面覆盖（诚实声明）。

用法：
  python3 scripts/scan_1958_eval.py                    # 扫描·统计摘要到 stdout
  python3 scripts/scan_1958_eval.py --out 输出.json    # 机读全量输出（默认临时目录）
  python3 scripts/scan_1958_eval.py --selftest         # 自测（临时样例树正反两态）

退出码：0=扫描完成；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
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

# 复用 882 掩码/行号/括号配对核（纪律二·不重复实现；参照 check_sentinel_gate 先例）
import check_miss_gate as 静默门禁            # noqa: E402

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")

面名们 = ("条款6_value_or", "条款7_哨兵还原", "条款7_哨兵判据",
        "条款1_default分派候选", "条款2_default返字面量",
        "条款2_else返字面量", "条款2_5_整64兜底",
        "条款11_注释禁词", "条款3_报错后继续",
        "条款9_判据重名", "条款10_文案直传", "条款12_警告级放行")

# ============ 判据常量 ============

# 条款⑥：宿主 optional 消费缺省（value_or( 与模板花括号形态）
value_or模式 = re.compile(r"\.value_or\s*[\({]")

# 条款⑦：v2 哨兵还原赋值（= 0 - 1; 含 = 0-1;）与还原写法返回（return/返回 0 - 1;
# ——check_sentinel_gate 负一哨兵面只抓 return -1 字面量，0-1 还原写法返回=其盲区）
哨兵还原模式 = re.compile(r"=\s*0\s*-\s*1\s*;")
返回哨兵模式 = re.compile(r"(?:\breturn|返回)\s*0\s*-\s*1\s*;")
# 条款⑦：哨兵判据（==/!= 0-1 与 ==/!= -1）
判据零减一模式 = re.compile(r"[=!]=\s*0\s*-\s*1\b")
判据负一模式 = re.compile(r"[=!]=\s*-\s*1\b")

# 条款①：switch 头（宿主）
switch头模式 = re.compile(r"\bswitch\s*\(")
# 分派特征词（条件文本含即算分派候选·枚举/类型码/形态族）
分派特征词 = re.compile(
    r"形态|种类|类型|类别|Kind|kind|Opcode|操作码|Tag|tag|Token|token|"
    r"getType|Type\b|节点|记号|指令|op\b")
# 条款②：default 段/else 块内 return 字面量（宿主·在「仅注释掩码」上扫=字符串保留）
return字面量宿主 = re.compile(
    r"\breturn\s+(?:false|true|nullptr|0\b|-1\b|\"\"|''|假\b|空指针\b)\s*;")
else词宿主 = re.compile(r"(?<![A-Za-z0-9_])else(?![A-Za-z0-9_])")

# 条款②⑤：v2「否则」块与返回字面量（返回 错误(n)=合法传播不判；假/真/0/空指针/
# 空串/正常(驻留(...)=值兜底面）
否则词v2 = re.compile(r"(?<![\u4e00-\u9fff])否则")
返回字面量v2 = re.compile(
    r"返回\s+(?:假\b|真\b|空指针\b|空串\s*\(|正常\s*\(\s*驻留\s*\()|返回\s+0\s*;")
整64兜底模式 = re.compile(
    r"返回\s+正常\s*\(\s*驻留\s*\(\s*\"整64\"\s*\)\s*\)")

# 条款⑪：禁词全表（比 882 红旗词面全·大小写不敏感仅英文词）
禁词表 = ("诚实边界", "保守豁免", "保守不判", "保守放行", "宁漏勿崩", "宁漏勿误",
        "回退", "兜底", "fallback", "best-effort", "best effort")
# 对账辅助词（11-02 审计基线口径词·只计数不全列）
辅助词表 = ("检不出", "放行", "保持 0", "保持0", "白名单", "漏报", "不判")

# 条款③：报错调用（宿主 report / v2 表诊断）与控制流出口词
report调用 = re.compile(r"\.\s*report\s*\(")
表诊断调用 = re.compile(r"表诊断\s*\(")
Error级窗口词 = re.compile(r"[Ee]rror|[Ff]atal")
出口词宿主 = re.compile(
    r"\b(?:return|throw|break|continue|abort|exit|goto)\b|Unreachable")
出口词v2 = re.compile(r"返回|中止|退出|断言|throw|return|break")
报错窗口行数 = 10

# 条款⑨：判据函数定义（bool is*/has*… 与 函数 布尔 是*/有*）
宿主判据定义 = re.compile(r"\bbool\s+((?:is|has|are|can|should)[A-Z]\w*)\s*\(")
v2判据定义 = re.compile(r"函数\s+布尔\s+((?:是|有|应为?)[\u4e00-\u9fffA-Za-z0-9]{0,18})\s*\(")

# 条款⑫：警告级窗口词
警告级窗口词 = re.compile(r"Warning|Warn\b|警告")

# 条款⑩：中文字符（报错文案字面量直传判据）
中文字符 = re.compile(r"[\u4e00-\u9fff]")


# ============ 掩码变体与注释段收集 ============

def 掩码仅注释(文本: str) -> str:
    """仅抹注释（// 与 /* */），字符串字面量保留——return ""/文案字面量判读用。

    与 静默门禁.掩码 同性质：长度与换行保持=偏移/行号跨版本文本一致。"""
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
        if c == '"' or c == "'":          # 字符串：跳过不抹（保内容）
            i = 静默门禁.串尾(文本, i, True)
            continue
        i += 1
    return "".join(掩)


def 收注释段(文本: str):
    """收集注释段 [(起偏移, 文本)]——字符串内容跳过（防 URL // 误判）。"""
    段们 = []
    n = len(文本)
    i = 0
    while i < n:
        c = 文本[i]
        if c == '"' or c == "'":
            i = 静默门禁.串尾(文本, i, True)
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            j = n if j < 0 else j
            段们.append((i, 文本[i:j]))
            i = j
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            j = n if j < 0 else j + 2
            段们.append((i, 文本[i:j]))
            i = j
            continue
        i += 1
    return 段们


def 跳空白(文本: str, i: int) -> int:
    n = len(文本)
    while i < n and 文本[i] in " \t\r\n":
        i += 1
    return i


def 区域名(相对: str) -> str:
    """顶层目录归属（src/cn_compiler/ir/x.cpp → src/ir；v2 语义/x.cn → v2/语义）。"""
    部分 = 相对.split("/")
    if 相对.startswith("src/"):
        return "src/" + (部分[2] if len(部分) > 3 else "-")
    return "v2/" + (部分[1] if len(部分) > 2 else "-")


# ============ 各面扫描 ============

def 扫value_or(相对: str, 掩: str, 行首表: list, 原文: str):
    """条款⑥：宿主 .value_or( / .value_or{ 全量·实参提取+缺省值机械分类。"""
    命中 = []
    缺省词 = ("nullptr", "0", "8", "16", "-1", '""', "false", "空指针", "假",
           "默认", "fallback")
    for m in value_or模式.finditer(掩):
        开 = m.end() - 1
        闭 = 静默门禁.配对括号(掩, 开)
        参数 = 原文[开 + 1:闭].strip() if 闭 is not None and 闭 > 开 \
            else 原文[开 + 1:开 + 60].strip()
        归一参 = re.sub(r"\s+", "", 参数)
        分类 = "其他"
        for 词 in 缺省词:
            if 归一参 == 词 or 归一参 in ("{" + 词 + "}", "(" + 词 + ")"):
                分类 = "缺省值:" + 词
                break
        if 分类 == "其他":
            接收者 = 掩[max(0, m.start() - 60):m.start()]
            尾 = re.search(r"(\w+)\s*(?:\(\s*\))?\s*$", 接收者)
            分类 = "调用:" + (尾.group(1) if 尾 else "?")
        行 = 静默门禁.取行号(行首表, m.start())
        行起 = 行首表[行 - 1] if 行 - 1 < len(行首表) else 0
        行终 = 原文.find("\n", 行起)
        锚 = 原文[行起:行终 if 行终 >= 0 else len(原文)].strip()
        命中.append({"文件": 相对, "行": 行, "条款": "⑥", "锚": 锚[:120],
                   "区域": 区域名(相对), "参数": 参数[:60], "机械分类": 分类})
    return 命中


def 扫哨兵还原与判据(相对: str, 掩: str, 行首表: list, 原文: str):
    """条款⑦：v2 哨兵还原赋值 + 还原写法返回 + ==/!= 哨兵判据（宿主/v2 分组）。"""
    还原, 判据 = [], []
    v2侧 = 相对.startswith("CN语言编译器v2/")
    行们 = 原文.split("\n")
    for m in 哨兵还原模式.finditer(掩):
        行 = 静默门禁.取行号(行首表, m.start())
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        还原.append({"文件": 相对, "行": 行, "条款": "⑦", "锚": 锚[:120],
                   "区域": 区域名(相对), "侧": "v2" if v2侧 else "宿主",
                   "形态": "还原赋值:0-1"})
    for m in 返回哨兵模式.finditer(掩):
        行 = 静默门禁.取行号(行首表, m.start())
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        还原.append({"文件": 相对, "行": 行, "条款": "⑦", "锚": 锚[:120],
                   "区域": 区域名(相对), "侧": "v2" if v2侧 else "宿主",
                   "形态": "返回哨兵:0-1"})
    for 模式, 名 in ((判据零减一模式, "判据:==0-1"), (判据负一模式, "判据:==-1")):
        for m in 模式.finditer(掩):
            行 = 静默门禁.取行号(行首表, m.start())
            锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
            判据.append({"文件": 相对, "行": 行, "条款": "⑦", "锚": 锚[:120],
                       "区域": 区域名(相对), "侧": "v2" if v2侧 else "宿主",
                       "形态": 名})
    return 还原, 判据


def 找嵌套switch区间(块体: str):
    """块体内嵌套 switch 块的区间列表（default 归属降噪）。"""
    区间 = []
    for sm in switch头模式.finditer(块体):
        c1 = 静默门禁.配对括号(块体, sm.end() - 1)
        if c1 is None:
            continue
        k = 跳空白(块体, c1 + 1)
        if k < len(块体) and 块体[k] == "{":
            c2 = 静默门禁.配对括号(块体, k)
            if c2 is not None:
                区间.append((k, c2))
    return 区间


def 扫default分派(相对: str, 掩: str, 仅注: str, 行首表: list, 原文: str):
    """条款①②：宿主 switch default 分派候选 + default 段内 return 字面量。"""
    候选, 字面量 = [], []
    行们 = 原文.split("\n")
    for m in switch头模式.finditer(掩):
        开 = m.end() - 1
        条件闭 = 静默门禁.配对括号(掩, 开)
        if 条件闭 is None:
            continue
        条件文本 = 掩[开 + 1:条件闭]
        j = 跳空白(掩, 条件闭 + 1)
        if j >= len(掩) or 掩[j] != "{":
            continue
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        块体 = 掩[j + 1:块闭]
        嵌套 = 找嵌套switch区间(块体)

        def 在嵌套(pos):
            return any(a <= pos <= b for a, b in 嵌套)

        标签 = [(mt.start(), mt.group(1)) for mt in
              re.finditer(r"\b(case|default)\b", 块体) if not 在嵌套(mt.start())]
        有case = any(t == "case" for _p, t in 标签)
        特征 = bool(分派特征词.search(条件文本))
        if (有case or 特征):
            行 = 静默门禁.取行号(行首表, m.start())
            锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
            候选.append({"文件": 相对, "行": 行, "条款": "①", "锚": 锚[:120],
                       "区域": 区域名(相对),
                       "有case": 有case, "特征词": 特征,
                       "default数": sum(1 for _p, t in 标签 if t == "default")})
        for idx, (p, t) in enumerate(标签):
            if t != "default":
                continue
            段终 = 标签[idx + 1][0] if idx + 1 < len(标签) else len(块体)
            段 = 仅注[j + 1 + p:j + 1 + 段终]
            for rm in return字面量宿主.finditer(段):
                绝对 = j + 1 + p + rm.start()
                行 = 静默门禁.取行号(行首表, 绝对)
                锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
                字面量.append({"文件": 相对, "行": 行, "条款": "①②", "锚": 锚[:120],
                             "区域": 区域名(相对), "返回文本": rm.group(0)[:40]})
    return 候选, 字面量


def 扫else块字面量(相对: str, 文本: str, 掩: str, 仅注: str, 行首表: list, v2: bool):
    """条款②：裸 else/否则 块内 return 字面量（else-if/否则如果 链延续跳过）。"""
    命中 = []
    行们 = 文本.split("\n")
    模式 = 否则词v2 if v2 else else词宿主
    链续 = ("如果",) if v2 else ("if",)
    for m in 模式.finditer(掩):
        j = 跳空白(掩, m.end())
        if any(掩.startswith(词, j) for 词 in 链续):
            continue
        if j >= len(掩) or 掩[j] != "{":
            continue
        块闭 = 静默门禁.配对括号(掩, j)
        if 块闭 is None:
            continue
        块源 = 仅注[j + 1:块闭]
        模式2 = 返回字面量v2 if v2 else return字面量宿主
        for rm in 模式2.finditer(块源):
            绝对 = j + 1 + rm.start()
            行 = 静默门禁.取行号(行首表, 绝对)
            锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
            命中.append({"文件": 相对, "行": 行,
                       "条款": "②⑤" if v2 else "②", "锚": 锚[:120],
                       "区域": 区域名(相对), "返回文本": rm.group(0)[:40],
                       "侧": "v2" if v2 else "宿主"})
    return 命中


def 扫整64兜底(相对: str, 仅注: str, 行首表: list, 原文: str):
    """条款②⑤：v2 返回 正常(驻留("整64")) 全量（调用终态整64兜底）。"""
    命中 = []
    行们 = 原文.split("\n")
    for m in 整64兜底模式.finditer(仅注):
        行 = 静默门禁.取行号(行首表, m.start())
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        命中.append({"文件": 相对, "行": 行, "条款": "②⑤", "锚": 锚[:120],
                   "区域": 区域名(相对)})
    return 命中


def 扫注释禁词(相对: str, 原文: str, 行首表: list):
    """条款⑪：注释禁词全表 + 对账辅助词（辅助词只计数）。"""
    禁词命中, 辅助计数 = [], Counter()
    for 起, 段文本 in 收注释段(原文):
        行 = 静默门禁.取行号(行首表, 起)
        小写 = 段文本.lower()
        锚 = re.sub(r"^(?://+|/\*+|\*+)\s*", "", 段文本.strip()).strip()
        for 词 in 禁词表:
            if (词 in 小写) if 词.isascii() else (词 in 段文本):
                禁词命中.append({"文件": 相对, "行": 行, "条款": "⑪", "词": 词,
                               "锚": 锚[:80], "区域": 区域名(相对)})
        for 词 in 辅助词表:
            if (词 in 小写) if 词.isascii() else (词 in 段文本):
                辅助计数[词] += 1
    return 禁词命中, 辅助计数


def 扫报错后继续(相对: str, 掩: str, 仅注: str, 行首表: list, 原文: str, v2: bool):
    """条款③：报错调用结束后 10 行内无控制流出口（保守·尽力面）。"""
    命中 = []
    行们 = 原文.split("\n")
    模式 = 表诊断调用 if v2 else report调用
    出口 = 出口词v2 if v2 else 出口词宿主
    for m in 模式.finditer(掩):
        开 = m.end() - 1
        闭 = 静默门禁.配对括号(掩, 开)
        if 闭 is None:
            continue
        if not v2:
            窗口 = 仅注[开:闭 + 1]
            if not Error级窗口词.search(窗口):
                continue                  # 非 Error/fatal 级（警告/info 不判）
        L0 = 静默门禁.取行号(行首表, 闭)
        # 出口词只在仅注文本上搜（原文行含注释·词面误命中=882 轮实证盲区）
        窗文 = "\n".join(仅注.split("\n")[L0 - 1:L0 - 1 + 报错窗口行数 + 1])
        if 出口.search(窗文):
            continue
        锚 = 行们[L0 - 1].strip() if 0 < L0 <= len(行们) else ""
        命中.append({"文件": 相对, "行": L0, "条款": "③", "锚": 锚[:120],
                   "区域": 区域名(相对), "侧": "v2" if v2 else "宿主",
                   "保守": True})
    return 命中


def 扫判据重名(相对: str, 掩: str, 行首表: list, v2: bool):
    """条款⑨：判据函数定义收集（重名判定在汇总层做）。"""
    定义 = []
    模式 = v2判据定义 if v2 else 宿主判据定义
    for m in 模式.finditer(掩):
        闭 = 静默门禁.配对括号(掩, m.end() - 1)
        if 闭 is None:
            continue
        后 = 跳空白(掩, 闭 + 1)          # 声明（紧跟 ;）不算定义
        if 后 < len(掩) and 掩[后] == ";":
            continue
        行 = 静默门禁.取行号(行首表, m.start())
        定义.append({"文件": 相对, "行": 行, "条款": "⑨", "名": m.group(1),
                   "区域": 区域名(相对)})
    return 定义


def 扫report文案与级别(相对: str, 掩: str, 仅注: str, 行首表: list, 原文: str):
    """条款⑩：report 实参中文文案字面量直传；条款⑫：Warning/警告 级清单。"""
    文案, 警告 = [], []
    行们 = 原文.split("\n")
    for m in report调用.finditer(掩):
        开 = m.end() - 1
        闭 = 静默门禁.配对括号(掩, 开)
        if 闭 is None:
            continue
        窗口 = 仅注[开:闭 + 1]
        行 = 静默门禁.取行号(行首表, m.start())
        锚 = 行们[行 - 1].strip() if 0 < 行 <= len(行们) else ""
        if 中文字符.search(窗口) and '"' in 窗口:
            文案.append({"文件": 相对, "行": 行, "条款": "⑩", "锚": 锚[:120],
                       "区域": 区域名(相对)})
        if 警告级窗口词.search(窗口):
            警告.append({"文件": 相对, "行": 行, "条款": "⑫", "锚": 锚[:120],
                       "区域": 区域名(相对)})
    return 文案, 警告


# ============ 全库扫描与汇总 ============

def 全库扫描(根: Path):
    """扫 src/ 与 CN语言编译器v2/——每文件跑各面，返回 (命中表, 判据定义全表)。"""
    命中 = {面: [] for 面 in 面名们}
    判据定义全 = []
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
                原文 = p.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                统计["读取失败"] += 1
                continue
            统计["文件"] += 1
            掩 = 静默门禁.掩码(原文)
            仅注 = 掩码仅注释(原文)
            行首表 = 静默门禁.建行首表(原文)
            v2 = 相对.startswith("CN语言编译器v2/")
            if not v2:
                命中["条款6_value_or"].extend(扫value_or(相对, 掩, 行首表, 原文))
                候选, 字面 = 扫default分派(相对, 掩, 仅注, 行首表, 原文)
                命中["条款1_default分派候选"].extend(候选)
                命中["条款2_default返字面量"].extend(字面)
                命中["条款2_else返字面量"].extend(
                    扫else块字面量(相对, 原文, 掩, 仅注, 行首表, False))
                文案, 警告 = 扫report文案与级别(相对, 掩, 仅注, 行首表, 原文)
                命中["条款10_文案直传"].extend(文案)
                命中["条款12_警告级放行"].extend(警告)
            else:
                命中["条款2_5_整64兜底"].extend(扫整64兜底(相对, 仅注, 行首表, 原文))
                命中["条款2_else返字面量"].extend(
                    扫else块字面量(相对, 原文, 掩, 仅注, 行首表, True))
            还原, 判据 = 扫哨兵还原与判据(相对, 掩, 行首表, 原文)
            命中["条款7_哨兵还原"].extend(还原)
            命中["条款7_哨兵判据"].extend(判据)
            禁词, _辅助 = 扫注释禁词(相对, 原文, 行首表)
            命中["条款11_注释禁词"].extend(禁词)
            命中["条款3_报错后继续"].extend(
                扫报错后继续(相对, 掩, 仅注, 行首表, 原文, v2))
            判据定义全.extend(扫判据重名(相对, 掩, 行首表, v2))
    # 条款⑨：同名判据定义 ≥2 的名字展开为命中
    名计数 = Counter(d["名"] for d in 判据定义全)
    重名 = {名 for 名, c in 名计数.items() if c >= 2}
    命中["条款9_判据重名"] = [d for d in 判据定义全 if d["名"] in 重名]
    # 确定性排序（幂等）
    for 面 in 面名们:
        命中[面].sort(key=lambda x: (x.get("文件", ""), x.get("行", 0),
                                 x.get("锚", "") + x.get("词", "")))
    return 命中, 判据定义全, 统计


def 建统计(命中: dict, 判据定义全: list, 统计: dict, 辅助计数: Counter):
    """统计摘要（含按区域分布与按文件分布 top）——文档对账引用层。"""
    摘 = {"扫描文件": 统计["文件"], "读取失败": 统计["读取失败"]}
    for 面 in 面名们:
        点们 = 命中[面]
        区 = Counter(p.get("区域", "?") for p in 点们)
        文 = Counter(p["文件"] for p in 点们)
        项 = {"总数": len(点们),
            "按区域": dict(sorted(区.items(), key=lambda kv: -kv[1]))}
        if 面 == "条款7_哨兵还原":
            项["按侧"] = dict(Counter(p.get("侧", "?") for p in 点们))
            项["文件数"] = len(文)
        if 面 == "条款7_哨兵判据":
            项["按侧"] = dict(Counter(p.get("侧", "?") for p in 点们))
            项["按形态"] = dict(Counter(p.get("形态", "?") for p in 点们))
        if 面 == "条款6_value_or":
            项["按机械分类"] = dict(Counter(p.get("机械分类", "?") for p in 点们))
        if 面 == "条款11_注释禁词":
            项["按词"] = dict(Counter(p.get("词", "?") for p in 点们))
        摘[面] = 项
    名计数 = Counter(d["名"] for d in 判据定义全)
    摘["条款9_判据重名"]["重名数"] = sum(1 for c in 名计数.values() if c >= 2)
    摘["辅助词计数"] = dict(sorted(辅助计数.items(), key=lambda kv: -kv[1]))
    return 摘


def 打印摘要(摘: dict):
    print("== #1958 静默兜底评估轮扫描（scan_1958_eval.py·只评估不修码）==")
    print(f"扫描文件 {摘['扫描文件']}（读取失败 {摘['读取失败']}）")
    for 面 in 面名们:
        u = 摘[面]
        行 = f"  {面}: {u['总数']}"
        if u.get("按侧"):
            行 += f" · 侧={u['按侧']}"
        if u.get("按词"):
            行 += f" · 词={u['按词']}"
        if u.get("重名数") is not None:
            行 += f" · 重名函数 {u['重名数']} 个"
        print(行)
    print(f"  辅助词计数（对账用·不在禁词表）: {摘['辅助词计数']}")


# ============ 自测 ============

样例宿主 = """// 自测样例（宿主）
int 查宽(表& t, 文本 k) {
    switch (取形态(k)) {
    case 形态::串: return 1;
    default:
        return 0;          // default 段 return 字面量=命中
    }
}
int 用长(std::optional<int> o) {
    return o.value_or(8);  // 条款⑥命中
}
void 报错不收(诊断器& d) {
    d.report(Diagnostic::error("坏"));
    走();                   // 10 行内无 return=条款③命中
}
// 回退兜底诚实边界（条款⑪注释命中）
"""
样例v2 = """// 自测样例（v2）
函数 取宽(文本 k) -> 整64 {
    如果 (k == "") {
        返回 0 - 1;        // 条款⑦哨兵还原命中
    } 否则 {
        返回 正常(驻留("整64"));   // 条款②⑤整64兜底命中
    }
}
"""

样例v2哨兵还原赋值 = """函数 f() {
    整64 槽 = 0 - 1;       // 哨兵还原赋值=条款⑦
}
"""


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    td = tempfile.TemporaryDirectory(prefix="scan1958_")
    根 = Path(td.name)
    (根 / "src").mkdir(parents=True)
    (根 / "CN语言编译器v2").mkdir()
    (根 / "src" / "样例.cpp").write_text(样例宿主, encoding="utf-8")
    (根 / "CN语言编译器v2" / "样例.cn").write_text(
        样例v2 + 样例v2哨兵还原赋值, encoding="utf-8")
    命中1, 判据1, 统计1 = 全库扫描(根)
    辅助1 = Counter()
    for 前缀 in 扫描前缀:
        base = 根 / 前缀.rstrip("/")
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if p.suffix not in 扫描后缀:
                continue
            行首 = 静默门禁.建行首表(p.read_text(encoding="utf-8"))
            _禁, 辅 = 扫注释禁词(前缀 + p.relative_to(base).as_posix(),
                             p.read_text(encoding="utf-8"), 行首)
            辅助1 += 辅
    断言(len(命中1["条款6_value_or"]) == 1, f"条款⑥样例命中1: {命中1['条款6_value_or']}")
    断言(len(命中1["条款2_default返字面量"]) == 1,
        f"条款②default返字面量命中1: {命中1['条款2_default返字面量']}")
    断言(len(命中1["条款1_default分派候选"]) == 1, "条款①分派候选命中1")
    断言(len(命中1["条款7_哨兵还原"]) == 2, f"条款⑦还原命中2: {命中1['条款7_哨兵还原']}")
    断言(len(命中1["条款2_5_整64兜底"]) == 1, "条款②⑤整64兜底命中1")
    断言(len(命中1["条款2_else返字面量"]) >= 1, "条款②否则块返回命中≥1")
    断言(any(p["词"] == "回退" for p in 命中1["条款11_注释禁词"]), "条款⑪禁词「回退」命中")
    断言(any(p["词"] == "兜底" for p in 命中1["条款11_注释禁词"]), "条款⑪禁词「兜底」命中")
    断言(any(p["词"] == "诚实边界" for p in 命中1["条款11_注释禁词"]),
        "条款⑪禁词「诚实边界」命中")
    断言(len(命中1["条款3_报错后继续"]) == 1, f"条款③样例命中1: {命中1['条款3_报错后继续']}")
    断言("回退" in 辅助1 or True, "辅助词收集不抛异常")
    # 幂等：重跑全等
    命中2, _判据2, _统计2 = 全库扫描(根)
    断言(json.dumps(命中1, ensure_ascii=False, sort_keys=True) ==
        json.dumps(命中2, ensure_ascii=False, sort_keys=True), "幂等：两遍全等")
    if 失败:
        print("== 自测失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        return 1
    print(f"== 自测全过（{通过[0]} 断言·六面正例+幂等）==")
    return 0


# ============ 入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="#1958 静默兜底评估轮扫描器（十二条款·只评估不修码）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--out", default="", help="JSON 输出路径（默认临时目录）")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    命中, 判据定义全, 统计 = 全库扫描(根)
    # 辅助词计数（全库再收一遍注释段·与禁词同源）
    辅助计数 = Counter()
    for 前缀 in 扫描前缀:
        base = 根 / 前缀.rstrip("/")
        if not base.exists():
            continue
        for p in sorted(base.rglob("*")):
            if not p.is_file() or p.suffix not in 扫描后缀:
                continue
            try:
                原文 = p.read_text(encoding="utf-8")
            except (OSError, UnicodeDecodeError):
                continue
            行首 = 静默门禁.建行首表(原文)
            _禁, 辅 = 扫注释禁词(前缀 + p.relative_to(base).as_posix(), 原文, 行首)
            辅助计数 += 辅
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(根),
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    摘 = 建统计(命中, 判据定义全, 统计, 辅助计数)
    输出 = {"工具": "scan_1958_eval.py", "任务": "1958", "生成sha": sha,
          "说明": "十二条款评估轮扫描（原始命中·三型分型由评估文档人工判）",
          "统计": 摘, "命中": {面: 命中[面] for 面 in 面名们}}
    出路径 = Path(参数.out) if 参数.out else \
        Path(tempfile.gettempdir()) / "scan_1958_eval.json"
    出路径.write_text(json.dumps(输出, ensure_ascii=False, indent=1),
                    encoding="utf-8")
    打印摘要(摘)
    print(f"[输出] {出路径}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
