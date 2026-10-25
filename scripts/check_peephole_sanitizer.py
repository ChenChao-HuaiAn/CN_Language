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

防假绿机械断言（审查 P2-1/P2-2/P2-3）：
  1. 源码推导差集：正则提取测试源码的 TEST 集合 S，与 --gtest_list_tests 集合 L
     比对——ASan 面 S-L 恰好={SKIP 例}、debug 面 S-L 恰好={两条死亡例}，L-S 空；
     用例静默消失立即显形，新增/删除用例无需手工改计数锚；
  2. 关键名双向检查：ASan 清单必须含两条 UAF 死亡例（OldViewAfterGasInPlaceIsAsanRed /
     OldViewAfterArm64IndirectInPlaceIsAsanRed），debug 清单不得含且必须含 SKIP 例
     ——CN_PEEPHOLE_ASAN_DEATH 宏失效或同数顶替即红；
  3. 汇总自洽：通过+跳过==总数、运行总数==清单数、FAILED 行零容忍；
  4. 跳过零容忍：ASan 口径出现任何 SKIPPED 即红；debug 口径恰好 1 条并核对用例名。
判别力自测=scripts/test_check_peephole_sanitizer.py（假输出正反例，不需编译器；
  显式失败分支，PYTHONOPTIMIZE=1/-O 下仍真实执行）。

用法：
    python3 scripts/check_peephole_sanitizer.py [--cxx g++] [--verbose]
    （Windows 下经 WSL：wsl.exe -e bash -lc
      "cd /mnt/c/<repo> && python3 scripts/check_peephole_sanitizer.py"）

退出码：0=全绿；1=构建/测试/计数/用例清单红；2=环境缺失（非 Linux 或未找到
编译器/gtest 源码）。
"""
from __future__ import annotations

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

# 口径锚（2026-10-23 复审 P2-2/P2-3：源码推导 + 关键名 + 自洽，不硬编码总数）：
#   源码 TEST 集合 S 由 tests/unit/codegen/test_peephole_slot*.cpp 正则提取（含
#   #if/#else 两支：2 条 ASan 死亡例 + 1 条非 ASan 显式 SKIP 例）；
#   ASan 口径清单 L 必须 = S - {跳过例名}；_GLIBCXX_DEBUG 口径 L 必须 = S - 死亡例名集。
#   运行汇总只做自洽（通过+跳过==总数、总数==清单数），用例消失/同数顶替由差集
#   与关键名断言抓；新增/删除用例不再需要手工改计数锚。
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
        仓库根 / "src" / "cn_compiler" / "codegen" / "peephole_slot_forward.cpp",
        仓库根 / "src" / "cn_compiler" / "codegen" / "peephole_slot_arm64_indirect.cpp",
    ]
    return 测试 + 实现


def 源码用例集() -> set[str]:
    """正则提取全部测试源文件的 TEST(套件, 用例)（与预处理分支无关：两支都算）。"""
    模式 = re.compile(r"\bTEST\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)\s*\)")
    用例: set[str] = set()
    for 源 in sorted((仓库根 / "tests" / "unit" / "codegen").glob("test_peephole_slot*.cpp")):
        文本 = 源.read_text(encoding="utf-8", errors="replace")
        用例.update(f"{命中.group(1)}.{命中.group(2)}" for 命中 in 模式.finditer(文本))
    return 用例


def 运行(命令: list[str], 环境: dict | None = None) -> subprocess.CompletedProcess:
    # 复审 P3-⑤：统一超时，防构建/链接/单测卡死拖住门禁（600s 远大于实测 1 分钟）
    return subprocess.run(命令, capture_output=True, text=True, env=环境, timeout=600)


def 解析汇总(输出: str) -> dict:
    """从 gtest 运行输出提取 总数/通过/跳过（缺失= None；跳过行缺失视为 0）。

    兼容 gtest 1.8/1.9 的 test cases 旧称与 suites 新称（复审 P3-①）。
    """
    def 取(模式: str) -> int | None:
        命中 = re.search(模式, 输出)
        return int(命中.group(1)) if 命中 else None

    return {
        "总数": 取(r"(\d+) tests? from \d+ test (?:suites?|cases?) ran"),
        "通过": 取(r"\[  PASSED  \] (\d+) tests?"),
        "跳过": 取(r"\[  SKIPPED \] (\d+) tests?") or 0,
    }


def 跳过用例名(输出: str) -> list[str]:
    # 参数化套件名形如 Suite/Param（复审 P3-②：允许 '/'）
    return re.findall(r"^\[  SKIPPED \] (Peephole[A-Za-z0-9_./]+)$", 输出, re.MULTILINE)


def 核对汇总(模式: str, 输出: str, 清单数: int | None = None) -> list[str]:
    """运行汇总自洽断言（复审 P2-2/P2-3）：不自洽/清单数不符/跳过面不符/FAILED=红。

    不硬编码总数：用例损失由 核对用例清单 的源码差集显形；这里只要求 gtest 自报
    数字内部自洽（通过+跳过==总数）且与已核清单数一致。
    """
    实际 = 解析汇总(输出)
    错: list[str] = []
    for 键 in ("总数", "通过"):
        if 实际[键] is None:
            错.append(f"{模式}：输出缺少「{键}」汇总行（gtest 未正常跑完？）")
    if 实际["总数"] is not None and 实际["通过"] is not None:
        if 实际["通过"] + 实际["跳过"] != 实际["总数"]:
            错.append(f"{模式}：汇总不自洽 通过{实际['通过']}"
                     f"+跳过{实际['跳过']} != 总数{实际['总数']}")
        if 清单数 is not None and 实际["总数"] != 清单数:
            错.append(f"{模式}：运行总数 {实际['总数']} != 清单用例数 {清单数}")
    if "[  FAILED  ]" in 输出:
        错.append(f"{模式}：出现 [  FAILED  ] 行")
    if 模式 == "asan+ubsan":
        # ASan 口径死亡例必须真正执行：任何 SKIP（含 SKIPPED 字面）即红（复审 P3-①）
        if "SKIPPED" in 输出 or 实际["跳过"] != 0:
            错.append("asan+ubsan：出现 SKIPPED（死亡例被 SKIP 分支顶替？）")
    else:
        名单 = 跳过用例名(输出)
        if 实际["跳过"] != 1 or 名单 != [跳过例名]:
            错.append(f"{模式}：跳过 {实际['跳过']} 例 {名单}，"
                     f"应恰好 1 例 [{跳过例名}]")
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


def 核对用例清单(模式: str, 输出: str, 源码集: set[str] | None = None) -> list[str]:
    """源码 TEST 集合 S 与 --gtest_list_tests 集合 L 的差集断言（复审 P2-2/P2-3）。

    ASan：S-L 必须恰好={跳过例名}、L-S 必须为空（死亡例两条实际参与构建）；
    debug：S-L 必须恰好=死亡例名集、L-S 必须为空（死亡例被 #else 排除）。
    另保留关键名双向检查：总数不变但死亡例被普通用例同数顶替时，差集为空而
    关键名缺失——必须红。源码集参数仅供自测注入合成源集合。
    """
    if 源码集 is None:
        源码集 = 源码用例集()
    集合 = set(解析用例清单(输出))
    错: list[str] = []
    if not 源码集:
        错.append(f"{模式}：未能从测试源码提取任何 TEST（正则/路径失效？）")
        return 错
    if 模式 == "asan+ubsan":
        期望遗漏 = {跳过例名}
        应含 = 死亡例名
        不应含 = (跳过例名,)
    else:
        期望遗漏 = set(死亡例名)
        应含 = (跳过例名,)
        不应含 = 死亡例名
    遗漏 = 源码集 - 集合
    多余 = 集合 - 源码集
    if 遗漏 != 期望遗漏:
        错.append(f"{模式}：源码有而清单无 {sorted(遗漏)} != 期望 {sorted(期望遗漏)}")
    if 多余:
        错.append(f"{模式}：清单有而源码无 {sorted(多余)}")
    for 名 in 应含:
        if 名 not in 集合:
            错.append(f"{模式}：关键用例缺失 {名}（判别宏失效或被同数顶替？）")
    for 名 in 不应含:
        if 名 in 集合:
            错.append(f"{模式}：清单不应含 {名}（口径分支未按宏生效）")
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
    清单数 = len(解析用例清单(清单结果.stdout))

    结果 = 运行([str(可执行), f"--gtest_filter={测试过滤}"], 环境)
    print(结果.stdout, flush=True)
    汇总错 = 核对汇总(模式, 结果.stdout, 清单数)
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
          "（源码差集 + 关键名 + 汇总自洽 + 跳过零容忍）", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
