#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""M6 自举固定点机械触发器（任务 #1221·2026-10-23）。

背景（#1221 覆盖对账结论）：M6=79_v2 自举闭环（fix_p ≡ fix_s 逐字节），此前只有
「云端全量每轮都跑、本地/CI 调用面靠自觉」：纯文档/前端轮白付 ~20 分钟重负载是
浪费面；反向更危险——写集动了 codegen/opt/ir 时，若本轮落在不能原生跑 M6 的平台
（交叉模拟 arm64 的 78/79 登记跳过）或调用面未开重负载（integrate 本地全量经
ci.ps1/run_e2e 默认跳过 78/79·GitHub Actions 工作流同样未带 --heavy-ci），79_v2
被计入「跳过」而轮次照绿=流程自觉而非机械保证。本模块把「动 codegen/opt/ir ⇒
本轮必须跑 M6」做成单点判定；执行面复用 run_e2e 79_v2（--require-m6 跳过即红 +
md5/首个差异字节审计记录）。

判定口径（结构性·非启发式）：写集路径前缀命中下列任一面即触发——
  宿主：src/cn_compiler/codegen/、src/cn_compiler/opt/、src/cn_compiler/ir/
  v2  ：CN语言编译器v2/代码生成/、CN语言编译器v2/IR/
纯前端（lexer/parser/semantic 不含 IR）、文档、脚本、tests 一律豁免（M6 是产物
逐字节面，前端改动由既有双编译/闭环用例承担；语义改动的 IR 产物变化经 ir/ 面命中）。

纯函数面（--selftest 正反例·零 git 零构建）：
  判定写集(文件们)             → {"触发", "命中", "说明"}
  平台可跑M6(平台, 交叉模拟)    → 本平台能否原生跑 M6（交叉模拟 arm64=否）
  判定轮(文件们, 平台, 交叉模拟) → 追加 {"平台可跑", "处置"}；处置∈未触发|必跑|未跑-转派
  误放行检查(点名清单)          → --allow-known-red/--allow-pool-red 点名含 79_v2=拒
  摘要(计划)                   → 摘要行文本（M6 未跑时必须显式原因）

纪律（miss 不静默）：写集不可判（git 异常/空清单歧义）由调用方按 fail-safe 触发
（多付一次 M6 成本优于漏验）；平台不可跑时 run_e2e 摘要与 ci_daemon 轮记录必须显式
「M6 未跑（原因）」，预验轮直接判红（fail-closed），develop 轮由平台轮转承担并在
结果里留「转派」原因。
"""
from __future__ import annotations

import subprocess
import sys

# M6 用例（tests/e2e/79_v2_自举闭环·run_e2e 特殊编排执行v2锚定链）
M6用例名 = "79_v2_自举闭环"
M6用例关键字 = "79_v2"

# 写集触发前缀（守卫=产物语义：代码生成/优化/IR 变化才可能改变 fix_p/fix_s 逐字节面）
M6写集前缀 = (
    "src/cn_compiler/codegen/",
    "src/cn_compiler/opt/",
    "src/cn_compiler/ir/",
    "CN语言编译器v2/代码生成/",
    "CN语言编译器v2/IR/",
)

# 可原生跑 M6 的平台（三平台后端均有 as/g++ 或 ml64/link 闭环编排）；arm64 需真机
M6原生平台 = ("win-x64", "linux-arm64", "linux-x86_64")


def _归一(路径: str) -> str:
    """写集路径归一：反斜杠→斜杠、去前导 ./——git 输出跨平台一致消费（586-a 同族：
    路径形态差异不得令前缀匹配静默失效）。只动前缀，不 strip 文件名自身字符。"""
    文本 = (路径 or "").replace("\\", "/")
    while 文本.startswith("./"):
        文本 = 文本[2:]
    return 文本


def 判定写集(文件们) -> dict:
    """纯函数：改动文件清单 → M6 触发判定（前缀精确匹配·含目录本身形态）。

    返回 {"触发": bool, "命中": [路径...], "说明": str}。空清单/None=不触发（调用方
    对「写集不可判」另行 fail-safe 触发，见模块头纪律段——本函数不把空当未知）。
    """
    命中 = set()
    for 原路径 in 文件们 or []:
        路径 = _归一(原路径)
        if not 路径:
            continue
        for 前缀 in M6写集前缀:
            if 路径 == 前缀.rstrip("/") or 路径.startswith(前缀):
                命中.add(路径)
                break
    命中们 = sorted(命中)
    if 命中们:
        说明 = "写集命中 %d 文件（codegen/opt/ir）" % len(命中们)
    else:
        说明 = "写集不含 codegen/opt/ir——M6 豁免"
    return {"触发": bool(命中们), "命中": 命中们, "说明": 说明}


def 平台可跑M6(平台: str, 交叉模拟: bool = False) -> bool:
    """本平台能否**原生**跑 M6：win-x64/linux-x86_64 恒可；linux-arm64 仅真机可
    （交叉模拟=x86_64 宿主 qemu 跑 arm64 ELF，78/79 登记跳过=数小时性能深坑）。"""
    if 平台 not in M6原生平台:
        return False
    if 平台 == "linux-arm64":
        return not 交叉模拟
    return True


def 判定轮(文件们, 平台: str, 交叉模拟: bool = False) -> dict:
    """纯函数：一轮（develop/预验/兜底）的 M6 计划。

    处置三态：
      未触发   = 写集不含 codegen/opt/ir（纯文档/脚本/前端轮零成本）
      必跑     = 触发且本平台可原生跑（执行面须 --require-m6：跳过即红）
      未跑-转派 = 触发但本平台不可原生跑（预验轮判红；develop 轮由平台轮转承担）
    """
    写集 = 判定写集(文件们)
    可跑 = 平台可跑M6(平台, 交叉模拟)
    计划 = dict(写集)
    计划["平台"] = 平台
    计划["交叉模拟"] = bool(交叉模拟)
    计划["平台可跑"] = 可跑
    if not 写集["触发"]:
        计划["处置"] = "未触发"
    elif 可跑:
        计划["处置"] = "必跑"
    else:
        计划["处置"] = "未跑-转派"
    计划["说明"] = 摘要(计划)
    return 计划


def 判定未知写集(原因: str, 平台: str, 交叉模拟: bool = False) -> dict:
    """写集不可判（git 异常/基线缺失）=fail-safe 触发——多付一次 M6 成本优于漏验。

    处置仍按平台能力分流（必跑/未跑-转派），并显式携带「写集不可判」+原因。"""
    计划 = 判定轮([M6写集前缀[0] + "<写集不可判>"], 平台, 交叉模拟)
    计划["写集不可判"] = True
    计划["写集原因"] = 原因
    计划["说明"] = "M6 fail-safe 触发（写集不可判：%s）" % 原因
    return 计划


def 摘要(计划: dict) -> str:
    """计划→单行摘要（M6 未跑必须带原因——静默豁免即缺陷）。"""
    处置 = (计划 or {}).get("处置")
    if 处置 == "未触发":
        return "M6 未触发（写集不含 codegen/opt/ir）"
    if 处置 == "必跑":
        return ("M6 必跑（写集命中 %d 文件：%s）"
                % (len(计划.get("命中") or []),
                   "、".join((计划.get("命中") or [])[:3])
                   + ("…" if len(计划.get("命中") or []) > 3 else "")))
    if 处置 == "未跑-转派":
        return ("M6 未跑（%s 交叉模拟无法原生跑 79_v2——由 win/linux-x64/arm64 真机轮承担）"
                % 计划.get("平台", "?"))
    return "M6 计划缺失（处置=%r）——按未跑处置" % (处置,)


def 误放行检查(点名清单) -> str | None:
    """M6 红禁放行：--allow-known-red/--allow-pool-red 点名含 79_v2 即拒。

    返回 None=点名清单不含 M6；非 None=拒绝原因。调用方在参数解析后与放行消费点
    双挂（单一事实源本函数）。"""
    违规 = [n for n in (点名清单 or []) if M6用例关键字 in str(n)]
    if 违规:
        return ("M6（%s·自举固定点）红不得以 --allow-known-red/--allow-pool-red 放行"
                "——点名 %s；请修复 codegen/opt/ir 后重跑（#1221 机械保证）。"
                % (M6用例名, "、".join(违规)))
    return None


def 红例含M6(红例们) -> bool:
    """失败/红用例清单是否含 M6（79_v2）——放行消费点硬拦判据（点名说辞不可绕过）。"""
    return any(M6用例关键字 in str(n) for n in (红例们 or []))


def _自测() -> int:
    """--selftest：纯函数正反例（零 git 零构建·CI 式）。"""
    断言们: list[tuple[str, bool]] = []

    def 断言(说明: str, 条件: bool) -> None:
        断言们.append((说明, bool(条件)))

    # ── 判定写集：五面前缀正例 ──
    正例 = [
        "src/cn_compiler/codegen/x64/指令发射.cpp",
        "src/cn_compiler/codegen/arm64/后端.cpp",
        "src/cn_compiler/opt/窥孔.cpp",
        "src/cn_compiler/ir/ir.cpp",
        "CN语言编译器v2/代码生成/代码生成ARM64.cn",
        "CN语言编译器v2/IR/优化.cn",
    ]
    for 路径 in 正例:
        断 = 判定写集([路径])
        断言("正例触发：%s" % 路径, 断["触发"] and 断["命中"] == [路径])

    # ── 判定写集：纯前端/文档/脚本/测试/相似前缀反例 ──
    反例 = [
        "src/cn_compiler/lexer/lexer.cpp",
        "src/cn_compiler/parser/parser.cpp",
        "src/cn_compiler/semantic/checker.cpp",
        "src/cn_compiler/driver/main.cpp",
        "src/cn_compiler/irx/不是IR目录.cpp",      # 前缀边界：ir/ ≠ irx/
        "src/cn_compiler/codegenx/a.cpp",          # 前缀边界：codegen/ ≠ codegenx/
        "CN语言编译器v2/词法/词法.cn",
        "CN语言编译器v2/语法/语法.cn",
        "CN语言编译器v2/语义/语义.cn",
        "CN语言编译器v2/主.cn",
        "docs/README.md",
        "scripts/m6_gate.py",
        "tests/e2e/79_v2_自举闭环/v2锚定链.txt",
    ]
    for 路径 in 反例:
        断 = 判定写集([路径])
        断言("反例豁免：%s" % 路径, not 断["触发"] and 断["命中"] == [])

    # ── 混合写集：任一命中即触发；命中清单去重排序 ──
    混合 = 判定写集(["docs/x.md",
                    "src/cn_compiler/opt/窥孔.cpp",
                    "src/cn_compiler/opt/窥孔.cpp",
                    "src/cn_compiler/lexer/lexer.cpp"])
    断言("混合写集：任一命中即触发且去重", 混合["触发"] and 混合["命中"] == ["src/cn_compiler/opt/窥孔.cpp"])

    # ── 路径归一：反斜杠/前导 ./ 形态仍命中（586-a 同族）──
    断言("路径归一：反斜杠写集命中",
         判定写集(["src\\cn_compiler\\ir\\ir.cpp"])["触发"])
    断言("路径归一：前导 ./ 写集命中",
         判定写集(["./src/cn_compiler/codegen/x64/a.cpp"])["触发"])
    断言("空写集=不触发（未知由调用方 fail-safe 另判）", not 判定写集([])["触发"])
    断言("None 写集=不触发不炸", not 判定写集(None)["触发"])

    # ── 平台能力 ──
    断言("win-x64 原生可跑", 平台可跑M6("win-x64"))
    断言("linux-x86_64 原生可跑", 平台可跑M6("linux-x86_64"))
    断言("linux-arm64 真机可跑", 平台可跑M6("linux-arm64", 交叉模拟=False))
    断言("linux-arm64 交叉模拟不可跑", not 平台可跑M6("linux-arm64", 交叉模拟=True))
    断言("未知平台不可跑（fail-closed）", not 平台可跑M6("darwin-arm64"))

    # ── 判定轮三态 ──
    码 = ["src/cn_compiler/codegen/x64/a.cpp"]
    未触发 = 判定轮(["docs/a.md"], "win-x64")
    必跑 = 判定轮(码, "win-x64")
    转派 = 判定轮(码, "linux-arm64", 交叉模拟=True)
    断言("轮三态：未触发", 未触发["处置"] == "未触发" and "M6 未触发" in 未触发["说明"])
    断言("轮三态：必跑", 必跑["处置"] == "必跑" and 必跑["平台可跑"]
         and "M6 必跑" in 必跑["说明"] and "codegen" in 必跑["说明"])
    断言("轮三态：交叉模拟转派", 转派["处置"] == "未跑-转派" and not 转派["平台可跑"]
         and "M6 未跑" in 转派["说明"] and "交叉模拟" in 转派["说明"])
    未知 = 判定未知写集("git diff 失败", "win-x64")
    断言("未知写集=fail-safe 触发（不可判不漏验）",
         未知["触发"] and 未知["处置"] == "必跑" and 未知.get("写集不可判")
         and "fail-safe" in 未知["说明"])
    未知转派 = 判定未知写集("基线缺失", "linux-arm64", 交叉模拟=True)
    断言("未知写集+交叉模拟=转派（处置按平台能力分流）",
         未知转派["触发"] and 未知转派["处置"] == "未跑-转派")

    # ── M6 红禁放行（点名清单）──
    断言("点名清单含 79_v2=拒", 误放行检查(["79_v2_自举闭环"]) is not None)
    断言("点名清单含 79_v2 变体=拒", 误放行检查(["前缀-79_v2-后缀"]) is not None)
    断言("点名清单不含 M6=放行", 误放行检查(["113_某用例", "127_v2_某闭环"]) is None)
    断言("空点名清单=放行", 误放行检查([]) is None and 误放行检查(None) is None)
    断言("红例含 M6=硬拦", 红例含M6(["79_v2_自举闭环"]) and 红例含M6(["79_v2_自举闭环: 固定点不一致"]))
    断言("红例不含 M6=不拦", not 红例含M6(["78_v2_自举链构建"]) and not 红例含M6([]))

    失败 = 0
    for 说明, 通过 in 断言们:
        print("  [%s] %s" % ("✓" if 通过 else "✗", 说明))
        if not 通过:
            失败 += 1
    print("\n[m6_gate selftest] %d 例·失败 %d" % (len(断言们), 失败))
    return 1 if 失败 else 0


def _命令行(argv: list[str]) -> int:
    """非纯函数胶水面：--changed-since <rev>（GitHub Actions 等不可改命令行调用面）。

    git diff --name-only <rev>..HEAD → 判定写集 → 触发时输出 CN_M6_REQUIRE=1
    （配 --gh-env 写入 $GITHUB_ENV 形态）。git 异常=fail-safe 触发（不漏验）；
    退出码恒 0（触发是信号而非失败——由 E2E 的 --require-m6 语义判红）。"""
    if "--changed-since" in argv:
        i = argv.index("--changed-since")
        基线 = argv[i + 1] if i + 1 < len(argv) else ""
        gh_env = "--gh-env" in argv
        if not 基线:
            print("错误: --changed-since 需要基线 rev", file=sys.stderr)
            return 2
        try:
            结果 = subprocess.run(
                ["git", "-c", "core.quotepath=false", "diff", "--name-only",
                 "%s..HEAD" % 基线],
                capture_output=True, text=True, timeout=120)
            文件们 = ([行.strip().strip('"') for 行 in (结果.stdout or "").splitlines()
                      if 行.strip()] if 结果.returncode == 0 else None)
            原因 = "" if 结果.returncode == 0 else (结果.stderr or "").strip()[:160]
        except Exception as 异常:                 # 超时/OSError 同样不可判
            文件们, 原因 = None, repr(异常)
        if 文件们 is None:
            print("M6 fail-safe 触发（写集不可判：%s）" % 原因)
            触发 = True
        else:
            判定 = 判定写集(文件们)
            触发 = 判定["触发"]
            print(判定["说明"])
        if 触发 and gh_env:
            print("CN_M6_REQUIRE=1")
        return 0
    print("用法: python scripts/m6_gate.py --selftest | "
          "--changed-since <rev> [--gh-env]")
    return 0


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        sys.exit(_自测())
    sys.exit(_命令行(sys.argv[1:]))
