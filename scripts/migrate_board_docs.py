#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""migrate_board_docs.py —— 共写文档一次性迁移（384·文档上服务器二期）。

交接.md / 项目记忆/教训.md / tests/e2e/coverage_map.md → 看板服务端
（board_docs 三资源·解析器=board_docs.解析交接文档/解析教训文档/解析覆盖文档）。
幂等：教训同标题服务端跳过·覆盖批量导入已占跳过·交接按已有条目文本去重。
用法：
  python scripts/migrate_board_docs.py --dry-run          # 只解析统计不发送
  python scripts/migrate_board_docs.py [--url http://TX_01:8301] [--令牌 <token>]
                                         [--根 <含源文件的树根>]   # 384 后本树源文件已删·
  令牌缺省读 scripts/queue_client.json 的「令牌」。               # 指向 develop 主树等
"""
from __future__ import annotations

import argparse
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

本树根 = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))
import board_docs as 文档  # noqa: E402  迁移解析与目标同源（单一实现）


def 调服务(url: str, 令牌: str, 方法: str, 路径: str, 体=None, 超时=30):
    请求 = urllib.request.Request(
        url.rstrip("/") + 路径,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None,
        method=方法)
    if 令牌 and 方法 == "POST":
        请求.add_header("Authorization", f"Bearer {令牌}")
    请求.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(请求, timeout=超时) as r:
            return r.status, json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return e.code, json.loads(e.read().decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return e.code, {}


def 读(相对: str) -> str:
    return (本树根 / 相对).read_text(encoding="utf-8")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--dry-run", action="store_true", help="只解析统计不发送")
    p.add_argument("--url", default="")
    p.add_argument("--令牌", default="")
    p.add_argument("--根", default="", help="源文件所在树根（缺省本树·384 删除后用 develop 主树）")
    a = p.parse_args()
    根 = Path(a.根) if a.根 else 本树根

    def 读源(相对: str) -> str:
        路径 = 根 / 相对
        if not 路径.exists():
            print(f"[失败] 源文件不存在：{路径}——384 已删除四文档，迁移须 --根 指向仍有源文件的树"
                  "（develop 主树 / git show HEAD~1 提取）")
            sys.exit(2)
        return 路径.read_text(encoding="utf-8")

    交 = 文档.解析交接文档(读源("交接.md"))
    教 = 文档.解析教训文档(读源(str(根 / "项目记忆" / "教训.md")))
    覆 = 文档.解析覆盖文档(读源(str(根 / "tests" / "e2e" / "coverage_map.md")))
    高全文 = sum(1 for t in 教 if t["权重"] >= 8 and t["正文"])
    print(f"[解析] 交接 {len(交)} 条（家 {sum(1 for x in 交 if x['机器']=='家机')}"
          f"/深 {sum(1 for x in 交 if x['机器']=='深度机')}"
          f"/单 {sum(1 for x in 交 if x['机器']=='单位机')}）·"
          f"教训 {len(教)} 条（高权重全文 {高全文}）·"
          f"覆盖单元 {len(覆['单元们'])}+豁免 {len(覆['豁免们'])}")
    if a.dry_run:
        print("[干跑] 未发送——对拍上述数字与源文件后去掉 --dry-run 真迁")
        return 0

    url = a.url
    令牌 = a.令牌
    if not url:
        cfg = json.loads((Path(__file__).resolve().parent / "queue_client.json")
                         .read_text(encoding="utf-8"))
        url = cfg["url"].replace(":8300", ":8301")   # 台账/文档资源在 board 服务
        令牌 = 令牌 or cfg.get("令牌", "")
    码, r = 调服务(url, 令牌, "GET", "/api/board")
    if 码 != 200:
        print(f"[失败] 服务不可达 {url}（码 {码}）——先部署 384 版 board_service")
        return 1

    # 交接：按已有条目文本去重（幂等）
    码, r = 调服务(url, 令牌, "GET", "/api/handoff?limit=200")
    已有 = {t["条目"] for t in r.get("条目们", [])}
    新交 = [x for x in 交 if x["条目"] not in 已有]
    跳交 = len(交) - len(新交)
    for i, x in enumerate(新交):
        码, r = 调服务(url, 令牌, "POST", "/api/handoff_add",
                      {"机器": x["机器"], "条目": x["条目"], "来源": "迁移交接md"})
        if 码 not in (200, 201):
            print(f"[失败] 交接第 {i+1} 条：{码} {r}")
            return 1
    # 教训：同标题服务端幂等跳过
    教跳 = 0
    for t in 教:
        码, r = 调服务(url, 令牌, "POST", "/api/lesson_add",
                      {"标题": t["标题"], "正文": t["正文"],
                       "权重": t["权重"], "来源": "迁移教训md"})
        if 码 not in (200, 201):
            print(f"[失败] 教训「{t['标题'][:30]}」：{码} {r}")
            return 1
        教跳 += 1 if r.get("跳过") else 0
    # 覆盖：批量幂等导入（59 单元 231 行一次发）
    码, r = 调服务(url, 令牌, "POST", "/api/coverage_import", 覆)
    if 码 != 200:
        print(f"[失败] 覆盖导入：{码} {r}")
        return 1
    print(f"[迁移完成] 交接 新{len(新交)}/跳{跳交}·教训 新{len(教)-教跳}/跳{教跳}"
          f"·覆盖 {r.get('导入')}+豁免{r.get('豁免导入')}"
          f"（跳 {r.get('跳过')}+{r.get('豁免跳过')}）")
    码, r = 调服务(url, 令牌, "GET", "/api/coverage")
    print(f"[复核] 覆盖单元在库 {len(r['单元们'])}·豁免在库 {len(r['豁免们'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
