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
# 189 兜底池化（2026-10-07 用户裁决丙案）：develop 轮真跑前两道优化——①纯文档轮免兜底
#   （基线..新头 变更集全 .md→免跑留档·result 档案幂等防重复判定·任何异常/非 md 文件
#   照跑=fail-safe 默认）②混合变更入池（task_enqueue ci/兜底-<sha10>·runner 池谁闲谁
#   认领·TX_02 本职轮下周期也参与认领·入池失败降级本机跑=门禁永不缺位·本地有红档不入
#   池防池内死循环重跑）。--force 每日 cron 与点名预验不受影响=门禁语义保持。
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
# 兼容 python3.8：注解泛型下标/联合字符串化（287·同 gate_quick 修复缘由）。
from __future__ import annotations

import json
import os
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
平台 = "win-x64" if 是win else "linux-x86_64"   # run_e2e --target 与结果上报共用
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
角色 = os.environ.get("CN_CI_ROLE", "")            # "pool"=纯池角色（家机实例·不轮询 develop）
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
    输出 = subprocess.run(["git", "ls-remote", 远端名, "refs/heads/" + 分支],
                          capture_output=True, text=True, cwd=仓库根, timeout=60)
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
    """纯文档轮免跑留档（189）：走既有 result 档案机制（develop 语义=latest+result+上报）
    ——「已跑过(sha)」据此自动跳过=幂等零重复判定；结果页可审计（绿=门禁无需全量·
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
    """②兜底轮入池（189）：develop 新头提交 ci/兜底-<sha10> 任务给 runner 池
    （谁空闲谁认领·TX_02 本职轮下周期也会认领）。入池失败（池不可达/拒绝）→ False
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


def 运行(命令: list[str], 日志, 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """流式写日志（不落内存大块）+总超时看护。"""
    print("  $", " ".join(命令[:6]), ("..." if len(命令) > 6 else ""), file=日志, flush=True)
    return subprocess.run(命令, stdout=日志, stderr=subprocess.STDOUT,
                          cwd=仓库根, timeout=超时秒, **kwargs)


def 找vcvars() -> Path | None:
    """vcvars64.bat 探测（291·复刻 wt.py 同款四版本候选）。"""
    for 版本 in ("Community", "Professional", "Enterprise", "BuildTools"):
        候选 = Path("C:/Program Files/Microsoft Visual Studio/2022/%s/VC/Auxiliary/Build/vcvars64.bat" % 版本)
        if 候选.exists():
            return 候选
    return None


def 带门禁锁跑(sha: str, 轮分支: str = "") -> dict | None:
    """291 win 三段外套 gate_lock：家机同机有人工开发全量门禁（与 TX_02 独占 CI 场景
    不同）——win 池实例三段与主树门禁全机互斥（AGENTS §7 本机全量每机 ≤1；池树经
    CN_GATE_LOCK_DIR 指向主树锁目录·跨树共享）。30 分钟未获得=让出（返回 None）：
    调用方不落盘不 complete——心跳停由服务端 300s 回收重派（本机忙让给别人=随机池本义）。
    linux 零开销直通（TX_02/WSL 行为不变·原注释口径「独占 CI 不套锁」保持）。"""
    门禁锁 = gate_lock.获取(1800, 形态="acquire", owner_pid=os.getpid()) if 是win else None
    if 是win and 门禁锁 is None:
        print("[让出] gate_lock 30 分钟未获得（本机门禁互忙）——本轮放弃，任务由服务端回收重派", flush=True)
        return None
    try:
        return 跑一轮(sha, 轮分支)
    finally:
        if 是win:
            gate_lock.释放(门禁锁, 静默=True)


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
        #    291：bash -c 复合命令拆三条裸 git 子进程——跨平台（win 不依赖 bash 在 PATH）
        同步rc, 同步输出 = 0, ""
        for 同步命令 in (["git", "fetch", 远端名, 轮分支],
                         ["git", "reset", "--hard", "-q", "FETCH_HEAD"],
                         ["git", "clean", "-fdq"]):
            同步 = subprocess.run(同步命令, capture_output=True, text=True,
                                  cwd=仓库根, timeout=300)
            同步输出 += (同步.stdout or "") + (同步.stderr or "")
            if 同步.returncode != 0:
                同步rc = 同步.returncode
                break
        步骤们["同步"] = {"rc": 同步rc}
        if 同步rc != 0:
            print(同步输出, file=日志, flush=True)
        elif 是win:
            # ② 291 win 构建链=vcvars64+Ninja+sccache（wt.py 开发树同口径）；首配仅
            #    build.ninja 缺失时做（每轮 reset/clean 不动 target/=增量天然保留）。
            #    cmd /c 复合命令串内嵌引号经 subprocess 列表参数转义必坏（实测 rc=1
            #    零输出）——生成 ASCII 临时脚本跑（wt.py 生成 ninja_build.cmd 同款手法；
            #    固定名覆盖写=无垃圾积累；cd /d %~dp0.. 自定位树根=四 worktree 各自独立）
            vcvars = 找vcvars()
            if vcvars is None:
                步骤们["配置"] = {"rc": -1, "说明": "未找到 vcvars64.bat（VS2022 安装不全）"}
            else:
                头行 = ["@echo off", 'cd /d "%~dp0.."',
                        'call "%s" >nul 2>&1' % vcvars,
                        'where cl >nul 2>&1 || (echo [FAIL] cl not found & exit /b 1)']
                if not (仓库根 / "target/build-ninja/build.ninja").exists():
                    launcher = ""
                    sccache = shutil.which("sccache")
                    if sccache:
                        launcher = ' -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="%s"' % sccache
                    配置脚本 = 日志目录 / "win_configure.cmd"
                    # CXX_FLAGS_DEBUG 必须显式 /Z7（wt.py 开发树同口径）——默认 /ZI 集中 PDB
                    # 在 --parallel 下多 cl.exe 并写同 .pdb 必撞 C1041（实测）
                    配置脚本.write_text("\r\n".join(头行 + [
                        'cmake -G Ninja -S . -B target/build-ninja '
                        '-DCMAKE_BUILD_TYPE=Debug '
                        '-DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1" ' + launcher])
                        + "\r\n", encoding="ascii")
                    步骤("配置", ["cmd", "/c", str(配置脚本)], 900)
                else:
                    步骤们["配置"] = {"rc": 0, "说明": "build-ninja 已配置（增量）"}
                if 步骤们["配置"]["rc"] == 0:
                    构建脚本 = 日志目录 / "win_build.cmd"
                    构建脚本.write_text("\r\n".join(头行 + [
                        "cmake --build target/build-ninja --parallel"]) + "\r\n",
                        encoding="ascii")
                    步骤("构建", ["cmd", "/c", str(构建脚本)], 单轮总超时秒)
                    # ③ 单测（产物落 target/ 根=CMAKE_RUNTIME_OUTPUT_DIRECTORY·exe 后缀）
                    单测 = next((p for p in [仓库根 / "target/cn_unit_tests.exe",
                                             仓库根 / "target/build/cn_unit_tests.exe",
                                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
                    if 单测 is None:
                        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
                    else:
                        步骤("单测", [str(单测)], 1800)
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
        # ③.5 静态门禁面（242·ci常规五项：spec 覆盖/CLI 契约/asm 位宽/台账完成度/行数冻结线
        #    ——两平台公共段·win 静态检查全可跑〔ci.ps1 实证三项〕；291 从 else 体内提出=
        #    elif 是win 分支同样必经——曾因缩进挂在 else 内被 win 分支整体跳过〔绿=缺键误判〕）
        #    cn 产物定位必须先于本段（原赋值在 ④ 段晚于引用=UnboundLocalError 必崩·259 修）
        if 同步rc == 0:
            cn = next((p for p in ([仓库根 / "target/cn.exe", 仓库根 / "target/cn"] if 是win
                                   else [仓库根 / "target/build/cn", 仓库根 / "target/cn"])
                       if p.exists()), None)
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
            # ④ E2E（探测平台面；红后串行复验=447-a 口径；cn 复用上文定位）
            if cn is None:
                步骤们["e2e"] = {"rc": -1, "说明": "未找到 cn 产物"}
            elif 步骤们.get("构建", {}).get("rc") == 0:
                # 924·运维小件：max-mem 按物理内存适配（4096 默认>3.6G 物理=形同虚设——920 预判）；
                #   E2E 全程 nice -n 10 降优先级（CI 高负载 sshd 饿死 banner 超时实锤·运维手册④）
                #   291：nice 为 posix 专属——win 无此前缀（同机互斥已由 gate_lock 承担）；
                #   --target 用运行时探测平台（win-x64 / linux-x86_64）
                保内存 = min(4096, int(物理内存MB() * 0.8))
                e2e基 = ([] if 是win else ["nice", "-n", "10"]) + [
                    sys.executable, "tests/e2e/run_e2e.py",
                    "--target", 平台, "--cn", str(cn),
                    "--max-mem-mb", str(保内存),
                    "--full-reason", "ci_daemon 云端全量门禁（TX_02/池 runner）"]
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
    结果 = {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
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
    """执行认领到的池任务（189 起两类）：预验任务 fetch 预验分支→跑一轮→落盘；
    兜底任务（ci/兜底-*·非真分支）fetch develop→跑一轮(develop)→落盘。
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
        取 = subprocess.run(["git", "fetch", 远端名, 取对象], capture_output=True,
                            text=True, cwd=仓库根, timeout=300)
        if 取.returncode != 0:
            # fetch 失败=本轮失败（分支可能已被 integrate finally 删除=同 sha 重试竞态）——
            # 报红让服务端回收重派；本地不落盘（非真实验证结果·防污染 result 档案）
            print("[池] fetch 失败：%s" % (取.stderr or "").strip()[:200], flush=True)
            结果 = {"sha": sha, "分支": 任务分支, "平台": 平台, "绿": False,
                    "步骤": {"同步": {"rc": 取.returncode}}, "总秒": 0,
                    "时刻": datetime.now().isoformat(timespec="seconds"),
                    "日志": "", "日志尾部": (取.stderr or "").splitlines()[-5:]}
        elif 兜底:
            结果 = 带门禁锁跑(sha, "develop")
            if 结果 is None:      # 291 让出（gate_lock 互忙）——不 complete·心跳停由服务端回收重派
                print("[池] 本轮让出（gate_lock）——任务将被回收重派", flush=True)
                return
            # 过期竞态语义化关闭（189·Rust 无数据竞争思路的服务端等价）：认领期间
            #   develop 前进→reset 到的是新头≠任务 sha——该 sha 的树已由新头轮隐含覆盖
            #   （git 树=快照·新头绿⇒含旧提交内容绿）。过期轮不落盘（防 latest 被非
            #   任务 sha 污染）·complete 绿=True 闭环（服务端不重派·详情留审计）。
            头 = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True,
                                text=True, cwd=仓库根, timeout=30)
            if 头.stdout.strip() != sha:
                结果 = {"sha": sha, "分支": 任务分支, "平台": 平台, "绿": True,
                        "步骤": {"过期跳过": {"rc": 0, "说明": "develop 已前进至 %s——任务 sha 树由新头轮覆盖（189）"
                                             % 头.stdout.strip()[:10]}},
                        "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
                        "日志": "", "日志尾部": []}
                print("[池] 兜底任务过期跳过（develop 已前进）：%s" % sha[:10], flush=True)
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
        #    （923 OOM 铁律——家机 7 实例各持各的锁文件）。
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
            if 角色 == "pool":      # 纯池角色（家机实例）无任务→空闲休眠
                if not 常驻:
                    return 0
                放锁(锁)
                time.sleep(轮询间隔秒)
                重抢锁(锁)
                continue
        # ── 原有轮询（TX_02 develop 本职 / CN_CI_BRANCH 点名预验）
        sha = 取远端SHA()
        if sha is None:
            print("[警告] ls-remote 失败——下轮重试", flush=True)
        elif (已跑过(sha) or 池已绿(sha)) and not 强制:   # 1021：并查池绿记录（预验可能他实例跑的）
            print("[跳过] %s 已跑过" % sha[:10], flush=True)
        else:
            # ── 189 兜底池化（2026-10-07 用户裁决丙案）：develop 轮真跑前两道优化——
            #    ①纯文档轮免兜底（变更集全 .md→免跑留档·幂等防重复判定）
            #    ②混合变更入池（runner 池谁闲谁跑·入池失败降级本机=门禁永不缺位）。
            #    --force（每日 cron 兜底）与点名预验分支不受影响=门禁语义保持。
            #    本地已有红档不入池（防池内死循环重跑红兜底·重跑由本机承担=原行为）。
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
                if not list(日志目录.glob("result_%s_*.json" % sha[:10])) and 兜底入池(sha):
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
    常 = "--loop" in sys.argv
    强 = "--force" in sys.argv
    等锁 = "--wait-lock" in sys.argv
    if "--once" not in sys.argv and not 常:
        print("用法: ci_daemon.py --loop | --once [--force] [--wait-lock]；无参默认 --once")
    raise SystemExit(主(常, 强, 等锁))
