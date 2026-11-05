#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""高冲突单点 JSON merge driver selftest（#1259 立·#1942 补丢行禁令）。

覆盖两面：
  正例——结构级并集不丢行：同锚位两侧各自追加、双桶并行追加、删除传播、
        登记表同锚不同函数共存、缺身份字段条目按全文保两侧、空基 add/add、
        并集幂等（不膨胀）、输出合法 JSON + 无重复。
  负例——fail-safe：解析失败/结构类型冲突/删改冲突/冲突标记/顶层结构不符
        一律退出非零，且**不改写 %A**（禁产出可能丢数据的合并结果）。

#1942 实测：#1942 前的旧 driver 在 T8/T9/T10 形状上退出 0 却吞掉分支侧新增行
（doc_coverage 只并集「特性清单」，其余顶层键取 ours；rebase 时 ours=上游 develop），
即本文件 T8/T9 在旧实现上 FAIL。回归防线=gate_quick 静态段「merge driver 并集自测」。
"""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import tempfile

仓库根 = pathlib.Path(__file__).resolve().parent.parent
通过 = 0
失败 = 0


def 断言(名: str, 条件: bool) -> None:
    global 通过, 失败
    if 条件:
        通过 += 1
        print(f"  PASS {名}")
    else:
        失败 += 1
        print(f"  FAIL {名}")


def 跑driver(脚本: str, base: dict | None, ours: dict, theirs: dict,
             原文base: str = None, 原文theirs: str = None) -> tuple[int, dict | None, str]:
    """返回 (退出码, 合并结果或 None, %A 原样文本)。原文* 用于注入坏件。"""
    with tempfile.TemporaryDirectory() as td:
        d = pathlib.Path(td)
        基文 = 原文base if 原文base is not None else json.dumps(base, ensure_ascii=False)
        (d / "b.json").write_text(基文, encoding="utf-8", newline="\n")
        (d / "a.json").write_text(json.dumps(ours, ensure_ascii=False), encoding="utf-8")
        对文 = 原文theirs if 原文theirs is not None else json.dumps(theirs, ensure_ascii=False)
        (d / "t.json").write_text(对文, encoding="utf-8")
        运行前 = (d / "a.json").read_text(encoding="utf-8")
        r = subprocess.run([sys.executable, str(仓库根 / "scripts" / 脚本),
                            str(d / "b.json"), str(d / "a.json"), str(d / "t.json")],
                           capture_output=True, text=True)
        运行后 = (d / "a.json").read_text(encoding="utf-8")
        出 = None
        if r.returncode == 0:
            出 = json.loads(运行后)
        return r.returncode, 出, 运行前 if r.returncode != 0 else 运行后


def 条(文件: str, 锚: str, 来源: str = "test", 函数: str = "f") -> dict:
    return {"文件": 文件, "函数": 函数, "锚文本": 锚, "来源": 来源, "类别": "上游已诊断",
            "静默风险": "低", "原因": "t", "登记人": "t", "登记日": "2026-10-27", "批准": "已批"}


def 台账(特性: list, 兜底: list, 豁免: list = None) -> dict:
    return {"元数据": {"任务": "999", "名称": "台账"}, "特性清单": 特性,
            "归类兜底": 兜底, "归类豁免": 豁免 or []}


def 特性(名: str, 例: list) -> dict:
    return {"特性名": 名, "章节": "001 §x", "e2e": 例}


def 桶(名: str, 模式: list, 说明: str = "s") -> dict:
    return {"名称": 名, "模式": 模式, "说明": 说明}


DOC = "merge_doc_coverage.py"
REG = "merge_miss_registry.py"


def 主() -> int:
    print("== T1 registry 三路并集 ==")
    base = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;")]}
    ours = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;"), 条("a.cn", "y;", 来源="ours")]}
    theirs = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;"), 条("b.cn", "z;", 来源="theirs")]}
    rc, 出, _ = 跑driver(REG, base, ours, theirs)
    断言("退出零", rc == 0)
    断言("三条并集", 出 and len(出["条目"]) == 3)
    断言("base 序+ours 新增+theirs 新增", 出 and [t["锚文本"] for t in 出["条目"]] == ["x;", "y;", "z;"])

    print("== T2 registry 同键分歧 ours 优先 ==")
    ours2 = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;", 来源="ours改")]}
    theirs2 = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;", 来源="theirs改")]}
    rc, 出, _ = 跑driver(REG, base, ours2, theirs2)
    断言("退出零", rc == 0)
    断言("ours 来源保留", 出 and 出["条目"][0]["来源"] == "ours改")

    print("== T3 registry fail-safe 冲突标记 ==")
    rc, _, 原样 = 跑driver(REG, base, ours2, theirs2, 原文theirs="<<<<<<< HEAD\n")
    断言("退出非零", rc != 0)
    断言("不改写 %A", 原样 == json.dumps(ours2, ensure_ascii=False))

    print("== T4 registry fail-safe 结构不符 ==")
    rc, _, _ = 跑driver(REG, base, ours2, {}, 原文theirs='{"无": 1}')
    断言("退出非零", rc != 0)

    print("== T5 doc_coverage 同特性 e2e 并集 ==")
    特 = lambda 例: 特性("整数运算语义", 例)
    dbase = 台账([特(["100_甲"])], [])
    dours = 台账([特(["100_甲", "200_乙"])], [])
    dtheirs = 台账([特(["100_甲", "300_丙"])], [])
    rc, 出, _ = 跑driver(DOC, dbase, dours, dtheirs)
    断言("退出零", rc == 0)
    断言("e2e 三例并集", 出 and 出["特性清单"][0]["e2e"] == ["100_甲", "200_乙", "300_丙"])

    print("== T6 doc_coverage theirs 新增特性 ==")
    dtheirs6 = 台账([特(["100_甲"]), 特性("新特性", ["9_新"])], [])
    rc, 出, _ = 跑driver(DOC, dbase, dours, dtheirs6)
    断言("两特性行", 出 and len(出["特性清单"]) == 2)

    print("== T7 sanity 闸 ==")
    原 = (仓库根 / "scripts" / "miss_registry.json").read_text(encoding="utf-8")
    r = subprocess.run([sys.executable, str(仓库根 / "scripts" / "check_registry_sanity.py")],
                       cwd=str(仓库根), capture_output=True, text=True)
    断言("develop 现件绿", r.returncode == 0)
    try:
        (仓库根 / "scripts" / "miss_registry.json").write_text("<<<<<<< HEAD\n", encoding="utf-8")
        r = subprocess.run([sys.executable, str(仓库根 / "scripts" / "check_registry_sanity.py")],
                           cwd=str(仓库根), capture_output=True, text=True)
        断言("冲突标记红", r.returncode == 1)
    finally:
        (仓库根 / "scripts" / "miss_registry.json").write_text(原, encoding="utf-8")
    r = subprocess.run([sys.executable, str(仓库根 / "scripts" / "check_registry_sanity.py")],
                       cwd=str(仓库根), capture_output=True, text=True)
    断言("恢复后绿", r.returncode == 0)

    # ---- #1942 丢行复现族（旧实现在 T8/T9 上退出 0 而吞行）----
    print("== T8 丢行复现：兜底桶同锚尾行后分支侧追加 ==")
    b8 = 台账([特性("零参方法", ["100_甲"])], [桶("综合回归", ["1537_旧", "1564_尾"])])
    o8 = 台账([特性("零参方法", ["100_甲", "1315_零参方法值类型推断"])],
              [桶("综合回归", ["1537_旧", "1564_尾"])])
    t8 = 台账([特性("零参方法", ["100_甲"])],
              [桶("综合回归", ["1537_旧", "1564_尾", "1685_容器声明初始化源形态深拷",
                               "1686_容器赋值源形态深拷"])])
    rc, 出, _ = 跑driver(DOC, b8, o8, t8)
    桶模 = 出["归类兜底"][0]["模式"] if 出 else []
    断言("退出零", rc == 0)
    断言("theirs 追加 1685/1686 未被吞", "1685_容器声明初始化源形态深拷" in 桶模
         and "1686_容器赋值源形态深拷" in 桶模)
    断言("ours 追加 1683 未被吞", 出 and "1315_零参方法值类型推断" in 出["特性清单"][0]["e2e"])
    断言("桶内条数=base+新增 无重复", 桶模 == ["1537_旧", "1564_尾",
                                              "1685_容器声明初始化源形态深拷",
                                              "1686_容器赋值源形态深拷"])

    print("== T9 同锚位两侧各自追加（同一位置双增）==")
    b9 = 台账([特性("X", ["a"])], [桶("B", ["尾锚"])])
    o9 = 台账([特性("X", ["a", "ours新增"])], [桶("B", ["尾锚", "ours行"])])
    t9 = 台账([特性("X", ["a"])], [桶("B", ["尾锚", "theirs行"])])
    rc, 出, _ = 跑driver(DOC, b9, o9, t9)
    断言("退出零", rc == 0)
    断言("两侧追加全在", 出 and 出["归类兜底"][0]["模式"] == ["尾锚", "ours行", "theirs行"])

    print("== T10 删除传播（一侧删另一侧未改）==")
    b10 = 台账([特性("X", ["a", "已清"])], [])
    o10 = 台账([特性("X", ["a"])], [])
    rc, 出, _ = 跑driver(DOC, b10, o10, b10)
    断言("退出零", rc == 0)
    断言("删除生效", 出 and 出["特性清单"][0]["e2e"] == ["a"])

    print("== T11 fail-safe 删改冲突 ==")
    b11 = 台账([特性("将被删", ["a"])], [])
    o11 = 台账([], [])                                  # ours 删除整行
    t11 = 台账([特性("将被删", ["a", "改后"])], [])       # theirs 改了同一行
    rc, _, 原样 = 跑driver(DOC, b11, o11, t11)
    断言("退出非零", rc != 0)
    断言("不改写 %A", 原样 == json.dumps(o11, ensure_ascii=False))

    print("== T12 fail-safe 结构类型冲突（同键数组 vs 对象）==")
    台12 = {"元数据": {"统计": [1, 2]}, "特性清单": [], "归类兜底": []}
    台12对 = {"元数据": {"统计": {"总行数": 1}}, "特性清单": [], "归类兜底": []}
    rc, _, _ = 跑driver(DOC, {"元数据": {"统计": {}}, "特性清单": [], "归类兜底": []},
                        台12, 台12对)
    断言("退出非零", rc != 0)

    print("== T13 fail-safe 解析失败 ==")
    rc, _, 原样 = 跑driver(DOC, dbase, dours, dtheirs, 原文theirs='{"特性清单": [')
    断言("退出非零", rc != 0)
    断言("不改写 %A", 原样 == json.dumps(dours, ensure_ascii=False))

    print("== T14 registry 同锚不同函数共存 ==")
    b14 = {"版本": 1, "条目": [条("a.cn", "} else {", 函数="f1")]}
    o14 = {"版本": 1, "条目": [条("a.cn", "} else {", 函数="f1"),
                              条("a.cn", "} else {", 函数="f2", 来源="ours")]}
    t14 = {"版本": 1, "条目": [条("a.cn", "} else {", 函数="f1"),
                              条("a.cn", "} else {", 函数="f3", 来源="theirs")]}
    rc, 出, _ = 跑driver(REG, b14, o14, t14)
    断言("退出零", rc == 0)
    断言("三函数三条共存", 出 and [t["函数"] for t in 出["条目"]] == ["f1", "f2", "f3"])

    print("== T15 registry 缺身份字段条目按全文保两侧 ==")
    无锚 = {"来源": "x", "类别": "上游已诊断", "批准": "已批"}
    b15 = {"版本": 1, "条目": []}
    o15 = {"版本": 1, "条目": [dict(无锚, 来源="ours")]}
    t15 = {"版本": 1, "条目": [dict(无锚, 来源="theirs")]}
    rc, 出, _ = 跑driver(REG, b15, o15, t15)
    断言("退出零", rc == 0)
    断言("两条都保留", 出 and sorted(t["来源"] for t in 出["条目"]) == ["ours", "theirs"])

    print("== T16 空基 add/add ==")
    rc, 出, _ = 跑driver(REG, None, {"版本": 1, "条目": [条("a.cn", "o;")]},
                        {"版本": 1, "条目": [条("b.cn", "t;")]}, 原文base="")
    断言("退出零", rc == 0)
    断言("两侧条目都在", 出 and len(出["条目"]) == 2)

    print("== T17 并集幂等（结果再并一次不膨胀）==")
    rc, 出1, _ = 跑driver(DOC, b8, o8, t8)
    rc2, 出2, _ = 跑driver(DOC, 出1, 出1, 出1)
    断言("两次退出零", rc == 0 and rc2 == 0)
    断言("字节一致", 出1 == 出2 and len(出2["归类兜底"][0]["模式"]) == 4)

    print("== T18 输出合法 JSON 且无重复 ===")
    rc, _, 文本 = 跑driver(DOC, b8, o8, t8)
    合法 = True
    try:
        json.loads(文本)
    except json.JSONDecodeError:
        合法 = False
    e2e = 出1["特性清单"][0]["e2e"]
    断言("合法 JSON", 合法)
    断言("e2e 无重复", len(e2e) == len(set(e2e)))

    print(f"== 汇总 == PASS {通过}  FAIL {失败}")
    return 1 if 失败 else 0


if __name__ == "__main__":
    sys.exit(主())
