#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_build_freshness.py —— 构建产物新鲜度机械核验（872·gate_quick 构建层防线）。

背景（任务 #872·652 集成反复卡 L1 实录）：ninja 增量构建「旧 obj 假新鲜」——
源内容已变但 mtime 假旧时构建系统不重编该 obj，链接行混合新旧 obj → LNK2005
多重定义假红（同树同代码 clean 全量 286/286 绿·增量必红）。088 的「每轮删
.ninja_deps 强制全量重扫」（wt.py 模板·win 主路径）已堵坑主体；本核验器是第二道
机械防线（795 只治 run_e2e exe 层·build obj 层同病未治）：构建后核验产物-输入
时间戳闭包，把「测旧物」从静默变显式名单。

核验面（构建后运行·gate_quick 构建步骤后调用）：
  ① 编译边：obj 缺失（构建图声明未落地）/ 源缺失 / obj.mtime < 源.mtime
     （漏编=假新鲜主判据——探针实证：源内容变+回拨 mtime→ninja 只重编另一个
     obj→LNK2005·见 target/probe872 探针与提交信息）；
  ② 库/链接边：产物缺失 / 产物.mtime < 任一输入（主+implicit）mtime（链接滞后）；
  ③ 核心产物（target/cn.exe、cn_unit_tests.exe 等）vs HEAD 提交时刻（rebase/
     检出后未重建形态·795 判据 a 的构建层版·判据语义与 tests/e2e/exe新鲜度 一致）；
  ④ 游离产物（*.obj/*.lib 不在当前构建图内）=黄提示（历史残留·不红——正常树
     也含 cmake 探测产物 2~3 个，红=误报税）。
类型判定与豁免边界（显式声明·防误报）：
  · 含未展开 ninja 变量（${...}）的边、phony/非 .obj/.lib/.exe 产物边=跳过核验；
  · 源 mtime 对照只覆盖「有编译边的源」——头文件级依赖正确性由 ninja deps 与
    088 全量重扫承担（本器不重复建 .ninja_deps 解析）；
  · Make 布局模式（linux build-quick·无 build.ninja）：obj→源按 CMakeFiles/*.dir
    布局映射，源在仓库根外（munged 名 =_/C_/E_ 段）的 obj 计入豁免计数不红；
  · 时间戳判据=严格小于（同秒不报——构建极快时源/obj 同秒为常态）。
rc：0=全绿（可含黄提示）；1=存在确凿不新鲜（红名单——构建机制失灵，继续跑门禁
    =测旧物；处置：ninja -t clean 或删构建目录后重建）。

用法：python scripts/check_build_freshness.py [--构建目录 <dir>] [--项目根 <dir>]
     [--上限 20]      （--项目根 缺省=本脚本所在仓库·自测注入临时树用）
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent      # main 里可被 --项目根 覆盖
_对象后缀 = {".obj", ".o", ".lib", ".a"}
_源后缀 = {".cpp", ".cc", ".cxx", ".c"}
_产物后缀 = {".exe", ".lib", ".a", ".dll", ".so"}


def 规一(路径: Path) -> str:
    """路径归一键（normcase：win 大小写不敏感——防大小写差造成假游离/假缺输入）。"""
    try:
        return os.path.normcase(str(路径.resolve()))
    except OSError:
        return os.path.normcase(str(路径))


def 短(路径: Path) -> str:
    try:
        return str(路径.relative_to(仓库根))
    except ValueError:
        return str(路径)


def 时分秒(秒: float) -> str:
    if 秒 >= 3600:
        return f"{秒 / 3600:.1f}h"
    if 秒 >= 60:
        return f"{秒 / 60:.1f}m"
    return f"{秒:.0f}s"


def _破转义(词: str) -> str:
    return 词.replace("\x01", " ").replace("\x02", ":").replace("$:", ":").replace("$$", "$")


def 分词(段: str) -> list[str]:
    """ninja 段→词表：$ 空格转义先占位防切分，路径反斜杠统一正斜杠。"""
    段 = 段.replace("$ ", "\x01")
    return [_破转义(t).replace("\\", "/") for t in 段.split()]


def _拼接续行(文本: str) -> list[str]:
    出, 累积 = [], ""
    for 行 in 文本.splitlines():
        累积 += 行
        if 累积.endswith("$") and not 累积.endswith("$$") and not 累积.endswith("$ "):
            累积 = 累积[:-1]
            continue
        出.append(累积)
        累积 = ""
    if 累积:
        出.append(累积)
    return 出


def 解析ninja图(构建目录: Path) -> tuple[list, set, int]:
    """解析 build.ninja → (边列表, 全部输出归一集, 跳过的变量边数)。

    边=(产物Path, 输入Path列表, 是否编译边, 是否链接边)；输入=主+implicit
    段（order-only 不触发重建·不纳入）；含未展开变量的输出边整体跳过（豁免）。"""
    文本 = (构建目录 / "build.ninja").read_text(encoding="utf-8", errors="replace")
    边们, 输出集, 跳过 = [], set(), 0
    for 行 in _拼接续行(文本):
        # $: 转义（C$:\... 路径冒号）必须先占位再切边——否则正则 (.+?): 在
        #   「C$」处截断，链接/库边（产物=绝对路径）全部错行丢失。占位符
        #   \x02 在 _破转义 还原为冒号。
        行 = 行.replace("$:", "\x02")
        m = re.match(r"^build\s+(.+?):\s*(\S+)\s*(.*)$", 行)
        if not m or m.group(2) == "phony":
            continue
        输出段, 输入段 = m.group(1), m.group(3)
        if "${" in 输出段 or "${" in 输入段:
            跳过 += 1
            continue
        输出词 = 分词(输出段.split("|")[0].strip())
        输入词 = 分词(输入段)
        if not 输出词:
            continue
        产物 = 构建目录 / 输出词[0] if not _是绝对(输出词[0]) else Path(输出词[0])
        输出集.add(规一(产物))
        主入, 隐入, 段位 = [], [], 0
        for t in 输入词:
            if t == "|":
                段位 = 1
                continue
            if t == "||":
                break
            p = Path(t) if _是绝对(t) else 构建目录 / t
            (主入 if 段位 == 0 else 隐入).append(p)
        输入们 = 主入 + 隐入
        编译边 = 产物.suffix.lower() in (".obj", ".o") and any(
            i.suffix.lower() in _源后缀 for i in 输入们)
        链接边 = 产物.suffix.lower() in _产物后缀 and any(
            i.suffix.lower() in _对象后缀 for i in 输入们)
        边们.append((产物, 输入们, 编译边, 链接边))
    return 边们, 输出集, 跳过


def _是绝对(词: str) -> bool:
    return bool(re.match(r"^[A-Za-z]:/", 词)) or 词.startswith("/")


def 核验ninja图(构建目录: Path, 红: list, 黄: list) -> tuple[int, int, int]:
    边们, 输出集, 跳过 = 解析ninja图(构建目录)
    编译数 = 链接数 = 0
    for 产物, 输入们, 编译边, 链接边 in 边们:
        if 编译边:
            编译数 += 1
            if not 产物.exists():
                红.append(f"{短(产物)} 未产出（构建图声明未落地）")
                continue
            源 = 输入们[0] if 输入们 else None
            if 源 is None:
                continue
            if not 源.exists():
                红.append(f"{短(产物)} 的源缺失：{短(源)}（陈旧构建图）")
                continue
            差 = 源.stat().st_mtime - 产物.stat().st_mtime
            if 差 > 0:
                红.append(f"{短(产物)} 旧于源 {时分秒(差)}"
                          f"（{短(源)}）——漏编/旧 obj 假新鲜面（872 LNK2005 同族）")
        elif 链接边:
            链接数 += 1
            if not 产物.exists():
                红.append(f"{短(产物)} 未产出（构建图声明未落地）")
                continue
            for i in 输入们:
                if not i.exists():
                    红.append(f"{短(产物)} 的输入缺失：{短(i)}")
                    break
                差 = i.stat().st_mtime - 产物.stat().st_mtime
                if 差 > 0:
                    红.append(f"{短(产物)} 旧于输入 {短(i)} {时分秒(差)}（链接滞后）")
                    break
    # 游离产物（黄·历史残留提示——不属于任何当前构建边输出）
    游离 = []
    for p in 构建目录.rglob("*"):
        try:
            if p.is_file() and p.suffix.lower() in _对象后缀 and 规一(p) not in 输出集:
                游离.append(p)
        except OSError:
            continue
    if 游离:
        示例 = "、".join(短(p) for p in 游离[:5])
        黄.append(f"游离产物 {len(游离)} 个不在当前构建图（历史残留·不参与链接·"
                  f"建议 ninja -t clean 清场）：{示例}{'……' if len(游离) > 5 else ''}")
    if 跳过:
        黄.append(f"{跳过} 条含未展开变量的边跳过核验（豁免边界·见模块头）")
    return 编译数, 链接数, 跳过


def 核验make布局(构建目录: Path, 红: list, 黄: list) -> tuple[int, int]:
    """Make 生成器布局（linux build-quick）：CMakeFiles/*.dir/** 下 obj→源按相对
    路径映射；源在仓库根外的 munged 名（首段含 _ 转码）计入豁免计数不红。"""
    全部obj = [p for p in 构建目录.rglob("*")
               if p.is_file() and p.suffix.lower() in (".obj", ".o")]
    映射数 = 豁免数 = 0
    for obj in 全部obj:
        相对 = None
        for 段根 in (构建目录 / "CMakeFiles").glob("*.dir"):
            try:
                相对 = obj.relative_to(段根)
                break
            except ValueError:
                continue
        if 相对 is None:
            豁免数 += 1
            continue
        # CMake Make 布局 obj 名=源全名+.o（src/a.cpp.o → 源 src/a.cpp）
        源 = 仓库根 / 相对.with_suffix("")
        if not 源.exists():
            豁免数 += 1          # 外部源 munged 名（gtest 等）——豁免边界，见模块头
            continue
        映射数 += 1
        差 = 源.stat().st_mtime - obj.stat().st_mtime
        if 差 > 0:
            红.append(f"{短(obj)} 旧于源 {时分秒(差)}（{短(源)}）——漏编/假新鲜面")
    # 链接产物 vs 全部 obj（构建目录内 obj 皆属编译链·exe 必须新于每个输入）
    for 名 in ("cn.exe", "cn", "cn_unit_tests.exe", "cn_unit_tests"):
        产物 = 仓库根 / "target" / 名
        if not 产物.exists():
            continue
        产物时刻 = 产物.stat().st_mtime
        for obj in 全部obj:
            差 = obj.stat().st_mtime - 产物时刻
            if 差 > 0:
                红.append(f"target/{名} 旧于输入 {短(obj)} {时分秒(差)}（链接滞后）")
                break
    if 豁免数:
        黄.append(f"{豁免数} 个 obj 无法映射到仓库内源（外部源 munged 名·豁免边界）")
    return 映射数, 豁免数


def 核验HEAD判据(红: list, 黄: list) -> None:
    """核心产物 vs HEAD 提交时刻（795 判据 a 同语义：exe 早于 HEAD=rebase/检出后
    未重建）。HEAD 不可知/产物不存在=黄字跳过（gate_quick 构建后在 pipeline 里，
    此判据是兜底——构建机制失灵时的最后防线）。"""
    产物们 = [仓库根 / "target" / 名 for 名 in
             ("cn.exe", "cn_unit_tests.exe", "cn", "cn_unit_tests")]
    存在们 = [p for p in 产物们 if p.exists()]
    if not 存在们:
        黄.append("核心产物（target/cn*）不存在——HEAD 判据跳过")
        return
    try:
        结 = subprocess.run(["git", "log", "-1", "--format=%ct"], cwd=str(仓库根),
                           capture_output=True, text=True, timeout=60)
        头时刻 = int((结.stdout or "").strip()) if 结.returncode == 0 else None
    except (subprocess.TimeoutExpired, ValueError, OSError):
        头时刻 = None
    if 头时刻 is None:
        黄.append("git HEAD 提交时刻不可知——HEAD 判据跳过")
        return
    for p in 存在们:
        if p.stat().st_mtime < 头时刻:
            红.append(f"{短(p)} 早于当前 HEAD 提交时刻（rebase/检出后未重建·"
                      f"795 判据 a 构建层版）")


def main() -> int:
    global 仓库根
    解析器 = argparse.ArgumentParser(description="构建产物新鲜度机械核验（872）")
    解析器.add_argument("--构建目录", default=None,
                       help="构建目录（缺省 win=target/build-ninja、linux=target/build-quick）")
    解析器.add_argument("--项目根", default=None,
                       help="项目根（缺省=本脚本所在仓库·自测注入临时树用）")
    解析器.add_argument("--上限", type=int, default=20, help="红/黄名单打印条数上限")
    参数 = 解析器.parse_args()
    if 参数.项目根:
        仓库根 = Path(参数.项目根).resolve()
    是win = sys.platform.startswith("win")
    if 参数.构建目录:
        构建目录 = Path(参数.构建目录)
    else:
        # 缺省=本平台主目录；主目录不存在回退另一候选（裸跑鲁棒·gate_quick 显式传）
        主 = "build-ninja" if 是win else "build-quick"
        备 = "build-quick" if 是win else "build-ninja"
        候选 = [仓库根 / "target" / 主, 仓库根 / "target" / 备]
        构建目录 = next((c for c in 候选 if c.exists()), 候选[0])
    if not 构建目录.is_absolute():
        构建目录 = (仓库根 / 构建目录).resolve()
    if not 构建目录.exists():
        print(f"[新鲜度] 构建目录不存在（{短(构建目录)}）——未构建树·跳过核验")
        return 0

    红: list[str] = []
    黄: list[str] = []
    起 = time.time()
    if (构建目录 / "build.ninja").exists():
        编译数, 链接数, _ = 核验ninja图(构建目录, 红, 黄)
        模式 = f"ninja 图（编译边 {编译数}·链接/库边 {链接数}）"
    elif (构建目录 / "CMakeCache.txt").exists():
        映射数, _ = 核验make布局(构建目录, 红, 黄)
        模式 = f"Make 布局（可映射 obj {映射数}）"
    else:
        print(f"[新鲜度] 构建目录无 build.ninja/CMakeCache.txt（{短(构建目录)}）——跳过核验")
        return 0
    核验HEAD判据(红, 黄)

    print(f"[新鲜度] 构建目录 {短(构建目录)}·{模式}（{time.time() - 起:.1f}s）")
    for 条 in 黄:
        print(f"  [黄] {条}")
    if 红:
        print(f"[新鲜度] ✗ {len(红)} 项不新鲜（虚假新鲜/链接滞后=构建机制失灵，"
              f"继续跑门禁=测旧物）：")
        for 条 in 红[:参数.上限]:
            print(f"  · {条}")
        if len(红) > 参数.上限:
            print(f"  · ……另 {len(红) - 参数.上限} 项（--上限 调大看全）")
        print("[新鲜度] 处置：ninja -t clean 或删构建目录后重跑（clean 兜底）；"
              "若为 sccache 命中污染面见 #859")
        return 1
    print("[新鲜度] ✓ 全部核验面新鲜（obj/链接产物/HEAD 判据）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
