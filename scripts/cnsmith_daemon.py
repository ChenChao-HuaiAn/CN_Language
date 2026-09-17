#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 采样轮自治 daemon（S1·plans/026 §2.9 用户已批「按这个办」）。

五步自治循环（§2.9 协议）：
  ① git fetch gitcode → 有新提交则拉取 + 重建编译器（cmake target/build）
  ② 生成 N 个新样本（种子=时间戳到秒·重启不撞号）+ 回归轨（命中样本库全量重跑=防修复复发）
  ③ 差分（复用 scripts/cnsmith_diff.py -O0 vs -O3·并行）
  ④ 命中样本存档 tests/cnsmith_hits/<类别>/（防 target/ 清场丢失）+ 归类
  ⑤ 三件套日志（M3 计时秒表）——**按月分文件+变化时记/汇总记法（286-a·S1b 根治）**：
    - 月文件 tests/cnsmith_hits/日志/YYYY-MM.md；索引指针=tests/cnsmith_hits/采样日志.md（固定 ≤15 行）
    - 连续「零新存·零已修·命中集合与上轮相同·基线无更新」的轮次合并为一行「汇总」
    - 任何变化（新命中/命中消失/已修/新存档/基线更新/编译或运行失败>0）独立成轮完整记录
    —— 单文件追加式旧记法在 ~50s/轮频率下日增 1~3 万行（286-a 用户质询立案），此记法为体积根治。

用法：
  python3 scripts/cnsmith_daemon.py --rounds 3 --samples 200   # 3 轮·每轮 200 新样本
  python3 scripts/cnsmith_daemon.py --once                     # 单轮（=--rounds 1）
  轮间睡 --sleep 秒（默认 600·CPU 密集型给开发轮让路）。

设计要点（§2.9）：
  - 三机通用：构建目标按架构探测（cnsmith_diff 已跨平台化）
  - daemon 只写 tests/cnsmith_hits/（免锁验证轮写面）与 target/、/tmp——
    **不直接写 plans/021**：排班追加由 AI 轮/用户「看日志+审排班」折入（§2.9 设计）
  - 回归轨=命中库全量：回归样本本轮未命中=已修/已变化（变化轮显式列出·文件留在库中继续防复发）
  - daemon 重启后首轮命中集合未知→完整记录一轮（重建去重基线·可接受）
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
月目录 = os.path.join(库, "日志")
指针文件 = os.path.join(库, "采样日志.md")
工作 = os.path.join(根, "target", "cnsmith")
差异清单 = os.path.join(工作, "work", "分歧清单.txt")
类别集 = ("diff", "build_err", "run_err")

指针文本 = """# CN-Smith 采样日志·索引指针（S1·三件套·M3 计时秒表）

> 本文件是固定指针，不再追加轮次——日志按月分文件：`tests/cnsmith_hits/日志/YYYY-MM.md`。
> 判据（plans/026 M3）：连续 4 周零新缺陷类别——按月文件顺序连续起算（自 2026-09 存量迁移起）。
> 记法（286-a·S1b）：「变化时记+定时汇总」——无变化轮次合并为汇总行；
> 新命中/命中消失/已修/新存档/基线更新/编译或运行失败>0 逐轮完整记录。
> daemon 只写月文件与命中库；排班追加由 AI 轮/用户审阅后折入 plans/021。
"""


def 跑(cmd, cwd=根, timeout=3600):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)


def 远程头():
    跑(["git", "fetch", "gitcode", "develop"], timeout=120)
    r = 跑(["git", "rev-parse", "gitcode/develop"])
    return r.stdout.strip()


def 本地头():
    return 跑(["git", "rev-parse", "HEAD"]).stdout.strip()


def 拉取并重建():
    """① 有新提交则 ff 拉取 + 重建编译器。返回 (当前commit, 动作说明)。

    重建后核验 target/cn 新鲜度（mtime 晚于重建开始·假重建陷阱防线：增量构建未真产出时
    沿用旧编译器=滞后基线假数据，家机 492e976 建议）。"""
    说明 = "无更新"
    远 = 远程头()
    if 远 and 远 != 本地头():
        r = 跑(["git", "pull", "--ff-only", "gitcode", "develop"], timeout=300)
        if r.returncode != 0:
            return 本地头(), "拉取失败（保持本地基线）:" + (r.stderr.strip().splitlines() or ["?"])[0][:60]
        说明 = "已拉取至 " + 远[:9]
        t建 = time.time()
        构建 = os.path.join(根, "target", "build")
        if not os.path.exists(os.path.join(构建, "CMakeCache.txt")):
            跑(["cmake", "-S", ".", "-B", "target/build"], timeout=1800)
        rb = 跑(["cmake", "--build", "target/build", "--config", "Debug", "-j", "8"], timeout=3600)
        if rb.returncode != 0:
            说明 += "；重建失败(沿用旧编译器=滞后基线!)"
        elif os.path.getmtime(os.path.join(根, "target", "cn")) * 1000 < t建:
            说明 += "；重建未更新 target/cn(假重建!沿用旧编译器)"
        else:
            说明 += "；重建成功(产物已核验新鲜)"
    return 本地头(), 说明


def 新样本轨(数量):
    """② 新样本（种子=yyMMddHHMMSS 到秒·重启/多轮不撞号=真「不同取样」）+ 回归轨（命中库全量拷入）。
    返回 (种子, 回归样本名列表)。"""
    for f in os.listdir(工作):
        p = os.path.join(工作, f)
        if f.endswith(".cn"):
            os.remove(p)
    种子 = int(time.strftime("%y%m%d%H%M%S"))
    r = 跑([sys.executable, "scripts/cnsmith_gen.py", "--seed", str(种子),
            "--count", str(数量), "--out", "target/cnsmith"], timeout=600)
    if r.returncode != 0:
        print("  生成失败:", (r.stderr or r.stdout)[:120])
    回归 = []
    if os.path.isdir(库):
        for cat in 类别集:
            d = os.path.join(库, cat)
            if not os.path.isdir(d):
                continue
            for f in sorted(os.listdir(d)):
                if f.endswith(".cn"):
                    shutil.copy(os.path.join(d, f), os.path.join(工作, f))
                    回归.append(f)
    return 种子, 回归


def 差分(编译器):
    """③ 调 cnsmith_diff（并行）。返回 (差分汇总文本, 分歧[(名, 类别, 说明)])。

    读清单前必删旧清单（家机 492e976 教训：diff 零分歧时不清清单→残留旧清单被误读=假命中）。"""
    if os.path.exists(差异清单):
        os.remove(差异清单)
    r = 跑([sys.executable, "scripts/cnsmith_diff.py", "--dir", "target/cnsmith",
            "--cn", 编译器, "--out", "target/cnsmith/work", "--jobs", "8"], timeout=7200)
    汇总行 = next((l for l in (r.stdout or "").splitlines() if l.startswith("总数")), "无汇总")
    分歧 = []
    if os.path.exists(差异清单):
        for line in open(差异清单, encoding="utf-8"):
            # T28②根治（303-a）：按清单行结构化标签 [diff/build_err/run_err] 分派——
            #   原实现靠详情文本含「运行失败」反推，rc=-8/timeout 详情不含该字样
            #   →必然落空误归 diff/+已归档样本每轮重建副本。
            m = re.match(r"(\S+\.cn)\s*\[(\w+)\]:\s*(.*)", line.strip())
            if m:
                名, 类别, 说明 = m.group(1), m.group(2), m.group(3)
                if 类别 not in 类别集 and 类别 != "ok":
                    类别, 说明 = "diff", "[%s] %s" % (类别, 说明)
                分歧.append((名, 类别, 说明))
    return 汇总行, 分歧


def 存档(分歧, 基线, 轮):
    """④ 命中样本存档入库（按类别·带 .meta）。返回新存档数。"""
    os.makedirs(库, exist_ok=True)
    新存 = 0
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
    return 新存


def _解析计数(差分文本):
    def 取(词):
        m = re.search(词 + r"\s*(\d+)", 差分文本)
        return int(m.group(1)) if m else 0
    return 取("一致"), 取("分歧"), 取("编译失败"), 取("运行失败")


class 日志写手:
    """⑤ 按月分文件+变化时记/汇总记法（S1b）。"""

    def __init__(self):
        self.当前月 = None
        self.当前路径 = None
        self.汇总 = None          # 无变化序列缓冲
        self.上轮命中 = None       # set((名,类别))；None=未知（重启首轮）
        self.上轮未命中回归 = None  # set(名)——T28③根治（303-a）：回归轨中未命中的稳态全集，
        #   消失回归=本轮未命中−上轮未命中（增量）。原实现直接用「回归名单−命中名」全集，
        #   已修样本永远留库防复发→该集合恒非空→「not 消失回归」恒假→无变化判据永不成立
        #   →每轮退化全量记录（月日志爆 5000 红线根因）。None=重启首轮（基线化不报变化）。

    def _切月(self, st):
        月 = time.strftime("%Y-%m", st)
        if 月 != self.当前月:
            self.当前月 = 月
            os.makedirs(月目录, exist_ok=True)
            self.当前路径 = os.path.join(月目录, 月 + ".md")
            if not os.path.exists(self.当前路径):
                with open(self.当前路径, "w", encoding="utf-8") as f:
                    f.write("# CN-Smith 采样日志 %s（S1·三件套·M3 计时秒表·按月分文件）\n\n" % 月)
                    f.write("> 判据（plans/026 M3）：连续 4 周零新缺陷类别——跨月文件连续起算（索引=../采样日志.md）。\n")
                    f.write("> 记法（286-a·S1b）：变化轮完整记录；连续无变化轮合并为「汇总」行。\n\n")
        return self.当前路径

    def _冲刷汇总(self):
        if not self.汇总:
            return
        m = self.汇总
        路径 = self._切月(time.localtime(m["末"]))
        with open(路径, "a", encoding="utf-8") as f:
            f.write("## 汇总（%s ~ %s·%d 轮·seed=%d~%d）：采样 %d+回归 %d ｜ %s ｜ 命中集合不变·共 %d 项\n\n"
                    % (time.strftime("%H:%M:%S", time.localtime(m["起"])),
                       time.strftime("%H:%M:%S", time.localtime(m["末"])),
                       m["轮数"], m["首种子"], m["末种子"], m["采样"], m["回归"],
                       m["差分文本"], len(self.上轮命中 or ())))
        self.汇总 = None

    def 记轮(self, 基线, 说明, 种子, 新样本数, 回归数, 回归名单, 差分文本, 分歧, 新存, 用时):
        时刻 = time.time()
        st = time.localtime(时刻)
        命中 = {(名, 类别) for 名, 类别, _ in 分歧}
        命中名 = {名 for 名, _ in 命中}
        未命中回归 = set(回归名单) - 命中名
        if self.上轮未命中回归 is None:
            消失回归 = []      # 重启首轮：稳态全集基线化（不报变化）
        else:
            消失回归 = sorted(未命中回归 - self.上轮未命中回归)   # 增量=本轮新消失的
        无变化 = (新存 == 0 and not 消失回归 and 说明 == "无更新"
                  and self.上轮命中 is not None and 命中 == self.上轮命中)
        if 无变化:
            if self.汇总 is None:
                一, 分, 编, 运 = _解析计数(差分文本)
                self.汇总 = dict(起=时刻, 末=时刻, 轮数=0, 采样=0, 回归=0,
                                 差分文本=差分文本, 首种子=种子, 末种子=种子)
            self.汇总["末"] = 时刻
            self.汇总["轮数"] += 1
            self.汇总["采样"] += 新样本数
            self.汇总["回归"] += 回归数
            self.汇总["末种子"] = 种子
            self.上轮命中 = 命中
            self.上轮未命中回归 = 未命中回归
            return
        self._冲刷汇总()
        路径 = self._切月(st)
        with open(路径, "a", encoding="utf-8") as f:
            f.write("## 轮（%s）\n" % time.strftime("%Y-%m-%d %H:%M:%S", st))
            f.write("- 基线：%s（%s）\n" % (基线[:9], 说明))
            f.write("- 采样：%d 新样本（seed=%d）+ 回归轨 %d\n" % (新样本数, 种子, 回归数))
            f.write("- 差分：%s\n" % 差分文本)
            if 分歧:
                f.write("- 命中归类（本轮完整列表）：\n")
                for 名, 类别, 说明 in 分歧[:20]:
                    f.write("  - %s [%s] %s\n" % (名, 类别, 说明[:80]))
                if len(分歧) > 20:
                    f.write("  - …共 %d 条\n" % len(分歧))
            else:
                f.write("- 命中归类：零命中\n")
            if 消失回归:
                f.write("- 回归轨已修/已变化（本轮未再命中·文件留库继续防复发）：\n")
                for n in 消失回归[:20]:
                    f.write("  - %s\n" % n)
                if len(消失回归) > 20:
                    f.write("  - …共 %d 条\n" % len(消失回归))
            f.write("- 存档：新存 %d·回归轨已修 %d·用时 %.1fs\n\n" % (新存, len(消失回归), 用时))
        self.上轮命中 = 命中
        self.上轮未命中回归 = 未命中回归

    def 收尾(self):
        self._冲刷汇总()


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
    if not os.path.exists(指针文件):
        os.makedirs(库, exist_ok=True)
        with open(指针文件, "w", encoding="utf-8") as f:
            f.write(指针文本)

    写手 = 日志写手()
    try:
        轮 = 0
        while a.rounds == 0 or 轮 < a.rounds:
            轮 += 1
            t0 = time.time()
            print("=== S1 采样轮 %d ===" % 轮, flush=True)
            基线, 说明 = 拉取并重建()                        # ①
            种子, 回归名单 = 新样本轨(a.samples)              # ②
            回归数 = len(回归名单)
            新样本数 = sum(1 for f in os.listdir(工作) if f.endswith(".cn")) - 回归数
            差分文本, 分歧 = 差分(编译器)                     # ③
            新存 = 存档(分歧, 基线, 轮)                      # ④
            写手.记轮(基线, 说明, 种子, 新样本数, 回归数, 回归名单, 差分文本, 分歧, 新存, time.time() - t0)  # ⑤
            print("  " + 差分文本)
            print("  存档新 %d·日志已记（%.1fs）" % (新存, time.time() - t0))
            if a.rounds == 0 or 轮 < a.rounds:
                time.sleep(a.sleep)
    finally:
        写手.收尾()
    return 0


if __name__ == "__main__":
    sys.exit(主())
