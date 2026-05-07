#!/usr/bin/env python3
# S1-S4修复前后对比报告生成脚本
import re

def parse_errors(filename):
    with open(filename, 'r', encoding='utf-8-sig') as f:
        content = f.read()
    
    lines = [l.strip() for l in content.split('\n') if re.search(r'\.c:\d+:\d+: error:', l)]
    
    # 按类型分类
    ptr_to_int = sum(1 for l in lines if 'makes integer from pointer' in l)
    int_to_ptr = sum(1 for l in lines if 'makes pointer from integer' in l)
    incompat_ptr = sum(1 for l in lines if 'incompatible pointer type' in l)
    incompat_assign = sum(1 for l in lines if 'incompatible types when assigning' in l)
    incompat_arg = sum(1 for l in lines if 'incompatible type for argument' in l or ('passing argument' in l and 'incompatible' in l))
    incompat_return = sum(1 for l in lines if 'incompatible types when returning' in l)
    not_struct = sum(1 for l in lines if 'not a structure or union' in l or 'request for member' in l or 'has no member named' in l)
    undeclared = sum(1 for l in lines if 'undeclared' in l)
    deref_ptr = sum(1 for l in lines if 'dereferencing pointer' in l or 'invalid type argument' in l)
    void_expr = sum(1 for l in lines if 'invalid use of void expression' in l)
    conflicting = sum(1 for l in lines if 'conflicting types' in l)
    too_few_args = sum(1 for l in lines if 'too few arguments' in l)
    invalid_operands = sum(1 for l in lines if 'invalid operands' in l)
    undefined_type = sum(1 for l in lines if 'invalid use of undefined type' in l)
    
    classified = (ptr_to_int + int_to_ptr + incompat_ptr + incompat_assign + 
                  incompat_arg + incompat_return + not_struct + undeclared + 
                  deref_ptr + void_expr + conflicting + too_few_args + 
                  invalid_operands + undefined_type)
    other = len(lines) - classified
    
    # 按文件统计
    file_errors = {}
    for l in lines:
        m = re.search(r'([A-Za-z]:[^\s:]+\.c):\d+:\d+: error:', l)
        if m:
            short = m.group(1).split('\\')[-1]
            file_errors[short] = file_errors.get(short, 0) + 1
    
    return {
        'total': len(lines),
        'ptr_to_int': ptr_to_int,
        'int_to_ptr': int_to_ptr,
        'ptr_int_total': ptr_to_int + int_to_ptr,
        'incompat_ptr': incompat_ptr,
        'incompat_assign': incompat_assign,
        'incompat_arg': incompat_arg,
        'incompat_return': incompat_return,
        'incompat_total': incompat_ptr + incompat_assign + incompat_arg + incompat_return,
        'not_struct': not_struct,
        'undeclared': undeclared,
        'deref_ptr': deref_ptr,
        'void_expr': void_expr,
        'conflicting': conflicting,
        'too_few_args': too_few_args,
        'invalid_operands': invalid_operands,
        'undefined_type': undefined_type,
        'other': other,
        'file_errors': file_errors,
    }

def pct_change(old, new):
    if old == 0:
        return "N/A" if new == 0 else f"+{new} (新增)"
    diff = new - old
    pct = (diff / old) * 100
    sign = "+" if diff > 0 else ""
    return f"{sign}{diff} ({sign}{pct:.1f}%)"

baseline = parse_errors('tools/gcc_errors_baseline.txt')
after = parse_errors('tools/gcc_errors_after_s1_s4.txt')

print("=" * 70)
print("CN语言自举编译 S1-S4修复效果对比报告")
print("=" * 70)
print()

print("## 1. 总体对比")
print()
print("| 指标 | 修复前 | 修复后 | 变化 |")
print("|------|--------|--------|------|")
print(f"| 总错误数 | {baseline['total']} | {after['total']} | {pct_change(baseline['total'], after['total'])} |")
print(f"| 指针/整数转换 | {baseline['ptr_int_total']} | {after['ptr_int_total']} | {pct_change(baseline['ptr_int_total'], after['ptr_int_total'])} |")
print(f"| 不兼容类型 | {baseline['incompat_total']} | {after['incompat_total']} | {pct_change(baseline['incompat_total'], after['incompat_total'])} |")
print(f"| 非结构体成员访问 | {baseline['not_struct']} | {after['not_struct']} | {pct_change(baseline['not_struct'], after['not_struct'])} |")
print()

print("## 2. 错误类型详细对比")
print()
print("| 错误类型 | 修复前 | 修复后 | 变化 |")
print("|----------|--------|--------|------|")
print(f"| 指针→整数 (makes integer from pointer) | {baseline['ptr_to_int']} | {after['ptr_to_int']} | {pct_change(baseline['ptr_to_int'], after['ptr_to_int'])} |")
print(f"| 整数→指针 (makes pointer from integer) | {baseline['int_to_ptr']} | {after['int_to_ptr']} | {pct_change(baseline['int_to_ptr'], after['int_to_ptr'])} |")
print(f"| 不兼容指针类型 (incompatible pointer type) | {baseline['incompat_ptr']} | {after['incompat_ptr']} | {pct_change(baseline['incompat_ptr'], after['incompat_ptr'])} |")
print(f"| 不兼容赋值类型 (incompatible types when assigning) | {baseline['incompat_assign']} | {after['incompat_assign']} | {pct_change(baseline['incompat_assign'], after['incompat_assign'])} |")
print(f"| 不兼容参数类型 (incompatible type for argument) | {baseline['incompat_arg']} | {after['incompat_arg']} | {pct_change(baseline['incompat_arg'], after['incompat_arg'])} |")
print(f"| 不兼容返回类型 (incompatible types when returning) | {baseline['incompat_return']} | {after['incompat_return']} | {pct_change(baseline['incompat_return'], after['incompat_return'])} |")
print(f"| 非结构体成员访问 | {baseline['not_struct']} | {after['not_struct']} | {pct_change(baseline['not_struct'], after['not_struct'])} |")
print(f"| 未声明标识符 (undeclared) | {baseline['undeclared']} | {after['undeclared']} | {pct_change(baseline['undeclared'], after['undeclared'])} |")
print(f"| 解引用指针类型错误 | {baseline['deref_ptr']} | {after['deref_ptr']} | {pct_change(baseline['deref_ptr'], after['deref_ptr'])} |")
print(f"| void表达式错误 | {baseline['void_expr']} | {after['void_expr']} | {pct_change(baseline['void_expr'], after['void_expr'])} |")
print(f"| 冲突类型 (conflicting types) | {baseline['conflicting']} | {after['conflicting']} | {pct_change(baseline['conflicting'], after['conflicting'])} |")
print(f"| 参数不足 (too few arguments) | {baseline['too_few_args']} | {after['too_few_args']} | {pct_change(baseline['too_few_args'], after['too_few_args'])} |")
print(f"| 无效操作数 (invalid operands) | {baseline['invalid_operands']} | {after['invalid_operands']} | {pct_change(baseline['invalid_operands'], after['invalid_operands'])} |")
print(f"| 未定义类型 (undefined type) | {baseline['undefined_type']} | {after['undefined_type']} | {pct_change(baseline['undefined_type'], after['undefined_type'])} |")
print(f"| 其他错误 | {baseline['other']} | {after['other']} | {pct_change(baseline['other'], after['other'])} |")
print()

print("## 3. 按文件对比")
print()
all_files = sorted(set(list(baseline['file_errors'].keys()) + list(after['file_errors'].keys())))
print("| 文件 | 修复前 | 修复后 | 变化 |")
print("|------|--------|--------|------|")
for f in all_files:
    b = baseline['file_errors'].get(f, 0)
    a = after['file_errors'].get(f, 0)
    print(f"| {f} | {b} | {a} | {pct_change(b, a)} |")
print()

print("## 4. 关键发现")
print()
# 计算改善和退步
improvements = []
regressions = []
for f in all_files:
    b = baseline['file_errors'].get(f, 0)
    a = after['file_errors'].get(f, 0)
    diff = b - a
    if diff > 0:
        improvements.append((f, b, a, diff))
    elif diff < 0:
        regressions.append((f, b, a, -diff))

if improvements:
    print("### 改善的文件:")
    for f, b, a, diff in sorted(improvements, key=lambda x: -x[3]):
        print(f"  - {f}: {b} → {a} (减少 {diff})")
    print()

if regressions:
    print("### 退步的文件:")
    for f, b, a, diff in sorted(regressions, key=lambda x: -x[3]):
        print(f"  - {f}: {b} → {a} (增加 {diff})")
    print()

print("### 总结:")
total_diff = baseline['total'] - after['total']
ptr_int_diff = baseline['ptr_int_total'] - after['ptr_int_total']
print(f"  - 总错误数变化: {baseline['total']} → {after['total']} ({'减少' if total_diff > 0 else '增加'} {abs(total_diff)})")
print(f"  - 指针/整数转换错误变化: {baseline['ptr_int_total']} → {after['ptr_int_total']} ({'减少' if ptr_int_diff > 0 else '增加'} {abs(ptr_int_diff)})")
if ptr_int_diff < 0:
    print(f"  - ⚠️ 指针/整数转换错误增加了 {abs(ptr_int_diff)} 个，S1-S4修复可能引入了退步")
    print(f"  - 需要检查cgen.c中的类型传播逻辑是否正确")
