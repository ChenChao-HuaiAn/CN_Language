#!/usr/bin/env bash
# CN 任务看板一键部署（382·照 deploy_website.sh 模式）
# 部署面：board_service.py + board_selftest.py(405) + board_tasks.py + board_docs.py(384)
#         + board_adjudication.py(403) + board_www/ 静态目录 → TX_01:/home/ubuntu
# 用法: bash scripts/deploy_board.sh [ssh别名]   默认 TX_01
# 注意：本脚本只更代码+重启服务（SQLite 数据与 systemd 单元不动）；
#       首次部署/021 迁移另行执行（见 plans/028 §看板）：
#         python3 board_tasks.py --db ~/board_state.db --migrate --021 <主表.md> [--归档 <归档.md>]*
set -euo pipefail

REMOTE="${1:-TX_01}"
REMOTE_DIR="/home/ubuntu"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

for f in board_service.py board_tasks.py board_docs.py board_adjudication.py board_selftest.py; do
    if [ ! -f "$SCRIPT_DIR/$f" ]; then
        echo "错误: 找不到 $SCRIPT_DIR/$f" >&2
        exit 1
    fi
done
if [ ! -f "$SCRIPT_DIR/board_www/index.html" ]; then
    echo "错误: 找不到 $SCRIPT_DIR/board_www/index.html" >&2
    exit 1
fi

echo "部署看板服务与前端 → $REMOTE:$REMOTE_DIR ..."
# 内联打包：CSS/JS 直嵌 index.html（一次 HTTP 请求=完整页面——弱网/重启窗口
# 不再出现「HTML 到了样式丢了」；board_www/ 源三件照常上传供独立路由）
DIST_TMP="$(mktemp -d)"
python3 - "$SCRIPT_DIR/board_www" "$DIST_TMP" <<'PY'
import pathlib
import sys

src, dst = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
html = (src / "index.html").read_text(encoding="utf-8")
css = ((src / "tokens.css").read_text(encoding="utf-8") + "\n"
       + (src / "board.css").read_text(encoding="utf-8"))
js = (src / "board.js").read_text(encoding="utf-8")
html = html.replace('<link rel="stylesheet" href="/tokens.css">',
                    "<style>\n" + css + "\n</style>")
html = html.replace('<link rel="stylesheet" href="/board.css">', "")
html = html.replace('<script src="/board.js"></script>',
                    "<script>\n" + js + "\n</script>")
(dst / "index.html").write_text(html, encoding="utf-8")
PY
scp -q "$SCRIPT_DIR/board_service.py" "$SCRIPT_DIR/board_tasks.py" \
    "$SCRIPT_DIR/board_docs.py" "$SCRIPT_DIR/board_adjudication.py" \
    "$SCRIPT_DIR/board_selftest.py" "$REMOTE:$REMOTE_DIR/"
ssh "$REMOTE" "mkdir -p '$REMOTE_DIR/board_www'"
tar czf - -C "$SCRIPT_DIR/board_www" . | ssh "$REMOTE" "tar xzf - -C '$REMOTE_DIR/board_www'"
scp -q "$DIST_TMP/index.html" "$REMOTE:$REMOTE_DIR/board_www/index.html"
rm -rf "$DIST_TMP"

echo "重启 cn-board 服务 ..."
ssh "$REMOTE" "sudo systemctl restart cn-board"
sleep 1.5

HTTP="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8301/" || true)"
if [ "$HTTP" = "200" ]; then
    echo "✓ 已部署并验证（http://124.222.106.84:8301 → 200）"
else
    echo "✗ 部署后本机回环探测异常（HTTP ${HTTP:-无响应}）——ssh $REMOTE 查 journalctl -u cn-board -n 30" >&2
    exit 1
fi
