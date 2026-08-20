# What "Modern" Means in 2026

> The philosophy module of this design system. Read this first — every other module
> inherits the verdict here. Captured **June 2026**, the one-year mark of Apple's Liquid Glass
> (WWDC25, June 9 2025) and Google's Material 3 Expressive (Google I/O, May 13 2025).
> AI agents: this is canonical. Build from these traits and tokens directly; do not re-fetch.

---

## 1. The verdict in one paragraph

**Modern in 2026 is calm, dark-first, near-monochrome confidence with physical motion.**
One dominant surface family (a *tinted* near-black, never `#000`), depth built from **1px
hairline borders + a surface ladder** instead of stacked drop shadows, a **single semantic
accent used like punctuation**, large headlines tightened to **-0.04em** tracking, and color
defined in **perceptually-uniform space (OKLCH / HCT / LCH)** so light and dark are *re-derived*,
never naively inverted. On top of that calm substrate sits **spring/physics motion** — Apple's
velocity-continuous springs and Google's re-targetable physics system both replaced the old
duration+easing model as the *primary* motion language. The taste rule underneath all of it:
**restraint is confidence.** Subtractive color, one focal point per screen, motion that
communicates state — not decoration. The frontier (Linear, Vercel, Raycast) keeps getting
*less* colorful and *less* noisy on purpose; "Structure should be felt, not seen." ([Linear](https://linear.app/now/behind-the-latest-design-refresh))

---

## 2. The four poles we triangulate

Modern 2026 is the *intersection* of four design cultures that all arrived — independently —
at depth-through-restraint. Each contributes one load-bearing idea.

| Pole | Signature | The idea we keep | The trap we drop |
|---|---|---|---|
| **Apple** — Liquid Glass | Translucent, refracting, gyro-reactive *navigation layer* floating above content; velocity-continuous springs; concentric corners. | Depth + physicality + content-first deference. Glass on the **chrome layer only**. | Glass on main content / glass-on-glass; over-translucency that kills legibility. |
| **Google** — M3 Expressive | Physics/spring motion (re-targetable mid-gesture), shape morphing, HCT dynamic color, tonal surfaces. | Motion as **interruptible physics**; perceptual color; tonal elevation over shadow. | Bouncy-everywhere; Roboto-by-default; rainbow dynamic-color on a brand product. |
| **Nothing** — Monochrome restraint | Grayscale canvas, hierarchy via **opacity tiers**, dot-grid substrate, **red as an event** not decoration, exposed structure. | Color is semantic and *rare*; monochrome discipline; honest, un-skeuomorphic surfaces. | Literal dot-matrix everywhere; novelty over usability. |
| **SaaS frontier** — Linear / Vercel / Raycast | Calm dark near-monochrome, hairline borders, one accent, bespoke type (Geist), surface ladders, no drop shadows. | Hairlines over shadows; subtractive color; tracked confident type; dark-native tokens. | Cold dev-tool minimalism copied onto warm consumer products. |

**The synthesis:** Nothing + the SaaS frontier give you the *calm restrained substrate*;
Apple + Google give you the *physical motion and depth* layered on top. A modern 2026 product
is a near-monochrome calm canvas with one accent, hairline structure, and spring-driven,
purposeful motion — themed, never shipped raw.

---

## 3. The recurring 2026 traits (the canon)

These nine traits recur across all four poles. Treat them as the definition of "modern" — if a
screen has most of them, it reads 2026; if it has the dated tells in §5, it reads 2021.

1. **Dark-first, tinted — never pure black.** Canvas is the brand hue at ~4–8% lightness or a
   warm/cool deep gray. Pure `#000` + light text makes type "vibrate." Build dark-native tokens;
   *do not* invert the light palette. (Linear backgrounds sit at 1–10% lightness; Raycast canvas `#07080a`.)
2. **Hairline borders + surface ladder = depth.** A 1px (0.5px on retina) border and a
   surface-color step do the work drop shadows used to. Shadows are *low-opacity, layered, optional* —
   used for genuinely floating elements, not every card. (Raycast ships *no* drop shadows at all.)
3. **OKLCH / perceptual color.** Define color in OKLCH (web), mirroring how Apple, Google (HCT)
   and Linear (LCH) all moved off sRGB/HSL. Lightness drives contrast; you adjust **L and C**
   for dark mode, never a naive invert. A red and a blue at the same L *read* equally bright.
4. **One semantic accent, used like punctuation.** Color carries meaning (accent / success /
   warning / danger), never decoration. Nothing's red-as-event and Vercel's single blue are the
   same discipline. Restraint signals confidence.
5. **Large headlines, tightened tracking.** Display type at ~48–72px with **-0.04em** letter-spacing
   and ~1.05–1.15 line-height is *the* marketing tell of the era. Body stays at normal tracking.
6. **Physics / spring motion.** Springs are the default for *user-driven* motion (taps, drags,
   toggles, sheets) because they preserve velocity and can be re-targeted mid-flight. Fixed-duration
   tweens stay for *continuous/automatic* motion (spinners, progress, scroll reveals). Keep bounce
   low (`bounce < 0.1`) for "pro," higher for "playful."
7. **Bento layouts.** Strict geometric grids where **cell size maps to importance**; the default
   for new B2B homepages by 2026. Mix outcome cards with feature cards; ≤12–15 cells.
8. **Tasteful glass — on the chrome layer only.** `backdrop-filter: blur()` + a noise overlay +
   subtle border, applied to nav/toolbars/sheets that *float above* content. Never on content
   itself, never stacked.
9. **Honest structure + texture.** Expose the grid (dot-grid substrates, clear lanes/columns),
   and layer **grain** (`feTurbulence` fractalNoise, opacity 0.05–0.12) over flat fills to escape
   digital banding. No faux materials, no skeuomorphic bevels.

---

## 4. What "modern" looks like in tokens (the contract)

The system speaks one framework-agnostic token vocabulary (CSS custom properties), mirrored in
Tailwind v4 `@theme`. The philosophy is *encoded* in the token values: tinted dark canvas,
OKLCH color, hairline-first elevation, tight display tracking, spring-approximating easing.

```css
/* :root = light. [data-theme="dark"] re-derives L & C — never a naive invert. */
:root {
  /* ── Color: semantic, OKLCH (perceptual) ───────────────────────────── */
  --color-bg:            oklch(0.99 0.004 250);   /* page canvas */
  --color-bg-subtle:     oklch(0.975 0.005 250);
  --color-surface:       oklch(1 0 0);            /* cards */
  --color-surface-2:     oklch(0.97 0.006 250);   /* elevated */
  --color-fg:            oklch(0.20 0.02 260);    /* primary text */
  --color-fg-muted:      oklch(0.45 0.015 260);   /* labels, metadata */
  --color-fg-subtle:     oklch(0.62 0.012 260);   /* hints, disabled */
  --color-border:        oklch(0.90 0.006 260);   /* THE depth primitive */
  --color-border-subtle: oklch(0.94 0.005 260);
  --color-accent:        oklch(0.62 0.20 256);    /* the one accent */
  --color-accent-fg:     oklch(0.99 0.01 256);
  --color-accent-hover:  oklch(0.56 0.21 256);
  --color-success:       oklch(0.68 0.17 150);   --color-success-fg: oklch(0.99 0.01 150);
  --color-warning:       oklch(0.80 0.16 80);    --color-warning-fg: oklch(0.25 0.05 80);
  --color-danger:        oklch(0.62 0.23 25);    --color-danger-fg:  oklch(0.99 0.01 25);
  --color-ring:          oklch(0.62 0.20 256);   /* focus, == accent hue */

  /* ── Space: 8pt-based, rem ──────────────────────────────────────────── */
  --space-1: 0.25rem;  --space-2: 0.5rem;  --space-3: 0.75rem; --space-4: 1rem;
  --space-6: 1.5rem;   --space-8: 2rem;    --space-12: 3rem;   --space-16: 4rem;
  --space-24: 6rem;    --space-32: 8rem;   /* big section rhythm ≈ 96px */

  /* ── Radius: intentional scale, NOT uniform 16px everywhere ─────────── */
  --radius-sm: 0.375rem; --radius-md: 0.5rem;  --radius-lg: 0.75rem;
  --radius-xl: 1rem;     --radius-2xl: 1.5rem; --radius-full: 9999px;

  /* ── Type: fluid clamp(), tight display tracking is the 2026 tell ───── */
  --font-display: "Geist", "Satoshi", ui-sans-serif, system-ui, sans-serif;
  --font-body:    "Inter", ui-sans-serif, system-ui, sans-serif;
  --font-mono:    "Geist Mono", ui-monospace, "SF Mono", monospace;
  --text-xs:  clamp(0.75rem, 0.74rem + 0.05vw, 0.78rem);  --leading-xs: 1.5;   --tracking-xs: 0;
  --text-sm:  clamp(0.875rem, 0.86rem + 0.07vw, 0.92rem); --leading-sm: 1.5;   --tracking-sm: 0;
  --text-base:clamp(1rem, 0.98rem + 0.1vw, 1.06rem);      --leading-base:1.55; --tracking-base: 0;
  --text-lg:  clamp(1.125rem, 1.08rem + 0.2vw, 1.25rem);  --leading-lg: 1.45;  --tracking-lg: -0.01em;
  --text-2xl: clamp(1.5rem, 1.35rem + 0.7vw, 2rem);       --leading-2xl:1.2;   --tracking-2xl: -0.02em;
  --text-4xl: clamp(2.25rem, 1.9rem + 1.7vw, 3.5rem);     --leading-4xl:1.1;   --tracking-4xl: -0.035em;
  --text-7xl: clamp(3.5rem, 2.6rem + 4.5vw, 6rem);        --leading-7xl:1.02;  --tracking-7xl: -0.045em;

  /* ── Elevation: hairline-first, shadows low-opacity & layered ───────── */
  --shadow-sm: 0 1px 2px -1px oklch(0.2 0.02 260 / 0.10);
  --shadow-md: 0 2px 4px -2px oklch(0.2 0.02 260 / 0.10),
               0 4px 12px -4px oklch(0.2 0.02 260 / 0.08);
  --shadow-lg: 0 8px 24px -8px oklch(0.2 0.02 260 / 0.14);
  --shadow-xl: 0 16px 48px -12px oklch(0.2 0.02 260 / 0.18);

  /* ── Motion: spring approximations + measured durations ─────────────── */
  --ease-out:     cubic-bezier(0.23, 1, 0.32, 1);   /* enter / rest */
  --ease-in-out:  cubic-bezier(0.65, 0, 0.35, 1);   /* continuous */
  /* CSS linear() spring approx ≈ stiffness 200 / damping 20 / low bounce */
  --ease-spring:  linear(0, 0.006, 0.025, 0.101, 0.539, 0.826, 0.967,
                         1.041, 1.057, 1.04, 1.011, 0.997, 0.996, 1.0);
  --dur-fast: 150ms; --dur-base: 250ms; --dur-slow: 400ms;
}

/* Dark = re-derived, not inverted. Raise L of text, lower L of surfaces,
   tame chroma so accents don't glare on a dark field. Canvas is TINTED. */
[data-theme="dark"], .dark {
  --color-bg:            oklch(0.16 0.012 260);   /* tinted near-black, not #000 */
  --color-bg-subtle:     oklch(0.18 0.013 260);
  --color-surface:       oklch(0.205 0.014 260);  /* card sits above canvas */
  --color-surface-2:     oklch(0.245 0.015 260);  /* elevated */
  --color-fg:            oklch(0.96 0.006 260);
  --color-fg-muted:      oklch(0.74 0.012 260);
  --color-fg-subtle:     oklch(0.58 0.012 260);
  --color-border:        oklch(1 0 0 / 0.10);     /* hairline = white @ low alpha */
  --color-border-subtle: oklch(1 0 0 / 0.06);
  --color-accent:        oklch(0.70 0.17 256);    /* lighter + less chroma in dark */
  --color-accent-hover:  oklch(0.76 0.16 256);
  --color-ring:          oklch(0.70 0.17 256);
  /* shadows nearly vanish in dark — depth comes from the surface ladder */
  --shadow-sm: 0 1px 2px -1px oklch(0 0 0 / 0.4);
  --shadow-md: 0 2px 8px -2px oklch(0 0 0 / 0.5);
}
```

### Tailwind v4 mirror (`@theme`)

```css
@import "tailwindcss";

@theme {
  --color-bg: var(--color-bg);
  --color-surface: var(--color-surface);
  --color-fg: var(--color-fg);
  --color-accent: var(--color-accent);
  --radius-lg: 0.75rem;
  --font-display: "Geist", ui-sans-serif, system-ui, sans-serif;
  --ease-spring: linear(0, 0.006, 0.101, 0.539, 0.826, 1.041, 1.04, 0.997, 1);
  --animate-reveal: reveal var(--dur-slow) var(--ease-out) both;
  @keyframes reveal { from { opacity: 0; transform: translateY(16px); } to { opacity: 1; transform: none; } }
}
/* dark mode driven by data-theme or .dark (override the color vars above) */
@custom-variant dark (&:where([data-theme="dark"], [data-theme="dark"] *, .dark, .dark *));
```

---

## 5. What is now DATED (the 2021 → 2026 diff)

If a screen carries these, it reads *old* — or worse, it reads as AI slop (the statistical median).
The test for slop: *if a fintech homepage could be a CRM or a PM tool with zero changes, it's slop.*

| Dated / slop tell | Why it dates the work | The 2026 move |
|---|---|---|
| Pure `#000` dark mode | Text vibrates; no surface ladder possible | Tinted near-black (`oklch(0.16 0.012 260)`) + surface steps |
| Drop shadow on every card | Flat "Material 2" depth; muddy on dark | 1px hairline + surface ladder; shadow only for floating layers |
| Blue→purple gradient on white | The single most over-produced AI default | One semantic accent; directional light if any |
| Gradient *text* on headings/metrics | Decorative; wrecks scannability | Solid-color text; hierarchy via size/weight/space |
| Inter/Roboto/system as the *only* font | The default in every AI tool | Bespoke or tracking-tuned display face (Geist, Satoshi) |
| Uniform 16px radius on everything | Safe, characterless | Intentional radius scale per element role |
| Cards nested 3–5 deep, each padded+shadowed | Most common training pattern | Flatten; one container; hairlines |
| Centered eyebrow + 64px headline + 2 identical CTAs | "Average of a million landing pages" | Real product copy; asymmetric layout; one focal point |
| Three identical emoji/icon feature cards | Universal feature-card template | Bento with varied cell sizes = importance |
| sRGB/HSL color math, naive dark invert | Inconsistent perceived brightness | OKLCH/HCT; re-derive L & C for dark |
| `ease-in`/`ease-out`/`linear` built-ins; long 600ms+ tweens everywhere | Generic, lifeless, jarring when interrupted | Springs for interaction; custom cubic-bezier; ≤400ms |
| Skeuomorphic bevels, faux glass with no noise | Flat and fake; banding | Honest surfaces; real glass = blur + noise + border |

---

## 6. Apple's contribution — depth, glass, springs (code)

Liquid Glass is a **functional navigation layer floating above content** — translucent, refracting,
adaptive in color, with specular highlights that react to device motion. Port the *principle*, not
the literal effect: glass on the **chrome layer only**, never on content, never stacked. Springs
preserve velocity continuity (a gesture's end velocity flows into the animation); concentric corners
mean a nested element's radius ≈ parent radius − padding.

```css
/* Tasteful web "glass" — chrome layer only (nav, toolbar, sheet). */
.glass-chrome {
  background: oklch(1 0 0 / 0.06);
  backdrop-filter: blur(12px) saturate(1.4);
  -webkit-backdrop-filter: blur(12px) saturate(1.4);   /* Safari: always ship the prefix */
  border: 1px solid oklch(1 0 0 / 0.12);
  border-radius: var(--radius-xl);
  position: relative;
  overflow: hidden;
}
.glass-chrome::after {                 /* noise = realism; flat blur looks fake */
  content: ""; position: absolute; inset: 0; pointer-events: none;
  background-image: url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg'>\
<filter id='g'><feTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='3' stitchTiles='stitch'/></filter>\
<rect width='100%25' height='100%25' filter='url(%23g)'/></svg>");
  opacity: 0.05; mix-blend-mode: overlay;
}

/* Concentric corners: child radius = parent radius − padding (curves stay parallel) */
.panel  { border-radius: var(--radius-2xl); padding: var(--space-4); }  /* 24px - 16px */
.panel > .inner { border-radius: var(--radius-lg); }                    /* ≈ 8px */
```

```swift
// Native Apple: glass is automatic on recompile; for custom UI, chrome layer only.
Text("Send Payment")
    .padding()
    .glassEffect(.regular.tint(.accentColor).interactive())   // scale+shimmer on touch
// Springs are the default since iOS 17 — velocity-continuous, two intuitive params:
withAnimation(.spring(duration: 0.5, bounce: 0.15)) { isExpanded.toggle() }
// .smooth (no overshoot) · .snappy (slight) · .bouncy (more) along the damping spectrum
```

---

## 7. Google's contribution — interruptible physics motion (code)

M3 Expressive **replaced duration+easing with a spring/physics system** as the primary motion
language. The reason is mechanical, not aesthetic: a spring can be **re-targeted mid-flight** —
critical for gesture-driven UIs where a fixed-duration tween feels jarring if interrupted. Two
categories: **spatial** (position/size/radius — overshoots, damping ~0.9) and **effects**
(color/opacity — no overshoot, damping 1.0). Speed rule: small/short → fast, full-screen → slow.

```kotlin
// Jetpack Compose — spatial spring (visible overshoot) vs effects spring (none)
val spatial = spring<Float>(dampingRatio = 0.9f, stiffness = 700f)   // default spatial
val effects = spring<Color>(dampingRatio = 1.0f, stiffness = 1600f)  // color/opacity, no bounce
// M3E motion scheme is "expressive" (bouncy) by default; swap to "standard" for calm products.
```

```ts
// Web equivalent with Motion (ex-framer-motion). Spring for INTERACTIVE; tween for CONTINUOUS.
import { animate } from "motion";
animate(el, { y: 0 }, { type: "spring", stiffness: 200, damping: 20 }); // re-targetable, low bounce
```

The web token `--ease-spring` (a CSS `linear()` curve) is the duration-based approximation for
cases where a real spring engine isn't available — it bakes the same low-bounce overshoot into a
pure-CSS transition. Use the real spring (Motion / SwiftUI / Compose) for gesture-driven UI; use
`--ease-spring` for cheap CSS-only transitions.

---

## 8. Nothing's contribution — monochrome discipline (code)

Nothing proves the thesis that **color is an event, not decoration**. Hierarchy comes from
*opacity tiers + scale + weight*, never hue. One chromatic color (red) is reserved for status. This
is the same discipline as Vercel's single blue — Nothing just takes it to the limit. Port the
*rule* (semantic, rare color; honest exposed structure), not necessarily the literal dot-matrix.

```css
/* Monochrome hierarchy via opacity tiers — maps onto our semantic fg tokens. */
.mono-display   { color: var(--color-fg); }          /* 1 focal element per screen */
.mono-primary   { color: var(--color-fg); opacity: 0.90; }
.mono-secondary { color: var(--color-fg-muted); }    /* labels, metadata */
.mono-disabled  { color: var(--color-fg-subtle); }   /* hints, off-states */
.status-record  { color: var(--color-danger); }      /* color ONLY for status/critical */

/* Dot-grid substrate — expose structure instead of hiding it under panels. */
.dot-grid {
  background-image: radial-gradient(var(--color-border) 1px, transparent 1px);
  background-size: 16px 16px;
  -webkit-mask-image: radial-gradient(ellipse 60% 60% at 50% 0%, #000 40%, transparent 100%);
          mask-image: radial-gradient(ellipse 60% 60% at 50% 0%, #000 40%, transparent 100%);
}
```

---

## 9. The philosophy in one component (React 19 + Tailwind v4 + Motion)

A staggered page-load reveal — *one orchestrated entrance beats scattered micro-interactions* —
that respects `prefers-reduced-motion` and uses the spring/easing tokens above. This is the
"feel" of modern 2026 in ~40 lines: calm surface, hairline border, one accent, low-bounce spring.

```tsx
"use client";
import { motion, useReducedMotion } from "motion/react";

const container = {
  hidden: {},
  show: { transition: { staggerChildren: 0.06, delayChildren: 0.04 } },
};
const item = {
  hidden: { opacity: 0, y: 16 },
  show: { opacity: 1, y: 0, transition: { type: "spring", stiffness: 200, damping: 22 } },
};

export function PhilosophyHero() {
  const reduce = useReducedMotion();
  // Reduced motion: keep the orchestration but swap travel for a pure cross-fade.
  const v = reduce ? { hidden: { opacity: 0 }, show: { opacity: 1 } } : item;

  return (
    <motion.section
      variants={container} initial="hidden" animate="show"
      className="mx-auto max-w-3xl px-6 py-24 [font-family:var(--font-display)]"
    >
      <motion.p variants={v} className="text-sm font-medium text-[--color-accent]">
        Design System · Philosophy
      </motion.p>
      <motion.h1
        variants={v}
        className="mt-3 text-4xl font-semibold tracking-[-0.04em] text-[--color-fg] sm:text-7xl"
      >
        Calm, dark-first, physical.
      </motion.h1>
      <motion.p variants={v} className="mt-5 max-w-xl text-lg leading-relaxed text-[--color-fg-muted]">
        Restraint is confidence. One accent, hairline structure, spring motion that communicates state.
      </motion.p>
      <motion.div variants={v} className="mt-8">
        <button
          className="rounded-[--radius-lg] border border-[--color-border] bg-[--color-accent]
                     px-5 py-2.5 text-sm font-medium text-[--color-accent-fg]
                     transition-[transform,background-color] duration-150 ease-[--ease-out]
                     hover:bg-[--color-accent-hover] active:scale-[0.98]
                     focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[--color-ring]
                     focus-visible:ring-offset-2 focus-visible:ring-offset-[--color-bg]
                     motion-reduce:transition-none motion-reduce:active:scale-100"
        >
          Read the system
        </button>
      </motion.div>
    </motion.section>
  );
}
```

---

## 10. Non-negotiables: accessibility, states, themes

Modern is not modern if it only renders the happy path. Every module in this system MUST design:

- **Error / empty / loading states** — never just the populated view. Loading = skeleton or the
  spring-friendly indeterminate state; empty = guidance + one primary action; error = a
  user-friendly message (detailed logs stay server-side), never a silent fail.
- **Focus is always visible.** `:focus-visible` with `--color-ring` at 2px + offset. Never
  `outline: none` without a replacement.
- **Contrast meets WCAG AA.** Body text ≥ 4.5:1, large text ≥ 3:1. OKLCH lightness makes this
  predictable — pair `--color-fg` on `--color-bg`, `--color-accent-fg` on `--color-accent`.
- **Honor `prefers-reduced-motion`.** Don't strip meaning — *swap* travel/scale/parallax for a
  dissolve or color shift (Apple's own approach). Keep transitions interruptible.
- **Tap targets ≥ 44×44px** (Apple) / **48×48dp** (Google). Spacing ≥ 8px between targets.
- **Theme via `[data-theme="dark"]` / `.dark` overriding the color vars** — adjust L & C, never a
  naive `invert()`. Light is `:root`; dark is re-derived.
- **shadcn / Tailwind defaults are themed, not shipped raw.** Map them onto these tokens (the raw
  `slate`/`zinc` defaults plus a 16px-everywhere radius are an instant slop tell).

```css
/* Reduced-motion baseline every page should ship */
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

---

## 11. Agent checklist — "is this modern 2026?"

1. Dark canvas is **tinted** (~4–8% L), not `#000`; dark tokens are re-derived, not inverted.
2. Depth = **1px hairline + surface ladder**; shadows reserved for genuinely floating layers.
3. Color is **semantic and OKLCH**; **one accent** used like punctuation; no gradient text.
4. Display headlines at **-0.04em** tracking, ~1.05–1.15 line-height; bespoke/tuned display font.
5. **Spring** motion for interaction (low bounce), tween for continuous; ≤400ms; honors reduced-motion.
6. Glass only on the **chrome layer** (blur + noise + border + `-webkit-` prefix); never on content.
7. **Bento** for feature sections: size = importance; mix outcome + feature cards; ≤12–15 cells.
8. Layer **grain** (fractalNoise, 0.05–0.12) over flat fills; expose structure (dot-grid / lanes).
9. Differentiate at the **layout** level, not just the colors. Real copy, real product screenshots.
10. **Error / empty / loading** states + visible focus + WCAG AA + ≥44px targets all designed.
11. Themed, never raw shadcn/Tailwind defaults. One focal point per screen.

> If a screen passes 9+ of these and avoids every dated tell in §5, it reads modern in June 2026.
> If you can swap the product name and it could be any other SaaS, go back to §5 and differentiate.

---

## Sources

- A calmer interface for a product in motion (Linear refresh) — https://linear.app/now/behind-the-latest-design-refresh
- How we redesigned the Linear UI (Part II, LCH theme engine) — https://linear.app/now/how-we-redesigned-the-linear-ui
- Geist — Introduction / Colors / Typography (Vercel) — https://vercel.com/geist/introduction
- Raycast DESIGN.md (VoltAgent/awesome-design-md) — https://github.com/VoltAgent/awesome-design-md/blob/main/design-md/raycast/DESIGN.md
- Why Linear Design Systems Break in Dark Mode (Chyshkala) — https://chyshkala.com/blog/why-linear-design-systems-break-in-dark-mode-and-how-to-fix-them
- Connect: behind the front-end experience (Stripe motion rules) — https://stripe.com/blog/connect-front-end-experience
- Grainy Gradients (CSS-Tricks) — https://css-tricks.com/grainy-gradients/
- Bento Grids Quietly Winning B2B SaaS Homepages 2026 (Pravin Kumar) — https://www.pravinkumar.co/blog/bento-grids-b2b-saas-homepage-design-trend-2026
- Slop (Impeccable) — https://impeccable.style/slop/
- AI Slop Web Design: Complete Guide (925studios) — https://www.925studios.co/blog/ai-slop-web-design-guide
- React transitions (Motion docs) — https://motion.dev/docs/react-transitions
- Liquid Glass | Apple Developer (Technology Overviews) — https://developer.apple.com/documentation/TechnologyOverviews/liquid-glass
- Human Interface Guidelines — https://developer.apple.com/design/human-interface-guidelines/
- Motion | Apple HIG — https://developer.apple.com/design/human-interface-guidelines/motion
- Animate with springs — WWDC23 session 10158 — https://developer.apple.com/videos/play/wwdc2023/10158/
- Apple introduces a delightful and elegant new software design (Newsroom, Jun 9 2025) — https://www.apple.com/newsroom/2025/06/apple-introduces-a-delightful-and-elegant-new-software-design/
- M3 Expressive: New Motion System (Google) — https://m3.material.io/blog/m3-expressive-motion-theming
- Motion overview / physics system (Material 3) — https://m3.material.io/styles/motion/overview/how-it-works
- Color system / how it works (HCT) — https://m3.material.io/styles/color/system/how-the-system-works
- Material Color Utilities — https://github.com/material-foundation/material-color-utilities
- Forget Bloatware: Nothing OS is Winning by Design — https://insights.made-in-china.com/Forget-Bloatware-Nothing-OS-is-Winning-by-Design_HTlfpDbPFnIJ.html
- ndot57: the Nothing Typeface (Nothing Community) — https://nothing.community/d/104-ndot57-the-nothing-typeface
- Nothing Phone (3)'s Glyph Matrix (Adam Bates, "less distraction, more meaning") — https://design-milk.com/the-nothing-phone-3s-glyph-matrix-turns-notifications-into-pixel-art/
- Nothing Design: Transparency & Brand Philosophy (Mugen Brands) — https://mugen-brands.com/en/nothing-design-transparency-brand-philosophy/
