#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析GCC警告"""

import re
from collections import Counter

with open('tools/gcc_errors_current_run.txt', 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()

lines = content.split('\n')
warning_lines = [l for l in lines if 'warning:' in l]

# 按警告类型分类
warning_types = Counter()
for line in warning_lines:
    m = re.search(r'warning:\s*(.+?)(?:\s*\[)', line)
    if m:
        warning_types[m.group(1).strip()] += 1
    else:
        m2 = re.search(r'warning:\s*(.+)', line)
        if m2:
            warning_types[m2.group(1).strip()[:80]] += 1

print('=== 警告类型分类 ===')
for wtype, count in warning_types.most_common():
    print(f'  {count:4d} | {wtype}')
