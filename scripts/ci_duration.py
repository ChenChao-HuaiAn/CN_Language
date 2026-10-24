#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ci_duration.py —— 门禁步骤耗时滚动基线（任务 #1385·「静默劣化」机械告警面）。

背景（#1299 查因结论）：预验构建段 63s→372s 是靠人眼翻门禁表发现的，不是被告警抓到的；
  同 sha 同平台在不同执行机/构建并行度下耗时可差数倍（1299 增设 runner/CN_BUILD_JOBS
  归因字段正是为此），因此基线**必须按执行机与口径分桶**，否则换机即误报。

口径（单一事实源）：
  · 样本=步骤记录里的 `秒` 字段（#1385 前的既有数据形状，runner 零协议新增）；
  · 键=(平台, 步骤, 口径, 执行机)——口径按步骤语义分桶：
      构建/配置 → 冷建/温建（含清产物 或 win 指纹清建标记「冷建」）；
      e2e/e2e并行 → 单实例/分片N（分片数变化=耗时口径变化·不混算）；
      check_asm_width → 脚本自报的扫描口径（全扫/目录扫N + 缓存冷热·1300 增量模式）；
      其余 → 常规；
  · 执行机维度=结果.runner（1299 归因字段）——同平台不同机不混算；
  · 滚动窗口=近 20 次（CN_DURATION_SAMPLES）中位数；样本仅收 rc=0 且非超时的步骤
    （失败步骤耗时由失败原因解释，不污染正常基线）；
  · 告警=当前 > 3×中位 **且** 增量 > 10s（CN_DURATION_ALERT_RATIO/DELTA）；样本
    < 5（CN_DURATION_MIN_SAMPLES）=显式「基线不足」，不静默放行也不误报；
  · 中位/样本不可用一律落「不足」条目（648 miss 不可静默），前端按黄字展示。

数据面：
  · 服务端（权威）：门禁表 `耗时告警` 列（JSON 对象）+ `耗时样本` 表（每键滚动样本）；
    /api/report 写入门禁结果时计算并落库（queue_service 调用 `记录并评估安全`），
    门禁快照/详情把告警徽标与基线详情带给 8300 门禁窗口与 8301 看板；
  · runner 本地（断网降级）：ci_report.落盘 前调 `本地补充`——本树 ci-logs/耗时基线.json
    原子累积样本，结果 JSON 带 `耗时告警`（来源=runner本机），上报不可达时本地仍可见。
    服务端可达时以服务端对象为准（跨 runner 汇总），本地对象仅作降级面。

边界：Python 脚本与 C++/v2 编译器实现分离（本模块只服务 CI 观测面，不参与编译产物）；
  check_asm_width 的口径元信息经 CN_STEP_META_DIR 环境目录回传（daemon 步骤记录器读取），
  元信息缺失=口径不足显式条目，不猜测。
"""
from __future__ import annotations

import json
import os
import sqlite3
import sys
import time
from pathlib import Path

import atomic_json   # 556：本机基线文件原子读写（半写损坏防护·读坏黄字）

# ===== 可调常量（环境变量覆盖；非法值=启动期显式报错，拒绝静默取默认） =====
def _环境数(名: str, 默认, 转换):
    """读数值型环境变量；未设=文档默认；非法=ValueError（部署配置错须显式暴露）。"""
    原 = os.environ.get(名)
    if 原 is None or str(原).strip() == "":
        return 默认
    try:
        return 转换(原)
    except (TypeError, ValueError):
        raise ValueError("环境变量 %s=%r 非法（期望数值）——拒绝静默取默认值" % (名, 原))


样本上限 = max(1, _环境数("CN_DURATION_SAMPLES", 20, int))
最小样本 = max(1, _环境数("CN_DURATION_MIN_SAMPLES", 5, int))
告警倍数 = max(1.01, _环境数("CN_DURATION_ALERT_RATIO", 3.0, float))
告警增量秒 = max(0.0, _环境数("CN_DURATION_ALERT_DELTA", 10.0, float))

判据文本 = ("近 %d 次同平台同步骤同口径同执行机中位；超 %.1f×中位且增量>%ss 判「耗时异常」；"
            "样本≥%d 才评估（不足=显式基线不足）；仅计 rc=0 非超时步骤"
            % (样本上限, 告警倍数, ("%g" % 告警增量秒), 最小样本))

# 需脚本自报口径的步骤（耗时不只由代码决定·与写集/缓存态强相关）
需元信息步骤 = frozenset(("check_asm_width",))
冷建步骤 = frozenset(("配置", "构建"))
分片步骤 = frozenset(("e2e", "e2e并行"))

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


def 评估数值(样本们: list, 当前秒: float, *, 最小: int | None = None,
             倍数: float | None = None, 增量: float | None = None) -> dict:
    """单步骤评估（纯函数·selftest 四态锚）：返回
    {状态: 正常|告警|不足, 中位, 倍数, 增量, 样本数, 原因?}。
    判据：当前 > 中位×倍数 **且** 当前-中位 > 增量；样本 < 最小=不足（不评估）。"""
    最小 = 最小样本 if 最小 is None else 最小
    倍数 = 告警倍数 if 倍数 is None else 倍数
    增量 = 告警增量秒 if 增量 is None else 增量
    有效 = [x for x in 样本们
            if isinstance(x, (int, float)) and not isinstance(x, bool)]
    n = len(有效)
    if n < 最小:
        return {"状态": "不足", "中位": None, "倍数": None, "增量": None,
                "样本数": n, "原因": "样本 %d < 最小 %d（基线不足·显式不静默）"
                % (n, 最小)}
    中位 = 中位数(有效)
    if 中位 is None or 中位 <= 0:
        return {"状态": "不足", "中位": 中位, "倍数": None, "增量": None,
                "样本数": n, "原因": "中位不可用（样本形态异常）"}
    倍 = float(当前秒) / 中位
    增 = float(当前秒) - 中位
    if 倍 > 倍数 and 增 > 增量:
        return {"状态": "告警", "中位": round(中位, 1), "倍数": round(倍, 1),
                "增量": round(增, 1), "样本数": n,
                "原因": "当前 %.1fs > %.1f×中位 %.1fs 且增量 %.1fs > %ss"
                % (当前秒, 倍数, 中位, 增, ("%g" % 增量))}
    return {"状态": "正常", "中位": round(中位, 1), "倍数": round(倍, 1),
            "增量": round(增, 1), "样本数": n}


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


def 推导口径(步骤名: str, 步: dict, 结果: dict) -> tuple[str, str]:
    """→(口径串, 不足原因)。不足原因非空=该步骤无法分桶（如执行机/脚本口径缺失）。"""
    runner = str(结果.get("runner") or "").strip()
    if not runner:
        return "", "执行机未归因（旧版 runner 无 runner 字段）——不混算不告警"
    机 = "runner=" + runner
    if 步骤名 in 冷建步骤:
        冷 = bool(步.get("冷建")) or _有清产物(结果.get("步骤") or {})
        jobs = 结果.get("CN_BUILD_JOBS")
        尾 = "·%d并行" % jobs if isinstance(jobs, int) and not isinstance(jobs, bool) else ""
        return ("冷建" if 冷 else "温建") + "·" + 机 + 尾, ""
    if 步骤名 in 分片步骤:
        片 = 取分片数(结果)
        return ("分片%d" % 片 if 片 > 1 else "单实例") + "·" + 机, ""
    if 步骤名 in 需元信息步骤:
        口径 = str(步.get("口径") or "").strip()
        if not 口径:
            return "", "步骤未输出口径元信息（CN_STEP_META_DIR 元信息缺失/旧版本脚本）"
        return 口径 + "·" + 机, ""
    return "常规·" + 机, ""


def _错误对象(来源: str, 说明: str) -> dict:
    """显式错误评估对象（不静默——前端/详情按错误黄红文案展示）。"""
    return {"版本": 1, "来源": 来源, "判据": 判据文本, "错误": 说明,
            "告警": [], "不足": [], "步骤": []}


def 评估(结果: dict, 取样本, *, 来源: str = "服务端汇总") -> dict:
    """评估一轮结果的全部可度量步骤（纯函数·取样本(步骤,口径)->[秒...] 注入）。

    返回对象（落库/前端共用）：
      {版本, 来源, 判据, 告警:[...], 不足:[...], 步骤:[...]}；步骤=全部有 秒 字段的
      步骤评估明细（含「未计入」「不足」态），告警=子集（徽标数据源），不足=子集
      （基线不足/口径缺失/执行机缺失·显式面）。"""
    if not isinstance(结果, dict):
        return _错误对象(来源, "结果非对象——无法评估耗时基线（显式·不静默）")
    步骤们 = 结果.get("步骤")
    if not isinstance(步骤们, dict):
        return _错误对象(来源, "结果缺步骤字段或形态非对象——无法评估耗时基线")
    告警: list[dict] = []
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
        口径, 缺因 = 推导口径(名, 步, 结果)
        if 缺因:
            条 = {"步骤": 名, "口径": 口径 or "—", "秒": round(秒, 1),
                  "中位": None, "倍数": None, "增量": None, "样本数": 0,
                  "状态": "不足", "原因": 缺因}
            不足.append({"步骤": 名, "口径": 口径 or "—", "样本数": 0, "原因": 缺因})
            步骤表.append(条)
            continue
        可计 = (步.get("rc") == 0) and not 步.get("超时")
        if not 可计:
            步骤表.append({"步骤": 名, "口径": 口径, "秒": round(秒, 1),
                          "中位": None, "倍数": None, "增量": None, "样本数": 0,
                          "状态": "未计入",
                          "原因": "rc=%s%s——失败/超时步骤不计入基线、不告警"
                          % (步.get("rc"), "·超时" if 步.get("超时") else "")})
            continue
        样本 = list(取样本(名, 口径) or [])
        评 = 评估数值(样本, 秒)
        条 = {"步骤": 名, "口径": 口径, "秒": round(秒, 1),
              "中位": 评["中位"], "倍数": 评["倍数"], "增量": 评["增量"],
              "样本数": 评["样本数"], "状态": 评["状态"]}
        if 评.get("原因"):
            条["原因"] = 评["原因"]
        if 评["状态"] == "告警":
            告警.append(条)
        elif 评["状态"] == "不足":
            不足.append({"步骤": 名, "口径": 口径, "样本数": 评["样本数"],
                         "原因": 评.get("原因") or "基线不足"})
        步骤表.append(条)
    出 = {"版本": 1, "来源": 来源, "判据": 判据文本,
          "告警": 告警, "不足": 不足, "步骤": 步骤表}
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
        平台 TEXT, 步骤 TEXT, 口径 TEXT, 秒 REAL, 时刻 TEXT, sha TEXT)""")
    con.execute("CREATE INDEX IF NOT EXISTS idx_耗时样本键 ON 耗时样本(平台,步骤,口径)")


def 读样本(con: sqlite3.Connection, 平台: str, 步骤: str, 口径: str) -> list:
    """读某键最近 N 个样本（rowid 倒序取 N 再反转=时间升序）；空=[]（评估层落不足）。"""
    行们 = con.execute(
        "SELECT 秒 FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? "
        "ORDER BY rowid DESC LIMIT ?", (平台, 步骤, 口径, 样本上限)).fetchall()
    return [r[0] for r in reversed(行们)]


def _记样本(con: sqlite3.Connection, 平台: str, 步骤: str, 口径: str,
            秒: float, 时戳: str, sha: str) -> None:
    # 幂等口径=同键同 sha 先删后插（重报/同 sha 重验不重复计数）；不同口径桶
    #   （如预验冷建 vs develop 温建）互不删除——同 sha 的多口径测量各自保留。
    con.execute("DELETE FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? AND sha=?",
                (平台, 步骤, 口径, sha))
    con.execute("INSERT INTO 耗时样本(平台,步骤,口径,秒,时刻,sha) VALUES(?,?,?,?,?,?)",
                (平台, 步骤, 口径, 秒, 时戳, sha))
    con.execute(
        "DELETE FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? AND rowid NOT IN "
        "(SELECT rowid FROM 耗时样本 WHERE 平台=? AND 步骤=? AND 口径=? "
        "ORDER BY rowid DESC LIMIT ?)",
        (平台, 步骤, 口径, 平台, 步骤, 口径, 样本上限))


def 记录并评估(con: sqlite3.Connection, 平台: str, sha: str, 结果: dict,
              时戳: str) -> dict:
    """门禁结果写入前调用：评估（读旧样本）→ 收本轮合格样本 → 裁剪至 N 条。

    幂等：同键同 sha 重报先删后插（上报重试/同 sha 重验不重复计数）；不同口径桶
    各自保留（预验冷建样本不因随后 develop 温建同 sha 上报而被误删）。
    仅 rc=0 非超时且 秒>0 的步骤入样本；失败/超时步骤只落「未计入」明细。"""
    评 = 评估(结果, lambda 名, 口径: 读样本(con, 平台, 名, 口径))
    步骤们 = 结果.get("步骤") if isinstance(结果.get("步骤"), dict) else {}
    for 条 in 评["步骤"]:
        if 条.get("状态") == "未计入" or not 条.get("口径"):
            continue
        步 = 步骤们.get(条["步骤"])
        if not isinstance(步, dict) or 步.get("rc") != 0 or 步.get("超时"):
            continue
        秒 = 步.get("秒")
        if isinstance(秒, (int, float)) and not isinstance(秒, bool) and 秒 > 0:
            _记样本(con, 平台, 条["步骤"], 条["口径"], float(秒), 时戳, sha)
    return 评


def 记录并评估安全(con: sqlite3.Connection, 平台: str, sha: str, 结果: dict,
                  时戳: str) -> dict:
    """记录并评估的 fail-open 壳：观测面异常不得拖垮门禁结果入库——
    异常变成显式「错误」对象（前端黄红文案），并打印告警，不静默吞。"""
    try:
        return 记录并评估(con, 平台, sha, 结果, 时戳)
    except Exception as e:   # noqa: BLE001 —— 观测面异常须显式留痕（非静默兜底）
        print("[耗时告警] 评估异常（门禁结果照常入库）：%r" % (e,), flush=True)
        return _错误对象("服务端汇总", "耗时基线评估异常：%r（结果照常入库）" % (e,))


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


def 列告警(文本) -> list | None:
    """门禁快照面：→告警列表（徽标数据源）；NULL=未评估→None；异常=显式一条。"""
    值 = 解析列(文本)
    if 值 is None:
        return None
    if 值.get("错误"):
        return [{"步骤": "耗时告警数据异常", "原因": str(值["错误"]),
                 "秒": None, "中位": None, "倍数": None, "增量": None, "样本数": None}]
    告警 = 值.get("告警")
    if not isinstance(告警, list):
        return [{"步骤": "耗时告警数据异常", "原因": "告警字段形态非数组",
                 "秒": None, "中位": None, "倍数": None, "增量": None, "样本数": None}]
    return 告警


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
        临时.write_text(json.dumps(元, ensure_ascii=False), encoding="utf-8")
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

def _读本地样本表(路径: Path) -> tuple[dict, str]:
    """→({步骤\\x1f口径: [{"秒","sha"}...]}, 损坏说明)。缺文件=({}, "")。"""
    旧 = atomic_json.读(路径)
    if 旧 is None:
        损坏 = "本机耗时基线文件损坏（已重建·本轮样本从零累积·旧文件保留待人工）" \
            if 路径.exists() else ""
        return {}, 损坏
    if not isinstance(旧, dict):
        return {}, "本机耗时基线文件形态非对象（已重建）"
    样本表: dict = {}
    for 条目 in 旧.get("样本") or []:
        if not isinstance(条目, dict):
            continue
        步, 口, 秒 = 条目.get("步骤"), 条目.get("口径"), 条目.get("秒")
        if not (isinstance(步, str) and 步 and isinstance(口, str) and 口):
            continue
        if not (isinstance(秒, (int, float)) and not isinstance(秒, bool) and 秒 > 0):
            continue
        样本表.setdefault(步 + _本机键分隔 + 口, []).append(
            {"秒": float(秒), "sha": str(条目.get("sha") or "")})
    return 样本表, ""


def _本地补充(结果: dict, 日志目录) -> dict:
    路径 = Path(日志目录) / 本地基文件名
    样本表, 损坏 = _读本地样本表(路径)
    来源 = "runner本机（断网降级）"
    平台 = str(结果.get("平台") or "").strip()
    if not 平台:
        结果["耗时告警"] = _错误对象(来源, "结果缺平台字段——无法评估耗时基线")
        return 结果

    def 取样本(名: str, 口径: str) -> list:
        return [x["秒"] for x in 样本表.get(名 + _本机键分隔 + 口径, [])]

    评 = 评估(结果, 取样本, 来源=来源)
    if 损坏:
        评["错误"] = 损坏
    sha = str(结果.get("sha") or "")
    步骤们 = 结果.get("步骤") if isinstance(结果.get("步骤"), dict) else {}
    for 条 in 评["步骤"]:
        if 条.get("状态") == "未计入" or not 条.get("口径"):
            continue
        步 = 步骤们.get(条["步骤"])
        if not isinstance(步, dict) or 步.get("rc") != 0 or 步.get("超时"):
            continue
        秒 = 步.get("秒")
        if not (isinstance(秒, (int, float)) and not isinstance(秒, bool) and 秒 > 0):
            continue
        键 = 条["步骤"] + _本机键分隔 + 条["口径"]
        旧表 = [x for x in 样本表.get(键, []) if not sha or x.get("sha") != sha]
        旧表.append({"秒": float(秒), "sha": sha})
        样本表[键] = 旧表[-样本上限:]
    条目们 = []
    for 键, 值们 in 样本表.items():
        步, 口 = 键.split(_本机键分隔, 1)
        for 值 in 值们:
            条目们.append({"步骤": 步, "口径": 口, "秒": 值["秒"], "sha": 值["sha"]})
    写成功 = atomic_json.写(路径, {"版本": 1,
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
                                    "本机耗时基线评估异常：%r（落盘照常）" % (e,))
        return 结果


# ═══════════ 自测（四态：正常/3×超时/样本不足/冷启动） ═══════════

def _自测结果(秒们: dict, *, sha: str = "a" * 40, 清产物: bool = True,
              分片数: int = 0, runner: str = "TX_02", 绿: bool = True) -> dict:
    """构造自测用门禁结果（步骤=秒们·清产物可控·分片数/执行机可控）。"""
    步骤们: dict = {"同步": {"rc": 0}}
    if 清产物:
        步骤们["清产物"] = {"rc": 0}
    for 名, 秒 in 秒们.items():
        步骤们[名] = {"rc": 0, "秒": 秒}
    return {"sha": sha, "平台": "linux-x86_64", "runner": runner,
            "CN_BUILD_JOBS": 2, "步骤": 步骤们, "绿": 绿, "分片数": 分片数}


def 自测() -> int:
    """四态+口径分桶+幂等+本地降级 自测（零网络·内存库/临时目录）。"""
    import tempfile
    断言数 = [0]

    def 断言(条件, 说明):
        断言数[0] += 1
        if not 条件:
            print("✗ 失败: " + 说明)
            raise SystemExit(1)
        print("  ✓ " + 说明)

    # ── 纯函数四态（评估数值）──
    正常样本 = [60, 61, 59, 60, 62, 60, 61, 60, 59, 60]
    断言(评估数值(正常样本, 62)["状态"] == "正常", "① 正常：62s 对中位 60s=正常（<3×）")
    超时评 = 评估数值(正常样本, 200)
    断言(超时评["状态"] == "告警" and 超时评["倍数"] == 3.3,
         "② 3×超时：200s 对中位 60s=告警（3.3×·增量 140s）")
    断言(评估数值([60, 60, 60], 200)["状态"] == "不足"
         and 评估数值([60, 60, 60], 200)["样本数"] == 3,
         "③ 样本不足：3 样本 < 5=显式不足（不告警不静默）")
    断言(评估数值([], 200)["状态"] == "不足", "④ 冷启动：零样本=显式不足")
    断言(评估数值([3, 3, 3, 3, 3], 9)["状态"] == "正常",
         "阈值：恰 3× 且增量 6s≤10s=不误报（增量门槛生效）")
    断言(评估数值([4, 4, 4, 4, 4], 20)["状态"] == "告警",
         "阈值：5×且增量 16s>10s=告警")

    # ── 口径分桶（冷/温、分片数、执行机、asm_width 元信息）──
    冷结果 = _自测结果({"配置": 5, "构建": 300})
    暖结果 = _自测结果({"配置": 2, "构建": 60}, 清产物=False)
    构建步 = {"rc": 0, "秒": 300}
    冷口 = 推导口径("构建", 构建步, 冷结果)[0]
    暖口 = 推导口径("构建", 构建步, 暖结果)[0]
    断言(冷口.startswith("冷建") and 暖口.startswith("温建") and 冷口 != 暖口,
         "口径：清产物在场=冷建·不在场=温建（两桶不混算）")
    断言("runner=TX_02" in 冷口 and "2并行" in 冷口,
         "口径：执行机 runner+构建并行度入桶（1299 归因）")
    片3 = 推导口径("e2e并行", {"rc": 0, "秒": 10}, _自测结果({}, 分片数=3))[0]
    片1 = 推导口径("e2e并行", {"rc": 0, "秒": 10}, _自测结果({}))[0]
    断言(片3.startswith("分片3") and 片1.startswith("单实例") and 片3 != 片1,
         "口径：分片数变化=独立桶（分片 3 vs 单实例）")
    缺因 = 推导口径("check_asm_width", {"rc": 0, "秒": 5}, _自测结果({}))[1]
    断言("元信息" in 缺因, "口径：asm_width 无脚本自报口径=显式不足（不猜测）")
    meta口 = 推导口径("check_asm_width", {"rc": 0, "秒": 5, "口径": "全扫·冷缓存"},
                     _自测结果({}))[0]
    断言(meta口.startswith("全扫·冷缓存"), "口径：asm_width 自报口径入桶")
    无名runner = _自测结果({})
    无名runner["runner"] = ""
    断言("执行机未归因" in 推导口径("构建", {"rc": 0, "秒": 1}, 无名runner)[1],
         "口径：执行机缺失=显式不足（旧版 runner 不混算）")

    # ── 步骤元信息协议（脚本→daemon 口径回传·陈旧/损坏显式）──
    with tempfile.TemporaryDirectory() as 元目录:
        原元目录 = os.environ.get("CN_STEP_META_DIR")
        os.environ["CN_STEP_META_DIR"] = 元目录
        try:
            写步骤元信息("check_asm_width", {"口径": "全扫·冷缓存"})
            断言(读步骤元信息("check_asm_width", time.time() - 5).get("口径") == "全扫·冷缓存",
                 "元信息协议：写读往返（口径回传入步骤记录）")
            断言(读步骤元信息("check_asm_width", time.time() - 5) == {},
                 "元信息协议：读取后删除（不跨轮残留）")
            写步骤元信息("check_asm_width", {"口径": "旧轮口径"})
            os.utime(Path(元目录) / "check_asm_width.json", (0, 0))
            断言(读步骤元信息("check_asm_width", time.time()) == {},
                 "元信息协议：mtime 早于步骤起点=陈旧→忽略")
        finally:
            if 原元目录 is None:
                os.environ.pop("CN_STEP_META_DIR", None)
            else:
                os.environ["CN_STEP_META_DIR"] = 原元目录

    # ── 服务端表：滚动/告警/幂等/裁剪（内存库）──
    con = sqlite3.connect(":memory:")
    con.execute("CREATE TABLE 门禁(sha TEXT, 平台 TEXT, 绿 INTEGER, 总秒 INTEGER, "
                "时刻 TEXT, 详情 TEXT, PRIMARY KEY(sha, 平台))")
    建表(con)
    建表(con)   # 幂等
    for i in range(1, 7):
        评 = 记录并评估(con, "linux-x86_64", "%040d" % i,
                        _自测结果({"配置": 5, "构建": 60}, sha="%040d" % i),
                        "10-23 10:00:%02d" % i)
        if i <= 5:
            断言(any(x["步骤"] == "构建" for x in 评["不足"]) and not 评["告警"],
                 "服务端：第 %d 轮构建样本<%d=基线不足且零告警" % (i, 最小样本))
        else:
            断言(not 评["告警"], "服务端：第 %d 轮构建 60s=正常（样本已足）" % i)
    突评 = 记录并评估(con, "linux-x86_64", "f" * 40,
                      _自测结果({"配置": 5, "构建": 200}, sha="f" * 40),
                      "10-23 10:01:00")
    断言(any(x["步骤"] == "构建" and x["状态"] == "告警" for x in 突评["告警"]),
         "服务端：第 7 轮构建 200s=耗时告警（基线样本足）")
    告警口径 = 突评["告警"][0]["口径"]
    样本数前 = len(读样本(con, "linux-x86_64", "构建", 告警口径))
    记录并评估(con, "linux-x86_64", "f" * 40,
               _自测结果({"配置": 5, "构建": 200}, sha="f" * 40), "10-23 10:01:10")
    断言(len(读样本(con, "linux-x86_64", "构建", 告警口径)) == 样本数前,
         "服务端：同 sha 重报幂等（样本不重复计数）")
    for i in range(1, 样本上限 + 6):
        记录并评估(con, "linux-x86_64", "%040d" % (200 + i),
                   _自测结果({"配置": 5, "构建": 60}, sha="%040d" % (200 + i)),
                   "10-23 10:02:%02d" % (i % 60))
    样本 = 读样本(con, "linux-x86_64", "构建", 告警口径)
    断言(len(样本) == 样本上限, "服务端：样本裁剪至 %d 条（滚动窗）" % 样本上限)
    # 冷/温分离：同一构建秒数在暖建桶够 5 样本后不因冷建样本少而告警
    for i in range(1, 7):
        记录并评估(con, "linux-x86_64", "%040d" % (100 + i),
                   _自测结果({"配置": 5, "构建": 61}, sha="%040d" % (100 + i), 清产物=False),
                   "10-23 10:02:%02d" % i)
    暖评 = 记录并评估(con, "linux-x86_64", "e" * 40,
                     _自测结果({"配置": 5, "构建": 62}, sha="e" * 40, 清产物=False),
                     "10-23 10:03:00")
    断言(not 暖评["告警"], "服务端：温建 62s 对温建中位 61s=正常（冷暖分离实锤）")
    # 同 sha 双口径（预验冷建 → develop 温建）：幂等删除只管同键，冷样本不被温报删
    同sha = "d" * 40
    记录并评估(con, "linux-x86_64", 同sha,
               _自测结果({"配置": 5, "构建": 300}, sha=同sha), "10-23 10:03:10")
    记录并评估(con, "linux-x86_64", 同sha,
               _自测结果({"配置": 5, "构建": 60}, sha=同sha, 清产物=False), "10-23 10:03:20")
    双口径数 = con.execute("SELECT COUNT(*) FROM 耗时样本 WHERE 平台=? AND sha=?",
                         ("linux-x86_64", 同sha)).fetchone()[0]
    断言(双口径数 == 4,
         "服务端：同 sha 冷/温双口径样本各自保留（幂等删除只管同键同 sha·冷样本不被温报误删）")
    # 失败步骤不入样本
    坏 = _自测结果({"配置": 5, "构建": 60}, sha="b" * 40)
    坏["步骤"]["构建"] = {"rc": 1, "秒": 999}
    坏评 = 记录并评估(con, "linux-x86_64", "b" * 40, 坏, "10-23 10:04:00")
    断言(any(x["步骤"] == "构建" and x["状态"] == "未计入" for x in 坏评["步骤"]),
         "服务端：rc≠0 步骤=未计入（失败耗时不清洗正常基线）")
    断言(列告警(None) is None and 列告警('{"告警":[{"步骤":"构建"}]}') == [{"步骤": "构建"}],
         "列解析：NULL=未评估（None）·正常 JSON=告警列表")
    坏列 = 列告警("{坏 json")
    断言(isinstance(坏列, list) and 坏列 and "损坏" in 坏列[0]["原因"],
         "列解析：坏 JSON=显式告警条目（不静默当正常）")
    断言(解析列(None) is None, "列解析：NULL 与「已评估无告警」可区分")

    # ── runner 本地降级（临时目录）──
    import tempfile
    with tempfile.TemporaryDirectory() as 临时:
        for i in range(1, 7):
            结果 = _自测结果({"配置": 5, "构建": 60}, sha="%040d" % i)
            本地补充(结果, 临时)
        assert 结果.get("耗时告警")
        本地突 = _自测结果({"配置": 5, "构建": 200}, sha="f" * 40)
        本地补充(本地突, 临时)
        断言(any(x["步骤"] == "构建" for x in 本地突["耗时告警"]["告警"]),
             "本地降级：第 7 轮 200s=告警（断网轮仍可见）")
        断言(本地突["耗时告警"]["来源"].startswith("runner本机"), "本地降级：来源标注本机口径")
        路径 = Path(临时) / 本地基文件名
        断言(路径.exists(), "本地降级：基线文件已落盘（ci-logs/耗时基线.json）")
        本地突2 = _自测结果({"配置": 5, "构建": 200}, sha="f" * 40)
        告警键 = "构建" + _本机键分隔 + 本地突["耗时告警"]["告警"][0]["口径"]
        本地突前 = len(_读本地样本表(路径)[0][告警键])
        本地补充(本地突2, 临时)
        断言(len(_读本地样本表(路径)[0][告警键]) == 本地突前,
             "本地降级：同 sha 重报幂等（样本不重复计数）")
        with tempfile.TemporaryDirectory() as 空目录:
            冷 = _自测结果({"构建": 60}, sha="c" * 40)
            本地补充(冷, 空目录)
            断言(any(x["步骤"] == "构建" for x in 冷["耗时告警"]["不足"]),
                 "本地降级：空目录冷启动=显式基线不足")
        坏目录 = Path(临时) / "坏"
        坏目录.mkdir()
        (坏目录 / 本地基文件名).write_text("{坏", encoding="utf-8")
        坏结果 = _自测结果({"构建": 60}, sha="d" * 40)
        本地补充(坏结果, 坏目录)
        断言("损坏" in (坏结果["耗时告警"].get("错误") or ""),
             "本地降级：坏基线文件=显式错误（保留待人工）")
    print("ci_duration 自测：%d 例全过（正常/3×超时/样本不足/冷启动·口径分桶·服务端表·本地降级）"
          % 断言数[0])
    return 0


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] != "--selftest":
        print("用法：python scripts/ci_duration.py [--selftest]", flush=True)
        sys.exit(2)
    sys.exit(自测())
