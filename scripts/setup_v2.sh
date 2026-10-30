#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
require_docker
for tool in cmake g++ python3; do
  command -v "$tool" >/dev/null || { echo "Missing $tool; see docs/V2_LOCAL_VERIFICATION.md" >&2; exit 1; }
done
# Exclusive creation preserves any user configuration, even during concurrent setup.
python3 - <<'PY'
from pathlib import Path
try:
    with Path('.env').open('x') as f:f.write(Path('.env.example').read_text())
except FileExistsError:pass
PY
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-lock.txt
python -m pip install --no-deps -e '.[test]'
export PYTHONPATH="$PWD/build-v2/python:$PWD/python${PYTHONPATH:+:$PYTHONPATH}"
cmake -S . -B build-v2 -DLIBRACORE_V2=ON -DPython3_EXECUTABLE="$PWD/.venv/bin/python" -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
cmake --build build-v2 --parallel "${BUILD_JOBS:-4}"
start_db
AI_ENABLED=false python -m libracore_api.database
AI_ENABLED=false python -m libracore_api.database --create-test --test
AI_ENABLED=false python -m libracore_api.seed
printf 'Setup complete. Run ./scripts/test_v2.sh\n'
