#!/usr/bin/env python3
# 网页登录墙公共模块（1425·2026-10-23 用户令「任务看板/云端测试网页不得外人可见」）：
#   人通道 = 用户名+密码（网页凭据 CN_WEB_USER/CN_WEB_PASS·systemd 注入·不入 git）
#           → 签发 HttpOnly 会话 cookie（SameSite=Lax·SQLite 持久重启不掉·7 天重登，
#             延续 430 会话体系）；错凭据 401+小睡防在线爆破·compare_digest 防时序侧信道。
#   机器通道 = Bearer 令牌（CN_BOARD_TOKEN/CN_QUEUE_TOKEN·CLI/脚本/CI daemon）不变。
#   消费方 = board_service(8301) 与 queue_service(8300)——登录墙口径两服务一致：
#     页面（非 /api/*）未鉴权 → 返回本模块登录页（200·自包含单文件零外部资源——
#       登录页自身不能被墙挡住样式/脚本）；API（/api/*）未鉴权 → 401 JSON。
#   fail-closed（430 fail-open 回环放行根治同源教训）：凭据未配置 = 登录全拒
#     （与错凭据同待·不给「未配置/口令错」区分面）——页面/数据一并不对外。
# 会话表（两服务各自 SQLite·同构）：会话(会话id TEXT PRIMARY KEY, 时戳 REAL)
#   ——board_service 430 既有同构表直接复用；queue_service 启动建表。
# 自检：python3 web_auth.py --selftest（凭据校验两态/会话签发·校验·过期/登录页标记）
import os
import secrets
import time

会话秒 = 7 * 24 * 3600                       # 登录会话有效期（430·7 天后须重登）
# 会话 cookie 名由各服务传入：Caddy 反代下 /board/（8301）与 /queue/（8300）同域，
# 同名 cookie Path=/ 会互相顶掉——两服务必须不同名（cn_board_session/cn_queue_session）。
看板cookie名 = "cn_board_session"            # 430 既有名（保持·旧登录态不掉）
队列cookie名 = "cn_queue_session"            # 8300 门禁窗口（1425 新增）


def _凭据() -> tuple:
    """现读环境变量（非 import 期缓存）：selftest 进程内设 env 即生效，
    免「先 import 后设环境变量」时序陷阱。"""
    return (os.environ.get("CN_WEB_USER", ""), os.environ.get("CN_WEB_PASS", ""))


def 校验凭据(输入用户名: str, 输入密码: str) -> bool:
    """网页凭据校验（1425）。未配置=拒（fail-closed）；比较用 compare_digest
    比字节（str 版遇非 ASCII 会 TypeError 断连——430 实录教训）；错凭据小睡
    0.6s 防在线爆破（430 同款）。"""
    实用户名, 实密码 = _凭据()
    if not 实用户名 or not 实密码:
        time.sleep(0.6)
        return False
    好 = secrets.compare_digest(输入用户名.encode("utf-8"), 实用户名.encode("utf-8")) \
        and secrets.compare_digest(输入密码.encode("utf-8"), 实密码.encode("utf-8"))
    if not 好:
        time.sleep(0.6)
    return 好


def 建会话表(con) -> None:
    """会话表（两服务同构·幂等）。"""
    con.execute("""CREATE TABLE IF NOT EXISTS 会话(
        会话id TEXT PRIMARY KEY,
        时戳 REAL)""")


def 签会话(con, 写锁) -> str:
    """签发会话：随机 sid 入库+过期惰性清（430 同款·写锁内事务）。返回 sid。"""
    sid = secrets.token_urlsafe(32)
    now = time.time()
    with 写锁:
        con.execute("DELETE FROM 会话 WHERE 时戳<?", (now - 会话秒,))
        con.execute("INSERT INTO 会话(会话id,时戳) VALUES(?,?)", (sid, now))
        con.commit()
    return sid


def 会话合法(con, 写锁, sid: str) -> bool:
    """sid 是否存活未过期（空 sid 短路——430 同款）。"""
    if not sid:
        return False
    with 写锁:
        行 = con.execute("SELECT 时戳 FROM 会话 WHERE 会话id=?", (sid,)).fetchone()
    return bool(行) and 行[0] >= time.time() - 会话秒


def 销会话(con, 写锁, sid: str) -> None:
    """注销会话（登出·430 同款）。"""
    if not sid:
        return
    with 写锁:
        con.execute("DELETE FROM 会话 WHERE 会话id=?", (sid,))
        con.commit()


def set_cookie头(sid: str, 名: str = 看板cookie名) -> tuple:
    """签发 Set-Cookie 头（HttpOnly·SameSite=Lax·Path=/——Caddy /board/、/queue/
    反代 strip 前缀后源侧路径恒以 / 计，Path=/ 两态通用·430 实证）。"""
    return ("Set-Cookie", f"{名}={sid}; Max-Age={会话秒}; Path=/; HttpOnly; SameSite=Lax")


def 清_cookie头(名: str = 看板cookie名) -> tuple:
    """登出清 cookie 头（Max-Age=0 立删）。"""
    return ("Set-Cookie", f"{名}=; Max-Age=0; Path=/; HttpOnly; SameSite=Lax")


def 登录页HTML(服务名: str) -> str:
    """自包含登录页（200·内联样式零外部资源——登录页自身不能被墙挡）。登录成功
    种 cookie 后 reload 取业务页；fetch 用相对路径——直连 :端口 与 Caddy /board/
    /queue/ 反代两态同源（同 queue_www 资源相对引用先例）。"""
    return """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>登录 · """ + 服务名 + """</title>
<style>
  :root { --fg:#221F18; --bg:#FCFAF6; --accent:#AD2F22; --line:#E4DFD5; --dim:#8A8577; }
  * { box-sizing:border-box; margin:0; padding:0; }
  body { min-height:100vh; display:flex; align-items:center; justify-content:center;
         font-family:"Microsoft YaHei","PingFang SC",system-ui,sans-serif;
         background:linear-gradient(160deg,#F5F1E8 0%,#FCFAF6 60%,#F0EADF 100%); color:var(--fg); }
  .卡 { width:min(360px,92vw); background:var(--bg); border:1px solid var(--line);
        border-radius:14px; padding:34px 30px 26px; box-shadow:0 8px 30px rgba(34,31,24,.10); }
  .题 { font-size:20px; font-weight:700; }
  .副 { font-size:12.5px; color:var(--dim); margin:6px 0 22px; }
  label { display:block; font-size:13px; margin:14px 0 6px; }
  input { width:100%; padding:10px 12px; font-size:14px; border:1px solid var(--line);
          border-radius:8px; background:#fff; outline:none; }
  input:focus { border-color:var(--accent); }
  .错 { min-height:18px; font-size:12.5px; color:var(--accent); margin-top:10px; }
  button { width:100%; margin-top:10px; padding:11px; font-size:15px; font-weight:600;
           color:#FCFAF6; background:var(--accent); border:0; border-radius:8px; cursor:pointer; }
  button:disabled { opacity:.55; cursor:wait; }
</style>
</head>
<body>
<div class="卡">
  <div class="题">""" + 服务名 + """ · 内部系统</div>
  <div class="副">仅限内部开发人员——请输入用户名与密码登录</div>
  <form id="表单">
    <label for="用户名">用户名</label>
    <input id="用户名" autocomplete="username" autofocus required>
    <label for="密码">密码</label>
    <input id="密码" type="password" autocomplete="current-password" required>
    <div id="错误" class="错"></div>
    <button id="提交" type="submit">登 录</button>
  </form>
</div>
<script>
(function () {
  var 表单 = document.getElementById("表单");
  表单.addEventListener("submit", function (e) {
    e.preventDefault();
    var 钮 = document.getElementById("提交");
    var 错 = document.getElementById("错误");
    错.textContent = "";
    钮.disabled = true;
    fetch("api/login", { method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ 用户名: document.getElementById("用户名").value.trim(),
                             密码: document.getElementById("密码").value }) })
      .then(function (r) {
        if (r.ok) { location.reload(); return; }
        return r.json().catch(function () { return {}; }).then(function (体) {
          错.textContent = 体.错误 || (体.说明 || ("登录失败（HTTP " + r.status + "）"));
          钮.disabled = false;
        });
      })
      .catch(function () { 错.textContent = "网络异常，请重试"; 钮.disabled = false; });
  });
})();
</script>
</body>
</html>
"""


if __name__ == "__main__":
    # 最小正反自测（#1011：改检查工具须带 selftest 证据）
    import sqlite3
    import threading

    os.environ["CN_WEB_USER"] = "甲"
    os.environ["CN_WEB_PASS"] = "密·码1"
    锁 = threading.Lock()
    con = sqlite3.connect(":memory:", check_same_thread=False)
    建会话表(con)

    assert not 校验凭据("甲", "错"), "错密码应拒"
    assert not 校验凭据("乙", "密·码1"), "错用户名应拒"
    assert 校验凭据("甲", "密·码1"), "对凭据应过"

    sid = 签会话(con, 锁)
    assert 会话合法(con, 锁, sid), "新签会话应合法"
    assert not 会话合法(con, 锁, ""), "空 sid 应拒"
    assert not 会话合法(con, 锁, "不存在的sid"), "陌生 sid 应拒"
    销会话(con, 锁, sid)
    assert not 会话合法(con, 锁, sid), "注销后应拒"

    # 过期面：直接种过期行
    con.execute("INSERT INTO 会话(会话id,时戳) VALUES('老',?)", (time.time() - 会话秒 - 1,))
    con.commit()
    assert not 会话合法(con, 锁, "老"), "过期会话应拒"

    os.environ.pop("CN_WEB_PASS")
    assert not 校验凭据("甲", "密·码1"), "凭据未配置应 fail-closed 拒"

    页 = 登录页HTML("测试服务")
    for 标记 in ("用户名", "密码", "api/login", "type=\"password\""):
        assert 标记 in 页, f"登录页缺标记 {标记}"
    assert "href=" not in 页 and "src=" not in 页, "登录页须零外部资源（自包含）"

    print("[OK] web_auth selftest 全绿（凭据两态·会话签发/校验/注销/过期·fail-closed·登录页自包含）")
