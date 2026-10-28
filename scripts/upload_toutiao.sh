#!/usr/bin/env bash
# 今日头条栏目一键生成+上传（win/linux 皆可跑）
# 用法: bash scripts/upload_toutiao.sh [ssh别名]   默认 TX_01
#
# 流程：website/tools/gen_toutiao.py 扫描头条草稿目录重新生成 website/toutiao/
#（列表页 + 去蓝框文章页）→ tar over ssh 上传到 TX_01:/var/www/cn-language/toutiao。
# 该目录不在 git 树内（.gitignore 排除）——website_auto_deploy.py 发布时会
# 原样保留现网 toutiao/，deploy_website.sh 整树覆盖时也会先挪走再放回。
#
# 草稿源目录在生成器里默认指向 C:\Users\ChenChao\Documents\cn-头条文章草稿，
# 其他机器用 --src 覆盖（直接跑 python website/tools/gen_toutiao.py --src <目录>）。
set -euo pipefail

REMOTE="${1:-TX_01}"
REMOTE_DIR="/var/www/cn-language"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEBSITE_DIR="$SCRIPT_DIR/../website"

if [ ! -f "$WEBSITE_DIR/tools/gen_toutiao.py" ]; then
    echo "错误: 找不到 $WEBSITE_DIR/tools/gen_toutiao.py" >&2
    exit 1
fi

echo "生成今日头条栏目 ..."
python "$WEBSITE_DIR/tools/gen_toutiao.py" --out "$WEBSITE_DIR/toutiao"

if [ ! -f "$WEBSITE_DIR/toutiao/index.html" ]; then
    echo "错误: 生成失败，$WEBSITE_DIR/toutiao/index.html 不存在" >&2
    exit 1
fi

echo "上传 $WEBSITE_DIR/toutiao → $REMOTE:$REMOTE_DIR/toutiao ..."
ssh "$REMOTE" "sudo mkdir -p '$REMOTE_DIR/toutiao' \
  && sudo rm -rf '$REMOTE_DIR/toutiao.new' \
  && sudo mkdir -p '$REMOTE_DIR/toutiao.new'"
tar czf - -C "$WEBSITE_DIR" toutiao | ssh "$REMOTE" "sudo tar xzf - -C '$REMOTE_DIR/toutiao.new' --strip-components=1"
# 暂存目录换名原子切换（失败保留旧版）
ssh "$REMOTE" "if sudo mv '$REMOTE_DIR/toutiao' '$REMOTE_DIR/toutiao.old'; then
    sudo mv '$REMOTE_DIR/toutiao.new' '$REMOTE_DIR/toutiao' \
      && sudo rm -rf '$REMOTE_DIR/toutiao.old' \
      || { sudo mv '$REMOTE_DIR/toutiao.old' '$REMOTE_DIR/toutiao'; exit 1; }
  else
    sudo mv '$REMOTE_DIR/toutiao.new' '$REMOTE_DIR/toutiao'
  fi"
echo "✓ 已上传。远端文章数: $(ssh "$REMOTE" "sudo find '$REMOTE_DIR/toutiao/articles' -type f | wc -l")"
