#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析当前GCC编译错误"""

import re
import sys
from collections import Counter, defaultdict

def main():
    with open('tools/gcc_errors_current_run.txt', 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    lines = content.split('\n')

    # 统计错误和警告
    error_lines = [l for l in lines if 'error:' in l]
    warning_lines = [l for l in lines if 'warning:' in l]

    print(f'=== 总体统计 ===')
    print(f'总错误数: {len(error_lines)}')
    print(f'总警告数: {len(warning_lines)}')
    print()

    # 按错误类型分类
    error_types = Counter()
    for line in error_lines:
        m = re.search(r'error:\s*(.+?)(?:\s*\[)', line)
        if m:
            error_types[m.group(1).strip()] += 1
        else:
            m2 = re.search(r'error:\s*(.+)', line)
            if m2:
                error_types[m2.group(1).strip()[:80]] += 1

    print('=== 按错误类型分类（前15） ===')
    for etype, count in error_types.most_common(15):
        print(f'  {count:4d} | {etype}')
    print()

    # 按源文件分布
    file_errors = Counter()
    for line in error_lines:
        m = re.match(r'([^:]+)', line)
        if m:
            fname = m.group(1).strip()
            parts = fname.replace('\\', '/').split('/')
            short = '/'.join(parts[-2:]) if len(parts) >= 2 else fname
            file_errors[short] += 1

    print('=== 按源文件分布 ===')
    for fname, count in file_errors.most_common():
        print(f'  {count:4d} | {fname}')
    print()

    # 更细粒度的错误模式
    print('=== 详细错误模式（前25） ===')
    patterns = Counter()
    for line in error_lines:
        if 'incompatible types' in line and 'assigning to' in line:
            m = re.search(r"assigning to type '([^']+)'.*from type '([^']+)'", line)
            if m:
                to_type = m.group(1)[:50]
                from_type = m.group(2)[:50]
                patterns[f'赋值不兼容: {to_type} <- {from_type}'] += 1
            else:
                patterns['赋值不兼容(其他)'] += 1
        elif 'incompatible types' in line:
            patterns['类型不兼容(其他)'] += 1
        elif 'passing argument' in line and 'makes pointer from integer' in line:
            patterns['参数: 整数转指针(无强制转换)'] += 1
        elif 'passing argument' in line and 'makes integer from pointer' in line:
            patterns['参数: 指针转整数(无强制转换)'] += 1
        elif 'passing argument' in line:
            m = re.search(r"argument (\d+).*expected '([^']+)'.*type '([^']+)'", line)
            if m:
                patterns[f'参数类型不匹配: 参数{m.group(1)}'] += 1
            else:
                patterns['参数类型不匹配(其他)'] += 1
        elif 'conflicting types' in line:
            patterns['函数声明冲突'] += 1
        elif 'invalid use of void' in line:
            patterns['void表达式无效使用'] += 1
        elif 'dereferencing' in line and 'void' in line:
            patterns['解引用void指针'] += 1
        elif 'assignment to' in line and 'from' in line:
            m = re.search(r"assignment to '([^']+)'.*from '([^']+)'", line)
            if m:
                patterns[f'赋值: {m.group(1)[:40]} <- {m.group(2)[:40]}'] += 1
            else:
                patterns['赋值不兼容(其他)'] += 1
        elif 'request for member' in line:
            patterns['成员访问错误'] += 1
        elif 'not a structure or union' in line:
            patterns['非结构体/联合体成员访问'] += 1
        elif 'too few arguments' in line:
            patterns['函数参数不足'] += 1
        elif 'declared inside parameter list' in line:
            patterns['参数列表内声明'] += 1
        else:
            m = re.search(r'error:\s*(.{1,80})', line)
            if m:
                patterns[m.group(1).strip()] += 1
            else:
                patterns['未知错误'] += 1

    for pattern, count in patterns.most_common(25):
        print(f'  {count:4d} | {pattern}')
    print()

    # 按文件和错误类型的交叉分析
    print('=== 按文件x错误类型交叉分析 ===')
    file_type_errors = defaultdict(Counter)
    for line in error_lines:
        m = re.match(r'([^:]+)', line)
        if m:
            fname = m.group(1).strip()
            parts = fname.replace('\\', '/').split('/')
            short = '/'.join(parts[-2:]) if len(parts) >= 2 else fname
            
            if 'incompatible types' in line and 'assigning to' in line:
                etype = '赋值类型不兼容'
            elif 'passing argument' in line:
                etype = '函数参数类型不匹配'
            elif 'conflicting types' in line:
                etype = '函数声明冲突'
            elif 'invalid use of void' in line or 'dereferencing' in line and 'void' in line:
                etype = 'void指针解引用'
            elif 'assignment to' in line and 'from' in line:
                etype = '赋值类型不兼容'
            elif 'request for member' in line or 'not a structure' in line:
                etype = '成员访问错误'
            elif 'too few arguments' in line:
                etype = '函数参数不足'
            else:
                etype = '其他'
            file_type_errors[short][etype] += 1

    for fname in sorted(file_type_errors.keys()):
        types = file_type_errors[fname]
        total = sum(types.values())
        type_str = ', '.join(f'{t}:{c}' for t, c in types.most_common())
        print(f'  {total:4d} | {fname} | {type_str}')

if __name__ == '__main__':
    main()
