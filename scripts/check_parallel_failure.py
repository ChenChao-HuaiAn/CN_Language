#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#1220 并行 codegen 工作线程失败传播门禁（§1.1a② 确定性失败）。

背景（1145/1219）：函数级并行 codegen 的任务函数在 worker 线程内执行。改前任务内
抛异常会直接逃逸线程函数 -> std::terminate（进程异常终止，无诊断/无 rc 契约）；
空函数文本则静默拼接出不完整汇编。本门禁锁定失败传播契约（真实编译器链路，
不 mock）：

  ① 任务异常（std::exception）：rc=1、诊断含内部错误+函数序号+函数名、
     进程不 terminate、无残留 asm 产物、决策行 reason=worker-error 且 workers
     等于实际线程数（观测与真实一致）。
  ② 未知异常（非 std::exception，catch (...) 通道）：同上。
  ③ 空函数文本（发射器零产出）：同上（诊断含「函数文本为空」）。
  ④ 已有产物不被截断/覆盖：失败编译后旧文件逐字节不变（不落盘=现有失败路径口径）。
  ⑤ 线程创建失败：确定性降级——rc=0、产物与串行基线逐字节一致、警告在
     Diagnostics（human stderr / --json stdout 数组）可见且计数=1、决策行
     workers=1（诚实暴露并行度损失）。
  ⑥ 三后端全覆盖（win-x64 / linux-x86_64 / linux-arm64 各自并行实现）。
  ⑦ 注入配置错误显式化（P2-1/P2-4）：非法模式、序号越界、2^64 整数回绕均
     rc=1 + 内部错误，不得回绕成小序号误命中某函数、不得产生决策行/残留产物。
  ⑧ --json 两态（P2-2）：失败=单个诊断数组含错误项（函数名/序号可见）；
     降级=单个数组含 1 条警告且 0 错误项（driver 在代码生成后统一冲刷；
     human 补差量、JSON 单数组；compile 成功行在 --json 模式抑制）。
  ⑨ 只读冻结 miss（1501）：unwarmed-class:<序号> 注入使指定任务查询未预热类名
     ——修复前=worker 静默写共享 classCache（并发写 UB 假绿）；修复后=冻结期零写
     + miss 登记 → join 后主线程报内部错误、rc=1、无残留产物、决策
     reason=readonly-miss 且 readonly=sealed（预热真实发生）。

注入点（生产未设=零行为）：CN_CODEGEN_FAILURE_INJECT=throw-std:<序号> /
throw-unknown:<序号> / empty-text:<序号> / thread-create / unwarmed-class:<序号>。
注入失效（未进并行路径/序号越界/取值非法/2^64 回绕）由编译器显式报内部错误，
本门禁另断言注入确实命中。

挂载：`check_parallel_determinism.py` 主检查通过后链式调用本脚本（同一编译器、
同一触发面），故 L1 写集条件步与云端恒跑门禁自动覆盖本面；selftest 另由
gate_quick 静态段无条件执行（纯合成夹具，无需编译器）。

用法：
  python scripts/check_parallel_failure.py [--cn target/cn.exe]
  python scripts/check_parallel_failure.py --selftest
退出码：0=通过；1=违例；2=环境/用法错误。
"""

from __future__ import annotations

import argparse
import io
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile

# 控制台编码（中文诊断防 GBK/C locale 崩——与 check_parallel_determinism 同口径）
try:
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
except Exception:
    pass

项目根 = pathlib.Path(__file__).resolve().parent.parent
并行实现源 = 项目根 / "src" / "cn_compiler" / "codegen" / "parallel_codegen.cpp"
标准库 = 项目根 / "stdlib"

决策前缀 = "[codegen-parallel]"
必需决策字段 = ("parallel", "functions", "threads", "workers", "labels",
                "shifted", "reason", "readonly", "prewarmed")
注入变量 = "CN_CODEGEN_FAILURE_INJECT"
并行线程 = "4"
编译超时 = 120

# 代表样本（IR 函数 20·覆盖面与确定性门禁同源；函数序号中/后段各测一次）
样本 = {"名": "1026_三元与方法链消费位释放", "入口": "主.cn"}
目标们 = ("win-x64", "linux-x86_64", "linux-arm64")


def 找编译器(显式路径):
    """返回可用编译器路径；未找到=无值。"""
    if 显式路径:
        候选 = [pathlib.Path(显式路径)]
    else:
        名 = "cn.exe" if sys.platform.startswith("win") else "cn"
        候选 = [项目根 / "target" / 名,
               项目根 / "target" / "Debug" / 名,
               项目根 / "target" / "build-ninja" / 名,
               项目根 / "target" / "build-quick" / 名]
    for 路径 in 候选:
        if 路径.exists():
            return 路径
    return None


def 读并行阈值():
    """从并行实现源码机械读取函数数阈值；读不到=门禁失明（拒绝静默通过）。"""
    if not 并行实现源.exists():
        return None, "并行实现源不存在: %s" % 并行实现源
    try:
        文本 = 并行实现源.read_text(encoding="utf-8", errors="replace")
    except OSError as 异常:
        return None, "读取并行实现源失败: %s" % 异常
    匹配 = re.search(r"kMinFunctionsForParallel\s*=\s*(\d+)\s*;", 文本)
    if 匹配 is None:
        return None, ("无法从 parallel_codegen.cpp 读取 kMinFunctionsForParallel"
                      "（源码形态变化=门禁失明，拒绝静默通过）")
    return int(匹配.group(1)), ""


def 解析决策(错误输出):
    """解析 [codegen-parallel] 决策行；返回 (字段字典|None, 错误)。"""
    行们 = [行.strip() for 行 in 错误输出.splitlines()
            if 行.strip().startswith(决策前缀)]
    if len(行们) != 1:
        return None, "并行决策摘要行数=%d（期望 1；观测通道失效）" % len(行们)
    字段 = {}
    for 片段 in 行们[0].split()[1:]:
        if "=" in 片段:
            键, 值 = 片段.split("=", 1)
            字段[键] = 值
    缺失 = [键 for 键 in 必需决策字段 if 键 not in 字段]
    if 缺失:
        return None, "决策摘要缺字段 %s: %s" % (缺失, 行们[0])
    return 字段, ""


class 真编译器:
    """真实 cn 编译器：compile + ir 函数名采集（缓存/注入环境显式隔离）。"""

    def __init__(self, 路径):
        self.路径 = pathlib.Path(路径)

    def _环境(self, 线程口径, 注入):
        环境 = dict(os.environ)
        环境.update({
            "CN_NO_PROGRAM_CACHE": "1",   # 程序缓存命中跳过 codegen=负测空转
            "CN_NO_AST_CACHE": "1",
            "CN_CODEGEN_STATS": "1",      # 决策行=workers/functions 观测通道
        })
        环境["CN_CODEGEN_THREADS"] = 线程口径
        环境.pop(注入变量, None)
        if 注入 is not None:
            环境[注入变量] = 注入
        return 环境

    def _运行(self, 命令, 环境):
        try:
            return subprocess.run(命令, cwd=str(项目根), env=环境,
                                  capture_output=True, text=True, encoding="utf-8",
                                  errors="replace", timeout=编译超时), ""
        except subprocess.TimeoutExpired:
            return None, "编译超时（%ds）" % 编译超时
        except OSError as 异常:
            return None, "无法执行编译器 %s: %s" % (self.路径, 异常)

    def 函数名(self, 入口):
        """返回 (函数名列表, 错误)。`cn ir` 输出行=「函数 <名> (返回 <类型>)」。"""
        结果, 运行错误 = self._运行(
            [str(self.路径), "ir", str(入口), "--stdlib", str(标准库)],
            self._环境("1", None))
        if 运行错误:
            return None, "cn ir 执行失败：%s" % 运行错误
        if 结果.returncode != 0:
            return None, "cn ir 失败 rc=%d: %s" % (
                结果.returncode, (结果.stderr or 结果.stdout).strip()[:300])
        名们 = []
        for 行 in 结果.stdout.splitlines():
            if not 行.startswith("函数 "):
                continue
            余 = 行[len("函数 "):]
            切 = 余.rfind(" (返回 ")
            if 切 < 0:
                return None, "无法解析 cn ir 函数行: %s" % 行
            名们.append(余[:切])
        if not 名们:
            return None, "cn ir 输出未含函数行"
        return 名们, ""

    def 编译(self, 入口, 目标, 输出, 线程口径="4", 注入=None, json诊断=False):
        命令 = [str(self.路径), "compile", str(入口), "--target", 目标,
                "--output", str(输出), "--stdlib", str(标准库)]
        if json诊断:
            命令.append("--json")
        结果, 运行错误 = self._运行(命令, self._环境(线程口径, 注入))
        if 运行错误:
            return {"rc": -99, "stderr": "[执行失败] " + 运行错误,
                    "stdout": "", "产物存在": False, "函数数": 0}
        return {"rc": 结果.returncode, "stderr": 结果.stderr or "",
                "stdout": 结果.stdout or "", "产物存在": pathlib.Path(输出).exists(),
                "函数数": 0}


def 检查失败结果(结果, 序号, 函数名, 消息片段, 期望无新产物=True):
    """失败编译断言；返回错误列表（任何一条不符=红）。"""
    错误 = []
    stderr = 结果["stderr"]
    if 结果["rc"] != 1:
        错误.append("rc=%s（契约=1；0=静默继续/其他值=异常终止）" % 结果["rc"])
    低 = stderr.lower()
    for 终止词 in ("terminate called", "std::terminate", "terminate_handler"):
        if 终止词 in 低:
            错误.append("stderr 出现 %r：工作线程异常逃逸（std::terminate）" % 终止词)
    if "内部错误" not in stderr:
        错误.append("诊断未含「内部错误」（编译器 bug 未显形）")
    if ("#" + str(序号)) not in stderr:
        错误.append("诊断未含函数序号 #%d" % 序号)
    if 函数名 not in stderr:
        错误.append("诊断未含函数名 %r" % 函数名)
    if 消息片段 not in stderr:
        错误.append("诊断未含原因片段 %r" % 消息片段)
    if 期望无新产物 and 结果["产物存在"]:
        错误.append("失败后残留汇编产物（部分完成产物必须不落盘）")
    决策, 决策错误 = 解析决策(stderr)
    if 决策错误:
        错误.append(决策错误)
    else:
        if 决策["parallel"] != "yes":
            错误.append("失败决策 parallel=%s（期望 yes=并行路径确实发生）"
                        % 决策["parallel"])
        if 决策["reason"] != "worker-error":
            错误.append("失败决策 reason=%s（期望 worker-error）" % 决策["reason"])
        if 决策["workers"] != 并行线程:
            错误.append("失败决策 workers=%s（期望 %s=实际启动线程数，观测失真）"
                        % (决策["workers"], 并行线程))
        if 决策["threads"] != 并行线程:
            错误.append("失败决策 threads=%s（期望 %s）"
                        % (决策["threads"], 并行线程))
        if 决策["functions"] != str(结果["函数数"]):
            错误.append("失败决策 functions=%s（期望 %d=IR 函数数，观测失真）"
                        % (决策["functions"], 结果["函数数"]))
    return 错误


def 检查降级结果(结果, 串行字节, 需要stderr警告=True):
    """线程创建失败降级断言（rc=0、产物==串行、警告与 workers 诚实）。
    --json 模式下诊断走 stdout（警告不在 stderr），需要stderr警告=False。"""
    错误 = []
    stderr = 结果["stderr"]
    if 结果["rc"] != 0:
        错误.append("rc=%s（线程创建失败=可恢复降级，产物确定应成功）" % 结果["rc"])
        return 错误
    if not 结果["产物存在"]:
        错误.append("降级路径未生成汇编产物")
        return 错误
    if 结果["产物字节"] != 串行字节:
        错误.append("降级产物与串行基线不一致（确定性被破坏）")
    if 需要stderr警告:
        if "线程创建失败" not in stderr or "警告" not in stderr:
            错误.append("stderr 未含线程创建失败警告（并行度损失被静默吞掉）")
        if ("worker 数=1/" + 并行线程) not in stderr:
            错误.append("警告未含实际 worker 数=1/%s" % 并行线程)
    if "内部错误" in stderr:
        错误.append("降级被误报为内部错误（应 rc=0 + 警告）")
    决策, 决策错误 = 解析决策(stderr)
    if 决策错误:
        错误.append(决策错误)
    else:
        if 决策["parallel"] != "yes" or 决策["reason"] != "eligible":
            错误.append("降级决策不符（parallel=%s reason=%s）"
                        % (决策["parallel"], 决策["reason"]))
        if 决策["threads"] != 并行线程 or 决策["workers"] != "1":
            错误.append("降级决策 threads=%s workers=%s（期望 %s/1）"
                        % (决策["threads"], 决策["workers"], 并行线程))
    return 错误


def 解析json诊断(文本):
    """解析 --json 诊断数组；返回 (列表|None, 错误)。多数组拼接/非数组=显式错误。"""
    try:
        诊断 = json.loads(文本)
    except (ValueError, TypeError) as 异常:
        return None, "stdout 不是合法 JSON（%s；尾部 %r）" % (异常, 文本[-200:])
    if not isinstance(诊断, list):
        return None, "stdout JSON 不是诊断数组（%s）" % type(诊断).__name__
    for 项 in 诊断:
        if not isinstance(项, dict) or "level" not in 项 or "message" not in 项:
            return None, "诊断项缺 level/message: %r" % (项,)
    return 诊断, ""


def 检查失败配置结果(结果, 消息片段):
    """注入配置错误断言（P2-4：非法值/序号越界/2^64 溢出必须显式报错，不得假绿）。"""
    错误 = []
    stderr = 结果["stderr"]
    if 结果["rc"] != 1:
        错误.append("rc=%s（配置错误契约=1）" % 结果["rc"])
    if "内部错误" not in stderr:
        错误.append("诊断未含「内部错误」")
    if 消息片段 not in stderr:
        错误.append("诊断未含原因片段 %r" % 消息片段)
    if "（函数 #" in stderr:
        错误.append("配置错误被伪装成某函数任务失败（不得回绕/误命中序号）")
    if 决策前缀 in stderr:
        错误.append("配置错误不应产生并行决策行（未进入执行阶段）")
    if 结果["产物存在"]:
        错误.append("配置错误后残留汇编产物")
    return 错误


def 检查未预热注入结果(结果):
    """1501：并行期未预热类查询=只读冻结 miss 显形断言（真实编译器注入）。

    修复前：worker 首次查询静默写共享 classCache（并发写 UB，rc=0 假绿）；
    修复后：冻结期零写 + miss 登记 → join 后主线程报内部错误、rc=1、无产物、
    决策 reason=readonly-miss 且 readonly=sealed（预热真实发生）。"""
    错误 = []
    stderr = 结果["stderr"]
    if 结果["rc"] != 1:
        错误.append("rc=%s（契约=1：未预热查询必须中止编译，禁静默继续）"
                    % 结果["rc"])
    if "内部错误" not in stderr:
        错误.append("诊断未含「内部错误」（只读冻结 miss 未显形）")
    if "__1499_未预热类__" not in stderr:
        错误.append("诊断未含未预热类名（miss 名丢失）")
    if "未预热" not in stderr:
        错误.append("诊断未含「未预热」原因片段")
    if 结果["产物存在"]:
        错误.append("未预热 miss 失败后残留汇编产物（部分产物必须不落盘）")
    决策, 决策错误 = 解析决策(stderr)
    if 决策错误:
        错误.append(决策错误)
    else:
        if 决策["parallel"] != "yes":
            错误.append("未预热 miss 决策 parallel=%s（期望 yes）"
                        % 决策["parallel"])
        if 决策["reason"] != "readonly-miss":
            错误.append("未预热 miss 决策 reason=%s（期望 readonly-miss）"
                        % 决策["reason"])
        if 决策["readonly"] != "sealed":
            错误.append("未预热 miss 决策 readonly=%s（期望 sealed=冻结真实发生）"
                        % 决策["readonly"])
        if 决策["prewarmed"] == "0":
            错误.append("未预热 miss 决策 prewarmed=0（预热通道未执行）")
    return 错误


def 检查json失败结果(结果, 序号, 函数名, 消息片段):
    """--json 失败态：单数组、错误计数=1、消息含函数名/序号（P2-2）。"""
    错误 = []
    if 结果["rc"] != 1:
        错误.append("rc=%s（契约=1）" % 结果["rc"])
    if 结果["产物存在"]:
        错误.append("失败后残留汇编产物")
    诊断, 解析错误 = 解析json诊断(结果["stdout"])
    if 解析错误:
        return 错误 + [解析错误]
    错误项 = [项 for 项 in 诊断 if 项.get("level") == "错误"]
    if len(错误项) != 1:
        错误.append("JSON 错误诊断数=%d（期望 1）" % len(错误项))
        return 错误
    消息 = 错误项[0].get("message", "")
    for 片段 in ("内部错误", "#" + str(序号), 函数名, 消息片段):
        if 片段 not in 消息:
            错误.append("JSON 错误诊断缺片段 %r" % 片段)
    return 错误


def 检查json降级结果(结果, 串行字节):
    """--json 降级态：警告在 stdout 单数组可见、计数=1、无错误项（P2-2）。"""
    错误 = 检查降级结果(结果, 串行字节, 需要stderr警告=False)
    诊断, 解析错误 = 解析json诊断(结果["stdout"])
    if 解析错误:
        return 错误 + [解析错误]
    警告项 = [项 for 项 in 诊断 if 项.get("level") == "警告"]
    错误项 = [项 for 项 in 诊断 if 项.get("level") == "错误"]
    if len(警告项) != 1:
        错误.append("JSON 警告诊断数=%d（期望 1——线程创建失败警告必须可见）"
                    % len(警告项))
    else:
        消息 = 警告项[0].get("message", "")
        if "线程创建失败" not in 消息:
            错误.append("JSON 警告未含「线程创建失败」")
        if ("worker 数=1/" + 并行线程) not in 消息:
            错误.append("JSON 警告未含实际 worker 数=1/%s" % 并行线程)
    if len(错误项) != 0:
        错误.append("降级 JSON 出现 %d 条错误项（应仅警告）" % len(错误项))
    return 错误


def 主检查(编译器):
    阈值, 阈值错误 = 读并行阈值()
    if 阈值错误:
        print("[并行失败传播] FAIL：" + 阈值错误, flush=True)
        return 1
    入口 = 项目根 / "tests" / "e2e" / 样本["名"] / 样本["入口"]
    if not 入口.exists():
        print("[并行失败传播] FAIL：样本入口不存在: %s" % 入口, flush=True)
        return 1
    函数名们, 取名错误 = 编译器.函数名(入口)
    if 取名错误:
        print("[并行失败传播] FAIL：" + 取名错误, flush=True)
        return 1
    if len(函数名们) < 阈值:
        print("[并行失败传播] FAIL：IR 函数数 %d < 并行阈值 %d——样本无法覆盖并行路径"
              % (len(函数名们), 阈值), flush=True)
        return 1
    中段 = len(函数名们) // 2
    后段 = len(函数名们) - 1
    未知段 = 1
    失败模式 = (("throw-std", 中段, "注入故障"),
                ("throw-unknown", 未知段, "未知异常"),
                ("empty-text", 后段, "函数文本为空"))
    错误 = []
    统计 = {"编译": 0, "失败用例": 0, "降级用例": 0, "配置用例": 0}
    for 目标 in 目标们:
        with tempfile.TemporaryDirectory(prefix="cnpf_") as 临时目录:
            临时 = pathlib.Path(临时目录)
            for 模式, 序号, 片段 in 失败模式:
                输出 = 临时 / (模式 + ".asm")
                结果 = 编译器.编译(入口, 目标, 输出, 并行线程,
                                   "%s:%d" % (模式, 序号))
                统计["编译"] += 1
                统计["失败用例"] += 1
                结果["函数数"] = len(函数名们)
                错误.extend(["%s/%s: %s" % (目标, 模式, 项)
                             for 项 in 检查失败结果(结果, 序号,
                                                    函数名们[序号], 片段)])
            # 1501：未预热类查询=只读冻结 miss 显形（修复前静默写共享缓存=假绿）
            未预热输出 = 临时 / "unwarmed-class.asm"
            未预热结果 = 编译器.编译(入口, 目标, 未预热输出, 并行线程,
                                     "unwarmed-class:%d" % 中段)
            统计["编译"] += 1
            统计["失败用例"] += 1
            错误.extend(["%s/unwarmed-class: %s" % (目标, 项)
                         for 项 in 检查未预热注入结果(未预热结果)])
            # 已有产物不得被失败路径截断/覆盖（写盘调用必须根本不发生）
            哨兵 = 临时 / "sentinel.asm"
            哨兵.write_bytes(b"OLD-ASM-MUST-STAY")
            结果 = 编译器.编译(入口, 目标, 哨兵, 并行线程,
                               "throw-std:%d" % 中段)
            统计["编译"] += 1
            结果["产物存在"] = True  # 旧文件存在不是「残留新产物」
            结果["函数数"] = len(函数名们)
            错误.extend(["%s/哨兵: %s" % (目标, 项)
                         for 项 in 检查失败结果(结果, 中段, 函数名们[中段],
                                                "注入故障", 期望无新产物=False)])
            if 哨兵.read_bytes() != b"OLD-ASM-MUST-STAY":
                错误.append("%s/哨兵: 失败编译覆盖/截断了已有产物" % 目标)
            # 线程创建失败：确定性降级（产物==串行基线，workers=1 诚实可见）
            串行输出 = 临时 / "serial.asm"
            串行结果 = 编译器.编译(入口, 目标, 串行输出, "1", None)
            统计["编译"] += 1
            if 串行结果["rc"] != 0 or not 串行输出.exists():
                错误.append("%s/降级基线: 串行编译失败 rc=%s"
                            % (目标, 串行结果["rc"]))
                continue
            降级输出 = 临时 / "thread-create.asm"
            降级结果 = 编译器.编译(入口, 目标, 降级输出, 并行线程,
                                   "thread-create")
            统计["编译"] += 1
            统计["降级用例"] += 1
            降级结果["产物字节"] = 降级输出.read_bytes() if 降级输出.exists() else b""
            错误.extend(["%s/thread-create: %s" % (目标, 项)
                         for 项 in 检查降级结果(降级结果, 串行输出.read_bytes())])
    # 注入配置错误三态（P2-1/P2-4）：非法模式/序号越界/2^64 溢出——真实编译断言，
    #   配置错误必须显式内部错误（不得回绕成小序号误命中某函数、不得静默假绿）。
    with tempfile.TemporaryDirectory(prefix="cnpf_cfg_") as 临时目录:
        临时 = pathlib.Path(临时目录)
        配置用例 = (
            ("bogus:5", "非法 CN_CODEGEN_FAILURE_INJECT 模式"),
            ("throw-std:999999", "测试故障注入函数序号越界"),
            ("throw-std:18446744073709551617",
             "非法 CN_CODEGEN_FAILURE_INJECT 函数序号"),
        )
        for 注入值, 片段 in 配置用例:
            输出 = 临时 / (注入值.replace(":", "_") + ".asm")
            结果 = 编译器.编译(入口, 目标们[0], 输出, 并行线程, 注入值)
            统计["编译"] += 1
            统计["配置用例"] += 1
            错误.extend(["配置/%s: %s" % (注入值, 项)
                         for 项 in 检查失败配置结果(结果, 片段)])
    # --json 两态（P2-2）：失败=单数组错误诊断；降级=警告在 stdout 可见、计数一致。
    with tempfile.TemporaryDirectory(prefix="cnpf_json_") as 临时目录:
        临时 = pathlib.Path(临时目录)
        输出 = 临时 / "json-fail.asm"
        结果 = 编译器.编译(入口, 目标们[0], 输出, 并行线程,
                           "throw-std:%d" % 中段, json诊断=True)
        统计["编译"] += 1
        结果["函数数"] = len(函数名们)
        错误.extend(["--json 失败: " + 项 for 项 in
                     检查json失败结果(结果, 中段, 函数名们[中段], "注入故障")])
        串行输出 = 临时 / "json-serial.asm"
        串行结果 = 编译器.编译(入口, 目标们[0], 串行输出, "1", None)
        统计["编译"] += 1
        if 串行结果["rc"] != 0 or not 串行输出.exists():
            错误.append("--json 降级基线: 串行编译失败 rc=%s" % 串行结果["rc"])
        else:
            降级输出 = 临时 / "json-thread-create.asm"
            降级结果 = 编译器.编译(入口, 目标们[0], 降级输出, 并行线程,
                                   "thread-create", json诊断=True)
            统计["编译"] += 1
            降级结果["产物字节"] = (降级输出.read_bytes()
                                   if 降级输出.exists() else b"")
            错误.extend(["--json 降级: " + 项 for 项 in
                         检查json降级结果(降级结果, 串行输出.read_bytes())])
    if 错误:
        print("[并行失败传播] FAIL：%d 项" % len(错误), flush=True)
        for 项 in 错误:
            print("  - " + 项, flush=True)
        return 1
    print("[并行失败传播] PASS ✓ 目标=%d 编译=%d（异常/未知异常/空文本/哨兵/"
          "降级/配置错误×3/只读冻结 miss/--json 两态）"
          % (len(目标们), 统计["编译"]), flush=True)
    return 0


# ============ 自测（合成夹具正反两态，无需编译器） ============

def _成功决策(workers="4", reason="worker-error"):
    # 1501：并行路径决策行新增语义只读冻结观测字段（readonly/prewarmed）
    return ("%s parallel=yes functions=20 threads=%s workers=%s labels=0 shifted=0 "
            "reason=%s readonly=sealed prewarmed=8\n"
            % (决策前缀, 并行线程, workers, reason))


def _正例文本(序号, 函数名, 片段="注入故障"):
    return ("未知位置: 错误: 内部错误: 并行代码生成中止（函数 #%d \"%s\"）: "
            "工作线程任务异常（std::exception）: %s\n" % (序号, 函数名, 片段)) + \
        _成功决策()


def 自测():
    """断言器判别力自证：正例必绿，每类违约必红。"""
    函数名 = "示例函数"
    序号 = 7
    基准 = {"rc": 1, "stderr": _正例文本(序号, 函数名),
            "产物存在": False, "函数数": 20}
    用例们 = [
        ("完整失败传播（正例）", dict(基准), ""),
        ("rc=0 静默继续", dict(基准, rc=0), "rc="),
        ("异常逃逸 terminate", dict(基准, stderr=基准["stderr"] +
                                    "terminate called after throwing\n"), "terminate"),
        ("诊断缺内部错误", dict(基准, stderr=基准["stderr"].replace("内部错误", "普通错误")),
         "内部错误"),
        ("诊断缺函数序号", dict(基准, stderr=基准["stderr"].replace(
            "函数 #7", "函数 #9")), "函数序号"),
        ("诊断缺函数名", dict(基准, stderr=基准["stderr"].replace(函数名, "别的函数")),
         "函数名"),
        ("残留部分产物", dict(基准, 产物存在=True), "残留汇编产物"),
        ("决策 reason 不符", dict(基准, stderr=_正例文本(序号, 函数名).replace(
            "reason=worker-error", "reason=eligible")), "reason="),
        ("workers 观测失真", dict(基准, stderr=_正例文本(序号, 函数名).replace(
            "workers=4", "workers=3")), "workers="),
        ("决策行缺失", dict(基准, stderr="未知位置: 错误: 内部错误（函数 #7 \"示例函数\"）: "
                             "注入故障\n"), "决策摘要行数"),
    ]
    失败 = 0
    for 名称, 结果, 期望 in 用例们:
        结果["函数数"] = 20
        错误 = 检查失败结果(结果, 序号, 函数名, "注入故障")
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    # 降级断言正反两态（human 通道：警告来自 Diagnostics，格式=「未知位置: 警告:」）
    降级基 = {"rc": 0, "stderr": ("未知位置: 警告: 并行 codegen：线程创建失败，实际 "
                                  "worker 数=1/4（任务由主线程+已建线程完成，"
                                  "产物确定性不变）\n" + _成功决策("1", "eligible")),
              "产物存在": True, "产物字节": b"ASM", "函数数": 20}
    降级用例 = [
        ("线程创建失败降级（正例）", dict(降级基), ""),
        ("降级产物与串行不一致", dict(降级基, 产物字节=b"ASM2"), "串行基线"),
        ("降级无警告", dict(降级基, stderr=_成功决策("1", "eligible")), "线程创建失败警告"),
        ("降级 workers 谎报", dict(降级基, stderr=降级基["stderr"].replace(
            "workers=1", "workers=4")), "workers="),
        ("降级 rc=1", dict(降级基, rc=1), "rc="),
    ]
    for 名称, 结果, 期望 in 降级用例:
        错误 = 检查降级结果(结果, b"ASM")
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    # 注入配置错误断言（P2-1/P2-4）：正例绿；伪装函数失败/决策行/残留=必红
    配置基 = {"rc": 1,
              "stderr": ("未知位置: 错误: 内部错误: 非法 "
                         "CN_CODEGEN_FAILURE_INJECT 函数序号（须十进制非负整）: "
                         "throw-std:18446744073709551617\n"),
              "产物存在": False, "函数数": 20}
    配置用例 = [
        ("配置错误显式报错（正例）", dict(配置基), ""),
        ("配置错误伪装函数失败", dict(配置基, stderr=配置基["stderr"] +
                                    "（函数 #1 \"示例函数\"）\n"), "伪装"),
        ("配置错误仍产决策行", dict(配置基, stderr=配置基["stderr"] +
                                    _成功决策()), "决策行"),
        ("配置错误残留产物", dict(配置基, 产物存在=True), "残留"),
    ]
    for 名称, 结果, 期望 in 配置用例:
        错误 = 检查失败配置结果(结果, "非法 CN_CODEGEN_FAILURE_INJECT 函数序号")
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    # 1501：只读冻结 miss 显形断言正反两态（未预热查询必须中止编译）
    未预热基 = {"rc": 1,
              "stderr": ("未知位置: 错误: 内部错误: 并行代码生成中止: "
                         "并行 codegen 语义只读缓存未预热命中（类名 "
                         "'__1499_未预热类__'）——预热清单不完整；已中止编译，"
                         "不产出部分汇编\n" + _成功决策(reason="readonly-miss")),
              "产物存在": False, "函数数": 20}
    未预热用例 = [
        ("只读冻结 miss 显形（正例）", dict(未预热基), ""),
        ("miss 静默继续", dict(未预热基, rc=0), "rc="),
        ("miss 未含类名", dict(未预热基, stderr=未预热基["stderr"].replace(
            "__1499_未预热类__", "某类")), "未预热类名"),
        ("miss 残留产物", dict(未预热基, 产物存在=True), "残留"),
        ("miss 决策 reason 不符", dict(未预热基, stderr=未预热基["stderr"].replace(
            "reason=readonly-miss", "reason=eligible")), "期望 readonly-miss"),
        ("miss 决策未冻结", dict(未预热基, stderr=未预热基["stderr"].replace(
            "readonly=sealed", "readonly=off")), "readonly="),
    ]
    for 名称, 结果, 期望 in 未预热用例:
        错误 = 检查未预热注入结果(结果)
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    # --json 失败态/降级态断言（P2-2）：单数组/计数/警告可见
    def _json项(level, message):
        return {"level": level, "file": "", "line": 0, "column": 0, "message": message}

    json失败基 = {"rc": 1, "stderr": "", "产物存在": False, "函数数": 20,
                 "stdout": json.dumps([_json项(
                     "错误", "内部错误: 并行代码生成中止（函数 #7 \"示例函数\"）: "
                             "工作线程任务异常: 注入故障")], ensure_ascii=False)}
    json失败用例 = [
        ("--json 失败单数组（正例）", dict(json失败基), ""),
        ("--json 两数组拼接", dict(json失败基, stdout=(
            json失败基["stdout"] + "\n" + json失败基["stdout"])), "合法 JSON"),
        ("--json 错误计数不符", dict(json失败基, stdout=json.dumps(
            [_json项("错误", "内部错误 #7 示例函数 注入故障"),
             _json项("错误", "重复错误")], ensure_ascii=False)), "错误诊断数=2"),
        ("--json 缺函数名", dict(json失败基, stdout=json.dumps(
            [_json项("错误", "内部错误（函数 #7）: 注入故障")],
            ensure_ascii=False)), "示例函数"),
    ]
    for 名称, 结果, 期望 in json失败用例:
        错误 = 检查json失败结果(结果, 序号, 函数名, "注入故障")
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    json降级基 = {"rc": 0, "stderr": _成功决策("1", "eligible"),
                 "产物存在": True, "产物字节": b"ASM", "函数数": 20,
                 "stdout": json.dumps([_json项(
                     "警告", "并行 codegen：线程创建失败，实际 worker 数=1/4"
                             "（任务由主线程+已建线程完成，产物确定性不变）")],
                     ensure_ascii=False)}
    json降级用例 = [
        ("--json 降级警告可见（正例）", dict(json降级基), ""),
        ("--json 降级缺警告", dict(json降级基, stdout="[]"), "警告诊断数=0"),
        ("--json 降级多错误项", dict(json降级基, stdout=json.dumps(
            [json.loads(json降级基["stdout"])[0], _json项("错误", "多余错误")],
            ensure_ascii=False)), "错误项"),
    ]
    for 名称, 结果, 期望 in json降级用例:
        错误 = 检查json降级结果(结果, b"ASM")
        if 期望 == "":
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else "期望红未命中: %s" % 错误
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 名称, 说明), flush=True)
        if not 通过:
            失败 += 1

    if 失败:
        print("[selftest] FAIL：%d 项" % 失败, flush=True)
        return 1
    print("[selftest] PASS ✓（失败传播 %d 态 + 降级 %d 态 + 配置错误 %d 态 + "
          "只读冻结 %d 态 + --json %d 态）"
          % (len(用例们), len(降级用例), len(配置用例), len(未预热用例),
             len(json失败用例) + len(json降级用例)), flush=True)
    return 0


def main():
    解析器 = argparse.ArgumentParser(
        description="并行 codegen 工作线程失败传播门禁（异常/空文本/线程创建失败）")
    解析器.add_argument("--cn", default="", help="cn 编译器路径（默认自动探测）")
    解析器.add_argument("--selftest", action="store_true",
                        help="合成夹具断言器正反自测（无需编译器）")
    参数 = 解析器.parse_args()
    if 参数.selftest:
        return 自测()
    编译器 = 找编译器(参数.cn)
    if 编译器 is None:
        print("[并行失败传播] FAIL：未找到 cn 编译器（--cn 指定或先构建 target/cn）",
              flush=True)
        return 2
    if not 标准库.is_dir():
        print("[并行失败传播] FAIL：标准库目录不存在: %s" % 标准库, flush=True)
        return 1
    return 主检查(真编译器(编译器))


if __name__ == "__main__":
    sys.exit(main())
