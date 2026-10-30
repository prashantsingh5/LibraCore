# V3 polish engineering notes

## Scope and authority

The source of truth is `LibraCore-V3-for-Work.zip`. Changes preserve the modular monolith: browser → authenticated FastAPI/typed tools → pybind11 → C++ → PostgreSQL. V1 domain algorithms, bindings and historical migrations 001–003 are unchanged. Repository changes add search predicates and structured context; pricing, budgets, quotas and loan validation continue through existing C++ rules.

## Catalog and inventory

Creation now accepts opening holdings for already-owned stock. The C++ repository validates this through a temporary Catalog and `addHoldings` before persisting it in the same transaction as resource creation. Failure rolls back both. This operation does not spend money, select a vendor or fabricate an acquisition record. Later paid stock enters through acquisitions. The edit form cannot overwrite holdings.

General search retains PostgreSQL full-text matching and adds exact ID/ISBN/ISSN plus escaped title/publisher substring predicates. Exact resource ID takes priority, followed by the selected deterministic sort. Ranked queries use offsets; plain ID browsing retains cursors. Literal `%`, `_` and backslash handling uses the existing pattern escaping.

## Covers and migration

Migration 004 adds `resource_covers`, keyed to resources with cascading removal and a constrained immutable WebP storage key. Existing records require no backfill. The `MediaStorage` protocol separates filesystem storage from metadata and permits a later storage adapter. Cover metadata is nonfinancial and uses the identity/application pool; inventory and finance remain C++.

Authenticated GET serves a default when there is no reference or file. PUT validates/decodes/re-encodes raster bytes, writes a UUID file and commits its metadata; DELETE removes the reference. Old immutable files remain for in-flight readers. Backups and eventual orphan cleanup are operational responsibilities. Resource and cover save are explicitly separate, with a visible partial-success/retry state.

## Shared UI

Presentation helpers format currencies, dates, roles, event/status enums and percentages. Amount entry converts decimal strings to integer minor units without floating multiplication; C++ remains authoritative. Resource forms expose only implemented subtype fields. Named department choices replace opaque IDs. Budget scope is applied explicitly rather than queried on every keystroke. Reports describe current snapshots and all-time purchase/cancellation history without implying an unimplemented year filter.

Focused result cards show availability, purchase eligibility and budget impact, loan/return outcomes, cancellation linkage and vendor offers. Original audit records remain intact; derived current status makes cancellation visible. Technical disclosures are optional. Original bundled artwork, cover thumbnails and theme-aware surfaces improve desktop/mobile composition without image-service dependencies.

## Security and AI

RBAC and fresh mutation checks remain server-side. Invalid login credentials retain a generic response; inactive-account guidance appears only after password verification. Student/Faculty purchase phrasing receives a useful refusal through a narrow precheck; typed tool policy remains authoritative even when phrasing does not match that precheck.

`auto` chooses a configured real provider or Mock when no credentials exist; Gemini defaults to the user-tested `gemini-2.5-flash`. There is no runtime fallback from provider failure to Mock. The existing single-use, five-minute, session-bound immutable confirmation remains; proposal/dismissal cannot mutate the domain.

## Daily operations and limits

Setup is separate from daily start. `start_v3.sh` verifies Docker, checks ports, starts/waits for PostgreSQL, applies checksummed migrations and starts API/frontend. It never reseeds. Ctrl+C stops processes it started; `stop_v3.sh` stops PostgreSQL without deleting volumes. The Windows/WSL guide explains first-time dependencies and troubleshooting.

Intentional limits: local single-node media, no automatic orphan garbage collector, one API worker, in-memory pending confirmations/rate limits, no persistent assistant transcript, no distributed search infrastructure. Cover decode is bounded but uploads are not a malware-scanning service. Browser fixture tests prove presentation, not database integration; local PostgreSQL/Docker regression remains essential.
