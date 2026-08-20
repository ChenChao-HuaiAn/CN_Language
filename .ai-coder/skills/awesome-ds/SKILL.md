---
name: awesome-ds
description: >-
  Apply the AwesomeDesignSystem to build or refine sophisticated, modern (2026),
  human-feeling frontend UI that avoids generic "AI slop". Use when building or
  improving ANY web UI, component, landing page, dashboard, or app screen; when
  choosing color, typography, spacing, motion, or layout; or when the user asks
  to make something "modern", "polished", "sophisticated", "high-end", or "less
  AI-generated", or references Apple, Linear, Stripe, Vercel, Nothing, or Goodpatch.
  Loads the design system's taste layer first, then routes to ONLY the needed modules.
---

# AwesomeDS — the design taste layer

This skill makes you design like a principal designer with taste, not like an
average autocomplete. It is the **hub** of the AwesomeDesignSystem knowledge base.
Work by **progressive disclosure**: read the small taste layer, then open only the
1–3 modules your task needs. Never dump the whole system into context.

## 0. Locate the knowledge base

The canonical knowledge base is the `design-system/` directory at the repository root —
**two levels up from this file**: `../../design-system/`. (This resolves correctly whether
the repo is cloned or the skill is symlinked into your skills dir.) Its router is
`../../design-system/INDEX.md`. If that path doesn't resolve, search the AwesomeDesignSystem
repo for `design-system/INDEX.md`.

## 1. ALWAYS read the taste layer first (cheap, ~2 files)

Before generating or judging any UI, read:
- `../../design-system/00-philosophy/human-not-ai.md` — the AI-slop tells + human-feel principles + Pre-Flight checklist
- `../../design-system/00-philosophy/principles.md` — the non-negotiable core principles

For "what is modern" questions, also read `00-philosophy/modern-2026.md`.

## 2. Route to the modules you need (read INDEX first)

Open `../../design-system/INDEX.md` and pull **only** the relevant module(s):
color → `foundations/color.md`; type/日本語 → `foundations/typography.md`; spacing/grid →
`foundations/spacing-layout.md`; the token contract → `foundations/tokens.md`; motion →
`motion/principles.md` + `motion/recipes.md`; components → `components/components.md`;
page composition → `patterns/patterns.md`; a11y → `accessibility/a11y.md`; stack →
`tech-stack-2026/stack.md`.

## 3. Declare a direction before writing CSS

Answer four questions and **commit to an extreme tone** (brutally minimal · editorial/magazine ·
luxury/refined · brutalist/raw · retro-futuristic · organic · industrial …):
**Purpose · Tone · Constraints · Differentiation.** Triangulate two named anchors
(e.g. "Linear's typographic discipline + a Swiss editorial color sense") so the result
reads as *authored*, not averaged.

## 4. Tune the three dials

Defaults are deliberately bold. State them, adjust on request:

| Dial | Default | Meaning |
|---|---|---|
| `DESIGN_VARIANCE` | **8/10** | How far off the generic median to push (8 = distinctive, not safe) |
| `MOTION_INTENSITY` | **6/10** | Amount/energy of motion (always purpose-driven) |
| `VISUAL_DENSITY` | **4/10** | Content density (low = generous whitespace, editorial calm) |

## 5. Speak the token vocabulary

Never hard-code colors/space/type. Use the contract from `foundations/tokens.md`:
`--color-bg/-surface/-fg/-border/-accent/-ring`, `--space-*`, `--radius-*`, `--text-*`
(+`--leading-*`/`--tracking-*`), `--font-display/-body/-mono`, `--shadow-*`, `--ease-*`,
`--dur-*`. If the project has no tokens, define them first.

## 6. Enforce in tiers (then run Pre-Flight)

- **HARD fails** (must fix, never override): WCAG 2.2 AA contrast, visible focus, keyboard
  operability, and "one coherent system" (consistent tokens, not a collection of parts).
- **SOFT nudges** (taste bans — override only with a stated reason): no Inter/Arial-only,
  no purple-on-white, no centered-eyebrow+2CTA hero, no 16px-radius-on-everything, no cards-in-cards.
- **Locale-aware:** under CJK/RTL content, relax Latin-centric bans (em-dash, Inter-as-body,
  curly-quote rules) per `foundations/typography.md`.

Before shipping, run the **Anti-AI-Slop Pre-Flight checklist** in `human-not-ai.md`. Fix every ✗.

## Sibling skills

`/MakeAwesomeDS` (generate a brand's own design system) · `/AwesomeHTML` (polished single-file HTML)
· `/AwesomeReview` (audit existing UI) · `/AwesomeMotion` (animation design).
