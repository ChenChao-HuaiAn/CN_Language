# -*- coding: utf-8 -*-
"""窥孔内核 string_view 生命周期机械锁（任务 1217）。

在 Linux/WSL 以 g++/clang++ 两种口径编译并运行 Peephole* 单测（含边界/随机语料
与「就地改写后旧视图」ASan 死亡例）：
  ① asan+ubsan：-fsanitize=address,undefined -fno-sanitize-recover=all
     -D_GLIBCXX_ASSERTIONS（悬垂/越界/未定义行为，ASan 报告即红）；
  ② glibcxx-debug：-D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS
     （string_view/容器越界断言；所有 TU 同宏，ABI 一致）。
两条口径任一构建或测试失败=非零退出。每次运行先做 ASan 判别力自检
（故意 use-after-free 的负例必须红、正常读正例必须绿），防工具链未实际启用假绿。
被编译面=窥孔两实现 TU + 四个单测 TU + 系统/随仓 gtest；不依赖完整工程构建。

防假绿三重机械断言（审查 P2-1）：
  1. 分口径精确计数：总数/通过/跳过逐项与「期望口径」比对——只断言「用例数>0」
     挡不住筛选名失效或尾部用例成批消失（例如死亡例被 SKIP 分支顶替）；
  2. 跳过零容忍：ASan 口径出现任何 [  SKIPPED ] 即红（死亡例消失的典型形态）；
     _GLIBCXX_DEBUG 口径只允许恰好 1 条并核对用例名；
  3. --gtest_list_tests 存在性：ASan 口径必须列出两条 UAF 死亡例
     （OldViewAfterGasInPlaceIsAsanRed / OldViewAfterArm64IndirectInPlaceIsAsanRed），
     _GLIBCXX_DEBUG 口径必须不列出它们——CN_PEEPHOLE_ASAN_DEATH 判别宏失效即红。
计数锚与新增用例联动：本文件「期望口径」是刻意硬编码的哨兵，新增/删除 Peephole
  用例时必须同步改这里（改不动=测试面已被静默改变，先查明原因）。
判别力自测=scripts/test_check_peephole_sanitizer.py（假输出正反例，不需编译器）。

用法：
    python3 scripts/check_peephole_sanitizer.py [--cxx g++] [--verbose]
    （Windows 下经 WSL：wsl.exe -e bash -lc
      "cd /mnt/c/<repo> && python3 scripts/check_peephole_sanitizer.py"）

退出码：0=全绿；1=构建/测试/计数/用例清单红；2=环境缺失（非 Linux 或未找到
编译器/gtest 源码）。
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

仓库根 = Path(__file__).resolve().parents[1]
构建根 = 仓库根 / "target" / "peephole-sanitizer"
测试过滤 = "Peephole*"

# 分口径精确计数锚（2026-10-23 审查 P2-1 实测；新增用例须同步更新并写进提交说明）：
#   asan+ubsan   = 全部用例（含两条 EXPECT_DEATH 死亡例）
#   glibcxx-debug= 全部用例 - 2 死亡例 + 1 条 ASan 边界显式 SKIP 例
期望口径 = {
    "asan+ubsan": {"总数": 147, "通过": 147, "跳过": 0},
    "glibcxx-debug": {"总数": 146, "通过": 145, "跳过": 1},
}
# 死亡例在 ASan 口径必须实际存在（宏失效时会被 SKIP 分支顶替而不报警）
死亡例名 = (
    "PeepholeSlotLifetime.OldViewAfterGasInPlaceIsAsanRed",
    "PeepholeSlotLifetime.OldViewAfterArm64IndirectInPlaceIsAsanRed",
)
# 非 ASan 口径唯一允许的跳过例（显式诚实边界，指向本脚本）
跳过例名 = "PeepholeSlotLifetime.OldViewAfterInPlaceDetectableOnlyUnderAsan"


def 找gtest源码() -> Path | None:
    """与 CMakeLists 同口径：随仓 third-party 优先，其次本树 target 拉取目录。"""
    候选 = [
        仓库根.parent.parent / "third-party" / "unittest" / "googletest",
        仓库根 / "target" / "build" / "_deps" / "googletest-src" / "googletest",
    ]
    for 目录 in 候选:
        if (目录 / "src" / "gtest-all.cc").exists():
            return 目录
    return None


def 源文件清单() -> list[Path]:
    测试 = sorted((仓库根 / "tests" / "unit" / "codegen").glob("test_peephole_slot*.cpp"))
    实现 = [
        仓库根 / "src" / "cn_compiler" / "codegen" / "peephole_slot.cpp",
        仓库根 / "src" / "cn_compiler" / "codegen" / "peephole_slot_arm64.cpp",
    ]
    return 测试 + 实现


def 运行(命令: list[str], 环境: dict | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(命令, capture_output=True, text=True, env=环境)


def 解析汇总(输出: str) -> dict:
    """从 gtest 运行输出提取 总数/通过/跳过（缺失= None；跳过行缺失视为 0）。"""
    def 取(模式: str) -> int | None:
        命中 = re.search(模式, 输出)
        return int(命中.group(1)) if 命中 else None

    return {
        "总数": 取(r"(\d+) tests? from \d+ test suites? ran"),
        "通过": 取(r"\[  PASSED  \] (\d+) tests?"),
        "跳过": 取(r"\[  SKIPPED \] (\d+) tests?") or 0,
    }


def 跳过用例名(输出: str) -> list[str]:
    return re.findall(r"^\[  SKIPPED \] (Peephole[A-Za-z0-9_.]+)$", 输出, re.MULTILINE)


def 核对汇总(模式: str, 输出: str) -> list[str]:
    """分口径计数断言：任一数字不符/出现 FAILED/跳过面不符=返回错误清单。"""
    期望 = 期望口径[模式]
    实际 = 解析汇总(输出)
    错: list[str] = []
    for 键 in ("总数", "通过"):
        if 实际[键] is None:
            错.append(f"{模式}：输出缺少「{键}」汇总行（gtest 未正常跑完？）")
        elif 实际[键] != 期望[键]:
            错.append(f"{模式}：{键} {实际[键]} != 期望 {期望[键]}")
    if "[  FAILED  ]" in 输出:
        错.append(f"{模式}：出现 [  FAILED  ] 行")
    if 实际["跳过"] != 期望["跳过"]:
        错.append(f"{模式}：跳过 {实际['跳过']} != 期望 {期望['跳过']}")
    if 模式 == "asan+ubsan":
        # ASan 口径死亡例必须真正执行：任何 SKIP（含 [  SKIPPED ] 字面）即红
        if "[  SKIPPED ]" in 输出 or 实际["跳过"] != 0:
            错.append("asan+ubsan：出现 SKIPPED（死亡例被 SKIP 分支顶替？）")
    else:
        名单 = 跳过用例名(输出)
        if 名单 != [跳过例名]:
            错.append(f"{模式}：跳过用例清单 {名单} != [{跳过例名}]")
    return 错


def 解析用例清单(输出: str) -> list[str]:
    """解析 --gtest_list_tests：套件行无缩进以 '.' 结尾，用例行缩进。"""
    清单: list[str] = []
    当前套件 = ""
    for 行 in 输出.splitlines():
        if not 行.strip() or 行.startswith("Running main()"):
            continue
        if not 行.startswith(" "):
            当前套件 = 行.strip()
            if 当前套件.endswith("."):
                当前套件 = 当前套件[:-1]
            continue
        if not 当前套件:
            continue
        清单.append(当前套件 + "." + 行.strip().split(" ")[0])
    return 清单


def 核对用例清单(模式: str, 输出: str) -> list[str]:
    """--gtest_list_tests 存在性断言：死亡例/跳过例必须落在正确口径。"""
    清单 = 解析用例清单(输出)
    集合 = set(清单)
    期望 = 期望口径[模式]
    错: list[str] = []
    if len(清单) != 期望["总数"]:
        错.append(f"{模式}：清单用例数 {len(清单)} != 期望 {期望['总数']}")
    if 模式 == "asan+ubsan":
        for 名 in 死亡例名:
            if 名 not in 集合:
                错.append(f"asan+ubsan：死亡例缺失 {名}"
                         "（CN_PEEPHOLE_ASAN_DEATH 判别宏失效？）")
        if 跳过例名 in 集合:
            错.append(f"asan+ubsan：清单出现非 ASan 分支用例 {跳过例名}")
    else:
        for 名 in 死亡例名:
            if 名 in 集合:
                错.append(f"{模式}：清单出现 ASan 死亡例 {名}"
                         "（CN_PEEPHOLE_ASAN_DEATH 判别宏未按口径生效）")
        if 跳过例名 not in 集合:
            错.append(f"{模式}：缺少 ASan 边界显式 SKIP 例 {跳过例名}")
    return 错


def 编译并运行(模式: str, 编译旗标: list[str], 链接旗标: list[str], cxx: str,
               gtest: Path, 详细: bool) -> bool:
    目录 = 构建根 / 模式
    if 目录.exists():
        shutil.rmtree(目录)
    目录.mkdir(parents=True)
    公共 = ["-std=c++17", "-O1", "-g", "-finput-charset=UTF-8",
            "-I", str(仓库根 / "src"),
            "-I", str(gtest / "include"),
            "-I", str(gtest),
            "-DGTEST_NO_LLVM_SUPPORT"]
    目标文件: list[Path] = []

    def 编一个(源: Path, 严格: bool) -> Path | None:
        目标 = 目录 / (源.stem + ".o")
        命令 = [cxx] + 公共 + 编译旗标 + (["-Wall", "-Wextra", "-Werror"] if 严格 else ["-w"])
        命令 += ["-c", str(源), "-o", str(目标)]
        if 详细:
            print("  " + " ".join(命令[-3:]), flush=True)
        结果 = 运行(命令)
        if 结果.returncode != 0:
            print(f"[红] 编译失败：{源}", flush=True)
            print(结果.stderr, flush=True)
            return None
        return 目标

    # gtest 自身按 CMake 口径静默编译（-w），被测源码/单测严格 -Werror
    for 源 in [gtest / "src" / "gtest-all.cc", gtest / "src" / "gtest_main.cc"]:
        目标 = 编一个(源, 严格=False)
        if 目标 is None:
            return False
        目标文件.append(目标)
    for 源 in 源文件清单():
        目标 = 编一个(源, 严格=True)
        if 目标 is None:
            return False
        目标文件.append(目标)

    可执行 = 目录 / "peephole_tests"
    链接 = [cxx] + 编译旗标 + 链接旗标 + [str(x) for x in 目标文件] + ["-o", str(可执行)]
    if os.name != "nt":
        链接.append("-pthread")
    结果 = 运行(链接)
    if 结果.returncode != 0:
        print(f"[红] 链接失败（{模式}）", flush=True)
        print(结果.stderr, flush=True)
        return False

    环境 = dict(os.environ)
    if "sanitize" in 模式:
        环境["ASAN_OPTIONS"] = "detect_leaks=1:abort_on_error=1:strict_string_checks=1"
        环境["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"

    # 存在性断言先行：死亡例/跳过例必须按口径实际参与构建
    清单结果 = 运行([str(可执行), f"--gtest_filter={测试过滤}", "--gtest_list_tests"], 环境)
    if 清单结果.returncode != 0:
        print(f"[红] --gtest_list_tests 失败（{模式}）rc={清单结果.returncode}", flush=True)
        print(清单结果.stderr, flush=True)
        return False
    清单错 = 核对用例清单(模式, 清单结果.stdout)
    if 清单错:
        for 条 in 清单错:
            print(f"[红] {条}", flush=True)
        return False

    结果 = 运行([str(可执行), f"--gtest_filter={测试过滤}"], 环境)
    print(结果.stdout, flush=True)
    汇总错 = 核对汇总(模式, 结果.stdout)
    if 汇总错:
        for 条 in 汇总错:
            print(f"[红] {条}", flush=True)
        print(f"[红] 单测口径不符（{模式}）rc={结果.returncode}", flush=True)
        print(结果.stderr, flush=True)
        return False
    if 结果.returncode != 0:
        print(f"[红] 单测失败（{模式}）rc={结果.returncode}", flush=True)
        print(结果.stderr, flush=True)
        return False
    实际 = 解析汇总(结果.stdout)
    print(f"[绿] {模式}：{实际['通过']} 例 PASSED，{实际['跳过']} 例 SKIPPED，"
          f"共 {实际['总数']} 例", flush=True)
    return True


def asan自检(cxx: str) -> bool:
    """ASan 工具链判别力自检（负例必红/正例必绿）：防「sanitizer 未实际启用」的假绿。"""
    目录 = 构建根 / "selftest"
    目录.mkdir(parents=True, exist_ok=True)
    正源 = 目录 / "ok.cpp"
    负源 = 目录 / "uaf.cpp"
    正源.write_text("#include <string>\nint main() {\n"
                    "  std::string s(64, 'x');\n"
                    "  volatile char c = s[63];\n"
                    "  return c == 'x' ? 0 : 1;\n}\n", encoding="utf-8")
    负源.write_text("#include <cstddef>\nint main() {\n"
                    "  char* p = new char[64];\n"
                    "  p[0] = 'x';\n"
                    "  delete[] p;\n"
                    "  volatile char c = p[0];  // 故意 use-after-free\n"
                    "  return c - 'x';\n}\n", encoding="utf-8")
    旗标 = ["-std=c++17", "-O1", "-g", "-fsanitize=address,undefined",
            "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    环境 = dict(os.environ)
    环境["ASAN_OPTIONS"] = "detect_leaks=0:abort_on_error=1"
    环境["UBSAN_OPTIONS"] = "halt_on_error=1"
    for 源, 名称, 期望红 in ((正源, "正例", False), (负源, "负例", True)):
        可执行 = 目录 / (源.stem + ".bin")
        编 = 运行([cxx] + 旗标 + [str(源), "-o", str(可执行)])
        if 编.returncode != 0:
            print(f"[红] ASan 自检编译失败（{名称}）", flush=True)
            print(编.stderr, flush=True)
            return False
        跑 = 运行([str(可执行)], 环境)
        红 = 跑.returncode != 0
        命中 = "heap-use-after-free" in (跑.stdout + 跑.stderr)
        if 红 != 期望红 or (期望红 and not 命中):
            print(f"[红] ASan 自检失明（{名称} rc={跑.returncode}）", flush=True)
            return False
    print("[自检] ASan 判别力成立：悬挂读必红（heap-use-after-free）·正常读绿", flush=True)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="窥孔 string_view 生命周期 sanitizer 机械锁")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--verbose", action="store_true")
    参数 = parser.parse_args()

    if os.name == "nt":
        print("[边界] 本检查面仅 Linux/WSL（ASan+UBSan 与 _GLIBCXX_DEBUG 口径）。",
              flush=True)
        print("       Windows 请经 WSL 运行：wsl.exe -e bash -lc "
              "\"cd /mnt/c/<repo> && python3 scripts/check_peephole_sanitizer.py\"", flush=True)
        return 2
    cxx = shutil.which(参数.cxx)
    if cxx is None:
        print(f"[边界] 未找到编译器 {参数.cxx}（需 g++ 或 clang++）", flush=True)
        return 2
    gtest = 找gtest源码()
    if gtest is None:
        print("[边界] 未找到 gtest 源码（third-party/unittest/googletest）", flush=True)
        return 2
    if not asan自检(cxx):
        return 1

    口径 = [
        ("asan+ubsan",
         ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
          "-fno-omit-frame-pointer", "-D_GLIBCXX_ASSERTIONS"],
         ["-fsanitize=address,undefined"]),
        ("glibcxx-debug",
         ["-D_GLIBCXX_DEBUG", "-D_GLIBCXX_ASSERTIONS"],
         []),
    ]
    for 模式, 编译旗标, 链接旗标 in 口径:
        print(f"== {模式}：g++ 编译 Peephole 单测 ==", flush=True)
        if not 编译并运行(模式, 编译旗标, 链接旗标, cxx, gtest, 参数.verbose):
            return 1
    print("全绿：ASan+UBSan 与 _GLIBCXX_DEBUG 两口径 Peephole 单测通过"
          "（精确计数 + 死亡例存在性 + 跳过零容忍）", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
