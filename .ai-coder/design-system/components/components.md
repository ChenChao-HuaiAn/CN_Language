---
title: "Core Components — Anatomy, States & Accessibility (2026)"
description: "The canonical, copy-paste verdict for core UI components. Framework-agnostic HTML/CSS + React 19 / Tailwind v4 (shadcn-style, THEMED). Every state, every a11y contract, built on CONTRACT tokens."
updated: 2026-06-09
tags: [components, button, input, dialog, tabs, tooltip, toast, accessibility, react-19, tailwind-v4]
---

# Core Components — Anatomy, States & Accessibility

> The verdict layer. Each component below ships its **anatomy**, **all interactive states**
> (default · hover · active · focus-visible · disabled · loading · error), and its **accessibility
> contract** — then the code. Framework-agnostic HTML/CSS **first**, React 19 + Tailwind v4
> (Base UI / Radix primitives, themed) alongside. Everything consumes the
> [CONTRACT tokens](../foundations/tokens.md): `--color-*`, `--space-*`, `--radius-*`, `--text-*`,
> `--shadow-*`, `--dur-*`, `--ease-*`. Never hardcode a hex, px, or ms.

A component is **not done** when it looks right — it is done when its states and a11y contract are
specified. Pinterest Gestalt's bar: visually accessible (4.5:1 text / 3:1 UI), screen-reader
compatible (role + name + ARIA), navigable (full keyboard, visible focus), understandable
(predictable, clear errors). Build on native semantics (`<button>`, `<label>`); ARIA only fills gaps.

---

## 0. The shared rules (apply to every component)

| Rule | Why | Token / mechanism |
|---|---|---|
| **Focus ring is non-negotiable** | WCAG 2.4.7 + 1.4.11 (3:1 non-text) | `:focus-visible { outline: 2px solid var(--color-ring); outline-offset: 2px }` |
| **44px min target** for primary controls | WCAG 2.5.5/2.5.8, Apple HIG | `min-block-size` on buttons/inputs |
| **Depth = hairline + surface ladder**, not heavy shadows | Linear/Raycast/Vercel taste | `1px solid var(--color-border)` over `--shadow-*` |
| **Motion communicates state**, ≤ 250ms, transform/opacity only | Stripe rule; GPU-cheap | `--dur-fast` + `--ease-out`; honor `prefers-reduced-motion` |
| **Never signal by color alone** | WCAG 1.4.1 | pair color with icon + text |
| **`disabled` ≠ removed** | AT must still announce | use `disabled` / `aria-disabled`, not `display:none` |

```css
/* Drop this once, globally. Every interactive component inherits the ring + reduced-motion. */
:where(button, a, input, select, textarea, [tabindex]):focus-visible {
  outline: 2px solid var(--color-ring);
  outline-offset: 2px;
  border-radius: inherit;
}
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after { animation-duration: 0.01ms !important; transition-duration: 0.01ms !important; }
}
```

**React 19 + Tailwind v4 setup** — wire CONTRACT tokens into the theme once, then components read
utilities *and* the raw vars. `@theme inline` keeps tokens out of the cascade where utilities should
not re-emit them; the color vars stay in plain `:root` / `.dark` so dark mode cascades.

```css
/* app.css */
@import "tailwindcss";
@theme inline {
  /* Bridge the CONTRACT tokens (declared in :root/.dark, outside @theme) into Tailwind
     utilities. `inline` emits the var() reference into utilities, so it resolves to the
     cascading runtime value (and dark mode just works) — this self-map is intentional. */
  --color-bg: var(--color-bg);             /* re-expose CONTRACT vars as Tailwind utilities */
  --color-surface: var(--color-surface);
  --color-fg: var(--color-fg);
  --color-accent: var(--color-accent);
  --color-ring: var(--color-ring);
  --radius-md: var(--radius-md);
  --ease-out: var(--ease-out);
}
```

```ts
// lib/cn.ts — the one helper every shadcn-style component uses
import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";
export const cn = (...inputs: ClassValue[]) => twMerge(clsx(inputs));
```

---

## 1. Button

The most-used interactive primitive. One dominant variant (`accent`), sharp secondary/ghost, and a
semantic `danger`. Restraint over a rainbow of variants.

**Anatomy:** `[ optional leading icon ] · label · [ optional trailing icon ]` inside a native
`<button>`. Loading swaps a spinner in for the leading slot; label stays to preserve width.

**States & contract**

| State | Visual | Implementation |
|---|---|---|
| default | accent fill, `--color-accent-fg` text | `background: var(--color-accent)` |
| hover | darken to `--color-accent-hover` | `:hover` |
| active | scale 0.98, no color shift | `:active { transform: scale(.98) }` |
| focus-visible | 2px `--color-ring`, 2px offset | global `:focus-visible` |
| disabled | 50% opacity, `not-allowed` | `disabled` attr + `aria-disabled` implicit |
| loading | spinner + `aria-busy`, click blocked | `disabled={loading}` `aria-busy` |
| a11y | native `<button type>`, label or `aria-label` for icon-only; 44px target | — |

### Framework-agnostic HTML/CSS

```html
<button class="btn btn--accent" type="button">Save changes</button>
<button class="btn btn--accent" type="button" aria-busy="true" disabled>
  <span class="btn__spinner" aria-hidden="true"></span> Saving…
</button>
<button class="btn btn--ghost btn--icon" type="button" aria-label="Delete">
  <svg aria-hidden="true" width="16" height="16"><!-- icon --></svg>
</button>
```

```css
.btn {
  --btn-pad-i: var(--space-4);
  display: inline-flex; align-items: center; justify-content: center; gap: var(--space-2);
  min-block-size: 2.5rem;                 /* md; --sm 2rem / --lg 3rem below */
  padding-inline: var(--btn-pad-i);
  border: 1px solid transparent;
  border-radius: var(--radius-md);
  font: 500 var(--text-sm) / var(--leading-sm) var(--font-body);
  letter-spacing: var(--tracking-sm);
  cursor: pointer; user-select: none; white-space: nowrap;
  transition: background var(--dur-fast) var(--ease-out),
              border-color var(--dur-fast) var(--ease-out),
              transform var(--dur-fast) var(--ease-out);
}
.btn:active { transform: scale(.98); }
.btn:disabled, .btn[aria-busy="true"] { opacity: .5; cursor: not-allowed; transform: none; }

/* variants — one dominant accent, sharp neutrals */
.btn--accent    { background: var(--color-accent); color: var(--color-accent-fg); }
.btn--accent:hover:not(:disabled) { background: var(--color-accent-hover); }
.btn--secondary { background: var(--color-surface); color: var(--color-fg); border-color: var(--color-border); }
.btn--secondary:hover:not(:disabled) { background: var(--color-surface-2); }
.btn--ghost     { background: transparent; color: var(--color-fg); }
.btn--ghost:hover:not(:disabled) { background: var(--color-surface-2); }
.btn--danger    { background: var(--color-danger); color: var(--color-danger-fg); }
.btn--danger:hover:not(:disabled) { filter: brightness(1.08); }

/* sizes */
.btn--sm { min-block-size: 2rem;   padding-inline: var(--space-3); font-size: var(--text-xs); }
.btn--lg { min-block-size: 3rem;   padding-inline: var(--space-6); font-size: var(--text-base); }
.btn--icon { padding-inline: 0; inline-size: 2.5rem; }

.btn__spinner {
  inline-size: 1em; block-size: 1em; border-radius: var(--radius-full);
  border: 2px solid currentColor; border-top-color: transparent;
  animation: btn-spin .6s linear infinite;
}
@keyframes btn-spin { to { transform: rotate(360deg); } }
@media (prefers-reduced-motion: reduce) { .btn__spinner { animation-duration: 1.2s; } }
```

### React 19 + Tailwind v4 (shadcn-style, themed)

```tsx
// ui/button.tsx
"use client";
import * as React from "react";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "@/lib/cn";

const button = cva(
  "inline-flex items-center justify-center gap-2 rounded-[var(--radius-md)] font-medium " +
    "whitespace-nowrap select-none transition-[background-color,border-color,transform] " +
    "duration-[var(--dur-fast)] ease-[var(--ease-out)] active:scale-[.98] " +
    "focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-[var(--color-ring)] " +
    "disabled:opacity-50 disabled:pointer-events-none aria-busy:opacity-50 aria-busy:pointer-events-none",
  {
    variants: {
      variant: {
        accent: "bg-[var(--color-accent)] text-[var(--color-accent-fg)] hover:bg-[var(--color-accent-hover)]",
        secondary: "bg-[var(--color-surface)] text-[var(--color-fg)] border border-[var(--color-border)] hover:bg-[var(--color-surface-2)]",
        ghost: "text-[var(--color-fg)] hover:bg-[var(--color-surface-2)]",
        danger: "bg-[var(--color-danger)] text-[var(--color-danger-fg)] hover:brightness-110",
      },
      size: { sm: "h-8 px-3 text-xs", md: "h-10 px-4 text-sm", lg: "h-12 px-6 text-base", icon: "h-10 w-10" },
    },
    defaultVariants: { variant: "accent", size: "md" },
  }
);

type ButtonProps = React.ComponentProps<"button"> &
  VariantProps<typeof button> & { loading?: boolean };

export function Button({ className, variant, size, loading, disabled, children, ...props }: ButtonProps) {
  return (
    <button
      className={cn(button({ variant, size }), className)}
      disabled={disabled || loading}
      aria-busy={loading || undefined}   // ref is a normal prop in React 19 — no forwardRef
      {...props}
    >
      {loading && <Spinner />}
      {children}
    </button>
  );
}

function Spinner() {
  return (
    <span
      aria-hidden="true"
      className="size-[1em] animate-spin rounded-full border-2 border-current border-t-transparent motion-reduce:[animation-duration:1.2s]"
    />
  );
}
```

---

## 2. Card

A surface container. **Flat, hairline-bordered, no card-in-card.** Depth comes from the surface
ladder (`--color-surface` on `--color-bg`) plus a 1px border — not a stack of shadows.

**Anatomy:** `Card > [ Card.Header (title + description) ] · Card.Body · [ Card.Footer (actions) ]`.
Interactive (clickable) cards add hover lift + a focus ring on the wrapping link/button.

**States:** static cards have no states. Interactive cards: hover (border brightens, optional 1px
`--shadow-sm`), focus-visible (ring on the anchor), active (scale 0.99). **A11y:** if the whole card
is a link, wrap content in a single `<a>`; don't nest interactive controls inside a clickable card.

### Framework-agnostic HTML/CSS

```html
<article class="card">
  <header class="card__header">
    <h3 class="card__title">Monthly report</h3>
    <p class="card__desc">Usage across all workspaces.</p>
  </header>
  <div class="card__body">…</div>
  <footer class="card__footer">
    <button class="btn btn--secondary btn--sm" type="button">Export</button>
  </footer>
</article>
```

```css
.card {
  display: flex; flex-direction: column; gap: var(--space-4);
  padding: var(--space-6);
  background: var(--color-surface);
  border: 1px solid var(--color-border);     /* THE depth primitive */
  border-radius: var(--radius-xl);           /* intentional: larger than buttons */
  box-shadow: var(--shadow-sm);
}
.card__header { display: flex; flex-direction: column; gap: var(--space-1); }
.card__title { font: 600 var(--text-lg)/var(--leading-lg) var(--font-display); color: var(--color-fg); }
.card__desc  { font: 400 var(--text-sm)/var(--leading-sm) var(--font-body); color: var(--color-fg-muted); }
.card__footer { display: flex; gap: var(--space-2); margin-block-start: auto; }

/* interactive variant */
a.card, .card--interactive {
  text-decoration: none;
  transition: border-color var(--dur-fast) var(--ease-out), transform var(--dur-fast) var(--ease-out);
}
a.card:hover, .card--interactive:hover { border-color: var(--color-fg-subtle); transform: translateY(-2px); }
a.card:active, .card--interactive:active { transform: translateY(0) scale(.99); }
```

### React 19 + Tailwind v4

```tsx
// ui/card.tsx
import * as React from "react";
import { cn } from "@/lib/cn";

const surface =
  "flex flex-col gap-4 rounded-[var(--radius-xl)] border border-[var(--color-border)] " +
  "bg-[var(--color-surface)] p-6 shadow-[var(--shadow-sm)]";

export function Card({ className, ...p }: React.ComponentProps<"div">) {
  return <div className={cn(surface, className)} {...p} />;
}
export function CardHeader({ className, ...p }: React.ComponentProps<"div">) {
  return <div className={cn("flex flex-col gap-1", className)} {...p} />;
}
export function CardTitle({ className, ...p }: React.ComponentProps<"h3">) {
  return <h3 className={cn("font-[var(--font-display)] text-lg font-semibold text-[var(--color-fg)]", className)} {...p} />;
}
export function CardDescription({ className, ...p }: React.ComponentProps<"p">) {
  return <p className={cn("text-sm text-[var(--color-fg-muted)]", className)} {...p} />;
}
export function CardFooter({ className, ...p }: React.ComponentProps<"div">) {
  return <div className={cn("mt-auto flex gap-2", className)} {...p} />;
}
```

---

## 3. Input + Field (label / help / error)

The accessible-form contract. **Field** owns the wiring (label `for`, help/error `aria-describedby`,
`aria-invalid`); **Input** is the bare control. Never ship a placeholder-as-label.

**Anatomy:** `Field > Label · [ Input | Textarea | Select ] · [ Help text ] · [ Error (role=alert) ]`.

**States & contract**

| State | Visual | Implementation |
|---|---|---|
| default | surface bg, hairline border | `border: 1px solid var(--color-border)` |
| hover | border → `--color-fg-subtle` | `:hover` |
| focus-visible | ring + accent border | global ring + `border-color: var(--color-accent)` |
| disabled | `--color-bg-subtle`, muted text | `disabled` attr |
| error | `--color-danger` border + message wired via `aria-describedby` | `aria-invalid="true"` |
| a11y | every input has a `<label for>`; error `role="alert"`; required marked in text not color | — |

### Framework-agnostic HTML/CSS

```html
<div class="field">
  <label class="field__label" for="email">Email <span class="field__req">(required)</span></label>
  <input class="input" id="email" name="email" type="email" required
         aria-describedby="email-help email-error" aria-invalid="true" />
  <p class="field__help" id="email-help">We'll only use this to send receipts.</p>
  <p class="field__error" id="email-error" role="alert">
    <svg aria-hidden="true" width="14" height="14"><!-- alert icon --></svg>
    Enter a valid email, e.g. name@example.com
  </p>
</div>
```

```css
.field { display: flex; flex-direction: column; gap: var(--space-2); }
.field__label { font: 500 var(--text-sm)/var(--leading-sm) var(--font-body); color: var(--color-fg); }
.field__req   { color: var(--color-fg-muted); font-weight: 400; }
.field__help  { font-size: var(--text-xs); color: var(--color-fg-muted); }
.field__error {
  display: inline-flex; align-items: center; gap: var(--space-1);
  font-size: var(--text-xs); color: var(--color-danger);   /* icon + text, never color alone */
}

.input {
  min-block-size: 2.5rem;
  padding-inline: var(--space-3);
  background: var(--color-surface);
  color: var(--color-fg);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-md);
  font: 400 var(--text-sm)/var(--leading-sm) var(--font-body);
  transition: border-color var(--dur-fast) var(--ease-out);
}
.input::placeholder { color: var(--color-fg-subtle); }
.input:hover:not(:disabled) { border-color: var(--color-fg-subtle); }
.input:focus-visible { border-color: var(--color-accent); outline: 2px solid var(--color-ring); outline-offset: 1px; }
.input:disabled { background: var(--color-bg-subtle); color: var(--color-fg-subtle); cursor: not-allowed; }
.input[aria-invalid="true"] { border-color: var(--color-danger); }
.input[aria-invalid="true"]:focus-visible { outline-color: var(--color-danger); }
```

### React 19 + Tailwind v4 (auto-wired Field)

```tsx
// ui/field.tsx
"use client";
import * as React from "react";
import { cn } from "@/lib/cn";

const FieldCtx = React.createContext<{ id: string; errorId: string; helpId: string; invalid: boolean }>(
  { id: "", errorId: "", helpId: "", invalid: false }
);

export function Field({ children, invalid = false }: { children: React.ReactNode; invalid?: boolean }) {
  const id = React.useId();
  const value = { id, errorId: `${id}-error`, helpId: `${id}-help`, invalid };
  return <FieldCtx value={value}><div className="flex flex-col gap-2">{children}</div></FieldCtx>;
}

export function Label({ className, ...p }: React.ComponentProps<"label">) {
  const { id } = React.use(FieldCtx);
  return <label htmlFor={id} className={cn("text-sm font-medium text-[var(--color-fg)]", className)} {...p} />;
}

export function Input({ className, ...p }: React.ComponentProps<"input">) {
  const { id, errorId, helpId, invalid } = React.use(FieldCtx);
  return (
    <input
      id={id}
      aria-invalid={invalid || undefined}
      aria-describedby={cn(helpId, invalid && errorId)}
      className={cn(
        "h-10 rounded-[var(--radius-md)] border border-[var(--color-border)] bg-[var(--color-surface)] px-3 text-sm text-[var(--color-fg)]",
        "placeholder:text-[var(--color-fg-subtle)] transition-colors duration-[var(--dur-fast)]",
        "hover:enabled:border-[var(--color-fg-subtle)]",
        "focus-visible:border-[var(--color-accent)] focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-[var(--color-ring)]",
        "disabled:bg-[var(--color-bg-subtle)] disabled:text-[var(--color-fg-subtle)] disabled:cursor-not-allowed",
        "aria-[invalid=true]:border-[var(--color-danger)] aria-[invalid=true]:focus-visible:outline-[var(--color-danger)]",
        className
      )}
      {...p}
    />
  );
}

export function HelpText({ className, ...p }: React.ComponentProps<"p">) {
  const { helpId } = React.use(FieldCtx);
  return <p id={helpId} className={cn("text-xs text-[var(--color-fg-muted)]", className)} {...p} />;
}

export function ErrorText({ className, ...p }: React.ComponentProps<"p">) {
  const { errorId, invalid } = React.use(FieldCtx);
  if (!invalid) return null;
  return <p id={errorId} role="alert" className={cn("inline-flex items-center gap-1 text-xs text-[var(--color-danger)]", className)} {...p} />;
}
```

---

## 4. Select

Prefer the **native `<select>`** for single-choice — it is the most accessible, mobile-friendly, and
zero-JS option. Reach for a custom listbox only when you need rich option content (icons, two-line
rows) or multi-select; then build on **Base UI `Select`** (data-driven) so the ARIA + keyboard
(↑/↓/Home/End/typeahead/Esc) come for free.

**States:** identical to `Input` (default/hover/focus/disabled/error) plus an open/expanded state and
per-option hover/selected/active-descendant for the custom variant. **A11y:** native `<select>` needs
only a `<label>`; the custom version needs `role="combobox"`/`listbox`, `aria-expanded`,
`aria-activedescendant`, and a visible focus ring.

### Framework-agnostic HTML/CSS (native — themed)

```html
<div class="field">
  <label class="field__label" for="plan">Plan</label>
  <div class="select">
    <select class="select__control input" id="plan" name="plan">
      <option value="" disabled selected>Choose a plan…</option>
      <option value="pro">Pro — $20/mo</option>
      <option value="team">Team — $40/mo</option>
    </select>
    <svg class="select__chevron" aria-hidden="true" width="16" height="16"><!-- chevron --></svg>
  </div>
</div>
```

```css
.select { position: relative; display: inline-grid; }
.select__control {
  appearance: none; -webkit-appearance: none;
  padding-inline-end: var(--space-8);          /* room for chevron */
  cursor: pointer;
}
.select__chevron {
  position: absolute; inset-block: 0; inset-inline-end: var(--space-3);
  margin-block: auto; color: var(--color-fg-muted); pointer-events: none;
  transition: transform var(--dur-fast) var(--ease-out);
}
.select__control:focus-visible + .select__chevron { transform: translateY(1px); }
```

### React 19 + Tailwind v4 (Base UI custom listbox, themed)

```tsx
// ui/select.tsx
"use client";
import { Select as S } from "@base-ui/react/select";
import { cn } from "@/lib/cn";

export function Select({ items, placeholder = "Select…" }: {
  items: { value: string; label: string }[];
  placeholder?: string;
}) {
  return (
    <S.Root items={items}>
      <S.Trigger
        className={cn(
          "inline-flex h-10 min-w-48 items-center justify-between gap-2 rounded-[var(--radius-md)]",
          "border border-[var(--color-border)] bg-[var(--color-surface)] px-3 text-sm text-[var(--color-fg)]",
          "hover:border-[var(--color-fg-subtle)] data-[popup-open]:border-[var(--color-accent)]",
          "focus-visible:outline-2 focus-visible:outline-offset-1 focus-visible:outline-[var(--color-ring)]",
          "disabled:cursor-not-allowed disabled:bg-[var(--color-bg-subtle)]"
        )}
      >
        <S.Value placeholder={placeholder} className="data-[placeholder]:text-[var(--color-fg-subtle)]" />
        <S.Icon className="text-[var(--color-fg-muted)]">▾</S.Icon>
      </S.Trigger>
      <S.Portal>
        <S.Positioner sideOffset={6}>
          <S.Popup
            className={cn(
              "min-w-[var(--anchor-width)] rounded-[var(--radius-lg)] border border-[var(--color-border)]",
              "bg-[var(--color-surface-2)] p-1 shadow-[var(--shadow-lg)]",
              "origin-[var(--transform-origin)] transition-[transform,opacity] duration-[var(--dur-fast)]",
              "data-[starting-style]:scale-95 data-[starting-style]:opacity-0"
            )}
          >
            {items.map((it) => (
              <S.Item
                key={it.value}
                value={it.value}
                className={cn(
                  "flex cursor-default items-center gap-2 rounded-[var(--radius-sm)] px-2 py-1.5 text-sm text-[var(--color-fg)]",
                  "data-[highlighted]:bg-[var(--color-accent)] data-[highlighted]:text-[var(--color-accent-fg)]"
                )}
              >
                <S.ItemIndicator className="text-[var(--color-accent)] data-[highlighted]:text-[var(--color-accent-fg)]">✓</S.ItemIndicator>
                <S.ItemText>{it.label}</S.ItemText>
              </S.Item>
            ))}
          </S.Popup>
        </S.Positioner>
      </S.Portal>
    </S.Root>
  );
}
```

---

## 5. Dialog / Modal

A focus-trapping overlay for a single blocking decision. Use the native **`<dialog>`** element for
the agnostic version — it gives you the top layer, backdrop, `Esc`-to-close, and focus trapping for
free. The HTML `popover`/`<dialog>` combo removed most reasons to hand-roll this.

**Anatomy:** `Backdrop · Dialog [ Header (title + close) · Body · Footer (actions) ]`.

**States & contract**

| Concern | Requirement |
|---|---|
| open/close | animate scale+opacity (≤250ms); `data-[starting-style]` for enter |
| focus | trap inside; **return focus** to the trigger on close |
| keyboard | `Esc` closes; `Tab` cycles within |
| semantics | `role="dialog"` `aria-modal="true"`; `aria-labelledby` → title; `aria-describedby` → body |
| scroll | lock background scroll while open |
| reduced-motion | swap transform animation for opacity-only |

### Framework-agnostic HTML/CSS (native `<dialog>`)

```html
<button class="btn btn--accent" type="button" data-open-dialog="confirm">Delete project</button>

<dialog class="dialog" id="confirm" aria-labelledby="confirm-title">
  <header class="dialog__header">
    <h2 class="dialog__title" id="confirm-title">Delete this project?</h2>
    <button class="btn btn--ghost btn--icon" type="button" aria-label="Close" data-close-dialog>✕</button>
  </header>
  <p class="dialog__body">This permanently removes all data. This action cannot be undone.</p>
  <footer class="dialog__footer">
    <button class="btn btn--secondary" type="button" data-close-dialog>Cancel</button>
    <button class="btn btn--danger" type="button">Delete</button>
  </footer>
</dialog>
```

```css
.dialog {
  width: min(32rem, calc(100vw - var(--space-8)));
  padding: var(--space-6);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-xl);
  background: var(--color-surface);
  color: var(--color-fg);
  box-shadow: var(--shadow-xl);
}
.dialog::backdrop { background: oklch(0 0 0 / 0.5); backdrop-filter: blur(2px); }
/* enter animation — :open is the modern hook; reduced-motion zeroes it via the global rule */
.dialog[open] { animation: dialog-in var(--dur-base) var(--ease-out); }
.dialog[open]::backdrop { animation: backdrop-in var(--dur-base) var(--ease-out); }
@keyframes dialog-in { from { opacity: 0; transform: translateY(8px) scale(.97); } }
@keyframes backdrop-in { from { opacity: 0; } }
.dialog__header { display: flex; align-items: start; justify-content: space-between; gap: var(--space-4); }
.dialog__title { font: 600 var(--text-xl)/var(--leading-xl) var(--font-display); }
.dialog__body { margin-block: var(--space-4); color: var(--color-fg-muted); }
.dialog__footer { display: flex; justify-content: flex-end; gap: var(--space-2); }
```

```js
// 6 lines of progressive enhancement — native dialog does the trapping + focus return
document.querySelectorAll("[data-open-dialog]").forEach((b) =>
  b.addEventListener("click", () => document.getElementById(b.dataset.openDialog).showModal())
);
document.querySelectorAll("[data-close-dialog]").forEach((b) =>
  b.addEventListener("click", () => b.closest("dialog").close())
);
```

### React 19 + Tailwind v4 (Base UI Dialog, themed)

```tsx
// ui/dialog.tsx
"use client";
import { Dialog as D } from "@base-ui/react/dialog";
import { cn } from "@/lib/cn";

export function ConfirmDialog({ trigger, title, children, onConfirm }: {
  trigger: React.ReactNode; title: string; children: React.ReactNode; onConfirm: () => void;
}) {
  return (
    <D.Root>
      <D.Trigger render={trigger as React.ReactElement} />
      <D.Portal>
        <D.Backdrop className="fixed inset-0 bg-black/50 backdrop-blur-[2px] transition-opacity duration-[var(--dur-base)] data-[starting-style]:opacity-0" />
        <D.Popup
          className={cn(
            "fixed left-1/2 top-1/2 w-[min(32rem,calc(100vw-2rem))] -translate-x-1/2 -translate-y-1/2",
            "rounded-[var(--radius-xl)] border border-[var(--color-border)] bg-[var(--color-surface)] p-6 shadow-[var(--shadow-xl)]",
            "transition-[transform,opacity] duration-[var(--dur-base)] ease-[var(--ease-out)]",
            "data-[starting-style]:translate-y-[calc(-50%+8px)] data-[starting-style]:scale-[.97] data-[starting-style]:opacity-0",
            "motion-reduce:transition-none"
          )}
        >
          <div className="flex items-start justify-between gap-4">
            <D.Title className="font-[var(--font-display)] text-xl font-semibold text-[var(--color-fg)]">{title}</D.Title>
            <D.Close className="rounded-[var(--radius-md)] p-1 text-[var(--color-fg-muted)] hover:bg-[var(--color-surface-2)] focus-visible:outline-2 focus-visible:outline-[var(--color-ring)]" aria-label="Close">✕</D.Close>
          </div>
          <D.Description className="my-4 text-[var(--color-fg-muted)]">{children}</D.Description>
          <div className="flex justify-end gap-2">
            <D.Close className="h-10 rounded-[var(--radius-md)] border border-[var(--color-border)] bg-[var(--color-surface)] px-4 text-sm hover:bg-[var(--color-surface-2)]">Cancel</D.Close>
            <button onClick={onConfirm} className="h-10 rounded-[var(--radius-md)] bg-[var(--color-danger)] px-4 text-sm text-[var(--color-danger-fg)] hover:brightness-110">Delete</button>
          </div>
        </D.Popup>
      </D.Portal>
    </D.Root>
  );
}
```

---

## 6. Tabs

Switch between sibling views within the same context. Use the WAI-ARIA Tabs pattern: arrow keys move
between tabs, `Tab` jumps from the active tab into the panel.

**Anatomy:** `Tablist [ Tab · Tab · … ] · Tabpanel`. An animated underline indicator marks the active
tab — one orchestrated movement, not a glow.

**States:** tab default/hover (fg brightens)/selected (accent underline + `--color-fg`)/
focus-visible (ring)/disabled. **A11y:** `role="tablist"` → `role="tab"` with `aria-selected` +
`aria-controls`; panel `role="tabpanel"` + `aria-labelledby`; inactive tabs `tabindex="-1"`, active
`tabindex="0"` (roving tabindex); `←/→/Home/End` move selection.

### Framework-agnostic HTML/CSS

```html
<div class="tabs">
  <div class="tabs__list" role="tablist" aria-label="Account settings">
    <button class="tabs__tab" role="tab" id="t-profile" aria-controls="p-profile" aria-selected="true" tabindex="0">Profile</button>
    <button class="tabs__tab" role="tab" id="t-billing" aria-controls="p-billing" aria-selected="false" tabindex="-1">Billing</button>
  </div>
  <div class="tabs__panel" role="tabpanel" id="p-profile" aria-labelledby="t-profile" tabindex="0">…</div>
  <div class="tabs__panel" role="tabpanel" id="p-billing" aria-labelledby="t-billing" tabindex="0" hidden>…</div>
</div>
```

```css
.tabs__list {
  display: inline-flex; gap: var(--space-1);
  border-block-end: 1px solid var(--color-border);   /* the track */
}
.tabs__tab {
  position: relative; padding: var(--space-2) var(--space-3);
  background: none; border: 0; cursor: pointer;
  font: 500 var(--text-sm)/var(--leading-sm) var(--font-body);
  color: var(--color-fg-muted);
  transition: color var(--dur-fast) var(--ease-out);
}
.tabs__tab:hover { color: var(--color-fg); }
.tabs__tab[aria-selected="true"] { color: var(--color-fg); }
.tabs__tab[aria-selected="true"]::after {   /* the indicator */
  content: ""; position: absolute; inset-inline: var(--space-3); inset-block-end: -1px;
  block-size: 2px; background: var(--color-accent); border-radius: var(--radius-full);
}
.tabs__tab:disabled { color: var(--color-fg-subtle); cursor: not-allowed; }
.tabs__panel { padding-block-start: var(--space-4); }
```

### React 19 + Tailwind v4 (Base UI Tabs with motion underline, themed)

```tsx
// ui/tabs.tsx
"use client";
import { Tabs as T } from "@base-ui/react/tabs";
import { cn } from "@/lib/cn";

export function Tabs({ tabs }: { tabs: { value: string; label: string; content: React.ReactNode }[] }) {
  return (
    <T.Root defaultValue={tabs[0]?.value}>
      <T.List className="relative flex gap-1 border-b border-[var(--color-border)]">
        {tabs.map((t) => (
          <T.Tab
            key={t.value}
            value={t.value}
            className={cn(
              "px-3 py-2 text-sm font-medium text-[var(--color-fg-muted)] transition-colors duration-[var(--dur-fast)]",
              "hover:text-[var(--color-fg)] data-[selected]:text-[var(--color-fg)]",
              "focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-[var(--color-ring)] disabled:text-[var(--color-fg-subtle)]"
            )}
          >
            {t.label}
          </T.Tab>
        ))}
        {/* Base UI's built-in indicator follows the active tab via a CSS transition */}
        <T.Indicator className="absolute bottom-[-1px] h-0.5 rounded-full bg-[var(--color-accent)] transition-all duration-[var(--dur-base)] ease-[var(--ease-out)] left-[var(--active-tab-left)] w-[var(--active-tab-width)] motion-reduce:transition-none" />
      </T.List>
      {tabs.map((t) => (
        <T.Panel key={t.value} value={t.value} className="pt-4 focus-visible:outline-2 focus-visible:outline-[var(--color-ring)]">
          {t.content}
        </T.Panel>
      ))}
    </T.Root>
  );
}
```

---

## 7. Tooltip

A small, non-interactive hint on hover/focus. **Never put essential or interactive content in a
tooltip** (it is invisible to touch and unreliable for AT) — use a Popover for that. Show on both
hover **and** keyboard focus; dismiss on `Esc`.

**States:** hidden → showing (after a small open delay ~400ms, instant close); positioned with a
flip/shift so it never clips the viewport. **A11y:** the trigger references the tip via
`aria-describedby`; the tip has `role="tooltip"`; it must be reachable by keyboard focus, not hover
only (WCAG 1.4.13: dismissible, hoverable, persistent).

### Framework-agnostic HTML/CSS (CSS-anchor positioned)

```html
<button class="btn btn--ghost btn--icon" type="button" aria-describedby="tip-copy" id="copy">
  <svg aria-hidden="true" width="16" height="16"><!-- copy --></svg>
</button>
<span class="tooltip" role="tooltip" id="tip-copy">Copy to clipboard</span>
```

```css
#copy { anchor-name: --copy-trigger; }
.tooltip {
  position: absolute; position-anchor: --copy-trigger;
  inset-block-end: anchor(top); inset-inline-start: anchor(center);
  translate: -50% calc(-1 * var(--space-2));
  max-inline-size: 16rem; padding: var(--space-1) var(--space-2);
  background: var(--color-fg); color: var(--color-bg);    /* inverted chip — high contrast */
  border-radius: var(--radius-sm);
  font: 500 var(--text-xs)/var(--leading-xs) var(--font-body);
  opacity: 0; visibility: hidden; pointer-events: none;
  transition: opacity var(--dur-fast) var(--ease-out), visibility var(--dur-fast);
}
/* show on hover OR keyboard focus — both required for a11y */
#copy:hover + .tooltip,
#copy:focus-visible + .tooltip { opacity: 1; visibility: visible; }
```

### React 19 + Tailwind v4 (Base UI Tooltip, themed)

```tsx
// ui/tooltip.tsx
"use client";
import { Tooltip as Tt } from "@base-ui/react/tooltip";
import { cn } from "@/lib/cn";

export function Tooltip({ children, content }: { children: React.ReactNode; content: string }) {
  return (
    <Tt.Provider delay={400}>          {/* open delay; close is instant */}
      <Tt.Root>
        <Tt.Trigger render={children as React.ReactElement} />
        <Tt.Portal>
          <Tt.Positioner sideOffset={8}>
            <Tt.Popup
              className={cn(
                "max-w-64 rounded-[var(--radius-sm)] bg-[var(--color-fg)] px-2 py-1 text-xs font-medium text-[var(--color-bg)]",
                "shadow-[var(--shadow-md)] transition-[transform,opacity] duration-[var(--dur-fast)] ease-[var(--ease-out)]",
                "data-[starting-style]:scale-95 data-[starting-style]:opacity-0 motion-reduce:transition-none"
              )}
            >
              {content}
            </Tt.Popup>
          </Tt.Positioner>
        </Tt.Portal>
      </Tt.Root>
    </Tt.Provider>
  );
}
```

---

## 8. Badge

A small status/label chip. Carries meaning via **icon + text**, never color alone (WCAG 1.4.1).
Non-interactive by default; if it represents a removable filter it becomes a Tag with a close button.

**Variants:** `neutral`, `accent`, `success`, `warning`, `danger`. **A11y:** if the badge conveys
status not obvious from text (e.g. a colored dot meaning "online"), give it an accessible name via
visually-hidden text or `aria-label`.

### Framework-agnostic HTML/CSS

```html
<span class="badge badge--success"><span class="badge__dot" aria-hidden="true"></span>Active</span>
<span class="badge badge--warning">Pending review</span>
<span class="badge badge--neutral">v4.3</span>
```

```css
.badge {
  display: inline-flex; align-items: center; gap: var(--space-1);
  padding: var(--space-1) var(--space-2);
  border: 1px solid transparent; border-radius: var(--radius-full);
  font: 500 var(--text-xs)/1 var(--font-body); letter-spacing: var(--tracking-xs);
  white-space: nowrap;
}
.badge__dot { inline-size: .5em; block-size: .5em; border-radius: var(--radius-full); background: currentColor; }
/* tinted, low-chroma fills — each pairs a fg + a subtle bg derived from the semantic color */
.badge--neutral { background: var(--color-surface-2); color: var(--color-fg-muted); border-color: var(--color-border); }
.badge--accent  { background: color-mix(in oklch, var(--color-accent) 14%, transparent); color: var(--color-accent); }
.badge--success { background: color-mix(in oklch, var(--color-success) 14%, transparent); color: var(--color-success); }
.badge--warning { background: color-mix(in oklch, var(--color-warning) 18%, transparent); color: var(--color-warning); }
.badge--danger  { background: color-mix(in oklch, var(--color-danger) 14%, transparent); color: var(--color-danger); }
```

### React 19 + Tailwind v4

```tsx
// ui/badge.tsx
import * as React from "react";
import { cva, type VariantProps } from "class-variance-authority";
import { cn } from "@/lib/cn";

const badge = cva(
  "inline-flex items-center gap-1 rounded-full border px-2 py-1 text-xs font-medium leading-none whitespace-nowrap",
  {
    variants: {
      variant: {
        neutral: "border-[var(--color-border)] bg-[var(--color-surface-2)] text-[var(--color-fg-muted)]",
        accent: "border-transparent bg-[color-mix(in_oklch,var(--color-accent)_14%,transparent)] text-[var(--color-accent)]",
        success: "border-transparent bg-[color-mix(in_oklch,var(--color-success)_14%,transparent)] text-[var(--color-success)]",
        warning: "border-transparent bg-[color-mix(in_oklch,var(--color-warning)_18%,transparent)] text-[var(--color-warning)]",
        danger: "border-transparent bg-[color-mix(in_oklch,var(--color-danger)_14%,transparent)] text-[var(--color-danger)]",
      },
    },
    defaultVariants: { variant: "neutral" },
  }
);

export function Badge({ className, variant, ...p }: React.ComponentProps<"span"> & VariantProps<typeof badge>) {
  return <span className={cn(badge({ variant }), className)} {...p} />;
}
```

---

## 9. Toast

A transient, **non-blocking** notification. It must announce itself to AT without stealing focus, so
it lives in an `aria-live` region. Auto-dismiss only for non-critical info; errors stay until
dismissed. Stack newest-on-top; cap at 3–4 visible.

**States:** enter (slide+fade in), resting, exit (fade out), paused-on-hover (resets the timer).
**A11y:** the live region is `role="status"` `aria-live="polite"` for info/success, `role="alert"`
`aria-live="assertive"` for errors; each toast has a close button; never auto-dismiss the only copy
of critical information.

### Framework-agnostic HTML/CSS

```html
<!-- one persistent region near the end of <body> -->
<div class="toast-region" role="status" aria-live="polite" aria-atomic="false">
  <div class="toast toast--success" role="alert">
    <svg class="toast__icon" aria-hidden="true" width="18" height="18"><!-- check --></svg>
    <div class="toast__copy">
      <p class="toast__title">Saved</p>
      <p class="toast__desc">Your changes are live.</p>
    </div>
    <button class="toast__close" type="button" aria-label="Dismiss">✕</button>
  </div>
</div>
```

```css
.toast-region {
  position: fixed; inset-block-end: var(--space-4); inset-inline-end: var(--space-4);
  display: flex; flex-direction: column-reverse; gap: var(--space-2);
  inline-size: min(22rem, calc(100vw - var(--space-8))); z-index: 60;
}
.toast {
  display: flex; align-items: start; gap: var(--space-3);
  padding: var(--space-3) var(--space-4);
  background: var(--color-surface-2);
  border: 1px solid var(--color-border);
  border-radius: var(--radius-lg);
  box-shadow: var(--shadow-lg);
  animation: toast-in var(--dur-base) var(--ease-spring) both;
}
.toast--success { border-inline-start: 3px solid var(--color-success); }
.toast--danger  { border-inline-start: 3px solid var(--color-danger); }
.toast__icon { color: var(--color-success); flex-shrink: 0; }
.toast__title { font: 600 var(--text-sm)/var(--leading-sm) var(--font-body); color: var(--color-fg); }
.toast__desc { font-size: var(--text-xs); color: var(--color-fg-muted); }
.toast__close { margin-inline-start: auto; background: none; border: 0; color: var(--color-fg-muted); cursor: pointer; }
@keyframes toast-in { from { opacity: 0; transform: translateX(var(--space-4)); } }
.toast--leaving { animation: toast-out var(--dur-fast) var(--ease-out) forwards; }
@keyframes toast-out { to { opacity: 0; transform: translateY(var(--space-2)); } }
```

### React 19 + Tailwind v4 (Base UI Toast, themed)

```tsx
// ui/toast.tsx
"use client";
import { Toast } from "@base-ui/react/toast";
import { cn } from "@/lib/cn";

export function ToastProvider({ children }: { children: React.ReactNode }) {
  return <Toast.Provider>{children}<Viewport /></Toast.Provider>;
}

function Viewport() {
  // Base UI renders toasts by mapping the manager's `toasts` array — there is no
  // <Toast.List> render-prop component. The viewport must live inside <Toast.Portal>.
  const { toasts } = Toast.useToastManager();
  return (
    <Toast.Portal>
      <Toast.Viewport className="fixed bottom-4 right-4 z-[60] flex w-[min(22rem,calc(100vw-2rem))] flex-col-reverse gap-2">
        {toasts.map((toast) => (
          <Toast.Root
            key={toast.id}
            toast={toast}
            className={cn(
              "flex items-start gap-3 rounded-[var(--radius-lg)] border border-[var(--color-border)] bg-[var(--color-surface-2)] p-3 shadow-[var(--shadow-lg)]",
              "transition-[transform,opacity] duration-[var(--dur-base)] ease-[var(--ease-spring)]",
              "data-[starting-style]:translate-x-4 data-[starting-style]:opacity-0",
              "data-[ending-style]:translate-y-2 data-[ending-style]:opacity-0 motion-reduce:transition-none",
              // custom variant is read from `toast.data` (set when you call toast.add)
              toast.data?.variant === "error" && "border-l-[3px] border-l-[var(--color-danger)]",
              toast.data?.variant === "success" && "border-l-[3px] border-l-[var(--color-success)]"
            )}
          >
            <div className="flex-1">
              <Toast.Title className="text-sm font-semibold text-[var(--color-fg)]" />
              <Toast.Description className="text-xs text-[var(--color-fg-muted)]" />
            </div>
            <Toast.Close className="ml-auto text-[var(--color-fg-muted)] hover:text-[var(--color-fg)] focus-visible:outline-2 focus-visible:outline-[var(--color-ring)]" aria-label="Dismiss">✕</Toast.Close>
          </Toast.Root>
        ))}
      </Toast.Viewport>
    </Toast.Portal>
  );
}

// usage — Base UI sets role="status"/"alert" + aria-live automatically; pass custom variant via `data`
// const toast = Toast.useToastManager();
// toast.add({ title: "Saved", description: "Your changes are live.", data: { variant: "success" } });
```

---

## 10. Empty · Loading · Error states (don't ship without them)

Every data-driven surface needs all three. Match them to the component, and never leave a blank box.

| State | Pattern | A11y |
|---|---|---|
| **Loading** | Skeleton blocks (shimmer animates `--color-bg-subtle` → `--color-surface-2`), **not** a centered spinner for content; spinner only for actions | `aria-busy="true"` on the region; visually-hidden "Loading" |
| **Empty** | Icon + one-line headline + one-line guidance + a single primary action ("Create your first project") | real copy, not "No data"; action is keyboard-reachable |
| **Error** | Inline `role="alert"` with what happened + how to fix + a retry button | `role="alert"` so it is announced; never color-only |

```css
.skeleton {
  border-radius: var(--radius-md);
  background: linear-gradient(90deg, var(--color-bg-subtle) 25%, var(--color-surface-2) 37%, var(--color-bg-subtle) 63%);
  background-size: 400% 100%;
  animation: skeleton 1.4s var(--ease-in-out) infinite;
}
@keyframes skeleton { from { background-position: 100% 0; } to { background-position: 0 0; } }
@media (prefers-reduced-motion: reduce) { .skeleton { animation: none; background: var(--color-bg-subtle); } }
```

```tsx
// Generic async boundary — React 19 Suspense + error pattern, themed states
function AsyncList({ promise }: { promise: Promise<Item[]> }) {
  const items = use(promise);                       // suspends → fallback below
  if (items.length === 0) return <EmptyState />;    // empty
  return <ul>{items.map((i) => <li key={i.id}>{i.name}</li>)}</ul>;
}
// <Suspense fallback={<div aria-busy className="space-y-2"><div className="skeleton h-6"/><div className="skeleton h-6"/></div>}>
//   <ErrorBoundary fallback={<p role="alert" className="text-[var(--color-danger)]">Couldn't load. <button>Retry</button></p>}>
//     <AsyncList promise={promise} />
//   </ErrorBoundary>
// </Suspense>
```

---

## 11. Dark mode (every component is already covered)

Because each component reads **only** semantic CONTRACT tokens, dark mode is a values swap — no
component CSS changes. Override the color vars under `[data-theme="dark"]` / `.dark`, adjusting **L
and C** (never a naive invert): surfaces ride a tinted ladder (not `#000`), borders become low-alpha
white, accent lightness rises slightly for contrast on dark.

```css
[data-theme="dark"], .dark {
  --color-bg:           oklch(0.16 0.006 264);   /* tinted, never pure black */
  --color-bg-subtle:    oklch(0.20 0.006 264);
  --color-surface:      oklch(0.21 0.007 264);
  --color-surface-2:    oklch(0.25 0.008 264);
  --color-fg:           oklch(0.96 0.004 264);
  --color-fg-muted:     oklch(0.74 0.006 264);
  --color-fg-subtle:    oklch(0.56 0.006 264);
  --color-border:        oklch(1 0 0 / 0.10);    /* low-alpha white hairline */
  --color-border-subtle: oklch(1 0 0 / 0.06);
  --color-accent:       oklch(0.70 0.17 264);    /* +L for contrast on dark */
  --color-accent-hover: oklch(0.76 0.16 264);
  --color-ring:         oklch(0.70 0.17 264);
}
```

> The shadow ladder also softens in dark mode (shadows read poorly) — depth leans harder on the
> surface ladder + hairlines, exactly the Linear/Raycast approach.

---

## Component checklist (audit gate before merge)

- [ ] All states present: default · hover · active · **focus-visible** · disabled · loading · error
- [ ] Focus ring via `--color-ring`, 2px, 2px offset, visible on keyboard nav
- [ ] Built on native semantics; ARIA only fills real gaps; correct role + accessible name
- [ ] Full keyboard operation (Tab/arrows/Esc as the pattern requires); 44px targets for primary controls
- [ ] No color-only signaling (icon + text accompany status color)
- [ ] Consumes **only** semantic CONTRACT tokens — zero hardcoded hex/px/ms
- [ ] Motion ≤ 250ms, transform/opacity only, honors `prefers-reduced-motion`
- [ ] Works in light + dark via the token swap (no component CSS forks)
- [ ] Empty / loading / error designed, not afterthoughts

---

## Sources

- WCAG 2.2 (W3C) — https://www.w3.org/TR/WCAG22/
- WAI-ARIA Authoring Practices (APG) — patterns: Tabs, Dialog, Combobox, Tooltip — https://www.w3.org/WAI/ARIA/apg/patterns/
- A11Y Pros — Accessibility in design systems — https://a11ypros.com/blog/accessibility-in-design-systems
- UXPin — UI component library checklist — https://www.uxpin.com/studio/blog/ui-component-library-checklist-essential-elements/
- Atlassian Design — Color (interaction-state tokens) — https://atlassian.design/foundations/color
- Atlassian Design — Elevation — https://atlassian.design/foundations/elevation
- Material Design 3 — Motion easing & duration — https://m3.material.io/styles/motion/easing-and-duration/tokens-specs
- shadcn/ui — Tailwind v4 — https://ui.shadcn.com/docs/tailwind-v4
- shadcn/ui — Registry MCP Server — https://ui.shadcn.com/docs/registry/mcp
- Base UI (MUI) — components & releases — https://github.com/mui/base-ui
- Base UI 1.0 release (InfoQ) — https://www.infoq.com/news/2026/02/baseui-v1-accessible/
- React Aria (Adobe) — https://react-spectrum.adobe.com/react-aria/
- React v19 (ref as prop, Actions) — https://react.dev/blog/2024/12/05/react-19
- React 19.2 (View Transitions, useEffectEvent) — https://react.dev/blog/2025/10/01/react-19-2
- Tailwind CSS v4 Theme variables — https://tailwindcss.com/docs/theme
- MDN — `<dialog>` element (top layer, focus trap) — https://developer.mozilla.org/en-US/docs/Web/HTML/Element/dialog
- MDN — CSS anchor positioning — https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_anchor_positioning
- Geist (Vercel) — functional color scale & typography — https://vercel.com/geist/colors
- How we redesigned the Linear UI (LCH theme engine) — https://linear.app/now/how-we-redesigned-the-linear-ui
- A calmer interface (Linear refresh — restraint, removed separators) — https://linear.app/now/behind-the-latest-design-refresh
- Raycast DESIGN.md (surface ladder, no drop shadows) — https://github.com/VoltAgent/awesome-design-md/blob/main/design-md/raycast/DESIGN.md
- Connect: front-end experience (Stripe motion rules) — https://stripe.com/blog/connect-front-end-experience
- Motion (ex framer-motion) — React transitions — https://motion.dev/docs/react-transitions
