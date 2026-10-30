#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/v3_common.sh"
v3_activate
export AI_ENABLED=false
python -m libracore_api.v3.benchmark "${1:-10000}"
