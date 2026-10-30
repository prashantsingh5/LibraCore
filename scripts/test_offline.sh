#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
activate_project
export LLM_PROVIDER=mock RUN_LIVE_AI_TESTS=false LIBRACORE_RUN_DB_TESTS=0
ctest --test-dir build-v2 --output-on-failure
python -m pytest -ra -m 'not database'
