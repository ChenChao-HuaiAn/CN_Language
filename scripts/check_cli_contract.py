#!/usr/bin/env python3
"""CLI 契约矩阵门禁（检查网第 2 层·plans/026 §2.7·249-a 建成）。

锚定 cn 驱动的**五维面**契约（对标 rust-lang/rust `tests/run-make` 层，非点检查）：
  ① 子命令面：build/compile/run/check/ir/ast/token 全 7 命令 × 成功/失败形态
  ② 退出码语义面：程序 rc 0/1/3/5/255 透传、运行期错误码、编译失败码、信号…
  ③ 选项面：--target 三平台/四优化级/--opt/--no-regalloc/--debug/--验证-ir/--cfi/
     --发布/--verbose/--output（含深层目录）/--version/--help/未知选项
  ④ 命令行形态面：无参数/未知子命令/缺文件/不存在文件/空文件/目录输入
  ⑤ 通道与诊断面：程序输出→stdout、编译诊断→stderr、诊断格式 `文件:行:列: 错误:`

首锚=B4（cn run 退出码透传·POSIX waitstatus 解包）——格「run 退出码透传」还原 846 行
裸 return 即 FAIL（反证留档 249-a）。次锚=目录输入拒绝（CLI 面发现并同轮根治）。

用法：
  python3 scripts/check_cli_contract.py [--cn target/cn] [--target linux-arm64]
  平台缺省=CN_CI_PLATFORM 自报优先（822·与 ci_daemon/run_e2e 同判据），
  无自报则按本机运行时探测（win-x64 / linux-arm64 / linux-x86_64）。
  交叉平台仅测 compile（产物形态，不需本地工具链）；运行级形态只测本机
  运行时平台（822 起可达性判据=运行时平台，与验证面缺省分离）。
  退出码：0=全格通过；1=有失败格。
"""
from __future__ import annotations

import argparse
import os
import platform as 平台模块
import re
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent

# 探针源（target/cli_contract/ 生成；target/ 已 gitignore）
合法程序 = "函数 主() -> 整32 {\n    打印行(7);\n    返回 0;\n}\n"
非法程序 = "函数 主() -> 整32 {\n    返回 ;\n}\n"
# 319-a（T8·方案甲）：常量除零已改编译期硬错误——运行期错误码格改用变量面样本
除零程序 = "函数 主() -> 整32 {\n    整32 乙 = 0;\n    整32 甲 = 10 / 乙;\n    打印行(甲);\n    返回 0;\n}\n"
语法错程序 = "函数 主() -> 整32 {\n    返回 0\n}\n"   # 缺分号（语法层）
# 汇合程序（256-a）：-O3 SSA/Phi 降级触达面（ir 打印搬运名/交叉后端 emitCopy）
汇合程序 = ("函数 取较大(整32 甲, 整32 乙) -> 整32 {\n"
            "    整32 结果值 = 0;\n"
            "    如果 (甲 > 乙) {\n"
            "        结果值 = 甲;\n"
            "    } 否则 {\n"
            "        结果值 = 乙;\n"
            "    }\n"
            "    返回 结果值;\n"
            "}\n"
            "函数 主() -> 整32 {\n"
            "    返回 取较大(1, 2);\n"
            "}\n")
词法错程序 = "函数 主() -> 整32 {\n    返回 @;\n}\n"   # 非法字符（词法层）


def 返回码源(n: int) -> str:
    return f"函数 主() -> 整32 {{\n    返回 {n};\n}}\n"


def 运行时平台() -> str:
    """本机真实执行平台（只看运行时探测·不受任何自报覆盖）——运行级格可达性
    判据用：产物要能本机执行才跑 build/run；交叉视角产物非本机形态必 SKIP。"""
    if sys.platform.startswith("win"):
        return "win-x64"
    m = 平台模块.machine().lower()
    return "linux-arm64" if ("aarch64" in m or "arm64" in m) else "linux-x86_64"


def 缺省目标平台() -> str:
    """验证面缺省目标：CN_CI_PLATFORM 自报优先（822·2026-10-15），缺省=运行时平台。

    交叉模拟实例（399·x86_64 宿主跑 linux-arm64 验证面·CN_AS/CN_CXX=交叉工具链）
    经 CN_CI_PLATFORM 自报身份——与 ci_daemon/run_e2e 同判据。此前本脚本不感知自报，
    在交叉实例上把验证面误判成 linux-x86_64：运行级格（build/run）把 x86_64 汇编
    喂给 arm64 交叉工具链必败——42/50 恒红 12 轮兜底存量实录（822 考古根治）。"""
    自报 = os.environ.get("CN_CI_PLATFORM", "").strip()
    return 自报 if 自报 else 运行时平台()


def 运行(cn: str, 参数: list[str], 超时: int = 180) -> subprocess.CompletedProcess:
    return subprocess.run(
        [cn, *参数], cwd=仓库根, capture_output=True, text=True,
        encoding="utf-8", errors="replace", timeout=超时,
    )


诊断 = lambda p: (p.stdout or "") + (p.stderr or "")
诊断格式 = re.compile(r".+:\d+:\d+: (错误|警告): ")


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="CLI 契约矩阵门禁（检查网第 2 层）")
    解析.add_argument("--cn", default="target/cn", help="cn 可执行文件路径（默认 target/cn）")
    解析.add_argument("--target", default="", help="目标平台（缺省=CN_CI_PLATFORM 自报，否则运行时探测）")
    参数 = 解析.parse_args()
    cn = 参数.cn if Path(参数.cn).is_absolute() else str(仓库根 / 参数.cn)
    target = 参数.target or 缺省目标平台()
    # 运行级可达（红例三返工·2026-10-12；822 判据源修正）：「交叉平台仅测 compile；
    #   运行级形态只测本机目标」此前未落实到 build/run 格——显式 --target 交叉平台
    #   时（x86 机模拟 arm64 视角/CI 矩阵）运行级格产物无法本机执行必红（8 格
    #   实证）。目标=本机运行时平台才执行运行级（build/run/退出码/除零/验证ir），
    #   交叉视角自动跳过（不计失败）。822 起可达性判据=运行时平台（真实执行面），
    #   不再与「验证面缺省」混用——交叉模拟实例（CN_CI_PLATFORM 自报 arm64）的
    #   验证面=linux-arm64≠本机 linux-x86_64，运行级照 SKIP 而 compile 面真跑。
    运行级可达 = (target == 运行时平台())

    探针 = 仓库根 / "target" / "cli_contract"
    探针.mkdir(parents=True, exist_ok=True)
    正常 = 探针 / "正常.cn"
    非法 = 探针 / "非法.cn"
    除零 = 探针 / "除零.cn"
    正常.write_text(合法程序, encoding="utf-8")
    汇合 = 探针 / "汇合.cn"
    汇合.write_text(汇合程序, encoding="utf-8")
    非法.write_text(非法程序, encoding="utf-8")
    除零.write_text(除零程序, encoding="utf-8")
    返回码路径 = {}
    for n in (1, 3, 5, 255):
        p = 探针 / f"返回{n}.cn"
        p.write_text(返回码源(n), encoding="utf-8")
        返回码路径[n] = p
    目录探针 = 探针 / "目录.cn"
    目录探针.mkdir(exist_ok=True)
    空文件 = 探针 / "空.cn"
    空文件.write_text("", encoding="utf-8")

    台 = ["--target", target] if target else []
    产物后缀 = ".exe" if target == "win-x64" else ""
    汇编后缀 = ".asm" if target == "win-x64" else ".s"
    格: list[tuple[str, bool, str]] = []
    维度 = {"名": ""}

    def 断言(名: str, ok: bool, 详情: str) -> None:
        格.append((名, ok, 详情))
        print(f"[{'PASS' if ok else 'FAIL'}] {名}：{详情}")

    def 换维度(名: str) -> None:
        维度["名"] = 名
        print(f"\n=== {名} ===")

    def 跳过(名: str, 原因: str) -> None:
        # 交叉视角运行级格：不计入失败（红例三返工——产物非本机可执行形态）
        格.append((名, True, f"SKIP：{原因}"))
        print(f"[SKIP] {名}：{原因}")

    # ============ ① 子命令面 ============
    换维度("① 子命令面（7 命令 × 成功/失败）")
    p = 运行(cn, ["check", str(正常), *台])
    断言("check 合法 rc=0", p.returncode == 0, f"rc={p.returncode}")
    p = 运行(cn, ["check", str(非法), *台])
    断言("check 非法 rc≠0+诊断", p.returncode != 0 and "错误" in 诊断(p), f"rc={p.returncode}")

    # 522：默认产物路径契约=target/<目标架构桶>/<stem>（桶名=--target 值·与
    #   src/cn_main_toolchain.cpp archBucketDir 同规则·零映射）
    产物 = 仓库根 / "target" / target / ("正常" + 产物后缀)
    if 运行级可达:
        p = 运行(cn, ["build", str(正常), *台])
        断言("build 合法 rc=0+产物", p.returncode == 0 and 产物.exists(), f"rc={p.returncode}")
    else:
        跳过("build 合法 rc=0+产物", "交叉视角产物非本机可执行形态（compile 面已测）")
    p = 运行(cn, ["build", str(非法), *台])
    断言("build 非法 rc≠0", p.returncode != 0, f"rc={p.returncode}")

    p = 运行(cn, ["compile", str(正常), *台])
    汇编 = 仓库根 / "target" / target / ("正常" + 汇编后缀)
    断言("compile 合法 rc=0+汇编产物", p.returncode == 0 and 汇编.exists(), f"rc={p.returncode}")
    p = 运行(cn, ["compile", str(非法), *台])
    断言("compile 非法 rc≠0", p.returncode != 0, f"rc={p.returncode}")

    if 运行级可达:
        p = 运行(cn, ["run", str(正常), *台])
        断言("run 合法 rc=0+程序输出", p.returncode == 0 and "7" in p.stdout, f"rc={p.returncode}")
        p = 运行(cn, ["run", str(非法), *台])
        断言("run 编译失败 rc≠0", p.returncode != 0, f"rc={p.returncode}")
    else:
        跳过("run 合法 rc=0+程序输出", "交叉视角产物非本机可执行形态")
        跳过("run 编译失败 rc≠0", "交叉视角产物非本机可执行形态")

    for 命令 in ("ir", "ast", "token"):
        p = 运行(cn, [命令, str(正常), *台])
        断言(f"{命令} 合法 rc=0+有输出", p.returncode == 0 and len(p.stdout.strip()) > 0,
             f"rc={p.returncode}·输出{len(p.stdout.strip())}字符")

    # 分层口径锚定：token=词法层 / ast=语法层 / ir=全流水线——各自拒绝**本层**错误，
    # 放行下游层错误（ast 对语义错 rc=0；token 对语法/语义错 rc=0）
    语法错路径 = 探针 / "语法错.cn"
    词法错路径 = 探针 / "词法错.cn"
    语法错路径.write_text(语法错程序, encoding="utf-8")
    词法错路径.write_text(词法错程序, encoding="utf-8")
    p = 运行(cn, ["ir", str(非法), *台])
    断言("ir 语义错 rc≠0（全流水线）", p.returncode != 0, f"rc={p.returncode}")
    p = 运行(cn, ["ast", str(语法错路径), *台])
    断言("ast 语法错 rc≠0", p.returncode != 0, f"rc={p.returncode}")
    p = 运行(cn, ["ast", str(非法), *台])
    断言("ast 语义错 rc=0（分层放行锚定）", p.returncode == 0, f"rc={p.returncode}")
    p = 运行(cn, ["token", str(词法错路径), *台])
    断言("token 词法错 rc≠0", p.returncode != 0, f"rc={p.returncode}")

    # ============ ② 退出码语义面 ============
    换维度("② 退出码语义面（透传/错误码/编译失败码）")
    if 运行级可达:
        for n in (1, 3, 5, 255):
            p = 运行(cn, ["run", str(返回码路径[n]), *台])
            断言(f"run 退出码透传 rc={n}（B4 锚定）", p.returncode == n, f"rc={p.returncode}（期望 {n}）")
        p = 运行(cn, ["run", str(除零), *台])
        断言("run 运行期错误码→rc=1", p.returncode == 1 and "错误码1" in 诊断(p), f"rc={p.returncode}")
    else:
        for n in (1, 3, 5, 255):
            跳过(f"run 退出码透传 rc={n}（B4 锚定）", "交叉视角产物非本机可执行形态")
        跳过("run 运行期错误码→rc=1", "交叉视角产物非本机可执行形态")

    # ============ ③ 选项面 ============
    换维度("③ 选项面（平台/优化/调试/输出/版本）")
    for o in (0, 1, 2, 3):
        p = 运行(cn, ["check", str(正常), f"-O{o}", *台])
        断言(f"-O{o} rc=0", p.returncode == 0, f"rc={p.returncode}")
    for opt in ("--opt", "--no-regalloc", "--debug", "--cfi", "--发布", "--release"):
        参数集 = ["check", str(正常), opt, "3", *台] if opt == "--opt" else ["check", str(正常), opt, *台]
        p = 运行(cn, 参数集)
        断言(f"{opt} rc=0", p.returncode == 0, f"rc={p.returncode}")

    # --验证-ir：build 正例（IR 生成后验证器跑优化前后两轮·253-a 自 check 格拆出——
    #   check 不触发 IR 生成，原格对验证器零触达）
    验证ir产物 = 仓库根 / "target" / "cli_contract_验证ir"
    if 运行级可达:
        p = 运行(cn, ["build", str(正常), "--验证-ir", "--output", str(验证ir产物), *台])
        断言("build --验证-ir rc=0（验证器优化前后两轮通过）", p.returncode == 0, f"rc={p.returncode}")
    else:
        跳过("build --验证-ir rc=0（验证器优化前后两轮通过）", "交叉视角产物非本机可执行形态")

    p = 运行(cn, ["check", str(正常), "--verbose", *台])
    断言("--verbose rc=0+附加输出", p.returncode == 0 and "命令:" in 诊断(p), f"rc={p.returncode}")

    指定输出 = 仓库根 / "target" / "cli_contract_指定.s"
    p = 运行(cn, ["compile", str(正常), "--output", str(指定输出), *台])
    断言("--output 指定路径+产物", p.returncode == 0 and 指定输出.exists(), f"rc={p.returncode}")
    深层输出 = 仓库根 / "target" / "cli_deep" / "a" / "b" / "深.s"
    p = 运行(cn, ["compile", str(正常), "--output", str(深层输出), *台])
    断言("--output 深层目录自动创建", p.returncode == 0 and 深层输出.exists(), f"rc={p.returncode}")

    for 交叉, 后缀 in (("win-x64", ".asm"), ("linux-x86_64", ".s")):
        if 交叉 == target:
            continue
        交产物 = 仓库根 / "target" / f"cli_contract_{交叉.replace('-', '_')}{后缀}"
        p = 运行(cn, ["compile", str(正常), "--target", 交叉, "--output", str(交产物)])
        断言(f"compile --target {交叉}（交叉产物）", p.returncode == 0 and 交产物.exists(),
             f"rc={p.returncode}")

    # -O3 SSA/Phi 降级覆盖触达格（256-a：ir 打印搬运名/交叉后端 emitCopy——
    #   红线要求新增代码行被门禁语料触达）
    p = 运行(cn, ["ir", str(汇合), "-O3"])
    断言("ir -O3 rc=0（SSA 降级指令打印）", p.returncode == 0, f"rc={p.returncode}")
    for 交叉, 后缀 in (("win-x64", ".asm"), ("linux-x86_64", ".s")):
        if 交叉 == target:
            continue
        交产物3 = 仓库根 / "target" / f"cli_contract_o3_{交叉.replace('-', '_')}{后缀}"
        p = 运行(cn, ["compile", str(汇合), "-O3", "--target", 交叉, "--output", str(交产物3)])
        断言(f"compile -O3 --target {交叉}（交叉后端 Copy 发射）",
             p.returncode == 0 and 交产物3.exists(), f"rc={p.returncode}")

    p = 运行(cn, ["--version"])
    断言("--version rc=0+版本串", p.returncode == 0 and "cn 0." in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["--help"])
    断言("--help rc=0+用法", p.returncode == 0 and "用法: cn" in 诊断(p), f"rc={p.returncode}")

    p = 运行(cn, ["check", str(正常), "--foo-bar", *台])
    断言("未知选项 rc≠0+诊断", p.returncode != 0 and "未知选项" in 诊断(p), f"rc={p.returncode}")

    # ============ ④ 命令行形态面 ============
    换维度("④ 命令行形态面（无参/未知命令/缺参/坏路径）")
    p = 运行(cn, [])
    断言("无参数 rc=0+用法", p.returncode == 0 and "用法: cn" in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["未知命令", str(正常)])
    断言("未知子命令 rc≠0+诊断", p.returncode != 0 and "未知命令" in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["check"])
    断言("缺文件参数 rc≠0+诊断", p.returncode != 0 and "缺少源文件参数" in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["check", str(探针 / "不存在.cn"), *台])
    断言("不存在文件 rc≠0+诊断", p.returncode != 0 and "错误" in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["check", str(目录探针), *台])
    断言("目录作为输入 rc≠0+诊断", p.returncode != 0 and "目录" in 诊断(p), f"rc={p.returncode}")
    p = 运行(cn, ["check", str(空文件), *台])
    断言("空文件 check rc=0（库组件口径放行）", p.returncode == 0, f"rc={p.returncode}")

    # ============ ⑤ 通道与诊断面 ============
    换维度("⑤ 通道与诊断面（stdout/stderr 分流+格式锚定）")
    p = 运行(cn, ["check", str(非法), *台])
    首个诊断行 = next((行 for 行 in p.stderr.splitlines() if 行.strip()), "")
    断言("诊断进 stderr 且格式=文件:行:列: 错误:",
         诊断格式.match(首个诊断行) is not None, f"首行={首个诊断行[:60]}")
    p = 运行(cn, ["check", str(正常), *台])
    断言("check 合法 stderr 无诊断", "错误" not in p.stderr, f"{len(p.stderr)}字节")
    if 运行级可达:
        p = 运行(cn, ["run", str(正常), *台])
        断言("run 程序输出→stdout（数=7）", "7" in p.stdout and "7" not in p.stderr, "stdout/stderr 分流")
        p = 运行(cn, ["run", str(非法), *台])
        断言("run 编译失败 stdout 无程序输出", "7" not in p.stdout, "分流")
    else:
        跳过("run 程序输出→stdout（数=7）", "交叉视角产物非本机可执行形态")
        跳过("run 编译失败 stdout 无程序输出", "交叉视角产物非本机可执行形态")

    失败 = [名 for 名, ok, _ in 格 if not ok]
    print(f"\nCLI 契约矩阵（检查网第 2 层·{target}）：{len(格) - len(失败)}/{len(格)} 格通过")
    if 失败:
        print("失败格：" + "；".join(失败))
    return 1 if 失败 else 0


if __name__ == "__main__":
    sys.exit(主流程())
