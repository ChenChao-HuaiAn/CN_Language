#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""perf_sample.py —— CN 编译器热点常态化采样统一入口（#1226）。

定位（与 #367 基准设施的分工）：
  · bench_self_host.py  = 端到端时间基线（M1/M2/M5·自举全树墙钟/CPU）。
  · bench_stdlib.py     = 生成代码/运行时微基准（b1~b8 墙钟·双编译）。
  · 本工具              = **函数级归因**（谁在消耗指令/self%/inclusive%），
                          不替代上两者，也不设置性能门禁阈值。

后端：
  · linux（x86_64/aarch64）：优先 perf（perf stat 指令数 + perf record 热点）；
    PMU 不可用时自动降级 callgrind。
  · win：原生 PE 无 perf/valgrind 面——默认只给墙钟降级报告；热点归因走
    WSL callgrind 代理（--proxy-compiler 指向同源 linux-x64 构建）。
  · linux-arm64：本机 aarch64 原生 perf；x86_64 宿主 qemu 只能给墙钟/相对时间，
    指令级归因=诚实边界（脚本显式拒绝，退出码 3）。

用法示例：
  python scripts/perf_sample.py --target linux-x64 --load v2-build --runs 3
  python scripts/perf_sample.py --target win-x64 --load v2-build --runs 3
  python scripts/perf_sample.py --target linux-x64 --load v2-build \
      --cache hot --runs 3
  python scripts/perf_sample.py --load exec --exec target/bench/b4_JSON解析.host
  python scripts/perf_sample.py --selftest

本脚本只读采样/生成报告到 target/perf/（gitignore），不改部署、不建 cron。
实现拆分：本文件=CLI/主流程；perf_sample_core.py=采样/解析/报告；
perf_sample_selftest.py=内置自测（三文件均 <1000 行）。
"""

from __future__ import annotations

import argparse
import os
import pathlib
import sys

from perf_sample_core import (
    合法后端,
    合法缓存,
    合法目标,
    合法负载,
    平台边界,
    执行器,
    探测主机,
    探测工具,
    构造负载,
    环境错误,
    生成报告,
    解析目标,
    跑一轮,
    负载失败,
    采样目录,
    选择后端,
    造执行器,
    写结果JSON,
    默认热点数,
    默认轮数,
    默认超时秒,
    退出成功,
    退出用法,
    退出边界,
    退出负载,
)
from perf_sample_selftest import 自测


# ============ 主程序 ============

def 建解析器() -> argparse.ArgumentParser:
    解析 = argparse.ArgumentParser(
        description="CN 编译器热点采样统一入口（#1226）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例：python scripts/perf_sample.py --target linux-x64 --load v2-build --runs 3")
    解析.add_argument("--target", choices=合法目标, default="auto",
                      help="目标平台（auto=按主机探测）")
    解析.add_argument("--backend", choices=合法后端, default="auto",
                      help="采样后端（auto=perf 优先·无 PMU 降级 callgrind）")
    解析.add_argument("--load", choices=合法负载, default="v2-build",
                      help="负载：v2-build=编译 v2 全树；exec=运行指定可执行文件")
    解析.add_argument("--compiler", default="", help="宿主编译器路径（默认 target/cn(.exe)）")
    解析.add_argument("--proxy-compiler", default="",
                      help="win 热点归因的 WSL 可执行 linux-x64 编译器（同源构建）")
    解析.add_argument("--entry", default="", help="v2-build 入口（默认 CN语言编译器v2/主.cn）")
    解析.add_argument("--output", default="", help="v2-build 产物路径（默认 target/perf/perf_load.*）")
    解析.add_argument("--exec", default="", help="--load exec 的可执行文件")
    解析.add_argument("--arg", action="append", default=[], help="--load exec 的参数（可重复）")
    解析.add_argument("--cache", choices=合法缓存, default="cold",
                      help="缓存口径：cold=禁用 AST/程序缓存；hot=先预热再测；asis=原样")
    解析.add_argument("--runs", type=int, default=默认轮数, help=f"采样轮数（默认 {默认轮数}）")
    解析.add_argument("--top", type=int, default=默认热点数, help=f"热点条数（默认 {默认热点数}）")
    解析.add_argument("--timeout", type=int, default=默认超时秒, help="单命令超时秒")
    解析.add_argument("--wsl-distro", default=os.environ.get("CN_WSL_DISTRO", ""),
                      help="WSL 发行版（默认用系统默认发行版）")
    解析.add_argument("--report", default=str(采样目录 / "采样报告.md"),
                      help="报告输出路径（默认 target/perf/采样报告.md）")
    解析.add_argument("--json", default="", help="另存 JSON 结果路径")
    解析.add_argument("--selftest", action="store_true", help="内置自测（固定小负载+解析+降级路径）")
    return 解析


def 主程序() -> int:
    参数 = 建解析器().parse_args()
    if 参数.selftest:
        return 自测()
    if 参数.runs < 1:
        print("错误: --runs 必须 >=1", file=sys.stderr)
        return 退出用法
    if 参数.top < 1:
        print("错误: --top 必须 >=1", file=sys.stderr)
        return 退出用法
    try:
        主机 = 探测主机()
        目标 = 解析目标(参数.target, 主机)
        执行器, 工具 = 准备执行面(主机, 目标, 参数)
        有代理 = bool(参数.proxy_compiler)
        后端信息 = 选择后端(参数.backend, 目标, 主机, 工具, 有代理)
        if 执行器.用WSL and 后端信息["后端"] != "callgrind" and 目标 == "win-x64":
            执行器 = 造执行器(False)
        负载 = 构造负载(参数, 目标, 后端信息, 执行器)
        if 参数.cache == "hot":
            print("[预热] 热缓存口径先跑一轮不计入样本...", flush=True)
            预热执行(执行器, 负载, 参数.timeout)
        结果们 = []
        for 轮号 in range(1, 参数.runs + 1):
            print(f"[{轮号}/{参数.runs}] {后端信息['后端']} 采样中...", flush=True)
            结果们.append(跑一轮(执行器, 后端信息, 负载, 参数, 轮号))
        报告 = 生成报告(参数, 目标, 后端信息, 负载, 结果们)
        报告路径 = pathlib.Path(参数.report)
        报告路径.parent.mkdir(parents=True, exist_ok=True)
        报告路径.write_text(报告, encoding="utf-8")
        if 参数.json:
            写结果JSON(参数.json, 目标, 后端信息, 负载, 结果们)
        print(报告)
        print(f"\n报告: {报告路径}")
        return 退出成功
    except 平台边界 as 异常:
        print(f"[平台边界] {异常}", file=sys.stderr)
        return 退出边界
    except 环境错误 as 异常:
        print(f"[环境错误] {异常}", file=sys.stderr)
        return 退出用法
    except 负载失败 as 异常:
        print(f"[负载失败] {异常}", file=sys.stderr)
        return 退出负载


def 准备执行面(主机: dict, 目标: str, 参数) -> tuple[执行器, dict]:
    """Windows 先探测 WSL；linux 目标必须走 WSL，win 目标无 WSL 时退回原生墙钟。"""
    if not 主机["是Windows"]:
        执行器 = 造执行器(False)
        return 执行器, (探测工具(执行器) if 参数.backend != "none" else _空工具())
    try:
        wsl执行器 = 造执行器(True, 参数.wsl_distro)
        if 目标 == "linux-x64" and not wsl执行器.有工具("sh"):
            raise 环境错误("linux-x64 目标在 Windows 上需要可用的 WSL（sh 不可达）")
        工具 = 探测工具(wsl执行器) if 参数.backend != "none" else _空工具()
        return wsl执行器, 工具
    except 环境错误:
        if 目标 == "linux-x64":
            raise
        return 造执行器(False), _空工具()


def _空工具() -> dict:
    return {"perf": False, "PMU": False, "valgrind": False,
            "callgrind_annotate": False, "c++filt": False, "python3": False}


def 预热执行(执行器: 执行器, 负载: dict, 超时: int) -> None:
    结果 = 执行器.运行(list(负载["argv"]), cwd=负载["cwd"],
                       环境=负载["环境"], 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"预热失败 rc={结果.returncode}: {(结果.stderr or '').strip()[:300]}")


if __name__ == "__main__":
    sys.exit(主程序())


if __name__ == "__main__":
    sys.exit(主程序())
