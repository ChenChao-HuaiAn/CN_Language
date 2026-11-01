#!/usr/bin/env bash
# 高冲突单点 JSON merge driver 每机一次性安装（#1259 立·与 githooks core.hooksPath 同级配置）
#
# 口径（#1942 定稿·两个 driver 共用 scripts/merge_struct_union.py 内核）：
#   ①结构级并集：按 JSON 结构（对象按键 / 数组按元素身份）三路并集求并，不按文本行
#     ——结构里没有「位置」，两侧在同一锚点后各自追加也不会互相吞掉。
#   ②丢行禁令：两侧新增全部保留、同身份元素不重复；任一顶层键/数组桶都走同一规则
#     （旧实现只并集「特性清单」、其余键取 ours，rebase 方向 ours=上游 develop，
#     分支侧新增整段被吞且退出码 0 → L1 check_doc_coverage 报未归类 → integrate 红）。
#   ③fail-safe：任一侧解析失败/含冲突标记/顶层结构不符/结构类型冲突/删改冲突 →
#     driver 退出非零且不改写 %A，git 落回普通文本冲突交人裁决；绝不产出可能丢数据
#     的合并结果（#1238/#1360 禁静默）。
# 未安装/安装未生效时=回退普通冲突（无害·driver 不被调用）。
set -e
# 解释器探测：win 上 python3 常不存在（仅 python），linux 反之——取首个可执行者。
# 注：shell 变量名须 ASCII（bash 标识符不支持中文）。
PY=""
for cand in python3 python; do
    if "$cand" -c "import sys" >/dev/null 2>&1; then PY="$cand"; break; fi
done
if [ -z "$PY" ]; then
    echo "[merge-drivers] 未找到可用 python 解释器（python3/python 均不可执行）" >&2
    exit 1
fi
git config merge.miss-registry.name  "miss_registry 结构级三路并集合并"
git config merge.miss-registry.driver "$PY scripts/merge_miss_registry.py %O %A %B"
git config merge.doc-coverage.name  "doc_coverage 结构级三路并集合并"
git config merge.doc-coverage.driver "$PY scripts/merge_doc_coverage.py %O %A %B"
echo "[merge-drivers] 已安装：miss-registry + doc-coverage（解释器=$PY·内核=scripts/merge_struct_union.py）"
echo "[merge-drivers] 查验：git config --get-regexp 'merge\.(miss-registry|doc-coverage)'"
echo "[merge-drivers] 自测：python scripts/test_merge_drivers.py（gate_quick 静态段亦跑）"
