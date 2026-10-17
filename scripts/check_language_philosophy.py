#!/usr/bin/env python3
# CN 语言特性基准门禁（647-a 立法·用户令 2026-09-23·AGENTS §3.1）
# 校验（909 重定向后的 v5 权威链）：
#   [1] AGENTS.md 哲学权威指引句在位（§1「分层安全四条与特性哲学四句话=plans/001 §1.1a+plans/019」）
#   [2] plans/001 §1.1a 分层安全总则标题+①锚句在位
#   [3] plans/019 设计哲学三句话在位
# 909 处置（2026-10-17·P3 自裁）：
#   ①原 [2] 段「plans/021 §三 呈报行含〔基准=019〕标注」删除——021 文档 382 退位/384 删除后
#     呈报流转与台账权威全在服务端（看板 API），无文件可查，重定向无语义等价面；
#   ②原 [1] 段三句话字串经实证随 1016 v5 减薄重写已不在 AGENTS.md（v5 条款权威链改为
#     001 §1.1a+019）——重定向到现行链路，检查强度不降。
# 退出码：0=全过；1=存在违例（逐条打印）
import io
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
fails = []


def 核验(相对: str, 标签: str, 句集: tuple) -> None:
    """文件在位+句集逐句在位核验（909 修复根因：读已删文件即崩=miss 未显形——
    现为缺失/缺句=显式违例，不崩不静默）。"""
    p = os.path.join(ROOT, 相对)
    if not os.path.exists(p):
        fails.append(f"{标签} 文件缺失: {相对}")
        return
    文本 = io.open(p, encoding="utf-8").read()
    for 句 in 句集:
        if 句 not in 文本:
            fails.append(f"{标签} 缺句: {句}")


# [1] AGENTS.md 哲学权威指引句（v5 §1 链路顶点）
核验("AGENTS.md", "[1] AGENTS.md", ("特性哲学四句话", "plans/001 §1.1a"))
# [2] 001 §1.1a 分层安全四条（标题+①锚句）
核验("plans/001 CN语言编译器设计规格书.md", "[2] 001 §1.1a",
     ("1.1a 分层安全总则", "编译期能证明的，必须在编译期拦"))
# [3] 019 设计哲学三句话
核验("plans/019-编译期安全分阶段实施路径.md", "[3] 019 设计哲学",
     ("默认路径零规则", "显式放弃安全=显式获得性能", "危险面收口而非全语言戒严"))

if fails:
    for f in fails:
        print(f)
    sys.exit(1)
print("CN 语言特性基准门禁 ✓（AGENTS 指引句·001 §1.1a 四条·019 三句话在位）")
