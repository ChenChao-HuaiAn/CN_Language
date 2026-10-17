#!/usr/bin/env bash
# CN 语言官网一键发布（纯静态·tar over ssh·win/linux 皆可跑）
# 用法: bash scripts/deploy_website.sh [ssh别名]   默认 TX_01
#
# 与 scripts/website_auto_deploy.py（992·TX_01 每 10 分钟 develop 自动通道）的关系：
#   本脚本=手动整树发布（本地树原样上传，适合改版/应急）；auto_deploy=服务器侧浅克隆
#   develop→重新生成 HTML（页脚带 develop sha 标注）→暂存+换名原子发布。两者写同一
#   目录 /var/www/cn-language，后到者胜；自动通道按已部署 sha（~/cn_website_deployed_sha）
#   幂等——手动发布后 develop 下次前进时自动通道照常接续，互不依赖。
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

# 844：整树覆盖会连 stats.json 一起清掉——发布后同步统计器并立即再生
# （TX_01 crontab 每 6h 随 develop 兜底刷新；失败不阻断发布，日志落 ~/cn_stats.log）
scp -q "$SCRIPT_DIR/website_stats.py" "$REMOTE:/home/ubuntu/website_stats.py"
ssh "$REMOTE" "python3 /home/ubuntu/website_stats.py >> /home/ubuntu/cn_stats.log 2>&1 \
  && echo '✓ 数据条统计已再生（stats.json）' \
  || echo '⚠ stats.json 再生失败——详见 TX_01:~/cn_stats.log（官网仍显静态兜底值）'"
