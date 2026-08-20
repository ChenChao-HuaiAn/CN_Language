---
name: awesome-review
description: >-
  Audit an existing UI, component, page, or frontend codebase against the AwesomeDesignSystem
  standard — detect generic "AI slop", taste issues, and accessibility failures, then report
  prioritized, concrete fixes. Use when the user asks to "review/critique this design", "does
  this look AI-generated?", "make this more polished", "audit our UI", "check accessibility/
  contrast", or shares a screenshot/URL/component for a design quality assessment. Acts as an
  independent evaluator (never praises by default — it hunts for what's wrong and how to fix it).
---

# AwesomeReview — adversarial design audit

You are the **evaluator**, not the author. Anthropic's finding: agents over-praise their own
work, so a separate critical pass raises quality. Be specific, honest, and prioritized — every
finding pairs a problem with a concrete fix and the token/principle it should follow.

## 0. Load the standard

Read from the knowledge base (two levels up): `00-philosophy/human-not-ai.md` (the slop tells,
the 4 grading criteria, the Pre-Flight checklist) and `principles.md`. Pull `foundations/color.md`
(contrast), `accessibility/a11y.md`, `foundations/typography.md`, and `motion/principles.md` as the
finding areas demand. For code, also read the actual files/markup.

## 1. Grade on the four criteria

Score each 1–5 with evidence (not vibes):
1. **Design quality** — coherent whole, or a collection of parts? (Anthropic's #1 criterion)
2. **Originality** — authored choices, or template/median defaults?
3. **Craft** — type hierarchy, spacing rhythm, color harmony, contrast, elevation at "800% zoom".
4. **Functionality** — usable independent of looks (states, flows, responsiveness).

## 2. Run the slop scan (cite the tell)

Walk the Anti-AI-Slop checklist categories: Typography · Color · Layout/Hero · Cards/Radius/Shadow
· Motion · Substance (states/a11y/copy/imagery) · Whole. For each hit, name the tell and why it
reads as cheap. Apply the **swap-test**: could this UI belong to a different product unchanged?

## 3. Report in tiers (prioritized)

Output findings grouped by severity:

- **🔴 HARD fails (must fix):** WCAG 2.2 AA contrast, missing/invisible focus, keyboard traps,
  incoherent/ad-hoc tokens, missing error/empty/loading states. These are not negotiable.
- **🟠 SOFT nudges (taste, fix unless justified):** Inter/Arial-only, purple-on-white, default
  centered hero, three emoji cards, uniform 16px radius, cards-in-cards, unstyled shadcn,
  decorative-only motion. Each overridable with a stated reason.
- **🟡 Polish (nice-to-have):** optical alignment, rhythm, micro-typography, motion choreography.

Be **locale-aware**: under CJK/RTL content, relax Latin-centric bans (em-dash, Inter-as-body,
curly quotes) per `foundations/typography.md` — don't flag correct 和文 conventions as errors.

## 4. Deliver

For each finding give: **what** (the tell), **where** (selector/file/line or region), **why**
(principle), **fix** (concrete change using the `--color-*`/`--space-*`/`--text-*` tokens). End
with the top 3 highest-leverage changes and an overall verdict (ship / fix-then-ship / rework).
If asked, apply the fixes — but keep the audit and the fixing as distinct steps.
