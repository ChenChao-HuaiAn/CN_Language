#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# stdlib 微基准族双测 runner（#367 ②·2026-10-09）
#   微基准族（tests/bench/b1~b9，b4 v2 挂起）：宿主 cn 与 v2 自举编译器（v2p）
#   双编译同测——双测铁律性能版（369 行同款纪律）。数字=优化验收基线
#   （#366/#368 对拍锚），报告落 target/bench/stdlib基线报告.md
#   （gitignore·数字入册=誊进 plans/032 规定的提交证据块）。
#   #1333（2026-10-25）：新增 b9 映射散列（200k 键 设置/获取/删除·O0/O2 两档），
#     口径=#1211 审查 P2-6 临时基准固化；报告补机器/构建类型/噪声带说明与样本明细。
#   用法: python scripts/bench_stdlib.py [--runs 3] [--only b1,b2] [--host-only]
#         --host-only = 仅宿主侧（win 无 as/g++ linux 链时开发自测；v2 列=挂，
#         默认双测行为不变·linux CI 照跑 v2）。
#         --selftest = 内置自测（纯本机零编译器·秒级：注册表/校验和正反例/
#         噪声带边界/中位极差/报告文本）。
#   判据（防假绿）：每用例编译 rc=0 + 校验行断言（终串长/两和相等/尾块和/错误码/
#   映射校验和），耗时数字只收集不判优——优化对拍由人按 plans/032 规程执行。
#   门禁边界（#1333）：本脚本不挂全量门禁硬阈值；如需门禁只校验 rc=0 与校验和
#   正确（噪声带 <2% 无结论，plans/032 §2.1）。

from __future__ import annotations   # P1-1：3.8 注解惰性化（list[str] 定义期不求值）

import argparse
import os
import pathlib
import platform
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
    # 368（2026-10-19）：扩容对照组——同负载「无预留（触发翻倍 realloc）」vs
    #   「预留(N)（零扩容）」，两组耗时差=扩容真实成本（实测锚定长期可复跑）。
    "b7_容器扩容对照": [r"B7 无预留: 耗时毫秒=(\d+)", r"B7 预留: 耗时毫秒=(\d+)",
                   r"B7 校验: 无预留验=3199984", r"B7 校验: 预留验=3199984"],
    # 1211（2026-10-21）：集合 哈希化——同负载增/查/漏检/删四段（哈希化前
    #   线性表 20000 键为 225~1063ms/段，哈希版毫秒级；数字只收集不判优）。
    "b8_集合哈希": [r"B8 添加: 耗时毫秒=(\d+)", r"B8 命中: 耗时毫秒=(\d+)",
                r"B8 漏检查: 耗时毫秒=(\d+)", r"B8 删除: 耗时毫秒=(\d+)",
                r"B8 校验: 命中=20000 漏中=0 删除=20000 余量=0"],
    # 1333（2026-10-25）：映射散列 200k 键 设置/获取/删除 三阶段——#1211 审查
    #   P2-6 临时基准固化入库。校验和 = Σ(3i+7), i∈[0,200000) = 60001100000。
    "b9_映射散列": [r"B9 设置: 耗时毫秒=(\d+)", r"B9 获取: 耗时毫秒=(\d+)",
                r"B9 删除: 耗时毫秒=(\d+)",
                r"B9 校验: 取和=60001100000 删除=200000 余量=0"],
    "b4_JSON解析": [r"B4 结果: 第(\d)", r"B4 轮: 耗时毫秒=(\d+)", r"B4 校验: 错误码=0",
                 r"B4 校验: 文档长=33009"],
    # 366（2026-10-13）：字符串构建器（b1 朴素连接同负载对照组·验收锚）
    "b5_字符串构建器": [r"B5 结果: 耗时毫秒=(\d+)", r"B5 校验: 终串长=262144"],
    # 728p（2026-10-14）：JSON 生成侧构建器改造（366 遗留生成侧同族·8192 段锚）
    "b6_JSON生成": [r"B6 结果: 第(\d)", r"B6 轮: 耗时毫秒=(\d+)", r"B6 校验: 文档长=286729",
                 r"B6 校验: 错误码=0", r"B6 校验: 生成长=286729",
                 r"B6 校验: 往返错误码=0", r"B6 校验: 往返相等=1"],
    # 728o（2026-10-14·366 遗留）：连接链融合（加号链形态·8192 片×32B 口径的
    #   10 倍放大量=81920 片；2/4/8/16 段链 + 循环累积形态）。确定性指标
    #   （总分配字节/次数·追踪分配器口径）不列判优正则=只收集入报告。
    "b6_连接链融合": [r"B6 段2 耗时毫秒=(\d+)", r"B6 段4 耗时毫秒=(\d+)",
                  r"B6 段8 耗时毫秒=(\d+)", r"B6 段16 耗时毫秒=(\d+)",
                  r"B6 累积 耗时毫秒=(\d+)", r"B6 校验: 和2=2621440",
                  r"B6 校验: 和4=2621440", r"B6 校验: 和8=2621440",
                  r"B6 校验: 和16=2621440", r"B6 校验: 累积长=262144"],
}

# 优化档注册表（#1333）：用例 → 档位列表；未登记=None（编译器默认：
#   宿主 -O2 / v2 -O0，保持既有数字口径不变）。b9 按 #1333 要求跑 O0/O2 两档，
#   两档同负载同校验和，只比较同一档内的数字（plans/032 §5.3 跨级别不比）。
用例优化档 = {"b9_映射散列": [0, 2]}

# 指标名（报告行顺序与 耗时毫秒 正则出现顺序一致）
指标名们 = {
    "b1_字符串连接": ["连接 8192×32B"],
    "b2_查表对比": ["查表法 2400 轮", "直读法 2400 轮"],
    "b3_容器扩容": ["向量<128B>×20 万追加"],
    "b7_容器扩容对照": ["128B×20 万 无预留", "128B×20 万 预留"],
    "b4_JSON解析": ["33KB 解析 第1轮", "第2轮", "第3轮"],
    "b5_字符串构建器": ["构建器 8192×32B"],
    "b6_JSON生成": ["8192 段生成 第1轮", "第2轮", "第3轮"],
    "b6_连接链融合": ["2 段链（不融合）", "4 段链（融合）",
                  "8 段链（融合）", "16 段链（融合）",
                  "循环累积（不融合）"],
    "b8_集合哈希": ["20000 键 添加", "20000 键 命中查",
                 "20000 键 漏检查", "20000 键 删除"],
    "b9_映射散列": ["200000 键 设置", "200000 键 获取", "200000 键 删除"],
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


运行时名们 = ["io_api", "mem_api", "mem_pool", "arena", "crash_handler",
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


def 编译v2用例(v2p: pathlib.Path, src: pathlib.Path, 产物: pathlib.Path,
               档旗: list[str] | None = None) -> None:
    """v2 侧编译：v2p 产 asm → as → g++ 借链 v2p.o+运行时（bench_self_host M2~M4 同款）。
    v2 驱动器按相对 cwd 读 stdlib/——工作目录做 stdlib 软链。
    档旗（#1333）：-O0/-O2 透传给 v2p（v2 CLI 扫描全部参数）；None=默认（v2 -O0）。"""
    工作 = 基目 / "v2work" / src.parent.name
    (工作 / "target").mkdir(parents=True, exist_ok=True)
    链接 = 工作 / "stdlib"
    if not 链接.exists():
        os.symlink(项目根 / "stdlib", 链接)
    asm = 工作 / "target" / "v2asm.s"   # v2 驱动器硬编码 cwd 下 target/v2asm.s
    if asm.exists():
        asm.unlink()
    r = 跑([str(v2p), str(src), "linux-x86_64"] + list(档旗 or []), cwd=工作)
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


def 解析输出(用例: str, 侧: str, 输出: str) -> list[int]:
    """校验断言（正则全 match）+ 提取耗时数字（b2 额外两和相等）。
    校验失败抛 RuntimeError（防假绿）；返回数字列表（顺序=正则声明顺序）。"""
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


def 一轮(exe: pathlib.Path, 用例: str, 侧: str, 档: int | None = None) -> list[int]:
    """编译（缓存产物）+运行一次→解析耗时数字们（校验断言失败即炸）。
    档（#1333）：None=编译器默认；0/2=显式 -O<档> 旗标（宿主/v2 同旗标）。"""
    产物 = 基目 / (f"{用例}.{侧}" + (f".O{档}" if 档 is not None else "")
                   + (".exe" if 侧 == "host" and os.name == "nt" else ""))
    src = 项目根 / "tests" / "bench" / 用例 / "主.cn"
    档旗 = [] if 档 is None else [f"-O{档}"]
    if 侧 == "host":
        # 385 随批：host 产物走本机目标链（win=win-x64 自带 ml64/link·原
        #   linux-x86_64 硬编码在 win 无 as/g++ 链必炸）；v2 侧 linux 链原样。
        目标 = "win-x64" if os.name == "nt" else "linux-x86_64"
        r = 跑([str(exe), "build", str(src), "--target", 目标, "--output", str(产物)]
              + 档旗)
        if r.returncode != 0:
            raise RuntimeError(f"[{用例}/{侧}] 编译失败: {(r.stderr or r.stdout).strip()[-300:]}")
    else:
        编译v2用例(exe, src, 产物, 档旗)
    r = 跑([str(产物)])
    if r.returncode != 0:
        raise RuntimeError(f"[{用例}/{侧}] 运行失败 rc={r.returncode}: "
                           f"{(r.stderr or r.stdout).strip()[-300:]}")
    return 解析输出(用例, 侧, r.stdout)


def 档显示(档: int | None) -> str:
    return "默认" if 档 is None else f"O{档}"


def 中位与极差(样本表: list[list[int]]) -> tuple[list[float], list[float]]:
    """样本表（每轮一行）→（每指标中位，每指标极差=最大-最小）。空表返回空。"""
    if not 样本表:
        return [], []
    中位 = [statistics.median(列) for 列 in zip(*样本表)]
    极差 = [max(列) - min(列) for 列 in zip(*样本表)]
    return 中位, 极差


def 判定差异(甲样本: list[float], 乙样本: list[float]) -> tuple[float, str]:
    """plans/032 §2.1 噪声带判定（只产结论措辞，不设门禁）。
    |中位差|<2% → 无结论；2%~4%（含端点）→ 须指令数复核；>4% → 超出噪声带
    （仍须指令数/等价性证据，不得只凭墙钟）。"""
    甲中 = statistics.median(甲样本)
    乙中 = statistics.median(乙样本)
    if 甲中 == 0:
        return 0.0, "基线中位为 0，无法判定"
    差异 = (乙中 - 甲中) / 甲中 * 100.0
    绝对值 = abs(差异)
    if 绝对值 < 2.0:
        return 差异, "无结论（噪声带 <2%）"
    if 绝对值 <= 4.0:
        return 差异, "须指令数复核（2%~4% 带）"
    return 差异, "超出噪声带（仍须指令数/等价性证据）"


def 探测构建类型() -> str:
    """从 CMakeCache 读宿主构建类型（Debug/Release）——报告口径必填（plans/032 §2.1）。"""
    for 缓存 in (项目根 / "target" / "build-ninja" / "CMakeCache.txt",
                 项目根 / "target" / "build-quick" / "CMakeCache.txt",
                 项目根 / "build" / "CMakeCache.txt"):
        if 缓存.exists():
            m = re.search(r"^CMAKE_BUILD_TYPE:STRING=(\S+)",
                          缓存.read_text(encoding="utf-8", errors="replace"), re.M)
            if m:
                return m.group(1)
    return "未知（非 CMake 构建或未配置）"


def 生成报告行(只跑: list[str], 结果: dict, 轮数: int,
               host: pathlib.Path, v2p: pathlib.Path | None) -> list[str]:
    """报告文本行（表 + 样本明细 + 口径说明）。selftest 直接检查文本。"""
    机器 = f"{platform.node()} {platform.machine()} {platform.processor()}".strip()
    平台 = ("win-x64" if os.name == "nt" else
            ("linux-" + platform.machine().lower()))
    行们 = [
        "# stdlib 微基准族基线报告（#367 ②·#1333 增 b9）",
        "",
        f"- 时间: {time.strftime('%Y-%m-%d %H:%M:%S')}（单机口径·跨机比较无意义）",
        f"- 机器: {机器}",
        f"- 平台: {平台} / 构建类型: {探测构建类型()}",
        f"- 宿主: {host}",
        f"- v2p: {v2p or '（挂起-only·未建）'}",
        f"- 轮数: {轮数}（取中位；样本明细见下）",
        "- 优化档: 未登记用例=编译器默认（宿主 -O2 / v2 -O0）；b9=显式 O0/O2 两档",
        "- 口径: 墙钟毫秒（plans/032 §2.1 最低可信度；win 无 PMU 只作参考，",
        "  收益判定须指令数；跨机不比绝对值）",
        "- 噪声带: 差异 <2% 无结论；2%~4% 须指令数复核；>4% 仍须指令数/等价性证据",
        "",
        "| 用例 | 指标 | 优化档 | 宿主(ms) | v2(ms) |",
        "|---|---|---|---:|---:|",
    ]
    for 用例 in 只跑:
        档们 = 用例优化档.get(用例, [None])
        for 档 in 档们:
            键 = (用例, "host", 档)
            if 键 not in 结果:
                continue
            中位, _ = 中位与极差(结果[键])
            v键 = (用例, "v2", 档)
            if v键 in 结果:
                v中位, _ = 中位与极差(结果[v键])
            else:
                v中位 = ["挂"] * len(中位)
            for i, 名 in enumerate(指标名们[用例]):
                v值 = v中位[i]
                行们.append(f"| {用例} | {名} | {档显示(档)} | {中位[i]:.0f} | "
                           f"{v值 if isinstance(v值, str) else f'{v值:.0f}'} |")
    # 样本明细（#1333：b9 至少 3 样本可复核）
    行们 += ["", "## 样本明细（墙钟毫秒·极差=最大-最小）", "",
             "| 用例 | 侧 | 优化档 | 样本 | 中位 | 极差 |", "|---|---|---|---|---:|---:|"]
    for 用例 in 只跑:
        for 档 in 用例优化档.get(用例, [None]):
            for 侧 in ("host", "v2"):
                键 = (用例, 侧, 档)
                if 键 not in 结果:
                    行们.append(f"| {用例} | {侧} | {档显示(档)} | （挂） | — | — |")
                    continue
                中位, 极差 = 中位与极差(结果[键])
                样本文本 = " / ".join(
                    "[" + ",".join(str(v) for v in 列) + "]" for 列 in zip(*结果[键]))
                行们.append(f"| {用例} | {侧} | {档显示(档)} | {样本文本} | "
                           f"{','.join(f'{v:.0f}' for v in 中位)} | "
                           f"{','.join(f'{v:.0f}' for v in 极差)} |")
    行们 += ["", "> 校验断言全过（终串长/两和相等/尾块和/错误码/映射校验和）·耗时只收数字",
            "> 不判优——优化对拍按 plans/032「性能优化验收规程」执行（改前改后交替≥3轮·",
            "> 指令数口径）。「挂」=v2 侧被在案缺陷阻断（见 v2挂起 注册表）或",
            "> --host-only 未跑 v2·单机口径。", ""]
    return 行们


def 自测() -> int:
    """纯本机零编译器自测（#1333）：注册表/校验和正反例/噪声带边界/中位极差/报告。"""
    print("== bench_stdlib 自测（#1333）==")
    失败: list[str] = []

    def 断言(名: str, 条件: bool, 详情: str = "") -> None:
        if 条件:
            print(f"  ✓ {名}")
        else:
            失败.append(名)
            print(f"  ✗ {名} {详情}")

    # 一、注册表完整性
    断言("b9_映射散列 已注册", "b9_映射散列" in 用例们)
    断言("b9 显式 O0/O2 两档", 用例优化档.get("b9_映射散列") == [0, 2],
         str(用例优化档.get("b9_映射散列")))
    断言("b9 校验正则四条", len(用例们["b9_映射散列"]) == 4,
         str(len(用例们["b9_映射散列"])))
    断言("指标名覆盖全部用例", set(指标名们) == set(用例们),
         f"缺={set(用例们) - set(指标名们)} 多={set(指标名们) - set(用例们)}")
    断言("b9 指标三条", len(指标名们["b9_映射散列"]) == 3)

    # 二、b9 校验和正反两态（防优化消除的确定值）
    正常输出 = ("B9 设置: 耗时毫秒=12\nB9 获取: 耗时毫秒=8\n"
              "B9 删除: 耗时毫秒=5\n"
              "B9 校验: 取和=60001100000 删除=200000 余量=0\n")
    数 = 解析输出("b9_映射散列", "host", 正常输出)
    断言("b9 正例：样本提取", 数 == [12, 8, 5], str(数))
    坏校验 = 正常输出.replace("60001100000", "60001100001")
    try:
        解析输出("b9_映射散列", "host", 坏校验)
        断言("b9 反例：校验和错必拒", False, "未抛错")
    except RuntimeError:
        断言("b9 反例：校验和错必拒", True)
    缺行 = "B9 设置: 耗时毫秒=1\nB9 获取: 耗时毫秒=2\n"
    try:
        解析输出("b9_映射散列", "host", 缺行)
        断言("b9 反例：缺校验行必拒", False, "未抛错")
    except RuntimeError:
        断言("b9 反例：缺校验行必拒", True)

    # 三、b2 两和不等的负例（既有校验逻辑不退化）
    坏b2 = ("B2 查表: 耗时毫秒=1\nB2 直读: 耗时毫秒=2\n"
           "B2 校验: 查表和=10\nB2 校验: 直读和=11\n")
    try:
        解析输出("b2_查表对比", "host", 坏b2)
        断言("b2 反例：两和不相等必拒", False, "未抛错")
    except RuntimeError:
        断言("b2 反例：两和不相等必拒", True)

    # 四、噪声带判定边界（plans/032 §2.1：<2% 无结论；2%~4% 须复核；>4% 超出）
    差1, 结论1 = 判定差异([100.0, 100.0, 100.0], [101.0, 101.0, 101.0])
    断言("噪声带 1% → 无结论", "无结论" in 结论1, f"{差1:.2f}% {结论1}")
    差2, 结论2 = 判定差异([100.0, 100.0, 100.0], [102.0, 102.0, 102.0])
    断言("噪声带 2% → 须复核", "须指令数复核" in 结论2, f"{差2:.2f}% {结论2}")
    差3, 结论3 = 判定差异([100.0, 100.0, 100.0], [104.0, 104.0, 104.0])
    断言("噪声带 4% → 须复核（端点）", "须指令数复核" in 结论3, f"{差3:.2f}% {结论3}")
    差4, 结论4 = 判定差异([100.0, 100.0, 100.0], [105.0, 105.0, 105.0])
    断言("噪声带 5% → 超出", "超出噪声带" in 结论4, f"{差4:.2f}% {结论4}")
    差5, 结论5 = 判定差异([100.0, 100.0, 100.0], [98.0, 98.0, 98.0])
    断言("噪声带 -2% → 须复核", "须指令数复核" in 结论5, f"{差5:.2f}% {结论5}")

    # 五、中位/极差
    中位, 极差 = 中位与极差([[10, 20, 30], [10, 10, 10], [10, 30, 20]])
    断言("中位计算", [round(v, 1) for v in 中位] == [10.0, 20.0, 20.0], str(中位))
    断言("极差计算", 极差 == [0, 20, 20], str(极差))

    # 六、报告文本（b9 两档 + 机器/构建类型/噪声带说明）
    假结果 = {
        ("b9_映射散列", "host", 0): [[10, 20, 30], [11, 21, 31], [12, 22, 32]],
        ("b9_映射散列", "host", 2): [[5, 9, 7], [6, 10, 8], [7, 11, 9]],
        ("b9_映射散列", "v2", 0): [[20, 40, 60], [21, 41, 61], [22, 42, 62]],
        ("b9_映射散列", "v2", 2): [[15, 30, 45], [16, 31, 46], [17, 32, 47]],
    }
    文本 = "\n".join(生成报告行(["b9_映射散列"], 假结果, 3,
                            pathlib.Path("target/cn.exe"), pathlib.Path("v2p_bench")))
    断言("报告含 b9 用例行", "| b9_映射散列 |" in 文本)
    断言("报告含 O0/O2 两档行", "| O0 |" in 文本 and "| O2 |" in 文本)
    断言("报告含构建类型", "构建类型" in 文本)
    断言("报告含噪声带说明", "噪声带" in 文本)
    断言("报告含样本明细", "样本明细" in 文本 and "极差" in 文本)
    断言("报告含 b9 校验口径", "60001100000" not in 文本 or "映射校验和" in 文本)

    总 = len(失败)
    print(f"\n自测结果: {'全绿' if 总 == 0 else f'{总} 项失败'}")
    return 0 if 总 == 0 else 1


def 主程序() -> int:
    解析 = argparse.ArgumentParser(description="stdlib 微基准族双测 runner（#367）")
    解析.add_argument("--runs", type=int, default=3, help="每侧运行次数（默认 3·取中位）")
    解析.add_argument("--only", default="", help="只跑指定用例（逗号分隔·默认全部）")
    解析.add_argument("--host-only", action="store_true",
                      help="仅宿主侧（win 无 as/g++ linux 链时开发自测；v2 列=挂）")
    解析.add_argument("--selftest", action="store_true",
                      help="内置自测（纯本机零编译器·秒级）")
    参数 = 解析.parse_args()

    if 参数.selftest:
        return 自测()

    只跑 = [s for s in 参数.only.split(",") if s] or list(用例们)
    for 名 in 只跑:
        if 名 not in 用例们:
            print(f"错误: 未知用例 {名}（合法: {'/'.join(用例们)}）")
            return 2

    host = 确保编译器("host")
    # 385 随批：only 全为挂起用例（如 b4=#362）时 v2 侧不参与——免强制现建 v2p
    #   （win 本机现建走 linux 目标链无 as·该缺口随 386 bench 门禁候选处理）
    # --host-only（#1211 返工）：win 无 as/g++ linux 链时的显式 host-only 自测通道；
    #   默认行为不变（linux CI 双测照跑）。
    v2p = (None if 参数.host_only else
           (确保编译器("v2") if any(n not in v2挂起 for n in 只跑) else None))
    print(f"宿主={host}\n  v2p={v2p or '（挂起-only/未建）'}\n  轮数={参数.runs}"
          f"{'（host-only）' if 参数.host_only else ''}\n")

    结果 = {}   # (用例, 侧, 档) → 样本表（每轮一行的数字列表）
    for 用例 in 只跑:
        侧面们 = ["host"] if (参数.host_only or 用例 in v2挂起) else ["host", "v2"]
        if 用例 in v2挂起:
            print(f"[黄] {用例}/v2 挂起——{v2挂起[用例]}（host-only 收口·清偿后恢复 dual）")
        for 侧, exe in [(s, host if s == "host" else v2p) for s in 侧面们]:
            for 档 in 用例优化档.get(用例, [None]):
                表 = []
                for _ in range(参数.runs):
                    表.append(一轮(exe, 用例, 侧, 档))
                结果[(用例, 侧, 档)] = 表
                中位, 极差 = 中位与极差(表)
                print(f"[{用例}/{侧}/{档显示(档)}] 样本={表} "
                      f"中位={[round(v, 1) for v in 中位]} 极差={极差}")

    # 报告
    基目.mkdir(parents=True, exist_ok=True)
    报告 = 基目 / "stdlib基线报告.md"
    行们 = 生成报告行(只跑, 结果, 参数.runs, host, v2p)
    报告.write_text("\n".join(行们), encoding="utf-8")
    print(f"\n报告: {报告}")
    return 0


if __name__ == "__main__":
    sys.exit(主程序())
