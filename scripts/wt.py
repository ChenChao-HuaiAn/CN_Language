#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""wt.py —— 本机多开 worktree 任务池管理（449-a·AGENTS.md §8.8）。

同机并行开发互踩的根因=多会话共享一个工作树与 target/（429-a/431-a 实录：
checkout 带走未提交内容 / E2E 中途 expected 消失 / cn.exe 占用 LNK1104）。
本脚本把「每任务一个 worktree」制度化：

  create <轮次> <标识>   建树+任务分支（fetch→develop 基准→gtest 两级深度校验→Ninja+sccache 开发树配置）
  list                   列出全部 worktree（分支/干净度/target 占用）
  remove <轮次>          删树（分支保留；--delete-branch 仅当已并入 develop 才删分支）
  cache [stats|start|stop|clear]   sccache 缓存服务快捷操作

实测口径（449-a·win/MSVC）：
  - VS 生成器（vcxproj ClCompile 原生任务）忽略 CMAKE_CXX_COMPILER_LAUNCHER——sccache 只能走 Ninja；
  - Ninja+MSVC 必须 /Z7 替代 /Zi（默认 /Zi 多 cl 并发写同一 PDB=C1041，加 /FS 也拦不住）；
  - 勿用 -DCMAKE_CXX_FLAGS 覆盖（会顶掉默认 /EHsc→C4530 被 /WX 拦成 C2220，且 CMakeCache 残留）；
  - 计时：VS --parallel 冷全量 127s｜Ninja+sccache 冷 23s｜同树删树重建 10.7s｜跨树首建 ~21s（48% 命中：
    共享路径的第三方〔gtest/运行时〕命中，树内绝对路径的项目源码 miss）。
"""
from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent


def 运行(命令: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(命令, capture_output=True, text=True, encoding="utf-8", errors="replace", **kwargs)


def 输出(命令: list[str]) -> str:
    结果 = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=本树根)
    return (结果.stdout or "").strip()


def 主树根() -> Path:
    """共享 .git 所在的主工作树根（worktree 下 git-common-dir 指回主树 .git）。"""
    共同目录 = 输出(["git", "rev-parse", "--git-common-dir"])
    路径 = Path(共同目录) if Path(共同目录).is_absolute() else (本树根 / 共同目录)
    return 路径.resolve().parent


def 机器名() -> str:
    系统 = platform.system()
    if 系统 == "Windows":
        return "家机"
    架构 = platform.machine().lower()
    return "单位机" if 架构 in ("aarch64", "arm64") else "深度机"


def 找sccache() -> Path | None:
    """探测 sccache：PATH → winget 包目录（新装 PATH 未刷新的会话）。"""
    命中 = shutil.which("sccache")
    if 命中:
        return Path(命中)
    包根 = Path.home() / "AppData/Local/Microsoft/WinGet/Packages"
    if 包根.exists():
        for 候选 in sorted(包根.glob("Mozilla.sccache*/sccache-v*/sccache.exe")):
            return 候选
    return None


def 找vcvars() -> Path | None:
    for 版本 in ("Community", "Professional", "Enterprise", "BuildTools"):
        候选 = Path(f"C:/Program Files/Microsoft Visual Studio/2022/{版本}/VC/Auxiliary/Build/vcvars64.bat")
        if 候选.exists():
            return 候选
    return None


def 找ninja() -> Path | None:
    命中 = shutil.which("ninja")
    if 命中:
        return Path(命中)
    候选 = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/"
                "CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
    return 候选 if 候选.exists() else None


设置脚本模板 = """@echo off
rem wt.py: Ninja+sccache dev-tree setup (ASCII only - cmd batch must not contain CJK)
call "{vcvars}" >nul 2>&1
where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)
cd /d {树}
set SCCACHE_DIR={缓存目录}
set SCCACHE_CACHE_SIZE={缓存上限}
cmake -G Ninja -S . -B target\\build-ninja -DCMAKE_MAKE_PROGRAM="{ninja}" -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="{sccache}" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1"
if errorlevel 1 (echo [FAIL] cmake configure & exit /b 1)
echo [OK] ninja tree configured
"""

构建脚本模板 = """@echo off
rem wt.py: Ninja+sccache incremental build (ASCII only)
call "{vcvars}" >nul 2>&1
where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)
cd /d {树}
set SCCACHE_DIR={缓存目录}
set SCCACHE_CACHE_SIZE={缓存上限}
cmake --build target\\build-ninja --parallel
if errorlevel 1 (echo [FAIL] build & exit /b 1)
echo [OK] ninja build done
"""


def 建树(轮次: str, 标识: str, 基准: str | None, 无ninja: bool) -> int:
    远程 = 基准 or "gitcode/develop"
    运行(["git", "fetch", "gitcode"])
    分支 = f"任务/{机器名()}-{轮次}-{标识}"
    树路径 = 主树根().parent / f"wt{轮次}"
    if 树路径.exists():
        print(f"[失败] {树路径} 已存在——同名 worktree 或残留，先 wt.py remove {轮次}")
        return 1
    if 输出(["git", "rev-parse", "--verify", f"refs/heads/{分支}"]):
        print(f"[失败] 本地分支 {分支} 已存在")
        return 1
    print(f"[1] git worktree add {树路径.name} -b {分支} {远程}")
    结果 = 运行(["git", "worktree", "add", str(树路径), "-b", 分支, 远程])
    if 结果.returncode != 0:
        print(f"[失败] {结果.stderr}")
        return 1

    gtest = 树路径 / "../../third-party/unittest/googletest/src/gtest-all.cc"
    if gtest.exists():
        print("[2] gtest 两级深度校验通过（../../third-party 可达，与主树共用同一份）")
    else:
        print("[警告] gtest 相对路径不可达（构建将回退 target/build/_deps，需先前拉取过）")

    sccache = 找sccache()
    if 无ninja or sccache is None:
        原因 = "--no-ninja 指定" if 无ninja else "sccache 未安装（winget install Mozilla.sccache）"
        print(f"[3] 跳过 Ninja 开发树配置（{原因}）——门禁用 VS 构建（build.ps1）不受影响")
    elif platform.system() != "Windows":
        ninja = shutil.which("ninja")
        if ninja is None:
            print("[3] 跳过：linux 侧未装 ninja（apt install ninja-build 后重跑 create 可补）")
        else:
            配置 = subprocess.run(["cmake", "-G", "Ninja", "-S", ".", "-B", "target/build-ninja",
                                   f"-DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH={sccache}",
                                   "-DCMAKE_BUILD_TYPE=Debug"], cwd=树路径)
            print(f"[3] Ninja 开发树配置{'完成' if 配置.returncode == 0 else '失败'}（build: "
                  f"cmake --build target/build-ninja --parallel）")
    else:
        vcvars, ninja = 找vcvars(), 找ninja()
        if vcvars is None or ninja is None:
            print("[3] 跳过：未找到 vcvars64.bat 或 ninja.exe（VS2022 安装不完整）")
        else:
            缓存目录 = os.environ.get("SCCACHE_DIR", str(Path.home() / "Documents/sccache-cache"))
            缓存上限 = os.environ.get("SCCACHE_CACHE_SIZE", "5G")
            公共 = dict(树=str(树路径), 缓存目录=缓存目录, 缓存上限=缓存上限)
            (树路径 / "target").mkdir(exist_ok=True)
            (树路径 / "target/ninja_setup.cmd").write_text(
                设置脚本模板.format(vcvars=vcvars, ninja=ninja, sccache=sccache, **公共), encoding="ascii")
            (树路径 / "target/ninja_build.cmd").write_text(
                构建脚本模板.format(vcvars=vcvars, **公共), encoding="ascii")
            配置 = subprocess.run(["cmd", "/c", "target\\ninja_setup.cmd"], cwd=树路径,
                                  capture_output=True, text=True)
            状态 = "完成（build: cmd /c target\\ninja_build.cmd）" if 配置.returncode == 0 \
                else f"失败——{配置.stdout and 配置.stdout.splitlines()[-1]}"
            print(f"[3] Ninja+sccache 开发树配置{状态}")

    print(f"""
[完成] {树路径}（分支 {分支}）
  下一步（AGENTS.md §8.1/§8.8）：①看板本机行直推（含分支名）②plans/025 六要素落盘
  ③push 分支到 gitcode=认领生效 ④门禁照旧 VS 口径（ci.ps1/build.ps1，已含 gate_lock 串行锁）""")
    return 0


def 列树() -> int:
    结果 = 输出(["git", "worktree", "list", "--porcelain"])
    树们: list[tuple[str, str]] = []
    路径 = ""
    for 行 in 结果.splitlines():
        if 行.startswith("worktree "):
            路径 = 行[9:]
        elif 行.startswith("branch ") and 路径:
            树们.append((路径, 行[7:].replace("refs/heads/", "")))
        elif 行.startswith("detached") and 路径:
            树们.append((路径, "(detached)"))
    表头 = f"{'分支':<44} {'树':<16} 状态"
    print(表头)
    print("-" * 74)
    for 树路径, 分支 in sorted(树们, key=lambda x: x[0]):
        状态 = 运行(["git", "-C", 树路径, "status", "--porcelain"])
        脏 = f"脏×{len(状态.stdout.splitlines())}" if 状态.stdout.strip() else "净"
        print(f"{分支:<44} {Path(树路径).name:<16} {脏}")
    return 0


def 删树(轮次: str, 删分支: bool) -> int:
    树路径 = 主树根().parent / f"wt{轮次}"
    if not 树路径.exists():
        print(f"[失败] {树路径} 不存在")
        return 1
    分支 = 运行(["git", "-C", str(树路径), "branch", "--show-current"]).stdout.strip()
    print(f"[1] 删除 {树路径}" + (f"（分支 {分支}）" if 分支 else ""))
    shutil.rmtree(树路径, ignore_errors=True)
    运行(["git", "worktree", "prune"])
    print("[2] git worktree prune 完成")
    if 删分支 and 分支:
        运行(["git", "fetch", "gitcode"])
        并入 = 运行(["git", "merge-base", "--is-ancestor", 分支, "gitcode/develop"])
        if 并入.returncode != 0:
            print(f"[3] [拒绝] 分支 {分支} 未并入 develop——保留分支（确认后手动 git branch -D）")
        else:
            独有提交 = 输出(["git", "rev-list", "--count", "gitcode/develop..{分支}"])
            运行(["git", "branch", "-d", 分支])
            说明 = "已并入 develop（含独有提交）" if int(独有提交 or 0) > 0 else "空分支（无独有提交·刚建即删无损失）"
            print(f"[3] 分支 {分支} {说明}，本地分支已删")
    elif 分支:
        print(f"[3] 分支 {分支} 保留（--delete-branch 可在已并入时删除）")
    return 0


def 缓存(动作: str) -> int:
    sccache = 找sccache()
    if sccache is None:
        print("[失败] sccache 未安装（winget install Mozilla.sccache）")
        return 1
    环境 = dict(os.environ, SCCACHE_DIR=os.environ.get("SCCACHE_DIR", str(Path.home() / "Documents/sccache-cache")))
    子命令 = {"stats": ["--show-stats"], "start": ["--start-server"], "stop": ["--stop-server"],
              "clear": ["--stop-server"]}[动作]
    结果 = subprocess.run([str(sccache), *子命令], env=环境)
    if 动作 == "clear" and 结果.returncode == 0:
        目录 = Path(环境["SCCACHE_DIR"])
        shutil.rmtree(目录, ignore_errors=True)
        print(f"[完成] 缓存目录已清（{目录}）")
    return 结果.returncode


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="本机多开 worktree 任务池管理（AGENTS.md §8.8）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p建 = 子.add_parser("create", help="建树+任务分支（含 Ninja+sccache 开发树）")
    p建.add_argument("轮次")
    p建.add_argument("标识")
    p建.add_argument("--base", default=None, help="基准 ref（默认 gitcode/develop）")
    p建.add_argument("--no-ninja", action="store_true", help="跳过 Ninja 开发树配置")
    p列 = 子.add_parser("list", help="列出全部 worktree")
    p删 = 子.add_parser("remove", help="删树（win 下 rm -rf+prune）")
    p删.add_argument("轮次")
    p删.add_argument("--delete-branch", action="store_true", help="分支已并入 develop 时一并删分支")
    p缓 = 子.add_parser("cache", help="sccache 缓存服务")
    p缓.add_argument("动作", choices=["stats", "start", "stop", "clear"], nargs="?", default="stats")
    参数 = 解析器.parse_args()
    if 参数.命令 == "create":
        return 建树(参数.轮次, 参数.标识, 参数.base, 参数.no_ninja)
    if 参数.命令 == "list":
        return 列树()
    if 参数.命令 == "remove":
        return 删树(参数.轮次, 参数.delete_branch)
    return 缓存(参数.动作)


if __name__ == "__main__":
    sys.exit(主流程())
