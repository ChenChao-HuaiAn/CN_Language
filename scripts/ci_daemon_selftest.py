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
（660）｜ci_daemon.py 行数红线（660）｜日志 漏参族自由名锚+e2e 并行红路径冒烟（728t）。
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


class _局部收集(ast.NodeVisitor):
    """728t：函数体局部定义名收集（不下钻嵌套函数体——嵌套函数名本身计局部）。
    覆盖 Store 目标/import 别名/异常 as/嵌套函数与类名。"""

    def __init__(self):
        self.局部: set[str] = set()

    def visit_FunctionDef(self, 节点):
        self.局部.add(节点.name)

    def visit_AsyncFunctionDef(self, 节点):
        self.局部.add(节点.name)

    def visit_ClassDef(self, 节点):
        self.局部.add(节点.name)

    def visit_Lambda(self, 节点):
        pass

    def visit_Name(self, 节点):
        if not isinstance(节点.ctx, ast.Load):
            self.局部.add(节点.id)

    def visit_alias(self, 节点):
        self.局部.add((节点.asname or 节点.name).split(".")[0])

    def visit_ExceptHandler(self, 节点):
        if 节点.name:
            self.局部.add(节点.name)
        self.generic_visit(节点)


class _引用收集(ast.NodeVisitor):
    """728t：函数体读取名收集（不下钻嵌套函数体）。"""

    def __init__(self):
        self.自由: set[str] = set()

    def visit_FunctionDef(self, 节点):
        pass

    def visit_AsyncFunctionDef(self, 节点):
        pass

    def visit_Lambda(self, 节点):
        pass

    def visit_Name(self, 节点):
        if isinstance(节点.ctx, ast.Load):
            self.自由.add(节点.id)


def _文件自由名问题(文件: Path) -> list[str]:
    """728t：单文件「模块级函数未定义自由名」扫描——自由名 ∉ 形参∪函数局部定义∪
    模块级定义∪内置/dunder。模块级定义=模块体顶层（含模块级 if 直接子句）的赋值名/
    函数类名/import 别名——**刻意不含函数体内 Store**（正因如此才能抓出 523 抽
    静态门禁与e2e段 漏传 日志：日志 是 跑一轮 的局部而非模块级名，函数体内引用即
    NameError）。返回 ["函数名(): 名1, 名2"] 问题清单（空=干净）。"""
    import builtins
    树 = ast.parse(文件.read_text(encoding="utf-8"))
    模块级: set[str] = set()
    for 语句 in 树.body:
        候补 = list(语句.body) + list(语句.orelse) if isinstance(语句, ast.If) else [语句]
        for 子 in 候补:
            if isinstance(子, (ast.Assign, ast.AnnAssign)):
                for 名节点 in ast.walk(子):
                    if isinstance(名节点, ast.Name) and isinstance(名节点.ctx, ast.Store):
                        模块级.add(名节点.id)
            elif isinstance(子, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                模块级.add(子.name)
            elif isinstance(子, (ast.Import, ast.ImportFrom)):
                for 别名 in 子.names:
                    模块级.add((别名.asname or 别名.name).split(".")[0])
    白名单 = 模块级 | set(dir(builtins)) | {"__file__", "__name__", "__doc__"}
    问题: list[str] = []
    for 函数 in [n for n in 树.body
                 if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef))]:
        局部 = _局部收集()
        引用 = _引用收集()
        for 语句 in 函数.body:
            局部.visit(语句)
            引用.visit(语句)
        形参 = {a.arg for a in 函数.args.args + 函数.args.kwonlyargs + 函数.args.posonlyargs}
        if 函数.args.vararg:
            形参.add(函数.args.vararg.arg)
        if 函数.args.kwarg:
            形参.add(函数.args.kwarg.arg)
        缺 = sorted(引用.自由 - 局部.局部 - 形参 - 白名单)
        if 缺:
            问题.append("%s(): %s" % (函数.name, ", ".join(缺)))
    return 问题


def _728t断言(主模块, 断言) -> None:
    """728t（2026-10-14）：静态门禁与e2e段 日志 漏参族——自由名锚（静态·双文件）+
    e2e 并行红路径冒烟（行为·零真构建：假 步骤/临时 cn 直调，逼出 [红即停] 行）。
    实录（修复前两态）：自由名锚直报「静态门禁与e2e段(): 日志」；冒烟在旧签名下
    TypeError=红。修复后两态全绿。902（2026-10-16 用户令）：并行红→串行复验废止
    （红即停）——冒烟断言随 935 一并对齐：锁 [红即停] 提示行与「不落串行键」，
    防 447-a 复验形态复活（原断言锁 447-a=902 后必红）。"""
    根 = Path(__file__).resolve().parent
    问题: list[str] = []
    for 文件 in ("ci_daemon.py", "ci_daemon_selftest.py"):
        问题 += _文件自由名问题(根 / 文件)
    断言("728t 自由名锚：模块级函数零未定义自由名（抽函数漏参族防线%s）"
         % ("｜" + "；".join(问题) if 问题 else ""), not 问题)

    import io
    调用: list[str] = []
    命令们: list = []                          # 842：捕获 (名, 命令) 断言 --json 明细参数
    步骤们: dict = {"构建": {"rc": 0}}        # 构建绿 → 进 e2e 分支

    def 假步骤(名: str, 命令, 超时秒) -> bool:
        # 与真 步骤() 闭包同构：落键 步骤们[名] 再返回绿否（真闭包必写，冒烟缺写即
        #   在本断言暴露=同构锁）；e2e并行 假红 → 逼出 [红即停] 提示行（902 路径）。
        调用.append(名)
        命令们.append((名, list(命令)))
        绿 = 名 != "e2e并行"
        步骤们[名] = {"rc": 0 if 绿 else 1}
        return 绿

    with tempfile.TemporaryDirectory() as 临时:
        假cn = Path(临时) / "cn.exe"
        假cn.write_text("x", encoding="utf-8")
        假日志 = io.StringIO()
        假sha = "a" * 40
        try:
            主模块.静态门禁与e2e段(步骤们, 假步骤, 假cn, 假日志, 假sha)
            崩 = None
        except TypeError:                     # 旧签名（842 前 4 参/728t 前 3 参）对照——
            try:                              # 同名函数旧形态直调，报更清晰路径
                主模块.静态门禁与e2e段(步骤们, 假步骤, 假cn, 假日志)
                崩 = None
            except Exception as e:
                崩 = e
        except Exception as e:
            崩 = e
    断言("728t 冒烟：e2e 并行红路径零异常（902 红即停·原 422 行 [复验] 演进%s）"
         % ("" if 崩 is None else "｜异常=%r" % (崩,)), 崩 is None)
    断言("728t 冒烟：红即停不落串行复验键且 e2e 取并行 rc（902 用户令·447-a 废止）",
         "e2e串行复验" not in 步骤们
         and 步骤们.get("e2e", {}).get("rc") == 步骤们.get("e2e并行", {}).get("rc") == 1)
    断言("728t 冒烟：[红即停] 提示行入日志句柄（file=日志 实连不炸）",
         "[红即停]" in 假日志.getvalue())
    断言("842 冒烟：e2e 命令带 --json 明细参数（本树 run_e2e 已支持·探测放行·"
         "红即停后仅并行主轮一条）",
         len([c for _, c in 命令们 if "--json" in c
              and any("e2e明细" in x for x in c)]) == 1)


def _842断言(主模块, 断言) -> None:
    """842（2026-10-15）：e2e --json 明细链路——读e2e明细正反两态（8300 页面红行
    详情数据源）+ 取提交历史形态（develop 红灯归因数据源·缺数据=空列表降级）。"""
    with tempfile.TemporaryDirectory() as 临时:
        好 = Path(临时) / "好.json"
        好.write_text('{"失败用例": [{"名称": "01_hello", "原因": "输出不一致"}]}',
                      encoding="utf-8")
        明细 = 主模块.读e2e明细(好)
        断言("842 读e2e明细：正例结构直达（失败用例随结果 JSON 上 8300）",
             isinstance(明细, dict) and 明细["失败用例"][0]["名称"] == "01_hello")
        坏 = Path(临时) / "坏.json"
        坏.write_text("{坏 JSON", encoding="utf-8")
        断言("842 读e2e明细：坏 JSON=None（页面降级提示·不炸轮）",
             主模块.读e2e明细(坏) is None)
        断言("842 读e2e明细：缺文件=None（e2e 未跑/旧版三态统一降级）",
             主模块.读e2e明细(Path(临时) / "无.json") is None)
    历史 = 主模块.取提交历史(5)
    断言("842 取提交历史：列表形态且条目含 sha/主题（归因数据源·git 异常=空列表不炸）",
         isinstance(历史, list) and all(
             isinstance(项, dict) and 项.get("sha") and "主题" in 项 for 项 in 历史))


def _935断言(主模块, 断言) -> None:
    """935（2026-10-17 win 池两夜实录 379/932）：轮级看门狗——ci_proc.运行树 整树
    回收行为直测（真起「父→孙」进程树→真超时→断言父/孙双亡 + [超时回收] 审计行），
    并锁 运行() 委托 与 E2E 专用整轮超时常量（防回退到 3h 悬空僵轮）。"""
    import subprocess
    import sys
    import time
    import ci_round_guard
    断言("935+578 融合锁：ci_daemon.运行 薄壳转发 ci_round_guard（轮追踪单一入口）",
         "ci_round_guard.运行" in inspect.getsource(主模块.运行))
    源追踪 = inspect.getsource(ci_round_guard.轮守卫.跑追踪)
    断言("935+578 融合锁：跑追踪 底层经 ci_proc 起子/超时回收（整树回收与轮追踪双立）",
         "ci_proc.起子" in 源追踪 and "ci_proc.超时回收" in 源追踪)
    断言("935 结构锁：e2e 步骤超时取 e2e整轮超时秒（专用看门狗·不悬空 3h）",
         "e2e整轮超时秒" in inspect.getsource(主模块.静态门禁与e2e段))
    断言("935 常量：e2e整轮超时秒 ∈ (0, 单轮总超时秒]（CN_E2E_TIMEOUT_SEC 可覆盖）",
         0 < getattr(主模块, "e2e整轮超时秒", 0) <= getattr(主模块, "单轮总超时秒", 0))

    with tempfile.TemporaryDirectory() as 临时:
        树pid文件 = Path(临时) / "树pid.txt"
        轮日志路径 = Path(临时) / "轮日志.txt"
        父脚本 = ("import os, subprocess, sys, time\n"
                  "子 = subprocess.Popen([sys.executable, '-c',\n"
                  "    'import time; time.sleep(120)'])\n"
                  "open(sys.argv[1], 'w').write('%d %d' % (os.getpid(), 子.pid))\n"
                  "time.sleep(120)\n")
        with 轮日志路径.open("w", encoding="utf-8") as 轮日志:
            try:
                主模块.运行([sys.executable, "-c", 父脚本, str(树pid文件)],
                          轮日志, 超时秒=5)
                崩 = None
            except subprocess.TimeoutExpired as e:
                崩 = e
        日志文本 = 轮日志路径.read_text(encoding="utf-8", errors="replace")
        断言("935 看门狗：超时抛 TimeoutExpired（步骤层 rc=-99 诚实红语义保持）",
             isinstance(崩, subprocess.TimeoutExpired))
        断言("935 看门狗：[超时回收] 审计行落日志（不静默·可审计）",
             "[超时回收]" in 日志文本)
        等写 = 0.0
        while 等写 < 20 and not 树pid文件.exists():
            time.sleep(0.2)
            等写 += 0.2
        父pid = 子pid = None
        if 树pid文件.exists():
            部分 = 树pid文件.read_text().split()
            if len(部分) == 2:
                父pid, 子pid = int(部分[0]), int(部分[1])
        断言("935 看门狗：父进程树已建立（父/孙 pid 采样在超时前落盘）",
             父pid is not None and 子pid is not None)
        if 父pid and 子pid:
            死光 = 0.0
            while 死光 < 20 and (主模块.gate_lock.pid存活(父pid)
                                 or 主模块.gate_lock.pid存活(子pid)):
                time.sleep(0.3)
                死光 += 0.3
            断言("935 看门狗：超时后父进程已回收（整树生命线）",
                 not 主模块.gate_lock.pid存活(父pid))
            断言("935 看门狗：超时后孙进程亦已回收（932 孤儿 worker 形态根治）",
                 not 主模块.gate_lock.pid存活(子pid))
        with 轮日志路径.open("w", encoding="utf-8") as 轮日志:
            不存 = 主模块.运行(["no-such-cmd-935.exe"], 轮日志, 超时秒=5)
        断言("935 兼容：命令不存在经 运行树 仍 rc=127（297 环境面语义不变）",
             不存.returncode == 127)


def _行数断言(断言) -> None:
    """660 行数红线：ci_daemon.py（自测外迁后）<1000 行——防再回涨到硬上限。"""
    脚本 = Path(__file__).resolve().parent / "ci_daemon.py"
    行数 = len(脚本.read_text(encoding="utf-8").splitlines())
    断言("660 行数红线：ci_daemon.py %d 行 < 1000（自测已外迁防回涨）" % 行数, 行数 < 1000)


def _770断言(主模块, 断言) -> None:
    """770 文本锁（2026-10-14 用户三条裁决·结构锁防复活）：①「头变化必验不可跳过」=
    源码零 已跑过(/池已绿( 调用与「[跳过]」输出（1008 零重复算力联动废止）；②角色区分
    废止=无「角色 == "pool"」分支（统一池·env 仅兼容）；③统一轮询入池=「已入池」防重集合
    在位；④兜底入池不再带认领限制（服务端轮转唯一权威）。"""
    源 = (Path(__file__).resolve().parent / "ci_daemon.py").read_text(encoding="utf-8")
    断言("770 文本锁：零 已跑过( 调用（头变化必验·本地绿档不复用）",
         "已跑过(" not in 源.replace("def 已跑过(", ""))
    断言("770 文本锁：零 池已绿( 调用（预验绿不复用·develop 面必复验）",
         "池已绿(" not in 源)
    断言("770 文本锁：零「[跳过]」输出行（不可跳过·用户令 2026-10-14）", "[跳过]" not in 源)
    断言("770 文本锁：零 角色 pool 分支（统一池·CN_CI_ROLE 废止）", '角色 == "pool"' not in 源)
    断言("770 文本锁：已入池 防重集合在位（常驻期幂等）", "已入池" in 源)
    断言("770 文本锁：兜底入池零认领限制实参（服务端轮转唯一权威）",
         '"task_enqueue", {"分支": "ci/兜底-%s" % sha[:10], "sha": sha}' in 源)
    断言("770 修复锁：头已交池（sha in 已入池）分支在位且先于开跑——本机不重复直跑"
         "（三平台同验实录 2026-10-14 深夜）",
         "if sha in 已入池 or list(日志目录.glob" in 源
         and 源.index("if sha in 已入池 or list") < 源.rindex("[开跑]"))
    断言("770 修复锁：入池失败降级显式化（[降级] 输出·门禁永不缺位）",
         "[降级] 兜底入池失败" in 源)


def 自测(主模块) -> int:
    """--selftest 入口（断言集 523+637+657+660+728t+770+842+935）——CI 式：逐条打印，任一红退出 1。"""
    断言们: list[tuple[str, bool]] = []

    def 断言(名: str, 条件: bool) -> None:
        断言们.append((名, bool(条件)))

    _523_637断言(主模块, 断言)
    _657断言(主模块, 断言)
    _660断言(主模块, 断言)
    _728t断言(主模块, 断言)
    _770断言(主模块, 断言)
    _842断言(主模块, 断言)
    _935断言(主模块, 断言)
    _行数断言(断言)
    for 名, ok in 断言们:
        print("  [%s] %s" % ("OK" if ok else "×", 名))
    红 = sum(1 for _, ok in 断言们 if not ok)
    print("== ci_daemon 自测（523+637+657+660+728t+770+842+935）：%d 断言，%s ==" % (len(断言们), "全绿" if 红 == 0 else "红 %d" % 红))
    return 0 if 红 == 0 else 1
