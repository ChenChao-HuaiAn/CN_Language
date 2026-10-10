#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# stdlib 微基准族双测 runner（#367 ②·2026-10-09）
#   四用例（tests/bench/b1~b4）：宿主 cn 与 v2 自举编译器（v2p）双编译同测——
#   双测铁律性能版（369 行同款纪律）。数字=优化验收基线（#366/#368 对拍锚），
#   报告落 target/bench/stdlib基线报告.md（gitignore·数字入册=誊进 plans/004）。
#   用法: python scripts/bench_stdlib.py [--runs 3] [--only b1,b2]
#   判据（防假绿）：每用例编译 rc=0 + 校验行断言（终串长/两和相等/尾块和/错误码），
#   耗时数字只收集不判优——优化对拍由人按 plans/004 规程执行。

import argparse
import os
import pathlib
import shutil
import re
import statistics
import subprocess
import sys
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

项目根 = pathlib.Path(__file__).resolve().parent.parent
基目 = 项目根 / "target" / "bench"

# 用例注册表：名 →（校验断言·stdout 正则全 match 集）
# v2挂起：v2 侧编译被在案缺陷阻断的用例（669 先例=host-only 收口·清偿后恢复 dual）
v2挂起 = {"b4_JSON解析": "#362（类含类类型字段布局崩·「成员字段偏移未知」7 错——669 同判）"}

用例们 = {
    "b1_字符串连接": [r"B1 结果: 耗时毫秒=(\d+)", r"B1 校验: 终串长=262144"],
    "b2_查表对比": [r"B2 查表: 耗时毫秒=(\d+)", r"B2 直读: 耗时毫秒=(\d+)",
                 r"B2 校验: 查表和=(\d+)", r"B2 校验: 直读和=(\d+)"],
    "b3_容器扩容": [r"B3 结果: 耗时毫秒=(\d+)", r"B3 校验: 尾块和=3199984"],
    "b4_JSON解析": [r"B4 结果: 第(\d)", r"B4 轮: 耗时毫秒=(\d+)", r"B4 校验: 错误码=0",
                 r"B4 校验: 文档长=33009"],
}


def 跑(命令: list, cwd=None) -> subprocess.CompletedProcess:
    return subprocess.run(命令, cwd=str(cwd or 项目根), capture_output=True,
                          text=True, encoding="utf-8", errors="replace")


def 确保编译器(侧: str) -> pathlib.Path:
    """宿主=target/cn；v2=v2p（无则用宿主 build v2 全树现建·bench_self_host M1 同款）"""
    if 侧 == "host":
        # 385 随批：win 产物带 .exe 后缀（ninja 链接 cn.exe）——候选探测与
        #   bench_self_host 同构（target/cn → target/cn.exe），linux 原样。
        exe = next((p for p in (项目根 / "target" / n for n in ("cn", "cn.exe"))
                    if p.exists()), None)
        if exe is None:
            print("错误: 未找到宿主编译器 target/cn（先 gate_quick 或 cmake --build）")
            sys.exit(2)
        return exe
    exe = 基目 / "v2p_bench"
    if exe.exists():
        return exe
    print("[v2p] 首次现建（宿主 build v2 全树·分钟级）...")
    host = 确保编译器("host")
    r = 跑([str(host), "build", str(项目根 / "CN语言编译器v2" / "主.cn"),
            "--target", "linux-x86_64", "--output", str(exe)])
    if r.returncode != 0 or not exe.exists():
        print("错误: v2p 现建失败:", (r.stderr or r.stdout).strip()[-300:])
        sys.exit(2)
    return exe


运行时名们 = ["io_api", "mem_api", "arena", "crash_handler",
            "intern_api", "runtime", "string_api", "i128_api",
            "math_api", "input_api", "file_api", "time_api", "system_api"]


def 准备运行时objs() -> list[pathlib.Path]:
    """运行时 .o 现场编译（bench_self_host 准备运行时objs 同款·不计时）"""
    objs = []
    obj目录 = 基目 / "v2work" / "runtime_objs"
    obj目录.mkdir(parents=True, exist_ok=True)
    cxx = shutil.which("g++")
    for 名 in 运行时名们:
        obj = obj目录 / f"{名}.o"
        src = 项目根 / "src" / "runtime" / f"{名}.cpp"
        if not src.exists():
            print(f"错误: 缺少运行时源文件: {src}")
            sys.exit(1)
        if not obj.exists():
            r = 跑([cxx, "-c", "-std=c++17", "-fno-exceptions", "-fno-rtti",
                    "-DCNRT_LINUX_MAIN", "-Isrc", "-O2", "-o", str(obj), str(src)])
            if r.returncode != 0:
                print(f"错误: 运行时 {名}.o 编译失败: {(r.stderr or r.stdout).strip()[:200]}")
                sys.exit(1)
        objs.append(obj)
    return objs


def 编译v2用例(v2p: pathlib.Path, src: pathlib.Path, 产物: pathlib.Path) -> None:
    """v2 侧编译：v2p 产 asm → as → g++ 借链 v2p.o+运行时（bench_self_host M2~M4 同款）。
    v2 驱动器按相对 cwd 读 stdlib/——工作目录做 stdlib 软链。"""
    工作 = 基目 / "v2work" / src.parent.name
    (工作 / "target").mkdir(parents=True, exist_ok=True)
    链接 = 工作 / "stdlib"
    if not 链接.exists():
        os.symlink(项目根 / "stdlib", 链接)
    asm = 工作 / "target" / "v2asm.s"   # v2 驱动器硬编码 cwd 下 target/v2asm.s
    if asm.exists():
        asm.unlink()
    r = 跑([str(v2p), str(src), "linux-x86_64"], cwd=工作)
    if r.returncode != 0 or not asm.exists():
        pathlib.Path("/tmp/v2fail.log").write_text(
            f"rc={r.returncode}\nCWD={工作}\nSTDERR:\n{r.stderr}\nSTDOUT:\n{r.stdout}",
            encoding="utf-8")
        raise RuntimeError(f"[v2] 编译失败: {(r.stderr or r.stdout).strip()[-300:]}（全量=/tmp/v2fail.log）")
    obj = 工作 / "use.o"
    r = 跑(["as", "-o", str(obj), str(asm)])
    if r.returncode != 0:
        raise RuntimeError(f"[v2] as 失败: {(r.stderr or r.stdout).strip()[-300:]}")
    v2pobj = 基目 / "v2p_bench.o"
    if not v2pobj.exists():
        raise RuntimeError("[v2] 缺借链 v2p_bench.o（v2p 现建产物应在）")
    r = 跑(["g++", "-no-pie", "-Wl,-z,muldefs", "-o", str(产物), str(obj), str(v2pobj)]
           + [str(o) for o in 准备运行时objs()])
    if r.returncode != 0:
        raise RuntimeError(f"[v2] 链接失败: {(r.stderr or r.stdout).strip()[-300:]}")


def 一轮(exe: pathlib.Path, 用例: str, 侧: str) -> list[int]:
    """编译（缓存产物）+运行一次→解析耗时数字们（校验断言失败即炸）"""
    产物 = 基目 / (f"{用例}.{侧}" + (".exe" if 侧 == "host" and os.name == "nt" else ""))
    src = 项目根 / "tests" / "bench" / 用例 / "主.cn"
    if 侧 == "host":
        # 385 随批：host 产物走本机目标链（win=win-x64 自带 ml64/link·原
        #   linux-x86_64 硬编码在 win 无 as/g++ 链必炸）；v2 侧 linux 链原样。
        目标 = "win-x64" if os.name == "nt" else "linux-x86_64"
        r = 跑([str(exe), "build", str(src), "--target", 目标, "--output", str(产物)])
        if r.returncode != 0:
            raise RuntimeError(f"[{用例}/{侧}] 编译失败: {(r.stderr or r.stdout).strip()[-300:]}")
    else:
        编译v2用例(exe, src, 产物)
    r = 跑([str(产物)])
    if r.returncode != 0:
        raise RuntimeError(f"[{用例}/{侧}] 运行失败 rc={r.returncode}: "
                           f"{(r.stderr or r.stdout).strip()[-300:]}")
    输出 = r.stdout
    模式们 = 用例们[用例]
    匹配们 = [re.search(p, 输出) for p in 模式们]
    缺 = [p for p, m in zip(模式们, 匹配们) if not m]
    if 缺:
        raise RuntimeError(f"[{用例}/{侧}] 校验断言未中: {缺}\n输出:\n{输出[:500]}")
    # b2 校验两和相等
    if 用例 == "b2_查表对比":
        if 匹配们[2].group(1) != 匹配们[3].group(1):
            raise RuntimeError(f"[{用例}/{侧}] 两法校验和不 "
                               f"等: {匹配们[2].group(1)} vs {匹配们[3].group(1)}")
    # 耗时数字=含「耗时毫秒」模式的 findall 平铺（b4 三轮同名前缀·search 会漏）
    数字们: list[int] = []
    for p, m in zip(模式们, 匹配们):
        if "耗时毫秒" in p:
            数字们 += [int(x) for x in re.findall(p, 输出)]
    if not 数字们:
        raise RuntimeError(f"[{用例}/{侧}] 未收集到耗时数字")
    return 数字们


def 主程序() -> int:
    解析 = argparse.ArgumentParser(description="stdlib 微基准族双测 runner（#367）")
    解析.add_argument("--runs", type=int, default=3, help="每侧运行次数（默认 3·取中位）")
    解析.add_argument("--only", default="", help="只跑指定用例（逗号分隔·默认全部）")
    参数 = 解析.parse_args()

    只跑 = [s for s in 参数.only.split(",") if s] or list(用例们)
    for 名 in 只跑:
        if 名 not in 用例们:
            print(f"错误: 未知用例 {名}（合法: {'/'.join(用例们)}）")
            return 2

    host = 确保编译器("host")
    # 385 随批：only 全为挂起用例（如 b4=#362）时 v2 侧不参与——免强制现建 v2p
    #   （win 本机现建走 linux 目标链无 as·该缺口随 386 bench 门禁候选处理）
    v2p = 确保编译器("v2") if any(n not in v2挂起 for n in 只跑) else None
    print(f"宿主={host}\n  v2p={v2p or '（挂起-only·未建）'}\n  轮数={参数.runs}\n")

    结果 = {}   # (用例, 侧) → 数字列表们
    for 用例 in 只跑:
        侧面们 = ["host"] + ([] if 用例 not in v2挂起 else []) + (
            ["v2"] if 用例 not in v2挂起 else [])
        if 用例 in v2挂起:
            print(f"[黄] {用例}/v2 挂起——{v2挂起[用例]}（host-only 收口·清偿后恢复 dual）")
        for 侧, exe in [(s, host if s == "host" else v2p) for s in 侧面们]:
            表 = []
            for _ in range(参数.runs):
                表.append(一轮(exe, 用例, 侧))
            结果[(用例, 侧)] = 表
            中位 = [statistics.median(col) for col in zip(*表)]
            print(f"[{用例}/{侧}] 样本={表} 中位={[round(v, 1) for v in 中位]}")

    # 报告
    基目.mkdir(parents=True, exist_ok=True)
    报告 = 基目 / "stdlib基线报告.md"
    行们 = [
        "# stdlib 微基准族基线报告（#367 ②）",
        "",
        f"- 时间: {time.strftime('%Y-%m-%d %H:%M:%S')}（单机口径·跨机比较无意义）",
        f"- 宿主: {host}",
        f"- v2p: {v2p or '（挂起-only·未建）'}",
        f"- 轮数: {参数.runs}（取中位）",
        "",
        "| 用例 | 指标 | 宿主(ms) | v2(ms) |",
        "|---|---|---:|---:|",
    ]
    for 用例 in 只跑:
        指标名们 = {"b1_字符串连接": ["连接 8192×32B"],
                "b2_查表对比": ["查表法 2400 轮", "直读法 2400 轮"],
                "b3_容器扩容": ["向量<128B>×20 万追加"],
                "b4_JSON解析": ["33KB 解析 第1轮", "第2轮", "第3轮"]}[用例]
        h = [statistics.median(c) for c in zip(*结果[(用例, "host")])]
        if (用例, "v2") in 结果:
            v = [statistics.median(c) for c in zip(*结果[(用例, "v2")])]
        else:
            v = ["挂"] * len(指标名们)
        for i, 名 in enumerate(指标名们):
            行们.append(f"| {用例} | {名} | {h[i]:.0f} | {v[i] if isinstance(v[i], str) else f'{v[i]:.0f}'} |")
    行们 += ["", "> 校验断言全过（终串长/两和相等/尾块和/错误码）·耗时只收数字不判优——",
            "> 「挂」=v2 侧被在案缺陷阻断（见 v2挂起 注册表·清偿后恢复 dual）·单机口径",
            "> 优化对拍按 plans/004「优化验收规程」执行（改前改后同机同负载）。", ""]
    报告.write_text("\n".join(行们), encoding="utf-8")
    print(f"\n报告: {报告}")
    return 0


if __name__ == "__main__":
    sys.exit(主程序())
