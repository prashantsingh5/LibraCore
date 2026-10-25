#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
activate_project
export LLM_PROVIDER=mock RUN_LIVE_AI_TESTS=false
start_db
python -m libracore_api.database --create-test --test
python -m libracore_api.benchmark "${1:-10000}" --reset-test
