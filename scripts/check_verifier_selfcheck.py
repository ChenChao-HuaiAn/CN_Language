#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_verifier_selfcheck.py —— 云端验证机器自身考核（894·「考官被考」缺口）。

背景（894 台账原文）：题库（837/841）、采样器（840）、诊断文案（839）三面已有
「考官被考」，唯独**云端验证机器本身**（池 runner/兜底轮/预验上报链）无考核——
它报绿是否可信？同族既有血迹=1018（预验解析瞬态）、868（预验红解析放行漏洞）、
975/863（runner 认领失败五连静默）、1061（runner 假绿面）。

本脚本=b 面「验证器自检套件」（纯本机·无池依赖·秒级）：坏结果注入四态单测——
把**已知该被拦的结果**（红/超时/空结果）注入验证机器各判定面，断言机器**必须
判红**（若判绿=验证机器失效·本门禁红）；绿态为对照（机器对绿结果确实判绿——
防「恒红」这种反方向失灵）。

注入面（三端判定 + 跨端管线 + 结构锁 + 元断言）：
  ① 聚合面：ci_daemon.聚合绿（runner 轮绿聚合）——红/超时四态 + 聚合白名单八键
     逐个注入红；对账周检类非白名单键 rc=1 不拖轮绿（973 有意语义登记锁）。
  ② 落盘/上报面：ci_daemon.落盘（临时目录隔离 + 上报桩）——ci/预验- 红/超时
     **不得写 latest.json 不得上报**（1008 放红漏洞的结构防线：latest=develop 头
     语义，预验红污染 latest=免跑误判）；预验绿写 latest+上报（联动对照）；
     空结果（{}）必须 fail-fast（不得静默落盘/产绿）。
  ③ 解析放行面：integrate.解析预验结果——红/超时 JSON 判红、空文本/空对象/
     绿字段坏形态判红（868 收严语义）、服务端回读 0/1 形态契约。
  ④ 服务端存储面：queue_service.处理写（临时 DB 直调·零 HTTP）——红/超时/空结果
     注入的存储语义（绿列 0、详情留档、空结果不崩）；认领失败分类（975）对同步
     超时算基础设施失败、对真实验证红/空结果不误报。
  ⑤ 跨端管线面：runner 结果 → 服务端详情 JSON 往返 → 集成解析判定——四态端到端
     判定与注入态一致（任一环把红/超时/空放行=上报链放红）。
  ⑥ 元断言（**自检面判别力自证**·110-a「探测器自身亦被测」同族）：注入「坏验证器」
     （聚合恒绿/解析恒放行/落盘红也写 latest）——断言自检面**必须抓住**；坏也全过
     =自检面空转失灵（比验证机器坏更隐蔽的失效）。
  ⑦ 结构锁：跑一轮 同步键首写（空步骤字典不可达的结构根据——聚合面「缺键不计」
     语义的安全前提）、report 段绿存储形态（None/缺失=0 不默认真）。

a 面（阴性对照轮）设计/运行手册=plans/028 §十六（周期标本分支入池→断言必红→
报绿=机器故障告警·可选实施清单）；本脚本即 b 面落地点。

边界（诚实声明）：坏结果注入证明的是**判定链语义**（红不会被翻绿），不替代 a 面
「真跑一轮全量后仍必红」的端到端考核；服务端面以进程内直调覆盖（HTTP/鉴权/并发
认领面由 test_pool_selftest.py 承担——分工：彼=服务活着且协议对，此=判定语义不撒谎）。
与既有自测的重复面标记：ci_daemon_selftest（runner 单元面）/integrate_selftest
（解析形态单元面）依旧在位——本脚本是其上的「四态注入考核」视角，保留双面互为佐证。

用法：python3 scripts/check_verifier_selfcheck.py
挂载：L1 gate_quick 静态段（每次提交·秒级）+手动；注册=scripts/check_registry.json。
退出码：0=全绿（验证机器判定面可信）；1=有红（判定面失效或自检面失判别力）。
"""
from __future__ import annotations

import atexit
import inspect
import io
import json
import os
import shutil
import sys
import tempfile
from pathlib import Path

try:  # 控制台编码（中文诊断防 GBK 崩——与 check_cross_asm_guards 同口径）
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
except Exception:
    pass

脚本目录 = Path(__file__).resolve().parent
sys.path.insert(0, str(脚本目录))

# 服务端面（④）以临时 DB 直调 queue_service 处理写——**必须先注入隔离环境再导入**：
#   模块级 con=建库() 读取 CN_QUEUE_DB（缺省=scripts/queue_state.db 真实队列库路径）
#   ——不隔离导入=本自检把考核数据写进真实生产账目（污染）；CN_BOARD_URL 指不可达
#   端口=并入检测（562）任何潜在调用走「本机拒绝」路径零外发。临时目录 atexit 清理。
#   直接赋值（非 setdefault）：宿主/服务器环境变量若指向真实库同样必须被隔离覆盖。
_隔离目录 = tempfile.mkdtemp(prefix="cn_894_selfcheck_")
os.environ["CN_QUEUE_DB"] = os.path.join(_隔离目录, "queue_selftest_894.db")
os.environ["CN_BOARD_URL"] = "http://127.0.0.1:9"
atexit.register(lambda: shutil.rmtree(_隔离目录, ignore_errors=True))

import ci_daemon          # noqa: E402（导入顺序=上方隔离前置依赖）
import integrate          # noqa: E402
import queue_service      # noqa: E402
import queue_result       # noqa: E402  1297：认领失败分类 实现外迁 queue_result（queue_service 先拆再加）
import ci_report          # noqa: E402  1297：落盘/上报 实现外迁（隔离猴补对象）

# 轮绿聚合白名单（与 ci_daemon.聚合绿 对齐——行为面逐键注入断言锁白名单；
#   此处别名仅作构造用，单一事实源仍在 聚合绿 实现内）
轮键 = ("同步", "清产物", "配置", "构建", "单测", "e2e")
附键 = "网站示例verify"
静态键 = "check_parallel_determinism"   # 1219：并行确定性门禁 rc=1 必须拖红
聚合白名单 = 轮键 + (附键, 静态键)


def 断言收集器(面前缀: str):
    """面断言收集器（归纳自各注入面同构的 (断言们, 查) 对·事不过三）——
    返回 (断言们, 查)：查 追加 ("面前缀·说明", bool(条件))。"""
    断言们: list[tuple[str, bool]] = []

    def 查(说明: str, 条件) -> None:
        断言们.append((面前缀 + "·" + 说明, bool(条件)))

    return 断言们, 查


def 造步骤(态: str) -> tuple[dict, bool]:
    """造 跑一轮 步骤们 形态（三态·白名单键全备=真轮形状）→ (步骤, 期望绿)。
    空结果态不走本函数（非步骤形态——落盘/解析面另有注入）；未识别态显式抛异常
    （miss 不可静默：四态枚举外的注入=调用方 bug 当场显形）。"""
    步骤 = {k: {"rc": 0, "秒": 1} for k in 聚合白名单}
    if 态 == "绿":
        return 步骤, True
    if 态 == "红":
        步骤["e2e"] = {"rc": 1, "秒": 1, "说明": "桩：E2E 有红用例"}
        return 步骤, False
    if 态 == "超时":
        步骤["e2e"] = {"rc": -99, "秒": 10, "超时": True}
        return 步骤, False
    raise ValueError("未知注入态 %r（合法：绿/红/超时——空结果态不走本函数）" % 态)


def 造结果(态: str, 分支: str = "ci/预验-aaaaaaaaaa") -> dict:
    """造 跑一轮 返回体形状（关键键齐备·同 跑一轮 尾部构造口径）。"""
    步骤, 绿 = 造步骤(态)
    return {"sha": "a" * 40, "分支": 分支, "平台": "linux-x86_64", "步骤": 步骤,
            "绿": 绿, "总秒": 12, "时刻": "894桩", "日志": "run_x.log",
            "日志尾部": ["桩结果（894 坏结果注入）"], "e2e明细": None, "提交历史": []}


def 坏聚合恒绿(步骤们) -> bool:
    """元断言样本：恒绿谎报聚合（红/超时全逃逸）——自检面必须抓住。"""
    return True


def 坏解析恒绿(文本) -> tuple[bool, str]:
    """元断言样本：恒放行解析（不加区分判绿）——自检面必须抓住。"""
    return True, "桩（坏验证器样本）"


def 坏落盘桩(结果) -> None:
    """元断言样本：红也写 latest 且上报（放红漏洞形态）——自检面必须抓住。"""
    (ci_daemon.日志目录 / "latest.json").write_text("{}", encoding="utf-8")
    ci_daemon.上报(结果)


def 聚合面断言(聚合绿) -> list[tuple[str, bool]]:
    """面①：runner 轮绿聚合（ci_daemon.聚合绿）四态 + 白名单逐键。
    参数化=元断言可注入坏实现（判别力自证）。"""
    断言们, 查 = 断言收集器("聚合面")
    步骤绿, _ = 造步骤("绿")
    步骤红, _ = 造步骤("红")
    步骤超, _ = 造步骤("超时")
    查("绿态（白名单键全 rc=0）聚合为绿", 聚合绿(步骤绿) is True)
    查("红态（e2e rc=1）聚合为红", 聚合绿(步骤红) is False)
    查("超时态（e2e rc=-99）聚合为红（超时漏绿=机器失效）", 聚合绿(步骤超) is False)
    for 键 in 聚合白名单:
        步骤 = {k: {"rc": 0} for k in 聚合白名单}
        步骤[键] = {"rc": 1}
        查("白名单键 %s rc=1 拖红" % 键, 聚合绿(步骤) is False)
    查("白名单含 check_parallel_determinism（1219 云端并行确定性红必拖轮红）",
        静态键 in 聚合白名单)
    查("实现锚：ci_daemon.聚合绿 源码白名单含 check_parallel_determinism"
        "（自检别名与单一事实源同步，防只改一侧）",
        "check_parallel_determinism" in inspect.getsource(聚合绿))
    查("负例构造键不在白名单（非白名单不拖红断言不得复用白名单键）",
        "负测期望对账" not in 聚合白名单)
    查("973 登记语义：非白名单键（对账周检步骤）rc=1 不拖轮绿（有意设计防误改）",
        聚合绿({**{k: {"rc": 0} for k in 聚合白名单},
                "负测期望对账": {"rc": 1}}) is True)
    return 断言们


def 解析面断言(解析预验结果) -> list[tuple[str, bool]]:
    """面③：集成放行解析（integrate.解析预验结果）四态 + 空/坏形态。
    与 integrate_selftest 既有形态断言的重复面标记：彼=单元面（函数直测·形态枚举），
    此=894 四态注入考核（上报链一环的端到端视角）——保留双面互为佐证；他日若提炼
    公共断言核，以彼处为形态基准。"""
    断言们, 查 = 断言收集器("解析面")
    绿, _ = 解析预验结果(json.dumps(造结果("绿"), ensure_ascii=False))
    查("绿态 JSON 判绿（放行链通）", 绿 is True)
    绿, 摘 = 解析预验结果(json.dumps(造结果("红"), ensure_ascii=False))
    查("红态 JSON 判红（放红漏洞）", 绿 is False)
    查("红态红步骤入摘要（定界线索）", "e2e" in 摘)
    绿, 摘 = 解析预验结果(json.dumps(造结果("超时"), ensure_ascii=False))
    查("超时态 JSON 判红（rc=-99 漏绿=机器失效）", 绿 is False)
    查("超时态红步骤入摘要", "e2e" in 摘)
    查("空文本判红（详情丢失不得放行·868）", 解析预验结果("")[0] is False)
    查("空对象判红（绿字段缺失=坏源形态·868）", 解析预验结果("{}")[0] is False)
    查("绿字段 None 判红（不翻绿）", 解析预验结果('{"绿": null}')[0] is False)
    查("绿字段 'false' 字符串判红（bool('false') 假绿面·868）",
        解析预验结果('{"绿": "false"}')[0] is False)
    查("服务端回读形态 绿=0 判红", 解析预验结果('{"绿": 0}')[0] is False)
    查("服务端回读形态 绿=1 判绿", 解析预验结果('{"绿": 1}')[0] is True)
    return 断言们


def 跑一次落盘(落盘实现, 结果: dict) -> dict:
    """隔离环境跑一次落盘实现：临时目录=ci_daemon.日志目录（猴补）、上报桩计数、
    try/finally 还原（长期进程环境不被污染）。返回事实字典 {latest, 预验档,
    result数, 上报, 崩}——断言面消费；被测实现的异常作为事实记录（不吞不掩饰），
    由「空结果 fail-fast」断言判定。"""
    with tempfile.TemporaryDirectory(prefix="cn_894_落盘_") as 临时:
        # 1297：落盘/上报 实现外迁 ci_report（模块全局 _日志目录/上报）——猴补对象
        #   跟实现走（原 ci_daemon.日志目录/上报 现为薄壳别名·补之不达真身）
        真目录, 真上报 = ci_report._日志目录, ci_report.上报
        次 = {"上报": 0}

        def 桩上报(结果) -> None:
            次["上报"] += 1

        崩 = None
        try:
            ci_report._日志目录 = Path(临时)
            ci_report.上报 = 桩上报
            try:
                落盘实现(结果)
            except Exception as e:
                崩 = e
        finally:
            ci_report._日志目录, ci_report.上报 = 真目录, 真上报
        目录 = Path(临时)
        return {"latest": (目录 / "latest.json").exists(),
                "预验档": bool(list(目录.glob("预验_*.json"))),
                "result数": len(list(目录.glob("result_*.json"))),
                "上报": 次["上报"], "崩": 崩}


def 落盘面断言(落盘实现) -> list[tuple[str, bool]]:
    """面②：落盘/上报（ci_daemon.落盘）——预验红态不得写 latest 不得上报（1008
    放红漏洞的结构防线：latest=develop 头语义，预验红污染 latest=免跑误判）。
    参数化=元断言注入坏落盘（红也写 latest）自证判别力。"""
    断言们, 查 = 断言收集器("落盘面")
    f = 跑一次落盘(落盘实现, 造结果("绿"))
    查("预验绿：latest 写（链顶将成 develop 头·1008 联动）", f["latest"])
    查("预验绿：上报一次", f["上报"] == 1)
    查("预验绿：预验档+result 留档", f["预验档"] and f["result数"] == 1)
    for 态 in ("红", "超时"):
        f = 跑一次落盘(落盘实现, 造结果(态))
        查("预验%s：不得写 latest（放红漏洞防线）" % 态, not f["latest"])
        查("预验%s：不得上报（develop 未收到该提交）" % 态, f["上报"] == 0)
        查("预验%s：预验档+result 留档（红也要可审计）" % 态,
            f["预验档"] and f["result数"] == 1)
    f = 跑一次落盘(落盘实现, 造结果("绿", 分支="develop"))
    查("develop 绿：latest+上报（常规轮语义对照）", f["latest"] and f["上报"] == 1)
    f = 跑一次落盘(落盘实现, {})
    查("空结果：fail-fast 抛异常（不得静默落盘/产绿）", f["崩"] is not None)
    查("空结果：零产物零上报（异常前无副作用）", not f["latest"] and f["上报"] == 0)
    return 断言们


def 服务端面断言(处理写, 详情查, 认领失败分类) -> list[tuple[str, bool]]:
    """面④：服务端存储（queue_service 直调·临时 DB·零 HTTP）——四态注入的存储
    语义 + 975 认领失败分类不误报。参数化=纯函数面（主流程只传真实现）。"""
    断言们, 查 = 断言收集器("服务端面")

    def 一轮(标记: str, 绿字段, 结果体):
        """enqueue→claim→complete 一条龙（每态独立 sha/分支/runner 防互扰）。
        返回 (认领命中, 详情行)。"""
        sha = 标记 * 40
        分支 = "ci/预验-" + sha[:10]
        runner = "r894-" + 标记
        处理写("task_enqueue", {"分支": 分支, "sha": sha})
        认 = 处理写("task_claim", {"runner": runner})
        命中 = bool(认.get("任务")) and 认["任务"]["分支"] == 分支
        处理写("task_complete", {"runner": runner, "分支": 分支,
                                 "绿": 绿字段, "结果": 结果体})
        return 命中, 详情查(sha[:10])

    结果红 = 造结果("红", 分支="ci/预验-" + "b" * 10)
    结果红["sha"] = "b" * 40
    命中, 行 = 一轮("b", False, 结果红)
    查("红注入：认领到本任务（考核前置）", 命中)
    查("红注入：状态完成", 行["状态"] == "完成")
    查("红注入：绿列=0（红被存成绿=放红）", 行["绿"] is False)
    查("红注入：详情留档且判红（红也要可审计）",
        isinstance(行["详情"], dict) and 行["详情"]["绿"] is False)

    结果超 = 造结果("超时", 分支="ci/预验-" + "c" * 10)
    结果超["sha"] = "c" * 40
    _, 行 = 一轮("c", False, 结果超)
    查("超时注入：绿列=0（rc=-99 不得存绿）", 行["绿"] is False)
    查("超时注入：详情保留超时标记（定界证据）",
        isinstance(行["详情"], dict)
        and (行["详情"].get("步骤") or {}).get("e2e", {}).get("rc") == -99)

    _, 行 = 一轮("d", False, None)
    查("空结果注入：不崩且状态完成·绿列=0（空详情不得存绿）",
        行["状态"] == "完成" and 行["绿"] is False)
    查("空结果注入：详情=None 回读（空形零谎报）", 行["详情"] is None)

    类别, 摘要 = 认领失败分类({"步骤": {"同步": {"rc": 128}},
                              "日志尾部": ["fatal: 预验分支不存在"]})
    查("975：同步失败分类（基础设施失败告警数据源）",
        类别 == "同步失败" and "fatal" in 摘要)
    查("975：同步段超时 -99 也归基础设施失败（不漏告警）",
        认领失败分类({"步骤": {"同步": {"rc": -99}}})[0] == "同步失败")
    查("975：真实验证红（同步 rc=0）不分类（不污染告警语义）",
        认领失败分类({"步骤": {"同步": {"rc": 0}, "e2e": {"rc": 1}}})[0] == "")
    查("975：空结果不分类不误报",
        认领失败分类(None) == ("", "") and 认领失败分类({}) == ("", ""))
    return 断言们


def 管线自洽断言(聚合绿, 解析预验结果) -> list[tuple[str, bool]]:
    """面⑤：跨端管线（runner 聚合 → 结果 JSON → 服务端详情往返 → 集成解析判定）
    ——四态端到端判定必须与注入态一致（任一环放红/超时/空=上报链放红）。
    参数化=元断言可注入坏端点（坏聚合=runner 谎报绿源；坏解析=集成端放行）。"""
    断言们, 查 = 断言收集器("管线面")
    期望表 = {"绿": True, "红": False, "超时": False}
    for 态, 期望 in 期望表.items():
        结果 = 造结果(态)
        查("%s 态：runner 绿字段与 聚合绿(步骤) 自洽（不谎报源）" % 态,
            结果["绿"] is 聚合绿(结果["步骤"]))
        写况 = json.dumps(结果, ensure_ascii=False)             # 服务端详情存储形态
        取回 = json.dumps(json.loads(写况), ensure_ascii=False)  # integrate 取回形态
        绿, _ = 解析预验结果(取回)
        查("%s 态：端到端判定=%s（期望 %s）" % (态, 绿, 期望), 绿 is 期望)
    绿, _ = 解析预验结果("")
    查("空结果态：详情缺失端到端判红（不得放行）", 绿 is False)
    return 断言们


def 结构断言条() -> list[tuple[str, bool]]:
    """面⑦：结构锁（防回退）。
    · 跑一轮 同步键首写：空步骤字典在真实链路不可达的结构根据（聚合面「缺键不计」
      语义的安全前提——同步键恒在，则「无键=绿」的聚合路径永不承载真实轮）。
    · report 段绿存储形态 (1 if 数据.get("绿") else 0)：None/缺失=0（不默认真）。"""
    断言们, 查 = 断言收集器("结构锁")
    源一轮 = inspect.getsource(ci_daemon.跑一轮)
    p同步 = 源一轮.find('步骤们["同步"] = {"rc": 同步rc}')
    p构 = [p for p in (源一轮.find("win构建链("), 源一轮.find("linux构建链("),
                     源一轮.find("静态门禁与e2e段(")) if p >= 0]
    查("跑一轮 同步键首写（先于构建段/静态段——空步骤字典不可达结构根据）",
        p同步 >= 0 and bool(p构) and p同步 < min(p构))
    服务源 = (脚本目录 / "queue_service.py").read_text(encoding="utf-8")
    查("report 段绿存储形态 (1 if 数据.get(\"绿\") else 0) 在位（None/缺失=0）",
        'None if 是执行中 else (1 if 数据.get("绿") else 0)' in 服务源)
    return 断言们


def 元断言条() -> list[tuple[str, bool]]:
    """面⑥：自检面判别力自证（110-a「探测器自身亦被测」同族）——注入坏验证器
    （恒绿聚合/恒放行解析/坏落盘），断言自检面**必须抓住**；坏也全过=自检面
    空转失灵（最隐蔽的失效：自检恒真、验证机器坏了也报绿）。"""
    断言们, 查 = 断言收集器("元断言")
    聚合败 = [名 for 名, ok in 聚合面断言(坏聚合恒绿) if not ok]
    查("坏聚合（恒绿谎报）被聚合面抓住 %d 条（判别力）" % len(聚合败), bool(聚合败))
    解析败 = [名 for 名, ok in 解析面断言(坏解析恒绿) if not ok]
    查("坏解析（恒放行）被解析面抓住 %d 条（判别力）" % len(解析败), bool(解析败))
    落盘败 = [名 for 名, ok in 落盘面断言(坏落盘桩) if not ok]
    查("坏落盘（红也写 latest/上报）被落盘面抓住 %d 条（判别力）" % len(落盘败),
        bool(落盘败))
    聚合管线败 = [名 for 名, ok in
                 管线自洽断言(坏聚合恒绿, integrate.解析预验结果) if not ok]
    查("坏聚合被管线面抓住（端到端谎报显形）", bool(聚合管线败))
    解析管线败 = [名 for 名, ok in
                 管线自洽断言(ci_daemon.聚合绿, 坏解析恒绿) if not ok]
    查("坏解析被管线面抓住（端到端放行显形）", bool(解析管线败))
    return 断言们


def main() -> int:
    print("== check_verifier_selfcheck（894·云端验证机器自身考核·坏结果注入四态）==",
          flush=True)
    全部: list[tuple[str, bool]] = []
    全部 += 聚合面断言(ci_daemon.聚合绿)
    全部 += 解析面断言(integrate.解析预验结果)
    全部 += 落盘面断言(ci_daemon.落盘)
    全部 += 服务端面断言(queue_service.处理写, queue_service.任务详情查,
                          queue_result.认领失败分类)   # 1297：实现外迁 queue_result
    全部 += 管线自洽断言(ci_daemon.聚合绿, integrate.解析预验结果)
    全部 += 结构断言条()
    全部 += 元断言条()
    for 名, ok in 全部:
        print("  [%s] %s" % ("OK" if ok else "×", 名))
    红 = sum(1 for _, ok in 全部 if not ok)
    print("== 894 验证机器自检：%d 断言，%s ==" % (
        len(全部),
        "全绿（判定面可信）" if 红 == 0
        else "红 %d——验证机器判定面失效或自检面失判别力" % 红), flush=True)
    return 0 if 红 == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
