/* ============================================================
   CN 语言语法高亮器（零依赖·手写分词器）
   关键字表逐字对齐 plans/001 §二（46 保留字 + 上下文关键字）。
   用法：<pre><code class="language-cn">…</code></pre>
   ============================================================ */
(function () {
  "use strict";

  /* 46 保留字（001 §2.1）：控制流10+声明3+安全1+常量3+OOP5+泛型1 */
  var KW = new Set([
    "如果", "否则", "当", "循环", "返回", "中断", "继续", "选择", "情况", "默认",
    "函数", "变量", "导入",
    "不安全",
    "真", "假", "无",
    "类", "接口", "自身", "父类", "友元",
    "泛型"
  ]);

  /* 类型关键字（001 §2.1 类型 21 个 + 错误处理 2 个，作类型位使用） */
  var TYP = new Set([
    "整数", "小数",
    "整8", "整16", "整32", "整64", "整128",
    "正8", "正16", "正32", "正64", "正128",
    "浮32", "浮64",
    "布尔", "字符", "字符串", "空类型", "结构体", "联合体", "枚举",
    "结果", "可选"
  ]);

  /* 上下文关键字（001 §2.1a·非保留字，固定语法位） */
  var CTX = new Set([
    "公开", "私有", "保护", "静态", "常量",
    "虚拟", "重写", "抽象",
    "作为", "模块", "运算符", "遍历", "每个", "属于", "对",
    "正常", "错误"
  ]);

  function isIdChar(ch) {
    if (!ch) return false;
    var c = ch.codePointAt(0);
    return (c >= 0x4e00 && c <= 0x9fff) ||  /* CJK 统一表意 */
           (c >= 0x3400 && c <= 0x4dbf) ||  /* 扩展 A */
           (c >= 0x61 && c <= 0x7a) ||      /* a-z */
           (c >= 0x41 && c <= 0x5a) ||      /* A-Z */
           (c >= 0x30 && c <= 0x39) ||      /* 0-9 */
           c === 0x5f;                      /* _ */
  }

  function escapeHtml(s) {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  function span(cls, text) {
    return '<span class="语言-' + cls + '">' + escapeHtml(text) + "</span>";
  }

  /* 判断 token 之后（跳过空白）是否紧跟 '(' —— 函数调用判定 */
  function nextNonSpaceIsParen(src, i) {
    var j = i;
    while (j < src.length && (src[j] === " " || src[j] === "\t")) j++;
    return src[j] === "(";
  }

  function highlightCN(src) {
    var out = "";
    var i = 0;
    var n = src.length;

    while (i < n) {
      var ch = src[i];

      /* 注释：// 至行尾 */
      if (ch === "/" && src[i + 1] === "/") {
        var eol = src.indexOf("\n", i);
        if (eol === -1) eol = n;
        out += span("注释", src.slice(i, eol));
        i = eol;
        continue;
      }
      /* 块注释：/* ... *\/ */
      if (ch === "/" && src[i + 1] === "*") {
        var end = src.indexOf("*/", i + 2);
        end = end === -1 ? n : end + 2;
        out += span("注释", src.slice(i, end));
        i = end;
        continue;
      }

      /* 字符串：原始/多行 前缀紧贴引号 → 前缀随串着色；普通串 */
      var strStart = -1;
      if ((ch === "原" && src.startsWith("原始", i)) ||
          (ch === "多" && src.startsWith("多行", i))) {
        var q = i + 2;
        while (q < n && (src[q] === " " || src[q] === "\t")) q++;
        if (src[q] === '"') strStart = i;
      } else if (ch === '"') {
        strStart = i;
      }
      if (strStart !== -1) {
        var j = strStart;
        if (src[strStart] !== '"') j = strStart + 2; /* 跳过前缀 */
        while (j < n && (src[j] === " " || src[j] === "\t")) j++;
        var k = j + 1;
        while (k < n && src[k] !== '"') {
          if (src[k] === "\\") k++; /* 跳过转义 */
          k++;
        }
        k = Math.min(k + 1, n);
        out += span("字符串", src.slice(strStart, k));
        i = k;
        continue;
      }

      /* 预处理指令：# + 标识（行首空白后的 #） */
      if (ch === "#") {
        var m = i + 1;
        while (m < n && isIdChar(src[m])) m++;
        out += span("指令", src.slice(i, m));
        i = m;
        continue;
      }

      /* 数字字面量（含 0x 十六进制与小数点） */
      if (ch >= "0" && ch <= "9") {
        var d = i;
        if (ch === "0" && (src[i + 1] === "x" || src[i + 1] === "X")) {
          d = i + 2;
          while (d < n && /[0-9a-fA-F_]/.test(src[d])) d++;
        } else {
          while (d < n && /[0-9_]/.test(src[d])) d++;
          if (src[d] === "." && src[d + 1] >= "0" && src[d + 1] <= "9") {
            d++;
            while (d < n && /[0-9_]/.test(src[d])) d++;
          }
        }
        out += span("数字", src.slice(i, d));
        i = d;
        continue;
      }

      /* 标识符（中英数下划线连续段·整段查表=天然最长匹配） */
      if (isIdChar(ch)) {
        var w = i;
        while (w < n && isIdChar(src[w])) w++;
        var word = src.slice(i, w);
        if (KW.has(word) || CTX.has(word)) {
          out += span("关键字", word);
        } else if (TYP.has(word)) {
          out += span("类型", word);
        } else if (nextNonSpaceIsParen(src, w)) {
          out += span("函数", word);
        } else {
          out += escapeHtml(word);
        }
        i = w;
        continue;
      }

      /* 其余：字符串以外的一律按标点/普通字符 */
      out += span("标点", ch);
      i++;
    }
    return out;
  }

  /* 简单 shell 高亮：注释淡墨，其余不动 */
  function highlightShell(src) {
    return src.split("\n").map(function (line) {
      var t = line.trimStart();
      if (t.startsWith("#")) return span("注释", line);
      return escapeHtml(line);
    }).join("\n");
  }

  function run() {
    document.querySelectorAll("pre code.language-cn, pre code[data-lang='cn']")
      .forEach(function (el) { el.innerHTML = highlightCN(el.textContent); });
    document.querySelectorAll("pre code.language-sh, pre code.language-powershell, pre code[data-lang='sh']")
      .forEach(function (el) { el.innerHTML = highlightShell(el.textContent); });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", run);
  } else {
    run();
  }
})();
