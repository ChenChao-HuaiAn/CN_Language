# CN 语言官网 · 部署指引（2026-10-07 已上线版）

> **现状**：官网已上线——`https://www.cn-language.com/`（主站+文档+CN-OS）。
> TX_01（124.222.106.84）+ Caddy TLS 终结，域名/证书/备案链路用户早已配置完成。
> ~~ICP 备案流程~~ 已作废（域名在线即备案链路已通，此前指引基于未验证假设，已勘误）。

## 线上架构

```
                    ┌─ /            → /var/www/cn-language（官网首页）
www.cn-language.com ┼─ /docs/…      → 文档站（16 页）
                    ├─ /os/         → CN-OS 落地页
                    ├─ /toutiao/    → 今日头条栏目（每日文章·独立上传通道，见下）
                    └─ /queue/      → 反代 127.0.0.1:8300（CI 队列/门禁面板）

cn-os.com / www      → 同树；根路径重写为 /os/index.html（CN-OS 独立域名入口）
```

- Caddy 配置：TX_01 `/etc/caddy/Caddyfile`；回滚备份：`/etc/caddy/Caddyfile.bak-20261007`。
- **队列面板地址自 2026-10-07 起为 `https://www.cn-language.com/queue/`**
  （原根路径已让位官网；〔通告〕@全机）。

## cn-os.com 域名启用（仅剩一步·用户操作）

服务器端站点块已配好。差一条 DNS 解析——域名注册商 DNS 控制台添加：

```
cn-os.com       A   124.222.106.84
www.cn-os.com   A   124.222.106.84
```

解析生效后 Caddy 自动为该域名签发 Let's Encrypt 证书（几分钟内），无需人工干预。
**注意**：`cn-language.com` 已备案不代表 `cn-os.com` 已备案（备案按域名计）——若解析生效后
访问被腾讯云「未备案」拦截页阻断，需在腾讯云备案控制台以同一主体为 cn-os.com
做一次「新增网站」（已有主体，流程快）。

## 日常发布

**自动通道（992·推荐）**：TX_01 每 10 分钟跑 `~/website_auto_deploy.py`（源=
`scripts/website_auto_deploy.py`）——浅克隆 develop → 比对已部署 sha → 有变化才在
服务器侧跑 `website/tools/gen_docs.py` 重新生成 HTML（页脚含「文档基线：develop
<short-sha>·<日期>·示例经编译器实测」标注）→ 暂存+换名原子发布 → 同步再生 stats.json。
日志 `TX_01:~/cn_website_deploy.log`（脚本自记·cron 行只回收 stderr 防重复行）；
已部署 sha 记录 `TX_01:~/cn_website_deployed_sha`；
缓存仓 `TX_01:~/cn_website_repo`（与 website_stats 的 cn_stats_repo 分仓，防 git 锁互撞）。

```bash
# 服务器上手跑一轮（幂等：sha 未变直接退出；--verbose 连「无变化」也打印）
ssh TX_01 'python3 /home/ubuntu/website_auto_deploy.py --verbose'
```

**手动通道（改版/应急·本地树原样上传）**：

```bash
bash scripts/deploy_website.sh          # tar over ssh → TX_01:/var/www/cn-language
```

- 两通道写同一目录 /var/www/cn-language，后到者胜；手动发布后自动通道在 develop
  下次前进时照常接续（按 sha 判定，不依赖手动发布记录）。
- 静态资源更新后 bump HTML 里的 `?v=N` 版本号（浏览器启发式缓存会吃无版本号更新）。
- 备案号页脚：cn-language.com 域名的备案号以备案控制台查询为准，确认后替换
  全站 18 个 HTML 页脚的「京ICP备00000000号（备案办理中·占位）」。

## 今日头条栏目（/toutiao/·2026-10-28 上线）

每日任务「今日头条」产出的文章网页（草稿库 `cn-头条文章草稿/第*.html`，
图片 base64 内嵌、约 96MB/233 篇）经 `website/tools/gen_toutiao.py` 转换后
发布到 `/toutiao/`：文章开头「使用说明」蓝色框整块剔除，正文补返回链接与
viewport；栏目列表页按日期分组、新文在前。

**通道与两条发布通道解耦**：文章树不在 git 内（`.gitignore` 排除
`website/toutiao/`），走独立上传——

```bash
bash scripts/upload_toutiao.sh [ssh别名]   # 生成 + tar over ssh 原子上传（默认 TX_01）
python website/tools/gen_toutiao.py --src <草稿目录>   # 其他机器单独生成
```

两条发布通道均保留现网 toutiao/：自动通道 `website_auto_deploy.py` 发布()
在切换前把现网 toutiao/ 搬进暂存目录；手动通道 `deploy_website.sh` 先挪到
`/var/www/cn-language.toutiao_keep` 整树覆盖后放回。**改 auto_deploy 后须
重新 `scp scripts/website_auto_deploy.py TX_01:/home/ubuntu/`**（服务器跑的
是家副本，不随 git 更新）。

## 回滚

- 官网内容：重跑发布脚本即整树覆盖（纯静态）。
- Caddy 配置：`sudo cp /etc/caddy/Caddyfile.bak-20261007 /etc/caddy/Caddyfile && sudo systemctl reload caddy`
  （回滚后队列面板回到根路径、官网下线）。
