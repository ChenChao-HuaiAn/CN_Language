#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_deploy_manifest.py —— 部署清单一致性门禁（任务 896·406/824 漏部署教训立法化）。

现象（406/824 两次实录）：deploy_board.sh 的 scp 拷贝清单与看板服务的真实运行依赖
  不一致——406 漏 board_adjudication.py、824 漏 board_xxx——服务上线后 import 崩/
  功能缺块，部署类漏项无机械防线、全靠事后人眼发现。
本门禁把「拷贝清单 ⊇ 运行依赖」变成断言（fail-closed·任何无法静态证明安全的面=红）：

  面① 依赖闭包：从部署脚本声明的清单模块出发（入口=systemd 启动模块·见顶部登记表），
    AST 解析全部 import（含函数内延迟导入与 importlib/__import__ 字面量动态导入），
    本地模块（scripts/<名>.py 存在者）的传递闭包必须全部在拷贝清单内；
    非字面量动态导入=无法静态判定→红（不静默放过）。
  面② 内联打包读取面：部署脚本 python heredoc 里 pathlib 链式 read_text/read_bytes
    读取的源文件（打包步骤的运行依赖）必须存在且被拷贝清单覆盖
    （deploy_queue.sh 漏 scp tokens.css 类=红）。
  面③ 静态前端面：各 www 部署目录的 index.html 本地引用（link/script 的
    href/src·跳过 data:/http(s)://）部署后必须存在——本地目录内存在（tar 整传）
    或 由 scp 的服务器目标路径补传（如 deploy_queue 从 board_www 补 tokens.css）二者之一。
  面④ 清单自洽：清单内文件/目录必须存在；globs 发现的 deploy_*.sh 必须已登记
    入口或豁免（新部署脚本=红提示登记——防门禁盲区）。

  正向比对恒为「依赖 ⊆ 清单」：清单多列（如 selftest）不红，只抓漏项。

登记面（新增部署脚本/入口变更时改顶部两表）：
  部署脚本入口 = {脚本名: systemd 启动模块}；非拷贝清单模式脚本放豁免表并注明缘由。

用法：
  python3 scripts/check_deploy_manifest.py             # 检查真实仓库（gate_quick 静态段调用）
  python3 scripts/check_deploy_manifest.py --root DIR  # 检查指定 scripts 目录（selftest/演练）
  python3 scripts/check_deploy_manifest.py --selftest  # 内置正反例自测（真实树+临时镜像树）
退出码：0=绿；1=红（缺项等）；2=用法/环境错误。
"""
# 兼容 python3.8（深度机/单位机系统 python）：注解惰性化
from __future__ import annotations

import ast
import re
import shutil
import sys
import tempfile
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent

# 部署脚本 → 服务运行入口模块（systemd 启动面：cn-board=board_service.py /
#   cn-queue=queue_service.py）。★新增「拷贝清单」模式部署脚本必须在此登记，
#   漏登记=门禁红（防新脚本绕过检查——与 check_registry_audit「新门禁必须注册」同构）。
部署脚本入口 = {
    "deploy_board.sh": "board_service.py",
    "deploy_queue.sh": "queue_service.py",
}
# 非拷贝清单模式的部署脚本——显式豁免并注明缘由（有新模式须重审此表）。
部署脚本豁免 = {
    "deploy_website.sh": "整树静态发布（tar 整个 website/ 目录）——无模块拷贝清单面",
}

# 内联打包 heredoc：python3 - <参数…> <<'标记' … 标记
_HEREDOC = re.compile(r"python3\s+-\s+([^\n]*?)\s*<<\s*'(\w+)'\n(.*?)\n\2(?:\n|$)", re.S)
# 续行合并（bash 反斜杠换行）
_续行 = re.compile(r"\\[ \t]*\n")
# scp 行内 $SCRIPT_DIR/<文件> 形态（清单文件级项）
_清单文件 = re.compile(r"\$SCRIPT_DIR/([^\s\"'$]+)")
# tar 目录上传面：tar czf - -C "$SCRIPT_DIR/<目录>" .
_tar目录 = re.compile(r"-C\s+\"\$SCRIPT_DIR/([^\"]+)\"")
# scp 服务器目标面：$REMOTE_DIR/<相对路径>（补传文件·非 "$REMOTE_DIR/" 目录尾）
_服务器目标 = re.compile(r"\$REMOTE_DIR/([^\s\"'$]+)")
# heredoc 变量绑定：A[, B] = pathlib.Path(sys.argv[k])[, pathlib.Path(sys.argv[m])]
_绑定argv = re.compile(
    r"([\w]+(?:\s*,\s*[\w]+)?)\s*=\s*pathlib\.Path\(sys\.argv\[(\d)\]\)"
    r"(?:\s*,\s*pathlib\.Path\(sys\.argv\[(\d)\]\))?")
# heredoc 子路径绑定：A = B / "段"
_绑定子路径 = re.compile(r"([\w]+)\s*=\s*([\w]+)\s*/\s*\"([^\"]+)\"")
# heredoc 读取链：变量(/\s*"段")* → .read_text() / .read_bytes()
_读取链 = re.compile(r"([\w]+)((?:\s*/\s*\"[^\"]+\")*)\s*\)?\s*\.read_(?:text|bytes)\b")
# index.html 本地资源引用
_标签 = re.compile(r"<(link|script)\b[^>]*>", re.S)
_属性 = re.compile(r"(?:href|src)\s*=\s*\"([^\"]+)\"")
_外链前缀 = ("data:", "http://", "https://", "//", "mailto:", "#")

# 动态导入函数名末段（ast.Attribute.attr / ast.Name.id 命中即动态导入面）
_动态导入名 = {"import_module", "__import__"}


def 解析部署脚本(脚本: Path) -> dict:
    """解析单个部署脚本 → {文件们, 目录们, 读取源们, 问题们}。

    文件们/目录们/读取源路径均为相对 scripts/ 的正斜杠字符串；
    读取源们=list[(相对路径, 描述)]；解析漂移（清单为空/读取链不可解析）=进问题们（fail-closed）。
    """
    文本 = 脚本.read_text(encoding="utf-8")
    名 = 脚本.name
    问题: list[str] = []
    文件们: set[str] = set()
    目录们: set[str] = set()
    读取源们: list[tuple[str, str]] = []
    服务器目标们: set[str] = set()

    # 先摘出 heredoc（其内容不参与 scp/tar 行匹配），再合并续行做识别
    heredoc们 = []
    主文本 = 文本
    for m in reversed(list(_HEREDOC.finditer(文本))):
        heredoc们.append((m.group(1), m.group(3)))
        主文本 = 主文本[:m.start()] + 主文本[m.end():]
    主文本 = _续行.sub(" ", 主文本)

    服务器目标们.update(_服务器目标.findall(主文本))
    for m in re.finditer(r"\bscp\b[^\n;]*", 主文本):
        文件们.update(_清单文件.findall(m.group(0)))
    for m in re.finditer(r"\btar\b[^\n;]*", 主文本):
        d = _tar目录.search(m.group(0))
        if d:
            目录们.add(d.group(1))

    if not 文件们 and not 目录们:
        问题.append(f"{名} 未解析出任何拷贝清单（scp $SCRIPT_DIR/… / tar -C $SCRIPT_DIR/…）"
                    f"——脚本形态漂移或清单为空，不能证明部署安全（fail-closed）")

    for 参数串, 代码 in heredoc们:
        读取源们 += 解析内联打包(名, 参数串, 代码, 问题)
    return {"文件们": 文件们, "目录们": 目录们, "读取源们": 读取源们,
            "服务器目标们": 服务器目标们, "问题们": 问题}


def 解析内联打包(名: str, 参数串: str, 代码: str, 问题: list[str]) -> list[tuple[str, str]]:
    """解析 heredoc 内联打包脚本的文件读取面 → 读取源们（不进问题=合法构建产物面）。

    绑定还原：pathlib.Path(sys.argv[k]) 基 + 链式 / "段" 子路径；读取链变量不在
    绑定表=无法静态判定源文件→红（静默放过=漏项防线失守）。
    """
    参数们 = re.findall(r"\"([^\"]+)\"", 参数串)
    argv表 = {i + 1: p for i, p in enumerate(参数们)}
    基表: dict[str, str] = {}
    for m in _绑定argv.finditer(代码):
        左名们 = [x.strip() for x in m.group(1).split(",")]
        下标们 = [x for x in (m.group(2), m.group(3)) if x]
        if len(左名们) != len(下标们):
            问题.append(f"{名} 内联打包绑定无法解析（左值/右值数不等）: {m.group(0)!r}")
            continue
        for 左名, 下标 in zip(左名们, 下标们):
            if int(下标) not in argv表:
                问题.append(f"{名} 内联打包引用 sys.argv[{下标}] 超出 python3 命令行参数"
                            f"（共 {len(argv表)} 个）——无法静态解析")
                continue
            基表[左名] = argv表[int(下标)]
    for _ in range(8):  # 子路径链迭代到不动点（a = b / "x"; c = a / "y" …）
        改 = False
        for m in _绑定子路径.finditer(代码):
            左, 基, 段 = m.group(1), m.group(2), m.group(3)
            if 基 in 基表 and 基表.get(左) != 基表[基] + "/" + 段:
                基表[左] = 基表[基] + "/" + 段
                改 = True
        if not 改:
            break

    读取源们: list[tuple[str, str]] = []
    for m in _读取链.finditer(代码):
        变量 = m.group(1)
        段们 = re.findall(r"\"([^\"]+)\"", m.group(2) or "")
        if 变量 not in 基表:
            问题.append(f"{名} 内联打包读取链无法静态解析：变量「{变量}」不在 "
                        f"pathlib.Path(sys.argv[…]) 绑定表——保持标准打包形态或更新本门禁解析器")
            continue
        完整 = 基表[变量] + (("".join("/" + s for s in 段们)) if 段们 else "")
        if 完整.startswith("$SCRIPT_DIR/"):
            读取源们.append((完整[len("$SCRIPT_DIR/"):], f"{名} 内联打包读取"))
        elif 完整.startswith("$"):
            continue  # 构建期产物（如 $DIST_TMP）——由打包步骤自产，非部署依赖
        else:
            问题.append(f"{名} 内联打包读取链基路径非 $SCRIPT_DIR/$DIST_TMP 形态：{完整!r}")
    return 读取源们


def 依赖闭包(根: Path, 起点们: set[str], 问题: list[str]) -> tuple[set[str], dict[str, set[str]]]:
    """从起点模块做本地模块传递闭包（AST import + 字面量动态导入）。

    返回 (闭包模块名集, 来源表 {模块名: {import 它的模块名…}})；非字面量动态导入=
    无法静态判定→红（冒烟口号：不能证明安全就不放行）。
    """
    访问过: set[str] = set()
    来源: dict[str, set[str]] = {}
    队列 = sorted(起点们)
    while 队列:
        当前 = 队列.pop()
        if 当前 in 访问过:
            continue
        访问过.add(当前)
        文件 = 根 / (当前 + ".py")
        if not 文件.exists():
            问题.append(f"依赖闭包解析目标不存在: {当前}.py（登记表/清单引用错误）")
            continue
        try:
            树 = ast.parse(文件.read_text(encoding="utf-8"))
        except (SyntaxError, UnicodeDecodeError) as e:
            问题.append(f"依赖闭包模块解析失败 {当前}.py: {e}——不能证明部署安全（fail-closed）")
            continue
        候选: set[str] = set()
        for 节点 in ast.walk(树):
            if isinstance(节点, ast.Import):
                候选.update(a.name.split(".")[0] for a in 节点.names)
            elif isinstance(节点, ast.ImportFrom) and not 节点.level and 节点.module:
                候选.add(节点.module.split(".")[0])
            elif isinstance(节点, ast.Call):
                函数 = 节点.func
                函数名 = 函数.attr if isinstance(函数, ast.Attribute) else (
                    函数.id if isinstance(函数, ast.Name) else "")
                if 函数名 in _动态导入名:
                    实际名 = "import_module" if 函数名 == "import_module" else "__import__"
                    if 节点.args and isinstance(节点.args[0], ast.Constant) \
                            and isinstance(节点.args[0].value, str):
                        候选.add(节点.args[0].value.split(".")[0])
                    else:
                        问题.append(f"{当前}.py 行 {getattr(节点, 'lineno', '?')} 存在非字面量动态导入"
                                    f"（{实际名}(…) 参数非常量）——无法静态判定目标模块，"
                                    f"不能证明部署安全（fail-closed·改字面量或更新本门禁）")
        for c in sorted(候选):
            if c == 当前 or not (根 / (c + ".py")).exists():
                continue  # 标准库/第三方/自指——非本地部署依赖
            来源.setdefault(c, set()).add(当前)
            if c not in 访问过:
                队列.append(c)
    return 访问过, 来源


def 解析前端引用(html文本: str) -> list[str]:
    """提取 index.html 的本地资源引用（link/script 的 href/src·去前导 /）。"""
    引用们 = []
    for m in _标签.finditer(html文本):
        属性 = _属性.search(m.group(0))
        if not 属性:
            continue
        u = 属性.group(1)
        if u.startswith(_外链前缀):
            continue
        引用们.append(u.lstrip("/"))
    return 引用们


def 被覆盖(路径: str, 文件们: set[str], 目录们: set[str]) -> bool:
    """部署覆盖判据：显式文件级 或 位于某个整传目录内。"""
    return 路径 in 文件们 or any(路径.startswith(d + "/") for d in 目录们)


def 检查(根: Path) -> tuple[list[str], list[str]]:
    """对根目录（scripts/ 形态）执行四面检查 → (问题们, 概述们)。"""
    问题: list[str] = []
    概述们: list[str] = []
    for s in sorted(根.glob("deploy_*.sh")):
        if s.name not in 部署脚本入口 and s.name not in 部署脚本豁免:
            问题.append(f"发现未登记的部署脚本 {s.name}——新部署脚本必须登记入口（顶部"
                        f"「部署脚本入口」表）或豁免（「部署脚本豁免」表）后门禁才覆盖")
    for 名, 入口 in sorted(部署脚本入口.items()):
        脚本 = 根 / 名
        if not 脚本.exists():
            问题.append(f"部署脚本缺失: {名}（登记表要求存在）")
            continue
        解析 = 解析部署脚本(脚本)
        问题 += 解析["问题们"]
        for f in sorted(解析["文件们"]):
            if not (根 / f).exists():
                问题.append(f"{名} 清单文件不存在: {f}——清单写了但文件不在树上")
        for d in sorted(解析["目录们"]):
            if not (根 / d).is_dir():
                问题.append(f"{名} 清单目录不存在: {d}/")
        if not (根 / 入口).exists():
            问题.append(f"{名} 运行入口模块缺失: {入口}（systemd 启动面·登记表）")
        else:
            起点们 = {f[:-3] for f in 解析["文件们"] if f.endswith(".py") and "/" not in f}
            起点们.add(入口[:-3])
            闭包, 来源 = 依赖闭包(根, 起点们, 问题)
            for m in sorted(闭包):
                if not 被覆盖(m + ".py", 解析["文件们"], 解析["目录们"]):
                    引者 = "、".join(sorted(来源.get(m, []))) or 入口
                    问题.append(f"{名} 部署清单缺运行依赖模块: {m}.py（被 {引者} import）"
                                f"——补入 scp 拷贝清单后重试")
        # 静态前端面 + 打包读取面
        静态数 = 0
        for d in sorted(解析["目录们"]):
            idx = 根 / d / "index.html"
            if not idx.exists():
                问题.append(f"{名} 部署目录缺 index.html: {d}/index.html")
                continue
            for ref in 解析前端引用(idx.read_text(encoding="utf-8")):
                静态数 += 1
                if (根 / d / ref).exists():
                    continue  # 本地目录内有（tar 整传即覆盖）
                if f"{d}/{ref}" in 解析["服务器目标们"]:
                    continue  # 部署链路 scp 补传（源存在性由清单/读取源面各自保证）
                问题.append(f"{名} 静态目录 {d}/ 引用的文件不存在且无补传: {ref}"
                            f"（来自 index.html——tar 传不了不存在的文件·scp 目标也无此路径）")
        for 路径, 描述 in 解析["读取源们"]:
            if not (根 / 路径).exists():
                问题.append(f"{名} 内联打包读取文件不存在: {路径}（{描述}）")
            elif not 被覆盖(路径, 解析["文件们"], 解析["目录们"]):
                问题.append(f"{名} 部署清单未含内联打包读取文件: {路径}（{描述}——"
                            f"打包步骤读它而清单没拷=线上缺文件）")
        概述们.append(f"{名}: 清单 {len(解析['文件们'])} 文件/{len(解析['目录们'])} 目录·"
                     f"依赖闭包 {len(闭包) if (根 / 入口).exists() else -1} 模块·"
                     f"静态引用 {静态数} 处·打包读取 {len(解析['读取源们'])} 处")
    return 问题, 概述们


# ── selftest：真实树正例 + 临时镜像树正反例 ──────────────────────────────────

_镜像文件 = ("deploy_board.sh", "deploy_queue.sh", "deploy_website.sh",
            "board_service.py", "board_tasks.py", "board_ci.py", "board_docs.py",
            "board_adjudication.py", "board_selftest.py", "queue_service.py",
            "queue_selftest.py")
_镜像目录 = ("board_www", "queue_www")


def _镜像树(真根: Path, 临时: Path) -> None:
    """把真实 scripts/ 的部署相关文件拷成临时镜像树（selftest 靶场）。"""
    for f in _镜像文件:
        shutil.copy2(真根 / f, 临时 / f)
    for d in _镜像目录:
        shutil.copytree(真根 / d, 临时 / d)


def _改文件(路径: Path, 原: str, 新: str) -> None:
    """替换靶点并断言确实命中（靶点未命中=selftest 自身失效·禁止假绿）。"""
    文本 = 路径.read_text(encoding="utf-8")
    if 原 not in 文本:
        raise AssertionError(f"selftest 靶点未命中（测试已失效）: {路径.name}: {原!r}")
    路径.write_text(文本.replace(原, 新), encoding="utf-8")


def selftest() -> int:
    """正反例自测：真实树正例 + 临时镜像树各反例（红后恢复再验绿）。"""
    失败: list[str] = []
    结果: list[tuple[str, bool, str]] = []

    def 记录(名: str, 通过: bool, 详情: str = "") -> None:
        结果.append((名, 通过, 详情))
        if not 通过:
            失败.append(名)

    def 跑例(名: str, 根: Path, 期望: str, 关键词们: tuple[str, ...] = ()) -> None:
        """期望='绿' 断言零问题；期望='红' 断言问题非空且每关键词至少一条命中。"""
        问题, _ = 检查(根)
        if 期望 == "绿":
            记录(名, not 问题, "；".join(问题[:3]))
        else:
            命中 = all(any(k in x for x in 问题) for k in 关键词们) and bool(问题)
            记录(名, 命中, f"问题={问题[:3]}")

    跑例("真实树正例（仓库当前状态应绿）", 脚本目录, "绿")

    临时 = Path(tempfile.mkdtemp(prefix="deploy_manifest_selftest_"))
    try:
        _镜像树(脚本目录, 临时)
        跑例("镜像树正例", 临时, "绿")

        # 反例 1：删清单项 board_adjudication.py（406 教训复现）
        _改文件(临时 / "deploy_board.sh",
              '"$SCRIPT_DIR/board_docs.py" "$SCRIPT_DIR/board_adjudication.py"',
              '"$SCRIPT_DIR/board_docs.py"')
        跑例("反例·删清单项 board_adjudication.py", 临时, "红", ("board_adjudication.py",))
        shutil.copy2(脚本目录 / "deploy_board.sh", 临时 / "deploy_board.sh")
        跑例("恢复清单后回绿", 临时, "绿")

        # 反例 2：新增未部署本地 import（824 类根因复现）
        (临时 / "board_phantom.py").write_text("# selftest 幽灵模块\n", encoding="utf-8")
        with (临时 / "board_service.py").open("a", encoding="utf-8") as f:
            f.write("\nimport board_phantom\n")
        跑例("反例·新增未部署 import board_phantom", 临时, "红", ("board_phantom.py",))
        shutil.copy2(脚本目录 / "board_service.py", 临时 / "board_service.py")
        (临时 / "board_phantom.py").unlink()
        跑例("恢复 import 后回绿", 临时, "绿")

        # 反例 3：index.html 引用不存在的静态文件
        with (临时 / "board_www" / "index.html").open("a", encoding="utf-8") as f:
            f.write('\n<link rel="stylesheet" href="/ghost.css">\n')
        跑例("反例·index.html 引用不存在 ghost.css", 临时, "红", ("ghost.css",))
        shutil.copy2(脚本目录 / "board_www" / "index.html", 临时 / "board_www" / "index.html")
        跑例("恢复 index.html 后回绿", 临时, "绿")

        # 反例 4：deploy_queue.sh 漏 scp tokens.css（打包读取面漏项复现）
        _改文件(临时 / "deploy_queue.sh",
              'scp -q "$SCRIPT_DIR/board_www/tokens.css" "$REMOTE:$REMOTE_DIR/queue_www/tokens.css"',
              '# selftest：临时删除 tokens.css 拷贝行')
        跑例("反例·deploy_queue 漏 scp tokens.css", 临时, "红",
             ("部署清单未含内联打包读取文件", "board_www/tokens.css"))
        shutil.copy2(脚本目录 / "deploy_queue.sh", 临时 / "deploy_queue.sh")
        跑例("恢复 tokens.css 拷贝后回绿", 临时, "绿")

        # 反例 5：heredoc 读取链变量不可解析（fail-closed 面）
        _改文件(临时 / "deploy_board.sh",
              '(src / "index.html").read_text', '(srcz / "index.html").read_text')
        跑例("反例·打包读取链变量不可解析", 临时, "红", ("无法静态解析",))
        shutil.copy2(脚本目录 / "deploy_board.sh", 临时 / "deploy_board.sh")
        跑例("恢复打包链后回绿", 临时, "绿")

        # 反例 6：未登记的部署脚本（门禁盲区防线）
        (临时 / "deploy_future.sh").write_text("#!/usr/bin/env bash\necho 未登记\n", encoding="utf-8")
        跑例("反例·未登记的 deploy_future.sh", 临时, "红", ("未登记的部署脚本",))
        (临时 / "deploy_future.sh").unlink()
        跑例("删除未登记脚本后回绿", 临时, "绿")
    finally:
        shutil.rmtree(临时, ignore_errors=True)

    for 名, 通过, 详情 in 结果:
        print(f"[selftest] {'✓' if 通过 else '✗'} {名}" + (f"（{详情}）" if not 通过 else ""))
    if 失败:
        print(f"[selftest] FAIL：{len(失败)}/{len(结果)} 例未过", flush=True)
        return 1
    print(f"[selftest] PASS ✓（{len(结果)} 例全过）", flush=True)
    return 0


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description="部署清单一致性门禁（896）")
    ap.add_argument("--root", default=None, help="被检查的 scripts 目录（默认=本脚本所在目录）")
    ap.add_argument("--selftest", action="store_true", help="内置正反例自测（临时镜像树）")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
    根 = Path(args.root).resolve() if args.root else 脚本目录
    if not 根.is_dir():
        print(f"[部署清单] 用法错误：--root 不是目录: {根}")
        return 2
    问题, 概述们 = 检查(根)
    if 问题:
        print("[部署清单] FAIL：", flush=True)
        for x in 问题:
            print(f"  - {x}", flush=True)
        print("  → 补齐拷贝清单/资源后重试（406 漏 board_adjudication·824 漏项教训）", flush=True)
        return 1
    print(f"[部署清单] PASS ✓（{len(概述们)} 个部署脚本·依赖闭包与静态面全对齐）", flush=True)
    for x in 概述们:
        print(f"  · {x}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
