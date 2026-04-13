#!/usr/bin/env python3
"""
移除所有 [DEBUG] 调试输出语句的脚本
"""

import os
import re

# 需要处理的文件列表
files = [
    'src/backend/cgen/cgen.c',
    'src/cli/cnc/main.c',
    'src/support/diagnostics/diagnostics.c',
    'src/semantics/symbols/symbol_table.c',
    'src/semantics/symbols/type_system.c',
    'src/semantics/checker/class_analyzer.c',
    'src/semantics/resolution/module_semantics.c',
    'src/semantics/checker/semantic_passes.c',
    'src/semantics/resolution/scope_builder.c',
    'src/ir/gen/irgen.c'
]

def remove_debug_statements(file_path):
    """移除文件中的调试语句"""
    if not os.path.exists(file_path):
        return 0, f"文件不存在"
    
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    new_lines = []
    removed_count = 0
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # 检查是否是调试语句的开始
        if 'fprintf(stderr, "[DEBUG]' in line or "fprintf(stderr, '[DEBUG]" in line:
            # 检查这一行是否已经包含完整的语句（以 ); 结尾）
            if ');' in line:
                removed_count += 1
                i += 1
                continue
            else:
                # 需要跳过多行直到找到 );
                removed_count += 1
                i += 1
                while i < len(lines):
                    if ');' in lines[i]:
                        i += 1
                        break
                    i += 1
                continue
        
        new_lines.append(line)
        i += 1
    
    if removed_count > 0:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        return removed_count, f"移除了 {removed_count} 条调试语句"
    
    return 0, "无调试语句"

def main():
    total_removed = 0
    
    print("开始移除调试语句...")
    print("=" * 60)
    
    for file_path in files:
        count, message = remove_debug_statements(file_path)
        print(f"{file_path}: {message}")
        total_removed += count
    
    print("=" * 60)
    print(f"总计移除: {total_removed} 条调试语句")

if __name__ == "__main__":
    main()
