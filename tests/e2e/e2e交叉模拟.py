#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""E2E 交叉模拟模式（任务399·2026-10-10）——无 arm64 硬件宿主上的 linux-arm64 验证面。

定位（用户裁决·三平台全天候池）：--target linux-arm64 跑在 x86_64 宿主（家机
WSL2 交叉实例）时自动激活交叉模拟——编译器本体（宿主 cn / v2p）按 x64 构建
原生跑（满速），只让它生成 arm64 代码；arm64 ELF 产物运行时自动套
qemu-aarch64 前缀（CPU 指令流=真 arm64·内核/syscall 层=qemu 转译）——模拟验证，
真机金标准仍由单位机 arm64 兜底。

三件套（run_e2e 注入点）：
  1. 判定交叉模拟(目标平台)  —— main 参数校验后调用一次（qemu 缺失 fail fast）
  2. 交叉运行命令(命令列表)  —— 运行命令 入口调用：首元素为 arm64 ELF 时拼前缀
  3. 构建交叉v2p双工件(...)  —— 确保v2p与运行时就绪 交叉分支调用

v2p 双工件性能设计：本体 v2p_linuxx64（x64·原生速度跑）+ 容器符号
v2p_linux.o（arm64·链进产物）——两次构建均由 x64 宿主 cn 驱动（生成 arm64
汇编与交叉 as 汇编都是宿主程序原生速度），模拟开销只落在最终用例产物运行面。

已知边界（呈报备档·非缺陷）：
  - 78_v2/79_v2 自举固定点链的 cn_self 是 arm64 ELF 全树编译（原生约 20 分钟，
    qemu 模拟下数小时=性能深坑）——交叉模拟跳过表登记（汇总可见，非静默豁免），
    真机兜底。
  - RLIMIT_AS 默认 8GB 对 qemu 的 guest 地址预留偏紧，如误伤按实例放宽
    --max-mem-mb（实测调整项）。
"""
import os
import platform
import shutil

# 交叉态传递（任务399 实测教训）：不用模块级全局——E2E 用例在进程池 worker 里
# 跑，fork 继承/spawn 重 import 两种语义下全局标志都会与主进程分叉（128_v2
# worker 走错分支实录）。改用环境变量 CN_XSIM/CN_XSIM_PREFIX 传递：main 一次性
# 判定写入，fork 继承与 spawn env 继承全态一致，worker 只读。
XSIM环境键 = "CN_XSIM"
前缀环境键 = "CN_XSIM_PREFIX"

# 交叉模拟跳过表（用例目录名精确/后缀匹配·同 平台跳过 语义）：
# 仅在交叉模拟激活时合并生效——真机 linux-arm64（单位机）不受影响。
交叉模拟跳过 = [
    # cn_self（arm64 ELF）全树编译 ×3 跳在 qemu 模拟下数小时=性能深坑；
    # 自举固定点验证由真机/原生平台兜底，此处登记制跳过（汇总显示）。
    "78_v2_自举链构建",
    "79_v2_自举闭环",
]


def 判定交叉模拟(目标平台: str) -> bool:
    """--target linux-arm64 且宿主非 arm64 → 激活交叉模拟（返回 True·写环境变量）。

    CN_RUN_PREFIX 环境变量：非空=自定义前缀（空白切分）；空串=禁用前缀
    （逃生门）；缺省=qemu-aarch64 -L /usr/aarch64-linux-gnu。
    前缀程序缺失即 SystemExit（配置错误尽早暴露，防整轮假红）。
    """
    if 目标平台 != "linux-arm64":
        return False
    if platform.machine().lower() in ("aarch64", "arm64"):
        return False  # 真机：原生闭环，不激活
    覆盖 = os.environ.get("CN_RUN_PREFIX")
    if 覆盖 is not None:
        前缀 = 覆盖.split() if 覆盖.strip() else []
    else:
        前缀 = ["qemu-aarch64", "-L", "/usr/aarch64-linux-gnu"]
    if 前缀 and shutil.which(前缀[0]) is None:
        raise SystemExit(
            f"错误: 交叉模拟需 {前缀[0]} 未找到——"
            "安装: sudo apt install qemu-user gcc-aarch64-linux-gnu "
            "g++-aarch64-linux-gnu（或设 CN_RUN_PREFIX= 禁用前缀）")
    os.environ[XSIM环境键] = "1"
    os.environ[前缀环境键] = " ".join(前缀)
    return True


def 交叉模拟激活() -> bool:
    """worker/主进程统一的激活判定（读环境变量·fork/spawn 全态一致）。"""
    return os.environ.get(XSIM环境键) == "1"


def 生效前缀() -> list:
    """生效运行前缀（空串=禁用·判定函数写入，worker 只读）。"""
    return os.environ.get(前缀环境键, "").split()


def 是arm64可执行(路径: str) -> bool:
    """ELF 头探测：e_machine（offset 18·小端 2 字节）==183(0xB7)=AArch64。
    非 ELF/读失败一律 False——shell 命令/git/编译器本体（x64 ELF）零误伤。"""
    try:
        with open(路径, "rb") as f:
            头 = f.read(20)
    except OSError:
        return False
    return (len(头) >= 20 and 头[:4] == b"\x7fELF"
            and 头[18] == 0xB7 and 头[19] == 0)


def 交叉运行命令(命令列表: list) -> list:
    """运行命令 入口的交叉注入：首元素为 arm64 ELF 产物时套 qemu 前缀。
    统一入口生效于全部产物运行点（普通用例/v2 闭环 v2out/双编译对照
    hostout/锚定链）——产物判定走 ELF 头而非调用点语义，零遗漏零误伤。"""
    前缀 = 生效前缀()
    if not (交叉模拟激活() and 前缀 and 命令列表):
        return 命令列表
    if 是arm64可执行(str(命令列表[0])):
        return 前缀 + list(命令列表)
    return 命令列表


def 构建交叉v2p双工件(编译器路径, 主cn, 项目根, 审计目录, 运行命令函数,
                      指纹函数, 构建超时秒数, 详细=False, 编号="PRE"):
    """交叉模式 v2p 双工件：本体 v2p_linuxx64（x64·跑）+ 符号 v2p_linux.o
    （arm64·链）。缓存复用既有 per-后缀 指纹键模式（输入不变不重建；符号侧
    独立键 v2p_symbol_key_linux.txt）。构建 cwd=项目根（与原生轮中间产物
    位置一致）。返回 (本体, 符号obj, 错误或None)。

    构建序列（必读本体的在前·符号的收尾）：cn 的运行时 obj 缓存
    （target/*.o）判据只有 mtime 不含架构（cn_main_toolchain compileRuntime）
    ——双架构构建同树互踩（实测：x64 io_api.o 被 arm64 链接撞 EM:62 重定位
    错误）。故每次构建前清 target/*.o 强制按本架构重编（14 件原生秒级），
    且符号构建放最后——并行用例窗口（全部 arm64 目标）看到的缓存终态恒为
    arm64。预热单线程窗口内完成，无并行污染。"""
    本体 = 审计目录 / "v2p_linuxx64"
    本体键 = 审计目录 / "v2p_build_key_linuxx64.txt"
    符号obj = 审计目录 / "v2p_linux.o"
    符号键 = 审计目录 / "v2p_symbol_key_linux.txt"
    本次指纹 = 指纹函数(编译器路径)

    def 清运行时obj缓存():
        n = 0
        for o in (项目根 / "target").glob("*.o"):
            o.unlink()
            n += 1
        return n

    # 本体（x64·与原生 linux-x86_64 轮共用既有缓存键）
    if not (本体.exists() and 本体键.exists()
            and 本体键.read_text(encoding="utf-8") == 本次指纹):
        # env -u 摘除 CN_AS/CN_CXX：本体构建是 x64 目标，cn 内部工具链必须走
        #   PATH 原生 as/g++（实测教训：交叉变量全局注入时 x64 本体被 arm64
        #   as 汇编必败）；清 obj 缓存防他架构残留参与链接
        清运行时obj缓存()
        编译 = 运行命令函数(["env", "-u", "CN_AS", "-u", "CN_CXX",
                             str(编译器路径), "build", str(主cn),
                             "--target", "linux-x86_64", "--output", str(本体)],
                            项目根, 超时秒数=构建超时秒数)
        if 编译.returncode != 0:
            return None, None, (f"{编号}-1 交叉本体构建失败(退出码{编译.returncode}): "
                                f"{(编译.stderr or 编译.stdout).strip()[:200]}")
        if not 本体.exists():
            return None, None, f"{编号}-1 交叉本体构建成功但未生成 {本体.name}"
        本体键.write_text(本次指纹, encoding="utf-8")
    elif 详细:
        print(f"    [{编号}-1] 交叉本体缓存命中，复用 {本体.name}")

    # 容器符号（arm64 .o·产物链接面；可执行副产品删除防误用）
    if not (符号obj.exists() and 符号键.exists()
            and 符号键.read_text(encoding="utf-8") == 本次指纹):
        # 收尾构建：清缓存（此时若本体刚建过=清 x64 残留）→ 重编 arm64
        #   运行时 obj →arm64 链接——终态缓存=arm64，正对并行用例窗口
        清运行时obj缓存()
        副产品 = 审计目录 / "v2p_linux"
        编译 = 运行命令函数([str(编译器路径), "build", str(主cn),
                             "--target", "linux-arm64", "--output", str(副产品)],
                            项目根, 超时秒数=构建超时秒数)
        if 编译.returncode != 0:
            return None, None, (f"{编号}-1a 交叉符号构建失败(退出码{编译.returncode}): "
                                f"{(编译.stderr or 编译.stdout).strip()[:200]}")
        if not 符号obj.exists():
            return None, None, f"{编号}-1a 交叉符号构建未产出 {符号obj.name}"
        if 副产品.exists():
            副产品.unlink()
        符号键.write_text(本次指纹, encoding="utf-8")
    elif 详细:
        print(f"    [{编号}-1a] 交叉符号缓存命中，复用 {符号obj.name}")
    return 本体, 符号obj, None
