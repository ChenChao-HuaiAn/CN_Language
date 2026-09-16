#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 采样轮自治 daemon（S1·plans/026 §2.9 用户已批「按这个办」）。

五步自治循环（§2.9 协议）：
  ① git fetch gitcode → 有新提交则拉取 + 重建编译器（cmake target/build）
  ② 生成 N 个新样本（轮换种子=日期+轮次）+ 回归轨（命中样本库全量重跑=防修复复发）
  ③ 差分（复用 scripts/cnsmith_diff.py -O0 vs -O3·并行）
  ④ 命中样本存档 tests/cnsmith_hits/<类别>/（防 target/ 清场丢失）+ 归类
  ⑤ 三件套日志追加 tests/cnsmith_hits/采样日志.md（基线 commit/采样量命中数/命中归类）
    —— 此日志即 M3 计时秒表（「连续 4 周零新类别」以首条日志起算）。

用法：
  python3 scripts/cnsmith_daemon.py --rounds 3 --samples 200   # 3 轮·每轮 200 新样本
  python3 scripts/cnsmith_daemon.py --once                     # 单轮（=--rounds 1）
  轮间睡 --sleep 秒（默认 600·CPU 密集型给开发轮让路）。

设计要点（§2.9）：
  - 三机通用：构建目标按架构探测（cnsmith_diff 已跨平台化）
  - daemon 只写 tests/cnsmith_hits/（免锁验证轮写面）与 target/、/tmp——
    **不直接写 plans/021**：排班追加由 AI 轮/用户「看日志+审排班」折入（§2.9 设计）
  - 回归轨=命中库全量：新编译器上 diff 消失=已修复（日志记「已修」）；仍在=未修复复发
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time

根 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
库 = os.path.join(根, "tests", "cnsmith_hits")
日志 = os.path.join(库, "采样日志.md")
工作 = os.path.join(根, "target", "cnsmith")
差异清单 = os.path.join(工作, "work", "分歧清单.txt")
类别集 = ("diff", "build_err", "run_err")


def 跑(cmd, cwd=根, timeout=3600):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)


def 远程头():
    跑(["git", "fetch", "gitcode", "develop"], timeout=120)
    r = 跑(["git", "rev-parse", "gitcode/develop"])
    return r.stdout.strip()


def 本地头():
    return 跑(["git", "rev-parse", "HEAD"]).stdout.strip()


def 拉取并重建():
    """① 有新提交则 ff 拉取 + 重建编译器。返回 (当前commit, 动作说明)。"""
    说明 = "无更新"
    远 = 远程头()
    if 远 and 远 != 本地头():
        r = 跑(["git", "pull", "--ff-only", "gitcode", "develop"], timeout=300)
        if r.returncode != 0:
            return 本地头(), "拉取失败（保持本地基线）:" + (r.stderr.strip().splitlines() or ["?"])[0][:60]
        说明 = "已拉取至 " + 远[:9]
        构建 = os.path.join(根, "target", "build")
        if not os.path.exists(os.path.join(构建, "CMakeCache.txt")):
            跑(["cmake", "-S", ".", "-B", "target/build"], timeout=1800)
        rb = 跑(["cmake", "--build", "target/build", "--config", "Debug", "-j", "8"], timeout=3600)
        说明 += "；重建" + ("成功" if rb.returncode == 0 else "失败(沿用旧编译器)")
    return 本地头(), 说明


def 新样本轨(轮, 数量, 基线):
    """② 新样本（轮换种子）+ 回归轨（命中库全量拷入）。返回样本目录与回归数。"""
    # 清空上轮工作样本（保留 work/ 由 diff 管理）
    for f in os.listdir(工作):
        p = os.path.join(工作, f)
        if f.endswith(".cn"):
            os.remove(p)
    种子 = int(time.strftime("%Y%m%d")) * 100 + 轮
    r = 跑([sys.executable, "scripts/cnsmith_gen.py", "--seed", str(种子),
            "--count", str(数量), "--out", "target/cnsmith"], timeout=600)
    if r.returncode != 0:
        print("  生成失败:", (r.stderr or r.stdout)[:120])
    回归数 = 0
    if os.path.isdir(库):
        for cat in 类别集:
            d = os.path.join(库, cat)
            if not os.path.isdir(d):
                continue
            for f in os.listdir(d):
                if f.endswith(".cn"):
                    shutil.copy(os.path.join(d, f), os.path.join(工作, f))
                    回归数 += 1
    return 种子, 回归数


def 差分(编译器):
    """③ 调 cnsmith_diff（并行）。返回 (汇总行列表, 分歧[(名, 类别, 说明)])。"""
    r = 跑([sys.executable, "scripts/cnsmith_diff.py", "--dir", "target/cnsmith",
            "--cn", 编译器, "--out", "target/cnsmith/work", "--jobs", "8"], timeout=7200)
    汇总 = [l for l in (r.stdout or "").splitlines() if l.startswith(("=== ", "总数"))]
    分歧 = []
    if os.path.exists(差异清单):
        for line in open(差异清单, encoding="utf-8"):
            m = re.match(r"(\S+\.cn)[:：]\s*(.*)", line.strip())
            if m:
                名, 说明 = m.group(1), m.group(2)
                类别 = "build_err" if "编译失败" in 说明 else ("run_err" if "运行失败" in 说明 else "diff")
                分歧.append((名, 类别, 说明))
    return 汇总, 分歧


def 存档(分歧, 基线, 轮):
    """④ 命中样本存档入库（按类别·带 .meta）。返回新存档数与「已修」数。"""
    os.makedirs(库, exist_ok=True)
    新存 = 0
    已修 = 0
    for 名, 类别, 说明 in 分歧:
        源 = os.path.join(工作, 名)
        d = os.path.join(库, 类别)
        os.makedirs(d, exist_ok=True)
        目标 = os.path.join(d, 名)
        已有 = os.path.exists(目标)
        if os.path.exists(源):
            shutil.copy(源, 目标)
            if not 已有:
                with open(目标 + ".meta", "w", encoding="utf-8") as f:
                    f.write("基线: %s\n轮: %d\n类别: %s\n说明: %s\n" % (基线[:9], 轮, 类别, 说明))
                新存 += 1
        else:
            已修 += 1  # 回归轨样本本轮未复现 = 已修复/已变化
    return 新存, 已修


def 写日志(轮, 基线, 说明, 种子, 新样本数, 回归数, 汇总, 新存, 已修, 分歧, 用时):
    """⑤ 三件套日志（M3 秒表）。"""
    os.makedirs(库, exist_ok=True)
    首次 = not os.path.exists(日志)
    with open(日志, "a", encoding="utf-8") as f:
        if 首次:
            f.write("# CN-Smith 采样日志（S1·三件套·M3 计时秒表）\n\n"
                    "> 判据（plans/026 M3）：连续 4 周零新缺陷类别——以本日志首条起算。\n"
                    "> daemon 只写本日志与命中库；排班追加由 AI 轮/用户审阅后折入 plans/021。\n\n")
        f.write("## 轮 %d（%s）\n" % (轮, time.strftime("%Y-%m-%d %H:%M:%S")))
        f.write("- 基线：%s（%s）\n" % (基线[:9], 说明))
        f.write("- 采样：%d 新样本（seed=%d）+ 回归轨 %d\n" % (新样本数, 种子, 回归数))
        f.write("- 差分：%s\n" % (汇总[1] if len(汇总) > 1 else "无汇总"))
        if 分歧:
            f.write("- 命中归类：\n")
            for 名, 类别, 说明 in 分歧[:10]:
                f.write("  - %s [%s] %s\n" % (名, 类别, 说明[:80]))
            if len(分歧) > 10:
                f.write("  - …共 %d 条\n" % len(分歧))
        else:
            f.write("- 命中归类：零命中\n")
        f.write("- 存档：新存 %d·回归轨未复现(已修/已变化) %d·用时 %.1fs\n\n" % (新存, 已修, 用时))


def 主():
    ap = argparse.ArgumentParser(description="CN-Smith 采样轮自治 daemon（S1）")
    ap.add_argument("--rounds", type=int, default=1, help="循环轮数（0=无限·默认 1）")
    ap.add_argument("--samples", type=int, default=200, help="每轮新样本数")
    ap.add_argument("--sleep", type=int, default=600, help="轮间睡眠秒数")
    a = ap.parse_args()

    编译器 = os.path.join(根, "target", "cn")
    if not os.path.exists(编译器):
        print("编译器不存在：target/cn（先构建）")
        return 1
    os.makedirs(工作, exist_ok=True)

    轮 = 0
    while a.rounds == 0 or 轮 < a.rounds:
        轮 += 1
        t0 = time.time()
        print("=== S1 采样轮 %d ===" % 轮, flush=True)
        基线, 说明 = 拉取并重建()                       # ①
        种子, 回归数 = 新样本轨(轮, a.samples, 基线)      # ②
        新样本数 = sum(1 for f in os.listdir(工作) if f.endswith(".cn")) - 回归数
        汇总, 分歧 = 差分(编译器)                        # ③
        新存, 已修 = 存档(分歧, 基线, 轮)                # ④
        写日志(轮, 基线, 说明, 种子, 新样本数, 回归数, 汇总, 新存, 已修, 分歧, time.time() - t0)  # ⑤
        for l in 汇总:
            print("  " + l)
        print("  存档新 %d·回归未复现 %d·日志已记（%.1fs）" % (新存, 已修, time.time() - t0))
        if a.rounds == 0 or 轮 < a.rounds:
            time.sleep(a.sleep)
    return 0


if __name__ == "__main__":
    sys.exit(主())
