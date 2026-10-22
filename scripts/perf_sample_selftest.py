#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""perf_sample_selftest.py —— perf_sample 内置自测（#1226/#1368）。"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import sys

from perf_sample_core import (
    _WSL路径存在,
    _校验callgrind结果,
    _校验产物,
    _是POSIX绝对路径,
    _wsl路径,
    反修饰,
    反修饰批大小,
    合并callgrind,
    退出成功,
    平台边界,
    负载失败,
    采样目录,
    浮点容差,
    环境错误,
    生成报告,
    缓存环境,
    探测主机,
    探测工具,
    构造负载,
    项目根,
    解析callgrind,
    解析perf报告,
    解析perf统计,
    跑callgrind,
    默认入口,
    解析目标,
    选择后端,
    造执行器,
    写结果JSON,
    callgrind注解,
)

def _自测夹具() -> dict:
    return {
        "callgrind_self": """
Ir                  file:function
--------------------------------------------------------------------------------
  600,000 ( 60.00%)  /src/a.cpp:foo()
  300,000 ( 30.00%)  /src/b.cpp:bar()
  100,000 ( 10.00%)  ???:_start
  1,000,000 (100.0%)  PROGRAM TOTALS
""",
        "callgrind_incl": """
Ir                  file:function
--------------------------------------------------------------------------------
  900,000 ( 90.00%)  /src/a.cpp:foo()
  300,000 ( 30.00%)  /src/b.cpp:bar()
  1,000,000 (100.0%)  PROGRAM TOTALS
""",
        "perf_self": """
# Overhead  Symbol
    45.00%  [.] foo()
    25.00%  [.] bar()
""",
        "perf_children": """
# Children      Self  Symbol
    80.00%    45.00%  foo()
    30.00%    25.00%  bar()
""",
        "perf_stat": "1000,,instructions,1.0,1\n12.5,msec,task-clock,1.0,1\n",
        "perf_no_pmu": "Error:\nNo supported events found.\n",
    }


def _自测外部(断言) -> None:
    主机 = 探测主机()
    用WSL = 主机["是Windows"]
    try:
        执行器 = 造执行器(用WSL)
    except 环境错误:
        断言("外部采样跳过：无 wsl", True)
        return
    工具 = 探测工具(执行器)
    if not (工具["valgrind"] and 工具["callgrind_annotate"]):
        断言("无 valgrind：后端降级 none", True)
        return
    if not 工具["python3"]:
        断言("无 python3：外部小负载跳过", True)
        return
    代码 = "x=0\nfor i in range(5000):\n    x+=i\nprint(x)\n"
    负载 = {"argv": ["python3", "-c", 代码], "cwd": 项目根, "环境": {},
            "显示": "python3 -c <固定小负载>", "输出": None}
    输出文件 = 采样目录 / f"selftest.callgrind.{os.getpid()}"
    try:
        结果 = 跑callgrind(执行器, 负载, 输出文件, 120)
        断言("固定小负载 callgrind rc=0", 结果.returncode == 0,
             (结果.stderr or "")[:200])
        if 结果.returncode != 0:
            return
        自身文本 = callgrind注解(执行器, 输出文件, False, 120)
        自身, 总数 = 解析callgrind(自身文本)
        断言("callgrind 解析出总指令数", 总数 > 0, str(总数))
        断言("callgrind 解析出热点条目", len(自身) > 0, str(len(自身)))
        热点 = 合并callgrind(自身, 自身, 执行器, 5)
        断言("callgrind 合并/排序非空", bool(热点) and 热点[0]["self"] > 0)
        if 主机["是Windows"] and _WSL路径存在(执行器, "/usr/bin/true"):
            try:
                参数代理 = argparse.Namespace(
                    load="v2-build", compiler="", proxy_compiler="/usr/bin/true",
                    cache="cold", entry="", output="", exec="", arg=[])
                负载代理 = 构造负载(参数代理, "win-x64",
                                  {"后端": "callgrind", "代理": True}, 执行器)
                断言("WSL POSIX 代理路径不 resolve",
                     负载代理["argv"][0] == "/usr/bin/true",
                     str(负载代理["argv"][0]))
            except (环境错误, 负载失败) as 异常:
                断言("WSL POSIX 代理路径不 resolve", False, str(异常))
        if 执行器.有工具("c++filt"):
            名称们 = [f"_Z3foo{i}" for i in range(反修饰批大小 + 25)]
            映射 = 反修饰(执行器, 名称们)
            断言("WSL c++filt 分批覆盖全部符号",
                 len(映射) == len(名称们), str(len(映射)))
    finally:
        if 输出文件.exists():
            输出文件.unlink()


def _自测解析(断言, 夹具: dict) -> None:
    自身, 总数 = 解析callgrind(夹具["callgrind_self"])
    断言("callgrind 总指令数解析", 总数 == 1_000_000, str(总数))
    断言("callgrind 条目数解析", len(自身) == 3, str(len(自身)))
    断言("callgrind 函数名/文件拆分",
        自身.get("/src/a.cpp:foo()", {}).get("函数") == "foo()")
    包含, _ = 解析callgrind(夹具["callgrind_incl"])
    合并 = 合并callgrind(自身, 包含, 执行器=None, 热点数=2)  # type: ignore[arg-type]
    断言("callgrind self/inclusive 合并",
        合并[0]["self"] == 60.0 and 合并[0]["包含"] == 90.0)
    断言("callgrind 按 self 降序", [x["self"] for x in 合并] == [60.0, 30.0])
    自身perf = 解析perf报告(夹具["perf_self"], False)
    包含perf = 解析perf报告(夹具["perf_children"], True)
    断言("perf self 解析", abs(自身perf.get("foo()", 0) - 45.0) < 浮点容差)
    断言("perf children 解析", abs(包含perf.get("foo()", 0) - 80.0) < 浮点容差)
    指令, 时钟 = 解析perf统计(夹具["perf_stat"])
    断言("perf stat instructions 解析", 指令 == [1000.0], str(指令))
    断言("perf stat task-clock 解析", 时钟 == [12.5], str(时钟))
    无PMU指令, _ = 解析perf统计(夹具["perf_no_pmu"])
    断言("无 PMU 输出不伪造指令数", 无PMU指令 == [])


def _自测后端(断言) -> None:
    主机win = {"系统": "windows", "架构": "amd64", "是Windows": True}
    主机linux = {"系统": "linux", "架构": "x86_64", "是Windows": False}
    主机arm = {"系统": "linux", "架构": "aarch64", "是Windows": False}
    工具无 = {"perf": False, "PMU": False, "valgrind": False,
            "callgrind_annotate": False, "c++filt": False, "python3": True}
    工具cg = dict(工具无, valgrind=True, callgrind_annotate=True)
    工具perf = dict(工具无, perf=True, PMU=True)
    try:
        解析目标("win-x64", 主机linux)
        断言("非 Windows 宿主 win-x64 拒绝", False)
    except 平台边界:
        断言("非 Windows 宿主 win-x64 拒绝", True)
    断言("auto win 无代理→none",
        选择后端("auto", "win-x64", 主机win, 工具cg, False)["后端"] == "none")
    断言("auto win 有代理→callgrind",
        选择后端("auto", "win-x64", 主机win, 工具cg, True)["后端"] == "callgrind")
    断言("auto linux 无 PMU→callgrind",
        选择后端("auto", "linux-x64", 主机linux, 工具cg, False)["后端"] == "callgrind")
    断言("auto linux 无工具→none",
        选择后端("auto", "linux-x64", 主机linux, 工具无, False)["后端"] == "none")
    断言("aarch64 宿主 arm64 可用",
        选择后端("auto", "linux-arm64", 主机arm, 工具perf, False)["后端"] == "perf")
    断言("none 允许跨架构墙钟",
        选择后端("none", "linux-arm64", 主机linux, 工具无, False)["后端"] == "none")

    def 应拒绝(名: str, 请求: str, 目标: str, 主机: dict, 工具: dict) -> None:
        try:
            选择后端(请求, 目标, 主机, 工具, False)
            断言(名, False, "未抛平台边界")
        except 平台边界:
            断言(名, True)

    应拒绝("auto x86 宿主 arm64 显式拒绝", "auto", "linux-arm64", 主机linux, 工具cg)
    应拒绝("显式 perf 跨架构拒绝", "perf", "linux-arm64", 主机linux, 工具perf)
    应拒绝("显式 callgrind 跨架构拒绝", "callgrind", "linux-arm64", 主机linux, 工具cg)
    应拒绝("反向跨架构 perf 拒绝", "perf", "linux-x64", 主机arm, 工具perf)
    应拒绝("显式 perf 无 PMU 拒绝", "perf", "linux-x64", 主机linux, 工具无)
    应拒绝("显式 callgrind 无工具拒绝", "callgrind", "linux-x64", 主机linux, 工具无)
    try:
        选择后端("callgrind", "win-x64", 主机win, 工具cg, False)
        断言("win 显式 callgrind 无代理拒绝", False)
    except 平台边界:
        断言("win 显式 callgrind 无代理拒绝", True)


def _自测执行(断言) -> None:
    执行器 = 造执行器(False)
    子 = 执行器.运行(
        [sys.executable, "-c",
         "import os; print(os.environ.get('CN_SELFTEST_ENV', ''))"],
        环境={"CN_SELFTEST_ENV": "注入成功"}, 超时=30)
    断言("原生子进程 env 注入", 子.returncode == 0
         and (子.stdout or "").strip() == "注入成功",
         f"rc={子.returncode} out={(子.stdout or '').strip()!r}")
    断言("WSL 路径转换",
        _wsl路径(pathlib.Path(r"C:\Users\x\a b.cn")) == "/mnt/c/Users/x/a b.cn")
    断言("POSIX 绝对路径判定", _是POSIX绝对路径("/mnt/c/x") and not _是POSIX绝对路径(r"C:\x"))
    冷环境 = 缓存环境("cold")
    断言("冷缓存环境",
        冷环境.get("CN_NO_AST_CACHE") == "1"
        and 冷环境.get("CN_NO_PROGRAM_CACHE") == "1"
        and 冷环境.get("CN_AST_CACHE_STATS") == "1")
    断言("热缓存环境含统计",
        缓存环境("hot").get("CN_AST_CACHE_STATS") == "1")
    try:
        _校验callgrind结果(0, {})
        断言("callgrind 零值抛错", False)
    except 负载失败:
        断言("callgrind 零值抛错", True)
    try:
        _校验产物({"输出": 采样目录 / "selftest_missing_output.bin"})
        断言("产物缺失防假绿", False)
    except 负载失败:
        断言("产物缺失防假绿", True)
    if 执行器.有工具("c++filt"):
        名称们 = [f"_Z3foo{i}" for i in range(反修饰批大小 + 25)]
        映射 = 反修饰(执行器, 名称们)
        断言("c++filt 分批覆盖全部符号", len(映射) == len(名称们), str(len(映射)))
    else:
        断言("c++filt 不可用：跳过分批", True)


def _自测构造(断言) -> None:
    执行器 = 造执行器(False)
    参数 = argparse.Namespace(load="exec", exec=sys.executable, arg=["-c", "pass"],
                             cache="asis", compiler=None, proxy_compiler=None,
                             entry=None, output=None)
    负载 = 构造负载(参数, "linux-x64", {"代理": False}, 执行器)
    断言("exec 负载构造",
        负载["argv"][0] == str(pathlib.Path(sys.executable).resolve()))
    参数v2 = argparse.Namespace(load="v2-build", exec="", arg=[],
                              cache="cold", compiler=sys.executable,
                              proxy_compiler="", entry="", output="")
    负载v2 = 构造负载(参数v2, "linux-x64", {"后端": "none", "代理": False}, 执行器)
    断言("v2-build 负载构造", 负载v2["argv"][1] == "build"
         and 负载v2["环境"].get("CN_NO_AST_CACHE") == "1")
    参数坏 = argparse.Namespace(load="v2-build", exec="", arg=[],
                              cache="cold", compiler=sys.executable,
                              proxy_compiler="", entry="",
                              output=str(默认入口))
    try:
        构造负载(参数坏, "linux-x64", {"后端": "none", "代理": False}, 执行器)
        断言("output 等于入口拒绝", False)
    except 环境错误:
        断言("output 等于入口拒绝", True)
    参数坏2 = argparse.Namespace(load="v2-build", exec="", arg=[],
                               cache="cold", compiler=sys.executable,
                               proxy_compiler="", entry="",
                               output=str(pathlib.Path(sys.executable).resolve()))
    try:
        构造负载(参数坏2, "linux-x64", {"后端": "none", "代理": False}, 执行器)
        断言("output 等于编译器拒绝", False)
    except 环境错误:
        断言("output 等于编译器拒绝", True)

    报告 = 生成报告(argparse.Namespace(load="exec", cache="asis", runs=1, top=5,
                                      timeout=30),
                     "linux-x64", {"后端": "none", "代理": False, "原因": "自测"},
                     负载, [{"轮号": 1, "总指令": 0, "task_clock": [], "热点": [],
                            "缓存统计": [], "墙钟秒": 0.01}])
    断言("none 报告含收益禁令", "不得用于宣称性能收益" in 报告)
    报告热 = 生成报告(argparse.Namespace(load="exec", cache="hot", runs=1, top=5,
                                        timeout=30),
                      "linux-x64", {"后端": "none", "代理": False, "原因": "自测"},
                      负载, [{"轮号": 1, "总指令": 0, "task_clock": [], "热点": [],
                             "缓存统计": ["[AST 缓存] 命中=3"], "墙钟秒": 0.01}])
    断言("热态报告含命中统计", "## 缓存统计" in 报告热 and "命中=3" in 报告热)
    夹具 = _自测夹具()
    自身cg, _ = 解析callgrind(夹具["callgrind_self"])
    包含cg, _ = 解析callgrind(夹具["callgrind_incl"])
    热点cg = 合并callgrind(自身cg, 包含cg, 执行器=None, 热点数=2)  # type: ignore[arg-type]
    报告cg = 生成报告(argparse.Namespace(load="exec", cache="cold", runs=1, top=5,
                                        timeout=30),
                      "linux-x64", {"后端": "callgrind", "代理": False, "原因": "自测"},
                      负载, [{"轮号": 1, "总指令": 1000, "task_clock": [],
                             "热点": 热点cg, "缓存统计": [], "墙钟秒": 0.1}])
    断言("callgrind 报告含 Ir 中位", "callgrind Ir 中位" in 报告cg)
    断言("callgrind 报告含热点表", "`foo()`" in 报告cg)
    断言("callgrind 报告 DSO 备注", "同名函数多文件条目" in 报告cg)

    json路径 = 采样目录 / "selftest_result.json"
    try:
        写结果JSON(str(json路径), "linux-x64",
                 {"后端": "none", "代理": False, "原因": "自测"}, 负载,
                 [{"轮号": 1, "总指令": 0, "热点": [], "墙钟秒": 0.01}])
        数据 = json.loads(json路径.read_text(encoding="utf-8"))
        断言("--json 落盘可解析", 数据.get("目标") == "linux-x64"
             and 数据["结果们"][0]["轮号"] == 1)
    finally:
        if json路径.exists():
            json路径.unlink()


def 自测() -> int:
    断言们: list[tuple[str, bool, str]] = []

    def 断言(名: str, 条件: bool, 详情: str = "") -> None:
        断言们.append((名, bool(条件), 详情))

    _自测解析(断言, _自测夹具())
    _自测后端(断言)
    _自测执行(断言)
    _自测构造(断言)
    _自测外部(断言)

    失败 = [x for x in 断言们 if not x[1]]
    for 名, 通过, 详情 in 断言们:
        print(f"[{'PASS' if 通过 else 'FAIL'}] {名}"
              + (f"  ← {详情}" if not 通过 and 详情 else ""))
    print(f"\n自测结果: {len(断言们) - len(失败)}/{len(断言们)} 通过")
    return 退出成功 if not 失败 else 1
