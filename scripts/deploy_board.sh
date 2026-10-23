#!/usr/bin/env bash
# CN 任务看板一键部署（382·照 deploy_website.sh 模式）
# 部署面：board_service.py + board_selftest.py(405) + board_tasks.py + board_docs.py(384)
#         + board_adjudication.py(403) + web_auth.py(1425 登录墙公共模块) + board_www/
#         静态目录 → TX_01:/home/ubuntu
# 用法: bash scripts/deploy_board.sh [ssh别名]   默认 TX_01
# 注意：本脚本只更代码+重启服务（SQLite 数据与 systemd 单元不动）；
#       首次部署/021 迁移另行执行（见 plans/028 §看板）：
#         python3 board_tasks.py --db ~/board_state.db --migrate --021 <主表.md> [--归档 <归档.md>]*
# ★1425 全站登录墙（2026-10-23 用户令）：systemd 单元须注入网页凭据（一次性·未配置=
#   网页登录全拒 fail-closed）：
#     sudo systemctl edit cn-board  →  [Service] Environment=CN_WEB_USER=<用户名>
#                                           Environment=CN_WEB_PASS=<密码>
#   （机器通道 CN_BOARD_TOKEN 照旧·CLI/脚本/daemon 不受影响；凭据不入 git）
# ★部署后必做（728m·教训 155「HTTP 200 ≠ 页面活着」——一次 JS 语法错误
#   整页瘫死，回环 200 探测全过、页面停「连接中…」四小时无人察觉）：
#   ① 本机跑 python3 scripts/infra_healthcheck.py          # 前端真 JS 冒烟（quickjs）
#   ② 人工浏览器开一次页面确认登录墙/渲染/样式/交互（人眼终审·与①互补）
set -euo pipefail

REMOTE="${1:-TX_01}"
REMOTE_DIR="/home/ubuntu"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

for f in board_service.py board_tasks.py board_ci.py board_docs.py board_adjudication.py board_selftest.py web_auth.py; do
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
    "$SCRIPT_DIR/board_ci.py" \
    "$SCRIPT_DIR/board_docs.py" "$SCRIPT_DIR/board_adjudication.py" \
    "$SCRIPT_DIR/board_selftest.py" "$SCRIPT_DIR/web_auth.py" "$REMOTE:$REMOTE_DIR/"
ssh "$REMOTE" "mkdir -p '$REMOTE_DIR/board_www'"
tar czf - -C "$SCRIPT_DIR/board_www" . | ssh "$REMOTE" "tar xzf - -C '$REMOTE_DIR/board_www'"
scp -q "$DIST_TMP/index.html" "$REMOTE:$REMOTE_DIR/board_www/index.html"
rm -rf "$DIST_TMP"

echo "重启 cn-board 服务 ..."
ssh "$REMOTE" "sudo systemctl restart cn-board"
sleep 1.5

# 1425 登录墙三态验证：页面未登录=登录页 200（含 api/login 标记）·API 无凭证=401·
# Bearer（机器通道·令牌读本树 queue_client.json）=200
TOKEN="$(python3 -c "import json,sys;print(json.load(open(sys.argv[1],encoding='utf-8')).get('令牌',''))" "$SCRIPT_DIR/queue_client.json" 2>/dev/null || true)"
HTTP="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8301/" || true)"
WALL="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:8301/api/tasks" || true)"
AUTH="$(ssh "$REMOTE" "curl -s -o /dev/null -w '%{http_code}' -H 'Authorization: Bearer $TOKEN' http://127.0.0.1:8301/api/tasks" || true)"
LOGIN_MARK="$(ssh "$REMOTE" "curl -s http://127.0.0.1:8301/ | grep -c 'api/login'" || true)"
if [ "$HTTP" = "200" ] && [ "$WALL" = "401" ] && [ "$AUTH" = "200" ] && [ "${LOGIN_MARK:-0}" != "0" ]; then
    echo "✓ 已部署并验证（1425 登录墙三态：未登录页面=登录页 200·API 无凭证 401·Bearer 放行 200）"
    echo "  但 200≠页面活着（教训 155）！接着跑:"
    echo "    python3 scripts/infra_healthcheck.py   # 前端真 JS 冒烟（语法+数据链）"
    echo "  并人工浏览器开一次 http://124.222.106.84:8301/ 确认登录墙+渲染/样式/交互"
    echo "  （https://www.cn-language.com/board/ 反代链同验）。"
else
    echo "✗ 部署后验证异常（页面 ${HTTP:-无}·墙 ${WALL:-无}·鉴权 ${AUTH:-无}·登录页标记 ${LOGIN_MARK:-无}）" >&2
    echo "  期望：页面 200（登录页）·墙 401·鉴权 200·标记>0——ssh $REMOTE 查 journalctl -u cn-board -n 30" >&2
    exit 1
fi
