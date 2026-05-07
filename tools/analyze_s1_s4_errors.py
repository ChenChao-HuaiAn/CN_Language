#!/usr/bin/env python3
# S1-S4修复后GCC错误精确分析脚本
import re
import sys

def analyze_errors(filename):
    with open(filename, 'r', encoding='utf-8-sig') as f:
        content = f.read()
    
    # 提取所有包含 error: 的行（排除PowerShell包装行）
    all_lines = content.split('\n')
    error_lines = []
    for line in all_lines:
        # 只匹配GCC错误行（包含文件路径和行号）
        if re.search(r'\.c:\d+:\d+: error:', line):
            error_lines.append(line.strip())
    
    # 分类
    ptr_to_int = 0  # 指针→整数转换 (makes integer from pointer)
    int_to_ptr = 0  # 整数→指针转换 (makes pointer from integer)
    incompat_ptr = 0  # 不兼容指针类型 (incompatible pointer type / assignment to 'X *' from incompatible pointer type)
    incompat_assign = 0  # 不兼容赋值类型 (incompatible types when assigning)
    incompat_arg = 0  # 不兼容参数类型 (incompatible type for argument / passing argument)
    incompat_return = 0  # 不兼容返回类型 (incompatible types when returning)
    not_struct = 0  # 非结构体成员访问
    undeclared = 0  # 未声明标识符
    deref_ptr = 0  # 解引用指针类型错误
    void_expr = 0  # void表达式错误
    conflicting_types = 0  # 冲突类型
    other = 0
    
    other_examples = []
    
    for line in error_lines:
        classified = True
        if 'makes integer from pointer without a cast' in line:
            ptr_to_int += 1
        elif 'makes pointer from integer without a cast' in line:
            int_to_ptr += 1
        elif 'incompatible pointer type' in line:
            incompat_ptr += 1
        elif 'incompatible types when assigning' in line:
            incompat_assign += 1
        elif 'incompatible type for argument' in line or 'passing argument' in line and 'incompatible' in line:
            incompat_arg += 1
        elif 'incompatible types when returning' in line:
            incompat_return += 1
        elif 'not a structure or union' in line or 'request for member' in line:
            not_struct += 1
        elif 'undeclared' in line:
            undeclared += 1
        elif 'dereferencing pointer' in line or 'invalid type argument' in line:
            deref_ptr += 1
        elif 'invalid use of void expression' in line:
            void_expr += 1
        elif 'conflicting types' in line:
            conflicting_types += 1
        else:
            classified = False
            other += 1
            if len(other_examples) < 30:
                other_examples.append(line[:200])
    
    total = len(error_lines)
    
    print("=== S1-S4修复后GCC错误精确分类 ===")
    print(f"总错误数: {total}")
    print(f"")
    print(f"--- 指针/整数转换 ---")
    print(f"  指针→整数 (makes integer from pointer): {ptr_to_int}")
    print(f"  整数→指针 (makes pointer from integer): {int_to_ptr}")
    print(f"  指针/整数转换小计: {ptr_to_int + int_to_ptr}")
    print(f"")
    print(f"--- 不兼容类型 ---")
    print(f"  不兼容指针类型 (incompatible pointer type): {incompat_ptr}")
    print(f"  不兼容赋值类型 (incompatible types when assigning): {incompat_assign}")
    print(f"  不兼容参数类型 (incompatible type for argument): {incompat_arg}")
    print(f"  不兼容返回类型 (incompatible types when returning): {incompat_return}")
    print(f"  不兼容类型小计: {incompat_ptr + incompat_assign + incompat_arg + incompat_return}")
    print(f"")
    print(f"--- 其他类型 ---")
    print(f"  非结构体成员访问 (not a structure or union): {not_struct}")
    print(f"  未声明标识符 (undeclared): {undeclared}")
    print(f"  解引用指针类型错误 (dereferencing): {deref_ptr}")
    print(f"  void表达式错误 (invalid use of void): {void_expr}")
    print(f"  冲突类型 (conflicting types): {conflicting_types}")
    print(f"  其他错误: {other}")
    
    # 按文件统计
    print(f"\n=== 按文件统计 ===")
    file_errors = {}
    for line in error_lines:
        m = re.match(r'.?([A-Za-z]:\\[^\s:]+\.c):(\d+):\d+: error:', line)
        if m:
            filepath = m.group(1)
            # 提取相对路径
            rel = filepath.replace('C:\\Users\\ChenChao\\Documents\\gitcode\\CN_Language\\', '')
            file_errors[rel] = file_errors.get(rel, 0) + 1
    
    for filepath, count in sorted(file_errors.items(), key=lambda x: -x[1]):
        short = filepath.replace('src_cn\\', '')
        print(f"  {short}: {count}")
    
    # 输出其他错误示例
    if other_examples:
        print(f"\n=== 其他错误示例（前30个）===")
        for ex in other_examples:
            print(f"  {ex}")

if __name__ == '__main__':
    analyze_errors('tools/gcc_errors_after_s1_s4.txt')
