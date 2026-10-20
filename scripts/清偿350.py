# -*- coding: utf-8 -*-
"""#1241 全量清偿脚本：按 pre1_warn_list.txt 的 (文件,行) 精确修复
「检查 结果.正常 后未处理错误分支」警告——在 如果(…正常){…} 块后补
否则 { <原返回>; }（原 miss 返回移入否则块·语义等价显式化）。
用法：python 清偿350.py（在仓库根跑；先备份或依赖 git 回滚）
"""
import re, os, sys

清单 = 'pre1_warn_list.txt'
by_file = {}
for l in open(清单, encoding='utf-8', errors='replace'):
    m = re.match(r'(.+?[.]cn):(\d+):\d+:', l)
    if m:
        by_file.setdefault(m.group(1).replace('/', os.sep), set()).add(int(m.group(2)))

修复数 = 0
skip = []
for p, rowset in sorted(by_file.items()):
    if not os.path.exists(p):
        skip.append((p, '文件不存在'))
        continue
    lines = open(p, encoding='utf-8').read().splitlines(keepends=True)
    changed = False
    done_rows = set()
    for row in sorted(rowset):
        if row - 1 >= len(lines) or row in done_rows:
            continue
        i = row - 1
        line = lines[i]
        # 形态：如果 (X.正常) { … } 且同块或紧随有 miss 返回（返回 0;/返回 X.错误;/返回 假; 等）
        if '.正常' not in line or '如果' not in line:
            continue
        # 向后找块尾「}」与紧随的返回行（最多 3 行）
        j = i
        while j < len(lines) and '}' not in lines[j]:
            j += 1
        if j + 1 < len(lines) and re.match(r'\s*返回 ', lines[j+1]):
            ret = lines[j+1].strip()
            indent = re.match(r'\s*', lines[j+1]).group(0)
            # 在返回行后插入 否则 块
            ins = indent + '} 否则 {\n' + indent + '    ' + ret + '\n' + indent + '}'
            # 原返回行删除
            del lines[j+1]
            lines[j] = lines[j].rstrip('\n') + '\n' + ins + '\n'
            changed = True
            done_rows.add(row)
            修复数 += 1
        else:
            skip.append((p, f'行{row} 形态不符'))
    if changed:
        open(p, 'w', encoding='utf-8').write(''.join(lines))

print('修复合计:', 修复数)
for x in skip[:10]:
    print('  SKIP:', x)
