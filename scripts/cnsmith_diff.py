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

# 306-a 防线加固：子进程虚拟内存上限（防病态样本/编译器进程内存失控触发系统
#   OOM 连坐宿主会话——2026-09-17 28.7GB OOM 实证）。
# 312-b win 兼容修复：resource 为 POSIX 专属模块（Windows 无·306-a 首发后 win
#   全族差分/回归轨 ModuleNotFoundError 瘫痪）；subprocess preexec_fn 亦为
#   POSIX 专属参数——win 分支跳过（Job Object 级上限留后续防线轮），linux
#   行为逐字节不变。
try:
    import resource
    _HAS_RESOURCE = True
except ImportError:
    _HAS_RESOURCE = False


def 子进程内存上限(gb=6):
    def _limit():
        resource.setrlimit(resource.RLIMIT_AS, (gb * 1024 ** 3,) * 2)
    return _limit


def 子进程防线参数():
    """POSIX=内存上限 preexec_fn；Windows=None（无对应原语·跳过）。"""
    if _HAS_RESOURCE:
        return {"preexec_fn": 子进程内存上限()}
    return {}


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


信号名 = {-4: "SIGILL", -6: "SIGABRT", -8: "SIGFPE", -9: "SIGKILL",
          -11: "SIGSEGV", -13: "SIGPIPE", -15: "SIGTERM", -24: "SIGXCPU"}


def 是崩溃(rc):
    """信号死亡（POSIX 负值）或 shell 形态 128+信号——与干净诊断（rc≥1）区分。"""
    return rc < 0 or rc >= 128


def run_one(cn, src, out_dir, level, target=None):
    """编译（level=优化旗标如 -O0/-O3）+运行，返回 (状态, 输出)。
状态：ok/build_err/compiler_crash/run_err
判据（786-a 根治「rc 一刀切」）：
  - 编译器进程信号死亡 → compiler_crash（T6 族「负向必诊断不崩溃」防线常设化——
    崩溃混入 build_err 诊断桶=防线眼盲，786-a 前 t71 族假命中同源）；
  - 编译诊断失败 rc≥1 → build_err（干净拒绝）；
  - 产物运行信号死亡/超时 → run_err；
  - 运行 rc≥0（含主函数返回非 0）→ ok，比对载荷=stdout+[rc=N]
    （返回值纳入 O0/O3 差分：合法程序返回非 0 不再误报 run_err〔t71 linux 假命中
    实证 rc=1=主函数返回值〕，且「O0/O3 返回值不一致」从此成为可检出的分歧面）。"""
    base = os.path.basename(src)[:-3]
    后缀 = ".exe" if (target or 探测目标平台()) == "win-x64" else ""
    exe = os.path.join(out_dir, "%s_%s%s" % (base, level.strip("-"), 后缀))
    b = subprocess.run([cn, "build", src, "--target", 探测目标平台(target), level,
                        "--output", exe],
                       capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=120,
                       **子进程防线参数())
    if b.returncode != 0:
        if 是崩溃(b.returncode):
            return "compiler_crash", "编译器崩溃 rc=%d %s: %s" % (
                b.returncode, 信号名.get(b.returncode, ""),
                ((b.stdout or "") + (b.stderr or ""))[:160])
        return "build_err", ((b.stdout or "") + (b.stderr or ""))[:200]
    try:
        r = subprocess.run([exe], capture_output=True, text=True,
                           encoding="utf-8", errors="replace", timeout=20,
                           **子进程防线参数())
    except subprocess.TimeoutExpired:
        return "run_err", "timeout"
    if 是崩溃(r.returncode):
        return "run_err", "rc=%d %s" % (r.returncode, 信号名.get(r.returncode, ""))
    return "ok", "%s[rc=%d]" % (r.stdout, r.returncode)


def run_sample(cn, src, out_dir, target):
    """单样本全链：O0 编译运行 + O3 编译运行 + 输出比对。
返回 (样本名, 类别, 详情)；类别 ∈ {ok, diff, build_err, compiler_crash, run_err}。"""
    name = os.path.basename(src)
    s0, o0 = run_one(cn, src, out_dir, "-O0", target)
    if s0 != "ok":
        return name, s0, o0
    s3, o3 = run_one(cn, src, out_dir, "-O3", target)
    if s3 != "ok":
        # O0 过而 O3 阶段失败：保留 O3 真实类别（build_err=「O0 过 O3 不过」的
        #   优化器引入编译失败=高价值信号，不再错标 run_err·compiler_crash 同理）
        return name, s3, "O3 阶段: " + o3
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
        print("总数 0 ｜ 一致 0 ｜ 分歧 0 ｜ 编译失败 0 ｜ 运行失败 0 ｜ 编译器崩溃 0")
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
    n_ccrash = sum(1 for _, k, _ in results if k == "compiler_crash")
    diffs = [(n, k, d) for n, k, d in results if k != "ok"]
    print("=== CN-Smith 优化器差分采样 ===")
    print("总数 %d ｜ 一致 %d ｜ 分歧 %d ｜ 编译失败 %d ｜ 运行失败 %d ｜ 编译器崩溃 %d"
          % (len(srcs), n_ok, n_diff, n_berr, n_rerr, n_ccrash))
    for name, kind, why in diffs[:10]:
        print("  分歧 [%s]:" % kind, name, "——", why)
    if diffs:
        # T28②根治（303-a）：清单行携带结构化类别标签 [diff/build_err/run_err]——
        #   原格式「名: 详情」把类别丢在详情文本里（run_err 详情=rc=-8 不含
        #   「运行失败」字样），daemon 只能文本反推→rc=-8 全部误归 diff/。
        前缀表 = {"build_err": "编译失败: ", "run_err": "运行失败: ",
                  "compiler_crash": "编译器崩溃: "}
        with open(os.path.join(a.out, "分歧清单.txt"), "w",
                  encoding="utf-8", newline="") as f:
            for name, kind, why in diffs:
                前缀 = 前缀表.get(kind, "")
                f.write("%s [%s]: %s%s\n" % (name, kind, 前缀, why))
    sys.exit(1 if (n_diff or n_rerr or n_ccrash) else 0)


if __name__ == "__main__":
    main()
