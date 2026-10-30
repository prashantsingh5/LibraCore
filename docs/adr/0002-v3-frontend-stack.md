# ADR 0002 — React feature modules and shared design tokens

## Context
V3 must present the existing domain as a coherent product across roles, devices and themes.

## Decision
React/TypeScript/Vite, React Router, TanStack Query, Tailwind, Radix/shadcn-style primitives, Lucide and Recharts. Organize screens by feature and centralize transport/types/tokens. Use a restrained academic identity, not generic dense CRUD tables.

## Alternatives considered
Server-rendered templates would not match the requested frontend stack. Redux would duplicate TanStack Query state. A full animation library adds weight without a necessary workflow.

## Why selected
The requested stack supports typed workflows, accessible dialogs, query invalidation and reusable layout without a second business engine.

## Consequences
Client and backend contracts must be checked together. Monetary strings are displayed from server outputs, and pricing/quota calculations remain native.

## Future implications
Generate client types after native response schemas become fully formalized; add route code splitting if bundle measurement justifies it. Preserve fixture-vs-full-stack test distinctions.
