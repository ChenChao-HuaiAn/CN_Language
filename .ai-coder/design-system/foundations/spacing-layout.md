---
title: "Spacing & Layout — Canonical Foundation"
description: "The 8pt spacing scale, radius scale, breakpoints, fluid containers, 4-8-12 column grids, bento composition, editorial asymmetry, safe areas, and density vs whitespace — with copy-paste CSS + React 19/Tailwind v4 recipes using the canonical token contract."
updated: 2026-06-09
tags: [foundations, spacing, layout, grid, bento, responsive, subgrid]
---

# Spacing & Layout

> The skeleton everything else hangs on. Get the rhythm right and a UI feels engineered; get it wrong and no color or type can save it. This module defines the **8pt spacing scale**, the **radius scale**, **breakpoints**, **fluid containers**, the **4 / 8 / 12 column grids**, **bento composition**, **editorial asymmetry**, **safe areas**, and **density vs whitespace** — all wired to the canonical token contract.
>
> Principle (Linear): *"Structure should be felt, not seen."* The grid is invisible; its consequences are not.

---

## 1. The 8pt spacing scale → `--space-*`

Every mature system (Carbon, Atlassian, Material 3, Apple) lands on an **8px base with a 4px half-step** for fine optical adjustments. Reasons: 8 divides cleanly across common DPRs, most icon/line-height values are multiples of 4/8, and a small fixed set kills the "is it 13 or 14px?" bikeshedding that produces inconsistent UIs.

**Tokens are static.** They do **not** change with viewport — instead you *jump steps* at breakpoints (a section that is `--space-16` apart on desktop becomes `--space-8` on mobile). This is the Carbon rule and it keeps rhythm predictable.

```css
:root {
  /* 8pt scale (rem, root = 16px). 1–2 are the 2px/4px optical half-steps. */
  --space-0: 0;
  --space-px: 1px;        /* hairline nudges only */
  --space-1: 0.25rem;     /* 4px  — icon gaps, tight inline */
  --space-2: 0.5rem;      /* 8px  — BASE UNIT, default inline gap */
  --space-3: 0.75rem;     /* 12px — compact padding */
  --space-4: 1rem;        /* 16px — standard control padding, body rhythm */
  --space-5: 1.25rem;     /* 20px */
  --space-6: 1.5rem;      /* 24px — card padding, stack gap */
  --space-8: 2rem;        /* 32px — group separation */
  --space-10: 2.5rem;     /* 40px */
  --space-12: 3rem;       /* 48px — sub-section rhythm */
  --space-16: 4rem;       /* 64px — section padding (mobile) */
  --space-20: 5rem;       /* 80px */
  --space-24: 6rem;       /* 96px — section rhythm (desktop) — the SaaS standard */
  --space-32: 8rem;       /* 128px — hero / major breaks */
}
```

> Naming maps **1 unit ≈ 0.25rem (4px)**, so `--space-4 = 16px`, `--space-8 = 32px`. This mirrors Tailwind's numeric scale exactly, so agents can translate `gap-6` ↔ `--space-6` ↔ `24px` without a lookup table.

### Scale rationale by range

| Range | Tokens | Job | Example |
|---|---|---|---|
| **Micro** 4–8px | `--space-1`, `--space-2` | Inside a component | icon↔label gap, chip padding |
| **Component** 12–24px | `--space-3`–`--space-6` | Padding, intra-card | card padding, form-field gap |
| **Layout** 32–80px | `--space-8`–`--space-20` | Between groups/regions | sidebar↔content, card grid gap |
| **Section** 96–128px | `--space-24`, `--space-32` | Page-level rhythm | hero, section vertical padding |

### The whitespace law

Spacing must encode **hierarchy**, not just separation. Gestalt proximity: elements closer together read as one group. The single most common slop tell is *uniform* spacing — a label 16px from its input and 16px from the next field, so nothing groups. Fix: **inner gap < outer gap** (e.g. label→input `--space-2`, field→field `--space-6`).

```css
/* A field group that reads as discrete units, not a wall */
.field        { display: grid; gap: var(--space-2); }   /* label ↔ control: tight */
.field-stack  { display: grid; gap: var(--space-6); }   /* field ↔ field: loose */
```

---

## 2. Radius scale → `--radius-*`

Reject the uniform-16px-on-everything default (a documented slop tell). Radius is a **hierarchy signal**: larger surfaces take larger radii; controls take smaller ones; nested radii are **concentric** (Apple's rule — inner radius ≈ outer radius − padding so curves stay parallel).

```css
:root {
  --radius-sm: 0.375rem;  /* 6px  — inputs, buttons, chips, small controls */
  --radius-md: 0.625rem;  /* 10px — cards, popovers, menus */
  --radius-lg: 1rem;      /* 16px — modals, large surfaces, media */
  --radius-xl: 1.5rem;    /* 24px — bento cells, hero panels */
  --radius-2xl: 2rem;     /* 32px — marketing feature blocks */
  --radius-full: 9999px;  /* pills, avatars, toggles */
}
```

### Concentric nesting (the premium tell)

When a child sits inside a padded parent, naive equal radii make the inner corner look pinched. Compute the child radius from the parent:

```css
.panel {
  --_pad: var(--space-4);          /* 16px */
  --_radius: var(--radius-lg);     /* 16px */
  padding: var(--_pad);
  border-radius: var(--_radius);
}
.panel > .panel__inner {
  /* concentric: inner = outer − padding, floored at the small radius */
  border-radius: max(var(--radius-sm), calc(var(--_radius) - var(--_pad)));
}
```

Map radius to **element role**, never apply one value globally:

| Role | Token | Why |
|---|---|---|
| Buttons / inputs / chips | `--radius-sm` | crisp, dev-tool feel (Vercel/Linear lean small) |
| Cards / menus / popovers | `--radius-md` | soft but not bubbly |
| Modals / media / sheets | `--radius-lg` | larger surface earns larger curve |
| Bento / hero panels | `--radius-xl`–`--radius-2xl` | marketing scale |
| Avatars / pills / toggles | `--radius-full` | shape *is* the meaning |

---

## 3. Breakpoints

Five breakpoints cover phone → ultrawide. Define them once as a documented contract. Container queries (below) handle component-level responsiveness; breakpoints handle **page layout** shifts (column counts, section padding jumps).

| Name | Min width | Target | Layout intent |
|---|---|---|---|
| `xs` | 0 | small phones | single column, stack everything |
| `sm` | 40rem / 640px | large phones | single column, larger type |
| `md` | 48rem / 768px | tablets | 2-up grids, sidebar may appear |
| `lg` | 64rem / 1024px | laptops | full 12-col grid, persistent sidebar |
| `xl` | 80rem / 1280px | desktop | content max-width caps |
| `2xl` | 96rem / 1536px | large desktop | gutters grow, content stays capped |

```css
:root {
  --bp-sm: 40rem;
  --bp-md: 48rem;
  --bp-lg: 64rem;
  --bp-xl: 80rem;
  --bp-2xl: 96rem;
}
```

> **Prefer intrinsic over breakpoints.** A `repeat(auto-fit, minmax())` grid or a container query needs *zero* breakpoints and never breaks at an untested width. Reach for media-query breakpoints only when the *page chrome* (nav layout, sidebar presence) must change — not for content cards.

---

## 4. Fluid container

The content column should grow with the viewport up to a readable cap, then center with gutters that themselves scale. Use a single `.container` primitive; do not re-invent max-widths per page.

```css
.container {
  width: 100%;
  margin-inline: auto;
  /* gutter grows from 16px (mobile) → 32px (desktop) via clamp */
  padding-inline: clamp(var(--space-4), 5vw, var(--space-8));
  /* cap reading width; 80rem ≈ 1280px is the SaaS sweet spot */
  max-width: var(--container-max, 80rem);
}

/* width variants — pick per surface, not per page */
.container--prose { --container-max: 42rem; }   /* 672px — long-form text, optimal measure */
.container--app   { --container-max: 90rem; }   /* 1440px — dense dashboards */
.container--wide  { --container-max: 96rem; }   /* 1536px — marketing/bento */
```

### Modern one-liner: the `--gutter` + `minmax` "breakout" container

Lets full-bleed children escape the gutter without wrapper divs (CSS Grid named lines):

```css
.layout-grid {
  --gutter: clamp(var(--space-4), 5vw, var(--space-8));
  --content: min(80rem, 100% - 2 * var(--gutter));
  display: grid;
  grid-template-columns:
    [full-start] var(--gutter)
    [content-start] var(--content) [content-end]
    var(--gutter) [full-end];
}
.layout-grid > *           { grid-column: content; }   /* default: in the column */
.layout-grid > .full-bleed { grid-column: full; }      /* opt-in: edge to edge */
```

---

## 5. Safe areas (mobile / notch / Liquid Glass)

Edge-to-edge backgrounds with content inset from the notch, Dynamic Island, home indicator, and rounded device corners. Apple's rule: **backgrounds extend full-bleed, content respects the safe area.** Combine `env()` with your own padding using `max()` so you never go below your design minimum.

```css
.app-shell {
  /* background reaches the physical edge; content never does */
  padding-top:    max(var(--space-4), env(safe-area-inset-top));
  padding-bottom: max(var(--space-4), env(safe-area-inset-bottom));
  padding-inline: max(var(--space-4), env(safe-area-inset-left), env(safe-area-inset-right));
}

/* a bottom action bar that clears the home indicator */
.bottom-bar {
  position: sticky;
  bottom: 0;
  padding-block-start: var(--space-3);
  padding-block-end: max(var(--space-3), env(safe-area-inset-bottom));
}
```

```html
<!-- REQUIRED for env() to resolve to real insets -->
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover" />
```

> Touch targets ≥ **44×44px** (Apple HIG / WCAG 2.5.5). Enforce with `min-block-size: 44px; min-inline-size: 44px;` on interactive controls, even when the visible glyph is smaller.

---

## 6. Density vs whitespace

One axis, two products. Marketing/consumer surfaces want **air**; dashboards/data tools want **density**. Encode it as a *toggle on the same tokens* (Primer's `condensed | normal | spacious` pattern) rather than forking components.

```css
:root {
  --density: 1;                         /* normal */
  --pad-control: calc(var(--space-3) * var(--density));
  --gap-stack:   calc(var(--space-6) * var(--density));
  --row-height:  calc(2.75rem * var(--density));   /* 44px target at normal */
}
[data-density="compact"]  { --density: 0.72; }   /* dense tables, IDE-like */
[data-density="spacious"] { --density: 1.4; }    /* marketing, onboarding */
```

| Surface | Density | Rationale |
|---|---|---|
| Marketing / landing | spacious | whitespace = confidence; ~96px section rhythm |
| App content / forms | normal | scannable, 44px targets |
| Data grids / logs / IDE | compact | maximize rows-per-screen (Vercel logs, Linear) |

> Whitespace is **not** wasted space — it is the cheapest hierarchy tool you have. Restraint reads as confidence. But density is a feature for power users: a 0.72 multiplier on a data grid puts ~40% more rows on screen without shrinking the font below legibility.

---

## 7. The 4 / 8 / 12 column grid

A **12-column** grid is the industry workhorse (divisible by 2, 3, 4, 6 → halves, thirds, quarters, sixths). Carbon uses 16 for data-dense enterprise; 12 is the right default for product + marketing. Collapse to **8 at tablet** and **4 at phone** so spans degrade predictably.

```css
.grid12 {
  display: grid;
  gap: var(--space-6);                       /* 24px gutter */
  grid-template-columns: repeat(4, 1fr);     /* phone: 4 cols */
}
@media (min-width: 48rem) {                  /* md: 8 cols */
  .grid12 { grid-template-columns: repeat(8, 1fr); }
}
@media (min-width: 64rem) {                  /* lg: 12 cols, wider gutter */
  .grid12 {
    grid-template-columns: repeat(12, 1fr);
    gap: var(--space-8);                     /* 32px gutter */
  }
}

/* span helpers — clamp so a desktop span never overflows a 4-col phone */
.col-3  { grid-column: span min(3, var(--cols, 12)); }
.col-4  { grid-column: span 4; }
.col-6  { grid-column: span 6; }
.col-8  { grid-column: span 8; }
.col-12 { grid-column: 1 / -1; }
```

> On a 4-column phone grid, `.col-6` would overflow. Either rely on the column-count collapse (a `span 6` simply clamps to the track count) or scope span classes inside `@media`. The safe default below avoids that footgun entirely with `auto-fit`.

### Intrinsic responsive grid (no breakpoints, the recommended default)

`auto-fit` + `minmax()` reflows from 1→N columns based purely on available width. This is the grid you reach for 90% of the time — it never breaks at an untested viewport.

```css
.auto-grid {
  display: grid;
  gap: var(--space-6);
  /* each card is ≥ 16rem; columns fit as many as possible, then stretch */
  grid-template-columns: repeat(auto-fit, minmax(min(16rem, 100%), 1fr));
}
```

The `min(16rem, 100%)` guard prevents overflow on viewports narrower than the track minimum — without it, a 16rem minimum on a 320px phone causes a horizontal scrollbar.

---

## 8. Holy-grail layout with subgrid

App shell: header, footer, left sidebar, optional right rail, fluid main. **Subgrid** lets the inner content align to the *same* column tracks as the page grid, so a card's internals line up with the global rhythm — the detail that makes a layout feel engineered rather than assembled.

```css
.app {
  min-block-size: 100dvh;
  display: grid;
  grid-template:
    "header  header  header" auto
    "sidebar main    rail"   1fr
    "footer  footer  footer" auto
    / minmax(0, 16rem) minmax(0, 1fr) minmax(0, 20rem);
  gap: var(--space-6);
}
.app > header  { grid-area: header; }
.app > .side   { grid-area: sidebar; }
.app > main    { grid-area: main; }
.app > .rail   { grid-area: rail; }
.app > footer  { grid-area: footer; }

/* collapse rail then sidebar as width shrinks */
@media (max-width: 80rem) {
  .app { grid-template:
    "header  header" auto
    "sidebar main"   1fr
    "footer  footer" auto
    / minmax(0, 16rem) minmax(0, 1fr); }
  .app > .rail { display: none; }
}
@media (max-width: 48rem) {
  .app { grid-template:
    "header" auto "main" 1fr "footer" auto / 1fr; }
  .app > .side { display: none; }   /* move to a drawer at this width */
}

/* SUBGRID: a card inherits the parent's columns so its header/body/footer
   align to the page grid instead of an arbitrary internal one */
.card-subgrid {
  display: grid;
  grid-template-columns: subgrid;
  grid-column: span 2;
  gap: inherit;
}
```

> `100dvh` (dynamic viewport height) — not `100vh` — so the shell doesn't jump when mobile browser chrome shows/hides.

---

## 9. Bento grid composition

Named for the Japanese lunchbox; mainstreamed by Apple's WWDC/product pages, now the **default for B2B SaaS feature sections** (one single-source industry estimate puts adoption ~67% across top ProductHunt SaaS — directional, not audited). The rule that separates a bento from "a rounded grid": **cell size maps to importance.** Equal cells = no hierarchy = slop.

```css
.bento {
  display: grid;
  gap: var(--space-4);                        /* 16px — tight enough to read as one unit */
  grid-template-columns: repeat(4, 1fr);
  grid-auto-rows: minmax(11rem, auto);        /* min row height, grows with content */
}
.bento > .cell {
  background: var(--color-surface);
  border: 1px solid var(--color-border);      /* hairline depth, not shadow */
  border-radius: var(--radius-xl);
  padding: var(--space-6);
}
/* hierarchy via span — ONE focal cell (top-left, F-pattern entry) */
.bento > .feature  { grid-column: span 2; grid-row: span 2; }
.bento > .wide     { grid-column: span 2; }
.bento > .tall     { grid-row: span 2; }

@media (max-width: 64rem) { .bento { grid-template-columns: repeat(2, 1fr); } }
@media (max-width: 40rem) {
  .bento { grid-template-columns: 1fr; }      /* restacks cleanly, no overlap */
  .bento > .feature, .bento > .wide, .bento > .tall { grid-column: auto; grid-row: auto; }
}
```

### Bento composition rules

- **One focal cell.** A 2×2 hero anchors the top-left (where eyes/VoiceOver start). Everything else supports it.
- **Mix card types.** Alternate *outcome* cards ("Cut sales cycle 30%") with *feature* cards. Put social proof in row 2–3.
- **Vary sizes deliberately** — a 2×1 wide for a chart, a 1×2 tall for a list, 1×1 for a stat.
- **Cap at ~12–15 cells.** More than that and hierarchy collapses back into a grid.
- **Hairlines, not shadows.** Bento cells separate by 1px border + the gap, never a drop shadow per cell (that's cards-in-cards slop).
- **Restack, never overlap.** At phone width every cell goes full-width in source order — make sure source order *is* importance order.

---

## 10. Editorial asymmetry

The centered-eyebrow + 64px headline + two-CTA hero is the statistical median — avoid it as a default. Asymmetry creates a focal point and signals a human made deliberate choices. The tool is an **off-balance grid**: give content unequal column weight (e.g. 7/5 or 8/4), left-align the headline, and let one element break the grid.

```css
/* Asymmetric hero: heavy text column + lighter media column */
.hero-editorial {
  display: grid;
  gap: var(--space-12);
  align-items: center;
  grid-template-columns: 1fr;                 /* phone: stacked */
}
@media (min-width: 64rem) {
  .hero-editorial {
    /* 7:5 split, not 50/50 — the asymmetry is the point */
    grid-template-columns: 7fr 5fr;
  }
}
.hero-editorial__title {
  /* left-aligned, fluid, tight tracking — the marketing tell */
  font: 600 var(--text-6xl)/var(--leading-tight) var(--font-display);
  letter-spacing: var(--tracking-tight);
  text-wrap: balance;                         /* even ragged edge on multi-line */
  max-inline-size: 18ch;                      /* hold an intentional line break */
}
.hero-editorial__lede {
  color: var(--color-fg-muted);
  max-inline-size: 52ch;                      /* readable measure for body */
  text-wrap: pretty;                          /* avoid orphans */
}
```

| Slop default | Editorial alternative |
|---|---|
| Centered everything | Left-aligned text, asymmetric 7/5 grid |
| 50/50 split | Intentional 7/5 or 8/4 weight |
| Eyebrow + 64px + 2 identical CTAs | One dominant headline, one primary CTA, one ghost link |
| Three identical feature cards | Bento with one focal cell + varied sizes |
| Full-width text line | Capped measure (`max-inline-size: 52ch`) + `text-wrap: balance/pretty` |

> `text-wrap: balance` on headings (≤ ~6 lines) and `text-wrap: pretty` on body copy ship the kind of typographic polish that used to require manual `<br>` tags. Use `ch` units to cap measure — type, not pixels, defines reading comfort.

---

## 11. Tailwind v4 `@theme` mirror

Tailwind v4 reads design tokens from a CSS-first `@theme` block; the framework-agnostic vars above *are* the source of truth, and `@theme` re-exposes them as utilities (`gap-6`, `rounded-xl`, `max-w-prose`, `p-24`). Because the numeric scale matches, no translation layer is needed.

```css
/* app.css */
@import "tailwindcss";

@theme {
  /* spacing — Tailwind multiplies its numeric scale by --spacing; keep 0.25rem base */
  --spacing: 0.25rem;                 /* p-4 = 1rem, p-24 = 6rem, etc. */

  /* radius — drives rounded-sm … rounded-2xl */
  --radius-sm: 0.375rem;
  --radius-md: 0.625rem;
  --radius-lg: 1rem;
  --radius-xl: 1.5rem;
  --radius-2xl: 2rem;

  /* breakpoints — drives sm:, md:, lg:, xl:, 2xl: */
  --breakpoint-sm: 40rem;
  --breakpoint-md: 48rem;
  --breakpoint-lg: 64rem;
  --breakpoint-xl: 80rem;
  --breakpoint-2xl: 96rem;

  /* container query sizes — drives @sm:, @md: with `@container` */
  --container-prose: 42rem;
  --container-app: 90rem;
}
```

```html
<!-- The auto-grid (section 7) as Tailwind utilities -->
<div class="grid gap-6 [grid-template-columns:repeat(auto-fit,minmax(min(16rem,100%),1fr))]">
  <article class="rounded-md border border-[--color-border] bg-[--color-surface] p-6">…</article>
</div>

<!-- Bento with a focal cell -->
<section class="grid grid-cols-4 gap-4 [grid-auto-rows:minmax(11rem,auto)] max-lg:grid-cols-2 max-sm:grid-cols-1">
  <div class="col-span-2 row-span-2 rounded-xl border border-[--color-border] bg-[--color-surface] p-6">…</div>
  <div class="col-span-2 rounded-xl border border-[--color-border] bg-[--color-surface] p-6">…</div>
</section>
```

---

## 12. React 19 layout primitives

Ship `Stack`, `Inline`, and `Grid` as the only spacing API surface — devs (and agents) never hand-write `margin`. This is the Atlassian/Polaris pattern: spacing lives on the *container's* `gap`, never on children (margins collapse unpredictably and break reordering).

```tsx
// layout.tsx — React 19, framework-agnostic CSS vars as the contract
import type { CSSProperties, ElementType, PropsWithChildren } from "react";

type Space = 0 | 1 | 2 | 3 | 4 | 6 | 8 | 12 | 16 | 24 | 32;
const gap = (s: Space) => (s === 0 ? "0" : `var(--space-${s})`);

type StackProps = PropsWithChildren<{
  as?: ElementType;
  gap?: Space;
  align?: CSSProperties["alignItems"];
}>;

/** Vertical rhythm. Owns the gap; children stay margin-free. */
export function Stack({ as: Tag = "div", gap: g = 6, align, children }: StackProps) {
  return (
    <Tag style={{ display: "grid", gap: gap(g), alignItems: align }}>
      {children}
    </Tag>
  );
}

/** Horizontal flow that wraps; same gap contract. */
export function Inline({ gap: g = 4, align = "center", children }: StackProps) {
  return (
    <div style={{ display: "flex", flexWrap: "wrap", gap: gap(g), alignItems: align }}>
      {children}
    </div>
  );
}

type GridProps = PropsWithChildren<{ min?: string; gap?: Space }>;

/** Intrinsic auto-fit grid — no breakpoints, never overflows. */
export function Grid({ min = "16rem", gap: g = 6, children }: GridProps) {
  return (
    <div
      style={{
        display: "grid",
        gap: gap(g),
        gridTemplateColumns: `repeat(auto-fit, minmax(min(${min}, 100%), 1fr))`,
      }}
    >
      {children}
    </div>
  );
}
```

```tsx
// usage — composition reads as a spacing spec
<Stack gap={12}>
  <h1>Dashboard</h1>
  <Grid min="18rem" gap={6}>
    <Card /><Card /><Card />
  </Grid>
  <Inline gap={2}>
    <Button>Save</Button>
    <Button variant="ghost">Cancel</Button>
  </Inline>
</Stack>
```

### Staggered page-load (one orchestrated entrance, not scattered)

Motion communicates *attention*, not decoration. A single staggered reveal of the layout grid on load beats a dozen micro-interactions. Honor `prefers-reduced-motion`.

```tsx
import { motion } from "motion/react";

const container = {
  hidden: {},
  show: { transition: { staggerChildren: 0.06, delayChildren: 0.04 } },
};
const item = {
  hidden: { opacity: 0, y: 12 },
  show: {
    opacity: 1, y: 0,
    transition: { type: "spring", stiffness: 200, damping: 24, bounce: 0.05 },
  },
};

export function RevealGrid({ children }: { children: React.ReactNode }) {
  return (
    <motion.div
      variants={container}
      initial="hidden"
      animate="show"
      style={{ display: "grid", gap: "var(--space-6)" }}
    >
      {Array.isArray(children)
        ? children.map((c, i) => <motion.div key={i} variants={item}>{c}</motion.div>)
        : children}
    </motion.div>
  );
}
```

```css
/* CSS-only equivalent via scroll-driven animation, plus the reduced-motion guard */
@media (prefers-reduced-motion: reduce) {
  * { animation-duration: 0.01ms !important; animation-iteration-count: 1 !important; }
}
```

---

## 13. Layout states: loading · empty · error

A grid is not "done" until its three off-happy-path states hold the same rhythm. Reserve space so layout doesn't shift (CLS) when content arrives — skeletons must match the real cell footprint.

```css
/* Skeleton cell — same box model as the real card, so no layout shift */
.skeleton {
  border-radius: var(--radius-md);
  background: var(--color-surface-2);
  min-block-size: 11rem;                  /* matches grid-auto-rows minimum */
  position: relative;
  overflow: hidden;
}
.skeleton::after {
  content: "";
  position: absolute;
  inset: 0;
  background: linear-gradient(90deg, transparent, var(--color-bg-subtle), transparent);
  translate: -100% 0;
  animation: shimmer 1.4s var(--ease-in-out, ease-in-out) infinite;
}
@keyframes shimmer { to { translate: 100% 0; } }
@media (prefers-reduced-motion: reduce) { .skeleton::after { animation: none; } }

/* Empty state — centered in the grid area, never a bare blank region */
.empty {
  grid-column: 1 / -1;
  display: grid;
  place-items: center;
  gap: var(--space-4);
  padding-block: var(--space-16);
  text-align: center;
  color: var(--color-fg-muted);
  border: 1px dashed var(--color-border);   /* dashed = "nothing here yet", not an error */
  border-radius: var(--radius-lg);
}

/* Error region — danger color carries semantics, but never color alone */
.layout-error {
  grid-column: 1 / -1;
  display: grid;
  gap: var(--space-3);
  padding: var(--space-6);
  border: 1px solid var(--color-danger);
  border-radius: var(--radius-md);
  color: var(--color-danger-fg);
  background: color-mix(in oklch, var(--color-danger) 8%, var(--color-surface));
}
```

```tsx
function CardGrid({ status, items }: {
  status: "loading" | "empty" | "error" | "ready";
  items: Item[];
}) {
  return (
    <Grid min="16rem" gap={6}>
      {status === "loading" &&
        Array.from({ length: 6 }, (_, i) => <div key={i} className="skeleton" aria-hidden="true" />)}
      {status === "empty" && (
        <div className="empty" role="status">
          <p>No projects yet</p>
          <Button>Create your first project</Button>
        </div>
      )}
      {status === "error" && (
        // role="alert" announces immediately; icon + text, never color alone
        <div className="layout-error" role="alert">
          <strong>Couldn't load projects</strong>
          <Button variant="ghost">Retry</Button>
        </div>
      )}
      {status === "ready" && items.map((it) => <Card key={it.id} {...it} />)}
    </Grid>
  );
}
```

> Loading uses `aria-hidden` (skeletons are noise to a screen reader); empty uses `role="status"` (polite); error uses `role="alert"` (assertive). The grid footprint is identical in all four states → zero layout shift.

---

## 14. Do / Don't

| ✅ Do | ❌ Don't |
|---|---|
| Use `gap` on the container for all spacing | Add `margin` to children (collapses, breaks reorder) |
| Pull every value from `--space-*` / `--radius-*` | Hardcode `13px`, `18px`, magic numbers |
| Inner gap < outer gap (proximity = grouping) | Uniform spacing everywhere (no hierarchy) |
| `auto-fit minmax(min(…),1fr)` for content grids | Stack media queries to fake reflow |
| Map radius to element role; nest concentrically | One 16px radius on everything |
| Hairline `1px` borders + surface ladder for depth | A drop shadow on every card (cards-in-cards) |
| One focal cell per bento; size = importance | Equal cells (just a rounded grid) |
| Asymmetric 7/5 hero, left-aligned, capped measure | Centered eyebrow + 64px + 2 identical CTAs |
| `100dvh`, `env(safe-area-inset-*)`, ≥44px targets | `100vh` + ignoring the notch/home indicator |
| Reserve skeleton space; match real footprint | Spinner-only loading that shifts layout on load |
| `text-wrap: balance/pretty`, `ch` measure caps | Full-width text lines, manual `<br>` hacks |

---

## Sources

- IBM Carbon — Spacing overview — https://carbondesignsystem.com/elements/spacing/overview/
- IBM Carbon — 2x Grid overview — https://carbondesignsystem.com/elements/2x-grid/overview/
- Atlassian Design — Spacing — https://atlassian.design/foundations/spacing
- GitHub Primer — Token names (density: condensed/normal/spacious) — https://primer.style/foundations/primitives/token-names
- Material Design 3 — Design tokens overview — https://m3.material.io/foundations/design-tokens/overview
- Apple — HIG Layout (safe areas, 44pt targets, concentricity) — https://developer.apple.com/design/human-interface-guidelines/layout
- Apple — Human Interface Guidelines — https://developer.apple.com/design/human-interface-guidelines/
- Linear — Behind the latest design refresh ("structure felt, not seen") — https://linear.app/now/behind-the-latest-design-refresh
- Geist (Vercel) — Typography & functional color scale — https://vercel.com/geist/typography
- Understanding the Bento Layout Trend (SaaSFrame) — https://www.saasframe.io/blog/the-bento-layout-trend
- Bento Grids Quietly Winning B2B SaaS Homepages 2026 (Pravin Kumar) — https://www.pravinkumar.co/blog/bento-grids-b2b-saas-homepage-design-trend-2026
- Crafting grid and dot backgrounds with CSS/Tailwind (ibelick) — https://ibelick.com/blog/create-grid-and-dot-backgrounds-with-css-tailwind-css
- Motion (ex framer-motion) — React transitions & springs — https://motion.dev/docs/react-transitions
- Tailwind CSS v4 — Theme variables (`@theme`) — https://tailwindcss.com/docs/theme
- WCAG 2.2 (target size 2.5.5) — https://www.w3.org/TR/WCAG22/
