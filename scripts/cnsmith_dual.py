#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 第四轨道·同源码双编译行为对拍（1130·104 篇行为级静默错值网）。

背景（任务 #1130·P0）：882 形态扫描对「逻辑看着对、优先级条件缺半句」结构性失明
（1047 实录：宿主内建限定名被同名用户函数截胡，静态扫描抓不到，靠人工探针撞出）。
本模块把「同源码双编译 stdout 对拍」做成**独立可复用轨道**：输入一批 CN 源码
（采样生成 / 目录批量），分别用宿主编译器与 v2p 编译运行，逐行比对
stdout + 退出码（stderr 按需），差异即「分叉候选」——建议立案清单随报告产出
（不自动写服务端台账，人类/编排者据此立项）。

判据（四态显形·miss 不可静默）：
  一致         双侧编译通过、运行 rc 相同、stdout 逐行相等
  行为分叉     双侧均过但 rc 或 stdout（或开启时 stderr）不同
  拒绝不一致   一侧过一侧拒（宿主拒 v2 放行 / 宿主过 v2 拒）
  工具链/产物  汇编/链接/超时/崩溃/未产 asm 等非语义差异（候选·需人工确认类别）
  跳过         时间预算耗尽（显式计数·禁止当绿）

输入源（可组合）：
  --对拍样本 N   随机采样（cnsmith_gen·种子+11）
  --对拍矩阵 N   矩阵样本均匀抽样（cnsmith_matrix·骨架×类型）
  --对拍内置 N   内置执法锁面（确定性·每轮必测）
  --对拍目录 D   目录批量：含 主.cn 的目录=一个多文件样本（整树复制）；
                 否则目录内每个顶层 .cn 各为独立样本（可逗号分隔多个目录）

报告（轮目录/对拍/）：
  差异报告.json  结构化：源码路径、两侧 rc/stdout/stderr、首差异行、类别、建议立案
  差异报告.md    人读：判定表 + 分叉候选明细 + 建议立案标题清单

常态化入口（CN-Smith 轨道入口，遵循既有 CLI 风格；--轨道/--track 同义）：
  python scripts/cnsmith_run.py --轨道 对拍                       # 默认采样面
  python scripts/cnsmith_run.py --track 对拍 --对拍目录 tests/e2e/01_hello
  python scripts/cnsmith_run.py --轨道 对拍 --对拍样本 24 --对拍矩阵 16 \
      --对拍内置 5 --对拍预算秒 1800                              # 周采样推荐参数

新特性轮强制（契约变更轮使用说明）：
  改动语义/语法/内建符号/代码生成后，集成前**必须**跑一轮
  `python scripts/cnsmith_run.py --轨道 对拍 --对拍预算秒 1800`
  （并行度默认 8；新特性涉及的 E2E 用例目录用 --对拍目录 一并纳入，
  如 `--对拍目录 tests/e2e/<新用例>,tests/e2e/<相邻族>`）——同一份源码
  两侧行为不一致即分叉，分叉候选须立案或当场修复后方可集成。

自校验：python scripts/cnsmith_dual.py --selftest
  （判定层正例/反例注入 + 报告结构 + 建议立案标题；给 --编译/--v2p 可选真跑）
"""
import argparse
import concurrent.futures
import importlib.util
import json
import os
import pathlib
import re
import shutil
import sys
import time

根 = pathlib.Path(__file__).resolve().parent.parent
默认归档根 = 根 / "target" / "cnsmith_runs"

# 内置对拍样本族（167）：分叉形态的确定性锁面——随机样本不保证覆盖之形态每轮必测，
#   且修复后可作回归锁。全部为**非法源码**（双侧应等效拒绝；一侧放行即不一致显形）：
#   ①缺右花括号/②文件截断/③缺右括号=167 自测现场命中（v2 解析静默吞错形态）；
#   ④移位超宽+三元左操作数=167 自测现场命中（v2 474 检查左操作数形态漏检）；
#   ⑤尾随非法字符=词法层拒绝一致性。
# 命名纪律：样本名第一段须 ASCII——run_e2e 隔离键=「首段+哈希」进 v2src/v2work
#   目录名与 v2p 入口路径（789/406 中文路径 GBK 教训）；其余段可中文（仅报告面）。
内置对拍样本 = [
    ("inner_缺右花括号",
     '函数 主() -> 整32 {\n    打印行("甲");\n    返回 0;\n'),
    ("inner_截断文件",
     '函数 主() -> 整32 {\n    打印行("甲");\n'),
    ("inner_缺右括号",
     '函数 主() -> 整32 {\n    打印行("甲";\n    返回 0;\n}\n'),
    ("inner_移位超宽_三元左操作数",
     '函数 主() -> 整32 {\n    整32 甲 = 1;\n    整32 乙 = ((甲 < 2) ? 1 : 0) >> 652;\n'
     '    打印(乙);\n    返回 0;\n}\n'),
    ("inner_尾随非法字符",
     '函数 主() -> 整32 {\n    返回 0;\n}\n@@\n'),
]

# 判定词表（报告/机读统一；新增判定须同步这里——闭集外传入即断言显形）
判定_一致 = "一致"
判定_行为分叉 = "行为分叉"
判定_拒绝不一致 = "拒绝不一致"
判定_工具链 = "工具链/产物异常"
判定_宿主基线无效 = "宿主基线无效"
判定_跳过 = "跳过"

# 单侧输出写入报告的截断上限（字符）——报告须给「两侧输出」，超限显式标注
输出截断上限 = 8000


# =====================================================================
# 通用工具
# =====================================================================
def 读文本(p, 默认=""):
    try:
        return pathlib.Path(p).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 默认


def 截断(文本, 上限=输出截断上限):
    """截断并显式标注（miss 不静默：截断本身是报告事实，不是丢数据）。"""
    文本 = 文本 or ""
    if len(文本) <= 上限:
        return 文本
    return 文本[:上限] + "\n…（截断：原文 %d 字符，上限 %d）" % (len(文本), 上限)


def 净化名(文本):
    """样本名净化：只留 ASCII 字母数字下划线（路径/产物名 ASCII 友好）。"""
    净 = re.sub(r"[^0-9A-Za-z_]", "_", 文本 or "")
    净 = re.sub(r"_+", "_", 净).strip("_")
    return 净 or "样本"


def 探测目标平台():
    if os.name == "nt":
        return "win-x64"
    import platform as _p
    m = _p.machine().lower()
    if m in ("aarch64", "arm64"):
        return "linux-arm64"
    return "linux-x86_64"


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


# =====================================================================
# 样本准备
# =====================================================================
def 随机样本(数量, 种子, 目录):
    """随机采样样本（合法程序面）。返回 (样本们, 错误说明)。"""
    样本们 = []
    if 数量 <= 0:
        return 样本们, ""
    目录.mkdir(parents=True, exist_ok=True)
    import subprocess
    r = subprocess.run([sys.executable, "scripts/cnsmith_gen.py", "--seed", str(种子 + 11),
                        "--count", str(数量), "--out", str(目录)],
                       cwd=str(根), capture_output=True, text=True, encoding="utf-8",
                       errors="replace", timeout=600)
    if r.returncode != 0:
        return 样本们, "对拍随机样本生成失败：" + ((r.stdout or "") + (r.stderr or ""))[-200:]
    for 名 in sorted(f for f in os.listdir(目录) if f.endswith(".cn")):
        样本们.append({"名": "rand_" + 名[:-3], "源": 目录 / 名,
                     "来源": "随机", "整树": False})
    return 样本们, ""


def 矩阵样本(数量, 矩阵目录, cn, 种子):
    """矩阵样本（骨架×类型·期望内嵌）。优先复用本轮已跑矩阵目录（零重复生成）。"""
    样本们 = []
    if 数量 <= 0:
        return 样本们, ""
    候 = sorted(f for f in os.listdir(矩阵目录) if f.endswith(".cn")) \
        if 矩阵目录.is_dir() else []
    if not 候:
        import subprocess
        矩阵目录.mkdir(parents=True, exist_ok=True)
        r = subprocess.run([sys.executable, "scripts/cnsmith_matrix.py", "--cn", str(cn),
                            "--out", str(矩阵目录), "--seed", str(种子 + 3)],
                           cwd=str(根), capture_output=True, text=True, encoding="utf-8",
                           errors="replace", timeout=3600)
        if not re.search(r"构件组合矩阵：绿\s*\d+\s*/\s*红\s*\d+", r.stdout or ""):
            return 样本们, "对拍矩阵样本生成无效：" + ((r.stdout or "") + (r.stderr or ""))[-200:]
        候 = sorted(f for f in os.listdir(矩阵目录) if f.endswith(".cn"))
    if len(候) > 数量:                          # 均匀抽样（骨架序均匀·不总取同一族）
        步 = (len(候) - 1) / (数量 - 1) if 数量 > 1 else 0
        候 = [候[round(i * 步)] for i in range(数量)]
    for 名 in 候:
        样本们.append({"名": "mat_" + 名[:-3], "源": 矩阵目录 / 名,
                     "来源": "矩阵", "整树": False})
    return 样本们, ""


def 内置样本(数量, 目录):
    样本们 = []
    if 数量 <= 0:
        return 样本们
    目录.mkdir(parents=True, exist_ok=True)
    for 名, 源码 in 内置对拍样本[:数量]:
        路径 = 目录 / (名 + ".cn")
        路径.write_text(源码, encoding="utf-8")
        样本们.append({"名": 名, "源": 路径, "来源": "内置", "整树": False})
    return 样本们


def 目录样本(目录们文本, 轮目录):
    """目录批量输入。含 主.cn 的目录=一个多文件样本（整树复制）；否则每个顶层 .cn
    各为独立样本。返回 (样本们, 错误说明)。"""
    样本们 = []
    if not (目录们文本 or "").strip():
        return 样本们, ""
    for 序, 目录文本 in enumerate(x.strip() for x in 目录们文本.split(",") if x.strip()):
        目录 = pathlib.Path(目录文本)
        if not 目录.is_absolute():
            目录 = 根 / 目录
        if not 目录.is_dir():
            return 样本们, "对拍目录不存在：%s" % 目录
        入口 = 目录 / "主.cn"
        if 入口.exists():
            样本们.append({"名": "dir_%03d_%s" % (序, 净化名(目录.name)) if 目录.name
                         else "dir_%03d" % 序,
                         "源": 入口, "根": 目录, "来源": "目录", "整树": True})
            continue
        文件们 = sorted(f for f in 目录.iterdir() if f.is_file() and f.suffix == ".cn")
        if not 文件们:
            return 样本们, "对拍目录内无 .cn 源码：%s" % 目录
        for 文序, 源 in enumerate(文件们):
            样本们.append({"名": "dir_%03d_%03d_%s" % (序, 文序, 净化名(源.stem)),
                         "源": 源, "根": 目录, "来源": "目录", "整树": False})
    return 样本们, ""


def 准备样本(a, 轮目录, 种子):
    """对拍样本全集 = 随机 + 矩阵 + 内置 + 目录批量。返回 (样本们, 错误说明)。"""
    样本们 = []
    随, 错 = 随机样本(a.对拍样本, 种子, 轮目录 / "对拍样本" / "随机")
    if 错:
        return [], 错
    样本们 += 随
    # 矩阵目录复用优先序：本轮矩阵轨道已跑 → 复用；否则落对拍样本/矩阵
    矩目录 = (轮目录 / "矩阵") if (轮目录 / "矩阵").is_dir() else (轮目录 / "对拍样本" / "矩阵")
    矩, 错 = 矩阵样本(a.对拍矩阵, 矩目录, a.cn, 种子)
    if 错:
        return [], 错
    样本们 += 矩
    样本们 += 内置样本(a.对拍内置, 轮目录 / "对拍样本" / "内置")
    目, 错 = 目录样本(a.对拍目录, 轮目录)
    if 错:
        return [], 错
    样本们 += 目
    # 同名去重（目录批量可能与采样名撞车）——后者加序号，防产物互踩
    已见, 去重 = set(), []
    for 样本 in 样本们:
        名 = 样本["名"]
        if 名 in 已见:
            样本["名"] = "%s_%d" % (名, len(去重))
        if 样本["名"] in 已见:              # 极端撞车（加序号仍重）→ 显式错误不静默
            return [], "对拍样本名去重失败：%s" % 样本["名"]
        已见.add(样本["名"])
        去重.append(样本)
    return 去重, ""


# =====================================================================
# 单样本双编译
# =====================================================================
def 准备登记目录(样本, 登记, 运行器):
    """把样本源码复制进隔离登记目录（整树样本=镜像源目录；单文件=复制为 主.cn，
    同名 .input/.args 随行——运行期注入数据与 E2E 同口径）。"""
    if 登记.exists():
        shutil.rmtree(登记)
    if 样本["整树"]:
        # target/.git 是脏产物/版本库面，不进快照；.expected 由本轨道自行生成
        运行器.拷贝用例树干净(
            样本["根"], 登记,
            ignore=shutil.ignore_patterns(".git", "target", "*.expected"))
        return 登记 / "主.cn"
    登记.mkdir(parents=True, exist_ok=True)
    入口 = 登记 / "主.cn"
    shutil.copy2(样本["源"], 入口)
    for 后缀 in (".input", ".args"):
        伴 = 样本["源"].with_suffix(后缀)
        if 伴.exists():
            shutil.copy2(伴, 入口.with_suffix(后缀))
    return 入口


def 宿主一侧(ctx, 登记, 名):
    """宿主编译+运行（真实生产链路）。返回明细 dict（编译失败时 rc=None）。"""
    运行器, 平台 = ctx["运行器"], ctx["平台"]
    后缀 = ".exe" if 平台 == "win-x64" else ""
    exe = ctx["用例输出"] / (名 + ".宿主" + 后缀)
    入口 = 登记 / "主.cn"
    编译 = 运行器.运行命令(
        [str(ctx["cn"]), "build", str(入口), "--target", 平台, "--output", str(exe)],
        根, 超时秒数=运行器.编译超时秒数)
    明细 = {"编译rc": 编译.returncode,
            "编译输出": 截断((编译.stdout or "") + (编译.stderr or ""), 2000)}
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


def v2负编译一次(ctx, 登记, 名, 平台):
    """v2p 负编译断言（宿主已拒绝 → 双侧应等效拒绝）。返回 {rc, 产asm, 输出}。"""
    运行器, v2p = ctx["运行器"], ctx["v2p"]
    工作 = ctx["用例输出"] / (名 + ".v2neg")
    if 工作.exists():
        shutil.rmtree(工作)
    运行器.拷贝用例树干净(
        登记, 工作,
        ignore=shutil.ignore_patterns(".git", "target", "*.expected"))
    (工作 / "target").mkdir(parents=True, exist_ok=True)
    运行器.建stdlib软链(工作, 平台)
    asm = 工作 / "target" / ("v2asm.asm" if 平台 == "win-x64" else "v2asm.s")
    if asm.exists():
        asm.unlink()
    r = 运行器.运行命令([str(v2p), str((工作 / "主.cn").resolve()), 平台], 工作,
                      内存上限MB=运行器.内存上限MB默认,
                      超时秒数=运行器.编译超时秒数)
    return {"rc": r.returncode, "产asm": asm.exists(),
            "输出": 截断((r.stdout or "") + (r.stderr or ""), 2000)}


def 首行差异(甲文本, 乙文本, 侧甲="宿主", 侧乙="v2"):
    """逐行比对（rstrip 后全等）→ None=一致；否则首差异 dict（1 基行号+两侧文本）。"""
    甲 = [行.rstrip() for 行 in (甲文本 or "").splitlines()]
    乙 = [行.rstrip() for 行 in (乙文本 or "").splitlines()]
    for i in range(max(len(甲), len(乙))):
        左 = 甲[i] if i < len(甲) else "<缺行>"
        右 = 乙[i] if i < len(乙) else "<缺行>"
        if 左 != 右:
            return {"行": i + 1, "类别": "stdout",
                    "甲侧": 侧甲, "乙侧": 侧乙,
                    "甲": 截断(左, 500), "乙": 截断(右, 500)}
    return None


def 比对两侧(宿主明细, v2明细, 比stderr=False):
    """双侧均编译通过时的行为判定。返回 (判定, 详情, 首差异或 None)。

    判据顺序（闭集枚举）：rc → stdout 逐行 → stderr（按需）。"""
    if 宿主明细.get("rc") != v2明细.get("rc"):
        return (判定_行为分叉,
                "退出码不一致：宿主 rc=%s ｜ v2 rc=%s" % (
                    宿主明细.get("rc"), v2明细.get("rc")),
                {"行": 0, "类别": "rc", "甲侧": "宿主", "乙侧": "v2",
                 "甲": "rc=%s" % 宿主明细.get("rc"), "乙": "rc=%s" % v2明细.get("rc")})
    首 = 首行差异(宿主明细.get("stdout", ""), v2明细.get("stdout", ""))
    if 首 is not None:
        return (判定_行为分叉,
                "stdout 第 %d 行起不一致：宿主 %r ｜ v2 %r" % (
                    首["行"], 首["甲"], 首["乙"]), 首)
    if 比stderr:
        首 = 首行差异(宿主明细.get("stderr", ""), v2明细.get("stderr", ""))
        if 首 is not None:
            首["类别"] = "stderr"
            return (判定_行为分叉,
                    "stderr 第 %d 行起不一致：宿主 %r ｜ v2 %r" % (
                        首["行"], 首["甲"], 首["乙"]), 首)
    return (判定_一致, "rc+stdout（%s）双侧一致" % ("+stderr" if 比stderr else "stderr未参与"),
            None)


def 建议立案标题(记录):
    """分叉候选 → 建议立案标题（人读；不自动写服务端——防误写）。"""
    if 记录["判定"] in (判定_一致, 判定_跳过):
        return ""
    首 = 记录.get("首差异") or {}
    定位 = ("首差异第 %s 行：%s=%r ｜ %s=%r" % (
        首.get("行"), 首.get("甲侧", "宿主"), 首.get("甲", "?"),
        首.get("乙侧", "v2"), 首.get("乙", "?"))
        if 首.get("类别") == "stdout" else
        "首差异=%s（%s=%r ｜ %s=%r）" % (
            首.get("类别"), 首.get("甲侧", "宿主"), 首.get("甲", "?"),
            首.get("乙侧", "v2"), 首.get("乙", "?"))
        if 首 else "无首差异定位")
    return "〔1130对拍〕%s：样本 %s（%s）宿主×v2 %s（%s）" % (
        记录["判定"], 记录["样本"], 记录["来源"],
        (记录.get("详情") or "")[:120], 定位)


def 对拍一例(ctx, 样本):
    """单样本双侧对拍 → 记录 dict（结构化差异报告的一行）。"""
    名, 平台 = 样本["名"], ctx["平台"]
    登记 = ctx["用例根"] / 名
    准备登记目录(样本, 登记, ctx["运行器"])
    记录 = {"样本": 名, "来源": 样本["来源"], "源码": str(样本["源"]),
            "源": str(样本["源"]), "整树": 样本["整树"]}
    # ===== 宿主侧 =====
    宿主 = 宿主一侧(ctx, 登记, 名)
    记录["宿主"] = {
        "编译rc": 宿主["编译rc"], "rc": 宿主.get("rc"),
        "stdout": 截断(宿主.get("stdout", "")), "stderr": 截断(宿主.get("stderr", "")),
        "stdout行数": len((宿主.get("stdout") or "").splitlines()),
        "stderr行数": len((宿主.get("stderr") or "").splitlines()),
        "编译输出": 宿主.get("编译输出", ""),
    }
    if 宿主["编译rc"] != 0:
        # 宿主拒绝 → 双侧应等效拒绝（轻量 v2 负编译断言）
        v2 = v2负编译一次(ctx, 登记, 名, 平台)
        记录["v2"] = {
            "编译": "放行(rc=0)" if v2["rc"] == 0 else "拒(rc=%d)" % v2["rc"],
            "rc": None, "stdout": "", "stderr": "",
            "编译输出": 截断(v2["输出"], 2000), "产asm": v2["产asm"],
        }
        if v2["rc"] == 0:
            # v2 放行：补齐**运行面**（任务 1130 要求双侧都跑）——宿主拒绝无输出
            #   基线，.expected 留空（判定不看输出，只取 v2 实际运行明细显形）；
            #   预期退出码传 0=只为驱动闭环跑起来，实际 rc 以明细为准。
            (登记 / "主.expected").write_text("", encoding="utf-8")
            明细 = {}
            状态, 说明 = ctx["运行器"].执行v2闭环(
                ctx["cn"], 登记, ctx["用例输出"], False, 平台, ["主.cn"],
                0, False, [], True, 明细)
            if 明细:
                记录["v2"].update(
                    rc=明细.get("rc"),
                    stdout=截断(明细.get("stdout", "")),
                    stderr=截断(明细.get("stderr", "")),
                    stdout行数=len((明细.get("stdout") or "").splitlines()),
                    stderr行数=len((明细.get("stderr") or "").splitlines()),
                    闭环状态=状态, 说明=截断(说明, 800))
            记录.update(判定=判定_拒绝不一致,
                        详情="宿主拒绝而 v2 放行（双侧拒绝不等效）"
                             + ("；v2 产物运行 rc=%s 输出%d行" % (
                                 记录["v2"].get("rc"),
                                 记录["v2"].get("stdout行数", 0))
                                if 明细 else "；v2 产物未跑起来（%s）" % 截断(说明, 200)))
            记录["首差异"] = {"行": 0, "类别": "编译判据", "甲侧": "宿主",
                            "乙侧": "v2", "甲": "拒(rc=%d)" % 宿主["编译rc"],
                            "乙": "放行(rc=0)" + (
                                "→运行 rc=%s" % 记录["v2"].get("rc") if 明细 else "")}
        elif v2["产asm"]:
            记录.update(判定=判定_工具链,
                        详情="v2 拒绝后仍产出 asm（错误产物纪律回归）")
            记录["首差异"] = {"行": 0, "类别": "错误产物", "甲侧": "宿主",
                            "乙侧": "v2", "甲": "拒(rc=%d)" % 宿主["编译rc"],
                            "乙": "拒但产 asm"}
        else:
            记录.update(判定=判定_一致, 详情="双侧等效拒绝（宿主 rc=%d ｜ v2 rc=%d）"
                        % (宿主["编译rc"], v2["rc"]), 首差异=None)
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    # ===== 宿主编译通过 =====
    if 宿主.get("rc") is None:
        记录.update(判定=判定_宿主基线无效, 详情="宿主编译通过但无运行结果（显形）")
        记录["v2"] = {"编译": "未执行", "说明": "宿主基线无效"}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    if 宿主["rc"] < 0:
        记录.update(判定=判定_宿主基线无效,
                    详情="宿主产物运行崩溃 rc=%d（无法作对拍基准·显形）" % 宿主["rc"])
        记录["v2"] = {"编译": "未执行", "说明": "宿主基线无效"}
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    # 宿主 stdout 作为 .expected（执行v2闭环 的运行级比对口径与 E2E 同源）
    (登记 / "主.expected").write_text(宿主.get("stdout", ""), encoding="utf-8")
    明细 = {}
    状态, 说明 = ctx["运行器"].执行v2闭环(
        ctx["cn"], 登记, ctx["用例输出"], False, 平台, ["主.cn"],
        宿主["rc"], False, [], True, 明细)
    记录["v2"] = {"编译": "通过" if 明细 else "失败",
                  "rc": 明细.get("rc"),
                  "stdout": 截断(明细.get("stdout", "")),
                  "stderr": 截断(明细.get("stderr", "")),
                  "stdout行数": len((明细.get("stdout") or "").splitlines()),
                  "stderr行数": len((明细.get("stderr") or "").splitlines()),
                  "闭环状态": 状态, "说明": 截断(说明, 800)}
    if 明细:
        判定, 详情, 首 = 比对两侧(记录["宿主"], 记录["v2"], ctx["比stderr"])
        记录.update(判定=判定, 详情=详情, 首差异=首)
        记录["建议立案"] = 建议立案标题(记录)
        return 记录
    # v2 未跑到运行阶段（编译/汇编/链接失败）——按说明步骤号归类（闭集枚举）
    if re.search(r"-[45](\.5)? ", 说明 or ""):
        记录.update(判定=判定_工具链, 详情="v2 工具链阶段失败：" + 截断(说明, 300))
    elif "-3 v2p 运行失败" in (说明 or ""):
        记录.update(判定=判定_行为分叉,
                    详情="宿主通过而 v2 编译拒绝：" + 截断(说明, 300))
    else:
        记录.update(判定=判定_工具链, 详情="v2 闭环未达运行阶段：" + 截断(说明, 300))
    记录["首差异"] = {"行": 0, "类别": "v2 阶段", "甲侧": "宿主",
                    "乙侧": "v2", "甲": "通过", "乙": 记录["详情"][:300]}
    记录["建议立案"] = 建议立案标题(记录)
    return 记录


def 记录速记(记录):
    """结果明细表的人读速记（报告/日志共用）。"""
    if 记录.get("判定") == 判定_跳过:
        return "跳过", "跳过"
    宿主 = 记录.get("宿主", {})
    v2 = 记录.get("v2", {})
    宿主文 = ("rc=%s 输出%d行" % (宿主.get("rc"), 宿主.get("stdout行数", 0))
              if 宿主.get("rc") is not None else
              "编译拒 rc=%s" % 宿主.get("编译rc"))
    v2文 = ("rc=%s 输出%d行" % (v2.get("rc"), v2.get("stdout行数", 0))
            if v2.get("rc") is not None else (v2.get("编译") or "?"))
    return 宿主文, v2文


# =====================================================================
# 差异报告
# =====================================================================
def 写差异报告(轮目录, 记录, 结果们, 参数):
    """结构化差异报告落盘（JSON + MD）。返回 (报告相对路径, 候选清单)。"""
    out = 轮目录 / "对拍"
    out.mkdir(parents=True, exist_ok=True)
    候选 = [r for r in 结果们 if r["判定"] not in (判定_一致, 判定_跳过)]
    跳过 = [r for r in 结果们 if r["判定"] == 判定_跳过]
    一致 = len(结果们) - len(候选) - len(跳过)
    结构 = {
        "轮次": 记录["轮次"], "时刻": 记录["时刻"], "平台": 记录["平台"],
        "编译器": 记录.get("编译器", {}),
        "参数": 参数,
        "样本数": len(结果们), "一致": 一致, "候选数": len(候选), "跳过数": len(跳过),
        "建议立案": [r["建议立案"] for r in 候选 if r.get("建议立案")],
        "候选": 候选,
        "跳过": [{"样本": r["样本"], "来源": r["来源"], "源码": r["源码"]} for r in 跳过],
        "全部结果": 结果们,
    }
    json路径 = out / "差异报告.json"
    json路径.write_text(json.dumps(结构, ensure_ascii=False, indent=1),
                        encoding="utf-8")
    md = ["# 同源码双编译 stdout 对拍报告（%s）" % 记录["轮次"], "",
          "- 平台：%s ｜ 样本 %d（一致 %d·候选 %d·跳过 %d）"
          % (记录["平台"], len(结果们), 一致, len(候选), len(跳过)),
          "- 参数：%s" % json.dumps(参数, ensure_ascii=False),
          "- 判据：双侧编译→运行→rc+stdout 逐行（stderr %s）"
          % ("参与" if 参数.get("比stderr") else "仅记录"),
          "", "## 建议立案标题（%d 件·不自动写服务端）" % len(候选)]
    if not 候选:
        md.append("零候选。")
    for r in 候选:
        md.append("- %s" % (r.get("建议立案") or "(标题生成失败)"))
    md += ["", "## 候选明细", ""]
    if not 候选:
        md.append("零候选。")
    for r in 候选:
        md += ["### %s（%s）" % (r["样本"], r["来源"]),
               "- 判定：**%s** ｜ 源码：`%s`" % (r["判定"], r["源码"]),
               "- 详情：%s" % r.get("详情", ""),
               "- 宿主：编译rc=%s rc=%s stdout=%d行 stderr=%d行" % (
                   r["宿主"].get("编译rc"), r["宿主"].get("rc"),
                   r["宿主"].get("stdout行数", 0), r["宿主"].get("stderr行数", 0)),
               "- v2：编译=%s rc=%s stdout=%d行 stderr=%d行" % (
                   r["v2"].get("编译"), r["v2"].get("rc"),
                   r["v2"].get("stdout行数", 0), r["v2"].get("stderr行数", 0))]
        首 = r.get("首差异")
        if 首:
            md.append("- 首差异（%s·第%s行）：宿主=%r ｜ v2=%r"
                      % (首.get("类别"), 首.get("行"), 首.get("甲"), 首.get("乙")))
        md += ["- 宿主 stdout：", "```", r["宿主"].get("stdout", "")[:3000], "```",
               "- v2 stdout：", "```", r["v2"].get("stdout", "")[:3000], "```"]
        if r["宿主"].get("stderr") or r["v2"].get("stderr"):
            md += ["- 宿主 stderr：", "```", r["宿主"].get("stderr", "")[:2000], "```",
                   "- v2 stderr：", "```", r["v2"].get("stderr", "")[:2000], "```"]
        if r["宿主"].get("编译输出"):
            md += ["- 宿主编译输出：", "```", r["宿主"].get("编译输出", "")[:2000], "```"]
        if r["v2"].get("编译输出"):
            md += ["- v2 编译输出：", "```", r["v2"].get("编译输出", "")[:2000], "```"]
        md.append("")
    if 跳过:
        md += ["## 跳过（时间预算·显式计数不当绿）", ""]
        for r in 跳过:
            md.append("- %s（%s）：%s" % (r["样本"], r["来源"], r.get("详情", "")))
        md.append("")
    md += ["", "## 全样本判定表", "",
           "| 样本 | 来源 | 宿主 | v2 | 判定 |", "|---|---|---|---|---|"]
    for r in 结果们:
        宿主文, v2文 = 记录速记(r)
        md.append("| %s | %s | %s | %s | %s |" % (
            r["样本"], r["来源"], 宿主文.replace("|", "/"),
            v2文.replace("|", "/"), r["判定"]))
    md.append("")
    md路径 = out / "差异报告.md"
    md路径.write_text("\n".join(md), encoding="utf-8")
    return str(json路径.relative_to(根)), str(md路径.relative_to(根)), 候选, 跳过, 一致


# =====================================================================
# 轨道入口（供 cnsmith_run.py 调用）
# =====================================================================
def 跳过记录(样本们, 预算):
    """预算耗尽样本 → 显式跳过记录（禁当绿；供轨道循环与 selftest 共用）。"""
    return [{"样本": s["名"], "来源": s["来源"], "源码": str(s["源"]),
             "源": str(s["源"]), "判定": 判定_跳过,
             "详情": "时间预算 %ds 耗尽（未执行·显式计数不当绿）" % 预算,
             "宿主": {}, "v2": {}, "首差异": None, "建议立案": ""}
            for s in 样本们]


def 轨道_对拍(a, 轮目录, 种子, 运行器, v2p, 平台):
    """CN-Smith 轨道「对拍」：准备样本→预热→并行双编译→结构化差异报告。"""
    if v2p is None:
        return {"状态": "无效", "说明": "v2p 未就绪（对拍无 v2 侧即无意义·显形）："
                                        + a.v2侧未就绪原因}
    样本们, 错 = 准备样本(a, 轮目录, 种子)
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
    #   并发首建竞态与 cache miss 并发写踩踏防线——889/Win ml64 并发假 build_err 同族）=====
    print("  [对拍] 预热样本 %s" % 样本们[0]["名"], flush=True)
    结果们.append(对拍一例(ctx, 样本们[0]))
    待跑 = 样本们[1:]
    完成数 = 1
    while 待跑:
        if 截止 is not None and time.monotonic() >= 截止:
            结果们.extend(跳过记录(待跑, 预算))
            待跑 = []
            break
        批 = 待跑[:并发]
        待跑 = 待跑[并发:]
        with concurrent.futures.ThreadPoolExecutor(max_workers=len(批)) as 池:
            批结果 = list(池.map(lambda s: 对拍一例(ctx, s), 批))
        结果们.extend(批结果)
        完成数 += len(批结果)
        print("  [对拍] 进度 %d/%d（%.1fs）" % (完成数, len(样本们), time.time() - t0),
              flush=True)
    结果们.sort(key=lambda r: r["样本"])
    参数 = {"随机": a.对拍样本, "矩阵": a.对拍矩阵, "内置": a.对拍内置,
            "目录": a.对拍目录 or "", "预算秒": 预算, "并发": 并发,
            "比stderr": bool(a.对拍比stderr)}
    记录 = {"轮次": a.轮次名 if hasattr(a, "轮次名") else 轮目录.name,
            "时刻": time.strftime("%Y-%m-%d %H:%M:%S"), "平台": 平台,
            "编译器": getattr(a, "编译指纹", {})}
    json相对, md相对, 候选, 跳过, 一致 = 写差异报告(轮目录, 记录, 结果们, 参数)
    命中 = len(候选)
    return {
        "状态": "完成", "样本数": len(结果们), "一致": 一致, "命中数": 命中,
        "跳过数": len(跳过), "清单": 候选, "结果明细": 结果们,
        "差异报告": json相对, "差异报告md": md相对,
        "建议立案": [r["建议立案"] for r in 候选 if r.get("建议立案")],
        "说明": "一致 %d/%d·分叉候选 %d%s" % (
            一致, len(结果们), 命中,
            ("·跳过 %d（预算耗尽）" % len(跳过)) if 跳过 else ""),
    }


# =====================================================================
# 自校验（判定层正/反例 + 报告结构 + 可选真跑）
# =====================================================================
def selftest(编译路径=None, 实跑=False):
    """对拍轨道自校验。返回 0=全绿 / 1=红。

    静态面（零编译依赖·判定层）：
      ①正例：双侧 rc/stdout 相同 → 一致（0 分叉）；
      ②反例注入：stdout 单行篡改 → 行为分叉 + 首差异行号正确；
      ③反例注入：rc 不同 → 行为分叉（类别 rc）；
      ④反例注入：行数不同（缺行）→ 检出；
      ⑤stderr 开关：默认不参与 / 开启后参与；
      ⑥建议立案标题非空且含样本名；
      ⑦报告结构：候选/跳过/建议立案键齐备（临时目录落盘回收）。
    端到端面（--实跑·需 v2p 就绪）：内置合法样本双编译一致→0 分叉；
      再对真实 v2 明细注入 stdout 差异→必须检出（人为注入分叉全链闭合）。
    """
    通过, 失败 = [], []

    def 签(名, 条件, 详情=""):
        if 条件:
            通过.append(名)
        else:
            失败.append((名, 详情))

    def 明细(rc, stdout="", stderr=""):
        return {"rc": rc, "stdout": stdout, "stderr": stderr}

    # ① 正例
    判定, 详情, 首 = 比对两侧(明细(0, "甲\n乙\n"), 明细(0, "甲\n乙\n"))
    签("正例：双侧 rc+stdout 相同 → 一致（0 分叉）",
       判定 == 判定_一致 and 首 is None, "%s %s" % (判定, 详情))
    # ② stdout 单行篡改
    判定, 详情, 首 = 比对两侧(明细(0, "甲\n乙\n丙\n"), 明细(0, "甲\nX\n丙\n"))
    签("反例：stdout 第2行篡改 → 行为分叉 + 首差异行=2",
       判定 == 判定_行为分叉 and 首 and 首["行"] == 2 and 首["类别"] == "stdout",
       "%s %s" % (判定, 首))
    # ③ rc 不同
    判定, 详情, 首 = 比对两侧(明细(0, "甲\n"), 明细(3, "甲\n"))
    签("反例：rc 不同（0 vs 3）→ 行为分叉（类别 rc）",
       判定 == 判定_行为分叉 and 首 and 首["类别"] == "rc", "%s %s" % (判定, 首))
    # ④ 行数不同
    判定, 详情, 首 = 比对两侧(明细(0, "甲\n乙\n"), 明细(0, "甲\n"))
    签("反例：v2 缺行 → 检出（首差异行=2·乙侧=<缺行>）",
       判定 == 判定_行为分叉 and 首 and 首["行"] == 2 and 首["乙"] == "<缺行>",
       "%s %s" % (判定, 首))
    # ⑤ stderr 开关
    判定1, _, _ = 比对两侧(明细(0, "甲\n", "警告甲\n"), 明细(0, "甲\n", "警告乙\n"))
    判定2, _, 首2 = 比对两侧(明细(0, "甲\n", "警告甲\n"), 明细(0, "甲\n", "警告乙\n"),
                       比stderr=True)
    签("stderr 按需：默认不参与（一致）｜开启后参与（分叉·类别 stderr）",
       判定1 == 判定_一致 and 判定2 == 判定_行为分叉 and 首2["类别"] == "stderr",
       "%s/%s %s" % (判定1, 判定2, 首2))
    # ⑥ 建议立案标题
    记录 = {"判定": 判定_行为分叉, "样本": "selftest样本", "来源": "自测",
            "详情": "stdout 第 2 行起不一致", "首差异": {"行": 2, "类别": "stdout",
                                                  "甲": "乙", "乙": "X"}}
    标题 = 建议立案标题(记录)
    签("建议立案标题含判定+样本名（不自动写服务端）",
       bool(标题) and "selftest样本" in 标题 and "〔1130对拍〕" in 标题, 标题)
    # ⑥b 预算跳过显形（不当绿）
    跳 = 跳过记录([{"名": "预算样本", "源": pathlib.Path("x.cn"), "来源": "自测"}], 1)
    签("预算耗尽 → 显式跳过记录（判定=跳过·不当绿）",
       len(跳) == 1 and 跳[0]["判定"] == 判定_跳过 and "预算" in 跳[0]["详情"],
       str(跳))
    # ⑦ 报告结构（临时目录）
    临时 = 根 / "target" / "cnsmith_dual_selftest"
    if 临时.exists():
        shutil.rmtree(临时)
    try:
        结果们 = [
            {"样本": "s1", "来源": "自测", "源码": "/x/s1.cn", "判定": 判定_一致,
             "详情": "", "宿主": {"编译rc": 0, "rc": 0, "stdout": "甲\n", "stderr": "",
                               "stdout行数": 1, "stderr行数": 0},
             "v2": {"编译": "通过", "rc": 0, "stdout": "甲\n", "stderr": "",
                    "stdout行数": 1, "stderr行数": 0}, "首差异": None, "建议立案": ""},
            {"样本": "s2", "来源": "自测", "源码": "/x/s2.cn", "判定": 判定_行为分叉,
             "详情": "stdout 第 1 行起不一致", "宿主": {"编译rc": 0, "rc": 0,
             "stdout": "甲\n", "stderr": "", "stdout行数": 1, "stderr行数": 0},
             "v2": {"编译": "通过", "rc": 0, "stdout": "乙\n", "stderr": "",
                    "stdout行数": 1, "stderr行数": 0},
             "首差异": {"行": 1, "类别": "stdout", "甲": "甲", "乙": "乙"},
             "建议立案": 标题},
            {"样本": "s3", "来源": "自测", "源码": "/x/s3.cn", "判定": 判定_跳过,
             "详情": "预算耗尽", "宿主": {}, "v2": {}, "首差异": None, "建议立案": ""},
        ]
        json相对, md相对, 候选, 跳过, 一致 = 写差异报告(
            临时, {"轮次": "自测", "时刻": "-", "平台": "自测", "编译器": {}},
            结果们, {"比stderr": False})
        数据 = json.loads(读文本(根 / json相对))
        签("报告结构：候选1/跳过1/一致1 + 建议立案清单齐备",
           len(候选) == 1 and len(跳过) == 1 and 一致 == 1
           and len(数据.get("建议立案", [])) == 1
           and 数据["候选"][0]["首差异"]["行"] == 1,
           "候选=%d 跳过=%d 一致=%d" % (len(候选), len(跳过), 一致))
        签("报告 md 含建议立案标题与首差异定位",
           "建议立案标题" in 读文本(根 / md相对) and "首差异" in 读文本(根 / md相对))
    finally:
        shutil.rmtree(临时, ignore_errors=True)

    # ===== 端到端面（可选·--实跑）：真实双编译 =====
    if 实跑:
        实跑结果 = _selftest实跑(编译路径)
        for 名, 条件, 详情 in 实跑结果:
            签(名, 条件, 详情)

    for 名 in 通过:
        print("  [通过] %s" % 名)
    for 名, 详情 in 失败:
        print("  [失败] %s%s" % (名, ("—— " + 详情) if 详情 else ""))
    print("selftest: %d 通过 / %d 失败" % (len(通过), len(失败)))
    return 1 if 失败 else 0


def _selftest实跑(编译路径=None):
    """端到端自测：内置合法样本→宿主/v2 双编译；反例=真实 v2 明细注入 stdout 差异。"""
    结果 = []
    cn = pathlib.Path(编译路径) if 编译路径 else (根 / "target" / "cn")
    if not cn.exists():
        结果.append(("端到端：编译器存在（%s）" % cn, False, "缺失——先构建 target/cn"))
        return 结果
    try:
        运行器 = 加载e2e运行器()
    except Exception as e:
        结果.append(("端到端：e2e 运行器加载", False, repr(e)))
        return 结果
    平台 = 探测目标平台()
    if 平台 == "win-x64":
        就绪 = 运行器.确保v2p就绪win(cn, False, "selftest")
    else:
        就绪 = 运行器.确保v2p与运行时就绪(cn, 平台, False, "selftest")
    if 就绪[0] is None:
        结果.append(("端到端：v2p 就绪", False, str(就绪[1])[:200]))
        return 结果
    结果.append(("端到端：v2p 就绪", True, ""))
    临时 = 根 / "target" / "cnsmith_dual_selftest_e2e"
    if 临时.exists():
        shutil.rmtree(临时)
    try:
        样本源 = 临时 / "源码"
        样本源.mkdir(parents=True)
        源码 = ('函数 主() -> 整32 {\n'
                '    整32 甲 = 7;\n    整32 乙 = 3;\n'
                '    打印行("对拍自测 %d", 甲 * 乙 + 1);\n'
                '    返回 0;\n}\n')
        (样本源 / "主.cn").write_text(源码, encoding="utf-8")
        用例输出 = 临时 / "输出"
        用例输出.mkdir()
        ctx = {"运行器": 运行器, "cn": cn, "平台": 平台, "v2p": 就绪[0],
               "用例输出": 用例输出, "用例根": 临时 / "用例", "比stderr": False}
        ctx["用例根"].mkdir()
        样本 = {"名": "selftest_端到端", "源": 样本源 / "主.cn", "来源": "自测",
                "整树": True, "根": 样本源}
        记录 = 对拍一例(ctx, 样本)
        结果.append(("端到端正例：同源码双编译 rc+stdout 一致（0 分叉）",
                     记录["判定"] == 判定_一致,
                     "%s ｜ %s" % (记录["判定"], 记录.get("详情", ""))))
        # 反例：拿真实 v2 明细注入一行差异，比对层必须检出
        if 记录["v2"].get("rc") is not None:
            假v2 = {"rc": 记录["v2"].get("rc"),
                    "stdout": (记录["v2"].get("stdout") or "") + "注入分叉行\n",
                    "stderr": 记录["v2"].get("stderr") or ""}
            判定, 详情, 首 = 比对两侧(记录["宿主"], 假v2)
            结果.append(("端到端反例：真实 v2 明细注入 stdout 差异 → 检出",
                         判定 == 判定_行为分叉 and bool(首),
                         "%s %s" % (判定, 详情)))
        else:
            结果.append(("端到端反例：真实 v2 明细可用", False,
                         "v2 未产出运行明细：%s" % (记录["v2"].get("说明", "")[:200])))
    finally:
        shutil.rmtree(临时, ignore_errors=True)
    return 结果


# =====================================================================
# 独立 CLI（--selftest / 单跑对拍）
# =====================================================================
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
    ap.add_argument("--对拍比stderr", action="store_true")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--cn", default="target/cn")
    ap.add_argument("--归档", default=str(默认归档根))
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--详细", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return selftest(a.编译, a.实跑)

    class 轨道参数:                     # cnsmith_run 轨道函数所需的最小参数面
        pass
    参 = 轨道参数()
    for 键 in ("对拍样本", "对拍矩阵", "对拍内置", "对拍目录", "对拍预算秒",
              "对拍比stderr", "jobs"):
        setattr(参, 键, getattr(a, 键))
    参.cn = pathlib.Path(a.cn)
    if not 参.cn.is_absolute():
        参.cn = 根 / 参.cn
    if not 参.cn.exists():
        print("编译器不存在：%s（先构建 target/cn）" % 参.cn)
        return 2
    平台 = 探测目标平台()
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
    if 结果.get("状态") != "完成":
        print("对拍无效：" + 结果.get("说明", ""))
        return 2
    print("[对拍] %s" % 结果["说明"])
    print("  差异报告：%s ｜ %s" % (结果["差异报告"], 结果["差异报告md"]))
    for 标题 in 结果["建议立案"]:
        print("  建议立案：%s" % 标题)
    return 0 if 结果["命中数"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
