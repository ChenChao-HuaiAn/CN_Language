#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_exe_freshness.py —— 宿主 exe 新鲜度核验（795）自测：纯 python·自跑自断言·exit 0/1。

零触碰真实构建：临时目录 git init 伪造项目根+os.utime 控制时刻+假构建通道注入
（不真编译）。七场景：
  ① 新鲜 exe 过（exe mtime ≥ HEAD 提交时刻且输入面无更新）；
  ② 陈旧拦·判据 a（exe mtime < HEAD 提交时刻——598 rebase 假红实锤面）；
  ③ 陈旧拦·判据 b（src 源比 exe 新——#175 工作区假绿实锤面）；
  ④ HEAD 不可知保守拦（非 git 目录·fail-safe）；
  ⑤ --cn 显式指定跳过核验（陈旧也放行·用户自担）；
  ⑥ 陈旧→自动重建→复验过（假构建通道注入验证主循环）+重建失败显式红指路 --cn；
  ⑦ 冷启动兜底（无候选→构建通道→重探测）+ 探测最新候选 556 语义保持。
用法：python scripts/test_exe_freshness.py   （rc=0=全绿）
"""
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(仓库根 / "tests" / "e2e"))
import exe新鲜度  # noqa: E402

通过 = 0


def 断言(条件, 说明):
    global 通过
    assert 条件, "自测失败：" + 说明
    通过 += 1
    print("  ✓ %s" % 说明)


def 临时git仓库(基础目录: str) -> Path:
    """git init+commit 的隔离项目根（HEAD 时刻≈now·src/ 预置一个旧源文件）。"""
    根 = Path(tempfile.mkdtemp(prefix="fx_", dir=基础目录))
    (根 / "src").mkdir()
    (根 / "src" / "旧源.cpp").write_text("// 旧源\n", encoding="utf-8")
    旧 = time.time() - 3600
    os.utime(根 / "src" / "旧源.cpp", (旧, 旧))
    for 命令 in (["git", "init", "-q"], ["git", "add", "-A"],
                 ["git", "-c", "user.email=t@t", "-c", "user.name=t",
                  "commit", "-q", "-m", "基"]):
        结果 = subprocess.run(命令, cwd=str(根), capture_output=True, text=True)
        assert 结果.returncode == 0, f"临时仓库初始化失败: {命令}\n{结果.stderr}"
    return 根


def 造exe(根: Path, 名: str, 时刻: float) -> Path:
    """伪造编译器产物（候选相对路径 target/cn.exe 布局·时刻由 os.utime 控制）。"""
    exe = 根 / "target" / 名
    exe.parent.mkdir(parents=True, exist_ok=True)
    exe.write_bytes(b"MZ-fake")
    os.utime(exe, (时刻, 时刻))
    return exe


def 候选们():
    return [Path("target") / "cn.exe"]


def 场景1_新鲜过(基础: str) -> None:
    根 = 临时git仓库(基础)
    头 = exe新鲜度.取HEAD提交时刻(根)
    assert 头 is not None
    exe = 造exe(根, "cn.exe", 头 + 60)   # 构建于 HEAD 提交 1 分钟后
    新鲜, 缘由 = exe新鲜度.核验新鲜度(exe, 根)
    断言(新鲜, "场景① 构建晚于 HEAD 提交→新鲜")
    就绪, 错 = exe新鲜度.确保编译器就绪("", 候选们(), 根, None, 600)
    断言(就绪 == exe.resolve() and 错 == "", "场景① 确保编译器就绪直通（不触发重建）")


def 场景2_判据a_晚于HEAD(基础: str) -> None:
    根 = 临时git仓库(基础)
    头 = exe新鲜度.取HEAD提交时刻(根)
    exe = 造exe(根, "cn.exe", 头 - 600)   # 构建于 HEAD 提交 10 分钟前（598 形态）
    新鲜, 缘由 = exe新鲜度.核验新鲜度(exe, 根)
    断言(not 新鲜, "场景② 构建早于 HEAD 提交→陈旧拦")
    断言("早于当前 HEAD 提交时刻" in 缘由 and "rebase" in 缘由,
         f"场景② 缘由指认 rebase 面（{缘由[:60]}…）")
    就绪, 错 = exe新鲜度.确保编译器就绪(
        "", 候选们(), 根,
        lambda *_a, **_k: type("R", (), {"returncode": 1, "stdout": "", "stderr": "boom"})(),
        600)
    断言(就绪 is None and "--cn" in 错, "场景② 拦后重建失败→显式红并指路 --cn")


def 场景3_判据b_源比exe新(基础: str) -> None:
    根 = 临时git仓库(基础)
    头 = exe新鲜度.取HEAD提交时刻(根)
    exe = 造exe(根, "cn.exe", 头 + 1)   # 构建于提交后（判据 a 过）·但早于即将的源修改
    time.sleep(1.1)
    (根 / "src" / "新源.cpp").write_text("// 未提交修改\n", encoding="utf-8")
    新鲜, 缘由 = exe新鲜度.核验新鲜度(exe, 根)
    断言(not 新鲜, "场景③ src 源新于 exe→陈旧拦（判据 b）")
    断言("未编入" in 缘由 and "新源.cpp" in 缘由,
         f"场景③ 缘由指认未提交修改与代表文件（{缘由[:60]}…）")


def 场景4_HEAD不可知保守拦(基础: str) -> None:
    根 = Path(tempfile.mkdtemp(prefix="fxnogit_", dir=基础))
    (根 / "target").mkdir()
    exe = 根 / "target" / "cn.exe"
    exe.write_bytes(b"MZ-fake")   # mtime=now（时刻本身没问题）
    新鲜, 缘由 = exe新鲜度.核验新鲜度(exe, 根)
    断言(not 新鲜, "场景④ 非 git 目录 HEAD 不可知→保守拦（fail-safe·523 同款）")
    断言("不可知" in 缘由, f"场景④ 缘由可读（{缘由[:50]}…）")


def 场景5_显式cn跳过(基础: str) -> None:
    根 = 临时git仓库(基础)
    头 = exe新鲜度.取HEAD提交时刻(根)
    exe = 造exe(根, "cn.exe", 头 - 99999)   # 极陈旧
    就绪, 错 = exe新鲜度.确保编译器就绪(str(exe), 候选们(), 根, None, 600)
    断言(就绪 == exe.resolve() and 错 == "", "场景⑤ --cn 显式指定陈旧 exe 也放行（跳过核验·用户自担）")
    就绪2, 错2 = exe新鲜度.确保编译器就绪(str(根 / "无此物.exe"), 候选们(), 根, None, 600)
    断言(就绪2 is None and "不存在" in 错2, "场景⑤ --cn 指向不存在路径仍显式红")


def 场景6_重建主循环(基础: str) -> None:
    根 = 临时git仓库(基础)
    头 = exe新鲜度.取HEAD提交时刻(根)
    造exe(根, "cn.exe", 头 - 600)   # 树内陈旧 exe（598 形态）
    重建调用 = {"数": 0}

    def 假构建成功(命令, 工作目录, 超时秒数=0):
        重建调用["数"] += 1
        重建调用["命令"] = 命令
        造exe(根, "cn.exe", time.time() + 5)   # 模拟构建产出新鲜 exe
        return type("R", (), {"returncode": 0, "stdout": "ok", "stderr": ""})()

    就绪, 错 = exe新鲜度.确保编译器就绪("", 候选们(), 根, 假构建成功, 600)
    断言(就绪 is not None and 错 == "", "场景⑥-a 陈旧→自动重建→复验过（主循环闭环）")
    断言(重建调用["数"] == 1, "场景⑥-a 重建恰好触发一次（不重复构建）")
    断言(time.time() - 5 <= 就绪.stat().st_mtime, "场景⑥-a 返回路径=重建后的新 exe")

    def 假构建失败(命令, 工作目录, 超时秒数=0):
        return type("R", (), {"returncode": 1, "stdout": "", "stderr": "boom"})()

    造exe(根, "cn.exe", 头 - 600)
    就绪2, 错2 = exe新鲜度.确保编译器就绪("", 候选们(), 根, 假构建失败, 600)
    断言(就绪2 is None, "场景⑥-b 重建失败→显式红")
    断言("--cn" in 错2 and "重建失败" in 错2, f"场景⑥-b 错误指路 --cn（{错2[:60]}…）")

    def 假构建无产出(命令, 工作目录, 超时秒数=0):
        return type("R", (), {"returncode": 0, "stdout": "", "stderr": ""})()

    就绪3, 错3 = exe新鲜度.确保编译器就绪("", 候选们(), 根, 假构建无产出, 600)
    断言(就绪3 is None and "仍陈旧" in 错3,
         f"场景⑥-c 重建无产出（exe 仍陈旧）→显式红防死循环（{错3[:50]}…）")


def 场景7_冷启动与探测语义(基础: str) -> None:
    根 = 临时git仓库(基础)
    断言(exe新鲜度.探测最新候选(候选们(), 根) is None, "场景⑦-a 无候选→None（冷启动态）")

    def 假构建(命令, 工作目录, 超时秒数=0):
        造exe(根, "cn.exe", time.time() + 5)
        return type("R", (), {"returncode": 0, "stdout": "ok", "stderr": ""})()

    就绪, 错 = exe新鲜度.确保编译器就绪("", 候选们(), 根, 假构建, 600)
    断言(就绪 is not None and 错 == "", "场景⑦-b 冷启动→构建通道兜底→自愈")
    # 556 语义保持：多候选取 mtime 最新（Debug 旧·根新→选根）
    造exe(根, "cn.exe", time.time() + 10)
    旧debug = 根 / "target" / "Debug" / "cn.exe"
    旧debug.parent.mkdir(exist_ok=True)
    旧debug.write_bytes(b"MZ")
    os.utime(旧debug, (time.time() - 9999, ) * 2)
    选 = exe新鲜度.探测最新候选([Path("target") / "Debug" / "cn.exe"] + 候选们(), 根)
    断言(选 == (根 / "target" / "cn.exe").resolve(), "场景⑦-c 多候选取 mtime 最新（556 语义保持）")


def main() -> int:
    win = sys.platform == "win32"
    with tempfile.TemporaryDirectory(prefix="cn_exe_freshness_test_") as 基础:
        print("== 场景① 新鲜 exe 过 ==")
        场景1_新鲜过(基础)
        print("== 场景② 判据 a：exe 早于 HEAD 提交（598 rebase 面） ==")
        场景2_判据a_晚于HEAD(基础)
        print("== 场景③ 判据 b：src 源比 exe 新（#175 假绿面） ==")
        场景3_判据b_源比exe新(基础)
        print("== 场景④ HEAD 不可知保守拦 ==")
        场景4_HEAD不可知保守拦(基础)
        print("== 场景⑤ --cn 显式指定跳过核验 ==")
        场景5_显式cn跳过(基础)
        print("== 场景⑥ 陈旧→自动重建主循环（假通道注入） ==")
        场景6_重建主循环(基础)
        print("== 场景⑦ 冷启动兜底与 556 探测语义 ==")
        场景7_冷启动与探测语义(基础)
        if win:
            print("== 场景⑧ win 重建脚本生成（静态断言·不执行） ==")
            命令们 = exe新鲜度._win重建命令(仓库根)
            if 命令们 is None:
                print("  - 跳过：本机无 vcvars64.bat（通道返回 None=合法路径）")
            else:
                # 已配置分支（伪造 build.ninja）：cmd 应含 088 重扫头修复
                已配根 = Path(tempfile.mkdtemp(prefix="fxw1_", dir=基础))
                (已配根 / "target" / "build-ninja").mkdir(parents=True)
                (已配根 / "target" / "build-ninja" / "build.ninja").write_text("# fake\n")
                cmd文本 = Path(exe新鲜度._win重建命令(已配根)[0][2]).read_text(encoding="ascii")
                断言("vcvars64.bat" in cmd文本, "场景⑧-a cmd 含 vcvars 调用")
                断言("cmake --build target/build-ninja" in cmd文本, "场景⑧-a cmd 含增量构建命令")
                断言(".ninja_deps" in cmd文本, "场景⑧-a cmd 含 088 重扫头修复")
                # 未配置分支：cmd 应含 cmake 配置（sccache 探测可选）与构建
                未配根 = Path(tempfile.mkdtemp(prefix="fxw2_", dir=基础))
                cmd2 = Path(exe新鲜度._win重建命令(未配根)[0][2]).read_text(encoding="ascii")
                断言("cmake -G Ninja -S . -B target/build-ninja" in cmd2,
                     "场景⑧-b 未配置分支 cmd 含 cmake 配置命令")
                断言("CMAKE_BUILD_TYPE=Debug" in cmd2, "场景⑧-b 配置带 Debug 口径（/Z7 防 C1041）")
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
