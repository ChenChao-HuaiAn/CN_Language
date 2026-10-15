#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""CN 官网数据条统计器（844·2026-10-15）——官网首页数据随 develop 自动统计。

痛点：官网「单元测试 1414 / E2E 用例 590+」手写死值，演进后遗漏（实测已漂到
1422/841）。本脚本在 TX_01 上跑，从 gitcode develop 最新树统计真值，落
/var/www/cn-language/assets/stats.json；官网 site.js 异步拉取刷新，拉取失败
保留 HTML 静态兜底值。

统计口径（与门禁语义对应·诚实原则）：
  unit_tests  tests/unit/**.cpp 中 `^\s*TEST\(` 宏计数（单测注册唯一途径）
  e2e_cases   tests/e2e/<编号>_<名称>/主.cn 的一级用例目录数（多文件入口必名 主.cn）

触发：TX_01 crontab 每 6 小时 + deploy_website.sh 发布后立即触发（防整树覆盖
rm -rf 后 stats.json 缺失窗口）。

用法：
  python3 website_stats.py                 # TX_01 正式跑（缓存仓+sudo 落官网目录）
  python3 website_stats.py --repo <路径>   # 本地已有仓库直接统计（--stdout 联调）
  python3 website_stats.py --stdout        # 结果只打印不落盘
"""
import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone, timedelta
from pathlib import Path

GIT_URL = "https://gitcode.com/ChenChao_GitCode/CN_Language.git"
CACHE_REPO = Path.home() / "cn_stats_repo"
OUT_REMOTE = "/var/www/cn-language/assets/stats.json"
LOG = Path.home() / "cn_stats.log"


def 跑(args, cwd=None, 检查=True):
    """子进程跑命令，输出 UTF-8 文本；失败抛 RuntimeError（由调用方分字段处置）。"""
    r = subprocess.run(args, cwd=cwd, capture_output=True, timeout=600)
    if 检查 and r.returncode != 0:
        raise RuntimeError("%s 失败 rc=%d: %s" % (" ".join(args), r.returncode,
                                                  r.stderr.decode("utf-8", "replace")[-300:]))
    return r.stdout.decode("utf-8", "replace")


def 取仓库():
    """缓存仓不存在则浅克隆；存在则 fetch+reset 到 origin/develop 最新。"""
    if not (CACHE_REPO / ".git").exists():
        print("缓存仓不存在，浅克隆 %s → %s ..." % (GIT_URL, CACHE_REPO))
        跑(["git", "clone", "--depth", "1", GIT_URL, str(CACHE_REPO)])
    else:
        跑(["git", "fetch", "--depth", "1", "origin", "develop"], cwd=CACHE_REPO)
        跑(["git", "reset", "--hard", "FETCH_HEAD"], cwd=CACHE_REPO)
    return CACHE_REPO


def 数单测(repo: Path) -> int:
    pat = re.compile(r"^\s*TEST\(", re.M)
    n = 0
    for root, _, files in os.walk(repo / "tests" / "unit"):
        for fn in files:
            if fn.endswith(".cpp"):
                n += len(pat.findall((Path(root) / fn).read_text(encoding="utf-8",
                                                                errors="replace")))
    return n


def 数用例(repo: Path) -> int:
    e2e = repo / "tests" / "e2e"
    return sum(1 for d in e2e.iterdir()
               if d.is_dir() and (d / "主.cn").is_file())


def 主():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", help="直接用本地已有仓库统计（跳过 clone/fetch·本地联调）")
    ap.add_argument("--stdout", action="store_true", help="只打印 JSON 不落盘")
    a = ap.parse_args()

    stats = {}
    失败 = []
    try:
        repo = Path(a.repo) if a.repo else 取仓库()
        stats["unit_tests"] = 数单测(repo)
        stats["e2e_cases"] = 数用例(repo)
        stats["unit_tests_口径"] = "tests/unit TEST 宏计数"
        stats["e2e_cases_口径"] = "tests/e2e 含 主.cn 的用例目录"
    except Exception as e:
        失败.append("统计: %s" % e)

    tz = timezone(timedelta(hours=8))
    stats["generated_at"] = datetime.now(tz).strftime("%Y-%m-%d %H:%M (UTC+8)")
    out = json.dumps(stats, ensure_ascii=False, indent=1) + "\n"

    if a.stdout:
        print(out, end="")
        return 0

    # 落盘：tmp 文件 + sudo install（/var/www 归 root·与 deploy_website.sh 同权限面）
    tmp = Path("/tmp/cn_stats_%d.json" % os.getpid())
    tmp.write_text(out, encoding="utf-8")
    try:
        跑(["sudo", "install", "-m", "644", str(tmp), OUT_REMOTE])
    except Exception as e:
        失败.append("落盘: %s" % e)
    finally:
        tmp.unlink(missing_ok=True)

    if 失败:
        for f in 失败:
            print("✗ " + f, file=sys.stderr)
        return 1
    print("✓ stats.json 已更新 → %s\n%s" % (OUT_REMOTE, out), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(主())
