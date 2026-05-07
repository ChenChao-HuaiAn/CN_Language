#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Compare GCC errors between before and after P0 fix"""
import re
import sys
from collections import Counter

def analyze_file(filename):
    with open(filename, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    errors = [l.strip() for l in lines if ': error:' in l]
    
    file_errors = Counter()
    for line in errors:
        m = re.match(r'^(.+?):(\d+):', line)
        if m:
            fname = m.group(1).split('\\')[-1]
            file_errors[fname] += 1
    
    type_errors = Counter()
    for line in errors:
        ll = line.lower()
        if 'long long' in ll and 'assign' in ll:
            type_errors['LL_ASSIGN'] += 1
        elif 'struct' in ll and 'assign' in ll:
            type_errors['STRUCT_ASSIGN'] += 1
        elif 'member' in ll and ('structure' in ll or 'union' in ll):
            type_errors['MEMBER_ACCESS'] += 1
        elif 'conflicting types' in ll:
            type_errors['FUNC_CONFLICT'] += 1
        elif 'argument' in ll and ('incompatible' in ll or 'passing' in ll):
            type_errors['FUNC_ARG'] += 1
        elif 'too few arguments' in ll:
            type_errors['FEW_ARGS'] += 1
        elif 'void expression' in ll:
            type_errors['VOID_EXPR'] += 1
        elif 'dereferencing' in ll or 'unary' in ll:
            type_errors['DEREF'] += 1
        elif 'char' in ll and 'assign' in ll:
            type_errors['CHAR_ASSIGN'] += 1
        elif 'returning' in ll and 'incompatible' in ll:
            type_errors['RETURN_MISMATCH'] += 1
        elif 'undeclared' in ll:
            type_errors['UNDECLARED'] += 1
        else:
            type_errors['OTHER'] += 1
    
    return len(errors), file_errors, type_errors

new_total, new_files, new_types = analyze_file('tools/gcc_errors_p0_fix.txt')
old_total, old_files, old_types = analyze_file('tools/gcc_errors_after_fix.txt')

# Write results to file to avoid encoding issues with console
with open('tools/compare_results.txt', 'w', encoding='utf-8') as out:
    out.write("=== P0 Fix Comparison Report ===\n\n")
    out.write(f"Baseline(204) -> Before P0({old_total}) -> After P0({new_total})\n\n")
    
    out.write("=== File Distribution Comparison ===\n")
    all_files = sorted(set(list(new_files.keys()) + list(old_files.keys())))
    for f in all_files:
        old_c = old_files.get(f, 0)
        new_c = new_files.get(f, 0)
        diff = new_c - old_c
        sign = '+' if diff > 0 else ''
        out.write(f"  {f}: {old_c} -> {new_c} ({sign}{diff})\n")
    
    out.write("\n=== Error Type Comparison ===\n")
    all_types = sorted(set(list(new_types.keys()) + list(old_types.keys())))
    for t in all_types:
        old_c = old_types.get(t, 0)
        new_c = new_types.get(t, 0)
        diff = new_c - old_c
        sign = '+' if diff > 0 else ''
        out.write(f"  {t}: {old_c} -> {new_c} ({sign}{diff})\n")
    
    out.write("\n=== Detailed New Error Types ===\n")
    # Re-analyze with more detail
    with open('tools/gcc_errors_p0_fix.txt', 'r', encoding='utf-8', errors='replace') as f:
        new_lines = [l.strip() for l in f if ': error:' in l]
    
    # Count specific patterns
    ll_from_struct = sum(1 for l in new_lines if 'long long' in l and 'struct' in l and 'assign' in l.lower())
    ll_from_charptr = sum(1 for l in new_lines if 'long long' in l and 'char *' in l and 'assign' in l.lower())
    ll_from_voidptr = sum(1 for l in new_lines if 'long long' in l and 'void *' in l and 'assign' in l.lower())
    ll_from_charpp = sum(1 for l in new_lines if 'long long' in l and 'char **' in l and 'assign' in l.lower())
    ll_from_intptr = sum(1 for l in new_lines if 'long long' in l and 'int *' in l and 'assign' in l.lower())
    
    out.write(f"  long long <- struct: {ll_from_struct}\n")
    out.write(f"  long long <- char*: {ll_from_charptr}\n")
    out.write(f"  long long <- void*: {ll_from_voidptr}\n")
    out.write(f"  long long <- char**: {ll_from_charpp}\n")
    out.write(f"  long long <- int*: {ll_from_intptr}\n")

print("Done. Results written to tools/compare_results.txt")
