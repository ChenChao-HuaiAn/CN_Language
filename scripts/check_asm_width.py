#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""D31/D32 门禁（258-a/260-a）：三后端汇编位宽与完备性全库扫描。

三维铁律（D31 方案C④ + D32 机械校验·机械可证「绝不产非法/缺位发射」）：
  1. arm64：movz/movk 作用于 wN（32 位）只允许 lsl #0/#16 ——
     `movk wN, #imm, lsl #32/48` 为非法编码（GNU as 拒绝 = 编译失败级，plans/021 D31）。
  2. linux_x64：dispatch 对未实现 opcode 落 default 只出「未支持操作码」注释
     = 静默丢值（258-a 实证 Copy 曾漏发射——比汇编失败更危险），指纹零命中。
  3. win-x64：mov 操作数宽度混配（r64 目标配 r32 源 / r32 目标配 r64 源）——
     x86 无此编码（MASM A2022 拒绝；GCC 面早暴露、MSVC 面晚暴露的跨机分叉点，
     CN-Smith s268 实证），指纹零命中。内存操作数与 movsx/movzx/movsxd 合法形态排除。

流程：tests/e2e 宿主通道全量用例经 `compile --target <后端>` 出 .s（只产汇编、
不调 as —— 任意平台可跑），逐文件扫描。负例（期望编译失败/期望check失败）与
v2 闭环用例（v2p 工具链通道）不在扫描面。旗标通道「编译选项.txt」与 runner 同口径。
自带正/负样本自检（探测器自身也是被测对象——110-a 教训）。

用法：python scripts/check_asm_width.py [--cn target/Debug/cn.exe]
退出码：0 = 全过；1 = 有违例或编译失败。
"""
import argparse
import concurrent.futures
import io
import os
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

# win-x64 mov 宽度混配（x86 无此编码，MASM A2022；s268/D32 族）：
#   仅锚定「逗号 + 纯寄存器 + 行尾/注释」形态，内存操作数([..])与
#   movsx/movzx/movsxd（\bmov\s 不匹配）天然排除
_R64 = r"(?:rax|rbx|rcx|rdx|rsi|rdi|rbp|rsp|r(?:8|9|1[0-5]))\b"
_R32 = r"(?:eax|ebx|ecx|edx|esi|edi|ebp|esp|r(?:8|9|1[0-5])d)\b"
WIN_BAD_MIX = re.compile(
    r"\bmov\s+" + _R64 + r"\s*,\s*" + _R32 + r"\s*(?:$|;)|"
    r"\bmov\s+" + _R32 + r"\s*,\s*" + _R64 + r"\s*(?:$|;)",
    re.MULTILINE)

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

SELF_WIN_LEGAL = (
    "    mov rax, rbx\n"          # r64 <- r64 合法
    "    mov eax, ebx\n"          # r32 <- r32 合法
    "    mov r14, rax\n"          # r64 <- r64 合法
    "    mov eax, dword ptr [rbp-56]\n"   # 内存操作数豁免
    "    mov r12, qword ptr [rbx+8]\n"
    "    movsxd rax, eax\n"       # 符号扩展合法
    "    mov eax, r14d\n"         # r32 <- r32（258-a cast 收缩产物）
    "    mov rax, offset @str0\n"
)
SELF_WIN_ILLEGAL = (
    "    mov rax, eax\n"          # r64 <- r32 非法（A2022）
    "    mov eax, r14\n"          # r32 <- r64 非法（s268 修复前形态）
    "    mov r9, ecx ; src: a.cn:3\n"     # 带行尾注释同样命中
)


def self_check() -> None:
    if ARM64_BAD_ENC.search(SELF_ARM64_LEGAL):
        raise SystemExit("自检失败：arm64 合法样本被误判")
    if len(ARM64_BAD_ENC.findall(SELF_ARM64_ILLEGAL)) != 2:
        raise SystemExit("自检失败：arm64 非法样本漏判")
    if not LX64_SILENT_DROP.search(SELF_SILENT_SAMPLE):
        raise SystemExit("自检失败：linux_x64 指纹漏判")
    if WIN_BAD_MIX.search(SELF_WIN_LEGAL):
        raise SystemExit("自检失败：win-x64 合法样本被误判:\n"
                         + WIN_BAD_MIX.search(SELF_WIN_LEGAL).group(0))
    if len(WIN_BAD_MIX.findall(SELF_WIN_ILLEGAL)) != 3:
        raise SystemExit("自检失败：win-x64 宽度混配样本漏判，仅命中 "
                         f"{len(WIN_BAD_MIX.findall(SELF_WIN_ILLEGAL))} 处")


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
    ap.add_argument("--jobs", type=int, default=0,
                    help="并行度（0=CPU 核数，1=串行）")
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
    win_hits = []        # (用例名, 行号, 片段)
    backends = (("linux-arm64", ARM64_BAD_ENC, arm64_hits),
                ("linux-x86_64", LX64_SILENT_DROP, silent_hits),
                ("win-x64", WIN_BAD_MIX, win_hits))
    jobs = args.jobs if args.jobs > 0 else (os.cpu_count() or 4)

    def scan_one(name, src, target, pattern):
        """单（用例×后端）任务：compile 出 .s + 扫描。
        返回 (fails, hits)：fails 为 (标签,消息) 列表；hits 为 (名,行,片段) 列表。"""
        fails, hits = [], []
        with tempfile.TemporaryDirectory() as td:
            out_s = pathlib.Path(td) / "out.s"
            cmd = [str(cn), "compile", str(src), "--target", target,
                   "--output", str(out_s)]
            cmd += read_case_flags(src.parent)
            r = subprocess.run(cmd, capture_output=True, timeout=120)
            if r.returncode != 0 or not out_s.exists():
                msg = r.stderr.decode("utf-8", "replace").strip().splitlines()
                fails.append((f"{name}[{target}]", msg[-1] if msg else "无 .s 产出"))
                return fails, hits
            text = out_s.read_text(encoding="utf-8", errors="replace")
            for m in pattern.finditer(text):
                lineno = text.count("\n", 0, m.start()) + 1
                hits.append((name, lineno, m.group(0).strip()))
        return fails, hits

    t0 = time.time()
    tasks = [(name, src, target, pattern)
             for name, src in cases for target, pattern, _b in backends]
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futs = {pool.submit(scan_one, *t): (t[0], t[2]) for t in tasks}
        for fut in concurrent.futures.as_completed(futs):
            name, target = futs[fut]
            fails, hits = fut.result()
            compile_fails.extend(fails)
            if hits:
                bucket = {"linux-arm64": arm64_hits,
                          "linux-x86_64": silent_hits,
                          "win-x64": win_hits}[target]
                bucket.extend(hits)

    print("== check_asm_width（D31/D32·258-a/260-a）==")
    print(f"用例 {len(cases)}×3 后端 ｜ 并行 {jobs}"
          f" ｜ 编译失败 {len(compile_fails)}"
          f" ｜ arm64 非法编码 {len(arm64_hits)}"
          f" ｜ linux_x64 静默丢值 {len(silent_hits)}"
          f" ｜ win-x64 宽度混配 {len(win_hits)}"
          f" ｜ 耗时 {time.time()-t0:.1f}s")
    for name, lineno, frag in arm64_hits[:20]:
        print(f"  ✗ arm64 {name}:{lineno}: {frag}")
    for name, lineno, frag in silent_hits[:20]:
        print(f"  ✗ linux_x64 静默丢值 {name}:{lineno}: {frag}")
    for name, lineno, frag in win_hits[:20]:
        print(f"  ✗ win-x64 宽度混配 {name}:{lineno}: {frag}")
    for name, msg in compile_fails[:20]:
        print(f"  ✗ 编译失败 {name}: {msg}")
    if arm64_hits or silent_hits or win_hits or compile_fails:
        print("FAIL")
        return 1
    print("PASS（arm64 零非法位宽编码 · linux_x64 零静默丢值 · win-x64 零宽度混配）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
