#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_daemon 自测体（660 自 ci_daemon.py 外迁·2026-10-14·单一职责）。

外迁缘由：ci_daemon.py 逼近 1000 行硬红线（AGENTS §3）——660 兜底跑前过期预检需增行，
先把 523/637/657 既有自测段整段外迁；自测属独立职责（CI 式断言面），此后功能增行不再
膨胀主模块。入口不变：python3 ci_daemon.py --selftest（主模块 --selftest 分支**惰性
导入**本模块——runner 常驻进程树盘可能为旧版无此文件，顶层导入会让全体 runner 启动崩）。
本模块以参数接收主模块（不做顶层 import ci_daemon——python ci_daemon.py 跑时那会是第二份
__main__ 副本）；行为级桩对照经猴补/还原主模块属性实现（零真 git 零真构建）。

断言面索引：win 构建脚本文本+指纹三态（523）｜预验清产物（637）｜同步后编排正反两态
+绿聚合三态+AST 互斥链结构锁（657）｜兜底过期纯函数三态+文本结构锁+行为级两态桩
（660）｜ci_daemon.py 行数红线（660）。
"""
from __future__ import annotations

import ast
import inspect
import tempfile
import threading
from pathlib import Path
from types import SimpleNamespace


def 桩跑池(主模块, 任务sha: str, 预检头: str, 跑后头: "str | None" = None) -> dict:
    """660 行为级两态桩：猴补 运行限时/带门禁锁跑/落盘/启动心跳/池配置/池调用 后直调
    池跑任务——零真 git 零真构建。预检头=rev-parse FETCH_HEAD 返回值（现 develop 头）；
    跑后头=跑后 rev-parse HEAD 返回值（缺省同预检头·模拟轮内未前进）。返回计数：整轮/
    落盘/complete 载荷——「过期→整轮零调用」与「非过期→整轮一次」的机械对照事实源。"""
    跑后头 = 预检头 if 跑后头 is None else 跑后头
    真件 = {名: getattr(主模块, 名) for 名 in
            ("运行限时", "带门禁锁跑", "落盘", "启动心跳", "池配置", "池调用")}
    记录 = {"整轮": 0, "落盘": 0, "完成": []}

    def 桩运行(命令, 超时秒, **kwargs):
        if 命令[-1] == "FETCH_HEAD":
            return SimpleNamespace(returncode=0, stdout=预检头 + "\n", stderr="")
        if 命令[-1] == "HEAD":
            return SimpleNamespace(returncode=0, stdout=跑后头 + "\n", stderr="")
        return SimpleNamespace(returncode=0, stdout="", stderr="")   # git fetch 等

    def 桩整轮(sha, 轮分支=""):
        记录["整轮"] += 1
        return {"sha": sha, "分支": 轮分支 or "develop", "平台": 主模块.平台, "绿": True,
                "步骤": {"同步": {"rc": 0}}, "总秒": 0, "时刻": "桩", "日志": "", "日志尾部": []}

    def 桩落盘(结果):
        记录["落盘"] += 1

    def 桩池调用(配置, 操作, 数据, 超时秒=10):
        if 操作 == "task_complete":
            记录["完成"].append(数据)
        return {"ok": True}

    try:
        主模块.运行限时 = 桩运行
        主模块.带门禁锁跑 = 桩整轮
        主模块.落盘 = 桩落盘
        主模块.启动心跳 = lambda 任务分支: threading.Event()
        主模块.池配置 = lambda: {"池地址": "", "池令牌": ""}
        主模块.池调用 = 桩池调用
        主模块.池跑任务({"分支": "ci/兜底-" + 任务sha[:10], "sha": 任务sha})
    finally:
        for 名, 件 in 真件.items():
            setattr(主模块, 名, 件)
    return 记录


def _523_637断言(主模块, 断言) -> None:
    """523 win 构建脚本文本+清建配置形态+指纹清建三态；637 预验清产物（临时目录隔离）。"""
    构建行 = 主模块.win构建行(Path("vcvars64.bat"))
    断言("构建脚本含 088 .ninja_deps 删除行",
         any(".ninja_deps" in l and "del /q" in l for l in 构建行))
    断言("构建脚本含 cmake --build 行",
         any("cmake --build target/build-ninja" in l for l in 构建行))
    断言("构建脚本含 errorlevel 拦截行（构建红显式退 1）",
         any("if errorlevel 1" in l for l in 构建行))
    断言("脚本头含 cl 探测行", any("where cl" in l for l in 构建行))
    清行 = 主模块.win配置行(Path("vcvars64.bat"), True)
    净行 = 主模块.win配置行(Path("vcvars64.bat"), False)
    断言("清建配置含 rmdir build-ninja 行",
         any("rmdir /s /q target\\build-ninja" in l for l in 清行))
    断言("增量配置不含 rmdir", not any("rmdir" in l for l in 净行))
    断言("两形态配置都含 cmake 配置行",
         all(any("cmake -G Ninja" in l for l in x) for x in (清行, 净行)))
    with tempfile.TemporaryDirectory() as 临时:
        记录 = Path(临时) / "last_build.sha"
        断言("指纹缺失→清建", 主模块.需指纹清建("a" * 40, 记录)[0])
        记录.write_text("b" * 40 + "\n", encoding="utf-8")
        断言("指纹一致→不清建", not 主模块.需指纹清建("b" * 40, 记录)[0])
        断言("指纹不一致→清建", 主模块.需指纹清建("c" * 40, 记录)[0])
        断言("HEAD 空头→清建", 主模块.需指纹清建("", 记录)[0])
    # 637 预验前置②干净构建（2026-10-14 用户立法）：预验清产物注入临时目录隔离断言
    with tempfile.TemporaryDirectory() as 临时:
        树637 = Path(临时)
        产物 = 树637 / "target" / "audit2"
        产物.mkdir(parents=True)
        (产物 / "v2p_linuxx64").write_text("x", encoding="utf-8")
        (树637 / "target" / "build" / "cn").parent.mkdir(parents=True)
        (树637 / "target" / "build" / "cn").write_text("x", encoding="utf-8")
        断言("637 清产物：target/ 残留态→清干净（含 audit2 v2p 指纹缓存）", 主模块.预验清产物(树637))
        断言("637 清产物：清后 target/ 不复存在（从零构建前提）", not (树637 / "target").exists())
        断言("637 清产物：无产物态→幂等放行", 主模块.预验清产物(树637))


def _657断言(主模块, 断言) -> None:
    """657（2026-10-14）：648-b 拆链修复的回归自测——编排纯函数正反两态 + 绿聚合
    口径三态 + 对 跑一轮 真实消费面的文本回归锁（上游修法零自测·本件补齐防复发）。"""
    同步后编排, 聚合绿, 跑一轮 = 主模块.同步后编排, 主模块.聚合绿, 主模块.跑一轮
    排 = 同步后编排(0, "ci/预验-abc123", 是win=False)
    断言("657 正：预验 linux 轮→清产物+linux 构建链并存（配置/构建/单测不缺席）",
         排["清产物"] and 排["构建段"] == "linux")
    排 = 同步后编排(0, "ci/预验-abc123", 是win=True)
    断言("657 正：预验 win 轮→清产物+win 构建链并存",
         排["清产物"] and 排["构建段"] == "win")
    排 = 同步后编排(0, "develop", 是win=False)
    断言("657 反：develop 轮→有构建链无清产物（523 增量口径保持）",
         not 排["清产物"] and 排["构建段"] == "linux")
    排 = 同步后编排(0, "ci/兜底-abc123", 是win=True)
    断言("657 反：兜底轮→有构建链无清产物",
         not 排["清产物"] and 排["构建段"] == "win")
    排 = 同步后编排(1, "ci/预验-abc123", 是win=False)
    断言("657 反：同步失败→清产物/构建段/静态段全不执行（仅同步键）",
         not 排["清产物"] and not 排["构建段"] and not 排["静态段"])
    断言("657 复核锁：清产物 rc=-1 → 轮必红（绿聚合含清产物键·integrate 无假绿面）",
         not 聚合绿({"同步": {"rc": 0}, "清产物": {"rc": -1}, "配置": {"rc": 0},
                    "构建": {"rc": 0}, "单测": {"rc": 0}, "e2e": {"rc": 0}}))
    断言("657 复核锁：预验全链六键全 0 → 轮绿",
         聚合绿({k: {"rc": 0} for k in ("同步", "清产物", "配置", "构建", "单测", "e2e")}))
    断言("657 复核锁：develop 轮无清产物键 → 不拖红",
         聚合绿({k: {"rc": 0} for k in ("同步", "配置", "构建", "单测", "e2e")}))
    源一轮 = inspect.getsource(跑一轮)
    断言("657 回归锁：跑一轮 真消费 同步后编排（编排单一事实源）", "同步后编排(" in 源一轮)
    # 657 回归锁（结构级·AST）：跑一轮 内不得存在「清产物与构建动作同属一条 if/elif 链」
    #   ——637 互斥吞构建段的结构特征（链命中=构建段被清产物分支短路·TX_02 假红病根）。
    #   对 637 版必红、对 648-b/657 版绿：优于特征串文本锁——648-b 清产物 elif 保留
    #   但已拆链（行为正确），按特征串会误报；按链结构判定才是真实拦截。
    全部If = [n for n in ast.walk(ast.parse(源一轮)) if isinstance(n, ast.If)]
    内嵌 = {id(n.orelse[0]) for n in 全部If
            if len(n.orelse) == 1 and isinstance(n.orelse[0], ast.If)}
    互斥链行 = []
    for 链头 in (n for n in 全部If if id(n) not in 内嵌):
        动作 = {c.func.id if isinstance(c.func, ast.Name) else
                getattr(c.func, "attr", "")
                for c in ast.walk(链头) if isinstance(c, ast.Call)}
        if "预验清产物" in 动作 and ({"win构建链", "linux构建链", "步骤"} & 动作):
            互斥链行.append(链头.lineno)
    断言("657 回归锁：跑一轮 无清产物/构建同链互斥结构（637 吞段形态·AST 级）",
         not 互斥链行)


def _660断言(主模块, 断言) -> None:
    """660（2026-10-14）：兜底跑前过期预检——纯函数三态 + 池跑任务 文本结构锁 +
    行为级两态桩（过期→整轮零调用；非过期→恰一次；轮内前进→跑后复查仍跳过）。"""
    断言("660 兜底过期：任务 sha ≠ develop 头 → True（跑前跳过·零整轮算力）",
         主模块.兜底过期("a" * 40, "b" * 40))
    断言("660 兜底过期：任务 sha = develop 头 → False（走原整轮路径）",
         not 主模块.兜底过期("a" * 40, "a" * 40))
    断言("660 兜底过期：头空（rev-parse 异常）→ False（保守不跳·fail-safe）",
         not 主模块.兜底过期("a" * 40, ""))
    源池 = inspect.getsource(主模块.池跑任务)
    p预检, p整轮 = 源池.find("兜底过期("), 源池.find('带门禁锁跑(sha, "develop")')
    断言("660 结构锁：池跑任务 内 兜底过期 判定先于 带门禁锁跑（文本锁·防顺序回退）",
         0 <= p预检 < p整轮)
    print("  -- 660 行为级两态桩直调（下方 [池] 行为日志=桩环境·零真 git 零真构建）--")
    记 = 桩跑池(主模块, "a" * 40, "b" * 40)
    断言("660 桩·过期轮：带门禁锁跑（整轮）零调用（不动构建/单测/E2E）", 记["整轮"] == 0)
    断言("660 桩·过期轮：不落盘（防 latest 被非任务 sha 污染）", 记["落盘"] == 0)
    断言("660 桩·过期轮：complete 绿=True+步骤含过期跳过（服务端不重派）",
         len(记["完成"]) == 1 and 记["完成"][0]["绿"] is True
         and "过期跳过" in 记["完成"][0]["结果"]["步骤"])
    记 = 桩跑池(主模块, "a" * 40, "a" * 40)
    断言("660 桩·非过期轮：整轮恰一次（原路径保持）", 记["整轮"] == 1)
    断言("660 桩·非过期轮：结果落盘一次（develop 语义）", 记["落盘"] == 1)
    记 = 桩跑池(主模块, "a" * 40, "a" * 40, 跑后头="b" * 40)
    说明 = 记["完成"][0]["结果"]["步骤"].get("过期跳过", {}).get("说明", "") if 记["完成"] else ""
    断言("660 桩·轮内前进：跑后复查仍过期跳过（双保险保留·不落盘·文案 189 语义）",
         记["整轮"] == 1 and 记["落盘"] == 0 and len(记["完成"]) == 1
         and "develop 已前进至 %s" % ("b" * 10) in 说明)


def _行数断言(断言) -> None:
    """660 行数红线：ci_daemon.py（自测外迁后）<1000 行——防再回涨到硬上限。"""
    脚本 = Path(__file__).resolve().parent / "ci_daemon.py"
    行数 = len(脚本.read_text(encoding="utf-8").splitlines())
    断言("660 行数红线：ci_daemon.py %d 行 < 1000（自测已外迁防回涨）" % 行数, 行数 < 1000)


def 自测(主模块) -> int:
    """--selftest 入口（断言集 523+637+657+660）——CI 式：逐条打印，任一红退出 1。"""
    断言们: list[tuple[str, bool]] = []

    def 断言(名: str, 条件: bool) -> None:
        断言们.append((名, bool(条件)))

    _523_637断言(主模块, 断言)
    _657断言(主模块, 断言)
    _660断言(主模块, 断言)
    _行数断言(断言)
    for 名, ok in 断言们:
        print("  [%s] %s" % ("OK" if ok else "×", 名))
    红 = sum(1 for _, ok in 断言们 if not ok)
    print("== ci_daemon 自测（523+637+657+660）：%d 断言，%s ==" % (len(断言们), "全绿" if 红 == 0 else "红 %d" % 红))
    return 0 if 红 == 0 else 1
