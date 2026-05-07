#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""详细分析修复后的GCC错误"""
import re

with open('tools/gcc_errors_after_fix.txt', 'r', encoding='utf-8', errors='replace') as f:
    lines = f.readlines()

# 找出 '赋值: long long int <- 其他指针/类型' 的具体错误
print('=== 赋值: long long int <- 其他指针/类型 的具体错误(前15条) ===')
count = 0
for line in lines:
    if ': error:' in line and 'assignment to' in line and 'long long int' in line:
        m = re.search(r': error: (.+)', line)
        if m and 'char *' not in m.group(1) and 'struct' not in m.group(1):
            fm = re.match(r'(.+?):(\d+):\d+: error:', line)
            if fm:
                fname = fm.group(1).replace('\\', '/').split('/')
                short = '/'.join(fname[-2:]) if len(fname) >= 2 else fm.group(1)
                print(f'  {short}:{fm.group(2)}: {m.group(1).strip()[:80]}')
            count += 1
            if count >= 15:
                break

print()
print('=== 赋值: struct <- 不兼容类型 的具体错误(前15条) ===')
count = 0
for line in lines:
    if ': error:' in line and 'assignment to' in line and 'struct' in line:
        m = re.search(r': error: (.+)', line)
        if m and 'long long' not in m.group(1):
            fm = re.match(r'(.+?):(\d+):\d+: error:', line)
            if fm:
                fname = fm.group(1).replace('\\', '/').split('/')
                short = '/'.join(fname[-2:]) if len(fname) >= 2 else fm.group(1)
                print(f'  {short}:{fm.group(2)}: {m.group(1).strip()[:80]}')
            count += 1
            if count >= 15:
                break

print()
print('=== 成员访问错误的具体错误(前12条) ===')
count = 0
for line in lines:
    if ': error:' in line and 'request for member' in line:
        m = re.search(r': error: (.+)', line)
        if m:
            fm = re.match(r'(.+?):(\d+):\d+: error:', line)
            if fm:
                fname = fm.group(1).replace('\\', '/').split('/')
                short = '/'.join(fname[-2:]) if len(fname) >= 2 else fm.group(1)
                print(f'  {short}:{fm.group(2)}: {m.group(1).strip()[:80]}')
            count += 1
            if count >= 12:
                break

print()
print('=== 修复前: long long int <- struct指针 的具体错误(前5条) ===')
with open('tools/gcc_errors_current_run.txt', 'r', encoding='utf-8', errors='replace') as f:
    before_lines = f.readlines()
count = 0
for line in before_lines:
    if ': error:' in line and 'assignment to' in line and 'long long int' in line and 'struct' in line:
        m = re.search(r': error: (.+)', line)
        if m:
            fm = re.match(r'(.+?):(\d+):\d+: error:', line)
            if fm:
                fname = fm.group(1).replace('\\', '/').split('/')
                short = '/'.join(fname[-2:]) if len(fname) >= 2 else fm.group(1)
                print(f'  {short}:{fm.group(2)}: {m.group(1).strip()[:80]}')
            count += 1
            if count >= 5:
                break

print()
print('=== 修复前: long long int <- char* 的具体错误(前5条) ===')
count = 0
for line in before_lines:
    if ': error:' in line and 'assignment to' in line and 'long long int' in line and 'char *' in line:
        m = re.search(r': error: (.+)', line)
        if m:
            fm = re.match(r'(.+?):(\d+):\d+: error:', line)
            if fm:
                fname = fm.group(1).replace('\\', '/').split('/')
                short = '/'.join(fname[-2:]) if len(fname) >= 2 else fm.group(1)
                print(f'  {short}:{fm.group(2)}: {m.group(1).strip()[:80]}')
            count += 1
            if count >= 5:
                break
