#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_sentinel_gate.py —— 哨兵与侥幸模式机械门禁（#1640·1360 结果化终态配套）。

病根（#1640）：1360「哨兵禁令全面化·存量一律结果化」完成后，**新代码再写哨兵**
仍靠人眼——存量结果化清完，回归入口没有任何机械拦截。本门禁补三类识别族 +
一条侥幸路径清点面（词面+形态双判据·可解释·误报走协议注释豁免通道）。

四面（族）判定条件：
  ①「负一哨兵」=-1 失败返回：代码行出现 `return -1;` / `返回 -1;`（可带括号·
    可 -1.0）。语义：内部 API 以 -1 表失败/未找到（#1360 禁；合法业务值如三路
    比较的 -1 走存量基线/协议注释豁免）。机械排除：注释与字符串内不判（掩码后）。
  ②「零重叠」=0 表未找到/失败：`return 0;` / `返回 0;`，且
    (a) 同分支条件命中查找未命中信号——显式词面（未找到/找不到/查不到/未命中/
        不存在/查找失败/查表失败/没找到）、`== -1` 等值、`< 0` 负值判定、
        `== …end()` 迭代器末等值；或
    (b) 同一行/上一行注释写明未找到族词面。
    说明：**`.empty()` 单独不作 miss 信号**（空容器是状态不是查找未命中；空串
    计数 0 是真业务值——#1640 校准实测该信号几乎全为误报，属定义收窄非放宽）。
    同一行/下行首返形态与 882 静默门禁同构（锚=if 行），一条协议注释可同时满足
    两门禁（登记表共用）。
  ③「目标尺寸拷贝」=按目标宽度整块搬（#1592 同族）：
    (a) host `memcpy/memmove(dst, src, sizeof(dst))`——第三参取目标表达式 sizeof；
    (b) host `emit(Opcode::CopyStruct, {dst,src}, …, size, …)` 的 size 参数
        （第 4 实参）含目标词面（目标/dst/dest/target/左类型）；
    (c) v2 `建IR指令(IR_结构体拷贝, 源, 目标, 0, 0, size, 0)` 的 size 参数
        （第 6 实参）含目标词面，或 size 为标识符且同函数回看 60 行内由
        `结果大小/结构体大小/对象大小/大小/sizeof/typeSizeOf(目标类型…)` 赋值。
    安全证明排除：拷贝语句前 8 行代码内有**同型/等价证明**（源码相等比较：
    `源… == 目标…`、`字符串比较(源…`、`tSame/eSame`、`srcCanon == dstCanon`）
    = 先证同型再整块拷（#1592 修复后的正确路径），不判。注释声称不算（只看代码）。
  ④「源型顶替」=侥幸路径清点：`源…类型/键/canon = …目标…类型/键…;` 形态——
    源类型未证明即按目标类型同型处理（#1592 残留「解析失败=旧整块拷贝」路径类）。
    存量=清点冻结，新增=红。

门禁三红（与 check_miss_gate.py 同构）：
  ①新增位置无豁免（不在冻结基线·无协议注释+登记条）；
  ②漏登记（代码有协议注释而 scripts/miss_registry.json 无条）；
  ③协议注释类别非法 / 登记无批准 / 已毙未清偿（#1238 批准制复用）。
  `// 静默豁免<类别>: 原因` 协议注释（同行或上一行）+ miss_registry.json 登记条
  （批准=已批|存量过渡）=唯一合法豁免通道。登记表共用：一条豁免同时满足 882
  静默门禁与本门禁（同位置时）。

冻结基线（只减不增·scripts/sentinel_freeze_baseline.json）：键=「文件::归一锚」，
同键重复=计数。基线内位置不红；新增位置红；键消失=消减提示（自动棘轮·不改红）。
基线记录「规则版本 + 规则指纹 + 生成sha」——同树同规则重跑逐字节可复现；规则
指纹变化（改了判据）打印黄字提示，须复核消减后 --update-baseline 收账。

诚实边界（不假装全覆盖）：
  ① 语义级哨兵机器不可全识——`int 码 = -1;` 再 `return 码;`（变量中转）、
     `return 静态转换<int>(-1);` 不判（只抓字面量返回形态·可解释性优先）。
  ② 0 重叠只抓「分支条件 miss 信号 + 返回 0」与「注释明写未找到」两形态；
     裸 `return 0;` 在无信号函数末不判（业务值概率高·禁噪声淹没）。
  ③ 目标尺寸拷贝只看显式实参/同函数近邻赋值，穿函数抽象（如
     emitStructCopyWithFields 内部按单类型取宽）不可见——这类由 #1592 修复
     的「先证同型」契约与评审承担。
  ④ 协议注释/登记条复用 882 的锚相容口径（归一后双向子串）；同文件同锚多位置
     的条目可能宽匹配——新增位置仍需**同行协议注释**才放行（显式动作不可省）。
  ⑤ 只读（--update-baseline 除外·禁手改基线）；扫描面=src/ + CN语言编译器v2/。

用法：
  python3 scripts/check_sentinel_gate.py                    # 门禁（rc=1=有红·L1 静态段）
  python3 scripts/check_sentinel_gate.py --report           # 报告模式（全列·rc=0）
  python3 scripts/check_sentinel_gate.py --json             # 机读输出
  python3 scripts/check_sentinel_gate.py --update-baseline  # 重写冻结基线（清账后收）
  python3 scripts/check_sentinel_gate.py --selftest         # 自测（正反两态+陈旧+复现）

退出码：0=绿；1=红（L1 硬拦）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
import time
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
仓库根 = 脚本目录.parent

# 复用既有扫描核（纪律二·重复归纳）：950 拆行/登记表读取 + 882 掩码/协议注释/
# 锚相容/登记条命中。两模块模块级各自重包 stdout（UTF-8），882 保留被替换包装器
# 引用（_保链）防 __del__ 关底层 buffer——本脚本不再重包 stdout（沿用 950 的）。
import check_miss_defaults as 核950          # noqa: E402
import check_miss_gate as 核882              # noqa: E402

登记表文件 = 仓库根 / "scripts" / "miss_registry.json"
基线文件 = 仓库根 / "scripts" / "sentinel_freeze_baseline.json"

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")
面名们 = ("负一哨兵", "零重叠", "目标尺寸拷贝", "源型顶替")

规则版本 = "1640-v1"

# ============ 判据常量 ============

负一返回 = re.compile(
    r"(?<![A-Za-z0-9_\u4e00-\u9fff])(?:return|返回)\s*\(?\s*-\s*1(?:\.0+)?\s*\)?\s*;")
零返回 = re.compile(
    r"(?<![A-Za-z0-9_\u4e00-\u9fff])(?:return|返回)\s*\(?\s*0\s*\)?\s*;")
分支头 = re.compile(r"(?<![A-Za-z0-9_\u4e00-\u9fff])(?:if|如果)\s*\(")
未找到词 = ("未找到", "找不到", "查不到", "未命中", "不存在",
        "查找失败", "查表失败", "没找到")
零重叠_等负一 = re.compile(r"==\s*-\s*1\b")
零重叠_负值 = re.compile(r"(?<![<>=!\-])<\s*0\b")
零重叠_末等值 = re.compile(
    r"==\s*[^;(){}]*\.(?:end|末)\s*\(\s*\)|\.(?:end|末)\s*\(\s*\)\s*==")
协议注释词 = re.compile(r"^静默豁免\s*")

拷贝_mem = re.compile(r"(?:std::)?(?:memcpy|memmove)\s*\(")
拷贝_名 = re.compile(r"^(?:void|const|char|unsigned|signed|size_t|struct)\b")
v2_拷贝 = re.compile(r"建IR指令\s*\(\s*IR_结构体拷贝\s*,")
host_拷贝 = re.compile(r"Opcode::CopyStruct\s*,")
目标词 = re.compile(r"目标|目的|dst|dest|target|左类型", re.IGNORECASE)
尺寸源 = re.compile(
    r"结果大小|结构体大小|对象大小|容器对象大小|类型大小|字节大小|sizeof|typeSizeOf"
    r"|大小\s*\(")
证明_同型 = re.compile(
    r"(?:源|src)[\w\u4e00-\u9fff]{0,14}\s*==\s*(?:目标|目的|dst|dest|target)"
    r"[\w\u4e00-\u9fff]{0,14}"
    r"|(?:目标|目的|dst|dest|target)[\w\u4e00-\u9fff]{0,14}\s*==\s*(?:源|src)"
    r"[\w\u4e00-\u9fff]{0,14}"
    r"|\btSame\b|\beSame\b|字符串比较\s*\(\s*源"
    r"|srcCanon\s*==\s*dstCanon|dstCanon\s*==\s*srcCanon")
赋值单等 = re.compile(r"(?<![=!<>+\-*/%&|^])([\w\u4e00-\u9fff]+)\s*=(?!=)\s*([^;=]+);")
标识符全 = re.compile(r"^[A-Za-z_\u4e00-\u9fff][\w\u4e00-\u9fff]*$")

回看_零分支 = 4        # 「if 下行首返」回看行数（与 882 同口径）
回看_尺寸赋值 = 60     # 拷贝 size 标识符的赋值回看行数（同函数近似）
窗_证明 = 8            # 同型/等价证明回看行数

# 规则指纹：判据常量序列化后取 sha256 前 16 位（基线记录·可复现审计）
_指纹件 = [
    规则版本, ",".join(面名们),
    负一返回.pattern, 零返回.pattern, 分支头.pattern, 未找到词,
    零重叠_等负一.pattern, 零重叠_负值.pattern, 零重叠_末等值.pattern,
    拷贝_mem.pattern, v2_拷贝.pattern, host_拷贝.pattern, 目标词.pattern,
    尺寸源.pattern, 证明_同型.pattern, 赋值单等.pattern,
    str(回看_零分支), str(回看_尺寸赋值), str(窗_证明),
]
规则指纹 = hashlib.sha256(
    "|".join(str(x) for x in _指纹件).encode("utf-8")).hexdigest()[:16]


# ============ 一、通用工具 ============

def 归一(文本: str) -> str:
    return re.sub(r"\s+", "", 文本 or "")


def 点键(点: dict) -> str:
    return 点["文件"] + "::" + 归一(点["锚"])


def 拆实参(文本: str, 开位: int):
    """`(` 后顶层逗号切分实参；返回 (实参列表, 配对右括号下标或 len)。"""
    深 = 0
    参 = []
    当前 = []
    for k in range(开位 + 1, len(文本)):
        c = 文本[k]
        if c in "([{":
            深 += 1
            当前.append(c)
            continue
        if c in ")]}":
            if 深 == 0:
                参.append("".join(当前))
                return 参, k
            深 -= 1
            当前.append(c)
            continue
        if c == "," and 深 == 0:
            参.append("".join(当前))
            当前 = []
            continue
        当前.append(c)
    参.append("".join(当前))
    return 参, len(文本)


def 取条件与闭位(行: str, 之前: int = -1):
    """最后一个 if/如果 的 (条件文本, 配对右括号下标)；`之前` 给定时要求配对
    右括号在其前（同行 if 与 return 的位置关系核对）。无则 ("", -1)。"""
    m = None
    for x in 分支头.finditer(行[:之前] if 之前 >= 0 else 行):
        m = x
    if m is None:
        return "", -1
    开 = 行.find("(", m.start())
    if 开 < 0:
        return "", -1
    参, 闭 = 拆实参(行, 开)
    return (参[0] if 闭 < len(行) else ""), 闭


def 条件命中(条件: str):
    """查找未命中信号判定：返回原因串或 ""（`.empty()` 不作信号·见 docstring）。"""
    原因 = []
    if any(w in 条件 for w in 未找到词):
        原因.append("显式未找到词面")
    if 零重叠_等负一.search(条件):
        原因.append("等值 -1 判定")
    if 零重叠_负值.search(条件):
        原因.append("负值判定（<0）·0 与合法值重叠")
    if 零重叠_末等值.search(条件):
        原因.append("迭代器 end 等值·未命中")
    return "+".join(原因)


def 加候选(候选: dict, 面: str, 相对: str, 行: int, 锚: str, 因: str):
    锚 = (锚 or "").strip()
    if not 锚:
        return
    候选[面].append({"面": 面, "文件": 相对, "行": 行, "锚": 锚, "因": 因})


def 同行协议(协议点们: list, 行号: int, 锚: str):
    for 协议 in 协议点们:
        if 协议["锚定行"] == 行号 and 核882.锚相容(协议["锚"], 锚):
            return 协议
    return None


def 条目命中(文件: str, 锚: str, 条目们: list):
    条 = 核882.条命中(文件, 锚, 条目们)
    return 条


def 批准问题(条: dict):
    """#1238 批准制复用：返回 (红种类, 说明) 或 None。"""
    if 条 is None:
        return ("漏登记", "代码有协议注释而 miss_registry.json 无条——补登记条目")
    批 = (条.get("批准") or "").strip()
    if 批 == "已毙":
        return ("已毙未清偿", "登记条批准=已毙而哨兵代码仍在——清偿后删条")
    if 批 not in ("已批", "存量过渡"):
        return ("登记无批准",
                f"登记条批准状态={批 or '（缺字段）'}≠已批/存量过渡——"
                f"#1238 兜底禁令：新豁免须呈报批准")
    return None


# ============ 二、规则①：负一哨兵 ============

def 扫负一(相对: str, 码行们: list, 候选: dict):
    for i, 行 in enumerate(码行们):
        if not 负一返回.search(行):
            continue
        加候选(候选, "负一哨兵", 相对, i + 1, 行.strip(),
             "字面量 -1 表失败/未找到（#1360 哨兵禁令）")


# ============ 三、规则②：零与合法值重叠 ============

def 扫零重叠(相对: str, 码行们: list, 行们: list, 候选: dict):
    for i, 行 in enumerate(码行们):
        m = 零返回.search(行)
        if not m:
            continue
        行号 = i + 1
        命中 = ""
        # (a1) 同行 if (cond) { return 0; } / if (cond) return 0;
        同行条件, 闭 = 取条件与闭位(行, m.start())
        if 同行条件 and 闭 < m.start():
            命中 = 条件命中(同行条件)
            if 命中:
                加候选(候选, "零重叠", 相对, 行号, 行.strip(),
                     "同行分支未命中→返回 0：" + 命中)
        # (a2) if (cond) { 换行首返 0 }（回看 4 行·首返语义·与 882 同构）
        if not 命中:
            for k in range(1, 回看_零分支 + 1):
                上 = i - k
                if 上 < 0:
                    break
                上行 = 码行们[上]
                条件, 闭 = 取条件与闭位(上行)
                if not 条件:
                    continue
                # 条件行右括号后只许 { 或空（单语句 if）；同行已闭合块不算
                尾 = 上行[闭 + 1:].strip() if 0 <= 闭 < len(上行) else ""
                if "}" in 尾:
                    break                      # 同行已闭合块（return 不属于其体）
                if 尾 not in ("", "{", ";") and not 尾.startswith("{"):
                    break
                # if 与 return 之间不得已有返回（首返）且块必须仍开着
                中间行们 = 码行们[上 + 1:i]
                中间 = "\n".join(中间行们)
                if re.search(r"(?:return|返回)\b", 中间):
                    break
                深 = (尾.count("{") - 尾.count("}")
                    + 中间.count("{") - 中间.count("}"))
                if 深 <= 0 and 尾.startswith("{"):
                    break                      # 块已闭合——return 不在该 if 体内
                if not 尾 and any(x.strip() for x in 中间行们):
                    break                      # 单语句 if 与 return 之间不得有别的语句
                命中 = 条件命中(条件)
                if 命中:
                    加候选(候选, "零重叠", 相对, 上 + 1,
                         上行.strip() + " │ " + 行.strip(),
                         "分支未命中→返回 0：" + 命中)
                break
        # (b) 注释明写未找到（同行/上一行）·协议注释自身不算
        if not 命中:
            注释 = 行们[i][2] if i < len(行们) else ""
            if not 协议注释词.match(re.sub(r"^(?://+|/\*+|\*+)\s*", "", 注释.strip())):
                上注释 = 行们[i - 1][2] if i > 0 else ""
                if not 协议注释词.match(re.sub(r"^(?://+|/\*+|\*+)\s*", "", 上注释.strip())):
                    词 = next((w for w in 未找到词 if w in 注释 or w in 上注释), "")
                    if 词:
                        加候选(候选, "零重叠", 相对, 行号, 行.strip(),
                             "注释写明未找到（" + 词 + "）→返回 0")


# ============ 四、规则③：按目标大小整块拷贝 ============

def 扫目标尺寸拷贝(相对: str, 码行们: list, 候选: dict):
    for i, 行 in enumerate(码行们):
        # (a) memcpy/memmove(dst, src, sizeof(dst))
        for m in 拷贝_mem.finditer(行):
            开 = 行.find("(", m.start())
            参, _闭 = 拆实参(行, 开)
            if len(参) < 3:
                continue
            dst = 参[0].strip()
            if 拷贝_名.match(dst):
                continue                      # 函数声明/定义（void* dst 形态）
            dm = re.match(r"^[&*\s]*(?:std::)?([A-Za-z_\u4e00-\u9fff][\w\u4e00-\u9fff]*)\s*$",
                         dst)
            if not dm or "sizeof" not in 参[2]:
                continue
            if re.search(r"\b" + re.escape(dm.group(1)) + r"\b", 参[2]):
                加候选(候选, "目标尺寸拷贝", 相对, i + 1, 行.strip(),
                     "memcpy 第三参取目标 sizeof(" + dm.group(1) + ")——源窄于目标即高位残留")
        # (b/c) IR 结构体拷贝：size 实参按目标词面取宽
        for m in v2_拷贝.finditer(行):
            开 = 行.find("(", m.start())
            参, _闭 = 拆实参(行, 开)
            参 = [x.strip() for x in 参]
            if len(参) < 6:
                continue
            尺寸 = 参[5]
            因 = ""
            if 目标词.search(尺寸):
                因 = "v2 IR 结构体拷贝 size 实参含目标词面（" + 尺寸[:40] + "）"
            elif 标识符全.match(尺寸):
                for k in range(1, min(回看_尺寸赋值, i) + 1):
                    上行 = 码行们[i - k]
                    命中 = re.search(r"^\s*" + re.escape(尺寸) + r"\s*=(?!=)\s*([^;]+);",
                                   上行)
                    if 命中 and 尺寸源.search(命中.group(1)) and 目标词.search(命中.group(1)):
                        因 = ("v2 IR 结构体拷贝 size=" + 尺寸 +
                             " 回溯自目标类型尺寸（" + 命中.group(1).strip()[:50] + "）")
                        break
            if 因 and not 有同型证明(码行们, i):
                加候选(候选, "目标尺寸拷贝", 相对, i + 1, 行.strip(), 因)
        for m in host_拷贝.finditer(行):
            emit位 = 行.rfind("emit", 0, m.start() + 1)
            if emit位 < 0:
                continue
            开 = 行.find("(", emit位)
            if 开 < 0:
                continue
            参, _闭 = 拆实参(行, 开)
            参 = [x.strip() for x in 参]
            if len(参) < 4:
                continue
            if 目标词.search(参[3]) and not 有同型证明(码行们, i):
                加候选(候选, "目标尺寸拷贝", 相对, i + 1, 行.strip(),
                     "host CopyStruct size 实参按目标宽度（" + 参[3][:40] + "）")


def 有同型证明(码行们: list, i: int) -> bool:
    """拷贝语句前 8 行代码内有源码层同型/等价证明（只看代码·注释声称不算）。"""
    窗 = "\n".join(码行们[max(0, i - 窗_证明):i + 1])
    return bool(证明_同型.search(窗))


# ============ 五、规则④：源型顶替（侥幸路径清点） ============

def 扫源型顶替(相对: str, 码行们: list, 候选: dict):
    for i, 行 in enumerate(码行们):
        for m in 赋值单等.finditer(行):
            左, 右 = m.group(1), m.group(2)
            左低 = 左.lower()
            if "源" not in 左 and "src" not in 左低:
                continue
            左有类标 = any(k in 左 for k in ("类型", "键")) or "canon" in 左低 or "type" in 左低
            右低 = 右.lower()
            右有目标 = bool(目标词.search(右))
            右有类标 = any(k in 右 for k in ("类型", "键")) or "canon" in 右低 or "type" in 右低
            # 右值须为裸目标类型表达式（标识符/成员链）——函数调用取源类型不判
            右裸 = bool(re.match(
                r"^[A-Za-z_\u4e00-\u9fff][\w\u4e00-\u9fff]*"
                r"(?:\s*(?:\.|->)\s*[\w\u4e00-\u9fff]+)*\s*$", 右.strip()))
            if 左有类标 and 右有目标 and 右有类标 and 右裸:
                加候选(候选, "源型顶替", 相对, i + 1, 行.strip(),
                     "源类型未证明即按目标类型同型处理（" + 左.strip() +
                     " ← " + 右.strip()[:40] + "）")


# ============ 六、单文件/全库扫描 ============

def 扫文件(相对: str, 文本: str):
    行们 = 核950.拆行(文本)
    码行们 = 核882.掩码(文本).split("\n")
    协议点们 = 核882.扫协议注释(相对, 行们)
    候选 = {面: [] for 面 in 面名们}
    扫负一(相对, 码行们, 候选)
    扫零重叠(相对, 码行们, 行们, 候选)
    扫目标尺寸拷贝(相对, 码行们, 候选)
    扫源型顶替(相对, 码行们, 候选)
    for 面 in 面名们:                       # 同位置同锚重复（窗口叠加）=去重
        去重 = {}
        for 点 in 候选[面]:
            去重.setdefault((点["行"], 归一(点["锚"])), 点)
        候选[面] = list(去重.values())
    return 候选, 协议点们


def 全库扫描(根: Path):
    候选 = {面: [] for 面 in 面名们}
    协议点们 = []
    文件们 = {}
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
            候选此, 协议此 = 扫文件(相对, 文本)
            for 面 in 面名们:
                候选[面].extend(候选此[面])
            协议点们.extend(协议此)
    return {"候选": 候选, "协议点": 协议点们, "文件文本": 文件们, "统计": 统计}


# ============ 七、基线读写 ============

def 读基线(路径: Path):
    if not 路径.exists():
        raise SystemExit(f"[门禁] 基线缺失：{路径.name}——先跑 "
                         f"python3 scripts/check_sentinel_gate.py --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁] 基线损坏（{e.__class__.__name__}）：{路径.name}")
    指纹 = 数据.get("规则指纹", "")
    return ({面: Counter(数据.get("面", {}).get(面, [])) for 面 in 面名们}, 指纹)


def 写基线(路径: Path, 扫描果: dict) -> dict:
    面 = {名: sorted(点键(p) for p in 扫描果["候选"][名]) for 名 in 面名们}
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=路径.parent.parent,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_sentinel_gate.py",
        "任务": "1640",
        "规则版本": 规则版本,
        "规则指纹": 规则指纹,
        "说明": ("#1640 哨兵/侥幸模式冻结基线（只减不增）：键=「文件::归一锚文本」，"
               "同键重复=计数。基线内=存量放行；新增位置=红（除非协议注释+"
               "miss_registry.json 登记条且批准=已批/存量过渡）；键消失=消减提示。"
               "手改基线=绕过门禁行为·禁（收账=--update-baseline·仅确认清偿后）。"),
        "生成sha": sha,
        "面": 面,
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1, sort_keys=False),
                  encoding="utf-8")
    return {名: len(面[名]) for 名 in 面名们}


# ============ 八、判定引擎（三红） ============

def 判定(扫描果: dict, 条目们: list, 基线: dict):
    红们, 豁免们, 消减, 面统 = [], [], {}, {}
    for 面 in 面名们:
        点们 = 扫描果["候选"][面]
        现计 = {}
        for 点 in 点们:
            现计.setdefault(点键(点), []).append(点)
        基计 = 基线.get(面, Counter())
        新增 = []
        for 键 in sorted(现计):
            b = 基计.get(键, 0)
            序 = sorted(现计[键], key=lambda x: (x["行"], x["锚"]))
            if len(序) > b:
                新增.extend(序[b:])
        减 = []
        for 键, b in sorted(基计.items()):
            c = len(现计.get(键, []))
            if c < b:
                减.extend([键] * (b - c))
        消减[面] = 减
        放行 = 红数 = 0
        for 点 in 新增:
            协议 = 同行协议(扫描果["协议点"], 点["行"], 点["锚"])
            if 协议 is not None:
                if 协议["类别"] not in 核882.豁免类别枚举:
                    红们.append({"种类": "协议注释类别非法", "文件": 点["文件"],
                               "行": 点["行"], "锚": 点["锚"][:120],
                               "说明": f"类别「{协议['类别']}」不在枚举 "
                                     f"{sorted(核882.豁免类别枚举)}"})
                    红数 += 1
                    continue
                条 = 条目命中(点["文件"], 点["锚"], 条目们)
                问题 = 批准问题(条)
                if 问题 is not None:
                    红们.append({"种类": 问题[0], "文件": 点["文件"], "行": 点["行"],
                               "锚": 点["锚"][:120], "说明": 问题[1]})
                    红数 += 1
                    continue
                放行 += 1
                豁免们.append({"面": 面, "文件": 点["文件"], "行": 点["行"],
                            "锚": 点["锚"][:100], "豁免": "协议注释+登记条目"})
                continue
            条 = 条目命中(点["文件"], 点["锚"], 条目们)
            if 条 is not None:
                红们.append({"种类": "新增位置缺协议注释", "文件": 点["文件"],
                           "行": 点["行"], "锚": 点["锚"][:120],
                           "说明": f"{面}：登记条目在但代码无协议注释——补"
                                 f"「// 静默豁免<类别>: 原因」"})
            else:
                红们.append({"种类": "新增" + 面, "文件": 点["文件"], "行": 点["行"],
                           "锚": 点["锚"][:120],
                           "说明": f"新增哨兵/侥幸位置无登记（{面}）——改写为 "
                                 f"结果/可选，或走协议注释+登记条通道"})
            红数 += 1
        面统[面] = {"现测": len(点们), "基线": sum(基计.values()), "新增": len(新增),
                   "豁免放行": 放行, "红": 红数, "消减": len(减)}
    return {"红": 红们, "豁免": 豁免们, "消减": 消减, "面统": 面统,
            "条目数": len(条目们), "协议点数": len(扫描果["协议点"])}


# ============ 九、输出 ============

def 打印红们(红们: list):
    排序 = sorted(红们, key=lambda r: (r["种类"], r["文件"], r["行"]))
    print(f"\n【红清单】（{len(排序)} 条·L1 硬拦）")
    for r in 排序:
        位 = f"{r['文件']}:{r['行']}" if r.get("行") else r["文件"]
        print(f"  [×] ({r['种类']}) {位}")
        if r.get("锚"):
            print(f"      锚：{r['锚'][:110]}")
        print(f"      处置：{r['说明']}")


def 打印报告(扫描果: dict, 判定果: dict, 指纹: str, 详列: bool, 用时: float = 0.0):
    统计 = 扫描果["统计"]
    print("== 哨兵与侥幸模式门禁（check_sentinel_gate.py·#1640）==")
    print(f"扫描面：src/ + CN语言编译器v2/（文件 {统计['文件']}·读取失败 "
          f"{统计['读取失败']}·用时 {用时:.2f}s）；规则 {规则版本} 指纹 {规则指纹}；"
          f"登记表条目 {判定果['条目数']} 条；协议注释点 {判定果['协议点数']} 处")
    if 指纹 and 指纹 != 规则指纹:
        print(f"[提示] 基线规则指纹 {指纹} ≠ 当前 {规则指纹}——判据已变更，"
              f"复核消减面后 --update-baseline 收账")
    print("【面级统计】面 | 现测 | 基线 | 新增 | 豁免放行 | 红 | 消减")
    for 面 in 面名们:
        u = 判定果["面统"][面]
        print(f"  {面} | {u['现测']} | {u['基线']} | {u['新增']} | "
              f"{u['豁免放行']} | {u['红']} | {u['消减']}")
    for 面 in 面名们:
        减 = 判定果["消减"].get(面, [])
        if 减:
            print(f"  [↓] {面} 基线位置消失 {len(减)} 处（自动消减·确认无误报后 "
                  f"--update-baseline 收基线）：{减[:3]}…")
    if 判定果["豁免"]:
        print(f"\n【豁免放行】（{len(判定果['豁免'])} 条·协议注释+登记条目）")
        for h in 判定果["豁免"][:10]:
            print(f"  [✓] {h['文件']}:{h['行']}（{h['面']}）{h['锚'][:80]}")
        if len(判定果["豁免"]) > 10:
            print(f"  …（其余 {len(判定果['豁免']) - 10} 条）")
    if 详列:
        print("\n【在测位置明细】（--report·全列）")
        for 面 in 面名们:
            print(f"-- {面}（{len(扫描果['候选'][面])}）--")
            for 点 in sorted(扫描果["候选"][面], key=lambda x: (x["文件"], x["行"])):
                print(f"  {点['文件']}:{点['行']}  {点['锚'][:100]}")
                print(f"      因：{点['因']}")
    if 判定果["红"]:
        打印红们(判定果["红"])
        print(f"\n[门禁·红] {len(判定果['红'])} 条——L1 硬拦；唯一合法通道="
              f"协议注释+miss_registry.json 登记条（批准=已批/存量过渡），或原地结果化。")
    else:
        print("\n[门禁·绿] 四面零红（存量冻结只减不增·新增面严）")


def 转json(判定果: dict, 扫描果: dict) -> str:
    输出 = {"工具": "check_sentinel_gate.py", "任务": "1640",
          "规则版本": 规则版本, "规则指纹": 规则指纹,
          "统计": 扫描果["统计"], "面统": 判定果["面统"],
          "条目数": 判定果["条目数"], "协议点数": 判定果["协议点数"],
          "豁免": 判定果["豁免"],
          "消减": {k: v[:50] for k, v in 判定果["消减"].items()},
          "红": 判定果["红"]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 十、自测（正反两态+三红+复现·零触碰真实树） ============

样例干净 = """// 干净样例：业务值 0/-1 不经哨兵通道
int 求和(int a, int b) {
    int 和 = a + b;
    return 和;
}
bool 有值(const int* p) {
    if (p == nullptr) return false;
    return true;
}
"""

样例v2干净 = """// v2 干净样例
函数 求和(整32 甲, 整32 乙) -> 整32 {
    返回 甲 + 乙;
}
"""


def 自测写表(路径: Path, 条目们: list):
    路径.write_text(json.dumps({"版本": 1, "条目": 条目们}, ensure_ascii=False),
                  encoding="utf-8")


def 自测建场景(保活: list, 文件们: dict):
    td = tempfile.TemporaryDirectory(prefix="csg1640_")
    保活.append(td)
    根 = Path(td.name)
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")
    表 = 根 / "registry.json"
    自测写表(表, [])
    基 = 根 / "baseline.json"
    写基线(基, 全库扫描(根))
    return 根, 表, 基


def 自测跑(根: Path, 表: Path, 基: Path):
    扫描果 = 全库扫描(根)
    条目们, 态 = 核950.读登记表(表)
    if 态 != "就绪":
        return 2, {"红": [{"种类": "登记表异常"}], "豁免": [], "消减": {},
                   "面统": {}, "条目数": 0, "协议点数": 0}, 扫描果
    基线, _指纹 = 读基线(基)
    果 = 判定(扫描果, 条目们, 基线)
    return (1 if 果["红"] else 0), 果, 扫描果


def 红种们(果) -> set:
    return {r["种类"] for r in 果["红"]}


def 自测场景一(断言, 保活):
    """基线放行 + 负一哨兵新增/漏登记/批准制三态。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/干净.cpp": 样例干净,
        "src/存量.cpp": "int 旧查() {\n    return -1;\n}\n",
        "CN语言编译器v2/干净.cn": 样例v2干净})
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"基线内 -1 存量放行=绿: 红={果['红']!r}")
    断言(果["面统"]["负一哨兵"]["现测"] == 1, "负一哨兵现测=1")
    存量 = 根 / "src" / "存量.cpp"
    存量.write_text("int 旧查() {\n    return -1;\n}\n"
                  "int 新查() {\n    return -1;\n}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增负一哨兵" in 红种们(果),
         f"反态①新 -1 哨兵真拦: {红种们(果)}")
    存量.write_text("int 旧查() {\n    return -1;\n}\n"
                  "int 新查() {\n    // 静默豁免业务值: 自测\n    return -1;\n}\n",
                  encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "漏登记" in 红种们(果), f"反态②漏登记（注释无条）: {红种们(果)}")
    条 = {"文件": "src/存量.cpp", "函数": "新查", "锚文本": "return -1;",
        "来源": "1640selftest", "类别": "业务值", "静默风险": "低",
        "原因": "自测", "登记人": "selftest"}
    自测写表(表, [条])
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "登记无批准" in 红种们(果), f"反态③登记无批准: {红种们(果)}")
    自测写表(表, [dict(条, 批准="已批")])
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态①协议注释+已批=放行: 红={果['红']!r}")
    自测写表(表, [dict(条, 批准="已毙")])
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "已毙未清偿" in 红种们(果), f"反态④已毙未清偿: {红种们(果)}")
    自测写表(表, [dict(条, 批准="已批")])
    (根 / "src" / "存量.cpp").write_text("int 无关() {\n    return 1;\n}\n",
                                     encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0 and 果["消减"]["负一哨兵"],
         f"陈旧检测：基线位置消失=消减非红: {果['消减']}")


def 自测场景二(断言, 保活):
    """零重叠：分支 miss/否定对照/注释形态；非候选形态不误报。"""
    根, 表, 基 = 自测建场景(保活, {"src/干净.cpp": 样例干净})
    样 = 根 / "src" / "查表.cpp"
    样.write_text("int 查宽(表& t, 键 k) {\n"
                "    auto it = t.find(k);\n"
                "    if (it == t.end()) return 0;\n"
                "    return it->second;\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增零重叠" in 红种们(果), f"反态⑤end 未命中→0 真拦: {红种们(果)}")
    样.write_text("int 查宽(表& t, 键 k) {\n"
                "    auto it = t.find(k);\n"
                "    if (it != t.end()) return 0;\n"
                "    return it->second;\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态②否定形态（!= end）不误报: 红={果['红']!r}")
    样.write_text("int 查宽(表& t, 键 k) {\n"
                "    auto it = t.find(k);\n"
                "    if (it == t.end()) { return 0; }\n"
                "    return it->second;\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and any(r["行"] == 3 for r in 果["红"]),
         f"反态⑥块内首返 0 锚在 if 行: {[(r['种类'], r['行']) for r in 果['红']][:3]}")
    样.write_text("int 查宽(表& t, 键 k) {\n"
                "    auto it = t.find(k);\n"
                "    if (it == t.end()) { return 0; }  // 静默豁免业务值: 自测\n"
                "    return it->second;\n"
                "}\n", encoding="utf-8")
    自测写表(表, [{"文件": "src/查表.cpp", "函数": "查宽", "锚文本": "if (it == t.end())",
               "来源": "1640selftest", "类别": "业务值", "静默风险": "低",
               "原因": "自测", "登记人": "selftest", "批准": "存量过渡"}])
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态③if 行协议注释+条=两门禁同锚放行: 红={果['红']!r}")
    自测写表(表, [])
    样.write_text("int 位置(表& t, 键 k) {\n"
                "    int 甲 = 定位(t, k);\n"
                "    if (甲 < 0) return 0;\n"
                "    return 甲;\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增零重叠" in 红种们(果),
         f"反态⑦负值判定→0（0 与合法索引重叠）真拦: {红种们(果)}")


def 自测场景三(断言, 保活):
    """目标尺寸拷贝：sizeof(dst) 真拦/sizeof(src) 对照/同型证明放行/源型顶替。"""
    根, 表, 基 = 自测建场景(保活, {"src/干净.cpp": 样例干净})
    样 = 根 / "src" / "拷贝.cpp"
    样.write_text("void 搬(char* 目标, char* 源) {\n"
                "    memcpy(目标, 源, sizeof(源));\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态④sizeof(源) 不误报: 红={果['红']!r}")
    样.write_text("void 搬(char* 目标, char* 源) {\n"
                "    memcpy(目标, 源, sizeof(目标));\n"
                "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增目标尺寸拷贝" in 红种们(果),
         f"反态⑧memcpy 按目标 sizeof 真拦: {红种们(果)}")
    样.write_text("void 搬(char* 目标, char* 源) {\n"
                "    memcpy(目标, 源, sizeof(源));\n"
                "}\n", encoding="utf-8")
    源v2 = 根 / "CN语言编译器v2" / "合成体.cn"
    源v2.parent.mkdir(parents=True, exist_ok=True)
    源v2.write_text("函数 发射(整64 源地址, 整64 目标地址, 整64 目标大) -> 空类型 {\n"
                 "    IR追加(指令, 建IR指令(IR_结构体拷贝, 源地址, 目标地址, 0, 0, 目标大, 0));\n"
                 "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增目标尺寸拷贝" in 红种们(果),
         f"反态⑨v2 IR 拷贝按目标尺寸真拦: {红种们(果)}")
    源v2.write_text("函数 发射(整64 源地址, 整64 目标地址, 整64 目标大) -> 空类型 {\n"
                 "    如果 (字符串比较(源T, 目标T)) {\n"
                 "        IR追加(指令, 建IR指令(IR_结构体拷贝, 源地址, 目标地址, 0, 0, 目标大, 0));\n"
                 "    }\n"
                 "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态⑤同型证明在→不判: 红={果['红']!r}")
    源v2.write_text("函数 顶替(字符串 目标类型文本) -> 字符串 {\n"
                 "    字符串 源类型文本 = 目标类型文本;\n"
                 "    返回 源类型文本;\n"
                 "}\n", encoding="utf-8")
    rc, 果, _ = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增源型顶替" in 红种们(果),
         f"反态⑩源类型顶替=侥幸路径真拦: {红种们(果)}")


def 自测场景四(断言, 保活):
    """基线缺失拒跑 + 可复现（两次生成逐字节一致）+ 指纹字段齐备。"""
    根, _表, 基 = 自测建场景(保活, {
        "src/干净.cpp": 样例干净,
        "src/存量.cpp": "int 旧查() {\n    return -1;\n}\n"})
    try:
        读基线(根 / "不存在.json")
        断言(False, "基线缺失应拒跑")
    except SystemExit:
        断言(True, "基线缺失=拒跑（防绕过）")
    甲 = 根 / "基甲.json"
    乙 = 根 / "基乙.json"
    写基线(甲, 全库扫描(根))
    写基线(乙, 全库扫描(根))
    断言(甲.read_bytes() == 乙.read_bytes(), "基线两次生成逐字节可复现")
    数据 = json.loads(甲.read_text(encoding="utf-8"))
    断言(数据.get("规则版本") == 规则版本 and len(数据.get("规则指纹", "")) == 16,
         "基线记录规则版本+指纹")
    断言(数据.get("生成sha") is not None, "基线记录生成 sha")
    断言(读基线(基)[1] == 规则指纹, "当前基线指纹=当前规则指纹")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    保活 = []
    for 场景 in (自测场景一, 自测场景二, 自测场景三, 自测场景四):
        try:
            场景(断言, 保活)
        except Exception as e:            # 场景异常=自测失败（显形不静默）
            失败.append(f"{场景.__name__} 异常：{e!r}")
    if 失败:
        print("== 哨兵门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 哨兵门禁自测全过（{通过[0]} 项断言·正反两态/三红/陈旧/复现齐备）==")
    return 0


# ============ 十一、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="哨兵与侥幸模式机械门禁（#1640·三族四面+冻结基线）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--registry", default=str(登记表文件), help="登记表路径")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（rc=0 不拦）")
    解析器.add_argument("--json", action="store_true", help="机读输出")
    解析器.add_argument("--update-baseline", action="store_true", help="重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    起点 = time.perf_counter()
    扫描果 = 全库扫描(根)
    用时 = time.perf_counter() - 起点
    if 参数.update_baseline:
        统计 = 写基线(Path(参数.baseline), 扫描果)
        print(f"[基线] 已重写 {Path(参数.baseline).name}（规则 {规则版本} 指纹 "
              f"{规则指纹}·用时 {用时:.2f}s）："
              + " · ".join(f"{面} {统计[面]}" for 面 in 面名们))
        return 0
    条目们, 态 = 核950.读登记表(Path(参数.registry))
    if 态 != "就绪":
        print(f"[门禁·红] 登记表状态异常：{态}（{参数.registry}）")
        return 1
    基路径 = Path(参数.baseline)
    if 参数.report and not 基路径.exists():
        基线 = {面: Counter() for 面 in 面名们}
        指纹 = ""
        print(f"[提示] 基线缺失（{基路径.name}）——报告模式按空基线全量展示（不拦）")
    else:
        基线, 指纹 = 读基线(基路径)
    判定果 = 判定(扫描果, 条目们, 基线)
    if 参数.json:
        print(转json(判定果, 扫描果))
    else:
        打印报告(扫描果, 判定果, 指纹, 详列=参数.report, 用时=用时)
    if 参数.report:
        return 0
    return 1 if 判定果["红"] else 0


if __name__ == "__main__":
    sys.exit(main())
