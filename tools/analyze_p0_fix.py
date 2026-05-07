#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""P0修复后GCC错误详细分析脚本"""

import re
from collections import Counter, defaultdict

def analyze_errors(filename):
    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    errors = []
    warnings = []
    
    for line in lines:
        line = line.strip()
        if ': error:' in line:
            errors.append(line)
        elif ': warning:' in line:
            warnings.append(line)
    
    print(f"=== P0修复后GCC错误统计 ===")
    print(f"总错误数: {len(errors)}")
    print(f"总警告数: {len(warnings)}")
    print()
    
    # 按错误类型分类
    error_types = Counter()
    error_details = defaultdict(list)
    
    for line in errors:
        # 提取错误消息
        m = re.search(r': error:\s+(.+)$', line)
        if not m:
            continue
        msg = m.group(1)
        
        # 提取文件名
        fm = re.match(r'^(.+?):(\d+):', line)
        fname = fm.group(1).split('\\')[-1] if fm else 'unknown'
        
        if 'invalid use of void expression' in msg:
            etype = 'void表达式无效使用'
        elif 'incompatible types when assigning' in msg and 'long long' in msg:
            etype = '赋值: long long ← 不兼容类型'
        elif 'incompatible types when assigning' in msg and 'struct' in msg:
            etype = '赋值: struct ← 不兼容类型'
        elif 'incompatible types when assigning' in msg and 'char' in msg and '*' in msg:
            etype = '赋值: char* ← 不兼容类型'
        elif 'incompatible types when assigning' in msg and 'char' in msg:
            etype = '赋值: char ← 不兼容类型'
        elif 'incompatible types when assigning' in msg:
            etype = '赋值: 其他类型不兼容'
        elif 'incompatible type for argument' in msg:
            etype = '函数参数类型不匹配'
        elif 'conflicting types' in msg:
            etype = '函数声明冲突'
        elif 'too few arguments' in msg:
            etype = '函数参数不足'
        elif 'dereferencing pointer' in msg or 'dereferencing' in msg:
            etype = '解引用无效类型'
        elif 'member' in msg and ('not a structure' in msg or 'not a union' in msg):
            etype = '成员访问错误'
        elif 'request for member' in msg:
            etype = '成员访问错误'
        elif 'subscripted value' in msg:
            etype = '数组下标错误'
        elif 'incompatible types when returning' in msg:
            etype = '返回类型不兼容'
        elif 'expected' in msg:
            etype = '语法/声明错误'
        elif 'undeclared' in msg:
            etype = '未声明标识符'
        elif 'redefinition' in msg:
            etype = '重定义错误'
        else:
            etype = f'其他: {msg[:50]}'
        
        error_types[etype] += 1
        error_details[etype].append((fname, msg[:80]))
    
    print("=== 按错误类型分类 ===")
    for etype, count in error_types.most_common():
        print(f"  {etype}: {count}")
    
    print()
    print("=== 按源文件分布 ===")
    file_errors = Counter()
    for line in errors:
        fm = re.match(r'^(.+?):(\d+):', line)
        if fm:
            fname = fm.group(1).split('\\')[-1]
            file_errors[fname] += 1
    
    for fname, count in file_errors.most_common():
        print(f"  {fname}: {count}")
    
    # 详细分析关键错误类型
    print()
    print("=== 关键错误类型详细 ===")
    
    # 1. long long赋值错误
    ll_errors = [l for l in errors if 'long long' in l and 'assigning' in l]
    print(f"\n--- long long赋值错误: {len(ll_errors)} ---")
    for e in ll_errors[:10]:
        print(f"  {e[:120]}")
    
    # 2. struct赋值错误
    struct_errors = [l for l in errors if 'struct' in l and 'assigning' in l]
    print(f"\n--- struct赋值错误: {len(struct_errors)} ---")
    for e in struct_errors[:10]:
        print(f"  {e[:120]}")
    
    # 3. 成员访问错误
    member_errors = [l for l in errors if 'member' in l.lower() and ('structure' in l.lower() or 'union' in l.lower())]
    print(f"\n--- 成员访问错误: {len(member_errors)} ---")
    for e in member_errors[:10]:
        print(f"  {e[:120]}")
    
    # 4. void表达式错误
    void_errors = [l for l in errors if 'void expression' in l]
    print(f"\n--- void表达式错误: {len(void_errors)} ---")
    for e in void_errors[:10]:
        print(f"  {e[:120]}")
    
    # 5. 函数声明冲突
    conflict_errors = [l for l in errors if 'conflicting types' in l]
    print(f"\n--- 函数声明冲突: {len(conflict_errors)} ---")
    for e in conflict_errors[:10]:
        print(f"  {e[:120]}")
    
    # 6. 函数参数不匹配
    arg_errors = [l for l in errors if 'argument' in l and 'incompatible' in l]
    print(f"\n--- 函数参数类型不匹配: {len(arg_errors)} ---")
    for e in arg_errors[:10]:
        print(f"  {e[:120]}")
    
    # 7. char*相关错误
    char_errors = [l for l in errors if 'char' in l and ('assigning' in l or 'incompatible' in l)]
    print(f"\n--- char相关错误: {len(char_errors)} ---")
    for e in char_errors[:10]:
        print(f"  {e[:120]}")

if __name__ == '__main__':
    analyze_errors('tools/gcc_errors_p0_fix.txt')
