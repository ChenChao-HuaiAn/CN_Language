#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""flaky_detector.py —— 非确定性侦探（任务 838·826 型非确定错值防再犯）。

【防误用口径·019 教训·必须守】同例二度红必须升级排查，禁判 flaky：
  本工具把「同例多轮运行输出/退出码不一致」判为非确定性（flaky）实锤；但反向
  口径必须守住——同一用例在同树同编译器同口径下**稳定红**（N 轮全红·原因一致）
  时，这不是「可以忽略的 flake」，而是真实红的可复现实证：必须按缺陷零容忍
  升级排查（现象/根因/影响面 + ≥3 方案呈报/立案），**禁止**以「可能是 flaky」
  名义搁置、重跑洗绿或降级放行。「不许 flaky」的另一半含义：一旦检出摇摆，
  摆动的两侧都必须定位（哪轮对/哪轮错、错值来源），flaky 判定不是免单而是立案
  依据。本工具报告在稳定红时显式提示「升级排查」，在 flaky 时输出差异样本供
  定位——两种终态都不允许被静默消化。

背景（826 型实录·头条 58 篇）：同一 IR 同构两处一好一坏=栈残留时序侥幸——错值
缺陷的非确定形态（时好时坏）用单次 E2E 无法捕获（跑一次绿就过去了）；对「错值
嫌疑用例」连跑 N 遍比对每次运行输出/退出码，任何轮间差异=非确定性实锤。

机制（零重复实现·复用 E2E 全链）：逐轮子进程调用
  python3 tests/e2e/run_e2e.py --filter <模式> --json <轮次文件> [--cn …] [--target …]
复用其全部机制（树锁/v2p 缓存/并行池/负例与双编译编排），每轮收集：
  - 子进程退出码；
  - 机器可读 JSON（842 契约：总计/通过/失败/未实现/跳过 + 失败与未实现明细含
    原因全文——「输出/退出码」差异样本即在失败原因中）。
比对面（任意两轮任何一项不同即差异）：
  - 退出码序列、汇总计数序列（总计/通过/失败/未实现/跳过）；
  - 逐用例状态序列（失败/未实现/非失败）与失败原因全文序列。
判定：全部一致=确定；出现差异=flaky（逐条列出差异样本与轮次）。

用法：
  python3 scripts/flaky_detector.py <用例名或模式> [--runs N] [--cn 路径]
                                    [--target 平台] [--jobs J] [--verbose]
                                    [--json-输出 路径]
  python3 scripts/flaky_detector.py --selftest    # 引擎自测（假执行器·正反两态）

判据/退出码：0=全部确定（稳定绿与稳定红均属确定·稳定红报告显式提示升级排查）；
1=检出非确定性（flaky）；2=用法/环境错误（模式无匹配、轮次全体数据缺失等）。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
import tempfile
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

脚本目录 = pathlib.Path(__file__).resolve().parent
项目根目录 = 脚本目录.parent
E2E目录 = 项目根目录 / "tests" / "e2e"
run_e2e路径 = E2E目录 / "run_e2e.py"

默认轮数 = 5          # 任务建议 N=5~10；单轮 v2p 缓存热态秒级~分钟级
最大轮数 = 50
每轮超时秒 = 3600     # 与 run_e2e 整池看门狗同量级（重负载单例 8~20 分钟）
ANSI正则 = re.compile(r"\x1b\[[0-9;]*m")
# 汇总计数守恒口径（run_e2e 写结果JSON）：总计 = 通过+失败+未实现+跳过
计数键们 = ("总计", "通过", "失败", "未实现", "跳过")


def 剥ANSI(文本: str) -> str:
    return ANSI正则.sub("", 文本 or "")


def 枚举目标用例(模式: str) -> list:
    """按 run_e2e 同款语义枚举目标用例目录名（排序）。

    与 tests/e2e/run_e2e.py 收集用例 同款：仅 NN_ 开头目录（name[:2].isdigit()）
    + 「模式 in 目录名」——第 2 处重复已留标记（第 3 处出现前提炼）；
    执行权威仍为 run_e2e 子进程本身，本函数只做「零匹配早退」防 N 轮空转。"""
    if not E2E目录.is_dir():
        return []
    return sorted(p.name for p in E2E目录.iterdir()
                  if p.is_dir() and p.name[:2].isdigit() and 模式 in p.name)


def 解析run_e2e摘要(标准输出: str) -> dict:
    """从 run_e2e 输出头提取「编译器/目标平台/全量理由」行（报告展示用）。

    提取失败=空 dict（报告写「未获取」）——展示面缺口不阻断判定（判定权威=rc+JSON）。"""
    摘要 = {}
    for 行 in 剥ANSI(标准输出).splitlines():
        行 = 行.strip()
        if 行.startswith("编译器:"):
            摘要["编译器"] = 行.split(":", 1)[1].strip()
        elif 行.startswith("目标平台:"):
            摘要["目标平台"] = 行.split(":", 1)[1].strip()
        elif 行.startswith("并行模式:"):
            摘要["并行模式"] = 行.split(":", 1)[1].strip()
    return 摘要


def 读轮JSON(路径: pathlib.Path):
    """读轮次 JSON；缺失/损坏返回 (None, 原因)——显式终态，不静默当空。"""
    if not 路径.exists():
        return None, "轮次 JSON 未生成（run_e2e 未启用 --json 写出或提前退出）"
    try:
        数据 = json.loads(路径.read_text(encoding="utf-8"))
    except (OSError, ValueError) as 异常:
        return None, f"轮次 JSON 不可读（{异常}）"
    if not isinstance(数据, dict) or "总计" not in 数据:
        return None, "轮次 JSON 结构不符（缺『总计』——842 契约）"
    return 数据, None


def 规范化轮JSON(原始: dict) -> dict:
    """把 run_e2e 的 JSON 字段规范化为比对用结构（键名与文档契约一致）。"""
    失败们 = [(x.get("名称", "?"), x.get("原因", "")) for x in 原始.get("失败用例", [])]
    未实现们 = [(x.get("名称", "?"), x.get("原因", "")) for x in 原始.get("未实现用例", [])]
    return {
        "总计": 原始.get("总计"),
        "通过": 原始.get("通过"),
        "失败": 原始.get("失败"),
        "未实现": 原始.get("未实现"),
        "跳过": 原始.get("跳过"),
        "失败用例": 失败们,
        "未实现用例": 未实现们,
        "时刻": 原始.get("时刻", ""),
    }


# ============ 轮次采集（执行器可注入——selftest 用假执行器走同一路径）============

def 执行轮(序号: int, 模式: str, 选项: dict, 轮json路径: pathlib.Path) -> dict:
    """真实执行器：子进程调 run_e2e --filter 模式 --json <轮文件>，收集一轮结果。

    返回 {序号, rc, 数据|None, 数据错误|None, 摘要, 输出尾}。"""
    命令 = [sys.executable, str(run_e2e路径), "--filter", 模式,
            "--json", str(轮json路径)]
    if 选项.get("cn"):
        命令 += ["--cn", 选项["cn"]]
    if 选项.get("target"):
        命令 += ["--target", 选项["target"]]
    if 选项.get("jobs"):
        命令 += ["--jobs", str(选项["jobs"])]
    if 选项.get("verbose"):
        命令 += ["--verbose"]
    if 轮json路径.exists():
        轮json路径.unlink()
    try:
        完成 = subprocess.run([str(c) for c in 命令], cwd=str(项目根目录),
                            capture_output=True, text=True, encoding="utf-8",
                            errors="replace", timeout=每轮超时秒)
        rc, 标准输出, 标准错误 = 完成.returncode, 完成.stdout or "", 完成.stderr or ""
    except subprocess.TimeoutExpired:
        rc, 标准输出, 标准错误 = -999, "", f"轮次超时（{每轮超时秒}s）"
    数据, 数据错误 = 读轮JSON(轮json路径)
    if 数据 is not None:
        数据 = 规范化轮JSON(数据)
    return {"序号": 序号, "rc": rc, "数据": 数据, "数据错误": 数据错误,
            "摘要": 解析run_e2e摘要(标准输出),
            "输出尾": " | ".join(剥ANSI(标准错误).strip().splitlines()[-2:])[:300],
            "输出头": " | ".join(剥ANSI(标准输出).strip().splitlines()[-3:])[:300]}


def 采集(模式: str, 轮数: int, 选项: dict, 执行器, 轮目录: pathlib.Path) -> list:
    """采集 轮数 个轮次（顺序执行——run_e2e 自管树锁，嵌套并发无意义且互踩）。"""
    轮们 = []
    for 序号 in range(1, 轮数 + 1):
        轮json = 轮目录 / f"轮{序号}.json"
        print(f"[轮 {序号}/{轮数}] 执行 run_e2e --filter {模式} …", flush=True)
        轮 = 执行器(序号, 模式, 选项, 轮json)
        _打印轮进度(轮)
        轮们.append(轮)
    return 轮们


def _打印轮进度(轮: dict) -> None:
    数据 = 轮.get("数据")
    if 数据 is None:
        print(f"  rc={轮['rc']} 数据缺失（{轮.get('数据错误')}）"
              f"{(' 尾部: ' + 轮['输出尾']) if 轮.get('输出尾') else ''}")
        return
    失败名们 = "、".join(名 for 名, _ in 数据["失败用例"]) or "无"
    print(f"  rc={轮['rc']} 总计={数据['总计']} 通过={数据['通过']} 失败={数据['失败']}"
          f" 未实现={数据['未实现']} 跳过={数据['跳过']} 失败用例: {失败名们}")


# ============ 判定引擎（纯函数·selftest 直测）============

def 比对轮次(轮们: list) -> dict:
    """逐用例+整体面比对 N 轮结果。返回裁定结构（纯数据）。

    {
      "用例判定们": [{名称, 判定: 确定|flaky|数据缺失, 状态序列, 失败原因们, 样本行们}],
      "整体差异们": [文本…],      # rc/计数/数据缺失层面的差异（即使用例面看不出）
      "flaky数": n, "稳定红数": n, "确定数": n, "缺失轮数": n,
    }
    """
    裁定 = {"用例判定们": [], "整体差异们": [], "flaky数": 0, "稳定红数": 0,
            "确定数": 0, "缺失轮数": sum(1 for 轮 in 轮们 if 轮.get("数据") is None),
            "缺失轮们": [轮.get("序号") for 轮 in 轮们 if 轮.get("数据") is None],
            "轮数": len(轮们)}
    有效轮们 = [轮 for 轮 in 轮们 if 轮.get("数据") is not None]

    # 整体面差异（只在数据完整的轮之间比对——数据缺失轮属环境面，单独显式列出
    # 于「缺失轮们」，不混入摇摆判定）：退出码序列 / 计数序列
    rc序列 = [轮.get("rc") for 轮 in 有效轮们]
    if len(set(rc序列)) > 1:
        裁定["整体差异们"].append(f"退出码序列摇摆: {rc序列}")
    for 键 in 计数键们:
        序列 = [轮["数据"].get(键) for 轮 in 有效轮们]
        if len(set(序列)) > 1:
            裁定["整体差异们"].append(f"计数『{键}』序列摇摆: {序列}（有效轮）")

    # 用例面：收集所有轮出现过的失败/未实现用例名并集
    名称集 = sorted({名 for 轮 in 有效轮们
                    for 名, _ in 轮["数据"]["失败用例"] + 轮["数据"]["未实现用例"]})
    for 名称 in 名称集:
        状态序列, 原因们, 样本行 = [], [], []
        for 轮 in 轮们:
            数据 = 轮.get("数据")
            if 数据 is None:
                状态序列.append("数据缺失")
                样本行.append(f"  轮{轮['序号']}: 数据缺失（{轮.get('数据错误')}）")
                continue
            失败因 = next((因 for 名, 因 in 数据["失败用例"] if 名 == 名称), None)
            未实现因 = next((因 for 名, 因 in 数据["未实现用例"] if 名 == 名称), None)
            if 失败因 is not None:
                状态序列.append("失败")
                原因们.append(失败因)
                样本行.append(f"  轮{轮['序号']}: 失败 — {失败因}")
            elif 未实现因 is not None:
                状态序列.append("未实现")
                原因们.append(未实现因)
                样本行.append(f"  轮{轮['序号']}: 未实现 — {未实现因}")
            else:
                状态序列.append("非失败")
                样本行.append(f"  轮{轮['序号']}: 非失败（通过或跳过——该轮未列名）")
        判定 = _裁定用例(状态序列, 原因们, 裁定["缺失轮数"])
        if 判定 == "flaky":
            裁定["flaky数"] += 1
        elif 判定 == "稳定红":
            裁定["稳定红数"] += 1
        else:
            裁定["确定数"] += 1
        裁定["用例判定们"].append({
            "名称": 名称, "判定": 判定, "状态序列": 状态序列,
            "失败原因们": 原因们, "样本行们": 样本行,
            "独立原因数": len(set(原因们)),
        })

    # 用例面无差异但整体面有差异时，仍计入 flaky 总量（防漏报）
    if 裁定["整体差异们"] and not 裁定["flaky数"]:
        裁定["flaky数"] = 1
    裁定["结论"] = _总体结论(裁定)
    return 裁定


def _裁定用例(状态序列: list, 原因们: list, 缺失轮数: int) -> str:
    """单用例裁定：稳定红 / flaky / 数据缺失 / 确定。

    稳定红 = N 轮全红且原因一致（019 教训：真实红的可复现实证·禁判 flaky 搁置）；
    flaky  = 状态往返（失败↔非失败/未实现）或同红异因（输出文本摇摆）。"""
    if "数据缺失" in 状态序列:
        return "数据缺失"
    if len(set(状态序列)) > 1:
        return "flaky"          # 失败↔通过/未实现 往返
    if len(set(原因们)) > 1:
        return "flaky"          # 同一状态但原因文本摇摆（如输出 diff 不同）
    if 状态序列 and 状态序列[0] == "失败":
        return "稳定红"
    return "确定"


def _总体结论(裁定: dict) -> str:
    if 裁定["缺失轮数"] >= 裁定["轮数"]:
        return "无法判定"      # 全体轮次无数据（run_e2e 未产出 JSON）——环境/用法错误面
    if 裁定["flaky数"]:
        return "检出非确定性"
    return "全部确定"


# ============ 报告 ============

def 打印报告(模式: str, 轮数: int, 轮们: list, 裁定: dict,
             json输出: str) -> int:
    print()
    print("=== 非确定性侦探（flaky_detector·任务 838）===")
    首轮摘要 = (轮们[0].get("摘要") or {}) if 轮们 else {}
    print(f"目标模式: {模式} ｜ 轮数: {轮数} ｜ 平台: {首轮摘要.get('目标平台', '未获取')}"
          f" ｜ 编译器: {首轮摘要.get('编译器', '未获取')}")
    print(f"每轮退出码: {[轮.get('rc') for 轮 in 轮们]}")
    for 轮 in 轮们:
        数据 = 轮.get("数据")
        if 数据 is not None:
            print(f"  轮{轮['序号']}: 总计={数据['总计']} 通过={数据['通过']}"
                  f" 失败={数据['失败']} 未实现={数据['未实现']} 跳过={数据['跳过']}")
        else:
            print(f"  轮{轮['序号']}: 数据缺失 — {轮.get('数据错误')}")
    if 裁定["缺失轮们"]:
        print(f"  ※ 数据缺失轮 {裁定['缺失轮们']}：判定基于其余 "
              f"{裁定['轮数'] - 裁定['缺失轮数']} 个有效轮（缺失面显式列出·未静默）")
    print()
    print("--- 用例判定 ---")
    if not 裁定["用例判定们"]:
        print("  （无失败/未实现用例——各轮全绿面：以整体面差异为准）")
    for 判定 in 裁定["用例判定们"]:
        _打印用例判定(判定)
    if 裁定["整体差异们"]:
        print("--- 整体面差异 ---")
        for 差异 in 裁定["整体差异们"]:
            print(f"  * {差异}")
    print()
    print(f"结论: {裁定['结论']} ｜ flaky {裁定['flaky数']} 例 ｜ "
          f"稳定红 {裁定['稳定红数']} 例 ｜ 确定 {裁定['确定数']} 例 ｜ "
          f"数据缺失轮 {裁定['缺失轮数']}")
    if 裁定["flaky数"]:
        print("→ 非确定性实锤：同树同编译器同口径下输出/退出码摇摆（差异样本见上）。"
              "两侧都必须定位（哪轮对/哪轮错、错值来源）——flaky 判定=立案依据非免单。")
    if 裁定["稳定红数"]:
        print("→ 稳定红（N 轮全红·原因一致）：不是 flaky，禁以「可能是 flaky」搁置——"
              "按真实缺陷升级排查（019 教训·现象/根因/影响面+方案呈报）。")
    if json输出:
        写判定JSON(json输出, 模式, 轮数, 轮们, 裁定)
    裁定码 = 1 if (裁定["flaky数"] or 裁定["结论"] == "无法判定") else 0
    if 裁定["结论"] == "无法判定":
        print("→ 全体轮次数据缺失（run_e2e 未产出 JSON）——用法/环境错误，退出码 2")
        return 2
    return 裁定码


def _打印用例判定(判定: dict) -> None:
    判定名 = 判定["判定"]
    if 判定名 == "flaky":
        print(f"  [flaky] {判定['名称']}：状态序列={判定['状态序列']}"
              f"（独立原因 {判定['独立原因数']} 种）")
        for 行 in 判定["样本行们"]:
            print(f"     {行}")
    elif 判定名 == "稳定红":
        print(f"  [稳定红] {判定['名称']}：{判定['状态序列'].count('失败')} 轮全红·原因一致"
              f"——升级排查（019 教训）")
    elif 判定名 == "数据缺失":
        print(f"  [数据缺失] {判定['名称']}：{判定['状态序列']}")
    else:
        非失败 = 判定["状态序列"].count("非失败")
        print(f"  [确定] {判定['名称']}：状态序列={判定['状态序列']}"
              f"（非失败 {非失败} 轮）")


def 写判定JSON(路径: str, 模式: str, 轮数: int, 轮们: list, 裁定: dict) -> None:
    """附加输出面：判定结果落盘（供后续机械化消费）；写失败=显式警告不改判定结论。"""
    内容 = {
        "工具": "flaky_detector", "任务": "838", "模式": 模式, "轮数": 轮数,
        "时刻": time.strftime("%Y-%m-%d %H:%M:%S"),
        "每轮退出码": [轮.get("rc") for 轮 in 轮们],
        "每轮汇总": [{k: 轮["数据"].get(k) for k in 计数键们}
                   if 轮.get("数据") is not None else None for 轮 in 轮们],
        "结论": 裁定["结论"], "flaky数": 裁定["flaky数"],
        "稳定红数": 裁定["稳定红数"], "确定数": 裁定["确定数"],
        "整体差异们": 裁定["整体差异们"],
        "用例判定们": [{k: v for k, v in 判定.items()} for 判定 in 裁定["用例判定们"]],
    }
    try:
        pathlib.Path(路径).write_text(
            json.dumps(内容, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
        print(f"判定 JSON 已写入: {路径}")
    except OSError as 异常:
        print(f"警告: 判定 JSON 写入失败（{异常}）——不影响本次判定结论")


# ============ 自测（假执行器·正反两态·零触碰真实树）============

def _假轮(序号: int, rc: int = 0, 数据: dict = None, 数据错误: str = None) -> dict:
    return {"序号": 序号, "rc": rc, "数据": 数据, "数据错误": 数据错误,
            "摘要": {"编译器": "假", "目标平台": "假"}, "输出尾": "", "输出头": ""}


def _假JSON(总计=1, 通过=1, 失败=0, 未实现=0, 跳过=0, 失败用例=None, 未实现用例=None) -> dict:
    return {"总计": 总计, "通过": 通过, "失败": 失败, "未实现": 未实现, "跳过": 跳过,
            "失败用例": 失败用例 or [], "未实现用例": 未实现用例 or [], "时刻": "假"}


def selftest() -> int:
    """引擎自测：正反两态全链（采集→比对→裁定）。
    真执行器替换为假执行器（注入预置轮数据），其余代码路径与生产完全一致。"""
    通过 = 失败 = 0
    用例 = "999_自测用例"

    def 签(名: str, 条件: bool, 详情: str = "") -> None:
        nonlocal 通过, 失败
        if 条件:
            通过 += 1
            print(f"  [签] {名}")
        else:
            失败 += 1
            print(f"  [X]  {名} {详情}")

    def 假执行器(轮们序列):
        def 执行(序号, 模式, 选项, 轮json路径):
            return 轮们序列[序号 - 1]
        return 执行

    def 绿轮(n):
        return _假轮(n, 0, _假JSON())

    def 红轮(n, 因="输出不一致\n    期望: ['42']\n    实际: ['43']"):
        return _假轮(n, 1, _假JSON(通过=0, 失败=1, 失败用例=[(用例, 因)]))

    # —— 反态1：绿红交替 → flaky 检出 + 差异样本 ——
    序列 = [绿轮(1), 红轮(2), 绿轮(3), 红轮(4), 绿轮(5)]
    with tempfile.TemporaryDirectory() as 临时:
        轮们 = 采集("999", 5, {}, 假执行器(序列), pathlib.Path(临时))
    裁定 = 比对轮次(轮们)
    签("绿红交替检出 flaky", 裁定["flaky数"] == 1 and 裁定["结论"] == "检出非确定性",
       f"裁定={裁定['结论']} flaky={裁定['flaky数']}")
    判定 = 裁定["用例判定们"][0] if 裁定["用例判定们"] else {}
    签("差异样本含轮次两侧", 判定.get("状态序列") == ["非失败", "失败", "非失败", "失败", "非失败"],
       f"序列={判定.get('状态序列')}")
    签("整体退出码序列摇摆可见", any("退出码" in x for x in 裁定["整体差异们"]),
       f"差异={裁定['整体差异们']}")

    # —— 反态2：全绿 → 确定·不误报 ——
    with tempfile.TemporaryDirectory() as 临时:
        轮们 = 采集("999", 5, {}, 假执行器([绿轮(i) for i in range(1, 6)]), pathlib.Path(临时))
    裁定 = 比对轮次(轮们)
    签("全绿不误报", 裁定["flaky数"] == 0 and 裁定["结论"] == "全部确定"
       and not 裁定["用例判定们"], f"裁定={裁定['结论']} flaky={裁定['flaky数']}")

    # —— 反态3：稳定红（同因）→ 确定-稳定红·提示升级排查（019 教训）——
    with tempfile.TemporaryDirectory() as 临时:
        轮们 = 采集("999", 3, {}, 假执行器([红轮(i) for i in range(1, 4)]), pathlib.Path(临时))
    裁定 = 比对轮次(轮们)
    判定 = 裁定["用例判定们"][0]
    签("稳定红=非 flaky", 裁定["flaky数"] == 0 and 裁定["稳定红数"] == 1
       and 判定["判定"] == "稳定红", f"裁定={裁定['稳定红数']}/{判定['判定']}")
    签("稳定红原因一致", 判定["独立原因数"] == 1, f"独立原因={判定['独立原因数']}")

    # —— 反态4：红但原因不同（同红异因）→ flaky ——
    轮们 = [_假轮(1, 1, _假JSON(通过=0, 失败=1, 失败用例=[(用例, "编译失败(退出码1)")])),
           _假轮(2, 1, _假JSON(通过=0, 失败=1, 失败用例=[(用例, "输出不一致 期望[1] 实际[2]")])),
           _假轮(3, 1, _假JSON(通过=0, 失败=1, 失败用例=[(用例, "编译失败(退出码1)")]))]
    裁定 = 比对轮次(轮们)
    签("同红异因检出 flaky", 裁定["flaky数"] == 1, f"flaky={裁定['flaky数']}")
    签("同红异因 rc 面不摇摆（仅用例面）", not 裁定["整体差异们"],
       f"整体={裁定['整体差异们']}")

    # —— 反态5：未实现↔通过 往返 → flaky ——
    轮们 = [_假轮(1, 0, _假JSON()),
           _假轮(2, 1, _假JSON(通过=0, 未实现=1, 未实现用例=[(用例, "尚未实现build")])),
           _假轮(3, 0, _假JSON())]
    裁定 = 比对轮次(轮们)
    签("未实现↔通过检出 flaky", 裁定["flaky数"] == 1, f"flaky={裁定['flaky数']}")

    # —— 反态6：数据缺失轮 → 显式「数据缺失」·不误判、不静默 ——
    轮们 = [绿轮(1), _假轮(2, 2, None, "轮次 JSON 未生成（run_e2e 未启用 --json 写出或提前退出）"), 绿轮(3)]
    裁定 = 比对轮次(轮们)
    签("数据缺失显式记录", 裁定["缺失轮数"] == 1 and 裁定["缺失轮们"] == [2]
       and 裁定["flaky数"] == 0, f"缺失={裁定['缺失轮们']} flaky={裁定['flaky数']}")
    签("数据缺失不污染摇摆比对", not 裁定["整体差异们"], f"整体={裁定['整体差异们']}")

    # —— 反态7：全体数据缺失 → 无法判定 ——
    轮们 = [_假轮(i, 2, None, "缺") for i in (1, 2)]
    裁定 = 比对轮次(轮们)
    签("全缺失=无法判定", 裁定["结论"] == "无法判定", f"结论={裁定['结论']}")

    # —— 反态8：通过↔跳过 计数摇摆（通过/跳过不可细分面）→ 整体面捕获 ——
    轮们 = [_假轮(1, 0, _假JSON()),
           _假轮(2, 0, _假JSON(通过=0, 跳过=1)),
           _假轮(3, 0, _假JSON())]
    裁定 = 比对轮次(轮们)
    签("通过↔跳过摇摆（整体面）", 裁定["flaky数"] == 1
       and any("『跳过』" in x for x in 裁定["整体差异们"]),
       f"flaky={裁定['flaky数']} 整体={裁定['整体差异们']}")

    # —— 反态9：JSON 解析函数（结构不符显式报罪）——
    with tempfile.TemporaryDirectory() as 临时:
        坏 = pathlib.Path(临时) / "坏.json"
        坏.write_text('{"foo": 1}', encoding="utf-8")
        数据, 错误 = 读轮JSON(坏)
        签("坏 JSON 显式报罪", 数据 is None and "结构不符" in 错误, 错误 or "")
        缺 = pathlib.Path(临时) / "不存在.json"
        数据, 错误 = 读轮JSON(缺)
        签("缺失 JSON 显式报罪", 数据 is None and "未生成" in 错误, 错误 or "")

    print(f"\nselftest: {通过} 通过 / {失败} 失败")
    return 0 if 失败 == 0 else 1


# ============ 入口 ============

def 主程序() -> int:
    解析器 = argparse.ArgumentParser(
        description="非确定性侦探（任务 838）：错值嫌疑用例连跑 N 遍比对输出/退出码",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n"
               "  python3 scripts/flaky_detector.py 826 --runs 8\n"
               "  python3 scripts/flaky_detector.py 826_i128 --runs 5 --target linux-x86_64\n"
               "  python3 scripts/flaky_detector.py --selftest")
    解析器.add_argument("目标", nargs="?", default=None,
                        help="用例名或模式（run_e2e --filter 同款语义：目录名包含即匹配）")
    解析器.add_argument("--runs", type=int, default=默认轮数,
                        help=f"连跑轮数（默认 {默认轮数}·任务建议 5~10）")
    解析器.add_argument("--cn", default=None, help="cn 编译器路径（透传 run_e2e）")
    解析器.add_argument("--target", default=None, help="目标平台（透传 run_e2e）")
    解析器.add_argument("--jobs", type=int, default=None, help="并行度（透传 run_e2e）")
    解析器.add_argument("--verbose", action="store_true", help="透传 run_e2e 详细输出")
    解析器.add_argument("--json-输出", dest="json输出", default=None, metavar="路径",
                        help="判定结果 JSON 落盘（附加输出面·写失败不改判定）")
    解析器.add_argument("--selftest", action="store_true", help="引擎自测（假执行器）")
    参数 = 解析器.parse_args()

    if 参数.selftest:
        return selftest()
    if not 参数.目标 or len(参数.目标.strip()) < 2:
        print("错误: 需给用例名或模式（≥2 字符——run_e2e 裸全量护栏同口径），"
              "如 flaky_detector.py 826 --runs 8")
        return 2
    if not (2 <= 参数.runs <= 最大轮数):
        print(f"错误: --runs 须在 2~{最大轮数} 之间（收到 {参数.runs}）")
        return 2
    if not run_e2e路径.exists():
        print(f"错误: 未找到 E2E 运行器 {run_e2e路径}")
        return 2

    匹配 = 枚举目标用例(参数.目标.strip())
    if not 匹配:
        print(f"错误: 模式『{参数.目标}』在 tests/e2e 无匹配用例（零轮空转已拒绝）")
        return 2
    print(f"目标匹配 {len(匹配)} 例: {'、'.join(匹配[:8])}"
          f"{'…' if len(匹配) > 8 else ''}")

    选项 = {"cn": 参数.cn, "target": 参数.target, "jobs": 参数.jobs,
            "verbose": 参数.verbose}
    with tempfile.TemporaryDirectory(prefix="flaky_detector_") as 临时:
        轮们 = 采集(参数.目标.strip(), 参数.runs, 选项, 执行轮, pathlib.Path(临时))
    裁定 = 比对轮次(轮们)
    return 打印报告(参数.目标.strip(), 参数.runs, 轮们, 裁定, 参数.json输出)


if __name__ == "__main__":
    sys.exit(主程序())
