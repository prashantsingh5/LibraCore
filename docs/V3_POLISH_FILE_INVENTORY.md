# V3 polish — file-change inventory

Compared byte-for-byte with `LibraCore-V3-for-Work.zip`. Generated dependencies/builds/caches and private configuration are excluded. No original source file was removed. Each row identifies a modified/new file and its purpose. Original V1 algorithms, headers, bindings, native tests and migrations 001–003 are unchanged.

| File | Change | Reason |
|---|---|---|
| `.dockerignore` | Modified | Exclude runtime cover storage from image context. |
| `.env.example` | Modified | Configure optional media root and tested Gemini default. |
| `.gitignore` | Modified | Ignore runtime media; retain intentional verification logs. |
| `README.md` | Modified | Summarize polished workflows, safe startup, covers and verification boundaries. |
| `SETUP.md` | Modified | Step-by-step WSL2 setup, daily start/stop, upgrade and troubleshooting. |
| `database/migrations/004_book_covers.sql` | New | Add isolated cover metadata without rewriting historical migrations. |
| `docker-compose.prod.yml` | Modified | Persist cover media in an owned writable volume with explicit media root. |
| `docker/Dockerfile.backend` | Modified | Create owned media directory for the unprivileged backend. |
| `docker/nginx.conf` | Modified | Allow bounded raster uploads through proxy. |
| `docs/V3_API_CHANGES.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_ARCHITECTURE.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_AUTHENTICATION.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_AUTHORIZATION.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_COMPLETION_REPORT.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_DEPLOYMENT.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_FILE_INVENTORY.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_FRONTEND_DESIGN.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_LOCAL_VERIFICATION.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_POLISH_ENGINEERING.md` | New | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_POLISH_FILE_INVENTORY.md` | New | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_POLISH_REPORT.md` | New | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_SECURITY.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/V3_TESTING.md` | Modified | Document final polish behavior, rationale, measured results or historical-report navigation. |
| `docs/screenshots/acquisitions-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/acquisitions-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/activity-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/activity-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/administration-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/administration-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/assistant-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/assistant-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/budget-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/budget-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/catalog-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/catalog-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/create-cover-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dark-1024.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dark-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dark-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dark-768.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dashboard-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/dashboard-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/edit-cover-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/loans-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/loans-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/login-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/login-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/reports-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/reports-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/resource-1440.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/resource-390.png` | Modified | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/restricted-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/settings-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/settings-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/vendors-1440.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/screenshots/vendors-390.png` | New | Actual refreshed Chromium fixture capture for visual review, not DB proof. |
| `docs/verification-polish/README.md` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/baseline-frontend.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/baseline-native-exact.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/baseline-python-exact.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/e2e-discovery.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-browser.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-configure.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-ctest.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-demo.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-frontend.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-main-tests.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-native-build.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-production-build.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/final-python.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `docs/verification-polish/package-check.log` | New | Recorded baseline/final verification evidence with explicit runtime boundaries. |
| `frontend/e2e/journeys.spec.ts` | Modified | Update selectors and normal-currency/success assertions for real role journeys. |
| `frontend/public/art/default-cover.svg` | New | Original bundled artwork/default asset; no external image dependency. |
| `frontend/public/art/knowledge-hall.svg` | New | Original bundled artwork/default asset; no external image dependency. |
| `frontend/src/app/main.tsx` | Modified | Guard legacy administration alias consistently. |
| `frontend/src/components/charts/Utilization.tsx` | Modified | Display percentages and readable utilization context. |
| `frontend/src/components/feedback/DataView.tsx` | Modified | Readable enum/date/currency/status/percentage values in shared detail output. |
| `frontend/src/components/feedback/ResultCard.tsx` | New | Focused search/availability/quote/loan/order/vendor/budget results and technical disclosure. |
| `frontend/src/components/layout/Gate.tsx` | New | Consistent accessible 403 and return action. |
| `frontend/src/components/layout/Shell.tsx` | Modified | Readable role and corrected administration-alias breadcrumb; formatting. |
| `frontend/src/components/ui/ActionForm.tsx` | Modified | Currency input validation, department/select labels, asynchronous success and focused result context. |
| `frontend/src/components/ui/DepartmentSelect.tsx` | New | Named paginated department choices instead of opaque text entry. |
| `frontend/src/components/ui/ResourceCover.tsx` | New | Shared protected Book covers and bundled fallback. |
| `frontend/src/features/acquisitions/Acquisitions.tsx` | Modified | Focused quote/result, derived cancellation status and immutable history linkage. |
| `frontend/src/features/assistant/Assistant.tsx` | Modified | Render common structured tool results through shared cards. |
| `frontend/src/features/auth/Auth.tsx` | Modified | Replace plain login decoration with original bundled artwork. |
| `frontend/src/features/budgets/Budgets.tsx` | Modified | Named explicit scope lookup, labeled quotas and ordinary currency inputs. |
| `frontend/src/features/catalog/Catalog.tsx` | Modified | General/exact search, ranked offset pagination and covers. |
| `frontend/src/features/catalog/ResourceDetail.tsx` | Modified | Type-aware metadata, cover, availability and formatted vendor offers. |
| `frontend/src/features/catalog/ResourceForm.tsx` | Modified | Six type-aware forms, opening stock, optional cover/revert and safe partial-success retry. |
| `frontend/src/features/events/Events.tsx` | Modified | Integrate shared readable event presentation; consistent formatting. |
| `frontend/src/features/loans/Loans.tsx` | Modified | Title/date/status display and focused partial/full return outcomes. |
| `frontend/src/features/reports/Reports.tsx` | Modified | Explain actual all-time/current report scope and gross history totals. |
| `frontend/src/features/users/Users.tsx` | Modified | Named departments, account/role labels and clearer success feedback. |
| `frontend/src/features/vendors/Vendors.tsx` | Modified | Normal currency input and focused offer result. |
| `frontend/src/lib/presentation.ts` | New | Central money/date/status/enum/role/greeting formatting. |
| `frontend/src/pages/Dashboard.tsx` | Modified | Safe greeting and readable role/loan/date context. |
| `frontend/src/services/api.ts` | Modified | Add raw-file cover upload transport with existing session/CSRF handling. |
| `frontend/src/styles/index.css` | Modified | Original visual system, covers/results/modal/403 layout, themes and corrected disclosure contrast. |
| `frontend/src/types/api.ts` | Modified | Add structured titles, derived cancellation context and presentation fields. |
| `frontend/tests/components.test.tsx` | Modified | Preserve and adapt shared UI regression fixtures/assertions to polished presentation; consistent formatting. |
| `frontend/tests/pages.test.tsx` | Modified | Preserve and adapt shared UI regression fixtures/assertions to polished presentation; consistent formatting. |
| `frontend/tests/polish.test.tsx` | New | Regressions for type-aware forms, media, formatting, budgets, departments, results and route roles. |
| `frontend/tests/setup.ts` | Modified | Preserve and adapt shared UI regression fixtures/assertions to polished presentation; consistent formatting. |
| `frontend/visual-tests/visual.spec.ts` | Modified | Expand fixture screens, real tool result, mobile cover dialog, themes and role checks. |
| `frontend/vite.config.ts` | Modified | Allow daily startup to proxy configured API port; consistent formatting. |
| `pyproject.toml` | Modified | Pin Pillow and include trusted fallback asset in wheel. |
| `python/libracore_api/ai/assistant.py` | Modified | Invoke role-aware intent messaging before provider interpretation. |
| `python/libracore_api/ai/providers.py` | Modified | Safe clear provider outage messages; preserve no-fallback behavior. |
| `python/libracore_api/config.py` | Modified | Media storage setting and configurable Gemini 2.5 Flash default. |
| `python/libracore_api/schemas.py` | Modified | Strict bounded create-only opening holdings input. |
| `python/libracore_api/v3/assets/default-cover.svg` | New | Original bundled artwork/default asset; no external image dependency. |
| `python/libracore_api/v3/auth.py` | Modified | Show inactive-account guidance only after password verification. |
| `python/libracore_api/v3/main.py` | Modified | Protected cover fetch/upload/revert with fresh authorization. |
| `python/libracore_api/v3/media.py` | New | Safe raster validation/re-encoding, storage protocol and transactional metadata. |
| `python/libracore_api/v3/policy.py` | Modified | Explain restricted purchase requests without changing authorization authority. |
| `python/tests_v3/test_http.py` | Modified | Role-aware AI refusals/proposals and five-role API authorization regression. |
| `python/tests_v3/test_media.py` | New | Image decode/size/type/path validation, rollback, default/replace/revert and HTTP security. |
| `python/tests_v3/test_startup.py` | New | Non-destructive unavailable-Docker guidance smoke test. |
| `python/tests_v3/test_v3_database.py` | Modified | DB-gated exact search/opening stock, forward migration preservation and cover metadata tests. |
| `python/tests_v3/unit/test_auth_store.py` | Modified | Inactive correct-password versus invalid-password regression. |
| `requirements-v3-lock.txt` | Modified | Pin Pillow for reproducible raster validation. |
| `scripts/start_v3.sh` | New | Safe daily database/API/frontend startup with readiness and process cleanup. |
| `scripts/stop_v3.sh` | New | Stop database while retaining its volume. |
| `src/v2/Mapping.cpp` | Modified | Bound initial_holdings in native numeric validation. |
| `src/v2/PostgresRepository.cpp` | Modified | Exact identifier search, C++-validated opening stock, titles and derived summaries; preserve domain formulas. |
