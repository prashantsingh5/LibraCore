# LibraCore V3 — Install once, start with one command

These instructions target **Windows 11 + Ubuntu 24.04 in WSL2 + Docker Desktop**. Run project commands in the **Ubuntu terminal**, not Windows Command Prompt. Keep the frozen project copy as a backup. The supplied ZIP contains source, not your database or credentials.

## 1. Prepare Windows (first time only)

1. Open PowerShell as Administrator and run `wsl --install -d Ubuntu-24.04`. Restart Windows if asked.
2. Open Ubuntu from Start and create its Linux username/password. The password is invisible while typing; this is normal.
3. Install Docker Desktop from Docker's official website and open it. In Settings → General, enable the WSL 2 engine. In Settings → Resources → WSL Integration, enable Ubuntu-24.04, then Apply & Restart. Wait until Docker Desktop reports that its engine is running.
4. In Ubuntu, check `docker info` and `docker compose version`. If either fails, return to Docker Desktop and check WSL integration.

## 2. Put the project in Ubuntu

Extract `LibraCore-V3-polished.zip`. Copy its `LibraCore` folder into your Linux home, for example `~/projects/LibraCore-V3`. Prefer Linux home over `/mnt/c` for faster builds and consistent file permissions. Enter it:

```bash
cd ~/projects/LibraCore-V3
chmod +x scripts/*.sh
sudo apt-get update
sudo apt-get install -y build-essential cmake python3-dev python3-venv libpq-dev libpqxx-dev nlohmann-json3-dev curl ca-certificates
```

Install NVM using the instructions in the official `nvm-sh/nvm` repository, then reopen Ubuntu and run:

```bash
nvm install 22
nvm use 22
node --version
npm --version
```

NVM manages Node.js for your Linux user. Node installed only in Windows is insufficient.

## 3. First-time setup

Keep Docker Desktop open, then run:

```bash
./scripts/setup_v3.sh
```

Setup creates a Python virtual environment (`.venv`), installs pinned packages, compiles C++, installs the frontend and Chromium, starts PostgreSQL and applies migrations. It creates `.env` from `.env.example` only if `.env` does not exist. **It never overwrites your existing `.env`.** Downloads may take several minutes.

If browser dependencies are missing:

```bash
cd frontend
npx playwright install --with-deps chromium
cd ..
```

### Existing installation upgrade

Back up your database and media before replacing application files. Keep your `.env` and PostgreSQL volume. Copy this source into a separate directory if you want to compare versions. Install updated dependencies with setup, then start normally. Migration 004 only creates a cover-metadata table; migrations 001–003 are unchanged and no record backfill/reset is needed.

**Never run database reset or E2E scripts against development data.** Supplied destructive tests enforce the isolated database name `libracore_test`.

## 4. Configure your private environment

Open `.env` with `nano .env` (Ctrl+O, Enter to save; Ctrl+X to exit) or VS Code. Do not share `.env`, screenshots of its secrets, passwords or API keys.

Defaults use local PostgreSQL, API port 8000 and website port 5173. Keep `PUBLIC_ORIGIN=http://localhost:5173` for the normal workflow. The database password must match the database volume's initialized password; changing `.env` alone does not change an existing PostgreSQL account.

AI is optional:

```dotenv
LLM_PROVIDER=auto
GEMINI_MODEL=gemini-2.5-flash
GEMINI_API_KEY=
LLM_FALLBACK_ON_ERROR=false
MEDIA_ROOT=var/media
```

With no provider keys, `auto` selects deterministic Mock. If an OpenAI key is configured, the existing auto-selection gives OpenAI precedence; set `LLM_PROVIDER=gemini` to explicitly select Gemini. A real provider needs its valid key/model. Failed live requests return an error; they never silently become successful Mock responses. Restart the application after changing `.env`.

## 5. Create access once

For a **demo installation**, run:

```bash
./scripts/demo_v3.sh
```

Enter a development-only passphrase when prompted. The five demo emails are `student@libracore.test`, `faculty@libracore.test`, `librarian@libracore.test`, `manager@libracore.test` and `admin@libracore.test`. Demo provisioning is explicit: it seeds representative data and resets demo credentials. **Do not run demo setup as a daily command on institutional data.**

For a normal installation, provision an administrator once:

```bash
source .venv/bin/activate
export PYTHONPATH="$PWD/python:$PWD/build-v3/python"
AI_ENABLED=false python -m libracore_api.v3.bootstrap --admin-id library-admin --name 'Library Administrator' --email admin@your-institution.example
```

Replace the email with yours. The command prompts privately for a password. Sign in and create departments, users, budgets and resources as appropriate.

## 6. Every later day

1. Open Docker Desktop and wait for its engine.
2. Open Ubuntu, enter the project directory, and run:

```bash
./scripts/start_v3.sh
```

This checks Docker and ports, starts/waits for PostgreSQL, safely applies pending migrations, starts the API, waits for readiness and starts the frontend. **It does not reset or reseed data.** Open **http://localhost:5173** and keep the terminal open.

- API readiness: http://127.0.0.1:8000/ready
- Website: http://localhost:5173
- Press **Ctrl+C** in the startup terminal to stop its API/frontend.
- Then run `./scripts/stop_v3.sh` to stop PostgreSQL. This retains its volume and covers.

Uploaded covers live under `var/media` by default. They survive application restarts; back up this directory together with PostgreSQL. Production Compose uses a persistent named media volume. Removing source folders, Docker volumes or media manually can destroy data—normal start/stop scripts do none of those things.

## 7. Verify your local release

```bash
./scripts/test_v3_offline.sh
./scripts/test_v3.sh
cd frontend
npx playwright test --config playwright.visual.config.ts
cd ..
docker compose -f docker-compose.prod.yml build
```

The offline command skips PostgreSQL tests explicitly. The full command uses only the isolated test database and exercises real native persistence plus browser role journeys. Ordinary tests force Mock; they never need Gemini credits. See `docs/V3_POLISH_REPORT.md` for what was executed in Work and what remains local-only.

## Common problems

| Symptom | What to do |
|---|---|
| Docker unavailable | Open Docker Desktop; enable Ubuntu WSL integration; retry `docker info`. |
| Connection refused on port 5432 | Run `./scripts/start_v3.sh`; inspect `docker compose ps`. Do not reset the database. |
| Database password authentication fails | Match `.env` to the password that initialized the existing volume. Ask the database administrator before rotating it. |
| Port 8000/5173 already in use | Stop the earlier LibraCore terminal with Ctrl+C, or stop the application occupying that port. Avoid starting two copies. |
| Login returns 503 | Check `/ready`, PostgreSQL health and backend terminal output. Keep credentials out of support screenshots. |
| Login says inactive | Correct credentials were verified, but the account is disabled. Ask an administrator to reactivate it. |
| AI temporarily unavailable | Library data has not changed from that failed request. Retry later or explicitly choose Mock for development, then restart. Never assume a failed live request succeeded. |
| Cover rejected | Use an actual JPG/PNG/WEBP under 5 MB, no more than 16 million pixels. Renaming a different format is insufficient. |
| Cover directory not writable | Check ownership of `MEDIA_ROOT`; the application user needs write access. Do not use world-writable permissions. |
| Migration checksum changed | Restore the original applied migration from your matching release. Do not edit history or delete migration receipts. |
| Browser missing libraries | Run `npx playwright install --with-deps chromium` from `frontend`. |

Production is a separate operation requiring HTTPS, strong secrets, backups and local verification. Read `docs/V3_DEPLOYMENT.md`; the daily script is for local development, not public hosting.
