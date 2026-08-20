---
title: "Color — Foundations"
description: "Canonical color foundation for an AI-agent-first design system. OKLCH authoring, P3 wide gamut with sRGB fallback, WCAG 2.x + APCA contrast, the semantic token contract, light/dark strategy, and a copy-paste CSS + Tailwind v4 @theme block."
module: foundations/color
updated: 2026-06-09
---

# Color

> **Verdict.** Author in **OKLCH** (perceptually uniform L/C/H). Ship **sRGB-safe values
> by default**, upgrade to **`display-p3`** under `@supports` for vibrancy. Theme through
> **semantic tokens only** — components never touch a raw `--brand-500`. Pass **WCAG 2.x AA
> as the legal floor**, tune with **APCA Lc** as the readability ceiling. Build the palette
> as **one dominant hue + one sharp accent** (60-30-10), never a timid even spread. Dark
> mode **lowers L and raises C** — it is a designed context, not an inversion.

This page is the single source of truth for color. Copy the [token block](#the-canonical-token-block)
verbatim; everything above it explains the decisions so you can extend it correctly.

---

## 1. Why OKLCH is the authoring space

OKLCH is the cylindrical form of Björn Ottosson's **Oklab** model: `L` lightness, `C` chroma,
`H` hue. It is **perceptually uniform** — equal numeric changes look equal regardless of hue.
A yellow and a blue at `L 0.65` read as equally bright; HSL cannot do this (`hsl(60 100% 50%)`
is far brighter than `hsl(240 100% 50%)` at the "same" lightness). This single property is why
tonal scales built by walking `L` *look* even with zero manual correction.

Part of **CSS Color Module Level 4**; supported in Chrome 111+, Firefox 113+, Safari 15.4+
(>90% of global traffic in 2026). It is the default authoring space of this system and of
Tailwind v4.

### Syntax and ranges

```text
oklch( <L> <C> <H> [ / <alpha> ]? )
```

| Component | Range | Meaning |
|-----------|-------|---------|
| `L` lightness | `0`–`1` or `0%`–`100%` | 0 = black, 1 = white. Perceptually uniform. |
| `C` chroma | `0` → ~`0.37` (unbounded) | 0 = gray. sRGB tops out ~`0.13–0.15`, P3 ~`0.20+`. |
| `H` hue | `0`–`360` deg | 0 ≈ red, 120 ≈ green, 240 ≈ blue (like HSL). |
| `alpha` | `0`–`1` / `%` | Optional opacity after `/`. |

```css
:root {
  --demo-brand:       oklch(0.55 0.20 264); /* base indigo */
  --demo-brand-light: oklch(0.75 0.20 264); /* only L moves → cohesive tint  */
  --demo-brand-dark:  oklch(0.35 0.20 264); /* only L moves → cohesive shade  */
  --demo-neutral:     oklch(0.50 0    0);   /* C = 0 → pure gray              */
  --demo-translucent: oklch(0.60 0.20 280 / 75%);
}
```

### The chroma trap (staying in gamut)

Maximum achievable chroma depends on **both L and H** — high chroma at extreme lightness
falls outside sRGB *and* P3 and gets clamped unpredictably across browsers. The rule:

1. Pick `L` and `H` first.
2. Raise `C` until just before the hue spectrum shows clipping (the [oklch.com](https://oklch.com/)
   picker visualizes the sRGB / P3 / Rec2020 boundaries live).
3. **Taper `C` toward the extremes** of a scale — chroma peaks in the 400–600 mid band and
   *must* fall off toward 50 and 950 or the lightest/darkest steps clip.

Keep neutrals at `C ≈ 0.004–0.02` (a whisper of the brand hue), brand mids at `C 0.12–0.20`.

### Derive, don't hand-pick: relative color syntax

```css
:root {
  --base: oklch(0.62 0.20 264);

  /* Harmonies — keep L & C, rotate H */
  --complement: oklch(from var(--base) l c calc(h + 180));
  --triadic-a:  oklch(from var(--base) l c calc(h + 120));
  --triadic-b:  oklch(from var(--base) l c calc(h - 120));

  /* Interaction states — scale L, keep C & H */
  --hover:  oklch(from var(--base) calc(l * 1.10) c h);
  --active: oklch(from var(--base) calc(l * 0.90) c h);
}
```

### Blend in a perceptual space: `color-mix()`

```css
:root {
  --bg-tinted:   color-mix(in oklab, var(--color-bg) 92%, var(--color-accent));
  --border-soft: color-mix(in oklab, var(--color-fg) 12%, transparent);
  --overlay:     color-mix(in oklab, var(--color-bg) 70%, transparent);
}
```

`color-mix(in oklab, …)` avoids the muddy gray midpoints that sRGB mixing produces — use it
for hover tints, scrims, and translucent borders instead of opacity hacks.

---

## 2. Building a tonal scale (50 → 950)

Anchor `H` and a target `C`, walk `L` in even perceptual steps, taper `C` at the ends.
Below is the system's dominant indigo scale (hue 264). Mirror this exact shape for any new hue.

```css
:root {
  --indigo-50:  oklch(0.971 0.013 264);
  --indigo-100: oklch(0.936 0.032 264);
  --indigo-200: oklch(0.885 0.062 264);
  --indigo-300: oklch(0.808 0.105 264);
  --indigo-400: oklch(0.704 0.165 264);
  --indigo-500: oklch(0.637 0.200 264); /* base — peak chroma */
  --indigo-600: oklch(0.560 0.190 264);
  --indigo-700: oklch(0.476 0.160 264);
  --indigo-800: oklch(0.398 0.125 264);
  --indigo-900: oklch(0.340 0.090 264);
  --indigo-950: oklch(0.243 0.055 264);
}
```

Note the **C column rises to 0.200 at 500, then tapers both ways** (0.013 at 50, 0.055 at 950).
This is the same shape Tailwind v4 ships, e.g. `--color-red-500: oklch(0.637 0.237 25.331)`.
Keep L steps perceptually even; let chroma do the in-gamut work.

---

## 3. Wide-gamut P3 — vibrancy that degrades gracefully

`display-p3` is ~25% larger than sRGB. Reach it via `color(display-p3 r g b)` (channels `0–1`)
or by pushing OKLCH chroma past the sRGB boundary. Wide-gamut hardware is now common (Apple
devices, modern laptops/phones).

**Engine status, June 2026:** Chromium ✅, WebKit/Safari ✅, Firefox parses the syntax but
full wide-color-gamut *rendering* is still gated behind its HDR work. So **always provide an
sRGB-safe value first** and upgrade conditionally.

```css
/* RECOMMENDED for tokens — @supports is required when feeding custom properties.
   The bare cascade-fallback trick (two `color:` lines) is NOT reliable through vars. */
:root { --color-accent: oklch(0.78 0.16 84); }            /* sRGB-safe default      */
@supports (color: color(display-p3 1 1 1)) {
  :root { --color-accent: oklch(0.80 0.21 84); }          /* push C past sRGB on P3  */
}

/* MOST ROBUST — gate on hardware capability AND syntax support */
@media (dynamic-range: high) {
  @supports (color: color(display-p3 0 0 0)) {
    :root { --color-accent: color(display-p3 0.96 0.62 0.07); }
  }
}
```

Why explicit fallbacks beat auto-clamping: gamut-mapping algorithms are **not standardized**,
so an unspecified P3→sRGB clamp can land on a color you did not choose. For privacy, capability
queries return a *class* of display, never the exact gamut:

```css
@media (color-gamut: p3) { /* at least a P3-class display */ }
@supports (background: color(display-p3 0 0 0)) { /* engine parses the syntax */ }
```

---

## 4. Contrast — WCAG 2.x floor, APCA ceiling

Run a **two-layer strategy**: ship WCAG 2.x AA because that is what scanners, auditors, and
courts enforce; then tune readability with APCA. Pick colors that pass **both**.

### WCAG 2.x — the legal floor (June 2026)

Luminance-ratio model.

| Level | Normal text | Large text (≥24px, or ≥18.66px bold) & UI/graphics |
|-------|-------------|----------------------------------------------------|
| **AA** (target) | **4.5:1** | **3:1** |
| **AAA** | 7:1 | 4.5:1 |

Known weaknesses: it **over-rates dark pairs** (a poor guide for dark mode) and has arbitrary
cliffs (`#777` on white = 4.48:1 *fails*; `#767676` = 4.54:1 *passes* — visually identical).
Myth-bust: there is no background where *both* pure black and pure white fail AA — one always
passes, which is exactly what `contrast-color()` exploits. **Do not drop WCAG 2 over a draft.**

### APCA — the readability ceiling (candidate, not law)

APCA (Advanced Perceptual Contrast Algorithm) is the **candidate** for WCAG 3. It outputs an
`Lc` value (practical range ≈ ±106) that is perceptually uniform, **polarity-aware** (swapping
text/bg changes the result), and **size/weight-aware**. **Status: removed from the WCAG 3
working draft in July 2023; the algorithm is officially "yet to be determined," and WCAG 3 is
unlikely to finalize before ~2028–2030.** Treat DevTools APCA readouts as informative.

| `Lc` | Use case | Representative min size / weight |
|------|----------|----------------------------------|
| **90** | Preferred fluent body; max for very large bold | 18px/300, 14px/400 |
| **75** | Minimum for columns of body text | 18px/400, 16px/500, 14px/700 |
| **60** | Min for non-body text (≈ old 4.5:1) | 18px/600, 16px/700 |
| **45** | Large/heavy text — headlines (≈ old 3:1); fine icons | 36px/400, 24px/700 |
| **30** | Absolute min for any text; placeholder/disabled; solid icons | ≥5.5px feature |
| **15** | Min for non-text discernibility; below = invisible | ≥5px feature |

Rough map: `Lc 45 ≈ 3:1`, `Lc 60 ≈ 4.5:1`, `Lc 75 = preferred body`. In dark mode, *trust APCA
over WCAG 2* because WCAG 2 over-rates dark pairs.

### Let the browser pick text color: `contrast-color()`

CSS Color 5 `contrast-color()` returns **black or white** — whichever contrasts better — at
style time (no runtime cost). **Baseline Newly Available, April 2026: Chrome 147, Firefox 146,
Safari 26.0.** Limits: discrete (no smooth animation), flat colors only, guarantees AA-ish but
not AAA 7:1 for mid-tones; ties return white. Always provide a static `--color-accent-fg`
fallback for older engines (the token block below does).

```css
.button {
  background: var(--color-accent);
  color: var(--color-accent-fg);                 /* static fallback */
}
@supports (color: contrast-color(red)) {
  .button { color: contrast-color(var(--color-accent)); }
}

/* Branded contrast text: take only the L of black/white, re-add the surface hue */
.chip { color: oklch(from contrast-color(var(--color-surface)) l 0.04 264); }
```

---

## 5. Semantic tokens — decouple meaning from value

Components consume **roles**, never raw hues. A primitive (`--indigo-500`) is a value; a
semantic token (`--color-accent`) is a *role*. The name stays constant across themes; only the
value it resolves to changes. Without this layer, dark mode means editing every component.

```
Primitives            →  Semantic role tokens     →  (optional) Component tokens
--indigo-500             --color-accent               --button-bg
--neutral-50             --color-surface              --card-bg
```

### The role contract

These are the **canonical names**. Use them everywhere; do not invent synonyms.

| Token | Role |
|-------|------|
| `--color-bg` | Page background — the lowest layer |
| `--color-bg-subtle` | Slightly inset/alternate page background |
| `--color-surface` | Cards, panels, sheets (one step above bg) |
| `--color-surface-2` | Raised surface: popovers, menus, nested panels |
| `--color-fg` | Primary text / icons |
| `--color-fg-muted` | Secondary text, labels, captions |
| `--color-fg-subtle` | Placeholder, disabled, hints (min APCA Lc 30) |
| `--color-border` | Default hairline borders, dividers |
| `--color-border-subtle` | Faint separators inside a surface |
| `--color-accent` | Dominant brand action: primary CTA, focus, links |
| `--color-accent-fg` | Text/icon **on** `--color-accent` |
| `--color-accent-hover` | Hover/active state of accent surfaces |
| `--color-success` / `-fg` | Positive state surface / text-on |
| `--color-warning` / `-fg` | Caution state surface / text-on |
| `--color-danger` / `-fg` | Destructive/error state surface / text-on |
| `--color-ring` | Focus ring (usually = accent) |

**Elevation note.** This system expresses depth with **1px hairline borders + layered surfaces**
(`bg → surface → surface-2`), not heavy shadows. In dark mode, raise surface `L` per layer.

---

## The canonical token block

Copy this verbatim into your global stylesheet. Light is the default; `[data-theme="dark"]`
and `.dark` override the same role names. Values are sRGB-safe OKLCH; a P3 upgrade block follows.

```css
/* ============================================================
   COLOR TOKENS — paste verbatim. Author = OKLCH. Theme = roles.
   Dominant hue: indigo 264. Sharp accent: amber 84.
   ============================================================ */
:root {
  color-scheme: light;

  /* --- Tier 1: primitives (dominant indigo scale) --- */
  --indigo-50:  oklch(0.971 0.013 264);
  --indigo-100: oklch(0.936 0.032 264);
  --indigo-200: oklch(0.885 0.062 264);
  --indigo-300: oklch(0.808 0.105 264);
  --indigo-400: oklch(0.704 0.165 264);
  --indigo-500: oklch(0.637 0.200 264);
  --indigo-600: oklch(0.560 0.190 264);
  --indigo-700: oklch(0.476 0.160 264);
  --indigo-800: oklch(0.398 0.125 264);
  --indigo-900: oklch(0.340 0.090 264);
  --indigo-950: oklch(0.243 0.055 264);

  /* neutrals borrow the brand hue at near-zero chroma → grays feel related */
  --neutral-0:   oklch(1     0     0);
  --neutral-50:  oklch(0.985 0.004 264);
  --neutral-100: oklch(0.967 0.006 264);
  --neutral-200: oklch(0.922 0.008 264);
  --neutral-300: oklch(0.860 0.010 264);
  --neutral-400: oklch(0.700 0.012 264);
  --neutral-500: oklch(0.556 0.014 264);
  --neutral-600: oklch(0.440 0.014 264);
  --neutral-700: oklch(0.372 0.013 264);
  --neutral-800: oklch(0.279 0.012 264);
  --neutral-900: oklch(0.210 0.012 264);
  --neutral-950: oklch(0.145 0.010 264);

  /* sharp accent + functional hues at matched weight */
  --amber-500:   oklch(0.780 0.160 84);
  --green-500:   oklch(0.700 0.160 150);
  --orange-500:  oklch(0.800 0.160 80);
  --red-500:     oklch(0.637 0.237 25);

  /* --- Tier 2: semantic roles (what components consume) --- */
  --color-bg:            var(--neutral-50);
  --color-bg-subtle:     var(--neutral-100);
  --color-surface:       var(--neutral-0);
  --color-surface-2:     var(--neutral-50);

  --color-fg:            var(--neutral-900);
  --color-fg-muted:      var(--neutral-600);
  --color-fg-subtle:     var(--neutral-500);

  --color-border:        var(--neutral-200);
  --color-border-subtle: var(--neutral-100);

  --color-accent:        var(--indigo-600);
  --color-accent-fg:     var(--neutral-0);   /* static AA fallback for contrast-color */
  --color-accent-hover:  var(--indigo-700);

  --color-success:       var(--green-500);
  --color-success-fg:    var(--neutral-0);
  --color-warning:       var(--orange-500);
  --color-warning-fg:    var(--neutral-950);
  --color-danger:        var(--red-500);
  --color-danger-fg:     var(--neutral-0);

  --color-ring:          var(--indigo-600);
}

/* ============================================================
   DARK THEME — lower L, RAISE C (~15–25%), nudge warm hues.
   NOT a 1−L inversion. Depth = lighter surfaces, not shadows.
   ============================================================ */
:root[data-theme="dark"],
.dark {
  color-scheme: dark;

  /* surfaces step UP in L per elevation layer */
  --color-bg:            oklch(0.180 0.012 264);
  --color-bg-subtle:     oklch(0.150 0.010 264);
  --color-surface:       oklch(0.215 0.013 264);
  --color-surface-2:     oklch(0.255 0.014 264);

  --color-fg:            oklch(0.945 0.010 264);
  --color-fg-muted:      oklch(0.760 0.014 264);
  --color-fg-subtle:     oklch(0.620 0.014 264);

  --color-border:        oklch(0.300 0.014 264);
  --color-border-subtle: oklch(0.255 0.012 264);

  /* accent: +L so it lifts off dark, +C to fight simultaneous contrast */
  --color-accent:        oklch(0.705 0.225 264);
  --color-accent-fg:     oklch(0.180 0.012 264);
  --color-accent-hover:  oklch(0.760 0.215 264);

  /* functional hues: +L +C; warm hues rotate slightly toward source (Bezold-Brücke) */
  --color-success:       oklch(0.760 0.185 150);
  --color-success-fg:    oklch(0.180 0.012 264);
  --color-warning:       oklch(0.840 0.185 82);
  --color-warning-fg:    oklch(0.180 0.012 264);
  --color-danger:        oklch(0.710 0.250 27);
  --color-danger-fg:     oklch(0.180 0.012 264);

  --color-ring:          oklch(0.705 0.225 264);
}

/* Follow the OS when no explicit theme is set */
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    color-scheme: dark;
    --color-bg:            oklch(0.180 0.012 264);
    --color-bg-subtle:     oklch(0.150 0.010 264);
    --color-surface:       oklch(0.215 0.013 264);
    --color-surface-2:     oklch(0.255 0.014 264);
    --color-fg:            oklch(0.945 0.010 264);
    --color-fg-muted:      oklch(0.760 0.014 264);
    --color-fg-subtle:     oklch(0.620 0.014 264);
    --color-border:        oklch(0.300 0.014 264);
    --color-border-subtle: oklch(0.255 0.012 264);
    --color-accent:        oklch(0.705 0.225 264);
    --color-accent-fg:     oklch(0.180 0.012 264);
    --color-accent-hover:  oklch(0.760 0.215 264);
    --color-success:       oklch(0.760 0.185 150);
    --color-warning:       oklch(0.840 0.185 82);
    --color-danger:        oklch(0.710 0.250 27);
    --color-ring:          oklch(0.705 0.225 264);
  }
}

/* ============================================================
   P3 UPGRADE — push chroma past the sRGB boundary on capable
   displays. Only the chromatic roles change; neutrals stay put.
   ============================================================ */
@supports (color: color(display-p3 1 1 1)) {
  :root {
    --color-accent: oklch(0.637 0.250 264);
    --color-danger: oklch(0.637 0.290 25);
  }
  :root[data-theme="dark"],
  .dark {
    --color-accent: oklch(0.705 0.280 264);
    --color-danger: oklch(0.710 0.300 27);
  }
}
```

### Dark mode is a designed context, not an inversion

Naively lifting light-mode colors onto dark surfaces makes them look washed-out. Two
perceptual facts drive the adjustments baked into the block above:

- **Simultaneous contrast** — a hue on near-black reads dimmer than on white, so **boost
  chroma ~15–25%** to keep perceived intensity (see accent: `C 0.200 → 0.225`).
- **Bezold-Brücke hue shift** — warm hues drift at low luminance, so **rotate hue slightly**
  toward the source (warning `84 → 82`, danger `25 → 27`).

**Elevation in dark = lighter surfaces, not shadows.** Raise `L` per layer
(`bg 0.180 → surface 0.215 → surface-2 0.255`); shadows barely read on dark backgrounds.
Re-verify contrast in *both* themes and lean on APCA in dark, since WCAG 2 over-rates dark pairs.

---

## 6. Composing a cohesive palette — dominant + sharp accent

Timid, evenly-weighted palettes read as generic. Compose a hierarchy:

1. **Pick the dominant hue `H`.** Base `L 0.55–0.65`, `C` as high as stays in gamut. *(indigo 264)*
2. **Build its tonal scale** by walking `L` (50→950), tapering `C` at the extremes (§2).
3. **Derive neutrals from the same hue** at `C ≈ 0.004–0.02` → grays feel related, not flat.
4. **Choose ONE sharp accent** — a complement (`H+180`) or strong analog — at *higher chroma /
   contrasting L* than the dominant so it visibly pops. *(amber 84 vs cool indigo.)* Use sparingly.
5. **Add functional hues** (danger ≈ 25, warning ≈ 80, success ≈ 150, info ≈ 240) at matched
   `L`/`C` so they share visual weight with the brand.
6. **Verify** every text/surface pair: WCAG 2 AA floor, then APCA Lc tune (§4).

> **60-30-10 in practice:** ~60% neutral surfaces, ~30% dominant brand, ~10% sharp accent.
> Keep accent chroma clearly above the dominant's so the eye always knows where to look.
> One dominant color + sharp accents beats a timid even palette — restraint reads as confidence.

---

## 7. Tailwind v4 `@theme` mirror

Tailwind v4 is CSS-first: tokens in `@theme` become build-time custom properties **and**
utilities. Its entire default palette is OKLCH; v4.1+ emits sRGB fallbacks. Mirror the role
contract with `@theme inline` so `bg-surface`, `text-fg-muted`, `ring-ring`, etc. all exist.

```css
@import "tailwindcss";

/* Primitives → generate bg-indigo-500, text-amber-500, … */
@theme {
  --color-indigo-50:  oklch(0.971 0.013 264);
  --color-indigo-500: oklch(0.637 0.200 264);
  --color-indigo-600: oklch(0.560 0.190 264);
  --color-indigo-700: oklch(0.476 0.160 264);
  --color-indigo-950: oklch(0.243 0.055 264);
  --color-amber-500:  oklch(0.780 0.160 84);
}

/* Roles map to live vars → utilities follow the theme automatically.
   `inline` means Tailwind emits var(--color-surface) literally, so a
   runtime [data-theme] swap re-themes every utility with zero rebuild. */
@theme inline {
  --color-bg:            var(--bg);
  --color-bg-subtle:     var(--bg-subtle);
  --color-surface:       var(--surface);
  --color-surface-2:     var(--surface-2);
  --color-fg:            var(--fg);
  --color-fg-muted:      var(--fg-muted);
  --color-fg-subtle:     var(--fg-subtle);
  --color-border:        var(--border);
  --color-border-subtle: var(--border-subtle);
  --color-accent:        var(--accent);
  --color-accent-fg:     var(--accent-fg);
  --color-accent-hover:  var(--accent-hover);
  --color-success:       var(--success);
  --color-warning:       var(--warning);
  --color-danger:        var(--danger);
  --color-ring:          var(--ring);
}

/* The actual values live on plain selectors so they can switch at runtime */
:root {
  --bg: oklch(0.985 0.004 264); --bg-subtle: oklch(0.967 0.006 264);
  --surface: oklch(1 0 0);      --surface-2: oklch(0.985 0.004 264);
  --fg: oklch(0.210 0.012 264); --fg-muted: oklch(0.440 0.014 264);
  --fg-subtle: oklch(0.556 0.014 264);
  --border: oklch(0.922 0.008 264); --border-subtle: oklch(0.967 0.006 264);
  --accent: oklch(0.560 0.190 264); --accent-fg: oklch(1 0 0);
  --accent-hover: oklch(0.476 0.160 264);
  --success: oklch(0.700 0.160 150); --warning: oklch(0.800 0.160 80);
  --danger: oklch(0.637 0.237 25);   --ring: oklch(0.560 0.190 264);
}
:root[data-theme="dark"], .dark {
  --bg: oklch(0.180 0.012 264); --bg-subtle: oklch(0.150 0.010 264);
  --surface: oklch(0.215 0.013 264); --surface-2: oklch(0.255 0.014 264);
  --fg: oklch(0.945 0.010 264); --fg-muted: oklch(0.760 0.014 264);
  --fg-subtle: oklch(0.620 0.014 264);
  --border: oklch(0.300 0.014 264); --border-subtle: oklch(0.255 0.012 264);
  --accent: oklch(0.705 0.225 264); --accent-fg: oklch(0.180 0.012 264);
  --accent-hover: oklch(0.760 0.215 264);
  --success: oklch(0.760 0.185 150); --warning: oklch(0.840 0.185 82);
  --danger: oklch(0.710 0.250 27);   --ring: oklch(0.705 0.225 264);
}
```

Gradient interpolation defaults to OKLAB; opt per-utility into `bg-linear-to-r/oklch` (vivid
for far-apart hues) or `/srgb`. Disable a built-in with `--color-red-*: initial`; full takeover
with `--color-*: initial`.

---

## 8. React 19 reference: a themed, accessible Button

Framework-agnostic CSS comes first; this is the parallel reference, not a replacement. It uses
only role tokens, supports a no-flash theme toggle, and never hardcodes a hue.

```tsx
"use client";

import { useState, useEffect } from "react";

type Variant = "accent" | "danger" | "ghost";

const variants: Record<Variant, string> = {
  // role tokens only — re-themes for free in dark mode
  accent: "bg-accent text-accent-fg hover:bg-accent-hover",
  danger: "bg-danger text-danger-fg hover:brightness-110",
  ghost:  "bg-transparent text-fg hover:bg-bg-subtle border border-border",
};

export function Button({
  variant = "accent",
  className = "",
  ...props
}: React.ButtonHTMLAttributes<HTMLButtonElement> & { variant?: Variant }) {
  return (
    <button
      className={[
        "inline-flex items-center justify-center rounded-lg px-4 py-2",
        "text-sm font-medium transition-colors duration-150 ease-out",
        // focus ring uses the semantic ring token, offset against the page bg
        "outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 focus-visible:ring-offset-bg",
        "disabled:opacity-50 disabled:pointer-events-none",
        variants[variant],
        className,
      ].join(" ")}
      {...props}
    />
  );
}

/** No-flash theme toggle: write [data-theme] before paint via inline script in <head>. */
export function useTheme() {
  const [theme, setTheme] = useState<"light" | "dark">("light");
  useEffect(() => {
    const saved = localStorage.getItem("theme") as "light" | "dark" | null;
    const sys = matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
    const next = saved ?? sys;
    document.documentElement.dataset.theme = next;
    setTheme(next);
  }, []);
  const toggle = () => {
    const next = theme === "dark" ? "light" : "dark";
    document.documentElement.dataset.theme = next;
    localStorage.setItem("theme", next);
    setTheme(next);
  };
  return { theme, toggle };
}
```

```html
<!-- Put in <head> BEFORE styles to prevent a light→dark flash on load -->
<script>
  (function () {
    var t = localStorage.getItem("theme")
      || (matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light");
    document.documentElement.dataset.theme = t;
  })();
</script>
```

---

## 9. State colors: error / empty / loading

Color must carry these states explicitly — never leave them to defaults.

```css
/* Error: danger surface tint + danger text, never red text on white alone */
.field-error {
  border-color: var(--color-danger);
  background: color-mix(in oklab, var(--color-danger) 8%, var(--color-surface));
  color: var(--color-danger);
}

/* Empty: muted, low-emphasis — recede, don't shout */
.empty-state { color: var(--color-fg-muted); background: var(--color-bg-subtle); }

/* Loading skeleton: shimmer between two surface steps (respect reduced motion) */
.skeleton {
  background: linear-gradient(90deg,
    var(--color-bg-subtle) 25%,
    color-mix(in oklab, var(--color-fg) 6%, var(--color-bg-subtle)) 50%,
    var(--color-bg-subtle) 75%);
  background-size: 200% 100%;
  animation: skeleton 1.4s ease-in-out infinite;
}
@keyframes skeleton { to { background-position: -200% 0; } }
@media (prefers-reduced-motion: reduce) { .skeleton { animation: none; } }
```

> Never signal state by **color alone** (WCAG 1.4.1). Pair danger with an icon/label, success
> with a checkmark — color reinforces, it does not encode.

---

## Quick reference

| Task | Reach for |
|------|-----------|
| Author or scale a color | `oklch(L C H)`, walk L, taper C |
| Derive a hover/shade | `oklch(from … calc(l * 0.9) c h)` |
| Blend / tint / scrim | `color-mix(in oklab, …)` |
| Auto text on a fill | `--color-accent-fg` + `@supports contrast-color()` |
| Add P3 vibrancy | sRGB default, upgrade under `@supports (color: color(display-p3 …))` |
| Theme a component | semantic role token (`--color-surface`, `--color-fg`, `--color-accent`) |
| Verify a pair | WCAG 2 AA floor (4.5:1 / 3:1), then APCA Lc 75/60/45 |
| Add a brand color | new OKLCH scale + new role tokens; never hardcode in a component |

---

## Sources

- MDN — `oklch()` (CSS Color 4) — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/oklch
- MDN — `color()` / display-p3 — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/color
- MDN — `contrast-color()` — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/contrast-color
- web.dev — Color themes with Baseline CSS (light-dark, relative color, color-mix) — https://web.dev/articles/baseline-in-action-color-theme
- WebKit Blog — Wide Gamut Color in CSS with Display-P3 — https://webkit.org/blog/10042/wide-gamut-color-in-css-with-display-p3/
- Chrome for Developers — Migrate to HD CSS color — https://developer.chrome.com/docs/css-ui/migrate-hd-color
- oklch.com — OKLCH Color Picker & Converter — https://oklch.com/
- Tailwind CSS — Colors (v4 OKLCH palette) — https://tailwindcss.com/docs/colors
- Tailwind CSS — Theme variables (`@theme`) — https://tailwindcss.com/docs/theme
- Tailwind CSS v4.0 release notes — https://tailwindcss.com/blog/tailwindcss-v4
- APCA — APCA in a Nutshell (Lc levels) — https://git.apcacontrast.com/documentation/APCA_in_a_Nutshell.html
- APCA — Why APCA as a New Contrast Method? — https://git.apcacontrast.com/documentation/WhyAPCA.html
- Adrian Roselli — WCAG3 Contrast as of April 2026 — https://adrianroselli.com/2026/04/wcag3-contrast-as-of-april-2026.html
- Smashing Magazine — Building Self-Correcting Color Systems With contrast-color() — https://www.smashingmagazine.com/2026/05/building-self-correcting-color-systems-contrast-color/
- Humbl Design — 2026 Engineering Guide to Color & Contrast (WCAG 2.2 + APCA) — https://humbldesign.io/blog-posts/color-accessibility-guide-wcag
- ColorArchive — OKLCH Perceptual Color Design Guide — https://colorarchive.org/guides/oklch-perceptual-color-design-guide/
- ColorArchive — Why dark mode colors need more saturation — https://colorarchive.org/notes/may-2026-dark-mode-saturation/
- Muzli — Dark Mode Design Systems: Patterns, Tokens, Hierarchy — https://muz.li/blog/dark-mode-design-systems-a-complete-guide-to-patterns-tokens-and-hierarchy/
- Evil Martians — Better dynamic themes in Tailwind with OKLCH — https://evilmartians.com/chronicles/better-dynamic-themes-in-tailwind-with-oklch-color-magic
