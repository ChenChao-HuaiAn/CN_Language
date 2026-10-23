#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""board_cli.py —— 文档资源统一 CLI（384·文档上服务器二期）。

交接/教训/规范覆盖三资源的写入口（服务端 board_docs 资源·TX_01:8301）：
  python scripts/board_cli.py 收工 --行 "本轮一句话+下一棒指针"   # 交接追加+会话注销（收工一条命令）
  python scripts/board_cli.py 交接 --行 "条目" [--机器 深度机]     # 只写交接
  python scripts/board_cli.py 教训 --标题 "标题（权重 N）" [--正文 "..."] [--权重 N] [--标注 活跃]
  python scripts/board_cli.py 教训索引 [--高权重 8]                # 扫读：一行索引/高权重全文标题
  python scripts/board_cli.py 教训看 <id>                          # 看全文
  python scripts/board_cli.py 用例 <单元ID> <正例|边界例|负例> <用例名[,用例名…]>   # 归入单元格（追加）
  python scripts/board_cli.py 豁免 <用例名[,用例名…]> <理由>        # 豁免登记（理由必填）
  python scripts/board_cli.py 单元删 <单元ID>                      # 删除覆盖单元行（误登记清洗·247）
  python scripts/board_cli.py 豁免删 <用例名[,用例名…]>             # 删除豁免行（撞号留痕/任务级行清洗·247）
  python scripts/board_cli.py 覆盖                                 # 覆盖矩阵摘要（缺口=空格）
  python scripts/board_cli.py 领用例号 <用例名> [--说明 "..."] [--号 N]   # E2E 用号领取（554·577 号名配对唯一）
  python scripts/board_cli.py 查用例号 [用例名]                          # 台账查询（建目录前自检·577）
  python scripts/board_cli.py 补用例名 <号> <用例名>                     # 已发放存量号补配名（811·未配名号专属）
  python scripts/board_cli.py 裁决 --批 <件们.json>                 # 裁决项批量上传（403·AI 侧）
  python scripts/board_cli.py 裁决 --号 379 --标题 "…" --讲解文件 x.md --选项文件 o.json
  python scripts/board_cli.py 裁决看 [--全部]                       # 裁决项一览（缺省只看待裁决）
  python scripts/board_cli.py 裁决删 <号>                           # 删待裁决件（471·误登记清理·已裁决终态禁删）
批文件格式：[{号,标题,优先级?,讲解,选项们:[{键,描述,推荐?}]}]——讲解长文走文件不走命令行。
机器名自动=CN_MACHINE_NAME（唯一覆盖口）或 hostname·平台-架构·指纹6（508·零配置·hostname 撞名根治）；--机器 可显式指定。
"""
from __future__ import annotations

import json
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

import atomic_json

脚本目录 = Path(__file__).resolve().parent
本树根 = 脚本目录.parent
默认地址 = "http://124.222.106.84:8301"


def 服务地址() -> str:
    import os
    return os.environ.get("CN_BOARD_URL", 默认地址).rstrip("/")


def 令牌() -> str:
    """556：委托 intent.读令牌() 统一兜底链（本树→主树副本→环境变量）·归纳零复制。"""
    import intent
    return intent.读令牌()


def 机器名(显式: str = "") -> str:
    """交接/教训归属标签（508）：显式 --机器 直通；否则统一走 intent 权威实现
    （hostname·平台-架构·指纹6·CN_MACHINE_NAME 覆盖口）——永不返回空。
    494 残留的三机名白名单/hostname 前缀别名猜测层废除（hostname 已撞名，猜测必错标：
    深度机 ChenChao-PC 会被别名表错标「家机」）。"""
    if 显式:
        return 显式
    import intent
    return intent.机器名()


def _失败文案(服务: str, 码: int, r: dict) -> str:
    """读/写入口统一失败诊断（1425）：401/403=鉴权失败（点名令牌/登录墙），
    其余=服务端错误码+响应体。非 dict 响应归一 {} 不崩栈；助手坏掉时本地
    码兜底仍识别 401/403（P3-D·门禁/写入口不降级）。"""
    if not isinstance(r, dict):
        r = {}
    鉴权失败 = 码 in (401, 403)
    try:
        import intent
        鉴权失败 = intent.鉴权失败码(码)
        if 鉴权失败:
            return intent.鉴权失败文案(服务, 码)
    except Exception as e:
        print(f"[黄] intent 鉴权助手不可用，按裸码兜底（诊断见上）：{e}", file=sys.stderr)
    if 鉴权失败:
        return f"{服务}鉴权失败（{码}）——令牌校验失败（详见上方诊断）"
    return f"{服务}返回 HTTP {码}：{r.get('错误', r)}"


def _读失败文案(服务: str, 码: int, r: dict) -> str:
    """GET 失败诊断（1425）——与写入口同文案口径（单点 _失败文案）。"""
    return _失败文案(服务, 码, r)


def _写失败文案(服务: str, 码: int, r: dict) -> str:
    """写入口失败诊断（1425）——与读入口同文案口径（单点 _失败文案）。"""
    return _失败文案(服务, 码, r)


def 调服务(方法: str, 路径: str, 体=None, 超时=15):
    """返回 (HTTP码, 响应dict)。GET/POST 有令牌就带 Bearer（1425 全站登录墙：
    GET 亦须鉴权·客户端统一「有令牌就带」口径）；服务不可达抛 OSError 由调用方兜底。"""
    请求 = urllib.request.Request(服务地址() + 路径, method=方法,
        data=json.dumps(体, ensure_ascii=False).encode("utf-8") if 体 is not None else None)
    try:
        import intent
        intent.加鉴权头(请求)   # 1425/P2-A：Bearer 写不可重定向头（跨源跳转不外发）
    except Exception as e:
        print(f"[黄] intent 鉴权助手不可用，本次裸请求（诊断见上）：{e}", file=sys.stderr)
    请求.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(请求, timeout=超时) as r:
            return r.status, json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        try:
            return e.code, json.loads(e.read().decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return e.code, {}


def 落缓存(名: str, 体) -> None:
    """GET 结果落本地缓存（board_cache.json·gitignore）——服务不可达时 standup/
    check_spec_coverage 降级读最后一份，不卡本地流程。556：读写统一走 atomic_json
    （原子写治三写者并发半写损坏·读容错治坏缓存炸读侧）。"""
    atomic_json.合并写(脚本目录 / "board_cache.json",
                       {名: 体, f"{名}时戳": time.time()})


def cmd_收工(a) -> int:
    if not a.行:
        print("[失败] --行 必填（本轮一句话+下一棒指针·v5 两行制）")
        return 2
    码, r = 调服务("POST", "/api/handoff_add", {"机器": 机器名(a.机器), "条目": a.行,
                                                "来源": "board_cli收工"})
    if 码 not in (200, 201):
        print(f"[失败] 交接写入：{_写失败文案('看板服务', 码, r)}")
        return 1
    print(f"[交接] 已登记（#{r.get('id')}）")
    # 会话注销（308i 会话粒度·与 intent.py 同键）
    sys.path.insert(0, str(脚本目录))
    try:
        import intent
        码2, r2 = 调服务("POST", "/api/intent_release",
                        {"机器": intent.机器名(), "对话id": intent.会话id()})
        print("[意图] 会话已注销" if 码2 == 200
              else f"[意图] 注销跳过（{_写失败文案('看板服务', 码2, r2)}）")
    except Exception as e:      # 注销失败不拦收工（看板 2h 惰性清兜底）
        print(f"[意图] 注销跳过（{e}）")
    return 0


def cmd_交接(a) -> int:
    if not a.行:
        print("[失败] 须 --行 条目")
        return 2
    码, r = 调服务("POST", "/api/handoff_add", {"机器": 机器名(a.机器), "条目": a.行,
                                                "来源": "board_cli"})
    if 码 not in (200, 201):
        print(f"[失败] {_写失败文案('看板服务', 码, r)}")
        return 1
    print(f"[交接] 已登记（#{r.get('id')}）")
    return 0


def cmd_教训(a) -> int:
    if not a.标题:
        print("[失败] --标题 必填（权重可写在标题里「权重 N」或 --权重 显式）")
        return 2
    码, r = 调服务("POST", "/api/lesson_add",
                  {"标题": a.标题, "正文": a.正文 or "", "权重": a.权重,
                   "标注": a.标注 or "活跃", "来源": "board_cli"})
    if 码 not in (200, 201):
        print(f"[失败] {_写失败文案('看板服务', 码, r)}")
        return 1
    动作 = "幂等跳过（同标题已在库）" if r.get("跳过") else f"已登记（#{r.get('id')}）"
    print(f"[教训] {动作}")
    return 0


def cmd_教训索引(a) -> int:
    try:
        键 = f"high_weight={a.高权重}" if a.高权重 else "index=1"
        码, r = 调服务("GET", f"/api/lessons?{键}")
    except OSError as e:
        print(f"[黄] 服务不可达（{e}）——读本地缓存")
        # 556：读侧走 atomic_json.读（坏缓存返回 None 黄字·原裸 json.loads 坏缓存即炸）
        r = (atomic_json.读(脚本目录 / "board_cache.json") or {}).get("lessons", {})
    else:
        if 码 != 200:
            print(f"[黄] {_读失败文案('看板服务', 码, r)}——读本地缓存")
            r = (atomic_json.读(脚本目录 / "board_cache.json") or {}).get("lessons", {})
        else:
            落缓存("lessons", r)
    for t in r.get("条目们", []):
        尾 = f"〔{t['标注']}〕" if t.get("标注", "活跃") != "活跃" else ""
        print(f"- #{t['id']} {t['标题']}（权重 {t['权重']}）{尾}")
    return 0


def cmd_教训看(a) -> int:
    码, r = 调服务("GET", "/api/lessons")
    if 码 != 200:
        print(f"[失败] {_读失败文案('看板服务', 码, r)}")
        return 1
    目 = next((t for t in r.get("条目们", []) if t["id"] == a.id), None)
    if not 目:
        print(f"[失败] 无 id={a.id}")
        return 1
    print(f"# {目['标题']}（权重 {目['权重']}·{目['时刻']}）\n\n{目['正文']}")
    return 0


def cmd_用例(a) -> int:
    """561：改走 /api/coverage_append 单条追加口（服务端内读改写原子）——原
    GET→拼接→POST 两步在两会话并发登记同单元同列时丢更新（后写覆盖先写）。"""
    if a.态 not in ("正例", "边界例", "负例"):
        print("[失败] 三态须为 正例|边界例|负例")
        return 2
    用例们 = ",".join(c.strip() for c in a.用例名.split(",") if c.strip())
    if not 用例们:
        print("[失败] 用例名必填（逗号分隔可批量）")
        return 2
    码, r = 调服务("POST", "/api/coverage_append",
                  {"单元ID": a.单元ID, "态": a.态, "用例名": 用例们})
    if 码 != 200:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}（写操作宁停不撞）")
        return 1
    尾 = f"新增 {r.get('新增')}" + (f"·重复跳过 {r['重复跳过']}" if r.get("重复跳过") else "")
    print(f"[用例] 单元 {a.单元ID}·{a.态} 已追加（{尾}·该列现 {r.get('该列数')} 例）")
    return 0


def cmd_豁免(a) -> int:
    if not a.用例名 or not a.理由:
        print("[失败] 豁免须 用例名+理由（防漏映射逃逸·check_spec_coverage --ci 口径）")
        return 2
    码, r = 调服务("POST", "/api/coverage_exempt_add",
                  {"用例": a.用例名, "理由": a.理由})
    if 码 != 200:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}")
        return 1
    print(f"[豁免] 已登记 {r['登记数']} 例")
    return 0


def cmd_单元删(a) -> int:
    """删除覆盖单元行（误登记/脏行清洗通道·247·仿 471 裁决删先例）。
    成功打服务端 journal（谁/何时/删了哪行·可审计）；不存在 404。"""
    if not a.单元ID:
        print("[失败] 单元ID 必填")
        return 2
    码, r = 调服务("POST", "/api/coverage_delete", {"单元ID": a.单元ID})
    if 码 != 200:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}")
        return 1
    行 = r.get("单元", {})
    print(f"[单元删] {行.get('单元ID')}（{行.get('标题', '')}）已删除")
    return 0


def cmd_豁免删(a) -> int:
    """删除豁免行（撞号改号留痕旧行/任务级行清洗·247/387）。
    用例名可逗号分隔批量；未命中的名字照实报告（幂等重放安全）。"""
    if not a.用例名:
        print("[失败] 用例名 必填（逗号分隔可批量）")
        return 2
    码, r = 调服务("POST", "/api/coverage_exempt_delete", {"用例": a.用例名})
    if 码 != 200:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}"
              + (f"（未命中：{','.join(r.get('未命中', []))}）" if r.get('未命中') else ""))
        return 1
    print(f"[豁免删] 已删 {r['删除数']} 行：" + ",".join(r.get("删除", [])))
    if r.get("未命中"):
        print(f"[豁免删] 未命中（未删）：" + ",".join(r["未命中"]))
    return 0


def cmd_覆盖(a) -> int:
    码, r = 调服务("GET", "/api/coverage")
    if 码 != 200:
        print(f"[失败] {_读失败文案('看板服务', 码, r)}")
        return 1
    落缓存("coverage", r)
    们 = r["单元们"]
    print(f"覆盖单元 {len(们)}·豁免 {len(r['豁免们'])}")
    for u in 们:
        缺 = [k for k in ("正例", "边界例", "负例") if not u[k]]
        print(f"  {u['单元ID']:<10} {u['标题'][:24]:<26}"
              f"{'缺口:' + ','.join(缺) if 缺 else '✓'}")
    return 0


def cmd_领用例号(a) -> int:
    """E2E 用例编号领取（554 建·577 号名配对唯一·744 编号权威收权）。
    服务端用例号台账原子发号：客户端只报用例名（描述）——编号由服务器按顺序
    发放（2026-10-14 用户令·带 请求号下限/指定号 旧通道已废）；号或用例名任一
    被占即 409 拒发——建用例目录前先领号（号↔用例名绑定即锁死，跨分支也可见·
    根治 815/816 型互不可见撞号）。服务不可达=硬拒（离线自编号已废·与任务立项
    「宁停不裂」同口径——自编号正是台账错位制造源）。"""
    用例名 = (a.用例名 or "").strip()
    if not 用例名:
        print("[失败] 用例名必填——E2E 领号只报用例名描述（2026-10-14 用户令："
              "编号由服务器发放）。用法：领用例号 <用例名>（建目录前完成，号名绑定才拦得住跨分支撞号）")
        return 1
    体 = {"机器": 机器名(), "用例名": 用例名, "说明": a.说明 or ""}
    try:
        码, r = 调服务("POST", "/api/case_number", 体)
    except OSError as e:
        print(f"[失败] 看板服务不可达（{e}）——领号硬拒不降级（2026-10-14 用户令："
              f"编号由服务器发放·离线自编号已废除；恢复网络/核对 CN_BOARD_URL 后重试）")
        return 1
    if 码 == 409:
        print(f"[拒发] {r.get('错误', '号或用例名已被占用')}")
        print("  处置：换用例名，或直接 `领用例号 <用例名>` 领服务端新号（勿复用他人号）")
        return 1
    if 码 != 201:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}——领号硬拒不降级")
        return 1
    号 = r["号"]
    print(f"[领用例号] {号} ← 「{用例名}」"
          + f"（服务端顺序发放·台账最大 {r.get('台账最大')}·占用 {r.get('占用数')}）")
    print(f"  命名：tests/e2e/{号}_<主题>；v2 面成对={号}_v2_<主题>（共号合法形态·台账只登记宿主名）")
    print(f"  撞号处置=后入者领新号：git mv 旧目录 新目录 + 覆盖矩阵登记名同步")
    return 0


def cmd_查用例号(a) -> int:
    """用例号台账查询（577）——建目录前/改号前自检：按用例名或号查占用（GET 只读）。"""
    if a.用例名:
        查询 = urllib.parse.urlencode({"name": a.用例名})
        码, r = 调服务("GET", "/api/case_numbers?" + 查询)
        if 码 != 200:
            print(f"[失败] {_读失败文案('看板服务', 码, r)}")
            return 1
        们 = r.get("命中们", [])
        if not 们:
            print(f"[查用例号] 「{a.用例名}」未登记——可领号：board_cli 领用例号 「{a.用例名}」")
            return 0
        print(f"[查用例号] 「{a.用例名}」已登记 {len(们)} 行：")
        for x in 们:
            print(f"  #{x['号']} 「{x.get('用例名') or '（未配名）'}」 {x.get('时刻', '')} {x.get('机器', '')}")
        print("  同名字=已占用——勿重复建用例目录；改号走 git mv + 覆盖矩阵登记名同步")
        return 0
    码, r = 调服务("GET", "/api/case_numbers")
    if 码 != 200:
        print(f"[失败] {_读失败文案('看板服务', 码, r)}")
        return 1
    们 = r.get("台账", [])
    配名数 = sum(1 for x in 们 if x.get("用例名"))
    print(f"[查用例号] 台账 {len(们)} 行·已配名 {配名数}·未配名 {len(们) - 配名数}（最近 15）：")
    for x in 们[-15:]:
        print(f"  #{x['号']} 「{x.get('用例名') or '（未配名）'}」 {x.get('时刻', '')}")
    if len(们) > 15:
        print(f"  …（前 {len(们) - 15} 行略·全量见 /api/case_numbers）")
    return 0


def cmd_补用例名(a) -> int:
    """已发放存量号补配用例名（811）——744 编号权威收权下「已领号未配名」黄提示的
    补录正路：只对台账「已发放且用例名空」的号配名（不发新号·不违 744）；台账无此号
    404、已配名 409 拒改（防篡改）、名被占 409（号名配对唯一·577 同判据）。
    服务不可达硬拒不降级（与领号同口径——台账唯一权威·宁停不裂）。"""
    号 = (a.号 or "").strip()
    用例名 = (a.用例名 or "").strip()
    if not 号 or not 用例名:
        print("[失败] 号与用例名必填——用法：补用例名 <号> <用例名>"
              "（只对台账已发放未配名号；新用例请走 领用例号 <用例名>）")
        return 2
    体 = {"号": 号, "用例名": 用例名, "机器": 机器名()}
    try:
        码, r = 调服务("POST", "/api/case_pair", 体)
    except OSError as e:
        print(f"[失败] 看板服务不可达（{e}）——补名硬拒不降级（恢复网络/核对 "
              f"CN_BOARD_URL 后重试）")
        return 1
    if 码 == 404:
        print(f"[失败] {r.get('错误', '')}")
        return 1
    if 码 == 409:
        print(f"[拒配] {r.get('错误', '')}")
        return 1
    if 码 != 200:
        print(f"[失败] {_写失败文案('看板服务', 码, r)}——补名硬拒不降级")
        return 1
    print(f"[补用例名] #{号} ← 「{用例名}」已锁定（号名配对唯一·台账真值）")
    print("  建目录/改名同步：tests/e2e 对应目录与覆盖矩阵登记名按 README 改号流程核对")
    return 0


def _读文件(路径: str) -> str:
    return Path(路径).read_text(encoding="utf-8")


def cmd_裁决(a) -> int:
    """裁决项上传（403）——--批 批量文件 或 单件四件套。已裁决项 409=跳过不算失败。"""
    if a.批:
        件们 = json.loads(_读文件(a.批))
        if not isinstance(件们, list):
            件们 = [件们]
    elif a.号 and a.标题 and a.讲解文件 and a.选项文件:
        件们 = [{"号": a.号, "标题": a.标题, "优先级": a.优先级,
                 "讲解": _读文件(a.讲解文件),
                 "选项们": json.loads(_读文件(a.选项文件))}]
    else:
        print("[失败] 须 --批 <件们.json> 或 --号/--标题/--讲解文件/--选项文件 四件套")
        return 2
    登记 = 跳过 = 败 = 0
    for 件 in 件们:
        码, r = 调服务("POST", "/api/adjudication_add", {**件, "来源": "board_cli"})
        if 码 in (200, 201):
            登记 += 1
            print(f"[裁决] #{件.get('号')} {r.get('动作', '已登记')}")
        elif 码 == 409:
            跳过 += 1
            print(f"[裁决] #{件.get('号')} 跳过（已裁决锁定）")
        else:
            败 += 1
            print(f"[失败] #{件.get('号')} {_写失败文案('看板服务', 码, r)}")
    print(f"[裁决] 登记 {登记}·跳过 {跳过}·失败 {败}")
    return 1 if 败 else 0


def cmd_裁决删(a) -> int:
    """删除待裁决件（471·误登记清理通道）——走 /api/adjudication_delete 同一接口。
    仅「待裁决」态可删：已裁决=终态留痕 409 禁删（改判走再提交翻转）、不存在 404。
    删后落缓存（列表已变·离线降级面读到新真相）。"""
    码, r = 调服务("POST", "/api/adjudication_delete", {"号": a.号})
    if 码 == 200:
        print(f"[裁决删] {r.get('动作', '已删除')}")
        try:    # 删后刷新缓存（列表已变）——失败不掩盖删除成功；非 200 不落缓存
            码2, r2 = 调服务("GET", "/api/adjudications")
            if 码2 == 200:
                落缓存("adjudications", r2)
        except OSError:
            pass
        return 0
    print(f"[失败] {_写失败文案('看板服务', 码, r)}")
    return 1


def cmd_裁决看(a) -> int:
    码, r = 调服务("GET", "/api/adjudications")
    if 码 != 200:
        print(f"[失败] {_读失败文案('看板服务', 码, r)}")
        return 1
    落缓存("adjudications", r)
    们 = r["裁决项们"]
    if not a.全部:
        们 = [x for x in 们 if x["状态"] == "待裁决"]
    print(f"裁决项 {len(们)} 件"
          + ("" if a.全部 else "（待裁决·--全部 看含已决）"))
    for x in 们:
        推荐 = next((o["键"] for o in x["选项们"] if o.get("推荐")), "")
        尾 = (f"→ 批{x['最新结论']}" if x["状态"] == "已裁决"
              else f"推荐={推荐 or '—'}")
        print(f"  #{x['号']} [{x['优先级']}] {x['状态']} {x['标题'][:40]} {尾}")
    return 0


def main() -> int:
    # 492 路过即心跳扩源：收工/教训/用例=AI 在干活的真实信号（daemon 线程·静默不阻断）
    try:
        import intent as _看板心跳
        _看板心跳.挂载路过续约()
    except Exception:
        pass
    import argparse
    p = argparse.ArgumentParser(description="文档资源统一 CLI（384）")
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("收工", help="交接追加+会话注销（收工一条命令）")
    s.add_argument("--行", required=True)
    s.add_argument("--机器", default="")
    s.set_defaults(f=cmd_收工)

    s = sub.add_parser("交接", help="只写交接")
    s.add_argument("--行", required=True)
    s.add_argument("--机器", default="")
    s.set_defaults(f=cmd_交接)

    s = sub.add_parser("教训", help="登记教训")
    s.add_argument("--标题", required=True)
    s.add_argument("--正文", default="")
    s.add_argument("--权重", type=int, default=0)
    s.add_argument("--标注", default="活跃")
    s.set_defaults(f=cmd_教训)

    s = sub.add_parser("教训索引", help="一行索引扫读")
    s.add_argument("--高权重", type=int, default=0)
    s.set_defaults(f=cmd_教训索引)

    s = sub.add_parser("教训看", help="看全文")
    s.add_argument("id", type=int)
    s.set_defaults(f=cmd_教训看)

    s = sub.add_parser("用例", help="E2E 用例归入覆盖单元")
    s.add_argument("单元ID")
    s.add_argument("态")
    s.add_argument("用例名")
    s.set_defaults(f=cmd_用例)

    s = sub.add_parser("豁免", help="豁免用例登记")
    s.add_argument("用例名")
    s.add_argument("理由")
    s.set_defaults(f=cmd_豁免)

    s = sub.add_parser("单元删", help="删除覆盖单元行（误登记清洗·247）")
    s.add_argument("单元ID")
    s.set_defaults(f=cmd_单元删)

    s = sub.add_parser("豁免删", help="删除豁免行（撞号留痕/任务级行清洗·247）")
    s.add_argument("用例名")
    s.set_defaults(f=cmd_豁免删)

    s = sub.add_parser("覆盖", help="覆盖矩阵摘要")
    s.set_defaults(f=cmd_覆盖)

    s = sub.add_parser("领用例号", help="E2E 用例编号领取（只报用例名·服务端顺序发放·744 编号权威收权）")
    s.add_argument("用例名", nargs="?", default="",
                   help="用例主题名（必填·建用例目录前领号·号↔名绑定入库·勿复用他人号）")
    s.add_argument("--说明", default="")
    s.set_defaults(f=cmd_领用例号)

    s = sub.add_parser("查用例号", help="用例号台账查询（建目录前/改号前自检·577）")
    s.add_argument("用例名", nargs="?", default="", help="按用例名子串查；省略=看台账最近 15 行")
    s.set_defaults(f=cmd_查用例号)

    s = sub.add_parser("补用例名", help="已发放存量号补配用例名（811·只对未配名号·已配名不可改）")
    s.add_argument("号", help="台账已发放未配名的用例号（核验黄提示「已领号未配名」的号）")
    s.add_argument("用例名", help="用例主题名（配对即锁定·号名绑定唯一）")
    s.set_defaults(f=cmd_补用例名)

    s = sub.add_parser("裁决", help="裁决项上传（AI 侧·批量或单件）")
    s.add_argument("--批", default="")
    s.add_argument("--号", default="")
    s.add_argument("--标题", default="")
    s.add_argument("--优先级", default="P2")
    s.add_argument("--讲解文件", default="")
    s.add_argument("--选项文件", default="")
    s.set_defaults(f=cmd_裁决)

    s = sub.add_parser("裁决看", help="裁决项一览")
    s.add_argument("--全部", action="store_true")
    s.set_defaults(f=cmd_裁决看)

    s = sub.add_parser("裁决删", help="删除待裁决件（误登记清理·已裁决终态禁删）")
    s.add_argument("号")
    s.set_defaults(f=cmd_裁决删)

    a = p.parse_args()
    try:
        return a.f(a)
    except OSError as e:
        print(f"[失败] 看板服务不可达（{e}）——写操作宁停不撞，恢复后重试")
        return 1


if __name__ == "__main__":
    sys.exit(main())
