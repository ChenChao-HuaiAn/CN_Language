/* ============================================================
   CN 任务看板 · 行为层（382·看板 v2）
   数据=/api/board 每 8s 轮询；tab 隔离渲染（只渲染激活页·其余标脏）；
   详情滑出面板；网页端写操作（裁决/新建任务）须管理员登录（430·
   会话 cookie HttpOnly 由服务端签发·JS 不可读——旧 localStorage
   原始令牌方案退役）；读操作全公开。
   ============================================================ */
"use strict";

let 数据 = null;                       // 最近一次 /api/board 聚合
let 激活板 = "看板";                    // 388：顶级板块（看板/交接/教训/覆盖）
let 激活页 = "飞行";                    // 看板板块内子页
let 脏页 = { 飞行: true, 分支: true, 认领: true, 裁决: true };
let 当前筛 = "就绪";
let 当前视图 = "列表";
let 当前详情 = null;

const $ = (id) => document.getElementById(id);
const 转义 = (s) => String(s ?? "").replace(/[&<>"]/g,
  (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const 状态签 = { "⬜": "待办", "🏃": "在飞", "⏸": "挂起", "✅": "完成" };
const 状态符 = { 待办: "⬜", 在飞: "🏃", 挂起: "⏸", 完成: "✅" };

/* 标题一句话概要（391 用户令：列表/卡片标题一句话概括·全文点号牌看详情面板）：
   ≤36 字直显；超长则取第一个「：」「——」前的分句（含分隔符·提示尚有下文），
   分句过短（<8 字·概括失义）或不存在则截前 30 字加省略号。 */
const 概要上限 = 30;
const 概要 = (题) => {
  const s = String(题 || "").replace(/\s+/g, " ").trim();
  if (s.length <= 概要上限 + 6) return s;
  for (const 分 of ["：", "——"]) {
    const i = s.indexOf(分);
    if (i >= 8 && i + 1 <= 概要上限) return s.slice(0, i + 1) + "…";
  }
  return s.slice(0, 概要上限) + "…";
};

/* 意图备注与台账标题同源去重（391：wt.py create 把同一句描述既写台账标题又写
   认领备注，前端大字/小字双显重复——忽略空白差异后相同或互为前缀即判重复）。 */
const 备注重复标题 = (备, 题) => {
  if (!备 || !题) return false;
  const a = String(备).replace(/\s+/g, ""), b = String(题).replace(/\s+/g, "");
  return a === b || b.startsWith(a) || a.startsWith(b);
};

const 相对时 = (s) => {
  if (!s) return "";
  const 分 = Math.floor((Date.now() / 1000 - s) / 60);
  if (分 < 1) return "刚刚";
  if (分 < 60) return 分 + " 分钟前";
  const 时 = Math.floor(分 / 60);
  if (时 < 24) return 时 + " 小时前";
  return Math.floor(时 / 24) + " 天前";
};

/* —— 管理员登录（430·裁决与写操作仅作者）——
   会话 cookie HttpOnly 由服务端签发，浏览器自动携带，JS 不接触原始口令令牌；
   登录态经 /api/me 恢复（刷新不丢）。已登录再点登录钮=登出。 */
let 已登录 = false;

function 更新登录UI() {
  const 钮 = $("登录钮");
  钮.textContent = 已登录 ? "登出" : "🔑 登录";
  钮.title = 已登录 ? "退出管理员登录" : "裁决与写操作须管理员登录";
  $("裁决登录提示").textContent = 已登录 ? "" : "🔒 提交裁决须管理员登录（右上 🔑）";
}

async function 查登录态() {
  try {
    const r = await 调API("GET", "api/me");
    已登录 = !!r.已登录;
  } catch (e) { 已登录 = false; }
  更新登录UI();
}

function 开登录() {
  if (已登录) {                          // 已登录再点=登出
    调API("POST", "api/logout").catch(() => {}).finally(() => {
      已登录 = false;
      更新登录UI();
      提示("已登出——裁决与写操作已上锁");
    });
    return;
  }
  $("登录遮罩").hidden = false;
  $("登录错").textContent = "";
  $("登录口令").value = "";
  $("登录口令").focus();
}

function 关登录() { $("登录遮罩").hidden = true; }

async function 提交登录() {
  const 口令 = $("登录口令").value;
  if (!口令) { $("登录错").textContent = "口令必填"; return; }
  $("登录提交").disabled = true;
  try {
    await 调API("POST", "api/login", { 口令 });
    已登录 = true;
    关登录();
    更新登录UI();
    提示("已登录——裁决与写操作已解锁", 3000);
  } catch (e) {
    $("登录错").textContent = e.码 === 401 ? "口令不符" : "登录失败：" + e.message;
  } finally {
    $("登录提交").disabled = false;
  }
}

function 提示(文, 毫秒 = 2600) {
  const 条 = $("提示条");
  条.textContent = 文; 条.classList.add("显");
  clearTimeout(条._时);
  条._时 = setTimeout(() => 条.classList.remove("显"), 毫秒);
}

async function 调API(方法, 路径, 体) {
  const r = await fetch(路径, { method: 方法,
    headers: { "Content-Type": "application/json" },
    body: 体 === undefined ? undefined : JSON.stringify(体) });   // 同源 fetch 自动携会话 cookie（430）
  const 数据体 = await r.json().catch(() => ({}));
  if (!r.ok) {
    const 错 = new Error(数据体.错误 || "HTTP " + r.status);
    错.码 = r.status; 错.体 = 数据体;
    throw 错;
  }
  return 数据体;
}

/* —— 数据拉取与调度 —— */

async function 拉取() {
  try {
    数据 = await 调API("GET", "api/board");   // 相对路径——直连 :8301 与反代 /board/ 双通
    $("状态灯").classList.remove("断");
    $("错误条").style.display = "none";
    $("元信息").textContent = "数据时刻 " + 数据.时刻 +
      (自动刷新开() ? " · 每 8s 自动" : " · 手动刷新模式");
    渲染冲突(数据.冲突们 || []);
    // 数据一到即全页标脏+渲染激活页——消除「切页早于数据到达」的竞态空白
    // （首拉/轮询更新同路：页面永远反映最新数据，不再依赖手动切页触发）
    脏页 = { 飞行: true, 分支: true, 认领: true, 裁决: true };
    渲染激活页();
  } catch (e) {
    $("状态灯").classList.add("断");
    $("错误条").textContent = 自动刷新开()
      ? "服务连接中断，正在重试…（网络不稳可改用 https://www.cn-language.com/board/）"
      : "服务连接中断——检查网络后点 ⟳ 刷新";
    $("错误条").style.display = "block";
  }
}

function 渲染激活页() {
  if (激活板 === "交接") return 渲染交接();
  if (激活板 === "教训") return 渲染教训();
  if (激活板 === "覆盖") return 渲染覆盖();
  // 待裁决子页数据独立拉（/api/adjudications）——不被 /api/board 未到挡住
  if (激活页 === "裁决") { if (脏页.裁决) { 渲染裁决(); 脏页.裁决 = false; } return; }
  if (!数据) return;                    // 看板板块：依赖 /api/board 聚合
  if (激活页 === "飞行" && 脏页.飞行) { 渲染意图(); 脏页.飞行 = false; }
  if (激活页 === "分支" && 脏页.分支) { 渲染在飞(); 脏页.分支 = false; }
  if (激活页 === "认领" && 脏页.认领) { 渲染任务(); 脏页.认领 = false; }
}

/* —— 板块：交接（388·全宽时间流·全文直显·不折叠）—— */

const 文档缓存 = { 交接: { 时: 0, 条目们: [] }, 教训: { 时: 0, 条目们: [] },
                   覆盖: { 时: 0, 体: null }, 裁决: { 时: 0, 项们: [] } };
let 交接机 = "全部";
let 教训筛态 = "高权重";
let 覆盖筛态 = "全部";
let 裁决筛态 = "待裁决";
let 裁决选 = {};                        // 403：号→已选选项键（列表快选与详情面板共用）
let 当前教训 = null;                    // 详情面板打开的教训 id
let 当前裁决 = null;                    // 详情面板打开的裁决项号

async function 拉文档(名, 路径, 存) {
  if (Date.now() - 文档缓存[名].时 < 30000) return true;   // 30s 节流
  try {
    const r = await 调API("GET", 路径);
    存(r);
    文档缓存[名].时 = Date.now();
    return true;
  } catch (e) {
    $("错误条").textContent = "数据拉取失败：" + e.message + "（可点 ⟳ 刷新重试）";
    $("错误条").style.display = "block";
    return false;
  }
}

async function 渲染交接() {
  const 好 = await 拉文档("交接", "api/handoff?limit=100",
    (r) => { 文档缓存.交接.条目们 = r.条目们 || []; });
  const 区 = $("交接流");
  if (!好) { 区.innerHTML = '<div class="空态">服务不可达——恢复后点 ⟳ 刷新</div>'; return; }
  const 全 = 文档缓存.交接.条目们;
  $("交接计数").textContent = "最近 " + 全.length + " 条";
  const 们 = 交接机 === "全部" ? 全 : 全.filter((t) => t.机器 === 交接机);
  if (!们.length) {
    区.innerHTML = '<div class="空态">' + (全.length ? "该机暂无条目" :
      '交接流为空——收工用 <code>python scripts/board_cli.py 收工 --行 "…"</code>') + "</div>";
    return;
  }
  区.innerHTML = '<div class="面板">' + 们.map((t) =>
    '<div class="交接卡"><div class="交接头">' +
    '<span class="签 朱砂">👤 ' + 转义(t.机器) + "</span>" +
    '<span class="行时刻">' + 转义(t.时刻 || "") + "</span></div>" +
    '<div class="交接全文">' + 转义(t.条目 || "") + "</div></div>").join("") + "</div>";
}

/* —— 板块：教训（388·点条目滑出全文——直接看内容·不甩命令）—— */

async function 渲染教训() {
  const 好 = await 拉文档("教训", "api/lessons",
    (r) => { 文档缓存.教训.条目们 = r.条目们 || []; });
  const 区 = $("教训流");
  if (!好) { 区.innerHTML = '<div class="空态">服务不可达——恢复后点 ⟳ 刷新</div>'; return; }
  const 全 = 文档缓存.教训.条目们;
  const 词 = ($("教训搜索").value || "").trim().toLowerCase();
  let 们 = 全;
  if (教训筛态 === "高权重") 们 = 们.filter((t) => t.权重 >= 8 && t.正文);
  if (词) 们 = 们.filter((t) => (t.标题 || "").toLowerCase().includes(词));
  if (!们.length) {
    区.innerHTML = '<div class="空态">' + (全.length ? "无匹配教训——试试清除搜索或切「全部索引」"
      : '教训库为空——登记用 <code>board_cli 教训</code>') + "</div>";
    return;
  }
  区.innerHTML = '<div class="面板">' + 们.map((t) =>
    '<div class="任务行" data-教训="' + t.id + '">' +
    '<span class="签 朱砂">权重 ' + 转义(t.权重) + "</span>" +
    '<span class="教训题">' + 转义(t.标题 || "") + "</span>" +
    '<span class="行右侧"><span class="行时刻">' + 转义(t.时刻 || "") + "</span></span></div>").join("") + "</div>";
}

function 开教训(id) {
  const t = 文档缓存.教训.条目们.find((x) => x.id === id);
  const 面板 = $("详情面板");
  当前教训 = id;
  if (!t) { 提示("教训 #" + id + " 不在缓存——点 ⟳ 刷新后重试", 4000); return; }
  面板.innerHTML =
    '<button class="关" data-关>×</button>' +
    '<div><span class="签 朱砂">权重 ' + 转义(t.权重) + "</span> " +
    (t.标注 && t.标注 !== "活跃" ? '<span class="签 边框 挂起">' + 转义(t.标注) + "</span> " : "") +
    '<span class="行时刻">' + 转义(t.时刻 || "") + "</span></div>" +
    '<div class="详题">' + 转义(t.标题 || "") + "</div>" +
    '<div class="教训全文">' + 转义(t.正文 || "（索引条目·无全文——重登时补正文）") + "</div>";
  面板.hidden = false;
  $("详情遮罩").hidden = false;
}

/* —— 板块：E2E 覆盖（388·矩阵表格直读）—— */

async function 渲染覆盖() {
  const 好 = await 拉文档("覆盖", "api/coverage",
    (r) => { 文档缓存.覆盖.体 = r; });
  const 区 = $("覆盖流"), 豁区 = $("豁免流");
  if (!好) { 区.innerHTML = '<div class="空态">服务不可达——恢复后点 ⟳ 刷新</div>'; 豁区.innerHTML = ""; return; }
  const 体 = 文档缓存.覆盖.体 || {};
  const 单元们 = 体.单元们 || [];
  const 词 = ($("覆盖搜索").value || "").trim().toLowerCase();
  let 们 = 单元们;
  if (覆盖筛态 === "缺口") 们 = 们.filter((u) => !u.正例 || !u.边界例 || !u.负例);
  if (词) 们 = 们.filter((u) => u.单元ID.toLowerCase().includes(词) ||
                               (u.标题 || "").toLowerCase().includes(词));
  $("覆盖计数").textContent = "单元 " + 单元们.length + "·豁免 " + (体.豁免们 || []).length;
  if (!们.length) {
    区.innerHTML = '<div class="空态">' + (单元们.length ? "无匹配单元" :
      '覆盖矩阵为空——迁移未跑？<code>migrate_board_docs.py</code>') + "</div>";
  } else {
    区.innerHTML = '<div class="覆盖表"><div class="覆盖行 覆盖头"><span>单元ID</span><span>单元</span>' +
      "<span>正例</span><span>边界例</span><span>负例</span></div>" +
      们.map((u) => {
        const 缺 = (k) => !u[k] ? '<span class="签 挂起">缺</span>' : "";
        const 格 = (k) => (u[k] || "").split(",").filter(Boolean)
          .map((c) => '<span class="用例名">' + 转义(c) + "</span>").join(" ") +
          (缺(k));
        return '<div class="覆盖行"><span class="覆盖ID">' + 转义(u.单元ID) + "</span>" +
          '<span class="覆盖题">' + 转义(u.标题 || "") + "</span>" +
          "<span>" + 格("正例") + "</span><span>" + 格("边界例") + "</span><span>" + 格("负例") + "</span></div>";
      }).join("") + "</div>";
  }
  const 豁 = 体.豁免们 || [];
  豁区.innerHTML = 豁.length
    ? '<div class="面板">' + 豁.map((e) =>
        '<div class="交接卡"><div class="交接头"><span class="mono">' + 转义(e.用例) + "</span></div>" +
        '<div class="备注行">' + 转义(e.理由 || "") + "</div></div>").join("") + "</div>"
    : '<div class="空态">无豁免</div>';
}

/* —— 看板子页：待裁决（403·网页裁决——AI 讲解+选项·单项/批量提交）—— */

async function 拉裁决() {
  if (Date.now() - 文档缓存.裁决.时 < 30000) return true;   // 30s 节流（同文档板块）
  try {
    const r = await 调API("GET", "api/adjudications");
    文档缓存.裁决.项们 = r.裁决项们 || [];
    文档缓存.裁决.时 = Date.now();
    return true;
  } catch (e) {
    $("错误条").textContent = "裁决数据拉取失败：" + e.message + "（可点 ⟳ 刷新重试）";
    $("错误条").style.display = "block";
    return false;
  }
}

function 裁决行HTML(x) {
  const 已决 = x.状态 === "已裁决";
  const 选键 = 裁决选[x.号];
  const 快选 = !已决
    ? '<span class="快选组">' + (x.选项们 || []).map((o) =>
        '<button class="快选' + (选键 === o.键 ? " 选中" : "") +
        '" data-选键="' + 转义(x.号) + "|" + 转义(o.键) + '"' +
        ' title="' + 转义(o.描述) + (o.推荐 ? "（推荐）" : "") + '">' +
        (o.推荐 ? "★" : "") + 转义(o.键) + "</button>").join("") + "</span>"
    : '<span class="签 完成">已裁决·批' + 转义(x.最新结论 || "") + "</span>";
  return '<div class="任务行' + (已决 ? " 完" : "") + '" data-裁决="' + 转义(x.号) + '">' +
    '<span class="优先级 ' + 转义(x.优先级 || "P2") + '">' + 转义(x.优先级 || "—") + "</span>" +
    '<span class="任务号">#' + 转义(x.号) + "</span>" +
    '<span class="任务题文">' + 转义(概要(x.标题)) + "</span>" +
    '<span class="行右侧">' + 快选 +
    '<span class="行时刻">' + 转义(x.更新时刻 || "") + "</span></span></div>";
}

function 更新批量钮() {
  const n = Object.keys(裁决选).length;
  const 钮 = $("裁决批量钮");
  钮.disabled = n === 0;
  钮.textContent = "⚖ 提交全部已选（" + n + "）";
}

async function 渲染裁决() {
  const 好 = await 拉裁决();
  const 区 = $("裁决区");
  if (!好) { 区.innerHTML = '<div class="空态">服务不可达——恢复后点 ⟳ 刷新</div>'; return; }
  const 全 = 文档缓存.裁决.项们;
  let 们 = 裁决筛态 === "待裁决" ? 全.filter((x) => x.状态 !== "已裁决") : 全;
  if (!们.length) {
    区.innerHTML = '<div class="空态">' + (全.length ? "暂无待裁决项——切「全部」看已决留痕"
      : '裁决项为空——AI 呈报用 <code>board_cli 裁决 --批 件们.json</code>') + "</div>";
    $("裁决批量钮").disabled = true;
    $("裁决批量钮").textContent = "⚖ 提交全部已选（0）";
    更新登录UI();                          // 未登录提示常显（430·提交须作者）
    return;
  }
  区.innerHTML = '<div class="面板">' + 们.map(裁决行HTML).join("") + "</div>";
  更新批量钮();
  更新登录UI();
}

function 开裁决(号) {
  const x = 文档缓存.裁决.项们.find((v) => v.号 === String(号));
  const 面板 = $("详情面板");
  当前裁决 = String(号);
  if (!x) { 提示("裁决项 #" + 号 + " 不在缓存——点 ⟳ 刷新后重试", 4000); return; }
  if (裁决选[x.号] === undefined && x.状态 !== "已裁决") {
    const 推 = (x.选项们 || []).find((o) => o.推荐);
    if (推) 裁决选[x.号] = 推.键;      // 详情内默认选中推荐项（可改）
  }
  const 选键 = 裁决选[x.号];
  const 选项卡 = (x.选项们 || []).map((o) =>
    '<div class="裁决选项卡' + (选键 === o.键 ? " 选中" : "") +
    '" data-裁键="' + 转义(o.键) + '">' +
    '<span class="键">' + (o.推荐 ? '<span class="推荐星">★</span>' : "") +
    转义(o.键) + "</span>" + 转义(o.描述) + "</div>").join("");
  const 记录们 = (x.记录们 || []).map((r) =>
    '<div class="裁决记录行"><span class="签 朱砂">批' + 转义(r.选项键) + "</span>" +
    '<span style="flex:1">' + 转义(r.选项描述) +
    (r.意见 ? '（' + 转义(r.意见) + "）" : "") + "</span>" +
    '<span class="行时刻">' + 转义(r.裁决人) + "·" + 转义(r.时刻 || "") + "</span></div>").join("");
  面板.innerHTML =
    '<button class="关" data-关>×</button>' +
    '<div><span class="详号">#' + 转义(x.号) + "</span> " +
    '<span class="签 ' + (x.状态 === "已裁决" ? "完成" : "挂起") + '">' + 转义(x.状态) + "</span> " +
    '<span class="签 边框">' + 转义(x.优先级 || "P2") + "</span></div>" +
    '<div class="详题">' + 转义(x.标题 || "") + "</div>" +
    '<div class="详节题">AI 讲解（按语言特性与安全规则）</div>' +
    '<div class="裁决讲解">' + 转义(x.讲解 || "（无讲解）") + "</div>" +
    '<div class="详节题">裁决选项' + (x.状态 !== "已裁决" ? "（点选·★=AI 推荐）" : "（如须改主意可重选提交）") + "</div>" +
    '<div class="裁决选项组">' + 选项卡 + "</div>" +
    '<div class="详节题">补充意见与裁决人（可空）</div>' +
    '<input id="裁决意见" class="裁决输入" placeholder="补充意见（可空）" maxlength="500">' +
    '<input id="裁决人" class="裁决输入" placeholder="裁决人（缺省：用户）" maxlength="40">' +
    '<div class="详操作"><button class="钮 主" data-提交裁决="' + 转义(x.号) + '"' +
    ' id="裁决提交钮">⚖ 提交本项裁决</button></div>' +
    (记录们 ? '<div class="详节题">裁决历史</div>' + 记录们 : "");
  面板.hidden = false;
  $("详情遮罩").hidden = false;
}

async function 提交裁决(件们) {
  if (!件们.length) return;
  $("裁决批量钮").disabled = true;
  const 提交钮 = $("裁决提交钮");
  if (提交钮) 提交钮.disabled = true;
  try {
    const r = await 调API("POST", "api/adjudicate", { 裁决们: 件们 });
    const 联动 = (r.结果们 || []).filter((x) => x.任务备注联动 === "已写入备注").length;
    提示("已裁决 " + r.成功 + " 件" + (联动 ? "·" + 联动 + " 件任务备注已联动" : "") +
         (r.失败 ? "·失败 " + r.失败 + " 件" : ""), 4000);
    for (const 件 of 件们) delete 裁决选[件.号];
    文档缓存.裁决.时 = 0;               // 强制重拉（提交后状态已变）
    脏页.裁决 = true;
    关详情();
    渲染激活页();
  } catch (e) {
    if (e.码 === 401) { 已登录 = false; 更新登录UI(); 提示("须管理员登录——请输入口令", 3000); 开登录(); }
    else 提示("裁决提交失败：" + e.message, 5000);
    if (提交钮) 提交钮.disabled = false;
    更新批量钮();
  }
}

/* —— 冲突横幅（全局·任意 tab 可见）—— */

function 渲染冲突(冲突们) {
  const 带 = $("冲突带");
  带.innerHTML = 冲突们.map((c) =>
    '<div class="冲突条">⚠ 任务号 <span class="mono">#' + 转义(c.号) +
    "</span> 被 " + c.会话们.map(转义).join(" 与 ") +
    " 同时声明——开工前先核对对方状态，避免两机同做一号</div>").join("");
}

/* —— Tab A：当前在飞（382.2 用户令：本栏=**纯机器会话视图**——只显示几台机器
   正在跑什么；会话静默 15 分钟黄标·2 小时自动清除=中断即从本栏消失；
   未完成的任务在「远端在飞分支」tab 继续（真实 git 分支·TX_01 每分钟级 ls-remote），
   新机器/新会话从那里接棒）—— */

function 渲染意图() {
  const 区 = $("意图区");
  const 们 = 数据.意图们 || [];
  const 字典 = 数据.任务字典 || {};
  if (!们.length) {
    区.innerHTML = '<div class="空态">当前没有任何机器在跑任务<br><br><code>' +
      "python scripts/intent.py claim &lt;任务号&gt;</code><br>登记后 8 秒内全网可见</div>";
    return;
  }
  const 按机 = {};
  们.forEach((i) => (按机[i.机器] = 按机[i.机器] || []).push(i));
  区.innerHTML = Object.entries(按机).map(([机, 会话组]) => {
    return '<div class="机卡"><div class="机头"><span class="灯"></span>' + 转义(机) +
      '<span class="副">' + 会话组.length + " 个会话</span></div>" +
      会话组.map((i) => {
        const t = i.在做 ? 字典[i.在做] : null;
        const 计划 = (i.计划 || "").split(",").filter(Boolean)
          .map((p) => '<span class="计划签">#' + 转义(p) + "</span>").join("");
        const 静默签 = i.失联
          ? '<span class="签 边框 挂起">会话静默 ' + Math.round(i.失联秒 / 60) + " 分钟</span>"
          : "";
        return '<div class="会话"><div class="行1">' +
          '<span class="对话">' + 转义(i.对话id) + "</span>" +
          (i.在做
            ? '<button class="号牌" data-详="' + 转义(i.在做) + '">#' + 转义(i.在做) + "</button>"
            : '<span class="对话">未挂任务</span>') +
          静默签 +
          '<span class="心跳行">' + 相对时(i.时戳) + "</span></div>" +
          (t ? '<div class="任务题">' + 转义(概要(t.标题)) + "</div>" : "") +
          (计划 ? '<div style="margin-top:4px">' + 计划 + "</div>" : "") +
          (i.备注 && !(t && 备注重复标题(i.备注, t.标题))
            ? '<div class="备注行">' + 转义(i.备注) + "</div>" : "") +
          "</div>";
      }).join("") + "</div>";
  }).join("");
}

/* —— Tab B：远端在飞分支 —— */

function 渲染在飞() {
  const 区 = $("在飞区");
  const 们 = 数据.在飞分支们 || [];
  const 字典 = 数据.任务字典 || {};
  if (!们.length) {
    区.innerHTML = '<div class="空态">暂无在飞数据——<code>intent.py claim</code> 时自动上报</div>';
    return;
  }
  区.innerHTML = '<div class="面板">' + 们.map((f) => {
    const 号 = f.号 || "";
    const t = 字典[号];
    const 徽 = t && 状态签[t.状态]
      ? '<span class="签 ' + 状态签[t.状态] + '">' + 状态符[状态签[t.状态]] + " " + 状态签[t.状态] + "</span>"
      : "";
    const 主们 = f.在做主们 || [];
    const 主徽 = 主们.length
      ? '<span class="签 朱砂">👤 ' + 转义(主们.join("、")) + "</span>"
      : '<span class="签 边框 挂起">未登记意图</span>';
    return '<div class="飞行' + (f.已并入 ? " 僵尸" : "") + '" ' +
      (号 ? 'data-详="' + 转义(号) + '"' : "") + '><div class="主块">' +
      '<div class="徽排"><span class="分支名">' + 转义(f.分支) + "</span>" + 徽 + 主徽 +
      (f.未推 ? '<span class="签 边框 挂起">⚠ 分支未推·认领未生效</span>' : "") +
      (f.已并入 ? '<span class="签 待办">已并入 develop</span>' : "") + "</div>" +
      '<div class="题">' + (t ? 转义(概要(t.标题)) : 转义(概要(f.提交题 || f.提交 || ""))) + "</div>" +
      "</div></div>";
  }).join("") + "</div>";
}

/* —— Tab C：任务台账（列表/泳道·筛选/搜索）—— */

function 过滤任务() {
  const 们 = (数据.任务们 || []).slice();
  const 词 = ($("搜索框").value || "").trim().toLowerCase();
  let 出 = 们;
  if (当前筛 === "就绪") 出 = 出.filter((t) => t.就绪);
  else if (当前筛 === "挂起") 出 = 出.filter((t) => t.状态 === "⏸");
  else if (当前筛 === "完成") 出 = 出.filter((t) => t.状态 === "✅");
  if (词) 出 = 出.filter((t) =>
    t.号.toLowerCase().includes(词) || (t.标题 || "").toLowerCase().includes(词));
  return 出;
}

function 任务行HTML(t, 泳道否) {
  const 非就绪待办 = t.状态 === "⬜" && !t.就绪;
  return '<div class="任务行' + (t.状态 === "✅" ? " 完" : "") + '" data-详="' + 转义(t.号) + '">' +
    '<span class="优先级 ' + 转义(t.优先级 || "P3") + '">' + 转义(t.优先级 || "—") + "</span>" +
    '<span class="任务号">#' + 转义(t.号) + "</span>" +
    '<span class="任务题文">' + 转义(概要(t.标题)) +
    (非就绪待办 && !泳道否 ? '<span class="前置缺">前置未完</span>' : "") + "</span>" +
    '<span class="行右侧">' +
    (t.疑似认领 ? '<span class="黄标">⚠ 疑似认领中</span>' : "") +
    (t.僵尸疑 ? '<span class="黄标" title="⬜ 无分支且超期未更新——疑似已修未销账/死行：请人工核实后置 ✅ 或重开（392）">🧟 僵尸疑</span>' : "") +
    '<span class="签 ' + (状态签[t.状态] || "待办") + '">' + t.状态 + " " + (状态签[t.状态] || "") + "</span>" +
    '<span class="行时刻">' + 转义(t.更新时刻 || "") + "</span></span></div>";
}

function 渲染任务() {
  const 区 = $("任务区");
  if (当前视图 === "泳道") {
    const 列 = { "⬜": [], "🏃": [], "⏸": [], "✅": [] };
    过滤任务().forEach((t) => (列[t.状态] = 列[t.状态] || []).push(t));
    const 名 = { "⬜": "待办", "🏃": "在飞", "⏸": "挂起", "✅": "完成" };
    区.innerHTML = '<div class="泳道">' + Object.keys(列).map((s) =>
      '<div class="泳列 列' + 名[s] + '"><div class="泳列头"><span>' + s + " " + 名[s] +
      '</span><span class="计数">' + 列[s].length + "</span></div>" +
      (列[s].length ? 列[s].map((t) => 任务行HTML(t, true)).join("")
                    : '<div class="空态" style="padding:var(--space-4)">暂无</div>') +
      "</div>").join("") + "</div>";
    return;
  }
  const 们 = 过滤任务();
  if (!们.length) {
    区.innerHTML = '<div class="空态">' +
      (当前筛 === "就绪" && !(数据.任务们 || []).some((t) => t.就绪)
        ? "暂无就绪任务——所有待办的前置尚未完成<br><br>或在右侧「全部」里浏览·或新建任务"
        : "无匹配任务——试试清除搜索或换筛选") + "</div>";
    return;
  }
  区.innerHTML = '<div class="面板">' + 们.map((t) => 任务行HTML(t, false)).join("") + "</div>";
}

/* —— 详情滑出面板 —— */

function 开详情(号) {
  const t = (数据.任务们 || []).find((x) => x.号 === String(号));
  const 面板 = $("详情面板");
  当前详情 = String(号);
  if (!t) {
    面板.innerHTML = '<button class="关" data-关>×</button>' +
      '<div class="空态">#' + 转义(号) + " 不在服务端台账中——<br><br>" +
      "该分支可能立项于旧制度（行在 021 文档里）·<br>迁移完成后即可在此查看</div>";
    面板.hidden = false; $("详情遮罩").hidden = false;
    return;
  }
  const 意图们 = (数据.意图们 || []).filter((i) => i.在做 === t.号);
  const 字典 = 数据.任务字典 || {};
  const 依赖们 = (t.前置们 || []).map((p) => {
    const pt = 字典[p];
    const 完成 = pt && pt.状态 === "✅";
    return '<div class="依赖行" data-详="' + 转义(p) + '">' +
      '<span class="签 ' + (完成 ? "完成" : "待办") + '">' + (pt ? pt.状态 : "？") + "</span>" +
      '<span class="任务号">#' + 转义(p) + "</span>" +
      '<span style="flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">' +
      转义(pt ? pt.标题 : "（不在台账）") + "</span>" +
      (完成 ? "" : '<span class="未完">未完成</span>') + "</div>";
  }).join("");
  const 在做们 = 意图们.length
    ? 意图们.map((i) => '<div class="依赖行" style="cursor:default">' +
        '<span class="签 朱砂">👤 ' + 转义(i.会话键) + "</span>" +
        '<span class="行时刻">心跳 ' + 相对时(i.时戳) + "</span>" +
        (i.失联 ? '<span class="签 失联">失联</span>' : "") + "</div>").join("")
    : '<div class="备注行">无在做会话</div>';
  面板.innerHTML =
    '<button class="关" data-关>×</button>' +
    '<div><span class="详号">#' + 转义(t.号) + "</span> " +
    '<span class="签 ' + (状态签[t.状态] || "待办") + '">' + t.状态 + " " + (状态签[t.状态] || "") + "</span> " +
    (t.就绪 ? '<span class="签 完成">可认领</span>' : "") + "</div>" +
    '<div class="详题">' + 转义(t.标题 || "") + "</div>" +
    '<dl class="详字段">' +
    "<dt>优先级</dt><dd>" + 转义(t.优先级) + "</dd>" +
    "<dt>前置</dt><dd>" + (t.前置 ? 转义(t.前置) : "—") + "</dd>" +
    "<dt>分支</dt><dd>" + (t.分支 ? '<span class="mono">' + 转义(t.分支) + "</span>" : "—") + "</dd>" +
    "<dt>归属</dt><dd>" + (t.归属 ? 转义(t.归属) : "—") + "</dd>" +
    "<dt>收口 sha</dt><dd>" + (t.收口sha ? '<span class="mono">' + 转义(t.收口sha) + "</span>" : "—") + "</dd>" +
    "<dt>来源</dt><dd>" + 转义(t.来源 || "—") + "</dd>" +
    "<dt>创建</dt><dd>" + 转义(t.创建时刻 || "—") + "</dd>" +
    "<dt>更新</dt><dd>" + 转义(t.更新时刻 || "—") + "</dd>" +
    "</dl>" +
    '<div class="详节题">前置依赖链</div>' + (依赖们 || '<div class="备注行">无前置</div>') +
    '<div class="详节题">谁在做</div>' + 在做们 +
    (t.备注 ? '<div class="详节题">备注</div><div class="详备注">' + 转义(t.备注) + "</div>" : "") +
    '<div class="详操作">' +
    '<button class="钮 次" data-复制命令="' + 转义(t.号) + '">复制认领命令</button>' +
    '<button class="钮 次" data-复制号="' + 转义(t.号) + '">复制编号</button>' +
    "</div>";
  面板.hidden = false;
  $("详情遮罩").hidden = false;
}

function 关详情() {
  $("详情面板").hidden = true;
  $("详情遮罩").hidden = true;
  当前详情 = null;
}

async function 复制文本(文) {
  try {
    await navigator.clipboard.writeText(文);
    提示("已复制：" + 文);
  } catch (e) {
    提示("复制失败——请手动选择：" + 文, 5000);
  }
}

/* —— 新建任务 —— */

async function 开新建() {
  $("新建遮罩").hidden = false;
  $("新建错").textContent = "";
  $("新标题").focus();
  try {
    const r = await 调API("GET", "api/tasks");
    $("下一号签").textContent = "下一号 #" + r.下一号;
  } catch (e) {
    $("下一号签").textContent = "下一号…";
  }
}

function 关新建() { $("新建遮罩").hidden = true; }

async function 提交新建() {
  const 体 = {
    标题: $("新标题").value.trim(),
    前置: $("新前置").value.trim(),
    优先级: $("新优先级").value,
    备注: $("新备注").value.trim(),
    来源: "网页",
    机器: "网页",
  };
  if (!体.标题) { $("新建错").textContent = "标题必填"; return; }
  $("新建提交").disabled = true;
  try {
    const r = await 调API("POST", "api/task_create", 体);
    关新建();
    $("新标题").value = ""; $("新前置").value = ""; $("新备注").value = "";
    提示("已立项 #" + r.号 + "（服务端发号·全局唯一）");
    await 拉取();
    开详情(r.号);
  } catch (e) {
    if (e.码 === 401) {
      已登录 = false; 更新登录UI();
      $("新建错").textContent = "立项须管理员登录——请输入口令";
      开登录();
    } else {
      $("新建错").textContent = e.message;
    }
  } finally {
    $("新建提交").disabled = false;
  }
}

/* —— 主题 —— */

function 应用主题(名) {
  if (名) document.documentElement.dataset.theme = 名;
  else delete document.documentElement.dataset.theme;
}
应用主题(localStorage.getItem("cn_board_theme") || null);

/* —— 事件绑定 —— */

/* 顶级板块导航（388·四板块并列） */
document.querySelectorAll(".板块签").forEach((钮) => {
  钮.addEventListener("click", () => {
    document.querySelectorAll(".板块签").forEach((b) => {
      b.classList.toggle("活", b === 钮);
      b.setAttribute("aria-selected", b === 钮 ? "true" : "false");
    });
    激活板 = 钮.dataset.板;
    切板块();
  });
});

function 切板块() {
  const 子页签 = $("子页签");
  子页签.hidden = 激活板 !== "看板";
  const 目标页 = 激活板 === "看板" ? "tab-" + 激活页 : "tab-" + 激活板;
  document.querySelectorAll("main .页").forEach((p) => { p.hidden = p.id !== 目标页; });
  渲染激活页();
}

/* 看板板块子页签 */
document.querySelectorAll("#子页签 .页签").forEach((钮) => {
  钮.addEventListener("click", () => {
    document.querySelectorAll("#子页签 .页签").forEach((b) => {
      b.classList.toggle("活", b === 钮);
      b.setAttribute("aria-selected", b === 钮 ? "true" : "false");
    });
    激活页 = 钮.dataset.tab;
    切板块();
  });
});

/* —— 自动刷新（388 用户令：默认关·手动开启——防轮询重渲染打断阅读/折叠展开态）—— */
function 自动刷新开() { return localStorage.getItem("cn_board_autorefresh") === "1"; }
function 应用刷新模式() {
  const 开 = 自动刷新开();
  $("自动刷新").checked = 开;
  $("元信息").textContent = 开 ? "自动刷新每 8 秒" : "自动刷新已关（⟳ 手动刷新）";
}
$("自动刷新").addEventListener("change", (e) => {
  localStorage.setItem("cn_board_autorefresh", e.target.checked ? "1" : "0");
  应用刷新模式();
  提示(e.target.checked ? "自动刷新已开启（每 8 秒）" : "自动刷新已关闭");
});
$("刷新钮").addEventListener("click", async () => {
  文档缓存.交接.时 = 0; 文档缓存.教训.时 = 0; 文档缓存.覆盖.时 = 0;
  文档缓存.裁决.时 = 0;   // 强制重拉
  脏页 = { 飞行: true, 分支: true, 认领: true, 裁决: true };
  await 拉取();
  渲染激活页();
  提示("已刷新");
});
应用刷新模式();

$("筛选组").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  当前筛 = 钮.dataset.筛;
  document.querySelectorAll("#筛选组 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  脏页.认领 = true; 渲染激活页();
});

$("视图组").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  当前视图 = 钮.dataset.视图;
  document.querySelectorAll("#视图组 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  脏页.认领 = true; 渲染激活页();
});

$("搜索框").addEventListener("input", () => { 脏页.认领 = true; 渲染激活页(); });

/* 交接/教训/覆盖 板块筛选（388） */
$("交接机筛").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  交接机 = 钮.dataset.机;
  document.querySelectorAll("#交接机筛 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  渲染交接();
});
$("教训筛").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  教训筛态 = 钮.dataset.教;
  document.querySelectorAll("#教训筛 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  渲染教训();
});
$("教训搜索").addEventListener("input", 渲染教训);
$("覆盖筛").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  覆盖筛态 = 钮.dataset.覆;
  document.querySelectorAll("#覆盖筛 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  渲染覆盖();
});
$("覆盖搜索").addEventListener("input", 渲染覆盖);
$("新建钮").addEventListener("click", 开新建);

/* 待裁决（403）：筛选/批量提交/行内快选/面板选项与提交 */
$("裁决筛").addEventListener("click", (e) => {
  const 钮 = e.target.closest(".筛");
  if (!钮) return;
  裁决筛态 = 钮.dataset.裁;
  document.querySelectorAll("#裁决筛 .筛").forEach((b) => b.classList.toggle("活", b === 钮));
  脏页.裁决 = true; 渲染激活页();
});
$("裁决批量钮").addEventListener("click", () => {
  const 件们 = Object.entries(裁决选).map(([号, 键]) => ({ 号, 选项键: 键 }));
  提交裁决(件们);
});
$("新建取消").addEventListener("click", 关新建);
$("新建遮罩").addEventListener("click", (e) => { if (e.target === $("新建遮罩")) 关新建(); });
$("新建提交").addEventListener("click", 提交新建);

/* 管理员登录（430） */
$("登录钮").addEventListener("click", 开登录);
$("登录取消").addEventListener("click", 关登录);
$("登录遮罩").addEventListener("click", (e) => { if (e.target === $("登录遮罩")) 关登录(); });
$("登录提交").addEventListener("click", 提交登录);
$("登录口令").addEventListener("keydown", (e) => { if (e.key === "Enter") 提交登录(); });
$("详情遮罩").addEventListener("click", 关详情);
$("主题钮").addEventListener("click", () => {
  const 现 = document.documentElement.dataset.theme ||
    (matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
  const 下 = 现 === "dark" ? "light" : "dark";
  应用主题(下);
  localStorage.setItem("cn_board_theme", 下);
});

document.addEventListener("click", (e) => {
  const 提交裁 = e.target.closest("[data-提交裁决]");
  if (提交裁) {
    const 号 = 提交裁.dataset.提交裁决;
    const 键 = 裁决选[号];
    if (!键) { 提示("先选择一个裁决选项", 3000); return; }
    提交裁决([{ 号, 选项键: 键,
      裁决人: ($("裁决人")?.value || "").trim() || "用户",
      意见: ($("裁决意见")?.value || "").trim() }]);
    return;
  }
  const 裁键 = e.target.closest("[data-裁键]");
  if (裁键 && 当前裁决) {                 // 详情面板内选项卡——先于行快选判断
    裁决选[当前裁决] = 裁键.dataset.裁键;
    document.querySelectorAll("#详情面板 .裁决选项卡").forEach((c) =>
      c.classList.toggle("选中", c.dataset.裁键 === 裁键.dataset.裁键));
    return;
  }
  const 选键 = e.target.closest("[data-选键]");
  if (选键) {                             // 列表行内快选（勿冒泡开详情）
    const [号, 键] = 选键.dataset.选键.split("|");
    裁决选[号] = 裁决选[号] === 键 ? undefined : 键;
    if (!裁决选[号]) delete 裁决选[号];
    选键.parentElement.querySelectorAll(".快选").forEach((b) =>
      b.classList.toggle("选中", b.dataset.选键 === 号 + "|" + 裁决选[号]));
    更新批量钮();
    return;
  }
  const 详 = e.target.closest("[data-详]");
  if (详) { 开详情(详.dataset.详); return; }
  const 裁 = e.target.closest("[data-裁决]");
  if (裁) { 开裁决(裁.dataset.裁决); return; }
  const 教 = e.target.closest("[data-教训]");
  if (教) { 开教训(Number(教.dataset.教训)); return; }
  const 复制命令 = e.target.closest("[data-复制命令]");
  if (复制命令) {
    复制文本("python scripts/wt.py create " + 复制命令.dataset.复制命令); return;
  }
  const 复制号 = e.target.closest("[data-复制号]");
  if (复制号) { 复制文本(复制号.dataset.复制号); return; }
  if (e.target.closest("[data-关]")) 关详情();
});

document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") { 关详情(); 关新建(); 关登录(); }
});

查登录态();
拉取();
setInterval(() => { if (自动刷新开() && 激活板 === "看板") {
  脏页 = { 飞行: true, 分支: true, 认领: true, 裁决: true };   // 裁决数据自身 30s 节流·快选态存内存不丢
  拉取().then(渲染激活页);
} }, 8000);
