#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""case_activity.py —— 题库活性指标（837）：E2E 管道实际消费面 × 看板登记面 机械比对。

病根（801·死配置 37 例）：569-a 波2 给负测目录写入了 双编译对照.txt（含
「预期编译失败=是+诊断锚」，=宿主+v2 双侧拒绝性锁定），但漏删旧标记
期望编译失败.txt——run_e2e 的通道分派是**首命中即 return** 的优先级链，
负 e2e 分支（期望编译失败.txt）先命中后 双编译对照.txt 从未被读取：37 条负向
规则的 v2 侧拒绝性零覆盖存活半个月（743 全量普查才发现）。本工具把「这类死配置」
变成每周机械比对（头条 53 篇·防再犯面）：

  消费面（管道实际会消费的用例/通道）——按 run_e2e.执行单个用例 的分派优先级
    静态复刻：平台跳过 → 负 e2e → check 负 → v2 锚定链 → v2 闭环 → 双编译对照
    → 普通编排；逐例记录 通道 / 被遮蔽的选择器 / 前置缺失。
  登记面（看板台账唯一权威）——①覆盖矩阵 /api/coverage（单元三态+豁免行·
    check_spec_coverage 同源拉取与解析）；②用例号台账 /api/case_numbers
    （case_ledger 同源·主题名归一匹配）。

比对输出（黄灯报告·默认 rc=0）：
  [死配置] 目录同时存在多个通道选择器文件、管道只读优先级最高者——其余
        「存在但永不消费」（801 形态）；按被遮蔽文件内容分档说明（双编译
        负测/双编译正测/v2 闭环/check 负/锚定链）。注：check_asm_width 等
        工具会**读**该配置做排除判定，但不执行该通道——执行消费方唯 run_e2e。
        906 起 run_e2e 对并存选择器**直接判失败**（拒绝并存的通道选择器·
        拒绝静默裁决）——本检测为静态周检面（无需执行即可点名·含历史分支；
        两者互补：runner 拦活配置，本工具点名单）。
  [跑不到] 选中的通道前置缺失（普通编排缺 .cn 或同名 .expected；负测缺源；
        check 负被检文件不存在；v2 通道配置源/供给/期望缺失或配置键非法）
        ——登记了但管道执行不了（必红空转或直接无法编排）。
  [登记未消费] 台账登记名在本树无对应目录：覆盖矩阵/豁免=名精确匹配（与
        check_spec_coverage --ci 同口径）；号台账=主题名归一匹配（case_ledger
        名匹配·含 v2专用: 前缀归一）。口径：服务端台账=全局面、本树=单分支面，
        他分支在飞用例常态出现在本清单（黄灯两义·与 check_case_numbers 613
        陈旧检测合读）。
  [消费未登记] 管道会消费但三面登记皆无（信息面·孤儿用例；硬判据在
        check_spec_coverage --ci 与 case_ledger 对照本地，此处仅计数）。

模型防漂移（miss 不可静默·648 同规）：本工具对 run_e2e.py 源码做**分派序审计**
——静态提取 执行单个用例 函数体内 `if (用例目录 / "X.txt").exists():` 直连分派与
配置路径别名（v2配置路径/双编译配置路径）分派的实际顺序，与本地模型表比对；
不一致=工具与 runner 漂移 → 报告 + rc=2（宁停不装）。平台跳过/重负载名单直接
导入 run_e2e 模块属性（单一权威·禁双份定义），属性缺失同样按漂移处置。

用法：
  python3 scripts/case_activity.py             # 全量比对（黄灯·rc=0）
  python3 scripts/case_activity.py --strict    # 死配置/跑不到 → rc=1（周检挂载点）
  python3 scripts/case_activity.py --json      # 机读报告（周检台账/看板消费）
  python3 scripts/case_activity.py --limit 50  # 清单截断（默认 20·0=全列）
  python3 scripts/case_activity.py --selftest  # 自测（临时样例树正反两态·零触碰真实树）

周检跑法（每周一次·建议挂轮值巡检）：`python3 scripts/case_activity.py --strict`
——红=须处置死配置/跑不到（本清单点名项按「提及即立项」落看板台账后处置）；
[登记未消费] 黄灯与 check_case_numbers 陈旧检测合读（他分支在飞两义）。

退出码：0=报告完成（--strict 且死配置/跑不到>0 → 1）；2=用法错误或模型漂移。
"""
from __future__ import annotations

import argparse
import io
import json
import os
import re
import sys
import tempfile
from pathlib import Path

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
E2E = 仓库根 / "tests" / "e2e"
sys.path.insert(0, str(脚本目录))                  # case_ledger/check_spec_coverage 复用
sys.path.insert(0, str(仓库根 / "tests" / "e2e"))  # run_e2e（权威配置解析/名单）

import case_ledger          # noqa: E402  号台账取回 + 主题名/名匹配（577 单一实现）
import check_spec_coverage  # noqa: E402  覆盖矩阵拉取/解析（--ci 同源）
import run_e2e              # noqa: E402  通道配置解析 + 平台跳过/重负载名单

# ============ 通道分派模型（与 run_e2e.执行单个用例 严格同序）============
# 首命中即 return——列表顺序=优先级；命中者的**后续选择器文件永不读取**
# （被遮蔽=死配置候选；801 根因）。模型与 runner 的一致性由 审计分派序() 机械保证，
# 不靠人工记忆（改 runner 分派=必须同步本表，否则 rc=2 停装）。
选择器序 = (
    ("期望编译失败.txt", "负 e2e"),
    ("期望check失败.txt", "check 负"),
    ("v2锚定链.txt", "v2 锚定链"),
    ("v2闭环.txt", "v2 闭环"),
    ("双编译对照.txt", "双编译对照"),
)
选择器通道 = dict(选择器序)
普通通道 = "普通编排"

# 分派序审计正则：仅认「行尾收口」的独立分派 if（`if (用例目录 / "X").exists():`）
# ——排除 569-a 容错块 `if A.exists() or B.exists():` 之类非分派读取（假漂移源）。
正则_直连分派 = re.compile(r'^\s*if \(用例目录 / "([^"]+\.txt)"\)\.exists\(\):\s*$')
正则_变量分派 = re.compile(r"^\s*if (\w+)\.exists\(\):\s*$")
正则_路径别名 = re.compile(r'^\s*(\w+)\s*=\s*用例目录 / "([^"]+\.txt)"\s*$')


def 有效行(路径: Path) -> list:
    """去空行/# 注释行（与 run_e2e 用例级控制文件同口径）。"""
    try:
        文本 = 路径.read_text(encoding="utf-8-sig")
    except OSError:
        return []
    return [行.strip() for 行 in 文本.splitlines()
            if 行.strip() and not 行.strip().startswith("#")]


def 查源(用例目录: Path):
    """复刻 run_e2e.查找源文件：单 .cn 取之；多 .cn 须含入口 主.cn。
    返回 (源路径|None, 缺失说明)。"""
    cn文件们 = sorted(用例目录.glob("*.cn"))
    if not cn文件们:
        return None, "用例目录缺 .cn 源文件"
    if len(cn文件们) > 1:
        入口 = 用例目录 / "主.cn"
        if 入口.exists():
            return 入口, ""
        return None, "多文件用例目录需含入口 主.cn"
    return cn文件们[0], ""


def 前置检查(用例目录: Path, 选择器) -> str:
    """选中通道的执行前置（""=可消费）。缺失说明=管道跑不到/必红空转。

    判据来源（逐条对齐 run_e2e 消费点，审计锚：执行负用例/执行check负用例/
    解析v2锚定链配置/解析v2闭环配置/执行双编译对照/普通编排主路径）。"""
    if 选择器 is None:  # 普通编排：源 + 同名 .expected 缺一必红
        源, 说明 = 查源(用例目录)
        if 源 is None:
            return 说明
        if not 源.with_suffix(".expected").exists():
            return "普通编排缺同名期望文件 %s.expected" % 源.stem
        return ""
    if 选择器 == "期望编译失败.txt":
        _, 说明 = 查源(用例目录)
        return 说明
    if 选择器 == "期望check失败.txt":
        行们 = 有效行(用例目录 / 选择器)
        被检名 = 行们[0] if 行们 else ""
        if 被检名:
            return "" if (用例目录 / 被检名).exists() else \
                "期望check失败.txt 指定的被检文件不存在: %s" % 被检名
        cn文件们 = sorted(用例目录.glob("*.cn"))
        return "" if len(cn文件们) == 1 else \
            "check负用例需在 期望check失败.txt 首行指定被检文件（目录 .cn 数=%d）" % len(cn文件们)
    try:
        if 选择器 == "v2锚定链.txt":
            run_e2e.解析v2锚定链配置(用例目录 / 选择器)
            return ""
        配置 = run_e2e.解析v2闭环配置(用例目录 / 选择器)
    except ValueError as 异:
        return "%s 配置键非法: %s" % (选择器, 异)
    except OSError as 异:
        return "%s 不可读: %s" % (选择器, 异)
    缺源 = [f for f in 配置["源文件们"] if not (用例目录 / f).exists()]
    if 缺源:
        return "%s 配置的源文件不存在: %s" % (选择器, "、".join(缺源))
    缺供 = [f for f in 配置["供给源们"] if not (用例目录 / f).exists()]
    if 缺供:
        return "%s 配置的供给源不存在: %s" % (选择器, "、".join(缺供))
    if 选择器 == "双编译对照.txt" and 配置["预期编译失败"]:
        return ""  # 负测双测：.expected 可选（无则轻量 v2 负编译断言·569-a）
    入口 = 配置["源文件们"][0]
    if not (用例目录 / 入口).with_suffix(".expected").exists():
        return "%s 缺 %s.expected（v2 侧锚行/断言必需）" % (选择器, Path(入口).stem)
    return ""


def 遮蔽说明(用例目录: Path, 文件: str) -> str:
    """被遮蔽选择器的损失描述（按文件内容分档——双编译负测=801 最重伤形态）。"""
    if 文件 == "双编译对照.txt":
        文本 = ""
        try:
            文本 = (用例目录 / 文件).read_text(encoding="utf-8-sig")
        except OSError:
            pass
        负 = any(行.strip() in ("预期编译失败=是", "预期编译失败=true",
                              "预期编译失败=True", "预期编译失败=1")
                for 行 in 文本.splitlines())
        if 负:
            return "v2 侧负测（宿主+v2 双侧等效拒绝）永不执行——801 死配置形态"
        return "v2 侧双编译对照永不执行（该例 v2 语义零覆盖）"
    return {
        "v2闭环.txt": "v2 侧闭环永不执行（该例 v2 语义零覆盖）",
        "v2锚定链.txt": "v2 自举锚定链永不执行",
        "期望check失败.txt": "check 拒绝断言永不执行（check 通道被负 e2e 遮蔽）",
        "期望编译失败.txt": "build 拒绝断言永不执行",
    }.get(文件, "该通道配置永不执行")


def 判定用例(用例目录: Path) -> dict:
    """单用例消费判定：{通道, 选择器, 被遮蔽们, 跑不到}（纯静态·零执行）。"""
    选择器们 = [f for f, _ in 选择器序 if (用例目录 / f).exists()]
    赢 = 选择器们[0] if 选择器们 else None
    return {
        "通道": 选择器通道.get(赢, 普通通道),
        "选择器": 赢,
        "被遮蔽们": [{"文件": f, "说明": 遮蔽说明(用例目录, f)} for f in 选择器们[1:]],
        "跑不到": 前置检查(用例目录, 赢),
    }


def 收集用例目录(根: Path) -> list:
    """run_e2e.收集用例 同口径：目录名前两位为数字（排序稳定）。"""
    if not 根.exists():
        return []
    return sorted((d for d in 根.iterdir() if d.is_dir() and d.name[:2].isdigit()),
                  key=lambda d: d.name)


def 非编号目录(根: Path) -> list:
    """tests/e2e 下不被管道收集的目录（名不以两位数字开头·排除 __pycache__）。
    含 .cn 源=疑似用例残留（跑不到·红）；否则=非用例目录（提示）。"""
    出 = []
    if not 根.exists():
        return 出
    for d in sorted(根.iterdir()):
        if not d.is_dir() or d.name == "__pycache__" or d.name[:2].isdigit():
            continue
        出.append({"目录": d.name, "含源": bool(list(d.glob("*.cn")))})
    return 出


def 平台差异面(用例名: str) -> list:
    """命中平台跳过名单的平台们（run_e2e 匹配式：名以短名结尾或全等）。"""
    命中 = []
    for 平台, 前缀们 in run_e2e.平台跳过.items():
        for 短名 in 前缀们:
            if 用例名.endswith(短名) or 用例名 == 短名:
                命中.append(平台)
                break
    return 命中


def 审计分派序(文本: str) -> list:
    """静态提取 执行单个用例 函数体的分派选择器顺序（直连+配置路径别名两种形态）。
    返回按源码出现序的选择器文件名列表（去重保序）。"""
    起 = 文本.find("def 执行单个用例(")
    止 = 文本.find("\ndef ", 起 + 1)
    体 = 文本[起:止 if 止 > 0 else len(文本)]
    别名 = {}
    序 = []
    for 行 in 体.splitlines():      # 逐行匹配（^/$ 锚定语义=行·不依赖 MULTILINE）
        m = 正则_路径别名.match(行)
        if m:
            别名[m.group(1)] = m.group(2)
            continue
        m = 正则_直连分派.match(行) or 正则_变量分派.match(行)
        if not m:
            continue
        组 = m.group(1)
        文件 = 别名.get(组, 组)
        if 文件.endswith(".txt") and 文件 not in 序:
            序.append(文件)
    return 序


def 审计runner() -> str:
    """模型 × runner 一致性审计（空串=一致；非空=漂移说明）。"""
    源 = 仓库根 / "tests" / "e2e" / "run_e2e.py"
    try:
        文本 = 源.read_text(encoding="utf-8")
    except OSError as 异:
        return "run_e2e.py 不可读（%s）——转批序审计无法执行" % 异
    实际 = 审计分派序(文本)
    期望 = [f for f, _ in 选择器序]
    if 实际 != 期望:
        return ("run_e2e 通道分派序与模型表漂移：runner=%s ≠ 模型=%s"
                "——核对 tests/e2e/run_e2e.py 执行单个用例 后同步本表" % (实际, 期望))
    for 名 in ("平台跳过", "内存保护用例前缀", "收集用例"):
        if not hasattr(run_e2e, 名):
            return "run_e2e 缺属性 %s（名单/口径重命名？）——模型须同步" % 名
    return ""


def 取覆盖登记面():
    """覆盖矩阵登记名 → (名→来源说明, 可用, 原因, 鉴权失败)。复用
    check_spec_coverage.parse_map；失败真因从 parse_map 的失败诊断透传
    （1485/P2-C：401 不再显示成「不可达（降级）」）。"""
    try:
        行们, 豁免 = check_spec_coverage.parse_map()
    except SystemExit:
        原因, 鉴权失败 = check_spec_coverage.失败诊断()
        return {}, False, 原因 or "覆盖服务不可用", 鉴权失败
    except OSError as e:
        return {}, False, "覆盖服务不可读（%s）" % e, False
    except (ValueError, AttributeError, KeyError) as e:
        # P2-1 最后防线：响应畸形/非 dict/缺键等漏网形态都不得炸报告面。
        return {}, False, "覆盖服务响应异常（%s）" % e, False
    原因, 鉴权失败 = check_spec_coverage.失败诊断()
    名表 = {}
    for 单元, 列们 in 行们.items():
        for 列, 名们 in 列们.items():
            for 名 in 名们:
                名表.setdefault(名, "覆盖矩阵[%s] %s" % (单元, 列))
    for 名 in 豁免:
        名表.setdefault(名, "豁免行")
    return 名表, True, 原因, 鉴权失败


def 取号台账登记面():
    """号台账行们 → (行们, 可用, 原因, 鉴权失败)。复用 case_ledger.台账取回
    （含缓存降级）；1485/P2-C：四元组原因/鉴权失败透传到报告，401 不再
    显示成「不可达（降级）」。"""
    try:
        行们, 可用, 原因, 鉴权失败 = case_ledger.台账取回()
    except OSError as e:
        return [], False, "号台账不可读（%s）" % e, False
    return 行们, 可用, 原因, 鉴权失败


def 消费面(根: Path) -> dict:
    """枚举收集口径目录并逐例判定 → 消费面快照。"""
    用例 = {}
    统计 = {}
    平台差异 = {}
    重负载 = []
    for d in 收集用例目录(根):
        判 = 判定用例(d)
        用例[d.name] = 判
        统计[判["通道"]] = 统计.get(判["通道"], 0) + 1
        跳 = 平台差异面(d.name)
        if 跳:
            平台差异[d.name] = 跳
        if any(前缀 in d.name for 前缀 in run_e2e.内存保护用例前缀):
            重负载.append(d.name)
    return {"用例": 用例, "统计": 统计, "平台差异": 平台差异,
            "重负载": 重负载, "非编号": 非编号目录(根)}


def 比对(快照: dict, 覆盖名表: dict, 覆盖可用: bool, 台账行们: list, 台账可用: bool,
         覆盖原因: str = "", 覆盖鉴权失败: bool = False,
         台账原因: str = "", 台账鉴权失败: bool = False) -> dict:
    """消费面 × 登记面 逐条对账（死配置/跑不到/登记未消费/消费未登记）。"""
    用例 = 快照["用例"]
    目录名集 = set(用例)
    # 登记面命中判定：覆盖面=名精确匹配（含 v2专用: 前缀归一）；号台账=主题名归一
    覆盖名集 = set()
    for 名 in 覆盖名表:
        覆盖名集.add(名)
        覆盖名集.add(case_ledger.去v2专用(名))
    主题集 = {case_ledger.主题名(名): 名 for 名 in 目录名集}

    def 已登记(目录名: str) -> bool:
        if 目录名 in 覆盖名集:
            return True
        return case_ledger.主题名(目录名) in 主题集 and any(
            case_ledger.名匹配(case_ledger.主题名(目录名), r.get("用例名", ""))
            for r in 台账行们 if r.get("用例名"))
    # 号台账行在本树的落点（主题名归一·名匹配）
    台账未消费 = []
    for r in 台账行们:
        名 = (r.get("用例名") or "").strip()
        if not 名:
            continue
        裸名 = case_ledger.去v2专用(名)
        if 裸名 in 目录名集:
            continue
        if any(case_ledger.名匹配(主题, 名) for 主题 in 主题集):
            continue
        台账未消费.append({"号": str(r.get("号", "")), "用例名": 名})
    # 覆盖面登记名在本树无目录（与 check_spec_coverage --ci 同口径·精确名）
    覆盖未消费 = sorted(名 for 名 in 覆盖名表 if 名 not in 目录名集)

    死配置, 跑不到, 消费未登记 = [], [], []
    for 名, 判 in sorted(用例.items()):
        if 判["跑不到"]:
            跑不到.append({"目录": 名, "通道": 判["通道"],
                         "说明": 判["跑不到"], "已登记": 已登记(名)})
        for x in 判["被遮蔽们"]:
            死配置.append({"目录": 名, "文件": x["文件"], "说明": x["说明"],
                         "已登记": 已登记(名)})
        if not 已登记(名):
            消费未登记.append(名)
    return {
        "用例总数": len(用例),
        "通道分布": dict(sorted(快照["统计"].items(), key=lambda kv: -kv[1])),
        "平台差异面": sorted(快照["平台差异"].items()),
        "重负载面": sorted(快照["重负载"]),
        "非编号目录": 快照["非编号"],
        "死配置": 死配置,
        "跑不到": 跑不到,
        "登记未消费_覆盖": 覆盖未消费,
        "登记未消费_号台账": 台账未消费,
        "消费未登记": sorted(消费未登记),
        "覆盖面可用": 覆盖可用,
        "覆盖原因": 覆盖原因,
        "覆盖鉴权失败": 覆盖鉴权失败,
        "号台账可用": 台账可用,
        "号台账原因": 台账原因,
        "号台账鉴权失败": 台账鉴权失败,
        "跨分支口径": ("覆盖矩阵与号台账=服务端全局面、本树=单分支面：他分支"
                    "在飞用例（已领号/已登记未集成）常态出现在「登记未消费」"
                    "清单——与 check_case_numbers 613 陈旧检测合读两义"),
    }


def _面状态(可用: bool, 原因: str, 鉴权失败: bool) -> str:
    """登记面状态（1485/P2-C）：可用 / 鉴权失败 / 不可达 三分，原因摘要随行。"""
    摘要 = (原因[:90] + "…") if len(原因 or "") > 90 else (原因 or "")
    if 可用:
        return "可用" if not 摘要 else "可用（降级缓存：%s）" % 摘要
    if 鉴权失败:
        return "鉴权失败（%s）" % (摘要 or "401/403")
    return "不可达（%s）" % (摘要 or "服务不可用")


def 打印报告(报告: dict, 漂移: str, 截断: int) -> None:
    def 列(项们, 模板, 空提示):
        if not 项们:
            print("  [✓] " + 空提示)
            return
        for x in (项们 if 截断 <= 0 else 项们[:截断]):
            print("  " + 模板.format(**x))
        if 截断 > 0 and len(项们) > 截断:
            print("  …（另 %d 条略·--limit 0 全列）" % (len(项们) - 截断))

    print("=== 题库活性指标（837）：E2E 消费面 × 看板登记面 ===")
    print("消费面：%d 例（管道收集口径=目录名前两位数字）" % 报告["用例总数"])
    print("  通道分布：" + " ｜ ".join("%s %d" % (k, v) for k, v in 报告["通道分布"].items()))
    if 报告["平台差异面"]:
        print("  平台差异面（某平台跳过·合法）：%d 例" % len(报告["平台差异面"]))
    if 报告["重负载面"]:
        print("  重负载面（本地默认跳过·云端消费）：%d 例" % len(报告["重负载面"]))
    print("登记面：覆盖矩阵 %s ｜ 号台账 %s"
          % (_面状态(报告["覆盖面可用"], 报告.get("覆盖原因", ""),
                    报告.get("覆盖鉴权失败", False)),
             _面状态(报告["号台账可用"], 报告.get("号台账原因", ""),
                    报告.get("号台账鉴权失败", False))))
    print()
    def 登记注(x):
        return "（已在台账登记）" if x["已登记"] else "（未登记）"

    print("—— 死配置（选择器存在但通道优先级下永不消费·801 形态）：%d 处 ——"
          % len(报告["死配置"]))
    列([{"目录": x["目录"], "文件": x["文件"], "说明": x["说明"], "注": 登记注(x)}
       for x in 报告["死配置"]],
       "[{目录}] 被遮蔽：{文件} —— {说明}{注}", "无死配置")
    print()
    print("—— 跑不到（选中通道前置缺失·管道无法消费）：%d 处 ——" % len(报告["跑不到"]))
    列([{"目录": x["目录"], "通道": x["通道"], "说明": x["说明"], "注": 登记注(x)}
       for x in 报告["跑不到"]],
       "[{目录}] {通道}：{说明}{注}", "无前置缺失")
    print()
    print("—— 登记未消费·目录缺失（服务端登记名在本树无目录）：覆盖 %d ｜ 号台账 %d ——"
          % (len(报告["登记未消费_覆盖"]), len(报告["登记未消费_号台账"])))
    if 报告["登记未消费_覆盖"]:
        print("  覆盖面（精确名·--ci 同口径）：")
        列([{"名": x} for x in 报告["登记未消费_覆盖"]], "{名}", "")
    if 报告["登记未消费_号台账"]:
        print("  号台账（主题名归一）：")
        列(报告["登记未消费_号台账"], "#{号} {用例名}", "")
    if not 报告["登记未消费_覆盖"] and not 报告["登记未消费_号台账"]:
        print("  [✓] 登记名全部在本树有落点")
    print("  口径：" + 报告["跨分支口径"])
    print()
    print("—— 消费未登记（信息面·孤儿用例）：%d 例 ——" % len(报告["消费未登记"]))
    列([{"名": x} for x in 报告["消费未登记"]], "{名}", "无孤儿用例")
    if 报告["非编号目录"]:
        print()
        print("—— 非编号目录（管道不收集）：%d 个 ——" % len(报告["非编号目录"]))
        列([{"目录": x["目录"],
            "注": "（含 .cn 源·疑似用例残留）" if x["含源"] else ""}
           for x in 报告["非编号目录"]], "{目录}{注}", "")
    if 漂移:
        print()
        print("[漂移] " + 漂移)
    print()
    if 报告["死配置"] or 报告["跑不到"]:
        print("[黄] 死配置 %d 处 + 跑不到 %d 处——处置：死配置删旧标记或改写为单通道"
              "（801 先例：删 期望编译失败.txt 激活双编译负测）；跑不到补前置文件或"
              "修正配置；每处按「提及即立项」落看板台账行。"
              % (len(报告["死配置"]), len(报告["跑不到"])))
    else:
        print("[✓] 消费面零死配置零前置缺失。")


def 建报告() -> tuple:
    """组装 (报告, 漂移说明)。"""
    漂移 = 审计runner()
    快照 = 消费面(E2E)
    覆盖名表, 覆盖可用, 覆盖原因, 覆盖鉴权失败 = 取覆盖登记面()
    台账行们, 台账可用, 台账原因, 台账鉴权失败 = 取号台账登记面()
    return 比对(快照, 覆盖名表, 覆盖可用, 台账行们, 台账可用,
                覆盖原因, 覆盖鉴权失败, 台账原因, 台账鉴权失败), 漂移


# ============ 自测（临时样例树正反两态·零触碰真实树）============

def _造样例树(根: Path) -> None:
    def 建(名, 文件们):
        d = 根 / 名
        d.mkdir(parents=True)
        for 文件名, 内容 in 文件们.items():
            (d / 文件名).write_text(内容, encoding="utf-8")
    (根 / "__pycache__").mkdir(parents=True)
    建("01_普通正例", {"主.cn": "打印(1);", "主.expected": "1"})
    建("02_双编译正例", {"主.cn": "x", "主.expected": "x",
                    "双编译对照.txt": "退出码=0\n源文件=主.cn\n"})
    建("03_死配置双测", {"主.cn": "x", "期望编译失败.txt": "拒绝锚",
                    "双编译对照.txt": "退出码=负\n预期编译失败=是\n诊断锚=拒绝锚\n"})
    建("04_缺源双编译", {"双编译对照.txt": "退出码=0\n源文件=主.cn\n"})
    建("05_负例正例", {"主.cn": "x", "期望编译失败.txt": "锚"})
    建("06_v2闭环缺期望", {"主.cn": "x", "v2闭环.txt": "退出码=0\n"})
    建("07_check负正例", {"被检.cn": "x",
                     "期望check失败.txt": "被检.cn\n未声明的标识符 '甲'\n"})
    建("08_check负缺被检", {"被检.cn": "x",
                       "期望check失败.txt": "别处.cn\n锚\n"})
    建("09_双编译负无期望", {"主.cn": "x",
                      "双编译对照.txt": "退出码=负\n预期编译失败=是\n"})
    建("10_多源缺入口", {"甲.cn": "x", "乙.cn": "y", "主.expected": "x"})
    建("11_配置键非法", {"主.cn": "x", "主.expected": "x",
                    "v2闭环.txt": "未知键=1\n"})
    建("12_普通缺期望", {"主.cn": "x"})
    建("13_锚定链正例", {"v2锚定链.txt": "阶段=链构建\n"})
    建("14_供给缺失", {"主.cn": "x", "主.expected": "x",
                   "双编译对照.txt": "退出码=0\n供给=无此.cn\n"})
    (根 / "残留探针").mkdir(parents=True)
    (根 / "残留探针" / "主.cn").write_text("x", encoding="utf-8")
    (根 / "无源目录").mkdir(parents=True)


def selftest() -> int:
    """正反两态自证：死配置/跑不到/登记未消费/消费未登记四类各自的命中与不误报。"""
    with tempfile.TemporaryDirectory(prefix="ca837_") as td:
        根 = Path(td)
        _造样例树(根)
        快照 = 消费面(根)
        断言数 = [0]

        def 断言(条件, 说明):
            assert 条件, "自测失败：" + 说明
            断言数[0] += 1
            print("  ✓ " + 说明)

        用例 = 快照["用例"]
        断言("01_普通正例" in 用例 and 用例["01_普通正例"]["通道"] == 普通通道,
           "普通编排用例通道判定")
        断言(用例["02_双编译正例"]["通道"] == "双编译对照"
           and not 用例["02_双编译正例"]["被遮蔽们"]
           and not 用例["02_双编译正例"]["跑不到"], "双编译正例：零死配置零跑不到")
        断言(用例["13_锚定链正例"]["通道"] == "v2 锚定链"
           and not 用例["13_锚定链正例"]["跑不到"], "v2 锚定链通道判定（无源亦可）")
        断言(用例["09_双编译负无期望"]["跑不到"] == "",
           "双编译负测无 .expected=合法（569-a 轻量断言面·不误报）")
        断言(用例["07_check负正例"]["跑不到"] == "", "check 负被检文件在位")

        # 死配置：03（负测双测被遮蔽·801 形态）命中且说明分档；02/04 不误报
        死 = [x for x in 用例.items() if x[1]["被遮蔽们"]]
        断言([名 for 名, _ in 死] == ["03_死配置双测"], "死配置仅命中 03（正例不误报）")
        断言("801 死配置形态" in 用例["03_死配置双测"]["被遮蔽们"][0]["说明"],
           "遮蔽说明按内容分档（双编译负测=801 形态）")

        # 跑不到：逐类命中
        for 名, 关键词 in (("04_缺源双编译", "源文件不存在"), ("06_v2闭环缺期望", "expected"),
                        ("08_check负缺被检", "被检文件不存在"), ("10_多源缺入口", "主.cn"),
                        ("11_配置键非法", "配置键非法"), ("12_普通缺期望", "期望文件"),
                        ("14_供给缺失", "供给源不存在")):
            断言(关键词 in 用例[名]["跑不到"],
               "%s → 跑不到（%s）" % (名, 关键词))
        非编号 = {x["目录"]: x["含源"] for x in 快照["非编号"]}
        断言(非编号 == {"残留探针": True, "无源目录": False},
           "非编号目录枚举（含源标记分流）")

        # 比对：登记面注入（覆盖面精确名 + 号台账主题名归一）
        覆盖名表 = {"01_普通正例": "覆盖矩阵[2.1] 正例",
                 "03_死配置双测": "豁免行", "404_不存在用例": "覆盖矩阵[9.9] 负例"}
        台账行们 = [{"号": "10", "用例名": "双编译正例"},
                  {"号": "11", "用例名": "不存在主题"},
                  {"号": "12", "用例名": ""}]
        报告 = 比对(快照, 覆盖名表, True, 台账行们, True)
        断言(len(报告["死配置"]) == 1 and 报告["死配置"][0]["已登记"],
           "死配置对账命中且标注已登记")
        断言([x["目录"] for x in 报告["跑不到"]] ==
           sorted(["04_缺源双编译", "06_v2闭环缺期望", "08_check负缺被检",
                   "10_多源缺入口", "11_配置键非法", "12_普通缺期望", "14_供给缺失"]),
           "跑不到清单完整（7 处·2~14 号样例）")
        断言(报告["登记未消费_覆盖"] == ["404_不存在用例"],
           "覆盖面登记未消费（悬空名）")
        断言([x["用例名"] for x in 报告["登记未消费_号台账"]] == ["不存在主题"],
           "号台账登记未消费（主题名归一·02 被 10 号行覆盖不误报）")
        断言("05_负例正例" in 报告["消费未登记"] and "01_普通正例" not in 报告["消费未登记"]
           and "02_双编译正例" not in 报告["消费未登记"],
           "消费未登记（05 三面皆无命中·01 覆盖面与 02 号台账行不误报）")

        # 防漂移审计：对真实 run_e2e.py 跑分派序审计（模型表与 runner 必须一致）
        断言(审计runner() == "", "分派序审计通过（模型与 run_e2e 一致）")
        断言(审计分派序("def 执行单个用例(用例目录):\n"
                    '    if (用例目录 / "甲.txt").exists():\n        return 1\n'
                    "def 别的(x):\n" + '    if (用例目录 / "乙.txt").exists():\n        return 2\n')
           == ["甲.txt"], "分派序审计只扫 执行单个用例 函数体（截断于下一个 def）")
        漂 = 审计分派序('def 执行单个用例(x):\n'
                    '    if (用例目录 / "丙.txt").exists():\n        return 1\n')
        断言(漂 != [f for f, _ in 选择器序], "漂移样本可被识别（顺序不符）")
        print("自测通过：%d 项断言全绿" % 断言数[0])
        return 0


def main() -> int:
    解析 = argparse.ArgumentParser(
        description="题库活性指标（837）：E2E 消费面 × 台账登记面 机械比对",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    解析.add_argument("--strict", action="store_true",
                    help="死配置/跑不到>0 → rc=1（周检挂载点）；登记未消费面仍黄灯")
    解析.add_argument("--json", action="store_true", help="机读报告（周检台账/看板消费）")
    解析.add_argument("--limit", type=int, default=20, help="清单截断条数（0=全列·默认 20）")
    解析.add_argument("--selftest", action="store_true", help="自测（临时样例树·零触碰真实树）")
    参数 = 解析.parse_args()
    if 参数.selftest:
        return selftest()
    报告, 漂移 = 建报告()
    if 参数.json:
        print(json.dumps({"报告": 报告, "漂移": 漂移}, ensure_ascii=False, indent=1))
    else:
        打印报告(报告, 漂移, 参数.limit)
    if 漂移:
        if not 参数.json:
            print("[FAIL] 模型与 runner 漂移——报告不可信，先同步模型表（rc=2）")
        return 2
    if 参数.strict and (报告["死配置"] or 报告["跑不到"]):
        if not 参数.json:
            print("[FAIL] --strict：死配置/跑不到存在（rc=1）")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
