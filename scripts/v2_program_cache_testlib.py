#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""v2 程序级缓存回归测试支撑库（#1273·适配 1272 返工版 5d7071f61）。

可复用 harness：构建 v2p / 隔离工作区 / 环境变量运行 / CNV2PC02 条目解析与
独立校验和复算 / 计数器与统计断言 / 夹具与基线。测试本体见
scripts/test_v2_program_cache.py（文件体量红线拆分）。
"""
from __future__ import annotations

import os
import pathlib
import re
import shutil
import subprocess
import sys

仓库根 = pathlib.Path(__file__).resolve().parent.parent
默认目标 = "win-x64" if sys.platform.startswith("win") else "linux-x86_64"
v2入口 = 仓库根 / "CN语言编译器v2" / "主.cn"
缓存目录相对 = pathlib.Path("target") / "cache" / "v2program"

环境统计 = {"CN_V2_PROGRAM_CACHE_STATS": "1"}
环境禁用 = {"CN_NO_V2_PROGRAM_CACHE": "1"}
环境验证 = {"CN_V2_PROGRAM_CACHE_VERIFY": "1"}

统计键 = ("命中", "首建", "失效重建", "损坏回退", "写入", "写失败", "验证不符")
统计行正则 = re.compile(
    r"\[程序缓存\] 命中=\d+ 首建=\d+ 失效重建=\d+ 损坏回退=\d+ 写入=\d+ "
    r"写失败=\d+ 验证不符=\d+\n?"
)
统计行特征 = re.compile(r"\[程序缓存\] 命中=")
条目名正则 = re.compile(r"[0-9a-f]{32}-[0-9a-f]{16}\.cnvpc")
MASK64 = (1 << 64) - 1


def 去统计(s: str) -> str:
    """移除统计行：冷/热计数天然不同；统计行外的 stderr 必须逐字节一致。"""
    return 统计行正则.sub("", s or "")


def 摘要128(data: bytes) -> tuple[int, int, int]:
    """与 src/runtime/file_api.cpp __cn_str_hash128 / hashFileImpl 同算法：
    FNV-1a 双通道（laneB 每字节掺 0x9e），无长度掺入。用于独立复算条目校验和。"""
    a = 14695981039346656037
    b = 1469598103934665603
    for 字节 in data:
        a = ((a ^ 字节) * 1099511628211) & MASK64
        b = ((b ^ (字节 + 0x9E)) * 1099511628211) & MASK64
    return a, b, 0


class 计数器:
    """PASS/FAIL 汇总：任一断言失败=最终 rc=1（失败必须显式非零退出）。"""

    def __init__(self) -> None:
        self.通过 = 0
        self.失败 = 0
        self.失败项们: list[str] = []

    def 断言(self, 条件: bool, 名称: str, 详情: str = "") -> bool:
        if 条件:
            self.通过 += 1
            print(f"  PASS {名称}" + (f"：{详情}" if 详情 else ""))
            return True
        self.失败 += 1
        self.失败项们.append(名称)
        print(f"  FAIL {名称}" + (f"：{详情}" if 详情 else ""))
        return False

    def 统计断言(self, r, 名称: str, **期望: int) -> bool:
        """七字段精确断言：未写出的字段按 0 计（多计/少计都红）。"""
        实际 = 统计(r.stderr or "")
        if not 实际:
            return self.断言(False, 名称,
                             f"未找到统计行（rc={r.returncode}）: {(r.stderr or '')[-300:]}")
        全期望 = {键: 期望.get(键, 0) for 键 in 统计键}
        不符 = {键: (实际.get(键), 值) for 键, 值 in 全期望.items() if 实际.get(键) != 值}
        return self.断言(not 不符, 名称,
                         f"统计={实际}" if not 不符 else f"期望 {全期望} 实际 {实际}")

    def 无统计断言(self, r, 名称: str) -> bool:
        return self.断言(统计行特征.search(r.stderr or "") is None, 名称,
                         (r.stderr or "")[-200:])

    def 段(self, 名称: str) -> None:
        print(f"\n== {名称} ==", flush=True)


def 统计(stderr: str) -> dict[str, int]:
    m = re.search(r"\[程序缓存\] 命中=(\d+) 首建=(\d+) 失效重建=(\d+) 损坏回退=(\d+) "
                  r"写入=(\d+) 写失败=(\d+) 验证不符=(\d+)", stderr or "")
    if not m:
        return {}
    return {k: int(v) for k, v in zip(统计键, m.groups())}


def 是祖先(父: pathlib.Path, 子: pathlib.Path) -> bool:
    """Python 3.8 兼容的 Path.is_relative_to（防 --工作根 指向仓库根/其祖先误删）。"""
    try:
        子.relative_to(父)
        return True
    except ValueError:
        return False


def 构建v2p(cn, 目标: str, 输出: pathlib.Path, 超时秒: float = 900) -> tuple[bool, str]:
    """宿主编译 v2 全树 -> v2p（真实生产链路；入参 resolve + 显式超时）。"""
    cn = pathlib.Path(cn).resolve()
    输出 = pathlib.Path(输出).resolve()
    输出.parent.mkdir(parents=True, exist_ok=True)
    try:
        r = subprocess.run(
            [str(cn), "build", str(v2入口), "--target", 目标, "--output", str(输出)],
            cwd=仓库根, capture_output=True, text=True, encoding="utf-8",
            errors="replace", timeout=超时秒,
        )
    except subprocess.TimeoutExpired:
        return False, f"v2p 构建超时（{超时秒}s）: {输出}"
    if r.returncode != 0 or not 输出.exists():
        return False, ((r.stderr or "") + (r.stdout or ""))[-2000:]
    return True, ""


def 建工作区(根: pathlib.Path, 目标: str, 源文件们: dict[str, str]) -> pathlib.Path:
    """新建隔离工作区：target/ + stdlib 链接 + 源文件（cwd 口径=默认缓存目录隔离）。"""
    根 = pathlib.Path(根).resolve()
    if 根.exists():
        shutil.rmtree(根)
    (根 / "target").mkdir(parents=True)
    stdlib = 根 / "stdlib"
    if not stdlib.exists():
        if os.name == "nt":
            subprocess.run(["cmd", "/c", "mklink", "/J", str(stdlib),
                            str(仓库根 / "stdlib")],
                           capture_output=True, check=True, timeout=60)
        else:
            os.symlink(仓库根 / "stdlib", stdlib)
    for 名, 内容 in 源文件们.items():
        路径 = 根 / 名
        路径.parent.mkdir(parents=True, exist_ok=True)
        路径.write_text(内容, encoding="utf-8")
    return 根


def 运行v2p(v2p, 区: pathlib.Path, 入口: str, 目标: str, 额外参数=(),
           环境=None, 超时秒: float = 180) -> subprocess.CompletedProcess:
    """v2p 真实调用：路径归一 + 环境变量注入（统计/禁用/验证/目录覆盖）+ 显式超时。"""
    v2p = pathlib.Path(v2p).resolve()
    区 = pathlib.Path(区).resolve()
    环境全 = dict(os.environ)
    环境全.update(环境 or {})
    命令 = [str(v2p), 入口, 目标, *额外参数]
    try:
        return subprocess.run(
            命令, cwd=区, env=环境全, capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=超时秒,
        )
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(命令, -99, "", f"超时（{超时秒}s）")


def 缓存目录(区: pathlib.Path, 目录覆盖=None) -> pathlib.Path:
    if 目录覆盖 is not None:
        return pathlib.Path(目录覆盖).resolve()
    return (pathlib.Path(区).resolve() / 缓存目录相对)


def 缓存条目们(区: pathlib.Path, 目录覆盖=None) -> list[pathlib.Path]:
    目录 = 缓存目录(区, 目录覆盖)
    return sorted(目录.glob("*.cnvpc")) if 目录.exists() else []


def asm路径(区: pathlib.Path, 目标: str) -> pathlib.Path:
    return pathlib.Path(区).resolve() / "target" / (
        "v2asm.asm" if 目标 == "win-x64" else "v2asm.s")


def 唯一条目(t: 计数器, 区: pathlib.Path, 名称: str, 目录覆盖=None):
    条目们 = 缓存条目们(区, 目录覆盖)
    if not t.断言(len(条目们) == 1, 名称, f"条目数={len(条目们)}"):
        return None
    return 条目们[0]


def 解析并校验条目(t: 计数器, 路径: pathlib.Path, 名称: str):
    """独立解析 CNV2PC02 文本头与两段载荷（格式契约回归；返回 None=已红）。"""
    try:
        原始 = 路径.read_bytes()
    except OSError as 异常:
        t.断言(False, f"{名称}: 条目可读", str(异常))
        return None
    段们 = 原始.split(b"\n", 7)
    if not t.断言(len(段们) == 8, f"{名称}: 七行头完整", f"段数={len(段们)}"):
        return None
    魔数, 键甲, 键乙, 长甲, 长乙, 校甲, 校乙, 余 = 段们
    t.断言(魔数 == b"CNV2PC02", f"{名称}: 魔数 CNV2PC02", repr(魔数))
    t.断言(all(re.fullmatch(rb"[0-9a-f]{16}", x) is not None
               for x in (键甲, 键乙, 校甲, 校乙)),
           f"{名称}: 键/校验字段均为 16 位小写十六进制")
    if not t.断言(re.fullmatch(rb"\d+", 长甲) and re.fullmatch(rb"\d+", 长乙),
                  f"{名称}: 长度行为十进制", f"{长甲!r} {长乙!r}"):
        return None
    汇编长 = int(长甲)
    警告长 = int(长乙)
    t.断言(汇编长 > 0 and 警告长 >= 0, f"{名称}: 长度非负/汇编非空",
           f"{汇编长}+{警告长}")
    t.断言(len(余) == 汇编长 + 警告长 + 1, f"{名称}: 载荷+尾换行长度精确",
           f"{len(余)} vs {汇编长 + 警告长 + 1}")
    t.断言(余[-1:] == b"\n", f"{名称}: 尾部固定换行")
    汇编 = 余[:汇编长]
    警告 = 余[汇编长:汇编长 + 警告长]
    算甲, 算乙, _ = 摘要128(长甲 + b"\n" + 长乙 + b"\n" + 汇编 + 警告)
    t.断言(算甲 == int(校甲, 16) and 算乙 == int(校乙, 16),
           f"{名称}: 校验和覆盖长度行+载荷（独立复算）",
           f"{算甲:016x}/{算乙:016x}")
    return {"字节": 原始, "魔数": 魔数, "键甲": 键甲, "键乙": 键乙,
            "长甲": 长甲, "长乙": 长乙, "校甲": 校甲, "校乙": 校乙,
            "汇编": 汇编, "警告": 警告, "汇编长": 汇编长, "警告长": 警告长}


def 重构条目(解析: dict, 汇编: bytes, 警告: bytes) -> bytes:
    """按 CNV2PC02 契约重建条目（用于构造「校验和合法的错条目」）。"""
    长甲 = str(len(汇编)).encode()
    长乙 = str(len(警告)).encode()
    甲, 乙, _ = 摘要128(长甲 + b"\n" + 长乙 + b"\n" + 汇编 + 警告)
    return (b"CNV2PC02\n" + 解析["键甲"] + b"\n" + 解析["键乙"] + b"\n" +
            长甲 + b"\n" + 长乙 + b"\n" +
            f"{甲:016x}".encode() + b"\n" + f"{乙:016x}".encode() + b"\n" +
            汇编 + 警告 + b"\n")


def 简单源(计算体: str = "    返回 值 + 1;\n") -> dict[str, str]:
    return {
        "src/主.cn": "导入 计算;\n函数 主() -> 整32 {\n    返回 整32(加倍(7));\n}\n",
        "src/计算.cn": "公开:\n函数 加倍(整64 值) -> 整64 {\n" + 计算体 + "}\n",
    }


def 警告源() -> dict[str, str]:
    """跨文件三条语义警告：验证 警告原文随载荷存储 + 命中按原顺序重放。"""
    return {
        "主.cn": (
            "导入 工具;\n"
            "函数 甲() -> 整32 {\n"
            "    结果<整64, 整32> r = 取值();\n"
            "    如果 (r.正常) {\n"
            "        IO::打印行(\"甲\");\n"
            "    }\n"
            "    返回 0;\n"
            "}\n"
            "函数 主() -> 整32 {\n"
            "    结果<整64, 整32> s = 取值();\n"
            "    如果 (s.正常) {\n"
            "        IO::打印行(\"主\");\n"
            "    }\n"
            "    返回 甲();\n"
            "}\n"
        ),
        "工具.cn": (
            "公开:\n"
            "函数 取值() -> 结果<整64, 整32> {\n"
            "    返回 某些(整64(3));\n"
            "}\n"
            "函数 检查(结果<整64, 整32> 值) -> 整32 {\n"
            "    如果 (值.正常) {\n"
            "        IO::打印行(\"工具\");\n"
            "    }\n"
            "    返回 0;\n"
            "}\n"
        ),
    }


def 货舱源() -> dict[str, str]:
    """货舱夹具（[货舱] 名称/版本 + [特性] 启用 + 条件编译）；入口用绝对路径调用。"""
    return {
        "乙.cn": "公开:\n函数 乙一() -> 字符串 { 返回 \"乙\"; }\n",
        "主.cn": (
            "导入 乙;\n"
            "函数 主() -> 整32 {\n"
            "    字符串 s = 乙一();\n"
            "#如果定义(甲)\n"
            "    s = s + \"甲\";\n"
            "#否则\n"
            "    s = s + \"无甲\";\n"
            "#结束如果\n"
            "#如果定义(乙特性)\n"
            "    s = s + \"乙\";\n"
            "#结束如果\n"
            "    IO::打印到错误(s + \"\\n\");\n"
            "    返回 0;\n"
            "}\n"
        ),
    }


def 写货舱(区: pathlib.Path, 特性: str, 版本: str = "1.0", 额外: str = "") -> None:
    (区 / "货舱.toml").write_text(
        '[货舱]\n名称 = "缓存自测"\n版本 = "%s"\n[特性]\n启用 = ["%s"]\n%s'
        % (版本, 特性, 额外), encoding="utf-8")


def 建立基线(t: 计数器, v2p, 区: pathlib.Path, 入口: str, 目标: str, 名称: str,
             环境=None, 目录覆盖=None):
    """冷编译基线（首建+写入）-> 产物/唯一条目/条目字节。多个用例共用。"""
    环境全 = dict(环境统计)
    环境全.update(环境 or {})
    r = 运行v2p(v2p, 区, 入口, 目标, (), 环境全)
    t.统计断言(r, f"{名称}：首建+写入", 首建=1, 写入=1)
    asm = asm路径(区, 目标)
    产物 = asm.read_bytes() if asm.exists() else b""
    t.断言(len(产物) > 0, f"{名称}：产物非空", f"{len(产物)} B")
    条目 = 唯一条目(t, 区, f"{名称}：恰有 1 个缓存条目", 目录覆盖)
    if 条目 is None:
        return None
    return {"统计结果": r, "asm": asm, "产物": 产物, "条目": 条目,
            "条目字节": 条目.read_bytes()}


