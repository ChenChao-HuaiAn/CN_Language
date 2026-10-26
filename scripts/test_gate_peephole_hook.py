# -*- coding: utf-8 -*-
"""窥孔 sanitizer 挂载结构锁（1486·2026-10-26）。

#1486 缺口=「sanitizer 锁只登记不执行」——本自测防挂载本身再退化（防未来
被静默删挂载/删逃生门/删平台条件）：
  ① ci_static_gates.静态门禁清单 真函数两态：非 win 平台尾项=check_peephole_
     sanitizer（云端恒跑兜底面）；win-x64 不含（检查面仅 Linux/WSL·恒 rc=2
     无意义）；两清单仅差这一项（防顺手改动清单其他项）。
  ② gate_quick 挂载文本锚（gate_quick 导入即执行整门禁·不可 import，按源码
     文本锁）：写集条件步存在（窥孔三前缀）、rc==2 黄字显式降级（缺失显形
     非静默）、CN_GATE_PEEPHOLE_FORCE 逃生门、win 侧 wsl 包装、900s 超时。
  ③ check_peephole_sanitizer 依赖解析锚：找gtest源码 两候选仍在（共享
     third-party + cmake _deps——挂载生效的依赖前提）。

零环境依赖（不编译不联网·秒级）；显式 if 失败（非 assert），PYTHONOPTIMIZE=1
下照样红。先例=test_check_peephole_sanitizer.py（1217 判别力自测同哲学）。
"""
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parents[1]
失败们: list[str] = []


def 断言(名: str, 条件: bool) -> None:
    print(("[OK] " if 条件 else "[红] ") + 名, flush=True)
    if not 条件:
        失败们.append(名)


def _载入(相对: str, 名: str):
    spec = importlib.util.spec_from_file_location(名, str(仓库根 / 相对))
    模块 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(模块)
    return 模块


def main() -> int:
    # ① 云端清单两态（真函数·非 win 恒跑兜底）
    门禁 = _载入("scripts/ci_static_gates.py", "cn_csg_hooktest")
    lin = 门禁.静态门禁清单("cn", "linux-x86_64")
    win = 门禁.静态门禁清单("cn", "win-x64")
    断言("云端 linux 清单尾项=check_peephole_sanitizer（1486 恒跑兜底）",
         bool(lin) and lin[-1][0] == "check_peephole_sanitizer")
    断言("云端 win 清单不含窥孔 sanitizer（检查面仅 Linux/WSL）",
         all(名 != "check_peephole_sanitizer" for 名, _ in win))
    断言("两平台清单仅差 sanitizer 一项（防清单被顺手改动）",
         len(lin) == len(win) + 1
         and [名 for 名, _ in lin][:-1] == [名 for 名, _ in win])

    # ② gate_quick 挂载文本锚（导入即执行·按源码锁）
    源 = (仓库根 / "scripts" / "gate_quick.py").read_text(encoding="utf-8")
    断言("gate_quick 写集窥孔三前缀存在（实现/单测/检查器自身）",
         all(锚 in 源 for 锚 in ("src/cn_compiler/codegen/peephole",
                                 "tests/unit/codegen/test_peephole",
                                 "scripts/check_peephole_sanitizer")))
    断言("gate_quick rc==2 环境缺失黄字显式降级（缺失显形非静默）",
         "_窥孔rc == 2" in 源 and "环境缺失" in 源)
    断言("gate_quick CN_GATE_PEEPHOLE_FORCE 逃生门存在",
         "CN_GATE_PEEPHOLE_FORCE" in 源)
    断言("gate_quick win 侧经 wsl.exe 包装（/mnt 路径转换）",
         "wsl.exe" in 源 and "/mnt/%s%s" in 源)
    断言("gate_quick 窥孔步 900s 超时存在", "timeout=900" in 源)

    # ③ 检查器依赖解析锚（挂载生效前提）
    检 = (仓库根 / "scripts" / "check_peephole_sanitizer.py").read_text(
        encoding="utf-8")
    断言("检查器 gtest 两候选仍在（共享 third-party + cmake _deps）",
         "third-party" in 检 and "googletest-src" in 检)
    断言("检查器 win 原生 rc=2 边界仍在（os.name 分派）",
         'os.name == "nt"' in 检 and "return 2" in 检)

    if 失败们:
        print("== 窥孔挂载结构锁：%d 项红 ==" % len(失败们), flush=True)
        return 1
    print("== 窥孔挂载结构锁全绿 ==", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
