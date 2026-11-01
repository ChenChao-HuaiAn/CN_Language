#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""miss_registry.json 结构级三路并集 merge driver（.gitattributes: merge=miss-registry）。

背景（#1259 立法）：registry 为全任务共写单点 JSON，每任务补条即撞同一文件
（1144/1142/1157/1156 连环实录；10-27 深度机 1187 轮五度冲突实证）——普通
git 合并对 JSON 数组整段冲突，人工合并疲劳且曾产出损坏 JSON 进 develop。

协议：git merge driver，调用形如 `python scripts/merge_miss_registry.py %O %A %B`：
  %O=共同基（base） %A=当前侧（ours·就地改写为合并结果） %B=对方侧（theirs）。
并集内核 = scripts/merge_struct_union.py（结构级并集·丢行禁令·fail-safe 口径见该
文件头部注释）。本文件只定本登记表的文件面参数：
  顶层必含「条目」数组；输出缩进 1。
  元素身份=(文件, 函数, 锚文本)——实测 develop 现件以「文件+锚文本」为身份有 77 组
  重号（同一行文本出现在不同函数），三元组仅 11 组且 10 组内容全同，余 1 组按出现序
  配对保两侧不丢；缺这两个字段的条目退化为完整序列化文本身份。
同键字段分歧取 ours（本侧已甄别的登记不丢）并逐条告警；顶层 版本/说明 同规则。

每机一次性安装：bash scripts/setup_merge_drivers.sh
（等价：git config merge.miss-registry.driver "python scripts/merge_miss_registry.py %O %A %B"）
未安装时 git 回退普通冲突（无害·本脚本不被调用）。
"""
from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import merge_struct_union as 内核


def 主() -> int:
    return 内核.运行驱动(必含键="条目", 计数名="条", 缩进=1,
                         日志前缀="[miss-registry-merge]")


if __name__ == "__main__":
    sys.exit(主())
