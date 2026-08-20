# Motion Recipes

> Copy-paste motion patterns for the Awesome Design System. Plain HTML/CSS first (token-driven, zero-JS where possible), then a React 19 + Motion (`motion/react`) reference alongside — never instead of. Every recipe ships its accessibility fallback.
>
> **The one rule that governs all of these:** one strong, orchestrated staggered page-load reveal beats a dozen scattered micro-interactions. Choreograph the entrance, keep the rest restrained, and honor `prefers-reduced-motion` everywhere.

Library line as of June 2026: **Motion v12.x** (`npm install motion`, import from `motion/react`). The old `framer-motion` path still aliases but new code uses `motion`.

---

## Motion token contract (the shared vocabulary)

Every recipe below references these tokens. Define them once; never hardcode a duration or curve in a component.

```css
:root {
  /* Easing — chosen, not default */
  --ease-out: cubic-bezier(0.16, 1, 0.3, 1);     /* expo-out: snappy entrance */
  --ease-in-out: cubic-bezier(0.65, 0, 0.35, 1); /* in-place state change */
  --ease-spring: linear(                          /* spring approx via linear() */
    0, 0.006, 0.025, 0.101, 0.539, 0.826, 0.967,
    1.041, 1.058, 1.045, 1.018, 1.001, 0.997, 1
  );

  /* Duration — contextual, scale to distance */
  --dur-fast: 150ms;   /* micro: hover, tap, toggle */
  --dur-base: 250ms;   /* standard enter/exit */
  --dur-slow: 400ms;   /* inter-screen, large-scale */

  /* Orchestration */
  --stagger: 70ms;     /* gap between siblings in a cascade */
}
```

```js
// Mirror in JS so Motion (React) reads the SAME values as CSS.
export const ease = {
  out: [0.16, 1, 0.3, 1],
  inOut: [0.65, 0, 0.35, 1],
};
export const dur = { fast: 0.15, base: 0.25, slow: 0.4 };
export const STAGGER = 0.07;
```

**Easing rule of thumb:** `--ease-out` for things **entering** (fast in, settle gently), ease-in for things **exiting** (accelerate off-screen), `--ease-in-out` for state changes **in place**. Springs replace cubic-beziers when you want natural, interruptible motion.

**Compositor-only rule:** animate `transform` and `opacity` only. Never `width`/`height`/`top`/`left`/`box-shadow`/`filter` in a hot path — they trigger layout/paint and drop frames.

---

## Recipe 1 — Staggered page-load reveal (the centerpiece)

**When to use:** the single deliberate entrance on first paint. Header → hero → content cascade. This is the orchestrated reveal that earns its keep; do not also scatter micro-interactions on top of it.

### React + Motion (variants + staggerChildren)

```tsx
"use client";
import { motion } from "motion/react";

const container = {
  hidden: { opacity: 0 },
  show: {
    opacity: 1,
    transition: {
      when: "beforeChildren",
      staggerChildren: 0.07,  // --stagger, in seconds
      delayChildren: 0.1,
    },
  },
};

const item = {
  hidden: { opacity: 0, y: 24 },
  show: {
    opacity: 1,
    y: 0,
    transition: { type: "spring", visualDuration: 0.4, bounce: 0.25 },
  },
};

export function Hero() {
  return (
    <motion.section variants={container} initial="hidden" animate="show">
      <motion.p variants={item} className="eyebrow">Design System</motion.p>
      <motion.h1 variants={item}>Motion that means something</motion.h1>
      <motion.p variants={item}>One orchestrated entrance. Then restraint.</motion.p>
      <motion.div variants={item}>
        <a href="/start">Get started</a>
      </motion.div>
    </motion.section>
  );
}
```

Variants propagate down the tree: children sharing the variant name (`hidden`/`show`) animate automatically when the parent's state changes — no per-child `animate` needed. Shape the cascade with `stagger()`:

```tsx
import { stagger } from "motion/react";
// delayChildren: stagger(0.07, { from: "center" })  // ripple out from middle
// from: "first" | "last" | "center" | <index>
```

### CSS-only (keyframe + `animation-delay` stagger)

**When to use:** the entrance is content already in the DOM at load (no JS, no hydration wait). GPU-composited, runs off the main thread.

```css
@keyframes reveal-up {
  from { opacity: 0; transform: translateY(24px); }
  to   { opacity: 1; transform: translateY(0); }
}

.reveal-group > * {
  animation: reveal-up var(--dur-slow) var(--ease-out) both;
}
/* Stagger via per-child delay. `both` keeps the from-state before start. */
.reveal-group > *:nth-child(1) { animation-delay: 100ms; }
.reveal-group > *:nth-child(2) { animation-delay: 170ms; }
.reveal-group > *:nth-child(3) { animation-delay: 240ms; }
.reveal-group > *:nth-child(4) { animation-delay: 310ms; }
```

Prefer a computed delay so you never maintain `nth-child` lists by hand:

```css
.reveal-group > * {
  animation: reveal-up var(--dur-slow) var(--ease-out) both;
  animation-delay: calc(100ms + var(--i) * var(--stagger));
}
/* In markup: <li style="--i:0">, <li style="--i:1"> … */
```

> Keep the total cascade under ~600ms so it never blocks reading. Stagger 50–100ms between siblings.

---

## Recipe 2 — Scroll-linked reveal (reveal-on-scroll)

**When to use:** content below the fold that should fade/rise in as it enters the viewport — once, not on every scroll-by.

### CSS-only (`animation-timeline: view()`) — preferred

Native, runs off the main thread, no JS. Browser support 2026 is **not yet Baseline** (Chromium since 2023, Safari 26 shipped, Firefox behind a flag) — gate it behind `@supports` and pair with a Motion fallback or a static-visible default.

```css
@keyframes reveal-up {
  from { opacity: 0; transform: translateY(40px); }
  to   { opacity: 1; transform: translateY(0); }
}

@supports (animation-timeline: view()) {
  .scroll-reveal {
    animation: reveal-up linear both;
    animation-timeline: view();        /* subject's own visibility in scrollport */
    animation-range: entry 0% cover 40%; /* animate from entering until 40% covered */
  }
}
```

> **Gotcha:** the `animation` shorthand resets `animation-timeline` to `auto`. Always declare `animation-timeline` **after** the shorthand (as above). Without `@supports`, an unsupported browser shows the `from` state stuck — so default the element to its final visible state and only opt into the keyframe inside `@supports`.

### React + Motion (`whileInView`)

**When to use:** you need the reveal-once behavior on every browser today, or the element mounts dynamically.

```tsx
"use client";
import { motion } from "motion/react";

export function ScrollReveal({ children }: { children: React.ReactNode }) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 40 }}
      whileInView={{ opacity: 1, y: 0 }}
      viewport={{ once: true, amount: 0.3 }}  // fire once, when 30% visible
      transition={{ duration: 0.5, ease: [0.16, 1, 0.3, 1] }}
    >
      {children}
    </motion.div>
  );
}
```

### Scroll-progress bar (two ways)

```css
/* CSS-only: scroll() = nearest scroller progress, off main thread */
.progress {
  position: fixed; inset-inline: 0; top: 0; height: 4px;
  background: var(--color-accent);
  transform-origin: left; transform: scaleX(0);
  animation: grow-x linear;
  animation-timeline: scroll(root block);
}
@keyframes grow-x { to { transform: scaleX(1); } }
```

```tsx
"use client";
import { motion, useScroll } from "motion/react";

export function ProgressBar() {
  const { scrollYProgress } = useScroll();
  return (
    <motion.div
      style={{
        scaleX: scrollYProgress, transformOrigin: "left",
        position: "fixed", insetInline: 0, top: 0, height: 4,
        background: "var(--color-accent)",
      }}
    />
  );
}
```

### Parallax tied to a section (`useScroll` + `useTransform`)

**When to use:** you need a value derived from scroll progress (parallax offset, opacity ramp) that no `whileInView` toggle can express. Parallax is vestibular-risky — gate behind `useReducedMotion`.

```tsx
"use client";
import { motion, useScroll, useTransform, useReducedMotion } from "motion/react";
import { useRef } from "react";

export function Parallax() {
  const ref = useRef<HTMLElement>(null);
  const reduce = useReducedMotion();
  const { scrollYProgress } = useScroll({
    target: ref,
    offset: ["start end", "end start"], // enters bottom → exits top
  });
  const y = useTransform(scrollYProgress, [0, 1], ["-20%", "20%"]);
  return (
    <section ref={ref} style={{ overflow: "hidden" }}>
      <motion.img style={{ y: reduce ? 0 : y }} src="/bg.jpg" alt="" />
    </section>
  );
}
```

---

## Recipe 3 — Layout & shared-element animations (FLIP)

**When to use:** an element changes size or position as the DOM reflows (list reorder, expand/collapse, filter, grid → detail). The `layout` prop animates the change with FLIP — it animates `transform`, never the layout properties themselves, so it stays compositor-fast.

```tsx
"use client";
import { motion } from "motion/react";

// Animate ANY layout change (reorder, flex/grid shift, size change)
export function ExpandCard({ open, children }: { open: boolean; children: React.ReactNode }) {
  return (
    <motion.div
      layout
      transition={{ type: "spring", stiffness: 350, damping: 30 }}
      data-open={open}
    >
      {children}
    </motion.div>
  );
}
```

- `layout="position"` animates only position, `layout="size"` only size — cheaper when you know which changed.
- Curved morph paths (v12.x): `transition={{ layout: { path: arc() } }}` curves the morph instead of a straight line.

### Shared-element transition (`layoutId`)

Two elements with the same `layoutId` across states morph into each other — the canonical "card expands into detail view" effect.

```tsx
"use client";
import { motion, AnimatePresence } from "motion/react";
import { useState } from "react";

export function Gallery({ items }: { items: { id: string; title: string }[] }) {
  const [active, setActive] = useState<string | null>(null);
  return (
    <>
      <ul className="grid">
        {items.map((it) => (
          <motion.li key={it.id} layoutId={`card-${it.id}`}
                     onClick={() => setActive(it.id)}>
            {it.title}
          </motion.li>
        ))}
      </ul>

      <AnimatePresence>
        {active && (
          <motion.div
            layoutId={`card-${active}`}          // morphs FROM the clicked card
            className="detail"
            onClick={() => setActive(null)}
          >
            <motion.h2 layout>{active}</motion.h2>
          </motion.div>
        )}
      </AnimatePresence>
    </>
  );
}
```

> Wrap reordering lists in `<AnimatePresence mode="popLayout">` so a removed item pops out of flow and siblings reflow smoothly instead of jumping.

---

## Recipe 4 — AnimatePresence exit animations

**When to use:** an element leaves the React tree (modal close, toast dismiss, route content swap) and you want it to animate **out** before unmounting. `exit` only runs inside `<AnimatePresence>`.

```tsx
"use client";
import { AnimatePresence, motion } from "motion/react";

export function Modal({ open, onClose, children }: {
  open: boolean; onClose: () => void; children: React.ReactNode;
}) {
  return (
    <AnimatePresence>
      {open && (
        <>
          <motion.div
            key="backdrop"
            className="backdrop"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            transition={{ duration: 0.2, ease: [0.16, 1, 0.3, 1] }}
            onClick={onClose}
          />
          <motion.div
            key="dialog"
            role="dialog"
            aria-modal="true"
            initial={{ opacity: 0, scale: 0.95, y: 12 }}
            animate={{ opacity: 1, scale: 1, y: 0 }}
            exit={{ opacity: 0, scale: 0.95, y: 12 }}
            transition={{ type: "spring", visualDuration: 0.3, bounce: 0.2 }}
          >
            {children}
          </motion.div>
        </>
      )}
    </AnimatePresence>
  );
}
```

**Modes & options:**

| Option | Effect |
| --- | --- |
| `mode="wait"` | Finish the exit before the next element enters (swaps, tabs). |
| `mode="popLayout"` | Removed item pops out of flow so siblings reflow smoothly. Custom children must use `forwardRef`. Pairs with `layout`. |
| `initial={false}` | Skip the first mount animation (already-present content). |
| `propagate` (v12.x) | A parent's exit cascades to children. |

### Staggered list with exit

```tsx
"use client";
import { AnimatePresence, motion } from "motion/react";

export function TaskList({ tasks }: { tasks: { id: string; label: string }[] }) {
  return (
    <motion.ul layout>
      <AnimatePresence mode="popLayout" initial={false}>
        {tasks.map((t) => (
          <motion.li
            key={t.id}
            layout
            initial={{ opacity: 0, x: -16 }}
            animate={{ opacity: 1, x: 0 }}
            exit={{ opacity: 0, x: 16 }}
            transition={{ type: "spring", stiffness: 400, damping: 32 }}
          >
            {t.label}
          </motion.li>
        ))}
      </AnimatePresence>
    </motion.ul>
  );
}
```

---

## Recipe 5 — Hover & tap springs (restrained micro-motion)

**When to use:** affordance feedback on interactive elements. Keep it small (`scale` 1.02–1.04) and fast (`--dur-fast`). This is the *only* place scattered micro-motion is allowed — and only because it communicates "this is pressable", not for decoration.

```tsx
"use client";
import { motion } from "motion/react";

export function Button({ children }: { children: React.ReactNode }) {
  return (
    <motion.button
      whileHover={{ scale: 1.03 }}
      whileTap={{ scale: 0.97 }}
      transition={{ type: "spring", stiffness: 400, damping: 25 }}
    >
      {children}
    </motion.button>
  );
}
```

`scale` is a physical prop, so it springs by default. The spring is interruptible: a fast hover-in / hover-out won't jank because Motion redirects velocity rather than restarting.

### CSS-only equivalent (no JS)

```css
.btn {
  transition: transform var(--dur-fast) var(--ease-out);
}
.btn:hover  { transform: scale(1.03); }
.btn:active { transform: scale(0.97); }
```

> Hover effects must never be the only signal — pair with a color/border change so keyboard and touch users get the same affordance. Never animate `box-shadow` on hover in a list; animate a pseudo-element's `opacity` instead.

---

## Recipe 6 — `@starting-style` entry (CSS-only, no JS)

**When to use:** an element animates in on first render or when it goes from `display: none` to visible — toasts, popovers, `<dialog>`, dropdowns. Baseline since Aug 2024. CSS transitions normally **don't** fire on first paint or on `display` changes; `@starting-style` plus `transition-behavior: allow-discrete` fix exactly that.

```css
.toast {
  opacity: 1;
  transform: translateY(0);
  transition:
    opacity var(--dur-base) var(--ease-out),
    transform var(--dur-base) var(--ease-out),
    display var(--dur-base) allow-discrete; /* discrete prop needs allow-discrete */
}

/* Starting point for the FIRST render / display:none → block */
@starting-style {
  .toast {
    opacity: 0;
    transform: translateY(8px);
  }
}
```

### Popover / `<dialog>` entry + exit

Place `@starting-style` **after** the open rule (equal specificity), and include `overlay`/`display` with `allow-discrete` so the top-layer element animates out too.

```css
[popover] {
  opacity: 0;
  transform: scale(0.96);
  transition:
    opacity var(--dur-base) var(--ease-out),
    transform var(--dur-base) var(--ease-out),
    overlay var(--dur-base) allow-discrete,
    display var(--dur-base) allow-discrete;
}
[popover]:popover-open {
  opacity: 1;
  transform: scale(1);
}
@starting-style {
  [popover]:popover-open { opacity: 0; transform: scale(0.96); }
}
```

> This gives you a fully animated, accessible open/close with zero JavaScript — the browser handles the discrete `display`/`overlay` toggle. Prefer this over a JS library for native popovers and dialogs.

---

## Recipe 7 — View Transitions API (same-doc + cross-doc)

**When to use:** animated transitions between two DOM states or two pages — route changes, tab swaps, sort/filter. The browser snapshots old + new states and cross-fades (or morphs named elements) for you.

**Browser support 2026:**
- Same-document `document.startViewTransition()` — Chrome/Edge 111+, Safari 18+, Firefox 144+ (**Baseline newly available, Oct 2025**).
- Cross-document `@view-transition` — Chrome/Edge 126+, Safari 18.2+, Firefox in progress.

### Same-document (SPA) — always guard for fallback

```js
function navigate(updateDOM) {
  // Graceful fallback when unsupported: just run the update
  if (!document.startViewTransition) return updateDOM();

  const transition = document.startViewTransition(() => updateDOM());
  transition.ready.then(() => {/* animations have started */});
  transition.finished.then(() => {/* fully settled */});
}
```

### Cross-document (MPA) — pure CSS opt-in, on BOTH pages

```css
@view-transition { navigation: auto; }
```

```js
// Optional lifecycle hooks for fine control
window.addEventListener("pageswap",  (e) => { /* outgoing page snapshot */ });
window.addEventListener("pagereveal", (e) => { /* incoming page reveal */ });
```

### Naming a shared element to morph it

Give the same `view-transition-name` on both states and the browser morphs that element between them. Override the default cross-fade by styling the generated pseudo-elements.

```css
.hero-image { view-transition-name: hero; } /* must be unique per snapshot */

/* Pseudo tree: ::view-transition > group > image-pair > old / new */
::view-transition-old(hero) { animation: var(--dur-base) var(--ease-out) both fade-out; }
::view-transition-new(hero) { animation: var(--dur-base) var(--ease-out) both fade-in;  }
```

> **React 19 / Next.js:** the App Router and React Router support View Transitions, and React 19 exposes an experimental `<ViewTransition>` component. For frameworks without a native helper, wrap your route/state update in `startViewTransition` as above.

---

## Recipe 8 — `prefers-reduced-motion` (required on every recipe)

**When to use:** always. Some motion (large pans, zooms, parallax) triggers vestibular discomfort. The goal is to **reduce, not necessarily remove** — swap large transforms for opacity fades so the experience still feels intentional.

### CSS global safety net

```css
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after {
    animation-duration: 0.01ms !important;
    animation-iteration-count: 1 !important;
    transition-duration: 0.01ms !important;
    scroll-behavior: auto !important;
  }
}
```

### View Transitions

```css
@media (prefers-reduced-motion: reduce) {
  ::view-transition-group(*),
  ::view-transition-old(*),
  ::view-transition-new(*) { animation: none !important; }
}
```

### Motion (React) — site-wide

```tsx
import { MotionConfig } from "motion/react";
// "user" disables transform/layout animations but keeps opacity/color fades
<MotionConfig reducedMotion="user">{children}</MotionConfig>;
```

### Motion (React) — per-component (replace transform with fade)

```tsx
"use client";
import { motion, useReducedMotion } from "motion/react";

export function Sidebar({ isOpen }: { isOpen: boolean }) {
  const reduce = useReducedMotion(); // boolean, updates live on OS toggle
  const animate = isOpen
    ? (reduce ? { opacity: 1 } : { x: 0,        opacity: 1 })
    : (reduce ? { opacity: 0 } : { x: "-100%",  opacity: 0 });
  return <motion.div animate={animate} aria-hidden={!isOpen} />;
}
```

> Also: disable Lenis smooth scroll (`lenis.destroy()` / don't mount `ReactLenis`), kill parallax, and stop autoplaying video under reduced motion. `useReducedMotion()` returns a plain boolean usable with any library. SSR: default to a safe value on first render, reconcile on the client. Test via Chrome DevTools "Emulate prefers-reduced-motion: reduce" **and** real OS settings.

---

## Orchestration checklist (the taste layer)

- [ ] **One** orchestrated staggered entrance per screen — not a dozen scattered micro-interactions.
- [ ] Stagger 50–100ms between siblings; total cascade under ~600ms so it never blocks reading.
- [ ] Motion communicates **state / attention / personality** — never decoration.
- [ ] Animate **only** `transform` and `opacity` in hot paths (compositor-only, 60fps+).
- [ ] Same interaction type → same duration + easing, pulled from tokens (never hardcoded).
- [ ] Keep every animation **interruptible** — never lock the UI until it finishes.
- [ ] Reveal **once** (`viewport={{ once: true }}` / `animation-fill-mode: both`), not on every scroll-by.
- [ ] Duration is **contextual** — scale to travel distance and size. Keep frequent UI snappy (`--dur-fast`).
- [ ] Springs (`visualDuration` + `bounce`) for natural, interruptible motion; cubic-beziers for utilitarian transitions.
- [ ] Every recipe ships a `prefers-reduced-motion` path.

---

## Quick pick: which tool for which job

| Need | Reach for | Recipe |
| --- | --- | --- |
| Page-load entrance cascade | Motion variants OR CSS keyframe stagger | 1 |
| Reveal-on-scroll (once) | CSS `view()` → Motion `whileInView` fallback | 2 |
| Scroll-progress bar | CSS `scroll()` OR Motion `useScroll` | 2 |
| Parallax / scroll-derived value | Motion `useScroll` + `useTransform` | 2 |
| Reorder / expand / grid→detail morph | Motion `layout` / `layoutId` (FLIP) | 3 |
| Modal / toast leaving the tree | Motion `AnimatePresence` + `exit` | 4 |
| Pressable affordance | `whileHover`/`whileTap` OR CSS `:hover` | 5 |
| Toast / popover / dialog entry, no JS | CSS `@starting-style` + `allow-discrete` | 6 |
| Route / page transition | View Transitions API | 7 |
| Honor motion sensitivity | `prefers-reduced-motion` everywhere | 8 |

---

## Sources

- Motion for React — Animation (variants, orchestration) — https://motion.dev/docs/react-animation
- Motion for React — Transitions (spring/tween/stagger defaults) — https://motion.dev/docs/react-transitions
- Motion for React — Scroll animations (useScroll/useTransform) — https://motion.dev/docs/react-scroll-animations
- Motion for React — Layout animations — https://motion.dev/docs/react-layout-animations
- Motion for React — Accessibility / useReducedMotion — https://motion.dev/docs/react-accessibility
- Motion for React — Quick start — https://motion.dev/docs/react-quick-start
- Motion — Changelog (v12.39.0, 2026-05-18) — https://motion.dev/changelog
- MDN — View Transition API — https://developer.mozilla.org/en-US/docs/Web/API/View_Transition_API
- MDN — animation-timeline (scroll-driven) — https://developer.mozilla.org/en-US/docs/Web/CSS/animation-timeline
- MDN — @starting-style — https://developer.mozilla.org/en-US/docs/Web/CSS/@starting-style
- MDN — prefers-reduced-motion — https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-reduced-motion
- Material Design 3 — Easing and duration — https://m3.material.io/styles/motion/easing-and-duration
- Nielsen Norman Group — Executing UX Animations: Duration — https://www.nngroup.com/articles/animation-duration/
- Josh W. Comeau — Accessible Animations with prefers-reduced-motion — https://www.joshwcomeau.com/react/prefers-reduced-motion/
