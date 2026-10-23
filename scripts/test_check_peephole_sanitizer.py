#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_check_peephole_sanitizer.py —— 机械锁计数/清单断言判别力自测（任务 1217）。

不编译、不跑 sanitizer：直接向 check_peephole_sanitizer.py 的核对函数喂合成
gtest 输出，证明以下假绿形态必红（审查 P2-1「保留自检判别力」的机械执行面）：
  ① 分口径总数/通过数改错（含少 1 例：任一条用例静默消失即红）；
  ② ASan 口径出现 SKIPPED（死亡例被 SKIP 分支顶替的典型形态）；
  ③ 死亡例从 --gtest_list_tests 清单消失（CN_PEEPHOLE_ASAN_DEATH 宏失效）；
  ④ _GLIBCXX_DEBUG 口径混入死亡例 / 缺少显式 SKIP 例；
  ⑤ FAILED 行 / 汇总行缺失。
用法：python scripts/test_check_peephole_sanitizer.py   （rc=0=全绿）
"""
import sys
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))
import check_peephole_sanitizer as 锁  # noqa: E402

通过 = 0


def 断言(条件: bool, 说明: str) -> None:
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1


def 期望(模式: str) -> dict:
    return 锁.期望口径[模式]


def 运行输出(总数: int, 通过数: int, 跳过: int = 0,
             跳过名单: tuple[str, ...] = ()) -> str:
    行 = [
        f"[==========] {总数} tests from 6 test suites ran. (1 ms total)",
        f"[  PASSED  ] {通过数} tests.",
    ]
    if 跳过:
        行.append(f"[  SKIPPED ] {跳过} test, listed below:")
        行.extend(f"[  SKIPPED ] {名}" for 名 in 跳过名单)
    return "\n".join(行) + "\n"


def 清单输出(总数: int, 包含: tuple[str, ...]) -> str:
    """按套件分组生成 --gtest_list_tests 输出（包含项替换同数目的普通用例）。"""
    用例 = [f"PeepholeSelfTest.Case{编号}" for 编号 in range(总数 - len(包含))]
    用例.extend(包含)
    分组: dict[str, list[str]] = {}
    for 名 in 用例:
        套件, 短名 = 名.split(".", 1)
        分组.setdefault(套件, []).append(短名)
    行: list[str] = []
    for 套件 in sorted(分组):
        行.append(套件 + ".")
        行.extend("  " + 短名 for 短名 in 分组[套件])
    return "\n".join(行) + "\n"


def 正例_asan() -> None:
    数 = 期望("asan+ubsan")
    汇总 = 运行输出(数["总数"], 数["通过"])
    清单 = 清单输出(数["总数"], 锁.死亡例名)
    断言(锁.核对汇总("asan+ubsan", 汇总) == [], "ASan 正确输出：计数核对无错")
    断言(锁.核对用例清单("asan+ubsan", 清单) == [], "ASan 正确清单：含两条死亡例无错")


def 正例_debug() -> None:
    数 = 期望("glibcxx-debug")
    汇总 = 运行输出(数["总数"], 数["通过"], 数["跳过"], (锁.跳过例名,))
    清单 = 清单输出(数["总数"], (锁.跳过例名,))
    断言(锁.核对汇总("glibcxx-debug", 汇总) == [], "debug 正确输出：计数核对无错")
    断言(锁.核对用例清单("glibcxx-debug", 清单) == [], "debug 正确清单：含 SKIP 例无错")


def 负例_计数改错() -> None:
    数 = 期望("asan+ubsan")
    少一 = 运行输出(数["总数"] - 1, 数["通过"] - 1)
    错 = 锁.核对汇总("asan+ubsan", 少一)
    断言(any("总数" in 条 for 条 in 错), "ASan 总数少 1（用例静默消失）：必红")
    断言(any("通过" in 条 for 条 in 错), "ASan 通过数少 1：必红")
    通过数多一 = 运行输出(数["总数"], 数["通过"] + 1)
    断言(锁.核对汇总("asan+ubsan", 通过数多一) != [], "ASan 通过数多 1：必红")
    缺汇总 = "[  PASSED  ] 147 tests.\n"
    断言(锁.核对汇总("asan+ubsan", 缺汇总) != [], "缺 tests ran 汇总行：必红")


def 负例_asan跳过() -> None:
    """死亡例被 SKIP 分支顶替：gtest 仍 rc=0，旧脚本会假绿。"""
    数 = 期望("asan+ubsan")
    汇总 = 运行输出(数["总数"], 数["通过"], 1, (锁.跳过例名,))
    错 = 锁.核对汇总("asan+ubsan", 汇总)
    断言(any("SKIPPED" in 条 for 条 in 错), "ASan 口径出现 SKIPPED：必红")
    清单 = 清单输出(数["总数"], (锁.跳过例名,))  # 清单里死亡例被跳过例顶替
    错 = 锁.核对用例清单("asan+ubsan", 清单)
    断言(any("死亡例缺失" in 条 for 条 in 错), "ASan 清单缺死亡例（宏失效）：必红")
    断言(any("非 ASan 分支用例" in 条 for 条 in 错), "ASan 清单混入 SKIP 例：必红")


def 负例_debug口径() -> None:
    数 = 期望("glibcxx-debug")
    汇总 = 运行输出(数["总数"], 数["通过"] - 1, 2, (锁.跳过例名, "PeepholeSelfTest.Extra"))
    断言(锁.核对汇总("glibcxx-debug", 汇总) != [], "debug 跳过 2 例（超允许面）：必红")
    清单 = 清单输出(数["总数"], 锁.死亡例名[:1] + (锁.跳过例名,))
    断言(锁.核对用例清单("glibcxx-debug", 清单) != [],
         "debug 清单混入死亡例（宏未按口径生效）：必红")
    清单 = 清单输出(数["总数"], ())
    断言(锁.核对用例清单("glibcxx-debug", 清单) != [],
         "debug 清单缺显式 SKIP 例（用例静默消失）：必红")


def 负例_failed行() -> None:
    数 = 期望("asan+ubsan")
    汇总 = 运行输出(数["总数"], 数["通过"]) + "[  FAILED  ] 1 test.\n"
    断言(锁.核对汇总("asan+ubsan", 汇总) != [], "出现 FAILED 行：必红")


def main() -> int:
    正例_asan()
    正例_debug()
    负例_计数改错()
    负例_asan跳过()
    负例_debug口径()
    负例_failed行()
    print(f"自测全绿：{通过} 项断言（正例 4 + 负例 {通过 - 4}）——"
          "计数/跳过/死亡例存在性任一被破坏，机械锁必红")
    return 0


if __name__ == "__main__":
    sys.exit(main())
