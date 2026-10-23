# -*- coding: utf-8 -*-
"""standup.py——开机一条命令（241·2026-10-07·诊断轮问题⑤「AI 开机成本高」落地）。

一键开工简报：fetch+落后提示 → 看板就绪摘要 → 交接本机节最近条目 → 教训高权重标题
→ 待裁决件数。384 起 交接/教训读看板服务端（两文档已退位删除），服务不可达降级读
board_cache.json 最后缓存（无网也能开工简报）。

用法：python scripts/standup.py [--机 深度机]   # --机 缺省自动=hostname·平台-架构·指纹6（508·hostname 别名猜测表已废·408 禁猜测一脉）
"""
from __future__ import annotations

import argparse
import json
import socket
import subprocess
import sys
import urllib.parse
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import atomic_json

本树根 = Path(__file__).resolve().parent.parent


def 看板地址() -> str:
    """384：看板服务地址（环境变量覆盖·缺省 TX_01:8301）。"""
    import os
    return os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301").rstrip("/")


def _落缓存(名: str, 体) -> None:
    """556：写走 atomic_json.合并写（原子·治多写者并发半写损坏）·归纳零复制。"""
    atomic_json.合并写(本树根 / "scripts" / "board_cache.json", {名: 体})


def 跑(命令: list[str], cwd: Path | None = None) -> str:
    r = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8",
                       errors="replace", cwd=cwd or 本树根)
    return r.stdout + (r.stderr if r.returncode else "")


def 落后提示() -> list[str]:
    行们: list[str] = []
    r = subprocess.run(["git", "fetch", "gitcode"], capture_output=True, text=True, cwd=本树根)
    if r.returncode != 0:
        # 556：重试一次——多会话并发 fetch 偶发 ref 锁冲突（瞬时失败·重试即过）
        r = subprocess.run(["git", "fetch", "gitcode"], capture_output=True, text=True, cwd=本树根)
    if r.returncode != 0:
        return ["⚠️ git fetch 失败（已重试一次·网络/远端异常）——先解决再开工"]
    本地 = 跑(["git", "rev-parse", "refs/heads/develop"]).strip()
    远端 = 跑(["git", "rev-parse", "gitcode/develop"]).strip()
    脏 = 跑(["git", "status", "--porcelain"]).strip()
    if 本地 != 远端:
        数 = len(跑(["git", "log", "--oneline", f"HEAD..gitcode/develop"]).strip().splitlines())
        if 数:    # 509：真落后才报落后——cherry-pick 集成后常见「sha 不同但 0 行落后」，报「落后 0 提交先 merge」是噪音且误导
            行们.append(f"📌 develop 落后远端 {数} 提交——先 git merge --ff-only gitcode/develop 对齐（fetch≠对齐）")
        else:
            行们.append("ℹ develop 与远端内容等价·仅 sha 差异（cherry-pick 改写）——git merge --ff-only gitcode/develop 可对齐指针")
    if 脏:
        行们.append(f"📌 工作树不干净（{len(脏.splitlines())} 项）——涉共享文档的写集请走 worktree")
    if not 行们:
        行们.append("✓ develop 已对齐·工作树干净")
    return 行们


def _读缓存(名: str):
    """384：看板文档 GET 结果的本地最后缓存（board_cli.落缓存 落盘·gitignore）
    ——服务不可达时降级读（无网也能开工简报）。556：读走 atomic_json.读
    （坏缓存返回 None+黄字·不再裸 json.loads 炸读侧）。"""
    return (atomic_json.读(本树根 / "scripts" / "board_cache.json") or {}).get(名)


def hooks自检() -> list[str]:
    """556：git hooks 安装自检（三拦=pre-push 拦直推 develop/commit-msg 拦纯外文
    提交信息/pre-commit 拦改 021——AGENTS §5.0）：core.hooksPath 空=未装·黄字+修复
    命令；路径配置了但目录不存在=坏配置同报（相对路径相对树根解析）。"""
    r = subprocess.run(["git", "config", "--get", "core.hooksPath"],
                       capture_output=True, text=True, cwd=本树根)
    路径 = r.stdout.strip()
    if 路径 and (本树根 / 路径).exists():
        return []
    提示 = "⚠️ git hooks 未安装（三拦全失效：直推 develop/纯外文提交信息/改 021 均不拦）"
    if 路径:
        提示 = f"⚠️ core.hooksPath={路径} 目录不存在（坏配置·hooks 失效）"
    return [提示, "   修复（每机一次性）：git config core.hooksPath scripts/githooks"]


def _鉴权头() -> dict:
    """1425 全站登录墙：看板 GET 有令牌就带 Bearer（intent.鉴权头 统一口径；
    import/读令牌失败=裸请求走降级缓存路径）。"""
    try:
        import intent
        return intent.鉴权头()
    except Exception:
        return {}


def 本机节(机: str, 主机: str = "") -> list[str]:
    """384：交接本机节改读看板服务端（交接.md 已退位删除）·降级读缓存。
    508：机器标识已指纹化（hostname·平台-架构·指纹6），服务端 machine=? 精确
    过滤读不到旧格式（「家机」/裸 hostname）条目——改拉混流后客户端按 hostname
    前缀过滤（新旧机器标识都以 hostname 开头·撞名机旧条目互认属撞名本质·滚动自愈）。"""
    import urllib.request
    前缀 = 主机 or 机.split("·")[0]
    头 = _鉴权头()   # 1425：看板 GET 有令牌就带 Bearer
    try:
        with urllib.request.urlopen(
                urllib.request.Request(f"{看板地址()}/api/handoff?limit=50",
                                       headers=头), timeout=8) as r:
            混流 = json.loads(r.read().decode("utf-8")).get("条目们", [])
        条 = [t for t in 混流 if str(t.get("机器", "")).startswith(前缀)][:2]
        _落缓存("handoff", {"机器": 机, "条目们": 条})
    except Exception:
        缓 = _读缓存("handoff") or {}
        条 = 缓.get("条目们", []) if 缓.get("机器") == 机 else []
        if not 条:
            return [f"（交接：服务不可达且无「{机}」缓存——board_cli 交接 可写·恢复后自愈）"]
    行们 = [f"## 交接·{机}节（最近 {len(条)} 条·服务端）"]
    行们 += [f"- {t['条目']}" for t in 条]
    return 行们


def 教训高权重(上限: int = 10) -> list[str]:
    """384：教训高权重改读看板服务端（教训.md 已退位删除）·降级读缓存。"""
    import urllib.request
    条 = []
    try:
        with urllib.request.urlopen(
                urllib.request.Request(f"{看板地址()}/api/lessons?high_weight=8",
                                       headers=_鉴权头()), timeout=8) as r:
            条 = json.loads(r.read().decode("utf-8")).get("条目们", [])
        _落缓存("lessons", {"条目们": 条})
    except Exception:
        条 = (_读缓存("lessons") or {}).get("条目们", [])
    if not 条:
        return ["## 教训·高权重：服务不可达且无缓存——恢复后自愈"]
    行们 = [f"## 教训·高权重标题（前 {min(上限, len(条))} 条·全文=board_cli 教训看 <id>）"]
    行们 += [f"- {t['标题'][:88]}{'…' if len(t['标题']) > 88 else ''}" for t in 条[:上限]]
    return 行们


def 看板与裁决() -> list[str]:
    行们 = ["## 看板（task_board --ready 摘要）"]
    就绪 = 跑([sys.executable, "scripts/task_board.py", "--ready"])
    行们 += 就绪.strip().splitlines()[:14]
    try:
        件 = json.loads(跑([sys.executable, "scripts/decision_brief.py", "--json"]))
        行们.append(f"## 待裁决 {len(件)} 件（python scripts/decision_brief.py 看一页纸）")
    except Exception:
        行们.append("## 待裁决：decision_brief 输出解析失败（脚本异常？）")
    return 行们


def 看板自动claim() -> list[str]:
    """308j：开工自动登记看板意图（生命周期机械化之一）；371 扩接棒兜底——
    当前分支=任务/<号> 直接 claim；否则树名 wt<号> 且远端 任务/<号> 在飞且
    服务端无该号在做活意图 → 接棒代 claim（integrate 组链切 batch 分支、
    detached HEAD、接棒不跑 create 的树不再漏登记——深度机 348 接棒 40h
    看板零可见实录·本机代登记兜底）。主树/服务不可达均静默零打扰；
    他机活意图在做=撞号保护只提示不抢。"""
    import re
    行们: list[str] = []

    def 收登记输出(文本: str) -> list[str]:
        return [l for l in 文本.strip().splitlines() if l.startswith(("[好]", "[黄]"))]

    意图脚本 = str(本树根 / "scripts" / "intent.py")
    分支 = 跑(["git", "branch", "--show-current"]).strip()
    m = re.fullmatch(r"任务/([0-9]+[a-z]?)", 分支)
    if m:
        return 收登记输出(跑([sys.executable, 意图脚本, "claim",
                              m.group(1), "--备注", "standup 开工自动登记"]))
    tm = re.fullmatch(r"wt([0-9]+[a-z]?)", 本树根.name)
    if not tm:
        return 行们
    号 = tm.group(1)
    r = 跑(["git", "ls-remote", "--heads", "gitcode", f"refs/heads/任务/{号}"])
    if not r.strip():
        return 行们
    try:
        sys.path.insert(0, str(本树根 / "scripts"))
        import intent as 看板
        数据 = 看板.调服务("GET", "/api/board")
    except Exception:
        return 行们                       # 服务不可达：降级不阻断（308j 口径）
    活主们 = [i["会话键"] for i in 数据.get("意图们", [])
              if not i.get("失联") and i.get("在做", "") == 号]
    if 活主们:
        return [f"[看板] #{号} 已由 {'、'.join(活主们)} 声明在做——本树接棒不代登记（撞号保护）"]
    return 收登记输出(跑([sys.executable, 意图脚本, "claim", 号,
                          "--备注", f"standup 接棒自动登记（树 {本树根.name}·远端 任务/{号} 在飞）"]))


def 主流程() -> int:
    解析 = argparse.ArgumentParser(description="开机一条命令（241）")
    解析.add_argument("--机", default=None, help="本机名（缺省自动=hostname·平台-架构·指纹6）")
    参数 = 解析.parse_args()
    主机 = socket.gethostname()
    # 508：机器标识统一走 intent 权威实现（hostname·平台-架构·指纹6）——
    # hostname 前缀别名猜测表废除（408 用户令「机器身份禁 hostname 猜测」一脉；
    # hostname 已撞名后猜测必错标：深度机 ChenChao-PC 曾被错标「家机」）。
    sys.path.insert(0, str(本树根 / "scripts"))
    import intent
    机 = 参数.机 or intent.机器名()
    输出 = ["═" * 46, f" 开工简报·{机}·{主机}", "═" * 46]
    输出 += 落后提示()
    输出 += hooks自检()
    输出 += 看板自动claim()
    输出 += 看板与裁决()
    输出 += 本机节(机, 主机)
    输出 += 教训高权重()
    print("\n".join(输出))
    return 0


if __name__ == "__main__":
    sys.exit(主流程())
