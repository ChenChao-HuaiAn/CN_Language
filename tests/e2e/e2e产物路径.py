#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""e2e产物路径.py —— run_e2e「共享构建产物」的分片作用域单点（任务 #1638 回归修复版）。

不变量（#1638 一轮回归教训·必须先读）：
  run_e2e 的每用例产物（v2src<隔离键>/v2work<隔离键>/dualhost<隔离键>/dualneg*/
  supply*/selfwork*/v2out_*/v2asm_*.o）路径文本**任何运行模式逐字不变**：
  用例程序会把自身源路径打进输出（[p6h] 已解析: target/audit2/v2src952_86cb5395/
  主.cn …），全库 201 行 .expected 锚住该文本、4 例硬编码完整隔离键；首版 #1638
  曾给隔离键追加 _s<i>of<n> 后缀 → 952 双编译对照云端必红（程序打印
  v2src952_86cb5395_s8of9/主.cn ≠ 期望）。故本模块**只**给「共享构建产物」加片
  作用域（v2p/v2pobj/rt .o/构建指纹/重建互斥锁），每用例目录与隔离键一律不碰。

用途：分片运行（--shard i/n）时，共享构建产物落 <项目根>/target/audit2/s<i>of<n>/，
各片各建各用、各持各的重建锁——同一棵树上多片运行零共享可写路径；未分片运行
保持 target/audit2 原布局（开发者定向运行与 check_knownlen_fastpath /
bench_self_host / check_source_coverage 的单点路径依赖不变）。

传递通道：E2E 并行池用 multiprocessing spawn（604-a·每用例独立 worker 进程）——
worker 不继承主进程模块全局，但继承 os.environ：作用域经 CN_E2E_ARTIFACT_SCOPE
传递；主进程在解析 --shard 后单点设置（启用作用域），worker 侧同式解析。标签限
ASCII（ml64/link/as/g++ 命令行禁中文路径——GBK 误读实测 LNK1181）。

契约（两态显式·无静默兜底）：
  · 未设/空串 = 不分片 → 共享构建产物落原布局（合法值，不是错误）；
  · 合法标签 = 共享构建目录后缀（run_e2e 生成 s<片序>of<片数>）；
  · 非法标签（路径分隔符/非 ASCII/超长/纯空白）= ValueError 显式抛，调用方报错
    中止——禁静默退回共享布局（静默退回=并行互踩复发面·882 纪律四同族）。

自检：`python tests/e2e/e2e产物路径.py --selftest`（零依赖·正反两态；run_e2e
--selftest 亦挂载）。
"""
from __future__ import annotations

import os
import pathlib
import sys

# 分片作用域环境变量名（主进程设置 → spawn worker 继承；见模块 docstring 通道说明）
作用域环境变量 = "CN_E2E_ARTIFACT_SCOPE"
# 标签长度上限：目录名后缀使用，过长会逼近 Windows 路径上限
作用域标签最大长 = 32


def 校验标签(标签: str) -> str:
    """校验作用域标签：仅 ASCII 字母/数字/下划线，长度 1..32；非法=ValueError。

    显式拒绝而非清洗替换：清洗会把两个不同片映射到同一目录（静默互踩复发）。"""
    if not 标签 or not 标签.strip():
        raise ValueError("作用域标签为空（空串=不分片，不得作为显式标签传入）")
    if len(标签) > 作用域标签最大长:
        raise ValueError(f"作用域标签过长: {len(标签)} > {作用域标签最大长}（{标签!r}）")
    for 字符 in 标签:
        if not (字符.isascii() and (字符.isalnum() or 字符 == "_")):
            raise ValueError(f"作用域标签含非法字符 {字符!r}（限 ASCII 字母/数字/下划线: {标签!r}）")
    return 标签


def 作用域标签() -> str:
    """当前作用域标签：未设/空串=不分片返回 ""；非法标签显式抛 ValueError。"""
    原值 = (os.environ.get(作用域环境变量) or "").strip()
    if not 原值:
        return ""
    return 校验标签(原值)


def 分片标签(片序: int, 片数: int) -> str:
    """分片→标签映射 s<片序>of<片数>（片序 0 起·片数≥1）；非法片参数=ValueError。"""
    if 片数 < 1:
        raise ValueError(f"片数须 ≥1（收到 {片数}）")
    if not (0 <= 片序 < 片数):
        raise ValueError(f"片序须落在 [0, 片数)（收到 片序={片序} 片数={片数}）")
    return "s%dof%d" % (片序, 片数)


def 启用作用域(片序: int, 片数: int) -> str:
    """设置本进程及 spawn 子进程的共享构建产物作用域（写环境变量·返回标签）。"""
    标签 = 分片标签(片序, 片数)
    os.environ[作用域环境变量] = 标签
    return 标签


def 共享构建目录(项目根) -> pathlib.Path:
    """共享构建产物根：<项目根>/target/audit2[ /<作用域标签>]（唯一随片隔离的面）。"""
    根 = pathlib.Path(项目根) / "target" / "audit2"
    标签 = 作用域标签()
    return 根 / 标签 if 标签 else 根


def 自检() -> int:
    """零依赖自检（正反两态）：非分片原布局、分片目录隔离、非法输入显式拒。"""
    断言们: list[tuple[str, bool]] = []

    def 抛值错误(动作) -> bool:
        try:
            动作()
        except ValueError:
            return True
        return False

    原值 = os.environ.get(作用域环境变量)
    try:
        os.environ.pop(作用域环境变量, None)
        断言们.append(("未设=原布局（共享构建根=target/audit2）",
                   作用域标签() == ""
                   and 共享构建目录("根") == pathlib.Path("根") / "target" / "audit2"))

        os.environ[作用域环境变量] = "s8of9"
        断言们.append(("作用域=共享构建根落片目录",
                   共享构建目录("根") == pathlib.Path("根") / "target" / "audit2" / "s8of9"))
        断言们.append(("合法标签原样返回", 作用域标签() == "s8of9"))

        os.environ[作用域环境变量] = ""
        断言们.append(("空串=不分片（合法值）", 作用域标签() == ""))

        for 非法 in ("a/b", "a\\b", "中文片", "..", "a b", "x" * (作用域标签最大长 + 1)):
            断言们.append((f"非法标签显式拒 {非法!r}",
                       抛值错误(lambda v=非法: 校验标签(v))))

        os.environ.pop(作用域环境变量, None)
        os.environ[作用域环境变量] = "s1of3"
        断言们.append(("启用作用域写环境（spawn 通道）",
                   启用作用域(1, 3) == "s1of3"
                   and os.environ.get(作用域环境变量) == "s1of3"))
        断言们.append(("片参数反例显式拒（片序越界/片数非法）",
                   抛值错误(lambda: 分片标签(3, 3)) and 抛值错误(lambda: 分片标签(0, 0))))
        断言们.append(("分片标签形态 s<i>of<n>", 分片标签(0, 12) == "s0of12"))
    finally:
        if 原值 is None:
            os.environ.pop(作用域环境变量, None)
        else:
            os.environ[作用域环境变量] = 原值
    for 说明, 过 in 断言们:
        print("  [%s] %s" % ("✓" if 过 else "✗", 说明))
    失败 = sum(1 for _, 过 in 断言们 if not 过)
    print("\n[分片共享构建作用域自检] %d 例·失败 %d" % (len(断言们), 失败))
    return 1 if 失败 else 0


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        sys.exit(自检())
    print("用法: python tests/e2e/e2e产物路径.py --selftest")
    sys.exit(2)
