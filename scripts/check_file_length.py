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


基线文件 = Path("scripts/file_length_baseline.json")


def 冻结线检查(仓库根: Path, 阈值: int, 更新基线: bool) -> int:
    """242（2026-10-07）冻结线：存量超标文件禁净增长，减行收基线——「不再更坏」先于「清零」。

    基线=scripts/file_length_baseline.json {路径: 行数}；任一文件超基线记录=红（净增长拦）；
    新超标文件=红；基线不存在的文件超阈=红。--update-baseline 以当前实测重写基线（减行后收）。
    """
    import json
    基线路径 = 仓库根 / 基线文件
    实测 = {路径: 行数 for 行数, 路径 in 超限清单(仓库根, 阈值)}
    if 更新基线:
        基线路径.write_text(json.dumps(实测, ensure_ascii=False, indent=1, sort_keys=True),
                            encoding="utf-8")
        print(f"[冻结线] 基线已更新（{len(实测)} 文件·scripts/file_length_baseline.json）")
        return 0
    if not 基线路径.exists():
        print(f"[冻结线·红] 基线缺失：{基线文件}（首次用 --update-baseline 生成）")
        return 1
    基线 = json.loads(基线路径.read_text(encoding="utf-8"))
    红们 = []
    for 路径 in sorted(set(基线) | set(实测)):
        旧, 新 = 基线.get(路径), 实测.get(路径)
        if 新 is None:
            红们 = 红们  # 超标文件降到阈内=好事，基线由 --update-baseline 收取
        elif 旧 is None:
            红们.append(f"新超标 {新:5d}  {路径}（冻结线禁新增超标文件）")
        elif 新 > 旧:
            红们.append(f"净增长 {旧}→{新}  {路径}（冻结线禁超限文件增长）")
    print(f"== check_file_length --freeze（242 冻结线）== 基线 {len(基线)} 文件")
    if 红们:
        for r in 红们:
            print(f"  [×] {r}")
        print(f"[冻结线·红] {len(红们)} 项净增长/新增超标——拆分前禁止更坏")
        return 1
    print("[冻结线·绿] 无净增长（超标存量只减不增·--update-baseline 收减行）")
    return 0


def main():
    ap = argparse.ArgumentParser(description="文件级行数门禁（报告/冻结线模式）")
    ap.add_argument("--root", default=".", help="仓库根目录")
    ap.add_argument("--threshold", type=int, default=THRESHOLD)
    ap.add_argument("--strict", action="store_true",
                    help="拦截模式：超限即退出码 1（二步启用·当前报告模式默认不拦）")
    ap.add_argument("--freeze", action="store_true",
                    help="242 冻结线：对基线查净增长（超标文件禁增长/禁新增·减行过）")
    ap.add_argument("--update-baseline", action="store_true",
                    help="以当前实测重写冻结线基线（减行后收基线·配合 --freeze 使用）")
    args = ap.parse_args()

    仓库根 = Path(args.root).resolve()
    if args.freeze or args.update_baseline:
        return 冻结线检查(仓库根, args.threshold, args.update_baseline)

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
