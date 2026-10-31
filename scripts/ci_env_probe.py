#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_env_probe.py —— runner 环境预检（#1462·2026-10-31）。

1212 集成实录（预验假红根源）：TX_03(VM-0-15-ubuntu) 树无 third-party gtest 源码
兜底、系统未装 gtest → cmake 配置 CMakeLists.txt:113 FATAL『未找到系统 Google
Test』→ 配置 rc=1、后续全部『未找到 cn 产物』、整轮假红且用例清单空。根因=新实例
入池前无环境预检——runner 对「能不能跑一轮」零知识，坏环境实例照领任务烧假红。

本模块职责（单一）：探测一轮所需最小环境，产出结构化详情供两处消费：
  ① 实例报活载荷「环境预检」键（ci_shard.启动实例报活 → 服务端实例表 → 面板）；
  ② 认领门（ci_pool.环境可领——不满足不领片/不领整任务·报活心跳保持·面板标注缺什么）。

判定口径与构建链逐项对齐（ci_build.linux构建链/win构建链·CMakeLists.txt gtest 段）：
  linux/arm64  编译器=g++（CMake 默认 CXX）·生成器=make（cmake 默认 Unix Makefiles）
  win-x64      编译器=VS2022 vcvars64.bat（ci_build.找vcvars）·生成器=ninja（-G Ninja）
  gtest 两级   本地源码兜底（CMakeLists 同款两路径）→ 系统包 cmake -P 探针
               （find_package(GTest)——与构建配置同一工具链·保真度最高）
  磁盘         shutil.disk_usage(仓库根).free ≥ CN_ENV_DISK_MIN_GB（缺省 5 GB）
  缓存(可选)   ccache/sccache——缺失不阻止领任务（1299 构建链「未启用照跑」同口径），
               只落详情供观测。

miss 纪律（882/#1239/#1360）：探测不做静默默认——探测不到/不可执行=显式落「缺失」
条目（说明带判定依据），消费点（认领门）强制检查「通过」字段分流；CN_ENV_DISK_MIN_GB
非法值=「磁盘」项显式失败（不静默取默认值继续）。

兼容 python3.8：无新语法面。
"""
from __future__ import annotations

import math
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import ci_build   # win 编译器链探测（找vcvars·依赖单向 ci_env_probe→ci_build）

探针超时秒 = 60   # 配置模式探针（CXX 编译器探测+find_package）实测 ~0.4s·余量防冷盘

# CMakeLists.txt gtest 段同款本地源码兜底路径（相对仓库根·第一候选=用户级
# third-party 目录·第二候选=先前 FetchContent 拉取的构建缓存）。
gtest本地兜底相对 = ("../../third-party/unittest/googletest/src/gtest-all.cc",
                  "target/build/_deps/googletest-src/googletest/src/gtest-all.cc")
磁盘默认GB = 5          # 一轮干净构建（源码+target/ 产物+缓存+日志）经验上界
版本超时秒 = 15


def 磁盘下限字节(环境=None) -> tuple:
    """CN_ENV_DISK_MIN_GB 解析（缺省 5 GB·非法=显式错误不静默取默认·882 口径）。
    返回 (字节, 错误说明)——错误非空时调用方必须把「磁盘」项判失败。"""
    环境 = os.environ if 环境 is None else 环境
    原始 = (环境.get("CN_ENV_DISK_MIN_GB") or "").strip()
    if not 原始:
        return 磁盘默认GB * 1024 ** 3, None
    try:
        值 = float(原始)
    except ValueError:
        return 0, "CN_ENV_DISK_MIN_GB=%r 非法（须数字 GB）" % 原始
    if not math.isfinite(值) or 值 <= 0:
        return 0, "CN_ENV_DISK_MIN_GB=%r 非法（须正数 GB）" % 原始
    return int(值 * 1024 ** 3), None


def _命令工具条目(项名: str, 命令名: str, 探测器, 执行器=None) -> dict:
    """PATH 可执行探测+版本首行（编译器/cmake/生成器共用）——路径命中但不可执行
    （权限/坏装）同样判失败（诚实显形·不按「路径在=能跑」静默放行）。
    执行器=subprocess.run 可注入（selftest 假环境·零真 PATH 依赖）。"""
    路径 = 探测器(命令名)
    if not 路径:
        return {"项": 项名, "通过": False, "命令": 命令名,
                "说明": "PATH 无 %s" % 命令名}
    try:
        r = (执行器 or subprocess.run)([str(路径), "--version"], capture_output=True,
                                       text=True, timeout=版本超时秒)
    except (OSError, subprocess.SubprocessError) as e:
        return {"项": 项名, "通过": False, "命令": 命令名,
                "说明": "%s 不可执行：%s" % (命令名, e)}
    if r.returncode != 0:
        return {"项": 项名, "通过": False, "命令": 命令名,
                "说明": "%s --version rc=%d（可执行但探测失败）" % (命令名, r.returncode)}
    首行们 = (r.stdout or "").strip().splitlines()
    return {"项": 项名, "通过": True, "命令": 命令名,
            "说明": 首行们[0] if 首行们 else 命令名}


def 编译器条目(是win: bool, 探测器, 执行器=None) -> dict:
    """win=VS2022 vcvars64.bat（cl 在 vcvars 环境·PATH 无 cl 是常态）；linux/arm64
    =g++（CMake 默认 CXX·-finput-charset 等选项按 GCC 家族口径）。"""
    if 是win:
        路径 = ci_build.找vcvars()
        if not 路径:
            return {"项": "编译器", "通过": False, "命令": "vcvars64.bat",
                    "说明": "未找到 vcvars64.bat（VS2022 安装不全）"}
        return {"项": "编译器", "通过": True, "命令": "vcvars64.bat",
                "说明": str(路径)}
    return _命令工具条目("编译器", "g++", 探测器, 执行器)


def 生成器条目(是win: bool, 探测器, 执行器=None) -> dict:
    """win=cmake -G Ninja 必须 ninja；linux/arm64=cmake 默认 Unix Makefiles 须 make。"""
    return _命令工具条目("生成器", "ninja" if 是win else "make", 探测器, 执行器)


def gtest本地兜底(仓库根: Path) -> str:
    """CMakeLists gtest 段同款两级本地源码兜底；返回命中路径，无=空串
    （否定回答·调用方据此转系统包探针）。"""
    for 相对 in gtest本地兜底相对:
        候选 = Path(仓库根) / 相对
        if 候选.exists():
            return str(候选)
    return ""


def gtest系统探针默认(cmake路径: str, 工作目录=None) -> tuple:
    """配置模式探针：最小 CXX 项目 find_package(GTest)——与构建配置**同一工具链
    同一搜索路径**（保真度最高）。不用 cmake -P：GTest Config 包（GTestTargets.cmake）
    含 add_library，脚本模式不可执行（实测『add_library command is not scriptable』
    必假 miss）；也不用 pkg-config/头文件硬编（与 find_package 搜索语义不等价）。
    项目须 LANGUAGES CXX：NONE 下 find_package 判定路径不同（实测 NONE 假 miss）。
    返回 (命中, 说明)。cmake 缺失=不命中+显式说明（不静默）。探针目录/构建目录
    用临时目录，finally 全清。"""
    if not cmake路径:
        return False, "cmake 不在 PATH——系统包探针无法执行（gtest 判缺失）"
    工作根 = Path(tempfile.mkdtemp(prefix="gtest_probe_"))
    try:
        (工作根 / "探针").mkdir()
        (工作根 / "探针" / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.20)\n"
            "project(gtest_probe LANGUAGES CXX)\n"
            "find_package(GTest QUIET)\n"
            "if(NOT GTest_FOUND)\n"
            '    message(FATAL_ERROR "GTEST_MISS")\n'
            "endif()\n"
            'message(STATUS "GTEST_OK")\n', encoding="utf-8")
        try:
            r = subprocess.run([str(cmake路径), "-S", str(工作根 / "探针"),
                                "-B", str(工作根 / "探针build")],
                               capture_output=True, text=True,
                               timeout=探针超时秒, cwd=str(工作目录 or "."))
        except (OSError, subprocess.SubprocessError) as e:
            return False, "探针执行失败：%s" % e
        if r.returncode == 0 and "GTEST_OK" in (r.stdout or ""):
            return True, "系统包 find_package(GTest) 命中"
        全行们 = [l for l in ((r.stderr or "") + (r.stdout or "")).splitlines()
                 if l.strip()]
        miss行 = next((l for l in 全行们 if "GTEST_MISS" in l), None)
        return False, "系统包未检出（%s）" % (
            (miss行 or (全行们[-1] if 全行们 else "探针 rc=%d" % r.returncode))[:120])
    finally:
        shutil.rmtree(工作根, ignore_errors=True)


def gtest条目(仓库根: Path, 探测器, 探针=None) -> dict:
    """gtest 判定（CMakeLists:80-113 语义复刻·两级）：本地源码兜底存在=通过
    （零探针成本）；否则系统包 cmake 探针定音。"""
    探针 = 探针 or gtest系统探针默认
    兜底 = gtest本地兜底(仓库根)
    if 兜底:
        return {"项": "gtest", "通过": True, "命令": "",
                "说明": "本地源码兜底 %s" % 兜底}
    命中, 说明 = 探针(探测器("cmake"))
    return {"项": "gtest", "通过": 命中, "命令": "cmake -P 探针", "说明": 说明}


def 磁盘条目(仓库根: Path, 磁盘函数, 环境=None) -> dict:
    """工作树所在盘余量 ≥ 下限（缺省 5 GB·CN_ENV_DISK_MIN_GB 可调）——预验 637
    清产物后冷编+缓存+日志的可用空间防线（写满盘=构建随机红·比缺工具更难归因）。"""
    下限, 错误 = 磁盘下限字节(环境)
    if 错误:
        return {"项": "磁盘", "通过": False, "命令": "", "说明": 错误}
    try:
        _总, _已用, 余 = 磁盘函数(str(仓库根))
    except OSError as e:
        return {"项": "磁盘", "通过": False, "命令": "",
                "说明": "disk_usage 失败：%s" % e}
    余GB = 余 / 1024 ** 3
    下限GB = 下限 / 1024 ** 3
    return {"项": "磁盘", "通过": 余 >= 下限, "命令": "",
            "说明": "余 %.1f GB%s下限 %.0f GB" % (余GB, "≥" if 余 >= 下限 else "<", 下限GB)}


def 缓存条目(是win: bool, 探测器) -> dict:
    """编译缓存（可选·缺失不阻止领任务）：linux 优先 ccache 后 sccache（1299 探测
    顺序）；win=sccache（291 构建链注入面）。可选=True 标记——判定可领 不计入。"""
    顺序 = ("sccache", "ccache") if 是win else ("ccache", "sccache")
    for 名 in 顺序:
        路径 = 探测器(名)
        if 路径:
            return {"项": "缓存", "通过": True, "可选": True, "命令": 名,
                    "说明": str(路径)}
    return {"项": "缓存", "通过": False, "可选": True, "命令": "",
            "说明": "ccache/sccache 未安装（可选·构建走原始编译器）"}


def 探测环境(仓库根: Path, 是win: bool, 环境=None, 探测器=None, 执行器=None,
             磁盘函数=None, gtest探针=None) -> dict:
    """一轮最小环境探测（唯一入口）——返回：
    {"通过": bool,            # 必选项（编译器/cmake/生成器/gtest/磁盘）全过
     "缺失": [项名…],          # 认领门/面板「缺什么」直接消费
     "项目": [{项,通过,可选?,命令,说明}…]}
    探测器/执行器/磁盘函数/gtest探针 可注入（selftest 假环境·零真 PATH 依赖）。"""
    环境 = os.environ if 环境 is None else 环境
    探测器 = 探测器 or shutil.which
    磁盘函数 = 磁盘函数 or shutil.disk_usage
    项目 = [
        编译器条目(是win, 探测器, 执行器),
        _命令工具条目("cmake", "cmake", 探测器, 执行器),
        生成器条目(是win, 探测器, 执行器),
        gtest条目(Path(仓库根), 探测器, gtest探针),
        磁盘条目(Path(仓库根), 磁盘函数, 环境),
        缓存条目(是win, 探测器),
    ]
    必缺 = [条目["项"] for 条目 in 项目
            if not 条目.get("通过") and not 条目.get("可选")]
    return {"通过": not 必缺, "缺失": 必缺, "项目": 项目}


def 判定可领(详情) -> bool:
    """认领门消费点（强制检查·不满足不领）——详情非 dict/缺「通过」键=False
    （fail-safe：结构未知按不可领·运维看报活日志/面板修环境）。"""
    if not isinstance(详情, dict):
        return False
    return bool(详情.get("通过"))


def 缺失一行(详情) -> str:
    """缺失项中文顿号摘要（日志/面板一行）——通过=空串；结构异常=显式文案
    （不冒充空缺失）。"""
    if not isinstance(详情, dict):
        return "（预检详情结构异常）"
    if 详情.get("通过"):
        return ""
    return "、".join(详情.get("缺失") or []) or "（缺失清单空但未通过·结构异常）"
