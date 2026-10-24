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
（660）｜ci_daemon.py 行数红线（660）｜日志 漏参族自由名锚+e2e 并行红路径冒烟（728t）｜
对账周检段周期三态+段行为桩两态+挂载结构锁（973）｜runner↔服务端失败分类契约锁（975）｜
linux 编译器缓存配置解析（auto/off/显式/非法值）+构建链行为级桩+执行机归因值/结构锁（1299）｜
分片退化编排五态+中文分支 URL 编码（1369）。
"""
from __future__ import annotations

import ast
import inspect
import os
import subprocess
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
    # #1221：ci_m6.py=E2E 段/轮写集外迁件——同受漏参自由名锚约束（外迁不减防线）
    for 文件 in ("ci_daemon.py", "ci_m6.py", "ci_daemon_selftest.py"):
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


def _973断言(主模块, 断言) -> None:
    """973（2026-10-17）：对账周检段（scripts/ci_reconcile.py）——周期判定三态 +
    段行为桩两态（临时目录隔离·桩 步骤/取对账基线·零真 git 零真跑）+ 挂载结构锁。"""
    import io
    import json as _json
    import tempfile
    import ci_reconcile

    # ① 周期判定（纯读·注入路径/周期/时刻）
    with tempfile.TemporaryDirectory() as 临时:
        状 = Path(临时) / "对账周检.json"
        断言("973 周期：无状态→到期（冷启动保守跑）",
             ci_reconcile.对账周期到期(状, 7.0, 1000000.0)[0])
        状.write_text(_json.dumps({"上次对账时刻": 1000000.0, "上次对账sha": "a" * 40}),
                      encoding="utf-8")
        断言("973 周期：距上次 1 天<7 天→未到期",
             not ci_reconcile.对账周期到期(状, 7.0, 1000000.0 + 86400)[0])
        断言("973 周期：距上次恰 7 天→到期",
             ci_reconcile.对账周期到期(状, 7.0, 1000000.0 + 7 * 86400)[0])
        状.write_text("{坏 JSON", encoding="utf-8")
        断言("973 周期：状态损坏→到期（保守=多对账不少对账）",
             ci_reconcile.对账周期到期(状, 7.0, 1000000.0)[0])
        状.write_text(_json.dumps({"上次对账sha": "a" * 40}), encoding="utf-8")
        断言("973 周期：缺「上次对账时刻」→到期（保守）",
             ci_reconcile.对账周期到期(状, 7.0, 1000000.0)[0])

    # ② 段行为（桩：假 步骤 记录命令/注入返回码；取对账基线 猴补桩）
    def 跑段(轮分支, 状态文件, 返回码=None, 桩基线=("b" * 40, "桩基线")):
        返回码 = 返回码 or {}
        调用: list = []
        步骤们: dict = {}

        def 假步骤(名, 命令, 超时秒):
            调用.append((名, [str(c) for c in 命令]))
            步骤们[名] = {"rc": int(返回码.get(名, 0))}
            return 步骤们[名]["rc"] == 0

        真基线 = ci_reconcile.取对账基线
        ci_reconcile.取对账基线 = lambda 状态, 头sha, 天数: 桩基线
        try:
            ci_reconcile.对账周检段(步骤们, 假步骤, io.StringIO(), 轮分支, "c" * 40,
                                   状态文件=状态文件, 周期天数=7.0)
        finally:
            ci_reconcile.取对账基线 = 真基线
        return 调用, 步骤们

    with tempfile.TemporaryDirectory() as 临时:
        预验状 = Path(临时) / "预验.json"
        调用, _ = 跑段("ci/预验-abc", 预验状)
        断言("973 段：预验轮零调用零落盘（设计面仅 develop 面轮）",
             调用 == [] and not 预验状.exists())
        状1 = Path(临时) / "账1.json"
        调用, 步骤们 = 跑段("develop", 状1)
        断言("973 段：develop 轮到期→两工具依序跑（负测期望对账→诊断锚对账）",
             [x[0] for x in 调用] == ["负测期望对账", "诊断锚对账"])
        断言("973 段：neg_expect_audit 命令形态（scripts 相对路径+--strict·静态门禁同款）",
             调用[0][1][1:] == ["scripts/neg_expect_audit.py", "--strict"])
        断言("973 段：check_diag_negtest 命令形态（--diff 检查点基线+--strict）",
             调用[1][1][1:] == ["scripts/check_diag_negtest.py", "--diff", "b" * 40, "--strict"])
        载 = _json.loads(状1.read_text(encoding="utf-8"))
        断言("973 段：绿→检查点推进（sha=本轮头+结果留档）",
             载.get("上次对账sha") == "c" * 40
             and 载.get("上次结果") == {"负测期望对账": 0, "诊断锚对账": 0})
        调用2, _ = 跑段("develop", 状1)
        断言("973 段：未到期→零调用（周检节拍）", 调用2 == [])
        状2 = Path(临时) / "账2.json"
        调用, 步骤们 = 跑段("develop", 状2, 返回码={"负测期望对账": 1})
        载 = _json.loads(状2.read_text(encoding="utf-8"))
        断言("973 段：红（rc=1）→步骤红播报+第二工具不被中断+检查点照推进（红=有结论）",
             步骤们["负测期望对账"]["rc"] == 1
             and [x[0] for x in 调用][-1] == "诊断锚对账"
             and 载.get("上次结果", {}).get("负测期望对账") == 1)
        状3 = Path(临时) / "账3.json"
        调用, _ = 跑段("develop", 状3, 返回码={"负测期望对账": -99})
        断言("973 段：工具超时（-99）→不推进检查点（下轮重试）且两工具均尝试",
             [x[0] for x in 调用][-1] == "诊断锚对账" and not 状3.exists())
        状4 = Path(临时) / "账4.json"
        调用, 步骤们 = 跑段("develop", 状4, 桩基线=("", "基线不可得"))
        断言("973 段：基线不可得→诊断锚对账显式留档跳过（rc=-1 显形·非静默·不推进）",
             [x[0] for x in 调用] == ["负测期望对账"]
             and 步骤们.get("诊断锚对账", {}).get("rc") == -1 and not 状4.exists())

    # ③ 挂载结构锁（防脱挂/防构建链搬回回涨）
    源一轮 = inspect.getsource(主模块.跑一轮)
    p静态, p对账 = 源一轮.find("静态门禁与e2e段("), 源一轮.find("ci_reconcile.对账周检段(")
    断言("973 结构锁：跑一轮 真调用 对账周检段 且在 静态门禁与e2e段 之后（防脱挂）",
         0 <= p静态 < p对账)
    断言("973 拆分锁：daemon 构建链=ci_build 薄壳（防搬回回涨·899 行余量保持）",
         "ci_build.win构建链" in inspect.getsource(主模块.win构建链)
         and "ci_build.linux构建链" in inspect.getsource(主模块.linux构建链))


def _975断言(主模块, 断言) -> None:
    """975（87 篇反思①·863 五连静默机械化）：runner↔服务端失败分类契约锁——
    queue_service.认领失败分类 按「结果.步骤.同步.rc 非零」识别同步失败（=runner
    池跑任务 fetch 失败/跑一轮 同步段异常的既有结果形状）；本锁防**任一端**漂移使
    服务端计数告警面静默失效（跨文件契约两端各锁一半·锁箱体=精简断言不复制实现）。"""
    源池 = inspect.getsource(主模块.池跑任务)
    断言("975 契约锁：fetch 失败结果含 步骤.同步.rc（服务端认领失败分类数据源）",
         '"步骤": {"同步": {"rc": 取.returncode}}' in 源池)
    源轮 = inspect.getsource(主模块.跑一轮)
    断言("975 契约锁：跑一轮 同步段失败落 步骤们[\"同步\"]（轮内同步异常同得分类）",
         '步骤们["同步"] = {"rc": 同步rc}' in 源轮)
    服务源 = (Path(__file__).resolve().parent / "queue_service.py").read_text(encoding="utf-8")
    # 1297：认领失败分类 实现外迁 queue_result.py（queue_service 加分片时先拆再加）——
    #   文本锁跟实现走（跨文件契约锁面不变·锁箱换文件）
    解析源 = (Path(__file__).resolve().parent / "queue_result.py").read_text(encoding="utf-8")
    断言("975 契约锁：服务端 认领失败分类 在位且按 步骤.同步.rc 判定（1297 起=queue_result）",
         "def 认领失败分类(" in 解析源 and '结果.get("步骤")' in 解析源)
    断言("975 契约锁：服务端 task_complete 调 记认领失败并告警（告警挂点不脱挂）",
         "记认领失败并告警(" in 服务源)


def _1299断言(主模块, 断言) -> None:
    """1299（2026-10-21/22 返工）：linux 编译器缓存配置解析（自动/关闭/显式/非法值
    区分·引号与分号拒绝·路径存在校验）+ 构建链行为级（假步骤捕获配置命令与日志）+
    配置命令行锁定 + 执行机归因（runner/CN_BUILD_JOBS·win 不适用）值断言与结构锁。"""
    import io
    import os
    import tempfile
    import ci_build
    import ci_report

    有cc = lambda 名: "/usr/bin/ccache" if 名 == "ccache" else None
    有sc = lambda 名: "/usr/bin/sccache" if 名 == "sccache" else None
    都有 = lambda 名: {"ccache": "/usr/bin/ccache", "sccache": "/usr/bin/sccache"}.get(名)
    无 = lambda 名: None

    决 = ci_build.解析缓存配置({}, 有cc)
    断言("1299 解析：自动命中 ccache（说明含命中路径）",
         决["launcher"] == "/usr/bin/ccache" and "自动探测命中" in 决["说明"]
         and 决["错误"] is None)
    断言("1299 解析：ccache 与 sccache 并存时优先 ccache",
         ci_build.解析缓存配置({}, 都有)["launcher"] == "/usr/bin/ccache")
    断言("1299 解析：ccache 缺失回退 sccache",
         ci_build.解析缓存配置({}, 有sc)["launcher"] == "/usr/bin/sccache")
    决 = ci_build.解析缓存配置({}, 无)
    断言("1299 解析：两者皆无=不启用且说明「均未安装」（不误报关闭）",
         决["launcher"] is None and "均未安装" in 决["说明"] and 决["错误"] is None)
    决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": "off"}, 有cc)
    断言("1299 解析：off 即使已装也关闭·说明「显式关闭」（不误报均未安装）",
         决["launcher"] is None and "显式关闭" in 决["说明"] and 决["错误"] is None)
    决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": "auto"}, 有cc)
    断言("1299 解析：auto 等同未设（正常命中）",
         决["launcher"] == "/usr/bin/ccache" and 决["错误"] is None)
    for 坏值 in ("1", "disable", "none"):
        决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": 坏值}, 无)
        断言("1299 解析：不支持的值 %r 显式报错（不静默降级）" % 坏值,
             决["launcher"] is None and 决["错误"] and "不是受支持的值" in 决["错误"])
    for 坏值, 关键词 in (('"off"', "引号"), ("off;rm -rf /", "非法字符"),
                        ("/opt/cc\nrm", "非法字符")):
        决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": 坏值}, 无)
        断言("1299 解析：%r 拒绝（%s）" % (坏值, 关键词),
             决["launcher"] is None and 决["错误"] and 关键词 in 决["错误"])
    决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": "/opt/wrap/cc"}, 无)
    断言("1299 解析：显式路径不存在=清晰错误（不原样交 cmake）",
         决["launcher"] is None and 决["错误"] and "不存在或不可执行" in 决["错误"])
    with tempfile.TemporaryDirectory() as 临时:
        候选 = Path(临时) / "cc-wrapper"
        候选.write_text("#!/bin/sh\nexec ccache \"$@\"\n", encoding="utf-8")
        os.chmod(str(候选), 0o755)
        决 = ci_build.解析缓存配置({"CN_BUILD_CACHE": str(候选)}, 无)
        断言("1299 解析：显式存在的 launcher 原样采用",
             决["launcher"] == str(候选) and 决["错误"] is None)
    断言("1299 兼容入口：非法值抛 ValueError（调用方显式红）",
         _抛值错误(lambda: ci_build.选编译器缓存({"CN_BUILD_CACHE": "1"}, 无)))
    断言("1299 配置命令：无缓存也显式传空 launcher（清 CMakeCache 残留）",
         ci_build.linux配置命令(None) == ["cmake", "-S", ".", "-B", "target/build",
                                       "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING="])
    断言("1299 配置命令：有缓存=launcher 路径入参",
         ci_build.linux配置命令("/usr/bin/ccache")[-1]
         == "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=/usr/bin/ccache")

    # 行为级：假步骤捕获配置命令+日志（猴补 解析缓存配置/仓库根·零真构建）
    def 跑链(决策: dict) -> tuple:
        真解, 真根 = ci_build.解析缓存配置, ci_build.仓库根
        记录: list = []
        步骤们: dict = {}
        日志 = io.StringIO()
        try:
            ci_build.解析缓存配置 = lambda: dict(决策)
            with tempfile.TemporaryDirectory() as 临时:
                ci_build.仓库根 = Path(临时)

                def 假步骤(名, 命令, 超时秒):
                    记录.append((名, 命令))
                    步骤们[名] = {"rc": 0}
                    return True
                ci_build.linux构建链(步骤们, 假步骤, 4, 3600, 日志)
        finally:
            ci_build.解析缓存配置, ci_build.仓库根 = 真解, 真根
        return 记录, 步骤们, 日志.getvalue()

    记录, 步骤们, 日志文 = 跑链({"launcher": "/usr/bin/ccache",
                                  "说明": "自动探测命中 /usr/bin/ccache", "错误": None})
    配置命令 = next((命令 for 名, 命令 in 记录 if 名 == "配置"), [])
    断言("1299 构建链行为：配置命令带 ccache launcher 且日志留痕",
         配置命令 and 配置命令[-1].endswith("/usr/bin/ccache")
         and "[1299] 编译器缓存：自动探测命中" in 日志文)
    记录, 步骤们, 日志文 = 跑链({"launcher": None, "说明": "配置非法",
                                  "错误": "CN_BUILD_CACHE=%r 指向的 launcher 不存在或不可执行" % "x"})
    断言("1299 构建链行为：非法配置=配置步 rc=-1 且零命令执行（不静默降级）",
         步骤们.get("配置", {}).get("rc") == -1 and not 记录
         and "不存在或不可执行" in 日志文)

    断言("1299 归因：linux 字段含 runner/CN_BUILD_JOBS",
         ci_report.执行机字段("home-wsl-1", 4, False)
         == {"runner": "home-wsl-1", "CN_BUILD_JOBS": 4})
    断言("1299 归因：win 不写不适用项（Ninja --parallel 全核·不虚报默认 2）",
         ci_report.执行机字段("CHENCHAO-PC-win1", 2, True) == {"runner": "CHENCHAO-PC-win1"})
    for 轮 in ("跑一轮", "跑分片轮"):
        断言("1299 归因锁：%s 经 ci_report.执行机字段 组装（防回退为写死）" % 轮,
             "ci_report.执行机字段(runner标识, 构建并行, 是win)"
             in inspect.getsource(getattr(主模块, 轮)))


def _抛值错误(调用) -> bool:
    """1299：断言调用抛 ValueError（兼容入口非法配置显式红）。"""
    try:
        调用()
    except ValueError:
        return True
    return False


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
    # 1297：兜底入池 实现外迁 ci_fallback.py——文本锁跟实现走
    兜底源 = (Path(__file__).resolve().parent / "ci_fallback.py").read_text(encoding="utf-8")
    断言("770 文本锁：兜底入池零认领限制实参（服务端轮转唯一权威·1297 起=ci_fallback）",
         '"task_enqueue", {"分支": "ci/兜底-%s" % sha[:10], "sha": sha}' in 兜底源)
    断言("770 修复锁：头已交池（sha in 已入池）分支在位且先于开跑——本机不重复直跑"
         "（三平台同验实录 2026-10-14 深夜）",
         "if sha in 已入池 or list(日志目录.glob" in 源
         and 源.index("if sha in 已入池 or list") < 源.rindex("[开跑]"))
    断言("770 修复锁：入池失败降级显式化（[降级] 输出·门禁永不缺位）",
         "[降级] 兜底入池失败" in 源)


def _1297断言(断言) -> None:
    """1297 分片聚合纯函数正反两态（ci_shard）——合并e2e明细（求和/并集去重/
    分片行附加/全无效=None 不编造假明细·648）与 片步骤值（全绿才 0·秒=max）。"""
    import ci_shard
    片0 = {"目标平台": "linux-x86_64", "总计": 5, "通过": 4, "失败": 1,
           "未实现": 0, "跳过": 0,
           "失败用例": [{"名称": "01_a", "原因": "x"}], "未实现用例": [],
           "M6": {"状态": "未跑", "原因": "本片不含"}}
    片2 = {"目标平台": "linux-x86_64", "总计": 5, "通过": 5, "失败": 0,
           "未实现": 1, "跳过": 0,
           "失败用例": [], "未实现用例": [{"名称": "09_u", "原因": "尚无"}],
           "M6": {"状态": "通过", "fix_p_md5": "abc"}}
    片2b = {"总计": 3, "通过": 3, "失败": 0, "未实现": 0, "跳过": 0,
            "失败用例": [{"名称": "01_a", "原因": "跨片重复上报"}], "未实现用例": []}
    行们 = [{"片序": 1, "认领者": "w1", "秒": 100, "绿": 1, "状态": "完成"}]
    合并 = ci_shard.合并e2e明细([片0, None, 片2, 片2b], 行们)
    断言("1297 合并：计数求和（总计 13=5+5+3·失败 1）",
         合并["总计"] == 13 and 合并["通过"] == 12 and 合并["失败"] == 1
         and 合并["未实现"] == 1)
    断言("1297 合并：失败用例跨片按名并集去重（01_a 只留首见·不虚增）",
         [例["名称"] for 例 in 合并["失败用例"]] == ["01_a"]
         and 合并["失败用例"][0]["原因"] == "x")
    断言("1297 合并：未实现用例收齐+分片行附加（观测面）",
         [例["名称"] for 例 in 合并["未实现用例"]] == ["09_u"]
         and 合并["分片"] == 行们)
    断言("1297 合并反态：全片无有效明细（旧版 run_e2e）=None（不编造假全零明细·648）",
         ci_shard.合并e2e明细([None, None], []) is None)
    断言("#1221 合并：M6 记录取「通过」优先（片 0 未跑+片 2 通过→通过）",
         合并.get("M6", {}).get("状态") == "通过"
         and 合并["M6"].get("fix_p_md5") == "abc")
    断言("1297 片步骤值：全绿 rc=0·秒=max（墙钟=最慢片）",
         ci_shard.片步骤值([0, 0, 0], [10, 300, 200]) == {"rc": 0, "秒": 300})
    断言("1297 片步骤值：任一红=首个非 0 诚实红（不吞片红）",
         ci_shard.片步骤值([0, 1, 2], [10, 20])["rc"] == 1)
    # 结构锁（10-23 P0 实录）：ci_daemon 调用点解包数量必须=ci_shard.主实例分片段
    #   返回元组数量（5=rc/秒/明细/片行/形态）。池实例重启后从被验树加载代码——
    #   4 元解包×5 元返回=ValueError: too many values to unpack 崩轮→任务回收重派
    #   （1394 预验四度重派根因·home-wsl-2 journal 实证）。
    import ast as _ast
    import inspect as _inspect
    # #1221：E2E 段外迁 ci_m6（调用点随迁移）——两文件合并扫描解包形态
    源 = ""
    for _名 in ("ci_daemon.py", "ci_m6.py"):
        源 += (Path(__file__).resolve().parent / _名).read_text(encoding="utf-8") + "\n"
    解包 = None
    for 节点 in _ast.walk(_ast.parse(源)):
        if (isinstance(节点, _ast.Assign) and isinstance(节点.value, _ast.Call)
                and "主实例分片段" in _ast.unparse(节点.value.func)):
            if 节点.targets and isinstance(节点.targets[0], _ast.Tuple):
                解包 = len(节点.targets[0].elts)
    函数树 = _ast.parse(_inspect.getsource(ci_shard.主实例分片段))
    返回们 = [len(n.value.elts) for n in _ast.walk(函数树)
            if isinstance(n, _ast.Return) and isinstance(n.value, _ast.Tuple)]
    断言("1297 结构锁：ci_daemon 解包数=主实例分片段返回数（解包 %s·返回 %s）"
         % (解包, 返回们),
         解包 is not None and bool(返回们) and all(r == 解包 for r in 返回们))


def _1369断言(主模块, 断言) -> None:
    """1369（2026-10-21·#1297 预验分片系统性超时·集成通道阻塞 P0）：主实例分片
    退化编排五态直调（桩=ci_shard 协议函数/步骤记录器·零真池零真跑）：
    ①无认领→宽限后退化单实例；②部分认领→收已认领+补跑缺失；③全认领→正常汇总；
    ④退化不重复跑/不丢片（迟到的完成片照收）；⑤退化结果 JSON 与单实例轮同格式；
    另锁⑥中文分支 URL percent-encode（5400s 空等病根之一）与⑦从片红=整轮红即停。
    桩面语义=服务端 CAS：仅缺口片可抢（执行中片抢补返回 ok=False——活片不双跑）。"""
    import io
    import json as _json
    import time as _time
    import ci_shard

    真件 = {名: getattr(ci_shard, 名) for 名 in
            ("片状态们", "抢补片", "交片", "片结果", "池配置",
             "认领宽限秒", "收片窗秒", "收片轮询秒")}

    def 行(片序, 状态, 绿=None, 秒=None):
        return {"片序": 片序, "状态": 状态, "绿": 绿, "秒": 秒}

    def 单例明细(总计=1):
        return {"目标平台": "linux-x86_64", "总计": 总计, "通过": 总计, "失败": 0,
                "未实现": 0, "跳过": 0, "失败用例": [], "未实现用例": []}

    def 跑(快照脚本, *, 宽限=0, 可抢=(), 片数=3, 从片总计=4):
        记录 = {"步骤": [], "抢补": [], "交片": []}
        序 = {"i": 0}
        日志 = io.StringIO()

        def 快照():
            i = min(序["i"], len(快照脚本) - 1)
            序["i"] += 1
            return [dict(x) for x in 快照脚本[i]]

        def 桩抢补(注入, runner, 分支, k):
            记录["抢补"].append(k)
            return {"ok": k in 可抢}

        def 桩交片(注入, runner, 分支, k, 绿, 结果, 秒):
            记录["交片"].append((k, 绿))
            return True

        def 桩片结果(池地址, 池令牌, 分支, k):
            return {"ok": True, "绿": True, "秒": 5, "结果": 单例明细(从片总计)}

        ci_shard.片状态们 = lambda *a: 快照()
        ci_shard.抢补片 = 桩抢补
        ci_shard.交片 = 桩交片
        ci_shard.片结果 = 桩片结果
        ci_shard.池配置 = lambda: {"池地址": "http://stub", "池令牌": ""}
        ci_shard.认领宽限秒 = 宽限
        ci_shard.收片窗秒 = 2
        ci_shard.收片轮询秒 = 0.01
        步骤们 = {}

        def 桩步骤(名, 命令, 超时秒):
            记录["步骤"].append(名)
            路径 = Path(命令[命令.index("--json") + 1])
            路径.write_text(_json.dumps(单例明细()), encoding="utf-8")
            步骤们[名] = {"rc": 0, "秒": 10}
            return True

        with tempfile.TemporaryDirectory() as 临时:
            rc, 秒, 合并, 行们, 形态 = ci_shard.主实例分片段(
                轮分支="ci/预验-abc123", 片数=片数, 召唤时刻=_time.time(),
                e2e基=["python", "tests/e2e/run_e2e.py"],
                明细参=lambda p: ["--json", str(p)], 步骤=桩步骤, 步骤们=步骤们,
                日志=日志, sha="a" * 40, 日志目录=Path(临时), e2e并行=2,
                e2e超时秒=10, runner="桩runner", 读明细=ci_shard.读e2e明细)
        return SimpleNamespace(rc=rc, 秒=秒, 合并=合并, 行们=行们 or [],
                               形态=形态, 步骤=记录["步骤"], 抢补=记录["抢补"],
                               交片=记录["交片"], 日志=日志.getvalue())

    try:
        r = 跑([[行(1, "缺口"), 行(2, "缺口")]], 宽限=0, 可抢={1, 2})
        断言("1369 ①无认领→宽限后退化单实例（片0+缺失片全补跑·总计守恒）",
             r.形态 == "退化单实例" and r.步骤 == ["e2e并行", "e2e片1", "e2e片2"]
             and r.rc == 0 and r.合并["总计"] == 3)
        断言("1369 ①退化显式留痕（[分片退化] 不静默换道）", "[分片退化]" in r.日志)
        单例键 = {"目标平台", "时刻", "总计", "通过", "失败", "未实现", "跳过",
                "失败用例", "未实现用例"}
        断言("1369 ⑤退化结果 JSON 与单实例轮同格式（键集一致·无分片键）",
             set(r.合并) == 单例键 and "分片" not in r.合并)
        计刻 = _time.time()
        r = 跑([[行(1, "缺口"), 行(2, "缺口")]], 宽限=0.05, 可抢={1, 2})
        断言("1369 ①宽限窗生效（无认领等满 CN_SHARD_GRACE_SEC 才退化·不提前抢跑）",
             r.形态 == "退化单实例" and _time.time() - 计刻 >= 0.04)

        r = 跑([[行(1, "执行中"), 行(2, "缺口")],
                [行(1, "执行中"), 行(2, "缺口")],
                [行(1, "完成", 1, 5), 行(2, "完成", 1, 4)]], 宽限=1, 可抢={2})
        断言("1369 ②部分认领→收已认领片（不重跑）+补跑缺失片·总计守恒",
             r.形态 == "分片" and r.步骤 == ["e2e并行", "e2e片2"]
             and r.rc == 0 and r.合并["总计"] == 6)
        断言("1369 ②已认领片从服务端收明细并附分片观测行",
             len(r.行们) == 2 and "分片" in r.合并)

        r = 跑([[行(1, "执行中"), 行(2, "执行中")],
                [行(1, "完成", 1, 5),行(2, "完成", 1, 4)]], 宽限=1)
        断言("1369 ③全认领→正常汇总（本机零补跑·总计=主片+双片）",
             r.形态 == "分片" and r.步骤 == ["e2e并行"]
             and r.rc == 0 and r.合并["总计"] == 9 and "分片" in r.合并)

        r = 跑([[行(1, "缺口"), 行(2, "缺口")],
                [行(1, "完成", 1, 5), 行(2, "缺口")],
                [行(1, "完成", 1, 5), 行(2, "缺口")]], 宽限=0, 可抢={2})
        断言("1369 ④退化不重复跑已由从片完成的片（迟到完成照收·只补缺失片）",
             r.形态 == "退化单实例" and r.步骤 == ["e2e并行", "e2e片2"]
             and r.交片 == [(2, True)])
        断言("1369 ④退化不丢片（各片明细计数全计入·总计=主片+完成片+补跑片）",
             r.合并["总计"] == 1 + 4 + 1)

        r = 跑([[行(1, "执行中"), 行(2, "执行中")],
                [行(1, "完成", 0, 5), 行(2, "执行中")]], 宽限=1)
        断言("1369 ⑦从片红→整轮红即停（不放宽门禁·零本地补跑）",
             r.rc == 1 and r.步骤 == ["e2e并行"] and "[红即停]" in r.日志)

        # ⑥ 中文分支 URL percent-encode——用真 片状态们 测：旧直拼触发
        #    UnicodeEncodeError 被吞成 None（主实例既不抢补也不判齐=5400s 空等病根）。
        真urlopen = ci_shard.urllib.request.urlopen
        捕获: dict = {}

        class 假响应:
            def __enter__(self):
                return self

            def __exit__(self, *a):
                return False

            def read(self):
                return '{"ok": true, "分片": []}'.encode("utf-8")

        try:
            def 桩urlopen(网址, timeout=10, **kw):
                捕获["url"] = getattr(网址, "full_url", 网址)
                return 假响应()
            ci_shard.urllib.request.urlopen = 桩urlopen
            快照回 = 真件["片状态们"]("http://tx01:8300", "", "ci/预验-abc123")
        finally:
            ci_shard.urllib.request.urlopen = 真urlopen
        断言("1369 ⑥中文分支 GET percent-encode（预验=%E9%A2%84%E9%AA%8C·病根不复现）",
             "预验" not in 捕获.get("url", "")
             and "%E9%A2%84%E9%AA%8C" in 捕获.get("url", "") and 快照回 == [])
    finally:
        for 名, 件 in 真件.items():
            setattr(ci_shard, 名, 件)


def _1423断言(断言) -> None:
    """#1423 本机共享门禁锁探测：锁不存在=不忙；活 pid=忙（跳过领片避免排队）；
    死 pid+心跳陈旧=陈锁不忙（可接管）；探测异常=不忙（不阻塞正常领任务）。"""
    import os
    import time as _t
    import ci_shard
    with tempfile.TemporaryDirectory() as 临时:
        锁 = Path(临时) / "gate.lock"
        断言("#1423 锁忙：锁不存在=不忙（无同机实例在跑）",
             not ci_shard.本机门禁锁忙(锁))
        锁.mkdir()
        (锁 / "info.json").write_text(
            '{"pid": %d, "形态": "run", "获取时间": "x"}' % os.getpid(),
            encoding="utf-8")
        断言("#1423 锁忙：锚 pid 存活=忙（同机实例在跑轮→调用方跳过领片）",
             ci_shard.本机门禁锁忙(锁))
        (锁 / "info.json").write_text(
            '{"pid": 999999, "形态": "run", "获取时间": "x"}', encoding="utf-8")
        旧 = _t.time() - 86400
        os.utime(锁 / "info.json", (旧, 旧))
        断言("#1423 锁忙：死 pid+心跳陈旧=陈锁不忙（可接管·不误跳过）",
             not ci_shard.本机门禁锁忙(锁))


def _1424断言(主模块, 断言) -> None:
    """#1424 分片轮独立树锁：带门禁锁跑(分片轮=True) 取本实例树锁目录（各池树独立·
    同机三 win 可并行），整任务/兜底轮仍取主树共享锁（923 OOM 防线）。桩 gate_lock
    获取记录 锁 参数 + 桩释放（零真锁·跨平台强制 是win=True 走 win 分支）。"""
    import gate_lock
    记录 = []
    原件获取, 原件释放 = gate_lock.获取, gate_lock.释放
    原win = getattr(主模块, "是win", None)
    with tempfile.TemporaryDirectory() as 临时:
        假锁 = Path(临时) / "gate.lock"
        假锁.mkdir()

        def 桩获取(超时秒, 形态="run", owner_pid=None, 锁=None,
                 轮心跳=False, 清护=False):
            记录.append(锁)
            return 假锁

        gate_lock.获取 = 桩获取
        gate_lock.释放 = lambda *a, **k: None
        主模块.是win = True
        try:
            主模块.带门禁锁跑("a" * 40, "develop",
                             跑器=lambda s, b: {"绿": True}, 分片轮=True)
            主模块.带门禁锁跑("a" * 40, "develop", 跑器=lambda s, b: {"绿": True})
        finally:
            gate_lock.获取, gate_lock.释放 = 原件获取, 原件释放
            主模块.是win = 原win
    断言("#1424 分片轮：取本实例树锁目录（各池树独立·同机可并行）",
         len(记录) == 2 and 记录[0] == gate_lock.树锁目录())
    断言("#1424 整任务/兜底轮：仍取主树共享锁（923 OOM 防线）",
         len(记录) == 2 and 记录[1] is None)
    源 = (Path(__file__).resolve().parent / "ci_daemon.py").read_text(encoding="utf-8")
    断言("#1424 结构锁：池跑分片 调用带门禁锁跑时传 分片轮=True",
         "分片轮=True" in 源)


def _1428断言(断言) -> None:
    """#1428 win 让位判断：池内有空闲 linux 实例→True（win 跳过领片/任务）；
    只有 win 空闲/无空闲/平台未知→False（win 照常补位）；状态不可判=None→False。"""
    import time as _t
    import ci_shard
    now = int(_t.time())

    def 状态(条目):
        return {"实例动态": 条目}

    断言("#1428 让位：空闲 linux 实例存在→True（win 让算力）",
         ci_shard.有非win空闲(状态([
             {"状态": "空闲", "平台": "linux-arm64", "最近活跃时戳": now - 30}])))
    断言("#1428 让位反态：只有 win 空闲→False（win 照常补位）",
         not ci_shard.有非win空闲(状态([
             {"状态": "空闲", "平台": "win-x64", "最近活跃时戳": now - 30}])))
    断言("#1428 让位反态：linux 实例忙/离线→False",
         not ci_shard.有非win空闲(状态([
             {"状态": "执行中", "平台": "linux-x86_64", "最近活跃时戳": now - 30},
             {"状态": "空闲", "平台": "linux-x86_64", "最近活跃时戳": now - 7200}])))
    断言("#1428 让位反态：平台未知（旧 daemon 空平台）不计入",
         not ci_shard.有非win空闲(状态([
             {"状态": "空闲", "平台": "", "最近活跃时戳": now - 30}])))
    断言("#1428 让位反态：池状态不可判=None→False（不误让位）",
         not ci_shard.有非win空闲(None))
    源 = (Path(__file__).resolve().parent / "ci_daemon.py").read_text(encoding="utf-8")
    import inspect as _inspect
    源get = _inspect.getsource(ci_shard.池状态) + _inspect.getsource(ci_shard.片状态们)
    断言("#1428 结构锁：GET（池状态/片状态）带 Bearer（#1425 登录墙·无令牌=401）",
         "Authorization" in 源get)
    断言("#1428 结构锁：池循环含 win 让位判断（[让位] + 有非win空闲）",
         "[让位]" in 源 and "有非win空闲" in 源)


def _1426断言(主模块, 断言) -> None:
    """#1426 分片按物理机限并发（客户端面）：物理机指纹=508 三要素且进程内稳定；
    领片/抢补/整任务认领/实例报活四处携带（服务端限流/让位数据源）。服务端限流
    行为见 queue_selftest.py 1426 段（同机 1 片/跨机并/上限 2/整任务不受限/缺失降级）。"""
    import inspect as _inspect
    import re as _re
    import ci_shard
    机 = ci_shard.物理机标识()
    段 = 机.split("·")
    断言("#1426 物理机指纹=hostname·平台-架构·指纹6（508 三要素）",
         len(段) == 3 and bool(段[0]) and bool(段[1])
         and _re.fullmatch("[0-9a-f]{6}", 段[2]) is not None)
    断言("#1426 指纹进程内稳定（缓存·报活/领片不重复探测）",
         ci_shard.物理机标识() == 机)
    记录 = {}

    def 桩池调用(操作, 数据):
        记录["操作"] = 操作
        记录.update(数据)
        return {"ok": True, "片": None}

    ci_shard.领片(桩池调用, "r1426", "win-x64")
    断言("#1426 领片请求携带物理机指纹（服务端按机限流）",
         记录.get("操作") == "task_shard_claim" and 记录.get("runner") == "r1426"
         and 记录.get("平台") == "win-x64" and 记录.get("物理机") == 机)
    记录.clear()
    ci_shard.抢补片(桩池调用, "r1426", "ci/预验-x", 2)
    断言("#1426 主实例抢补请求也带指纹（片行审计·服务端不限流）",
         记录.get("物理机") == 机 and 记录.get("片序") == 2)
    源daemon = _inspect.getsource(主模块.池认领任务)
    断言("#1426 整任务认领携带物理机指纹（主实例片 0 计入同机限流）",
         '"物理机"' in 源daemon and "物理机标识" in 源daemon)
    源报活 = _inspect.getsource(ci_shard.启动实例报活)
    断言("#1426 实例报活携带物理机指纹（空闲实例落表供让位判定）",
         '"物理机"' in 源报活 and "物理机标识" in 源报活)


def _1401断言(断言) -> None:
    """#1401 自适应分片度：任务少时一个任务吃满空闲实例、多任务按任务数均分；
    旧服务端无实例动态退回池忙闲；零空闲=1（跳过召唤·不空等宽限窗）；CN_SHARD_N
    显式覆盖。纯函数直调（零网络·真实池快照路径由 计算分片数 单点拼接）。"""
    import os
    import time
    import ci_shard

    def 状态(执行中=0, 注册=0, 预验=0, 空闲活跃=None):
        d = {"池忙闲": {"注册实例": 注册, "执行中": 执行中},
             "预验任务": [{"分支": "ci/预验-%d" % i, "状态": "执行中"}
                        for i in range(预验)]}
        if 空闲活跃 is not None:
            现在 = int(time.time())
            d["实例动态"] = [{"状态": "空闲", "最近活跃时戳": 现在 - 60}] * 空闲活跃
        return d

    断言("1401 自适应：池不可达=保守 3（与现网既有 n=3 等价）",
         ci_shard.估算分片数(None) == 3)
    断言("1401 自适应：3 预验任务/9 空闲→n=4（均分空闲·每任务 1 主 3 从）",
         ci_shard.估算分片数(状态(执行中=3, 注册=12, 预验=3, 空闲活跃=9)) == 4)
    断言("1401 自适应：1 预验任务/9 空闲→n=10（一个任务吃满空闲实例）",
         ci_shard.估算分片数(状态(执行中=1, 注册=10, 预验=1, 空闲活跃=9)) == 10)
    断言("1401 自适应：上限 clamp 12（1 任务/30 空闲）",
         ci_shard.估算分片数(状态(执行中=1, 注册=31, 预验=1, 空闲活跃=30)) == 12)
    断言("1401 自适应：零空闲→n=1（不召唤·不空等宽限窗）",
         ci_shard.估算分片数(状态(执行中=3, 注册=3, 预验=2, 空闲活跃=0)) == 1)
    断言("1401 自适应：旧服务端无实例动态→注册-执行中退化估算",
         ci_shard.估算分片数(状态(执行中=2, 注册=8, 预验=2)) == 4)
    旧 = os.environ.get("CN_SHARD_N")
    try:
        os.environ["CN_SHARD_N"] = "1"
        断言("1401 显式 CN_SHARD_N=1=分片关闭", not ci_shard.分片启用())
        os.environ["CN_SHARD_N"] = "5"
        断言("1401 显式 CN_SHARD_N=5=固定片数（池状态不参与）",
             ci_shard.分片启用() and ci_shard.计算分片数("") == 5)
    finally:
        if 旧 is None:
            os.environ.pop("CN_SHARD_N", None)
        else:
            os.environ["CN_SHARD_N"] = 旧


def _1392断言(断言) -> None:
    """1392 实例空闲报活：实例指纹纯函数（daemon指纹=ci_daemon.py 内容 sha256 前 10
    ——同内容同指纹·内容变即变=scp 直更同样可感知；树头非 git 目录=空串显式不猜，
    服务端空串不覆盖已有值）。报活写入口（服务端 instance_ping）与四态派生由
    queue_selftest 1392 段覆盖（端到端·此处只测客户端指纹面）。"""
    import hashlib
    import ci_shard
    with tempfile.TemporaryDirectory() as 临时:
        根 = Path(临时)
        (根 / "scripts").mkdir()
        文件 = 根 / "scripts" / "ci_daemon.py"
        文件.write_text("# 1392 指纹桩\n", encoding="utf-8")
        指 = ci_shard.实例指纹(根)
        期望 = hashlib.sha256(
            文件.read_text(encoding="utf-8").replace("\r\n", "\n").encode("utf-8")
        ).hexdigest()[:10]
        断言("1392 指纹：daemon指纹=文件内容 sha256 前 10（同内容同指纹·文本归一化）",
             指["daemon指纹"] == 期望 and len(指["daemon指纹"]) == 10)
        断言("1392 指纹：非 git 目录树头=空串显式（不猜不编·前端显示 —）",
             指["树头"] == "")
        文件.write_text("# 内容变→指纹变（scp 直更同样反映）\n", encoding="utf-8")
        断言("1392 指纹：内容变更=指纹变更（旧版实例可被众数比对抓出）",
             ci_shard.实例指纹(根)["daemon指纹"] != 期望)
        # #1418：CRLF/LF 同内容同指纹（win 池树 checkout 为 CRLF·linux 为 LF——
        #   同提交同代码不得因行尾被判「版本不一致」跨平台假警）
        with 文件.open("w", encoding="utf-8", newline="") as f:
            f.write("# 行尾桩\r\n第二行\r\n")
        指CRLF = ci_shard.实例指纹(根)["daemon指纹"]
        with 文件.open("w", encoding="utf-8", newline="") as f:
            f.write("# 行尾桩\n第二行\n")
        指LF = ci_shard.实例指纹(根)["daemon指纹"]
        断言("#1418 指纹：CRLF/LF 同内容同指纹（跨平台行尾不再假警）",
             指CRLF == 指LF and len(指CRLF) == 10)


def _1460断言(断言) -> None:
    """#1460 实例刷新（「更新实例」按钮）：需刷新判定两态 + 对齐树头真实 git 两态
    （成功=reset 到远端头并返回新 sha；失败=RuntimeError 显式，不静默吞）。
    服务端广播戳存取与 ping 搭车下发由 queue_selftest #1460 段覆盖（协议端到端）。"""
    import ci_shard
    断言("#1460 需刷新正：请求戳>已执行戳 → 应对齐",
         ci_shard.需刷新({"请求": 200.0, "已执行": 100.0}) is True)
    断言("#1460 需刷新反：请求≤已执行/键缺失（从未广播）=不动作",
         ci_shard.需刷新({"请求": 100.0, "已执行": 100.0}) is False
         and ci_shard.需刷新({}) is False)
    with tempfile.TemporaryDirectory() as 临时:
        源, 克 = Path(临时) / "源", Path(临时) / "克"
        源.mkdir()
        环境 = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t",
                "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@t",
                "GIT_CONFIG_GLOBAL": str(Path(临时) / "gitconfig"),
                "GIT_CONFIG_SYSTEM": os.devnull}

        def 跑(目录, *参):
            return subprocess.run(["git", *参], cwd=str(目录), capture_output=True,
                                  text=True, env=环境)
        跑(源, "init", "-q", "-b", "develop")
        (源 / "a.txt").write_text("2", encoding="utf-8")
        跑(源, "add", ".")
        跑(源, "commit", "-q", "-m", "一")
        跑(Path(临时), "clone", "-q", str(源), str(克))
        (源 / "a.txt").write_text("3", encoding="utf-8")
        跑(源, "commit", "-q", "-am", "二")
        新头 = ci_shard.对齐树头(克, str(源), "develop")
        断言("#1460 对齐树头正：fetch+reset 到远端头并返回新 HEAD（树内容同步）",
             新头 == 跑(源, "rev-parse", "HEAD").stdout.strip()
             and (克 / "a.txt").read_text(encoding="utf-8") == "3")
        try:
            ci_shard.对齐树头(克, str(Path(临时) / "不存在"), "develop")
            失败显式 = False
        except RuntimeError:
            失败显式 = True
        断言("#1460 对齐树头反：无效远端=RuntimeError 显式（不静默吞·下轮重试）",
             失败显式)


def _1463断言(主模块, 断言) -> None:
    """#1463 面板版本列失真根治两态：①树头实时——取树头 每次重算（启动快照缺陷
    根治·刷新/任务 checkout 后面板即见）；②结构锁——池跑任务/池跑分片 收尾均
    跑完对齐（树头回 develop·防跑完停在任务分支头致恒红）。"""
    import ci_shard
    with tempfile.TemporaryDirectory() as 临时:
        根 = Path(临时)
        环境 = {**os.environ, "GIT_AUTHOR_NAME": "t", "GIT_AUTHOR_EMAIL": "t@t",
                "GIT_COMMITTER_NAME": "t", "GIT_COMMITTER_EMAIL": "t@t",
                "GIT_CONFIG_GLOBAL": str(Path(临时) / "gitconfig"),
                "GIT_CONFIG_SYSTEM": os.devnull}

        def 跑(*参):
            return subprocess.run(["git", *参], cwd=str(根), capture_output=True,
                                  text=True, env=环境)
        跑("init", "-q", "-b", "develop")
        (根 / "a.txt").write_text("1", encoding="utf-8")
        跑("add", ".")
        跑("commit", "-q", "-m", "一")
        头一 = ci_shard.取树头(根)
        (根 / "a.txt").write_text("2", encoding="utf-8")
        跑("commit", "-qam", "二")
        头二 = ci_shard.取树头(根)
        断言("#1463 树头实时：二次取树头随 HEAD 变化（面板不再恒显启动快照）",
             头一 != 头二 and len(头一) == 10
             and 头二 == 跑("rev-parse", "HEAD").stdout.strip()[:10])
    源码 = inspect.getsource(主模块.池跑任务) + inspect.getsource(主模块.池跑分片)
    断言("#1463 结构锁：池跑任务/池跑分片 收尾均 跑完对齐（树头回 develop·防恒红）",
         源码.count("跑完对齐(仓库根)") == 2)


def _1468断言(主模块, 断言) -> None:
    """#1468 兜底轮过期早退结构锁：①跑一轮 构建后含 develop 头核对（过期早退+
    早退新头标记·顺序在构建段之后）；②池跑任务 据早退新头按过期语义闭环（不落盘）。"""
    跑源 = inspect.getsource(主模块.跑一轮)
    断言("#1468 结构锁：跑一轮 构建后核对 develop 头并早退（现头≠sha→跳过 e2e）",
         "过期早退" in 跑源 and "早退新头" in 跑源
         and 跑源.index("构建段执行") < 跑源.index("过期早退"))
    池源 = inspect.getsource(主模块.池跑任务)
    断言("#1468 结构锁：池跑任务 早退结果转过期语义（绿闭环·不落盘）",
         '结果.get("早退新头")' in 池源 and "过期跳过结果" in 池源)


def _1221断言(主模块, 断言) -> None:
    """#1221 自举固定点机械触发器：轮写集（git 胶水·注入运行）三态 + 轮计划
    三态（触发/豁免/交叉模拟转派）+ E2E 段 M6 门禁三态（命令带 --require-m6 /
    预验交叉模拟 fail-closed / develop 转派）+ 聚合绿 M6平台保障 红绿两态。
    全部零真 git 零真构建（运行=桩·步骤=桩·临时 cn 与明细目录隔离）。"""
    import io
    import tempfile
    import ci_m6
    import m6_gate

    class 桩结果:
        def __init__(self, rc=0, out="", err=""):
            self.returncode, self.stdout, self.stderr = rc, out, err

    # ① 预验轮：三点 diff + quotepath=false（中文路径前缀判定不被转义静默击穿）
    记录: list = []

    def 桩运行(命令):
        记录.append(list(命令))
        return 桩结果(0, "src/cn_compiler/ir/ir.cpp\nCN语言编译器v2/代码生成/代码生成.cn\n")

    文件们, 原因 = ci_m6.轮写集("ci/预验-abc123", "a" * 40, Path("."),
                                 运行=桩运行, 基线查=lambda: "b" * 40)
    断言("#1221 预验轮写集：三点 diff（origin/develop...sha）+ 中文路径 quotepath 关",
         文件们 == ["src/cn_compiler/ir/ir.cpp", "CN语言编译器v2/代码生成/代码生成.cn"]
         and "-c" in 记录[0] and "core.quotepath=false" in 记录[0]
         and 记录[0][-1] == "origin/develop..." + "a" * 40)

    # ② develop/兜底轮：基线=上次已验绿 sha（.. 两点）；基线=本轮 sha 时退 sha~1
    记录.clear()
    ci_m6.轮写集("develop", "a" * 40, Path("."), 运行=桩运行, 基线查=lambda: "c" * 40)
    ci_m6.轮写集("develop", "a" * 40, Path("."), 运行=桩运行, 基线查=lambda: "a" * 40)
    断言("#1221 develop 轮写集：基线=上次已验绿（两点）；同 sha 退 sha~1",
         记录[0][-1] == "c" * 40 + ".." + "a" * 40
         and 记录[1][-1] == "a" * 40 + "~1.." + "a" * 40)

    # ③ git 失败=不可判（调用方 fail-safe 触发，禁静默当空写集）
    文件们错, 原因错 = ci_m6.轮写集("develop", "a" * 40, Path("."),
                                    运行=lambda 命令: 桩结果(128, "", "fatal"),
                                    基线查=lambda: "c" * 40)
    断言("#1221 写集不可判：git 失败→None+原因（fail-safe 触发）",
        文件们错 is None and "git diff 失败" in 原因错)

    # ④ 轮计划三态：codegen 触发/文档豁免/交叉模拟 arm64 转派
    码运行 = lambda 命令: 桩结果(0, "src/cn_compiler/codegen/x64/a.cpp\n")
    文运行 = lambda 命令: 桩结果(0, "docs/a.md\n")
    必跑 = ci_m6.轮计划("ci/预验-x", "a" * 40, "win-x64", True, Path("."), "origin",
                        "AMD64", 基线查=lambda: "b" * 40, 运行=码运行)
    豁免 = ci_m6.轮计划("ci/预验-x", "a" * 40, "win-x64", True, Path("."), "origin",
                        "AMD64", 基线查=lambda: "b" * 40, 运行=文运行)
    转派 = ci_m6.轮计划("ci/预验-x", "a" * 40, "linux-arm64", False, Path("."),
                        "origin", "x86_64", 基线查=lambda: "b" * 40, 运行=码运行)
    断言("#1221 轮计划：codegen 写集→必跑（win 原生）",
         必跑["处置"] == "必跑" and 必跑["平台可跑"] and 必跑["命中"])
    断言("#1221 轮计划：纯文档写集→未触发（零成本）", 豁免["处置"] == "未触发")
    断言("#1221 轮计划：交叉模拟 arm64→未跑-转派（显式原因）",
         转派["处置"] == "未跑-转派" and not 转派["平台可跑"])
    断言("#1221 轮计划：写集不可判→fail-safe 必跑",
         ci_m6.轮计划("develop", "a" * 40, "win-x64", True, Path("."), "origin", "AMD64",
                     基线查=lambda: "c" * 40,
                     运行=lambda 命令: 桩结果(1, "", "bad"))["处置"] == "必跑")

    # ⑤ 平台判据：真机 arm64 可跑/交叉模拟不可跑/win 原生
    断言("#1221 平台判据：arm64 真机可跑·交叉模拟不可跑",
         ci_m6.原生平台(False, "aarch64") == "linux-arm64"
         and not ci_m6.交叉模拟态("linux-arm64", "aarch64")
         and ci_m6.交叉模拟态("linux-arm64", "x86_64")
         and ci_m6.原生平台(True, "AMD64") == "win-x64")

    # ⑥ E2E 段 M6 门禁三态（桩步骤·临时 cn/明细）
    with tempfile.TemporaryDirectory() as 临时:
        假cn = Path(临时) / "cn.exe"
        假cn.write_text("x", encoding="utf-8")
        明细 = Path(临时) / "e2e明细.json"
        日志 = io.StringIO()

        def 跑一段(计划):
            步骤们: dict = {"构建": {"rc": 0}}
            命令们: list = []

            def 假步骤(名, 命令, 超时秒):
                命令们.append((名, list(命令)))
                步骤们[名] = {"rc": 0}
                return True
            ci_m6.e2e段(步骤们, 假步骤, 假cn, 日志, "a" * 40, "ci/预验-x", 0, None,
                        计划, 是win=True, 平台="win-x64", 日志目录=Path(临时),
                        e2e并行=4, e2e超时秒=60, runner标识="selftest",
                        启动心跳=lambda 分支, 片=None: None, e2e明细路径=明细)
            return 步骤们, 命令们

        步们, 命令们 = 跑一段(dict(必跑))
        断言("#1221 E2E 段：必跑→命令带 --require-m6 且 M6平台保障 rc=0",
             any("--require-m6" in c for _, c in 命令们)
             and 步们.get("M6平台保障", {}).get("rc") == 0
             and 主模块.聚合绿(步们))
        步们2, _ = 跑一段(dict(转派, 处置="未跑-转派"))
        断言("#1221 E2E 段：预验×交叉模拟→M6平台保障 rc=-1 且轮红（fail-closed）",
             步们2.get("M6平台保障", {}).get("rc") == -1
             and not 主模块.聚合绿(步们2))
        步们3: dict = {"构建": {"rc": 0}}
        ci_m6.e2e段(步们3, lambda 名, 命令, 秒: (步们3.__setitem__(名, {"rc": 0}), True)[1],
                    假cn, 日志, "a" * 40, "develop", 0, None, dict(转派),
                    是win=True, 平台="win-x64", 日志目录=Path(临时), e2e并行=4,
                    e2e超时秒=60, runner标识="selftest",
                    启动心跳=lambda 分支, 片=None: None, e2e明细路径=明细)
        断言("#1221 E2E 段：develop×交叉模拟→转派 rc=0 且原因显式（不静默）",
             步们3.get("M6平台保障", {}).get("rc") == 0
             and "M6 未跑" in 步们3["M6平台保障"]["说明"]
             and 主模块.聚合绿(步们3))
        断言("#1221 E2E 段：未触发→不写 M6平台保障（纯文档轮零成本）",
             "M6平台保障" not in 跑一段(dict(豁免))[0])
    # ⑦ 轮记录回填：明细「M6」键→计划["执行"]（md5 随轮结果上 8300）
    with tempfile.TemporaryDirectory() as 临时:
        假cn = Path(临时) / "cn.exe"
        假cn.write_text("x", encoding="utf-8")
        明细 = Path(临时) / "e2e明细.json"
        明细.write_text('{"M6": {"状态": "通过", "fix_p_md5": "abc", '
                        '"fix_s_md5": "abc"}}', encoding="utf-8")
        计划 = dict(必跑)
        ci_m6.e2e段({}, lambda 名, 命令, 秒: True, 假cn, io.StringIO(), "a" * 40,
                    "develop", 0, None, 计划, 是win=True, 平台="win-x64",
                    日志目录=Path(临时), e2e并行=4, e2e超时秒=60, runner标识="selftest",
                    启动心跳=lambda 分支, 片=None: None, e2e明细路径=明细)
        断言("#1221 明细回填：run_e2e --json M6 键→计划[执行]（md5 可审计）",
             计划.get("执行", {}).get("fix_p_md5") == "abc")


def 自测(主模块) -> int:
    """--selftest 入口（断言集 523+637+657+660+728t+770+842+935+973+975+1297+1299+1369+1392+1401+1423+1424+1426+1428+1460+1463）——CI 式：逐条打印，任一红退出 1。"""
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
    _973断言(主模块, 断言)
    _975断言(主模块, 断言)
    _1297断言(断言)
    _1299断言(主模块, 断言)
    _1369断言(主模块, 断言)
    _1392断言(断言)
    _1401断言(断言)
    _1423断言(断言)
    _1424断言(主模块, 断言)
    _1428断言(断言)
    _1426断言(主模块, 断言)
    _1460断言(断言)
    _1463断言(主模块, 断言)
    _1468断言(主模块, 断言)
    _1221断言(主模块, 断言)
    _行数断言(断言)
    for 名, ok in 断言们:
        print("  [%s] %s" % ("OK" if ok else "×", 名))
    红 = sum(1 for _, ok in 断言们 if not ok)
    print("== ci_daemon 自测（523+637+657+660+728t+770+842+935+973+975+1297+1299+1369+1392+1401+1423+1424+1426+1428+1460+1463+1468+1221）：%d 断言，%s ==" % (len(断言们), "全绿" if 红 == 0 else "红 %d" % 红))
    return 0 if 红 == 0 else 1
