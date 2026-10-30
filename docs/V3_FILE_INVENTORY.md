> Historical V3 implementation record. For the final polish pass, see [completion report](V3_POLISH_REPORT.md) and [changed-file inventory](V3_POLISH_FILE_INVENTORY.md).

# V3 file inventory

Compared byte-for-byte with the authoritative `LibraCore-V2(3).zip`. No original files were removed. V1 domain sources/tests and migrations 001/002 are unchanged. No Git history was present in the supplied archive.

| File | Change | Reason |
|---|---|---|
| `.env.example` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `.gitignore` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `CMakeLists.txt` | Modified | Build the V3 native database regression target with warnings and threads. |
| `README.md` | Modified | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `include/libracore/Repository.h` | Modified | Share pool ownership while retaining repository interface. |
| `pyproject.toml` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `python/libracore_api/ai/assistant.py` | Modified | Filter and reauthorize tools; bind confirmations to authenticated sessions. |
| `python/libracore_api/config.py` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `python/libracore_api/run.py` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `python/libracore_api/schemas.py` | Modified | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `python/tests/test_database.py` | Modified | Follow migration manifest count and configurable build directory; preserve business assertions. |
| `src/v2/Mapping.cpp` | Modified | Bound event cursor numeric input. |
| `src/v2/PostgresRepository.cpp` | Modified | Bounded RAII connection pool; cursor search/events; structured lists, vendor metadata and quotes. |
| `.dockerignore` | Created | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `.github/workflows/ci.yml` | Created | Reproducible setup, tests, CI or container deployment. |
| `database/migrations/003_authentication.sql` | Created | Add credentials, hashed sessions and security-event storage without modifying V2 migrations. |
| `docker-compose.prod.yml` | Created | Reproducible setup, tests, CI or container deployment. |
| `docker/Dockerfile.backend` | Created | Reproducible setup, tests, CI or container deployment. |
| `docker/nginx.conf` | Created | Reproducible setup, tests, CI or container deployment. |
| `docs/V2_README.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_API_CHANGES.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_ARCHITECTURE.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_AUTHENTICATION.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_AUTHORIZATION.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_CI_CD.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_COMPLETION_REPORT.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_DEPLOYMENT.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_FILE_INVENTORY.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_FRONTEND_DESIGN.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_LOCAL_VERIFICATION.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_SECURITY.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/V3_TESTING.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/adr/0001-v3-authentication-strategy.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/adr/0002-v3-frontend-stack.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/adr/0003-v3-role-permission-model.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/adr/0004-v3-ai-authorization.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/adr/0005-v3-deployment-model.md` | Created | V3 architecture, operating guidance, release evidence or preserved V2 documentation. |
| `docs/screenshots/README.md` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/assistant-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/assistant-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/budget-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/budget-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/catalog-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/catalog-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dark-1024.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dark-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dark-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dark-768.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dashboard-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/dashboard-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/loans-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/loans-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/login-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/login-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/resource-1440.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/screenshots/resource-390.png` | Created | Requested UI screenshots with synthetic-fixture provenance. |
| `docs/verification-v3/baseline-build.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/baseline-configure.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/baseline-ctest.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/baseline-fault.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/baseline-main.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/baseline-python.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/browser-components.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/browser-install-final.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/browser-install-retry.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/browser-install.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/combined-python.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/e2e-discovery.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-benchmark-10000.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-benchmark-100000.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-benchmark-1000000.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-browser-components.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-build.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-configure.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-ctest.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-demo.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-frontend-static.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/final-offline.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/frontend-build.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/frontend-tests.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/git-check.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/native-build.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/native-configure.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/pytest-v3.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/source-comparison.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `docs/verification-v3/unittest.log` | Created | Recorded verification evidence; failures/blocked attempts remain explicit. |
| `frontend/Dockerfile` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/components.json` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/e2e/journeys.spec.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/eslint.config.js` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/index.html` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/package-lock.json` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/package.json` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/playwright.config.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/playwright.visual.config.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/app/main.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/charts/Utilization.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/feedback/DataView.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/feedback/States.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/layout/Shell.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/ui/ActionForm.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/ui/button.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/components/ui/dialog.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/acquisitions/Acquisitions.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/assistant/Assistant.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/auth/Auth.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/auth/Settings.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/budgets/Budgets.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/catalog/Catalog.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/catalog/ResourceDetail.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/catalog/ResourceForm.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/events/Events.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/loans/Loans.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/reports/Reports.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/users/Users.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/features/vendors/Vendors.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/lib/utils.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/pages/Dashboard.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/services/api.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/styles/index.css` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/src/types/api.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/tests/components.test.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/tests/pages.test.tsx` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/tests/setup.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/tsconfig.json` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/visual-tests/visual.spec.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `frontend/vite.config.ts` | Created | React application, design system, dependencies, component/browser tests or production serving. |
| `python/libracore_api/v3/__init__.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/auth.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/benchmark.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/bootstrap.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/main.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/policy.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/run.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/libracore_api/v3/security.py` | Created | V3 authentication, authorization, secure HTTP, bootstrap or benchmark implementation. |
| `python/tests_v3/conftest.py` | Created | Security, policy and real PostgreSQL regression verification. |
| `python/tests_v3/test_http.py` | Created | Security, policy and real PostgreSQL regression verification. |
| `python/tests_v3/test_v3_database.py` | Created | Security, policy and real PostgreSQL regression verification. |
| `python/tests_v3/unit/test_auth_store.py` | Created | Security, policy and real PostgreSQL regression verification. |
| `python/tests_v3/unit/test_security.py` | Created | Security, policy and real PostgreSQL regression verification. |
| `requirements-v3-lock.txt` | Created | V3 configuration/dependencies, secure default runner, pagination inputs or generated-file exclusions. |
| `scripts/benchmark_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/demo_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/dev_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/e2e_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/setup_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/test_v3.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/test_v3_offline.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `scripts/v3_common.sh` | Created | Reproducible setup, tests, CI or container deployment. |
| `tests/v3/test_postgres_v3.cpp` | Created | Security, policy and real PostgreSQL regression verification. |
