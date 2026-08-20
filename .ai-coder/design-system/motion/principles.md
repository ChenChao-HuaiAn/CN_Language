# Motion Principles

> The canonical verdict on motion for this design system. Motion communicates **state, attention, and personality** — never decoration. This module gives you the philosophy, the exact easing/duration/spring tokens, orchestration recipes, and copy-paste code (CSS, React 19 + Motion, Tailwind v4). AI agents: everything you need is embedded here. Do not fetch the links.

---

## TL;DR — the seven laws

1. **Motion has a job.** Every animation answers one of: *What changed?* (state), *Look here* (attention), *This product has a point of view* (personality). If it answers none, delete it.
2. **One orchestrated entrance beats a dozen micro-interactions.** Lead with a single staggered page-load cascade (header → hero → content), then keep ongoing motion restrained.
3. **Springs for user-driven motion, easing for automatic motion.** Drags, toggles, sheets → spring (carries gesture velocity). Spinners, progress, ambient loops → easing/linear.
4. **Animate `transform` and `opacity` only.** They are compositor-only and hit 60fps+. Animating `width`/`height`/`top`/`left`/`box-shadow`/`filter` in a hot path is a bug.
5. **Ease-out enters, ease-in exits, ease-in-out moves in place.** Distance and size scale duration — never one fixed number for everything.
6. **`prefers-reduced-motion` is mandatory, and it means *reduce*, not *remove*.** Swap large transforms for opacity/color so meaning survives.
7. **Interruptible always.** An animation that locks the UI until it finishes is a bug dressed as a feature.

---

## 1. Purpose-driven motion

Apple and Google converged on the same doctrine: motion enlivens and clarifies, it never ornaments. Three legitimate jobs:

| Job | What it does | Examples |
| --- | --- | --- |
| **State** | Shows that something changed and *how* | Toggle flips, sheet slides up, list item reorders, validation turns a field red |
| **Attention** | Directs the eye to one focal point | Staggered reveal landing on the primary CTA, a badge popping in, an error shaking once |
| **Personality** | Expresses the product's point of view | A confident spring overshoot, a signature easing curve, a branded loading rhythm |

**Rules that fall out of this:**

- **Avoid motion on frequent, repeated interactions.** A 300ms animation seen 40×/day becomes friction. Keep high-frequency UI snappy (≤150ms) or instant.
- **Respect physical credibility.** A panel that slid *down* from the top must dismiss *upward* — not sideways. Direction encodes spatial memory (Apple HIG).
- **One focal point per screen.** Motion competes for attention; if everything moves, nothing is emphasized.
- **Restraint = confidence.** The most premium-feeling interfaces move *less*, but with precise timing and a single deliberate easing voice.

---

## 2. The spring-physics shift (2025→2026)

Both Apple (iOS 17+) and Google (Material 3 Expressive, May 2025) moved their *primary* motion language from fixed duration+easing to **physics-based springs**. The reason is interruptibility: a duration curve re-targeted mid-flight feels jarring; a spring recomputes a natural trajectory from its current position **and velocity** to the new target.

### Why springs win for interactive motion

- **Velocity continuity** — a spring picks up the speed from the end of a drag/swipe. Easing curves cannot; they always start from zero velocity, which feels disconnected after a gesture.
- **Re-targetable** — change the destination mid-animation and it stays smooth.
- **Two intuitive knobs** — modern springs (Apple, Motion) are parameterized by perceptual **duration** + **bounce**, not opaque stiffness/damping/mass.

### Apple's model: `duration` + `bounce`

```swift
// SwiftUI — since iOS 17, withAnimation defaults to a spring
withAnimation(.spring(duration: 0.6, bounce: 0.2)) { isExpanded.toggle() }

// Named presets along the damping spectrum:
.smooth   // critically damped, no overshoot
.snappy   // slight bounce (~0.15)
.bouncy   // more overshoot (~0.3)
```

`duration` here is the **perceptual** time-to-target (predictable), distinct from the unpredictable physical settling time. Sequence follow-up UI on the perceptual duration, never the settle.

### Material 3 Expressive: spatial vs. effects, three speeds

M3E splits springs into two categories and three speeds. **Spatial** springs (position, size, rotation, corner radius) overshoot and bounce; **Effects** springs (color, opacity) never overshoot (damping = 1.0).

| Token | Damping | Stiffness | Use |
| --- | --- | --- | --- |
| `motionSpringFastSpatial` | 0.9 | 1400 | Small components (switches, chips) |
| `motionSpringFastEffects` | 1.0 | 3800 | Small-component color/opacity |
| `motionSpringDefaultSpatial` | 0.9 | 700 | Partial-screen positioning |
| `motionSpringDefaultEffects` | 1.0 | 1600 | Partial-screen color/opacity |
| `motionSpringSlowSpatial` | 0.9 | 300 | Full-screen positioning |
| `motionSpringSlowEffects` | 1.0 | 800 | Full-screen color/opacity |

> **Speed rule:** small / short distance → **fast**; full-screen → **slow**; everything between → **default**. Numbers are device-scaled, but the ordering (fast > default > slow) is invariant.

### Spring cheat-sheet (our defaults, framework-agnostic)

| Intent | Apple `duration`/`bounce` | Motion `visualDuration`/`bounce` | M3E analog | Feel |
| --- | --- | --- | --- | --- |
| **Effects** (color/opacity) | n/a (use tween) | — | `defaultEffects` (damping 1.0) | No overshoot |
| **Snappy** (switch, chip, tap) | `0.3 / 0.15` | `{ visualDuration: 0.3, bounce: 0.15 }` | `fastSpatial` | Crisp, tiny settle |
| **Standard** (sheet, popover, card) | `0.5 / 0.2` | `{ visualDuration: 0.5, bounce: 0.2 }` | `defaultSpatial` | Lively, controlled |
| **Expressive** (hero, FAB morph) | `0.6 / 0.35` | `{ visualDuration: 0.6, bounce: 0.35 }` | `slowSpatial` (expressive) | Playful overshoot |
| **Calm** (full-screen nav, push/pop) | `0.5 / 0` (`.smooth`) | `{ visualDuration: 0.5, bounce: 0 }` | `slowSpatial` (standard) | No bounce, deliberate |

**Bounce guide:** `0` = critically damped (no overshoot, use for exits + utilitarian nav + all color/opacity); `0.15–0.2` = the safe default for UI; `0.3–0.4` = expressive hero moments only. Above `0.5` reads as gimmicky outside playful brands.

---

## 3. Easing + duration tokens (the utilitarian baseline)

Springs are primary for interactive motion, but fixed easing/duration is still the right tool for **automatic, continuous, or one-directional** motion (fades, spinners, progress, simple enter/exit). Linear motion never exists in the real world — objects accelerate and decelerate — so every tween needs a curve.

**Which curve, when:**

- **Ease-out** for elements **entering** — fast in, settles gently, feels responsive. This is the workhorse; default to it.
- **Ease-in** for elements **exiting** — accelerates off-screen, demands no attention.
- **Ease-in-out** for elements **moving between states in place** — symmetric, balanced.
- **Linear** only for continuous loops (spinners, marquees, progress fills) where acceleration would look wrong.

### Canonical CSS tokens

```css
:root {
  /* Easing — chosen curves, not browser defaults */
  --ease-out:    cubic-bezier(0.16, 1, 0.3, 1);     /* expo-out: snappy, premium entrance */
  --ease-in:     cubic-bezier(0.7, 0, 0.84, 0);     /* expo-in: accelerate off-screen */
  --ease-in-out: cubic-bezier(0.65, 0, 0.35, 1);    /* symmetric, in-place moves */
  --ease-spring: linear(                            /* spring overshoot via CSS linear() */
    0, 0.006, 0.025, 0.101, 0.224, 0.394, 0.591, 0.781,
    0.925, 1.019, 1.063, 1.07, 1.057, 1.035, 1.014,
    0.999, 0.991, 0.988, 0.991, 0.995, 1
  );

  /* Duration — contextual baseline, scale with distance/size */
  --dur-fast: 150ms;   /* micro-interactions: hover, tap, toggle, focus */
  --dur-base: 250ms;   /* standard transitions: most enter/exit */
  --dur-slow: 400ms;   /* inter-screen / large-distance / page transitions */
}
```

> **Why these curves?** The browser defaults (`ease`, `ease-in-out`) are timid and over-symmetric. `cubic-bezier(0.16, 1, 0.3, 1)` (expo-out) front-loads the motion so UI snaps toward the user then settles — the single biggest upgrade over `ease`. The `--ease-spring` `linear()` approximation gives CSS a real overshoot for the cases where you can't reach for JS.

### Duration is contextual, never a constant

| Use case | Token | Easing |
| --- | --- | --- |
| Hover, tap, toggle, focus ring | `--dur-fast` (150ms) | `--ease-out` |
| Standard enter | `--dur-base` (250ms) | `--ease-out` |
| Standard exit | `--dur-fast`→`--dur-base` | `--ease-in` |
| Move between states in place | `--dur-base` (250ms) | `--ease-in-out` |
| Inter-screen / page transition | `--dur-slow` (400ms) | `--ease-in-out` |
| Large-distance / full-screen | `--dur-slow`+ (scale up) | `--ease-in-out` or calm spring |

**Material 3's reference grid** maps neatly onto this: `short3` 150ms ≈ `--dur-fast`, `medium1` 250ms ≈ `--dur-base`, `medium4` 400ms ≈ `--dur-slow`. M3's emphasized curves are great alternates: `emphasized-decelerate` `cubic-bezier(0.05, 0.7, 0.1, 1)` for entrances, `emphasized-accelerate` `cubic-bezier(0.3, 0, 0.8, 0.15)` for exits.

### Tailwind v4 `@theme` mirror

```css
/* app.css — single source of truth, mirrored into Tailwind utilities */
@import "tailwindcss";

@theme {
  --ease-out:    cubic-bezier(0.16, 1, 0.3, 1);
  --ease-in:     cubic-bezier(0.7, 0, 0.84, 0);
  --ease-in-out: cubic-bezier(0.65, 0, 0.35, 1);

  --animate-duration-fast: 150ms;
  --animate-duration-base: 250ms;
  --animate-duration-slow: 400ms;
}
```

```html
<!-- Usage: tokens become utilities — ease-out, duration-base, etc. -->
<button class="transition-transform duration-fast ease-out hover:scale-[1.03] active:scale-[0.97]">
  Send Payment
</button>
```

---

## 4. Orchestration: one staggered page-load

The single highest-leverage motion decision: **lead with one deliberate, staggered entrance, then keep the rest quiet.** A scattered field of micro-interactions reads as noise; an orchestrated cascade reads as craft. This is where personality lives.

**The recipe:**

- **Cascade order follows reading order:** header → hero headline → subhead → primary CTA → supporting content. The eye lands on the focal point last, so the CTA is where motion *resolves*.
- **Stagger 50–100ms between siblings.** Below 50ms it looks simultaneous; above 100ms it drags.
- **Total cascade under ~600ms** so it never blocks reading. Content must be usable immediately.
- **Enter with `--ease-out` or a `bounce: 0.2` spring**, translating ≤24px. Large travel triggers vestibular discomfort and looks heavy.
- **Reveal once.** Never re-animate on every scroll-by.

### Framework-agnostic: CSS-only staggered reveal

Uses `@starting-style` (Baseline since Aug 2024) so the entrance fires on first paint with zero JS. The stagger is a per-item `--i` custom property feeding `transition-delay`.

```css
.reveal {
  --i: 0;
  opacity: 1;
  transform: translateY(0);
  transition:
    opacity var(--dur-base) var(--ease-out),
    transform var(--dur-base) var(--ease-out);
  transition-delay: calc(var(--i) * 80ms);
}

/* First-paint starting state — the "hidden" frame */
@starting-style {
  .reveal {
    opacity: 0;
    transform: translateY(20px);
  }
}
```

```html
<header class="reveal" style="--i: 0">Logo</header>
<h1     class="reveal" style="--i: 1">One sharp headline</h1>
<p      class="reveal" style="--i: 2">A calm, confident subhead.</p>
<a      class="reveal" style="--i: 3" href="/start">Start free</a>
```

### React 19 + Motion: the canonical entrance

Variants propagate down the tree; children with the same variant name animate automatically. `delayChildren: stagger()` is the orchestration backbone. The focal CTA gets the only spring with visible bounce.

```tsx
"use client";
import { motion, stagger } from "motion/react";

const container = {
  hidden: { opacity: 0 },
  show: {
    opacity: 1,
    transition: { when: "beforeChildren", delayChildren: stagger(0.08) },
  },
};

const item = {
  hidden: { opacity: 0, y: 20 },
  show: {
    opacity: 1,
    y: 0,
    transition: { type: "spring", visualDuration: 0.5, bounce: 0.2 },
  },
};

export function Hero() {
  return (
    <motion.section variants={container} initial="hidden" animate="show">
      <motion.p variants={item} className="text-fg-muted">Design system</motion.p>
      <motion.h1 variants={item} className="text-6xl font-display">
        Motion with a point of view
      </motion.h1>
      <motion.p variants={item} className="text-fg-muted">
        Restraint is the feature.
      </motion.p>
      {/* Focal point resolves last with the only expressive bounce */}
      <motion.a
        variants={{
          hidden: { opacity: 0, y: 20, scale: 0.96 },
          show: {
            opacity: 1, y: 0, scale: 1,
            transition: { type: "spring", visualDuration: 0.6, bounce: 0.35 },
          },
        }}
        href="/start"
      >
        Start free
      </motion.a>
    </motion.section>
  );
}
```

`stagger()` also takes `from: "first" | "last" | "center" | <index>` to shape the cascade origin — `from: "center"` radiates outward, great for grids.

---

## 5. Choreography: stagger, exit, reveal-on-scroll

### Reveal-once on scroll (CSS-first, Motion fallback)

Native scroll-driven animation runs off the main thread. As of 2026 it is shipping in Chromium and Safari 26 but not yet Baseline (Firefox behind a flag) — gate it behind `@supports` and treat it as progressive enhancement.

```css
@supports (animation-timeline: view()) {
  .on-scroll {
    animation: reveal-up linear both;
    animation-timeline: view();
    animation-range: entry 0% cover 35%;   /* animate while entering the viewport */
  }
}
@keyframes reveal-up {
  from { opacity: 0; transform: translateY(24px); }
  to   { opacity: 1; transform: translateY(0); }
}
```

> **Gotcha:** the `animation` shorthand resets `animation-timeline` to `auto`. Always declare `animation-timeline` *after* any `animation` shorthand.

When you need JS (broad browser support, or per-item stagger), `whileInView` fires once with no manual transform math:

```tsx
<motion.div
  initial={{ opacity: 0, y: 32 }}
  whileInView={{ opacity: 1, y: 0 }}
  viewport={{ once: true, amount: 0.3 }}   // fire once, when 30% visible
  transition={{ duration: 0.5, ease: [0.16, 1, 0.3, 1] }}  /* --ease-out */
/>
```

### Exit choreography with `AnimatePresence`

`exit` only runs when the element is wrapped in `AnimatePresence` and removed from the tree. Match the entrance: enter `--ease-out`, exit `--ease-in` (faster, less attention).

```tsx
"use client";
import { AnimatePresence, motion } from "motion/react";

export function Toast({ open, message }: { open: boolean; message: string }) {
  return (
    <AnimatePresence>
      {open && (
        <motion.div
          key="toast"
          role="status"
          initial={{ opacity: 0, y: 12, scale: 0.98 }}
          animate={{ opacity: 1, y: 0, scale: 1 }}
          exit={{ opacity: 0, y: 8, scale: 0.98, transition: { duration: 0.15, ease: [0.7, 0, 0.84, 0] } }}
          transition={{ type: "spring", visualDuration: 0.3, bounce: 0.15 }}
        >
          {message}
        </motion.div>
      )}
    </AnimatePresence>
  );
}
```

`AnimatePresence` modes: `mode="wait"` finishes the exit before the next enters; `mode="popLayout"` pops removed items out of flow so siblings reflow smoothly (pair with the `layout` prop for reordering lists).

### Shared-element & layout (FLIP)

The `layout` prop animates size/position changes by animating transforms (FLIP) — performant, never layout-thrashing. A matching `layoutId` across two elements morphs between them (list card → detail view).

```tsx
<motion.div layout transition={{ type: "spring", visualDuration: 0.5, bounce: 0.2 }} />

{/* Shared element: same layoutId in two places morphs between them */}
<motion.div layoutId="card-42" />   {/* list view */}
<motion.div layoutId="card-42" />   {/* detail view — morphs from the card */}
```

### Page / route transitions (View Transitions API)

Browser-native morph between DOM states. Same-document is Baseline (Oct 2025); cross-document (`@view-transition { navigation: auto; }`) ships in Chrome 126+/Safari 18.2+. Always feature-detect.

```ts
function navigate(updateDOM: () => void) {
  if (!document.startViewTransition) return updateDOM();  // graceful fallback
  document.startViewTransition(() => updateDOM());
}
```

---

## 6. `prefers-reduced-motion` (mandatory)

Large pans, zooms, parallax, and spinning trigger vestibular discomfort for real users. The OS exposes the preference; honor it everywhere. **Reduce — don't strip.** If motion conveys meaning (a state change, a hierarchy shift), keep a non-moving equivalent: cross-fade, highlight fade, or color shift. The OS itself swaps slide/zoom for dissolve.

### CSS global safety net (ship this in your reset)

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

This is a blunt floor, not a strategy. The intentional approach keeps a fade where the full motion was a transform:

```css
.sidebar {
  transition: transform var(--dur-base) var(--ease-out);
  transform: translateX(0);
}
.sidebar[data-state="closed"] { transform: translateX(-100%); }

@media (prefers-reduced-motion: reduce) {
  .sidebar {
    /* swap the slide for an opacity fade — meaning survives, motion doesn't */
    transition: opacity var(--dur-fast) var(--ease-out);
    transform: none;
  }
  .sidebar[data-state="closed"] { opacity: 0; }
}
```

### View Transitions under reduced motion

```css
@media (prefers-reduced-motion: reduce) {
  ::view-transition-group(*),
  ::view-transition-old(*),
  ::view-transition-new(*) { animation: none !important; }
}
```

### Motion (React): site-wide + per-component

`MotionConfig reducedMotion="user"` disables transform/layout animations site-wide while **keeping opacity/color** — exactly the "reduce not remove" doctrine.

```tsx
"use client";
import { MotionConfig } from "motion/react";

export function Providers({ children }: { children: React.ReactNode }) {
  return <MotionConfig reducedMotion="user">{children}</MotionConfig>;
}
```

For surgical control, `useReducedMotion()` returns a live boolean — swap transform for fade per component:

```tsx
"use client";
import { motion, useReducedMotion } from "motion/react";

export function Sheet({ open }: { open: boolean }) {
  const reduce = useReducedMotion();
  const animate = open
    ? reduce ? { opacity: 1 } : { y: 0, opacity: 1 }
    : reduce ? { opacity: 0 } : { y: "100%", opacity: 0 };
  return (
    <motion.div
      animate={animate}
      transition={reduce
        ? { duration: 0.15 }
        : { type: "spring", visualDuration: 0.5, bounce: 0.2 }}
    />
  );
}
```

Also disable smooth-scroll (e.g. Lenis), parallax, and autoplaying video under reduced motion. SSR: default to a safe value on first render, reconcile on the client. Test with DevTools "Emulate prefers-reduced-motion: reduce" **and** the real OS setting.

---

## 7. Performance: transform + opacity only

Animating layout-affecting properties forces the browser to recompute geometry every frame (layout → paint → composite), which drops frames. **Only `transform` and `opacity` are compositor-only** — they skip layout and paint, run on the GPU, and can even run off the main thread.

| Property | Cost | Verdict |
| --- | --- | --- |
| `transform`, `opacity` | Composite only | ✅ Animate freely |
| `color`, `background-color` | Paint | ⚠️ OK for small areas / low frequency |
| `box-shadow`, `filter`, `backdrop-filter` | Paint (expensive) | ⚠️ Avoid in hot paths; animate a layered pseudo-element's opacity instead |
| `width`, `height`, `top`, `left`, `margin` | Layout + paint | ❌ Never in a hot path — use `transform: scale()` / `translate()` |

**Tactics:**

- Need a size change? Animate `transform: scaleX()/scaleY()` (FLIP), or let Motion's `layout` prop do the FLIP math for you.
- Need an elevation change on hover? Pre-render two shadow layers and cross-fade their `opacity` — never animate `box-shadow` directly.
- `will-change: transform` sparingly, on the element about to animate; remove it after. Permanent `will-change` wastes GPU memory.
- Keep cascades **interruptible** — never block interaction until an animation finishes.

```css
/* Elevation-on-hover without animating box-shadow */
.card { position: relative; transition: transform var(--dur-fast) var(--ease-out); }
.card::after {
  content: ""; position: absolute; inset: 0; border-radius: inherit;
  box-shadow: var(--shadow-lg); opacity: 0;
  transition: opacity var(--dur-fast) var(--ease-out);
  pointer-events: none;
}
.card:hover { transform: translateY(-2px); }
.card:hover::after { opacity: 1; }   /* cross-fade the shadow, don't animate it */
```

---

## 8. State motion: loading, empty, error

Motion must cover the unglamorous states, and these are where `--ease-spring`/`linear` and reduced-motion rules earn their keep.

**Loading** — continuous motion uses `linear` (acceleration looks wrong on a loop). Prefer skeletons over spinners for content; a pulsing skeleton communicates "structure incoming."

```css
.skeleton {
  background: linear-gradient(90deg,
    var(--color-surface-2) 25%, var(--color-surface) 37%, var(--color-surface-2) 63%);
  background-size: 400% 100%;
  animation: shimmer 1.4s linear infinite;
}
@keyframes shimmer { to { background-position: -200% 0; } }

@media (prefers-reduced-motion: reduce) {
  .skeleton { animation: none; background: var(--color-surface-2); }  /* static fallback */
}
```

**Empty** — animate the empty state *in* once (a calm fade-up), then hold still. No looping illustration; it reads as a stuck UI.

**Error** — one decisive signal, never a loop. A single horizontal shake on a field plus a color shift to `--color-danger` says "fix this" without nagging. Honor reduced-motion by dropping the shake and keeping the color.

```tsx
"use client";
import { motion, useReducedMotion } from "motion/react";

export function FieldError({ invalid }: { invalid: boolean }) {
  const reduce = useReducedMotion();
  return (
    <motion.input
      aria-invalid={invalid}
      animate={invalid && !reduce ? { x: [0, -6, 6, -4, 4, 0] } : { x: 0 }}
      transition={{ duration: 0.35, ease: [0.65, 0, 0.35, 1] }}  /* --ease-in-out */
      style={{ borderColor: invalid ? "var(--color-danger)" : "var(--color-border)" }}
    />
  );
}
```

---

## 9. Do / Don't

| Don't | Do |
| --- | --- |
| Animate `width`/`height`/`top`/`left` | Animate `transform` + `opacity` |
| Scatter micro-interactions everywhere | One orchestrated staggered entrance, then restraint |
| Use the browser default `ease` curve | Use `--ease-out` (`cubic-bezier(0.16, 1, 0.3, 1)`) |
| One fixed duration for everything | Scale duration to distance/size; fast for frequent UI |
| `bounce: 0.6+` on everything | `bounce: 0.15–0.2` default; `0.3–0.4` for hero only |
| Spring an automatic spinner | `linear` for loops, springs for user-driven motion |
| Strip all motion under reduced-motion | Reduce to a fade/color shift so meaning survives |
| Loop an error or empty-state animation | One decisive signal, then hold still |
| Block the UI until the animation ends | Keep every animation interruptible |
| Animate `box-shadow` on hover | Cross-fade a layered pseudo-element's opacity |

---

## Sources

- Motion for React — Animation (variants, orchestration) — https://motion.dev/docs/react-animation
- Motion for React — Transitions (spring `visualDuration`/`bounce`, stagger) — https://motion.dev/docs/react-transitions
- Motion for React — Scroll animations (`useScroll`, `whileInView`) — https://motion.dev/docs/react-scroll-animations
- Motion for React — Layout animations (FLIP, `layoutId`) — https://motion.dev/docs/react-layout-animations
- Motion for React — Accessibility / `useReducedMotion` / `MotionConfig` — https://motion.dev/docs/react-accessibility
- Apple HIG — Motion — https://developer.apple.com/design/human-interface-guidelines/motion
- Animate with springs — WWDC23 session 10158 — https://developer.apple.com/videos/play/wwdc2023/10158/
- `spring(duration:bounce:blendDuration:)` — Apple Developer — https://developer.apple.com/documentation/SwiftUI/Animation/spring(duration:bounce:blendDuration:)
- Apple — Reduced Motion evaluation criteria — https://developer.apple.com/help/app-store-connect/manage-app-accessibility/reduced-motion-evaluation-criteria/
- Material Design 3 — Motion overview (physics/springs) — https://m3.material.io/styles/motion/overview/how-it-works
- Material Design 3 — Easing & duration tokens/specs — https://m3.material.io/styles/motion/easing-and-duration/tokens-specs
- M3 Expressive — New Motion System (blog) — https://m3.material.io/blog/m3-expressive-motion-theming
- MDC-Android — Motion docs (spring/easing tokens) — https://github.com/material-components/material-components-android/blob/master/docs/theming/Motion.md
- MDN — `@starting-style` — https://developer.mozilla.org/en-US/docs/Web/CSS/@starting-style
- MDN — `animation-timeline` (scroll-driven) — https://developer.mozilla.org/en-US/docs/Web/CSS/animation-timeline
- MDN — View Transition API — https://developer.mozilla.org/en-US/docs/Web/API/View_Transition_API
- MDN — `prefers-reduced-motion` — https://developer.mozilla.org/en-US/docs/Web/CSS/@media/prefers-reduced-motion
- Nielsen Norman Group — Executing UX Animations: Duration — https://www.nngroup.com/articles/animation-duration/
- Josh W. Comeau — Accessible Animations with prefers-reduced-motion — https://www.joshwcomeau.com/react/prefers-reduced-motion/
