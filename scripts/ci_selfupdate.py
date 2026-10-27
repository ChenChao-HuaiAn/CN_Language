"""#1706 daemon 自升级——常驻进程自身代码指纹检测+空闲点退出（守护拉起）。

背景：daemon 是常驻进程，启动加载代码后与磁盘脱钩——develop 新顶含 daemon
自身改动（ci_daemon/ci_shard/queue_* 等闭包文件）时，树头对齐（#1460）只更新
磁盘，运行中进程仍是旧逻辑，须重启才生效（此前=逐机手动重启·028 §七/§九，
「daemon 重启=加载工作树代码」）。

机制（用户裁决 2026-10-27·候选①+每轮空闲自动检测）：启动时定格本进程已加载
的 scripts/ 闭包文件集并算聚合指纹；主循环空闲点（持门禁锁·无任务在跑·develop
池模式，与 #1460 树头对齐同块同点）重算比对，变化即放锁优雅退出（rc=3）——由
既有守护拉起加载新代码：win=CN-CI-winpool-keepalive 计划任务（≤15min），
linux（WSL/arm64/深度机/TX_02）=systemd Restart=always（秒级）。零协议改动、
零新增部署面；新进程报活自然带新指纹（面板版本列/⚠旧版标记随之归位）。

防循环：新代码启动即崩由守护层天然限速兜住（win 15min/次探活；systemd
RestartSec+默认起始率限制），本模块不另设退避。
"""
from __future__ import annotations
import hashlib
import sys
from pathlib import Path

import ci_shard


def 常驻闭包文件(仓库根: Path) -> list[Path]:
    """进程已加载且落在本仓库 scripts/ 下的模块文件集（启动时调用一次定格）。
    动态枚举 sys.modules 而非硬编码清单——daemon 新增依赖时指纹面自动跟随，
    不存在「清单漏登记=改动不被检测」的静默缺口；__main__（ci_daemon.py 自身）
    一并覆盖。返回按路径排序的清单（聚合哈希的稳定序）。"""
    目录 = (仓库根 / "scripts").resolve()
    文件们: set[Path] = set()
    for 模块 in tuple(sys.modules.values()):
        路径 = getattr(模块, "__file__", None)
        if not 路径:
            continue
        p = Path(路径).resolve()
        if p.parent == 目录 and p.suffix == ".py":
            文件们.add(p)
    if not 文件们:   # __main__（ci_daemon.py）必在——空集=调用时机异常，显式失败不静默
        raise RuntimeError("scripts/ 闭包枚举为空（调用时机异常·须在模块加载后调用）")
    return sorted(文件们)


def 闭包指纹(文件们: list[Path]) -> str:
    """聚合指纹：逐文件 CRLF→LF 归一后 sha256，连同文件名再聚合一层（归一口径=
    ci_shard.实例指纹 同款·#1418 防 win/linux 行尾假差；文件名参与防「两文件
    内容互换」碰撞）。任一文件缺失/读失败=RuntimeError 向上抛（调用方告警下轮
    重试——树被外部动过属显式异常，不静默当「未变化」）。"""
    聚合 = hashlib.sha256()
    for p in 文件们:
        try:
            内容 = p.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n")
        except OSError as e:
            raise RuntimeError("闭包文件读失败 %s：%s" % (p.name, e)) from e
        单件 = hashlib.sha256(内容.encode("utf-8")).hexdigest()
        聚合.update(("%s\x00%s\n" % (p.name, 单件)).encode("utf-8"))
    return 聚合.hexdigest()[:16]


def 定格闭包(仓库根: Path) -> tuple[list[Path], str]:
    """启动时一次定格：常驻闭包文件集+聚合指纹（此后空闲点检测的基准——
    文件集固定只比内容，避免运行期枚举集漂移造成假阴/假阳）。"""
    文件们 = 常驻闭包文件(仓库根)
    return 文件们, 闭包指纹(文件们)


def 空闲维护(刷新状态: dict, 仓库根: Path, 远端: str, 分支: str,
             文件们: list[Path], 启动指纹: str) -> bool:
    """空闲点维护（ci_daemon.主 池模式空闲点调用·持门禁锁·无任务在跑）：
    ①树头对齐广播消费（#1460 原位迁入——执行中只记戳，跑完回到本点自动跟随）；
    ②daemon 自身代码指纹比对（#1706）——变化返回 True（调用方放锁退出 rc=3
    待守护拉起；对齐在前=先拉新代码再检测，同一空闲点一次完成升级闭环）。
    RuntimeError（fetch 失败/闭包读失败）向上抛——调用方告警下轮重试。"""
    新头 = ci_shard.处理刷新请求(刷新状态, 仓库根, 远端, 分支)
    if 新头:
        print("[刷新] 实例树头已对齐 %s@%s（服务端更新实例广播）"
              % (分支, 新头[:10]), flush=True)
    if 闭包指纹(文件们) != 启动指纹:
        print("[自升级] daemon 自身代码已更新（磁盘≠进程加载版）——"
              "退出待守护拉起（#1706）", flush=True)
        return True
    return False
