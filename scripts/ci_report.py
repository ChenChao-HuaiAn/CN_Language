#!/usr/bin/env python3
# 云 CI 报告与留档面（1297 自 ci_daemon.py 拆出·原样搬移+初始化注入）：
#   落盘（develop/预验/兜底三档留档规则）/上报（TX_01 结果页）/上报执行中（在跑轮
#   实时可见）/免跑留档（189 纯文档轮留档）/取提交历史+提交时刻（842/1070 归因与
#   排序数据源）——拆出动机=ci_daemon 加分片时守 1000 行硬红线（AGENTS §3
#   「先拆再加」·973 外迁 ci_build 同款）。
# 依赖方向：ci_daemon → 本模块（单向·本模块禁 import ci_daemon）；仓库根/平台/
#   日志目录/远端名 由 ci_daemon 启动时 初始化() 注入（模块全局·daemon 单线程使用
#   与原语义一致）。

import json
import os
import subprocess
import urllib.request
from datetime import datetime
from pathlib import Path

_仓库根: Path = Path(__file__).resolve().parent.parent
_平台 = ""
_日志目录 = _仓库根 / "ci-logs"
_远端名 = "origin"


def 初始化(仓库根: Path, 平台: str, 日志目录: Path, 远端名: str) -> None:
    """ci_daemon 启动时注入运行环境（原模块全局引用改经本模块全局）。"""
    global _仓库根, _平台, _日志目录, _远端名
    _仓库根, _平台, _日志目录, _远端名 = 仓库根, 平台, 日志目录, 远端名


def 执行机字段(runner: str, 构建并行: int, 是win: bool) -> dict:
    """1299：门禁结果执行机归因字段（构建段 63s↔372s 查因必需——同 sha 同平台不同
    runner/作业数可差数倍；详情无执行机只能翻各机 ci-logs 考古）。
    win 侧 Ninja `--parallel` 用全核，`CN_BUILD_JOBS` 不适用→不写入（不虚报默认 2）。"""
    字段 = {"runner": runner}
    if not 是win:
        字段["CN_BUILD_JOBS"] = 构建并行
    return 字段


def 上报(结果: dict) -> None:
    """可选上报 TX_01 队列服务——URL/token 从环境变量或 ci_config.json 读；不可达仅警告。"""
    配置 = {}
    配置文件 = _仓库根 / "ci_config.json"
    if 配置文件.exists():
        try:
            配置.update(json.loads(配置文件.read_text(encoding="utf-8")))
        except Exception:
            pass
    url = os.environ.get("CN_CI_REPORT_URL") or 配置.get("上报地址")
    if not url:
        return
    令牌 = os.environ.get("CN_CI_TOKEN") or 配置.get("令牌", "")
    try:
        请求 = urllib.request.Request(url, data=json.dumps(结果, ensure_ascii=False).encode("utf-8"),
                                      headers={"Content-Type": "application/json",
                                               "Authorization": "Bearer " + 令牌})
        urllib.request.urlopen(请求, timeout=10)
    except Exception as e:
        print("[警告] 上报失败（不影响本地结果）：%s" % e, flush=True)


def 落盘(结果: dict) -> None:
    """结果落盘+可选上报（1008·预验模式规则·770 修订）：
    - develop 常规轮：latest.json + result_*.json + 上报（既有行为）。
    - 非 develop 分支（CN_CI_BRANCH 预验）：
      · 恒写 预验_<sha10>.json（integrate.py 轮询取回的契约文件）+ result_*.json（历史档）；
      · ci/预验-* 且绿 → 额外写 latest.json+上报：链顶即将成为 develop 头（1008 方案甲；
        770 起 develop 面仍复验·此处 latest 语义=「最近被验证的头」供免跑判据）；
      · ci/预验-* 且红 → 不写 latest 不上报（develop 未收到该提交——latest 必须保持 develop 语义）；
      · 任务分支预验（028 §三·PR CI 同款）→ 上报（带 分支 字段·结果页可区分），不写 latest。
    """
    文本 = json.dumps(结果, ensure_ascii=False, indent=1)
    sha10 = 结果["sha"][:10]
    当前分支 = 结果.get("分支", "develop")
    if 当前分支 == "develop":
        (_日志目录 / "latest.json").write_text(文本, encoding="utf-8")
        (_日志目录 / ("result_%s_%s.json" % (sha10,
                      datetime.now().strftime("%m%d_%H%M%S")))).write_text(
            文本, encoding="utf-8")
        上报(结果)
        return
    (_日志目录 / ("预验_%s.json" % sha10)).write_text(文本, encoding="utf-8")
    (_日志目录 / ("result_%s_%s.json" % (sha10,
                  datetime.now().strftime("%m%d_%H%M%S")))).write_text(
        文本, encoding="utf-8")
    if 当前分支.startswith("ci/预验-"):
        if 结果["绿"]:
            (_日志目录 / "latest.json").write_text(文本, encoding="utf-8")
            上报(结果)
    else:
        上报(结果)


def 上报执行中(sha: str, 分支: str, 平台: str) -> None:
    """190（1035 轮）：开跑即上报「执行中」——状态页门禁表实时可见在跑轮（1030 误判
    「漏跑」根因=执行中不可见）；完成轮由 落盘 上报覆盖同 sha 行（INSERT OR REPLACE）。
    1070：随行「提交时刻」——门禁排序键（执行中行同享提交序位）。"""
    上报({"sha": sha, "分支": 分支, "平台": 平台, "绿": None, "总秒": None,
          "状态": "执行中", "提交时刻": 提交时刻(sha)})


def 免跑留档(sha: str, 缘由: str) -> None:
    """纯文档轮免跑留档（189·770 保留）：走既有 result 档案机制（develop 语义=latest+result+
    上报）——本地 result 档据此防重复入池=幂等零重复判定；结果页可审计（绿=门禁无需全量·
    步骤字段明记免跑缘由）。绿 的口径：result「绿」由 同步/配置/构建/单测/e2e 键聚合
    （跑一轮 同款），免跑结果无这些键→聚合为 True——语义=「本 sha 无需全量即合规」。"""
    步骤字段 = {"免跑": {"rc": 0, "说明": "纯文档轮免兜底（189）·" + 缘由}}
    结果 = {"sha": sha, "分支": "develop", "平台": _平台, "绿": True,
            "步骤": 步骤字段,
            "总秒": 0, "时刻": datetime.now().isoformat(timespec="seconds"),
            "日志": "", "日志尾部": ["[免跑] %s %s" % (sha[:10], 缘由)]}
    _日志目录.mkdir(exist_ok=True)     # 189 实测补：冷启动树 ci-logs 未建时免跑留档曾崩
    落盘(结果)
    print("[免跑] %s %s（留档可审计）" % (sha[:10], 缘由), flush=True)


def 取提交历史(条数: int = 100) -> list:
    """842：最近 N 条提交（sha+主题）——develop 验证红归因数据源（8300 页面
    「自上次绿以来并入」区）。git 失败/异常=空列表（归因是附加信息：缺失时
    页面降级提示，不炸轮不谎报）。encoding 显式 utf-8（win 默认 locale 解码
    git 输出的中文提交信息必乱码）。"""
    try:
        r = subprocess.run(["git", "log", "-%d" % 条数, "--format=%H%x09%s"],
                           capture_output=True, text=True, encoding="utf-8",
                           errors="replace", cwd=_仓库根, timeout=60)
        if r.returncode != 0:
            return []
        出 = []
        for 行 in (r.stdout or "").splitlines():
            部分 = 行.split("\t", 1)
            if len(部分) == 2:
                出.append({"sha": 部分[0], "主题": 部分[1]})
        return 出
    except Exception:
        return []


def 提交时刻(sha: str) -> int | None:
    """1070：sha 的提交 epoch 秒（committer date）——8300 门禁表排序键（提交序取代
    完成时刻序·池并行下完成时刻与提交谱系无关）。git 出错/输出非纯数字=None 显式
    哨兵：服务端排序 COALESCE 分流沉底（宁缺不错排·不静默取假值）。"""
    try:
        r = subprocess.run(["git", "log", "-1", "--format=%ct", sha],
                           capture_output=True, text=True, encoding="utf-8",
                           errors="replace", cwd=_仓库根, timeout=30)
        if r.returncode != 0:
            return None
        文本 = (r.stdout or "").strip()
        return int(文本) if 文本.isdigit() else None
    except Exception:
        return None
