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
#   多实例支持（家机 runner）：每实例独立工作目录（各自 flock/克隆）+CN_RUNNER_ID 区分身份。
# 189 兜底池化（2026-10-07 用户裁决丙案）：develop 轮真跑前两道优化——①纯文档轮免兜底
#   （基线..新头 变更集全 .md→免跑留档·result 档案幂等防重复判定·任何异常/非 md 文件
#   照跑=fail-safe 默认）②混合变更入池（task_enqueue ci/兜底-<sha10>·入池失败降级本机
#   跑=门禁永不缺位）。--force 每日 cron 与点名预验不受影响=门禁语义保持。
# 770 develop 验证重造（2026-10-14 用户三条裁决）：①头变化必验不可跳过——删「已跑过/
#   池已绿」跳过（1008 零重复算力联动废止·预验绿过的头 develop 面仍复验），纯文档免跑保留；
#   ②派发=服务端 win→linux→arm64 游标轮转+平台内随机空闲+该平台全忙顺序跳平台（服务端
#   queue_service 落地·客户端零派发逻辑）；③所有实例统一「池认领+轮询发现新头入池（幂等
#   已入池集合防重）」——CN_CI_ROLE 角色区分废止（env 保留仅供 run_e2e CI 识别与旧部署兼容）。
# 用法：--loop  常驻轮询（systemd 服务）
#       --once  单轮检查（SHA 未变跳过；配 --force 无条件跑一轮=每日 cron 兜底）
# 部署：/etc/systemd/system/cn-ci.service（ExecStart=... --loop·Restart=always）
#       每日兜底：30 4 * * * ... --once --force（flock 非阻塞=正在跑时静默退出）
# 口径差异说明：三段不套 gate_lock（其存在理由=同机多 worktree 防互抢·AGENTS §8.8；
#   TX_02 独占 CI 无此场景）；E2E --jobs 3（4C3.6G 校准起点·家机实例 CN_E2E_JOBS=4）。

# 291 win 池实例（2026-10-08 用户裁决·混合随机平台池）：win 实例与 linux 实例同池认领
#   同一任务集（任务不带平台标签·谁空闲谁认领）——任一平台红灯=平台差异缺陷暴露
#   （预验位红=develop 进不去·拦截前移）；门禁表 PK=(sha,平台) 天然分平台显示，池协议零改动。
#   win 侧差异全部收敛在平台分支内：msvcrt 锁/GlobalMemoryStatusEx/vcvars+Ninja 构建链/
#   E2E --target win-x64；三段外套 gate_lock（家机同机有人工开发·与 TX_02 独占场景不同）。
#   平台身份一律运行时探测（sys.platform），不依赖任何静态配置——共享文档不指定本机类型。
# 523 win 增量构建混合态治本（2026-10-13）：池每轮 reset+clean(-fdq 不带 -x) 保留 target/
#   增量，跨 HEAD 的对象/生成文件混合→坏 cn.exe（0xC0000005/挂死/ml64 失败·干净重建即愈
#   ·554 实录三例）。两道修：①HEAD 指纹清建——构建前比对 ci-logs/last_build.sha 与实际
#   HEAD，不一致/缺失→rmdir build-ninja 全量重配重编（与「干净重建即愈」同构=治本；
#   sccache 命中兜成本·指纹一致轮零税）；构建绿且单测绿才更新指纹（可疑产物下轮必清）。
#   ②每轮对齐 wt.py 开发树 088 修复：删 .ninja_deps（/showIncludes 被 sccache 吞→陈旧
#   header deps→ODR 混合对象→静默崩溃）。clean -x 全清不做（每轮全量重编=过度代价·
#   增量治理由指纹机制承担）。--selftest=生成文本+判定三态轻量自测（零真构建·660 起
#   实现体外迁 ci_daemon_selftest.py·仅 --selftest 分支惰性导入）。
# 660 兜底跑前过期预检（2026-10-14）：兜底池任务旧序「先跑整轮再判 develop 是否前进」——
#   被 develop 新头淘汰的陈轮每轮烧整轮算力（池内多行陈轮反复认领空跑·TX_02 OOM 实录）；
#   修=fetch 后仅 rev-parse FETCH_HEAD 与任务 sha 比对，不等即过期跳过（零构建零副作用）。
# 兼容 python3.8：注解泛型下标/联合字符串化（287·同 gate_quick 修复缘由）。
from __future__ import annotations

import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

是win = sys.platform.startswith("win")          # 仓库惯例（gate_quick/gate_lock 同款）
# 294：平台三分——linux 再按机架分 arm64/x86_64（291 两分把 aarch64 误标 linux-x86_64，
#   E2E 会跑错 --target·门禁表平台列错行）；单位机（麒麟 arm64）池实例依赖本探测。
# 任务399（2026-10-10）：CN_CI_PLATFORM 显式覆盖——交叉模拟实例（x64 宿主跑
#   linux-arm64 验证面·WSL 里 qemu-user）自报 linux-arm64；缺省仍运行时探测
#   （291 惯例不变·覆盖变量=实例部署配置，与 CN_RUNNER_ID 同类，非共享文档写死）。
机架 = platform.machine().lower()
平台 = (os.environ.get("CN_CI_PLATFORM", "").strip()
        or ("win-x64" if 是win
            else "linux-arm64" if 机架 in ("aarch64", "arm64")
            else "linux-x86_64"))               # run_e2e --target 与结果上报共用
if 是win:
    import msvcrt
else:
    import fcntl
import gate_lock        # win 三段互斥用（linux 零调用·TX_02 行为不变）

# ===== 可调常量（集中区·911 先例） =====
轮询间隔秒 = 90            # git ls-remote 周期
e2e并行 = int(os.environ.get("CN_E2E_JOBS", "6" if 是win else "3"))
                                                        # TX_02 4C3.6G=3；家机 WSL2 实例 env=4；
                                                        # win 4 实例峰值 4×6=24 jobs+构建<28 线程（291）
构建并行 = int(os.environ.get("CN_BUILD_JOBS", "2"))    # 首验实锤：全核 Make 编译 cc1plus 叠加 OOM
                                                        # （9daba·10-01）——TX_02 限 2；家机 8G/实例=4
单轮总超时秒 = 3 * 3600    # 防挂死（构建+单测+E2E+串行复验的理论上界）
远端名 = "origin"
分支 = os.environ.get("CN_CI_BRANCH", "develop")   # 常规盯 develop；任务分支预验=PR CI 同款用法
角色 = os.environ.get("CN_CI_ROLE", "")            # 770 起不再区分（统一池·env 仅供 run_e2e CI 识别与旧部署兼容）
runner标识 = os.environ.get("CN_RUNNER_ID", socket.gethostname())   # 池内身份（认领/心跳/完成）
心跳间隔秒 = 60            # task_heartbeat 周期（服务端 CN_TASK_STALE_SEC=300 回收阈值）
仓库根 = Path(__file__).resolve().parent.parent
日志目录 = 仓库根 / "ci-logs"
锁文件 = 仓库根 / "ci-logs" / "daemon.lock"


def 抢锁(锁, 阻塞: bool = False) -> bool:
    """跨平台劝告锁（291）：posix=flock；win=msvcrt.locking——区域锁从文件指针起算，
    统一 seek(0) 保证各进程锁同一字节；LK_LOCK 仅约 10s 重试，阻塞等待自行轮询。"""
    锁.seek(0)
    if 是win:
        try:
            msvcrt.locking(锁.fileno(), msvcrt.LK_LOCK if 阻塞 else msvcrt.LK_NBLCK, 1)
            return True
        except OSError:
            if not 阻塞:
                return False
            while True:
                time.sleep(轮询间隔秒)
                锁.seek(0)
                try:
                    msvcrt.locking(锁.fileno(), msvcrt.LK_NBLCK, 1)
                    return True
                except OSError:
                    continue
    try:
        fcntl.flock(锁, fcntl.LOCK_EX | (0 if 阻塞 else fcntl.LOCK_NB))
        return True
    except OSError:
        return False


def 放锁(锁) -> None:
    if 是win:
        锁.seek(0)
        try:
            msvcrt.locking(锁.fileno(), msvcrt.LK_UNLCK, 1)
        except OSError:
            pass
    else:
        fcntl.flock(锁, fcntl.LOCK_UN)


def 物理内存MB() -> int:
    """max-mem 保险丝按物理内存适配用（924）——posix 读 /proc/meminfo；win=GlobalMemoryStatusEx。"""
    if 是win:
        import ctypes

        class 内存状态(ctypes.Structure):
            _fields_ = [("长度", ctypes.c_ulong), ("负载", ctypes.c_ulong),
                        ("总物理", ctypes.c_ulonglong), ("可用物理", ctypes.c_ulonglong),
                        ("总页文件", ctypes.c_ulonglong), ("可用页文件", ctypes.c_ulonglong),
                        ("总虚拟", ctypes.c_ulonglong), ("可用虚拟", ctypes.c_ulonglong),
                        ("可用扩展虚拟", ctypes.c_ulonglong)]
        状态 = 内存状态(长度=ctypes.sizeof(内存状态))
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(状态)):
            return int(状态.总物理 // (1024 * 1024))
        return 4096
    try:
        for 行 in open("/proc/meminfo"):
            if 行.startswith("MemTotal:"):
                return int(行.split()[1]) // 1024
    except Exception:
        pass
    return 4096


def 取远端SHA() -> str | None:
    """git ls-remote 只查 develop 头——零流量无副作用。"""
    输出 = 运行限时(["git", "ls-remote", 远端名, "refs/heads/" + 分支],
                    60, capture_output=True, text=True, cwd=仓库根)
    匹配 = re.search(r"^([0-9a-f]{40})\t", 输出.stdout, re.M)
    return 匹配.group(1) if 匹配 else None


# ═══════════════ 189 兜底池化改造（2026-10-07 用户裁决丙案） ═══════════════

def 上次已验SHA() -> str | None:
    """免跑判定基线=最近一次 develop 验证绿的树头。latest.json 的 sha 即该值
    （develop 轮绿/预验绿链顶都写 latest——预验绿链顶=验绿后 push 的 develop 头）。
    读不到（冷启动/损坏）→ None（调用方照跑=保守）。"""
    try:
        return json.loads((日志目录 / "latest.json").read_text(encoding="utf-8")).get("sha")
    except Exception:
        return None


def 纯文档变更(基线sha: str, 新sha: str) -> tuple[bool, str]:
    """①纯文档轮免兜底判定（189）：基线..新 的变更文件**全部 .md**→真（免跑）。
    fail-safe 默认（Rust 同款哲学：默认保守）——fetch/diff 任何异常、基线缺失、
    出现任何非 .md 文件（代码/脚本/配置·尤其 scripts/——242 坏脚本广播教训）
    一律判假=照跑。返回 (判定, 缘由/文件摘要)。"""
    try:
        取 = subprocess.run(["git", "fetch", 远端名, "develop"], capture_output=True,
                            text=True, cwd=仓库根, timeout=120)
        if 取.returncode != 0:
            return False, "fetch 失败照跑：%s" % (取.stderr or "").strip()[:120]
        diff = subprocess.run(["git", "diff", "--name-only", "%s..%s" % (基线sha, 新sha)],
                              capture_output=True, text=True, cwd=仓库根, timeout=120)
        if diff.returncode != 0:
            return False, "diff 失败照跑：%s" % (diff.stderr or "").strip()[:120]
        文件们 = [l for l in diff.stdout.splitlines() if l.strip()]
        if not 文件们:
            return True, "变更集为空（基线即新头）"
        非md = [f for f in 文件们 if not f.lower().endswith(".md")]
        if 非md:
            return False, "含非文档 %d 个（首=%s）" % (len(非md), 非md[0][:80])
        return True, "纯文档 %d 个全 .md" % len(文件们)
    except Exception as e:
        return False, "判定异常照跑：%s" % str(e)[:120]


def 免跑留档(sha: str, 缘由: str) -> None:
    """纯文档轮免跑留档（189·770 保留）：走既有 result 档案机制（develop 语义=latest+result+
    上报）——本地 result 档据此防重复入池=幂等零重复判定；结果页可审计（绿=门禁无需全量·
    步骤字段明记免跑缘由）。绿 的口径：result「绿」由 同步/配置/构建/单测/e2e 键聚合
    （跑一轮 同款），免跑结果无这些键→聚合为 True——语义=「本 sha 无需全量即合规」。"""
    步骤字段 = {"免跑": {"rc": 0, "说明": "纯文档轮免兜底（189）·" + 缘由}}
    结果 = {"sha": sha, "分支": "develop", "平台": 平台, "绿": True,
            "步骤": 步骤字段,
            "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": "", "日志尾部": ["[免跑] %s %s" % (sha[:10], 缘由)]}
    日志目录.mkdir(exist_ok=True)     # 189 实测补：冷启动树 ci-logs 未建时免跑留档曾崩
    落盘(结果)
    print("[免跑] %s %s（留档可审计）" % (sha[:10], 缘由), flush=True)


def 兜底入池(sha: str) -> bool:
    """develop 验证任务入池（189 引入·770 派发语义升级）：develop 新头提交 ci/兜底-<sha10>
    任务给验证池——服务端按 win→linux→arm64 游标轮转指派平台+平台内随机空闲+全忙放宽
    （queue_service 落地·客户端零派发逻辑）。入池失败（池不可达/拒绝）→ False
    （调用方降级本机跑=三层降级精神·门禁永不因池而缺位）。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return False
    回 = 池调用(配置, "task_enqueue", {"分支": "ci/兜底-%s" % sha[:10], "sha": sha})
    if 回 and 回.get("ok"):
        print("[兜底入池] ci/兜底-%s（runner 池接手）" % sha[:10], flush=True)
        return True
    print("[兜底入池失败] 降级本机跑：%s" % 回, flush=True)
    return False


def 运行限时(命令: list[str], 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """subprocess.run 的超时安全壳（294）：超时返回伪 CompletedProcess（rc=-99·与 步骤()
    超时标记同口径）而非抛 TimeoutExpired 炸进程——单位机实例实况：网络抖动 git fetch
    300s 超时未捕获，systemd restart 循环 12 次（轮结果与审计全丢）。"""
    try:
        return subprocess.run(命令, timeout=超时秒, **kwargs)
    except subprocess.TimeoutExpired as e:
        出 = e.stdout if isinstance(e.stdout, str) else (e.stdout or b"").decode("utf-8", "replace")
        return subprocess.CompletedProcess(命令, -99,
                                           stdout=出[-400:], stderr="超时 %ds" % 超时秒)


def 运行(命令: list[str], 日志, 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """流式写日志（不落内存大块）+总超时看护；可执行不存在（FileNotFoundError·如服务
    环境 PATH 缺用户级 cmake——单位机实例实录崩循环）记 rc=127 轮红而非炸进程（297·
    与 294 运行限时同族：环境面异常一律步骤红·不进程崩）。"""
    print("  $", " ".join(命令[:6]), ("..." if len(命令) > 6 else ""), file=日志, flush=True)
    try:
        return subprocess.run(命令, stdout=日志, stderr=subprocess.STDOUT,
                              cwd=仓库根, timeout=超时秒, **kwargs)
    except FileNotFoundError as e:
        print("  [FAIL] 可执行不存在：%s" % e, file=日志, flush=True)
        return subprocess.CompletedProcess(命令, 127)


def 找vcvars() -> Path | None:
    """vcvars64.bat 探测（291·复刻 wt.py 同款四版本候选）。"""
    for 版本 in ("Community", "Professional", "Enterprise", "BuildTools"):
        候选 = Path("C:/Program Files/Microsoft Visual Studio/2022/%s/VC/Auxiliary/Build/vcvars64.bat" % 版本)
        if 候选.exists():
            return 候选
    return None


# ═══════════════ 523 win 构建链（混合态治本·指纹清建+088 对齐） ═══════════════

def 需指纹清建(当前头: str, 记录路径: Path | None = None) -> tuple[bool, str]:
    """HEAD 指纹清建判定（523·治本第一道）：比对实际 HEAD 与上次成功构建指纹
    （ci-logs/last_build.sha·各实例独立）。不等/缺失/损坏/空头→清建（fail-safe
    保守：宁可多清一次全编——sccache 命中约 30s·漏清=随机红重跑更贵）。
    返回 (是否清建, 缘由)。记录路径参数供 --selftest 隔离注入，生产用缺省。"""
    if not 当前头:
        return True, "HEAD 不可知（rev-parse 失败/空）→保守清建"
    记录 = 记录路径 or (日志目录 / "last_build.sha")
    try:
        上次 = 记录.read_text(encoding="utf-8").strip()
    except Exception:
        return True, "无构建指纹（冷启动/部署升级/上轮未绿）→保守清建"
    if 上次 != 当前头:
        return True, "HEAD 变化（上=%s→现=%s）→清建防混合态" % (上次[:10], 当前头[:10])
    return False, "指纹一致→保留增量"


def win头行(vcvars: Path) -> list[str]:
    """win_configure.cmd / win_build.cmd 公共头（291 原内联段提出·523 抽函数）。"""
    return ["@echo off", 'cd /d "%~dp0.."',
            'call "%s" >nul 2>&1' % vcvars,
            'where cl >nul 2>&1 || (echo [FAIL] cl not found & exit /b 1)']


def win配置行(vcvars: Path, 清建: bool) -> list[str]:
    """win_configure.cmd 生成（291 原内联段提出）。清建形态（523）=先 rmdir 整个
    build-ninja——与「干净重建即愈」完全同构=治本；半清（只删 obj/.ninja_*）无法
    论证穷尽残留面（.ninja_log 时间戳/生成文件/rsp/PDB·ILK 链接中间态均可陈旧），
    留随机红尾巴。cmd 文本必须 ASCII（encoding="ascii"·rem 注释用英文）。"""
    行 = win头行(vcvars)
    if 清建:
        行 += ["rem 523 root-fix: HEAD fingerprint changed -> full clean rebuild",
               "rem (mirrors the known-good fix: clean rebuild; sccache absorbs cost)",
               "if exist target\\build-ninja rmdir /s /q target\\build-ninja"]
    launcher = ""
    sccache = shutil.which("sccache")
    if sccache:
        launcher = ' -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="%s"' % sccache
    # CXX_FLAGS_DEBUG 必须显式 /Z7（wt.py 开发树同口径）——默认 /ZI 集中 PDB
    # 在 --parallel 下多 cl.exe 并写同 .pdb 必撞 C1041（实测）
    行.append('cmake -G Ninja -S . -B target/build-ninja '
              '-DCMAKE_BUILD_TYPE=Debug '
              '-DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1" ' + launcher)
    return 行


def win构建行(vcvars: Path) -> list[str]:
    """win_build.cmd 生成（291 原内联段提出+523 对齐 wt.py 开发树 088 修复）：
    每轮构建前删 .ninja_deps——MSVC+Ninja 的 header deps 经 /showIncludes 记录，
    sccache（CMAKE_CXX_COMPILER_LAUNCHER）可能吞其 stdout→陈旧 deps→该重编的
    没重编→ODR 混合对象→静默崩溃；删后强制全量重扫头（成本由 sccache 命中吸收
    ·wt.py 实测冷全量 ~23s）。"""

    return win头行(vcvars) + [
        "rem 088 root-fix: drop stale .ninja_deps so every build re-scans all headers.",
        "rem MSVC+Ninja records header deps from /showIncludes stdout, which sccache",
        "rem (CMAKE_CXX_COMPILER_LAUNCHER) may swallow -> stale deps -> ODR mixed",
        "rem objects -> silent crash. Deleting .ninja_deps forces a full re-scan",
        "rem (cost absorbed by sccache hit; measured cold full build ~23s).",
        "if exist target\\build-ninja\\.ninja_deps del /q target\\build-ninja\\.ninja_deps",
        "cmake --build target/build-ninja --parallel",
        "if errorlevel 1 (echo [FAIL] build & exit /b 1)"]


def win构建链(步骤们: dict, 步骤, 日志, vcvars: Path, 当前头: str) -> None:
    """win 构建链整段（291 原内联于 跑一轮·523 抽出=跑一轮 拆函数+指纹挂点）：
    配置（指纹不一致→先 rmdir 全清再重配）→构建（每轮先删 .ninja_deps）→单测。
    指纹写盘口径：构建 rc==0 **且单测 rc==0** 才更新 last_build.sha——构建绿但
    单测挂死/红=产物可疑（可能混合态·554 症候）→不更新，下轮重试必清建
    （同夜两轮均坏、第二轮即愈的对症面；真实红缺陷重试多付 ~30s 清建=可接受）。"""
    清建, 缘由 = 需指纹清建(当前头)
    print("  [523] %s" % 缘由, file=日志, flush=True)
    if 清建 or not (仓库根 / "target/build-ninja/build.ninja").exists():
        配置脚本 = 日志目录 / "win_configure.cmd"
        配置脚本.write_text("\r\n".join(win配置行(vcvars, 清建)) + "\r\n", encoding="ascii")
        步骤("配置", ["cmd", "/c", str(配置脚本)], 900)
    else:
        步骤们["配置"] = {"rc": 0, "说明": "build-ninja 已配置（增量）"}
    if 步骤们["配置"].get("rc") != 0:
        return
    构建脚本 = 日志目录 / "win_build.cmd"
    构建脚本.write_text("\r\n".join(win构建行(vcvars)) + "\r\n", encoding="ascii")
    步骤("构建", ["cmd", "/c", str(构建脚本)], 单轮总超时秒)
    # 单测（产物落 target/ 根=CMAKE_RUNTIME_OUTPUT_DIRECTORY·exe 后缀）
    单测 = next((p for p in [仓库根 / "target/cn_unit_tests.exe",
                             仓库根 / "target/build/cn_unit_tests.exe",
                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
    if 单测 is None:
        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
        return
    步骤("单测", [str(单测)], 1800)
    if 步骤们["构建"]["rc"] == 0 and 步骤们["单测"]["rc"] == 0:
        (日志目录 / "last_build.sha").write_text(当前头 + "\n", encoding="utf-8")
        print("  [523] 构建指纹已更新：%s" % 当前头[:10], file=日志, flush=True)


def linux构建链(步骤们: dict, 步骤) -> None:
    """linux 构建段整段（657 自 跑一轮 抽出·与 win构建链 对称=跑一轮 回 ≤100 行
    纪律）：②配置（默认生成器 Make·与 integrate 同口径；产物落 target/ 根）→构建→
    ③单测（产物三 fallback=integrate 同款）。逻辑与抽出前逐行等价（零行为变化）。"""
    if not 步骤("配置", ["cmake", "-S", ".", "-B", "target/build"], 600):
        return  # 配置红——步骤们 已记 rc，构建/单测不跑（原 pass 分支语义）
    if not 步骤("构建", ["cmake", "--build", "target/build",
                        "--parallel", str(构建并行)], 单轮总超时秒):
        return  # 构建红——单测不跑（原 elif 语义）
    单测 = next((p for p in [仓库根 / "target/build/tests/unit/cn_unit_tests",
                             仓库根 / "target/build/cn_unit_tests",
                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
    if 单测 is None:
        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
    else:
        步骤("单测", [str(单测)], 1800)


def 静态门禁与e2e段(步骤们: dict, 步骤, cn: Path | None, 日志) -> None:
    """③.5 静态门禁面+④ E2E（242/447-a/924 原内联于 跑一轮·523 抽出=跑一轮
    拆函数回 ≤100 行）——两平台公共段；cn 产物定位必须先于本段（原赋值在 ④ 段
    晚于引用=UnboundLocalError 必崩·259 修）。逻辑逐行保持原样。
    728t（2026-10-14）：523 抽出时漏传 日志 形参——e2e 并行红打 422 行 [复验] 提示
    行即 NameError 崩轮（wt2/wt3 双实例实录·红轮永不落盘/不上报→服务端 300s 回收
    重派=池算力黑洞第二形态）。修=签名补 日志 + 调用点传参；selftest 加自由名锚
    （通用·防抽函数漏参族再犯）+ 422 路径冒烟两态。"""
    for 名, 参 in (("check_spec_coverage", []),   # 243：非 strict（TX_02「指针有效性」存量红=244 修·红不拦兜底轮）
                  ("check_cli_contract", ["--cn", str(cn)]),
                  ("check_asm_width", ["--cn", str(cn)]),
                  ("check_matrix_coverage", []),
                  ("check_file_length", ["--freeze"])):
        if cn is None and "--cn" in 参:
            步骤们[名] = {"rc": -1, "说明": "未找到 cn 产物·跳过"}
            continue
        if 步骤(名, [sys.executable, f"scripts/{名}.py"] + 参, 900):
            pass
    # ④ E2E（探测平台面；红后串行复验=447-a 口径）
    if cn is None:
        步骤们["e2e"] = {"rc": -1, "说明": "未找到 cn 产物"}
    elif 步骤们.get("构建", {}).get("rc") == 0:
        # 924·运维小件：max-mem 按物理内存适配（4096 默认>3.6G 物理=形同虚设——920 预判）；
        #   E2E 全程 nice -n 10 降优先级（CI 高负载 sshd 饿死 banner 超时实锤·运维手册④）
        #   291：nice 为 posix 专属——win 无此前缀（同机互斥已由 gate_lock 承担）；
        #   --target 用运行时探测平台（win-x64 / linux-x86_64）
        保内存 = min(4096, int(物理内存MB() * 0.8))
        # 566（2026-10-13 裁决乙案·修复审查 P0-1）：78/79 重负载用例本地默认跳过留
        #   云端——云端 runner 是其唯一合法执行面。放行走 CI 通道显式开关 --heavy-ci
        #   （行为=跳过立法前完全一致：--jobs/锁/内存保护全不动；不传 --allow-heavy
        #   ——那是本地通道·强制并发=1+机器锁，与云端池化并行语义冲突）；环境判据
        #   （CN_RUNNER_ID/CN_CI_ROLE/CN_CI_BRANCH 任一在设·028 部署模板）兜底自动
        #   放行——旧版 ci_daemon（仍传旧名 --allow-heavy）+新 run_e2e 时经环境判据
        #   命中 CI 通道自愈（旧 daemon 部署面池实例三变量在设；TX_02 待部署清单补
        #   systemd Environment 后 daemon 重启生效）。
        e2e基 = ([] if 是win else ["nice", "-n", "10"]) + [
            sys.executable, "tests/e2e/run_e2e.py",
            "--target", 平台, "--cn", str(cn),
            "--max-mem-mb", str(保内存),
            "--heavy-ci",
            "--full-reason", "ci_daemon 云端全量门禁（TX_02/池 runner）"]
        if 步骤("e2e并行", e2e基 + ["--jobs", str(e2e并行)], 单轮总超时秒):
            步骤们["e2e"] = {"rc": 0}
        else:
            print("  [复验] 并行红——--jobs 1 串行复验（447-a 口径）", file=日志, flush=True)
            步骤("e2e串行复验", e2e基 + ["--jobs", "1"], 单轮总超时秒)
            步骤们["e2e"] = {"rc": 步骤们["e2e串行复验"]["rc"]}


def 带门禁锁跑(sha: str, 轮分支: str = "") -> dict | None:
    """291 win 三段外套 gate_lock：家机同机有人工开发全量门禁（与 TX_02 独占 CI 场景
    不同）——win 池实例三段与主树门禁全机互斥（AGENTS §7 本机全量每机 ≤1；池树经
    CN_GATE_LOCK_DIR 指向主树锁目录·跨树共享）。30 分钟未获得=让出（返回 None）：
    调用方不落盘不 complete——心跳停由服务端 300s 回收重派（本机忙让给别人=随机池本义）。
    linux 零开销直通（TX_02/WSL 行为不变·原注释口径「独占 CI 不套锁」保持）。
    566（审查 P0-2）：持锁期间向本进程环境注入 CN_GATE_LOCK_HELD=1——嵌套链子进程
    （run_e2e --allow-heavy / gate_lock acquire·run 等）见之即豁免跳过取机器锁，
    防「父进程已持锁·子进程再取同一把」无限自死锁（子进程经 os.environ 继承）；
    finally 恢复原值（长期驻留 daemon 的环境不被污染）。"""
    门禁锁 = gate_lock.获取(1800, 形态="acquire", owner_pid=os.getpid()) if 是win else None
    if 是win and 门禁锁 is None:
        print("[让出] gate_lock 30 分钟未获得（本机门禁互忙）——本轮放弃，任务由服务端回收重派", flush=True)
        return None
    豁免原值 = os.environ.get("CN_GATE_LOCK_HELD")
    if 是win:
        os.environ["CN_GATE_LOCK_HELD"] = "1"
    try:
        return 跑一轮(sha, 轮分支)
    finally:
        if 是win:
            if 豁免原值 is None:
                os.environ.pop("CN_GATE_LOCK_HELD", None)
            else:
                os.environ["CN_GATE_LOCK_HELD"] = 豁免原值
            gate_lock.释放(门禁锁, 静默=True)


def 预验清产物(树根: Path) -> bool:
    """637（2026-10-14 用户立法）：预验前置②——历史编译中间产物全部清零。
    rm -rf <树根>/target/（含 target/audit2 的 v2p 指纹缓存）——之后从零配置构建。
    树根参数供 --selftest 注入临时目录隔离断言（生产传 仓库根）。返回是否清干净。"""
    目标目录 = 树根 / "target"
    if 目标目录.exists():
        shutil.rmtree(目标目录, ignore_errors=True)
    return not 目标目录.exists()


def 同步后编排(同步rc: int, 轮分支: str, 是win: bool) -> dict:
    """657（2026-10-14）：同步完成后的步骤编排决策——纯函数（零副作用·零真构建），
    --selftest 正反两态直调；跑一轮 消费其返回值=编排口径单一事实源（648-b 语义如实
    镜像·复核锁定不回改）：同步失败→三段全不执行；ci/预验-* 轮→清产物（637 干净
    构建·独立前序步骤·不再与构建段 elif 互斥）与平台构建段并存（linux=配置/构建/
    单测；win=win构建链）；develop/兜底轮→无清产物（523 增量口径），构建段照常。
    返回 {"清产物": bool, "构建段": "win"|"linux"|"", "静态段": bool}。"""
    if 同步rc != 0:
        return {"清产物": False, "构建段": "", "静态段": False}
    return {"清产物": 轮分支.startswith("ci/预验-"),
            "构建段": "win" if 是win else "linux",
            "静态段": True}


def 聚合绿(步骤们: dict) -> bool:
    """657（2026-10-14）：轮绿聚合口径（自 跑一轮 结果尾部抽出·逐行等价）——
    六键（同步/清产物/配置/构建/单测/e2e）中存在的键全 rc==0 才绿；缺键不计
    （未执行段不拖红）；清产物 rc=-1（target/ 残留·预验前置②不满足）即轮必红
    ——--selftest 锁定该口径（648-b 复核结论：清产物失败时后续段照跑，但总绿
    必 False，integrate 只凭「绿」字段放行=无假绿面）。"""
    return all(s.get("rc") == 0 for k, s in 步骤们.items()
               if k in ("同步", "清产物", "配置", "构建", "单测", "e2e"))


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
        #    523 口径：target/ 增量治理**不靠 clean -x 全清**（每轮全量重编=过度代价），
        #    靠 win构建链 HEAD 指纹清建——指纹不一致才 rmdir 整目录重建，一致才吃增量；
        #    clean 面维持 -fdq 不变（linux 侧 target/build 由 cmake 依赖图自洽·无此病史）。
        #    291：bash -c 复合命令拆三条裸 git 子进程——跨平台（win 不依赖 bash 在 PATH）
        同步rc, 同步输出 = 0, ""
        for 同步命令 in (["git", "fetch", 远端名, 轮分支],
                         ["git", "reset", "--hard", "-q", "FETCH_HEAD"],
                         ["git", "clean", "-fdq"]):
            同步 = 运行限时(同步命令, 300, capture_output=True, text=True, cwd=仓库根)
            同步输出 += (同步.stdout or "") + (同步.stderr or "")
            if 同步.returncode != 0:
                同步rc = 同步.returncode
                break
        步骤们["同步"] = {"rc": 同步rc}
        if 同步rc != 0:
            print(同步输出, file=日志, flush=True)
        # 657：编排决策=纯函数（--selftest 正反两态直调·单一事实源）。648-b 拆链后的
        #   语义：ci/预验-* 轮「清产物+构建段」并存——清产物是独立前序步骤而非 elif
        #   互斥分支（637 回归形态已由自测文本锁封死）；develop/兜底轮无清产物
        #   （523 增量口径）；同步失败两段均不执行。
        排 = 同步后编排(同步rc, 轮分支, 是win)
        if 排["清产物"]:
            # 637（2026-10-14 用户立法·最高优先级）：预验前必须「干净构建」——②历史编译
            #    中间产物全部清零后从零配置构建——防脏产物假绿/增量混合态不可复现
            #    （483/551 超时作废循环诱因之一）。仅 ci/预验-* 轮执行（523 增量口径对
            #    develop/兜底轮保持——每轮全清=过度代价·立法对象是预验可信面）；target
            #    全清后 win 指纹清建必然触发全量重建（自动兼容）。①rebase 新鲜（链顶含
            #    当时最新 develop）由 integrate 组链 checkout -B 批分支 <最新> 结构性
            #    保证+预验前置检查断言（integrate.py·637）。
            清干净 = 预验清产物(仓库根)
            步骤们["清产物"] = {"rc": 0 if 清干净 else -1}
            print("[637] 干净构建%s" % ("：target/（含 v2p 指纹缓存）已全清——从零构建"
                                        if 清干净 else "失败：target/ 残留（预验前置②不满足·本轮判红）"),
                  flush=True)
        # 648-b 结构修复（637 引入 elif 互斥吞构建段·TX_02 预验 3600854863 两轮
        #   实锤：预验分支命中清产物分支后 elif 链终止→配置/构建/单测整段跳过→
        #   target 刚清空 cn=None→cli_contract/asm_width/e2e 三步 rc=-1 假红·
        #   总秒 2~5。637 自身集成时 runner 尚为旧代码故绿=首个新预验任务才踩中）。
        #   构建段为独立条件（同步成功即跑），预验清产物只是其前序步骤（657 自测锁）。
        if 排["构建段"] == "win":
            # ② 291 win 构建链=vcvars64+Ninja+sccache（wt.py 开发树同口径）——
            #    cmd /c 复合命令串内嵌引号经 subprocess 列表参数转义必坏（实测 rc=1
            #    零输出）——生成 ASCII 临时脚本跑（wt.py 生成 ninja_build.cmd 同款手法；
            #    固定名覆盖写=无垃圾积累；cd /d %~dp0.. 自定位树根=四 worktree 各自独立）。
            #    523：构建/配置/单测整段抽 win构建链（指纹清建挂点）——reset 后取实际
            #    HEAD 做指纹（189 兜底过期竞态下 reset 到的可能非任务 sha·以实际构建树为准）。
            头 = 运行限时(["git", "rev-parse", "HEAD"], 30,
                          capture_output=True, text=True, cwd=仓库根)
            vcvars = 找vcvars()
            if vcvars is None:
                步骤们["配置"] = {"rc": -1, "说明": "未找到 vcvars64.bat（VS2022 安装不全）"}
            else:
                win构建链(步骤们, 步骤, 日志, vcvars, 头.stdout.strip())
        elif 排["构建段"] == "linux":
            # ② linux 构建段（与 win构建链 对称·657 抽函数=跑一轮 回 ≤100 行纪律）
            linux构建链(步骤们, 步骤)
        # ③.5 静态门禁面+④ E2E（242/447-a·两平台公共段——291 曾因缩进挂在 else 体内被
        #    win 分支整体跳过〔绿=缺键误判〕；523 抽 静态门禁与e2e段·跑一轮 拆函数回 ≤100 行）
        if 排["静态段"]:
            cn = next((p for p in ([仓库根 / "target/cn.exe", 仓库根 / "target/cn"] if 是win
                                   else [仓库根 / "target/build/cn", 仓库根 / "target/cn"])
                       if p.exists()), None)
            静态门禁与e2e段(步骤们, 步骤, cn, 日志)
        尾部 = []
    # 日志尾部摘录进结果（供 TX_01 结果页直显；完整日志在 run_*.log）
    with 轮日志路径.open("r", encoding="utf-8", errors="replace") as f:
        尾部 = f.readlines()[-40:]
    结果 = {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
            "绿": 聚合绿(步骤们),
            "总秒": round(time.time() - 开始), "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": "".join(尾部).splitlines()[-20:]}
    return 结果


def 落盘(结果: dict) -> None:
    """结果落盘+可选上报（1008·预验模式规则·770 修订）：
    - develop 常规轮：latest.json + result_*.json + 上报（既有行为）。
    - 非 develop 分支（CN_CI_BRANCH 预验）：
      · 恒写 预验_<sha10>.json（integrate.py 轮询取回的契约文件）+ result_*.json（历史档）；
      · ci/预验-* 且绿 → 额外写 latest.json+上报：链顶即将成为 develop 头（1008 方案甲；
        770 起 develop 面仍复验·此处 latest 语义=「最近被验证的头」供免跑判据）；
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
    """向池认领一个预验任务（服务端写锁内原子+心跳超时回收）。无任务/池不可达/未配置 → None。
    337：自报平台（win-x64/linux-x86_64/linux-arm64·294 三分运行时探测）——服务端按任务
    认领限制过滤，只派给白名单含本平台的任务；缺省语义（不限任务）对旧/新 runner 完全一致。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return None
    回 = 池调用(配置, "task_claim", {"runner": runner标识, "平台": 平台})
    if 回 and 回.get("ok"):
        return 回.get("任务")
    return None


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


def 兜底过期(任务sha: str, develop头sha: str) -> bool:
    """660（2026-10-14）：兜底轮跑前预检——任务 sha ≠ develop 现头（上方 fetch 的
    FETCH_HEAD）即过期（该 sha 树由新头轮隐含覆盖·189 语义）→跳过整轮零算力；
    头空（rev-parse 异常）保守不跳（fail-safe·走原整轮+跑后复查双保险）。"""
    return bool(develop头sha) and 任务sha != develop头sha


def 过期跳过结果(sha: str, 任务分支: str, 头sha: str) -> dict:
    """189 过期跳过结果构造（660：跑前预检与跑后复查共用=形状单一事实源）：绿=True
    闭环防重派·不落盘防 latest 被非任务 sha 污染（详情文案两处调用完全一致）。"""
    return {"sha": sha, "分支": 任务分支, "平台": 平台, "绿": True,
            "步骤": {"过期跳过": {"rc": 0, "说明": "develop 已前进至 %s——任务 sha 树由新头轮覆盖（189）"
                                 % 头sha[:10]}},
            "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": "", "日志尾部": []}


def 池跑任务(任务: dict) -> None:
    """执行认领到的池任务（189 起两类）：预验任务 fetch 预验分支→跑一轮→落盘；
    兜底任务（ci/兜底-*·非真分支）fetch develop→跑一轮(develop)→落盘（660：fetch 后
    FETCH_HEAD≠任务 sha 即跑前过期跳过=零整轮算力·跑后复查保留为双保险）。
    均以 complete 收尾（3 次重试）。落盘复用 落盘()（develop 兜底=latest+result+上报；
    ci/预验- 绿写 latest+上报=develop 轮免重跑联动保留）。"""
    任务分支, sha = 任务["分支"], 任务["sha"]
    兜底 = 任务分支.startswith("ci/兜底-")
    print("[池] 认领 %s（sha=%s·runner=%s·%s）" % (任务分支, sha[:10], runner标识,
          "兜底" if 兜底 else "预验"), flush=True)
    配置 = 池配置()
    停 = 启动心跳(任务分支)
    try:
        取对象 = "develop" if 兜底 else 任务分支
        取 = 运行限时(["git", "fetch", 远端名, 取对象], 300,
                      capture_output=True, text=True, cwd=仓库根)
        if 取.returncode != 0:
            # fetch 失败=本轮失败（分支可能已被 integrate finally 删除=同 sha 重试竞态）——
            # 报红让服务端回收重派；本地不落盘（非真实验证结果·防污染 result 档案）
            print("[池] fetch 失败：%s" % (取.stderr or "").strip()[:200], flush=True)
            结果 = {"sha": sha, "分支": 任务分支, "平台": 平台, "绿": False,
                    "步骤": {"同步": {"rc": 取.returncode}}, "总秒": 0,
                    "时刻": datetime.now().isoformat(timespec="seconds"),
                    "日志": "", "日志尾部": (取.stderr or "").splitlines()[-5:]}
        elif 兜底:
            # 660（2026-10-14）跑前预检：陈轮（任务 sha 已被 develop 新头淘汰）在此直接
            #   过期收场——旧序先跑整轮再判过期=陈轮烧整轮算力（池内陈轮反复认领空跑·
            #   TX_02 OOM 实录）；fetch 已在上方完成，仅 rev-parse FETCH_HEAD 与任务 sha
            #   比对（零构建·不动工作树·不 reset）。不等→过期跳过；相等→原整轮路径，
            #   轮内 develop 再前进由下方跑后复查兜底（双保险保留）。
            预检头 = 运行限时(["git", "rev-parse", "FETCH_HEAD"], 30,
                             capture_output=True, text=True, cwd=仓库根).stdout.strip()
            if 兜底过期(sha, 预检头):
                结果 = 过期跳过结果(sha, 任务分支, 预检头)
                print("[池] 兜底任务过期跳过（跑前预检·develop 已前进）：%s" % sha[:10], flush=True)
            else:
                结果 = 带门禁锁跑(sha, "develop")
                if 结果 is None:      # 291 让出（gate_lock 互忙）——不 complete·心跳停由服务端回收重派
                    print("[池] 本轮让出（gate_lock）——任务将被回收重派", flush=True)
                    return
                # 过期竞态语义化关闭（189·Rust 无数据竞争思路的服务端等价）：认领期间
                #   develop 前进→reset 到的是新头≠任务 sha——该 sha 的树已由新头轮隐含覆盖
                #   （git 树=快照·新头绿⇒含旧提交内容绿）。过期轮不落盘（防 latest 被非
                #   任务 sha 污染）·complete 绿=True 闭环（服务端不重派·详情留审计）。
                头 = 运行限时(["git", "rev-parse", "HEAD"], 30,
                              capture_output=True, text=True, cwd=仓库根).stdout.strip()
                if 头 != sha:
                    结果 = 过期跳过结果(sha, 任务分支, 头)
                    print("[池] 兜底任务过期跳过（跑后复查·develop 已前进）：%s" % sha[:10], flush=True)
                else:
                    落盘(结果)
        else:
            结果 = 带门禁锁跑(sha, 任务分支)
            if 结果 is None:      # 291 让出（gate_lock 互忙）——不 complete·心跳停由服务端回收重派
                print("[池] 本轮让出（gate_lock）——任务将被回收重派", flush=True)
                return
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
    已入池: set[str] = set()   # 770：本轮常驻期已入池的头（幂等防重——服务端幂等兜底·此处省每轮空 POST）
    # 184 锁模型治本（1021 饿死实录）：锁从「进程生命周期级」改「轮次级」——常驻每轮
    #   跑完释放（LOCK_UN）、下轮重抢；ssh 点名 --wait-lock 的等待=至多**当前轮完成**
    #   （而非等常驻进程退出）即可插队。923 OOM 铁律不变：锁窗内同机只跑一个全量。
    try:                                    # 非阻塞抢锁：cron 兜底撞上在跑轮=静默让路
        已获 = 抢锁(锁)
    except OSError:
        已获 = False
    if not 已获:
        if not 等锁:
            print("[锁] 已有轮在跑——退出。", flush=True)
            return 0
        # --wait-lock（1008·ci/预验 触发用）：等现有轮完成（184 前语义=等持锁进程退出·饿死）
        print("[锁] 已有轮在跑——等待（--wait-lock·至多当前轮）……", flush=True)
        抢锁(锁, 阻塞=True)
    while True:
        # ── 1021 池优先段：只在本职分支 develop 时进池（CN_CI_BRANCH 点名模式=ssh 直发
        #    降级路径·被点名跑指定预验分支，不抢池内别的任务）；锁在手=同机单任务
        #    （923 OOM 铁律——家机各实例各持各的锁文件）。770：所有实例统一此路径
        #    （角色区分废止——预验与 develop 验证同池认领，派发语义在服务端）。
        if 分支 == "develop":
            任务 = 池认领任务()
            if 任务:
                池跑任务(任务)
                if not 常驻:
                    return 0
                放锁(锁)      # 184：轮间释放·点名可插队
                time.sleep(轮询间隔秒)
                重抢锁(锁)
                continue
        # ── develop 轮询（770：所有实例统一——发现新头→入池派发；头变化必验不可跳过）
        sha = 取远端SHA()
        if sha is None:
            print("[警告] ls-remote 失败——下轮重试", flush=True)
        else:
            # ── 189 纯文档免兜底（770 保留：变更集全 .md→免跑留档·编译器零变化不烧全量；
            #    其余任何头一律入池真验——「已跑过/池已绿」跳过已废止=预验绿过的头仍复验）。
            #    入池失败（池不可达）降级本机跑=门禁永不缺位；本地已有该 sha 红档不入池
            #    （终态已记录·重跑同红无益，新头自然覆盖）。
            if 分支 == "develop" and not 强制:
                基线 = 上次已验SHA()
                if 基线 and 基线 != sha:
                    免, 缘由 = 纯文档变更(基线, sha)
                    if 免:
                        免跑留档(sha, 缘由)
                        if not 常驻:
                            return 0
                        放锁(锁)      # 184：轮间释放·点名可插队
                        time.sleep(轮询间隔秒)
                        重抢锁(锁)
                        continue
                if (sha not in 已入池
                        and not list(日志目录.glob("result_%s_*.json" % sha[:10]))
                        and 兜底入池(sha)):
                    已入池.add(sha)   # 入池成功即记（服务端幂等终态兜底·常驻期不重复 POST）
                    if not 常驻:
                        return 0
                    放锁(锁)
                    time.sleep(轮询间隔秒)
                    重抢锁(锁)
                    continue
            print("[开跑] %s @ %s（分支=%s%s）" % (sha[:10],
                  datetime.now().strftime("%H:%M:%S"), 分支,
                  "·预验" if 分支 != "develop" else ""), flush=True)
            # 190（1035 轮）：开跑即上报「执行中」——状态页门禁表实时可见在跑轮。
            #   1030 轮误判「六次推送漏跑」实录：1759d685 兜底 16:18-16:57 在跑，
            #   运营者 16:2x 查状态页只见完成轮（执行中不可见）→误立 190。
            #   完成轮由 落盘 上报覆盖同 sha 行（INSERT OR REPLACE·绿/红/耗时齐）。
            if 分支 == "develop":
                上报({"sha": sha, "分支": 分支, "平台": 平台,
                      "绿": None, "总秒": None, "状态": "执行中"})
            结果 = 带门禁锁跑(sha)
            if 结果 is not None:     # None=291 让出（gate_lock 互忙）——不落盘·下轮再战
                落盘(结果)
                print("[完成] 绿=%s 总秒=%s 详情=%s" % (结果["绿"], 结果["总秒"], 结果["日志"]), flush=True)
        if not 常驻:
            return 0
        放锁(锁)                                      # 184：轮间释放·点名可插队
        time.sleep(轮询间隔秒)
        重抢锁(锁)                                   # 184：非阻塞重抢（抢不到=他轮在跑·等它完成）


def 重抢锁(锁) -> None:
    """184：常驻轮间非阻塞重抢——抢不到=点名预验/兜底在跑，等它完成（90s 步进）。
    轮次级锁核心：ssh 点名 --wait-lock 至多等到当前轮完成即可插队（923 铁律由锁窗保持）。"""
    while not 抢锁(锁):
        print("[锁] 他轮在跑（点名预验/兜底）——%ds 后再试。" % 轮询间隔秒, flush=True)
        time.sleep(轮询间隔秒)


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        # 660（2026-10-14）：自测体外迁 ci_daemon_selftest.py（行数红线·单一职责）——
        #   仅此分支惰性导入：runner 常驻进程树盘可能为旧版无此模块，顶层导入会炸启动。
        import ci_daemon_selftest
        raise SystemExit(ci_daemon_selftest.自测(sys.modules[__name__]))
    常 = "--loop" in sys.argv
    强 = "--force" in sys.argv
    等锁 = "--wait-lock" in sys.argv
    if "--once" not in sys.argv and not 常:
        print("用法: ci_daemon.py --loop | --once [--force] [--wait-lock]；无参默认 --once")
    raise SystemExit(主(常, 强, 等锁))
