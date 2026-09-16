#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""D31 门禁（258-a）：arm64 / linux_x64 汇编位宽与完备性全库扫描。

双维铁律（D31 方案C④·机械可证「绝不产非法/缺位发射」）：
  1. arm64：movz/movk 作用于 wN（32 位）只允许 lsl #0/#16 ——
     `movk wN, #imm, lsl #32/48` 为非法编码（GNU as 拒绝 = 编译失败级，plans/021 D31）。
  2. linux_x64：dispatch 对未实现 opcode 落 default 只出「未支持操作码」注释
     = 静默丢值（258-a 实证 Copy 曾漏发射——比汇编失败更危险），指纹零命中。

流程：tests/e2e 宿主通道全量用例经 `compile --target <后端>` 出 .s（只产汇编、
不调 as —— 任意平台可跑），逐文件扫描。负例（期望编译失败/期望check失败）与
v2 闭环用例（v2p 工具链通道）不在扫描面。旗标通道「编译选项.txt」与 runner 同口径。
自带正/负样本自检（探测器自身也是被测对象——110-a 教训）。

用法：python scripts/check_asm_width.py [--cn target/Debug/cn.exe]
退出码：0 = 全过；1 = 有违例或编译失败。
"""
import argparse
import io
import pathlib
import re
import subprocess
import sys
import tempfile
import time

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# 与 runner 平台跳过口径一致（62_ffi 依赖 Windows API；69 运行时计数平台差异）
SKIP_DIRS = {"62_ffi", "69_memory_management"}

# arm64 非法编码：movz/movk + wN + lsl #32/#48（w 寄存器只允许 lsl #0/#16）
ARM64_BAD_ENC = re.compile(r"\bmov[kz]\s+w\d+\s*,\s*#\d+\s*,\s*lsl\s*#(?:32|48)\b")

# linux_x64 未支持 opcode 指纹（dispatch default 只出注释 = 静默丢值）
LX64_SILENT_DROP = re.compile(r"未支持操作码")

# ---- 自检样本（探测器自身也是被测对象——110-a 教训）----
SELF_ARM64_LEGAL = (
    "movz w9, #61952\n"
    "movk w9, #10757, lsl #16\n"    # wN lsl#16 合法
    "movk x10, #1, lsl #32\n"       # xN lsl#32 合法（64 位寄存器）
    "movk x10, #65535, lsl #48\n"   # xN lsl#48 合法
)
SELF_ARM64_ILLEGAL = (
    "movk w9, #1, lsl #32\n"        # 非法：wN lsl#32
    "movz w10, #65535, lsl #48\n"   # 非法：wN lsl#48
)
SELF_SILENT_SAMPLE = "    // 未支持操作码\n"


def self_check() -> None:
    if ARM64_BAD_ENC.search(SELF_ARM64_LEGAL):
        raise SystemExit("自检失败：arm64 合法样本被误判")
    if len(ARM64_BAD_ENC.findall(SELF_ARM64_ILLEGAL)) != 2:
        raise SystemExit("自检失败：arm64 非法样本漏判")
    if not LX64_SILENT_DROP.search(SELF_SILENT_SAMPLE):
        raise SystemExit("自检失败：linux_x64 指纹漏判")


def read_case_flags(case_dir: pathlib.Path) -> list:
    """用例级编译旗标（runner「编译选项.txt」通道同口径）。"""
    marker = case_dir / "编译选项.txt"
    if not marker.exists():
        return []
    return [ln.strip() for ln in marker.read_text(encoding="utf-8-sig").splitlines()
            if ln.strip()]


def collect_cases(root: pathlib.Path) -> list:
    cases = []
    for d in sorted(root.iterdir()):
        if not d.is_dir() or d.name.startswith("__"):
            continue
        if d.name in SKIP_DIRS:
            continue
        # 负例（预期编译失败，与 runner 口径同源）不在产物合法性扫描面
        if (d / "期望编译失败.txt").exists() or (d / "期望check失败.txt").exists():
            continue
        # v2 自举闭环用例：编译器 = v2p 工具链而非宿主，宿主诊断失败是常态面
        if (d / "v2闭环.txt").exists():
            continue
        src = d / "主.cn"
        if not src.exists():
            cn_srcs = sorted(d.glob("*.cn"))
            if not cn_srcs:
                continue
            src = cn_srcs[0]
        cases.append((d.name, src))
    return cases


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cn", default="target/Debug/cn.exe")
    ap.add_argument("--e2e", default="tests/e2e")
    args = ap.parse_args()

    self_check()
    cn = pathlib.Path(args.cn)
    if not cn.exists():
        print(f"未找到编译器: {cn}")
        return 1

    cases = collect_cases(pathlib.Path(args.e2e))
    compile_fails = []   # (用例名[后端], 消息)
    arm64_hits = []      # (用例名, 行号, 片段)
    silent_hits = []     # (用例名, 行号, 片段)
    backends = (("linux-arm64", ARM64_BAD_ENC, arm64_hits),
                ("linux-x86_64", LX64_SILENT_DROP, silent_hits))
    t0 = time.time()
    for name, src in cases:
        for target, pattern, bucket in backends:
            with tempfile.TemporaryDirectory() as td:
                out_s = pathlib.Path(td) / "out.s"
                cmd = [str(cn), "compile", str(src), "--target", target,
                       "--output", str(out_s)]
                cmd += read_case_flags(src.parent)
                r = subprocess.run(cmd, capture_output=True, timeout=120)
                if r.returncode != 0 or not out_s.exists():
                    msg = r.stderr.decode("utf-8", "replace").strip().splitlines()
                    compile_fails.append((f"{name}[{target}]",
                                          msg[-1] if msg else "无 .s 产出"))
                    continue
                text = out_s.read_text(encoding="utf-8", errors="replace")
                for m in pattern.finditer(text):
                    lineno = text.count("\n", 0, m.start()) + 1
                    bucket.append((name, lineno, m.group(0).strip()))

    print("== check_asm_width（D31·258-a）==")
    print(f"用例 {len(cases)}×2 后端 ｜ 编译失败 {len(compile_fails)}"
          f" ｜ arm64 非法编码 {len(arm64_hits)}"
          f" ｜ linux_x64 静默丢值 {len(silent_hits)}"
          f" ｜ 耗时 {time.time()-t0:.1f}s")
    for name, lineno, frag in arm64_hits[:20]:
        print(f"  ✗ arm64 {name}:{lineno}: {frag}")
    for name, lineno, frag in silent_hits[:20]:
        print(f"  ✗ linux_x64 静默丢值 {name}:{lineno}: {frag}")
    for name, msg in compile_fails[:20]:
        print(f"  ✗ 编译失败 {name}: {msg}")
    if arm64_hits or silent_hits or compile_fails:
        print("FAIL")
        return 1
    print("PASS（arm64 零非法位宽编码 · linux_x64 零静默丢值）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
