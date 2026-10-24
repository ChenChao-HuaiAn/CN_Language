#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""D31/D32 门禁（258-a/260-a）：三后端汇编位宽与完备性全库扫描。

三维铁律（D31 方案C④ + D32 机械校验·机械可证「绝不产非法/缺位发射」）：
  1. arm64：movz/movk 作用于 wN（32 位）只允许 lsl #0/#16 ——
     `movk wN, #imm, lsl #32/48` 为非法编码（GNU as 拒绝 = 编译失败级，plans/021 D31）。
  2. linux_x64：dispatch 对未实现 opcode 落 default 只出「未支持操作码」注释
     = 静默丢值（258-a 实证 Copy 曾漏发射——比汇编失败更危险），指纹零命中。
  3. win-x64：操作数宽度混配（r64 目标配 r32 源 / r32 目标配 r64 源）——
     覆盖 mov 全族与 cmp/test/算术族前两纯寄存器操作数（1281：#1159 窄整型
     纳入分配后 `cmp ebx, r14` 实锤）。x86 无此编码（MASM A2022 拒绝；GCC 面
     早暴露、MSVC 面晚暴露的跨机分叉点，CN-Smith s268 实证），指纹零命中。
     内存操作数（[rbx] 等寻址基址）与 movsx/movzx/movsxd 合法形态排除。

流程：tests/e2e 宿主通道全量用例经 `compile --target <后端>` 出 .s（只产汇编、
不调 as —— 任意平台可跑），逐文件扫描。负例（期望编译失败/期望check失败）与
v2 闭环用例（v2p 工具链通道）不在扫描面。旗标通道「编译选项.txt」与 runner 同口径。
自带正/负样本自检（探测器自身也是被测对象——110-a 教训）。

用法：python scripts/check_asm_width.py [--cn target/Debug/cn.exe] [--基线 <sha>]
退出码：0 = 全过；1 = 有违例或编译失败。

1300 写集增量（2026-10-21·门禁表实测 10-17 5~8s→10-21 645s——后者是该检查云轮上
首次以完整形态真跑的真实价格：全用例×3 后端各起真实编译子进程·无缓存无增量；
按用例入库增速约一周撞 900s 步骤超时）：带 --基线 <sha> 时按 git diff 基线..HEAD
分类扫描面——
  ① 编译器核心面变更（src/CN语言编译器v2/stdlib/runtime/tools/CMake）→ 全扫
    （编译器动了才可能全局影响 asm·安全面不松）；
  ② 仅 tests/e2e/<目录> 变更 → 只扫这些目录；
  ③ 仅明确无关面（.md/website/docs/plans/scripts/项目记忆）→ 跳过（显式说明）；
  ④ 含未知路径（不在①③清单的新顶层）→ 保守全扫（宁多扫不漏扫）。
不带 --基线（develop/兜底轮·本地）=恒全扫（develop 平台轮转=权威全量面·铁律保持）。
1300 内容寻址缓存：键=(源内容+旗标+后端+编译器面 git 树哈希)——编译器面未变的轮
  cn 二进制即便重建也语义等价（树哈希未变），命中 .s 与重编逐字节同源=数学等价；
  缓存放 target/cache/asm_width（与 1142 target/cache/ast 同址）。云端预验轮因
  637 清产物恒冷（缓存主要收益面=本地开发轮与未来 637 豁免接口）。
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

import ci_duration   # 1385：扫描口径回传（耗时基线按 全扫/目录扫N·缓存冷热 分桶）

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# 与 runner 平台跳过口径一致（62_ffi 依赖 Windows API；69 运行时计数平台差异）
SKIP_DIRS = {"62_ffi", "69_memory_management"}

# arm64 非法编码：movz/movk + wN + lsl #32/#48（w 寄存器只允许 lsl #0/#16）
ARM64_BAD_ENC = re.compile(r"\bmov[kz]\s+w\d+\s*,\s*#\d+\s*,\s*lsl\s*#(?:32|48)\b")

# linux_x64 未支持 opcode 指纹（dispatch default 只出注释 = 静默丢值）
LX64_SILENT_DROP = re.compile(r"未支持操作码")

# win-x64 宽度混配（x86 无此编码，MASM A2022；s268/D32 族）：
#   仅锚定「逗号 + 纯寄存器 + 行尾/注释」形态，内存操作数([..])与
#   movsx/movzx/movsxd（\bmov\s 不匹配）天然排除。
# 1281（#1159 回归）：同一虚拟寄存器在两消费点宽度不一致不限于 mov——
#   `cmp ebx, r14`（32 位工作名 + 64 位源名）实锤 ml64 A2022。判据扩到
#   比较/算术族前两操作数（两序均拦）；内存寻址基址（[rbx]）不参与判定
#   （cmp eax, dword ptr [rbx] 合法，第二操作数非纯寄存器不命中）。
_R64 = r"(?:rax|rbx|rcx|rdx|rsi|rdi|rbp|rsp|r(?:8|9|1[0-5]))\b"
_R32 = r"(?:eax|ebx|ecx|edx|esi|edi|ebp|esp|r(?:8|9|1[0-5])d)\b"
_ALU = r"(?:cmp|test|add|sub|adc|sbb|and|or|xor|imul)"
# 4.（316-a·C22 丙/T44）win `[rbp0]` = varSlotOf 未登记回 0 拼接（rbp0 家族·
#    MASM A2006 undefined symbol·T44 汇合临时槽实锤）——合法槽恒带非零偏移文本。
WIN_BAD_MIX = re.compile(
    r"\[rbp0\]|"
    r"\bmov\s+" + _R64 + r"\s*,\s*" + _R32 + r"\s*(?:$|;)|"
    r"\bmov\s+" + _R32 + r"\s*,\s*" + _R64 + r"\s*(?:$|;)|"
    r"^\s*" + _ALU + r"\s+" + _R64 + r"\s*,\s*" + _R32 + r"\s*(?:$|;)|"
    r"^\s*" + _ALU + r"\s+" + _R32 + r"\s*,\s*" + _R64 + r"\s*(?:$|;)",
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
    "    mov [rbp-232], rax\n"    # 合法栈槽恒带偏移（316-a C22 丙）
    "    mov rax, [rbp0abc]\n"    # 非完整记号 [rbp0]（标识符后缀≠命中）
)
SELF_WIN_ILLEGAL = (
    "    mov rax, eax\n"          # r64 <- r32 非法（A2022）
    "    mov eax, r14\n"          # r32 <- r64 非法（s268 修复前形态）
    "    mov r9, ecx ; src: a.cn:3\n"     # 带行尾注释同样命中
    "    mov [rbp0], rax\n"       # rbp0 家族（T44 高半槽偏移 0 拼接·316-a）
    "    cmp ebx, r14\n"          # 1281 实锤形态（32 位工作名 + 64 位源名）
    "    test r12, ecx\n"         # 同族反向（64 位首操作数 + 32 位源）
    "    imul eax, r13\n"         # 同族乘法
)
SELF_WIN_ALU_LEGAL = (
    "    cmp ebx, ecx\n"          # 同宽（32）合法
    "    cmp r12, r13\n"          # 同宽（64）合法
    "    cmp eax, dword ptr [rbx]\n"  # 内存寻址基址不参与宽度判定
    "    add r12, 8\n"            # 64 位 + 立即数
    "    shl rax, cl\n"           # 移位量 cl 固定 8 位（不同指令族判定）
    "    and al, cl\n"            # 8 位位运算合法
    "    movsxd rcx, eax\n"       # 符号扩展指令合法（非 mov 精确形态）
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
    if len(WIN_BAD_MIX.findall(SELF_WIN_ILLEGAL)) != 7:
        raise SystemExit("自检失败：win-x64 宽度混配/rbp0 样本漏判，仅命中 "
                         f"{len(WIN_BAD_MIX.findall(SELF_WIN_ILLEGAL))} 处")
    if WIN_BAD_MIX.search(SELF_WIN_ALU_LEGAL):
        raise SystemExit("自检失败：win-x64 同宽/内存样本被误判：\n"
                         + WIN_BAD_MIX.search(SELF_WIN_ALU_LEGAL).group(0))
    # 1257：旗标通道与 runner 同口径——空行/# 注释行不得混入 argv
    #   （1402 首行注释曾被当「多余的源文件参数」，三后端每跑 3 假红）
    with tempfile.TemporaryDirectory() as td:
        case = pathlib.Path(td)
        (case / "编译选项.txt").write_text(
            "# 注释行不是旗标\n-D\n中文宏名\n\n", encoding="utf-8")
        flags = read_case_flags(case)
        if flags != ["-D", "中文宏名"]:
            raise SystemExit("自检失败：编译选项 # 注释/空行未过滤: " + repr(flags))
    # 1300：写集分类正反两态（纯函数·扫描面判定的四象限）
    模式, 们 = 写集扫描面(["src/cn_main.cpp"])
    if 模式 != "全":
        raise SystemExit("自检失败：编译器面变更应全扫")
    模式, 们 = 写集扫描面(["tests/e2e/01_hello/主.cn", "tests/e2e/02_x/y.cn"])
    if 模式 != "目录" or 们 != ["01_hello", "02_x"]:
        raise SystemExit(f"自检失败：用例目录变更应目录扫，得 {模式} {们}")
    模式, 们 = 写集扫描面(["README.md", "website/x.py", "plans/001.md"])
    if 模式 != "跳过":
        raise SystemExit(f"自检失败：纯无关面应跳过，得 {模式}")
    模式, 们 = 写集扫描面(["新顶层/未知.cn"])
    if 模式 != "全":
        raise SystemExit(f"自检失败：未知路径应保守全扫，得 {模式}")
    模式, 们 = 写集扫描面(["tests/e2e/01_hello/主.cn", "scripts/x.py"])
    if 模式 != "目录" or 们 != ["01_hello"]:
        raise SystemExit(f"自检失败：用例+scripts 混合应目录扫，得 {模式} {们}")
    # 1385：耗时基线口径串正反态（全扫冷缓存/目录扫部分命中/跳过/零任务）
    if 扫描口径("全", [], 0, 300) != "全扫·冷缓存":
        raise SystemExit("自检失败：全扫冷缓存口径错")
    if 扫描口径("目录", ["a", "b"], 3, 10) != "目录扫2·部分命中":
        raise SystemExit("自检失败：目录扫部分命中口径错")
    if 扫描口径("跳过", [], 0, 0) != "跳过":
        raise SystemExit("自检失败：写集跳过口径错")
    if 扫描口径("全", [], 10, 10) != "全扫·全命中":
        raise SystemExit("自检失败：全命中口径错")
    if 扫描口径("全", [], 0, 0) != "全扫·零任务":
        raise SystemExit("自检失败：零任务口径错")


def read_case_flags(case_dir: pathlib.Path) -> list:
    """用例级编译旗标（runner「编译选项.txt」通道同口径）。

    空行与 # 注释行跳过（与 tests/e2e/run_e2e.py 查找编译选项文件 一致；
    1257：1402 用例首行注释曾被当旗标传入，三后端每跑必报假编译失败）。
    """
    marker = case_dir / "编译选项.txt"
    if not marker.exists():
        return []
    return [ln.strip() for ln in marker.read_text(encoding="utf-8-sig").splitlines()
            if ln.strip() and not ln.strip().startswith("#")]


# ---- 1300 写集分类（纯函数·selftest 正反两态面）----
编译器面前缀 = ("src/", "CN语言编译器v2/", "stdlib/", "runtime/", "tools/",
                "third-party/")
编译器面文件 = ("CMakeLists.txt",)
明确无关前缀 = ("website/", "docs/", "plans/", "scripts/", "项目记忆/", ".zcode/",
                ".github/")


def 扫描口径(模式: str, 目录们: list, 命中: int, 任务数: int) -> str:
    """1385：耗时基线口径串（daemon 经 CN_STEP_META_DIR 回读）——
    扫描面（全扫/目录扫N/跳过）× 缓存状态（冷/部分/全命中/零任务）。
    645s 全扫冷缓存 vs 秒级跳过/全命中不混算基线，避免每次写集变化都误报。"""
    if 模式 == "跳过":
        return "跳过"
    面 = "全扫" if 模式 == "全" else "目录扫%d" % len(目录们)
    if 任务数 <= 0:
        缓存 = "零任务"
    elif 命中 <= 0:
        缓存 = "冷缓存"
    elif 命中 >= 任务数:
        缓存 = "全命中"
    else:
        缓存 = "部分命中"
    return 面 + "·" + 缓存


def 写集扫描面(变更文件们: list) -> tuple:
    """变更文件清单 → (模式, 用例目录名们)。模式∈("全","目录","跳过")。
    分类序=①编译器核心面→全扫；②tests/e2e/<目录>→目录扫（去重保序）；
    ③纯明确无关面→跳过；④含未知路径→保守全扫（宁多扫不漏扫——新顶层目录
    语义未知时不可假设无关）。"""
    若编译器面 = any(f.startswith(编译器面前缀)
                    or f in 编译器面文件
                    or f.startswith("CMakeLists") or f.endswith(".cmake")
                    for f in 变更文件们)
    if 若编译器面:
        return "全", []
    用例们 = []
    全无关 = True
    for f in 变更文件们:
        if f.startswith("tests/e2e/"):
            全无关 = False
            目 = f.split("/")[2] if f.count("/") >= 2 else ""
            if 目 and 目 not in 用例们:
                用例们.append(目)
        elif not f.startswith(明确无关前缀) and not f.lower().endswith(".md"):
            全无关 = False   # 未知路径——保守全扫
    if 全无关:
        return "跳过", []
    if 用例们:
        return "目录", 用例们
    return "全", []   # 混有未知路径且无用例目录命中→保守全扫


def 编译器面树哈希(仓库根: pathlib.Path) -> str:
    """编译器核心面的 git 树哈希拼接（内容寻址·面未变则串不变）——缓存键的
    编译器维度：面未变时 cn 即便重建（字节可不同）语义等价，命中 .s 与重编同源。
    git 失败=返回异常文本（键全变=缓存恒 miss·退化为无缓存正确性无损）。"""
    import hashlib
    h = hashlib.sha256()
    for 面 in ("src", "stdlib", "runtime", "tools", "CN语言编译器v2"):
        r = subprocess.run(["git", "rev-parse", "HEAD:" + 面], cwd=仓库根,
                           capture_output=True, text=True, timeout=30)
        h.update((r.stdout.strip() if r.returncode == 0 else "miss:" + 面).encode())
    return h.hexdigest()[:16]


def 缓存条目路径(缓存目录: pathlib.Path, src: pathlib.Path, 旗标: list,
                 后端: str, 编译器哈希: str) -> pathlib.Path:
    """1300 内容寻址缓存条目——键=sha256(源内容+旗标串+后端+编译器面树哈希)：
    源/旗标/编译器面任一变即 miss（保守键·宁重编不错用陈产物；编译器面未变时
    cn 重建后语义等价·命中 .s 与重编同源=数学等价非「脏缓存」）。"""
    import hashlib
    h = hashlib.sha256()
    h.update(src.read_bytes())
    h.update(("|".join(旗标) + "|" + 后端 + "|" + 编译器哈希).encode("utf-8"))
    return 缓存目录 / (h.hexdigest() + ".s")


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
        # 569-a C25 波2 转批后的负测双测形态（双编译对照.txt+预期编译失败=是·
        #   双侧都必须拒绝）：578-a 补同步——此前仅认旧三形态，151 例转批负测
        #   混入扫描面致编译失败 450（develop 现态 569 直入时本段漏验的隐藏红）
        dual = d / "双编译对照.txt"
        if dual.exists() and "预期编译失败=是" in dual.read_text(encoding="utf-8-sig"):
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
    ap.add_argument("--基线", default="",
                    help="写集基线 sha（1300）：带=按 git diff 基线..HEAD 分类扫描面"
                         "（编译器面→全扫/仅用例目录→目录扫/纯无关→跳过/未知→保守全扫）；"
                         "不带=全扫（develop 兜底轮权威面·本地默认）")
    args = ap.parse_args()

    self_check()
    cn = pathlib.Path(args.cn)
    if not cn.exists():
        print(f"未找到编译器: {cn}")
        return 1

    cases = collect_cases(pathlib.Path(args.e2e))
    扫描模式, 扫描目录们 = "全", []
    if args.基线:
        r = subprocess.run(["git", "diff", "--name-only", args.基线 + "..HEAD"],
                           capture_output=True, text=True, timeout=60)
        变更们 = [l.strip() for l in (r.stdout or "").splitlines() if l.strip()] \
            if r.returncode == 0 else []
        if r.returncode != 0:
            print(f"写集 diff 失败（rc={r.returncode}）——保守全扫")
        else:
            扫描模式, 扫描目录们 = 写集扫描面(变更们)
            if 扫描模式 == "目录":
                集 = set(扫描目录们)
                cases = [c for c in cases if c[0] in 集]
            elif 扫描模式 == "跳过":
                ci_duration.写步骤元信息("check_asm_width",
                                        {"口径": 扫描口径("跳过", [], 0, 0),
                                         "说明": "写集基线无编译器面/用例面变更"})
                print(f"== check_asm_width（D31/D32·258-a/260-a）==\n"
                      f"写集基线 {args.基线[:10]} 无编译器面/用例面变更"
                      f"（纯无关面 {len(变更们)} 文件）——跳过扫描（1300 增量·"
                      f"develop 兜底轮不带 --基线 恒全扫）\nPASS（写集跳过·三后端零变更面）")
                return 0
    编译哈希 = 编译器面树哈希(pathlib.Path("."))   # 1300：缓存键编译器维度（git 失败=恒 miss 无损）
    缓存目录 = pathlib.Path("target/cache/asm_width")
    compile_fails = []   # (用例名[后端], 消息)
    arm64_hits = []      # (用例名, 行号, 片段)
    silent_hits = []     # (用例名, 行号, 片段)
    win_hits = []        # (用例名, 行号, 片段)
    backends = (("linux-arm64", ARM64_BAD_ENC, arm64_hits),
                ("linux-x86_64", LX64_SILENT_DROP, silent_hits),
                ("win-x64", WIN_BAD_MIX, win_hits))
    jobs = args.jobs if args.jobs > 0 else (os.cpu_count() or 4)
    命中缓存数 = [0]

    def scan_one(name, src, target, pattern):
        """单（用例×后端）任务：compile 出 .s + 扫描（1300：内容寻址缓存命中跳编译）。
        返回 (fails, hits)：fails 为 (标签,消息) 列表；hits 为 (名,行,片段) 列表。"""
        fails, hits = [], []
        旗标 = read_case_flags(src.parent)
        条目 = 缓存条目路径(缓存目录, src, 旗标, target, 编译哈希)
        text = None
        if 条目.exists():
            try:
                text = 条目.read_text(encoding="utf-8", errors="replace")
                命中缓存数[0] += 1
            except OSError:
                text = None
        if text is None:
            with tempfile.TemporaryDirectory() as td:
                out_s = pathlib.Path(td) / "out.s"
                cmd = [str(cn), "compile", str(src), "--target", target,
                       "--output", str(out_s)]
                cmd += 旗标
                r = subprocess.run(cmd, capture_output=True, timeout=120)
                if r.returncode != 0 or not out_s.exists():
                    msg = r.stderr.decode("utf-8", "replace").strip().splitlines()
                    fails.append((f"{name}[{target}]", msg[-1] if msg else "无 .s 产出"))
                    return fails, hits
                text = out_s.read_text(encoding="utf-8", errors="replace")
            try:
                缓存目录.mkdir(parents=True, exist_ok=True)
                条目.write_text(text, encoding="utf-8")
            except OSError:
                pass   # 缓存写失败=退化为无缓存（正确性无损·下次重编）
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
    模式说明 = ("全扫" if 扫描模式 == "全"
               else "目录扫（%d 个：%s）" % (len(扫描目录们), "、".join(扫描目录们[:5])
                                            + ("…" if len(扫描目录们) > 5 else "")))
    ci_duration.写步骤元信息(
        "check_asm_width",
        {"口径": 扫描口径(扫描模式, 扫描目录们, 命中缓存数[0], len(tasks)),
         "说明": 模式说明})
    print(f"扫描面 {模式说明}{f'（1300 写集基线 {args.基线[:10]}）' if args.基线 else ''}"
          f" ｜ 缓存命中 {命中缓存数[0]}/{len(tasks)}")
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
