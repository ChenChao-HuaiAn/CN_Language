#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""文件级行数门禁（报告模式）——G 区 T1 收口第一步（538-a）。

背景：§3 规则「每个源文件 ≤1000 行」——check_fn_length.py 只测函数级，
文件级零门禁（T1 实测 8 文件超限）。本脚本=文件级检测第一步：
  - 默认报告模式：扫描全仓 .cpp/.hpp/.cn 输出超限清单（不拦截）；
  - --strict：超限即退出码 1（二步拦截模式——待超限文件清零后由门禁登记启用）。

口径：与 check_fn_length.py 一致（统计物理行数·含注释与空行）。
范围：src/（宿主）+CN语言编译器v2/（v2 自举树）。
"""
import argparse
import sys
from pathlib import Path

THRESHOLD = 1000  # §3 规则：每个源文件 ≤1000 行
SUFFIXES = (".cpp", ".hpp", ".cn")
SCAN_ROOTS = ("src", "CN语言编译器v2")


def 超限清单(仓库根: Path, 阈值: int):
    清单 = []
    for 根 in SCAN_ROOTS:
        根目录 = 仓库根 / 根
        if not 根目录.exists():
            continue
        for f in sorted(根目录.rglob("*")):
            if f.suffix not in SUFFIXES:
                continue
            try:
                行数 = sum(1 for _ in f.open(encoding="utf-8", errors="replace"))
            except OSError:
                continue
            if 行数 > 阈值:
                清单.append((行数, f.relative_to(仓库根).as_posix()))
    清单.sort(reverse=True)
    return 清单


def main():
    ap = argparse.ArgumentParser(description="文件级行数门禁（报告模式）")
    ap.add_argument("--root", default=".", help="仓库根目录")
    ap.add_argument("--threshold", type=int, default=THRESHOLD)
    ap.add_argument("--strict", action="store_true",
                    help="拦截模式：超限即退出码 1（二步启用·当前报告模式默认不拦）")
    args = ap.parse_args()

    仓库根 = Path(args.root).resolve()
    清单 = 超限清单(仓库根, args.threshold)

    print(f"== check_file_length（T1·538-a 报告模式）==")
    print(f"阈值 {args.threshold} 行 ｜ 范围 {', '.join(SCAN_ROOTS)} ｜ 超限 {len(清单)} 文件")
    for 行数, 路径 in 清单:
        print(f"  {行数:5d}  {路径}")
    if 清单:
        print("[报告] 存在超限文件（报告模式不拦截；--strict 拦截待拆分清零后启用）")
        return 1 if args.strict else 0
    print("[OK] 无超限文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
