#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_naming.py —— 标识符禁嵌任务号/日期机械门禁（#1716 立法执行面）。

背景（#1716·2026-10-27 用户批准立法）：任何标识符（文件/函数/变量/类型/常量）名字须说
「它是什么」而非「它出自哪」——禁嵌任务号、日期与一次性过程编号（病名实录
classReturnOwnership1479 / cb1470 / n1593；同语义双名分裂 kMaxVirtualRegId1256 vs
kMaxVirtualRegId1212）。任务号只出现在注释与提交信息（证据溯源归注释·过程记录归提交
信息）。立法只落条文无机械执法时不可执行（批1 之后合入提交仍新增带号名）——本脚本为
其执法面：判红即 L1 拦截提交。

扫描面（递归）：
  src/**/*.cpp|*.hpp|*.h|*.cc
  CN语言编译器v2/**/*.cn
  scripts/**/*.py
同时扫文件名（同扩展名·文件名主干按同一判据切 token）。

判据：
  ① 预处理剥除注释与字符串/字符字面量（C++/CN：行注释、块注释、双引号串、
     单引号串、三引号串；Python：#、单/双/三引号）——剥除保留换行，行号与原文一致；
  ② 按 token 切分 [A-Za-z0-9_\\u4e00-\\u9fff]+（中英文混合算一个 token）；
  ③ token 内出现连续 3~8 位数字段（前后不接数字）即命中；跳过以数字开头的 token
     （数字字面量）与纯数字 token。

豁免（逐条·不按类批量）：
  ① 数字 ∈ {128,256,512,1024}（位宽/容量语义：整128 / i128 / IR容器128）；
  ② 数字以 0 开头（001/002 等规范号）；
  ③ 路径面 tests/e2e、tests/cnsmith_hits、tests/cnsmith_cases、plans、website、docs、
     归档、项目记忆、target —— 文件名与内容都跳过（用例编号/归档样例/文档编号属规范面）；
  ④ 协议注释（命中行或其上一行）：`命名豁免<类别>: 原因`，类别限
     位宽语义|第三方错误码|用例编号|归档样例|规范编号；缺原因或类别非法=不生效（按命中算）。

冻结基线（只减不增·照 check_miss_gate.py 模式）：scripts/naming_freeze_baseline.json =
  JSON 对象，键 = 「相对路径|标识符」，值 = 出现次数（稳定排序）。存量全在基线=绿；
  新增键=红；同键计数增加=红；键消失或计数下降=绿（提示可 --freeze 收缩）。首次上线=
  以当前实测生成基线，存量清偿（#1762 宿主改名等）后 --freeze 收账。

用法：
  python3 scripts/check_naming.py                    # 门禁（rc=1=有红·gate_quick 静态段调用）
  python3 scripts/check_naming.py --report           # 报告模式（目录汇总+top 清单·rc=0）
  python3 scripts/check_naming.py --freeze           # 以当前实测重写冻结基线
  python3 scripts/check_naming.py --approval         # 列出全部协议注释豁免条目（审计面）
  python3 scripts/check_naming.py --selftest         # 自测（临时样例树·正反两态·秒级）

退出码：0=绿；1=红（有新增/增长/读取失败）；2=用法错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）：注解里的 list[str] 等泛型下标在 3.8 会
# TypeError——future import 使注解字符串化。
from __future__ import annotations

import argparse
import json
import re
import sys
import tempfile
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
基线文件 = 脚本目录 / "naming_freeze_baseline.json"

# ============ 常量与判据表 ============

# 扫描面：相对仓库根的前缀 → 扩展名集合（任务判据明列）
扫描面 = (
    ("src", (".cpp", ".hpp", ".h", ".cc")),
    ("CN语言编译器v2", (".cn",)),
    ("scripts", (".py",)),
)
# 规范/归档面：文件名与内容一律跳过（用例编号/归档样例/文档编号属规范面）
豁免路径面 = ("tests/e2e/", "tests/cnsmith_hits/", "tests/cnsmith_cases/",
          "plans/", "website/", "docs/", "归档/", "项目记忆/", "target/")
# 标识符 token（中英文混合算一个 token）
标识符模式 = re.compile(r"[0-9A-Za-z_\u4e00-\u9fff]+")
# 连续 3~8 位数字段（前后不接数字；9 位以上不是 3~8 位段）
数字段模式 = re.compile(r"(?<![0-9])([0-9]{3,8})(?![0-9])")
# 位宽/容量语义数字（整128 / i128 / IR容器128）
豁免数字 = ("128", "256", "512", "1024")
群豁免数字 = set(豁免数字)
# 协议注释（逐条豁免）：行内写「命名豁免」+ 类别 + 冒号 + 原因——
# 类别枚举之外/缺原因=不生效
协议注释模式 = re.compile(r"命名豁免\s*([^\s:：]+)\s*[:：]\s*(.*)$")
协议类别枚举 = ("位宽语义", "第三方错误码", "用例编号", "归档样例", "规范编号")

# 特殊字符跳转（普通代码区用 C 正则快进·只对注释/字符串起点循环）
_特殊_cpp = re.compile(r"[\"'/]")
_特殊_py = re.compile(r"[\"'#]")


# ============ 一、剥除（注释/字符串抹空格·换行保留=行号不变） ============

def _抹(格: list, 起: int, 止: int):
    """把 [起,止) 内容抹为空格（换行保留）。"""
    for k in range(起, min(止, len(格))):
        if 格[k] != "\n":
            格[k] = " "


def _串尾(源: str, 引号位: int) -> int:
    """普通字符串/字符字面量结束下标（含结束引号；未闭合=文尾）。

    转义判定=引号前连续反斜杠奇偶（奇数=被转义）。"""
    引 = 源[引号位]
    n = len(源)
    j = 引号位 + 1
    while True:
        j = 源.find(引, j)
        if j < 0:
            return n
        反斜杠数 = 0
        k = j - 1
        while k > 引号位 and 源[k] == "\\":
            反斜杠数 += 1
            k -= 1
        if 反斜杠数 % 2 == 0:
            return j + 1
        j += 1


def _原始串尾(源: str, 引号位: int):
    """C++ 原始字符串 R"分隔(内容)分隔" 结束下标；非原始串返回 None。

    前缀必须整体紧邻引号（R / u8R / uR / UR / LR），防普通标识符误判。"""
    k = 引号位 - 1
    while k >= 0 and (源[k].isalnum() or 源[k] == "_"):
        k -= 1
    前缀 = 源[k + 1:引号位]
    if 前缀 not in ("R", "u8R", "uR", "UR", "LR"):
        return None
    括 = 源.find("(", 引号位 + 1)
    if 括 < 0:
        return len(源)
    分隔 = 源[引号位 + 1:括]
    if len(分隔) > 16 or any(ch.isspace() or ch in "()\\\"" for ch in 分隔):
        return None                      # 非法分隔符=按普通串处理（不静默跳过）
    收 = 源.find(")" + 分隔 + '"', 括 + 1)
    return len(源) if 收 < 0 else 收 + len(分隔) + 2


def 剥除_cpp(源: str, 仅字符串: bool = False) -> str:
    """剥除 C++/CN 注释与字符串/字符字面量（换行保留·等长掩码）。

    仅字符串=True 时保留注释（协议注释只在注释区生效·防字符串内示例被误认）。"""
    格 = list(源)
    n = len(源)
    i = 0
    while True:
        m = _特殊_cpp.search(源, i)
        if not m:
            break
        i = m.start()
        c = 源[i]
        if c == "/":
            if 仅字符串:
                # 注释整体不进入字符串语义（其内引号不是字符串定界）
                if i + 1 < n and 源[i + 1] == "/":
                    j = 源.find("\n", i)
                    i = n if j < 0 else j
                elif i + 1 < n and 源[i + 1] == "*":
                    收 = 源.find("*/", i + 2)
                    i = n if 收 < 0 else 收 + 2
                else:
                    i += 1
                continue
            if i + 1 < n and 源[i + 1] == "/":
                j = 源.find("\n", i)
                j = n if j < 0 else j
            elif i + 1 < n and 源[i + 1] == "*":
                j = 源.find("*/", i + 2)
                j = n if j < 0 else j + 2
            else:
                i += 1
                continue
        elif c == '"':
            if 源.startswith('"""', i):
                # CN 三引号字符串（跨行）
                收 = 源.find('"""', i + 3)
                j = n if 收 < 0 else 收 + 3
            else:
                j = _原始串尾(源, i)
                if j is None:
                    j = _串尾(源, i)
        else:
            # C++14 数字分隔符（1'000'000）不当作字符字面量起点
            if 0 < i < n - 1 and 源[i - 1].isdigit() and 源[i + 1].isdigit():
                i += 1
                continue
            j = _串尾(源, i)
        _抹(格, i, j)
        i = j
    return "".join(格)


def _py单行串尾(源: str, 起: int, 引: str) -> int:
    """Python 单引号串结束下标（转义跳过；未闭合到行尾）。"""
    n = len(源)
    j = 起 + 1
    while j < n:
        if 源[j] == "\\":
            j += 2
            continue
        if 源[j] == 引:
            return j + 1
        if 源[j] == "\n":
            return j
        j += 1
    return n


def _py三引号尾(源: str, 起: int, 引: str) -> int:
    """Python 三引号串结束下标（未闭合=文尾）。"""
    n = len(源)
    三 = 引 * 3
    j = 起 + 3
    while j < n:
        if 源[j] == "\\":
            j += 2
            continue
        if 源.startswith(三, j):
            return j + 3
        j += 1
    return n


def 剥除_py(源: str, 仅字符串: bool = False) -> str:
    """剥除 Python 注释与字符串/三引号串（换行保留·等长掩码）。

    仅字符串=True 时保留注释（协议注释只在注释区生效）。"""
    格 = list(源)
    n = len(源)
    i = 0
    while True:
        m = _特殊_py.search(源, i)
        if not m:
            break
        i = m.start()
        c = 源[i]
        if c == "#":
            if 仅字符串:
                j = 源.find("\n", i)
                i = n if j < 0 else j
                continue
            j = 源.find("\n", i)
            j = n if j < 0 else j
        elif 源.startswith(c * 3, i):
            j = _py三引号尾(源, i, c)
        else:
            j = _py单行串尾(源, i, c)
        _抹(格, i, j)
        i = j
    return "".join(格)


# ============ 二、命名判据（token → 违规数字段） ============

def 违规数字(token: str):
    """token 内首个未豁免数字段；不命中返回 None。

    跳过以数字开头的 token（数字字面量/纯数字）；连续 3~8 位段才判
    （1~2 位如 arg0/utf8/xmm0/整32 是语义数字·不属任务号/日期）。"""
    if not token or token[0].isdigit():
        return None
    for m in 数字段模式.finditer(token):
        段 = m.group(1)
        if 段 in 群豁免数字 or 段.startswith("0"):
            continue
        return 段
    return None


def 路径豁免(相对: str) -> bool:
    """规范/归档面路径（文件名与内容都跳过）。"""
    规整 = 相对.replace("\\", "/")
    return any(规整.startswith(面) for 面 in 豁免路径面)


def 解协议注释(行: str):
    """解析一行内的协议注释；返回 {类别, 原因, 生效} 或 None。"""
    m = 协议注释模式.search(行)
    if not m:
        return None
    类别 = m.group(1).strip()
    原因 = m.group(2).strip()
    if 原因.endswith("*/"):
        原因 = 原因[:-2].strip()
    return {"类别": 类别, "原因": 原因,
            "生效": 类别 in 协议类别枚举 and bool(原因)}


def 有协议豁免(注行们: list, 序号: int) -> bool:
    """命中行（1-based）或上一行的注释区含生效协议注释 → 豁免。"""
    for 行号 in (序号, 序号 - 1):
        if 行号 < 1 or 行号 > len(注行们):
            continue
        行 = 注行们[行号 - 1]
        if "命名豁免" not in 行:
            continue
        条 = 解协议注释(行)
        if 条 is not None and 条["生效"]:
            return True
    return False


def 扫内容(相对: str, 原文: str, 是否py: bool) -> list:
    """单文件内容扫描：剥除后逐行切 token→判据→协议豁免。"""
    命中们 = []
    if not 数字段模式.search(原文):
        return 命中们                     # 原文无 3~8 位数字段=剥除后不可能命中
    剥 = 剥除_py(原文) if 是否py else 剥除_cpp(原文)
    注行们 = None                         # 保留注释的掩码（仅命中时惰性构造）
    for 序号, 行 in enumerate(剥.split("\n"), 1):
        if not 数字段模式.search(行):
            continue
        for m in 标识符模式.finditer(行):
            tok = m.group(0)
            段 = 违规数字(tok)
            if 段 is None:
                continue
            if 注行们 is None:
                注 = 剥除_py(原文, True) if 是否py else 剥除_cpp(原文, True)
                注行们 = 注.split("\n")
            if 有协议豁免(注行们, 序号):
                continue
            命中们.append({"文件": 相对, "行": 序号, "标识符": tok,
                        "数字": 段, "来源": "内容"})
    return 命中们


def 扫文件名(相对: str) -> list:
    """文件名主干扫描（同判据·无行号）。"""
    命中们 = []
    for m in 标识符模式.finditer(Path(相对).stem):
        tok = m.group(0)
        段 = 违规数字(tok)
        if 段 is None:
            continue
        命中们.append({"文件": 相对, "行": 0, "标识符": tok,
                    "数字": 段, "来源": "文件名"})
    return 命中们


def 收集协议(相对: str, 原文: str, 是否py: bool) -> list:
    """收集文件注释区内全部协议注释（--approval 审计面·含不生效条目）。"""
    if "命名豁免" not in 原文:
        return []
    剥 = 剥除_py(原文, True) if 是否py else 剥除_cpp(原文, True)
    条们 = []
    for 序号, 行 in enumerate(剥.split("\n"), 1):
        if "命名豁免" not in 行:
            continue
        条 = 解协议注释(行)
        if 条 is not None:
            条们.append({"文件": 相对, "行": 序号, **条})
    return 条们


# ============ 三、全库扫描 ============

def 枚举文件(根: Path):
    """按扫描面枚举文件 → (相对路径, 绝对路径, 是否py)。"""
    for 前缀, 后缀们 in 扫描面:
        基 = 根 / 前缀
        if not 基.is_dir():
            continue
        for p in sorted(基.rglob("*")):
            if not p.is_file() or p.suffix.lower() not in 后缀们:
                continue
            相对 = 前缀 + "/" + p.relative_to(基).as_posix()
            if 路径豁免(相对):
                continue
            yield 相对, p, p.suffix.lower() == ".py"


def 全库扫描(根: Path) -> dict:
    """扫描全部扫描面：返回 {命中, 协议, 统计}。"""
    命中们 = []
    协议们 = []
    统计 = {"文件": 0, "内容": 0, "文件名": 0, "读取失败": []}
    for 相对, p, 是否py in 枚举文件(根):
        统计["文件"] += 1
        段命 = 扫文件名(相对)
        统计["文件名"] += len(段命)
        命中们.extend(段命)
        try:
            原文 = p.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError) as e:
            统计["读取失败"].append((相对, e.__class__.__name__))
            continue
        内容命 = 扫内容(相对, 原文, 是否py)
        统计["内容"] += len(内容命)
        命中们.extend(内容命)
        协议们.extend(收集协议(相对, 原文, 是否py))
    return {"命中": 命中们, "协议": 协议们, "统计": 统计}


def 建键(命中: dict) -> str:
    return 命中["文件"] + "|" + 命中["标识符"]


# ============ 四、基线读写与判定 ============

def 读基线(路径: Path):
    """读冻结基线（JSON 对象 键→次数）；缺失返回 None；损坏=退出（不静默）。"""
    if not 路径.exists():
        return None
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[命名门禁] 基线损坏（{e.__class__.__name__}）：{路径}")
    if not isinstance(数据, dict):
        raise SystemExit(f"[命名门禁] 基线格式非法（应为 JSON 对象）：{路径}")
    计 = Counter()
    for 键, 次 in 数据.items():
        if not isinstance(键, str) or "|" not in 键:
            raise SystemExit(f"[命名门禁] 基线键非法：{键!r}")
        if not isinstance(次, int) or isinstance(次, bool) or 次 <= 0:
            raise SystemExit(f"[命名门禁] 基线计数非法：{键}={次!r}")
        计[键] = 次
    return 计


def 写基线(路径: Path, 当前: Counter) -> int:
    """写冻结基线（键稳定排序·JSON 对象）。返回键数。"""
    数据 = {键: 当前[键] for 键 in sorted(当前) if 当前[键] > 0}
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1) + "\n",
                 encoding="utf-8")
    return len(数据)


def 判定(当前: Counter, 基线: Counter) -> dict:
    """基线对账：新键=红·计数增长=红·消失/下降=绿（可收缩）。"""
    新键 = sorted(set(当前) - set(基线))
    增长 = sorted((键, 基线[键], 当前[键]) for 键 in set(当前) & set(基线)
                if 当前[键] > 基线[键])
    消失 = sorted(set(基线) - set(当前))
    下降 = sorted((键, 基线[键], 当前[键]) for 键 in set(当前) & set(基线)
                if 当前[键] < 基线[键])
    return {"新键": 新键, "增长": 增长, "消失": 消失, "下降": 下降,
            "红": bool(新键 or 增长)}


# ============ 五、报告输出 ============

def 目录归属(相对: str) -> str:
    前 = 相对.split("/", 1)[0]
    return 前 if 前 in ("src", "CN语言编译器v2", "scripts") else "其他"


def 分目录统计(计: Counter) -> dict:
    """键计数 → 目录 → (处数, 键数)。"""
    果 = {}
    for 键, 次 in 计.items():
        目 = 目录归属(键.split("|", 1)[0])
        旧 = 果.get(目, (0, 0))
        果[目] = (旧[0] + 次, 旧[1] + 1)
    return 果


def 打印扫描摘要(扫描果: dict):
    统计 = 扫描果["统计"]
    内容计 = Counter(建键(h) for h in 扫描果["命中"] if h["来源"] == "内容")
    文件名计 = Counter(建键(h) for h in 扫描果["命中"] if h["来源"] == "文件名")
    全键 = set(内容计) | set(文件名计)
    print(f"[扫描] 文件 {统计['文件']} · 内容命中 {统计['内容']} 处 · "
          f"文件名命中 {统计['文件名']} 处 · 唯一键 {len(全键)}")
    目们 = 分目录统计(内容计)
    分布 = " · ".join(f"{目} {目们[目][0]} 处（键 {目们[目][1]}）"
                    for 目 in ("src", "CN语言编译器v2", "scripts") if 目 in 目们)
    print(f"[分布] {分布} · 文件名 {统计['文件名']} 处")


def 打印读取失败(扫描果: dict):
    失败 = 扫描果["统计"]["读取失败"]
    if not 失败:
        return
    print(f"[命名门禁·红] 读取失败 {len(失败)} 文件（无法核验·不静默跳过）：")
    for 相对, 类 in 失败[:20]:
        print(f"  ! {相对}（{类}）")


def 打印豁免审计(协议们: list):
    """--approval：全列协议注释豁免条目（审计面·含不生效）。"""
    生效 = [条 for 条 in 协议们 if 条["生效"]]
    不生效 = [条 for 条 in 协议们 if not 条["生效"]]
    print(f"== 命名豁免审计（协议注释·逐条）== 生效 {len(生效)} / 不生效 {len(不生效)}")
    for 条 in 生效:
        print(f"  [生效] {条['文件']}:{条['行']} {条['类别']} · {条['原因']}")
    for 条 in 不生效:
        因 = ("类别非法「" + 条["类别"] + "」"
              if 条["类别"] not in 协议类别枚举 else "缺原因")
        print(f"  [不生效] {条['文件']}:{条['行']} {因} · {条['原因']}")


def 打印报告(扫描果: dict, 判定果: dict, 缺基线: bool):
    print("== 命名门禁报告（#1716·标识符禁嵌任务号/日期）==")
    print(f"[判据] token 内连续 3~8 位数字段（豁免 {'/'.join(豁免数字)} 与 0 开头）")
    打印扫描摘要(扫描果)
    print("[top 文件]")
    计 = Counter(h["文件"] for h in 扫描果["命中"])
    for 文件, 次数 in 计.most_common(20):
        print(f"  {次数:6d}  {文件}")
    打印读取失败(扫描果)
    if 缺基线:
        print("[提示] 基线缺失——报告模式不判红（先 --freeze 生成基线）")
        return
    收缩 = len(判定果["消失"]) + len(判定果["下降"])
    print(f"[基线] 新增键 {len(判定果['新键'])} · 计数增长 {len(判定果['增长'])} · "
          f"可收缩 {收缩}")
    for 键 in 判定果["新键"][:20]:
        print(f"  + {键}")
    for 键, 旧, 新 in 判定果["增长"][:20]:
        print(f"  ~ {键} {旧}→{新}")


def 打印门禁(判定果: dict, 当前: Counter):
    if 判定果["红"]:
        if 判定果["新键"]:
            print(f"[命名门禁·红] 新增带号标识符 {len(判定果['新键'])} 键"
                  f"（{sum(当前[键] for 键 in 判定果['新键'])} 处）：")
            for 键 in 判定果["新键"][:30]:
                print(f"  + {键} ×{当前[键]}")
        if 判定果["增长"]:
            print(f"[命名门禁·红] 既有标识符出现次数增加 {len(判定果['增长'])} 键：")
            for 键, 旧, 新 in 判定果["增长"][:30]:
                print(f"  ~ {键} {旧}→{新}")
        print("[命名门禁] 修复=改名为表意名（名字说「它是什么」）；任务号只进注释与提交信息。"
              "协议豁免=命中行或上一行写「命名豁免<类别>: 原因」（逐条·不按类批量）。")
    else:
        print(f"[命名门禁] 绿：新增 0 · 计数增长 0 · 存量键 {len(当前)}"
              f"（命中 {sum(当前.values())} 处）")
    收缩 = len(判定果["消失"]) + len(判定果["下降"])
    if 收缩:
        print(f"[提示] 基线有 {收缩} 键可收缩（键消失/计数下降）——确认后 --freeze 收账")


# ============ 六、自测（正反两态·临时样例树·零触碰真实树） ============

样例合法 = """int 整32 = 1;
long i128 = 0;
unsigned T_整128 = 0;
int arg0 = 0;
const char* utf8 = "x";
int xmm0 = 0;
int 编号001 = 0;
"""

样例违规 = """int currentFnUnsafe474_ = 1;
int 负节919 = 2;
int 负节919 = 3;
int floatDigitsTrimZeros1161 = 4;
"""

样例剥除_cpp = """// badComment5678
/* badBlock5678 */
const char* s = "badString5678";
char c = 'a';
long v = 1'000'000;
"""

样例剥除_py = "\n".join([
    "# badComment5678",
    's = "badString5678"',
    "t = '''badTriple5678'''",
    'u = """badTriple5678"""',
]) + "\n"

样例协议 = "\n".join([
    "错误码1120 = 1  # 命名豁免第三方错误码: LNK1120",
    "",
    "坏名5678 = 1  # 命名豁免临时: 自测",
    "",
    "坏名5679 = 1  # 命名豁免第三方错误码:",
]) + "\n"

自测预期 = Counter({
    "src/违规.cpp|currentFnUnsafe474_": 1,
    "src/违规.cpp|负节919": 2,
    "src/违规.cpp|floatDigitsTrimZeros1161": 1,
    "src/生成摘除1674.cpp|生成摘除1674": 1,
    "scripts/协议.py|坏名5678": 1,
    "scripts/协议.py|坏名5679": 1,
})


def 自测建树(根: Path):
    """建临时样例树（覆盖扫描面/文件名/协议/剥除/归档）。"""
    文件们 = {
        "src/合法.cpp": 样例合法 + "int64_t 号001 = 0;\n",
        "src/i128.hpp": "int 好名 = 0;\n",
        "src/违规.cpp": 样例违规,
        "src/生成摘除1674.cpp": "int 好名 = 0;\n",
        "src/剥除.cpp": 样例剥除_cpp,
        "scripts/剥除.py": 样例剥除_py,
        "scripts/协议.py": 样例协议,
        "归档/样例.py": "bad1234 = 1\n",
        "项目记忆/样例.cpp": "bad1234 = 1;\n",
    }
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    def 重扫(根: Path) -> Counter:
        return Counter(建键(h) for h in 全库扫描(根)["命中"])

    with tempfile.TemporaryDirectory(prefix="cn_naming_") as td:
        根 = Path(td)
        自测建树(根)
        扫描果 = 全库扫描(根)
        当前 = Counter(建键(h) for h in 扫描果["命中"])
        断言(当前 == 自测预期, f"判据命中集合: {sorted(当前.items())}")
        断言(扫描果["统计"]["内容"] == 6, f"内容命中=6: {扫描果['统计']['内容']!r}")
        断言(扫描果["统计"]["文件名"] == 1, f"文件名命中=1: {扫描果['统计']['文件名']!r}")
        断言(not any(h["文件"].startswith("归档/") for h in 扫描果["命中"]),
              "归档目录内容跳过")
        断言(not any(h["文件"].startswith("项目记忆/") for h in 扫描果["命中"]),
              "项目记忆目录内容跳过")
        断言(路径豁免("tests/e2e/001/主.cn") and 路径豁免("plans/001.md"),
              "规范面路径豁免表生效")
        断言(not 路径豁免("src/foo.cpp"), "源码面不在豁免表")
        协议们 = 扫描果["协议"]
        断言(sum(1 for 条 in 协议们 if 条["生效"]) == 1, "协议注释生效 1 条")
        断言(sum(1 for 条 in 协议们 if not 条["生效"]) == 2,
              "协议注释不生效 2 条（类别非法/缺原因）")

        基 = 根 / "baseline.json"
        断言(写基线(基, 当前) == len(自测预期), "基线写键数")
        基线 = 读基线(基)
        断言(基线 == 当前, "基线读回与实测一致")
        果 = 判定(当前, 基线)
        断言(not 果["红"], f"存量全在基线=绿: {果!r}")

        违规 = 根 / "src" / "违规.cpp"
        违规.write_text(样例违规 + "int 负节919 = 5;\n", encoding="utf-8")
        果 = 判定(重扫(根), 基线)
        断言(bool(果["红"]) and bool(果["增长"]), f"反态①计数增长真拦: {果!r}")

        违规.write_text(样例违规, encoding="utf-8")
        新增 = 根 / "src" / "新增.cpp"
        新增.write_text("int 新增名1479 = 1;\n", encoding="utf-8")
        果 = 判定(重扫(根), 基线)
        断言(bool(果["红"]) and 果["新键"] == ["src/新增.cpp|新增名1479"],
              f"反态②新键真拦: {果!r}")
        新增.unlink()
        果 = 判定(重扫(根), 基线)
        断言(not 果["红"], f"新键删除后回绿: {果!r}")

        行们 = 样例违规.split("\n")
        违规.write_text("\n".join(行们[:2] + 行们[3:]), encoding="utf-8")
        果 = 判定(重扫(根), 基线)
        断言(not 果["红"] and 果["下降"] == [("src/违规.cpp|负节919", 2, 1)],
              f"计数下降=绿+可收缩: {果!r}")

        协议 = 根 / "scripts" / "协议.py"
        协议.write_text("错误码1120 = 1  # 命名豁免第三方错误码: LNK1120\n"
                     "\n"
                     "坏名5678 = 1  # 命名豁免用例编号: 自测\n"
                     "\n"
                     "坏名5679 = 1  # 命名豁免第三方错误码:\n", encoding="utf-8")
        果 = 判定(重扫(根), 基线)
        断言(果["消失"] == ["scripts/协议.py|坏名5678"],
              f"补齐协议注释后键消失=可收缩: {果!r}")

    if 失败:
        print("== 命名门禁自测：失败 ==")
        for 项 in 失败:
            print("  ✘ " + 项)
        print(f"[自测] {len(失败)} 项失败（通过 {通过[0]}）")
        return 1
    print(f"== 命名门禁自测全过（{通过[0]} 项断言·正反两态/豁免/剥除/基线齐备）==")
    return 0


# ============ 七、入口 ============

def main() -> int:
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except AttributeError:
        pass
    解析器 = argparse.ArgumentParser(
        description="标识符禁嵌任务号/日期机械门禁（#1716·冻结基线）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（不判红）")
    解析器.add_argument("--check", action="store_true", help="门禁模式（默认行为）")
    解析器.add_argument("--freeze", action="store_true", help="以当前实测重写冻结基线")
    解析器.add_argument("--approval", action="store_true",
                        help="列出全部协议注释豁免条目（审计面）")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    if not 根.is_dir():
        print(f"[命名门禁] 根目录不存在：{根}")
        return 2
    扫描果 = 全库扫描(根)
    当前 = Counter(建键(h) for h in 扫描果["命中"])

    if 参数.approval:
        打印豁免审计(扫描果["协议"])
        return 0
    if 参数.freeze:
        键数 = 写基线(Path(参数.baseline), 当前)
        print(f"[基线] 已写 {Path(参数.baseline).name}：键 {键数} · "
              f"命中 {sum(当前.values())} 处")
        打印扫描摘要(扫描果)
        return 0

    基路径 = Path(参数.baseline)
    if 参数.report:
        基线 = 读基线(基路径) if 基路径.exists() else Counter()
        打印报告(扫描果, 判定(当前, 基线), 缺基线=not 基路径.exists())
        return 0

    基线 = 读基线(基路径)
    if 基线 is None:
        print(f"[命名门禁·红] 基线缺失：{基路径.name}——先跑 "
              f"python3 scripts/check_naming.py --freeze 生成")
        打印扫描摘要(扫描果)
        打印读取失败(扫描果)
        return 1
    判定果 = 判定(当前, 基线)
    打印门禁(判定果, 当前)
    打印读取失败(扫描果)
    return 1 if (判定果["红"] or 扫描果["统计"]["读取失败"]) else 0


if __name__ == "__main__":
    sys.exit(main())
