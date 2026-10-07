# CN 语言官网 · 部署指引（2026-10-07 已上线版）

> **现状**：官网已上线——`https://www.cn-language.com/`（主站+文档+CN-OS）。
> TX_01（124.222.106.84）+ Caddy TLS 终结，域名/证书/备案链路用户早已配置完成。
> ~~ICP 备案流程~~ 已作废（域名在线即备案链路已通，此前指引基于未验证假设，已勘误）。

## 线上架构

```
                    ┌─ /            → /var/www/cn-language（官网首页）
www.cn-language.com ┼─ /docs/…      → 文档站（16 页）
                    ├─ /os/         → CN-OS 落地页
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

```bash
bash scripts/deploy_website.sh          # tar over ssh → TX_01:/var/www/cn-language
```

- 静态资源更新后 bump HTML 里的 `?v=N` 版本号（浏览器启发式缓存会吃无版本号更新）。
- 备案号页脚：cn-language.com 域名的备案号以备案控制台查询为准，确认后替换
  全站 18 个 HTML 页脚的「京ICP备00000000号（备案办理中·占位）」。

## 回滚

- 官网内容：重跑发布脚本即整树覆盖（纯静态）。
- Caddy 配置：`sudo cp /etc/caddy/Caddyfile.bak-20261007 /etc/caddy/Caddyfile && sudo systemctl reload caddy`
  （回滚后队列面板回到根路径、官网下线）。
