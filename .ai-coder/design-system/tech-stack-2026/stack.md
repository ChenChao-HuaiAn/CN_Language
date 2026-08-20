---
title: The AI-Native 2026 Stack — Canonical Verdict
description: The opinionated default frontend stack for June 2026, with exact versions, WHY, minimal setup, alternatives, and the AI-native bridge (v0 + shadcn registry + MCP + design tokens).
date: 2026-06-09
status: canonical
audience: AI agents and engineers — read without fetching links
---

# The AI-Native 2026 Stack

> **The verdict.** Build on **Next.js 16 (App Router) + React 19 + Tailwind v4 + shadcn/ui (Base UI primitives) + TanStack Query + Zustand**, deployed on **Vercel**, generated and edited by **v0 grounded in a shadcn registry over MCP**. This is not a survey — it is the default. Deviate only for the specific workloads called out below.

This module is decision-oriented. Every choice has a one-line WHY, a current version, and copy-paste setup. The taste layer (color, motion, type) lives in the other modules; here we lock the *plumbing* so agents stop re-litigating it.

---

## The default stack at a glance

| Layer | Pick | Version (Jun 2026) | One-line WHY |
|---|---|---|---|
| UI runtime | **React** | 19.2 | Actions, `use()`, RSC, Compiler 1.0 — all stable |
| Meta-framework | **Next.js** (App Router) | 16.2 | Most mature RSC; Turbopack default; Cache Components |
| Styling | **Tailwind CSS** | v4.3 | CSS-first `@theme`, OKLCH, runtime CSS vars |
| Components | **shadcn/ui** (copy-paste) | CLI 3.x | You own the source — AI edits real code |
| Primitives | **Base UI** (default) / Radix (existing) | 1.x | Headless, accessible, full-time maintained |
| Server-state | **TanStack Query** | v5 | Suspense-first, smaller, the standard |
| Client-state | **Zustand** | v5 | Zero-boilerplate, no Provider, selector subs |
| Bundler | **Turbopack** (in Next) | stable | 2–5× builds, ~10× Fast Refresh |
| Runtime / PM | **Bun** | 1.2+ | Fastest installs + scripts |
| Fonts | **`next/font`** + variable fonts | — | Self-hosted, zero CLS, GDPR-safe |
| Deploy | **Vercel** (default) / Cloudflare (cost) | — | Fluid Compute vs V8 isolates |
| AI codegen | **v0 + shadcn registry + MCP** | — | Design-system-grounded generation |

**One line to memorize:** `Next 16 + React 19 + Tailwind v4 + shadcn/ui (Base UI) + TanStack Query + Zustand`, on Vercel, via v0/MCP.

**Two escape hatches you will actually use:** content-heavy sites → **Astro 6**; no-lock-in type-safe full-stack → **TanStack Start v1** on **Vite 8**.

---

## 1. React 19 — the foundation

**Current:** 19.2 (Oct 2025); 19.0 stable Dec 5 2024. React **Compiler 1.0** shipped Oct 2025.

**WHY it wins:** React 19 collapses the `useState + useTransition + manual try/catch` ritual into **Actions**, makes data declarative with **`use()`**, and ships **Server Components + Server Actions** as a stable, framework-backed model. The Compiler deletes most hand-written `useMemo`/`useCallback`. `ref` is now a plain prop — `forwardRef` is gone (codemod available).

### Actions — the new form/mutation primitive

```tsx
"use client";
import { useActionState } from "react";
import { useFormStatus } from "react-dom";

async function updateName(_prev: string, formData: FormData) {
  const name = formData.get("name") as string;
  const res = await fetch("/api/name", { method: "POST", body: name });
  if (!res.ok) return "Failed to update"; // returned value becomes error state
  return ""; // success
}

function SubmitButton() {
  const { pending } = useFormStatus(); // reads the nearest parent <form>
  return (
    <button className="btn-primary" disabled={pending}>
      {pending ? "Saving…" : "Save"}
    </button>
  );
}

export function NameForm() {
  const [error, formAction, isPending] = useActionState(updateName, "");
  return (
    <form action={formAction} aria-busy={isPending}>
      <input name="name" className="input" />
      <SubmitButton />
      {error && <p role="alert" className="text-(--color-danger)">{error}</p>}
    </form>
  );
}
```

### `use()` — read a promise/context in render

Unlike other hooks, `use()` may be called conditionally and in loops. Production-stable in **Server Components**; on the client it integrates with Suspense.

```tsx
import { use, Suspense } from "react";

function Comments({ commentsPromise }: { commentsPromise: Promise<string[]> }) {
  const comments = use(commentsPromise); // suspends until resolved
  return <ul>{comments.map((c) => <li key={c}>{c}</li>)}</ul>;
}

export default function Page() {
  const commentsPromise = fetch("/api/comments").then((r) => r.json());
  return (
    <Suspense fallback={<p className="text-(--color-fg-muted)">Loading…</p>}>
      <Comments commentsPromise={commentsPromise} />
    </Suspense>
  );
}
```

### RSC rules you must not get wrong

- RSC are the **default** in `app/` — zero JS shipped. They cannot use `useState`/`useEffect`/context; add `"use client"` for interactivity.
- `"use server"` marks **Server Actions** (callable from client), **not** Server Components.
- Client Components cannot import Server Components (the reverse is fine — pass them as `children`).
- RSC is stable as a *React feature*, but bundler APIs are not semver-stable across 19.x minors — **use a framework** (Next.js is the most mature). 19.2 adds **View Transitions**, **`useEffectEvent`**, **`<Activity>`**.

---

## 2. Next.js 16 — the default meta-framework

**Current:** 16.2 (Mar 18 2026); 16.0 shipped Oct 21 2025. App Router is the path; Pages Router is legacy.

**WHY it wins:** the most mature RSC implementation, **Turbopack is now the default bundler** (2–5× builds, up to 10× Fast Refresh), **Cache Components** make caching explicit and opt-in, and **Devtools MCP** lets agents diagnose inside the dev loop. `create-next-app` scaffolds an `AGENTS.md`.

### Cache Components + PPR — the new mental model

Everything is dynamic (request-time) by default; caching is **opt-in** via `"use cache"`. **PPR** serves a static shell instantly and streams dynamic content through Suspense.

```ts
// next.config.ts — recommended defaults
const nextConfig = {
  cacheComponents: true,   // enables PPR + the "use cache" model
  // reactCompiler: true,  // build-time memoization (Babel-based → slower builds); enable when ready
};
export default nextConfig;
```

```tsx
// "use cache" caches a component/function; the compiler derives the cache key
async function ProductList() {
  "use cache";
  const products = await db.products.findMany();
  return <ul>{products.map((p) => <li key={p.id}>{p.name}</li>)}</ul>;
}
```

### Caching APIs (16) — the profile arg is now required

```ts
import { revalidateTag, updateTag, refresh } from "next/cache";

revalidateTag("blog-posts", "max"); // SWR; the profile arg is REQUIRED in 16
updateTag(`user-${id}`);            // Server Action: read-your-writes (expire + read fresh, same request)
refresh();                          // Server Action: refresh uncached data only (cache untouched)
```

### `proxy.ts` (formerly `middleware.ts`)

```ts
// proxy.ts — Node.js runtime; makes the network boundary explicit
import { NextRequest, NextResponse } from "next/server";
export default function proxy(request: NextRequest) {
  return NextResponse.redirect(new URL("/home", request.url));
}
```

### Minimal setup

```bash
npx create-next-app@latest    # App Router + TS + Tailwind v4 + ESLint + AGENTS.md by default
# Upgrade an existing app:
npx @next/codemod@canary upgrade latest
```

### Breaking changes you will hit (16)

| Change | Action |
|---|---|
| Node 18 dropped | Require **Node 20.9+**, TS 5.1+, Safari 16.4+/Chrome 111+ |
| `params`/`searchParams`/`cookies()`/`headers()`/`draftMode()` async | **`await`** them everywhere |
| `experimental.ppr` / `dynamicIO` removed | Use `cacheComponents: true` |
| `next lint` removed | Move to ESLint flat config or **Biome** |
| `images.domains` deprecated | Use `images.remotePatterns` |
| Want webpack | `next build --webpack` to opt out of Turbopack |

**AI-native angle:** **Next Devtools MCP** gives agents routing/caching/rendering knowledge, unified browser+server logs, automatic stack-trace access, and active-route awareness — so the agent fixes the bug inside the dev loop instead of guessing.

---

## 3. Framework alternatives — pick by workload

Do not reach for these by default. Reach for them when the workload column describes your app exactly.

| Framework | Version | Pick it when… | The reason |
|---|---|---|---|
| **Next.js 16** | 16.2 | App-first product, RSC, edge SSR | Most mature RSC; deepest Vercel integration |
| **Astro 6** | stable (Mar 2026) | Content / marketing / docs / blog | Ships ~zero JS; Content Layer; Server Islands |
| **TanStack Start v1** | v1 (GA late 2025) | Type-safe full-stack, no Vercel lock-in | End-to-end types; server functions; Vite + Nitro |
| **React Router v7** | v7 | Migrating off Remix | Remix folded in; v6→v7 non-breaking |
| **SvelteKit + Svelte 5** | SK 2.57 / Svelte 5.55 | Smallest bundles, lean apps | Runes reactivity; single-command deploy |

### Astro 6 — content-driven sites

**Content Layer** is a unified, type-safe loader API (Markdown, CMS, REST, assets). Markdown builds up to **5× faster**, MDX up to **2×**, memory cut **25–50%**. **Server Islands** combine static HTML with deferred dynamic components via `server:defer`, and React/Vue/Svelte/Solid islands can coexist on one page.

```ts
// src/content.config.ts (v6 path; was src/content/config.ts in v5)
import { defineCollection, z } from "astro:content";
import { glob } from "astro/loaders";

const blog = defineCollection({
  loader: glob({ pattern: "**/*.md", base: "./src/blog" }),
  schema: z.object({ title: z.string(), date: z.date() }),
});
export const collections = { blog };
```

```astro
<!-- Server Island: skipped on first paint, streamed in later -->
<UserAvatar server:defer>
  <Placeholder slot="fallback" />
</UserAvatar>
```

Migration (v5→v6): Node `^22.12 || ^24`, Zod major bump (`.nonempty()` → `.min(1)`), removed `Astro.glob` and `getEntryBySlug`.

### TanStack Start v1 — type-safe, no lock-in

Built on TanStack Router + Vite + Nitro. **Server functions** are TypeScript functions that always run on the server but are called like normal functions from the client — no hand-written API contracts. Pairs natively with TanStack Query/Router/Table/Form. RSC is experimental in v1 — **choose Next.js if you need RSC today**.

```ts
import { createServerFn } from "@tanstack/react-start";

const getUser = createServerFn({ method: "GET" })
  .handler(async () => db.user.findFirst()); // runs only on the server
```

### SvelteKit + Svelte 5 runes

Four runes replace implicit reactivity and now work **outside `.svelte` files** (in `.svelte.ts`). Adopt file-by-file — runes mode activates the moment you use any rune.

```svelte
<script>
  let count = $state(0);
  let doubled = $derived(count * 2);
  $effect(() => console.log("count is", count)); // runs after DOM update
</script>
<button onclick={() => count++}>{count} / {doubled}</button>
```

Scaffold with the `sv` CLI; `$app/stores` is deprecated in favor of `$app/state`. **React Router v7** is the Remix successor (single `react-router` package; framework mode adds Vite compiler, SSR, type-safe loaders/actions, HMR).

---

## 4. Tailwind CSS v4 — styling that doubles as design tokens

**Current:** v4.3 (v4.0 early 2025). New **Oxide** engine (Rust + Lightning CSS). No JS config — all customization is CSS-first via `@theme`.

**WHY it wins:** every design token becomes a **real CSS variable at runtime**, so themes override without a rebuild — the styling layer *is* the token layer. The palette is **OKLCH** for a wider gamut and perceptually-uniform lightness. Models are heavily trained on its utility vocabulary, and v0/shadcn emit it natively.

### Setup (Vite path; Next ships it via `create-next-app`)

```bash
bun add tailwindcss @tailwindcss/vite
```

```ts
// vite.config.ts
import tailwindcss from "@tailwindcss/vite";
export default { plugins: [tailwindcss()] };
```

```css
/* app.css — single import; no @tailwind base/components/utilities */
@import "tailwindcss";
```

### `@theme` — wire the canonical token contract

The design system's framework-agnostic CSS variables are mirrored here so Tailwind generates utilities for them. Define semantic tokens once; theming cascades.

```css
@import "tailwindcss";

@theme {
  /* Color — semantic, OKLCH(L C H). These create bg-*, text-*, border-* utilities */
  --color-bg: oklch(0.99 0.003 240);
  --color-bg-subtle: oklch(0.97 0.005 240);
  --color-surface: oklch(1 0 0);
  --color-surface-2: oklch(0.98 0.004 240);
  --color-fg: oklch(0.21 0.01 240);
  --color-fg-muted: oklch(0.50 0.01 240);
  --color-fg-subtle: oklch(0.65 0.008 240);
  --color-border: oklch(0.91 0.006 240);
  --color-border-subtle: oklch(0.94 0.004 240);
  --color-accent: oklch(0.58 0.18 256);
  --color-accent-fg: oklch(0.99 0.005 256);
  --color-accent-hover: oklch(0.52 0.19 256);
  --color-success: oklch(0.62 0.15 150);
  --color-warning: oklch(0.75 0.15 75);
  --color-danger: oklch(0.58 0.21 25);
  --color-ring: oklch(0.58 0.18 256);

  /* Space — 8pt-based */
  --spacing: 0.25rem;            /* p-4 = calc(var(--spacing) * 4) = 1rem */

  /* Radius */
  --radius-sm: 0.25rem;
  --radius-md: 0.375rem;
  --radius-lg: 0.5rem;
  --radius-xl: 0.75rem;
  --radius-2xl: 1rem;

  /* Type — fluid clamp() + variable font */
  --font-body: var(--font-sans), ui-sans-serif, system-ui, sans-serif;
  --font-mono: "Berkeley Mono", ui-monospace, monospace;
  --text-base: clamp(1rem, 0.95rem + 0.25vw, 1.0625rem);
  --text-3xl: clamp(1.875rem, 1.5rem + 1.5vw, 2.5rem);

  /* Elevation — low-opacity layered shadows over hairline borders */
  --shadow-sm: 0 1px 2px oklch(0 0 0 / 0.05);
  --shadow-md: 0 2px 4px oklch(0 0 0 / 0.05), 0 4px 12px oklch(0 0 0 / 0.06);

  /* Motion */
  --ease-out: cubic-bezier(0.2, 0, 0, 1);
  --ease-spring: linear(0, 0.4 9%, 0.9 25%, 1.05 45%, 1 60%, 1);
}
```

### Dark mode — adjust L & C, never naive invert

```css
/* Plain @theme lets a parent selector override the variable — that is the cascade */
[data-theme="dark"] {
  --color-bg: oklch(0.17 0.01 240);
  --color-surface: oklch(0.21 0.012 240);
  --color-fg: oklch(0.96 0.005 240);
  --color-fg-muted: oklch(0.70 0.01 240);
  --color-border: oklch(0.30 0.01 240);
  --color-accent: oklch(0.68 0.16 256); /* lift L, ease C for dark surfaces */
}
```

- Use plain `@theme` (cascades); use `@theme inline` **only** when the utility should NOT participate in the cascade.
- Clear a namespace with `--color-*: initial;` before redefining a custom palette.
- Gradient interpolation: `bg-linear-to-r/oklch` vs `/srgb` (oklab is default).
- **Browser floor (OKLCH default):** Safari 16.4+, Chrome 111+, Firefox 128+.

### When to swap styling engine

| Alternative | Pick it when… |
|---|---|
| **CSS Modules** | Zero-runtime, locally-scoped plain CSS; incremental migration off legacy |
| **vanilla-extract** | TypeScript-authored, **type-safe** zero-runtime tokens (`createTheme`/contracts) |
| **Tailwind v4** (default) | The AI-native choice — models + v0 + shadcn all speak it |

---

## 5. Component layers — own your code

Modern UI is **layered**: headless accessibility **primitives** → **shadcn/ui** copy-paste components styled with your themed tokens → your design system. Never ship shadcn/Tailwind defaults raw — theme them.

| Layer | Library | Current | Role |
|---|---|---|---|
| Copy-paste components | **shadcn/ui** | CLI 3.x | You own the source; not an npm dependency |
| Headless primitives | **Base UI** (default) | 1.x (Dec 2025) | Unstyled, accessible; render-prop composition |
| Headless primitives | **Radix UI** | mature | Original; `asChild`; ~131M weekly downloads |
| Accessibility hooks | **React Aria** (Adobe) | latest | Hooks-first; deepest a11y + i18n; most verbose |

### shadcn/ui — the AI-native distribution model

Not a library — a **code-distribution system**. `npx shadcn add button` copies source *into your repo*, so you own and customize it. Upstream changes are not auto-pulled (re-run `add` or merge).

```bash
npx shadcn@latest init                       # writes components.json
npx shadcn@latest add button card dialog form
```

**CLI 3.0 (Aug 2025)** added **namespaced registries** and a rewritten **MCP server** (zero-config, works with any registry):

```jsonc
// components.json — mix registries; deps auto-resolve
{
  "registries": {
    "@acme": "https://registry.acme.com/{name}.json",
    "@v0": "https://v0.dev/r/{name}.json"
  }
}
```

```bash
npx shadcn@latest add @acme/button @v0/dashboard
```

In **Dec 2025** shadcn added **Base UI** as a selectable primitive layer alongside Radix — new projects pick either; existing Radix projects need no migration.

### Base UI vs Radix — the call

- **Base UI** (creators of Radix, Floating UI, Material UI) hit **1.0 in Dec 2025**, full-time MUI-backed. Single dep `@base-ui/react`. Broader coverage (multi-select, combobox), **render-prop** composition instead of `asChild`.
- **Radix** pioneered headless React; still huge and reliable, momentum slowed post-WorkOS acquisition.
- **Verdict:** new projects → **Base UI**; existing Radix-based shadcn apps → stay, no rush.

```tsx
// Base UI — explicit render prop instead of Radix's asChild. Themed with token vars.
import { Dialog } from "@base-ui/react/dialog";

export function ConfirmDialog() {
  return (
    <Dialog.Root>
      <Dialog.Trigger render={<button className="btn-primary" />}>Delete</Dialog.Trigger>
      <Dialog.Portal>
        <Dialog.Backdrop className="fixed inset-0 bg-black/40 backdrop-blur-sm" />
        <Dialog.Popup className="fixed inset-1/2 -translate-1/2 rounded-(--radius-xl) bg-(--color-surface) p-6 shadow-md ring-1 ring-(--color-border)">
          <Dialog.Title className="text-(--color-fg) font-(--font-display)">Delete item?</Dialog.Title>
          <Dialog.Description className="text-(--color-fg-muted)">This cannot be undone.</Dialog.Description>
        </Dialog.Popup>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
```

**React Aria** — reach for it when accessibility is *contractual* (government/enterprise WAI-ARIA). Hooks (`useButton`, `useDialog`, `useTabs`) cover 40+ patterns with rigorous ARIA, i18n, RTL, locale-aware dates/numbers — more code, maximum control. Skip it for fast v0-style prototyping.

---

## 6. State & data — split the two axes

Server-state (async data from a backend) and client-state (UI/ephemeral) are different problems. Use the right tool for each.

### TanStack Query v5 — server-state

~20% smaller than v4; single-object API; Suspense first-class (`useSuspenseQuery`). Renames: `cacheTime`→`gcTime`, `keepPreviousData`→`placeholderData`, `loading`→`pending`. Ships a codemod + ESLint rule. **Always design loading / error / empty.**

```tsx
import { useQuery } from "@tanstack/react-query";

function Todos() {
  const { data, isPending, error } = useQuery({
    queryKey: ["todos"],
    queryFn: () => fetch("/api/todos").then((r) => r.json()),
  });

  if (isPending) return <TodosSkeleton />;                              // loading
  if (error) return <p role="alert" className="text-(--color-danger)">Couldn’t load todos.</p>; // error
  if (data.length === 0) return <EmptyState label="No todos yet" />;    // empty

  return <ul>{data.map((t) => <li key={t.id}>{t.title}</li>)}</ul>;
}
```

> In Next App Router: fetch **initial** data in Server Components / Server Actions; use TanStack Query for client-side caching, refetching, infinite lists, and mutations.

### Zustand v5 — client-state

User-satisfaction leader in State of React 2025. Zero-boilerplate `create()`, no Provider, selector subscriptions, `useSyncExternalStore` under the hood. **Always update immutably — new objects, never mutation.**

```ts
import { create } from "zustand";
import { persist } from "zustand/middleware";

type CartState = { items: string[]; add: (id: string) => void };

export const useCart = create<CartState>()(
  persist(
    (set) => ({
      items: [],
      add: (id) => set((s) => ({ items: [...s.items, id] })), // new array, no mutation
    }),
    { name: "cart" }
  )
);

// subscribe to a slice only — avoids needless re-renders
const count = useCart((s) => s.items.length);
```

**Alternatives:** Redux Toolkit for large, long-lived apps needing strict structure/debuggability; Jotai for atomic/derived graphs. For most UI-heavy apps, **Zustand + TanStack Query** covers both axes with minimal ceremony.

---

## 7. Build tooling — Rust everywhere

The 2026 theme: core JS tooling is rewritten in Rust/Zig. You rarely choose this directly — the framework picks it — but know the map.

| Tool | Current | Use for | Notes |
|---|---|---|---|
| **Turbopack** | stable in Next 16 | Next.js apps | Default bundler; Next-locked (no standalone yet) |
| **Vite 8 + Rolldown** | v8 (Rolldown RC) | Everything non-Next | Rolldown (Rust) replaces esbuild + Rollup |
| **Rspack** | stable | webpack migrations | Fastest cold start/prod; keeps webpack config |
| **Bun** | 1.2+ | installs, scripts, server bundles | Zig runtime; fastest installs; complements Vite |

**Guidance:** Rspack for migrations, Turbopack for Next.js shops, Vite for everything else. Run **Vite *with* Bun** (Bun for installs/scripts/runtime, Vite for HMR + framework plugins + prod build) — they complement, not compete. Treat vendor "10–30×" numbers as best-case; independent benchmarks show HMR roughly comparable between Turbopack and Vite+Rolldown.

```bash
# Minimal Vite 8 + React + Tailwind, driven by Bun
bun create vite@latest my-app -- --template react-ts
cd my-app && bun add tailwindcss @tailwindcss/vite
bun install && bun run dev
```

---

## 8. Fonts — self-hosted variable fonts

**Best practice:** self-host (privacy + GDPR + speed), prefer **variable fonts** (one file, all weights), expose a **CSS variable** so Tailwind/CSS consumes it. `next/font` preloads at build and uses `size-adjust` fallbacks to kill CLS. Per the taste layer: do **not** ship Inter/Roboto/system as the *only* font — pick a distinctive `--font-display` and pair it with a clean body face.

```tsx
// app/layout.tsx — auto self-hosted; zero runtime request to Google
import { Geist } from "next/font/google";
import localFont from "next/font/local";

const body = Geist({ subsets: ["latin"], variable: "--font-body" });
const display = localFont({ src: "./fonts/Hubot.woff2", variable: "--font-display" });

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en" className={`${body.variable} ${display.variable}`}>
      <body>{children}</body>
    </html>
  );
}
```

```css
/* Tailwind v4 — wire the CSS vars into the theme */
@theme {
  --font-body: var(--font-body), ui-sans-serif, system-ui, sans-serif;
  --font-display: var(--font-display), ui-sans-serif, system-ui;
}
```

**Fontsource** (framework-agnostic, version-locked) — fonts as npm dependencies with auto fallbacks. Use for non-Next setups (Vite/Astro/SvelteKit) or when you want fonts pinned in `package.json`:

```ts
import "@fontsource-variable/inter"; // self-hosted, no external request
```

---

## 9. Deployment — Vercel vs Cloudflare

| Platform | Pick it for | Model | Cost shape |
|---|---|---|---|
| **Vercel** | Next.js SSR, zero-config | **Fluid Compute** (warm Node, near-zero cold start, 2 vCPU/4GB) | Usage-based Pro; pricier at high traffic |
| **Cloudflare Workers** | Edge logic, global, cost | **V8 isolates**, 330+ cities, sub-5ms cold starts, full storage (KV/R2/D1/DO) | ~$5/mo for 10M req, no egress fees |

- **Vercel** is the default — deepest Next integration; **Fluid Compute** is ~1.2–5× faster SSR than Workers for render-heavy apps. Lock-in is **framework-shaped** (ISR/on-demand revalidation lean on Vercel infra).
- **Cloudflare Workers** wins on cost + cold-start distribution. Run Next via **OpenNext** (`@opennextjs/cloudflare`, Node.js runtime, supports Next 14/15/16). Lock-in is **storage-shaped** (Durable Objects non-portable; R2 is S3-compatible, D1 is SQLite).
- **Hybrid is the 2026 winner:** auth at the edge, SSR on Fluid Compute, CPU-heavy jobs on serverless.

```bash
# Vercel (default)
npx vercel deploy

# Cloudflare via OpenNext (cost-sensitive global edge)
bun add @opennextjs/cloudflare
npx @opennextjs/cloudflare && npx wrangler deploy   # auto-detects Next config
```

---

## 10. The AI-native angle — why *this exact* stack

The stack is chosen for what models **generate well** and what they can be **grounded** on. This is the load-bearing reason the defaults are not negotiable.

### Why models love it

- **shadcn/ui + Tailwind + Base UI/Radix** are the patterns models are most trained on. v0 uses shadcn/ui *by default* — every extra abstraction layer (wrapper components, overridden styles, mismatched APIs) makes output harder to predict. Owning the source lets models edit *real* code.
- **Copy-paste, not npm:** generated code drops into a repo that already owns identical component source — no version drift, no wrapper guessing.

### v0 (Vercel)

AI UI generator on Vercel's frontend-tuned model (React + Tailwind + shadcn/ui). Rebranded v0.dev → **v0.app** (Jan 2026); 6M+ developers (Mar 2026). 2026 added a sandbox runtime, a Git panel (branches/PRs from chat), DB integrations, and agentic workflows; it generates multi-page Next apps. Caveat: frontend-focused; one-click deploy is Vercel-only.

### Registry + MCP = grounding (the design-tokens bridge)

A **shadcn registry** is a distribution spec that passes your **design system context** (components, blocks, tokens) to AI models. It supports **MCP**, so the *same* registry grounds v0, Cursor, Windsurf, and Claude Code — any MCP-ready tool — in your real components and tokens. **The token contract is the bridge** between a human-maintained design system and AI codegen.

```jsonc
// registry.json — host at /r/registry.json (root) for MCP discovery
{
  "name": "acme-ui",
  "homepage": "https://registry.acme.com",
  "items": [
    {
      "name": "button",
      "type": "registry:component",
      "files": [{ "path": "ui/button.tsx", "type": "registry:component" }],
      "registryDependencies": ["@shadcn/button"]
    }
  ]
}
```

```bash
# Inspect the registry MCP server inside Claude Code
/mcp
```

**AI-consumable registry rules:** accurate dependency lists, `registryDependencies` for relationships, kebab-case names, concise descriptions so agents understand each item. The **Registry Starter** (Next + shadcn) ships an "Open in v0" button that deep-links `/r/<name>.json` into a v0 chat.

> **Security:** public/community MCP registries are a supply-chain target — review installed code; prefer private/internal registries for proprietary systems.

---

## 11. The full minimal setup (copy-paste)

```bash
# 1. Scaffold (App Router + TS + Tailwind v4 + ESLint + AGENTS.md)
npx create-next-app@latest acme-app
cd acme-app

# 2. Components — shadcn/ui (choose Base UI primitives when prompted)
npx shadcn@latest init
npx shadcn@latest add button card dialog form

# 3. Data + client state
bun add @tanstack/react-query zustand

# 4. Enable the new caching model -> set cacheComponents: true in next.config.ts

# 5. Deploy
npx vercel deploy
```

```ts
// next.config.ts — recommended defaults
const nextConfig = {
  cacheComponents: true,           // PPR + "use cache"
  // reactCompiler: true,          // enable once build-time cost is acceptable
};
export default nextConfig;
```

### Decision shortcuts

| If you are building… | Then ship… |
|---|---|
| App-first product, RSC | **Next 16** + the default stack (don't overthink it) |
| Marketing / docs / blog | **Astro 6** + Tailwind v4 + island components |
| No-lock-in type-safe full-stack | **TanStack Start v1** + TanStack Query on **Vite 8** |
| Migrating off Remix | **React Router v7** framework mode |
| Cost-sensitive global edge | Next via **OpenNext on Cloudflare Workers** |
| Contractual accessibility | **React Aria** at the primitive layer |
| Smallest possible bundle | **SvelteKit + Svelte 5** runes |

---

## Sources

- React v19 (Dec 5 2024) — https://react.dev/blog/2024/12/05/react-19
- React 19.2 (Oct 1 2025) — https://react.dev/blog/2025/10/01/react-19-2
- `use` API reference — https://react.dev/reference/react/use
- Next.js 16 release (Oct 21 2025) — https://nextjs.org/blog/next-16
- Next.js 16.2 / Turbopack (Mar 2026) — https://nextjs.org/blog/next-16-2-turbopack
- Next.js Upgrading: Version 16 — https://nextjs.org/docs/app/guides/upgrading/version-16
- Tailwind CSS v4.0 — https://tailwindcss.com/blog/tailwindcss-v4
- Tailwind CSS Theme variables — https://tailwindcss.com/docs/theme
- shadcn/ui Registry MCP Server — https://ui.shadcn.com/docs/registry/mcp
- shadcn/ui Namespaces — https://ui.shadcn.com/docs/registry/namespace
- shadcn CLI 3.0 + MCP changelog (Aug 2025) — https://ui.shadcn.com/docs/changelog/2025-08-cli-3-mcp
- shadcn/ui Tailwind v4 — https://ui.shadcn.com/docs/tailwind-v4
- Base UI (MUI) GitHub — https://github.com/mui/base-ui
- Base UI 1.0 (InfoQ, Feb 2026) — https://www.infoq.com/news/2026/02/baseui-v1-accessible/
- shadcn vs Base UI vs Radix (2026) — https://www.pkgpulse.com/guides/shadcn-ui-vs-base-ui-vs-radix-components-2026
- React Aria (Adobe) — https://react-spectrum.adobe.com/react-aria/
- TanStack Start v1 — https://tanstack.com/blog/announcing-tanstack-start-v1
- TanStack Query v5 — https://tanstack.com/blog/announcing-tanstack-query-v5
- TanStack Query v5 migration — https://tanstack.com/query/v5/docs/react/guides/migrating-to-v5
- React Router v7 (Remix) — https://remix.run/blog/react-router-v7
- Astro 6.0 (Mar 10 2026) — https://astro.build/blog/astro-6/
- Astro Content Layer deep dive — https://astro.build/blog/content-layer-deep-dive/
- Svelte 5 runes — https://svelte.dev/blog/runes
- Zustand (pmndrs) — https://github.com/pmndrs/zustand
- Rolldown 10-30x (The Register, Mar 2026) — https://www.theregister.com/software/2026/03/16/vite-team-claims-10-30x-faster-builds-with-rolldown/
- Bun vs Vite (2026) — https://www.pkgpulse.com/guides/bun-vs-vite-2026
- Next.js Font Optimization — https://nextjs.org/docs/app/getting-started/fonts
- OpenNext for Cloudflare — https://opennext.js.org/cloudflare
- Cloudflare Workers vs Vercel 2026 — https://www.kunalganglani.com/blog/cloudflare-workers-vs-vercel-2026
- v0 Design systems — https://v0.app/docs/design-systems
- AI-powered prototyping with design systems (Vercel) — https://vercel.com/blog/ai-powered-prototyping-with-design-systems
- shadcn/ui Registry Starter — https://vercel.com/templates/next.js/shadcn-ui-registry-starter
