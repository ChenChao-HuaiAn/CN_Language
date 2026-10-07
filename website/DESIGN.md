# CN 语言官网 · 设计宣言（DESIGN.md）

> 依 AwesomeDesignSystem 流程：先宣言方向，再写 CSS。所有页面与后续迭代均以本文件为准。

## 方向宣言（Purpose / Tone / Constraints / Differentiation）

- **Purpose**：向中文开发者（从新手到系统级工程师）介绍并教会 CN 语言——一门全中文语法的
  系统级编程语言。要传达的第一印象：**这是严肃的工程（自举、三后端、600+ E2E），同时是
  独一无二的中文语言**。
- **Tone（一个极端）**：**墨韵印刷 / 编辑部风**——「宣纸上的活字期刊」。纸白为底、墨黑正文、
  朱砂一点；衬线宋体大标题、大量留白、细线分隔；代码块是版面的主角（带语法高亮与
  「批注」气质）。拒绝科技感渐变、拒绝居中 hero 俗套、拒绝卡片海。
- **Constraints**：零构建零外链（中文 webfont 动辄 MB 级，故正文/标题用系统字体栈、
  以「宋体衬线标题」为刻意选择——CJK 网站的正当模式，见 design-system typography §7）；
  WCAG 2.2 AA；`prefers-reduced-motion` 全量回退；暗色「墨玉」主题；国内访问速度优先。
- **Differentiation（两个具名锚点）**：**宋版书的排版纪律**（衬线大标题、竖排点缀、朱砂批注）
  × **Zig 官网的代码优先工程感**（数据说话、无废话、代码块即版面中心）。
  Swap-test：把这套版面套到任何英文编程语言官网上都不成立——竖排汉字、朱砂印章、
  宋体标题、中文代码，是 CN 语言专属身份。

## 色彩（OKLCH，语义命名）

- 一个主导强调色：**朱砂**（hue 30，印章批注色）——用于行动点、关键字高亮、当前态。
  无紫、无蓝渐变。状态色：竹绿（成功）、赭石（警告）、绯红（危险），均与朱砂同重量级。
- 中性色从暖墨色相（hue 85）推导：宣纸白 → 墨黑。暗色主题为「墨玉」：暖墨黑底 +
  宣纸白字，朱砂提亮（L+C 上调，非 naive 反转）。
- 代码高亮是语法语义着色：关键字=朱砂、类型=赭石、字符串=竹绿、注释=淡墨、
  数字=青墨（低饱和）。

## 字体（三轨，系统栈）

- `--font-display` 标题/身份：宋体系衬线（`Source Han Serif SC → Songti SC → SimSun`）。
- `--font-body` 正文：黑体系无衬线（`Source Han Sans SC → PingFang SC → Microsoft YaHei`）。
- `--font-mono` 代码：`Cascadia Code → JetBrains Mono → Consolas` + 中文兜底黑体。
- 中文排版律：正文行高 1.85、行宽 ≤ 42 全角字、**禁斜体**（CJK 无斜体，强调用字重与朱砂）、
  标题 `text-wrap: balance`、`line-break: strict` 禁则处理。

## 动效

一次编排的入场 stagger（hero 元素 60ms 步进上浮）> 零散微交互；只动 transform/opacity；
`--ease-out` + `--dur-base`；`prefers-reduced-motion` 时溶解为直出。

## 执法清单（发布前逐项过）

Ant-i-AI-Slop checklist（design-system/00-philosophy/human-not-ai.md §5）：
无默认字体滥殓（宋体标题=声明过的选择）、无紫渐变、hero 非居中四件套、
无卡片套卡片、层级靠 3 倍字号跳变与字重极值、焦点环可见、AA 对比度双主题达标、
文案有具体声音无套话。组件只读语义 token，禁止硬编码色值。
