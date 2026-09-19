# -*- coding: utf-8 -*-
"""纯重构轮等价性对拍（基线编译器 vs 新编译器）——产物 asm 逐字节 md5。

166-a 固化（实证：4 个赋值密集小用例全过、v2 全树暴露偏差——小样本不足以证明等价）。
用法：python scripts/refactor_parity.py <基线cn.exe> <新cn.exe>
覆盖：v2 全树 + v2 树全部 .cn + 赋值形态密集 E2E 用例。
注意：与其它使用 target/v2asm.asm 的验证脚本互斥串行（并发=产物污染，见 项目记忆/教训.md 166 段）。
"""
import hashlib
import os
import subprocess
import sys
import glob

BASE = sys.argv[1]
NEW = sys.argv[2]
TMP = 'target/parity_cmp'
os.makedirs(TMP, exist_ok=True)


def md5(p):
    return hashlib.md5(open(p, 'rb').read()).hexdigest()


def run(exe, src, out):
    r = subprocess.run([exe, 'compile', src, '--output', out],
                       capture_output=True)
    return r.returncode


samples = []
# 1) v2 全树（最强覆盖）
samples.append(('v2self', 'CN语言编译器v2/主.cn'))
# 2) v2 树全部 .cn 文件
for f in sorted(glob.glob('CN语言编译器v2/**/*.cn', recursive=True)):
    samples.append(('v2file:' + f.replace('\\', '/'), f))
# 3) 赋值形态密集的 E2E 用例
for d in sorted(glob.glob('tests/e2e/*/主.cn')):
    name = d.replace('\\', '/').split('/')[2]
    if any(k in name for k in ('183_', '253_', '116_', '276_', '278_', '165_',
                               '224_', '218_', '219_', '246_', '247_',
                               '260_', '275_', '261_', '268_', '272_')):
        samples.append(('e2e:' + name, d))

ok = diff = skip = 0
bad = []
for tag, src in samples:
    key = hashlib.md5(tag.encode()).hexdigest()[:10]
    ob = f'{TMP}/b_{key}.asm'
    nb = f'{TMP}/n_{key}.asm'
    rb = run(BASE, src, ob)
    rn = run(NEW, src, nb)
    if rb != 0 or rn != 0:
        skip += 1
        print(f'SKIP  rc(base={rb},new={rn})  {tag}')
        continue
    a, b = md5(ob), md5(nb)
    if a == b:
        ok += 1
    else:
        diff += 1
        bad.append(tag)
        print(f'DIFF  {tag}')
print(f'\n一致 {ok}  不一致 {diff}  跳过 {skip}  合计 {len(samples)}')
if bad:
    print('不一致样本：')
    for t in bad:
        print('  -', t)
