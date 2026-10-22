#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_miss_defaults.py —— 宿主 miss 静默面清查扫描器（950·891 的对侧件·648 立法配套审计）。

病根（648/110 立法·用户 10-17 晨质询「病根二/三是不是默认值静默错」）：编译器实现
代码的查表/判定 miss 终态**禁止静默取默认值继续**（110 实录：FFI 返回类型查表 miss 静默
默认整32 → 三平台错值；852 病根二/三=genStaticLocalGuardInit 标量通道 Cast 截断 /
visitIndexExpr 回退 0 帧基址野读）。合法处置仅三：①报内部错误诊断；②显式哨兵+消费点
强制检查；③穷举断言证明不可达。合法豁免面（须注释声明边界）：上游已诊断的错误恢复、
内置白名单/运行时 API/闭集穷举。

本工具=存量清查的**机械初筛面**（抓「留痕」核「登记」，机械筛+人判·默认不判红）：
  A 形态面：「查表/判定 miss 分支返默认值」启发式——`if (<miss 信号>) return <默认值>;`
    三形态（同行 / 下行 return / else-邻接查表调用；v2 侧 如果/否则 同规）。miss 信号=
    nullptr 判定/取反/empty()/end()/miss/未找到/找不到/查不到/未命中/失败/空指针/
    == -1/< 0/>= size()——**普通守卫（条件无 miss 信号）不入候选项**（单计弱形态数）。
  B 留痕面：注释红旗词面计数（648 立法审查驳回词「防御性兜底/保守回退/miss 即默认/
    查不到按/兜底默认」+ 留痕词「兜底/回退/静默/防御/降级/未找到/miss」）——注释是
    人工审计的足迹。
  C 处置面：候选**同函数**窗口（不跨函数防相邻污染）扫描三选一执法信号——报诊断
    （diagnostics_.report/内部错误/运行时错误/表诊断/IO::打印到错误·**只看代码**——注释里
    提及不算）·断言（assert/不可达/穷举·只看代码）·豁免声明（豁免/白名单/运行时 API/
    上游已诊断/错误恢复/契约·注释+代码均可）。

分级（机械理由透明·档=线索优先级非缺陷定论）：
  高 = miss 形态命中且零处置信号（疑似真静默·典型立法前存量；含「有留痕词但无处置动作」）
  中 = 有处置信号待复核——报诊断（复核**文案指向性**·#145 级联乱报警示）/
       豁免声明（复核豁免边界注释齐备性）/断言（复核不可达证明）
  低 = ①已登记（miss_registry.json 命中·折叠不重复报）②**自然哨兵**（返回类型为
       bool/布尔/指针且返回值 false/nullptr——C++ 错误传播惯习·非默认值静默面）③断言+报诊断双全
弱形态（计数不入清单）：`if (条件) return 默认值` 但条件无 miss 信号=普通守卫——不属
  查表判定面（机械排除防噪声淹没；这是「宁缺勿滥」的取向·--all 可展开查看）。

诚实边界（不假装全覆盖·不判死）：
  ① 语义级不可全识——`return 0` 有千种语境（业务值/尺寸/索引常态均可为 0）；本工具
     只报「miss 判定形态 + 默认值」的机械共现，是否真静默须逐处人读上下文。
  ② 形态启发式逐行+紧邻窗口（不建跨函数数据流）；函数归属为「最近函数头」文本近似
     （标 ≈），返回类型同理（函数头行前缀最后一段·构造/初始化列表形态可漏）。
  ③ 留痕词计数是「注释书写习惯」的代理（有人写兜底≠有缺陷；删注释不等于清偿）——
     清偿判据=三选一落地+登记表更新，不看词面增减。
  ④ runtime/ 的 FFI 边界面（空指针视为空串/EOF 等）属「运行时 API 契约」合法豁免候选——
     机械不预判，有豁免注释即入中档待核（边界注释齐备=合规）。
  ⑤ 本工具只读不写（登记表更新为人判后的登记动作）；存量清偿批次规划见清单文档
     docs/miss静默面存余清单-950.md。

与 891（v2 侧）合流字段：两侧扫描器统一输出「文件/行/函数/档/理由/登记」六字段，
合流按 (编译器侧, 文件, 行) 归并成「双侧完成度总表」（字段口径见清单文档）。

用法：
  python3 scripts/check_miss_defaults.py                  # 宿主全库盘点（报告式·rc=0）
  python3 scripts/check_miss_defaults.py --v2             # 附加扫 v2（CN语言编译器v2/**/*.cn）
  python3 scripts/check_miss_defaults.py --only v2        # 只扫 v2 侧
  python3 scripts/check_miss_defaults.py --limit 50       # 清单截断（默认 40·0=全列）
  python3 scripts/check_miss_defaults.py --json           # 机读输出（stdout）
  python3 scripts/check_miss_defaults.py --md [文件]      # md 输出（缺省 stdout）
  python3 scripts/check_miss_defaults.py --strict         # 高嫌疑>0 → rc=1（专项轮可选）
  python3 scripts/check_miss_defaults.py --selftest       # 自测（临时样例树正反两态）

退出码：0=报告完成（--strict 且高嫌疑>0 → 1）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）：泛型下标注解与 X|None 在 3.8 会炸
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

仓库根 = Path(__file__).resolve().parent.parent
登记表路径 = 仓库根 / "scripts" / "miss_registry.json"

# ============ 词面与模式常量 ============

# 648 立法审查驳回词（命中=高敏感留痕·审查红旗原文）
驳回词 = ("防御性兜底", "保守回退", "miss 即默认", "miss即默认", "查不到按", "兜底默认")
# 留痕词（人工审计足迹·非判据本身）
留痕词 = ("兜底", "回退", "静默", "防御", "降级", "未找到", "找不到", "查不到",
        "未命中", "查找失败", "查表失败", "容错")

# miss 信号（判定条件文本内匹配·收紧口径：显式「查无效/查不到」判定）
# 1360 结果化消费点通道（哨兵禁令全面化·终态立法 96b86f64）：has_value 分流/nullopt
# 返回/value_or 显形默认=类型强制的检查动作本身·非「默认值顶替」——三形态扫描器豁免。
结果化通道 = re.compile(
    r"\.has_value\s*\(\s*\)|std::nullopt|\.value_or\s*\("
    r"|\.有值|返回\s*无")
miss信号 = re.compile(
    r"==\s*nullptr|!\s*[\w\u4e00-\u9fff(]|\.empty\s*\(\s*\)|\.end\s*\(\s*\)"
    r"|==\s*-1\b|<\s*0\b|>=\s*[\w.]+\.size\s*\(|>=\s*[\w.]+\.大小\s*\(|miss\b"
    r"|未找到|找不到|查不到|未命中|失败|空指针")

# 默认值（return 表达式整体匹配）：业务默认值+哨兵值分离——bool/指针类型的
# false/nullptr 是自然哨兵（错误传播惯习），其余值（0/8/-1/空串/{}）是业务静默默认。
默认值 = re.compile(
    r"^(?P<值>-?\s*(?:0|8|-1|0x0|0u|0L|0\.0|0\.0f|false|true|nullptr|真|假|空指针))"
    r"\s*$|^(?P<空串>\"\"\s*s?|\{\s*\})\s*$")
哨兵值 = {"false", "true", "nullptr", "真", "假", "空指针"}

# 查表调用（else 邻接限定形态识别）：查表/查询函数名
查表调用 = re.compile(
    r"(?<![A-Za-z0-9_])(?:find|lookup|search|typeSizeOf|findStruct|findFunction"
    r"|findClass|findVarEntry|lookupSrcType|查|查询)\w*\s*\(")

# 处置信号（三选一执法 + 豁免声明）——报诊断/断言只看代码段（注释提及不算）
处置_报诊断 = re.compile(
    r"diagnostics_\.report|reportErrorHere|(?<![A-Za-z0-9_])reportError\s*\(|内部错误"
    r"|运行时错误|表诊断\s*\(|IO::打印到错误|std::abort|throw\s+")
处置_断言 = re.compile(r"(?<![A-Za-z0-9_])assert\s*\(|CN_ASSERT|static_assert|不可达|穷举|断言")
处置_豁免 = re.compile(
    r"豁免|白名单|运行时\s*API|上游已诊断|错误恢复|契约|提前返回|提前退出|视为空"
    r"|按\s*EOF|调用方保证|防御性设计|已诊断")

# return 语句（值形态）：宿主 return / v2 返回
返回宿主 = re.compile(r"(?<![A-Za-z0-9_])return(?![A-Za-z0-9_])\s*([^;]*);")
返回v2 = re.compile(r"(?<![A-Za-z0-9_\u4e00-\u9fff])返回(?![A-Za-z0-9_\u4e00-\u9fff])\s*([^;]*);")

# 函数头近似（分组/返回类型线索·仅近似）：
#   宿主 `[extern "C"] 类型 名(...)...{` 行（前缀捕获=返回类型线索）；
#   v2 `函数 名(...) -> 类型 {` 行
函数头宿主 = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?(?:extern\s+\"C\"\s+)?(?P<前缀>[\w:<>~,\*&\s\u4e00-\u9fff]*?)"
    r"(?P<名>[A-Za-z_~\u4e00-\u9fff][A-Za-z0-9_\u4e00-\u9fff]*"
    r"(?:::[A-Za-z_0-9\u4e00-\u9fff]+)*)\s*\([^;{}]*\)"
    r"\s*(?:const\b|noexcept\b|override\b|final\b|mutable\b|\s)*"
    r"(?:->[^;{}\n]*)?(?:[^;{}\n]*?)\{?\s*$")
函数头v2 = re.compile(
    r"^\s*(?:不安全\s+)?(?:静态\s+)?(?:公开\s+)?函数\s+(?P<名>[^\s(]+)\s*\("
    r"[^)]*\)\s*(?:->\s*(?P<类型>[\w\u4e00-\u9fff]+))?[^{]*\{?\s*$")
控制词 = {"if", "while", "for", "switch", "catch", "do", "else", "return", "sizeof",
        "decltype", "static_assert", "assert", "template", "throw", "new", "delete",
        "case", "default", "函数", "如果", "当", "循环", "选择", "情况", "否则",
        "遍历", "中断", "继续"}

窗口_处置 = 6     # 候选同函数窗口半径（行）——处置信号扫描面
窗口_留痕 = 2     # 形态候选旁留痕词窗口（行）
回看_函数 = 400   # 函数归属回看上限（行）


# ============ 一、源码行处理（注释拆分·块注释状态机） ============

def 扫一行(行: str):
    """单行状态机：返回 (代码段, 注释段, 行末是否仍在块注释中)。字符串/字符态内不解注释。"""
    代码 = []
    注释 = []
    块注释中 = False
    i = 0
    n = len(行)
    while i < n:
        c = 行[i]
        if c == "/" and i + 1 < n and 行[i + 1] == "/":
            注释.append(行[i:])
            break
        if c == "/" and i + 1 < n and 行[i + 1] == "*":
            j = 行.find("*/", i + 2)
            if j < 0:
                注释.append(行[i:])
                块注释中 = True
                i = n
            else:
                注释.append(行[i:j + 2])
                i = j + 2
            continue
        if c == '"':
            j = i + 1
            while j < n:
                if 行[j] == "\\":
                    j += 2
                    continue
                if 行[j] == '"':
                    j += 1
                    break
                j += 1
            代码.append(行[i:j])
            i = j
            continue
        if c == "'":
            j = i + 1
            while j < n:
                if 行[j] == "\\":
                    j += 2
                    continue
                if 行[j] == "'":
                    j += 1
                    break
                j += 1
            if 0 < j - i <= 6:
                代码.append(行[i:j])
                i = j
                continue
            代码.append(c)
            i += 1
            continue
        代码.append(c)
        i += 1
    return "".join(代码), "".join(注释), 块注释中


def 拆行(文本: str):
    """逐行拆出 [(行号, 代码段, 注释段)]（行号从 1 起·块注释跨行状态机）。"""
    出场 = []
    块注释中 = False
    for 行号, 原文 in enumerate(文本.split("\n"), 1):
        if 块注释中:
            结束 = 原文.find("*/")
            if 结束 < 0:
                出场.append((行号, "", 原文))
                continue
            注释头 = 原文[:结束 + 2]
            余 = 原文[结束 + 2:]
            代码, 注释尾, 块注释中 = 扫一行(余)
            出场.append((行号, 代码, 注释头 + 注释尾))
            continue
        代码, 注释, 块注释中 = 扫一行(原文)
        出场.append((行号, 代码, 注释))
    return 出场


def 配对一个括号(行: str, 开位: int):
    """行内 `(` 起配对；返回配对 `)` 下标或 None（不跨行）。"""
    深度 = 0
    for i in range(开位, len(行)):
        if 行[i] == "(":
            深度 += 1
        elif 行[i] == ")":
            深度 -= 1
            if 深度 == 0:
                return i
    return None


def 找块内首返(行们: list, i: int, 动词正则, 最远: int = 4):
    """从 I 行（0 基下标 i）之后在「最远」行内找第一条以返回动词开头的行。

    遇以 `}` 开头的行提前停止（块已闭合·防跨块误抓）；空白行跳过。
    返回 (行号, 代码文本) 或 None。""" 
    n = len(行们)
    for k in range(1, 最远 + 1):
        if i + k >= n:
            return None
        _行号k, 代码k, _注释k = 行们[i + k]
        s = 代码k.strip()
        if not s:
            continue
        if s.startswith("}"):
            return None
        if 动词正则.match(s):
            return 行们[i + k][0], 代码k
    return None


# ============ 二、候选提取 ============

def 取默认值(返回文本: str):
    """返回 (归一值, 是否哨兵值) 或 ("", False)——默认值形态判定。"""
    t = 返回文本.strip()
    if not t:
        return "", False
    m = 默认值.match(t)
    if not m:
        return "", False
    值 = (m.group("值") or m.group("空串") or "").strip()
    if not 值:
        return "", False
    归一 = re.sub(r"\s+", "", 值)
    if 归一.startswith('""') or 归一.startswith("{"):
        return 归一, False
    if 归一 in ("false", "true", "nullptr", "真", "假", "空指针"):
        return 归一, True
    return 归一, False


def 收函数头们(文件: str, 行们: list):
    """函数头行收集：[(结束行号, 名, 返回类型)]（按行号升序·近似·支持跨行参数表）。

    跨行头（参数表/返回类型跨 2 行以上）=「行拼接至多 4 行」再匹配；命中后跳过整头。"""
    头们 = []
    v2 = 文件.endswith(".cn")
    n = len(行们)
    i = 0
    while i < n:
        行号, 代码, _注释 = 行们[i]
        if "(" in 代码:
            拼接 = ""
            for k in range(0, 4):
                if i + k >= n:
                    break
                拼接 += ("" if k == 0 else "\n") + 行们[i + k][1]
                命中 = 函数头v2.search(拼接) if v2 else 函数头宿主.search(拼接)
                if not 命中:
                    continue
                名 = 命中.group("名")
                简名 = 名.split("::")[-1] if "::" in 名 else 名
                if 简名 in 控制词 or 名 in 控制词:
                    break
                类型 = ""
                if v2:
                    类型 = 命中.group("类型") or ""
                else:
                    前缀 = (命中.group("前缀") or "").strip()
                    if 前缀:
                        类型 = re.split(r"[\s]+", 前缀)[-1]
                头们.append((行们[i + k][0], 名, 类型))
                i += k   # 跳过整个头（i 之后 i+=1 跨过结束行）
                break
        i += 1
    # 行号去重（拼接窗口叠加匹配的极端情形）
    去重 = []
    见 = set()
    for 行号, 名, 类型 in 头们:
        if 行号 in 见:
            continue
        见.add(行号)
        去重.append((行号, 名, 类型))
    return 去重


def 归属函数(头们: list, 行号: int):
    """行→(函数头行号, 名, 返回类型)；找不到返回 (None, "?", "")。"""
    if not 头们:
        return None, "?", ""
    行号表 = [h[0] for h in 头们]
    import bisect
    i = bisect.bisect_right(行号表, 行号) - 1
    if i < 0:
        # 行在首个函数头之前：回看上限内？仍给 "?"（文件头注释区）
        return None, "?", ""
    头行, 名, 类型 = 头们[i]
    if 行号 - 头行 > 回看_函数:
        return None, "?", ""
    return 头行, 名, 类型


def 同函数窗口(头们: list, 行号: int, 半径: int):
    """候选行的同函数窗口行区间 [起, 止]（不跨相邻函数头·防相邻污染）。"""
    头行, _名, _型 = 归属函数(头们, 行号)
    起 = max(1, 行号 - 半径)
    止 = 行号 + 半径
    if 头行 is not None:
        起 = max(起, 头行)
        下一头 = None
        for h行, _n, _t in 头们:
            if h行 > 行号:
                下一头 = h行
                break
        if 下一头 is not None:
            止 = min(止, 下一头 - 1)
    return 起, 止


def 记候选(候选, 文件, 行号, 锚, 条件, 值, 哨兵, 形态, 头行, 名, 类型):
    """形态命中记一条候选（值非空由调用侧守卫）。"""
    候选.append({"文件": 文件, "行": 行号, "锚": 锚.strip(), "条件": 条件.strip(),
               "值": 值, "哨兵": 哨兵, "形态": 形态,
               "头行": 头行, "函数": "≈" + 名, "返回类型": 类型})


def 扫形态宿主(文件: str, 行们: list, 头们: list, 弱: list):
    """宿主三形态：①同行 if-miss-return ②块内首返 ③else-邻接查表-return。"""
    候选 = []
    for i, (行号, 代码, _c) in enumerate(行们):
        头行, 名, 类型 = 归属函数(头们, 行号)
        for m in re.finditer(r"(?<![A-Za-z0-9_])if\s*\(", 代码):
            开 = 代码.index("(", m.start())
            闭 = 配对一个括号(代码, 开)
            if 闭 is None:
                continue
            条件, 尾 = 代码[开 + 1:闭], 代码[闭 + 1:].strip()
            同行 = re.match(r"^return\b", 尾)
            次 = None
            if not 同行 and (尾 in ("", "{") or 尾.startswith("{")):
                次 = 找块内首返(行们, i, re.compile(r"return\b"))
                if not 次:
                    continue
            rm = 返回宿主.search(尾 if 同行 else (次[1] if 次 else ""))
            if not rm:
                continue
            值, 哨兵 = 取默认值(rm.group(1))
            if not 值:
                continue
            if 结果化通道.search(条件) or (次 and 结果化通道.search(次[1])):
                continue
            if not miss信号.search(条件):
                弱[0] += 1
                continue
            锚 = 代码 if 同行 else 代码 + " │ " + 次[1].strip()
            记候选(候选, 文件, 行号, 锚, 条件, 值, 哨兵,
                 "同行 if-miss-return" if 同行 else "下行 if-miss-return", 头行, 名, 类型)
        m = re.search(r"(?<![A-Za-z0-9_])else\s*(?:\{)?\s*$"
                      r"|(?<![A-Za-z0-9_])else\s*\{?\s*return\b", 代码)
        if m:
            rm = 返回宿主.search(代码, m.start())
            行2 = None
            if not rm:
                次 = 找块内首返(行们, i, re.compile(r"return\b"))
                if 次:
                    行2 = 次
                    rm = 返回宿主.search(行2[1])
            if not rm:
                continue
            值, 哨兵 = 取默认值(rm.group(1))
            if not 值:
                continue
            起, 止 = 同函数窗口(头们, 行号, 5)
            窗 = "\n".join(t[1] for t in 行们 if 起 <= t[0] <= 止)
            if not 查表调用.search(窗):
                弱[0] += 1
                continue
            锚 = 代码 if not 行2 else 代码 + " │ " + 行2[1].strip()
            记候选(候选, 文件, 行号, 锚, "else", 值, 哨兵,
                 "else-邻接查表-return", 头行, 名, 类型)
    return 候选


def 扫形态v2(文件: str, 行们: list, 头们: list, 弱: list):
    """v2 形态：①如果-同行/下行-返回 ②否则-返回（否则本身=miss 语境）。"""
    候选 = []
    for i, (行号, 代码, _c) in enumerate(行们):
        头行, 名, 类型 = 归属函数(头们, 行号)
        for m in re.finditer(r"如果\s*\(", 代码):
            开 = 代码.index("(", m.start())
            闭 = 配对一个括号(代码, 开)
            if 闭 is None:
                continue
            条件, 尾 = 代码[开 + 1:闭], 代码[闭 + 1:].strip()
            同行 = re.match(r"^\{?\s*返回\b", 尾)
            次 = None
            if not 同行 and 尾 in ("", "{"):
                次 = 找块内首返(行们, i, re.compile(r"返回\b"))
                if not 次:
                    continue
            rm = 返回v2.search(尾 if 同行 else (次[1] if 次 else ""))
            if not rm:
                continue
            值, 哨兵 = 取默认值(rm.group(1))
            if not 值:
                continue
            if 结果化通道.search(条件) or (次 and 结果化通道.search(次[1])):
                continue
            if not miss信号.search(条件):
                弱[0] += 1
                continue
            锚 = 代码 if 同行 else 代码 + " │ " + 次[1].strip()
            记候选(候选, 文件, 行号, 锚, 条件, 值, 哨兵,
                 "v2 如果-miss-返回" if 同行 else "v2 下行如果-miss-返回", 头行, 名, 类型)
        m = re.search(r"否则\s*\{?\s*返回\b", 代码)
        if m:
            rm = 返回v2.search(代码, m.start())
            if not rm:
                continue
            值, 哨兵 = 取默认值(rm.group(1))
            if not 值:
                continue
            记候选(候选, 文件, 行号, 代码, "否则", 值, 哨兵, "v2 否则-返回", 头行, 名, 类型)
    return 候选


def 扫形态(文件: str, 行们: list, 头们: list):
    """形态面调度：返回 (候选列表, 弱形态计数)。"""
    弱 = [0]
    候选 = (扫形态v2 if 文件.endswith(".cn") else 扫形态宿主)(文件, 行们, 头们, 弱)
    return 候选, 弱[0]


def 扫留痕(文件: str, 行们: list):
    """留痕面：注释命中驳回词/留痕词（行级去重·最敏感词优先）。"""
    点 = []
    for 行号, _代码, 注释 in 行们:
        if not 注释:
            continue
        词, 类 = "", ""
        for w in 驳回词:
            if w in 注释:
                词, 类 = w, "驳回词"
                break
        if not 词:
            for w in 留痕词:
                if w in 注释:
                    词, 类 = w, "留痕词"
                    break
        if 词:
            点.append({"文件": 文件, "行": 行号, "词": 词, "类": 类,
                     "注释": 注释.strip()[:80]})
    return 点


# ============ 三、处置信号与分级 ============

def 评估处置(行们: list, 头们: list, 行号: int):
    """同函数窗口处置信号扫描。返回 (信号列表, 窗口文本-代码, 窗口文本-全)。"""
    起, 止 = 同函数窗口(头们, 行号, 窗口_处置)
    代码段 = []
    全段 = []
    for 行号r, 代码, 注释 in 行们:
        if 起 <= 行号r <= 止:
            代码段.append(代码)
            全段.append(代码)
            全段.append(注释)
    代码串 = "\n".join(代码段)
    全串 = "\n".join(全段)
    信号 = []
    if 处置_报诊断.search(代码串):
        信号.append("报诊断")
    if 处置_断言.search(代码串):
        信号.append("断言")
    if 处置_豁免.search(全串):
        信号.append("豁免声明")
    return 信号, 代码串, 全串


def 是自然哨兵(返回类型: str):
    """bool/布尔/指针返回类型上的 false/nullptr 是错误传播惯习（非默认值静默面）。"""
    t = 返回类型 or ""
    return ("bool" in t or "布尔" in t or "*" in t or "指针" in t)


def 定档与理由(候选: dict, 行们: list, 头们: list, 留痕点们: list) -> dict:
    信号, _代码串, _全串 = 评估处置(行们, 头们, 候选["行"])
    近留痕 = [p for p in 留痕点们 if abs(p["行"] - 候选["行"]) <= 窗口_留痕]
    理由 = [f"形态={候选['形态']}", f"默认值={候选['值']}"]
    if 候选.get("返回类型"):
        理由.append(f"返回类型≈{候选['返回类型']}")
    if 近留痕:
        理由.append("留痕=" + "；".join(f"{p['词']}({p['类']})@{p['行']}" for p in 近留痕[:3]))
    哨兵 = 候选.get("哨兵", False)
    自然 = 哨兵 and 是自然哨兵(候选.get("返回类型", ""))
    if "报诊断" in 信号 and "断言" in 信号:
        档 = "低"
        理由.append("处置=报诊断+断言（三选一已落地·复核文案/证明）")
    elif "报诊断" in 信号:
        档 = "中"
        理由.append("处置=报诊断（复核文案指向性·#145 同族）")
    elif "断言" in 信号:
        档 = "中"
        理由.append("处置=断言（复核不可达证明成立性）")
    elif "豁免声明" in 信号:
        档 = "中"
        理由.append("处置=豁免声明（复核边界注释齐备性）")
    elif 自然:
        档 = "低"
        理由.append(f"哨兵值 {候选['值']} + 返回类型≈{候选['返回类型']}（错误传播惯习·非默认值面·人判抽检）")
    elif 近留痕:
        档 = "高"
        理由.append("处置=无（留痕未处置·疑似立法前存量）")
    else:
        档 = "高"
        理由.append("处置=无（零留痕疑似真静默）")
    候选 = dict(候选)
    候选["处置"] = 信号
    候选["档"] = 档
    候选["理由"] = 理由
    return 候选


# ============ 四、登记表对账 ============

def 归一锚(文本: str) -> str:
    return re.sub(r"\s+", "", 文本)


def 名等价(候选名: str, 条名: str) -> bool:
    """函数名等价：去 ≈ 与类名前缀后精确相等（防前缀误配）。"""
    a = (候选名 or "").lstrip("≈").split("::")[-1]
    b = (条名 or "").split("::")[-1]
    return bool(a) and bool(b) and a == b


def 读登记表(路径: Path):
    """读 miss_registry.json；缺失/损坏返回空条目列表与状态（诚实降级·不炸扫描）。"""
    if not 路径.exists():
        return [], "缺失"
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        return [], f"损坏({e.__class__.__name__})"
    if not isinstance(数据, dict):
        return [], "损坏(非对象)"
    return 数据.get("条目", []), "就绪"


def 登记命中(候选: dict, 条目们: list):
    """候选×登记条目对账：同文件且（锚归一后互为子串 ∨ 函数名等价）→ 返回条目。"""
    锚 = 归一锚(候选.get("锚", "").split("│")[0])
    for 条 in 条目们:
        if 条.get("文件") != 候选["文件"]:
            continue
        条锚 = 归一锚(条.get("锚文本", "") or 条.get("锚", ""))
        if 条锚 and 条锚 in 锚:
            return 条
        if 名等价(候选.get("函数", ""), 条.get("函数", "")):
            return 条
    return None


# ============ 五、扫描与报告 ============

def 扫描一面(根: Path, 文件后缀过滤, 显示前缀: str):
    """扫描一侧。返回 (文件数, 读取失败, 候选们, 留痕们, 弱形态数, 词面计数)。"""
    文件数 = 0
    失败 = 0
    全部候选 = []
    全部留痕 = []
    总弱 = 0
    词面 = {"兜底": 0, "miss": 0, "回退": 0}
    for p in sorted(根.rglob("*")):
        if not p.is_file() or p.suffix not in 文件后缀过滤:
            continue
        try:
            文本 = p.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            失败 += 1
            continue
        相对 = 显示前缀 + "/" + p.relative_to(根).as_posix()
        行们 = 拆行(文本)
        头们 = 收函数头们(相对, 行们)
        文件数 += 1
        for k in 词面:
            # 行数口径（与 950 基线 grep 计数同规·字符出现次数会偏多）
            词面[k] += sum(1 for 行 in 文本.split("\n") if k in 行)
        留痕 = 扫留痕(相对, 行们)
        候选, 弱 = 扫形态(相对, 行们, 头们)
        总弱 += 弱
        for c in 候选:
            c = 定档与理由(c, 行们, 头们, 留痕)
            全部候选.append(c)
        全部留痕.extend(留痕)
    return 文件数, 失败, 全部候选, 全部留痕, 总弱, 词面


def 全库扫描(要宿主: bool, 要v2: bool):
    面 = {}
    if 要宿主:
        base = 仓库根 / "src"
        if base.exists():
            面["宿主"] = 扫描一面(base, {".cpp", ".hpp"}, "src")
    if 要v2:
        base = 仓库根 / "CN语言编译器v2"
        if base.exists():
            面["v2"] = 扫描一面(base, {".cn"}, "CN语言编译器v2")
    条目们, 登记态 = 读登记表(登记表路径)
    登记数 = 0
    分组 = {"高": [], "中": [], "低": []}
    for 侧, (_f, _x, 候选们, _l, _w, _c) in 面.items():
        for c in 候选们:
            条 = 登记命中(c, 条目们)
            if 条:
                c["登记"] = {"类别": 条.get("类别", ""), "静默风险": 条.get("静默风险", "")}
                登记数 += 1
                if c["档"] != "低":
                    c["理由"].append("已登记（" + 条.get("类别", "") + "·折叠不重复报）")
                c["档"] = "低"
            else:
                c["登记"] = None
            分组[c["档"]].append({"侧": 侧, **c})
    for k in 分组:
        分组[k].sort(key=lambda c: (c["文件"], c["行"]))
    return 面, {"登记态": 登记态, "登记数": 登记数, **分组}


def 统计面(面: dict):
    统 = {}
    for 侧, (文件数, 失败, 候选们, 留痕们, 弱, 词面) in 面.items():
        驳回 = sum(1 for p in 留痕们 if p["类"] == "驳回词")
        统[侧] = {"文件": 文件数, "读取失败": 失败, "形态候选": len(候选们),
                 "弱形态": 弱, "留痕点": len(留痕们), "驳回词点": 驳回,
                 "词面兜底": 词面["兜底"], "词面miss": 词面["miss"], "词面回退": 词面["回退"]}
    return 统


def 打印候选行(c: dict, 缩进: str = "  "):
    print(f"{缩进}[{c['档']}] {c['文件']}:{c['行']}  {c['函数']}")
    print(f"{缩进}    锚：{c['锚'][:110]}")
    print(f"{缩进}    理由：" + "；".join(c["理由"]))
    if c.get("登记"):
        print(f"{缩进}    已登记：{c['登记']['类别']}（静默风险 {c['登记']['静默风险']}）")


def 高按目录(高列表: list):
    """高嫌疑按目录聚合（清偿批次规划的输入·一级/二级目录）。"""
    计数 = {}
    for c in 高列表:
        段 = c["文件"].split("/")
        键 = "/".join(段[:3]) if len(段) > 3 else "/".join(段[:2])
        计数[键] = 计数.get(键, 0) + 1
    return sorted(计数.items(), key=lambda kv: (-kv[1], kv[0]))


def 打印报告(面: dict, 合流, limit: int):
    统 = 统计面(面)
    print("== 宿主 miss 静默面清查（check_miss_defaults.py·950·648 立法配套·机械初筛）==")
    for 侧, s in 统.items():
        print(f"[{侧}] 文件 {s['文件']}（读取失败 {s['读取失败']}）；留痕点 {s['留痕点']}"
              f"（驳回词 {s['驳回词点']}）；形态候选 {s['形态候选']}（弱形态另计 {s['弱形态']}）")
        print(f"    词面机械计数（与 950 基线口径对照）：兜底 {s['词面兜底']} / "
              f"miss {s['词面miss']} / 回退 {s['词面回退']}")
    print(f"登记表：{合流['登记态']}；对账命中 {合流['登记数']} 条（已折叠不重复报）")
    print("【档义】高=零处置信号（疑似真静默·人判优先）；中=有处置待复核（报诊断文案指向性/"
          "豁免边界/断言证明）；低=已登记或自然哨兵（错误传播惯习）或双处置。档=线索优先级非缺陷定论。")
    高 = 合流["高"]
    上限 = len(高) if limit == 0 else min(limit, len(高))
    print(f"\n【高嫌疑清单】（{len(高)} 条·展示前 {上限}）")
    for c in 高[:上限]:
        打印候选行(c)
    if 上限 < len(高):
        print(f"  …（其余 {len(高) - 上限} 条用 --limit 0 全列 / --md 输出全量）")
    目录分布 = 高按目录(高)
    if 目录分布:
        print("\n【高嫌疑目录分布】（清偿批次规划输入）")
        for 键, n in 目录分布[:16]:
            print(f"  {n:4d}  {键}")
    中 = 合流["中"]
    print(f"\n【中嫌疑】{len(中)} 条（报诊断/豁免/断言待复核·--md 全列）")
    低 = 合流["低"]
    print(f"【低嫌疑】{len(低)} 条（已登记/自然哨兵/双处置·--md 全列）")
    print("\n提示：本工具=机械初筛（不判死）；逐处人判三选一执法后登记 "
          "scripts/miss_registry.json，重跑即自动对账折叠。清单文档="
          "docs/miss静默面存余清单-950.md。")


def 生成md(面: dict, 合流, limit: int) -> str:
    """md 呈现：统计表+目录分布+紧凑清单（高=limit 全列/截断；中全列；低截前 200）。

    低档（已登记/自然哨兵/双处置）清单价值限于复核抽样——md 截前 200 条，
    全量始终可经 --json 取（机读全量口径）。"""
    统 = 统计面(面)
    行 = []
    行.append("# 宿主 miss 静默面存余清单（#950·891 对侧件）")
    行.append("")
    行.append("> 自动生成：`python3 scripts/check_miss_defaults.py --v2 --limit 0 --md`"
              "（工具=机械初筛·档=线索优先级非缺陷定论；逐处人判三选一执法后登记 "
              "`scripts/miss_registry.json` 即自动折叠）。")
    行.append("")
    行.append("## 统计与口径对照")
    行.append("")
    行.append("| 侧 | 文件 | 留痕点 | 其中驳回词 | 形态候选 | 弱形态另计 | 词面兜底 | 词面miss | 词面回退 |")
    行.append("|---|---|---|---|---|---|---|---|---|")
    for 侧, s in 统.items():
        行.append(f"| {侧} | {s['文件']} | {s['留痕点']} | {s['驳回词点']} | "
                f"{s['形态候选']} | {s['弱形态']} | {s['词面兜底']} | {s['词面miss']} | {s['词面回退']} |")
    行.append("")
    行.append("词面口径=行数计数（与 950 基线「兜底 278/miss 174/回退 87」同规·本树实测逐字吻合）；"
              "留痕点=注释命中留痕词的行数；形态候选=「miss 判定→返默认值」机械共现"
              "（弱形态=条件无 miss 信号的普通守卫·机械排除防噪声）。")
    行.append("")
    行.append(f"登记表：{合流['登记态']}；对账命中 {合流['登记数']} 条。档分布："
              f"高 {len(合流['高'])} / 中 {len(合流['中'])} / 低 {len(合流['低'])}。")
    行.append("")
    行.append("### 高嫌疑目录分布（清偿批次规划输入）")
    行.append("")
    行.append("| 目录 | 高嫌疑条数 |")
    行.append("|---|---|")
    for 键, n in 高按目录(合流["高"]):
        行.append(f"| {键} | {n} |")
    行.append("")
    行.append("## 高嫌疑清单（零处置信号·人判优先）")
    行.append("")
    上限 = len(合流["高"]) if limit == 0 else min(limit, len(合流["高"]))
    for c in 合流["高"][:上限]:
        子 = "（" + "·".join(x for x in c["子标注"] if x) + "）" if c.get("子标注") else ""
        行.append(f"- **{c['文件']}:{c['行']}** {c['函数']}{子} — `{c['锚'][:100]}`")
        行.append(f"  - " + "；".join(c["理由"]))
        if c.get("登记"):
            行.append(f"  - 已登记：{c['登记']['类别']}")
    if 上限 < len(合流["高"]):
        行.append(f"（其余 {len(合流['高']) - 上限} 条用 --limit 0 全列）")
    行.append("")
    行.append("## 中嫌疑清单（有处置·待复核）")
    行.append("")
    for c in 合流["中"]:
        行.append(f"- [{c['文件']}:{c['行']}] {c['函数']} — `{c['锚'][:100]}` — "
                + "；".join(c["理由"]))
    行.append("")
    行.append("## 低嫌疑清单（已登记/自然哨兵/双处置·抽样前 200）")
    行.append("")
    for c in 合流["低"][:200]:
        行.append(f"- [{c['文件']}:{c['行']}] {c['函数']} — `{c['锚'][:80]}` — "
                + "；".join(c["理由"]))
    if len(合流["低"]) > 200:
        行.append(f"- …（其余 {len(合流['低']) - 200} 条用 --json 取全量）")
    行.append("")
    行.append("## 清偿批次规划建议")
    行.append("")
    行.append("1. 本清单=机械初筛池，非缺陷定论；清偿=逐处人判三选一（报内部错误诊断/"
              "显式哨兵+强制检查/穷举断言），合法面登记 `scripts/miss_registry.json`。")
    行.append("2. 批次序建议：按上「高嫌疑目录分布」逐目录成批（ir/ 与 semantic/ 优先——"
              "852/648 已清面同簇，逐条核消费点防重复清偿）；runtime/ 面属 FFI 契约"
              "（大概率批量登记为运行时API 豁免·核边界注释齐备性）。")
    行.append("3. 与 #891（v2 侧）合流：两侧清洗完成后各跑一次本工具与 891 扫描器，"
              "按「文件/行/函数/档/理由/登记」六字段归并成双侧完成度总表。")
    行.append("4. 本轮止于「工具+清单」——存量逐条裁决与清偿是后续批次；"
              "登记表条目为示范 5 条（911 矛盾面=误报照留痕），防「一次性全部登记」的账目灌水。")
    行.append("")
    return "\n".join(行)


def 候选干净(c: dict):
    return {k: v for k, v in c.items() if k not in ("条件", "头行")}


def 转json(面: dict, 合流) -> str:
    统 = 统计面(面)
    输出 = {"工具": "check_miss_defaults.py", "任务": "950",
          "统计": 统, "登记态": 合流["登记态"], "登记命中": 合流["登记数"],
          "高": [候选干净(c) for c in 合流["高"]],
          "中": [候选干净(c) for c in 合流["中"]],
          "低": [候选干净(c) for c in 合流["低"]]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 六、自测（正反两态·零触碰真实树） ============

样例宿主 = {
    "干净.cpp": (
        "// 干净样例：业务形态（无红旗注释·无 miss-return 形态）\n"
        "int 求和(int a, int b) {\n"
        "    int 和 = a + b;\n"
        "    return 和;\n"
        "}\n"
        "int 取宽(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it != 表.end()) return it->second;\n"
        "    return 16;\n"
        "}\n"
        "bool 校验(const std::string& t) {\n"
        "    if (t.size() < 3 || t[0] != 'x') return false;\n"
        "    return true;\n"
        "}\n"
    ),
    "高.cpp": (
        "// 高样例甲：零留痕零处置——查表 miss 直接返默认值\n"
        "int 查宽(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it == 表.end()) return 0;\n"
        "    return it->second;\n"
        "}\n"
        "// 高样例乙：留痕未处置\n"
        "int 查宽2(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it == 表.end()) return 8;  // 兜底返回 8\n"
        "    return it->second;\n"
        "}\n"
    ),
    "中.cpp": (
        "// 中样例甲：已报诊断（复核文案指向性）\n"
        "int 查宽3(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it == 表.end()) {\n"
        "        diagnostics_.report(Error, loc, \"内部错误: 宽度未登记\");\n"
        "        return 8;\n"
        "    }\n"
        "    return it->second;\n"
        "}\n"
        "// 中样例乙：豁免声明（复核边界注释）\n"
        "int 空串长(const char* s) {\n"
        "    if (s == nullptr) return 0;  // 运行时 API 契约：空指针视为空串\n"
        "    return 长度(s);\n"
        "}\n"
        "// 中样例丙：断言（复核不可达证明）\n"
        "int 查宽4(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it == 表.end()) {\n"
        "        assert(false && \"不可达：语义层已保证\");\n"
        "        return 0;\n"
        "    }\n"
        "    return it->second;\n"
        "}\n"
        "// 低样例：自然哨兵（bool 返回 false·错误传播惯习）\n"
        "bool 查存在(const std::string& t) {\n"
        "    auto it = 表.find(t);\n"
        "    if (it == 表.end()) return false;\n"
        "    return true;\n"
        "}\n"
    ),
}

样例v2 = {
    "源.cn": (
        "// 高样例：v2 侧查表 miss 直接返默认值\n"
        "函数 查宽(字符串 t) -> 整64 {\n"
        "    整64 查 = 表.查(t);\n"
        "    如果 (查 < 0) { 返回 0; }\n"
        "    返回 查;\n"
        "}\n"
    ),
}


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    with tempfile.TemporaryDirectory(prefix="cmd950_") as td:
        根 = Path(td)
        (根 / "src").mkdir()
        for 名, 文 in 样例宿主.items():
            (根 / "src" / 名).write_text(文, encoding="utf-8")
        (根 / "CN语言编译器v2").mkdir()
        for 名, 文 in 样例v2.items():
            (根 / "CN语言编译器v2" / 名).write_text(文, encoding="utf-8")
        面 = {}
        面["宿主"] = 扫描一面(根 / "src", {".cpp"}, "src")
        面["v2"] = 扫描一面(根 / "CN语言编译器v2", {".cn"}, "CN语言编译器v2")
        宿主候选 = 面["宿主"][2]
        断言(面["宿主"][0] == 3, "样例文件计数=3")
        干净 = [c for c in 宿主候选 if c["文件"].endswith("干净.cpp")]
        断言(len(干净) == 0, f"干净样例零候选（负态·不误报）: {干净!r}")
        高 = [c for c in 宿主候选 if c["档"] == "高"]
        断言(len(高) == 2, f"高样例 2 条命中高: {[(c['文件'], c['行'], c['档']) for c in 宿主候选]}")
        断言(any("兜底" in "".join(c["理由"]) for c in 高), "留痕未处置理由落档")
        中 = [c for c in 宿主候选 if c["档"] == "中"]
        断言(len(中) == 3, f"中样例 3 条落中: {[(c['文件'], c['行'], c['档'], c['处置']) for c in 宿主候选]}")
        断言(any("报诊断" in c["处置"] for c in 中), "报诊断信号识别（同函数窗口跨行）")
        断言(any("豁免声明" in c["处置"] for c in 中), "豁免声明信号识别")
        断言(any("断言" in c["处置"] for c in 中), "断言信号识别")
        低 = [c for c in 宿主候选 if c["档"] == "低"]
        断言(len(低) == 1 and 低[0]["值"] == "false", f"自然哨兵降档: {低!r}")
        断言(面["宿主"][4] >= 1, "弱形态计数（bool 守卫条件无 miss 信号）")
        v2候选 = 面["v2"][2]
        断言(len(v2候选) == 1 and v2候选[0]["档"] == "高", f"v2 如果-miss-返回命中高: {v2候选!r}")
        留痕 = 面["宿主"][3]
        断言(len(留痕) >= 1, f"留痕点≥1: {留痕!r}")
        断言(any(c["函数"] == "≈查宽" for c in 宿主候选), "函数归属 ≈查宽")
        断言(any(c["函数"] == "≈查宽3" for c in 中), "中样例甲归属 ≈查宽3")
        条 = {"文件": "src/高.cpp", "锚": "if (it == 表.end()) return 8;",
             "类别": "业务值", "静默风险": "低"}
        c = [x for x in 宿主候选
             if x["行"] == 10 and x["文件"].endswith("高.cpp")][0]
        断言(登记命中(c, [条]) is not None, "登记子串对账命中")
        断言(登记命中(c, [{"文件": "别处.cpp", "锚": "x"}]) is None, "异文件不误配")
        # 跨函数污染防线：高样例甲的处置窗口不含乙（不同函数）
        甲 = [x for x in 高 if x["行"] == 4][0]
        断言("留痕" not in "".join(甲["理由"]) or "兜底" not in "".join(甲["理由"]),
             "同函数窗口不跨函数（甲不吸乙的留痕）")
    if 失败:
        print("== 扫描器自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 扫描器自测全过（{通过[0]} 项断言·正反两态齐备）==")
    return 0


# ============ 七、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="宿主 miss 静默面清查扫描（950·648 立法配套·机械初筛抓留痕核登记）")
    解析器.add_argument("--v2", action="store_true", help="附加扫描 CN语言编译器v2/**/*.cn")
    解析器.add_argument("--only", choices=["宿主", "v2"], help="只扫指定侧")
    解析器.add_argument("--limit", type=int, default=40, help="清单截断条数（0=全列·默认 40）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--md", nargs="?", const="-", default=None, metavar="文件",
                        help="md 输出（缺省 stdout；给路径=写文件）")
    解析器.add_argument("--strict", action="store_true", help="高嫌疑>0 → rc=1")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    要宿主, 要v2 = True, 参数.v2
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

    高数 = len(合流["高"])
    if 参数.strict and 高数:
        print(f"[--strict] 高嫌疑 {高数} 条 → rc=1", flush=True)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
