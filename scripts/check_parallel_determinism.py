#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#1219 并行 codegen 产物确定性常态门禁（并行 vs 串行逐字节 + 并行两遍自一致）。

背景（1145）：函数级并行发射按原函数序拼接，设计承诺「并行产物与串行逐字节
一致」。该承诺当轮一次性验证过，之后无常态门禁——线程数决策、worker 池拼接序、
标签编号平移、常量池回退等任何改动都可能悄悄破坏确定性。本门禁把对照常态化：

  ① 同一输入：并行跑两遍（CN_CODEGEN_THREADS=4）+ 串行跑一遍（=1）——
     - 并行1 == 串行 逐字节（1145 设计承诺）
     - 并行1 == 并行2 逐字节（自一致）
  ② 观测通道断言（CN_CODEGEN_STATS=1·1219 增）：并行两遍必须报告
     parallel=yes 且 labels>0（证明并行路径与标签平移真实执行，而非常量池
     静默回退/阈值空转）；串行必须报告 parallel=no reason=threads-serial。
  ③ 样本非空转：`cn ir` 统计 IR 函数数，必须 >= 源码阈值
     kMinFunctionsForParallel（从 parallel_codegen.cpp 机械读取；读不到=红）。
  ④ 三后端全覆盖：win-x64 / linux-x86_64 / linux-arm64 各有独立并行实现。

红线：逐字节差异/决策不符/样本失格/观测缺失=FAIL；不提供 --allow 类放行参数。
缓存隔离：CN_NO_PROGRAM_CACHE=1（程序缓存命中会跳过代码生成，使对照假绿）+
CN_NO_AST_CACHE=1。
自测（--selftest）：合成夹具正例绿/反例必红（逐字节差异/两遍不一致/并行未生效/
标签平移空转/决策摘要缺失/阈值空转/编译失败/产物缺失），另用真实编译器跑一组
三后端正例（找不到 cn 产物时黄字跳过真实段，逻辑反例仍全跑）。

用法：
  python scripts/check_parallel_determinism.py [--cn target/cn.exe]
  python scripts/check_parallel_determinism.py --selftest [--cn <编译器>]
退出码：0=通过；1=违例；2=环境/用法错误。
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
#   容器/字符串/lambda/结构体；每例 IR 函数数须 >= 并行阈值（脚本机械核验，
#   低于阈值=红，防未来用例缩水或阈值上调造成门禁空转）。
样本们 = (
    {"名": "1026_三元与方法链消费位释放", "入口": "主.cn",
     "覆盖": "多函数（18 用户函数）/类/字符串/方法链"},
    {"名": "1371_无析构类元素释放级联", "入口": "主.cn",
     "覆盖": "类/容器（向量、链表）/字符串/析构链"},
    {"名": "1064_函数返回泛型容器临时值成员访问", "入口": "主.cn",
     "覆盖": "泛型容器（向量、映射）实例化/结构体/结果类型"},
    {"名": "1403_lambda引用捕获字符串类型链", "入口": "主.cn",
     "覆盖": "字符串/lambda 捕获/结构体"},
)

# 三后端全覆盖（compile 只产汇编文本，任意宿主可交叉生成）
目标们 = ("win-x64", "linux-x86_64", "linux-arm64")

# 并行口径固定=4 线程（显式非默认：机器核数=1 时也必须走并行路径）；
#   串行口径=CN_CODEGEN_THREADS=1（1145 既有逃生门/对拍基线）。
并行线程数 = "4"

编译超时 = 180  # 单次编译上限（秒）

决策前缀 = "[codegen-parallel]"
必需决策字段 = ("parallel", "functions", "threads", "labels", "reason")


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


class 真编译器:
    """真实 cn 编译器：编译（产汇编）+ IR 函数数统计。"""

    def __init__(self, 路径):
        self.路径 = pathlib.Path(路径)
        self.函数数缓存 = {}

    def _环境(self):
        环境 = dict(os.environ)
        环境.update({
            "CN_NO_PROGRAM_CACHE": "1",   # 程序缓存命中跳过 codegen=对照假绿
            "CN_NO_AST_CACHE": "1",
            "CN_CODEGEN_STATS": "1",      # 1219 观测通道（仅 stderr 诊断）
        })
        return 环境

    def _运行(self, 命令, 环境=None):
        try:
            return subprocess.run(命令, cwd=str(项目根), env=环境,
                                  capture_output=True,
                                  text=True, encoding="utf-8", errors="replace",
                                  timeout=编译超时)
        except subprocess.TimeoutExpired:
            return None

    def 函数数(self, 入口):
        if 入口 in self.函数数缓存:
            return self.函数数缓存[入口]
        结果 = self._运行([str(self.路径), "ir", str(入口),
                          "--stdlib", str(标准库)], self._环境())
        if 结果 is None:
            值 = (0, "cn ir 超时（%ds）" % 编译超时)
        elif 结果.returncode != 0:
            值 = (0, "cn ir 失败 rc=%d: %s" % (
                结果.returncode, (结果.stderr or 结果.stdout).strip()[:300]))
        else:
            数量 = sum(1 for 行 in 结果.stdout.splitlines() if 行.startswith("函数 "))
            值 = (数量, "" if 数量 > 0 else "cn ir 输出未含函数行")
        self.函数数缓存[入口] = 值
        return 值

    def 编译(self, 入口, 目标, 输出, 线程数):
        环境 = self._环境()
        环境["CN_CODEGEN_THREADS"] = 线程数
        结果 = self._运行([str(self.路径), "compile", str(入口),
                          "--target", 目标, "--output", str(输出),
                          "--stdlib", str(标准库)], 环境)
        if 结果 is None:
            return -99, "[超时] 编译超过 %ds" % 编译超时
        return 结果.returncode, (结果.stderr or "") + (
            "" if 结果.returncode == 0 else (结果.stdout or ""))


def 执行对照(编译器, 目标们_, 样本清单, 阈值):
    """执行全部样本×目标的并行/串行对照；返回 (错误列表, 统计)。"""
    错误 = []
    统计 = {"编译次数": 0, "标签合计": 0}
    for 样本 in 样本清单:
        名 = 样本["名"]
        入口 = 样本["入口路径"]
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
                for 标签, 线程 in (("并行1", 并行线程数),
                                  ("并行2", 并行线程数),
                                  ("串行", "1")):
                    输出 = pathlib.Path(临时目录) / (标签 + ".asm")
                    rc, 错误输出 = 编译器.编译(入口, 目标, 输出, 线程)
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
                        if 决策["parallel"] != "no" or 决策["threads"] != "1":
                            错误.append("%s/%s/串行: 决策不符（期望 parallel=no "
                                        "threads=1，实得 parallel=%s threads=%s）"
                                        % (名, 目标, 决策["parallel"], 决策["threads"]))
                        elif 决策["reason"] != "threads-serial":
                            错误.append("%s/%s/串行: reason=%s（期望 threads-serial）"
                                        % (名, 目标, 决策["reason"]))
                    else:
                        if 决策["parallel"] != "yes":
                            错误.append("%s/%s/%s: 并行未生效（决策 parallel=%s "
                                        "reason=%s）——线程数决策/常量池回退被静默改变"
                                        % (名, 目标, 标签, 决策["parallel"],
                                           决策["reason"]))
                        if 决策["threads"] != 并行线程数:
                            错误.append("%s/%s/%s: 线程数决策不符（期望 %s，实得 %s）"
                                        % (名, 目标, 标签, 并行线程数, 决策["threads"]))
                        if 决策["functions"] != str(函数数):
                            错误.append("%s/%s/%s: 函数数不符（IR=%d，决策=%s）"
                                        % (名, 目标, 标签, 函数数, 决策["functions"]))
                        try:
                            标签数 = int(决策["labels"])
                        except ValueError:
                            错误.append("%s/%s/%s: labels 字段非整数: %s"
                                        % (名, 目标, 标签, 决策["labels"]))
                            标签数 = 0
                        if 标签数 <= 0:
                            错误.append("%s/%s/%s: 标签平移量=%s（期望 >0）——"
                                        "标签编号平移面未被本次样本覆盖"
                                        % (名, 目标, 标签, 决策["labels"]))
                        统计["标签合计"] += max(标签数, 0)
                    产出[标签] = 输出.read_bytes()
                if len(产出) != 3:
                    continue  # 本目标已有错误（编译失败/产物缺失/决策缺失）
                差异 = 差异描述(产出["并行1"], 产出["串行"])
                if 差异:
                    错误.append("%s/%s: 并行与串行产物不一致——%s（1145 逐字节"
                                "一致承诺被破坏）" % (名, 目标, 差异))
                差异 = 差异描述(产出["并行1"], 产出["并行2"])
                if 差异:
                    错误.append("%s/%s: 两次并行产物不一致——%s（非确定性输出）"
                                % (名, 目标, 差异))
    return 错误, 统计


def 打印错误(错误):
    print("[并行确定性] FAIL：%d 项" % len(错误), flush=True)
    for 项 in 错误:
        print("  - " + 项, flush=True)


def 主检查(编译器, 目标们_):
    阈值, 阈值错误 = 读并行阈值()
    if 阈值错误:
        print("[并行确定性] FAIL：" + 阈值错误, flush=True)
        return 1
    if not 标准库.is_dir():
        打印错误(["标准库目录不存在: %s" % 标准库])
        return 1
    清单 = [{"名": 样本["名"],
             "入口路径": 项目根 / "tests" / "e2e" / 样本["名"] / 样本["入口"],
             "覆盖": 样本["覆盖"]} for 样本 in 样本们]
    起点 = time.time()
    错误, 统计 = 执行对照(编译器, 目标们_, 清单, 阈值)
    耗时 = time.time() - 起点
    if 错误:
        打印错误(错误)
        print("[并行确定性] 耗时 %.2fs（样本=%d 目标=%d 编译=%d）"
              % (耗时, len(清单), len(目标们_), 统计["编译次数"]), flush=True)
        return 1
    print("[并行确定性] PASS ✓ 样本=%d 目标=%d 编译=%d 标签平移合计=%d 耗时=%.2fs"
          % (len(清单), len(目标们_), 统计["编译次数"], 统计["标签合计"], 耗时),
          flush=True)
    return 0


# ============ 自测（合成夹具正反两态 + 真实编译正例） ============

class 假编译器:
    """自测夹具：按调用序号/线程数返回构造的产物与决策行。"""

    def __init__(self, 内容函数, 决策函数, 函数数=20):
        self.内容函数 = 内容函数
        self.决策函数 = 决策函数
        self.函数数_ = 函数数
        self.调用序号 = 0

    def 函数数(self, 入口):
        if self.函数数_ <= 0:
            return 0, "夹具 IR 失败"
        return self.函数数_, ""

    def 编译(self, 入口, 目标, 输出, 线程数):
        self.调用序号 += 1
        rc, 诊断 = self.内容函数(self.调用序号, 线程数, pathlib.Path(输出))
        return rc, 诊断 + self.决策函数(self.调用序号, 线程数)


def 正常决策(标签数="42"):
    def 决策(序号, 线程数):
        if 线程数 == "1":
            return "%s parallel=no functions=20 threads=1 labels=0 " \
                   "reason=threads-serial\n" % 决策前缀
        return "%s parallel=yes functions=20 threads=4 labels=%s " \
               "reason=eligible\n" % (决策前缀, 标签数)
    return 决策


def 固定内容(内容=b"ASM\n"):
    def 写(序号, 线程数, 路径):
        路径.write_bytes(内容)
        return 0, ""
    return 写


def 自测用例():
    """返回 [(用例名, 假编译器|None, 期望错误子串)]。"""
    返回 = []
    # 正例：三遍产物一致 + 决策正常
    返回.append(("正常并行（正例）", 假编译器(固定内容(), 正常决策()), None))
    # 串行产物单字节差异
    def 串行差异(序号, 线程数, 路径):
        路径.write_bytes(b"ASX\n" if 线程数 == "1" else b"ASM\n")
        return 0, ""
    返回.append(("串行产物单字节差异", 假编译器(串行差异, 正常决策()),
                 "并行与串行产物不一致"))
    # 两次并行不一致
    def 两遍不一致(序号, 线程数, 路径):
        路径.write_bytes(b"ASM1\n" if 序号 == 1 or 线程数 == "1" else b"ASM2\n")
        return 0, ""
    返回.append(("并行两遍非确定性", 假编译器(两遍不一致, 正常决策()),
                 "两次并行产物不一致"))
    # 并行未生效
    def 决策未生效(序号, 线程数):
        if 线程数 == "1":
            return "%s parallel=no functions=20 threads=1 labels=0 " \
                   "reason=threads-serial\n" % 决策前缀
        return "%s parallel=no functions=20 threads=4 labels=0 " \
               "reason=below-threshold\n" % 决策前缀
    返回.append(("并行未生效（决策=no）", 假编译器(固定内容(), 决策未生效),
                 "并行未生效"))
    # 标签平移空转
    返回.append(("标签平移空转（labels=0）",
                 假编译器(固定内容(), 正常决策(标签数="0")), "标签平移量=0"))
    # 决策摘要缺失
    def 无决策(序号, 线程数):
        return ""
    返回.append(("决策摘要缺失", 假编译器(固定内容(), 无决策), "决策摘要行数=0"))
    # 阈值空转：IR 函数数低于阈值
    返回.append(("函数数低于并行阈值", 假编译器(固定内容(), 正常决策(), 函数数=3),
                 "样本无法覆盖并行路径"))
    # 编译失败
    def 编译失败(序号, 线程数, 路径):
        return 1, "夹具编译失败"
    返回.append(("编译失败", 假编译器(编译失败, 正常决策()), "编译失败"))
    # 产物缺失
    def 无产物(序号, 线程数, 路径):
        return 0, ""
    返回.append(("产物缺失", 假编译器(无产物, 正常决策()), "未生成汇编产物"))
    return 返回


def 跑自测(显式cn):
    失败 = 0
    阈值, 阈值错误 = 读并行阈值()
    if 阈值错误:
        print("[selftest] FAIL：" + 阈值错误, flush=True)
        return 1
    基准样本 = {"名": "夹具", "入口路径": 项目根 / "AGENTS.md", "覆盖": "自测"}
    for 用例名, 夹具, 期望 in 自测用例():
        错误, _ = 执行对照(夹具, ["win-x64"], [基准样本], 阈值)
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

    编译器 = 找编译器(显式cn)
    if 编译器 is None:
        print("[selftest] 黄字：未找到 cn 产物——跳过真实编译正例"
              "（逻辑反例已全跑；L1 构建后/云端会覆盖真实段）", flush=True)
    else:
        真实样本 = {"名": 样本们[0]["名"],
                    "入口路径": 项目根 / "tests" / "e2e" / 样本们[0]["名"] /
                               样本们[0]["入口"],
                    "覆盖": 样本们[0]["覆盖"]}
        错误, 统计 = 执行对照(真编译器(编译器), list(目标们), [真实样本], 阈值)
        if 错误:
            失败 += 1
            print("[selftest] ✗ 真实编译正例（%s×三后端）FAIL：" % 编译器, flush=True)
            for 项 in 错误:
                print("  - " + 项, flush=True)
        else:
            print("[selftest] ✓ 真实编译正例（%s×三后端·编译 %d 次·标签=%d）"
                  % (编译器, 统计["编译次数"], 统计["标签合计"]), flush=True)

    if 失败:
        print("[selftest] FAIL：%d 项" % 失败, flush=True)
        return 1
    print("[selftest] PASS ✓（合成正反两态 + 真实编译正例）", flush=True)
    return 0


def main():
    解析器 = argparse.ArgumentParser(
        description="并行 codegen 产物确定性门禁（并行 vs 串行逐字节 + 并行两遍自一致）")
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
