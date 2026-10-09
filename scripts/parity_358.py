# -*- coding: utf-8 -*-
"""358 判定层抽取专用对拍（三后端×双编译器·asm 逐字节 md5）。

任务 358 硬验收面：判定层抽取前后，win-x64/linux-x86_64/linux-arm64 三后端
产物汇编必须逐字节一致（零分叉红线的机械化执行面）。

用法（两步）:
  改前基线:  python scripts/parity_358.py save target/parity_358/base.md5 \
                 --exe target/cn_base_358.exe
  改后对拍:  python scripts/parity_358.py cmp  target/parity_358/base.md5 \
                 --base target/cn_base_358.exe --new target/cn.exe

覆盖面: v2 全树全部 .cn（大程序最强覆盖）+ tests/e2e 全部 主.cn。
注意: 产物只落 target/parity_358/（gitignore·不入库）; 与其它使用
target/v2asm.asm 的脚本互斥串行（教训 166 段·本脚本用独立目录无冲突，
但仍勿并发双实例）。
"""
import hashlib
import os
import subprocess
import sys
import glob

TARGETS = ['win-x64', 'linux-x86_64', 'linux-arm64']
TMP = 'target/parity_358'


def md5(p):
    return hashlib.md5(open(p, 'rb').read()).hexdigest()


def compile_one(exe, src, target, out):
    r = subprocess.run([exe, 'compile', src, '--target', target,
                        '--output', out], capture_output=True)
    return r.returncode


def samples():
    out = []
    out.append(('v2self', 'CN语言编译器v2/主.cn'))
    for f in sorted(glob.glob('CN语言编译器v2/**/*.cn', recursive=True)):
        out.append(('v2file:' + f.replace('\\', '/'), f))
    for d in sorted(glob.glob('tests/e2e/*/主.cn')):
        name = d.replace('\\', '/').split('/')[2]
        out.append(('e2e:' + name, d))
    return out


def key_of(tag, target):
    k = hashlib.md5((tag + '|' + target).encode('utf-8')).hexdigest()[:12]
    return k


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    os.makedirs(TMP, exist_ok=True)
    if mode == 'save':
        list_path = sys.argv[2]
        exe = sys.argv[sys.argv.index('--exe') + 1]
        rows = []
        for tag, src in samples():
            for t in TARGETS:
                out = f'{TMP}/s_{key_of(tag, t)}.asm'
                rc = compile_one(exe, src, t, out)
                if rc != 0 or not os.path.exists(out):
                    rows.append(f'{tag}|{t}|SKIP|rc{rc}')
                else:
                    rows.append(f'{tag}|{t}|{md5(out)}')
        open(list_path, 'w', encoding='utf-8').write('\n'.join(rows) + '\n')
        n_ok = sum(1 for r in rows if '|SKIP|' not in r)
        print(f'[基线已存] {list_path}  样本={len(rows)}  可比对={n_ok}  跳过={len(rows)-n_ok}')
        return
    if mode == 'cmp':
        list_path = sys.argv[2]
        base = sys.argv[sys.argv.index('--base') + 1]
        new = sys.argv[sys.argv.index('--new') + 1]
        base_rows = {}
        for line in open(list_path, encoding='utf-8'):
            line = line.strip()
            if not line:
                continue
            tag, t, h = line.split('|', 2)
            base_rows[(tag, t)] = h
        ok = diff = skip = 0
        diffs = []
        for tag, src in samples():
            for t in TARGETS:
                if (tag, t) not in base_rows:
                    continue
                bh = base_rows[(tag, t)]
                if bh.startswith('SKIP'):
                    skip += 1
                    continue
                out = f'{TMP}/n_{key_of(tag, t)}.asm'
                rc = compile_one(new, src, t, out)
                if rc != 0 or not os.path.exists(out):
                    # 基线可编而新版编不出=行为变化（红）
                    diff += 1
                    diffs.append(f'{tag}|{t}|基线{bh[:8]} 新版rc{rc}(编译失败)')
                    continue
                nh = md5(out)
                if nh == bh:
                    ok += 1
                else:
                    diff += 1
                    diffs.append(f'{tag}|{t}|基线{bh[:8]} 新版{nh[:8]}')
        print(f'[对拍结果] 一致={ok}  差异={diff}  跳过(双侧同skip)={skip}')
        for d in diffs[:40]:
            print('  DIFF', d)
        if len(diffs) > 40:
            print(f'  ...其余 {len(diffs)-40} 条省略')
        sys.exit(1 if diff else 0)
    print(__doc__)
    sys.exit(2)


if __name__ == '__main__':
    main()
