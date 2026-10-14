#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""exe新鲜度 —— 宿主编译器 exe 新鲜度机械核验（795·教训 #177 消费面机制化）。

病根（598 十五小时假红死循环·#177 实锤）：rebase/挂新父改变分支基（源码语义）
但二进制产物 mtime 不变——E2E 选择器按 mtime 选 exe 只保证「最近构建」，
不保证「构建对应当前基」。池 runner ci_daemon rebase 后沿用树内旧 exe 跑预验
→宿主侧假红→踢出→再 rebase→再假红。同族教训 #175（陈旧 exe 假绿一晚两犯——
假绿比假红更危险，它反向洗白定性）。

既有防线与互补：523 的 HEAD 指纹清建（ci_daemon 构建面·防混合态）+v2p/rt
指纹缓存（run_e2e v2 面·防陈旧 obj）——宿主 exe 的**消费面**此前零核验，
本模块补上最后一道。

核验判据（任一命中=陈旧·与 523 同款 fail-safe 保守）：
  a. exe mtime < 当前 HEAD 提交时刻（git log -1 --format=%ct）——rebase 挂新父
     必前移 HEAD 时刻→必拦（598 场景机械覆盖）；HEAD 不可知（非 git/命令失败）
     →保守按陈旧；
  b. 构建输入面（src/ 全树 .cpp/.hpp/.h + 根 CMakeLists.txt）任一 mtime >
     exe mtime——工作区未提交修改未编入（#175 主树实锤面）。
     误报面=同内容 checkout/touch 刷 mtime→多重建一次（sccache 命中约 30s），
     523 同口径「宁可多清一次全编——漏清=随机红重跑更贵」。

取舍备档（任务行）：嵌入式指纹（构建时注入 HEAD sha 到 exe 资源段）可免疫
touch 类误报，但需改双平台构建系统（win 资源 rc/linux 链接段）+dirty 树语义
处理，代价与收益不成比例——取 mtime+HEAD 时刻比对（零构建系统改动）。

处置（任务书口径）：--cn 显式指定=跳过核验（用户自担新鲜度）；自动探测命中
陈旧=默认走既有构建通道强制重建→重探测→复验；不可重建（构建失败/仍陈旧）
=显式红 exit 2 并指路（--cn 显式指定或手动构建）。
"""
import pathlib
import shutil
import subprocess
import sys
import time

# vcvars64.bat 四版本候选（复刻 wt.py/ci_daemon 同款·win 重建通道用）
_VCVERS版本们 = ("Community", "Professional", "Enterprise", "BuildTools")

# 构建输入面：src/ 下参与宿主编译的源扩展 + 根构建描述文件
_SRC扩展们 = (".cpp", ".hpp", ".h")


def 找vcvars() -> pathlib.Path | None:
    """vcvars64.bat 探测（wt.py/ci_daemon 同款四版本候选·win 重建通道）。"""
    for 版本 in _VCVERS版本们:
        候选 = pathlib.Path(
            f"C:/Program Files/Microsoft Visual Studio/2022/{版本}/VC/Auxiliary/Build/vcvars64.bat")
        if 候选.exists():
            return 候选
    return None


def 找ninja() -> pathlib.Path | None:
    """ninja 探测（wt.py 同款：PATH 优先·VS 自带版兜底）——配置分支必需
    （cmake -G Ninja 依赖·本机 PATH 常无 ninja，VS 组件版不在 PATH）。"""
    命中 = shutil.which("ninja")
    if 命中:
        return pathlib.Path(命中)
    候选 = pathlib.Path("C:/Program Files/Microsoft Visual Studio/2022/Community/"
                        "Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
    return 候选 if 候选.exists() else None


def 取HEAD提交时刻(项目根: pathlib.Path) -> int | None:
    """当前 HEAD 提交时刻（Unix 秒）；不可知返回 None（调用方保守按陈旧）。"""
    try:
        结果 = subprocess.run(["git", "log", "-1", "--format=%ct"], cwd=str(项目根),
                              capture_output=True, text=True, timeout=15)
        文本 = (结果.stdout or "").strip()
        if 结果.returncode == 0 and 文本.isdigit():
            return int(文本)
    except Exception:
        pass
    return None


def 构建输入最新时刻(项目根: pathlib.Path) -> tuple[int, str]:
    """构建输入面（src/ 全树 C++ 源+根 CMakeLists.txt）最新 mtime 与代表文件。

    返回 (最新时刻, 代表文件相对路径)——代表文件=取到最新值的那个（诊断可读）；
    面为空（无 src 树·异常态）返回 (0, "")=判据 b 恒不命中。"""
    最新 = 0.0
    代表 = ""
    src根 = 项目根 / "src"
    候选们 = list(src根.rglob("*")) if src根.exists() else []
    候选们.append(项目根 / "CMakeLists.txt")
    for f in 候选们:
        try:
            if not f.is_file() or f.suffix not in _SRC扩展们:
                continue
            时刻 = f.stat().st_mtime
            if 时刻 > 最新:
                最新, 代表 = 时刻, str(f.relative_to(项目根))
        except OSError:
            continue
    return 最新, 代表


def 核验新鲜度(exe路径: pathlib.Path, 项目根: pathlib.Path) -> tuple[bool, str]:
    """宿主 exe 新鲜度核验。返回 (是否新鲜, 缘由描述)——缘由供人读与日志留痕。"""
    if not exe路径.exists():
        return False, f"exe 不存在: {exe路径}"
    exe时刻 = exe路径.stat().st_mtime
    exe时刻文本 = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(exe时刻))
    头时刻 = 取HEAD提交时刻(项目根)
    if 头时刻 is None:
        return False, ("HEAD 提交时刻不可知（非 git 仓库/命令失败）→保守按陈旧"
                       "（fail-safe·523 同款：漏拦=假红死循环，多拦=重建一次）")
    头文本 = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(头时刻))
    if exe时刻 < 头时刻:
        return False, (f"exe 构建时刻 {exe时刻文本} 早于当前 HEAD 提交时刻 {头文本}"
                       f"——rebase/挂新父后旧 exe 保留旧行为（598 假红死循环实锤面·#177）")
    源时刻, 源代表 = 构建输入最新时刻(项目根)
    if 源时刻 > exe时刻:
        源文本 = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(源时刻))
        return False, (f"构建输入 {源代表} 时刻 {源文本} 晚于 exe 构建时刻 {exe时刻文本}"
                       f"——工作区源码修改未编入（#175 假绿实锤面）")
    return True, f"新鲜（exe {exe时刻文本} ≥ HEAD 提交时刻与全部构建输入）"


def 探测最新候选(候选们: list, 项目根: pathlib.Path) -> pathlib.Path | None:
    """既有探测语义（556：多候选取 mtime 最新）·无存在候选返回 None。"""
    存在们 = [p.resolve() for p in (项目根 / c for c in 候选们) if p.exists()]
    if not 存在们:
        return None
    return max(存在们, key=lambda p: p.stat().st_mtime)


def _win重建命令(项目根: pathlib.Path) -> list[list[str]] | None:
    """win 重建命令序列（cmd /c 执行·vcvars 缺失返回 None）。

    与 ci_daemon 523/088 同口径：build-ninja 已配置→增量构建（先删 .ninja_deps
    强制全量重扫头·088）；未配置→先配置（sccache 可选探测·/Z7 防 C1041）再构建。"""
    vcvars = 找vcvars()
    if vcvars is None:
        return None
    已配置 = (项目根 / "target" / "build-ninja" / "build.ninja").exists()
    行 = ["@echo off",
          'cd /d "%~dp0.."',
          'call "%s" >nul 2>&1' % vcvars,
          "where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)"]
    sccache = shutil.which("sccache")
    启动器 = (' -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="%s"' % sccache) if sccache else ""
    if 已配置:
        # 088 root-fix：删 .ninja_deps 强制全量重扫头（sccache 吸收成本）
        行 += ["if exist target\\build-ninja\\.ninja_deps del /q target\\build-ninja\\.ninja_deps",
               "cmake --build target/build-ninja --parallel"]
    else:
        # 未配置树：ninja 必需（cmake -G Ninja·wt.py 同款探测含 VS 自带版）
        ninja = 找ninja()
        if ninja is None:
            return None
        行 += ['cmake -G Ninja -S . -B target/build-ninja '
               '-DCMAKE_MAKE_PROGRAM="%s" -DCMAKE_BUILD_TYPE=Debug '
               '-DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1"' % ninja + 启动器,
               "if errorlevel 1 (echo [FAIL] configure & exit /b 1)",
               "cmake --build target/build-ninja --parallel"]
    行.append("if errorlevel 1 (echo [FAIL] build & exit /b 1)")
    return [["cmd", "/c", str(_生成cmd(项目根, 行))]]


def _生成cmd(项目根: pathlib.Path, 行们: list[str]) -> pathlib.Path:
    """重建脚本落盘（target/exe_freshness_build.cmd·ASCII·ci_daemon 同法）。"""
    目标目录 = 项目根 / "target"
    目标目录.mkdir(parents=True, exist_ok=True)
    脚本 = 目标目录 / "exe_freshness_build.cmd"
    脚本.write_text("\r\n".join(行们) + "\r\n", encoding="ascii")
    return 脚本


def _linux重建命令(项目根: pathlib.Path) -> list[list[str]]:
    """linux 重建命令序列（gate_quick 同款：build-quick 缺配先配置再增量构建）。"""
    命令们 = []
    if not (项目根 / "target" / "build-quick" / "CMakeCache.txt").exists():
        命令们.append(["cmake", "-S", ".", "-B", "target/build-quick"])
    命令们.append(["cmake", "--build", "target/build-quick", "--parallel"])
    return 命令们


def 重建宿主编译器(项目根: pathlib.Path, 运行命令函数, 超时秒数: int,
                   详细: bool = False) -> tuple[bool, str]:
    """走既有构建通道重建宿主编译器。返回 (是否成功, 错误描述)。

    win=vcvars+Ninja（523/088 同口径）；linux=cmake build-quick（gate_quick 同款）。
    需在树锁内调用（run_e2e 主程序天然满足）——防同树并发构建互踩。"""
    命令们 = (_win重建命令(项目根) if sys.platform == "win32"
              else _linux重建命令(项目根))
    if 命令们 is None:
        return False, ("未找到 vcvars64.bat（VS2022 四版本候选均缺）或 ninja.exe"
                       "（PATH 与 VS 组件路径均缺）——无法自动重建；"
                       "请手动构建或 --cn 显式指定可用编译器")
    for 命令 in 命令们:
        if 详细:
            print(f"    [795] 重建步: {' '.join(命令)}")
        结果 = 运行命令函数(命令, 项目根, 超时秒数=超时秒数)
        if 结果.returncode != 0:
            摘要 = ((结果.stderr or "") + (结果.stdout or "")).strip()[:400]
            return False, (f"重建命令失败（rc={结果.returncode}）: {' '.join(命令)}\n"
                           f"    {摘要}")
    return True, ""


def 确保编译器就绪(显式路径: str, 编译器候选们: list, 项目根: pathlib.Path,
                   运行命令函数, 超时秒数: int, 详细: bool = False
                   ) -> tuple[pathlib.Path | None, str]:
    """宿主编译器就绪总入口（run_e2e 主程序注入点·795）。

    返回 (编译器路径, "") 或 (None, 错误描述——含指路·调用方显式红退出)。
    --cn 显式指定=跳过核验（用户自担·任务书三场景之③）；
    自动探测=选最新→核验→陈旧→重建→重探测→复验→仍坏显式红。
    冷启动（无候选）也走重建兜底（原行为=直接红——CI 冷启动树自愈）。"""
    if 显式路径:
        路径 = pathlib.Path(显式路径).resolve()
        if not 路径.exists():
            return None, f"错误: 编译器不存在: {路径}"
        print(f"选择 {路径}（--cn 显式指定→跳过新鲜度核验·795）")
        return 路径, ""
    选择 = 探测最新候选(编译器候选们, 项目根)
    if 选择 is None:
        print("未找到编译器候选——尝试构建通道兜底（795·冷启动自愈）...")
        好, 错 = 重建宿主编译器(项目根, 运行命令函数, 超时秒数, 详细)
        if not 好:
            return None, ("错误: 未找到编译器且构建通道兜底失败——请使用 --cn 指定路径"
                          f"（如 target/Debug/cn.exe）或先手动构建\n  {错}")
        选择 = 探测最新候选(编译器候选们, 项目根)
        if 选择 is None:
            return None, ("错误: 构建成功但仍未探测到编译器产物——检查构建输出路径"
                          "或使用 --cn 显式指定")
        print(f"✓ [795] 构建兜底完成: {选择}")
    else:
        新鲜, 缘由 = 核验新鲜度(选择, 项目根)
        if 新鲜:
            print(f"选择 {选择}（mtime 最新·795 新鲜度核验通过）")
            return 选择, ""
        print(f"⚠ [795] 宿主编译器陈旧: {缘由}")
        print("  → 强制触发重建（走既有构建通道·教训 #177 消费面防线）...")
        好, 错 = 重建宿主编译器(项目根, 运行命令函数, 超时秒数, 详细)
        if not 好:
            return None, ("错误: 宿主编译器陈旧且自动重建失败——"
                          "请手动构建后重跑，或 --cn 显式指定可用编译器\n  " + 错)
        重建后 = 探测最新候选(编译器候选们, 项目根)
        if 重建后 is None:
            return None, "错误: 重建后未探测到编译器产物——检查构建输出路径"
        新鲜2, 缘由2 = 核验新鲜度(重建后, 项目根)
        if not 新鲜2:
            return None, (f"错误: 重建后编译器仍陈旧（{缘由2}）——构建通道可能指向"
                          f"其他输出路径；请 --cn 显式指定刚构建的编译器")
        print(f"✓ [795] 重建完成并通过复验: {重建后}")
        选择 = 重建后
    return 选择, ""
