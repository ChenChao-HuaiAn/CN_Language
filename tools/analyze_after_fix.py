#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析修复后的GCC错误 - 与修复前对比"""

import re
import sys
from collections import Counter

def classify_error(msg):
    """分类错误消息"""
    if 'assignment to' in msg and 'long long int' in msg and 'char *' in msg:
        return '赋值: long long int <- char*'
    elif 'assignment to' in msg and 'long long int' in msg and 'struct' in msg:
        return '赋值: long long int <- struct指针'
    elif 'assignment to' in msg and 'long long int' in msg:
        return '赋值: long long int <- 其他指针/类型'
    elif 'assignment to' in msg and 'char *' in msg and 'incompat' in msg:
        return '赋值: char* <- 不兼容类型'
    elif 'assignment to' in msg and 'char' in msg and 'char *' in msg:
        return '赋值: char <- char*'
    elif 'assignment to' in msg and 'struct' in msg:
        return '赋值: struct <- 不兼容类型'
    elif 'assignment to' in msg:
        return '赋值: 其他不兼容'
    elif 'incompatible type for argument' in msg:
        return '函数参数类型不匹配'
    elif 'passing argument' in msg and 'incompat' in msg:
        return '函数参数类型不匹配'
    elif 'too few arguments' in msg:
        return '函数参数不足'
    elif 'conflicting types' in msg:
        return '函数声明冲突'
    elif 'request for member' in msg:
        return '成员访问错误'
    elif 'invalid type argument' in msg and 'unary' in msg:
        return '解引用无效类型'
    elif 'incompatible types when assigning' in msg:
        return '赋值类型不兼容(其他)'
    elif 'return' in msg and 'incompat' in msg:
        return '返回类型不兼容'
    elif 'dereferencing' in msg:
        return '解引用指针类型错误'
    elif 'passing argument' in msg:
        return '函数参数类型不匹配'
    elif 'undeclared' in msg or 'undeclared' in msg:
        return '变量未声明'
    elif 'invalid use of void' in msg:
        return 'void表达式无效使用'
    elif 'invalid operands to binary' in msg:
        return '二元运算操作数无效'
    elif 'invalid use of undefined type' in msg:
        return '未定义类型使用'
    elif 'incompatible types when' in msg:
        return '类型不兼容(其他)'
    else:
        return '其他'

def extract_file(line):
    """从错误行提取文件名（不含行号）"""
    # 格式: src_cn/语义分析器/类型系统.c:1328:6: error: ...
    m = re.match(r'(.+?):\d+:\d+: error:', line)
    if m:
        return m.group(1).strip()
    # 格式: src_cn/语义分析器/类型系统.c:1328: error: ...
    m = re.match(r'(.+?):\d+: error:', line)
    if m:
        return m.group(1).strip()
    return None

def analyze_file(filepath, label):
    """分析一个GCC错误文件"""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        content = f.read()

    lines = content.split('\n')
    error_lines = [l for l in lines if ': error:' in l]
    warning_lines = [l for l in lines if ': warning:' in l]

    # 按错误类型分类
    error_types = Counter()
    for line in error_lines:
        m = re.search(r': error: (.+)', line)
        if m:
            msg = m.group(1).strip()
            etype = classify_error(msg)
            error_types[etype] += 1

    # 按源文件分类
    file_errors = Counter()
    for line in error_lines:
        fname = extract_file(line)
        if fname:
            # 简化路径
            parts = fname.replace('\\', '/').split('/')
            # 找到src_cn后的部分
            short = fname
            for i, p in enumerate(parts):
                if p == 'src_cn' and i + 1 < len(parts):
                    short = '/'.join(parts[i+1:])
                    break
            file_errors[short] += 1

    return len(error_lines), len(warning_lines), error_types, file_errors

def main():
    # 分析修复后的文件
    after_errors, after_warnings, after_types, after_files = analyze_file(
        'tools/gcc_errors_after_fix.txt', '修复后')

    # 分析修复前的文件
    before_file = 'tools/gcc_errors_current_run.txt'  # 204错误/111警告的基准
    before_errors, before_warnings, before_types, before_files = analyze_file(
        before_file, '修复前')

    print("=" * 70)
    print("RC1-RC5修复效果验证报告")
    print("=" * 70)
    print()

    # 总体对比
    print("=== 1. 总体对比 ===")
    print(f"  {'指标':<20} {'修复前':>10} {'修复后':>10} {'变化':>10}")
    print(f"  {'-'*50}")
    print(f"  {'GCC错误数':<20} {before_errors:>10} {after_errors:>10} {after_errors - before_errors:>+10}")
    print(f"  {'GCC警告数':<20} {before_warnings:>10} {after_warnings:>10} {after_warnings - before_warnings:>+10}")

    # 按错误类型对比
    print()
    print("=== 2. 按错误类型对比 ===")
    all_types = set(list(before_types.keys()) + list(after_types.keys()))
    print(f"  {'错误类型':<35} {'修复前':>8} {'修复后':>8} {'变化':>8}")
    print(f"  {'-'*60}")
    for etype in sorted(all_types, key=lambda x: after_types.get(x, 0) - before_types.get(x, 0)):
        b = before_types.get(etype, 0)
        a = after_types.get(etype, 0)
        change = a - b
        marker = "↓" if change < 0 else ("↑" if change > 0 else "=")
        print(f"  {etype:<35} {b:>8} {a:>8} {change:>+7} {marker}")

    # 按源文件对比
    print()
    print("=== 3. 按源文件错误分布对比 ===")
    all_files = set(list(before_files.keys()) + list(after_files.keys()))
    print(f"  {'源文件':<30} {'修复前':>8} {'修复后':>8} {'变化':>8}")
    print(f"  {'-'*55}")
    for fname in sorted(all_files, key=lambda x: after_files.get(x, 0) - before_files.get(x, 0)):
        b = before_files.get(fname, 0)
        a = after_files.get(fname, 0)
        change = a - b
        marker = "↓" if change < 0 else ("↑" if change > 0 else "=")
        print(f"  {fname:<30} {b:>8} {a:>8} {change:>+7} {marker}")

    # 修复后错误类型详细分布
    print()
    print("=== 4. 修复后错误类型详细分布 ===")
    for etype, count in after_types.most_common():
        pct = count / after_errors * 100 if after_errors > 0 else 0
        print(f"  {count:4d} ({pct:5.1f}%)  {etype}")

    # 修复后文件错误分布
    print()
    print("=== 5. 修复后文件错误分布 ===")
    for fname, count in after_files.most_common():
        pct = count / after_errors * 100 if after_errors > 0 else 0
        print(f"  {count:4d} ({pct:5.1f}%)  {fname}")

    # CNC编译成功率
    print()
    print("=== 6. CNC编译成功率 ===")
    total_cn = 36
    success_cn = 25
    fail_cn = 11
    print(f"  总.cn文件数: {total_cn}")
    print(f"  成功生成.c: {success_cn} ({success_cn/total_cn*100:.1f}%)")
    print(f"  CNC编译失败: {fail_cn} ({fail_cn/total_cn*100:.1f}%)")
    print()
    print("  CNC编译失败的文件:")
    failed_files = [
        "诊断消息表.cn - 常量初始化表达式必须是编译时常量",
        "诊断恢复.cn - 解析器无法识别token",
        "诊断修复.cn - 解析器无法识别token",
        "类型解析.cn - 解析器无法识别token",
        "声明解析.cn - 解析器无法识别token",
        "语句解析.cn - 解析器无法识别token",
        "语义检查.cn - 解析/语义错误",
        "作用域构建.cn - 解析/语义错误",
        "IR生成.cn - 解析/语义错误",
        "主程序/cnc.cn - 解析/语义错误",
        "词元.cn - 之前已成功(首次编译)",
    ]
    for f in failed_files:
        print(f"    - {f}")

    # 修复效果评估
    print()
    print("=== 7. 修复效果评估 ===")
    print("  RC1修复(寄存器类型推断):")
    ll_before = before_types.get('赋值: long long int <- 其他指针/类型', 0)
    ll_after = after_types.get('赋值: long long int <- 其他指针/类型', 0)
    print(f"    long long int赋值错误: {ll_before} -> {ll_after} (变化: {ll_after - ll_before:+d})")

    print("  RC3修复(char/char*类型):")
    char_before = before_types.get('赋值: char <- char*', 0) + before_types.get('赋值: char* <- 不兼容类型', 0)
    char_after = after_types.get('赋值: char <- char*', 0) + after_types.get('赋值: char* <- 不兼容类型', 0)
    print(f"    char/char*赋值错误: {char_before} -> {char_after} (变化: {char_after - char_before:+d})")

    print("  RC4修复(成员访问. vs ->):")
    member_before = before_types.get('成员访问错误', 0)
    member_after = after_types.get('成员访问错误', 0)
    print(f"    成员访问错误: {member_before} -> {member_after} (变化: {member_after - member_before:+d})")

    print("  RC5修复(函数声明一致性):")
    conflict_before = before_types.get('函数声明冲突', 0)
    conflict_after = after_types.get('函数声明冲突', 0)
    print(f"    函数声明冲突: {conflict_before} -> {conflict_after} (变化: {conflict_after - conflict_before:+d})")

    print("  RC2修复(IR类型默认值):")
    param_before = before_types.get('函数参数类型不匹配', 0)
    param_after = after_types.get('函数参数类型不匹配', 0)
    print(f"    函数参数类型不匹配: {param_before} -> {param_after} (变化: {param_after - param_before:+d})")

    # 下一步建议
    print()
    print("=== 8. 剩余错误分析和下一步建议 ===")
    print("  主要剩余问题:")
    top3 = after_types.most_common(3)
    for etype, count in top3:
        print(f"    - {etype}: {count}个 ({count/after_errors*100:.1f}%)")

if __name__ == '__main__':
    main()
