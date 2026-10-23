#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_check_peephole_sanitizer.py —— 机械锁断言判别力自测（任务 1217）。

不编译、不跑 sanitizer：直接向 check_peephole_sanitizer.py 的核对函数喂合成
gtest 输出，证明以下假绿形态必红（审查 P2-1/P2-2/P2-3「保留自检判别力」的
机械执行面）：
  ① 运行汇总与清单数不符 / 通过+跳过!=总数 / 缺汇总行 / FAILED 行；
  ② ASan 口径出现 SKIPPED（死亡例被 SKIP 分支顶替的典型形态）；
  ③ 死亡例从 --gtest_list_tests 清单消失（CN_PEEPHOLE_ASAN_DEATH 宏失效）；
  ④ 同数顶替：总数不变但两条死亡例被普通用例替换（源码差集为空，关键名缺失）；
  ⑤ _GLIBCXX_DEBUG 口径混入死亡例 / 缺少显式 SKIP 例 / 跳过数不符；
  ⑥ gtest 1.8/1.9 的「test cases」旧称与含 '/' 的参数化跳过名。
断言统一用显式 if not + SystemExit(1)（不用 assert）：python3 -O /
  PYTHONOPTIMIZE=1 下仍真实执行，不会出现「assert 被移除→假绿」。
用法：python scripts/test_check_peephole_sanitizer.py   （rc=0=全绿）
"""
from __future__ import annotations

import sys
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
import check_peephole_sanitizer as 锁  # noqa: E402

正例通过 = 0
负例通过 = 0


def 断言(条件: bool, 说明: str, 负例: bool = False) -> None:
    """显式失败（-O 下 assert 会被移除，故禁用 assert——审查 P2-1）。"""
    global 正例通过, 负例通过
    if not 条件:
        print("自测失败：" + 说明, flush=True)
        raise SystemExit(1)
    if 负例:
        负例通过 += 1
    else:
        正例通过 += 1


def 汇总输出(总数: int, 通过数: int, 跳过: int = 0,
             跳过名单: tuple[str, ...] = (), 旧称: bool = False) -> str:
    套件词 = "cases" if 旧称 else "suites"
    行 = [
        f"[==========] {总数} tests from 6 test {套件词} ran. (1 ms total)",
        f"[  PASSED  ] {通过数} tests.",
    ]
    if 跳过:
        行.append(f"[  SKIPPED ] {跳过} "
                 + ("test, listed below:" if 跳过 == 1 else "tests, listed below:"))
        行.extend(f"[  SKIPPED ] {名}" for 名 in 跳过名单)
    return "\n".join(行) + "\n"


def 清单输出(用例集: set[str]) -> str:
    """按套件分组生成 --gtest_list_tests 输出。"""
    分组: dict[str, list[str]] = {}
    for 名 in sorted(用例集):
        套件, 短名 = 名.split(".", 1)
        分组.setdefault(套件, []).append(短名)
    行: list[str] = []
    for 套件 in sorted(分组):
        行.append(套件 + ".")
        行.extend("  " + 短名 for 短名 in 分组[套件])
    return "\n".join(行) + "\n"


def 用例集() -> tuple[set[str], set[str], set[str]]:
    """实测源码 TEST 集合 S、死亡例集、跳过例集。"""
    return 锁.源码用例集(), set(锁.死亡例名), {锁.跳过例名}


def 正例_asansan() -> None:
    源码集, 死亡集, 跳过集 = 用例集()
    L = 源码集 - 跳过集
    错 = 锁.核对用例清单("asan+ubsan", 清单输出(L), 源码集=源码集)
    断言(错 == [], f"ASan 正确清单（S-SKIP 例，含两条死亡例）无错：{错}")
    汇总 = 汇总输出(len(L), len(L))
    断言(锁.核对汇总("asan+ubsan", 汇总, len(L)) == [], "ASan 正确汇总自洽无错")


def 正例_debug() -> None:
    源码集, 死亡集, 跳过集 = 用例集()
    L = 源码集 - 死亡集
    错 = 锁.核对用例清单("glibcxx-debug", 清单输出(L), 源码集=源码集)
    断言(错 == [], f"debug 正确清单（S-死亡例，含显式 SKIP 例）无错：{错}")
    汇总 = 汇总输出(len(L), len(L) - 1, 1, (锁.跳过例名,))
    断言(锁.核对汇总("glibcxx-debug", 汇总, len(L)) == [], "debug 正确汇总自洽无错")


def 正例_格式兼容() -> None:
    旧 = 汇总输出(10, 10, 旧称=True)
    断言(锁.解析汇总(旧)["总数"] == 10, "gtest 1.8/1.9「test cases」旧称可解析")
    参数化名 = "PeepholeSlotLifetime/Param.OldView/0"
    名单 = 锁.跳过用例名(汇总输出(5, 4, 1, (参数化名,)))
    断言(名单 == [参数化名], "参数化跳过名（含 '/'）可解析")


def 负例_汇总不自洽() -> None:
    源码集, _, 跳过集 = 用例集()
    L = 源码集 - 跳过集
    汇总 = 汇总输出(len(L) - 1, len(L) - 1)
    错 = 锁.核对汇总("asan+ubsan", 汇总, len(L))
    断言(any("运行总数" in 条 for 条 in 错), "运行总数 != 清单数：必红", 负例=True)
    汇总 = 汇总输出(len(L), len(L) - 1)
    错 = 锁.核对汇总("asan+ubsan", 汇总, len(L))
    断言(any("不自洽" in 条 for 条 in 错), "通过+跳过 != 总数：必红", 负例=True)
    缺行 = "[  PASSED  ] 1 test.\n"
    断言(锁.核对汇总("asan+ubsan", 缺行, 1) != [], "缺 tests ran 汇总行：必红",
         负例=True)
    失败行 = 汇总输出(len(L), len(L)) + "[  FAILED  ] 1 test.\n"
    断言(锁.核对汇总("asan+ubsan", 失败行, len(L)) != [], "出现 FAILED 行：必红",
         负例=True)


def 负例_asan跳过() -> None:
    源码集, _, 跳过集 = 用例集()
    L = 源码集 - 跳过集
    汇总 = 汇总输出(len(L) - 1, len(L) - 1, 1, (锁.跳过例名,))
    错 = 锁.核对汇总("asan+ubsan", 汇总, len(L) - 1)
    断言(any("SKIPPED" in 条 for 条 in 错), "ASan 口径出现 SKIPPED：必红", 负例=True)


def 负例_宏失效() -> None:
    """#if 0 隐藏死亡例：死亡例被 SKIP 分支顶替（清单差集/关键名/跳过面三重红）。"""
    源码集, 死亡集, 跳过集 = 用例集()
    L = 源码集 - 死亡集  # 只剩非 ASan 分支（含跳过例）
    错 = 锁.核对用例清单("asan+ubsan", 清单输出(L), 源码集=源码集)
    断言(any("源码有而清单无" in 条 for 条 in 错), "ASan 清单缺死亡例（差集）：必红",
         负例=True)
    断言(any("关键用例缺失" in 条 for 条 in 错), "ASan 清单缺死亡例（关键名）：必红",
         负例=True)
    断言(any("不应含" in 条 for 条 in 错), "ASan 清单混入 SKIP 例：必红", 负例=True)


def 负例_同数顶替() -> None:
    """总数不变但死亡例被两条普通用例替换：差集为空，关键名缺失必须红。"""
    源码集, 死亡集, 跳过集 = 用例集()
    替身 = {"PeepholeSelfTest.SubA", "PeepholeSelfTest.SubB"}
    篡改源 = (源码集 - 死亡集) | 替身  # 源码本身被改成同数普通用例
    L = 篡改源 - 跳过集
    错 = 锁.核对用例清单("asan+ubsan", 清单输出(L), 源码集=篡改源)
    断言(any("关键用例缺失" in 条 for 条 in 错), "同数顶替：关键名缺失必红", 负例=True)
    断言(not any("源码有而清单无" in 条 for 条 in 错),
         "同数顶替场景差集确为空（证明红色来自关键名断言而非差集）", 负例=True)


def 负例_debug口径() -> None:
    源码集, 死亡集, 跳过集 = 用例集()
    汇总 = 汇总输出(len(源码集) - 2, len(源码集) - 4, 2,
                 (锁.跳过例名, "PeepholeSelfTest.Extra"))
    错 = 锁.核对汇总("glibcxx-debug", 汇总, len(源码集) - 2)
    断言(any("应恰好 1 例" in 条 for 条 in 错), "debug 跳过 2 例：必红", 负例=True)
    L = 源码集 - 跳过集  # 误把死亡例编进 debug 口径
    错 = 锁.核对用例清单("glibcxx-debug", 清单输出(L), 源码集=源码集)
    断言(any("不应含" in 条 for 条 in 错), "debug 清单混入死亡例：必红", 负例=True)
    L = 源码集 - 死亡集 - 跳过集  # 显式 SKIP 例本身也消失
    错 = 锁.核对用例清单("glibcxx-debug", 清单输出(L), 源码集=源码集)
    断言(any("关键用例缺失" in 条 for 条 in 错),
         "debug 清单缺显式 SKIP 例：必红", 负例=True)
    错 = 锁.核对用例清单("glibcxx-debug", 清单输出(L), 源码集=set())
    断言(错 != [], "源码提取集合为空（正则/路径失效）：必红", 负例=True)


def main() -> int:
    正例_asansan()
    正例_debug()
    正例_格式兼容()
    负例_汇总不自洽()
    负例_asan跳过()
    负例_宏失效()
    负例_同数顶替()
    负例_debug口径()
    断言(正例通过 > 0 and 负例通过 > 0, "正/负例计数器均被真实执行（非空转）")
    print(f"自测全绿：正例 {正例通过} 项 + 负例 {负例通过} 项——"
          "汇总自洽/源码差集/关键名/SKIP 口径任一被破坏，机械锁必红")
    return 0


if __name__ == "__main__":
    sys.exit(main())
