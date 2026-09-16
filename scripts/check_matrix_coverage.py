#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""构件面检测台账完成度门禁（B8 制度·plans/026 §2.10·262-a）

校验 tests/matrix/面台账.md 的未测行（⬜）数相对基线**只减不增**——
新增台账行必须同轮带探针记录（🔶）或转正 E2E（✅），否则 FAIL。
基线存于 tests/matrix/覆盖基线.txt（上轮 ⬜ 数·脚本自动维护）。

用法：
  python3 scripts/check_matrix_coverage.py             # 常规校验（提交前自检）
  python3 scripts/check_matrix_coverage.py --init      # 首次建基线（或清零重建）
  python3 scripts/check_matrix_coverage.py --quiet     # 只输出覆盖数（供 plans/021 戳引用）
"""
import argparse
import os
import sys

LEDGER = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      "tests", "matrix", "面台账.md")
BASELINE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "tests", "matrix", "覆盖基线.txt")
MARKERS = ("⬜", "🔶", "✅")


def parse_ledger():
    """返回 (总行数, {状态: 行数})；台账缺失或无数据行时抛 RuntimeError。"""
    if not os.path.exists(LEDGER):
        raise RuntimeError("面台账不存在: %s" % LEDGER)
    counts = {m: 0 for m in MARKERS}
    total = 0
    with open(LEDGER, encoding="utf-8") as f:
        for line in f:
            if not line.lstrip().startswith("|"):
                continue
            hit = [m for m in MARKERS if m in line]
            # 数据行=恰好含一个状态符号（表头/分隔行不含）
            if len(hit) == 1:
                total += 1
                counts[hit[0]] += 1
    if total == 0:
        raise RuntimeError("面台账无数据行（解析到 0 行）: %s" % LEDGER)
    return total, counts


def read_baseline():
    try:
        with open(BASELINE, encoding="utf-8") as f:
            return int(f.read().strip())
    except (OSError, ValueError):
        return None


def write_baseline(value):
    with open(BASELINE, "w", encoding="utf-8") as f:
        f.write("%d\n" % value)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--init", action="store_true", help="首次建基线（未测数=当前值）")
    ap.add_argument("--quiet", action="store_true", help="只输出覆盖数一行")
    args = ap.parse_args()

    try:
        total, counts = parse_ledger()
    except RuntimeError as e:
        print("[面覆盖门禁] FAIL：%s" % e)
        return 1

    untested = counts["⬜"]
    covered = counts["✅"] + counts["🔶"]
    summary = "面覆盖 %d/%d（未测 %d·已探针 %d·已转正 %d）" % (
        covered, total, untested, counts["🔶"], counts["✅"])

    if args.init:
        write_baseline(untested)
        if not args.quiet:
            print("[面覆盖门禁] 基线已建立：%s" % summary)
        return 0

    baseline = read_baseline()
    if baseline is None:
        print("[面覆盖门禁] FAIL：基线缺失（%s）——先跑 --init 建基线" % BASELINE)
        return 1

    if untested > baseline:
        print("[面覆盖门禁] FAIL：%s——未测行数 %d > 基线 %d（B8 纪律：新增台账行"
              "必须同轮带探针记录或转正 E2E）" % (summary, untested, baseline))
        return 1

    write_baseline(untested)
    if not args.quiet:
        print("[面覆盖门禁] PASS：%s（基线 %d → %d）" % (summary, baseline, untested))
    else:
        print(summary)
    return 0


if __name__ == "__main__":
    sys.exit(main())
