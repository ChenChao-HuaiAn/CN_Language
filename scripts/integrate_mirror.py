#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""integrate GitHub 镜像补推（#1403 抽出·单一归属）。

背景（2026-10-23 用户报「github 一直推送不上」实证）：
  - GitHub develop 停在 5ec6a0dd8（10-12），gitcode develop 已在其后 648 提交；
    GitHub 唯一独有提交的补丁经 `git cherry -v` 判定为 `-`（已等价存在于 develop）
    ——10-12 后 develop 经历 rebase，旧历史 hash 全变，ff 推送永久 non-fast-forward。
  - 批路径原 `git push github develop` 推本地 develop 引用（陈旧），而非当前 tip。
本模块口径：
  ①补推(tip) 恒推 `<tip>:develop`（与 solo 原 HEAD:develop 同口径）；
  ②失败打印 cherry 判定 + 一次性对齐命令；**不自动 force**（覆盖远端历史须人工授权）；
  ③推送/诊断可注入运行器（selftest 纯函数面）。
"""
import subprocess

镜像远程 = "github"
集成分支 = "develop"


def _运行(命令, 超时=120):
    return subprocess.run(命令, capture_output=True, text=True,
                          encoding="utf-8", errors="replace", timeout=超时)


def _输出(命令):
    try:
        结果 = _运行(命令, 超时=60)
    except (OSError, subprocess.TimeoutExpired):
        return ""
    return (结果.stdout or "").strip() if 结果.returncode == 0 else ""


def 镜像诊断(独有行们, 远端sha, tip):
    """纯函数：`git cherry -v` 输出行 + 远端 sha + 目标 tip → 诊断文本行（selftest 面）。"""
    if not 独有行们:
        return ["  镜像推送被拒但无独有提交（疑似并发竞争/分支保护规则）——"
                "稍后重试或检查仓库设置"]
    已含 = all(行.startswith("-") for 行 in 独有行们)
    if 已含:
        return [
            "  镜像历史分叉：远端独有 %d 提交的补丁均已等价存在于 %s"
            "（cherry 全为 '-'·rebase 旧线）" % (len(独有行们), tip[:9]),
            "  一次性对齐（需人工授权·内容零丢失）：",
            "    git push --force-with-lease=%s:%s %s %s:%s"
            % (集成分支, 远端sha, 镜像远程, tip, 集成分支),
        ]
    return ["  镜像远端独有 %d 提交含未合入内容（cherry 存在 '+' 行）——"
            "禁 force，需人工合并后再补推" % len(独有行们)]


def 补推镜像(tip, 运行器=_运行, 输出器=_输出):
    """推送 `<tip>:develop` 到镜像；失败打印诊断（不自动 force）。返回 bool。"""
    结果 = 运行器(["git", "push", 镜像远程, "%s:%s" % (tip, 集成分支)])
    if 结果.returncode == 0:
        return True
    print("  [警告] github 镜像补推失败——按惯例下次提交补推（不影响集成有效性）。")
    运行器(["git", "fetch", 镜像远程, 集成分支])       # 刷新远端引用供诊断（失败降级）
    远端sha = 输出器(["git", "rev-parse", "%s/%s" % (镜像远程, 集成分支)]) or "?"
    独有文本 = 输出器(["git", "cherry", "-v", tip, "%s/%s" % (镜像远程, 集成分支)])
    for 行 in 镜像诊断(独有文本.splitlines() if 独有文本 else [], 远端sha, tip):
        print(行)
    return False


def selftest():
    """纯函数自测（正反例）：分叉已含给 force-with-lease；含 '+' 禁 force；无独有不臆造。"""
    通过 = 失败 = 0

    def 签(名, 条件):
        nonlocal 通过, 失败
        if 条件:
            通过 += 1
            print("  [通过] %s" % 名)
        else:
            失败 += 1
            print("  [失败] %s" % 名)

    文 = "\n".join(镜像诊断(["- 5ec6a0dd8 补丁", "- aabbcc 补丁"], "5ec6a0dd8", "c442d242f"))
    签("分叉诊断：已含补丁（cherry 全 -）→ 给 force-with-lease 一次性命令",
       "force-with-lease=develop:5ec6a0dd8" in 文 and "c442d242f" in 文)
    文2 = "\n".join(镜像诊断(["+ 1234567 真独有"], "1234567", "c442d242f"))
    签("分叉诊断：存在 '+' 独有 → 禁 force/人工合并",
       "禁 force" in 文2 and "force-with-lease" not in 文2)
    文3 = "\n".join(镜像诊断([], "5ec6a0dd8", "c442d242f"))
    签("分叉诊断：无独有提交 → 保护规则/并发提示（不臆造 force）",
       "分支保护" in 文3 and "force" not in 文3)
    print("integrate_mirror selftest: %d 通过 / %d 失败" % (通过, 失败))
    return 0 if 失败 == 0 else 1


if __name__ == "__main__":
    raise SystemExit(selftest())
