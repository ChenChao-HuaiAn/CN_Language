#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C++ 源码 ASCII 标识符门禁（Rust `deny(non_ascii_idents)` 同构）。

背景（lessons「跨机-a」，2026-09-14 用户裁决方案A）：GCC 10 才支持 UTF-8 标识符
（UAX #31），MSVC 与 GCC≥10 接受，单位机麒麟 GCC 9.3 直接构建瘫痪
（`error: stray '\\346'`）。本脚本扫描全部 C++ 源码的**代码区**（剥离注释、
字符串、字符字面量、原始字符串后），任何非 ASCII 字符均判违例——代码区非 ASCII
只可能出现在标识符中，故等价于 AGENTS.md 纪律「C++ 标识符一律 ASCII」。

用法：python3 scripts/check_ascii_idents.py [根目录]   # 默认仓库根
退出码：0=全部合规；1=存在违例（清单见 stdout）
"""

import re
import sys
from pathlib import Path

# 扫描的 C++ 源文件扩展名
CPP_EXTS = {'.cpp', '.hpp', '.h', '.hh', '.cc', '.cxx', '.ipp', '.inl'}
# 排除目录（生成产物 / 版本库内部 / 第三方缓存）
EXCLUDE_DIRS = {'target', '.git', 'build', '__pycache__'}

# 代码区 token：标识符形态（ASCII 词字符与任意非 ASCII 连写为一个 token）
TOKEN_RE = re.compile(r'[0-9A-Za-z_\u0080-\U0010ffff]+')
# 原始字符串前缀：R / u8R / uR / UR / LR（前缀必须整体紧邻引号）
RAW_PREFIX_RE = re.compile(r'(?:u8|[uUL])?R$')


def strip_noncode(text: str) -> str:
    """剥离注释 / 字符串 / 字符字面量 / 原始字符串，保留行结构（换行原位保留）。

    返回文本中仅剩代码区字符——后续对其按行做非 ASCII 检测即可。
    """
    out = []
    i, n = 0, len(text)
    LINE, BLOCK, STR, CHR = range(4)
    state = None
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ''
        if state is None:
            if c == '/' and nxt == '/':
                state = LINE
                i += 2
                continue
            if c == '/' and nxt == '*':
                state = BLOCK
                i += 2
                continue
            if c == '"':
                # 原始字符串探测：紧邻前缀 R / u8R / uR / UR / LR
                if RAW_PREFIX_RE.search(''.join(out[-6:])):
                    paren = text.find('(', i + 1)
                    head = text[i + 1:paren] if paren != -1 else ''
                    if paren != -1 and '\n' not in head and not any(ch.isspace() for ch in head):
                        raw_end = ')' + head + '"'
                        close = text.find(raw_end, paren + 1)
                        seg = text[paren + 1:] if close == -1 else text[paren + 1:close]
                        i = n if close == -1 else close + len(raw_end)
                        # 行号保真：剥离内容但原位保留其内部换行
                        out.append('""' + '\n' * seg.count('\n'))
                        continue
                state = STR
                i += 1
                continue
            if c == "'":
                state = CHR
                i += 1
                continue
            out.append(c)
            i += 1
        elif state == LINE:
            if c == '\n':
                state = None
                out.append(c)
            i += 1
        elif state == BLOCK:
            if c == '*' and nxt == '/':
                state = None
                i += 2
            else:
                if c == '\n':
                    out.append(c)
                i += 1
        elif state == STR:
            if c == '\\':
                i += 2
                continue
            if c == '"':
                state = None
            elif c == '\n':
                out.append(c)  # 未闭合容忍：保留行结构
            i += 1
        else:  # CHR
            if c == '\\':
                i += 2
                continue
            if c == "'":
                state = None
            i += 1
    return ''.join(out)


def main() -> int:
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except AttributeError:
        pass
    root = (Path(sys.argv[1]).resolve() if len(sys.argv) > 1
            else Path(__file__).resolve().parent.parent)
    files = sorted(p for p in root.rglob('*')
                   if p.suffix.lower() in CPP_EXTS
                   and not (set(p.parts) & EXCLUDE_DIRS))
    violations = []  # (路径, 行号, token, 源行)
    for path in files:
        text = path.read_text(encoding='utf-8', errors='replace')
        if text.startswith('\ufeff'):  # UTF-8 BOM 是编码标记，非代码字符
            text = text[1:]
        code = strip_noncode(text)
        if code.isascii():
            continue
        src_lines = text.splitlines()
        code_lines = code.splitlines()
        for lineno, line in enumerate(code_lines, 1):
            if line.isascii():
                continue
            for m in TOKEN_RE.finditer(line):
                tok = m.group(0)
                if not tok.isascii():
                    src = src_lines[lineno - 1].strip() if lineno - 1 < len(src_lines) else ''
                    violations.append((path, lineno, tok, src))
    if violations:
        print(f'[ASCII门禁] 违例 {len(violations)} 处（{len({v[0] for v in violations})} 文件）：')
        for path, lineno, tok, src in violations:
            rel = path.relative_to(root)
            print(f'  {rel}:{lineno}: 非ASCII标识符 \'{tok}\'')
            print(f'      ← {src}')
        print('[ASCII门禁] 修复：标识符改为 ASCII（语义对应英文）；注释/字符串内的中文不受限。')
        return 1
    print(f'[ASCII门禁] 合规：{len(files)} 个 C++ 文件代码区全 ASCII ✓')
    return 0


if __name__ == '__main__':
    sys.exit(main())
