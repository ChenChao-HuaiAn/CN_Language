#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_cli.py —— 文档资源统一 CLI（384·文档上服务器二期）。

交接/教训/规范覆盖三资源的写入口（服务端 board_docs 资源·TX_01:8301）：
  python scripts/board_cli.py 收工 --行 "本轮一句话+下一棒指针"   # 交接追加+会话注销（收工一条命令）
  python scripts/board_cli.py 交接 --行 "条目" [--机器 深度机]     # 只写交接
  python scripts/board_cli.py 教训 --标题 "标题（权重 N）" [--正文 "..."] [--权重 N] [--标注 活跃]
  python scripts/board_cli.py 教训索引 [--高权重 8]                # 扫读：一行索引/高权重全文标题
  python scripts/board_cli.py 教训看 <id>                          # 看全文
  python scripts/board_cli.py 用例 <单元ID> <正例|边界例|负例> <用例名[,用例名…]>   # 归入单元格（追加）
  python scripts/board_cli.py 豁免 <用例名[,用例名…]> <理由>        # 豁免登记（理由必填）
  python scripts/board_cli.py 覆盖                                 # 覆盖矩阵摘要（缺口=空格）
  python scripts/board_cli.py 裁决 --批 <件们.json>                 # 裁决项批量上传（403·AI 侧）
  python scripts/board_cli.py 裁决 --号 379 --标题 "…" --讲解文件 x.md --选项文件 o.json
  python scripts/board_cli.py 裁决看 [--全部]                       # 裁决项一览（缺省只看待裁决）
批文件格式：[{号,标题,优先级?,讲解,选项们:[{键,描述,推荐?}]}]——讲解长文走文件不走命令行。
机器名自动=CN_MACHINE_NAME（须为 家机/深度机/单位机）或 hostname 前缀别名；识别不出用 --机器。
"""
from __future__ import annotations

import json
import socket
import sys
import urllib.parse
import urllib.request
from pathlib import Path

脚本目录 = Path(__file__).resolve().parent
本树根 = 脚本目录.parent
默认地址 = "http://124.222.106.84:8301"
三机名 = ("家机", "深度机", "单位机")
主机别名 = {"deepin": "深度机", "CHENCHAO-W": "家机", "CHENCHAO-PC": "家机",
            "arm64": "单位机", "user-pc": "单位机"}


def 服务地址() -> str:
    import os
    return os.environ.get("CN_BOARD_URL", 默认地址).rstrip("/")


def 令牌() -> str:
    try:
        return json.loads((脚本目录 / "queue_client.json").read_text(encoding="utf-8")) \
            .get("令牌", "")
    except (OSError, ValueError):
        return ""


def 机器名(显式: str = "") -> str:
    import os
    if 显式:
        return 显式
    环境 = os.environ.get("CN_MACHINE_NAME", "").strip()
    if 环境 in 三机名:
        return 环境
    host = socket.gethostname()
    for 前缀, 名 in 主机别名.items():
        if host.upper().startswith(前缀.upper()):
            return 名
    return ""


def 调服务(方法: str, 路径: str, 体=None, 超时=15):
    """返回 (HTTP码, 响应dict)。写操作带 Bearer；服务不可达抛 OSError 由调用方兜底。"""
    请求 = urllib.request.Request(服务地址() + 路径, method=方法,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
    if 令牌() and 方法 == "POST":
        请求.add_header("Authorization", f"Bearer {令牌()}")
    请求.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(请求, timeout=超时) as r:
            return r.status, json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return e.code, json.loads(e.read().decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return e.code, {}


def 落缓存(名: str, 体) -> None:
    """GET 结果落本地缓存（board_cache.json·gitignore）——服务不可达时 standup/
    check_spec_coverage 降级读最后一份，不卡本地流程。"""
    缓 = 脚本目录 / "board_cache.json"
    try:
        数据 = json.loads(缓.read_text(encoding="utf-8")) if 缓.exists() else {}
        数据[名] = 体
        数据[f"{名}时戳"] = __import__("time").time()
        缓.write_text(json.dumps(数据, ensure_ascii=False, indent=1), encoding="utf-8")
    except (OSError, ValueError):
        pass


def cmd_收工(a) -> int:
    机器 = 机器名(a.机器)
    if not 机器:
        print("[失败] 机器名识别不出——用 --机器 家机|深度机|单位机 显式指定")
        return 2
    if not a.行:
        print("[失败] --行 必填（本轮一句话+下一棒指针·v5 两行制）")
        return 2
    码, r = 调服务("POST", "/api/handoff_add", {"机器": 机器, "条目": a.行,
                                                "来源": "board_cli收工"})
    if 码 not in (200, 201):
        print(f"[失败] 交接写入：{码} {r.get('错误', r)}")
        return 1
    print(f"[交接] 已登记（#{r.get('id')}）")
    # 会话注销（308i 会话粒度·与 intent.py 同键）
    sys.path.insert(0, str(脚本目录))
    try:
        import intent
        码2, r2 = 调服务("POST", "/api/intent_release",
                        {"机器": intent.机器名(), "对话id": intent.会话id()})
        print("[意图] 会话已注销" if 码2 == 200 else f"[意图] 注销跳过（{码2}）")
    except Exception as e:      # 注销失败不拦收工（看板 2h 惰性清兜底）
        print(f"[意图] 注销跳过（{e}）")
    return 0


def cmd_交接(a) -> int:
    机器 = 机器名(a.机器)
    if not 机器 or not a.行:
        print("[失败] 须 --行 条目（机器识别不出时加 --机器）")
        return 2
    码, r = 调服务("POST", "/api/handoff_add", {"机器": 机器, "条目": a.行,
                                                "来源": "board_cli"})
    if 码 not in (200, 201):
        print(f"[失败] {码} {r.get('错误', r)}")
        return 1
    print(f"[交接] 已登记（#{r.get('id')}）")
    return 0


def cmd_教训(a) -> int:
    if not a.标题:
        print("[失败] --标题 必填（权重可写在标题里「权重 N」或 --权重 显式）")
        return 2
    码, r = 调服务("POST", "/api/lesson_add",
                  {"标题": a.标题, "正文": a.正文 or "", "权重": a.权重,
                   "标注": a.标注 or "活跃", "来源": "board_cli"})
    if 码 not in (200, 201):
        print(f"[失败] {码} {r.get('错误', r)}")
        return 1
    动作 = "幂等跳过（同标题已在库）" if r.get("跳过") else f"已登记（#{r.get('id')}）"
    print(f"[教训] {动作}")
    return 0


def cmd_教训索引(a) -> int:
    try:
        键 = f"high_weight={a.高权重}" if a.高权重 else "index=1"
        码, r = 调服务("GET", f"/api/lessons?{键}")
        落缓存("lessons", r)
    except OSError as e:
        print(f"[黄] 服务不可达（{e}）——读本地缓存")
        缓 = 脚本目录 / "board_cache.json"
        r = json.loads(缓.read_text(encoding="utf-8")).get("lessons", {}) \
            if 缓.exists() else {}
    for t in r.get("条目们", []):
        尾 = f"〔{t['标注']}〕" if t.get("标注", "活跃") != "活跃" else ""
        print(f"- #{t['id']} {t['标题']}（权重 {t['权重']}）{尾}")
    return 0


def cmd_教训看(a) -> int:
    码, r = 调服务("GET", "/api/lessons")
    目 = next((t for t in r.get("条目们", []) if t["id"] == a.id), None)
    if not 目:
        print(f"[失败] 无 id={a.id}")
        return 1
    print(f"# {目['标题']}（权重 {目['权重']}·{目['时刻']}）\n\n{目['正文']}")
    return 0


def cmd_用例(a) -> int:
    if a.态 not in ("正例", "边界例", "负例"):
        print("[失败] 三态须为 正例|边界例|负例")
        return 2
    用例们 = ",".join(c.strip() for c in a.用例名.split(",") if c.strip())
    if not 用例们:
        print("[失败] 用例名必填（逗号分隔可批量）")
        return 2
    码, r = 调服务("GET", "/api/coverage")
    if 码 != 200:
        print(f"[失败] 覆盖表拉取 {码}——服务不可达时勿盲写（防覆盖他机同列）")
        return 1
    旧 = next((u for u in r["单元们"] if u["单元ID"] == a.单元ID), None)
    现值 = 旧[a.态] if 旧 else ""
    新列 = f"{现值},{用例们}" if 现值 else 用例们
    体 = {"单元ID": a.单元ID, "标题": 旧["标题"] if 旧 else a.单元ID}
    for k in ("正例", "边界例", "负例"):
        体[k] = 新列 if k == a.态 else (旧[k] if 旧 else "")
    码, r = 调服务("POST", "/api/coverage_update", 体)
    if 码 != 200:
        print(f"[失败] {码} {r.get('错误', r)}")
        return 1
    print(f"[用例] 单元 {a.单元ID}·{a.态} 已归入（该列现 {len(r['单元'][a.态].split(','))} 例）")
    return 0


def cmd_豁免(a) -> int:
    if not a.用例名 or not a.理由:
        print("[失败] 豁免须 用例名+理由（防漏映射逃逸·check_spec_coverage --ci 口径）")
        return 2
    码, r = 调服务("POST", "/api/coverage_exempt_add",
                  {"用例": a.用例名, "理由": a.理由})
    if 码 != 200:
        print(f"[失败] {码} {r.get('错误', r)}")
        return 1
    print(f"[豁免] 已登记 {r['登记数']} 例")
    return 0


def cmd_覆盖(a) -> int:
    码, r = 调服务("GET", "/api/coverage")
    落缓存("coverage", r)
    if 码 != 200:
        print(f"[失败] {码}")
        return 1
    们 = r["单元们"]
    print(f"覆盖单元 {len(们)}·豁免 {len(r['豁免们'])}")
    for u in 们:
        缺 = [k for k in ("正例", "边界例", "负例") if not u[k]]
        print(f"  {u['单元ID']:<10} {u['标题'][:24]:<26}"
              f"{'缺口:' + ','.join(缺) if 缺 else '✓'}")
    return 0


def _读文件(路径: str) -> str:
    return Path(路径).read_text(encoding="utf-8")


def cmd_裁决(a) -> int:
    """裁决项上传（403）——--批 批量文件 或 单件四件套。已裁决项 409=跳过不算失败。"""
    if a.批:
        件们 = json.loads(_读文件(a.批))
        if not isinstance(件们, list):
            件们 = [件们]
    elif a.号 and a.标题 and a.讲解文件 and a.选项文件:
        件们 = [{"号": a.号, "标题": a.标题, "优先级": a.优先级,
                 "讲解": _读文件(a.讲解文件),
                 "选项们": json.loads(_读文件(a.选项文件))}]
    else:
        print("[失败] 须 --批 <件们.json> 或 --号/--标题/--讲解文件/--选项文件 四件套")
        return 2
    登记 = 跳过 = 败 = 0
    for 件 in 件们:
        码, r = 调服务("POST", "/api/adjudication_add", {**件, "来源": "board_cli"})
        if 码 in (200, 201):
            登记 += 1
            print(f"[裁决] #{件.get('号')} {r.get('动作', '已登记')}")
        elif 码 == 409:
            跳过 += 1
            print(f"[裁决] #{件.get('号')} 跳过（已裁决锁定）")
        else:
            败 += 1
            print(f"[失败] #{件.get('号')} {码} {r.get('错误', r)}")
    print(f"[裁决] 登记 {登记}·跳过 {跳过}·失败 {败}")
    return 1 if 败 else 0


def cmd_裁决看(a) -> int:
    码, r = 调服务("GET", "/api/adjudications")
    落缓存("adjudications", r)
    if 码 != 200:
        print(f"[失败] {码}")
        return 1
    们 = r["裁决项们"]
    if not a.全部:
        们 = [x for x in 们 if x["状态"] == "待裁决"]
    print(f"裁决项 {len(们)} 件"
          + ("" if a.全部 else "（待裁决·--全部 看含已决）"))
    for x in 们:
        推荐 = next((o["键"] for o in x["选项们"] if o.get("推荐")), "")
        尾 = (f"→ 批{x['最新结论']}" if x["状态"] == "已裁决"
              else f"推荐={推荐 or '—'}")
        print(f"  #{x['号']} [{x['优先级']}] {x['状态']} {x['标题'][:40]} {尾}")
    return 0


def main() -> int:
    import argparse
    p = argparse.ArgumentParser(description="文档资源统一 CLI（384）")
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("收工", help="交接追加+会话注销（收工一条命令）")
    s.add_argument("--行", required=True)
    s.add_argument("--机器", default="")
    s.set_defaults(f=cmd_收工)

    s = sub.add_parser("交接", help="只写交接")
    s.add_argument("--行", required=True)
    s.add_argument("--机器", default="")
    s.set_defaults(f=cmd_交接)

    s = sub.add_parser("教训", help="登记教训")
    s.add_argument("--标题", required=True)
    s.add_argument("--正文", default="")
    s.add_argument("--权重", type=int, default=0)
    s.add_argument("--标注", default="活跃")
    s.set_defaults(f=cmd_教训)

    s = sub.add_parser("教训索引", help="一行索引扫读")
    s.add_argument("--高权重", type=int, default=0)
    s.set_defaults(f=cmd_教训索引)

    s = sub.add_parser("教训看", help="看全文")
    s.add_argument("id", type=int)
    s.set_defaults(f=cmd_教训看)

    s = sub.add_parser("用例", help="E2E 用例归入覆盖单元")
    s.add_argument("单元ID")
    s.add_argument("态")
    s.add_argument("用例名")
    s.set_defaults(f=cmd_用例)

    s = sub.add_parser("豁免", help="豁免用例登记")
    s.add_argument("用例名")
    s.add_argument("理由")
    s.set_defaults(f=cmd_豁免)

    s = sub.add_parser("覆盖", help="覆盖矩阵摘要")
    s.set_defaults(f=cmd_覆盖)

    s = sub.add_parser("裁决", help="裁决项上传（AI 侧·批量或单件）")
    s.add_argument("--批", default="")
    s.add_argument("--号", default="")
    s.add_argument("--标题", default="")
    s.add_argument("--优先级", default="P2")
    s.add_argument("--讲解文件", default="")
    s.add_argument("--选项文件", default="")
    s.set_defaults(f=cmd_裁决)

    s = sub.add_parser("裁决看", help="裁决项一览")
    s.add_argument("--全部", action="store_true")
    s.set_defaults(f=cmd_裁决看)

    a = p.parse_args()
    try:
        return a.f(a)
    except OSError as e:
        print(f"[失败] 看板服务不可达（{e}）——写操作宁停不撞，恢复后重试")
        return 1


if __name__ == "__main__":
    sys.exit(main())
