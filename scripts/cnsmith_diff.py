#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 优化器差分 runner（支柱二·plans/026 §2.2·245-a）

对生成的 CN 程序做 -O0 vs -O3 编译运行输出逐字节比对（CSmith 同款
优化器差分）：分歧=优化器缺陷候选（立案+反证三件套）。

并行架构（260-a·用户令「每次运行太慢→设计并行」）：
  - 样本级并行：ThreadPoolExecutor（子进程等待期释放 GIL，无序列化开销）
  - --jobs N（默认=CPU 核数；1=串行调试口径）
  - 预热：并行前串行编译首个样本——建立运行时 obj 缓存（target/ 固定
    落点·cache miss 并发写=踩踏），之后并行阶段全部命中只读
  - 每样本产物名唯一（<stem>_O0/_O3）＝中间 .asm/.obj 天然隔离
  - 结果按样本名排序聚合＝输出与串行口径逐字节一致（可复现）

用法：
  python scripts/cnsmith_diff.py --dir target/cnsmith [--cn target/Debug/cn.exe]
                                 [--jobs 8]
"""
import argparse
import concurrent.futures
import os
import subprocess
import sys
import threading
import time


def 探测目标平台(target=None):
    """按架构探测构建目标（256-a 跨平台化：原硬编码 win-x64=家机口径）。"""
    if target:
        return target
    import platform as _p
    m = _p.machine().lower()
    if os.name == "nt":
        return "win-x64"
    if m in ("aarch64", "arm64"):
        return "linux-arm64"
    return "linux-x86_64"


def run_one(cn, src, out_dir, level, target=None):
    """编译（level=优化旗标如 -O0/-O3）+运行，返回 (状态, 输出)。
状态：ok/build_err/run_err"""
    base = os.path.basename(src)[:-3]
    后缀 = ".exe" if (target or 探测目标平台()) == "win-x64" else ""
    exe = os.path.join(out_dir, "%s_%s%s" % (base, level.strip("-"), 后缀))
    b = subprocess.run([cn, "build", src, "--target", 探测目标平台(target), level,
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


def run_sample(cn, src, out_dir, target):
    """单样本全链：O0 编译运行 + O3 编译运行 + 输出比对。
返回 (样本名, 类别, 详情)；类别 ∈ {ok, diff, build_err, run_err}。"""
    name = os.path.basename(src)
    s0, o0 = run_one(cn, src, out_dir, "-O0", target)
    if s0 != "ok":
        return name, ("build_err" if s0 == "build_err" else "run_err"), o0
    s3, o3 = run_one(cn, src, out_dir, "-O3", target)
    if s3 != "ok":
        return name, "run_err", "O3 运行失败: " + o3
    if o0 != o3:
        return name, "diff", "输出分歧 O0/O3"
    return name, "ok", ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="target/cnsmith")
    ap.add_argument("--cn", default="target/Debug/cn.exe")
    ap.add_argument("--out", default="target/cnsmith/work")
    ap.add_argument("--target", default=None, help="构建目标平台（默认按架构探测）")
    ap.add_argument("--jobs", type=int, default=0,
                    help="并行度（0=CPU 核数，1=串行）")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    srcs = sorted(f for f in os.listdir(a.dir) if f.endswith(".cn"))
    jobs = a.jobs if a.jobs > 0 else (os.cpu_count() or 4)
    target = 探测目标平台(a.target)

    t0 = time.time()
    results = []
    if not srcs:
        print("=== CN-Smith 优化器差分采样 ===")
        print("总数 0 ｜ 一致 0 ｜ 分歧 0 ｜ 编译失败 0 ｜ 运行失败 0")
        sys.exit(0)

    # 预热：串行编译首个样本——建立运行时 obj 缓存（target/ 固定落点，
    #   cache miss 并发写会踩踏；预热后并行阶段全部命中只读）
    done = 0
    lock = threading.Lock()

    def 计数():
        nonlocal done
        with lock:
            done += 1
            return done

    # 预热用 O0 单级别即可建缓存（O3 复用同一运行时 obj）
    run_one(a.cn, os.path.join(a.dir, srcs[0]), a.out, "-O0", target)

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futs = {}
        for name in srcs:
            src = os.path.join(a.dir, name)
            futs[pool.submit(run_sample, a.cn, src, a.out, target)] = name
        for fut in concurrent.futures.as_completed(futs):
            results.append(fut.result())
            n = 计数()
            if jobs > 1 and n % 50 == 0:
                print("  进度 %d/%d（%.1fs）" % (n, len(srcs), time.time() - t0),
                      flush=True)

    results.sort(key=lambda r: r[0])
    n_ok = sum(1 for _, k, _ in results if k == "ok")
    n_diff = sum(1 for _, k, _ in results if k == "diff")
    n_berr = sum(1 for _, k, _ in results if k == "build_err")
    n_rerr = sum(1 for _, k, _ in results if k == "run_err")
    diffs = [(n, k, d) for n, k, d in results if k != "ok"]
    print("=== CN-Smith 优化器差分采样 ===")
    print("总数 %d ｜ 一致 %d ｜ 分歧 %d ｜ 编译失败 %d ｜ 运行失败 %d"
          % (len(srcs), n_ok, n_diff, n_berr, n_rerr))
    for name, kind, why in diffs[:10]:
        print("  分歧 [%s]:" % kind, name, "——", why)
    if diffs:
        # T28②根治（303-a）：清单行携带结构化类别标签 [diff/build_err/run_err]——
        #   原格式「名: 详情」把类别丢在详情文本里（run_err 详情=rc=-8 不含
        #   「运行失败」字样），daemon 只能文本反推→rc=-8 全部误归 diff/。
        with open(os.path.join(a.out, "分歧清单.txt"), "w",
                  encoding="utf-8", newline="") as f:
            for name, kind, why in diffs:
                前缀 = {"build_err": "编译失败: ", "run_err": "运行失败: "}.get(kind, "")
                f.write("%s [%s]: %s%s\n" % (name, kind, 前缀, why))
    sys.exit(1 if (n_diff or n_rerr) else 0)


if __name__ == "__main__":
    main()
