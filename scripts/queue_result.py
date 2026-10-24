#!/usr/bin/env python3
# 预验任务结果解析面（1297 自 queue_service.py 拆出·原样搬移+参数化注入）：
#   975 认领失败分类/连续计数告警 + 825 完成红首因提取——全部为 ci_daemon 结果
#   JSON 的纯解析函数（零 HTTP/零路由依赖），拆出动机=queue_service 加分片协议
#   时守 900 行警戒线（AGENTS §3「先拆再加」·973 外迁 ci_build 同款手法）。
# 依赖方向：queue_service → 本模块（单向·本模块禁 import queue_service）。
# con/时戳函数 由调用方注入（写锁内单线程使用·与原语义一致）。

from __future__ import annotations   # P1-1：3.8 注解惰性化（list/tuple 泛型定义期不求值）

import re
import os

# 975 预验失败计数告警（87 篇反思①·863 五连静默机械化）：runner 认领失败（同步异常）
#   连续同类达此阈值 → journal 播报 `[975 预验失败告警]` + 行上告警时刻（页面可见）。
#   下限钳 1=参数校验（0/负值非法——防告警节拍取整除零崩任务完成路径；语义=每次告警）。
预验失败告警阈值 = max(1, int(os.environ.get("CN_PREFAIL_ALERT_N", "3")))


def 认领失败分类(结果) -> tuple[str, str]:
    """975（87 篇反思①·863 五连静默机械化）：认领阶段基础设施失败分类+原因摘要——
    数据源=ci_daemon task_complete 结果 JSON（runner 侧零协议新增：失败形状「步骤.同步.rc
    非零+日志尾部含 stderr」为 863 时代即有的既有形状，服务端解析即可·旧 runner 同得覆盖）。

    类别枚举（单一事实源·#901 fetch 删除时机残余 / #574 solo 静默 同域扩展点）：
    当前唯一类别=「同步失败」——认领后 git fetch 预验分支失败（分支已被删除=863 病灶）
    或轮内 fetch/reset/clean 异常；静默特征=完成红→重入队→再认领再失败（五连
    746/764/758/644/728f 全链无告警面·全靠人盯队列）。构建/单测/e2e 红（同步 rc=0）
    属真实验证结论，不属基础设施失败=不分类（否则真实红会污染告警语义——真红走
    既有「预检红·返工」红治理面）。

    前向兼容：结果.失败类别 显式字段优先（runner 将来显式声明新类别时零服务端改动）；
    缺省回退按上面隐式形状分类。返回 (类别, 摘要)；非认领失败=("", "")。"""
    if not isinstance(结果, dict):
        return "", ""
    显式 = str(结果.get("失败类别") or "").strip()
    if 显式:
        return 显式, str(结果.get("失败摘要") or "").strip()[:200]
    同步 = (结果.get("步骤") or {}).get("同步")
    rc = 同步.get("rc") if isinstance(同步, dict) else None
    if not isinstance(rc, int) or rc == 0:
        return "", ""
    尾部 = [str(行).strip() for 行 in (结果.get("日志尾部") or []) if str(行).strip()]
    摘要 = "；".join(尾部)[:200] or ("同步段 rc=%s" % rc)
    return "同步失败", 摘要


def 红用例名们(结果) -> list[str]:
    """825 残余 A：真实红失败用例名提取——①结果.e2e明细.失败用例[].名称（842 主源·
    run_e2e --json 明细随轮上报）；②退化扫描 结果.日志尾部「✗ 名:」形态行（旧版
    runner 无 e2e明细·与 integrate.py 解析预验红用例们 同形）。保序去重；
    miss/无受控取值一律空列表（调用点以其为空与否显式分流，不静默冒名）。"""
    if not isinstance(结果, dict):
        return []
    出: list[str] = []
    明细 = 结果.get("e2e明细")
    if isinstance(明细, dict) and isinstance(明细.get("失败用例"), list):
        for 例 in 明细["失败用例"]:
            if isinstance(例, dict) and str(例.get("名称") or "").strip():
                出.append(str(例["名称"]).strip())
    if 出:
        return 出
    for 行 in 结果.get("日志尾部") or []:
        m = re.search(r"✗\s+(\S+?)[：:]", str(行))
        if m and m.group(1) not in 出:
            出.append(m.group(1))
    return 出


def 用例名串(用例们: list[str]) -> str:
    """825：失败用例清单缩写——前 3 名顿号连列·超 3 附「等」（真实红首因两分支共用）。"""
    return "、".join(用例们[:3]) + (" 等" if len(用例们) > 3 else "")


def 真实红首因(结果) -> str:
    """825 残余 A（825 备注①剩余·失败原因显形最后一段）：完成红首因摘要——
    红步骤名+失败用例名，落「失败原因」列（快照/详情/页面积木三面流出）。

    口径：①按步骤执行序（结果.步骤 保序）取首个 rc 非零/超时步骤为首因；e2e 红
    带用例清单（e2e明细 主源·日志尾部 ✗ 形态兜底·最多列 3 名）；②红但无可辨红
    步骤（结果形态异常/旧 runner）=显式文本（miss 不静默·648——不编不藏）；
    ③本字段对全部完成红统一落文本（含 975 认领失败=同步 rc 非零——首因即同步；
    与 失败类别/失败摘要 的连续计数告警面互不替代，各司其职）。"""
    if not isinstance(结果, dict):
        return "未识别红步骤（结果非对象）"
    步骤 = 结果.get("步骤")
    if not isinstance(步骤, dict) or not 步骤:
        return "未识别红步骤（结果缺步骤字段）"
    用例们 = 红用例名们(结果)
    for 名, 步 in 步骤.items():
        if not isinstance(步, dict):
            continue
        rc = 步.get("rc")
        if (isinstance(rc, int) and rc != 0) or 步.get("超时"):
            if str(名) == "e2e" and 用例们:
                return "e2e：失败 %d 例（%s）" % (len(用例们), 用例名串(用例们))
            if str(名) == "M6平台保障":
                # #1221：平台不可验与代码红分流（红治理按类别归因·不复用编译器缺陷口径）
                return "M6平台保障：%s" % (str(步.get("说明") or "平台不可验")[:200])
            return "%s（rc=%s%s）" % (名, rc, "·超时" if 步.get("超时") else "")
    if 用例们:   # 红在聚合面外/步骤 rc 形态缺失——用例清单仍为首因证据
        return "e2e：失败 %d 例（%s）" % (len(用例们), 用例名串(用例们))
    return "未识别红步骤（步骤无 rc 非零项）"


def 记认领失败并告警(con, 时戳, 分支: str, 类别: str, 摘要: str) -> str:
    """975：认领失败计数与阈值告警（task_complete 内调用·须在写锁内）——返回告警行
    文本（空=未告警；调用方 print 进服务端 journal·`[975 预验失败告警]` 前缀可 grep）。

    计数语义=「连续同类」：类别与上次不同则计数重置为 1；类别为空（真实验证绿/红=
    基础设施面恢复证据）→ 连续链清零（告警时刻保留可审计）。重跑（task_enqueue
    完成红→重置排队）不清计数=同 sha 跨重跑累计（五连回环正由此暴露）。
    告警节拍=计数每跨过 阈值 的整数倍一次（持续失败持续可见·防刷屏）。
    幂等：complete 的状态机校验（状态=执行中∧认领者匹配）使 runner 重试/被回收旧
    runner 复活天然不重复计数。con/时戳 由调用方注入（本模块零全局连接）。"""
    if not 类别:
        con.execute("UPDATE 预验任务 SET 失败计数=0,失败类别=NULL,失败摘要=NULL WHERE 分支=?",
                    (分支,))
        return ""
    行 = con.execute("SELECT 失败计数,失败类别,sha FROM 预验任务 WHERE 分支=?", (分支,)).fetchone()
    # 行必在（调用点 task_complete 刚以 分支∧认领者∧执行中 校验 UPDATE rowcount>0·同写锁内）——
    # 三元兜底=穷举声明不可达③（防 None 解包崩），非语义默认（miss 处置纪律）。
    旧计数, 旧类别, sha = (int(行[0] or 0), 行[1], str(行[2] or "")) if 行 else (0, None, "")
    新计数 = 旧计数 + 1 if 旧类别 == 类别 else 1
    con.execute("UPDATE 预验任务 SET 失败计数=?,失败类别=?,失败摘要=? WHERE 分支=?",
                (新计数, 类别, (摘要 or "")[:200], 分支))
    if (新计数 < 预验失败告警阈值
            or 新计数 // 预验失败告警阈值 <= (新计数 - 1) // 预验失败告警阈值):
        return ""
    con.execute("UPDATE 预验任务 SET 告警时刻=? WHERE 分支=?", (时戳(), 分支))
    return ("[975 预验失败告警] %s sha=%s 连续同类认领失败 %d 次（阈值 %d）：类别=%s 原因=%s"
            % (分支, sha[:10], 新计数, 预验失败告警阈值, 类别, (摘要 or "")[:200]))
