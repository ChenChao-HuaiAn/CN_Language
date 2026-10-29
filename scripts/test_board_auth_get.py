#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""test_board_auth_get.py —— 1425 登录墙后只读 GET 全族令牌/401 分流自测（任务 1485）。

正反两态（零真实网络·mock urllib.request.urlopen）：
  正：令牌可得 → 各客户端 GET 请求必带 Authorization: Bearer（单点 intent.鉴权头）；
  反：令牌不可得 → 不带头（与登录墙前行为一致）；
  401：鉴权失败文案与「服务不可达」严格分流（点名 queue_client.json/环境变量令牌），
      wt.py GET 401 不得再误报「台账无任务」。

覆盖：intent/task_board/standup/decision_brief/branch_cleanup/check_spec_coverage/
infra_healthcheck/patrol_probe/wt/integrate/board_cli。check_deploy_manifest.py 无
8300/8301 HTTP 调用（纯静态清单门禁），不在断言面。

用法：python scripts/test_board_auth_get.py
"""
from __future__ import annotations

import contextlib
import io
import json
import os
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))

import board_cli          # noqa: E402
import branch_cleanup     # noqa: E402
import case_activity      # noqa: E402
import case_ledger        # noqa: E402
import check_case_numbers  # noqa: E402
import check_spec_coverage  # noqa: E402
import decision_brief     # noqa: E402
import infra_healthcheck  # noqa: E402
import integrate          # noqa: E402
import intent             # noqa: E402
import migrate_board_docs  # noqa: E402
import patrol_probe       # noqa: E402
import standup            # noqa: E402
import task_board         # noqa: E402
import wt                 # noqa: E402

通过 = 0
失败: list[str] = []


def 签(名: str, 条件: bool, 详情: str = "") -> None:
    global 通过
    if 条件:
        通过 += 1
        print(f"  [PASS] {名}")
    else:
        失败.append(名)
        print(f"  [FAIL] {名}" + (f"（{详情}）" if 详情 else ""))


class 假响应:
    """urlopen 上下文管理器最小替身（只实现读取面）。"""

    status = 200

    def __init__(self, 体: bytes):
        self.体 = 体

    def read(self) -> bytes:
        return self.体

    def __enter__(self):
        return self

    def __exit__(self, *_):
        return False


class 假服务:
    """按场景返回 200/401 并记录请求（含请求头）。"""

    def __init__(self):
        self.模式 = "ok"
        self.码 = 401
        self.体 = "{}"
        self.有令牌 = True
        self.记录: list[urllib.request.Request] = []
        self.请求日志: list[dict] = []   # 跨 env 块累计（令牌态请求级审计）
        self.免鉴权URL: set[str] = set()  # 按设计不该带令牌的 URL（外链/跨源探针）

    def __call__(self, 请求, timeout=None):
        self.记录.append(请求)
        头键 = {k.lower() for k, _v in 请求.header_items()}
        self.请求日志.append({"url": 请求.full_url,
                              "auth": "authorization" in 头键,
                              "有令牌": self.有令牌,
                              "期望鉴权": 请求.full_url not in self.免鉴权URL})
        if self.模式 == "http错误":
            raise urllib.error.HTTPError(
                请求.full_url, self.码, "Unauthorized", {},
                io.BytesIO('{"错误":"未登录或令牌不符（GET 亦须登录·1425 登录墙）"}'
                           .encode("utf-8")))
        if self.模式 == "连接错误":
            raise urllib.error.URLError("selftest connection refused")
        return 假响应(self.体.encode("utf-8"))

    def 头(self, 索引: int = -1) -> dict:
        return dict(self.记录[索引].header_items())


假 = 假服务()


@contextlib.contextmanager
def 环境(体: str = "{}", 模式: str = "ok", 有令牌: bool = True, 码: int = 401):
    """替换 urlopen + 令牌读取（确定性正反两态）；退出恢复。"""
    原urlopen, 原读令牌 = urllib.request.urlopen, intent.读令牌
    urllib.request.urlopen = 假
    intent.读令牌 = (lambda: "selftest-token") if 有令牌 else (lambda: "")
    假.模式, 假.体, 假.记录, 假.码, 假.有令牌 = 模式, 体, [], 码, 有令牌
    try:
        yield
    finally:
        urllib.request.urlopen = 原urlopen
        intent.读令牌 = 原读令牌


def 测_令牌头() -> None:
    print("[1] 令牌头单点（intent.鉴权头/读令牌兜底链）")
    with 环境():
        签("有令牌 → Bearer 头", intent.鉴权头().get("Authorization") == "Bearer selftest-token",
           str(intent.鉴权头()))
    with 环境(有令牌=False):
        签("令牌落空 → 裸请求（行为不变）", intent.鉴权头() == {}, str(intent.鉴权头()))


def 测_GET带令牌() -> None:
    print("[2] 全族 GET 请求携带 Authorization")
    with 环境():
        intent.调服务("GET", "/api/tasks")
        签("intent.调服务", 假.头().get("Authorization") == "Bearer selftest-token")

    with 环境():
        board_cli.调服务("GET", "/api/tasks")
        签("board_cli.调服务", 假.头().get("Authorization") == "Bearer selftest-token")

    with 环境(体='{"任务们": []}'):
        数据, 错误, 鉴权失败, 连接失败 = task_board.服务调("/api/tasks")
        签("task_board.服务调", 假.头().get("Authorization") == "Bearer selftest-token"
           and 数据 == {"任务们": []} and not 错误 and not 鉴权失败 and not 连接失败,
           f"{数据}/{错误}")

    with 环境(体='{"任务": {"号": "1485", "状态": "🏃"}}'):
        数据 = wt.服务调("GET", "/api/task/1485")
        签("wt.服务调 GET", 假.头().get("Authorization") == "Bearer selftest-token"
           and 数据.get("任务", {}).get("状态") == "🏃", str(数据))

    with 环境(体='{"条目们": []}'):
        原落 = standup._落缓存
        standup._落缓存 = lambda *_a, **_k: None
        try:
            standup.本机节("测试机")
        finally:
            standup._落缓存 = 原落
        签("standup.本机节", 假.头().get("Authorization") == "Bearer selftest-token")

    with 环境(体='{"任务们": []}'):
        原写, 原读 = decision_brief.atomic_json.合并写, decision_brief.atomic_json.读
        decision_brief.atomic_json.合并写 = lambda *_a, **_k: None
        try:
            任务们, 来源 = decision_brief.拉任务们()
        finally:
            decision_brief.atomic_json.合并写, decision_brief.atomic_json.读 = 原写, 原读
        签("decision_brief.拉任务们", 假.头().get("Authorization") == "Bearer selftest-token"
           and 来源 == "服务端台账", f"{任务们}/{来源}")

    with 环境(体='{"任务们": []}'):
        集合, 错误 = branch_cleanup.台账在飞分支集()
        签("branch_cleanup.台账在飞分支集", 假.头().get("Authorization") == "Bearer selftest-token"
           and 集合 == set() and not 错误, f"{集合}/{错误}")

    with 环境(体='{"单元们": [], "豁免们": []}'):
        原cache = check_spec_coverage.CACHE
        check_spec_coverage.CACHE = str(Path(tempfile.mkdtemp(prefix="cov_auth_")) / "c.json")
        try:
            rows, exempt = check_spec_coverage.parse_map()
        finally:
            check_spec_coverage.CACHE = 原cache
        签("check_spec_coverage.parse_map",
           假.头().get("Authorization") == "Bearer selftest-token"
           and rows == {} and exempt == {}, f"{rows}/{exempt}")

    with 环境(体="{}"):
        码, _体, _错, _毫秒 = infra_healthcheck.取地址("http://selftest", "/api/board")
        签("infra_healthcheck.取地址", 码 == 200
           and 假.头().get("Authorization") == "Bearer selftest-token", f"码={码}")

    with 环境(体='{"任务": {"号": "1485", "状态": "🏃"}}'):
        任务, 可达, 错误, 鉴权败 = patrol_probe.读任务("1485")
        签("patrol_probe.读任务（委托 board_cli）",
           假.头().get("Authorization") == "Bearer selftest-token"
           and 可达 and not 鉴权败 and 任务.get("状态") == "🏃", f"{任务}/{错误}")

    with 环境(体="{}"):
        integrate.板查询("/api/tasks")
        签("integrate.板查询", 假.头().get("Authorization") == "Bearer selftest-token")

    with 环境(体="{}"):
        码, _ = migrate_board_docs.调服务("http://board", "selftest-token", "GET", "/api/board")
        签("migrate_board_docs GET 也带 Bearer（不再限 POST）",
           码 == 200 and 假.头().get("Authorization") == "Bearer selftest-token", f"码={码}")

    with 环境(体="body"):
        假.免鉴权URL.add("http://cdn.example/theme.css")
        码, _体, _错, _毫秒 = infra_healthcheck.取地址(
            "http://cdn.example/theme.css", "", 同源基址="http://selftest")
        签("infra 外链/CDN 请求不携带看板令牌",
           码 == 200 and "Authorization" not in 假.头(), str(假.头()))


def 测_无令牌裸请求() -> None:
    print("[3] 令牌落空 → 各客户端不带 Authorization（行为不变）")
    with 环境(有令牌=False, 体='{"任务们": []}'):
        task_board.服务调("/api/tasks")
        签("task_board 无令牌裸请求", "Authorization" not in 假.头(), str(假.头()))
    with 环境(有令牌=False, 体='{"任务们": []}'):
        branch_cleanup.台账在飞分支集()
        签("branch_cleanup 无令牌裸请求", "Authorization" not in 假.头(), str(假.头()))


def 测_未认证分流() -> None:
    print("[4] 401 鉴权失败与「服务不可达」文案分流")
    with 环境(模式="http错误"):
        数据, 错误, 鉴权失败, 连接失败 = task_board.服务调("/api/tasks")
        签("task_board 401 → 鉴权失败（非不可达）",
           数据 is None and 鉴权失败 and not 连接失败 and "鉴权失败（401）" in 错误
           and "不可达" not in 错误, 错误)

    with 环境(模式="http错误"):
        集合, 错误 = branch_cleanup.台账在飞分支集()
        签("branch_cleanup 401 → 鉴权失败（非不可达）",
           集合 is None and "鉴权失败（401）" in 错误 and "不可达" not in 错误, 错误)

    with 环境(模式="http错误"):
        原写, 原读 = decision_brief.atomic_json.合并写, decision_brief.atomic_json.读
        decision_brief.atomic_json.合并写 = lambda *_a, **_k: None
        decision_brief.atomic_json.读 = lambda _p: {
            "tasks": [{"号": "1", "标题": "缓存", "状态": "⏸", "优先级": "P1",
                       "备注": "", "前置": ""}]}
        try:
            任务们, 来源 = decision_brief.拉任务们()
        finally:
            decision_brief.atomic_json.合并写, decision_brief.atomic_json.读 = 原写, 原读
        签("decision_brief 401 → 缓存来源点名鉴权失败",
           len(任务们) == 1 and "鉴权失败（401）" in 来源, 来源)

    with 环境(模式="http错误"):
        原cache = check_spec_coverage.CACHE
        check_spec_coverage.CACHE = str(Path(tempfile.mkdtemp(prefix="cov_auth_")) / "无缓存.json")
        错误流 = io.StringIO()
        try:
            with contextlib.redirect_stderr(错误流):
                try:
                    check_spec_coverage.parse_map()
                except SystemExit as e:
                    码 = e.code
        finally:
            check_spec_coverage.CACHE = 原cache
        文本 = 错误流.getvalue()
        签("check_spec_coverage 401 无缓存 → 鉴权失败 exit 2",
           码 == 2 and "鉴权失败（401）" in 文本 and "不可达" not in 文本, 文本)

    with 环境(模式="http错误"):
        原服务调用 = patrol_probe.服务调用
        patrol_probe.服务调用 = lambda *_a, **_k: (
            401, {"错误": "未登录或令牌不符（GET 亦须登录·1425 登录墙）"})
        try:
            任务, 可达, 错误, 鉴权败 = patrol_probe.读任务("1485")
        finally:
            patrol_probe.服务调用 = 原服务调用
        签("patrol_probe 401 → 鉴权失败位（不误报任务不存在）",
           任务 is None and 可达 and 鉴权败 and "鉴权失败（401）" in 错误, 错误)

    with 环境(模式="http错误"):
        输出流 = io.StringIO()
        with contextlib.redirect_stdout(输出流):
            数据 = integrate.板查询("/api/tasks")
        文本 = 输出流.getvalue()
        签("integrate 板查询 401 → 鉴权失败黄字（非静默 None）",
           数据 is None and "鉴权失败（401）" in 文本 and "不可达" not in 文本, 文本)

    鉴权 = standup._失败文案(urllib.error.HTTPError("http://x", 401, "x", {}, io.BytesIO(b"{}")))
    签("standup 失败文案 401 → 鉴权失败", "鉴权失败（401）" in 鉴权 and "不可达" not in 鉴权, 鉴权)

    说明 = infra_healthcheck._失败说明(401)
    签("infra_healthcheck 401 → 鉴权失败", "鉴权失败（401）" in 说明 and "不可达" not in 说明, 说明)

    文案 = board_cli._读失败文案("看板服务", 401, {})
    签("board_cli 读失败文案 401 → 鉴权失败", "鉴权失败（401）" in 文案, 文案)

    文案 = wt.鉴权失败文案({"HTTP错误": 401})
    签("wt GET 响应 401 → 鉴权失败（不再落到台账无任务）", "鉴权失败（401）" in 文案, 文案)

    with 环境(模式="http错误"):
        响应 = wt.服务调("GET", "/api/task/1485")
        签("wt.服务调 401 保留 HTTP错误 码",
           isinstance(响应, dict) and 响应.get("HTTP错误") == 401
           and "鉴权失败（401）" in wt.鉴权失败文案(响应), str(响应))

    with 环境(模式="http错误"):
        码, _体, 错, _毫秒 = infra_healthcheck.取地址("http://selftest", "/api/board")
        签("infra_healthcheck 取地址 401 显形为 401（非连接失败 0）",
           码 == 401 and not 错 and 假.头().get("Authorization") == "Bearer selftest-token",
           f"码={码}/{错}")


class 假参数:
    """board_cli 写入口最小参数容器。"""


def 测_无权限分流() -> None:
    print("[5] 403 同口径鉴权失败")
    with 环境(模式="http错误", 码=403):
        数据, 错误, 鉴权失败, 连接失败 = task_board.服务调("/api/tasks")
        签("task_board 403 → 鉴权失败（403·权限不足）",
           数据 is None and 鉴权失败 and not 连接失败 and "鉴权失败（403）" in 错误
           and "权限不足" in 错误 and "不可达" not in 错误, 错误)
    with 环境(模式="http错误", 码=403):
        集合, 错误 = branch_cleanup.台账在飞分支集()
        签("branch_cleanup 403 → 鉴权失败（403）",
           集合 is None and "鉴权失败（403）" in 错误 and "不可达" not in 错误, 错误)


def 测_断网态() -> None:
    print("[5b] 连接层失败仍全部「不可达」（不冒充鉴权失败）")
    with 环境(模式="连接错误"):
        数据, 错误, 鉴权失败, 连接失败 = task_board.服务调("/api/tasks")
        签("task_board 断网 → 不可达", 数据 is None and not 鉴权失败 and 连接失败
           and "不可达" in 错误 and "鉴权失败" not in 错误, 错误)
    with 环境(模式="连接错误"):
        集合, 错误 = branch_cleanup.台账在飞分支集()
        签("branch_cleanup 断网 → 不可达", 集合 is None and "不可达" in 错误
           and "鉴权失败" not in 错误, 错误)
    with 环境(模式="连接错误"):
        行们, 可用, 原因, 鉴权 = case_ledger.台账取回()
        签("case_ledger 断网 → 不可达", not 可用 and not 鉴权 and "不可达" in 原因
           and "鉴权失败" not in 原因, 原因)
    with 环境(模式="连接错误"):
        原argv = sys.argv
        sys.argv = ["check_case_numbers.py"]
        输出 = io.StringIO()
        try:
            with contextlib.redirect_stdout(输出):
                返回 = check_case_numbers.main()
        finally:
            sys.argv = 原argv
        文本 = 输出.getvalue()
        签("check_case_numbers 断网 → 降级黄字不冒充鉴权失败",
           返回 in (0, 1) and "不可达" in 文本 and "鉴权失败" not in 文本,
           f"rc={返回}·{文本[-160:]}")


def 测_用例台账未认证() -> None:
    print("[6] case_ledger/check_case_numbers 401/403 不静默放行")
    临时 = Path(tempfile.mkdtemp(prefix="case_ledger_auth_")) / "ledger.json"
    原缓存 = case_ledger.缓存
    case_ledger.缓存 = 临时
    原argv = sys.argv
    try:
        with 环境(模式="http错误"):
            行们, 可用, 原因, 鉴权 = case_ledger.台账取回()
            签("case_ledger 台账取回 401 → 鉴权失败位且原因点名",
               行们 == [] and not 可用 and 鉴权 and "鉴权失败（401）" in 原因
               and "不可达" not in 原因, 原因)
            签("case_ledger 401 无缓存 → 不写缓存", not 临时.exists())
        旧文 = '{"台账": [{"号": "1", "用例名": "旧"}]}'
        临时.write_text(旧文, encoding="utf-8")
        with 环境(模式="http错误"):
            行们, 可用, 原因, 鉴权 = case_ledger.台账取回()
            签("case_ledger 401 有缓存 → 只读且不写盘",
               len(行们) == 1 and not 可用 and 鉴权 and "读本地缓存" in 原因
               and 临时.read_text(encoding="utf-8") == 旧文, 原因)

        for 码, 标签 in ((401, "401"), (403, "403")):
            输出 = io.StringIO()
            sys.argv = ["check_case_numbers.py"]
            try:
                with 环境(模式="http错误", 码=码), contextlib.redirect_stdout(输出):
                    返回 = check_case_numbers.main()
            finally:
                sys.argv = 原argv
            文本 = 输出.getvalue()
            签(f"check_case_numbers {标签} → 硬红 rc=1 且点名鉴权失败",
               返回 == 1 and f"鉴权失败（{码}）" in 文本 and "服务不可达" not in 文本,
               f"rc={返回}·{文本[-160:]}")
    finally:
        case_ledger.缓存 = 原缓存
        sys.argv = 原argv


def 测_写入口未认证() -> None:
    print("[7] board_cli 写入口 401 统一鉴权失败文案")
    场景们 = [
        ("交接", board_cli.cmd_交接, {"行": "x", "机器": ""}),
        ("收工", board_cli.cmd_收工, {"行": "x", "机器": ""}),
        ("教训", board_cli.cmd_教训, {"标题": "x", "正文": "", "权重": 1, "标注": ""}),
        ("用例", board_cli.cmd_用例, {"态": "正例", "用例名": "x", "单元ID": "1"}),
        ("豁免", board_cli.cmd_豁免, {"用例名": "x", "理由": "y"}),
    ]
    for 名, 函数, 字段 in 场景们:
        with 环境(模式="http错误"):
            a = 假参数()
            for 键, 值 in 字段.items():
                setattr(a, 键, 值)
            输出 = io.StringIO()
            with contextlib.redirect_stdout(输出):
                返回 = 函数(a)
            文本 = 输出.getvalue()
            签(f"board_cli 写入口 {名} 401 → 鉴权失败文案",
               返回 == 1 and "鉴权失败（401）" in 文本, 文本[-160:])

    临时 = Path(tempfile.mkdtemp(prefix="boardcli_cache_"))
    原目录 = board_cli.脚本目录
    board_cli.脚本目录 = 临时
    try:
        with 环境(模式="http错误"):
            输出 = io.StringIO()
            with contextlib.redirect_stdout(输出):
                返回 = board_cli.cmd_覆盖(假参数())
            签("board_cli 读入口 401 → 不写 board_cache（防错误体污染）",
               返回 == 1 and not (临时 / "board_cache.json").exists(),
               f"rc={返回}·{输出.getvalue()[-120:]}")
    finally:
        board_cli.脚本目录 = 原目录


def 测_全入口未认证矩阵() -> None:
    print("[8] 全入口 401 矩阵（12 读入口逐个走真 401 路径）")
    临时 = Path(tempfile.mkdtemp(prefix="matrix_auth_"))
    原cov, 原ledger = check_spec_coverage.CACHE, case_ledger.缓存
    原落 = standup._落缓存
    check_spec_coverage.CACHE = str(临时 / "cov.json")
    case_ledger.缓存 = 临时 / "ledger.json"
    standup._落缓存 = lambda *_a, **_k: None
    入口们 = (
        ("intent.调服务", lambda: intent.调服务("GET", "/api/tasks"), "401"),
        ("board_cli.调服务", lambda: board_cli.调服务("GET", "/api/tasks"), "401"),
        ("task_board.服务调", lambda: task_board.服务调("/api/tasks"), "鉴权失败（401）"),
        ("wt.服务调", lambda: wt.服务调("GET", "/api/task/1"), "401"),
        ("standup.本机节", lambda: standup.本机节("测试机"), "鉴权失败（401）"),
        ("decision_brief.拉任务们", lambda: decision_brief.拉任务们(), "鉴权失败（401）"),
        ("branch_cleanup.台账在飞分支集", lambda: branch_cleanup.台账在飞分支集(),
         "鉴权失败（401）"),
        ("check_spec_coverage.parse_map", lambda: check_spec_coverage.parse_map(),
         "鉴权失败（401）"),
        ("infra_healthcheck.取地址",
         lambda: infra_healthcheck.取地址("http://selftest", "/api/board"), "401"),
        ("integrate.板查询", lambda: integrate.板查询("/api/tasks"), "鉴权失败（401）"),
        ("case_ledger.台账取回", lambda: case_ledger.台账取回(), "鉴权失败（401）"),
        ("patrol_probe.读任务", lambda: patrol_probe.读任务("1"), "鉴权失败（401）"),
    )
    try:
        with 环境(模式="http错误"):
            for 名, 调, 标记 in 入口们:
                输出, 错误流 = io.StringIO(), io.StringIO()
                with contextlib.redirect_stdout(输出), contextlib.redirect_stderr(错误流):
                    try:
                        值 = 调()
                    except SystemExit as e:
                        值 = f"SystemExit({e.code})"
                    except Exception as e:
                        值 = f"{type(e).__name__}: {e}"
                关键 = 值
                if isinstance(值, tuple):
                    if 名 in ("case_ledger.台账取回", "patrol_probe.读任务"):
                        关键 = 值[2]
                    elif 名 in ("task_board.服务调", "branch_cleanup.台账在飞分支集",
                                "decision_brief.拉任务们"):
                        关键 = 值[1]
                    elif 名 == "infra_healthcheck.取地址":
                        关键 = infra_healthcheck._失败说明(值[0])
                文本 = f"{关键!r} {输出.getvalue()} {错误流.getvalue()}"
                签(f"矩阵 {名} 401 → 点名 {标记}（非不可达）",
                   标记 in 文本 and "不可达" not in 文本, 文本[-180:])
    finally:
        check_spec_coverage.CACHE, case_ledger.缓存 = 原cov, 原ledger
        standup._落缓存 = 原落


def _注解节点们(tree):
    """收集全部注解位置节点（函数参数/返回/AnnAssign）——只看注解位，不看表达式位
    （set 并集等非注解 `|` 不会误报）。"""
    import ast
    for node in ast.walk(tree):
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.Lambda)):
            a = node.args
            for arg in list(a.posonlyargs) + list(a.args) + list(a.kwonlyargs):
                if arg.annotation is not None:
                    yield arg.annotation
            for arg in (a.vararg, a.kwarg):
                if arg is not None and arg.annotation is not None:
                    yield arg.annotation
            if getattr(node, "returns", None) is not None:
                yield node.returns
        elif isinstance(node, ast.AnnAssign) and node.annotation is not None:
            yield node.annotation


def _注解违规(ann) -> list:
    """注解位内的 PEP604（`X | Y`）/PEP585（`list[...]` 等内建泛型）命中。"""
    import ast
    命中 = []
    for n in ast.walk(ann):
        if isinstance(n, ast.BinOp) and isinstance(n.op, ast.BitOr):
            命中.append("PEP604")
        elif (isinstance(n, ast.Subscript) and isinstance(n.value, ast.Name)
              and n.value.id in {"list", "dict", "tuple", "set"}):
            命中.append("PEP585:" + n.value.id)
    return 命中


def 测_py38兼容全扫描() -> None:
    print("[0] Python 3.8 兼容全扫描（P1-1）")
    # 注：ast.parse(feature_version=(3,8)) 只拦新语法，**不能**拦 PEP604 注解的
    # 运行期求值（它是表达式）；后者由「注解位 AST 扫描 + future import 断言」负责。
    import ast
    语法坏, 违规们 = [], []
    for 路径 in sorted(Path(__file__).resolve().parent.glob("*.py")):
        源 = 路径.read_text(encoding="utf-8")
        try:
            tree = ast.parse(源, filename=str(路径), feature_version=(3, 8))
        except SyntaxError as e:
            语法坏.append(f"{路径.name}: {e}")
            continue
        if "from __future__ import annotations" in 源:
            continue
        命中 = []
        for ann in _注解节点们(tree):
            命中 += _注解违规(ann)
        if 命中:
            违规们.append(f"{路径.name}（{','.join(sorted(set(命中)))}）")
    签("scripts/*.py 3.8 语法面全部可解析", not 语法坏, "；".join(语法坏[:6]))
    签("scripts/*.py 注解位 PEP604/PEP585 均有 future annotations",
       not 违规们, "；".join(违规们[:10]))
    print(f"  扫描 scripts/*.py：语法坏 {len(语法坏)}·缺 future 违规 {len(违规们)}")
    for x in 违规们:
        print(f"   - 违规：{x}")


def 测_响应畸形硬红() -> None:
    print("[8d] task_board --check 200+畸形响应 → rc=1（P2-2）")
    for 标签, 体, 标记 in (
            ("HTML/非 JSON", "not-json", "响应非 JSON"),
            ("null", "null", "响应为 null"),
            ("空对象", "{}", "缺「任务们」"),
            ("数组", "[1,2]", "响应非对象"),
            ("元素非对象", '{"任务们": [1]}', "元素畸形"),
            ("元素空对象", '{"任务们": [{}]}', "元素畸形")):
        with 环境(体=体):
            输出 = io.StringIO()
            with contextlib.redirect_stdout(输出), contextlib.redirect_stderr(输出):
                返回 = task_board.cmd_check()
            文本 = 输出.getvalue()
            签(f"task_board --check 200+{标签} → rc=1 硬红",
               返回 == 1 and 标记 in 文本 and "断网降级" not in 文本, 文本[-180:])


def 测_覆盖响应畸形不落盘() -> None:
    print("[8e] check_spec_coverage 非 dict/坏 JSON：不写缓存、有缓存降级、无缓存 exit 2（P2-1）")
    临时 = Path(tempfile.mkdtemp(prefix="cov_nondict_")) / "cov.json"
    原CACHE = check_spec_coverage.CACHE
    check_spec_coverage.CACHE = str(临时)
    try:
        with 环境(体="[1,2,3]"):
            错误流 = io.StringIO()
            with contextlib.redirect_stderr(错误流):
                try:
                    check_spec_coverage.parse_map()
                    码 = "未退出"
                except SystemExit as e:
                    码 = e.code
        签("覆盖 200+数组 → 响应异常 exit 2 且不写缓存",
           码 == 2 and "响应异常" in 错误流.getvalue() and not 临时.exists(),
           f"码={码}·{错误流.getvalue()[-160:]}")

        for 标签, 坏体 in (("单元们字符串", '{"单元们": "abc"}'),
                           ("单元们元素空对象", '{"单元们": [{}]}'),
                           ("豁免们非列表", '{"单元们": [], "豁免们": "abc"}')):
            with 环境(体=坏体):
                错误流 = io.StringIO()
                with contextlib.redirect_stderr(错误流):
                    try:
                        check_spec_coverage.parse_map()
                        码x = "未退出"
                    except SystemExit as e:
                        码x = e.code
            签(f"覆盖 200+{标签} → 结构异常 exit 2 且不写缓存",
               码x == 2 and "结构异常" in 错误流.getvalue() and not 临时.exists(),
               f"码={码x}·{错误流.getvalue()[-160:]}")

        旧文 = ('{"单元们": [{"单元ID": "x", "正例": "a", "边界例": "", "负例": ""}],'
                ' "豁免们": []}')
        临时.write_text(旧文, encoding="utf-8")
        with 环境(体="[1,2,3]"):
            错误流 = io.StringIO()
            with contextlib.redirect_stderr(错误流):
                行们, 豁免 = check_spec_coverage.parse_map()
        签("覆盖 200+数组 → 降级读合法缓存且坏体不覆盖",
           "x" in 行们 and 临时.read_text(encoding="utf-8") == 旧文
           and "响应异常" in 错误流.getvalue(), 错误流.getvalue()[-160:])

        临时.write_text("[1,2,3]", encoding="utf-8")
        with 环境(体="[1,2,3]"):
            错误流 = io.StringIO()
            with contextlib.redirect_stderr(错误流):
                try:
                    check_spec_coverage.parse_map()
                    码2 = "未退出"
                except SystemExit as e:
                    码2 = e.code
        签("覆盖 坏缓存 + 数组响应 → exit 2 不崩",
           码2 == 2 and "响应异常" in 错误流.getvalue(), f"码={码2}")

        临时.write_text('{"其他": 1}', encoding="utf-8")
        with 环境(体="[1,2,3]"):
            错误流 = io.StringIO()
            with contextlib.redirect_stderr(错误流):
                try:
                    check_spec_coverage.parse_map()
                    码3 = "未退出"
                except SystemExit as e:
                    码3 = e.code
        签("覆盖 结构不合法缓存（{其他:1}）按无缓存 → exit 2",
           码3 == 2 and "响应异常" in 错误流.getvalue(), f"码={码3}")

        原parse = check_spec_coverage.parse_map
        错误流 = io.StringIO()
        try:
            check_spec_coverage.parse_map = lambda: (_ for _ in ()).throw(
                ValueError("覆盖服务响应非对象（list）"))
            with contextlib.redirect_stderr(错误流):
                名表, 可用, 原因, 鉴权 = case_activity.取覆盖登记面()
        finally:
            check_spec_coverage.parse_map = 原parse
        签("case_activity 覆盖响应异常兜底不崩",
           not 可用 and "响应异常" in 原因, 原因)
    finally:
        check_spec_coverage.CACHE = 原CACHE


def 测_check硬红() -> None:
    print("[8b] task_board --check 401 硬红 / 断网降级（P2-D）")
    with 环境(模式="http错误"):
        输出 = io.StringIO()
        with contextlib.redirect_stdout(输出), contextlib.redirect_stderr(输出):
            返回 = task_board.cmd_check()
        文本 = 输出.getvalue()
        签("task_board --check 401 → rc=1 硬红",
           返回 == 1 and "硬红" in 文本 and "鉴权失败（401）" in 文本, 文本[-160:])
    with 环境(模式="连接错误"):
        输出 = io.StringIO()
        with contextlib.redirect_stdout(输出), contextlib.redirect_stderr(输出):
            返回 = task_board.cmd_check()
        文本 = 输出.getvalue()
        签("task_board --check 断网 → rc=0 黄字降级",
           返回 == 0 and "跳过" in 文本 and "不可达" in 文本, 文本[-160:])


def 测_重定向不转发令牌() -> None:
    print("[13] 302 跨源重定向不转发 Authorization（P2-A）")
    收到 = {"入口": [], "外源": []}

    class 外源(BaseHTTPRequestHandler):
        def do_GET(self):
            收到["外源"].append(self.headers.get("Authorization"))
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write('{"任务们": []}'.encode("utf-8"))

        def log_message(self, *_a):
            pass

    外源服务 = ThreadingHTTPServer(("127.0.0.1", 0), 外源)
    外源端口 = 外源服务.server_address[1]

    class 入口(BaseHTTPRequestHandler):
        def do_GET(self):
            收到["入口"].append(self.headers.get("Authorization"))
            self.send_response(302)
            self.send_header("Location", f"http://127.0.0.1:{外源端口}/target")
            self.end_headers()

        def log_message(self, *_a):
            pass

    入口服务 = ThreadingHTTPServer(("127.0.0.1", 0), 入口)
    入口端口 = 入口服务.server_address[1]
    线程们 = [threading.Thread(target=s.serve_forever, daemon=True)
              for s in (外源服务, 入口服务)]
    for t in 线程们:
        t.start()
    原env, 原读 = os.environ.get("CN_BOARD_URL"), intent.读令牌
    intent.读令牌 = lambda: "redirect-token"
    os.environ["CN_BOARD_URL"] = f"http://127.0.0.1:{入口端口}"
    try:
        码, 体 = board_cli.调服务("GET", "/api/tasks")
    finally:
        intent.读令牌 = 原读
        if 原env is None:
            os.environ.pop("CN_BOARD_URL", None)
        else:
            os.environ["CN_BOARD_URL"] = 原env
        for s in (入口服务, 外源服务):
            s.shutdown()
            s.server_close()
    签("重定向入口收到 Bearer（首请求照发）",
       收到["入口"] and 收到["入口"][0] == "Bearer redirect-token", str(收到))
    签("302 跨端口外源未收到 Authorization（token_leak=False）",
       收到["外源"] and 收到["外源"][0] is None, str(收到))
    签("重定向目标响应正常返回", 码 == 200 and 体 == {"任务们": []}, f"{码}/{体}")


def 测_同源三元组() -> None:
    print("[14] infra 同源判据 (scheme, host, port) 三元组（P2-A）")
    with 环境(体="{}"):
        假.免鉴权URL.add("http://example.com:443/x")
        码, *_ = infra_healthcheck.取地址(
            "http://example.com:443/x", "", 同源基址="https://example.com")
        签("https 基址 + http 目标（同 host/port）→ 不带头",
           码 == 200 and "Authorization" not in 假.头(), str(假.头()))
    with 环境(体="{}"):
        假.免鉴权URL.add("http://example.com:8080/x")
        码, *_ = infra_healthcheck.取地址(
            "http://example.com:8080/x", "", 同源基址="http://example.com:8081")
        签("跨端口同 host → 不带头",
           码 == 200 and "Authorization" not in 假.头(), str(假.头()))
    with 环境(体="{}"):
        码, *_ = infra_healthcheck.取地址(
            "http://example.com:8081/ok", "", 同源基址="http://example.com:8081")
        签("三元组全同 → 带头",
           码 == 200 and 假.头().get("Authorization") == "Bearer selftest-token",
           str(假.头()))


def 测_case_activity分列() -> None:
    print("[8c] case_activity 台账/覆盖原因透传与鉴权失败分列（P2-C）")
    原取 = case_ledger.台账取回
    case_ledger.台账取回 = lambda 允许缓存=True: (
        [], False, "看板服务鉴权失败（401）——令牌校验失败", True)
    try:
        行们, 可用, 原因, 鉴权 = case_activity.取号台账登记面()
    finally:
        case_ledger.台账取回 = 原取
    签("case_activity 号台账四元组透传",
       行们 == [] and not 可用 and 鉴权 and "鉴权失败（401）" in 原因, 原因)

    快照 = {"用例": {}, "统计": {}, "平台差异": {}, "重负载": [], "非编号": []}
    报告 = case_activity.比对(
        快照, {}, False, [], False,
        覆盖原因="覆盖服务鉴权失败（401）", 覆盖鉴权失败=True,
        台账原因="看板服务鉴权失败（401）", 台账鉴权失败=True)
    输出 = io.StringIO()
    with contextlib.redirect_stdout(输出):
        case_activity.打印报告(报告, "", 0)
    文本 = 输出.getvalue()
    签("报告按鉴权失败分列（覆盖/号台账）",
       "覆盖矩阵 鉴权失败（覆盖服务鉴权失败（401））" in 文本
       and "号台账 鉴权失败（看板服务鉴权失败（401））" in 文本
       and "不可达（降级）" not in 文本, 文本[:300])

    报告2 = case_activity.比对(
        快照, {}, False, [], False,
        覆盖原因="覆盖服务不可达", 覆盖鉴权失败=False,
        台账原因="看板服务不可达", 台账鉴权失败=False)
    输出2 = io.StringIO()
    with contextlib.redirect_stdout(输出2):
        case_activity.打印报告(报告2, "", 0)
    签("报告按不可达分列", "覆盖矩阵 不可达（覆盖服务不可达）" in 输出2.getvalue()
       and "号台账 不可达（看板服务不可达）" in 输出2.getvalue(), 输出2.getvalue()[:300])


def 测_令牌缓存() -> None:
    print("[9] 读令牌进程内缓存 + mtime 失效")
    临时 = Path(tempfile.mkdtemp(prefix="tok_cache_"))
    配置 = 临时 / "queue_client.json"
    配置.write_text('{"令牌": "t1"}', encoding="utf-8")
    原脚本目录 = intent.脚本目录
    原缓存 = dict(intent._令牌缓存)
    原主树 = dict(intent._主树目录缓存)
    intent.脚本目录 = 临时
    intent._令牌缓存.update(键=None, 值="")
    intent._主树目录缓存.update(已算=True, 值=None)   # 只看临时副本，不触真树
    try:
        签("读令牌首次 = t1", intent.读令牌() == "t1")
        签("读令牌缓存命中 = t1", intent.读令牌() == "t1")
        time.sleep(0.01)
        配置.write_text('{"令牌": "t2"}', encoding="utf-8")
        此刻 = time.time_ns()
        os.utime(配置, ns=(此刻, 此刻))
        签("令牌文件 mtime 变化 → 缓存失效取 t2", intent.读令牌() == "t2",
           intent.读令牌())

        原run = intent.subprocess.run
        次数 = {"n": 0}

        def 假run(*_a, **_k):
            次数["n"] += 1
            return type("R", (), {"returncode": 0,
                                  "stdout": str(临时 / "不是-git-common-dir") + "\n"})()

        intent._主树目录缓存.update(已算=False, 值=None)
        intent.subprocess.run = 假run
        try:
            intent.读令牌()
            intent.读令牌()
        finally:
            intent.subprocess.run = 原run
        签("主树目录反推只跑一次 git rev-parse（进程内缓存）", 次数["n"] == 1,
           f"调用 {次数['n']} 次")
    finally:
        intent.脚本目录 = 原脚本目录
        intent._令牌缓存.clear()
        intent._令牌缓存.update(原缓存)
        intent._主树目录缓存.clear()
        intent._主树目录缓存.update(原主树)


def 测_诊断与来源() -> None:
    print("[10] 静默诊断/空响应真因/来源字段")
    with 环境(体="{}"):
        _, 错误, 鉴权失败, 连接失败 = task_board.拉台账()
        签("task_board 空响应 → 缺「任务们」真因",
           not 鉴权失败 and not 连接失败 and "缺「任务们」" in 错误, 错误)

    with 环境(体='{"条目们": []}'):
        原落 = standup._落缓存
        standup._落缓存 = lambda *_a, **_k: None
        try:
            文本 = "\n".join(standup.教训高权重())
        finally:
            standup._落缓存 = 原落
        签("standup 服务端无数据 ≠ 降级无缓存",
           "服务端无数据" in 文本 and "无缓存" not in 文本, 文本)

    with 环境(模式="http错误"):
        原读 = standup._读缓存
        standup._读缓存 = lambda *_a, **_k: {}
        try:
            文本 = "\n".join(standup.教训高权重())
        finally:
            standup._读缓存 = 原读
        签("standup 401 且无缓存 → 鉴权失败真因",
           "鉴权失败（401）" in 文本 and "且无缓存" in 文本, 文本)

    with 环境(体='{"任务们": []}'):
        原写 = decision_brief.atomic_json.合并写
        decision_brief.atomic_json.合并写 = lambda *_a, **_k: None
        原argv = sys.argv
        sys.argv = ["decision_brief.py", "--json"]
        输出 = io.StringIO()
        try:
            with contextlib.redirect_stdout(输出):
                返回 = decision_brief.主流程()
        finally:
            sys.argv = 原argv
            decision_brief.atomic_json.合并写 = 原写
        数据 = json.loads(输出.getvalue())
        签("decision_brief --json 含「来源」字段",
           返回 == 0 and 数据.get("来源") == "服务端台账" and 数据.get("件们") == [],
           str(数据)[:160])

    with 环境(模式="http错误"):
        原集 = set(intent._已收口鉴权告警)
        intent._已收口鉴权告警.clear()
        流1, 流2 = io.StringIO(), io.StringIO()
        try:
            with contextlib.redirect_stderr(流1):
                intent.已收口("1")
            with contextlib.redirect_stderr(流2):
                intent.已收口("2")
        finally:
            intent._已收口鉴权告警.clear()
            intent._已收口鉴权告警.update(原集)
        签("intent.已收口 401 → 鉴权失败诊断（本进程限频一行）",
           "鉴权失败（401）" in 流1.getvalue() and 流2.getvalue() == "",
           f"首={流1.getvalue()!r} 次={流2.getvalue()!r}")

    with 环境(模式="http错误"):
        原跑 = standup.跑
        原根 = standup.本树根

        def 假跑(命令, cwd=None):
            if 命令[:2] == ["git", "branch"]:
                return ""
            if 命令[:2] == ["git", "ls-remote"]:
                return "sha refs/heads/任务/1485"
            return ""

        standup.跑 = 假跑
        # 本断言走「树名 wt<号>」接棒分支；worktree 实际名可能不是 wt<数字>
        #（如 wt-red/批次树），只把名称替换为规范 wt1485，避免测试依赖检出目录名。
        standup.本树根 = 原根.parent / "wt1485"
        错误流 = io.StringIO()
        try:
            with contextlib.redirect_stderr(错误流):
                standup.看板自动claim()
        finally:
            standup.跑 = 原跑
            standup.本树根 = 原根
        签("standup 看板自动登记 401 → 一行鉴权诊断",
           "鉴权失败（401）" in 错误流.getvalue(), 错误流.getvalue())

    import builtins
    原import = builtins.__import__

    def 坏import(name, *a, **k):
        if name == "intent":
            raise ImportError("selftest 模拟 intent 缺失")
        return 原import(name, *a, **k)

    with 环境(有令牌=False, 体='{"任务们": []}'):
        builtins.__import__ = 坏import
        错误流 = io.StringIO()
        try:
            with contextlib.redirect_stderr(错误流):
                task_board.服务调("/api/tasks")
        finally:
            builtins.__import__ = 原import
        签("intent 缺失 → stderr 一行诊断而非静默裸请求",
           "鉴权助手不可用" in 错误流.getvalue(), 错误流.getvalue())

    with 环境(有令牌=False, 模式="http错误"):
        错误流 = io.StringIO()
        builtins.__import__ = 坏import
        try:
            with contextlib.redirect_stderr(错误流):
                数据, 错误, 鉴权, 连接失败 = task_board.服务调("/api/tasks")
        finally:
            builtins.__import__ = 原import
        签("P3-D intent 缺失时 401 本地兜底仍硬识别",
           数据 is None and 鉴权 and not 连接失败 and "鉴权失败（401）" in 错误, 错误)

    with 环境(有令牌=False):
        错误流 = io.StringIO()
        builtins.__import__ = 坏import
        try:
            with contextlib.redirect_stderr(错误流):
                文案 = standup._失败文案(urllib.error.HTTPError(
                    "http://x", 401, "Unauthorized", {}, io.BytesIO(b"{}")))
        finally:
            builtins.__import__ = 原import
        签("P3-1 standup._失败文案 助手异常 → stderr 诊断 + 401 兜底",
           "鉴权助手不可用" in 错误流.getvalue()
           and "鉴权失败（401）" in 文案, f"{错误流.getvalue()}·{文案}")

    with 环境(体="not-json"):
        码, r = migrate_board_docs.调服务("http://selftest", "tok", "GET", "/x")
        签("P3-A JSON 解析错误 → 码 -1（不当连接层失败）", 码 == -1, f"码={码}")
        签("P3-A 失败原因 非 JSON 单列",
           "非 JSON" in migrate_board_docs.失败原因(码, r, "http://selftest"),
           migrate_board_docs.失败原因(码, r, "http://selftest"))
    签("P3-A 后续写失败走失败原因",
       "HTTP 409" in migrate_board_docs.失败原因(409, {"错误": "冲突"}, "http://x"),
       migrate_board_docs.失败原因(409, {"错误": "冲突"}, "http://x"))
    体, 错误 = migrate_board_docs.响应体(200, [1], "http://x", "教训")
    签("P3-5 migrate 成功码+非 dict → 明确错误不取字段",
       体 is None and "响应非对象" in 错误, 错误)
    体, 错误 = migrate_board_docs.响应体(200, {"跳过": 1}, "http://x", "教训")
    签("P3-5 migrate 成功码+dict → 通过", 体 == {"跳过": 1} and not 错误, 错误)
    体, 错误 = migrate_board_docs.响应体(401, {"错误": "x"}, "http://x", "教训")
    签("P3-5 migrate 401 → 鉴权失败文案", 体 is None and "鉴权失败（401）" in 错误, 错误)

    with 环境(体='{"其他": []}'):
        原读 = decision_brief.atomic_json.读
        decision_brief.atomic_json.读 = lambda _p: None
        错误流 = io.StringIO()
        try:
            with contextlib.redirect_stderr(错误流):
                任务们, 来源 = decision_brief.拉任务们()
        finally:
            decision_brief.atomic_json.读 = 原读
        签("P3-B decision_brief 缺「任务们」→ 真因",
           "缺「任务们」" in 错误流.getvalue(), 错误流.getvalue())

    with 环境(体='{"其他": []}'):
        原读 = standup._读缓存
        standup._读缓存 = lambda *_a, **_k: {}
        try:
            文本 = "\n".join(standup.教训高权重())
        finally:
            standup._读缓存 = 原读
        签("P3-B standup 缺「条目们」→ 真因",
           "缺「条目们」" in 文本, 文本)

    with 环境(体="[]"):
        _, 错误, 鉴权, 连接失败 = task_board.拉台账()
        签("P3-C task_board 非 dict 响应不崩栈",
           not 鉴权 and not 连接失败 and "响应非对象" in 错误, 错误)
    签("P3-C board_cli 失败文案非 dict 归一不崩",
       "HTTP 500" in board_cli._读失败文案("看板服务", 500, ["x"]),
       board_cli._读失败文案("看板服务", 500, ["x"]))
    临时 = Path(tempfile.mkdtemp(prefix="ledger_nondict_")) / "c.json"
    临时.write_text("[]", encoding="utf-8")
    原缓存 = case_ledger.缓存
    case_ledger.缓存 = 临时
    try:
        签("P3-C case_ledger 非 dict 缓存不崩", case_ledger._读缓存行() == [])
    finally:
        case_ledger.缓存 = 原缓存

    with 环境(体='{"台账": [{"号": "9", "用例名": "原子写"}]}'):
        临时 = Path(tempfile.mkdtemp(prefix="ledger_atomic_")) / "c.json"
        原缓存 = case_ledger.缓存
        case_ledger.缓存 = 临时
        try:
            行们, 可用, _原因, _鉴权 = case_ledger.台账取回()
            签("P3-H case_ledger 缓存原子写可读回",
               可用 and len(行们) == 1
               and json.loads(临时.read_text(encoding="utf-8"))["台账"][0]["号"] == "9",
               临时.read_text(encoding="utf-8")[:120])
        finally:
            case_ledger.缓存 = 原缓存

    签("P3-J 403 文案点名「权限不足」",
       "权限不足" in intent.鉴权失败文案("看板服务", 403),
       intent.鉴权失败文案("看板服务", 403))


def 测_wt_HTTP错误不误报() -> None:
    print("[11] wt.py 非 401 HTTP 错误不落「台账无任务+幽灵分支」")
    原运行, 原服务调, 原输出 = wt.运行, wt.服务调, wt.输出

    class 假结果:
        returncode = 0
        stdout = b""
        stderr = b""

    try:
        for 码, 标记 in ((500, "HTTP 500"), (401, "鉴权失败（401）")):
            wt.运行 = lambda *_a, **_k: 假结果()
            wt.输出 = lambda *_a, **_k: ""
            wt.服务调 = lambda *_a, **_k: {"HTTP错误": 码, "错误": "selftest"}
            输出 = io.StringIO()
            with contextlib.redirect_stdout(输出):
                返回 = wt.建树("1485")
            文本 = 输出.getvalue()
            签(f"wt 建树 {码} → {标记} 且不落台账无任务",
               返回 == 1 and 标记 in 文本 and "台账无任务" not in 文本, 文本[-200:])
    finally:
        wt.运行, wt.服务调, wt.输出 = 原运行, 原服务调, 原输出


def 测_请求审计() -> None:
    print("[12] 请求级审计（令牌态全带 Authorization / 外链零泄漏 / 38+）")
    带令牌 = [r for r in 假.请求日志 if r["有令牌"]]
    无头 = [r for r in 带令牌 if r["期望鉴权"] and not r["auth"]]
    设计无头 = [r for r in 带令牌 if not r["期望鉴权"]]
    泄漏 = [r for r in 设计无头 if r["auth"]]
    签(f"令牌态请求 >= 38（实 {len(带令牌)}）", len(带令牌) >= 38, f"实 {len(带令牌)}")
    签(f"应鉴权请求 auth=False = 0（实 {len(无头)}；按设计无头 {len(设计无头)} 条）",
       not 无头, str(无头[:3]))
    签("token_leak=False（外链/跨源请求无 Authorization）", not 泄漏, str(泄漏[:3]))


def main() -> int:
    print("== 看板只读 GET 令牌/401 分流自测（任务 1485）==")
    假.请求日志.clear()
    假.免鉴权URL.clear()
    测_令牌头()
    测_GET带令牌()
    测_无令牌裸请求()
    测_未认证分流()
    测_无权限分流()
    测_断网态()
    测_用例台账未认证()
    测_写入口未认证()
    测_全入口未认证矩阵()
    测_case_activity分列()
    测_check硬红()
    测_py38兼容全扫描()
    测_响应畸形硬红()
    测_覆盖响应畸形不落盘()
    测_令牌缓存()
    测_诊断与来源()
    测_wt_HTTP错误不误报()
    测_重定向不转发令牌()
    测_同源三元组()
    测_请求审计()
    print(f"\n[自测] 通过 {通过}·失败 {len(失败)}")
    for 名 in 失败:
        print(f"  [✗] {名}")
    return 0 if not 失败 else 1


if __name__ == "__main__":
    sys.exit(main())
