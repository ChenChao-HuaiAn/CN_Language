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

# 交叉模拟全局态（main 一次性判定·worker 并行只读）
交叉模拟 = False
交叉前缀 = []

# 交叉模拟跳过表（用例目录名精确/后缀匹配·同 平台跳过 语义）：
# 仅在交叉模拟激活时合并生效——真机 linux-arm64（单位机）不受影响。
交叉模拟跳过 = [
    # cn_self（arm64 ELF）全树编译 ×3 跳在 qemu 模拟下数小时=性能深坑；
    # 自举固定点验证由真机/原生平台兜底，此处登记制跳过（汇总显示）。
    "78_v2_自举链构建",
    "79_v2_自举闭环",
]


def 判定交叉模拟(目标平台: str) -> bool:
    """--target linux-arm64 且宿主非 arm64 → 激活交叉模拟（返回 True）。

    CN_RUN_PREFIX 环境变量：非空=自定义前缀（空白切分）；空串=禁用前缀
    （逃生门）；缺省=qemu-aarch64 -L /usr/aarch64-linux-gnu。
    前缀程序缺失即 SystemExit（配置错误尽早暴露，防整轮假红）。
    """
    global 交叉模拟, 交叉前缀
    if 目标平台 != "linux-arm64":
        return False
    if platform.machine().lower() in ("aarch64", "arm64"):
        return False  # 真机：原生闭环，不激活
    交叉模拟 = True
    覆盖 = os.environ.get("CN_RUN_PREFIX")
    if 覆盖 is not None:
        交叉前缀 = 覆盖.split() if 覆盖.strip() else []
    else:
        交叉前缀 = ["qemu-aarch64", "-L", "/usr/aarch64-linux-gnu"]
    if 交叉前缀 and shutil.which(交叉前缀[0]) is None:
        raise SystemExit(
            f"错误: 交叉模拟需 {交叉前缀[0]} 未找到——"
            "安装: sudo apt install qemu-user gcc-aarch64-linux-gnu "
            "g++-aarch64-linux-gnu（或设 CN_RUN_PREFIX= 禁用前缀）")
    return True


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
    if not (交叉模拟 and 交叉前缀 and 命令列表):
        return 命令列表
    if 是arm64可执行(str(命令列表[0])):
        return 交叉前缀 + list(命令列表)
    return 命令列表


def 构建交叉v2p双工件(编译器路径, 主cn, 项目根, 审计目录, 运行命令函数,
                      指纹函数, 构建超时秒数, 详细=False, 编号="PRE"):
    """交叉模式 v2p 双工件：本体 v2p_linuxx64（x64·跑）+ 符号 v2p_linux.o
    （arm64·链）。缓存复用既有 per-后缀 指纹键模式（输入不变不重建；符号侧
    独立键 v2p_symbol_key_linux.txt）。构建 cwd=项目根（与原生轮中间产物
    位置一致）。返回 (本体, 符号obj, 错误或None)。"""
    本体 = 审计目录 / "v2p_linuxx64"
    本体键 = 审计目录 / "v2p_build_key_linuxx64.txt"
    符号obj = 审计目录 / "v2p_linux.o"
    符号键 = 审计目录 / "v2p_symbol_key_linux.txt"
    本次指纹 = 指纹函数(编译器路径)

    # 本体（x64·与原生 linux-x86_64 轮共用既有缓存键）
    if not (本体.exists() and 本体键.exists()
            and 本体键.read_text(encoding="utf-8") == 本次指纹):
        编译 = 运行命令函数([str(编译器路径), "build", str(主cn),
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
