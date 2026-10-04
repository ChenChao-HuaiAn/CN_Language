#!/usr/bin/env python3
# 云 CI 守护进程（920 治理专项·TX_02 部署·业界对照=CI 农场）：
#   轮询 gitcode develop SHA → 变化则自动跑全量门禁（完全复刻 scripts/integrate.py 的
#   Linux 三段口径：cmake 配置 target/build → 零警告构建 → 单测 → E2E
#   --target linux-x86_64〔红后 --jobs 1 串行复验〕）→ 结果落 ci-logs/<sha>.json
#   + latest.json，可选 HTTP 上报 TX_01 队列服务（环境变量/配置文件提供 URL 时）。
# 开发轮从此不在本机付全量验证税（AGENTS §8 v4：L3 全量=云端每推送+每日兜底）。
# 1021 预验执行器池化：常驻循环加「池优先段」——先向 TX_01 认领预验任务（task_claim·写锁内
#   原子），领到=fetch 该预验分支→跑一轮→task_complete（跑期间后台线程心跳续约·断了被服务端
#   回收重派）；没领到/池不可达→原有 develop 轮询行为不变（三层降级：池→ssh 直发→逃生门）。
#   CN_CI_ROLE=pool（家机 WSL2 实例）=纯池角色不轮询 develop；TX_02 无此 env=池+develop 双职。
#   多实例支持（家机 7 runner）：每实例独立工作目录（各自 flock/克隆）+CN_RUNNER_ID 区分身份。
# 用法：--loop  常驻轮询（systemd 服务）
#       --once  单轮检查（SHA 未变跳过；配 --force 无条件跑一轮=每日 cron 兜底）
# 部署：/etc/systemd/system/cn-ci.service（ExecStart=... --loop·Restart=always）
#       每日兜底：30 4 * * * ... --once --force（flock 非阻塞=正在跑时静默退出）
# 口径差异说明：三段不套 gate_lock（其存在理由=同机多 worktree 防互抢·AGENTS §8.8；
#   TX_02 独占 CI 无此场景）；E2E --jobs 3（4C3.6G 校准起点·家机实例 CN_E2E_JOBS=4）。

import fcntl
import json
import os
import re
import socket
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

# ===== 可调常量（集中区·911 先例） =====
轮询间隔秒 = 90            # git ls-remote 周期
e2e并行 = int(os.environ.get("CN_E2E_JOBS", "3"))      # TX_02 4C3.6G=3；家机 WSL2 实例 env=4
构建并行 = int(os.environ.get("CN_BUILD_JOBS", "2"))    # 首验实锤：全核 Make 编译 cc1plus 叠加 OOM
                                                        # （9daba·10-01）——TX_02 限 2；家机 8G/实例=4
单轮总超时秒 = 3 * 3600    # 防挂死（构建+单测+E2E+串行复验的理论上界）
远端名 = "origin"
分支 = os.environ.get("CN_CI_BRANCH", "develop")   # 常规盯 develop；任务分支预验=PR CI 同款用法
角色 = os.environ.get("CN_CI_ROLE", "")            # "pool"=纯池角色（家机实例·不轮询 develop）
runner标识 = os.environ.get("CN_RUNNER_ID", socket.gethostname())   # 池内身份（认领/心跳/完成）
心跳间隔秒 = 60            # task_heartbeat 周期（服务端 CN_TASK_STALE_SEC=300 回收阈值）
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


def 跑一轮(sha: str, 轮分支: str = "") -> dict:
    """对给定 SHA 跑全量门禁三段，返回结构化结果（口径=integrate.py Linux 分支）。
    轮分支空=全局 分支（develop 轮询/点名分支）；池任务传认领的预验分支（1021）。"""
    轮分支 = 轮分支 or 分支
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
                               (远端名, 轮分支)], capture_output=True, text=True,
                              cwd=仓库根, timeout=300)
        步骤们["同步"] = {"rc": 同步.returncode}
        if 同步.returncode != 0:
            print(同步.stdout + 同步.stderr, file=日志, flush=True)
        else:
            # ② 配置（默认生成器 Make·与 integrate 同口径；产物落 target/ 根=CMakeLists 16 行）
            if not 步骤("配置", ["cmake", "-S", ".", "-B", "target/build"], 600):
                pass  # 步骤们 已记 rc——下方统一收尾
            elif 步骤("构建", ["cmake", "--build", "target/build",
                               "--parallel", str(构建并行)], 单轮总超时秒):
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
    结果 = {"sha": sha, "分支": 轮分支, "平台": "linux-x86_64", "步骤": 步骤们,
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
    红 sha 若经 --no-cloud-gate 逃生门上了 develop，仍须 develop 轮真验）。
    1021：预验可能被池内其他实例（家机 WSL2）领跑——本机无 result 文件，须并查 池已绿()。"""
    for 文件 in 日志目录.glob("result_%s_*.json" % sha[:10]):
        try:
            if json.loads(文件.read_text(encoding="utf-8")).get("绿"):
                return True
        except Exception:
            continue
    return False


# ═══════════════ 1021 预验执行器池（TX_01 认领池客户端·家机/TX_02 同构） ═══════════════

def 池配置() -> dict:
    """池连接参数——env CN_POOL_URL/CN_POOL_TOKEN 优先，否则 ci_config.json（gitignore）的
    池地址/池令牌。未配置=空地址（池段整体跳过=纯 TX_02 旧行为）。"""
    配置 = {}
    配置文件 = 仓库根 / "ci_config.json"
    if 配置文件.exists():
        try:
            配置.update(json.loads(配置文件.read_text(encoding="utf-8")))
        except Exception:
            pass
    return {"池地址": os.environ.get("CN_POOL_URL") or 配置.get("池地址", ""),
            "池令牌": os.environ.get("CN_POOL_TOKEN") or 配置.get("池令牌", "")}


def 池调用(配置: dict, 操作: str, 数据: dict, 超时秒: int = 10):
    """POST TX_01 池 API。可达但业务拒绝（4xx）→ 读回响应体返回；网络不可达/异常 → None
    （调用方自行降级：认领失败=回 develop 轮询，complete 失败=重试后放弃留档）。"""
    try:
        请求 = urllib.request.Request(
            配置["池地址"].rstrip("/") + "/api/" + 操作,
            data=json.dumps(数据, ensure_ascii=False).encode("utf-8"),
            headers={"Content-Type": "application/json",
                     "Authorization": "Bearer " + 配置.get("池令牌", "")})
        with urllib.request.urlopen(请求, timeout=超时秒) as r:
            return json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return json.loads(e.read().decode("utf-8"))
        except Exception:
            return None
    except Exception:
        return None


def 池认领任务() -> dict | None:
    """向池认领一个预验任务（服务端写锁内原子+心跳超时回收）。无任务/池不可达/未配置 → None。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return None
    回 = 池调用(配置, "task_claim", {"runner": runner标识})
    if 回 and 回.get("ok"):
        return 回.get("任务")
    return None


def 池已绿(sha: str) -> bool:
    """零重复算力联动（1008 在池化下的续接）：该 sha 是否已被池内任一实例验绿——
    预验在家机跑绿后 push develop，TX_02 develop 轮据此跳过。池不可达/未配置 → False
    （回退本地 result 判定）。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return False
    try:
        with urllib.request.urlopen(配置["池地址"].rstrip("/") + "/api/state", timeout=10) as r:
            任务们 = json.loads(r.read().decode("utf-8")).get("预验任务") or []
    except Exception:
        return False
    return any(t.get("sha") == sha and t.get("状态") == "完成" and t.get("绿")
               for t in 任务们)


def 启动心跳(任务分支: str) -> threading.Event:
    """跑轮期间每 心跳间隔秒 向池续约（返回 stop 事件·池不可达静默——服务端超时回收是
    兜底而非灾难：complete 校验认领者，被重派后旧结果会被拒）。"""
    停 = threading.Event()

    def 跑():
        配置 = 池配置()
        while not 停.wait(心跳间隔秒):
            池调用(配置, "task_heartbeat", {"runner": runner标识, "分支": 任务分支})

    threading.Thread(target=跑, daemon=True).start()
    return 停


def 池跑任务(任务: dict) -> None:
    """执行认领到的预验任务：fetch 预验分支 → 跑一轮 → complete（3 次重试）。
    落盘复用 落盘()（ci/预验- 绿写 latest+上报=develop 轮免重跑联动保留）。"""
    任务分支, sha = 任务["分支"], 任务["sha"]
    print("[池] 认领 %s（sha=%s·runner=%s）" % (任务分支, sha[:10], runner标识), flush=True)
    配置 = 池配置()
    停 = 启动心跳(任务分支)
    try:
        取 = subprocess.run(["git", "fetch", 远端名, 任务分支], capture_output=True,
                            text=True, cwd=仓库根, timeout=300)
        if 取.returncode != 0:
            # fetch 失败=本轮失败（分支可能已被 integrate finally 删除=同 sha 重试竞态）——
            # 报红让 integrate 拦截重试；本地不落盘（非真实验证结果·防污染 result 档案）
            print("[池] fetch 失败：%s" % (取.stderr or "").strip()[:200], flush=True)
            结果 = {"sha": sha, "分支": 任务分支, "平台": "linux-x86_64", "绿": False,
                    "步骤": {"同步": {"rc": 取.returncode}}, "总秒": 0,
                    "时刻": datetime.now().isoformat(timespec="seconds"),
                    "日志": "", "日志尾部": (取.stderr or "").splitlines()[-5:]}
        else:
            结果 = 跑一轮(sha, 任务分支)
            落盘(结果)
        for 试 in range(3):     # complete 重试（TX_01 短暂抖动不触发回收重派的重复全量）
            回 = 池调用(配置, "task_complete",
                        {"runner": runner标识, "分支": 任务分支,
                         "绿": bool(结果.get("绿")), "结果": 结果})
            if 回 and 回.get("ok"):
                break
            print("[池警告] complete 第 %d 次失败：%s——10s 后重试" % (试 + 1, 回), flush=True)
            time.sleep(10)
        else:
            print("[池警告] complete 三次失败——本地留档·任务将被服务端回收重派（浪费一次算力·正确性无损）",
                  flush=True)
        print("[池完成] %s 绿=%s 总秒=%s" % (任务分支, 结果.get("绿"), 结果.get("总秒")), flush=True)
    finally:
        停.set()


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
        # ── 1021 池优先段：只在本职分支 develop 时进池（CN_CI_BRANCH 点名模式=ssh 直发
        #    降级路径·被点名跑指定预验分支，不抢池内别的任务）；flock 在手=同机单任务
        #    （923 OOM 铁律在池化下的保持形态——家机 7 实例各持各的锁文件）。
        if 分支 == "develop":
            任务 = 池认领任务()
            if 任务:
                池跑任务(任务)
                if not 常驻:
                    return 0
                time.sleep(轮询间隔秒)
                continue
            if 角色 == "pool":      # 纯池角色（家机实例）无任务→空闲休眠
                if not 常驻:
                    return 0
                time.sleep(轮询间隔秒)
                continue
        # ── 原有轮询（TX_02 develop 本职 / CN_CI_BRANCH 点名预验）
        sha = 取远端SHA()
        if sha is None:
            print("[警告] ls-remote 失败——下轮重试", flush=True)
        elif (已跑过(sha) or 池已绿(sha)) and not 强制:   # 1021：并查池绿记录（预验可能他实例跑的）
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
