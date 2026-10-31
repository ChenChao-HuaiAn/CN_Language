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
import re
import shutil
import subprocess
from pathlib import Path

import ci_report   # 1385：构建起点宿主负载档（同宿主多实例可见面）

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


# ═══════════ 1385 构建缓存命中态/负载档（耗时基线口径维度） ═══════════
缓存命中行 = re.compile(r"(?i)^\s*(?:cache\s+)?hits?\s*[:=]?\s*([\d,]+)")
缓存未中行 = re.compile(r"(?i)^\s*(?:cache\s+)?miss(?:es)?\s*[:=]?\s*([\d,]+)")
# ccache 3.x：无 Summary「Hits: n」行，命中拆成 direct/preprocessed 两行（求和）
缓存3命中行 = re.compile(
    r"(?i)^\s*cache\s+hit\s*\((?:direct|preprocessed)\)\s*[:=]?\s*([\d,]+)")
缓存3未中行 = re.compile(r"(?i)^\s*cache\s+miss\s*[:=]?\s*([\d,]+)")


def 读缓存计数(launcher, 执行=None) -> dict | None:
    """ccache -s / sccache --show-stats → {命中, 未中} 累计计数；不可用/解析失败=None。

    解析口径：优先 Summary 行（ccache 4.x「Hits: n / m」/ sccache「Cache hits n」）；
    无 Summary 时回退 ccache 3.x 的「cache hit (direct)/(preprocessed)」求和。
    sccache 后续 rate 行不会覆盖计数。None=显式「缓存未知」（调用方落独立口径桶）。
    执行=subprocess.run 可注入（selftest 零真实缓存工具）。
    边界（P2-4）：ccache/sccache 是宿主共享缓存——同宿主多实例共用统计文件，
    构建前后差可能含他实例编译；档位只作粗粒度口径，跨实例污染时落相邻档，
    不承诺精确归因（精确归因需 per-instance cache dir，另任务面）。"""
    if not launcher:
        return None
    名 = Path(str(launcher)).name.lower()
    if "ccache" in 名:
        命令 = [str(launcher), "-s"]
    elif "sccache" in 名:
        命令 = [str(launcher), "--show-stats"]
    else:
        return None
    try:
        r = (执行 or subprocess.run)(命令, capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return None
    if r.returncode != 0:
        return None
    命中 = 未中 = None
    旧命中 = 0
    for 行 in (r.stdout or "").splitlines():
        if 命中 is None:
            m = 缓存命中行.match(行)
            if m:
                命中 = int(m.group(1).replace(",", ""))
        if 未中 is None:
            m = 缓存未中行.match(行)
            if m:
                未中 = int(m.group(1).replace(",", ""))
        m = 缓存3命中行.match(行)
        if m:
            旧命中 += int(m.group(1).replace(",", ""))
        if 未中 is None:
            m = 缓存3未中行.match(行)
            if m:
                未中 = int(m.group(1).replace(",", ""))
    if 命中 is None and 旧命中 > 0:
        命中 = 旧命中   # ccache 3.x 兜底（Summary 行缺失）
    if 命中 is None or 未中 is None:
        return None
    return {"命中": 命中, "未中": 未中}


def 缓存档(前: dict | None, 后: dict | None) -> str:
    """构建前后累计计数差 → 命中率档（高命中≥80% / 部分≥30% / 低命中 / 无编译）。
    计数回退（stats 重置）或缺失=「缓存未知」显式档（不猜测）。"""
    if 前 is None or 后 is None:
        return "缓存未知"
    命中 = 后["命中"] - 前["命中"]
    未中 = 后["未中"] - 前["未中"]
    if 命中 < 0 or 未中 < 0:
        return "缓存未知"
    总 = 命中 + 未中
    if 总 <= 0:
        return "无编译"
    比 = float(命中) / float(总)
    if 比 >= 0.8:
        return "高命中"
    if 比 >= 0.3:
        return "部分命中"
    return "低命中"


def 标记构建维度(步骤们: dict, 缓存态: str, 负载: str) -> None:
    """构建步骤并入 缓存态/负载档（rc 失败也记录——评估层按 rc 分流为未计入）。"""
    步 = 步骤们.get("构建")
    if not isinstance(步, dict):
        return
    步["缓存态"] = 缓存态
    步["负载档"] = 负载


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
    冷建本轮 = 清建 or not (仓库根 / "target/build-ninja/build.ninja").exists()
    if 冷建本轮:
        配置脚本 = 日志目录 / "win_configure.cmd"
        配置脚本.write_text("\r\n".join(win配置行(vcvars, 清建)) + "\r\n", encoding="ascii")
        步骤("配置", ["cmd", "/c", str(配置脚本)], 900)
    else:
        步骤们["配置"] = {"rc": 0, "说明": "build-ninja 已配置（增量）"}
    if 步骤们["配置"].get("rc") != 0:
        return
    构建脚本 = 日志目录 / "win_build.cmd"
    构建脚本.write_text("\r\n".join(win构建行(vcvars)) + "\r\n", encoding="ascii")
    # 1385 P1-3：构建前后 sccache 统计差 → 缓存命中档；构建起点宿主负载档。
    #   win 侧 sccache 由 wt.py 配置注入；探测不到=「缓存未知」（不臆断无缓存）。
    sccache = shutil.which("sccache")
    前统计 = 读缓存计数(sccache) if sccache else None
    负载 = ci_report.负载档()
    步骤("构建", ["cmd", "/c", str(构建脚本)], 单轮总超时秒)
    后统计 = 读缓存计数(sccache) if sccache else None
    标记构建维度(步骤们, 缓存档(前统计, 后统计) if sccache else "缓存未知", 负载)
    # 1385：指纹清建/首次配置=冷建——配置/构建耗时基线按冷暖分桶（温建 60s 对
    #   冷建 300s 不误报；linux 侧冷建由预验「清产物」步骤识别·本标记为 win 专属补充）。
    if 冷建本轮:
        步骤们.setdefault("配置", {})["冷建"] = True
        步骤们.setdefault("构建", {})["冷建"] = True
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
缓存非法字符 = frozenset((";", "\r", "\n", "\0"))


def 解析缓存配置(环境=None, 探测器=None) -> dict:
    """linux 构建编译器缓存配置解析（1299·纯函数可注入自测）。

    CN_BUILD_CACHE 语义（运维旋钮·缺省 auto）：
      · 未设 / auto           → 依次探测 ccache、sccache；都无=不启用（构建照旧）；
      · off/0/false/no/关闭/否 → 显式关闭（即使已安装也不用）；
      · 含路径分隔符的值       → 显式 launcher：必须存在且可执行；
      · 裸命令名               → 按 PATH 解析；解析不到=配置错误；
      · 引号/分号/换行等       → 配置错误（不猜、不静默降级——882/1239 口径）。
    返回 {"launcher": str|None, "说明": str, "错误": str|None}：错误非空时调用方
    必须把配置步记红并中止构建（不得 fallback 到「未启用」继续跑）。"""
    环境 = os.environ if 环境 is None else 环境
    原始 = 环境.get("CN_BUILD_CACHE")
    指定 = (原始 or "").strip()
    探测器 = 探测器 or shutil.which

    def 错误(说明: str, 错误: str) -> dict:
        return {"launcher": None, "说明": 说明, "错误": 错误}

    if not 指定 or 指定.lower() == "auto":
        for 名 in ("ccache", "sccache"):
            路径 = 探测器(名)
            if 路径:
                return {"launcher": 路径, "说明": "自动探测命中 %s" % 路径, "错误": None}
        return {"launcher": None,
                "说明": "自动探测：ccache/sccache 均未安装（构建走原始编译器）",
                "错误": None}
    if 指定.lower() in 缓存关闭词:
        return {"launcher": None, "说明": "显式关闭（CN_BUILD_CACHE=%s）" % 指定,
                "错误": None}
    坏字 = "".join(sorted(set(指定) & 缓存非法字符))
    if 坏字:
        return 错误("配置非法",
                    "CN_BUILD_CACHE 含非法字符 %r：%r（拒绝进入构建）" % (坏字, 指定))
    if '"' in 指定 or "'" in 指定:
        return 错误("配置非法",
                    "CN_BUILD_CACHE 不支持带引号的值 %r（systemd Environment= 的引号由 "
                    "systemd 剥离；值内引号=配置错误）" % 指定)
    if os.path.sep in 指定 or (os.path.altsep and os.path.altsep in 指定):
        候选 = Path(os.path.expanduser(指定))
        if not 候选.is_file() or not os.access(str(候选), os.X_OK):
            return 错误("配置非法",
                        "CN_BUILD_CACHE=%r 指向的 launcher 不存在或不可执行" % 指定)
        return {"launcher": str(候选), "说明": "显式 launcher %s" % 候选, "错误": None}
    命中 = 探测器(指定)
    if not 命中:
        return 错误("配置非法",
                    "CN_BUILD_CACHE=%r 不是受支持的值（可用 auto/off/关闭 或可执行 "
                    "launcher 路径；不支持 1/disable/none 等别名）" % 指定)
    return {"launcher": 命中, "说明": "显式 launcher %s" % 命中, "错误": None}


def 选编译器缓存(环境=None, 探测器=None) -> str | None:
    """兼容入口（1299）：返回 launcher 或 None；非法配置抛 ValueError（调用方显式红，
    不得静默按「未启用」继续）。详细原因/说明用 解析缓存配置。"""
    决策 = 解析缓存配置(环境, 探测器)
    if 决策["错误"]:
        raise ValueError(决策["错误"])
    return 决策["launcher"]


# ═══════════════ #1462 配置失败错误原文回传（门禁详情归因零猜测） ═══════════════
# 1212 集成实录：配置步骤假红只有 rc 与后续「未找到 cn 产物」类笼统摘要——CMake
# Error 原文（『未找到系统 Google Test』FATAL）只埋在轮日志文件里=归因困难。
# ci_round_guard.运行 以 stderr=subprocess.STDOUT 合流写轮日志——提取轮日志尾部
# 即得原文；步骤记录器 在「配置」步骤 rc!=0 时把本函数结果落 步骤["错误原文"]。
配置错误尾窗字节 = 128 * 1024   # 尾窗（配置失败轮总秒 2~5·CMake Error 段贴近日志尾）
配置错误段上限行 = 40           # 回传段上限（防多 Error 轮撑大 task_complete 载荷）


def 提取配置错误段(日志) -> str:
    """配置步骤失败时自轮日志提取 CMake Error 段原文——取尾窗内首个含「CMake Error」
    的行至窗尾、截上限行数（一段输出可含多个 Error at·首 Error 起全量最省归因）。
    无匹配=空串（否定回答·消费点非空才落键——vcvars 缺失等非 cmake 形态失败不硬凑）；
    日志=None（自测桩调用面）或无 name 属性=空串。"""
    if 日志 is None:
        return ""
    路径 = getattr(日志, "name", "")
    if not 路径:
        return ""
    try:
        日志.flush()     # 运行() 流式写文件对象——读盘前先落缓冲
        with open(路径, "r", encoding="utf-8", errors="replace") as f:
            f.seek(0, os.SEEK_END)
            尺寸 = f.tell()
            f.seek(max(0, 尺寸 - 配置错误尾窗字节))
            尾窗 = f.read()
    except OSError:
        return ""
    行们 = 尾窗.splitlines()
    起 = next((i for i, 行 in enumerate(行们) if "CMake Error" in 行), None)
    if 起 is None:
        return ""
    return "\n".join(行们[起:起 + 配置错误段上限行])


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
    ③单测（产物三 fallback=integrate 同款）。除 1299 缓存接线外与抽出前逐行等价。
    973 外迁：构建并行/单轮总超时秒 由调用方注入（daemon 常量单源·911）。
    1299：配置前解析编译器缓存（ccache/sccache）并显式传入 launcher；配置非法=配置步
    诚实红并中止（不静默降级）；日志参数可选（None=不输出·兼容自测桩调用面）。"""
    决策 = 解析缓存配置()
    if 日志 is not None:
        print("  [1299] 编译器缓存：%s" % 决策["说明"], file=日志, flush=True)
    if 决策["错误"]:
        if 日志 is not None:
            print("  [1299] %s" % 决策["错误"], file=日志, flush=True)
        步骤们["配置"] = {"rc": -1, "说明": 决策["错误"]}
        return  # 配置非法——不进入 cmake（防带病构建/静默降级）
    if not 步骤("配置", linux配置命令(决策["launcher"]), 600):
        return  # 配置红——步骤们 已记 rc，构建/单测不跑（原 pass 分支语义）
    # 1385 P1-3：构建前后缓存统计差 → 命中档；构建起点宿主负载档（同宿主并行可见）
    前统计 = 读缓存计数(决策["launcher"]) if 决策["launcher"] else None
    负载 = ci_report.负载档()
    if not 步骤("构建", ["cmake", "--build", "target/build",
                        "--parallel", str(构建并行)], 单轮总超时秒):
        return  # 构建红——单测不跑（原 elif 语义）
    后统计 = 读缓存计数(决策["launcher"]) if 决策["launcher"] else None
    标记构建维度(步骤们, 缓存档(前统计, 后统计) if 决策["launcher"] else "无缓存", 负载)
    单测 = next((p for p in [仓库根 / "target/build/tests/unit/cn_unit_tests",
                             仓库根 / "target/build/cn_unit_tests",
                             仓库根 / "target/cn_unit_tests"] if p.exists()), None)
    if 单测 is None:
        步骤们["单测"] = {"rc": -1, "说明": "未找到单测产物"}
    else:
        步骤("单测", [str(单测)], 1800)
