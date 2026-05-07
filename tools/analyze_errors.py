#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析GCC错误并生成分类统计报告"""

import re
import sys
from collections import defaultdict

def main():
    with open('tools/gcc_errors_clean2.txt', 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()
    
    # 提取所有error行
    error_lines = [line for line in content.split('\n') if 'error:' in line]
    
    print(f"=== 修复后GCC错误统计 ===")
    print(f"总错误数: {len(error_lines)}")
    print()
    
    # 按错误大类分类
    categories = defaultdict(int)
    for line in error_lines:
        if 'assignment to' in line and 'from' in line:
            categories['类型赋值不兼容(assignment to...from)'] += 1
        elif 'incompatible types when assigning' in line:
            categories['类型赋值不兼容(incompatible types assigning)'] += 1
        elif 'incompatible types when returning' in line:
            categories['返回类型不兼容'] += 1
        elif 'incompatible type for argument' in line:
            categories['函数参数类型不兼容'] += 1
        elif 'passing argument' in line:
            categories['函数参数类型不匹配'] += 1
        elif 'too few arguments' in line:
            categories['函数参数不足'] += 1
        elif 'conflicting types' in line:
            categories['函数声明冲突'] += 1
        elif "invalid type argument of unary '*'" in line:
            categories['解引用无效类型(*)'] += 1
        elif "invalid type argument of '->'" in line:
            categories['箭头操作符无效类型(->)'] += 1
        elif 'invalid type argument' in line:
            categories['操作符无效类型'] += 1
        elif 'request for member' in line:
            categories['成员访问错误'] += 1
        elif 'invalid use of void' in line:
            categories['void表达式无效使用'] += 1
        elif 'invalid operands to binary' in line:
            categories['二元运算符操作数无效'] += 1
        elif 'invalid use of undefined' in line:
            categories['未定义类型使用'] += 1
        elif 'undeclared' in line:
            categories['未声明标识符'] += 1
        elif 'returning' in line and 'from a function' in line:
            categories['返回类型不兼容'] += 1
        else:
            # 提取错误消息
            m = re.search(r'error: (.+)', line)
            if m:
                categories[f'其他: {m.group(1)[:60]}'] += 1
            else:
                categories['其他: 未知'] += 1
    
    print("=== 按错误大类分类 ===")
    for cat, count in sorted(categories.items(), key=lambda x: -x[1]):
        print(f"  {count}: {cat}")
    
    # 按源文件分类
    file_errors = defaultdict(int)
    for line in error_lines:
        m = re.search(r'([^\\/:]+\.c):', line)
        if m:
            file_errors[m.group(1)] += 1
        else:
            file_errors['unknown'] += 1
    
    print()
    print("=== 按源文件分布 ===")
    for f, count in sorted(file_errors.items(), key=lambda x: -x[1]):
        print(f"  {count}: {f}")
    
    # 赋值不兼容细分 - 目标类型
    print()
    print("=== 类型赋值不兼容 - 目标类型统计 ===")
    target_types = defaultdict(int)
    for line in error_lines:
        m = re.search(r"assignment to '([^']+)'", line)
        if m:
            target_types[m.group(1)] += 1
        m2 = re.search(r"assigning to type '([^']+)'", line)
        if m2:
            target_types[m2.group(1)] += 1
    for t, count in sorted(target_types.items(), key=lambda x: -x[1])[:15]:
        print(f"  {count}: {t}")
    
    # 函数参数不足详情
    print()
    print("=== 函数参数不足 - 函数名 ===")
    few_args = defaultdict(int)
    for line in error_lines:
        if 'too few arguments' in line:
            m = re.search(r"function '([^']+)'", line)
            if m:
                few_args[m.group(1)] += 1
            else:
                few_args['unknown'] += 1
    for f, count in sorted(few_args.items(), key=lambda x: -x[1]):
        print(f"  {count}: {f}")
    
    # 函数声明冲突详情
    print()
    print("=== 函数声明冲突 - 函数名 ===")
    conflicts = defaultdict(int)
    for line in error_lines:
        if 'conflicting types' in line:
            m = re.search(r"for '([^']+)'", line)
            if m:
                conflicts[m.group(1)] += 1
            else:
                conflicts['unknown'] += 1
    for f, count in sorted(conflicts.items(), key=lambda x: -x[1]):
        print(f"  {count}: {f}")
    
    # 对比
    print()
    print("=== 修复效果对比 ===")
    print("  初始: 348")
    print("  P1后: 176")
    print("  P1+P2后: 146")
    print("  P1+P2+P3后(旧C+补丁): 135")
    print(f"  P1+P2+P3后(新cnc重新生成): {len(error_lines)}")
    
    # CNC编译统计
    print()
    print("=== CNC编译统计 ===")
    print("  成功: 26 / 39")
    print("  失败: 13")
    print("  失败文件: IR生成.cn, cnc.cn, 断言.cn, 测试框架.cn, 测试结果.cn,")
    print("           诊断修复.cn, 诊断恢复.cn, 诊断消息表.cn, 作用域构建.cn,")
    print("           语义检查.cn, 声明解析.cn, 类型解析.cn, 语句解析.cn")

if __name__ == '__main__':
    main()
