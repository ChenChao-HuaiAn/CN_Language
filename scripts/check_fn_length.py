# -*- coding: utf-8 -*-
"""D1 函数级残留扫描（单一职责纪律：每函数 ≤100 行）：{} 配对 ≥100 行。

166-a 固化（原 165-a 口径未留档，本轮实测基线=77 个：宿主 27 + v2 树 50）。
用法：python scripts/check_fn_length.py [根目录...]（默认 src/cn_compiler + CN语言编译器v2）
输出：超百行函数清单（按行数降序）。D1 为长期滚动任务，本脚本供轮次度量（未接入 ci.ps1：
未清零前接入即门禁红）。
"""
import re
import sys
import glob

ROOTS = sys.argv[1:] or ['src/cn_compiler', 'CN语言编译器v2']
KW = re.compile(r'\b(if|for|while|switch|else|do|catch|namespace|struct|class|enum|union|try)\b')
THRESH = 100


def texts(root):
    for pat in ('**/*.cpp', '**/*.hpp', '**/*.cc', '**/*.cn'):
        for f in glob.glob(f'{root}/{pat}', recursive=True):
            yield f


def scan(path):
    lines = open(path, encoding='utf-8', errors='replace').read().split('\n')
    # 大括号配对（忽略行内字符串/注释中的括号：项目风格下行注释不含裸括号影响可控）
    stack = []
    pairs = []
    for idx, ln in enumerate(lines):
        code = ln.split('//')[0]
        for ch in code:
            if ch == '{':
                stack.append(idx)
            elif ch == '}':
                if stack:
                    pairs.append((stack.pop(), idx))
    out = []
    for st, en in pairs:
        n = en - st + 1
        if n < THRESH:
            continue
        # 向上回溯取签名区（遇前一块结尾/空行即停，最多 15 行）
        win = []
        k = st - 1
        while k >= 0 and len(win) < 15:
            s = lines[k].strip()
            if s.endswith('}') or s.endswith('};'):
                break
            win.append(lines[k])
            k -= 1
        win.reverse()
        win.append(lines[st])
        head = ' '.join(win).strip()
        if KW.search(head):
            continue
        if path.endswith('.cn'):
            fl = [w.strip() for w in win
                  if re.match(r'^\s*(不安全\s+)?(静态\s+)?(公开\s+|私有\s+)?函数\b', w)]
            if not fl:
                continue
            name = fl[-1].split('(')[0].split()[-1]
        else:
            pre = head.split('(')[0]
            if '=' in pre or '.' in pre or '->' in pre:
                continue
            m = re.search(r'([~\w:]+)\s*$', pre)
            if m is None:
                continue
            name = m.group(1)
        out.append((n, name, st + 1, en + 1))
    return out


allf = []
for root in ROOTS:
    for f in texts(root):
        got = scan(f)
        # 去嵌套：被同文件其他候选完全包含的块丢弃
        keep = []
        for c in got:
            n, name, a, b = c
            if any((o[2] <= a and b <= o[3] and (o[2], o[3]) != (a, b)) for o in got):
                continue
            keep.append(c)
        for n, name, a, b in keep:
            allf.append((n, f.replace('\\', '/'), name, a, b))
allf.sort(key=lambda x: -x[0])
print(f'超百行函数共 {len(allf)} 个（口径：{{}} 配对 ≥{THRESH} 行）')
for n, f, name, a, b in allf[:15]:
    print(f'{n:5d}  {f}:{a}-{b}  {name}')
