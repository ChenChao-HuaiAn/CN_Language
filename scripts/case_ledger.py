#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""case_ledger.py —— E2E 用例号台账门禁核验与存量补录（577·接续 554 权威化）。

病根链：554 装了「服务端台账发号 + check_case_numbers 唯一性门禁」，但两处缺口让
815/816 型跨分支撞号复发——①领号是可选项且门禁只扫本地工作树（两分支各跑各绿·
集成才撞）；②台账只记「号被领」不记「用例名」，号与用例无绑定（他人自拟同号
服务端无从拦）。577 修法：领号带用例名（服务端号名配对唯一·409 拒发）+ 本模块
把台账核验接入门禁（同号异名/异号同名即红）+ 存量一键补录。

核验判据（本地 tests/e2e 目录 × 服务端台账）：
  - 同号异名：本地 #N 主题 ≠ 台账 #N 已配名 → 红（号被他人占用·你须改号）；反向
    「本地 #N 已在他号登记（异号同名）」同判红（名被占用）——若占名行 #N' 在本树
    无对应目录，疑似改号旧行残留（613：改号漏同步），红消息附注该事实。
  - 台账无此号 / 号未配名 / 服务不可达 → 黄字提示不拦（历史号 554 前存量面·
    断网不堵本地；须拦形态=台账已配名冲突，硬拦截点在集成与云端全量）。
  - 陈旧行（613）：台账已配名 #N「M」在本树既无 #N 目录、M 也无任何匹配目录 →
    黄字提示不拦——他分支在用（常态·忽略）与改号残留（须清理）不可机械区分；
    真实残留形态（旧号有行 + 新号本地活目录）由占名冲突红正面拦截。
  - v2 成对：v2 目录与宿主目录同号且同主题=合法（门禁侧成对判据先行），这里只
    校验宿主面，v2 目录自动跳过。
命令：
  python scripts/case_ledger.py 核验        # 台账核验（门禁同源·独立可跑）
  python scripts/case_ledger.py 导入本地    # 存量补录：本地全部用例的「号↔主题」配对导入台账
  python scripts/case_ledger.py 查 <用例名> # 台账按名查询（转发 board_cli 查用例号）
退出码：核验=0 通过/提示·1 冲突；导入=0 全成·1 有失败。
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


def 取服务():
    """获取 (服务地址, 令牌)——复用 board_cli 实现（单一来源·延迟导入防环）。"""
    sys.path.insert(0, str(脚本目录))
    import board_cli
    return board_cli.服务地址(), board_cli.令牌()


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


def 台账取回(允许缓存: bool = True):
    """台账全量 → (行们, 可用)。服务不可达读缓存（556 原子写口径）；无缓存返回 ([], False)。"""
    try:
        码, r = 调服务("GET", "/api/case_numbers")
    except OSError:
        码, r = 0, {}
    if 码 == 200 and "台账" in r:
        try:
            缓存.write_text(json.dumps({"台账": r["台账"], "时戳": time.time()},
                                       ensure_ascii=False), encoding="utf-8")
        except OSError:
            pass
        return r["台账"], True
    if 允许缓存:
        try:
            旧 = json.loads(缓存.read_text(encoding="utf-8"))
            return 旧.get("台账", []), False
        except (OSError, ValueError):
            pass
    return [], False


def 核验台账(取消豁免=()) -> dict:
    """本地目录 × 服务端台账核验——返回 {冲突们, 台账可用, 未配名号数, 提示们, 陈旧们}。
    冲突形态（红·详见文件头）：同号异名/异号同名；提示形态（黄）：已领号未配名；
    陈旧形态（黄·613）：台账已配名行在本树无对应目录（他分支在用或改号残留）。
    豁免号（554 收编表）跳过。"""
    本地 = 扫描本地用例()
    行们, 可用 = 台账取回()
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
            提示们.append(f"#{用['号']} 「{用['主题']}」台账已领号未配名（建议补录）")
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
    return {"冲突们": 冲突们, "台账可用": 可用,
            "未配名号数": sum(1 for x in 行们 if not x.get("用例名")),
            "本地用例数": len(本地), "提示们": 提示们, "陈旧们": 陈旧们}


def 导入本地(打印=print) -> int:
    """存量补录：本地全部用例的「号↔主题」配对逐条导入台账（已存在号=服务端校验后更新）。
    每号必登记一次（含 v2-only 号——跨分支窗口根治：该号入台账后他人再自拟同号即被拦）；
    宿主侧名优先（成对=NN_主题 与 NN_v2_主题 主题名归一后相同·无差别）；v2-only 号取
    v2 目录主题名（=去 v2_ 前缀·将来宿主侧建同号同主题可自洽核验）。
    用 --号 通道（编号_主题 直配·幂等）；失败逐条打印不中断。返回失败数。"""
    本地 = 扫描本地用例()
    按号 = {}
    for 用 in 本地:
        按号.setdefault(用["号"], []).append(用)
    选用 = {}
    for 号, 们 in 按号.items():
        宿主 = [u for u in 们 if not u["v2"]]
        选用[号] = (宿主[0] if 宿主 else 们[0])
    成败 = 0
    for 号 in sorted(选用, key=int):
        用 = 选用[号]
        结 = _领一号(用)
        if 结.get("好"):
            标 = "✓" if 结["试名"] == 用["主题"] else "✓v2"
            打印(f"  [{标}] #{用['号']}「{结['试名']}」")
        else:
            打印(f"  [败] #{用['号']}「{用['主题']}」{结['错']}")
            成败 += 1
    return 成败


def _领一号(用: dict) -> dict:
    """单号导入（逐层降级：裸主题名 → v2专用: 前缀名 → v2专用<号>: 前缀名）。
    二次/三次尝试=名已配给同主题旧号（如 266_v2_不安全边界 与 265_不安全边界 同主题
    的历史编号形态）——前缀保台账唯一；<号> 段对「双 v2 号撞同一主题」（754/755 族）
    机械保唯一（门禁侧去前缀归一比对·577 立法）。返回 {好, 试名} 或 {好:False, 错}。"""
    for 试名 in (用["主题"], "v2专用:" + 用["主题"], f"v2专用{用['号']}:" + 用["主题"]):
        try:
            码, r = 调服务("POST", "/api/case_number",
                          {"机器": "case_ledger导入", "用例名": 试名,
                           "指定号": 用["号"], "说明": "577 存量补录"})
        except OSError as e:
            码, r = 0, {"错误": f"服务不可达（{e}）"}
        if 码 == 201 and str(r.get("号")) == 用["号"]:
            return {"好": True, "试名": 试名}
        if "已于号" in str(r.get("错误", "")):
            continue            # 名占用（同主题旧号）→ 降级带前缀名
        return {"好": False, "错": f"{码} {r.get('错误', r)}"}   # 非名占用类失败不重试
    return {"好": False, "错": "同名占用（含 v2专用: 前缀）"}


def main() -> int:
    参数 = sys.argv[1:]
    if not 参数 or 参数[0] not in ("核验", "导入本地", "查"):
        print("用法: case_ledger.py 核验 | 导入本地 | 查 <用例名>")
        return 2
    if 参数[0] == "核验":
        结 = 核验台账()
        可 = "可用" if 结["台账可用"] else "不可达·读缓存"
        台账面 = f"台账{可}" if 结["台账可用"] else f"台账不可达（{结['未配名号数']} 未配名疑似）"
        print(f"[case_ledger] 本地 {结['本地用例数']} 用例·{台账面}")
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
    if 参数[0] == "导入本地":
        sys.path.insert(0, str(脚本目录))
        import board_cli
        print(f"[case_ledger·导入] {board_cli.服务地址()} 台账存量补录：")
        败 = 导入本地()
        print(f"[case_ledger·导入] 完成·失败 {败}")
        return 1 if 败 else 0
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
