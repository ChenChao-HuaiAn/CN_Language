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
# 973 对账工具周期化挂载（2026-10-17）：兜底/develop 面轮静态段后挂「对账周检段」
#   （ci_reconcile.py·每周到期跑两对账工具 --strict·红步骤播报不拖轮绿）；构建链外迁 ci_build。
# 1392 实例空闲报活（2026-10-22）：daemon 空闲也每 60s POST instance_ping+版本指纹
#   （树头+ci_daemon.py 内容 sha256·启动时算一次）——8300 实例页签实时动态数据源；
#   实现在 ci_shard（池协议客户端同域单点·本文件零实现）。
# 兼容 python3.8：注解泛型下标/联合字符串化（287·同 gate_quick 修复缘由）。
from __future__ import annotations

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
import ci_round_guard   # 578：轮守卫（子进程追踪+轮心跳）——运行/运行限时 底层接线
import ci_proc          # 935：轮级子进程树生命线（Job Object/进程组·超时整树回收）
import ci_build         # 973：win/linux 构建链（523/657 原样外迁·薄壳见下）
import ci_reconcile     # 973：对账周检段（兜底轮周期对账·周检挂载点）
import ci_shard         # 1297：预验分片协议+池 HTTP 客户端单点（依赖单向）
import ci_m6            # #1221：M6 写集触发+E2E 段（判定/编排外迁·守 900 行警戒线）
import ci_static_gates   # 1148：静态门禁清单外迁（660 行数红线·#1221 拆分先例同构）
import ci_round         # #1221 复审：轮结果组装/直跑收口（外迁守 1000 行）
import ci_fallback      # 1297：develop 兜底面（189/660/770 外迁·薄壳见下）
import ci_duration      # 1385：步骤口径元信息（脚本→daemon 回传·耗时基线分桶）
import ci_report        # 1297：报告与留档面（落盘/上报/提交历史 外迁·薄壳见下）

# ===== 可调常量（集中区·911 先例） =====
轮询间隔秒 = 90            # git ls-remote 周期
e2e并行 = int(os.environ.get("CN_E2E_JOBS", "6" if 是win else "3"))
                                                        # TX_02 4C3.6G=3；家机 WSL2 实例 env=4；
                                                        # win 4 实例峰值 4×6=24 jobs+构建<28 线程（291）
构建并行 = int(os.environ.get("CN_BUILD_JOBS", "2"))    # 首验实锤：全核 Make 编译 cc1plus 叠加 OOM
                                                        # （9daba·10-01）——TX_02 限 2；家机 8G/实例=4
单轮总超时秒 = 3 * 3600    # 防挂死（构建+单测+E2E+串行复验的理论上界）
e2e整轮超时秒 = int(os.environ.get("CN_E2E_TIMEOUT_SEC", "10800"))
# 935（2026-10-17）：E2E 步骤专用整轮看门狗（原默认 90 分钟=win 全量 ~33 分钟+
# 重负载长尾余量）——超时=整树回收+步骤 rc=-99 诚实红，不再悬空 3h 僵轮。
# 1404（2026-10-23·P0 前置）：大写集批（1360 批2 ir 31 文件/批3A semantic 50 文件）
#   全量分片集实测 5577~5603s 收不齐·四连红零用例失败（纯收片超时）——默认窗
#   90→180 分钟（env 可覆盖·单轮总超时 3h 上限兜底防挂死不变）。
远端名 = "origin"
分支 = os.environ.get("CN_CI_BRANCH", "develop")   # 常规盯 develop；任务分支预验=PR CI 同款用法
角色 = os.environ.get("CN_CI_ROLE", "")            # 770 起不再区分（统一池·env 仅供 run_e2e CI 识别与旧部署兼容）
runner标识 = os.environ.get("CN_RUNNER_ID", socket.gethostname())   # 池内身份（认领/心跳/完成）
心跳间隔秒 = 60            # task_heartbeat 周期（服务端 CN_TASK_STALE_SEC=300 回收阈值）
仓库根 = Path(__file__).resolve().parent.parent
日志目录 = 仓库根 / "ci-logs"
锁文件 = 仓库根 / "ci-logs" / "daemon.lock"


# 抢锁/放锁 → 1297 薄壳别名（实现外迁 gate_lock.py 同域文件锁区·ci_daemon 守 1000 行）
抢锁 = gate_lock.抢锁
放锁 = gate_lock.放锁


def 取远端SHA() -> str | None:
    """git ls-remote 只查 develop 头——零流量无副作用。"""
    输出 = 运行限时(["git", "ls-remote", 远端名, "refs/heads/" + 分支],
                    60, capture_output=True, text=True, cwd=仓库根)
    匹配 = re.search(r"^([0-9a-f]{40})\t", 输出.stdout, re.M)
    return 匹配.group(1) if 匹配 else None


# ═══════════════ 1297 拆分薄壳（兜底面/报告面/池协议 外迁保调用面零改） ═══════════════
# 兜底面（189/660/770）→ ci_fallback.py；报告面（落盘/上报/留档/提交历史）→
#   ci_report.py；池 HTTP 客户端 → ci_shard.py（分片协议同域单点）。动机=ci_daemon
#   加分片协议时守 1000 行硬红线（「先拆再加」·973 外迁 ci_build 同款）。
#   初始化注入运行环境（仓库根/平台/日志目录/远端名·两模块同参）。
ci_fallback.初始化(仓库根, 平台, 日志目录, 远端名)
ci_report.初始化(仓库根, 平台, 日志目录, 远端名)
上次已验SHA = ci_fallback.上次已验SHA
纯文档变更 = ci_fallback.纯文档变更
兜底入池 = ci_fallback.兜底入池
兜底过期 = ci_fallback.兜底过期
过期跳过结果 = ci_fallback.过期跳过结果
免跑留档 = ci_report.免跑留档
落盘 = ci_report.落盘
上报 = ci_report.上报
上报执行中 = ci_report.上报执行中
取提交历史 = ci_report.取提交历史
提交时刻 = ci_report.提交时刻
池配置 = ci_shard.池配置
池调用 = ci_shard.池调用


def 运行限时(命令: list[str], 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """294/578：超时安全壳（rc=-99 不炸进程·轮守卫追踪）。主体外迁 ci_round_guard
    （1000 行红线·660 自测外迁先例）——猴补/调用面不变。"""
    return ci_round_guard.运行限时(命令, 超时秒, **kwargs)


def 运行(命令: list[str], 日志, 超时秒: int, **kwargs) -> subprocess.CompletedProcess:
    """297/578/935：流式日志运行（rc=127 缺可执行·轮守卫追踪·整树生命线）。
    主体外迁 ci_round_guard（1000 行红线）；其 跑追踪 底层经 ci_proc.起子（Job
    Object 整树·超时 ci_proc.超时回收+[超时回收] 审计）＝578 轮追踪与 935 树回收
    双立（融合点详情见 scripts/ci_round_guard.py 跑追踪 docstring）。"""
    return ci_round_guard.运行(命令, 日志, 超时秒, 仓库根, **kwargs)


# 973 拆分：win/linux 构建链整段外迁 ci_build.py（523/657 逻辑原样搬移）——ci_daemon
#   998 行超 AGENTS §3 警戒线（900）之「先拆再加」；本区=薄壳/别名保自测与调用面
#   零改（578 运行体、660 自测体外迁先例）；超时/并行常量单源仍在 daemon（911）。
找vcvars = ci_build.找vcvars
需指纹清建 = ci_build.需指纹清建
win头行 = ci_build.win头行
win配置行 = ci_build.win配置行
win构建行 = ci_build.win构建行


def win构建链(步骤们: dict, 步骤, 日志, vcvars: Path, 当前头: str) -> None:
    """973 薄壳：实现见 ci_build.win构建链（523 原样·单轮总超时秒注入单源）。"""
    return ci_build.win构建链(步骤们, 步骤, 日志, vcvars, 当前头, 单轮总超时秒)


def linux构建链(步骤们: dict, 步骤, 日志=None) -> None:
    """973 薄壳：实现见 ci_build.linux构建链（657 原样·并行/超时常量注入单源；
    1299 增日志参数=编译器缓存选择留痕·None 兼容自测桩调用面）。"""
    return ci_build.linux构建链(步骤们, 步骤, 构建并行, 单轮总超时秒, 日志)


def e2e命令基(cn: Path, 保内存: int, 须M6: bool = False) -> list:
    """1297 提取：e2e 命令公共段（主实例全量/分片与从实例片轮三处共用·纪律二）——
    1369 起实现外迁 ci_shard（分片编排同域单点）；本薄壳保调用面零改。
    566 注释原文见 静态门禁与e2e段（放行通道语义不变）；#1221 须M6→--require-m6。"""
    return ci_shard.e2e命令基(cn, 保内存, 平台, 是win, 须M6=须M6)


def e2e明细参(明细路径) -> list:
    """1297 提取：--json 明细参数（跨版本兼容窗口防护·e2e支持json 探测）——
    分片形态下各片写各自临时明细（主实例聚合后写回主明细路径）。
    1369 起实现外迁 ci_shard（薄壳保调用面）。"""
    return ci_shard.e2e明细参(明细路径)



def 静态门禁与e2e段(步骤们: dict, 步骤, cn: Path | None, 日志, sha: str,
                    轮分支: str = "", 分片: int = 0,
                    分片召唤时刻: float | None = None,
                    m6计划: dict | None = None) -> dict:
    """③.5 静态门禁面+④ E2E（242/447-a/924 原内联于 跑一轮·523 抽出=跑一轮
    拆函数回 ≤100 行）——两平台公共段；cn 产物定位必须先于本段（原赋值在 ④ 段
    晚于引用=UnboundLocalError 必崩·259 修）。逻辑逐行保持原样。
    728t（2026-10-14）：523 抽出时漏传 日志 形参——e2e 并行红打 422 行 [复验] 提示
    行即 NameError 崩轮（wt2/wt3 双实例实录·红轮永不落盘/不上报→服务端 300s 回收
    重派=池算力黑洞第二形态）。修=签名补 日志 + 调用点传参；selftest 加自由名锚
    （通用·防抽函数漏参族再犯）+ 422 路径冒烟两态。
    842（2026-10-15）：e2e 命令带 --json 明细（失败用例/期望实际 diff·8300 页面
    红行详情数据源）——sha 形参供明细文件命名；传参前探 run_e2e 源码防跨版本
    窗口（新 daemon × 旧头工作树=未知参数假红·详见 e2e支持json）。
    992（2026-10-17）：挂「网站示例verify」恒跑步（官网文档示例=契约面·改语义/
    语法必须同步官网数据）——本地 L1 仅写集触发，云端全量面为唯一恒覆盖点；
    平台=gen_docs 运行时原生探测（交叉模拟实例不参与 qemu 面——示例验证非平台
    覆盖职责）；不传 env（假步骤冒烟桩三参签名兼容·产物经 找编译器 的
    target/cn(.exe) 定位）。
    1369（2026-10-21）：分片召唤时刻=宽限窗锚点（池跑任务注入）——召唤后
    CN_SHARD_GRACE_SEC 内无认领立即退化单实例序列（详见 ci_shard.主实例分片段）。
    #1221（2026-10-23）：返回 M6 计划（写集含编译器产物面（codegen/opt/ir/model/协议常量/stdlib） ⇒ 本轮必须真跑
    79_v2）——e2e 段外迁 ci_m6.e2e段；本函数只做静态门禁/verify + 转交。
    """
    # #1221：M6 写集判定（纯判定单点 m6_gate + git 胶水 ci_m6）——预验轮基线=
    #   origin/develop（三点·链顶=合并结果）；develop/兜底轮=上次已验绿 sha。
    #   调用方（跑一轮/跑分片轮）已算好则直用（结果记录与 e2e 命令同一计划）。
    m6计划 = m6计划 or ci_m6.轮计划(轮分支 or 分支, sha, 平台, 是win, 仓库根, 远端名, 机架)
    for 名, 参 in ci_static_gates.静态门禁清单(cn, 平台):
        if cn is None and "--cn" in 参:
            步骤们[名] = {"rc": -1, "说明": "未找到 cn 产物·跳过"}
            continue
        # 1300：asm_width 写集增量——仅预验轮传组链基线（分支名内嵌**链顶 sha10**
        #   〔非 develop tip·#1221 复审更正〕；M6 轮为 ci/预验-<链顶10>-m6<hex>，
        #   由 ci_round.预验分支sha10 取前 10 位）；develop/兜底轮不带=恒全扫。
        #   纯用例/文档轮由此从全扫 ~645s 降至目录扫/跳过（秒级）；编译器面轮仍全扫。
        #   #1519（已立·另轮处理）：该基线取链顶致 diff 为空=静默跳过，本轮仅更正表述。
        if 名 == "check_asm_width" and 轮分支.startswith("ci/预验-"):
            分支sha10 = ci_round.预验分支sha10(轮分支)
            if 分支sha10:
                参 = 参 + ["--基线", 分支sha10]
        if 步骤(名, [sys.executable, f"scripts/{名}.py"] + 参, 900):
            pass
    # ③.6 网站示例 verify（992）：文档示例包壳编译+运行+stdout 比对（239 例实测
    #   linux 36s）——红=文档与编译器漂移（语义/语法改而未同步 website 数据）或
    #   编译器回归显形。恒跑（云端全量=唯一恒覆盖面·本地 L1 仅写集触发）。
    if cn is None:
        步骤们["网站示例verify"] = {"rc": -1, "说明": "未找到 cn 产物·跳过"}
    elif 步骤("网站示例verify",
              [sys.executable, "website/tools/gen_docs.py", "--verify"], 900):
        pass
    # ④ E2E（探测平台面；红后串行复验=447-a 口径）
    # #1221：E2E 段外迁 ci_m6.e2e段（1297/1369/842/902 语义逐行保持）——只为守
    #   本文件 900 行警戒线并单点挂 M6 门禁：写集触发轮向 e2e 传 --require-m6，
    #   执行记录（md5/首个差异字节）从 --json 明细回填进 m6计划["执行"]。
    if cn is None:
        步骤们["e2e"] = {"rc": -1, "说明": "未找到 cn 产物"}
    elif 步骤们.get("构建", {}).get("rc") == 0:
        # 842：明细文件先清残留（本轮未跑到 e2e 时读侧不误读旧明细）；同路径串行
        #   复验轮覆盖并行轮=读到的恒是最终轮（与 步骤们["e2e"] 取最终 rc 同口径）。
        #   1297 分片形态下各片写各自临时明细，聚合版在收片完成后写回本路径。
        e2e明细路径 = 日志目录 / ("e2e明细_%s.json" % sha[:10])
        try:
            e2e明细路径.unlink()
        except OSError:
            pass
        ci_m6.e2e段(
            步骤们, 步骤, cn, 日志, sha, 轮分支, 分片, 分片召唤时刻, m6计划,
            是win=是win, 平台=平台, 日志目录=日志目录, e2e并行=e2e并行,
            e2e超时秒=e2e整轮超时秒, runner标识=runner标识, 启动心跳=启动心跳,
            e2e明细路径=e2e明细路径)
    return m6计划


# 842：e2e 明细读写/探测 → 1369 起实现外迁 ci_shard（分片编排同域单点），
#   本薄壳别名保调用面与 842 自测零改。
e2e支持json = ci_shard.e2e支持json
读e2e明细 = ci_shard.读e2e明细


# 取提交历史/提交时刻 → 1297 薄壳别名（见文件头部拆分薄壳区·实现外迁 ci_report.py）


def 带门禁锁跑(sha: str, 轮分支: str = "", 跑器=None, 分片轮: bool = False) -> dict | None:
    """291 win 三段外套 gate_lock：家机同机有人工开发全量门禁（与 TX_02 独占 CI 场景
    不同）——win 池实例三段与主树门禁全机互斥（AGENTS §7 本机全量每机 ≤1；池树经
    CN_GATE_LOCK_DIR 指向主树锁目录·跨树共享）。30 分钟未获得=让出（返回 None）：
    调用方不落盘不 complete——心跳停由服务端 300s 回收重派（本机忙让给别人=随机池本义）。
    linux 零开销直通（TX_02/WSL 行为不变·原注释口径「独占 CI 不套锁」保持）。
    566（审查 P0-2）：持锁期间向本进程环境注入 CN_GATE_LOCK_HELD=1——嵌套链子进程
    （run_e2e --allow-heavy / gate_lock acquire·run 等）见之即跳过取机器锁，
    防「父进程已持锁·子进程再取同一把」无限自死锁（子进程经 os.environ 继承）；
    finally 恢复原值（长期驻留 daemon 的环境不被污染）。
    578（任务 #578）：获取登记「轮心跳」（owner=常驻 daemon 永活使 917 锚判据对
    「锚活+轮卡」退化 4h）——持锁跑轮期间 轮守卫.起 启动轮心跳线程，按轮内活跃
    子进程+主线程进度续约；轮卡死/僵死（子进程消失等）停续约→排队方按 gate_lock
    判据③ 分钟级接管（实录：win1 daemon 僵死 55 分钟锁无可清判据）。
    1297：跑器=sha,轮分支 二参 callable（缺省 跑一轮）——从实例片轮传
    lambda s, b: 跑分片轮(...)。
    #1424：分片轮=True → 取**本实例树锁**（gate_lock.树锁目录()=本池树 target/tree.lock，
    wt1/2/3 各自独立）而非主树共享 gate.lock——片=构建+~192 例轻负载、各树 target 独立，
    同机三 win 可并行跑片（治「只有 win 有锁竞争」）；整任务/兜底全量仍取主树共享锁
    （923 OOM 防线保留）。"""
    跑器 = 跑器 or 跑一轮
    锁目录 = gate_lock.树锁目录() if (是win and 分片轮) else None
    门禁锁 = gate_lock.获取(1800, 形态="acquire", owner_pid=os.getpid(),
                          轮心跳=True, 锁=锁目录) if 是win else None
    if 是win and 门禁锁 is None:
        print("[让出] gate_lock 30 分钟未获得（本机门禁互忙）——本轮放弃，任务由服务端回收重派", flush=True)
        return None
    豁免原值 = os.environ.get("CN_GATE_LOCK_HELD")
    if 是win:
        os.environ["CN_GATE_LOCK_HELD"] = "1"
    try:
        if 是win:
            ci_round_guard.守.起(门禁锁)  # 578：轮心跳续约（探测=活跃子进程+进度）
        return 跑器(sha, 轮分支)
    finally:
        if 是win:
            ci_round_guard.守.停()        # 578：先停续约再释放（防释放后线程复活写残留）
            if 豁免原值 is None:
                os.environ.pop("CN_GATE_LOCK_HELD", None)
            else:
                os.environ["CN_GATE_LOCK_HELD"] = 豁免原值
            gate_lock.释放(门禁锁, 静默=True)


def 步骤记录器(步骤们: dict, 日志):
    """1297 提取：步骤封装工厂（跑一轮 与 跑分片轮 共用·纪律二）——流式日志运行+
    rc/秒记录+超时 rc=-99 诚实红（297/578/935 口径原样·逐行保持·1385 并入脚本口径元信息）。"""
    def 步骤(名: str, 命令: list[str], 超时秒: int):
        t0 = time.time()
        try:
            r = 运行(命令, 日志, 超时秒)
            步骤们[名] = {"rc": r.returncode, "秒": round(time.time() - t0),
                          **ci_duration.读步骤元信息(名, t0)}
        except subprocess.TimeoutExpired:
            步骤们[名] = {"rc": -99, "秒": 超时秒, "超时": True}
        return 步骤们[名]["rc"] == 0
    return 步骤


def 执行同步(日志, 轮分支: str) -> tuple:
    """1297 提取：①同步代码到该 SHA（523/291 口径原样·三 git 子命令跨平台）——
    跑一轮 与 跑分片轮 共用。返回 (同步rc, 同步输出)。"""
    同步rc, 同步输出 = 0, ""
    for 同步命令 in (["git", "fetch", 远端名, 轮分支],
                     ["git", "reset", "--hard", "-q", "FETCH_HEAD"],
                     ["git", "clean", "-fdq"]):
        同步 = 运行限时(同步命令, 300, capture_output=True, text=True, cwd=仓库根)
        同步输出 += (同步.stdout or "") + (同步.stderr or "")
        if 同步.returncode != 0:
            同步rc = 同步.returncode
            break
    return 同步rc, 同步输出


def 构建段执行(排: dict, 步骤们: dict, 步骤, 日志) -> None:
    """1297 提取：②平台构建段（win=vcvars+Ninja 链·linux=cmake 链·657 编排原样）——
    跑一轮 与 跑分片轮 共用；同步成功才进（调用方判 排["构建段"]）。"""
    if 排["构建段"] == "win":
        # ② 291 win 构建链=vcvars64+Ninja+sccache（523：构建/配置/单测整段抽
        #    win构建链——reset 后取实际 HEAD 做指纹·189 兜底过期竞态下以实际构建树为准）
        头 = 运行限时(["git", "rev-parse", "HEAD"], 30,
                      capture_output=True, text=True, cwd=仓库根)
        vcvars = 找vcvars()
        if vcvars is None:
            步骤们["配置"] = {"rc": -1, "说明": "未找到 vcvars64.bat（VS2022 安装不全）"}
        else:
            win构建链(步骤们, 步骤, 日志, vcvars, 头.stdout.strip())
    elif 排["构建段"] == "linux":
        # ② linux 构建段（与 win构建链 对称·657 抽函数=跑一轮 拆函数回 ≤100 行纪律；
        #    1299 传日志=编译器缓存选择（ccache/sccache）留痕到轮日志）
        linux构建链(步骤们, 步骤, 日志)


def 预验清产物(树根: Path) -> bool:
    """637（2026-10-14 用户立法）：预验前置②——历史编译中间产物全部清零。
    rm -rf <树根>/target/（含 target/audit2 的 v2p 指纹缓存）——之后从零配置构建。
    树根参数供 --selftest 注入临时目录隔离断言（生产传 仓库根）。返回是否清干净。
    578：清产物是长纯 Python 段（Windows 大目录 rmtree 分钟级）——前后调
    轮守卫.进度()，轮心跳探测据「主线程进度新鲜」判轮在推进（防长段误判轮卡死）。"""
    ci_round_guard.守.进度()
    目标目录 = 树根 / "target"
    if 目标目录.exists():
        shutil.rmtree(目标目录, ignore_errors=True)
    ci_round_guard.守.进度()
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
    白名单键（同步/清产物/配置/构建/单测/e2e/网站示例verify/
    check_parallel_determinism）中存在的键全 rc==0 才绿；缺键不计
    （未执行段不拖红）；清产物 rc=-1（target/ 残留·预验前置②不满足）即轮必红
    ——--selftest 锁定该口径（648-b 复核结论：清产物失败时后续段照跑，但总绿
    必 False·integrate 只凭「绿」字段放行=无假绿面）。
    992：加「网站示例verify」键（文档示例漂移=真红·必须进绿聚合防假绿）。
    1219：加 check_parallel_determinism（并行产物确定性红=真红；云端全量恒跑
    该门禁，若不入聚合则 rc=1 仍轮绿并可合入——假绿通道）。
    #1221：加「M6平台保障」（触发轮在交叉模拟平台=rc -1 必红；capable 轮=rc 0
    ——M6 红由 e2e 步骤 rc 承担，不重复计）。
    （1219 复审登记：本文件已越 900 警戒线——#1221 已把 E2E 段外迁 ci_m6，
    后续功能改动继续按「先拆再加」执行。）"""
    return all(s.get("rc") == 0 for k, s in 步骤们.items()
               if k in ("同步", "清产物", "配置", "构建", "单测", "e2e",
                        "网站示例verify", "check_parallel_determinism",
                        "M6平台保障"))


def 跑一轮(sha: str, 轮分支: str = "", 分片: int = 0,
          分片召唤时刻: float | None = None) -> dict:
    """对给定 SHA 跑全量门禁三段，返回结构化结果（口径=integrate.py Linux 分支）。
    轮分支空=全局 分支（develop 轮询/点名分支）；池任务传认领的预验分支（1021）。
    1297：分片>1=主实例分片形态（本机跑片 0 重型独立面+收片聚合·仅 ci/预验-* 由
    池跑任务 传入；develop/兜底轮恒 0=全量不变）。1369：分片召唤时刻=认领宽限窗
    锚点（池跑任务 召唤后记录·缺省=进 e2e 段时起算）。"""
    轮分支 = 轮分支 or 分支
    开始 = time.time()
    轮时刻 = datetime.now().strftime("%m%d_%H%M%S")
    轮日志路径 = 日志目录 / ("run_%s_%s.log" % (sha[:10], 轮时刻))
    步骤们 = {}
    with 轮日志路径.open("w", encoding="utf-8") as 日志:
        print("== 云 CI 轮开始 %s @ %s ==" % (sha[:10], datetime.now()), file=日志, flush=True)

        步骤 = 步骤记录器(步骤们, 日志)

        # ① 同步代码到该 SHA（reset 保干净树；git clean 不带 -x=保留 ignored 的 target/ 增量）
        #    523 口径：target/ 增量治理**不靠 clean -x 全清**（每轮全量重编=过度代价），
        #    靠 win构建链 HEAD 指纹清建——指纹不一致才 rmdir 整目录重建，一致才吃增量；
        #    clean 面维持 -fdq 不变（linux 侧 target/build 由 cmake 依赖图自洽·无此病史）。
        #    291：bash -c 复合命令拆三条裸 git 子进程——跨平台（win 不依赖 bash 在 PATH）
        #    1297：三命令段提取 执行同步（与 跑分片轮 共用）。
        同步rc, 同步输出 = 执行同步(日志, 轮分支)
        步骤们["同步"] = {"rc": 同步rc}
        if 同步rc != 0:
            print(同步输出, file=日志, flush=True)
        # #1221：M6 计划（写集含编译器产物面（codegen/opt/ir/model/协议常量/stdlib） ⇒ 本轮必跑 79_v2）——同步后算一次，
        #   E2E 命令与结果「M6」键共用同一计划（git diff 与工作树无关·同步失败也
        #   可判；不可判=fail-safe 触发）。
        m6计划 = ci_m6.轮计划(轮分支, sha, 平台, 是win, 仓库根, 远端名, 机架)
        # #1468（#1221 复审 P2-2 前移）：develop/兜底轮先判「头已前进」——已过期头
        #   直接绿闭环过期跳过（不写 latest），不再落到 M6 平台早退变成红行
        #   （过期头本无需验证，平台不可验判定对它无意义）。
        if 轮分支 == "develop":
            现头 = 取远端SHA()
            if 现头 and 现头 != sha:
                print("  [过期早退] develop 已前进 %s→%s——跳过构建/静态门禁/E2E（节省算力）"
                      % (sha[:10], 现头[:10]), file=日志, flush=True)
                步骤们["e2e"] = {"rc": 0, "说明": "过期早退（develop 已前进 %s）" % 现头[:10]}
                return ci_round.轮早退结果(sha, 轮分支, 步骤们, 开始, 轮日志路径,
                                            现头, m6计划, 平台=平台)
        # #1221 复审 P1-4①：M6 触发但平台不可原生验（交叉模拟 arm64 等）——构建前
        #   早退判红（不烧清产物/构建/E2E；该轮不落 latest，下一轮原生平台重验）。
        if (同步rc == 0 and m6计划.get("触发") and not m6计划.get("平台可跑")
                and ci_m6.不可验处置(轮分支, m6计划, 日志)):
            return ci_round.轮M6不可验结果(sha, 轮分支, 步骤们, 开始, 轮日志路径,
                                           m6计划, 平台=平台)
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
        # ② 平台构建段（1297 提取 构建段执行：win=vcvars+Ninja 指纹清建链·linux=cmake 链
        #    ——跑一轮 与 跑分片轮 共用；历史细节注释见 构建段执行/ci_build.py）
        构建段执行(排, 步骤们, 步骤, 日志)
        # ③.5 静态门禁面+④ E2E（242/447-a·两平台公共段——291 曾因缩进挂在 else 体内被
        #    win 分支整体跳过〔绿=缺键误判〕；523 抽 静态门禁与e2e段·跑一轮 拆函数回 ≤100 行）
        if 排["静态段"]:
            cn = next((p for p in ([仓库根 / "target/cn.exe", 仓库根 / "target/cn"] if 是win
                                   else [仓库根 / "target/build/cn", 仓库根 / "target/cn"])
                       if p.exists()), None)
            静态门禁与e2e段(步骤们, 步骤, cn, 日志, sha, 轮分支=轮分支, 分片=分片,
                            分片召唤时刻=分片召唤时刻, m6计划=m6计划)
            # 973：对账周检段（仅 develop 面轮·周期到期才跑·分支/周期自判在模块内）
            ci_reconcile.对账周检段(步骤们, 步骤, 日志, 轮分支, sha)
        尾部 = []
    return ci_round.轮结果构造(sha, 轮分支, 步骤们, 开始, 轮日志路径, m6计划,
                             平台=平台, 日志目录=日志目录, runner标识=runner标识,
                             构建并行=构建并行, 是win=是win, 聚合绿=聚合绿, 分片数=分片)


def 跑分片轮(sha: str, 轮分支: str, 片序: int = 0, 片数: int = 1) -> dict:
    """1297 从实例片轮：同步→清产物（预验 637 立法·与主实例同规）→构建→e2e 单片
    →交片（交片由 池跑分片 收尾）。不跑单测/静态门禁/verify/对账——那些主实例一份
    即全量覆盖，片只分摊 e2e 段（分片的意义所在）。结果结构与绿聚合口径与 跑一轮
    同构（聚合绿 只看在键·片轮步骤集只有 同步/清产物/配置/构建/e2e）。
    跑器适配：经 带门禁锁跑(跑器=lambda s, b: 跑分片轮(s, b, 片序, 片数)) 调用，
    win 机器锁/轮心跳/豁免注入与全量轮完全一致。"""
    开始 = time.time()
    轮时刻 = datetime.now().strftime("%m%d_%H%M%S")
    轮日志路径 = 日志目录 / ("run_片%d_%s_%s.log" % (片序, sha[:10], 轮时刻))
    步骤们 = {}
    with 轮日志路径.open("w", encoding="utf-8") as 日志:
        print("== 云 CI 分片轮 %s#%d/%d @ %s ==" % (sha[:10], 片序, 片数, datetime.now()),
              file=日志, flush=True)
        步骤 = 步骤记录器(步骤们, 日志)
        同步rc, 同步输出 = 执行同步(日志, 轮分支)
        步骤们["同步"] = {"rc": 同步rc}
        if 同步rc != 0:
            print(同步输出, file=日志, flush=True)
        # #1221：M6 计划——仅片 0 计算（从片恒不含 79_v2·省 git fetch/diff；P2）；
        #   片 0 在不可原生平台构建前早退判红（P1-4①）。
        if 片序 == 0:
            m6计划 = ci_m6.轮计划(轮分支, sha, 平台, 是win, 仓库根, 远端名, 机架)
        else:
            m6计划 = {"触发": False, "处置": "未触发", "平台可跑": True, "平台": 平台,
                      "说明": "从片不含 79_v2（M6 由片 0 主实例承担）"}
        if (同步rc == 0 and 片序 == 0 and m6计划.get("触发")
                and not m6计划.get("平台可跑")
                and ci_m6.不可验处置(轮分支, m6计划, 日志)):
            return ci_round.轮M6不可验结果(sha, 轮分支, 步骤们, 开始, 轮日志路径,
                                           m6计划, 平台=平台)
        排 = 同步后编排(同步rc, 轮分支, 是win)
        if 排["清产物"]:
            # 637（2026-10-14 用户立法）：预验前干净构建——从实例与主实例同规
            #（脏产物假绿防线不分主从；target 全清后 win 指纹清建必然全量重建）
            清干净 = 预验清产物(仓库根)
            步骤们["清产物"] = {"rc": 0 if 清干净 else -1}
            print("[637] 分片侧干净构建%s" % ("：target/ 已全清" if 清干净
                                            else "失败：target/ 残留（本轮判红）"), flush=True)
        构建段执行(排, 步骤们, 步骤, 日志)
        if 同步rc == 0:
            # ④ e2e 单片（构建绿才跑·命令口径与主实例 e2e命令基 完全一致+--shard）
            cn = next((p for p in ([仓库根 / "target/cn.exe", 仓库根 / "target/cn"] if 是win
                                   else [仓库根 / "target/build/cn", 仓库根 / "target/cn"])
                       if p.exists()), None)
            if cn is None:
                步骤们["e2e"] = {"rc": -1, "说明": "未找到 cn 产物"}
            elif 步骤们.get("构建", {}).get("rc") == 0:
                # #1608：物理内存探测失败=e2e 步骤 rc=-1（内存保险丝或判红 已写·
                #   聚合绿白名单键=必红），不再按 1528 的 4096 兜底拿假数字跑。
                保内存 = ci_m6.内存保险丝或判红(步骤们, 日志)
                if 保内存 is not None:
                    片明细路径 = 日志目录 / ("e2e明细_片%d_%s.json" % (片序, sha[:10]))
                    try:
                        片明细路径.unlink()
                    except OSError:
                        pass
                    if 步骤("e2e并行", e2e命令基(cn, 保内存, 须M6=片序 == 0)
                            + e2e明细参(片明细路径)
                            + ["--shard", "%d/%d" % (片序, 片数), "--jobs", str(e2e并行)],
                            e2e整轮超时秒):
                        步骤们["e2e"] = {"rc": 0}
                    else:
                        print("  [分片红] 片 %d/%d 失败用例——如实上报（902 红由主实例聚合定案）"
                              % (片序, 片数), file=日志, flush=True)
                        步骤们["e2e"] = {"rc": 步骤们["e2e并行"]["rc"]}
    with 轮日志路径.open("r", encoding="utf-8", errors="replace") as f:
        尾部 = f.readlines()[-40:]
    return {"sha": sha, "分支": 轮分支, "平台": 平台, "步骤": 步骤们,
            # 1299：执行机归因（分片行同口径·主实例聚合时可见各片机器）
            **ci_report.执行机字段(runner标识, 构建并行, 是win),
            "绿": 聚合绿(步骤们),
            "总秒": round(time.time() - 开始), "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": 轮日志路径.name, "日志尾部": "".join(尾部).splitlines()[-20:],
            "片序": 片序, "片数": 片数,
            # #1221：分片行 M6 计划（从片不含 79_v2·原因显式；主实例聚合以片 0 为准）
            "M6": m6计划,
            "e2e明细": 读e2e明细(日志目录 / ("e2e明细_片%d_%s.json" % (片序, sha[:10])))}


# 落盘/上报/上报执行中/免跑留档/取提交历史/提交时刻 → 1297 薄壳别名（见文件头部
#   拆分薄壳区·实现外迁 ci_report.py）


# ═══════════════ 1021 预验执行器池（TX_01 认领池客户端·家机/TX_02 同构） ═══════════════
# 池配置/池调用 → 1297 薄壳别名（见文件头部拆分薄壳区·实现归一 ci_shard.py 池协议单点）

def 池认领任务() -> dict | None:
    """向池认领一个预验任务（服务端写锁内原子+心跳超时回收）。无任务/池不可达/未配置 → None。
    337：自报平台（win-x64/linux-x86_64/linux-arm64·294 三分运行时探测）——服务端按任务
    认领限制过滤，只派给白名单含本平台的任务；缺省语义（不限任务）对旧/新 runner 完全一致。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return None
    回 = 池调用(配置, "task_claim", {"runner": runner标识, "平台": 平台,
                                     "物理机": ci_shard.物理机标识(),
                                     # #1221：M6 原生可跑能力位（实例表观测/派发数据源）
                                     "M6原生": ci_m6.M6原生可跑(平台, 机架)})
    if 回 and 回.get("ok"):
        return 回.get("任务")
    return None


def 启动心跳(任务分支: str, 片序: int | None = None) -> threading.Event:
    """跑轮期间每 心跳间隔秒 向池续约（返回 stop 事件·池不可达静默——服务端超时回收是
    兜底而非灾难：complete 校验认领者，被重派后旧结果会被拒）。
    1297：片序非 None=片心跳（task_shard_heartbeat·从实例片轮续约——片心跳断=片回
    缺口可被主实例抢补；父任务心跳仍由主实例续约，两者互不相干）。"""
    停 = threading.Event()

    def 跑():
        配置 = 池配置()
        while not 停.wait(心跳间隔秒):
            if 片序 is None:
                池调用(配置, "task_heartbeat", {"runner": runner标识, "分支": 任务分支})
            else:
                池调用(配置, "task_shard_heartbeat",
                       {"runner": runner标识, "分支": 任务分支, "片序": 片序})

    threading.Thread(target=跑, daemon=True).start()
    return 停


def 跑完对齐(仓库根) -> None:
    """#1463 任务收尾树头回 develop（池跑任务/池跑分片 finally 调用）：此前跑完树
    停在任务分支头——空闲实例面板版本列恒显任务 sha =「版本不一致」误报（用户
    10-23 实报 TX_03 红标·实际树已是 develop）。失败仅告警：任务结果已上报，
    树头停留只影响面板参考值，不影响门禁正确性。"""
    try:
        ci_shard.对齐树头(仓库根, 远端名, "develop")
    except RuntimeError as e:
        print("[警告] 跑完对齐 develop 失败（树头停留任务分支）：%s" % e, flush=True)


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
                # #1468：构建后 E2E 前已早退（结果带 早退新头）=过期（此时 HEAD 仍=任务
                #   sha，不能靠 rev-parse 判）——按过期语义闭环，不落盘。
                if 结果.get("早退新头"):
                    结果 = 过期跳过结果(sha, 任务分支, 结果["早退新头"])
                    print("[池] 兜底任务过期跳过（构建后早退·develop 已前进）：%s" % sha[:10], flush=True)
                else:
                    头 = 运行限时(["git", "rev-parse", "HEAD"], 30,
                                  capture_output=True, text=True, cwd=仓库根).stdout.strip()
                    if 头 != sha:
                        结果 = 过期跳过结果(sha, 任务分支, 头)
                        print("[池] 兜底任务过期跳过（跑后复查·develop 已前进）：%s" % sha[:10], flush=True)
                    else:
                        落盘(结果)
        else:
            # 1297 分片召唤：预验任务且分片开启→构建前召唤从片（空闲实例并行构建跑片；
            #   召唤失败=退化单实例全量·加速面不是正确性面）。CN_CI_BRANCH 点名降级
            #   路径（ssh 直发）非 develop 主循环——此处只对池认领的 ci/预验-* 生效。
            #   1369：召唤时刻锚定认领宽限窗（CN_SHARD_GRACE_SEC）——构建耗时已覆盖
            #   大部分宽限，进 e2e 段时快照一次即可判「无认领→立即退化」。
            #   #1401：分片度按池空闲实例/执行中预验任务数自适应（CN_SHARD_N 可固定）。
            分片n = 0
            if 任务分支.startswith("ci/预验-") and ci_shard.分片启用():
                分片n = ci_shard.计算分片数(配置["池地址"], 配置["池令牌"])
            召唤时刻 = time.time() if 分片n > 1 else None
            if 分片n > 1:
                ci_shard.召唤从片(lambda o, d: 池调用(配置, o, d),
                                  任务分支, runner标识, 分片n)
            结果 = 带门禁锁跑(
                sha, 任务分支,
                跑器=((lambda s, b: 跑一轮(s, b, 分片n, 召唤时刻))
                     if 分片n > 1 else None))
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
        跑完对齐(仓库根)       # #1463：树头回 develop（面板版本列=真实树状态）


def 池跑分片(片任务: dict) -> None:
    """1297 从实例执行认领到的缺口片：跑分片轮（同步/清产物/构建/e2e 单片·win 套
    机器锁）→交片（3 次重试）。片红=如实上报（整轮红由主实例聚合定案·902 不在从侧
    重复触发）；交片全败=片行留执行中，片心跳断后服务端回收回缺口（可被主实例
    抢补/他实例重领——正确性无损·浪费一次算力与 task_complete 同口径）。"""
    分支s, sha = 片任务["分支"], 片任务["sha"]
    片序, 片数 = 片任务["片序"], 片任务["片数"]
    print("[分片] 认领 %s#%d/%d（runner=%s）" % (分支s, 片序, 片数, runner标识), flush=True)
    配置 = 池配置()
    停 = 启动心跳(分支s, 片序)
    try:
        结果 = 带门禁锁跑(sha, 分支s,
                          跑器=lambda s, b: 跑分片轮(s, b, 片序, 片数),
                          分片轮=True)   # #1424：分片轮走本实例树锁·同机可并行
        if 结果 is None:
            print("[分片] 本轮让出（gate_lock）——片心跳停由服务端回收回缺口", flush=True)
            return
        ci_shard.交片(lambda o, d: 池调用(配置, o, d), runner标识, 分支s, 片序,
                      bool(结果.get("绿")), 结果, 结果.get("总秒"))
        print("[分片完成] %s#%d/%d 绿=%s 总秒=%s"
              % (分支s, 片序, 片数, 结果.get("绿"), 结果.get("总秒")), flush=True)
    finally:
        停.set()
        跑完对齐(仓库根)       # #1463：树头回 develop（面板版本列=真实树状态）


def 主(常驻: bool, 强制: bool, 等锁: bool = False) -> int:
    日志目录.mkdir(exist_ok=True)
    # 1392：空闲报活+版本指纹（忙闲都发·池未配置=不启动）；#1460：返回刷新广播状态
    刷新状态 = ci_shard.启动实例报活(runner标识, 平台, 仓库根,
                                    M6原生=ci_m6.M6原生可跑(平台, 机架))
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
            # #1460：服务端「更新实例」广播——空闲点（持门禁锁·无任务在跑）对齐 develop
            #   树头；执行中收到只记戳（报活线程），跑完回到本点自动跟随。点名模式
            #   （CN_CI_BRANCH≠develop）不刷新（该模式树正被点名分支占用）。
            try:
                新头 = ci_shard.处理刷新请求(刷新状态, 仓库根, 远端名, 分支)
                if 新头:
                    print("[刷新] 实例树头已对齐 %s@%s（服务端更新实例广播）"
                          % (分支, 新头[:10]), flush=True)
            except RuntimeError as e:
                print("[刷新] 树头对齐失败（下轮重试）：%s" % e, flush=True)
            # #1428：win 让位——池内存在空闲 linux 实例时，win 本轮不领片也不领整任务。
            #   win 三实例与 WSL/xsim 同处一台物理机：win 参与分片=同机 CPU 饱和
            #   （1419 普通片 192 例 1893s/整轮 37 分钟 vs 1417 全轮 827s）。让位把算力
            #   留给非 win；无非 linux 空闲（或池不可达）时 win 照常补位，不浪费算力。
            # #1545：服务端轮转已把 develop/兜底头指派给 win-x64 时不让位（否则 win 档永不认领）。
            if 是win:
                池c = 池配置()
                池态 = ci_shard.池状态(池c["池地址"], 池c["池令牌"])
                if (ci_shard.有非win空闲(池态)
                        and not ci_shard.有win可领任务(池态)):
                    print("[让位] 池内有空闲 linux 实例——win 本轮不领片/不领任务"
                          "（让算力·#1428）", flush=True)
                    if not 常驻:
                        return 0
                    放锁(锁)
                    time.sleep(轮询间隔秒)
                    重抢锁(锁)
                    continue
            # ── 1297 从实例优先领片：分片开启时先看有无缺口片（空闲实例自主补位——
            #    召唤只登记不指派，谁空闲谁领；无缺口再领整任务=池忙时行为与现状一致）
            #    #1424：领片**不受本机锁忙影响**——分片轮走本实例树锁（#1424），同机
            #    三 win 可并行跑片（1423 的锁忙跳过只针对整任务/兜底轮的共享主树锁）。
            if ci_shard.分片启用():
                池c = 池配置()
                片任务 = ci_shard.领片(lambda o, d: 池调用(池c, o, d), runner标识, 平台,
                                      M6原生=ci_m6.M6原生可跑(平台, 机架))
                if 片任务:
                    池跑分片(片任务)
                    if not 常驻:
                        return 0
                    放锁(锁)      # 184：轮间释放·点名可插队
                    time.sleep(轮询间隔秒)
                    重抢锁(锁)
                    continue
            # #1423：整任务/兜底轮取共享主树锁——同机已有实例持锁时跳过（领了也排队
            #   15~30 分钟）。分片轮已由上面独立树锁分流，不受此限。
            if ci_shard.本机门禁锁忙():
                print("[本机锁忙] 共享门禁锁被同机实例持有——本轮不领整任务（避免排队）",
                      flush=True)
                if not 常驻:
                    return 0
                放锁(锁)
                time.sleep(轮询间隔秒)
                重抢锁(锁)
                continue
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
        # #1221 P2-6：ls-remote 瞬时失败短间隔重试 2 次（网络抖动不直接判红）
        sha = ci_round.带重试取远端SHA(取远端SHA)
        if sha is None:
            # #1221 复审 P2-1：点名预验（ci/预验-*）ls-remote 失败——按分支内嵌 sha10
            #   合成红结果落盘，ssh 轮询 60s 内收口（否则守满 240 分钟总超时）。
            if 分支.startswith("ci/预验-"):
                内嵌sha10 = ci_round.预验分支sha10(分支)
                if 内嵌sha10:
                    落盘(ci_round.ls远程失败红结果(分支, 平台, 内嵌sha10))
                    print("[预验] ls-remote 失败——已合成红结果（预验_%s.json）并落盘"
                          % 内嵌sha10, flush=True)
            print("[警告] ls-remote 失败——下轮重试", flush=True)
        elif 分支 == "develop" and not 强制:
            # ── 770 头派发块（轮转派发唯一入口）：新头→入池（服务端 win→linux→arm64 轮转
            #    +平台内随机空闲+全忙放宽）；**头一旦入池=交池负责，本机不再跑**——
            #    770 修复（2026-10-14 深夜三平台同验实录）：此前「已入池」只挡重复入池，
            #    下一轮头未变即漏 continue 直落本机直跑=全池每实例各烧一轮同头。
            #    本机直跑仅剩两条路：入池失败降级（池不可达=门禁永不缺位）与 --force 每日 cron。
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
            if sha in 已入池 or list(日志目录.glob("result_%s_*.json" % sha[:10])):
                # 头已交池负责（本进程入过）/本地已有该 sha 结论档（含红终态）——本机不跑，
                # 等池派发或下一个头（新头≠sha 自然走出此分支）。
                if not 常驻:
                    return 0
                放锁(锁)
                time.sleep(轮询间隔秒)
                重抢锁(锁)
                continue
            if 兜底入池(sha):
                已入池.add(sha)   # 入池成功=池负责（服务端幂等·行已存在亦 ok）
                if not 常驻:
                    return 0
                放锁(锁)
                time.sleep(轮询间隔秒)
                重抢锁(锁)
                continue
            print("[降级] 兜底入池失败——本机直跑（门禁永不缺位）：%s" % sha[:10], flush=True)
            # ── 降级直跑落到下方开跑（develop 分支照发「执行中」上报=观察面可见） ──
            print("[开跑] %s @ %s（分支=%s%s）" % (sha[:10],
                  datetime.now().strftime("%H:%M:%S"), 分支,
                  "·预验" if 分支 != "develop" else ""), flush=True)
            # 190（1035 轮）：开跑即上报「执行中」（细节见 上报执行中 函数）
            上报执行中(sha, 分支, 平台)
            结果 = 带门禁锁跑(sha)
            if 结果 is not None:     # None=291 让出（gate_lock 互忙）——不落盘·下轮再战
                ci_round.直跑收口(sha, 结果, 落盘, 仓库根=仓库根, 轮分支=分支)   # #1221 P1-10/P1-2：过期不落盘·预验轮照落红
        else:
            # ── CN_CI_BRANCH 点名预验 / --force 每日兜底（770：直跑仅剩的两条非轮转路径） ──
            print("[开跑] %s @ %s（分支=%s%s）" % (sha[:10],
                  datetime.now().strftime("%H:%M:%S"), 分支,
                  "·预验" if 分支 != "develop" else ""), flush=True)
            if 分支 == "develop":
                上报执行中(sha, 分支, 平台)   # 190：在跑轮实时可见（细节见函数）
            结果 = 带门禁锁跑(sha)
            if 结果 is not None:     # None=291 让出（gate_lock 互忙）——不落盘·下轮再战
                ci_round.直跑收口(sha, 结果, 落盘, 仓库根=仓库根, 轮分支=分支)   # #1221 P1-10/P1-2：过期不落盘·预验轮照落红
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
