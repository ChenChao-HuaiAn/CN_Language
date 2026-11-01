#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""doc_coverage.json 结构级三路并集 merge driver（.gitattributes: merge=doc-coverage）。

并集内核 = scripts/merge_struct_union.py（结构级并集·丢行禁令·fail-safe 口径见该文件
头部注释）。本文件只定本台账的文件面参数：
  顶层必含「特性清单」数组；输出缩进 2。
  元素身份：特性清单行=特性名，归类兜底桶=名称，归类豁免条=用例；
            e2e/模式 等数组元素为标量，身份=其值。
历史教训（#1942）：旧实现只并集「特性清单」，其余顶层键一律取 ours——rebase 时
  ours=上游 develop，任务分支在归类兜底桶追加的用例整段消失且退出码 0，
  L1 check_doc_coverage 报未归类 → integrate 两轮红。现全部顶层键走结构级并集。

每机一次性安装：bash scripts/setup_merge_drivers.sh
（等价：git config merge.doc-coverage.driver "python scripts/merge_doc_coverage.py %O %A %B"）
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import merge_struct_union as 内核


def 主() -> int:
    return 内核.运行驱动(必含键="特性清单", 计数名="特性行", 缩进=2,
                         日志前缀="[doc-coverage-merge]")


if __name__ == "__main__":
    sys.exit(主())
