---
name: make-awesome-ds
description: >-
  Generate a complete, brand-specific design system (tokens + DESIGN.md + previews)
  for a company, product, brand, or app, grounded in the AwesomeDesignSystem method.
  Use when the user asks to "create a design system", "define our brand's design language",
  "set up design tokens", "establish a visual identity / style guide", or wants a reusable
  token + component foundation for a new or existing product. Produces an authored,
  non-generic system with an OKLCH palette, a distinctive type pairing, and the three dials set.
---

# MakeAwesomeDS — author a brand's own design system

Turn a brand brief into a real, opinionated design system that an AI or a team can build
from. The output is **authored, not averaged**: it commits to a point of view.

## 0. Ground yourself in the method

Read the taste layer + token contract from the knowledge base (two levels up):
- `../../design-system/00-philosophy/principles.md` and `human-not-ai.md` (the standard)
- `../../design-system/foundations/tokens.md` (the token contract to instantiate)
- `../../design-system/foundations/color.md` + `typography.md` (palette & type method)
Pull others (`spacing-layout`, `motion/*`, `patterns`) only as needed.

## 1. Establish the brief (ask or infer ≤4 things)

Brand/product name · what it does · audience · the feeling it should evoke. Then **declare a
direction**: Purpose · Tone (pick an extreme) · Constraints · Differentiation, anchored to two
named references. Set the dials: `DESIGN_VARIANCE` (default 8), `MOTION_INTENSITY` (6),
`VISUAL_DENSITY` (4). Record the rationale — a system without a "why" drifts to slop.

## 2. Build the foundations

1. **Color** — choose ONE dominant brand color, derive an OKLCH tonal scale (even-L steps,
   taper chroma at the ends), add 1 sharp accent + semantic success/warning/danger. Map to the
   semantic `--color-*` contract. Provide light + dark (adjust L/C, don't invert).
2. **Type** — pick a distinctive display + a legible body (+ mono). Never default to Inter-only.
   Define the fluid `--text-*` scale and `--font-*`. Include 和文/CJK stacks if the brand needs them.
3. **Space/Radius/Shadow/Motion** — instantiate `--space-*`, `--radius-*`, `--shadow-*`,
   `--ease-*`, `--dur-*` consistent with the tone (tighter radii = sharper brand, etc.).

## 3. Emit the artifacts

Write these files (ask where; default to `./design-system/` in the user's project):

1. **`tokens.css`** — the full `:root{}` (OKLCH) + dark overrides + a Tailwind v4 `@theme{}` mirror.
2. **`DESIGN.md`** — the brand's system doc, using this 9-section schema:
   ```
   1. Brand & Direction   (purpose, tone, anchors, dials, the "why")
   2. Color               (palette, roles, light/dark, contrast notes)
   3. Typography          (fonts, scale, pairing rules, i18n)
   4. Spacing & Layout    (scale, grid, density)
   5. Motion              (principles, easing/duration, signature moment)
   6. Components           (key components, states, variants)
   7. Patterns            (hero/sections/nav — the brand's compositions)
   8. Accessibility       (contrast/focus/keyboard commitments)
   9. Distinctiveness     (anti-imitation: what makes this ONLY this brand)
   ```
3. **`preview.html`** + **`preview-dark.html`** — a single-file demo applying the tokens to a
   hero + components, so the system is verifiable at a glance (use `/AwesomeHTML` conventions).

## 4. Verify (tiered)

Run the Pre-Flight checklist from `human-not-ai.md`. HARD fails (contrast, focus, coherence)
must pass. Apply the swap-test: *could this system belong to a different brand unchanged?*
If yes, push `DESIGN_VARIANCE` up and add distinctiveness. Keep the system **coherent as a whole**.
