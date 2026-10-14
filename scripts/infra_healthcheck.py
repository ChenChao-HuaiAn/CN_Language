#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CN 基础设施「自己还活着」健康自检（任务 728m·2026-10-14·49~51 篇审计建议）

背景（教训 155·权重 8）：看板前端一次部署引入 JS 语法错误整页瘫死——HTTP 200
探测全过、页面停「连接中…」四小时无人察觉：「HTTP 200 ≠ 页面活着」。本工具把
看板(8301)/集成队列(8300)/验证池的健康检查清单化、可执行化：部署后必跑 + 每日
巡检各一条命令，FAIL 项必带可排查证据（教训 145 信噪比：不刷无指向告警）。

检查项（逐项 PASS/FAIL/WARN + 证据）：
  [A. 看板 8301]
    A1 页面可达（HTTP 200）
    A2 页面结构（关键元素 id 齐全）
    A3 /api/board 结构（必需字段存在、类型正确）
    A4a 前端脚本语法 parse（quickjs 引擎逐脚本 Function 构造检查）
    A4b 前端执行级真冒烟（quickjs + mock DOM 执行 board.js + 真实 /api/board
        数据——「连接中…」必须被数据替换；无 quickjs=降级静态弱检查
        〔括号/引号粗配平 + 关键函数存在性〕并明确标 WARN 弱检查）
    A5 页面外链资源可达（内联部署=无外链·PASS 注明）
  [B. 集成队列 8300]
    B1 /api/state 可达（HTTP 200）
    B2 结构（队列/门禁最近/预验任务 字段齐全）
    B3 验证池摘要（信息项：排队/执行中/完成·不判 FAIL——夜间闲置属正常）
退出码：0=无 FAIL（可含 WARN/INFO）；1=有 FAIL；2=用法/环境错误。

用法：
  python3 scripts/infra_healthcheck.py                       # 默认公网目标
  python3 scripts/infra_healthcheck.py --board-url http://… --queue-url http://…
  python3 scripts/infra_healthcheck.py --json                # 结构化输出
  python3 scripts/infra_healthcheck.py --selftest            # 注入式两态自检
默认目标可用环境变量覆盖：CN_BOARD_URL / CN_QUEUE_URL。
"""

import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from html.parser import HTMLParser
from pathlib import Path

# ---- 常量 ----

默认看板基址 = os.environ.get("CN_BOARD_URL", "http://124.222.106.84:8301")
默认队列基址 = os.environ.get("CN_QUEUE_URL", "http://124.222.106.84:8300")
默认超时秒 = 8.0
证据上限 = 220            # 证据片段截断长度（够定位·不刷屏）

# 页面必备元素 id（前端初始化/渲染直接消费·缺一即页面必瘫或功能残缺）
看板必备元素 = ("元信息", "状态灯", "错误条", "任务区", "在飞区", "冲突带",
                "意图区", "自动刷新", "刷新钮")
# /api/board 聚合必需字段（board_service.聚合视图 契约）
看板必需字段 = {"意图们": list, "在飞分支们": list, "任务们": list,
                "冲突们": list, "失联秒": (int, float), "时刻": (str, int, float)}
# /api/state 必需字段（queue_service 契约）
队列必需字段 = {"队列": list, "门禁最近": list, "预验任务": list}
# 前端关键函数（静态弱检查用：board.js 数据链四梁八柱）
关键函数们 = ("调API", "拉取", "渲染激活页", "渲染任务")

级别们 = ("PASS", "FAIL", "WARN", "INFO")


class 检查结果:
    """一条检查结论：编号/名/级别/详情/证据。证据=可排查线索（#145）。"""

    def __init__(self, 编号, 名, 级别, 详情="", 证据=""):
        self.编号 = 编号
        self.名 = 名
        self.级别 = 级别
        self.详情 = 详情
        self.证据 = 证据

    def 字典(self):
        return {"编号": self.编号, "名": self.名, "级别": self.级别,
                "详情": self.详情, "证据": self.证据}


def 证据片段(文本, 上限=证据上限):
    """响应/异常截断成单行证据片段（换行折成 \\n 字面量·保留定位信息）。"""
    s = (文本 or "").strip()
    s = s.replace("\r", " ").replace("\n", "\\n")
    return s[:上限] + ("…" if len(s) > 上限 else "")


# ---- HTTP 工具 ----

def 取地址(基址, 路径, 超时秒=默认超时秒):
    """GET 一个路径；返回 (码, 文本, 错因, 毫秒)。码=0 表示连接层失败（错因非空）。
    HTTPError（4xx/5xx）按「正常响应」返回——错误码是检查判据的一部分。"""
    地址 = 基址.rstrip("/") + 路径
    请求 = urllib.request.Request(地址, headers={"User-Agent": "cn-infra-healthcheck/728m"})
    起 = time.monotonic()
    try:
        with urllib.request.urlopen(请求, timeout=超时秒) as 响应:
            return 响应.status, 响应.read().decode("utf-8", "replace"), "", \
                int((time.monotonic() - 起) * 1000)
    except urllib.error.HTTPError as e:
        try:
            体 = e.read().decode("utf-8", "replace")
        except OSError:
            体 = ""
        return e.code, 体, "", int((time.monotonic() - 起) * 1000)
    except (OSError, ValueError) as e:
        return 0, "", f"{type(e).__name__}: {e}", int((time.monotonic() - 起) * 1000)


def 绝对值(基址, 源):
    """外链资源地址归一（http(s) 原样·相对路径拼基址）——三处消费点共用。"""
    if 源.startswith(("http://", "https://", "data:", "#")):
        return 源
    return 基址.rstrip("/") + "/" + 源.lstrip("/")


# ---- 页面解析 ----

class 页面解析器(HTMLParser):
    """从看板 HTML 提取：内联脚本文本、外链资源（script src/link href）、
    元素 id 集合与 id 元素首段文本（供 mock DOM 还原「连接中…」初值）。"""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.元素id集 = set()
        self.脚本们 = []            # [(描述, 文本)]
        self.外链脚本 = []          # <script src=…>
        self.外链样式们 = []        # <link href=…>
        self.元素初值 = {}          # id → 首段非空文本（近似：第一个元素内 data）
        self._脚本缓冲 = None
        self._等待初值 = None

    def handle_starttag(self, tag, attrs):
        属性 = dict(attrs)
        标识 = 属性.get("id")
        if 标识:
            self.元素id集.add(标识)
            if 标识 not in self.元素初值 and self._等待初值 is None:
                self._等待初值 = 标识
        if tag == "script":
            if 属性.get("src"):
                self.外链脚本.append(属性["src"])
            else:
                self._脚本缓冲 = []
        elif tag == "link" and 属性.get("href"):
            self.外链样式们.append(属性["href"])

    def handle_data(self, data):
        if self._脚本缓冲 is not None:
            self._脚本缓冲.append(data)
        if self._等待初值 is not None:
            段 = data.strip()
            if 段:
                self.元素初值[self._等待初值] = 段
                self._等待初值 = None

    def handle_endtag(self, tag):
        if tag == "script" and self._脚本缓冲 is not None:
            self.脚本们.append((f"内联脚本#{len(self.脚本们) + 1}", "".join(self._脚本缓冲)))
            self._脚本缓冲 = None
        if self._等待初值 is not None:
            self._等待初值 = None      # 元素闭合仍无文本——空元素，放弃


# ---- 前端真 JS 冒烟（quickjs）----

# mock DOM 环境（quickjs 无 DOM·注入最小可执行面）。覆盖面=board.js 实际用到的
# 全局：document（getElementById/querySelectorAll/addEventListener/documentElement）、
# localStorage、navigator.clipboard、fetch、AbortController、setTimeout/setInterval。
# fetch 按路径分发：api/board→注入的真实板面数据；api/me→未登录；其余→{}。
# __元件 是元素注册表（断言读 __元件["元信息"].textContent）。
MOCK_JS = r"""
var __元件 = {};
var __请求记录 = [];
function __造元素(id) {
  if (__元件[id]) return __元件[id];
  var el = {
    id: id, textContent: "", innerHTML: "", title: "", value: "", hidden: false,
    checked: false, disabled: false, style: {}, dataset: {},
    classList: { add: function(){}, remove: function(){}, toggle: function(){},
                 contains: function(){ return false; } },
    addEventListener: function(){}, removeEventListener: function(){},
    querySelectorAll: function(){ return []; }, querySelector: function(){ return null; },
    closest: function(){ return null; }, appendChild: function(){}, removeChild: function(){},
    focus: function(){}, select: function(){}, scrollIntoView: function(){},
    parentElement: null, children: []
  };
  el.parentElement = el;
  __元件[id] = el;
  return el;
}
var document = {
  getElementById: function(id){ return __造元素(id); },
  querySelectorAll: function(){ return []; },
  querySelector: function(){ return null; },
  addEventListener: function(){}, removeEventListener: function(){},
  documentElement: { dataset: {} },
  createElement: function(){ return __造元素("__新元素"); },
  cookie: ""
};
var localStorage = { getItem: function(){ return null; },
                     setItem: function(){}, removeItem: function(){} };
var navigator = { clipboard: { writeText: function(){ return Promise.resolve(); } } };
function AbortController(){ this.signal = {}; this.abort = function(){}; }
var setTimeout = function(){ return 0; };
var clearTimeout = function(){};
var setInterval = function(){ return 0; };
var clearInterval = function(){};
var window = globalThis;
var __板面数据 = {};
function fetch(路径) {
  __请求记录.push(String(路径));
  var 体 = {};
  if (String(路径).indexOf("api/board") >= 0) 体 = __板面数据;
  else if (String(路径).indexOf("api/me") >= 0) 体 = { "已登录": false };
  return Promise.resolve({ ok: true, status: 200,
                           json: function(){ return Promise.resolve(体); } });
}
"""


def JS字面量(值):
    """Python 值 → JS 字面量文本（JSON 是 JS 子集；U+2028/2029 转义防旧解析歧义）。"""
    return (json.dumps(值, ensure_ascii=False)
            .replace("\u2028", "\\u2028").replace("\u2029", "\\u2029"))


def 组装mock(板面数据, 元素初值):
    """mock 前缀 + 注入：真实板面数据 + 元素初值（还原页面「连接中…」态）。"""
    # 先给关键 id 注初值——检查「初值是否被数据替换」须先有初值
    片段 = ["var __初值来源 = " + JS字面量(元素初值) + ";"]
    片段.append('for (var k in __初值来源) { __造元素(k).textContent = __初值来源[k]; }')
    return MOCK_JS + "\n" + "\n".join(片段) + "\nvar __板面数据 = " + JS字面量(板面数据) + ";\n"


def 静态弱检查(脚本们):
    """无 quickjs 降级路径：括号粗配平 + 关键函数存在性。
    明确标 WARN——不是真冒烟（语法级 run 不可得），真冒烟需人工浏览器。"""
    合并 = "\n".join(文本 for _, 文本 in 脚本们)
    问题 = 括号粗配平(合并)
    缺函数 = [名 for 名 in 关键函数们
              if not re.search(r"function\s+" + re.escape(名) + r"\s*\(", 合并)
              and not re.search(r"(?:const|let|var)\s+" + re.escape(名) + r"\s*=", 合并)]
    if 问题:
        return 检查结果("A4a", "前端脚本静态弱检查", "FAIL",
                        "括号/引号粗配平未过（弱检查·命中须人工复核）", 证据=问题)
    if 缺函数:
        return 检查结果("A4a", "前端脚本静态弱检查", "FAIL",
                        "关键函数缺失（弱检查）", 证据="未找到：" + "、".join(缺函数))
    return 检查结果("A4a", "前端脚本静态弱检查", "WARN",
                    "quickjs 不可用——已降级为静态弱检查（括号配平+关键函数存在性）；"
                    "真 JS 冒烟需部署机人工开浏览器确认")

def 括号粗配平(文本):
    """近似 JS 词法扫描（跳过字符串/注释/正则字面量），返回首个不平衡点描述或 None。
    「粗」=正则字面量用启发式判定（上一有效字符），命中时证据指向行号人工复核。"""
    配对面 = {")": "(", "]": "[", "}": "{"}
    名 = {"(": "圆", "[": "方", "{": "花"}
    栈 = []                    # [('('|'['|'{', 行号)]——未闭合时按行号点名（可排查证据）
    行 = 1
    i = 0
    n = len(文本)
    上一有效 = ""
    while i < n:
        c = 文本[i]
        if c == "\n":
            行 += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "/":
            j = 文本.find("\n", i)
            i = n if j < 0 else j
            continue
        if c == "/" and i + 1 < n and 文本[i + 1] == "*":
            j = 文本.find("*/", i + 2)
            行 += 文本.count("\n", i, j if j >= 0 else n)
            i = n if j < 0 else j + 2
            continue
        if c in "'\"`":
            引 = c
            j = i + 1
            while j < n:
                if 文本[j] == "\\":
                    j += 2
                    continue
                if 文本[j] == "\n":
                    行 += 1
                if 文本[j] == 引:
                    break
                j += 1
            if j >= n:
                return f"第 {行} 行起字符串未闭合（引号 {引}）"
            i = j + 1
            上一有效 = 引
            continue
        if c == "/" and (not 上一有效 or 上一有效 in "(=,:[!&|?{};+-*/%<>~^"):
            # 疑似正则字面量：扫到未转义 / 为止（字符类内 / 不算结束）
            j = i + 1
            类内 = False
            闭 = False
            while j < n:
                if 文本[j] == "\\":
                    j += 2
                    continue
                if 文本[j] == "[":
                    类内 = True
                elif 文本[j] == "]":
                    类内 = False
                elif 文本[j] == "/" and not 类内:
                    闭 = True
                    break
                elif 文本[j] == "\n":
                    break
                j += 1
            if 闭:
                j += 1
                while j < n and (文本[j].isalnum() or 文本[j] == "_"):
                    j += 1
                i = j
                上一有效 = "/"
                continue
        if c in ("(", "[", "{"):
            栈.append((c, 行))
        elif c in 配对面:
            左 = 配对面[c]
            位 = next((k for k in range(len(栈) - 1, -1, -1) if 栈[k][0] == 左), None)
            if 位 is None:
                return f"第 {行} 行处多余右{名[左]}括号 '{c}'（无对应左括号）"
            del 栈[位]
        上一有效 = c
        i += 1
    if 栈:
        return "括号未闭合：" + "、".join(
            f"'{k}'于第 {行号} 行" for k, 行号 in 栈[:5]) + ("…" if len(栈) > 5 else "")
    return None


def 找quickjs():
    """延迟导入 quickjs（本机 1.19.4 可用；无则降级静态弱检查）。"""
    try:
        import quickjs
        return quickjs
    except ImportError:
        return None


def 前端冒烟(脚本们, 板面数据, 元素初值, quickjs模块):
    """真 JS 冒烟两级：
    A4a 语法 parse——Function 构造只解析不执行（逐脚本·无副作用）；
    A4b 执行级——mock DOM 中按序 eval 全部脚本，驱动微任务，断言
        fetch 被调到 api/board 且「元信息」被成功文案替换（#155 判据）。
    返回 (语法结果, 执行结果) 两条 检查结果。"""
    if quickjs模块 is None:
        降 = 静态弱检查(脚本们)
        return 降, 检查结果("A4b", "前端执行级冒烟", "WARN",
                            "quickjs 不可用——跳过执行级（弱检查见 A4a）")
    # —— A4a：逐脚本语法 parse（Function 构造不执行函数体）——
    for 名, 文本 in 脚本们:
        try:
            _ctx_语法 = quickjs模块.Context()
            _ctx_语法.eval("Function(" + JS字面量(文本) + ")")
        except Exception as e:
            消息 = str(e)
            if 消息.lstrip().startswith("SyntaxError"):
                return (检查结果("A4a", "前端脚本语法 parse", "FAIL",
                                 f"{名} 语法错误（浏览器同位置必瘫页）",
                                 证据=证据片段(消息)),
                        检查结果("A4b", "前端执行级冒烟", "FAIL",
                                 "语法已坏·执行级无从谈起", 证据="见 A4a"))
    # —— A4b：mock 环境执行 ——
    ctx = quickjs模块.Context()
    try:
        ctx.eval(组装mock(板面数据, 元素初值))
    except Exception as e:
        # mock 注入失败=检查器自身缺陷（非页面缺陷）——不冒充 FAIL（#145）
        return (检查结果("A4a", "前端脚本语法 parse", "PASS", "全部脚本可解析"),
                检查结果("A4b", "前端执行级冒烟", "WARN",
                         "检查器 mock 环境注入失败（工具缺陷·非页面缺陷）",
                         证据=证据片段(str(e))))
    执行异常 = None
    for 名, 文本 in 脚本们:
        try:
            ctx.eval(文本)
        except Exception as e:
            执行异常 = (名, str(e))
            break
    if 执行异常 is not None:
        return (检查结果("A4a", "前端脚本语法 parse", "PASS", "全部脚本可解析"),
                检查结果("A4b", "前端执行级冒烟", "FAIL",
                         f"{执行异常[0]} 执行抛异常（初始化中断）",
                         证据=证据片段(执行异常[1])))
    任务数 = 0
    while ctx.execute_pending_job():
        任务数 += 1
        if 任务数 > 5000:
            return (检查结果("A4a", "前端脚本语法 parse", "PASS", "全部脚本可解析"),
                    检查结果("A4b", "前端执行级冒烟", "FAIL",
                             "微任务不收敛（疑似死循环）", 证据=f"已驱动 {任务数} 个 job"))
    try:
        元信息 = str(ctx.eval('(__元件["元信息"] || {}).textContent || ""'))
        请求记录 = str(ctx.eval("JSON.stringify(__请求记录)"))
    except Exception as e:
        return (检查结果("A4a", "前端脚本语法 parse", "PASS", "全部脚本可解析"),
                检查结果("A4b", "前端执行级冒烟", "WARN",
                         "检查器读取断言值失败（工具缺陷·非页面缺陷）",
                         证据=证据片段(str(e))))
    初值 = 元素初值.get("元信息", "")
    if "数据时刻" in 元信息:
        return (检查结果("A4a", "前端脚本语法 parse", "PASS",
                         f"全部脚本可解析（{len(脚本们)} 条）"),
                检查结果("A4b", "前端执行级冒烟", "PASS",
                         f"数据链已活：元信息=「{证据片段(元信息, 80)}」（初值「{初值}」已被替换）",
                         证据=f"请求={证据片段(请求记录, 120)}"))
    if 元信息 and 元信息 != 初值 and "中断" not in 元信息 and "超时" not in 元信息 and "连接中" not in 元信息:
        # 成功文案可能演进（如「数据时刻」改词）——未见失败字样即视为活·详情留实际值
        return (检查结果("A4a", "前端脚本语法 parse", "PASS",
                         f"全部脚本可解析（{len(脚本们)} 条）"),
                检查结果("A4b", "前端执行级冒烟", "PASS",
                         f"元信息已更新为「{证据片段(元信息, 80)}」",
                         证据=f"请求={证据片段(请求记录, 120)}"))
    return (检查结果("A4a", "前端脚本语法 parse", "PASS",
                     f"全部脚本可解析（{len(脚本们)} 条）"),
            检查结果("A4b", "前端执行级冒烟", "FAIL",
                     "元信息未被数据替换——" +
                     ("仍为初值（前端脚本未完成数据链·#155 形态）" if 元信息 == 初值 else "走了失败分支"),
                     证据=f"元信息=「{证据片段(元信息, 80)}」·请求记录={证据片段(请求记录, 120)}"))


# ---- A/B 两组检查 ----

_A组跳过项 = (("A2", "页面结构"), ("A3", "/api/board 结构"),
              ("A4a", "前端脚本语法 parse"), ("A4b", "前端执行级冒烟"),
              ("A5", "外链资源"))


def 查看板(基址, 超时秒, 结果们, quickjs模块):
    """A 组编排：看板 8301。A1 网关失败则 A2~A5 标跳过（INFO·不刷 FAIL）。"""
    码, 体, 错, 毫秒 = 取地址(基址, "/", 超时秒)
    if 码 != 200:
        结果们.append(检查结果("A1", "看板页面可达", "FAIL",
                              f"GET {基址}/ 未返回 200（码={码 or '无响应'}·{毫秒}ms）",
                              证据=证据片段(错 or 体)))
        for 编号, 名 in _A组跳过项:
            结果们.append(检查结果(编号, 名, "INFO", "跳过（A1 页面不可达）"))
        return
    结果们.append(检查结果("A1", "看板页面可达", "PASS", f"HTTP 200·{毫秒}ms·{len(体)} 字节"))
    解析 = _页面结构检查(体, 结果们)
    板面数据 = _板面结构检查(基址, 超时秒, 结果们)
    _前端冒烟检查(基址, 超时秒, 解析, 板面数据, len(体), 结果们, quickjs模块)
    _外链资源检查(基址, 超时秒, 解析, 结果们)


def _页面结构检查(体, 结果们):
    """A2：解析页面 HTML 并核对关键元素 id；返回 页面解析器（失败时 None）。"""
    解析 = 页面解析器()
    try:
        解析.feed(体)
    except Exception as e:      # HTMLParser 容错强·几乎不抛；仍兜底不炸栈
        结果们.append(检查结果("A2", "页面结构", "WARN", "HTML 解析异常（工具侧）",
                             证据=证据片段(str(e))))
        return None
    缺 = [i for i in 看板必备元素 if i not in 解析.元素id集]
    if 缺:
        结果们.append(检查结果("A2", "页面结构", "FAIL",
                              f"关键元素 id 缺失 {len(缺)} 个",
                              证据="缺失：" + "、".join(缺)))
    else:
        结果们.append(检查结果("A2", "页面结构", "PASS",
                              f"关键元素齐全（{len(看板必备元素)} 项）"))
    return 解析


def _板面结构检查(基址, 超时秒, 结果们):
    """A3：/api/board 可达 + 必需字段/类型；返回板面数据字典（不可用/残缺时 None
    ——A4b 将以最小数据继续且 A3 已显式 FAIL·防级联乱报 #145）。"""
    码, 体, 错, _ = 取地址(基址, "/api/board", 超时秒)
    if 码 != 200:
        结果们.append(检查结果("A3", "/api/board 结构", "FAIL",
                              f"GET {基址}/api/board 未返回 200（码={码 or '无响应'}）",
                              证据=证据片段(错 or 体)))
        return None
    try:
        板面数据 = json.loads(体)
    except ValueError:
        结果们.append(检查结果("A3", "/api/board 结构", "FAIL", "响应非 JSON",
                             证据=证据片段(体)))
        return None
    if not isinstance(板面数据, dict):
        结果们.append(检查结果("A3", "/api/board 结构", "FAIL", "响应 JSON 非对象",
                             证据=证据片段(体)))
        return None
    缺字段 = [k for k, 类型 in 看板必需字段.items()
              if k not in 板面数据 or not isinstance(板面数据[k], 类型)]
    if 缺字段:
        结果们.append(检查结果("A3", "/api/board 结构", "FAIL",
                              f"必需字段缺失/类型不符 {len(缺字段)} 个",
                              证据="问题字段：" + "、".join(缺字段) +
                              "·实际顶层键=" + "、".join(sorted(板面数据.keys())[:10])))
        return None
    结果们.append(检查结果("A3", "/api/board 结构", "PASS",
                          f"字段齐全·任务们={len(板面数据['任务们'])}·"
                          f"在飞={len(板面数据['在飞分支们'])}·"
                          f"冲突={len(板面数据['冲突们'])}·时刻={板面数据['时刻']}"))
    return 板面数据


def _前端冒烟检查(基址, 超时秒, 解析, 板面数据, 体长, 结果们, quickjs模块):
    """A4a/A4b：脚本收集（内联+外链）与真 JS 冒烟。"""
    if 解析 is None:
        结果们.append(检查结果("A4a", "前端脚本语法 parse", "INFO", "跳过（页面解析失败）"))
        结果们.append(检查结果("A4b", "前端执行级冒烟", "INFO", "跳过（页面解析失败）"))
        return
    脚本们 = list(解析.脚本们)
    for 源 in 解析.外链脚本:      # 外链脚本并入冒烟面（内联部署下为空）
        源码, 源体, _, _ = 取地址(绝对值(基址, 源), "", 超时秒)
        if 源码 == 200:
            脚本们.append((f"外链 {源}", 源体))
    if not 脚本们:
        结果们.append(检查结果("A4a", "前端脚本语法 parse", "FAIL",
                              "页面无任何脚本（前端逻辑缺失·页面必瘫）",
                              证据=f"HTML {体长} 字节·未见 <script>"))
        结果们.append(检查结果("A4b", "前端执行级冒烟", "INFO", "跳过（无脚本）"))
        return
    数据 = 板面数据 if isinstance(板面数据, dict) else \
        {"意图们": [], "在飞分支们": [], "任务们": [], "冲突们": [],
         "失联秒": 0, "时刻": "00-00 00:00:00"}
    语法结果, 执行结果 = 前端冒烟(脚本们, 数据, 解析.元素初值, quickjs模块)
    结果们.append(语法结果)
    结果们.append(执行结果)


def _外链资源检查(基址, 超时秒, 解析, 结果们):
    """A5：外链样式/脚本可达性（内联部署=无外链·PASS 注明）。"""
    if 解析 is None:
        结果们.append(检查结果("A5", "外链资源", "INFO", "跳过（页面解析失败）"))
        return
    相对 = ("http://", "https://", "data:", "#")
    外链 = [h for h in 解析.外链样式们 if not h.startswith(相对)]
    外链 += [s for s in 解析.外链脚本 if not s.startswith(相对)]
    if not 外链:
        结果们.append(检查结果("A5", "外链资源", "PASS",
                              "内联单文件部署（HTML+CSS+JS 全内联·无外链依赖）"))
        return
    坏 = []
    for 源 in 外链[:20]:
        源码, _, 源错, _ = 取地址(绝对值(基址, 源), "", 超时秒)
        if 源码 != 200:
            坏.append(f"{源}(码={源码 or 源错})")
    if 坏:
        结果们.append(检查结果("A5", "外链资源", "WARN",
                              f"外链资源不可达 {len(坏)}/{len(外链)}（样式/图标降级）",
                              证据=证据片段("、".join(坏))))
    else:
        结果们.append(检查结果("A5", "外链资源", "PASS",
                              f"外链资源全部可达（{len(外链)} 项）"))


def 检队列(基址, 超时秒, 结果们):
    """B 组：集成队列 8300。B3 为信息项（池闲置/静默属正常·不判 FAIL）。"""
    码, 体, 错, 毫秒 = 取地址(基址, "/api/state", 超时秒)
    if 码 != 200:
        结果们.append(检查结果("B1", "队列 /api/state 可达", "FAIL",
                              f"GET {基址}/api/state 未返回 200（码={码 or '无响应'}·{毫秒}ms）",
                              证据=证据片段(错 or 体)))
        结果们.append(检查结果("B2", "队列结构", "INFO", "跳过（B1 不可达）"))
        结果们.append(检查结果("B3", "验证池摘要", "INFO", "跳过（B1 不可达）"))
        return
    结果们.append(检查结果("B1", "队列 /api/state 可达", "PASS", f"HTTP 200·{毫秒}ms"))
    try:
        状态 = json.loads(体)
    except ValueError:
        结果们.append(检查结果("B2", "队列结构", "FAIL", "响应非 JSON", 证据=证据片段(体)))
        结果们.append(检查结果("B3", "验证池摘要", "INFO", "跳过（结构不可读）"))
        return
    缺 = [k for k, 类型 in 队列必需字段.items()
          if k not in 状态 or not isinstance(状态[k], 类型)]
    if 缺:
        结果们.append(检查结果("B2", "队列结构", "FAIL",
                              "必需字段缺失/类型不符", 证据="问题字段：" + "、".join(缺)))
    else:
        结果们.append(检查结果("B2", "队列结构", "PASS",
                              f"字段齐全·队列={len(状态['队列'])}·"
                              f"门禁最近={len(状态['门禁最近'])}·"
                              f"预验任务={len(状态['预验任务'])}"))

    # B3 池摘要（信息项·不判 FAIL——夜间/低负载池闲置属正常·信噪比）
    任务行 = 状态.get("预验任务") or []
    计数 = {}
    执行中 = []
    最新 = None
    for 行 in 任务行:
        态 = 行.get("状态") or "未知"
        计数[态] = 计数.get(态, 0) + 1
        if 态 == "执行中":
            执行中.append(f"{行.get('认领者') or '?'}({行.get('更新时刻') or '?'})")
        if 最新 is None:
            最新 = 行
    摘要 = "预验池：" + ("、".join(f"{k} {v}" for k, v in sorted(计数.items())) if 计数 else "空")
    if 执行中:
        摘要 += "·认领中=" + "、".join(执行中[:5])
    完成 = [r for r in 任务行 if r.get("状态") == "完成"]
    绿红 = f"（绿 {sum(1 for r in 完成 if r.get('绿') == 1)}/"
    绿红 += f"红 {sum(1 for r in 完成 if r.get('绿') == 0)}/"
    绿红 += f"未知 {sum(1 for r in 完成 if r.get('绿') is None)}）" if 完成 else ""
    if 最新:
        摘要 += f"·最新 {最新.get('分支', '?')} [{最新.get('状态')}] {最新.get('更新时刻', '')}{绿红}"
    结果们.append(检查结果("B3", "验证池摘要", "INFO", 摘要))


# ---- 报告 ----

符号 = {"PASS": "✓", "FAIL": "✗", "WARN": "⚠", "INFO": "·"}


def 打印报告(结果们, 看板基址, 队列基址):
    print("== CN 基础设施健康自检（728m）==")
    print(f"   看板目标 {看板基址} · 队列目标 {队列基址}")
    当前组 = None
    for 结 in 结果们:
        组 = "A. 看板 8301" if 结.编号.startswith("A") else "B. 队列 8300"
        if 组 != 当前组:
            print(f"[{组}]")
            当前组 = 组
        行 = f"  {符号[结.级别]} {结.编号} {结.名}"
        if 结.详情:
            行 += f"——{结.详情}"
        print(行)
        if 结.证据 and 结.级别 in ("FAIL", "WARN"):
            print(f"      证据：{结.证据}")
    失败数 = sum(1 for r in 结果们 if r.级别 == "FAIL")
    警告数 = sum(1 for r in 结果们 if r.级别 == "WARN")
    print("结论：" + ("PASS（全部健康）" if 失败数 == 0 else f"FAIL（{失败数} 项不通过）") +
          (f"·{警告数} 项警告" if 警告数 else ""))
    if 失败数:
        print("处置：逐条看「证据」列；看板类问题先 ssh TX_01 查 journalctl -u cn-board -n 30，"
              "队列类问题查 journalctl -u cn-queue -n 30（plans/028 §四 故障处置）")


def 有FAIL(结果们):
    return sum(1 for r in 结果们 if r.级别 == "FAIL") > 0


# ---- 自检（注入式两态）----

def _自检打包页面(脚本替换=None, 追加头=None):
    """用仓库内 board_www 源三件复刻 deploy_board.sh 的内联打包（自检用）。
    脚本替换=[(旧,新)] 注入语法错误等瘫痪态；追加头=在 <head> 后插 HTML 片段。"""
    www = Path(__file__).resolve().parent / "board_www"
    html = (www / "index.html").read_text(encoding="utf-8")
    css = (www / "tokens.css").read_text(encoding="utf-8") + "\n" + \
          (www / "board.css").read_text(encoding="utf-8")
    js = (www / "board.js").read_text(encoding="utf-8")
    for 旧, 新 in (脚本替换 or []):
        assert 旧 in js, f"自检锚点未命中：{旧}"
        js = js.replace(旧, 新)
    html = html.replace('<link rel="stylesheet" href="/tokens.css">', "<style>\n" + css + "\n</style>")
    html = html.replace('<link rel="stylesheet" href="/board.css">', "")
    html = html.replace('<script src="/board.js"></script>', "<script>\n" + js + "\n</script>")
    if 追加头:
        html = html.replace("<head>", "<head>\n" + 追加头, 1)
    return html


def _自检起服务(场景):
    """本地临时 HTTP 服务扮演看板+队列（同端口两路径·按场景 dict 应答）；
    返回 (基址, 实例)。场景=可变 dict·同一服务实例内切场景复用。"""
    import threading
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

    class 处理器(BaseHTTPRequestHandler):
        def log_message(self, *a):      # 静音
            pass

        def _回(self, 码, 体, 类型="application/json; charset=utf-8"):
            数据 = 体.encode("utf-8") if isinstance(体, str) else 体
            self.send_response(码)
            self.send_header("Content-Type", 类型)
            self.send_header("Content-Length", str(len(数据)))
            self.end_headers()
            self.wfile.write(数据)

        def do_GET(self):
            路径 = self.path.split("?")[0]
            if 路径 == "/":
                return self._回(场景["页面码"], 场景["页面"], "text/html; charset=utf-8")
            if 路径 == "/api/board":
                return self._回(场景["板面码"], json.dumps(场景["板面"], ensure_ascii=False))
            if 路径 == "/api/me":
                return self._回(200, '{"已登录": false}')
            if 路径 == "/api/state":
                if 场景["状态体覆写"] is not None:
                    return self._回(场景["状态码"], 场景["状态体覆写"])
                return self._回(场景["状态码"], json.dumps(场景["状态"], ensure_ascii=False))
            return self._回(404, "{}")

    实例 = ThreadingHTTPServer(("127.0.0.1", 0), 处理器)
    threading.Thread(target=实例.serve_forever, daemon=True).start()
    return f"http://127.0.0.1:{实例.server_address[1]}", 实例


def _自检场景表():
    """瘫痪态场景表：每项=(场景名, 场景覆写, [(编号, 期望级别, 证据须含或None)…])。
    FAIL 期望自动附带「证据非空」契约校验（#145：FAIL 必带可排查证据）。"""
    return [
        ("语法坏（#155 复刻：HTTP 200 但 JS 语法错·唯语法/执行级抓住）",
         {"页面": _自检打包页面([("const 概要上限 = 30;", "const 概要上限 = ;;;30;")])},
         [("A1", "PASS", None), ("A4a", "FAIL", "SyntaxError"), ("A4b", "FAIL", None)]),
        ("运行坏（语法过·初始化抛异常）",
         {"页面": _自检打包页面([("const 概要上限 = 30;",
                                 "const 概要上限 = __不存在函数();")])},
         [("A4a", "PASS", None), ("A4b", "FAIL", None)]),
        ("板面缺字段", {"板面": {"任务们": []}},
         [("A3", "FAIL", "意图们")]),
        ("元素缺失", {"页面": _自检打包页面().replace('<span id="元信息">连接中…</span>', "")},
         [("A2", "FAIL", "元信息")]),
        ("队列 500", {"状态码": 500, "状态体覆写": '{"错误": "内部错误"}'},
         [("B1", "FAIL", "500")]),
        ("队列缺字段", {"状态体覆写": '{"队列": []}'},
         [("B2", "FAIL", "门禁最近")]),
        ("外链 404", {"页面": _自检打包页面(追加头='<link rel="stylesheet" href="/不存在.css">')},
         [("A5", "WARN", "不存在.css")]),
    ]


def 自检():
    """注入式两态自检（866 教训：自证须含对目标真实拦截的正反两态）：
    本地临时 HTTP 服务扮演看板+队列（同端口两路径），场景可变——正常态要求
    零 FAIL，每个瘫痪态要求对应项 FAIL 且证据指向根因。"""
    最小板面 = {"意图们": [], "在飞分支们": [
                    {"分支": "任务/9001", "号": "9001", "提交": "abc1234",
                     "提交题": "自检样例", "时刻": "01-02 03:04:05"}],
                "任务们": [{"号": "9001", "标题": "自检样例任务", "状态": "🏃",
                           "优先级": "P3", "就绪": False, "更新时刻": "01-02 03:04"}],
                "冲突们": [], "失联秒": 3, "时刻": "01-02 03:04:05"}
    最小状态 = {"队列": [], "门禁最近": [], "预验任务": [
                    {"序号": 1, "分支": "ci/预验-deadbeef00", "sha": "deadbeef00",
                     "源分支": None, "状态": "完成", "认领者": "自检机", "绿": 1,
                     "认领限制": None, "入队时刻": "01-02 03:00", "更新时刻": "01-02 03:05"}]}
    场景 = {"页面": _自检打包页面(), "页面码": 200, "板面": 最小板面,
            "板面码": 200, "状态": 最小状态, "状态码": 200, "状态体覆写": None}
    基址, 实例 = _自检起服务(场景)
    失败们 = []

    def 签(名, 条件, 补注=""):
        print(("  ✓ " if 条件 else "  ✗ ") + 名 + (f"  [{补注}]" if 补注 and not 条件 else ""))
        if not 条件:
            失败们.append(名)

    def 跑():
        结果们 = []
        查看板(基址, 3.0, 结果们, 找quickjs())
        检队列(基址, 3.0, 结果们)
        return {r.编号: r for r in 结果们}

    def 恢复():
        场景.update({"页面": _自检打包页面(), "页面码": 200, "板面": 最小板面,
                     "板面码": 200, "状态": 最小状态, "状态码": 200, "状态体覆写": None})

    print("== infra_healthcheck 自检（注入式两态）==")

    # ① 正常态：全链健康 → 零 FAIL，且执行级冒烟过（「连接中」被数据替换）
    结果 = 跑()
    签("正常态：A1 页面可达 PASS", 结果["A1"].级别 == "PASS", 结果["A1"].证据)
    签("正常态：A2 页面结构 PASS", 结果["A2"].级别 == "PASS", 结果["A2"].证据)
    签("正常态：A3 板面结构 PASS", 结果["A3"].级别 == "PASS", 结果["A3"].证据)
    签("正常态：A4b 执行级冒烟 PASS（元信息被数据替换）",
        结果["A4b"].级别 == "PASS", f"{结果['A4b'].详情}·{结果['A4b'].证据}")
    签("正常态：B1 队列可达 PASS", 结果["B1"].级别 == "PASS", 结果["B1"].证据)
    签("正常态：B2 队列结构 PASS", 结果["B2"].级别 == "PASS", 结果["B2"].证据)
    签("正常态：全链零 FAIL", not 有FAIL(list(结果.values())))

    # ② 瘫痪态场景表：逐项注入——对应项 FAIL/WARN 且证据点名根因
    for 场景名, 覆写, 期望们 in _自检场景表():
        恢复()
        场景.update(覆写)
        结果 = 跑()
        for 编号, 级别, 须含 in 期望们:
            结 = 结果[编号]
            条件 = 结.级别 == 级别 and (须含 is None or 须含 in (结.证据 + 结.详情))
            if 级别 == "FAIL" and not 结.证据:
                条件 = False            # FAIL 必带证据契约（#145·机器校验）
            名 = f"{场景名}：{编号} {级别}" + (f" 且证据含「{须含}」" if 须含 else "")
            签(名, 条件, f"{结.详情}·{结.证据}")
        if 场景名.startswith("语法坏"):
            签("语法坏：整体 有FAIL=True（退出码将=1）", 有FAIL(list(结果.values())))

    # ⑨ 降级路径（无 quickjs）：弱检查=WARN 不误报 FAIL；但括号未闭合能抓住
    恢复()
    本模块 = sys.modules[__name__]
    原找 = 本模块.找quickjs
    本模块.找quickjs = lambda: None
    try:
        结果 = 跑()
        签("降级：无 quickjs 时 A4a=WARN 弱检查（非 FAIL）", 结果["A4a"].级别 == "WARN",
            f"{结果['A4a'].详情}·{结果['A4a'].证据}")
        签("降级：A4b=WARN（执行级跳过）", 结果["A4b"].级别 == "WARN", 结果["A4b"].详情)
        签("降级：正常页面全链无 FAIL（不误报）", not 有FAIL(list(结果.values())))
        场景["页面"] = _自检打包页面([("const 概要上限 = 30;", "const 概要上限 = (30;")])
        结果 = 跑()
        签("降级：括号未闭合被弱检查 FAIL 且证据含行号",
            结果["A4a"].级别 == "FAIL" and "行" in 结果["A4a"].证据,
            f"{结果['A4a'].详情}·{结果['A4a'].证据}")
    finally:
        本模块.找quickjs = 原找

    恢复()
    实例.shutdown()
    print(f"自检结论：{'全部通过' if not 失败们 else '失败 ' + str(len(失败们)) + ' 项'}")
    return 1 if 失败们 else 0


# ---- 入口 ----

def main(argv=None):
    解析 = argparse.ArgumentParser(
        description="CN 基础设施健康自检（看板 8301/集成队列 8300/验证池·728m）")
    解析.add_argument("--board-url", default=默认看板基址, help=f"看板基址（默认 {默认看板基址}）")
    解析.add_argument("--queue-url", default=默认队列基址, help=f"队列基址（默认 {默认队列基址}）")
    解析.add_argument("--timeout", type=float, default=默认超时秒, help="单请求超时秒（默认 8）")
    解析.add_argument("--json", action="store_true", help="结构化 JSON 输出")
    解析.add_argument("--selftest", action="store_true", help="注入式两态自检（本地临时服务）")
    参数 = 解析.parse_args(argv)
    if 参数.selftest:
        return 自检()
    结果们 = []
    try:
        查看板(参数.board_url, 参数.timeout, 结果们, 找quickjs())
        检队列(参数.queue_url, 参数.timeout, 结果们)
    except KeyboardInterrupt:
        print("中断", file=sys.stderr)
        return 2
    if 参数.json:
        print(json.dumps({"看板": 参数.board_url, "队列": 参数.queue_url,
                          "结果们": [r.字典() for r in 结果们],
                          "有失败": 有FAIL(结果们)}, ensure_ascii=False, indent=2))
    else:
        打印报告(结果们, 参数.board_url, 参数.queue_url)
    return 1 if 有FAIL(结果们) else 0


if __name__ == "__main__":
    sys.exit(main())
