#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_win_asm.py —— v2 编译器 win(MASM) 目标产物 asm 的静态符号门禁（501-a 立）。

**为什么需要**：win 侧 E2E 的 v2 产物由 ml64 汇编，而 linux/arm64 侧走 GAS 后端
且**不发射 EXTERN**（GAS 未定义符号默认外部）——所以「win 产物 asm 形态错误」这类
缺陷在深度机/单位机全绿、只在家机爆红（501-a 实证：家机 174 例 `symbol redefinition:
strData_N`，本机 linux 全量 483=481/0/2 零红）。本脚本在**任意平台**用 v2p 交叉生成
win 目标 asm 并做等价判据扫描，把跨平台分叉提前到本机暴露。

**等价判据（ml64 报错形态前置）**：
 ① EXTERN 声明 ∩ 本地定义 ≠ ∅  → ml64 `A2005 symbol redefinition`
    （501-a 原形：T69 新增的 lea 行 EXTERN 收集把 strData_N/floatData_N 本地常量
     池标签误判为跨单元引用）
 ② EXTERN 操作数非裸符号（含 `[` 或空白）→ ml64 语法错误
    （501-a 同源：IR_取地址 的 `lea rax, [rbp-N]` 被收成 `EXTERN [rbp-16]:PROC`）
 ③ `lea rax, <裸符号>` 引用既非本地定义亦无 EXTERN → 将来 ml64 `A2006 undefined symbol`
    （防「修复过度收窄」：真外部符号如 __chkstk/cn_alloc 必须仍被声明）

用法::

    # 1) 先建 v2p（v2 树改动后必重建）
    target/cn build CN语言编译器v2/主.cn --target linux-x86_64 --output target/audit2/v2p_linuxx64
    # 2) 扫描全部 v2 路径用例（双编译对照.txt / v2闭环.txt）
    python3 scripts/check_win_asm.py target/audit2/v2p_linuxx64 tests/e2e --jobs 8

退出码：0=零冲突（通过的用例至少 1 个）；1=存在冲突/未解析引用；2=参数或环境错误。
注意：`v2p 未产 asm` 的用例（负测预期失败）不计入判据——与 runner 负路径纪律一致。
"""
from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

EXTERN_RE = re.compile(r"^EXTERN\s+(?P<sym>.*?):PROC\s*$")
定义_RE = re.compile(r"^(?P<sym>[?A-Za-z_][\w$@?]*)\s+(?:PROC|db|REAL8)\b"
                     r"|^(?P<sym2>[?A-Za-z_][\w$@?]*):\s*$")
LEA_RE = re.compile(r"^\s+lea rax, (?P<op>.+?)\s*$")


def 扫描产物(asm路径: pathlib.Path) -> dict:
    """返回 {冲突, 非法EXTERN, 未解析引用, 本地数, EXTERN数, 引用数}"""
    本地, 外部, 非法, 引用 = set(), set(), [], []
    文本 = asm路径.read_text(encoding="utf-8", errors="replace")
    for 行 in 文本.splitlines():
        匹配 = EXTERN_RE.match(行)
        if 匹配:
            符号 = 匹配.group("sym")
            if 符号 == "" or "[" in 符号 or " " in 符号 or "\t" in 符号:
                非法.append(符号)
            else:
                外部.add(符号)
            continue
        匹配2 = 定义_RE.match(行)
        if 匹配2:
            本地.add(匹配2.group("sym") or 匹配2.group("sym2"))
        匹配3 = LEA_RE.match(行)
        if 匹配3:
            操作数 = 匹配3.group("op")
            if 操作数 and "[" not in 操作数 and " " not in 操作数 and "\t" not in 操作数:
                引用.append(操作数)
    未解析 = sorted({符 for 符 in 引用 if 符 not in 本地 and 符 not in 外部})
    # 判据④（518-b·家机回签建议）：EXTERN 操作数=**寄存器名** → ml64 A2008
    #   （T69 间接调用 `call r11` 被 EXTERN 收集器误收·七例实证）
    for 符 in 外部:
        小写 = 符.lower()
        if 2 <= len(小写) <= 4 and 小写[0] in ("r", "e") and all(c.isalnum() and c.isascii() for c in 小写):
            非法.append(符 + "（寄存器名·A2008 风险）")
    return {"冲突": sorted(外部 & 本地), "非法EXTERN": 非法, "未解析": 未解析,
            "本地数": len(本地), "EXTERN数": len(外部), "引用数": len(引用)}


def 跑用例(v2p: pathlib.Path, 用例根: pathlib.Path, 用例: pathlib.Path, 工作根: pathlib.Path) -> dict:
    名称 = 用例.name
    工作 = 工作根 / f"w_{名称}"
    源码 = 工作根 / f"s_{名称}"
    for 路径 in (工作, 源码):
        if 路径.exists():
            shutil.rmtree(路径, ignore_errors=True)
    (工作 / "target").mkdir(parents=True, exist_ok=True)
    shutil.copytree(用例, 源码, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args"))
    stdlib链 = 工作 / "stdlib"
    if not stdlib链.exists():
        stdlib链.symlink_to(用例根.parent / "stdlib")
    入口 = 源码 / "主.cn"
    if not 入口.exists():
        cn们 = sorted(源码.glob("*.cn"))
        if not cn们:
            return {"名称": 名称, "状态": "无源文件"}
        入口 = cn们[0]
    参 = [str(v2p), str(入口.resolve()), "win-x64"]
    编译选项 = 用例 / "编译选项.txt"
    if 编译选项.exists():
        参.append(str(编译选项.resolve()))
    try:
        结果 = subprocess.run(参, cwd=工作, capture_output=True, text=True, timeout=180)
    except subprocess.TimeoutExpired:
        return {"名称": 名称, "状态": "超时"}
    asm = 工作 / "target" / "v2asm.asm"
    if 结果.returncode != 0 or not asm.exists():
        return {"名称": 名称, "状态": f"v2p 未产 asm（rc={结果.returncode}·负测/编译失败形态）"}
    扫描 = 扫描产物(asm)
    坏 = 扫描["冲突"] or 扫描["非法EXTERN"] or 扫描["未解析"]
    return {"名称": 名称, "状态": "冲突" if 坏 else "OK", **扫描}


def 主() -> int:
    解析 = argparse.ArgumentParser(description="v2 win 目标 asm 符号门禁（ml64 报错形态前置）")
    解析.add_argument("v2p", help="v2p 可执行（宿主 --target linux-x86_64 编 v2 树所得）")
    解析.add_argument("用例根", help="E2E 用例根目录（tests/e2e）")
    解析.add_argument("--jobs", type=int, default=8)
    解析.add_argument("--only", default=None, help="按目录名前缀过滤（逗号分隔）")
    解析.add_argument("--工作根", default="/tmp/cn_win_asm_check")
    参 = 解析.parse_args()
    v2p = pathlib.Path(参.v2p).resolve()
    用例根 = pathlib.Path(参.用例根).resolve()
    if not v2p.exists():
        print(f"[错误] v2p 不存在: {v2p}（先 target/cn build CN语言编译器v2/主.cn …）")
        return 2
    if not (用例根 / "coverage_map.md").exists():
        print(f"[错误] 用例根不像 tests/e2e: {用例根}")
        return 2
    工作根 = pathlib.Path(参.工作根)
    工作根.mkdir(parents=True, exist_ok=True)
    用例们 = [目录 for 目录 in sorted(用例根.iterdir())
              if 目录.is_dir()
              and (not 参.only or any(目录.name.startswith(段) for 段 in 参.only.split(",")))
              and ((目录 / "双编译对照.txt").exists() or (目录 / "v2闭环.txt").exists())]
    结果们 = []
    with ThreadPoolExecutor(max_workers=参.jobs) as 池:
        期货 = [池.submit(跑用例, v2p, 用例根, 用例, 工作根) for 用例 in 用例们]
        for 期货项 in 期货:
            结果们.append(期货项.result())
    坏们 = [r for r in 结果们 if r.get("状态") == "冲突"]
    好数 = sum(1 for r in 结果们 if r.get("状态") == "OK")
    print(f"扫描 v2 路径用例 {len(结果们)} 个（双编译对照/闭环配置面）")
    print(f"  产 asm 且符号面干净: {好数}")
    print(f"  v2p 未产 asm       : {len(结果们) - 好数 - len(坏们)}（负测/编译失败形态·不参与判据）")
    print(f"  ★冲突用例           : {len(坏们)}")
    for 项 in 坏们[:12]:
        print(f"    - {项['名称']}: 冲突={项['冲突'][:4]} 非法EXTERN={项['非法EXTERN'][:4]} 未解析={项['未解析'][:4]}")
    if len(坏们) > 12:
        print(f"    …（余 {len(坏们) - 12} 例）")
    if 好数 == 0:
        print("[错误] 无一用例产出 asm——v2p 或环境不可用")
        return 2
    print("结论：" + ("win 产物符号面干净 ✓" if not 坏们 else f"存在 {len(坏们)} 例冲突 ✗"))
    return 1 if 坏们 else 0


if __name__ == "__main__":
    sys.exit(主())
