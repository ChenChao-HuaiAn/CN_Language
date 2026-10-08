# -*- coding: utf-8 -*-
"""CN 语言文档站·标准库文档生成器（任务 268 文档补全批）

数据驱动：模块页数据在 docs_data_*.py（每页=章节+函数条目列表），
本文件只做三件事：
  1. generate  — 渲染 stdlib.html 索引页 + stdlib/ 模块页 + builtin.html
  2. verify    — 把全部条目示例包壳成完整 .cn 程序，逐个 编译+运行+比对输出
  3. check     — 站内断链/锚点自检

用法（仓库根目录）：
  python website/tools/gen_docs.py            # 生成 HTML
  python website/tools/gen_docs.py --verify   # 示例编译验证（需 target/cn.exe）
  python website/tools/gen_docs.py --check    # 断链/锚点自检

示例验证约定：条目.示例 = 主函数体内语句（不含 返回）；验证壳 =
  页.导入头 + "函数 主() -> 整32 {" + 示例 + "返回 0;}"。
  需要顶层定义的条目直接给 原始=完整程序。
"""
import html
import io
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SITE = os.path.dirname(HERE)                # website/
DOCS = os.path.join(SITE, "docs")           # website/docs/
REPO = os.path.dirname(SITE)                # 仓库根

# ---------------------------------------------------------------- 数据装载
sys.path.insert(0, HERE)
import docs_data_core      # noqa: E402
import docs_data_mapset    # noqa: E402
import docs_data_text      # noqa: E402
import docs_data_io        # noqa: E402
import docs_data_builtin   # noqa: E402
import docs_data_start     # noqa: E402
import docs_data_tut       # noqa: E402
import docs_data_tut2      # noqa: E402
import docs_data_adv       # noqa: E402
import docs_data_ref1      # noqa: E402
import docs_data_ref2      # noqa: E402
import docs_data_ref3      # noqa: E402

MODULES = (
    docs_data_core.PAGES
    + docs_data_mapset.PAGES
    + docs_data_text.PAGES
    + docs_data_io.PAGES
    + docs_data_builtin.PAGES
)

# 通用叙述页（快速开始/教程/进阶/语言参考/CLI）：文件路径（相对 docs/）→ 页定义
GEN_PAGES = (
    docs_data_start.PAGES
    + docs_data_tut.PAGES
    + docs_data_tut2.PAGES
    + docs_data_adv.PAGES
    + docs_data_ref1.PAGES
    + docs_data_ref2.PAGES
    + docs_data_ref3.PAGES
)

# 模块页顺序链（页脚导航 + 索引页排序）：内置 → 各模块 → CLI（站内已有页）
CHAIN = ["stdlib.html", "builtin.html"] + [
    m["文件"] for m in MODULES if m["文件"] != "builtin.html"] + ["cli.html"]

CSS_VER = {"site": 4, "tokens": 2, "hl": 1, "js": 2}   # 改资源必 bump ?v=N


def esc(s):
    return html.escape(s, quote=False)


# ---------------------------------------------------------------- 页面骨架
# 全站线性阅读链（页脚导航+侧栏高亮的唯一依据）：文件相对 docs/ 路径 → 显示题名
READ_CHAIN = [
    ("index.html", "文档总览"),
    ("getting-started.html", "快速开始"),
    ("tutorial-syntax.html", "教程·语法基础"),
    ("tutorial-oop.html", "教程·类与泛型"),
    ("tutorial-safety.html", "教程·安全与并发"),
    ("tutorial-advanced.html", "教程·进阶篇"),
    ("reference/index.html", "参考总览"),
    ("reference/lexical.html", "词法结构"),
    ("reference/types.html", "类型系统"),
    ("reference/control-flow.html", "控制流"),
    ("reference/functions.html", "函数"),
    ("reference/oop.html", "类与对象"),
    ("reference/errors.html", "错误处理"),
    ("reference/concurrency.html", "并发"),
    ("reference/modules.html", "模块系统"),
    ("stdlib.html", "标准库总览"),
    ("cli.html", "命令行参考"),
]

SIDE_GROUPS = [
    ("入门", [("index.html", "文档总览"), ("getting-started.html", "快速开始")]),
    ("教程", [("tutorial-syntax.html", "语法基础"),
              ("tutorial-oop.html", "类与泛型"),
              ("tutorial-safety.html", "安全与并发"),
              ("tutorial-advanced.html", "进阶篇")]),
    ("语言参考", [("reference/index.html", "参考总览"),
                 ("reference/lexical.html", "词法结构"),
                 ("reference/types.html", "类型系统"),
                 ("reference/control-flow.html", "控制流"),
                 ("reference/functions.html", "函数"),
                 ("reference/oop.html", "类与对象"),
                 ("reference/errors.html", "错误处理"),
                 ("reference/concurrency.html", "并发"),
                 ("reference/modules.html", "模块系统")]),
    ("标准库", [("stdlib.html", "标准库总览")] + [
        ("stdlib/" + m["文件"], m["题名"]) for m in MODULES]),
    ("资源", [("cli.html", "命令行参考")]),
]


def 骨架(题名, 描述, 正文html, 目录项, 当前路径, 页脚导航, 侧栏当前=None):
    """文档页骨架：顶栏+侧栏+正文+目录+页脚（数据驱动·当前路径=相对 docs/ 路径）。"""
    rel = os.path.relpath(DOCS, os.path.dirname(os.path.join(DOCS, 当前路径)))
    rel = rel.replace("\\", "/")
    if not rel.endswith("/"):
        rel += "/"
    if rel == "./":
        rel = ""
    a = lambda href: href if href.startswith("http") else (rel + href)  # noqa: E731
    侧栏 = []
    for 组题, 链接们 in SIDE_GROUPS:
        项 = []
        for 路径, 文本 in 链接们:
            cur = ' aria-current="page"' if 路径 == 当前路径 else ""
            项.append('<a href="%s"%s>%s</a>' % (a(路径), cur, 文本))
        侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">%s</p>%s</div>'
                    % (组题, "".join(项)))
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">资源</p>'
                '<a href="%s" target="_blank" rel="noopener">源码仓库 ↗</a></div>'
                % "https://gitcode.com/ChenChao_GitCode/CN_Language")

    toc = "".join('<a href="#%s">%s</a>' % (i, t) for i, t in 目录项)
    上面, 下面 = 页脚导航

    def 页脚项(目标, 方向):
        if 目标 is None:
            return ""
        if 目标 == "cli.html":
            return ('<a class="下一" href="%s"><span class="向">下一页 →</span>'
                    '<span class="题">命令行参考</span></a>' % a("cli.html"))
        if 目标 == "stdlib.html":
            return ('<a href="%s"><span class="向">← 上一页</span>'
                    '<span class="题">标准库总览</span></a>' % a("stdlib.html"))
        for m in MODULES:
            if m["文件"] == 目标:
                cls = "" if 方向 == "上" else "下一"
                向 = "← 上一页" if 方向 == "上" else "下一页 →"
                return ('<a class="%s" href="%s"><span class="向">%s</span>'
                        '<span class="题">%s</span></a>'
                        % (cls, a("stdlib/" + m["文件"]), 向, m["题名"]))
        # 通用页（READ_CHAIN）
        for 路径, 文本 in READ_CHAIN:
            if 路径 == 目标:
                cls = "" if 方向 == "上" else "下一"
                向 = "← 上一页" if 方向 == "上" else "下一页 →"
                return ('<a class="%s" href="%s"><span class="向">%s</span>'
                        '<span class="题">%s</span></a>' % (cls, a(路径), 向, 文本))
        return ""

    nav = ""
    if 上面 or 下面:
        nav = '<nav class="页脚导航">%s%s</nav>' % (
            页脚项(上面, "上"), 页脚项(下面, "下"))
    return '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%s · CN 语言</title>
<meta name="description" content="%s">
<script>
  (function () {
    try {
      var t = localStorage.getItem("cn-theme");
      if (t === "dark" || t === "light") document.documentElement.setAttribute("data-theme", t);
    } catch (e) {}
  })()
</script>
<link rel="stylesheet" href="%s../assets/tokens.css?v=%d">
<link rel="stylesheet" href="%s../assets/site.css?v=%d">
</head>
<body>

<header class="顶栏">
  <div class="容器 顶栏-内">
    <a class="标识" href="%s">CN<span class="标识-点">·</span>语言</a>
    <button class="导航-开关" data-导航钮 aria-expanded="false">菜单</button>
    <nav class="导航" aria-label="站点导航">
      <a href="%s">首页</a>
      <a href="%s" aria-current="page">文档</a>
      <a href="%s">快速开始</a>
      <a href="%s">语言参考</a>
      <a href="%s">标准库</a>
      <a href="%s">CN-OS</a>
      <a href="https://gitcode.com/ChenChao_GitCode/CN_Language" target="_blank" rel="noopener">源码 ↗</a>
      <button class="主题钮" data-主题钮 aria-label="切换主题">月</button>
    </nav>
  </div>
</header>

<main class="容器">
  <div class="文档骨架">
    <aside class="侧栏" aria-label="文档导航">%s</aside>
    <article class="正文">%s%s</article>
    <aside class="目录" aria-label="本页目录">
      <p class="目录-题">本页目录</p>
%s
    </aside>
  </div>
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
          <li><a href="%s">官方文档</a></li>
          <li><a href="%s">快速开始</a></li>
          <li><a href="%s">语言参考</a></li>
          <li><a href="%s">标准库</a></li>
        </ul>
      </div>
      <div>
        <h4>项目</h4>
        <ul>
          <li><a href="https://gitcode.com/ChenChao_GitCode/CN_Language" target="_blank" rel="noopener">源码仓库 ↗</a></li>
          <li><a href="%s">CN-OS 蓝图</a></li>
          <li><a href="%s">命令行参考</a></li>
        </ul>
      </div>
      <div>
        <h4>状态</h4>
        <ul>
          <li>自举固定点已达成</li>
          <li>v2 自举重建进行中</li>
          <li>license：见仓库</li>
        </ul>
      </div>
    </div>
    <div class="页脚-底">
      <span>© 2026 CN 语言项目</span>
      <span>京ICP备00000000号（备案办理中·占位）</span>
    </div>
  </div>
</footer>

<script src="%s../assets/cn-highlight.js?v=%d"></script>
<script src="%ssite.js?v=%d"></script>
</body>
</html>
''' % (题名, 描述, rel, CSS_VER["tokens"], rel, CSS_VER["site"],
       rel + "../" if rel else "../",  # 标识→站点根
       rel + "../", rel, a("getting-started.html"), a("reference/"),
       a("stdlib.html"), rel + "../os/" if rel else "../os/",
       "".join(侧栏), 正文html, nav, toc,
       rel + ".././" if rel else "./", a("getting-started.html"),
       a("reference/"), a("stdlib.html"), rel + "../os/" if rel else "../os/",
       a("cli.html"),
       rel, CSS_VER["hl"], rel, CSS_VER["js"])


# ---------------------------------------------------------------- 条目渲染
def 码窗(名, 代码, 输出=None):
    out = ""
    if 输出 is not None:
        out = ('\n<p class="输出行">输出：%s</p>' % esc(输出))
    return ('<div class="码窗"><p class="码窗-顶"><span class="码窗-名">%s</span>'
            '<span class="码窗-徽">CN</span></p>'
            '<pre><code class="language-cn">%s</code></pre></div>%s'
            % (esc(名), esc(代码.strip("\n")), out))


def 条目html(条, 页导入, 综合窗名=None):
    徽 = "".join(' <span class="签%s">%s</span>' % (
        {"不安全": " 签-赭", "内部": " 签-灰", "常量": " 签-绿"}.get(b, ""), b)
        for b in 条.get("徽", []))
    h = ['<section class="函数条" id="%s">' % 条["id"]]
    h.append('<h3><code>%s</code>%s</h3>' % (esc(条["名"]), 徽))
    h.append('<p class="函数定义"><code>%s</code></p>' % 条["签"])
    h.append('<p>%s</p>' % 条["述"])
    if 条.get("参数"):
        rows = "".join('<tr><td><code>%s</code></td><td><code>%s</code></td><td>%s</td></tr>'
                       % (n, t, d) for n, t, d in 条["参数"])
        h.append('<table class="表 参数表"><thead><tr><th>参数</th><th>类型</th>'
                 '<th>说明</th></tr></thead><tbody>%s</tbody></table>' % rows)
    if 条.get("返回"):
        h.append('<p><strong>返回</strong>　%s</p>' % 条["返回"])
    if 条.get("注意"):
        h.append('<div class="批注 批注-赭">%s</div>' % 条["注意"])
    示 = 条.get("示例")
    if 示:
        名 = 综合窗名 or (条["id"] + ".cn")
        h.append(码窗(名, 示, 条.get("输出")))
    h.append("</section>")
    return "\n".join(h)


def 模块页html(m, 上一, 下一):
    页径 = "stdlib/" + m["文件"]
    正 = ['<nav class="面包屑" aria-label="面包屑"><a href="../">文档</a> / '
          '<a href="../stdlib.html">标准库</a> / %s</nav>' % m["题名"]]
    正.append("<h1>%s</h1>" % m["题名"])
    正.append('<p style="color: var(--color-fg-muted); font-size: var(--text-lg); line-height: 2;">%s</p>'
              % m["副题"])
    正.append(m.get("头注", ""))
    if m.get("导入头"):
        导入s = "\n".join(m["导入头"])
        正.append(码窗("导入写法", 导入s))
    toc = []
    for 章 in m["章节"]:
        toc.append((章["id"], 章["题名"]))
        正.append('<h2 id="%s">%s</h2>' % (章["id"], 章["题名"]))
        if 章.get("述"):
            正.append("<p>%s</p>" % 章["述"])
        if 章.get("综合"):
            正.append(码窗(章.get("综合名", 章["id"] + ".cn"),
                           章["综合"], 章.get("综合输出")))
        for 条 in 章["条目"]:
            正.append(条目html(条, m.get("导入头")))
    return 骨架("%s · 标准库" % m["题名"], m["副题"].replace("&", "&amp;"),
                "\n".join(正), toc, 页径, (上一, 下一), m["题名"])


# ---------------------------------------------------------------- 索引页
def 索引页html():
    正 = ['<nav class="面包屑" aria-label="面包屑"><a href="./">文档</a> / 标准库</nav>']
    正.append("<h1>标准库</h1>")
    正.append('<p style="color: var(--color-fg-muted); font-size: var(--text-lg); line-height: 2;">'
              '13 个模块、%d 个函数与类方法，全部用 CN 语言写成——每个函数都有定义、'
              '用法与可运行的示例。标准库自身就是最好的进阶教程。</p>'
              % 统计())
    卡 = []
    for m in MODULES:
        n = sum(len(ch["条目"]) for ch in m["章节"])
        卡.append('<a class="库卡" href="stdlib/%s"><span class="库卡-题">%s</span>'
                  '<span class="库卡-述">%s</span>'
                  '<span class="库卡-数">%d 个条目</span></a>'
                  % (m["文件"], m["题名"], m.get("卡片", ""), n))
    正.append('<div class="库卡排">%s</div>' % "".join(卡))

    正.append('<h2 id="速查">全部函数速查</h2>')
    正.append("<p>按模块列出全部公开 API（内部辅助以「内部」徽章标注）。"
              "点击函数名跳转到逐函数文档。</p>")
    正.append('<table class="表"><thead><tr><th>模块</th><th>条目</th><th>一句话</th></tr></thead><tbody>')
    for m in MODULES:
        first = True
        for ch in m["章节"]:
            for 条 in ch["条目"]:
                mod = "" if not first else '<a href="stdlib/%s#%s">%s</a>' % (
                    m["文件"], ch["id"], m["题名"])
                first = False
                内 = ' <span class="签 签-灰">内部</span>' if "内部" in 条.get("徽", []) else ""
                正.append('<tr><td>%s</td><td><a href="stdlib/%s#%s"><code>%s</code></a>%s</td>'
                          '<td>%s</td></tr>'
                          % (mod, m["文件"], 条["id"], esc(条["名"]), 内, 条.get("一句话", "")))
    正.append("</tbody></table>")

    正.append('<h2 id="约定">阅读约定</h2>')
    正.append("""<ul>
<li><strong>结果&lt;T, 整32&gt; 必须检查</strong>——把返回值直接当语句丢弃是编译错误
（「返回值被丢弃未检查」）。本文全部示例遵守此规则。</li>
<li><span class="签 签-赭">不安全</span> = 手动内存/裸指针域函数，须在
<code>不安全</code> 函数或块内调用；标准库容器类方法多为不安全（容器内部管理堆内存），
但<strong>使用容器本身不需要不安全块</strong>——析构自动释放（RAII）。</li>
<li><span class="签 签-灰">内部</span> = 实现细节条目（编译器注入挂点/散列辅助），
供读源码者对照，不建议业务代码直接调用。</li>
<li>示例输出经本地编译器实际运行验证（<code>cn build</code> + 比对 stdout）。</li>
</ul>""")
    toc = [("速查", "全部函数速查"), ("约定", "阅读约定")]
    return 骨架("标准库 · CN 语言",
                "CN 语言标准库：13 个模块逐函数文档——定义、用法、可运行示例。",
                "\n".join(正), toc, "stdlib.html", (None, "builtin.html"), "标准库")


def 统计():
    return sum(len(ch["条目"]) for m in MODULES for ch in m["章节"])


# ---------------------------------------------------------------- 通用叙述页
def 块html(块):
    """叙述块节点 → HTML。tuple 形态按首元素分派；dict 形态=码块（可带 编译足矣）。"""
    if isinstance(块, dict):
        return 码窗(块["名"], 块["代码"], 块.get("输出"))
    t = 块[0]
    if t == "p":
        return "<p>%s</p>" % 块[1]
    if t == "h3":
        return '<h3 id="%s">%s</h3>' % (块[1], 块[1])
    if t == "码文":
        # 非 CN 代码窗（shell/命令行）：只展示，不进示例验证
        return ('<div class="码窗"><p class="码窗-顶"><span class="码窗-名">%s</span>'
                '<span class="码窗-徽">SHELL</span></p>'
                '<pre><code>%s</code></pre></div>' % (esc(块[1]), esc(块[2].strip("\n"))))
    if t == "码":
        名, 代码, 输出 = 块[1], 块[2], 块[3] if len(块) > 3 else None
        return 码窗(名, 代码, 输出)
    if t == "表":
        表头, 行们 = 块[1], 块[2]
        rows = "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % c for c in r)
                       for r in 行们)
        head = "".join("<th>%s</th>" % h for h in 表头)
        return ('<table class="表"><thead><tr>%s</tr></thead>'
                '<tbody>%s</tbody></table>' % (head, rows))
    if t == "批注":
        题, 内容 = 块[1], 块[2]
        变体 = 块[3] if len(块) > 3 else ""
        题行 = '<span class="批注-题">%s</span>' % 题 if 题 else ""
        return '<div class="批注 %s">%s%s</div>' % (变体, 题行, 内容)
    if t == "ul":
        return "<ul>%s</ul>" % "".join("<li>%s</li>" % x for x in 块[1])
    if t == "ol":
        return "<ol>%s</ol>" % "".join("<li>%s</li>" % x for x in 块[1])
    raise ValueError("未知块类型: %r" % (t,))


def 通用页html(页, 上一, 下一):
    正 = []
    # 面包屑：[(相对href|"" 表示纯文本, 文本), ...] 最后一项为当前页（纯文本）
    面 = []
    for h, t in 页["面包屑"]:
        if h:
            面.append('<a href="%s">%s</a>' % (h, t))
        else:
            面.append(t)
    正.append('<nav class="面包屑" aria-label="面包屑">%s</nav>' % " / ".join(面))
    正.append("<h1>%s</h1>" % 页["题名"])
    正.append('<p style="color: var(--color-fg-muted); font-size: var(--text-lg); line-height: 2;">%s</p>'
              % 页["副题"])
    if 页.get("头注"):
        正.append(页["头注"])
    toc = []
    for ch in 页["章节"]:
        toc.append((ch["id"], ch["题名"]))
        正.append('<h2 id="%s">%s</h2>' % (ch["id"], ch["题名"]))
        if ch.get("述"):
            正.append("<p>%s</p>" % ch["述"])
        for 块 in ch.get("块", []):
            正.append(块html(块))
        for 条 in ch.get("条目", []):
            正.append(条目html(条, 页.get("导入头")))
    return 骨架("%s · CN 语言" % 页["题名"], 页["副题"].replace("&", "&amp;"),
                "\n".join(正), toc, 页["文件"], (上一, 下一))


# ---------------------------------------------------------------- 生成
def generate():
    os.makedirs(os.path.join(DOCS, "stdlib"), exist_ok=True)
    with io.open(os.path.join(DOCS, "stdlib.html"), "w", encoding="utf-8", newline="\n") as f:
        f.write(索引页html())
    模块文件 = [m["文件"] for m in MODULES]
    for i, m in enumerate(MODULES):
        上一 = "builtin.html" if i == 0 else 模块文件[i - 1]
        下一 = "cli.html" if i == len(MODULES) - 1 else 模块文件[i + 1]
        p = os.path.join(DOCS, "stdlib", m["文件"])
        with io.open(p, "w", encoding="utf-8", newline="\n") as f:
            f.write(模块页html(m, 上一, 下一))
    for i, 页 in enumerate(GEN_PAGES):
        上一 = "stdlib.html" if i == 0 else GEN_PAGES[i - 1]["文件"]
        下一 = "stdlib.html" if i == len(GEN_PAGES) - 1 else GEN_PAGES[i + 1]["文件"]
        p = os.path.join(DOCS, 页["文件"])
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with io.open(p, "w", encoding="utf-8", newline="\n") as f:
            f.write(通用页html(页, 上一, 下一))
    print("生成：stdlib.html + stdlib/ × %d（%d 条目）+ 通用页 × %d"
          % (len(MODULES), 统计(), len(GEN_PAGES)))


# ---------------------------------------------------------------- 验证
def 条目程序(m, 条):
    """条目示例 → 完整 .cn 程序文本。示例若自带 函数 主(（完整程序形态）直接用。"""
    if 条.get("原始"):
        return 条["原始"]
    body = 条["示例"].rstrip()
    if "函数 主(" in body:
        return body
    导入s = "\n".join(m.get("导入头", []))
    return "%s\n\n函数 主() -> 整32 {\n%s\n    返回 0;\n}\n" % (导入s, body)


def 找编译器():
    """优先本树构建（=当前 develop 源码·含最新语言特性），回退主树。"""
    本树 = os.path.join(REPO, "target", "cn.exe")
    主树 = os.path.normpath(os.path.join(REPO, "..", "CN_Language_C",
                                          "target", "cn.exe"))
    for c in (os.environ.get("CN_EXE"), 本树, 主树):
        if c and os.path.exists(c):
            return c
    return None


def verify(cn_exe=None, 仅失败=False):
    cn = cn_exe or 找编译器()
    if not cn:
        print("找不到编译器")
        return 1
    tmp = tempfile.mkdtemp(prefix="cndocs_")
    总数 = 通过 = 0
    fails = []

    def 跑一个(页标识, 条, 导入头):
        nonlocal 总数, 通过
        示 = 条.get("示例") or 条.get("原始") or 条.get("代码")
        if not 示:
            return
        总数 += 1
        src = os.path.join(tmp, "t%d.cn" % 总数)
        exe = os.path.join(tmp, "t%d.exe" % 总数)
        if 条.get("原始"):
            程序 = 条["原始"]
        else:
            body = 示.rstrip()
            if "函数 主(" in body:
                程序 = body
            else:
                导入s = "\n".join(导入头 or [])
                程序 = "%s\n\n函数 主() -> 整32 {\n%s\n    返回 0;\n}\n" % (导入s, body)
        with io.open(src, "w", encoding="utf-8", newline="\n") as f:
            f.write(程序)
        r = subprocess.run([cn, "build", src, "--output", exe],
                           capture_output=True, text=True,
                           cwd=REPO, errors="replace")
        if 条.get("预期失败"):
            # 负例块：必须编译失败（且诊断含 预期诊断 关键词——给了才比对）
            if r.returncode == 0:
                fails.append((页标识, 条.get("id", "?"), "负例未拦截",
                              "预期编译失败却成功了"))
            elif (条.get("预期诊断") and 条["预期诊断"] not in (r.stdout + r.stderr)):
                fails.append((页标识, 条.get("id", "?"), "负例诊断不符",
                              "缺关键词 %r" % 条["预期诊断"]))
            else:
                通过 += 1
            return
        if r.returncode != 0:
            fails.append((页标识, 条.get("id", "?"), "编译失败", (r.stdout + r.stderr)[-400:]))
            return
        if 条.get("编译足矣"):
            通过 += 1
            return
        run = subprocess.run([exe], capture_output=True, text=True,
                             errors="replace", timeout=30, stdin=subprocess.DEVNULL)
        out = run.stdout.replace("\r\n", "\n")
        期望 = 条.get("输出") if "输出" in 条 else 条.get("综合输出")
        if 期望 is not None and out != 期望:
            fails.append((页标识, 条.get("id", "?"), "输出不符",
                          "期望=%r 实际=%r" % (期望, out)))
            return
        if run.returncode != 0:
            fails.append((页标识, 条.get("id", "?"), "运行退出码 %d" % run.returncode, out[-200:]))
            return
        通过 += 1

    for m in MODULES:
        for ch in m["章节"]:
            验证对象 = []
            if ch.get("综合"):
                验证对象.append(dict(id=ch["id"], 示例=ch["综合"],
                                     输出=ch.get("综合输出"),
                                     原始=ch.get("综合原始"),
                                     编译足矣=ch.get("综合编译足矣")))
            验证对象 += [条 for 条 in ch["条目"]]
            for 条 in 验证对象:
                跑一个(m["文件"], 条, m.get("导入头"))
    for 页 in GEN_PAGES:
        for ch in 页["章节"]:
            if ch.get("综合"):
                跑一个(页["文件"], dict(id=ch["id"] + "·综合", 示例=ch["综合"],
                                   输出=ch.get("综合输出"),
                                   原始=ch.get("综合原始"),
                                   编译足矣=ch.get("综合编译足矣")),
                     页.get("导入头"))
            for 块 in ch.get("块", []):
                if isinstance(块, dict) and 块.get("代码"):
                    跑一个(页["文件"], dict(块, id=块.get("名", "码")), 页.get("导入头"))
                elif isinstance(块, tuple) and 块[0] == "码" and 块[2]:
                    跑一个(页["文件"], dict(id=块[1], 示例=块[2],
                                       输出=块[3] if len(块) > 3 else None,
                                       编译足矣=块[4] if len(块) > 4 else False),
                         页.get("导入头"))
            for 条 in ch.get("条目", []):
                跑一个(页["文件"], 条, 页.get("导入头"))
    print("示例验证：%d/%d 通过（编译+运行+输出比对）" % (通过, 总数))
    for f in fails:
        print("  [失败] %s :: %s — %s\n    %s" % f)
    if not 仅失败 and not fails:
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)
    return 1 if fails else 0


# ---------------------------------------------------------------- 断链自检
def check_links():
    生成目录 = {}
    for root, _, files in os.walk(SITE):
        for fn in files:
            if fn.endswith(".html"):
                p = os.path.relpath(os.path.join(root, fn), SITE).replace("\\", "/")
                生成目录[p] = io.open(os.path.join(root, fn), encoding="utf-8").read()
    坏 = []
    for 页, 文本 in 生成目录.items():
        # 只检查 <a> 的导航链接（link/script 的资源引用不在此列）
        base = os.path.dirname(页)
        for m in re.finditer(r'<a[^>]*href="([^"]*)"', 文本):
            href = m.group(1)
            if href.startswith(("http://", "https://", "mailto:")) or href == "":
                continue
            if "#" in href:
                target, frag = href.split("#", 1)
            else:
                target, frag = href, None
            if target == "":
                t = 页
            else:
                t = os.path.normpath(os.path.join(base, target)).replace("\\", "/")
                if t == "." or t == "..":
                    t = "index.html"
                if target.endswith("/") and not t.endswith("index.html"):
                    t += "/index.html"
            if t not in 生成目录:
                坏.append("%s → %s（页面缺失）" % (页, href))
            elif frag and ('id="%s"' % frag) not in 生成目录[t]:
                坏.append("%s → %s#%s（锚点缺失）" % (页, href, frag))
    if 坏:
        print("断链 %d 处：" % len(坏))
        for b in 坏:
            print("  " + b)
        return 1
    print("断链/锚点自检：%d 页全部通过" % len(生成目录))
    return 0


if __name__ == "__main__":
    if "--verify" in sys.argv:
        sys.exit(verify())
    if "--check" in sys.argv:
        sys.exit(check_links())
    generate()
    sys.exit(check_links())
