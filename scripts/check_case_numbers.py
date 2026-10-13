#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_case_numbers.py —— E2E 用例编号唯一性门禁（554·接续 487/489/496 治理链）。

病根（554 轮考古）：E2E 用例编号（tests/e2e/<编号>_<名称>/）无权威发号也无唯一性
校验——487/489/496 三轮纯人工全库盘点治理存量（42 组 46 目录改号），496 落地后
未装任何防再撞机制，10 分钟后并行任务 488 rebase 进来即再撞 752/753（存活一天
无门禁可拦）。本门禁机械拦增量：L1（gate_quick 静态段）/ win 全量（ci.ps1）/云端。

判据：同一数字编号对应多个用例目录 = 撞号。两类合法形态除外：
  ① 宿主+v2 成对（恰两目录，其一去掉编号前缀与 "v2_" 段后与另一全等——
     项目长期约定 NN_主题 / NN_v2_主题 共号成对）；
  ② 显式豁免号（下表·496 口径留档：早期功能分节/宿主与 v2 合用一号）。

处置（照 487/489/496 先例）：先入者保留·后入者领新号改号——
  python scripts/board_cli.py 领用例号        # 服务端原子发号
改号同步面：目录改名 + 主.cn 首行注释 + 服务端覆盖矩阵登记名 + 全库引用。

退出码：0=通过（无真撞号）；1=发现真撞号（CI 红）。
用法：python scripts/check_case_numbers.py
"""
import os
import re
import sys

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
    """去掉编号前缀与 v2_ 段：83_v2_交集_全私有类 → 交集_全私有类。"""
    无名 = _去编号段(目录名)
    return 无名[3:] if 无名.startswith("v2_") else 无名


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
    组 = 扫描分组()
    真撞 = []
    for 号, 们 in sorted(组.items(), key=lambda kv: int(kv[0])):
        if len(们) == 1 or 号 in 豁免号:
            continue
        if len(们) == 2 and 是成对(们[0], 们[1]):
            continue
        真撞.append((号, 们))
    多号组数 = sum(1 for 们 in 组.values() if len(们) > 1)
    print("=== E2E 用例编号唯一性门禁（554·AGENTS §4 条文执行面）===")
    print("目录 %d·唯一编号 %d·多目录编号 %d 组（豁免 %d 号·成对与豁免之外的组=真撞号）"
          % (sum(len(v) for v in 组.values()), len(组), 多号组数, len(豁免号)))
    if not 真撞:
        print("[PASS] 无真撞号——全部多目录编号组均为宿主+v2 成对或 496 口径豁免。")
        return 0
    print("[FAIL] 发现 %d 组真撞号（同一编号对应多个不同用例目录）：" % len(真撞))
    for 号, 们 in 真撞:
        print("  #%s:" % 号)
        for 名 in 们:
            print("      %s" % 名)
    print("")
    print("处置（487/489/496 先例·先入者保留·后入者改号）：")
    print("  1) python scripts/board_cli.py 领用例号            # 领取新号（服务端原子发号）")
    print("  2) git mv tests/e2e/<后入用例目录> tests/e2e/<新号>_<主题>")
    print("  3) 同步：主.cn 首行注释 / 服务端覆盖矩阵登记名（board_cli 用例·豁免删）")
    print("  4) 重跑本门禁至绿（python scripts/check_case_numbers.py）")
    return 1


if __name__ == "__main__":
    sys.exit(main())
