#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""integrate.py 自测面（637 拆出·2026-10-14）——纯函数正反例（CI 式·无 git 副作用）。

拆分缘由：integrate.py 存量超 1000 行红线（单一职责·AGENTS §3）——自测是独立职责
（CI 式断言面），拆出后 integrate.py 新增功能不再膨胀自测体；本模块被 integrate.py
`--selftest` 延迟导入（函数内 import——彼时 integrate 模块已加载完毕，无循环导入）。

976（状态机专项）覆盖矩阵——每状态转移至少一例（编号↔integrate.py 转移对照
见 docs/integrate状态机-976.md §8）：
  ⓪ 伞批销账行解析（392）                 ②b 解析队列读侧（v5 写侧退役后消费面）
  ①  时刻解析/② 看板写函数族 → 976 退役删除（函数与测试同步退场·注释留档）
  ③b 写集三点语义（1008）                 ④ 临时用例拦截（1008/562）
  ⑤ 预验结果解析（1008）                  ⑤b 池轮询「已取消」收场（562）
  ⑤c 池轮询全转移（976 补缺）：
     完成绿/完成红/点名全中放行/未点名硬拦/行 miss 重入队/3 连不可达/
     池忙继续等（830）/排队超时降级+取消（1026）/337 守限制/总超时
  ⑦ 预验前置检查（637 两前置）            ⑧+⑨ 云端预验门禁 mock 全路径：
     863 六例（哨兵保留/--wait 照删）/dry-run/830 复用绿/推分支失败/337 守卫/
     901 降级保留（绿/触发失败/入队响应丢失）/入队明确拒绝照删/562 已并入收场
  ⑩ 976 归纳件：E2E 红用例行解析/预验入队载荷构造
  ⑪ 868 预验红解析放行面核查：解析边界矩阵（非对象壳/坏字段/绿形态）+
     win/linux 复验面空清单硬红（722「空集合恒真」残留加固·点名机制不回归）
  （原「③选批成员/⑥失联判定」两段随 830 并行化退役删除——批主/拼车/接管面已废除）
"""
from __future__ import annotations

import json
import types

import integrate
from integrate import (board_closeout, 解析队列, 队列段标题, 临时用例检查,
                       解析预验结果, 池轮询预验, 已并入收场,
                       批写集, 预验前置检查, 云端预验门禁, 已入池哨兵,
                       解析E2E红用例行, 构造预验入队载荷, 三点间改动, 全量门禁)


def 自测() -> int:
    """--selftest：纯函数正反例（CI 式·无 git 副作用）。"""
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

    # ① 时刻解析（976：函数随看板写侧退役删除——原 3 例同步退场，读侧按字符串序消费时刻）

    # ② 队列加行/幂等/改状态/清行（976：v5 直推废档收尾——写侧函数族与 8 例同步退役）

    # ②b 解析队列（读侧保留面）：v5 写侧退役后唯一看板文本消费点（队列读 双源合并/--status）
    板q = ("# 看板\n\n" + 队列段标题 + "\n\n"
           "| 分支 | 基线 | 报名时刻 | 写集摘要 | 状态 |\n|---|---|---|---|---|\n"
           "| 任务/895 | aaaa | 09-30 14:00 | src/ir 两文件 | 排队 |\n")
    行们q = 解析队列(板q)
    断言(len(行们q) == 1 and 行们q[0]["分支"] == "任务/895" and 行们q[0]["状态"] == "排队",
         "解析队列：段内表行提取（列序 分支/基线/时刻/摘要/状态）")
    断言(解析队列("无段文本") == [], "解析队列：无段=空清单不炸")

    # ③（原「选批成员」段随 830 并行化退役删除——成员恒=自己·无批主无拼车）

    # ③b 写集三点语义（1008）：命令形态锚——回退两点 diff 即测试红（#993 虚大回归锚·
    #   注入 输出=纯函数面·零真 git）。语义验证见提交信息（落后基线场景实测：176→3 文件）。
    真输出写集 = integrate.输出
    try:
        账写集: list = []
        integrate.输出 = lambda 命令: (账写集.append(list(命令)), "")[1]
        断言(三点间改动("基假", "终假") == [], "写集：空输出→空清单不炸")
        断言(any("基假...终假" in " ".join(c) for c in 账写集),
             "写集：三点 diff 形态（<起点>...<终点>=merge-base 语义·1008 防虚大回归）")
    finally:
        integrate.输出 = 真输出写集

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
    # ⑤ 扩展（868 边界矩阵）：非对象壳/坏字段/绿形态三类解析失败全判红（宁拦勿放）
    断言(not 解析预验结果('"裸字符串壳"')[0], "868 预验解析：合法 JSON 非对象（字符串壳）=判红不炸")
    断言(not 解析预验结果("[1, 2]")[0], "868 预验解析：数组壳=判红不炸（消 AttributeError 裸崩面）")
    断言(not 解析预验结果('{"绿": true, "步骤": "坏壳"}')[0],
         "868 预验解析：步骤字段形态坏=判红不炸（消 s.get 裸崩面）")
    断言(not 解析预验结果('{"绿": "false", "步骤": {"e2e": {"rc": 1}}}')[0],
         "868 预验解析：绿=字符串 false=判红（bool('false')=True 字符串假绿面清零）")
    断言(not 解析预验结果('{"绿": null, "步骤": {}}')[0], "868 预验解析：绿缺失/None=判红")
    断言(解析预验结果('{"绿": 1, "步骤": {}}')[0],
         "868 预验解析：绿=数值 1 兼容判绿（服务端 INTEGER 列回读形态）")
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

    # ⑤c 池轮询预验全转移（976 补缺）：完成绿/红/点名（216）·行 miss 重入队（337/830 字段随行）·
    #   3 连不可达·池忙继续等（830）·排队超时降级+取消（1026）·337 守限制·总超时——
    #   注入状态查+假 服务调用（详情/入队/取消面）·测毕恢复绑定（零真网络/零真 git）
    预验支 = "ci/预验-" + "a" * 10
    完成行 = [{"分支": 预验支, "状态": "完成", "认领者": "TX_02"}]
    绿详情 = {"绿": True, "总秒": 800, "日志": "r.log", "步骤": {"构建": {"rc": 0}}, "日志尾部": []}
    红详情 = {"绿": False, "总秒": 900, "日志": "r.log", "步骤": {"e2e": {"rc": 1}},
             "日志尾部": ["✗ 501_某用例: 失败"]}
    真服务调用池 = integrate.服务调用
    try:
        def 造假服务池(状态序列, 详情=None, 台账=None):
            def 假服务池(路径, 数据=None):
                if "task_enqueue" in 路径 or "task_cancel" in 路径:
                    if 台账 is not None:
                        台账.append({"路径": 路径, "数据": 数据})
                    return {"ok": True}
                if "task_result" in 路径:
                    return {"详情": 详情}
                return next(状态序列)   # 耗尽应抛 StopIteration=未预期轮次（测试显形非静默）
            return 假服务池

        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 绿详情)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0)
        断言(拿到c and 原因c is None, "池轮询：完成行+详情绿=拿到结论放行（绿链）")
        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 红详情)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0)
        断言(拿到c and 原因c is not None and "云端预验红" in 原因c,
             "池轮询：完成行+详情红=拿到结论拦截（红链）")
        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 红详情)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, ["501_某用例"], 轮询间隔=0)
        断言(拿到c and 原因c is None, "池轮询：红用例全命中点名=披露放行（216）")
        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 红详情)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, ["别用例"], 轮询间隔=0)
        断言(拿到c and 原因c is not None, "池轮询：存在未点名红=硬拦（216）")
        # 868（722「空集合恒真」同族核查锚）：红但 ✗ 清单未取得（空清单）——无论点名
        #   非空与否仍硬拦——放行面非空自证（若回归为「未点名为空即放行」此二例即红）
        红详情无清单 = {"绿": False, "总秒": 900, "日志": "r.log",
                       "步骤": {"e2e": {"rc": 1}}, "日志尾部": ["等待云端结果……"]}
        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 红详情无清单)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0)
        断言(拿到c and 原因c is not None,
             "868 池轮询：红但 ✗ 清单空=硬拦（空清单≠无红·不放行）")
        integrate.服务调用 = 造假服务池(iter([{"预验任务": 完成行, "池忙闲": {}}]), 红详情无清单)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, ["501_某用例"], 轮询间隔=0)
        断言(拿到c and 原因c is not None,
             "868 池轮询：红但 ✗ 清单空+点名非空=仍硬拦（空清单不许点名放行）")
        重入账单: list = []
        integrate.服务调用 = 造假服务池(iter([{"预验任务": [], "池忙闲": {}},
                                        {"预验任务": 完成行, "池忙闲": {}}]), 绿详情, 重入账单)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 源分支="任务/976",
                                认领限制="win-x64", 内容键="k" * 40, 轮询间隔=0)
        断言(拿到c and 原因c is None and len(重入账单) == 1
             and 重入账单[0]["数据"].get("内容键") == "k" * 40
             and 重入账单[0]["数据"].get("认领限制") == "win-x64",
             "池轮询：行 miss=防御性重入队（337/830 随行字段补带）")
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0, 状态查=lambda 路径: None)
        断言(拿到c is False and 原因c is None,
             "池轮询：TX_01 连续 3 轮不可达=未拿到·交调用方降级（不白熬总超时）")
        integrate.服务调用 = 造假服务池(iter([
            {"预验任务": [{"分支": 预验支, "状态": "排队"}],
             "池忙闲": {"池全忙": True, "注册实例": 3, "执行中": 3}},
            {"预验任务": 完成行, "池忙闲": {}}]), 绿详情)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0)
        断言(拿到c and 原因c is None, "池轮询：池全忙排队=继续等待不降级（830 池忙不误判）")
        取消账单: list = []
        integrate.服务调用 = 造假服务池(
            iter([{"预验任务": [{"分支": 预验支, "状态": "排队"}], "池忙闲": {}}] * 8),
            None, 取消账单)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0.01, 降级秒=0)
        断言(拿到c is False and 原因c is None
             and any("task_cancel" in 调["路径"] for 调 in 取消账单),
             "池轮询：排队超时（池有空闲）=取消池行+降级（1026 防双跑·d48b5c5f 同族）")
        取消账单2: list = []
        integrate.服务调用 = 造假服务池(iter([
            {"预验任务": [{"分支": 预验支, "状态": "排队"}], "池忙闲": {}},
            {"预验任务": [{"分支": 预验支, "状态": "排队"}], "池忙闲": {}},
            {"预验任务": 完成行, "池忙闲": {}}]), 绿详情, 取消账单2)
        拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 认领限制="win-x64",
                                轮询间隔=0.01, 降级秒=0)
        断言(拿到c and 原因c is None
             and not any("task_cancel" in 调["路径"] for 调 in 取消账单2),
             "池轮询：337 守限制（无 linux-x86_64）=跳过降级继续等（runner 上线即领）")
        真超时 = integrate.预验总超时分钟
        try:
            integrate.预验总超时分钟 = 0   # 0 注入使 while 零迭代=总超时口径（240 分钟原值）
            拿到c, 原因c = 池轮询预验(预验支, "a" * 40, [], 轮询间隔=0)
            断言(拿到c is False and 原因c is None, "池轮询：总超时=未拿到交调用方降级重跑")
        finally:
            integrate.预验总超时分钟 = 真超时
    finally:
        integrate.服务调用 = 真服务调用池

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

    # ⑧+⑨ 云端预验门禁 mock 全路径（monkeypatch 假 git/网络/subprocess 面·测毕恢复·零副作用）：
    #   ⑧ 863 六例（非 --wait 哨兵保留·--wait 结论后照删）；⑨ 976 扩展（dry-run/830 复用绿/
    #   推分支失败/337 守卫/901 降级保留/入队响应丢失保留/入队明确拒绝照删/562 已并入收场）。
    #   删除前提不变量（863 立·901 补全）：预验分支删除 ⟺ 池行已确认终态或从未建行。
    假SHA = "e" * 40
    真绑门禁 = {名: getattr(integrate, 名) for 名 in
               ("运行", "输出", "服务调用", "读CI配置", "查可复用预验绿",
                "池轮询预验", "subprocess")}
    调用账: list = []
    面 = {"复用绿": False, "池轮询": (True, None), "入队": {"ok": True}, "推失败": False}
    绿JSON门禁 = json.dumps({"绿": True, "总秒": 800, "日志": "b.log",
                          "步骤": {"同步": {"rc": 0}, "构建": {"rc": 0}}, "日志尾部": []},
                         ensure_ascii=False)
    try:
        def 假运行(命令, **_):
            调用账.append(list(命令))
            rc = 1 if (面["推失败"] and 命令[:2] == ["git", "push"]
                       and any("refs/heads/ci/预验-" in str(c) for c in 命令)) else 0
            return types.SimpleNamespace(returncode=rc, stdout="", stderr="")

        def 假subprocess运行(命令, **kw):
            文本 = " ".join(str(c) for c in 命令)
            if "pkill" in 文本:
                return types.SimpleNamespace(returncode=0, stdout="", stderr="")
            if "cat" in 文本 and "预验_" in 文本:
                return types.SimpleNamespace(returncode=0, stdout=绿JSON门禁, stderr="")
            return types.SimpleNamespace(returncode=0, stdout="触发成功", stderr="")

        integrate.运行 = 假运行
        integrate.输出 = lambda 命令: 假SHA
        integrate.服务调用 = lambda 路径, 数据=None: (
            (dict(面["入队"]) if 面["入队"] else None) if "task_enqueue" in 路径
            else {"预验任务": [], "池忙闲": {}})
        integrate.读CI配置 = lambda: {"ssh": "假机", "目录": "/tmp"}
        integrate.查可复用预验绿 = lambda 源, 键, 状态查=None: 面["复用绿"]
        integrate.池轮询预验 = lambda *a, **kw: 面["池轮询"]
        integrate.subprocess = types.SimpleNamespace(run=假subprocess运行)

        def 造参数(wait: bool, pool_limit=None, dry_run=False):
            return types.SimpleNamespace(dry_run=dry_run, remote="gitcode", wait=wait,
                                         allow_pool_red=[], pool_limit=pool_limit or [])

        def 复位面():
            面.update({"复用绿": False, "池轮询": (True, None), "入队": {"ok": True},
                      "推失败": False})
            调用账.clear()

        # —— ⑧ 863 六例（删除时机回归锚·原样保留）——
        回值 = 云端预验门禁(假SHA, 造参数(False), 源分支="任务/863", 等待=False)
        断言(回值 == 已入池哨兵, "863 非 --wait：入池成功返回哨兵（不挂机·不 push）")
        断言(not any("--delete" in c for c in 调用账),
             "863 非 --wait 哨兵路径：不触发远端预验分支删除（runner 待 fetch 消费·五连实录回归锚）")
        断言(any("refs/heads/ci/预验-" in " ".join(c) for c in 调用账),
             "863 非 --wait 哨兵路径：预验分支推送照常（入池前提不变）")
        复位面()
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None, "863 --wait 对照：池轮询拿到绿结论放行（None）")
        断言(any("--delete" in c for c in 调用账),
             "863 --wait 对照：拿到结论后照删远端预验分支（收口语义不回归）")

        # —— ⑨ 976 扩展转移 ——
        复位面()
        回值 = 云端预验门禁(假SHA, 造参数(True, dry_run=True), 源分支="任务/863", 等待=True)
        断言(回值 is None and not any("ci/预验-" in " ".join(c) for c in 调用账),
             "⑨ 门禁：--dry-run=跳过云端真跑（零推送零删除）")
        复位面()
        面["复用绿"] = True
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None and not any("refs/heads/ci/预验-" in " ".join(c) for c in 调用账),
             "⑨ 门禁：830 复用绿命中=免重跑直接放行（零分支推送）")
        复位面()
        面["推失败"] = True
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is not None and "推送失败" in 回值, "⑨ 门禁：预验分支推送失败=拦截")
        复位面()
        面["池轮询"] = (False, None)
        回值 = 云端预验门禁(假SHA, 造参数(True, pool_limit=["win-x64"]),
                        源分支="任务/863", 等待=True)
        断言(回值 is not None and "linux-x86_64" in 回值
             and not any("--delete" in c for c in 调用账),
             "⑨ 门禁：337 守卫（限制无 linux-x86_64）=拦截+分支保留（行活待白名单 runner）")
        复位面()
        面["池轮询"] = (False, None)
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None, "⑨ 901 降级：池超时→ssh 直发拿到绿=放行")
        断言(not any("--delete" in c for c in 调用账),
             "⑨ 901 降级：池行可能仍活（未确认终态）——预验分支保留不删（863 同族消除）")
        复位面()
        面["池轮询"] = (False, None)
        面["入队"] = None
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None and not any("--delete" in c for c in 调用账),
             "⑨ 901 同族：入队响应丢失（请求可能已建行）=保守保留（miss 不静默判定）")
        复位面()
        面["入队"] = {"ok": False, "说明": "拒因X"}
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None and any("--delete" in c for c in 调用账),
             "⑨ 门禁：入队明确拒绝（服务端确认未建行）=降级 ssh 收场照删（无 runner 可认领）")
        复位面()
        面["入队"] = {"ok": False,
                   "说明": "源分支 任务/x 已并入 develop（台账已销账）——预验无意义·拒绝入池（562）"}
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is None and any("--delete" in c for c in 调用账),
             "⑨ 门禁：562 入队被拒已并入——develop 已含链顶=放行收场（不降级重跑全量）")
        复位面()
        面["池轮询"] = (False, None)
        integrate.subprocess = types.SimpleNamespace(run=lambda 命令, **kw: (
            types.SimpleNamespace(returncode=1, stdout="boom", stderr="boom")
            if "cat" not in " ".join(str(c) for c in 命令) else
            types.SimpleNamespace(returncode=0, stdout=绿JSON门禁, stderr="")))
        回值 = 云端预验门禁(假SHA, 造参数(True), 源分支="任务/863", 等待=True)
        断言(回值 is not None and "触发失败" in 回值
             and not any("--delete" in c for c in 调用账),
             "⑨ 901 同族：降级 ssh 触发失败=拦截+分支保留（降级前置位统一）")
    finally:
        for 名, 值 in 真绑门禁.items():
            setattr(integrate, 名, 值)

    # ⑩ 976 归纳件（纯函数）：E2E 红用例行解析 + 预验入队载荷构造
    断言(解析E2E红用例行("✗ 524_条件编译双编译: 失败\n普通行\n✗ 501_x: y")
         == ["524_条件编译双编译", "501_x"],
         "⑩ 红用例行：✗ 行提取（win 分步/linux 串行复验共用面·447-a）")
    断言(解析E2E红用例行("") == [] and 解析E2E红用例行(None) == [],
         "⑩ 红用例行：空文本/None 不炸（与内联面同韧性）")
    断言(解析E2E红用例行("\x1b[31m✗ 501_某用例: 失败\x1b[0m\n普通行\n✗ 502_b: x")
         == ["501_某用例", "502_b"],
         "868 红用例行：ANSI 色包裹 ✗ 行可提取（先剥 SGR 转义再判行首）")
    断言(构造预验入队载荷("ci/预验-a", "s" * 40, "任务/9")
         == {"分支": "ci/预验-a", "sha": "s" * 40, "源分支": "任务/9"},
         "⑩ 入队载荷：无可选字段=仅三必带（主入队/防御重入队同构）")
    载荷k = 构造预验入队载荷("ci/预验-a", "s" * 40, "任务/9", "win-x64", "k" * 40)
    断言(载荷k["认领限制"] == "win-x64" and 载荷k["内容键"] == "k" * 40 and len(载荷k) == 5,
         "⑩ 入队载荷：337 认领限制+830 内容键随行")

    # ⑪ 868 复验面空清单守卫（全量门禁 win/linux 分步复验·722「空集合恒真」残留加固回归锚）：
    #   注入假 运行/运行捕获（假 rc/stdout·零真进程·单测路径仅 exists 只读）——串行 rc 非零
    #   时：✗ 清单空=硬红（修复锚——修复前 win 面空清单落「未点名为空」=披露放行；linux 面
    #   点名非空时空清单未点名恒空=放行）；清单非空=点名/未点名两分（点名机制不回归）。
    真运行复验 = integrate.运行
    真运行捕获复验 = integrate.运行捕获
    假串行出口 = {"stdout": ""}
    try:
        def 假运行复验(命令, **_):
            文 = " ".join(str(c) for c in 命令)
            if "powershell" in 文:            # win 首步 ci.ps1 未过 → 进分步复验
                return types.SimpleNamespace(returncode=1, stdout="", stderr="")
            return types.SimpleNamespace(returncode=0, stdout="", stderr="")   # 构建/单测过

        def 假运行捕获复验(命令):
            文 = " ".join(str(c) for c in 命令)
            if "--jobs 1" in 文:              # 串行复验轮（win/linux 两平台同判据）
                return types.SimpleNamespace(returncode=1, stdout=假串行出口["stdout"], stderr="")
            return types.SimpleNamespace(returncode=1, stdout="并行轮输出\n", stderr="")

        integrate.运行 = 假运行复验
        integrate.运行捕获 = 假运行捕获复验

        假串行出口["stdout"] = "（编译输出·无 ✗ 汇总行）\n"
        问题 = 全量门禁("win", [], 快速通道=False)
        断言(问题 is not None and "解析为空" in 问题,
             "868 win 复验：串行 rc 非零+✗ 清单空=硬红（修复锚·原披露放行）")
        假串行出口["stdout"] = "  ✗ 501_某用例: 失败原因\n"
        问题 = 全量门禁("win", [], 快速通道=False)
        断言(问题 is not None and "真红" in 问题,
             "868 win 复验：清单非空+无点名=未点名真红拦截")
        问题 = 全量门禁("win", ["501_某用例"], 快速通道=False)
        断言(问题 is None, "868 win 复验：清单全命中点名=披露放行（点名机制不回归）")

        假串行出口["stdout"] = ""
        问题 = 全量门禁("linux-x64", [], 快速通道=False)
        断言(问题 is not None and "解析为空" in 问题,
             "868 linux 复验：串行 rc 非零+✗ 清单空+无点名=硬红")
        问题 = 全量门禁("linux-x64", ["501_某用例"], 快速通道=False)
        断言(问题 is not None and "解析为空" in 问题,
             "868 linux 复验：✗ 清单空+点名非空=仍硬红（修复锚·原「已知红非空且未点名空」恒真放行）")
        假串行出口["stdout"] = "✗ 501_某用例: 失败\n"
        问题 = 全量门禁("linux-x64", [], 快速通道=False)
        断言(问题 is not None and "未点名" in 问题,
             "868 linux 复验：清单非空+无点名=未点名真红拦截")
        问题 = 全量门禁("linux-x64", ["501_某用例"], 快速通道=False)
        断言(问题 is None, "868 linux 复验：清单全命中点名=披露放行（点名机制不回归）")
    finally:
        integrate.运行 = 真运行复验
        integrate.运行捕获 = 真运行捕获复验

    print(f"\n[selftest] {案例数} 例全过 ✓")
    return 0
