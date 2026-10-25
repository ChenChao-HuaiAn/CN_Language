#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN-Smith 轨道入口（167·964 余量收口：daemon/定时轨道挂载 + 双侧行为对拍常态化差分报告）。

背景（167 台账·2026-10-14 并案登记）：964 轮交付构件组合矩阵（cnsmith_matrix.py），
余量=「daemon 轨道挂载」（采样能按轮/定时自动跑并归档）+「44 篇审计『双侧行为对拍
常态化差分报告』缺口」（并案评估）。本入口把 CN-Smith 四条轨道收敛为单命令：

  随机  随机程序 -O0 vs -O3 优化器差分（cnsmith_gen + cnsmith_diff·同 daemon 面）
  负向  负向注入样本双侧干净拒（宿主+v2 必拒·零崩溃·零误接受；**rc=0 双条件**=
        产物落盘存在才叫误接受，rc=0 无产物=「假绿」严重命中——#1108 类防线）
  矩阵  构件组合模板矩阵（cnsmith_matrix·骨架×类型轴）
  对拍  宿主×v2 双侧行为对拍（同源双编译双运行：退出码一致 + 输出逐行一致）

每轮产出（归档默认 target/cnsmith_runs/<轮次>/·gitignore 勿污 git）：
  轮次.json（结构化）· 报告.md（人类可读）· 日志.txt（步骤与命中明细）
  对拍/差异报告.json + 对拍/差异报告.md（1130：两侧输出/首差异行/rc/建议立案）
  归档根另落「最新.json / 最新-报告.md」指针——供看板巡检/交接引用；
  stdout 末行=机器可读摘要行（automation/巡检可直接 grep）。
  **种子派生区间提示（#1109）**：报告显式列出主种子 +0/+7/+11/+3 各轨种子区间，
  手工 --seed 时警示相邻主种子大面积重叠（实测 20261018/20261019 随机 59/60）
  并引导默认时间戳种子；归档内其他轮次的重叠数一并实测列出。

对拍轨道（1130 常态化·独立实现 scripts/cnsmith_dual.py）：
  输入=随机采样+矩阵抽样+内置锁面+`--对拍目录` 目录批量（含 主.cn 或
  v2闭环/双编译对照配置的目录=多文件样本并自动读配置；否则每个 .cn 各自
  独立样本）；宿主与 v2p 同源码双编译双运行，逐行比对 stdout+退出码
  （stderr 按 `--对拍比stderr`），差异=分叉候选并给建议立案标题。
  **跳过/配置不对齐 > 0 时轨道状态=无效（exit 2）**——无结果不算过，禁当绿。
  **缓存对拍轴（#1556·--缓存对拍 开|关 默认开）**：对对拍样本前 N
  （--缓存对拍样本·默认 12）中宿主编译通过者追加「同源码同编译器连跑两次
  宿主 build+运行」（独立 CN_AST_CACHE_DIR·首编写缓存→命中跑挂
  CN_AST_CACHE_VERIFY）——断言命中≥1/损坏回退=0/双跑输出一致；分叉=缓存
  分叉命中（归档桶 缓存/）。v2 侧不参与本轴（另一套编译链）。
  **新特性轮强制**（语义/语法/内建符号/代码生成改动）：集成前必跑
  `python3 scripts/cnsmith_run.py --轨道 对拍 --对拍预算秒 1800`
  （新特性相关 E2E 用例目录以 `--对拍目录 tests/e2e/<用例>` 一并纳入）。
  周采样推荐：`--轨道 对拍 --对拍样本 24 --对拍矩阵 16 --对拍内置 5 --对拍预算秒 1800`。

判定契约（miss 不可静默——差分无结果不算过）：
  全绿=0 ｜ 红=1（任一轨道命中）｜ 无效=2（任一轨道未产出结果/解析失败/就绪失败）
  轨道「无效」在报告与退出码中显式显形，禁止当绿。对拍「跳过」（预算耗尽）显式计数。

用法：
  python3 scripts/cnsmith_run.py                          # 默认四轨道·小规模
  python3 scripts/cnsmith_run.py --轨道 矩阵 --样本 100    # 单轨道
  python3 scripts/cnsmith_run.py --track 对拍              # 英文别名同义（1130）
  python3 scripts/cnsmith_run.py --轨道 对拍 --对拍目录 tests/e2e/01_hello
  python3 scripts/cnsmith_run.py --轨道 全部 --入日志      # 摘要行追加命中库日志（入库面）
  python3 scripts/cnsmith_run.py --轮 3 --间隔 600         # 按轮常驻（每轮独立归档）
  python3 scripts/cnsmith_run.py --selftest                # 对拍轨道自校验（1130）

ZCode automation「CN-Smith」定时任务推荐 prompt（仓库根执行）：
  git pull --ff-only && python3 scripts/cnsmith_run.py --轨道 全部 --入日志
（automation 已 paused 现状下，等效形态=手动/daemon 挂载；见报告「消费方式」节。）
"""
import argparse
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time

根 = pathlib.Path(__file__).resolve().parent.parent
轨道全集 = ("随机", "负向", "矩阵", "对拍")
命中库 = 根 / "tests" / "cnsmith_hits"
默认归档根 = 根 / "target" / "cnsmith_runs"

# 命中入库类别桶（对齐 daemon 既有 diff/build_err/run_err 目录制）：
#   dual=双侧对拍不一致（167 新增）｜matrix=矩阵红例（167 新增）
类别桶_对拍 = "dual"
类别桶_矩阵 = "matrix"
# 负向轨道严重命中类别（干净拒之外的异常态）：
#   「假绿」=rc=0 但产物未落盘（#1108 类：编译器静默失败，下游不得当成功）
负向严重类 = ("崩溃", "误接受", "错误产物", "假绿")

# 1130：对拍轨道独立实现（同源码双编译 stdout 对拍·结构化差异报告+建议立案）。
#   本入口只做轨道编排与报告聚合；对拍核心/自校验在 scripts/cnsmith_dual.py
#   （独立可复用：目录批量/并行/预算/首差异行/建议立案；见该模块 docstring）。
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import cnsmith_dual  # noqa: E402  （路径注入后导入，兼容 python scripts/ 直跑）
import cnsmith_dual_report as 对拍报告  # noqa: E402  （种子派生区间提示共用）
from cnsmith_dual_samples import (  # noqa: E402  （偏移口径单一来源）
    负向种子偏移, 对拍矩阵种子偏移)
内置对拍样本 = cnsmith_dual.内置对拍样本


# =====================================================================
# 通用工具
# =====================================================================
def 跑(命令, cwd=None, 超时=600):
    return subprocess.run([str(x) for x in 命令], cwd=str(cwd or 根),
                          capture_output=True, text=True, encoding="utf-8",
                          errors="replace", timeout=超时)


def 探测目标平台():
    if os.name == "nt":
        return "win-x64"
    import platform as _p
    m = _p.machine().lower()
    if m in ("aarch64", "arm64"):
        return "linux-arm64"
    return "linux-x86_64"


def 编译指纹(cn: pathlib.Path):
    """编译器身份（报告环境锚——基线对不上时报告仍可追溯）。"""
    d = {"路径": str(cn.name)}
    if cn.exists():
        st = cn.stat()
        d["大小"] = st.st_size
        d["mtime"] = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(st.st_mtime))
    头 = 跑(["git", "rev-parse", "HEAD"])
    if 头.returncode == 0:
        d["仓库HEAD"] = 头.stdout.strip()
    return d


def 读文本(p, 默认=""):
    try:
        return pathlib.Path(p).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 默认


# =====================================================================
# 轨道 1：随机差分（优化器 -O0 vs -O3）
# =====================================================================
def 解析差分汇总(文本):
    """cnsmith_diff 汇总行 → dict；缺行返回 None（miss 不静默：调用方转「无效」）。"""
    m = re.search(r"总数\s*(\d+)\s*｜\s*一致\s*(\d+)\s*｜\s*分歧\s*(\d+)\s*｜\s*"
                  r"编译失败\s*(\d+)\s*｜\s*运行失败\s*(\d+)\s*｜\s*编译器崩溃\s*(\d+)", 文本)
    if not m:
        return None
    return dict(zip(("总数", "一致", "分歧", "编译失败", "运行失败", "编译器崩溃"),
                    (int(x) for x in m.groups())))


def 解析分歧清单(路径):
    """分歧清单.txt → [{样本, 类别, 详情}]（结构化标签格式见 cnsmith_diff 303-a 根治）。"""
    行们 = []
    for 行 in 读文本(路径).splitlines():
        m = re.match(r"(\S+\.cn)\s*\[(\w+)\]:\s*(.*)", 行.strip())
        if m:
            行们.append({"样本": m.group(1), "类别": m.group(2), "详情": m.group(3)[:200]})
    return 行们


def 轨道_随机(a, 轮目录, 种子):
    out = 轮目录 / "随机"
    out.mkdir(parents=True, exist_ok=True)
    gen = 跑([sys.executable, "scripts/cnsmith_gen.py", "--seed", str(种子),
              "--count", str(a.样本), "--out", str(out)], 超时=600)
    if gen.returncode != 0:
        return {"状态": "无效", "说明": "生成失败：" + (gen.stderr or gen.stdout)[-300:]}
    diff = 跑([sys.executable, "scripts/cnsmith_diff.py", "--dir", str(out),
               "--cn", str(a.cn), "--out", str(out / "work"), "--jobs", str(a.jobs),
               "--target", 探测目标平台()], 超时=7200)
    汇总 = 解析差分汇总(diff.stdout)
    if 汇总 is None:
        return {"状态": "无效", "说明": "差分汇总行缺失（无结果≠过）",
                "输出": diff.stdout[-500:] + diff.stderr[-300:]}
    清单 = 解析分歧清单(out / "work" / "分歧清单.txt")
    命中 = 汇总["分歧"] + 汇总["运行失败"] + 汇总["编译器崩溃"]
    说明 = ("红：%d 项命中（分歧/运行失败/编译器崩溃）" % 命中) if 命中 else (
        "编译失败 %d 项待甄别（生成器域局限或编译器真拒绝——见清单）" % 汇总["编译失败"]
        if 汇总["编译失败"] else "全绿")
    if 命中 > 0 and not 清单:
        说明 += "；⚠ 清单文件缺失——命中明细不可得（显形）"
    for e in 清单:
        e["源"] = str(out / e["样本"])
    return {"状态": "完成", "汇总": 汇总, "命中数": 命中, "清单": 清单,
            "待甄别": 汇总["编译失败"], "说明": 说明, "输出": diff.stdout[-500:]}


# =====================================================================
# 轨道 2：负向注入（双侧干净拒）
# =====================================================================
def 单侧负向判定(rc, 输出, 产物路径):
    """单侧负向样本判定 → (类别, 说明)。类别 ∈ 干净拒/误接受/假绿/崩溃/拒无诊断/错误产物。

    **rc=0 双条件（#1108 类假绿防线）**：rc=0 且产物落盘存在才叫「误接受」；
    rc=0 但产物缺失=「假绿」（编译器静默失败仍报成功）——同属严重命中，
    不得因为「没产物」把它误当拒绝（假绿正是 #1108 推倒重来的形态）。"""
    if rc < 0:
        return "崩溃", "信号级崩溃 rc=%d（负向样本须干净拒绝）" % rc
    if rc == 0:
        if 产物路径 is not None and not pathlib.Path(产物路径).exists():
            return "假绿", ("编译器 rc=0 但未落盘产物（#1108 类假绿·"
                          "rc=0 不等同成功·双条件缺产物）")
        return "误接受", "编译器 rc=0 放行非法程序（负向防线失守）"
    if 产物路径 is not None and pathlib.Path(产物路径).exists():
        return "错误产物", "拒绝后仍产出产物（错误产物纪律回归）"
    if not (输出 or "").strip():
        return "拒无诊断", "拒绝（rc=%d）但无任何诊断输出" % rc
    return "干净拒", ""


def v2p编译一次(运行器, v2p, 源, 工作目录, 平台):
    """v2 侧单样本编译（工作目录隔离·asm 落点 target/v2asm.s）→ (rc, 输出, asm路径)。"""
    (工作目录 / "target").mkdir(parents=True, exist_ok=True)
    # 1038 修：win 侧原 os.symlink 需开发者模式/管理员特权（未开=OSError 中断）；
    #   改用 run_e2e 同款 win junction（mklink /J·999 先例「建stdlib软链」）
    运行器.建stdlib软链(工作目录, 平台)
    入口 = 工作目录 / pathlib.Path(源).name
    shutil.copy2(源, 入口)
    asm = 工作目录 / "target" / "v2asm.s"
    if asm.exists():
        asm.unlink()
    r = 运行器.运行命令([str(v2p), str(入口), 平台], 工作目录,
                     内存上限MB=运行器.内存上限MB默认, 超时秒数=运行器.编译超时秒数)
    return r.returncode, (r.stdout or "") + (r.stderr or ""), asm


def 轨道_负向(a, 轮目录, 种子, 运行器, v2p, 平台):
    out = 轮目录 / "负向"
    out.mkdir(parents=True, exist_ok=True)
    gen = 跑([sys.executable, "scripts/cnsmith_gen.py", "--seed", str(种子 + 负向种子偏移),
              "--count", str(a.样本), "--out", str(out), "--negative"], 超时=600)
    if gen.returncode != 0:
        return {"状态": "无效", "说明": "负向生成失败：" + (gen.stderr or gen.stdout)[-300:]}
    样本们 = sorted(f for f in os.listdir(out) if f.endswith(".cn"))
    if not 样本们:
        return {"状态": "无效", "说明": "负向样本 0 个（生成器跳过全部——无结果不算过）"}
    工作 = out / "work"
    工作.mkdir(exist_ok=True)
    命中, 黄例 = [], []
    for 名 in 样本们:
        源 = out / 名
        宿主exe = 工作 / (名[:-3] + ".宿主")
        r宿 = 运行器.运行命令([str(a.cn), "build", str(源), "--target", 平台, "-O0",
                           "--output", str(宿主exe)], 根,
                          超时秒数=运行器.编译超时秒数)
        c宿, s宿 = 单侧负向判定(r宿.returncode,
                              (r宿.stdout or "") + (r宿.stderr or ""), 宿主exe)
        if v2p is None:
            c2, s2 = "未就绪", a.v2侧未就绪原因
        else:
            rc2, 输出2, asm2 = v2p编译一次(运行器, v2p, 源, 工作 / (名[:-3] + ".v2"), 平台)
            c2, s2 = 单侧负向判定(rc2, 输出2, asm2)
        if c宿 in 负向严重类 or c2 in 负向严重类 or (c宿 == "干净拒") != (c2 == "干净拒"):
            细节 = "宿主=%s%s；v2=%s%s" % (c宿, ("(%s)" % s宿) if s宿 else "",
                                        c2, ("(%s)" % s2) if s2 else "")
            命中.append({"样本": 名, "宿主": c宿, "v2": c2, "源": str(源),
                        "详情": 细节[:220]})
        elif c宿 == "拒无诊断" or c2 == "拒无诊断":
            黄例.append({"样本": 名, "宿主": c宿, "v2": c2, "源": str(源)})
    绿 = len(样本们) - len(命中) - len(黄例)
    状态 = "无效" if v2p is None else "完成"
    if 状态 == "完成":
        说明 = ("双侧干净拒 %d/%d·命中 %d%s"
                % (绿, len(样本们), len(命中),
                   ("·黄例（拒无诊断）%d" % len(黄例)) if 黄例 else ""))
    else:
        说明 = "宿主侧照常判定；v2 侧未就绪（显形）：" + a.v2侧未就绪原因
    return {"状态": 状态, "样本数": len(样本们), "命中数": len(命中), "清单": 命中,
            "黄例": 黄例, "说明": 说明}


# =====================================================================
# 轨道 3：构件矩阵
# =====================================================================
def 解析矩阵汇总(文本):
    m = re.search(r"构件组合矩阵：绿\s*(\d+)\s*/\s*红\s*(\d+)", 文本)
    if not m:
        return None
    return {"绿": int(m.group(1)), "红": int(m.group(2))}


def 轨道_矩阵(a, 轮目录, 种子):
    out = 轮目录 / "矩阵"
    r = 跑([sys.executable, "scripts/cnsmith_matrix.py", "--cn", str(a.cn),
            "--out", str(out), "--seed", str(种子 + 对拍矩阵种子偏移)], 超时=3600)
    汇总 = 解析矩阵汇总(r.stdout or "")
    if 汇总 is None:
        return {"状态": "无效", "说明": "矩阵汇总行缺失（无结果≠过）",
                "输出": (r.stdout or "")[-500:] + (r.stderr or "")[-300:]}
    红例 = []
    for 行 in (r.stdout or "").splitlines():
        m = re.match(r"\s*\[红\]\s*(\S+)\s*\[(\w+)\]\s*(.*)", 行)
        if m:
            红例.append({"样本": m.group(1), "类别": m.group(2), "详情": m.group(3)[:200],
                        "源": str(out / (m.group(1) + ".cn"))})
    说明 = ("全绿（骨架×类型 %d 例）" % 汇总["绿"]) if 汇总["红"] == 0 \
        else "红 %d 例" % 汇总["红"]
    if 汇总["红"] > 0 and not 红例:
        说明 += "；⚠ 红例明细解析为空（显形）"
    return {"状态": "完成", "汇总": 汇总, "命中数": 汇总["红"], "清单": 红例, "说明": 说明}




# =====================================================================
# 命中归档 + 报告
# =====================================================================
def 归档命中(根目录, 类别, 轮次名, 源路径, meta行们, 源是目录=False):
    """命中样本归档（目录可切换）：默认轮目录/命中/<类别>/（target 面·零污 git）；
    --入命中库 时 tests/cnsmith_hits/<类别>/（对齐 daemon 存档制·防 target 清场丢失）。
    已存在同名（历史命中）不覆盖，返回 ""。源是目录=整树样本（多文件程序）整目录归档。"""
    目录 = pathlib.Path(根目录) / 类别
    目录.mkdir(parents=True, exist_ok=True)
    源 = pathlib.Path(源路径)
    if 源是目录:
        源 = 源.parent if 源.is_file() else 源     # 整树样本：源=目录（或入口文件的父目录）
        名 = 轮次名.split("-")[0] + "_" + 源.name
        目标 = 目录 / 名
        if 目标.exists() or not 源.exists():
            return ""
        shutil.copytree(源, 目标,
                        ignore=shutil.ignore_patterns(".git", "target", "*.expected"))
    else:
        名 = 轮次名.split("-")[0] + "_" + pathlib.Path(源路径).name
        目标 = 目录 / 名
        if 目标.exists() or not 源.exists():
            return ""
        shutil.copy2(源, 目标)
    (目录 / (名 + ".meta")).write_text("\n".join(meta行们) + "\n", encoding="utf-8")
    return str(目标.relative_to(根))


def 汇总判定(轨道结果们):
    """轮次判定：红优先（已知缺陷不被过程无效掩盖）→ 无效 → 全绿。

    P0-1（1130 审查）：对拍轨道「跳过>0」（预算耗尽/配置不对齐）由轨道自身置
    状态=无效（exe 码 2）——无结果不算过，禁当绿。命中仍全轨统计（无效轨道
    若已跑出分叉候选，红优先显形）。"""
    命中 = sum(v.get("命中数", 0) for v in 轨道结果们.values())
    无效 = [k for k, v in 轨道结果们.items() if v.get("状态") != "完成"]
    if 命中:
        return "红", 命中, 无效
    if 无效:
        return "无效", 0, 无效
    return "全绿", 0, []


def 轨道速记(k, v):
    if k == "对拍":                  # 1130：无效（跳过）时仍给结果面，不吞一致数
        缓 = v.get("缓存对拍") or {}
        if 缓.get("启用"):
            # #1556：缓存对拍轴独立面（一致/样本数·命中数附注）进摘要行/报告表
            缓段 = " 缓存对拍=%d/%d" % (缓.get("一致", 0), 缓.get("样本数", 0))
            if 缓.get("命中"):
                缓段 += "(分叉%d)" % 缓["命中"]
            if 缓.get("统计缺失"):
                缓段 += "(统计缺失%d)" % 缓["统计缺失"]
        elif 缓:
            缓段 = " 缓存对拍=关"
        else:
            缓段 = ""
        return "对拍=%d/%d%s%s%s" % (
            v.get("一致", 0), v.get("样本数", 0),
            ("(跳过%d)" % v.get("跳过数", 0)) if v.get("跳过数") else "",
            "(无效)" if v.get("状态") != "完成" else "", 缓段)
    if v.get("状态") != "完成":
        return "%s=无效" % k
    if k == "随机":
        return "随机=%d/%d" % (v["汇总"]["一致"], v["汇总"]["总数"])
    if k == "负向":
        return "负向=%d/%d" % (v["样本数"] - v["命中数"], v["样本数"])
    if k == "矩阵":
        return "矩阵=%d/%d" % (v["汇总"]["绿"], v["汇总"]["绿"] + v["汇总"]["红"])
    return k


def 摘要行(记录):
    速记 = " ".join(轨道速记(k, v) for k, v in 记录.get("轨道们", {}).items())
    return ("[CN-Smith] 轮次=%s 判定=%s %s 命中=%d 报告=%s/报告.md"
            % (记录["轮次"], 记录.get("判定", "?"), 速记, 记录.get("命中合计", 0),
               记录.get("报告相对", "")))


# =====================================================================
# 种子派生区间提示（#1109）
# =====================================================================
def 历史种子们(归档根, 当前轮目录):
    """归档内已跑轮次的主种子与轨道样本数（种子重叠实测警示的数据源）。

    只读轮次.json；解析失败/缺字段=跳过（历史面缺失不影响本轮判定，报告
    显式给空列表）。只取最近 16 轮——防长期归档把报告渲染撑爆。"""
    结果 = []
    当前 = pathlib.Path(当前轮目录).resolve()
    for 路径 in sorted(pathlib.Path(归档根).glob("*/轮次.json")):
        try:
            if 路径.parent.resolve() == 当前:
                continue
            数据 = json.loads(路径.read_text(encoding="utf-8"))
            主种子 = 数据.get("种子")
            if not isinstance(主种子, int):
                continue
            参 = 数据.get("轨道参数") or {}
            轨道们 = 数据.get("轨道们") or {}
            结果.append((路径.parent.name, 主种子, {
                "随机差分": 参.get("随机样本", 0) if "随机" in 轨道们 else 0,
                "负向注入": 参.get("随机样本", 0) if "负向" in 轨道们 else 0,
                "对拍随机": 参.get("对拍随机", 0) if "对拍" in 轨道们 else 0,
                "构件矩阵": 参.get("对拍矩阵", 0) if "对拍" in 轨道们 else 0,
            }))
        except (OSError, ValueError):
            continue
    return 结果[-16:]


def 写报告(轮目录, 记录, 轨道结果们):
    判定, 命中总数, 无效 = 汇总判定(轨道结果们)
    记录["判定"], 记录["命中合计"], 记录["轨道们"] = 判定, 命中总数, 轨道结果们
    (轮目录 / "轮次.json").write_text(
        json.dumps(记录, ensure_ascii=False, indent=1), encoding="utf-8")
    行 = ["# CN-Smith 轨道报告（%s）" % 记录["轮次"], "",
          "- 时刻：%s ｜ 平台：%s ｜ 判定：**%s**" % (记录["时刻"], 记录["平台"], 判定),
          "- 编译器：%s（sha=%s·mtime=%s）" % (记录["编译器"].get("路径"),
                                          记录["编译器"].get("仓库HEAD", "?")[:9],
                                          记录["编译器"].get("mtime", "?")),
          "- 种子：%s（来源：%s）｜ 归档：%s" % (
              记录["种子"], 记录.get("种子来源", "?"), 记录["报告相对"]), ""]
    种子 = 记录.get("种子派生")
    if 种子:
        行 += 对拍报告.种子报告节(种子) + [""]   # 与对拍差异报告同一渲染源
    if 无效:
        行 += ["> **轨道未完成（无结果/覆盖不足——miss 不静默·无结果不算过）**："
             + "、".join(无效), ""]
    行 += ["| 轨道 | 状态 | 结果 | 说明 |", "|---|---|---|---|"]
    for k, v in 轨道结果们.items():
        行.append("| %s | %s | %s | %s |" % (k, v.get("状态", "?"), 轨道速记(k, v),
                                          (v.get("说明", "") or "").replace("|", "/")))
    行 += ["", "## 命中清单（%d 件）" % 命中总数]
    if 命中总数 == 0:
        行.append("零命中。")
    for k, v in 轨道结果们.items():
        for e in v.get("清单", []):
            行.append("- [%s] %s：%s" % (k, e.get("样本"),
                                       (e.get("详情") or e.get("v2") or "")[:200]))
    for k, v in 轨道结果们.items():                 # 黄例（拒无诊断等观察面）
        for e in v.get("黄例", []):
            行.append("- [%s·黄例] %s：宿主=%s v2=%s" % (k, e["样本"], e["宿主"], e["v2"]))
    if 记录.get("归档"):
        行.append("")
        行.append("- 命中样本归档 %d 件：%s%s" % (
            len(记录["归档"]), "、".join(记录["归档"][:5]),
            "…" if len(记录["归档"]) > 5 else ""))
    if 轨道结果们.get("对拍", {}).get("结果明细"):   # 对拍全清单（常态证据面）
        对拍结果 = 轨道结果们["对拍"]
        行 += ["", "## 双侧对拍明细（宿主×v2）", "",
              "- 结构化差异报告：`%s`（JSON）｜ `%s`（MD）" % (
                 对拍结果.get("差异报告", "?"), 对拍结果.get("差异报告md", "?")),
              "- 样本 %d：一致 %d ｜ 分叉候选 %d ｜ 跳过 %d" % (
                 对拍结果.get("样本数", 0), 对拍结果.get("一致", 0),
                 对拍结果.get("命中数", 0), 对拍结果.get("跳过数", 0))]
        缓 = 对拍结果.get("缓存对拍") or {}
        if 缓:                        # #1556：缓存对拍轴汇总面（报告/JSON/日志三处同源）
            if 缓.get("启用"):
                行.append("- 缓存对拍轴（#1556·宿主 AST 缓存二次编译对拍·v2 不参与）："
                          "样本 %d（上限 %d·宿主拒绝排除 %d）｜ 一致 %d ｜ 缓存分叉 %d ｜ "
                          "跳过 %d（统计缺失 %d）"
                          % (缓.get("样本数", 0), 缓.get("上限", 0), 缓.get("排除数", 0),
                             缓.get("一致", 0), 缓.get("命中", 0),
                             缓.get("跳过", 0), 缓.get("统计缺失", 0)))
            else:
                行.append("- 缓存对拍轴：关（%s）" % 缓.get("说明", "未启用"))
        if 对拍结果.get("建议立案"):
            行 += ["", "### 建议立案标题（%d 件·不自动写服务端，人类/编排者据此立项）"
                   % len(对拍结果["建议立案"])]
            行 += ["- %s" % 标 for 标 in 对拍结果["建议立案"]]
        行 += ["", "| 样本 | 宿主 | v2 | 判定 | 详情 |", "|---|---|---|---|---|"]
        for r in 对拍结果["结果明细"]:
            宿主文, v2文 = cnsmith_dual.记录速记(r)
            行.append("| %s | %s | %s | %s | %s |" % (
                r["样本"], 宿主文.replace("|", "/"), v2文.replace("|", "/"), r["判定"],
                (r.get("详情") or "").replace("|", "/")[:120]))
    行 += ["", "## 消费方式",
          "- 摘要行（机器可读投影·automation/巡检可直接 grep）：`%s`" % 摘要行(记录),
          "- 命中样本归档：%s（`--入命中库` 切入 tests/cnsmith_hits/<类别>/ 资产面）"
          % ("轮目录/命中/（target 面）" if not 记录.get("归档入命中库")
             else "tests/cnsmith_hits/<类别>/ + 轮目录/命中/"),
          "- 看板/交接引用：回复报告路径+摘要行；`--入日志` 时摘要追加 "
          "tests/cnsmith_hits/日志/ 当日文件（入库面）。",
          "- automation：「CN-Smith」定时任务恢复时 prompt=`git pull --ff-only && "
          "python3 scripts/cnsmith_run.py --轨道 全部 --入日志`（当前 paused）。", ""]
    报告文本 = "\n".join(行)
    (轮目录 / "报告.md").write_text(报告文本, encoding="utf-8")
    (轮目录.parent / "最新.json").write_text(
        json.dumps(记录, ensure_ascii=False, indent=1), encoding="utf-8")
    (轮目录.parent / "最新-报告.md").write_text(报告文本, encoding="utf-8")
    return 判定, 命中总数


def 写日志(轮目录, 轨道结果们):
    行 = ["# CN-Smith 轮次日志（%s）" % 轮目录.name, ""]
    for k, v in 轨道结果们.items():
        行 += ["## 轨道 %s（%s）" % (k, v.get("状态", "?")),
              "- 说明：%s" % v.get("说明", ""), "- 汇总：%s" % json.dumps(
                  v.get("汇总", {}), ensure_ascii=False)]
        if v.get("输出"):
            行 += ["- 尾部输出：", "```", v["输出"][-800:], "```"]
        if v.get("差异报告"):
            行 += ["- 差异报告：%s ｜ %s" % (v["差异报告"], v.get("差异报告md", "")),
                  "- 样本 %d：一致 %d ｜ 候选 %d ｜ 跳过 %d" % (
                      v.get("样本数", 0), v.get("一致", 0),
                      v.get("命中数", 0), v.get("跳过数", 0))]
            缓 = v.get("缓存对拍") or {}
            if 缓:                       # #1556：缓存对拍轴汇总（结构化进日志）
                行.append("- 缓存对拍：%s" % json.dumps(缓, ensure_ascii=False))
        for 标 in v.get("建议立案", []):
            行.append("- 建议立案：%s" % 标)
        for e in v.get("清单", []):
            行.append("- 命中：%s %s" % (e.get("样本"), (e.get("详情") or "")[:200]))
        for e in v.get("黄例", []):
            行.append("- 黄例：%s 宿主=%s v2=%s" % (e["样本"], e["宿主"], e["v2"]))
    (轮目录 / "日志.txt").write_text("\n".join(行) + "\n", encoding="utf-8")


def 归档本轮命中(a, 轮目录, 轮次名, 记录, 轨道结果们):
    """命中样本归档（默认 target 面·--入命中库 切入库面）→ 归档相对路径列表。"""
    落 = []
    目标根 = 命中库 if a.入命中库 else (轮目录 / "命中")
    for k, v in 轨道结果们.items():
        if v.get("状态") != "完成":
            continue
        for e in v.get("清单", []):
            源 = e.get("源")
            if not 源:
                continue
            if k == "随机":
                桶 = e.get("类别", "diff")
                元 = ["基线: %s" % 记录["编译器"].get("仓库HEAD", "?")[:9],
                      "轮: %s（轨道入口·随机）" % 轮次名,
                      "类别: %s" % e.get("类别", ""), "说明: %s" % (e.get("详情") or "")[:300]]
            elif k == "矩阵":
                桶 = 类别桶_矩阵
                元 = ["基线: %s" % 记录["编译器"].get("仓库HEAD", "?")[:9],
                      "轮: %s（轨道入口·矩阵）" % 轮次名,
                      "说明: 矩阵红例 %s" % (e.get("详情") or "")[:260]]
            elif k == "负向":
                # 1022（#1398 随批根治）：负向记录是 {样本,宿主:类别串,v2:类别串,
                #   详情}——与对拍记录（宿主/v2=dict）结构不同，原落 else 分支
                #   调 记录速记 即 AttributeError（'str' has no 'get'）：负向
                #   全绿轮永不触发，本轮族程序命中首次显形。按轨道显式分派。
                桶 = "neg"
                元 = ["基线: %s" % 记录["编译器"].get("仓库HEAD", "?")[:9],
                      "轮: %s（轨道入口·负向注入）" % 轮次名,
                      "宿主: %s；v2: %s" % (e.get("宿主", "?"), e.get("v2", "?")),
                      "说明: %s" % (e.get("详情") or "")[:260]]
            else:
                if e.get("缓存轴"):
                    # #1556：缓存对拍轴命中归档进独立桶「缓存/」（与宿主×v2 分叉 dual/ 分开）
                    桶 = "缓存"
                    统计 = e.get("缓存统计") or {}
                    元 = ["基线: %s" % 记录["编译器"].get("仓库HEAD", "?")[:9],
                          "轮: %s（轨道入口·缓存对拍轴）" % 轮次名,
                          "轴: 缓存对拍（宿主 AST 缓存二次编译对拍·v2 不参与）",
                          "判定: %s" % e.get("判定", ""),
                          "统计: 首跑=%s ｜ 命中跑=%s" % (
                              json.dumps(统计.get("首跑"), ensure_ascii=False),
                              json.dumps(统计.get("命中跑"), ensure_ascii=False)),
                          "验证行: %s" % ("；".join(e.get("缓存验证行们") or [])[:200] or "无"),
                          "说明: %s" % (e.get("详情") or "")[:260]]
                else:
                    桶 = 类别桶_对拍
                    宿主文, v2文 = cnsmith_dual.记录速记(e)
                    元 = ["基线: %s" % 记录["编译器"].get("仓库HEAD", "?")[:9],
                          "轮: %s（轨道入口·双侧对拍）" % 轮次名,
                          "宿主: %s；v2: %s" % (宿主文, v2文),
                          "判定: %s" % e.get("判定", ""),
                          "说明: %s" % (e.get("详情") or "")[:260]]
            路径 = 归档命中(目标根, 桶, 轮次名, 源, 元,
                         源是目录=bool(e.get("整树")))
            if 路径:
                落.append(路径)
    return 落


# =====================================================================
# 轨道 4：双侧行为对拍（宿主×v2·1130 独立实现 cnsmith_dual）
# =====================================================================
def 轨道_对拍(a, 轮目录, 种子, 运行器, v2p, 平台):
    """转调独立对拍轨道（目录批量/并行/预算/首差异/建议立案见 cnsmith_dual）。"""
    return cnsmith_dual.轨道_对拍(a, 轮目录, 种子, 运行器, v2p, 平台)


# =====================================================================
# 主流程
# =====================================================================
def 就绪v2侧(a, 平台):
    """v2p 就绪（指纹缓存）；失败=显式记因（不静默降级）。返回 (运行器, v2p 或 None)。"""
    try:
        运行器 = cnsmith_dual.加载e2e运行器()
    except Exception as e:
        a.v2侧未就绪原因 = "e2e 运行器加载失败：%r" % (e,)
        return None, None
    if 平台 == "win-x64":
        # 1038 修：win 侧就绪=确保v2p就绪win（ml64/link 工具链）；原无条件调
        #   确保v2p与运行时就绪（linux 专用 as/g++ 探测）→ win 上恒「未找到 as」
        #   两轨道无效（#1020 轮 H7 在案·本轮 win 实证复现+根治）
        就绪 = 运行器.确保v2p就绪win(a.cn, a.详细, "cnsmith")
    else:
        就绪 = 运行器.确保v2p与运行时就绪(a.cn, 平台, a.详细, "cnsmith")
    if 就绪[0] is None:
        a.v2侧未就绪原因 = "v2p 就绪失败：" + str(就绪[1])[:200]
        return 运行器, None
    return 运行器, 就绪[0]


def 单轮(a, 轮次名, 归档根):
    轮目录 = 归档根 / 轮次名
    轮目录.mkdir(parents=True, exist_ok=True)
    if not a.cn.exists():
        print("编译器不存在：%s（先构建 target/cn）" % a.cn)
        return 2
    种子 = a.seed if a.seed else int(time.strftime("%y%m%d%H%M%S"))
    种子来源 = "命令行 --seed" if a.seed else "时间戳（默认）"
    平台 = 探测目标平台()
    历史 = 历史种子们(归档根, 轮目录)
    当前数量们 = {}
    if "随机" in a.轨道:
        当前数量们["随机差分"] = a.样本
    if "负向" in a.轨道:
        当前数量们["负向注入"] = a.样本
    if "对拍" in a.轨道:                   # 矩阵样本只在 对拍 轨道内生成（+3 仅注释）
        当前数量们["对拍随机"] = a.对拍样本
        当前数量们["构件矩阵"] = a.对拍矩阵
    记录 = {"轮次": 轮次名, "时刻": time.strftime("%Y-%m-%d %H:%M:%S"), "平台": 平台,
            "种子": 种子, "种子来源": 种子来源, "编译器": 编译指纹(a.cn),
            "轨道参数": {"随机样本": a.样本, "对拍随机": a.对拍样本,
                       "对拍矩阵": a.对拍矩阵, "对拍目录": a.对拍目录 or "",
                       "对拍预算秒": a.对拍预算秒,
                       "对拍比stderr": bool(a.对拍比stderr),
                       "缓存对拍": a.缓存对拍, "缓存对拍样本": a.缓存对拍样本},
            "种子派生": 对拍报告.种子派生信息(种子, 种子来源, 当前数量们, 历史),
            "报告相对": str(轮目录.relative_to(根))}
    # 1130：对拍轨道报告所需的轮次/编译器身份上下文（与主轮次记录同源，防漂移）
    a.轮次名 = 轮次名
    a.编译指纹 = 记录["编译器"]
    a.历史种子们 = 历史
    运行器 = v2p = None
    a.v2侧未就绪原因 = ""
    if "对拍" in a.轨道 or "负向" in a.轨道:
        运行器, v2p = 就绪v2侧(a, 平台)
    轨道结果们 = {}
    for k in a.轨道:
        t0 = time.time()
        print("=== 轨道 %s 开始 ===" % k, flush=True)
        try:
            if k == "随机":
                轨道结果们[k] = 轨道_随机(a, 轮目录, 种子)
            elif k == "负向":
                轨道结果们[k] = 轨道_负向(a, 轮目录, 种子, 运行器, v2p, 平台)
            elif k == "矩阵":
                轨道结果们[k] = 轨道_矩阵(a, 轮目录, 种子)
            else:
                轨道结果们[k] = 轨道_对拍(a, 轮目录, 种子, 运行器, v2p, 平台)
        except Exception as e:      # 轨道异常=显式无效（「有结果才算过」的反面）
            轨道结果们[k] = {"状态": "无效", "说明": "轨道异常：%r" % (e,)}
        print("  [%s] %s（%.1fs）" % (k, 轨道结果们[k].get("说明", ""), time.time() - t0),
              flush=True)
    记录["归档"] = 归档本轮命中(a, 轮目录, 轮次名, 记录, 轨道结果们)
    记录["归档入命中库"] = a.入命中库
    判定, 命中总数 = 写报告(轮目录, 记录, 轨道结果们)
    写日志(轮目录, 轨道结果们)
    print(摘要行(记录), flush=True)
    if a.入日志:
        日 = time.strftime("%Y-%m-%d")
        日志目录 = 命中库 / "日志"
        日志目录.mkdir(parents=True, exist_ok=True)
        with open(日志目录 / (日 + ".md"), "a", encoding="utf-8") as f:
            f.write("\n## 轨道轮（%s·%s）\n- %s\n- 归档：%s（命中归档 %d 件）\n\n"
                    % (记录["时刻"], 轮次名, 摘要行(记录), 记录["报告相对"],
                       len(记录["归档"])))
        print("  摘要已入日志：tests/cnsmith_hits/日志/%s.md" % 日, flush=True)
    return 0 if 判定 == "全绿" else (1 if 判定 == "红" else 2)


def main():
    ap = argparse.ArgumentParser(description="CN-Smith 轨道入口（167·1130 对拍常态轨）")
    ap.add_argument("--轨道", "--track", dest="轨道", default="全部",
                    help="逗号分隔：随机,负向,矩阵,对拍；全部=四轨道全上（--track 同义别名）")
    ap.add_argument("--样本", type=int, default=24, help="随机/负向轨道样本数（默认 24·小规模）")
    ap.add_argument("--对拍样本", type=int, default=12, help="对拍轨道随机样本数（默认 12）")
    ap.add_argument("--对拍矩阵", type=int, default=12, help="对拍轨道矩阵抽样数（默认 12·0=不取）")
    ap.add_argument("--对拍内置", type=int, default=len(内置对拍样本),
                    help="对拍轨道内置执法样本数（默认 %d·0=不取；族上限 %d）"
                         % (len(内置对拍样本), len(内置对拍样本)))
    ap.add_argument("--对拍目录", default="",
                    help="对拍轨道目录批量输入（逗号分隔）：含 主.cn 或 v2闭环/双编译对照"
                         "配置的目录=一个多文件样本（自动读配置）；否则每个顶层 .cn 各为一个样本")
    ap.add_argument("--对拍预算秒", type=int, default=1800,
                    help="对拍轨道时间预算（秒·默认 1800=周采样推荐；0=不限；"
                         "耗尽样本显式计「跳过」→ 轨道无效 exit 2·不当绿）")
    ap.add_argument("--对拍比stderr", action="store_true",
                    help="对拍判定纳入 stderr 逐行比对（默认仅记录不参与判定）")
    ap.add_argument("--缓存对拍", choices=("开", "关"), default="开",
                    help="对拍轨道缓存对拍轴（#1556·宿主 AST 缓存二次编译对拍：首编写缓存→"
                         "命中跑挂 CN_AST_CACHE_VERIFY·断言命中≥1/损坏回退=0/双跑输出一致；"
                         "默认开·纯文档轮可关）")
    ap.add_argument("--缓存对拍样本", type=int, default=12,
                    help="缓存对拍轴样本上限（对拍样本前 N 个中宿主编译通过者·默认 12·"
                         "性能护栏：避免全样本翻倍耗时）")
    ap.add_argument("--seed", type=int, default=0, help="种子（0=时间戳·默认）")
    ap.add_argument("--cn", default=cnsmith_dual.默认编译器相对路径(),
                    help="宿主编译器路径（相对仓库根·#1617 平台感知默认："
                         "win=target/cn.exe/其余=target/cn）")
    ap.add_argument("--归档", default=str(默认归档根), help="归档根目录（默认 target/cnsmith_runs）")
    ap.add_argument("--标签", default="手动", help="轮次名标签")
    ap.add_argument("--轮", type=int, default=1, help="循环轮数（按轮自动跑）")
    ap.add_argument("--间隔", type=int, default=0, help="轮间睡眠秒数")
    ap.add_argument("--jobs", type=int, default=8, help="随机差分/对拍并行度")
    ap.add_argument("--入日志", action="store_true",
                    help="摘要行追加 tests/cnsmith_hits/日志/当日文件（入库面·默认关）")
    ap.add_argument("--入命中库", action="store_true",
                    help="命中样本入库 tests/cnsmith_hits/<类别>/（默认落轮目录/命中/·零污 git）")
    ap.add_argument("--详细", action="store_true", help="v2p 就绪等详细输出")
    ap.add_argument("--selftest", action="store_true",
                    help="对拍轨道自校验（1130·判定层正/反例+报告结构；加 --selftest实跑 附加端到端）")
    ap.add_argument("--selftest实跑", action="store_true",
                    help="selftest 附加端到端真跑（需 v2p 就绪）")
    a = ap.parse_args()
    if a.selftest:
        import cnsmith_dual_selftest as 自测          # 延迟导入（对拍自测模块）
        return 自测.selftest(cnsmith_dual.解析编译器路径(a.cn), a.selftest实跑)
    a.轨道 = list(轨道全集) if a.轨道.strip() == "全部" else \
        [x.strip() for x in a.轨道.split(",") if x.strip()]
    未知 = [x for x in a.轨道 if x not in 轨道全集]
    if 未知:
        print("未知轨道：%s（可选：%s）" % (未知, "、".join(轨道全集)))
        return 2
    # 编译器路径归一（Path + 相对根 + win .exe 探测·审查 P2-1 共同入口）
    a.cn = cnsmith_dual.解析编译器路径(a.cn)
    # 1617 启动探测：异平台编译器（跨机同步 ELF/PE）硬拒——禁带错编译器跑完全轨
    诊断, 允跑 = cnsmith_dual.编译器平台诊断(a.cn, 平台=cnsmith_dual.探测目标平台())
    if not 允跑:
        print(诊断)
        return 2
    归档根 = pathlib.Path(a.归档)
    if not 归档根.is_absolute():
        归档根 = 根 / 归档根
    归档根.mkdir(parents=True, exist_ok=True)
    码 = 0
    for 轮号 in range(max(1, a.轮)):
        轮次名 = time.strftime("%Y%m%d-%H%M%S") + "-" + a.标签
        if a.轮 > 1:
            轮次名 += "-r%d" % (轮号 + 1)
        码 = 单轮(a, 轮次名, 归档根)
        if 轮号 + 1 < a.轮 and a.间隔 > 0:
            time.sleep(a.间隔)
    return 码


if __name__ == "__main__":
    # 555 树锁接入（同 run_e2e/gate_quick 入口口径）：本入口跑编译/运行（宿主编译器+
    #   v2p+as/g++）——同树并发 E2E/构建互踩防线（target/v2p 缓存双写·清场互杀 rc=143）。
    #   同树后来者排队等待；跨树天然并行。逃生门：CN_NO_TREE_LOCK=1（完全跳过）；
    #   CN_TREE_LOCK_HELD=1=嵌套链外层已持锁（gate_quick --l2 调子进程同款语义）。
    import gate_lock
    sys.exit(gate_lock.树锁运行入口(main))
