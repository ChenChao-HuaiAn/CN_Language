#!/usr/bin/env python3
# CN 语言特性基准门禁（647-a 立法·用户令 2026-09-23·AGENTS §3.1）
# 校验：
#   [1] AGENTS.md 存在 §3.1 CN 语言特性基准条款（三句话权威基准在位）
#   [2] plans/021 §三 呈报态（📣）行含「〔基准=019〕」标注（无标注=无效呈报）
# 退出码：0=全过；1=存在违例（逐条打印）
import io
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
fails = []

# [1] AGENTS 条款在位
ag = io.open(os.path.join(ROOT, "AGENTS.md"), encoding="utf-8").read()
if "CN 语言特性基准" not in ag:
    fails.append("[1] AGENTS.md 缺少 §3.1 CN 语言特性基准条款")
for phrase in ("默认路径零规则", "显式放弃安全=显式获得性能", "危险面收口而非全语言戒严"):
    if phrase not in ag:
        fails.append("[1] AGENTS.md 哲学条文缺失句: " + phrase)

# [2] 021 📣 行须含基准标注
p21 = os.path.join(ROOT, "plans", "021-任务进度观察表.md")
t = io.open(p21, encoding="utf-8").read()
for line in t.split("\n"):
    if line.startswith(">"):
        continue
    if "📣" in line and "〔基准=019〕" not in line and "〔基准=三句话〕" not in line and "基准=019" not in line:
        fails.append("[2] 021 呈报行缺基准标注: " + line.strip()[:120])

if fails:
    for f in fails:
        print(f)
    sys.exit(1)
print("CN 语言特性基准门禁 ✓（AGENTS 条款在位·021 📣 行基准标注齐全）")
