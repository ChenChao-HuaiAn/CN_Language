#!/usr/bin/env python3
"""分类GCC错误"""
import re
from collections import Counter

with open('tools/gcc_errors_only.txt', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# 提取错误类型
error_types = []
for line in lines:
    line = line.strip()
    m = re.search(r'error:\s*(.+)', line)
    if m:
        msg = m.group(1).strip()
        # 归类
        if 'undeclared' in msg:
            error_types.append('undeclared identifier')
        elif 'expected expression before' in msg:
            error_types.append('expected expression before X')
        elif 'incompatible types when assigning' in msg:
            error_types.append('incompatible types (assignment)')
        elif 'assignment to' in msg and 'from incompatible' in msg:
            error_types.append('incompatible assignment')
        elif 'incompatible type for argument' in msg:
            error_types.append('incompatible argument type')
        elif 'has no member' in msg:
            error_types.append('has no member')
        elif 'request for member' in msg and 'not a structure or union' in msg:
            error_types.append('member access on non-struct')
        elif 'dereferencing pointer' in msg:
            error_types.append('dereferencing pointer to incomplete type')
        elif 'passing argument' in msg and 'incompatible' in msg:
            error_types.append('passing incompatible argument')
        elif 'redefinition' in msg:
            error_types.append('redefinition')
        elif 'conflicting types' in msg:
            error_types.append('conflicting types')
        elif 'subscripted value' in msg:
            error_types.append('subscripted value is not array')
        elif 'void value' in msg:
            error_types.append('void value not ignored')
        elif 'too many arguments' in msg:
            error_types.append('too many arguments')
        elif 'too few arguments' in msg:
            error_types.append('too few arguments')
        elif 'invalid type' in msg:
            error_types.append('invalid type')
        elif 'expected' in msg and 'before' in msg:
            error_types.append('expected X before Y')
        elif 'cannot' in msg:
            error_types.append('cannot do X')
        elif 'implicit declaration' in msg:
            error_types.append('implicit declaration')
        else:
            error_types.append(msg[:80])

counter = Counter(error_types)
print(f'总错误数: {len(lines)}')
print()
print('=== 按错误类型分类 ===')
for et, count in counter.most_common():
    print(f'  {count:4d}: {et}')

# 按源文件分类
print()
print('=== 按源文件分类 ===')
file_errors = Counter()
for line in lines:
    line = line.strip()
    m = re.match(r'(.+?):\d+:\d+: error:', line)
    if m:
        filepath = m.group(1)
        # 只取文件名
        parts = filepath.replace('\\', '/').split('/')
        short = '/'.join(parts[-2:]) if len(parts) >= 2 else parts[-1]
        file_errors[short] += 1

for f, count in file_errors.most_common():
    print(f'  {count:4d}: {f}')

# 输出代表性错误示例
print()
print('=== 代表性错误示例 ===')
seen_types = set()
for line in lines:
    line = line.strip()
    m = re.search(r'error:\s*(.+)', line)
    if m:
        msg = m.group(1).strip()
        # 简化类型
        et = None
        if 'undeclared' in msg: et = 'undeclared identifier'
        elif 'expected expression before' in msg: et = 'expected expression before X'
        elif 'incompatible types when assigning' in msg: et = 'incompatible types (assignment)'
        elif 'assignment to' in msg and 'from incompatible' in msg: et = 'incompatible assignment'
        elif 'has no member' in msg: et = 'has no member'
        elif 'request for member' in msg and 'not a structure' in msg: et = 'member access on non-struct'
        elif 'dereferencing pointer' in msg: et = 'dereferencing pointer to incomplete type'
        elif 'passing argument' in msg and 'incompatible' in msg: et = 'passing incompatible argument'
        elif 'conflicting types' in msg: et = 'conflicting types'
        elif 'void value' in msg: et = 'void value not ignored'
        
        if et and et not in seen_types:
            seen_types.add(et)
            print(f'  [{et}]: {line[:150]}')
