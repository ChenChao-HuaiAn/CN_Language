#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ARM64 槽窥孔结构指标计数器（任务 1319·v2 ARM64 窥孔面缺失量化）。

定位：只读统计工具——对 linux-arm64 GAS 产物（宿主或 v2 发射）按**宿主同源判据**
（src/cn_compiler/codegen/peephole_slot_arm64.cpp：1157 固定帧槽 + 1243 大偏移
间接寻址）统计「可被消除但尚未消除」的候选对数，供 #1319 前后指标对照与
plans/032 §4.2「预期产物变化类」证据链使用。

统计口径（逐项对齐宿主判据，缺一不算候选）：
  ① 固定帧槽（1157/1279）：相邻（中间仅注释/空行）store→load 同槽对，
     槽偏移在**整个函数段**内恰好出现 2 次，同整数/浮点族，读宽 ≤ 存宽；
     全删 = 同寄存器同族且存读均 64 位（宿主 fullDeleteArm），其余为改写。
  ② 大偏移间接（1243）：store [x13] 前有 x13←x29±off 物化块、load [x13] 前有
     逐行**同文**物化块；全段该 x29 相对偏移（直寻 token + 物化块尾/链）恰好
     2 次；删除点后 x13 先定义后读（两态逆序安全表，标签=入口重置未定义）；
     store 源/load 目的不得是 x13；同槽/同族/读宽 ≤ 存宽。
      全删 = 同寄存器同族且均 64 位；其余为改写（fmov/mov/sxtb/…）。

用法:
  python scripts/peephole_arm64_stats.py <asm 文件...> [--json] [--verbose]
  python scripts/peephole_arm64_stats.py --selftest

说明：v2 产物槽形态为 `[x29, #-8]`（逗号后有空格），宿主为 `[x29,#-8]`；
解析两种形态均接受。v2 槽地址寄存器恒 x13（与宿主同约定）。
"""

import argparse
import json
import re
import sys

# 行内 token 边界（寄存器名前后非标识符字符）
标识符字符 = set("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")

# 槽访问行：str/strb/strh/ldr/ldrb/ldrh/ldrsb/ldrsh  reg, [x29, #±N] / [x13]
槽访问式 = re.compile(
    r"^\s*(str|strb|strh|ldr|ldrb|ldrh|ldrsb|ldrsh)\s+"
    r"([xwds]\d+)\s*,\s*\[\s*(x29|x13)\s*(?:,\s*(#[+-]?\d+))?\s*\]\s*(?://.*)?$")
# 物化块立即数尾：sub/add xN, x29, #N
物化尾式 = re.compile(
    r"^\s*(sub|add)\s+(x\d+)\s*,\s*x29\s*,\s*#(\d+)\s*(?://.*)?$")
# 物化块自引用尾：sub/add xN, x29, xN（须由 movz/movk 链先行定义）
物化自尾式 = re.compile(
    r"^\s*(sub|add)\s+(x\d+)\s*,\s*x29\s*,\s*(x\d+)\s*(?://.*)?$")
物化头式 = re.compile(
    r"^\s*movz\s+(x\d+)\s*,\s*#(\d+)(?:\s*,\s*lsl\s*#(\d+))?\s*(?://.*)?$")
物化片式 = re.compile(
    r"^\s*movk\s+(x\d+)\s*,\s*#(\d+)\s*,\s*lsl\s*#(\d+)\s*(?://.*)?$")


def 是标签(行):
    s = 行.strip()
    return s.endswith(":") and not s.startswith("//") and not s.startswith(";")


def 是注释或空(行):
    s = 行.strip()
    if not s:
        return True
    return s.startswith("//") or s.startswith(";") or s.startswith("/*") or s.startswith("*")


def 含寄存器(行, 名="x13"):
    """行内是否出现归一化寄存器 token 名（前后边界非标识符字符）。"""
    pos = 0
    while True:
        pos = 行.find(名, pos)
        if pos < 0:
            return False
        left = pos == 0 or 行[pos - 1] not in 标识符字符
        end = pos + len(名)
        right = end >= len(行) or 行[end] not in 标识符字符
        if left and right:
            return True
        pos = end


def 解析槽访问(行):
    """-> (助记符, 寄存器, 基址, 偏移|None) 或 None。基址 x29 无偏移时偏移=0。"""
    m = 槽访问式.match(行)
    if not m:
        return None
    mnem, reg, base, off = m.groups()
    if base == "x29":
        off = int(off[1:]) if off else 0
    else:
        off = None  # [x13] 无偏移
    return mnem, reg, base, off


def 寄存器宽(reg):
    """寄存器形态 -> 访问字节宽（x/d=8, w/s=4）。返回 None=未知形态。"""
    形 = reg[0]
    if 形 in ("x", "d"):
        return 8
    if 形 in ("w", "s"):
        return 4
    return None


def 解析槽访问语义(行, 期望基址):
    """解析为访问语义 dict（store/load 共用），形态不匹配返回 None。

    width: 访问位宽（str=reg 宽；strb=8/strh=16；ldrb=8/ldrh=16；
            ldrsb=8(符号)/ldrsh=16(符号)；ldr/str=reg 宽）
    """
    r = 解析槽访问(行)
    if r is None:
        return None
    mnem, reg, base, off = r
    if base != 期望基址:
        return None
    rw = 寄存器宽(reg)
    if rw is None:
        return None
    浮点 = reg[0] in ("d", "s")
    符号 = False
    if mnem in ("str", "ldr"):
        宽 = rw * 8
    elif mnem in ("strb", "ldrb"):
        宽 = 8
    elif mnem in ("strh", "ldrh"):
        宽 = 16
    elif mnem == "ldrsb":
        宽 = 8
        符号 = True
    else:  # ldrsh
        宽 = 16
        符号 = True
    return {
        "助记符": mnem, "寄存器": reg, "基址": base, "槽键": off,
        "宽": 宽, "浮点": 浮点, "符号扩展": 符号,
    }


def 分析物化行(行):
    """-> ('MovzHead'|'MovkPart'|'ImmTail'|'SelfTail', 寄存器, 立即数, 移位, 是否sub)"""
    m = 物化头式.match(行)
    if m:
        return ("MovzHead", m.group(1), int(m.group(2)), int(m.group(3) or 0), False)
    m = 物化片式.match(行)
    if m:
        return ("MovkPart", m.group(1), int(m.group(2)), int(m.group(3)), False)
    m = 物化尾式.match(行)
    if m:
        return ("ImmTail", m.group(2), int(m.group(3)), 0, m.group(1) == "sub")
    m = 物化自尾式.match(行)
    if m and m.group(2) == m.group(3):
        return ("SelfTail", m.group(2), 0, 0, m.group(1) == "sub")
    return None


def 是纯x13读(行):
    """sub/add <dst>, <src>, x13（dst/src 均非 x13）：只读 x13 形态。"""
    m = re.match(r"^\s*(sub|add)\s+(x\d+)\s*,\s*(x\d+)\s*,\s*x13\s*(?://.*)?$", 行)
    if not m:
        return False
    return m.group(2) != "x13" and m.group(3) != "x13"


def 找物化块(行们, end, 段起):
    """从 end（不含）向前跳过注释/空行后识别物化块末行。

    -> dict(start, len, offset, reg, tail_index) 或 None
    注：v2 产物每条 IR 指令前有 `// op=N` 注释行——块① 可能紧邻 store，
    也可能与 store 之间夹注释（注释无执行语义，等价宿主严格相邻口径）。
    """
    tail = end - 1
    while tail >= 段起 and 是注释或空(行们[tail]):
        tail -= 1
    if tail < 段起:
        return None
    last = 分析物化行(行们[tail])
    if last is None:
        return None
    kind, reg, imm, shift, is_sub = last
    if kind == "ImmTail":
        return {"start": tail, "len": 1, "offset": (-imm if is_sub else imm),
                "reg": reg, "tail_index": tail}
    if kind != "SelfTail":
        return None
    head = -1
    k = tail - 1
    while k >= 段起:
        ln = 分析物化行(行们[k])
        if ln is None:
            break
        if ln[0] == "MovkPart" and ln[1] == reg:
            k -= 1
            continue
        if ln[0] == "MovzHead" and ln[1] == reg:
            head = k
        break
    if head < 0:
        return None
    值 = 0
    for k in range(head, tail):
        ln = 分析物化行(行们[k])
        if ln is None or ln[1] != reg:
            return None
        if ln[0] == "MovzHead":
            值 = ln[2] << ln[3]
        elif ln[0] == "MovkPart":
            值 |= ln[2] << ln[3]
        else:
            return None
    return {"start": head, "len": tail - head + 1,
            "offset": (-值 if is_sub else 值), "reg": reg, "tail_index": tail}


def 识物化块正向(行们, s, 段终):
    """从 s（非注释行）正向识别物化块 -> dict(start, len, offset, reg) 或 None。"""
    if s >= 段终:
        return None
    ln = 分析物化行(行们[s])
    if ln is None:
        return None
    kind, reg, imm, shift, is_sub = ln
    if kind == "ImmTail":
        return {"start": s, "len": 1, "offset": (-imm if is_sub else imm), "reg": reg}
    if kind != "MovzHead":
        return None
    值 = imm << shift
    k = s + 1
    while k < 段终:
        nxt = 分析物化行(行们[k])
        if nxt is not None and nxt[0] == "MovkPart" and nxt[1] == reg:
            值 |= nxt[2] << nxt[3]
            k += 1
            continue
        break
    if k >= 段终:
        return None
    tail = 分析物化行(行们[k])
    if tail is None or tail[0] != "SelfTail" or tail[1] != reg:
        return None
    return {"start": s, "len": k + 1 - s,
            "offset": (-值 if tail[4] else 值), "reg": reg}


def 收集直寻槽计数(行们, 段起, 段终):
    """固定帧槽口径：只数 [x29] 直寻 token（按偏移）。"""
    计数 = {}
    for k in range(段起, 段终):
        r = 解析槽访问(行们[k])
        if r is None:
            continue
        mnem, reg, base, off = r
        if base != "x29":
            continue
        计数[off] = 计数.get(off, 0) + 1
    return 计数


def 收集x29相对计数(行们, 段起, 段终):
    """大偏移口径：直寻 token + 物化块尾/链（任意目标寄存器）。"""
    计数 = {}
    待定寄存器 = None
    待定值 = 0
    有待定 = False
    for k in range(段起, 段终):
        ln = 分析物化行(行们[k])
        if ln is not None:
            kind, reg, imm, shift, is_sub = ln
            if kind == "MovzHead":
                待定寄存器 = reg
                待定值 = imm << shift
                有待定 = True
                continue
            if kind == "MovkPart":
                if 有待定 and reg == 待定寄存器:
                    待定值 |= imm << shift
                else:
                    有待定 = False
                continue
            if kind == "ImmTail":
                键 = -imm if is_sub else imm
                计数[键] = 计数.get(键, 0) + 1
                有待定 = False
                continue
            if kind == "SelfTail":
                if 有待定 and reg == 待定寄存器:
                    键 = -待定值 if is_sub else 待定值
                    计数[键] = 计数.get(键, 0) + 1
                有待定 = False
                continue
        有待定 = False
        r = 解析槽访问(行们[k])
        if r is not None and r[2] == "x29":
            计数[r[3]] = 计数.get(r[3], 0) + 1
    return 计数


def 计算尾部安全(行们, 段起, 段终):
    """safe0/safe1 逆序表（进入时 x13 未定义/已定义仍安全）。"""
    n = 段终 - 段起
    safe0 = [True] * (n + 1)
    safe1 = [True] * (n + 1)
    for idx in range(n - 1, -1, -1):
        行 = 行们[段起 + idx]
        if 是标签(行):
            safe0[idx] = safe0[idx + 1]
            safe1[idx] = safe0[idx + 1]
            continue
        if not 含寄存器(行, "x13"):
            safe0[idx] = safe0[idx + 1]
            safe1[idx] = safe1[idx + 1]
            continue
        ln = 分析物化行(行)
        if ln is not None and ln[0] in ("MovzHead", "ImmTail"):
            safe0[idx] = safe1[idx + 1]
            safe1[idx] = safe1[idx + 1]
            continue
        if ln is not None and ln[0] in ("MovkPart", "SelfTail"):
            safe0[idx] = False
            safe1[idx] = safe1[idx + 1]
            continue
        if 是纯x13读(行):
            safe0[idx] = False
            safe1[idx] = safe1[idx + 1]
            continue
        # [x13] 以未定义地址读写内存 -> 进入未定义即不安全；已定义访问不破坏
        r = 解析槽访问(行)
        if r is not None and r[2] == "x13":
            safe0[idx] = False
            safe1[idx] = safe1[idx + 1]
            continue
        safe0[idx] = False
        safe1[idx] = False
    return safe0, safe1


def 区段划分(行们):
    """-> [(段起, 段终)]：.globl 起到下一 .globl 或数据段/节指令前。"""
    段们 = []
    起 = None
    for k, 行 in enumerate(行们):
        s = 行.strip()
        if s.startswith(".globl "):
            if 起 is not None:
                段们.append((起, k))
            起 = k
            continue
        if 起 is not None and (s == ".data" or s.startswith(".section ") or s == ".text"):
            段们.append((起, k))
            起 = None
    if 起 is not None:
        段们.append((起, len(行们)))
    return 段们


def 统计段(行们, 段起, 段终):
    直寻计数 = 收集直寻槽计数(行们, 段起, 段终)
    x29计数 = 收集x29相对计数(行们, 段起, 段终)
    safe0, _safe1 = 计算尾部安全(行们, 段起, 段终)
    结果 = {
        "固定帧槽候选": 0, "固定帧槽全删": 0, "固定帧槽改写": 0,
        "间接候选": 0, "间接全删": 0, "间接改写": 0,
    }
    # ① 固定帧槽
    i = 段起
    while i + 1 < 段终:
        存 = 解析槽访问语义(行们[i], "x29")
        if 存 is None or not 存["助记符"].startswith("str"):
            i += 1
            continue
        j = i + 1
        while j < 段终 and 是注释或空(行们[j]):
            j += 1
        if j >= 段终:
            break
        载 = 解析槽访问语义(行们[j], "x29")
        if 载 is not None and not 载["助记符"].startswith("ldr"):
            载 = None
        命中 = (载 is not None and 存["槽键"] == 载["槽键"]
                and 存["浮点"] == 载["浮点"] and 载["宽"] <= 存["宽"]
                and 直寻计数.get(存["槽键"], 0) == 2)
        if 命中:
            结果["固定帧槽候选"] += 1
            全删 = (存["寄存器"] == 载["寄存器"] and 存["浮点"] == 载["浮点"]
                    and 存["宽"] == 64 and 载["宽"] == 64)
            if 全删:
                结果["固定帧槽全删"] += 1
            else:
                结果["固定帧槽改写"] += 1
            i = j + 1
            continue
        i += 1
    # ② 大偏移间接
    i = 段起
    while i + 2 < 段终:
        存 = 解析槽访问语义(行们[i], "x13")
        if 存 is None or not 存["助记符"].startswith("str"):
            i += 1
            continue
        块1 = 找物化块(行们, i, 段起)
        if 块1 is None or 块1["reg"] != "x13":
            i += 1
            continue
        s2 = i + 1
        while s2 < 段终 and 是注释或空(行们[s2]):
            s2 += 1
        块2 = 识物化块正向(行们, s2, 段终)
        if 块2 is None or 块2["reg"] != "x13" or 块2["len"] != 块1["len"]:
            i += 1
            continue
        if 块2["offset"] != 块1["offset"]:
            i += 1
            continue
        load_idx = s2 + 块2["len"]
        while load_idx < 段终 and 是注释或空(行们[load_idx]):
            load_idx += 1
        if load_idx >= 段终:
            i += 1
            continue
        同文 = all(行们[块1["start"] + k] == 行们[s2 + k] for k in range(块1["len"]))
        if not 同文:
            i += 1
            continue
        载 = 解析槽访问语义(行们[load_idx], "x13")
        if 载 is not None and not 载["助记符"].startswith("ldr"):
            载 = None
        命中 = (载 is not None
                and 存["槽键"] == 载["槽键"] and 存["浮点"] == 载["浮点"]
                and 载["宽"] <= 存["宽"]
                and 存["寄存器"] != "x13" and 载["寄存器"] != "x13"
                and x29计数.get(块1["offset"], 0) == 2
                and safe0[(load_idx + 1) - 段起])
        if 命中:
            结果["间接候选"] += 1
            全删 = (存["寄存器"] == 载["寄存器"] and 存["浮点"] == 载["浮点"]
                    and 存["宽"] == 64 and 载["宽"] == 64)
            if 全删:
                结果["间接全删"] += 1
            else:
                结果["间接改写"] += 1
            i = load_idx + 1
            continue
        i += 1
    return 结果


def 统计文件(路径):
    行们 = [行.rstrip("\r") for 行 in
            open(路径, encoding="utf-8", errors="replace").read().split("\n")]
    def 基址(行):
        r = 解析槽访问(行)
        return r[2] if r is not None else None

    汇总 = {
        "文件": 路径,
        "总行数": len(行们),
        "函数段数": 0,
        "x13引用行": sum(1 for 行 in 行们 if 含寄存器(行, "x13")),
        "x13间接访问行": sum(1 for 行 in 行们 if 基址(行) == "x13"),
        "x29直寻访问行": sum(1 for 行 in 行们 if 基址(行) == "x29"),
        "固定帧槽候选": 0, "固定帧槽全删": 0, "固定帧槽改写": 0,
        "间接候选": 0, "间接全删": 0, "间接改写": 0,
    }
    for 段起, 段终 in 区段划分(行们):
        汇总["函数段数"] += 1
        r = 统计段(行们, 段起, 段终)
        for 键 in ("固定帧槽候选", "固定帧槽全删", "固定帧槽改写",
                   "间接候选", "间接全删", "间接改写"):
            汇总[键] += r[键]
    汇总["可消除对合计"] = 汇总["固定帧槽候选"] + 汇总["间接候选"]
    return 汇总


# ==================== 自测 ====================

样例_固定正例 = """\
.globl f
f:
    str x9, [x29, #-8]
    ldr x9, [x29, #-8]
    ret
"""

样例_固定三处负例 = """\
.globl f
f:
    str x9, [x29, #-8]
    ldr x9, [x29, #-8]
    ldr x10, [x29, #-8]
    ret
"""

样例_固定隔注释正例 = """\
.globl f
f:
    str x9, [x29, #-8]
    // op=1
    ldr x10, [x29, #-8]
    ret
"""

样例_间接正例 = """\
.globl f
f:
    sub x13, x29, #600
    str x9, [x13]
    sub x13, x29, #600
    ldr x9, [x13]
    ret
"""

样例_间接不同文负例 = """\
.globl f
f:
    sub x13, x29, #600
    str x9, [x13]
    sub x13, x29, #608
    ldr x9, [x13]
    ret
"""

样例_间接尾部不安全负例 = """\
.globl f
f:
    sub x13, x29, #600
    str x9, [x13]
    sub x13, x29, #600
    ldr x9, [x13]
    ldr x10, [x13]
    ret
"""

样例_间接偏移三处负例 = """\
.globl f
f:
    sub x13, x29, #600
    str x9, [x13]
    sub x13, x29, #600
    ldr x9, [x13]
    sub x13, x29, #600
    str x10, [x13]
    ret
"""

样例_间接标签重置安全正例 = """\
.globl f
f:
    sub x13, x29, #600
    str x9, [x13]
    sub x13, x29, #600
    ldr x9, [x13]
L1:
    sub x13, x29, #608
    ldr x10, [x13]
    ret
"""


def 自测():
    def 数量(文本, 键):
        import tempfile
        import os
        fd, 路径 = tempfile.mkstemp(suffix=".s")
        try:
            with os.fdopen(fd, "w", encoding="utf-8", newline="") as f:
                f.write(文本)
            return 统计文件(路径)[键]
        finally:
            os.unlink(路径)

    用例 = [
        ("固定帧槽正例全删", 样例_固定正例, "固定帧槽全删", 1),
        ("固定帧槽同槽三处拒绝", 样例_固定三处负例, "固定帧槽候选", 0),
        ("固定帧槽跨注释命中", 样例_固定隔注释正例, "固定帧槽候选", 1),
        ("固定帧槽跨注释改写", 样例_固定隔注释正例, "固定帧槽改写", 1),
        ("间接正例全删", 样例_间接正例, "间接全删", 1),
        ("间接两块不同文拒绝", 样例_间接不同文负例, "间接候选", 0),
        ("间接尾部未定义读拒绝", 样例_间接尾部不安全负例, "间接候选", 0),
        ("间接偏移三处拒绝", 样例_间接偏移三处负例, "间接候选", 0),
        ("间接标签后新定义安全", 样例_间接标签重置安全正例, "间接候选", 1),
    ]
    失败 = 0
    for 名, 文本, 键, 期望 in 用例:
        实际 = 数量(文本, 键)
        ok = 实际 == 期望
        失败 += 0 if ok else 1
        print(("[OK] " if ok else "[FAIL] ") + 名 + f"：{键}={实际}（期望 {期望}）")
    if 失败:
        print(f"自测失败 {失败} 项")
        return 1
    print(f"自测通过 {len(用例)}/{len(用例)}")
    return 0


def main():
    ap = argparse.ArgumentParser(description="ARM64 槽窥孔结构指标计数器（#1319）")
    ap.add_argument("文件", nargs="*", help="linux-arm64 GAS 产物路径")
    ap.add_argument("--json", action="store_true", help="输出 JSON")
    ap.add_argument("--verbose", action="store_true", help="逐文件明细")
    ap.add_argument("--selftest", action="store_true", help="运行内置正反自测")
    args = ap.parse_args()
    if args.selftest:
        return 自测()
    if not args.文件:
        ap.print_help()
        return 2
    汇总们 = [统计文件(p) for p in args.文件]
    合计 = {
        "文件": f"{len(汇总们)} 个文件合计",
        "总行数": sum(r["总行数"] for r in 汇总们),
        "函数段数": sum(r["函数段数"] for r in 汇总们),
        "x13引用行": sum(r["x13引用行"] for r in 汇总们),
        "x13间接访问行": sum(r["x13间接访问行"] for r in 汇总们),
        "x29直寻访问行": sum(r["x29直寻访问行"] for r in 汇总们),
        "固定帧槽候选": sum(r["固定帧槽候选"] for r in 汇总们),
        "固定帧槽全删": sum(r["固定帧槽全删"] for r in 汇总们),
        "固定帧槽改写": sum(r["固定帧槽改写"] for r in 汇总们),
        "间接候选": sum(r["间接候选"] for r in 汇总们),
        "间接全删": sum(r["间接全删"] for r in 汇总们),
        "间接改写": sum(r["间接改写"] for r in 汇总们),
    }
    合计["可消除对合计"] = 合计["固定帧槽候选"] + 合计["间接候选"]
    if args.json:
        print(json.dumps({"明细": 汇总们, "合计": 合计}, ensure_ascii=False, indent=2))
        return 0
    if args.verbose:
        for r in 汇总们:
            print(f"{r['文件']}: 行={r['总行数']} 段={r['函数段数']} "
                  f"x13引用={r['x13引用行']} 间接访问={r['x13间接访问行']} "
                  f"直寻访问={r['x29直寻访问行']} "
                  f"固定帧槽对={r['固定帧槽候选']}(全删{r['固定帧槽全删']}/改写{r['固定帧槽改写']}) "
                  f"间接对={r['间接候选']}(全删{r['间接全删']}/改写{r['间接改写']})")
    print("== 合计 ==")
    for 键 in ("总行数", "函数段数", "x13引用行", "x13间接访问行", "x29直寻访问行",
               "固定帧槽候选", "固定帧槽全删", "固定帧槽改写",
               "间接候选", "间接全删", "间接改写", "可消除对合计"):
        print(f"{键}: {合计[键]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
