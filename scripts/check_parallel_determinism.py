#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#1219 并行 codegen 产物确定性常态门禁（并行 vs 串行逐字节 + 多线程数自一致）。

背景（1145）：函数级并行发射按原函数序拼接，设计承诺「并行产物与串行逐字节
一致」。该承诺当轮一次性验证过，之后无常态门禁——线程数决策、worker 池拼接序、
标签编号平移、常量池回退等任何改动都可能悄悄破坏确定性。本门禁把对照常态化：

  ① 同一输入五种口径（全部与串行逐字节对照、并行互相自一致）：
     - 并行1：CN_CODEGEN_THREADS=4
     - 并行2：CN_CODEGEN_THREADS=4（同口径第二遍=自一致/非确定性抓捕）
     - 并行3：CN_CODEGEN_THREADS=2（不同线程数=交错面扩大/线程数敏感性）
     - 自动：不注入该变量（生产默认=hardware_concurrency；单核 runner 显式跳过
       并打印原因）
     - 串行：CN_CODEGEN_THREADS=1
  ② 观测通道断言（CN_CODEGEN_STATS=1·1219 增）：并行口径必须报告
     parallel=yes、workers>=2（实际启动 worker 数，线程创建失败退化即红）、
     labels>0 且 shifted>0（index>0 函数标签数之和——证明编号平移真实发生，
     首函数 base=0 不计）；串行必须 parallel=no reason=threads-serial；
     自动口径另断言 threads>=2（生产默认分支未退化）。
  ③ 样本非空转：`cn ir` 统计 IR 函数数，必须 >= 源码阈值
     kMinFunctionsForParallel（从 parallel_codegen.cpp 机械读取；读不到=红）。
  ④ 三后端全覆盖：win-x64 / linux-x86_64 / linux-arm64 各有独立并行实现。
  ⑤ 全局 300s deadline：超时 rc=1（防单步挂死拖垮门禁轮）。
  ⑥ 失败传播负测链（1220）：确定性对照通过后链式执行
     scripts/check_parallel_failure.py（真实编译器注入任务异常/未知异常/空文本/
     线程创建失败，断言 rc 契约、诊断含函数名、无残留产物、workers 观测）；
     该脚本 rc 即本门禁 rc（同一触发面/云端恒跑自动覆盖本面）。

红线：逐字节差异/决策不符/样本失格/观测缺失/全局超时/失败传播违约=FAIL；
不提供 --allow 类放行参数。缓存隔离：CN_NO_PROGRAM_CACHE=1（程序缓存命中会跳过
代码生成，使对照假绿）+ CN_NO_AST_CACHE=1。
自测（--selftest）：合成夹具正反两态（逐字节差异/并行两遍不一致/并行未生效/
自动口径退化/标签或平移空转/决策摘要缺失/阈值空转/串行决策或 reason 不符/
并行线程数或函数数不符/labels 非整数/worker 数不足/编译失败/产物缺失），
另用真实编译器跑一组三后端正例；链式执行 1220 失败传播断言器自测；
显式 --cn 不存在=rc 2。

用法：
  python scripts/check_parallel_determinism.py [--cn target/cn.exe]
  python scripts/check_parallel_determinism.py --selftest [--cn <编译器>]
退出码：0=通过；1=违例/全局超时；2=环境/用法错误。
"""

import argparse
import io
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import time

# 控制台编码（中文诊断防 GBK/C locale 崩——与 check_parallel_prefix 等门禁同口径）
try:
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
except Exception:
    pass

项目根 = pathlib.Path(__file__).resolve().parent.parent
并行实现源 = 项目根 / "src" / "cn_compiler" / "codegen" / "parallel_codegen.cpp"
标准库 = 项目根 / "stdlib"

# 抽样集（tests/e2e 代表性用例·避开 78_v2/79_v2 重负载）：覆盖多函数/类/泛型
#   容器/字符串/lambda/结构体；数字=实测 IR 函数数（cn ir 计数·脚本另行机械
#   核验 >= 并行阈值，低于阈值=红，防未来用例缩水或阈值上调造成门禁空转）。
样本们 = (
    {"名": "1026_三元与方法链消费位释放", "入口": "主.cn",
     "覆盖": "IR 函数 20/类/字符串/方法链"},
    {"名": "1371_无析构类元素释放级联", "入口": "主.cn",
     "覆盖": "IR 函数 87/类/容器（向量、链表）/字符串/析构链"},
    {"名": "1064_函数返回泛型容器临时值成员访问", "入口": "主.cn",
     "覆盖": "IR 函数 58/泛型容器（向量、映射）实例化/结构体/结果类型"},
    {"名": "1403_lambda引用捕获字符串类型链", "入口": "主.cn",
     "覆盖": "IR 函数 20/字符串/lambda 捕获/结构体"},
)

# 三后端全覆盖（compile 只产汇编文本，任意宿主可交叉生成）
目标们 = ("win-x64", "linux-x86_64", "linux-arm64")

# 显式线程口径：4/4（同口径复跑）/2（不同线程数）/1（串行基线）；
#   另加「自动」口径（不注入变量=生产默认 hardware_concurrency）。
并行线程数 = "4"
第二并行线程数 = "2"
串行线程数 = "1"

编译超时 = 180          # 单次编译上限（秒）
全局超时秒 = 300        # 整轮门禁 deadline（秒）
决策前缀 = "[codegen-parallel]"
必需决策字段 = ("parallel", "functions", "threads", "workers", "labels",
                "shifted", "reason")
# reason 白名单（解析期校验：未知原因=决策通道语义漂移，拒绝静默放过）
允许原因 = {"eligible", "threads-serial", "below-threshold", "constant-pool-miss"}

全局截止时刻 = None


def 设置全局截止():
    global 全局截止时刻
    全局截止时刻 = time.monotonic() + 全局超时秒


def 超全局截止():
    return 全局截止时刻 is not None and time.monotonic() >= 全局截止时刻


def 剩余超时秒():
    if 全局截止时刻 is None:
        return 编译超时
    return max(1, min(编译超时, int(全局截止时刻 - time.monotonic())))


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


def 核数信息():
    """返回 (可用于判定的核数, 说明)。os.cpu_count()=None → 按单核处理并注明。"""
    核数 = os.cpu_count()
    if 核数 is None:
        return 1, "无法判定核数（按单核跳过）"
    return 核数, "本机 os.cpu_count()=%d" % 核数


def 读并行阈值():
    """从并行实现源码机械读取函数数阈值；返回 (阈值|None, 错误)。"""
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
        return None, ("并行决策摘要行数=%d（期望 1；CN_CODEGEN_STATS 观测通道失效"
                      "=无法证明本次并行真实生效）" % len(行们))
    字段 = {}
    for 片段 in 行们[0].split()[1:]:
        if "=" in 片段:
            键, 值 = 片段.split("=", 1)
            字段[键] = 值
    缺失 = [键 for 键 in 必需决策字段 if 键 not in 字段]
    if 缺失:
        return None, "决策摘要缺字段 %s: %s" % (缺失, 行们[0])
    if 字段["reason"] not in 允许原因:
        return None, "决策摘要 reason 不在白名单 %s: %s" % (
            sorted(允许原因), 字段["reason"])
    return 字段, ""


def 差异描述(甲, 乙):
    """逐字节比较；返回空串=相同，否则首个差异位置描述。"""
    上限 = min(len(甲), len(乙))
    for 位置 in range(上限):
        if 甲[位置] != 乙[位置]:
            return "字节偏移 %d（长度 %d vs %d）" % (位置, len(甲), len(乙))
    if len(甲) != len(乙):
        return "前缀相同但长度不同（%d vs %d）" % (len(甲), len(乙))
    return ""


def 解析整数(文本):
    """严格十进制整数；非法=None（调用方必须显式报错，禁静默取默认）。"""
    try:
        return int(文本)
    except (TypeError, ValueError):
        return None


class 真编译器:
    """真实 cn 编译器：编译（产汇编）+ IR 函数数统计。"""

    def __init__(self, 路径):
        self.路径 = pathlib.Path(路径)
        self.函数数缓存 = {}

    def _环境(self, 线程口径):
        环境 = dict(os.environ)
        环境.update({
            "CN_NO_PROGRAM_CACHE": "1",   # 程序缓存命中跳过 codegen=对照假绿
            "CN_NO_AST_CACHE": "1",
            "CN_CODEGEN_STATS": "1",      # 1219 观测通道（仅 stderr 诊断）
        })
        # 自动口径=彻底不注入（父进程残留也必须移除），走生产默认分支
        环境.pop("CN_CODEGEN_THREADS", None)
        if 线程口径 is not None:
            环境["CN_CODEGEN_THREADS"] = str(线程口径)
        return 环境

    def _运行(self, 命令, 环境):
        """返回 (CompletedProcess|None, 错误说明)。超时/不可执行=显式错误，不抛裸异常。"""
        if 超全局截止():
            return None, "全局超时（%ds）" % 全局超时秒
        try:
            return subprocess.run(命令, cwd=str(项目根), env=环境,
                                  capture_output=True,
                                  text=True, encoding="utf-8", errors="replace",
                                  timeout=剩余超时秒()), ""
        except subprocess.TimeoutExpired:
            return None, "超时（剩余 deadline=%ds）" % 剩余超时秒()
        except OSError as 异常:
            return None, "无法执行编译器 %s: %s" % (self.路径, 异常)

    def 函数数(self, 入口):
        if 入口 in self.函数数缓存:
            return self.函数数缓存[入口]
        结果, 运行错误 = self._运行([str(self.路径), "ir", str(入口),
                                    "--stdlib", str(标准库)], self._环境(None))
        if 运行错误:
            值 = (0, "cn ir 执行失败：%s" % 运行错误)
        elif 结果.returncode != 0:
            值 = (0, "cn ir 失败 rc=%d: %s" % (
                结果.returncode, (结果.stderr or 结果.stdout).strip()[:300]))
        else:
            数量 = sum(1 for 行 in 结果.stdout.splitlines() if 行.startswith("函数 "))
            值 = (数量, "" if 数量 > 0 else "cn ir 输出未含函数行")
        self.函数数缓存[入口] = 值
        return 值

    def 编译(self, 入口, 目标, 输出, 线程口径):
        结果, 运行错误 = self._运行([str(self.路径), "compile", str(入口),
                                    "--target", 目标, "--output", str(输出),
                                    "--stdlib", str(标准库)], self._环境(线程口径))
        if 运行错误:
            return -99, "[执行失败] " + 运行错误
        return 结果.returncode, (结果.stderr or "") + (
            "" if 结果.returncode == 0 else (结果.stdout or ""))


def 并行口径断言(名, 目标, 标签, 决策, 函数数, 期望线程):
    """并行口径决策断言；返回错误列表。期望线程=None 表示自动口径（>=2）。"""
    错误 = []
    if 决策["parallel"] != "yes":
        前缀 = "自动口径" if 期望线程 is None else 标签
        错误.append("%s/%s/%s: 并行未生效（决策 parallel=%s reason=%s）——"
                    "线程数决策/常量池回退被静默改变"
                    % (名, 目标, 前缀, 决策["parallel"], 决策["reason"]))
    实际线程 = 解析整数(决策["threads"])
    if 期望线程 is None:
        if 实际线程 is None or 实际线程 < 2:
            错误.append("%s/%s/%s: 自动口径 threads=%s（期望 >=2）——生产默认"
                        "硬件并发分支退化为串行" % (名, 目标, 标签, 决策["threads"]))
    elif 决策["threads"] != 期望线程:
        错误.append("%s/%s/%s: 线程数决策不符（期望 %s，实得 %s）"
                    % (名, 目标, 标签, 期望线程, 决策["threads"]))
    实际workers = 解析整数(决策["workers"])
    if 实际线程 is None:
        # threads 本身已在上方判红；workers 无法按 threads 对照，只保留下限
        if 实际workers is None or 实际workers < 2:
            错误.append("%s/%s/%s: 实际 worker 数=%s（期望 >=2；threads 不可解析）"
                        "——线程创建失败退化被吞掉，并行未真实发生"
                        % (名, 目标, 标签, 决策["workers"]))
    elif 实际workers is None or 实际workers < 2:
        错误.append("%s/%s/%s: 实际 worker 数=%s（期望 == threads=%s 且 >=2）"
                    "——线程创建失败退化被吞掉，并行未真实发生"
                    % (名, 目标, 标签, 决策["workers"], 决策["threads"]))
    elif 实际workers != 实际线程:
        错误.append("%s/%s/%s: 实际 worker 数=%d != threads=%d——少建线程/"
                    "线程创建失败退化被吞掉，并行未按决策线程数真实发生"
                    % (名, 目标, 标签, 实际workers, 实际线程))
    if 决策["functions"] != str(函数数):
        错误.append("%s/%s/%s: 函数数不符（IR=%d，决策=%s）"
                    % (名, 目标, 标签, 函数数, 决策["functions"]))
    标签数 = 解析整数(决策["labels"])
    if 标签数 is None:
        错误.append("%s/%s/%s: labels 字段非整数: %s"
                    % (名, 目标, 标签, 决策["labels"]))
    elif 标签数 <= 0:
        错误.append("%s/%s/%s: 标签总量=%s（期望 >0）——标签编号平移面未被"
                    "本次样本覆盖" % (名, 目标, 标签, 决策["labels"]))
    平移量 = 解析整数(决策["shifted"])
    if 平移量 is None:
        错误.append("%s/%s/%s: shifted 字段非整数: %s"
                    % (名, 目标, 标签, 决策["shifted"]))
    elif 平移量 <= 0:
        错误.append("%s/%s/%s: 平移面覆盖量=%s（期望 >0）——无 bases!=0 的函数"
                    "进入平移面（首函数/前序无标签均不计）"
                    % (名, 目标, 标签, 决策["shifted"]))
    return 错误


def 串行口径断言(名, 目标, 决策):
    错误 = []
    if 决策["parallel"] != "no" or 决策["threads"] != 串行线程数:
        错误.append("%s/%s/串行: 串行决策不符（期望 parallel=no threads=%s，"
                    "实得 parallel=%s threads=%s）"
                    % (名, 目标, 串行线程数, 决策["parallel"], 决策["threads"]))
    elif 决策["reason"] != "threads-serial":
        错误.append("%s/%s/串行: 串行 reason=%s（期望 threads-serial）"
                    % (名, 目标, 决策["reason"]))
    if 决策["workers"] != "0":
        错误.append("%s/%s/串行: workers=%s（期望 0）"
                    % (名, 目标, 决策["workers"]))
    return 错误


def 执行对照(编译器, 目标们_, 样本清单, 阈值, 自动口径可用=None):
    """执行全部样本×目标的并行/串行对照；返回 (错误列表, 统计)。"""
    错误 = []
    统计 = {"编译次数": 0, "标签合计": 0, "平移合计": 0,
            "自动线程": None, "自动worker": None}
    if 自动口径可用 is None:
        自动口径可用 = 核数信息()[0] >= 2
    统计["自动跳过"] = not 自动口径可用
    运行们 = [("并行1", 并行线程数, 并行线程数),
              ("并行2", 并行线程数, 并行线程数),
              ("并行3", 第二并行线程数, 第二并行线程数)]
    if 自动口径可用:
        运行们.append(("自动", None, None))
    运行们.append(("串行", 串行线程数, 串行线程数))
    for 样本 in 样本清单:
        名 = 样本["名"]
        入口 = 样本["入口路径"]
        if 超全局截止():
            错误.append("全局超时（%ds）——中止执行" % 全局超时秒)
            return 错误, 统计
        if not 入口.exists():
            错误.append("%s: 样本入口不存在: %s" % (名, 入口))
            continue
        函数数, 推理错误 = 编译器.函数数(入口)
        if 推理错误:
            错误.append("%s: IR 函数数不可得——%s" % (名, 推理错误))
            continue
        if 函数数 < 阈值:
            错误.append("%s: IR 函数数 %d < 并行阈值 %d——样本无法覆盖并行路径"
                        "（门禁将空转，拒绝通过）" % (名, 函数数, 阈值))
            continue
        for 目标 in 目标们_:
            with tempfile.TemporaryDirectory(prefix="cnpd_") as 临时目录:
                产出 = {}
                for 标签, 线程口径, 期望线程 in 运行们:
                    if 超全局截止():
                        错误.append("全局超时（%ds）——中止执行" % 全局超时秒)
                        return 错误, 统计
                    输出 = pathlib.Path(临时目录) / (标签 + ".asm")
                    rc, 错误输出 = 编译器.编译(入口, 目标, 输出, 线程口径)
                    统计["编译次数"] += 1
                    if rc != 0:
                        错误.append("%s/%s/%s: 编译失败 rc=%d: %s" % (
                            名, 目标, 标签, rc, 错误输出.strip()[:300]))
                        continue
                    if not 输出.exists():
                        错误.append("%s/%s/%s: 未生成汇编产物 %s" % (
                            名, 目标, 标签, 输出))
                        continue
                    决策, 决策错误 = 解析决策(错误输出)
                    if 决策错误:
                        错误.append("%s/%s/%s: %s" % (名, 目标, 标签, 决策错误))
                        continue
                    if 标签 == "串行":
                        错误.extend(串行口径断言(名, 目标, 决策))
                    else:
                        错误.extend(并行口径断言(名, 目标, 标签, 决策, 函数数,
                                               期望线程))
                        线程数 = 解析整数(决策["threads"]) or 0
                        worker数 = 解析整数(决策["workers"]) or 0
                        统计["标签合计"] += max(解析整数(决策["labels"]) or 0, 0)
                        统计["平移合计"] += max(解析整数(决策["shifted"]) or 0, 0)
                        if 线程口径 is None:
                            统计["自动线程"] = 线程数
                            统计["自动worker"] = worker数
                    产出[标签] = 输出.read_bytes()
                需要的 = [标签 for 标签, _, _ in 运行们]
                if any(标签 not in 产出 for 标签 in 需要的):
                    continue  # 本目标已有错误（编译失败/产物缺失/决策缺失）
                基准 = 产出["并行1"]
                for 标签 in ("并行2", "并行3") + \
                        (("自动",) if 自动口径可用 else ()):
                    差异 = 差异描述(基准, 产出[标签])
                    if 差异:
                        错误.append("%s/%s: 两次并行产物不一致（并行1 vs %s）"
                                    "——%s（非确定性输出）" % (名, 目标, 标签, 差异))
                for 标签 in ("并行1", "并行2", "并行3") + \
                        (("自动",) if 自动口径可用 else ()):
                    差异 = 差异描述(产出[标签], 产出["串行"])
                    if 差异:
                        错误.append("%s/%s: 并行与串行产物不一致（%s vs 串行）"
                                    "——%s（1145 逐字节一致承诺被破坏）"
                                    % (名, 目标, 标签, 差异))
    return 错误, 统计


def 打印错误(错误):
    print("[并行确定性] FAIL：%d 项" % len(错误), flush=True)
    for 项 in 错误:
        print("  - " + 项, flush=True)


def 主检查(编译器, 目标们_):
    设置全局截止()
    阈值, 阈值错误 = 读并行阈值()
    if 阈值错误:
        print("[并行确定性] FAIL：" + 阈值错误, flush=True)
        return 1
    if not 标准库.is_dir():
        打印错误(["标准库目录不存在: %s" % 标准库])
        return 1
    核数, 核数说明 = 核数信息()
    自动口径可用 = 核数 >= 2
    if not 自动口径可用:
        print("[并行确定性] 跳过自动口径：%s——无法验证生产默认硬件并发"
              "（显式线程数 4/2/1 仍全覆盖；不假装通过）" % 核数说明, flush=True)
    清单 = [{"名": 样本["名"],
             "入口路径": 项目根 / "tests" / "e2e" / 样本["名"] / 样本["入口"],
             "覆盖": 样本["覆盖"]} for 样本 in 样本们]
    起点 = time.time()
    错误, 统计 = 执行对照(编译器, 目标们_, 清单, 阈值, 自动口径可用)
    耗时 = time.time() - 起点
    if 统计.get("自动跳过"):
        自动摘要 = "跳过（%s）" % 核数说明
    else:
        自动摘要 = "threads=%s/workers=%s" % (统计["自动线程"], 统计["自动worker"])
    if 错误:
        打印错误(错误)
        print("[并行确定性] 耗时 %.2fs（样本=%d 目标=%d 编译=%d 自动口径=%s）"
              % (耗时, len(清单), len(目标们_), 统计["编译次数"], 自动摘要),
              flush=True)
        return 1
    # 1220：链式失败传播负测先跑，全部通过后才打印唯一 PASS（避免「先 PASS 后 FAIL」）
    失败脚本 = 项目根 / "scripts" / "check_parallel_failure.py"
    if not 失败脚本.exists():
        print("[并行确定性] FAIL：1220 失败传播脚本缺失: %s" % 失败脚本, flush=True)
        return 1
    print("[并行确定性] 链式执行 1220 工作线程失败传播负测……", flush=True)
    if subprocess.run([sys.executable, str(失败脚本), "--cn", str(编译器.路径)],
                      cwd=str(项目根)).returncode != 0:
        print("[并行确定性] FAIL：失败传播负测未通过（明细见上方输出）", flush=True)
        return 1
    print("[并行确定性] PASS ✓ 样本=%d 目标=%d 编译=%d 标签合计=%d 平移合计=%d "
          "自动口径=%s 耗时=%.2fs"
          % (len(清单), len(目标们_), 统计["编译次数"], 统计["标签合计"],
             统计["平移合计"], 自动摘要, 耗时), flush=True)
    return 0


# ============ 自测（合成夹具正反两态 + 真实编译正例） ============

class 假编译器:
    """自测夹具：按调用序号/线程口径返回构造的产物与决策行。"""

    def __init__(self, 内容函数, 决策函数, 函数数=20):
        self.内容函数 = 内容函数
        self.决策函数 = 决策函数
        self.函数数_ = 函数数
        self.调用序号 = 0

    def 函数数(self, 入口):
        if self.函数数_ <= 0:
            return 0, "夹具 IR 失败"
        return self.函数数_, ""

    def 编译(self, 入口, 目标, 输出, 线程口径):
        self.调用序号 += 1
        rc, 诊断 = self.内容函数(self.调用序号, 线程口径, pathlib.Path(输出))
        return rc, 诊断 + self.决策函数(self.调用序号, 线程口径)


def 正常决策(标签数="42", 平移量="17"):
    def 决策(序号, 线程口径):
        if 线程口径 == "1":
            return ("%s parallel=no functions=20 threads=1 workers=0 labels=0 "
                    "shifted=0 reason=threads-serial\n" % 决策前缀)
        线程 = "20" if 线程口径 is None else str(线程口径)
        return ("%s parallel=yes functions=20 threads=%s workers=%s labels=%s "
                "shifted=%s reason=eligible\n"
                % (决策前缀, 线程, 线程, 标签数, 平移量))
    return 决策


def 固定内容(内容=b"ASM\n"):
    def 写(序号, 线程口径, 路径):
        路径.write_bytes(内容)
        return 0, ""
    return 写


def 并行夹具():
    """并行口径正/反夹具（正常/非确定性/未生效/自动退化/标签/线程/函数/worker）。"""
    返回 = []
    返回.append(("正常并行（正例）", 假编译器(固定内容(), 正常决策()), None))
    # 两次并行不一致（第 2 遍不同）
    def 两遍不一致(序号, 线程口径, 路径):
        路径.write_bytes(b"ASM1\n" if 序号 == 1 or 线程口径 == "1" else b"ASM2\n")
        return 0, ""
    返回.append(("并行两遍非确定性", 假编译器(两遍不一致, 正常决策()),
                 "两次并行产物不一致"))
    # 显式并行未生效
    def 决策未生效(序号, 线程口径):
        if 线程口径 == "1":
            return ("%s parallel=no functions=20 threads=1 workers=0 labels=0 "
                    "shifted=0 reason=threads-serial\n" % 决策前缀)
        return ("%s parallel=no functions=20 threads=%s workers=1 labels=0 "
                "shifted=0 reason=below-threshold\n"
                % (决策前缀, 线程口径))
    返回.append(("并行未生效（决策=no）", 假编译器(固定内容(), 决策未生效),
                 "并行未生效"))
    # 自动口径退化为串行（生产默认分支）
    def 决策自动退化(序号, 线程口径):
        if 线程口径 == "1":
            return ("%s parallel=no functions=20 threads=1 workers=0 labels=0 "
                    "shifted=0 reason=threads-serial\n" % 决策前缀)
        if 线程口径 is None:
            return ("%s parallel=no functions=20 threads=1 workers=1 labels=0 "
                    "shifted=0 reason=threads-serial\n" % 决策前缀)
        return 正常决策()(序号, 线程口径)
    返回.append(("自动口径退化为串行", 假编译器(固定内容(), 决策自动退化),
                 "自动口径"))
    # 标签/平移面空转
    返回.append(("标签总量空转（labels=0）",
                 假编译器(固定内容(), 正常决策(标签数="0")), "标签总量=0"))
    返回.append(("平移面覆盖空转（shifted=0）",
                 假编译器(固定内容(), 正常决策(平移量="0")), "平移面覆盖量=0"))
    # 并行线程数不符（显式 4 报 2）
    def 决策线程不符(序号, 线程口径):
        if 线程口径 == "4":
            return ("%s parallel=yes functions=20 threads=2 workers=2 labels=42 "
                    "shifted=17 reason=eligible\n" % 决策前缀)
        return 正常决策()(序号, 线程口径)
    返回.append(("并行线程数决策不符", 假编译器(固定内容(), 决策线程不符),
                 "线程数决策不符"))
    # 函数数不符
    def 决策函数数不符(序号, 线程口径):
        if 线程口径 == "1":
            return 正常决策()(序号, 线程口径)
        return ("%s parallel=yes functions=19 threads=%s workers=%s labels=42 "
                "shifted=17 reason=eligible\n"
                % (决策前缀, 线程口径, 线程口径))
    返回.append(("并行函数数不符", 假编译器(固定内容(), 决策函数数不符),
                 "函数数不符"))
    # labels 非整数
    def 决策标签非整(序号, 线程口径):
        if 线程口径 == "1":
            return 正常决策()(序号, 线程口径)
        return ("%s parallel=yes functions=20 threads=%s workers=%s labels=abc "
                "shifted=17 reason=eligible\n"
                % (决策前缀, 线程口径, 线程口径))
    返回.append(("labels 非整数", 假编译器(固定内容(), 决策标签非整),
                 "labels 字段非整数"))
    # worker 数不足（线程创建失败被吞）
    def 决策worker不足(序号, 线程口径):
        if 线程口径 == "1":
            return 正常决策()(序号, 线程口径)
        return ("%s parallel=yes functions=20 threads=%s workers=1 labels=42 "
                "shifted=17 reason=eligible\n" % (决策前缀, 线程口径))
    返回.append(("worker 数不足", 假编译器(固定内容(), 决策worker不足),
                 "实际 worker 数=1"))
    # 少建一线程（threads=4 实启动 3）
    def 决策worker少建(序号, 线程口径):
        if 线程口径 == "1":
            return 正常决策()(序号, 线程口径)
        return ("%s parallel=yes functions=20 threads=%s workers=3 labels=42 "
                "shifted=17 reason=eligible\n" % (决策前缀, 线程口径))
    返回.append(("少建线程（workers=3/threads=4）",
                 假编译器(固定内容(), 决策worker少建),
                 "实际 worker 数=3 != threads=4"))
    return 返回


def 串行边界夹具():
    """串行边界、决策摘要通道与基础设施反例夹具。"""
    返回 = []
    # 串行产物单字节差异
    def 串行差异(序号, 线程口径, 路径):
        路径.write_bytes(b"ASX\n" if 线程口径 == "1" else b"ASM\n")
        return 0, ""
    返回.append(("串行产物单字节差异", 假编译器(串行差异, 正常决策()),
                 "并行与串行产物不一致"))
    # 决策摘要缺失
    def 无决策(序号, 线程口径):
        return ""
    返回.append(("决策摘要缺失", 假编译器(固定内容(), 无决策), "决策摘要行数=0"))
    # 阈值空转
    返回.append(("函数数低于并行阈值", 假编译器(固定内容(), 正常决策(), 函数数=3),
                 "样本无法覆盖并行路径"))
    # 串行决策不符 / 串行 reason 不符
    def 串行决策不符(序号, 线程口径):
        if 线程口径 == "1":
            return ("%s parallel=yes functions=20 threads=1 workers=2 labels=42 "
                    "shifted=17 reason=eligible\n" % 决策前缀)
        return 正常决策()(序号, 线程口径)
    返回.append(("串行决策不符", 假编译器(固定内容(), 串行决策不符),
                 "串行决策不符"))

    def 串行reason不符(序号, 线程口径):
        if 线程口径 == "1":
            return ("%s parallel=no functions=20 threads=1 workers=0 labels=0 "
                    "shifted=0 reason=below-threshold\n" % 决策前缀)
        return 正常决策()(序号, 线程口径)
    返回.append(("串行 reason 不符", 假编译器(固定内容(), 串行reason不符),
                 "串行 reason"))
    # 编译失败 / 产物缺失
    def 编译失败(序号, 线程口径, 路径):
        return 1, "夹具编译失败"
    返回.append(("编译失败", 假编译器(编译失败, 正常决策()), "编译失败"))

    def 无产物(序号, 线程口径, 路径):
        return 0, ""
    返回.append(("产物缺失", 假编译器(无产物, 正常决策()), "未生成汇编产物"))
    return 返回


def 自测用例():
    """返回全部合成夹具（并行面 + 串行边界面）；None=期望绿。"""
    return 并行夹具() + 串行边界夹具()


def 跑自测(显式cn):
    设置全局截止()
    if 显式cn and not pathlib.Path(显式cn).exists():
        print("[selftest] FAIL：显式 --cn 路径不存在: %s" % 显式cn, flush=True)
        return 2
    失败 = 0
    阈值, 阈值错误 = 读并行阈值()
    if 阈值错误:
        print("[selftest] FAIL：" + 阈值错误, flush=True)
        return 1
    基准样本 = {"名": "夹具", "入口路径": 项目根 / "AGENTS.md", "覆盖": "自测"}
    for 用例名, 夹具, 期望 in 自测用例():
        if 超全局截止():
            print("[selftest] FAIL：全局超时（%ds）" % 全局超时秒, flush=True)
            return 1
        错误, _ = 执行对照(夹具, ["win-x64"], [基准样本], 阈值, 自动口径可用=True)
        if 期望 is None:
            通过 = not 错误
            说明 = "绿" if 通过 else "期望绿实红: %s" % 错误
        else:
            通过 = any(期望 in 项 for 项 in 错误)
            说明 = "红（命中「%s」）" % 期望 if 通过 else \
                "期望红未命中「%s」: %s" % (期望, 错误)
        print("[selftest] %s %s：%s" % ("✓" if 通过 else "✗", 用例名, 说明),
              flush=True)
        if not 通过:
            失败 += 1

    # 单核模拟：自动口径必须显式跳过（4/2/1 三口径仍跑），不得把跳过当通过
    错误, 统计 = 执行对照(假编译器(固定内容(), 正常决策()), ["win-x64"],
                        [基准样本], 阈值, 自动口径可用=False)
    通过 = not 错误 and bool(统计.get("自动跳过"))
    print("[selftest] %s 单核模拟：自动口径跳过（4/2/1 三口径）%s"
          % ("✓" if 通过 else "✗",
             "：绿" if 通过 else "：%s" % 错误), flush=True)
    if not 通过:
        失败 += 1

    核数, 核数说明 = 核数信息()
    自动可用 = 核数 >= 2
    if not 自动可用:
        print("[selftest] 自动口径跳过：%s——真实段按 4/2/1 三口径"
              % 核数说明, flush=True)
    编译器 = 找编译器(显式cn)
    真实段已跑 = False
    if 编译器 is None:
        print("[selftest] 黄字：未找到 cn 产物——跳过真实编译正例"
              "（逻辑反例已全跑；L1 构建后/云端会覆盖真实段）", flush=True)
    else:
        真实段已跑 = True
        真实样本 = {"名": 样本们[0]["名"],
                    "入口路径": 项目根 / "tests" / "e2e" / 样本们[0]["名"] /
                               样本们[0]["入口"],
                    "覆盖": 样本们[0]["覆盖"]}
        错误, 统计 = 执行对照(真编译器(编译器), list(目标们), [真实样本], 阈值,
                            自动口径可用=自动可用)
        if 错误:
            失败 += 1
            print("[selftest] ✗ 真实编译正例（%s×三后端）FAIL：" % 编译器, flush=True)
            for 项 in 错误:
                print("  - " + 项, flush=True)
        else:
            自动摘要 = ("自动口径 threads=%s/workers=%s"
                        % (统计["自动线程"], 统计["自动worker"])) \
                if 自动可用 else "自动口径跳过"
            print("[selftest] ✓ 真实编译正例（%s×三后端·编译 %d 次·%s）"
                  % (编译器, 统计["编译次数"], 自动摘要), flush=True)

    if 失败:
        print("[selftest] FAIL：%d 项" % 失败, flush=True)
        return 1
    # 1220：链式失败传播门禁的断言器自测（纯合成·无需编译器）——判别力不随演进退化
    失败脚本 = 项目根 / "scripts" / "check_parallel_failure.py"
    if not 失败脚本.exists():
        print("[selftest] ✗ 1220 失败传播脚本缺失: %s" % 失败脚本, flush=True)
        return 1
    if subprocess.run([sys.executable, str(失败脚本), "--selftest"],
                      cwd=str(项目根)).returncode != 0:
        print("[selftest] ✗ 1220 失败传播断言器自测 FAIL", flush=True)
        return 1
    print("[selftest] ✓ 1220 失败传播断言器自测", flush=True)
    if 真实段已跑:
        结尾 = "合成正反两态 + 真实编译正例"
        if not 自动可用:
            结尾 += "（4/2/1 三口径；自动口径跳过）"
    else:
        结尾 = "仅合成正反两态；真实编译正例未跑——未找到 cn 产物"
    print("[selftest] PASS ✓（%s）" % 结尾, flush=True)
    return 0


def main():
    解析器 = argparse.ArgumentParser(
        description="并行 codegen 产物确定性门禁（并行 vs 串行逐字节 + 多线程数自一致）")
    解析器.add_argument("--cn", default="", help="cn 编译器路径（默认自动探测）")
    解析器.add_argument("--selftest", action="store_true",
                        help="正反两态自测（合成夹具 + 真实编译正例）")
    参数 = 解析器.parse_args()
    if 参数.selftest:
        return 跑自测(参数.cn)
    编译器 = 找编译器(参数.cn)
    if 编译器 is None:
        print("[并行确定性] FAIL：未找到 cn 编译器（--cn 指定或先构建 target/cn）",
              flush=True)
        return 2
    return 主检查(真编译器(编译器), list(目标们))


if __name__ == "__main__":
    sys.exit(main())
