/* ============================================================
   CN 任务看板 · 行为层（382·看板 v2）
   数据=/api/board 每 8s 轮询；tab 隔离渲染（只渲染激活页·其余标脏）；
   详情滑出面板；网页端写操作仅「新建任务」（POST 带 Bearer 令牌——
   令牌存浏览器 localStorage·未设置时引导输入；读操作全公开）。
   ============================================================ */
"use strict";

let 数据 = null;                       // 最近一次 /api/board 聚合
let 激活页 = "飞行";
let 脏页 = { 飞行: true, 分支: true, 认领: true };
let 当前筛 = "就绪";
let 当前视图 = "列表";
let 当前详情 = null;

const $ = (id) => document.getElementById(id);
const 转义 = (s) => String(s ?? "").replace(/[&<>"]/g,
  (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
const 状态签 = { "⬜": "待办", "🏃": "在飞", "⏸": "挂起", "✅": "完成" };
const 状态符 = { 待办: "⬜", 在飞: "🏃", 挂起: "⏸", 完成: "✅" };

const 相对时 = (s) => {
  if (!s) return "";
  const 分 = Math.floor((Date.now() / 1000 - s) / 60);
  if (分 < 1) return "刚刚";
  if (分 < 60) return 分 + " 分钟前";
  const 时 = Math.floor(分 / 60);
  if (时 < 24) return 时 + " 小时前";
  return Math.floor(时 / 24) + " 天前";
};

const 取令牌 = () => localStorage.getItem("cn_board_token") || "";
const 设令牌 = () => {
  const t = prompt("看板 API 令牌（与 queue_client.json 同源·仅存本浏览器）：", 取令牌());
  if (t !== null) localStorage.setItem("cn_board_token", t.trim());
  return 取令牌();
};

function 提示(文, 毫秒 = 2600) {
  const 条 = $("提示条");
  条.textContent = 文; 条.classList.add("显");
  clearTimeout(条._时);
  条._时 = setTimeout(() => 条.classList.remove("显"), 毫秒);
}

async function 调API(方法, 路径, 体) {
  const 头 = { "Content-Type": "application/json" };
  const 令牌 = 取令牌();
  if (令牌) 头["Authorization"] = "Bearer " + 令牌;
  const r = await fetch(路径, { method: 方法, headers: 头,
    body: 体 === undefined ? undefined : JSON.stringify(体) });
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
    $("元信息").textContent = "数据时刻 " + 数据.时刻 + " · 每 8s 自动刷新";
    渲染冲突(数据.冲突们 || []);
    // 数据一到即全页标脏+渲染激活页——消除「切页早于数据到达」的竞态空白
    // （首拉/轮询更新同路：页面永远反映最新数据，不再依赖手动切页触发）
    脏页 = { 飞行: true, 分支: true, 认领: true };
    渲染激活页();
  } catch (e) {
    $("状态灯").classList.add("断");
    $("错误条").textContent =
      "服务连接中断，正在重试…（网络不稳可改用 https://www.cn-language.com/board/）";
    $("错误条").style.display = "block";
  }
}

function 渲染激活页() {
  if (!数据) return;
  if (激活页 === "飞行" && 脏页.飞行) { 渲染意图(); 脏页.飞行 = false; }
  if (激活页 === "分支" && 脏页.分支) { 渲染在飞(); 脏页.分支 = false; }
  if (激活页 === "认领" && 脏页.认领) { 渲染任务(); 脏页.认领 = false; }
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
          (t ? '<div class="任务题">' + 转义(t.标题) + "</div>" : "") +
          (计划 ? '<div style="margin-top:4px">' + 计划 + "</div>" : "") +
          (i.备注 ? '<div class="备注行">' + 转义(i.备注) + "</div>" : "") +
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
      '<div class="题">' + (t ? 转义(t.标题) : 转义(f.提交题 || f.提交 || "")) + "</div>" +
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
    '<span class="任务题文">' + 转义(t.标题 || "") +
    (非就绪待办 && !泳道否 ? '<span class="前置缺">前置未完</span>' : "") + "</span>" +
    '<span class="行右侧">' +
    (t.疑似认领 ? '<span class="黄标">⚠ 疑似认领中</span>' : "") +
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
      $("新建错").textContent = "需要 API 令牌（写操作鉴权）——即将弹出输入框";
      设令牌();
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

document.querySelectorAll(".页签").forEach((钮) => {
  钮.addEventListener("click", () => {
    document.querySelectorAll(".页签").forEach((b) => {
      b.classList.toggle("活", b === 钮);
      b.setAttribute("aria-selected", b === 钮 ? "true" : "false");
    });
    激活页 = 钮.dataset.tab;
    document.querySelectorAll("main .页").forEach((p) => { p.hidden = p.id !== "tab-" + 激活页; });
    渲染激活页();
  });
});

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
$("新建钮").addEventListener("click", 开新建);
$("新建取消").addEventListener("click", 关新建);
$("新建遮罩").addEventListener("click", (e) => { if (e.target === $("新建遮罩")) 关新建(); });
$("新建提交").addEventListener("click", 提交新建);
$("详情遮罩").addEventListener("click", 关详情);
$("主题钮").addEventListener("click", () => {
  const 现 = document.documentElement.dataset.theme ||
    (matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
  const 下 = 现 === "dark" ? "light" : "dark";
  应用主题(下);
  localStorage.setItem("cn_board_theme", 下);
});

document.addEventListener("click", (e) => {
  const 详 = e.target.closest("[data-详]");
  if (详) { 开详情(详.dataset.详); return; }
  const 复制命令 = e.target.closest("[data-复制命令]");
  if (复制命令) {
    复制文本("python scripts/wt.py create " + 复制命令.dataset.复制命令); return;
  }
  const 复制号 = e.target.closest("[data-复制号]");
  if (复制号) { 复制文本(复制号.dataset.复制号); return; }
  if (e.target.closest("[data-关]")) 关详情();
});

document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") { 关详情(); 关新建(); }
});

拉取();
setInterval(拉取, 8000);
