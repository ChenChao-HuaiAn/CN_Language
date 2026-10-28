#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""「今日头条」栏目生成器——把每日任务产出的头条文章网页搬上官网。

输入：头条文章草稿目录下的 第*.html（自带样式的独立页面，图片 base64 内嵌；
开头有一块「使用说明」蓝色虚线框 <div class="说明">，属发布操作指引，不上官网）。

输出（website/toutiao/，生成产物不入 git——见仓库 .gitignore，经
scripts/upload_toutiao.sh 单独上传 TX_01，website_auto_deploy.py 发布时
会原样保留现网 toutiao/ 目录）：
  toutiao/index.html              栏目列表页（官网风格·按日期分组·新文在前）
  toutiao/articles/第N篇-*.html   文章页（剔除蓝框·补 viewport 与返回链接）

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

# 官网导航（与 index.html / gen_docs.py 顶栏同构；当前=今日头条）
NAV_LINKS = [
    ("../", "首页", False),
    ("../docs/", "文档", False),
    ("../docs/getting-started.html", "快速开始", False),
    ("../docs/reference/", "语言参考", False),
    ("./", "今日头条", True),
    ("../os/", "CN-OS", False),
    ("https://gitcode.com/ChenChao_GitCode/CN_Language", "源码 ↗", False),
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


def 导航html():
    项 = []
    for href, 名, 当前 in NAV_LINKS:
        attr = ' aria-current="page"' if 当前 else ""
        项.append('      <a href="%s"%s>%s</a>' % (href, attr, 名))
    return "\n".join(项)


def 解析篇目(草稿: Path, 最小篇号: int):
    """扫描 第N篇-标题.html → [{号, 题, 源, 日期, 副题}]；篇号倒序。"""
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
        篇目.append({
            "号": 号,
            "题": m.group(2),
            "源": f,
            "日期": datetime.fromtimestamp(f.stat().st_mtime).strftime("%Y-%m-%d"),
            "副题": 副,
            "原文": 原文,
        })
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


def 列表页(篇目, 生成时刻) -> str:
    """官网风格栏目列表页：按日期分组（新→旧），组内篇号倒序。"""
    分组 = {}
    for p in 篇目:
        分组.setdefault(p["日期"], []).append(p)
    日期序 = sorted(分组.keys(), reverse=True)

    组html = []
    for d in 日期序:
        组 = 分组[d]
        条 = []
        for p in 组:
            副题html = ('\n            <span class="条-副">%s</span>' % p["副题"]) if p["副题"] else ""
            条.append(
                '        <a class="条" href="articles/%s">\n'
                '          <span class="条-号">第 %d 篇</span>\n'
                '          <span class="条-主">\n'
                '            <span class="条-题">%s</span>%s\n'
                "          </span>\n"
                "        </a>" % (p["源"].name, p["号"], p["题"], 副题html))
        组html.append(
            '    <section class="日组">\n'
            "      <h2 class=\"日组-头\"><span class=\"日组-日期\">%s</span>"
            "<span class=\"日组-数\">%d 篇</span></h2>\n"
            "      <div class=\"日组-列\">\n%s\n      </div>\n"
            "    </section>" % (d, len(组), "\n".join(条)))

    return """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>今日头条 · CN 语言</title>
<meta name="description" content="CN 语言项目每日开发实录专栏：编译器演进、语言设计、工程实践——同一批文章在头条号「CN-Language中文编程语言」同步发布。">
<script>
  (function () {
    try {
      var t = localStorage.getItem("cn-theme");
      if (t === "dark" || t === "light") document.documentElement.setAttribute("data-theme", t);
    } catch (e) {}
  })();
</script>
<link rel="icon" href="../assets/brand/favicon.svg" type="image/svg+xml">
<link rel="icon" href="../assets/brand/favicon-32.png" sizes="32x32" type="image/png">
<link rel="stylesheet" href="../assets/tokens.css?v=1">
<link rel="stylesheet" href="../assets/site.css?v=6">
<style>
  .栏目-头 { max-width: 860px; margin: 0 auto; padding-block: var(--space-8) var(--space-6); }
  .栏目-述 { color: var(--color-fg-muted); line-height: 1.9; margin-top: var(--space-4); max-width: 62ch; }
  .栏目-体 { max-width: 860px; margin: 0 auto; padding-block-end: var(--space-9); }
  .日组 { margin-block-end: var(--space-7); }
  .日组-头 { display: flex; align-items: baseline; gap: var(--space-3);
             margin: 0 0 var(--space-4); padding-block-end: var(--space-2);
             border-bottom: 1px solid var(--color-line, rgba(128,128,128,.25)); }
  .日组-日期 { font-size: 1.05rem; font-weight: 700; }
  .日组-数 { font-size: .85rem; color: var(--color-fg-muted); }
  .日组-列 { display: grid; gap: var(--space-3); }
  .条 { display: flex; align-items: baseline; gap: var(--space-4);
        padding: var(--space-4) var(--space-5);
        border: 1px solid var(--color-line, rgba(128,128,128,.2));
        border-radius: 10px; text-decoration: none; color: inherit;
        transition: border-color .15s, transform .15s; }
  .条:hover { border-color: var(--color-accent, #AD2F22); transform: translateX(2px); }
  .条-号 { flex: none; font-size: .82rem; color: var(--color-fg-muted);
           white-space: nowrap; font-variant-numeric: tabular-nums; }
  .条-主 { display: flex; flex-direction: column; gap: 2px; min-width: 0; }
  .条-题 { font-weight: 600; font-size: 1.02rem; }
  .条-副 { font-size: .88rem; color: var(--color-fg-muted); line-height: 1.6; }
  @media (max-width: 640px) {
    .条 { flex-direction: column; gap: var(--space-1); padding: var(--space-3) var(--space-4); }
  }
</style>
</head>
<body>

<header class="顶栏">
  <div class="容器 顶栏-内">
    <a class="标识" href="../">%(logo)sCN<span class="标识-点">·</span>语言</a>
    <button class="导航-开关" data-导航钮 aria-expanded="false">菜单</button>
    <nav class="导航" aria-label="站点导航">
%(nav)s
      <button class="主题钮" data-主题钮 aria-label="切换主题">月</button>
    </nav>
  </div>
</header>

<main>
  <div class="栏目-头">
    <p class="眉题">开发实录 · 每日连载</p>
    <h1>今日头条</h1>
    <p class="栏目-述">CN 语言项目的每日开发实录：编译器演进、语言设计裁定、
    工程实践与历史教训——篇篇带真实源码与可复现实验，同一批文章在头条号
    「CN-Language中文编程语言」同步发布。新篇在上，共 %(总数)d 篇。</p>
  </div>
  <div class="栏目-体">
%(组)s
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
          <li><a href="../docs/">官方文档</a></li>
          <li><a href="../docs/getting-started.html">快速开始</a></li>
          <li><a href="../docs/reference/">语言参考</a></li>
        </ul>
      </div>
      <div>
        <h4>栏目</h4>
        <ul>
          <li><a href="./">今日头条</a></li>
          <li><a href="../os/">CN-OS 蓝图</a></li>
          <li><a href="https://gitcode.com/ChenChao_GitCode/CN_Language" target="_blank" rel="noopener">源码仓库 ↗</a></li>
        </ul>
      </div>
    </div>
    <div class="页脚-底">
      <span>© 2026 CN 语言项目</span>
      <span>栏目更新于 %(时刻)s</span>
    </div>
  </div>
</footer>

<script src="../assets/site.js?v=3"></script>
</body>
</html>
""" % {"logo": LOGO_SVG, "nav": 导航html(), "总数": len(篇目),
       "组": "\n".join(组html), "时刻": 生成时刻}


def 主() -> int:
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

    时刻 = time.strftime("%Y-%m-%d %H:%M")
    (输出 / "index.html").write_text(列表页(篇目, 时刻), encoding="utf-8")

    print("✓ 今日头条栏目已生成：%d 篇（%s ~ %s）→ %s"
          % (len(篇目), 篇目[-1]["源"].stem, 篇目[0]["源"].stem, 输出))
    return 0


if __name__ == "__main__":
    sys.exit(主())
