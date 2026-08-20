# Typography

> **The verdict.** Canonical typography foundation for the design system. Copy-paste-ready, framework-agnostic CSS first, then a React 19 + Tailwind v4 + Motion reference. Every value maps to the canonical token contract (`--text-*`, `--leading-*`, `--tracking-*`, `--font-*`). AI agents: read this without fetching links — the real code and concrete values are embedded below.

Type is the loudest signal of taste in a product. The default move — Inter/Roboto/system on white — reads as "unfinished," not "neutral." This module gives you a distinctive-but-disciplined system: **one display face for identity, one body face for invisibility**, a fluid modular scale, optical per-size rhythm, hardened web-font loading, and a full dedicated Japanese (和文) section.

---

## 1. Principles (the non-negotiables)

1. **One display + one body.** The display face carries identity at large sizes; the body face must vanish at 16–20px over long reading. Add a mono only when you render code. Contrast them in style (serif vs sans), weight, or personality — never ship two near-identical sans.
2. **Hierarchy is contrast, not count.** Build levels with size, weight, color, and space. One focal type element per screen. A timid scale (16/18/20/24) reads as no hierarchy at all.
3. **Optical tuning per size.** Big text wants tighter leading and negative tracking; small text wants looser leading and slightly positive tracking. A single global `line-height` is an anti-pattern.
4. **Fluid, but accessible.** `clamp(rem + vw, …)` always mixes `rem` into the preferred term so text still responds to user zoom (WCAG 1.4.4). Never floor body below `1rem`/16px.
5. **和文 is its own system.** Japanese needs more leading (1.6–1.9), gentle tracking (0.04–0.08em), `palt`, correct kinsoku, and **no italics, ever**. Do not bolt Latin defaults onto Japanese.

---

## 2. Font Selection — Escape the Inter/Roboto/Arial Default

Neutral UI sans-serifs are excellent where the font should be invisible (dense dashboards). They are the wrong default for a branded product or marketing surface. Pick a **distinctive body font**, or pair a **display face (personality) + body face (legibility)** with deliberate contrast.

### Distinctive body-font alternatives to Inter (2026)

| Font | Foundry / source | Character | Best for | License |
|------|------------------|-----------|----------|---------|
| **Geist / Geist Mono** | Vercel | Neutral-but-crisp, engineered | Dev tools, SaaS | OFL, free |
| **Satoshi** | Fontshare (ITF) | Squared curves, editorial, "expensive" | Premium SaaS, fintech | Free (Fontshare); self-host |
| **General Sans** | Fontshare (ITF) | Clean geometric workhorse | SaaS body | Free (Fontshare); self-host |
| **Space Grotesk** | Florian Karsten | Mono-inspired quirk, squared terminals | Developer / aerospace vibe | OFL, Google Fonts |
| **Plus Jakarta Sans** | Tokotype | Bright geometric, high x-height | Energetic startups, B2B | OFL, Google Fonts |
| **Mona Sans / Hubot Sans** | GitHub | Grotesque; variable `wght`/`wdth`/`slnt` | Apps, dashboards | OFL, free |
| **Inclusive Sans** | Olivia King | A11y-first, open counters | A11y-critical UI | OFL, Google Fonts (2025) |
| **Aeonik** (commercial) | CoType Foundry | Neo-grotesque, mechanical + warm | Premium brand | Paid |

### High-impact display + body pairings (by context)

| Display (headlines) | Body | Vibe / when to reach for it |
|---------------------|------|-----------------------------|
| **Bricolage Grotesque** | Satoshi / Inter | The signature 2025–26 pairing; distinctive yet professional. Default for most products. |
| **Clash Display** | Satoshi | High-impact e-commerce / landing. Big, confident hero type. |
| **General Sans** | Satoshi | The SaaS workhorse pairing; quiet, trustworthy. |
| **Syne** | DM Sans | Angular personality, memorable headlines for studios/agencies. |
| **Playfair Display** (serif) | Inter | Editorial luxury, lifestyle/boutique, long-form reading. |
| **Space Grotesk** | General Sans | Technical / developer brand without going full-mono. |

> **Licensing reality.** Fontshare faces (Satoshi, General Sans, Clash Display, Switzer) are free but **not on Google Fonts** — you must self-host. Always verify webfont licensing before self-hosting commercial faces; some licenses forbid self-hosting entirely.

### How this maps to the token contract

The contract exposes three font tokens. Bind them once; the rest of the system references the token, never a raw family name.

```css
:root {
  /* Display = identity. Body = invisible. Mono = code/tabular only. */
  --font-display: "Bricolage Grotesque", "Satoshi", system-ui, sans-serif;
  --font-body:    "Satoshi", "Inter", system-ui, sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;
}
```

---

## 3. Variable Fonts

A variable font packs many weights/widths/styles into **one** file via the OpenType variations spec. One ~82 KB variable file can replace ~223 KB of static cuts (~63% smaller — savings scale with how many cuts you'd otherwise ship). Browser support is ~94% globally in 2026. Use them when you need 3+ weights/styles; for 1–2 weights a static WOFF2 may still win.

### Axes: registered (lowercase) vs custom (UPPERCASE) — tags are case-sensitive

| Tag | Axis | Notes |
|-----|------|-------|
| `wght` | Weight | Prefer the high-level `font-weight` property |
| `wdth` | Width | Condensed → extended; via `font-stretch` |
| `ital` | Italic | Toggle (0/1) |
| `slnt` | Slant | Oblique angle (degrees) |
| `opsz` | Optical size | Auto via `font-optical-sizing: auto` |
| `GRAD` | Grade (custom) | Changes weight **without reflow** — safe to animate |

### Prefer standard properties; drop to the low-level only when needed

```css
/* GOOD: high-level properties cascade & animate cleanly. */
h1 { font-weight: 750; font-stretch: 110%; font-optical-sizing: auto; }

/* LOW-LEVEL escape hatch: only for axes with no standard property (e.g. GRAD).
   No % sign inside font-variation-settings — ever. Set ALL axes in one declaration. */
.dense { font-variation-settings: "wght" 600, "GRAD" 88; }
```

`font-variation-settings` is a low-level escape hatch: anything you set there does not compose cleanly with the high-level props, so set every axis you need in one declaration when you use it.

### Animate weight without layout shift (use GRAD)

```css
.btn {
  font-variation-settings: "wght" 500, "GRAD" 0;
  transition: font-variation-settings var(--dur-fast, 150ms) var(--ease-out, ease);
}
.btn:hover { font-variation-settings: "wght" 500, "GRAD" 120; } /* density up, zero reflow */
```

### `@font-face` for a variable font

```css
@font-face {
  font-family: "Satoshi";
  src: url("/fonts/Satoshi-Variable.woff2") format("woff2-variations");
  font-weight: 300 900;     /* declare the supported range */
  font-stretch: 75% 125%;
  font-display: swap;
}
```

---

## 4. Fluid Type Scale → `--text-*` Tokens

`clamp(MIN, PREFERRED, MAX)` renders the preferred value bounded by min/max. For fluid type the **preferred term mixes `rem` + `vw`** (never `vw` alone) — the `rem` part keeps text responsive to the user's font preference and zoom, which is an accessibility requirement (WCAG 1.4.4).

```css
font-size: clamp(1rem, 0.91rem + 0.43vw, 1.25rem);
/*               ↑min  ↑rem base   ↑vw    ↑max  */
```

### The canonical scale (Major Third ≈ 1.25, Utopia-generated)

These are the exact `--text-*` tokens for the system, each with a paired `--leading-*` and `--tracking-*`. Body is `--text-base` (16→20px fluid).

```css
:root {
  /* ---- FONT FAMILIES ---- */
  --font-display: "Bricolage Grotesque", "Satoshi", system-ui, sans-serif;
  --font-body:    "Satoshi", "Inter", system-ui, sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;

  /* ---- FLUID SIZE TOKENS: clamp(min, rem + vw, max) ---- */
  --text-xs:   clamp(0.75rem, 0.71rem + 0.18vw, 0.83rem);  /* 12 → 13 */
  --text-sm:   clamp(0.875rem, 0.82rem + 0.27vw, 1rem);    /* 14 → 16 */
  --text-base: clamp(1rem, 0.91rem + 0.43vw, 1.25rem);     /* 16 → 20  (BODY) */
  --text-lg:   clamp(1.2rem, 1.07rem + 0.65vw, 1.56rem);   /* 19 → 25 */
  --text-xl:   clamp(1.44rem, 1.26rem + 0.93vw, 1.95rem);  /* 23 → 31 */
  --text-2xl:  clamp(1.73rem, 1.47rem + 1.29vw, 2.44rem);  /* 28 → 39 */
  --text-3xl:  clamp(2.07rem, 1.72rem + 1.77vw, 3.05rem);  /* 33 → 49 */
  --text-4xl:  clamp(2.49rem, 2.01rem + 2.40vw, 3.82rem);  /* 40 → 61 */
  --text-5xl:  clamp(2.99rem, 2.34rem + 3.24vw, 4.77rem);  /* 48 → 76 */
  --text-6xl:  clamp(3.58rem, 2.71rem + 4.36vw, 5.96rem);  /* 57 → 95 */
  --text-7xl:  clamp(4.30rem, 3.12rem + 5.87vw, 7.45rem);  /* 69 → 119 */

  /* ---- LINE-HEIGHT TOKENS (size-dependent: tighter as size grows) ---- */
  --leading-none:    1;
  --leading-tight:   1.1;    /* display / H1 */
  --leading-snug:    1.2;    /* H2 / H3 */
  --leading-normal:  1.45;   /* small / caption */
  --leading-body:    1.6;    /* long-form body (Latin) */
  --leading-relaxed: 1.75;

  /* ---- TRACKING TOKENS (negative as size grows; positive for caps/small) ---- */
  --tracking-tighter: -0.03em; /* large display */
  --tracking-tight:   -0.02em; /* H1 / H2 */
  --tracking-normal:  0;       /* body */
  --tracking-wide:    0.02em;  /* small / caption */
  --tracking-caps:    0.08em;  /* all-caps eyebrows / labels */
}
```

### Role → token mapping (use this, don't improvise)

| Role | size | leading | tracking | family | measure |
|------|------|---------|----------|--------|---------|
| Hero / display | `--text-6xl`/`7xl` | `--leading-none`/`tight` | `--tracking-tighter` | `--font-display` | 8–16ch |
| H1 | `--text-4xl` | `--leading-tight` | `--tracking-tight` | `--font-display` | ≤ 18ch |
| H2 | `--text-3xl` | `--leading-snug` | `--tracking-tight` | `--font-display` | — |
| H3 | `--text-2xl` | `--leading-snug` | `--tracking-normal` | `--font-display`/`body` | — |
| Lead / intro | `--text-lg` | `--leading-body` | `--tracking-normal` | `--font-body` | 50–66ch |
| Body | `--text-base` | `--leading-body` | `--tracking-normal` | `--font-body` | **60–75ch** |
| Small / caption | `--text-sm` | `--leading-normal` | `--tracking-wide` | `--font-body` | — |
| Micro / legal | `--text-xs` | `--leading-normal` | `--tracking-wide` | `--font-body` | — |
| Eyebrow / label | `--text-sm` | `--leading-none` | `--tracking-caps` | `--font-body` | — |
| Code | `--text-sm` | `--leading-normal` | `--tracking-normal` | `--font-mono` | — |

```css
h1 { font: 750 var(--text-4xl)/var(--leading-tight) var(--font-display);
     letter-spacing: var(--tracking-tight); text-wrap: balance; max-width: 18ch; }
h2 { font: 700 var(--text-3xl)/var(--leading-snug) var(--font-display);
     letter-spacing: var(--tracking-tight); text-wrap: balance; }
p  { font: 400 var(--text-base)/var(--leading-body) var(--font-body);
     max-width: 66ch; text-wrap: pretty; }
.eyebrow { font: 600 var(--text-sm)/var(--leading-none) var(--font-body);
     letter-spacing: var(--tracking-caps); text-transform: uppercase; }
```

### Container-query units (component-aware, ship in 2026)

Scale by the component, not the viewport — the same card works in a sidebar or full-width.

```css
.card { container-type: inline-size; }
.card h2 { font-size: clamp(1.25rem, 5cqi, 2rem); } /* cqi = container inline size */
```

### Accessibility guardrails (do NOT skip)

- **Never floor body below `1rem` (16px); keep body max 18–22px.**
- **Cap the max at ≤ ~2.5× the min** so extreme viewports don't blow type up.
- **WCAG 1.4.4 (Resize text, AA):** `vw`-bounded text fails if a user can't reach 200% zoom. **Test at 200% browser zoom — text must visibly enlarge.** Mixing `rem` into the preferred term is what makes it pass.
- Fluid ≠ universal. Long-form reading wants conservative scaling; landing pages can be dramatic.

> **Tooling:** generate scales with [Utopia.fyi](https://utopia.fyi/) or a Fluid Type Scale Calculator — don't hand-math the `vw` coefficients.

---

## 5. Per-Size Rhythm, Measure & Modern Rendering

`line-height`, `letter-spacing`, and `measure` (line length) are **size-dependent** — this is optical tuning, not one global value.

| Role | font-size | line-height | letter-spacing | measure (max-width) |
|------|-----------|-------------|----------------|---------------------|
| Display / H1 | ≥ 2.4rem | 1.05–1.15 | −0.02 to −0.03em | 8–18ch (short) |
| H2 / H3 | 1.5–2rem | 1.15–1.25 | −0.01 to −0.02em | — |
| Body | 1–1.25rem | 1.5–1.65 | 0 | **60–75ch** |
| Small / caption | 0.8–0.9rem | 1.4–1.5 | +0.01 to +0.02em | — |
| All-caps label | any | — | **+0.05 to +0.1em** | — |

**Rules of thumb:** tracking shrinks (toward negative) as size grows; leading shrinks as size grows; only all-caps and small text get positive tracking. Constrain body `max-width` in `ch` so the measure tracks the font.

### Modern wrapping & rendering (ship in 2026)

```css
:root { --measure: 66ch; } /* ideal Latin line length 45–75ch */

h1, h2, h3 { text-wrap: balance; max-width: 40ch; } /* needs a max-width to balance */
p          { text-wrap: pretty; max-width: var(--measure); } /* no orphaned last word */
body       { -webkit-font-smoothing: antialiased; text-rendering: optimizeLegibility; }
```

- `text-wrap: balance` — even line widths for headings. Support: **Chrome 114+, Firefox 121+, Safari 17.5+**. Chromium balances ≤ 4–6 lines, Firefox ≤ 10. **Never** on long paragraphs.
- `text-wrap: pretty` — prevents orphaned last words in body. **Chrome 117+, Edge 117+, Safari 26+; Firefox not supported as of early 2026** (degrades to normal wrap). WebKit's `pretty` rebalances more lines than Chromium's last-4-line approach.
- Both are progressive enhancement — unsupported browsers fall back to normal wrap, no breakage.

### Numerals & OpenType in UI

```css
.tabular  { font-variant-numeric: tabular-nums; }     /* align numbers in tables/timers/prices */
.fraction { font-variant-numeric: diagonal-fractions; }
.no-lig   { font-variant-ligatures: none; }           /* code tokens */
```

> **Tabular figures are not optional** in tables, dashboards, timers, and price columns. Proportional figures make numbers jitter as they change — use `tabular-nums` anywhere a digit updates in place.

---

## 6. Web Font Loading & Performance

Fonts are a top cause of CLS and slow LCP. The optimization stack — WOFF2 (~60–70% smaller) + subsetting (50–80% more) + self-hosting (200–500ms faster) + preload + `font-display` + fallback metric matching — can yield ~2.6s better LCP and CLS < 0.1. **Budget: < 100 KB total fonts, 2–4 files max.**

### `font-display` strategies

| Value | Behavior | Use when |
|-------|----------|----------|
| `swap` | Fallback immediately, swap on load (visible shift) | Default; pair with matched fallback metrics |
| `optional` | ~100ms window, else fallback for the whole visit → **zero CLS** | Performance-critical pages |
| `fallback` | Short block, then swap within a window | Middle ground |
| `block` | Hide text until load | **Avoid** (FOIT) |

### Subsetting

A full Inter has 2,500+ glyphs; English needs ~200 — subsetting can cut 90 KB → 15 KB. Google Fonts subsets automatically. Self-hosted: use `pyftsubset` (fonttools).

```bash
pyftsubset Inter.ttf \
  --unicodes="U+0000-00FF,U+2018-2019,U+201C-201D,U+2013-2014" \
  --layout-features="kern,liga,calt" \
  --flavor=woff2 --output-file=Inter.subset.woff2
```

### Self-host + preload + match fallback metrics (framework-agnostic)

```html
<link rel="preload" href="/fonts/Satoshi-Variable.woff2" as="font" type="font/woff2" crossorigin>
```
```css
@font-face {
  font-family: "Satoshi";
  src: url("/fonts/Satoshi-Variable.woff2") format("woff2-variations");
  font-weight: 300 900;
  font-display: swap;
}
/* Metric-matched fallback kills CLS during swap (web.dev). */
@font-face {
  font-family: "Satoshi-fallback";
  src: local("Arial");
  size-adjust: 97%;        /* scale glyphs to match x-height/width */
  ascent-override: 95%;
  descent-override: 24%;
  line-gap-override: 0%;
}
:root { --font-body: "Satoshi", "Satoshi-fallback", system-ui, sans-serif; }
```

> **Browser support nuance.** `size-adjust`: Chrome/Edge 87+, Firefox 89+, **and Safari**. The override trio (`ascent-override`/`descent-override`/`line-gap-override`): Chrome/Edge 87+, Firefox 89+, but **NOT Safari as of 2026** (WebKit bug #219735 open through Safari 26.x) — Safari uses only `size-adjust`; the trio degrades gracefully. Tools like Fontaine / `next/font` compute these automatically.

### `next/font` (does subsetting, self-host, preload, CLS fix for you)

```ts
// app/fonts.ts
import { Inter, Geist_Mono } from "next/font/google";
import localFont from "next/font/local";

export const body = Inter({
  subsets: ["latin"],
  variable: "--font-body",
  display: "swap",
  adjustFontFallback: true,   // auto size-adjust fallback → near-zero CLS
});

export const mono = Geist_Mono({ subsets: ["latin"], variable: "--font-mono", display: "swap" });

// Self-hosted Fontshare/commercial variable face for display:
export const display = localFont({
  src: "../public/fonts/BricolageGrotesque-Variable.woff2",
  variable: "--font-display",
  weight: "200 800",
  display: "swap",
});
```
```tsx
// app/layout.tsx
<html lang="en" className={`${body.variable} ${display.variable} ${mono.variable}`}>
```

`next/font` fetches at **build time** (no runtime request to Google), removes unused styles, preloads, and generates the metric-matched fallback automatically. Prefer it over a `<link>` to `fonts.googleapis.com`.

### Verify

View source for `@font-face` with `font-display: swap`; confirm WOFF2 files < 100 KB; throttle to Slow 3G (text shows in fallback, then swaps); Lighthouse CLS < 0.1.

---

## 7. Japanese (和文) Typography — Dedicated Section

Japanese needs different rules than Latin. There is no x-height/baseline system; every glyph (kana, kanji) is ~the size of a Latin capital and sits on a square em-box. Kanji are dense (more strokes per glyph), so 和文 needs **more leading, gentle tracking, and proportional-metrics kerning**. **Italics do not exist** — never apply `font-style: italic` to Japanese (browsers synthesize an ugly slanted oblique). For emphasis use weight, color, or 「」 brackets / 傍点 (`text-emphasis`).

### Core values

| Property | Latin norm | 和文 recommendation |
|----------|-----------|---------------------|
| `line-height` | 1.4–1.6 | **1.6–1.9** (commonly ~1.7–1.85; some editorial sites 2.0) |
| `letter-spacing` | 0 | **0.04–0.08em** (range 0.05–0.15em by font) |
| measure (CPL) | 45–75 chars | **~35–45 全角** chars per line |
| size in mixed UI | base | reduce 和文 **10–15%** vs Latin, or scale Latin up |
| alignment | left | `justify` works well (no inter-word spaces to stretch) |
| emphasis | italic | **weight / `text-emphasis` (傍点) / 「」** — never italic |

### `palt`: proportional metrics (文字詰め) — the key refinement

Default 和文 is full-width monospaced (`pwid`), which looks loose — especially kana in Yu Gothic. Enabling **`palt`** applies the font's proportional spacing tables, tightening kana and punctuation dramatically. `palt` ≠ `pwid`: they are different features and only `palt` gives the polished tightening. Supported in all modern browsers.

```css
.ja {
  font-feature-settings: "palt" 1;  /* proportional alternate metrics */
  letter-spacing: 0.04em;           /* with palt on, add a little back for breathing room */
}
```

### Production 和文 body CSS — wired to the token contract

```css
:root {
  /* Pair a 和文 face with a Latin face; list BOTH JP + romanized names —
     some browsers only match one spelling. Latin name FIRST so Latin glyphs
     render in the Latin face (和欧混植), then the 和文 face fills the rest. */
  --font-ja: "Inter", "Noto Sans JP", "Hiragino Kaku Gothic ProN",
             "ヒラギノ角ゴ ProN", "Yu Gothic", "游ゴシック", "Meiryo",
             system-ui, sans-serif;

  /* JP-specific rhythm tokens (extend the contract for 和文) */
  --leading-ja:        1.8;     /* body */
  --leading-ja-head:   1.4;     /* headings — tighter, still > Latin */
  --tracking-ja:       0.04em;  /* 0.04–0.08em */
  --tracking-ja-head:  0.02em;
}

.ja, [lang="ja"] {
  font-family: var(--font-ja);
  font-size: var(--text-base);          /* ≥ 12px floor; 16px for general/older audiences */
  line-height: var(--leading-ja);       /* 和文 wants 1.6–1.9 */
  letter-spacing: var(--tracking-ja);   /* 0.04–0.08em */
  font-feature-settings: "palt" 1;
  text-align: justify;
  word-break: normal;
  line-break: strict;                   /* correct kinsoku (禁則) line-breaking */
  overflow-wrap: anywhere;
}

.ja h1, .ja h2, .ja h3, [lang="ja"] :is(h1, h2, h3) {
  line-height: var(--leading-ja-head);  /* headings tighter than body, still > Latin */
  letter-spacing: var(--tracking-ja-head);
  font-feature-settings: "palt" 1;
  font-style: normal;                   /* guard against synthetic obliques */
}
```

### 和欧混植 (mixed Japanese + Latin)

Listing the **Latin face first** in `--font-ja` means Latin glyphs and digits use the Latin face while 和文 falls through to the Japanese face — the cleanest mixed setting without per-span markup. When the Latin face's cap-height clashes with the 和文 size, nudge with `font-size-adjust` or drop the Latin face slightly. Reduce 和文 ~10–15% or scale Latin up so they feel optically equal.

### kinsoku (禁則処理), punctuation & emphasis

```css
.ja { line-break: strict; word-break: normal; }  /* don't start a line with 。、）」 */

/* Japanese emphasis = 傍点 (dots above), NOT italic. */
.ja em, .ja .emphasis {
  font-style: normal;
  text-emphasis: filled dot;
  text-emphasis-position: over right;
}
```

For visually removing extra space inside brackets/punctuation (、。！？「」（）), layer the **Yaku Han** webfont (`yakuhanjp`) *before* your 和文 face in the stack — though many JP readers don't consciously notice. Avoid `word-break: break-all` for body copy.

### Recommended JP webfonts (2026)

| Font | Role | Source / note |
|------|------|---------------|
| **Noto Sans JP** | Standard body / UI | Google + Adobe, OFL, variable; used by 政府デジタル庁 design system, JP Slack |
| **Noto Serif JP** | Editorial body / headings | OFL, variable |
| **Zen Kaku Gothic New** | Modern headings & body | Google Fonts, 5 weights, clean geometric |
| **BIZ UDPGothic / UDGothic** | UD body, max legibility | Morisawa, free on Google Fonts |
| **M PLUS 1 / M PLUS 1p** | Versatile UI | Variable, geometric nuance |
| **IBM Plex Sans JP** | Technical / docs (pairs with code) | OFL |
| **LINE Seed JP** | Mixed-script by design | Latin harmony built in |
| **Dela Gothic One** | Impact display | Heavy headline |
| **Hiragino / Yu Gothic / Meiryo** | System body | Zero download — best default for body |

### 2026 performance reality — the biggest 和文 gotcha

CJK fonts have an order of magnitude more glyphs (thousands), so files are huge. **Loading full Noto Sans JP on every page is now an anti-pattern.** The recommended pattern:

1. **Body text → system fonts** (Hiragino / Yu Gothic via the stack above). Often sufficient and zero-cost.
2. **One webfont for headings only** (limited character count → small payload).
3. Add a 和文 body webfont (Noto Sans JP / BIZ UDPGothic) only if you must match brand.
4. Limit weights: start with **400 + 700**; add more only when needed.
5. Always `font-display: swap` with a full fallback chain.
6. With Next.js use `next/font` to self-host & build-subset (Google handles `unicode-range` slicing). Confirm commercial JP font licenses allow self-hosting.

---

## 8. Tailwind v4 `@theme` Tokens

Tailwind v4 defines design tokens in CSS via `@theme`. Wire `next/font` CSS variables and the fluid `clamp()` values directly into font + size tokens; Tailwind generates `text-base`, `font-display`, `leading-tight`, etc. for you.

```css
/* app/globals.css */
@import "tailwindcss";

@theme {
  /* font families → point at next/font CSS variables */
  --font-display: var(--font-display), "Bricolage Grotesque", system-ui, sans-serif;
  --font-body:    var(--font-body), "Satoshi", system-ui, sans-serif;
  --font-mono:    var(--font-mono), ui-monospace, monospace;
  --font-ja:      var(--font-body), "Noto Sans JP", "Hiragino Kaku Gothic ProN",
                  "Yu Gothic", "Meiryo", sans-serif;

  /* fluid type scale → utilities text-xs … text-7xl */
  --text-xs:   clamp(0.75rem, 0.71rem + 0.18vw, 0.83rem);
  --text-sm:   clamp(0.875rem, 0.82rem + 0.27vw, 1rem);
  --text-base: clamp(1rem, 0.91rem + 0.43vw, 1.25rem);
  --text-lg:   clamp(1.2rem, 1.07rem + 0.65vw, 1.56rem);
  --text-xl:   clamp(1.44rem, 1.26rem + 0.93vw, 1.95rem);
  --text-2xl:  clamp(1.73rem, 1.47rem + 1.29vw, 2.44rem);
  --text-3xl:  clamp(2.07rem, 1.72rem + 1.77vw, 3.05rem);
  --text-4xl:  clamp(2.49rem, 2.01rem + 2.40vw, 3.82rem);
  --text-5xl:  clamp(2.99rem, 2.34rem + 3.24vw, 4.77rem);
  --text-6xl:  clamp(3.58rem, 2.71rem + 4.36vw, 5.96rem);
  --text-7xl:  clamp(4.30rem, 3.12rem + 5.87vw, 7.45rem);

  /* line-height & tracking */
  --leading-tight:  1.1;
  --leading-snug:   1.2;
  --leading-body:   1.6;
  --leading-ja:     1.8;
  --tracking-tight: -0.02em;
  --tracking-wide:  0.08em;
}
```
```html
<h1 class="font-display text-4xl leading-tight tracking-tight text-balance max-w-[18ch]">…</h1>
<p  class="font-body text-base leading-body max-w-[66ch] text-pretty">…</p>
<p  class="font-ja text-base leading-ja tracking-[0.04em] [font-feature-settings:'palt'_1]">日本語の本文…</p>
```

> Tailwind v4 ships `text-balance` / `text-pretty` for `text-wrap`. Arbitrary `[font-feature-settings:'palt'_1]` enables `palt`. The `_` is Tailwind's space escape inside arbitrary values.

---

## 9. React 19 Reference — Type Components + Orchestrated Load

A small set of typed primitives keeps every surface on-scale. Below: a polymorphic `Text` component and a Motion-driven staggered page-load (one orchestrated reveal beats scattered micro-interactions).

```tsx
// components/Text.tsx — React 19
import { type ElementType, type ComponentPropsWithoutRef } from "react";

const styles = {
  display: "font-display text-6xl leading-none tracking-[-0.03em] text-balance",
  h1:      "font-display text-4xl leading-tight tracking-tight text-balance max-w-[18ch]",
  h2:      "font-display text-3xl leading-snug tracking-tight text-balance",
  lead:    "font-body text-lg leading-body max-w-[60ch] text-pretty",
  body:    "font-body text-base leading-body max-w-[66ch] text-pretty",
  caption: "font-body text-sm leading-normal tracking-wide text-[var(--color-fg-muted)]",
  eyebrow: "font-body text-sm leading-none tracking-[0.08em] uppercase text-[var(--color-accent)]",
} as const;

type Variant = keyof typeof styles;

type TextProps<T extends ElementType> = {
  as?: T;
  variant?: Variant;
} & Omit<ComponentPropsWithoutRef<T>, "as">;

export function Text<T extends ElementType = "p">({
  as, variant = "body", className = "", ...rest
}: TextProps<T>) {
  const Tag = (as ?? "p") as ElementType;
  return <Tag className={`${styles[variant]} ${className}`} {...rest} />;
}
```

```tsx
// components/HeroType.tsx — orchestrated staggered reveal (Motion / framer-motion)
"use client";
import { motion } from "motion/react";

const container = {
  hidden: {},
  show: { transition: { staggerChildren: 0.08, delayChildren: 0.1 } },
};
const line = {
  hidden: { opacity: 0, y: "0.5em" },
  show: {
    opacity: 1,
    y: 0,
    transition: { duration: 0.4, ease: [0.16, 1, 0.3, 1] }, // --ease-out / expo-out
  },
};

export function HeroType() {
  return (
    <motion.header variants={container} initial="hidden" animate="show">
      <motion.p variants={line}
        className="font-body text-sm uppercase tracking-[0.08em] text-[var(--color-accent)]">
        Design System
      </motion.p>
      <motion.h1 variants={line}
        className="font-display text-6xl leading-none tracking-[-0.03em] text-balance max-w-[14ch]">
        Type that carries the brand, not just the copy.
      </motion.h1>
      <motion.p variants={line}
        className="font-body text-lg leading-body max-w-[52ch] text-pretty text-[var(--color-fg-muted)]">
        One display face for identity, one body face for invisibility.
      </motion.p>
    </motion.header>
  );
}
```

> Respect `prefers-reduced-motion`: gate the reveal so users who opt out get the final state instantly.

```css
@media (prefers-reduced-motion: reduce) {
  * { animation: none !important; transition: none !important; }
}
```

---

## 10. States — Loading, Empty, Error

Typography is part of every UI state. Theme these — never ship raw skeletons or default error text.

```tsx
// Loading: shaped skeleton lines that match the real measure/leading (no layout jump on swap)
export function TextSkeleton() {
  return (
    <div className="animate-pulse space-y-[var(--space-3)]" aria-hidden="true">
      <div className="h-[var(--text-3xl)] w-3/5 rounded-[var(--radius-md)] bg-[var(--color-surface-2)]" />
      <div className="h-[var(--text-base)] w-full max-w-[66ch] rounded-[var(--radius-sm)] bg-[var(--color-surface-2)]" />
      <div className="h-[var(--text-base)] w-11/12 max-w-[66ch] rounded-[var(--radius-sm)] bg-[var(--color-surface-2)]" />
    </div>
  );
}
```
```html
<!-- Empty: a real focal heading + muted helper, never a lone gray line -->
<div class="text-center">
  <p class="font-display text-2xl leading-snug">No results yet</p>
  <p class="font-body text-sm leading-normal text-[var(--color-fg-muted)] max-w-[40ch] mx-auto">
    Try a broader search term to see matching records.
  </p>
</div>

<!-- Error: danger color is semantic, not decorative -->
<p role="alert" class="font-body text-sm leading-normal text-[var(--color-danger)]">
  We couldn't load this content. Check your connection and retry.
</p>
```

---

## 11. Implementation Checklist

- [ ] Distinctive face chosen (not default Inter/Roboto); `--font-display` + `--font-body` have real contrast.
- [ ] Variable font where 3+ weights used; `font-weight`/`font-stretch` over raw `font-variation-settings`.
- [ ] Fluid scale via `clamp(rem + vw)`; body floor ≥ 16px; max ≤ 2.5× min; **passes 200% zoom (WCAG 1.4.4)**.
- [ ] line-height & tracking tuned per size (tight + negative for display, loose + positive for small/caps).
- [ ] `text-wrap: balance` on headings (with `max-width`), `text-wrap: pretty` on body; `tabular-nums` on numeric columns.
- [ ] WOFF2 + subset + self-host + preload; `font-display: swap`; metric-matched fallback (or `next/font` `adjustFontFallback`). Budget < 100 KB.
- [ ] Loading / empty / error states themed with scale tokens; `prefers-reduced-motion` respected.
- [ ] **JP:** line-height 1.6–1.9, letter-spacing 0.04–0.08em, `font-feature-settings: "palt" 1`, `line-break: strict`, **no italics** (傍点 for emphasis), dual JP+Latin stack (和欧混植), body = system fonts, webfont for headings only.

---

## Sources

- Variable fonts guide — MDN — https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_fonts/Variable_fonts_guide
- font-variation-settings — MDN — https://developer.mozilla.org/en-US/docs/Web/CSS/font-variation-settings
- Variable fonts global support (~94%) — caniuse — https://caniuse.com/variable-fonts
- Modern Fluid Typography Using CSS Clamp — Smashing Magazine — https://www.smashingmagazine.com/2022/01/modern-fluid-typography-css-clamp/
- Consistent, Fluidly Scaling Type and Spacing — CSS-Tricks — https://css-tricks.com/consistent-fluidly-scaling-type-and-spacing/
- Creating a Fluid Type Scale with CSS Clamp — Aleksandr Hovhannisyan — https://www.aleksandrhovhannisyan.com/blog/fluid-type-scale-with-css-clamp/
- Utopia — Fluid type & space calculator — https://utopia.fyi/
- WCAG 1.4.4 & fluid type caveat — Adrian Roselli — https://adrianroselli.com/2019/12/responsive-type-and-zoom.html
- CSS text-wrap: balance — Chrome for Developers — https://developer.chrome.com/docs/css-ui/css-text-wrap-balance
- CSS text-wrap: pretty — Chrome for Developers — https://developer.chrome.com/blog/css-text-wrap-pretty
- Better typography with text-wrap: pretty — WebKit — https://webkit.org/blog/16547/better-typography-with-text-wrap-pretty/
- CSS font metric overrides (size-adjust / ascent-override) — web.dev — https://web.dev/articles/css-size-adjust
- ascent-override @font-face support (Safari unsupported) — caniuse — https://caniuse.com/mdn-css_at-rules_font-face_ascent-override
- WebKit bug #219735 (override descriptors, still open) — https://bugs.webkit.org/show_bug.cgi?id=219735
- Custom fonts without compromise using next/font — Vercel — https://vercel.com/blog/nextjs-next-font
- next/font component — Next.js docs — https://nextjs.org/docs/app/api-reference/components/font
- Complete Web Font Optimization Guide: WOFF2, Subsetting & Performance — Font-Converters — https://font-converters.com/guides/web-font-optimization
- The Most Comprehensive Guide to Web Typography in Japanese — Masaharu Hayataki — https://medium.com/@masaharuhayataki/japanese-web-typography-anatomy-and-best-practices-185449b7be65
- Using CSS font-feature-settings for better kerning in Japanese text — ICS MEDIA — https://ics.media/en/entry/14087/
- Seven rules for perfect Japanese typography — AQ — https://www.aqworks.com/blog/perfect-japanese-typography
- Typesetting principles of Chinese, Japanese, and Korean (CJK) text — Typotheque — https://www.typotheque.com/articles/typesetting-cjk-text
- Japanese typography on the web — tips and tricks — Pavel Laptev — https://pavellaptev.medium.com/japanese-typography-on-the-web-tips-and-tricks-981f120ad20e
- Google Fonts の日本語フォント（2025年末の現状） — Zenn (yhay81) — https://zenn.dev/yhay81/articles/202512-webfont
- Noto Sans Japanese — Google Fonts — https://fonts.google.com/noto/specimen/Noto+Sans+JP
- Fontshare (Satoshi, General Sans, Clash Display, Switzer) — ITF — https://www.fontshare.com/
- Geist — Vercel — https://vercel.com/font
- Mona Sans / Hubot Sans — GitHub — https://github.com/github/mona-sans
- 15 typefaces designers can't stop using in 2026 — Creative Boom — https://www.creativeboom.com/resources/15-typefaces-designers-cant-stop-using-or-admiring-in-2026/
- Tailwind CSS v4 theme variables — Tailwind CSS docs — https://tailwindcss.com/docs/theme
