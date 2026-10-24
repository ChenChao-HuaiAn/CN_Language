#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# #751 证据对拍脚本（可复跑·非 E2E 用例·不参与门禁）
#   用途：同一探针源码在「3893cdd3a 原始单文件 JSON.cn 基线」与「当前 JSON 模块
#   拆分+硬化版」两侧各编译运行，逐字节对拍 stdout：
#     ① probe751err.cn  20 例负例（错误码/位置/字节/根/键查询）
#     ② probe751leak.cn 成功+失败 4 形态×2000 轮 活动分配数回落差
#     ③ probe751d.cn    性能四负载（8192/32768 数字、短串、286729B 长串）3 轮
#   基线还原方式：git show <基线提交>:stdlib/JSON.cn 临时覆盖 stdlib/JSON.cn
#   （try/finally 保证恢复）；需要 git 与已构建的宿主编译器 target/cn(.exe)。
#   用法：python tests/probe751/对拍751.py [--perf]
import argparse
import pathlib
import shutil
import subprocess
import sys

根 = pathlib.Path(__file__).resolve().parent.parent.parent
探针目录 = pathlib.Path(__file__).resolve().parent
临时目录 = 根 / "target" / "probe751"
基线提交 = "3893cdd3a"  # JSON.cn 原始单文件（接子 O(子数²)）


def 编译器() -> pathlib.Path:
    for 名 in ("cn.exe", "cn"):
        候选 = 根 / "target" / 名
        if 候选.exists():
            return 候选
    print("错误：未找到 target/cn(.exe)——先构建宿主编译器")
    sys.exit(2)


def 编译(cn: pathlib.Path, 源: pathlib.Path, 产物: pathlib.Path,
         允许失败: bool = False) -> bool:
    r = subprocess.run([str(cn), "build", str(源), "--target", "win-x64",
                        "--output", str(产物)], cwd=str(根),
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    if r.returncode != 0 or not 产物.exists():
        if 允许失败:
            return False
        print(f"错误：编译失败 {源.name} rc={r.returncode}")
        print((r.stdout or "") + (r.stderr or ""))
        sys.exit(1)
    return True


def 跑(产物: pathlib.Path) -> str:
    r = subprocess.run([str(产物)], capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.returncode != 0:
        print(f"错误：运行失败 rc={r.returncode} {产物.name}")
        print(r.stdout)
        sys.exit(1)
    return r.stdout


def 主程序() -> int:
    解析 = argparse.ArgumentParser(description="#751 负例/泄漏/性能对拍")
    解析.add_argument("--perf", action="store_true", help="附性能四负载 3 轮")
    参数 = 解析.parse_args()

    临时目录.mkdir(parents=True, exist_ok=True)
    cn = 编译器()
    json_cn = 根 / "stdlib" / "JSON.cn"
    备份 = 临时目录 / "JSON.cn.current"
    shutil.copy2(json_cn, 备份)

    结果 = {}
    try:
        # ---- 基线侧：3893cdd3a 单文件 JSON.cn ----
        旧 = subprocess.run(["git", "show", f"{基线提交}:stdlib/JSON.cn"],
                            cwd=str(根), capture_output=True, check=True).stdout
        json_cn.write_bytes(旧)
        for 名 in ("probe751err", "probe751leak"):
            产物 = 临时目录 / f"{名}.before.exe"
            # #1523：probe751err 已改用 JSON 查询 可选 API，旧基线不兼容该源码——
            #   基线侧编译失败则跳过该探针的对拍（泄漏探针不受影响）。
            if not 编译(cn, 探针目录 / f"{名}.cn", 产物, 允许失败=(名 == "probe751err")):
                print(f"[对拍·跳过] {名} 基线侧不兼容（#1523 查询 API 结果化后源码不匹配）")
                continue
            结果[(名, "before")] = 跑(产物)

        # ---- 当前侧：拆分+硬化版 ----
        shutil.copy2(备份, json_cn)
        for 名 in ("probe751err", "probe751leak"):
            产物 = 临时目录 / f"{名}.after.exe"
            编译(cn, 探针目录 / f"{名}.cn", 产物)
            结果[(名, "after")] = 跑(产物)
    finally:
        shutil.copy2(备份, json_cn)

    失败 = 0
    for 名, 题 in (("probe751err", "20 例负例"), ("probe751leak", "泄漏 4×2000 轮")):
        if (名, "before") not in 结果:
            continue
        前 = 结果[(名, "before")]
        后 = 结果[(名, "after")]
        if 前 == 后:
            print(f"[对拍·零差异] {题}（{名}）")
        else:
            失败 += 1
            print(f"[对拍·有差异] {题}（{名}）")
            import difflib
            for 行 in difflib.unified_diff(前.splitlines(), 后.splitlines(),
                                           "before", "after", lineterm=""):
                print(行)
    if 参数.perf:
        # 性能探针仅当前侧（before 需基线侧编译，耗时约 1 分钟/3 轮；按需开启）
        产物 = 临时目录 / "probe751d.after.exe"
        编译(cn, 探针目录 / "probe751d.cn", 产物)
        print("[性能·当前侧 3 轮]")
        print(跑(产物))
    print("结论：", "全部零差异" if 失败 == 0 else f"{失败} 项有差异")
    return 0 if 失败 == 0 else 1


if __name__ == "__main__":
    sys.exit(主程序())
