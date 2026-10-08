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

MODULES = (
    docs_data_core.PAGES
    + docs_data_mapset.PAGES
    + docs_data_text.PAGES
    + docs_data_io.PAGES
    + docs_data_builtin.PAGES
)

# 模块页顺序链（页脚导航 + 索引页排序）：内置 → 各模块 → CLI（站内已有页）
CHAIN = ["stdlib.html", "builtin.html"] + [
    m["文件"] for m in MODULES if m["文件"] != "builtin.html"] + ["cli.html"]

CSS_VER = {"site": 4, "tokens": 2, "hl": 1, "js": 2}   # 改资源必 bump ?v=N


def esc(s):
    return html.escape(s, quote=False)


# ---------------------------------------------------------------- 页面骨架
def 骨架(题名, 描述, 正文html, 目录项, 当前路径, 页脚导航, 侧栏当前):
    """文档页骨架：顶栏+侧栏+正文+目录+页脚（与手写页同构）。"""
    rel = os.path.relpath(DOCS, os.path.dirname(os.path.join(DOCS, 当前路径)))
    rel = rel.replace("\\", "/")
    if not rel.endswith("/"):
        rel += "/"
    if rel == "./":
        rel = ""
    a = lambda href: href if href.startswith("http") else (rel + href)  # noqa: E731
    侧栏 = []
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">入门</p>'
                '<a href="%s">文档总览</a><a href="%s">快速开始</a></div>'
                % (a(".././" if rel else "./"), a("getting-started.html")))
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">教程</p>'
                '<a href="%s">语法基础</a><a href="%s">类与泛型</a>'
                '<a href="%s">安全与并发</a></div>'
                % (a("tutorial-syntax.html"), a("tutorial-oop.html"),
                   a("tutorial-safety.html")))
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">语言参考</p>'
                '<a href="%s">参考总览</a><a href="%s">词法结构</a>'
                '<a href="%s">类型系统</a><a href="%s">控制流</a>'
                '<a href="%s">函数</a><a href="%s">类与对象</a>'
                '<a href="%s">错误处理</a><a href="%s">并发</a>'
                '<a href="%s">模块系统</a></div>'
                % (a("reference/"), a("reference/lexical.html"),
                   a("reference/types.html"), a("reference/control-flow.html"),
                   a("reference/functions.html"), a("reference/oop.html"),
                   a("reference/errors.html"), a("reference/concurrency.html"),
                   a("reference/modules.html")))
    库链 = ['<a href="%s"%s>标准库总览</a>' % (a("stdlib.html"),
                                   " aria-current=\"page\"" if 当前路径 == "stdlib.html" else "")]
    for m in MODULES:
        cur = ' aria-current="page"' if m["文件"] == 当前路径 else ""
        库链.append('<a href="%s"%s>%s</a>' % (a("stdlib/" + m["文件"]), cur, m["题名"]))
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">标准库</p>%s</div>'
                % "".join(库链))
    侧栏.append('<div class="侧栏-组"><p class="侧栏-组题">资源</p>'
                '<a href="%s">命令行参考</a><a href="%s" target="_blank" rel="noopener">源码仓库 ↗</a></div>'
                % (a("cli.html"), "https://gitcode.com/ChenChao_GitCode/CN_Language"))

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
    print("生成：stdlib.html + stdlib/ × %d（%d 个条目）" % (len(MODULES), 统计()))


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
    """优先最新主树构建（本文档描述当前语言行为），回退本树。"""
    主树 = os.path.normpath(os.path.join(REPO, "..", "CN_Language_C",
                                          "target", "cn.exe"))
    for c in (os.environ.get("CN_EXE"), 主树, os.path.join(REPO, "target", "cn.exe")):
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
                示 = 条.get("示例") or 条.get("原始")
                if not 示:
                    continue
                总数 += 1
                src = os.path.join(tmp, "t%d.cn" % 总数)
                exe = os.path.join(tmp, "t%d.exe" % 总数)
                with io.open(src, "w", encoding="utf-8", newline="\n") as f:
                    f.write(条目程序(m, 条))
                r = subprocess.run([cn, "build", src, "--output", exe],
                                   capture_output=True, text=True,
                                   cwd=REPO, errors="replace")
                if r.returncode != 0:
                    fails.append((m["文件"], 条["id"], "编译失败", (r.stdout + r.stderr)[-400:]))
                    continue
                if 条.get("编译足矣"):
                    通过 += 1
                    continue
                run = subprocess.run([exe], capture_output=True, text=True,
                                     errors="replace", timeout=30)
                out = run.stdout.replace("\nr\nn", "\n")
                期望 = 条.get("输出")
                if 期望 is not None and out != 期望:
                    fails.append((m["文件"], 条["id"], "输出不符",
                                  "期望=%r 实际=%r" % (期望, out)))
                    continue
                if run.returncode != 0:
                    fails.append((m["文件"], 条["id"], "运行退出码 %d" % run.returncode, out[-200:]))
                    continue
                通过 += 1
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
