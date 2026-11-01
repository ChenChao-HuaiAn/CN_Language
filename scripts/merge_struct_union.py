#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""高冲突单点 JSON 结构级三路并集内核（merge driver 共用·丢行禁令·fail-safe 口径）。

现象（#1942 实证）：doc_coverage.json 旧并集实现在 rebase 方向静默吞行——
  任务分支在「归类兜底」模式数组尾行 1564 之后追加 1685/1686，develop 侧同桶尾行
  也是 1564；旧实现只对「特性清单」做元素级并集、其余顶层键一律取 ours（rebase 时
  ours=上游 develop），于是任务分支的追加整段消失**且退出码 0**，L1
  check_doc_coverage --strict 报「未归类用例」→ integrate 两轮红。只要按文本行/
  按位置求并集，「两侧同一位置各自追加」就必然有二义性、任何启发式都会丢一侧；
  故改为按 JSON 结构求并集——结构里没有「位置」概念。

并集语义（结构级）：
  对象 = 键并集（base 序 + ours 新增键 + theirs 新增键），同键递归三路并集。
  数组 = 元素并集；顺序 = base 序 + ours 新增 + theirs 新增；两侧新增全部保留、
         同身份元素不重复。
  标量 = 三路取一：仅一侧改则取改后值；两侧同改取 ours（登记制既有口径）并告警。
  删除 = 一侧删除而另一侧未改 → 删除生效（清理可传播）；一侧删除而另一侧已改 →
         删改冲突（无法安全裁决 → fail-safe）。
  元素身份 = 对象取身份字段组首个全命中（特性名 / 名称 / 用例 / 文件+函数+锚文本）；
         无身份字段的对象与标量取完整规范文本；同身份多元素按出现序配对，
         绝不把两条不同内容的条目并成一条。
         注：登记表条目身份取 (文件,函数,锚文本) 三元组——实测 develop 现件
         以「文件+锚文本」为身份有 77 组重号（同一行文本出现在不同函数），
         三元组仅 11 组且 10 组内容全同，余 1 组由出现序配对保两侧不丢。

fail-safe 口径（#1238/#1360 禁静默）：任一侧解析失败/含冲突标记/顶层结构不符/
  结构类型冲突/删改冲突 → 抛 并集冲突，驱动以非零码退出且**不改写 %A**，由 git
  落回普通文本冲突交人裁决；禁止产出一个可能丢数据的合并结果。

调用方：merge_doc_coverage.py / merge_miss_registry.py（各自只定本文件面的
必含顶层键、输出缩进与日志前缀）。
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path
from typing import Any

# 缺失哨兵：区分「键/元素不存在」与「值为 null」——并集语义必须知道谁删了谁
缺失 = object()

# 身份字段组优先级：特性行 → 兜底桶 → 豁免条 → 登记表条目（三元组 → 二元组）
身份字段序: tuple[tuple[str, ...], ...] = (
    ("特性名",),
    ("名称",),
    ("用例",),
    ("文件", "函数", "锚文本"),
    ("文件", "锚文本"),
)

# 冲突标记（与 check_registry_sanity.py 同口径）：半成品禁参与并集
冲突标记 = ("<<<<<<<", ">>>>>>>")


class 并集冲突(Exception):
    """结构级并集无法安全裁决（fail-safe 通道·调用方须退出非零）。"""


class 并集统计:
    """并集过程的显形记录（分歧/删除/并列身份）——驱动侧逐条打印，禁静默。"""

    def __init__(self) -> None:
        self.标量分歧: list[str] = []
        self.删除生效 = 0
        self.并列身份组 = 0


def 规范文本(值: Any) -> str:
    """类型严格的内容指纹（1 与 True、1 与 1.0 不互等）。"""
    return json.dumps(值, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def 是标量(值: Any) -> bool:
    return 值 is None or isinstance(值, (str, int, float, bool))


def 类型名(值: Any) -> str:
    return "缺失" if 值 is 缺失 else type(值).__name__


def 元素身份(元素: Any, 字段序: tuple[tuple[str, ...], ...] = 身份字段序) -> str:
    """数组元素身份键：对象取身份字段组首个全命中；其余取完整规范文本。"""
    if isinstance(元素, dict):
        for 组 in 字段序:
            if all(元素.get(字段) not in (None, "") for 字段 in 组):
                return "键=" + 规范文本([list(组), [元素[字段] for 字段 in 组]])
    return "全文=" + 规范文本(元素)


def 元素标签(元素: Any) -> str:
    """冲突诊断用的可读标签（身份字段值优先）。"""
    if isinstance(元素, dict):
        for 字段 in ("特性名", "名称", "用例", "锚文本"):
            if 元素.get(字段):
                return str(元素[字段])[:40]
    return 规范文本(元素)[:40]


def _键序(*表: Any) -> list[str]:
    """base 序 + ours 新增 + theirs 新增（去重保序）。"""
    序: list[str] = []
    有: set[str] = set()
    for 单表 in 表:
        for 键 in 单表:
            if 键 not in 有:
                有.add(键)
                序.append(键)
    return 序


def _分组(表: list, 字段序: tuple[tuple[str, ...], ...]) -> dict[str, list]:
    """按身份分组，组内保持原序（同身份多元素按出现序配对）。"""
    组: dict[str, list] = {}
    for 元素 in 表:
        组.setdefault(元素身份(元素, 字段序), []).append(元素)
    return 组


def _键值并集(基: Any, 本: Any, 对: Any, 路径: str,
              字段序: tuple[tuple[str, ...], ...], 计: 并集统计) -> Any:
    """单键/单元素的删除裁决；返回 缺失 表示该项在结果中不存在。"""
    if 本 is 缺失 and 对 is 缺失:
        计.删除生效 += 1
        return 缺失
    if 本 is 缺失 or 对 is 缺失:
        存续 = 对 if 本 is 缺失 else 本
        if 基 is 缺失:
            return 存续                     # 仅一侧新增（另一侧本就没有）→ 保留
        删除侧 = "ours" if 本 is 缺失 else "theirs"
        if 规范文本(存续) != 规范文本(基):
            raise 并集冲突(
                f"{路径} 删除/修改冲突：{删除侧} 已删除而另一侧已改内容，无法安全裁决")
        计.删除生效 += 1
        return 缺失
    return _并集(基, 本, 对, 路径, 字段序, 计)


def _字典并集(基: Any, 本: dict, 对: dict, 路径: str,
              字段序: tuple[tuple[str, ...], ...], 计: 并集统计) -> dict:
    基表 = {} if 基 is 缺失 else 基
    出: dict[str, Any] = {}
    for 键 in _键序(基表, 本, 对):
        值 = _键值并集(基表.get(键, 缺失), 本.get(键, 缺失), 对.get(键, 缺失),
                       f"{路径}.{键}", 字段序, 计)
        if 值 is not 缺失:
            出[键] = 值
    return 出


def _列表并集(基: Any, 本: list, 对: list, 路径: str,
              字段序: tuple[tuple[str, ...], ...], 计: 并集统计) -> list:
    基表 = [] if 基 is 缺失 else 基
    基组 = _分组(基表, 字段序)
    本组 = _分组(本, 字段序)
    对组 = _分组(对, 字段序)
    出: list[Any] = []
    for 键 in _键序(基组, 本组, 对组):
        基列 = 基组.get(键, [])
        本列 = 本组.get(键, [])
        对列 = 对组.get(键, [])
        if max(len(基列), len(本列), len(对列)) > 1:
            全同 = len({规范文本(x) for 列 in (基列, 本列, 对列) for x in 列}) == 1
            if not 全同:
                计.并列身份组 += 1
        首元素 = 基列[0] if 基列 else (本列[0] if 本列 else 对列[0])
        for 序 in range(max(len(基列), len(本列), len(对列))):
            值 = _键值并集(
                基列[序] if 序 < len(基列) else 缺失,
                本列[序] if 序 < len(本列) else 缺失,
                对列[序] if 序 < len(对列) else 缺失,
                f"{路径}[{元素标签(首元素)}#{序}]", 字段序, 计)
            if 值 is not 缺失:
                出.append(值)
    return 出


def _标量并集(基: Any, 本: Any, 对: Any, 路径: str, 计: 并集统计) -> Any:
    if 规范文本(本) == 规范文本(对):
        return 本
    if 基 is not 缺失 and 规范文本(本) == 规范文本(基):
        return 对
    if 基 is not 缺失 and 规范文本(对) == 规范文本(基):
        return 本
    计.标量分歧.append(路径)
    return 本


def _并集(基: Any, 本: Any, 对: Any, 路径: str,
          字段序: tuple[tuple[str, ...], ...], 计: 并集统计) -> Any:
    if isinstance(本, dict) and isinstance(对, dict):
        if 基 is not 缺失 and not isinstance(基, dict):
            raise 并集冲突(
                f"{路径} 结构类型冲突：base={类型名(基)} ours=dict theirs=dict")
        return _字典并集(基, 本, 对, 路径, 字段序, 计)
    if isinstance(本, list) and isinstance(对, list):
        if 基 is not 缺失 and not isinstance(基, list):
            raise 并集冲突(
                f"{路径} 结构类型冲突：base={类型名(基)} ours=list theirs=list")
        return _列表并集(基, 本, 对, 路径, 字段序, 计)
    if 是标量(本) and 是标量(对):
        if 基 is not 缺失 and not 是标量(基):
            raise 并集冲突(
                f"{路径} 结构类型冲突：base={类型名(基)} ours={类型名(本)} "
                f"theirs={类型名(对)}")
        return _标量并集(基, 本, 对, 路径, 计)
    raise 并集冲突(
        f"{路径} 结构类型冲突：base={类型名(基)} ours={类型名(本)} theirs={类型名(对)}")


def 三路并集(基: Any, 本: Any, 对: Any,
            字段序: tuple[tuple[str, ...], ...] = 身份字段序) -> tuple[Any, 并集统计]:
    """结构级三路并集入口；结构无法裁决时抛 并集冲突。"""
    计 = 并集统计()
    出 = _并集(基, 本, 对, "$", 字段序, 计)
    return 出, 计


def 读JSON(路径: str, 必含键: str | None, 允许空基: bool = False) -> Any:
    """读一侧 JSON：冲突标记/解析失败/顶层结构不符一律抛 并集冲突（fail-safe）。

    允许空基=True 用于 %O（无共同基的 add/add 场景为 0 字节）：返回 缺失 由调用方
    按 本 侧类型补空容器。
    """
    文本 = Path(路径).read_text(encoding="utf-8")
    for 标记 in 冲突标记:
        if 标记 in 文本:
            raise 并集冲突(f"{路径} 含冲突标记 {标记}（半成品禁参与并集）")
    if 文本.strip() == "":
        if 允许空基:
            return 缺失
        raise 并集冲突(f"{路径} 为空文件")
    try:
        文档 = json.loads(文本)
    except json.JSONDecodeError as e:
        raise 并集冲突(f"{路径} JSON 不可解析: {e}") from e
    if 必含键 is not None and (not isinstance(文档, dict)
                               or not isinstance(文档.get(必含键), list)):
        raise 并集冲突(f"{路径} 结构不符（顶层须含 {必含键} 数组）")
    return 文档


def _原子写(路径: str, 文本: str) -> None:
    """先写临时件再替换：绝不让半成品覆盖 %A（git 侧仍持有冲突态）。

    换行固定 LF（447-a 同族：win 检出若落 CRLF，合并结果与库 blob 全文件异形，
    下游 diff/门禁噪声掩盖真实改动面）。
    """
    目标 = Path(路径)
    临时 = 目标.with_name(目标.name + ".union_tmp")
    try:
        with 临时.open("w", encoding="utf-8", newline="\n") as f:
            f.write(文本)
        os.replace(临时, 目标)
    finally:
        if 临时.exists():
            临时.unlink()


def 运行驱动(必含键: str, 计数名: str, 缩进: int, 日志前缀: str) -> int:
    """git merge driver 主流程：<base> <ours> <theirs>，就地改写 ours。"""
    参 = sys.argv
    if len(参) != 4:
        print(f"用法: {Path(参[0]).name} <base> <ours> <theirs>", file=sys.stderr)
        return 2
    基路径, 本路径, 对路径 = 参[1], 参[2], 参[3]
    try:
        基 = 读JSON(基路径, 必含键, 允许空基=True)
        本 = 读JSON(本路径, 必含键)
        对 = 读JSON(对路径, 必含键)
        if 基 is 缺失:
            基 = {} if isinstance(本, dict) else []
        出, 计 = 三路并集(基, 本, 对)
        if not isinstance(出, dict) or not isinstance(出.get(必含键), list):
            raise 并集冲突(f"并集结果结构不符（顶层须含 {必含键} 数组）")
        for 分歧路径 in 计.标量分歧:
            print(f"{日志前缀} 告警：{分歧路径} 两侧均改且不同，取 ours（登记制既有口径）",
                  file=sys.stderr)
        if 计.删除生效:
            print(f"{日志前缀} 删除生效 {计.删除生效} 项（一侧删除而另一侧未改）",
                  file=sys.stderr)
        if 计.并列身份组:
            print(f"{日志前缀} 并列身份 {计.并列身份组} 组（同身份多元素按出现序配对）",
                  file=sys.stderr)
        _原子写(本路径, json.dumps(出, ensure_ascii=False, indent=缩进) + "\n")
    except 并集冲突 as e:
        print(f"{日志前缀} fail-safe 回退普通冲突: {e}", file=sys.stderr)
        return 1
    except OSError as e:
        print(f"{日志前缀} fail-safe 回退普通冲突: {e}", file=sys.stderr)
        return 1
    print(f"{日志前缀} 结构并集完成：{len(出[必含键])} {计数名}")
    return 0
