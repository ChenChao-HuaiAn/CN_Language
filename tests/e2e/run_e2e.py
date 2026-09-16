#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# CN语言E2E测试运行器：遍历用例目录，编译->运行->比对输出
# 用法: python3 run_e2e.py [--cn <编译器路径>] [--verbose] [--filter <模式>]
#
# 用例目录结构约定：
#   tests/e2e/<编号>_<名称>/<用例>.cn        # CN语言源文件
#   tests/e2e/<编号>_<名称>/<用例>.expected  # 期望输出（逐行比对）
#   tests/e2e/<编号>_<名称>/主.cn + 依赖.cn  # 多文件模块用例（入口=主.cn）

import argparse
import ctypes
import hashlib
import os
import pathlib
import re
import signal
import subprocess
import platform
import sys
import threading
import time

# 强制stdout/stderr使用UTF-8输出（避免Windows管道/控制台代码页导致中文乱码）
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")

# ============ 常量 ============

# E2E用例根目录（本文件所在目录）
用例根目录 = pathlib.Path(__file__).resolve().parent
# 项目根目录（用例根目录的上两级：tests/e2e -> tests -> 项目根）
项目根目录 = 用例根目录.parent.parent

# 默认编译器候选路径（按优先级自动探测；Windows 下带 .exe，Linux 下无后缀）
编译器候选 = [
    pathlib.Path("target") / "Debug" / "cn.exe",
    pathlib.Path("target") / "Debug" / "cn",
    pathlib.Path("target") / "Release" / "cn.exe",
    pathlib.Path("target") / "Release" / "cn",
    pathlib.Path("target") / "cn.exe",
    pathlib.Path("target") / "cn",
]

# 编译器"尚未实现"标记（阶段零预期输出，用于区分真实失败）
未实现标记 = "尚未实现"

# 运行子进程内存保护（2026-08-24 用户要求；78/79 迁移 v2 版后随迁，2026-09-14）：
# v2 锚定链用例（78_v2 自举链构建 / 79_v2 自举闭环）等大规模编译用例运行时，
# 子进程（v2p / cn_self 编译 v2 全树）内存可能失控（v1 时代旧组件链实测 26GB+ 卡死）。
# 超过 内存上限MB 的进程将被自动终止并判为失败。
# 单位：MB。0 = 不启用（默认仅对 v2 锚定链等重负载用例启用，避免小用例轮询开销）。
内存上限MB默认 = 4096
内存保护用例前缀 = ("78_v2_自举链构建", "79_v2_自举闭环")
内存轮询间隔秒 = 0.5

# 平台限制用例跳过列表：某些用例因平台特性差异（API/ABI/工具链）无法在特定平台运行
# 键 = 目标平台，值 = 用例目录名前缀列表（不含编号前缀的短名匹配）
# 注：65_string_index 的 ARM64 跳过已摘除（2026-09-03）——原「char 符号扩展差异」
#   在宿主 LoadPtr i8 双后端统一符号扩展（movsx/ldrsb）后不再成立，手动实测输出
#   与期望逐行一致（字节和 -376），随全量回归复验。
平台跳过 = {
    "linux-arm64": [
        "62_ffi",                # 依赖 Windows API GetTickCount64
        "69_memory_management",  # 运行时初始化计数在 Linux 上行为不同
    ],
    # plans/016（2026-09-05）：linux-x86_64 平台——宿主后端已支持（本机原生闭环）。
    # plans/017 T3（2026-09-06）：v2 自举编译器 X64L 后端（SysV GAS）落地 +
    #   run_e2e.py v2 闭环编排平台参数化——v2 闭环 23 例在本机解锁真实运行。
    # 78/79 迁移（2026-09-14）：v1 的 78_chain_build（v1 链仅 MASM 后端）与
    #   79_bootstrap_closed_loop（依赖 ml64/link）已迁移为 v2 版（78_v2/79_v2，
    #   三平台支持：win ml64/link；linux as/g++）——跳过项随 v1 用例一并摘除。
    "linux-x86_64": [
        "62_ffi",                # 依赖 Windows API GetTickCount64
        "69_memory_management",  # 运行时初始化计数在 Linux 上行为不同
    ],
    "win-x64": [
        # win-x64 暂无非平台限制用例
    ],
}

# ============ 终端颜色（Windows启用ANSI VT100） ============


def 启用终端颜色() -> bool:
    """启用Windows终端VT100颜色支持，返回是否支持颜色输出"""
    if os.environ.get("NO_COLOR"):  # 遵循 NO_COLOR 约定
        return False
    if not sys.stdout.isatty():     # 非终端（如重定向）不输出颜色
        return False
    if sys.platform == "win32":
        try:
            # 启用标准输出的虚拟终端处理 (ENABLE_VIRTUAL_TERMINAL_PROCESSING)
            kernel32 = ctypes.windll.kernel32
            句柄 = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
            模式 = ctypes.c_uint32()
            kernel32.GetConsoleMode(句柄, ctypes.byref(模式))
            kernel32.SetConsoleMode(句柄, 模式.value | 0x0004)
            return True
        except Exception:
            return False
    return True


颜色支持 = 启用终端颜色()


def 着色(文本: str, 颜色码: str) -> str:
    """为文本添加ANSI颜色（终端不支持时原样返回）"""
    if not 颜色支持:
        return 文本
    return f"\033[{颜色码}m{文本}\033[0m"


# 颜色便捷函数：PASS绿 / FAIL红 / 未实现黄 / 信息青 / 标题粗体
绿色 = lambda 文本: 着色(文本, "32")
红色 = lambda 文本: 着色(文本, "31")
黄色 = lambda 文本: 着色(文本, "33")
青色 = lambda 文本: 着色(文本, "36")
粗体 = lambda 文本: 着色(文本, "1")

# ============ 工具函数 ============


def 查询进程内存MB(进程ID: int) -> int:
    """查询进程当前工作集内存（MB）；查询失败返回 0（不触发保护误杀）

    说明：Windows 用 GetProcessMemoryInfo(WorkingSetSize)；
          Linux 用 /proc/<pid>/status 的 VmRSS（kB）。工作集含共享页，
          对"防失控"足够（失控时工作集必然随分配暴涨）。
    """
    try:
        if sys.platform == "win32":
            PROCESS_QUERY_INFORMATION = 0x0400
            PROCESS_VM_READ = 0x0010
            句柄 = ctypes.windll.kernel32.OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, 进程ID)
            if not 句柄:
                return 0
            try:
                class 进程内存计数(ctypes.Structure):
                    _fields_ = [
                        ("cb", ctypes.c_ulong),
                        ("页错误数", ctypes.c_ulong),
                        ("峰值工作集", ctypes.c_size_t),
                        ("工作集", ctypes.c_size_t),
                        ("峰值分页池", ctypes.c_size_t),
                        ("分页池", ctypes.c_size_t),
                        ("峰值非分页池", ctypes.c_size_t),
                        ("非分页池", ctypes.c_size_t),
                        ("峰值页文件", ctypes.c_size_t),
                        ("页文件", ctypes.c_size_t),
                        ("私有使用", ctypes.c_size_t),
                    ]
                计数 = 进程内存计数()
                计数.cb = ctypes.sizeof(进程内存计数)
                if ctypes.windll.psapi.GetProcessMemoryInfo(
                        句柄, ctypes.byref(计数), 计数.cb):
                    return int(计数.工作集 // (1024 * 1024))
                return 0
            finally:
                ctypes.windll.kernel32.CloseHandle(句柄)
        else:
            # Linux：/proc/<pid>/status 的 VmRSS（kB）
            with open(f"/proc/{进程ID}/status", encoding="utf-8") as f:
                for 行 in f:
                    if 行.startswith("VmRSS:"):
                        return int(行.split()[1]) // 1024  # kB -> MB
            return 0
    except Exception:
        return 0


def 终止进程树(进程: subprocess.Popen) -> None:
    """强制终止进程及其子进程树（Windows taskkill /T；Linux 进程组 SIGKILL）"""
    try:
        if sys.platform == "win32":
            subprocess.run(
                ["taskkill", "/PID", str(进程.pid), "/T", "/F"],
                capture_output=True, timeout=30)
        else:
            os.killpg(进程.pid, signal.SIGKILL)
    except Exception:
        pass


def 运行命令(命令列表: list, 工作目录: pathlib.Path,
             标准输入: str = "", 内存上限MB: int = 0,
             超时秒数: int = 0) -> subprocess.CompletedProcess:
    """执行命令并返回结果（捕获stdout/stderr，UTF-8解码容错）

    标准输入: 可选 stdin 注入字符串（Task 6.2 IO 输入用例用，默认空）
    内存上限MB: >0 时启用内存保护——每 内存轮询间隔秒 轮询子进程工作集，
      超过上限立即 终止进程树 并以退出码 -9（returncode）标记失败，
      stderr 给出"内存超限"原因。防 v2 锚定链等大规模编译用例内存失控卡死机器
      （2026-08-24 实测：v1 时代旧组件链（79_bootstrap_closed_loop）工作集涨到 26GB+）。
    """
    # Linux 下增大栈大小限制（CN自举编译器函数栈帧较大，默认8MB可能不足）
    preexec_fn = None
    if sys.platform != "win32":
        def _set_stack_limit():
            import resource
            try:
                resource.setrlimit(resource.RLIMIT_STACK,
                                  (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
            except (ValueError, OSError):
                pass
        preexec_fn = _set_stack_limit

    # ---- 内存保护路径：Popen + 轮询工作集，超限立即终止 ----
    # 2026-08-24 防死锁修复：轮询期间必须持续排空 stdout/stderr 管道——
    #   78/79 组件链编译器输出量大，管道缓冲（约 64KB）写满后子进程阻塞在
    #   write 上挂死（不计算/不退出/内存不涨，轮询永不触发，实测表现为
    #   "卡死不动"）。用后台读取线程持续排空，管道永不阻塞。
    if 内存上限MB > 0:
        启动参数 = dict(
            cwd=str(工作目录), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, encoding="utf-8", errors="replace",
            preexec_fn=preexec_fn)  # Linux 栈上限放大（Windows 忽略）
        if sys.platform != "win32":
            # Linux 独立进程组，便于 killpg 终止整棵子进程树
            启动参数["start_new_session"] = True
        进程 = subprocess.Popen(命令列表, **启动参数)
        if 标准输入:
            try:
                进程.stdin.write(标准输入)
                进程.stdin.flush()
            except Exception:
                pass
            finally:
                进程.stdin.close()

        # 后台排空线程：持续读取 stdout/stderr，防管道缓冲写满导致子进程死锁
        import threading
        收集输出 = {"stdout": "", "stderr": ""}

        def 排空(流, 键):
            try:
                for 行 in iter(流.readline, ""):
                    收集输出[键] += 行
            except Exception:
                pass

        排空线程们 = [
            threading.Thread(target=排空, args=(进程.stdout, "stdout")),
            threading.Thread(target=排空, args=(进程.stderr, "stderr")),
        ]
        for 线程 in 排空线程们:
            线程.daemon = True
            线程.start()

        while 进程.poll() is None:
            内存MB = 查询进程内存MB(进程.pid)
            if 内存MB > 内存上限MB:
                终止进程树(进程)
                try:
                    进程.wait(timeout=30)
                except Exception:
                    pass
                # 回收线程残留输出
                for 线程 in 排空线程们:
                    线程.join(timeout=2)
                return subprocess.CompletedProcess(
                    命令列表, -9, 收集输出["stdout"],
                    f"运行内存超限：工作集 {内存MB}MB > 上限 {内存上限MB}MB，"
                    "已自动终止（防 OOM 卡死）\n--- 终止前 stdout 尾部 ---\n"
                    + 收集输出["stdout"][-2500:]
                    + "\n--- 终止前 stderr 尾部 ---\n"
                    + 收集输出["stderr"][-2500:])
            time.sleep(内存轮询间隔秒)
        # 进程已退出：排空线程收尾后合并输出
        for 线程 in 排空线程们:
            线程.join(timeout=3)
        return subprocess.CompletedProcess(
            命令列表, 进程.returncode, 收集输出["stdout"], 收集输出["stderr"])

    # ---- 普通路径（无内存保护）：与历史行为完全一致 ----
    #    缺陷④防护（2026-09-04 用户裁决 C4）：编译步骤偶发零 CPU 挂起（终门禁
    #    cn build 66_switch_all 实测一次，taskkill 后单跑正常——疑 AV/文件锁/
    #    句柄竞争）——超时秒数 >0 时启用超时保护+重试一次，防偶发挂起拖死门禁
    if 超时秒数 and 超时秒数 > 0:
        for 尝试轮 in range(2):
            try:
                return subprocess.run(
                    命令列表, cwd=str(工作目录), capture_output=True,
                    text=True, encoding="utf-8", errors="replace",
                    input=标准输入, preexec_fn=preexec_fn, timeout=超时秒数)
            except subprocess.TimeoutExpired as 超时异常:
                标记 = f"[runner] 命令超时({超时秒数}s)第{尝试轮 + 1}次: {' '.join(str(c) for c in 命令列表[:3])}"
                print(标记, file=sys.stderr)
                if 尝试轮 == 1:
                    return subprocess.CompletedProcess(
                        命令列表, -9, (超时异常.stdout or b"").decode("utf-8", errors="replace") if isinstance(超时异常.stdout, bytes) else (超时异常.stdout or ""),
                        标记 + "\n")
        # 不可达（上方 return 覆盖两轮）
    return subprocess.run(
        命令列表, cwd=str(工作目录), capture_output=True,
        text=True, encoding="utf-8", errors="replace",
        input=标准输入, preexec_fn=preexec_fn)


# ============ asm size 说明符静态门禁（plans/021 §3-C14，111-a 落地）============
# 背景：MASM 的 movsd/movss 内存操作数须显式 qword/dword ptr（缺 → ml64 报 A2070）；
#   linux GAS 无此要求，缺陷只在 win 侧暴露（109-a 补 10 处 + 110-a 补 4 处两轮漏网）。
# 挂点（win 平台编译产物、ml64 之前静态扫描——把汇编器错误提前为门禁失败并给出定位）：
#   ① v2 闭环产物 target/v2asm.asm（v2 win 后端 = 两轮漏网的主缺陷面）
#   ② 双编译对照宿主侧产物 hostout.asm（宿主 win 后端同族面）
# 规则：去注释（; 起）后，movsd/movss 任一操作数含 '[' 但不含 'ptr' 即违规。
# 自检：正/负样本内联于 asm说明符扫描器自检（探测器本身也是被测对象——110-a 教训：
#   首版计数 bug 经注入反证才暴露）；E2E 每次启动即自检，扫描器失效先于被测物报错。
asm说明符_违规样本 = (
    "    movsd xmm0, [rbp-152]\n"
    "    movss [rax], xmm1\n"
    "    movsd xmm0, xmm1\n")
asm说明符_合规样本 = (
    "    movsd xmm0, qword ptr [rbp-152]\n"
    "    movss dword ptr [rax], xmm1\n"
    "    movsd qword ptr [rbp-232], xmm0\n"
    "    movsd xmm0, xmm1\n"
    "    movsd xmm0, qword ptr [rbp-8] ; movsd [rbp-16], xmm0\n")


def 扫描浮点访存说明符(asm文本: str) -> list:
    """静态扫描 movsd/movss 裸内存操作数（缺 size 说明符）；返回 [(行号, 行原文)]"""
    违规 = []
    for 序号, 原行 in enumerate(asm文本.split("\n"), 1):
        行 = 原行.split(";", 1)[0].strip()
        匹配 = re.match(r"^(movs[sd])\s+(.+)$", 行)
        if not 匹配:
            continue
        for 操作数 in 匹配.group(2).split(","):
            if "[" in 操作数 and "ptr" not in 操作数:
                违规.append((序号, 原行.strip()))
    return 违规


def asm说明符扫描器自检() -> str:
    """扫描器正/负样本自检（空串=通过；非空=扫描器自身失效描述）"""
    合规违规 = 扫描浮点访存说明符(asm说明符_合规样本)
    if 合规违规:
        return f"合规样本误报: {合规违规[:2]}"
    违规检出 = 扫描浮点访存说明符(asm说明符_违规样本)
    if len(违规检出) != 2:
        return f"违规样本漏报（检出 {len(违规检出)}/2）"
    return ""


def 扫描asm产物说明符(asm路径: pathlib.Path, 标签: str) -> str:
    """扫描 win 编译产物 asm 的 size 说明符；返回 ""（合规）或含定位的失败描述"""
    if not asm路径.exists():
        return ""
    违规 = 扫描浮点访存说明符(asm路径.read_text(encoding="utf-8", errors="replace"))
    if not 违规:
        return ""
    例 = "\n".join(f"      L{行号}: {原文}" for 行号, 原文 in 违规[:5])
    return (f"{标签} 产物 asm 有 {len(违规)} 处 movsd/movss 裸内存操作数"
            f"（缺 qword/dword ptr → ml64 A2070；{asm路径.name}）:\n{例}")


def 探测编译器(显式路径: str) -> pathlib.Path:
    """确定编译器路径：优先使用 --cn 显式参数，否则按候选路径自动探测"""
    if 显式路径:
        路径 = pathlib.Path(显式路径).resolve()
        if not 路径.exists():
            print(红色(f"错误: 编译器不存在: {路径}"))
            sys.exit(2)
        return 路径
    for 候选 in 编译器候选:
        路径 = (项目根目录 / 候选).resolve()
        if 路径.exists():
            return 路径
    print(红色("错误: 未找到编译器，请使用 --cn 指定路径（如 target/Debug/cn.exe）"))
    sys.exit(2)


def 收集用例(过滤模式: str) -> list:
    """收集所有E2E用例目录（形如 NN_名称），支持 --filter 按名称过滤"""
    全部 = sorted(
        [d for d in 用例根目录.iterdir() if d.is_dir() and d.name[:2].isdigit()],
        key=lambda d: d.name)
    if 过滤模式:
        return [d for d in 全部 if 过滤模式 in d.name]
    return 全部


def 查找源文件(用例目录: pathlib.Path) -> pathlib.Path:
    """查找用例目录下的 .cn 源文件（取第一个，抛出异常说明缺失）"""
    cn文件们 = sorted(用例目录.glob("*.cn"))
    if not cn文件们:
        raise FileNotFoundError(f"用例目录缺少 .cn 源文件: {用例目录.name}")
    # 多文件模块用例（Task 3.6，规格书08-四）：目录含多个 .cn 时，
    # 入口文件约定为 主.cn（模块名 == 主）。单文件用例保持取第一个行为不变。
    if len(cn文件们) > 1:
        for 候选 in cn文件们:
            if 候选.name == "主.cn":
                return 候选
        # 无 主.cn 的多文件目录：按约定报错提示（避免误取依赖模块当入口）
        raise FileNotFoundError(
            f"多文件用例目录需含入口 主.cn: {用例目录.name}")
    return cn文件们[0]


def 查找期望文件(源文件: pathlib.Path) -> pathlib.Path:
    """查找与源文件同名的 .expected 期望输出文件"""
    期望文件 = 源文件.with_suffix(".expected")
    if not 期望文件.exists():
        raise FileNotFoundError(f"缺少期望输出文件: {期望文件}")
    return 期望文件


def 查找输入文件(源文件: pathlib.Path) -> pathlib.Path:
    """查找与源文件同名的 .input 标准输入文件（不存在返回 None，Task 6.2 IO 输入用）"""
    输入文件 = 源文件.with_suffix(".input")
    if 输入文件.exists():
        return 输入文件
    return None


def 查找参数文件(源文件: pathlib.Path) -> list:
    """查找与源文件同名的 .args 命令行参数文件（不存在返回空列表，Task 6.5 系统库用）

    文件格式：每行一个参数（首行即 argv[1]；argv[0] 恒为可执行文件名本身）。
    空白行与 # 注释行跳过（与 .input 注入同模式，方便维护中文参数）。
    """
    参数文件 = 源文件.with_suffix(".args")
    if not 参数文件.exists():
        return []
    参数列表 = []
    for 行 in 参数文件.read_text(encoding="utf-8").splitlines():
        行 = 行.strip()
        if 行 and not 行.startswith("#"):
            参数列表.append(行)
    return 参数列表

# ============ 核心逻辑 ============


def 查找编译选项文件(用例目录: pathlib.Path) -> list:
    """用例级编译旗标通道（256-a 立）：用例目录含 `编译选项.txt` 时，其每行
    （去空行/# 注释）作为一个旗标追加到 cn build 命令行（如 -O3）——
    第 2 层工具链契约面扩展（三机共享契约：§8.4 广播）。"""
    标记 = 用例目录 / "编译选项.txt"
    if not 标记.exists():
        return []
    flags = []
    for line in 标记.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            flags.append(line)
    return flags


def 解码诊断(原始字节) -> str:
    """解码编译器诊断：cn.exe 的 stderr 为 UTF-16LE（FF FE BOM），stdout 为 UTF-8"""
    if not 原始字节:
        return ""
    if isinstance(原始字节, str):
        # C4 连带：运行命令 走 text=True 路径时已是解码后字符串（旧裸
        # subprocess.run 无 text 传 bytes——两种形态兼容）
        return 原始字节
    if 原始字节.startswith(b"\xff\xfe"):
        return 原始字节.decode("utf-16-le", errors="replace")
    if 原始字节.startswith(b"\xfe\xff"):
        return 原始字节.decode("utf-16-be", errors="replace")
    return 原始字节.decode("utf-8", errors="replace")


def 执行负用例(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
             输出目录: pathlib.Path, 详细: bool, 目标平台: str) -> tuple:
    """
    执行「负 e2e」用例：预期编译失败（P0-3 负向规范规则承载通道）。
    目录含 期望编译失败.txt（内容可选 = 期望诊断子串）：
      - 编译成功   -> 失败（规范负向规则被误实现/断言失效）
      - 编译失败 且（无子串 或 stderr/stdout 含子串） -> 通过
    返回 (状态, 说明)
    """
    名称 = 用例目录.name
    try:
        源文件 = 查找源文件(用例目录)
    except FileNotFoundError as 异常:
        return "失败", str(异常)
    负标记 = 用例目录 / "期望编译失败.txt"
    期望子串 = 负标记.read_text(encoding="utf-8").strip() if 负标记.exists() else ""
    可执行后缀 = ".exe" if 目标平台 == "win-x64" else ""
    输出可执行 = 输出目录 / f"{名称}{可执行后缀}"
    if 输出可执行.exists():
        输出可执行.unlink()
    编译命令 = [str(编译器路径), "build", str(源文件),
              "--target", 目标平台, "--output", str(输出可执行),
              *查找编译选项文件(用例目录)]
    if 详细:
        print(f"    [负编译] {' '.join(编译命令)}")
    # 字节模式采集：cn.exe stderr 为 UTF-16LE，须按字节解码诊断
    编译结果 = 运行命令([str(c) for c in 编译命令], 项目根目录, 超时秒数=180)
    if 编译结果.returncode == 0:
        return "失败", f"预期编译失败但编译成功: {源文件.name}"
    输出 = 解码诊断(编译结果.stderr) + 解码诊断(编译结果.stdout)
    if 期望子串 and 期望子串 not in 输出:
        return "失败", f"编译如预期失败但缺期望诊断[{期望子串}]，实际输出: {输出.strip()[:160]}"
    return "通过", "负测试通过（预期编译失败）"


def 执行check负用例(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
                   详细: bool, 目标平台: str) -> tuple:
    """
    执行「check 负用例」：预期 cn check 检查失败（簇⑥ 假绿根治验证通道，2026-09-05）。
    目录含 期望check失败.txt（逐行：首行 = 被检文件名，相对用例目录，可空=目录唯一 .cn；
    次行起 = 期望诊断子串，可空）：
      - check 通过   -> 失败（假绿：私有声明被模块可见性过滤=检查没发生）
      - check 失败 且（无子串 或 stderr/stdout 含子串） -> 通过
    与 期望编译失败.txt 通道同构（那是 build 须失败；本通道是 check 须失败）。
    """
    名称 = 用例目录.name
    标记 = 用例目录 / "期望check失败.txt"
    有效行 = [行.strip() for 行 in 标记.read_text(encoding="utf-8").splitlines()
              if 行.strip() and not 行.strip().startswith("#")]
    被检名 = 有效行[0] if 有效行 else ""
    期望子串 = 有效行[1] if len(有效行) > 1 else ""
    cn文件们 = sorted(用例目录.glob("*.cn"))
    if 被检名:
        源文件 = 用例目录 / 被检名
        if not 源文件.exists():
            return "失败", f"期望check失败.txt 指定的被检文件不存在: {被检名}"
    elif len(cn文件们) == 1:
        源文件 = cn文件们[0]
    else:
        return "失败", f"check负用例需在 期望check失败.txt 首行指定被检文件: {名称}"
    编译命令 = [str(编译器路径), "check", str(源文件), "--target", 目标平台]
    if 详细:
        print(f"    [check负] {' '.join(编译命令)}")
    # 字节模式采集：cn.exe stderr 为 UTF-16LE，须按字节解码诊断
    编译结果 = 运行命令([str(c) for c in 编译命令], 项目根目录, 超时秒数=180)
    if 编译结果.returncode == 0:
        return "失败", f"预期check失败但检查通过（假绿）: {源文件.name}"
    输出 = 解码诊断(编译结果.stderr) + 解码诊断(编译结果.stdout)
    if 期望子串 and 期望子串 not in 输出:
        return "失败", f"check如预期失败但缺期望诊断[{期望子串}]，实际输出: {输出.strip()[:160]}"
    return "通过", "check负测试通过（预期检查失败，私有声明未被静默过滤）"


def 执行单个用例(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
                 输出目录: pathlib.Path, 详细: bool, 目标平台: str) -> tuple:
    """
    执行单个E2E用例：编译->运行->比对输出
    返回 (状态, 说明)；状态取值: "通过" / "失败" / "未实现" / "跳过"
    """
    名称 = 用例目录.name
    # ============ 平台限制跳过检查 ============
    # 检查当前平台是否在跳过列表中（用例名匹配前缀）
    if 目标平台 in 平台跳过:
        for 跳过前缀 in 平台跳过[目标平台]:
            if 名称.endswith(跳过前缀) or 名称 == 跳过前缀:
                return "跳过", f"平台限制（{目标平台}）：{跳过前缀}"

    # ============ 负 e2e：预期编译失败用例（P0-3） ============
    # 目录含 期望编译失败.txt 时按负用例执行（负向规范规则承载通道）
    if (用例目录 / "期望编译失败.txt").exists():
        return 执行负用例(编译器路径, 用例目录, 输出目录, 详细, 目标平台)

    # ============ check 负用例：预期 cn check 失败（簇⑥ 假绿根治通道） ============
    # 目录含 期望check失败.txt 时按 check 负用例执行（check 命令行为承载通道）
    if (用例目录 / "期望check失败.txt").exists():
        return 执行check负用例(编译器路径, 用例目录, 详细, 目标平台)

    # ============ v2 锚定链用例（78_v2/79_v2，2026-09-14 自 v1 78/79 迁移）：特殊编排 ============
    # 对象=v2 自举编译器全树**自身**（用例目录无 主.cn——检测须先于 查找源文件）：
    #   阶段=链构建：宿主编译 v2 全树建 v2p（复用指纹缓存）-> v2p 编译 v2 全树
    #     -> fix_p.asm（v1 78_chain_build 的 v2 对应物：CN 版编译器编译自身全部源码）
    #   阶段=闭环：链构建 + 汇编 fix_p -> 链接 cn_self（绑定自检 cn_main 归属
    #     cn_self.obj，防虚假验收——v1 79 教训）-> cn_self 再编译 v2 全树 -> fix_s.asm
    #     -> fix_p ≡ fix_s 逐字节（自举固定点；断言=自洽性，对 v2 树演进稳定）
    # 三平台支持：win-x64 走 ml64/link；linux-arm64 / linux-x86_64 走 as/g++
    #   （对齐既有 v2 闭环编排；linux 分支动态验证随跨机轮）
    if (用例目录 / "v2锚定链.txt").exists():
        return 执行v2锚定链(编译器路径, 用例目录, 输出目录, 详细, 目标平台)

    try:
        源文件 = 查找源文件(用例目录)
        期望文件 = 查找期望文件(源文件)
    except FileNotFoundError as 异常:
        return "失败", str(异常)

    # ============ v2 自举链接闭环用例（119/120…）：特殊编排 ============
    # v1 79 闭环是 C++ 版产物 + CN 组件链；v2 系列闭环是 v2 重建产物 + 宿主运行时：
    #   宿主编译 v2 组件（入口 主.cn）-> v2p.exe -> 编译多文件程序
    #   （入口 主.cn + 导入模块，P6h 多源文件支持）-> v2asm.asm
    #   -> ml64/link（对齐宿主链接命令：/ENTRY:WinMainCRTStartup + 运行时 .obj）
    #   -> 运行 v2 产物 exe，退出码须等于用例预期值（v2 代码真实执行验证）
    # 阶段A（2026-09-02）双平台：win-x64 走 ml64/link；linux-arm64 走 as/g++
    #   （v2 新增 GAS 后端，v2p 第 2 参数 目标平台 分派；链接对齐宿主 linux 命令）
    # plans/017 T3（2026-09-06）：linux-x86_64 并入 as/g++ 编排——执行v2闭环Linux
    #   平台参数化（宿主编译 --target 与 v2p 第 2 参数随平台；v2 X64L 后端）
    # 元组第三元素（可缺省）= 是否链接 v2p.obj（P7b 容器用例：v2 生成代码调用
    #   宿主编译的容器类方法符号，实现在 v2p.obj——stdlib 源码级并入编译产物）
    # 元组第四元素（可缺省）= 供给源列表（②b B7，2026-09-02）：用例目录下的 .cn
    #   经宿主真实管线编译为 .o 参与链接且置于 v2p.obj 之前——v2p 只实例化过
    #   v2 自身用到的类型（如 126 的 词条），用例自有类型须供给符号与布局权威
    # v2 闭环用例（数据驱动，2026-09-11 用户裁决去硬编码）：用例目录含
    #   v2闭环.txt 即走 v2 自举链（宿主构建 v2p -> v2p 编译用例 -> as/g++ ->
    #   运行 -> 退出码比对）——配置全在用例目录（新增 v2 用例零 runner 改动）：
    #     退出码=N|负（负=预期 v2p 编译中止的负路径）
    #     源文件=主.cn,依赖.cn（默认 主.cn）
    #     借链=是|否（链接 v2p.obj——容器符号供给）
    #     供给=供给.cn（宿主管线编译供给源——用例自有类型布局权威）
    #   链接v2pobj（P7b 容器用例：v2 生成代码调用宿主编译的容器类方法符号，
    #   实现在 v2p.obj——stdlib 源码级并入编译产物）；供给源们（②b B7）：v2p
    #   只实例化过 v2 自身用到的类型（如 126 的 词条），用例自有类型须供给符号
    v2配置路径 = 用例目录 / "v2闭环.txt"
    if v2配置路径.exists():
        if 目标平台 not in ("win-x64", "linux-arm64", "linux-x86_64"):
            return "失败", f"{名称} 闭环用例仅支持 win-x64 / linux-arm64 / linux-x86_64（当前 {目标平台}）"
        v2配置 = 解析v2闭环配置(v2配置路径)
        源文件名们 = v2配置["源文件们"]
        预期退出码 = v2配置["预期退出码"]
        链接v2pobj = v2配置["链接v2pobj"]
        供给源们 = v2配置["供给源们"]
        return 执行v2闭环(编译器路径, 用例目录, 输出目录, 详细, 目标平台, 源文件名们, 预期退出码,
                        链接v2pobj, 供给源们)

    # ============ 双编译对照用例（plans/020 移植纪律 9 的门禁化，2026-09-11 第七十三轮）============
    # 目录含 双编译对照.txt 即走本编排（配置键与 v2闭环.txt 同格式，复用同一解析器）：
    #   同一份源码分别经**宿主真实管线**与 **v2 自举链**各编译运行，断言两侧退出码相等。
    #   动机：宿主与 v2 是两套独立实现（v2 为重实现非移植），同一机制两侧各自重写——
    #   「各自用例全绿」推不出「行为等价」（73-a 实证：v2 块级 RAII 从未生效而 v2 用例族
    #   全绿，因那些用例只断言内容，泄漏不改内容）。本编排是发现重实现分叉的门禁。
    #   三平台全支持（111-a 放开 win-x64）：win 侧 v2 产物已按用例隔离（v2work<编号>），
    #   宿主侧产物在隔离宿主目录（hostout）——两侧互不踩，31 个平台守卫用例转真跑。
    双编译配置路径 = 用例目录 / "双编译对照.txt"
    if 双编译配置路径.exists():
        if 目标平台 not in ("win-x64", "linux-arm64", "linux-x86_64"):
            return "失败", f"{名称} 双编译对照仅支持 win-x64 / linux-arm64 / linux-x86_64（当前 {目标平台}）"
        双配置 = 解析v2闭环配置(双编译配置路径)
        return 执行双编译对照(编译器路径, 用例目录, 输出目录, 详细, 目标平台,
                            双配置["源文件们"], 双配置["预期退出码"],
                            双配置["链接v2pobj"], 双配置["供给源们"])

    # 可执行文件后缀：Windows 下 .exe；Linux 下无后缀
    可执行后缀 = ".exe" if 目标平台 == "win-x64" else ""
    输出可执行 = 输出目录 / f"{名称}{可执行后缀}"
    # 清理陈旧的可执行文件（同时清理两平台后缀，避免残留旧产物误判），
    # 确保"编译成功但未生成"可被可靠检测
    for 后缀 in (".exe", ""):
        旧文件 = 输出目录 / f"{名称}{后缀}"
        if 旧文件.exists() and 旧文件.is_file():
            旧文件.unlink()

    # 1. 编译：cn build <源文件> --target <平台> --output <可执行文件>
    if 详细:
        print(f"    [编译] {编译器路径} build {源文件.name} "
              f"--target {目标平台} --output {输出可执行}")
    额外旗标 = 查找编译选项文件(用例目录)
    编译结果 = 运行命令([str(编译器路径), "build", str(源文件),
                      "--target", 目标平台,
                      "--output", str(输出可执行), *额外旗标], 项目根目录)
    if 编译结果.returncode != 0:
        提示 = (编译结果.stderr or 编译结果.stdout).strip()
        # 编译器尚未实现build命令：标记为"未实现"而非真实失败（阶段零预期状态）
        if 未实现标记 in 提示:
            return "未实现", f"编译器尚未实现build命令: {提示[:60]}"
        return "失败", f"编译失败(退出码{编译结果.returncode}): {提示[:200]}"
    if not 输出可执行.exists():
        return "失败", "编译返回成功但未生成可执行文件"

    # 2. 运行可执行文件（Task 6.2：存在同名 .input 文件时注入标准输入；
    #    Task 6.5：存在同名 .args 文件时追加命令行参数——argv[0]=可执行文件，
    #    后续参数来自 .args 文件每行一项）
    输入文件 = 查找输入文件(源文件)
    标准输入 = ""
    if 输入文件 is not None:
        标准输入 = 输入文件.read_text(encoding="utf-8")
    参数列表 = 查找参数文件(源文件)
    if 详细:
        if 输入文件 is not None:
            print(f"    [运行] {输出可执行} < {输入文件.name}")
        elif 参数列表:
            print(f"    [运行] {输出可执行} {' '.join(参数列表)}")
        else:
            print(f"    [运行] {输出可执行}")
    # 内存保护（2026-08-24）：78/79 等大规模编译用例运行时监控子进程工作集，
    # 超过 内存上限MB默认 立即自动终止并判失败（防 OOM 卡死拖垮机器）
    运行内存上限 = 内存上限MB默认 if any(
        前缀 in 名称 for 前缀 in 内存保护用例前缀) else 0
    运行结果 = 运行命令([str(输出可执行)] + 参数列表, 项目根目录, 标准输入,
                     运行内存上限)
    if 运行结果.returncode != 0:
        return "失败", f"运行失败(退出码{运行结果.returncode}): {运行结果.stderr.strip()[:200]}"

    # 3. 比对期望输出（逐行比对，忽略行尾空白差异）
    期望 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines()]
    实际 = [行.rstrip() for 行 in 运行结果.stdout.splitlines()]
    if 实际 != 期望:
        return "失败", f"输出不一致\n    期望: {期望}\n    实际: {实际}"
    return "通过", ""


# ============ v2 锚定链用例编排（78_v2 自举链构建 / 79_v2 自举闭环）============
# 2026-09-14 自 v1 78_chain_build / 79_bootstrap_closed_loop 迁移：对象由 v1 组件链
#   （CN语言编译器/，因字符串行 IR 内存失控被推倒）改为 v2 自举编译器全树自身
#   （CN语言编译器v2/）。锚定链口径（对齐既有 target/p111win/self_chain111.py 实践）：
#   [1] 宿主编译 v2 全树 -> v2p（ml64/link 或 as/g++；复用 确保v2p就绪* 指纹缓存）
#   [2] fix_p：v2p 编译 v2 全树自身 -> fix_p.asm（阶段=链构建 到此即通过）
#   [3] ml64/as 汇编 fix_p -> cn_self.obj
#   [4] link cn_self：cn_self.obj 在前 + v2p.obj 借链 + 运行时 objs，靠前定义胜出
#       （/FORCE:MULTIPLE | -Wl,-z,muldefs——防 v1 79 虚假验收教训重演）
#   [5] 绑定自检：cn_main 归属须为 cn_self.obj（win=map 解析；linux=ld -Map + nm 双检）
#   [6] fix_s：cn_self 编译 v2 全树自身 -> fix_s.asm
#   [7] 固定点：fix_p ≡ fix_s 逐字节一致（断言=自洽性，不依赖绝对行数——
#       v2 树每次变更行数锚即变，绝对行数锚会把树演化误判为用例失败）
# 三平台：win-x64 走 ml64/link；linux-arm64 / linux-x86_64 走 as/g++
#   （linux 分支对齐既有 v2 闭环编排；动态验证随跨机轮）
# 产物全部落 target/audit2/selfwork<编号>/（规则19），不污染工作区源码目录


def 解析v2锚定链配置(配置路径: pathlib.Path) -> dict:
    """v2锚定链.txt 键值解析：阶段（链构建|闭环，默认 闭环）；# 后为注释。"""
    配置 = {"阶段": "闭环"}
    for 原行 in 配置路径.read_text(encoding="utf-8").splitlines():
        行 = 原行.split("#", 1)[0].strip()
        if not 行 or "=" not in 行:
            continue
        键, 值 = (x.strip() for x in 行.split("=", 1))
        if 键 == "阶段":
            if 值 not in ("链构建", "闭环"):
                raise ValueError(f"v2锚定链.txt 阶段取值非法: {值}（须 链构建|闭环）")
            配置["阶段"] = 值
        else:
            raise ValueError(f"v2锚定链.txt 未知配置键: {键}（{配置路径}）")
    return 配置


def 锚定链编译v2全树(exe, 工作目录: pathlib.Path, 主入口: str, 目标asm: pathlib.Path,
                    详细: bool, 标签: str, 目标平台: str) -> tuple:
    """[2]/[6] 共用：exe 编译 v2 全树自身 -> 工作目录/target/v2asm.{asm|s} -> 拷贝为 目标asm。
    断言：退出码 0 + 产物存在 + 含入口符号 cn_main（防「报错仍产 asm / 空产物」假绿）。
    返回 (状态, 说明, 行数)；非「通过」时 行数=0。"""
    if 目标平台 == "win-x64":
        asm路径 = 工作目录 / "target" / "v2asm.asm"
        入口标记 = "cn_main PROC"
        命令 = [str(exe), 主入口]
    else:
        asm路径 = 工作目录 / "target" / "v2asm.s"
        入口标记 = ".globl cn_main"
        命令 = [str(exe), 主入口, 目标平台]  # GAS 后端分派（v2p 第 2 参数）
    if asm路径.exists():
        asm路径.unlink()
    if 详细:
        print(f"    [{标签}] {pathlib.Path(exe).name} {主入口}")
    运行结果 = 运行命令(命令, 工作目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", (f"{标签} 编译 v2 全树失败(退出码{运行结果.returncode}): "
                        f"{(运行结果.stderr or 运行结果.stdout or '').strip()[:300]}"), 0
    if not asm路径.exists():
        return "失败", f"{标签} 未生成 {asm路径.name}（v2 编译未落盘）", 0
    内容 = asm路径.read_text(encoding="utf-8", errors="replace")
    if 入口标记 not in 内容:
        return "失败", f"{标签} 产物缺少入口符号 cn_main（v2 代码生成入口未对齐宿主）", 0
    目标asm.write_bytes(asm路径.read_bytes())
    return "通过", "", 内容.count("\n") + 1


def 执行v2锚定链(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
                 输出目录: pathlib.Path, 详细: bool, 目标平台: str) -> tuple:
    """执行 v2 锚定链用例（78_v2 自举链构建 / 79_v2 自举闭环）——详见上方编排注释。"""
    import filecmp
    名称 = 用例目录.name
    编号 = 名称.split("_")[0]
    配置 = 解析v2锚定链配置(用例目录 / "v2锚定链.txt")
    阶段 = 配置["阶段"]
    if 目标平台 not in ("win-x64", "linux-arm64", "linux-x86_64"):
        return "失败", f"{名称} v2 锚定链仅支持 win-x64 / linux-arm64 / linux-x86_64（当前 {目标平台}）"
    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)
    主入口 = str((项目根目录 / "CN语言编译器v2" / "主.cn").resolve())
    # 产物行数下界（防「近乎空产物」假绿；win 首锚口径 408105 行，下界取约四分之一保守）
    行数下界 = 100000

    # 工作目录隔离（对齐 v2work<编号> 模式）：v2 驱动器输出路径为相对 cwd 的
    #   target/v2asm.*，各用例独立工作目录（cwd 隔离），产物互不踩
    工作目录 = 审计目录 / f"selfwork{编号}"
    (工作目录 / "target").mkdir(parents=True, exist_ok=True)
    # v2p/cn_self 的 stdlib 签名扫描按相对 cwd 读 stdlib/容器.cn（IR签名.cn:358）——
    #   workdir 内链接到项目根 stdlib（模块导入按入口目录解析不受 cwd 影响，唯此一处；
    #   win junction 不需管理员权限 / linux symlink）
    stdlib链 = 工作目录 / "stdlib"
    if not stdlib链.exists():
        if 目标平台 == "win-x64":
            subprocess.run(["cmd", "/c", "mklink", "/J", str(stdlib链),
                            str(项目根目录 / "stdlib")], capture_output=True)
        else:
            os.symlink(项目根目录 / "stdlib", stdlib链)

    # [1] 就绪：v2p（宿主编译 v2 全树；指纹缓存命中零重建）+ 工具链 + 运行时
    if 目标平台 == "win-x64":
        就绪 = 确保v2p就绪win(编译器路径, 详细, 编号)
        if 就绪[0] is None:
            return "失败", 就绪[1]
        v2p, v2pobj, 运行时objs, ML64, LINK, LIB路径们 = 就绪
    else:
        就绪 = 确保v2p与运行时就绪(编译器路径, 目标平台, 详细, 编号)
        if 就绪[0] is None:
            return "失败", 就绪[1]
        v2p, v2pobj, 运行时objs, as工具, cxx工具 = 就绪

    # ===== [2] fix_p：v2p 编译 v2 全树自身 -> fix_p.asm =====
    fixp = 工作目录 / "fix_p.asm"
    状态, 说明, 行数p = 锚定链编译v2全树(v2p, 工作目录, 主入口, fixp, 详细,
                                        f"{编号}-2 fix_p", 目标平台)
    if 状态 != "通过":
        return 状态, 说明
    if 行数p < 行数下界:
        return "失败", (f"{编号}-2 fix_p 行数 {行数p} < 下界 {行数下界}"
                        "（产物近乎空/严重退化——防假绿下界触发）")
    if 阶段 == "链构建":
        return "通过", (f"v2 链构建成立：v2p 编译 v2 全树自身 -> fix_p（{行数p} 行，"
                        f"含 cn_main；{目标平台}）")

    # ===== [3] 汇编 fix_p -> cn_self.obj =====
    cn_self_obj = 工作目录 / "cn_self.obj"
    cn_self_exe = 工作目录 / "cn_self.exe"
    if cn_self_obj.exists():
        cn_self_obj.unlink()
    if 目标平台 == "win-x64":
        if 详细:
            print(f"    [{编号}-3] ml64 汇编 fix_p.asm")
        汇编结果 = 运行命令([str(ML64), "/nologo", "/c", f"/Fo{cn_self_obj}", str(fixp)],
                          项目根目录)
    else:
        if 详细:
            print(f"    [{编号}-3] as 汇编 fix_p.s")
        汇编结果 = 运行命令([as工具, "-o", str(cn_self_obj), str(fixp)], 项目根目录)
    if 汇编结果.returncode != 0:
        return "失败", (f"{编号}-3 汇编 fix_p 失败(退出码{汇编结果.returncode}): "
                        f"{(汇编结果.stderr or 汇编结果.stdout or '').strip()[:300]}")
    if not cn_self_obj.exists():
        return "失败", f"{编号}-3 汇编返回成功但未生成 cn_self.obj"

    # ===== [4] 链接 cn_self（cn_self.obj 在前 + v2p.obj 借链 + 运行时；靠前定义胜出）=====
    if cn_self_exe.exists():
        cn_self_exe.unlink()
    map文件 = 工作目录 / f"{编号}_self_link.map"
    if map文件.exists():
        map文件.unlink()
    if 目标平台 == "win-x64":
        响应文件 = 工作目录 / f"{编号}_self_link.rsp"
        rsp_lines = ["/nologo", "/ENTRY:WinMainCRTStartup", "/SUBSYSTEM:CONSOLE",
                     "/STACK:8388608", "/FORCE:MULTIPLE"]
        for lib in LIB路径们:
            rsp_lines.append(f"/LIBPATH:{lib}")
        rsp_lines += ["/DEFAULTLIB:libcmt.lib", "/DEFAULTLIB:libucrt.lib",
                      "/DEFAULTLIB:kernel32.lib", "/DEFAULTLIB:shell32.lib",
                      f"/OUT:{cn_self_exe}", f"/MAP:{map文件}"]
        # cn_self.obj 必须在 v2p.obj 之前（防 v1 79 虚假验收教训：命令行靠前定义胜出）
        rsp_lines += [str(cn_self_obj), str(v2pobj)] + [str(o) for o in 运行时objs]
        with open(响应文件, "w", encoding="utf-8") as f:
            for 行 in rsp_lines:
                f.write(f'"{行}"\n')
        if 详细:
            print(f"    [{编号}-4] link -> cn_self.exe（cn_self.obj 在前 + v2p.obj 借链）")
        链接结果 = 运行命令([str(LINK), f"@{响应文件}"], 项目根目录)
        if 链接结果.returncode != 0:
            return "失败", (f"{编号}-4 链接失败(退出码{链接结果.returncode}): "
                            f"{(链接结果.stdout or '').strip()[:300]}")
    else:
        链接命令 = [cxx工具, "-no-pie", "-Wl,-z,muldefs", "-Wl,-Map," + str(map文件),
                    "-o", str(cn_self_exe), str(cn_self_obj), str(v2pobj)]
        链接命令 += [str(o) for o in 运行时objs]
        if 详细:
            print(f"    [{编号}-4] g++ -no-pie -Wl,-z,muldefs -> cn_self（cn_self.obj 在前）")
        链接结果 = 运行命令(链接命令, 项目根目录)
        if 链接结果.returncode != 0:
            return "失败", (f"{编号}-4 链接失败(退出码{链接结果.returncode}): "
                            f"{(链接结果.stderr or 链接结果.stdout or '').strip()[:300]}")
    if not cn_self_exe.exists():
        return "失败", f"{编号}-4 链接返回成功但未生成 cn_self.exe"

    # ===== [5] 绑定自检：cn_main 归属须为 cn_self.obj（防虚假验收——v1 79 教训）=====
    if 目标平台 == "win-x64":
        map内容 = map文件.read_text(encoding="utf-8", errors="replace") if map文件.exists() else ""
        cn_main行 = [l.strip() for l in map内容.splitlines()
                    if re.search(r"\bcn_main\b", l) and "Rva" not in l]
        归属 = [l for l in cn_main行 if l.endswith("cn_self.obj")]
        if not 归属:
            return "失败", (f"{编号}-5 绑定自检失败：cn_main 未绑定 cn_self.obj"
                            f"（虚假验收风险——链接顺序/借链失效）\n    map 行: {cn_main行[:5]}")
        归属说明 = "map: cn_main ∈ cn_self.obj ✓"
    else:
        # linux：ld -Map 解析（段块内符号归属）+ nm 复核（cn_self.obj 定义 T cn_main）
        map内容 = map文件.read_text(encoding="utf-8", errors="replace") if map文件.exists() else ""
        当前obj = None
        cn_main归属 = None
        for l in map内容.splitlines():
            m = re.match(r"^\s+\.\S+\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+\s+(\S+)", l)
            if m:
                当前obj = m.group(1)
                continue
            if 当前obj and re.match(r"^\s+0x[0-9a-fA-F]+\s+cn_main$", l):
                cn_main归属 = 当前obj
                break
        nm检查 = 运行命令(["nm", str(cn_self_obj)], 项目根目录)
        if " T cn_main" not in (nm检查.stdout or ""):
            return "失败", f"{编号}-5 绑定自检失败: cn_self.obj 未定义 T cn_main"
        if cn_main归属 is None or "cn_self.obj" not in cn_main归属:
            return "失败", (f"{编号}-5 绑定自检失败：ld -Map 中 cn_main 归属 {cn_main归属!r}"
                            "（非 cn_self.obj——虚假验收风险）")
        归属说明 = "ld -Map: cn_main ∈ cn_self.obj ✓ + nm T cn_main ✓"

    # ===== [6] fix_s：cn_self 编译 v2 全树自身 -> fix_s.asm =====
    fixs = 工作目录 / "fix_s.asm"
    状态, 说明, 行数s = 锚定链编译v2全树(cn_self_exe, 工作目录, 主入口, fixs, 详细,
                                        f"{编号}-6 fix_s", 目标平台)
    if 状态 != "通过":
        return 状态, 说明

    # ===== [7] 固定点判定：fix_p ≡ fix_s 逐字节一致 =====
    if filecmp.cmp(str(fixp), str(fixs), shallow=False):
        return "通过", (f"v2 自举闭环成立：fix_p ≡ fix_s 逐字节一致（{行数p} 行；"
                        f"{归属说明}；{目标平台}）")
    内容p = fixp.read_text(encoding="utf-8", errors="replace").splitlines()
    内容s = fixs.read_text(encoding="utf-8", errors="replace").splitlines()
    差异行 = [i for i, (x, y) in enumerate(zip(内容p, 内容s)) if x != y]
    return "失败", (f"{编号}-7 固定点不一致！fix_p={len(内容p)} 行 fix_s={len(内容s)} 行，"
                    f"首个差异行号={差异行[:5]}（v2 编译确定性/代码生成分叉）")


# v2 闭环用例配置解析（数据驱动，2026-09-11 用户裁决去硬编码）：用例目录的
#   v2闭环.txt 键值行取代 runner 内注册表——新增 v2 用例只需建目录+配置文件。
#   键：退出码（整数|负=负路径）、源文件（逗号分隔，默认 主.cn）、
#       借链（是|否）、供给（逗号分隔）；# 后为注释。
def 解析v2闭环配置(配置路径: pathlib.Path) -> dict:
    配置 = {"源文件们": ["主.cn"], "预期退出码": 0, "链接v2pobj": False, "供给源们": []}
    for 原行 in 配置路径.read_text(encoding="utf-8").splitlines():
        行 = 原行.split("#", 1)[0].strip()
        if not 行 or "=" not in 行:
            continue
        键, 值 = (x.strip() for x in 行.split("=", 1))
        if 键 == "退出码":
            配置["预期退出码"] = None if 值 == "负" else int(值)
        elif 键 == "源文件":
            配置["源文件们"] = [x.strip() for x in 值.split(",") if x.strip()] or ["主.cn"]
        elif 键 == "借链":
            配置["链接v2pobj"] = 值 in ("是", "true", "True", "1")
        elif 键 == "供给":
            配置["供给源们"] = [x.strip() for x in 值.split(",") if x.strip()]
        else:
            raise ValueError(f"v2闭环.txt 未知配置键: {键}（{配置路径}）")
    return 配置


# 判定用例是否 v2 系（预热共用——文件存在性判定）：
#   v2闭环.txt（纯 v2 用例）/ 双编译对照.txt（双编译对照用例，内部调用 v2 闭环）/
#   v2锚定链.txt（78_v2/79_v2 自举链构建与闭环，共享 v2p）
#   都依赖共享工件 v2p/v2 运行时 .o，故同进预热面（111-a 起三平台统一并行，
#   v2 产物已按用例隔离）。
def 是v2闭环用例(用例目录: pathlib.Path) -> bool:
    return ((用例目录 / "v2闭环.txt").exists() or (用例目录 / "双编译对照.txt").exists()
            or (用例目录 / "v2锚定链.txt").exists())



def 计算v2构建指纹(编译器路径: pathlib.Path) -> str:
    """v2p 构建缓存指纹（2026-09-11 并行提速）：v2 全树 .cn 的（相对路径+大小+
    mtime）排序汇总 + 宿主编译器（大小+mtime）——任一变化即指纹变化，触发重建。
    对齐 cargo 增量构建理念：输入未变不重建（30+ v2 用例全量 E2E 的最大瓶颈）。"""
    v2目录 = 项目根目录 / "CN语言编译器v2"
    项们 = []
    for f in sorted(v2目录.rglob("*.cn")):
        st = f.stat()
        项们.append(f"{f.relative_to(项目根目录)}:{st.st_size}:{int(st.st_mtime)}")
    if 编译器路径.exists():
        cst = 编译器路径.stat()
        项们.append(f"cn:{cst.st_size}:{int(cst.st_mtime)}")
    return hashlib.md5("\n".join(项们).encode("utf-8")).hexdigest()


def 计算运行时构建指纹(cxx工具: str) -> str:
    """rt objs 构建缓存指纹（2026-09-11 68-a 方案A 根治）：src/runtime/*.cpp 的
    （相对路径+大小+mtime）排序汇总 + 编译命令（cxx 路径+编译标志串）——任一
    变化即指纹变化，触发整批重编。与 计算v2构建指纹 同构（cargo fingerprint
    理念：命令行也纳入指纹）；此前「缺则现编」只查 .o 存在性，跨机 pull 后
    .cpp 已变而旧 .o 仍参与门禁链接（68-a 立案实证：陈旧 io/intern objs 掩盖
    8de2103/ea600b0 运行时修复）。"""
    项们 = []
    for f in sorted((项目根目录 / "src" / "runtime").glob("*.cpp")):
        st = f.stat()
        项们.append(f"{f.relative_to(项目根目录)}:{st.st_size}:{int(st.st_mtime)}")
    # 编译命令面：cxx 路径/大小/mtime + 标志串（命令行变化=产物语义变化，cargo 同款）
    cxx路径 = pathlib.Path(cxx工具)
    if cxx路径.exists():
        cst = cxx路径.stat()
        项们.append(f"cxx:{cst.st_size}:{int(cst.st_mtime)}")
    else:
        项们.append(f"cxx:{cxx工具}")
    项们.append("flags:-std=c++17 -fno-exceptions -fno-rtti -DCNRT_LINUX_MAIN -Isrc")
    return hashlib.md5("\n".join(项们).encode("utf-8")).hexdigest()


def 确保v2p与运行时就绪(编译器路径: pathlib.Path, 目标平台: str, 详细: bool, 编号: str = "PRE"):
    """v2 闭环共享工件就绪（2026-09-11 全量并行裁决）：工具链探测 + 运行时 .o
    （指纹缓存=src/runtime 全部 .cpp 路径/大小/mtime+cxx+标志串，不符整批重编
    ——68-a 方案A 根治，杜绝陈旧 .o 参与门禁链接）+ v2p 构建缓存（指纹=v2 全树
    .cn mtime/size+编译器 mtime/size，cargo 增量构建理念——输入不变不重建）。
    返回 (v2p, v2pobj, 运行时objs, as工具, cxx工具)；失败返回 (None, 错误信息)。
    并行纪律：主程序在统一并行池启动前调用一次（预热——消除并发构建竞态），
    各 v2 用例再调用时必命中缓存（纯只读，无竞态）。"""
    import shutil
    import os
    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)

    # 工具链探测（对齐宿主 cn_main.cpp：CN_AS/CN_CXX 环境变量优先，PATH，便携 gcc7 兜底）
    as工具 = os.environ.get("CN_AS") or shutil.which("as") or "/home/user/gcc7/usr/bin/as"
    cxx工具 = os.environ.get("CN_CXX") or shutil.which("g++") or "/home/user/gcc7/usr/bin/g++"
    if shutil.which(as工具) is None and not pathlib.Path(as工具).exists():
        return None, f"未找到 as 汇编器（{as工具}）"
    if shutil.which(cxx工具) is None and not pathlib.Path(cxx工具).exists():
        return None, f"未找到 g++ 链接器（{cxx工具}）"

    # 运行时 .o（对齐宿主编译命令 g++ -c -std=c++17 -fno-exceptions -fno-rtti
    #   -DCNRT_LINUX_MAIN）+ 指纹缓存（2026-09-11 68-a 方案A 根治：指纹=src/runtime
    #   全部 .cpp 路径/大小/mtime+cxx+标志串，不符整批重编——杜绝跨机 pull 后陈旧
    #   .o 参与门禁链接；整批原子重编避免半新半旧混链，与 v2p 缓存同款模式）
    运行时名们 = ["io_api", "intern_api", "runtime", "string_api", "i128_api",
                "math_api", "input_api", "file_api", "time_api", "system_api"]
    rt缓存键路径 = 审计目录 / "rt_build_key.txt"
    rt指纹 = 计算运行时构建指纹(cxx工具)
    rt全部存在 = all((审计目录 / f"{模块}.o").exists() for 模块 in 运行时名们)
    if rt全部存在 and rt缓存键路径.exists() \
            and rt缓存键路径.read_text(encoding="utf-8") == rt指纹:
        if 详细:
            print(f"    [{编号}-0] rt objs 缓存命中（src/runtime 未变），复用 {审计目录}/*.o")
    else:
        for 模块 in 运行时名们:
            obj = 审计目录 / f"{模块}.o"
            if obj.exists():
                obj.unlink()
            编译rt = 运行命令([cxx工具, "-c", "-std=c++17", "-fno-exceptions", "-fno-rtti",
                            "-DCNRT_LINUX_MAIN", "-Isrc", "-o", str(obj),
                            f"src/runtime/{模块}.cpp"], 项目根目录)
            if 编译rt.returncode != 0:
                return None, f"{编号}-0 运行时 {模块}.o 编译失败: {(编译rt.stderr or 编译rt.stdout).strip()[:200]}"
        rt缓存键路径.write_text(rt指纹, encoding="utf-8")
    运行时objs = [审计目录 / f"{模块}.o" for 模块 in 运行时名们]

    # v2p 构建缓存（30+ v2 用例全量 E2E 最大瓶颈——每个全树构建数十秒）
    产物后缀 = "linux" if 目标平台 == "linux-arm64" else "linuxx64"
    v2p = 审计目录 / f"v2p_{产物后缀}"
    v2pobj = 审计目录 / f"v2p_{产物后缀}.o"
    缓存键路径 = 审计目录 / f"v2p_build_key_{产物后缀}.txt"
    本次指纹 = 计算v2构建指纹(编译器路径)
    if (v2p.exists() and v2pobj.exists() and 缓存键路径.exists()
            and 缓存键路径.read_text(encoding="utf-8") == 本次指纹):
        if 详细:
            print(f"    [{编号}-1] v2p 构建缓存命中（v2 源码与编译器未变），复用 {v2p.name}")
        return v2p, v2pobj, 运行时objs, as工具, cxx工具
    if v2p.exists():
        v2p.unlink()
    if v2pobj.exists():
        v2pobj.unlink()
    if 详细:
        print(f"    [{编号}-1] {编译器路径} build 主.cn -> {v2p.name}（{目标平台}）")
    编译结果 = 运行命令([str(编译器路径), "build", str(项目根目录 / "CN语言编译器v2" / "主.cn"),
                      "--target", 目标平台, "--output", str(v2p)], 项目根目录)
    if 编译结果.returncode != 0:
        return None, f"{编号}-1 编译 v2 组件失败(退出码{编译结果.returncode}): {(编译结果.stderr or 编译结果.stdout).strip()[:200]}"
    if not v2p.exists():
        return None, f"{编号}-1 编译返回成功但未生成 {v2p.name}"
    if not v2pobj.exists():
        return None, f"{编号}-1 中间产物 {v2pobj.name} 未留存（容器符号提供者）"
    缓存键路径.write_text(本次指纹, encoding="utf-8")
    return v2p, v2pobj, 运行时objs, as工具, cxx工具


def 确保v2p就绪win(编译器路径: pathlib.Path, 详细: bool, 编号: str = "PRE"):
    """win-x64 侧 v2p 就绪（111-a：自 执行v2闭环 原内联步骤0/1 提取，供预热与各用例共用）：
    工具链探测（ml64/link + MSVC/Kits LIB 路径）→ 运行时 .obj 存在性 → v2p 构建缓存
    （v2 全树指纹，输入未变不重建）。
    并发安全：池启动前单线程预热一次（写缓存），此后各用例只读命中——消除并发首建
    竞态（对齐 linux 分支 确保v2p与运行时就绪 的同款预热模式）。
    返回 (v2p, v2pobj, 运行时objs, ML64, LINK, LIB路径们)；失败返回 (None, 错误描述)。"""
    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)
    v2源码目录 = 项目根目录 / "CN语言编译器v2"
    v2p = 审计目录 / "v2p.exe"
    v2pobj = 审计目录 / "v2p.obj"

    # 工具链绝对路径（VS 2022，与79同源探测）
    MSVC根 = r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
    ML64 = None
    LINK = None
    for 版本 in ("14.44.35207", "14.38.33130"):
        候选ml64 = pathlib.Path(MSVC根) / 版本 / "bin" / "Hostx64" / "x64" / "ml64.exe"
        候选link = pathlib.Path(MSVC根) / 版本 / "bin" / "Hostx64" / "x64" / "link.exe"
        if 候选ml64.exists() and ML64 is None:
            ML64 = 候选ml64
        if 候选link.exists() and LINK is None:
            LINK = 候选link
    if ML64 is None or LINK is None:
        return None, "未找到 ml64/link（VS 2022 MSVC 工具链）"

    # LIB 路径（MSVC + Windows Kits）
    msvc版本目录 = ML64.parent.parent.parent.parent
    kits根 = pathlib.Path(r"C:\Program Files (x86)\Windows Kits\10\lib")
    kits版 = sorted((p for p in kits根.glob("10.*") if p.is_dir()), reverse=True) if kits根.exists() else []
    if not kits版:
        return None, f"未找到 Windows Kits lib 目录: {kits根}\\10.*（请确认 Win10 SDK 安装）"
    LIB路径们 = [
        str(msvc版本目录 / "lib" / "x64"),
        str(kits版[0] / "ucrt" / "x64"),
        str(kits版[0] / "um" / "x64"),
    ]
    for lib路径 in LIB路径们:
        if not pathlib.Path(lib路径).exists():
            return None, f"LIB 路径不存在: {lib路径}"

    # 运行时 .obj（C++ 版构建产物；对齐宿主链接命令 10 个）
    运行时名们 = ["io_api", "intern_api", "runtime", "string_api", "i128_api",
                "math_api", "input_api", "file_api", "time_api", "system_api"]
    运行时objs = [项目根目录 / "target" / f"{m}.obj" for m in 运行时名们]
    for obj in 运行时objs:
        if not obj.exists():
            return None, f"缺少运行时 .obj: {obj.name}（请先构建 C++ 版编译器）"

    # v2p 构建缓存（同 Linux 分支——输入未变不重建）
    缓存键路径 = 审计目录 / "v2p_build_key_win.txt"
    本次指纹 = 计算v2构建指纹(编译器路径)
    if (v2p.exists() and v2pobj.exists() and 缓存键路径.exists()
            and 缓存键路径.read_text(encoding="utf-8") == 本次指纹):
        if 详细:
            print(f"    [{编号}-1] v2p 构建缓存命中（v2 源码与编译器未变），复用 {v2p.name}")
        return v2p, v2pobj, 运行时objs, ML64, LINK, LIB路径们
    if v2p.exists():
        v2p.unlink()
    if v2pobj.exists():
        v2pobj.unlink()
    if 详细:
        print(f"    [{编号}-1] {编译器路径} build 主.cn -> v2p.exe")
    编译结果 = 运行命令([str(编译器路径), "build", str(v2源码目录 / "主.cn"),
                      "--target", "win-x64", "--output", str(v2p)], 项目根目录)
    if 编译结果.returncode != 0:
        return None, f"{编号}-1 编译 v2 组件失败(退出码{编译结果.returncode}): {(编译结果.stderr or 编译结果.stdout).strip()[:200]}"
    if not v2p.exists():
        return None, f"{编号}-1 编译返回成功但未生成 v2p.exe"
    if not v2pobj.exists():
        return None, f"{编号}-1 中间产物 {v2pobj.name} 未留存（容器符号提供者）"
    缓存键路径.write_text(本次指纹, encoding="utf-8")
    return v2p, v2pobj, 运行时objs, ML64, LINK, LIB路径们


def 执行v2闭环Linux(编译器路径: pathlib.Path, 目标平台: str, 详细: bool,
                   源文件名们: list, 预期退出码: int, 链接v2pobj: bool,
                   期望文件, 审计目录: pathlib.Path, v2源码目录: pathlib.Path,
                   用例目录: pathlib.Path, 名称: str, 编号: str,
                   供给源们: list = None) -> tuple:
    """执行 v2 自举链接闭环（linux 两平台，阶段A 2026-09-02 ARM64 首建；plans/017 T3
    2026-09-06 平台参数化并入 linux-x86_64）：v2 GAS 后端 -> as -> g++ -> 运行

    目标平台 = linux-arm64 | linux-x86_64（宿主编译 --target 与 v2p 第 2 参数
    随平台分派；linux-x86_64 走 v2 X64L 后端，SysV GAS）。arm64 产物沿用
    v2p_linux* 原名（行为零变化）；x86_64 用 v2p_linuxx64*（两平台全量先后
    运行中间产物不互踩）。
    链接对齐宿主 linux 命令（cn_main.cpp）：g++ -no-pie + 运行时 .o（-DCNRT_LINUX_MAIN）；
    链接 v2p .o 时 cn_main 双定义 -> -Wl,-z,muldefs + v2asm.o 命令行在前
    （靠前定义胜出——对齐 win64 /FORCE:MULTIPLE 编排）。
    供给源们（②b B7）：用例自有类型符号供给——宿主 cn build 编译（.o 为构建副
    产品留存），链接置于 v2p .o 之前（用例结构体布局权威）。
    预期退出码 None（139/141 负路径，2026-09-04 ARM64 复验轮对齐）：v2p 须编译失败
    ——退出码非 0、target/v2asm.s 不产出、中止诊断行（.expected 固化）在输出中
    （v2 语义错误即中止纪律平台无关，win64 同款）。
    POSIX 退出码 8 位截断：退出码比对取 预期退出码 % 256（win64 为 32 位全值）。"""
    import shutil
    import os
    if 供给源们 is None:
        供给源们 = []

    就绪 = 确保v2p与运行时就绪(编译器路径, 目标平台, 详细, 编号)
    if 就绪[0] is None:
        return "失败", 就绪[1]
    v2p, v2pobj, 运行时objs, as工具, cxx工具 = 就绪
    产物后缀 = "linux" if 目标平台 == "linux-arm64" else "linuxx64"
    # v2 产物按用例隔离（2026-09-11 全量并行裁决）：v2p 的 asm 输出路径为相对
    #   cwd 的 target/v2asm.s（v2 驱动器写死相对路径）——每用例独立工作目录
    #   （cwd 隔离，对齐 cargo test 进程隔离理念），v2asm/obj/exe 互不踩；
    #   入口参数改绝对路径（cwd 不再是项目根），日志锚行比对前把绝对前缀
    #   适配回相对（.expected 文件保持相对路径文本不动）。
    工作目录 = 审计目录 / f"v2work{编号}"
    (工作目录 / "target").mkdir(parents=True, exist_ok=True)
    # v2p 的 stdlib 签名扫描按相对 cwd 读 stdlib/容器.cn（IR签名.cn:358）——
    #   workdir 内软链到项目根 stdlib（模块导入按入口目录解析不受 cwd 影响，唯此一处）
    stdlib链 = 工作目录 / "stdlib"
    if not stdlib链.exists():
        os.symlink(项目根目录 / "stdlib", stdlib链)
    v2asm路径 = 工作目录 / "target" / "v2asm.s"

    # ===== 负路径闭环（139/141，win64 同款 2026-09-04；linux 侧 ARM64 复验轮对齐）：
    #   预期退出码 None = v2p 须编译失败——语义错误即中止纪律的 E2E 锚定：
    #   v2p 退出码非 0、target/v2asm.s 不产出、中止诊断行（.expected 固化）在
    #   输出中——防「报错仍产 asm」回归。v2p 运行须带目标平台第 2 参数
    #   （GAS 后端分派；win64 默认后端无需参数）=====
    if 预期退出码 is None:
        v2src目录 = 审计目录 / f"v2src{编号}"
        v2src目录.mkdir(parents=True, exist_ok=True)
        for 文件名 in 源文件名们:
            src = 用例目录 / 文件名
            if not src.exists():
                return "失败", f"{编号}-N 缺少用例文件: {文件名}"
            shutil.copy2(src, v2src目录 / 文件名)
        # plans/018（2026-09-07）：负路径与正路径步骤 2 同款整树复制——模块系统
        #   负测用例（168~170 等）的导入依赖模块文件须随入口就位（ignore 期望/
        #   输入文件；既有平铺负测（139/141/146/149）行为等价）
        shutil.copytree(用例目录, v2src目录, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args"))
        入口参数 = str((审计目录 / f"v2src{编号}" / "主.cn").resolve())
        if v2asm路径.exists():
            v2asm路径.unlink()
        if 详细:
            print(f"    [{编号}-N] {v2p.name} {入口参数} {目标平台}（预期语义错误中止）")
        运行结果 = 运行命令([str(v2p), 入口参数, 目标平台], 工作目录, 内存上限MB=内存上限MB默认)
        if 运行结果.returncode == 0:
            return "失败", f"{编号}-N 预期 v2p 语义错误中止但退出码 0（错误产物纪律回归）"
        if v2asm路径.exists():
            return "失败", f"{编号}-N v2p 语义错误中止后仍产出 target/v2asm.s（错误产物纪律回归）"
        期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
        # 入口为绝对路径（cwd 隔离）——日志锚行比对前把绝对前缀适配回相对
        #   （.expected 保持相对路径文本不动，与既有 win64 .asm 平台适配同族）
        实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or "")).replace(
            str(项目根目录) + "/", "")
        for 行 in 期望行们:
            # 平台适配（与正路径步骤3 同款）：期望若引用 win64 路径须替换为 GAS 产物名
            适配行 = 行.replace("target/v2asm.asm", "target/v2asm.s")
            if 适配行 not in 实际输出:
                return "失败", f"{编号}-N v2p 输出缺少期望行: {适配行!r}\n    实际: {实际输出[:400]}"
        return "通过", (f"v2 语义错误中止负路径闭环成立（{目标平台}，退出码 {运行结果.returncode}，"
                        "无 asm 产出，诊断行固化）")

    # ===== 步骤1.5：供给源编译（②b B7）——用例自有类型符号，宿主真实管线 =====
    #   注意：宿主模块系统按规范08-四 仅以 主.cn 为入口模块（其余按导入模块合并
    #   公开声明）——供给源须拷贝为独立目录的 主.cn 再编译，函数才完整发射。
    供给objs = []
    for 供给名 in 供给源们:
        供给src = 用例目录 / 供给名
        if not 供给src.exists():
            return "失败", f"{编号}-1.5 缺少供给源文件: {供给名}"
        # 供给产物路径一律 ASCII（supplyN_M）：rsp/命令行传给 ml64/link/as/g++ 的路径
        # 若含中文，会按系统代码页误读（win GBK 下 UTF-8 路径成乱码，LNK1181）
        供给目录 = 审计目录 / f"supply{编号}_{len(供给objs)}"
        供给目录.mkdir(parents=True, exist_ok=True)
        shutil.copy2(供给src, 供给目录 / "主.cn")
        供给输出 = 供给目录 / "supply"
        供给obj = 供给目录 / "supply.o"
        if 供给输出.exists():
            供给输出.unlink()
        if 供给obj.exists():
            供给obj.unlink()
        if 详细:
            print(f"    [{编号}-1.5] {编译器路径} build {供给名} -> {供给obj.name}（符号供给）")
        供给编译 = 运行命令([str(编译器路径), "build", str(供给目录 / "主.cn"),
                          "--target", 目标平台, "--output", str(供给输出)], 项目根目录)
        if 供给编译.returncode != 0:
            return "失败", f"{编号}-1.5 供给源 {供给名} 编译失败(退出码{供给编译.returncode}): {(供给编译.stderr or 供给编译.stdout).strip()[:200]}"
        if not 供给obj.exists():
            return "失败", f"{编号}-1.5 供给源 {供给名} 未产出 {供给obj.name}"
        供给objs.append(供给obj)

    # ===== 步骤2：准备多文件程序（入口 主.cn + 导入模块文件） =====
    v2src目录 = 审计目录 / f"v2src{编号}"
    v2src目录.mkdir(parents=True, exist_ok=True)
    for 文件名 in 源文件名们:
        src = 用例目录 / 文件名
        if not src.exists():
            return "失败", f"{编号}-2 缺少用例文件: {文件名}"
        shutil.copy2(src, v2src目录 / 文件名)
    # plans/018 P6b（2026-09-07）：模块系统加载器用例目录树——父挂子子模块
    #   （<名>/<子>.cn）、货舱.toml、依赖/<名>/… 须随入口整树复制（ignore
    #   期望/输入文件；既有平铺用例行为等价）
    shutil.copytree(用例目录, v2src目录, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args"))

    # ===== 步骤3：运行 v2p（第 2 参数目标平台分派 GAS 后端）=====
    入口参数 = str((审计目录 / f"v2src{编号}" / "主.cn").resolve())
    if v2asm路径.exists():
        v2asm路径.unlink()
    if 详细:
        print(f"    [{编号}-3] {v2p.name} {入口参数} {目标平台}")
    运行结果 = 运行命令([str(v2p), 入口参数, 目标平台], 工作目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", f"{编号}-3 v2p 运行失败(退出码{运行结果.returncode}): {(运行结果.stderr or '').strip()[:300]}"
    if not v2asm路径.exists():
        return "失败", f"{编号}-3 v2p 未生成 target/v2asm.s"
    期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
    # 入口为绝对路径（cwd 隔离）——日志锚行比对前把绝对前缀适配回相对
    实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or "")).replace(
        str(项目根目录) + "/", "")
    for 行 in 期望行们:
        # 平台适配（不改测试文件）：GAS 后端输出 target/v2asm.s（win64 期望为 .asm）
        适配行 = 行.replace("target/v2asm.asm", "target/v2asm.s")
        if 适配行 not in 实际输出:
            return "失败", f"{编号}-3 v2p 输出缺少期望行: {适配行!r}\n    实际: {实际输出[:400]}"
    asm内容 = v2asm路径.read_text(encoding="utf-8", errors="replace")
    if ".globl cn_main" not in asm内容:
        return "失败", f"{编号}-3.5 v2asm.s 缺少入口符号 cn_main（v2 代码生成入口未对齐宿主）"

    # ===== 步骤4：as 汇编 target/v2asm.s -> v2asm_<平台>.o =====
    v2obj = 审计目录 / f"v2asm_{编号}_{产物后缀}.o"
    if v2obj.exists():
        v2obj.unlink()
    if 详细:
        print(f"    [{编号}-4] as v2asm.s")
    汇编结果 = 运行命令([as工具, "-o", str(v2obj), str(v2asm路径)], 项目根目录)
    if 汇编结果.returncode != 0:
        return "失败", f"{编号}-4 as 汇编失败(退出码{汇编结果.returncode}): {(汇编结果.stderr or 汇编结果.stdout).strip()[:300]}"
    if not v2obj.exists():
        return "失败", f"{编号}-4 as 返回成功但未生成 v2asm_linux.o"

    # ===== 步骤5：链接（对齐宿主 linux 链接命令 g++ -no-pie + 运行时 .o）=====
    输出exe = 审计目录 / f"v2out_{编号}_{产物后缀}"
    if 输出exe.exists():
        输出exe.unlink()
    链接命令 = [cxx工具, "-no-pie"]
    if 链接v2pobj:
        # cn_main 双定义（v2asm 与 v2p 各有 主()）——muldefs + 命令行在前者胜出
        链接命令.append("-Wl,-z,muldefs")
    链接命令 += ["-o", str(输出exe), str(v2obj)]
    # 供给 .o 置于 v2p_linux.o 之前（②b B7：用例自有类型布局权威——muldefs 下先定义胜出）
    链接命令 += [str(o) for o in 供给objs]
    if 链接v2pobj:
        链接命令.append(str(v2pobj))
    链接命令 += [str(o) for o in 运行时objs]
    if 详细:
        print(f"    [{编号}-5] g++ -no-pie -> {输出exe.name}")
    链接结果 = 运行命令(链接命令, 项目根目录)
    if 链接结果.returncode != 0:
        return "失败", f"{编号}-5 链接失败(退出码{链接结果.returncode}): {(链接结果.stderr or 链接结果.stdout).strip()[:300]}"
    if not 输出exe.exists():
        return "失败", f"{编号}-5 链接返回成功但未生成 exe"
    # 符号自检（防虚假验收，对齐 win64 map 自检）：cn_main 须由 v2asm_linux.o 定义；
    #   容器用例的 向量$ 方法符号须来自 v2p_linux.o（muldefs 下 v2asm 在前绑定 cn_main）
    nm检查 = 运行命令(["nm", str(v2obj)], 项目根目录)
    if " T cn_main" not in (nm检查.stdout or ""):
        return "失败", f"{编号}-5.5 符号自检失败: v2asm_linux.o 未定义 T cn_main"
    if 链接v2pobj:
        nm检查2 = 运行命令(["nm", str(v2pobj)], 项目根目录)
        if "T _E59091E9878F24" not in (nm检查2.stdout or ""):  # 向量$（E59091E9878F24）hex 前缀
            return "失败", f"{编号}-5.5 符号自检失败: v2p_linux.o 缺少向量类方法符号"

    # ===== 步骤6：运行 v2 产物 -> 退出码须等于用例预期值（POSIX 8 位截断）=====
    运行结果2 = 运行命令([str(输出exe)], 项目根目录)
    预期值 = 预期退出码 % 256
    if 运行结果2.returncode != 预期值:
        return "失败", (f"{编号}-6 v2 产物运行退出码={运行结果2.returncode}"
                        f"（期望 {预期值} = {预期退出码} % 256）: {(运行结果2.stderr or '').strip()[:200]}")
    return "通过", f"v2 多文件编译（{名称}）-> as/g++（对齐宿主 linux 链接命令，{目标平台}）-> 运行 闭环成立（退出码 {预期值}）"


def 执行v2闭环(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
               输出目录: pathlib.Path, 详细: bool, 目标平台: str,
               源文件名们: list, 预期退出码: int, 链接v2pobj: bool = False,
               供给源们: list = None) -> tuple:
    """执行 v2 自举链接闭环（119/120… 通用，三平台）：v2 多文件编译 -> 链接宿主运行时 -> 运行

    目标平台 = win-x64（ml64/link 原路径）| linux-arm64 / linux-x86_64（as/g++，
      阶段A 2026-09-02 ARM64 首建；plans/017 T3 2026-09-06 平台参数化并入 x86_64）；
    源文件名们 = 用例的全部源文件（首个为主入口 主.cn，须含导入模块文件）；
    预期退出码 = v2 产物 exe 运行的期望退出码（用例断言值）；
    链接v2pobj = 是否链接 v2p.obj（P7b 容器用例：容器方法实现来自宿主编译的
      v2 组件 obj——rustc 预编译 std 模式；/FORCE:MULTIPLE + v2asm.obj 在前
      保证 cn_main 绑定 v2 产物，容器符号绑定 v2p.obj，map 自检双方向）；
    供给源们 = 用例自有类型的符号供给 .cn 列表（②b B7，2026-09-02）——宿主真实
      管线编译为 .obj，链接置于 v2p.obj 之前（用例结构体布局权威）。"""
    名称 = 用例目录.name
    编号 = 名称.split("_")[0]  # 步骤号前缀与 v2src 目录名后缀（119/120…）
    期望文件 = 查找期望文件(用例目录 / 源文件名们[0])
    if 供给源们 is None:
        供给源们 = []
    import shutil

    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)
    v2源码目录 = 项目根目录 / "CN语言编译器v2"

    # ---- linux 分支（阶段A：v2 GAS 后端 + as/g++ 编排；plans/017 T3 参数化双平台）----
    #   负路径（预期退出码 None，139/141）已对齐（2026-09-04 ARM64 复验轮）——
    #   执行v2闭环Linux 内同款分支：v2p 须失败且不产 target/v2asm.s
    if 目标平台 in ("linux-arm64", "linux-x86_64"):
        return 执行v2闭环Linux(编译器路径, 目标平台, 详细, 源文件名们, 预期退出码, 链接v2pobj,
                              期望文件, 审计目录, v2源码目录, 用例目录, 名称, 编号, 供给源们)

    # ---- win-x64 原路径（ml64/link）----
    # v2 产物按用例隔离（111-a，对齐 linux 分支）：v2 驱动器 asm 输出路径为相对
    #   cwd 的 target/v2asm.asm——此前共享项目根 target/v2asm.asm（v2 用例被迫
    #   单线程串行，并发即互踩）。现每用例独立工作目录（cwd 隔离，对齐 cargo test
    #   进程隔离理念），v2asm/obj/exe 互不踩；入口参数改绝对路径（cwd 不再是项目
    #   根），日志锚行比对前把绝对前缀适配回相对（同 linux 分支做法——v2 输出
    #   路径已归一为正斜杠形式）。
    工作目录 = 审计目录 / f"v2work{编号}"
    (工作目录 / "target").mkdir(parents=True, exist_ok=True)
    # v2p 的 stdlib 签名扫描按相对 cwd 读 stdlib/容器.cn（IR签名.cn:358）——
    #   workdir 内 junction 到项目根 stdlib（模块导入按入口目录解析不受 cwd 影响，
    #   唯此一处）；junction 不需管理员权限（对齐 linux 分支 os.symlink 同款）
    stdlib链 = 工作目录 / "stdlib"
    if not stdlib链.exists():
        subprocess.run(["cmd", "/c", "mklink", "/J", str(stdlib链),
                        str(项目根目录 / "stdlib")], capture_output=True)
    v2asm路径 = 工作目录 / "target" / "v2asm.asm"

    # 工具链/运行时/v2p 就绪（111-a 提取为函数：主程序池启动前单线程预热，消除
    #   并发首建竞态；此处调用为 filter 直跑等路径的兜底——缓存命中时零重建）
    就绪 = 确保v2p就绪win(编译器路径, 详细, 编号)
    if 就绪[0] is None:
        return "失败", 就绪[1]
    v2p, v2pobj, 运行时objs, ML64, LINK, LIB路径们 = 就绪

    # ===== 负路径闭环（灰色点⑤，2026-09-04）：预期退出码 None = v2p 须编译失败 =====
    #   语义错误即中止纪律的 E2E 锚定：v2p 退出码非 0、target/v2asm.asm 不产出、
    #   中止诊断行（.expected 固化）在输出中——防「报错仍产 asm」回归
    if 预期退出码 is None:
        入口参数 = str((审计目录 / f"v2src{编号}" / "主.cn").resolve())
        v2src目录 = 审计目录 / f"v2src{编号}"
        v2src目录.mkdir(parents=True, exist_ok=True)
        for 文件名 in 源文件名们:
            src = 用例目录 / 文件名
            if not src.exists():
                return "失败", f"{编号}-N 缺少用例文件: {文件名}"
            shutil.copy2(src, v2src目录 / 文件名)
        # plans/018 跨机轮 win 侧对齐（2026-09-09）：负路径与 linux 分支/正路径
        #   同款整树复制——模块系统负测（168~170）的导入依赖模块文件须随入口
        #   就位（ignore 期望/输入文件；既有平铺负测 139/141/146/149 行为等价）
        shutil.copytree(用例目录, v2src目录, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args"))
        if v2asm路径.exists():
            v2asm路径.unlink()
        if 详细:
            print(f"    [{编号}-N] {v2p.name} {入口参数}（预期语义错误中止）")
        运行结果 = 运行命令([str(v2p), 入口参数], 工作目录, 内存上限MB=内存上限MB默认)
        if 运行结果.returncode == 0:
            return "失败", f"{编号}-N 预期 v2p 语义错误中止但退出码 0（错误产物纪律回归）"
        if v2asm路径.exists():
            return "失败", f"{编号}-N v2p 语义错误中止后仍产出 target/v2asm.asm（错误产物纪律回归）"
        期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
        # 入口为绝对路径（cwd 隔离）——日志锚行比对前把绝对前缀适配回相对
        #   （.expected 保持相对路径文本不动；v2 输出路径已归一为正斜杠）
        实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or "")).replace(
            str(项目根目录).replace("\\", "/") + "/", "")
        for 行 in 期望行们:
            if 行 not in 实际输出:
                return "失败", f"{编号}-N v2p 输出缺少期望行: {行!r}\n    实际: {实际输出[:400]}"
        return "通过", f"v2 语义错误中止负路径闭环成立（退出码 {运行结果.returncode}，无 asm 产出，诊断行固化）"

    # ===== 步骤2：准备多文件程序（入口 主.cn + 导入模块文件） =====
    v2src目录 = 审计目录 / f"v2src{编号}"
    v2src目录.mkdir(parents=True, exist_ok=True)
    for 文件名 in 源文件名们:
        src = 用例目录 / 文件名
        if not src.exists():
            return "失败", f"{编号}-2 缺少用例文件: {文件名}"
        shutil.copy2(src, v2src目录 / 文件名)
    # plans/018 P6b（2026-09-07）：模块系统加载器用例目录树——父挂子子模块
    #   （<名>/<子>.cn）、货舱.toml、依赖/<名>/… 须随入口整树复制（ignore
    #   期望/输入文件；既有平铺用例行为等价）
    shutil.copytree(用例目录, v2src目录, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args"))

    # ===== 步骤2.5：供给源编译（②b B7，对齐 linux 分支）——用例自有类型符号 =====
    #   同 linux：须拷贝为独立目录 主.cn（宿主仅以 主.cn 为入口模块，规范08-四）
    供给objs = []
    for 供给名 in 供给源们:
        供给src = 用例目录 / 供给名
        if not 供给src.exists():
            return "失败", f"{编号}-2.5 缺少供给源文件: {供给名}"
        # 供给产物路径一律 ASCII（supplyN_M）：rsp 传给 link.exe 的路径含中文时
        # 按系统代码页（GBK）误读 UTF-8 → LNK1181（对齐 linux 分支同款惯例）
        供给目录 = 审计目录 / f"supply{编号}_{len(供给objs)}"
        供给目录.mkdir(parents=True, exist_ok=True)
        shutil.copy2(供给src, 供给目录 / "主.cn")
        供给输出 = 供给目录 / "supply.exe"
        供给obj = 供给目录 / "supply.obj"
        if 供给输出.exists():
            供给输出.unlink()
        if 供给obj.exists():
            供给obj.unlink()
        if 详细:
            print(f"    [{编号}-2.5] {编译器路径} build {供给名} -> {供给obj.name}（符号供给）")
        供给编译 = 运行命令([str(编译器路径), "build", str(供给目录 / "主.cn"),
                          "--target", "win-x64", "--output", str(供给输出)], 项目根目录)
        if 供给编译.returncode != 0:
            return "失败", f"{编号}-2.5 供给源 {供给名} 编译失败(退出码{供给编译.returncode}): {(供给编译.stderr or 供给编译.stdout).strip()[:200]}"
        if not 供给obj.exists():
            return "失败", f"{编号}-2.5 供给源 {供给名} 未产出 {供给obj.name}"
        供给objs.append(供给obj)

    # ===== 步骤3：运行 v2p（多文件编译：入口 + 自动加载导入模块） =====
    #   入口绝对路径 + cwd=隔离工作目录（111-a）——v2 产物 target/v2asm.asm 落工作目录
    入口参数 = str((审计目录 / f"v2src{编号}" / "主.cn").resolve())
    if v2asm路径.exists():
        v2asm路径.unlink()
    if 详细:
        print(f"    [{编号}-3] {v2p.name} {入口参数}")
    运行结果 = 运行命令([str(v2p), 入口参数], 工作目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", f"{编号}-3 v2p 运行失败(退出码{运行结果.returncode}): {(运行结果.stderr or '').strip()[:300]}"
    if not v2asm路径.exists():
        return "失败", f"{编号}-3 v2p 未生成 target/v2asm.asm"
    # 输出比对：.expected 每行（去空）须为 v2p 实际输出的子串（数值列不参与精确比对）
    期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
    # 入口为绝对路径（cwd 隔离）——日志锚行比对前把绝对前缀适配回相对
    #   （.expected 保持相对路径文本不动；v2 输出路径已归一为正斜杠）
    实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or "")).replace(
        str(项目根目录).replace("\\", "/") + "/", "")
    for 行 in 期望行们:
        if 行 not in 实际输出:
            return "失败", f"{编号}-3 v2p 输出缺少期望行: {行!r}\n    实际: {实际输出[:400]}"
    # 入口符号自检：v2 生成的 asm 必须含 cn_main（对齐宿主，链接后由运行时 entry 调用）
    asm内容 = v2asm路径.read_text(encoding="utf-8", errors="replace")
    if "cn_main PROC" not in asm内容:
        return "失败", f"{编号}-3.5 v2asm.asm 缺少入口符号 cn_main（v2 代码生成入口未对齐宿主）"

    # ===== 步骤3.8：asm size 说明符静态门禁（plans/021 C14，111-a 落地）=====
    #   win 后端固有面：movsd/movss 内存操作数缺 qword/dword ptr → ml64 A2070。
    #   在汇编前扫描产出 asm，把汇编器错误提前为带定位的门禁失败（109-a/110-a
    #   两轮补漏的第三轮防线）。
    扫描失败 = 扫描asm产物说明符(v2asm路径, f"{编号} v2 产物")
    if 扫描失败:
        return "失败", 扫描失败

    # ===== 步骤4：ml64 汇编 target/v2asm.asm -> v2asm.obj =====
    v2obj = 工作目录 / "v2asm.obj"
    if v2obj.exists():
        v2obj.unlink()
    if 详细:
        print(f"    [{编号}-4] ml64 v2asm.asm")
    汇编结果 = 运行命令([str(ML64), "/nologo", "/c", f"/Fo{v2obj}", str(v2asm路径)], 项目根目录)
    if 汇编结果.returncode != 0:
        return "失败", f"{编号}-4 ml64 汇编失败(退出码{汇编结果.returncode}): {汇编结果.stdout.strip()[:300]}"
    if not v2obj.exists():
        return "失败", f"{编号}-4 ml64 返回成功但未生成 v2asm.obj"

    # ===== 步骤5：链接（对齐宿主链接命令 /ENTRY:WinMainCRTStartup + 运行时 obj）=====
    输出exe = 工作目录 / "v2out.exe"
    if 输出exe.exists():
        输出exe.unlink()
    响应文件 = 工作目录 / f"{编号}_link.rsp"
    rsp_lines = [
        "/nologo", "/ENTRY:WinMainCRTStartup", "/SUBSYSTEM:CONSOLE",
        "/STACK:8388608",
    ]
    for lib in LIB路径们:
        rsp_lines.append(f"/LIBPATH:{lib}")
    rsp_lines += ["/DEFAULTLIB:libcmt.lib", "/DEFAULTLIB:libucrt.lib",
                  "/DEFAULTLIB:kernel32.lib", "/DEFAULTLIB:shell32.lib",
                  f"/OUT:{输出exe}"]
    if 链接v2pobj:
        # P7b：v2p.obj 提供 stdlib 容器类方法实现；与 v2asm.obj 的 cn_main 双定义
        #   -> /FORCE:MULTIPLE + v2asm.obj 在前（79 防虚假验收同款：命令行靠前定义胜出）
        if not v2pobj.exists():
            return "失败", "123-5 缺少 target/audit2/v2p.obj（步骤1 cn build 未产出）"
        rsp_lines.append("/FORCE:MULTIPLE")
        # 供给 .obj 置于 v2p.obj 之前（②b B7：用例自有类型布局权威）
        rsp_lines += [str(v2obj)] + [str(o) for o in 供给objs] + [str(v2pobj)] + [str(o) for o in 运行时objs]
    else:
        rsp_lines += [str(v2obj)] + [str(o) for o in 运行时objs]
    # map 文件供符号方向自检（v2asm.obj 必须贡献 cn_main）
    map文件 = 工作目录 / f"{编号}_link.map"
    if map文件.exists():
        map文件.unlink()
    rsp_lines.append(f"/MAP:{map文件}")
    with open(响应文件, "w", encoding="utf-8") as f:
        for 行 in rsp_lines:
            f.write(f'"{行}"\n')
    if 详细:
        print(f"    [{编号}-5] link -> {输出exe.name}")
    链接结果 = 运行命令([str(LINK), f"@{响应文件}"], 项目根目录)
    if 链接结果.returncode != 0:
        return "失败", f"{编号}-5 链接失败(退出码{链接结果.returncode}): {链接结果.stdout.strip()[:300]}"
    if not 输出exe.exists():
        return "失败", f"{编号}-5 链接返回成功但未生成 exe"
    # 符号方向自检（防虚假验收：cn_main 必须来自 v2asm.obj 而非其他 obj）
    map内容 = map文件.read_text(encoding="utf-8", errors="replace") if map文件.exists() else ""
    if "v2asm.obj" not in map内容 or "cn_main" not in map内容:
        return "失败", f"{编号}-5.5 符号自检失败: map 中未找到来自 v2asm.obj 的 cn_main"
    if 链接v2pobj:
        # P7b 双向自检：容器方法符号（向量$... 构造/追加）必须来自 v2p.obj——
        #   否则容器链路退化为其他来源，闭环是假的
        向量行 = [l for l in map内容.splitlines()
                if "E59091E9878F" in l and "v2p.obj" in l and " f " in l]
        if not 向量行:
            return "失败", "123-5.5 符号自检失败: map 中未找到来自 v2p.obj 的向量类方法符号"

    # ===== 步骤6：运行 v2 产物 exe -> 退出码须等于用例预期值 =====
    运行结果2 = 运行命令([str(输出exe)], 项目根目录)
    if 运行结果2.returncode != 预期退出码:
        return "失败", (f"{编号}-6 v2 产物运行退出码={运行结果2.returncode}"
                        f"（期望 {预期退出码}）: {(运行结果2.stderr or '').strip()[:200]}")
    return "通过", f"v2 多文件编译（{名称}）-> ml64/link（对齐宿主链接命令）-> 运行 闭环成立（退出码 {预期退出码}）"


# ============ 双编译对照编排（plans/020 移植纪律 9 的门禁化，2026-09-11 第七十三轮）============
# 目的：把「同结构双编译器对照」从手工手法变成门禁——宿主与 v2 是两套独立实现
#   （v2 为重实现非移植），同一语义机制两侧各自重写，**功能测试各自全绿 ≠ 行为等价**。
# 73-a 实证：v2 块级作用域 RAII 从未生效（预扫登记使块出口基线恒等），而 v2 用例族
#   （208_v2/212_v2 等）全绿——那些用例只断言内容，而泄漏/晚释放不改内容。
# 断言链（三层）：
#   ① 宿主侧退出码 == 用例期望值（宿主语义正确性锚定——防两侧同错反而"对照通过"）
#   ② v2 侧退出码 == 宿主侧实测退出码（**等价性判据**——v2 侧以宿主实测值为期望，
#      复用 执行v2闭环 全链，零重复编排代码）
#   ③ 生成物层信息（两侧 asm 释放调用计数）打印供调试——不作硬断言（两侧 IR 结构
#      不同、计数天然不等；运行级退出码才是等价判据）
# 用例源码自带断言（内存::活动分配数 差值折入退出码校验和），故本编排只判退出码：
#   任一形态的释放机制在一侧失效 → 该侧退出码变化 → ② 失败。
def 执行双编译对照(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
                   输出目录: pathlib.Path, 详细: bool, 目标平台: str,
                   源文件名们: list, 预期退出码: int, 链接v2pobj: bool = False,
                   供给源们: list = None) -> tuple:
    """同一源码两侧（宿主真实管线 / v2 自举链）各编各跑，断言退出码一致。

    实现：宿主侧编译 + 运行取实测退出码 H（断言 H == 用例期望值）→ v2 侧以 H 为期望
    调用 执行v2闭环（其内部比对退出码；不等即失败并回传两侧差值）。
    供给源们（v2 侧的用例自有类型符号供给）语义同 v2 闭环。"""
    import shutil
    if 供给源们 is None:
        供给源们 = []
    名称 = 用例目录.name
    编号 = 名称.split("_")[0]
    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)

    # ===== 步骤1：宿主侧——源码树复制到隔离工作目录（cwd 无关；模块树随拷）=====
    宿主目录 = 审计目录 / f"dualhost{编号}"
    宿主目录.mkdir(parents=True, exist_ok=True)
    for 文件名 in 源文件名们:
        源 = 用例目录 / 文件名
        if not 源.exists():
            return "失败", f"{编号}-D1 缺少用例文件: {文件名}"
        shutil.copy2(源, 宿主目录 / 文件名)
    shutil.copytree(用例目录, 宿主目录, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns("*.expected", "*.input", "*.args",
                                                  "v2闭环.txt", "双编译对照.txt"))
    宿主可执行 = 宿主目录 / ("hostout.exe" if 目标平台 == "win-x64" else "hostout")
    if 宿主可执行.exists():
        宿主可执行.unlink()
    入口 = 宿主目录 / 源文件名们[0]
    if 详细:
        print(f"    [{编号}-D1] 宿主侧 {编译器路径.name} build {入口.name} --target {目标平台}")
    宿主编译 = 运行命令([str(编译器路径), "build", str(入口), "--target", 目标平台,
                     "--output", str(宿主可执行)], 项目根目录)
    if 宿主编译.returncode != 0:
        return "失败", (f"{编号}-D1 宿主侧编译失败(退出码{宿主编译.returncode}): "
                        f"{(宿主编译.stderr or 宿主编译.stdout).strip()[:200]}")
    if not 宿主可执行.exists():
        return "失败", f"{编号}-D1 宿主侧编译返回成功但未生成可执行文件"

    # ===== 步骤1.5：宿主产物 asm 的 size 说明符扫描（plans/021 C14，仅 win 平台）=====
    #   宿主 win 后端产物 asm 落在输出路径旁（stem + .asm）——与 v2 侧同款静态门禁
    #   （宿主侧同族缺陷面；GAS 无 size 说明符要求，仅 win 平台挂）
    if 目标平台 == "win-x64":
        扫描失败 = 扫描asm产物说明符(宿主可执行.with_suffix(".asm"), f"{编号} 宿主侧")
        if 扫描失败:
            return "失败", 扫描失败

    # ===== 步骤2：宿主侧运行取实测退出码 =====
    宿主运行 = 运行命令([str(宿主可执行)], 项目根目录)
    宿主码 = 宿主运行.returncode
    宿主期望 = 预期退出码 % 256
    if 宿主码 != 宿主期望:
        return "失败", (f"{编号}-D2 宿主侧退出码={宿主码}（期望 {宿主期望} = {预期退出码} % 256）"
                        f": {(宿主运行.stderr or '').strip()[:200]}")
    if 详细:
        print(f"    [{编号}-D2] 宿主侧退出码={宿主码}（== 用例期望值，宿主语义锚定通过）")

    # ===== 步骤3：v2 侧——以宿主实测码为期望走既有 v2 闭环（等价性判据）=====
    状态, 说明 = 执行v2闭环(编译器路径, 用例目录, 输出目录, 详细, 目标平台,
                          源文件名们, 宿主码, 链接v2pobj, 供给源们)
    if 状态 != "通过":
        return "失败", (f"{编号}-D3 双编译对照失败：宿主侧退出码={宿主码}，"
                        f"v2 侧未达同值 → {说明}")
    返回说明 = (f"双编译对照成立（同一源码两侧等效）：宿主 {宿主码} == v2 {宿主码}"
                f"（{目标平台}；用例期望 {宿主期望} 由宿主侧锚定）")
    return "通过", 返回说明


def 打印汇总(总数: int, 通过数: int, 失败数: int, 未实现数: int, 跳过数: int = 0) -> None:
    """打印E2E结果汇总与颜色标记"""
    print(粗体("E2E结果汇总"))
    print(f"  总计: {总数}  通过: {绿色(str(通过数))}  "
          f"失败: {红色(str(失败数))}  未实现: {黄色(str(未实现数))}  "
          f"跳过: {青色(str(跳过数))}")


def 打印详情(失败列表: list, 未实现列表: list, 详细: bool) -> None:
    """打印失败与未实现用例详情"""
    if 失败列表:
        print(f"\n{红色('失败用例:')}")
        for 名称, 原因 in 失败列表:
            print(f"  {红色('✗')} {名称}: {原因}")
    if 未实现列表 and 详细:
        print(f"\n{黄色('未实现用例（编译器尚未实现，阶段零预期）:')}")
        for 名称, 原因 in 未实现列表:
            print(f"  {黄色('!')} {名称}: {原因}")

# ============ 入口 ============


def 主程序() -> int:
    # 重负载用例内存保护上限：CLI --max-mem-mb 可覆盖模块默认（78/79 超限自动终止）
    global 内存上限MB默认
    解析器 = argparse.ArgumentParser(
        description="CN语言E2E测试运行器：编译->链接->运行->比对输出",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n"
               "  python3 run_e2e.py\n"
               "  python3 run_e2e.py --cn target/Debug/cn --verbose\n"
               "  python3 run_e2e.py --target linux-arm64 --cn target/Debug/cn\n"
               "  python3 run_e2e.py --target linux-x86_64 --cn target/cn\n"
               "  python3 run_e2e.py --filter 01_hello")
    解析器.add_argument("--cn", help="cn编译器路径（默认自动探测 target/Debug 等）")
    解析器.add_argument("--target", default=None,
                        help="目标平台（win-x64 | linux-arm64 | linux-x86_64；默认按本机平台自动推断）")
    解析器.add_argument("--max-mem-mb", type=int, default=内存上限MB默认,
                        help=f"重负载用例（78/79）运行子进程内存上限MB，超过自动终止（默认 {内存上限MB默认}MB；0=不启用）")
    解析器.add_argument("--verbose", "-v", action="store_true", help="详细输出（显示编译/运行命令）")
    解析器.add_argument("--filter", help="仅运行目录名包含指定模式的用例")
    解析器.add_argument("--target-dir", default="target", help="可执行文件输出目录（默认 target）")
    解析器.add_argument("--strict", action="store_true",
                        help="将'未实现'用例视为失败（阶段一完成后全量验证用）")
    解析器.add_argument("--jobs", "-j", type=int, default=1,
                        help="并行任务数（默认 1=串行原行为；>1 时非 v2 用例并行执行、"
                             "v2 用例保持串行——v2 用例共享 target/v2asm.s 与 v2p 构建缓存。"
                             "v2p 构建缓存在任何模式下生效：v2 源码与编译器未变不重建）")
    参数 = 解析器.parse_args()

    # 覆盖模块级默认（超限自动终止的防护阈值）
    if 参数.max_mem_mb is not None and 参数.max_mem_mb >= 0:
        内存上限MB默认 = 参数.max_mem_mb

    # 目标平台：显式指定优先；否则按本机平台自动推断
    #   （Windows -> win-x64；Linux ARM64 -> linux-arm64；Linux x86_64 -> linux-x86_64，
    #    plans/016 起原生支持本机闭环）
    目标平台 = 参数.target
    if 目标平台 is None:
        if sys.platform == "win32":
            目标平台 = "win-x64"
        elif platform.machine() in ("x86_64", "AMD64", "amd64"):
            目标平台 = "linux-x86_64"
        else:
            目标平台 = "linux-arm64"
    if 目标平台 not in ("win-x64", "linux-arm64", "linux-x86_64"):
        print(红色(f"错误: 无效目标平台 {目标平台}（应为 win-x64、linux-arm64 或 linux-x86_64）"))
        return 2

    # 探测编译器与输出目录
    编译器路径 = 探测编译器(参数.cn)
    输出目录 = (项目根目录 / 参数.target_dir).resolve()
    输出目录.mkdir(parents=True, exist_ok=True)

    print(粗体("CN语言 E2E 测试运行器"))
    print(f"  编译器: {青色(str(编译器路径))}")
    print(f"  目标平台: {青色(目标平台)}")
    print(f"  输出目录: {输出目录}")

    # 门禁自身自检（探测器也是被测对象——110-a 教训）：asm size 说明符扫描器
    #   正/负样本内联自检——扫描器失效时先于被测物报错，防"门禁恒绿"假象
    扫描器问题 = asm说明符扫描器自检()
    if 扫描器问题:
        print(红色(f"错误: asm size 说明符扫描器自检失败（{扫描器问题}）——门禁不可信，中止"))
        return 2
    print()

    # 收集用例
    用例目录们 = 收集用例(参数.filter)
    if not 用例目录们:
        print(黄色("未找到E2E用例目录（过滤条件无匹配或无用例）"))
        return 1

    # 逐个执行并统计（--jobs>1 并行模式，2026-09-11 用户裁决「E2E 太慢」提速：
    #   非 v2 用例并行池 + v2 用例单线程池**并发**执行——v2 用例共享 target/v2asm.s
    #   固定产物路径与 v2p 构建缓存，必须串行；宿主用例产物按用例目录名隔离
    #   （target/<用例名>）可安全并行；对齐 cargo test / cargo-nextest 进程隔离
    #   并行理念——测试链路本身零变化，仅并发调度。任务内详细 print 以行粒度
    #   交错为可接受代价（与 cargo 并行测试输出一致），结果行经打印锁整块输出）
    通过数 = 0
    跳过数 = 0
    失败列表 = []
    未实现列表 = []

    def 记录结果(用例目录, 状态, 原因, 即时打印: bool) -> None:
        nonlocal 通过数, 跳过数
        if 状态 == "通过":
            通过数 += 1
            if 即时打印:
                print(f"  {绿色('PASS')} 通过")
        elif 状态 == "跳过":
            跳过数 += 1
            if 即时打印:
                print(f"  {青色('SKIP')} {原因}")
        elif 状态 == "未实现":
            未实现列表.append((用例目录.name, 原因))
            if 即时打印:
                print(f"  {黄色('SKIP')} 未实现: {原因}")
        else:
            失败列表.append((用例目录.name, 原因))
            if 即时打印:
                print(f"  {红色('FAIL')} {原因}")

    if 参数.jobs <= 1:
        for 用例目录 in 用例目录们:
            print(f"运行用例: {用例目录.name}")
            状态, 原因 = 执行单个用例(编译器路径, 用例目录, 输出目录, 参数.verbose, 目标平台)
            记录结果(用例目录, 状态, 原因, 即时打印=True)
            print()
    else:
        from concurrent.futures import ThreadPoolExecutor, as_completed
        打印锁 = threading.Lock()

        def 并行任务(用例目录):
            状态, 原因 = 执行单个用例(编译器路径, 用例目录, 输出目录, 参数.verbose, 目标平台)
            with 打印锁:
                print(f"运行用例: {用例目录.name}")
                记录结果(用例目录, 状态, 原因, 即时打印=True)
                print()
            return 状态

        # 全量统一并行（2026-09-11 用户裁决：01 起全部用例一个池）——三平台同款：
        #   v2 产物已按用例隔离（v2work<编号> workdir / v2asm.obj / v2out.exe），
        #   唯一共享工件 v2p/运行时 .o 在池启动前预热（确保v2p与运行时就绪 /
        #   确保v2p就绪win——消除并发构建竞态；预热本身数秒级，缓存命中时瞬时）。
        #   win-x64 自 111-a 起同款（此前 v2 产物共享 target/v2asm.asm → v2 桶串行）。
        池们 = []
        if 目标平台 in ("linux-arm64", "linux-x86_64"):
            v2用例们 = [d for d in 用例目录们 if 是v2闭环用例(d)]
            if v2用例们:
                print(青色(f"预热: v2p 构建缓存（{len(v2用例们)} 个 v2 用例共享工件）..."))
                就绪 = 确保v2p与运行时就绪(编译器路径, 目标平台, 参数.verbose)
                if 就绪[0] is None:
                    print(红色(f"预热失败: {就绪[1]}"))
                    return 1
            print(青色(f"并行模式: jobs={参数.jobs}（{len(用例目录们)} 个用例统一并行）"))
            print()
            池们.append(("统一", ThreadPoolExecutor(max_workers=参数.jobs),
                         [(d, "统一") for d in 用例目录们]))
        else:
            v2用例们 = [d for d in 用例目录们 if 是v2闭环用例(d)]
            if v2用例们:
                print(青色(f"预热: v2p 构建缓存（{len(v2用例们)} 个 v2 用例共享工件）..."))
                就绪 = 确保v2p就绪win(编译器路径, 参数.verbose)
                if 就绪[0] is None:
                    print(红色(f"预热失败: {就绪[1]}"))
                    return 1
            print(青色(f"并行模式: jobs={参数.jobs}（{len(用例目录们)} 个用例统一并行）"))
            print()
            池们.append(("统一", ThreadPoolExecutor(max_workers=参数.jobs),
                         [(d, "统一") for d in 用例目录们]))

        futures = []
        with 池们[0][1] as _池0:
            futures += [_池0.submit(并行任务, d) for d, _ in 池们[0][2]]
            if len(池们) > 1:
                with 池们[1][1] as _池1:
                    futures += [_池1.submit(并行任务, d) for d, _ in 池们[1][2]]
                    for fu in as_completed(futures):
                        fu.result()
            else:
                for fu in as_completed(futures):
                    fu.result()

    # 汇总与退出码：有真实失败返回1；strict模式下未实现也算失败
    总数 = len(用例目录们)
    未实现数 = len(未实现列表)
    失败数 = len(失败列表)
    打印汇总(总数, 通过数, 失败数, 未实现数, 跳过数)
    打印详情(失败列表, 未实现列表, 参数.verbose)

    if 失败数 > 0:
        return 1
    if 参数.strict and 未实现数 > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(主程序())
