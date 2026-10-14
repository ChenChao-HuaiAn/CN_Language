#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""intent.py —— 会话意图登记客户端（308a·board_service 配套 CLI）。

治 285 撞车实录的「意图不可见」：分支存在=认领只回答"号被占了"，不回答"谁、
哪个会话、正在做还是仅仅计划做"。本 CLI 把各机 AI 会话的在做/计划任务号登记到
board_service（TX_01:8301），网页看板实时可见+同号双声明冲突高亮——他机开工前
一眼看到"这个号已被深度机某会话在做"，打架死在源头。

用法：
  python scripts/intent.py claim 308a --planned 110,276 --备注 "看板服务·先做服务端"
  python scripts/intent.py refresh        # 心跳续约（重发最近一次登记内容）
  python scripts/intent.py release        # 会话收工注销
  python scripts/intent.py show           # 查看全服务端意图+冲突+在飞分支

降级原则（照 queue_service 不可达回退旧路径精神）：服务不可达时 claim/release/
refresh 黄字提示后 exit 0（不阻断开发工作——分支存在=认领仍是有效兜底）；
show 无法取数 exit 1。令牌统一走 读令牌()：本树 scripts/queue_client.json →
主树副本（git-common-dir 反推）→ 环境变量 CN_BOARD_TOKEN/CN_QUEUE_TOKEN
（556·治令牌轮换后旧 worktree 静默 401）；URL 可用环境变量 CN_BOARD_URL 覆盖。
会话 id 按 worktree 粒度（308i：仓库根目录名·持久于 ~/.cache/cn_board_session_id_<树名>；
同机多 worktree 并行互不覆盖，CN_BOARD_SESSION 环境变量可显式覆盖·特殊场景用）；
机器标识取 CN_MACHINE_NAME 环境变量（唯一覆盖口·原样生效）或自动合成本机标识：
hostname·平台-架构·机器指纹6（508·用户令：hostname 已撞名——家机/深度机同为
ChenChao-PC，由运行时探测的机器指纹区分，零配置·加新机自动唯一；494「显示统一
按机器用户名」精神保留，hostname 仍在首位）。
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import threading
import urllib.error
import urllib.request
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
仓库根 = 脚本目录.parent
缓存目录 = Path.home() / ".cache"
# 308i：会话按 worktree 粒度（仓库根目录名即会话名）——治全机单文件多会话互覆
# （同机多 worktree 并行是本项目常态·一会话一树纪律下 worktree 粒度=会话粒度；
#   同树多会话等特殊场景用 CN_BOARD_SESSION 环境变量显式命名）
会话键 = os.environ.get("CN_BOARD_SESSION", "").strip() or 仓库根.name
会话文件 = 缓存目录 / f"cn_board_session_id_{会话键}"
登记缓存 = 缓存目录 / f"cn_board_intent_last_{会话键}.json"
默认地址 = "http://124.222.106.84:8301"


def 服务地址() -> str:
    import os
    return os.environ.get("CN_BOARD_URL", 默认地址).rstrip("/")


def 主树脚本目录() -> Path | None:
    """556：worktree 反推主树 scripts 目录（git-common-dir）——本树即主树时返回自身。"""
    try:
        r = subprocess.run(["git", "rev-parse", "--git-common-dir"], cwd=脚本目录,
                           capture_output=True, text=True, timeout=5)
        if r.returncode != 0:
            return None
        公共 = Path(r.stdout.strip())
        if not 公共.is_absolute():
            公共 = (脚本目录 / 公共).resolve()
        return 公共.parent / "scripts"
    except (OSError, ValueError, subprocess.SubprocessError):
        return None


def 读令牌() -> str:
    """556：令牌统一兜底链=本树 scripts/queue_client.json → 主树副本（git-common-dir
    反推·治令牌轮换后旧 worktree 静默 401）→ 环境变量 CN_BOARD_TOKEN/CN_QUEUE_TOKEN。
    全部落空返回空串——调用方「有令牌就带」语义不变。看板系读取点
    （board_cli/wt/integrate/migrate_board_docs）统一走此实现·归纳零复制。"""
    主 = 主树脚本目录()
    候选们 = [脚本目录 / "queue_client.json"]
    if 主 and 主 != 脚本目录:
        候选们.append(主 / "queue_client.json")
    for 候选 in 候选们:
        try:
            t = json.loads(候选.read_text(encoding="utf-8")).get("令牌", "")
            if t:
                return str(t)
        except (OSError, ValueError):
            continue
    for 键 in ("CN_BOARD_TOKEN", "CN_QUEUE_TOKEN"):
        t = os.environ.get(键, "").strip()
        if t:
            return t
    return ""


def 会话id() -> str:
    if not 会话文件.exists():
        会话文件.parent.mkdir(parents=True, exist_ok=True)
        会话文件.write_text(会话键, encoding="utf-8")   # worktree 名即 id·看板行「机器-树名」直读
    return 会话文件.read_text(encoding="utf-8").strip() or 会话键


def 机器指纹() -> str:
    """运行时探测本机唯一指纹（508·零配置·加新机自动唯一·重装系统才会变）：
    win=注册表 MachineGuid·linux=/etc/machine-id（dbus 路径兜底）·其余=MAC
    （uuid.getnode）。全不可得退 hostname+平台串。返回 sha256 前 6 位十六进制
    ——hostname 撞名（家机/深度机同 ChenChao-PC）后的最后一道区分。"""
    import hashlib
    import uuid
    原值 = ""
    try:
        if sys.platform.startswith("win"):
            import winreg
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                                r"SOFTWARE\Microsoft\Cryptography") as 键:
                原值 = str(winreg.QueryValueEx(键, "MachineGuid")[0])
        else:
            for 路径 in ("/etc/machine-id", "/var/lib/dbus/machine-id"):
                try:
                    原值 = Path(路径).read_text(encoding="ascii", errors="ignore").strip()
                    if 原值:
                        break
                except OSError:
                    continue
    except OSError:
        原值 = ""
    if not 原值:
        try:
            原值 = str(uuid.getnode())    # MAC 整数（拿不到网卡时 Python 自回退随机数·仍稳定于进程生命周期）
        except Exception:
            原值 = f"{socket.gethostname()}:{sys.platform}"
    return hashlib.sha256(原值.encode("utf-8")).hexdigest()[:6]


def 平台段() -> str:
    """平台-架构短标签（508·看板一眼分辨 win/linux/arm64）。"""
    import platform
    系统 = {"win32": "win", "linux": "linux", "darwin": "mac"}.get(
        sys.platform, (sys.platform or "unknown").split(".")[0])
    架构原 = (platform.machine() or "").upper()
    架构 = {"AMD64": "x64", "X86_64": "x64", "AARCH64": "arm64",
            "ARM64": "arm64"}.get(架构原, 架构原.lower() or "unknown")
    return f"{系统}-{架构}"


def 机器名() -> str:
    import os
    import socket
    # 508·用户令（2026-10-12）：hostname 已撞名（家机/深度机同为 ChenChao-PC，
    # 看板在飞分组混机）——机器标识=hostname·平台-架构·机器指纹6。指纹=运行时
    # 探测 machine-id/MachineGuid，零配置·加新机自动唯一。494「显示统一按机器
    # 用户名」精神保留：hostname 仍在首位，后缀只承担撞名区分。
    # CN_MACHINE_NAME 环境变量=唯一覆盖口（原样生效·特殊场景手动指定）。
    # 分隔符用「·」不与「-」混——服务端 494 归属串拆分正则 ^(.+)-wt(\d+)$ 以「-」定位树名段。
    环境 = os.environ.get("CN_MACHINE_NAME", "").strip()
    if 环境:
        return 环境
    return f"{socket.gethostname()}·{平台段()}·{机器指纹()}"


def 调服务(方法: str, 路径: str, 体=None, 超时=8):
    请求 = urllib.request.Request(服务地址() + 路径, method=方法,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
    if 读令牌():
        请求.add_header("Authorization", f"Bearer {读令牌()}")
    with urllib.request.urlopen(请求, timeout=超时) as r:
        return json.loads(r.read().decode("utf-8"))


def 收视野号集() -> list:
    """本机视野内全部在飞行号（316·与 wt.py 收集视野号集 同构：主表+分支名+
    远端任务分支树内 021 行号·654 起 ✅ 完成号剔除）——claim/show 时随板上报，
    双机视野齐=号占冲突自动亮（✅ 历史号不亮·服务端比对侧同滤）。"""
    try:
        import wt as _wt
        return sorted(_wt.收集视野号集())
    except Exception:
        return []


def 核对号(号: str):
    """382：claim 前查服务端台账（存在=黄字预警·✅=已收口勿认领）——
    返回 None=服务不可达（跳过）·True=台账无此号（干净）·False=有占用预警。"""
    try:
        r = 调服务("GET", f"/api/task/{号}")
        t = (r or {}).get("任务")
        if t is None:
            return True
        if t.get("状态") == "✅":
            print(f"[黄] ⚠ #{号} 已收口完成（✅ {t.get('收口sha', '')}）——勿再认领，重开请立新号")
        else:
            print(f"[黄] ⚠ 台账已有 #{号}（状态 {t.get('状态')}·{t.get('标题', '')[:40]}）"
                  f"——接棒用 wt.py create {号}（认领态以台账为准）")
        return False
    except (urllib.error.URLError, OSError, ValueError):
        return None


def 上报视野() -> None:
    """316：把本机视野号集上报服务端（only_view 只记账不发号）——尽力而为静默。"""
    try:
        调服务("POST", "/api/claim_number",
               {"机器": 机器名(), "上报者": f"{机器名()}-{仓库根.name}",
                "视野号们": 收视野号集(), "only_view": True}, 超时=10)
    except (urllib.error.URLError, OSError, ValueError):
        pass


def 上报在飞分支() -> None:
    """把本机看到的远端在飞任务分支上报给看板（尽力而为·失败静默——
    服务端不持 git 凭据不依赖外网，三机任一活着看板即有在飞数据）。
    308e：逐支带「已并入」判定（头提交是 develop 祖先=任务已收口·僵尸分支面；
    本地 develop 引用过旧只可能误判「未并入」=保守方向安全）；并上报任务字典
    （号→标题/状态·021 主表+归档全量·看板分支行直接显示任务内容）。"""
    try:
        r = subprocess.run(["git", "ls-remote", "--heads", "gitcode"],
                           capture_output=True, text=True, cwd=仓库根, timeout=30)
        分支们 = []
        for 行 in r.stdout.splitlines():
            m = re.match(r"^([0-9a-f]+)\s+refs/heads/(任务/.+)$", 行.strip())
            if m:
                sha = m.group(1)[:10]
                并入 = subprocess.run(
                    ["git", "merge-base", "--is-ancestor", sha, "gitcode/develop"],
                    cwd=仓库根, timeout=15).returncode == 0
                分支们.append({"分支": m.group(2), "提交": sha, "时刻": "",
                               "已并入": 并入})
        if 分支们:
            调服务("POST", "/api/report_flights",
                   {"上报者": 机器名(), "分支们": 分支们}, 超时=5)
    except (OSError, ValueError, subprocess.SubprocessError):
        pass    # 尽力而为：上报失败不影响登记主流程
    上报视野()    # 316：视野号集随每次 claim/show 上报（双机视野齐=号占冲突亮）
    # 382：021 快照上报通道已退役——任务台账唯一权威=服务端任务表（/api/task_create），
    # 客户端不再解析本地 021 上报（快照会过期、任务表永不过期）。


def 落登记(在做: str, 计划: str, 备注: str) -> int:
    体 = {"机器": 机器名(), "对话id": 会话id(),
          "在做": 在做, "计划": 计划, "备注": 备注}
    try:
        调服务("POST", "/api/intent", 体)
        上报在飞分支()
        登记缓存.write_text(json.dumps(体, ensure_ascii=False), encoding="utf-8")
    except (urllib.error.URLError, OSError, ValueError) as e:
        print(f"[黄] 看板服务不可达（{服务地址()}）：{e}——意图未上板，"
              f"分支存在=认领仍是有效兜底（降级不阻断）")
        return 0
    print(f"[好] 意图已上板：{机器名()}-{会话id()} 在做#{在做 or '—'}"
          f" 计划[{计划 or '—'}]（路过即心跳·15 分钟无心跳且无分支推进=黄标·行不删）")
    return 0


def 已收口(号: str) -> bool:
    """316：在做任务是否已收口——382 起以服务端台账为准（GET /api/task/<号> 状态 ✅）。
    384：本地 021 降级段删除（021 文件已终局删除·服务不可达返回 False=不下板，
    下次路过再查——漏删后果仅意图行滞留，492 起行永不自动清·人工 release 兜底）。"""
    if not 号:
        return False
    try:
        t = (调服务("GET", f"/api/task/{号}") or {}).get("任务")
        if t is not None:
            return t.get("状态") == "✅"
    except (urllib.error.URLError, OSError, ValueError):
        pass
    return False


def 路过续约() -> None:
    """有本会话历史登记则后台线程续约（308k·路过即心跳——task_board 每次被跑时
    挂此函数：活会话自动续约在线·真停工/被杀不跑脚本→自然失联灰显）。静默失败。"""
    if not 登记缓存.exists():
        return
    def _发():
        try:
            旧体 = json.loads(登记缓存.read_text(encoding="utf-8"))
            if 已收口(str(旧体.get("在做", ""))):
                登记缓存.unlink()    # 316：任务已收口→下板（防复活·窄通道收口兜底）
                return
            旧体["机器"] = 机器名()   # 508：机器名升级/覆盖口变更后首次路过即迁新标识（旧行断心跳自然黄标消失）
            调服务("POST", "/api/intent", 旧体, 超时=4)
        except (urllib.error.URLError, OSError, ValueError):
            try:    # 492：仅失败才重试一次（原无条件二次重发=每次路过双倍 POST 冗余）
                旧体 = json.loads(登记缓存.read_text(encoding="utf-8"))
                旧体["机器"] = 机器名()
                调服务("POST", "/api/intent", 旧体, 超时=4)
            except (urllib.error.URLError, OSError, ValueError):
                pass
    threading.Thread(target=_发, daemon=True).start()


def 挂载路过续约() -> None:
    """492：给看板系/验证系脚本一行挂载「路过即心跳」（daemon 线程·挂载方再套
    try/except 静默）——扩源治子Agent 编排长跑期无看板触点→心跳断→会话误判失联。
    挂载点全景：task_board / gate_quick / wt / board_cli / run_e2e（归纳零复制）。"""
    threading.Thread(target=路过续约, daemon=True).start()


def 命令认领(参数) -> int:
    核对号(参数.任务号)    # 382：台账核对预警链（✅/已占用黄字·不阻断意图登记）
    return 落登记(参数.任务号, 参数.计划, 参数.备注)


def 命令续约(_参数) -> int:
    if not 登记缓存.exists():
        print("[黄] 无历史登记可续约——先 claim")
        return 0
    旧 = json.loads(登记缓存.read_text(encoding="utf-8"))
    return 落登记(旧.get("在做", ""), 旧.get("计划", ""), 旧.get("备注", ""))


def 命令注销(_参数) -> int:
    try:
        调服务("POST", "/api/intent_release", {"机器": 机器名(), "对话id": 会话id()})
    except (urllib.error.URLError, OSError, ValueError) as e:
        try:
            登记缓存.unlink()    # 556：断网分支同样清缓存（原漏——文案称「已清」实未清，
        except OSError:          #   下次路过续约复活已注销意图=注销失效）
            pass
        print(f"[黄] 看板服务不可达：{e}——本地缓存已清，服务端行等心跳超时自然失联灰显")
        return 0
    if 登记缓存.exists():
        登记缓存.unlink()    # 316：清缓存防路过续约复活已注销意图
    print(f"[好] 已注销：{机器名()}-{会话id()}")
    return 0


def 命令查看(_参数) -> int:
    try:
        数据 = 调服务("GET", "/api/board")
    except (urllib.error.URLError, OSError, ValueError) as e:
        print(f"[失败] 看板服务不可达（{服务地址()}）：{e}")
        return 1
    print(f"会话意图（失联阈值 {数据.get('失联秒')}s·时刻 {数据.get('时刻')}）：")
    for i in 数据.get("意图们", []):
        状态 = "〔失联〕" if i["失联"] else ""
        print(f"  {i['机器']}-{i['对话id']} 在做#{i['在做'] or '—'} "
              f"计划[{i['计划'] or '—'}] 心跳{i['心跳时刻']}{状态}"
              f"{' ' + i['备注'] if i['备注'] else ''}")
    冲突们 = 数据.get("冲突们", [])
    print(f"冲突预警：{'无' if not 冲突们 else ''}")
    for c in 冲突们:
        print(f"  ⚠ #{c['号']} 被 {' 与 '.join(c['会话们'])} 同时声明")
    在飞 = 数据.get("在飞分支们", [])
    print(f"在飞分支（{len(在飞)}）：")
    for f in 在飞:
        # 371：归属显示改按 在做主们（活意图在做）——原样打印「上报者」=最后上报
        # 视野的机器，随上报者整体翻转误导归属（371 呈报实证）；上报者降级为
        # 无归属时的溯源小注。
        主们 = f.get("在做主们") or []
        归属 = "、".join(主们) if 主们 else f"未登记·最后上报:{f.get('上报者', '?')}"
        print(f"  {f['分支']} {f['提交']} {f['时刻']} {归属}")
    return 0


def main() -> int:
    解析器 = argparse.ArgumentParser(description="会话意图登记客户端（308a·看板配套）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p领 = 子.add_parser("claim", help="登记/更新本会话意图（即心跳）")
    p领.add_argument("任务号", help="正在做的任务号（如 308a）")
    p领.add_argument("--planned", dest="计划", default="",
                     help="后续轮次计划做的任务号（逗号分隔·打架预警面）")
    p领.add_argument("--备注", default="", help="一句话说明（≤200 字）")
    子.add_parser("refresh", help="心跳续约（重发最近登记）")
    子.add_parser("release", help="会话收工注销")
    子.add_parser("show", help="查看全服务端意图+冲突+在飞分支")
    参数 = 解析器.parse_args()
    return {"claim": 命令认领, "refresh": 命令续约,
            "release": 命令注销, "show": 命令查看}[参数.命令](参数)


if __name__ == "__main__":
    sys.exit(main())
