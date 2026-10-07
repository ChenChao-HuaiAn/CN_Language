# -*- coding: utf-8 -*-
# task_board.py —— 021 任务总账工具（v5·2026-10-04 立法轮 1016·226 轮收口归档制）
#
# 职责（单一）：解析 plans/021-任务进度观察表.md 的任务总账表（含 项目记忆/归档/ 的
#   已完成任务归档切片），提供「就绪队列/关键路径/阻塞视图」（--ready）与「账实相符
#   检查」（--check）。
#
# 任务行格式（v5 总账·每任务一行走完全生命周期·收口即归档）：
#   | # | 任务 | 状态 | 前置 | 优先级 | 备注/下一棒 |
#   状态：⬜ 待办 / 🏃 在飞（备注含分支名 任务/<本行任务号>·196 轮立规）/
#         ⏸ 挂起（含「待用户批」）/ ✅ 完成（状态列含集成 sha10·收口后即剪切归档）
#   前置：依赖任务号列表（逗号分隔）或 — ；父子任务同款（父前置=子任务号列表）。
#   号支持字母后缀子任务（178a）；比对键=去前导零+后缀（087≡87），展示补零三位。
#
# 收口归档制（226 轮·2026-10-06 用户令「归档后的任务从任务栏删除·记录放 项目记忆/归档」）：
#   integrate 收口 ✅ 行自动剪切至 项目记忆/归档/plans021-已完成任务归档-<年月>.md；
#   归档行参与依赖图（前置引用=已完成）/跳号基准/重号防复用；分支与 sha 账实只核主表行；
#   号全局唯一含归档——新任务取全局最大有效号+1（主表回退不复用归档号）。
#
# 设计纪律（v5）：不检查行数/戳/文档形态（v4 教训：机械检查误报=折腾税）；
#   --check 只做真防遗漏：账实相符（🏃⇔分支存在且号段=本行/✅⇔sha/⬜⇔带优先级/
#   任务号重号——分支名=任务号要求全表唯一/任务号乱序——总账须按号升序排列
#   （2026-10-06 用户令·字母子号随父号））、依赖就绪（抢跑拦截）、依赖环检测。
#   230 修法①②（2026-10-07）：跳号基准并入远端 任务/<号>/batch/<号> 在飞纯数字号集
#   （在飞行号住未合分支树·226 立规）+幽灵分支检查（远端任务分支在主表/归档/分支树
#   均无行=红·分支树有行=在飞未合提示）。
#   旧格式分支 任务/<机>-<轮>-<标识>（196 轮前）过渡兼容：只提示不算红·收工即删。
#
# 用法：
#   python scripts/task_board.py --ready          # 开工前看一眼：该做什么
#   python scripts/task_board.py --check          # 提交前账实检查（integrate 挂同款）
#   python scripts/task_board.py --check --json   # 机读（integrate 消费）

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

仓库根 = Path(__file__).resolve().parent.parent
总账路径 = 仓库根 / "plans" / "021-任务进度观察表.md"
归档目录 = 仓库根 / "项目记忆" / "归档"
# 收口归档制（226 轮·2026-10-06 用户令）：✅ 行由 integrate 自动剪切至归档文件，
#   归档行参与依赖图/取号基准/重号防复用；分支与 sha 账实检查只对主表行做（封存不再核）。
归档名模式 = "plans021-已完成任务归档-*.md"

优先级序 = {"P0": 0, "P1": 1, "P2": 2, "P3": 3}
状态_待办, 状态_在飞, 状态_挂起, 状态_完成 = "⬜", "🏃", "⏸", "✅"


def 键化(号: str) -> str:
    """号规范化键：087→87（前导零等价）·178a→178a（字母后缀子任务原样保留）。"""
    m = re.fullmatch(r"(\d+)([a-z]?)", 号)
    return f"{int(m.group(1))}{m.group(2)}" if m else 号


def 展示号(k: str) -> str:
    m = re.fullmatch(r"(\d+)([a-z]?)", str(k))
    if not m:
        return str(k)
    n, 后缀 = int(m.group(1)), m.group(2)
    return f"{n:03d}{后缀}" if n < 1000 else f"{n}{后缀}"


def 解析任务行(行: str):
    """总账数据行 -> dict；非数据行返回 None。"""
    s = 行.strip()
    if not s.startswith("|") or s.startswith("| #") or set(s) <= set("|- "):
        return None
    段 = [c.strip() for c in s.strip("|").split("|")]
    if len(段) < 6 or not re.fullmatch(r"\d+[a-z]?", 段[0]):
        return None
    号 = 键化(段[0])
    任务 = 段[1]
    状态列 = 段[2]
    if 状态_完成 in 状态列:
        状态 = 状态_完成
    elif 状态_在飞 in 状态列:
        状态 = 状态_在飞
    elif 状态_挂起 in 状态列:
        状态 = 状态_挂起
    else:
        状态 = 状态_待办
    sha匹配 = re.search(r"\b([0-9a-f]{8,10})\b", 状态列)
    sha = sha匹配.group(1) if (状态 == 状态_完成 and sha匹配) else None
    前置 = []
    for tok in re.split(r"[，,;/ ]+", 段[3]):
        tok = tok.strip()
        if re.fullmatch(r"\d+[a-z]?", tok):
            前置.append(键化(tok))
    优先级 = 段[4] if 段[4] in 优先级序 else "P2"
    备注 = 段[5]
    分支 = None
    m = re.search(r"任务/[\w\u4e00-\u9fff-]+", 备注)
    if m:
        分支 = m.group(0)
    return {"号": 号, "任务": 任务, "状态": 状态, "sha": sha,
            "前置": 前置, "优先级": 优先级, "备注": 备注, "分支": 分支, "行": s}


def 读总账():
    if not 总账路径.exists():
        print(f"[task_board] 缺 {总账路径}", file=sys.stderr)
        sys.exit(2)
    行们 = 总账路径.read_text(encoding="utf-8").splitlines()
    return [t for t in (解析任务行(l) for l in 行们) if t]


def 读归档任务():
    """已归档任务行（收口归档制·226 轮）：归档行一律视为 ✅ 参与依赖图/取号/重号。"""
    任务们 = []
    for 文件 in sorted(归档目录.glob(归档名模式)):
        for 行 in 文件.read_text(encoding="utf-8").splitlines():
            t = 解析任务行(行)
            if t and t["状态"] == 状态_完成:
                任务们.append(t)
    return 任务们


def 读全表():
    """合并视图（主表行优先）：返回 (主表序列, 合并字典)。前置引用归档号=已完成不红。"""
    主表 = 读总账()
    表 = {t["号"]: t for t in 读归档任务()}
    表.update({t["号"]: t for t in 主表})
    return 主表, 表


def 远端分支表() -> set:
    r = subprocess.run(["git", "ls-remote", "--heads", "gitcode"],
                       capture_output=True, text=True, cwd=仓库根, timeout=60)
    return {l.split("refs/heads/")[1].strip() for l in r.stdout.splitlines()
            if "refs/heads/" in l}


def 在飞分支号集() -> set:
    """本地 remote-tracking+本地 heads 的 任务/<号>/batch/<号> 纯数字号集
    （269 补·230 修法①同族第四源）：在飞行号住未合分支树（226 立规），
    主表+归档看不见——本地并行会话 create 后未 push 的号同样占用号段
    （268 官网案实录：wt268 已立未推 → --check 误报 269 跳号）。已集成号必已入
    主表/归档（集成即删分支·立项行随批收口），并集无增量。零网络（只读本地引用）。
    """
    号集 = set()
    r = subprocess.run(["git", "for-each-ref", "--format=%(refname)", "refs/"],
                       capture_output=True, text=True, cwd=仓库根, timeout=30)
    for l in r.stdout.splitlines():
        m = re.search(r"(?:任务|batch)/(\d+)$", l.strip())
        if m:
            号集.add(m.group(1))
    return 号集


def 分支树含行(分支: str, 号: str) -> bool:
    """远端分支树内的 021 总账是否含本号行（230 修法②·核实 226 立规「立项行随分支」在飞形态）。

    主表无行的远端任务分支：分支树有本号行=在飞未合正常形态；也无=幽灵分支。
    只读本地 remote-tracking 引用（零网络）。
    """
    引用 = 分支 if 分支.startswith("gitcode/") else f"gitcode/{分支}"
    r = subprocess.run(["git", "ls-tree", "--name-only", 引用, "plans/"],
                       capture_output=True, text=True, cwd=仓库根, timeout=30)
    for 名 in r.stdout.splitlines():
        名 = 名.strip()
        if re.search(r"plans/021.*\.md$", 名):
            r2 = subprocess.run(["git", "show", f"{引用}:{名}"],
                                capture_output=True, text=True, cwd=仓库根, timeout=30)
            for 行 in r2.stdout.splitlines():
                m = re.match(r"^\|\s*(\d+[a-z]?)\s*\|", 行)
                if m and 键化(m.group(1)) == 号:
                    return True
    return False


def 找环(表):
    """有向图环检测（DFS 三色标记）。返回环路径列表。"""
    白, 灰, 黑 = 0, 1, 2
    色 = {k: 白 for k in 表}
    栈, 环们 = [], []

    def stack_slice(st, target):
        for i in range(len(st) - 1, -1, -1):
            if st[i] == target:
                return st[i:]
        return st

    def dfs(u):
        色[u] = 灰
        栈.append(u)
        for v in 表[u]["前置"]:
            if v not in 表:
                continue
            if 色[v] == 灰:
                环们.append(stack_slice(栈, v) + [v])
            elif 色[v] == 白:
                dfs(v)
        栈.pop()
        色[u] = 黑

    for k in 表:
        if 色[k] == 白:
            dfs(k)
    return 环们


def 基线就绪集(表):
    完成态 = {k: (v["状态"] == 状态_完成) for k, v in 表.items()}
    就绪集 = {k for k, t in 表.items()
              if not 完成态[k] and t["状态"] == 状态_待办
              and all(完成态.get(d, False) for d in t["前置"] if d in 表)}
    return 完成态, 就绪集


def 传递解锁数(表, 号):
    """完成 号 后（含级联）**新**就绪的任务数——扣除当前已就绪（基线），关键路径口径。"""
    完成态, 基线 = 基线就绪集(表)
    完成态[号] = True

    def 就绪(k):
        return all(完成态.get(d, False) for d in 表[k]["前置"] if d in 表)

    n, 变 = 0, True
    while 变:
        变 = False
        for k, t in 表.items():
            if k not in 基线 and not 完成态[k] and t["状态"] == 状态_待办 and 就绪(k):
                完成态[k] = True
                n += 1
                变 = True
    return n


def 前置状态摘要(表, t):
    段 = []
    for d in t["前置"]:
        if d not in 表:
            段.append(f"{展示号(d)}(未知号)")
        else:
            段.append(f"{展示号(d)}({'✅' if 表[d]['状态']==状态_完成 else 表[d]['状态']})")
    return "、".join(段)


def cmd_ready():
    任务们, 表 = 读全表()
    归档们 = 读归档任务()
    缺号 = sorted({d for t in 任务们 for d in t["前置"] if d not in 表})
    环们 = 找环(表)
    就绪 = [t for t in 任务们 if t["状态"] == 状态_待办
            and all(d in 表 and 表[d]["状态"] == 状态_完成 for d in t["前置"])]
    就绪.sort(key=lambda t: (优先级序[t["优先级"]], -传递解锁数(表, t["号"])))
    在飞 = [t for t in 任务们 if t["状态"] == 状态_在飞]
    挂起 = [t for t in 任务们 if t["状态"] == 状态_挂起]
    print(f"任务总账：{len(表)} 项（待办 {sum(1 for t in 任务们 if t['状态']==状态_待办)}"
          f"/在飞 {len(在飞)}/挂起 {len(挂起)}/完成 {sum(1 for t in 任务们 if t['状态']==状态_完成)}"
          f"）+已归档 {len(归档们)}（收口归档制·主表只留活任务）")
    if 缺号:
        print(f"⚠ 前置引用了不存在的任务号：{[展示号(d) for d in 缺号]}")
    if 环们:
        print(f"⚠ 依赖环：{[[展示号(x) for x in 环] for 环 in 环们]}")
    print("\n就绪队列（前置全绿·优先级+解锁数排序）：")
    for t in 就绪[:8]:
        解锁 = 传递解锁数(表, t["号"])
        print(f"  #{展示号(t['号'])} [{t['优先级']}] {t['任务'][:46]}"
              f"（完成解锁 {解锁} 项）")
    if 就绪:
        最 = 就绪[0]
        print(f"\n最紧迫：#{展示号(最['号'])} {最['任务']}")
    if 在飞:
        print("\n在飞：")
        for t in 在飞:
            print(f"  #{展示号(t['号'])} [{t['优先级']}] {t['任务'][:40]}（{t['分支'] or '未记分支'}）")
    卡链 = [t for t in 任务们 if t["状态"] == 状态_待办
            and any(d in 表 and 表[d]["状态"] in (状态_挂起,) for d in t["前置"])]
    if 挂起 or 卡链:
        print("\n卡点（⏸ 链）：")
        for t in 挂起:
            print(f"  #{展示号(t['号'])} {t['任务'][:40]}——{t['备注'][:36]}")
        for t in 卡链[:5]:
            print(f"  #{展示号(t['号'])} 被 ⏸ 前置卡住（{前置状态摘要(表, t)}）")
    return 0


def cmd_check(as_json: bool = False) -> int:
    任务们, 表 = 读全表()
    归档号集 = {t["号"] for t in 读归档任务()}
    缺号 = sorted({d for t in 任务们 for d in t["前置"] if d not in 表})
    问题 = []
    提示 = []
    for d in 缺号:
        问题.append(f"前置引用未知任务号：{展示号(d)}")
    for 环 in 找环(表):
        问题.append(f"依赖环：{' → '.join(展示号(x) for x in 环)}")
    重号们 = {k: c for k, c in Counter(t["号"] for t in 任务们).items() if c > 1}
    for k in sorted(重号们):
        问题.append(f"任务号重号：{展示号(k)} ×{重号们[k]}"
                    f"（分支名=任务/<任务号> 要求全表唯一·后立行改号·196 立规）")
    # 主表行复用已归档号拦截（收口归档制·226 轮）：号全局唯一含归档——新任务取全局 max+1
    for k in sorted({t["号"] for t in 任务们} & 归档号集):
        问题.append(f"任务号复用已归档号：#{展示号(k)}（收口归档制·号全局唯一含归档·"
                    f"新任务取全局最大有效号+1·226 轮立法）")
    # 行序检查（2026-10-06 用户令）：总账须按任务号升序排列（字母子号随父号之后·不占主序列）
    序列 = []
    for t in 任务们:
        m = re.fullmatch(r"(\d+)([a-z]?)", t["号"])
        序列.append(((int(m.group(1)), m.group(2)), t))
    for (键前, 行前), (键后, 行后) in zip(序列, 序列[1:]):
        if 键后 < 键前:
            问题.append(f"任务号乱序：#{展示号(行后['号'])} 紧跟 #{展示号(行前['号'])} 之后"
                        f"（相邻降序对·总账须按任务号升序·2026-10-06 用户令·将错位行移至升序位）")
    # 编号规则（2026-10-05 用户令·021 头部立法同源）：禁用号 353/354+按序取号不跳号
    #   跳号基准含已归档号（226 收口归档制）——主表最大号回退后仍按全局历史最大取号
    禁用号 = {"353", "354"}
    for t in 任务们:
        if t["号"] in 禁用号:
            问题.append(f"禁用号在用：#{展示号(t['号'])}"
                        f"（2026-10-05 用户令已改 198/199·永久禁用·原行应挂「原号 353/354」链）")
    数字任务号 = sorted({int(t["号"]) for t in 表.values()
                        if t["号"].isdigit() and t["号"] not in 禁用号}
                       | {int(n) for n in 在飞分支号集()
                          if n.isdigit() and n not in 禁用号})
    for v in 数字任务号:
        其余 = [x for x in 数字任务号 if x != v]
        if 其余 and v > max(其余) + 1:
            问题.append(f"任务号跳号：#{v}（除本行外最大有效号 {max(其余)}"
                        f"·用户令 2026-10-05 按顺序取号 max+1·禁用号 353/354 跳过·历史补记账须用户特批）")
    分支们 = 远端分支表()
    for t in 任务们:
        号 = t["号"]
        if t["状态"] == 状态_在飞:
            if not t["分支"]:
                问题.append(f"#{展示号(号)} 🏃 未记分支名（备注列须含 分支=任务/{展示号(号)}）")
            elif t["分支"] not in 分支们:
                问题.append(f"#{展示号(号)} 🏃 分支 {t['分支']} 远端不存在（已删=漏销账，改 ✅+sha；"
                            f"本地未推=推分支）")
            else:
                m = re.fullmatch(r"任务/(\d+[a-z]?)", t["分支"])
                if m and 键化(m.group(1)) != 号:
                    问题.append(f"#{展示号(号)} 🏃 分支 {t['分支']} 号段与本行不符"
                                f"（分支名=任务/<本行任务号>·196 立规）")
                elif not m:
                    提示.append(f"#{展示号(号)} 🏃 旧格式分支 {t['分支']}（过渡兼容·收工即删·新任务禁用）")
            未绿 = [d for d in t["前置"] if d in 表 and 表[d]["状态"] != 状态_完成]
            if 未绿:
                问题.append(f"#{展示号(号)} 🏃 前置未全绿：{前置状态摘要(表, t)}（抢跑依赖）")
        elif t["状态"] == 状态_完成:
            if not t["sha"]:
                问题.append(f"#{展示号(号)} ✅ 缺集成 sha（状态列补 sha10）")
        elif t["状态"] == 状态_待办:
            if t["分支"] and t["分支"] in 分支们:
                问题.append(f"#{展示号(号)} ⬜ 但分支 {t['分支']} 在远端存在（实际在飞？改 🏃）")
    # 230 修法②检查面（2026-10-07·与 wt.py create 侧同轮）：远端 任务/<号> 分支在主表+归档
    #   均无行——分支树有本号行=在飞未合正常形态（226 立规行随分支·提示留痕·集成随批入账）；
    #   分支树也无行=幽灵分支（红：已收口漏删=删分支；在飞漏立=分支树补立项行）
    有账号集 = {t["号"] for t in 表.values()}
    for b in sorted(分支们):
        m = re.fullmatch(r"任务/(\d+)", b)
        if not m or m.group(1) in 禁用号:
            continue
        幽灵号 = 键化(m.group(1))
        if 幽灵号 in 有账号集:
            continue
        if 分支树含行(b, 幽灵号):
            提示.append(f"远端分支 {b} 在飞未合（立项行随分支·226 新规·主表无行=正常形态）")
        else:
            问题.append(f"幽灵分支：远端 {b} 存在但主表/归档/分支树均无此号行"
                        f"（已收口漏删=删分支；在飞漏立=分支树补立项行）")
    # 241（2026-10-07）：备注列限长警告（AGENTS 单行 ≤400 字符旧规机械化·报告不拦——
    # 现存超长行系活任务的战报上下文，留给各任务认领/收口时自然重写，不搞运动式整改）
    for 件 in 表.values():
        if 件["状态"] in (状态_待办, 状态_在飞, 状态_挂起) and len(件.get("备注", "")) > 400:
            提示.append(f"#{展示号(件['号'])} 备注列 {len(件['备注'])} 字符 >400——战报下沉提交信息/"
                        f"归档，行内留结论+指针（认领/收口时顺手重写）")
    if as_json:
        print(json.dumps({"ok": not 问题, "问题": 问题}, ensure_ascii=False))
    else:
        for p in 提示:
            print(f"  [～] {p}")
        if 问题:
            print(f"[task_board] 账实不符 {len(问题)} 项：")
            for p in 问题:
                print(f"  [×] {p}")
        else:
            print(f"[task_board] 账实相符 ✓（主表 {len(任务们)} 项活任务·"
                  f"{sum(1 for t in 任务们 if t['状态']==状态_在飞)} 在飞·"
                  f"已归档 {len(归档号集)}）")
    return 1 if 问题 else 0


def main():
    ap = argparse.ArgumentParser(description="021 任务总账：图谱与账实检查（v5）")
    ap.add_argument("--ready", action="store_true", help="就绪队列/最紧迫/阻塞视图")
    ap.add_argument("--check", action="store_true", help="账实相符+依赖环检查")
    ap.add_argument("--json", action="store_true", help="--check 机读输出")
    参数 = ap.parse_args()
    if 参数.ready:
        return cmd_ready()
    if 参数.check:
        return cmd_check(参数.json)
    ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
