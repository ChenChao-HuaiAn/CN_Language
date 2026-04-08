#!/usr/bin/env python3
"""移除 C 文件中的 [DEBUG] 调试输出"""

import re
import sys

def remove_debug_output(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_len = len(content)
    
    # 匹配多行的 fprintf(stderr, "[DEBUG]... 语句
    # 使用非贪婪匹配，从 fprintf 开始到 ); 结束
    pattern = r'\s*fprintf\s*\(\s*stderr\s*,\s*"\[DEBUG\].*?\);'
    content = re.sub(pattern, '', content, flags=re.DOTALL)
    
    # 清理多余的空行
    content = re.sub(r'\n\s*\n\s*\n', '\n\n', content)
    
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    
    # 统计剩余
    remaining = len(re.findall(r'\[DEBUG\]', content))
    removed = original_len - len(content)
    
    return remaining, removed

if __name__ == '__main__':
    files = [
        'src/semantics/resolution/scope_builder.c',
        'src/semantics/checker/semantic_passes.c',
        'src/semantics/symbols/type_system.c'
    ]
    
    for filepath in files:
        try:
            remaining, removed = remove_debug_output(filepath)
            print(f'{filepath}: {remaining} DEBUG remaining, {removed} chars removed')
        except Exception as e:
            print(f'{filepath}: ERROR - {e}')
