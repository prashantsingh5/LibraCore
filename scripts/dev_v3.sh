#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/v3_common.sh"
v3_activate
# One API worker: in-memory assistant proposals and rate limits are intentionally single-node.
python -m libracore_api.v3.run & backend_pid=$!
trap 'kill "$backend_pid" 2>/dev/null || true' EXIT INT TERM
npm --prefix frontend run dev
