#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""规范覆盖率矩阵门禁（支柱一·plans/026 §2.1·A9）

可测单元口径：plans/001 语言面章节（§二 关键字体系 / §三 类型系统 / §四 词法结构 /
§五 语法规范 / 附录A·B）内的三级/四级/五级标题=一个可测单元；「示例」类标题不算单元。
实现面章节（§六~§十三 编译器架构/IR/代码生成/优化器/运行时/路线图/测试策略/项目结构）
不入用例矩阵——由锚定链/对拍/单测等支柱覆盖（plans/026 §2.1 口径，防「100%」争议）。

三态：每个单元 × {正例， 边界例， 负例}；数据源=tests/e2e/coverage_map.md。

用法：
  python scripts/check_spec_coverage.py            # 报告模式：覆盖率统计+缺口清单
  python scripts/check_spec_coverage.py --ci       # CI 硬门禁：映射指针有效性+孤儿用例须豁免
  python scripts/check_spec_coverage.py --strict   # 严格门禁：--ci + 三态 100%（M1 达成后切换）
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPEC = os.path.join(ROOT, "plans", "001 CN语言编译器设计规格书.md")
MAP = os.path.join(ROOT, "tests", "e2e", "coverage_map.md")
E2E = os.path.join(ROOT, "tests", "e2e")

SPEC_CHAPTERS = ("二、", "三、", "四、", "五、", "附录")  # 语言规范面章节（## 标题前缀）
SPEC_APPENDIX_OK = ("附录A", "附录B")  # 可测契约附录；附录C=mangling 实现细节、附录D=参考资料，不入矩阵
COLS = ("正例", "边界例", "负例")

# B3 口径排除（2026-09-16 用户裁决·plans/026 M1 注记）：7 格属「低语义密度」——规范条文不定义
# 独立可测的新行为（行为由邻近格用例锚定：287/288/302/274/89/301 等，非删测试）。
# 判据：优先级=求值语义非拒绝规则（4.5）；EBNF=语法记法非独立功能（4.6.2）；续行/别名=合法性
# 规则无负例形态（4.6#规则2/3.1）；边界=关键字/移除词边界由正例+负例对承载（2.1/2.2）。
# 分母 183→176；M1 判据=可测格 100%（--strict 按 176 格判定）。
EXCLUDED_CELLS = (
    ("2.1", "边界例"),   # 关键字分类表：关键字边界（合法性由 89 穷举正例+301 拒绝负例承载）
    ("2.2", "边界例"),   # 移除的关键字：移除词边界（合法性由 302 正例+274 拒绝负例承载）
    ("3.1", "负例"),     # 默认类型别名：等价规则无拒绝面（行为由 04_types 邻近格承载）
    ("4.5", "负例"),     # 运算符优先级：求值语义非拒绝规则（行为由 287 矩阵承载）
    ("4.6.2", "边界例"),  # EBNF：语法记法非独立功能
    ("4.6.2", "负例"),   # EBNF：语法记法非独立功能
    ("4.6#规则2：续行（换行≡空格，表达式内部天然续行）", "负例"),  # 续行=合法性规则无负例形态（行为由 288 承载）
)


def parse_units():
    """解析规格书标题树 → 可测单元（有序 dict：ID → 标题）。"""
    units = {}
    order = []
    chapter_in = False
    parent = None
    num_re = re.compile(r"^(\d+(?:\.\d+)*[A-Za-z]?)\s+(.*)$")
    with io.open(SPEC, encoding="utf-8") as f:
        for line in f:
            m = re.match(r"^(#{2,5})\s+(.*?)\s*$", line)
            if not m:
                continue
            level, text = len(m.group(1)), m.group(2).replace("**", "").strip()
            if level == 2:
                chapter_in = text.startswith(SPEC_CHAPTERS)
                continue
            if not chapter_in or level < 3:
                continue
            if text.startswith("附录") and not text.startswith(SPEC_APPENDIX_OK):
                continue  # 附录C/D 非可测语言单元
            if "示例" in text:
                continue  # 示例代码段不是可测规则单元
            if level == 3:
                nm = num_re.match(text)
                if nm:
                    uid = nm.group(1)
                elif text.startswith("附录"):
                    uid = text.split("：")[0]  # 附录A / 附录B
                else:
                    uid = text
                parent = uid
            else:  # #### / #####：自带点编号用编号，否则挂最近 ### 的编号
                nm = num_re.match(text)
                uid = nm.group(1) if nm else "%s#%s" % (parent, text)
            if uid not in units:
                units[uid] = text
                order.append(uid)
    return units, order


def parse_map():
    """解析映射表 → (rows: ID→{三态:[用例]}, exempt: 用例→理由)。"""
    rows, exempt, mode = {}, {}, "main"
    if not os.path.isfile(MAP):
        return rows, exempt
    with io.open(MAP, encoding="utf-8") as f:
        for line in f:
            s = line.strip()
            if s.startswith("## "):
                mode = "exempt" if "豁免" in s else "main"
                continue
            if not s.startswith("|"):
                continue
            cells = [c.strip() for c in s.strip("|").split("|")]
            if mode == "main":
                if len(cells) < 5 or cells[0] in ("单元ID", "") or cells[0].startswith(":-") or cells[0].startswith("---"):
                    continue
                rows[cells[0]] = {COLS[i]: [x.strip() for x in cells[2 + i].split(",") if x.strip()] for i in range(3)}
            else:
                if len(cells) < 2 or cells[0] in ("用例", "") or cells[0].startswith(":-") or cells[0].startswith("---"):
                    continue
                for c in cells[0].split(","):  # 豁免行同样支持一格多例（逗号分隔）
                    c = c.strip()
                    if c:
                        exempt[c] = cells[1]
    return rows, exempt


def scan_cases():
    """tests/e2e/ 下全部用例目录（排除 __pycache__）。"""
    return sorted(d for d in os.listdir(E2E) if os.path.isdir(os.path.join(E2E, d)) and d != "__pycache__")


def main():
    argv = set(sys.argv[1:])
    strict = "--strict" in argv
    ci = "--ci" in argv or strict
    units, order = parse_units()
    rows, exempt = parse_map()
    all_cases = scan_cases()
    errors = []

    # ① 映射单元必须在规范单元清单中（规范标题变更→映射同步强制）
    for uid in rows:
        if uid not in units:
            errors.append("映射表单元 [%s] 不在规范单元清单中（规范标题可能已变更，须同步映射）" % uid)
    # ② 悬空指针：映射/豁免引用的用例目录必须存在
    for uid, cols in rows.items():
        for k, cases in cols.items():
            for c in cases:
                if c not in all_cases:
                    errors.append("单元 [%s] %s 引用不存在的用例目录 [%s]" % (uid, k, c))
    for c in exempt:
        if c not in all_cases:
            errors.append("豁免表引用不存在的用例目录 [%s]" % c)
    # ③ 孤儿用例：未被映射且未豁免=漏映射逃逸
    mapped = set()
    for cols in rows.values():
        for cases in cols.values():
            mapped.update(cases)
    orphans = [c for c in all_cases if c not in mapped and c not in exempt]

    # 覆盖统计（B3 口径：EXCLUDED_CELLS 不计入分母；若意外已有用例指针则提示移除映射）
    full, partial, zero = [], [], []
    gaps = []
    strict_missing = []  # --strict 判据面：非排除格的缺口 (uid, [缺格])
    filled = 0
    total_cells = 0
    excluded_seen = []
    for uid in order:
        cols = rows.get(uid)
        missing = [k for k in COLS if not (cols or {}).get(k)]
        eff_missing = [k for k in missing if (uid, k) not in EXCLUDED_CELLS]
        if eff_missing:
            strict_missing.append((uid, eff_missing))
        for k in COLS:
            if (uid, k) in EXCLUDED_CELLS:
                if cols and cols.get(k):
                    excluded_seen.append((uid, k))
                continue
            total_cells += 1
            if cols and cols.get(k):
                filled += 1
        if not cols or not any(cols.values()):
            zero.append(uid)
        elif all(cols[k] for k in COLS):
            full.append(uid)
        else:
            partial.append(uid)
            gaps.append((uid, missing, eff_missing))
    for uid in zero:
        gaps.append((uid, list(COLS), [k for k in COLS if (uid, k) not in EXCLUDED_CELLS]))
    pct = 100.0 * filled / total_cells if total_cells else 0.0

    print("=== 规范覆盖率矩阵（支柱一·plans/026 §2.1）===")
    print("规范可测单元：%d 个（语言面 §二~§五+附录A/B；实现面章节不入矩阵）" % len(order))
    print("三态覆盖（B3 口径：排除 %d 个低语义密度格·分母 %d）：" % (len(EXCLUDED_CELLS), total_cells))
    print("全 %d ｜ 部分 %d ｜ 零覆盖 %d ｜ 格级覆盖率 %d/%d = %.1f%%"
          % (len(full), len(partial), len(zero), filled, total_cells, pct))
    if excluded_seen:
        print("\n—— 口径排除格却存在用例指针（B3 裁决该 7 格不计分母，映射应移除）——")
        for uid, k in excluded_seen:
            print("  [%s] %s" % (uid, k))
    if zero:
        print("\n—— 零覆盖单元（缺口=补用例排班队列）——")
        for uid in zero:
            print("  [%s] %s" % (uid, units[uid]))
    if partial:
        print("\n—— 部分覆盖单元缺空格 ——")
        for uid, missing, eff in gaps:
            if uid in partial:
                if eff:
                    print("  [%s] %s ：缺 %s" % (uid, units[uid], "、".join(eff)))
                else:
                    print("  [%s] %s ：仅缺口径排除格（%s）" % (uid, units[uid], "、".join(missing)))
    if orphans:
        print("\n—— 孤儿用例（未被映射且未豁免：%d 个）——" % len(orphans))
        for c in orphans:
            print("  %s" % c)
    if errors:
        print("\n—— 映射有效性错误（%d 个）——" % len(errors))
        for e in errors:
            print("  " + e)

    if ci:
        if errors:
            print("\n[FAIL] 规范覆盖映射门禁失败（指针有效性）")
            sys.exit(1)
        if orphans:
            print("\n[FAIL] 孤儿用例未豁免登记（coverage_map.md 豁免区）")
            sys.exit(1)
        if strict and strict_missing:
            print("\n[FAIL] 严格模式：可测格（B3 口径 %d 格）未达 100%%（当前 %.1f%%）——M1 未达成" % (total_cells, pct))
            for uid, ks in strict_missing:
                print("  [%s] 缺 %s" % (uid, "、".join(ks)))
            sys.exit(1)
        if strict:
            print("[PASS] 严格模式：可测格 %d/%d = 100%%（B3 口径）——M1 判据①达成" % (filled, total_cells))
        print("\n[PASS] 规范覆盖映射门禁（--ci：指针有效+零孤儿）通过；覆盖率 %.1f%%（缺口清单见报告模式）" % pct)
    else:
        print("\n报告模式（--ci=硬门禁 / --strict=三态 100%%；缺口清单=自动生成的排班队列）")


if __name__ == "__main__":
    main()
