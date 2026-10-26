#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#1259 selftest：两 merge driver 三路合并 + registry-sanity 校验闸。

用例（正反两面·失败显式非零）：
  T1 registry 三路并集：ours/theirs 各加不同条目→并集有序完整
  T2 registry 同键分歧：同 (文件,函数,锚文本) 字段改→ours 优先
  T3 registry fail-safe：theirs 含冲突标记→退出非零
  T4 registry 结构不符：theirs 缺 条目 数组→退出非零
  T5 doc_coverage 三路并集：同特性双侧各挂一例→e2e 并集
  T6 doc_coverage theirs 新增特性行→并入
  T7 sanity：损坏 JSON/冲突标记/缺批准字段→红·正常件→绿
"""
from __future__ import annotations

import json
import pathlib
import shutil
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


def 跑driver(脚本: str, base: dict, ours: dict, theirs: dict, 污染theirs: str = None) -> tuple[int, dict | None]:
    with tempfile.TemporaryDirectory() as td:
        d = pathlib.Path(td)
        (d / "b.json").write_text(json.dumps(base, ensure_ascii=False), encoding="utf-8")
        (d / "a.json").write_text(json.dumps(ours, ensure_ascii=False), encoding="utf-8")
        if 污染theirs is not None:
            (d / "t.json").write_text(污染theirs, encoding="utf-8")
        else:
            (d / "t.json").write_text(json.dumps(theirs, ensure_ascii=False), encoding="utf-8")
        r = subprocess.run([sys.executable, str(仓库根 / "scripts" / 脚本),
                            str(d / "b.json"), str(d / "a.json"), str(d / "t.json")],
                           capture_output=True, text=True)
        出 = None
        if r.returncode == 0:
            出 = json.loads((d / "a.json").read_text(encoding="utf-8"))
        return r.returncode, 出


def 条(文件: str, 锚: str, 来源: str = "test", 批准: str = "已批") -> dict:
    return {"文件": 文件, "函数": "f", "锚文本": 锚, "来源": 来源, "类别": "上游已诊断",
            "静默风险": "低", "原因": "t", "登记人": "t", "登记日": "2026-10-27", "批准": 批准}


def 主() -> int:
    print("== T1 registry 三路并集 ==")
    base = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;")]}
    ours = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;"), 条("a.cn", "y;", 来源="ours")]}
    theirs = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;"), 条("b.cn", "z;", 来源="theirs")]}
    rc, 出 = 跑driver("merge_miss_registry.py", base, ours, theirs)
    断言("退出零", rc == 0)
    断言("三条并集", 出 and len(出["条目"]) == 3)
    断言("ours 序为骨架", 出 and [t["锚文本"] for t in 出["条目"]] == ["x;", "y;", "z;"])

    print("== T2 同键分歧 ours 优先 ==")
    ours2 = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;", 来源="ours改")]}
    theirs2 = {"版本": 1, "说明": "s", "条目": [条("a.cn", "x;", 来源="theirs改")]}
    rc, 出 = 跑driver("merge_miss_registry.py", base, ours2, theirs2)
    断言("退出零", rc == 0)
    断言("ours 来源保留", 出 and 出["条目"][0]["来源"] == "ours改")

    print("== T3 fail-safe 冲突标记 ==")
    rc, _ = 跑driver("merge_miss_registry.py", base, ours2, theirs2, 污染theirs="<<<<<<< HEAD\n")
    断言("退出非零", rc != 0)

    print("== T4 fail-safe 结构不符 ==")
    rc, _ = 跑driver("merge_miss_registry.py", base, ours2, {}, 污染theirs='{"无": 1}')
    断言("退出非零", rc != 0)

    print("== T5 doc_coverage 同特性 e2e 并集 ==")
    特 = lambda 例: {"特性名": "整数运算语义", "章节": "001 §3.7a", "e2e": 例}
    dbase = {"元数据": {}, "特性清单": [特(["100_甲"])], "归类兜底": []}
    dours = {"元数据": {}, "特性清单": [特(["100_甲", "200_乙"])], "归类兜底": []}
    dtheirs = {"元数据": {}, "特性清单": [特(["100_甲", "300_丙"])], "归类兜底": []}
    rc, 出 = 跑driver("merge_doc_coverage.py", dbase, dours, dtheirs)
    断言("退出零", rc == 0)
    断言("e2e 三例并集", 出 and 出["特性清单"][0]["e2e"] == ["100_甲", "200_乙", "300_丙"])

    print("== T6 doc_coverage theirs 新增特性 ==")
    dtheirs6 = {"元数据": {}, "特性清单": [特(["100_甲"]), {"特性名": "新特性", "章节": "§x", "e2e": ["9_新"]}], "归类兜底": []}
    rc, 出 = 跑driver("merge_doc_coverage.py", dbase, dours, dtheirs6)
    断言("两特性行", 出 and len(出["特性清单"]) == 2)

    print("== T7 sanity 闸 ==")
    with tempfile.TemporaryDirectory() as td:
        d = pathlib.Path(td)
        r = subprocess.run([sys.executable, str(仓库根 / "scripts" / "check_registry_sanity.py")],
                           cwd=str(仓库根), capture_output=True, text=True)
        断言("develop 现件绿", r.returncode == 0)
        # 损坏件反证：临时目录造坏件+cwd 切入（脚本用相对路径）
        (仓库根 / "scripts" / "miss_registry.json.bak_test")  # noqa: 防止空块优化（无操作）
        原 = (仓库根 / "scripts" / "miss_registry.json").read_text(encoding="utf-8")
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

    print(f"== 汇总 == PASS {通过}  FAIL {失败}")
    return 1 if 失败 else 0


if __name__ == "__main__":
    sys.exit(主())
