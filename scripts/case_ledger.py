#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""case_ledger.py —— E2E 用例号台账门禁核验与只读对照（577·接续 554 权威化·744 收权）。

病根链：554 装了「服务端台账发号 + check_case_numbers 唯一性门禁」，但两处缺口让
815/816 型跨分支撞号复发——①领号是可选项且门禁只扫本地工作树（两分支各跑各绿·
集成才撞）；②台账只记「号被领」不记「用例名」，号与用例无绑定（他人自拟同号
服务端无从拦）。577 修法：领号带用例名（服务端号名配对唯一·409 拒发）+ 本模块
把台账核验接入门禁（同号异名/异号同名即红）。744 编号权威收权（2026-10-14 用户令）：
客户端只报用例名领号、编号由服务器顺序发放——原「导入本地」带指定号写台账的
存量补录通道废除（存量已全量补齐·生产台账配名差集零），改为只读对照报告。

核验判据（本地 tests/e2e 目录 × 服务端台账）：
  - 同号异名：本地 #N 主题 ≠ 台账 #N 已配名 → 红（号被他人占用·你须改号）；反向
    「本地 #N 已在他号登记（异号同名）」同判红（名被占用）——若占名行 #N' 在本树
    无对应目录，疑似改号旧行残留（613：改号漏同步），红消息附注该事实。
  - 台账无此号 / 号未配名 / 服务不可达 → 黄字提示不拦（历史号 554 前存量面·
    断网不堵本地；须拦形态=台账已配名冲突，硬拦截点在集成与云端全量）。
  - 401/403（1425 登录墙鉴权失败）→ 硬红（核验/门禁不静默拿旧缓存放行——
    真因=令牌缺失/轮换/不符，与 task_board --ready 同口径）。
    号未配名补录通道（811）：board_cli 补用例名 <号> <用例名>——服务端对「已发放
    且名空」的存量号补配名（744 收权不发新号·已配名 409 拒改防篡改）。
  - 陈旧行（613）：台账已配名 #N「M」在本树既无 #N 目录、M 也无任何匹配目录 →
    黄字提示不拦——他分支在用（常态·忽略）与改号残留（须清理）不可机械区分；
    真实残留形态（旧号有行 + 新号本地活目录）由占名冲突红正面拦截。
  - v2 成对：v2 目录与宿主目录同号且同主题=合法（门禁侧成对判据先行），这里只
    校验宿主面，v2 目录自动跳过。
命令：
  python scripts/case_ledger.py 核验        # 台账核验（门禁同源·独立可跑）
  python scripts/case_ledger.py 对照本地    # 只读对照：本地目录×台账差集报告（游离/黄/陈旧）
  python scripts/case_ledger.py 查 <用例名> # 台账按名查询（转发 board_cli 查用例号）
退出码：核验=0 通过/提示·1 冲突/鉴权失败；对照=0 无游离·1 有游离/台账不可读。
"""
from __future__ import annotations

import json
import os
import re
import sys
import time
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
本树根 = 脚本目录.parent
E2E = 本树根 / "tests" / "e2e"
缓存 = 脚本目录 / "case_ledger_cache.json"


def 调服务(方法: str, 路径: str, 体=None, 超时=15):
    """转发 board_cli.调服务（单一实现·错误处置与 CLI 完全一致）。"""
    sys.path.insert(0, str(脚本目录))
    import board_cli
    return board_cli.调服务(方法, 路径, 体, 超时)


def 主题名(目录名: str) -> str:
    """83_v2_交集_全私有类 → 交集_全私有类；83_交集_全私有类负测 → 交集_全私有类。
    去掉编号前缀/可选 v2_ 段/可选负测 后缀——比对名 v2 与宿主侧同形（554 口径）。"""
    无名 = 目录名.split("_", 1)[1] if "_" in 目录名 else ""
    if 无名.startswith("v2_"):
        无名 = 无名[3:]
    return 无名[:-2] if 无名.endswith("负测") else 无名


def 去v2专用(名: str) -> str:
    """台账 v2 专用登记名前缀（577）：`v2专用:主题`/`v2专用<号>:主题` → `主题`。
    形态=前缀非后缀：`主题_v2专用` 会被「负测」剥离/主题碰撞干扰（实证）。"""
    m = re.match(r"^v2专用[0-9]*:", 名)
    return 名[m.end():] if m else 名


def 名匹配(本地主题: str, 台账名: str) -> bool:
    """主题×台账名匹配（双向归一·见核验台账）：v2专用: 前缀不参与比对。
    四形态：台账名=主题 / v2专用:主题 / 主题=台账名+负测（宿主负测对）/ 反向同理。"""
    if not 台账名:
        return True
    a, b = 本地主题, 台账名
    裸b = 去v2专用(b)
    return a in (裸b, 裸b + "负测") or 裸b in (a, a + "负测")


def 扫描本地用例() -> list:
    """tests/e2e 全部「编号_主题」目录 → [{号, 目录, 主题, v2}]（排除 __pycache__）。"""
    出 = []
    try:
        名们 = sorted(os.listdir(E2E))
    except OSError:
        return 出
    for 名 in 名们:
        if 名 == "__pycache__" or not os.path.isdir(E2E / 名):
            continue
        匹配 = re.match(r"^(\d+)_", 名)
        if 匹配:
            出.append({"号": 匹配.group(1), "目录": 名,
                       "主题": 主题名(名),
                       "v2": 名.split("_", 1)[1].startswith("v2_")})
    return 出


def _读缓存行() -> list:
    """本地最后一份台账缓存；缺失/损坏返回 []（556 容错口径）。"""
    try:
        旧 = json.loads(缓存.read_text(encoding="utf-8"))
        return 旧.get("台账", [])
    except (OSError, ValueError):
        return []


def 台账取回(允许缓存: bool = True):
    """台账全量 → (行们, 可用, 原因, 鉴权失败)。

    服务不可达读缓存（556 原子写口径）；401/403=鉴权失败（与网络不可达分流·
    intent.鉴权失败码/文案单点）——原因由调用方原样打印，鉴权失败位供门禁硬红。
    200 但缺「台账」字段=响应损坏，同样给出真因（不冒充不可达）。"""
    try:
        码, r = 调服务("GET", "/api/case_numbers")
    except (OSError, ValueError) as e:
        旧 = _读缓存行() if 允许缓存 else []
        return 旧, False, f"看板服务不可达（{e}）" + ("·读本地缓存" if 旧 else "·无缓存"), False
    if 码 == 200 and "台账" in r:
        try:
            缓存.write_text(json.dumps({"台账": r["台账"], "时戳": time.time()},
                                       ensure_ascii=False), encoding="utf-8")
        except OSError:
            pass
        return r["台账"], True, "", False
    鉴权失败 = False
    try:
        import intent
        鉴权失败 = intent.鉴权失败码(码)
    except Exception as e:
        print(f"[黄] intent 鉴权助手不可用，按裸码诊断（诊断见上）：{e}", file=sys.stderr)
    if 鉴权失败:
        try:
            import intent
            原因 = intent.鉴权失败文案("看板服务", 码)
        except Exception:
            原因 = f"看板服务鉴权失败（{码}）——检查 scripts/queue_client.json 令牌/登录墙"
    elif 码 == 200:
        原因 = "看板台账响应缺「台账」字段（服务端版本不符或响应损坏）"
    else:
        体 = (r or {}).get("错误", "")
        原因 = f"看板服务返回 HTTP {码}" + (f"：{体}" if 体 else "")
    旧 = _读缓存行() if 允许缓存 else []
    return 旧, False, 原因 + ("·读本地缓存" if 旧 else "·无缓存"), 鉴权失败


def 核验台账(取消豁免=()) -> dict:
    """本地目录 × 服务端台账核验——返回 {冲突们, 台账可用, 原因, 鉴权失败,
    未配名号数, 提示们, 陈旧们}。
    冲突形态（红·详见文件头）：同号异名/异号同名；提示形态（黄）：已领号未配名；
    陈旧形态（黄·613）：台账已配名行在本树无对应目录（他分支在用或改号残留）。
    豁免号（554 收编表）跳过。"""
    本地 = 扫描本地用例()
    行们, 可用, 原因, 鉴权失败 = 台账取回()
    号名图 = {str(x["号"]): x.get("用例名", "") for x in 行们}
    占名图 = {x["用例名"]: str(x["号"]) for x in 行们 if x.get("用例名")}
    本地号集 = {u["号"] for u in 本地}
    本地主题集 = {u["主题"] for u in 本地}
    冲突们, 提示们, 陈旧们 = [], [], []
    检过 = set()
    for 用 in 本地:
        if 用["号"] in 取消豁免 or 用["v2"] or 用["号"] in 检过:
            continue
        检过.add(用["号"])
        台账名 = 号名图.get(用["号"], "")
        if not 名匹配(用["主题"], 台账名):
            冲突们.append(f"#{用['号']} 本地「{用['主题']}」≠ 台账「{台账名}」"
                          f"（{用['目录']}）——号已被他人用例占用，须领新号改号")
        elif not 台账名 and 用["号"] in 号名图:
            提示们.append(f"#{用['号']} 「{用['主题']}」台账已领号未配名"
                          f"（补录：board_cli 补用例名 {用['号']} <用例名>·811 通道）")
        占号 = 占名图.get(用["主题"]) or 占名图.get(用["主题"] + "_v2专用")
        if 占号 and 占号 != 用["号"]:
            残留 = ("" if 占号 in 本地号集 else
                  f"；台账 #{占号} 在本树无对应目录——疑似改号旧行残留"
                  f"（613 陈旧形态·核对后按 tests/e2e/README.md 改号流程清理旧行）")
            冲突们.append(f"「{用['主题']}」本地 #{用['号']} ≠ 台账 #{占号}"
                          f"（{用['目录']}）——名字已在台账配给他人号，须领新号改号{残留}")
    # 613 陈旧行：台账已配名 × 本树既无该号目录、也无该名匹配目录（他分支在用或残留）
    for x in 行们:
        名 = x.get("用例名", "")
        号 = str(x["号"])
        if not 名 or 号 in 本地号集 or 号 in 取消豁免:
            continue
        if any(名匹配(主题, 名) for 主题 in 本地主题集):
            continue        # 名在本树活着（他号面）——占名冲突已覆盖，不重复计陈旧
        陈旧们.append(f"台账 #{号}「{名}」本树无对应用例目录——他分支在用则忽略；"
                     f"若为改号遗留=陈旧残留（处置见 tests/e2e/README.md 用例改号流程）")
    return {"冲突们": 冲突们, "台账可用": 可用, "原因": 原因, "鉴权失败": 鉴权失败,
            "未配名号数": sum(1 for x in 行们 if not x.get("用例名")),
            "本地用例数": len(本地), "提示们": 提示们, "陈旧们": 陈旧们}


def 对照本地(打印=print) -> int:
    """本地目录 × 服务端台账只读对照（744 编号权威收权：客户端写号通道已废——
    原「导入本地」带指定号写台账改为只读报告·2026-10-14 用户令）。
    输出三类：① 本地号不在台账=游离用例（服务端领号前历史/离线自编号产物——
    处置=按 tests/e2e/README.md 改号流程：服务端领新号→git mv→覆盖矩阵登记同步）；
    ② 台账已领号未配名（黄·补录走 board_cli 补用例名 <号> <名>——811 存量补配通道）；③ 台账有而本树无（陈旧·613）。
    返回游离数（>0=1 退出码·可作巡检信号）。"""
    本地 = 扫描本地用例()
    行们, 可用, 原因, _鉴权失败 = 台账取回()
    if not 可用:
        打印(f"[case_ledger·对照] {原因}——对照须以服务端台账为基准，修复后重试")
        return -1
    台账号集 = {str(x["号"]) for x in 行们}
    游离们 = [u for u in 本地 if u["号"] not in 台账号集]
    打印(f"[case_ledger·对照] 本地 {len(本地)} 用例 × 台账 {len(行们)} 行：")
    for u in 游离们:
        打印(f"  [游离] #{u['号']}（{u['目录']}）台账无此号——"
             f"按 tests/e2e/README.md 改号流程处置：服务端领新号→git mv→覆盖矩阵登记名同步")
    if not 游离们:
        打印("  [✓] 无游离用例（本地号全部在台账）")
    结 = 核验台账()
    for 提示 in 结["提示们"][:8]:
        打印(f"  [黄] {提示}")
    for 陈旧 in 结["陈旧们"][:8]:
        打印(f"  [陈旧] {陈旧}")
    return len(游离们)


def main() -> int:
    参数 = sys.argv[1:]
    if not 参数 or 参数[0] not in ("核验", "对照本地", "查"):
        print("用法: case_ledger.py 核验 | 对照本地 | 查 <用例名>")
        return 2
    if 参数[0] == "核验":
        结 = 核验台账()
        if 结["鉴权失败"]:
            print(f"[FAIL] {结['原因']}——台账核验无法进行"
                  "（401/403 不得静默降级；修复令牌后重跑）")
            return 1
        可 = "可用" if 结["台账可用"] else "不可读"
        台账面 = f"台账{可}" if 结["台账可用"] else f"台账不可读（{结['未配名号数']} 未配名疑似）"
        print(f"[case_ledger] 本地 {结['本地用例数']} 用例·{台账面}")
        if not 结["台账可用"]:
            print(f"  [黄] {结['原因']}")
        for 提示 in 结["提示们"][:8]:
            print(f"  [黄] {提示}")
        if 结["提示们"][8:]:
            print(f"  …（另 {len(结['提示们']) - 8} 条提示略）")
        for 陈旧 in 结["陈旧们"][:8]:
            print(f"  [陈旧] {陈旧}")
        if 结["陈旧们"][8:]:
            print(f"  …（另 {len(结['陈旧们']) - 8} 条陈旧行略）")
        if 结["冲突们"]:
            print(f"[FAIL] 台账冲突 {len(结['冲突们'])} 处：")
            for x in 结["冲突们"]:
                print(f"  {x}")
            return 1
        print("[PASS] 本地用例号名与台账一致（或台账无对应行）。")
        return 0
    if 参数[0] == "对照本地":
        print(f"[case_ledger·对照] 只读对照（744 编号权威收权·客户端写号通道已废）：")
        游离 = 对照本地()
        if 游离 < 0:
            return 1
        print(f"[case_ledger·对照] 完成·游离 {游离}")
        return 1 if 游离 else 0
    # 查 <用例名>——转发 board_cli 查用例号
    sys.path.insert(0, str(脚本目录))
    import board_cli
    import argparse
    class _假:
        pass
    a = _假()
    a.用例名 = 参数[1] if len(参数) > 1 else ""
    return board_cli.cmd_查用例号(a)


if __name__ == "__main__":
    sys.exit(main())
