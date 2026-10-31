#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_pool.py —— 池任务执行面（认领→心跳→跑任务/跑分片→complete 生命周期）。

#1462（2026-10-31）自 ci_daemon.py 原样外迁（「先拆再加」：ci_daemon 961 行超 900
警戒线，本轮加环境预检门/CMake Error 回传/报活载荷三挂点前先拆；973 外迁 ci_build、
1297 外迁 ci_fallback/ci_report/ci_shard 同款）。搬移**逐行保持**——selftest 源码
文本锁（660/975/#1463/#1468/#1665 族 inspect.getsource 锚）跟随函数对象继续命中。

依赖形态（与 ci_fallback/ci_report 初始化注入先例同构）：
  · 池协议/构建链/兜底判定/落盘 等单点模块直接 import（零循环：本模块只被
    ci_daemon 顶层 import，不回引 ci_daemon）；
  · ci_daemon 侧实现面（跑一轮/跑分片轮/带门禁锁跑）与运行常量（runner标识/平台/
    交叉模拟实例/是win/机架/仓库根/远端名/心跳间隔秒）经 初始化() 注入模块级名——
    注入后本模块自洽，selftest 可对模块属性猴补（660 桩跑池同款）。

#1462 新增：环境可领（认领门）——ci_env_probe.探测环境 不满足不领片/不领整任务
（1212 实录：坏环境实例照领任务烧整轮假红；报活心跳保持·详情随 instance_ping
上报面板标注缺什么）。
"""
from __future__ import annotations

import threading
import time
from datetime import datetime

import ci_shard        # 池协议客户端单点（池配置/池调用/召唤/交片/对齐树头）
import ci_m6           # M6 原生可跑能力位
import ci_fallback     # 兜底判定（兜底过期/过期跳过结果·ci_daemon 已初始化）
import ci_report       # 结果落盘（ci_daemon 已初始化）
import ci_round_guard  # 运行限时（294/578 超时安全壳）
import ci_env_probe    # #1462：环境预检（探测+判定单点）

# ── 单点别名（模块级·selftest 猴补面·与 ci_daemon 薄壳同款） ──
落盘 = ci_report.落盘
运行限时 = ci_round_guard.运行限时
池配置 = ci_shard.池配置
池调用 = ci_shard.池调用
兜底过期 = ci_fallback.兜底过期
过期跳过结果 = ci_fallback.过期跳过结果

# ── 初始化注入的运行环境（ci_daemon.主 启动前一次性赋值） ──
runner标识 = ""        # 池内身份（认领/心跳/完成·CN_RUNNER_ID）
平台 = ""              # win-x64/linux-x86_64/linux-arm64（337 自报）
交叉模拟实例 = False    # #1665：交叉模拟能力位（整任务让路）
是win = False
机架 = ""
仓库根 = None          # Path
远端名 = "origin"
心跳间隔秒 = 60
跑一轮 = None          # callable(sha, 轮分支, 分片=0, 召唤时刻=None)——ci_daemon 侧
跑分片轮 = None        # callable(sha, 轮分支, 片序, 片数)——ci_daemon 侧
带门禁锁跑 = None      # callable(sha, 轮分支, 跑器=None, 分片轮=False)——ci_daemon 侧


def 初始化(**注入) -> None:
    """ci_daemon.主 启动注入（键=上方模块级名单·缺键保持缺省）。显式逐键赋值
    （不用 globals().update——注入面显式可审计）。"""
    global runner标识, 平台, 交叉模拟实例, 是win, 机架, 仓库根, 远端名, 心跳间隔秒
    global 跑一轮, 跑分片轮, 带门禁锁跑
    runner标识 = 注入.get("runner标识", runner标识)
    平台 = 注入.get("平台", 平台)
    交叉模拟实例 = 注入.get("交叉模拟实例", 交叉模拟实例)
    是win = 注入.get("是win", 是win)
    机架 = 注入.get("机架", 机架)
    仓库根 = 注入.get("仓库根", 仓库根)
    远端名 = 注入.get("远端名", 远端名)
    心跳间隔秒 = 注入.get("心跳间隔秒", 心跳间隔秒)
    跑一轮 = 注入.get("跑一轮", 跑一轮)
    跑分片轮 = 注入.get("跑分片轮", 跑分片轮)
    带门禁锁跑 = 注入.get("带门禁锁跑", 带门禁锁跑)


def 环境可领() -> bool:
    """#1462 认领门：一轮最小环境（编译器/cmake/生成器/gtest/磁盘）不满足=不领
    片/不领整任务——报活线程照发心跳并携「环境预检」详情（面板标注缺什么），
    运维修好 60s 内自动恢复领任务。develop 派发面不受影响（入池派发跑的是别的
    实例·本实例只是不认领）；ssh 点名/--force 显式通道不拦（人工在场·红也是
    真红显形）。探测每次实时跑（装 gtest 后 60s 内恢复·无缓存状态）。"""
    详情 = ci_env_probe.探测环境(仓库根, 是win)
    if ci_env_probe.判定可领(详情):
        return True
    print("[环境门] 缺 %s——本轮不领片/不领整任务（详情随报活上报·修复后自动恢复；"
          "1212 假红根源治理：坏环境烧整轮假红不如留在池外显形）"
          % ci_env_probe.缺失一行(详情), flush=True)
    return False


# ═══════════════ 1021 预验执行器池（TX_01 认领池客户端·家机/TX_02 同构） ═══════════════


def 池认领任务() -> dict | None:
    """向池认领一个预验任务（服务端写锁内原子+心跳超时回收）。无任务/池不可达/未配置 → None。
    337：自报平台（win-x64/linux-x86_64/linux-arm64·294 三分运行时探测）——服务端按任务
    认领限制过滤，只派给白名单含本平台的任务；缺省语义（不限任务）对旧/新 runner 完全一致。
    #1667：交叉模拟实例默认不领整任务（预验整任务与 ci/兜底-* 轮都跑全量 e2e·qemu
    i128 族错值假红；服务端能力位机械拒绝为权威，此处客户端先行让路省一次请求）；
    CN_XSIM_ALLOW_TASK=1 显式放行。每 runner 一次显式日志（不静默空转）。"""
    配置 = 池配置()
    if not 配置["池地址"]:
        return None
    可领, 让路行 = ci_shard.整任务认领判定(交叉模拟实例, runner标识)
    if 让路行:
        print(让路行, flush=True)
    if not 可领:
        return None
    回 = 池调用(配置, "task_claim", {"runner": runner标识, "平台": 平台,
                                     "物理机": ci_shard.物理机标识(),
                                     # #1221：M6 原生可跑能力位（实例表观测/派发数据源）
                                     "M6原生": ci_m6.M6原生可跑(平台, 机架),
                                     # #1665：交叉模拟能力位（服务端落表+认领拒绝数据源）
                                     "交叉模拟": 交叉模拟实例})
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
    树头停留只影响面板参考值，不影响门禁正确性。（形参 仓库根 遮蔽模块级同名
    注入值——与外迁前 ci_daemon 形态一致·调用点传参语义不变。）"""
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
                #   #1634：arm64 轮（原生 arm64/xsim 实例作主实例）平台禁分片
                #   ——runner 标识进判定（xsim 宿主 x64·平台段判不出）。
                分片n = ci_shard.计算分片数(配置["池地址"], 配置["池令牌"],
                                             runner=runner标识)
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
