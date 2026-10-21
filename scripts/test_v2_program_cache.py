#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_v2_program_cache.py —— #1273 v2 程序级预编译缓存自动化回归（最终 1272 版）。

放置论证（为什么不是 E2E 单例）：
  tests/e2e/<编号>/主.cn + .expected 的模型=「一次编译 + 一次输出比对」，无法表达本
  回归需要的跨运行状态：同一工作区重复编译（命中原条目）、改被导入文件、切 -O/目标
  平台/-D/货舱特性、破坏缓存条目、替换编译器自身、构造缓存读写失败。拆成多个单例还
  会因工作目录不同而失去「同一缓存键/同一条目」语义；因此按 AGENTS §4 用独立回归
  脚本直驱 v2p 真实生产链路（宿主 build -> 闭包扫描 -> 命中/首建 -> 写 asm -> 字节
  比对），并由 gate_quick 写集条件步接入 L1。重型自举（v2p 编译 v2 全树）只在
  --自举 显式开启，默认不跑。

适配基线（2026-10-21 返工版 5d7071f61）：
  - 条目格式 CNV2PC02（7 行文本头 + 汇编文本 + 警告文本 + 尾换行；校验和覆盖分片
    长度行与载荷）；扩展名 .cnvpc；文件名=<入口/选项/宏/依赖签名 32hex>-<编译器
    身份 16hex>.cnvpc。
  - 控制面全部为环境变量：CN_NO_V2_PROGRAM_CACHE / CN_V2_PROGRAM_CACHE_STATS /
    CN_V2_PROGRAM_CACHE_VERIFY / CN_V2_PROGRAM_CACHE_DIR；--verbose 等命令旁路。
  - 统计七字段：命中/首建/失效重建/损坏回退/写入/写失败/验证不符（禁用态不输出
    统计行）。
  - 编译器身份并入条目文件名：不同编译器身份各自保留条目（不再原地覆盖）。
  - 键新增：拓扑闭包（路径/目录/模块/源摘要/包根/入口标记）、货舱 [货舱] 名称版本
    与原文摘要、[依赖] 名+版本、[特性] 并入宏集合。

覆盖矩阵（每项均真实运行 v2p）：
  ① 首建 miss + 写缓存 + 条目格式/摘要自校验 + 默认目录/CN_V2_PROGRAM_CACHE_DIR；
  ② 同输入二次 hit + asm 逐字节一致 + 警告重放 stderr 逐字节一致 + 条目不改写；
     VERIFY=1 命中仍完整重编译且 asm/stderr 一致；
  ③ 改被 import 文件（等长内容）-> 失效重建 -> 再热命中；touch 不失效（内容键）；
  ④ 改 -O 级别 / 目标平台 / -D 宏集合 -> 独立条目且回切仍命中；
  ⑤ 损坏五形态（魔数/头部截断/载荷字节/长度行/校验和字段）-> 显式告警 + 回退
     重编译 + 重建条目与基准逐字节一致 + 再热命中；
  ⑥ 编译器身份变化（追加字节）-> 新身份新条目（首建）且旧条目保留；切回原编译器
     直接命中；touch 不失效（内容身份）；
  ⑦ 验证通道：CN_V2_PROGRAM_CACHE_VERIFY=1 正常命中双一致；篡改 asm / 篡改 stderr
     的「校验和合法的错条目」均被 VERIFY 抓住（验证不符=1）并以重编译产物覆盖；
     CN_NO_V2_PROGRAM_CACHE=1 不读/不写/不修复缓存；--verbose 命令旁路；
  ⑧ 统计七字段逐场景精确计数（禁用态无统计行）；
  ⑨ 成功与失败路径均无 *.tmp* 残留，缓存目录仅含 .cnvpc 条目；
  ⑩ 读取失败（条目被目录占位）与写入失败（缓存目录被文件占位）-> 显式告警/计数，
     编译结果正确、不静默复用/不静默成功，修复后重建并命中；
  ⑪ 货舱键：[特性] 切换 -> 独立条目；[货舱] 名称/版本/原文注释变化 -> 失效重建；
     恢复旧配置 -> 命中旧条目（多配置条目并存）。

用法：
  python scripts/test_v2_program_cache.py [--cn target/cn] [--目标 linux-x86_64]
      [--工作根 target/cache_test_1273] [--自举] [--v2p <既有 v2p>]
  python scripts/test_v2_program_cache.py --自测   # 零 v2p 依赖自测（计数器/解析/退出码）
  前置：target/cn 已构建（gate_quick ② 增量构建后即满足）。
  退出码：0=全部断言通过；1=有失败；2=环境/构建失败。
"""
from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import time
import traceback

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from v2_program_cache_testlib import (  # noqa: E402
    仓库根, 默认目标, v2入口, 缓存目录相对, 环境统计, 环境禁用, 环境验证,
    统计行特征, 条目名正则, 计数器, 去统计, 摘要128, 是祖先,
    构建v2p, 建工作区, 运行v2p, 缓存目录, 缓存条目们, asm路径, 唯一条目,
    解析并校验条目, 重构条目, 简单源, 警告源, 货舱源, 写货舱, 建立基线,
)


def 用例_首建与格式(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("① 首建 miss + 写缓存 + CNV2PC02 格式/摘要自校验 + 目录/环境")
    区 = 建工作区(根 / "首建格式", 目标, 简单源())
    r1 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.断言(r1.returncode == 0, "冷编译 rc=0", (r1.stderr or "")[-200:])
    t.统计断言(r1, "冷编译=首建+写入", 首建=1, 写入=1)
    asm = asm路径(区, 目标)
    t.断言(asm.exists() and asm.stat().st_size > 0, "冷编译产物存在且非空",
           f"{asm.stat().st_size if asm.exists() else 0} B")
    冷产物 = asm.read_bytes() if asm.exists() else b""
    默认目录 = 区 / 缓存目录相对
    t.断言(默认目录.is_dir(), "默认缓存目录=target/cache/v2program", str(默认目录))
    条目 = 唯一条目(t, 区, "冷编译后恰有 1 个缓存条目")
    if 条目 is None:
        return
    t.断言(条目.parent == 默认目录.resolve(), "条目位于默认缓存目录", str(条目.parent))
    t.断言(条目名正则.fullmatch(条目.name) is not None,
           "条目名=签名32hex-身份16hex.cnvpc", 条目.name)
    解析 = 解析并校验条目(t, 条目, "首建条目")
    if 解析 is None:
        return
    t.断言(解析["汇编"] == 冷产物, "缓存汇编载荷=落盘 asm 逐字节一致",
           f"{len(解析['汇编'])} B")
    冷条目字节 = 条目.read_bytes()
    冷时间 = 条目.stat().st_mtime_ns

    asm.unlink()
    r2 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "同输入二次=命中", 命中=1)
    t.断言(asm.exists() and asm.read_bytes() == 冷产物,
           "命中重建 asm（产物被删后仍逐字节一致）")
    t.断言(条目.read_bytes() == 冷条目字节, "命中不改写条目文件")
    t.断言(条目.stat().st_mtime_ns == 冷时间, "命中不更新条目 mtime")
    t.断言(去统计(r1.stderr) == 去统计(r2.stderr),
           "冷/热 stderr（统计行外）逐字节一致")

    r3 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), {})
    t.无统计断言(r3, "未开统计环境变量=零统计输出")
    t.断言(asm.read_bytes() == 冷产物 and 条目.stat().st_mtime_ns == 冷时间,
           "无统计环境变量仍命中（产物一致且条目未改写）")

    覆盖区 = 建工作区(根 / "目录覆盖", 目标, 简单源())
    覆盖目录 = 覆盖区 / "custom-cache"
    覆盖环境 = dict(环境统计)
    覆盖环境["CN_V2_PROGRAM_CACHE_DIR"] = str(覆盖目录)
    o1 = 运行v2p(v2p, 覆盖区, "src/主.cn", 目标, (), 覆盖环境)
    t.统计断言(o1, "CN_V2_PROGRAM_CACHE_DIR 覆盖=首建+写入", 首建=1, 写入=1)
    t.断言(缓存目录(覆盖区, 覆盖目录).is_dir() and
           len(缓存条目们(覆盖区, 覆盖目录)) == 1,
           "覆盖目录出现唯一条目", str(覆盖目录))
    t.断言(not (覆盖区 / 缓存目录相对).exists(), "覆盖时不落默认缓存目录")
    o2 = 运行v2p(v2p, 覆盖区, "src/主.cn", 目标, (), 覆盖环境)
    t.统计断言(o2, "覆盖目录二次命中", 命中=1)


def 用例_警告重放(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("② 命中 + asm 一致 + 警告重放一致 + 条目不改写 + VERIFY 正常路径")
    区 = 建工作区(根 / "警告重放", 目标, 警告源())
    r1 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.断言(r1.returncode == 0, "带警告程序冷编译 rc=0", (r1.stderr or "")[-200:])
    t.统计断言(r1, "冷编译统计", 首建=1, 写入=1)
    t.断言((r1.stderr or "").count("[警告]") == 3, "冷编译 stderr 含 3 条语言警告",
           f"实际 {(r1.stderr or '').count('[警告]')} 条")
    冷体 = 去统计(r1.stderr)
    asm = asm路径(区, 目标)
    冷产物 = asm.read_bytes() if asm.exists() else b""
    条目 = 唯一条目(t, 区, "警告程序恰有 1 个条目")
    if 条目 is None:
        return
    解析 = 解析并校验条目(t, 条目, "警告条目")
    if 解析 is None:
        return
    t.断言(解析["汇编"] == 冷产物, "警告条目汇编载荷=落盘 asm")
    t.断言(解析["警告"].decode("utf-8", "replace").count("[警告]") == 3,
           "条目警告载荷含 3 条原文", f"{len(解析['警告'])} B")
    条目字节 = 条目.read_bytes()
    条目时间 = 条目.stat().st_mtime_ns

    r2 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "二次编译=命中", 命中=1)
    t.断言(asm.read_bytes() == 冷产物, "命中 asm 逐字节一致")
    t.断言(去统计(r2.stderr) == 冷体, "命中警告重放 stderr 逐字节一致")
    t.断言(条目.read_bytes() == 条目字节 and 条目.stat().st_mtime_ns == 条目时间,
           "命中不改写条目（字节/mtime 均不变）")

    验证环境 = dict(环境统计)
    验证环境.update(环境验证)
    r3 = 运行v2p(v2p, 区, "主.cn", 目标, (), 验证环境)
    t.统计断言(r3, "VERIFY 正常命中=命中+写回+验证不符0",
               命中=1, 写入=1, 验证不符=0)
    t.断言(asm.read_bytes() == 冷产物, "VERIFY 命中 asm=基线")
    t.断言(去统计(r3.stderr) == 冷体, "VERIFY 重编译 stderr=命中重放 stderr")
    t.断言("警告: [程序缓存] 验证不符" not in (r3.stderr or ""),
           "VERIFY 正常路径无验证不符告警")
    t.断言(条目.read_bytes() == 条目字节, "VERIFY 重编译结果=基线条目（未漂移）")


def 用例_导入失效(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("③ 改被 import 文件 -> 失效重建；touch 不失效（内容键）")
    区 = 建工作区(根 / "导入失效", 目标, 简单源("    返回 值 + 1;\n"))
    基线 = 建立基线(t, v2p, 区, "src/主.cn", 目标, "基线冷编译")
    if 基线 is None:
        return
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    条目 = 基线["条目"]
    基线条目 = 基线["条目字节"]
    基线名 = 条目.name

    计算 = 区 / "src/计算.cn"
    os.utime(计算, None)
    r2 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "仅 touch 导入文件=仍命中（内容键非 mtime）", 命中=1)
    t.断言(条目.read_bytes() == 基线条目, "touch 后条目字节不变")

    计算.write_text("公开:\n函数 加倍(整64 值) -> 整64 {\n    返回 值 + 2;\n}\n",
                    encoding="utf-8")
    r3 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r3, "等长内容修改=失效重建", 失效重建=1, 写入=1)
    改后产物 = asm.read_bytes()
    t.断言(改后产物 != 基线产物, "修改后产物确已变化（非空/非旧值）",
           f"{len(基线产物)}->{len(改后产物)} B")
    t.断言(缓存条目们(区) == [条目], "同签名条目原地覆盖（文件数不累积）",
           f"{[p.name for p in 缓存条目们(区)]}")
    t.断言(条目.name == 基线名 and 条目.read_bytes() != 基线条目,
           "条目名不变、键内容已更新")
    r4 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r4, "改后二次=命中", 命中=1)
    t.断言(asm.read_bytes() == 改后产物, "改后命中产物一致")

    计算.write_text("公开:\n函数 加倍(整64 值) -> 整64 {\n    返回 值 + 1;\n}\n",
                    encoding="utf-8")
    r5 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r5, "改回原内容=再次失效重建", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 基线产物, "改回后产物=基线")
    t.断言(条目.read_bytes() == 基线条目, "改回后条目字节=基线（确定性重建）")
    r6 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r6, "改回后热命中", 命中=1)


def 用例_选项与平台(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("④ -O 级别 / 目标平台 / -D 宏集合 -> 独立条目且回切命中")
    区 = 建工作区(根 / "选项平台", 目标, {
        "主.cn": "函数 主() -> 整32 { 返回 2 + 3 * 4; }\n",
    })
    asm = asm路径(区, 目标)
    r0 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r0, "O0 首建", 首建=1, 写入=1)
    o0 = asm.read_bytes()
    r1 = 运行v2p(v2p, 区, "主.cn", 目标, ("-O1",), 环境统计)
    t.统计断言(r1, "-O1 独立首建", 首建=1, 写入=1)
    o1 = asm.read_bytes()
    t.断言(len(缓存条目们(区)) == 2, "-O1 新增独立条目", f"{len(缓存条目们(区))} 个")
    r1b = 运行v2p(v2p, 区, "主.cn", 目标, ("-O1",), 环境统计)
    t.统计断言(r1b, "-O1 二次命中", 命中=1)
    t.断言(asm.read_bytes() == o1, "-O1 命中产物一致")
    r2 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "回切 -O0 命中（旧条目保留）", 命中=1)
    t.断言(asm.read_bytes() == o0, "回切 -O0 产物=基线")

    备用目标 = "linux-arm64" if 目标 != "linux-arm64" else "linux-x86_64"
    r3 = 运行v2p(v2p, 区, "主.cn", 备用目标, (), 环境统计)
    t.统计断言(r3, f"切目标 {备用目标}=独立首建", 首建=1, 写入=1)
    备用asm = asm路径(区, 备用目标)
    备用产物 = 备用asm.read_bytes()
    t.断言(备用asm.exists() and 备用产物 != o0, "备用目标产物存在且与主目标不同",
           f"{len(备用产物)} B")
    t.断言(len(缓存条目们(区)) == 3, "目标平台新增独立条目",
           f"{len(缓存条目们(区))} 个")
    r4 = 运行v2p(v2p, 区, "主.cn", 备用目标, (), 环境统计)
    t.统计断言(r4, f"备用目标 {备用目标} 二次命中", 命中=1)
    r5 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r5, "回切主目标命中", 命中=1)
    t.断言(asm.read_bytes() == o0, "回切主目标产物=基线")

    宏区 = 建工作区(根 / "宏集合", 目标, {
        "主.cn": (
            "函数 主() -> 整32 {\n"
            "    #如果定义 特性甲\n"
            "    返回 8;\n"
            "    #否则\n"
            "    返回 4;\n"
            "    #结束如果\n"
            "}\n"
        ),
    })
    宏asm = asm路径(宏区, 目标)
    m0 = 运行v2p(v2p, 宏区, "主.cn", 目标, (), 环境统计)
    t.统计断言(m0, "无 -D 首建", 首建=1, 写入=1)
    无宏产物 = 宏asm.read_bytes()
    m1 = 运行v2p(v2p, 宏区, "主.cn", 目标, ("-D", "特性甲"), 环境统计)
    t.统计断言(m1, "-D 宏独立首建", 首建=1, 写入=1)
    有宏产物 = 宏asm.read_bytes()
    t.断言(有宏产物 != 无宏产物, "-D 宏改变产物（宏集合入键与条目名）",
           f"{len(无宏产物)}->{len(有宏产物)} B")
    t.断言(len(缓存条目们(宏区)) == 2, "-D 宏新增独立条目",
           f"{len(缓存条目们(宏区))} 个")
    m2 = 运行v2p(v2p, 宏区, "主.cn", 目标, ("-D", "特性甲"), 环境统计)
    t.统计断言(m2, "-D 宏二次命中", 命中=1)
    m3 = 运行v2p(v2p, 宏区, "主.cn", 目标, (), 环境统计)
    t.统计断言(m3, "回切无宏命中", 命中=1)
    t.断言(宏asm.read_bytes() == 无宏产物, "回切无宏产物=基线")


def 用例_损坏形态(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑤ 损坏五形态 -> 显式告警 + 回退重编译 + 重建一致 + 再热命中")
    区 = 建工作区(根 / "损坏形态", 目标, {
        "主.cn": "函数 主() -> 整32 { 返回 2 + 3 * 4; }\n",
    })
    基线 = 建立基线(t, v2p, 区, "主.cn", 目标, "基线冷编译")
    if 基线 is None:
        return
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    条目 = 基线["条目"]
    基线条目 = 基线["条目字节"]
    解析 = 解析并校验条目(t, 条目, "基线条目")
    if 解析 is None:
        return
    段们 = 基线条目.split(b"\n", 7)
    头长 = len(基线条目) - len(段们[7])

    def 魔数(b: bytes) -> bytes:
        x = bytearray(b)
        x[0] = ord("X")
        return bytes(x)

    def 头部截断(b: bytes) -> bytes:
        return b"CNV2PC02\n"

    def 载荷字节(b: bytes) -> bytes:
        x = bytearray(b)
        for i in range(头长, 头长 + 解析["汇编长"]):
            if x[i] != 0x0A:
                x[i] ^= 1
                return bytes(x)
        return b

    def 长度行(b: bytes) -> bytes:
        新长 = str(int(段们[3]) + 1).encode()
        return (b"CNV2PC02\n" + 段们[1] + b"\n" + 段们[2] + b"\n" + 新长 + b"\n" +
                段们[4] + b"\n" + 段们[5] + b"\n" + 段们[6] + b"\n" + 段们[7])

    def 校验和字段(b: bytes) -> bytes:
        旧 = 段们[5]
        改 = bytearray(旧)
        改[0] = ord("0") if 旧[0:1] != b"0" else ord("1")
        return (b"CNV2PC02\n" + 段们[1] + b"\n" + 段们[2] + b"\n" + 段们[3] + b"\n" +
                段们[4] + b"\n" + bytes(改) + b"\n" + 段们[6] + b"\n" + 段们[7])

    形态们 = [
        ("魔数", 魔数, "魔数不符（格式版本变化）"),
        ("头部截断", 头部截断, "头部不完整"),
        ("载荷字节", 载荷字节, "载荷校验和不符"),
        ("长度行", 长度行, "载荷长度不符"),
        ("校验和字段", 校验和字段, "载荷校验和不符"),
    ]
    for 名, 破坏, 告警 in 形态们:
        新字节 = 破坏(基线条目)
        t.断言(新字节 != 基线条目, f"损坏-{名}=已实际改写条目")
        条目.write_bytes(新字节)
        r = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
        ok = t.统计断言(r, f"损坏-{名}=损坏回退+重建", 损坏回退=1, 写入=1)
        t.断言(r.returncode == 0 and ok, f"损坏-{名}=rc=0（回退后编译成功）",
               f"rc={r.returncode}")
        t.断言(f"缓存条目无效（{告警}）" in (r.stderr or ""),
               f"损坏-{名}=显式告警原文 [{告警}]", (r.stderr or "")[-180:])
        t.断言(条目.name in (r.stderr or ""), f"损坏-{名}=告警含条目名", 条目.name)
        t.断言(条目.read_bytes() == 基线条目, f"损坏-{名}=重建条目与基准逐字节一致")
        t.断言(asm.read_bytes() == 基线产物, f"损坏-{名}=重建产物=基线")
        r2 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
        t.统计断言(r2, f"损坏-{名}后热命中", 命中=1)
        t.断言(not list(条目.parent.glob("*.tmp*")), f"损坏-{名}处理无临时文件残留")


def 用例_编译器身份(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑥ 编译器身份变化 -> 新身份新条目且旧条目保留；切回直接命中")
    区 = 建工作区(根 / "编译器身份", 目标, {
        "主.cn": "函数 主() -> 整32 { 返回 2 + 3 * 4; }\n",
    })
    基线 = 建立基线(t, v2p, 区, "主.cn", 目标, "原编译器冷编译")
    if 基线 is None:
        return
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    原件 = 基线["条目"]
    原条目字节 = 基线["条目字节"]

    os.utime(v2p, None)
    r1 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r1, "touch 编译器=仍命中（身份按内容）", 命中=1)
    t.断言(原件.read_bytes() == 原条目字节, "touch 编译器后条目不变")

    变异 = 根 / "v2p_变体"
    shutil.copy2(v2p, 变异)
    with 变异.open("ab") as f:
        f.write(b"X")
    os.chmod(变异, 0o755)
    t.断言(变异.read_bytes() != pathlib.Path(v2p).read_bytes(), "变异编译器内容已改变")
    r2 = 运行v2p(变异, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "变异身份=首建+写入（新条目，非失效）", 首建=1, 写入=1)
    t.断言(asm.read_bytes() == 基线产物, "变异身份产物=基线（身份只入条目名/键）")
    条目们 = 缓存条目们(区)
    t.断言(len(条目们) == 2, "新旧编译器身份条目并存", f"{len(条目们)} 个")
    t.断言(原件.read_bytes() == 原条目字节, "旧身份条目字节未被改写")
    原前缀, 原身份 = 原件.stem.split("-", 1)
    变体们 = [p for p in 条目们 if p != 原件]
    if not t.断言(len(变体们) == 1, "变异身份条目可唯一定位", f"{[p.name for p in 变体们]}"):
        return
    变前缀, 变身份 = 变体们[0].stem.split("-", 1)
    t.断言(原前缀 == 变前缀, "同一输入键前缀一致", f"{原前缀} vs {变前缀}")
    t.断言(原身份 != 变身份, "编译器身份摘要不同", f"{原身份} vs {变身份}")
    r3 = 运行v2p(变异, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r3, "变异身份二次命中", 命中=1)
    r4 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r4, "切回原编译器=直接命中旧条目", 命中=1)
    t.断言(原件.read_bytes() == 原条目字节, "切回后原条目字节不变")
    t.断言(len(缓存条目们(区)) == 2, "切回后两条目仍并存", f"{len(缓存条目们(区))} 个")


def 用例_验证通道(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑦a 无缓存旁路 / 命令旁路：不读不写不修复缓存")
    区 = 建工作区(根 / "无缓存旁路", 目标, 简单源())
    基线 = 建立基线(t, v2p, 区, "src/主.cn", 目标, "冷编译")
    if 基线 is None:
        return
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    条目 = 基线["条目"]
    条目字节 = 基线["条目字节"]
    冷体 = 去统计(基线["统计结果"].stderr)
    r2 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "命中", 命中=1)

    禁用环境 = dict(环境统计)
    禁用环境.update(环境禁用)
    asm.unlink()
    r3 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 禁用环境)
    t.无统计断言(r3, "CN_NO_V2_PROGRAM_CACHE=1 无统计行（禁用不计入统计）")
    t.断言(asm.exists() and asm.read_bytes() == 基线产物,
           "无缓存产物=命中产物逐字节一致")
    t.断言((r3.stderr or "") == 冷体, "无缓存完整编译 stderr=命中重放 stderr")
    t.断言(条目.read_bytes() == 条目字节, "无缓存不写/不修复条目")

    损坏 = bytearray(条目字节)
    损坏[0] = ord("X")
    条目.write_bytes(bytes(损坏))
    r4 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 禁用环境)
    t.断言(r4.returncode == 0, "禁用态遇损坏条目仍 rc=0", f"rc={r4.returncode}")
    t.无统计断言(r4, "禁用态不检查损坏（无统计行）")
    t.断言("缓存条目无效" not in (r4.stderr or ""), "禁用态不读取/不告警损坏条目")
    t.断言(条目.read_bytes() == bytes(损坏), "禁用态不修复损坏条目")
    t.断言(asm.read_bytes() == 基线产物, "禁用态产物仍正确")
    条目.write_bytes(条目字节)

    新区 = 建工作区(根 / "无缓存首编", 目标, 简单源())
    新asm = asm路径(新区, 目标)
    n1 = 运行v2p(v2p, 新区, "src/主.cn", 目标, (), 禁用环境)
    t.无统计断言(n1, "无缓存首编无统计行")
    t.断言(not (新区 / 缓存目录相对).exists(), "无缓存首编不创建缓存目录")
    首编产物 = 新asm.read_bytes()
    n2 = 运行v2p(v2p, 新区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(n2, "随后开启缓存=首建", 首建=1, 写入=1)
    t.断言(新asm.read_bytes() == 首编产物, "开启缓存后产物=无缓存首编产物")
    n3 = 运行v2p(v2p, 新区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(n3, "随后热命中", 命中=1)

    v1 = 运行v2p(v2p, 新区, "src/主.cn", 目标, ("--verbose",), 环境统计)
    t.无统计断言(v1, "--verbose 命令旁路=无统计行")
    t.断言("[cga]" in (v1.stderr or ""), "--verbose 确有详细诊断输出")
    t.断言(新asm.read_bytes() == 首编产物, "--verbose 旁路产物=基线")
    t.断言(len(缓存条目们(新区)) == 1, "--verbose 不新增条目", f"{len(缓存条目们(新区))} 个")


def 用例_验证不符(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑦b VERIFY 判别力：校验和合法的错 asm / 错 stderr 均被抓住并覆盖")
    区 = 建工作区(根 / "验证不符asm", 目标, {
        "主.cn": "函数 主() -> 整32 { 返回 2 + 3 * 4; }\n",
    })
    基线 = 建立基线(t, v2p, 区, "主.cn", 目标, "asm 基线")
    if 基线 is None:
        return
    条目 = 基线["条目"]
    基线条目 = 基线["条目字节"]
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    解析 = 解析并校验条目(t, 条目, "asm 基线条目")
    if 解析 is None:
        return
    错汇编 = bytearray(解析["汇编"])
    位置 = next((i for i, b in enumerate(错汇编) if b != 0x0A), None)
    if not t.断言(位置 is not None, "asm 扰动点可定位"):
        return
    错汇编[位置] ^= 1
    条目.write_bytes(重构条目(解析, bytes(错汇编), 解析["警告"]))
    坏 = 解析并校验条目(t, 条目, "错 asm 条目（校验和合法）")
    t.断言(坏 is not None, "错 asm 条目结构/校验和合法（仅内容错）")
    验证环境 = dict(环境统计)
    验证环境.update(环境验证)
    r1 = 运行v2p(v2p, 区, "主.cn", 目标, (), 验证环境)
    t.统计断言(r1, "VERIFY 抓住错 asm=验证不符1+写回",
               命中=1, 写入=1, 验证不符=1)
    t.断言("验证不符（缓存产物与重编译产物不一致）" in (r1.stderr or ""),
           "错 asm=显式验证不符告警", (r1.stderr or "")[-220:])
    t.断言(asm.read_bytes() == 基线产物, "错 asm 被重编译产物覆盖（最终产物正确）")
    t.断言(条目.read_bytes() == 基线条目, "错 asm 条目被重写为基准")
    r2 = 运行v2p(v2p, 区, "主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "覆盖后正常命中", 命中=1)

    警区 = 建工作区(根 / "验证不符stderr", 目标, 警告源())
    警基线 = 建立基线(t, v2p, 警区, "主.cn", 目标, "stderr 基线")
    if 警基线 is None:
        return
    警条目 = 警基线["条目"]
    警条目字节 = 警基线["条目字节"]
    警解析 = 解析并校验条目(t, 警条目, "stderr 基线条目")
    if 警解析 is None:
        return
    错警告 = bytearray(警解析["警告"])
    警位置 = next((i for i, b in enumerate(错警告) if b not in (0x0A, 0x00)), None)
    if not t.断言(警位置 is not None, "stderr 扰动点可定位"):
        return
    错警告[警位置] ^= 1
    警条目.write_bytes(重构条目(警解析, 警解析["汇编"], bytes(错警告)))
    警坏 = 解析并校验条目(t, 警条目, "错 stderr 条目（校验和合法）")
    t.断言(警坏 is not None, "错 stderr 条目结构/校验和合法（仅内容错）")
    r3 = 运行v2p(v2p, 警区, "主.cn", 目标, (), 验证环境)
    t.统计断言(r3, "VERIFY 抓住错 stderr=验证不符1+写回",
               命中=1, 写入=1, 验证不符=1)
    t.断言("验证不符（缓存 stderr 与重编译stderr 不一致）" in (r3.stderr or ""),
           "错 stderr=显式验证不符告警", (r3.stderr or "")[-220:])
    t.断言(警条目.read_bytes() == 警条目字节, "错 stderr 条目被重写为基准")
    t.断言(asm路径(警区, 目标).read_bytes() == 警基线["产物"],
           "错 stderr 场景产物仍正确")


def 用例_统计字段(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑧ 统计七字段逐场景精确计数（禁用态无统计行）")
    区 = 建工作区(根 / "统计字段", 目标, 简单源())
    基线 = 建立基线(t, v2p, 区, "src/主.cn", 目标, "首建场景")
    if 基线 is None:
        return
    r2 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "命中：仅 命中=1", 命中=1)
    (区 / "src/计算.cn").write_text(
        "公开:\n函数 加倍(整64 值) -> 整64 {\n    返回 值 + 2;\n}\n", encoding="utf-8")
    r3 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r3, "失效：仅 失效重建=1 写入=1", 失效重建=1, 写入=1)
    条目 = 基线["条目"]
    原 = bytearray(条目.read_bytes())
    原[0] = ord("X")
    条目.write_bytes(bytes(原))
    r4 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r4, "损坏：仅 损坏回退=1 写入=1", 损坏回退=1, 写入=1)
    r5 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r5, "修复后：仅 命中=1", 命中=1)
    验证环境 = dict(环境统计)
    验证环境.update(环境验证)
    r6 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 验证环境)
    t.统计断言(r6, "VERIFY 正常：命中+写入+验证不符0",
               命中=1, 写入=1, 验证不符=0)

    写区 = 建工作区(根 / "统计写失败", 目标, 简单源())
    (写区 / "target/cache").mkdir(parents=True)
    (写区 / "target/cache/v2program").write_text("占位文件（阻断缓存目录）", encoding="utf-8")
    w1 = 运行v2p(v2p, 写区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(w1, "写失败：仅 首建=1 写失败=1", 首建=1, 写失败=1)
    (写区 / "target/cache/v2program").unlink()
    w2 = 运行v2p(v2p, 写区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(w2, "解除故障后：首建+写入", 首建=1, 写入=1)

    禁用区 = 建工作区(根 / "统计禁用", 目标, 简单源())
    禁用环境 = dict(环境统计)
    禁用环境.update(环境禁用)
    d1 = 运行v2p(v2p, 禁用区, "src/主.cn", 目标, (), 禁用环境)
    t.无统计断言(d1, "禁用：无统计行（七字段不出现禁用计数）")


def 用例_读写失败(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑩ 读取失败 / 写入失败 -> 显式告警，不静默复用/不静默成功")
    区 = 建工作区(根 / "条目被目录占位", 目标, 简单源())
    基线 = 建立基线(t, v2p, 区, "src/主.cn", 目标, "目录占位基线")
    if 基线 is None:
        return
    asm = 基线["asm"]
    基线产物 = 基线["产物"]
    条目 = 基线["条目"]
    条目.unlink()
    条目.mkdir()
    (条目 / "占位子文件").write_text("x", encoding="utf-8")
    r1 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.断言(r1.returncode == 0, "条目被目录占位=回退后 rc=0", f"rc={r1.returncode}")
    t.统计断言(r1, "条目被目录占位=读取失败+写入失败", 损坏回退=1, 写失败=1)
    t.断言("缓存条目无效（文件不可读）" in (r1.stderr or ""),
           "条目被目录占位=读取失败显式告警")
    t.断言("条目写入失败（编译产物不受影响）" in (r1.stderr or ""),
           "条目被目录占位=写入失败显式告警")
    t.断言(asm.read_bytes() == 基线产物, "条目被目录占位=回退产物=基线")
    t.断言(条目.is_dir() and (条目 / "占位子文件").exists(),
           "写入失败不删除/不替换占位目录（未静默改道）")
    t.断言(not list(条目.parent.glob("*.tmp*")), "rename 失败路径清理临时文件")
    (条目 / "占位子文件").unlink()
    条目.rmdir()
    r2 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r2, "清除占位后=首建+写入", 首建=1, 写入=1)
    t.断言(asm.read_bytes() == 基线产物, "清除占位后产物=基线")
    r3 = 运行v2p(v2p, 区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(r3, "清除占位后热命中", 命中=1)

    写区 = 建工作区(根 / "缓存目录被文件占位", 目标, 简单源())
    (写区 / "target/cache").mkdir(parents=True)
    占位 = 写区 / "target/cache/v2program"
    占位.write_text("占位文件（缓存目录不可创建）", encoding="utf-8")
    w1 = 运行v2p(v2p, 写区, "src/主.cn", 目标, (), 环境统计)
    t.断言(w1.returncode == 0, "缓存目录被文件占位=rc=0", f"rc={w1.returncode}")
    t.统计断言(w1, "缓存目录被文件占位=首建+写失败", 首建=1, 写失败=1)
    t.断言("条目写入失败（编译产物不受影响）" in (w1.stderr or ""),
           "缓存目录被文件占位=写入失败显式告警")
    写区asm = asm路径(写区, 目标)
    写区产物 = 写区asm.read_bytes()
    t.断言(len(写区产物) > 0, "写入失败仍产出正确 asm", f"{len(写区产物)} B")
    t.断言(占位.is_file(), "写入失败不破坏占位路径（未静默改道）")
    占位.unlink()
    w2 = 运行v2p(v2p, 写区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(w2, "解除故障后=首建+写入", 首建=1, 写入=1)
    t.断言(写区asm.read_bytes() == 写区产物,
           "解除故障后产物=写入失败场景产物（正确性未受影响）")
    w3 = 运行v2p(v2p, 写区, "src/主.cn", 目标, (), 环境统计)
    t.统计断言(w3, "解除故障后热命中", 命中=1)


def 用例_货舱键(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑪ 货舱键：[特性] 独立条目；[货舱] 名称/版本/原文变化失效；恢复命中旧条目")
    区 = 建工作区(根 / "货舱键", 目标, 货舱源())
    写货舱(区, "甲")
    入口 = str(区 / "主.cn")
    基 = 建立基线(t, v2p, 区, 入口, 目标, "货舱特性甲")
    if 基 is None:
        return
    asm = 基["asm"]
    甲产物 = 基["产物"]
    r2 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r2, "货舱特性甲二次命中", 命中=1)

    写货舱(区, "乙特性")
    r3 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r3, "货舱特性切换=独立首建", 首建=1, 写入=1)
    乙产物 = asm.read_bytes()
    t.断言(乙产物 != 甲产物, "特性切换改变产物", f"{len(甲产物)}->{len(乙产物)} B")
    t.断言(len(缓存条目们(区)) == 2, "特性切换新增独立条目",
           f"{len(缓存条目们(区))} 个")
    r4 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r4, "货舱特性乙二次命中", 命中=1)

    写货舱(区, "乙特性", "2.0")
    r5 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r5, "货舱版本变化=失效重建（同条目）", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 乙产物, "版本变化不改变产物")
    t.断言(len(缓存条目们(区)) == 2, "版本变化不新增条目（原地覆盖）",
           f"{len(缓存条目们(区))} 个")

    写货舱(区, "乙特性", "2.0", "# 仅注释变化\n")
    r6 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r6, "货舱原文注释变化=失效重建（原文摘要入键）",
               失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 乙产物, "注释变化不改变产物")

    写货舱(区, "甲", "1.0")
    r7 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计)
    t.统计断言(r7, "恢复旧配置=命中旧条目", 命中=1)
    t.断言(asm.read_bytes() == 甲产物, "恢复旧配置产物=甲基线")
    t.断言(len(缓存条目们(区)) == 2, "恢复后两条目并存",
           f"{len(缓存条目们(区))} 个")


def 用例_无临时残留(t: 计数器, 根: pathlib.Path) -> None:
    t.段("⑨ 无 *.tmp* 残留 + 缓存目录仅含 .cnvpc 条目")
    根 = 根.resolve()
    残留 = [str(p.relative_to(根)) for p in 根.rglob("*") if ".tmp" in p.name]
    t.断言(not 残留, "工作根下无任何 *.tmp* 残留", "、".join(残留[:5]) or "无")
    条目们 = list(根.rglob("*.cnvpc"))
    t.断言(len(条目们) >= 10, "至少检查 10 个缓存条目", f"{len(条目们)} 个")
    缓存目录们 = {p.parent for p in 条目们}
    非法 = []
    for 目录 in 缓存目录们:
        for p in 目录.iterdir():
            if p.is_file() and 条目名正则.fullmatch(p.name):
                continue
            非法.append(str(p.relative_to(根)))
    t.断言(not 非法, "缓存目录仅含 签名-身份.cnvpc 条目", "、".join(非法[:5]) or "无")


def 运行自举(t: 计数器, v2p, 目标: str, 根: pathlib.Path) -> None:
    t.段("可选重型：--自举（v2p 编译 v2 全树冷/热）")
    区 = 建工作区(根 / "自举", 目标, {})
    入口 = str(v2入口)
    开始 = time.monotonic()
    r1 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计, 超时秒=3600)
    冷秒 = time.monotonic() - 开始
    t.断言(r1.returncode == 0, "自举冷编译 rc=0", f"{冷秒:.1f}s")
    t.统计断言(r1, "自举冷=首建+写入", 首建=1, 写入=1)
    asm = asm路径(区, 目标)
    冷产物 = asm.read_bytes() if asm.exists() else b""
    t.断言(len(冷产物) > 0, "自举冷产物非空", f"{len(冷产物)} B")
    标记 = "cn_main PROC" if 目标 == "win-x64" else ".globl cn_main"
    t.断言(标记 in 冷产物.decode("utf-8", "replace"), "自举产物含 cn_main 入口",
           f"标记={标记}")
    开始 = time.monotonic()
    r2 = 运行v2p(v2p, 区, 入口, 目标, (), 环境统计, 超时秒=3600)
    热秒 = time.monotonic() - 开始
    t.统计断言(r2, "自举热=命中", 命中=1)
    t.断言(asm.read_bytes() == 冷产物, "自举冷/热产物逐字节一致")
    t.断言(去统计(r1.stderr) == 去统计(r2.stderr),
           "自举冷/热 stderr（统计行外）逐字节一致")
    print(f"  [数据] 自举冷={冷秒:.1f}s 热={热秒:.1f}s "
          f"加速={冷秒 / max(热秒, 1e-9):.2f}x")


def 汇总退出码(t: 计数器) -> int:
    """失败=1（显式非零）；全过=0。--自测 直测该判据（禁静默绿）。"""
    return 0 if t.失败 == 0 else 1


def 自测() -> int:
    """最小正反自测（零 v2p 依赖）：计数器/统计解析/无统计/摘要空串/退出码判据。"""
    t = 计数器()
    正 = t.断言(True, "自测·正样本")
    负 = t.断言(False, "自测·负样本（预期打印 FAIL）")
    合成 = subprocess.CompletedProcess(
        [], 0, "",
        "[警告] x[程序缓存] 命中=2 首建=0 失效重建=0 损坏回退=0 写入=0 "
        "写失败=0 验证不符=0\n")
    统计成 = t.统计断言(合成, "自测·统计七字段解析", 命中=2)
    去 = 去统计(合成.stderr) == "[警告] x"
    无统 = 统计行特征.search("[警告] x\n") is None
    空摘要 = 摘要128(b"") == (14695981039346656037, 1469598103934665603, 0)
    判据 = 汇总退出码(t) == 1 and 汇总退出码(计数器()) == 0
    ok = 正 and not 负 and 统计成 and 去 and 无统 and 空摘要 and 判据
    print(f"自测：{'PASS' if ok else 'FAIL'}（负样本被计数={not 负}·"
          f"空串摘要={空摘要}·退出码判据={判据}）")
    return 0 if ok else 1


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="v2 程序级预编译缓存自动化回归（#1273）")
    解析.add_argument("--cn", default=str(仓库根 / "target" /
                     ("cn.exe" if sys.platform.startswith("win") else "cn")))
    解析.add_argument("--目标", default=默认目标)
    解析.add_argument("--工作根", default=str(仓库根 / "target" / "cache_test_1273"))
    解析.add_argument("--v2p", default="",
                     help="既有 v2p（缺省=用 --cn 重建；给定则直接使用）")
    解析.add_argument("--自举", action="store_true",
                     help="额外跑 v2 全树自编译冷/热（数分钟·默认不跑）")
    解析.add_argument("--自测", action="store_true",
                     help="零 v2p 依赖的最小正反自测（计数器/解析/退出码判据）")
    参数 = 解析.parse_args()
    if 参数.自测:
        return 自测()
    cn = pathlib.Path(参数.cn).resolve()
    if not cn.exists():
        print(f"错误: 宿主编译器不存在: {cn}（先构建 target/cn）")
        return 2
    根 = pathlib.Path(参数.工作根).resolve()
    if 根 == 仓库根 or 是祖先(根, 仓库根):
        print(f"错误: --工作根 不能是仓库根或其祖先（防止误删）: {根}")
        return 2
    if 根.exists():
        shutil.rmtree(根)
    根.mkdir(parents=True)
    t = 计数器()
    print(f"== #1273 v2 程序级预编译缓存回归（目标={参数.目标}）==", flush=True)
    print(f"   宿主={cn}")
    开始 = time.monotonic()
    if 参数.v2p:
        v2p = pathlib.Path(参数.v2p).resolve()
        if not v2p.exists():
            print(f"错误: 指定 v2p 不存在: {v2p}")
            return 2
    else:
        v2p = 根 / "v2p"
        ok, 错误 = 构建v2p(cn, 参数.目标, v2p)
        if not ok:
            print(f"错误: v2p 构建失败:\n{错误}")
            return 2
    print(f"   v2p={v2p}（{v2p.stat().st_size} B·{time.monotonic() - 开始:.1f}s）",
          flush=True)
    try:
        用例_首建与格式(t, v2p, 参数.目标, 根)
        用例_警告重放(t, v2p, 参数.目标, 根)
        用例_导入失效(t, v2p, 参数.目标, 根)
        用例_选项与平台(t, v2p, 参数.目标, 根)
        用例_损坏形态(t, v2p, 参数.目标, 根)
        用例_编译器身份(t, v2p, 参数.目标, 根)
        用例_验证通道(t, v2p, 参数.目标, 根)
        用例_验证不符(t, v2p, 参数.目标, 根)
        用例_统计字段(t, v2p, 参数.目标, 根)
        用例_读写失败(t, v2p, 参数.目标, 根)
        用例_货舱键(t, v2p, 参数.目标, 根)
        用例_无临时残留(t, 根)
        if 参数.自举:
            运行自举(t, v2p, 参数.目标, 根)
    except Exception:
        t.失败 += 1
        t.失败项们.append("未捕获异常")
        print("  FAIL 未捕获异常：")
        traceback.print_exc()
    用时 = time.monotonic() - 开始
    print(f"\n== 汇总（{用时:.1f}s）==")
    print(f"  PASS {t.通过}  FAIL {t.失败}")
    if t.失败项们:
        print("  失败项：" + "、".join(t.失败项们))
    return 汇总退出码(t)


if __name__ == "__main__":
    sys.exit(主流程())



