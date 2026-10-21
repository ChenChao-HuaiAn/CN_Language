#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_v2_output_dir.py —— #1138 回归：v2 写 asm 前自动递归创建输出目录。

背景：宿主 compile/build 走 cn_main.cpp ensureDirExists 逐级建目录；v2 驱动器
  写死 cwd 相对 target/v2asm.{s|asm}，修前目录缺失直接 [cg1] asm 写文件失败
  rc=1（#1108 显式失败落地后与宿主分叉显形：目录缺失时宿主成功、v2 拒绝）。
  修法=写asm文件 打开前 stdlib 创建目录 递归建齐（三后端共用），失败显式诊断。

run_e2e 的 v2 闭环/双编译通道会**预建** 工作目录/target（runner 隔离产物需要），
无法覆盖「目录缺失」场景——本独立脚本因此存在（任务 #1138 裁决：独立最小回归
+ 覆盖豁免登记）。E2E 1442 覆盖 stdlib 创建目录 的递归/幂等/失败三态；本脚本
覆盖编译器写 asm 这一入口面。

两态用例（真实生产链路：宿主编译器构建 v2p → v2p 编译最小程序 → 看盘）：
  ① 正：三后端（win-x64 / linux-x86_64 / linux-arm64）× cwd 无 target/ →
     v2p rc=0 且 target/v2asm.{s|asm} 落盘、含入口符号（同一 写asm文件 路径零后端差异）；
  ② 负：cwd 的 target 是**同名文件**（中段挡路）→ v2p rc=1 且 stderr 含
     「asm 输出目录创建失败」（禁静默继续），不产 asm。

用法：python scripts/test_v2_output_dir.py [--cn target/cn]
  --cn   宿主编译器路径（缺省 target/cn 或 target/cn.exe）
中间产物默认保留在 target/test_v2_output_dir 供审计（target/ 已 gitignore）。
退出码：0=全绿；1=断言失败；2=前置缺失（编译器不存在/构建失败）。
"""
from __future__ import annotations

import argparse
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

仓库根 = Path(__file__).resolve().parent.parent
工作根 = 仓库根 / "target" / "test_v2_output_dir"
超时秒 = 1800


def 目标平台() -> str:
    if sys.platform.startswith("win"):
        return "win-x64"
    if platform.machine().lower() in ("aarch64", "arm64"):
        return "linux-arm64"
    return "linux-x86_64"


def 断言行(条件: bool, 说明: str) -> None:
    if not 条件:
        raise AssertionError(说明)
    print("  ✓ " + 说明)


def 运行(命令: list, cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run([str(x) for x in 命令], cwd=str(cwd), capture_output=True,
                          text=True, encoding="utf-8", errors="replace", timeout=超时秒)


def 找编译器(显式: Optional[str]) -> Optional[Path]:
    if 显式:
        p = Path(显式)
        return p if p.exists() else None
    for 名 in ("cn.exe", "cn"):
        p = 仓库根 / "target" / 名
        if p.exists():
            return p
    return None


def 主流程() -> int:
    参数解析 = argparse.ArgumentParser()
    参数解析.add_argument("--cn", default=None)
    参数 = 参数解析.parse_args()

    cn = 找编译器(参数.cn)
    if cn is None:
        print("✗ 未找到宿主编译器（target/cn 或 --cn 指定）——请先构建")
        return 2
    宿主平台 = 目标平台()
    后缀 = ".exe" if 宿主平台 == "win-x64" else ""

    # 工作根每次清空重建（残留 target 目录会掩盖「目录缺失」场景）
    if 工作根.exists():
        shutil.rmtree(工作根)
    工作根.mkdir(parents=True, exist_ok=True)
    源 = 工作根 / "最小.cn"
    源.write_text("函数 主() -> 整32 {\n    返回 0;\n}\n", encoding="utf-8")

    # ---- 前置：宿主编译器构建 v2p（真实生产链路；一份二进制分派三后端）----
    v2p = 工作根 / ("v2p" + 后缀)
    print(f"[1] {cn} build → {v2p.name}（{宿主平台}）")
    构建 = 运行([cn, "build", str(仓库根 / "CN语言编译器v2" / "主.cn"),
                "--target", 宿主平台, "--output", str(v2p)], 仓库根)
    if 构建.returncode != 0 or not v2p.exists():
        print("✗ v2p 构建失败 rc=%d" % 构建.returncode)
        print((构建.stderr or 构建.stdout or "").strip()[:800])
        return 2
    断言行(True, "v2p 构建成功")

    def 跑v2p(工作目录: Path, 目标: str) -> subprocess.CompletedProcess:
        return 运行([v2p, 源, 目标], 工作目录)

    # ---- ① 正：三后端 × cwd 无 target/，v2p 必须自建并成功写盘 ----
    三后端 = (("win-x64", "v2asm.asm", "cn_main PROC"),
              ("linux-x86_64", "v2asm.s", ".globl cn_main"),
              ("linux-arm64", "v2asm.s", ".globl cn_main"))
    for 目标, asm名, 入口标记 in 三后端:
        正目录 = 工作根 / ("正_目录缺失_" + 目标)
        正目录.mkdir(parents=True, exist_ok=True)
        断言行(not (正目录 / "target").exists(), f"[{目标}] 正样本前置：cwd 无 target/")
        正结果 = 跑v2p(正目录, 目标)
        print(f"    [{目标}] v2p rc=%d" % 正结果.returncode)
        print(f"    [{目标}] stderr: "
              + (正结果.stderr or "").strip()[:400].replace("\n", " | "))
        断言行(正结果.returncode == 0, f"[{目标}] v2p rc=0（目录缺失不再拒绝）")
        正asm = 正目录 / "target" / asm名
        断言行((正目录 / "target").is_dir(), f"[{目标}] target/ 已由 v2p 自动创建")
        断言行(正asm.is_file() and 正asm.stat().st_size > 0,
               f"[{目标}] target/{asm名} 已落盘且非空")
        正文 = 正asm.read_text(encoding="utf-8", errors="replace")
        断言行(入口标记 in 正文, f"[{目标}] asm 含入口符号锚「{入口标记}」")
        print(f"    [{目标}] ls: "
              + ", ".join(sorted(p.name for p in (正目录 / "target").iterdir())))

    # ---- ② 负：target 是同名文件 → 显式诊断 + rc=1 + 不产 asm（一个后端足够·共享路径）----
    负目录 = 工作根 / "负_同名文件挡路"
    负目录.mkdir(parents=True, exist_ok=True)
    (负目录 / "target").write_text("占位文件，非目录\n", encoding="utf-8")
    负结果 = 跑v2p(负目录, "linux-x86_64")
    诊断 = (负结果.stderr or "") + (负结果.stdout or "")
    print("    [负] v2p rc=%d" % 负结果.returncode)
    print("    [负] stderr: " + (负结果.stderr or "").strip()[:500].replace("\n", " | "))
    断言行(负结果.returncode == 1, "v2p rc=1（建目录失败显式拒绝）")
    断言行("asm 输出目录创建失败" in 诊断, "诊断原文含「asm 输出目录创建失败」")
    断言行(not (负目录 / "target" / "v2asm.s").exists(), "失败路径不产 asm（无错误产物）")

    print(f"\n全绿：{工作根}（中间产物保留供审计；可手工删除）")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(主流程())
    except AssertionError as 异常:
        print("✗ 断言失败：" + str(异常))
        sys.exit(1)
