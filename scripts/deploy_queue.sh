#!/usr/bin/env bash
# CN 门禁窗口（队列服务 8300）一键部署（842·照 deploy_board.sh 模式）
# 部署面：queue_service.py + queue_selftest.py + queue_result.py + queue_shard.py（1297 分片协议+结果解析面）+ queue_instances.py（1392 实例动态面）+ queue_www/（index/queue.css/queue.js）
#         → TX_01:/home/ubuntu；tokens.css 由 board_www 拷入生成（tokens 同源单份·
#         队列部署面独立不依赖 board_www 在场）。
# 用法: bash scripts/deploy_queue.sh [ssh别名]   默认 TX_01
# 注意：本脚本只更代码+重启服务（SQLite 数据与 systemd 单元不动）。
set -euo pipefail

REMOTE="${1:-TX_01}"
REMOTE_DIR="/home/ubuntu"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

for f in queue_service.py queue_selftest.py queue_result.py queue_shard.py queue_instances.py; do
    if [ ! -f "$SCRIPT_DIR/$f" ]; then
        echo "错误: 找不到 $SCRIPT_DIR/$f" >&2
        exit 1
    fi
done
for f in index.html queue.css queue.js; do
    if [ ! -f "$SCRIPT_DIR/queue_www/$f" ]; then
        echo "错误: 找不到 $SCRIPT_DIR/queue_www/$f" >&2
        exit 1
    fi
done
if [ ! -f "$SCRIPT_DIR/board_www/tokens.css" ]; then
    echo "错误: 找不到 $SCRIPT_DIR/board_www/tokens.css（墨韵 token 同源单份）" >&2
    exit 1
fi

echo "部署门禁窗口（队列服务） → $REMOTE:$REMOTE_DIR ..."
# 内联打包：tokens.css+queue.css+queue.js 直嵌 index.html（一次 HTTP 请求=完整页面·
#   弱网/重启窗口不出现「HTML 到了样式丢了」；queue_www/ 源三件照常上传供独立路由）
DIST_TMP="$(mktemp -d)"
python3 - "$SCRIPT_DIR" "$DIST_TMP" <<'PY'
import pathlib
import sys

脚本, dst = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
www = 脚本 / "queue_www"
html = (www / "index.html").read_text(encoding="utf-8")
css = ((脚本 / "board_www" / "tokens.css").read_text(encoding="utf-8") + "\n"
       + (www / "queue.css").read_text(encoding="utf-8"))
js = (www / "queue.js").read_text(encoding="utf-8")
html = html.replace('<link rel="stylesheet" href="tokens.css">',
                    "<style>\n" + css + "\n</style>")
html = html.replace('<link rel="stylesheet" href="queue.css">', "")
html = html.replace('<script src="queue.js"></script>',
                    "<script>\n" + js + "\n</script>")
(dst / "index.html").write_text(html, encoding="utf-8")
PY
scp -q "$SCRIPT_DIR/queue_service.py" "$SCRIPT_DIR/queue_selftest.py" "$SCRIPT_DIR/queue_result.py" "$SCRIPT_DIR/queue_shard.py" "$SCRIPT_DIR/queue_instances.py" "$REMOTE:$REMOTE_DIR/"
ssh "$REMOTE" "mkdir -p '$REMOTE_DIR/queue_www'"
tar czf - -C "$SCRIPT_DIR/queue_www" . | ssh "$REMOTE" "tar xzf - -C '$REMOTE_DIR/queue_www'"
scp -q "$SCRIPT_DIR/board_www/tokens.css" "$REMOTE:$REMOTE_DIR/queue_www/tokens.css"
scp -q "$DIST_TMP/index.html" "$REMOTE:$REMOTE_DIR/queue_www/index.html"
rm -rf "$DIST_TMP"

echo "重启 cn-queue 服务 ..."
ssh "$REMOTE" "sudo systemctl restart cn-queue"
sleep 1.5

HTTP="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8300/" || true)"
STATE="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8300/api/state" || true)"
if [ "$HTTP" = "200" ] && [ "$STATE" = "200" ]; then
    echo "✓ 已部署并验证（页面 200 + /api/state 200）—"
    echo "  直连 http://124.222.106.84:8300/ · 网页 https://www.cn-language.com/queue/"
else
    echo "✗ 部署后本机回环探测异常（页面 ${HTTP:-无响应}·state ${STATE:-无响应}）——ssh $REMOTE 查 journalctl -u cn-queue -n 30" >&2
    exit 1
fi
