#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""887 交叉汇编守卫序列对照门禁（835 病根立法化·机械对照替代人眼）。

背景：835 五根因之一=arm64 溢出守卫判据条件码写反（`b.gt` 序被写成 `b.le`
序：负值全放行、cmn 段成死码）——人眼逐行对照汇编的漏网概率永不为零；
425 轮交叉模拟五件套（qemu 交叉 + asm 双态对照）仍靠人工执行。本门禁把
「同一源码 × 三后端（win-x64/linux-x86_64/linux-arm64）汇编产物的关键守卫
序列语义等价」变成脚本断言：

  ① 内嵌探针源码（窄有符号/无符号加减、宽加减、乘法、移位域、空指针字段面）
     经宿主 `cn compile --target <t> -O0` 出 .s（只产汇编、不调 as/ld——任意
     平台可跑；-O0 守卫形态最全、不受折叠改写——835 缺陷在该形态亦复现）；
  ② 从各平台 asm 提取「到达运行时错误块（call __cn_runtime_error）的路径
     谓词」：从错误块向前线性扫描（cmp/cmn/test/标志设置指令 + 条件跳转链 +
     顺序落入），归一为「错误判定函数」——平台语法差异（x64 jcc / arm64 b.xx、
     cmn 取负等价、标志位检测、乘法恒等式、shr+jz 高半检测）由模板表消解；
  ③ 每个平台的守卫多重集（错误码 × 采样判定向量）与**语言语义期望表**逐类
     对照（期望=001 §3.7a 溢出错误码 8 / 空指针错误码 3 / 移位域判据），并附
     三平台互照（同判定向量多重集必须一致）。条件码笔误（方向反转、家族混用、
     边界常量改值、分支落点错）必在采样点暴露——机械可证，不靠人眼。

文件结构：本文件=逻辑面；探针源码/语义期望表/条件码映射数据面在
`scripts/cross_asm_guards_data.py`（887 拆分·单一职责）。

判定强度与边界（诚实声明·不假装全覆盖）：
  - 采样集覆盖全部类别边界 ±2 邻域：单界/双界常量笔误与方向反转必暴露
    （阶跃边界不同 ⇒ 边界邻域必有分歧点）；另并入产物动态常量邻域（保险）。
    32 位窗口承载的类别按可达数学域裁剪采样（窗口内无截断·等价性成立）。
  - 标志原子（jno/jnc/b.vc/b.cc…）语义化=架构手册公理硬编码：add/sub/imul
    的 OF 置位 ⟺ 数学结果越 [−2^(w−1), 2^(w−1)−1]；add/sub/mul 的 CF 置位
    ⟺ 数学结果越 [0, 2^w−1]（模型见各后端发射器与 plans/021 D 族）。
  - 「顺序落入」的可达性：前驱段以无条件跳转结束且无「跳进本段」的引用=
    死码，判「落入不存在」（835 反转形态的 cmn 死码段即由此显形——落入谓词
    若勉强算出会把 835 反转误判等价，见自检负样本 1）；被引用 label 的段落
    入口按「局部谓词」口径（段内链），前置守卫上下文不并入——串联冗余守卫
    （同值重复判零/溢出）的完整路径条件恒空集，而守卫条件码方向正确性由
    本段谓词体现。
  - 移位守卫判定对象=移位量（IR 现状移位量 32 位承载——超 32 位移位量面不在
    本门禁覆盖，另案立项）。
  - 「未识别形态」= 红（miss 不可静默）：模板覆盖不到的错误块形态显式报红并
    给出上下文——禁静默跳过；合法重构导致守卫清单变化时更新期望表即可。

用法：python scripts/check_cross_asm_guards.py [--cn target/cn.exe]
退出码：0 = 全过；1 = 违例（自检失败/编译失败/对照不符）。
"""
import argparse
import concurrent.futures
import io
import pathlib
import re
import subprocess
import sys
import tempfile
import time

try:  # 控制台编码（中文诊断防 GBK 崩——与 check_asm_width 同口径）
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
except Exception:
    pass

仓库根 = pathlib.Path(__file__).resolve().parent.parent

# 887：探针/期望表/条件码映射数据面拆至 cross_asm_guards_data（单一职责·
#   主脚本 1000 行红线内）；同目录导入（用户级 sys.path 不含 scripts/）。
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from cross_asm_guards_data import (探针源码, 期望类别, 期望计数, 采样点, 自检样本,
                                   比较码, 相等码, 不等码, 标志码, 条件跳转集,
                                   运算类, 标志指令集)  # noqa: E402

def 类别匹配(g, 类别项, 点们):
    """守卫 g 是否与类别项（名, 码, f, 裁剪域）语义等价（在其判定域上）。"""
    名, 码, f, 域 = 类别项
    if g.错误码 != 码:
        return False
    点列 = [x for x in 点们 if 域 is None or 域[0] <= x <= 域[1]]
    return g.向量(点列) == tuple(f(x) for x in 点列)


def 寄存器家族(名):
    """归一寄存器家族（忽略宽度）：x9/w9 -> ("arm",9)；rax/eax/ax/al -> ("x86","ax")；
    r10/r10d/r10b -> ("x86",10)。"""
    名 = 名.strip().lstrip("#")
    m = re.fullmatch(r"[xw](\d+)", 名)
    if m:
        return ("arm", int(m.group(1)))
    m = re.fullmatch(r"r(\d+)([dwb])?", 名)
    if m:
        return ("x86", int(m.group(1)))
    m = re.fullmatch(r"[re]?(ax|bx|cx|dx|si|di|bp|sp)", 名)
    if m:
        return ("x86", m.group(1))
    if re.fullmatch(r"[abcd][lh]", 名):
        return ("x86", {"a": "ax", "b": "bx", "c": "cx", "d": "dx"}[名[0]])
    return None


def 宽度规范(名):
    """x86 家族名/arm 名 -> 位宽（严格映射·表驱动）。"""
    名 = 名.strip()
    m = re.fullmatch(r"r(\d+)([dwb])?", 名)
    if m:
        return {"d": 32, "w": 16, "b": 8, None: 64}[m.group(2)]
    m = re.fullmatch(r"([er]?)(ax|bx|cx|dx|si|di|bp|sp)", 名)
    if m:
        if m.group(1) == "r":
            return 64
        if m.group(1) == "e":
            return 32
        return 16
    m = re.fullmatch(r"[abcd]([lh])", 名)
    if m:
        return 8
    return None


def _寄存器宽(名):
    """arm64 xN/wN 与 x86 全家族 -> 位宽 or None。"""
    名 = (名 or "").strip().lstrip("#")
    m = re.fullmatch(r"[xw](\d+)", 名)
    if m:
        return 64 if 名[0] == "x" else 32
    return 宽度规范(名)


def 符号扩展(v, 宽):
    v &= (1 << 宽) - 1
    return v - (1 << 宽) if v >> (宽 - 1) else v


# ============ asm 行解析 ============
标签正则 = re.compile(r"^\s*([@\.\w$]+)\s*:\s*$")
指令正则 = re.compile(r"^\s*([a-z][a-z0-9\.]*)\s*(.*?)\s*$")
非指令词 = {
    "data", "text", "byte", "word", "long", "quad", "string", "asciz", "zero",
    "globl", "type", "size", "extern", "section", "file", "align", "p2align",
    "segment", "ends", "endp", "proc", "public", "dword", "qword", "db", "dw",
    "dd", "dq", "org", "equ", "set", "end", "include", "def", "assume",
}


def 剥注释(行):
    i = 行.find("//")
    if i >= 0:
        行 = 行[:i]
    i = 行.find(";")   # win 行尾注释；产物无字符串字面量/分号语句（声明如上）
    if i >= 0:
        行 = 行[:i]
    return 行.rstrip()


def 解析行(行):
    行 = 剥注释(行)
    if not 行.strip():
        return None
    m = 标签正则.match(行)
    if m:
        return ("label", m.group(1))
    m = 指令正则.match(行)
    if m and m.group(1) not in 非指令词:
        return ("insn", (m.group(1), m.group(2)))
    return ("other", 行.strip())


def 操作数们(文本):
    return [s.strip() for s in 文本.split(",") if s.strip()]


def 无条件跳(助记, 文本):
    return 助记 == "jmp" or (助记 == "b" and not 文本.startswith("."))


def 是终结(助记, 文本):
    return 助记 == "ret" or 无条件跳(助记, 文本)


def 是条件跳转(助记):
    return 助记 in 条件跳转集


# ============ 谓词原子 ============
class 原子:
    def __init__(self, f, 描述, 常量们=()):
        self.f = f                # 语义：该条件分支「条件成立」
        self.描述 = 描述
        self.常量们 = list(常量们)


def 原子_比较(imm, 符号性, 算子, 宽, 描述):
    def f(x):
        v = x & ((1 << 宽) - 1)
        vv = 符号扩展(v, 宽) if 符号性 == "s" else v
        return {">": vv > imm, ">=": vv >= imm, "<": vv < imm,
                "<=": vv <= imm, "==": vv == imm, "!=": vv != imm}[算子]
    return 原子(f, 描述, (imm,))


def 原子_标志(标志, 宽, 极性, 描述):
    """标志置位语义（架构公理·模板硬编码，见文件头声明）。"""
    if 标志 == "of":
        lo, hi = -(1 << (宽 - 1)), (1 << (宽 - 1)) - 1
    else:
        lo, hi = 0, (1 << 宽) - 1

    def f(x):
        return (not (lo <= x <= hi)) == 极性
    return 原子(f, 描述)


def 原子_域内(lo, hi, 正极性, 描述):
    """乘法恒等式/高半检测模板 -> 数学域（恒等成立 ⟺ 积在 32/64 位域内——
    证明见各后端乘法守卫实现注释；正极性=True 表示条件成立=域内）。"""
    def f(x):
        return (lo <= x <= hi) == 正极性
    return 原子(f, 描述, (lo, hi))


def 原子_移位零(reg宽, K, 极性, 描述):
    """x86 `shr R, K; jz/jnz`：R 机器值右移 K 为零否（极性=True=条件成立=零）。"""
    def f(x):
        return (((x & ((1 << reg宽) - 1)) >> K) == 0) == 极性
    return 原子(f, 描述, (K,))


# ============ 单条跳转 -> 条件成立原子（模板解析） ============
def 查常量寄存器(行们, 起点行, 目标名):
    """从起点行向前在同段内找目标寄存器家族最近的 mov/movz 立即数装载。
    窗口内出现任何写同家族的非装载指令/call/边界 -> None（不认识就给 None——
    由调用方报「未识别」红，不静默）。"""
    家族 = 寄存器家族(目标名)
    if 家族 is None:
        return None
    i = 起点行 - 1
    while i >= 0:
        it = 行们[i]
        if it is None:
            i -= 1
            continue
        if it[0] == "label":
            return None
        if it[0] != "insn":
            i -= 1
            continue
        助记, 文本 = it[1]
        if 助记 in ("call", "bl"):
            return None
        操作数 = 操作数们(文本)
        if 助记 in ("mov", "movz", "movw") and 操作数:
            if 寄存器家族(操作数[0]) == 家族 and len(操作数) >= 2:
                m = re.fullmatch(r"#?(0[xX][0-9a-fA-F]+|\d+)", 操作数[1].strip())
                return int(m.group(1), 0) if m else None
        else:
            if 操作数 and 寄存器家族(操作数[0]) == 家族:
                return None  # 该家族被其它指令写过——常量信息不可信
        if 是终结(助记, 文本):
            return None
        i -= 1
    return None


def 找最近标志指令(行们, 跳转行):
    i = 跳转行 - 1
    while i >= 0:
        it = 行们[i]
        if it is None:
            i -= 1
            continue
        if it[0] == "label":
            return None, "块边界（跳转与标志指令间无 label 的直发形态被打破）"
        if it[0] != "insn":
            i -= 1
            continue
        助记, 文本 = it[1]
        if 助记 in ("call", "bl"):
            return None, "call/bl 破坏标志"
        if 助记 in 标志指令集:
            return i, None
        if 是终结(助记, 文本):
            return None, "终结指令阻断"
        i -= 1
    return None, "至文件头未见标志指令"


def 解析跳转原子(行们, 跳转行, 条件码, 跳转文本):
    """-> (原子, None) 或 (None, 原因)。原子语义 = 「该分支条件成立」。"""
    idx, 原因 = 找最近标志指令(行们, 跳转行)
    if idx is None:
        return None, f"最近标志指令未找到：{原因}"
    助记, 文本 = 行们[idx][1]
    操作数 = 操作数们(文本)
    原描述 = f"{助记} {文本} ｜ {条件码} {跳转文本}"

    # ---- 相等/不等类（判零、移位零、乘法恒等）----
    if 条件码 in ("cbz", "cbnz"):
        # 908：umulh 与 cbz 相邻即恒等成立的反向结构（arm64 u64 乘法——
        #   `umulh x12, …; cbz x12, ok`）。原 `近` 取 idx 之前三行——真实
        #   arm64 u64 序列里 umulh 就是最近标志指令（idx 行自身），永不在
        #   前窗内 → 该分支从未命中过（887 探针缺 u64 乘法样本·同轮补齐）。
        #   修法=近窗口含 idx 行（umulh 命中）；对既有样本含 cmp 家族面
        #   零影响（它们走下方 相等/不等 分支，不达本支）。
        近 = [行们[j][1][0] for j in range(max(0, idx - 3), idx + 1)
              if 行们[j] and 行们[j][0] == "insn"]
        if "umulh" in 近:
            return 原子_域内(0, 2 ** 64 - 1, 条件码 == "cbz",
                           f"umulh 高半恒等式（{原描述}）"), None
        return None, f"cbz/cbnz 非乘法恒等形态：{原描述}"

    if 条件码 in 相等码 or 条件码 in 不等码:
        正 = 条件码 in 相等码   # True=条件成立=「==0」/域内
        if 助记 == "cmp" and len(操作数) >= 3:
            # 乘法恒等式校对（cmp X12, W12, sxtw / cmp X12, X10, asr #63）
            后缀 = 操作数[2].replace(" ", "")
            近 = [行们[j][1][0] for j in range(max(0, idx - 3), idx)
                  if 行们[j] and 行们[j][0] == "insn"]
            if 后缀 == "sxtw" and "smull" in 近:
                return 原子_域内(-(2 ** 31), 2 ** 31 - 1, 正,
                               f"smull 回扩恒等式（{原描述}）"), None
            if 后缀 == "uxtw" and "umull" in 近:
                return 原子_域内(0, 2 ** 32 - 1, 正,
                               f"umull 回扩恒等式（{原描述}）"), None
            if 后缀 == "asr#63" and "smulh" in 近:
                return 原子_域内(-(2 ** 63), 2 ** 63 - 1, 正,
                               f"smulh 符号位恒等式（{原描述}）"), None
            return None, f"三操作数 cmp 非乘法恒等式模板：{原描述}"
        if 助记 == "cmp":
            第一 = 操作数[0] if 操作数 else ""
            第二 = 操作数[1].lstrip("#") if len(操作数) > 1 else ""
            if 第二 == "0":
                宽 = _寄存器宽(第一)
                if 宽 is None:
                    return None, f"cmp 操作数寄存器不可识别：{文本}"
                return 原子_比较(0, "s", "==" if 正 else "!=", 宽, 原描述), None
            return None, f"相等/不等条件码但比较对象非常量零：{文本}"
        if 助记 == "test":
            if len(操作数) >= 2 and 操作数[0] == 操作数[1]:
                宽 = _寄存器宽(操作数[0])
                if 宽 is None:
                    return None, f"test 寄存器不可识别：{文本}"
                return 原子_比较(0, "u", "==" if 正 else "!=", 宽, 原描述), None
            return None, f"test 非自比较形态：{文本}"
        if 助记 in ("shr", "sar", "shl"):
            if len(操作数) >= 2:
                m = re.fullmatch(r"#?(\d+)", 操作数[1].strip())
                宽 = _寄存器宽(操作数[0])
                if m and 宽:
                    return 原子_移位零(宽, int(m.group(1)), 正, 原描述), None
            return None, f"移位+相等条件形态不可解析：{原描述}"
        return None, f"相等/不等条件码无模板（前驱 {助记}）：{原描述}"

    # ---- 比较类 ----
    if 条件码 in 比较码:
        if 助记 == "cmp":
            if len(操作数) >= 3:
                return None, f"三操作数 cmp 配比较条件码未识别：{原描述}"
            第一 = 操作数[0] if 操作数 else ""
            宽 = _寄存器宽(第一)
            if 宽 is None:
                return None, f"cmp 第一操作数不可识别：{文本}"
            if len(操作数) < 2:
                return None, f"cmp 缺第二操作数：{文本}"
            第二 = 操作数[1].lstrip("#")
            m = re.fullmatch(r"-?(0[xX][0-9a-fA-F]+|\d+)", 第二)
            if m:
                imm = int(第二, 0)
            else:
                # 常量寄存器形态（arm64 movz x12,#32767; cmp w9,w12）——查装载
                imm = 查常量寄存器(行们, idx, 操作数[1])
                if imm is None:
                    return None, f"cmp 第二操作数非常量（寄存器无装载记录）：{文本}"
            符号性, 算子 = 比较码[条件码]
            return 原子_比较(imm, 符号性, 算子, 宽, 原描述), None
        if 助记 == "cmn" and 条件码 in 比较码:
            符号性, 算子 = 比较码[条件码]
            if 符号性 != "s":
                return None, f"cmn 与无符号条件码组合未识别：{原描述}"
            if len(操作数) >= 2:
                m = re.fullmatch(r"#?(\d+)", 操作数[1].strip())
                if m:
                    imm = -int(m.group(1))
                    宽 = _寄存器宽(操作数[0])
                    return 原子_比较(imm, "s", 算子, 宽, 原描述), None
            return None, f"cmn 操作数不可解析：{原描述}"
        # 运算前驱的无符号比较码家族（jb==jc 等）
        if 条件码 in 标志码 and 助记 in 运算类:
            标志, 极性 = 标志码[条件码]
            宽 = _寄存器宽(操作数[0]) if 操作数 else None
            if 宽 is None:
                return None, f"运算操作数不可识别：{文本}"
            return 原子_标志(标志, 宽, 极性, f"标志（{标志}·{宽}位）：{原描述}"), None
        return None, f"比较条件码但前驱非 cmp/cmn（{助记}）：{原描述}"

    # ---- 标志类 ----
    if 条件码 in 标志码:
        标志, 极性 = 标志码[条件码]
        if 助记 not in 运算类:
            return None, f"标志条件码但前驱非运算类（{助记}）：{原描述}"
        宽 = _寄存器宽(操作数[0]) if 操作数 else None
        if 宽 is None:
            return None, f"运算操作数不可识别：{文本}"
        return 原子_标志(标志, 宽, 极性, f"标志（{标志}·{宽}位）：{原描述}"), None

    return None, f"条件码无模板：{条件码}"


# ============ 错误块解析 ============
class 守卫:
    def __init__(self, 平台, 行号, 错误码, 链们, 描述们, 常量们):
        self.平台 = 平台
        self.行号 = 行号              # 错误码装载行（1 基）
        self.错误码 = 错误码
        self.链们 = 链们              # 每条入边 = [(原子, 取否)]
        self.描述们 = 描述们
        self.常量们 = 常量们

    def 判定(self, x):
        for 链 in self.链们:
            if all((not a.f(x)) if 取否 else a.f(x) for a, 取否 in 链):
                return True
        return False

    def 向量(self, 点们):
        return tuple(self.判定(x) for x in 点们)

    def 摘要(self):
        return " ∨ ".join(
            " ∧ ".join(("¬" if 取否 else "") + (a.描述 if a else "?")
                       for a, 取否 in 链) for 链 in self.链们)


def 剥行(文本):
    行们 = []
    for 原 in 文本.split("\n"):
        行们.append(解析行(原))
    return 行们


def 错误块信息(行们, call行):
    """从 call 行向前定位错误块：错误码装载行 + 起始 label（可无）。
    错误码装载在 call 前 4 行内（win: mov rcx,8 / arm64: movz x0,#8 /
    lx64: mov rdi,8——发射模板使装载紧邻 call）。"""
    mov行, 错误码 = None, None
    for i in range(max(0, call行 - 4), call行):
        it = 行们[i]
        if it and it[0] == "insn":
            助记, 文本 = it[1]
            if 助记 in ("mov", "movz", "movw", "movl"):
                操作数 = 操作数们(文本)
                if len(操作数) >= 2:
                    m = re.fullmatch(r"#?(0[xX][0-9a-fA-F]+|\d+)", 操作数[1].strip())
                    if m:
                        mov行, 错误码 = i, int(m.group(1), 0)
    return mov行, 错误码


def 解析守卫们(文本, 平台):
    行们 = 剥行(文本)
    守卫们, 报告们 = [], []
    # 跳转引用图：label 名 -> [(行号, 助记, 文本)]（条件/无条件都记）
    引用 = {}
    全label行 = {}
    for i, it in enumerate(行们):
        if it is None:
            continue
        if it[0] == "label":
            全label行[it[1]] = i
        elif it[0] == "insn":
            助记, 文本i = it[1]
            if (是条件跳转(助记) or 无条件跳(助记, 文本i)) and 文本i:
                目标 = 操作数们(文本i)[-1]
                引用.setdefault(目标, []).append((i, 助记, 文本i))

    for call行, it in enumerate(行们):
        if not (it and it[0] == "insn"):
            continue
        助记, 文本i = it[1]
        if 助记 not in ("call", "bl") or "__cn_runtime_error" not in 文本i:
            continue
        mov行, 错误码 = 错误块信息(行们, call行)
        if mov行 is None:
            报告们.append(f"守卫@{call行 + 1} 错误码未识别（call 前 4 行无 mov 立即数）")
            continue
        # 错误块 label（可无）
        errlabel = None
        p = mov行 - 1
        while p >= 0 and 行们[p] is None:
            p -= 1
        if p >= 0 and 行们[p] and 行们[p][0] == "label":
            errlabel = 行们[p][1]
        # ok 相关 label（错误块后第一个 label——解析失败的跳转目标是否「守卫相关」判据）
        近label = set()
        for i in range(max(0, mov行 - 40), min(len(行们), call行 + 40)):
            if 行们[i] and 行们[i][0] == "label":
                近label.add(行们[i][1])

        链们, 描述们, 常量们, 问题们 = [], [], [], []

        def 加边(链, 描述):
            if 链 is None:
                return
            链们.append(链)
            描述们.append(描述)
            for a, _n in 链:
                if a:
                    常量们.extend(a.常量们)

        # 扫描函数：从 起点行 向前扫描守卫段（含起点），累积条件跳转的否定。
        # 返回 (链, 停止标签)；停止标签：
        #   "entry"        函数/段入口界（文件头/上一函数尾）·可达
        #   "ret"          上一函数尾（同 entry·可达）
        #   "uncond"       无条件跳终结——落入需检查「跳入引用」（死码判定点）
        #   "entry_label:L" 被引用 label L——段落入口（前置守卫上下文——按
        #                  「局部谓词」口径不参与对照：串联冗余守卫（同值
        #                  重复判零）的完整路径条件恒空集，但守卫自身条件码
        #                  正确性=对照面；方向错误由本段谓词显形）
        #   "foreign:X"    无关分支（停止标志，不报红）
        #   (None,None)    解析失败（已报红）
        def 扫描(起点行):
            链 = []
            i = 起点行
            while i >= 0:
                it = 行们[i]
                if it is None:
                    i -= 1
                    continue
                if it[0] == "label":
                    if errlabel is not None and it[1] == errlabel:
                        i -= 1   # 错误块自身 label 透明
                        continue
                    if 引用.get(it[1]):
                        return 链, "entry_label:" + it[1]
                    i -= 1       # 无引用 label（顺序流入标记）透明
                    continue
                if it[0] == "insn":
                    助记i, 文本x = it[1]
                    if 是终结(助记i, 文本x):
                        停止类 = "uncond" if 无条件跳(助记i, 文本x) else "ret"
                        return 链, 停止类
                    if 是条件跳转(助记i):
                        原子a, 原因 = 解析跳转原子(行们, i, 助记i, 文本x)
                        if 原子a is not None:
                            链.append((原子a, True))
                        else:
                            目标 = 操作数们(文本x)[-1] if 文本x else ""
                            if 目标 not in 近label:
                                return 链, "foreign:" + 目标
                            问题们.append(
                                f"守卫@{mov行 + 1}（错误码 {错误码}）守卫段分支 "
                                f"{助记i} {文本x}（行 {i + 1}）解析失败：{原因}")
                            return None, None
                i -= 1
            return 链, "entry"

        def 引用边链(j行, j助记, j文本):
            """构造「经由某条跳转进入（或跳向 E）」的完整链。"""
            链 = 扫描(j行 - 1)
            if 链[0] is None:
                return None
            链 = list(链[0])
            if not 无条件跳(j助记, j文本):
                原子a, 原因 = 解析跳转原子(行们, j行, j助记, j文本)
                if 原子a is None:
                    return None
                链.append((原子a, False))
            return 链

        # ---- 边 A：跳转到错误块 label 的指令 ----
        if errlabel is not None:
            for (j行, j助记, j文本) in 引用.get(errlabel, []):
                链 = 引用边链(j行, j助记, j文本)
                if 链 is None:
                    问题们.append(f"守卫@{mov行 + 1}（错误码 {错误码}）跳 err 的分支 "
                                f"{j助记} {j文本}（行 {j行 + 1}）链路解析失败")
                    continue
                加边(链, f"{j助记} {j文本} -> err（行 {j行 + 1}）")

        # ---- 边 B：顺序落入 ----
        # 落入点：错误码装载行向前跳过 label/空行的第一条指令
        p = mov行 - 1
        while p >= 0 and (行们[p] is None or 行们[p][0] == "label"):
            p -= 1
        if p >= 0 and 行们[p] and 行们[p][0] == "insn" and not 是终结(*行们[p][1]):
            链, 停止 = 扫描(p)
            if 链 is None:
                pass                                   # 已报红
            elif 停止 == "uncond":
                # 前驱段以无条件跳转结束——落入须经由「跳进本段」的引用；
                # 无引用=死码（835 反转形态的 cmn 死码段在此显形——落入谓词
                # 若勉强算出会把反转形态误判等价）。
                区间 = range(p + 1, mov行)
                被引 = any(行们[q] and 行们[q][0] == "label" and
                          行们[q][1] != errlabel and 引用.get(行们[q][1])
                          for q in 区间)
                if 被引:
                    报告们.append(f"守卫@{mov行 + 1}（错误码 {错误码}）落入段入口为"
                                f"无条件跳后引用区——形态未覆盖，请扩展模板")
                # 无引用：落入不存在（正确判定·不加边）
            elif 停止.startswith("entry_label:"):
                # 段落入口为被引用 label——按「局部谓词」口径：本段链即守卫
                # 判定语义（入口 label 的前置守卫上下文不并入——串联冗余守卫
                # （同值重复判零/溢出）的完整路径条件恒空集，而守卫条件码
                # 方向正确性由本段谓词体现；入口名保留在描述里供人工追查）。
                入口label = 停止.split(":", 1)[1]
                加边(链, f"顺序落入（段内谓词·入口 {入口label} 前置上下文省略）")
            else:
                加边(链, "顺序落入 err")

        if 问题们:
            报告们.extend(问题们)
            continue
        if not 链们:
            报告们.append(f"守卫@{mov行 + 1}（错误码 {错误码}）零入边——形态未识别")
            continue
        守卫们.append(守卫(平台, mov行 + 1, 错误码, 链们, 描述们, 常量们))
    return 守卫们, 报告们


# ============ 自检（探测器自身也是被测对象——110-a 教训） ============
def _类别(名):
    return next(t for t in 期望类别 if t[0] == 名)


def 自检() -> tuple:
    """探测器自测（样本数据面=自检样本·908 外置拆分）：正样本须判等价、
    负样本须判红（探测器自身亦被测·110-a）。返回 (正样本数, 负样本数)。"""
    失败 = []

    def 单样本(名称, 文本, 类别名, 应通过=True):
        守卫们, 报告们 = 解析守卫们(文本, "自检")
        if 报告们 or len(守卫们) != 1:
            失败.append(f"{名称}：解析异常 {报告们 or f'{len(守卫们)} 守卫'}")
            return
        实际通过 = 类别匹配(守卫们[0], _类别(类别名), 采样点)
        if 应通过 and not 实际通过:
            失败.append(f"{名称}：对照不符 {守卫们[0].摘要()}")
        if not 应通过 and 实际通过:
            失败.append(f"{名称}：笔误样本未被捕获（误判等价）")

    正数 = 负数 = 0
    for 名称, 文本, 类别名, 应通过 in 自检样本:
        if 应通过:
            正数 += 1
        else:
            负数 += 1
        单样本(名称, 文本, 类别名, 应通过)

    if 失败:
        raise SystemExit("自检失败：\n  " + "\n  ".join(失败))
    return 正数, 负数


def 找编译器(参数路径):
    if 参数路径:
        p = pathlib.Path(参数路径)
        return p if p.exists() else None
    for 候选 in ("target/cn.exe", "target/cn", "target/Debug/cn.exe",
                 "target/Debug/cn", "target/build-ninja/cn.exe",
                 "target/build-quick/cn"):
        p = 仓库根 / 候选
        if p.exists():
            return p
    return None


def main():
    ap = argparse.ArgumentParser(description="887 交叉汇编守卫序列对照门禁")
    ap.add_argument("--cn", default=None, help="cn 编译器路径（默认自动探测）")
    ap.add_argument("--jobs", type=int, default=3)
    args = ap.parse_args()

    t0 = time.time()
    正数, 负数 = 自检()
    print("== check_cross_asm_guards（887·交叉汇编守卫序列对照·835 病根立法化）==")
    print(f"自检：正 {正数} 负 {负数} 样本全过（探测器自身亦被测·样本表 908 外置）")

    cn = 找编译器(args.cn)
    if cn is None:
        print("✗ 未找到 cn 编译器（--cn 指定或先构建 target/cn）")
        return 1

    平台们 = ["win-x64", "linux-x86_64", "linux-arm64"]
    with tempfile.TemporaryDirectory(prefix="cn_guards_") as td:
        td = pathlib.Path(td)
        (td / "探针.cn").write_text(探针源码, encoding="utf-8")
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futs = {}
            for t in 平台们:
                futs[t] = pool.submit(
                    subprocess.run,
                    [str(cn), "compile", str(td / "探针.cn"), "--target", t,
                     "-O0", "--output", str(td / f"{t}.s")],
                    cwd=str(仓库根), capture_output=True, timeout=180)
            失败们 = []
            for t, fut in futs.items():
                r = fut.result()
                if r.returncode != 0 or not (td / f"{t}.s").exists():
                    尾 = (r.stderr or b"").decode("utf-8", "replace").strip().splitlines()
                    失败们.append(f"{t}: rc={r.returncode} "
                                f"{尾[-1] if 尾 else '无 .s 产物'}")
            if 失败们:
                print("✗ 探针编译失败：")
                for f in 失败们:
                    print(f"   {f}")
                return 1

        全守卫, 全报告 = {}, {}
        for t in 平台们:
            text = (td / f"{t}.s").read_text(encoding="utf-8", errors="replace")
            守卫们, 报告们 = 解析守卫们(text, t)
            全守卫[t], 全报告[t] = 守卫们, 报告们

        # 动态采样（并入产物谓词常量邻域）
        动态 = set(采样点)
        for 守卫们 in 全守卫.values():
            for g in 守卫们:
                for c in g.常量们:
                    if -(2 ** 65) < c < 2 ** 65:
                        # 908：邻域点逐点过滤到 64 位承载窗口内——u64 域内
                        #   常量 2^64-1 的邻域含 2^64，该点对「机器窗口原子」
                        #   型守卫（判零 test/cmp、移位高半 shr）会按 64 位截断
                        #   求值而非数学值，与期望判定（数学值）失配——使
                        #   判零/无符号32 等类别在引入 u64 乘法探针后集体
                        #   「未登记」（908 首跑实录）。窗口外数学值自洽面由
                        #   负数越界点（-1 等）覆盖；越 ±2^64 点不承载。
                        动态.update({p for p in (c - 1, c, c + 1)
                                    if -(2 ** 64) < p < 2 ** 64})
        点们 = sorted(动态)

        总失败 = 0
        归类 = {}   # 平台 -> [(守卫, 命中类别名 or None)]
        print(f"编译器 {cn}")
        for t in 平台们:
            守卫们, 报告们 = 全守卫[t], 全报告[t]
            实际分布, 未登记, 本归类 = {}, [], []
            for g in 守卫们:
                命中 = next((项[0] for 项 in 期望类别
                             if 类别匹配(g, 项, 点们)), None)
                本归类.append((g, 命中))
                if 命中 is None:
                    未登记.append(g)
                else:
                    实际分布[命中] = 实际分布.get(命中, 0) + 1
            归类[t] = 本归类
            差异 = [f"{名} 实际 {实际分布.get(名, 0)} 期望 {计数}"
                    for 名, 计数 in 期望计数.items()
                    if 实际分布.get(名, 0) != 计数]
            总失败 += len(报告们) + len(未登记) + len(差异)
            if not (报告们 or 未登记 or 差异):
                分布文本 = "、".join(f"{名}×{实际分布[名]}"
                                     for 名, _c, _f, _d in 期望类别 if 名 in 实际分布)
                print(f"  ✓ {t}：{len(守卫们)} 守卫全过（{分布文本}）")
            else:
                print(f"  ✗ {t}：{len(守卫们)} 守卫")
                for 报 in 报告们[:10]:
                    print(f"      未识别：{报}")
                for g in 未登记[:10]:
                    print(f"      未登记形态：行 {g.行号} 错误码 {g.错误码} "
                          f"谓词 = {g.摘要()}")
                for d in 差异[:10]:
                    print(f"      计数差异：{d}")

        # 跨平台互照（按类别裁剪域内向量分组——冗余防线）
        互照失败 = 0
        if all(not 全报告[t] for t in 平台们):
            from collections import Counter
            签名 = {}
            for t in 平台们:
                签名[t] = Counter()
                for g, 命中 in 归类[t]:
                    if 命中 is None:
                        continue
                    域 = _类别(命中)[3]
                    点列 = [x for x in 点们 if 域 is None or 域[0] <= x <= 域[1]]
                    签名[t][(命中, g.向量(点列))] += 1
            if not (签名[平台们[0]] == 签名[平台们[1]] == 签名[平台们[2]]):
                互照失败 = 1
                print("  ✗ 跨平台互照：三后端守卫判定向量多重集不一致")
                for t in 平台们:
                    种类 = len({k for k in 签名[t]})
                    print(f"      {t}: {sum(签名[t].values())} 守卫 / {种类} 种判定")

    if 总失败 or 互照失败:
        print(f"FAIL（{总失败 + 互照失败} 项——守卫序列跨平台对照不符；"
              f"如属合法重构请更新期望表）｜{time.time() - t0:.1f}s")
        return 1
    print(f"PASS（三后端守卫序列与语义期望逐类一致·机械对照替代人眼）"
          f"｜{time.time() - t0:.1f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
