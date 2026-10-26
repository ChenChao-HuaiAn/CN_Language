#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""doc_coverage.json 三路并集合并驱动（#1259 高冲突面治理·同 miss-registry 实录：
每任务挂用例登记即撞同一文件·10-27 深度机 1187 轮五度冲突实证）。

协议：git merge driver（.gitattributes: website/tools/doc_coverage.json merge=doc-coverage）。
语义：
  特性清单 按「特性名」键三路合并——同特性：e2e 数组并集（ours 序为骨架·
  theirs 新增追加在后）；非清单顶层键（元数据/归类兜底/归类豁免）=ours 为准。
fail-safe：任一侧 JSON 不可解析/结构不符 → 退出非零回退普通冲突。

每机一次性安装：
  git config merge.doc-coverage.name  "doc_coverage 三路并集合并"
  git config merge.doc-coverage.driver "python3 scripts/merge_doc_coverage.py %O %A %B"
"""
from __future__ import annotations

import json
import sys
from pathlib import Path


def 读侧(路径: str) -> dict:
    文本 = Path(路径).read_text(encoding="utf-8")
    for 标记 in ("<<<<<<<", "=======", ">>>>>>>"):
        if 标记 in 文本:
            raise ValueError(f"{路径} 含冲突标记 {标记}")
    d = json.loads(文本)
    if not isinstance(d, dict) or not isinstance(d.get("特性清单"), list):
        raise ValueError(f"{路径} 结构不符（顶层须含 特性清单 数组）")
    return d


def 数组并集(ours表: list, theirs表: list) -> list:
    出 = list(ours表)
    有 = {json.dumps(x, ensure_ascii=False, sort_keys=True) for x in 出}
    for x in theirs表:
        k = json.dumps(x, ensure_ascii=False, sort_keys=True)
        if k not in 有:
            出.append(x)
            有.add(k)
    return 出


def 三路并集(base: dict, ours: dict, theirs: dict) -> dict:
    出 = dict(ours)
    base特 = {f.get("特性名"): f for f in base.get("特性清单", []) if f.get("特性名")}
    ours特 = {f.get("特性名"): f for f in ours.get("特性清单", []) if f.get("特性名")}
    theirs特 = {f.get("特性名"): f for f in theirs.get("特性清单", []) if f.get("特性名")}
    清单 = []
    for f in ours.get("特性清单", []):
        名 = f.get("特性名")
        t = theirs特.get(名)
        if t is None:
            清单.append(f)
            continue
        g = dict(f)
        g["e2e"] = 数组并集(f.get("e2e", []), t.get("e2e", []))
        # 双侧各自追加的其它列表字段（如 官网覆盖）同法并集
        for k, v in f.items():
            if k != "e2e" and isinstance(v, list) and isinstance(t.get(k), list):
                g[k] = 数组并集(v, t[k])
        清单.append(g)
    for 名, t in theirs特.items():
        if 名 not in ours特 and 名 not in base特:
            清单.append(t)                   # theirs 新增特性行
    出["特性清单"] = 清单
    return 出


def 主() -> int:
    if len(sys.argv) != 4:
        print("用法: merge_doc_coverage.py <base> <ours> <theirs>", file=sys.stderr)
        return 1
    try:
        base = 读侧(sys.argv[1])
        ours = 读侧(sys.argv[2])
        theirs = 读侧(sys.argv[3])
        出 = 三路并集(base, ours, theirs)
    except (ValueError, json.JSONDecodeError, OSError) as e:
        print(f"[doc-coverage-merge] fail-safe 回退普通冲突: {e}", file=sys.stderr)
        return 1
    Path(sys.argv[2]).write_text(
        json.dumps(出, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"[doc-coverage-merge] 三路并集完成：{len(出['特性清单'])} 特性行")
    return 0


if __name__ == "__main__":
    sys.exit(主())
