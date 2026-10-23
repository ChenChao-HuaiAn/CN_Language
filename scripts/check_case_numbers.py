#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_case_numbers.py —— E2E 用例编号唯一性门禁（554 建·577 台账核验增强）。

病根（554 轮考古）：E2E 用例编号（tests/e2e/<编号>_<名称>/）无权威发号也无唯一性
校验——487/489/496 三轮纯人工全库盘点治理存量（42 组 46 目录改号），496 落地后
未装任何防再撞机制，10 分钟后并行任务 488 rebase 进来即再撞 752/753（存活一天
无门禁可拦）。本门禁机械拦增量：L1（gate_quick 静态段）/ win 全量（ci.ps1）/云端。

判据两层（577）：
  ① 本地目录：同一数字编号对应多个用例目录 = 撞号。两类合法形态除外：
     宿主+v2 成对（恰两目录，其一去掉编号前缀与 "v2_" 段后与另一全等）；
     显式豁免号（下表·496 口径留档：早期功能分节/宿主与 v2 合用一号）。
  ② 台账核验（case_ledger.核验台账）：本地「编号_主题」× 服务端用例号台账比对，
     同号异名/异号同名即真撞号——跨分支可见（领号即入台账·815/816 型互不可见
     窗口根治面）；台账无此号/号未配名=黄字提示（554 前未配名存量面）。
     613 增陈旧检测：台账已配名行在本树无对应目录=黄字提示（他分支在用或改号
     残留·真实残留形态并入占名冲突红附注）；服务不可达降级=本地判据照跑·
     台账核验黄字（集成与云端全量为硬拦截点）；401/403=鉴权失败硬红
     （1425 登录墙后不得拿旧缓存静默放行·与 task_board --ready 同口径）。

处置（487/489/496 先例）：先入者保留·后入者领新号改号——
  python scripts/board_cli.py 领用例号 <用例名>   # 服务端原子发号（号名配对唯一）
改号同步面（六步全文=tests/e2e/README.md「用例目录命名约定·改号流程」）：目录改名 +
主.cn 首行注释 + 服务端覆盖矩阵登记名 + 豁免行（旧删新补）+ 号台账旧行核对。

退出码：0=通过（无真撞号）；1=发现真撞号/台账鉴权失败（CI 红；--strict 下台账
冲突/提示亦红）。
用法：python scripts/check_case_numbers.py [--strict]
"""
import os
import re
import sys

import case_ledger

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
E2E = os.path.join(ROOT, "tests", "e2e")

# 496 口径豁免（decc47ff 甄别留档·新增豁免必须在表内写明来由与批准轮次）
豁免号 = {
    "24": "早期功能分节（24_error_handling/24_preprocessor）",
    "25": "早期功能分节（25_math/25_operator）",
    "84": "早期规则分节（84_强制检查_规则1丢弃/规则3可选·覆盖率矩阵两行分引）",
    "210": "宿主与 v2 合用一号（返回借用拒绝/返回拥有字符串 双特性各带宿主+v2 对）",
}


def _去编号段(目录名: str) -> str:
    """去掉编号前缀段：83_v2_交集_全私有类 → v2_交集_全私有类。"""
    段 = 目录名.split("_", 1)
    return 段[1] if len(段) > 1 else ""


def 主题名(目录名: str) -> str:
    """去掉编号前缀与 v2_ 段（83_v2_交集_全私有类 → 交集_全私有类）——单一实现归
    case_ledger（台账核验同口径·归纳零复制）。"""
    return case_ledger.主题名(目录名)


def 是成对(a: str, b: str) -> bool:
    """NN_主题 与 NN_v2_主题 判定（恰一方带 v2_ 段·顺序无关）。两种历史惯例形态：
      ① 主题全等（如 112_方案A…/112_v2_方案A…）；
      ② 宿主侧多「负测」后缀（如 82_交集_模块私有类负测/82_v2_交集_模块私有类·
         159/163/265/275/279 同型——负测对的 v2 版沿用正测名，宿主版显式标负测）。"""
    a_v2 = _去编号段(a).startswith("v2_")
    b_v2 = _去编号段(b).startswith("v2_")
    if a_v2 == b_v2:
        return False
    ta, tb = 主题名(a), 主题名(b)
    return ta == tb or ta + "负测" == tb or tb + "负测" == ta


def 扫描分组() -> dict:
    """编号 → [目录名们]（仅数字前缀目录·排除 __pycache__）。"""
    组 = {}
    for 名 in sorted(os.listdir(E2E)):
        if 名 == "__pycache__" or not os.path.isdir(os.path.join(E2E, 名)):
            continue
        匹配 = re.match(r"^(\d+)_", 名)
        if 匹配:
            组.setdefault(匹配.group(1), []).append(名)
    return 组


def main() -> int:
    import argparse
    p = argparse.ArgumentParser(description="E2E 用例编号唯一性门禁（554·577 台账核验）")
    p.add_argument("--strict", action="store_true",
                   help="台账核验提示项（未配名）亦判红（CI 全量口径）")
    a = p.parse_args()
    组 = 扫描分组()
    真撞 = []
    for 号, 们 in sorted(组.items(), key=lambda kv: int(kv[0])):
        if len(们) == 1 or 号 in 豁免号:
            continue
        if len(们) == 2 and 是成对(们[0], 们[1]):
            continue
        真撞.append((号, 们))
    多号组数 = sum(1 for 们 in 组.values() if len(们) > 1)
    print("=== E2E 用例编号唯一性门禁（554 建·577 台账核验·AGENTS §4 条文执行面）===")
    print("目录 %d·唯一编号 %d·多目录编号 %d 组（豁免 %d 号·成对与豁免之外的组=真撞号）"
          % (sum(len(v) for v in 组.values()), len(组), 多号组数, len(豁免号)))
    if 真撞:
        print("[FAIL] 发现 %d 组真撞号（同一编号对应多个不同用例目录）：" % len(真撞))
        for 号, 们 in 真撞:
            print("  #%s:" % 号)
            for 名 in 们:
                print("      %s" % 名)
        print("")
        print("处置（487/489/496 先例·先入者保留·后入者改号·六步·全文=tests/e2e/README.md）：")
        print("  1) python scripts/board_cli.py 领用例号 <用例名>   # 先领新号（必带名·号名配对唯一）")
        print("  2) git mv tests/e2e/<后入用例目录> tests/e2e/<新号>_<主题>")
        print("  3) 主.cn 首行注释改新号（目录内引用同改）")
        print("  4) 覆盖矩阵登记名同步（旧名→新名·board_cli 用例 <单元ID> <态> <新名>；旧名见改号流程）")
        print("  5) 豁免行同步：board_cli 豁免删 <旧名>（新名如需豁免再 豁免 <新名> <理由>）")
        print("  6) 重跑本门禁至绿（python scripts/check_case_numbers.py）")
        return 1
    print("[PASS] 本地无真撞号（成对与豁免之外的组=0）。")
    # 577 第二层：台账核验（同号异名/异号同名·跨分支可见面）
    结 = case_ledger.核验台账(取消豁免=tuple(豁免号))
    if 结["鉴权失败"]:
        print(f"[FAIL] {结['原因']}——台账核验无法进行"
              "（401/403 不得静默降级；修复令牌后重跑，与 task_board --ready 同口径）")
        return 1
    if not 结["台账可用"] and 结["本地用例数"]:
        源 = "读本地缓存" if "读本地缓存" in 结["原因"] else "跳过"
        print(f"[黄] 台账核验降级（{结['原因']}）——{源}；集成与云端全量为硬拦截点。")
    for 提示 in 结["提示们"][:8]:
        print(f"  [黄] {提示}")
    if 结["提示们"][8:]:
        print(f"  …（另 {len(结['提示们']) - 8} 条提示略）")
    for 陈旧 in 结["陈旧们"][:8]:
        print(f"  [陈旧] {陈旧}")
    if 结["陈旧们"][8:]:
        print(f"  …（另 {len(结['陈旧们']) - 8} 条陈旧行略）")
    if 结["冲突们"]:
        print(f"[FAIL] 台账核验冲突 {len(结['冲突们'])} 处（同号异名/异号同名）：")
        for x in 结["冲突们"]:
            print(f"  {x}")
        print("处置：后入者 `board_cli 领用例号 <用例名>` 领新号后 git mv 改号"
              "（台账已配名号=他人在用·勿复用）")
        return 1
    if a.strict and 结["提示们"]:
        print(f"[FAIL] --strict：{len(结['提示们'])} 条台账未配名提示按红处理"
              "（补录：python scripts/board_cli.py 补用例名 <号> <用例名>——"
              "811 存量补配通道·只对已发放未配名号）")
        return 1
    print("[PASS] 台账核验一致（或无对应行/断网降级）。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
