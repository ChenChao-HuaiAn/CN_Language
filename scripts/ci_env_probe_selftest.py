#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_env_probe 自测体（#1462·单一职责独立文件——ci_daemon_selftest 已 2000 行不再纳新）。

断言面索引：假环境全绿正态（探测器全命中+磁盘大+gtest 探针命中）｜PATH 剥夺模拟
反态（探测器全 None→缺 g++/cmake/make·不通过）｜gtest 单项缺失反态（本地兜底不存在
+探针不命中）｜本地源码兜底命中（临时树假 gtest-all.cc）｜磁盘下限解析三态（缺省/
合法/非法值显式失败不静默取默认）｜判定可领/缺失一行 正反两态（结构异常 fail-safe）｜
gtest系统探针默认 结构两态（真 cmake 跑命中/缺 cmake 显式说明——机器差异只锚结构
不锚命中态）｜探测环境 平台分派（win=vcvars+ninja·linux=g+++make）。
运行：python3 scripts/ci_env_probe_selftest.py → 全绿 rc=0。
"""
from __future__ import annotations

import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ci_env_probe   # noqa: E402

失败们: list = []


def 断言(条件, 说明: str) -> None:
    """两态断言（正文+FAIL 行·全绿静默——与 ci_daemon_selftest 断言同风格）。"""
    if 条件:
        print("  ok %s" % 说明, flush=True)
    else:
        失败们.append(说明)
        print("  FAIL %s" % 说明, flush=True)


def 假探测器(命中们) -> "object":
    """注入式探测器：命中们 里的命令名返回 /假/bin/<名>，其余 None（PATH 剥夺模拟）。"""
    def 探(名):
        if 名 in 命中们:
            return "/假/bin/" + 名
        return None
    return 探


def 桩执行器(命令, **_kwargs):
    """注入式执行器：rc=0+版本首行（配合假探测器·零真子进程）。"""
    from types import SimpleNamespace
    return SimpleNamespace(returncode=0, stdout="假工具 13.2.0（桩）", stderr="")


def 大磁盘(_路径):
    return (100 * 1024 ** 3, 1, 50 * 1024 ** 3)     # 50 GB 余


def 小磁盘(_路径):
    return (100 * 1024 ** 3, 98 * 1024 ** 3, 2 * 1024 ** 3)   # 2 GB 余


def 命中探针(_cmake):
    return True, "系统包 find_package(GTest) 命中"


def 落空探针(_cmake):
    return False, "系统包未检出（CMake Error GTEST_MISS）"


def _全绿正态() -> None:
    print("── 假环境全绿正态（linux 口径）", flush=True)
    详情 = ci_env_probe.探测环境(
        Path("/tmp/任意树"), False,
        探测器=假探测器({"g++", "cmake", "make"}), 执行器=桩执行器,
        磁盘函数=大磁盘, gtest探针=命中探针)
    断言(详情["通过"] is True, "全绿正态：通过=True")
    断言(详情["缺失"] == [], "全绿正态：缺失清单空")
    断言({条["项"] for 条 in 详情["项目"]} ==
         {"编译器", "cmake", "生成器", "gtest", "磁盘", "缓存"},
         "全绿正态：项目清单六项齐")
    缓存条 = next(条 for 条 in 详情["项目"] if 条["项"] == "缓存")
    断言(缓存条.get("可选") is True and 缓存条["通过"] is False,
         "缓存缺失=可选不通过但不入缺失清单")
    断言(ci_env_probe.判定可领(详情) is True, "判定可领：全绿=True")
    断言(ci_env_probe.缺失一行(详情) == "", "缺失一行：全绿=空串")


def _path剥夺反态() -> None:
    print("── PATH 剥夺模拟反态（探测器全 None·linux 口径）", flush=True)
    详情 = ci_env_probe.探测环境(
        Path("/tmp/任意树"), False,
        探测器=假探测器(set()),      # 全 None=裸环境无 g++/cmake/make
        执行器=桩执行器, 磁盘函数=大磁盘, gtest探针=落空探针)
    断言(详情["通过"] is False, "PATH 剥夺：通过=False")
    断言("编译器" in 详情["缺失"] and "cmake" in 详情["缺失"]
         and "生成器" in 详情["缺失"] and "gtest" in 详情["缺失"],
         "PATH 剥夺：缺失=编译器、cmake、生成器、gtest")
    断言(ci_env_probe.判定可领(详情) is False, "判定可领：反态=False（不领任务）")
    断言(ci_env_probe.缺失一行(详情) == "编译器、cmake、生成器、gtest",
         "缺失一行：反态=顿号摘要")


def _gtest单项反态与兜底() -> None:
    print("── gtest 单项：探针落空反态+本地兜底命中", flush=True)
    详情 = ci_env_probe.探测环境(
        Path("/tmp/任意树"), False,
        探测器=假探测器({"g++", "cmake", "make"}), 执行器=桩执行器,
        磁盘函数=大磁盘, gtest探针=落空探针)
    断言(详情["通过"] is False and 详情["缺失"] == ["gtest"],
         "gtest 落空：仅 gtest 入缺失（其余项绿）")
    with tempfile.TemporaryDirectory() as 临时:
        # 夹具复刻真实布局：仓库树距根两层（CMakeLists 的 ../../third-party=
        # 仓库根上两级）——树=根/部署/CN_Language_C·兜底=根/third-party（临时目录
        # 内闭合·不泄漏 /tmp）
        根 = Path(临时)
        树 = 根 / "部署" / "CN_Language_C"
        树.mkdir(parents=True)
        假兜底 = 根 / "third-party" / "unittest" / "googletest" / "src"
        假兜底.mkdir(parents=True)
        (假兜底 / "gtest-all.cc").write_text("// 假 gtest 聚合源", encoding="utf-8")
        详情2 = ci_env_probe.探测环境(
            树, False, 探测器=假探测器({"g++", "cmake", "make"}),
            执行器=桩执行器, 磁盘函数=大磁盘, gtest探针=落空探针)
        断言(详情2["通过"] is True, "本地兜底命中：探针落空仍通过（CMakeLists 同款兜底序）")
        g条 = next(条 for 条 in 详情2["项目"] if 条["项"] == "gtest")
        断言("third-party/unittest/googletest" in g条["说明"],
             "本地兜底命中：说明带命中路径")
    断言(ci_env_probe.gtest本地兜底(Path("/tmp/任意树")) == "",
         "gtest本地兜底：无兜底=空串（否定回答）")


def _磁盘下限三态() -> None:
    print("── 磁盘下限解析三态（缺省/合法/非法显式失败）", flush=True)
    缺省字节, 错误 = ci_env_probe.磁盘下限字节({})
    断言(错误 is None and 缺省字节 == 5 * 1024 ** 3, "磁盘下限：缺省=5GB")
    合法, 错误 = ci_env_probe.磁盘下限字节({"CN_ENV_DISK_MIN_GB": "3.5"})
    断言(错误 is None and 合法 == int(3.5 * 1024 ** 3), "磁盘下限：合法小数接受")
    条 = ci_env_probe.磁盘条目(Path("/tmp/任意树"), 大磁盘,
                             {"CN_ENV_DISK_MIN_GB": "abc"})
    断言(条["通过"] is False and "非法" in 条["说明"],
         "磁盘下限：非法值=磁盘项显式失败（不静默取默认）")
    条2 = ci_env_probe.磁盘条目(Path("/tmp/任意树"), 小磁盘, {})
    断言(条2["通过"] is False and "<" in 条2["说明"],
         "磁盘余量不足：显式失败带余/下限数字")
    详情 = ci_env_probe.探测环境(
        Path("/tmp/任意树"), False,
        探测器=假探测器({"g++", "cmake", "make"}), 执行器=桩执行器,
        磁盘函数=小磁盘, gtest探针=命中探针)
    断言(详情["通过"] is False and 详情["缺失"] == ["磁盘"],
         "磁盘不足：整轮不通过（写满盘防线）")


def _结构异常fail_safe() -> None:
    print("── 判定可领/缺失一行 结构异常 fail-safe", flush=True)
    断言(ci_env_probe.判定可领(None) is False, "判定可领：None=False")
    断言(ci_env_probe.判定可领("通过") is False, "判定可领：坏类型=False")
    断言(ci_env_probe.判定可领({}) is False, "判定可领：缺键=False")
    断言(ci_env_probe.缺失一行(None) == "（预检详情结构异常）",
         "缺失一行：结构异常=显式文案")
    断言(ci_env_probe.缺失一行({"通过": False, "缺失": []}) == "（缺失清单空但未通过·结构异常）",
         "缺失一行：未通过但清单空=显式文案（不冒充空缺失）")


def _探针结构两态() -> None:
    print("── gtest系统探针默认 结构两态（真 cmake·机器差异只锚结构）", flush=True)
    cmake = shutil.which("cmake")
    命中, 说明 = ci_env_probe.gtest系统探针默认(cmake)
    断言(isinstance(命中, bool) and isinstance(说明, str) and 说明,
         "探针真跑：返回 (bool, 非空说明) 结构")
    断言(命中 is True, "探针真跑：本机 find_package(GTest) 命中（L1 构建环境自证）")
    落空, 说明2 = ci_env_probe.gtest系统探针默认("")
    断言(落空 is False and "cmake 不在 PATH" in 说明2,
         "探针：cmake 缺失=显式说明不命中（不静默）")


def _平台分派() -> None:
    print("── 探测环境 平台分派（win=vcvars+ninja·linux=g+++make）", flush=True)
    详情win = ci_env_probe.探测环境(
        Path("C:/任意树"), True,
        探测器=假探测器({"cmake", "ninja"}), 执行器=桩执行器,
        磁盘函数=大磁盘, gtest探针=命中探针)
    编译器 = next(条 for 条 in 详情win["项目"] if 条["项"] == "编译器")
    生成器 = next(条 for 条 in 详情win["项目"] if 条["项"] == "生成器")
    断言(编译器["命令"] == "vcvars64.bat", "win 编译器=vcvars64.bat 探测")
    断言(生成器["命令"] == "ninja", "win 生成器=ninja（-G Ninja 面）")
    详情lin = ci_env_probe.探测环境(
        Path("/tmp/任意树"), False,
        探测器=假探测器({"g++", "cmake", "make"}), 执行器=桩执行器,
        磁盘函数=大磁盘, gtest探针=命中探针)
    生成器l = next(条 for 条 in 详情lin["项目"] if 条["项"] == "生成器")
    断言(生成器l["命令"] == "make", "linux 生成器=make（默认 Unix Makefiles 面）")
    详情win缺 = ci_env_probe.探测环境(
        Path("C:/任意树"), True,
        探测器=假探测器({"cmake"}),   # ninja 缺
        执行器=桩执行器, 磁盘函数=大磁盘, gtest探针=命中探针)
    断言(详情win缺["通过"] is False and "生成器" in 详情win缺["缺失"],
         "win 缺 ninja=不通过（-G Ninja 必需）")


def 主() -> int:
    print("== ci_env_probe 自测（#1462 假环境正反两态） ==", flush=True)
    _全绿正态()
    _path剥夺反态()
    _gtest单项反态与兜底()
    _磁盘下限三态()
    _结构异常fail_safe()
    _探针结构两态()
    _平台分派()
    if 失败们:
        print("== 自测红：%d 项失败" % len(失败们), flush=True)
        return 1
    print("== ci_env_probe 自测全绿 ==", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(主())
