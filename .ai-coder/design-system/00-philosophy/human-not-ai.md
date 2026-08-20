# Human, Not AI — The Taste Layer

> **Module:** `00-philosophy/human-not-ai.md` · **Status:** Canonical (June 2026)
>
> **For AI agents:** This is the *taste layer* of the design system — the verdict you read
> BEFORE generating any frontend, and the bar you grade against AFTER. Everything here is
> distilled and upgraded from primary sources (Anthropic's frontend-aesthetics guidance,
> the `frontend-design` Skill, and Anthropic's engineering work on grading subjective
> quality) plus credible 2025–2026 design critique. Embed the values; do not fetch links.
>
> **The one-sentence verdict:** *Thoughtlessness makes slop; AI just scales it.* Supply a
> point of view and AI is a force multiplier. Supply nothing and it returns the median of
> the internet, dressed up nicely.

---

## TL;DR — what to do every time

1. **Declare a tone before writing CSS.** Pick an extreme (editorial, brutalist, luxury…).
   Name two reference anchors. Write them down. *Then* code.
2. **One dominant color + sharp accents.** Never a timid, evenly-distributed palette.
   Color is semantic (function-named tokens), never decoration.
3. **A deliberate, distinctive font.** Never Inter/Roboto/Arial/system as the *only* choice.
   No reflexive Space Grotesk or Instrument Serif either.
4. **Real hierarchy via contrast** — size, weight, color, space. One focal point per screen.
5. **Motion communicates state/attention/personality.** One orchestrated staggered page-load
   beats scattered micro-interactions. Chosen easing + duration.
6. **Design the unhappy paths** — error, empty, loading — and accessibility, every time.
7. **Theme shadcn/Tailwind defaults.** A kit is a starting point, not a finished look.
8. **Run the [Anti-AI-Slop Checklist](#the-anti-ai-slop-checklist) before shipping.** Every
   ✗ is a slop tell to fix.
9. **Use a generator/evaluator split.** Never let the generating agent grade its own work.

---

## 1. What "AI Slop" Is

**AI slop** = output that is *superficially competent but lacks substance, point of view, or
contextual fit.* The 2026 academic framing (Kommers et al., "Why Slop Matters," MINT Lab /
Indiana University, arXiv:2601.06060) names three prototypical properties:

- **Superficial competence** — looks right while lacking substance.
- **Asymmetric effort** — trivial to produce, costly to evaluate and clean up.
- **Mass producibility** — generated at scale, so it floods every surface.

In web design the defining symptom is **distributional convergence**. Ask an LLM to "build a
landing page" with no constraints and you don't get *design* — you get **the median of every
Tailwind tutorial scraped from GitHub between 2019 and 2024.** Models predict the most
probable next token, so "safe" choices that offend no one dominate. This is **not a knowledge
gap.** The model knows typography and color theory; it **defaults to the statistical center
unless explicitly steered off it.**

> The problem isn't that AI creates *bad* designs. It's that AI creates the *same* design
> everyone else gets.

**The origin story of the purple.** In August 2025 Tailwind creator Adam Wathan publicly
apologized (1M+ views) for defaulting Tailwind UI's example buttons to `bg-indigo-500` years
earlier — an arbitrary choice that cascaded through thousands of tutorials and codebases,
saturating LLM training data with purple-as-default-accent. That single default is a large
part of why AI-built sites turn purple.

---

## 2. The Hallmarks of AI Slop (the visual tells)

The acid test: **a site is sloppy when a fintech homepage could be swapped onto a CRM or a
to-do app with zero changes.** The design then communicates nothing about brand, product, or
people. Memorize these tells so you can catch yourself producing them.

| Dimension | The Tell | Why it reads as cheap |
|---|---|---|
| **Typography** | Inter / Roboto / Arial / Open Sans / system fonts; Space Grotesk as the "I tried" upgrade; lately Geist + Instrument Serif converging too | A font no one chose. "Never intentionally styled." |
| **Color** | Purple/violet → blue gradient on white; cyan-on-dark; pastel rainbow accents; timid evenly-distributed palettes | No dominant color, no point of view. Omnipresent to meaninglessness. |
| **Hero** | Centered eyebrow + 64px headline + subhead + two CTAs | The single most-copied macrostructure on the web. |
| **Layout** | Three-up feature cards w/ emoji or icon; logo soup; pricing toggle; FAQ accordion; identical nav | Predictable, interchangeable, brandless. |
| **Cards** | Thick colored border on one side of a rounded card; cards-inside-cards 3–5 levels deep, each with its own padding + shadow | The single most recognizable signature of AI-generated UI. |
| **Radius / spacing** | Uniform 16px radius on *everything*; identical padding & card heights; `rounded-xl` default buttons | Flat hierarchy — nothing is emphasized because everything is equal. |
| **Shadows** | `0.1` opacity soft drop shadows everywhere; generic glassmorphism; animated gradient blobs | Decorative default, not a considered elevation system. |
| **Motion** | Uniform fade-ins, or none; buttons that snap; scattered unmotivated micro-interactions | Motion as decoration, not communication. |
| **Imagery** | Stock photos of diverse groups at laptops in unreal lighting; AI illustrations "too smooth, too symmetrical" | Plastic; signals no real product or team. |
| **Copy** | "Build the future of work," "Scale without limits"; hedging ("may help"); generic superlatives | Generically averaged voice; no human said this. |
| **Components** | Default shadcn cards with no styling on top; Tailwind defaults untouched | Using a kit ≠ having taste. |
| **Missing** | No hierarchy beyond size; no error/empty/loading states; no a11y | Competence without substance. |

> **Default shadcn is not the enemy — *unstyled* shadcn is.** shadcn/ui is an unstyled
> primitive layer meant to be themed. Shipping its raw defaults (slate palette, default
> radius, no type choice) is the most common "kit-with-no-taste" tell. Map it onto the token
> contract (`--color-*`, `--radius-*`, `--font-*`) before you ship a single component.

---

## 3. Why Slop Happens (so you can self-correct)

1. **Convergence under ambiguity.** Vague prompts → the statistical mean. The fix is
   *direction*, not more requests.
2. **Over-specification also fails.** Filling a prompt with rigid pixel-level specs burns the
   model's capacity on conservative defaults, leaving no room for creative choice — "technically
   correct but visually dead." **Both extremes — total openness AND pixel-locking — produce
   dead output.**
3. **The sweet spot is the "right altitude": principle-based direction.** Tell the model
   *what to think about* (typography, color, motion, backgrounds), not exactly *what to
   produce.* Direct attention to specific dimensions one at a time.
4. **Taste requires lived experience.** A human develops taste by feeling the emotional impact
   of thousands of layouts; an LLM has "statistical correlations between tokens." So the taste
   must be *supplied* — via constraints, references, and tokens up front.

---

## 4. The 10 Human-Feel Principles

The 2025–2026 reorientation: design value moved **from polish to presence** — from surface
perfection toward *judgment and intent.* Briefs now literally request work that "feels human,"
a phrase nearly unheard of before 2024. What separates designers now "isn't access, it's
judgment, taste, and decision-making." Here is what that resolves to in practice.

1. **Point of view.** A clear authorial stance. The work could only have come from *this*
   brand for *this* audience. "Architecture that could come from anywhere effectively comes
   from nowhere." Triangulate between two specific anchors (e.g. *Linear's typographic
   discipline × Pitchfork's editorial color*) so the result reads as *designed*, not copied.
2. **Restraint = confidence.** White space "signals confidence — it feels editorial,
   controlled, intentional." Minimal because the idea is strong enough to stand alone, not
   because there are no ideas. Every element earns its place.
3. **Intentional imperfection.** Slight wobble, hand-drawn marks, grain/paper/noise texture,
   asymmetry, "off" spacing — read as **care**, not carelessness, when they come from
   understanding the rules well enough to bend them. "Looseness that is intentional."
4. **Real content hierarchy.** Emphasis lives in *contrast* (size, weight, color, space), not
   uniform sizing. One thing is clearly the most important on every screen.
5. **Editorial confidence.** Magazine-grade asymmetry: pull quotes, overlapping text/image,
   dramatic type hierarchy. Layout as composition, not a grid of equal boxes.
6. **Considered motion.** Animation communicates *state change, attention, or personality* —
   never decoration. One well-orchestrated page load (staggered reveals) beats scattered
   micro-interactions. Easing curves and durations are chosen, not defaulted.
7. **Craft at 800% zoom.** Optical alignment, consistent spacing rhythm, color harmony,
   contrast ratios, considered elevation. The details refined *after* the layout "works."
8. **Originality.** Evidence of deliberate custom decisions a human designer would recognize
   as choices — not a template filled in.
9. **Contextual fit.** Color is *semantic* (function, not decoration); imagery is real
   (product screenshots, actual team, custom illustration); copy sounds like a specific human
   ("Would our CEO actually say this?").
10. **Coherent whole.** Color, type, layout, imagery, and motion combine into **one distinct
    mood and identity** — "a coherent whole rather than a collection of parts." This is
    Anthropic's #1 grading criterion for design quality.

### Slop vs. human, side by side

| Decision | Slop default | Human verdict |
|---|---|---|
| Font | Inter / system, unstated | Distinctive pairing declared before CSS (`--font-display` ≠ `--font-body`) |
| Palette | Purple→blue gradient on white, even spread | One dominant `--color-accent` + semantic `--color-success/warning/danger` |
| Hero | Centered eyebrow + 64px + 2 CTAs | Asymmetric, single focal point, real copy |
| Emphasis | Everything 16px radius, equal weight | 3×+ size jumps, weight extremes, deliberate `--radius-*` scale |
| Cards | Cards-in-cards, one-sided color border | 1px `--color-border` hairline + layered `--shadow-*` |
| Motion | Uniform fade-in or snap | Staggered load with `--ease-spring` + chosen `--dur-*` |
| States | Happy path only | Error / empty / loading all designed |
| Copy | "Scale without limits" | Specific, opinionated, no hedging |

---

## 5. Anthropic's Four Grading Criteria + the Generator/Evaluator Split

From Anthropic's engineering work on grading subjective quality. The reframe that makes taste
*gradable*: stop asking "Is this beautiful?" and start asking **"Does this follow our
principles for good design?"**

| # | Criterion | The grading question |
|---|---|---|
| 1 | **Design quality** | Does the design feel like a *coherent whole* rather than a collection of parts? Do color, type, layout, and imagery combine into one distinct mood/identity? |
| 2 | **Originality** | Are there custom decisions over templates? Would a human designer recognize deliberate creative choices? |
| 3 | **Craft** | Is the technical execution clean — type hierarchy, spacing consistency, color harmony, contrast ratios, elevation? |
| 4 | **Functionality** | Is it usable independent of aesthetics — states, affordances, accessibility? |

### The generator/evaluator split (GAN-inspired)

**Never let the generating agent grade its own work.** Agents "tend to confidently praise the
work even when the quality is obviously mediocre." Two separate roles drive the generator
toward stronger output:

- **Generator** — produces the frontend against the tone brief + token contract.
- **Evaluator** — a *fresh context* that scores against the four criteria and the
  [checklist](#the-anti-ai-slop-checklist), returns ✗-list, and **does not** generate.

```ts
// Pseudocode for the loop. The evaluator never sees the generator's self-assessment.
type Score = { quality: number; originality: number; craft: number; functionality: number };

async function designLoop(brief: ToneBrief, maxRounds = 3) {
  let artifact = await generate(brief);                 // role: generator
  for (let round = 0; round < maxRounds; round++) {
    const review = await evaluate(artifact, CRITERIA);  // role: evaluator (fresh context)
    if (review.min >= 8 && review.slopTells.length === 0) return artifact;
    artifact = await generate(brief, review.slopTells); // regenerate against the ✗-list
  }
  return artifact; // escalate to a human if still failing
}
```

> The score gate (every criterion ≥ 8/10 *and* zero slop tells) is deliberately strict.
> "Good enough" is how the median creeps back in.

---

## The Anti-AI-Slop Checklist

Run this before shipping any frontend. Each ✗ is a slop tell to fix. Keep it copy-paste
runnable — paste into a PR description, an issue, or the evaluator's prompt.

```text
TYPOGRAPHY
[ ] No Inter / Roboto / Arial / Open Sans / Lato / system-default font
[ ] No reflexive Space Grotesk OR Instrument Serif as the only "choice"
[ ] A deliberate font choice stated BEFORE writing CSS
[ ] High-contrast pairing (display + mono, or serif + geometric sans)
[ ] Weight extremes used (100/200 vs 800/900 — not 400 vs 600)
[ ] Size jumps of 3x+, not 1.5x

COLOR
[ ] No purple/violet gradient on white; no default cyan-on-dark
[ ] One dominant color + sharp accents (NOT a timid even palette)
[ ] Color is semantic (function-named CSS vars), not decorative
[ ] Committed to ONE cohesive aesthetic (named tone)

LAYOUT
[ ] Hero is not the default centered-eyebrow-headline-subhead-2CTA
[ ] Not three identical feature cards with emoji/icons
[ ] Radius/padding/shadow are an intentional SYSTEM, not 16px-on-everything
[ ] No cards-in-cards nesting; no one-sided thick colored border
[ ] Real visual hierarchy — one clear focal point per screen

MOTION
[ ] Motion communicates state/attention/personality, not decoration
[ ] Chosen easing + duration (not snap; not uniform fade)
[ ] One orchestrated page-load moment > scattered micro-interactions

SUBSTANCE
[ ] Error / empty / loading states designed
[ ] Accessibility (contrast, focus, semantics) handled
[ ] Copy has a specific voice; no hedging or generic superlatives
[ ] Real imagery (screenshots/team/custom), not generic stock/AI art
[ ] Default shadcn/Tailwind has been THEMED, not shipped raw

WHOLE
[ ] Could ONLY be this brand for this audience (swap-test fails for others)
[ ] Reads as a coherent whole, not a collection of parts
```

### Optional: a CI guard for the cheapest tells

Static checks can't grade taste, but they can fail the build on the most mechanical slop. Wire
this into a pre-commit hook or CI step; treat it as a smoke alarm, not a judge.

```js
// scripts/slop-guard.mjs — fails CI on the cheapest, most mechanical AI-slop tells.
import { readFileSync } from "node:fs";
import { globSync } from "node:fs";

const FILES = globSync("src/**/*.{css,tsx,jsx}");
const TELLS = [
  { re: /font-family:[^;]*\b(Inter|Roboto|Arial|Open Sans)\b/i, msg: "banned default font" },
  { re: /\bfrom-(purple|violet|indigo)-\d+\s+to-(blue|cyan)-\d+/, msg: "purple→blue gradient" },
  { re: /rgba?\([^)]*,\s*0?\.1\s*\)/, msg: "0.1-opacity default shadow" },
  { re: /\bbg-(indigo|violet|purple)-(400|500|600)\b/, msg: "Tailwind default purple accent" },
];

let failed = 0;
for (const file of FILES) {
  const text = readFileSync(file, "utf8");
  for (const { re, msg } of TELLS) {
    if (re.test(text)) { console.error(`✗ ${file}: ${msg}`); failed++; }
  }
}
if (failed) { console.error(`\n${failed} slop tell(s). Fix or justify in PR.`); process.exit(1); }
console.log("✓ no mechanical slop tells");
```

---

## 6. The "Right Altitude" Method (the prompt-side fix)

The repeatable loop from Anthropic's guidance:
**(1) identify convergent defaults → (2) provide concrete alternatives → (3) structure
guidance at the right altitude → (4) make it reusable via a Skill.**

Three strategies, applied to each dimension *individually* — not all at once:

- **Guide specific design dimensions** — typography, color, motion, backgrounds, one at a time.
- **Reference design inspirations** — IDE themes, cultural aesthetics, two named anchors.
- **Call out common defaults explicitly** — "avoid Inter," "no purple-on-white."

> **Altitude, defined.** Too low = pixel-locked specs that burn the model's creative budget on
> conservative defaults. Too high = "build a landing page," which returns the median. The right
> altitude tells the model *what to think about*, not *what to produce.*

### Step 0 — commit to a direction before any CSS

Answer four questions and **declare a tone.** Push the tone toward an extreme; the middle is
where the median lives.

| Question | Why it kills slop |
|---|---|
| **Purpose** — what is this for, who reads it? | Anchors every later choice to context. |
| **Tone** — pick an extreme (below) | A named extreme repels the statistical center. |
| **Constraints** — brand, perf, a11y, platform | Real limits force real decisions. |
| **Differentiation** — two named anchors to triangulate | Triangulation = a point of view, not a copy. |

**Tone extremes to choose from:**

> brutally minimal · maximalist chaos · retro-futuristic · organic/natural · luxury/refined ·
> playful/toy-like · editorial/magazine · brutalist/raw · art deco/geometric · soft/pastel ·
> industrial/utilitarian

### Anthropic's distilled aesthetics prompt (copy-paste, agent-ready)

```text
<frontend_aesthetics>
You tend to converge toward generic, "on distribution" outputs. In frontend
design, this creates what users call the "AI slop" aesthetic. Avoid this: make
creative, distinctive frontends that surprise and delight. Focus on:

Typography: Choose fonts that are beautiful, unique, and interesting. Avoid
generic fonts like Arial and Inter; opt instead for distinctive choices that
elevate the frontend's aesthetics.

Color & Theme: Commit to a cohesive aesthetic. Use CSS variables for
consistency. Dominant colors with sharp accents outperform timid,
evenly-distributed palettes. Draw from IDE themes and cultural aesthetics.

Motion: Use animations for effects and micro-interactions. Prioritize CSS-only
solutions for HTML. Use the Motion library for React when available. Focus on
high-impact moments: one well-orchestrated page load with staggered reveals
(animation-delay) creates more delight than scattered micro-interactions.

Backgrounds: Create atmosphere and depth rather than defaulting to solid colors.
Layer CSS gradients, use geometric patterns, or add contextual effects that
match the overall aesthetic.

Avoid generic AI-generated aesthetics:
- Overused font families (Inter, Roboto, Arial, system fonts)
- Clichéd color schemes (particularly purple gradients on white backgrounds)
- Predictable layouts and component patterns
- Cookie-cutter design that lacks context-specific character

Interpret creatively and make unexpected choices that feel genuinely designed
for the context. Vary between light and dark themes, different fonts, different
aesthetics. You still tend to converge on common choices (Space Grotesk, for
example) across generations. Avoid this: it is critical that you think outside
the box!
</frontend_aesthetics>
```

### Distinctive font choices by context (instead of Inter)

Map the winner onto `--font-display` / `--font-body` / `--font-mono`. Pick a *high-contrast
pairing*, not two neighbors on the same axis.

| Context | Pick from |
|---|---|
| Code / technical aesthetic | JetBrains Mono, Fira Code |
| Editorial | Playfair Display, Crimson Pro, Fraunces, Newsreader |
| Startup / display | Clash Display, Satoshi, Cabinet Grotesk |
| Technical systems | IBM Plex family, Source Sans 3 |
| Distinctive / character | Bricolage Grotesque, Obviously |

---

## 7. Anchored to the Token Contract

Philosophy is cheap until it touches tokens. Here is the taste layer expressed as the system's
canonical variables — *one dominant accent, semantic color, layered hairline elevation, chosen
motion.* This is the shape every module inherits.

```css
/* The taste layer, made concrete. Light theme; dark overrides L & C, never naive-inverts. */
:root {
  /* One dominant accent + semantic colors (OKLCH). NOT an even rainbow. */
  --color-accent: oklch(0.62 0.19 28);        /* a committed, non-purple hue */
  --color-accent-fg: oklch(0.98 0.01 28);
  --color-accent-hover: oklch(0.56 0.20 28);
  --color-success: oklch(0.70 0.15 150);
  --color-warning: oklch(0.80 0.14 80);
  --color-danger: oklch(0.58 0.20 25);
  --color-ring: var(--color-accent);

  /* Hierarchy via contrast: 3x+ size jumps, weight extremes. */
  --text-7xl: clamp(3.5rem, 2rem + 7.5vw, 6rem);
  --text-base: clamp(1rem, 0.95rem + 0.25vw, 1.125rem);
  --tracking-tight: -0.02em;

  /* Elevation: 1px hairline + layered low-opacity shadow, NOT a 0.1 blob. */
  --color-border: oklch(0.90 0.005 270);
  --shadow-md: 0 1px 2px oklch(0 0 0 / 0.04), 0 4px 12px oklch(0 0 0 / 0.06);

  /* Motion: chosen easing + duration. Spring is a linear() approximation. */
  --ease-spring: linear(0, 0.06, 0.23, 0.48, 0.73, 0.91, 1.02, 1.04, 1);
  --dur-base: 250ms;
}

[data-theme="dark"] {
  --color-accent: oklch(0.68 0.17 28);   /* lift L, ease C for dark surfaces */
  --color-border: oklch(0.28 0.01 270);
}
```

```css
/* One orchestrated, staggered page-load > scattered micro-interactions. */
@media (prefers-reduced-motion: no-preference) {
  .stagger > * {
    animation: rise var(--dur-base) var(--ease-spring) backwards;
  }
  .stagger > :nth-child(1) { animation-delay: 0ms; }
  .stagger > :nth-child(2) { animation-delay: 60ms; }
  .stagger > :nth-child(3) { animation-delay: 120ms; }
}
@keyframes rise { from { opacity: 0; transform: translateY(8px); } }
```

The same contract in Tailwind v4, mirrored so utilities and CSS vars never drift:

```css
/* app.css — Tailwind v4 @theme mirrors the CSS vars one-to-one. */
@import "tailwindcss";
@theme {
  --color-accent: oklch(0.62 0.19 28);
  --color-accent-fg: oklch(0.98 0.01 28);
  --color-success: oklch(0.70 0.15 150);
  --color-danger: oklch(0.58 0.20 25);
  --font-display: "Clash Display", ui-sans-serif, sans-serif; /* declared, not Inter */
  --font-mono: "JetBrains Mono", ui-monospace, monospace;
  --radius-lg: 0.75rem;
  --ease-spring: linear(0, 0.06, 0.23, 0.48, 0.73, 0.91, 1.02, 1.04, 1);
}
```

A React 19 + Motion reference for the orchestrated load — alongside the CSS, never instead of it:

```tsx
"use client";
import { motion } from "motion/react";

const container = { hidden: {}, show: { transition: { staggerChildren: 0.06 } } };
const item = {
  hidden: { opacity: 0, y: 8 },
  // duration + easing are CHOSEN, mirroring --dur-base / --ease-spring
  show: { opacity: 1, y: 0, transition: { duration: 0.25, ease: [0.22, 1, 0.36, 1] } },
};

export function PageLoad({ children }: { children: React.ReactNode[] }) {
  return (
    <motion.div variants={container} initial="hidden" animate="show">
      {children.map((child, i) => (
        <motion.div key={i} variants={item}>{child}</motion.div>
      ))}
    </motion.div>
  );
}
```

---

## 8. Quotable Lines (reason with these)

- "You're not getting design. You're getting the **median.**"
- "The problem isn't *bad* designs. It's the *same* design everyone else gets."
- "**Thoughtlessness** makes slop; AI just **scales** it."
- "**Tell the model what to think about, not what to produce.**"
- "**Dominant colors with sharp accents** outperform timid, evenly-distributed palettes."
- "One **well-orchestrated page load** beats scattered micro-interactions."
- "Restraint isn't a lack of ideas — it's an idea **strong enough to stand alone.**"
- "Architecture that could come from anywhere **comes from nowhere.**"
- "Coherent **whole**, not a collection of parts."
- "The choice: be an **author or just an operator.**"
- "Using a kit ≠ having taste. shadcn is a starting point, not a finished look."

---

## 9. Caveats (intellectual honesty)

- Conversion/engagement stats from marketing blogs ("91% lower conversion," "human copy wins
  94.12% of the time") are **uncited marketing claims** — directional, not fact.
- "AI can't do detail at 800% zoom" is a designer's *opinion/prediction*, not settled fact;
  model image fidelity is improving fast.
- The point is **not** that AI inherently makes slop. Thoughtlessness does. AI used by someone
  with a point of view is a force multiplier; used with nothing supplied, it returns nothing —
  "dressed up nicely."
- Even purpose-built anti-slop tools fall into the trap: critics noted Claude Design's own
  marketing visuals drifting toward Inter/purple/three-cards — **the gravity toward the median
  is real, and constant.** Re-run the checklist even on work that was supposed to be safe.

---

## Sources

- Prompting for frontend aesthetics — Claude Cookbook — https://platform.claude.com/cookbook/coding-prompting-for-frontend-aesthetics
- Improving frontend design through Skills — Claude — https://claude.com/blog/improving-frontend-design-through-skills
- Harness design for long-running application development — Anthropic Engineering — https://www.anthropic.com/engineering/harness-design-long-running-apps
- Frontend Design plugin — Anthropic — https://claude.com/plugins/frontend-design
- Why Your AI Keeps Building the Same Purple Gradient Website — prg.sh — https://prg.sh/ramblings/Why-Your-AI-Keeps-Building-the-Same-Purple-Gradient-Website
- AI Slop Web Design: Complete Guide to Spotting and Fixing Generic Websites (2026) — 925studios — https://www.925studios.co/blog/ai-slop-web-design-guide
- Claude Design produces AI slop unless you tell it not to — The Adpharm — https://www.theadpharm.com/insights/claude-design-without-the-ai-slop-look
- Design Trends 2026: Imperfection, Rebellion, and the Return of Human Work — Lindsay Marsh — https://lindsaymarsh.substack.com/p/design-trends-2026-imperfection-rebellion
- The Design Shifts That Will Matter in 2026 — KOVA Collective — https://www.kovacollective.co/post/the-design-shifts-that-will-matter-in-2026
- AI slop — Wikipedia — https://en.wikipedia.org/wiki/AI_slop
- Why Slop Matters — Kommers et al. (MINT Lab, Indiana University), arXiv:2601.06060 — https://arxiv.org/abs/2601.06060
- Hallmark: Stop AI-Generated UI Slop in One Command — DEV Community — https://dev.to/rams901/hallmark-stop-ai-generated-ui-slop-in-one-command-in-2026-3p9n
- Teaching Claude to Design Better — Justin Wetch — https://www.justinwetch.com/blog/improvingclaudefrontend
