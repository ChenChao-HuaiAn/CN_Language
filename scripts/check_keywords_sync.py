#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""关键字单一事实源自检（plans/024 §7.5，162-a 落地）：

比对四处关键字清单的一致性：
  ① 宿主实现：src/cn_compiler/lexer/lexer.cpp 的 keywordTable
  ② 规范：plans/001 §2.1 分类表 + §2.1a 上下文关键字表（保留字仅比对 §2.1）
  ③ 契约用例：tests/e2e/89_关键字穷举/主.cn 的用例串
  ④ v2 自举组件：CN语言编译器v2/词法/词法分析.cn 的 关键字ID表
  ⑤ v1 自举组件：CN语言编译器/词法/关键字.cn 的 查找关键字 组表
     （v1 含 遍历/中/每个 三个 v1 遗留上下文词，白名单豁免）

比对规则：①②③ 必须集合相等；④⑤ 允许为 ①②③ 的子集——
v2/v1 面未实现项按显式白名单登记（白名单只减不增，新增差异即失败）。
正/负样本自检：脚本内置自测（--self-test），负样本=人为抽走一词须报非零。
输出：差异明细；不一致=退出码非零（CI 失败）。
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

# v2/v1 面未实现项白名单（④⑤ 允许缺失的 ① 词；只减不增）
# v2 面 162-a 后=22 词（控制流/声明3/类接口结构体枚举联合体/真假无/不安全）——
#   类型别名区 18 词 + 友元/自身/父类 + 结果/可选 + 泛型 未在 v2 实现（继承既有台账）
V2_WHITELIST: set[str] = {
    "整数", "小数", "整8", "整16", "整32", "整64", "整128",
    "正8", "正16", "正32", "正64", "正128", "浮32", "浮64",
    "布尔", "字符", "字符串", "空类型",
    "友元", "自身", "父类", "结果", "可选", "泛型",
}
V1_EXTRA: set[str] = {"遍历", "中", "每个"}  # v1 遗留上下文词（不参与保留字比对缺失惩罚）


def 提取宿主表() -> set[str]:
    """lexer.cpp keywordTable 的 {"词", TokenType::Kw_X} 条目"""
    txt = (ROOT / "src/cn_compiler/lexer/lexer.cpp").read_text(encoding="utf-8")
    block = txt[txt.index("keywordTable()"):]
    block = block[: block.index("};")]
    return set(re.findall(r'\{"([^"]+)",\s*TokenType::Kw_\w+\}', block))


def 提取规范表() -> set[str]:
    """plans/001 §2.1 分类表：标题行（- / ** 起）的冒号后段或下一行按顿号切分"""
    txt = (ROOT / "plans/001 CN语言编译器设计规格书.md").read_text(encoding="utf-8")
    sec = txt[txt.index("### 2.1 关键字分类表"): txt.index("### 2.1a")]
    words: set[str] = set()
    行s = sec.splitlines()

    def 吸收(body: str) -> None:
        if not body:
            return
        for w in body.split("、"):
            w = re.sub(r"[（(].*?[)）]", "", w.strip()).strip("。；;，, ")
            if w and len(w) <= 8 and not re.search(r"[()**`|-]", w):
                words.add(w)

    for i, raw in enumerate(行s):
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith(">"):
            continue
        is_head = line.startswith("- ") or line.startswith("**")
        if is_head:
            # 标题行：词在冒号后段，或独占下一行
            body = re.split(r"[：:]", line, maxsplit=1)[1] if re.search(r"[：:]", line) else ""
            if "、" not in body:
                nxt = 行s[i + 1].strip() if i + 1 < len(行s) else ""
                if nxt and "、" not in nxt:
                    body = nxt  # 单词独占行（如 不安全）
            吸收(body)
        elif "、" in line:
            吸收(line)
    return words


def 提取用例串() -> set[str]:
    """89 用例 主.cn 的字符串字面量保留字串"""
    txt = (ROOT / "tests/e2e/89_关键字穷举/主.cn").read_text(encoding="utf-8")
    m = re.search(r'字符串 源码 = "([^"]+)"', txt)
    if not m:
        raise SystemExit("89_关键字穷举/主.cn 缺少 源码 字符串字面量")
    return set(m.group(1).split())


def 提取v2表() -> set[str]:
    """v2 词法分析.cn 关键字ID表 的 整32追加(表, 整32(驻留("词"))) 条目"""
    txt = (ROOT / "CN语言编译器v2/词法/词法分析.cn").read_text(encoding="utf-8")
    block = txt[txt.index("函数 关键字ID表"):]
    block = block[: block.index("\n}", 1)]
    return set(re.findall(r'驻留\("([^"]+)"\)', block))


def 自检() -> int:
    宿主 = 提取宿主表()
    规范 = 提取规范表()
    v2 = 提取v2表()
    问题: list[str] = []

    # ①② 必须完全一致
    for 名, 集合 in (("宿主 lexer", 宿主), ("规范 §2.1", 规范)):
        缺 = 宿主 - 集合
        多 = 集合 - 宿主
        if 缺:
            问题.append(f"{名} 缺失: {' '.join(sorted(缺))}")
        if 多:
            问题.append(f"{名} 多余: {' '.join(sorted(多))}")

    # ③ 允许子集（白名单外缺失即失败）
    缺 = 宿主 - v2 - V2_WHITELIST
    if 缺:
        问题.append(f"v2 表 非白名单缺失: {' '.join(sorted(缺))}")
    多 = v2 - 宿主
    if 多:
        问题.append(f"v2 表 含已删词: {' '.join(sorted(多))}")

    print(f"[关键字同步] 宿主={len(宿主)} 规范={len(规范)} "
          f"v2={len(v2)}（白名单 {len(V2_WHITELIST)}）")
    if 问题:
        for q in 问题:
            print(f"  ✗ {q}")
        return 1
    print("  ✓ 四处清单一致（单一事实源成立）")
    return 0


def 负样本自检() -> int:
    """负样本：人为从宿主表源码抽走一词后解析须报差异"""
    p = ROOT / "src/cn_compiler/lexer/lexer.cpp"
    原文 = p.read_text(encoding="utf-8")
    抽词 = sorted(提取宿主表())[0]
    篡改 = 原文.replace(f'{{"{抽词}"', f'{{"@已删@"', 1)
    p.write_text(篡改, encoding="utf-8")
    try:
        rc = 自检()
    finally:
        p.write_text(原文, encoding="utf-8")
    if rc == 0:
        print("  ✗ 负样本未被检出（门禁失效）")
        return 1
    print("  ✓ 负样本检出正常（脚本自身受检）")
    return 0


if __name__ == "__main__":
    rc = 自检()
    if "--self-test" in sys.argv:
        rc = max(rc, 负样本自检())
    sys.exit(rc)
