# CN 语言 · 品牌 Logo 使用说明

主 logo 于 2026-10-10 由用户裁决定稿：**方案戊「怀中印」**——CN 汉字骨架字标，
C 的怀抱里安放一方「中」字金印。寓意：**C 系骨架承载中文语义**（怀中抱中）。

设计过程与候选方案全记录见仓库外评审稿：`~/Documents/cn-logo设计/logo设计方案评审.html`。

## 文件清单

| 文件 | 用途 |
|------|------|
| `logo.svg` | 主 logo·通用版（浅底/白底） |
| `logo-暗底.svg` | 墨玉暗色主题版（骨架提亮·朱砂提亮·印面深墨） |
| `logo-单色墨.svg` | 单色印刷/水印/刻章（印章「中」真镂空，任意底可用） |
| `logo-单色宣纸.svg` | 深底反白单色版（同上镂空） |
| `favicon.svg` / `favicon-32.png` / `favicon-16.png` | 浏览器标签页图标 |
| `logo-<尺寸>.png`（16~512） | 位图尺寸梯（透明底 RGBA） |
| `印章-辅助.svg` / `印章-辅助-512.png` | 辅助标识备选稿：甲「金石印章」完整版（规范「定稿之印」等典礼场合·未启用） |

## 色板（对齐 `website/assets/tokens.css`，勿改值）

| 语义 | token | hex（浅主题） | hex（暗主题） |
|------|-------|--------------|--------------|
| 墨黑（骨架） | `--color-fg` | `#221F18` | `#E9E6E0` |
| 朱砂（印底） | `--color-accent` | `#AD2F22` | `#EB755F` |
| 印面白（「中」字） | `--color-accent-fg` | `#FCFAF6` | `#14120F`（深墨） |
| 宣纸白 | `--color-bg` | `#F9F6F2` | — |

暗色主题的印面用深墨而非白，是刻意选择：暗主题朱砂已提亮（L 0.695），
白字对比不足（≈2.5:1），深墨字对比达标（≈6.9:1）——与 tokens 的 `accent-fg` 语义一致。

## 使用规则

- **选版**：浅底用 `logo.svg`，墨玉暗底用 `logo-暗底.svg`；灰度印刷/水印用单色版；
  禁止在深色照片上直接叠通用版。
- **最小尺寸**：SVG 无限缩放；位图建议 ≥24px 用彩色版，16px 仅 favicon 场景
  （此时印章收缩为红点，环抱构图仍成立）。
- **禁用**：不拉伸变形、不加投影/描边/渐变、不更换色值、不将印章移出 C 的怀抱、
  印章内不替换其它字样。
- **留白**：logo 四周预留不小于「印章边长（约 logo 高度 21%）」的净空。

## 官网页面挂载 favicon

```html
<link rel="icon" href="assets/brand/favicon.svg" type="image/svg+xml">
<link rel="icon" href="assets/brand/favicon-32.png" sizes="32x32" type="image/png">
<link rel="icon" href="assets/brand/favicon-16.png" sizes="16x16" type="image/png">
```

`index.html` 已挂载；`docs/` 子页面后续维护轮同样处理。
