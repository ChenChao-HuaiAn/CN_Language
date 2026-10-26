#!/usr/bin/env python3
# 交叉模拟实例识别面（#1665 建·#1667 扩展整任务让路·2026-10-26）：
#   背景：xsim 实例（x86_64 宿主 qemu-user 跑 arm64 目标）在普通 e2e 上出 i128 族
#   错值（500/789/826/833/663/716/737/800/948 等：期望非 0 实际 0），arm64 真机同族
#   全绿——qemu 模拟下错值，不是编译器语义缺陷；xsim 的「全量绿」无信息量且假红
#   阻塞集成（#1665 分片面、#1667 整任务/兜底轮面）。本模块=识别/平台推导/让路
#   开关单一事实源（实现自 ci_shard 外迁：ci_shard 叠加 #1632/#1634/#1640 后已
#   1026 行超 1000 红线，先拆再加）。
#   规则（服务端以能力位机械拒绝为权威，客户端先行让路）：
#     ① 分片领片（#1665）：交叉模拟实例不领普通 e2e 片（显式邀请=认领限制含 xsim）；
#     ② 整任务认领（#1667）：不领会跑全量 e2e 的预验整任务与 ci/兜底-* 轮；
#     ③ 逃生门：CN_XSIM_ALLOW_SHARD=1 / CN_XSIM_ALLOW_TASK=1 显式放行（默认禁）。
#   依赖方向：ci_shard/ci_daemon → 本模块（单向·本模块只依赖 os·无反向导入）。
from __future__ import annotations   # P1-1：3.8 注解惰性化（tuple 泛型定义期不求值）

import os

交叉模拟环境键 = "CN_XSIM"          # tests/e2e/e2e交叉模拟.py 激活键（同源）
交叉模拟领片开关 = "CN_XSIM_ALLOW_SHARD"
交叉模拟领任务开关 = "CN_XSIM_ALLOW_TASK"   # #1667：整任务/兜底轮显式放行
交叉工具链环境键们 = ("CN_AS", "CN_CXX")


def 交叉工具链是arm64(环境=None) -> bool:
    """#1665：CN_AS/CN_CXX 指向 aarch64 交叉工具链=本实例做 arm64 交叉构建（028 §十
    部署实证）。即使 CN_CI_PLATFORM 漏配也可识别（平台推导的数据源）；环境可注入
    （selftest 零环境依赖）。"""
    环 = os.environ if 环境 is None else 环境
    for 键 in 交叉工具链环境键们:
        值 = str(环.get(键, "") or "").strip().lower()
        if "aarch64" in 值 or "arm64" in 值:
            return True
    return False


def 本实例交叉模拟(平台: str, 机架: str, runner: str = "", 环境=None) -> bool:
    """#1665：本实例是否交叉模拟（qemu-user 跑非宿主架构目标）。四源任一为真：
    ① 目标平台 linux-arm64 且宿主架构非 arm64（x64 宿主跑 arm64 目标·与
       ci_m6.交叉模拟态 同构判据）；
    ② CN_XSIM 环境非空且非 0/false/no/off（e2e 激活键·部署可显式标记）；
    ③ CN_AS/CN_CXX 指向 aarch64 交叉工具链（028 部署实证·平台漏配亦可识别）；
    ④ runner 标识含 xsim（#1634 同源识别·近期 arm64 轮执行机均 home-xsim-*）。
    纯函数（环境可注入·selftest 直调），识别结果随池协议上报为能力位。"""
    环 = os.environ if 环境 is None else 环境
    标记 = str(环.get(交叉模拟环境键, "") or "").strip().lower()
    if 标记 and 标记 not in ("0", "false", "no", "off"):
        return True
    机 = str(机架 or "").strip().lower()
    if str(平台 or "").strip() == "linux-arm64" and 机 not in ("aarch64", "arm64"):
        return True
    if 交叉工具链是arm64(环) and 机 not in ("aarch64", "arm64"):
        return True
    return "xsim" in str(runner or "").lower()


def 规范平台(显式平台: str, 机架: str, 环境=None) -> tuple:
    """#1665 平台如实上报：显式 CN_CI_PLATFORM 优先（用户配置权威）；未显式时若
    CN_AS/CN_CXX 指向 aarch64 交叉工具链且宿主非 arm64 → 目标平台按 linux-arm64
    上报（真实目标平台，不冒充宿主平台）。返回 (平台, 说明)——平台空=保持原探测；
    说明非空=调用方必须打印（显式推导，不静默改）。"""
    显式 = str(显式平台 or "").strip()
    if 显式:
        return 显式, ""
    机 = str(机架 or "").strip().lower()
    if 机 not in ("aarch64", "arm64") and 交叉工具链是arm64(环境):
        return "linux-arm64", ("检测到 aarch64 交叉工具链而 CN_CI_PLATFORM 未设——"
                               "平台按目标平台 linux-arm64 上报（#1665 平台如实上报）")
    return "", ""


def _开关(名: str, 环境=None) -> bool:
    """显式逃生门解析：未设=关；1/true/yes/on=开；0/false/no/off=关；
    其余值=显式告警后按关（配置面不静默吞键·1369 同口径）。"""
    环 = os.environ if 环境 is None else 环境
    原 = str(环.get(名, "") or "").strip().lower()
    if not 原:
        return False
    if 原 in ("1", "true", "yes", "on"):
        return True
    if 原 in ("0", "false", "no", "off"):
        return False
    print("[分片配置] %s=%r 非法——按默认（不放行）" % (名, 原), flush=True)
    return False


def 交叉模拟允许领片(环境=None) -> bool:
    """#1665：交叉模拟实例默认不领普通 e2e 分片；CN_XSIM_ALLOW_SHARD=1/true/yes 显式
    放行（承担交叉模拟设计面时用·默认禁）。非法值=按默认告警（不静默吞键）。"""
    return _开关(交叉模拟领片开关, 环境)


def 交叉模拟允许领任务(环境=None) -> bool:
    """#1667：交叉模拟实例默认不认领会跑全量 e2e 的整任务（预验整任务 + ci/兜底-*
    轮都走 task_claim）；CN_XSIM_ALLOW_TASK=1/true/yes 显式放行。非法值=告警+默认禁。"""
    return _开关(交叉模拟领任务开关, 环境)


def 启动平台信息(探测平台: str, 显式平台: str, 机架: str, runner: str,
                环境=None) -> tuple:
    """#1665：daemon 启动平台信息单点（ci_daemon 只打印日志行）——
    返回 (平台, 交叉模拟, 日志行们)：
      平台 = 显式 CN_CI_PLATFORM 优先；未显式且交叉工具链指向 aarch64 → linux-arm64
             （真实目标平台·不冒充宿主）；其余保持运行时探测值（零行为变化）。
      交叉模拟 = 本实例交叉模拟(平台, 机架, runner)（四源判定）。
      日志行 = 推导说明 + 交叉模拟判定 + 配置矛盾告警（调用方逐条打印·不静默）。"""
    规, 说明 = 规范平台(显式平台, 机架, 环境)
    平台 = 规 or str(探测平台 or "")
    交叉 = 本实例交叉模拟(平台, 机架, runner, 环境)
    行们 = []
    if 说明:
        行们.append("[平台] " + 说明)
    if 交叉:
        行们.append("[平台] 本实例判定为交叉模拟（平台=%s·宿主=%s）——不领普通 e2e "
                    "分片/整任务（#1665/#1667）" % (平台, 机架))
        if 平台 not in ("linux-arm64", "linux-arm64-xsim"):
            行们.append("[平台警告] 交叉模拟检测为真但平台=%s（CN_CI_PLATFORM 与交叉工具链/"
                        "命名矛盾）——按显式配置上报，池侧已按能力位保护" % 平台)
    return 平台, 交叉, 行们
