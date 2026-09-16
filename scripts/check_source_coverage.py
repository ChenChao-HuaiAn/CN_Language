#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""检查网第 3 层：编译器源码覆盖率全链门禁（plans/026 §2.8·250-a 建成）。

步骤：coverage 构建 → 清旧 gcda → 单测 → E2E（先清 v2p 指纹缓存=强制全树编译刺激）
     → CLI 契约矩阵 → gcov -b -i 聚合 → 报告 → 阈值门禁 →（可选）新代码覆盖红线。

口径（026 §2.8：本层=「编译器自身源码行/分支」覆盖，与支柱一语言行为面互补）：
  - 源码口径 = 编译器本体 src/cn_compiler/**.cpp + src/cn_main.cpp；
    src/runtime/** 由编译产物程序执行（非编译器执行面），不入口径；
  - 行覆盖   = 任一插桩对象（cn / cn_unit_tests）该行 count>0 即覆盖；
  - 分支覆盖 = branches[] 中 count>0 的分支 / 全部分支；
  - 阈值门禁 = 当前行/分支覆盖率 >= 基线 - 容差（基线按平台键控入库）；
  - 红线     = --diff-base REV 之后新增/修改的源码可行行必须全部被门禁语料触达。

win/MSVC 无 gcov 设施 → 不适用（026 §2.8 诚实边界：先跑 linux 口径）。
用法：
  python3 scripts/check_source_coverage.py                    # 全链（默认门禁态）
  python3 scripts/check_source_coverage.py --update-baseline  # 首跑/基线更新入库
  python3 scripts/check_source_coverage.py --skip-build --skip-e2e   # 仅重聚合已有数据
  python3 scripts/check_source_coverage.py --diff-base HEAD~1        # 新代码红线
"""

import argparse
import gzip
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
from pathlib import Path

项目根 = Path(__file__).resolve().parent.parent
构建目录 = 项目根 / "target" / "build-cov"
产物目录 = 项目根 / "target" / "cov"
报告目录 = 项目根 / "target" / "coverage"
基线文件 = 项目根 / "scripts" / "coverage_baseline.json"

# 源码口径：编译器本体（相对项目根的 posix 路径前缀/精确文件）
源码前缀 = ("src/cn_compiler/",)
源码精确 = ("src/cn_main.cpp",)

默认容差 = 0.5  # 百分点；覆盖劣化超过容差即红灯


def 运行(命令, cwd=None, 捕获=False):
    """运行命令；捕获态返回 (rc, 全部输出文本)。"""
    print(f"  $ {' '.join(str(c) for c in 命令)}", flush=True)
    if 捕获:
        r = subprocess.run([str(c) for c in 命令], cwd=cwd,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        return r.returncode, r.stdout
    r = subprocess.run([str(c) for c in 命令], cwd=cwd)
    return r.returncode, ""


def 探测e2e目标():
    """按机器架构映射 E2E --target（本机口径；可被参数覆盖）。"""
    m = platform.machine().lower()
    if m in ("aarch64", "arm64"):
        return "linux-arm64"
    if m in ("x86_64", "amd64"):
        return "linux-x86_64"
    sys.exit(f"[覆盖] 无法按架构 {m} 探测 E2E 目标平台，请用 --e2e-target 显式指定")


def gcc版本():
    rc, 输出 = 运行(["gcc", "-dumpfullversion"], 捕获=True)
    if rc != 0:
        rc, 输出 = 运行(["gcc", "-dumpversion"], 捕获=True)
    return 输出.strip().splitlines()[-1] if rc == 0 else "unknown"


def 步骤构建(并行):
    print("[覆盖 1/6] coverage 构建（GCC --coverage·零警告门禁）...", flush=True)
    rc, _ = 运行(["cmake", "-S", ".", "-B", 构建目录, "-DCN_ENABLE_COVERAGE=ON"])
    if rc != 0:
        sys.exit("[覆盖] cmake 配置失败")
    rc, 输出 = 运行(["cmake", "--build", 构建目录, "-j", 并行], 捕获=True)
    日志 = 报告目录 / "build.log"
    报告目录.mkdir(parents=True, exist_ok=True)
    日志.write_text(输出, encoding="utf-8")
    # 只匹配诊断形态「error:」/「warning:」（GCC 输出 file:line: error: msg）——
    #   防文件名误伤（ir_error_ctor.cpp 等含 "error" 子串·256-a 实测误报）
    n_bad = len(re.findall(r"(?:^|[\s:])error:|(?:^|[\s:])warning:", 输出, re.MULTILINE))
    if rc != 0:
        print(输出[-3000:], flush=True)
        sys.exit(f"[覆盖] 构建失败（rc={rc}·日志 {日志}）")
    if n_bad:
        sys.exit(f"[覆盖] 构建日志含 error/warning {n_bad} 处（零警告门禁）——见 {日志}")
    print(f"  构建零警告 ✓（日志 {日志}）", flush=True)


def 步骤清旧gcda():
    print("[覆盖] 清旧 gcda（口径=本轮门禁语料；构建前后各一次防时间戳冲突）...", flush=True)
    n = 0
    for p in 构建目录.rglob("*.gcda"):
        p.unlink()
        n += 1
    print(f"  已清 {n} 个 gcda", flush=True)


def 步骤单测():
    print("[覆盖 3/6] 单元测试（插桩版）...", flush=True)
    rc, 输出 = 运行([产物目录 / "cn_unit_tests"], 捕获=True)
    for 行文本 in 输出.splitlines():
        if "PASSED" in 行文本 or "FAILED" in 行文本:
            print("  " + 行文本.strip(), flush=True)
    if rc != 0:
        print(输出[-3000:], flush=True)
        sys.exit(f"[覆盖] 单测失败（rc={rc}）")


def 步骤e2e(e2e_target, jobs):
    print(f"[覆盖 4/6] 全量 E2E（--target {e2e_target} --jobs {jobs}·先清 v2p 指纹缓存强制全树编译刺激）...",
          flush=True)
    for p in (项目根 / "target" / "audit2").glob("v2p_*"):
        p.unlink()
        print(f"  清缓存 {p.name}", flush=True)
    rc, _ = 运行(["python3", "tests/e2e/run_e2e.py", "--cn", 产物目录 / "cn",
                  "--target", e2e_target, "--jobs", jobs])
    if rc != 0:
        sys.exit(f"[覆盖] E2E 失败（rc={rc}）")


def 步骤矩阵():
    print("[覆盖 5/6] CLI 契约矩阵（检查网第 2 层·同为覆盖语料）...", flush=True)
    rc, _ = 运行(["python3", "scripts/check_cli_contract.py", "--cn", 产物目录 / "cn"])
    if rc != 0:
        sys.exit(f"[覆盖] CLI 契约矩阵失败（rc={rc}）")


def 是编译器源码(相对路径):
    p = 相对路径.replace("\\", "/")
    return p.startswith(源码前缀) or p in 源码精确


def 聚合覆盖数据():
    """扫 build-cov 全部 gcda → gcov -b -i → 按源文件聚合行/分支计数。

    返回 (聚合, gcov告警列表)。聚合={源相对路径: {"可行行": {行号: 最大count},
    "分支": {(行号,序): 最大count}}}（同源文件被多个插桩对象链接 → count 取 max
    = OR 语义）。gcov 告警（文件损坏/stamp mismatch）不致命但必须记录——红线判据
    依赖「数据与当前源码同步」，存在告警时红线拒判（防失配假绿）。
    """
    gcda列表 = sorted(构建目录.rglob("*.gcda"))
    if not gcda列表:
        sys.exit("[覆盖] build-cov 下无 gcda——先跑门禁语料（勿 --skip 全部语料步）")
    聚合 = {}
    告警 = []
    # 失配守卫：gcda 早于同位 gcno ⇒ gcno 在语料之后重新生成过（gcov 会静默弃用
    # 旧 gcda 数据=该文件覆盖归 0）——数据过期，红线必须拒判（250-a 反证②实证）
    for gcda in gcda列表:
        gcno = gcda.with_suffix(".gcno")
        if gcno.exists() and gcda.stat().st_mtime < gcno.stat().st_mtime:
            rel = os.path.relpath(gcda, 构建目录)
            告警.append(f"gcda 早于 gcno（构建后未重跑语料·数据过期）: {rel}")
    if 告警:
        for x in 告警[:5]:
            print(f"  [失配] {x}", flush=True)
    with tempfile.TemporaryDirectory() as 临时:
        for i, gcda in enumerate(gcda列表):
            rc, 输出 = 运行(["gcov", "-b", "-i", str(gcda)], cwd=临时, 捕获=True)
            gz列表 = list(Path(临时).glob("*.gcov.json.gz"))
            if rc != 0 or not gz列表:
                # 单文件损坏（如并行写竞争/stamp mismatch）不致命：告警并跳过
                告警.append(f"{gcda.name}: rc={rc} {输出.strip().splitlines()[-1:] if 输出.strip() else ''}")
                print(f"  [告警] gcov 失败跳过 {gcda.name}（红线判据将拒判）", flush=True)
                for g in gz列表:
                    g.unlink()
                continue
            数据 = {"files": []}
            for g in gz列表:
                with gzip.open(g, "rt", encoding="utf-8") as f:
                    内容 = json.load(f)
                数据["files"].extend(内容.get("files", []))
                g.unlink()
            for 文件 in 数据.get("files", []):
                src = 文件.get("file", "")
                rel = os.path.relpath(src, 项目根) if os.path.isabs(src) else src
                if not 是编译器源码(rel):
                    continue
                表 = 聚合.setdefault(rel, {"可行行": {}, "分支": {}})
                for 行 in 文件.get("lines", []):
                    ln = 行["line_number"]
                    表["可行行"][ln] = max(表["可行行"].get(ln, 0), 行.get("count", 0))
                    for 序, br in enumerate(行.get("branches", [])):
                        键 = (ln, 序)
                        表["分支"][键] = max(表["分支"].get(键, 0), br.get("count", 0))
            if (i + 1) % 50 == 0:
                print(f"  gcov 进度 {i+1}/{len(gcda列表)}", flush=True)
    return 聚合, 告警


def 汇总统计(聚合):
    文件统计 = []
    总可行 = 总覆盖 = 总分支 = 总分支覆盖 = 0
    for rel, 表 in sorted(聚合.items()):
        可行 = 表["可行行"]
        行覆盖数 = sum(1 for c in 可行.values() if c > 0)
        b总 = len(表["分支"])
        b覆盖 = sum(1 for c in 表["分支"].values() if c > 0)
        总可行 += len(可行)
        总覆盖 += 行覆盖数
        总分支 += b总
        总分支覆盖 += b覆盖
        文件统计.append({
            "file": rel,
            "可行行": len(可行), "覆盖行": 行覆盖数,
            "行覆盖%": round(100.0 * 行覆盖数 / len(可行), 2) if 可行 else 100.0,
            "分支总数": b总, "分支覆盖数": b覆盖,
            "分支覆盖%": round(100.0 * b覆盖 / b总, 2) if b总 else None,
        })
    行pct = round(100.0 * 总覆盖 / 总可行, 2) if 总可行 else 0.0
    分支pct = round(100.0 * 总分支覆盖 / 总分支, 2) if 总分支 else 0.0
    return 文件统计, 行pct, 分支pct


def 写报告(文件统计, 行pct, 分支pct, 平台键):
    报告目录.mkdir(parents=True, exist_ok=True)
    低覆盖 = sorted(文件统计, key=lambda x: x["行覆盖%"])[:15]
    md = ["# 编译器源码覆盖率报告（检查网第 3 层·plans/026 §2.8）", "",
          f"- 平台键：`{平台键}`",
          f"- 源码口径：src/cn_compiler/** + src/cn_main.cpp（{len(文件统计)} 个编译单元源文件）",
          f"- **行覆盖：{行pct}%**（可行行口径）/ **分支覆盖：{分支pct}%**",
          "", "## 低覆盖 top（行覆盖% 最低 15 文件）", "",
          "| 文件 | 行覆盖% | 覆盖行/可行行 | 分支覆盖% |", "|---|---|---|---|"]
    for x in 低覆盖:
        md.append(f"| {x['file']} | {x['行覆盖%']} | {x['覆盖行']}/{x['可行行']} | "
                  f"{x['分支覆盖%'] if x['分支覆盖%'] is not None else '—'} |")
    (报告目录 / "report.md").write_text("\n".join(md) + "\n", encoding="utf-8")
    (报告目录 / "report.json").write_text(json.dumps({
        "平台键": 平台键, "行覆盖%": 行pct, "分支覆盖%": 分支pct,
        "文件数": len(文件统计), "文件明细": 文件统计}, ensure_ascii=False, indent=1),
        encoding="utf-8")
    print(f"  报告：{报告目录/'report.md'} + report.json", flush=True)


def 读取基线():
    if not 基线文件.exists():
        print(f"[覆盖] 基线不存在（{基线文件}）——首跑请加 --update-baseline 生成后入库",
              flush=True)
        sys.exit(2)
    return json.loads(基线文件.read_text(encoding="utf-8"))


def 步骤门禁与基线(文件统计, 行pct, 分支pct, 平台键, 更新基线, 容差):
    print(f"[覆盖 6/6] 阈值门禁（行 {行pct}% / 分支 {分支pct}%·容差 {容差}pp·基线平台键 {平台键}）...",
          flush=True)
    if 更新基线:
        基线 = 读取基线() if 基线文件.exists() else {"说明":
            "检查网第 3 层覆盖率基线（平台键控·防劣化门禁；更新用 --update-baseline）",
            "platforms": {}}
        基线["platforms"][平台键] = {"行覆盖%": 行pct, "分支覆盖%": 分支pct,
                                  "文件数": len(文件统计)}
        基线文件.write_text(json.dumps(基线, ensure_ascii=False, indent=1) + "\n",
                            encoding="utf-8")
        print(f"  基线已更新入库：{基线文件}", flush=True)
        return
    基线 = 读取基线()
    ref = 基线["platforms"].get(平台键)
    if not ref:
        print(f"[覆盖] 本平台（{平台键}）无基线——首跑请加 --update-baseline 生成后入库",
              flush=True)
        sys.exit(2)
    for 名, 当前, 基准 in (("行覆盖", 行pct, ref["行覆盖%"]),
                        ("分支覆盖", 分支pct, ref["分支覆盖%"])):
        if 当前 < 基准 - 容差:
            sys.exit(f"[覆盖] 门禁 FAIL：{名} {当前}% < 基线 {基准}% - 容差 {容差}pp"
                     f"（覆盖劣化=测试语料触达面收缩，先查语料/实现变化）")
    print(f"  门禁 PASS ✓（行 {行pct}%≥{ref['行覆盖%'] - 容差}·"
          f"分支 {分支pct}%≥{ref['分支覆盖%'] - 容差}）", flush=True)


def 步骤红线(diff_base, 复用聚合=None, 复用告警=None):
    """新代码覆盖红线：diff_base 之后新增/修改的源码可行行必须被门禁语料触达。

    防假绿守卫：①gcov 告警（stamp mismatch/损坏=数据与源码失配）→ 拒判 FAIL；
    ②diff 涉及文件完全无覆盖数据 → 视为未触达 FAIL（没被构建=没被测试）。
    """
    print(f"[红线] 新代码覆盖红线（--diff-base {diff_base}）...", flush=True)
    rc, diff = 运行(["git", "diff", "--unified=0", diff_base, "--",
                     "src/cn_compiler", "src/cn_main.cpp"], cwd=项目根, 捕获=True)
    if rc != 0:
        sys.exit(f"[红线] git diff 失败（rc={rc}）")
    if 复用聚合 is None:
        聚合, 告警 = 聚合覆盖数据()
    else:
        聚合, 告警 = 复用聚合, 复用告警
    if 告警:
        for x in 告警[:5]:
            print(f"  [失配] {x}", flush=True)
        sys.exit(f"[红线] FAIL：gcov 覆盖数据存在 {len(告警)} 处告警（与当前源码失配）"
                 f"——必须全量重建+重跑门禁语料后再判红线")
    当前文件 = None
    新行 = 0
    未触达 = []
    hunk头 = re.compile(r"^\+\+\+\s+b/(.+)$")
    hunk区间 = re.compile(r"^@@\s+-\d+(?:,\d+)?\s+\+(\d+)(?:,(\d+))?\s+@@")
    for 行文本 in diff.splitlines():
        m = hunk头.match(行文本)
        if m:
            当前文件 = m.group(1)
            continue
        m = hunk区间.match(行文本)
        if m:
            当前区间起始 = int(m.group(1))
            当前区间偏移 = 0
            continue
        if 当前文件 is None or not 行文本.startswith("+") or 行文本.startswith("+++"):
            continue
        # unified=0：+行按出现序自区间起始递增
        行号 = 当前区间起始 + 当前区间偏移
        当前区间偏移 += 1
        新行 += 1
        表 = 聚合.get(当前文件)
        if 表 is None:
            未触达.append(f"{当前文件}:{行号}: [文件无覆盖数据] {行文本[1:].strip()[:80]}")
            continue
        if 行号 in 表["可行行"] and 表["可行行"][行号] == 0:
            未触达.append(f"{当前文件}:{行号}: {行文本[1:].strip()[:80]}")
    if 未触达:
        print(f"[红线] FAIL：{len(未触达)}/{新行} 个新增可行行未被门禁语料触达：", flush=True)
        for x in 未触达[:30]:
            print(f"  {x}", flush=True)
        sys.exit(1)
    print(f"[红线] PASS ✓（新增可行行 {新行} 行全部触达）", flush=True)


def main():
    ap = argparse.ArgumentParser(description="检查网第 3 层：编译器源码覆盖率全链门禁")
    ap.add_argument("--skip-build", action="store_true", help="复用已有 coverage 构建")
    ap.add_argument("--skip-unit", action="store_true", help="跳过单测")
    ap.add_argument("--skip-e2e", action="store_true", help="跳过全量 E2E")
    ap.add_argument("--skip-matrix", action="store_true", help="跳过 CLI 契约矩阵")
    ap.add_argument("--update-baseline", action="store_true", help="本次结果写入基线入库")
    ap.add_argument("--diff-base", metavar="REV", help="新代码红线基线 revision（git diff 基）")
    ap.add_argument("--jobs", type=int, default=4, help="E2E 并行度（默认 4）")
    ap.add_argument("--e2e-target", default=None, help="E2E 目标平台（默认按架构探测）")
    ap.add_argument("--tolerance", type=float, default=默认容差, help="阈值容差（百分点）")
    参数 = ap.parse_args()

    if os.name == "nt":
        sys.exit("[覆盖] win/MSVC 无 gcov 设施——本门禁 linux 口径（plans/026 §2.8 诚实边界）")

    e2e_target = 参数.e2e_target or 探测e2e目标()
    平台键 = f"{e2e_target}/gcc-{gcc版本()}"
    print(f"[覆盖] 检查网第 3 层全链（平台键 {平台键}·源码口径=编译器本体）", flush=True)

    if not 参数.skip_build:
        # 构建前清残留 gcda：源码变更后旧 gcda 与新 gcno 时间戳冲突会经 libgcov
        #   在构建期（try_compile 等）打印 profiling error 误触零警告门禁（255-a 实测）
        步骤清旧gcda()
        步骤构建(参数.jobs)
    # 清旧 gcda 仅在语料步会执行时有意义（口径=本轮语料）；全跳语料=复用已有数据重聚合
    if not (参数.skip_unit and 参数.skip_e2e and 参数.skip_matrix):
        步骤清旧gcda()
    if not 参数.skip_unit:
        步骤单测()
    if not 参数.skip_e2e:
        步骤e2e(e2e_target, 参数.jobs)
    if not 参数.skip_matrix:
        步骤矩阵()

    print(f"[聚合] gcov -b -i 扫描 → 按源文件聚合...", flush=True)
    聚合, 覆盖告警 = 聚合覆盖数据()
    文件统计, 行pct, 分支pct = 汇总统计(聚合)
    print(f"  行覆盖 {行pct}%（{len(文件统计)} 源文件）·分支覆盖 {分支pct}%", flush=True)
    写报告(文件统计, 行pct, 分支pct, 平台键)
    步骤门禁与基线(文件统计, 行pct, 分支pct, 平台键, 参数.update_baseline, 参数.tolerance)
    if 参数.diff_base:
        步骤红线(参数.diff_base, 复用聚合=聚合, 复用告警=覆盖告警)
    print("[覆盖] 全链通过 ✓", flush=True)


if __name__ == "__main__":
    main()
