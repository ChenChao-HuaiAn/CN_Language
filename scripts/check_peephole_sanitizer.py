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

用法：
    python3 scripts/check_peephole_sanitizer.py [--cxx g++] [--verbose]
    （Windows 下经 WSL：wsl.exe -e bash -lc
      "cd /mnt/c/<repo> && python3 scripts/check_peephole_sanitizer.py"）

退出码：0=全绿；1=构建/测试红；2=环境缺失（非 Linux 或未找到编译器/gtest 源码）。
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
    结果 = 运行([str(可执行), f"--gtest_filter={测试过滤}"], 环境)
    print(结果.stdout, flush=True)
    if 结果.returncode != 0:
        print(f"[红] 单测失败（{模式}）rc={结果.returncode}", flush=True)
        print(结果.stderr, flush=True)
        return False
    汇总 = re.search(r"(\d+) tests? from \d+ test suites? ran", 结果.stdout)
    if not 汇总 or int(汇总.group(1)) == 0:
        print(f"[红] 未发现任何 Peephole 用例（{模式}）", flush=True)
        return False
    print(f"[绿] {模式}：{汇总.group(1)} 例通过", flush=True)
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
    print("全绿：ASan+UBSan 与 _GLIBCXX_DEBUG 两口径 Peephole 单测通过", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
