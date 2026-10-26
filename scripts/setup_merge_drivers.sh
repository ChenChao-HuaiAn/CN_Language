#!/usr/bin/env bash
# 高冲突单点 JSON merge driver 每机一次性安装（#1259·与 githooks core.hooksPath 同级配置）
set -e
git config merge.miss-registry.name  "miss_registry 三路并集合并"
git config merge.miss-registry.driver "python3 scripts/merge_miss_registry.py %O %A %B"
git config merge.doc-coverage.name  "doc_coverage 三路并集合并"
git config merge.doc-coverage.driver "python3 scripts/merge_doc_coverage.py %O %A %B"
echo "[merge-drivers] 已安装：miss-registry + doc-coverage（git config 查验：git config --get-regexp 'merge\.(miss-registry|doc-coverage)'）"
