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

用例引用（1060·2026-10-18 用户令）：示例条目支持「用例引用」形态——
  块 tuple ("用例", "用例名"[, 说明]) 或条目 dict 键 用例="用例名"：
  展示直接读 tests/e2e/<用例名>/ 的 主.cn 与 主.expected（与 run_e2e 门禁
  同一份文件·单一事实源）；verify 对引用只做映射轻校验（存在性+非负测·
  不重复编译——「进了 develop 的必然绿灯」）。存量手写示例保留过渡，
  迁移工具=docs_migrate_e2e.py（预检→建用例→改写→登记·幂等状态在
  docs_migrate_state.json）。
"""
import html
import io
import os
import platform
import re
import subprocess
import sys
# win GBK console 打印安全（云端 runner stdout=GBK 时 ✓/中文详情崩=假红）
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SITE = os.path.dirname(HERE)                # website/
DOCS = os.path.join(SITE, "docs")           # website/docs/
REPO = os.path.dirname(SITE)                # 仓库根

# 顶栏主 logo（方案戊「怀中印」内联片段·权威源=assets/brand/logo-内联.svg·844）
with io.open(os.path.join(SITE, "assets", "brand", "logo-内联.svg"),
             encoding="utf-8") as _f:
    LOGO_SVG = _f.read().strip()

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

CSS_VER = {"site": 6, "tokens": 2, "hl": 1, "js": 2}   # 改资源必 bump ?v=N


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
<link rel="icon" href="%s../assets/brand/favicon.svg" type="image/svg+xml">
<link rel="icon" href="%s../assets/brand/favicon-32.png" sizes="32x32" type="image/png">
<link rel="icon" href="%s../assets/brand/favicon-16.png" sizes="16x16" type="image/png">
<link rel="stylesheet" href="%s../assets/tokens.css?v=%d">
<link rel="stylesheet" href="%s../assets/site.css?v=%d">
</head>
<body>

<header class="顶栏">
  <div class="容器 顶栏-内">
    <a class="标识" href="%s">%sCN<span class="标识-点">·</span>语言</a>
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
      <span>%s</span>
      <span>京ICP备00000000号（备案办理中·占位）</span>
    </div>
  </div>
</footer>

<script src="%s../assets/cn-highlight.js?v=%d"></script>
<script src="%s../assets/site.js?v=%d"></script>
</body>
</html>
''' % (题名, 描述, rel, rel, rel, rel, CSS_VER["tokens"], rel, CSS_VER["site"],
       rel + "../" if rel else "../",  # 标识→站点根
       LOGO_SVG,                       # 顶栏主 logo（内联·双主题 tokens 自适应）
       rel + "../", rel, a("getting-started.html"), a("reference/"),
       a("stdlib.html"), rel + "../os/" if rel else "../os/",
       "".join(侧栏), 正文html, nav, toc,
       rel + ".././" if rel else "./", a("getting-started.html"),
       a("reference/"), a("stdlib.html"), rel + "../os/" if rel else "../os/",
       a("cli.html"),
       文档基线(),
       rel, CSS_VER["hl"], rel, CSS_VER["js"])


# ---------------------------------------------------------------- 条目渲染
# ---------------------------------------------------------------- 用例引用（1060）
# 用户令（2026-10-18）：官网示例与 E2E 用例同源化——「进了 develop 的必然绿灯，
#   无需重复验证官方案例」。示例条目支持「用例引用」形态（块 tuple ("用例", 名[, 说明])、
#   条目 dict 键 用例=名）：渲染直接读 tests/e2e/<用例名>/ 的 主.cn 与 主.expected
#   ——官网展示与 run_e2e 门禁同一份文件，天然同步零漂移；verify 对引用条目只做
#   映射轻校验（目录/主.cn/主.expected 齐备且非负测），不再重复编译运行。
def 用例目录(用例名):
    """用例名 → 目录：支持「全名」或「<号>_<用例名>」带号目录（1060 迁移用例
    目录名=号_名，docs_data 引用写不含号用例名——按后缀唯一匹配）。"""
    直 = os.path.join(REPO, "tests", "e2e", 用例名)
    if os.path.isdir(直):
        return 直
    import glob as _glob
    命中 = sorted(_glob.glob(os.path.join(REPO, "tests", "e2e", "*_" + 用例名)))
    if len(命中) == 1:
        return 命中[0]
    if len(命中) > 1:
        raise SystemExit("用例名不唯一（多目录后缀匹配）：%s -> %s" % (用例名, 命中))
    return 直


def 用例资源(用例名):
    """读引用用例的 主.cn 源码与 主.expected 期望输出（渲染用·显示即门禁文件）。"""
    目录 = 用例目录(用例名)
    主 = os.path.join(目录, "主.cn")
    if not os.path.isfile(主):
        raise SystemExit("用例引用无效（缺 主.cn）：%s" % 用例名)
    with io.open(主, encoding="utf-8") as f:
        源码 = f.read()
    期 = os.path.join(目录, "主.expected")
    期望 = ""
    if os.path.isfile(期):
        with io.open(期, encoding="utf-8") as f:
            期望 = f.read().rstrip("\n")
    return 源码, 期望


def 用例校验(用例名):
    """引用用例轻校验（verify 分流·不编译不运行）：目录/主.cn/主.expected 齐备
    且非负测。返回错误描述或 None。"""
    目录 = 用例目录(用例名)
    if not os.path.isdir(目录):
        return "用例目录不存在"
    if not os.path.isfile(os.path.join(目录, "主.cn")):
        return "缺 主.cn"
    if not os.path.isfile(os.path.join(目录, "主.expected")):
        return "缺 主.expected"
    with io.open(os.path.join(目录, "主.cn"), encoding="utf-8") as f:
        头 = f.read(800)
    if "预期失败" in 头:
        return "负测用例（预期失败）不可作教学示例"
    return None


def 码窗(名, 代码, 输出=None, 徽="CN"):
    out = ""
    if 输出 is not None:
        out = ('\n<p class="输出行">输出：%s</p>' % esc(输出))
    return ('<div class="码窗"><p class="码窗-顶"><span class="码窗-名">%s</span>'
            '<span class="码窗-徽">%s</span></p>'
            '<pre><code class="language-cn">%s</code></pre></div>%s'
            % (esc(名), esc(徽), esc(代码.strip("\n")), out))


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
    elif 条.get("用例"):
        # 1060：用例引用形态——展示文件即 E2E 门禁文件（同源）
        源码, 期望 = 用例资源(条["用例"])
        h.append(码窗(综合窗名 or "主.cn", 源码, 期望 or None))
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
    if t == "码览":
        # 展示专用码窗（多包工程断面等多文件实测素材）：只展示不进 --verify
        # （素材随 E2E 双编译实测，无法在单文件临时目录中独立复现）；徽默认 CN
        return 码窗(块[1], 块[2], 徽=块[3] if len(块) > 3 else "CN")
    if t == "码":
        名, 代码, 输出 = 块[1], 块[2], 块[3] if len(块) > 3 else None
        return 码窗(名, 代码, 输出)
    if t == "用例":
        # 1060：用例引用形态——展示= tests/e2e/<名>/主.cn（与 run_e2e 门禁同源）
        用例名 = 块[1]
        说明 = 块[2] if len(块) > 2 else None
        源码, 期望 = 用例资源(用例名)
        窗 = 码窗("主.cn", 源码, 期望 or None)
        if 说明:
            return "<p>%s</p>\n%s" % (说明, 窗)
        return 窗
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
    """优先本树构建（=当前 develop 源码·含最新语言特性），回退主树。
    992 Linux 兼容：win 产物 target/cn.exe、linux 产物 target/cn 各列候选。"""
    候选 = [os.environ.get("CN_EXE"),
            os.path.join(REPO, "target", "cn.exe"),
            os.path.join(REPO, "target", "cn"),
            os.path.normpath(os.path.join(REPO, "..", "CN_Language_C", "target", "cn.exe")),
            os.path.normpath(os.path.join(REPO, "..", "CN_Language_C", "target", "cn"))]
    for c in 候选:
        if c and os.path.exists(c):
            return c
    return None


def 运行时平台():
    """本机真实执行平台（只凭运行时探测·不读自报覆盖）——1081/822/run_e2e 同判据。"""
    if sys.platform.startswith("win"):
        return "win-x64"
    return ("linux-arm64" if platform.machine().lower() in ("aarch64", "arm64")
            else "linux-x86_64")


def 目标平台():
    """示例验证的目标平台（992）：CN_DOCS_TARGET 显式覆盖，缺省运行时探测
    （与 gate_quick/ci_daemon/check_cli_contract 平台三分同口径·1081 起单一源）。"""
    显式 = os.environ.get("CN_DOCS_TARGET")
    if 显式:
        return 显式
    return 运行时平台()


def 交叉模拟跳过():
    """1081（2026-10-18）：交叉模拟实例自判——CN_CI_PLATFORM 自报≠本机运行时
    平台（与 run_e2e 判定交叉模拟/check_cli_contract 822 运行级可达同口径·
    单一事实源）。病根（xsim 六轮恒红）：028 xsim 实例（x64 宿主自报 linux-arm64 +
    CN_AS/CN_CXX=arm64 交叉工具链）下本步骤继承交叉变量——宿主 cn 工具链探测
    对 CN_AS/CN_CXX 无条件优先、不感知 --target（cn_main_toolchain.cpp），
    linux-x86_64 汇编喂 aarch64-as 必败。示例验证非平台覆盖职责（992）：
    交叉实例跳过留痕（rc=0 不计红），原生平台实例照常真跑兜底。"""
    自报 = os.environ.get("CN_CI_PLATFORM", "").strip()
    本机 = 运行时平台()
    if not 自报 or 自报 == 本机:
        return False
    print("示例验证：跳过（交叉模拟实例——CN_CI_PLATFORM 自报 %s≠本机 %s："
          "交叉工具链下示例编译必败·示例验证非平台覆盖职责，"
          "由原生平台实例兜底·1081）" % (自报, 本机))
    return True


_基线缓存 = []      # 文档基线标注每进程只算一次（骨架逐页引用）


def 文档基线():
    """页脚版本标注（992）：git 快照=分支 short-sha·提交日期；无 git/异常=优雅降级。
    线上（TX_01 浅克隆 develop）生成形态=「文档基线：develop 4fee0cb·2026-10-17·
    示例经编译器实测」；detached HEAD（reset --hard FETCH_HEAD 形态）按 develop 记。"""
    if _基线缓存:
        return _基线缓存[0]
    标注 = "文档基线：本地生成"
    try:
        def git(*参):
            r = subprocess.run(["git", "-C", REPO] + list(参), capture_output=True,
                               text=True, timeout=15, errors="replace")
            return r.stdout.strip() if r.returncode == 0 else ""

        sha = git("rev-parse", "--short", "HEAD")
        日期 = git("show", "-s", "--format=%cs", "HEAD")
        分支 = git("rev-parse", "--abbrev-ref", "HEAD")
        if not 分支 or 分支 == "HEAD":
            分支 = "develop"
        if sha and 日期:
            标注 = "文档基线：%s %s·%s·示例经编译器实测" % (分支, sha, 日期)
    except Exception:
        标注 = "文档基线：本地生成"
    _基线缓存.append(标注)
    return 标注


def verify(cn_exe=None, 仅失败=False):
    cn = cn_exe or 找编译器()
    if not cn:
        print("找不到编译器")
        return 1
    # 1081：交叉模拟实例前置跳过（留痕·rc=0 不计红——判据见 交叉模拟跳过 docstring）
    if 交叉模拟跳过():
        return 0
    平台 = 目标平台()                     # 992：显式目标平台（win 缺省=win-x64 会走 MSVC 链）
    产物后缀 = ".exe" if 平台 == "win-x64" else ""
    tmp = tempfile.mkdtemp(prefix="cndocs_")
    跳过 = 0

    # 1015（2026-10-18）：收集-并行执行-汇总三段式——原逐例串行在 win 家机实测
    #   268 例约 25-30 分钟（每例 cn build 走 vcvars 链）·多轮集成被 600s/2400s
    #   超时反复误杀（963/983/790 伞批三实录）；并行化按「单例独立子目录」隔离
    #   （文件 I/O 示例写 笔记.txt/日志.txt 曾落 cwd——992 起 cwd=tmp·并行下每例
    #   再隔一层 t<N>/ 根除跨例互写），编译/运行互不共享可写面；编号=输入顺序
    #   预分配·结果经 map 保序汇总——判定行为与旧实现逐字一致。
    #   并发度=CN_DOC_VERIFY_JOBS 环境变量（缺省 8；1=退化为串行旧行为）。
    任务们 = []   # (序号, 页标识, 页序号, 条, 导入头)——顺序=原三层遍历序
    引用们 = []   # 1060：用例引用清单（轻校验·不编译）
    页序号表 = {}

    def 收集(页标识, 条, 导入头):
        nonlocal 跳过
        # 1017：平台限定条目（pthread_* / cn_install_thread_stack_guard 等 linux 专有
        #   符号示例）——非匹配平台跳过，不计入总数（仅显式声明"平台"的条目受影响，
        #   其余条目行为零变化；渲染面不经此路径）。
        if isinstance(条, dict) and 条.get("平台") and 平台 not in 条["平台"]:
            跳过 += 1
            return
        # 1060：用例引用条目——不编译，进轻校验清单（映射有效性）
        if isinstance(条, dict) and 条.get("用例"):
            引用们.append(条["用例"])
            return
        示 = 条.get("示例") or 条.get("原始") or 条.get("代码")
        if not 示:
            return
        if 页标识 not in 页序号表:
            页序号表[页标识] = len(页序号表) + 1
        任务们.append((len(任务们) + 1, 页标识, 页序号表[页标识], 条, 导入头))

    def 跑一个(任务):
        """单例执行（并行 worker）：编译/运行文件落**页级共享目录**（同页示例存在
        "写→读"序列依赖：file.html 写入文件(日志.txt)→读取文件族 依赖前序用例产物；
        页内串行执行序=原串行语义），页间目录隔离（防跨页杂散文件互写）。
        返回 "通过" 或 ("失败", 失败元组)。"""
        序号, 页标识, 页序, 条, 导入头 = 任务
        工作目录 = os.path.join(tmp, "p%d" % 页序)
        os.makedirs(工作目录, exist_ok=True)
        src = os.path.join(工作目录, "t%d.cn" % 序号)
        exe = os.path.join(工作目录, "t%d%s" % (序号, 产物后缀))
        示 = 条.get("示例") or 条.get("原始") or 条.get("代码")
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
        r = subprocess.run([cn, "build", src, "--output", exe, "--target", 平台],
                           capture_output=True, text=True, encoding="utf-8",
                           cwd=REPO, errors="replace")  # 显式 UTF-8：win 管道默认 GBK 解码=中文诊断/输出乱码假红
        if 条.get("预期失败"):
            # 负例块：必须编译失败（且诊断含 预期诊断 关键词——给了才比对）
            if r.returncode == 0:
                return ("失败", (页标识, 条.get("id", "?"), "负例未拦截",
                                 "预期编译失败却成功了"))
            if (条.get("预期诊断") and 条["预期诊断"] not in (r.stdout + r.stderr)):
                return ("失败", (页标识, 条.get("id", "?"), "负例诊断不符",
                                 "缺关键词 %r" % 条["预期诊断"]))
            return "通过"
        if r.returncode != 0:
            return ("失败", (页标识, 条.get("id", "?"), "编译失败",
                             (r.stdout + r.stderr)[-400:]))
        if 条.get("编译足矣"):
            return "通过"
        run = subprocess.run([exe], capture_output=True, text=True, encoding="utf-8",
                             errors="replace", timeout=30, stdin=subprocess.DEVNULL,
                             cwd=工作目录)  # 992：示例程序的相对路径落临时目录（文件 I/O
                                            # 示例曾把 笔记.txt/日志.txt 写进仓库根）；
                                            # 1015：再隔 t<N>/ 支持并行（跨例零互写）
        out = run.stdout.replace("\r\n", "\n")
        期望 = 条.get("输出") if "输出" in 条 else 条.get("综合输出")
        if 期望 is not None and out != 期望:
            return ("失败", (页标识, 条.get("id", "?"), "输出不符",
                             "期望=%r 实际=%r" % (期望, out)))
        if run.returncode != 0:
            return ("失败", (页标识, 条.get("id", "?"), "运行退出码 %d" % run.returncode,
                             out[-200:]))
        return "通过"

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
                收集(m["文件"], 条, m.get("导入头"))
    for 页 in GEN_PAGES:
        for ch in 页["章节"]:
            if ch.get("综合"):
                收集(页["文件"], dict(id=ch["id"] + "·综合", 示例=ch["综合"],
                                   输出=ch.get("综合输出"),
                                   原始=ch.get("综合原始"),
                                   编译足矣=ch.get("综合编译足矣")),
                     页.get("导入头"))
            for 块 in ch.get("块", []):
                if isinstance(块, dict) and 块.get("用例"):
                    引用们.append(块["用例"])   # 1060：用例引用（dict 块形态）
                elif isinstance(块, dict) and 块.get("代码"):
                    收集(页["文件"], dict(块, id=块.get("名", "码")), 页.get("导入头"))
                elif isinstance(块, tuple) and 块[0] == "用例":
                    引用们.append(块[1])       # 1060：用例引用（块形态）
                elif isinstance(块, tuple) and 块[0] == "码" and 块[2]:
                    收集(页["文件"], dict(id=块[1], 示例=块[2],
                                       输出=块[3] if len(块) > 3 else None,
                                       编译足矣=块[4] if len(块) > 4 else False),
                         页.get("导入头"))
            for 条 in ch.get("条目", []):
                收集(页["文件"], 条, 页.get("导入头"))

    任务数 = len(任务们)
    try:
        并发 = int(os.environ.get("CN_DOC_VERIFY_JOBS") or 8)
    except ValueError:
        并发 = 8
    if 任务数 > 1 and 并发 > 1:
        # 1015：按页分组——页内**串行保序**（同页示例存在"写→读"序列依赖：file.html
        #   写入文件(日志.txt)→读取文件/读取文件行/文件大小/关闭文件 依赖前序用例
        #   产物落共享 cwd，并行打散序即"输出不符"·实测 4 例实录），页间并行；
        #   各例仍各自 t<N>/ 独立目录（防跨页杂散文件互写）——等价于原串行语义的
        #   页级并行化。首例（第一页首例）先行=顺带承担 cn build 首编预热。
        页组 = {}
        页序 = []
        for t in 任务们:
            if t[1] not in 页组:
                页组[t[1]] = []
                页序.append(t[1])
            页组[t[1]].append(t)

        def 跑一页(页名):
            return [跑一个(t) for t in 页组[页名]]

        from concurrent.futures import ThreadPoolExecutor
        结果 = []
        余页 = 页序
        if 页序:
            结果 += 跑一页(页序[0])      # 首卷先行·串行承担 cn build 首编预热
            余页 = 页序[1:]
        with ThreadPoolExecutor(max_workers=并发) as 池:
            页结果 = list(池.map(跑一页, 余页))   # map 保序=原页序
        结果 += [r for 组 in 页结果 for r in 组]
    else:
        结果 = [跑一个(t) for t in 任务们]
    通过 = sum(1 for r in 结果 if r == "通过")
    fails = [r[1] for r in 结果 if isinstance(r, tuple) and r[0] == "失败"]
    print("示例验证：%d/%d 通过（编译+运行+输出比对%s%s）" % (
        通过, 任务数, "·平台跳过 %d" % 跳过 if 跳过 else "",
        "·%d 路并发" % 并发 if 并发 > 1 and 任务数 > 1 else ""))
    for f in fails:
        print("  [失败] %s :: %s — %s\n    %s" % f)
    # 1060：用例引用轻校验（不编译——develop 门禁已保证 E2E 绿灯，用户令「进了
    #   develop 的必然绿灯，无需重复验证」；此处只锚「官网引用的映射有效性」）。
    引用去重 = sorted(set(引用们))
    引用失败 = []
    for 名 in 引用去重:
        错 = 用例校验(名)
        if 错:
            引用失败.append((名, 错))
    if 引用去重:
        print("用例引用校验：%d 例（去重后·失败 %d）" % (len(引用去重), len(引用失败)))
    for 名, 错 in 引用失败:
        print("  [引用失败] %s: %s" % (名, 错))
    if not 仅失败 and not fails:
        import shutil
        shutil.rmtree(tmp, ignore_errors=True)
    return 1 if (fails or 引用失败) else 0


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
