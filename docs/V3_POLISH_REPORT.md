# LibraCore V3 — Final polish completion report

29 October 2026 · Source: `LibraCore-V3-for-Work.zip`

## Outcome

The updated source is packaged separately as `LibraCore-V3-polished.zip`. The original upload was not modified. No commit, tag, push or deployment was performed. This is ready for the user's final local regression, not a claim that unavailable PostgreSQL/Docker/live-provider checks passed.

## Baseline and preserved authority

An untouched extraction of the authoritative ZIP passed native CTest 3/3, Python 126 tests (45 database skips), and the original 26 frontend tests. Logs with `baseline-*-exact` identify the isolated native/Python baseline. An earlier overlapping/interrupted build was not treated as a source failure. The user's previous local 171-test/live-Gemini verification remains historical evidence, not a run performed here.

Byte comparison confirms original V1 `src` files, headers, bindings, all native tests and migrations 001–003 are unchanged. The repository adds search predicates, opening-stock validation and structured presentation context. No pricing/tax/budget/quota/loan formulas moved to Python, React or the LLM. PostgreSQL remains durable domain authority; existing immutable acquisition and cancellation records are retained.

## Implemented corrections

| Area | Result |
|---|---|
| Visual identity | Original bundled knowledge-hall artwork, editorial headings, richer theme-aware surfaces, catalog covers and responsive layouts |
| Optional Book covers | Authenticated fetch, authorized upload/replace/revert, safe raster decoding and WebP re-encoding, default fallback, durable storage and additive migration 004 |
| Resource forms | Six implemented resource types show relevant fields; readable enums, normal currency input, inline validation and explicit create-only opening holdings |
| Resource detail | Relevant metadata, formatted prices, title/cover and prominent holdings/borrowed/available counts |
| Search | Exact resource ID/ISBN/ISSN, title/author/publisher matching, exact-ID ranking and stable sorting; original full-text support retained |
| Lending | Titles alongside IDs, unambiguous dates, partial/complete return labels and focused results |
| Acquisitions | Concise eligibility/vendor/budget impact, derived cancelled status and immutable cancellation linkage |
| AI | Purpose-built common result cards, role-aware refusal and provider-failure messages, existing explicit confirmation invariant retained |
| Accounts and roles | Inactive guidance only after correct password, named department choices, readable role/greeting and account labels |
| Budget/reporting | Explicit scope submission, labeled quotas/currency, utilization percentages and honest current/all-time scope |
| Vendors and activity | Focused selected-offer success, normal prices, readable event labels, resource titles and timestamps |
| Route safety | Consistent 403 for both administration URLs; API role/ownership/department checks preserved |
| Daily run | Docker guidance, health waiting, safe migrations, no reseeding, one start command and clean stop without deleting data |
| Documentation | README, SETUP, versioned engineering/security/API/deployment/testing docs, file inventory and screenshots |

Opening holdings represent inventory already owned by the institution, not a paid acquisition: they do not debit budgets or invent purchase history. C++ validates stock within the resource-creation transaction. Resource save and optional cover save are separate transactions; cover failure after creation is explicitly shown and retry uses the saved resource.

## Measured verification

| Check | Observed result |
|---|---|
| Clean CMake configure/build, C++17 | PASS; GNU 13.3.0; native warnings enabled, no compiler warnings |
| CTest | 3/3 PASS: main, allocation-failure and mapping |
| Direct main tests | PASS: “All tests passed.” |
| Direct fault injection | PASS: 7 transaction scenarios, 58 allocation-failure points |
| Original demo | PASS, complete recorded output |
| Python unittest | 28/28 PASS |
| Python pytest | 151 PASS, 48 explicitly skipped for unavailable PostgreSQL |
| V3 Python coverage | 74% statements (650 statements, 167 missed) |
| Ruff / Mypy | PASS / PASS; 9 typed source files checked |
| Frontend Vitest | 50/50 PASS |
| Frontend coverage | 81.22% statements/lines; 71.61% branches; 62.23% functions |
| ESLint / TypeScript | PASS / PASS |
| Production Vite build | PASS |
| Chromium browser fixtures | 12/12 PASS; 1440, 1024, 768 and 390px widths; no serious/critical axe violations in checked views |
| Browser behaviors | Light/system/dark, mobile navigation focus, six-type form switching, cover revert and all-five-role administration route boundaries |
| Real E2E discovery | 14 journeys discovered; execution blocked by PostgreSQL absence |
| Startup | Shell syntax and unavailable-Docker smoke test PASS; full start/stop requires local Docker |
| Python wheel | PASS; trusted default-cover SVG included in package |
| Protected original files | Byte-identical to uploaded source as described above |

Evidence is in [verification-polish](verification-polish/). Coverage counts are observations, not substitutes for DB runtime tests. Browser fixtures explicitly stub HTTP and do not establish persistence or live AI success. Python media/API tests include real image decoding and filesystem behavior with controlled identity/storage doubles. New real-DB tests cover exact identifiers/opening holdings, forward migration preservation and cover metadata replace/clear; they are among the 48 skips.

Intermediate verification caught a low-contrast technical disclosure, now corrected. Interrupted native builds left a zero-byte object; the final clean rebuild and direct test runs passed. Earlier scratch attempts are not bundled as final evidence. The archived final logs reflect the successful current source. Npm environment/color notices are tooling notices, not suppressed source warnings.

## Environment-blocked checks

**NOT EXECUTED — ENVIRONMENT BLOCKED:** no Docker daemon/CLI or PostgreSQL server is available in this Work runtime. Therefore actual fresh/upgrade migrations, native PostgreSQL executables, V3 PostgreSQL tests, transaction/idempotency/concurrency cases, seed/demo database flow, full browser-to-DB role journeys, Docker builds/Compose health and media-volume durability require local WSL2 verification. Native DB executables compile, which is not a runtime pass. Migration 004 was reviewed as additive DDL; applied migration checksums are unchanged, but live migration safety must be confirmed on a backup locally.

Live Gemini was not called: no key was requested or used. Normal tests force Mock and block live clients. `auto` and configurable `gemini-2.5-flash` are the defaults; there is no silent runtime fallback. Keep the user's successful earlier Gemini result separate from this pass.

## Local acceptance commands

Back up PostgreSQL and the existing media directory/volume first. Keep the private `.env` and existing database volume. In WSL2 Ubuntu with Docker Desktop running:

```bash
chmod +x scripts/*.sh
./scripts/setup_v3.sh
./scripts/test_v3.sh
cd frontend
npx playwright install --with-deps chromium
npx playwright test --config=playwright.visual.config.ts
cd ..
docker compose -f docker-compose.prod.yml build
./scripts/start_v3.sh
# Ctrl+C stops the API/frontend started by this terminal
./scripts/stop_v3.sh
```

`test_v3.sh` uses the guarded isolated test DB and Mock. Do not run demo seeding or destructive test/reset commands against institutional data. Production startup additionally requires the HTTPS origin/secrets described in [deployment](V3_DEPLOYMENT.md).

Manually verify: current V3 backup migrates forward without losing stock/loans; cover upload/replace/revert survives restart; search exact IDs/ISBN/ISSN; opening holdings are available without budget changes; partial returns; quotes/purchase/cancellation; inactive credentials; all five roles and manager cross-department rejection; proposal/dismissal leaves zero mutation; confirmation executes once. Optionally repeat the opt-in Gemini smoke separately.

## Screenshots and visual review

Captured from the actual application with explicit representative fixtures. No personal credentials are included. Major pages were checked across four widths and representative captures visually inspected. Screenshots show appearance, not a live database.

- [Login](screenshots/login-1440.png), [overview](screenshots/dashboard-1440.png), [catalog](screenshots/catalog-1440.png)
- [Resource detail](screenshots/resource-390.png), [create cover](screenshots/create-cover-390.png), [edit cover](screenshots/edit-cover-390.png)
- [Lending](screenshots/loans-1440.png), [acquisitions](screenshots/acquisitions-1440.png), [budget](screenshots/budget-1440.png)
- [Vendor](screenshots/vendors-1440.png), [reports](screenshots/reports-1440.png), [activity](screenshots/activity-1440.png)
- [Administration](screenshots/administration-1440.png), [assistant results](screenshots/assistant-1440.png), [settings](screenshots/settings-1440.png)
- [Restricted access](screenshots/restricted-1440.png), [dark mobile](screenshots/dark-390.png)

## Source cleanliness and remaining boundaries

The ZIP export has no Git metadata; `git status`, `git diff` and `git ls-files` cannot describe its history. A byte-level comparison against the uploaded archive and a per-file [inventory](V3_POLISH_FILE_INVENTORY.md) are supplied instead. No source files from the upload were deleted. Generated caches, installed environments/dependencies, builds/binaries, coverage HTML, traces, runtime media, editor backups and private `.env` are excluded from the new archive; intentional verification logs and screenshots are included.

V3 intentionally remains a modular monolith with one API worker, process-local pending confirmations/rate limits, local filesystem media and no persisted assistant transcript. Old cover files need operational retention/garbage collection, and database plus media need coordinated backups. The role-aware natural-language refusal is a narrow UX aid; typed authorization is always authoritative. There is no new distributed infrastructure or change to core financial semantics.

V3 POLISH PASS COMPLETE — READY FOR FINAL LOCAL REGRESSION
