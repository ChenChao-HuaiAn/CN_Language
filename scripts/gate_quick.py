#!/usr/bin/env python3
# L1 快速门禁（920 治理·三级门禁第一级——提交前·目标 ≤3 分钟增量）：
#   ① 静态三检查（ascii 标识符/关键字同步/门禁注册审计·秒级）
#   ② 增量构建（win=Ninja+sccache〔wt.py 开发树〕；linux=Make 增量）——零警告口径不变
#   ③ 全量单测（1410 例·实测秒级——无需做受影响选择）
# 不含 E2E：L2=影响面子集（任务分支收工）、L3=云端全量（TX_02 每推送+每日兜底·AGENTS v4）。
# 用法：python scripts/gate_quick.py   （rc=0=可提交；任何一步红=修复后再跑）

import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
是win = sys.platform.startswith("win")


def 跑(命令: list[str], **kw) -> subprocess.CompletedProcess:
    print("  $", " ".join(str(c) for c in 命令)[:100], flush=True)
    return subprocess.run([str(c) for c in 命令], cwd=仓库根, **kw)


def 步(名: str, 命令: list[str], **kw) -> bool:
    r = 跑(命令, **kw)
    print(f"[{名}] {'✓' if r.returncode == 0 else '✗ rc=' + str(r.returncode)}", flush=True)
    return r.returncode == 0


print("== L1 快速门禁（920·提交前）==", flush=True)

# ① 静态三检查（与 ci.ps1 第 0~0.5 步同源·不依赖构建产物）
for 名 in ("check_ascii_idents", "check_keywords_sync", "check_registry_audit"):
    if not 步(名, [sys.executable, f"scripts/{名}.py"]):
        raise SystemExit(1)

# ② 增量构建（win=Ninja+sccache〔ninja_build.cmd=vcvars+cache 一体〕；linux=Make 增量）
if 是win:
    if not 步("增量构建", ["cmd", "/c", "target\\ninja_build.cmd"]):
        raise SystemExit(1)
else:
    if not (仓库根 / "target/build-quick/CMakeCache.txt").exists():
        if not 步("首次配置", ["cmake", "-S", ".", "-B", "target/build-quick"]):
            raise SystemExit(1)
    if not 步("增量构建", ["cmake", "--build", "target/build-quick", "--parallel"]):
        raise SystemExit(1)

# ③ 全量单测（产物落源根 target/〔CMakeLists CMAKE_RUNTIME_OUTPUT_DIRECTORY〕）
单测 = next((p for p in [
    仓库根 / "target/cn_unit_tests.exe", 仓库根 / "target/cn_unit_tests",
    仓库根 / "target/build-ninja/cn_unit_tests.exe",
    仓库根 / "target/build-quick/cn_unit_tests",
] if p.exists()), None)
if 单测 is None:
    print("✗ 未找到单测产物（target/cn_unit_tests）——构建是否成功？")
    raise SystemExit(1)
if not 步("单测", [单测]):
    raise SystemExit(1)

print("== L1 全绿（可提交；L2 影响面收工跑·L3 云端全量自动）==", flush=True)
