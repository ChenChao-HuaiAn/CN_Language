#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""CN 官网自动部署器（992·2026-10-17）——TX_01 每 10 分钟巡检 develop：
website/ 有变化才「服务器侧重新生成 HTML + 原子发布」到 /var/www/cn-language。

三件机制分工（同一发布目录，后到者胜）：
  - scripts/deploy_website.sh（844 前身）：手动整树发布（本地树 → TX_01），仍可用；
  - 本脚本：develop 自动同步通道——浅克隆（独立缓存仓 ~/cn_website_repo·与
    website_stats.py 的 ~/cn_stats_repo 分仓防 git 锁相撞）→ 分辨 sha → 服务器侧
    gen_docs.py 重生成（页脚带 develop <sha> 基线标注·992 件 2）→ 暂存+换名原子发布；
  - scripts/website_stats.py（844）：每 6h 统计真值；本脚本发布后同步调用再生
    stats.json（与 deploy_website.sh 同规·防整树覆盖后缺失窗口）。

幂等与失败无害：已部署 sha（~/cn_website_deployed_sha）相同=直接退出零副作用；
生成失败/复制失败=现网目录一字未动；换名切换失败=旧目录回滚。日志
~/cn_website_deploy.log（仅状态变化/发布/失败登记·无变化不刷屏）。

TX_01 部署：scp 本文件到 /home/ubuntu/ + crontab 每 10 分钟
  */10 * * * * python3 /home/ubuntu/website_auto_deploy.py >> /home/ubuntu/cn_website_deploy.log 2>&1

用法：
  python3 website_auto_deploy.py                  # TX_01 正式跑（cron 同款）
  python3 website_auto_deploy.py --verbose        # 无变化也打印（联调）
  python3 website_auto_deploy.py --repo <路径>    # 用本地已有仓库（跳过 clone/fetch）
  python3 website_auto_deploy.py --branch <名>    # 指定部署分支（默认 develop·联调）
  python3 website_auto_deploy.py --force          # sha 相同也强制重发（调试）
"""
import argparse
import subprocess
import sys
from datetime import datetime, timedelta, timezone
from pathlib import Path

GIT_URL = "https://gitcode.com/ChenChao_GitCode/CN_Language.git"
CACHE_REPO = Path.home() / "cn_website_repo"
TARGET_DIR = Path("/var/www/cn-language")
STAGING_DIR = Path("/var/www/cn-language.staging")
OLD_DIR = Path("/var/www/cn-language.old")
SHA_FILE = Path.home() / "cn_website_deployed_sha"
LOG = Path.home() / "cn_website_deploy.log"


def 记(文本, 回显=True):
    """带时间戳登录 ~/cn_website_deploy.log（cron 重定向 stdout 亦可见）。"""
    时 = datetime.now(timezone(timedelta(hours=8))).strftime("%Y-%m-%d %H:%M:%S")
    行 = "%s %s" % (时, 文本)
    with LOG.open("a", encoding="utf-8") as f:
        f.write(行 + "\n")
    if 回显:
        print(行, flush=True)


def 跑(args, cwd=None, 检查=True, 超时=900):
    """子进程跑命令：UTF-8 文本；检查=True 且 rc≠0 抛 RuntimeError。"""
    r = subprocess.run(args, cwd=cwd, capture_output=True, timeout=超时)
    if 检查 and r.returncode != 0:
        raise RuntimeError("%s 失败 rc=%d: %s" % (
            " ".join(str(a) for a in args), r.returncode,
            r.stderr.decode("utf-8", "replace").strip()[-300:]))
    return r


def 取仓库(分支: str) -> Path:
    """缓存仓不存在则浅克隆；存在则 fetch+reset 到 origin/<分支> 最新（stats 同款）。"""
    if not (CACHE_REPO / ".git").exists():
        记("缓存仓不存在，浅克隆 %s（分支 %s）→ %s ..." % (GIT_URL, 分支, CACHE_REPO))
        跑(["git", "clone", "--depth", "1", "--branch", 分支, GIT_URL, str(CACHE_REPO)])
    else:
        跑(["git", "fetch", "--depth", "1", "origin", 分支], cwd=CACHE_REPO, 超时=600)
        跑(["git", "reset", "--hard", "-q", "FETCH_HEAD"], cwd=CACHE_REPO)
        跑(["git", "clean", "-fdq"], cwd=CACHE_REPO)
    return CACHE_REPO


def 发布(repo: Path):
    """暂存整树 + 换名原子切换（失败回滚旧目录）——现网全程可读。"""
    跑(["sudo", "rm", "-rf", str(STAGING_DIR), str(OLD_DIR)])
    跑(["sudo", "mkdir", "-p", str(STAGING_DIR)])
    跑(["sudo", "cp", "-a", str(repo / "website") + "/.", str(STAGING_DIR) + "/"])
    有现网 = 跑(["sudo", "test", "-e", str(TARGET_DIR)], 检查=False).returncode == 0
    if 有现网:
        跑(["sudo", "mv", str(TARGET_DIR), str(OLD_DIR)])
    try:
        跑(["sudo", "mv", str(STAGING_DIR), str(TARGET_DIR)])
    except RuntimeError:
        if 有现网:   # 尽力回滚：切换失败不留空窗
            subprocess.run(["sudo", "mv", str(OLD_DIR), str(TARGET_DIR)], timeout=60)
        raise
    if 有现网:
        跑(["sudo", "rm", "-rf", str(OLD_DIR)], 检查=False)   # 清理失败无害（下轮再清）


def 再生统计(repo: Path):
    """同步最新统计器到 ~/website_stats.py 并跑一轮（与 deploy_website.sh 同规）；
    失败只记日志不抛（6h cron 兜底·官网静态兜底值不受影响）。"""
    try:
        家副本 = Path.home() / "website_stats.py"
        家副本.write_bytes((repo / "scripts" / "website_stats.py").read_bytes())
        r = 跑(["python3", str(家副本)], 检查=False, 超时=600)
        if r.returncode == 0:
            记("stats.json 已再生（website_stats.py 同源同步）")
        else:
            记("⚠ stats.json 再生失败 rc=%d——6h cron 兜底（详见 ~/cn_stats.log）" % r.returncode)
    except Exception as e:
        记("⚠ 统计器同步/再生异常：%s（发布本体不受影响）" % e)


def 主() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--repo", help="用本地已有仓库（跳过 clone/fetch·联调）")
    ap.add_argument("--branch", default="develop", help="部署分支（默认 develop）")
    ap.add_argument("--verbose", action="store_true", help="无变化也打印")
    ap.add_argument("--force", action="store_true", help="sha 相同也强制重发")
    a = ap.parse_args()

    try:
        repo = Path(a.repo) if a.repo else 取仓库(a.branch)
        sha = 跑(["git", "-C", str(repo), "rev-parse", "HEAD"]).stdout.decode().strip()
        已部署 = SHA_FILE.read_text(encoding="utf-8").strip() if SHA_FILE.exists() else ""
        if sha == 已部署 and not a.force:
            if a.verbose:
                记("无变化（%s）——跳过" % sha[:10])
            return 0
        记("发现新 %s %s（上次=%s）——生成+发布" % (a.branch, sha[:10],
                                            已部署[:10] or "无"))
        生成 = 跑(["python3", "website/tools/gen_docs.py"], cwd=repo, 检查=False, 超时=600)
        if 生成.returncode != 0:
            记("✗ gen_docs 生成失败 rc=%d——现网未动（%s）" % (
                生成.returncode,
                生成.stderr.decode("utf-8", "replace").strip()[-200:] or "见 stdout"))
            return 1
        发布(repo)
        记("✓ 已发布 %s → %s" % (sha[:10], TARGET_DIR))
        再生统计(repo)
        SHA_FILE.write_text(sha + "\n", encoding="utf-8")
        记("已部署 sha 记录更新：%s" % sha[:10])
        return 0
    except Exception as e:
        记("✗ 部署失败（现网保持原状）：%s" % str(e)[:400])
        return 1


if __name__ == "__main__":
    sys.exit(主())
