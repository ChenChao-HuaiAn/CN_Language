---
name: awesome-html
description: >-
  Turn Markdown, research notes, reports, or any content into a polished, self-contained
  single-file HTML document styled with the AwesomeDesignSystem. Use when the user asks to
  "make this into an HTML page/report/handout", "create a beautiful HTML doc", "turn this
  markdown into a webpage", build a single-file slide/one-pager, or wants a shareable,
  no-build HTML artifact that looks designed rather than default. Produces one .html file
  (inline tokens, distinctive type, tasteful CSS-only motion, dark mode, print-ready).
---

# AwesomeHTML — content → designed single-file HTML

Produce ONE self-contained `.html` file (all CSS, fonts, and JS inline or via CDN — no build
step, opens by double-click) that turns content into an editorial, designed document. It must
read as authored, never like a default Markdown export.

## 0. Load what you need

Read from the knowledge base (two levels up): `foundations/tokens.md` (inline these tokens),
`foundations/typography.md` (long-form type + 日本語 if present), `motion/recipes.md` (the
**CSS-only** recipes — no React here), and skim `00-philosophy/human-not-ai.md` for the standard.

## 1. Commit to a document direction

Editorial/magazine is the strong default for documents. Choose a distinctive display+body pair
(e.g. Fraunces/Newsreader + a clean grotesque; or a technical JetBrains Mono accent). Pick a
dominant color + one accent in OKLCH. `VISUAL_DENSITY` low: generous measure and whitespace make
long-form feel premium.

## 2. Non-negotiable structure & rules

- **Self-contained:** one file. Inline `<style>` with the `:root{}` token block + `.dark` overrides.
  Fonts via a CDN `<link>` (Fontshare/Google) with a system fallback; note self-hosting for production.
- **Reading experience:** body measure 60–75ch, `--leading` ~1.6 (1.7–1.9 for 和文), fluid `--text-*`
  scale, `text-wrap: balance` on headings / `pretty` on paragraphs. Style code blocks, tables,
  blockquotes/callouts, and links deliberately — not browser defaults.
- **Hierarchy:** a real title block (not a centered cliché), a sticky/inline TOC for long docs,
  clear section rhythm, one focal accent. Number or letter sections if it aids scanning.
- **Motion (CSS only):** ONE orchestrated load — staggered section reveal via `@keyframes` +
  `animation-delay`, or scroll-driven `animation-timeline: view()`. Wrap in
  `@media (prefers-reduced-motion: reduce)` to disable. No scattered effects.
- **Dark mode:** a small inline JS toggle flipping `data-theme`; respect `prefers-color-scheme`.
- **Print:** a `@media print` block (legible, ink-friendly, hide nav/toggle).
- **A11y:** semantic landmarks, heading order, `:focus-visible` ring via `--color-ring`, AA contrast.

## 3. Skeleton

```html
<!doctype html><html lang="..." data-theme="light"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>…</title>
<link rel="preconnect" href="https://fonts.bunny.net"><link href="…" rel="stylesheet">
<style>
  :root{ /* paste the OKLCH token contract: colors, --space-*, --radius-*, --text-*, --font-*, --ease-*, --dur-* */ }
  [data-theme="dark"]{ /* color overrides (adjust L/C) */ }
  /* base reset, type, layout (grid + measure), components, ONE staggered reveal, print, reduced-motion */
</style></head>
<body>
  <header>…title block…</header>
  <nav aria-label="Contents">…TOC…</nav>
  <main>…content sections…</main>
  <footer>…</footer>
  <script>/* theme toggle + persist */</script>
</body></html>
```

## 4. Verify

Run the Pre-Flight checklist (`human-not-ai.md`). Confirm: single file opens standalone, dark mode
works, reduced-motion disables animation, prints cleanly, AA contrast, and it reads as *designed*.
If converting 日本語 content, apply the 和文 typography rules from `foundations/typography.md`.
