#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""wt.py —— 本机多开 worktree 任务池管理（449-a·AGENTS.md §7）。

同机并行开发互踩的根因=多会话共享一个工作树与 target/（429-a/431-a 实录：
checkout 带走未提交内容 / E2E 中途 expected 消失 / cn.exe 占用 LNK1104）。
本脚本把「每任务一个 worktree」制度化：

  create <任务号>        建树+任务分支 任务/<任务号>（021 无此行且远端无此分支拒建；远端分支已存在=
                         接棒同分支续做——机器停摆他机无缝接力·196 轮立规·不重立行；新开则基于 develop；
                         跳号基准=主表+归档+远端在飞三源（230 修法①·在飞行号住未合分支树）；
                         gtest 两级深度校验→Ninja+sccache 开发树配置）
  list                   列出全部 worktree（分支/干净度/target 占用）
  remove <任务号>        删树（分支保留；--delete-branch 仅当已并入 develop 才删分支）
  cache [stats|start|stop|clear]   sccache 缓存服务快捷操作

命名沿革：196 轮前为 任务/<机>-<轮>-<标识>（轮次号系 AI 会话编号·用户不可读）——
分支生命周期自 194 轮起=任务生命周期：任务号即分支名，收工集成删分支，重开重建。

实测口径（449-a·win/MSVC）：
  - VS 生成器（vcxproj ClCompile 原生任务）忽略 CMAKE_CXX_COMPILER_LAUNCHER——sccache 只能走 Ninja；
  - Ninja+MSVC 必须 /Z7 替代 /Zi（默认 /Zi 多 cl 并发写同一 PDB=C1041，加 /FS 也拦不住）；
  - 勿用 -DCMAKE_CXX_FLAGS 覆盖（会顶掉默认 /EHsc→C4530 被 /WX 拦成 C2220，且 CMakeCache 残留）；
  - 计时：VS --parallel 冷全量 127s｜Ninja+sccache 冷 23s｜同树删树重建 10.7s｜跨树首建 ~21s（48% 命中：
    共享路径的第三方〔gtest/运行时〕命中，树内绝对路径的项目源码 miss）。
"""
from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import urllib.error
import urllib.request
from datetime import datetime
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent


def 扫行号(目录: Path, 模式: str) -> set[str]:
    """扫目录下匹配模式的 021 账本，解析行首任务号集合（主表/归档/接棒树三处共用）。"""
    号集: set[str] = set()
    for 账本 in sorted(目录.glob(模式)):
        for 行 in 账本.read_text(encoding="utf-8").splitlines():
            m = re.match(r"^\|\s*(\d+[a-z]?)\s*\|", 行)
            if m:
                号集.add(m.group(1))
    return 号集


def 读021行号() -> set[str] | None:
    """解析 021 总账主表任务号集合（纯任务号分支名的行号校验源·196 轮立规）。

    返回 None=找不到 021 文件（放行建树·树内脚本自举场景不硬拦）。
    230 修法①注：本函数只看本地工作树的主表+归档——226 立规「立项行随任务分支走」后
    在飞行号住在未合分支树，主表 max 偏小；跳号/接棒判定须叠加 读远端在飞行号()。
    """
    if not sorted((本树根 / "plans").glob("021*.md")):
        return None
    return 扫行号(本树根 / "plans", "021*.md")


def 读归档行号() -> set[str]:
    """解析 021 已归档任务号集合（收口归档制·226 轮立法）——仅用于跳号基准与禁建校验，
    不参与「021 无此行拒建」存在性校验（归档号不在主表·重立须用新号）。
    """
    return 扫行号(本树根 / "项目记忆" / "归档", "plans021-已完成任务归档-*.md")


def 读远端在飞行号() -> set[str]:
    """fetch 后扫远端 任务/<号>/batch/<号> 纯数字分支号集合（230 修法①·2026-10-07）。

    在飞行号只存在于未合分支树（226 立规），本地主表+归档看不见——不并入基准则
    轻则误拦真序号（229 轮 create 被误判跳号·须主表临时登记在飞行过检再还原）、
    重则两会话先后取同一号=重号撞车。与 task_board --check 修法①（99f21e53）同口径。
    """
    号集: set[str] = set()
    for 引用 in 输出(["git", "for-each-ref", "--format=%(refname:short)",
                      "refs/remotes/gitcode"]).splitlines():
        m = re.fullmatch(r"gitcode/(?:任务|batch)/(\d+)", 引用.strip())
        if m:
            号集.add(m.group(1))
    return 号集


def 读本地在飞行号() -> set[str]:
    """扫本地 refs/heads 任务/<号>/batch/<号> 纯数字分支号（269 补·230 修法①同族第四源）。

    本地并行会话 create 后未 push 的在飞行（268 官网案实录：wt268 worktree+分支已立
    未推，三源只见 max=267 → create 269 被误拦跳号）。已集成分支号必已入主表/归档
    （集成即删分支·立项行随批收口），并集无增量——本源只增不漏。
    """
    号集: set[str] = set()
    for 引用 in 输出(["git", "for-each-ref", "--format=%(refname:short)",
                      "refs/heads"]).splitlines():
        m = re.fullmatch(r"(?:任务|batch)/(\d+)", 引用.strip())
        if m:
            号集.add(m.group(1))
    return 号集


def 收集视野号集() -> set[str]:
    """本机视野内全部在飞行号（308j 乙+）——主表+归档+远端/本地分支名四源并集，
    再叠加**远端任务分支树内 021 行号**（309 双占型：号住在 任务/308 分支树内·
    分支名正则看不见；每支一次 git show·在飞 <25 支秒级。树内立项行随分支首提交
    push（226 立规）——push 后即入本视野·上报服务端后全机可见）。"""
    号集 = (读021行号() or set()) | 读归档行号() | 读远端在飞行号() | 读本地在飞行号()
    for 引用 in 输出(["git", "for-each-ref", "--format=%(refname:short)",
                      "refs/remotes/gitcode"]).splitlines():
        引用 = 引用.strip()
        if not re.fullmatch(r"gitcode/(?:任务|batch)/[0-9]+[a-z]?", 引用):
            continue
        内容 = 输出(["git", "show", f"{引用}:plans/021-任务进度观察表.md"])
        for 行 in 内容.splitlines():
            m = re.match(r"^\|\s*([0-9]+[a-z]?)\s*\|", 行)
            if m:
                号集.add(m.group(1))
    return 号集


def 板服务地址() -> str:
    return os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301").rstrip("/")


def 机名() -> str:
    return os.environ.get("CN_MACHINE_NAME", "").strip() or socket.gethostname()


def 服务发号(描述: str, 请求号: str = "") -> dict | None:
    """308j 乙+：服务器发号权威。请求号空=要新号（返回含「号」）；非空=核对（返回含
    「已发」True=被占 409·False=放行）。服务不可达返回 None（调用方降级不停摆）。"""
    令牌文 = ""
    try:
        令牌文 = json.loads((本树根 / "scripts" / "queue_client.json")
                            .read_text(encoding="utf-8")).get("令牌", "")
    except (OSError, ValueError):
        pass
    体 = {"机器": 机名(), "上报者": f"{机名()}-{本树根.name}",
          "视野号们": sorted(收集视野号集()), "描述": (描述 or "")[:200]}
    if 请求号:
        体["请求号"] = 请求号
    请求 = urllib.request.Request(板服务地址() + "/api/claim_number", method="POST",
        data=json.dumps(体, ensure_ascii=False).encode("utf-8"))
    if 令牌文:
        请求.add_header("Authorization", f"Bearer {令牌文}")
    try:
        with urllib.request.urlopen(请求, timeout=10) as r:
            return json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return {"HTTP错误": e.code, **json.loads(e.read().decode("utf-8"))}
        except (ValueError, UnicodeDecodeError):
            return {"HTTP错误": e.code}
    except (urllib.error.URLError, OSError, ValueError):
        return None


def 自动claim意图(任务号: str, 描述: str = "") -> None:
    """create 成功后自动登记看板意图（308j 生命周期机械化·开工即上板零自觉依赖；
    intent.py 缺席或服务不可达均静默——降级不阻断）。"""
    intent = 本树根 / "scripts" / "intent.py"
    if not intent.exists():
        return
    try:
        # 308k：会话键=新树名（CN_BOARD_SESSION）——claim 若从别的树发起（常见：
        # create 命令在主树/他树跑），键错记发起树=登记缓存错位→路过续约找不到缓存
        # 失效（实测 313 意图挂 wt308j 键实录）。
        env = dict(os.environ, CN_BOARD_SESSION=f"wt{任务号}")
        备注 = (描述 or "").strip()[:180] or "wt.py create 自动登记"
        r = 运行([sys.executable, str(intent), "claim", 任务号,
                  "--备注", 备注], env=env)
        if "已上板" in (r.stdout or ""):
            print(f"[看板] 意图已自动登记（{任务号}·30min 心跳·收口自动注销·"
                  f"会话中途 refresh 续约·路过即心跳）")
    except OSError:
        pass


def 运行(命令: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(命令, capture_output=True, text=True, encoding="utf-8", errors="replace", **kwargs)


def 输出(命令: list[str]) -> str:
    结果 = subprocess.run(命令, capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=本树根)
    return (结果.stdout or "").strip()


def 主树根() -> Path:
    """共享 .git 所在的主工作树根（worktree 下 git-common-dir 指回主树 .git）。"""
    共同目录 = 输出(["git", "rev-parse", "--git-common-dir"])
    路径 = Path(共同目录) if Path(共同目录).is_absolute() else (本树根 / 共同目录)
    return 路径.resolve().parent


def 找sccache() -> Path | None:
    """探测 sccache：PATH → winget 包目录（新装 PATH 未刷新的会话）。"""
    命中 = shutil.which("sccache")
    if 命中:
        return Path(命中)
    包根 = Path.home() / "AppData/Local/Microsoft/WinGet/Packages"
    if 包根.exists():
        for 候选 in sorted(包根.glob("Mozilla.sccache*/sccache-v*/sccache.exe")):
            return 候选
    return None


def 找vcvars() -> Path | None:
    for 版本 in ("Community", "Professional", "Enterprise", "BuildTools"):
        候选 = Path(f"C:/Program Files/Microsoft Visual Studio/2022/{版本}/VC/Auxiliary/Build/vcvars64.bat")
        if 候选.exists():
            return 候选
    return None


def 找ninja() -> Path | None:
    命中 = shutil.which("ninja")
    if 命中:
        return Path(命中)
    候选 = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/"
                "CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
    return 候选 if 候选.exists() else None


设置脚本模板 = """@echo off
rem wt.py: Ninja+sccache dev-tree setup (ASCII only - cmd batch must not contain CJK)
call "{vcvars}" >nul 2>&1
where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)
cd /d {树}
set SCCACHE_DIR={缓存目录}
set SCCACHE_CACHE_SIZE={缓存上限}
cmake -G Ninja -S . -B target\\build-ninja -DCMAKE_MAKE_PROGRAM="{ninja}" -DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH="{sccache}" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS_DEBUG:STRING="/Z7 /Ob0 /Od /RTC1"
if errorlevel 1 (echo [FAIL] cmake configure & exit /b 1)
echo [OK] ninja tree configured
"""

构建脚本模板 = """@echo off
rem wt.py: Ninja+sccache incremental build (ASCII only)
call "{vcvars}" >nul 2>&1
where cl >nul 2>&1 || (echo [FAIL] cl not found after vcvars & exit /b 1)
cd /d {树}
set SCCACHE_DIR={缓存目录}
set SCCACHE_CACHE_SIZE={缓存上限}
rem 088 root-fix: drop stale .ninja_deps so every build re-scans all headers.
rem MSVC+Ninja records header deps from /showIncludes stdout, which sccache
rem (CMAKE_CXX_COMPILER_LAUNCHER) may swallow -> stale deps -> ODR mixed
rem objects -> silent crash. Deleting .ninja_deps forces a full re-scan
rem (cost absorbed by sccache hit; measured cold full build ~23s).
if exist target\\build-ninja\\.ninja_deps del /q target\\build-ninja\\.ninja_deps
cmake --build target\\build-ninja --parallel
if errorlevel 1 (echo [FAIL] build & exit /b 1)
echo [OK] ninja build done
"""


def 净化行文(文: str) -> str:
    """立行描述去表格破坏符（| 换全角／·断行并空格·限 300 字符防备注膨胀）。"""
    文 = 文.replace("|", "／").replace("\n", " ").strip()
    return 文[:300]


def 立行于树(树路径: Path, 任务号: str, 描述: str, 前置: str, 优先级: str) -> bool:
    """在任务分支 worktree 的 021 主表升序位插立项行并首提交（238 立项命令化）。

    行只落任务分支（226 立规：立项行随任务分支 push·主树零接触）——集成时随批入 develop。
    """
    账本们 = sorted((树路径 / "plans").glob("021*.md"))
    if not 账本们:
        print("[警告] 树内无 021 总账——跳过自动立行（行须手工补）")
        return False
    描述, 前置, 优先级 = 净化行文(描述), 净化行文(前置) or "—", 净化行文(优先级)
    新行 = (f"| {任务号} | {描述} | 🏃 | {前置} | {优先级} "
            f"| 分支=任务/{任务号}（{任务号} 立项建树即认领） |")
    for 账本 in 账本们:
        行们 = 账本.read_text(encoding="utf-8").splitlines(keepends=True)
        本号键 = int(任务号) if 任务号.isdigit() else float("inf")
        升序位 = len(行们)
        for i, 行 in enumerate(行们):
            m = re.match(r"^\|\s*(\d+)\s*\|", 行)
            if m and int(m.group(1)) > 本号键:
                升序位 = i
                break
        行们.insert(升序位, 新行 + "\n")
        账本.write_text("".join(行们), encoding="utf-8")
    提交 = 运行(["git", "-C", str(树路径), "add", str(账本们[0].relative_to(树路径))])
    提交 = 运行(["git", "-C", str(树路径), "commit", "-m",
                 f"{任务号} 立项：{描述}（create --行 自动立项·行随任务分支首提交·226 立规机械化）"])
    if 提交.returncode != 0:
        print(f"[警告] 立项行提交失败：{提交.stderr}——行已写入树，须手工 commit")
        return False
    print(f"[4] 021 立项行已随分支首提交（{任务号} 行·升序位）——集成时随批入 develop")
    return True


def 建树(任务号: str = "", 无ninja: bool = False, 立行: str | None = None,
         前置: str = "—", 优先级: str = "P1", 接管: bool = False,
         不推分支: bool = False) -> int:
    # 308j 乙+：create 不带号=服务器权威发号（台账∪各机视野并集 max+1·原子无撞）；
    # 服务不可达降级四源+树内视野本地取号黄字不停摆——权威可降级，开发永不停。
    if 任务号 and not re.fullmatch(r"\d+[a-z]?", 任务号):
        print(f"[失败] 任务号「{任务号}」不合法——须为 021 总账行号形态（如 087、178a）")
        return 1
    自动发号 = not 任务号
    if 自动发号:
        发号 = 服务发号(立行 or "")
        if 发号 and 发号.get("号"):
            任务号 = str(发号["号"])
            print(f"[发号] 服务器权威发号：{任务号}（台账∪全机视野并集 max+1·原子无撞）")
        else:
            有效 = sorted(int(n) for n in 收集视野号集()
                          if n.isdigit() and n not in {"353", "354"})
            候选 = (有效[-1] + 1) if 有效 else 1
            while str(候选) in {"353", "354"}:
                候选 += 1
            任务号 = str(候选)
            print(f"[黄] 看板服务不可达——离线取号 {任务号}（四源+树内视野本地基准·"
                  f"恢复后建议 intent.py show 对账）")
        print(f"[提示] 后续步骤按任务号 {任务号} 继续")
    # 编号规则（2026-10-05 用户令·021 头部立法同源）：禁用号 353/354 永久拒建+按序取号不跳号
    禁用号 = {"353", "354"}
    if 任务号 in 禁用号:
        print(f"[失败] 任务号 {任务号} 为禁用号（2026-10-05 用户令已改 198/199·永久禁用）——新号取当前最大有效号+1")
        return 1
    # fetch 提前+prune（230 修法②）：接棒判定/跳号基准全用 fetch 后的最新账实——旧序校验在前
    # fetch 在后=校验的是过期账；无 --prune 则已删远端分支的陈旧引用残留（集成即删分支纪律下
    # 收口分支的本地引用滞留）→接棒判定接上远端已不存在的死分支（2026-10-07 实测 branch -r
    # 20+ 支中仅 10 支真实存在）
    运行(["git", "fetch", "--prune", "gitcode"])
    分支 = f"任务/{任务号}"
    远端分支在 = bool(输出(["git", "rev-parse", "--verify", f"refs/remotes/gitcode/{分支}"]))
    号集 = 读021行号()
    自动立项 = False
    接棒 = 远端分支在
    if 接棒:
        # 230 修法②（2026-10-07）：远端分支已存在=号已被认领（跨机重号拦截面）——一律接棒
        # 同分支续做，不新开不重立行；021 主表无此行不再拦（行住分支树·226 立规——
        # create 233 接棒者被「无此行」误拦实录）。--行 在接棒态忽略（重立行=集成时主表重号）。
        if 立行:
            print(f"[提示] 远端 {分支} 已存在（在飞认领）——接棒续做·--行 忽略（行住分支树·重立=重号）")
        # 308a 丙案（285 撞车实录根治）：接棒前强制「对方活跃度」知情——fetch 已最新，
        # 展示远端分支头提交时刻/作者/题；48h 内有提交=他机活跃中，无 --takeover 显式
        # 确认则拒（两机同推一分支=push 竞争互踩）；陈旧分支黄字提示后放行（停摆接力不变）。
        头 = 输出(["git", "log", "-1", "--format=%ci%x09%an%x09%s", f"gitcode/{分支}"])
        头段们 = 头.split("\t", 2) if 头 else []
        头时刻文本 = 头段们[0].strip() if len(头段们) > 0 else ""
        作者 = 头段们[1].strip() if len(头段们) > 1 else "?"
        题 = 头段们[2].strip() if len(头段们) > 2 else "?"
        活跃中 = True
        try:
            头时刻 = datetime.fromisoformat(头时刻文本)
            if 头时刻.tzinfo is None:
                头时刻 = 头时刻.astimezone()
            活跃中 = abs((datetime.now(头时刻.tzinfo) - 头时刻).total_seconds()) < 48 * 3600
        except ValueError:
            pass    # 时刻解析失败=从严按活跃处理
        print(f"[接棒知情] {分支} 头提交：{头时刻文本[:16]} · {作者} · {题[:70]}")
        if 活跃中 and not 接管:
            print(f"[失败] 该分支近 48h 有提交（他机可能活跃中）——盲接棒会与对方 push 竞争"
                  f"（285 撞车实录根治·308a）。确认对方已停工后显式："
                  f"wt.py create {任务号} --takeover")
            return 1
        if 活跃中:
            print("[接管] --takeover 显式确认——同分支续做（请在提交信息/交接注明接管缘由）")
        else:
            print("[提示] 陈旧分支（48h 无提交）——正常接棒（停摆接力语义）")
    elif 号集 is not None and 任务号 not in 号集:
        if 立行:
            # 238 立项命令化（2026-10-07）：021 无此行+create --行 → 建树后自动立行于
            # 任务分支首提交（226 立规「立项行随任务分支 push」机械化——主树零接触）
            自动立项 = True
        else:
            归档提示 = "（此号已收口归档·号全局唯一防复用——重立须用新号=全局最大+1）" \
                if 任务号 in 读归档行号() else ""
            print(f"[失败] 021 总账无任务 {任务号} 且远端无 {分支}{归档提示}"
                  f"——用 create {任务号} --行 \"一句话描述\" "
                  f"一条命令立项建树（238 起），或先在 plans/021 加行再建树")
            return 1
    if 号集 is not None and 任务号.isdigit() and not 接棒 and not 自动发号:
        # 跳号基准（230 修法①+269 补）：主表+归档+远端在飞+本地在飞四源取 max——
        # 在飞行号住未合分支树（226 立规），只看主表+归档则 max 偏小、真序号被误拦
        # （229 轮实录）；268 案补本地源：本地并行会话已建未推分支同样占用号段。
        # 308j：自动发号的号免本校验——服务端按台账∪全机视野（含树内号）取的 max+1，
        # 本地四源基准反而更窄（314 被 308 旧基准误拦实录·信任链=服务器权威）。
        在飞号集 = 读远端在飞行号() | 读本地在飞行号()
        全号集 = 号集 | 读归档行号() | 在飞号集
        其余序列 = [int(n) for n in 全号集
                    if n.isdigit() and n not in 禁用号 and int(n) != int(任务号)]
        if 其余序列 and int(任务号) > max(其余序列) + 1:
            print(f"[失败] 任务号 {任务号} 跳号——除本号外最大有效号 {max(其余序列)}"
                  f"（主表+归档+远端在飞共 {len(全号集)} 号·含未合分支在飞行号），"
                  f"用户令 2026-10-05：按顺序取号（max+1·禁用号 353/354 跳过·历史补记账须用户特批）")
            return 1
    if not 接棒 and 任务号 and not 自动发号:
        # 308j 乙+：带号新建走服务端台账核对（台账已发/他机视野占用→409 拒——
        # 309 双占型在取号瞬间死掉；服务不可达黄字跳过·本地四源校验已兜底）。
        # 自动发号免核对——号是刚从这台账里领的，再核对=自己拦自己（实测实录）。
        核对 = 服务发号(立行 or "", 请求号=任务号)
        if 核对 is None:
            print("[黄] 看板服务不可达——跳过服务端台账核对（本地四源校验已过·"
                  "撞号风险自担）")
        elif 核对.get("已发"):
            print(f"[失败] {核对.get('错误', f'服务端台账此号已发出')}（乙+ 发号权威）")
            return 1
    树路径 = 主树根().parent / f"wt{任务号}"
    if 树路径.exists():
        print(f"[失败] {树路径} 已存在——同名 worktree 或残留，先 wt.py remove {任务号}")
        return 1
    if 输出(["git", "rev-parse", "--verify", f"refs/heads/{分支}"]):
        print(f"[失败] 本地分支 {分支} 已存在——若为接棒残留，先 git branch -D {分支}（远端为准）再建树")
        return 1
    if 接棒:
        基准, 模式 = f"gitcode/{分支}", "接棒（远端分支已存在·同分支续做——机器停摆他机无缝接力）"
    else:
        基准, 模式 = "gitcode/develop", "新开（基于 develop）"
    print(f"[1] git worktree add {树路径.name} -b {分支} {基准}（{模式}）")
    结果 = 运行(["git", "worktree", "add", str(树路径), "-b", 分支, 基准])
    if 结果.returncode != 0:
        print(f"[失败] {结果.stderr}")
        return 1

    gtest = 树路径 / "../../third-party/unittest/googletest/src/gtest-all.cc"
    if gtest.exists():
        print("[2] gtest 两级深度校验通过（../../third-party 可达，与主树共用同一份）")
    else:
        print("[警告] gtest 相对路径不可达（构建将回退 target/build/_deps，需先前拉取过）")

    sccache = 找sccache()
    if 无ninja or sccache is None:
        原因 = "--no-ninja 指定" if 无ninja else "sccache 未安装（winget install Mozilla.sccache）"
        print(f"[3] 跳过 Ninja 开发树配置（{原因}）——门禁用 VS 构建（build.ps1）不受影响")
    elif platform.system() != "Windows":
        ninja = shutil.which("ninja")
        if ninja is None:
            print("[3] 跳过：linux 侧未装 ninja（apt install ninja-build 后重跑 create 可补）")
        else:
            配置 = subprocess.run(["cmake", "-G", "Ninja", "-S", ".", "-B", "target/build-ninja",
                                   f"-DCMAKE_CXX_COMPILER_LAUNCHER:FILEPATH={sccache}",
                                   "-DCMAKE_BUILD_TYPE=Debug"], cwd=树路径)
            print(f"[3] Ninja 开发树配置{'完成' if 配置.returncode == 0 else '失败'}（build: "
                  f"cmake --build target/build-ninja --parallel）")
    else:
        vcvars, ninja = 找vcvars(), 找ninja()
        if vcvars is None or ninja is None:
            print("[3] 跳过：未找到 vcvars64.bat 或 ninja.exe（VS2022 安装不完整）")
        else:
            缓存目录 = os.environ.get("SCCACHE_DIR", str(Path.home() / "Documents/sccache-cache"))
            缓存上限 = os.environ.get("SCCACHE_CACHE_SIZE", "5G")
            公共 = dict(树=str(树路径), 缓存目录=缓存目录, 缓存上限=缓存上限)
            (树路径 / "target").mkdir(exist_ok=True)
            (树路径 / "target/ninja_setup.cmd").write_text(
                设置脚本模板.format(vcvars=vcvars, ninja=ninja, sccache=sccache, **公共), encoding="ascii")
            (树路径 / "target/ninja_build.cmd").write_text(
                构建脚本模板.format(vcvars=vcvars, **公共), encoding="ascii")
            配置 = subprocess.run(["cmd", "/c", "target\\ninja_setup.cmd"], cwd=树路径,
                                  capture_output=True, text=True)
            状态 = "完成（build: cmd /c target\\ninja_build.cmd）" if 配置.returncode == 0 \
                else f"失败——{配置.stdout and 配置.stdout.splitlines()[-1]}"
            print(f"[3] Ninja+sccache 开发树配置{状态}")

    if 自动立项:
        立行于树(树路径, 任务号, 立行 or "", 前置, 优先级)
    elif 接棒 and 任务号 not in 扫行号(树路径 / "plans", "021*.md"):
        # 230 修法②配套：接棒树须含本号立项行（226 立规）——旧分支缺行=集成收口扫不到=静默漏销账
        print(f"[警告] 分支 {分支} 树内 021 无本号立项行——须手工补行"
              f"（集成收口扫工作树总账·缺行=静默漏销账·226 实录）")

    # 238：不入库运维凭据同步进新树（worktree 里跑 integrate 读 scripts/queue_client.json——
    # gitignore 文件新树天然缺失，此前靠手工 export CN_QUEUE_*，忘装即「看板已废档」失败）
    凭据 = 本树根 / "scripts" / "queue_client.json"
    if 凭据.exists():
        (树路径 / "scripts").mkdir(exist_ok=True)
        shutil.copy(凭据, 树路径 / "scripts" / "queue_client.json")
        print("[5] queue_client.json 已同步进新树（gitignore 运维面·不入库）")

    # 329：create 即推分支——push=认领（196 立规）·治「意图已上板而在飞区无此分支」
    # 窗口（create 只建本地分支·用户忘 push 则看板两区不一致·用户报 BUG 2026-10-09）。
    # 失败仅警告不阻断（离线/权限面·--no-push 逃生门保留旧节奏）。
    if 不推分支:
        print("[6] 跳过自动推分支（--no-push）——认领待手动 push 生效")
    else:
        推 = 运行(["git", "push", "-u", "gitcode", 分支])
        if 推.returncode == 0:
            print(f"[6] 分支已自动推送（push=认领·看板在飞区 8s 内可见）")
        else:
            print(f"[黄] 自动推分支失败：{(推.stderr or '').strip().splitlines()[-1] if (推.stderr or '').strip() else '?'}"
                  f"——手动 git push -u gitcode {分支}（认领待生效）")

    print(f"""
[完成] {树路径}（分支 {分支}·{模式}）
  下一步（AGENTS.md §2/§7）：{'①021 立项行已自动随分支（--行 模式）' if 自动立项 else '①plans/021 改行 ⬜→🏃+备注分支=任务/'+任务号} ②（分支已自动推送·如 --no-push 则手动 push 认领）
  ③提交前 L1 门禁 gate_quick.py（win 全量=ci.ps1）④收工 integrate.py（自动 021 收口）""")
    if not 接棒:
        自动claim意图(任务号, 立行 or "")
    return 0


def 列树() -> int:
    结果 = 输出(["git", "worktree", "list", "--porcelain"])
    树们: list[tuple[str, str]] = []
    路径 = ""
    for 行 in 结果.splitlines():
        if 行.startswith("worktree "):
            路径 = 行[9:]
        elif 行.startswith("branch ") and 路径:
            树们.append((路径, 行[7:].replace("refs/heads/", "")))
        elif 行.startswith("detached") and 路径:
            树们.append((路径, "(detached)"))
    表头 = f"{'分支':<44} {'树':<16} 状态"
    print(表头)
    print("-" * 74)
    for 树路径, 分支 in sorted(树们, key=lambda x: x[0]):
        状态 = 运行(["git", "-C", 树路径, "status", "--porcelain"])
        脏 = f"脏×{len(状态.stdout.splitlines())}" if 状态.stdout.strip() else "净"
        print(f"{分支:<44} {Path(树路径).name:<16} {脏}")
    return 0


def 删树(任务号: str, 删分支: bool) -> int:
    树路径 = 主树根().parent / f"wt{任务号}"
    if not 树路径.exists():
        print(f"[失败] {树路径} 不存在")
        return 1
    分支 = 运行(["git", "-C", str(树路径), "branch", "--show-current"]).stdout.strip()
    print(f"[1] 删除 {树路径}" + (f"（分支 {分支}）" if 分支 else ""))
    shutil.rmtree(树路径, ignore_errors=True)
    运行(["git", "worktree", "prune"])
    print("[2] git worktree prune 完成")
    if 删分支 and 分支:
        运行(["git", "fetch", "gitcode"])
        并入 = 运行(["git", "merge-base", "--is-ancestor", 分支, "gitcode/develop"])
        if 并入.returncode != 0:
            print(f"[3] [拒绝] 分支 {分支} 未并入 develop——保留分支（确认后手动 git branch -D）")
        else:
            独有提交 = 输出(["git", "rev-list", "--count", f"gitcode/develop..{分支}"])
            运行(["git", "branch", "-d", 分支])
            说明 = "已并入 develop（含独有提交）" if int(独有提交 or 0) > 0 else "空分支（无独有提交·刚建即删无损失）"
            print(f"[3] 分支 {分支} {说明}，本地分支已删")
    elif 分支:
        print(f"[3] 分支 {分支} 保留（--delete-branch 可在已并入时删除）")
    return 0


def 缓存(动作: str) -> int:
    sccache = 找sccache()
    if sccache is None:
        print("[失败] sccache 未安装（winget install Mozilla.sccache）")
        return 1
    环境 = dict(os.environ, SCCACHE_DIR=os.environ.get("SCCACHE_DIR", str(Path.home() / "Documents/sccache-cache")))
    子命令 = {"stats": ["--show-stats"], "start": ["--start-server"], "stop": ["--stop-server"],
              "clear": ["--stop-server"]}[动作]
    结果 = subprocess.run([str(sccache), *子命令], env=环境)
    if 动作 == "clear" and 结果.returncode == 0:
        目录 = Path(环境["SCCACHE_DIR"])
        shutil.rmtree(目录, ignore_errors=True)
        print(f"[完成] 缓存目录已清（{目录}）")
    return 结果.returncode


def 主流程() -> int:
    解析器 = argparse.ArgumentParser(description="本机多开 worktree 任务池管理（AGENTS.md §7）")
    子 = 解析器.add_subparsers(dest="命令", required=True)
    p建 = 子.add_parser("create", help="建树+任务分支 任务/<任务号>（含 Ninja+sccache 开发树）")
    p建.add_argument("任务号", nargs="?", default="",
                     help="021 总账行号（如 087、178a；远端分支已存在=接棒续做·"
                          "省略=服务器权威自动发号 308j·服务不可达降级本地取号）")
    p建.add_argument("--行", help="一句话任务描述——021 无此行时自动立项建行（238 立项命令化·行随分支首提交）")
    p建.add_argument("--前置", default="—", help="前置任务号（逗号分隔·默认 —=无）")
    p建.add_argument("--优先级", default="P1", choices=["P0", "P1", "P2", "P3"], help="默认 P1")
    p建.add_argument("--no-ninja", action="store_true", help="跳过 Ninja 开发树配置")
    p建.add_argument("--no-push", action="store_true",
                     help="跳过 create 后的自动推分支（旧节奏：手动 push 认领）")
    p建.add_argument("--takeover", action="store_true",
                     help="接管他机 48h 内仍活跃的任务分支（285 撞车根治显式确认面·308a）")
    p列 = 子.add_parser("list", help="列出全部 worktree")
    p删 = 子.add_parser("remove", help="删树（win 下 rm -rf+prune）")
    p删.add_argument("任务号")
    p删.add_argument("--delete-branch", action="store_true", help="分支已并入 develop 时一并删分支")
    p缓 = 子.add_parser("cache", help="sccache 缓存服务")
    p缓.add_argument("动作", choices=["stats", "start", "stop", "clear"], nargs="?", default="stats")
    参数 = 解析器.parse_args()
    if 参数.命令 == "create":
        return 建树(参数.任务号, 参数.no_ninja, getattr(参数, "行", None),
                    getattr(参数, "前置", "—"), getattr(参数, "优先级", "P1"),
                    getattr(参数, "takeover", False),
                    getattr(参数, "no_push", False))
    if 参数.命令 == "list":
        return 列树()
    if 参数.命令 == "remove":
        return 删树(参数.任务号, 参数.delete_branch)
    return 缓存(参数.动作)


if __name__ == "__main__":
    sys.exit(主流程())
