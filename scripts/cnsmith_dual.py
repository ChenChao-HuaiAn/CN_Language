#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 第四轨道·同源码双编译行为对拍（1130·104 篇行为级静默错值网）。

背景（任务 #1130·P0）：882 形态扫描对「逻辑看着对、优先级条件缺半句」结构性失明
（1047 实录：宿主内建限定名被同名用户函数截胡，静态扫描抓不到，靠人工探针撞出）。
本轨道把「同源码双编译 stdout 对拍」做成**独立可复用轨道**：输入一批 CN 源码
（采样生成 / 目录批量），分别用宿主编译器与 v2p 编译运行，逐行比对
stdout + 退出码（stderr 按需），差异即「分叉候选」——建议立案清单随报告产出
（不自动写服务端台账，人类/编排者据此立项）。

模块分工（1130 审查拆分支债·1109 体量红线再拆·#1556 缓存轴再拆）：
  cnsmith_dual.py          判定 + 单样本双编译 + 轨道入口 + CLI（本文件）
  cnsmith_dual_samples.py  样本准备（随机/矩阵/内置/目录批量 + 用例配置）
  cnsmith_dual_report.py   判定/原因词表 + 差异报告 + 建议立案 + 种子派生提示
  cnsmith_dual_cache.py    缓存对拍轴（#1556·宿主 AST 缓存二次编译对拍·v2 不参与）
  cnsmith_dual_selftest.py 自校验（判定层正/反例 + 报告结构 + 可选端到端）

判定（闭集·miss 不可静默）：
  一致         双侧编译通过、运行 rc 相同、stdout 逐行相等
  行为分叉     双侧均过但 rc/stdout（或开启时 stderr）不同
  拒绝不一致   一侧过一侧拒（原因细分：宿主拒v2放 / 宿主过v2拒 / 宿主放行负测）
  工具链/产物  汇编/链接/符号自检/未产 asm/超时/崩溃等（候选·原因闭集标注）
  宿主基线无效 宿主侧拒绝/崩溃/退出码与配置不符（无法作对拍基准）
  跳过         时间预算耗尽 / 用例配置不对齐（**与 P0-1 联动：轨道状态=无效、
               exit 2——无结果不算过，禁止当绿**）
  抽查异常     单样本执行异常（已隔离，不中断整轮）

  **v2 成功双条件（#1108 类假绿防线）**：v2 侧「放行」= rc=0 **且** asm 产物
  落盘存在。rc=0 但无 asm 归「工具链/产物异常·v2未生成asm」，既不当作放行，
  也不当作拒绝（禁把无产物混成「双侧等效拒绝」假绿）。

输入源（可组合）：
  --对拍样本 N   随机采样（cnsmith_gen·主种子+11）
  --对拍矩阵 N   矩阵样本均匀抽样（cnsmith_matrix·骨架×类型·主种子+3 仅注释）
  --对拍内置 N   内置执法锁面（确定性·每轮必测；超族上限显式提示）
  --对拍目录 D   目录批量（逗号分隔）。含 `主.cn` 或 `v2闭环.txt`/`双编译对照.txt`
                 的目录=一个多文件样本（整树复制·**读取用例配置**：源文件/退出码/
                 借链/供给/负测/诊断锚；配置不对齐显式跳过并计无效）；
                 否则目录内每个顶层 .cn 各为独立样本。

报告（轮目录/对拍/）：
  差异报告.json  结构化：源码路径、两侧 rc/stdout/stderr、首差异行、判定/原因、
                 建议立案、种子派生区间；跳过与不对齐单列
  差异报告.md    人读：判定表 + 候选明细 + 建议立案标题 + 跳过清单 + 种子派生提示

常态化入口（CN-Smith 轨道入口，遵循既有 CLI 风格；--轨道/--track 同义）：
  python scripts/cnsmith_run.py --轨道 对拍                       # 默认采样面
  python scripts/cnsmith_run.py --track 对拍 --对拍目录 tests/e2e/144_v2_泛型星号实参
  python scripts/cnsmith_run.py --轨道 对拍 --对拍样本 24 --对拍矩阵 16 \
      --对拍内置 5 --对拍预算秒 1800                              # 周采样推荐参数

新特性轮强制（契约变更轮使用说明）：
  改动语义/语法/内建符号/代码生成后，集成前**必须**跑一轮
  `python scripts/cnsmith_run.py --轨道 对拍 --对拍预算秒 1800`
  （并行度默认 8；新特性涉及的 E2E 用例目录用 --对拍目录 一并纳入，
  如 `--对拍目录 tests/e2e/<新用例>,tests/e2e/<相邻族>`）——同一份源码
  两侧行为不一致即分叉，分叉候选须立案或当场修复后方可集成。

自校验：python scripts/cnsmith_dual.py --selftest
  （判定层正/反例 + 轨道级跳过无效断言 + 报告结构 + 建议立案；
   `--实跑` 附加真实双编译端到端）。
树锁：本 CLI 直跑编译/运行（共享 target/audit2）——与 cnsmith_run 同款
  gate_lock.树锁运行入口（555·同树并发互踩防线）。
"""
import argparse
import concurrent.futures
import importlib.util
import os
import pathlib
import shutil
import sys
import time

根 = pathlib.Path(__file__).resolve().parent.parent
默认归档根 = 根 / "target" / "cnsmith_runs"

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import cnsmith_dual_report as 报告  # noqa: E402  （scripts 路径注入后导入）
from cnsmith_dual_report import (  # noqa: E402,F401  （re-export 供 cnsmith_run 消费）
    判定_一致, 判定_行为分叉, 判定_拒绝不一致, 判定_工具链, 判定_宿主基线无效,
    判定_跳过, 判定_抽查异常,
    原因_一致, 原因_RC不一致, 原因_STDOUT不一致, 原因_STDERR不一致,
    原因_宿主拒v2放, 原因_宿主过v2拒, 原因_宿主拒绝负测配置,
    原因_双侧拒因异, 双侧拒因一致否,
    原因_v2产错误产物, 原因_汇编失败, 原因_链接失败, 原因_符号自检失败,
    原因_v2未产asm, 原因_v2未达运行, 原因_宿主基线, 原因_宿主崩溃,
    原因_配置退出码不符, 原因_配置诊断锚不符, 原因_配置不对齐,
    原因_预算跳过, 原因_抽查异常,
    截断, 建议立案标题, 记录速记, 写差异报告,
)

# 样本准备/内置锁面/用例配置已拆分至 cnsmith_dual_samples（1109 体量红线
#   「先拆再加」；本文件保留判定/单样本双编译/轨道入口/CLI）。以下导入即
#   re-export：cnsmith_run 与自测模块经 cnsmith_dual 访问这些符号（接口不变）。
from cnsmith_dual_samples import (  # noqa: E402,F401
    内置对拍样本, 对拍随机种子偏移, 对拍矩阵种子偏移, 负向种子偏移, 随机种子偏移,
    种子派生表, 净化名, 随机样本, 矩阵样本, 内置样本, 读取用例配置, 目录样本, 准备样本,
)
# 缓存对拍轴（#1556）：循环导入安全——本模块与 cnsmith_dual_cache 顶层互不取
#   对方属性（缓存轴的 核心.xxx 只在其函数体内解析），运行期属性已完整。
import cnsmith_dual_cache as 缓存轴  # noqa: E402,F401  （轨道_对拍 挂轴+re-export）


# =====================================================================
# 通用工具
# =====================================================================
def 读文本(p, 默认=""):
    try:
        return pathlib.Path(p).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 默认


def 探测目标平台():
    if os.name == "nt":
        return "win-x64"
    import platform as _p
    m = _p.machine().lower()
    if m in ("aarch64", "arm64"):
        return "linux-arm64"
    return "linux-x86_64"


def 解析编译器路径(路径文本):
    """宿主编译器路径归一：Path 化 + 相对根 + win `.exe` 探测（审查 P2-1）。

    探测结果显式进报告「编译器.路径」（不静默换路径）。"""
    p = pathlib.Path(路径文本)
    if not p.is_absolute():
        p = 根 / p
    if not p.exists() and p.with_suffix(".exe").exists():
        p = p.with_suffix(".exe")
    return p


def 默认编译器相对路径(系统=None):
    """平台感知默认宿主编译器相对路径（#1617）：win=target/cn.exe；其余=target/cn。

    根因（1617 实证）：win 主树 target/ 下存在跨机同步回的 linux ELF `cn`，
    旧默认 target/cn 使 CreateProcess 撞 WinError 193（%1 不是有效的 Win32
    应用程序）四轨道全废。系统参数可注入——selftest 断言映射纯函数用。"""
    import platform as _p
    if (系统 or _p.system()) == "Windows":
        return "target/cn.exe"
    return "target/cn"


def 编译器二进制格式(路径):
    """读文件头判二进制格式（#1617 启动探测数据源）：'elf' / 'pe' / ''。

    ''=不存在/不可读/不足 4 字节/无法识别（脚本壳等）——不拦，交由 OS
    执行期自报错（不属本防线职责，miss 不静默的豁免面在调用方显形）。"""
    try:
        with open(路径, "rb") as f:
            头 = f.read(4)
    except OSError:
        return ""
    if 头 == b"\x7fELF":
        return "elf"
    if 头[:2] == b"MZ":
        return "pe"
    return ""


def 编译器平台诊断(路径文本, 系统=None, 平台=None):
    """#1617 启动探测：编译器二进制格式与当前平台不符 → (诊断, False)；否则 ("", True)。

    禁止带着异平台编译器跑完全轨（win 撞 linux ELF=WinError 193 四轨道全废；
    linux 侧 exec PE 同理废轮）。文件不存在交调用方既有「编译器不存在」分支显形；
    格式无法识别不拦（OS 执行期自报错）。系统/平台可注入——selftest 正反例
    纯函数断言用（平台缺省给通用字样，调用方传 探测目标平台() 更精确）。"""
    import platform as _p
    p = pathlib.Path(路径文本)
    if not p.exists():
        return "", True
    格式 = 编译器二进制格式(p)
    if not 格式:
        return "", True
    系统 = 系统 or _p.system()
    if 系统 == "Windows" and 格式 == "elf":
        return ("编译器 %s 是 ELF（linux 产物）与当前平台 %s 不符——请用 --cn 指定"
                "本机构建产物（如 target/cn.exe）" % (p, 平台 or "win"), False)
    if 系统 != "Windows" and 格式 == "pe":
        return ("编译器 %s 是 PE（win 产物）与当前平台 %s 不符——请用 --cn 指定"
                "本机构建产物（如 target/cn）" % (p, 平台 or "linux"), False)
    return "", True


def 加载e2e运行器():
    """惰性加载 tests/e2e/run_e2e.py——复用 v2p 就绪（指纹缓存）/运行防线（内存上限+
    进程树）/执行v2闭环（v2 编译→汇编→链接→运行→断言）与 E2E 双编译对照同源判据。
    测试设施同层复用（不重复实现）；顶层仅 stdout 重配置+心跳容错，无重副作用。"""
    路径 = 根 / "tests" / "e2e" / "run_e2e.py"
    sys.path.insert(0, str(路径.parent))
    spec = importlib.util.spec_from_file_location("cnsmith_dual_e2e", 路径)
    模 = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(模)
    return 模


def 跳过记录(样本, 预算, 原因, 详情):
    """单样本显式跳过（预算耗尽/配置不对齐——与 P0-1 联动：轨道状态无效）。"""
    return {"样本": 样本["名"], "来源": 样本["来源"], "源码": str(样本["源"]),
            "源": str(样本["源"]), "整树": bool(样本.get("整树")),
            "判定": 判定_跳过, "原因": 原因,
            "详情": 详情 if 原因 != 原因_预算跳过
            else "时间预算 %ds 耗尽（未执行·无结果不算过）" % 预算,
            "宿主": {}, "v2": {}, "首差异": None, "建议立案": ""}


def 预算跳过记录(样本们, 预算):
    """预算耗尽样本集合 → 跳过记录（不当绿；供轨道循环与 selftest 共用）。"""
    return [跳过记录(s, 预算, 原因_预算跳过, "")
            for s in 样本们]


# =====================================================================
# 判定
# =====================================================================
def 首行差异(甲文本, 乙文本, 类别="stdout"):
    """逐行比对（rstrip 后全等）→ None=一致；否则首差异 dict（1 基行号+两侧文本）。"""
    甲 = [行.rstrip() for 行 in (甲文本 or "").splitlines()]
    乙 = [行.rstrip() for 行 in (乙文本 or "").splitlines()]
    for i in range(max(len(甲), len(乙))):
        左 = 甲[i] if i < len(甲) else "<缺行>"
        右 = 乙[i] if i < len(乙) else "<缺行>"
        if 左 != 右:
            return {"行": i + 1, "类别": 类别, "甲侧": "宿主", "乙侧": "v2",
                    "甲": 截断(左, 500), "乙": 截断(右, 500)}
    return None


def 警告面一致否(甲编译输出, 乙编译输出):
    """1366②：编译期警告行集比对（else 立法警告发 stderr·执法面盲区）。

    提取双侧编译输出中含「警告」的行（进度/缓存行不参与）·行集不同=不一致。
    返回 (是否一致, 甲首警告, 乙首警告)。"""
    甲警告 = [ln for ln in (甲编译输出 or "").splitlines() if "警告" in ln]
    乙警告 = [ln for ln in (乙编译输出 or "").splitlines() if "警告" in ln]
    if 甲警告 == 乙警告:
        return True, "", ""
    return False, (甲警告[0] if 甲警告 else "（无警告）"), \
        (乙警告[0] if 乙警告 else "（无警告）")


def 比对两侧(宿主明细, v2明细, 比stderr=False):
    """双侧均编译通过时的行为判定。返回 (判定, 详情, 首差异或 None, 原因)。"""
    if 宿主明细.get("rc") != v2明细.get("rc"):
        return (判定_行为分叉,
                "退出码不一致：宿主 rc=%s ｜ v2 rc=%s" % (
                    宿主明细.get("rc"), v2明细.get("rc")),
                {"行": 0, "类别": "rc", "甲侧": "宿主", "乙侧": "v2",
                 "甲": "rc=%s" % 宿主明细.get("rc"), "乙": "rc=%s" % v2明细.get("rc")},
                原因_RC不一致)
    首 = 首行差异(宿主明细.get("stdout", ""), v2明细.get("stdout", ""), "stdout")
    if 首 is not None:
        return (判定_行为分叉,
                "stdout 第 %d 行起不一致：宿主 %r ｜ v2 %r" % (
                    首["行"], 首["甲"], 首["乙"]), 首, 原因_STDOUT不一致)
    if 比stderr:
        首 = 首行差异(宿主明细.get("stderr", ""), v2明细.get("stderr", ""), "stderr")
        if 首 is not None:
            return (判定_行为分叉,
                    "stderr 第 %d 行起不一致：宿主 %r ｜ v2 %r" % (
                        首["行"], 首["甲"], 首["乙"]), 首, 原因_STDERR不一致)
    return (判定_一致, "rc+stdout（%s）双侧一致"
            % ("+stderr" if 比stderr else "stderr未参与"), None, 原因_一致)


def 是否纯v2负路径(样本):
    """纯 v2 负路径用例（v2闭环.txt + `退出码=负`）：v2 按配置**必须拒绝**，
    宿主无等价基线——对拍一律跳过（与宿主 rc 无关；审查第二轮 264_v2 实录：
    宿主放行 rc=0 时曾被误判「宿主过v2拒」并生成假立案）。"""
    配置 = 样本.get("配置") or {}
    return (样本.get("配置类型") == "v2闭环.txt"
            and 配置.get("预期退出码") is None)


def 宿主基线判定(宿主, 配置, 平台, 锚判定=None, 名称="", 注入路径们=None):
    """宿主侧能否作对拍基准（对齐 E2E 双编译对照「宿主语义锚定」层）。

    返回 (原因, 详情)；空原因=可作基线（含「无配置且宿主拒绝」——走双侧拒绝判定）。
    负测诊断锚检查复用 run_e2e.负测锚.锚命中（913 防线：剥诊断行首路径/位置回显，
    防用例目录名/路径满足锚串=假绿）；`锚判定` 缺省=内部错误显形（禁静默放行）。"""
    配置 = 配置 or {}
    if 宿主["编译rc"] != 0:
        if 配置.get("预期编译失败"):
            锚 = 配置.get("诊断锚", "")
            if 锚:
                if 锚判定 is None:
                    return (原因_配置诊断锚不符,
                            "内部错误：诊断锚检查缺少 913 防线（负测锚）")
                if not 锚判定(锚, 宿主.get("编译输出", ""), 注入路径们 or [], 名称):
                    return 原因_配置诊断锚不符, "宿主已拒绝但缺期望诊断锚[%s]" % 锚
            return "", ""                       # 负测宿主拒绝=预期行为
        if 配置:
            return 原因_宿主基线, "宿主拒绝（rc=%d）但用例配置预期编译成功" % 宿主["编译rc"]
        return "", ""                           # 无配置 → 双侧拒绝一致性判定
    if 配置 and 配置.get("预期编译失败"):
        return 原因_宿主基线, "宿主放行负测源码（配置预期编译失败）"
    if 宿主.get("rc") is not None and 宿主["rc"] < 0:
        return 原因_宿主崩溃, "宿主产物崩溃 rc=%d（无法作对拍基准）" % 宿主["rc"]
    if 配置 and 配置.get("预期退出码") is not None and 宿主.get("rc") is not None:
        期望 = 配置["预期退出码"] if 平台 == "win-x64" else 配置["预期退出码"] % 256
        if 宿主["rc"] != 期望:
            return 原因_配置退出码不符, "宿主退出码=%s 与配置期望 %s 不符" % (宿主["rc"], 期望)
    return "", ""


def v2判定放行(rc, 产asm):
    """v2 单侧「放行/成功」判定 = **双条件**（rc=0 且 asm 落盘存在）。

    #1108 实录：v2 写 asm 失败时仍返回 rc=0（假绿），下游把 rc=0 当编译成功。
    采样判定不得只看 rc：rc=0 无 asm 是工具链/产物异常（显形），既不是「放行」，
    也不是「拒绝」——禁把无产物当拒绝混成「双侧等效拒绝」的假绿。
    返回 (类别, 原因, 详情)；类别 ∈ 放行 / 假绿 / 错误产物 / 拒绝。"""
    if rc == 0 and 产asm:
        return "放行", "", ""
    if rc == 0:
        return ("假绿", 原因_v2未产asm,
                "v2 编译返回成功（rc=0）但未生成 asm——#1108 类假绿"
                "（rc=0 不等同成功·双条件缺产物）")
    if 产asm:
        return ("错误产物", 原因_v2产错误产物,
                "v2 拒绝（rc=%d）后仍产出 asm（错误产物纪律回归）" % rc)
    return "拒绝", "", ""


def v2阶段判定(说明):
    """v2 未达运行阶段的原因归类（审查 P2-4：v2p 未生成 asm 归工具链、拒绝按方向）。

    返回 (判定, 原因, 详情)。"""
    说明 = 说明 or ""
    if "-3.5 " in 说明 or "-5.5 " in 说明:
        return 判定_工具链, 原因_符号自检失败, "v2 符号自检失败：" + 截断(说明, 300)
    if "-4 " in 说明:
        return 判定_工具链, 原因_汇编失败, "v2 汇编阶段失败：" + 截断(说明, 300)
    if "-5 " in 说明:
        return 判定_工具链, 原因_链接失败, "v2 链接阶段失败：" + 截断(说明, 300)
    if "-3 v2p 运行失败" in 说明:
        return (判定_拒绝不一致, 原因_宿主过v2拒,
                "宿主通过而 v2 编译拒绝：" + 截断(说明, 300))
    if "-3 v2p 未生成" in 说明:
        return (判定_工具链, 原因_v2未产asm,
                "v2 编译返回成功但未生成 asm（错误产物纪律）：" + 截断(说明, 300))
    return 判定_工具链, 原因_v2未达运行, "v2 闭环未达运行阶段：" + 截断(说明, 300)


# =====================================================================
# 单样本双编译
# =====================================================================
def 准备登记目录(样本, 登记, 运行器):
    """把样本源码复制进隔离登记目录（整树样本=镜像源目录；单文件=复制为 主.cn，
    同名 .input/.args 随行——运行期注入数据与 E2E 同口径）。返回入口路径。"""
    if 登记.exists():
        shutil.rmtree(登记)
    if 样本["整树"]:
        # target/.git 是脏产物/版本库面，不进快照；.expected 由本轨道自行生成
        运行器.拷贝用例树干净(
            样本["根"], 登记,
            ignore=shutil.ignore_patterns(".git", "target", "*.expected"))
        return 登记 / 样本.get("入口名", "主.cn")
    登记.mkdir(parents=True, exist_ok=True)
    入口 = 登记 / "主.cn"
    shutil.copy2(样本["源"], 入口)
    for 后缀 in (".input", ".args"):
        伴 = 样本["源"].with_suffix(后缀)
        if 伴.exists():
            shutil.copy2(伴, 入口.with_suffix(后缀))
    # 审查 P2-5：平铺目录样本同样随行用例级配置（编译选项/运行栈上限）——
    #   与整树样本同口径，防两侧编译/运行参数漂移
    for 伴名 in ("编译选项.txt", "运行栈上限.txt"):
        伴 = 样本["源"].parent / 伴名
        if 伴.exists():
            shutil.copy2(伴, 登记 / 伴名)
    return 入口


def 配置对齐问题(样本, 登记):
    """整树样本的配置可用性校验：源文件/供给源缺失或配置解析失败=显式不对齐。"""
    问题 = 样本.get("配置问题") or ""
    if 问题:
        return 问题
    配置 = 样本.get("配置") or {}
    源文件们 = 配置.get("源文件们") or [样本.get("入口名", "主.cn")]
    缺源 = [f for f in 源文件们 if not (登记 / f).is_file()]
    if 缺源:
        return "配置不对齐：源文件缺失 %s" % "、".join(缺源)
    缺供 = [f for f in 配置.get("供给源们", []) if not (登记 / f).is_file()]
    if 缺供:
        return "配置不对齐：供给源缺失 %s" % "、".join(缺供)
    return ""


def 宿主一侧(ctx, 登记, 名, 入口名, 环境变量=None):
    """宿主编译+运行（真实生产链路 + 用例编译选项）。返回明细 dict。

    环境变量（#1556 缓存对拍轴注入面·None=不注入）：透传 run_e2e 运行命令的
    env 合并点（{**os.environ, **注入}）——缓存轴以独立 CN_AST_CACHE_DIR/
    CN_AST_CACHE_STATS/CN_AST_CACHE_VERIFY 驱动同一生产编译链路。"""
    运行器, 平台 = ctx["运行器"], ctx["平台"]
    后缀 = ".exe" if 平台 == "win-x64" else ""
    exe = ctx["用例输出"] / (名 + ".宿主" + 后缀)
    入口 = 登记 / 入口名
    选项 = 运行器.查找编译选项文件(登记)              # 审查 P2-10：与 v2 侧对齐
    编译 = 运行器.运行命令(
        [str(ctx["cn"]), "build", str(入口), "--target", 平台, "--output", str(exe)]
        + 选项, 根, 超时秒数=运行器.编译超时秒数, 环境变量=环境变量)
    明细 = {"编译rc": 编译.returncode,
            "编译输出": 截断((编译.stdout or "") + (编译.stderr or ""), 2000),
            # #1556：编译期 stderr 原文单独保留（[AST 缓存] 统计行/验证行的解析源——
            #   编译输出 键是 stdout+stderr 截断混合面，正则须对未混流原文解析）
            "编译stderr": 编译.stderr or ""}
    if 编译.returncode != 0:
        return 明细
    if not exe.exists():
        明细["编译rc"] = -1
        明细["编译输出"] = "编译返回成功但未生成可执行文件（错误产物纪律回归）"
        return 明细
    输入文件 = 运行器.查找输入文件(入口)
    输入文本 = 读文本(输入文件) if 输入文件 is not None else ""
    参数列表 = 运行器.查找参数文件(入口)
    运行 = 运行器.运行命令(
        [str(exe)] + 参数列表, 根, 输入文本,
        超时秒数=运行器.运行超时秒数,
        栈上限MB=运行器.查找运行栈上限MB(登记))
    明细.update(rc=运行.returncode, stdout=运行.stdout or "", stderr=运行.stderr or "")
    return 明细


def v2负编译一次(ctx, 登记, 名, 入口名):
    """v2p 负编译断言（宿主已拒绝 → 双侧应等效拒绝）。返回 {rc, 产asm, 输出}。"""
    运行器, v2p = ctx["运行器"], ctx["v2p"]
    工作 = ctx["用例输出"] / (名 + ".v2neg")
    if 工作.exists():
        shutil.rmtree(工作)
    运行器.拷贝用例树干净(
        登记, 工作,
        ignore=shutil.ignore_patterns(".git", "target", "*.expected"))
    (工作 / "target").mkdir(parents=True, exist_ok=True)
    运行器.建stdlib软链(工作, ctx["平台"])
    asm = 工作 / "target" / ("v2asm.asm" if ctx["平台"] == "win-x64" else "v2asm.s")
    if asm.exists():
        asm.unlink()
    选项 = 运行器.查找编译选项文件(工作)              # 与宿主/正路径同口径
    r = 运行器.运行命令([str(v2p), str((工作 / 入口名).resolve()), ctx["平台"]] + 选项,
                      工作, 内存上限MB=运行器.内存上限MB默认,
                      超时秒数=运行器.编译超时秒数)
    return {"rc": r.returncode, "产asm": asm.exists(),
            "输出": 截断((r.stdout or "") + (r.stderr or ""), 2000)}


def _宿主拒绝分支(ctx, 记录, 样本, 登记, 配置, 源文件们, 入口名, 宿主):
    """宿主拒绝 → 双侧拒绝等价性判定（无配置样本 / 负测配置）。

    v2 侧「放行」= rc=0 **且** asm 落盘（#1108 类假绿防线·双条件）；rc=0 无
    asm 归工具链/产物异常，不进入运行面、也不算双侧拒绝。"""
    平台 = ctx["平台"]
    v2 = v2负编译一次(ctx, 登记, 记录["样本"], 入口名)
    类别, v2原因, v2详情 = v2判定放行(v2["rc"], v2["产asm"])
    编译文 = {"放行": "放行(rc=0+asm)",
            "假绿": "rc=0 但未产 asm（假绿）",
            "错误产物": "拒(rc=%d)但产 asm" % v2["rc"],
            "拒绝": "拒(rc=%d)" % v2["rc"]}[类别]
    记录["v2"] = {
        "编译": 编译文,
        "rc": None, "stdout": "", "stderr": "",
        "编译输出": 截断(v2["输出"], 2000), "产asm": v2["产asm"],
    }
    if 类别 == "假绿":
        记录.update(判定=判定_工具链, 原因=v2原因, 详情=v2详情)
        记录["首差异"] = {"行": 0, "类别": "v2 产物", "甲侧": "宿主",
                        "乙侧": "v2", "甲": "拒(rc=%d)" % 宿主["编译rc"],
                        "乙": "rc=0 但未产 asm（假绿）"}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    if 类别 == "错误产物":
        记录.update(判定=判定_工具链, 原因=v2原因, 详情=v2详情)
        记录["首差异"] = {"行": 0, "类别": "错误产物", "甲侧": "宿主",
                        "乙侧": "v2", "甲": "拒(rc=%d)" % 宿主["编译rc"],
                        "乙": "拒但产 asm"}
    elif 类别 == "放行":
        # v2 放行（rc=0+asm）：补齐**运行面**（宿主拒绝无输出基线；断言输出=False
        #   防 -6a 噪音）。注意 执行v2闭环 的签名前置步骤无条件查找 .expected
        #   （编译日志锚行通道），故仍写空文件占位——否则 FileNotFoundError
        #   （1130 实测教训）。
        (登记 / 入口名).with_suffix(".expected").write_text("", encoding="utf-8")
        明细 = {}
        状态, 说明 = ctx["运行器"].执行v2闭环(
            ctx["cn"], 登记, ctx["用例输出"], False, 平台, 源文件们,
            0, 配置.get("链接v2pobj", False), 配置.get("供给源们", []), False, 明细)
        if 明细:
            记录["v2"].update(
                rc=明细.get("rc"),
                stdout=截断(明细.get("stdout", "")),
                stderr=截断(明细.get("stderr", "")),
                stdout行数=len((明细.get("stdout") or "").splitlines()),
                stderr行数=len((明细.get("stderr") or "").splitlines()),
                闭环状态=状态, 说明=截断(说明, 800))
        记录.update(判定=判定_拒绝不一致, 原因=原因_宿主拒v2放,
                    详情="宿主拒绝而 v2 放行（双侧拒绝不等效）"
                         + ("；v2 产物运行 rc=%s 输出%d行" % (
                             记录["v2"].get("rc"),
                             记录["v2"].get("stdout行数", 0))
                            if 明细 else "；v2 产物未跑起来（%s）" % 截断(说明, 200)))
        记录["首差异"] = {"行": 0, "类别": "编译判据", "甲侧": "宿主",
                        "乙侧": "v2", "甲": "拒(rc=%d)" % 宿主["编译rc"],
                        "乙": "放行(rc=0)" + (
                            "→运行 rc=%s" % 记录["v2"].get("rc") if 明细 else "")}
    else:
        # #1679：双侧都拒 ≠ 等效——比对首诊断拒因签名（a1c 实录：宿主语义层
        #   错维误拦 vs v2 解析层不认语法，拒因/阶段完全不同曾被判「一致」假绿）。
        #   提取不到首诊断的一侧不参与比对（维持一致·详情带签名供人工复核）。
        拒因一致, 甲签名, 乙签名 = 双侧拒因一致否(
            宿主.get("编译输出", ""), v2["输出"])
        if 拒因一致:
            记录.update(判定=判定_一致, 原因=原因_一致,
                        详情="双侧等效拒绝（宿主 rc=%d ｜ v2 rc=%d）"
                             % (宿主["编译rc"], v2["rc"]))
        else:
            记录.update(判定=判定_拒绝不一致, 原因=原因_双侧拒因异,
                        详情="双侧拒绝但拒因不同（零分叉：诊断不一致=分叉）"
                             "——宿主首诊断=%r ｜ v2 首诊断=%r" % (甲签名, 乙签名))
            记录["首差异"] = {"行": 0, "类别": "拒因", "甲侧": "宿主",
                            "乙侧": "v2", "甲": 甲签名, "乙": 乙签名}
    记录["建议立案"] = 建议立案标题(记录)
    return 记录


def _双侧运行分支(ctx, 记录, 配置, 源文件们, 入口名, 宿主, 登记):
    """双侧均编译通过 → 运行级 rc/stdout（stderr 按需）对拍。"""
    平台 = ctx["平台"]
    # 宿主 stdout 作为 .expected（执行v2闭环 的运行级比对口径与 E2E 同源）
    (登记 / 入口名).with_suffix(".expected").write_text(
        宿主.get("stdout", ""), encoding="utf-8")
    明细 = {}
    状态, 说明 = ctx["运行器"].执行v2闭环(
        ctx["cn"], 登记, ctx["用例输出"], False, 平台, 源文件们,
        宿主["rc"], 配置.get("链接v2pobj", False), 配置.get("供给源们", []), True, 明细)
    记录["v2"] = {"编译": "通过" if 明细 else "失败",
                  "rc": 明细.get("rc"),
                  "stdout": 截断(明细.get("stdout", "")),
                  "stderr": 截断(明细.get("stderr", "")),
                  "stdout行数": len((明细.get("stdout") or "").splitlines()),
                  "stderr行数": len((明细.get("stderr") or "").splitlines()),
                  "闭环状态": 状态, "说明": 截断(说明, 800)}
    if 明细:
        # 1366②（警告面执法盲区·1022c 第4问）：编译期警告行比对（else 立法
        #   警告发 stderr·宿主静默 v2 警告曾判「一致绿」）——提取双侧编译输出
        #   警告行（含「警告」前缀）逐行比对；进度/缓存行不参与。
        警告面一致, 甲首警告, 乙首警告 = 警告面一致否(
            记录["宿主"].get("编译输出", ""), 记录["v2"].get("编译输出", ""))
        if not 警告面一致:
            首警告差 = {"行": 0, "类别": "警告面", "甲侧": "宿主", "乙侧": "v2",
                    "甲": 甲首警告, "乙": 乙首警告}
            记录.update(判定=判定_行为分叉,
                        原因="编译警告面不一致（else 立法警告执法面·1366②）",
                        详情="编译警告行集不同——首差=%r vs %r"
                             % (甲首警告, 乙首警告),
                        首差异=首警告差)
            记录["建议立案"] = 建议立案标题(记录)
            return 记录
        判定, 详情, 首, 原因 = 比对两侧(记录["宿主"], 记录["v2"], ctx["比stderr"])
        记录.update(判定=判定, 详情=详情, 首差异=首, 原因=原因)
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    判定, 原因, 详情 = v2阶段判定(说明)
    记录.update(判定=判定, 原因=原因, 详情=详情)
    记录["首差异"] = {"行": 0, "类别": "v2 阶段", "甲侧": "宿主",
                    "乙侧": "v2", "甲": "通过", "乙": 详情[:300]}
    记录["建议立案"] = 建议立案标题(记录)
    return 记录


def 对拍一例(ctx, 样本):
    """单样本双侧对拍 → 记录 dict（结构化差异报告的一行）。"""
    名, 平台 = 样本["名"], ctx["平台"]
    登记 = ctx["用例根"] / 名
    准备登记目录(样本, 登记, ctx["运行器"])
    配置 = 样本.get("配置") or {}
    源文件们 = 配置.get("源文件们") or [样本.get("入口名", "主.cn")]
    入口名 = 源文件们[0]
    记录 = {"样本": 名, "来源": 样本["来源"], "源码": str(样本["源"]),
            "源": str(样本["源"]), "整树": bool(样本.get("整树")),
            "入口": 入口名,
            "配置": 样本.get("配置类型", "") if 样本.get("配置") else "",
            "配置类型": 样本.get("配置类型", "") if 样本.get("配置") else "",
            "判定": "", "原因": "", "详情": "", "首差异": None, "建议立案": ""}
    问题 = 配置对齐问题(样本, 登记)
    if 问题:
        return dict(跳过记录(样本, 0, 原因_配置不对齐, 问题),
                    源码=str(样本["源"]), 入口=入口名,
                    配置=记录["配置"], 配置类型=记录["配置类型"])
    if 是否纯v2负路径(样本):
        # 纯 v2 负路径：v2 按配置必须拒绝，宿主无等价基线——一律跳过（与宿主 rc 无关）
        return dict(跳过记录(
            样本, 0, 原因_配置不对齐,
            "纯 v2 负路径用例（v2闭环.txt·退出码=负）无宿主等价基线——对拍不适用"),
            源码=str(样本["源"]), 入口=入口名,
            配置=记录["配置"], 配置类型=记录["配置类型"])
    # ===== 宿主侧 =====
    宿主 = 宿主一侧(ctx, 登记, 名, 入口名)
    记录["宿主"] = {
        "编译rc": 宿主["编译rc"], "rc": 宿主.get("rc"),
        "stdout": 截断(宿主.get("stdout", "")), "stderr": 截断(宿主.get("stderr", "")),
        "stdout行数": len((宿主.get("stdout") or "").splitlines()),
        "stderr行数": len((宿主.get("stderr") or "").splitlines()),
        "编译输出": 宿主.get("编译输出", ""),
    }
    # 纯 v2 正路径用例（v2闭环.txt）宿主侧拒绝=对拍无宿主基线 → 显式跳过计无效
    if 宿主["编译rc"] != 0 and 记录["配置类型"] == "v2闭环.txt":
        return dict(跳过记录(
            样本, 0, 原因_配置不对齐,
            "纯 v2 用例（v2闭环.txt）宿主侧拒绝 rc=%d——对拍无宿主基线，轨道不适用"
            % 宿主["编译rc"]),
            源码=str(样本["源"]), 入口=入口名,
            配置=记录["配置"], 配置类型=记录["配置类型"], 宿主=记录["宿主"])
    基线原因, 基线详情 = 宿主基线判定(
        宿主, 配置, 平台, ctx["运行器"].负测锚.锚命中, 名,
        [登记 / 入口名, ctx["用例输出"] / (名 + ".宿主" + (".exe" if 平台 == "win-x64" else ""))])
    if 基线原因:
        记录.update(判定=判定_宿主基线无效, 原因=基线原因, 详情=基线详情)
        记录["首差异"] = {"行": 0, "类别": "宿主基线", "甲侧": "宿主", "乙侧": "-",
                        "甲": 基线详情[:200], "乙": "-"}
        记录["v2"] = {"编译": "未执行", "说明": "宿主基线无效"}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    if 宿主["编译rc"] != 0:
        return _宿主拒绝分支(ctx, 记录, 样本, 登记, 配置, 源文件们, 入口名, 宿主)
    if 宿主.get("rc") is None:
        记录.update(判定=判定_宿主基线无效, 原因=原因_宿主基线,
                    详情="宿主编译通过但无运行结果（显形）")
        记录["v2"] = {"编译": "未执行", "说明": "宿主基线无效"}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    return _双侧运行分支(ctx, 记录, 配置, 源文件们, 入口名, 宿主, 登记)


def 安全对拍(ctx, 样本):
    """单样本异常隔离（审查 P2-9）：异常转「抽查异常」记录，不中断整轮。"""
    try:
        return 对拍一例(ctx, 样本)
    except Exception as e:
        记录 = {"样本": 样本["名"], "来源": 样本["来源"], "源码": str(样本["源"]),
                "源": str(样本["源"]), "整树": bool(样本.get("整树")),
                "入口": 样本.get("入口名", "主.cn"), "配置": "", "配置类型": "",
                "判定": 判定_抽查异常, "原因": 原因_抽查异常,
                "详情": "抽查执行异常：%r" % (e,), "宿主": {}, "v2": {},
                "首差异": None, "建议立案": ""}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录


# =====================================================================
# 轨道入口（供 cnsmith_run.py 调用）
# =====================================================================
def 轨道结果状态(一致, 候选, 跳过, 样本数):
    """轨道状态契约（P0-1）：跳过>0（预算/配置不对齐）=无效（exit 2）——不当绿。"""
    if 跳过 > 0:
        return "无效", ("覆盖不足：跳过 %d/%d（预算耗尽/配置不对齐·无结果不算过）"
                       "·一致 %d·候选 %d" % (跳过, 样本数, 一致, 候选))
    return "完成", "一致 %d/%d·分叉候选 %d" % (一致, 样本数, 候选)


def 轨道_对拍(a, 轮目录, 种子, 运行器, v2p, 平台):
    """CN-Smith 轨道「对拍」：准备样本→预热→并行双编译→结构化差异报告。"""
    if v2p is None:
        return {"状态": "无效", "说明": "v2p 未就绪（对拍无 v2 侧即无意义·显形）："
                                        + a.v2侧未就绪原因}
    样本们, 错 = 准备样本(a, 轮目录, 种子, 运行器)
    if 错:
        return {"状态": "无效", "说明": 错}
    if not 样本们:
        return {"状态": "无效", "说明": "对拍样本 0 个（无结果不算过）"}
    ctx = {"运行器": 运行器, "cn": a.cn, "平台": 平台, "v2p": v2p,
           "用例输出": 轮目录 / "对拍输出", "用例根": 轮目录 / "对拍用例",
           "比stderr": bool(a.对拍比stderr)}
    ctx["用例输出"].mkdir(parents=True, exist_ok=True)
    ctx["用例根"].mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    起点 = time.monotonic()          # 预算计时必须与截止同源（混用 epoch/monotonic=永不触发）
    预算 = max(0, a.对拍预算秒)
    截止 = (起点 + 预算) if 预算 > 0 else None
    并发 = max(1, a.jobs)
    结果们 = []
    # ===== 预热：串行跑首个样本（建立宿主 runtime obj / v2p 共享产物缓存；
    #   并发首建竞态与 cache miss 并发写踩踏防线）=====
    print("  [对拍] 预热样本 %s" % 样本们[0]["名"], flush=True)
    结果们.append(安全对拍(ctx, 样本们[0]))
    待跑 = 样本们[1:]
    完成数 = 1
    while 待跑:
        if 截止 is not None and time.monotonic() >= 截止:
            结果们.extend(预算跳过记录(待跑, 预算))
            待跑 = []
            break
        批 = 待跑[:并发]
        待跑 = 待跑[并发:]
        with concurrent.futures.ThreadPoolExecutor(max_workers=len(批)) as 池:
            批结果 = list(池.map(lambda s: 安全对拍(ctx, s), 批))
        结果们.extend(批结果)
        完成数 += len(批结果)
        print("  [对拍] 进度 %d/%d（%.1fs）" % (完成数, len(样本们), time.time() - t0),
              flush=True)
    结果们.sort(key=lambda r: r["样本"])
    # ===== 缓存对拍轴（#1556）：宿主 AST 缓存「二次编译对拍」——同源码同编译器
    #   连跑两次 build+运行（独立 CN_AST_CACHE_DIR·首编写缓存→命中跑挂
    #   CN_AST_CACHE_VERIFY）。v2p 不参与本轴（另一套编译链·无 CN_AST_CACHE_*
    #   环境面——边界见 cnsmith_dual_cache 模块 docstring）；分叉候选与跳过
    #   计入本轨道统一判定面（跳过>0=无效·禁当绿）。
    缓存汇总 = {"启用": False, "说明": "缓存对拍=关"}
    if getattr(a, "缓存对拍", "开") != "关":
        ctx["缓存根"] = 轮目录 / "对拍缓存"
        ctx["缓存根"].mkdir(parents=True, exist_ok=True)
        ctx["并发"] = 并发
        缓存记录们, 缓存汇总 = 缓存轴.轨道_缓存对拍(
            ctx, 样本们, 结果们, 上限=getattr(a, "缓存对拍样本", 12),
            截止=截止, 预算=预算)
        结果们.extend(缓存记录们)      # 缓存轴记录排尾部（主轴已 sort·不混序）
    参数 = {"随机": a.对拍样本, "矩阵": a.对拍矩阵, "内置": a.对拍内置,
            "目录": a.对拍目录 or "", "预算秒": 预算, "并发": 并发,
            "比stderr": bool(a.对拍比stderr),
            "缓存对拍": 缓存汇总,
            "主种子": 种子,
            "种子来源": ("命令行 --seed" if getattr(a, "seed", 0)
                       else "时间戳（默认）"),
            "历史种子们": getattr(a, "历史种子们", None)}
    记录 = {"轮次": getattr(a, "轮次名", 轮目录.name),
            "时刻": time.strftime("%Y-%m-%d %H:%M:%S"), "平台": 平台,
            "编译器": getattr(a, "编译指纹", {})}
    json路径, md路径, 候选, 跳过, 一致 = 写差异报告(轮目录, 记录, 结果们, 参数)
    try:
        json相对 = str(pathlib.Path(json路径).relative_to(根))
    except ValueError:
        json相对 = json路径
    try:
        md相对 = str(pathlib.Path(md路径).relative_to(根))
    except ValueError:
        md相对 = md路径
    状态, 说明 = 轨道结果状态(一致, len(候选), len(跳过), len(结果们))
    return {
        "状态": 状态, "样本数": len(结果们), "一致": 一致,
        "命中数": len(候选), "跳过数": len(跳过),
        "清单": 候选, "结果明细": 结果们,
        "差异报告": json相对, "差异报告md": md相对,
        "缓存对拍": 缓存汇总,
        "建议立案": [r["建议立案"] for r in 候选 if r.get("建议立案")],
        "说明": 说明,
    }


# =====================================================================
# 独立 CLI（--selftest / 单跑对拍）
# =====================================================================
def 结果退出码(结果):
    """CLI 退出码契约（审查 P2-1：与 cnsmith_run 一致的红优先）。

    命中>0=红（exit 1，即使轨道因跳过置无效也先报红并打印建议立案）；
    无效=exit 2；全绿=0。"""
    if 结果.get("命中数", 0) > 0:
        return 1
    if 结果.get("状态") != "完成":
        return 2
    return 0


def 打印轨道结果(结果):
    """CLI 收尾打印 + 退出码（红优先·审查 P2-1；早期无效无差异报告字段时守卫）。

    抽出为纯函数供 selftest 覆盖 main 打印路径（第三轮审查：早期无效返回只有
    状态+说明，无条件取 差异报告 键会 KeyError 且被 exit 1 冒充红）。"""
    码 = 结果退出码(结果)
    报告行 = ("  差异报告：%s ｜ %s" % (结果["差异报告"], 结果.get("差异报告md", ""))
              if 结果.get("差异报告") else "")
    if 结果.get("命中数", 0) > 0:          # 红优先：先打印命中与建议立案
        print("[对拍] %s" % 结果.get("说明", ""))
        if 报告行:
            print(报告行)
        for 标题 in 结果.get("建议立案", []):
            print("  建议立案：%s" % 标题)
        if 结果.get("状态") != "完成":
            print("  ⚠ 覆盖不足：跳过 %d（轨道无效但命中优先判红·exit 1）"
                  % 结果.get("跳过数", 0))
        return 码
    if 结果.get("状态") != "完成":
        print("对拍无效：" + 结果.get("说明", ""))
        if 报告行:
            print(报告行)
        return 码
    print("[对拍] %s" % 结果.get("说明", ""))
    if 报告行:
        print(报告行)
    return 码


def main():
    ap = argparse.ArgumentParser(
        description="CN-Smith 第四轨道·同源码双编译 stdout 对拍（1130）")
    ap.add_argument("--selftest", action="store_true", help="自校验（判定层正/反例+报告结构）")
    ap.add_argument("--实跑", action="store_true", help="selftest 附加端到端真跑（需 v2p 就绪）")
    ap.add_argument("--编译", default=None, help="selftest 端到端用宿主编译器路径")
    # 独立单跑面（与 cnsmith_run --轨道 对拍 同构参数）
    ap.add_argument("--对拍样本", type=int, default=12)
    ap.add_argument("--对拍矩阵", type=int, default=12)
    ap.add_argument("--对拍内置", type=int, default=len(内置对拍样本))
    ap.add_argument("--对拍目录", default="")
    ap.add_argument("--对拍预算秒", type=int, default=1800)
    ap.add_argument("--对拍比stderr", dest="对拍比stderr",
                    action="store_true", default=True)
    ap.add_argument("--对拍不比stderr", dest="对拍比stderr",
                    action="store_false")
    # 缓存对拍轴（#1556·与 cnsmith_run 同构：默认开·纯文档轮可关）
    ap.add_argument("--缓存对拍", choices=("开", "关"), default="开")
    ap.add_argument("--缓存对拍样本", type=int, default=12)
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--cn", default=默认编译器相对路径(),
                    help="宿主编译器路径（相对仓库根·#1617 平台感知默认："
                         "win=target/cn.exe/其余=target/cn）")
    ap.add_argument("--归档", default=str(默认归档根))
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--详细", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        import cnsmith_dual_selftest as 自测              # 延迟导入（防循环）
        return 自测.selftest(解析编译器路径(a.编译) if a.编译 else None, a.实跑)

    class 轨道参数:                     # cnsmith_run 轨道函数所需的最小参数面
        pass
    参 = 轨道参数()
    for 键 in ("对拍样本", "对拍矩阵", "对拍内置", "对拍目录", "对拍预算秒",
              "对拍比stderr", "缓存对拍", "缓存对拍样本", "jobs"):
        setattr(参, 键, getattr(a, 键))
    参.seed = a.seed                       # 种子来源进报告（#1109 派生区间提示）
    参.历史种子们 = None                   # 独立 CLI 无归档上下文（主报告面才扫描）
    参.cn = 解析编译器路径(a.cn)
    if not 参.cn.exists():
        print("编译器不存在：%s（先构建 target/cn）" % 参.cn)
        return 2
    平台 = 探测目标平台()
    # 1617 启动探测：异平台编译器（跨机同步 ELF/PE）硬拒——禁带错编译器跑完全轨
    诊断, 允跑 = 编译器平台诊断(参.cn, 平台=平台)
    if not 允跑:
        print(诊断)
        return 2
    try:
        运行器 = 加载e2e运行器()
    except Exception as e:
        print("e2e 运行器加载失败：%r" % (e,))
        return 2
    if 平台 == "win-x64":
        就绪 = 运行器.确保v2p就绪win(参.cn, a.详细, "cnsmith-dual")
    else:
        就绪 = 运行器.确保v2p与运行时就绪(参.cn, 平台, a.详细, "cnsmith-dual")
    if 就绪[0] is None:
        print("v2p 未就绪（对拍无 v2 侧即无意义）：" + str(就绪[1])[:300])
        return 2
    参.v2侧未就绪原因 = ""
    轮次名 = time.strftime("%Y%m%d-%H%M%S") + "-对拍"
    归档根 = pathlib.Path(a.归档)
    if not 归档根.is_absolute():
        归档根 = 根 / 归档根
    轮目录 = 归档根 / 轮次名
    轮目录.mkdir(parents=True, exist_ok=True)
    参.轮次名 = 轮次名
    try:
        import subprocess
        头 = subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(根),
                            capture_output=True, text=True)
        参.编译指纹 = {"路径": str(参.cn.name), "仓库HEAD": (头.stdout or "").strip()}
    except OSError:
        参.编译指纹 = {"路径": str(参.cn.name)}
    种子 = a.seed if a.seed else int(time.strftime("%y%m%d%H%M%S"))
    结果 = 轨道_对拍(参, 轮目录, 种子, 运行器, 就绪[0], 平台)
    return 打印轨道结果(结果)


if __name__ == "__main__":
    # 555 树锁接入（与 cnsmith_run/run_e2e/gate_quick 同口径）：本入口直编译/运行
    #   （共享 target/audit2），同树并发=互踩源；--selftest --实跑 同样在锁内。
    import gate_lock
    sys.exit(gate_lock.树锁运行入口(main))
