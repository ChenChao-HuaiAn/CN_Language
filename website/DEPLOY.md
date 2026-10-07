# CN 语言官网 · 部署与备案指引

> 目标架构：腾讯云 TX_01（Linux）+ Caddy（自动 HTTPS）双域名静态站。
> `cn-language.com` / `www` → CN 语言主站+文档；`cn-os.com` / `www` → CN-OS 落地页。

## 分工总览

| 事项 | 谁做 | 状态 |
|---|---|---|
| 站点制作（website/ 静态站） | AI | ✅ 已完成 |
| TX_01 安装 Caddy + 站点落盘 + 服务配置 | AI（ssh TX_01） | 备案后/预发时执行 |
| 腾讯云控制台：安全组放行 80/443 | **用户** | 待办 |
| ICP 备案（两域名合并办） | **用户** | 待办（1~4 周） |
| 域名解析 A 记录 → TX_01 公网 IP | **用户** | 备案通过后 |
| HTTPS 证书 | Caddy 自动 | 解析生效后自动 |

## 一、用户操作清单（按序）

### 1. 安全组放行（现在就可做）
腾讯云控制台 → 云服务器 TX_01 → 安全组 → 入站规则添加：
- TCP 80（HTTP/备案验证/Caddy ACME）
- TCP 443（HTTPS）

### 2. ICP 备案（核心路径·约 1~4 周）
- 入口：腾讯云控制台 → 备案 → 开始备案（微信小程序「腾讯云网站备案」更快）。
- **两域名可合并为一次网站备案**（同一主体同一服务器）：`cn-language.com` 与 `cn-os.com`。
- 需要准备：
  - 主体信息：个人身份证（或企业营业执照+法人身份证）；
  - 服务号：在备案控制台选 TX_01 实例生成「备案服务号」；
  - 网站信息：网站名称建议「CN 语言官网」「CN-OS 官网」（含「语言/OS」等词一般可过，
    勿带「中国/国家」等词）；网站内容选「其他」或「博客/个人空间」类目按实际填写；
  - 域名证书：域名注册商处可下载（两域名需已完成实名认证，且注册满 3 天）。
- 流程：提交初审（1~2 天）→ 管局审核（各省 1~20 天）→ 短信通知备案号。
- **备案期间域名不得解析到国内服务器对外提供服务**（接入商检查）。
  预览可用「本地 hosts 指 IP」或 ssh 隧道，不对外。

### 3. 备案通过后：域名解析
域名注册商 DNS 控制台，各添加两条 A 记录：
```
cn-language.com   A   <TX_01 公网 IP>
www.cn-language.com   A   <TX_01 公网 IP>
cn-os.com   A   <TX_01 公网 IP>
www.cn-os.com   A   <TX_01 公网 IP>
```
（IP 以 TX_01 控制台为准；TTL 默认即可。）

## 二、AI 执行的服务器端（用户放行端口后）

```bash
# 1. 安装 Caddy（Debian/Ubuntu）
ssh TX_01 'sudo apt-get install -y debian-keyring debian-archive-keyring apt-transport-https curl'
ssh TX_01 'curl -1sLf "https://dl.cloudsmith.io/public/caddy/stable/gpg.key" | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg'
ssh TX_01 'curl -1sLf "https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt" | sudo tee /etc/apt/sources.list.d/caddy-stable.list'
ssh TX_01 'sudo apt-get update && sudo apt-get install -y caddy'

# 2. 站点落盘
bash scripts/deploy_website.sh

# 3. Caddyfile（/etc/caddy/Caddyfile）
#    cn-language.com, www.cn-language.com {
#        root * /var/www/cn-language
#        encode gzip
#        file_server
#    }
#    cn-os.com, www.cn-os.com {
#        root * /var/www/cn-language
#        handle_path /os/* { file_server { root /var/www/cn-language } }   # os 子站同树
#        handle { redir / /os/ permanent }                                  # cn-os.com 根跳落地页
#        file_server
#    }
#    备案未下时先注释域名、用 :8080 预览
ssh TX_01 'sudo systemctl reload caddy'
```
HTTPS 由 Caddy 对两个域名自动签发（Let's Encrypt），解析生效后数分钟内完成。

## 三、日常发布

```bash
bash scripts/deploy_website.sh          # 默认 TX_01
bash scripts/deploy_website.sh TX_02    # 也可指定别名
```

## 四、更新节奏（防折腾税）

- 与里程碑绑定：自举全链贯通、对外发布版、语言规范大版本——各更新一次。
- 静态资源更新后 bump HTML 里的 `?v=N` 版本号。
- 备案号下来后：替换各页页脚「京ICP备00000000号（备案办理中·占位）」为真实备案号
  （含链接 https://beian.miit.gov.cn/ ，全站 18 个 HTML 统一替换）。

## 五、回滚

站点为纯静态文件：`ssh TX_01 'rm -rf /var/www/cn-language/*'` 后重跑发布脚本即全新覆盖；
Caddy 停启：`sudo systemctl stop|start caddy`。
