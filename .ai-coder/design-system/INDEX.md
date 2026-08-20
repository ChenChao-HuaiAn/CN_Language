# Design System — INDEX (router for AI agents)

> **How to use this file.** This is a *map*, not content. Read it first, then open **only**
> the 1–3 modules your task needs. Each module is self-contained and embeds copy-paste code,
> so you never have to fetch an external link to be productive. This keeps your context lean.
>
> **Always-read core:** before generating ANY UI, read `00-philosophy/human-not-ai.md`
> (the taste layer) + `00-philosophy/principles.md`. They are short and prevent "AI slop".

## Route by task

| If the task is about… | Read these module(s) |
|---|---|
| **Any new UI / "make it modern / sophisticated"** | `00-philosophy/human-not-ai.md`, `00-philosophy/modern-2026.md`, then the specific module(s) below |
| Defining the design ethos / what "good"/"modern" means | `00-philosophy/principles.md`, `00-philosophy/modern-2026.md` |
| Detecting / fixing generic "AI slop" | `00-philosophy/human-not-ai.md` |
| **Color**, palettes, OKLCH, dark mode, contrast | `foundations/color.md` |
| **Typography**, fonts, type scale, 日本語/和文 type | `foundations/typography.md` |
| **Spacing**, grid, layout, bento, breakpoints | `foundations/spacing-layout.md` |
| **Design tokens** (the canonical variable set, Tailwind v4 `@theme`) | `foundations/tokens.md` |
| **Motion** principles, easing, springs, reduce-motion | `motion/principles.md` |
| **Animation code** (Motion/React + CSS recipes) | `motion/recipes.md` |
| **Components** (button, card, input, dialog, tabs…) + states + a11y | `components/components.md` |
| **Page composition** (hero, bento, sections, nav, backgrounds) | `patterns/patterns.md` |
| **Accessibility** (WCAG, focus, keyboard, contrast) | `accessibility/a11y.md` |
| **Tech stack** choice / project setup (2026) | `tech-stack-2026/stack.md` |
| Building a brand-specific design system | `foundations/tokens.md` + `00-philosophy/*` (then the `/MakeAwesomeDS` skill) |
| Producing a polished single-file HTML doc | `foundations/tokens.md` + `foundations/typography.md` + `motion/recipes.md` (then `/AwesomeHTML`) |

## Module map

```
00-philosophy/
  human-not-ai.md     ← taste layer: AI-slop tells, human-feel principles, anti-slop checklist
  modern-2026.md      ← definition of "modern" as of 2026-06 (Apple/Google/Nothing/SaaS frontier)
  principles.md       ← the non-negotiable core principles every module serves
foundations/
  color.md            ← OKLCH, P3, contrast, dark mode, semantic color tokens
  typography.md       ← font selection/pairing, fluid scale, 和文 (Japanese) typography
  spacing-layout.md   ← 8pt scale, grid, bento, breakpoints, editorial composition
  tokens.md           ← THE canonical token contract (CSS vars + Tailwind v4 @theme) ★ source of truth
motion/
  principles.md       ← purpose-driven motion, springs, easing/duration tokens, reduce-motion
  recipes.md          ← copy-paste Motion (React) + CSS animation recipes
components/
  components.md       ← button/card/input/dialog/tabs… with all states + a11y (framework-agnostic + React)
patterns/
  patterns.md         ← hero/bento/sections/nav/footer/backgrounds (anti-slop, with code)
accessibility/
  a11y.md             ← WCAG 2.2, focus, keyboard, contrast, reduce-motion, 日本語 a11y
tech-stack-2026/
  stack.md            ← the recommended AI-native 2026 stack + alternatives + setup
```

## The token contract (memorize the names)

All modules speak one token vocabulary. Full definitions live in `foundations/tokens.md`.

- **Color:** `--color-bg`, `--color-bg-subtle`, `--color-surface`, `--color-surface-2`, `--color-fg`, `--color-fg-muted`, `--color-fg-subtle`, `--color-border`, `--color-border-subtle`, `--color-accent`, `--color-accent-fg`, `--color-accent-hover`, `--color-success|warning|danger(+ -fg)`, `--color-ring`
- **Space:** `--space-1`…`--space-32` · **Radius:** `--radius-sm|md|lg|xl|2xl|full`
- **Type:** `--text-xs`…`--text-7xl` (+ `--leading-*`, `--tracking-*`) · **Fonts:** `--font-display|body|mono`
- **Elevation:** hairline `1px` borders + `--shadow-sm|md|lg|xl`
- **Motion:** `--ease-out|in-out|spring`, `--dur-fast|base|slow`

## Non-negotiables (the 10-second version)

1. Read the taste layer first. Never ship the statistical median.
2. One dominant color + sharp accents. Color is semantic. No purple-on-white.
3. A deliberate, distinctive font choice — never Inter/Arial as the only option.
4. Real hierarchy via contrast; one focal point per screen. Restraint = confidence.
5. Use the token vocabulary; theme shadcn/Tailwind — never ship raw defaults.
6. Motion communicates; one orchestrated page-load beats scattered micro-interactions.
7. Always design error/empty/loading states and meet WCAG 2.2 AA.
8. Framework-agnostic first; React/Tailwind v4/Motion as the reference implementation.
