---
title: "Best-Practice Frontend Design for AI Agents — The Flagship"
description: "The single source-linked definition of best-practice frontend design as of June 2026, organized so an AI agent can act on it directly: the taste layer, the modern-2026 standard, the layered design-system model, the canonical token contract, motion, components, patterns, accessibility, the 2026 stack, and how to operate. Read this one file; route to the deep modules from here."
updated: 2026-06-09
status: canonical · flagship
audience: AI agents and engineers — act on this directly; do not re-fetch
tags: [design-system, ai-agents, anti-slop, oklch, tailwind-v4, react-19, wcag-2.2, motion, tokens]
---

# Best-Practice Frontend Design for AI Agents

> **If you read one file, read this.** It is the map *and* the contract: the standard for
> "modern, human, non-slop" frontend as of **June 2026**, organized so you can act without
> fetching anything. It distills the whole system — philosophy, foundations, motion, components,
> patterns, accessibility, stack — and cross-links the deep modules. The deep modules embed the
> full copy-paste code; this file gives you the load-bearing essence and the canonical names.

---

## 1. TL;DR — the standard in one breath + how to use this file

**The standard, one breath:** *Frame the right problem, then ship a **coherent whole** that could
only be **this** brand for **this** audience — calm, dark-capable, near-monochrome confidence with
**one semantic accent used like punctuation**, real hierarchy from contrast (3×+ size jumps, weight
extremes), depth from **1px hairlines + a surface ladder** (not stacked shadows), color authored in
**OKLCH**, a **deliberate, distinctive font** (never Inter/system as the only choice), **physics/spring
motion that communicates state** in one orchestrated page-load, every **error/empty/loading** state
designed, **WCAG 2.2 AA** met — all themed onto a single **token contract**, never shipped raw.*

The opposite — the statistical median of every Tailwind tutorial — is **AI slop**: superficially
competent, point-of-view-free, swappable between a fintech, a CRM, and a to-do app with zero
changes. *Thoughtlessness makes slop; AI just scales it.*

**How to use this file (the operating loop):**

1. **Read the taste layer first.** [`./00-philosophy/human-not-ai.md`](./00-philosophy/human-not-ai.md)
   + [`./00-philosophy/principles.md`](./00-philosophy/principles.md). Short; prevents slop. (§3 here.)
2. **Route via the INDEX.** [`./INDEX.md`](./INDEX.md) maps a task → the 1–3 modules it needs. Open
   only those. Keep context lean.
3. **Declare a direction before any CSS.** Purpose · tone (an extreme) · constraints · two named
   anchors to triangulate. (§3, the "right altitude" method.)
4. **Use the token vocabulary.** Everything composes from the canonical contract
   ([`./foundations/tokens.md`](./foundations/tokens.md) — source of truth). Components read
   *semantic* names only, never raw hex/px/ms. (§4–§5 here.)
5. **Run the Pre-Flight before shipping.** The Anti-AI-Slop Checklist + the four grading criteria,
   via a **generator/evaluator split** — never let the agent that built it grade it. (§3, §10.)

> **Non-negotiables (10-second version):** taste layer first · one accent, semantic color, OKLCH ·
> a deliberate font · hierarchy via contrast, one focal point · theme shadcn/Tailwind, never raw ·
> motion communicates, one orchestrated load · design error/empty/loading + WCAG 2.2 AA ·
> framework-agnostic first, React 19 / Tailwind v4 / Motion as the reference.

---

## 2. What "modern" means in 2026

**The verdict (triangulated definition):** *Modern in 2026 is calm, dark-capable, near-monochrome
confidence with physical motion.* It is the intersection of four design cultures that all arrived,
independently, at **depth-through-restraint** — captured one year after Apple's Liquid Glass (WWDC25,
Jun 9 2025) and Google's Material 3 Expressive (Google I/O, May 13 2025). Full treatment +
contributor code: [`./00-philosophy/modern-2026.md`](./00-philosophy/modern-2026.md).

| Pole | The idea we keep | The trap we drop |
|---|---|---|
| **Apple** — Liquid Glass | Depth, physicality, content-first deference; glass on the **chrome layer only**; velocity-continuous springs; concentric corners | Glass on content / glass-on-glass; over-translucency that kills legibility |
| **Google** — M3 Expressive | Motion as **interruptible physics** (re-targetable mid-gesture); perceptual color (HCT); tonal elevation | Bouncy-everywhere; Roboto-by-default; rainbow dynamic-color on a brand product |
| **Nothing** — Monochrome restraint | Color is semantic and *rare*; hierarchy via **opacity tiers**; honest, exposed structure | Literal dot-matrix everywhere; novelty over usability |
| **SaaS frontier** — Linear / Vercel / Raycast | Hairlines over shadows; subtractive color; one accent; tracked confident type; dark-native tokens | Cold dev-tool minimalism copied onto warm consumer products |

**The recurring traits (the canon — if a screen has most, it reads 2026):**

1. **Dark-capable, tinted — never pure `#000`** (canvas at ~4–8% L; pure black makes text vibrate).
2. **Hairline borders + surface ladder = depth** (1px / 0.5px-retina; shadows reserved for floating layers).
3. **OKLCH / perceptual color** — adjust **L and C** for dark mode; never a naive invert.
4. **One semantic accent, used like punctuation** — meaning, not decoration.
5. **Large headlines, tightened tracking** (~48–72px at **-0.04em**, line-height ~1.05–1.15).
6. **Physics/spring motion** for user-driven UI; fixed-duration tweens for continuous/automatic.
7. **Bento layouts** where **cell size maps to importance** (≤12–15 cells).
8. **Tasteful glass on the chrome layer only** (`backdrop-filter` blur + noise + border + `-webkit-` prefix).
9. **Honest structure + texture** — expose the grid; layer **grain** (`feTurbulence`, opacity 0.05–0.12).

**What is now DATED (the 2021 → 2026 diff):** pure `#000` dark mode · a drop shadow on every card ·
blue→purple gradient on white · gradient *text* on headings/metrics · Inter/Roboto/system as the
*only* font · uniform 16px radius on everything · cards nested 3–5 deep · centered eyebrow + 64px +
2 identical CTAs · three identical emoji feature cards · sRGB/HSL math + naive dark invert ·
built-in `ease`/long 600ms+ tweens · skeuomorphic bevels / faux glass with no noise.

> **The test for modern:** pass 9+ of the canon traits, avoid every dated tell, and **fail the
> swap-test** — if you can swap the product name and it could be any other SaaS, differentiate at
> the *layout* level, not just the colors.

---

## 3. Human, not AI — the taste layer

This is the verdict you read **before** generating any frontend and the bar you grade against
**after**. Full checklist, the slop-tell table, the "right altitude" method, and Anthropic's
distilled aesthetics prompt: [`./00-philosophy/human-not-ai.md`](./00-philosophy/human-not-ai.md).

**What slop is:** output that is *superficially competent but lacks substance, point of view, or
contextual fit* — academically (Kommers et al., arXiv:2601.06060): superficial competence,
asymmetric effort, mass producibility. In web design the symptom is **distributional convergence**:
ask for "a landing page" with no constraints and you get the median of every scraped tutorial. This
is **not a knowledge gap** — the model defaults to the statistical center unless steered off it.
(The infamous `bg-indigo-500` Tailwind default is a large part of why AI sites turn purple.)

**The slop tells (memorize, to catch yourself):** Inter/Roboto/system fonts (Space Grotesk as the
"I tried"); purple→blue gradient on white, timid even palettes; the centered-eyebrow hero; three
emoji feature cards; **cards-in-cards** with one-sided colored borders (the single most recognizable
AI-UI signature); uniform 16px radius; `0.1`-opacity shadows everywhere; uniform fade-ins or snaps;
plastic stock/AI imagery; hedging copy ("may help," "scale without limits"); raw shadcn defaults.

**Human-feel principles (the 10, compact):** (1) a clear **point of view** — triangulate two named
anchors; (2) **restraint = confidence** — white space is editorial intent; (3) **intentional
imperfection** — grain/asymmetry that reads as care; (4) real **content hierarchy** via contrast;
(5) **editorial confidence** — magazine asymmetry; (6) **considered motion** — communicates, never
decorates; (7) **craft at 800% zoom** — optical alignment, spacing rhythm; (8) **originality** —
custom decisions over templates; (9) **contextual fit** — semantic color, real imagery, a specific
voice; (10) a **coherent whole**, not a collection of parts.

**The right-altitude fix:** vague prompts converge to the mean; pixel-locked specs go "technically
correct but visually dead." The sweet spot is **principle-based direction** — tell the model *what
to think about* (typography, color, motion, backgrounds), one dimension at a time, not exactly
*what to produce.* **Step 0:** declare purpose · a tone *extreme* (editorial, brutalist, luxury,
industrial…) · real constraints · two anchors — *then* code.

**The four grading criteria (makes taste gradable — ask "does this follow our principles?" not "is
this beautiful?"):**

| # | Criterion | The grading question |
|---|---|---|
| 1 | **Design quality** | Does it feel like a *coherent whole* rather than a collection of parts? (Anthropic's #1.) |
| 2 | **Originality** | Custom decisions over templates? Would a designer recognize deliberate choices? |
| 3 | **Craft** | Clean execution — type hierarchy, spacing rhythm, color harmony, contrast, elevation? |
| 4 | **Functionality** | Usable independent of aesthetics — states, affordances, accessibility? |

> **Generator/evaluator split (GAN-inspired):** the **generator** builds against the tone brief +
> token contract; a separate **evaluator** in a *fresh context* scores the four criteria + the
> checklist, returns a ✗-list, and does **not** generate. Gate strictly (every criterion ≥ 8/10 and
> zero slop tells) — "good enough" is how the median creeps back in.

---

## 4. The required elements of a design system

A design system is a **layered model**, each layer consuming only the one below it. This is the
convergent shape of Material 3 (`ref/sys/comp`), Fluent 2, and Primer — adopted here as a flat,
framework-agnostic, AI-pasteable surface. The one rule that makes theming free: **components
reference only semantic (or component) tokens — never primitives.**

```
PHILOSOPHY   the taste layer + principles — the "why" every layer serves   → 00-philosophy/
   ↓
TOKENS       primitives → semantic roles → component knobs (the contract)   → foundations/tokens.md ★
   ↓
FOUNDATIONS  color · typography · spacing/layout — values for the roles     → foundations/*
   ↓
MOTION       easing/duration/spring tokens + orchestration recipes          → motion/*
   ↓
COMPONENTS   button/card/input/dialog/tabs… — anatomy · states · a11y       → components/components.md
   ↓
PATTERNS     hero/bento/sections/nav/footer/backgrounds — composition        → patterns/patterns.md
   ↓
GUIDELINES   accessibility baseline + usage rules + do/don't                  → accessibility/a11y.md
   ↓
GOVERNANCE   generator/evaluator split · the slop CI guard · versioning      → §3, §10
```

**The required-elements checklist** — a system is incomplete until each is true:

- [ ] **Philosophy** written down: principles + the anti-slop taste layer, used as gates.
- [ ] **Tokens** in three tiers; semantic names are stable across light/dark/high-contrast.
- [ ] **Color** authored in OKLCH; one dominant hue + one sharp accent; semantic roles; dark re-derived.
- [ ] **Typography** with a deliberate display + body (+ mono); a fluid scale; per-size rhythm; 日本語 covered.
- [ ] **Spacing/layout** on an 8pt scale; intentional radius scale; intrinsic grids; bento; safe areas.
- [ ] **Motion** tokens (easing/duration/spring) + one orchestrated load; `prefers-reduced-motion` everywhere.
- [ ] **Components** ship every state (default·hover·active·focus-visible·disabled·loading·error) + a11y.
- [ ] **Patterns** differentiate at the *layout* level; atmosphere is layered and subtractive.
- [ ] **Accessibility** baseline (WCAG 2.2 AA) is a contract every token/component/pattern honors.
- [ ] **Governance**: generator/evaluator split, a mechanical slop CI guard, and the swap-test pass.

---

## 5. Foundations at a glance

Three foundations turn the philosophy into values. Each has a dedicated module with the full
treatment; the essence and the **one copy-paste token block** are below.

**Color** ([`./foundations/color.md`](./foundations/color.md)) — author in **OKLCH** (perceptually
uniform L/C/H: equal L = equal perceived brightness, so tonal scales walk L and *look* even). Ship
**sRGB-safe by default**, upgrade to `display-p3` under `@supports`. Compose **one dominant hue + one
sharp accent** (60-30-10), neutrals borrowing the brand hue at near-zero chroma so grays feel
related. Taper chroma at scale extremes (peaks at 400–600). Theme through **semantic roles only**.
Contrast: **WCAG 2.x AA is the legal floor**, **APCA `Lc` is the readability ceiling** — pass both;
trust APCA in dark mode (WCAG 2 over-rates dark pairs). Dark mode **lowers L, raises C ~15–25%**
(simultaneous contrast) and nudges warm hues (Bezold-Brücke) — a designed context, not an inversion.

**Typography** ([`./foundations/typography.md`](./foundations/typography.md)) — **one display face
for identity, one body face for invisibility** (+ mono for code), with real contrast between them.
Escape the Inter/Roboto default (Geist, Satoshi, Bricolage Grotesque, Clash Display, General Sans…).
Hierarchy is **contrast, not count**: 3×+ size jumps, weight extremes. **Optical tuning per size** —
big text wants tighter leading + negative tracking; small text the reverse. Fluid scale via
`clamp(rem + vw)` — the `rem` term keeps text responsive to zoom (**WCAG 1.4.4: must pass 200%
zoom**; body floor ≥ 16px, max ≤ ~2.5× min). Self-host variable WOFF2, preload, `font-display: swap`,
metric-matched fallback (or `next/font` `adjustFontFallback`). **日本語 (和文) is its own system:**
line-height **1.6–1.9**, letter-spacing **0.04–0.08em**, `font-feature-settings: "palt" 1`,
`line-break: strict` (kinsoku), **never italics** (use 傍点 `text-emphasis` / weight), dual JP+Latin
stack with the **Latin face listed first** (和欧混植); body = system fonts, webfont for headings only.

**Spacing & layout** ([`./foundations/spacing-layout.md`](./foundations/spacing-layout.md)) — an
**8pt scale** with a 4px half-step; tokens are static, you *jump steps* at breakpoints. The
whitespace law: **inner gap < outer gap** (proximity = grouping). An **intentional radius scale**
mapped to element role, nested **concentrically** (child radius ≈ parent − padding). Prefer
**intrinsic grids** — `repeat(auto-fit, minmax(min(16rem,100%),1fr))` needs zero breakpoints and
never breaks at an untested width. **Bento** = cell size maps to importance, one focal cell, hairlines
not shadows. **Editorial asymmetry** (7/5 or 8/4, left-anchored) over the centered median. Safe areas
via `env(safe-area-inset-*)` + `max()`; `100dvh` not `100vh`; tap targets ≥ 44×44px.

### The key copy-paste token block

This is the **canonical token contract** condensed — light is `:root`, dark re-derives the same
role names. Source of truth (with primitives, P3 upgrade, Tailwind v4 `@theme inline` mirror, and
the verification checklist): [`./foundations/tokens.md`](./foundations/tokens.md). Components consume
these **semantic names only** — never a raw `--brand-*`/`--neutral-*`, never a hardcoded hex/px/ms.

```css
/* ── THE CONTRACT (condensed). Author = OKLCH. Theme = semantic roles. ─────────── */
:root {
  color-scheme: light dark;

  /* COLOR — semantic roles (the names every component & module consumes) */
  --color-bg:            oklch(0.985 0.004 264);  /* page canvas */
  --color-bg-subtle:     oklch(0.967 0.006 264);  /* sunken wells, code blocks */
  --color-surface:       oklch(1 0 0);            /* cards, panels */
  --color-surface-2:     oklch(0.967 0.006 264);  /* raised: menus, popovers */
  --color-fg:            oklch(0.210 0.012 264);  /* primary text */
  --color-fg-muted:      oklch(0.446 0.013 264);  /* secondary, labels */
  --color-fg-subtle:     oklch(0.556 0.013 264);  /* placeholder, disabled, hints */
  --color-border:        oklch(0.922 0.008 264);  /* THE depth primitive (hairline) */
  --color-border-subtle: oklch(0.967 0.006 264);
  --color-accent:        oklch(0.560 0.190 264);  /* the ONE accent (≈30% of UI) */
  --color-accent-fg:     oklch(1 0 0);            /* text/icon ON accent */
  --color-accent-hover:  oklch(0.476 0.160 264);
  --color-success:       oklch(0.700 0.160 150);  --color-success-fg: oklch(1 0 0);
  --color-warning:       oklch(0.800 0.160 80);   --color-warning-fg: oklch(0.145 0.010 264);
  --color-danger:        oklch(0.637 0.237 25);   --color-danger-fg:  oklch(1 0 0);
  --color-ring:          oklch(0.560 0.190 264);  /* focus — WCAG 2.4.7 + 1.4.11 (≥3:1) */

  /* SPACE — 8pt scale (rem; 1rem = 16px), --space-1 = 4px half-step */
  --space-1: 0.25rem; --space-2: 0.5rem;  --space-3: 0.75rem; --space-4: 1rem;
  --space-6: 1.5rem;  --space-8: 2rem;    --space-12: 3rem;   --space-16: 4rem;
  --space-24: 6rem;   --space-32: 8rem;   /* section rhythm ≈ 96px / hero ≈ 128px */

  /* RADIUS — intentional hierarchy, NOT uniform 16px; nest concentrically */
  --radius-sm: 0.25rem; --radius-md: 0.5rem; --radius-lg: 0.75rem;
  --radius-xl: 1rem;    --radius-2xl: 1.5rem; --radius-full: 9999px;

  /* TYPE — one display + body + mono; fluid clamp(rem + vw); tight display tracking */
  --font-display: "Bricolage Grotesque", var(--font-body), sans-serif;
  --font-body:    "Satoshi", system-ui, -apple-system, "Segoe UI", sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;
  --text-xs:   clamp(0.75rem, 0.72rem + 0.15vw, 0.83rem);
  --text-sm:   clamp(0.875rem, 0.84rem + 0.18vw, 0.95rem);
  --text-base: clamp(1rem, 0.91rem + 0.43vw, 1.125rem);   /* body — floor 16px */
  --text-lg:   clamp(1.125rem, 1.02rem + 0.52vw, 1.35rem);
  --text-2xl:  clamp(1.62rem, 1.40rem + 1.10vw, 2.05rem);
  --text-4xl:  clamp(2.34rem, 1.90rem + 2.20vw, 3.30rem);
  --text-7xl:  clamp(4.00rem, 2.74rem + 6.30vw, 6.75rem);  /* display */
  --leading-tight: 1.15; --leading-snug: 1.3; --leading-normal: 1.5;
  --leading-relaxed: 1.65; --leading-ja: 1.8;   /* 和文 wants 1.6–1.9 */
  --tracking-tighter: -0.03em; --tracking-tight: -0.015em;
  --tracking-normal: 0; --tracking-wide: 0.02em; --tracking-caps: 0.08em;

  /* ELEVATION — hairline-first; shadows layered, low-opacity, for floating layers only */
  --shadow-sm: 0 1px 2px -1px oklch(0 0 0 / 0.08), 0 1px 1px oklch(0 0 0 / 0.04);
  --shadow-md: 0 2px 4px -2px oklch(0 0 0 / 0.10), 0 4px 8px -2px oklch(0 0 0 / 0.06);
  --shadow-lg: 0 4px 8px -3px oklch(0 0 0 / 0.10), 0 12px 20px -4px oklch(0 0 0 / 0.08);
  --shadow-xl: 0 8px 16px -4px oklch(0 0 0 / 0.12), 0 24px 40px -8px oklch(0 0 0 / 0.10);

  /* MOTION — chosen easing + duration; spring via CSS linear() approximation */
  --ease-out:    cubic-bezier(0.16, 1, 0.3, 1);   /* expo-out: snappy entrance */
  --ease-in-out: cubic-bezier(0.65, 0, 0.35, 1);  /* in-place state change */
  --ease-spring: linear(0, 0.006, 0.101, 0.539, 0.826, 1.041, 1.057, 1.011, 0.997, 1);
  --dur-fast: 150ms; --dur-base: 250ms; --dur-slow: 400ms;
}

/* DARK — re-derive L & C (raise text L, raise surface L per layer, +C on accent).
   NOT a naive invert. Depth = lighter surfaces, since shadows barely read on dark. */
[data-theme="dark"], .dark {
  color-scheme: dark;
  --color-bg:            oklch(0.180 0.014 264);  /* tinted near-black, never #000 */
  --color-bg-subtle:     oklch(0.150 0.012 264);
  --color-surface:       oklch(0.210 0.015 264);
  --color-surface-2:     oklch(0.250 0.016 264);
  --color-fg:            oklch(0.945 0.010 264);
  --color-fg-muted:      oklch(0.730 0.014 264);
  --color-fg-subtle:     oklch(0.560 0.014 264);
  --color-border:        oklch(1 0 0 / 0.10);     /* low-alpha white hairline */
  --color-border-subtle: oklch(1 0 0 / 0.06);
  --color-accent:        oklch(0.720 0.225 264);  /* +L +C so it lifts off dark */
  --color-accent-hover:  oklch(0.785 0.215 264);
  --color-success:       oklch(0.760 0.180 150);
  --color-warning:       oklch(0.850 0.175 80);
  --color-danger:        oklch(0.710 0.250 27);   /* hue nudged toward source */
  --color-ring:          oklch(0.760 0.215 264);
}

/* Reduced-motion baseline every page ships */
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important; animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important; scroll-behavior: auto !important;
  }
}
```

> **Tailwind v4 caveat:** a `--color-*` token cannot reference itself. Put runtime values under a
> `--ds-*` alias on `:root`/`.dark`, then `@theme inline { --color-bg: var(--ds-bg); … }` so a single
> `.dark` class re-themes every utility live. Use plain `@theme` for static scales (type/space/radius/
> shadow/motion). Full mirror in [`./foundations/tokens.md`](./foundations/tokens.md) §9.

---

## 6. Motion at a glance

Motion communicates **state, attention, or personality** — never decoration. If an animation
answers none of those, delete it. Full philosophy + the spring cheat-sheet:
[`./motion/principles.md`](./motion/principles.md); copy-paste patterns (page-load, scroll reveal,
FLIP/`layoutId`, `AnimatePresence`, `@starting-style`, View Transitions):
[`./motion/recipes.md`](./motion/recipes.md).

**The laws:** (1) motion has a job; (2) **one orchestrated entrance beats a dozen
micro-interactions** — lead with a single staggered page-load (header → hero → CTA, resolving on
the focal point), then keep the rest restrained; (3) **springs for user-driven** motion (drags,
toggles, sheets — they carry gesture velocity and re-target mid-flight), **easing/linear for
automatic** motion (spinners, progress, reveals); (4) animate **`transform` + `opacity` only**
(compositor-only, 60fps) — never `width`/`height`/`top`/`left`/`box-shadow`/`filter` in a hot path;
(5) **ease-out enters, ease-in exits, ease-in-out moves in place**; duration scales with distance;
(6) **`prefers-reduced-motion` is mandatory and means *reduce*, not *remove*** — swap travel for a
cross-fade so meaning survives; (7) keep everything **interruptible**. Spring bounce: `0` for
exits/utilitarian nav, `0.15–0.2` the safe UI default, `0.3–0.4` for hero moments only.

### The signature recipe — one orchestrated, staggered page-load

```tsx
"use client";
import { motion, useReducedMotion } from "motion/react";

const container = { hidden: {}, show: { transition: { staggerChildren: 0.08, delayChildren: 0.05 } } };
const item = {
  hidden: { opacity: 0, y: 16 },
  // duration + easing are CHOSEN, mirroring --dur-base / a low-bounce spring
  show: { opacity: 1, y: 0, transition: { type: "spring", stiffness: 200, damping: 26, bounce: 0.05 } },
};

export function PageLoad({ children }: { children: React.ReactNode[] }) {
  const reduce = useReducedMotion();              // reduced motion: cross-fade, no travel
  const v = reduce ? { hidden: { opacity: 0 }, show: { opacity: 1 } } : item;
  return (
    <motion.div variants={reduce ? undefined : container} initial="hidden" animate="show">
      {children.map((child, i) => <motion.div key={i} variants={v}>{child}</motion.div>)}
    </motion.div>
  );
}
```

```css
/* CSS-only equivalent (no JS) — @starting-style fires on first paint; --i drives the stagger */
.reveal { opacity: 1; transform: translateY(0);
  transition: opacity var(--dur-base) var(--ease-out), transform var(--dur-base) var(--ease-out);
  transition-delay: calc(var(--i, 0) * 70ms); }
@starting-style { .reveal { opacity: 0; transform: translateY(20px); } }
```

> Keep the cascade under ~600ms (stagger 50–100ms, travel ≤ 24px). Reveal **once**, never on every
> scroll-by. For below-the-fold use CSS `animation-timeline: view()` behind `@supports`, or Motion's
> `whileInView={{ once: true }}`. Page/route changes: the View Transitions API (feature-detect).

---

## 7. Components & patterns at a glance

A component is **not done when it looks right — it is done when its states and a11y contract are
specified.** Full anatomy + every state + framework-agnostic and React 19 / Base UI code for
button · card · input/field · select · dialog · tabs · tooltip · badge · toast:
[`./components/components.md`](./components/components.md). Page composition — navbar · editorial
hero · bento · feature sections · pricing · CTA band · footer · background atmosphere:
[`./patterns/patterns.md`](./patterns/patterns.md).

**The shared rules (every component):**

| Rule | Mechanism |
|---|---|
| Focus ring is non-negotiable | `:focus-visible { outline: 2px solid var(--color-ring); outline-offset: 2px }` |
| 44px min target for primary controls | `min-block-size` on buttons/inputs |
| Depth = hairline + surface ladder, not heavy shadows | `1px solid var(--color-border)` over `--shadow-*` |
| Motion ≤ 250ms, transform/opacity only | `--dur-fast` + `--ease-out`; honor reduced-motion |
| Never signal by color alone (WCAG 1.4.1) | pair color with icon + text |
| `disabled` ≠ removed (AT must announce) | `disabled`/`aria-disabled`, not `display:none` |
| Consume only semantic tokens | zero hardcoded hex/px/ms |

**Every state, every time** — `default · hover · active · focus-visible · disabled · loading ·
error`. Loading = a **skeleton** that matches the real footprint (`aria-busy`, not just a spinner);
**empty** = icon + headline + guidance + one primary action (`role="status"`, real copy, never "No
data"); **error** = inline `role="alert"` with what happened + how to fix + a focusable retry.
Build on native semantics (`<button>`, `<label>`, `<dialog showModal()>`); ARIA only fills gaps.

**The anti-slop composition rules** (differentiate at the *layout* level — a themed button on a
slop layout is still slop):

1. **One focal point per screen.** Hierarchy from contrast (size·weight·color·space), not decoration.
2. **Asymmetry over symmetry.** Left-anchored editorial hero (7/5 or 8/4), one primary CTA + one
   quiet text link — never the centered eyebrow + 64px + two equal buttons.
3. **Bento where size = importance.** One 2×2 focal cell, mix outcome + feature cards, ≤12–15 cells,
   hairlines not per-card shadows. Never three identical icon-on-top cards.
4. **One accent like punctuation.** Solid `--color-fg` headings; emphasis is tonal
   (`--color-fg-subtle`), **never gradient text** on headings/metrics (protect scannability).
5. **Depth = hairline borders + surface ladder.** Shadow only on genuinely floating elements
   (the one featured pricing plan, a popover). No cards-in-cards, no one-sided thick colored border.
6. **Intentional radius scale** mapped to role (`sm` chips → `xl/2xl` panels), nested concentrically.
7. **Atmosphere is layered + subtractive** — deep tinted base → **one** color layer (mesh **or**
   aurora, masked) → optional dot/line grid → **grain on top** (0.04–0.10) to kill banding. Tint
   every layer toward the one accent; a literal rainbow mesh is the slop tell. Concentrate the loud
   atmosphere in the hero and the one CTA band only.

---

## 8. Accessibility baseline (WCAG 2.2 AA essentials)

Accessibility is a **contract every token, component, and pattern honors** — not a final QA pass.
**WCAG 2.2 AA is the legal floor** (EN 301 549 / European Accessibility Act, ADA case law, Section
508); APCA is the readability ceiling you tune toward. Automated tools catch only ~30–40% of success
criteria — manual + screen-reader testing is non-negotiable. Full recipes (focus, keyboard, forms,
dialogs, live regions, 日本語 a11y, forced-colors): [`./accessibility/a11y.md`](./accessibility/a11y.md).

| Concern | Do this | SC |
|---|---|---|
| Text contrast | 4.5:1 body, 3:1 large (≥24px or ≥18.66px bold) | 1.4.3 |
| UI / icon / focus-ring contrast | 3:1 against adjacent colors | 1.4.11 |
| Visible focus | `:focus-visible` + `--color-ring`, 2px + 2px offset; never `outline:none` alone | 2.4.7, 2.4.13 |
| Focus not obscured | sticky headers don't hide the focused element; set `scroll-margin` | 2.4.11 |
| Pointer target size | ≥24×24px (AA); aim ≥44×44px (AAA / Apple HIG) | 2.5.8, 2.5.5 |
| Keyboard | everything operable, no traps, logical DOM order, no positive `tabindex` | 2.1.1, 2.1.2 |
| Semantics | native element first; ARIA only fills gaps; accessible name on every control | 4.1.2 |
| Forms | programmatic label (not placeholder) + `aria-describedby`/`aria-invalid` error; `autocomplete` | 1.3.1, 3.3.1, 1.3.5 |
| Auth | **never disable paste / block password managers** on password/OTP fields | 3.3.8 |
| Motion | honor `prefers-reduced-motion`; nothing flashes >3×/sec | 2.3.3, 2.3.1 |
| Don't rely on color | pair color with text/icon/shape | 1.4.1 |

```css
/* The two lines every project ships: keyboard-only ring + don't-fight-the-system in forced colors */
:where(a, button, input, select, textarea, [tabindex]):focus-visible {
  outline: 2px solid var(--color-ring); outline-offset: 2px; border-radius: inherit;
}
:focus:not(:focus-visible) { outline: none; }
@media (forced-colors: active) { :focus-visible { outline: 2px solid Highlight; } }
```

Dialogs: prefer native `<dialog showModal()>` (top layer + backdrop + `Esc` + focus trap free); move
focus in on open, **restore to the trigger on close**. Set `<html lang>` (`lang="ja"` for Japanese,
mark English spans) so AT picks the right voice. Errors/loading/empty each get the correct live-region
role (`alert` / `status` busy / plain `status`). Ship `axe-core`/Lighthouse in CI as a floor, then
test with VoiceOver/NVDA + keyboard-only.

---

## 9. The recommended 2026 tech stack (decision summary)

**The default, not a survey:** `Next.js 16 (App Router) + React 19 + Tailwind v4 + shadcn/ui (Base UI
primitives) + TanStack Query + Zustand`, deployed on **Vercel**, generated/edited by **v0 grounded in
a shadcn registry over MCP**. Deviate only for the workload called out. Full WHY + versions + setup:
[`./tech-stack-2026/stack.md`](./tech-stack-2026/stack.md).

| Layer | Pick | One-line WHY |
|---|---|---|
| UI runtime | **React 19.2** | Actions, `use()`, RSC, Compiler 1.0 — all stable; `ref` is a prop |
| Meta-framework | **Next.js 16** (App Router) | Most mature RSC; Turbopack default; Cache Components (`"use cache"`) |
| Styling | **Tailwind CSS v4** | CSS-first `@theme`, OKLCH, runtime CSS vars — **the styling layer is the token layer** |
| Components | **shadcn/ui** (copy-paste) | You own the source — AI edits real code, no version drift |
| Primitives | **Base UI** (new) / Radix (existing) | Headless, accessible, render-prop composition |
| Server-state | **TanStack Query v5** | Suspense-first; design loading/error/empty |
| Client-state | **Zustand v5** | Zero-boilerplate, selector subs; **always update immutably** |
| Fonts | **`next/font`** + variable fonts | Self-hosted, zero CLS, GDPR-safe |
| Deploy | **Vercel** (default) / Cloudflare (cost) | Fluid Compute vs V8 isolates |
| AI codegen | **v0 + shadcn registry + MCP** | the **token contract is the bridge** to design-system-grounded generation |

**Escape hatches:** content/marketing/docs → **Astro 6**; no-lock-in type-safe full-stack →
**TanStack Start v1** on **Vite 8**; migrating off Remix → **React Router v7**; cost-sensitive global
edge → Next via **OpenNext on Cloudflare**; contractual a11y → **React Aria** at the primitive layer.

> **Why this exact stack:** it is what models *generate well* and can be *grounded* on. A shadcn
> registry passes your real components + tokens to v0/Cursor/Claude Code over MCP — keep it
> private/internal for proprietary systems (public registries are a supply-chain target).

---

## 10. How to operate

**The three dials** (Anthropic's "right altitude" — set them per task, never leave them implicit):

1. **Direction dial — purpose & tone.** Declare purpose, a tone *extreme*, real constraints, and two
   named anchors to triangulate *before* any CSS. Too high ("build a landing page") returns the
   median; too low (pixel-locked specs) goes visually dead. Aim at *what to think about*.
2. **Token dial — depth of theming.** Map shadcn/Tailwind onto the contract (`--color-*`,
   `--space-*`, `--radius-*`, `--font-*`, `--ease-*`, `--dur-*`). The default `slate`/`zinc` palette
   + 16px-everywhere radius is an instant slop tell. Theme, never ship raw.
3. **Motion dial — restraint vs expression.** One orchestrated load + once-only scroll reveals +
   restrained hover/active is the budget. Bounce `0.15–0.2` for "pro," up to `0.3–0.4` for one hero
   moment. Everything interruptible, everything honoring reduced-motion.

**Tiered enforcement** (cheap → expensive, all required):

- **Tier 0 — mechanical CI guard.** Fail the build on the cheapest tells (banned default fonts,
  purple→blue gradient, `0.1`-opacity shadow, `bg-indigo-500`). A smoke alarm, not a judge. (Script
  in [`./00-philosophy/human-not-ai.md`](./00-philosophy/human-not-ai.md).)
- **Tier 1 — Pre-Flight checklist.** Run the Anti-AI-Slop Checklist (typography · color · layout ·
  motion · substance · whole) before shipping. Every ✗ is a tell to fix.
- **Tier 2 — generator/evaluator gate.** A fresh-context evaluator scores the four criteria; ship
  only at ≥ 8/10 each with zero tells. Never let the generator grade itself.
- **Tier 3 — the swap-test + 800% zoom.** Could this be any other product? Then differentiate. Then
  refine optical alignment and spacing rhythm at zoom.

**Locale-awareness.** Default Latin assumptions break 日本語: set `<html lang="ja">`, line-height
1.6–1.9, gentle tracking, `palt`, `line-break: strict`, **no italics** (傍点 instead), system fonts
for body + webfont for headings only (CJK files are huge). Mark mixed English spans `lang="en"`.

**The five skills** (operate the system; invoke with a leading slash):

| Skill | Use it to… |
|---|---|
| **`awesome-ds`** | Apply this design system to a UI task — the always-on taste + token layer. |
| **`make-awesome-ds`** | Build a brand-specific design system from the contract (start: tokens + philosophy). |
| **`awesome-html`** | Produce a polished single-file HTML doc (tokens + type + motion recipes). |
| **`awesome-motion`** | Add orchestrated, accessible motion (the principles + recipes layer). |
| **`awesome-review`** | Run the generator/evaluator gate — grade against the four criteria + checklist. |

> **The one rule under all of it:** be an **author, not an operator.** Supply a point of view and AI
> is a force multiplier; supply nothing and it returns the median, dressed up nicely. Re-run the
> checklist even on work that was *supposed* to be safe — the gravity toward the median is constant.

---

## Sources

The primary sources behind the verdict, consolidated. Each deep module carries its own fuller list;
these are the load-bearing ones an agent should be able to cite.

**Anti-slop · taste · grading (Anthropic + critique)**
- Prompting for frontend aesthetics — Claude Cookbook — https://platform.claude.com/cookbook/coding-prompting-for-frontend-aesthetics
- Improving frontend design through Skills (four criteria; generator/evaluator split) — Claude — https://claude.com/blog/improving-frontend-design-through-skills
- Harness design for long-running application development — Anthropic Engineering — https://www.anthropic.com/engineering/harness-design-long-running-apps
- Why Your AI Keeps Building the Same Purple Gradient Website — prg.sh — https://prg.sh/ramblings/Why-Your-AI-Keeps-Building-the-Same-Purple-Gradient-Website
- AI Slop Web Design: Complete Guide (2026) — 925studios — https://www.925studios.co/blog/ai-slop-web-design-guide
- Why Slop Matters — Kommers et al. (MINT Lab, Indiana University), arXiv:2601.06060 — https://arxiv.org/abs/2601.06060
- Design Trends 2026: Imperfection, Rebellion, and the Return of Human Work — Lindsay Marsh — https://lindsaymarsh.substack.com/p/design-trends-2026-imperfection-rebellion

**Modern 2026 — the four poles**
- A calmer interface for a product in motion (Linear refresh; "structure felt, not seen") — https://linear.app/now/behind-the-latest-design-refresh
- How we redesigned the Linear UI (Part II, LCH theme engine) — https://linear.app/now/how-we-redesigned-the-linear-ui
- Geist — Introduction / Colors / Typography (Vercel) — https://vercel.com/geist/introduction
- Raycast DESIGN.md (surface ladder, no drop shadows) — https://github.com/VoltAgent/awesome-design-md/blob/main/design-md/raycast/DESIGN.md
- Liquid Glass | Apple Developer — https://developer.apple.com/documentation/TechnologyOverviews/liquid-glass
- Apple — Human Interface Guidelines (Clarity / Deference / Depth; motion; layout) — https://developer.apple.com/design/human-interface-guidelines/
- Animate with springs — WWDC23 session 10158 — https://developer.apple.com/videos/play/wwdc2023/10158/
- M3 Expressive: New Motion System (Google) — https://m3.material.io/blog/m3-expressive-motion-theming
- Material 3 — Color system / how it works (HCT) — https://m3.material.io/styles/color/system/how-the-system-works
- ndot57: the Nothing Typeface — https://nothing.community/d/104-ndot57-the-nothing-typeface

**Philosophy lineage**
- Goodpatch Global — "Less but More: The Minimum Lovable Product (MLP) Mindset" — https://medium.com/goodpatch-global/less-but-more-the-minimum-lovable-product-mindset-46b1e09f6f53

**Color · contrast**
- MDN — `oklch()` (CSS Color 4) — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/oklch
- MDN — `contrast-color()` — https://developer.mozilla.org/en-US/docs/Web/CSS/color_value/contrast-color
- oklch.com — OKLCH Color Picker & Converter — https://oklch.com/
- APCA in a Nutshell (Lc levels) — https://git.apcacontrast.com/documentation/APCA_in_a_Nutshell.html
- Adrian Roselli — WCAG3 Contrast as of April 2026 — https://adrianroselli.com/2026/04/wcag3-contrast-as-of-april-2026.html
- web.dev — Color themes with Baseline CSS — https://web.dev/articles/baseline-in-action-color-theme

**Typography**
- MDN — Variable fonts guide — https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_fonts/Variable_fonts_guide
- Smashing Magazine — Modern Fluid Typography Using CSS clamp() — https://www.smashingmagazine.com/2022/01/modern-fluid-typography-css-clamp/
- Utopia — Fluid type & space calculator — https://utopia.fyi/
- WCAG 1.4.4 & fluid type caveat — Adrian Roselli — https://adrianroselli.com/2019/12/responsive-type-and-zoom.html
- Custom fonts without compromise using next/font — Vercel — https://vercel.com/blog/nextjs-next-font
- The Most Comprehensive Guide to Web Typography in Japanese — Masaharu Hayataki — https://medium.com/@masaharuhayataki/japanese-web-typography-anatomy-and-best-practices-185449b7be65
- Fontshare (Satoshi, General Sans, Clash Display) — ITF — https://www.fontshare.com/

**Spacing · layout · tokens**
- IBM Carbon — Spacing overview (8px scale) — https://carbondesignsystem.com/elements/spacing/overview/
- Material Design 3 — Design tokens overview (ref/sys/comp) — https://m3.material.io/foundations/design-tokens/overview
- Apple — HIG Layout (safe areas, 44pt targets, concentricity) — https://developer.apple.com/design/human-interface-guidelines/layout
- Bento Grids Quietly Winning B2B SaaS Homepages 2026 — https://www.pravinkumar.co/blog/bento-grids-b2b-saas-homepage-design-trend-2026
- Tailwind CSS — Theme variables (`@theme`) — https://tailwindcss.com/docs/theme
- Evil Martians — Better dynamic themes in Tailwind with OKLCH — https://evilmartians.com/chronicles/better-dynamic-themes-in-tailwind-with-oklch-color-magic

**Motion**
- Motion for React — Transitions (spring `visualDuration`/`bounce`, stagger) — https://motion.dev/docs/react-transitions
- Apple HIG — Motion — https://developer.apple.com/design/human-interface-guidelines/motion
- Material Design 3 — Easing & duration tokens/specs — https://m3.material.io/styles/motion/easing-and-duration/tokens-specs
- MDN — `@starting-style` — https://developer.mozilla.org/en-US/docs/Web/CSS/@starting-style
- MDN — View Transition API — https://developer.mozilla.org/en-US/docs/Web/API/View_Transition_API
- MDN — `prefers-reduced-motion` — https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-reduced-motion

**Components · accessibility**
- WCAG 2.2 (W3C Recommendation) — https://www.w3.org/TR/WCAG22/
- WCAG 2.2 — What's New — https://www.w3.org/WAI/standards-guidelines/wcag/new-in-22/
- W3C — ARIA Authoring Practices Guide (APG) — https://www.w3.org/WAI/ARIA/apg/
- MDN — `<dialog>` element (top layer, focus trap) — https://developer.mozilla.org/en-US/docs/Web/HTML/Element/dialog
- WebAIM — Contrast & Color Accessibility — https://webaim.org/articles/contrast/
- shadcn/ui — Tailwind v4 — https://ui.shadcn.com/docs/tailwind-v4
- Base UI (MUI) — https://github.com/mui/base-ui

**2026 stack**
- React v19 — https://react.dev/blog/2024/12/05/react-19 · React 19.2 — https://react.dev/blog/2025/10/01/react-19-2
- Next.js 16 release — https://nextjs.org/blog/next-16
- Tailwind CSS v4.0 — https://tailwindcss.com/blog/tailwindcss-v4
- shadcn/ui Registry MCP Server — https://ui.shadcn.com/docs/registry/mcp
- TanStack Query v5 — https://tanstack.com/blog/announcing-tanstack-query-v5
- Astro 6.0 — https://astro.build/blog/astro-6/
- v0 Design systems — https://v0.app/docs/design-systems
