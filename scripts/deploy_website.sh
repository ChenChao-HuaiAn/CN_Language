#!/usr/bin/env bash
# CN 语言官网一键发布（纯静态·tar over ssh·win/linux 皆可跑）
# 用法: bash scripts/deploy_website.sh [ssh别名]   默认 TX_01
set -euo pipefail

REMOTE="${1:-TX_01}"
REMOTE_DIR="/var/www/cn-language"

# 定位 website/ 目录（脚本在 scripts/ 下，站点在 website/）
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEBSITE_DIR="$SCRIPT_DIR/../website"

if [ ! -f "$WEBSITE_DIR/index.html" ]; then
    echo "错误: 找不到 $WEBSITE_DIR/index.html" >&2
    exit 1
fi

echo "发布 $WEBSITE_DIR → $REMOTE:$REMOTE_DIR ..."
# 远端建目录并清空旧内容，再整树解包（原子性靠最后 mv 更稳，静态站此处从简）
ssh "$REMOTE" "sudo mkdir -p '$REMOTE_DIR' && sudo rm -rf '$REMOTE_DIR'/* "
tar czf - -C "$WEBSITE_DIR" . | ssh "$REMOTE" "sudo tar xzf - -C '$REMOTE_DIR'"
echo "✓ 已发布。远端文件数: $(ssh "$REMOTE" "sudo find '$REMOTE_DIR' -type f | wc -l")"
