# -*- coding: utf-8 -*-
"""PRE-1a 存量批量补空否则（临时工具·清完可删）：对清单逐处补「} 否则 {\n<缩>}\n」。
语义=miss 掉出块继续（与原隐式行为零变化）·检查器满足·语法已试刀验证。"""
import re, glob, io

清单 = 'docs/1360-PRE1A存量清单-103处.md'
文 = io.open(清单, encoding='utf-8').read()
条们 = re.findall(r'  (\S+):(\d+)  \.(\w+)(?:  in (\w+))?', 文)
print('清单处数', len(条们))

按文件 = {}
for f, l, x, fn in 条们:
    按文件.setdefault(f, []).append(int(l))

总补 = 0
跳过 = []
for f, 行单 in sorted(按文件.items()):
    p = 'CN语言编译器v2/' + f
    源 = io.open(p, encoding='utf-8').read()
    行们 = 源.split('\n')
    # 从后往前改（行号不漂移）
    for 行号 in sorted(行单, reverse=True):
        i = 行号 - 1
        # 找「如果 (X.正常) {」行（可能有前导空白）
        while i < len(行们) and not re.match(r'^ *如果 \(\w+\.正常\) *\{ *$', 行们[i]):
            i += 1
            if i > 行号 + 2:
                break
        if i >= len(行们) or not re.match(r'^ *如果 \(\w+\.正常\) *\{ *$', 行们[i]):
            # 单行形态「如果 (X.正常) { ...; }」——跳过（少见面·后续甄别）
            跳过.append((f, 行号))
            continue
        缩 = len(行们[i]) - len(行们[i].lstrip())
        缩串 = ' ' * 缩
        # 括号配对找收口
        深 = 0
        j = i
        收口 = None
        while j < len(行们):
            深 += 行们[j].count('{') - 行们[j].count('}')
            if 深 == 0:
                收口 = j
                break
            j += 1
        if 收口 is None:
            跳过.append((f, 行号))
            continue
        收口行 = 行们[收口]
        if 收口行.strip() == '}':
            # 独立 } 行 → 后插空否则块
            行们[收口] = 收口行 + '\n' + 缩串 + '否则 {\n' + 缩串 + '}'
        elif 收口行.rstrip().endswith('}'):
            # 语句 } 同行 → 行尾接 否则 {
            行们[收口] = 收口行.rstrip() + ' 否则 {\n' + 缩串 + '}'
        else:
            跳过.append((f, 行号))
            continue
        总补 += 1
    io.open(p, 'w', encoding='utf-8', newline='').write('\n'.join(行们))

print('补空否则', 总补, '处·跳过', len(跳过), '处')
for f, l in 跳过[:12]:
    print('  跳过:', f, l)
