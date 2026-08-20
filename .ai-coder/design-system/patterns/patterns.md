# Composition Patterns

> The canonical, copy-paste verdict for **marketing-page composition** in this design system.
> Editorial hero, bento grid, feature sections, pricing, sticky navbar, footer, CTA band, and the
> background atmosphere recipes (mesh, grain, dot-grid, aurora). Framework-agnostic HTML/CSS first,
> React 19 + Tailwind v4 + Motion alongside. Every pattern uses the **canonical token contract**.
>
> Distilled from the Linear / Vercel / Stripe / Raycast SaaS lineage, Apple's HIG (deference,
> depth, springs), and Nothing's monochrome restraint. June 2026.

---

## 0. The thesis (why these patterns, not the slop ones)

The statistical median marketing page is: centered eyebrow → 64px headline → two CTAs, three
identical icon-on-top feature cards, a blue→purple gradient on white, uniform 16px radius
everywhere. **Every pattern below is deliberately the opposite of that.** Our rules:

1. **One focal point per screen.** Hierarchy comes from contrast (size, weight, color, space), not
   from decoration. Restraint reads as confidence.
2. **Asymmetry over symmetry.** Editorial layouts (off-center, left-anchored, lane-based) beat the
   safe centered stack. Differentiate at the *layout* level — a themed button on a slop layout is
   still slop.
3. **Depth = hairline borders + a surface ladder**, not drop shadows on everything. Shadows are
   reserved low-opacity, layered, and rare.
4. **One accent, used like punctuation.** Color is semantic (`--color-accent`, `--color-success`…).
   No gradient text on headings or metrics — protect scannability.
5. **Motion is one orchestrated page-load moment**, not scattered micro-interactions. Chosen
   easing, chosen duration, honor `prefers-reduced-motion`.
6. **Atmosphere is layered and subtle** — grain over gradients, masked edges on patterns, blurred
   low-opacity glow. Never a rave.

### Token contract quick-reference (consumed by every block below)

```css
/* These are DEFINED in the color/space/type/motion modules. Shown here so the
   patterns are self-contained for an agent. Values are the dark-first defaults. */
:root {
  /* surface ladder — tinted, never #000 */
  --color-bg: oklch(0.16 0.006 265);
  --color-bg-subtle: oklch(0.19 0.006 265);
  --color-surface: oklch(0.21 0.007 265);
  --color-surface-2: oklch(0.25 0.008 265);
  /* text hierarchy */
  --color-fg: oklch(0.97 0.004 265);
  --color-fg-muted: oklch(0.78 0.006 265);
  --color-fg-subtle: oklch(0.62 0.007 265);
  /* hairlines do the work of shadows */
  --color-border: oklch(1 0 0 / 0.10);
  --color-border-subtle: oklch(1 0 0 / 0.06);
  /* ONE accent + states */
  --color-accent: oklch(0.62 0.19 256);
  --color-accent-fg: oklch(0.99 0 0);
  --color-accent-hover: oklch(0.67 0.19 256);
  --color-ring: oklch(0.62 0.19 256 / 0.55);
  --color-success: oklch(0.70 0.17 152);
  --color-warning: oklch(0.80 0.16 85);
  --color-danger: oklch(0.64 0.21 25);
  /* space — 8pt */
  --space-1:0.25rem; --space-2:0.5rem; --space-3:0.75rem; --space-4:1rem;
  --space-6:1.5rem; --space-8:2rem; --space-12:3rem; --space-16:4rem;
  --space-20:5rem; --space-24:6rem; --space-32:8rem;
  /* radius — intentional, not uniform */
  --radius-sm:0.375rem; --radius-md:0.625rem; --radius-lg:0.875rem;
  --radius-xl:1.25rem; --radius-2xl:1.75rem; --radius-full:9999px;
  /* type — fluid clamp + tight display tracking */
  --text-sm:clamp(0.82rem,0.8rem+0.1vw,0.875rem);
  --text-base:clamp(0.95rem,0.92rem+0.15vw,1rem);
  --text-lg:clamp(1.05rem,1rem+0.3vw,1.2rem);
  --text-3xl:clamp(1.7rem,1.3rem+1.9vw,2.4rem);
  --text-5xl:clamp(2.6rem,1.8rem+3.6vw,4rem);
  --text-7xl:clamp(3.4rem,2.1rem+5.6vw,6rem);
  --leading-tight:1.08; --leading-snug:1.18; --leading-body:1.55;
  --tracking-tight:-0.02em; --tracking-display:-0.04em;
  --font-display:"Geist","Inter",system-ui,sans-serif;
  --font-body:"Geist","Inter",system-ui,sans-serif;
  --font-mono:"Geist Mono","SF Mono",ui-monospace,monospace;
  /* elevation — low-opacity, layered */
  --shadow-sm:0 1px 2px oklch(0 0 0 / 0.20);
  --shadow-md:0 4px 12px -2px oklch(0 0 0 / 0.28);
  --shadow-lg:0 12px 32px -8px oklch(0 0 0 / 0.36);
  --shadow-xl:0 24px 64px -16px oklch(0 0 0 / 0.46);
  /* motion */
  --ease-out:cubic-bezier(0.23,1,0.32,1);
  --ease-in-out:cubic-bezier(0.65,0,0.35,1);
  --ease-spring:linear(0,0.18,0.55,0.86,1.04,1.08,1.03,1,0.99,1);
  --dur-fast:150ms; --dur-base:250ms; --dur-slow:400ms;
}
```

```css
/* Global, required for every page below */
*,*::before,*::after { box-sizing: border-box; }
@media (prefers-reduced-motion: reduce) {
  *,*::before,*::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
.container { width: min(100% - var(--space-8), 1200px); margin-inline: auto; }
```

---

## 1. Navbar — sticky, hairline, blur

A floating chrome layer that *recedes*: tinted glass + a single bottom hairline. No heavy shadow,
no full opaque bar. It gets a `data-scrolled` state once the page moves so the blur only appears
when content is behind it (Apple HIG: controls elevate the content beneath them).

```html
<header class="nav" data-scrolled="false">
  <div class="container nav__row">
    <a class="nav__brand" href="/">
      <span class="nav__logo" aria-hidden="true"></span>
      Orbit
    </a>
    <nav class="nav__links" aria-label="Primary">
      <a href="/product">Product</a>
      <a href="/docs">Docs</a>
      <a href="/pricing">Pricing</a>
      <a href="/changelog">Changelog</a>
    </nav>
    <div class="nav__actions">
      <a class="btn btn--ghost" href="/login">Log in</a>
      <a class="btn btn--primary" href="/start">Start free</a>
    </div>
  </div>
</header>
```

```css
.nav {
  position: sticky; top: 0; z-index: 50;
  border-bottom: 1px solid transparent;
  transition: border-color var(--dur-base) var(--ease-out),
              background-color var(--dur-base) var(--ease-out);
}
.nav[data-scrolled="true"] {
  background: color-mix(in oklch, var(--color-bg) 72%, transparent);
  -webkit-backdrop-filter: blur(14px) saturate(1.4);
          backdrop-filter: blur(14px) saturate(1.4);
  border-bottom-color: var(--color-border-subtle);
}
.nav__row {
  display: flex; align-items: center; gap: var(--space-8);
  height: 4rem;
}
.nav__brand {
  display: inline-flex; align-items: center; gap: var(--space-2);
  font-family: var(--font-display); font-weight: 600;
  letter-spacing: var(--tracking-tight); color: var(--color-fg);
  text-decoration: none;
}
.nav__logo {
  width: 0.85rem; height: 0.85rem; border-radius: var(--radius-sm);
  background: var(--color-accent);
}
.nav__links {
  display: flex; gap: var(--space-6); margin-right: auto;
  font-size: var(--text-sm);
}
.nav__links a {
  color: var(--color-fg-muted); text-decoration: none;
  transition: color var(--dur-fast) var(--ease-out);
}
.nav__links a:hover { color: var(--color-fg); }
.nav__actions { display: flex; align-items: center; gap: var(--space-3); }
@media (max-width: 768px) { .nav__links { display: none; } }

/* button primitives reused across patterns */
.btn {
  --pad-y: 0.5rem; --pad-x: 0.9rem;
  display: inline-flex; align-items: center; justify-content: center;
  gap: var(--space-2); padding: var(--pad-y) var(--pad-x);
  font: 500 var(--text-sm)/1 var(--font-body);
  border-radius: var(--radius-md); border: 1px solid transparent;
  cursor: pointer; text-decoration: none; white-space: nowrap;
  transition: transform var(--dur-fast) var(--ease-out),
              background-color var(--dur-fast) var(--ease-out),
              border-color var(--dur-fast) var(--ease-out);
}
.btn:active { transform: scale(0.97); }
.btn:focus-visible { outline: 2px solid var(--color-ring); outline-offset: 2px; }
.btn--primary { background: var(--color-accent); color: var(--color-accent-fg); }
.btn--primary:hover { background: var(--color-accent-hover); }
.btn--ghost { color: var(--color-fg-muted); }
.btn--ghost:hover { color: var(--color-fg); background: var(--color-surface); }
.btn--outline { color: var(--color-fg); border-color: var(--color-border); }
.btn--outline:hover { border-color: var(--color-fg-subtle); background: var(--color-surface); }
```

```tsx
// React 19: scroll-state hook drives the hairline. No layout thrash — passive listener.
"use client";
import { useEffect, useState } from "react";

export function useScrolled(threshold = 8) {
  const [scrolled, setScrolled] = useState(false);
  useEffect(() => {
    const onScroll = () => setScrolled(window.scrollY > threshold);
    onScroll();
    window.addEventListener("scroll", onScroll, { passive: true });
    return () => window.removeEventListener("scroll", onScroll);
  }, [threshold]);
  return scrolled;
}

export function Navbar() {
  const scrolled = useScrolled();
  return (
    <header
      data-scrolled={scrolled ? "true" : "false"}
      className="sticky top-0 z-50 border-b border-transparent transition-colors duration-[var(--dur-base)]
                 data-[scrolled=true]:border-border-subtle
                 data-[scrolled=true]:bg-bg/70 data-[scrolled=true]:backdrop-blur-xl"
    >
      {/* ...nav__row content... */}
    </header>
  );
}
```

---

## 2. Hero — editorial / asymmetric (NOT centered-eyebrow + 2 CTAs)

The default centered hero is the single most over-trained pattern. Ours is **left-anchored and
asymmetric**: an oversized headline on a content lane to the left, a live product panel offset to
the right, one dominant CTA + one quiet text link (not two equal buttons), and a thin status row.
The grid is `1.1fr 0.9fr` — deliberately *not* 50/50 — so the eye has a clear entry point
(upper-leading, per Apple's HIG) instead of bouncing between two balanced halves.

```html
<section class="hero">
  <div class="hero__atmos" aria-hidden="true"></div>
  <div class="container hero__grid">
    <div class="hero__lede" data-stagger>
      <p class="hero__kicker" data-stagger-item>
        <span class="dot"></span> Shipping v4 — now with realtime
      </p>
      <h1 class="hero__title" data-stagger-item>
        Ship the back end<br>your users <em>never notice.</em>
      </h1>
      <p class="hero__sub" data-stagger-item>
        Orbit is the data layer for teams who measure latency in feelings.
        Edge-replicated, typed end-to-end, zero ops.
      </p>
      <div class="hero__cta" data-stagger-item>
        <a class="btn btn--primary btn--lg" href="/start">Start building</a>
        <a class="hero__link" href="/docs">Read the docs →</a>
      </div>
      <ul class="hero__proof" data-stagger-item>
        <li><strong>14ms</strong> p99 reads</li>
        <li><strong>99.99%</strong> uptime</li>
        <li><strong>0</strong> cold starts</li>
      </ul>
    </div>

    <div class="hero__panel" data-stagger-item>
      <div class="panel__bar">
        <span class="panel__dotrow"><i></i><i></i><i></i></span>
        <code>orbit deploy --prod</code>
      </div>
      <pre class="panel__body"><code><span class="c-key">const</span> db = <span class="c-fn">orbit</span>({
  region: <span class="c-str">"auto"</span>,
  schema,
});
<span class="c-cmt">// typed, edge-replicated, 14ms p99</span>
<span class="c-key">await</span> db.users.<span class="c-fn">where</span>({ active: <span class="c-key">true</span> });</code></pre>
    </div>
  </div>
</section>
```

```css
.hero { position: relative; overflow: hidden; padding-block: clamp(4rem, 8vw, 8rem); }
.hero__grid {
  display: grid; grid-template-columns: 1.1fr 0.9fr;
  gap: var(--space-16); align-items: center;
}
.hero__kicker {
  display: inline-flex; align-items: center; gap: var(--space-2);
  font: 500 var(--text-sm)/1 var(--font-mono);
  color: var(--color-fg-muted); margin: 0 0 var(--space-6);
  padding: var(--space-2) var(--space-3);
  border: 1px solid var(--color-border-subtle); border-radius: var(--radius-full);
  width: max-content;
}
.hero__kicker .dot {
  width: 7px; height: 7px; border-radius: var(--radius-full);
  background: var(--color-success);
  box-shadow: 0 0 0 3px color-mix(in oklch, var(--color-success) 25%, transparent);
}
.hero__title {
  margin: 0 0 var(--space-6);
  font: 600 var(--text-7xl)/var(--leading-tight) var(--font-display);
  letter-spacing: var(--tracking-display);
  color: var(--color-fg); text-wrap: balance;
}
.hero__title em { font-style: normal; color: var(--color-fg-subtle); } /* tonal accent, not gradient */
.hero__sub {
  margin: 0 0 var(--space-8); max-width: 34ch;
  font: 400 var(--text-lg)/var(--leading-body) var(--font-body);
  color: var(--color-fg-muted);
}
.hero__cta { display: flex; align-items: center; gap: var(--space-6); margin-bottom: var(--space-12); }
.btn--lg { --pad-y: 0.72rem; --pad-x: 1.3rem; font-size: var(--text-base); border-radius: var(--radius-lg); }
.hero__link {
  font: 500 var(--text-sm) var(--font-body); color: var(--color-fg-muted);
  text-decoration: none; transition: color var(--dur-fast) var(--ease-out);
}
.hero__link:hover { color: var(--color-accent); }
.hero__proof {
  display: flex; gap: var(--space-8); list-style: none; margin: 0; padding: 0;
  font-size: var(--text-sm); color: var(--color-fg-subtle);
}
.hero__proof strong { display: block; font-size: var(--text-3xl); font-weight: 600;
  color: var(--color-fg); letter-spacing: var(--tracking-tight); }

/* offset, lit product panel — the asymmetric counterweight */
.hero__panel {
  position: relative; border-radius: var(--radius-xl); overflow: hidden;
  border: 1px solid transparent;
  background:
    linear-gradient(var(--color-surface), var(--color-surface)) padding-box,
    linear-gradient(180deg, oklch(1 0 0 / 0.14), oklch(1 0 0 / 0.02)) border-box;
  box-shadow: var(--shadow-xl);
  transform: translateY(-0.5rem) rotate(0.6deg);
}
.panel__bar {
  display: flex; align-items: center; gap: var(--space-3);
  padding: var(--space-3) var(--space-4);
  border-bottom: 1px solid var(--color-border-subtle);
  font: var(--text-sm) var(--font-mono); color: var(--color-fg-subtle);
}
.panel__dotrow { display: inline-flex; gap: 6px; }
.panel__dotrow i { width: 10px; height: 10px; border-radius: var(--radius-full);
  background: var(--color-border); }
.panel__body { margin: 0; padding: var(--space-6);
  font: var(--text-sm)/1.7 var(--font-mono); color: var(--color-fg-muted); overflow-x: auto; }
.c-key { color: var(--color-accent); } .c-fn { color: var(--color-success); }
.c-str { color: var(--color-warning); } .c-cmt { color: var(--color-fg-subtle); font-style: italic; }

@media (max-width: 900px) {
  .hero__grid { grid-template-columns: 1fr; gap: var(--space-12); }
  .hero__panel { transform: none; }
}
```

```tsx
// React 19 + Motion: ONE orchestrated page-load stagger (the page's single big motion moment).
// Parent owns the timeline; children inherit it. Honors reduced-motion via useReducedMotion.
"use client";
import { motion, useReducedMotion } from "motion/react";

const container = {
  hidden: {},
  show: { transition: { staggerChildren: 0.08, delayChildren: 0.05 } },
};
const item = {
  hidden: { opacity: 0, y: 16, filter: "blur(4px)" },
  show: {
    opacity: 1, y: 0, filter: "blur(0px)",
    transition: { type: "spring", stiffness: 200, damping: 26, bounce: 0.05 },
  },
};

export function HeroLede() {
  const reduce = useReducedMotion();
  const v = reduce ? undefined : item; // skip transform; fall back to instant
  return (
    <motion.div
      variants={reduce ? undefined : container}
      initial={reduce ? undefined : "hidden"}
      animate={reduce ? undefined : "show"}
    >
      <motion.p variants={v} className="hero__kicker">…</motion.p>
      <motion.h1 variants={v} className="hero__title">…</motion.h1>
      <motion.p variants={v} className="hero__sub">…</motion.p>
      <motion.div variants={v} className="hero__cta">…</motion.div>
    </motion.div>
  );
}
```

> **Anti-slop notes.** Headline emphasis is *tonal* (`em` → `--color-fg-subtle`), never gradient
> text. One primary button + one text link, not two filled CTAs. The panel is rotated 0.6deg and
> pulled up — small asymmetry that signals "designed by a human." The kicker carries a real status
> dot, not a decorative pill.

---

## 3. Bento feature grid — size maps to importance

The bento (Apple-2023 lineage, now the B2B default) replaces three identical icon cards. **Cells
are deliberately unequal**: a 2×2 hero outcome cell anchors the upper-left F-pattern entry, with
wide/tall/standard cells around it. Mix *outcome* cards ("Cut p99 by 60%") with *feature* cards.
Keep it to ≤ 9 cells; all-equal cells are just a rounded grid (slop).

```html
<section class="section">
  <div class="container">
    <header class="section__head">
      <p class="eyebrow">Platform</p>
      <h2 class="section__title">Everything is one primitive away.</h2>
    </header>
    <div class="bento">
      <article class="bento__cell bento__cell--hero">
        <h3>Edge-replicated reads</h3>
        <p>Your data lives where your users are. No region config, no cold reads.</p>
        <div class="bento__metric"><strong>60%</strong> lower p99</div>
        <div class="bento__viz" aria-hidden="true"></div>
      </article>
      <article class="bento__cell bento__cell--wide">
        <h3>Typed end-to-end</h3>
        <p>Schema generates client types. Break a query, break the build — not prod.</p>
      </article>
      <article class="bento__cell">
        <h3>Zero ops</h3>
        <p>No pools, no shards to babysit.</p>
      </article>
      <article class="bento__cell">
        <h3>Realtime</h3>
        <p>Subscribe to any query.</p>
      </article>
      <article class="bento__cell bento__cell--tall">
        <h3>Audit everything</h3>
        <p>Every mutation is logged, diffable, and replayable to any point in time.</p>
        <div class="bento__viz" aria-hidden="true"></div>
      </article>
    </div>
  </div>
</section>
```

```css
.section { padding-block: var(--space-24); }
.section__head { max-width: 48ch; margin-bottom: var(--space-12); }
.eyebrow {
  margin: 0 0 var(--space-3); font: 500 var(--text-sm) var(--font-mono);
  letter-spacing: 0.04em; text-transform: uppercase; color: var(--color-accent);
}
.section__title {
  margin: 0; font: 600 var(--text-5xl)/var(--leading-snug) var(--font-display);
  letter-spacing: var(--tracking-display); color: var(--color-fg); text-wrap: balance;
}
.bento {
  display: grid; gap: var(--space-4);
  grid-template-columns: repeat(4, 1fr);
  grid-auto-rows: 200px;
}
.bento__cell {
  position: relative; overflow: hidden;
  display: flex; flex-direction: column; gap: var(--space-2);
  padding: var(--space-6); border-radius: var(--radius-xl);
  background: var(--color-surface); border: 1px solid var(--color-border-subtle);
  transition: border-color var(--dur-base) var(--ease-out),
              background-color var(--dur-base) var(--ease-out);
}
.bento__cell:hover { border-color: var(--color-border); background: var(--color-surface-2); }
.bento__cell--hero { grid-column: span 2; grid-row: span 2; }
.bento__cell--wide { grid-column: span 2; }
.bento__cell--tall { grid-row: span 2; }
.bento__cell h3 {
  margin: 0; font: 600 var(--text-lg) var(--font-display);
  letter-spacing: var(--tracking-tight); color: var(--color-fg);
}
.bento__cell p { margin: 0; font: var(--text-base)/var(--leading-body) var(--font-body);
  color: var(--color-fg-muted); max-width: 40ch; }
.bento__metric { margin-top: auto; font: var(--text-sm) var(--font-mono); color: var(--color-fg-subtle); }
.bento__metric strong { font-size: var(--text-3xl); color: var(--color-fg); }
/* in-cell atmosphere: a masked accent glow, bottom-right of large cells only */
.bento__viz {
  position: absolute; right: -20%; bottom: -30%; width: 70%; aspect-ratio: 1;
  background: radial-gradient(circle, color-mix(in oklch, var(--color-accent) 40%, transparent), transparent 70%);
  filter: blur(40px); opacity: 0.5; pointer-events: none;
}
@media (max-width: 900px) {
  .bento { grid-template-columns: repeat(2, 1fr); }
  .bento__cell--hero { grid-column: span 2; grid-row: span 1; }
}
@media (max-width: 560px) {
  .bento { grid-template-columns: 1fr; grid-auto-rows: auto; }
  .bento__cell { min-height: 160px; }
}
```

```tsx
// React 19: data-driven bento with a typed cell span. Scroll-reveal via native CSS (see §9),
// so no JS observer needed. Spans are explicit, not auto — importance is a design decision.
type Span = "hero" | "wide" | "tall" | "std";
type Cell = { title: string; body: string; metric?: string; span: Span };

const SPAN: Record<Span, string> = {
  hero: "md:col-span-2 md:row-span-2",
  wide: "md:col-span-2",
  tall: "md:row-span-2",
  std: "",
};

export function Bento({ cells }: { cells: Cell[] }) {
  return (
    <div className="grid grid-cols-1 gap-4 md:grid-cols-4 md:auto-rows-[200px]">
      {cells.map((c) => (
        <article
          key={c.title}
          className={`group relative flex flex-col gap-2 overflow-hidden rounded-xl border
            border-border-subtle bg-surface p-6 transition-colors hover:border-border
            hover:bg-surface-2 ${SPAN[c.span]}`}
        >
          <h3 className="text-lg font-semibold tracking-tight text-fg">{c.title}</h3>
          <p className="max-w-[40ch] leading-relaxed text-fg-muted">{c.body}</p>
          {c.metric && (
            <span className="mt-auto font-mono text-sm text-fg-subtle">{c.metric}</span>
          )}
        </article>
      ))}
    </div>
  );
}
```

---

## 4. Feature section — real hierarchy, alternating sides

For deep-dive features, alternate a text lane against a large media panel. Hierarchy is built with
**contrast**, not chrome: a mono eyebrow, an oversized headline, generous space, and a single
inline link. The media side carries the visual weight. Alternate `data-flip` every other row so
the page has rhythm instead of a monotone left rail.

```html
<section class="feature" data-flip="false">
  <div class="container feature__grid">
    <div class="feature__copy">
      <p class="eyebrow">Observability</p>
      <h2 class="feature__title">See the query before it hits prod.</h2>
      <p class="feature__body">
        Every query is typed, traced, and costed at build time. The slow ones surface in
        review — not in your 2am pager.
      </p>
      <ul class="feature__list">
        <li>Build-time query cost estimates</li>
        <li>Per-route latency budgets</li>
        <li>Replayable audit log</li>
      </ul>
      <a class="feature__link" href="/docs/observability">How tracing works →</a>
    </div>
    <div class="feature__media">
      <div class="feature__media-inner" aria-hidden="true"></div>
    </div>
  </div>
</section>
```

```css
.feature { padding-block: var(--space-20); }
.feature__grid {
  display: grid; grid-template-columns: 0.85fr 1.15fr;
  gap: var(--space-16); align-items: center;
}
.feature[data-flip="true"] .feature__copy  { order: 2; }
.feature[data-flip="true"] .feature__media { order: 1; }
.feature__title {
  margin: var(--space-3) 0 var(--space-4);
  font: 600 var(--text-3xl)/var(--leading-snug) var(--font-display);
  letter-spacing: var(--tracking-display); color: var(--color-fg); text-wrap: balance;
}
.feature__body { margin: 0 0 var(--space-6); max-width: 42ch;
  font: var(--text-lg)/var(--leading-body) var(--font-body); color: var(--color-fg-muted); }
.feature__list { list-style: none; margin: 0 0 var(--space-6); padding: 0;
  display: grid; gap: var(--space-3); }
.feature__list li { position: relative; padding-left: var(--space-6);
  font: var(--text-base) var(--font-body); color: var(--color-fg); }
.feature__list li::before {
  content: ""; position: absolute; left: 0; top: 0.55em;
  width: 6px; height: 6px; border-radius: var(--radius-full); background: var(--color-accent);
}
.feature__link { font: 500 var(--text-base) var(--font-body); color: var(--color-accent);
  text-decoration: none; }
.feature__link:hover { text-decoration: underline; text-underline-offset: 3px; }
.feature__media {
  aspect-ratio: 4 / 3; border-radius: var(--radius-2xl); overflow: hidden;
  border: 1px solid var(--color-border-subtle); background: var(--color-bg-subtle);
  position: relative;
}
.feature__media-inner {
  position: absolute; inset: 0;
  background:
    radial-gradient(60% 50% at 70% 20%, color-mix(in oklch, var(--color-accent) 22%, transparent), transparent 70%),
    linear-gradient(to right, var(--color-border-subtle) 1px, transparent 1px) 0 0 / 28px 28px,
    linear-gradient(to bottom, var(--color-border-subtle) 1px, transparent 1px) 0 0 / 28px 28px;
}
@media (max-width: 900px) {
  .feature__grid { grid-template-columns: 1fr; gap: var(--space-8); }
  .feature[data-flip="true"] .feature__copy,
  .feature[data-flip="true"] .feature__media { order: 0; }
}
```

---

## 5. Pricing — anchored tiers, one highlighted plan

Three tiers, but **not three identical cards**. The recommended plan is lifted: accent border,
a "Most popular" tab, slightly larger. Price uses the display font + tabular numerals so digits
don't jitter. No gradient on the card — the highlight is a 1px accent border + faint glow.

```html
<section class="section">
  <div class="container">
    <header class="section__head section__head--center">
      <p class="eyebrow">Pricing</p>
      <h2 class="section__title">Pay for throughput, not seats.</h2>
    </header>
    <div class="pricing">
      <article class="plan">
        <h3 class="plan__name">Hobby</h3>
        <p class="plan__price"><span>$0</span><small>/mo</small></p>
        <p class="plan__note">For side projects and prototypes.</p>
        <a class="btn btn--outline plan__cta" href="/start">Start free</a>
        <ul class="plan__feats">
          <li>1 project</li><li>100k reads/day</li><li>Community support</li>
        </ul>
      </article>

      <article class="plan plan--featured">
        <span class="plan__badge">Most popular</span>
        <h3 class="plan__name">Pro</h3>
        <p class="plan__price"><span>$29</span><small>/mo</small></p>
        <p class="plan__note">For teams shipping to production.</p>
        <a class="btn btn--primary plan__cta" href="/start">Start 14-day trial</a>
        <ul class="plan__feats">
          <li>Unlimited projects</li><li>10M reads/day</li><li>Edge replication</li>
          <li>Priority support</li>
        </ul>
      </article>

      <article class="plan">
        <h3 class="plan__name">Scale</h3>
        <p class="plan__price"><span>Custom</span></p>
        <p class="plan__note">SLA, SSO, and dedicated infra.</p>
        <a class="btn btn--outline plan__cta" href="/contact">Talk to us</a>
        <ul class="plan__feats">
          <li>Everything in Pro</li><li>99.99% SLA</li><li>SSO + audit export</li>
        </ul>
      </article>
    </div>
  </div>
</section>
```

```css
.section__head--center { margin-inline: auto; text-align: center; }
.pricing {
  display: grid; gap: var(--space-6); align-items: start;
  grid-template-columns: repeat(3, 1fr);
}
.plan {
  position: relative; display: flex; flex-direction: column; gap: var(--space-4);
  padding: var(--space-8); border-radius: var(--radius-xl);
  background: var(--color-surface); border: 1px solid var(--color-border-subtle);
}
/* the one lifted plan — accent border + faint directional glow, NOT a gradient fill */
.plan--featured {
  border-color: color-mix(in oklch, var(--color-accent) 55%, transparent);
  background: var(--color-surface-2);
  box-shadow: 0 0 0 1px color-mix(in oklch, var(--color-accent) 25%, transparent),
              0 24px 64px -24px color-mix(in oklch, var(--color-accent) 45%, transparent);
  transform: translateY(-0.75rem);
}
.plan__badge {
  position: absolute; top: calc(-1 * var(--space-3)); left: var(--space-8);
  padding: var(--space-1) var(--space-3); border-radius: var(--radius-full);
  font: 500 var(--text-sm) var(--font-mono); color: var(--color-accent-fg);
  background: var(--color-accent);
}
.plan__name { margin: 0; font: 600 var(--text-lg) var(--font-display); color: var(--color-fg); }
.plan__price { margin: 0; display: flex; align-items: baseline; gap: var(--space-2);
  font-variant-numeric: tabular-nums; }
.plan__price span { font: 600 var(--text-5xl) var(--font-display);
  letter-spacing: var(--tracking-display); color: var(--color-fg); }
.plan__price small { font: var(--text-base) var(--font-body); color: var(--color-fg-subtle); }
.plan__note { margin: 0; font: var(--text-base) var(--font-body); color: var(--color-fg-muted); }
.plan__cta { width: 100%; }
.plan__feats { list-style: none; margin: var(--space-2) 0 0; padding: 0;
  display: grid; gap: var(--space-3); border-top: 1px solid var(--color-border-subtle);
  padding-top: var(--space-6); }
.plan__feats li { position: relative; padding-left: var(--space-6);
  font: var(--text-base) var(--font-body); color: var(--color-fg-muted); }
.plan__feats li::before {
  content: "✓"; position: absolute; left: 0; color: var(--color-success); font-weight: 600;
}
@media (max-width: 860px) {
  .pricing { grid-template-columns: 1fr; max-width: 28rem; margin-inline: auto; }
  .plan--featured { transform: none; }
}
```

---

## 6. CTA band — the one loud moment

The page earns *one* high-contrast band. This is where atmosphere concentrates: aurora glow +
grain over a deep surface, a single oversized headline, one primary button. Everything else on the
page was restrained so this lands.

```html
<section class="cta-band">
  <div class="cta-band__atmos" aria-hidden="true"></div>
  <div class="container cta-band__inner">
    <h2 class="cta-band__title">Your users will never thank you for this.</h2>
    <p class="cta-band__sub">That's the point. Ship the data layer they never notice.</p>
    <div class="cta-band__actions">
      <a class="btn btn--primary btn--lg" href="/start">Start building free</a>
      <a class="btn btn--outline btn--lg" href="/demo">Book a demo</a>
    </div>
  </div>
</section>
```

```css
.cta-band { position: relative; overflow: hidden; margin-block: var(--space-24);
  border-block: 1px solid var(--color-border-subtle); }
.cta-band__inner { position: relative; z-index: 1; text-align: center;
  padding-block: clamp(4rem, 8vw, 7rem); }
.cta-band__title { margin: 0 auto var(--space-4); max-width: 18ch;
  font: 600 var(--text-5xl)/var(--leading-snug) var(--font-display);
  letter-spacing: var(--tracking-display); color: var(--color-fg); text-wrap: balance; }
.cta-band__sub { margin: 0 auto var(--space-8); max-width: 42ch;
  font: var(--text-lg) var(--font-body); color: var(--color-fg-muted); }
.cta-band__actions { display: flex; gap: var(--space-4); justify-content: center; flex-wrap: wrap; }
/* atmosphere stack: deep base + aurora glow (see §8.4) + grain (see §8.2) */
.cta-band__atmos { position: absolute; inset: 0; z-index: 0; background: var(--color-bg); }
.cta-band__atmos::before {
  content: ""; position: absolute; inset: -30%;
  background:
    radial-gradient(40% 40% at 30% 30%, color-mix(in oklch, var(--color-accent) 45%, transparent), transparent 70%),
    radial-gradient(45% 45% at 75% 60%, color-mix(in oklch, var(--color-accent) 28%, transparent), transparent 70%);
  filter: blur(70px); opacity: 0.7;
}
```

---

## 7. Footer — structured, quiet, honest

Multi-column link map, a brand lane with a status pill, a thin legal row. No reverse-contrast
panel-in-panel; it sits on the page bg with a single top hairline. Structure is *exposed*, Nothing
style: clear lanes, honest labels.

```html
<footer class="footer">
  <div class="container footer__grid">
    <div class="footer__brand">
      <a class="nav__brand" href="/"><span class="nav__logo"></span> Orbit</a>
      <p class="footer__tag">The data layer for teams who measure latency in feelings.</p>
      <p class="footer__status"><span class="dot"></span> All systems operational</p>
    </div>
    <nav class="footer__col" aria-label="Product">
      <h4>Product</h4><a href="/features">Features</a><a href="/pricing">Pricing</a>
      <a href="/changelog">Changelog</a><a href="/status">Status</a>
    </nav>
    <nav class="footer__col" aria-label="Developers">
      <h4>Developers</h4><a href="/docs">Docs</a><a href="/api">API</a>
      <a href="/sdk">SDKs</a><a href="/examples">Examples</a>
    </nav>
    <nav class="footer__col" aria-label="Company">
      <h4>Company</h4><a href="/about">About</a><a href="/blog">Blog</a>
      <a href="/careers">Careers</a><a href="/contact">Contact</a>
    </nav>
  </div>
  <div class="container footer__legal">
    <span>© 2026 Orbit, Inc.</span>
    <div class="footer__legal-links">
      <a href="/privacy">Privacy</a><a href="/terms">Terms</a>
    </div>
  </div>
</footer>
```

```css
.footer { border-top: 1px solid var(--color-border-subtle); padding-top: var(--space-20); }
.footer__grid { display: grid; gap: var(--space-12);
  grid-template-columns: 2fr 1fr 1fr 1fr; }
.footer__tag { margin: var(--space-4) 0; max-width: 32ch;
  font: var(--text-base)/var(--leading-body) var(--font-body); color: var(--color-fg-muted); }
.footer__status { display: inline-flex; align-items: center; gap: var(--space-2);
  font: var(--text-sm) var(--font-mono); color: var(--color-fg-subtle); margin: 0; }
.footer__status .dot { width: 7px; height: 7px; border-radius: var(--radius-full);
  background: var(--color-success); }
.footer__col { display: flex; flex-direction: column; gap: var(--space-3); }
.footer__col h4 { margin: 0 0 var(--space-2); font: 600 var(--text-sm) var(--font-display);
  letter-spacing: 0.02em; color: var(--color-fg); }
.footer__col a { font: var(--text-sm) var(--font-body); color: var(--color-fg-muted);
  text-decoration: none; transition: color var(--dur-fast) var(--ease-out); }
.footer__col a:hover { color: var(--color-fg); }
.footer__legal { display: flex; justify-content: space-between; align-items: center;
  margin-top: var(--space-16); padding-block: var(--space-6);
  border-top: 1px solid var(--color-border-subtle);
  font: var(--text-sm) var(--font-mono); color: var(--color-fg-subtle); }
.footer__legal-links { display: flex; gap: var(--space-6); }
.footer__legal-links a { color: var(--color-fg-subtle); text-decoration: none; }
.footer__legal-links a:hover { color: var(--color-fg); }
@media (max-width: 760px) {
  .footer__grid { grid-template-columns: 1fr 1fr; }
  .footer__brand { grid-column: 1 / -1; }
  .footer__legal { flex-direction: column; gap: var(--space-4); }
}
```

---

## 8. Marketing background atmosphere (copy-paste)

Atmosphere is **layered and subtractive**. The order that reads as crafted (Stripe/Linear): deep
tinted base → one directional glow/mesh → masked structural pattern (dots or grid) → grain on top
to kill banding. Each layer is `position:absolute; inset:0; pointer-events:none` behind content.

### 8.1 The stack (compose these, don't use all at once)

```html
<div class="atmos">
  <div class="atmos__mesh"></div>   <!-- pick ONE color layer: mesh OR aurora -->
  <div class="atmos__dots"></div>   <!-- optional structure -->
  <div class="atmos__grain"></div>  <!-- always last, on top -->
</div>
```
```css
.atmos { position: absolute; inset: 0; z-index: 0; overflow: hidden;
  background: var(--color-bg); pointer-events: none; }
.atmos > * { position: absolute; inset: 0; }
```

### 8.2 Grain / noise (kills banding, adds tactility — opacity 0.04–0.10)

```css
.atmos__grain {
  background-image: url("data:image/svg+xml,\
<svg xmlns='http://www.w3.org/2000/svg'><filter id='n'>\
<feTurbulence type='fractalNoise' baseFrequency='.8' numOctaves='3' stitchTiles='stitch'/>\
</filter><rect width='100%' height='100%' filter='url(%23n)'/></svg>");
  background-size: 180px;          /* keeps grain physically constant on any viewport */
  opacity: 0.07;
  mix-blend-mode: overlay;
}
```

### 8.3 Gradient mesh (cheap layered-radial fallback; use WebGL/Whatamesh for animated)

```css
.atmos__mesh {
  background:
    radial-gradient(40% 50% at 18% 12%, color-mix(in oklch, var(--color-accent) 30%, transparent), transparent 60%),
    radial-gradient(38% 44% at 82% 18%, color-mix(in oklch, var(--color-success) 22%, transparent), transparent 60%),
    radial-gradient(50% 50% at 60% 95%, color-mix(in oklch, var(--color-accent) 24%, transparent), transparent 65%);
  filter: blur(8px);               /* softens the seams between radials */
  opacity: 0.85;
  /* fade the whole thing into the page so it never hard-edges */
  -webkit-mask-image: radial-gradient(120% 90% at 50% 0%, #000 35%, transparent 100%);
          mask-image: radial-gradient(120% 90% at 50% 0%, #000 35%, transparent 100%);
}
```

For the animated Stripe-style version, drive a WebGL canvas (the gradient is a fragment shader, not
CSS — it offloads to the GPU and a ScrollObserver pauses it off-screen):

```html
<canvas id="gradient-canvas" data-transition-in></canvas>
<script type="module">
  import { Gradient } from "https://esm.sh/whatamesh";
  new Gradient().initGradient("#gradient-canvas");
</script>
```
```css
#gradient-canvas {
  width: 100%; height: 100%;
  --gradient-color-1: #1a1f3a; --gradient-color-2: #2b3a8c;
  --gradient-color-3: #3f5bd6; --gradient-color-4: #1e2747;  /* tint to your accent, not rainbow */
}
```

### 8.4 Aurora / glow (blurred low-opacity blobs — directional light, not a rave)

```css
.atmos__aurora { overflow: hidden; }
.atmos__aurora::before {
  content: ""; position: absolute; inset: -20%;
  background:
    radial-gradient(40% 40% at 30% 20%, color-mix(in oklch, var(--color-accent)  35%, transparent), transparent 70%),
    radial-gradient(35% 35% at 75% 30%, color-mix(in oklch, var(--color-success) 25%, transparent), transparent 70%),
    radial-gradient(45% 45% at 60% 80%, color-mix(in oklch, var(--color-accent)  20%, transparent), transparent 70%);
  filter: blur(64px);                                  /* the magic is the blur */
  animation: aurora 18s var(--ease-in-out) infinite alternate;
}
@keyframes aurora {
  from { transform: translate3d(-4%, 0, 0) scale(1); }
  to   { transform: translate3d(4%, 2%, 0) scale(1.08); }
}
@media (prefers-reduced-motion: reduce) { .atmos__aurora::before { animation: none; } }
```

### 8.5 Dot-grid & line-grid (masked so it fades instead of tiling to infinity)

```css
/* Nothing-style dot substrate */
.atmos__dots {
  background-image: radial-gradient(var(--color-border) 1px, transparent 1px);
  background-size: 18px 18px;
  -webkit-mask-image: radial-gradient(ellipse 60% 55% at 50% 0%, #000 40%, transparent 100%);
          mask-image: radial-gradient(ellipse 60% 55% at 50% 0%, #000 40%, transparent 100%);
}
/* line grid (use for "schematic / engineering" feel — pairs well with mono type) */
.atmos__grid {
  background-image:
    linear-gradient(to right,  var(--color-border-subtle) 1px, transparent 1px),
    linear-gradient(to bottom, var(--color-border-subtle) 1px, transparent 1px);
  background-size: 28px 28px;
  -webkit-mask-image: linear-gradient(to bottom, #000, transparent 80%);
          mask-image: linear-gradient(to bottom, #000, transparent 80%);
}
```

### 8.6 Tailwind v4 `@theme` for the atmosphere

```css
@import "tailwindcss";
@theme {
  --animate-aurora: aurora 18s var(--ease-in-out) infinite alternate;
}
/* @keyframes lives at top level (a sibling of @theme), not nested inside it */
@keyframes aurora {
  from { transform: translate3d(-4%, 0, 0) scale(1); }
  to   { transform: translate3d(4%, 2%, 0) scale(1.08); }
}
/* usage: <div class="animate-aurora">. NB Tailwind v4 size util: bg-size-[18px_18px] (v3 was bg-[size:..]) */
```

> **Rules.** One color layer (mesh **or** aurora), never both at full strength. Always mask edges.
> Always top with grain at 0.04–0.10. Tint every layer toward the *one* accent — a literal rainbow
> mesh is the slop tell. Concentrate the loudest atmosphere in the CTA band (§6) and hero only.

---

## 9. The page-load moment + scroll reveal

Pick **one** orchestrated entrance, not scattered micro-interactions. The hero staggers in on load
(§2, Motion). Below the fold, sections reveal once on scroll with **native CSS scroll-driven
animation** — no JS observer, GPU-cheap, and free in browsers that lack support (the keyframe just
doesn't run, content is already visible).

```css
@keyframes reveal {
  from { opacity: 0; transform: translateY(24px); }
  to   { opacity: 1; transform: none; }
}
@supports (animation-timeline: view()) {
  .reveal {
    animation: reveal both var(--ease-out);
    animation-timeline: view();
    animation-range: entry 0% cover 28%;   /* finishes shortly after the element enters */
  }
  .reveal-stagger > * {
    animation: reveal both var(--ease-out);
    animation-timeline: view();
    animation-range: entry 0% cover 30%;
  }
}
@media (prefers-reduced-motion: reduce) {
  .reveal, .reveal-stagger > * { animation: none; opacity: 1; transform: none; }
}
```

```tsx
// Motion equivalent for components that need JS (whileInView fires once).
// Animate ONLY transform + opacity (cheap, GPU-composited); stay under ~500ms.
import { motion } from "motion/react";

export function Reveal({ children }: { children: React.ReactNode }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 24 }}
      whileInView={{ opacity: 1, y: 0 }}
      viewport={{ once: true, margin: "-15% 0px" }}
      transition={{ type: "spring", stiffness: 200, damping: 24, bounce: 0.05 }}
    >
      {children}
    </motion.div>
  );
}
```

**Motion budget:** one page-load stagger + scroll reveals + hover/active on interactive elements.
Springs (`stiffness 200 / damping 20–26 / bounce < 0.1`) for user-driven UI (buttons, tabs);
fixed-duration tweens (`--dur-base var(--ease-out)`) for coordinated reveals. Keep everything
interruptible and under 500ms.

---

## 10. Required states — loading, empty, error

Marketing pages still embed live widgets (status, pricing fetched from an API, a demo console).
Every async surface ships all three states, themed to the tokens — never a raw spinner on white.

```html
<!-- Loading: skeleton with a token-based shimmer, not a spinner -->
<div class="skeleton" aria-hidden="true"></div>

<!-- Empty: honest, with one action -->
<div class="state state--empty" role="status">
  <p class="state__title">Nothing here yet</p>
  <p class="state__body">Connect a data source to see live metrics.</p>
  <a class="btn btn--outline" href="/connect">Connect a source</a>
</div>

<!-- Error: danger token, recoverable, never a dead end -->
<div class="state state--error" role="alert">
  <p class="state__title">Couldn't load metrics</p>
  <p class="state__body">We hit a snag reaching the edge. Your data is safe.</p>
  <button class="btn btn--outline" type="button">Try again</button>
</div>
```

```css
.skeleton {
  height: 1rem; border-radius: var(--radius-sm);
  background: linear-gradient(90deg,
    var(--color-surface) 25%, var(--color-surface-2) 37%, var(--color-surface) 63%);
  background-size: 400% 100%;
  animation: shimmer 1.4s var(--ease-in-out) infinite;
}
@keyframes shimmer { from { background-position: 100% 0; } to { background-position: -100% 0; } }
@media (prefers-reduced-motion: reduce) { .skeleton { animation: none; } }

.state {
  display: grid; gap: var(--space-2); justify-items: start;
  padding: var(--space-8); border-radius: var(--radius-xl);
  border: 1px solid var(--color-border-subtle); background: var(--color-surface);
}
.state__title { margin: 0; font: 600 var(--text-lg) var(--font-display); color: var(--color-fg); }
.state__body  { margin: 0 0 var(--space-3); font: var(--text-base) var(--font-body);
  color: var(--color-fg-muted); max-width: 40ch; }
.state--error { border-color: color-mix(in oklch, var(--color-danger) 40%, transparent); }
.state--error .state__title { color: var(--color-danger); }
```

---

## 11. Accessibility checklist (every pattern)

- **Landmarks:** `header` / `nav[aria-label]` / `main` / `footer`. One `h1` (the hero), logical
  `h2…` order; section heads use real headings, eyebrows are `p`, not headings.
- **Focus:** every interactive element shows `:focus-visible` ring (`--color-ring`, offset 2px).
  Never remove outlines without a replacement.
- **Contrast:** `--color-fg` on `--color-bg` ≥ 7:1; `--color-fg-muted` ≥ 4.5:1; check accent text
  hits 4.5:1. Run it — OKLCH lightness is a guide, not a guarantee.
- **Targets:** interactive controls ≥ 44×44px (Apple HIG / WCAG AAA). Pad small links.
- **Motion:** every animation has a `prefers-reduced-motion` path; reveals fall back to visible,
  not hidden. Decorative atmosphere is `aria-hidden` + `pointer-events:none`.
- **Color is never the only signal:** the status dot has a text label; error state has a heading,
  not just red.
- **Reduced transparency:** for the navbar/glass, provide an opaque fallback via
  `@media (prefers-reduced-transparency: reduce) { .nav[data-scrolled="true"]{ background: var(--color-bg); backdrop-filter:none; } }`.

---

## 12. Anti-slop verdict table

| Slop default | This system's verdict |
|---|---|
| Centered eyebrow + 64px headline + 2 equal CTAs | Left-anchored asymmetric hero, 1 primary + 1 text link |
| Three identical icon-on-top cards | Bento where **size = importance**; mix outcome + feature cells |
| Gradient text on headline/metrics | Solid `--color-fg`; emphasis is tonal (`--color-fg-subtle`) |
| Blue→purple gradient on white | One tinted accent atmosphere on a deep tinted base |
| Drop shadow on every card | Hairline borders + surface ladder; shadow only on the floating panel/featured plan |
| Uniform 16px radius everywhere | Intentional scale: `sm` chips → `xl/2xl` panels/media |
| Rainbow mesh background | Single-accent-tinted mesh **or** aurora, masked, grain on top |
| Scattered micro-interactions | One orchestrated page-load stagger + once-only scroll reveals |
| Three equal pricing cards | One lifted "most popular" plan (accent border + faint glow) |
| Raw shadcn/spinner states | Themed skeleton + empty + error, all on tokens |

---

## Sources

- Geist (Vercel) — Introduction / Colors / Typography — https://vercel.com/geist/introduction · https://vercel.com/geist/colors · https://vercel.com/geist/typography
- How we redesigned the Linear UI (Part II) — https://linear.app/now/how-we-redesigned-the-linear-ui
- A calmer interface for a product in motion (Linear refresh) — https://linear.app/now/behind-the-latest-design-refresh
- Raycast DESIGN.md (VoltAgent/awesome-design-md) — https://github.com/VoltAgent/awesome-design-md/blob/main/design-md/raycast/DESIGN.md
- Connect: behind the front-end experience (Stripe) — https://stripe.com/blog/connect-front-end-experience
- How To create the Stripe Website Gradient Effect (Bram.us) — https://www.bram.us/2021/10/13/how-to-create-the-stripe-website-gradient-effect/
- How to make animated gradients like Stripe (DEV / jordienr) — https://dev.to/jordienr/how-to-make-animated-gradients-like-stripe-56nh
- Grainy Gradients (CSS-Tricks) — https://css-tricks.com/grainy-gradients/
- Aurora UI — how to create with CSS (DEV / albertwalicki) — https://dev.to/albertwalicki/aurora-ui-how-to-create-with-css-4b6g
- Aurora Background (Aceternity UI) — https://ui.aceternity.com/components/aurora-background
- Crafting grid and dot backgrounds with CSS/Tailwind (ibelick) — https://ibelick.com/blog/create-grid-and-dot-backgrounds-with-css-tailwind-css
- Understanding the Bento Layout Trend (SaaSFrame) — https://www.saasframe.io/blog/the-bento-layout-trend
- Bento Grids Quietly Winning B2B SaaS Homepages 2026 (Pravin Kumar) — https://www.pravinkumar.co/blog/bento-grids-b2b-saas-homepage-design-trend-2026
- Apple Human Interface Guidelines — Layout / Motion / Materials — https://developer.apple.com/design/human-interface-guidelines/
- Liquid Glass | Apple Developer — https://developer.apple.com/documentation/TechnologyOverviews/liquid-glass
- Animate with springs — WWDC23 session 10158 — https://developer.apple.com/videos/play/wwdc2023/10158/
- React transitions (Motion docs) — https://motion.dev/docs/react-transitions
- Slop (Impeccable) — https://impeccable.style/slop/
- AI Slop Web Design: Complete Guide (925studios) — https://www.925studios.co/blog/ai-slop-web-design-guide
- ndot57: the Nothing Typeface (Nothing Community) — https://nothing.community/d/104-ndot57-the-nothing-typeface
- Nothing Design: Transparency & Brand Philosophy (Mugen Brands) — https://mugen-brands.com/en/nothing-design-transparency-brand-philosophy/
