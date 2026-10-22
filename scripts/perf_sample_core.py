#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""perf_sample_core.py —— perf_sample 的采样/解析/报告核心（#1226/#1368）。

入口 scripts/perf_sample.py；内置自测 scripts/perf_sample_selftest.py。
三文件拆分是为守住单文件 ≤1000 行红线（#1368 P2 体量自查）。
"""

from __future__ import annotations

import json
import os
import pathlib
import platform as 平台模块
import re
import shlex
import shutil
import statistics
import subprocess
import sys
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

# ============ 常量 ============

项目根 = pathlib.Path(__file__).resolve().parent.parent
采样目录 = 项目根 / "target" / "perf"
默认入口 = 项目根 / "CN语言编译器v2" / "主.cn"
默认轮数 = 3
默认热点数 = 20
默认超时秒 = 3600
默认采样频率 = 999
自测超时秒 = 120
自测循环数 = 5000
浮点容差 = 1e-9
反修饰批大小 = 200
最小产物字节 = 64 * 1024
最小产物行数 = 100
文本产物后缀 = (".s", ".asm", ".txt")
缓存统计行 = re.compile(r"\[(?:AST 缓存|程序缓存)\].*")

平台映射 = {
    "linux-x64": "linux-x86_64",
    "linux-arm64": "linux-arm64",
    "win-x64": "win-x64",
}
合法目标 = ("auto", "linux-x64", "linux-arm64", "win-x64")
合法后端 = ("auto", "perf", "callgrind", "none")
合法负载 = ("v2-build", "exec")
合法缓存 = ("cold", "hot", "asis")

# 退出码（与项目脚本惯例对齐：2=用法/环境，3=平台边界，4=负载失败）
退出成功 = 0
退出用法 = 2
退出边界 = 3
退出负载 = 4

# perf / callgrind 解析正则
callgrind行 = re.compile(r"^\s*([\d,]+)\s+\(\s*([\d.]+)%\)\s+(.+?)\s*$")
perf自身行 = re.compile(r"^\s*([\d.]+)%\s+(.+?)\s*$")
perf包含行 = re.compile(r"^\s*([\d.]+)%\s+([\d.]+)%\s+(.+?)\s*$")
perf符号前缀 = re.compile(r"^\[[a-zA-Z.]\]\s*")
callgrind二进制后缀 = re.compile(r"\s+\[[^\]]+\]$")
program_totals = re.compile(r"^\s*([\d,]+)\s+\(\s*100\.0+%\s*\)\s+PROGRAM TOTALS")


class 环境错误(RuntimeError):
    """用法/环境错误（退出码 2）。"""


class 平台边界(RuntimeError):
    """目标平台无法给出可信指令级口径（退出码 3）。"""


class 负载失败(RuntimeError):
    """被测负载执行失败（退出码 4）。"""


# ============ 执行器（原生 / WSL） ============

def _wsl路径(路径: pathlib.Path) -> str:
    """Windows 绝对路径 → WSL /mnt/<盘符>/...；已是 POSIX 路径原样返回。"""
    文本 = str(路径)
    if 文本.startswith("/"):
        return 文本
    匹配 = re.match(r"^([A-Za-z]):[\\/](.*)$", 文本)
    if 匹配 is None:
        raise 环境错误(f"无法把路径转换为 WSL 路径: {文本}")
    盘符 = 匹配.group(1).lower()
    余下 = 匹配.group(2).replace("\\", "/")
    return f"/mnt/{盘符}/{余下}"


class 执行器:
    """统一执行面：原生子进程或 `wsl -- bash -lc`。"""

    def __init__(self, 用WSL: bool, 发行版: str = ""):
        self.用WSL = 用WSL
        self.发行版 = 发行版
        self._可用缓存: dict[str, bool] = {}

    def 路径(self, 路径: pathlib.Path) -> str:
        return _wsl路径(路径) if self.用WSL else str(路径)

    def _命令(self, argv: list, cwd: pathlib.Path | None,
              环境: dict | None) -> list:
        if not self.用WSL:
            命令 = [str(x) for x in argv]
            return 命令
        片段 = []
        if cwd is not None:
            片段.append(f"cd {shlex.quote(self.路径(cwd))}")
        前置 = " ".join(f"{k}={shlex.quote(str(v))}" for k, v in (环境 or {}).items())
        正文 = " ".join(shlex.quote(str(x)) for x in argv)
        片段.append(f"{前置} {正文}".strip())
        外壳 = " && ".join(片段)
        前缀 = ["wsl"]
        if self.发行版:
            前缀 += ["-d", self.发行版]
        return 前缀 + ["--", "bash", "-lc", 外壳]

    def 运行(self, argv: list, cwd: pathlib.Path | None = None,
             环境: dict | None = None, 超时: int = 默认超时秒) -> subprocess.CompletedProcess:
        命令 = self._命令(argv, cwd, 环境)
        # 原生执行必须显式注入完整环境：cold 的 CN_NO_AST_CACHE/CN_NO_PROGRAM_CACHE
        # 只有进入子进程 env 才真正生效（P0 复审实证：此前仅拼进 WSL 外壳命令）。
        运行环境 = None if self.用WSL else {**os.environ, **(环境 or {})}
        try:
            return subprocess.run(命令, capture_output=True, text=True,
                                  encoding="utf-8", errors="replace", timeout=超时,
                                  env=运行环境)
        except subprocess.TimeoutExpired:
            return subprocess.CompletedProcess(命令, -9, "", f"超时（>{超时} 秒）")
        except FileNotFoundError as 异常:
            return subprocess.CompletedProcess(命令, 127, "", str(异常))

    def 有工具(self, 名: str) -> bool:
        if 名 in self._可用缓存:
            return self._可用缓存[名]
        结果 = self.运行(["sh", "-lc", f"command -v {shlex.quote(名)}"], 超时=30)
        可用 = 结果.returncode == 0 and bool((结果.stdout or "").strip())
        self._可用缓存[名] = 可用
        return 可用


def 造执行器(用WSL: bool, 发行版: str = "") -> 执行器:
    if 用WSL and shutil.which("wsl") is None:
        raise 环境错误("未找到 wsl（Windows 侧热点归因需要 WSL + valgrind）")
    return 执行器(用WSL, 发行版)


# ============ 主机/目标/后端选择 ============

def 探测主机() -> dict:
    return {
        "系统": "windows" if os.name == "nt" else "linux",
        "架构": 平台模块.machine().lower(),
        "是Windows": os.name == "nt",
    }


def 解析目标(请求: str, 主机: dict) -> str:
    if 请求 != "auto":
        if 请求 == "win-x64" and not 主机["是Windows"]:
            raise 平台边界("win-x64 目标只能在 Windows 主机执行")
        return 请求
    if 主机["是Windows"]:
        return "win-x64"
    if 主机["架构"] in ("x86_64", "amd64"):
        return "linux-x64"
    if 主机["架构"] in ("aarch64", "arm64"):
        return "linux-arm64"
    raise 平台边界(f"未知主机架构 {主机['架构']}：auto 无法选目标")


def 探测工具(执行器: 执行器) -> dict:
    工具 = {
        "perf": 执行器.有工具("perf"),
        "valgrind": 执行器.有工具("valgrind"),
        "callgrind_annotate": 执行器.有工具("callgrind_annotate"),
        "c++filt": 执行器.有工具("c++filt"),
        "python3": 执行器.有工具("python3"),
        "PMU": False,
    }
    if 工具["perf"]:
        结果 = 执行器.运行(["perf", "stat", "-e", "instructions", "true"], 超时=60)
        输出 = (结果.stdout or "") + (结果.stderr or "")
        工具["PMU"] = (结果.returncode == 0
                       and "No supported events found" not in 输出
                       and re.search(r"[\d,.]+\s+instructions", 输出) is not None)
    return 工具


def _规范架构(文本: str) -> str:
    值 = (文本 or "").lower()
    if 值 in ("x86_64", "amd64", "x64"):
        return "x86_64"
    if 值 in ("aarch64", "arm64"):
        return "aarch64"
    return 值


def _目标架构(目标: str) -> str:
    return "aarch64" if 目标 == "linux-arm64" else "x86_64"


def _校验架构边界(请求: str, 目标: str, 主机: dict) -> None:
    """指令级后端统一边界：目标架构必须等于宿主架构。

    交叉架构（含反向：arm64 宿主测 x64 目标）的指令数不可作为收益证据；qemu/代理
    只能给墙钟。显式 perf/callgrind 与 auto 一律退出码 3；确需墙钟用 --backend none。
    """
    目标架 = _目标架构(目标)
    宿主架 = _规范架构(主机["架构"])
    if 目标架 == 宿主架:
        return
    raise 平台边界(
        f"目标架构 {目标架} != 宿主架构 {宿主架}：指令级采样（perf/callgrind）"
        "不得跨架构使用（qemu/代理结果不能宣称指令数收益）。"
        "需要墙钟请显式 --backend none；需要指令级请在目标架构真机运行。")


def 选择后端(请求: str, 目标: str, 主机: dict, 工具: dict,
             有代理: bool) -> dict:
    """返回 {后端, 代理, 原因}。auto 永不在无工具时假装有指令数。"""
    if 请求 == "none":
        return {"后端": "none", "代理": False, "原因": "用户显式 --backend none"}
    _校验架构边界(请求, 目标, 主机)
    if 请求 == "perf":
        if not 工具["PMU"]:
            raise 平台边界("显式要求 perf，但当前环境 PMU 不可用（No supported events）")
        return {"后端": "perf", "代理": False, "原因": "用户显式 perf"}
    if 请求 == "callgrind":
        if 主机["是Windows"] and not 有代理:
            raise 平台边界(
                "win 原生 PE 无法 callgrind；显式 callgrind 需同时给 --proxy-compiler "
                "指定同源 linux-x64 ELF（或改用 --backend none 只取墙钟）")
        if not (工具["valgrind"] and 工具["callgrind_annotate"]):
            raise 平台边界("显式要求 callgrind，但缺少 valgrind/callgrind_annotate")
        return {"后端": "callgrind", "代理": bool(有代理), "原因": "用户显式 callgrind"}
    # auto
    if 主机["是Windows"]:
        if 工具["valgrind"] and 工具["callgrind_annotate"] and 有代理:
            return {"后端": "callgrind", "代理": True,
                    "原因": "win 原生无 PMU/valgrind；WSL callgrind 代理同源 linux-x64 构建"}
        return {"后端": "none", "代理": False,
                "原因": "win 原生无 PMU/callgrind；未提供 --proxy-compiler，只有墙钟参考"}
    if 工具["PMU"]:
        return {"后端": "perf", "代理": False, "原因": "原生 Linux PMU 可用"}
    if 工具["valgrind"] and 工具["callgrind_annotate"]:
        return {"后端": "callgrind", "代理": False,
                "原因": "PMU 不可用，自动降级 callgrind"}
    return {"后端": "none", "代理": False,
            "原因": "无 perf PMU 且无 valgrind：只有墙钟参考，禁止宣称收益"}


# ============ 负载构造 ============

def _默认编译器(目标: str) -> pathlib.Path | None:
    候选名 = ("cn.exe", "cn") if 目标 == "win-x64" else ("cn", "cn.exe")
    for 名 in 候选名:
        路径 = 项目根 / "target" / 名
        if 路径.exists():
            return 路径
    return None


def 缓存环境(缓存: str) -> dict:
    if 缓存 == "cold":
        # 冷态同时开统计：报告里命中必须为 0，作为「禁用真的传到子进程」的机械证据。
        return {"CN_NO_AST_CACHE": "1", "CN_NO_PROGRAM_CACHE": "1",
                "CN_AST_CACHE_STATS": "1", "CN_PROGRAM_CACHE_STATS": "1"}
    if 缓存 == "hot":
        # 热态必须能核对命中：编译器统计走 stderr，由报告统一收集。
        return {"CN_AST_CACHE_STATS": "1", "CN_PROGRAM_CACHE_STATS": "1"}
    return {}


def _是ELF(路径: pathlib.Path) -> bool:
    try:
        with open(路径, "rb") as 文件:
            return 文件.read(4) == b"\x7fELF"
    except OSError:
        return False


def _是POSIX绝对路径(文本: str) -> bool:
    return str(文本).startswith("/")


def _WSL路径存在(执行器: 执行器, 文本: str) -> bool:
    return 执行器.运行(["test", "-e", 文本], 超时=30).returncode == 0


def _WSL是ELF(执行器: 执行器, 文本: str) -> bool:
    结果 = 执行器.运行(
        ["sh", "-lc", f"od -An -tx1 -N4 {shlex.quote(文本)}"], 超时=30)
    return 结果.returncode == 0 and "7f 45 4c 46" in (结果.stdout or "").lower()


def _是ELF_执行器(执行器: 执行器, 文本: str) -> bool:
    if 执行器.用WSL and _是POSIX绝对路径(文本):
        return _WSL是ELF(执行器, 文本)
    return _是ELF(pathlib.Path(文本))


def _解析路径(文本: str, 执行器: 执行器, 用途: str) -> str:
    """解析可执行/入口路径；WSL 的 /mnt/... 原样保留（不在 Windows 上 resolve）。"""
    文本 = str(文本)
    if 执行器.用WSL and _是POSIX绝对路径(文本):
        if not _WSL路径存在(执行器, 文本):
            raise 环境错误(f"{用途}不存在: {文本}")
        return 文本
    if os.name == "nt" and _是POSIX绝对路径(文本):
        raise 环境错误(
            f"{用途}在 Windows 原生执行下不能用 POSIX 路径: {文本}"
            "（请用 C:\\... 写法；/mnt/... 仅 WSL 代理面支持）")
    路径 = pathlib.Path(文本).resolve()
    if not 路径.exists():
        raise 环境错误(f"{用途}不存在: {路径}")
    return 执行器.路径(路径)


def _同路径(左: str, 右: str) -> bool:
    左, 右 = str(左), str(右)
    if _是POSIX绝对路径(左) or _是POSIX绝对路径(右):
        return 左.rstrip("/") == 右.rstrip("/")
    return os.path.normcase(os.path.abspath(左)) == os.path.normcase(os.path.abspath(右))


def _转换参数(执行器: 执行器, 文本: str) -> str:
    """WSL 执行时把「存在的 Windows 绝对路径」参数转成 /mnt/...；其它原样。"""
    if not 执行器.用WSL:
        return 文本
    if re.match(r"^[A-Za-z]:[\\/]", 文本) and pathlib.Path(文本).exists():
        return 执行器.路径(pathlib.Path(文本).resolve())
    return 文本


def 构造负载(参数, 目标: str, 后端信息: dict, 执行器: 执行器) -> dict:
    if 参数.load == "exec":
        if not 参数.exec:
            raise 环境错误("--load exec 必须给 --exec <可执行文件>")
        可执行 = _解析路径(参数.exec, 执行器, "被测可执行文件")
        环境 = {} if 参数.cache == "asis" else 缓存环境(参数.cache)
        参数们 = [_转换参数(执行器, str(x)) for x in (参数.arg or [])]
        return {"argv": [可执行] + 参数们,
                "cwd": 项目根, "环境": 环境,
                "显示": f"{可执行} {' '.join(参数们)}".strip(),
                "输出": None}
    # v2-build
    代理 = bool(后端信息.get("代理"))
    候选 = 参数.proxy_compiler if 代理 and 参数.proxy_compiler else 参数.compiler
    if not 候选:
        默认 = _默认编译器(目标)
        if 默认 is not None:
            候选 = str(默认)
    if not 候选:
        raise 环境错误("未找到宿主编译器：先构建 target/cn(.exe) 或用 --compiler 指定")
    编译器参数 = _解析路径(候选, 执行器, "宿主编译器")
    if (后端信息["后端"] == "callgrind"
            or (执行器.用WSL and 目标 == "linux-x64")) and not _是ELF_执行器(执行器, 编译器参数):
        raise 环境错误(
            f"callgrind 只能采样 ELF：{编译器参数} 不是 ELF（win 原生 PE 请用 "
            "--proxy-compiler 指定同源 linux-x64 构建；linux 目标请用 --compiler 指定 ELF）")
    if 代理:
        编译目标 = "linux-x86_64"
    else:
        编译目标 = 平台映射[目标]
    入口文本 = 参数.entry if 参数.entry else str(默认入口)
    入口参数 = _解析路径(入口文本, 执行器, "v2 入口")
    if 参数.output and os.name == "nt" and _是POSIX绝对路径(参数.output):
        raise 环境错误(
            f"--output 在 Windows 请用本机路径（收到 {参数.output}）；"
            "POSIX 路径仅 --compiler/--proxy-compiler/--exec 支持")
    输出 = pathlib.Path(参数.output).resolve() if 参数.output else (
        采样目录 / ("perf_load.exe" if 编译目标 == "win-x64" else "perf_load.bin"))
    if _同路径(str(输出), 入口参数) or _同路径(str(输出), 编译器参数):
        raise 环境错误("--output 不得等于 v2 入口或宿主编译器（防覆盖输入/编译器）")
    输出.parent.mkdir(parents=True, exist_ok=True)
    if 输出.exists():
        输出.unlink()
    环境 = 缓存环境(参数.cache)
    输出参数 = 执行器.路径(输出)
    return {"argv": [编译器参数, "build", 入口参数,
                     "--target", 编译目标, "--output", 输出参数],
            "cwd": 项目根, "环境": 环境,
            "显示": f"{编译器参数} build {入口参数} --target {编译目标} --output {输出}",
            "输出": 输出}


# ============ 产物守卫 / 缓存统计 ============

def 提取缓存统计(文本: str) -> list[str]:
    """从编译器 stdout/stderr 提取 [AST 缓存]/[程序缓存] 统计行。"""
    return [行.strip() for 行 in (文本 or "").splitlines() if 缓存统计行.search(行)]


def _清理产物(负载: dict) -> None:
    输出 = 负载.get("输出")
    if not 输出:
        return
    路径 = pathlib.Path(str(输出))
    if 路径.exists():
        路径.unlink()


def _校验产物(负载: dict) -> None:
    """冷态/热态都做防假绿：产物必须存在且达到大小（文本再查行数）。"""
    输出 = 负载.get("输出")
    if not 输出:
        return
    路径 = pathlib.Path(str(输出))
    if not 路径.exists():
        raise 负载失败(f"负载未产出预期产物（防假绿）: {路径}")
    大小 = 路径.stat().st_size
    if 大小 <= 0:
        raise 负载失败(f"产物为空（防假绿）: {路径}")
    if 路径.suffix.lower() in 文本产物后缀:
        try:
            行数 = sum(1 for _ in 路径.open("r", encoding="utf-8", errors="replace"))
        except OSError as 异常:
            raise 负载失败(f"产物不可读: {路径}（{异常}）")
        if 行数 < 最小产物行数:
            raise 负载失败(
                f"文本产物行数过少（{行数} < {最小产物行数}）: {路径}")
    elif 大小 < 最小产物字节:
        raise 负载失败(
            f"产物过小（{大小} 字节 < {最小产物字节}）：疑似空跑/截断: {路径}")


def _校验callgrind结果(总数: int, 自身: dict) -> None:
    if 总数 <= 0 or not 自身:
        raise 负载失败(
            f"callgrind 采集为零（Ir={总数}·热点条目={len(自身)}）："
            "负载可能未执行/输出为空；零值不得作为采样结果")


# ============ callgrind 采样 ============

def 跑callgrind(执行器: 执行器, 负载: dict, 输出文件: pathlib.Path,
                超时: int) -> subprocess.CompletedProcess:
    输出文件.parent.mkdir(parents=True, exist_ok=True)
    参数 = ["valgrind", "--tool=callgrind", "--quiet",
            f"--callgrind-out-file={执行器.路径(输出文件)}"] + list(负载["argv"])
    return 执行器.运行(参数, cwd=负载["cwd"], 环境=负载["环境"], 超时=超时)


def callgrind注解(执行器: 执行器, 输出文件: pathlib.Path, inclusive: bool,
                   超时: int) -> str:
    参数 = ["callgrind_annotate", "--threshold=100",
            "--inclusive=" + ("yes" if inclusive else "no"),
            "--show-percs=yes", "--auto=no", 执行器.路径(输出文件)]
    结果 = 执行器.运行(参数, 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"callgrind_annotate 失败: {(结果.stderr or '').strip()[:300]}")
    return 结果.stdout or ""


def 解析callgrind(文本: str) -> tuple[dict, int]:
    """返回 ({原始 file:function: {'文件','函数','计数','百分比'}}, 总指令数)。"""
    条目: dict[str, dict] = {}
    总数 = 0
    for 行 in 文本.splitlines():
        匹配 = program_totals.match(行)
        if 匹配:
            总数 = int(匹配.group(1).replace(",", ""))
            continue
        匹配 = callgrind行.match(行)
        if not 匹配:
            continue
        计数 = int(匹配.group(1).replace(",", ""))
        百分比 = float(匹配.group(2))
        主体 = callgrind二进制后缀.sub("", 匹配.group(3))
        if ":" not in 主体:
            continue
        文件, 函数 = 主体.split(":", 1)
        键 = f"{文件}:{函数}"
        条目[键] = {"文件": 文件, "函数": 函数, "计数": 计数, "百分比": 百分比}
    if 总数 == 0:
        总数 = sum(项["计数"] for 项 in 条目.values())
    return 条目, 总数


def 反修饰(执行器: 执行器 | None, 名称们: list[str]) -> dict[str, str]:
    唯一 = sorted({x for x in 名称们 if x and not x.startswith("0x")})
    if not 唯一:
        return {}
    映射: dict[str, str] = {}
    if 执行器 is None or not 执行器.有工具("c++filt"):
        return {x: x for x in 唯一}
    # 分批：大型编译单元符号可达数千，整表塞 argv 会触 ARG_MAX。
    for 起点 in range(0, len(唯一), 反修饰批大小):
        块 = 唯一[起点:起点 + 反修饰批大小]
        结果 = 执行器.运行(["c++filt"] + 块, 超时=60)
        行们 = [x for x in (结果.stdout or "").splitlines() if x.strip()]
        if 结果.returncode != 0 or len(行们) != len(块):
            for 名 in 块:
                映射[名] = 名
        else:
            映射.update(dict(zip(块, 行们)))
    return 映射


def 合并callgrind(自身: dict, 包含: dict, 执行器: 执行器,
                   热点数: int) -> list[dict]:
    函数们 = [项["函数"] for 项 in 自身.values()] + [项["函数"] for 项 in 包含.values()]
    翻译 = 反修饰(执行器, 函数们)
    合并: dict[str, dict] = {}
    for 键, 项 in 自身.items():
        合并[键] = {"文件": 项["文件"], "函数": 翻译.get(项["函数"], 项["函数"]),
                    "self": 项["百分比"], "包含": 0.0, "指令": 项["计数"]}
    for 键, 项 in 包含.items():
        if 键 not in 合并:
            合并[键] = {"文件": 项["文件"], "函数": 翻译.get(项["函数"], 项["函数"]),
                        "self": 0.0, "包含": 项["百分比"], "指令": 项["计数"]}
        else:
            合并[键]["包含"] = 项["百分比"]
    排序 = sorted(合并.values(), key=lambda x: (-x["self"], -x["包含"], x["函数"]))
    return 排序[:热点数]


# ============ perf 采样 ============

def 解析perf统计(文本: str) -> tuple[list[float], list[float]]:
    指令们: list[float] = []
    时钟们: list[float] = []
    for 行 in 文本.splitlines():
        列 = [x.strip() for x in 行.split(",")]
        if len(列) < 3:
            continue
        数值, 单位, 事件 = 列[0], 列[1], 列[2]
        if "No supported events" in 行 or not re.match(r"^[\d.]+$", 数值.replace(",", "")):
            continue
        try:
            值 = float(数值.replace(",", ""))
        except ValueError:
            continue
        if 事件.startswith("instructions"):
            指令们.append(值)
        elif 事件.startswith("task-clock"):
            时钟们.append(值)
    return 指令们, 时钟们


def 解析perf报告(文本: str, 包含: bool) -> dict[str, float]:
    结果: dict[str, float] = {}
    for 行 in 文本.splitlines():
        行 = 行.strip()
        if not 行 or 行.startswith("#"):
            continue
        匹配 = perf包含行.match(行) if 包含 else perf自身行.match(行)
        if not 匹配:
            continue
        if 包含:
            数值 = float(匹配.group(1))
            符号 = perf符号前缀.sub("", 匹配.group(3)).strip()
        else:
            数值 = float(匹配.group(1))
            符号 = perf符号前缀.sub("", 匹配.group(2)).strip()
        if not 符号 or 符号 in ("Symbol", "Shared Object"):
            continue
        结果[符号] = 结果.get(符号, 0.0) + 数值
    return 结果


def 跑perf统计(执行器: 执行器, 负载: dict,
                超时: int) -> tuple[list[float], list[float], str]:
    参数 = ["perf", "stat", "-e", "instructions,task-clock", "-x,"] + list(负载["argv"])
    结果 = 执行器.运行(参数, cwd=负载["cwd"], 环境=负载["环境"], 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"perf stat 失败: {(结果.stderr or '').strip()[:300]}")
    文本 = (结果.stdout or "") + "\n" + (结果.stderr or "")
    指令们, 时钟们 = 解析perf统计(文本)
    return 指令们, 时钟们, 文本


def 跑perf记录(执行器: 执行器, 负载: dict, 数据文件: pathlib.Path,
                超时: int) -> None:
    数据文件.parent.mkdir(parents=True, exist_ok=True)
    参数 = ["perf", "record", "-F", str(默认采样频率), "-g", "-o",
            执行器.路径(数据文件), "--"] + list(负载["argv"])
    结果 = 执行器.运行(参数, cwd=负载["cwd"], 环境=负载["环境"], 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"perf record 失败（perf_event_paranoid?）: "
                       f"{(结果.stderr or '').strip()[:300]}")


def perf报告(执行器: 执行器, 数据文件: pathlib.Path, 包含: bool,
             超时: int) -> dict[str, float]:
    参数 = ["perf", "report", "--stdio", "--sort", "symbol", "--percent-limit", "0",
            "-i", 执行器.路径(数据文件)]
    if 包含:
        参数.insert(2, "--children")
    else:
        参数.insert(2, "--no-children")
    结果 = 执行器.运行(参数, 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"perf report 失败: {(结果.stderr or '').strip()[:300]}")
    return 解析perf报告(结果.stdout or "", 包含)


def 合并perf(自身: dict, 包含: dict, 热点数: int) -> list[dict]:
    键们 = set(自身) | set(包含)
    行们 = [{"文件": "", "函数": 键, "self": 自身.get(键, 0.0),
             "包含": 包含.get(键, 0.0), "指令": 0}
            for 键 in 键们]
    排序 = sorted(行们, key=lambda x: (-x["self"], -x["包含"], x["函数"]))
    return 排序[:热点数]


# ============ 一轮测量 ============

def 跑一轮(执行器: 执行器, 后端信息: dict, 负载: dict, 参数,
           轮号: int) -> dict:
    后端 = 后端信息["后端"]
    if 参数.cache == "cold":
        _清理产物(负载)  # 每轮清，防上一轮产物让本次“空跑成功”
    开始 = time.perf_counter()
    结果 = {"轮号": 轮号, "指令": [], "task_clock": [], "热点": [],
            "总指令": 0, "缓存统计": []}
    if 后端 == "callgrind":
        输出文件 = 采样目录 / f"callgrind.out.{os.getpid()}.{轮号}"
        运行结果 = 跑callgrind(执行器, 负载, 输出文件, 参数.timeout)
        if 运行结果.returncode != 0:
            raise 负载失败(f"callgrind 采样失败 rc={运行结果.returncode}: "
                           f"{(运行结果.stderr or '').strip()[:300]}")
        结果["缓存统计"] = 提取缓存统计(
            (运行结果.stdout or "") + "\n" + (运行结果.stderr or ""))
        自身文本 = callgrind注解(执行器, 输出文件, False, 参数.timeout)
        包含文本 = callgrind注解(执行器, 输出文件, True, 参数.timeout)
        自身, 总数 = 解析callgrind(自身文本)
        包含, _ = 解析callgrind(包含文本)
        _校验callgrind结果(总数, 自身)
        结果["总指令"] = 总数
        结果["指令"] = [总数]
        结果["热点"] = 合并callgrind(自身, 包含, 执行器, 参数.top)
    elif 后端 == "perf":
        指令们, 时钟们, 文本 = 跑perf统计(执行器, 负载, 参数.timeout)
        if not 指令们:
            raise 负载失败("perf stat 未给出 instructions（PMU 不支持或事件被禁）")
        结果["指令"] = 指令们
        结果["task_clock"] = 时钟们
        结果["缓存统计"] = 提取缓存统计(文本)
        数据文件 = 采样目录 / f"perf.data.{os.getpid()}.{轮号}"
        跑perf记录(执行器, 负载, 数据文件, 参数.timeout)
        自身 = perf报告(执行器, 数据文件, False, 参数.timeout)
        包含 = perf报告(执行器, 数据文件, True, 参数.timeout)
        结果["热点"] = 合并perf(自身, 包含, 参数.top)
    else:
        运行结果 = 执行器.运行(list(负载["argv"]), cwd=负载["cwd"],
                               环境=负载["环境"], 超时=参数.timeout)
        if 运行结果.returncode != 0:
            raise 负载失败(f"负载失败 rc={运行结果.returncode}: "
                           f"{(运行结果.stderr or '').strip()[:300]}")
        结果["缓存统计"] = 提取缓存统计(
            (运行结果.stdout or "") + "\n" + (运行结果.stderr or ""))
    _校验产物(负载)
    结果["墙钟秒"] = time.perf_counter() - 开始
    return 结果


# ============ 报告 ============

def _中位(样本: list[float]) -> float:
    return statistics.median(样本) if 样本 else 0.0


def _热点表(行们: list[dict]) -> list[str]:
    表 = ["| # | 函数 | self% | inclusive% | self 指令 | 文件 |",
          "|---:|---|---:|---:|---:|---|"]
    for 序号, 行 in enumerate(行们, 1):
        文件 = os.path.basename(行.get("文件", "")) or "—"
        指令 = f"{行.get('指令', 0):,}" if 行.get("指令") else "—"
        表.append(f"| {序号} | `{行['函数']}` | {行['self']:.2f} | "
                  f"{行['包含']:.2f} | {指令} | {文件} |")
    return 表


def 生成报告(参数, 目标: str, 后端信息: dict, 负载: dict,
             结果们: list[dict]) -> str:
    指令样本 = [r["总指令"] for r in 结果们 if r.get("总指令")]
    perf指令 = [x for r in 结果们 for x in r.get("指令", [])]
    时钟样本 = [x for r in 结果们 for x in r.get("task_clock", [])]
    墙钟样本 = [r["墙钟秒"] for r in 结果们]
    行们 = [
        "# 性能采样报告（perf_sample.py·#1226）",
        "",
        f"- 时间: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        f"- 目标平台: {目标}（主机 {平台模块.system()} {平台模块.machine()}）",
        f"- 后端: {后端信息['后端']}（{后端信息['原因']}）",
        f"- 代理口径: {'是（WSL callgrind·linux-x64 代理）' if 后端信息.get('代理') else '否'}",
        f"- 负载: {参数.load} · 缓存: {参数.cache} · 轮数: {参数.runs}",
        f"- 命令: `{负载['显示']}`",
        "",
    ]
    if 后端信息["后端"] == "none":
        行们 += ["> **警告：本轮没有指令级采样器（无 PMU/无 callgrind）。**",
                "> 下表只有墙钟参考，**不得用于宣称性能收益**（plans/032 §2.1/§3）。",
                ""]
    if 后端信息.get("代理"):
        行们 += ["> **代理边界：** win 原生 PE 无 valgrind；热点表来自同源 linux-x64",
                "> 构建的 WSL callgrind，仅作函数级归因，不是 win 指令数证据。", ""]
    if 后端信息["后端"] == "callgrind":
        行们 += [f"- callgrind Ir 样本: {[f'{x:,}' for x in 指令样本]}",
                f"- callgrind Ir 中位: **{_中位(指令样本):,.0f}**",
                "- 口径：Ir=valgrind 插桩指令数（确定性·A/B 同口径可比；绝对值≠原生指令数）。", ""]
    elif 后端信息["后端"] == "perf":
        行们 += [f"- instructions 样本: {[f'{x:,.0f}' for x in perf指令]}",
                f"- instructions 中位: **{_中位(perf指令):,.0f}**",
                f"- task-clock 中位: {_中位(时钟样本):.2f} ms" if 时钟样本
                else "- task-clock: —", ""]
    else:
        行们 += [f"- 墙钟样本: {[round(x, 3) for x in 墙钟样本]} 秒",
                f"- 墙钟中位: {_中位(墙钟样本):.3f} 秒（**仅参考**）", ""]
    if 参数.cache in ("cold", "hot"):
        口径 = "冷态（缓存禁用·命中应为 0）" if 参数.cache == "cold" else "热态"
        行们 += [f"## 缓存统计（{口径}）", ""]
        for 结果 in 结果们:
            统计 = 结果.get("缓存统计") or []
            行们.append(f"- 第 {结果['轮号']} 轮: "
                       + ("；".join(统计) if 统计 else "未捕获（编译器未输出统计）"))
        if 参数.cache == "hot":
            行们 += ["", "> 热态命中只说明重复编译省时，不得冒充编译器变快或门禁收益"
                    "（plans/032 §2.3）。"]
        else:
            行们 += ["", "> 冷态统计用于证明 CN_NO_AST_CACHE/CN_NO_PROGRAM_CACHE "
                    "确实进入子进程；命中非 0 即环境注入失败（plans/032 §2.3）。"]
        行们 += [""]
    行们 += ["## 热点 Top %d" % 参数.top, ""]
    if 后端信息["后端"] == "none":
        行们 += ["（无：后端 none 未采样，不产出热点表）", ""]
    else:
        for 结果 in 结果们:
            if 参数.runs > 1:
                行们.append(f"### 第 {结果['轮号']} 轮")
            行们 += _热点表(结果["热点"]) + [""]
    备注 = ["## 口径备注", ""]
    if 后端信息["后端"] == "callgrind":
        备注.append(
            "1. inclusive% 可能 >100%（递归/重复计数）；self% 是函数自身指令占比；"
            "同名函数多文件条目=内联副本/不同 DSO，分列保留。")
    elif 后端信息["后端"] == "perf":
        备注.append(
            "1. inclusive% 可能 >100%（递归/重复计数）；self% 是函数自身指令占比；"
            "perf 按符号聚合，同名不同 DSO 已合并（需要区分时用 "
            "`perf report --sort dso,symbol`）。")
    else:
        备注.append("1. 本轮无指令级数据（后端 none），不做 self/inclusive 解读。")
    备注 += [
        "2. 墙钟受多会话/负载扰动；收益判定以指令数为准（plans/032 §2.1/§3）。",
        "3. 冷/热缓存口径必须分开报告；缓存命中不得冒充编译门禁收益（plans/032 §2.3）。",
        "4. 本报告不设阈值、不参与门禁；它是 #367 端到端基线的归因补充。",
        "",
    ]
    return "\n".join(行们 + 备注)


# ============ JSON ============

def 写结果JSON(路径: str, 目标: str, 后端信息: dict, 负载: dict,
               结果们: list[dict]) -> pathlib.Path:
    json路径 = pathlib.Path(路径)
    json路径.parent.mkdir(parents=True, exist_ok=True)
    json路径.write_text(json.dumps(
        {"目标": 目标, "后端": 后端信息, "负载": 负载["显示"],
         "结果们": 结果们}, ensure_ascii=False, indent=2), encoding="utf-8")
    return json路径
