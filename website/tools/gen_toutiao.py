#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""「今日头条」栏目生成器——把每日任务产出的头条文章网页搬上官网。

输入：头条文章草稿目录下的 第*.html（自带样式的独立页面，图片 base64 内嵌；
开头有一块「使用说明」蓝色虚线框 <div class="说明">，属发布操作指引，不上官网）。

输出（website/toutiao/，生成产物不入 git——见仓库 .gitignore，经
scripts/upload_toutiao.sh 单独上传 TX_01，website_auto_deploy.py 发布时
会原样保留现网 toutiao/ 目录）：
  toutiao/index.html                首页（最新一天文章 + 归档导航 + 创刊教程）
  toutiao/2026/index.html           年页（各月导航）
  toutiao/2026/10/index.html        月页（各日导航）
  toutiao/2026/10/28/index.html     日页（该日文章·篇号升序）
  toutiao/articles/第N篇-*.html     文章页（剔除蓝框·补 viewport 与返回链接）

日期口径（10-28 用户质询修正）：**产出日期 ≠ 文件 mtime**——批次生成器重跑会
连带刷新早期篇目的 mtime（实录：第 10~15 篇被刷进当天）。历史篇号的日期以
草稿库「进度与约定.md」生成器分档表 + 发送包 zip 名日期 + 批次台账为准，
硬编码于下方 日期映射；映射未覆盖的篇号（未来新篇）回退 mtime。

用法：
  python website/tools/gen_toutiao.py                     # 默认草稿目录·全量
  python website/tools/gen_toutiao.py --src <草稿目录>    # 指定源目录
  python website/tools/gen_toutiao.py --min 187           # 只收篇号 >= 187
"""
import argparse
import re
import shutil
import sys
import time
from datetime import datetime
from pathlib import Path

默认草稿目录 = Path(r"C:\Users\ChenChao\Documents\cn-头条文章草稿")

# ---------------------------------------------------------------- 日期映射
# 依据 = 进度与约定.md 生成器分档表（16~186）+ 发送包 zip 名 + 批次台账（187+）：
#   16~24=10-11（gen_article2/3/4）｜25~31=10-12（5/6）｜32~39=10-13（8 重写定稿，
#   夹逼 10-12 与 10-14 之间）｜40~48=10-14（9/10/11）｜49~63=10-15（12/13/14/15/16，
#   12/13 漏标按同段 55~63 归 10-15）｜64~76=10-16（17~22，73/75/76 夹逼 74=10-16 与
#   77=10-17 之间）｜77~87=10-17｜88~97=10-18｜98~107=10-19｜108~118=10-20｜
#   119~121=10-21｜122~131=10-22｜132~139=10-23｜140~148=10-24｜149~166=10-25｜
#   167~176=10-26｜177~216=10-27｜217~226+227~232=10-28。
# 第 10~15 篇（gen_article.py 第一批开发动态）：分档表未标日期，第 14 篇正文唯一
#   硬日期证据 2026-10-10，且 16~18 为「10-11 战报」——归 2026-10-10。
# 第 00~09 篇为教程（序言/快速开始/语法基础等），非每日任务产物 → 创刊教程特殊组。
# 映射上限之后的篇号（未来新篇）回退 mtime 当日。
日期映射 = [
    ("2026-10-10", 10, 15),
    ("2026-10-11", 16, 24),
    ("2026-10-12", 25, 31),
    ("2026-10-13", 32, 39),
    ("2026-10-14", 40, 48),
    ("2026-10-15", 49, 63),
    ("2026-10-16", 64, 76),
    ("2026-10-17", 77, 87),
    ("2026-10-18", 88, 97),
    ("2026-10-19", 98, 107),
    ("2026-10-20", 108, 118),
    ("2026-10-21", 119, 121),
    ("2026-10-22", 122, 131),
    ("2026-10-23", 132, 139),
    ("2026-10-24", 140, 148),
    ("2026-10-25", 149, 166),
    ("2026-10-26", 167, 176),
    ("2026-10-27", 177, 216),
    ("2026-10-28", 217, 232),
]
映射上限 = 日期映射[-1][2]
教程上限 = 9   # 00~09 创刊教程特殊组

# 官网导航（与 index.html / gen_docs.py 顶栏同构；当前=今日头条）
NAV_LINKS = [
    ("首页", "./"),
    ("文档", "../docs/"),
    ("快速开始", "../docs/getting-started.html"),
    ("语言参考", "../docs/reference/"),
    ("今日头条", "./"),
    ("CN-OS", "../os/"),
    ("源码 ↗", "https://gitcode.com/ChenChao_GitCode/CN_Language"),
]

LOGO_SVG = ('<svg class="标识-图" viewBox="0 0 512 512" aria-hidden="true">'
            '<path d="M 232 158 H 148 Q 92 158 92 214 V 298 Q 92 354 148 354 H 232" '
            'fill="none" stroke="var(--color-fg, #221F18)" stroke-width="46"/>'
            '<path d="M 292 354 V 158 L 420 354 V 158" fill="none" '
            'stroke="var(--color-fg, #221F18)" stroke-width="46" stroke-linejoin="bevel"/>'
            '<g transform="translate(118.5 189.5) scale(0.26)">'
            '<rect x="48" y="48" width="416" height="416" rx="56" fill="var(--color-accent, #AD2F22)"/>'
            '<rect x="156" y="180" width="200" height="152" fill="none" '
            'stroke="var(--color-accent-fg, #FCFAF6)" stroke-width="34"/>'
            '<rect x="239" y="106" width="34" height="300" fill="var(--color-accent-fg, #FCFAF6)"/>'
            '</g></svg>')

页样式 = """
  .栏目-头 { max-width: 1080px; margin: 0 auto; padding-block: var(--space-7) var(--space-5); }
  .栏目-述 { color: var(--color-fg-muted); line-height: 1.85; margin-top: var(--space-3); max-width: 62ch; }
  .栏目-体 { max-width: 1080px; margin: 0 auto; padding-block-end: var(--space-9); }
  /* 首页双栏：主栏=最新一天文章流；侧栏=归档/月浏览/教程入口。≤900px 折单栏 */
  .首页-格 { display: grid; grid-template-columns: minmax(0, 1fr) 280px;
             gap: var(--space-7); align-items: start; }
  @media (max-width: 900px) { .首页-格 { grid-template-columns: 1fr; } }
  .面包屑-条 { display: flex; flex-wrap: wrap; gap: var(--space-2); align-items: baseline;
               margin: var(--space-5) 0 0; font-size: .92rem; color: var(--color-fg-muted); }
  .面包屑-条 a { color: var(--color-fg-muted); text-decoration: none; }
  .面包屑-条 a:hover { color: var(--color-accent, #AD2F22); }
  .节题 { display: flex; align-items: baseline; gap: var(--space-3); flex-wrap: wrap;
          margin: 0 0 var(--space-4); padding-block-end: var(--space-2);
          border-bottom: 1px solid var(--color-line, rgba(128,128,128,.25)); }
  .侧-节 { margin-block-start: var(--space-6); }
  .节题-主 { font-size: 1.08rem; font-weight: 700; }
  .节题-数 { font-size: .82rem; color: var(--color-fg-muted); }
  .日组-列 { display: grid; gap: var(--space-2); }
  /* 紧凑文章条目：篇号+标题一行，副题第二行小字 */
  .条 { display: flex; flex-direction: column; gap: 3px; padding: 12px 18px;
        border: 1px solid var(--color-line, rgba(128,128,128,.2));
        border-radius: 10px; text-decoration: none; color: inherit;
        transition: border-color .15s, transform .15s; }
  .条:hover { border-color: var(--color-accent, #AD2F22); transform: translateX(2px); }
  .条-题 { font-weight: 600; font-size: 1rem; line-height: 1.55; }
  .条-号 { font-size: .8rem; color: var(--color-fg-muted); margin-right: 10px;
           font-weight: 400; font-variant-numeric: tabular-nums; }
  .条-副 { font-size: .86rem; color: var(--color-fg-muted); line-height: 1.6; }
  /* 侧栏归档行与入口卡 */
  .档组 { display: grid; gap: 6px; }
  .档行 { display: flex; justify-content: space-between; align-items: baseline; gap: var(--space-3);
          padding: 9px 14px; border: 1px solid var(--color-line, rgba(128,128,128,.2));
          border-radius: 8px; text-decoration: none; color: inherit; font-size: .93rem;
          transition: border-color .15s; }
  .档行:hover { border-color: var(--color-accent, #AD2F22); }
  .档行:hover .档行-日 { color: var(--color-accent, #AD2F22); }
  .档行-日 { font-weight: 600; }
  .档行-数 { font-size: .8rem; color: var(--color-fg-muted); white-space: nowrap;
             font-variant-numeric: tabular-nums; }
  .侧卡 { display: block; padding: 14px 16px; border: 1px solid var(--color-line, rgba(128,128,128,.2));
          border-radius: 10px; text-decoration: none; color: inherit; transition: border-color .15s; }
  .侧卡:hover { border-color: var(--color-accent, #AD2F22); }
  .侧卡-题 { display: block; font-weight: 700; font-size: .98rem; }
  .侧卡-述 { display: block; font-size: .84rem; color: var(--color-fg-muted);
             margin-top: 5px; line-height: 1.65; }
  .格排 { display: grid; gap: var(--space-3);
          grid-template-columns: repeat(auto-fill, minmax(240px, 1fr)); }
  .格 { display: flex; align-items: baseline; justify-content: space-between; gap: var(--space-3);
        padding: var(--space-4) var(--space-5);
        border: 1px solid var(--color-line, rgba(128,128,128,.2));
        border-radius: 10px; text-decoration: none; color: inherit;
        transition: border-color .15s, transform .15s; }
  .格:hover { border-color: var(--color-accent, #AD2F22); transform: translateY(-2px); }
  .格-题 { font-weight: 600; }
  .格-数 { font-size: .82rem; color: var(--color-fg-muted); white-space: nowrap; }
  .翻页 { display: flex; justify-content: space-between; gap: var(--space-4);
          margin-top: var(--space-7); }
  .翻页 a { text-decoration: none; color: var(--color-fg-muted); font-size: .95rem;
            padding: var(--space-2) var(--space-4);
            border: 1px solid var(--color-line, rgba(128,128,128,.2)); border-radius: 8px; }
  .翻页 a:hover { color: var(--color-accent, #AD2F22); border-color: var(--color-accent, #AD2F22); }
  @media (max-width: 640px) {
    .条 { padding: 10px 14px; }
  }
"""


def 页骨架(题名, 描述, 主体html, 前缀="./"):
    """栏目页共用骨架：顶栏+主体+页脚（前缀=到 toutiao/ 根的相对路径）。"""
    导航 = "\n".join(
        '      <a href="%s%s"%s>%s</a>' % (
            前缀 if not href.startswith("http") else "",
            href,
            ' aria-current="page"' if 名 == "今日头条" else "",
            名)
        for 名, href in NAV_LINKS
        if 名 != "今日头条" or True)
    return """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%s</title>
<meta name="description" content="%s">
<script>
  (function () {
    try {
      var t = localStorage.getItem("cn-theme");
      if (t === "dark" || t === "light") document.documentElement.setAttribute("data-theme", t);
    } catch (e) {}
  })();
</script>
<link rel="icon" href="%s../assets/brand/favicon.svg" type="image/svg+xml">
<link rel="icon" href="%s../assets/brand/favicon-32.png" sizes="32x32" type="image/png">
<link rel="stylesheet" href="%s../assets/tokens.css?v=1">
<link rel="stylesheet" href="%s../assets/site.css?v=6">
<style>%s</style>
</head>
<body>

<header class="顶栏">
  <div class="容器 顶栏-内">
    <a class="标识" href="%s.">%sCN<span class="标识-点">·</span>语言</a>
    <button class="导航-开关" data-导航钮 aria-expanded="false">菜单</button>
    <nav class="导航" aria-label="站点导航">
%s
      <button class="主题钮" data-主题钮 aria-label="切换主题">月</button>
    </nav>
  </div>
</header>

<main>
%s
</main>

<footer class="页脚">
  <div class="容器">
    <div class="页脚-排">
      <div>
        <p class="页脚-标语">CN<span style="color: var(--color-accent);">·</span>语言</p>
        <p class="页脚-述">全中文语法的系统级编程语言。以母语的直觉，驾驭机器的全部性能。</p>
      </div>
      <div>
        <h4>语言</h4>
        <ul>
          <li><a href="%s../docs/">官方文档</a></li>
          <li><a href="%s../docs/getting-started.html">快速开始</a></li>
          <li><a href="%s../docs/reference/">语言参考</a></li>
        </ul>
      </div>
      <div>
        <h4>栏目</h4>
        <ul>
          <li><a href="%s.">今日头条</a></li>
          <li><a href="%s../os/">CN-OS 蓝图</a></li>
          <li><a href="https://gitcode.com/ChenChao_GitCode/CN_Language" target="_blank" rel="noopener">源码仓库 ↗</a></li>
        </ul>
      </div>
    </div>
    <div class="页脚-底">
      <span>© 2026 CN 语言项目</span>
      <span>栏目更新于 %s</span>
    </div>
  </div>
</footer>

<script src="%s../assets/site.js?v=3"></script>
</body>
</html>
""" % (题名, 描述, 前缀, 前缀, 前缀, 前缀, 页样式, 前缀, LOGO_SVG, 导航,
       主体html, 前缀, 前缀, 前缀, 前缀, 前缀, 生成时刻, 前缀)


def 条目html(篇, 文章前缀):
    副题html = ('\n      <span class="条-副">%s</span>' % 篇["副题"]) if 篇["副题"] else ""
    return ('      <a class="条" href="%s%s">\n'
            '        <span class="条-题"><span class="条-号">第 %d 篇</span>%s</span>%s\n'
            "      </a>" % (文章前缀, 篇["源"].name, 篇["号"], 篇["题"], 副题html))


def 解析篇目(草稿: Path, 最小篇号: int):
    """扫描 第N篇-标题.html → [{号, 题, 源, 日期, 教程, 副题}]；篇号倒序。
    日期：映射表优先（历史篇），未覆盖回退 mtime；00~09 = 创刊教程组。"""
    篇目 = []
    for f in sorted(草稿.glob("第*.html")):
        m = re.match(r"第(\d+)篇-(.+)\.html$", f.name)
        if not m:
            continue
        号 = int(m.group(1))
        if 号 < 最小篇号:
            continue
        原文 = f.read_text(encoding="utf-8", errors="replace")
        # 副题尽力取蓝框备选标题 B 行（有则列表展示，无则留空）
        副 = ""
        蓝 = re.search(r'<div class="说明">.*?</div>', 原文, flags=re.S)
        if 蓝:
            mb = re.search(r"B\.\s*([^<\r\n]+)", 蓝.group(0))
            if mb:
                副 = mb.group(1).strip()
        if 号 <= 教程上限:
            日期, 教程 = None, True
        else:
            教程 = False
            日期 = next((d for d, 起, 止 in 日期映射 if 起 <= 号 <= 止), None)
            if 日期 is None:   # 映射外的新篇：mtime 当日
                日期 = datetime.fromtimestamp(f.stat().st_mtime).strftime("%Y-%m-%d")
        篇目.append({"号": 号, "题": m.group(2), "源": f, "日期": 日期,
                     "教程": 教程, "副题": 副, "原文": 原文})
    篇目.sort(key=lambda p: -p["号"])
    return 篇目


def 转文章页(篇: dict) -> str:
    """剔除蓝框（含其样式规则）→ 补 viewport/返回链接/标题。"""
    s = 篇["原文"]
    # 蓝框整块删除（锚定到 <article 为止，防 div 内嵌套误截）
    s2 = re.sub(r'<div class="说明">.*?(?=<article)', "", s, count=1, flags=re.S)
    if s2 == s:
        raise RuntimeError("未找到蓝框：%s" % 篇["源"].name)
    # head 内的 .说明 样式规则一并清除（无元素引用·保持源干净）
    def 清样式(m):
        return re.sub(r"[ \t]*\.说明[^{]*\{[^}]*\}\n?", "", m.group(0))
    s2 = re.sub(r"<style>.*?</style>", 清样式, s2, count=1, flags=re.S)
    # viewport（原页面无·移动端可读）
    s2 = s2.replace("<meta charset=\"UTF-8\">",
                    "<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
                    1)
    # 页面标题（原为「头条草稿 · 文件名」）
    s2 = re.sub(r"<title>.*?</title>",
                "<title>第%d篇 · %s — CN 语言·今日头条</title>" % (篇["号"], 篇["题"]),
                s2, count=1)
    # 正文前插返回栏目链接（内联样式·不依赖官网 CSS）
    返回 = ('<a class="返回栏目" href="../">← 返回「今日头条」栏目</a>\n\n<article class="文章">')
    s2 = s2.replace('<article class="文章">', 返回, 1)
    样式 = ("  .返回栏目 { display:block; max-width:720px; margin:18px auto 0; "
            "font-size:14px; color:#4a7dd6; text-decoration:none; }\n"
            "  .返回栏目:hover { text-decoration:underline; }\n")
    s2 = s2.replace("</style>", 样式 + "</style>", 1)
    return s2


def 日期组(篇目):
    """按日期归组：{日期: [篇…]}（篇号升序）。教程组不进日期体系。"""
    分组 = {}
    for p in sorted((p for p in 篇目 if not p["教程"]), key=lambda p: p["号"]):
        分组.setdefault(p["日期"], []).append(p)
    return 分组


def 条目组块(标题, 数注, 篇们, 文章前缀):
    """一节条目列表（篇号升序）。"""
    条 = "\n".join(条目html(p, 文章前缀) for p in 篇们)
    return ('    <h2 class="节题"><span class="节题-主">%s</span>'
            '<span class="节题-数">%s</span></h2>\n'
            '    <div class="日组-列">\n%s\n    </div>' % (标题, 数注, 条))


def 首页(篇目) -> str:
    """双栏布局：主栏=最新一天文章流（升序）；侧栏=往期归档（平铺各日直达）+
    按月浏览入口+创刊教程入口卡。≤900px 折单栏（主栏在前）。"""
    分组 = 日期组(篇目)
    日期序 = sorted(分组.keys(), reverse=True)
    最新 = 日期序[0]
    y, m, d = 最新.split("-")

    # 侧栏：往期归档（不含最新日·新→旧直达各日）
    档行们 = []
    for 日期 in 日期序[1:]:
        dy, dm, dd = 日期.split("-")
        档行们.append('        <a class="档行" href="%s/">\n'
                      '          <span class="档行-日">%d 月 %d 日</span>\n'
                      '          <span class="档行-数">%d 篇</span>\n'
                      "        </a>" % (日期.replace("-", "/"), int(dm), int(dd), len(分组[日期])))
    # 侧栏：按月浏览（年/月页入口）
    月组 = {}
    for 日期, 篇们 in 分组.items():
        月组.setdefault(日期[:7], []).append(日期)
    月行们 = []
    for 年月 in sorted(月组, reverse=True):
        数 = sum(len(分组[d]) for d in 月组[年月])
        年, 月 = 年月.split("-")
        月行们.append('        <a class="档行" href="%s/">\n'
                      '          <span class="档行-日">%d 年 %d 月</span>\n'
                      '          <span class="档行-数">%d 篇</span>\n'
                      "        </a>" % (年月.replace("-", "/"), int(年), int(月), 数))
    # 侧栏：创刊教程入口卡
    教程们 = sorted((p for p in 篇目 if p["教程"]), key=lambda p: p["号"])
    教程卡 = ""
    if 教程们:
        教程卡 = ('      <div class="侧-节">\n'
                  '        <a class="侧卡" href="tutorial/">\n'
                  '          <span class="侧卡-题">创刊教程 · %d 篇</span>\n'
                  '          <span class="侧卡-述">第 00~%02d 篇 · 栏目开篇：从序言、'
                  "快速开始到标准库速览，零基础认识 CN 语言。</span>\n"
                  "        </a>\n"
                  "      </div>" % (len(教程们), 教程们[-1]["号"]))

    主体 = """  <div class="栏目-头">
    <p class="眉题">开发实录 · 每日连载</p>
    <h1>今日头条</h1>
    <p class="栏目-述">CN 语言项目的每日开发实录：编译器演进、语言设计裁定、
    工程实践与历史教训——篇篇带真实源码与可复现实验，同一批文章在头条号
    「CN-Language中文编程语言」同步发布。共 %(总)d 篇，按产出日期归档。</p>
  </div>
  <div class="栏目-体">
    <div class="首页-格">
      <section>
%(最新节)s
      </section>
      <aside>
        <h2 class="节题"><span class="节题-主">往期归档</span></h2>
        <div class="档组">
%(档行)s
        </div>
        <div class="侧-节">
          <h2 class="节题"><span class="节题-主">按月浏览</span></h2>
          <div class="档组">
%(月行)s
          </div>
        </div>
%(教程卡)s
      </aside>
    </div>
  </div>""" % {"总": len(篇目),
               "最新节": 条目组块("%d 年 %d 月 %d 日" % (int(y), int(m), int(d)),
                                  "最新 · %d 篇 · 篇号升序" % len(分组[最新]),
                                  分组[最新], "articles/"),
               "档行": "\n".join(档行们),
               "月行": "\n".join(月行们),
               "教程卡": 教程卡}
    return 页骨架("今日头条 · CN 语言",
                  "CN 语言项目每日开发实录专栏：编译器演进、语言设计、工程实践——"
                  "同批文章在头条号「CN-Language中文编程语言」同步发布。",
                  主体, 前缀="./")


def 教程页(教程们) -> str:
    """创刊教程独立页：与新闻流分离（首页仅侧栏入口卡指来）。"""
    主体 = """  <div class="栏目-头">
    <nav class="面包屑-条"><a href="../">今日头条</a> › <span>创刊教程</span></nav>
    <h1>创刊教程</h1>
    <p class="栏目-述">栏目开篇的第 00~%02d 篇：不依赖每日任务时段，从零认识
    CN 语言——序言、快速开始、语法基础、类与泛型、安全与并发、进阶、语言参考与
    标准库速览。全部示例经编译器实测。按篇号升序排列。</p>
  </div>
  <div class="栏目-体">
    <div class="日组-列">
%s
    </div>
  </div>""" % (教程们[-1]["号"], "\n".join(条目html(p, "../articles/") for p in 教程们))
    return 页骨架("创刊教程 · 今日头条 · CN 语言",
                  "今日头条栏目创刊教程：第 00~09 篇，零基础认识 CN 语言。",
                  主体, 前缀="../")


def 年页(年, 分组) -> str:
    月组 = {}
    for 日期, 篇们 in 分组.items():
        if 日期.startswith(年):
            月组.setdefault(日期[5:7], []).append(日期)
    格 = []
    for 月 in sorted(月组, reverse=True):
        数 = sum(len(分组[d]) for d in 月组[月])
        格.append('        <a class="格" href="%s/">\n'
                  '          <span class="格-题">%d 月</span>\n'
                  '          <span class="格-数">%d 篇</span>\n'
                  "        </a>" % (月, int(月), 数))
    主体 = """  <div class="栏目-头">
    <nav class="面包屑-条"><a href="../">今日头条</a> › <span>%s 年</span></nav>
    <h1>%s 年</h1>
  </div>
  <div class="栏目-体">
    <div class="格排">
%s
    </div>
  </div>""" % (年, 年, "\n".join(格))
    return 页骨架("%s 年 · 今日头条 · CN 语言" % 年,
                  "今日头条栏目 %s 年归档" % 年, 主体, 前缀="../")


def 月页(年月, 分组) -> str:
    年, 月 = 年月.split("-")
    日们 = sorted((d for d in 分组 if d.startswith(年月)), reverse=True)
    格 = []
    for 日期 in 日们:
        格.append('        <a class="格" href="%s/">\n'
                  '          <span class="格-题">%d 月 %d 日</span>\n'
                  '          <span class="格-数">%d 篇</span>\n'
                  "        </a>" % (日期[8:], int(月), int(日期[8:]), len(分组[日期])))
    主体 = """  <div class="栏目-头">
    <nav class="面包屑-条"><a href="../../">今日头条</a> › <a href="../">%s 年</a> › <span>%s 月</span></nav>
    <h1>%s 年 %s 月</h1>
  </div>
  <div class="栏目-体">
    <div class="格排">
%s
    </div>
  </div>""" % (年, 月, 年, 月, "\n".join(格))
    return 页骨架("%s 年 %s 月 · 今日头条 · CN 语言" % (年, 月),
                  "今日头条栏目 %s 年 %s 月归档" % (年, 月), 主体, 前缀="../../")


def 日页(日期, 篇们, 前日, 后日) -> str:
    年, 月, 日 = 日期.split("-")
    面包 = ('    <nav class="面包屑-条"><a href="../../../">今日头条</a> › '
            '<a href="../../">%s 年</a> › <a href="../">%s 月</a> › <span>%s 日</span></nav>'
            % (年, 月, 日))

    def 日链(目标):   # 跨月/跨年从日页恒定向上三级再下行（toutiao/Y/M/D/ 形态）
        y2, m2, d2 = 目标.split("-")
        return "../../../%s/%s/%s/" % (y2, m2, d2)

    主体 = """  <div class="栏目-头">
%(面包)s
    <h1>%(年)s 年 %(月)s 月 %(日)s 日</h1>
    <p class="栏目-述">当日 %(n)s 篇 · 篇号升序</p>
  </div>
  <div class="栏目-体">
    <div class="日组-列">
%(条)s
    </div>
    <nav class="翻页">%(前)s<span></span>%(后)s</nav>
  </div>""" % {"面包": 面包, "年": 年, "月": 月, "日": 日, "n": len(篇们),
               "条": "\n".join(条目html(p, "../../../articles/") for p in 篇们),
               "前": ('<a href="%s">← %s</a>' % (日链(前日), 前日)) if 前日 else "<span></span>",
               "后": ('<a href="%s">%s →</a>' % (日链(后日), 后日)) if 后日 else "<span></span>"}
    return 页骨架("%s 年 %s 月 %s 日 · 今日头条 · CN 语言" % (年, 月, 日),
                  "今日头条栏目 %s-%s-%s 全天文章" % (年, 月, 日), 主体, 前缀="../../../")


def 主() -> int:
    global 生成时刻
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", default=str(默认草稿目录), help="头条文章草稿目录")
    ap.add_argument("--out", default=None, help="输出目录（默认 website/toutiao）")
    ap.add_argument("--min", type=int, default=0, help="只收篇号 >= N（默认全量）")
    a = ap.parse_args()

    草稿 = Path(a.src)
    if not 草稿.is_dir():
        print("✗ 草稿目录不存在：%s" % 草稿, file=sys.stderr)
        return 1
    输出 = Path(a.out) if a.out else Path(__file__).resolve().parent.parent / "toutiao"
    文章目录 = 输出 / "articles"

    篇目 = 解析篇目(草稿, a.min)
    if not 篇目:
        print("✗ 未在 %s 找到 第*.html" % 草稿, file=sys.stderr)
        return 1

    if 文章目录.exists():
        shutil.rmtree(文章目录)
    文章目录.mkdir(parents=True)
    for p in 篇目:
        (文章目录 / p["源"].name).write_text(转文章页(p), encoding="utf-8")

    生成时刻 = time.strftime("%Y-%m-%d %H:%M")
    分组 = 日期组(篇目)
    日期序 = sorted(分组.keys(), reverse=True)

    (输出 / "index.html").write_text(首页(篇目), encoding="utf-8")
    教程们 = sorted((p for p in 篇目 if p["教程"]), key=lambda p: p["号"])
    if 教程们:
        教程目录 = 输出 / "tutorial"
        教程目录.mkdir(parents=True, exist_ok=True)
        (教程目录 / "index.html").write_text(教程页(教程们), encoding="utf-8")
    年们 = sorted({d[:4] for d in 日期序}, reverse=True)
    for 年 in 年们:
        年目录 = 输出 / 年
        年目录.mkdir(parents=True, exist_ok=True)
        (年目录 / "index.html").write_text(年页(年, 分组), encoding="utf-8")
        月们 = sorted({d[5:7] for d in 日期序 if d.startswith(年)}, reverse=True)
        for 月 in 月们:
            月目录 = 年目录 / 月
            月目录.mkdir(parents=True, exist_ok=True)
            (月目录 / "index.html").write_text(月页("%s-%s" % (年, 月), 分组), encoding="utf-8")
    # 日页（升序 + 前后日翻页按有文章日期序列）
    升序 = list(reversed(日期序))
    for i, 日期 in enumerate(升序):
        前日 = 升序[i - 1] if i > 0 else None
        后日 = 升序[i + 1] if i < len(升序) - 1 else None
        目录 = 输出 / 日期.replace("-", "/")
        目录.mkdir(parents=True, exist_ok=True)
        (目录 / "index.html").write_text(
            日页(日期, 分组[日期], 前日, 后日), encoding="utf-8")

    print("✓ 今日头条栏目已生成：%d 篇（%s ~ %s）· %d 个日期页 → %s"
          % (len(篇目), 篇目[-1]["源"].stem, 篇目[0]["源"].stem, len(日期序), 输出))
    return 0


if __name__ == "__main__":
    sys.exit(主())
