#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# CN语言自举性能基线测量脚本（plans/004 D-1 · 560-a v2 口径三平台改造）
# 目标：自举后跑 CN 版编译器对比 C++ 版，验证「性能达 C -O0」里程碑
#
# 【560-a 范围修正】原版测量对象 = v1 组件链（CN语言编译器/ 五组件）——v1 因
#   字符串行 IR 内存失控已推倒（79 已 v2 化），测量对象整体过时。现改为 v2 自举
#   编译器全树（CN语言编译器v2/，fix_p ≡ fix_s 固定点口径），编排对齐 78_v2/79_v2
#   锚定链（run_e2e.py 执行v2锚定链 = 蓝本）。
#
# 测量点（v2 口径）：
#   M1: C++版（宿主）build v2 全树时间（cn build CN语言编译器v2/主.cn -> v2p）
#   M2: CN版编译 v2 全树时间（v2p 编译自身 -> fix_p；3 次取中位数）——性能比 = M2/M1
#   M3: as 汇编 fix_p -> cn_self.obj（win=ml64）
#   M4: 链接 cn_self（cn_self.obj 在前 + v2p.obj 借链 + 运行时 objs；靠前定义胜出）
#   M5: cn_self 再编译 v2 全树 -> fix_s（3 次取中位数）
#   M6: 正确性验证：fix_p ≡ fix_s 逐字节一致（自举固定点·自洽性断言）
#
# 三平台：win-x64 走 ml64/link（原逻辑保留）；linux-arm64 / linux-x86_64 走
#   as/g++ -no-pie（CN_AS/CN_CXX 环境变量优先 -> PATH -> gcc7 兜底，对齐 run_e2e.py）。
#
# 【036 交叉模拟支持】x86_64 宿主上测 linux-arm64（无 arm64 真机时）：
#   宿主 cn 用 CN_AS=aarch64-linux-gnu-as / CN_CXX=aarch64-linux-gnu-g++ 交叉构建；
#   v2p/cn_self 等 arm64 ELF 由 运行命令 入口按 ELF 头（e_machine=0xB7）自动套
#   qemu 前缀（默认 qemu-aarch64 -L /usr/aarch64-linux-gnu，可用 CN_RUN_PREFIX 覆盖）。
#   qemu 下全树编译很慢：--超时秒 给单命令超时（默认 1800），--样本数 控制 M2/M5
#   采样数（默认 3·取中位）；超时/失败如实入报告（M2 失败=仅 M1 的部分基线）。
#   --跳过M5：M4 后停止（qemu 下 M5 常需数小时——只取 M1~M4 时用；报告标注未执行）。
#
# 用法: python scripts/bench_self_host.py [--platform linux-arm64] [--cn <编译器路径>]
#       [--超时秒 1800] [--样本数 3]
# 产物: target/bench/性能基线报告.md（报告）+ target/bench/selfwork/（中间产物，
#       独立子路径——不与 E2E 的 target/audit2/selfwork78|79 互踩）

import argparse
import os
import pathlib
import platform
import shutil
import subprocess
import sys
import time

# 强制 stdout/stderr 使用 UTF-8 输出（避免 Windows 管道/控制台代码页导致中文乱码）
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")

# ============ 常量 ============

# 项目根目录（本文件所在目录的上两级：scripts -> 项目根）
项目根目录 = pathlib.Path(__file__).resolve().parent.parent
# v2 自举编译器全树入口（560-a：测量对象自 v1 组件链改为 v2 全树）
源文件 = 项目根目录 / "CN语言编译器v2" / "主.cn"
# bench 独立工作目录（规则 19：产物统一放 target/ 下；不与 E2E 审计目录互踩）
基准目录 = 项目根目录 / "target" / "bench"
工作目录 = 基准目录 / "selfwork"

# 平台合法集（对齐 run_e2e.py）
平台们 = ("win-x64", "linux-arm64", "linux-x86_64")

# 运行时 .o 名（cn build 自动编译到 target/<目标平台桶>/；linux 侧现场 g++ -c 同名单，
#   对齐 run_e2e.py 确保v2p与运行时就绪 的运行时名们——io_api 与 time/system 等
#   560-a 实测全名单取 runner 同款，缺一链接期 undefined；349 重构E：io_api 拆出
#   mem_api/arena/crash_handler 三件同编同链；945：mem_api 再拆 mem_pool 同链；
#   522：win 侧消费 target/win-x64/*.obj）
运行时名们 = ["io_api", "mem_api", "mem_pool", "arena", "crash_handler",
            "intern_api", "runtime", "thread_api", "string_api", "i128_api",
            "math_api", "input_api", "file_api", "time_api", "system_api"]

# v2 产物行数下界（防「近乎空产物」假绿；对齐 run_e2e.py 锚定链下界口径）
行数下界 = 100000

# 交叉模拟运行前缀（main 一次性判定；空=原生执行）与命令超时/采样数（命令行可覆盖）
交叉前缀 = []
命令超时秒 = 1800
样本数 = 3

# ============ 工具函数 ============


def 是arm64可执行(路径: str) -> bool:
    """ELF 头探测：e_machine（offset 18·小端 2 字节）==183(0xB7)=AArch64。
    非 ELF/读失败一律 False——宿主工具链程序（x64 ELF）零误伤。
    与 tests/e2e/e2e交叉模拟.py 同判据（第二处·第三处出现前归并）。"""
    try:
        with open(路径, "rb") as f:
            头 = f.read(20)
    except OSError:
        return False
    return (len(头) >= 20 and 头[:4] == b"\x7fELF"
            and 头[18] == 0xB7 and 头[19] == 0)


def 交叉运行命令(命令列表: list) -> list:
    """arm64 ELF 产物自动套 qemu 前缀（x86_64 宿主跑 linux-arm64 目标时）。
    与 tests/e2e/e2e交叉模拟.py 同策略（第二处·第三处出现前归并）。"""
    if 交叉前缀 and 命令列表 and 是arm64可执行(str(命令列表[0])):
        return 交叉前缀 + list(命令列表)
    return 命令列表


def 运行命令(命令列表: list, 工作目录: pathlib.Path) -> subprocess.CompletedProcess:
    """执行命令并返回结果（捕获 stdout/stderr，UTF-8 解码容错）。
    交叉模拟激活时先套 qemu 前缀；命令超时 → 合成 returncode=-9 结果（不抛异常，
    由调用方按普通失败处理并如实入报告——基线任务禁因超时伪造数字）。"""
    命令列表 = 交叉运行命令(list(命令列表))
    try:
        return subprocess.run(
            命令列表, cwd=str(工作目录), capture_output=True,
            text=True, encoding="utf-8", errors="replace",
            timeout=命令超时秒 if 命令超时秒 > 0 else None)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(
            命令列表, -9, "",
            f"超时（>{命令超时秒} 秒·命令: {' '.join(str(c) for c in 命令列表[:3])}）")


def 计时(函数, *参数, **关键字参数) -> tuple:
    """执行函数并返回 (结果, 耗时毫秒)，用 time.perf_counter() 精确计时"""
    开始 = time.perf_counter()
    结果 = 函数(*参数, **关键字参数)
    结束 = time.perf_counter()
    return 结果, (结束 - 开始) * 1000.0


def 探测平台(显式平台: str) -> str:
    """确定目标平台：优先 --platform，否则按宿主机探测。
    显式 linux-arm64 且宿主为 x86_64 时由 判定交叉模拟 激活 qemu 前缀。"""
    if 显式平台:
        if 显式平台 not in 平台们:
            print(f"错误: --platform 取值非法: {显式平台}（须 {'/'.join(平台们)}）")
            sys.exit(2)
        return 显式平台
    机器 = platform.machine().lower()
    if machine_is_arm(机器):
        return "linux-arm64" if sys.platform.startswith("linux") else "win-arm64"
    if sys.platform.startswith("linux"):
        return "linux-x86_64"
    if sys.platform.startswith("win"):
        return "win-x64"
    print(f"错误: 无法识别宿主平台（{sys.platform}/{机器}），请用 --platform 指定")
    sys.exit(2)


def machine_is_arm(机器: str) -> bool:
    return 机器 in ("aarch64", "arm64", "armv8l", "armv7l")


def 判定交叉模拟(目标平台: str) -> bool:
    """x86_64 宿主 × linux-arm64 目标 → 激活 qemu 运行前缀（返回 True）。
    CN_RUN_PREFIX 语义对齐 run_e2e.py：非空=自定义前缀；空串=禁用；
    缺省=qemu-aarch64 -L /usr/aarch64-linux-gnu。前缀程序缺失=配置错误立即退出。"""
    global 交叉前缀
    交叉前缀 = []
    if 目标平台 != "linux-arm64" or machine_is_arm(platform.machine().lower()):
        return False
    覆盖 = os.environ.get("CN_RUN_PREFIX")
    if 覆盖 is not None:
        前缀 = 覆盖.split() if 覆盖.strip() else []
    else:
        前缀 = ["qemu-aarch64", "-L", "/usr/aarch64-linux-gnu"]
    if 前缀 and shutil.which(前缀[0]) is None:
        print(f"错误: 交叉模拟需 {前缀[0]} 未找到——安装 qemu-user 或设 CN_RUN_PREFIX 覆盖")
        sys.exit(2)
    交叉前缀 = 前缀
    return bool(前缀)


def 探测编译器(显式路径: str) -> pathlib.Path:
    """确定编译器路径：优先 --cn 显式参数，否则按候选路径自动探测"""
    if 显式路径:
        路径 = pathlib.Path(显式路径).resolve()
        if not 路径.exists():
            print(f"错误: 编译器不存在: {路径}")
            sys.exit(2)
        return 路径
    候选们 = [
        项目根目录 / "target" / "Release" / "cn.exe",
        项目根目录 / "target" / "Debug" / "cn.exe",
        项目根目录 / "target" / "cn",
        项目根目录 / "target" / "cn.exe",
        项目根目录 / "target" / "Debug" / "cn",
    ]
    for 候选 in 候选们:
        if 候选.exists():
            return 候选.resolve()
    print("错误: 未找到编译器，请使用 --cn 指定路径（如 target/cn）")
    sys.exit(2)


def 探测linux工具链() -> tuple:
    """linux 侧工具链：as 汇编器 + g++ 链接器（CN_AS/CN_CXX 环境变量优先，
    PATH，便携 gcc7 兜底——对齐 run_e2e.py 确保v2p与运行时就绪 的探测口径）"""
    as工具 = os.environ.get("CN_AS") or shutil.which("as") or "/home/user/gcc7/usr/bin/as"
    cxx工具 = os.environ.get("CN_CXX") or shutil.which("g++") or "/home/user/gcc7/usr/bin/g++"
    if shutil.which(as工具) is None and not pathlib.Path(as工具).exists():
        print(f"错误: 未找到 as 汇编器（{as工具}）")
        sys.exit(2)
    if shutil.which(cxx工具) is None and not pathlib.Path(cxx工具).exists():
        print(f"错误: 未找到 g++ 链接器（{cxx工具}）")
        sys.exit(2)
    return as工具, cxx工具


def 探测win工具链() -> tuple:
    """win 侧工具链：ml64/link（VS 2022 MSVC——原版逻辑保留，供家机跨机复验）"""
    msvc根 = pathlib.Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC")
    ML64 = LINK = None
    msvc版本目录 = None
    for 版本 in ("14.44.35207", "14.38.33130"):
        候选ml64 = msvc根 / 版本 / "bin" / "Hostx64" / "x64" / "ml64.exe"
        候选link = msvc根 / 版本 / "bin" / "Hostx64" / "x64" / "link.exe"
        if 候选ml64.exists() and ML64 is None:
            ML64 = 候选ml64
            msvc版本目录 = 候选ml64.parent.parent.parent.parent
        if 候选link.exists() and LINK is None:
            LINK = 候选link
    if ML64 is None or LINK is None:
        print("错误: 未找到 ml64/link（VS 2022 MSVC 工具链），请确认 Visual Studio 2022 已安装")
        sys.exit(2)
    kits根 = pathlib.Path(r"C:\Program Files (x86)\Windows Kits\10\lib")
    kits版 = sorted((p for p in kits根.glob("10.*") if p.is_dir()),
                    reverse=True) if kits根.exists() else []
    if not kits版:
        print(f"错误: 未找到 Windows Kits lib 目录: {kits根}\\10.*")
        sys.exit(2)
    LIB路径们 = [str(msvc版本目录 / "lib" / "x64"),
               str(kits版[0] / "ucrt" / "x64"),
               str(kits版[0] / "um" / "x64")]
    return ML64, LINK, LIB路径们


def 准备运行时objs(平台: str, cxx工具: str) -> list:
    """运行时 .o 现场编译（linux；不计时——M2/M5 只测编译器本体）。
    linux 编译命令对齐 run_e2e.py（g++ -c -std=c++17 -fno-exceptions
    -fno-rtti -DCNRT_LINUX_MAIN）；win 侧由 cn build 自动产出
    target/win-x64/*.obj（522：架构桶）。"""
    objs = []
    obj目录 = 基准目录 / "runtime_objs"
    obj目录.mkdir(parents=True, exist_ok=True)
    for 名 in 运行时名们:
        obj = obj目录 / f"{名}.o"
        src = 项目根目录 / "src" / "runtime" / f"{名}.cpp"
        if not src.exists():
            print(f"错误: 缺少运行时源文件: {src}")
            sys.exit(1)
        结果 = 运行命令([cxx工具, "-c", "-std=c++17", "-fno-exceptions", "-fno-rtti",
                      "-DCNRT_LINUX_MAIN", "-Isrc", "-O2", "-o", str(obj), str(src)],
                     项目根目录)
        if 结果.returncode != 0:
            print(f"错误: 运行时 {名}.o 编译失败: {(结果.stderr or 结果.stdout).strip()[:200]}")
            sys.exit(1)
        objs.append(obj)
    return objs


def 统计输入规模() -> dict:
    """统计 v2 自举树源码行数（输入规模）"""
    结果 = {}
    for 路径 in sorted((项目根目录 / "CN语言编译器v2").rglob("*.cn")):
        行数 = len(路径.read_text(encoding="utf-8", errors="replace").splitlines())
        结果[str(路径.relative_to(项目根目录 / "CN语言编译器v2"))] = 行数
    return 结果


def 编译v2全树(exe: pathlib.Path, 目标平台: str) -> tuple:
    """[M2/M5 共用]：exe 编译 v2 全树自身 -> 工作目录/target/v2asm.{asm|s}。
    断言：退出码 0 + 产物存在 + 含入口符号 cn_main（防假绿·对齐锚定链）。
    返回 (产物路径|None, 行数, 错误消息)。**不退出**——调用方定夺：M2 失败=
    核心指标缺失（硬退出）；M5 失败=缺陷域（cn_self 自身编译失败）→ 降级出
    报告、保留 M1~M4 成果（720-a linux 首跑实证：M4 链接修复后 cn_self 编译
    第二跳失败=家机 708-e 链在飞缺陷域，M1~M4 实测成果不该被一起吞掉）。"""
    if 目标平台 == "win-x64":
        asm路径 = 工作目录 / "target" / "v2asm.asm"
        入口标记 = "cn_main PROC"
        命令 = [str(exe), str(源文件)]
    else:
        asm路径 = 工作目录 / "target" / "v2asm.s"
        入口标记 = ".globl cn_main"
        命令 = [str(exe), str(源文件), 目标平台]  # GAS 后端分派（v2 驱动器第 2 参数）
    if asm路径.exists():
        asm路径.unlink()
    结果 = 运行命令(命令, 工作目录)
    if 结果.returncode != 0:
        # 720-a：落错取尾部——v2p/cn_self 的 [p6h]/[sem5] 进度流走 stderr，
        #   真错误在末尾（首跑降级报告实证「前 300 字符全是已解析进度行」
        #   无诊断价值）；stderr 空则退 stdout。两路统一取尾 300 字符。
        错输出 = (结果.stderr or "").strip() or (结果.stdout or "").strip()
        return None, 0, (f"编译 v2 全树失败(退出码{结果.returncode}): "
                         f"{错输出[-300:]}")
    if not asm路径.exists():
        return None, 0, "编译退出码 0 但未落盘 v2asm（防假绿）"
    内容 = asm路径.read_text(encoding="utf-8", errors="replace")
    if 入口标记 not in 内容:
        return None, 0, "产物缺少入口符号 cn_main（v2 代码生成入口未对齐宿主·防假绿）"
    行数 = 内容.count("\n") + 1
    if 行数 < 行数下界:
        return None, 0, f"产物 {行数} 行 < 下界 {行数下界}（近乎空产物·防假绿下界触发）"
    return asm路径, 行数, ""


def 编译v2全树计时(exe: pathlib.Path, 目标平台: str) -> tuple:
    """编译 v2 全树并计时，返回 (耗时ms, 行数, asm路径|None, 错误消息)"""
    开始 = time.perf_counter()
    asm路径, 行数, 错误 = 编译v2全树(exe, 目标平台)
    耗时 = (time.perf_counter() - 开始) * 1000.0
    return (耗时, 行数, asm路径, 错误)


# ============ 测量点实现 ============


def 测量M1(编译器路径: pathlib.Path, 目标平台: str, v2p: pathlib.Path) -> float:
    """M1: C++版（宿主）build v2 全树时间（cn build CN语言编译器v2/主.cn -> v2p）"""
    if v2p.exists():
        v2p.unlink()
    结果, 耗时 = 计时(运行命令, [str(编译器路径), "build", str(源文件),
                              "--target", 目标平台, "--output", str(v2p)],
                      项目根目录)
    if 结果.returncode != 0:
        print(f"错误: M1 编译失败(退出码{结果.returncode}): "
              f"{(结果.stderr or 结果.stdout).strip()[:300]}")
        sys.exit(1)
    if not v2p.exists():
        print("错误: M1 编译返回成功但未生成 v2p")
        sys.exit(1)
    return 耗时


def 测量M2(目标平台: str, v2p: pathlib.Path, fixp: pathlib.Path) -> tuple:
    """M2: CN版编译 v2 全树 -> fix_p（样本数 次取中位数）；返回 (中位数, 样本表,
    行数, 错误消息)。失败/超时=核心指标缺失，由主程序出「仅 M1」部分基线报告
    （036：qemu 模拟下 M2 可能远超单命令超时——如实记录，不硬吞）。"""
    样本 = []
    行数 = 0
    for i in range(样本数):
        耗时, 行数, asm, 错误 = 编译v2全树计时(v2p, 目标平台)
        if 错误:
            return 0.0, [round(v, 3) for v in 样本], 0, 错误
        样本.append(耗时)
        shutil.copy2(asm, fixp)
    样本.sort()
    return 样本[len(样本) // 2], [round(v, 3) for v in 样本], 行数, ""


def 测量M3(as工具, ML64, 目标平台: str, fixp: pathlib.Path,
           cn_self_obj: pathlib.Path) -> float:
    """M3: 汇编 fix_p -> cn_self.obj（linux=as / win=ml64）"""
    if cn_self_obj.exists():
        cn_self_obj.unlink()
    if 目标平台 == "win-x64":
        结果, 耗时 = 计时(运行命令, [str(ML64), "/nologo", "/c",
                              f"/Fo{cn_self_obj}", str(fixp)], 项目根目录)
    else:
        结果, 耗时 = 计时(运行命令, [as工具, "-o", str(cn_self_obj), str(fixp)],
                      项目根目录)
    if 结果.returncode != 0:
        print(f"错误: M3 汇编失败(退出码{结果.returncode}): "
              f"{(结果.stderr or 结果.stdout or '').strip()[:300]}")
        sys.exit(1)
    if not cn_self_obj.exists():
        print("错误: M3 汇编返回成功但未生成 cn_self.obj")
        sys.exit(1)
    return 耗时


def 测量M4(目标平台: str, LINK, LIB路径们, v2pobj, cn_self_obj: pathlib.Path,
           cn_self_exe: pathlib.Path) -> float:
    """M4: 链接 cn_self（cn_self.obj 在前 + v2p.obj 借链 + 运行时；靠前定义胜出——
    防 v1 79 虚假验收教训重演）。linux 加绑定自检所需 -Wl,-Map。"""
    if cn_self_exe.exists():
        cn_self_exe.unlink()
    map文件 = 工作目录 / "cn_self_link.map"
    if map文件.exists():
        map文件.unlink()
    if 目标平台 == "win-x64":
        响应文件 = 工作目录 / "cn_self_link.rsp"
        rsp_lines = ["/nologo", "/ENTRY:WinMainCRTStartup", "/SUBSYSTEM:CONSOLE",
                     "/STACK:8388608", "/FORCE:MULTIPLE"]
        for lib in LIB路径们:
            rsp_lines.append(f"/LIBPATH:{lib}")
        rsp_lines += ["/DEFAULTLIB:libcmt.lib", "/DEFAULTLIB:libucrt.lib",
                      "/DEFAULTLIB:kernel32.lib", "/DEFAULTLIB:shell32.lib",
                      f"/OUT:{cn_self_exe}", f"/MAP:{map文件}"]
        # cn_self.obj 必须在 v2p.obj 之前（防 v1 79 虚假验收教训：命令行靠前定义胜出）；
        #   637-a：win 面补运行时 objs（对齐 run_e2e 执行v2锚定链 [4] 链接清单——
        #   原只塞 cn_self.obj+v2pobj=None 缺十件运行时=LNK2019 面）
        #   522：运行时 obj 落架构桶 target/win-x64/（与 CMakeLists MSVC 段/cmd 侧
        #   run_e2e 消费点同规则·桶名=目标平台标识）
        rsp_lines += [str(cn_self_obj), str(v2pobj)]
        rsp_lines += [str(项目根目录 / "target" / "win-x64" / f"{名}.obj")
                      for 名 in 运行时名们]
        with open(响应文件, "w", encoding="utf-8") as f:
            for 行 in rsp_lines:
                f.write(f'"{行}"\n')
        结果, 耗时 = 计时(运行命令, [str(LINK), f"@{响应文件}"], 项目根目录)
    else:
        运行时objs = [obj目录 / f"{名}.o" for 名 in 运行时名们
                  for obj目录 in [基准目录 / "runtime_objs"]]
        命令 = [os.environ.get("CN_CXX") or shutil.which("g++") or "/home/user/gcc7/usr/bin/g++",
                "-no-pie", "-Wl,-z,muldefs", "-Wl,-Map," + str(map文件),
                "-o", str(cn_self_exe), str(cn_self_obj), str(v2pobj)]
        命令 += [str(o) for o in 运行时objs]
        # 996 线程波 1：thread_api.o 入链配套（glibc<2.34 缺 -lpthread 必炸·
        # >=2.34 平台合并 libpthread 掩盖——对齐 run_e2e.py 链接命令）
        命令 += ["-lpthread"]
        结果, 耗时 = 计时(运行命令, 命令, 项目根目录)
    if 结果.returncode != 0:
        print(f"错误: M4 链接失败(退出码{结果.returncode}): "
              f"{(结果.stderr or 结果.stdout or '').strip()[:300]}")
        sys.exit(1)
    if not cn_self_exe.exists():
        print("错误: M4 链接返回成功但未生成 cn_self")
        sys.exit(1)
    return 耗时


def 测量M5(目标平台: str, cn_self_exe: pathlib.Path, fixs: pathlib.Path) -> tuple:
    """M5: cn_self 再编译 v2 全树 -> fix_s（3 次取中位数）。
    返回 (中位数, 样本表, 行数, 错误消息)。720-a 降级：cn_self 编译失败
    （cn_self 缺陷域·非基线口径问题）→ 返回 (0.0, [], 0, 原因) 由主程序出
    降级报告（M1~M4 成果保留）。"""
    样本 = []
    行数 = 0
    for i in range(样本数):
        耗时, 行数, asm, 错误 = 编译v2全树计时(cn_self_exe, 目标平台)
        if 错误:
            return 0.0, [round(v, 3) for v in 样本], 0, 错误
        样本.append(耗时)
        shutil.copy2(asm, fixs)
    样本.sort()
    return 样本[len(样本) // 2], [round(v, 3) for v in 样本], 行数, ""


def 测量M6(fixp: pathlib.Path, fixs: pathlib.Path) -> bool:
    """M6: 正确性验证：fix_p ≡ fix_s 逐字节一致（自举固定点·自洽性断言）"""
    if not fixs.exists():
        print("错误: M6 fix_s 未生成")
        sys.exit(1)
    return fixp.read_bytes() == fixs.read_bytes()


def 绑定自检(目标平台: str, map文件: pathlib.Path, cn_self_obj: pathlib.Path) -> None:
    """绑定自检：cn_main 归属须为 cn_self.obj（防虚假验收——v1 79 教训；
    linux=ld -Map 段块归属解析；win=map 文本行尾归属·供家机跨机复验）"""
    if not map文件.exists():
        print("错误: 绑定自检缺少 map 文件")
        sys.exit(1)
    import re
    map内容 = map文件.read_text(encoding="utf-8", errors="replace")
    if 目标平台 == "win-x64":
        cn_main行 = [l.strip() for l in map内容.splitlines()
                    if re.search(r"\bcn_main\b", l) and "Rva" not in l]
        if not [l for l in cn_main行 if l.endswith("cn_self.obj")]:
            print(f"错误: 绑定自检失败：cn_main 未绑定 cn_self.obj\n    map 行: {cn_main行[:5]}")
            sys.exit(1)
        return
    当前obj = None
    归属 = None
    for l in map内容.splitlines():
        m = re.match(r"^\s+\.\S+\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+\s+(\S+)", l)
        if m:
            当前obj = m.group(1)
        if re.search(r"\bcn_main\b", l) and 当前obj:
            归属 = 当前obj
            break
    obj名 = pathlib.Path(cn_self_obj).name
    if not 归属 or obj名 not in 归属:
        print(f"错误: 绑定自检失败：cn_main 归属={归属}（期望含 {obj名}——虚假验收风险）")
        sys.exit(1)


# ============ 报告生成 ============


def 报告视图(测量结果: dict, M6一致: bool) -> dict:
    """计算报告展示值（失败/未执行分支在此单点收口），返回视图字典。"""
    m1 = 测量结果["M1"]
    m2 = 测量结果.get("M2", 0.0)
    m2错误 = 测量结果.get("M2错误", "")
    m3 = 测量结果.get("M3")
    m4 = 测量结果.get("M4")
    m5 = 测量结果.get("M5", 0.0)
    m5错误 = 测量结果.get("M5错误", "") if not m2错误 else ""
    m5跳过 = bool(测量结果.get("M5跳过", False)) and not m2错误
    比值vscpp = (m2 / m1) if (m1 > 0 and m2 > 0) else float("inf")
    比值自洽 = (m5 / m2) if (m2 > 0 and m5 > 0) else float("inf")
    if m2错误:
        结论 = ("⚠️ 部分基线：仅 M1 有效（M2 编译失败/超时——M3~M6 未执行；"
                "036 qemu 交叉模拟性能深坑或口径问题·详见末节）")
        m5显示 = "— 未执行（M2 失败）"
        m6显示 = "— 未执行（M2 失败）"
    elif m5错误:
        结论 = ("⚠️ 部分基线：M1~M4 有效（M5/M6 未达成——cn_self 自身编译失败·"
                "缺陷域非口径问题·详见末节）")
        m5显示 = "失败（降级·见末节）"
        m6显示 = "— 未执行（M5 失败）"
    elif m5跳过:
        结论 = ("⚠️ 部分基线：M1~M4 有效（M5/M6 按 --跳过M5 未执行——"
                "qemu 下 M5 预算不足的显式口径）")
        m5显示 = "— 跳过（--跳过M5）"
        m6显示 = "— 未执行（M5 跳过）"
    else:
        结论 = ("✅ 自举固定点成立（fix_p ≡ fix_s 逐字节）·基线入库"
                if M6一致 else "❌ 固定点不一致——性能对比无效（M6 失败）")
        m5显示 = f"{m5:.1f}"
        m6显示 = "✅ 一致" if M6一致 else "❌ 不一致"
    return {
        "m1": m1, "m2": m2, "m2错误": m2错误, "m5错误": m5错误,
        "m2显示": f"{m2:.1f}" if not m2错误 else "失败/超时（见末节）",
        "m3显示": f"{m3:.1f}" if m3 is not None else "— 未执行",
        "m4显示": f"{m4:.1f}" if m4 is not None else "— 未执行",
        "m5显示": m5显示, "m6显示": m6显示, "结论": 结论,
        "比值vscpp显示": f"{比值vscpp:.2f}x" if 比值vscpp != float("inf") else "—",
        "比值自洽显示": f"{比值自洽:.3f}x" if 比值自洽 != float("inf") else "—",
    }


def 报告正文行们(测量结果: dict, 视图: dict, 编译器路径: pathlib.Path,
                输入规模: dict, 目标平台: str) -> list:
    """报告头部与两张表（不含失败降级说明节）。"""
    cpu信息 = platform.processor() or platform.machine()
    平台说明 = 目标平台
    if 交叉前缀:
        平台说明 += (f"（qemu 交叉模拟：宿主 {platform.machine()} + "
                    f"{' '.join(交叉前缀)}）")
    return [
        "# CN 语言自举性能基线报告（plans/004 D-1 · 560-a v2 口径）",
        "",
        f"- **生成时间**: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        f"- **平台**: {平台说明}（本机自举基线·跨机比较无意义）",
        f"- **编译器（C++ 宿主）**: {编译器路径}",
        f"- **CPU**: {cpu信息}",
        f"- **测量对象**: CN语言编译器v2 全树（{sum(输入规模.values())} 行·"
        f"{len(输入规模)} 文件）",
        f"- **口径**: 单命令超时 {命令超时秒} 秒·M2/M5 样本数 {样本数}（取中位）",
        "",
        "## 一、各测量点耗时",
        "",
        "| 测量点 | 说明 | 耗时(ms) |",
        "|--------|------|---------:|",
        f"| M1 | C++ 宿主 build v2 全树（cn build -> v2p） | {视图['m1']:.1f} |",
        f"| M2 | CN 版编译 v2 全树 -> fix_p（{样本数} 次中位数·样本 "
        f"{测量结果.get('M2样本', [])}） | {视图['m2显示']} |",
        f"| M3 | as 汇编 fix_p -> cn_self.obj | {视图['m3显示']} |",
        f"| M4 | 链接 cn_self（cn_self.obj 在前 + v2p.obj 借链 + 运行时） | "
        f"{视图['m4显示']} |",
        f"| M5 | cn_self 再编译 v2 全树 -> fix_s（{样本数} 次中位数·样本 "
        f"{测量结果.get('M5样本', [])}） | {视图['m5显示']} |",
        f"| M6 | 固定点 fix_p ≡ fix_s 逐字节 | {视图['m6显示']} |",
        "",
        "## 二、性能比值",
        "",
        "| 对比项 | 基准 | CN 版 | 比值 |",
        "|--------|-----:|------:|-----:|",
        f"| 编译 v2 全树（M2/M1） | {视图['m1']:.1f} ms（C++ 宿主） | "
        f"{视图['m2显示']} | {视图['比值vscpp显示']} |",
        f"| 两代自洽（M5/M2·应≈1） | {视图['m2显示']}（v2p） | "
        f"{视图['m5显示']} | {视图['比值自洽显示']} |",
        "",
        f"- **v2p / cn_self 产物行数**: {测量结果.get('M2行数', 0)} / "
        f"{测量结果.get('M5行数', 0)}（下界 {行数下界}）",
        f"- **结论**: {视图['结论']}",
        "",
        "> **口径说明**：M2/M5 计时含编译器进程启动；比值 M2/M1 为「CN 版 vs C++ 版",
        "> 编译同一棵 v2 全树」的性能比（plans/004「性能达 C -O0」里程碑口径）；",
        "> M5/M2 为自举两代编译器自洽比（固定点成立时应 ≈1.00，偏差大=产物不自洽）。",
        "> 单机口径——跨机比较无意义（CPU/负载不同），仅作本平台纵向基线。",
        "> qemu 交叉模拟数值含指令转译开销，不等价于 arm64 真机性能。",
        "",
    ]


def 失败说明行们(测量结果: dict) -> list:
    """M2 失败/超时或 M5 失败时的第三节说明（互斥）。"""
    m2错误 = 测量结果.get("M2错误", "")
    m5错误 = 测量结果.get("M5错误", "")
    if m2错误:
        return [
            "## 三、M2 失败/超时说明（036 交叉模拟）",
            "",
            f"- **M2 错误**: {m2错误}",
            "- **性质**：M2 是基线核心指标；失败/超时=M3~M6 无输入，本报告仅 M1 有效。",
            "- **处置**：如实入册（禁伪造数字）；可放宽 `--超时秒` 或改真机复测。",
            "",
        ]
    if m5错误:
        return [
            "## 三、M5 失败降级说明（720-a）",
            "",
            f"- **M5 错误**: {m5错误}",
            "- **性质**：cn_self（v2 自举编译产物）自身编译 v2 全树失败——缺陷域",
            "  （非基线口径问题）。M1~M4 测量不受影响（v2p=宿主编译产物·有效）。",
            "- **证据链**：79_v2 自举闭环用例同点失败（run_e2e --filter 79_v2）——",
            "  同一 cn_self 缺陷；M5/M6 随该缺陷清偿后重跑本脚本回填。",
            "",
        ]
    if 测量结果.get("M5跳过"):
        return [
            "## 三、M5 跳过说明（--跳过M5）",
            "",
            "- **性质**：M1~M4 已完成且有效；M5/M6 按显式参数未执行（非失败）。",
            "- **用途**：qemu 交叉模拟下 M5 常需数小时——预算不足时先取 M1~M4。",
            "- **回填**：真机或放宽超时后重跑本脚本（不带 --跳过M5）即可补齐。",
            "",
        ]
    return []


def 生成报告(测量结果: dict, 编译器路径: pathlib.Path, 输入规模: dict,
             M6一致: bool, 目标平台: str) -> pathlib.Path:
    """生成性能基线报告 target/bench/性能基线报告.md（036：支持 M2 失败/超时的
    部分基线——M1 成果保留，未执行点如实标「—」而非伪造数字）"""
    报告路径 = 基准目录 / "性能基线报告.md"
    视图 = 报告视图(测量结果, M6一致)
    行们 = 报告正文行们(测量结果, 视图, 编译器路径, 输入规模, 目标平台)
    行们 += 失败说明行们(测量结果)
    报告路径.parent.mkdir(parents=True, exist_ok=True)
    with open(报告路径, "w", encoding="utf-8") as f:
        f.write("\n".join(行们) + "\n")
    return 报告路径


# ============ 主程序 ============


def 解析参数() -> tuple:
    """解析命令行并回写全局口径（超时/样本数），返回
    (目标平台, 编译器路径, 跳过M5)。"""
    解析器 = argparse.ArgumentParser(
        description="CN语言自举性能基线测量（plans/004 D-1 · 560-a v2 口径三平台）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n"
               "  python scripts/bench_self_host.py\n"
               "  python scripts/bench_self_host.py --platform linux-arm64 --cn target/cn\n"
               "  CN_AS=aarch64-linux-gnu-as CN_CXX=aarch64-linux-gnu-g++ \\\n"
               "    python scripts/bench_self_host.py --platform linux-arm64 --样本数 1")
    解析器.add_argument("--platform", help="目标平台（默认按宿主探测）")
    解析器.add_argument("--cn", help="cn编译器路径（默认自动探测 target/ 下）")
    解析器.add_argument("--超时秒", type=int, default=1800,
                        help="单条命令超时秒数（默认 1800·0=不限；超时如实入报告）")
    解析器.add_argument("--样本数", type=int, default=3,
                        help="M2/M5 采样次数（默认 3·取中位；qemu 模拟可设 1）")
    解析器.add_argument("--跳过M5", action="store_true",
                        help="M4 后停止（qemu 下 M5 常需数小时·只取 M1~M4 时用）")
    参数 = 解析器.parse_args()

    global 命令超时秒, 样本数
    命令超时秒 = 参数.超时秒
    样本数 = max(1, 参数.样本数)
    return 探测平台(参数.platform), 探测编译器(参数.cn), 参数.跳过M5


def 准备工具链(目标平台: str) -> tuple:
    """探测 as/cxx 与 win 工具链；交叉模拟激活时校验 aarch64 工具链配置。
    返回 (as工具, cxx工具, ML64, LINK, LIB路径们)。"""
    if 目标平台 == "win-x64":
        ML64, LINK, LIB路径们 = 探测win工具链()
        return None, None, ML64, LINK, LIB路径们
    as工具, cxx工具 = 探测linux工具链()
    if 判定交叉模拟(目标平台):
        print(f"  [交叉模拟] 宿主 {platform.machine()} × linux-arm64——"
              f"产物运行前缀: {' '.join(交叉前缀)}")
        if not ("aarch64" in pathlib.Path(as工具).name
                and "aarch64" in pathlib.Path(cxx工具).name):
            print(f"错误: 交叉模拟需 aarch64 工具链（当前 CN_AS={as工具}、"
                  f"CN_CXX={cxx工具}）——设 CN_AS=aarch64-linux-gnu-as "
                  f"CN_CXX=aarch64-linux-gnu-g++ 或 CN_RUN_PREFIX= 禁用")
            sys.exit(2)
    return as工具, cxx工具, None, None, None


def 准备工作目录() -> None:
    """重建 bench 工作目录并软链 stdlib（v2 驱动器按相对 cwd 读 stdlib/容器.cn）。"""
    if 工作目录.exists():
        shutil.rmtree(工作目录)
    (工作目录 / "target").mkdir(parents=True, exist_ok=True)
    # 637-a win 兼容：os.symlink 需 SeCreateSymbolicLinkPrivilege（开发者模式/
    #   管理员）——无特权时回退 mklink /J junction（同解析语义·无需特权）
    链接目标 = 工作目录 / "stdlib"
    try:
        os.symlink(项目根目录 / "stdlib", 链接目标)
    except OSError:
        subprocess.run(
            ["cmd", "/c", "mklink", "/J", str(链接目标), str(项目根目录 / "stdlib")],
            check=True, capture_output=True)


def 准备借链对象(目标平台: str, cxx工具: str):
    """M4 借链对象前置准备：win=audit2/v2p.obj（须先跑 E2E 预热）；linux=现场
    编译运行时 .o（不计时；720-a 恢复）并返回 None——linux 的 v2p.o 由 M1 产出，
    须待 M1 后由 执行测量 校验（时序：m1 → v2p.o 存在）。"""
    if 目标平台 == "win-x64":
        # 637-a 根治：win 面借链 obj=audit2/v2p.obj（cn_self.obj 靠前定义胜出·
        #   对齐 run_e2e 执行v2锚定链 [4]；原 win 分支 v2pobj=None 直塞 rsp
        #   → LNK1181「无法打开 None.obj」——bench win 面首次实跑暴露）
        v2pobj = 项目根目录 / "target" / "audit2" / "v2p.obj"
        if not v2pobj.exists():
            print(f"错误: M4 缺少借链 obj: {v2pobj}（先跑 E2E 预热或 78/79 单跑）")
            sys.exit(1)
        缺失运行时 = [项目根目录 / "target" / "win-x64" / f"{名}.obj"
                  for 名 in 运行时名们
                  if not (项目根目录 / "target" / "win-x64" / f"{名}.obj").exists()]
        if 缺失运行时:
            print(f"错误: M4 缺少运行时 obj（cn build 后即产出）: {缺失运行时[0].name}")
            sys.exit(1)
        return v2pobj
    # 720-a：linux 侧运行时 .o 现场编译——637-a win 面重构时误删本调用（win 需求
    #   补回、linux 面漏补·linux-x64 首跑 720 轮实测暴露 M4 缺 io_api.o 等 5 件）。
    准备运行时objs(目标平台, cxx工具)
    return None


def 执行测量(目标平台: str, 编译器路径: pathlib.Path, as工具, ML64, LINK,
             LIB路径们, v2pobj, 跳过M5: bool = False) -> tuple:
    """执行 M1~M6；返回 (测量结果, M6一致, 是否提前失败)。
    提前失败仅指 M2（核心指标缺失）——保留 M1 出部分报告。
    跳过M5=True：M4 绑定自检后停止（报告标注 M5/M6 未执行）。"""
    测量结果 = {}
    v2p = 工作目录 / ("v2p.exe" if 目标平台 == "win-x64" else "v2p")
    print("[M1] C++ 宿主 build v2 全树 ...")
    测量结果["M1"] = 测量M1(编译器路径, 目标平台, v2p)
    print(f"      {测量结果['M1']:.1f} ms")
    if v2pobj is None:  # linux：M1 的中间产物 v2p.o = M4 借链对象
        v2pobj = 工作目录 / "v2p.o"
        if not v2pobj.exists():
            print(f"错误: M4 缺少借链 obj: {v2pobj}（cn build 中间产物未留存）")
            sys.exit(1)

    fixp = 工作目录 / ("fix_p.asm" if 目标平台 == "win-x64" else "fix_p.s")
    print(f"[M2] CN 版编译 v2 全树 -> fix_p（{样本数} 次取中位数）...")
    (测量结果["M2"], 测量结果["M2样本"], 测量结果["M2行数"],
     测量结果["M2错误"]) = 测量M2(目标平台, v2p, fixp)
    if 测量结果["M2错误"]:
        print(f"      [M2 失败/超时·仅 M1 部分基线] {测量结果['M2错误']}")
        return 测量结果, False, True
    print(f"      样本: {测量结果['M2样本']} -> 中位数 {测量结果['M2']:.1f} ms"
          f"（{测量结果['M2行数']} 行）")

    cn_self_obj = 工作目录 / "cn_self.obj"
    print("[M3] 汇编 fix_p -> cn_self.obj ...")
    测量结果["M3"] = 测量M3(as工具, ML64, 目标平台, fixp, cn_self_obj)
    print(f"      {测量结果['M3']:.1f} ms")

    cn_self_exe = 工作目录 / ("cn_self.exe" if 目标平台 == "win-x64" else "cn_self")
    print("[M4] 链接 cn_self ...")
    测量结果["M4"] = 测量M4(目标平台, LINK, LIB路径们, v2pobj,
                        cn_self_obj, cn_self_exe)
    print(f"      {测量结果['M4']:.1f} ms")

    map文件 = 工作目录 / "cn_self_link.map"
    绑定自检(目标平台, map文件, cn_self_obj)
    print("      绑定自检: cn_main ∈ cn_self.obj ✓")
    if 跳过M5:
        测量结果["M5跳过"] = True
        print("[M5] 跳过（--跳过M5·M5/M6 未执行）")
        return 测量结果, False, False

    fixs = 工作目录 / ("fix_s.asm" if 目标平台 == "win-x64" else "fix_s.s")
    print(f"[M5] cn_self 再编译 v2 全树 -> fix_s（{样本数} 次取中位数）...")
    测量结果["M5"], 测量结果["M5样本"], 测量结果["M5行数"], 测量结果["M5错误"] = \
        测量M5(目标平台, cn_self_exe, fixs)
    if 测量结果["M5错误"]:
        print(f"      [M5 失败·降级出报告] {测量结果['M5错误']}")
        return 测量结果, False, False
    print(f"      样本: {测量结果['M5样本']} -> 中位数 {测量结果['M5']:.1f} ms"
          f"（{测量结果['M5行数']} 行）")

    print("[M6] 正确性验证（fix_p ≡ fix_s）...")
    M6一致 = 测量M6(fixp, fixs)
    print(f"      {'✅ 两次产物逐字节一致（自举固定点成立）' if M6一致 else '❌ 不一致'}")
    return 测量结果, M6一致, False


def 主程序() -> int:
    目标平台, 编译器路径, 跳过M5 = 解析参数()
    as工具, cxx工具, ML64, LINK, LIB路径们 = 准备工具链(目标平台)
    准备工作目录()

    输入规模 = 统计输入规模()
    print("CN语言 自举性能基线测量（v2 口径）")
    print(f"  平台: {目标平台}")
    print(f"  编译器（C++ 宿主）: {编译器路径}")
    print(f"  输入规模: {sum(输入规模.values())} 行 / {len(输入规模)} 文件")
    print(f"  单命令超时: {命令超时秒} 秒 | M2/M5 样本数: {样本数}"
          f"{' | 跳过M5' if 跳过M5 else ''}")
    print()

    v2pobj = 准备借链对象(目标平台, cxx工具)
    测量结果, M6一致, 提前失败 = 执行测量(
        目标平台, 编译器路径, as工具, ML64, LINK, LIB路径们, v2pobj, 跳过M5)
    报告路径 = 生成报告(测量结果, 编译器路径, 输入规模, M6一致, 目标平台)
    print()
    print(f"性能基线报告{'（部分）' if 提前失败 else ''}: {报告路径}")
    return 0 if (M6一致 and not 提前失败) else 1


if __name__ == "__main__":
    sys.exit(主程序())
