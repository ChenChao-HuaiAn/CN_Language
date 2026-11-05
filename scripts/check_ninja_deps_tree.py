#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_ninja_deps_tree.py —— ninja .ninja_deps 跨树路径污染校验（构建后兜底）。

背景（#2103 病根）：sccache 缓存键对 MSVC 不含源文件绝对路径，跨树共享缓存
时异树编译的 stdout（含 /showIncludes 头文件路径=异树绝对路径）被本树构建
命中重放，ninja 把异树头路径写进本树 .ninja_deps → 头文件改动时 ninja 看
异树路径 mtime 未变 → 跳过重编 → 旧代码混编（LNK2019 假分叉/语义假象）。
配套防线两道：①缓存目录按树路径哈希分化（scripts/wt.py 树缓存目录·构建
前 stop-server 让分区归位）；②本脚本构建后校验 depslog——任何漏网（跨树
并行构建的启动交错窗口）当场红，把「静默混编」变成「显形构建失败」。

判据：depslog 中出现的路径若匹配任务树目录模式 `\\wt<数字>\\` 且不以
--tree 为前缀 → 跨树污染。第三方库/系统头（全机共用·无 wt 模式）不误伤。
路径提取对 depslog 结构免疫：路径记录=NUL 分隔的可打印 ASCII 串，直接
按 NUL 分段提取（版本演化不敏感）。

用法：
  python scripts/check_ninja_deps_tree.py [--build-dir target/build-ninja]
                                          [--tree <树路径>]
  python scripts/check_ninja_deps_tree.py --selftest   （最小正反自测）
退出码：0=干净；1=发现异树路径（红）；2=deps 文件缺失（视为干净·新树未构建）。
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# 任务树 worktree 目录模式：\wt<数字>\（wt.py create 建树惯例·wt + 台账号）
异树模式 = re.compile(r"[\\/]" + "wt" + r"\d{2,6}[\\/]")


def 提取路径(deps字节: bytes) -> list[str]:
    """从 depslog 原始字节提取候选路径（NUL 分段·取可打印 ASCII 串）。"""
    候选: list[str] = []
    for 段 in deps字节.split(b"\x00"):
        try:
            文本 = 段.decode("ascii")
        except UnicodeDecodeError:
            continue
        文本 = 文本.strip()
        # 路径形态：含路径分隔符且以源/头扩展名结尾（deps 记录的路径串）
        if (
            len(文本) >= 6
            and ("/" in 文本 or "\\" in 文本)
            and 文本.endswith((".cpp", ".hpp", ".h", ".cc", ".obj"))
        ):
            候选.append(文本)
    return 候选


def 校验(构建目录: Path, 本树: Path) -> tuple[int, list[str]]:
    """返回 (退出码, 异树路径列表)。deps 缺失=新树未构建视为干净。"""
    deps = 构建目录 / ".ninja_deps"
    if not deps.exists():
        return 2, []
    异树: list[str] = []
    本树规范 = str(本树.resolve())
    for 路径 in 提取路径(deps.read_bytes()):
        if 异树模式.search(路径) and not 路径.replace("/", "\\").startswith(
            本树规范.replace("/", "\\")
        ):
            异树.append(路径)
    return (1 if 异树 else 0), 异树


def selftest() -> int:
    """最小正反自测：手工编码 depslog 字节流（版本头+路径记录）正反两态。"""
    失败: list[str] = []

    def 断言(名: str, 条件: bool) -> None:
        print(f"[{'绿' if 条件 else '红'}] {名}")
        if not 条件:
            失败.append(名)

    def 编码记录(路径: str, mtime: int = 0) -> bytes:
        路径字节 = 路径.encode("ascii") + b"\x00"  # 记录串 NUL 结尾（观测格式）
        return len(路径字节).to_bytes(4, "little") + 路径字节 + mtime.to_bytes(8, "little")

    头 = b"# ninjadeps\n" + (4).to_bytes(4, "little")  # 魔数+版本
    本树 = "C:\\repo\\wt2100"

    # 正态：本树路径 + third-party 共用路径 + 相对路径 → 干净
    干净流 = (
        头
        + 编码记录(本树 + r"\src\cn_compiler\lexer\lexer.cpp.obj")
        + 编码记录(本树 + r"\src\cn_compiler\common\optional_safe.hpp")
        + 编码记录("C_/Users/x/Documents/third-party/unittest/googletest/src/gtest-all.cc")
        + 编码记录("CMakeFiles/cn.dir/src/main.cpp.obj")
    )
    码, 异树 = 校验字节流(干净流, 本树)
    断言("正态：本树+第三方+相对路径=干净", 码 == 0 and 异树 == [])

    # 反态：异树 wt2025 头路径混入 → 红
    污染流 = (
        头
        + 编码记录(本树 + r"\src\cn_compiler\lexer\lexer.cpp.obj")
        + 编码记录("C:\\repo\\wt2025\\src\\cn_compiler\\model\\types.hpp")
    )
    码, 异树 = 校验字节流(污染流, 本树)
    断言("反态：异树头路径=红且被点名", 码 == 1 and len(异树) == 1 and "wt2025" in 异树[0])

    # 边界：本树自身是任务树（wt2100）不得自伤
    自身流 = 头 + 编码记录(本树 + r"\src\a.hpp")
    码, 异树 = 校验字节流(自身流, 本树)
    断言("边界：本树 wt 路径不误报", 码 == 0)

    if 失败:
        print(f"[失败] {len(失败)} 项红：{'、'.join(失败)}")
        return 1
    print("[全绿] deps 校验正反边界三态通过")
    return 0


def 校验字节流(数据: bytes, 本树: str) -> tuple[int, list[str]]:
    """selftest 注入口：对内存字节流跑同一判据。"""
    异树: list[str] = []
    本树规范 = 本树.replace("/", "\\")
    for 路径 in 提取路径(数据):
        if 异树模式.search(路径) and not 路径.replace("/", "\\").startswith(本树规范):
            异树.append(路径)
    return (1 if 异树 else 0), 异树


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="ninja .ninja_deps 跨树路径污染校验")
    解析.add_argument("--build-dir", default="target/build-ninja")
    解析.add_argument("--tree", default=str(Path.cwd().resolve()))
    解析.add_argument("--selftest", action="store_true", help="内置最小正反自测")
    参数 = 解析.parse_args()
    if 参数.selftest:
        return selftest()
    码, 异树 = 校验(Path(参数.build_dir), Path(参数.tree))
    if 码 == 2:
        return 0  # 新树未构建·deps 未生成=干净
    if 码 == 1:
        print("[红] .ninja_deps 发现跨树路径污染（异树缓存重放实录）——", flush=True)
        for 路径 in 异树:
            print(f"  异树路径: {路径}", flush=True)
        print("  处置: 该构建产物 obj 不可信（可能跳编混编）——请清理重建：", flush=True)
        print("    rmdir /s /q target\\build-ninja  然后重跑构建（缓存按树分区·重编一次即净）", flush=True)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(主流程())
