---
title: "Design Tokens — The Canonical Token Contract"
description: "The single source of truth for AwesomeDesignSystem. Framework-agnostic CSS custom properties (OKLCH color, fluid type, 8pt space, motion, elevation) mirrored in Tailwind v4 @theme, with dark-mode overrides. AI-agent-first: copy-paste, no fetching."
updated: 2026-06-09
tags: [design-tokens, oklch, tailwind-v4, dark-mode, foundations, css-variables]
status: canonical
---

# Design Tokens — The Canonical Token Contract

> This is **the** file every other module references. Buttons, cards, dialogs, type
> ramps — all of them consume the names defined here, never raw values. Theming
> (light → dark → high-contrast) is a *values swap on stable names*, not a rewrite.
> Author colors in **OKLCH** (perceptually uniform), express depth with **1px hairline
> borders + layered low-opacity shadows**, and animate with **chosen easing + duration**.

Every mature system — Material 3 (`ref/sys/comp`), Fluent 2 (`global/alias/control`),
Primer (`base/functional/component`) — converges on the same three-tier model. We adopt
it with a flat, framework-agnostic CSS-variable surface so an AI agent can paste one
`:root {}` block and ship.

---

## 1. The Three-Tier Token Architecture

```
TIER 1 — Primitive        TIER 2 — Semantic (role)      TIER 3 — Component
raw, hue-anchored values  what components consume        per-component overrides
--brand-500               --color-accent                 --button-bg
--neutral-50              --color-surface                --card-bg
oklch(0.637 0.20 264)     var(--brand-500)               var(--color-surface)
```

| Tier | Role | Who reads it | Changes when |
|------|------|--------------|--------------|
| **Primitive** | Raw OKLCH scales, raw px/rem steps. No meaning. | Only semantic tokens. | You rebrand (new hue). |
| **Semantic** | A *role*: `--color-surface`, `--color-accent`, `--space-4`. | **Components.** | You theme (light↔dark). |
| **Component** | Per-component knobs that point at semantic tokens. | One component's CSS. | You tune one component. |

**The one rule that makes theming free:** components reference **only** semantic (or
component) tokens — never primitives. Then light/dark/high-contrast is a values table
rebinding the *same* names. This is exactly how M3 Dynamic Color re-themes an entire UI
from a new wallpaper: `ref` tokens regenerate, `sys`/`comp` names are untouched.

> **Naming convention.** Semantic color tokens are `--color-<role>`; a foreground that
> sits *on* a colored surface is `--color-<role>-fg`. Scales count up in intensity
> (`-subtle` < base < `-hover`). Space/radius/type use numeric or t-shirt steps. Keep
> the vocabulary small and boring — predictability beats cleverness for both humans and agents.

---

## 2. Color — OKLCH primitives → semantic roles

OKLCH (`oklch(L C H)`) is perceptually uniform: equal `L` = equal perceived brightness
across hues, so even-`L` steps *look* even and dark mode becomes principled math, not
guesswork. We anchor a **dominant** hue (indigo, H 264) carrying ~30% of the UI, derive
**neutrals from the same hue** at near-zero chroma so grays feel related, add **one sharp
accent** (amber, H 84) reserved for ~10% emphasis, and match functional hues (danger 25,
warning 80, success 150, info 240) to the brand's visual weight.

> **Chroma taper rule:** chroma peaks in the 400–600 lightness band and **must taper**
> toward 50 and 950 or the extremes clip out of sRGB/P3 gamut.

### Tier 1 — Primitive scales (the `:root` opens here)

```css
:root {
  color-scheme: light dark;

  /* ── BRAND (dominant, indigo H 264) ────────────────────────────── */
  --brand-50:  oklch(0.971 0.013 264);
  --brand-100: oklch(0.936 0.032 264);
  --brand-200: oklch(0.885 0.062 264);
  --brand-300: oklch(0.808 0.105 264);
  --brand-400: oklch(0.704 0.165 264);
  --brand-500: oklch(0.637 0.200 264); /* base */
  --brand-600: oklch(0.560 0.190 264);
  --brand-700: oklch(0.476 0.160 264);
  --brand-800: oklch(0.398 0.125 264);
  --brand-900: oklch(0.340 0.090 264);
  --brand-950: oklch(0.243 0.055 264);

  /* ── ACCENT (sharp, amber H 84 — complement-ish, higher chroma) ── */
  --accent-50:  oklch(0.980 0.020 84);
  --accent-100: oklch(0.955 0.045 84);
  --accent-300: oklch(0.880 0.130 84);
  --accent-500: oklch(0.780 0.210 84); /* base — pops vs cool indigo */
  --accent-600: oklch(0.700 0.190 84);
  --accent-700: oklch(0.600 0.150 84);

  /* ── NEUTRALS (borrow brand hue 264, near-zero chroma) ─────────── */
  --neutral-0:   oklch(1     0     0);
  --neutral-50:  oklch(0.985 0.004 264);
  --neutral-100: oklch(0.967 0.006 264);
  --neutral-200: oklch(0.928 0.008 264);
  --neutral-300: oklch(0.872 0.010 264);
  --neutral-400: oklch(0.715 0.012 264);
  --neutral-500: oklch(0.556 0.013 264);
  --neutral-600: oklch(0.446 0.013 264);
  --neutral-700: oklch(0.372 0.012 264);
  --neutral-800: oklch(0.279 0.011 264);
  --neutral-900: oklch(0.210 0.012 264);
  --neutral-950: oklch(0.145 0.010 264);

  /* ── FUNCTIONAL hues (matched L/C to share brand weight) ───────── */
  --success-500: oklch(0.700 0.160 150);
  --success-600: oklch(0.610 0.150 150);
  --warning-500: oklch(0.800 0.160 80);
  --warning-600: oklch(0.720 0.155 80);
  --danger-500:  oklch(0.637 0.237 25);
  --danger-600:  oklch(0.560 0.225 25);
  --info-500:    oklch(0.660 0.160 240);
  --info-600:    oklch(0.580 0.150 240);
```

The primitive block stays open — semantic roles follow in the same `:root` (next section)
so a single paste yields a working theme.

### Tier 2 — Semantic color roles (the contract surface)

These are **the names every component and module consumes** — `--color-bg`, `--color-accent`,
`--color-ring`, and so on. Dark mode rebinds *these exact names* (§8). They point at primitives
in light mode; the values flip per theme, the names never do.

```css
  /* ── SURFACES (60% of UI — neutral, layered) ───────────────────── */
  --color-bg:           var(--neutral-50);   /* page background */
  --color-bg-subtle:    var(--neutral-100);  /* sunken wells, code blocks */
  --color-surface:      var(--neutral-0);    /* cards, panels */
  --color-surface-2:    var(--neutral-100);  /* raised-on-surface (menus, popovers) */

  /* ── FOREGROUNDS (text/icons by emphasis) ──────────────────────── */
  --color-fg:        var(--neutral-900);  /* primary text — AA on bg */
  --color-fg-muted:  var(--neutral-600);  /* secondary text, captions */
  --color-fg-subtle: var(--neutral-400);  /* placeholder, disabled, hints */

  /* ── BORDERS (hairlines — our primary depth cue) ───────────────── */
  --color-border:        var(--neutral-200);
  --color-border-subtle: var(--neutral-100);

  /* ── ACCENT (the brand action color — ~30% dominant) ───────────── */
  --color-accent:       var(--brand-500);
  --color-accent-fg:    var(--neutral-0);   /* text/icon ON accent */
  --color-accent-hover: var(--brand-600);

  /* ── STATUS (semantic feedback; each has a matching -fg) ────────── */
  --color-success:    var(--success-500);
  --color-success-fg: var(--neutral-0);
  --color-warning:    var(--warning-500);
  --color-warning-fg: var(--neutral-950); /* dark text on bright amber */
  --color-danger:     var(--danger-500);
  --color-danger-fg:  var(--neutral-0);

  /* ── FOCUS RING (WCAG 2.4.7 + 1.4.11 ≥3:1 non-text) ────────────── */
  --color-ring: var(--brand-500);
```

> **Tailwind v4 caveat (read before §9).** Tailwind generates color utilities (`bg-accent`,
> `text-fg`) from `--color-*` tokens declared in `@theme`. A token **cannot reference itself**
> (`--color-bg: var(--color-bg)` is invalid). So when you also use Tailwind, define this Tier-2
> layer under a neutral **`--ds-*`** alias in `:root`/`.dark` (e.g. `--ds-bg`, `--ds-accent`),
> and let `@theme inline` map `--color-bg: var(--ds-bg)`. In **plain CSS** (no Tailwind) you can
> keep `--color-*` as the runtime names directly. Both paths are shown — pick one and stay
> consistent. The `--ds-*` path is what makes a single `.dark` class re-theme every utility.

---

## 3. Space — 8pt-based scale (rem)

One rhythm for intra-component padding *and* inter-component layout. `--space-1 = 0.25rem
(4px)` is the half-step; the system is built on multiples of 8 with 4px micro-steps. Tokens
do **not** change with viewport — you *jump steps* at breakpoints.

```css
  /* ── SPACE (rem; 1rem = 16px) ──────────────────────────────────── */
  --space-0:   0;
  --space-px:  1px;
  --space-1:   0.25rem;  /*  4px */
  --space-2:   0.5rem;   /*  8px */
  --space-3:   0.75rem;  /* 12px */
  --space-4:   1rem;     /* 16px */
  --space-5:   1.25rem;  /* 20px */
  --space-6:   1.5rem;   /* 24px */
  --space-8:   2rem;     /* 32px */
  --space-10:  2.5rem;   /* 40px */
  --space-12:  3rem;     /* 48px */
  --space-16:  4rem;     /* 64px */
  --space-20:  5rem;     /* 80px */
  --space-24:  6rem;     /* 96px */
  --space-32:  8rem;     /* 128px */
```

---

## 4. Radius — corner scale

A *deliberate* radius hierarchy beats a uniform 16px on everything. Small controls get
small radii; overlays/large cards get larger; pills/avatars use `full`. **Concentricity
(Apple HIG):** a nested element's radius ≈ parent radius − padding, so curves stay parallel.

```css
  /* ── RADIUS ────────────────────────────────────────────────────── */
  --radius-sm:   0.25rem;  /*  4px — inputs, chips, small buttons */
  --radius-md:   0.5rem;   /*  8px — buttons, default */
  --radius-lg:   0.75rem;  /* 12px — cards */
  --radius-xl:   1rem;     /* 16px — modals, large panels */
  --radius-2xl:  1.5rem;   /* 24px — hero / feature surfaces */
  --radius-full: 9999px;   /* pills, avatars, switches */
```

---

## 5. Typography — fluid scale, leading, tracking, fonts

Fluid `clamp(MIN, rem + vw, MAX)` — the `rem` term keeps text responsive to user zoom
(WCAG 1.4.4; passes 200% zoom). Body floors at **1rem (16px)**, max ≤ ~2.5× min. Leading
and tracking are **size-dependent**: large text wants tighter leading + negative tracking;
small text wants looser + positive. We pair **one display + one body + one mono** — escape
the Inter-only default.

```css
  /* ── FONT FAMILIES (wire to next/font CSS vars; metric-matched fallbacks) ─ */
  --font-display: "Bricolage Grotesque", var(--font-body), sans-serif;
  --font-body:    "Satoshi", system-ui, -apple-system, "Segoe UI", sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;

  /* ── FLUID TYPE SCALE (clamp: min → max across viewport) ───────── */
  --text-xs:   clamp(0.75rem, 0.72rem + 0.15vw, 0.83rem);  /* 12→13px */
  --text-sm:   clamp(0.875rem, 0.84rem + 0.18vw, 0.95rem); /* 14→15px */
  --text-base: clamp(1rem, 0.91rem + 0.43vw, 1.125rem);    /* 16→18px — body */
  --text-lg:   clamp(1.125rem, 1.02rem + 0.52vw, 1.35rem); /* 18→22px */
  --text-xl:   clamp(1.35rem, 1.20rem + 0.75vw, 1.62rem);
  --text-2xl:  clamp(1.62rem, 1.40rem + 1.10vw, 2.05rem);
  --text-3xl:  clamp(1.95rem, 1.64rem + 1.55vw, 2.60rem);
  --text-4xl:  clamp(2.34rem, 1.90rem + 2.20vw, 3.30rem);
  --text-5xl:  clamp(2.80rem, 2.18rem + 3.10vw, 4.20rem);
  --text-6xl:  clamp(3.36rem, 2.48rem + 4.40vw, 5.30rem);
  --text-7xl:  clamp(4.00rem, 2.74rem + 6.30vw, 6.75rem);  /* display */

  /* ── LINE HEIGHT (tighter as size grows) ───────────────────────── */
  --leading-none:    1;
  --leading-tight:   1.15;  /* display / h1 */
  --leading-snug:    1.3;   /* h2 / h3 */
  --leading-normal:  1.5;   /* UI default */
  --leading-relaxed: 1.65;  /* long-form body */
  --leading-ja:      1.8;   /* 和文 wants 1.6–1.9 */

  /* ── LETTER SPACING (negative for display, positive for caps/small) ─ */
  --tracking-tighter: -0.03em; /* big display */
  --tracking-tight:   -0.015em;/* headings */
  --tracking-normal:  0;       /* body */
  --tracking-wide:    0.02em;  /* small / captions */
  --tracking-caps:    0.08em;  /* all-caps eyebrows/labels */

  /* ── FONT WEIGHTS (semantic; variable-font friendly) ───────────── */
  --weight-normal:   400;
  --weight-medium:   500;
  --weight-semibold: 600;
  --weight-bold:     700;
```

(Still inside the open `:root {}` — `--space-*`, `--radius-*`, and the type tokens above all
live in the same block; it closes at the end of §7.)

---

## 6. Elevation — hairline border + layered low-opacity shadows

Default depth is a **1px hairline border**, not a heavy drop shadow. Where a surface truly
floats (menu, popover, dialog), add a **layered, low-opacity** shadow — two stacked layers
(a tight contact shadow + a soft ambient one) read far more naturally than one blurry box.
In **dark mode**, shadows barely register, so depth is expressed by *raising surface
lightness* per layer instead (see §8).

```css
  /* ── SHADOWS (layered, low-opacity; tuned for light mode) ──────── */
  --shadow-sm:
    0 1px 2px -1px oklch(0 0 0 / 0.08),
    0 1px 1px    oklch(0 0 0 / 0.04);
  --shadow-md:
    0 2px 4px -2px oklch(0 0 0 / 0.10),
    0 4px 8px -2px oklch(0 0 0 / 0.06);
  --shadow-lg:
    0 4px 8px  -3px oklch(0 0 0 / 0.10),
    0 12px 20px -4px oklch(0 0 0 / 0.08);
  --shadow-xl:
    0 8px 16px -4px oklch(0 0 0 / 0.12),
    0 24px 40px -8px oklch(0 0 0 / 0.10);
```

---

## 7. Motion & Z-index

Motion communicates **state/attention/personality** — never decoration. Pair a chosen
**easing** with a chosen **duration**. `--ease-spring` is a CSS `linear()` approximation of
a lightly-damped spring (the modern way to get spring feel in pure CSS, echoing M3
Expressive / Apple's spring defaults). Always honor `prefers-reduced-motion`.

```css
  /* ── EASING ────────────────────────────────────────────────────── */
  --ease-out:     cubic-bezier(0.2, 0, 0, 1);      /* entering / standard-decelerate */
  --ease-in-out:  cubic-bezier(0.4, 0, 0.2, 1);    /* moving on-screen */
  /* spring approx: slight overshoot then settle (linear() — 22 samples) */
  --ease-spring:  linear(
    0, 0.006, 0.025, 0.101, 0.539, 0.802, 0.954, 1.041, 1.078,
    1.085, 1.072, 1.054, 1.034, 1.018, 1.006, 0.999, 0.997,
    0.998, 1.0, 1.001, 1.001, 1);

  /* ── DURATION ──────────────────────────────────────────────────── */
  --dur-fast: 150ms;  /* hovers, small state changes */
  --dur-base: 250ms;  /* most transitions, enters */
  --dur-slow: 400ms;  /* large surfaces, page-load orchestration */

  /* ── Z-INDEX (named stacking order) ────────────────────────────── */
  --z-base:     0;
  --z-dropdown: 1000;
  --z-sticky:   1100;
  --z-overlay:  1200;  /* scrims / backdrops */
  --z-modal:    1300;
  --z-popover:  1400;
  --z-toast:    1500;
  --z-tooltip:  1600;
}
```

That closes the `:root {}`. Pasting §2–§7 verbatim gives a complete, working light theme.

```css
/* Global reduced-motion guard — collapse durations everywhere */
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

---

## 8. Dark Mode — adjust L & C, never naive invert

Dark is a **first-class context**, not flipped light. Two perceptual facts drive the values:

1. **Simultaneous contrast** — a hue on near-black reads dimmer than on white, so we **boost
   chroma ~15–25%** on interactive/status colors to keep perceived intensity.
2. **Dark-mode elevation = lighter surface, not shadow** — depth is a *rising L* per layer
   (`bg 0.18 → surface 0.21 → surface-2 0.25`), because shadows barely read on dark.

Only the **semantic** names are rebound — components never change. Re-verify contrast in
*both* themes; WCAG 2 over-rates dark pairs, so lean on APCA there.

```css
/* Toggleable (attribute or class) — does not depend on OS preference */
[data-theme="dark"],
.dark {
  /* SURFACES — rise in L to express elevation */
  --color-bg:           oklch(0.180 0.014 264);
  --color-bg-subtle:    oklch(0.150 0.012 264);
  --color-surface:      oklch(0.210 0.015 264);
  --color-surface-2:    oklch(0.250 0.016 264);

  /* FOREGROUNDS — high-L, low-C; not pure white (reduces halation) */
  --color-fg:        oklch(0.945 0.010 264);
  --color-fg-muted:  oklch(0.730 0.014 264);
  --color-fg-subtle: oklch(0.560 0.014 264);

  /* BORDERS — lift, don't darken */
  --color-border:        oklch(0.310 0.016 264);
  --color-border-subtle: oklch(0.250 0.014 264);

  /* ACCENT — +L and +C so it stays vivid on dark */
  --color-accent:       oklch(0.720 0.225 264);
  --color-accent-fg:    oklch(0.180 0.014 264);
  --color-accent-hover: oklch(0.785 0.215 264);

  /* STATUS — +L/+C; danger nudges hue toward source (Bezold-Brücke) */
  --color-success:    oklch(0.760 0.180 150);
  --color-success-fg: oklch(0.180 0.014 264);
  --color-warning:    oklch(0.850 0.175 80);
  --color-warning-fg: oklch(0.180 0.014 264);
  --color-danger:     oklch(0.710 0.250 27);
  --color-danger-fg:  oklch(0.180 0.014 264);

  /* FOCUS RING — brighter so it survives the dark surface */
  --color-ring: oklch(0.760 0.215 264);

  /* SHADOWS — deeper, but secondary to the surface-L cue */
  --shadow-sm: 0 1px 2px -1px oklch(0 0 0 / 0.40), 0 1px 1px oklch(0 0 0 / 0.30);
  --shadow-md: 0 2px 4px -2px oklch(0 0 0 / 0.45), 0 4px 8px -2px oklch(0 0 0 / 0.30);
  --shadow-lg: 0 4px 8px -3px oklch(0 0 0 / 0.50), 0 12px 20px -4px oklch(0 0 0 / 0.35);
  --shadow-xl: 0 8px 16px -4px oklch(0 0 0 / 0.55), 0 24px 40px -8px oklch(0 0 0 / 0.40);
}

/* Honor OS preference as the DEFAULT, unless an explicit [data-theme] wins */
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    /* duplicate the dark block above, or @apply via a shared custom-media build step */
    color-scheme: dark;
  }
}
```

> **Build tip.** To avoid duplicating the dark block, generate it with a single source and
> emit both the `.dark` selector and the `@media (prefers-color-scheme: dark)` variant, or
> author once with CSS `light-dark(light-val, dark-val)` per token and toggle `color-scheme`.
> The attribute/class form above is the most portable and gives users a manual override.

---

## 9. Tailwind v4 `@theme` Mirror

Tailwind v4 is CSS-first: tokens declared in `@theme` become both build-time custom
properties **and** utilities (`bg-accent`, `text-fg-muted`, `p-4`, `rounded-lg`,
`shadow-md`, `ease-spring`, `duration-base`). For the **semantic color layer** we move the
runtime values to a neutral **`--ds-*`** alias in `:root`/`.dark`, then use **`@theme inline`**
to map `--color-* → var(--ds-*)`. This is the documented Tailwind v4 pattern for tokens that
reference other variables — and it means a single `.dark` class re-themes every Tailwind
utility for free (no `dark:` variant spam, no self-referential tokens).

```css
/* app/globals.css */
@import "tailwindcss";

/* 1. Primitives + scalar tokens live in :root (paste §2–§7 here). */

/* 2. Semantic runtime layer under --ds-* (these flip in .dark; see §8). */
:root {
  --ds-bg: var(--neutral-50);      --ds-bg-subtle: var(--neutral-100);
  --ds-surface: var(--neutral-0);  --ds-surface-2: var(--neutral-100);
  --ds-fg: var(--neutral-900);     --ds-fg-muted: var(--neutral-600);
  --ds-fg-subtle: var(--neutral-400);
  --ds-border: var(--neutral-200); --ds-border-subtle: var(--neutral-100);
  --ds-accent: var(--brand-500);   --ds-accent-fg: var(--neutral-0);
  --ds-accent-hover: var(--brand-600);
  --ds-success: var(--success-500);   --ds-success-fg: var(--neutral-0);
  --ds-warning: var(--warning-500);   --ds-warning-fg: var(--neutral-950);
  --ds-danger: var(--danger-500);     --ds-danger-fg: var(--neutral-0);
  --ds-ring: var(--brand-500);
}
[data-theme="dark"], .dark {
  --ds-bg: oklch(0.180 0.014 264);    --ds-bg-subtle: oklch(0.150 0.012 264);
  --ds-surface: oklch(0.210 0.015 264); --ds-surface-2: oklch(0.250 0.016 264);
  --ds-fg: oklch(0.945 0.010 264);    --ds-fg-muted: oklch(0.730 0.014 264);
  --ds-fg-subtle: oklch(0.560 0.014 264);
  --ds-border: oklch(0.310 0.016 264); --ds-border-subtle: oklch(0.250 0.014 264);
  --ds-accent: oklch(0.720 0.225 264); --ds-accent-fg: oklch(0.180 0.014 264);
  --ds-accent-hover: oklch(0.785 0.215 264);
  --ds-success: oklch(0.760 0.180 150); --ds-success-fg: oklch(0.180 0.014 264);
  --ds-warning: oklch(0.850 0.175 80);  --ds-warning-fg: oklch(0.180 0.014 264);
  --ds-danger: oklch(0.710 0.250 27);   --ds-danger-fg: oklch(0.180 0.014 264);
  --ds-ring: oklch(0.760 0.215 264);
}

/* 3. Map the canonical --color-* contract onto the --ds-* runtime layer.
   `inline` keeps the var() reference live → utilities update under .dark. */
@theme inline {
  /* ── COLOR (utilities: bg-bg, bg-surface, text-fg, border-border, …) ── */
  --color-bg:            var(--ds-bg);
  --color-bg-subtle:     var(--ds-bg-subtle);
  --color-surface:       var(--ds-surface);
  --color-surface-2:     var(--ds-surface-2);
  --color-fg:            var(--ds-fg);
  --color-fg-muted:      var(--ds-fg-muted);
  --color-fg-subtle:     var(--ds-fg-subtle);
  --color-border:        var(--ds-border);
  --color-border-subtle: var(--ds-border-subtle);
  --color-accent:        var(--ds-accent);
  --color-accent-fg:     var(--ds-accent-fg);
  --color-accent-hover:  var(--ds-accent-hover);
  --color-success:       var(--ds-success);
  --color-success-fg:    var(--ds-success-fg);
  --color-warning:       var(--ds-warning);
  --color-warning-fg:    var(--ds-warning-fg);
  --color-danger:        var(--ds-danger);
  --color-danger-fg:     var(--ds-danger-fg);
  --color-ring:          var(--ds-ring);
}

@theme {
  /* ── FONT FAMILIES (font-display / font-body / font-mono) ──────── */
  --font-display: "Bricolage Grotesque", var(--font-body), sans-serif;
  --font-body:    "Satoshi", system-ui, -apple-system, "Segoe UI", sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;

  /* ── FLUID TYPE (text-base, text-3xl, …) ───────────────────────── */
  --text-xs:   clamp(0.75rem, 0.72rem + 0.15vw, 0.83rem);
  --text-sm:   clamp(0.875rem, 0.84rem + 0.18vw, 0.95rem);
  --text-base: clamp(1rem, 0.91rem + 0.43vw, 1.125rem);
  --text-lg:   clamp(1.125rem, 1.02rem + 0.52vw, 1.35rem);
  --text-xl:   clamp(1.35rem, 1.20rem + 0.75vw, 1.62rem);
  --text-2xl:  clamp(1.62rem, 1.40rem + 1.10vw, 2.05rem);
  --text-3xl:  clamp(1.95rem, 1.64rem + 1.55vw, 2.60rem);
  --text-4xl:  clamp(2.34rem, 1.90rem + 2.20vw, 3.30rem);
  --text-5xl:  clamp(2.80rem, 2.18rem + 3.10vw, 4.20rem);
  --text-6xl:  clamp(3.36rem, 2.48rem + 4.40vw, 5.30rem);
  --text-7xl:  clamp(4.00rem, 2.74rem + 6.30vw, 6.75rem);

  /* ── LEADING / TRACKING (leading-tight, tracking-caps, …) ──────── */
  --leading-none:    1;
  --leading-tight:   1.15;
  --leading-snug:    1.3;
  --leading-normal:  1.5;
  --leading-relaxed: 1.65;
  --tracking-tighter: -0.03em;
  --tracking-tight:   -0.015em;
  --tracking-normal:  0;
  --tracking-wide:    0.02em;
  --tracking-caps:    0.08em;

  /* ── SPACING (p-1 … p-32; Tailwind generates the in-between too) ── */
  --spacing: 0.25rem; /* base step → space-* scale derives from this in v4 */

  /* ── RADIUS (rounded-sm … rounded-2xl, rounded-full) ───────────── */
  --radius-sm:   0.25rem;
  --radius-md:   0.5rem;
  --radius-lg:   0.75rem;
  --radius-xl:   1rem;
  --radius-2xl:  1.5rem;

  /* ── SHADOW (shadow-sm … shadow-xl) ────────────────────────────── */
  --shadow-sm: 0 1px 2px -1px oklch(0 0 0 / 0.08), 0 1px 1px oklch(0 0 0 / 0.04);
  --shadow-md: 0 2px 4px -2px oklch(0 0 0 / 0.10), 0 4px 8px -2px oklch(0 0 0 / 0.06);
  --shadow-lg: 0 4px 8px -3px oklch(0 0 0 / 0.10), 0 12px 20px -4px oklch(0 0 0 / 0.08);
  --shadow-xl: 0 8px 16px -4px oklch(0 0 0 / 0.12), 0 24px 40px -8px oklch(0 0 0 / 0.10);

  /* ── MOTION (ease-out, ease-spring, duration-base, …) ──────────── */
  --ease-out:    cubic-bezier(0.2, 0, 0, 1);
  --ease-in-out: cubic-bezier(0.4, 0, 0.2, 1);
  --ease-spring: linear(
    0, 0.006, 0.025, 0.101, 0.539, 0.802, 0.954, 1.041, 1.078,
    1.085, 1.072, 1.054, 1.034, 1.018, 1.006, 0.999, 0.997,
    0.998, 1.0, 1.001, 1.001, 1);
  --animate-duration-fast: 150ms; /* see note: use arbitrary duration-[var(--dur-*)] too */
}
```

> **Why `@theme inline` for color but plain `@theme` for the rest.** `inline` keeps the
> *reference* (`var(--ds-accent)`) in the generated utility instead of inlining the value at
> build time. So when `.dark` rebinds `--ds-accent`, every `bg-accent` updates live — that is
> precisely why the runtime color layer needs the separate `--ds-*` name (a `--color-*` token
> cannot point at itself). Static tokens (type, space, radius, shadow, motion) don't change per
> theme, so plain `@theme` — which inlines the value — is correct and slightly faster.

---

## 10. Tier 3 — Component tokens & consumption

Component tokens are thin aliases over semantic tokens. They give one component a tuning
knob without leaking primitives. **A component must never reach past this layer to a
primitive.**

```css
/* Component tokens: point ONLY at semantic tokens */
.button {
  --button-bg:     var(--color-accent);
  --button-fg:     var(--color-accent-fg);
  --button-bg-hover: var(--color-accent-hover);

  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  min-block-size: 2.75rem;              /* 44px — WCAG 2.5.5 target size */
  padding-inline: var(--space-4);
  border: 1px solid transparent;        /* hairline-first elevation */
  border-radius: var(--radius-md);
  background: var(--button-bg);
  color: var(--button-fg);
  font: var(--weight-semibold) var(--text-sm) / var(--leading-none) var(--font-body);
  transition: background var(--dur-fast) var(--ease-out),
              box-shadow var(--dur-fast) var(--ease-out);
}
.button:hover  { background: var(--button-bg-hover); }
.button:focus-visible {
  outline: 2px solid var(--color-ring); /* 2.4.7 + 1.4.11 ≥3:1 */
  outline-offset: 2px;
}
.button:disabled { opacity: 0.5; cursor: not-allowed; }

/* Card: hairline border IS the elevation; shadow only when it floats */
.card {
  --card-bg: var(--color-surface);
  background: var(--card-bg);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  padding: var(--space-6);
}
.card[data-floating] { box-shadow: var(--shadow-md); }
```

### React 19 + Tailwind v4 — theme provider & utility usage

```tsx
// theme-provider.tsx — sets [data-theme] on <html>; no flash of wrong theme
"use client";
import { createContext, use, useEffect, useState } from "react";

type Theme = "light" | "dark" | "system";
const ThemeCtx = createContext<{ theme: Theme; setTheme: (t: Theme) => void }>({
  theme: "system",
  setTheme: () => {},
});

export function ThemeProvider({ children }: { children: React.ReactNode }) {
  const [theme, setTheme] = useState<Theme>("system");

  useEffect(() => {
    const root = document.documentElement;
    if (theme === "system") root.removeAttribute("data-theme");
    else root.setAttribute("data-theme", theme);
    localStorage.setItem("theme", theme);
  }, [theme]);

  return <ThemeCtx value={{ theme, setTheme }}>{children}</ThemeCtx>;
}

export const useTheme = () => use(ThemeCtx);
```

```tsx
// Consume tokens via Tailwind utilities — re-themes automatically under .dark
export function Callout({ children }: { children: React.ReactNode }) {
  return (
    <div
      role="status"
      className="flex gap-3 rounded-lg border border-border bg-surface
                 p-6 text-fg shadow-sm"
    >
      <span className="text-accent" aria-hidden>★</span>
      <p className="text-base leading-relaxed text-fg-muted">{children}</p>
    </div>
  );
}
```

```html
<!-- Pre-hydration script: apply saved/system theme before paint (kills FOUC) -->
<script>
  (function () {
    var t = localStorage.getItem("theme");
    if (t === "dark" || t === "light") document.documentElement.setAttribute("data-theme", t);
  })();
</script>
```

---

## 11. Token reference (quick index)

| Group | Tokens | Notes |
|-------|--------|-------|
| **Surface** | `--color-bg`, `--color-bg-subtle`, `--color-surface`, `--color-surface-2` | ~60% of UI; dark = rising L |
| **Foreground** | `--color-fg`, `--color-fg-muted`, `--color-fg-subtle` | AA on their paired surface |
| **Border** | `--color-border`, `--color-border-subtle` | primary depth cue (hairline) |
| **Accent** | `--color-accent`, `--color-accent-fg`, `--color-accent-hover` | ~30% dominant brand action |
| **Status** | `--color-success`, `--color-warning`, `--color-danger` (+ `-fg`) | matched visual weight |
| **Focus** | `--color-ring` | WCAG 2.4.7 / 1.4.11 |
| **Space** | `--space-0 … --space-32` (+ `--space-px`) | 8pt base, 4px micro-step |
| **Radius** | `--radius-sm/md/lg/xl/2xl/full` | deliberate hierarchy, concentric |
| **Type** | `--text-xs … --text-7xl` | fluid `clamp()`, body floor 16px |
| **Leading** | `--leading-none/tight/snug/normal/relaxed/ja` | tighter as size grows |
| **Tracking** | `--tracking-tighter/tight/normal/wide/caps` | negative for display, + for caps |
| **Font** | `--font-display/body/mono` | one display + body + mono |
| **Shadow** | `--shadow-sm/md/lg/xl` | layered, low-opacity |
| **Easing** | `--ease-out/in-out/spring` | spring via `linear()` |
| **Duration** | `--dur-fast(150)/base(250)/slow(400)` | + reduced-motion guard |
| **Z-index** | `--z-dropdown … --z-tooltip` | named stacking order |

---

## 12. Verification checklist

- [ ] Every component reads **semantic/component** tokens — `grep` finds **no** `--brand-*`/`--neutral-*` in component CSS.
- [ ] All text/surface pairs pass **WCAG 2.x AA** (4.5:1 text, 3:1 large/UI) in **both** themes; APCA-tune on top.
- [ ] Dark mode adjusts **L & C** (chroma +15–25% on accents) — not a naive invert; elevation via rising surface L.
- [ ] Fluid type passes **200% zoom** (WCAG 1.4.4) — `rem` term present in every `clamp()`.
- [ ] Focus ring visible on every interactive element; targets ≥ 44px.
- [ ] `prefers-reduced-motion` honored globally; motion has chosen easing + duration.
- [ ] OKLCH chroma tapers at scale extremes (50/950) — no out-of-gamut clipping.
- [ ] Tailwind `@theme inline` for color (live re-theme); plain `@theme` for static scales.

---

## Sources

- MDN — `oklch()` (CSS Color 4) — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/oklch
- MDN — `contrast-color()` — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/contrast-color
- web.dev — Color themes with Baseline CSS (`light-dark`, relative color, `color-mix`) — https://web.dev/articles/baseline-in-action-color-theme
- oklch.com — OKLCH Color Picker & Converter — https://oklch.com/
- Tailwind CSS — Theme variables (`@theme`) — https://tailwindcss.com/docs/theme
- Tailwind CSS — Colors (v4 OKLCH palette) — https://tailwindcss.com/docs/colors
- Tailwind CSS v4.0 release notes — https://tailwindcss.com/blog/tailwindcss-v4
- Evil Martians — Better dynamic themes in Tailwind with OKLCH — https://evilmartians.com/chronicles/better-dynamic-themes-in-tailwind-with-oklch-color-magic
- Material Design 3 — Design tokens overview (ref/sys/comp) — https://m3.material.io/foundations/design-tokens/overview
- Material Design 3 — Motion easing & duration tokens-specs — https://m3.material.io/styles/motion/easing-and-duration/tokens-specs
- Microsoft Fluent 2 — Design tokens (global/alias/control) — https://fluent2.microsoft.design/design-tokens
- GitHub Primer — Token names (base/functional/component) — https://primer.style/foundations/primitives/token-names
- Atlassian Design — Spacing (8px base, numeric suffix) — https://atlassian.design/foundations/spacing
- Atlassian Design — Elevation (dark = lighter surface) — https://atlassian.design/foundations/elevation
- IBM Carbon — Spacing overview (8px scale) — https://carbondesignsystem.com/elements/spacing/overview/
- Apple — HIG Typography (Dynamic Type) — https://developer.apple.com/design/human-interface-guidelines/typography
- Apple — HIG Layout (44pt targets, concentricity) — https://developer.apple.com/design/human-interface-guidelines/layout
- Utopia — Fluid type & space calculator — https://utopia.fyi/
- Smashing Magazine — Modern Fluid Typography Using CSS clamp() — https://www.smashingmagazine.com/2022/01/modern-fluid-typography-css-clamp/
- ColorArchive — Why dark mode colors need more saturation — https://colorarchive.org/notes/may-2026-dark-mode-saturation/
- APCA — APCA in a Nutshell (Lc levels) — https://git.apcacontrast.com/documentation/APCA_in_a_Nutshell.html
- WCAG 2.2 (W3C) — https://www.w3.org/TR/WCAG22/
