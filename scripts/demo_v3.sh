#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/v3_common.sh"
v3_activate
export LLM_PROVIDER=mock RUN_LIVE_AI_TESTS=false
python -m libracore_api.v3.bootstrap --demo
exec ./scripts/dev_v3.sh
