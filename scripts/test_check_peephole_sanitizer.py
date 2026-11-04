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
  ⑥ gtest 1.8/1.9 的「test cases」旧称与含 '/' 的参数化跳过名；
  ⑦ 判别力样品屏障锚（#1858）：指针隐藏/内存屏障/源码序/防误删注释任一缺失
     必红——防「看似死代码」被考古清理后样品变脆、-O1 再失明（三态篡改负例
     验证锚检查自身判别力）。
断言统一用显式 if not + SystemExit(1)（不用 assert）：python3 -O /
  PYTHONOPTIMIZE=1 下仍真实执行，不会出现「assert 被移除→假绿」。
用法：python scripts/test_check_peephole_sanitizer.py   （rc=0=全绿）
      加 --真编译：另行真编译跑主工具 asan自检（正例绿+负例 -O0/-O1 双档红，
      需 g++/clang++；默认不开——L1 静态段零编译器秒级的既定设计，判别力
      运行时锁面=主工具每次运行必跑 asan自检）
"""
from __future__ import annotations

import argparse
import shutil
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


def 正例_负例屏障锚() -> None:
    """#1858 判别力样品「顽固性」锚齐全（指针隐藏/屏障/源码序/防误删注释）。"""
    错 = 锁.负例屏障锚检查(锁.负例源码)
    断言(错 == [], f"判别力样品屏障锚齐全（负例 free→use 机器序有双守卫固化）：{错}")


def 负例_屏障被拆() -> None:
    """锚检查自身判别力：三态篡改（删屏障/去 volatile/乱序）必红。"""
    锚 = 'asm volatile("" : : : "memory");'
    错 = 锁.负例屏障锚检查(锁.负例源码.replace(锚, ""))
    断言(any("屏障" in 条 for 条 in 错),
         "屏障行被删（考古清理「假死代码」形态）：必红", 负例=True)
    篡改 = 锁.负例源码.replace("char* volatile 隐藏指针 = p;", "char* 隐藏指针 = p;")
    错 = 锁.负例屏障锚检查(篡改)
    断言(any("指针隐藏" in 条 for 条 in 错),
         "指针隐藏被去 volatile（样品变脆形态）：必红", 负例=True)
    行表 = 锁.负例源码.splitlines(keepends=True)
    释放位 = next(i for i, 行 in enumerate(行表) if "delete[] 隐藏指针;" in 行)
    使用位 = next(i for i, 行 in enumerate(行表) if "隐藏指针[0]" in 行)
    行表[释放位], 行表[使用位] = 行表[使用位], 行表[释放位]
    错 = 锁.负例屏障锚检查("".join(行表))
    断言(any("顺序" in 条 for 条 in 错),
         "释放与使用源码序颠倒（被顺手重排形态）：必红", 负例=True)


def 正例_真编译判别力() -> None:
    """--真编译 显式段：真编译跑主工具 asan自检（正例绿+负例 -O0/-O1 双档必红）。

    默认不开（L1 静态段零编译器秒级的既定设计·1217 返工）；判别力运行时锁面
    =主工具每次运行必跑 asan自检。本段供 #1011 证据轮/换环境收口显式验证。
    """
    cxx = shutil.which("g++") or shutil.which("clang++")
    if cxx is None:
        print("[跳过] 未找到 g++/clang++：真编译判别力段未执行"
              "（静态锚断言已跑；主工具 asan自检 运行时硬依赖编译器）", flush=True)
        return
    断言(锁.asan自检(cxx) is True, "真编译判别力：正例绿+负例 -O0/-O1 双档必红")


def main() -> int:
    解析器 = argparse.ArgumentParser(description="窥孔机械锁判别力自测")
    解析器.add_argument("--真编译", action="store_true",
                       help="真编译跑主工具 asan自检（需 g++/clang++）")
    参数 = 解析器.parse_args()
    正例_asansan()
    正例_debug()
    正例_格式兼容()
    正例_负例屏障锚()
    负例_汇总不自洽()
    负例_asan跳过()
    负例_宏失效()
    负例_同数顶替()
    负例_debug口径()
    负例_屏障被拆()
    断言(正例通过 > 0 and 负例通过 > 0, "正/负例计数器均被真实执行（非空转）")
    if 参数.真编译:
        正例_真编译判别力()
    print(f"自测全绿：正例 {正例通过} 项 + 负例 {负例通过} 项——"
          "汇总自洽/源码差集/关键名/SKIP 口径/判别力样品屏障锚任一被破坏，机械锁必红")
    return 0


if __name__ == "__main__":
    sys.exit(main())
