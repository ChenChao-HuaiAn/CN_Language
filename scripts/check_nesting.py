#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_nesting.py —— 分支嵌套深度 ≤3 机械门禁（#2089·001 §14.7 要点③）。

权威条文=001 §14.7「分支结构总法」要点③（2026-11-05 用户裁决「嵌套层数最大 3 层」
·逐字在案）：if/else-if/for/while/switch/选择 计层；纯块 {} 不计；lambda/函数指针
函数体重置计数；早返回不计且鼓励（早返回使后续代码回到浅层·守卫式天然低深）；
**新增硬限**+存量冻结基线只减不增（与函数≤100 行、文件 900/1000 两线联立）。
宿主 C++ 与 v2 CN 双侧同规（要点⑤）。

计层判据（机械·定律甲）：
  计层词=if/如果/for/循环/while/当/switch/选择（控制词后首个 `{` 记「控制块」·
    计层）；else/否则/do/try/catch 后首个 `{` 记「纯块」（不计层——else-if 链与
    裸 else 块均与所属 if 同层·显式枚举链不因链长叠层·鼓励方向）；其余 `{` 均
    纯块（函数体/类体/命名空间/case 复合块/裸作用域块）。
  深度=当前位置到最近「重置点」之间的控制块层数；重置点=lambda 体（判据：`{`
    前一 token=`)` 且向前配对 `(` 的前一 token=`]`——CN lambda `[捕获](参数){体}`
    与 C++ 同形）。函数体本身=扫描单元起点（天然 0 基线）。
  早返回不计：return/continue/break/中断/继续 为普通 token·不推栈——守卫式
    `if(!x){return;}` 恒为深 1·后续线性代码深 0（鼓励方向自动成立）。

扫描单元：函数体（函数头识别复用 950 扫描核 收函数头们·近似；头行起首个 `{`
  配对到 `}`）。同文件同函数头锚多报=计数键。最大深 > 上限（3）=候选红点。

豁免通道（#1238 批准制·882 登记制复用）：协议注释「// 静默豁免<类别>: 原因」
  （锚定行=函数头行同行或上一行·锚相容）+miss_registry.json 登记条（批准=已批/
  存量过渡）。类别沿用现有枚举（业务值/闭集穷举/待裁决等·不扩枚举零波及）。

冻结基线（只减不增·file_length/dispatch_freeze 先例）：
  scripts/nesting_freeze_baseline.json，键=「文件::归一锚文本（函数头行）」·
  同键重复=计数。基线内=存量放行；新增/计数增加=红（除非协议注释+登记条且批准=
  已批/存量过渡）；键消失=消减提示（自动棘轮）。基线记录「规则版本+规则指纹+
  生成sha」——指纹变化打印黄字提示，复核消减后 --update-baseline 收账。

诚实边界（不假装全覆盖）：
  ① 函数头=近似识别（950 扫描核同源）——漏识别函数头则该函数不扫（漏报方向·
    存量基线兜底·不静默代答）；
  ② 单语句体（无块形 `if (x) y;`）不判（只判块形·与面①只判块形同哲学）；
  ③ 预处理宏展开生成的控制结构不可见（掩码不过宏展开）；
  ④ 嵌套类/嵌套函数与外层函数重叠扫描时按外层函数计（多收取向·收严）；
  ⑤ 本工具只读不写（--update-baseline 除外·禁手改基线）；扫描面=src/ +
    CN语言编译器v2/。

用法：
  python3 scripts/check_nesting.py                    # 门禁（rc=1=有红·L1 静态段）
  python3 scripts/check_nesting.py --report           # 报告模式（全列·rc=0）
  python3 scripts/check_nesting.py --json             # 机读输出（stdout）
  python3 scripts/check_nesting.py --update-baseline  # 以当前实测重写冻结基线
  python3 scripts/check_nesting.py --selftest         # 自测（临时样例树·正反两态）

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
from collections import Counter
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
仓库根 = 脚本目录.parent

# 复用既有扫描核（纪律二·不重复实现；dispatch/miss/sentinel 同先例）：
# 950 函数头近似识别+拆行+登记表读取；882 掩码/配对括号/行号/协议注释/登记条
# 命中/锚相容/豁免类别枚举；648 词法 token 化（注释跳过·字符串整体成 token）；
# 1959 三红判定件（同行协议/批准问题·点键同构复用）。
import check_miss_defaults as 默认值扫描          # noqa: E402
import check_miss_gate as 静默门禁              # noqa: E402
import check_return_paths as 返回路径             # noqa: E402
import check_dispatch_gate as 分派门禁           # noqa: E402（三红判定件复用）

登记表文件 = 仓库根 / "scripts" / "miss_registry.json"
基线文件 = 仓库根 / "scripts" / "nesting_freeze_baseline.json"

扫描后缀 = {".cpp", ".hpp", ".cn"}
扫描前缀 = ("src/", "CN语言编译器v2/")

规则版本 = "2089-v1"
嵌套上限 = 3          # §14.7 要点③：嵌套层数最大 3 层（新增硬限）
条件跨行上限 = 400     # 函数体搜索窗口（防病态文件）

# 计层控制词（后首个 `{`=控制块·计层）
计层词 = {"if", "如果", "for", "循环", "while", "当", "switch", "选择"}
# 不计层分支词（后首个 `{`=纯块·else-if 链同层不叠·case 标签不设待控制）
不计层词 = {"else", "否则", "do", "try", "catch"}

# 规则指纹（基线记录·可复现审计·1640 先例）
_指纹件 = [规则版本, str(嵌套上限),
         ",".join(sorted(计层词)), ",".join(sorted(不计层词))]
规则指纹 = hashlib.sha256(
    "|".join(str(x) for x in _指纹件).encode("utf-8")).hexdigest()[:16]


# ============ 一、扫描核（token 栈式数层） ============

def 当前深(栈: list) -> int:
    """当前位置嵌套深：栈顶向栈底数控制块·遇最近重置点即停。"""
    深 = 0
    for x in reversed(栈):
        if x == "重":
            break
        if x == "控":
            深 += 1
    return 深


def 判块类型(toks: list, i: int, 待控制) -> str:
    """toks[i]=="{"：返回 "控"（计层）|"纯"（不计）|"重"（lambda 重置）。

    lambda 判据：{ 前一 token=) 且向前配对 ( 的前一 token=]（CN/C++ lambda
    同形 `[捕获](参数){体}`·parser_expr 探测判据同源）。"""
    if 待控制 == "计":
        return "控"
    if 待控制 is not None or i <= 0 or toks[i - 1][0] != ")":
        return "纯"
    深 = 0
    j = i - 1
    while j >= 0:
        t = toks[j][0]
        if t == ")":
            深 += 1
        elif t == "(":
            深 -= 1
            if 深 == 0:
                if j > 0 and toks[j - 1][0] == "]":
                    return "重"
                break
        j -= 1
    return "纯"


def 函数最大嵌套深(体文本: str) -> int:
    """函数体文本→最大嵌套深（token 栈式扫描·见模块头判据）。"""
    toks = 返回路径.词法(体文本)
    栈: list = []
    待控制 = None
    括号深 = 0
    最大 = 0
    for i, (tok, _行) in enumerate(toks):
        if tok in ("(", "["):
            括号深 += 1
            continue
        if tok in (")", "]"):
            括号深 = max(0, 括号深 - 1)
            continue
        if 括号深 > 0:
            continue                    # 条件/参数表内不参与（if (a[0]) 误配防）
        if tok in 计层词:
            待控制 = "计"
            continue
        if tok in 不计层词:
            待控制 = "纯"
            continue
        if tok == "{":
            栈.append(判块类型(toks, i, 待控制))
            待控制 = None
            深 = 当前深(栈)
            if 深 > 最大:
                最大 = 深
            continue
        if tok == "}":
            if 栈:
                栈.pop()
            待控制 = None
            continue
        if 待控制 is not None:
            待控制 = None               # 单语句体（无块形）→ 不判（诚实边界②）
    return 最大


# ============ 二、函数体定位与单文件扫描 ============

def 找函数体区间(掩: str, 头偏移: int):
    """函数头行偏移→(体 `{` 偏移, 配对 `}` 偏移)；找不到=(-1, -1)。"""
    开 = 掩.find("{", 头偏移)
    if 开 < 0:
        return -1, -1
    闭 = 静默门禁.配对括号(掩, 开)
    if 闭 is None:
        return -1, -1
    return 开, 闭


def 扫文件(相对: str, 文本: str, 命中: list) -> list:
    """单文件扫描：函数头识别→函数体定位→最大嵌套深>上限=候选红点。

    红点锚=函数头行代码文本（基线键=文件::归一锚·同函数头锚多报=计数）。
    返回协议注释点列表（豁免判定用）。"""
    掩 = 静默门禁.掩码(文本)
    行首表 = 静默门禁.建行首表(文本)
    行们 = 默认值扫描.拆行(文本)
    头们 = 默认值扫描.收函数头们(相对, 行们)
    新点 = []
    for 头行号, 名, _类型 in 头们:
        if not 0 < 头行号 <= len(行首表) or not 0 < 头行号 <= len(行们):
            continue
        开, 闭 = 找函数体区间(掩, 行首表[头行号 - 1])
        if 开 < 0:
            continue                    # 函数体定位失败→不判（诚实边界①漏报方向）
        深 = 函数最大嵌套深(文本[开 + 1:闭])
        if 深 <= 嵌套上限:
            continue
        锚 = 行们[头行号 - 1][1].strip()
        if 锚:
            新点.append({"文件": 相对, "行": 头行号, "锚": 锚, "深": 深, "名": 名})
    # 同头行去重（跨行头拼接窗口极端叠加）
    去重 = {}
    for 点 in 新点:
        去重.setdefault((点["行"], 静默门禁.归一(点["锚"])), 点)
    命中.extend(去重.values())
    return 静默门禁.扫协议注释(相对, 行们)


def 全库扫描(根: Path):
    """扫 src/ 与 CN语言编译器v2/（882/1959 同扫描面口径）。"""
    命中: list = []
    协议点们: list = []
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
            协议点们.extend(扫文件(相对, 文本, 命中))
    return {"候选": 命中, "协议点": 协议点们, "统计": 统计}


# ============ 三、基线读写（只减不增棘轮·dispatch/sentinel 同构） ============

def 点键(点: dict) -> str:
    return 点["文件"] + "::" + 静默门禁.归一(点["锚"])


def 读基线(路径: Path):
    """读冻结基线；缺失=异常（门禁拒跑·防绕过）；损坏=异常。"""
    if not 路径.exists():
        raise SystemExit(f"[门禁] 基线缺失：{路径.name}——先跑 "
                         f"python3 scripts/check_nesting.py --update-baseline 生成")
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError, UnicodeDecodeError) as e:
        raise SystemExit(f"[门禁] 基线损坏（{e.__class__.__name__}）：{路径.name}")
    return Counter(数据.get("超限", [])), 数据.get("规则指纹", "")


def 写基线(路径: Path, 命中: list) -> int:
    try:
        sha = subprocess.run(["git", "rev-parse", "HEAD"], cwd=路径.parent.parent,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    except Exception:
        sha = ""
    数据 = {
        "版本": 1,
        "工具": "scripts/check_nesting.py",
        "任务": "2089",
        "规则版本": 规则版本,
        "规则指纹": 规则指纹,
        "说明": ("#2089 分支嵌套深度>3 冻结基线（001 §14.7 要点③·只减不增）："
               "键=「文件::归一函数头锚」，同键重复=计数。基线内=存量放行；新增/"
               "计数增加=红（除非协议注释+miss_registry.json 登记条且批准=已批/"
               "存量过渡）；键消失=消减提示（自动棘轮·改写守卫式即降）。手改基线="
               "绕过门禁行为·禁（收账=--update-baseline·仅确认清偿后）。挂载轮基线"
               "随头生成（388 篇教训）。"),
        "生成sha": sha,
        "超限": sorted(点键(p) for p in 命中),
    }
    路径.write_text(json.dumps(数据, ensure_ascii=False, indent=1), encoding="utf-8")
    return len(数据["超限"])


# ============ 四、判定引擎（三红·1959 同构·复用其批准件） ============

def 判定(命中: list, 条目们: list, 基线: Counter, 协议点们: list):
    """三红：①新增/计数增加无豁免 ②漏登记 ③类别非法/登记无批准/已毙未清偿。"""
    红们, 豁免们 = [], []
    现计: dict = {}
    for 点 in 命中:
        现计.setdefault(点键(点), []).append(点)
    新增 = []
    for 键 in sorted(现计):
        b = 基线.get(键, 0)
        序 = sorted(现计[键], key=lambda x: (x["行"], x["锚"]))
        if len(序) > b:
            新增.extend(序[b:])          # 前 b 个视作基线存量·超出的才是新增
    减 = []
    for 键, b in sorted(基线.items()):
        c = len(现计.get(键, []))
        if c < b:
            减.extend([键] * (b - c))
    红数 = 放行 = 0
    for 点 in 新增:
        协议 = 分派门禁.同行协议(协议点们, 点["行"], 点["锚"])
        if 协议 is not None:
            if 协议["类别"] not in 静默门禁.豁免类别枚举:
                红们.append({"种类": "协议注释类别非法", "文件": 点["文件"],
                           "行": 点["行"], "锚": 点["锚"][:120],
                           "说明": f"类别「{协议['类别']}」不在枚举 "
                                 f"{sorted(静默门禁.豁免类别枚举)}"})
                红数 += 1
                continue
            条 = 静默门禁.条命中(点["文件"], 点["锚"], 条目们)
            问题 = 分派门禁.批准问题(条)
            if 问题 is not None:
                红们.append({"种类": 问题[0], "文件": 点["文件"], "行": 点["行"],
                           "锚": 点["锚"][:120], "说明": 问题[1]})
                红数 += 1
                continue
            放行 += 1
            豁免们.append({"文件": 点["文件"], "行": 点["行"],
                         "锚": 点["锚"][:100], "豁免": "协议注释+登记条目"})
            continue
        条 = 静默门禁.条命中(点["文件"], 点["锚"], 条目们)
        if 条 is not None:
            红们.append({"种类": "新增位置缺协议注释", "文件": 点["文件"],
                       "行": 点["行"], "锚": 点["锚"][:120],
                       "说明": "登记条目在但代码无协议注释——补"
                             "「// 静默豁免<类别>: 原因」（函数头行同行或上一行）"})
        else:
            红们.append({"种类": "新增嵌套超限", "文件": 点["文件"], "行": 点["行"],
                       "锚": 点["锚"][:120], "深": 点["深"],
                       "说明": f"函数嵌套深 {点['深']} > {嵌套上限}（001 §14.7 要点③）——"
                             f"改写守卫式早返回（A/B/C 三式），或走协议注释+登记条通道"})
        红数 += 1
    统 = {"现测": len(命中), "基线": sum(基线.values()), "新增": len(新增),
         "豁免放行": 放行, "红": 红数, "消减": len(减)}
    return {"红": 红们, "豁免": 豁免们, "消减": 减, "统": 统,
            "条目数": len(条目们), "协议点数": len(协议点们)}



# ============ 五、输出 ============

def 打印红们(红们: list):
    排序 = sorted(红们, key=lambda r: (r["种类"], r["文件"], r["行"]))
    print(f"\n【红清单】（{len(排序)} 条·L1 硬拦）")
    for r in 排序:
        位 = f"{r['文件']}:{r['行']}" if r.get("行") else r["文件"]
        深 = f" 深{r['深']}" if r.get("深") else ""
        print(f"  [×] ({r['种类']}) {位}{深}")
        if r.get("锚"):
            print(f"      锚：{r['锚'][:110]}")
        print(f"      处置：{r['说明']}")


def 打印报告(命中: list, 判定果: dict, 指纹: str, 详列: bool, 统计: dict):
    print("== 分支嵌套深度门禁（check_nesting.py·#2089·001 §14.7 要点③）==")
    print(f"扫描面：src/ + CN语言编译器v2/（文件 {统计['文件']}·读取失败 "
          f"{统计['读取失败']}）；上限 {嵌套上限} 层；规则 {规则版本} 指纹 {规则指纹}；"
          f"登记表条目 {判定果['条目数']} 条；协议注释点 {判定果['协议点数']} 处")
    u = 判定果["统"]
    print(f"【统计】现测 {u['现测']} | 基线 {u['基线']} | 新增 {u['新增']} | "
          f"豁免放行 {u['豁免放行']} | 红 {u['红']} | 消减 {u['消减']}")
    if 指纹 and 指纹 != 规则指纹:
        print(f"[提示] 基线规则指纹 {指纹} ≠ 当前 {规则指纹}——判据已变更，"
              f"复核消减面后 --update-baseline 收账")
    if 判定果["消减"]:
        print(f"  [↓] 基线超限位置消失 {len(判定果['消减'])} 处（自动消减·确认无误报后 "
              f"--update-baseline 收基线）：{判定果['消减'][:3]}…")
    if 判定果["豁免"]:
        print(f"\n【豁免放行】（{len(判定果['豁免'])} 条·协议注释+登记条目）")
        for h in 判定果["豁免"][:10]:
            print(f"  [✓] {h['文件']}:{h['行']} {h['锚'][:80]}")
        if len(判定果["豁免"]) > 10:
            print(f"  …（其余 {len(判定果['豁免']) - 10} 条）")
    if 详列:
        print("\n【在测超限位置明细】（--report·全列）")
        for 点 in sorted(命中, key=lambda x: (x["文件"], x["行"])):
            print(f"  {点['文件']}:{点['行']}  深{点['深']}  {点['锚'][:100]}")
    if 判定果["红"]:
        打印红们(判定果["红"])
        print(f"\n[门禁·红] {len(判定果['红'])} 条——L1 硬拦；唯一合法通道="
              f"协议注释+miss_registry.json 登记条（批准=已批/存量过渡），或原地"
              f"改写守卫式（001 §14.7 A/B/C 三式）。")
    else:
        print("\n[门禁·绿] 嵌套零红（存量冻结只减不增·新增硬限）")


def 转json(判定果: dict, 命中: list, 统计: dict) -> str:
    输出 = {"工具": "check_nesting.py", "任务": "2089",
          "规则版本": 规则版本, "规则指纹": 规则指纹, "上限": 嵌套上限,
          "统计": 统计, "面统": 判定果["统"],
          "条目数": 判定果["条目数"], "协议点数": 判定果["协议点数"],
          "豁免": 判定果["豁免"],
          "消减": 判定果["消减"][:50],
          "红": 判定果["红"],
          "在测": [{"文件": p["文件"], "行": p["行"], "深": p["深"],
                  "锚": p["锚"][:100]} for p in 命中[:200]]}
    return json.dumps(输出, ensure_ascii=False, indent=1)


# ============ 六、自测（正反两态+形态判据+基线演化+幂等·零触碰真实树） ============

好样例 = """// 守卫式（早返回·深 1）与 else-if 链（同层不叠）+纯块不计
int 守卫样(int k, int* out) {
    if (out == nullptr) return -1;
    if (k < 0) { return -1; }
    if (k > 100) { return -1; }
    *out = k * 2;
    return 0;
}
int 链样(int x) {
    if (x > 0) { return 1; }
    else if (x < 0) { return 2; }
    else if (x == 0) { return 0; }
    else { return -1; }
}
int 裸块样(int k) {
    { int t = k + 1; k = t; }
    return k;
}
int lambda样(int k) {
    auto fn = [](int x) {
        if (x > 0) {
            if (x > 10) {
                if (x > 100) {
                    return x;
                }
            }
        }
        return 0;
    };
    return fn(k) + k;                  // lambda 体内深 3（重置·不叠加外层）
}
"""

坏样例 = """// 四层嵌套=红（>3）
int 嵌套样(int a, int b, int c, int d) {
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                if (d > 0) {
                    return a + b + c + d;      // 深 4=红
                }
            }
        }
    }
    return 0;
}
"""

样例v2 = """// CN 侧：循环/选择 同规计层
函数 v2样(甲: 整32) -> 整32 {
    如果 (甲 > 0) {
        循环 (i: 整32 = 0; i < 3; i = i + 1) {
            当 (甲 > 100) {
                甲 = 甲 - 100;
            }
        }
    }
    返回 甲;
}
函数 v2好样(甲: 整32) -> 整32 {
    选择 (甲) {
        情况 1: { 返回 1; }
        情况 2: { 返回 2; }
    }
    返回 0;
}
"""


def 自测写表(路径: Path, 条目们: list):
    路径.write_text(json.dumps({"版本": 1, "条目": 条目们}, ensure_ascii=False),
                 encoding="utf-8")


def 自测建场景(保活: list, 文件们: dict):
    """独立临时树 + 以当前实测生成基线。返回 (根, 表, 基)。"""
    td = tempfile.TemporaryDirectory(prefix="cn2089_")
    保活.append(td)
    根 = Path(td.name)
    for 相对, 内容 in 文件们.items():
        p = 根 / 相对
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(内容, encoding="utf-8")
    表 = 根 / "registry.json"
    自测写表(表, [])
    基 = 根 / "baseline.json"
    写基线(基, 全库扫描(根)["候选"])
    return 根, 表, 基


def 自测跑(根: Path, 表: Path, 基: Path):
    """临时树门禁调用：返回 (rc, 判定果, 扫描果)。"""
    扫描果 = 全库扫描(根)
    条目们, _态 = 默认值扫描.读登记表(表)
    基线, _指纹 = 读基线(基)
    果 = 判定(扫描果["候选"], 条目们, 基线, 扫描果.get("协议点", []))
    return (1 if 果["红"] else 0), 果, 扫描果


def 红种们(果) -> set:
    return {r["种类"] for r in 果["红"]}


def 自测场景一(断言, 保活):
    """场景一：形态判据（好样本零命中）+超限真拦+基线演化。"""
    根, 表, 基 = 自测建场景(保活, {
        "src/好样.cpp": 好样例,
        "src/坏样.cpp": 坏样例,
        "CN语言编译器v2/v2样.cn": 样例v2})
    扫描果 = 全库扫描(根)
    锚深 = {(p["文件"], p["行"]): p["深"] for p in 扫描果["候选"]}
    断言(all(深 > 嵌套上限 for 深 in 锚深.values()),
        f"候选全为真超限（守卫/链/裸块/lambda重置/v2 深3限内不误报）: {锚深!r}")
    断言(len(扫描果["候选"]) == 1 and 锚深.get(("src/坏样.cpp", 2)) == 4,
        f"超限候选=1（宿主四层嵌套）: {锚深!r}")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"基线生成后存量放行=绿: 红={果['红']!r}")
    # 反态①：新增超限函数=红
    坏 = 根 / "src" / "坏样.cpp"
    坏.write_text(坏样例 +
                "int 新坏(int a) {\n    if (a > 0) {\n        if (a > 1) {\n"
                "            if (a > 2) {\n                if (a > 3) {\n"
                "                    return a;\n                }\n            }\n"
                "        }\n    }\n    return 0;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "新增嵌套超限" in 红种们(果), f"反态①新增四层嵌套真拦: {红种们(果)}")
    # 正态①：协议注释+登记条（存量过渡）=放行
    自测写表(表, [{"文件": "src/坏样.cpp", "函数": "新坏",
                "锚文本": "int 新坏(int a) {", "来源": "2089selftest",
                "类别": "待裁决", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest", "批准": "存量过渡"}])
    坏.write_text(坏样例 +
                "// 静默豁免待裁决: 自测登记（新坏·嵌套暂超限待改写）\n"
                "int 新坏(int a) {\n    if (a > 0) {\n        if (a > 1) {\n"
                "            if (a > 2) {\n                if (a > 3) {\n"
                "                    return a;\n                }\n            }\n"
                "        }\n    }\n    return 0;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0, f"正态①协议注释+登记条=放行: 红={果['红']!r}")
    # 反态②：注释有而表空=漏登记
    自测写表(表, [])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 1 and "漏登记" in 红种们(果), f"反态②漏登记: {红种们(果)}")
    # 反态③：登记条无批准=登记无批准（1238）
    自测写表(表, [{"文件": "src/坏样.cpp", "函数": "新坏",
                "锚文本": "int 新坏(int a) {", "来源": "2089selftest",
                "类别": "待裁决", "静默风险": "低", "原因": "自测登记",
                "登记人": "selftest"}])
    rc, 果, _s = 自测跑(根, 表, 基)
    断言("登记无批准" in 红种们(果), f"反态③登记无批准: {红种们(果)}")
    # 正态②：守卫化改写=位置消失自动消减（基线内嵌套样键改写后消失）
    自测写表(表, [])
    坏.write_text("int 嵌套样(int a, int b, int c, int d) {\n"
                "    if (a <= 0) return 0;\n    if (b <= 0) return 0;\n"
                "    if (c <= 0) return 0;\n    if (d <= 0) return 0;\n"
                "    return a + b + c + d;\n}\n"
                "int 新坏(int a) {\n    if (a <= 3) return 0;\n"
                "    if (a <= 2) return 0;\n    if (a <= 1) return 0;\n"
                "    if (a <= 0) return 0;\n    return a;\n}\n", encoding="utf-8")
    rc, 果, _s = 自测跑(根, 表, 基)
    断言(rc == 0 and len(果["消减"]) == 1,
        f"正态②守卫化改写=基线键消失自动消减: {果['消减']!r}")


def 自测场景二(断言, 保活):
    """场景二：lambda 重置与外层叠加边界+单语句体不判+基线缺失拒跑+幂等。"""
    # lambda 体内超限（重置后仍 >3）=红；重置后 ≤3=绿
    根2, 表2, 基2 = 自测建场景(保活, {
        "src/lambda超限.cpp":
        "int 持有(void) { return 0; }\n"
        "int 外样(int k) {\n"
        "    auto fn = [k](int x) {\n"
        "        if (x > 0) {\n            if (x > 1) {\n"
        "                if (x > 2) {\n                    if (x > 3) {\n"
        "                        return x;          // lambda 体内深 4=红\n"
        "                    }\n                }\n            }\n        }\n"
        "        return 0;\n"
        "    };\n"
        "    return fn(k);\n"
        "}\n"})
    扫描果2 = 全库扫描(根2)
    断言(len(扫描果2["候选"]) == 1 and 扫描果2["候选"][0]["深"] == 4,
        f"lambda 体内深 4 真报（重置后计数不叠外层但上限同规）: {扫描果2['候选']!r}")
    # 单语句体链不判（诚实边界②）
    根3, _t3, _b3 = 自测建场景(保活, {
        "src/单语句.cpp":
        "int 链式单语句(int a) {\n"
        "    if (a > 0) if (a > 1) if (a > 2) if (a > 3) return 1;\n"
        "    return 0;\n"
        "}\n"})
    扫描果3 = 全库扫描(根3)
    断言(len(扫描果3["候选"]) == 0, f"单语句体链不判（诚实边界②）: {扫描果3['候选']!r}")
    # 基线缺失=拒跑（防绕过）
    try:
        读基线(根3 / "不存在.json")
        断言(False, "基线缺失应拒跑")
    except SystemExit:
        断言(True, "基线缺失=拒跑（防绕过）")
    # 幂等：同一棵树重扫全等
    根4, _t4, _b4 = 自测建场景(保活, {
        "src/坏样.cpp": 坏样例, "CN语言编译器v2/v2样.cn": 样例v2})
    扫1 = 全库扫描(根4)
    扫2 = 全库扫描(根4)
    断言(json.dumps(扫1["候选"], ensure_ascii=False, sort_keys=True) ==
        json.dumps(扫2["候选"], ensure_ascii=False, sort_keys=True), "幂等：两遍全等")


def 自测() -> int:
    通过 = [0]
    失败 = []

    def 断言(条件, 说明):
        if 条件:
            通过[0] += 1
        else:
            失败.append(说明)

    保活 = []                            # TemporaryDirectory 保活（防提前清理）
    for 场景 in (自测场景一, 自测场景二):
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
    print(f"== 门禁自测全过（{通过[0]} 项断言·正反两态/形态判据/lambda重置/三红各一例/消减棘轮/幂等）==")
    return 0


# ============ 七、入口 ============

def main() -> int:
    解析器 = argparse.ArgumentParser(
        description="分支嵌套深度≤3机械门禁（#2089·001 §14.7 要点③+冻结基线）")
    解析器.add_argument("--root", default=str(仓库根), help="仓库根目录")
    解析器.add_argument("--registry", default=str(登记表文件), help="登记表路径")
    解析器.add_argument("--baseline", default=str(基线文件), help="冻结基线路径")
    解析器.add_argument("--report", action="store_true", help="报告模式（全列·rc=0 不拦）")
    解析器.add_argument("--json", action="store_true", help="机读输出（stdout）")
    解析器.add_argument("--update-baseline", action="store_true",
                      help="以当前实测重写冻结基线")
    解析器.add_argument("--selftest", action="store_true", help="自测（临时样例树）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return 自测()

    根 = Path(参数.root).resolve()
    扫描果 = 全库扫描(根)
    if 参数.update_baseline:
        统计 = 写基线(Path(参数.baseline), 扫描果["候选"])
        print(f"[基线] 已重写 {Path(参数.baseline).name}：超限存量 {统计} 处冻结")
        return 0
    条目们, 表态 = 默认值扫描.读登记表(Path(参数.registry))
    if 表态 != "就绪":
        print(f"[门禁·红] 登记表状态异常：{表态}（{参数.registry}）")
        return 1
    基线, 指纹 = 读基线(Path(参数.baseline))
    判定果 = 判定(扫描果["候选"], 条目们, 基线, 扫描果.get("协议点", []))

    if 参数.json:
        print(转json(判定果, 扫描果["候选"], 扫描果["统计"]))
    else:
        打印报告(扫描果["候选"], 判定果, 指纹, 详列=参数.report, 统计=扫描果["统计"])
    if 参数.report:
        return 0
    return 1 if 判定果["红"] else 0


if __name__ == "__main__":
    sys.exit(main())
