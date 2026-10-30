#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "$0")/v3_common.sh"
v3_activate

export LLM_PROVIDER=mock
export RUN_LIVE_AI_TESTS=false

# Reset is guarded by actual database name and only targets libracore_test.
python -m libracore_api.database --reset-test

export LIBRACORE_DEMO_PASSWORD="$(
    python -c 'import secrets; print(secrets.token_urlsafe(24))'
)"

python -m libracore_api.v3.bootstrap --demo --test

export DATABASE_URL="$(
    python -c 'from libracore_api.config import Settings; print(Settings(ai_enabled=False).db_url(True))'
)"

export PUBLIC_ORIGIN=http://localhost:5173
export API_HOST=127.0.0.1
export API_PORT=8000

backend_pid=""

wait_for_api() {
    python - <<'PY'
import time
import urllib.request

for _ in range(60):
    try:
        urllib.request.urlopen(
            "http://127.0.0.1:8000/ready",
            timeout=1,
        )
        break
    except Exception:
        time.sleep(0.5)
else:
    raise SystemExit("API readiness failed")
PY
}

start_backend() {
    python -m libracore_api.v3.run &
    backend_pid=$!
    wait_for_api
}

stop_backend() {
    if [[ -n "${backend_pid:-}" ]]; then
        kill "$backend_pid" 2>/dev/null || true
        wait "$backend_pid" 2>/dev/null || true
        backend_pid=""
    fi
}

cleanup() {
    stop_backend
    unset LIBRACORE_DEMO_PASSWORD DATABASE_URL
}

trap cleanup EXIT INT TERM

# Each viewport project gets a fresh backend process so the real
# production login rate limiter remains enabled without causing
# cross-project E2E interference.
start_backend
(
    cd frontend
    npx playwright test --project=desktop
)

stop_backend

start_backend
(
    cd frontend
    npx playwright test --project=mobile
)
