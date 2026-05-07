#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析GCC编译错误基线"""

import re
import sys
from collections import defaultdict

def analyze_errors(filename):
    """分析GCC错误文件"""
    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    
    lines = content.split('\n')
    
    # 统计变量
    error_count = 0
    warning_count = 0
    file_errors = defaultdict(lambda: {'errors': 0, 'warnings': 0, 'error_types': defaultdict(int)})
    error_categories = defaultdict(int)
    
    # 错误分类模式
    patterns = {
        '指针→整数 (int-conversion)': r'makes integer from pointer without a cast|makes pointer from integer without a cast',
        '不兼容指针类型 (incompatible-pointer-types)': r'incompatible pointer type|incompatible types when assigning',
        '未声明标识符 (undeclared)': r'undeclared \(first use',
        'const限定符丢弃 (discarded-qualifiers)': r'discards .const. qualifier',
        '解引用void指针 (dereferencing void *)': r'dereferencing .void \*',
        '非结构体/联合体成员 (not a structure or union)': r'not a structure or union',
        '函数类型冲突 (conflicting types)': r'conflicting types',
        '无效一元操作 (invalid type argument)': r'invalid type argument of unary',
        '结构体无成员 (has no member)': r'has no member named',
        '返回类型不兼容 (incompatible return)': r'incompatible types when returning',
        'GEP结果类型降级 (char*→char)': r"assignment to 'char \*' from 'char'",
    }
    
    current_file = None
    
    for line in lines:
        # 提取文件名
        file_match = re.match(r'c:\\.*?src_cn\\(.+?\.c):', line)
        if file_match:
            current_file = file_match.group(1)
        
        # 统计错误
        if ': error:' in line:
            error_count += 1
            if current_file:
                file_errors[current_file]['errors'] += 1
            
            # 分类错误
            for cat_name, pattern in patterns.items():
                if re.search(pattern, line):
                    error_categories[cat_name] += 1
                    if current_file:
                        file_errors[current_file]['error_types'][cat_name] += 1
                    break
            else:
                # 未分类的错误
                error_match = re.search(r': error: (.+?)(?:\[|-)', line)
                if error_match:
                    msg = error_match.group(1).strip()[:60]
                else:
                    msg = line.strip()[:80]
                error_categories[f'其他: {msg}'] += 1
                if current_file:
                    file_errors[current_file]['error_types']['其他'] += 1
        
        # 统计警告
        if ': warning:' in line:
            warning_count += 1
            if current_file:
                file_errors[current_file]['warnings'] += 1
    
    return error_count, warning_count, file_errors, error_categories

def main():
    filename = 'tools/gcc_errors_baseline.txt'
    error_count, warning_count, file_errors, error_categories = analyze_errors(filename)
    
    print("=" * 70)
    print("CN语言自举编译GCC错误基线报告")
    print("=" * 70)
    print()
    
    print(f"总错误数: {error_count}")
    print(f"总警告数: {warning_count}")
    print()
    
    # 按错误数排序文件
    print("-" * 70)
    print("各文件错误统计（按错误数降序）")
    print("-" * 70)
    sorted_files = sorted(file_errors.items(), key=lambda x: x[1]['errors'], reverse=True)
    for fname, stats in sorted_files:
        if stats['errors'] > 0 or stats['warnings'] > 0:
            print(f"  {fname}: {stats['errors']} 错误, {stats['warnings']} 警告")
    
    print()
    
    # 错误分类统计
    print("-" * 70)
    print("错误分类统计（按数量降序）")
    print("-" * 70)
    sorted_cats = sorted(error_categories.items(), key=lambda x: x[1], reverse=True)
    for cat, count in sorted_cats:
        pct = count * 100.0 / error_count if error_count > 0 else 0
        print(f"  {cat}: {count} ({pct:.1f}%)")
    
    print()
    
    # 各文件的错误类型详情
    print("-" * 70)
    print("各文件错误类型详情")
    print("-" * 70)
    for fname, stats in sorted_files:
        if stats['errors'] > 0:
            print(f"\n  [{fname}] - {stats['errors']} 错误:")
            sorted_types = sorted(stats['error_types'].items(), key=lambda x: x[1], reverse=True)
            for etype, count in sorted_types:
                print(f"    - {etype}: {count}")
    
    # 无错误的文件
    print()
    print("-" * 70)
    print("无错误的文件")
    print("-" * 70)
    no_error_files = [f for f, s in sorted_files if s['errors'] == 0]
    for f in no_error_files:
        print(f"  {f}")
    
    print()
    print("=" * 70)
    print("报告结束")
    print("=" * 70)

if __name__ == '__main__':
    main()
