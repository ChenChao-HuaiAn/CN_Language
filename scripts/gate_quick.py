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

# ④ L2 影响面子集（--l2·924 首版保守映射：写集中新增/改动的 E2E 用例目录全跑 +
#    触 CN语言编译器v2/ 树加跑 v2 面+自举锚；src 模块→用例关键词精映射=后续演进（025 §1.99 边界）。
#    映射不覆盖的改动=诚实兜底：提示走 L3 云端全量，不静默漏。）
if "--l2" in sys.argv:
    import subprocess as _sp
    基准 = _sp.run(["git", "merge-base", "HEAD", "gitcode/develop"], cwd=仓库根,
                   capture_output=True, text=True).stdout.strip()
    # 979 修复（586 轮教训同族再犯）：core.quotepath 默认把中文用例目录名转义成
    #   "\346\227\..." 八进制串 → 子串 filter 永不匹配 → L2 子集空转 rc=1。
    #   显式关 quotepath 保证中文路径原样输出（filter 匹配靠它）。
    改动 = (_sp.run(["git", "-c", "core.quotepath=false", "diff", "--name-only", 基准],
                    cwd=仓库根, capture_output=True, text=True).stdout or "").split()
    # 979 修复②：split 索引 [1]=「e2e」段，用例目录名在 [2]——原 [1] 使任何含
    #   用例改动的写集 filter 恒为「e2e」（零匹配 rc=1·--l2 首战 924 起潜伏，
    #   979 首个触用例写集实锤）。
    # 354 修复③：tests/e2e/ 根级文件（README.md/run_e2e.py/coverage_map.md）的
    #   [2] 段=文件名非用例目录名（354 轮写集实锤：--filter README.md 零匹配
    #   rc=1）——is_dir 判据根治：只有真实用例目录才进滤集。
    用例集 = sorted({p.split("/")[2] for p in 改动
                     if p.startswith("tests/e2e/") and len(p.split("/")) > 2
                     and (仓库根 / "tests" / "e2e" / p.split("/")[2]).is_dir()})
    滤们 = 用例集 + (["_v2", "自举"] if any(p.startswith("CN语言编译器v2/") for p in 改动) else [])
    if not 滤们:
        print("[L2] 写集无新增/改动用例且未触 v2 树——无需子集（L3 云端全量兜底）")
    else:
        目标 = "win-x64" if 是win else "linux-x86_64"
        for 滤 in 滤们:
            if not 步("L2:" + 滤, [sys.executable, "tests/e2e/run_e2e.py",
                                   "--target", 目标, "--cn", str(单测.parent / ("cn.exe" if 是win else "cn")),
                                   "--filter", 滤]):
                raise SystemExit(1)


print("== L1 全绿（可提交；L2 影响面收工跑·L3 云端全量自动）==", flush=True)
