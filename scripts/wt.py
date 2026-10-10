#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""wt.py —— 本机多开 worktree 任务池管理（449-a·AGENTS.md §7）。

同机并行开发互踩的根因=多会话共享一个工作树与 target/（429-a/431-a 实录：
checkout 带走未提交内容 / E2E 中途 expected 消失 / cn.exe 占用 LNK1104）。
本脚本把「每任务一个 worktree」制度化：

  create <任务号>        建树+任务分支 任务/<任务号>（382 看板 v2：立项/认领走服务端
                         任务台账 API——号唯一由服务端保证·服务不可达硬拒不再离线降级；
                         021 文档已退位·立项行不再落树；远端分支已存在=接棒同分支续做
                         ——机器停摆他机无缝接力·196 轮立规；台账 ⬜/⏸ 行=待认领，
                         create 即 ⬜→🏃；台账 ✅=已收口拒复用；带 --行 一句话即立项；
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
    """解析 021 已归档任务号集合（收口归档制·226 轮立法）——382 后仅作视野上报源
    （服务端台账已为唯一权威·迁移窗口期保证全机视野完整）。"""
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


def 服务调(方法: str, 路径: str, 体=None) -> dict | None:
    """看板 API 统一调用（382）。GET 只读公开；POST 带 Bearer 令牌。
    服务不可达返回 None——调用方一律硬拒（382 立规：任务台账唯一权威在服务端，
    离线降级取号已废除，宁停不裂）。"""
    令牌文 = ""
    try:
        令牌文 = json.loads((本树根 / "scripts" / "queue_client.json")
                            .read_text(encoding="utf-8")).get("令牌", "")
    except (OSError, ValueError):
        pass
    请求 = urllib.request.Request(板服务地址() + 路径, method=方法,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
    if 令牌文 and 方法 == "POST":
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


def 服务立项(描述: str, 前置: str, 优先级: str, 请求号: str = "") -> dict | None:
    """382：立项走服务端唯一权威（号唯一=任务表主键+事务·根治撞号/跳号）。
    请求号空=服务端发新号；非空=指定号（已占 409）。"""
    return 服务调("POST", "/api/task_create", {
        "标题": (描述 or "").strip() or "（待补标题）",
        "前置": 前置, "优先级": 优先级,
        "机器": 机名(), "对话id": 本树根.name,
        "请求号": 请求号, "来源": "wt.py"})


def 服务认领(任务号: str) -> dict | None:
    """382：认领=台账状态 ⬜/⏸ → 🏃（分支名服务端自动合成 任务/<号>）。"""
    return 服务调("POST", "/api/task_update",
                  {"号": 任务号, "状态": "🏃", "归属": f"{机名()}-wt{任务号}"})


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


def 接棒知情(分支: str, 接管: bool) -> bool:
    """308a 丙案（285 撞车实录根治）保留：接棒前强制「对方活跃度」知情——
    fetch 已最新，展示远端分支头提交时刻/作者/题；48h 内有提交=他机活跃中，
    无 --takeover 显式确认则拒。返回 False=拒接。"""
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
              f"wt.py create {分支.split('/')[1]} --takeover")
        return False
    if 活跃中:
        print("[接管] --takeover 显式确认——同分支续做（请在提交信息/交接注明接管缘由）")
    else:
        print("[提示] 陈旧分支（48h 无提交）——正常接棒（停摆接力语义）")
    return True


def 建树(任务号: str = "", 无ninja: bool = False, 立行: str | None = None,
         前置: str = "—", 优先级: str = "P1", 接管: bool = False,
         不推分支: bool = False) -> int:
    # 382 看板 v2：任务台账唯一权威=服务端任务表（board_service 8301）。
    # create=「服务端立项/认领 → 建树 → push 分支」——021 文档已退位（行不再落树），
    # 服务不可达一律硬拒（离线取号降级废除：号唯一由服务端保证·宁停不裂）。
    运行(["git", "fetch", "--prune", "gitcode"])
    新立项 = False
    if not 任务号:
        # 自动发号：服务端任务表∪发号台账 max+1（全局唯一·禁用号跳过）
        结 = 服务立项(立行 or "", 前置, 优先级)
        if 结 is None:
            print("[失败] 看板服务不可达——立项硬拒（382 立规：号唯一由服务端保证·"
                  "离线取号已废除；恢复网络/核对 CN_BOARD_URL 后重试）")
            return 1
        if not 结.get("号"):
            print(f"[失败] 立项被拒：{结.get('错误', 结)}")
            return 1
        任务号 = str(结["号"])
        新立项 = True
        print(f"[发号] 服务端发号：{任务号}（任务表∪台账 max+1·全局唯一）")
    else:
        if not re.fullmatch(r"[0-9]+[a-z]?", 任务号):
            print(f"[失败] 任务号「{任务号}」不合法——数字+可选小写字母后缀（如 087、178a）")
            return 1
        if 任务号 in {"353", "354"}:
            print(f"[失败] 任务号 {任务号} 为永久禁用号（2026-10-05 用户令·新号=全局最大+1）")
            return 1
    分支 = f"任务/{任务号}"
    远端分支在 = bool(输出(["git", "rev-parse", "--verify", f"refs/remotes/gitcode/{分支}"]))
    接棒 = False
    if not 新立项:
        任务 = 服务调("GET", f"/api/task/{任务号}")
        if 任务 is None:
            print("[失败] 看板服务不可达——无法核对台账，create 硬拒（382 立规）")
            return 1
        t = 任务.get("任务")
        if t is None:
            if 远端分支在:
                # 迁移窗口期兼容：远端分支在而服务端台账无（旧制度在飞任务）
                print(f"[黄] 服务端台账无 #{任务号} 但远端 {分支} 存在——旧制度在飞任务·"
                      f"接棒续做（迁移窗口期兼容·收口时服务端补账）")
                接棒 = True
            elif 立行:
                结 = 服务立项(立行, 前置, 优先级, 请求号=任务号)
                if 结 is None:
                    print("[失败] 看板服务不可达——立项硬拒")
                    return 1
                if not 结.get("号"):
                    print(f"[失败] 立项被拒：{结.get('错误', 结)}")
                    return 1
                新立项 = True
                print(f"[立项] 服务端已立项 #{任务号}：{立行[:60]}")
            else:
                print(f"[失败] 台账无任务 {任务号} 且远端无 {分支}"
                      f"——用 wt.py create {任务号} --行 \"一句话描述\" 立项建树，"
                      f"或看板网页「＋ 新建任务」（382 起立项入口=看板服务端）")
                return 1
        elif t["状态"] == "✅":
            print(f"[失败] 任务 {任务号} 已收口完成（✅ {t.get('收口sha', '')}）——"
                  f"号全局唯一防复用·重开场景立新号")
            return 1
        elif t["状态"] == "🏃":
            接棒 = True
            if 立行:
                print("[提示] 任务已在飞——--行 忽略（标题以服务端台账为准）")
        else:
            # ⬜ 待办 / ⏸ 挂起——create 即认领（先服务端流转成功再建树）
            认领 = 服务认领(任务号)
            if 认领 is None:
                print("[失败] 看板服务不可达——认领硬拒（树未建·零半成品）")
                return 1
            if 认领.get("HTTP错误"):
                print(f"[失败] 认领被拒：{认领.get('错误', 认领)}")
                return 1
            print(f"[认领] 台账 #{任务号} {t['状态']}→🏃（归属 {机名()}-wt{任务号}）")
    if 接棒 and not 接棒知情(分支, 接管):
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

    # 238 沿革：不入库运维凭据同步进新树（worktree 里跑 integrate 读 scripts/queue_client.json）
    凭据 = 本树根 / "scripts" / "queue_client.json"
    if 凭据.exists():
        (树路径 / "scripts").mkdir(exist_ok=True)
        shutil.copy(凭据, 树路径 / "scripts" / "queue_client.json")
        print("[4] queue_client.json 已同步进新树（gitignore 运维面·不入库）")

    # 329 沿革：create 即推分支——push=认领（196 立规）。
    if 不推分支:
        print("[5] 跳过自动推分支（--no-push）——认领待手动 push 生效")
    else:
        推 = 运行(["git", "push", "-u", "gitcode", 分支])
        if 推.returncode == 0:
            print("[5] 分支已自动推送（push=认领·看板在飞区 8s 内可见）")
        else:
            print(f"[黄] 自动推分支失败：{(推.stderr or '').strip().splitlines()[-1] if (推.stderr or '').strip() else '?'}"
                  f"——手动 git push -u gitcode {分支}（认领待生效）")

    if not 接棒:
        # 382：新立项/待办认领后补一笔 🏃 流转（自动发号路径立项时是 ⬜——建树即认领）
        认领 = 服务认领(任务号)
        if 认领 and not 认领.get("HTTP错误"):
            print(f"[6] 台账 #{任务号} 已置 🏃（归属 {机名()}-wt{任务号}）")
    print(f"""
[完成] {树路径}（分支 {分支}·{模式}）
  下一步（382 看板 v2）：①直接开发——任务台账已服务端记账（plans/021 已退位·勿再改它）
  ②（分支已自动推送·如 --no-push 则手动 push 认领）③提交前 L1 门禁 gate_quick.py
  ④收工 integrate.py（自动服务端销账 ✅+sha）""")
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
                     help="任务号（如 087、178a；台账 ⬜/⏸=认领·🏃=接棒·省略=服务端自动发号）")
    p建.add_argument("--行", help="一句话任务描述——台账无此号时即立项（382 起立项入口=看板服务端）")
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
