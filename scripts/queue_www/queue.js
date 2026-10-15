"use strict";
/* CN 门禁窗口前端（842·队列服务 8300·仿 CN 任务看板 board.js 模式）——
   三页签（最新门禁/验证池/集成队列）+ 每页 20 条分页 + 详情滑出面板：
   ①失败用例明细（名称+原因·含期望/实际 diff）②红步骤 ③develop 红灯归因
   （「自上次绿以来并入」的提交·〔任务号〕高亮）④日志尾部。
   数据面：GET api/state 聚合；详情按需 api/gate_result / api/task_result
   （相对路径——直连 :8300 与 Caddy /queue/ 反代两态同源）。
   所有插值经 转义() 防 XSS；渲染=innerHTML 字符串拼接（与看板同法）。 */

const $ = (id) => document.getElementById(id);
const 每页条数 = 20;

const 状态 = {
  页: "门禁",
  页码: { 门禁: 1, 池: 1, 队列: 1 },
  只看红: false,
  仅活跃: false,
  自动刷新: localStorage.getItem("cn_queue_autorefresh") === "1",
};
let 快照 = null;
let 定时器 = null;

function 转义(v) {
  return String(v === null || v === undefined ? "" : v).replace(/[&<>"]/g,
    (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
}

async function 调API(路径) {
  const 控 = new AbortController();
  const 计 = setTimeout(() => 控.abort(), 20000);
  try {
    const 响 = await fetch(路径, { signal: 控.signal, cache: "no-store" });
    if (!响.ok) throw new Error("HTTP " + 响.status);
    return await 响.json();
  } finally {
    clearTimeout(计);
  }
}

/* —— 通用小件 —— */
function 结果签(绿) {
  if (绿 === null || 绿 === undefined) return '<span class="签 挂起">⏳ 执行中</span>';
  return 绿 ? '<span class="签 完成">✅ 绿</span>' : '<span class="签 阶段红">❌ 红</span>';
}

const 池状态签 = (s) => ({
  "排队": '<span class="签 待办">排队</span>',
  "执行中": '<span class="签 队列中">执行中</span>',
  "完成": '<span class="签 完成">完成</span>',
  "已取消": '<span class="签 阶段灰">已取消</span>',
}[s] || `<span class="签 待办">${转义(s)}</span>`);

const 队列状态签 = (s) => ({
  "排队": '<span class="签 待办">排队</span>',
  "集成中": '<span class="签 队列中">集成中</span>',
  "冲突出批": '<span class="签 挂起">冲突出批</span>',
  "归因出批": '<span class="签 挂起">归因出批</span>',
  "已踢出": '<span class="签 阶段灰">已踢出</span>',
  "已完成": '<span class="签 完成">已完成</span>',
}[s] || `<span class="签 待办">${转义(s)}</span>`);

function 分页渲染(页名, 带索引们) {
  const 总数 = 带索引们.length;
  const 页数 = Math.max(1, Math.ceil(总数 / 每页条数));
  状态.页码[页名] = Math.min(Math.max(1, 状态.页码[页名]), 页数);
  const 当 = 状态.页码[页名];
  return {
    显示: 带索引们.slice((当 - 1) * 每页条数, 当 * 每页条数),
    控件: `<div class="分页">
      <button class="钮 次 窄" data-翻="${页名}|${当 - 1}" ${当 <= 1 ? "disabled" : ""}>‹ 上一页</button>
      <span class="页计">第 ${当} / ${页数} 页 · 共 ${总数} 条</span>
      <button class="钮 次 窄" data-翻="${页名}|${当 + 1}" ${当 >= 页数 ? "disabled" : ""}>下一页 ›</button>
    </div>`,
  };
}

/* —— 页签一：最新门禁 —— */
function 渲染门禁() {
  const 根 = $("门禁区");
  const 全部 = (快照.门禁最近 || []).map((r, i) => ({ r, i }));
  const 显 = 状态.只看红 ? 全部.filter((x) => x.r["绿"] === 0) : 全部;
  $("门禁计数").textContent = `共 ${显.length} 条${状态.只看红 ? "（只看红）" : ""}`;
  if (!显.length) {
    根.innerHTML = `<div class="空态">暂无门禁记录${状态.只看红 ? "（当前无红轮）" : ""}</div>`;
    return;
  }
  const { 显示, 控件 } = 分页渲染("门禁", 显);
  const 行 = 显示.map(({ r, i }) => `
    <div class="数据行 门禁行">
      <span class="mono">${转义(r.sha.slice(0, 10))}</span>
      <span class="mono">${转义(r.平台)}</span>
      <span>${结果签(r["绿"])}</span>
      <span class="mono">${r["绿"] === null ? "—" : 转义(r.总秒) + "s"}</span>
      <span class="时间列">${转义(r.时刻)}</span>
      <span><button class="钮 次 窄" data-详门="${i}">${r["绿"] === 0 ? "失败详情" : "详情"}</button></span>
    </div>`).join("");
  根.innerHTML = `<div class="表槽">
    <div class="数据行 表头 门禁行"><span>SHA</span><span>平台</span><span>结果</span>
      <span>耗时</span><span>时刻</span><span></span></div>
    ${行}</div>${控件}`;
}

/* —— 页签二：验证池 —— */
function 渲染池() {
  const 根 = $("池区");
  const 全部 = (快照.预验任务 || []).map((r, i) => ({ r, i }));
  $("池计数").textContent = `共 ${全部.length} 条`;
  if (!全部.length) { 根.innerHTML = `<div class="空态">验证池为空</div>`; return; }
  const { 显示, 控件 } = 分页渲染("池", 全部);
  const 行 = 显示.map(({ r }) => {
    const 源 = r["源分支"] || r["分支"];
    const 结果 = r["状态"] === "完成" ? 结果签(r["绿"]) : "—";
    return `
    <div class="数据行 池行">
      <span class="mono">${转义(源)}</span>
      <span class="mono">${转义((r.sha || "").slice(0, 10))}</span>
      <span>${池状态签(r["状态"])}</span>
      <span>${结果}</span>
      <span>${转义(r["认领者"] || "—")}</span>
      <span class="淡">${转义(r["认领限制"] || "不限")}</span>
      <span class="时间列">${转义(r["入队时刻"] || "")}</span>
      <span><button class="钮 次 窄" data-详池="${转义((r.sha || "").slice(0, 10))}"
        data-源="${转义(源)}" data-态="${转义(r["状态"])}">详情</button></span>
    </div>`;
  }).join("");
  根.innerHTML = `<div class="表槽">
    <div class="数据行 表头 池行"><span>分支</span><span>SHA</span><span>状态</span><span>结果</span>
      <span>认领者</span><span>认领限制</span><span>入队</span><span></span></div>
    ${行}</div>${控件}`;
}

/* —— 页签三：集成队列 —— */
const 活跃态们 = new Set(["排队", "集成中", "冲突出批", "归因出批"]);

function 渲染队列() {
  const 根 = $("队列区");
  const 全部 = (快照.队列 || []).map((r, i) => ({ r, i }));   // 服务端已最新在前（848）
  const 显 = 状态.仅活跃 ? 全部.filter((x) => 活跃态们.has(x.r["状态"])) : 全部;
  const 活跃数 = 全部.filter((x) => 活跃态们.has(x.r["状态"])).length;
  $("队列计数").textContent = 状态.仅活跃
    ? `活跃 ${显.length} 条（共 ${全部.length}）`
    : `共 ${全部.length} 条（活跃 ${活跃数}）`;
  if (!显.length) {
    根.innerHTML = `<div class="空态">${状态.仅活跃 ? "当前无活跃队列行（集成队列空闲）"
                                                   : "集成队列为空"}</div>`;
    return;
  }
  const { 显示, 控件 } = 分页渲染("队列", 显);
  const 行 = 显示.map(({ r }) => `
    <div class="数据行 队列行">
      <span class="mono">${转义(r["分支"])}</span>
      <span class="mono">${转义((r["基线"] || "").slice(0, 10))}</span>
      <span class="时间列">${转义(r["报名时刻"] || "")}</span>
      <span>${队列状态签(r["状态"])}</span>
      <span class="淡">${转义((r["写集摘要"] || "").slice(0, 80))}</span>
    </div>`).join("");
  根.innerHTML = `<div class="表槽">
    <div class="数据行 表头 队列行"><span>分支</span><span>基线</span><span>报名时刻</span>
      <span>状态</span><span>写集</span></div>
    ${行}</div>${控件}`;
}

function 渲染激活页() {
  if (!快照) return;
  if (状态.页 === "门禁") 渲染门禁();
  else if (状态.页 === "池") 渲染池();
  else 渲染队列();
}

/* —— 详情面板 —— */
function 面板头() { return `<button class="关" data-关 aria-label="关闭">×</button>`; }

function 面板打开(头HTML) {
  $("详情面板").innerHTML = 面板头() + 头HTML +
    `<div class="骨架行"></div><div class="骨架行"></div>`;
  $("详情遮罩").hidden = false;
  $("详情面板").hidden = false;
  $("详情面板").scrollTop = 0;
}

function 面板关闭() {
  $("详情遮罩").hidden = true;
  $("详情面板").hidden = true;
  $("详情面板").innerHTML = "";
}

function 面板去骨架() {
  $("详情面板").querySelectorAll(".骨架行").forEach((n) => n.remove());
}

function 面板补(HTML) {
  $("详情面板").insertAdjacentHTML("beforeend", HTML);
}

/* 结果详情公共区：失败用例 / e2e 统计 / 红步骤 / 日志尾部（门禁与池共用） */
function 渲染结果详情(详情) {
  const 部分 = [];
  const 明细 = 详情 && 详情["e2e明细"];
  const 失败们 = 明细 && Array.isArray(明细["失败用例"]) ? 明细["失败用例"] : [];
  if (失败们.length) {
    部分.push(`<div class="详节题">失败用例（${失败们.length}）</div>` + 失败们.map((u) => `
      <div class="败例"><div class="名">✗ ${转义(u["名称"])}</div>
      <pre>${转义(u["原因"])}</pre></div>`).join(""));
  } else {
    部分.push(`<div class="详节题">失败用例</div>
      <div class="淡">无 e2e 失败明细（旧版上报 / 未跑到 e2e / 红在构建或同步步——
      看下方红步骤与日志尾部）</div>`);
  }
  if (明细) {
    部分.push(`<div class="淡" style="margin-top:.4em">e2e：总计 ${转义(明细["总计"])}
      通过 ${转义(明细["通过"])} 失败 ${转义(明细["失败"])}
      未实现 ${转义(明细["未实现"] || 0)} 跳过 ${转义(明细["跳过"] || 0)}
      · ${转义(明细["目标平台"] || "")}</div>`);
  }
  const 步骤 = (详情 && 详情["步骤"]) || {};
  const 红步 = Object.entries(步骤).filter(([, s]) => s && (s.rc !== 0 || s["超时"]));
  if (红步.length) {
    部分.push(`<div class="详节题">红步骤</div>` + 红步.map(([名, s]) => `
      <div class="归因项"><span class="sha">${转义(名)}</span>
      <span class="主文">rc=${转义(s.rc)}${s["超时"] ? "（超时）" : ""}${
        s["说明"] ? " · " + 转义(s["说明"]) : ""}${
        s["秒"] !== undefined ? " · " + 转义(s["秒"]) + "s" : ""}</span></div>`).join(""));
  }
  const 尾 = (详情 && 详情["日志尾部"]) || [];
  if (尾.length) {
    部分.push(`<div class="详节题">日志尾部 <span class="淡">（完整日志 ${
      转义(详情["日志"] || "")} 在 runner 机 ci-logs/）</span></div>
      <pre class="日志块">${转义(尾.join("\n"))}</pre>`);
  }
  return 部分.join("");
}

/* develop 红灯归因（渲染面——算法在服务端 归因截取 单点·8300/8301 共用）：
   本次并入=自上次绿以来的提交区间。边界缺失/历史窗口未见=全列+标注。 */
function 归因区(归因) {
  const 头 = `<div class="详节题">归因 — 本次并入（自上次绿 ${
    归因 && 归因.边界sha ? 转义(归因.边界sha.slice(0, 10)) : "·列表内更早无绿"} 以来）</div>`;
  if (!归因 || !归因.历史条数) {
    return 头 + `<div class="淡">此轮无提交历史数据（旧版上报——新数据随下轮生效）</div>`;
  }
  const 提交们 = 归因.提交们 || [];
  const 尾注 = 归因.遇边 ? "" : (归因.边界sha
    ? `<div class="淡" style="margin-top:.4em">提交历史窗口内未见上次绿
       ${转义(归因.边界sha.slice(0, 10))}——以上为最近 ${归因.历史条数} 条并入（最早一条前可能还有）</div>`
    : `<div class="淡" style="margin-top:.4em">列表窗口内更早无绿行可比——以上为最近 ${归因.历史条数} 条并入（可能含更早轮的）</div>`);
  if (!提交们.length) return 头 + `<div class="淡">区间内无新增提交（同头重验？）</div>` + 尾注;
  return 头 + 提交们.map((条) => `<div class="归因项">
    <span class="sha">${转义(条.sha.slice(0, 10))}</span>
    <span class="主文">${标任务号(转义(条["主题"] || ""))}</span></div>`).join("") + 尾注;
}

/* 〔任务号〕高亮（提交主题形如「〔780〕826-D2a 宿主侧错值红根治…」） */
function 标任务号(文本) {
  return 文本.replace(/〔[^〕]{1,20}〕/g, (m) => `<span class="任务号">${m}</span>`);
}

function 详情字段行(行, 附加) {
  return `<dl class="详字段">
    <dt>SHA</dt><dd class="mono">${转义(行.sha)}</dd>
    <dt>平台</dt><dd class="mono">${转义(行.平台)}</dd>
    <dt>结果</dt><dd>${结果签(行["绿"])}</dd>
    <dt>耗时</dt><dd>${行["绿"] === null ? "—" : 转义(行.总秒) + "s"}</dd>
    <dt>时刻</dt><dd>${转义(行.时刻)}</dd>
    ${附加 || ""}
  </dl>`;
}

async function 开门禁详情(原索引) {
  const 行 = (快照.门禁最近 || [])[原索引];
  if (!行) return;
  面板打开(`<div class="详号 mono">${转义(行.sha.slice(0, 10))}</div>
    <div class="详题">最近门禁 · ${转义(行.平台)}</div>`);
  let r = null;
  try {
    r = await 调API("api/gate_result?sha=" + encodeURIComponent(行.sha)
                    + "&plat=" + encodeURIComponent(行.平台));
  } catch (e) { /* 404（旧行无详情）/网络——下方按无详情渲染 */ }
  if ($("详情面板").hidden) return;   // 加载期间被关闭
  面板去骨架();
  const 详情 = r && r.详情 ? r.详情 : null;
  面板补(详情字段行(行) +
    (详情 ? 渲染结果详情(详情) + 归因区(r && r.归因)
          : `<div class="空态">此轮无详情数据（旧行 / 免跑轮）</div>`));
}

async function 开池详情(sha10, 源分支) {
  面板打开(`<div class="详号 mono">${转义(sha10)}</div>
    <div class="详题">${转义(源分支 || "验证池任务")}</div>`);
  let r = null;
  try {
    r = await 调API("api/task_result?sha=" + encodeURIComponent(sha10));
  } catch (e) { /* 无详情 */ }
  if ($("详情面板").hidden) return;
  面板去骨架();
  const 详情 = r && r.详情 ? r.详情 : null;
  const 状态签 = r && r["状态"] ? 池状态签(r["状态"]) : "";
  面板补(`<dl class="详字段">
    <dt>SHA</dt><dd class="mono">${转义(sha10)}</dd>
    <dt>分支</dt><dd class="mono">${转义(源分支 || "")}</dd>
    <dt>状态</dt><dd>${状态签} ${r && r["绿"] !== null && r["绿"] !== undefined ? 结果签(r["绿"]) : ""}</dd>
    </dl>` +
    (详情 ? 渲染结果详情(详情)
          : `<div class="空态">此任务无详情数据（排队/执行中 / 旧行）</div>`));
}

/* —— 数据拉取 —— */
async function 拉取() {
  try {
    快照 = await 调API("api/state");
    $("状态灯").className = "灯";
    $("错误条").style.display = "none";
    $("元信息").textContent = "数据时刻 " + new Date().toLocaleTimeString("zh-CN", { hour12: false });
    渲染激活页();
  } catch (e) {
    $("状态灯").className = "灯 断";
    $("错误条").style.display = "block";
  }
}

/* —— 交互 —— */
document.addEventListener("click", (e) => {
  const 页钮 = e.target.closest("[data-页]");
  if (页钮) {
    状态.页 = 页钮.dataset.页;
    document.querySelectorAll("[data-页]").forEach((b) => {
      b.classList.toggle("活", b === 页钮);
      b.setAttribute("aria-selected", b === 页钮 ? "true" : "false");
    });
    ["门禁", "池", "队列"].forEach((名) => { $("页-" + 名).hidden = 名 !== 状态.页; });
    渲染激活页();
    return;
  }
  const 翻 = e.target.closest("[data-翻]");
  if (翻) {
    const [页名, 号] = 翻.dataset.翻.split("|");
    状态.页码[页名] = parseInt(号, 10) || 1;
    渲染激活页();
    return;
  }
  const 红 = e.target.closest("[data-红]");
  if (红) {
    状态.只看红 = 红.dataset.红 === "只看红";
    状态.页码["门禁"] = 1;
    document.querySelectorAll("[data-红]").forEach((b) => {
      b.classList.toggle("活", b === 红);
    });
    渲染门禁();
    return;
  }
  const 活 = e.target.closest("[data-活]");
  if (活) {
    状态.仅活跃 = 活.dataset.活 === "仅活跃";
    状态.页码["队列"] = 1;
    document.querySelectorAll("[data-活]").forEach((b) => {
      b.classList.toggle("活", b === 活);
    });
    渲染队列();
    return;
  }
  const 详门 = e.target.closest("[data-详门]");
  if (详门) { 开门禁详情(parseInt(详门.dataset.详门, 10)); return; }
  const 详池 = e.target.closest("[data-详池]");
  if (详池) {
    开池详情(详池.dataset.详池, 详池.dataset.源);
    return;
  }
  if (e.target.closest("[data-关]") || e.target === $("详情遮罩")) { 面板关闭(); return; }
});

document.addEventListener("keydown", (e) => { if (e.key === "Escape") 面板关闭(); });

$("刷新钮").addEventListener("click", 拉取);

/* —— 主题（墨韵双主题·localStorage·默认跟随系统）—— */
$("主题钮").addEventListener("click", () => {
  const 现 = document.documentElement.dataset.theme;
  const 当前暗 = 现 ? 现 === "dark" : matchMedia("(prefers-color-scheme: dark)").matches;
  const 新 = 当前暗 ? "light" : "dark";
  document.documentElement.dataset.theme = 新;
  localStorage.setItem("cn_queue_theme", 新);
});
(function 初始化主题() {
  const 存 = localStorage.getItem("cn_queue_theme");
  if (存) document.documentElement.dataset.theme = 存;
})();

/* —— 自动刷新（默认关·防打断阅读）—— */
function 设自动刷新(开) {
  状态.自动刷新 = 开;
  $("自动刷新").checked = 开;
  if (定时器) { clearInterval(定时器); 定时器 = null; }
  if (开) 定时器 = setInterval(拉取, 8000);
}
$("自动刷新").addEventListener("change", (e) => {
  localStorage.setItem("cn_queue_autorefresh", e.target.checked ? "1" : "0");
  设自动刷新(e.target.checked);
});

设自动刷新(状态.自动刷新);
拉取();
