# 00 · Philosophy — Core Principles

> **What this is:** the non-negotiable contract every other module in this design
> system serves. It fuses three lineages: **Apple's** Clarity / Deference / Depth (a
> recede-the-chrome, content-first discipline), **Goodpatch's** emotional craft +
> *Good Problems → Good People → Good Prototypes → Good Products* process and the
> **Minimum Lovable Product**, and a hard **anti-AI-slop** stance (thoughtlessness
> makes slop; AI just scales it).
>
> **How to use it (AI agents):** treat each principle as a gate. Before you ship a
> screen, component, or token change, name which principle it serves and run its
> do/don't. If a choice violates a principle, it is wrong even if it "looks fine."
> These instructions override convenience defaults.

---

## The 10 principles at a glance

| # | Principle | One-line why |
|---|---|---|
| 1 | **Frame the right problem first** | A beautiful answer to the wrong question is still wrong. |
| 2 | **Content first, chrome recedes** | The user's content is the product; UI is the frame, not the picture. |
| 3 | **Hierarchy through contrast, one focal point** | Equal emphasis is no emphasis — the eye needs somewhere to land. |
| 4 | **One dominant color, sharp semantic accents** | A point of view beats a timid, even palette. |
| 5 | **Design for hearts, build the Minimum Lovable Product** | Usable is table stakes; *lovable* is the bar. |
| 6 | **Restraint is confidence; refuse the median** | The work could only be *this* brand for *this* audience. |
| 7 | **Motion communicates, never decorates** | Animation earns its place by clarifying state, or it goes. |
| 8 | **Depth via layers and hairlines, not noise** | Real material has order; slop has stacked shadows. |
| 9 | **Accessibility & every state are first-class** | Empty/loading/error and a11y are the design, not the cleanup. |
| 10 | **Coherent whole, crafted at 800% zoom** | Identity is the sum of details no single screen reveals. |

---

## 1. Frame the right problem first

**Why:** A beautiful answer to the wrong question is still wrong. Goodpatch starts every
engagement at *Good Problems* — judging fit, understanding both the business challenge
*and* the user's real pain — before any pixel. Design is upstream, not decoration.

- **Do:** Write the problem, the user, and the success signal in one sentence before
  opening a design tool. Pull scope *back* under constraint; never expand time/budget.
- **Don't:** Jump to a UI layout because the ticket says "add a settings page." Don't
  call the first build an "MVP" when it has no designed-in *viability* — that is just a
  Minimal Product.

```text
PROBLEM:  Reviewers abandon multi-step approvals because they lose their place.
USER:     A manager approving 20+ requests/day on mobile, between meetings.
SIGNAL:   Median approvals-per-session up; "where was I?" support tickets → 0.
```

> **Agent rule:** if you cannot fill the three lines above, you are not ready to design.
> Ask the clarifying question instead of guessing the median.

---

## 2. Content first, chrome recedes

**Why:** The user's content is the product; UI is the frame, not the picture. Apple's
**Deference**: a crisp, unobtrusive interface helps people understand content without
competing with it. Controls *elevate* content; they don't bury it.

- **Do:** Let backgrounds run edge-to-edge; float the navigation layer above content
  (toolbars, tab bars, sheets) as a distinct functional layer with a 1px hairline or a
  subtle translucent surface. Keep the principal item in the upper-leading area.
- **Don't:** Box content inside cards-in-cards 3–5 levels deep, each with its own padding
  and shadow — the single most recognizable AI-slop signature. Don't apply "glass," heavy
  fills, or chrome to *content* itself; reserve materials for the navigation layer.

```css
/* The navigation layer floats above content — hairline + restraint, not a heavy box */
.app-bar {
  background: color-mix(in oklch, var(--color-surface) 88%, transparent);
  backdrop-filter: blur(12px) saturate(1.4);
  border-bottom: 1px solid var(--color-border-subtle);
  /* no big drop shadow; depth comes from the hairline + blur */
}
/* Content sits on the base canvas, unboxed */
.reading-pane { background: var(--color-bg); color: var(--color-fg); }
```

---

## 3. Hierarchy through contrast, one focal point per screen

**Why:** Equal emphasis is no emphasis. AI slop's flat hierarchy — uniform 16px radius,
identical padding, one font size — reads as "nothing matters." Real emphasis lives in
*contrast*: size, weight, color, and space. Every screen has exactly one thing that is
clearly most important.

- **Do:** Jump type sizes by 3x+ (e.g. `--text-5xl` headline over `--text-base` body),
  not a timid 1.5x. Use weight extremes (300 vs 800), not 400-vs-600. Let one element own
  the space; demote the rest with `--color-fg-muted` / `--color-fg-subtle`.
- **Don't:** Render three identical feature cards with the same weight, size, and an emoji
  each. Don't make every heading the same size with bold doing all the work.

```css
/* A real type ramp: dramatic jumps, paired leading/tracking, weight extremes */
.h-hero    { font: 800 var(--text-6xl)/var(--leading-tight) var(--font-display);
             letter-spacing: var(--tracking-tight); color: var(--color-fg); }
.h-section { font: 700 var(--text-3xl)/var(--leading-snug) var(--font-display); }
.body      { font: 400 var(--text-base)/var(--leading-relaxed) var(--font-body);
             color: var(--color-fg-muted); }
.kicker    { font: 600 var(--text-xs)/var(--leading-normal) var(--font-mono);
             letter-spacing: var(--tracking-wide); color: var(--color-fg-subtle);
             text-transform: uppercase; }
```

> **Swap-test:** if your headline and body were the same size, would the page still tell
> the reader where to look? If yes, your hierarchy is doing nothing.

---

## 4. One dominant color, sharp semantic accents

**Why:** A point of view beats a timid, even palette. The slop tell is the
purple/indigo → blue gradient on white — an arbitrary `bg-indigo-500` default that
saturated training data. Restraint with one committed color reads as confidence; color is
*semantic* (function-named), never decoration.

- **Do:** Commit to one dominant `--color-accent` used sparingly for the single most
  important action, and reserve `--color-success/-warning/-danger` strictly for state.
  Name color by role, never by hue.
- **Don't:** Ship a purple-on-white gradient, pastel-rainbow accents, or `bg-indigo-500`.
  Don't use `--color-danger` red as a decorative highlight — it means "destructive."

```css
:root {
  /* Semantic, OKLCH — adjust L & C in dark mode, never naive-invert */
  --color-accent:       oklch(0.62 0.19 256);   /* one dominant brand hue */
  --color-accent-fg:    oklch(0.98 0.01 256);
  --color-accent-hover: oklch(0.56 0.20 256);
  --color-success:      oklch(0.70 0.17 150);
  --color-warning:      oklch(0.80 0.16 85);
  --color-danger:       oklch(0.62 0.22 27);
  --color-ring:         var(--color-accent);
}
[data-theme="dark"] {
  --color-accent:       oklch(0.70 0.17 256);   /* lift L, ease C for dark canvas */
  --color-accent-hover: oklch(0.76 0.16 256);
}
```

> **Agent rule:** exactly one accent dominates. If you reach for a second decorative hue,
> you are decorating, not designing — use space and weight instead.

---

## 5. Design for hearts — build the Minimum Lovable Product

**Why:** Usable is table stakes; *lovable* is the bar. Goodpatch's DNA is "creating
lovable products"; the goal is to *move hearts* (心を動かす), not merely complete tasks.
Build **from the inside out**: desirability (human value) precedes feasibility and
viability — invert the usual order, and never defer lovability to "later."

- **Do:** Pick the one interaction that will make a user *love* this (the iPhone's
  multi-touch moment) and polish it past "fine." Aim for the first 1000 *fans*, not the
  first 1000 users. Pull scope back to make that one thing lovable rather than shipping
  ten things that are merely adequate.
- **Don't:** Spread effort thinly across every feature so nothing delights. Don't strip
  personality to hit a deadline — cut *scope*, not *craft*.

```text
MLP focusing question (answer before building):
  "Which single moment, done beautifully, turns a user into a fan?"
  → Make THAT moment lovable. Defer or cut everything that competes for its polish.
```

> **Constraint crafts clarity:** when over budget, the move is *pull back the scope*, not
> lower the quality. A small, lovable surface beats a large, likeable one.

---

## 6. Restraint is confidence; refuse the median

**Why:** "You're not getting design — you're getting the median." Models converge on the
statistical center: Inter, purple-on-white, centered-eyebrow hero, three emoji cards. The
fix is *direction* and *restraint*. White space signals confidence; the work could only be
*this* brand for *this* audience. Triangulate between two named anchors so the result reads
as authored, not copied.

- **Do:** Declare a tone before writing CSS (e.g. *editorial/magazine*, *industrial/
  utilitarian*, *brutalist/raw*) and a font pairing with real contrast (display + mono,
  or serif + geometric sans). State two reference anchors. Let empty space carry weight.
- **Don't:** Default to Inter/Roboto/Arial/system as the *only* font, reach reflexively for
  Space Grotesk or Instrument Serif, or ship untouched shadcn/Tailwind defaults. A kit is a
  starting point, not taste.

```css
/* Commit to a deliberate, high-contrast pairing — not the system default */
:root {
  --font-display: "Fraunces", "Times New Roman", serif;   /* editorial voice */
  --font-body:    "Source Sans 3", system-ui, sans-serif; /* calm workhorse  */
  --font-mono:    "JetBrains Mono", ui-monospace, monospace;
}
```

> **Swap-test (Anthropic's #1 tell):** could this exact UI be dropped onto a fintech, a CRM,
> *and* a to-do app with zero changes? If yes, it communicates nothing — it is slop.
> Make it fail the swap-test on purpose.

---

## 7. Motion communicates, never decorates

**Why:** Motion exists to show *state change, attention, or personality* — never as
decoration. Apple: add motion *purposefully*, keep it quick and precise, and avoid
animating frequent interactions. One orchestrated, staggered page-load beats scattered
micro-interactions. Easing and duration are *chosen*, not defaulted.

- **Do:** Use `--ease-spring` for user-driven motion (taps, drags, toggles) so it carries
  velocity; use `--ease-out`/`--ease-in-out` for entrances and continuous motion. Stagger
  one reveal on load. Always honor `prefers-reduced-motion` with a dissolve/cross-fade
  fallback — never strip meaning, swap the *form*.
- **Don't:** Apply uniform fade-ins to everything, snap buttons with no easing, or scatter
  unmotivated hover wiggles. Don't make motion the *only* way meaning is conveyed.

```css
:root {
  --dur-fast: 150ms; --dur-base: 250ms; --dur-slow: 400ms;
  --ease-out: cubic-bezier(0.16, 1, 0.3, 1);
  --ease-in-out: cubic-bezier(0.65, 0, 0.35, 1);
  /* spring approximation via linear() — overshoots, then settles */
  --ease-spring: linear(0, 0.18 7.3%, 0.66 19%, 1.01 35%, 1.06 49%, 1 73%, 1);
}
/* One orchestrated page-load: staggered, meaningful, not decorative */
@media (prefers-reduced-motion: no-preference) {
  .stagger > * { opacity: 0; transform: translateY(8px);
    animation: rise var(--dur-base) var(--ease-out) forwards;
    animation-delay: calc(var(--i, 0) * 60ms); }
  @keyframes rise { to { opacity: 1; transform: none; } }
}
@media (prefers-reduced-motion: reduce) {
  .stagger > * { opacity: 1; transform: none; animation: none; } /* dissolve, no travel */
}
```

---

## 8. Depth via layers and hairlines, not noise

**Why:** Real material has order; slop stacks `0.1`-opacity shadows on everything and calls
it depth. Apple's **Depth**: distinct layers and credible motion convey hierarchy. Prefer
1px hairline borders and a *layered, low-opacity* shadow system; let elevation mean
"this floats above that," not "this is a box."

- **Do:** Separate surfaces with `--color-border` / `--color-border-subtle` hairlines and a
  restrained `--shadow-sm/md/lg/xl` scale reserved for genuinely floating layers (menus,
  popovers, sheets). Keep nested radii *concentric*: child radius ≈ parent radius − padding.
- **Don't:** Put a soft `0.1` drop shadow on every card, use generic glassmorphism as
  decoration, or apply one uniform 16px radius to everything. Don't stack glass on glass.

```css
:root {
  --shadow-sm: 0 1px 2px oklch(0 0 0 / 0.06);
  --shadow-md: 0 2px 4px oklch(0 0 0 / 0.05), 0 4px 12px oklch(0 0 0 / 0.06);
  --shadow-lg: 0 8px 24px oklch(0 0 0 / 0.08), 0 2px 6px oklch(0 0 0 / 0.05);
}
/* Concentric radii: the inner curve stays parallel to the outer */
.panel { border-radius: var(--radius-xl); padding: var(--space-4); }
.panel > .control { border-radius: calc(var(--radius-xl) - var(--space-4)); }
/* Resting surfaces lean on hairlines; shadow is for things that truly float */
.card  { background: var(--color-surface); border: 1px solid var(--color-border-subtle); }
.popover { box-shadow: var(--shadow-lg); border: 1px solid var(--color-border); }
```

---

## 9. Accessibility & every state are first-class

**Why:** Competence without substance is slop. A design that only renders the happy path
is unfinished. Apple treats Dynamic Type, Reduce Motion, Increase Contrast, and ≥44pt
targets as *defaults* with meaningful fallbacks. Empty / loading / error states and a11y
are the design, not the cleanup afterward.

- **Do:** Design the empty, loading (skeleton, not just a spinner), and error states up
  front. Ensure ≥4.5:1 text contrast, visible `--color-ring` focus, ≥44×44px tap targets,
  and semantic HTML. Write error copy that says what happened and what to do next.
- **Don't:** Ship a screen with only its populated state. Don't remove focus outlines, rely
  on color alone to signal state, or use generic hedging copy ("something went wrong").

```css
/* Focus is never optional — visible, themeable, keyboard-only */
:where(a, button, input, [tabindex]):focus-visible {
  outline: 2px solid var(--color-ring);
  outline-offset: 2px;
  border-radius: var(--radius-sm);
}
.tap-target { min-block-size: 44px; min-inline-size: 44px; }
```

```text
ERROR-COPY contract:  [what happened] + [why, if useful] + [the next action]
  Bad:  "Something went wrong."
  Good: "Couldn't save your draft — you're offline. We'll retry when you reconnect."
```

> **Agent rule:** a component is not "done" until its empty, loading, and error states and
> its keyboard/contrast behavior all exist. List all four states before you call it shipped.

---

## 10. Coherent whole, crafted at 800% zoom

**Why:** Identity is the sum of details no single screen reveals — Anthropic's #1 grading
criterion is "does it feel like a coherent whole rather than a collection of parts?" One
type family, one icon set, one material, one motion vocabulary across every surface.
Goodpatch: *"beauty in process" is itself a deliverable* — the craft of how you build shows.

- **Do:** Source every value from the canonical tokens (`--color-*`, `--space-*`,
  `--text-*`, `--radius-*`, `--shadow-*`, `--ease-*`, `--dur-*`) so surfaces stay in sync.
  Refine optical alignment and spacing rhythm after the layout "works." Keep a
  generator/evaluator split — never let the agent that built it grade it.
- **Don't:** Hardcode hex values, magic pixel numbers, or one-off radii. Don't mix three
  font families or two motion styles. Don't let an agent praise its own output.

```css
/* Everything composes from one token vocabulary — that is the coherence */
.btn {
  font: 600 var(--text-sm)/var(--leading-none) var(--font-body);
  padding: var(--space-2) var(--space-4);
  border-radius: var(--radius-md);
  background: var(--color-accent); color: var(--color-accent-fg);
  transition: background var(--dur-fast) var(--ease-out);
}
.btn:hover { background: var(--color-accent-hover); }
```

> **Grading reframe:** ask not "is this beautiful?" but **"does this follow our
> principles?"** That makes taste gradable — and keeps the system honest across modules.

---

## Reference: principles as a React 19 + Tailwind v4 + Motion component

The same principles, framework-agnostic CSS above and a React reference here — never
*instead* of the plain version. Tailwind v4 mirrors the tokens via `@theme`.

```tsx
// CTA.tsx — one focal point (#3), one accent (#4), communicative motion (#7),
// first-class focus + reduced-motion (#9), token-only styling (#10).
"use client";
import { motion, useReducedMotion } from "motion/react";

export function CallToAction({ label, onAct }: { label: string; onAct: () => void }) {
  const reduce = useReducedMotion();
  return (
    <motion.button
      onClick={onAct}
      initial={reduce ? false : { opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.25, ease: [0.16, 1, 0.3, 1] }} // --ease-out / --dur-base
      whileTap={reduce ? undefined : { scale: 0.97 }}
      className="rounded-[--radius-md] bg-[--color-accent] px-[--space-4] py-[--space-2]
                 font-semibold text-[--color-accent-fg]
                 hover:bg-[--color-accent-hover]
                 focus-visible:outline focus-visible:outline-2
                 focus-visible:outline-offset-2 focus-visible:outline-[--color-ring]"
    >
      {label}
    </motion.button>
  );
}
```

```css
/* tailwind v4 — tokens are the single source of truth, mirrored into utilities */
@theme {
  --color-accent: oklch(0.62 0.19 256);
  --color-accent-fg: oklch(0.98 0.01 256);
  --color-accent-hover: oklch(0.56 0.20 256);
  --color-ring: var(--color-accent);
  --radius-md: 0.5rem;
  --space-2: 0.5rem;
  --space-4: 1rem;
  --text-sm: clamp(0.875rem, 0.84rem + 0.18vw, 0.95rem);
  --font-body: "Source Sans 3", system-ui, sans-serif;
}
```

---

## The contract in one breath

> Frame the **right problem** (1). Let **content lead** and chrome recede (2). Build real
> **hierarchy** to one focal point (3) with **one dominant, semantic color** (4). Aim for
> **lovable**, not merely usable (5). Choose **restraint** and refuse the median (6). Make
> **motion mean something** (7) and **depth come from order**, not noise (8). Treat
> **accessibility and every state** as the design (9). Ship a **coherent whole**, crafted
> to the last detail (10). Every other module in this system is an instance of these ten.

---

## Sources

- Apple — Human Interface Guidelines (Clarity / Deference / Depth; motion; layout; materials) — https://developer.apple.com/design/human-interface-guidelines/
- Apple — Liquid Glass (navigation layer vs. content; concentricity; depth) — https://developer.apple.com/documentation/TechnologyOverviews/liquid-glass
- Apple — Motion HIG (purposeful, quick motion; springs; Reduce Motion fallbacks) — https://developer.apple.com/design/human-interface-guidelines/motion
- Goodpatch — "Prototyping As Mindset" (Good Problems → People → Prototypes → Products; "good software is never done") — https://www.slideshare.net/slideshow/prototyping-as-mindset/55464932
- Goodpatch Global — "Less but More: The Minimum Lovable Product (MLP) Mindset" (4 pillars; desirability-first; first 1000 fans) — https://medium.com/goodpatch-global/less-but-more-the-minimum-lovable-product-mindset-46b1e09f6f53
- Goodpatch — Company / mission "prove the power of design," "move hearts," lovable-product DNA — https://goodpatch.com/company
- btrax / freshtrax — "AI's Impact on Design: Insights from Goodpatch CEO Naofumi Tsuchiya" (keep the human parts human; upstream strategy) — https://blog.btrax.com/ais-impact-on-design/
- Anthropic — Prompting for frontend aesthetics (Claude Cookbook; anti-slop, right-altitude direction) — https://platform.claude.com/cookbook/coding-prompting-for-frontend-aesthetics
- Anthropic — Improving frontend design through Skills (four grading criteria; generator/evaluator split) — https://claude.com/blog/improving-frontend-design-through-skills
- prg.sh — "Why Your AI Keeps Building the Same Purple Gradient Website" (distributional convergence; the indigo default) — https://prg.sh/ramblings/Why-Your-AI-Keeps-Building-the-Same-Purple-Gradient-Website
- 925studios — "AI Slop Web Design: Complete Guide (2026)" (visual tells; the swap-test) — https://www.925studios.co/blog/ai-slop-web-design-guide
- Kommers et al. — "Why Slop Matters" (MINT Lab, Indiana University), arXiv:2601.06060 — https://arxiv.org/abs/2601.06060
