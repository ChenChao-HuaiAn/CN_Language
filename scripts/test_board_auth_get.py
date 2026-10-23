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
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))

import board_cli          # noqa: E402
import branch_cleanup     # noqa: E402
import check_spec_coverage  # noqa: E402
import decision_brief     # noqa: E402
import infra_healthcheck  # noqa: E402
import integrate          # noqa: E402
import intent             # noqa: E402
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
        self.记录: list[urllib.request.Request] = []

    def __call__(self, 请求, timeout=None):
        self.记录.append(请求)
        if self.模式 == "http错误":
            raise urllib.error.HTTPError(
                请求.full_url, self.码, "Unauthorized", {},
                io.BytesIO('{"错误":"未登录或令牌不符（GET 亦须登录·1425 登录墙）"}'
                           .encode("utf-8")))
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
    假.模式, 假.体, 假.记录, 假.码 = 模式, 体, [], 码
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
        数据, 错误 = task_board.服务调("/api/tasks")
        签("task_board.服务调", 假.头().get("Authorization") == "Bearer selftest-token"
           and 数据 == {"任务们": []} and not 错误, f"{数据}/{错误}")

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


def 测_无令牌裸请求() -> None:
    print("[3] 令牌落空 → 各客户端不带 Authorization（行为不变）")
    with 环境(有令牌=False, 体='{"任务们": []}'):
        task_board.服务调("/api/tasks")
        签("task_board 无令牌裸请求", "Authorization" not in 假.头(), str(假.头()))
    with 环境(有令牌=False, 体='{"任务们": []}'):
        branch_cleanup.台账在飞分支集()
        签("branch_cleanup 无令牌裸请求", "Authorization" not in 假.头(), str(假.头()))


def 测_401分流() -> None:
    print("[4] 401 鉴权失败与「服务不可达」文案分流")
    with 环境(模式="http错误"):
        数据, 错误 = task_board.服务调("/api/tasks")
        签("task_board 401 → 鉴权失败（非不可达）",
           数据 is None and "鉴权失败（401）" in 错误 and "不可达" not in 错误, 错误)

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


def main() -> int:
    print("== 看板只读 GET 令牌/401 分流自测（任务 1485）==")
    测_令牌头()
    测_GET带令牌()
    测_无令牌裸请求()
    测_401分流()
    print(f"\n[自测] 通过 {通过}·失败 {len(失败)}")
    for 名 in 失败:
        print(f"  [✗] {名}")
    return 0 if not 失败 else 1


if __name__ == "__main__":
    sys.exit(main())
