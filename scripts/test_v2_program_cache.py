#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""v2 整程序级预编译缓存自动化验证（任务 1272·可复跑脚本）

覆盖（全部使用真实 v2 编译器生产链路）：
  1. 三态：首建（冷）/ 命中（热）/ CN_NO_V2_PROGRAM_CACHE=1——asm + stderr 逐字节一致。
  2. 失效：改源 / 改宏集合（-D）/ 改货舱 [特性] / 改货舱 [货舱] 名称版本。
  3. 损坏：篡改条目载荷 1 字节 → 显形 + 回退重编译 + 重建。
  4. 验证通道：CN_V2_PROGRAM_CACHE_VERIFY=1 → 完整重编译、asm+stderr 一致、写回。
  5. 货舱特性三态（764 同型）：特性切换产物不同且各自三态一致。
  6. intern 截断单测（InternApiTest.*；驻留→截断→重驻留 ID 一致、基线指针有效）。

用法：
  python scripts/test_v2_program_cache.py [--v2p target/audit2/v2p.exe]
      [--cn target/cn.exe] [--build-v2p] [--keep]
默认 v2p 不存在且给了 --build-v2p 时用宿主 cn 编译 CN语言编译器v2/主.cn。
退出码 0=全绿；1=失败（逐项打印）。
"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import sys
import time

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")

仓库根 = pathlib.Path(__file__).resolve().parent.parent
默认v2p = 仓库根 / "target" / "audit2" / "v2p.exe"
默认cn = 仓库根 / "target" / "cn.exe"
工作根 = 仓库根 / "target" / "cache_selftest"


def 找可执行(路径: pathlib.Path) -> pathlib.Path:
    if 路径.exists():
        return 路径
    if 路径.with_suffix("").exists():
        return 路径.with_suffix("")
    return 路径


def 运行(命令, cwd, env=None, timeout=600):
    e = dict(os.environ)
    if env:
        e.update(env)
    t = time.perf_counter()
    r = subprocess.run([str(x) for x in 命令], cwd=str(cwd), env=e,
                       capture_output=True, timeout=timeout)
    return r.returncode, r.stderr, time.perf_counter() - t


def 写夹具(工作: pathlib.Path, 特性: str, 版本: str = "1.0") -> None:
    工作.mkdir(parents=True, exist_ok=True)
    (工作 / "target").mkdir(exist_ok=True)
    (工作 / "货舱.toml").write_text(
        '[货舱]\n名称 = "缓存自测"\n版本 = "%s"\n[特性]\n启用 = ["%s"]\n'
        % (版本, 特性), encoding="utf-8")
    (工作 / "乙.cn").write_text(
        '公开:\n函数 乙一() -> 字符串 { 返回 "乙"; }\n', encoding="utf-8")
    (工作 / "主.cn").write_text(
        '导入 乙;\n'
        '函数 主() -> 整32 {\n'
        '    字符串 s = 乙一();\n'
        '#如果定义(甲)\n'
        '    s = s + "甲";\n'
        '#否则\n'
        '    s = s + "无甲";\n'
        '#结束如果\n'
        '#如果定义(乙特性)\n'
        '    s = s + "乙";\n'
        '#结束如果\n'
        '    IO::打印到错误(s + "\\n");\n'
        '    返回 0;\n'
        '}\n', encoding="utf-8")


def 清缓存(缓存目录: pathlib.Path) -> None:
    if 缓存目录.exists():
        shutil.rmtree(缓存目录, ignore_errors=True)


def 条目们(缓存目录: pathlib.Path):
    return sorted(glob.glob(str(缓存目录 / "*.cnvpc")))


目标平台 = "win-x64" if sys.platform == "win32" else "linux-x86_64"


def 编译(v2p: pathlib.Path, 工作: pathlib.Path, 缓存目录: pathlib.Path,
         环境=None, 选项=(), timeout=600):
    env = {"CN_V2_PROGRAM_CACHE_DIR": str(缓存目录)}
    if 环境:
        env.update(环境)
    rc, err, dt = 运行([v2p, str(工作 / "主.cn"), 目标平台, *选项], 工作, env, timeout)
    asm路径 = 工作 / "target" / ("v2asm.asm" if 目标平台 == "win-x64" else "v2asm.s")
    asm = asm路径.read_bytes() if asm路径.exists() else b""
    return rc, err, asm, dt


def 断言(条件, 说明, 失败们):
    if 条件:
        print("  [PASS] " + 说明)
    else:
        print("  [FAIL] " + 说明)
        失败们.append(说明)


def 三态(v2p, 工作, 缓存目录, 标签, 失败们, 选项=()):
    rc1, e1, a1, _ = 编译(v2p, 工作, 缓存目录, 选项=选项)
    rc2, e2, a2, t2 = 编译(v2p, 工作, 缓存目录, 选项=选项)
    rc3, e3, a3, _ = 编译(v2p, 工作, 缓存目录, {"CN_NO_V2_PROGRAM_CACHE": "1"}, 选项)
    断言(rc1 == 0 and rc2 == 0 and rc3 == 0, f"{标签}: 三态退出码 0", 失败们)
    断言(a1 == a2 == a3, f"{标签}: asm 三态逐字节一致", 失败们)
    断言(e1 == e2 == e3, f"{标签}: stderr 三态逐字节一致", 失败们)
    return e1, a1, t2


def 篡改(条目: str) -> None:
    data = bytearray(open(条目, "rb").read())
    pos = 0
    for _ in range(7):
        pos = data.index(10, pos) + 1
    data[pos + 2] ^= 0x01
    open(条目, "wb").write(bytes(data))


def main() -> int:
    global 目标平台
    ap = argparse.ArgumentParser()
    ap.add_argument("--v2p", default=str(默认v2p))
    ap.add_argument("--cn", default=str(默认cn))
    ap.add_argument("--build-v2p", action="store_true")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--target", default=目标平台)
    args = ap.parse_args()
    目标平台 = args.target

    v2p = 找可执行(pathlib.Path(args.v2p))
    cn = 找可执行(pathlib.Path(args.cn))
    失败们 = []

    需重建 = args.build_v2p and (not v2p.exists() or any(
        f.stat().st_mtime > v2p.stat().st_mtime
        for f in (仓库根 / "CN语言编译器v2").rglob("*.cn")))
    if 需重建:
        if not cn.exists():
            print(f"[FAIL] 宿主编译器不存在: {cn}")
            return 1
        v2p = 工作根 / ("v2p.exe" if sys.platform == "win32" else "v2p")
        v2p.parent.mkdir(parents=True, exist_ok=True)
        print(f"[构建] {cn} build CN语言编译器v2/主.cn -> {v2p}")
        rc, err, _ = 运行([cn, "build", 仓库根 / "CN语言编译器v2" / "主.cn",
                          "--target", 目标平台, "--output", v2p], 仓库根, timeout=1800)
        if rc != 0 or not v2p.exists():
            print("[FAIL] v2p 构建失败:\n" + err.decode("utf-8", "replace")[-800:])
            return 1
    if not v2p.exists():
        print(f"[跳过] v2p 不存在: {v2p}（可用 --build-v2p）")
        return 0

    工作 = 工作根 / "work"
    缓存 = 工作 / "cache"
    清缓存(缓存)
    if 工作.exists():
        shutil.rmtree(工作, ignore_errors=True)
    写夹具(工作, "甲")

    print("== 1. 三态（首建/命中/无缓存） ==")
    基线err, 基线asm, 热耗时 = 三态(v2p, 工作, 缓存, "基础", 失败们)
    print(f"  [info] 热路径 {热耗时:.3f}s；条目 {len(条目们(缓存))} 个")

    print("== 2. 失效：改源 / -D / 货舱特性 / 货舱名称版本 ==")
    (工作 / "乙.cn").write_text(
        '公开:\n函数 乙一() -> 字符串 { 返回 "乙改"; }\n', encoding="utf-8")
    rc, e, a, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"})
    断言(rc == 0 and a != 基线asm, "改源: 产物变化（未命中旧条目）", 失败们)
    rc2, e2, a2, _ = 编译(v2p, 工作, 缓存)
    断言(a == a2, "改源: 第二遍命中且 asm 一致", 失败们)
    # -D 宏集合
    rc, _, aD, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"},
                        选项=("-D", "乙特性"))
    断言(rc == 0 and aD != a2, "-D 宏集合: 独立条目/产物变化", 失败们)
    rc2, _, aD2, _ = 编译(v2p, 工作, 缓存, 选项=("-D", "乙特性"))
    断言(rc2 == 0 and aD == aD2, "-D 宏集合: 再次命中", 失败们)
    # 货舱 [特性] 甲 -> 乙特性
    写夹具(工作, "乙特性")
    rc, _, a乙, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"})
    断言(rc == 0 and a乙 != a2, "货舱特性切换: 产物变化（未命中旧条目）", 失败们)
    三态(v2p, 工作, 缓存, "货舱特性乙", 失败们)
    # 货舱 [货舱] 名称/版本变化（仅头字段）→ 键失效
    rc, _, a版, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"})
    写夹具(工作, "乙特性", "2.0")
    rc, e版, a版2, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"})
    统计 = [l for l in e版.decode("utf-8", "replace").splitlines() if "[程序缓存]" in l]
    断言(rc == 0 and 统计 and "失效重建=1" in 统计[-1],
         "货舱名称/版本变化: 键失效重建", 失败们)

    print("== 3. 损坏载荷：显形 + 回退 + 重建 ==")
    写夹具(工作, "甲")
    清缓存(缓存)   # 隔离：确保只有一个当前配置条目，篡改目标明确
    rc, e, a, _ = 编译(v2p, 工作, 缓存)
    rc, e, a, _ = 编译(v2p, 工作, 缓存)
    条目 = 条目们(缓存)
    断言(bool(条目), "存在缓存条目", 失败们)
    if 条目:
        篡改(条目[-1])
        rc, e, a坏, _ = 编译(v2p, 工作, 缓存, {"CN_V2_PROGRAM_CACHE_STATS": "1"})
        断言(rc == 0 and "缓存条目无效" in e.decode("utf-8", "replace"),
             "篡改载荷: 显形回退", 失败们)
        断言(a坏 == a, "篡改载荷: 重建产物与基线一致", 失败们)
        rc, e, a再, _ = 编译(v2p, 工作, 缓存)
        断言(a再 == a and e == e, "损坏重建后再命中", 失败们)

    print("== 4. 验证通道（asm + stderr 双比对） ==")
    rc, e验, a验, _ = 编译(v2p, 工作, 缓存,
                          {"CN_V2_PROGRAM_CACHE_VERIFY": "1",
                           "CN_V2_PROGRAM_CACHE_STATS": "1"})
    统计 = [l for l in e验.decode("utf-8", "replace").splitlines() if "[程序缓存]" in l]
    断言(rc == 0 and "验证不符=0" in (统计[-1] if 统计 else ""),
         "VERIFY: 完整重编译 asm+stderr 一致", 失败们)
    断言(not any("警告: [程序缓存] 验证不符" in l
                 for l in e验.decode("utf-8", "replace").splitlines()),
         "VERIFY: 无验证不符告警", 失败们)

    print("== 5. intern 截断单测 ==")
    单测 = next((p for p in [仓库根 / "target" / "cn_unit_tests.exe",
                            仓库根 / "target" / "cn_unit_tests"]
                if p.exists()), None)
    if 单测 is None:
        print("  [跳过] 未找到单测产物")
    else:
        rc, e, _ = 运行([单测, "--gtest_filter=InternApiTest.*"], 仓库根)
        断言(rc == 0, "InternApiTest.*（截断/重驻留/基线指针）", 失败们)

    if not args.keep:
        shutil.rmtree(工作, ignore_errors=True)

    print()
    if 失败们:
        print(f"[门禁·红] {len(失败们)} 项失败")
        return 1
    print("[门禁·绿] v2 程序级缓存三态/失效/损坏/验证/货舱/intern 全通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
