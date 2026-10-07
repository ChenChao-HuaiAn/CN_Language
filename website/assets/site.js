/* ============================================================
   CN 语言官网 · 站点行为（主题切换/移动导航/目录跟随）
   ============================================================ */
(function () {
  "use strict";

  /* ── 主题 ── */
  function applyTheme(t) {
    if (t === "dark" || t === "light") {
      document.documentElement.setAttribute("data-theme", t);
    } else {
      document.documentElement.removeAttribute("data-theme");
    }
  }

  function currentTheme() {
    return document.documentElement.getAttribute("data-theme") ||
      (window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
  }

  function initTheme() {
    var saved = null;
    try { saved = localStorage.getItem("cn-theme"); } catch (e) { /* 隐私模式忽略 */ }
    applyTheme(saved);
    document.querySelectorAll("[data-主题钮]").forEach(function (btn) {
      btn.addEventListener("click", function () {
        var next = currentTheme() === "dark" ? "light" : "dark";
        applyTheme(next);
        try { localStorage.setItem("cn-theme", next); } catch (e) { /* 忽略 */ }
        syncLabels();
      });
    });
    function syncLabels() {
      var dark = currentTheme() === "dark";
      document.querySelectorAll("[data-主题钮]").forEach(function (btn) {
        btn.textContent = dark ? "日" : "月";
        btn.setAttribute("aria-label", dark ? "切换到浅色主题" : "切换到深色主题");
      });
    }
    syncLabels();
  }

  /* ── 移动导航 ── */
  function initNav() {
    var toggle = document.querySelector("[data-导航钮]");
    var nav = document.querySelector(".导航");
    if (!toggle || !nav) return;
    toggle.addEventListener("click", function () {
      var open = nav.classList.toggle("开");
      toggle.setAttribute("aria-expanded", open ? "true" : "false");
      toggle.textContent = open ? "关闭" : "菜单";
    });
  }

  /* ── 目录跟随高亮 ── */
  function initToc() {
    var toc = document.querySelector(".目录");
    if (!toc) return;
    var links = Array.prototype.slice.call(toc.querySelectorAll("a[href^='#']"));
    if (!links.length || !("IntersectionObserver" in window)) return;
    var map = {};
    links.forEach(function (a) {
      var id = decodeURIComponent(a.getAttribute("href").slice(1));
      var h = document.getElementById(id);
      if (h) map[id] = a;
    });
    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        var a = map[entry.target.id];
        if (!a) return;
        if (entry.isIntersecting) {
          links.forEach(function (x) { x.classList.remove("当前"); });
          a.classList.add("当前");
        }
      });
    }, { rootMargin: "-20% 0px -70% 0px" });
    Object.keys(map).forEach(function (id) {
      observer.observe(document.getElementById(id));
    });
  }

  /* ── 入场编排：JS 触发（渐进增强·动画不跑内容也可见）── */
  function initEntrance() {
    if (!document.querySelector(".入场")) return;
    requestAnimationFrame(function () {
      requestAnimationFrame(function () {
        document.querySelectorAll(".入场").forEach(function (el) {
          el.classList.add("播");
        });
      });
    });
  }

  initTheme();
  initNav();
  initToc();
  initEntrance();
})();
