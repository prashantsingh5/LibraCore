# CI and release workflow

`.github/workflows/ci.yml` runs on push and pull request with read-only repository permissions. It uses Ubuntu 24.04, Python 3.12, Node 22 and a PostgreSQL 16.6 service. pip/npm caches are keyed by dependency manifests; CMake build outputs are not shared across incompatible runners.

The job builds the C++ engine, repository and binding, runs CTest/fault tests, Python unittest and pytest including the isolated DB group, Ruff, mypy, frontend ESLint/typecheck/component coverage/production build, real Playwright journeys and accessibility checks. It then builds both production images. Coverage and browser reports are uploaded even on failure. CI credentials are disposable service-container values, never real deployment secrets.

`LLM_PROVIDER=mock` and `RUN_LIVE_AI_TESTS=false` are fixed in CI. No live provider smoke test is part of CI. The E2E runner generates a random test password in memory and never prints it.

The workflow validates a deployable artifact but does not push images, deploy to a host, create a Git tag or modify repository history. Deployment is an explicit operator action after successful verification. The GitHub workflow could be statically inspected here, but it has not been executed on GitHub by this task.
