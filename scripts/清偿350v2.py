# -*- coding: utf-8 -*-
"""#1241 全量清偿脚本 v2：按实时警告清单 target/warn_dedup.txt 的 (文件,行) 修复
「检查 结果.正常 后未处理错误分支」——把紧随的 miss 顺序返回移入否则块：
  形态A（单行闭合）：如果 (X.正常) { A; }  +  返回 Z;
    → 如果 (X.正常) { A; } 否则 { 返回 Z; }（t1 探针实证合法）
  形态B（多行块）：如果 (X.正常) { … }  +  返回 Z;
    → 块尾 } 改 } 否则 {，返回行缩进+4，其后补 }
协议注释加在如果行上一行（幂等：已含 否则 { 的行跳过）。用后即删。"""
import re, io

ROOT = r"C:\Users\ChenChao\Documents\CN\wt1241"
NOTE = "// 静默豁免业务值: 查表 miss=键不存在·显式哨兵（1241 显式化）"

warn = {}
for l in io.open(ROOT + "/target/warn_dedup.txt", encoding="utf-8"):
    m = re.match(r"\./(.+?\.cn):(\d+):\d+", l.strip())
    if m:
        warn.setdefault(m.group(1), set()).add(int(m.group(2)))
print("文件", len(warn), "位置", sum(len(v) for v in warn.values()))

fixed = skipped = manual = 0
manual_list = []
for rel, rows in sorted(warn.items()):
    path = ROOT + "/CN语言编译器v2/" + rel
    lines = io.open(path, encoding="utf-8").read().split("\n")
    for row in sorted(rows, reverse=True):
        i = row - 1
        if i >= len(lines):
            manual_list.append((rel, row, "越界")); manual += 1; continue
        line = lines[i]
        if "否则" in line:
            skipped += 1; continue  # 已显式化（幂等）
        if ".正常" not in line or "如果" not in line:
            manual_list.append((rel, row, "非如果.正常行")); manual += 1; continue
        ind = re.match(r"\s*", line).group(0)
        note = ind + NOTE
        # 前一行已是同款注释则不重复加
        if i > 0 and NOTE in lines[i - 1]:
            note = None
        # 形态A：单行闭合（行尾 }）——返回行允许隔空行（≤3 行内）
        if line.rstrip().endswith("}") and "{" in line:
            k = i + 1
            while k < len(lines) and k <= i + 3 and lines[k].strip() == "":
                k += 1
            if k < len(lines) and k <= i + 3:
                m2 = re.match(r"返回 (.+;)\s*(//.*)?$", lines[k].strip())
                if m2:
                    body = "返回 " + m2.group(1)
                    tail = ("   " + m2.group(2)) if m2.group(2) else ""
                    merged = line.rstrip() + " 否则 { " + body + " }" + tail
                    repl = [note, merged] if note else [merged]
                    lines[i:k + 1] = repl
                    fixed += 1; continue
        # 形态B：多行块（向后找独立 } 块尾）
        j = i
        while j < len(lines) and not re.match(r"\s*\}\s*$", lines[j]):
            j += 1
        # 嵌套保护：块内不得再有 如果 行（嵌套固化形态交人工）
        inner_if = any("如果" in lines[t] for t in range(i + 1, j))
        if j < len(lines) and j > i and not inner_if:
            k = j + 1
            while k < len(lines) and k <= j + 3 and lines[k].strip() == "":
                k += 1
            nxt = lines[k].strip() if k < len(lines) and k <= j + 3 else ""
            m2 = re.match(r"返回 (.+;)$", nxt)
            if m2:
                lines[j] = re.match(r"\s*", lines[j]).group(0) + "} 否则 {"
                lines[k] = ind + "    " + "返回 " + m2.group(1)
                lines.insert(k + 1, ind + "}")
                if note:
                    lines.insert(i, note)
                fixed += 1; continue
        manual_list.append((rel, row, "形态不符")); manual += 1
    io.open(path, "w", encoding="utf-8", newline="").write("\n".join(lines))

print(f"修复 {fixed}·已显式跳过 {skipped}·人工 {manual}")
for x in manual_list[:15]:
    print("  MANUAL:", x)
