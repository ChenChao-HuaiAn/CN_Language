# -*- coding: utf-8 -*-
import re
src = open('src/cn_compiler/ir/ir.cpp', encoding='utf-8').read()
pat = re.compile(r'IRGenerator::(\w+)\(', re.M)
seen = []
for m in pat.finditer(src):
    if m.group(1) not in seen:
        seen.append(m.group(1))
for s in seen:
    print(s)
