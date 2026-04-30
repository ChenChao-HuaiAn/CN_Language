#!/usr/bin/env python3
"""详细分类GCC错误 - 特别是expected expression before X"""
import re
from collections import Counter

with open('tools/gcc_errors_only.txt', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# 分析 "expected expression before X" 的具体类型
exp_lines = [l.strip() for l in lines if 'expected expression before' in l]
before_types = Counter()
for l in exp_lines:
    m = re.search(r"before '(\S+)'", l)
    if m:
        before_types[m.group(1)] += 1
    else:
        before_types['<no match>'] += 1

print(f"=== 'expected expression before X' 详细分类 ({len(exp_lines)}个) ===")
for bt, count in before_types.most_common():
    print(f"  {count:4d}: before '{bt}'")

# 分析 "too few arguments" 的详细
few_lines = [l.strip() for l in lines if 'too few arguments' in l]
print(f"\n=== 'too few arguments' 示例 ({len(few_lines)}个) ===")
for l in few_lines[:5]:
    print(f"  {l[:200]}")

# 分析 assignment 类型错误
assign_lines = [l.strip() for l in lines if 'assignment to' in l and 'error:' in l]
assign_types = Counter()
for l in assign_lines:
    m = re.search(r"assignment to '(.+?)' from", l)
    if m:
        assign_types[f"to '{m.group(1)}'"] += 1
    else:
        assign_types['<other>'] += 1

print(f"\n=== assignment 类型错误详细 ({len(assign_lines)}个) ===")
for at, count in assign_types.most_common():
    print(f"  {count:4d}: {at}")

# 分析 passing argument 错误
passing_lines = [l.strip() for l in lines if 'passing argument' in l and 'error:' in l]
print(f"\n=== passing argument 错误示例 ({len(passing_lines)}个) ===")
for l in passing_lines[:8]:
    print(f"  {l[:200]}")

# 分析 conflicting types 错误
conflict_lines = [l.strip() for l in lines if 'conflicting types' in l]
print(f"\n=== conflicting types 错误示例 ({len(conflict_lines)}个) ===")
for l in conflict_lines[:5]:
    print(f"  {l[:200]}")

# 分析 invalid type 错误
invalid_lines = [l.strip() for l in lines if 'invalid type' in l and 'error:' in l]
print(f"\n=== invalid type 错误示例 ({len(invalid_lines)}个) ===")
for l in invalid_lines[:5]:
    print(f"  {l[:200]}")

# 查看词元.c的具体错误分布
print("\n=== 词元.c 错误类型分布 (100个) ===")
ciyuan_lines = [l.strip() for l in lines if '词元.c' in l and 'error:' in l]
ciyuan_types = Counter()
for l in ciyuan_lines:
    m = re.search(r'error:\s*(.+)', l)
    if m:
        msg = m.group(1).strip()[:80]
        ciyuan_types[msg] += 1
for ct, count in ciyuan_types.most_common(10):
    print(f"  {count:4d}: {ct}")
