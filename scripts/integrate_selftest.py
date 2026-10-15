#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""integrate.py 自测面（637 拆出·2026-10-14）——纯函数正反例（CI 式·无 git 副作用）。

拆分缘由：integrate.py 存量超 1000 行红线（单一职责·AGENTS §3）——自测是独立职责
（CI 式断言面），拆出后 integrate.py 新增功能不再膨胀自测体；本模块被 integrate.py
`--selftest` 延迟导入（函数内 import——彼时 integrate 模块已加载完毕，无循环导入）。

场景索引：
  ⓪ 伞批销账行解析（392）      ① 时刻解析
  ② 队列加行/幂等/改状态/清行    ③ 临时用例拦截（1008/562）
  ④ 预验结果解析（1008）+池轮询收场（562）
  ⑤ 预验前置检查（637：①rebase 新鲜 ②产物清零）
  ⑥ 预验绿复用判定（830：源分支+内容键）  ⑦ 池忙不误判降级（830）
  ⑧ 预验分支删除时机（863：非 --wait 哨兵保留·--wait 结论后照删）
  （原「③选批成员/失联判定」两段随 830 并行化退役删除——批主/拼车/接管面已废除）
"""
from __future__ import annotations

import types

import integrate
from integrate import (board_closeout, 解析时刻, 队列加行, 队列改状态, 队列清行们,
                       解析队列, 队列段标题, 临时用例检查,
                       解析预验结果, 池轮询预验, 已并入收场, 查可复用预验绿,
                       批写集, 预验前置检查, 云端预验门禁, 已入池哨兵)


def 自测() -> int:
    """--selftest：纯函数正反例（CI 式·无 git 副作用）。"""
    空板 = "# 看板\n\n## 各机小节\n"
    案例数 = 0

    def 断言(条件: bool, 说明: str) -> None:
        nonlocal 案例数
        案例数 += 1
        标记 = "✓" if 条件 else "✗"
        print(f"  [{标记}] {说明}")
        if not 条件:
            raise AssertionError(说明)

    # ⓪ 伞批销账行解析（392·board_closeout 纯函数面）
    断言(board_closeout.解析销账号("修复x\n[销账:372,374,375,376]")
         == ["372", "374", "375", "376"], "销账行：全角/半角逗号混合提取")
    断言(board_closeout.解析销账号("立 379·让位改 372") == [],
         "销账行：无登记不销（「提及≠销账」铁律）")
    断言(board_closeout.解析销账号("[销账:308a、316]")[0] == "308a",
         "销账行：字母后缀号+顿号分隔")

    # ① 时刻解析
    断言(解析时刻("09-30 14:05") is not None, "时刻解析：正常「09-30 14:05」")
    断言(解析时刻("2026-09-30") is None, "时刻解析：坏格式拒绝（非 MM-DD HH:MM）")
    断言(解析时刻("99-99 99:99") is None, "时刻解析：越界值拒绝")

    # ② 队列加行/幂等/改状态/清行
    板1 = 队列加行(空板, {"分支": "任务/895", "基线": "aaaa", "时刻": "09-30 14:00",
                        "摘要": "src/ir 两文件", "状态": "排队"})
    断言(len(解析队列(板1)) == 1, "加行：段不存在时建段且行可解析")
    断言(队列段标题 in 板1, "加行：段标题按规范落盘")
    板2 = 队列加行(板1, {"分支": "任务/895", "基线": "aaaa", "时刻": "09-30 14:10",
                        "摘要": "重复报名", "状态": "排队"})
    断言(解析队列(板2) == 解析队列(板1), "加行幂等：同分支重复报名不产生第二行")
    板3 = 队列加行(板2, {"分支": "任务/911", "基线": "bbbb", "时刻": "09-30 14:06",
                        "摘要": "scripts", "状态": "排队"})
    断言(len(解析队列(板3)) == 2, "加行：第二成员入段")
    板4 = 队列改状态(板3, "任务/895", "集成中")
    断言([x for x in 解析队列(板4) if x["分支"] == "任务/895"][0]["状态"] == "集成中",
         "改状态：指定行状态切换")
    断言(队列改状态(板4, "任务/999", "集成中") == 板4, "改状态：未知分支原文不动")
    板5 = 队列清行们(板4, ["任务/895"])
    断言(len(解析队列(板5)) == 1, "清行：销账移除指定分支行")
    断言(队列段标题 in 板5, "清行后段壳保留（三机共见结构稳定）")

    # ③（原「选批成员」段随 830 并行化退役删除——成员恒=自己·无批主无拼车）

    # ④ 临时用例拦截（1008 纪律 2·562 起注入树像=纯函数面·不再依赖真树状态）
    断言(临时用例检查(["src/a.cpp", "tests/e2e/501_正常用例/x.cn"]) is None,
         "tmp 拦截：正常用例名与非 e2e 路径放行")
    树像562 = {"HEAD:tests/e2e/990tmp_128最小/x.cn", "HEAD:tests/e2e/990Tmp_测试/x.cn",
               "HEAD:tests/e2e/501_x/TMP说明.md"}
    查树562 = lambda 修订, 路径: f"{修订}:{路径}" in 树像562
    断言(临时用例检查(["tests/e2e/990tmp_128最小/x.cn"], 查树562) is not None,
         "tmp 拦截：tmp 目录名拒绝（注入树像）")
    断言(临时用例检查(["tests/e2e/990Tmp_测试/x.cn"], 查树562) is not None,
         "tmp 拦截：大小写不敏感（Tmp·注入树像）")
    断言(临时用例检查(["tests/e2e/501_x/TMP说明.md"], 查树562) is None,
         "tmp 拦截：tmp 仅判用例目录名（路径第三段）——用例内文件名放行")
    断言(临时用例检查(["docs/tmp笔记.md", "src/tmp.cpp"], 查树562) is None,
         "tmp 拦截：e2e 外路径放行")

    # ⑤ 预验结果解析（1008）
    绿, 摘 = 解析预验结果('{"绿": true, "总秒": 800, "日志": "run_x.log",'
                          '"步骤": {"构建": {"rc": 0}}, "日志尾部": []}')
    断言(绿 and "run_x.log" in 摘, "预验解析：绿 JSON 判绿+日志名入摘要")
    红, 摘2 = 解析预验结果('{"绿": false, "总秒": 2000, "日志": "run_y.log",'
                           '"步骤": {"构建": {"rc": 0}, "e2e": {"rc": 1}},'
                           '"日志尾部": ["✗ 524_条件编译双编译: x"]}')
    断言(not 红 and "e2e" in 摘2 and "524" in 摘2, "预验解析：红 JSON 判红+红步骤与尾部入摘要")
    断言(not 解析预验结果("非JSON文本")[0], "预验解析：坏 JSON 判红不炸")
    断言(临时用例检查(批写集([{"分支": "任务/家机-9-x"}], "aaaa")) is None,
         "tmp 拦截：批写集空路径不越界")

    # ⑤b 池轮询「已取消」即时收场（562 修复·审查 P1-1）——注入式纯函数面（不碰真服务/真树）
    注入状态562b = lambda 路径: {"预验任务": [{"分支": "ci/预验-deadbeef00", "状态": "已取消",
                                              "认领者": None}]}
    拿到b, 原因b = 池轮询预验("ci/预验-deadbeef00", "d" * 40, [],
                             已含链顶查=lambda 链顶: True, 状态查=注入状态562b, 轮询间隔=0)
    断言(拿到b and 原因b is None,
         "562 池见已取消·develop 已含链顶=即时放行收场（不白等 75 分钟·不降级重跑全量）")
    拿到b, 原因b = 池轮询预验("ci/预验-deadbeef00", "d" * 40, [],
                             已含链顶查=lambda 链顶: False, 状态查=注入状态562b, 轮询间隔=0)
    断言(拿到b and 原因b is not None and "rebase" in 原因b,
         "562 池见已取消·develop 未含链顶=即时拦截（本链过时·给 rebase 出路）")
    断言(已并入收场("x", 已含链顶查=lambda s: True) == (True, None) and
         已并入收场("x", 已含链顶查=lambda s: False)[1] is not None,
         "562 已并入收场：已含链顶=放行/未含=拦截（入队被拒同族共用同一判定）")

    # ⑥（原「失联判定基准」段随 830 并行化退役删除——无批主无接管）

    # ⑦ 预验前置检查（637 用户立法·与单人封批同批落地：预验前必须满足两前置）
    断言(预验前置检查("c" * 40, "b" * 40, 祖先查询=lambda 基, 孙: True) is None,
         "637 前置①：链顶含当时最新 develop（祖先关系成立）→放行预验")
    前置拒 = 预验前置检查("c" * 40, "b" * 40, 祖先查询=lambda 基, 孙: False)
    断言(前置拒 is not None and "①" in 前置拒 and "rebase" in 前置拒,
         "637 前置①：过时链（不含最新 develop）→拒绝预验（给 rebase 出路）")
    前置拒 = 预验前置检查("c" * 40, "b" * 40, 祖先查询=lambda 基, 孙: True,
                         产物残留查询=lambda: ["target/build/cn",
                                               "target/audit2/v2p_linuxx64"])
    断言(前置拒 is not None and "②" in 前置拒 and "v2p" in 前置拒,
         "637 前置②：历史编译产物残留（构建树+v2p 指纹缓存）→拒绝预验（干净构建立法）")
    断言(预验前置检查("c" * 40, "b" * 40, 祖先查询=lambda 基, 孙: True,
                      产物残留查询=lambda: []) is None,
         "637 前置②：产物已清零→放行预验")

    # ⑧ 预验分支删除时机（863 修复·monkeypatch 假 git/网络面·测毕恢复·零副作用）：
    #   非 --wait 哨兵路径=入池即返，runner 认领后首步 fetch 预验分支——删=同步 fatal 被踢
    #   （746/764/758/644/728f 五连实录）；--wait 拿到结论后照删（收口语义不回归对照组）。
    假sha863 = "e" * 40
    真绑定863 = {名: getattr(integrate, 名) for 名 in
                 ("运行", "输出", "服务调用", "读CI配置", "查可复用预验绿",
                  "池轮询预验", "subprocess")}
    try:
        调用账 = []

        def 假运行(命令, **_):
            调用账.append(list(命令))
            回 = types.SimpleNamespace(returncode=0, stdout="", stderr="")
            return 回

        integrate.运行 = 假运行
        integrate.输出 = lambda 命令: 假sha863          # rev-parse develop/内容键 全假 sha
        integrate.服务调用 = lambda 路径, 数据=None: (
            {"ok": True} if "task_enqueue" in 路径 else {"预验任务": [], "池忙闲": {}})
        integrate.读CI配置 = lambda: {"ssh": "假机", "目录": "/tmp"}
        integrate.查可复用预验绿 = lambda 源, 键, 状态查=None: False
        integrate.池轮询预验 = lambda *a, **kw: (True, None)   # --wait 对照：立即拿到绿结论
        integrate.subprocess = types.SimpleNamespace(run=lambda *a, **kw: None)  # ssh pkill 假执行

        def 造参数(wait: bool):
            return types.SimpleNamespace(dry_run=False, remote="gitcode", wait=wait,
                                         allow_pool_red=[], pool_limit=[])

        回值 = 云端预验门禁(假sha863, 造参数(False), 源分支="任务/863", 等待=False)
        断言(回值 == 已入池哨兵, "863 非 --wait：入池成功返回哨兵（不挂机·不 push）")
        断言(not any("--delete" in c for c in 调用账),
             "863 非 --wait 哨兵路径：不触发远端预验分支删除（runner 待 fetch 消费·五连实录回归锚）")
        断言(any("refs/heads/ci/预验-" in " ".join(c) for c in 调用账),
             "863 非 --wait 哨兵路径：预验分支推送照常（入池前提不变）")

        调用账.clear()
        回值 = 云端预验门禁(假sha863, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None, "863 --wait 对照：池轮询拿到绿结论放行（None）")
        断言(any("--delete" in c for c in 调用账),
             "863 --wait 对照：拿到结论后照删远端预验分支（收口语义不回归）")
    finally:
        for 名, 值 in 真绑定863.items():
            setattr(integrate, 名, 值)

    print(f"\n[selftest] {案例数} 例全过 ✓")
    return 0
