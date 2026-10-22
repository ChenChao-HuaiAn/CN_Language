#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""perf_sample.py —— CN 编译器热点常态化采样统一入口（#1226）。

定位（与 #367 基准设施的分工）：
  · bench_self_host.py  = 端到端时间基线（M1/M2/M5·自举全树墙钟/CPU）。
  · bench_stdlib.py     = 生成代码/运行时微基准（b1~b8 墙钟·双编译）。
  · 本工具              = **函数级归因**（谁在消耗指令/self%/inclusive%），
                          不替代上两者，也不设置性能门禁阈值。

后端：
  · linux（x86_64/aarch64）：优先 perf（perf stat 指令数 + perf record 热点）；
    PMU 不可用时自动降级 callgrind。
  · win：原生 PE 无 perf/valgrind 面——默认只给墙钟降级报告；热点归因走
    WSL callgrind 代理（--proxy-compiler 指向同源 linux-x64 构建）。
  · linux-arm64：本机 aarch64 原生 perf；x86_64 宿主 qemu 只能给墙钟/相对时间，
    指令级归因=诚实边界（脚本显式拒绝，退出码 3）。

用法示例：
  python scripts/perf_sample.py --target linux-x64 --load v2-build --runs 3
  python scripts/perf_sample.py --target win-x64 --load v2-build --runs 3
  python scripts/perf_sample.py --target linux-x64 --load v2-build \
      --cache hot --runs 3
  python scripts/perf_sample.py --load exec --exec target/bench/b4_JSON解析.host
  python scripts/perf_sample.py --selftest

本脚本只读采样/生成报告到 target/perf/（gitignore），不改部署、不建 cron。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
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
        try:
            return subprocess.run(命令, capture_output=True, text=True,
                                  encoding="utf-8", errors="replace", timeout=超时)
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
            raise 环境错误("win-x64 目标只能在 Windows 主机执行")
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


def 选择后端(请求: str, 目标: str, 主机: dict, 工具: dict,
             有代理: bool) -> dict:
    """返回 {后端, 代理, 原因}。auto 永不在无工具时假装有指令数。"""
    if 请求 == "none":
        return {"后端": "none", "代理": False, "原因": "用户显式 --backend none"}
    if 请求 == "perf":
        if not 工具["PMU"]:
            raise 平台边界("显式要求 perf，但当前环境 PMU 不可用（No supported events）")
        return {"后端": "perf", "代理": False, "原因": "用户显式 perf"}
    if 请求 == "callgrind":
        if not (工具["valgrind"] and 工具["callgrind_annotate"]):
            raise 平台边界("显式要求 callgrind，但缺少 valgrind/callgrind_annotate")
        return {"后端": "callgrind", "代理": bool(有代理), "原因": "用户显式 callgrind"}
    # auto
    if 目标 == "linux-arm64" and 主机["架构"] not in ("aarch64", "arm64"):
        raise 平台边界(
            "linux-arm64 指令级采样需要 aarch64 原生 perf；x86_64 上 qemu 仅能测墙钟，"
            "不得用 qemu 结果宣称指令数收益。请在 arm64 真机运行，或改用 --target linux-x64 代理。")
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
        return {"CN_NO_AST_CACHE": "1", "CN_NO_PROGRAM_CACHE": "1"}
    return {}


def _是ELF(路径: pathlib.Path) -> bool:
    try:
        with open(路径, "rb") as 文件:
            return 文件.read(4) == b"\x7fELF"
    except OSError:
        return False


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
        可执行 = pathlib.Path(参数.exec).resolve()
        if not 可执行.exists():
            raise 环境错误(f"被测可执行文件不存在: {可执行}")
        环境 = {} if 参数.cache == "asis" else 缓存环境(参数.cache)
        参数们 = [_转换参数(执行器, str(x)) for x in (参数.arg or [])]
        return {"argv": [执行器.路径(可执行)] + 参数们,
                "cwd": 项目根, "环境": 环境,
                "显示": f"{可执行} {' '.join(参数们)}".strip(),
                "输出": None}
    # v2-build
    代理 = bool(后端信息.get("代理"))
    编译器 = pathlib.Path(参数.proxy_compiler).resolve() if 代理 and 参数.proxy_compiler else None
    if 编译器 is None:
        编译器 = pathlib.Path(参数.compiler).resolve() if 参数.compiler else _默认编译器(目标)
    if 编译器 is None or not 编译器.exists():
        raise 环境错误("未找到宿主编译器：先构建 target/cn(.exe) 或用 --compiler 指定")
    if (后端信息["后端"] == "callgrind"
            or (执行器.用WSL and 目标 == "linux-x64")) and not _是ELF(编译器):
        raise 环境错误(
            f"callgrind 只能采样 ELF：{编译器} 不是 ELF（win 原生 PE 请用 --proxy-compiler "
            "指定同源 linux-x64 构建；linux 目标请用 --compiler 指定 ELF 编译器）")
    if 代理:
        编译目标 = "linux-x86_64"
    else:
        编译目标 = 平台映射[目标]
    入口 = pathlib.Path(参数.entry).resolve() if 参数.entry else 默认入口
    if not 入口.exists():
        raise 环境错误(f"v2 入口不存在: {入口}")
    输出 = pathlib.Path(参数.output).resolve() if 参数.output else (
        采样目录 / ("perf_load.exe" if 编译目标 == "win-x64" else "perf_load.bin"))
    输出.parent.mkdir(parents=True, exist_ok=True)
    if 输出.exists():
        输出.unlink()
    环境 = 缓存环境(参数.cache)
    编译器参数 = 执行器.路径(编译器)
    入口参数 = 执行器.路径(入口)
    输出参数 = 执行器.路径(输出)
    return {"argv": [编译器参数, "build", 入口参数,
                     "--target", 编译目标, "--output", 输出参数],
            "cwd": 项目根, "环境": 环境,
            "显示": f"{编译器} build {入口} --target {编译目标} --output {输出}",
            "输出": 输出}


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
    if not 唯一 or 执行器 is None or not 执行器.有工具("c++filt"):
        return {x: x for x in 唯一}
    结果 = 执行器.运行(["c++filt"] + 唯一, 超时=60)
    if 结果.returncode != 0:
        return {x: x for x in 唯一}
    映射 = [x for x in (结果.stdout or "").splitlines() if x.strip()]
    if len(映射) != len(唯一):
        return {x: x for x in 唯一}
    return dict(zip(唯一, 映射))


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


def 跑perf统计(执行器: 执行器, 负载: dict, 超时: int) -> tuple[list[float], list[float]]:
    参数 = ["perf", "stat", "-e", "instructions,task-clock", "-x,"] + list(负载["argv"])
    结果 = 执行器.运行(参数, cwd=负载["cwd"], 环境=负载["环境"], 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"perf stat 失败: {(结果.stderr or '').strip()[:300]}")
    return 解析perf统计((结果.stdout or "") + "\n" + (结果.stderr or ""))


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
    开始 = time.perf_counter()
    结果 = {"轮号": 轮号, "指令": [], "task_clock": [], "热点": [], "总指令": 0}
    if 后端 == "callgrind":
        输出文件 = 采样目录 / f"callgrind.out.{os.getpid()}.{轮号}"
        运行结果 = 跑callgrind(执行器, 负载, 输出文件, 参数.timeout)
        if 运行结果.returncode != 0:
            raise 负载失败(f"callgrind 采样失败 rc={运行结果.returncode}: "
                           f"{(运行结果.stderr or '').strip()[:300]}")
        自身文本 = callgrind注解(执行器, 输出文件, False, 参数.timeout)
        包含文本 = callgrind注解(执行器, 输出文件, True, 参数.timeout)
        自身, 总数 = 解析callgrind(自身文本)
        包含, _ = 解析callgrind(包含文本)
        结果["总指令"] = 总数
        结果["指令"] = [总数]
        结果["热点"] = 合并callgrind(自身, 包含, 执行器, 参数.top)
    elif 后端 == "perf":
        指令们, 时钟们 = 跑perf统计(执行器, 负载, 参数.timeout)
        if not 指令们:
            raise 负载失败("perf stat 未给出 instructions（PMU 不支持或事件被禁）")
        结果["指令"] = 指令们
        结果["task_clock"] = 时钟们
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
                "> 下表只有墙钟参考，**不得用于宣称性能收益**（plans/032 §2/§3）。",
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
    行们 += ["## 热点 Top %d" % 参数.top, ""]
    if 后端信息["后端"] == "none":
        行们 += ["（无：后端 none 未采样，不产出热点表）", ""]
    else:
        for 结果 in 结果们:
            if 参数.runs > 1:
                行们.append(f"### 第 {结果['轮号']} 轮")
            行们 += _热点表(结果["热点"]) + [""]
    行们 += [
        "## 口径备注",
        "",
        "1. inclusive% 可能 >100%（递归/重复计数）；self% 是函数自身指令占比；同名函数"
        "多文件条目=内联副本/不同 DSO，分列保留。",
        "2. 墙钟受多会话/负载扰动；收益判定以指令数为准（plans/032 §2/§3）。",
        "3. 冷/热缓存口径必须分开报告；缓存命中不得冒充编译门禁收益（plans/032 §4）。",
        "4. 本报告不设阈值、不参与门禁；它是 #367 端到端基线的归因补充。",
        "",
    ]
    return "\n".join(行们)


# ============ 自测 ============

def _自测夹具() -> dict:
    return {
        "callgrind_self": """
Ir                  file:function
--------------------------------------------------------------------------------
  600,000 ( 60.00%)  /src/a.cpp:foo()
  300,000 ( 30.00%)  /src/b.cpp:bar()
  100,000 ( 10.00%)  ???:_start
  1,000,000 (100.0%)  PROGRAM TOTALS
""",
        "callgrind_incl": """
Ir                  file:function
--------------------------------------------------------------------------------
  900,000 ( 90.00%)  /src/a.cpp:foo()
  300,000 ( 30.00%)  /src/b.cpp:bar()
  1,000,000 (100.0%)  PROGRAM TOTALS
""",
        "perf_self": """
# Overhead  Symbol
    45.00%  [.] foo()
    25.00%  [.] bar()
""",
        "perf_children": """
# Children      Self  Symbol
    80.00%    45.00%  foo()
    30.00%    25.00%  bar()
""",
        "perf_stat": "1000,,instructions,1.0,1\n12.5,msec,task-clock,1.0,1\n",
        "perf_no_pmu": "Error:\nNo supported events found.\n",
    }


def _自测外部(断言) -> None:
    主机 = 探测主机()
    用WSL = 主机["是Windows"]
    try:
        执行器 = 造执行器(用WSL)
    except 环境错误:
        断言("外部采样跳过：无 wsl", True)
        return
    工具 = 探测工具(执行器)
    if not (工具["valgrind"] and 工具["callgrind_annotate"]):
        断言("无 valgrind：后端降级 none", True)
        return
    if not 工具["python3"]:
        断言("无 python3：外部小负载跳过", True)
        return
    代码 = "x=0\nfor i in range(5000):\n    x+=i\nprint(x)\n"
    负载 = {"argv": ["python3", "-c", 代码], "cwd": 项目根, "环境": {},
            "显示": "python3 -c <固定小负载>", "输出": None}
    输出文件 = 采样目录 / f"selftest.callgrind.{os.getpid()}"
    try:
        结果 = 跑callgrind(执行器, 负载, 输出文件, 120)
        断言("固定小负载 callgrind rc=0", 结果.returncode == 0,
             (结果.stderr or "")[:200])
        if 结果.returncode != 0:
            return
        自身文本 = callgrind注解(执行器, 输出文件, False, 120)
        自身, 总数 = 解析callgrind(自身文本)
        断言("callgrind 解析出总指令数", 总数 > 0, str(总数))
        断言("callgrind 解析出热点条目", len(自身) > 0, str(len(自身)))
        热点 = 合并callgrind(自身, 自身, 执行器, 5)
        断言("callgrind 合并/排序非空", bool(热点) and 热点[0]["self"] > 0)
    finally:
        if 输出文件.exists():
            输出文件.unlink()


def 自测() -> int:
    夹具 = _自测夹具()
    断言们: list[tuple[str, bool, str]] = []

    def 断言(名: str, 条件: bool, 详情: str = "") -> None:
        断言们.append((名, bool(条件), 详情))

    自身, 总数 = 解析callgrind(夹具["callgrind_self"])
    断言("callgrind 总指令数解析", 总数 == 1_000_000, str(总数))
    断言("callgrind 条目数解析", len(自身) == 3, str(len(自身)))
    断言("callgrind 函数名/文件拆分",
        自身.get("/src/a.cpp:foo()", {}).get("函数") == "foo()")
    包含, _ = 解析callgrind(夹具["callgrind_incl"])
    合并 = 合并callgrind(自身, 包含, 执行器=None, 热点数=2)  # type: ignore[arg-type]
    断言("callgrind self/inclusive 合并", 合并[0]["self"] == 60.0 and 合并[0]["包含"] == 90.0)
    断言("callgrind 按 self 降序", [x["self"] for x in 合并] == [60.0, 30.0])

    自身perf = 解析perf报告(夹具["perf_self"], False)
    包含perf = 解析perf报告(夹具["perf_children"], True)
    断言("perf self 解析", abs(自身perf.get("foo()", 0) - 45.0) < 1e-9)
    断言("perf children 解析", abs(包含perf.get("foo()", 0) - 80.0) < 1e-9)
    指令, 时钟 = 解析perf统计(夹具["perf_stat"])
    断言("perf stat instructions 解析", 指令 == [1000.0], str(指令))
    断言("perf stat task-clock 解析", 时钟 == [12.5], str(时钟))
    无PMU指令, _ = 解析perf统计(夹具["perf_no_pmu"])
    断言("无 PMU 输出不伪造指令数", 无PMU指令 == [])

    主机win = {"系统": "windows", "架构": "amd64", "是Windows": True}
    主机linux = {"系统": "linux", "架构": "x86_64", "是Windows": False}
    主机arm = {"系统": "linux", "架构": "aarch64", "是Windows": False}
    工具无 = {"perf": False, "PMU": False, "valgrind": False,
            "callgrind_annotate": False, "c++filt": False, "python3": True}
    工具cg = dict(工具无, valgrind=True, callgrind_annotate=True)
    断言("auto win 无代理→none", 选择后端("auto", "win-x64", 主机win, 工具cg, False)["后端"] == "none")
    断言("auto win 有代理→callgrind", 选择后端("auto", "win-x64", 主机win, 工具cg, True)["后端"] == "callgrind")
    断言("auto linux 无 PMU→callgrind", 选择后端("auto", "linux-x64", 主机linux, 工具cg, False)["后端"] == "callgrind")
    断言("auto linux 无工具→none", 选择后端("auto", "linux-x64", 主机linux, 工具无, False)["后端"] == "none")
    try:
        选择后端("auto", "linux-arm64", 主机linux, 工具cg, False)
        断言("x86 宿主 arm64 显式拒绝", False)
    except 平台边界:
        断言("x86 宿主 arm64 显式拒绝", True)
    断言("aarch64 宿主 arm64 可用", 选择后端("auto", "linux-arm64", 主机arm,
                                               dict(工具无, perf=True, PMU=True), False)["后端"] == "perf")

    断言("WSL 路径转换", _wsl路径(pathlib.Path(r"C:\Users\x\a b.cn")) == "/mnt/c/Users/x/a b.cn")
    断言("冷缓存环境", 缓存环境("cold") == {"CN_NO_AST_CACHE": "1", "CN_NO_PROGRAM_CACHE": "1"})
    断言("热缓存环境", 缓存环境("hot") == {})

    参数 = argparse.Namespace(load="exec", exec=sys.executable, arg=["-c", "pass"],
                             cache="asis", compiler=None, proxy_compiler=None,
                             entry=None, output=None)
    负载 = 构造负载(参数, "linux-x64", {"代理": False}, 造执行器(False))
    断言("exec 负载构造", 负载["argv"][0] == str(pathlib.Path(sys.executable).resolve()))
    参数v2 = argparse.Namespace(load="v2-build", exec="", arg=[],
                              cache="cold", compiler=sys.executable,
                              proxy_compiler="", entry="", output="")
    负载v2 = 构造负载(参数v2, "linux-x64", {"后端": "none", "代理": False},
                     造执行器(False))
    断言("v2-build 负载构造", 负载v2["argv"][1] == "build"
         and 负载v2["环境"].get("CN_NO_AST_CACHE") == "1")
    报告 = 生成报告(argparse.Namespace(load="exec", cache="asis", runs=1, top=5,
                                      timeout=30),
                     "linux-x64", {"后端": "none", "代理": False, "原因": "自测"},
                     负载, [{"轮号": 1, "总指令": 0, "task_clock": [], "热点": [],
                            "墙钟秒": 0.01}])
    断言("none 报告含收益禁令", "不得用于宣称性能收益" in 报告)
    报告cg = 生成报告(argparse.Namespace(load="exec", cache="cold", runs=1, top=5,
                                        timeout=30),
                      "linux-x64", {"后端": "callgrind", "代理": False, "原因": "自测"},
                      负载, [{"轮号": 1, "总指令": 1000, "task_clock": [],
                             "热点": 合并, "墙钟秒": 0.1}])
    断言("callgrind 报告含 Ir 中位", "callgrind Ir 中位" in 报告cg)
    断言("callgrind 报告含热点表", "`foo()`" in 报告cg)

    _自测外部(断言)

    失败 = [x for x in 断言们 if not x[1]]
    for 名, 通过, 详情 in 断言们:
        print(f"[{'PASS' if 通过 else 'FAIL'}] {名}" + (f"  ← {详情}" if not 通过 and 详情 else ""))
    print(f"\n自测结果: {len(断言们) - len(失败)}/{len(断言们)} 通过")
    return 退出成功 if not 失败 else 1


# ============ 主程序 ============

def 建解析器() -> argparse.ArgumentParser:
    解析 = argparse.ArgumentParser(
        description="CN 编译器热点采样统一入口（#1226）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例：python scripts/perf_sample.py --target linux-x64 --load v2-build --runs 3")
    解析.add_argument("--target", choices=合法目标, default="auto",
                      help="目标平台（auto=按主机探测）")
    解析.add_argument("--backend", choices=合法后端, default="auto",
                      help="采样后端（auto=perf 优先·无 PMU 降级 callgrind）")
    解析.add_argument("--load", choices=合法负载, default="v2-build",
                      help="负载：v2-build=编译 v2 全树；exec=运行指定可执行文件")
    解析.add_argument("--compiler", default="", help="宿主编译器路径（默认 target/cn(.exe)）")
    解析.add_argument("--proxy-compiler", default="",
                      help="win 热点归因的 WSL 可执行 linux-x64 编译器（同源构建）")
    解析.add_argument("--entry", default="", help="v2-build 入口（默认 CN语言编译器v2/主.cn）")
    解析.add_argument("--output", default="", help="v2-build 产物路径（默认 target/perf/perf_load.*）")
    解析.add_argument("--exec", default="", help="--load exec 的可执行文件")
    解析.add_argument("--arg", action="append", default=[], help="--load exec 的参数（可重复）")
    解析.add_argument("--cache", choices=合法缓存, default="cold",
                      help="缓存口径：cold=禁用 AST/程序缓存；hot=先预热再测；asis=原样")
    解析.add_argument("--runs", type=int, default=默认轮数, help=f"采样轮数（默认 {默认轮数}）")
    解析.add_argument("--top", type=int, default=默认热点数, help=f"热点条数（默认 {默认热点数}）")
    解析.add_argument("--timeout", type=int, default=默认超时秒, help="单命令超时秒")
    解析.add_argument("--wsl-distro", default=os.environ.get("CN_WSL_DISTRO", ""),
                      help="WSL 发行版（默认用系统默认发行版）")
    解析.add_argument("--report", default=str(采样目录 / "采样报告.md"),
                      help="报告输出路径（默认 target/perf/采样报告.md）")
    解析.add_argument("--json", default="", help="另存 JSON 结果路径")
    解析.add_argument("--selftest", action="store_true", help="内置自测（固定小负载+解析+降级路径）")
    return 解析


def 主程序() -> int:
    参数 = 建解析器().parse_args()
    if 参数.selftest:
        return 自测()
    if 参数.runs < 1:
        print("错误: --runs 必须 >=1", file=sys.stderr)
        return 退出用法
    if 参数.top < 1:
        print("错误: --top 必须 >=1", file=sys.stderr)
        return 退出用法
    try:
        主机 = 探测主机()
        目标 = 解析目标(参数.target, 主机)
        执行器, 工具 = 准备执行面(主机, 目标, 参数)
        有代理 = bool(参数.proxy_compiler)
        后端信息 = 选择后端(参数.backend, 目标, 主机, 工具, 有代理)
        if 执行器.用WSL and 后端信息["后端"] != "callgrind" and 目标 == "win-x64":
            执行器 = 造执行器(False)
        负载 = 构造负载(参数, 目标, 后端信息, 执行器)
        if 参数.cache == "hot":
            print("[预热] 热缓存口径先跑一轮不计入样本...", flush=True)
            预热执行(执行器, 负载, 参数.timeout)
        结果们 = []
        for 轮号 in range(1, 参数.runs + 1):
            print(f"[{轮号}/{参数.runs}] {后端信息['后端']} 采样中...", flush=True)
            结果们.append(跑一轮(执行器, 后端信息, 负载, 参数, 轮号))
        报告 = 生成报告(参数, 目标, 后端信息, 负载, 结果们)
        报告路径 = pathlib.Path(参数.report)
        报告路径.parent.mkdir(parents=True, exist_ok=True)
        报告路径.write_text(报告, encoding="utf-8")
        if 参数.json:
            json路径 = pathlib.Path(参数.json)
            json路径.parent.mkdir(parents=True, exist_ok=True)
            json路径.write_text(json.dumps(
                {"目标": 目标, "后端": 后端信息, "负载": 负载["显示"],
                 "结果们": 结果们}, ensure_ascii=False, indent=2), encoding="utf-8")
        print(报告)
        print(f"\n报告: {报告路径}")
        return 退出成功
    except 平台边界 as 异常:
        print(f"[平台边界] {异常}", file=sys.stderr)
        return 退出边界
    except 环境错误 as 异常:
        print(f"[环境错误] {异常}", file=sys.stderr)
        return 退出用法
    except 负载失败 as 异常:
        print(f"[负载失败] {异常}", file=sys.stderr)
        return 退出负载


def 准备执行面(主机: dict, 目标: str, 参数) -> tuple[执行器, dict]:
    """Windows 先探测 WSL；linux 目标必须走 WSL，win 目标无 WSL 时退回原生墙钟。"""
    if not 主机["是Windows"]:
        执行器 = 造执行器(False)
        return 执行器, (探测工具(执行器) if 参数.backend != "none" else _空工具())
    try:
        wsl执行器 = 造执行器(True, 参数.wsl_distro)
        if 目标 == "linux-x64" and not wsl执行器.有工具("sh"):
            raise 环境错误("linux-x64 目标在 Windows 上需要可用的 WSL（sh 不可达）")
        工具 = 探测工具(wsl执行器) if 参数.backend != "none" else _空工具()
        return wsl执行器, 工具
    except 环境错误:
        if 目标 == "linux-x64":
            raise
        return 造执行器(False), _空工具()


def _空工具() -> dict:
    return {"perf": False, "PMU": False, "valgrind": False,
            "callgrind_annotate": False, "c++filt": False, "python3": False}


def 预热执行(执行器: 执行器, 负载: dict, 超时: int) -> None:
    结果 = 执行器.运行(list(负载["argv"]), cwd=负载["cwd"],
                       环境=负载["环境"], 超时=超时)
    if 结果.returncode != 0:
        raise 负载失败(f"预热失败 rc={结果.returncode}: {(结果.stderr or '').strip()[:300]}")


if __name__ == "__main__":
    sys.exit(主程序())
