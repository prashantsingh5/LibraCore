# Local verification on Windows 11 and WSL2

Use Ubuntu 24.04 in WSL2 with Docker Desktop WSL integration. Extract the V3 archive into a **new** directory, for example `~/projects/LibraCore-V3`; keep the frozen V2 checkout. Place your private `.env` there if desired. Setup preserves an existing `.env` and never prints it.

Install host development dependencies (the PostgreSQL server remains in Docker):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3-dev python3-venv libpq-dev libpqxx-dev nlohmann-json3-dev
# Install Node.js 22 LTS using your preferred version manager, then verify:
node --version
npm --version
docker info
docker compose version
cd ~/projects/LibraCore-V3
chmod +x scripts/*.sh
./scripts/setup_v3.sh
./scripts/test_v3.sh
./scripts/demo_v3.sh
```

If Chromium reports missing system libraries:

```bash
cd frontend
npx playwright install --with-deps chromium
cd ..
```

Open `http://localhost:5173`. The demo asks for a development-only password shared by five demo accounts: `student@libracore.test`, `faculty@libracore.test`, `librarian@libracore.test`, `manager@libracore.test`, `admin@libracore.test`. No password is shipped or logged. Demo provisioning resets these demo credentials and revokes their sessions, so do not use demo accounts for production users. The API is on `127.0.0.1:8000` and uses one worker.

## Upgrade an existing V2 database

Back up the database first. Migrations 001/002 remain byte-identical; 003 is additive and checksum-tracked. Existing users remain without credentials until provisioned.

```bash
source .venv/bin/activate
export PYTHONPATH="$PWD/build-v3/python:$PWD/python"
AI_ENABLED=false python -m libracore_api.database
AI_ENABLED=false python -m libracore_api.v3.bootstrap --admin-id library-admin --name 'Library Administrator' --email admin@your-institution.example
```

Use a new unused administrator ID. This command prompts privately for a password. Then sign in and use Administration to provision existing users. `PUBLIC_ORIGIN` must match the exact browser origin. All programmatic mutation clients must send that Origin and a CSRF token after login.

## Verification variants

- `./scripts/test_v3_offline.sh`: builds all native targets and runs non-DB Python/frontend checks; DB cases are explicitly skipped.
- `./scripts/test_v3.sh`: requires Docker, enables all database tests, runs native/Python/type/lint/component/build checks, then real browser E2E and axe checks. Any failure exits nonzero.
- `./scripts/e2e_v3.sh`: resets only the isolated database named `libracore_test`, creates fresh test-only accounts with an ephemeral random password and launches the secure backend plus Vite. Never run it against development data.
- `./scripts/benchmark_v3.sh 10000` (or 100000/1000000): destructive **test-DB-only** benchmark of generation, search, deep offset, cursor and pooled concurrent reads.

V3 Work-mode evidence does not replace these local full-stack checks. Never interpret skipped DB cases or browser transport fixtures as a passed PostgreSQL release gate.

## Final polish regression

See [SETUP](../SETUP.md) for first-time installation and everyday start/stop. After backing up the existing database and media:

```bash
./scripts/setup_v3.sh       # preserve .env; safely apply pending migration 004
./scripts/test_v3.sh        # isolated test DB, C++/Python/UI and real role journeys
cd frontend
npx playwright install chromium
npx playwright test --config=playwright.visual.config.ts
cd ..
docker compose -f docker-compose.prod.yml build
./scripts/start_v3.sh       # normal data; no reset/reseed
# Ctrl+C when finished
./scripts/stop_v3.sh
```

The full test script sets Mock and runs destructive fixtures only in the guarded isolated test database. Do not point test variables at institutional data. For production Compose startup use the HTTPS configuration in deployment docs. Manually verify migration 004 on a backup of your current V3 database, upload/replace/revert a cover across restarts, exact IDs/ISBN/ISSN, opening holdings, partial returns, cancellation linkage and all five roles. A live Gemini smoke test is optional, separately opted in, and must never be part of normal CI.
