#!/usr/bin/env bash
# Daily startup: never reset or reseed institutional data.
set -euo pipefail
source "$(dirname "$0")/common.sh"
require_docker
command -v setsid >/dev/null || { echo "Install util-linux (setsid) before starting." >&2; exit 1; }
source scripts/v3_common.sh
v3_activate
[[ -f .env ]] || { echo 'First run: ./scripts/setup_v3.sh (creates private .env without overwriting one).' >&2; exit 1; }
[[ -d frontend/node_modules && -d build-v3/python ]] || { echo 'Dependencies/build missing. Run ./scripts/setup_v3.sh first.' >&2; exit 1; }
python - <<'PY'
import socket
from libracore_api.config import Settings
s=Settings(ai_enabled=False)
for port in (s.api_port,5173):
    with socket.socket() as sock:
        try:sock.bind(('127.0.0.1',port))
        except OSError:raise SystemExit(f'Port {port} is already in use. Stop the earlier LibraCore terminal or the application using it, then retry.')
PY
echo 'Starting PostgreSQL and waiting for health (no data reset)…'
start_db
AI_ENABLED=false python -m libracore_api.database || { echo 'Migration failed. Check PostgreSQL settings in your private .env; do not reset your database.' >&2; exit 1; }
python -m libracore_api.v3.run & backend_pid=$!
frontend_pid=''
cleanup() {
  kill "$backend_pid" 2>/dev/null || true
  if [[ -n "$frontend_pid" ]]; then kill -- "-$frontend_pid" 2>/dev/null || true; fi
}
trap cleanup EXIT
trap 'exit 130' INT TERM
python - <<'PY'
import time,urllib.request
from libracore_api.config import Settings
s=Settings(ai_enabled=False)
for _ in range(40):
    try:
        urllib.request.urlopen(f'http://127.0.0.1:{s.api_port}/ready',timeout=1);break
    except Exception:time.sleep(.5)
else:raise SystemExit('Backend did not become ready. Check the error above, .env database connection and port availability.')
print(f'API ready: http://127.0.0.1:{s.api_port}/ready')
PY
echo 'Open http://localhost:5173 · Keep this terminal open. Ctrl+C stops the web/API processes.'
echo 'To stop PostgreSQL afterwards: ./scripts/stop_v3.sh (stored data is retained).'
export LIBRACORE_API_PORT="$(python -c 'from libracore_api.config import Settings; print(Settings(ai_enabled=False).api_port)')"
setsid npm --prefix frontend run dev -- --strictPort & frontend_pid=$!
wait -n "$backend_pid" "$frontend_pid"
