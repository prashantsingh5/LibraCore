# Local verification: Windows 11 → WSL Ubuntu 24.04 → Docker Desktop

## 1. Prepare an empty Linux project directory

Download `LibraCore-V2.zip` to Windows Downloads. Open an **Ubuntu WSL terminal**,
not Windows CMD/PowerShell. The ZIP contains a `LibraCore/` root directory.

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 python3-venv python3-dev \
  libpq-dev libpqxx-dev nlohmann-json3-dev pkg-config unzip
mkdir -p ~/projects
WIN_PROFILE=$(cmd.exe /c 'echo %USERPROFILE%' 2>/dev/null | tr -d '\r')
WIN_PROFILE_WSL=$(wslpath "$WIN_PROFILE")
```

If `~/projects/LibraCore` already exists, preserve it and choose a fresh extraction
directory. For a new project directory:

```bash
unzip "$WIN_PROFILE_WSL/Downloads/LibraCore-V2.zip" -d ~/projects
cd ~/projects/LibraCore
chmod +x scripts/*.sh
```

If your browser saved the archive elsewhere, change only the ZIP path. No source
code changes are needed. Do not build under `/mnt/c/Windows/system32`.

Only PostgreSQL client/development packages are installed above; no host database
server is installed.

## 2. Preserve your private configuration

The delivered ZIP intentionally excludes `.env` and API keys. Place your existing
local environment file at `~/projects/LibraCore/.env` before setup. If its download
name is `.env(1)`, rename that local copy to `.env`. Do not overwrite an existing
project `.env`. Restrict access:

```bash
chmod 600 .env
```

If you have no existing file, setup will create one from `.env.example`.
Missing values fall back to `.env.example`; environment variables take precedence.
For Gemini set `GEMINI_API_KEY` and a model available to your account. The example
model is configuration, not a guaranteed entitlement. No OpenAI key is required.
`LLM_PROVIDER=auto` chooses Gemini when only its key exists. An explicitly empty
model requires fixing the configuration value, not editing Python/C++.

Database defaults are `libracore` and `libracore_test` on localhost:5432. If you
change POSTGRES variables, blank URLs are derived safely. If URLs are nonempty,
make sure their host/port/database/user/password agree with Compose configuration.
Changing passwords in `.env` does not reset an already initialized PostgreSQL
volume's password. For a new installation use a fresh project/volume; do not
remove existing data to troubleshoot without a backup.

## 3. Verify Docker access from Ubuntu

Start Docker Desktop on Windows and enable its integration for Ubuntu-24.04.
Then run in WSL:

```bash
docker --version
docker compose version
docker info
docker run --rm hello-world
```

If `docker info` fails, fix Docker Desktop/WSL integration first. The scripts print
a useful error and do not replace PostgreSQL with a different engine.

## 4. Run setup and the complete suite

```bash
cd ~/projects/LibraCore
set -o pipefail
./scripts/setup_v2.sh 2>&1 | tee setup-v2.log
```

When using `tee`, enable pipeline error propagation before the commands:

```bash
set -o pipefail
./scripts/test_v2.sh 2>&1 | tee test-v2.log
```

Setup installs pinned Python packages, creates the extension, starts PostgreSQL
with health checking, creates the separate test database, migrates both DBs and
seeds development data. Existing `.env` is never overwritten. It is safe to rerun
setup; seed purchases use stable idempotency keys and migrations use checksums.

Full tests must finish with no failures and **no database-test skips**. The direct
C++ repository executable runs from the DB test fixture after reset/seed. Python
tests force Mock and block external SDK client construction. Real keys in `.env`
do not change normal test behavior.

## 5. Explicit component commands, if diagnosing a failure

```bash
source .venv/bin/activate
export PYTHONPATH="$PWD/build-v2/python:$PWD/python"
docker compose up -d --wait postgres
AI_ENABLED=false python -m libracore_api.database
AI_ENABLED=false python -m libracore_api.database --create-test --test
cmake -S . -B build-v2 -DLIBRACORE_V2=ON \
  -DPython3_EXECUTABLE="$PWD/.venv/bin/python" \
  -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
cmake --build build-v2 --parallel 4
ctest --test-dir build-v2 --output-on-failure
LLM_PROVIDER=mock LIBRACORE_RUN_DB_TESTS=1 python -m pytest -ra
```

The reset helper refuses any target except the separate `libracore_test`. Tests
reset that database before each case. Never run concurrent test/benchmark runs.

## 6. Benchmarks

```bash
set -o pipefail
./scripts/benchmark_v2.sh 10000 2>&1 | tee benchmark-db-10000.log
./scripts/benchmark_v2.sh 100000 2>&1 | tee benchmark-db-100000.log
./build-v2/bookmgmt_benchmark 10000
./build-v2/bookmgmt_benchmark 100000
./build-v2/bookmgmt_benchmark 1000000
```

Each DB benchmark resets only `libracore_test`, loads the requested synthetic count
plus six seeds and prints actual timings. One million persistent rows is optional:
`./scripts/benchmark_v2.sh 1000000`. No dataset loads during ordinary setup.

## 7. Launch API and inspect the demo

```bash
LLM_PROVIDER=mock ./scripts/demo_v2.sh
```

Visit `http://127.0.0.1:8000/docs` unless you changed API_PORT. Browse resources,
quote a purchase, submit with a unique request_id, borrow for U001, return the
loan, and inspect reports/events. Try `Cancel order 1` through the assistant:
it returns a confirmation proposal before any cancellation. Seed order 1 may be
uncancellable while its copies are lent; C++ will reject it appropriately.

For configured Gemini, run the demo without the `LLM_PROVIDER=mock` override.
For a minimal **optional live** interpretation smoke test:

```bash
RUN_LIVE_AI_TESTS=true LLM_PROVIDER=gemini ./scripts/live_ai_smoke.sh
```

This makes one live provider request and no business write. Quota failure is
reported separately. Do not run this to validate the offline test suite.

## 8. Freeze only after local verification

Retain setup/test/benchmark logs. Confirm all DB groups passed, no secrets were
added to source, and the API demonstration works. The Work delivery status is
“implementation complete; local database verification required,” not a freeze.
If local commands fail, return the error log after removing any private values.
Do not share `.env`, `docker compose config` output or database URLs with secrets.
No scripts commit, push, tag or destroy the development volume.
