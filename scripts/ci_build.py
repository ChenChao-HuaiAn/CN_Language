#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_build.py —— 两平台构建链（配置/构建/单测·973 自 ci_daemon.py 原样外迁）。

973（2026-10-17）外迁缘由：ci_daemon.py 达 998 行（AGENTS §3 警戒线 900 行上「先拆
再加」为硬纪律；578 运行体/660 自测体先例）——对账周检挂载需在 daemon 增挂点，先
把「项量最大且职责单一」的两平台构建链整段原样搬出。逻辑与搬出前**逐行等价**：
  291 win 构建链=vcvars64+Ninja+sccache（wt.py 开发树同口径）；523 混合态治本=HEAD
  指纹清建（rmdir 整目录重建）+ 088 .ninja_deps 删除；657 linux 构建段（与 win
  对称·配置/构建/单测三段·integrate 同口径）。
调用面（daemon 薄壳/别名，原样搬运不改行为——578 先例）：
  · 超时/并行两常量保持 daemon 单源（911 可调常量集中区）：win构建链/linux构建链
    以参数注入（单轮总超时秒 / 构建并行），本模块内部逐行代码不变。
  · 树根/日志目录本模块自推导（与 ci_daemon 同式·__file__ 上两级——同树同值）。

1299（2026-10-21）：linux 构建段补编译器缓存（ccache/sccache 自动探测）——缘由=门禁表
构建段 63s（家机 WSL 4 作业）↔372s（深度机 2 核 2 作业）差异中「预验 637 清产物后全量
冷编」是常态成本；win 侧早由 sccache（291）吸收 523 清建成本，linux 侧此前零缓存=两
平台构建配置不对等。预验「干净构建」语义不变（target/ 仍全清、CMake 从零配置；缓存
只复用「预处理后源码+编译器+编译选项」哈希完全一致的编译结果，等价 win sccache 先例），
告警文本照常回放（ccache 命中会重放 stderr）。CN_BUILD_CACHE 控制：未设/auto=探测
ccache→sccache→不启用；off/0/false/no/关闭/否=显式关闭；其他值=原样作 launcher 路径。
"""
from __future__ import annotations

import os
import shutil
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
日志目录 = 仓库根 / "ci-logs"


def 找vcvars() -> Path | None:
    """vcvars64.bat 探测（291·复刻 wt.py 同款四版本候选）。"""
    for 版本 in ("Community", "Professional", "Enterprise", "BuildTools"):
        候选 = Path("C:/Program Files/Microsoft Visual Studio/2022/%s/VC/Auxiliary/Build/vcvars64.bat" % 版本)
        if 候选.exists():
            return 候选
    return None


# ═══════════════ 523 win 构建链（混合态治本·指纹清建+088 对齐） ═══════════════

def 需指纹清建(当前头: str, 记录路径: Path | None = None) -> tuple[bool, str]:
    """HEAD 指纹清建判定（523·治本第一道）：比对实际 HEAD 与上次成功构建指纹
    （ci-logs/last_build.sha·各实例独立）。不等/缺失/损坏/空头→清建（fail-safe
    保守：宁可多清一次全编——sccache 命中约 30s·漏清=随机红重跑更贵）。
    返回 (是否清建, 缘由)。记录路径参数供 --selftest 隔离注入，生产用缺省。"""
    if not 当前头:
        return True, "HEAD 不可知（rev-parse 失败/空）→保守清建"
    记录 = 记录路径 or (日志目录 / "last_build.sha")
    try:
        上次 = 记录.read_text(encoding="utf-8").strip()
    except Exception:
        return True, "无构建指纹（冷启动/部署升级/上轮未绿）→保守清建"
    if 上次 != 当前头:
        return True, "HEAD 变化（上=%s→现=%s）→清建防混合态" % (上次[:10], 当前头[:10])
    return False, "指纹一致→保留增量"


def win头行(vcvars: Path) -> list[str]:
    """win_configure.cmd / win_build.cmd 公共头（291 原内联段提出·523 抽函数）。"""
    return ["@echo off", 'cd /d "%~dp0.."',
            'call "%s" >nul 2>&1' % vcvars,
            'where cl >nul 2>&1 || (echo [FAIL] cl not found & exit /b 1)']


def win配置行(vcvars: Path, 清建: bool) -> list[str]:
    """win_configure.cmd 生成（291 原内联段提出）。清建形态（523）=先 rmdir 整个
    build-ninja——与「干净重建即愈」完全同构=治本；半清（只删 obj/.ninja_*）无法
    论证穷尽残留面（.ninja_log 时间戳/生成文件/rsp/PDB·ILK 链接中间态均可陈旧），
    留随机红尾巴。cmd 文本必须 ASCII（encoding="ascii"·rem 注释用英文）。"""
    行 = win头行(vcvars)
    if 清建:
        行 += ["rem 523 root-fix: HEAD fingerprint changed -> full clean rebuild",
               "rem (mirrors the known-good fix: clean rebuild; sccache absorbs cost)",
               "if exist target\\build-ninja rmdir /s /q target\\build-ninja"]
    launcher = ""
    sccache = shutil.which("sccache")
    if sccache:
        launcher = ' -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="%s"' % sccache
    # CXX_FLAGS_DEBUG 必须显式 /Z7（wt.py 开发树同口径）——默认 /ZI 集中 PDB
    # 在 --parallel 下多 cl.exe 并写同 .pdb 必撞 C1041（实测）
    行.append('cmake -G Ninja -S . -B target/build-ninja '
              '-DCMAKE_BUILD_TYPE=Debug '
              '-DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1" ' + launcher)
    return 行


def win构建行(vcvars: Path) -> list[str]:
    """win_build.cmd 生成（291 原内联段提出+523 对齐 wt.py 开发树 088 修复）：
    每轮构建前删 .ninja_deps——MSVC+Ninja 的 header deps 经 /showIncludes 记录，
    sccache（CMAKE_CXX_COMPILER_LAUNCHER）可能吞其 stdout→陈旧 deps→该重编的
    没重编→ODR 混合对象→静默崩溃；删后强制全量重扫头（成本由 sccache 命中吸收
    ·wt.py 实测冷全量 ~23s）。"""

    return win头行(vcvars) + [
        "rem 088 root-fix: drop stale .ninja_deps so every build re-scans all headers.",
        "rem MSVC+Ninja records header deps from /showIncludes stdout, which sccache",
        "rem (CMAKE_CXX_COMPILER_LAUNCHER) may swallow -> stale deps -> ODR mixed",
        "rem objects -> silent crash. Deleting .ninja_deps forces a full re-scan",
        "rem (cost absorbed by sccache hit; measured cold full build ~23s).",
        "if exist target\\build-ninja\\.ninja_deps del /q target\\build-ninja\\.ninja_deps",
        "cmake --build target/build-ninja --parallel",
        "if errorlevel 1 (echo [FAIL] build & exit /b 1)"]


def win构建链(步骤们: dict, 步骤, 日志, vcvars: Path, 当前头: str,
              单轮总超时秒: int) -> None:
    """win 构建链整段（291 原内联于 跑一轮·523 抽出=跑一轮 拆函数+指纹挂点）：
    配置（指纹不一致→先 rmdir 全清再重配）→构建（每轮先删 .ninja_deps）→单测。
    指纹写盘口径：构建 rc==0 **且单测 rc==0** 才更新 last_build.sha——构建绿但
    单测挂死/红=产物可疑（可能混合态·554 症候）→不更新，下轮重试必清建
    （同夜两轮均坏、第二轮即愈的对症面；真实红缺陷重试多付 ~30s 清建=可接受）。
    973 外迁：单轮总超时秒 由调用方注入（daemon 常量单源·911）。"""
    清建, 缘由 = 需指纹清建(当前头)
    print("  [523] %s" % 缘由, file=日志, flush=True)
    if 清建 or not (仓库根 / "target/build-ninja/build.ninja").exists():
        配置脚本 = 日志目录 / "win_configure.cmd"
        配置脚本.write_text("\r\n".join(win配置行(vcvars, 清建)) + "\r\n", encoding="ascii")
        步骤("配置", ["cmd", "/c", str(配置脚本)], 900)
    else:
        步骤们["配置"] = {"rc": 0, "说明": "build-ninja 已配置（增量）"}
    if 步骤们["配置"].get("rc") != 0:
        return
    构建脚本 = 日志目录 / "win_build.cmd"
    构建脚本.write_text("\r\n".join(win构建行(vcvars)) + "\r\n", encoding="ascii")
    步骤("构建", ["cmd", "/c", str(构建脚本)], 单轮总超时秒)
    # 单测（产物落 target/ 根=CMAKE_RUNTIME_OUTPUT_DIRECTORY·exe 后缀）
    单测 = next((p for p in [仓库根 / "target/cn_unit_tests.exe",
                             仓库根 / "target/build/cn_unit_tests.exe",
                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
    if 单测 is None:
        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
        return
    步骤("单测", [str(单测)], 1800)
    if 步骤们["构建"]["rc"] == 0 and 步骤们["单测"]["rc"] == 0:
        (日志目录 / "last_build.sha").write_text(当前头 + "\n", encoding="utf-8")
        print("  [523] 构建指纹已更新：%s" % 当前头[:10], file=日志, flush=True)


# ═══════════ 1299 linux 编译器缓存（ccache/sccache·预验冷建成本） ═══════════
缓存关闭词 = frozenset(("off", "0", "false", "no", "关闭", "否"))


def 选编译器缓存(环境=None, 探测器=None) -> str | None:
    """linux 构建编译器缓存选择（1299·纯函数可注入自测）。

    CN_BUILD_CACHE 语义（运维旋钮·缺省 auto）：
      · 未设 / auto      → 依次探测 ccache、sccache，命中返回路径；都无→None（构建照旧）；
      · off/0/false/no/关闭/否 → None（显式关闭，即使已安装也不用）；
      · 其他非空值       → 原样返回（显式 launcher 路径/包装器名，存在性交给 cmake 报错）。
    返回 None=不启用（**可选**语义，非「不存在」哨兵：调用方据此显式传空 launcher，
    防 CMakeCache 残留旧 launcher 造成「已卸载缓存仍被调用」的构建红）。"""
    环境 = os.environ if 环境 is None else 环境
    指定 = (环境.get("CN_BUILD_CACHE") or "").strip()
    if 指定.lower() in 缓存关闭词:
        return None
    if 指定 and 指定.lower() != "auto":
        return 指定
    探测器 = 探测器 or shutil.which
    for 名 in ("ccache", "sccache"):
        路径 = 探测器(名)
        if 路径:
            return 路径
    return None


def linux配置命令(launcher: str | None) -> list[str]:
    """linux cmake 配置命令（1299 抽出·正反两态自测锚）：make 增量口径不变，只额外
    显式传入 CMAKE_CXX_COMPILER_LAUNCHER——launcher=None 也传空值（覆盖 CMakeCache
    里可能残留的旧 launcher；实测空 :STRING= 配置合法）。"""
    return ["cmake", "-S", ".", "-B", "target/build",
            "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=%s" % (launcher or "")]


def linux构建链(步骤们: dict, 步骤, 构建并行: int, 单轮总超时秒: int,
               日志=None) -> None:
    """linux 构建段整段（657 自 跑一轮 抽出·与 win构建链 对称=跑一轮 回 ≤100 行
    纪律）：②配置（默认生成器 Make·与 integrate 同口径；产物落 target/ 根）→构建→
    ③单测（产物三 fallback=integrate 同款）。逻辑与抽出前逐行等价（零行为变化）。
    973 外迁：构建并行/单轮总超时秒 由调用方注入（daemon 常量单源·911）。
    1299：配置前探测编译器缓存（ccache/sccache）并显式传入 launcher；日志参数可选
    （None=不额外输出·兼容自测桩调用面）。"""
    launcher = 选编译器缓存()
    if 日志 is not None:
        print("  [1299] 编译器缓存：%s" % (launcher or "未启用（ccache/sccache 均未安装·构建照旧）"),
              file=日志, flush=True)
    if not 步骤("配置", linux配置命令(launcher), 600):
        return  # 配置红——步骤们 已记 rc，构建/单测不跑（原 pass 分支语义）
    if not 步骤("构建", ["cmake", "--build", "target/build",
                        "--parallel", str(构建并行)], 单轮总超时秒):
        return  # 构建红——单测不跑（原 elif 语义）
    单测 = next((p for p in [仓库根 / "target/build/tests/unit/cn_unit_tests",
                             仓库根 / "target/build/cn_unit_tests",
                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
    if 单测 is None:
        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
    else:
        步骤("单测", [str(单测)], 1800)
