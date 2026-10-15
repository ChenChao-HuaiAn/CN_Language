#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""diag_dual_diff.py —— 诊断文案双侧对拍（任务 839·头条 59 篇·文案逐字一致红线的机械保证）。

背景：负测双编译对照（569-a）只取宿主侧子串锚（T63 政策：v2 文案与宿主不同串属
合规面）——「拒绝行为等价」有门禁，但「文案逐字差异」没有观测面：哪些负例两侧
文案完全一致、哪些差异大（甚至深层形状不同），无人能答。本工具=报告制补充面
（不硬拦·不改 run_e2e 门禁行为；与 728n check_diag_negtest 互补——那是单侧
诊断↔法条锚对账，这是双侧互对）：对同一非法程序双侧编译收集诊断文本，逐条
归一（去路径/行号前缀噪声）后 diff，产出「文案逐字差异报告」供审计。

枚举面（与 run_e2e 负例三通道同源·优先级同 run_e2e 分派）：
  ① 期望编译失败.txt（build 负例·宿主 cn build 失败）
  ② 期望check失败.txt（check 负例·宿主 cn check 失败·v2 无 check 命令面=以
     编译语义近似并标注）
  ③ 双编译对照.txt 且 预期编译失败=是（dual 负配·含宿主侧诊断锚）
注：①②③ 可同目录并存（run_e2e 实际分派 ①>②>③）；本工具对每例**双侧实跑**
（照出「未双测」面=工具价值），标签与锚按优先级取（dual 的 诊断锚 优先）。

执行（零重复实现·复用 run_e2e 机制·import 先例=gate_quick 取权威名单）：
  宿主侧：cn build/check（隔离产物目录；与 run_e2e 执行负用例同款命令行）；
  v2 侧：拷用例树干净到 target/audit2/diagv2<隔离键> → v2p <入口> <平台>
  （与 run_e2e 双编译对照 DN 负路径同款）；v2p/运行时经 确保v2p与运行时就绪
  指纹缓存（整运行持树锁——同 run_e2e，防 target 缓存互踩）。

归一化（只压噪声不压文案·两级）：
  级1 位置级：路径前缀→裸文件名；行列数字→@ 占位（`.cn:19:5:`→`.cn:@`、
  `（行=19）`→`（行=@）`、`行 9:`→`行 @:`）。
  级2 正文级：级1 再剥定位段/通道标记（`[sem5] 正确诊断: [错误] `、`错误: `、
  `[parse] ` 等；尾部 `（行=@）`），得到「诊断正文」。框架/汇总行（宿主
  `模块 '主' 解析失败`、v2 `语义错误 N 个，中止…`/`语法错误 N 个，中止编译…`）
  单独归类不参与正文比对；v2 侧 `[p6h] 已解析` 进度行剔除并计数（显式列出）。

判定（级2 正文集合比较）：
  一致（集合相等）/ 部分一致（交集非空·各有独有）/ 全异（交集空且双侧有正文）/
  v2未拒绝（v2p rc=0=行为不对等事实）/ 宿主未拒绝（宿主 rc=0=异常显形）/
  单侧无正文（一侧 rc≠0 但零诊断）/ 异常退出（信号级 rc<0）。
  附加事实：严格面（级1 行序列相同=连位置模式都一致）、诊断锚双侧命中表。

用法：
  python3 scripts/diag_dual_diff.py [--filter 模式] [--cn 路径] [--target 平台]
                                    [--输出 报告路径] [--明细 例名] [--verbose]
  python3 scripts/diag_dual_diff.py --selftest    # 归一化/判定引擎自测（真实样本锚定）

退出码：0=报告完成（报告制工具不硬拦；异常面在报告中显形）；2=用法/环境错误。
"""

# 兼容 python3.8（深度机/单位机系统 python）
from __future__ import annotations

import argparse
import os
import pathlib
import platform
import re
import shutil
import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

脚本目录 = pathlib.Path(__file__).resolve().parent
项目根目录 = 脚本目录.parent
E2E目录 = 项目根目录 / "tests" / "e2e"
sys.path.insert(0, str(E2E目录))
import run_e2e  # noqa: E402  # 复用权威机制（先例=gate_quick 导入 run_e2e 取名单）

审计目录 = run_e2e.项目根目录 / "target" / "audit2"
忽略拷贝 = ("*.expected", "*.input", "*.args", "v2闭环.txt", "双编译对照.txt",
           "期望编译失败.txt", "期望check失败.txt")


# ============ 归一化（纯函数·selftest 用真实样本锚定）============

路径正则 = re.compile(r"[^\s:：]*[/\\]([^\s/\\:：]+\.cn)")
位置正则们 = [
    (re.compile(r"([^\s:：]+\.cn):\d+(?::\d+)?"), r"\1:@"),
    (re.compile(r"（行=\d+）"), "（行=@）"),
    (re.compile(r"行\s+\d+\s*[:：]"), "行 @:"),
]
# 循环剥离的「定位段/通道标记」前缀（每轮剥一个·直到不动）
前缀正则们 = [
    re.compile(r"^[^\s:：]+\.cn:@\s*(?:[:：]\s*)?"),
    re.compile(r"^\[[^\]\n]{1,16}\]\s*"),
    re.compile(r"^(?:正确诊断|警告诊断|错误诊断|诊断)\s*[:：]\s*"),
    re.compile(r"^\[(?:错误|警告)\]\s*"),
    re.compile(r"^(?:错误|警告)\s*[:：]\s*"),
    re.compile(r"^行\s*@\s*[:：]\s*"),
]
尾部位置正则 = re.compile(r"\s*[（(]行=@[)）]\s*$")
进度行正则 = re.compile(r"^\[[^\]\n]{1,16}\]\s*已解析[:：]")
# 框架/汇总行（各自实现的收尾声明·非文案本体）——只收已知形态，未匹配
# 的汇总形态会留在正文里显形（多显形优先于漏显形）
框架行正则们 = [
    re.compile(r"^模块\s+'[^']*'\s*解析失败$"),
    re.compile(r"^语义错误\s+\d+\s*个，中止 IR 生成与代码生成（退出码 \d+）$"),
    re.compile(r"^语法错误\s+\d+\s*个，中止编译（不产 IR/asm）$"),
    re.compile(r"^词法错误\s+\d+\s*个.*$"),
]


def 归一位置行(行: str) -> str:
    """级1：剥路径前缀 + 行列数字→@（结构不动）。"""
    行 = 路径正则.sub(r"\1", 行)
    for 正则, 替换 in 位置正则们:
        行 = 正则.sub(替换, 行)
    return 行.strip()


def 归一正文行(行: str) -> str:
    """级2：级1 基础上循环剥定位段/通道标记 + 去尾部位置注。"""
    行 = 归一位置行(行)
    for _ in range(8):          # 已知最深 3 层（[sem5]→正确诊断:→[错误]）；8 轮裕量
        原 = 行
        for 正则 in 前缀正则们:
            行 = 正则.sub("", 行, count=1)
        if 行 == 原:
            break
    行 = 尾部位置正则.sub("", 行)
    return 行.strip()


def 归一文本(原始: str) -> dict:
    """把一侧诊断原始文本归一为三分面：位置行们/正文行们/进度行数 + 框架行们。"""
    位置行们, 正文行们, 框架行们 = [], [], []
    进度行数 = 0
    for 行 in (原始 or "").replace("\r\n", "\n").splitlines():
        行 = 行.strip()
        if not 行:
            continue
        if 进度行正则.match(行):
            进度行数 += 1
            continue
        位置行 = 归一位置行(行)
        正文行 = 归一正文行(行)
        if any(正则.match(正文行) for 正则 in 框架行正则们):
            框架行们.append(正文行)
            continue
        位置行们.append(位置行)
        正文行们.append(正文行)
    return {"位置行们": 位置行们, "正文行们": 正文行们,
            "框架行们": 框架行们, "进度行数": 进度行数}


# ============ 用例枚举 ============

def 探测目标平台(显式: str) -> str:
    """与 run_e2e 主程序同款推断（第二处重复已留标记——第 3 处出现前提炼）。"""
    if 显式:
        return 显式
    if sys.platform == "win32":
        return "win-x64"
    if platform.machine() in ("x86_64", "AMD64", "amd64"):
        return "linux-x86_64"
    return "linux-arm64"


def 解析配置键们(配置路径: pathlib.Path) -> dict:
    """读用例 双编译对照.txt/v2闭环.txt 的键值（复用 run_e2e 解析器·语义权威）。"""
    return run_e2e.解析v2闭环配置(配置路径)


def 枚举负例(过滤模式: str) -> list:
    """枚举负例用例（三通道·优先级 ①>②>③ 与 run_e2e 分派一致）。

    返回 [{名称, 目录, 类别, 入口名, 诊断锚, 被检名(可选)}]。"""
    例们 = []
    for 目录 in sorted(E2E目录.iterdir()):
        if not (目录.is_dir() and 目录.name[:2].isdigit()):
            continue
        if 过滤模式 and 过滤模式 not in 目录.name:
            continue
        锚文件 = 目录 / "期望编译失败.txt"
        check文件 = 目录 / "期望check失败.txt"
        dual文件 = 目录 / "双编译对照.txt"
        dual锚 = ""
        dual是 = False
        if dual文件.exists():
            try:
                配置 = 解析配置键们(dual文件)
                dual是 = 配置.get("预期编译失败", False)
                dual锚 = 配置.get("诊断锚", "")
            except ValueError:
                dual是 = False
        if 锚文件.exists():
            例们.append({"名称": 目录.name, "目录": 目录, "类别": "期望编译失败",
                        "入口名": 查找入口名(目录),
                        "诊断锚": dual锚 if dual是 else 锚文件.read_text(
                            encoding="utf-8").strip(),
                        "dual共存": dual是})
        elif check文件.exists():
            有效行 = [x.strip() for x in check文件.read_text(encoding="utf-8").splitlines()
                     if x.strip() and not x.strip().startswith("#")]
            例们.append({"名称": 目录.name, "目录": 目录, "类别": "check失败",
                        "入口名": 有效行[0] if 有效行 else 查找入口名(目录),
                        "诊断锚": 有效行[1] if len(有效行) > 1 else "",
                        "dual共存": dual是})
        elif dual是:
            配置 = 解析配置键们(dual文件)
            例们.append({"名称": 目录.name, "目录": 目录, "类别": "dual负配",
                        "入口名": (配置.get("源文件们") or ["主.cn"])[0],
                        "诊断锚": dual锚, "dual共存": True})
    return 例们


def 查找入口名(目录: pathlib.Path) -> str:
    try:
        return run_e2e.查找源文件(目录).name
    except FileNotFoundError:
        return "主.cn"


# ============ 双侧采集 ============

def 收集宿主(例: dict, cn: pathlib.Path, 平台: str, 详细: bool) -> dict:
    """宿主侧：build（或 check）收集诊断。返回 {rc, 原始}。"""
    入口 = 例["目录"] / 例["入口名"]
    首选 = 例.get("首选命令", "build")
    if 首选 == "check":
        命令 = [str(cn), "check", str(入口), "--target", 平台]
    else:
        隔离 = 审计目录 / f"diaghost{run_e2e.隔离键(例['名称'])}"
        隔离.mkdir(parents=True, exist_ok=True)
        产物 = 隔离 / (例["名称"] + (".exe" if 平台 == "win-x64" else ""))
        if 产物.exists():
            产物.unlink()
        命令 = [str(cn), "build", str(入口), "--target", 平台, "--output", str(产物),
               *run_e2e.查找编译选项文件(例["目录"])]
    if 详细:
        print(f"    [宿主] {' '.join(命令)}")
    运行 = run_e2e.运行命令(命令, run_e2e.项目根目录,
                          超时秒数=run_e2e.编译超时秒数)
    原始 = run_e2e.解码诊断(运行.stderr) + run_e2e.解码诊断(运行.stdout)
    return {"rc": 运行.returncode, "原始": 原始}


def 收集v2(例: dict, v2p: pathlib.Path, 平台: str, 详细: bool) -> dict:
    """v2 侧：拷用例树干净→v2p 编译收集诊断（与 run_e2e dual DN 负路径同款）。

    顺序纪律：拷贝用例树干净=先 rmtree 再 copytree（快照镜像语义）——target/
    子目录**必须在其后**创建，否则被快照清理连带删除（v2p 写 target/v2asm.s
    将失败·诊断输出混入「asm 写文件失败」噪音）。stdlib 软链同后建（v2p 的
    stdlib 签名扫描按相对 cwd 读 stdlib/容器.cn——缺链则输出混入「签名扫描
    打开失败」噪音；对齐 run_e2e 执行v2闭环Linux 的 stdlib 链先例）。"""
    v2目录 = 审计目录 / f"diagv2{run_e2e.隔离键(例['名称'])}"
    源 = 例["目录"] / 例["入口名"]
    if not 源.exists():
        return {"rc": None, "原始": "", "错误": f"用例缺源文件 {例['入口名']}"}
    run_e2e.拷贝用例树干净(例["目录"], v2目录, ignore=shutil.ignore_patterns(*忽略拷贝))
    (v2目录 / "target").mkdir(parents=True, exist_ok=True)
    stdlib链 = v2目录 / "stdlib"
    if not stdlib链.exists():
        os.symlink(run_e2e.项目根目录 / "stdlib", stdlib链)
    入口 = str((v2目录 / 例["入口名"]).resolve())
    命令 = [str(v2p), 入口, 平台, *run_e2e.查找编译选项文件(例["目录"])]
    if 详细:
        print(f"    [v2] {' '.join(命令)}（cwd={v2目录}）")
    运行 = run_e2e.运行命令(命令, v2目录, 内存上限MB=run_e2e.内存上限MB默认,
                          超时秒数=run_e2e.编译超时秒数)
    原始 = run_e2e.解码诊断(运行.stderr) + run_e2e.解码诊断(运行.stdout)
    return {"rc": 运行.returncode, "原始": 原始}


# ============ 判定（纯函数·selftest 直测）============

def 判定一例(宿主: dict, v2: dict, 诊断锚: str) -> dict:
    """单例裁定：verdict + 各方归一结果 + 独有行 + 锚命中。"""
    宿主归一 = 归一文本(宿主.get("原始", ""))
    v2归一 = 归一文本(v2.get("原始", ""))
    宿主集 = set(宿主归一["正文行们"])
    v2集 = set(v2归一["正文行们"])
    交集 = 宿主集 & v2集
    rc宿, rc2 = 宿主.get("rc"), v2.get("rc")
    verdict = _裁定verdict(rc宿, rc2, 宿主集, v2集, 交集)
    return {
        "verdict": verdict,
        "宿主": {**宿主, **宿主归一},
        "v2": {**v2, **v2归一},
        "交集": sorted(交集),
        "仅宿主": sorted(宿主集 - v2集),
        "仅v2": sorted(v2集 - 宿主集),
        "严格逐行相同": 宿主归一["位置行们"] == v2归一["位置行们"],
        "锚宿主命中": (诊断锚 in 宿主.get("原始", "")) if 诊断锚 else None,
        "锚v2命中": (诊断锚 in v2.get("原始", "")) if 诊断锚 else None,
    }


def _裁定verdict(rc宿, rc2, 宿主集: set, v2集: set, 交集: set) -> str:
    if rc宿 is None or rc2 is None:
        return "采集异常"
    if rc宿 < 0 or rc2 < 0:
        return "异常退出（信号级）"
    if rc宿 == 0:
        return "宿主未拒绝（异常）"
    if rc2 == 0:
        return "v2未拒绝（行为不对等）"
    if not 宿主集 or not v2集:
        return "单侧无正文"
    if 宿主集 == v2集:
        return "一致"
    if 交集:
        return "部分一致"
    return "全异"


# ============ 报告 ============

def 汇总判定(结果们: list) -> dict:
    计数 = {}
    for 例结果 in 结果们:
        键 = 例结果["裁定"]["verdict"]
        计数[键] = 计数.get(键, 0) + 1
    return 计数


def 打印报告(结果们: list, 计数: dict, 输出路径: str, 过滤模式: str) -> None:
    print()
    print("=== 诊断文案双侧对拍（diag_dual_diff·任务 839）===")
    print(f"负例总数: {len(结果们)}" +
          (f" ｜ 过滤: {过滤模式}" if 过滤模式 else "") + " ｜ 报告制工具（不硬拦）")
    print("判定分布: " + " ｜ ".join(f"{k} {v}" for k, v in sorted(计数.items())))
    for 关注 in ("全异", "部分一致", "v2未拒绝（行为不对等）", "宿主未拒绝（异常）",
                "单侧无正文", "异常退出（信号级）", "采集异常"):
        if 计数.get(关注):
            print(f"\n--- {关注}（{计数[关注]} 例）---")
            for 例结果 in 结果们:
                if 例结果["裁定"]["verdict"] == 关注:
                    _打印例结果(例结果)
    if 计数.get("一致"):
        print(f"\n--- 一致（{计数['一致']} 例·正文集合相等）---")
        名们 = [例结果["例"]["名称"] for 例结果 in 结果们
               if 例结果["裁定"]["verdict"] == "一致"]
        for i in range(0, len(名们), 3):
            print("  " + "、".join(名们[i:i + 3]))
    严格数 = sum(1 for 例结果 in 结果们 if 例结果["裁定"]["严格逐行相同"])
    锚宿 = sum(1 for 例结果 in 结果们 if 例结果["裁定"]["锚宿主命中"])
    锚v2 = sum(1 for 例结果 in 结果们 if 例结果["裁定"]["锚v2命中"])
    有锚 = sum(1 for 例结果 in 结果们 if 例结果["裁定"]["锚宿主命中"] is not None)
    print(f"\n附加事实：严格逐行相同（连位置模式都一致）{严格数}/{len(结果们)} ｜ "
          f"诊断锚命中：宿主 {锚宿}/{有锚}·v2 {锚v2}/{有锚}")
    print("（T63 政策：v2 文案与宿主不同串属合规面——本报告只显形差异事实，不判缺陷；"
          "差异大的用例供审计关注）")
    if 输出路径:
        写完整报告(输出路径, 结果们, 计数, 过滤模式)


def _打印例结果(例结果: dict) -> None:
    例 = 例结果["例"]
    裁定 = 例结果["裁定"]
    锚面 = ""
    if 裁定["锚宿主命中"] is not None:
        锚面 = f" ｜ 锚[宿主{'✓' if 裁定['锚宿主命中'] else '✗'}" \
              f" v2{'✓' if 裁定['锚v2命中'] else '✗'}]"
    print(f"  [{裁定['verdict']}] {例['名称']}（{例['类别']}"
          f"{'·dual共存' if 例.get('dual共存') else ''}）"
          f" rc[宿主{裁定['宿主'].get('rc')} v2{裁定['v2'].get('rc')}]{锚面}")
    for 行 in (裁定["仅宿主"] or [])[:4]:
        print(f"      仅宿主: {行}")
    for 行 in (裁定["仅v2"] or [])[:4]:
        print(f"      仅v2:   {行}")
    if 裁定["交集"]:
        print(f"      （正文交集 {len(裁定['交集'])} 行，如: {裁定['交集'][0][:60]}）")


def 写完整报告(路径: str, 结果们: list, 计数: dict, 过滤模式: str) -> None:
    """全量报告落盘（附加输出面·写失败显式警告不改判定）。"""
    行们 = ["# 诊断文案双侧对拍报告（diag_dual_diff·任务 839）", "",
           f"- 负例总数: {len(结果们)}" + (f" ｜ 过滤: {过滤模式}" if 过滤模式 else ""),
           "- 判定分布: " + " ｜ ".join(f"{k} {v}" for k, v in sorted(计数.items())),
           "- 口径: 报告制（不硬拦·不改 run_e2e 门禁）；T63 政策下 v2 文案与宿主"
           "不同串属合规面，本报告只显形差异事实供审计。", ""]
    for 例结果 in 结果们:
        例, 裁定 = 例结果["例"], 例结果["裁定"]
        行们.append(f"## [{裁定['verdict']}] {例['名称']}（{例['类别']}）")
        行们.append(f"- rc: 宿主 {裁定['宿主'].get('rc')} / v2 {裁定['v2'].get('rc')}"
                   f" ｜ 严格逐行相同: {裁定['严格逐行相同']}")
        if 裁定["锚宿主命中"] is not None:
            行们.append(f"- 诊断锚[{例['诊断锚']}]: 宿主"
                       f"{'✓' if 裁定['锚宿主命中'] else '✗'} / v2"
                       f"{'✓' if 裁定['锚v2命中'] else '✗'}")
        if 裁定["仅宿主"]:
            行们.append("- 仅宿主行:")
            行们 += [f"  - {x}" for x in 裁定["仅宿主"][:10]]
        if 裁定["仅v2"]:
            行们.append("- 仅v2行:")
            行们 += [f"  - {x}" for x in 裁定["仅v2"][:10]]
        if 裁定["交集"]:
            行们.append("- 两侧一致行:")
            行们 += [f"  - {x}" for x in 裁定["交集"][:10]]
        if 裁定["宿主"].get("框架行们") or 裁定["v2"].get("框架行们"):
            行们.append(f"- 框架行: 宿主 {裁定['宿主'].get('框架行们')} / "
                       f"v2 {裁定['v2'].get('框架行们')}")
        行们.append(f"- v2 进度行剔除 {裁定['v2'].get('进度行数', 0)} 行"
                   f"（原始输出保留于 target/audit2/diagv2*，可查）")
        行们.append("")
    try:
        pathlib.Path(路径).write_text("\n".join(行们) + "\n", encoding="utf-8")
        print(f"\n完整报告已写入: {路径}")
    except OSError as 异常:
        print(f"\n警告: 完整报告写入失败（{异常}）——不影响本次对拍结论")


# ============ 自测（真实样本锚定的归一化正反态 + 判定表）============

def selftest() -> int:
    通过 = 失败 = 0

    def 签(名: str, 条件: bool, 详情: str = "") -> None:
        nonlocal 通过, 失败
        if 条件:
            通过 += 1
            print(f"  [签] {名}")
        else:
            失败 += 1
            print(f"  [X]  {名} {详情}")

    # —— 归一化：真实样本四形态（宿主语义/双编译 v2 语义/v2 语法/宿主框架）——
    宿主语义 = "tests/e2e/101_接口未实现赋值/主.cn:19:5: 错误: 无法将 '无接口对象' 隐式转换为 '形状'"
    v2语义 = "[sem5] 正确诊断: [错误] 无法将 '无接口对象' 隐式转换为 '形状'（行=19）"
    签("宿主语义行→正文", 归一正文行(宿主语义) == "无法将 '无接口对象' 隐式转换为 '形状'",
       f"得到={归一正文行(宿主语义)!r}")
    签("v2语义行→同正文", 归一正文行(v2语义) == "无法将 '无接口对象' 隐式转换为 '形状'",
       f"得到={归一正文行(v2语义)!r}")
    签("双侧语义正文相等", 归一正文行(宿主语义) == 归一正文行(v2语义))

    v2语法 = "[parse] 行 9: 保留字不可作变量名（声明名位遇到关键字「自身」）"
    签("v2语法行剥[parse]与行号", 归一正文行(v2语法)
       == "保留字不可作变量名（声明名位遇到关键字「自身」）",
       f"得到={归一正文行(v2语法)!r}")

    宿主多错 = "tests/e2e/638_自身保留字负测/主.cn:6:14: 错误: 结构体字段预期名称"
    签("宿主多错行剥路径位置", 归一正文行(宿主多错) == "结构体字段预期名称",
       f"得到={归一正文行(宿主多错)!r}")

    # —— 归一化·反态：文案中的数字不被吃（只动定位数字）——
    签("文案内数字不动", 归一正文行("主.cn:3:1: 错误: 至少需要 2 个参数")
       == "至少需要 2 个参数", f"得到={归一正文行('主.cn:3:1: 错误: 至少需要 2 个参数')!r}")

    # —— 进度行/框架行归类 ——
    归一 = 归一文本("[p6h] 已解析: /x/y/主.cn 节点=42\n"
                   "[sem5] 正确诊断: [错误] 真诊断（行=5）\n"
                   "[sem5] 语义错误 1 个，中止 IR 生成与代码生成（退出码 1）\n")
    签("进度行剔除计数", 归一["进度行数"] == 1, f"进度={归一['进度行数']}")
    签("框架行归类", 归一["框架行们"]
       == ["语义错误 1 个，中止 IR 生成与代码生成（退出码 1）"], f"框架={归一['框架行们']}")
    签("正文仅真诊断", 归一["正文行们"] == ["真诊断"], f"正文={归一['正文行们']}")

    # —— 判定：一致（101 实况：宿主 1 行 + v2 1 行正文 + v2 汇总框架行）——
    宿主 = {"rc": 1, "原始": 宿主语义}
    v2 = {"rc": 1, "原始": v2语义 + "\n[sem5] 语义错误 1 个，中止 IR 生成与代码生成（退出码 1）"}
    裁定 = 判定一例(宿主, v2, "无法将 '无接口对象' 隐式转换为 '形状'")
    签("101 实况=一致", 裁定["verdict"] == "一致", f"verdict={裁定['verdict']}")
    签("双锚命中", 裁定["锚宿主命中"] and 裁定["锚v2命中"])

    # —— 判定：全异（638 实况：语法层文案完全不同）——
    宿主 = {"rc": 1, "原始": "tests/e2e/638_自身保留字负测/主.cn:9:9: 错误: 预期变量名，实际为 '自身'\n"
                            "错误: 模块 '主' 解析失败\n"}
    v2 = {"rc": 1, "原始": "[parse] 行 9: 保留字不可作变量名（声明名位遇到关键字「自身」）\n"
                        "[parse] 语法错误 1 个，中止编译（不产 IR/asm）\n"}
    裁定 = 判定一例(宿主, v2, "自身")
    签("638 实况=全异", 裁定["verdict"] == "全异", f"verdict={裁定['verdict']}")
    签("全异样本两侧非空", 裁定["仅宿主"] and 裁定["仅v2"])

    # —— 判定：部分一致 ——
    宿主 = {"rc": 1, "原始": "主.cn:1:1: 错误: 甲错误\n主.cn:2:1: 错误: 乙错误\n"}
    v2 = {"rc": 1, "原始": "[sem5] 正确诊断: [错误] 乙错误（行=2）\n"}
    裁定 = 判定一例(宿主, v2, "")
    签("部分一致", 裁定["verdict"] == "部分一致", f"verdict={裁定['verdict']}")

    # —— 判定：v2 未拒绝（202 实况形态：v2p rc=0 放行）——
    裁定 = 判定一例({"rc": 1, "原始": "主.cn:16:11: 错误: 同一调用中变量 'x' 被可变借用两次"},
                  {"rc": 0, "原始": "[p6h] 已解析: /x/主.cn 节点=42\n"}, "")
    签("v2未拒绝显形", 裁定["verdict"] == "v2未拒绝（行为不对等）",
       f"verdict={裁定['verdict']}")

    # —— 判定：单侧无正文（v2 拒绝但零诊断）——
    裁定 = 判定一例({"rc": 1, "原始": "主.cn:1:1: 错误: 甲错误\n"},
                  {"rc": 1, "原始": "[p6h] 已解析: /x/主.cn 节点=1\n"}, "")
    签("单侧无正文", 裁定["verdict"] == "单侧无正文", f"verdict={裁定['verdict']}")

    # —— 判定：采集异常（用例缺源文件）——
    裁定 = 判定一例({"rc": 1, "原始": "错误: 甲\n"},
                  {"rc": None, "原始": "", "错误": "用例缺源文件 主.cn"}, "")
    签("采集异常显形", 裁定["verdict"] == "采集异常", f"verdict={裁定['verdict']}")

    # —— 与上游 run_e2e import 的连通性（机制单点权威）——
    签("run_e2e 机制连通", callable(run_e2e.运行命令) and callable(run_e2e.解码诊断)
       and run_e2e.编译超时秒数 > 0)

    print(f"\nselftest: {通过} 通过 / {失败} 失败")
    return 0 if 失败 == 0 else 1


# ============ 主流程 ============

def 主程序() -> int:
    参数 = _解析参数()
    if 参数.selftest:
        return selftest()
    平台 = 探测目标平台(参数.target)
    编译器 = _探测编译器(参数.cn)
    if not 编译器.exists():
        print(f"错误: 未找到宿主编译器 {编译器}（先构建 target/cn）")
        return 2
    例们 = 枚举负例(参数.filter or "")
    if not 例们:
        print(f"错误: 无匹配负例（过滤『{参数.filter or ''}』）")
        return 2
    print(f"负例枚举: {len(例们)} 例 ｜ 平台 {平台} ｜ 编译器 {编译器} ｜ "
          f"（编译，v2p 首跑请稍候）", flush=True)
    就绪 = _v2p就绪(编译器, 平台, 参数.verbose)
    if 就绪 is None:
        return 2
    v2p = 就绪
    结果们 = []
    for i, 例 in enumerate(例们, 1):
        print(f"[{i}/{len(例们)}] {例['名称']}（{例['类别']}）", flush=True)
        例["首选命令"] = "check" if 例["类别"] == "check失败" else "build"
        宿主 = 收集宿主(例, 编译器, 平台, 参数.verbose)
        v2 = 收集v2(例, v2p, 平台, 参数.verbose)
        结果们.append({"例": 例, "裁定": 判定一例(宿主, v2, 例.get("诊断锚", ""))})
    计数 = 汇总判定(结果们)
    打印报告(结果们, 计数, 参数.输出, 参数.filter or "")
    return 0


def _解析参数():
    解析器 = argparse.ArgumentParser(
        description="诊断文案双侧对拍（任务 839·报告制）",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="示例:\n"
               "  python3 scripts/diag_dual_diff.py --filter 10    # 过滤子集\n"
               "  python3 scripts/diag_dual_diff.py --输出 target/diag_dual_报告.txt\n"
               "  python3 scripts/diag_dual_diff.py --selftest")
    解析器.add_argument("--filter", default=None, help="按用例目录名子串过滤（与 run_e2e 同款）")
    解析器.add_argument("--cn", default=None, help="宿主编译器路径（默认自动探测 target/…）")
    解析器.add_argument("--target", default=None, help="目标平台（默认按本机推断）")
    解析器.add_argument("--输出", dest="输出", default=None, metavar="路径",
                       help="完整报告落盘路径（附加输出面）")
    解析器.add_argument("--verbose", "-v", action="store_true", help="打印执行命令")
    解析器.add_argument("--selftest", action="store_true", help="引擎自测（零触碰真实树）")
    return 解析器.parse_args()


def _探测编译器(显式: str):
    if 显式:
        return pathlib.Path(显式 if pathlib.Path(显式).is_absolute()
                           else 项目根目录 / 显式)
    for 候选 in run_e2e.编译器候选:
        p = 项目根目录 / 候选
        if p.exists():
            return p
    return 项目根目录 / "target" / "cn"


def _v2p就绪(编译器, 平台: str, 详细: bool):
    if 平台 == "win-x64":
        就绪 = run_e2e.确保v2p就绪win(编译器, 详细, "D839")
    else:
        就绪 = run_e2e.确保v2p与运行时就绪(编译器, 平台, 详细, "D839")
    if 就绪[0] is None:
        print(f"错误: v2p 就绪失败：{就绪[1]}")
        return None
    print(f"v2p 就绪: {就绪[0]}")
    return 就绪[0]


if __name__ == "__main__":
    # 555 树锁：本工具直调 v2p 构建/宿主编译（写 target/audit2 缓存）——整运行期
    #   持本树锁（与 run_e2e/gate_quick 互斥防缓存互踩·#156 教训）；CN_NO_TREE_LOCK
    #   逃生门与 CN_TREE_LOCK_HELD 嵌套豁免由 gate_lock 入口统一处理（先例=run_e2e）。
    import gate_lock as _树锁
    sys.exit(_树锁.树锁运行入口(主程序))
