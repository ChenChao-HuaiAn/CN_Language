#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""槽窥孔产物差分与前态距离分析（任务 1279 审查 P2·可复跑证据工具）。

用途（只读两条汇编文本，不调编译器）：
  ① 差分核验：new 必须是 base 仅删若干「同寄存器自搬行」的结果——过滤自搬行后
     逐字节一致；否则报错并给出首个不一致行号。
  ② 获证前态距离分布：对 base 中「被删除的自搬行」，向前找最近一次写该寄存器，
     归类零扩展写/未知写/控制流/无写，并统计零扩展写距离直方图。
  ③ 残余原因分类：对 new 中仍保留的自搬行做同款回溯——找到窗口内零扩展写
     仍保留 = 跨宽面（64 存→32 读，本任务判据外）；否则 = 证明不成立。

用法：
  python tests/tools/peephole_artifact_diff.py --base 旧.s --new 新.s --arch x86
  python tests/tools/peephole_artifact_diff.py --base 旧.s --new 新.s --arch arm
  python tests/tools/peephole_artifact_diff.py --selftest
"""
from __future__ import annotations

import argparse
import collections
import re
import sys

# 自搬行形态：
#   x86：mov eax, eax / mov r10d, r10d（32 位）；mov r10, r10（64 位，本任务不产生新删）
#   arm：mov w3, w3（32 位）；mov x3, x3（64 位，本任务不产生新删）
自搬_X86 = re.compile(r"^\s*mov\s+([a-z0-9]+),\s*\1\s*$")
自搬_ARM = re.compile(r"^\s*mov\s+([wx][0-9]+),\s*\1\s*$")

# x86 32 位寄存器名 -> 64 位规范名（与 peephole_slot.cpp 表一致）
X86_32 = {}
X86_64 = {}
X86_16 = {}
X86_8 = {}
_名64 = ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
         "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
_名32 = ["eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
         "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"]
_名16 = ["ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
         "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"]
_名8 = ["al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"]
for _i in range(16):
    X86_64[_名64[_i]] = _名64[_i]
    X86_32[_名32[_i]] = _名64[_i]
    X86_16[_名16[_i]] = _名64[_i]
    X86_8[_名8[_i]] = _名64[_i]

窗口 = 128  # 与 peephole_slot_internal.hpp kProofScanLimit 一致


def 去注释(行: str) -> str:
    p = 行.find("//")
    return (行[:p] if p >= 0 else 行).strip()


def 切操作数(文本: str):
    """助记符 + 操作数（'[' 内逗号不切）；返回 (mnem, [ops])。"""
    if not 文本:
        return "", []
    sp = 文本.find(" ")
    if sp < 0:
        return 文本, []
    mnem = 文本[:sp]
    rest = 文本[sp + 1:]
    ops, cur, depth = [], "", 0
    for ch in rest:
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        if ch == "," and depth == 0:
            if cur.strip():
                ops.append(cur.strip())
            cur = ""
            continue
        cur += ch
    if cur.strip():
        ops.append(cur.strip())
    return mnem, ops


def 整型寄存器(op: str):
    """纯整型寄存器操作数 -> (64 位规范名, 位宽)；否则 None。"""
    if not op or "[" in op or " " in op or "\t" in op:
        return None
    if op in X86_64:
        return X86_64[op], 64
    if op in X86_32:
        return X86_32[op], 32
    if op in X86_16:
        return X86_16[op], 16
    if op in X86_8:
        return X86_8[op], 8
    return None


def 立即数(op: str):
    m = re.fullmatch(r"([+-]?)(0[xX][0-9a-fA-F]+|[0-9]+)", op.strip())
    if not m:
        return None
    v = int(m.group(2), 0)
    return -v if m.group(1) == "-" else v


def 提及(op: str, reg64: str) -> bool:
    r = 整型寄存器(op)
    return bool(r) and r[0] == reg64


_x86控制 = {"call", "ret", "leave", "jmp", "mul", "div", "idiv", "cqo", "cdq",
          "cwd", "cdqe", "cwde", "cbw", "cmpxchg", "cmpxchg8b", "cmpxchg16b",
          "xadd", "xchg", "syscall", "sysenter", "int", "ud2", "hlt", "cpuid",
          "rdtsc", "rdtscp", "lock", "rep", "repe", "repne", "movsb", "movsq",
          "stosb", "stosq", "lodsb", "lodsq", "scasb", "scasq", "cmpsb", "cmpsq"}
_x86只读 = {"cmp", "test", "push", "bt", "prefetch", "prefetchw", "nop",
          "ucomisd", "ucomiss", "comisd", "comiss"}


def 分类_x86(行: str, reg64: str):
    """-> (类别, 写宽, 零扩展) 类别∈ zero/none/read/write/control。"""
    s = 去注释(行)
    if not s:
        return "none", 0, False
    if s.endswith(":"):
        return "control", 0, False
    if s.startswith("."):
        return "none", 0, False
    mnem, ops = 切操作数(s)
    m = mnem.lower()
    if m in _x86控制 or (len(m) >= 2 and m[0] == "j") or \
            (4 <= len(m) <= 16 and m.startswith("cmov")):
        return "control", 0, False
    if m == "imul" and len(ops) <= 1:
        return "control", 0, False
    if m in _x86只读:
        for op in ops:
            if 提及(op, reg64):
                return "read", 0, False
        return "none", 0, False
    dst = 整型寄存器(ops[0]) if ops else None
    if dst and dst[0] == reg64:
        w = dst[1]
        if w == 32:
            return "zero", 32, True
        if w in (8, 16):
            return "none", w, False
        if w == 64:
            if m == "mov":
                src = 整型寄存器(ops[1]) if len(ops) > 1 else None
                if src and src[0] == reg64 and src[1] == 64:
                    return "none", 64, False
                v = 立即数(ops[1]) if len(ops) > 1 else None
                return "write", 64, (v is not None and 0 <= v <= 0xFFFFFFFF)
            if m == "movzx":
                return "zero", 64, True
            return "write", 64, False
        return "write", w, False
    for op in ops[1:]:
        if 提及(op, reg64):
            return "read", 0, False
    return "none", 0, False


def 分类_arm(行: str, reg64: str):
    s = 去注释(行)
    if not s:
        return "none", 0, False
    if s.endswith(":"):
        return "control", 0, False
    if s.startswith("."):
        return "none", 0, False
    mnem, ops = 切操作数(s)
    m = mnem.lower()
    if reg64[:1] != "x" or not reg64[1:].isdigit():
        return "control", 0, False
    num = reg64[1:]
    if m in ("b", "bl", "br", "blr", "ret", "cbz", "cbnz", "tbz", "tbnz",
             "ldp", "ldpsw", "ldxp", "ldaxp", "stxp", "stlxp", "stlex",
             "stlxr", "ldxr", "ldaxr", "svc", "hvc", "smc", "brk", "udf") or \
            (2 <= len(m) <= 16 and m[0] == "b" and m[1] == "."):
        return "control", 0, False
    只读 = ("str", "strb", "strh", "stur", "sturb", "sturh", "stp", "stnp",
            "stlr", "stllr", "stlrb", "stlrh", "stllrb", "stllrh", "sttr",
            "sttrb", "sttrh", "cmp", "cmn", "tst", "fcmp", "fcmpe", "ccmp",
            "ccmn", "prfm")
    if m in 只读:
        for op in ops:
            mm = re.fullmatch(r"[wx]([0-9]+)", op)
            if mm and mm.group(1) == num:
                return "read", 0, False
        return "none", 0, False
    if ops:
        mm = re.fullmatch(r"([wx])([0-9]+)", ops[0])
        if mm and mm.group(2) == num:
            if mm.group(1) == "w":
                return "zero", 32, True
            # X 目的
            if m == "mov" and len(ops) >= 2:
                if len(ops) == 3:
                    zs = re.fullmatch(r"(lsl|lsr|asr|ror)\s+#([0-9]+)", ops[2])
                    if not zs or int(zs.group(2)) != 0:
                        return "write", 64, False
                src = re.fullmatch(r"([wx])([0-9]+)", ops[1])
                if src and src.group(2) == num:
                    if src.group(1) == "x":
                        return "none", 64, False
                    return "zero", 64, True
                v = None
                if ops[1].startswith("#"):
                    v = 立即数(ops[1][1:])
                if v is not None:
                    return "write", 64, (0 <= v <= 0xFFFFFFFF)
            elif m == "movz":
                for op in ops[2:]:
                    if "#32" in op or "#48" in op:
                        return "write", 64, False
                return "zero", 64, True
            elif m == "movk":
                for op in ops[2:]:
                    if "#32" in op or "#48" in op:
                        return "write", 64, False
                return "none", 64, False
            elif m in ("uxtb", "uxth", "uxtw", "cset"):
                return "zero", 64, True
            return "write", 64, False
    for op in ops[1:]:
        mm = re.fullmatch(r"[wx]([0-9]+)", op)
        if mm and mm.group(1) == num:
            return "read", 0, False
    return "none", 0, False


def 自搬正则(arch: str):
    return 自搬_ARM if arch == "arm" else 自搬_X86


def 回溯(行们, idx: int, reg64: str, 分类):
    """自 idx-1 向前 ≤窗口 行找最近一次写；返回 (类别, 写宽, 零扩展, 距离)。"""
    lo = max(0, idx - 窗口)
    for j in range(idx - 1, lo - 1, -1):
        kind, w, z = 分类(行们[j], reg64)
        if kind == "control":
            return "control", 0, False, idx - j
        if kind in ("zero", "write"):
            return kind, w, z, idx - j
    return "none", 0, False, 0


def 自搬寄存器(行: str, arch: str):
    mm = 自搬正则(arch).match(行)
    if not mm:
        return None
    tok = mm.group(1)
    if arch == "arm":
        return "x" + tok[1:]
    if tok in X86_32:
        return X86_32[tok]
    if tok in X86_64:
        return X86_64[tok]
    return None


def 差分(base, new, arch):
    正则 = 自搬正则(arch)
    被删 = []       # base 行号（自搬）
    i = j = 0
    while i < len(base) and j < len(new):
        if base[i] == new[j]:
            i += 1
            j += 1
            continue
        if 正则.match(base[i]):
            被删.append(i)
            i += 1
            continue
        return None, f"首个不一致 base:{i + 1} new:{j + 1}: {base[i]!r} != {new[j]!r}"
    while i < len(base):
        if 正则.match(base[i]):
            被删.append(i)
        elif base[i].strip() or (j < len(new)):
            return None, f"尾部不一致 base:{i + 1}: {base[i]!r}"
        i += 1
    if j < len(new):
        return None, f"new 尾部多余行 new:{j + 1}: {new[j]!r}"
    return 被删, ""


def 距离直方(值们):
    """精确距离 -> 次数（只打印实际出现的距离，升序）。"""
    return dict(sorted(collections.Counter(值们).items()))


def 主流程(参数):
    base = open(参数.base, encoding="utf-8", errors="replace").read().splitlines()
    new = open(参数.new, encoding="utf-8", errors="replace").read().splitlines()
    分类 = 分类_arm if 参数.arch == "arm" else 分类_x86
    被删, 错误 = 差分(base, new, 参数.arch)
    if 被删 is None:
        print(f"[红] 产物差分非「仅删自搬行」：{错误}")
        return 1
    print(f"[绿] 过滤自搬行后逐字节一致：base {len(base)} 行 → new {len(new)} 行，"
          f"删除自搬 {len(被删)} 行")
    # ① 被删自搬行的前态距离/类别
    类别计数 = collections.Counter()
    距离们 = []
    原因们 = collections.Counter()
    for idx in 被删:
        reg = 自搬寄存器(base[idx], 参数.arch)
        kind, w, z, d = 回溯(base, idx, reg, 分类)
        类别计数[f"{kind}{'+zero' if z else ''}"] += 1
        if kind in ("zero", "write") and z:
            距离们.append(d)
        else:
            原因们[f"{kind}(w={w})"] += 1
    print("  被删自搬前态写类别:", dict(类别计数))
    print("  获证前态距离（仅 zero）:", 距离直方(距离们))
    print("  被删但前态非零证明:", dict(原因们))
    # ② 残余自搬原因
    残余 = []      # new 行号
    for k, ln in enumerate(new):
        if 自搬正则(参数.arch).match(ln):
            残余.append(k)
    残余原因 = collections.Counter()
    残余距离 = []
    for k in 残余:
        reg = 自搬寄存器(new[k], 参数.arch)
        kind, w, z, d = 回溯(new, k, reg, 分类)
        if kind in ("zero", "write") and z and d <= 窗口:
            残余原因["零证明在窗内仍保留（=跨宽 64 存→32 读面）"] += 1
            残余距离.append(d)
        else:
            残余原因[f"{kind}(w={w})"] += 1
    print(f"  残余自搬 {len(残余)} 行:", dict(残余原因))
    if 残余距离:
        print("  残余跨宽面距离:", 距离直方(残余距离))
    return 0


def 自测():
    用例 = [
        # 旧窥孔产物：store 已删、load 已改写为自搬；new 再删自搬行
        (["    mov r10d, dword ptr [rbp-16]",
          "    mov r10d, r10d",
          "    add r9d, r10d"],
         ["    mov r10d, dword ptr [rbp-16]", "    add r9d, r10d"], "x86", 1, 0),
        (["    mov r10d, r10d"], ["    mov r10d, r10d"], "x86", 0, 1),
        (["    cset x3, gt", "    mov w3, w3"],
         ["    cset x3, gt"], "arm", 1, 0),
        (["    stlr w3, [x1]", "    mov w3, w3"],
         ["    stlr w3, [x1]", "    mov w3, w3"], "arm", 0, 1),
    ]
    for base, new, arch, want删, want余 in 用例:
        被删, 错误 = 差分(base, new, arch)
        if 被删 is None or len(被删) != want删:
            print(f"[自测失败] 差分 {arch}: {错误 or 被删}"); return 1
        余 = sum(1 for ln in new if 自搬正则(arch).match(ln))
        if 余 != want余:
            print(f"[自测失败] 残余 {arch}: {余} != {want余}"); return 1
    # 差分拒绝：new 改了非自搬行
    被删, 错误 = 差分(["    ret"], ["    nop"], "x86")
    if 被删 is not None:
        print("[自测失败] 非自搬行变化未被拒"); return 1
    print("[自测绿] 4 正例 + 1 负例")
    return 0


def main():
    ap = argparse.ArgumentParser(description="1279 槽窥孔产物差分与前态距离分析")
    ap.add_argument("--base")
    ap.add_argument("--new")
    ap.add_argument("--arch", choices=("x86", "arm"), default="x86")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return 自测()
    if not a.base or not a.new:
        print("需要 --base 与 --new（或 --selftest）")
        return 2
    return 主流程(a)


if __name__ == "__main__":
    sys.exit(main())
