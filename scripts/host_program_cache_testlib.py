#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""宿主程序级缓存回归测试支撑库（#1431·复用 #1273 v2 缓存 harness 形态）。

复用面（直接 import scripts/v2_program_cache_testlib.py 的通用部件）：
  计数器/统计七字段解析/去统计/是祖先——两侧统计行格式逐字相同
  （[程序缓存] 命中=.. 首建=.. 失效重建=.. 损坏回退=.. 写入=.. 写失败=.. 验证不符=..）。
宿主专有面：
  - 真实调用 target/cn.exe compile（生产链路：闭包扫描→缓存命中/首建→写 asm）；
  - CNPGMC01 条目格式（52 字节头 + 变长载荷）独立解析/校验和复算/重构；
  - 默认缓存目录=<项目根>/target/cache/program（项目根=stdlibDir 父目录）与
    CN_PROGRAM_CACHE_DIR 覆盖；
  - 编译器身份测试用同目录副本（stdlib 探测不受影响）。

测试本体见 scripts/test_host_program_cache.py（七组场景·文件体量红线拆分）。
"""
from __future__ import annotations

import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from v2_program_cache_testlib import (  # noqa: E402,F401  通用 harness 复用
    仓库根,
    计数器,
    统计,
    统计键,
    统计行正则,
    统计行特征,
    去统计,
    是祖先,
)

默认目标 = "win-x64" if sys.platform.startswith("win") else "linux-x86_64"
缓存目录相对 = pathlib.Path("target") / "cache" / "program"
AST缓存目录相对 = pathlib.Path("target") / "cache" / "ast"

环境统计 = {"CN_PROGRAM_CACHE_STATS": "1"}
环境禁用 = {"CN_NO_PROGRAM_CACHE": "1"}
环境验证 = {"CN_PROGRAM_CACHE_VERIFY": "1"}

魔术 = b"CNPGMC01"
头条 = 52          # 魔数 8 + 5×u64 + 载荷长 u32
载荷头 = 16        # 汇编长/human 长/JSON 长/保留
FNV基准 = 14695981039346656037
FNV质数 = 1099511628211
掩码64 = (1 << 64) - 1
条目名正则 = re.compile(r"[0-9a-f]{32}\.cnpg")


def fnv1a64(数据: bytes) -> int:
    值 = FNV基准
    for 字节 in 数据:
        值 = ((值 ^ 字节) * FNV质数) & 掩码64
    return 值


def 建工作区(根: pathlib.Path, 源文件们: dict) -> pathlib.Path:
    """新建隔离工作区（target/ + 源文件）；缓存目录由运行cn 显式注入隔离。"""
    根 = pathlib.Path(根).resolve()
    if 根.exists():
        shutil.rmtree(根)
    (根 / "target").mkdir(parents=True)
    for 名, 内容 in 源文件们.items():
        路径 = 根 / 名
        路径.parent.mkdir(parents=True, exist_ok=True)
        路径.write_text(内容, encoding="utf-8")
    return 根


def 运行cn(cn, 区: pathlib.Path, 入口: str, 目标: str, 额外参数=(),
          环境=None, 超时秒: float = 180, 目录隔离: bool = True,
          缓存目录=None) -> subprocess.CompletedProcess:
    """真实调用宿主 cn.exe compile（cwd=隔离工作区）。

    目录隔离=True 时注入 CN_PROGRAM_CACHE_DIR / CN_AST_CACHE_DIR（每工作区独立，
    不污染仓库 target/cache）；False=走编译器默认目录探测（默认目录场景专用）。
    """
    cn = pathlib.Path(cn).resolve()
    区 = pathlib.Path(区).resolve()
    环境全 = dict(os.environ)
    # 继承环境里可能残留的缓存开关全部清掉（测量确定性）
    for 名 in ("CN_NO_PROGRAM_CACHE", "CN_PROGRAM_CACHE_VERIFY",
               "CN_PROGRAM_CACHE_DIR", "CN_AST_CACHE_DIR", "CN_AST_CACHE_VERIFY",
               "CN_NO_AST_CACHE", "CN_NO_STR_ACCUM"):
        环境全.pop(名, None)
    if 目录隔离:
        环境全["CN_PROGRAM_CACHE_DIR"] = str(
            pathlib.Path(缓存目录).resolve() if 缓存目录 is not None
            else 区 / 缓存目录相对)
    # AST 缓存始终隔离（默认目录场景只放开程序缓存目录探测，防污染仓库缓存）
    环境全["CN_AST_CACHE_DIR"] = str(区 / AST缓存目录相对)
    环境全.update(环境 or {})
    命令 = [str(cn), "compile", 入口, "--target", 目标, *额外参数]
    try:
        return subprocess.run(
            命令, cwd=区, env=环境全, capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=超时秒)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(命令, -99, "", "超时（%ss）" % 超时秒)


def 缓存目录(区: pathlib.Path, 目录覆盖=None) -> pathlib.Path:
    if 目录覆盖 is not None:
        return pathlib.Path(目录覆盖).resolve()
    return pathlib.Path(区).resolve() / 缓存目录相对


def 缓存条目们(区: pathlib.Path, 目录覆盖=None) -> list:
    目录 = 缓存目录(区, 目录覆盖)
    return sorted(目录.glob("*.cnpg")) if 目录.exists() else []


def asm路径(区: pathlib.Path, 目标: str, 入口: str = "entry.cn") -> pathlib.Path:
    后缀 = ".asm" if 目标 == "win-x64" else ".s"
    return pathlib.Path(区).resolve() / "target" / 目标 / (pathlib.Path(入口).stem + 后缀)


def 唯一条目(t: 计数器, 区: pathlib.Path, 名称: str, 目录覆盖=None):
    条目们 = 缓存条目们(区, 目录覆盖)
    if not t.断言(len(条目们) == 1, 名称, "条目数=%d" % len(条目们)):
        return None
    return 条目们[0]


def 解析条目(t: 计数器, 路径: pathlib.Path, 名称: str):
    """独立解析 CNPGMC01 头 + 载荷三层结构并复算校验和（返回 None=已红）。"""
    try:
        原始 = 路径.read_bytes()
    except OSError as 异常:
        t.断言(False, 名称 + ": 条目可读", str(异常))
        return None
    if not t.断言(len(原始) >= 头条 + 载荷头, 名称 + ": 长度 ≥ 头+载荷头",
                  "%d B" % len(原始)):
        return None
    t.断言(原始[:8] == 魔术, 名称 + ": 魔数 CNPGMC01", repr(原始[:8]))
    键甲 = int.from_bytes(原始[8:16], "little")
    键乙 = int.from_bytes(原始[16:24], "little")
    存储校验 = int.from_bytes(原始[40:48], "little")
    载荷长 = int.from_bytes(原始[48:52], "little")
    t.断言(len(原始) - 头条 == 载荷长, 名称 + ": 头部载荷长与实际一致",
           "%d vs %d" % (载荷长, len(原始) - 头条))
    载荷 = 原始[头条:]
    t.断言(fnv1a64(载荷) == 存储校验, 名称 + ": FNV 校验和覆盖全载荷",
           "%016x" % fnv1a64(载荷))
    汇编长, human长, json长, 保留 = struct.unpack("<IIII", 载荷[:载荷头])
    t.断言(汇编长 > 0, 名称 + ": 汇编分片非空", "%d B" % 汇编长)
    t.断言(载荷头 + 汇编长 + human长 + json长 == 载荷长, 名称 + ": 载荷分片长度精确",
           "%d+%d+%d+%d" % (载荷头, 汇编长, human长, json长))
    体 = 载荷[载荷头:]
    return {"字节": 原始, "键甲": 键甲, "键乙": 键乙, "载荷长": 载荷长,
            "汇编": 体[:汇编长], "警告human": 体[汇编长:汇编长 + human长],
            "警告json": 体[汇编长 + human长:汇编长 + human长 + json长],
            "汇编长": 汇编长, "human长": human长, "json长": json长}


def 重构条目(解析: dict, 汇编: bytes, 警告human: bytes, 警告json: bytes) -> bytes:
    """按 CNPGMC01 契约重建条目（保留原键头·重算长度与校验和=校验和合法的伪造）。"""
    载荷 = (struct.pack("<IIII", len(汇编), len(警告human), len(警告json), 0) +
            汇编 + 警告human + 警告json)
    头 = bytearray(解析["字节"][:头条])
    结构 = struct.pack("<Q", fnv1a64(载荷))
    头[40:48] = 结构
    头[48:52] = struct.pack("<I", len(载荷))
    return bytes(头) + 载荷


def 简单源(计算体: str = "    返回 值 + 1;\n") -> dict:
    return {
        "entry.cn": "导入 计算;\n函数 主() -> 整32 {\n    返回 整32(加倍(7));\n}\n",
        "计算.cn": "公开:\n函数 加倍(整64 值) -> 整64 {\n" + 计算体 + "}\n",
    }


def 警告源() -> dict:
    """跨文件三条语义警告（检查 结果.正常 后未处理错误分支）——重放对照用。"""
    return {
        "main.cn": (
            "导入 工具;\n"
            "函数 甲() -> 整32 {\n"
            "    结果<整64, 整32> r = 取值();\n"
            "    如果 (r.正常) {\n"
            "        返回 1;\n"
            "    }\n"
            "    返回 0;\n"
            "}\n"
            "函数 主() -> 整32 {\n"
            "    结果<整64, 整32> s = 取值();\n"
            "    如果 (s.正常) {\n"
            "        返回 2;\n"
            "    }\n"
            "    返回 甲();\n"
            "}\n"
        ),
        "工具.cn": (
            "公开:\n"
            "函数 取值() -> 结果<整64, 整32> {\n"
            "    返回 正常(整64(3));\n"
            "}\n"
            "函数 检查(结果<整64, 整32> 值) -> 整32 {\n"
            "    如果 (值.正常) {\n"
            "        返回 3;\n"
            "    }\n"
            "    返回 0;\n"
            "}\n"
        ),
    }


def 宏源() -> dict:
    return {
        "entry.cn": (
            "函数 主() -> 整32 {\n"
            "    #如果定义 特性甲\n"
            "    返回 8;\n"
            "    #否则\n"
            "    返回 4;\n"
            "    #结束如果\n"
            "}\n"
        ),
    }


def 写货舱(区: pathlib.Path, 特性: str, 版本: str = "1.0",
           依赖: str = "") -> None:
    文本 = ('[货舱]\n名称 = "cachetest"\n版本 = "%s"\n[特性]\n启用 = ["%s"]\n%s'
            % (版本, 特性, 依赖))
    (pathlib.Path(区) / "货舱.toml").write_text(文本, encoding="utf-8")


def 建立基线(t: 计数器, cn, 区: pathlib.Path, 入口: str, 目标: str, 名称: str,
             环境=None, 目录隔离: bool = True, 缓存目录=None):
    """冷编译基线（首建+写入）→ 产物/唯一条目/条目字节。"""
    环境全 = dict(环境统计)
    环境全.update(环境 or {})
    r = 运行cn(cn, 区, 入口, 目标, (), 环境全, 目录隔离=目录隔离,
              缓存目录=缓存目录)
    t.统计断言(r, 名称 + "：首建+写入", 首建=1, 写入=1)
    asm = asm路径(区, 目标, 入口)
    产物 = asm.read_bytes() if asm.exists() else b""
    t.断言(len(产物) > 0, 名称 + "：产物非空", "%d B" % len(产物))
    条目 = 唯一条目(t, 区, 名称 + "：恰有 1 个缓存条目", 缓存目录)
    if 条目 is None:
        return None
    return {"统计结果": r, "asm": asm, "产物": 产物, "条目": 条目,
            "条目字节": 条目.read_bytes()}


def 无临时残留(t: 计数器, 根: pathlib.Path) -> None:
    根 = pathlib.Path(根).resolve()
    残留 = [str(p.relative_to(根)) for p in 根.rglob("*") if ".tmp" in p.name]
    t.断言(not 残留, "工作根下无任何 *.tmp* 残留", "、".join(残留[:5]) or "无")
    条目们 = list(根.rglob("*.cnpg"))
    t.断言(len(条目们) >= 10, "至少检查 10 个缓存条目", "%d 个" % len(条目们))
    非法 = []
    for 条目 in 条目们:
        if 条目名正则.fullmatch(条目.name) is None:
            非法.append(str(条目.relative_to(根)))
    t.断言(not 非法, "条目名一律 32hex.cnpg", "、".join(非法[:5]) or "无")
