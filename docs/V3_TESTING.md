# V3 testing strategy

The existing V1 domain/fault tests and V2 mapping, binding, provider, transaction and concurrency suites are retained. Two V2 test assumptions were intentionally generalized: migration count follows the migration manifest instead of hardcoding two, and the direct C++ DB executable path can use `LIBRACORE_BUILD_DIR` (default remains build-v2). Neither changes a business-rule assertion.

| Layer | Implementation | Scope |
|---|---|---|
| C++ regression | `tests/test_*.cpp` | Original V1 behavior |
| Allocation failure | `tests/fault_injection.cpp` | Seven scenarios, 58 injection points |
| Native mapping / DB | `tests/v2/` | Rehydration and persistent domain semantics |
| Python pure unit | `python/tests_v3/unit` | Passwords, CSRF, limiter, role matrix, ownership, auth-store SQL doubles, confirmation races |
| Secure HTTP | `python/tests_v3/test_http.py` | Cookie auth gates, forbidden direct calls, CSRF/Origin, AI permissions, namespace and headers with explicit identity/core doubles |
| PostgreSQL integration | `python/tests_v3/test_v3_database.py` | Fresh/upgrade migrations, hash-only sessions, expiry/revocation/activity, password changes/races, guard ordering, cursor pages and pool concurrency |
| Frontend unit/component | `frontend/tests` | Accessible controls, API transport, forms, dialogs, feature screens, role navigation and confirmation |
| Browser component | `frontend/visual-tests` | Fixture-based UI, four widths, axe and screenshots |
| Real browser E2E | `frontend/e2e` | Student/Faculty/Librarian/Manager/Admin/AI journeys against secure API/C++/PostgreSQL |

Test doubles verify orchestration, not PostgreSQL behavior. Static SQL inspection and successful native compilation do not establish migration or transaction runtime success. All database tests are explicitly skipped unless `LIBRACORE_RUN_DB_TESTS=1`; the full local script sets it and fails on real failures. V3 database tests reuse only the guarded `libracore_test` database and never run concurrently with other resetting suites.

Frontend tests include error/loading/empty states, resource cards, budget values, admin forms and single-use confirmation behavior. Automated axe checks exercise the actual rendered interface and do not disable color-contrast rules. Real E2E covers both desktop and mobile. Traces are kept on local failures but excluded from the source archive because entered test passwords may appear in them.

## Commands

```bash
python -m unittest discover -s python/tests_v3/unit -v
python -m pytest python/tests python/tests_v3 -ra --cov=libracore_api.v3 --cov-report=term-missing
python -m ruff check python/libracore_api/v3 python/tests_v3
python -m mypy python/libracore_api/v3 --follow-imports=silent --ignore-missing-imports
npm --prefix frontend run typecheck
npm --prefix frontend run lint
npm --prefix frontend run coverage
npm --prefix frontend run build
cd frontend && npx playwright test --config=playwright.visual.config.ts
```

Use the scripts to configure PYTHONPATH and build native dependencies first. Native warnings remain enabled (`-Wall -Wextra -Wpedantic`); no warning suppression was added. Coverage reports show observed statements/branches/functions separately. Suggested coverage targets are not substituted for meaningful DB/E2E execution. Final measured coverage and all execution boundaries are recorded in `V3_COMPLETION_REPORT.md`.

## Final polish verification (29 October 2026)

The expanded suite passes 151 Python tests (48 PostgreSQL skips), 28 unittest cases, 50 frontend tests and 12 Chromium fixture tests. Native CTest remains 3/3 and fault injection remains 7 scenarios/58 points. Python V3 coverage is 74%; frontend statements are 81.22%. New cases cover raster validation/storage/authorization, exact identifiers and opening stock (DB-gated), type-aware forms, presentation, department selection, role-aware AI proposals/refusals, account inactivity, consistent 403 and startup failure guidance. See [polish report](V3_POLISH_REPORT.md) for logs, browser scope and the exact local-only regression commands.
