#!/usr/bin/env python3
# 云 CI 守护进程（920 治理专项·TX_02 部署·业界对照=CI 农场）：
#   轮询 gitcode develop SHA → 变化则自动跑全量门禁（完全复刻 scripts/integrate.py 的
#   Linux 三段口径：cmake 配置 target/build → 零警告构建 → 单测 → E2E
#   --target linux-x86_64〔红后 --jobs 1 串行复验〕）→ 结果落 ci-logs/<sha>.json
#   + latest.json，可选 HTTP 上报 TX_01 队列服务（环境变量/配置文件提供 URL 时）。
# 开发轮从此不在本机付全量验证税（AGENTS §8 v4：L3 全量=云端每推送+每日兜底）。
# 用法：--loop  常驻轮询（systemd 服务）
#       --once  单轮检查（SHA 未变跳过；配 --force 无条件跑一轮=每日 cron 兜底）
# 部署：/etc/systemd/system/cn-ci.service（ExecStart=... --loop·Restart=always）
#       每日兜底：30 4 * * * ... --once --force（flock 非阻塞=正在跑时静默退出）
# 口径差异说明：三段不套 gate_lock（其存在理由=同机多 worktree 防互抢·AGENTS §8.8；
#   TX_02 独占 CI 无此场景）；E2E --jobs 3（4C3.6G 校准起点·非深度机 8 核的 8）。

import fcntl
import json
import os
import re
import subprocess
import sys
import time
import urllib.request
from datetime import datetime
from pathlib import Path

# ===== 可调常量（集中区·911 先例） =====
轮询间隔秒 = 90            # git ls-remote 周期
e2e并行 = 3                # 4 核 3.6G 内存保守起点（实测后调）
构建并行 = 2              # 首验实锤：全核 Make 编译 cc1plus 叠加 OOM（9daba 首轮 Killed·10-01）——限 2+swap 兜底
单轮总超时秒 = 3 * 3600    # 防挂死（构建+单测+E2E+串行复验的理论上界）
远端名 = "origin"
分支 = os.environ.get("CN_CI_BRANCH", "develop")   # 常规盯 develop；任务分支预验=PR CI 同款用法
仓库根 = Path(__file__).resolve().parent.parent
日志目录 = 仓库根 / "ci-logs"
锁文件 = 仓库根 / "ci-logs" / "daemon.lock"


def 物理内存MB() -> int:
    """读 /proc/meminfo——max-mem 保险丝按物理内存适配用（924）。"""
    try:
        for 行 in open("/proc/meminfo"):
            if 行.startswith("MemTotal:"):
                return int(行.split()[1]) // 1024
    except Exception:
        pass
    return 4096


def 取远端SHA() -> str | None:
    """git ls-remote 只查 develop 头——零流量无副作用。"""
    输出 = subprocess.run(["git", "ls-remote", 远端名, "refs/heads/" + 分支],
                          capture_output=True, text=True, cwd=仓库根, timeout=60)
    匹配 = re.search(r"^([0-9a-f]{40})\t", 输出.stdout, re.M)
    return 匹配.group(1) if 匹配 else None


def 运行(命令: list[str], 日志, 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """流式写日志（不落内存大块）+总超时看护。"""
    print("  $", " ".join(命令[:6]), ("..." if len(命令) > 6 else ""), file=日志, flush=True)
    return subprocess.run(命令, stdout=日志, stderr=subprocess.STDOUT,
                          cwd=仓库根, timeout=超时秒, **kwargs)


def 跑一轮(sha: str) -> dict:
    """对给定 SHA 跑全量门禁三段，返回结构化结果（口径=integrate.py Linux 分支）。"""
    开始 = time.time()
    轮时刻 = datetime.now().strftime("%m%d_%H%M%S")
    轮日志路径 = 日志目录 / ("run_%s_%s.log" % (sha[:10], 轮时刻))
    步骤们 = {}
    with 轮日志路径.open("w", encoding="utf-8") as 日志:
        print("== 云 CI 轮开始 %s @ %s ==" % (sha[:10], datetime.now()), file=日志, flush=True)

        def 步骤(名: str, 命令: list[str], 超时秒: int):
            t0 = time.time()
            try:
                r = 运行(命令, 日志, 超时秒)
                步骤们[名] = {"rc": r.returncode, "秒": round(time.time() - t0)}
            except subprocess.TimeoutExpired:
                步骤们[名] = {"rc": -99, "秒": 超时秒, "超时": True}
            return 步骤们[名]["rc"] == 0

        # ① 同步代码到该 SHA（reset 保干净树；git clean 不带 -x=保留 ignored 的 target/ 增量）
        同步 = subprocess.run(["bash", "-c",
                               "git fetch %s %s && git reset --hard -q FETCH_HEAD && git clean -fdq" %
                               (远端名, 分支)], capture_output=True, text=True,
                              cwd=仓库根, timeout=300)
        步骤们["同步"] = {"rc": 同步.returncode}
        if 同步.returncode != 0:
            print(同步.stdout + 同步.stderr, file=日志, flush=True)
        else:
            # ② 配置（默认生成器 Make·与 integrate 同口径；产物落 target/ 根=CMakeLists 16 行）
            if not 步骤("配置", ["cmake", "-S", ".", "-B", "target/build"], 600):
                pass  # 步骤们 已记 rc——下方统一收尾
            elif 步骤("构建", ["cmake", "--build", "target/build", "--parallel", "2"], 单轮总超时秒):
                # ③ 单测（产物三 fallback=integrate 同款）
                单测 = next((p for p in [仓库根 / "target/build/tests/unit/cn_unit_tests",
                                         仓库根 / "target/build/cn_unit_tests",
                                         仓库根 / "target/cn_unit_tests"] if p.exists()), None)
                if 单测 is None:
                    步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
                else:
                    步骤("单测", [str(单测)], 1800)
            # ④ E2E（linux-x86_64 面；红后串行复验=447-a 口径）
            cn = next((p for p in [仓库根 / "target/build/cn", 仓库根 / "target/cn"] if p.exists()), None)
            if cn is None:
                步骤们["e2e"] = {"rc": -1, "说明": "未找到 cn 产物"}
            elif 步骤们.get("构建", {}).get("rc") == 0:
                # 924·运维小件：max-mem 按物理内存适配（4096 默认>3.6G 物理=形同虚设——920 预判）；
                #   E2E 全程 nice -n 10 降优先级（CI 高负载 sshd 饿死 banner 超时实锤·运维手册④）
                保内存 = min(4096, int(物理内存MB() * 0.8))
                e2e基 = ["nice", "-n", "10", sys.executable, "tests/e2e/run_e2e.py",
                         "--target", "linux-x86_64", "--cn", str(cn),
                         "--max-mem-mb", str(保内存)]
                if 步骤("e2e并行", e2e基 + ["--jobs", str(e2e并行)], 单轮总超时秒):
                    步骤们["e2e"] = {"rc": 0}
                else:
                    print("  [复验] 并行红——--jobs 1 串行复验（447-a 口径）", file=日志, flush=True)
                    步骤("e2e串行复验", e2e基 + ["--jobs", "1"], 单轮总超时秒)
                    步骤们["e2e"] = {"rc": 步骤们["e2e串行复验"]["rc"]}
        尾部 = []
    # 日志尾部摘录进结果（供 TX_01 结果页直显；完整日志在 run_*.log）
    with 轮日志路径.open("r", encoding="utf-8", errors="replace") as f:
        尾部 = f.readlines()[-40:]
    结果 = {"sha": sha, "分支": 分支, "平台": "linux-x86_64", "步骤": 步骤们,
            "绿": all(s.get("rc") == 0 for k, s in 步骤们.items() if k in ("同步", "配置", "构建", "单测", "e2e")),
            "总秒": round(time.time() - 开始), "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": "".join(尾部).splitlines()[-20:]}
    return 结果


def 落盘(结果: dict) -> None:
    """结果落盘+可选上报（1008·预验模式规则）：
    - develop 常规轮：latest.json + result_*.json + 上报（既有行为）。
    - 非 develop 分支（CN_CI_BRANCH 预验）：
      · 恒写 预验_<sha10>.json（integrate.py 轮询取回的契约文件）+ result_*.json（历史档）；
      · ci/预验-* 且绿 → 额外写 latest.json+上报：链顶即将成为 develop 头，develop 轮询轮
        按「已跑过(sha)」跳过=云端零重复算力（1008 方案甲核心联动）；
      · ci/预验-* 且红 → 不写 latest 不上报（develop 未收到该提交——latest 必须保持 develop 语义）；
      · 任务分支预验（028 §三·PR CI 同款）→ 上报（带 分支 字段·结果页可区分），不写 latest。
    """
    文本 = json.dumps(结果, ensure_ascii=False, indent=1)
    sha10 = 结果["sha"][:10]
    当前分支 = 结果.get("分支", "develop")
    if 当前分支 == "develop":
        (日志目录 / "latest.json").write_text(文本, encoding="utf-8")
        (日志目录 / ("result_%s_%s.json" % (sha10,
                      datetime.now().strftime("%m%d_%H%M%S")))).write_text(
            文本, encoding="utf-8")
        上报(结果)
        return
    (日志目录 / ("预验_%s.json" % sha10)).write_text(文本, encoding="utf-8")
    (日志目录 / ("result_%s_%s.json" % (sha10,
                  datetime.now().strftime("%m%d_%H%M%S")))).write_text(
        文本, encoding="utf-8")
    if 当前分支.startswith("ci/预验-"):
        if 结果["绿"]:
            (日志目录 / "latest.json").write_text(文本, encoding="utf-8")
            上报(结果)
    else:
        上报(结果)


def 上报(结果: dict) -> None:
    """可选上报 TX_01 队列服务——URL/token 从环境变量或 ci_config.json 读；不可达仅警告。"""
    配置 = {}
    配置文件 = 仓库根 / "ci_config.json"
    if 配置文件.exists():
        try:
            配置.update(json.loads(配置文件.read_text(encoding="utf-8")))
        except Exception:
            pass
    url = os.environ.get("CN_CI_REPORT_URL") or 配置.get("上报地址")
    if not url:
        return
    令牌 = os.environ.get("CN_CI_TOKEN") or 配置.get("令牌", "")
    try:
        请求 = urllib.request.Request(url, data=json.dumps(结果, ensure_ascii=False).encode("utf-8"),
                                      headers={"Content-Type": "application/json",
                                               "Authorization": "Bearer " + 令牌})
        urllib.request.urlopen(请求, timeout=10)
    except Exception as e:
        print("[警告] 上报失败（不影响本地结果）：%s" % e, flush=True)


def 已跑过(sha: str) -> bool:
    """该 SHA 是否已有**绿**的 result_*.json（1008 修正：预验绿会覆盖 latest.json〔链顶=未来
    develop 头〕，单看 latest 会把刚验过的 develop 头误判未跑→重复全量；按 result 历史档的
    绿记录判定——预验绿的链顶 push 后 develop 轮据此跳过=云端零重复算力。红 result 不算数：
    红 sha 若经 --no-cloud-gate 逃生门上了 develop，仍须 develop 轮真验）。"""
    for 文件 in 日志目录.glob("result_%s_*.json" % sha[:10]):
        try:
            if json.loads(文件.read_text(encoding="utf-8")).get("绿"):
                return True
        except Exception:
            continue
    return False


def 主(常驻: bool, 强制: bool, 等锁: bool = False) -> int:
    日志目录.mkdir(exist_ok=True)
    锁 = (锁文件.open("a+"))
    try:                                    # 非阻塞锁：cron 兜底撞上常驻跑轮=静默让路
        fcntl.flock(锁, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        if not 等锁:
            print("[锁] 已有轮在跑——退出。", flush=True)
            return 0
        # --wait-lock（1008·ci/预验 触发用）：排队等现有轮（develop 常驻轮/其他预验）完成——
        # 3.6G 内存双全量并发=923 OOM 铁律，预验必须串行排队而非并发抢跑
        print("[锁] 已有轮在跑——等待（--wait-lock·预验排队）……", flush=True)
        fcntl.flock(锁, fcntl.LOCK_EX)
    while True:
        sha = 取远端SHA()
        if sha is None:
            print("[警告] ls-remote 失败——下轮重试", flush=True)
        elif 已跑过(sha) and not 强制:
            print("[跳过] %s 已跑过" % sha[:10], flush=True)
        else:
            print("[开跑] %s @ %s（分支=%s%s）" % (sha[:10],
                  datetime.now().strftime("%H:%M:%S"), 分支,
                  "·预验" if 分支 != "develop" else ""), flush=True)
            结果 = 跑一轮(sha)
            落盘(结果)
            print("[完成] 绿=%s 总秒=%s 详情=%s" % (结果["绿"], 结果["总秒"], 结果["日志"]), flush=True)
        if not 常驻:
            return 0
        time.sleep(轮询间隔秒)


if __name__ == "__main__":
    常 = "--loop" in sys.argv
    强 = "--force" in sys.argv
    等锁 = "--wait-lock" in sys.argv
    if "--once" not in sys.argv and not 常:
        print("用法: ci_daemon.py --loop | --once [--force] [--wait-lock]；无参默认 --once")
    raise SystemExit(主(常, 强, 等锁))
