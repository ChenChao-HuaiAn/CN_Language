#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_gate_quick_timeout.py —— 860 自测：gate_quick 子进程显式超时+整树回收。

背景（任务 #860）：2026-10-15 07:36 家机 wt652 树 gate_quick 单测 cn_unit_tests
烧满一核挂死 18.4h（CPU 0 输出·无超时），父进程持树锁阻塞整树。修=全部子进程
步骤经 ci_proc.运行树 整树生命线+分级超时（CN_GATE_*_TIMEOUT）+超时 rc=-99
诚实红+[超时回收] 审计行。

本自测在**隔离临时树**上跑真实生产脚本（复制真 gate_quick/gate_lock/ci_proc，
伪造静态检查脚本与构建脚本-挂死替身），四场景：
  D 静态检查步骤挂死 → CN_GATE_STATIC_TIMEOUT 回收（父+后代 python 全灭）；
  A 构建步骤挂死（cmd→python 父→python 后代两层树）→ CN_GATE_BUILD_TIMEOUT 回收；
  B 单测步骤挂死（csc 编真 PE·win / cc 编 ELF·linux）→ CN_GATE_TEST_TIMEOUT 回收；
  C 全绿直通（假构建假单测均秒退）→ rc=0（防「恒红假测试」自证）。
每场景断言：进程按时退出（不挂）、rc=1、输出含 [超时回收] 审计与 rc=-99、
挂死进程 pid 已死（整树回收）。
用法：python scripts/test_gate_quick_timeout.py   （rc=0=全绿）
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(仓库根 / "scripts"))
import gate_lock  # noqa: E402  （pid存活 复用）

是win = sys.platform.startswith("win")
通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


# 场景 A/D 的挂死替身：自身挂死 + 起一个挂死后代（验证整树回收而非只杀直接子）
挂死脚本 = """\
import os, subprocess, sys, time
pidf = os.path.join(os.environ["CN_TEST_DIR"], "{文件}")
后代 = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(99999)"])
open(pidf, "w").write(str(os.getpid()) + " " + str(后代.pid))
time.sleep(99999)
"""

秒退脚本 = "import sys\nsys.exit(0)\n"


def 造临时树(基础: str) -> Path:
    """最小 gate_quick 运行面：scripts/{gate_quick,gate_lock,ci_proc,伪造检查}.py
    + target/（构建脚本与单测产物由各场景注入）。"""
    根 = Path(tempfile.mkdtemp(prefix="gq_", dir=基础))
    (根 / "scripts").mkdir()
    (根 / "target").mkdir()
    for 真 in ("gate_quick.py", "gate_lock.py", "ci_proc.py", "check_build_freshness.py"):
        shutil.copy(仓库根 / "scripts" / 真, 根 / "scripts" / 真)
    for 伪 in ("check_ascii_idents", "check_keywords_sync", "check_registry_audit",
               "check_case_numbers", "check_miss_gate", "check_file_length",
               "check_layer_deps"):
        (根 / "scripts" / f"{伪}.py").write_text(秒退脚本, encoding="utf-8")
    (根 / "scripts" / "fake_hang.py").write_text(
        挂死脚本.format(文件="hang.pid"), encoding="utf-8")
    (根 / "scripts" / "fake_hang_build.py").write_text(
        挂死脚本.format(文件="build.pid"), encoding="utf-8")
    # 假构建脚本：win=cmd 调 python 挂死替身；linux 分支 gate_quick 走 cmake 路径，
    #   本自测仅覆盖 win 构建步骤（linux 构建分支同一 跑/步 机制·静态与单测场景跨平台）。
    假构建 = ("@echo off\r\n"
              f'"{sys.executable}" "%~dp0..\\scripts\\fake_hang_build.py"\r\n')
    (根 / "target" / "ninja_build.cmd").write_text(假构建, encoding="ascii")
    return 根


def 跑门禁(根: Path, 超时们: dict, 附加环境: dict | None = None, 上限秒: int = 90):
    """在临时树跑真 gate_quick（子进程·捕获输出·硬上限防本测试自身挂死）。"""
    环境 = dict(os.environ, CN_NO_TREE_LOCK="1", CN_TEST_DIR=str(根 / "target"))
    for k in ("CN_GATE_STATIC_TIMEOUT", "CN_GATE_BUILD_TIMEOUT",
              "CN_GATE_TEST_TIMEOUT", "CN_GATE_L2_TIMEOUT"):
        环境.pop(k, None)
    环境.update(超时们)
    if 附加环境:
        环境.update(附加环境)
    起 = time.time()
    try:
        结 = subprocess.run(
            [sys.executable, str(根 / "scripts" / "gate_quick.py")],
            cwd=str(根), env=环境, capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=上限秒)
        return 结.returncode, 结.stdout + 结.stderr, time.time() - 起
    except subprocess.TimeoutExpired as e:
        出 = (e.stdout or b"") + (e.stderr or b"")
        if isinstance(出, bytes):
            出 = 出.decode("utf-8", "replace")
        return None, 出, time.time() - 起


def 等文件(路径: Path, 上限秒: float = 20) -> str:
    for _ in range(int(上限秒 * 20)):
        if 路径.exists():
            return 路径.read_text(encoding="utf-8").strip()
        time.sleep(0.05)
    raise AssertionError(f"挂死替身未写出 pid 文件：{路径}")


def 场景D_静态检查超时(基础: str) -> None:
    根 = 造临时树(基础)
    (根 / "scripts" / "check_ascii_idents.py").write_text(
        挂死脚本.format(文件="static.pid"), encoding="utf-8")
    pid文件 = 根 / "target" / "static.pid"
    码, 出, 秒 = 跑门禁(根, {"CN_GATE_STATIC_TIMEOUT": "5"})
    断言(码 == 1 and 秒 < 60, f"场景D 静态检查挂死：按时退出（{秒:.1f}s·rc={码}）")
    断言("rc=-99" in 出 and "[超时回收]" in 出, "场景D rc=-99 诚实红+[超时回收] 审计行")
    父, 子 = 等文件(pid文件).split()
    断言(not gate_lock.pid存活(int(父)) and not gate_lock.pid存活(int(子)),
         "场景D 整树回收：父+后代进程全灭")


def 场景A_构建超时(基础: str) -> None:
    根 = 造临时树(基础)
    pid文件 = 根 / "target" / "build.pid"
    码, 出, 秒 = 跑门禁(根, {"CN_GATE_BUILD_TIMEOUT": "8"})
    断言(码 == 1 and 秒 < 70, f"场景A 构建挂死：按时退出（{秒:.1f}s·rc={码}）")
    断言("[增量构建] ✗ rc=-99" in 出 and "[超时回收]" in 出 and "超时 8s" in 出,
         "场景A 构建步骤 rc=-99+超时秒数审计（CN_GATE_BUILD_TIMEOUT=8 生效）")
    父, 子 = 等文件(pid文件).split()
    断言(not gate_lock.pid存活(int(父)) and not gate_lock.pid存活(int(子)),
         "场景A 整树回收：cmd→python 父→python 后代全灭")


def 找csc() -> Path:
    """VS Roslyn csc 探测（固定路径+固定路径不在时 vswhere 兜底）。"""
    固定 = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild"
                r"\Current\Bin\Roslyn\csc.exe")
    if 固定.exists():
        return 固定
    vswhere = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
    if vswhere.exists():
        出 = subprocess.run([str(vswhere), "-latest", "-find",
                            "MSBuild\\Current\\Bin\\Roslyn\\csc.exe"],
                           capture_output=True, text=True).stdout.strip()
        if 出:
            return Path(出)
    raise AssertionError("未找到 csc.exe（VS Roslyn）——win 场景需要 VS2022 安装面")


def 编挂死程序(目标: Path, pid环境变量: str) -> None:
    """真 PE（win=csc）/ELF（linux=cc）：写 pid 到环境变量指定文件后 Sleep。"""
    if 是win:
        源 = 目标.with_suffix(".cs")
        源.write_text(
            "using System.Diagnostics; using System.IO; using System.Threading;\n"
            "class P { static void Main() {\n"
            f'  File.WriteAllText(System.Environment.GetEnvironmentVariable("{pid环境变量}"),\n'
            "    Process.GetCurrentProcess().Id.ToString());\n"
            "  Thread.Sleep(999999999); } }\n", encoding="utf-8")
        r = subprocess.run([str(找csc()), "/nologo", f"/out:{目标}", str(源)],
                           capture_output=True, text=True)
        assert r.returncode == 0, f"csc 编译失败：{r.stdout} {r.stderr}"
    else:
        源 = 目标.with_suffix(".c")
        源.write_text(
            "#include <stdio.h>\n#include <stdlib.h>\n#include <unistd.h>\n"
            "int main(){ FILE*f=fopen(getenv(\"%s\"),\"w\");"
            "fprintf(f,\"%%d\",getpid()); fclose(f);"
            "for(;;) sleep(9999); }\n" % pid环境变量, encoding="utf-8")
        r = subprocess.run(["cc", "-o", str(目标), str(源)], capture_output=True, text=True)
        assert r.returncode == 0, f"cc 编译失败：{r.stderr}"


def 场景B_单测超时(基础: str) -> None:
    根 = 造临时树(基础)
    (根 / "target" / "ninja_build.cmd").write_text("@exit /b 0\r\n", encoding="ascii")
    编挂死程序(根 / "target" / "cn_unit_tests.exe", "CN_TEST_PID_FILE")
    pid文件 = 根 / "target" / "unit.pid"
    码, 出, 秒 = 跑门禁(根, {"CN_GATE_TEST_TIMEOUT": "8"},
                       附加环境={"CN_TEST_PID_FILE": str(pid文件)})
    断言(码 == 1 and 秒 < 70, f"场景B 单测挂死：按时退出（{秒:.1f}s·rc={码}）")
    断言("[单测] ✗ rc=-99" in 出 and "[超时回收]" in 出 and "超时 8s" in 出,
         "场景B 单测步骤 rc=-99+超时秒数审计（CN_GATE_TEST_TIMEOUT=8 生效）")
    公 = 等文件(pid文件)
    断言(not gate_lock.pid存活(int(公)), "场景B 整树回收：挂死单测进程已灭")


def 编秒退程序(目标: Path) -> None:
    源 = 目标.with_suffix(".cs")
    源.write_text("class P { static void Main() {} }\n", encoding="utf-8")
    r = subprocess.run([str(找csc()), "/nologo", f"/out:{目标}", str(源)],
                       capture_output=True, text=True)
    assert r.returncode == 0, f"csc 编译失败：{r.stdout} {r.stderr}"


def 场景C_全绿直通(基础: str) -> None:
    根 = 造临时树(基础)
    (根 / "target" / "ninja_build.cmd").write_text("@exit /b 0\r\n", encoding="ascii")
    编秒退程序(根 / "target" / "cn_unit_tests.exe")
    码, 出, 秒 = 跑门禁(根, {"CN_GATE_STATIC_TIMEOUT": "60", "CN_GATE_BUILD_TIMEOUT": "60",
                            "CN_GATE_TEST_TIMEOUT": "60"})
    断言(码 == 0, f"场景C 全绿直通 rc=0（{秒:.1f}s——防恒红假测试）")
    断言("== L1 全绿" in 出, "场景C 全绿横幅在")


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="cn_gq_timeout_") as 基础:
        print("== 场景D 静态检查步骤挂死 → CN_GATE_STATIC_TIMEOUT ==")
        场景D_静态检查超时(基础)
        if 是win:
            print("== 场景A 构建步骤挂死（两层树）→ CN_GATE_BUILD_TIMEOUT ==")
            场景A_构建超时(基础)
            print("== 场景B 单测步骤挂死（真 PE）→ CN_GATE_TEST_TIMEOUT ==")
            场景B_单测超时(基础)
            print("== 场景C 全绿直通 ==")
            场景C_全绿直通(基础)
        else:
            # linux 的 gate_quick 构建分支=真实 cmake 调用（临时树无法低成本伪造
            #   挂死），A/B/C 场景为 win 专属；构建/单测步骤与场景 D 共用同一
            #   跑/步 接线（超时参数逐点传入），linux 完整性由本机/云端 L1 实跑承担。
            print("[跳过] 场景 A/B/C：win 专属（linux 构建走真实 cmake·非本自测靶面）")
    print(f"\n[全部通过] {通过} 项断言")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as 异常:
        print(f"\n[自测失败] {异常}")
        raise SystemExit(1)
    except KeyboardInterrupt:
        print("\n[自测中断]")
        raise SystemExit(130)
