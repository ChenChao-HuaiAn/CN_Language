# -*- coding: utf-8 -*-
"""1241 修复脚本 v2（两遍法）：坏形态
    如果 (X) { A; }
    [// 注释]
    } 否则 {            ← 多余前导 }（语法坏）
        B;
    }
重组为：
    [// 注释]
    如果 (X) { A; } 否则 { B; }
判定：} 否则 { 往上最近的非注释行为单行闭合如果（IF1）才算坏；否则为合法多行形态跳过。
用后即删。"""
import io, re, glob

ROOT = r"C:\Users\ChenChao\Documents\CN\wt1241"
FILES = sorted(glob.glob(ROOT + "/CN语言编译器v2/**/*.cn", recursive=True))
print(f"扫描 {len(FILES)} 个 v2 源文件")
ROOT = r"C:\Users\ChenChao\Documents\CN\wt1241"
BAD = re.compile(r"^\s*\} 否则 \{\s*$")
ORPH = re.compile(r"^\s*否则 \{.+\}\s*$")  # 孤立单行否则块（如果行已闭合·否则丢前导}）
IF1 = re.compile(r"^\s*如果 \(.*\{.*\}\s*$")
END = re.compile(r"^\s*\}\s*$")

total = 0
for rel in FILES:
    path = rel
    with io.open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    fixes = []  # (j起点, k终点含, 替换行列表)
    i, n = 0, len(lines)
    while i < n:
        if BAD.match(lines[i]):
            j = i - 1
            while j >= 0 and lines[j].lstrip().startswith("//"):
                j -= 1
            assert j >= 0, f"{rel}:{i+1} 找不到如果行"
            if END.match(lines[j]):
                # 模式三：多行如果块已闭合，孤立 } 否则 { 追加其后——
                # j 行改写为 } 否则 {（前导 } 即块闭合），BAD 行删除
                ind_j = len(lines[j]) - len(lines[j].lstrip())
                ind_i = len(lines[i]) - len(lines[i].lstrip())
                if ind_j != ind_i:
                    i += 1
                    continue
                notes = [lines[t] for t in range(j + 1, i)]
                merged = lines[j].rstrip()[:-1].rstrip() + " } 否则 {"
                fixes.append((j, i, notes + [merged]))
                i += 1
                continue
            if not IF1.match(lines[j]):
                i += 1  # 合法多行形态
                continue
            # 缩进判据：坏形态的孤立 } 否则 { 与如果行同层级；外层如果链的
            # } 否则 { 缩进浅于内层单行如果（主.cn 否则如果链误修教训）
            ind_j = len(lines[j]) - len(lines[j].lstrip())
            ind_i = len(lines[i]) - len(lines[i].lstrip())
            if ind_j != ind_i:
                i += 1
                continue
            head = lines[j].rstrip()
            assert head.endswith("}"), f"{rel}:{j+1} 如果行尾非右花括号"
            k = i + 1
            body = []
            while k < n and not END.match(lines[k]):
                body.append(lines[k].strip()); k += 1
            assert k < n and len(body) >= 1, f"{rel}:{i+1} 否则块体异常"
            merged = head[:-1].rstrip() + " } 否则 { " + " ".join(body) + " }"
            notes = [lines[t] for t in range(j + 1, i) if lines[t].lstrip().startswith("//")]
            fixes.append((j, k, notes + [merged]))
            i = k + 1
        elif ORPH.match(lines[i]):
            # 孤立单行否则块：并入上一行单行闭合如果
            j = i - 1
            while j >= 0 and lines[j].lstrip().startswith("//"):
                j -= 1
            assert j >= 0, f"{rel}:{i+1} 否则找不到如果行"
            if IF1.match(lines[j]):
                ind_j = len(lines[j]) - len(lines[j].lstrip())
                ind_i = len(lines[i]) - len(lines[i].lstrip())
                if ind_j == ind_i:
                    merged = lines[j].rstrip() + " " + lines[i].strip()
                    notes = [lines[t] for t in range(j + 1, i)]
                    fixes.append((j, i, notes + [merged]))
                    i += 1
                    continue
            i += 1
        else:
            i += 1
    for j, k, rep in sorted(fixes, reverse=True):
        lines[j:k + 1] = rep
    if fixes:
        with io.open(path, "w", encoding="utf-8", newline="") as f:
            f.write("\n".join(lines))
        print(f"修复 {rel} x{len(fixes)}")
        total += len(fixes)
print(f"共 {total} 处")
