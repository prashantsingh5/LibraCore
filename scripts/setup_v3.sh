#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
require_docker
for tool in cmake g++ python3 node npm; do command -v "$tool" >/dev/null || { echo "Missing $tool; see docs/V3_LOCAL_VERIFICATION.md" >&2; exit 1; }; done
python3 - <<'PY'
from pathlib import Path
import os
try:
    fd=os.open('.env',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    with os.fdopen(fd,'w') as f:f.write(Path('.env.example').read_text())
except FileExistsError:pass
PY
python3 -m venv .venv
source "$(dirname "$0")/v3_common.sh"
v3_activate
python -m pip install -r requirements-v3-lock.txt
python -m pip install --no-deps -e '.[test]'
v3_build
start_db
AI_ENABLED=false python -m libracore_api.database
AI_ENABLED=false python -m libracore_api.database --create-test --test
npm --prefix frontend ci
(cd frontend && npx playwright install chromium)
echo 'Setup complete. Run ./scripts/demo_v3.sh for demo accounts, or provision an administrator as documented.'
