#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 优化器差分 runner（支柱二·plans/026 §2.2·245-a）

对生成的 CN 程序做 -O0 vs -O3 编译运行输出逐字节比对（CSmith 同款
优化器差分）：分歧=优化器缺陷候选（立案+反证三件套）。

用法：
  python scripts/cnsmith_diff.py --dir target/cnsmith [--cn target/Debug/cn.exe]
"""
import argparse
import os
import subprocess
import sys


def run_one(cn, src, out_dir, level):
    """编译（level=优化旗标如 -O0/-O3）+运行，返回 (状态, 输出)。
状态：ok/build_err/run_err"""
    base = os.path.basename(src)[:-3]
    exe = os.path.join(out_dir, "%s_%s.exe" % (base, level.strip("-")))
    b = subprocess.run([cn, "build", src, "--target", "win-x64", level,
                        "--output", exe],
                       capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=120)
    if b.returncode != 0:
        return "build_err", ((b.stdout or "") + (b.stderr or ""))[:200]
    try:
        r = subprocess.run([exe], capture_output=True, text=True,
                           encoding="utf-8", errors="replace", timeout=20)
    except subprocess.TimeoutExpired:
        return "run_err", "timeout"
    if r.returncode != 0:
        return "run_err", "rc=%d" % r.returncode
    return "ok", r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="target/cnsmith")
    ap.add_argument("--cn", default="target/Debug/cn.exe")
    ap.add_argument("--out", default="target/cnsmith/work")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    srcs = sorted(f for f in os.listdir(a.dir) if f.endswith(".cn"))
    n_ok = n_diff = n_berr = n_rerr = 0
    diffs = []
    for src_name in srcs:
        src = os.path.join(a.dir, src_name)
        s0, o0 = run_one(a.cn, src, a.out, "-O0")
        if s0 != "ok":
            n_berr += 1 if s0 == "build_err" else 0
            n_rerr += 1 if s0 == "run_err" else 0
            continue
        s3, o3 = run_one(a.cn, src, a.out, "-O3")
        if s3 != "ok":
            n_rerr += 1
            diffs.append((src_name, "O3 运行失败: " + o3))
            continue
        if o0 != o3:
            n_diff += 1
            diffs.append((src_name, "输出分歧 O0/O3"))
        else:
            n_ok += 1
    print("=== CN-Smith 优化器差分采样 ===")
    print("总数 %d ｜ 一致 %d ｜ 分歧 %d ｜ 编译失败 %d ｜ 运行失败 %d"
          % (len(srcs), n_ok, n_diff, n_berr, n_rerr))
    for name, why in diffs[:10]:
        print("  分歧:", name, "——", why)
    if diffs:
        with open(os.path.join(a.out, "分歧清单.txt"), "w",
                  encoding="utf-8", newline="") as f:
            for name, why in diffs:
                f.write("%s: %s\n" % (name, why))
    sys.exit(1 if (n_diff or n_rerr) else 0)


if __name__ == "__main__":
    main()
