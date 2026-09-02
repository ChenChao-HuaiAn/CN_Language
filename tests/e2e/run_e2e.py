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
import os
import pathlib
import signal
import subprocess
import sys
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

# 运行子进程内存保护（2026-08-24 用户要求）：
# 79_bootstrap_closed_loop 等大规模编译用例运行时，子进程（如 CN 组件链编译器）
# 内存可能失控（实测 26GB+ 卡死）。超过 内存上限MB 的进程将被自动终止并判为失败。
# 单位：MB。0 = 不启用（默认仅对 78/79 等重负载用例启用，避免小用例轮询开销）。
内存上限MB默认 = 4096
内存保护用例前缀 = ("78_chain_build", "79_bootstrap_closed_loop")
内存轮询间隔秒 = 0.5

# 平台限制用例跳过列表：某些用例因平台特性差异（API/ABI/工具链）无法在特定平台运行
# 键 = 目标平台，值 = 用例目录名前缀列表（不含编号前缀的短名匹配）
平台跳过 = {
    "linux-arm64": [
        "62_ffi",                # 依赖 Windows API GetTickCount64
        "65_string_index",       # ARM64 char 默认为 unsigned char，符号扩展差异
        "69_memory_management",  # 运行时初始化计数在 Linux 上行为不同
        "79_bootstrap_closed_loop",  # 依赖 ml64/link MSVC 工具链
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
             标准输入: str = "", 内存上限MB: int = 0) -> subprocess.CompletedProcess:
    """执行命令并返回结果（捕获stdout/stderr，UTF-8解码容错）

    标准输入: 可选 stdin 注入字符串（Task 6.2 IO 输入用例用，默认空）
    内存上限MB: >0 时启用内存保护——每 内存轮询间隔秒 轮询子进程工作集，
      超过上限立即 终止进程树 并以退出码 -9（returncode）标记失败，
      stderr 给出"内存超限"原因。防 79 等大规模编译用例内存失控卡死机器
      （2026-08-24 实测：79_bootstrap_closed_loop.exe 工作集涨到 26GB+）。
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
    return subprocess.run(
        命令列表, cwd=str(工作目录), capture_output=True,
        text=True, encoding="utf-8", errors="replace",
        input=标准输入, preexec_fn=preexec_fn)


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


def 解码诊断(原始字节: bytes) -> str:
    """解码编译器诊断：cn.exe 的 stderr 为 UTF-16LE（FF FE BOM），stdout 为 UTF-8"""
    if not 原始字节:
        return ""
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
              "--target", 目标平台, "--output", str(输出可执行)]
    if 详细:
        print(f"    [负编译] {' '.join(编译命令)}")
    # 字节模式采集：cn.exe stderr 为 UTF-16LE，须按字节解码诊断
    编译结果 = subprocess.run(编译命令, cwd=str(项目根目录), capture_output=True)
    if 编译结果.returncode == 0:
        return "失败", f"预期编译失败但编译成功: {源文件.name}"
    输出 = 解码诊断(编译结果.stderr) + 解码诊断(编译结果.stdout)
    if 期望子串 and 期望子串 not in 输出:
        return "失败", f"编译如预期失败但缺期望诊断[{期望子串}]，实际输出: {输出.strip()[:160]}"
    return "通过", "负测试通过（预期编译失败）"


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

    try:
        源文件 = 查找源文件(用例目录)
        期望文件 = 查找期望文件(源文件)
    except FileNotFoundError as 异常:
        return "失败", str(异常)

    # ============ 79 自举闭环用例：特殊编排 ============
    # 流程：编译79主.cn -> 运行落盘5个链.asm(第一次) -> ml64汇编5个.obj
    #       -> 链接(79入口obj提供cn_main+向量方法, 链.obj提供CN组件函数)
    #       -> 运行CN版编译器exe再次落盘(第二次) -> 比对两次产物一致
    # 仅 win-x64 平台支持（依赖 ml64/link 与运行时 .obj）
    if 名称 == "79_bootstrap_closed_loop":
        if 目标平台 != "win-x64":
            return "失败", "79闭环用例仅支持 win-x64（依赖 ml64/link）"
        return 执行79闭环(编译器路径, 用例目录, 输出目录, 详细)

    # ============ v2 自举链接闭环用例（119/120…）：特殊编排 ============
    # v1 79 闭环是 C++ 版产物 + CN 组件链；v2 系列闭环是 v2 重建产物 + 宿主运行时：
    #   宿主编译 v2 组件（入口 主.cn）-> v2p.exe -> 编译多文件程序
    #   （入口 主.cn + 导入模块，P6h 多源文件支持）-> v2asm.asm
    #   -> ml64/link（对齐宿主链接命令：/ENTRY:WinMainCRTStartup + 运行时 .obj）
    #   -> 运行 v2 产物 exe，退出码须等于用例预期值（v2 代码真实执行验证）
    # 阶段A（2026-09-02）双平台：win-x64 走 ml64/link；linux-arm64 走 as/g++
    #   （v2 新增 GAS 后端，v2p 第 2 参数 目标平台 分派；链接对齐宿主 linux 命令）
    # 元组第三元素（可缺省）= 是否链接 v2p.obj（P7b 容器用例：v2 生成代码调用
    #   宿主编译的容器类方法符号，实现在 v2p.obj——stdlib 源码级并入编译产物）
    # 元组第四元素（可缺省）= 供给源列表（②b B7，2026-09-02）：用例目录下的 .cn
    #   经宿主真实管线编译为 .o 参与链接且置于 v2p.obj 之前——v2p 只实例化过
    #   v2 自身用到的类型（如 126 的 词条），用例自有类型须供给符号与布局权威
    v2闭环用例们 = {
        "119_v2_多文件链接闭环": (["主.cn", "计算.cn"], 14),   # 加倍(7)
        "120_v2_顶层常量": (["主.cn", "常量库.cn"], 62),       # 常量和() + 系数*增量 = 38+24
        "123_v2_容器": (["主.cn"], 21333, True),              # P7b：向量/映射/结果/字符串/RAII = 21333
        "125_v2_控制流与短路与转义": (["主.cn"], 0, True),    # 三缺陷根治：中途回退/短路/转义 = 0
        "126_v2_结构体元素容器": (["主.cn"], 0, True, ["供给.cn"]),  # ②b：布局/视图/传参/深拷贝析构 = 0
    }
    if 名称 in v2闭环用例们:
        if 目标平台 != "win-x64" and 目标平台 != "linux-arm64":
            return "失败", f"{名称} 闭环用例仅支持 win-x64 / linux-arm64（当前 {目标平台}）"
        条目 = v2闭环用例们[名称]
        源文件名们, 预期退出码 = 条目[0], 条目[1]
        链接v2pobj = 条目[2] if len(条目) > 2 else False
        供给源们 = 条目[3] if len(条目) > 3 else []
        return 执行v2闭环(编译器路径, 用例目录, 输出目录, 详细, 目标平台, 源文件名们, 预期退出码,
                        链接v2pobj, 供给源们)

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
    编译结果 = 运行命令([str(编译器路径), "build", str(源文件),
                      "--target", 目标平台,
                      "--output", str(输出可执行)], 项目根目录)
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


# ============ 79 自举闭环用例编排 ============
# 完整闭环：C++版编译79主.cn -> 运行落盘5个链.asm(第一次,CN组件链产物)
#   -> ml64汇编5个.obj -> 链接(79入口obj提供cn_main+向量方法, 链.obj提供CN组件函数)
#   -> 运行CN版编译器exe再次落盘(第二次) -> 比对两次产物一致 => 自举闭环成立
# 产物全部落 target/audit2/（规则19），不污染工作区源码目录


def 执行79闭环(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
               输出目录: pathlib.Path, 详细: bool) -> tuple:
    """执行 79 自举闭环用例：编译->落盘->汇编->链接->再落盘->比对"""
    名称 = "79_bootstrap_closed_loop"
    源文件 = 查找源文件(用例目录)
    期望文件 = 查找期望文件(源文件)

    # 工作目录：target/audit2/（全部产物落此，规则19）
    审计目录 = 项目根目录 / "target" / "audit2"
    审计目录.mkdir(parents=True, exist_ok=True)

    # 工具链绝对路径（VS 2022）
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
        return "失败", "未找到 ml64/link（VS 2022 MSVC 工具链）"

    # LIB 路径（MSVC + Windows Kits）--MSVC lib 随已探测版本，Windows Kits 用已装最新版
    # ML64 位于 <MSVC根>\<版本>\bin\Hostx64\x64\ml64.exe -> lib 在 <MSVC根>\<版本>\lib\x64
    msvc版本目录 = ML64.parent.parent.parent.parent
    kits根 = pathlib.Path(r"C:\Program Files (x86)\Windows Kits\10\lib")
    kits版 = sorted((p for p in kits根.glob("10.*") if p.is_dir()), reverse=True) if kits根.exists() else []
    if not kits版:
        return "失败", f"未找到 Windows Kits lib 目录: {kits根}\\10.*（请确认 Win10 SDK 安装）"
    LIB路径们 = [
        str(msvc版本目录 / "lib" / "x64"),
        str(kits版[0] / "ucrt" / "x64"),
        str(kits版[0] / "um" / "x64"),
    ]
    for lib路径 in LIB路径们:
        if not pathlib.Path(lib路径).exists():
            return "失败", f"LIB 路径不存在: {lib路径}"

    # 运行时 .obj（C++ 版构建产物，target/ 下）
    运行时名们 = ["io_api", "runtime", "string_api", "i128_api", "math_api",
                "input_api", "file_api", "time_api", "system_api"]
    运行时objs = [项目根目录 / "target" / f"{m}.obj" for m in 运行时名们]
    for obj in 运行时objs:
        if not obj.exists():
            return "失败", f"缺少运行时 .obj: {obj.name}（请先构建 C++ 版编译器）"

    # 模块名 -> ASCII 名（避免 link 响应文件中文路径在 GBK 代码页下乱码）
    模块们 = [("词法分析", "lexer"), ("语法分析", "parser"), ("语义分析", "semantic"),
            ("IR生成", "irgen"), ("代码生成", "codegen")]

    # ===== 步骤1：C++版编译 79 主.cn -> 79_bootstrap_closed_loop.exe =====
    输出可执行 = 输出目录 / f"{名称}.exe"
    if 输出可执行.exists():
        输出可执行.unlink()
    if 详细:
        print(f"    [79-1] {编译器路径} build {源文件.name} --output {输出可执行}")
    编译结果 = 运行命令([str(编译器路径), "build", str(源文件),
                      "--target", "win-x64", "--output", str(输出可执行)],
                     项目根目录)
    if 编译结果.returncode != 0:
        return "失败", f"79-1 编译失败(退出码{编译结果.returncode}): {(编译结果.stderr or 编译结果.stdout).strip()[:200]}"
    if not 输出可执行.exists():
        return "失败", "79-1 编译返回成功但未生成可执行文件"

    # ===== 步骤2：运行 -> 落盘 5 个 *_链.asm（第一次，CN 组件链产物） =====
    # 内存保护（2026-08-24 用户要求）：组件链编译器内存可能失控（实测 26GB+），
    # 超过 内存上限MB默认 立即自动终止并判失败
    运行结果 = 运行命令([str(输出可执行)], 项目根目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", f"79-2 运行失败(退出码{运行结果.returncode}): {运行结果.stderr.strip()[:200]}"
    # 校验 5 个 .asm 已落盘
    for 模块, _ in 模块们:
        asm = 审计目录 / f"{模块}_链.asm"
        if not asm.exists():
            return "失败", f"79-2 未生成 {asm.name}"
    # 备份第一次产物
    第一次目录 = 审计目录 / "79_第一次"
    第一次目录.mkdir(parents=True, exist_ok=True)
    for 模块, _ in 模块们:
        import shutil
        shutil.copy2(审计目录 / f"{模块}_链.asm", 第一次目录 / f"{模块}_链.asm")

    # ===== 步骤3：ml64 汇编 5 个 .asm -> 5 个 .obj =====
    for 模块, ascii名 in 模块们:
        asm = 审计目录 / f"{模块}_链.asm"
        obj = 审计目录 / f"{ascii名}_chain.obj"
        if 详细:
            print(f"    [79-3] ml64 {asm.name}")
        汇编结果 = 运行命令([str(ML64), "/nologo", "/c", f"/Fo{obj}", str(asm)],
                         项目根目录)
        if 汇编结果.returncode != 0:
            return "失败", f"79-3 ml64 汇编 {模块} 失败(退出码{汇编结果.returncode}): {汇编结果.stdout.strip()[:200]}"

    # ===== 步骤4：链接 -> CN 版编译器 exe =====
    # 【防虚假验收关键】链接顺序必须是：链.obj（CN组件自编译产物）在前，
    # 入口obj（C++版 cn build 产物，其中内联了同名组件函数）在后。
    # /FORCE:MULTIPLE 下 MSVC link 保留命令行靠前的第一个定义--
    # 若入口obj在前，组件符号全部绑定C++版内联实现，链.obj被LNK4006整体忽略，
    # 第二次落盘实际由C++版组件执行，「CN vs CN」固定点沦为「C++ vs C++」假验证。
    入口obj = 输出目录 / f"{名称}.obj"
    if not 入口obj.exists():
        return "失败", f"79-4 缺少入口 obj: {入口obj.name}（cn build 未产出 .obj）"
    链objs = [审计目录 / f"{ascii名}_chain.obj" for _, ascii名 in 模块们]
    输出exe = 审计目录 / "cn_compiler_self.exe"
    # 响应文件（避免中文路径在 GBK 代码页下乱码）
    响应文件 = 审计目录 / "79_link.rsp"
    rsp_lines = [
        "/nologo", "/ENTRY:WinMainCRTStartup", "/SUBSYSTEM:CONSOLE",
        "/STACK:8388608", "/FORCE:MULTIPLE",
    ]
    for lib in LIB路径们:
        rsp_lines.append(f"/LIBPATH:{lib}")
    rsp_lines += ["/DEFAULTLIB:libcmt.lib", "/DEFAULTLIB:libucrt.lib",
                  "/DEFAULTLIB:kernel32.lib", "/DEFAULTLIB:shell32.lib",
                  f"/OUT:{输出exe}"]
    # 链.obj 必须排在入口obj之前（见上方防虚假验收说明）
    rsp_lines += [str(o) for o in 链objs] + [str(入口obj)] + [str(o) for o in 运行时objs]
    # 生成 map 文件供符号保留方向自检（防组件符号被C++版内联定义覆盖）
    map文件 = 审计目录 / "79_link.map"
    if map文件.exists():
        map文件.unlink()
    rsp_lines.append(f"/MAP:{map文件}")
    with open(响应文件, "w", encoding="utf-8") as f:
        for 行 in rsp_lines:
            f.write(f'"{行}"\n')
    if 详细:
        print(f"    [79-4] link -> {输出exe.name}")
    链接结果 = 运行命令([str(LINK), f"@{响应文件}"], 项目根目录)
    if 链接结果.returncode != 0:
        return "失败", f"79-4 链接失败(退出码{链接结果.returncode}): {链接结果.stdout.strip()[:300]}"
    if not 输出exe.exists():
        return "失败", "79-4 链接返回成功但未生成 exe"

    # ===== 步骤4.5：符号保留方向自检（防虚假验收） =====
    # /MAP 产物中：每个链.obj 必须实际贡献符号（组件函数绑定CN自编译版本），
    # cn_main 必须来自入口obj；否则闭环退化为C++版自演（入口obj内联组件覆盖链.obj）
    map内容 = map文件.read_text(encoding="utf-8", errors="replace") if map文件.exists() else ""
    for ascii名 in [a for _, a in 模块们]:
        链obj名 = f"{ascii名}_chain.obj"
        if 链obj名 not in map内容:
            return "失败", (f"79-4.5 符号自检失败: map 中 {链obj名} 未贡献任何符号"
                            "（组件符号被入口obj的C++内联版覆盖，闭环是假的）")
    if "cn_main" not in map内容 or 入口obj.name not in map内容:
        return "失败", f"79-4.5 符号自检失败: map 中未找到来自 {入口obj.name} 的 cn_main"

    # ===== 步骤5：运行 CN 版编译器 exe -> 再次落盘（第二次） =====
    # 【防假通过】先删除 5 个旧的 *_链.asm：若第二次运行未真正写文件，
    # 步骤6 的存在性检查与比对将用第一次的旧产物蒙混过关
    # （写入虽是截断重写，但删旧文件可确保「存在=第二次真的写了」）
    for 模块, _ in 模块们:
        旧asm = 审计目录 / f"{模块}_链.asm"
        if 旧asm.exists():
            旧asm.unlink()
    # CN 版编译器同样启用内存保护（与步骤2一致）
    运行结果2 = 运行命令([str(输出exe)], 项目根目录, 内存上限MB=内存上限MB默认)
    if 运行结果2.returncode != 0:
        return "失败", f"79-5 CN版编译器运行失败(退出码{运行结果2.returncode}): {运行结果2.stderr.strip()[:200]}"

    # ===== 步骤6：比对两次产物一致（自举固定点） =====
    for 模块, _ in 模块们:
        第一次 = 第一次目录 / f"{模块}_链.asm"
        第二次 = 审计目录 / f"{模块}_链.asm"
        if not 第二次.exists():
            return "失败", f"79-6 第二次未生成 {第二次.name}"
        内容1 = 第一次.read_bytes()
        内容2 = 第二次.read_bytes()
        if 内容1 != 内容2:
            return "失败", (f"79-6 自举固定点不一致: {模块} "
                            f"(第一次{len(内容1)}字节 vs 第二次{len(内容2)}字节)")

    # ===== 步骤7：比对运行输出与期望 =====
    期望 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines()]
    实际 = [行.rstrip() for 行 in 运行结果2.stdout.splitlines()]
    if 实际 != 期望:
        return "失败", f"79-7 输出不一致\n    期望: {期望}\n    实际: {实际}"

    # 判据说明：第一次落盘=C++版编译产物运行结果，第二次落盘=CN自编译版组件运行结果，
    # 两者逐字节一致 = 「CN版编译器再次编译自身源码 -> 产物行为一致」（阶段7验收步骤2）。
    # 这同时蕴含固定点：CN版自编译两次产物也必然一致（同为CN版组件行为）。
    return "通过", "自举闭环成立：CN自编译版组件产物与C++版逐字节一致（阶段7验收步骤2：产物行为一致）"


def 执行v2闭环Linux(编译器路径: pathlib.Path, 详细: bool,
                   源文件名们: list, 预期退出码: int, 链接v2pobj: bool,
                   期望文件, 审计目录: pathlib.Path, v2源码目录: pathlib.Path,
                   用例目录: pathlib.Path, 名称: str, 编号: str,
                   供给源们: list = None) -> tuple:
    """执行 v2 自举链接闭环（linux-arm64，阶段A 2026-09-02）：v2 GAS 后端 -> as -> g++ -> 运行

    链接对齐宿主 linux 命令（cn_main.cpp）：g++ -no-pie + 运行时 .o（-DCNRT_LINUX_MAIN）；
    链接 v2p_linux.o 时 cn_main 双定义 -> -Wl,-z,muldefs + v2asm.o 命令行在前
    （靠前定义胜出——对齐 win64 /FORCE:MULTIPLE 编排）。
    供给源们（②b B7）：用例自有类型符号供给——宿主 cn build 编译（.o 为构建副
    产品留存），链接置于 v2p_linux.o 之前（用例结构体布局权威）。
    POSIX 退出码 8 位截断：退出码比对取 预期退出码 % 256（win64 为 32 位全值）。"""
    import shutil
    import os
    if 供给源们 is None:
        供给源们 = []

    # 工具链探测（对齐宿主 cn_main.cpp：CN_AS/CN_CXX 环境变量优先，PATH，便携 gcc7 兜底）
    as工具 = os.environ.get("CN_AS") or shutil.which("as") or "/home/user/gcc7/usr/bin/as"
    cxx工具 = os.environ.get("CN_CXX") or shutil.which("g++") or "/home/user/gcc7/usr/bin/g++"
    if shutil.which(as工具) is None and not pathlib.Path(as工具).exists():
        return "失败", f"未找到 as 汇编器（{as工具}）"
    if shutil.which(cxx工具) is None and not pathlib.Path(cxx工具).exists():
        return "失败", f"未找到 g++ 链接器（{cxx工具}）"

    v2p = 审计目录 / "v2p_linux"
    v2pobj = 审计目录 / "v2p_linux.o"
    v2asm路径 = 项目根目录 / "target" / "v2asm.s"

    # 运行时 .o（对齐宿主编译命令 g++ -c -std=c++17 -fno-exceptions -fno-rtti
    #   -DCNRT_LINUX_MAIN；缺则现编——宿主 cn build 缓存可能被清，此处保证自包含）
    运行时名们 = ["io_api", "intern_api", "runtime", "string_api", "i128_api",
                "math_api", "input_api", "file_api", "time_api", "system_api"]
    运行时objs = []
    for 模块 in 运行时名们:
        obj = 审计目录 / f"{模块}.o"
        if not obj.exists():
            编译rt = 运行命令([cxx工具, "-c", "-std=c++17", "-fno-exceptions", "-fno-rtti",
                            "-DCNRT_LINUX_MAIN", "-Isrc", "-o", str(obj),
                            f"src/runtime/{模块}.cpp"], 项目根目录)
            if 编译rt.returncode != 0:
                return "失败", f"{编号}-0 运行时 {模块}.o 编译失败: {(编译rt.stderr or 编译rt.stdout).strip()[:200]}"
        运行时objs.append(obj)

    # ===== 步骤1：宿主编译 v2 组件（linux-arm64）-> v2p_linux（中间 .o 留存供链接）=====
    if v2p.exists():
        v2p.unlink()
    if v2pobj.exists():
        v2pobj.unlink()
    if 详细:
        print(f"    [{编号}-1] {编译器路径} build 主.cn -> v2p_linux（linux-arm64）")
    编译结果 = 运行命令([str(编译器路径), "build", str(v2源码目录 / "主.cn"),
                      "--target", "linux-arm64", "--output", str(v2p)], 项目根目录)
    if 编译结果.returncode != 0:
        return "失败", f"{编号}-1 编译 v2 组件失败(退出码{编译结果.returncode}): {(编译结果.stderr or 编译结果.stdout).strip()[:200]}"
    if not v2p.exists():
        return "失败", f"{编号}-1 编译返回成功但未生成 v2p_linux"
    if not v2pobj.exists():
        return "失败", f"{编号}-1 中间产物 v2p_linux.o 未留存（容器符号提供者）"

    # ===== 步骤1.5：供给源编译（②b B7）——用例自有类型符号，宿主真实管线 =====
    #   注意：宿主模块系统按规范08-四 仅以 主.cn 为入口模块（其余按导入模块合并
    #   公开声明）——供给源须拷贝为独立目录的 主.cn 再编译，函数才完整发射。
    供给objs = []
    for 供给名 in 供给源们:
        供给src = 用例目录 / 供给名
        if not 供给src.exists():
            return "失败", f"{编号}-1.5 缺少供给源文件: {供给名}"
        供给目录 = 审计目录 / f"供给{编号}_{pathlib.Path(供给名).stem}"
        供给目录.mkdir(parents=True, exist_ok=True)
        shutil.copy2(供给src, 供给目录 / "主.cn")
        供给输出 = 供给目录 / "供给"
        供给obj = 供给目录 / "供给.o"
        if 供给输出.exists():
            供给输出.unlink()
        if 供给obj.exists():
            供给obj.unlink()
        if 详细:
            print(f"    [{编号}-1.5] {编译器路径} build {供给名} -> {供给obj.name}（符号供给）")
        供给编译 = 运行命令([str(编译器路径), "build", str(供给目录 / "主.cn"),
                          "--target", "linux-arm64", "--output", str(供给输出)], 项目根目录)
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

    # ===== 步骤3：运行 v2p（第 2 参数 linux-arm64 分派 GAS 后端）=====
    入口参数 = f"target/audit2/v2src{编号}/主.cn"
    if v2asm路径.exists():
        v2asm路径.unlink()
    if 详细:
        print(f"    [{编号}-3] {v2p.name} {入口参数} linux-arm64")
    运行结果 = 运行命令([str(v2p), 入口参数, "linux-arm64"], 项目根目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", f"{编号}-3 v2p 运行失败(退出码{运行结果.returncode}): {(运行结果.stderr or '').strip()[:300]}"
    if not v2asm路径.exists():
        return "失败", f"{编号}-3 v2p 未生成 target/v2asm.s"
    期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
    实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or ""))
    for 行 in 期望行们:
        # 平台适配（不改测试文件）：GAS 后端输出 target/v2asm.s（win64 期望为 .asm）
        适配行 = 行.replace("target/v2asm.asm", "target/v2asm.s")
        if 适配行 not in 实际输出:
            return "失败", f"{编号}-3 v2p 输出缺少期望行: {适配行!r}\n    实际: {实际输出[:400]}"
    asm内容 = v2asm路径.read_text(encoding="utf-8", errors="replace")
    if ".globl cn_main" not in asm内容:
        return "失败", f"{编号}-3.5 v2asm.s 缺少入口符号 cn_main（v2 代码生成入口未对齐宿主）"

    # ===== 步骤4：as 汇编 target/v2asm.s -> v2asm_linux.o =====
    v2obj = 审计目录 / "v2asm_linux.o"
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
    输出exe = 审计目录 / "v2out_linux"
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
    return "通过", f"v2 多文件编译（{名称}）-> as/g++（对齐宿主 linux 链接命令）-> 运行 闭环成立（退出码 {预期值}）"


def 执行v2闭环(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
               输出目录: pathlib.Path, 详细: bool, 目标平台: str,
               源文件名们: list, 预期退出码: int, 链接v2pobj: bool = False,
               供给源们: list = None) -> tuple:
    """执行 v2 自举链接闭环（119/120… 通用，双平台）：v2 多文件编译 -> 链接宿主运行时 -> 运行

    目标平台 = win-x64（ml64/link 原路径）| linux-arm64（as/g++，阶段A 2026-09-02）；
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

    # ---- linux-arm64 分支（阶段A：v2 GAS 后端 + as/g++ 编排）----
    if 目标平台 == "linux-arm64":
        return 执行v2闭环Linux(编译器路径, 详细, 源文件名们, 预期退出码, 链接v2pobj,
                              期望文件, 审计目录, v2源码目录, 用例目录, 名称, 编号, 供给源们)

    # ---- win-x64 原路径（ml64/link，P6h/P7b 既有编排不变）----
    v2p = 审计目录 / "v2p.exe"
    v2asm路径 = 项目根目录 / "target" / "v2asm.asm"

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
        return "失败", "未找到 ml64/link（VS 2022 MSVC 工具链）"

    # LIB 路径（MSVC + Windows Kits）
    msvc版本目录 = ML64.parent.parent.parent.parent
    kits根 = pathlib.Path(r"C:\Program Files (x86)\Windows Kits\10\lib")
    kits版 = sorted((p for p in kits根.glob("10.*") if p.is_dir()), reverse=True) if kits根.exists() else []
    if not kits版:
        return "失败", f"未找到 Windows Kits lib 目录: {kits根}\\10.*（请确认 Win10 SDK 安装）"
    LIB路径们 = [
        str(msvc版本目录 / "lib" / "x64"),
        str(kits版[0] / "ucrt" / "x64"),
        str(kits版[0] / "um" / "x64"),
    ]
    for lib路径 in LIB路径们:
        if not pathlib.Path(lib路径).exists():
            return "失败", f"LIB 路径不存在: {lib路径}"

    # 运行时 .obj（C++ 版构建产物；对齐宿主链接命令 10 个）
    运行时名们 = ["io_api", "intern_api", "runtime", "string_api", "i128_api",
                "math_api", "input_api", "file_api", "time_api", "system_api"]
    运行时objs = [项目根目录 / "target" / f"{m}.obj" for m in 运行时名们]
    for obj in 运行时objs:
        if not obj.exists():
            return "失败", f"缺少运行时 .obj: {obj.name}（请先构建 C++ 版编译器）"

    # ===== 步骤1：宿主编译 v2 组件（入口 主.cn，自动加载 6 个模块）-> v2p.exe =====
    if v2p.exists():
        v2p.unlink()
    if 详细:
        print(f"    [{编号}-1] {编译器路径} build 主.cn -> v2p.exe")
    编译结果 = 运行命令([str(编译器路径), "build", str(v2源码目录 / "主.cn"),
                      "--target", "win-x64", "--output", str(v2p)], 项目根目录)
    if 编译结果.returncode != 0:
        return "失败", f"{编号}-1 编译 v2 组件失败(退出码{编译结果.returncode}): {(编译结果.stderr or 编译结果.stdout).strip()[:200]}"
    if not v2p.exists():
        return "失败", f"{编号}-1 编译返回成功但未生成 v2p.exe"

    # ===== 步骤2：准备多文件程序（入口 主.cn + 导入模块文件） =====
    v2src目录 = 审计目录 / f"v2src{编号}"
    v2src目录.mkdir(parents=True, exist_ok=True)
    for 文件名 in 源文件名们:
        src = 用例目录 / 文件名
        if not src.exists():
            return "失败", f"{编号}-2 缺少用例文件: {文件名}"
        shutil.copy2(src, v2src目录 / 文件名)

    # ===== 步骤2.5：供给源编译（②b B7，对齐 linux 分支）——用例自有类型符号 =====
    #   同 linux：须拷贝为独立目录 主.cn（宿主仅以 主.cn 为入口模块，规范08-四）
    供给objs = []
    for 供给名 in 供给源们:
        供给src = 用例目录 / 供给名
        if not 供给src.exists():
            return "失败", f"{编号}-2.5 缺少供给源文件: {供给名}"
        供给目录 = 审计目录 / f"供给{编号}_{pathlib.Path(供给名).stem}"
        供给目录.mkdir(parents=True, exist_ok=True)
        shutil.copy2(供给src, 供给目录 / "主.cn")
        供给输出 = 供给目录 / "供给.exe"
        供给obj = 供给目录 / "供给.obj"
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
    入口参数 = f"target/audit2/v2src{编号}/主.cn"
    if v2asm路径.exists():
        v2asm路径.unlink()
    if 详细:
        print(f"    [{编号}-3] {v2p.name} {入口参数}")
    运行结果 = 运行命令([str(v2p), 入口参数], 项目根目录, 内存上限MB=内存上限MB默认)
    if 运行结果.returncode != 0:
        return "失败", f"{编号}-3 v2p 运行失败(退出码{运行结果.returncode}): {(运行结果.stderr or '').strip()[:300]}"
    if not v2asm路径.exists():
        return "失败", f"{编号}-3 v2p 未生成 target/v2asm.asm"
    # 输出比对：.expected 每行（去空）须为 v2p 实际输出的子串（数值列不参与精确比对）
    期望行们 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines() if 行.rstrip()]
    实际输出 = ((运行结果.stderr or "") + "\n" + (运行结果.stdout or ""))
    for 行 in 期望行们:
        if 行 not in 实际输出:
            return "失败", f"{编号}-3 v2p 输出缺少期望行: {行!r}\n    实际: {实际输出[:400]}"
    # 入口符号自检：v2 生成的 asm 必须含 cn_main（对齐宿主，链接后由运行时 entry 调用）
    asm内容 = v2asm路径.read_text(encoding="utf-8", errors="replace")
    if "cn_main PROC" not in asm内容:
        return "失败", f"{编号}-3.5 v2asm.asm 缺少入口符号 cn_main（v2 代码生成入口未对齐宿主）"

    # ===== 步骤4：ml64 汇编 target/v2asm.asm -> v2asm.obj =====
    v2obj = 审计目录 / "v2asm.obj"
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
    输出exe = 审计目录 / "v2out.exe"
    if 输出exe.exists():
        输出exe.unlink()
    响应文件 = 审计目录 / f"{编号}_link.rsp"
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
        v2pobj = 审计目录 / "v2p.obj"
        if not v2pobj.exists():
            return "失败", "123-5 缺少 target/audit2/v2p.obj（步骤1 cn build 未产出）"
        rsp_lines.append("/FORCE:MULTIPLE")
        # 供给 .obj 置于 v2p.obj 之前（②b B7：用例自有类型布局权威）
        rsp_lines += [str(v2obj)] + [str(o) for o in 供给objs] + [str(v2pobj)] + [str(o) for o in 运行时objs]
    else:
        rsp_lines += [str(v2obj)] + [str(o) for o in 运行时objs]
    # map 文件供符号方向自检（v2asm.obj 必须贡献 cn_main）
    map文件 = 审计目录 / f"{编号}_link.map"
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
        import re as _re
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
               "  python3 run_e2e.py --filter 01_hello")
    解析器.add_argument("--cn", help="cn编译器路径（默认自动探测 target/Debug 等）")
    解析器.add_argument("--target", default=None,
                        help="目标平台（win-x64 | linux-arm64；默认按本机平台自动推断）")
    解析器.add_argument("--max-mem-mb", type=int, default=内存上限MB默认,
                        help=f"重负载用例（78/79）运行子进程内存上限MB，超过自动终止（默认 {内存上限MB默认}MB；0=不启用）")
    解析器.add_argument("--verbose", "-v", action="store_true", help="详细输出（显示编译/运行命令）")
    解析器.add_argument("--filter", help="仅运行目录名包含指定模式的用例")
    解析器.add_argument("--target-dir", default="target", help="可执行文件输出目录（默认 target）")
    解析器.add_argument("--strict", action="store_true",
                        help="将'未实现'用例视为失败（阶段一完成后全量验证用）")
    参数 = 解析器.parse_args()

    # 覆盖模块级默认（超限自动终止的防护阈值）
    if 参数.max_mem_mb is not None and 参数.max_mem_mb >= 0:
        内存上限MB默认 = 参数.max_mem_mb

    # 目标平台：显式指定优先；否则按本机平台自动推断（Windows -> win-x64，其他 -> linux-arm64）
    目标平台 = 参数.target
    if 目标平台 is None:
        目标平台 = "win-x64" if sys.platform == "win32" else "linux-arm64"
    if 目标平台 not in ("win-x64", "linux-arm64"):
        print(红色(f"错误: 无效目标平台 {目标平台}（应为 win-x64 或 linux-arm64）"))
        return 2

    # 探测编译器与输出目录
    编译器路径 = 探测编译器(参数.cn)
    输出目录 = (项目根目录 / 参数.target_dir).resolve()
    输出目录.mkdir(parents=True, exist_ok=True)

    print(粗体("CN语言 E2E 测试运行器"))
    print(f"  编译器: {青色(str(编译器路径))}")
    print(f"  目标平台: {青色(目标平台)}")
    print(f"  输出目录: {输出目录}")
    print()

    # 收集用例
    用例目录们 = 收集用例(参数.filter)
    if not 用例目录们:
        print(黄色("未找到E2E用例目录（过滤条件无匹配或无用例）"))
        return 1

    # 逐个执行并统计
    通过数 = 0
    跳过数 = 0
    失败列表 = []
    未实现列表 = []
    for 用例目录 in 用例目录们:
        print(f"运行用例: {用例目录.name}")
        状态, 原因 = 执行单个用例(编译器路径, 用例目录, 输出目录, 参数.verbose, 目标平台)
        if 状态 == "通过":
            通过数 += 1
            print(f"  {绿色('PASS')} 通过")
        elif 状态 == "跳过":
            跳过数 += 1
            print(f"  {青色('SKIP')} {原因}")
        elif 状态 == "未实现":
            未实现列表.append((用例目录.name, 原因))
            print(f"  {黄色('SKIP')} 未实现: {原因}")
        else:
            失败列表.append((用例目录.name, 原因))
            print(f"  {红色('FAIL')} {原因}")
        print()

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
