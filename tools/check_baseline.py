#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""检查修复前的基准错误文件"""
import re

files = [
    'tools/gcc_errors_current.txt',
    'tools/gcc_errors_current_run.txt', 
    'tools/gcc_errors_after_all_p3.txt',
    'tools/gcc_errors_clean2.txt',
    'tools/gcc_errors_p1.txt',
    'tools/gcc_errors_after_p1.txt',
    'tools/gcc_errors_after_p2.txt',
    'tools/gcc_errors_after_p3_1.txt',
]
for f in files:
    try:
        with open(f, 'r', encoding='utf-8', errors='replace') as fh:
            content = fh.read()
        lines = content.split('\n')
        errors = len([l for l in lines if ': error:' in l])
        warnings = len([l for l in lines if ': warning:' in l])
        print(f'{f}: {errors} errors, {warnings} warnings')
    except Exception as e:
        print(f'{f}: ERROR - {e}')
