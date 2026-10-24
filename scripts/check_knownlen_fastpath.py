#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#1310 字符串下标「编译期已知长度」快路径回归门禁（防优化被静默回退）。

背景（#1310 审查 E2E 项）：
  `字符串[i]` 越界检查在对象为字符串字面量/顶层字符串常量时应使用编译期已知
  字节长度（ConstInt 比较），**不再调用 __cn_str_len(strlen)**；变量/字段/调用
  返回等未知形态必须保留运行时 strlen 原路径。行为用例（1462/1463/1464）只能
  证明语义正确，无法发现「快路径被整体回退成 strlen」（结果仍然正确、只是变慢）
  ——本门禁用产物计数把该回归机械化。

检测面（三层，任一失败=红）：
  ① 宿主 IR：同一程序中「1 个字面量下标 + 1 个变量下标」→ `[__cn_str_len]`
     出现次数必须 == 1（仅变量）；两个变量下标对照样本必须 == 2（计数器分辨力）。
  ② v2 产物 asm（v2p 就绪时）：同上探针 → 产物 `call __cn_str_len` 行数 1/2。
     v2p 未就绪 → 黄字跳过产物段（L2/云端在 v2p 预热后覆盖），源码断言照跑。
  ③ v2 源码静态断言：`生成表达式下标.cn` 必须保留「已知长度」传参链路
     （发射字符串越界检查(… 已知长度s) / 推导字符串下标已知长度 / 某些(字符串
     长度)）——防快路径代码整体被删/调用被改而无人知晓（文本锁，教训 #275：
     仅作 v2p 不可达时的兜底，产物段优先）。

自测：`python scripts/check_knownlen_fastpath.py --selftest`（计数核正反样本）。
用法：`python scripts/check_knownlen_fastpath.py [--cn target/cn.exe]
        [--v2p target/audit2/v2p.exe]`（gate_quick 写集触发时自动调用）。
"""

from __future__ import annotations   # P1-1：3.8 注解惰性化（list[str] 等定义期不求值）

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

项目根 = pathlib.Path(__file__).resolve().parent.parent

# 探针源码：①快路径样本（字面量下标 0 次 strlen + 变量下标 1 次）
快路径样本 = """函数 主() -> 整32 {
    字符串 s = "ABCDE";
    打印(整32("ABCDE"[2]));
    打印(整32(s[2]));
    返回 0;
}
"""
# 对照样本（两个变量下标 = 2 次 strlen）——证明计数器能分辨快路径是否生效
对照样本 = """函数 主() -> 整32 {
    字符串 a = "ABCDE";
    字符串 b = "XY";
    打印(整32(a[2]));
    打印(整32(b[1]));
    返回 0;
}
"""


def 统计IR长度调用(ir文本: str) -> int:
    """宿主 `cn ir` 文本中 __cn_str_len 调用数（IR 行形态 `[__cn_str_len]`）。"""
    return ir文本.count("[__cn_str_len]")


def 统计asm长度调用(asm文本: str) -> int:
    """v2/宿主汇编中 `call __cn_str_len` 调用数（EXTERN 声明行不计）。"""
    模式 = re.compile(r"^\s*call\s+__cn_str_len\b", re.IGNORECASE | re.MULTILINE)
    return len(模式.findall(asm文本))


def 运行(命令, cwd=None, 超时秒=180):
    return subprocess.run(命令, cwd=str(cwd) if cwd else None,
                          capture_output=True, text=True, encoding="utf-8",
                          errors="replace", timeout=超时秒)


def 宿主检查(cn: pathlib.Path, 工作目录: pathlib.Path) -> list[str]:
    错误 = []
    快路径源 = 工作目录 / "快路径.cn"
    对照源 = 工作目录 / "对照.cn"
    快路径源.write_text(快路径样本, encoding="utf-8")
    对照源.write_text(对照样本, encoding="utf-8")
    for 名称, 源, 期望 in (("快路径样本", 快路径源, 1), ("对照样本", 对照源, 2)):
        结果 = 运行([str(cn), "ir", str(源)])
        if 结果.returncode != 0:
            错误.append(f"宿主 {名称} `cn ir` 失败(rc={结果.returncode}): "
                        f"{(结果.stderr or 结果.stdout).strip()[:200]}")
            continue
        实测 = 统计IR长度调用(结果.stdout)
        if 实测 != 期望:
            错误.append(f"宿主 {名称} __cn_str_len 调用={实测}（期望 {期望}）"
                        f"——已知长度快路径"+("未生效/被回退" if 期望 == 1 else "计数面异常"))
    return 错误


def v2产物检查(v2p: pathlib.Path, 工作目录: pathlib.Path) -> list[str]:
    错误 = []
    # v2p 的 stdlib 按 cwd 相对读 stdlib/<名>.cn（run_e2e 同口径）——临时工作目录
    #   内复制 stdlib 树，产物 target/v2asm.asm|.s 落临时目录不污染本树。
    #   1528 同族（linux 门禁必红根治）：产物名随平台——win 后端=v2asm.asm·
    #   GAS 后端（linux/arm64）=v2asm.s（1485 终审在 win 机验收写死 .asm·
    #   linux 机器 L1 恒红「未生成」实录·1479 批集成被卡顺手根治）。
    工作目录.mkdir(parents=True, exist_ok=True)
    标准库 = 工作目录 / "stdlib"
    if not 标准库.exists():
        shutil.copytree(项目根 / "stdlib", 标准库)
    是win本机 = sys.platform == "win32"
    平台参数 = "win-x64" if 是win本机 else "linux-x86_64"
    asm名 = "v2asm.asm" if 是win本机 else "v2asm.s"
    asm路径 = 工作目录 / "target" / asm名
    for 名称, 源码, 期望 in (("快路径样本", 快路径样本, 1),
                           ("对照样本", 对照样本, 2)):
        源 = 工作目录 / f"{名称}.cn"
        源.write_text(源码, encoding="utf-8")
        if asm路径.exists():
            asm路径.unlink()
        结果 = 运行([str(v2p), str(源), 平台参数], cwd=工作目录)
        if 结果.returncode != 0:
            错误.append(f"v2 {名称} 编译失败(rc={结果.returncode}): "
                        f"{(结果.stderr or 结果.stdout).strip()[:200]}")
            continue
        if not asm路径.exists():
            错误.append(f"v2 {名称} 未生成 {asm路径}")
            continue
        实测 = 统计asm长度调用(asm路径.read_text(encoding="utf-8", errors="replace"))
        if 实测 != 期望:
            错误.append(f"v2 {名称} call __cn_str_len={实测}（期望 {期望}）"
                        f"——已知长度快路径"+("未生效/被回退" if 期望 == 1 else "计数面异常"))
    return 错误


def 源码断言() -> list[str]:
    源路径 = 项目根 / "CN语言编译器v2" / "IR" / "生成表达式下标.cn"
    if not 源路径.exists():
        return [f"v2 源码缺失: {源路径}"]
    文本 = 源路径.read_text(encoding="utf-8", errors="replace")
    必需 = (
        "发射字符串越界检查(下标值, 对象值, 指令, 寄存器计数, 块计数, 已知长度s)",
        "推导字符串下标已知长度",
        "某些(字符串长度",
    )
    缺失 = [锚 for 锚 in 必需 if 锚 not in 文本]
    if 缺失:
        return [f"v2 源码快路径链路缺失（疑似回退）: {缺失}"]
    return []


def 自测() -> int:
    失败 = []
    if 统计IR长度调用("调用 %v1 %v2 (i64) [__cn_str_len]\n") != 1:
        失败.append("IR 计数正样本失败")
    if 统计IR长度调用("调 用 [__cn_str_len] [__cn_str_len]") != 2:
        失败.append("IR 计数多调用样本失败")
    asm = "    call __cn_str_len\nEXTERN __cn_str_len:PROC\nCALL  __cn_str_len\n"
    if 统计asm长度调用(asm) != 2:
        失败.append("asm 计数未排除 EXTERN 或大小写失败")
    if 失败:
        print("[selftest] ✗ " + "；".join(失败))
        return 1
    print("[selftest] ✓ 正反样本全过（IR 1/2、asm 忽略 EXTERN/大小写）")
    return 0


def 默认v2p路径() -> pathlib.Path:
    """平台化 v2p 路径（勿硬编码 win 名）：win=target/audit2/v2p.exe；
    linux 按 run_e2e 命名——arm64=v2p_linux，x86_64=v2p_linuxx64
    （见 tests/e2e/run_e2e.py 确保v2p与运行时就绪 产物后缀）。"""
    if sys.platform == "win32":
        return 项目根 / "target" / "audit2" / "v2p.exe"
    import platform
    机器 = platform.machine().lower()
    if "arm64" in 机器 or "aarch64" in 机器:
        return 项目根 / "target" / "audit2" / "v2p_linux"
    return 项目根 / "target" / "audit2" / "v2p_linuxx64"


def main() -> int:
    parser = argparse.ArgumentParser(description="#1310 已知长度快路径回归门禁")
    parser.add_argument("--cn", default="", help="宿主编译器路径（默认 target/cn[.exe]）")
    parser.add_argument("--v2p", default="", help="v2p 路径（默认按平台探测 target/audit2/v2p[_linuxx64|_linux][.exe]）")
    parser.add_argument("--selftest", action="store_true", help="计数核正反自测")
    args = parser.parse_args()
    if args.selftest:
        return 自测()

    cn = pathlib.Path(args.cn) if args.cn else (
        项目根 / "target" / ("cn.exe" if sys.platform == "win32" else "cn"))
    v2p = pathlib.Path(args.v2p) if args.v2p else 默认v2p路径()
    if not cn.exists():
        print(f"[已知长度快路径] ✗ 宿主编译器缺失: {cn}")
        return 1

    错误 = []
    工作目录 = pathlib.Path(tempfile.mkdtemp(prefix="cn_knownlen_", dir=str(项目根 / "target")))
    try:
        错误 += 宿主检查(cn, 工作目录)
        if v2p.exists():
            v2目录 = 工作目录 / "v2"
            错误 += v2产物检查(v2p, v2目录)
        else:
            print(f"[已知长度快路径] [黄] v2p 未就绪（{v2p}）——v2 产物计数段跳过，"
                  f"源码静态断言照跑（L2/云端 v2p 预热后覆盖产物段）")
        错误 += 源码断言()
    finally:
        shutil.rmtree(工作目录, ignore_errors=True)

    if 错误:
        print("[已知长度快路径] ✗ 回归（#1310 快路径被回退/被破坏）：")
        for e in 错误:
            print("  [×] " + e)
        return 1
    print("[已知长度快路径] ✓ 宿主 IR（1/2）"
          + ("+ v2 产物 asm（1/2）" if v2p.exists() else "")
          + "+ v2 源码链路断言全过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
