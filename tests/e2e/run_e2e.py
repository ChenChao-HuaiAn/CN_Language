#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# CN语言E2E测试运行器：遍历用例目录，编译->运行->比对输出
# 用法: python3 run_e2e.py [--cn <编译器路径>] [--verbose] [--filter <模式>]
#
# 用例目录结构约定：
#   tests/e2e/<编号>_<名称>/<用例>.cn        # CN语言源文件
#   tests/e2e/<编号>_<名称>/<用例>.expected  # 期望输出（逐行比对）

import argparse
import ctypes
import os
import pathlib
import subprocess
import sys

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

# 默认编译器候选路径（按优先级自动探测）
编译器候选 = [
    pathlib.Path("target") / "Debug" / "cn.exe",
    pathlib.Path("target") / "Release" / "cn.exe",
    pathlib.Path("target") / "cn.exe",
]

# 编译器"尚未实现"标记（阶段零预期输出，用于区分真实失败）
未实现标记 = "尚未实现"

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


def 运行命令(命令列表: list, 工作目录: pathlib.Path) -> subprocess.CompletedProcess:
    """执行命令并返回结果（捕获stdout/stderr，UTF-8解码容错）"""
    return subprocess.run(
        命令列表, cwd=str(工作目录), capture_output=True,
        text=True, encoding="utf-8", errors="replace")


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
    return cn文件们[0]


def 查找期望文件(源文件: pathlib.Path) -> pathlib.Path:
    """查找与源文件同名的 .expected 期望输出文件"""
    期望文件 = 源文件.with_suffix(".expected")
    if not 期望文件.exists():
        raise FileNotFoundError(f"缺少期望输出文件: {期望文件}")
    return 期望文件

# ============ 核心逻辑 ============


def 执行单个用例(编译器路径: pathlib.Path, 用例目录: pathlib.Path,
                输出目录: pathlib.Path, 详细: bool) -> tuple:
    """
    执行单个E2E用例：编译->运行->比对输出
    返回 (状态, 说明)；状态取值: "通过" / "失败" / "未实现"
    """
    名称 = 用例目录.name
    try:
        源文件 = 查找源文件(用例目录)
        期望文件 = 查找期望文件(源文件)
    except FileNotFoundError as 异常:
        return "失败", str(异常)

    输出可执行 = 输出目录 / f"{名称}.exe"
    # 清理陈旧的可执行文件，确保"编译成功但未生成"可被可靠检测
    if 输出可执行.exists():
        输出可执行.unlink()

    # 1. 编译：cn build <源文件> --output <可执行文件>
    if 详细:
        print(f"    [编译] {编译器路径} build {源文件.name} --output {输出可执行}")
    编译结果 = 运行命令([str(编译器路径), "build", str(源文件),
                      "--output", str(输出可执行)], 项目根目录)
    if 编译结果.returncode != 0:
        提示 = (编译结果.stderr or 编译结果.stdout).strip()
        # 编译器尚未实现build命令：标记为"未实现"而非真实失败（阶段零预期状态）
        if 未实现标记 in 提示:
            return "未实现", f"编译器尚未实现build命令: {提示[:60]}"
        return "失败", f"编译失败(退出码{编译结果.returncode}): {提示[:200]}"
    if not 输出可执行.exists():
        return "失败", "编译返回成功但未生成可执行文件"

    # 2. 运行可执行文件
    if 详细:
        print(f"    [运行] {输出可执行}")
    运行结果 = 运行命令([str(输出可执行)], 项目根目录)
    if 运行结果.returncode != 0:
        return "失败", f"运行失败(退出码{运行结果.returncode}): {运行结果.stderr.strip()[:200]}"

    # 3. 比对期望输出（逐行比对，忽略行尾空白差异）
    期望 = [行.rstrip() for 行 in 期望文件.read_text(encoding="utf-8").splitlines()]
    实际 = [行.rstrip() for 行 in 运行结果.stdout.splitlines()]
    if 实际 != 期望:
        return "失败", f"输出不一致\n    期望: {期望}\n    实际: {实际}"
    return "通过", ""


def 打印汇总(总数: int, 通过数: int, 失败数: int, 未实现数: int) -> None:
    """打印E2E结果汇总与颜色标记"""
    print(粗体("E2E结果汇总"))
    print(f"  总计: {总数}  通过: {绿色(str(通过数))}  "
          f"失败: {红色(str(失败数))}  未实现: {黄色(str(未实现数))}")


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
    解析器 = argparse.ArgumentParser(
        description="CN语言E2E测试运行器：编译->链接->运行->比对输出",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n"
               "  python3 run_e2e.py\n"
               "  python3 run_e2e.py --cn target/Debug/cn.exe --verbose\n"
               "  python3 run_e2e.py --filter 01_hello")
    解析器.add_argument("--cn", help="cn编译器路径（默认自动探测 target/Debug 等）")
    解析器.add_argument("--verbose", "-v", action="store_true", help="详细输出（显示编译/运行命令）")
    解析器.add_argument("--filter", help="仅运行目录名包含指定模式的用例")
    解析器.add_argument("--target-dir", default="target", help="可执行文件输出目录（默认 target）")
    解析器.add_argument("--strict", action="store_true",
                        help="将'未实现'用例视为失败（阶段一完成后全量验证用）")
    参数 = 解析器.parse_args()

    # 探测编译器与输出目录
    编译器路径 = 探测编译器(参数.cn)
    输出目录 = (项目根目录 / 参数.target_dir).resolve()
    输出目录.mkdir(parents=True, exist_ok=True)

    print(粗体("CN语言 E2E 测试运行器"))
    print(f"  编译器: {青色(str(编译器路径))}")
    print(f"  输出目录: {输出目录}")
    print()

    # 收集用例
    用例目录们 = 收集用例(参数.filter)
    if not 用例目录们:
        print(黄色("未找到E2E用例目录（过滤条件无匹配或无用例）"))
        return 1

    # 逐个执行并统计
    通过数 = 0
    失败列表 = []
    未实现列表 = []
    for 用例目录 in 用例目录们:
        print(f"运行用例: {用例目录.name}")
        状态, 原因 = 执行单个用例(编译器路径, 用例目录, 输出目录, 参数.verbose)
        if 状态 == "通过":
            通过数 += 1
            print(f"  {绿色('PASS')} 通过")
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
    打印汇总(总数, 通过数, 失败数, 未实现数)
    打印详情(失败列表, 未实现列表, 参数.verbose)

    if 失败数 > 0:
        return 1
    if 参数.strict and 未实现数 > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(主程序())
