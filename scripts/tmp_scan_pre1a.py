# -*- coding: utf-8 -*-
"""PRE-1a 同款扫描（临时工具）：「如果 (X.正常)」块收口无 否则=报警形态。
配对大括号定位块收口·检查后继非空白是否 否则。输出全部命中（文件/行/缩进）。"""
import re, glob, io, sys

基 = sys.argv[1] if len(sys.argv) > 1 else 'CN语言编译器v2'
命中 = []
for f in sorted(glob.glob(基 + '/**/*.cn', recursive=True)):
    文 = io.open(f, encoding='utf-8').read()
    for m in re.finditer(r'( *)如果 \((\w+(?:\.\w+)*)\.正常\) *\{\n', 文):
        缩 = len(m.group(1))
        缩串 = ' ' * 缩
        # 从块开 { 起配对
        开 = 文.index('{', m.start())
        深 = 0
        i = 开
        while i < len(文):
            if 文[i] == '{':
                深 += 1
            elif 文[i] == '}':
                深 -= 1
                if 深 == 0:
                    break
            i += 1
        收口行尾 = 文.index('\n', i) if '\n' in 文[i:] else len(文)
        尾 = 文[i:收口行尾].strip()          # } 同行剩余
        后续 = 文[收口行尾 + 1:收口行尾 + 80]
        后续首 = 后续.strip()
        有否则 = 尾.startswith('} 否则') or 尾 == '}' and 后续首.startswith('否则') or '否则' in 尾
        if not 有否则:
            行 = 文[:m.start()].count('\n') + 1
            函数 = ''
            前文 = 文[:m.start()].split('\n')
            for k in range(len(前文) - 1, -1, -1):
                fm = re.search(r'函数 (\w+)\(', 前文[k])
                if fm:
                    函数 = fm.group(1)
                    break
            命中.append((f.replace(chr(92), '/').replace('CN语言编译器v2/', ''), 行, m.group(2), 函数))

print('肯定如果缺否则', len(命中), '处:')
for f, l, x, fn in 命中:
    print(f'  {f}:{l}  .{x}  in {fn}')
