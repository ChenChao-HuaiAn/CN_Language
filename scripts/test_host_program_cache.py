#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_host_program_cache.py —— #1431 宿主程序级预编译缓存自动化回归（七组场景）。

放置论证（为什么不是 E2E 单例）：
  tests/e2e/<编号>/主.cn+.expected 的模型=「一次编译 + 一次输出比对」，无法表达
  「同一工作区重复编译命中同一条目 / 改被导入文件 / 切选项/宏/目标 / 破坏条目 /
  替换编译器自身 / VERIFY 伪造条目」等跨运行状态；按 AGENTS §4 用独立回归脚本直驱
  宿主 cn.exe 真实生产链路（闭包扫描→缓存命中/首建→写汇编），并由 gate_quick 写集
  条件步接入 L1（写集触缓存实现面才跑·云端全量兜底）。

七组场景（每组均真实编译·产物逐字节对照·统计七字段精确计数）：
  ① 条目身份：入口路径/编译选项（-O/--debug/--发布/目标平台）/条目格式
     （CNPGMC01·52 字节头+载荷三层/FNV 自校验/32hex.cnpg 命名）/默认目录/
     CN_PROGRAM_CACHE_DIR 覆盖/命中不改写条目；
  ② 内容键失效：改被导入文件（等长）→失效重建；改入口（等长）→失效重建；
     touch 不失效（内容键非 mtime）；改回原内容→确定性重建且条目字节回基线；
  ③ 宏集合：-D 宏 →独立条目且产物变化；货舱 [特性] 并入宏集合 →独立条目；
     [依赖] 名清单入键（未使用的依赖声明也失效）；回切命中旧条目；
  ④ 损坏五形态：截断/乱码/空/元数据不符/版本不符 →显式告警+回退重编译+
     重建条目与基准逐字节一致+再热命中；无 *.tmp* 残留；
  ⑤ 编译器身份：touch 编译器不失效（内容身份）；追加字节的编译器副本→失效
     重建（同条目名·键已变）；切回原编译器→再次重建且条目回基线；
  ⑥ VERIFY 抓伪造：正常命中双一致（写入=0）；校验和合法的伪造汇编/伪造警告
     均被抓住（验证不符=1+显式错误+以重编译产物覆盖）；CN_NO_PROGRAM_CACHE
     禁用态不读不写不修复；
  ⑦ 警告重放：冷编译三条语义警告随条目存储；命中 stderr（统计行外）逐字节
     重放；条目不改写；--json 通道命中 stdout 逐字节重放。

用法：
  python scripts/test_host_program_cache.py [--cn target/cn.exe] [--目标 win-x64]
      [--工作根 target/cache_test_1431]
  python scripts/test_host_program_cache.py --自测   # 零编译器依赖（解析/重构/计数器）
退出码：0=全部断言通过；1=有失败；2=环境失败。
"""
from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import sys
import time
import traceback

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from host_program_cache_testlib import (  # noqa: E402
    仓库根, 默认目标, 环境统计, 环境禁用, 环境验证, 条目名正则, 计数器, 去统计,
    是祖先, 建工作区, 运行cn, 缓存目录, 缓存条目们, asm路径, 唯一条目,
    解析条目, 重构条目, 简单源, 警告源, 宏源, 写货舱, 建立基线, 无临时残留,
)


def 条目未改写(t: 计数器, 条目, 字节: bytes, 时间: int, 名称: str) -> None:
    t.断言(条目.read_bytes() == 字节, 名称 + "：条目字节未改写")
    t.断言(条目.stat().st_mtime_ns == 时间, 名称 + "：条目 mtime 未更新")


# ==================== ① 条目身份 ====================

def 用例_条目身份(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("① 条目身份：入口路径/编译选项/条目格式/默认目录与覆盖")
    区 = 建工作区(根 / "条目身份", 简单源())
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    条目字节, 条目时间 = 基线["条目字节"], 条目.stat().st_mtime_ns
    t.断言(条目.parent == 缓存目录(区), "条目位于默认隔离目录", str(条目.parent))
    t.断言(条目名正则.fullmatch(条目.name) is not None, "条目名=32hex.cnpg", 条目.name)
    解析 = 解析条目(t, 条目, "基线条目")
    if 解析 is None:
        return
    t.断言(解析["汇编"] == 产物, "缓存汇编载荷=落盘 asm 逐字节一致",
           "%d B" % len(产物))
    冷体 = 去统计(基线["统计结果"].stderr)

    asm.unlink()
    r2 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r2, "同输入二次=命中", 命中=1)
    t.断言(asm.exists() and asm.read_bytes() == 产物, "命中重建 asm 逐字节一致")
    条目未改写(t, 条目, 条目字节, 条目时间, "命中")
    t.断言(去统计(r2.stderr) == 冷体, "冷/热 stderr（统计行外）逐字节一致")

    覆盖区 = 建工作区(根 / "目录覆盖", 简单源())
    覆盖目录 = 覆盖区 / "custom-cache"
    覆盖环境 = dict(环境统计)
    覆盖环境["CN_PROGRAM_CACHE_DIR"] = str(覆盖目录)
    o1 = 运行cn(cn, 覆盖区, "entry.cn", 目标, (), 覆盖环境)
    t.统计断言(o1, "CN_PROGRAM_CACHE_DIR 覆盖=首建+写入", 首建=1, 写入=1)
    t.断言(len(缓存条目们(覆盖区, 覆盖目录)) == 1, "覆盖目录出现唯一条目")
    t.断言(not (覆盖区 / "target" / "cache" / "program").exists(),
           "覆盖时不落默认缓存目录")
    o2 = 运行cn(cn, 覆盖区, "entry.cn", 目标, (), 覆盖环境)
    t.统计断言(o2, "覆盖目录二次命中", 命中=1)

    默区 = 建工作区(根 / "default_dir", 简单源())
    (默区 / "stdlib").mkdir()
    # 相对 --stdlib：项目根=路径首段（cwd），缓存目录=./target/cache/program——
    #   避免绝对路径注入对工作区目录编码的依赖（默认目录判定面本身不变）。
    d1 = 运行cn(cn, 默区, "entry.cn", 目标, ("--stdlib", "./stdlib"),
               环境统计, 目录隔离=False)
    t.统计断言(d1, "默认目录（--stdlib 假根）=首建+写入", 首建=1, 写入=1)
    t.断言(len(缓存条目们(默区)) == 1, "默认目录=<项目根>/target/cache/program",
           str(缓存目录(默区)))
    d2 = 运行cn(cn, 默区, "entry.cn", 目标, ("--stdlib", "./stdlib"),
               环境统计, 目录隔离=False)
    t.统计断言(d2, "默认目录二次命中", 命中=1)

    o1级 = 运行cn(cn, 区, "entry.cn", 目标, ("-O1",), 环境统计)
    t.统计断言(o1级, "-O1=独立首建", 首建=1, 写入=1)
    t.断言(len(缓存条目们(区)) == 2, "-O1 新增独立条目",
           "%d 个" % len(缓存条目们(区)))
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, ("-O1",), 环境统计),
               "-O1 二次命中", 命中=1)
    调试 = 运行cn(cn, 区, "entry.cn", 目标, ("--debug",), 环境统计)
    t.统计断言(调试, "--debug=独立首建", 首建=1, 写入=1)
    发布 = 运行cn(cn, 区, "entry.cn", 目标, ("--发布",), 环境统计)
    t.统计断言(发布, "--发布=独立首建", 首建=1, 写入=1)
    t.断言(len(缓存条目们(区)) == 4, "选项组合各自独立条目",
           "%d 个" % len(缓存条目们(区)))
    备用目标 = "linux-x86_64" if 目标 != "linux-x86_64" else "linux-arm64"
    r目标 = 运行cn(cn, 区, "entry.cn", 备用目标, (), 环境统计)
    t.统计断言(r目标, "切目标 %s=独立首建" % 备用目标, 首建=1, 写入=1)
    备用产物 = asm路径(区, 备用目标, "entry.cn")
    备用字节 = 备用产物.read_bytes() if 备用产物.exists() else b""
    t.断言(len(备用字节) > 0 and 备用字节 != 产物,
           "备用目标产物存在且与主目标不同", "%d B" % len(备用字节))
    t.断言(len(缓存条目们(区)) == 5, "目标平台新增独立条目",
           "%d 个" % len(缓存条目们(区)))
    回切 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(回切, "回切主目标默认选项=命中（旧条目保留）", 命中=1)
    t.断言(asm.read_bytes() == 产物, "回切产物=基线")

    复制 = 区 / "entry2.cn"
    复制.write_bytes((区 / "entry.cn").read_bytes())
    r复制 = 运行cn(cn, 区, "entry2.cn", 目标, (), 环境统计)
    t.统计断言(r复制, "同内容不同入口路径=独立首建（路径入条目身份）",
             首建=1, 写入=1)
    t.断言(len(缓存条目们(区)) == 6, "入口路径新增独立条目",
           "%d 个" % len(缓存条目们(区)))
    t.统计断言(运行cn(cn, 区, "entry2.cn", 目标, (), 环境统计),
               "新入口二次命中", 命中=1)


# ==================== ② 内容键失效 ====================

def 用例_内容键失效(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("② 内容键失效：改导入/改入口/等长内容/touch 不失效")
    区 = 建工作区(根 / "内容键失效", 简单源("    返回 值 + 1;\n"))
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    基线条目, 基线名 = 基线["条目字节"], 条目.name
    计算 = 区 / "计算.cn"

    os.utime(计算, None)
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "仅 touch 导入文件=仍命中（内容键非 mtime）", 命中=1)
    t.断言(条目.read_bytes() == 基线条目, "touch 后条目字节不变")

    计算.write_text("公开:\n函数 加倍(整64 值) -> 整64 {\n    返回 值 + 2;\n}\n",
                    encoding="utf-8")
    r改 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r改, "等长修改导入文件=失效重建", 失效重建=1, 写入=1)
    改后产物 = asm.read_bytes()
    t.断言(改后产物 != 产物, "修改后产物确已变化",
           "%d->%d B" % (len(产物), len(改后产物)))
    t.断言(缓存条目们(区) == [条目], "同签名条目原地覆盖（文件数不累积）")
    t.断言(条目.name == 基线名 and 条目.read_bytes() != 基线条目,
           "条目名不变、键内容已更新")
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "改后二次=命中", 命中=1)
    t.断言(asm.read_bytes() == 改后产物, "改后命中产物一致")

    计算.write_text("公开:\n函数 加倍(整64 值) -> 整64 {\n    返回 值 + 1;\n}\n",
                    encoding="utf-8")
    r回 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r回, "改回原内容=再次失效重建", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 产物, "改回后产物=基线")
    t.断言(条目.read_bytes() == 基线条目, "改回后条目字节=基线（确定性重建）")
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "改回后热命中", 命中=1)

    入口 = 区 / "entry.cn"
    入口.write_text(入口.read_text(encoding="utf-8").replace("加倍(7)", "加倍(8)"),
                    encoding="utf-8")
    r入 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r入, "等长修改入口文件=失效重建", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() != 产物, "入口修改后产物变化")
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "入口修改后热命中", 命中=1)


# ==================== ③ 宏集合与依赖 ====================

def 用例_宏集合(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("③ 宏集合：-D / 货舱 [特性] 独立条目；[依赖] 名清单入键")
    区 = 建工作区(根 / "宏集合", 宏源())
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "无宏基线")
    if 基线 is None:
        return
    asm, 无宏产物 = 基线["asm"], 基线["产物"]
    r宏 = 运行cn(cn, 区, "entry.cn", 目标, ("-D", "特性甲"), 环境统计)
    t.统计断言(r宏, "-D 宏=独立首建", 首建=1, 写入=1)
    有宏产物 = asm.read_bytes()
    t.断言(有宏产物 != 无宏产物, "-D 宏改变产物（宏集合入键与条目名）",
           "%d->%d B" % (len(无宏产物), len(有宏产物)))
    t.断言(len(缓存条目们(区)) == 2, "-D 宏新增独立条目",
           "%d 个" % len(缓存条目们(区)))
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, ("-D", "特性甲"), 环境统计),
               "-D 宏二次命中", 命中=1)
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "回切无宏=命中", 命中=1)
    t.断言(asm.read_bytes() == 无宏产物, "回切无宏产物=基线")

    货区 = 建工作区(根 / "货舱特性", 宏源())
    写货舱(货区, "特性甲")
    货基 = 建立基线(t, cn, 货区, "entry.cn", 目标, "货舱特性甲")
    if 货基 is None:
        return
    甲产物 = 货基["产物"]
    t.统计断言(运行cn(cn, 货区, "entry.cn", 目标, (), 环境统计),
               "货舱特性甲二次命中", 命中=1)
    写货舱(货区, "特性乙")
    r乙 = 运行cn(cn, 货区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r乙, "货舱特性切换=独立首建（并入宏集合）", 首建=1, 写入=1)
    乙产物 = asm路径(货区, 目标).read_bytes()
    t.断言(乙产物 != 甲产物, "特性切换改变产物",
           "%d->%d B" % (len(甲产物), len(乙产物)))
    t.断言(len(缓存条目们(货区)) == 2, "特性切换新增独立条目")
    写货舱(货区, "特性甲")
    r回 = 运行cn(cn, 货区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r回, "恢复旧特性=命中旧条目", 命中=1)
    t.断言(asm路径(货区, 目标).read_bytes() == 甲产物, "恢复后产物=甲基线")

    依区 = 建工作区(根 / "依赖清单", 简单源())
    依基 = 建立基线(t, cn, 依区, "entry.cn", 目标, "无货舱基线")
    if 依基 is None:
        return
    (依区 / "货舱.toml").write_text(
        '[货舱]\n名称 = "cachetest"\n版本 = "1.0"\n[依赖]\n某库 = "内置"\n',
        encoding="utf-8")
    r依 = 运行cn(cn, 依区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r依, "[依赖] 名清单入键=失效重建（即使依赖未被导入）",
               失效重建=1, 写入=1)
    t.断言(asm路径(依区, 目标).read_bytes() == 依基["产物"],
           "依赖声明不改变产物")
    (依区 / "货舱.toml").unlink()
    r删 = 运行cn(cn, 依区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r删, "移除货舱=再次失效重建", 失效重建=1, 写入=1)
    t.断言(asm路径(依区, 目标).read_bytes() == 依基["产物"], "移除后产物=基线")


# ==================== ④ 损坏五形态 ====================

def 用例_损坏形态(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("④ 损坏五形态：截断/乱码/空/元数据不符/版本不符 → 显形回退重建")
    区 = 建工作区(根 / "损坏形态", 简单源())
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    基线条目 = 基线["条目字节"]

    def 元数据不符(b: bytes) -> bytes:
        x = bytearray(b)
        x[48:52] = (int.from_bytes(x[48:52], "little") + 1).to_bytes(4, "little")
        return bytes(x)

    def 版本不符(b: bytes) -> bytes:
        x = bytearray(b)
        x[7] = ord("2")
        return bytes(x)

    形态们 = [
        ("截断", lambda b: b[:20], "文件长度不足头部"),
        ("乱码", lambda b: b"\xff" * 200, "魔数不符（格式版本变化）"),
        ("空", lambda b: b"", "文件不可读或尺寸非法"),
        ("元数据不符", 元数据不符, "载荷长度不符"),
        ("版本不符", 版本不符, "魔数不符（格式版本变化）"),
    ]
    for 名, 破坏, 告警 in 形态们:
        新字节 = 破坏(基线条目)
        t.断言(新字节 != 基线条目 or 名 == "空", "损坏-%s=已实际改写条目" % 名)
        条目.write_bytes(新字节)
        r = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
        ok = t.统计断言(r, "损坏-%s=损坏回退+重建" % 名, 损坏回退=1, 写入=1)
        t.断言(r.returncode == 0 and ok, "损坏-%s=rc=0（回退后编译成功）" % 名,
               "rc=%d" % r.returncode)
        t.断言("警告: [程序缓存] 缓存条目无效（%s）" % 告警 in (r.stderr or ""),
               "损坏-%s=显式告警原文 [%s]" % (名, 告警), (r.stderr or "")[-180:])
        t.断言(条目.name in (r.stderr or ""), "损坏-%s=告警含条目名" % 名, 条目.name)
        t.断言(条目.read_bytes() == 基线条目,
               "损坏-%s=重建条目与基准逐字节一致" % 名)
        t.断言(asm.read_bytes() == 产物, "损坏-%s=重建产物=基线" % 名)
        t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
                   "损坏-%s后热命中" % 名, 命中=1)
        t.断言(not list(条目.parent.glob("*.tmp*")), "损坏-%s处理无临时文件残留" % 名)


# ==================== ⑤ 编译器身份 ====================

def 用例_编译器身份(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑤ 编译器身份：内容哈希入键（touch 不失效·副本失效·切回重建）")
    区 = 建工作区(根 / "编译器身份", 简单源())
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "原编译器基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    基线条目 = 基线["条目字节"]

    cn = pathlib.Path(cn).resolve()
    os.utime(cn, None)
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "touch 编译器=仍命中（身份按内容）", 命中=1)
    t.断言(条目.read_bytes() == 基线条目, "touch 编译器后条目不变")

    变异 = 根 / ("cn_identity_1431" + (".exe" if os.name == "nt" else ""))
    if 变异.exists():
        变异.unlink()
    shutil.copy2(cn, 变异)
    with 变异.open("ab") as f:
        f.write(b"X")
    os.chmod(变异, 0o755)
    t.断言(变异.read_bytes() != cn.read_bytes(), "变异编译器内容已改变")
    r变 = 运行cn(变异, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r变, "变异身份=失效重建（条目名不变·键已变）", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 产物, "变异身份产物=基线（身份只入内容键）")
    t.断言(缓存条目们(区) == [条目] and 条目.read_bytes() != 基线条目,
           "同条目名原地覆盖为新身份键（宿主 1156 既有设计）")
    t.统计断言(运行cn(变异, 区, "entry.cn", 目标, (), 环境统计),
               "变异身份二次命中", 命中=1)
    r回 = 运行cn(cn, 区, "entry.cn", 目标, (), 环境统计)
    t.统计断言(r回, "切回原编译器=再次失效重建", 失效重建=1, 写入=1)
    t.断言(asm.read_bytes() == 产物, "切回后产物=基线")
    t.断言(条目.read_bytes() == 基线条目, "切回后条目字节=基线（确定性重建）")
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "切回后热命中", 命中=1)


# ==================== ⑥ VERIFY 抓伪造 ====================

def 用例_验证通道(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑥ VERIFY：正常命中双一致；伪造汇编/伪造警告被抓并覆盖")
    区 = 建工作区(根 / "验证通道", 简单源())
    基线 = 建立基线(t, cn, 区, "entry.cn", 目标, "asm 基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    基线条目 = 基线["条目字节"]
    验证环境 = dict(环境统计)
    验证环境.update(环境验证)
    r正常 = 运行cn(cn, 区, "entry.cn", 目标, (), 验证环境)
    t.统计断言(r正常, "VERIFY 正常命中=命中+不写回+验证不符0", 命中=1)
    t.断言("[程序缓存验证] 通过：命中产物与重编译产物逐字节一致" in (r正常.stderr or ""),
           "VERIFY 正常路径显式报告逐字节一致")
    t.断言(asm.read_bytes() == 产物 and 条目.read_bytes() == 基线条目,
           "VERIFY 正常路径产物/条目均不变")

    解析 = 解析条目(t, 条目, "asm 基线条目")
    if 解析 is None:
        return
    错汇编 = bytearray(解析["汇编"])
    位置 = next((i for i, b in enumerate(错汇编) if b != 0x0A), None)
    if not t.断言(位置 is not None, "asm 扰动点可定位"):
        return
    错汇编[位置] ^= 1
    条目.write_bytes(重构条目(解析, bytes(错汇编), 解析["警告human"],
                              解析["警告json"]))
    坏 = 解析条目(t, 条目, "错 asm 条目（校验和合法）")
    t.断言(坏 is not None, "错 asm 条目结构/校验和合法（仅内容错）")
    r错 = 运行cn(cn, 区, "entry.cn", 目标, (), 验证环境)
    t.统计断言(r错, "VERIFY 抓住错 asm=验证不符1+写回",
               命中=1, 写入=1, 验证不符=1)
    t.断言("错误: [程序缓存验证] 命中产物与重编译产物不一致" in (r错.stderr or ""),
           "错 asm=显式验证不符告警", (r错.stderr or "")[-220:])
    t.断言(asm.read_bytes() == 产物, "错 asm 被重编译产物覆盖（最终产物正确）")
    t.断言(条目.read_bytes() == 基线条目, "错 asm 条目被重写为基准")
    t.统计断言(运行cn(cn, 区, "entry.cn", 目标, (), 环境统计),
               "覆盖后正常命中", 命中=1)

    警区 = 建工作区(根 / "验证通道警告", 警告源())
    警基 = 建立基线(t, cn, 警区, "main.cn", 目标, "警告基线")
    if 警基 is None:
        return
    警条目 = 警基["条目"]
    警基线条目 = 警基["条目字节"]
    警解析 = 解析条目(t, 警条目, "警告基线条目")
    if 警解析 is None:
        return
    错警告 = bytearray(警解析["警告human"])
    警位 = next((i for i, b in enumerate(错警告) if b not in (0x0A, 0x00)), None)
    if not t.断言(警位 is not None, "警告扰动点可定位"):
        return
    错警告[警位] ^= 1
    警条目.write_bytes(重构条目(警解析, 警解析["汇编"], bytes(错警告),
                                警解析["警告json"]))
    t.断言(解析条目(t, 警条目, "错警告条目（校验和合法）") is not None,
           "错警告条目结构/校验和合法（仅内容错）")
    r警 = 运行cn(cn, 警区, "main.cn", 目标, (), 验证环境)
    t.统计断言(r警, "VERIFY 抓住错警告=验证不符1+写回",
               命中=1, 写入=1, 验证不符=1)
    t.断言("错误: [程序缓存验证] 命中警告与重编译警告不一致" in (r警.stderr or ""),
           "错警告=显式验证不符告警（与 v2 1273 双面比对对齐）",
           (r警.stderr or "")[-220:])
    t.断言(警条目.read_bytes() == 警基线条目, "错警告条目被重写为基准")
    t.断言(asm路径(警区, 目标, "main.cn").read_bytes() == 警基["产物"],
           "错警告场景产物仍正确")

    禁环境 = dict(环境统计)
    禁环境.update(环境禁用)
    警条目.write_bytes(警基线条目)
    asm路径(警区, 目标, "main.cn").unlink()
    r禁 = 运行cn(cn, 警区, "main.cn", 目标, (), 禁环境)
    t.统计断言(r禁, "CN_NO_PROGRAM_CACHE=1 七字段全零（禁用不计入统计）")
    t.断言(asm路径(警区, 目标, "main.cn").read_bytes() == 警基["产物"],
           "禁用态完整编译产物=基线")
    t.断言(警条目.read_bytes() == 警基线条目, "禁用态不写/不修复条目")
    新区 = 建工作区(根 / "禁用不建目录", 简单源())
    t.统计断言(运行cn(cn, 新区, "entry.cn", 目标, (), 禁环境),
               "禁用态首编七字段全零")
    t.断言(not 缓存目录(新区).exists(), "禁用态不创建缓存目录")


# ==================== ⑦ 警告重放 ====================

def 用例_警告重放(t: 计数器, cn, 目标: str, 根: pathlib.Path) -> None:
    t.段("⑦ 警告重放：命中条目的警告仍输出（human/JSON 双通道逐字节）")
    区 = 建工作区(根 / "警告重放", 警告源())
    基线 = 建立基线(t, cn, 区, "main.cn", 目标, "警告基线")
    if 基线 is None:
        return
    asm, 产物, 条目 = 基线["asm"], 基线["产物"], 基线["条目"]
    条目字节, 条目时间 = 基线["条目字节"], 条目.stat().st_mtime_ns
    冷体 = 去统计(基线["统计结果"].stderr)
    t.断言(冷体.count("警告:") == 3, "冷编译 stderr 含 3 条语言警告",
           "%d 条" % 冷体.count("警告:"))
    解析 = 解析条目(t, 条目, "警告条目")
    if 解析 is None:
        return
    t.断言(解析["汇编"] == 产物, "警告条目汇编载荷=落盘 asm")
    t.断言(解析["警告human"].decode("utf-8", "replace").count("警告:") == 3,
           "条目 human 警告载荷含 3 条原文")
    t.断言(len(解析["警告json"]) > 0, "条目 JSON 警告载荷非空",
           "%d B" % len(解析["警告json"]))

    r热 = 运行cn(cn, 区, "main.cn", 目标, (), 环境统计)
    t.统计断言(r热, "二次编译=命中", 命中=1)
    t.断言(asm.read_bytes() == 产物, "命中 asm 逐字节一致")
    t.断言(去统计(r热.stderr) == 冷体, "命中警告重放 stderr 逐字节一致")
    条目未改写(t, 条目, 条目字节, 条目时间, "命中警告重放")

    json区 = 建工作区(根 / "警告重放json", 警告源())
    rj1 = 运行cn(cn, json区, "main.cn", 目标, ("--json",), 环境统计)
    t.统计断言(rj1, "--json 冷编译=首建+写入", 首建=1, 写入=1)
    json冷 = rj1.stdout or ""
    t.断言(json冷.strip().startswith("[") and "警告" in json冷,
           "--json 冷输出为含警告的 JSON 数组", json冷[:120])
    rj2 = 运行cn(cn, json区, "main.cn", 目标, ("--json",), 环境统计)
    t.统计断言(rj2, "--json 二次=命中", 命中=1)
    t.断言((rj2.stdout or "") == json冷, "--json 命中 stdout 逐字节重放")
    t.断言(asm路径(json区, 目标, "main.cn").read_bytes() == 产物,
           "--json 命中产物=基线")


# ==================== 汇总/自测 ====================

def 汇总退出码(t: 计数器) -> int:
    return 0 if t.失败 == 0 else 1


def 自测() -> int:
    """零编译器依赖：CNPGMC01 解析/重构往返 + 计数器正反态 + 统计解析。"""
    import subprocess
    from host_program_cache_testlib import fnv1a64, 重构条目, 头条, 载荷头
    t = 计数器()
    正 = t.断言(True, "自测·正样本")
    负 = t.断言(False, "自测·负样本（预期打印 FAIL）")
    合成 = subprocess.CompletedProcess(
        [], 0, "", "[警告] x[程序缓存] 命中=2 首建=0 失效重建=0 损坏回退=0 写入=0 "
        "写失败=0 验证不符=0\n")
    统计成 = t.统计断言(合成, "自测·统计七字段解析", 命中=2)
    空摘要 = fnv1a64(b"") == 14695981039346656037
    汇编 = b"mov eax, 1\n"
    警告 = "警告: x\n".encode("utf-8")
    头 = bytearray(b"CNPGMC01" + b"\x00" * (头条 - 8))
    载荷 = (len(汇编).to_bytes(4, "little") + len(警告).to_bytes(4, "little") +
            (0).to_bytes(4, "little") + (0).to_bytes(4, "little") + 汇编 + 警告)
    头[40:48] = fnv1a64(载荷).to_bytes(8, "little")
    头[48:52] = len(载荷).to_bytes(4, "little")
    原始 = bytes(头) + 载荷
    解析 = {"字节": 原始}
    重建 = 重构条目(解析, 汇编, 警告, b"")
    往返 = (重建[:8] == b"CNPGMC01" and
            int.from_bytes(重建[40:48], "little") == fnv1a64(重建[头条:]) and
            int.from_bytes(重建[48:52], "little") == len(重建) - 头条 and
            重建[头条:] == 载荷)
    判据 = 汇总退出码(t) == 1 and 汇总退出码(计数器()) == 0
    ok = 正 and not 负 and 统计成 and 空摘要 and 往返 and 判据
    print("自测：%s（负样本被计数=%s·FNV空串=%s·条目往返=%s·退出码判据=%s）"
          % ("PASS" if ok else "FAIL", not 负, 空摘要, 往返, 判据))
    return 0 if ok else 1


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="宿主程序级预编译缓存自动化回归（#1431）")
    解析.add_argument("--cn", default=str(仓库根 / "target" /
                     ("cn.exe" if sys.platform.startswith("win") else "cn")))
    解析.add_argument("--目标", default=默认目标)
    解析.add_argument("--工作根", default=str(仓库根 / "target" / "cache_test_1431"))
    解析.add_argument("--自测", action="store_true",
                     help="零编译器依赖的最小正反自测")
    参数 = 解析.parse_args()
    if 参数.自测:
        return 自测()
    cn = pathlib.Path(参数.cn).resolve()
    if not cn.exists():
        print("错误: 宿主编译器不存在: %s（先构建 target/cn）" % cn)
        return 2
    根 = pathlib.Path(参数.工作根).resolve()
    if 根 == 仓库根 or 是祖先(根, 仓库根):
        print("错误: --工作根 不能是仓库根或其祖先（防止误删）: %s" % 根)
        return 2
    if 根.exists():
        shutil.rmtree(根)
    根.mkdir(parents=True)
    t = 计数器()
    print("== #1431 宿主程序级预编译缓存回归（目标=%s）==" % 参数.目标, flush=True)
    print("   宿主=%s" % cn)
    开始 = time.monotonic()
    try:
        用例_条目身份(t, cn, 参数.目标, 根)
        用例_内容键失效(t, cn, 参数.目标, 根)
        用例_宏集合(t, cn, 参数.目标, 根)
        用例_损坏形态(t, cn, 参数.目标, 根)
        用例_编译器身份(t, cn, 参数.目标, 根)
        用例_验证通道(t, cn, 参数.目标, 根)
        用例_警告重放(t, cn, 参数.目标, 根)
        无临时残留(t, 根)
    except Exception:
        t.失败 += 1
        t.失败项们.append("未捕获异常")
        print("  FAIL 未捕获异常：")
        traceback.print_exc()
    用时 = time.monotonic() - 开始
    print("\n== 汇总（%.1fs）==" % 用时)
    print("  PASS %d  FAIL %d" % (t.通过, t.失败))
    if t.失败项们:
        print("  失败项：" + "、".join(t.失败项们))
    return 汇总退出码(t)


if __name__ == "__main__":
    sys.exit(主流程())
