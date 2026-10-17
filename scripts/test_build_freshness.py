#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_build_freshness.py —— check_build_freshness.py（872）自测：纯伪造布局·
os.utime 控制时刻·不真编译。场景：
  ① ninja 图全新鲜 → rc=0（含 $: 转义绝对路径解析回归——链接/库边不丢）；
  ② obj 旧于源（漏编/假新鲜主判据）→ rc=1 且名单指认；
  ③ 编译边 obj 缺失（构建图声明未落地）→ rc=1；
  ④ 游离 obj（不在当前构建图）→ rc=0+黄提示（不误红）；
  ⑤ 链接产物旧于输入（链接滞后）→ rc=1；
  ⑥ 核心产物早于 HEAD 提交时刻（rebase 后未重建）→ rc=1；
  ⑦ Make 布局模式（无 build.ninja）：obj 旧于源红 / 全新鲜绿；
  ⑧ Make 布局链接输入按 target 归属（1006）：cn 旧于无关 unit obj 不误红（#993
     场景：跨目标 obj 不是输入）/ 触 cn 真实输入 obj 仍红（不假绿）。
用法：python scripts/test_build_freshness.py   （rc=0=全绿）
"""
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
检测器 = 仓库根 / "scripts" / "check_build_freshness.py"
通过 = 0
时刻 = time.time()


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


def 触(路径: Path, 秒: float) -> None:
    os.utime(路径, (秒, 秒))


def 造git(根: Path) -> None:
    (根 / "src").mkdir(parents=True, exist_ok=True)
    for 命令 in (["git", "init", "-q"], ["git", "add", "-A"],
                 ["git", "-c", "user.email=t@t", "-c", "user.name=t",
                  "commit", "-q", "--allow-empty", "-m", "基"]):
        结 = subprocess.run(命令, cwd=str(根), capture_output=True, text=True)
        assert 结.returncode == 0, f"临时仓库初始化失败：{命令}\n{结.stderr}"


def ninja文(根: Path) -> str:
    """最小 ninja 图：2 编译边 + 1 库边 + 1 exe 边（绝对路径全走 C$: 转义·防回归）。"""
    def 转(p: Path) -> str:
        return p.as_posix().replace(":", "$:", 1)
    return f"""rule CXX_COMPILER__x_Debug
  command = cl /c
build CMakeFiles\\x.dir\\src\\a.cpp.obj: CXX_COMPILER__x_Debug ..\\..\\src\\a.cpp
build CMakeFiles\\x.dir\\src\\b.cpp.obj: CXX_COMPILER__x_Debug ..\\..\\src\\b.cpp
rule CXX_STATIC_LIBRARY_LINKER__x_Debug
  command = lib
build {转(根 / 'target' / 'x.lib')}: CXX_STATIC_LIBRARY_LINKER__x_Debug CMakeFiles\\x.dir\\src\\a.cpp.obj CMakeFiles\\x.dir\\src\\b.cpp.obj
rule CXX_EXECUTABLE_LINKER__p_Debug
  command = link
build {转(根 / 'target' / 'cn.exe')}: CXX_EXECUTABLE_LINKER__p_Debug CMakeFiles\\x.dir\\src\\a.cpp.obj CMakeFiles\\x.dir\\src\\b.cpp.obj | {转(根 / 'target' / 'x.lib')}
"""


def 造ninja树(基础: str) -> Path:
    根 = Path(tempfile.mkdtemp(prefix="bf_", dir=基础))
    造git(根)
    构建目录 = 根 / "target" / "build-ninja"
    (构建目录 / "CMakeFiles" / "x.dir" / "src").mkdir(parents=True)
    (构建目录 / "build.ninja").write_text(ninja文(根), encoding="utf-8")
    for 名 in ("a.cpp", "b.cpp"):
        (根 / "src" / 名).write_text(f"// {名}\n", encoding="utf-8")
    触(根 / "src" / "a.cpp", 时刻 - 300)
    触(根 / "src" / "b.cpp", 时刻 - 300)
    return 根


def 造obj们(根: Path, a秒圆=100, b秒圆=100, 库秒圆=80, exe秒圆=None) -> None:
    """图内 obj/lib/exe 全新鲜基线（各场景在其上做减法）。
    exe秒圆=None=保写入时刻（now·≥ HEAD 提交时刻——防 HEAD 判据混入）；负值=未来。"""
    编 = 根 / "target" / "build-ninja" / "CMakeFiles" / "x.dir" / "src"
    for 名, 秒 in (("a.cpp.obj", a秒圆), ("b.cpp.obj", b秒圆)):
        p = 编 / 名
        p.write_bytes(b"OBJ")
        触(p, 时刻 - 秒)
    库 = 根 / "target" / "x.lib"
    库.write_bytes(b"LIB")
    触(库, 时刻 - 库秒圆)
    exe = 根 / "target" / "cn.exe"
    exe.write_bytes(b"MZ")
    if exe秒圆 is not None:
        触(exe, 时刻 - exe秒圆)


def 跑核验(根: Path, 构建目录: Path | None = None):
    命令 = [sys.executable, str(检测器), "--项目根", str(根)]
    if 构建目录 is not None:
        命令 += ["--构建目录", str(构建目录)]
    r = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=120)
    return r.returncode, r.stdout + r.stderr


def 场景1_全新鲜(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根)
    码, 出 = 跑核验(根)
    断言(码 == 0, f"场景① ninja 全新鲜 rc=0（链接/库边 $: 转义解析不丢）\n{出}")
    断言("编译边 2" in 出 and "链接/库边 2" in 出, "场景① 边统计正确（2 编译+2 链接）")


def 场景2_obj旧于源(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根, a秒圆=500, b秒圆=100)          # a.obj 比源(now-300)旧
    码, 出 = 跑核验(根)
    断言(码 == 1 and "a.cpp.obj" in 出 and "旧于源" in 出,
         "场景② obj 旧于源→rc=1 名单指认（假新鲜主判据）")
    断言("b.cpp.obj 旧于源" not in 出, "场景② 新鲜 obj 不误报")


def 场景3_obj缺失(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根)
    (根 / "target" / "build-ninja" / "CMakeFiles" / "x.dir" / "src" / "b.cpp.obj").unlink()
    码, 出 = 跑核验(根)
    断言(码 == 1 and "未产出" in 出 and "b.cpp.obj" in 出, "场景③ obj 缺失→rc=1 未产出")


def 场景4_游离(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根)
    流浪 = 根 / "target" / "build-ninja" / "CMakeFiles" / "x.dir" / "旧残留.obj"
    流浪.write_bytes(b"OLD")
    触(流浪, 时刻 - 9999)
    码, 出 = 跑核验(根)
    断言(码 == 0, "场景④ 游离 obj→不红（健康树残留为常态）")
    断言("游离产物 1 个" in 出 and "旧残留.obj" in 出, "场景④ 黄提示名单指认")


def 场景5_链接滞后(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根, b秒圆=-50)                      # b.obj 未来时刻→新于 exe
    码, 出 = 跑核验(根)
    断言(码 == 1 and "链接滞后" in 出 and "cn.exe" in 出, "场景⑤ 链接滞后→rc=1")


def 场景6_HEAD判据(基础: str) -> None:
    根 = 造ninja树(基础)
    造obj们(根, a秒圆=1000, b秒圆=1000, 库秒圆=900)
    触(根 / "src" / "a.cpp", 时刻 - 2000)
    触(根 / "src" / "b.cpp", 时刻 - 2000)
    头 = int(subprocess.run(["git", "log", "-1", "--format=%ct"], cwd=str(根),
                           capture_output=True, text=True).stdout.strip())
    触(根 / "target" / "cn.exe", 头 - 600)      # rebase 后未重建形态
    码, 出 = 跑核验(根)
    断言(码 == 1 and "早于当前 HEAD 提交时刻" in 出, "场景⑥ exe 早于 HEAD→rc=1")
    名单行 = [l for l in 出.splitlines() if l.strip().startswith("·")]
    断言(len(名单行) == 1 and "HEAD 提交时刻" in 名单行[0],
         f"场景⑥ 红名单恰一项（无其它类红混入）：{名单行}")


def 场景7_Make布局(基础: str) -> None:
    根 = Path(tempfile.mkdtemp(prefix="bfm_", dir=基础))
    造git(根)
    (根 / "src").mkdir(exist_ok=True)
    源 = 根 / "src" / "a.cpp"
    源.write_text("// a\n", encoding="utf-8")
    构建目录 = 根 / "target" / "build-quick"
    obj目录 = 构建目录 / "CMakeFiles" / "x.dir" / "src"
    obj目录.mkdir(parents=True)
    (构建目录 / "CMakeCache.txt").write_text("# fake\n", encoding="utf-8")
    obj = obj目录 / "a.cpp.o"
    obj.write_bytes(b"O")
    产物 = 根 / "target" / "cn"
    产物.write_bytes(b"ELF")                    # mtime=now（≥ HEAD 时刻）
    触(源, 时刻 - 100)
    触(obj, 时刻 - 500)                         # obj 旧于源
    码, 出 = 跑核验(根, 构建目录)
    断言(码 == 1 and "旧于源" in 出 and "a.cpp.o" in 出, f"场景⑦ Make 布局 obj 旧于源→rc=1\n{出}")
    触(obj, 时刻 - 50)                          # 转全新鲜（cn 保写入时刻·≥ HEAD）
    码2, 出2 = 跑核验(根, 构建目录)
    断言(码2 == 0, f"场景⑦ Make 布局全新鲜→rc=0\n{出2}")


def 场景8_Make链接归属(基础: str) -> None:
    """1006 回归锚：Make 布局链接输入按 link.txt（-o 命中该产物）解析——cn 只对
    自身输入比较；cn_unit_tests 的 obj 新于 cn 不算 cn 陈旧（#993 误报本尊）。
    时刻设计：产物/无关 obj 取 now 之后（保 ≥ HEAD 提交时刻·防 HEAD 判据混入），
    cn < 无关 unit obj，但 cn > 自身全部输入（=误报场景的「本应绿」态）。"""
    现在 = time.time()      # 场景内现取（模块级时刻早于本场景 git commit·会触发 HEAD 判据）
    根 = Path(tempfile.mkdtemp(prefix="bfl_", dir=基础))
    造git(根)
    (根 / "src").mkdir(exist_ok=True)
    (根 / "tests/unit/ir").mkdir(parents=True)
    (根 / "src" / "a.cpp").write_text("// a\n", encoding="utf-8")
    (根 / "tests/unit/ir" / "test_x.cpp").write_text("// t\n", encoding="utf-8")
    构建目录 = 根 / "target" / "build-quick"
    obj目录 = 构建目录 / "CMakeFiles" / "cn.dir" / "src"
    unit目录 = 构建目录 / "tests/unit/CMakeFiles/cn_unit_tests.dir/ir"
    obj目录.mkdir(parents=True)
    unit目录.mkdir(parents=True)
    (构建目录 / "CMakeCache.txt").write_text("# fake\n", encoding="utf-8")
    (构建目录 / "CMakeFiles/cn.dir/link.txt").write_text(
        f"/usr/bin/c++ -g -o {根}/target/cn CMakeFiles/cn.dir/src/a.cpp.o {根}/target/libx.a\n",
        encoding="utf-8")
    (构建目录 / "tests/unit/CMakeFiles/cn_unit_tests.dir/link.txt").write_text(
        f"/usr/bin/c++ -g -o {根}/target/cn_unit_tests "
        f"CMakeFiles/cn_unit_tests.dir/ir/test_x.cpp.o {根}/target/libx.a\n", encoding="utf-8")
    obj = obj目录 / "a.cpp.o"
    obj.write_bytes(b"O")
    无关obj = unit目录 / "test_x.cpp.o"
    无关obj.write_bytes(b"O")
    库 = 根 / "target" / "libx.a"
    库.write_bytes(b"A")
    cn = 根 / "target" / "cn"
    cn.write_bytes(b"ELF")
    单测 = 根 / "target" / "cn_unit_tests"
    单测.write_bytes(b"ELF")
    触(根 / "src" / "a.cpp", 现在 - 300)
    触(obj, 现在 - 100)                         # cn 真实输入（早期）
    触(库, 现在 - 80)                           # 既有共同输入
    触(无关obj, 现在 + 2)                       # 晚于 cn——旧法据此误报链接滞后
    触(cn, 现在 + 1)                            # cn 新于自身全部输入（应绿）
    触(单测, 现在 + 4)                          # 新于自身全部输入（应绿）
    码, 出 = 跑核验(根, 构建目录)
    断言(码 == 0 and "链接滞后" not in 出,
         f"场景⑧a cn 旧于无关 unit obj 但新于自身输入→rc=0（#993 误报回归锚）\n{出}")
    触(obj, 现在 + 6)                           # 触 cn 真实输入（link.txt 取出的 obj）
    码2, 出2 = 跑核验(根, 构建目录)
    断言(码2 == 1 and "链接滞后" in 出2 and "a.cpp.o" in 出2,
         "场景⑧b cn 真实输入 obj 被触→仍红（防假绿·不放松）")
    断言("cn_unit_tests 旧于输入" not in 出2, "场景⑧b 归属过滤：unit 产物不因 cn 输入被牵连")
    触(obj, 现在 - 100)                         # 还原
    码3, 出3 = 跑核验(根, 构建目录)
    断言(码3 == 0, f"场景⑧c 还原后复绿\n{出3}")


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="cn_bf_") as 基础:
        print("== 场景① ninja 全新鲜（$: 转义回归） ==")
        场景1_全新鲜(基础)
        print("== 场景② obj 旧于源 ==")
        场景2_obj旧于源(基础)
        print("== 场景③ obj 缺失 ==")
        场景3_obj缺失(基础)
        print("== 场景④ 游离 obj 黄提示 ==")
        场景4_游离(基础)
        print("== 场景⑤ 链接滞后 ==")
        场景5_链接滞后(基础)
        print("== 场景⑥ HEAD 判据 ==")
        场景6_HEAD判据(基础)
        print("== 场景⑦ Make 布局模式 ==")
        场景7_Make布局(基础)
        print("== 场景⑧ Make 布局链接输入按 target 归属（1006） ==")
        场景8_Make链接归属(基础)
    print(f"\n[全部通过] {通过} 项断言")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as 异常:
        print(f"\n[自测失败] {异常}")
        raise SystemExit(1)
    except KeyboardInterrupt:
        print("\n[自测中断]")
        raise SystemExit(130)
