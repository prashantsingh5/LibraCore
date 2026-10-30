# Frontend design and interaction contract

The visual identity combines midnight blue navigation, a warm neutral canvas, restrained green accents and typographic book-cover motifs. The implementation is React/TypeScript/Vite with React Router, TanStack Query, Tailwind CSS, Radix-backed shadcn-style button/dialog primitives, Lucide icons and Recharts. There is no Redux or additional animation framework.

Design variables live in `src/styles/index.css`: surfaces/text/borders/accent, radii and transition speed. Layouts use a spacing rhythm of roughly 8/16/24/32 pixels, soft borders and modest shadows. Semantic badges retain written labels (Active, Returned, Overdue, Approved, Rejected) rather than color-only meaning.

The shell has role-aware navigation, breadcrumbs, catalog search access, theme selection, account controls and an assistant entry point. Theme choices are Light/Dark/System; only this UI preference is stored in localStorage. Server state stays in TanStack Query with 30-second stale time and targeted mutation invalidation.

## Workflows

- Login uses a dedicated two-column identity/form composition; mobile prioritizes the form.
- Dashboard combines personal loans, due-date alerts, budget state for financial roles, quick navigation and real catalog results. Financial roles also receive recent activity/acquisitions.
- Catalog supports cards/list, title/author/identifier/category/year/availability filters and sorting. ID ordering uses cursor pages; other sorts use offset pages.
- Details include availability, subtype metadata, authorized offers and a borrower-aware lending flow. Resource editors retain subtype fields, DRM and format.
- Lending separates active/returned/history and permits partial returns. Staff can browse all borrowers.
- Acquisition previews are C++ results, including costs, vendor selection and budget/quotas before/after; confirmation does not calculate money in JavaScript.
- Budget pages show exact currency-aware strings and percentage charts with accompanying text. Administration provisions accounts/credentials and changes access; no public signup.
- Assistant results use structured cards and explicit confirmation controls, not raw JSON, HTML from the model or browser confirm dialogs.

## Accessibility and responsiveness

Critical screens are checked at 1440, 1024, 768 and 390 pixels. The mobile sidebar opens over the workspace; grids collapse to cards instead of shrinking tables. Dialogs use Radix focus handling, forms use visible labels, loading/error states use status/alert semantics, and the page provides a skip link and visible focus outlines. Reduced-motion preferences disable transitions/animations. All charts have equivalent textual values.

Automated axe checks target serious/critical violations, with manual screenshot review. These checks support the WCAG 2.2 AA target but are not a formal accessibility certification. `frontend/visual-tests` uses explicit transport fixtures to exercise rendering without PostgreSQL; `frontend/e2e` exercises the real API and database. Screenshot filenames in `docs/screenshots` are **browser component fixture captures**, not proof of a live production database.

## Maintenance

Typed API shapes are centralized in `src/types/api.ts`; the fetch client handles credentials, CSRF and structured errors. Types are manually maintained rather than generated because the native V2 boundary has flexible structured outputs. Optional native fields remain rendered as semantic key/value details. Run typecheck, component tests and the real E2E suite when changing a contract.

## Polish presentation layer

`lib/presentation.ts` centralizes currency display/input conversion, readable dates, role names, greetings, enum and loan/order status labels. `ResultCard` renders common HTTP/AI outcomes; optional technical disclosures retain structured detail. `ResourceCover`, `ResourceForm` and `DepartmentSelect` share cover fallback, six type-aware forms and named department choices. Opening stock and purchase costs are explicitly separate concepts.

Bundled original `knowledge-hall.svg` and `default-cover.svg`, editorial headings, warm surfaces and teal/gold accents give the workspace a consistent identity without external assets. Theme variables preserve light/dark/system modes, focus and reduced motion. Browser screenshots are generated with explicit fixture data, not presented as live PostgreSQL evidence.
