"use strict";
/* CN 门禁窗口前端（842·队列服务 8300·仿 CN 任务看板 board.js 模式）——
   页签（最新门禁/验证池/实例〔1392〕/集成队列）+ 每页 20 条分页 + 详情滑出面板：
   ①失败用例明细（名称+原因·含期望/实际 diff）②红步骤 ③develop 红灯归因
   （「自上次绿以来并入」的提交·〔任务号〕高亮）④日志尾部
   ⑤实例页（1392/#1461）：逐实例 执行中/心跳超时/空闲/疑似离线（离线沉底）+ 当前任务 + 版本
   （树头·daemon指纹；指纹与众数派不同=⚠ 旧版嫌疑·众数比对在前端）。
   数据面：GET api/state 聚合；详情按需 api/gate_result / api/task_result
   （相对路径——直连 :8300 与 Caddy /queue/ 反代两态同源）。
   所有插值经 转义() 防 XSS；渲染=innerHTML 字符串拼接（与看板同法）。 */

const $ = (id) => document.getElementById(id);
const 每页条数 = 20;

const 状态 = {
  页: "门禁",
  页码: { 门禁: 1, 池: 1, 实例: 1, 队列: 1 },
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

async function 调API(路径, 选项 = {}) {
  const 控 = new AbortController();
  const 计 = setTimeout(() => 控.abort(), 20000);
  try {
    const 响 = await fetch(路径, { signal: 控.signal, cache: "no-store", ...选项 });
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

/* 975：认领失败徽标（863 五连静默机械化）——连续同类失败计数≥1 显示（同步失败×N）；
   达阈值告警（告警时刻非空）红签；title=最近失败原因摘要（hover 可读原文）。 */
function 失败徽(r) {
  const 数 = r["失败计数"] || 0;
  if (!数) return "";
  const 签 = r["告警时刻"] ? "签 阶段红" : "签 挂起";
  const 题 = `${r["失败类别"] || "失败"}：${(r["失败摘要"] || "").slice(0, 160)}`
    + (r["告警时刻"] ? ` · 已告警 ${r["告警时刻"]}` : "");
  return ` <span class="${签}" title="${转义(题)}">⚠ ${转义(r["失败类别"] || "失败")}×${数}</span>`;
}

/* 825 残余 A：完成红「失败原因」徽（红步骤名+失败用例名·title 全文）——真实红首因
   行内可见，不必点开详情（构建/单测/e2e 红不再只有 rc）。 */
function 红因徽(r) {
  if (r["状态"] !== "完成" || r["绿"] !== 0 || !r["失败原因"]) return "";
  const 因 = String(r["失败原因"]);
  return ` <span class="签 阶段红" title="${转义("失败原因：" + 因)}">${
    转义(因.length > 26 ? 因.slice(0, 26) + "…" : 因)}</span>`;
}

/* 825 残余 B（并行度观测）：池忙闲摘要条——注册实例/执行中/排队数+三态提示；
   池饱和时明示「排队是正常态·有空闲实例即被认领」（用户 10-15「预检才两个·
   池实例空转」观察面）。快照无池忙闲字段=隐藏（旧服务端兼容）。 */
function 渲染忙闲条() {
  const 条 = $("池忙闲条");
  const 忙 = (快照 && 快照["池忙闲"]) || null;
  if (!忙) { 条.hidden = true; return; }
  const 注 = Number(忙["注册实例"] || 0), 执 = Number(忙["执行中"] || 0),
        排 = Number(忙["排队"] || 0);
  let 态 = '<span class="签 完成">有空闲实例</span> <span class="淡">排队行将在数秒内被认领</span>';
  if (注 === 0) {
    态 = '<span class="签 挂起">暂无注册实例</span> <span class="淡">runner 未上线或尚未认领过任务</span>';
  } else if (忙["池全忙"]) {
    态 = '<span class="签 阶段红">池全忙</span> <span class="淡">排队是正常态——有空闲实例即被认领（先空闲先认领）</span>';
  }
  条.hidden = false;
  条.innerHTML = `实例注册 <b class="mono">${注}</b> · 执行中 <b class="mono">${执}</b> · `
    + `排队 <b class="mono">${排}</b> ｜ ${态}`;
}

/* 825 残余 B：排队原因（行内 title）——池全忙/限平台/有空闲待认领三分支。 */
function 排队因(r) {
  const 忙 = (快照 && 快照["池忙闲"]) || {};
  if (忙["池全忙"]) return "池全忙——等任一实例空出（先空闲先认领）";
  if (r["认领限制"]) return `限 ${r["认领限制"]} 平台实例认领——该平台空闲即被领走`;
  return "池有空闲实例——待实例轮询认领（数秒级）";
}

/* 入队时刻（%m-%d %H:%M:%S·无年份）→ 已等待分钟；坏串/未来时刻=null（宁缺不编）。 */
function 等分(时刻) {
  const m = /^(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/.exec(String(时刻 || ""));
  if (!m) return null;
  const t = new Date(new Date().getFullYear(), +m[1] - 1, +m[2], +m[3], +m[4], +m[5]);
  const 差 = (Date.now() - t.getTime()) / 60000;
  return 差 >= 0 ? Math.round(差) : null;
}

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
  渲染忙闲条();
  const 全部 = (快照.预验任务 || []).map((r, i) => ({ r, i }));
  $("池计数").textContent = `共 ${全部.length} 条`;
  if (!全部.length) { 根.innerHTML = `<div class="空态">验证池为空</div>`; return; }
  const { 显示, 控件 } = 分页渲染("池", 全部);
  const 行 = 显示.map(({ r }) => {
    const 源 = r["源分支"] || r["分支"];
    const 结果 = r["状态"] === "完成" ? 结果签(r["绿"]) : "—";
    const 等待 = r["状态"] === "排队" || r["状态"] === "执行中" ? 等分(r["入队时刻"]) : null;
    const 时 = 转义(r["入队时刻"] || "")
      + (等待 === null ? "" : `（等 ${等待} 分）`);
    return `
    <div class="数据行 池行">
      <span class="mono">${转义(源)}</span>
      <span class="mono">${转义((r.sha || "").slice(0, 10))}</span>
      <span>${池状态签(r["状态"])}</span>
      <span>${结果}${失败徽(r)}${红因徽(r)}</span>
      <span>${转义(r["认领者"] || "—")}</span>
      <span class="淡">${转义(r["认领限制"] || "不限")}</span>
      <span class="时间列" title="${转义(r["状态"] === "排队" ? 排队因(r) : "")}">${时}</span>
      <span><button class="钮 次 窄" data-详池="${转义((r.sha || "").slice(0, 10))}"
        data-源="${转义(源)}" data-态="${转义(r["状态"])}">详情</button></span>
    </div>`;
  }).join("");
  根.innerHTML = `<div class="表槽">
    <div class="数据行 表头 池行"><span>分支</span><span>SHA</span><span>状态</span><span>结果</span>
      <span>认领者</span><span>认领限制</span><span>入队</span><span></span></div>
    ${行}</div>${控件}`;
}

/* —— 页签三：实例（1392·云端实例实时动态）—— */
const 实例状态签 = (s) => ({
  "执行中": '<span class="签 队列中">执行中</span>',
  "心跳超时": '<span class="签 阶段红">心跳超时</span>',
  "疑似离线": '<span class="签 挂起">疑似离线</span>',
  "空闲": '<span class="签 完成">空闲</span>',
}[s] || `<span class="签 待办">${转义(s)}</span>`);

/* epoch 秒 → 「X 前」（相对时刻·负差/缺失=刚刚/—·不编）。 */
function 秒前(时戳) {
  if (!时戳) return "—";
  const 差 = Math.round(Date.now() / 1000 - Number(时戳));
  if (差 < 0) return "刚刚";
  if (差 < 60) return 差 + " 秒前";
  if (差 < 3600) return Math.round(差 / 60) + " 分钟前";
  return Math.round(差 / 3600) + " 小时前";
}

/* 版本一致性（#1418）：树头优先——同树头=同提交（跨平台行尾差异不影响）；
   树头与众数不同（且非空）=真旧版（红）；同树头但指纹不同=文件字节差异
   （黄提示·非旧版）。服务端无权威最新版本源，多数派比对是抓旧版的实用信号。
   #1465：执行中/心跳超时实例的树头=任务分支 checkout（预期语义，不是旧版）——
   不参与众数、不判红；跑完自动回 develop（#1463 跑完对齐）后恢复参与。 */
function 有任务树头(r) { return r["状态"] === "执行中" || r["状态"] === "心跳超时"; }

function 版本众数(们) {
  const 可比 = 们.filter((r) => !有任务树头(r));
  const 头计数 = {};
  可比.forEach((r) => { const h = r["树头"]; if (h) 头计数[h] = (头计数[h] || 0) + 1; });
  const 头条目 = Object.entries(头计数).sort((a, b) => b[1] - a[1])[0] || null;
  const 指计数 = {};
  可比.forEach((r) => { const f = r["daemon指纹"]; if (f) 指计数[f] = (指计数[f] || 0) + 1; });
  const 指条目 = Object.entries(指计数).sort((a, b) => b[1] - a[1])[0] || null;
  return { 树头众数: 头条目 ? 头条目[0] : null,
           真旧版: (r) => !有任务树头(r) && 头条目 !== null && Boolean(r["树头"])
                          && r["树头"] !== 头条目[0],
           指纹差异: (r) => !有任务树头(r) && 头条目 !== null && r["树头"] === 头条目[0]
                            && Boolean(r["daemon指纹"]) && 指条目 !== null
                            && r["daemon指纹"] !== 指条目[0],
           有版本: (r) => Boolean(r["树头"]) || Boolean(r["daemon指纹"]) };
}

function 渲染实例统计条(们) {
  const 条 = $("实例统计条");
  if (!们.length) { 条.hidden = true; return; }
  const 计 = {};
  们.forEach((r) => { 计[r["状态"]] = (计[r["状态"]] || 0) + 1; });
  const 众 = 版本众数(们);
  const 真旧数 = 们.filter((r) => 众.真旧版(r)).length;
  const 指纹异数 = 们.filter((r) => 众.指纹差异(r)).length;
  const 有版本 = 们.some((r) => 众.有版本(r));
  条.hidden = false;
  条.innerHTML = `实例 <b class="mono">${们.length}</b> · 执行中 <b class="mono">${计["执行中"] || 0}</b> · `
    + `心跳超时 <b class="mono">${计["心跳超时"] || 0}</b> · 疑似离线 <b class="mono">${计["疑似离线"] || 0}</b> · `
    + `空闲 <b class="mono">${计["空闲"] || 0}</b> ｜ `
    + (真旧数
      ? `<span class="签 阶段红">版本不一致 ${真旧数} 台</span> <span class="淡">树头与众数不同——疑似旧版未更新</span>`
      : (!有版本
        ? `<span class="签 挂起">版本未上报</span> <span class="淡">全部实例尚无树头/指纹（旧版进程未发 instance_ping）——升级重启后出现</span>`
        : `<span class="签 完成">版本一致</span>`
          + (指纹异数
            ? ` <span class="签 挂起">指纹差异 ${指纹异数} 台</span> <span class="淡">同提交但文件字节不同（跨平台行尾）——非旧版</span>`
            : "")));
}

function 渲染实例() {
  const 根 = $("实例区");
  const 们 = 快照["实例动态"] || [];
  渲染实例统计条(们);
  /* #1460：上次「更新实例」广播时刻（0/缺=从未广播——旧服务端无此字段亦显示空） */
  $("实例刷新提示").textContent = 快照["刷新请求"]
    ? "上次更新广播 " + 秒前(快照["刷新请求"]) : "";
  $("实例计数").textContent = `共 ${们.length} 台`;
  if (!们.length) {
    根.innerHTML = `<div class="空态">暂无注册实例（runner 认领任务或空闲报活后出现在此）</div>`;
    return;
  }
  const 众 = 版本众数(们);
  const 行 = 们.map((r) => {
    const 任 = r["当前任务"];
    const 完 = r["最近完成"];
    const 真旧 = 众.真旧版(r);
    const 指纹异 = 众.指纹差异(r);
    const 已跑 = 任 ? 等分(任["认领时刻"]) : null;
    const 片签 = 任 && 任["形态"] === "分片"
      ? ` <span class="签 队列中">分片 #${转义(String(任["片序"] ?? "?"))}/${转义(String(任["片数"] ?? "?"))}</span>`
      : "";
    const 当前 = 任
      ? `${转义(任["源分支"] || 任["分支"])}${片签}<span class="淡"> · ${转义((任["sha"] || "").slice(0, 10))}</span>`
      : "—";
    const 完成列 = 完
      ? `${完["绿"] ? "✅" : "❌"} <span class="mono">${转义((完["sha"] || "").slice(0, 10))}</span>
         <span class="时间列">${转义(完["时刻"] || "")}</span>`
      : "—";
    return `
    <div class="数据行 实例行">
      <span class="mono" title="${转义(r["注册时刻"] ? "注册于 " + r["注册时刻"] : "")}">${转义(r["名称"])}</span>
      <span class="mono 淡">${转义(r["平台"] || "—")}</span>
      <span>${实例状态签(r["状态"])}</span>
      <span class="mono">${当前}</span>
      <span class="mono">${已跑 === null ? "—" : 已跑 + " 分"}</span>
      <span class="时间列">${秒前(r["最近活跃时戳"])}</span>
      <span class="mono${真旧 ? " 版本警" : ""}" title="树头（同提交判定主依据）+ daemon 指纹（进程启动版）${
        真旧 ? " · ⚠ 树头与众数不同——疑似旧版 daemon 未更新"
             : (指纹异 ? " · 同提交但指纹不同（跨平台行尾差异·非旧版）" : "")}">${
        转义(r["树头"] || "—")} · ${转义(r["daemon指纹"] || "—")}${真旧 ? " ⚠" : ""}</span>
      <span>${完成列}</span>
    </div>`;
  }).join("");
  根.innerHTML = `<div class="表槽">
    <div class="数据行 表头 实例行"><span>实例</span><span>平台</span><span>状态</span>
      <span>当前任务</span><span>已跑</span><span>最近报活</span><span>版本（树头·指纹）</span><span>最近完成</span></div>
    ${行}</div>`;
}

/* —— 页签四：集成队列 —— */
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
  else if (状态.页 === "实例") 渲染实例();
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
   本次并入=自上次绿以来的提交区间（边界=链内最近绿·1070 起带平台/时刻）；
   边界缺失/历史窗口未见=全列+标注。本行sha10 标记红提交在区间中的位置。 */
function 归因区(归因, 本行sha10) {
  const 边界题 = 归因 && 归因.边界sha
    ? 转义(归因.边界sha.slice(0, 10)) +
      (归因.边界平台 ? `（${转义(归因.边界平台)} · ${转义(String(归因.边界时刻 || "").slice(0, 11))}）` : "")
    : "·列表内更早无绿";
  const 头 = `<div class="详节题">归因 — 本次并入（自上次绿 ${边界题} 以来）</div>`;
  if (!归因 || !归因.历史条数) {
    return 头 + `<div class="淡">此轮无提交历史数据（旧版上报——新数据随下轮生效）</div>`;
  }
  const 提交们 = 归因.提交们 || [];
  const 尾注 = 归因.遇边 ? "" :
    `<div class="淡" style="margin-top:.4em">提交历史窗口内未见上次绿——以上为最近 ${
      归因.历史条数} 条并入（可能不完整·更旧行随窗口滚动）</div>`;
  if (!提交们.length) return 头 + `<div class="淡">区间内无新增提交（同头重验？）</div>` + 尾注;
  return 头 + 提交们.map((条) => `<div class="归因项">
    <span class="sha">${转义(条.sha.slice(0, 10))}</span>
    <span class="主文">${标任务号(转义(条["主题"] || ""))}${
      本行sha10 && String(条.sha).startsWith(本行sha10)
        ? ' <span class="任务号">本行</span>' : ""}</span></div>`).join("") + 尾注;
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
    (详情 ? 渲染结果详情(详情) + 归因区(r && r.归因, 行.sha.slice(0, 10))
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
    (r && r["失败原因"] ? `<div class="详节题">失败原因（完成红首因·825）</div>
      <div class="归因项"><span class="sha">首因</span>
      <span class="主文">${转义(r["失败原因"])}</span></div>` : "") +
    (r && r["失败计数"] ? `<div class="详节题">认领失败（975·连续同类 ${转义(r["失败计数"])} 次）</div>
      <div class="归因项"><span class="sha">${转义(r["失败类别"] || "失败")}</span>
      <span class="主文">${转义(r["失败摘要"] || "无摘要")}${
        r["告警时刻"] ? ` · 已告警 ${转义(r["告警时刻"])}` : ""}</span></div>` : "") +
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
    ["门禁", "池", "实例", "队列"].forEach((名) => { $("页-" + 名).hidden = 名 !== 状态.页; });
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

/* —— #1460 更新实例：广播「对齐 develop 树头」——空闲实例下次报活（≤60s）携走戳、
   轮间空闲点（≤90s）执行 fetch+reset；执行中实例跑完自动跟随。旧版 daemon 需升级
   （旧进程忽略该戳·页面提示「上次更新广播」照常记录）。 */
$("更新实例钮").addEventListener("click", async () => {
  const 钮 = $("更新实例钮");
  钮.disabled = true;
  try {
    await 调API("api/instance_refresh",
      { method: "POST", headers: { "Content-Type": "application/json" }, body: "{}" });
    await 拉取();
  } catch (e) {
    $("实例刷新提示").textContent =
      "广播失败（" + e.message + "）——服务端未升级或登录已过期";
  } finally {
    钮.disabled = false;
  }
});

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
