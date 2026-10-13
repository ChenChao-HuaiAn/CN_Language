#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""atomic_json.py——看板系共享缓存 JSON 读写（556·四写者归纳零复制·纯函数零状态）。

治 board_cache.json（standup/board_cli/decision_brief 三写者共写）与
coverage_cache.json（check_spec_coverage）的半写损坏/并发丢更新：写=同目录
临时文件（pid 后缀）写全后 os.replace 一步替换——读者永远见到完整旧版或完整
新版；读=json 失败返回 None+黄字（不自动删坏缓存·留证待人工·降级由调用方定）。

用法：读(路径)=缺/坏返回 None；写(路径,数据)=原子整体写；合并写(路径,键值)=读旧合并原子写回。
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path


def 读(路径):
    """读 JSON；不存在返回 None（正常缺省·静默），损坏返回 None+黄字。"""
    try:
        return json.loads(Path(路径).read_text(encoding="utf-8"))
    except FileNotFoundError:
        return None
    except (OSError, ValueError) as e:
        print(f"[黄] 缓存 {Path(路径).name} 损坏不可读（{e}）——降级忽略（文件保留待人工查看）",
              file=sys.stderr)
        return None


def 写(路径, 数据) -> bool:
    """原子写 JSON：临时文件（pid 后缀）+os.replace 同目录一步替换。"""
    p = Path(路径)
    临时 = p.with_name(f"{p.name}.{os.getpid()}.tmp")
    try:
        临时.write_text(json.dumps(数据, ensure_ascii=False, indent=1), encoding="utf-8")
        os.replace(临时, p)
        return True
    except OSError as e:
        print(f"[黄] 缓存 {p.name} 写入失败（{e}）——跳过（缓存尽力而为·不阻断主流程）",
              file=sys.stderr)
        try:
            临时.unlink()
        except OSError:
            pass
        return False


def 合并写(路径, 键值: dict) -> bool:
    """读旧→合并新键→原子写回（多写者共写一文件的更新面）。坏旧文件不阻写
    （读侧容错后从 {} 起·本次成功写回即自愈）；读旧与写回间竞窗仍可能丢并发
    更新（写者频率极低·半写损坏已根治·加锁代价不值）。"""
    数据 = 读(路径)
    if not isinstance(数据, dict):
        数据 = {}
    数据.update(键值)
    return 写(路径, 数据)
