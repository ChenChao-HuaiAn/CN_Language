#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_duration.py —— 门禁步骤耗时滚动基线（任务 #1385·「静默劣化」机械告警面）。

背景（#1299 查因结论）：预验构建段 63s→372s 是靠人眼翻门禁表发现的，不是被告警抓到的；
  同 sha 同平台在不同执行机/构建并行度/编译器缓存命中态下耗时可差数倍（1299 增设
  runner/CN_BUILD_JOBS 归因字段正是为此），因此基线**必须按执行机与口径分桶**。

口径（单一事实源·键=(平台, 步骤, 口径, 执行机)）：
  · 样本=步骤记录里的 `秒` 字段（runner 既有形状）+ #1385 起的增量字段（分片数/
    分片形态/缓存态/负载档/构建并行度——全部可选·缺失落显式未知档）；仅收 rc=0、
    非超时、有限正数样本；
  · 构建/配置 → 冷建/温建（清产物成功或 win 指纹清建/首次配置）；构建另加
    编译器缓存命中档（ccache -s / sccache --show-stats 的构建前后命中率）与
    构建起点宿主负载档（同宿主多实例抢 CPU 可见面·#1385 P1-3）；
  · e2e/e2e并行 → 单实例 / 分片N·分布式墙钟 / 分片N·退化单实例（P1-1：召唤失败退化为
    串行与并行耗时不可混算；形态由 ci_shard 透传，缺失=形态未知独立桶；
    #1632 起「分布式墙钟」=真实墙钟含收片等待——与 #1632 前 max(片秒) 历史桶隔离）。
  · check_asm_width → 脚本经 CN_STEP_META_DIR 回传「全扫/目录扫N·用例M·目录摘要H·
    缓存冷热」（P1-2：目录数相同但用例/目录不同=不同成本，同桶会 15s↔1.5s 误报）；
  · 其余步骤 → 常规·执行机。
  · 执行机维度=结果.runner（linux 附 CN_BUILD_JOBS；win 报「全核N」并行度）。

判据：
  · 告警 = 当前 > 3×近 20 次中位 **且** 增量 > 10s（CN_DURATION_ALERT_RATIO/DELTA）；
  · 慢漂 = 最近 5 次中位 ÷ 前 15 次中位 > 1.5×，或 ÷ 更早 15 次中位 > 1.5×
    （P1-4：20 次滚动中位对 60→180s 分 40 轮爬升零告警——独立漂移判据补盲）；
  · 去抖 = 构建/配置连续 2 轮超阈才告警；首轮超阈落「待确认」（P1-3；
    冷建双峰/换机等一次性尖峰不升级为告警）；
  · 样本 < 5（CN_DURATION_MIN_SAMPLES）=显式「基线不足」，不告警也不静默；
  · 坏值（NaN/Inf）=显式「不足/形态坏」，不参与中位；落库 JSON 一律 allow_nan=False。

数据面：
  · 服务端（权威）：门禁表 `耗时告警` 列（JSON 对象）+ `耗时样本` 表（滚动样本·
    超阈/epoch 列·30 天过期清理）；/api/report 写入时评估落库（门禁写入字段），
    快照/详情把告警、慢漂、平台/时刻带给 8300 门禁窗口与 8301 看板；
  · runner 本地（断网降级）：ci_report.落盘 前调 本地补充——本树 ci-logs/耗时基线.json
    原子累积样本（坏文件先改名 .坏<时间戳> 留档再重建），结果 JSON 带 `耗时告警`
    （来源=runner本机）；服务端可达时以服务端对象为准。
  · 配置 CN_DURATION_* 惰性解析：非法值=stderr 黄字+文档默认并记入评估对象
    「配置警告」（不拖垮服务/runner 启动·不静默）。

边界：Python 脚本与 C++/v2 编译器实现分离（本模块只服务 CI 观测面）；
  check_asm_width 口径元信息经 CN_STEP_META_DIR 环境目录回传（daemon 步骤记录器读取），
  元信息缺失=口径不足显式条目，不猜测。
"""
from __future__ import annotations

import json
import math
import os
import sqlite3
import sys
import time
from pathlib import Path

import atomic_json   # 556：本机基线文件原子读写（半写损坏防护·读坏黄字）

# ===== 配置（惰性解析·非法值显式告警后用文档默认，不拖垮服务启动） =====
默认配置 = {"样本": 20, "最小": 5, "倍数": 3.0, "增量": 10.0,
            "保留": 60, "保留天": 30}
_配置缓存 = {"键": None, "值": None}


def _数值(名: str, 默认, 转换, 警告: list) -> object:
    原 = os.environ.get(名)
    if 原 is None or not str(原).strip():
        return 默认
    try:
        值 = 转换(原)
    except (TypeError, ValueError):
        警告.append("环境变量 %s=%r 非法——按默认 %r（显式警告·不静默）"
                    % (名, 原, 默认))
        print("[黄] " + 警告[-1], file=sys.stderr, flush=True)
        return 默认
    return 值


def _取界(名: str, 默认, 转换, 下限, 上限, 警告: list):
    """读数值环境变量并按界校验：越界=黄字+默认（P2-7：不静默钳到边界值）。"""
    值 = _数值(名, 默认, 转换, 警告)
    if 值 < 下限 or 值 > 上限:
        警告.append("环境变量 %s=%r 越界（允许 %r..%r）——按默认 %r"
                    "（显式警告·不静默钳）" % (名, 值, 下限, 上限, 默认))
        print("[黄] " + 警告[-1], file=sys.stderr, flush=True)
        return 默认
    return 值


def 取配置() -> dict:
    """惰性读 CN_DURATION_*（环境变化才重算）；非法/越界=黄字+默认并记警告。
    P2-7：SAMPLES 上限 100·RATIO 必须 >1·负值/越界一律显式回落默认（不静默钳）。"""
    键 = tuple(os.environ.get(名) for 名 in
              ("CN_DURATION_SAMPLES", "CN_DURATION_MIN_SAMPLES",
               "CN_DURATION_ALERT_RATIO", "CN_DURATION_ALERT_DELTA",
               "CN_DURATION_DRIFT_SAMPLES", "CN_DURATION_RETAIN_DAYS"))
    if _配置缓存["键"] == 键 and _配置缓存["值"] is not None:
        return _配置缓存["值"]
    警告: list[str] = []
    配置 = {
        "样本": _取界("CN_DURATION_SAMPLES", 默认配置["样本"], int,
                     1, 100, 警告),
        "最小": _取界("CN_DURATION_MIN_SAMPLES", 默认配置["最小"], int,
                     1, 100, 警告),
        "倍数": _取界("CN_DURATION_ALERT_RATIO", 默认配置["倍数"], float,
                     1.01, 100.0, 警告),
        "增量": _取界("CN_DURATION_ALERT_DELTA", 默认配置["增量"], float,
                     0.0, 86400.0, 警告),
        "保留": _取界("CN_DURATION_DRIFT_SAMPLES", 默认配置["保留"], int,
                     20, 1000, 警告),
        "保留天": _取界("CN_DURATION_RETAIN_DAYS", 默认配置["保留天"], int,
                       0, 3650, 警告),
        "警告": 警告,
    }
    if 配置["最小"] > 配置["样本"]:
        警告.append("CN_DURATION_MIN_SAMPLES(%d) > CN_DURATION_SAMPLES(%d)——"
                    "按默认最小 %d（显式）"
                    % (配置["最小"], 配置["样本"], min(默认配置["最小"], 配置["样本"])))
        print("[黄] " + 警告[-1], file=sys.stderr, flush=True)
        配置["最小"] = min(默认配置["最小"], 配置["样本"])
    if 配置["保留"] < 配置["样本"] + 15:
        新保留 = 配置["样本"] + 15
        警告.append("CN_DURATION_DRIFT_SAMPLES(%d) < 样本窗+15——提升到 %d"
                    "（漂移基准所需·显式）" % (配置["保留"], 新保留))
        print("[黄] " + 警告[-1], file=sys.stderr, flush=True)
        配置["保留"] = 新保留
    _配置缓存["键"], _配置缓存["值"] = 键, 配置
    return 配置


def 判据文本(配置: dict | None = None) -> str:
    配置 = 配置 or 取配置()
    return ("告警=当前 > %.1f×近 %d 次中位且增量>%ss；慢漂=最近 5 次中位÷前 15 次"
            "（或更早 15 次）中位 >1.5×；构建/配置连续 2 轮超阈才告警；"
            "样本≥%d 才评估（不足=显式基线不足）；仅计 rc=0 非超时有限值步骤"
            % (配置["倍数"], 配置["样本"], ("%g" % 配置["增量"]), 配置["最小"]))


# 需脚本自报口径的步骤（耗时不只由代码决定·与写集/缓存态强相关）
需元信息步骤 = frozenset(("check_asm_width",))
冷建步骤 = frozenset(("配置", "构建"))
分片步骤 = frozenset(("e2e", "e2e并行"))
去抖步骤 = frozenset(("配置", "构建"))

漂移窗 = 5          # 最近 5 次
漂移基准 = 15       # 前 15 次 / 更早 15 次
漂移倍数 = 1.5

# 本机基线文件名（ci-logs/ 下·runner 私有）
本地基文件名 = "耗时基线.json"
_本机键分隔 = "\x1f"   # 步骤/口径 复合键分隔（JSON 键安全）


def 中位数(值们: list) -> float | None:
    """中位数（偶数=中间两值均值）；空= None（调用方落「基线不足」显式态）。"""
    排 = sorted(float(x) for x in 值们)
    n = len(排)
    if n == 0:
        return None
    mid = n // 2
    return 排[mid] if n % 2 else (排[mid - 1] + 排[mid]) / 2.0


def _有限正数(值) -> bool:
    """样本严格正数（0 样本不入基线）。"""
    return (isinstance(值, (int, float)) and not isinstance(值, bool)
            and math.isfinite(float(值)) and float(值) > 0)


def _有限非负(值) -> bool:
    """当前秒可为零（<0.5s 取整）——0 是短步骤中性值，负数/NaN/Inf 才是形态坏。"""
    return (isinstance(值, (int, float)) and not isinstance(值, bool)
            and math.isfinite(float(值)) and float(值) >= 0)


def _漂移判定(有效: list, 样本窗: int, 当前秒: float) -> dict | None:
    """慢漂：最近 5 次中位 vs 前 15 次（样本窗内）/更早 15 次（样本窗外）中位 >1.5×。
    仅样本数≥样本窗（20）时评估；P2-1：另要求当前轮仍 >=1.2×基准——劣化恢复后
    近期窗仍含高值时不残留黄签。返回 {类型,最近中位,基准中位,倍数,原因} 或 None。"""
    if len(有效) < 样本窗:
        return None
    最近 = 中位数(有效[-漂移窗:])
    if 最近 is None or 最近 <= 0:
        return None

    def 比(基准们, 类型说明, 位置说明):
        if len(基准们) < 漂移基准:
            return None
        基准 = 中位数(基准们)
        if 基准 is None or 基准 <= 0:
            return None
        倍 = 最近 / 基准
        if 倍 <= 漂移倍数:
            return None
        if 当前秒 < 基准 * 1.2:   # 恢复抑制：当前轮已回落=不残留慢漂黄签
            return None
        return {"类型": 类型说明,
                "最近中位": round(最近, 1), "基准中位": round(基准, 1),
                "倍数": round(倍, 2),
                "原因": "最近 5 次中位 %.1fs > %s %.1fs 的 %.2f×（慢漂黄签·不升级红）"
                        % (最近, 位置说明, 基准, 倍)}

    窗内前 = 有效[-样本窗:-漂移窗]
    结 = 比(窗内前, "近期", "前 15 次")
    if 结:
        return 结
    更早 = 有效[:-样本窗][-漂移基准:]
    return 比(更早, "长期", "更早 15 次")


def 评估数值(样本们: list, 当前秒: float, *, 配置: dict | None = None) -> dict:
    """单步骤评估（纯函数·selftest 四态锚）：返回
    {状态: 正常|告警|慢漂|不足, 中位, 倍数, 增量, 样本数, 原因?, 漂移?}。
    告警/中位只用最近 N=20 次；慢漂用完整保留样本（默认 60）的近期/长期中位比较。
    P2-3：当前秒 0 合法（<0.5s 取整·短步骤中性），仅负数/NaN/Inf=形态坏。"""
    配置 = 配置 or 取配置()
    有效 = [float(x) for x in 样本们 if _有限正数(x)]
    当前 = float(当前秒) if _有限非负(当前秒) else None
    if 当前 is None:
        return {"状态": "不足", "中位": None, "倍数": None, "增量": None,
                "样本数": len(有效), "原因": "当前秒为负/NaN/Inf（结果形态坏·显式）"}
    窗口 = 有效[-配置["样本"]:]
    n = len(窗口)
    if n < 配置["最小"]:
        return {"状态": "不足", "中位": None, "倍数": None, "增量": None,
                "样本数": n, "原因": "样本 %d < 最小 %d（基线不足·显式不静默）"
                % (n, 配置["最小"])}
    中位 = 中位数(窗口)
    if 中位 is None or 中位 <= 0:
        return {"状态": "不足", "中位": 中位, "倍数": None, "增量": None,
                "样本数": n, "原因": "中位不可用（样本形态异常）"}
    倍 = 当前 / 中位
    增 = 当前 - 中位
    if 倍 > 配置["倍数"] and 增 > 配置["增量"]:
        return {"状态": "告警", "中位": round(中位, 1), "倍数": round(倍, 1),
                "增量": round(增, 1), "样本数": n,
                "原因": "当前 %.1fs > %.1f×中位 %.1fs 且增量 %.1fs > %ss"
                % (当前, 配置["倍数"], 中位, 增, ("%g" % 配置["增量"]))}
    结 = {"状态": "正常", "中位": round(中位, 1), "倍数": round(倍, 1),
          "增量": round(增, 1), "样本数": n}
    漂 = _漂移判定(有效, 配置["样本"], 当前)
    if 漂:
        结["状态"] = "慢漂"
        结["漂移"] = 漂
        结["原因"] = 漂["原因"]
    return 结


def _有清产物(步骤们: dict) -> bool:
    """预验 637 清产物成功=构建冷建（linux 侧；win 另有指纹清建标记）。"""
    清 = 步骤们.get("清产物")
    return isinstance(清, dict) and 清.get("rc") == 0


def 取分片数(结果: dict) -> int:
    """分片数：优先结果.分片数（#1385 起主实例轮上报）；旧版回退 e2e明细.分片 行数+1
    （主实例片 0 不在服务端分片行内）；皆无=0=单实例口径（显式默认·不猜测）。"""
    值 = 结果.get("分片数")
    if isinstance(值, int) and not isinstance(值, bool) and 值 > 0:
        return 值
    明细 = 结果.get("e2e明细")
    if isinstance(明细, dict) and isinstance(明细.get("分片"), list) and 明细["分片"]:
        return len(明细["分片"]) + 1
    return 0


def _并行描述(结果: dict) -> str:
    """执行机并行度：linux=CN_BUILD_JOBS；win=「全核N」构建并行度；缺失=并行未知。"""
    jobs = 结果.get("CN_BUILD_JOBS")
    if isinstance(jobs, int) and not isinstance(jobs, bool):
        return "%d并行" % jobs
    描述 = str(结果.get("构建并行度") or "").strip()
    return 描述 or "并行未知"


def 推导口径(步骤名: str, 步: dict, 结果: dict) -> tuple[str, str]:
    """→(口径串, 不足原因)。不足原因非空=该步骤无法分桶（如执行机/脚本口径缺失）。"""
    runner = str(结果.get("runner") or "").strip()
    if not runner:
        return "", "执行机未归因（旧版 runner 无 runner 字段）——不混算不告警"
    机 = "runner=" + runner
    if 步骤名 in 冷建步骤:
        冷 = bool(步.get("冷建")) or _有清产物(结果.get("步骤") or {})
        前 = ("冷建" if 冷 else "温建") + "·" + 机 + "·" + _并行描述(结果)
        if 步骤名 == "构建":
            # P1-3：编译器缓存命中档 + 构建起点负载档（缺失=显式未知档·独立桶）
            缓存 = str(步.get("缓存态") or "缓存未知").strip()
            负载 = str(步.get("负载档") or "负载未知").strip()
            return 前 + "·" + 缓存 + "·" + 负载, ""
        return 前, ""
    if 步骤名 in 分片步骤:
        片 = 取分片数(结果)
        if 片 > 1:
            形态 = str(结果.get("分片形态") or 步.get("分片形态") or "").strip()
            if 形态 == "分片":
                # #1632（plans/034 §5A ④）：e2e 秒=真实墙钟（含收片等待）——桶名带
                #   「墙钟」与 max(片秒) 历史样本隔离（#1385 口径变更须分桶·防混算
                #   误告警；旧桶冻结无新样本·新桶样本<5=基线不足显式不告警）。
                形态档 = "分布式墙钟"
            elif 形态 == "退化单实例":
                # #1632 起退化 e2e 秒同取真实墙钟（原=串行 sum·差=片间调度间隙
                #   秒级）——近似语义本就≈墙钟，同桶继续积累不分裂。
                形态档 = "退化单实例"
            else:
                形态档 = "形态未知"   # 旧版/缺失·显式独立桶（不混算不误报）
            return "分片%d·%s·%s" % (片, 形态档, 机), ""
        return "单实例·" + 机, ""
    if 步骤名 in 需元信息步骤:
        口径 = str(步.get("口径") or "").strip()
        if not 口径:
            return "", "步骤未输出口径元信息（CN_STEP_META_DIR 元信息缺失/旧版本脚本）"
        return 口径 + "·" + 机, ""
    return "常规·" + 机, ""


def _错误对象(来源: str, 说明: str, 配置: dict | None = None) -> dict:
    """显式错误评估对象（不静默——前端/详情按错误黄红文案展示）。"""
    配置 = 配置 or 取配置()
    出 = {"版本": 2, "来源": 来源, "判据": 判据文本(配置), "错误": 说明,
          "告警": [], "慢漂": [], "待确认": [], "不足": [], "步骤": []}
    if 配置.get("警告"):
        出["配置警告"] = list(配置["警告"])
    return 出


def 评估(结果: dict, 取样本, *, 取上次超阈=None, 来源: str = "服务端汇总",
         配置: dict | None = None) -> dict:
    """评估一轮结果的全部可度量步骤（纯函数·取样本(步骤,口径)->[秒...] 注入）。

    返回对象（落库/前端共用）：
      {版本, 来源, 判据, 告警, 慢漂, 待确认, 不足, 步骤, [配置警告?/错误?/说明?]}。
      步骤=全部有 秒 字段的评估明细（含「未计入/待确认/不足」态）；告警=3× 超阈
      （去抖后）；慢漂=慢漂判据命中（黄签）；待确认=构建/配置首轮超阈（去抖）；
      不足=样本不足/口径缺失/执行机缺失（显式面）。"""
    配置 = 配置 or 取配置()
    if not isinstance(结果, dict):
        return _错误对象(来源, "结果非对象——无法评估耗时基线（显式·不静默）", 配置)
    步骤们 = 结果.get("步骤")
    if not isinstance(步骤们, dict):
        return _错误对象(来源, "结果缺步骤字段或形态非对象——无法评估耗时基线", 配置)
    告警: list[dict] = []
    慢漂: list[dict] = []
    待确认: list[dict] = []
    不足: list[dict] = []
    步骤表: list[dict] = []
    for 原名, 步 in 步骤们.items():
        名 = str(原名)
        if not isinstance(步, dict):
            不足.append({"步骤": 名, "口径": "", "样本数": 0,
                         "原因": "步骤记录非对象（结果形态坏）"})
            continue
        原秒 = 步.get("秒")
        if not isinstance(原秒, (int, float)) or isinstance(原秒, bool):
            continue   # 未计时步骤（同步/清产物/e2e 汇总键等）——不在耗时基线面
        秒 = float(原秒)
        if not math.isfinite(秒) or 秒 < 0:
            条 = {"步骤": 名, "口径": "—", "秒": None, "中位": None,
                  "倍数": None, "增量": None, "样本数": 0, "状态": "不足",
                  "样本超阈": 0, "原因": "步骤秒为负/NaN/Inf（结果形态坏·显式）"}
            不足.append({"步骤": 名, "口径": "—", "样本数": 0,
                         "原因": 条["原因"]})
            步骤表.append(条)
            continue
        if 秒 == 0:
            # P2-3：<0.5s 取整为 0=短步骤中性（未计入），不是形态坏、不入样本。
            步骤表.append({"步骤": 名, "口径": "—", "秒": 0, "中位": None,
                          "倍数": None, "增量": None, "样本数": 0,
                          "状态": "未计入", "样本超阈": 0,
                          "原因": "0 秒（<0.5s 取整·短步骤不参与基线）"})
            continue
        口径, 缺因 = 推导口径(名, 步, 结果)
        if 缺因:
            条 = {"步骤": 名, "口径": 口径 or "—", "秒": round(秒, 1),
                  "中位": None, "倍数": None, "增量": None, "样本数": 0,
                  "状态": "不足", "原因": 缺因, "样本超阈": 0}
            不足.append({"步骤": 名, "口径": 口径 or "—", "样本数": 0, "原因": 缺因})
            步骤表.append(条)
            continue
        可计 = (步.get("rc") == 0) and not 步.get("超时")
        if not 可计:
            步骤表.append({"步骤": 名, "口径": 口径, "秒": round(秒, 1),
                          "中位": None, "倍数": None, "增量": None, "样本数": 0,
                          "状态": "未计入", "样本超阈": 0,
                          "原因": "rc=%s%s——失败/超时步骤不计入基线、不告警"
                          % (步.get("rc"), "·超时" if 步.get("超时") else "")})
            continue
        样本 = list(取样本(名, 口径) or [])
        评 = 评估数值(样本, 秒, 配置=配置)
        条 = {"步骤": 名, "口径": 口径, "秒": round(秒, 1),
              "中位": 评["中位"], "倍数": 评["倍数"], "增量": 评["增量"],
              "样本数": 评["样本数"], "状态": 评["状态"],
              "样本超阈": 1 if 评["状态"] == "告警" else 0}
        if 评.get("原因"):
            条["原因"] = 评["原因"]
        if 评.get("漂移"):
            条["漂移"] = 评["漂移"]
        if 评["状态"] == "告警":
            上轮 = 0
            if 名 in 去抖步骤 and 取上次超阈 is not None:
                上轮 = int(取上次超阈(名, 口径) or 0)
            if 名 in 去抖步骤 and 上轮 != 1:
                条["状态"] = "待确认"
                条["原因"] = "首轮超阈（%s）——连续 2 轮超阈才告警（去抖）" \
                    % 条.get("原因", "超阈")
                待确认.append({"步骤": 名, "口径": 口径, "秒": 条["秒"],
                             "中位": 条["中位"], "倍数": 条["倍数"],
                             "样本数": 条["样本数"], "原因": 条["原因"]})
            else:
                告警.append(条)
        elif 评["状态"] == "慢漂":
            # P3：徽标显示真实漂移倍数（近期/长期中位比），非当前/中位（回归后≈1）
            条["漂移倍数"] = (评.get("漂移") or {}).get("倍数")
            条["漂移原因"] = (评.get("漂移") or {}).get("原因")
            慢漂.append(条)
        elif 评["状态"] == "不足":
            不足.append({"步骤": 名, "口径": 口径, "样本数": 评["样本数"],
                         "原因": 评.get("原因") or "基线不足"})
        步骤表.append(条)
    出 = {"版本": 2, "来源": 来源, "判据": 判据文本(配置),
          "告警": 告警, "慢漂": 慢漂, "待确认": 待确认, "不足": 不足, "步骤": 步骤表}
    if 配置.get("警告"):
        出["配置警告"] = list(配置["警告"])
    if not 步骤表:
        出["说明"] = "本轮无可度量步骤（免跑/早期退出/旧版结果）"
    return 出


# ═══════════ 服务端：耗时样本表 + 门禁列（queue_service 接线） ═══════════

def 建表(con: sqlite3.Connection) -> None:
    """建耗时样本表 + 门禁.耗时告警 列（幂等·旧库迁移）。
    门禁表须已存在（queue_service.建库 先建门禁再调本函数）——缺失=显式 RuntimeError。"""
    列们 = {r[1] for r in con.execute("PRAGMA table_info(门禁)").fetchall()}
    if not 列们:
        raise RuntimeError("门禁表不存在——耗时基线建表须在 queue_service.建库 之后")
    if "耗时告警" not in 列们:
        con.execute("ALTER TABLE 门禁 ADD COLUMN 耗时告警 TEXT")
    con.execute("""CREATE TABLE IF NOT EXISTS 耗时样本(
        平台 TEXT, 步骤 TEXT, 口径 TEXT, 秒 REAL, 时刻 TEXT, sha TEXT,
        超阈 INTEGER DEFAULT 0, epoch INTEGER)""")
    样本列 = {r[1] for r in con.execute("PRAGMA table_info(耗时样本)").fetchall()}
    if "超阈" not in 样本列:
        con.execute("ALTER TABLE 耗时样本 ADD COLUMN 超阈 INTEGER DEFAULT 0")
    if "epoch" not in 样本列:
        con.execute("ALTER TABLE 耗时样本 ADD COLUMN epoch INTEGER")
    con.execute("CREATE INDEX IF NOT EXISTS idx_耗时样本键 ON 耗时样本(平台,步骤,口径)")
    con.execute("CREATE INDEX IF NOT EXISTS idx_耗时样本龄 ON 耗时样本(epoch)")


def 读样本(con: sqlite3.Connection, 平台: str, 步骤: str, 口径: str,
          配置: dict | None = None) -> list:
    """读某键最近「保留」条样本（rowid 倒序取 N 再反转=时间升序）；空=[]（评估层落不足）。"""
    配置 = 配置 or 取配置()
    行们 = con.execute(
        "SELECT 秒 FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? "
        "ORDER BY rowid DESC LIMIT ?", (平台, 步骤, 口径, 配置["保留"])).fetchall()
    return [r[0] for r in reversed(行们)]


def _读上次超阈(con: sqlite3.Connection, 平台: str, 步骤: str, 口径: str,
              当前sha: str = "") -> int:
    """上一轮超阈标志（去抖链）；上一行同 sha=同轮重报→0（不把重试当第二轮）。"""
    行 = con.execute("SELECT sha, 超阈 FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? "
                    "ORDER BY rowid DESC LIMIT 1", (平台, 步骤, 口径)).fetchone()
    if not 行:
        return 0
    if 当前sha and str(行[0] or "") == 当前sha:
        return 0
    return int(行[1] or 0)


def _记样本(con: sqlite3.Connection, 平台: str, 步骤: str, 口径: str,
            秒: float, 时戳: str, sha: str, 超阈: int, epoch: float,
            配置: dict) -> None:
    # 幂等口径=同键同 sha 先删后插（重报/同 sha 重验不重复计数）；不同口径桶
    #   （如预验冷建 vs develop 温建）互不删除——同 sha 的多口径测量各自保留。
    con.execute("DELETE FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? AND sha=?",
                (平台, 步骤, 口径, sha))
    con.execute("INSERT INTO 耗时样本(平台,步骤,口径,秒,时刻,sha,超阈,epoch) "
                "VALUES(?,?,?,?,?,?,?,?)",
                (平台, 步骤, 口径, 秒, 时戳, sha, 1 if 超阈 else 0, epoch))
    con.execute(
        "DELETE FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? AND rowid NOT IN "
        "(SELECT rowid FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? "
        "ORDER BY rowid DESC LIMIT ?)",
        (平台, 步骤, 口径, 平台, 步骤, 口径, 配置["保留"]))


def 清理过期样本(con: sqlite3.Connection, 现在: float,
                配置: dict | None = None) -> int:
    """删 epoch 早于「保留天」的样本（键过期清理·默认 30 天；0=禁用）。"""
    配置 = 配置 or 取配置()
    if 配置["保留天"] <= 0:
        return 0
    cur = con.execute("DELETE FROM 耗时样本 WHERE epoch IS NOT NULL AND epoch < ?",
                      (现在 - 配置["保留天"] * 86400.0,))
    return int(cur.rowcount or 0)


def 记录并评估(con: sqlite3.Connection, 平台: str, sha: str, 结果: dict,
              时戳: str, *, 现在: float | None = None,
              配置: dict | None = None) -> dict:
    """门禁结果写入前调用：评估（读旧样本）→ 收本轮合格样本 → 裁剪/过期清理。

    幂等：同键同 sha 重报先删后插；不同口径桶各自保留。平台空串=拒绝建桶（显式
    错误对象·P2）。仅 rc=0 非超时且有限正数秒的步骤入样本；失败/超时/0 秒步骤只落
    「未计入」明细；缺因步骤（口径哨兵「—」）不入库（P2-6·防假基线桶）。
    构建/配置首轮超阈落「待确认」并以超阈=1 入样本（去抖）。"""
    配置 = 配置 or 取配置()
    平台 = str(平台 or "").strip()
    if not 平台:
        return _错误对象("服务端汇总", "平台为空——拒绝建立耗时基线桶（显式）", 配置)
    现在 = time.time() if 现在 is None else float(现在)
    清理过期样本(con, 现在, 配置)
    评 = 评估(结果,
              lambda 名, 口径: 读样本(con, 平台, 名, 口径, 配置),
             取上次超阈=lambda 名, 口径: _读上次超阈(con, 平台, 名, 口径, sha),
             配置=配置)
    步骤们 = 结果.get("步骤") if isinstance(结果.get("步骤"), dict) else {}
    for 条 in 评["步骤"]:
        if (条.get("状态") == "未计入" or not 条.get("口径")
                or 条.get("口径") == "—"):
            continue
        步 = 步骤们.get(条["步骤"])
        if not isinstance(步, dict) or 步.get("rc") != 0 or 步.get("超时"):
            continue
        秒 = 步.get("秒")
        if _有限正数(秒):
            _记样本(con, 平台, 条["步骤"], 条["口径"], float(秒), 时戳, sha,
                    条.get("样本超阈", 0), 现在, 配置)
    return 评


def _详情文本(数据: dict) -> str:
    """门禁.详情 列文本（严格 JSON·NaN/Inf 显式拒绝并落错误详情）。"""
    try:
        return json.dumps(数据, ensure_ascii=False, allow_nan=False)[:500000]
    except ValueError as e:
        print("[警告] 上报详情含 NaN/Inf——拒绝序列化（显式错误详情）：%r" % (e,),
              flush=True)
        步骤们 = 数据.get("步骤")
        return json.dumps(
            {"错误": "上报详情含 NaN/Inf，已拒绝序列化（%r）" % (e,),
             "步骤名": sorted(str(k) for k in 步骤们) if isinstance(步骤们, dict) else None},
            ensure_ascii=False, allow_nan=False)[:500000]


def 门禁写入字段(con: sqlite3.Connection, 平台: str, sha: str, 数据: dict,
                时戳: str, *, 是执行中: bool, 现在: float | None = None):
    """queue_service /api/report 专用（P2 行数治理：写库评估/播报/序列化外迁）。

    →(耗时告警列文本|None, 详情列文本, 播报行们)。执行中=列保留旧值（P2：
    不覆盖已完成行旧告警）且不评估；异常=显式错误对象，门禁结果照常入库。"""
    if 是执行中:
        旧行 = con.execute("SELECT 耗时告警 FROM 门禁 WHERE sha=? AND 平台=?",
                          (sha, 平台)).fetchone()
        return (旧行[0] if 旧行 else None), _详情文本(数据), []
    try:
        评 = 记录并评估(con, 平台, sha, 数据, 时戳, 现在=现在)
    except Exception as e:   # noqa: BLE001 —— 观测面异常须显式留痕（非静默兜底）
        print("[耗时告警] 评估异常（门禁结果照常入库）：%r" % (e,), flush=True)
        评 = _错误对象("服务端汇总", "耗时基线评估异常：%r（结果照常入库）" % (e,))
    行们 = []
    for 条 in 评.get("告警") or []:
        行们.append("[耗时告警] %s %s %s 秒=%s 中位=%s（%s×·样本 %s·口径 %s）"
                    % (sha[:10], 平台, 条.get("步骤"), 条.get("秒"),
                       条.get("中位"), 条.get("倍数"), 条.get("样本数"),
                       条.get("口径")))
    for 条 in 评.get("慢漂") or []:
        行们.append("[耗时慢漂] %s %s %s 秒=%s·%s"
                    % (sha[:10], 平台, 条.get("步骤"), 条.get("秒"),
                       条.get("原因") or "慢漂"))
    try:
        耗时列 = json.dumps(评, ensure_ascii=False, allow_nan=False)
    except ValueError as e:
        print("[耗时告警] 评估落库序列化失败（NaN/Inf·显式错误对象）：%r"
              % (e,), flush=True)
        耗时列 = json.dumps(
            {"版本": 2, "错误": "耗时评估序列化失败：%r" % (e,),
             "告警": [], "慢漂": [], "待确认": [], "不足": [], "步骤": []},
            ensure_ascii=False, allow_nan=False)
    return 耗时列, _详情文本(数据), 行们


def 解析列(文本) -> dict | None:
    """门禁.耗时告警 列 → 对象。NULL=未评估（旧行/执行中）→None；坏 JSON/形态坏=
    显式错误对象（列保留待人工·不静默当正常）。"""
    if not 文本:
        return None
    try:
        值 = json.loads(文本)
    except (ValueError, TypeError):
        return _错误对象("服务端汇总", "门禁.耗时告警 列 JSON 损坏（列保留待人工）")
    if not isinstance(值, dict):
        return _错误对象("服务端汇总", "门禁.耗时告警 列形态非对象（列保留待人工）")
    return 值


def _列列表(文本, 键: str) -> list | None:
    值 = 解析列(文本)
    if 值 is None:
        return None
    if 值.get("错误"):
        return [{"步骤": "耗时告警数据异常", "原因": str(值["错误"]),
                 "秒": None, "中位": None, "倍数": None, "增量": None,
                 "样本数": None}]
    if 键 not in 值:
        # P2-2：v1 旧对象无 慢漂/待确认 键=无信号（空数组），不挂假异常徽标；
        #   仅「键存在但类型错」才是列损坏。
        return []
    列表 = 值.get(键)
    if not isinstance(列表, list):
        return [{"步骤": "耗时告警数据异常", "原因": "%s 字段形态非数组" % 键,
                 "秒": None, "中位": None, "倍数": None, "增量": None,
                 "样本数": None}]
    return 列表


def 列告警(文本) -> list | None:
    """门禁快照面：→告警列表（徽标数据源）；NULL=未评估→None；异常=显式一条。"""
    return _列列表(文本, "告警")


def 列慢漂(文本) -> list | None:
    """门禁快照面：→慢漂列表（黄签数据源）；NULL=未评估→None；异常=显式一条。"""
    return _列列表(文本, "慢漂")


# ═══════════ 步骤口径元信息协议（脚本 → daemon 步骤记录器） ═══════════
# 耗时不只由代码决定、也与脚本内部扫描面/缓存态强相关的步骤（check_asm_width），
# 由脚本把口径写进 CN_STEP_META_DIR/<步骤名>.json；daemon 步骤记录器跑完读取并
# 并入步骤记录（口径入基线桶）。环境变量缺省=无消费者的本地手跑，不写。

def 准备步骤元信息目录(日志目录) -> None:
    """daemon 启动时准备回传目录并注入 CN_STEP_META_DIR（子进程继承）。"""
    目录 = Path(日志目录) / "步骤元信息"
    try:
        目录.mkdir(parents=True, exist_ok=True)
    except OSError as e:
        print("[黄] 步骤元信息目录创建失败（口径降级为显式不足）：%r" % (e,),
              file=sys.stderr, flush=True)
        return
    os.environ.setdefault("CN_STEP_META_DIR", str(目录))


def 步骤元信息目录():
    """→Path 或 None（未注入=不在 daemon 步骤上下文）。"""
    原 = os.environ.get("CN_STEP_META_DIR", "").strip()
    return Path(原) if 原 else None


def 写步骤元信息(步骤名: str, 元: dict) -> None:
    """脚本侧写口径元信息（原子替换）；未注入目录=本地手跑无消费者，不写。"""
    目录 = 步骤元信息目录()
    if 目录 is None:
        return
    try:
        目录.mkdir(parents=True, exist_ok=True)
        目标 = 目录 / (步骤名 + ".json")
        临时 = 目标.with_name(目标.name + ".%d.tmp" % os.getpid())
        临时.write_text(json.dumps(元, ensure_ascii=False, allow_nan=False),
                        encoding="utf-8")
        os.replace(临时, 目标)
    except OSError as e:
        print("[黄] 步骤元信息写入失败（%s）：%r" % (步骤名, e),
              file=sys.stderr, flush=True)


def 读步骤元信息(步骤名: str, 不早于: float) -> dict:
    """daemon 步骤记录器读取元信息并入步骤记录（口径键）；缺失/陈旧={}。

    陈旧判定=文件 mtime 早于步骤起点（上一轮残留·脚本本轮未回传）→删除并返回 {}；
    asm_width 等步骤的元信息缺失由评估层落「口径不足」显式条目（不猜测口径）。"""
    目录 = 步骤元信息目录()
    if 目录 is None:
        return {}
    目标 = 目录 / (步骤名 + ".json")
    try:
        修改时刻 = 目标.stat().st_mtime
    except OSError:
        return {}
    if 修改时刻 < 不早于 - 1.0:   # 1s 宽限=文件系统时间粒度
        try:
            目标.unlink()
        except OSError:
            pass
        return {}
    try:
        值 = json.loads(目标.read_text(encoding="utf-8"))
    except (OSError, ValueError) as e:
        print("[黄] 步骤元信息损坏（%s）：%r——按口径缺失处理" % (步骤名, e),
              file=sys.stderr, flush=True)
        值 = None
    try:
        目标.unlink()
    except OSError:
        pass
    if not isinstance(值, dict):
        return {}
    口径 = str(值.get("口径") or "").strip()
    return {"口径": 口径} if 口径 else {}


# ═══════════ runner 本地降级面（ci_report.落盘 前调用） ═══════════

def _坏档改名(路径: Path, 说明: str) -> str:
    """坏基线文件改名留档（.坏<时间戳>·P2）；改名失败=返回含失败原因的显式说明。"""
    备 = 路径.with_name(路径.name + ".坏" + time.strftime("%Y%m%d_%H%M%S"))
    try:
        os.replace(路径, 备)
        return 说明 + "（已改名留档 %s）" % 备.name
    except OSError as e:
        return 说明 + "（改名留档失败 %r·原文件保留）" % (e,)


def _读本地样本表(路径: Path) -> tuple[dict, str]:
    """→({步骤\\x1f口径: [{"秒","sha","超阈","时戳"}...]}, 损坏说明)。缺文件=({}, "")。"""
    损坏 = ""
    if not 路径.exists():
        return {}, ""
    旧 = atomic_json.读(路径)
    if 旧 is None:
        return ({}, _坏档改名(路径, "本机耗时基线文件损坏——已重建（本轮样本从零累积）"))
    if not isinstance(旧, dict):
        return ({}, _坏档改名(路径, "本机耗时基线文件形态非对象——已重建"))
    配置 = 取配置()
    截止 = time.time() - 配置["保留天"] * 86400.0
    样本表: dict = {}
    for 条目 in 旧.get("样本") or []:
        if not isinstance(条目, dict):
            continue
        步, 口, 秒 = 条目.get("步骤"), 条目.get("口径"), 条目.get("秒")
        if not (isinstance(步, str) and 步 and isinstance(口, str) and 口):
            continue
        if not _有限正数(秒):
            continue
        时戳 = 条目.get("时戳")
        if (配置["保留天"] > 0 and isinstance(时戳, (int, float))
                and not isinstance(时戳, bool) and 时戳 < 截止):
            continue   # 键过期清理（本机面·P2）
        样本表.setdefault(步 + _本机键分隔 + 口, []).append(
            {"秒": float(秒), "sha": str(条目.get("sha") or ""),
             "超阈": 1 if 条目.get("超阈") else 0,
             "时戳": float(时戳) if isinstance(时戳, (int, float)) else 0.0})
    return 样本表, 损坏


def _本地补充(结果: dict, 日志目录) -> dict:
    配置 = 取配置()
    路径 = Path(日志目录) / 本地基文件名
    样本表, 损坏 = _读本地样本表(路径)
    来源 = "runner本机（断网降级）"
    平台 = str(结果.get("平台") or "").strip()
    if not 平台:
        结果["耗时告警"] = _错误对象(来源, "结果缺平台字段——无法评估耗时基线", 配置)
        return 结果

    def 取样本(名: str, 口径: str) -> list:
        return [x["秒"] for x in 样本表.get(名 + _本机键分隔 + 口径, [])]

    def 取上次超阈(名: str, 口径: str) -> int:
        值们 = 样本表.get(名 + _本机键分隔 + 口径, [])
        if not 值们:
            return 0
        上 = 值们[-1]
        # 同轮重报（上一行同 sha）=不把重试当第二轮（去抖链只看真正的新轮）
        if 上.get("sha") and 上.get("sha") == str(结果.get("sha") or ""):
            return 0
        return int(上.get("超阈", 0))

    评 = 评估(结果, 取样本, 取上次超阈=取上次超阈, 来源=来源, 配置=配置)
    if 损坏:
        评["错误"] = 损坏
    sha = str(结果.get("sha") or "")
    步骤们 = 结果.get("步骤") if isinstance(结果.get("步骤"), dict) else {}
    for 条 in 评["步骤"]:
        if (条.get("状态") == "未计入" or not 条.get("口径")
                or 条.get("口径") == "—"):
            continue
        步 = 步骤们.get(条["步骤"])
        if not isinstance(步, dict) or 步.get("rc") != 0 or 步.get("超时"):
            continue
        秒 = 步.get("秒")
        if not _有限正数(秒):
            continue
        键 = 条["步骤"] + _本机键分隔 + 条["口径"]
        旧表 = [x for x in 样本表.get(键, []) if not sha or x.get("sha") != sha]
        旧表.append({"秒": float(秒), "sha": sha,
                     "超阈": int(条.get("样本超阈", 0)), "时戳": time.time()})
        样本表[键] = 旧表[-配置["保留"]:]
    条目们 = []
    for 键, 值们 in 样本表.items():
        步, 口 = 键.split(_本机键分隔, 1)
        for 值 in 值们:
            条目们.append({"步骤": 步, "口径": 口, "秒": 值["秒"],
                          "sha": 值["sha"], "超阈": 值.get("超阈", 0),
                          "时戳": 值.get("时戳", 0.0)})
    写成功 = atomic_json.写(路径, {"版本": 2,
                                  "更新时刻": time.strftime("%Y-%m-%d %H:%M:%S"),
                                  "样本": 条目们})
    if not 写成功:
        评["错误"] = "本机耗时基线文件写入失败（下轮基线可能回退·门禁结果不受影响）"
    结果["耗时告警"] = 评
    return 结果


def 本地补充(结果, 日志目录) -> dict:
    """runner 本地基线降级面（ci_report.落盘 调用）：评估+累积本机样本并写回结果。

    任何异常都收敛为结果里的显式「错误」对象——观测面不得阻断门禁结果落盘。"""
    if not isinstance(结果, dict):
        return 结果
    try:
        return _本地补充(结果, 日志目录)
    except Exception as e:   # noqa: BLE001 —— 观测面异常须显式留痕（非静默兜底）
        print("[警告] 本机耗时基线评估异常（落盘照常）：%r" % (e,), flush=True)
        结果["耗时告警"] = _错误对象("runner本机（断网降级）",
                                    "本机耗时基线评估异常：%r（落盘照常）" % (e,),
                                    取配置())
        return 结果



if __name__ == "__main__":
    # 自测体外迁 ci_duration_selftest（先拆再加·主模块守 1000 行红线）；入口不变。
    if len(sys.argv) > 1 and sys.argv[1] != "--selftest":
        print("用法：python scripts/ci_duration.py [--selftest]", flush=True)
        sys.exit(2)
    import ci_duration_selftest
    sys.exit(ci_duration_selftest.跑())
